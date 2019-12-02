#define FDP_MODULE "wireshark"
#include "pcap.hpp"

#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/nt/nt_types.hpp>
#include <icebox/os.hpp>
#include <icebox/reader.hpp>
#include <icebox/utils/path.hpp>
#include <icebox/utils/pe.hpp>

#include <map>
#include <thread>

namespace
{

    struct MDL
    {
        MDL*     Next;
        uint16_t Size;
        uint16_t MdlFlags;
        void*    Process;
        void*    MappedSystemVa; // see creators for field size annotations.
        void*    StartVa;        // see creators for validity; could be address 0.
        uint32_t ByteCount;
        uint32_t ByteOffset;
    };

    struct NET_BUFFER
    {
        NET_BUFFER* Next;
        MDL*        CurrentMdl;
        uint32_t    CurrentMdlOffset;
        uint32_t    reserved;
        uint32_t    stDataLength;
        MDL*        MdlChain;
        uint32_t    DataOffset;
    };

    struct NET_BUFFER_LIST
    {
        NET_BUFFER_LIST* Next;           // Next NetBufferList in the chain
        NET_BUFFER*      FirstNetBuffer; // First NetBuffer on this NetBufferList
        void*            Context;
        NET_BUFFER_LIST* ParentNetBufferList;
        void*            NdisPoolHandle;
        void*            NdisReserved[2];
        void*            ProtocolReserved[4];
        void*            MiniportReserved[2];
        void*            Scratch;
        void*            SourceHandle;
        uint32_t         NblFlags;       // public flags
        int32_t          ChildRefCount;
        uint32_t         Flags;          // private flags used by NDIs, protocols, miniport, etc.
        union
        {
            uint32_t Status;
            uint32_t NdisReserved2;
        };
    };

    static std::vector<uint8_t> read_NetBufferList(core::Core& core, uint64_t netBuffListAddress)
    {
        std::vector<uint8_t> invalid;

        const auto reader = reader::make(core);
        NET_BUFFER_LIST      netBufferList;
        NET_BUFFER           netBuffer;
        MDL                  mdl;
        std::vector<uint8_t> data;

        for(uint64_t addr = netBuffListAddress; addr != 0; addr = reinterpret_cast<uint64_t>(netBufferList.Next))
        {
            auto ok = reader.read_all(&netBufferList, addr, sizeof netBufferList);
            if(!ok)
                return invalid;

            ok = reader.read_all(&netBuffer, (uint64_t) netBufferList.FirstNetBuffer, sizeof netBuffer);
            if(!ok)
                return invalid;

            uint32_t mdlOffset  = netBuffer.DataOffset;
            uint32_t dataOffset = 0;
            uint32_t dataSize   = netBuffer.stDataLength;
            uint32_t size       = 0;
            auto mdlAddress     = (uint64_t) netBuffer.CurrentMdl;

            data.resize(data.size() + dataSize);
            do
            {
                ok = reader.read_all(&mdl, mdlAddress, sizeof mdl);
                if(!ok)
                    return invalid;
                size = std::min(mdl.ByteCount - mdlOffset, dataSize);
                ok   = reader.read_all(&data[dataOffset], (uint64_t) mdl.MappedSystemVa + mdlOffset, size);
                if(!ok)
                    return invalid;

                dataSize -= size;
                dataOffset += size;
                mdlAddress = (uint64_t) mdl.Next;
                mdlOffset  = 0;
            } while(mdlAddress && dataSize);
        }
        return data;
    }

    static bool is_wow64_emulated(core::Core& core, const proc_t proc, const uint64_t addr)
    {
        const auto mod = modules::find(core, proc, addr);
        if(!mod)
            return false;

        const auto mod_name = modules::name(core, proc, *mod);
        if(!mod_name)
            return false;

        const auto filename = path::filename(*mod_name);
        return filename == "wow64cpu.dll";
    }

