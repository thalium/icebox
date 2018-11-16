#include "syscall_tracer.hpp"

#define FDP_MODULE "syscall_tracer"
#include "log.hpp"
#include "os.hpp"

#include "utils/sanitizer.hpp"
#include "utils/json.hpp"
#include "utils/file_helper.hpp"

#include <unordered_map>

namespace
{
    using json = nlohmann::json;

    struct callstack_frame_t
    {
        uint64_t idx;
        uint64_t size;
    };

    struct bp_trigger_info_t
    {
        callstack_frame_t cs_frame;
        uint64_t          args_idx;
    };
}

struct syscall_tracer::SyscallPlugin::Data
{
    proc_t                             target;
    uint64_t                           trigger_nbr;
    std::vector<callstack::callstep_t> callsteps;
    std::vector<bp_trigger_info_t>     bp_trigger_infos;
    json                               args;
};

namespace
{
    json create_calltree(core::Core& core, proc_t target, const std::vector<callstack::callstep_t>& callsteps, std::vector<bp_trigger_info_t>& bp_trigger_infos, json& args)
    {
        json calltree;
        std::unordered_map<uint64_t, std::vector<bp_trigger_info_t>> intermediate_tree;
        std::vector<bp_trigger_info_t>                               end_nodes;

        // Prepare nodes
        for (auto& info : bp_trigger_infos)
        {
            if(info.cs_frame.size == 0){
                end_nodes.push_back(info);
                continue;
            }

            uint64_t addr = callsteps[info.cs_frame.idx + info.cs_frame.size - 1].current_addr;

            if(intermediate_tree.find(addr) == intermediate_tree.end())
                intermediate_tree[addr] = std::vector<bp_trigger_info_t>();

            info.cs_frame.size--;
            intermediate_tree[addr].push_back(info);
        }

        // Call function recursively to create subtrees
        for (auto& it : intermediate_tree)
        {
            auto cursor = core.sym.find(it.first);
            if (!cursor)
                cursor = sym::Cursor{sanitizer::sanitize_filename(""), "<nosymbol>", it.first};
                // cursor = sym::Cursor{sanitizer::sanitize_filename(*(core.os->mod_name(target, callsteps[it.first].mod))), "<nosymbol>",
                //                                                     callsteps[it.first].current_addr - core.os->mod_span(target, callsteps[it.first].mod)->addr};
            calltree[sym::to_string(*cursor).data()] = create_calltree(core, target, callsteps, it.second, args);
        }

        // Place args (contained in a json that was made by the observer) on end nodes
        int j = 0;
        for (const auto& end_node : end_nodes)
            calltree["Args"][j++] = args[end_node.args_idx];

        return calltree;
    }

}

syscall_tracer::SyscallPlugin::SyscallPlugin(core::Core& core, pe::Pe& pe)
    : d_(std::make_unique<Data>())
    , core_(core)
    , pe_(pe)
    , syscall_monitor_(core)
{
}

syscall_tracer::SyscallPlugin::~SyscallPlugin()
{
}

bool syscall_tracer::SyscallPlugin::setup(proc_t target)
{
    d_->target = target;
    d_->trigger_nbr = 0;

    callstack_ = callstack::make_callstack_nt(core_, pe_);
    if(!callstack_)
        FAIL(false, "Unable to create callstack object");

    const auto ok = syscall_monitor_.setup(d_->target);
    if(!ok)
        FAIL(false, "Unable to setup syscall_monitor");


    // Register NtWriteFile observer
    syscall_monitor_.register_NtWriteFile([&](nt::HANDLE FileHandle, nt::HANDLE Event, nt::PIO_APC_ROUTINE ApcRoutine, nt::PVOID ApcContext,
                                                nt::PIO_STATUS_BLOCK IoStatusBlock, nt::PVOID Buffer, nt::ULONG Length,
                                                nt::PLARGE_INTEGER ByteOffsetm, nt::PULONG Key)
    {
        LOG(INFO, "NtWriteFile : %" PRIx64 " - %" PRIx64 " - %"  PRIx64 " - %" PRIx64 " - %"  PRIx64 " - %" PRIx64 " - %"  PRIx64 " - %" PRIx64 " - %" PRIx64,
                FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffsetm, Key);

        std::vector<uint8_t> buf;
        buf.resize(Length);
        auto ok = core_.mem.virtual_read(&buf[0], Buffer, Length);
        if(!ok)
            return 1;

        char* dst = reinterpret_cast<char*>(&buf[0]);
        dst[Length-1] = '\0';

        d_->args[d_->trigger_nbr]["FileHandle"] = FileHandle;
        d_->args[d_->trigger_nbr]["Buffer"]     = dst;

        private_get_callstack();
        d_->trigger_nbr++;
        return 0;
    });

    // Register NtClose observer
    syscall_monitor_.register_NtClose([&](nt::HANDLE paramHandle)
    {
        LOG(INFO, "NtClose : %" PRIx64, paramHandle);

        d_->args[d_->trigger_nbr]["Handle"] = paramHandle;

        //private_get_callstack();
        d_->trigger_nbr++;
        return 0;
    });

    return true;
}

bool syscall_tracer::SyscallPlugin::private_get_callstack()
{
    const auto cs_depth = 10;

    uint64_t idx = d_->callsteps.size();
    uint64_t cs_size = 0;
    const auto rip = core_.regs.read(FDP_RIP_REGISTER);
    const auto rsp = core_.regs.read(FDP_RSP_REGISTER);
    const auto rbp = core_.regs.read(FDP_RBP_REGISTER);
    callstack_->get_callstack(d_->target, *rip, *rsp, *rbp, [&](callstack::callstep_t cstep)
    {
        d_->callsteps.push_back(cstep);

        cs_size++;
        if (cs_size<cs_depth)
            return WALK_NEXT;

        return WALK_STOP;
    });

    d_->bp_trigger_infos.push_back(bp_trigger_info_t{idx, cs_size, d_->trigger_nbr});
    return true;
}

bool syscall_tracer::SyscallPlugin::produce_output(std::string file_name)
{
    std::vector<bp_trigger_info_t> frames = d_->bp_trigger_infos;
    const json output = create_calltree(core_, d_->target, d_->callsteps, frames, d_->args);

    //Dump output
    file_helper::write_file(file_name, output.dump());

    return true;
}
