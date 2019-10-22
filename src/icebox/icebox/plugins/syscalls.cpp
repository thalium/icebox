#include "syscalls.hpp"

#define FDP_MODULE "syscalls"
#include "log.hpp"
#include "os.hpp"

#include "callstack.hpp"
#include "nt/objects_nt.hpp"
#include "reader.hpp"
#include "tracer/syscalls.gen.hpp"
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

    using json      = nlohmann::json;
    using Callsteps = std::vector<callstack::callstep_t>;
    using Triggers  = std::vector<bp_trigger_info_t>;
    using Data      = plugins::Syscalls::Data;
}

struct plugins::Syscalls::Data
{
    Data(core::Core& core, sym::Symbols& syms, proc_t proc);

    bool setup();

    core::Core&                            core_;
    sym::Symbols&                          syms_;
    proc_t                                 proc_;
    nt::syscalls                           syscalls_;
    nt::ObjectNt                           objects_;
    std::shared_ptr<callstack::ICallstack> callstack_;
    Callsteps                              callsteps_;
    Triggers                               triggers_;
    json                                   args_;
    uint64_t                               nb_triggers_;
};

plugins::Syscalls::Data::Data(core::Core& core, sym::Symbols& syms, proc_t proc)
    : core_(core)
    , syms_(syms)
    , proc_(proc)
    , syscalls_(core, syms, "ntdll")
    , objects_(core, proc)
    , nb_triggers_()
{
}

plugins::Syscalls::Syscalls(core::Core& core, sym::Symbols& syms, proc_t proc)
    : d_(std::make_unique<Data>(core, syms, proc))
{
    d_->setup();
}

plugins::Syscalls::~Syscalls() = default;

namespace
{
    static json create_calltree(core::Core& core, sym::Symbols& syms, Triggers& triggers, json& args, proc_t target, const Callsteps& callsteps)
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
            auto cursor = syms.find(it.first);
            if(!cursor)
                cursor = sym::Cursor{"_", "_>", it.first};

            calltree[sym::to_string(*cursor).data()] = create_calltree(core, syms, it.second, args, target, callsteps);
        }

        // Place args (contained in a json that was made by the observer) on end nodes
        size_t i = 0;
        for(const auto& end_node : end_nodes)
            calltree["Args"][i++] = args[end_node.args_idx];

        return calltree;
    }

    static bool private_get_callstack(Data& d)
    {
        constexpr size_t cs_max_depth = 70;

        const auto idx = d.callsteps_.size();
        size_t cs_size = 0;
        d.callstack_->get_callstack(d.proc_, [&](callstack::callstep_t cstep)
        {
            d.callsteps_.push_back(cstep);

            if(false)
            {
                auto cursor = d.syms_.find(cstep.addr);
                if(!cursor)
                    cursor = sym::Cursor{"_", "_", cstep.addr};

                LOG(INFO, "%zd - %s", cs_size, sym::to_string(*cursor).data());
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

bool Data::setup()
{
    nb_triggers_ = 0;

    callstack_ = callstack::make_callstack_nt(core_);
    if(!callstack_)
        return FAIL(false, "unable to create callstack object");

    syscalls_.register_NtWriteFile(proc_, [=](nt::HANDLE FileHandle, nt::HANDLE /*Event*/, nt::PIO_APC_ROUTINE /*ApcRoutine*/, nt::PVOID /*ApcContext*/,
                                              nt::PIO_STATUS_BLOCK /*IoStatusBlock*/, nt::PVOID Buffer, nt::ULONG Length,
                                              nt::PLARGE_INTEGER /*ByteOffsetm*/, nt::PULONG /*Key*/)
    {
        std::vector<char> buf(Length);
        const auto reader = reader::make(core_, proc_);
        const auto ok     = reader.read(&buf[0], Buffer, Length);
        if(!ok)
            return 1;

        buf[Length - 1]         = 0;
        const auto obj          = objects_.obj_read(FileHandle);
        const auto obj_typename = objects_.obj_type(*obj);
        const auto file         = objects_.file_read(FileHandle);
        const auto obj_filename = objects_.file_name(*file);
        const auto device_obj   = objects_.file_device(*file);
        const auto driver_obj   = objects_.device_driver(*device_obj);
        const auto driver_name  = objects_.driver_name(*driver_obj);
        LOG(INFO, " File handle; 0x%" PRIx64 ", typename : %s, filename : %s, driver_name : %s", FileHandle, obj_typename->data(), obj_filename->data(), driver_name->data());

        args_[nb_triggers_]["FileName"] = obj_filename->data();
        args_[nb_triggers_]["Buffer"]   = buf;
        private_get_callstack(*this);
        nb_triggers_++;
        return 0;
    });

    syscalls_.register_NtClose(proc_, [=](nt::HANDLE paramHandle)
    {
        args_[nb_triggers_]["Handle"] = paramHandle;
        private_get_callstack(*this);
        nb_triggers_++;
        return 0;
    });

    syscalls_.register_NtDeviceIoControlFile(proc_, [=](nt::HANDLE /*FileHandle*/, nt::HANDLE /*Event*/, nt::PIO_APC_ROUTINE /*ApcRoutine*/,
                                                        nt::PVOID /*ApcContext*/, nt::PIO_STATUS_BLOCK /*IoStatusBlock*/, nt::ULONG /*IoControlCode*/,
                                                        nt::PVOID /*InputBuffer*/, nt::ULONG /*InputBufferLength*/, nt::PVOID /*OutputBuffer*/,
                                                        nt::ULONG /*OutputBufferLength*/)
    {
        return 0;
    });

    return true;
}

bool plugins::Syscalls::generate(const fs::path& file_name)
{
    auto& d           = *d_;
    const auto output = create_calltree(d.core_, d.syms_, d.triggers_, d.args_, d.proc_, d.callsteps_);
    const auto dump   = output.dump();
    const auto ok     = file::write(file_name, dump.data(), dump.size());
    return !!ok;
}
