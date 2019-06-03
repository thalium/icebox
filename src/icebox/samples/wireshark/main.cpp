#define FDP_MODULE "wireshark"
#include "pcap.hpp"
#include <icebox/callstack.hpp>
#include <icebox/core.hpp>
#include <icebox/log.hpp>
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

    typedef struct _MDL
    {
        struct _MDL* Next;
        uint16_t     Size;
        uint16_t     MdlFlags;
        void*        Process;
        void* MappedSystemVa; /* see creators for field size annotations. */
        void* StartVa;        /* see creators for validity; could be address 0.  */
        uint32_t ByteCount;
        uint32_t ByteOffset;
    } MDL, *PMDL;

    typedef struct _NET_BUFFER* PNET_BUFFER;

    typedef struct _NET_BUFFER
    {
        PNET_BUFFER Next;
        PMDL        CurrentMdl;
        uint32_t    CurrentMdlOffset;
        uint32_t    reserved;
        uint32_t    stDataLength;
        PMDL        MdlChain;
        uint32_t    DataOffset;
    } NET_BUFFER, *PNET_BUFFER;

    typedef struct _NET_BUFFER_LIST* PNET_BUFFER_LIST;

    typedef struct _NET_BUFFER_LIST
    {
        PNET_BUFFER_LIST Next;           // Next NetBufferList in the chain
        PNET_BUFFER      FirstNetBuffer; // First NetBuffer on this NetBufferList
        void*            Context;
        PNET_BUFFER_LIST ParentNetBufferList;
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
    } NET_BUFFER_LIST, *PNET_BUFFER_LIST;

    static std::vector<uint8_t> readNetBufferList(core::Core& core, uint64_t netBuffListAddress)
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

    static int capture(core::Core& core, std::string capture_path)
    {
        std::map<uint64_t, core::Breakpoint> user_bps;
        pcap::FileWriterNG                   pcap;
        sym::Loader kernel_sym(core);
        uint64_t bp_id = 0;

        const auto kernel_callstack = callstack::make_callstack_nt(core);
        if(!kernel_callstack)
            return -1;

        const auto user_callstack = callstack::make_callstack_nt(core);
        if(!user_callstack)
            return -1;
        //
        // ndis!NdisSendNetBufferLists
        //
        const auto ndisCallSendHandler = kernel_sym.symbols().symbol("ndis", "NdisSendNetBufferLists");
        if(!ndisCallSendHandler)
            return FAIL(-1, "unable to set a BP on ndis!NdisSendNetBufferLists");

        const auto bp_send = core.state.set_breakpoint("NdisSendNetBufferLists", *ndisCallSendHandler, [&]
        {
            const auto netBufferLists = core.os->read_arg(1);
            const auto curProc        = core.os->proc_current();
            const auto procName       = core.os->proc_name(*curProc);

            const auto data = readNetBufferList(core, (*netBufferLists).val);

            std::string comment(*procName);
            comment += "(" + std::to_string(core.os->proc_id(*curProc));
            comment += ")\n";
            const auto packet = new pcap::Packet(data);
            if(!packet)
                return -1;
            pcap.addPacket(packet);

            auto time = std::chrono::high_resolution_clock::now();
            packet->setTime(std::chrono::duration_cast<std::chrono::nanoseconds>(time.time_since_epoch()).count() / 1000);

            LOG(INFO, " => {} bytes SENT", data.size());
            bool bAddComment = false;
            kernel_callstack->get_callstack(*curProc, [&](callstack::callstep_t callstep)
            {
                if(!core.os->is_kernel_address(callstep.addr))
                {
                    bAddComment = true;
                    //
                    // let's capture the user-callstack
                    //
                    struct Private
                    {
                        core::Core&                           core_;
                        pcap::Packet*                         packet_;
                        uint64_t                              id;
                        std::map<uint64_t, core::Breakpoint>& bps;
                    } p{core, packet, bp_id, user_bps};

                    const auto bp   = core.state.set_breakpoint("NdisSendNetBufferLists user return", callstep.addr, [=]
                    {
                        auto comment = packet->getComment();
                        p.bps.erase(p.id);
                        const auto proc = p.core_.os->proc_current();
                        // sym::Loader user_sym(p.core_, *proc);
                        user_callstack->get_callstack(*proc, [&](callstack::callstep_t step)
                        {
                            LOG(INFO, "{:#x}", step.addr);
                            /*
                            const auto cursor = user_sym.symbols().find(step.addr);
                            if(!cursor)
                            {
                                LOG(INFO, "{:#x}", step.addr);
                                comment += std::to_string(step.addr);
                            }
                            else
                            {
                                LOG(INFO, "{}", sym::to_string(*cursor).data());
                                comment += sym::to_string(*cursor).data();
                                comment += "\n";
                            }
                            */
                            return WALK_NEXT;
                        });
                        packet->setComment(comment);
                        return 0;
                    });
                    user_bps[bp_id] = bp;
                    bp_id++;
                    return WALK_STOP;
                }
                const auto cursor = kernel_sym.symbols().find(callstep.addr);
                if(!cursor)
                {
                    LOG(INFO, "{:#x}", callstep.addr);
                    comment += std::to_string(callstep.addr);
                }
                else
                {
                    comment += sym::to_string(*cursor).data();
                    comment += "\n";
                    LOG(INFO, "{}", sym::to_string(*cursor).data());
                }
                return WALK_NEXT;
            });
            if(bAddComment)
                packet->setComment(comment);
            return 0;
        });
        //
        // ndis!NdisReturnNetBufferLists
        //
        const auto NdisReturnNetBufferLists = kernel_sym.symbols().symbol("ndis", "NdisMIndicateReceiveNetBufferLists");
        if(!NdisReturnNetBufferLists)
            return FAIL(-1, "unable to et a BP on ndis!NdisMIndicateReceiveNetBufferLists");

        const auto bp_recv = core.state.set_breakpoint("NdisMIndicateReceiveNetBufferLists", *NdisReturnNetBufferLists, [&]
        {
            const auto netBufferLists = core.os->read_arg(1);

            const auto data      = readNetBufferList(core, (*netBufferLists).val);
            pcap::Packet* packet = new pcap::Packet(data);
            if(!packet)
                return -1;
            auto time = std::chrono::high_resolution_clock::now();
            packet->setTime(std::chrono::duration_cast<std::chrono::nanoseconds>(time.time_since_epoch()).count() / 1000);
            pcap.addPacket(packet);
            LOG(INFO, " <= {} bytes RECEIVED", data.size());
            return 0;
        });

        while(true)
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
    {
        return FAIL(-1, "unable to start core at {}", name.data());
    }

    capture(core, capture_path);
    //
    // resume VM
    //
    core.state.pause();
    core.state.resume();
    return 0;
}
