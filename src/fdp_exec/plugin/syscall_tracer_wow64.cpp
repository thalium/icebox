#include "syscall_tracer.hpp"

#define FDP_MODULE "syscall_tracer"
#include "log.hpp"
#include "os.hpp"

#include "callstack.hpp"
#include "monitor/syscallswow64.gen.hpp"
#include "nt/objects_nt.hpp"
#include "reader.hpp"
#include "utils/file.hpp"
#include "utils/fnview.hpp"
#include "utils/pe.hpp"

#include <nlohmann/json.hpp>

namespace
{
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

    using json        = nlohmann::json;
    using Callsteps   = std::vector<callstack::callstep_t>;
    using Triggers    = std::vector<bp_trigger_info_t>;
    using SyscallData = syscall_tracer::SyscallPluginWow64::Data;
}

struct syscall_tracer::SyscallPluginWow64::Data
{
    Data(core::Core& core);

    core::Core&                            core_;
    monitor::syscallswow64                 syscalls_;
    std::shared_ptr<callstack::ICallstack> callstack_;
    Callsteps                              callsteps_;
    Triggers                               triggers_;
    json                                   args_;
    proc_t                                 target_;
    uint64_t                               nb_triggers_;
};

syscall_tracer::SyscallPluginWow64::Data::Data(core::Core& core)
    : core_(core)
    , syscalls_(core, "wntdll")
    , target_()
    , nb_triggers_()
{
}

syscall_tracer::SyscallPluginWow64::SyscallPluginWow64(core::Core& core)
    : d_(std::make_unique<Data>(core))
{
}

syscall_tracer::SyscallPluginWow64::~SyscallPluginWow64() = default;

namespace
{
    json create_calltree(core::Core& core, Triggers& triggers, json& args, proc_t target, const Callsteps& callsteps)
    {
        using TriggersHash = std::unordered_map<uint64_t, Triggers>;
        json         calltree;
        Triggers     end_nodes;
        TriggersHash intermediate_tree;

        // Prepare nodes
        for(auto& info : triggers)
        {
            if(info.cs_frame.size == 0)
            {
                end_nodes.push_back(info);
                continue;
            }

            const auto addr = callsteps[info.cs_frame.idx + info.cs_frame.size - 1].addr;
            if(intermediate_tree.find(addr) == intermediate_tree.end())
                intermediate_tree[addr] = {};

            info.cs_frame.size--;
            intermediate_tree[addr].push_back(info);
        }

        // Call function recursively to create subtrees
        for(auto& it : intermediate_tree)
        {
            auto cursor = core.sym.find(it.first);
            if(!cursor)
                cursor = sym::Cursor{"_", "_>", it.first};

            calltree[sym::to_string(*cursor).data()] = create_calltree(core, it.second, args, target, callsteps);
        }

        // Place args (contained in a json that was made by the observer) on end nodes
        size_t i = 0;
        for(const auto& end_node : end_nodes)
            calltree["Args"][i++] = args[end_node.args_idx];

        return calltree;
    }

    bool private_get_callstack(SyscallData& d)
    {
        constexpr size_t cs_max_depth = 70;

        const auto idx = d.callsteps_.size();
        size_t cs_size = 0;
        const auto rip = d.core_.regs.read(FDP_RIP_REGISTER);
        const auto rsp = d.core_.regs.read(FDP_RSP_REGISTER);
        const auto rbp = d.core_.regs.read(FDP_RBP_REGISTER);
        d.callstack_->get_callstack(d.target_, {rip, rsp, rbp}, [&](callstack::callstep_t cstep)
        {
            d.callsteps_.push_back(cstep);

            if(false)
            {
                auto cursor = d.core_.sym.find(cstep.addr);
                if(!cursor)
                    cursor = sym::Cursor{"_", "_", cstep.addr};

                LOG(INFO, "{} - {}", cs_size, sym::to_string(*cursor).data());
            }

            cs_size++;
            if(cs_size < cs_max_depth)
                return WALK_NEXT;

            return WALK_STOP;
        });

        d.triggers_.push_back(bp_trigger_info_t{{idx, cs_size}, d.nb_triggers_});
        return true;
    }
}

bool syscall_tracer::SyscallPluginWow64::setup(proc_t target)
{
    d_->target_      = target;
    d_->nb_triggers_ = 0;

    d_->callstack_ = callstack::make_callstack_nt(d_->core_);
    if(!d_->callstack_)
        FAIL(false, "Unable to create callstack object");

    d_->syscalls_.register_all(target, [=]()
    {
        private_get_callstack(*d_);
        d_->nb_triggers_++;
        return 0;
    });

    return true;
}

bool syscall_tracer::SyscallPluginWow64::generate(const fs::path& file_name)
{
    const auto output = create_calltree(d_->core_, d_->triggers_, d_->args_, d_->target_, d_->callsteps_);
    const auto dump   = output.dump();
    const auto ok     = file::write(file_name, dump.data(), dump.size());
    return !!ok;
}
