#define FDP_MODULE "wireshark"
#include "pcap.hpp"
#include <icebox/callstack.hpp>
#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/nt/nt_types.hpp>
#include <icebox/os.hpp>
#include <icebox/plugins/sym_loader.hpp>
#include <icebox/reader.hpp>
#include <icebox/utils/fnview.hpp>
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
        void* MappedSystemVa; /* see creators for field size annotations. */
        void* StartVa;        /* see creators for validity; could be address 0.  */
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
        NET_BUFFER_LIST netBufferList;
        NET_BUFFER      netBuffer;
        MDL             mdl;
        auto netBufferListsAddr = netBuffListAddress;
        std::vector<uint8_t> data;

        while(netBufferListsAddr)
        {
            auto ok = reader.read(&netBufferList, netBufferListsAddr, sizeof(netBufferList));
            if(!ok)
                return invalid;

            netBufferListsAddr = (uint64_t) netBufferList.Next;

            ok = reader.read(&netBuffer, (uint64_t) netBufferList.FirstNetBuffer, sizeof(netBuffer));
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
                ok = reader.read(&mdl, mdlAddress, sizeof(mdl));
                if(!ok)
                    return invalid;
                size = std::min(mdl.ByteCount - mdlOffset, dataSize);
                ok   = reader.read(&data[dataOffset], (uint64_t) mdl.MappedSystemVa + mdlOffset, size);
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
        const auto mod = core.os->mod_find(proc, addr);
        if(!mod)
            return WALK_NEXT;

        const auto mod_name = core.os->mod_name(proc, *mod);
        if(!mod_name)
            return WALK_NEXT;

        const auto filename = path::filename(*mod_name);
        return filename == "wow64cpu.dll";
    }

    static opt<callstack::context_t> get_saved_wow64_ctx(core::Core& core, proc_t proc)
    {
        const auto reader = reader::make(core, proc);

        const auto cs            = core.regs.read(FDP_CS_REGISTER);
        const auto is_kernel_ctx = cs && 0x0F == 0x00;
        const auto teb           = core.regs.read(is_kernel_ctx ? MSR_KERNEL_GS_BASE : MSR_GS_BASE);

        auto& ksyms             = core.os->kernel_symbols();
        const auto TEB_TlsSlots = ksyms.struc_offset("nt", "_TEB", "TlsSlots");
        if(!TEB_TlsSlots)
            return {};

        const auto TlsSlot       = teb + *TEB_TlsSlots + 8;
        const auto WOW64_CONTEXT = reader.read(TlsSlot);
        if(!WOW64_CONTEXT)
            return {};
        LOG(INFO, "TEB is: {:#x}, r12 is: {:#x}, r13 is: {:#x}", teb, TlsSlot, *WOW64_CONTEXT);

        auto offset        = 4; // FIXME: why 4 on 10240 ?
        const auto off_ebp = offsetof(nt_types::_WOW64_CONTEXT, Ebp) + offset;
        const auto ebp     = reader.le32(*WOW64_CONTEXT + off_ebp);
        if(!ebp)
            return {};

        const auto off_eip = offsetof(nt_types::_WOW64_CONTEXT, Eip) + offset;
        const auto eip     = reader.le32(*WOW64_CONTEXT + off_eip);
        if(!eip)
            return {};

        const auto off_esp = offsetof(nt_types::_WOW64_CONTEXT, Esp) + offset;
        const auto esp     = reader.le32(*WOW64_CONTEXT + off_esp);
        if(!esp)
            return {};

        const auto zero64 = 0x0000000000000000;
        const auto ip     = zero64 | *eip;
        const auto sp     = zero64 | *esp;
        const auto bp     = zero64 | *ebp;
        return callstack::context_t{ip, sp, bp, 0};
    }

    static void load_proc_symbols(core::Core& core, proc_t proc, sym::Symbols& symbols, flags_e flag)
    {
        const auto reader = reader::make(core, proc);
        core.os->mod_list(proc, [&](mod_t mod)
        {
            if(flag && mod.flags != flag)
                return WALK_NEXT;

            const auto name = core.os->mod_name(proc, mod);
            if(!name)
                return WALK_NEXT;

            const auto span = core.os->mod_span(proc, mod);
            if(!span)
                return WALK_NEXT;

            const auto debug = pe::find_debug_codeview(reader, *span);
            if(!debug)
                return WALK_NEXT;

            std::vector<uint8_t> buffer;
            buffer.resize(debug->size);
            auto ok = reader.read(&buffer[0], debug->addr, debug->size);
            if(!ok)
                return WALK_NEXT;

            const auto filename = path::filename(*name).replace_extension("").generic_string();
            const auto inserted = symbols.insert(filename.data(), *span, &buffer[0], buffer.size());
            if(!inserted)
                return WALK_NEXT;

            return WALK_NEXT;
        });
    }

    struct Private
    {
        core::Core&                      core;
        pcap::Packet                     packet;
        int                              id;
        pcap::FileWriterNG*              pcap;
        std::map<int, core::Breakpoint>& bps;
    };

    using CallStack = std::shared_ptr<callstack::ICallstack>;

    static std::string get_callstep_name(sym::Symbols& syms, uint64_t addr)
    {
        const auto cur = syms.find(addr);
        std::string comment;
        if(!cur)
            comment = std::to_string(addr);
        else
            comment = sym::to_string(*cur).data();

        comment += "\n";
        return comment;
    }

    static void get_user_callstack32(core::Core& core, pcap::Packet& packet, proc_t proc, const CallStack& callstack)
    {
        sym::Symbols symbols;
        load_proc_symbols(core, proc, symbols, FLAGS_32BIT);

        const auto ctx = get_saved_wow64_ctx(core, proc);
        if(!ctx)
            return;

        callstack->get_callstack_from_context(proc, *ctx, FLAGS_32BIT, [&](callstack::callstep_t step)
        {
            packet.meta.comment += get_callstep_name(symbols, step.addr);
            return WALK_NEXT;
        });
    }

    static void get_user_callstack64(core::Core& core, pcap::Packet& packet, proc_t proc, const CallStack& callstack)
    {
        sym::Symbols symbols;
        load_proc_symbols(core, proc, symbols, FLAGS_NONE);

        callstack->get_callstack(proc, [&](callstack::callstep_t step)
        {
            packet.meta.comment += get_callstep_name(symbols, step.addr);
            return WALK_NEXT;
        });
    }

    static int capture(core::Core& core, const std::string& capture_path)
    {
        std::map<int, core::Breakpoint> user_bps;
        pcap::FileWriterNG              pcap;
        sym::Loader kernel_sym(core);
        kernel_sym.drv_listen();
        int bp_id = 0;

        const auto kernel_callstack = callstack::make_callstack_nt(core);
        if(!kernel_callstack)
            return -1;

        const auto user_callstack = callstack::make_callstack_nt(core);
        if(!user_callstack)
            return -1;

        // ndis!NdisSendNetBufferLists
        const auto ndisCallSendHandler = kernel_sym.symbols().symbol("ndis", "NdisSendNetBufferLists");
        if(!ndisCallSendHandler)
            return FAIL(-1, "unable to set a BP on ndis!NdisSendNetBufferLists");

        const auto bp_send = core.state.set_breakpoint("NdisSendNetBufferLists", *ndisCallSendHandler, [&]
        {
            const auto netBufferLists = core.os->read_arg(1);
            const auto curProc        = core.os->proc_current();
            const auto procName       = core.os->proc_name(*curProc);

            pcap::Packet packet;
            packet.data = read_NetBufferList(core, (*netBufferLists).val);

            packet.meta.comment = *procName + "(" + std::to_string(core.os->proc_id(*curProc)) + ")\n";

            const auto now        = std::chrono::high_resolution_clock::now();
            packet.meta.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count() / 1000;

            auto continue_to_userland = false;
            kernel_callstack->get_callstack(*curProc, [&](callstack::callstep_t callstep)
            {
                if(core.os->is_kernel_address(callstep.addr))
                {
                    packet.meta.comment += get_callstep_name(kernel_sym.symbols(), callstep.addr);
                    return WALK_NEXT;
                }

                const auto proc = core.os->proc_current();
                if(!proc)
                    return WALK_STOP;

                const auto is_wow64cpu = is_wow64_emulated(core, *proc, callstep.addr);
                const auto thread      = core.os->thread_current();
                if(!thread)
                    return WALK_STOP;

                continue_to_userland = true;
                Private p            = {core, packet, bp_id, &pcap, user_bps};
                const auto bp        = core.state.set_breakpoint("NdisSendNetBufferLists user return", callstep.addr, *thread, [=]
                {
                    auto packet = p.packet;
                    if(is_wow64cpu)
                        get_user_callstack32(p.core, packet, *proc, user_callstack);
                    else
                        get_user_callstack64(p.core, packet, *proc, user_callstack);

                    LOG(INFO, "Callstack: {}", packet.meta.comment);
                    p.pcap->add_packet(packet);
                    p.bps.erase(p.id);
                    return 0;
                });
                user_bps[bp_id]      = bp;
                bp_id++;

                return WALK_STOP;
            });

            if(!continue_to_userland)
            {
                pcap.add_packet(packet);
                LOG(INFO, "Callstack: {}", packet.meta.comment);
            }

            return 0;
        });

        // ndis!NdisReturnNetBufferLists
        const auto NdisReturnNetBufferLists = kernel_sym.symbols().symbol("ndis", "NdisMIndicateReceiveNetBufferLists");
        if(!NdisReturnNetBufferLists)
            return FAIL(-1, "unable to et a BP on ndis!NdisMIndicateReceiveNetBufferLists");

        const auto bp_recv = core.state.set_breakpoint("NdisMIndicateReceiveNetBufferLists", *NdisReturnNetBufferLists, [&]
        {
            const auto netBufferLists = core.os->read_arg(1);

            pcap::Packet packet;
            packet.data           = read_NetBufferList(core, (*netBufferLists).val);
            auto now              = std::chrono::high_resolution_clock::now();
            packet.meta.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count() / 1000;
            pcap.add_packet(packet);
            return 0;
        });

        const auto now = std::chrono::high_resolution_clock::now();
        const auto end = now + std::chrono::minutes(3);
        while(std::chrono::high_resolution_clock::now() < end)
        {
            core.state.resume();
            core.state.wait();
        }
        pcap.write(capture_path.data());

        return 0;
    }
}
int main(int argc, char** argv)
{
    logg::init(argc, argv);
    if(argc != 3)
        return FAIL(-1, "usage: wireshark <name> <cpature file path>");

    const auto name = std::string{argv[1]};
    LOG(INFO, "starting on: {}", name.data());
    const auto capture_path = std::string{argv[2]};
    LOG(INFO, "capture path: {}", capture_path.data());

    core::Core core;
    const auto ok = core.setup(name);
    if(!ok)
        return FAIL(-1, "unable to start core at {}", name.data());

    capture(core, capture_path);

    // resume VM
    core.state.pause();
    core.state.resume();
    return 0;
}