    static opt<callstacks::context_t> get_saved_wow64_ctx(core::Core& core, proc_t proc)
    {
        const auto reader = reader::make(core, proc);

        const auto cs            = registers::read(core, reg_e::cs);
        const auto is_kernel_ctx = cs && 0x0F == 0x00;
        const auto teb           = registers::read_msr(core, is_kernel_ctx ? msr_e::kernel_gs_base : msr_e::gs_base);
        const auto TEB_TlsSlots  = symbols::struc_offset(core, symbols::kernel, "nt", "_TEB", "TlsSlots");
        if(!TEB_TlsSlots)
            return {};

        const auto TlsSlot       = teb + *TEB_TlsSlots + 8;
        const auto WOW64_CONTEXT = reader.read(TlsSlot);
        if(!WOW64_CONTEXT)
            return {};
        LOG(INFO, "TEB is: 0x%" PRIx64 ", r12 is: 0x%" PRIx64 ", r13 is: 0x%" PRIx64, teb, TlsSlot, *WOW64_CONTEXT);

        auto offset = 4; // FIXME: why 4 on 10240 ?
        nt_types::_WOW64_CONTEXT wow64ctx;
        const auto ok = reader.read_all(&wow64ctx, *WOW64_CONTEXT + offset, sizeof wow64ctx);
        if(!ok)
            return {};

        const auto ebp   = static_cast<uint64_t>(wow64ctx.Ebp);
        const auto eip   = static_cast<uint64_t>(wow64ctx.Eip);
        const auto esp   = static_cast<uint64_t>(wow64ctx.Esp);
        const auto segcs = static_cast<uint64_t>(wow64ctx.SegCs);
        return callstacks::context_t{eip, esp, ebp, segcs, flags::x86};
    }

    static void load_proc_symbols(core::Core& core, proc_t proc, flags_t flags)
    {
        modules::list(core, proc, [&](mod_t mod)
        {
            if(!os::check_flags(mod.flags, flags))
                return walk_e::next;

            const auto inserted = symbols::load_module(core, proc, mod);
            if(!inserted)
                return walk_e::next;

            return walk_e::next;
        });
    }

    using Breakpoints = std::map<int, state::Breakpoint>;

    struct Private
    {
        core::Core&      core;
        pcap::metadata_t meta;
        int              id;
        bool             is_wow64cpu;
        pcap::Writer*    pcap;
        Breakpoints&     bps;
    };

    static std::string get_callstep_name(core::Core& core, proc_t proc, uint64_t addr)
    {
        const auto symbol = symbols::find(core, proc, addr);
        return symbols::to_string(symbol) + "\n";
    }

    static void get_user_callstack32(core::Core& core, pcap::metadata_t& meta, proc_t proc)
    {
        load_proc_symbols(core, proc, flags::x86);

        const auto ctx = get_saved_wow64_ctx(core, proc);
        if(!ctx)
            return;

        auto callers = std::vector<callstacks::caller_t>(128);
        const auto n = callstacks::read_from(core, &callers[0], callers.size(), proc, *ctx);
        for(size_t i = 0; i < n; ++i)
            meta.comment += get_callstep_name(core, proc, callers[i].addr);
    }

    static void get_user_callstack64(core::Core& core, pcap::metadata_t& meta, proc_t proc)
    {
        load_proc_symbols(core, proc, flags::x64);

        auto callers = std::vector<callstacks::caller_t>(128);
        const auto n = callstacks::read(core, &callers[0], callers.size(), proc);
        for(size_t i = 0; i < n; ++i)
            meta.comment += get_callstep_name(core, proc, callers[i].addr);
    }

    static bool break_in_userland(Private& p, proc_t proc, uint64_t addr, const std::vector<uint8_t>& data)
    {
        const auto thread = threads::current(p.core);
        if(!thread)
            return false;

        const auto bp = state::break_on_thread(p.core, "NdisSendNetBufferLists user return", *thread, addr, [=]
        {
            auto meta = p.meta;
            if(p.is_wow64cpu)
                get_user_callstack32(p.core, meta, proc);
            else
                get_user_callstack64(p.core, meta, proc);

            LOG(INFO, "Callstack: %s", meta.comment.data());
            p.pcap->add_packet(meta, &data[0], data.size());
            p.bps.erase(p.id);
            return 0;
        });
        p.bps[p.id]   = bp;
        return true;
    }

