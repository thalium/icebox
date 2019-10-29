#include "syscalls.hpp"

#define FDP_MODULE "syscalls"
#include "core.hpp"
#include "log.hpp"
#include "nt/nt_objects.hpp"
#include "reader.hpp"
#include "tracer/syscalls.gen.hpp"
#include "utils/file.hpp"
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
    using Callsteps = std::vector<callstacks::caller_t>;
    using Triggers  = std::vector<bp_trigger_info_t>;
    using Data      = plugins::Syscalls::Data;
}

struct plugins::Syscalls::Data
{
    Data(core::Core& core, proc_t proc);

    bool setup();

    core::Core&      core_;
    proc_t           proc_;
    nt::syscalls     syscalls_;
    objects::Handler objects_;
    Callsteps        callsteps_;
    Triggers         triggers_;
    json             args_;
    uint64_t         nb_triggers_;
};

plugins::Syscalls::Data::Data(core::Core& core, proc_t proc)
    : core_(core)
    , proc_(proc)
    , syscalls_(core, "ntdll")
    , objects_(objects::make(core, proc))
    , nb_triggers_()
{
}

plugins::Syscalls::Syscalls(core::Core& core, proc_t proc)
    : d_(std::make_unique<Data>(core, proc))
{
    d_->setup();
}

plugins::Syscalls::~Syscalls() = default;

namespace
{
    static json create_calltree(core::Core& core, Triggers& triggers, json& args, proc_t target, const Callsteps& callsteps)
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
            const auto symbol     = symbols::find(core, target, it.first);
            const auto symbol_txt = symbols::to_string(symbol);
            calltree[symbol_txt]  = create_calltree(core, it.second, args, target, callsteps);
        }

        // Place args (contained in a json that was made by the observer) on end nodes
        size_t i = 0;
        for(const auto& end_node : end_nodes)
            calltree["Args"][i++] = args[end_node.args_idx];

        return calltree;
    }

    static bool private_get_callstack(Data& d)
    {
        constexpr auto max_size = 128;
        const auto idx          = d.callsteps_.size();
        d.callsteps_.resize(idx + max_size);
        const auto n = callstacks::read(d.core_, &d.callsteps_[idx], max_size, d.proc_);
        if(false)
            for(size_t i = idx; i < idx + n; ++i)
            {
                const auto addr   = d.callsteps_[i].addr;
                const auto symbol = symbols::find(d.core_, d.proc_, addr);
                LOG(INFO, "%zd - %s", i - idx, symbols::to_string(symbol).data());
            }
        d.triggers_.push_back(bp_trigger_info_t{{idx, n}, d.nb_triggers_});
        return true;
    }
}

bool Data::setup()
{
    nb_triggers_ = 0;
    syscalls_.register_NtWriteFile(proc_, [=](nt::HANDLE FileHandle, nt::HANDLE /*Event*/, nt::PIO_APC_ROUTINE /*ApcRoutine*/, nt::PVOID /*ApcContext*/,
                                              nt::PIO_STATUS_BLOCK /*IoStatusBlock*/, nt::PVOID Buffer, nt::ULONG Length,
                                              nt::PLARGE_INTEGER /*ByteOffsetm*/, nt::PULONG /*Key*/)
    {
        std::vector<char> buf(Length);
        const auto reader = reader::make(core_, proc_);
        const auto ok     = reader.read_all(&buf[0], Buffer, Length);
        if(!ok)
            return 1;

        buf[Length - 1]         = 0;
        const auto obj          = objects::read(*objects_, FileHandle);
        const auto obj_typename = objects::type(*objects_, *obj);
        const auto file         = objects::file_read(*objects_, FileHandle);
        const auto obj_filename = objects::file_name(*objects_, *file);
        const auto device_obj   = objects::file_device(*objects_, *file);
        const auto driver_obj   = objects::device_driver(*objects_, *device_obj);
        const auto driver_name  = objects::driver_name(*objects_, *driver_obj);
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
    const auto output = create_calltree(d.core_, d.triggers_, d.args_, d.proc_, d.callsteps_);
    const auto dump   = output.dump();
    const auto ok     = file::write(file_name, dump.data(), dump.size());
    return !!ok;
}