    static int capture(core::Core& core, const std::string& capture_path)
    {
        Breakpoints  user_bps;
        pcap::Writer pcap;
        int bp_id = 0;

        symbols::load_drivers(core);
        // ndis!NdisSendNetBufferLists
        const auto NdisSendNetBufferLists = symbols::address(core, symbols::kernel, "ndis", "NdisSendNetBufferLists");
        if(!NdisSendNetBufferLists)
            return FAIL(-1, "unable to set a BP on ndis!NdisSendNetBufferLists");

        const auto bp_send = state::break_on(core, "NdisSendNetBufferLists", *NdisSendNetBufferLists, [&]
        {
            const auto proc = process::current(core);
            if(!proc)
                return -1;

            const auto netBufferLists = functions::read_arg(core, 1);
            if(!netBufferLists)
                return -1;

            const auto data = read_NetBufferList(core, netBufferLists->val);

            const auto proc_name = process::name(core, *proc);
            if(!proc_name)
                return -1;

            pcap::metadata_t meta;
            meta.comment   = *proc_name + "(" + std::to_string(process::pid(core, *proc)) + ")\n";
            const auto now = std::chrono::high_resolution_clock::now();
            meta.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count() / 1000;

            auto continue_to_userland = false;
            auto callers              = std::vector<callstacks::caller_t>(128);
            const auto n              = callstacks::read(core, &callers[0], callers.size(), *proc);
            for(size_t i = 0; i < n; ++i)
            {
                const auto addr = callers[i].addr;
                if(!os::is_kernel_address(core, addr))
                {
                    continue_to_userland   = true;
                    const auto is_wow64cpu = is_wow64_emulated(core, *proc, addr);
                    Private p              = {core, meta, bp_id, is_wow64cpu, &pcap, user_bps};
                    break_in_userland(p, *proc, addr, data);
                    bp_id++;
                    break;
                }

                meta.comment += get_callstep_name(core, *proc, addr);
            }

            if(!continue_to_userland)
                pcap.add_packet(meta, &data[0], data.size());

            return 0;
        });

        // ndis!NdisReturnNetBufferLists
        const auto NdisReturnNetBufferLists = symbols::address(core, symbols::kernel, "ndis", "NdisMIndicateReceiveNetBufferLists");
        if(!NdisReturnNetBufferLists)
            return FAIL(-1, "unable to et a BP on ndis!NdisMIndicateReceiveNetBufferLists");

        const auto bp_recv = state::break_on(core, "NdisMIndicateReceiveNetBufferLists", *NdisReturnNetBufferLists, [&]
        {
            const auto netBufferLists = functions::read_arg(core, 1);
            if(!netBufferLists)
                return -1;

            const auto data = read_NetBufferList(core, (*netBufferLists).val);
            pcap::metadata_t meta;
            auto now       = std::chrono::high_resolution_clock::now();
            meta.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count() / 1000;
            pcap.add_packet(meta, &data[0], data.size());
            return 0;
        });

        const auto now = std::chrono::high_resolution_clock::now();
        const auto end = now + std::chrono::seconds(30);
        while(std::chrono::high_resolution_clock::now() < end)
        {
            state::resume(core);
            state::wait(core);
        }
        pcap.write(capture_path.data());

        return 0;
    }
}
int main(int argc, char** argv)
{
    logg::init(argc, argv);
    if(argc != 3)
        return FAIL(-1, "usage: wireshark <name> <path_to_capture_file>");

    const auto name = std::string{argv[1]};
    LOG(INFO, "starting on: %s", name.data());
    const auto capture_path = std::string{argv[2]};
    LOG(INFO, "capture path: %s", capture_path.data());

    const auto core = core::attach(name);
    if(!core)
        return FAIL(-1, "unable to start core at %s", name.data());

    state::pause(*core);
    const auto ret = capture(*core, capture_path);
    state::resume(*core);
    return ret;
}
