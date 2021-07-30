#include "syscalls.hpp"

#define FDP_MODULE "syscall_tracer"
#include "core.hpp"
#include "endian.hpp"
#include "log.hpp"
#include "nt/nt.hpp"
#include "nt/nt_objects.hpp"
#include "tracer/syscalls32.gen.hpp"
#include "utils/file.hpp"
#include "utils/pe.hpp"
#include "utils/utf8.hpp"
#include "utils/utils.hpp"

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
    using Data      = plugins::Syscalls32::Data;
}

struct plugins::Syscalls32::Data
{
    Data(core::Core& core, proc_t proc);

    bool setup();

    core::Core&       core_;
    proc_t            proc_;
    wow64::syscalls32 syscalls_;
    objects::Handler  objects_;
    Callsteps         callsteps_;
    Triggers          triggers_;
    json              args_;
    uint64_t          nb_triggers_;
};

plugins::Syscalls32::Data::Data(core::Core& core, proc_t proc)
    : core_(core)
    , proc_(proc)
    , syscalls_(core, "wntdll")
    , objects_(objects::make(core, proc))
    , nb_triggers_()
{
}

plugins::Syscalls32::Syscalls32(core::Core& core, proc_t proc)
    : d_(std::make_unique<Data>(core, proc))
{
    d_->setup();
}

plugins::Syscalls32::~Syscalls32() = default;

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
            const auto symbol_txt = symbols::string(core, target, it.first);
            calltree[symbol_txt]  = create_calltree(core, it.second, args, target, callsteps);
        }

        // Place args (contained in a json that was made by the observer) on end nodes
        size_t i = 0;
        for(const auto& end_node : end_nodes)
            calltree["Args"][i++] = args[end_node.args_idx];

        return calltree;
    }

    bool private_get_callstack(Data& d)
    {
        constexpr auto max_size = 128;
        const auto     idx      = d.callsteps_.size();
        d.callsteps_.resize(idx + max_size);
        const auto n = callstacks::read(d.core_, &d.callsteps_[idx], max_size, d.proc_);
        if(false)
            for(size_t i = idx; i < idx + n; ++i)
            {
                const auto addr   = d.callsteps_[i].addr;
                const auto symbol = symbols::string(d.core_, d.proc_, addr);
                LOG(INFO, "%zd - %s", i - idx, symbol.data());
            }
        d.triggers_.push_back(bp_trigger_info_t{{idx, n}, d.nb_triggers_});
        return true;
    }
}

bool Data::setup()
{
    nb_triggers_ = 0;

    // Horrible workaround : Windows's 32 bit version on ntdll patches 4 bits of this function. Setting a breakpoint on this page with FDP
    // creates a glitch on the instruction that reads the patched bytes. Single stepping this instruction "simplify" the page resolution
    // and does not trigger the bug...
    syscalls_.register_ZwQueryInformationProcess(proc_, [=](wow64::HANDLE /*ProcessHandle*/,
                                                            wow64::PROCESSINFOCLASS /*ProcessInformationClass*/,
                                                            wow64::PVOID /*ProcessInformation*/,
                                                            wow64::ULONG /*ProcessInformationLength*/,
                                                            wow64::PULONG /*ReturnLength*/)
    {
        const auto nbr = 4;
        for(auto i = 0; i < nbr; i++)
        {
            const auto ok = state::single_step(core_);
            if(!ok)
                return FAIL(1, "Unable to single step");
        }
        return 0;
    });

    syscalls_.register_NtWriteFile(proc_, [=](wow64::HANDLE FileHandle, wow64::HANDLE /*Event*/, wow64::PIO_APC_ROUTINE /*ApcRoutine*/, wow64::PVOID /*ApcContext*/,
                                              wow64::PIO_STATUS_BLOCK /*IoStatusBlock*/, wow64::PVOID Buffer, wow64::ULONG Length,
                                              wow64::PLARGE_INTEGER /*ByteOffsetm*/, wow64::PULONG /*Key*/)
    {
        std::vector<char> buf(Length);
        const auto        io = memory::make_io(core_, proc_);
        const auto        ok = io.read_all(&buf[0], Buffer, Length);
        if(!ok)
            return 1;

        buf[Length - 1] = 0;
        const auto file = objects::file_read(*objects_, FileHandle);
        if(!file)
            return 1;

        const auto obj_filename = objects::file_name(*objects_, *file);
        if(!obj_filename)
            return 1;

        const auto device_obj = objects::file_device(*objects_, *file);
        if(!device_obj)
            return 1;

        const auto driver_obj = objects::device_driver(*objects_, *device_obj);
        if(!driver_obj)
            return 1;

        const auto driver_name = objects::driver_name(*objects_, *driver_obj);
        if(!driver_name)
            return 1;

        LOG(INFO, " File handle; 0x%x, filename : %s, driver_name %s", FileHandle, obj_filename->data(), driver_name->data());

        args_[nb_triggers_]["FileName"]    = obj_filename->data();
        args_[nb_triggers_]["Driver Name"] = driver_name->data();
        args_[nb_triggers_]["Buffer"]      = buf;
        return 0;
    });

    syscalls_.register_ZwDeviceIoControlFile(proc_, [=](wow64::HANDLE FileHandle, wow64::HANDLE /*Event*/, wow64::PIO_APC_ROUTINE /*ApcRoutine*/,
                                                        wow64::PVOID /*ApcContext*/, wow64::PIO_STATUS_BLOCK /*IoStatusBlock*/, wow64::ULONG IoControlCode,
                                                        wow64::PVOID /*InputBuffer*/, wow64::ULONG /*InputBufferLength*/, wow64::PVOID /*OutputBuffer*/,
                                                        wow64::ULONG /*OutputBufferLength*/)
    {
        const auto obj = objects::file_read(*objects_, FileHandle);
        if(!obj)
            return 1;

        const auto device_obj = objects::file_device(*objects_, *obj);
        if(!device_obj)
            return 1;

        const auto driver_obj = objects::device_driver(*objects_, *device_obj);
        if(!driver_obj)
            return 1;

        const auto driver_name = objects::driver_name(*objects_, *driver_obj);
        if(!driver_name)
            return 1;

        args_[nb_triggers_]["Driver Name"]   = driver_name->data();
        args_[nb_triggers_]["IoControlCode"] = nt::ioctl_code_dump(static_cast<nt::IOCTL_CODE>(IoControlCode));
        return 0;
    });

    syscalls_.register_NtOpenFile(proc_, [=](wow64::PHANDLE /*FileHandle*/, wow64::ACCESS_MASK /*DesiredAccess*/, wow64::POBJECT_ATTRIBUTES ObjectAttributes,
                                             wow64::PIO_STATUS_BLOCK /*IoStatusBlock*/, wow64::ULONG /*ShareAccess*/, wow64::ULONG /*OpenOptions*/)
    {
        wow64::_OBJECT_ATTRIBUTES attr;
        const auto                io = memory::make_io(core_, proc_);
        const auto                ok = io.read_all(&attr, ObjectAttributes, sizeof attr);
        if(!ok)
            return 1;

        if(!attr.ObjectName)
            return 1;

        const auto object_name = wow64::read_unicode_string(io, attr.ObjectName);
        if(!object_name)
            return 1;

        args_[nb_triggers_]["FileName"] = object_name->data();
        return 0;
    });

    syscalls_.register_NtCreateUserProcess(proc_, [&](wow64::PHANDLE /*ProcessHandle*/, wow64::PHANDLE /*ThreadHandle*/, wow64::ACCESS_MASK /*ProcessDesiredAccess*/,
                                                      wow64::ACCESS_MASK /*ThreadDesiredAccess*/, wow64::POBJECT_ATTRIBUTES /*ProcessObjectAttributes*/,
                                                      wow64::POBJECT_ATTRIBUTES /*ThreadObjectAttributes*/, wow64::ULONG /*ProcessFlags*/, wow64::ULONG /*ThreadFlags*/,
                                                      wow64::PRTL_USER_PROCESS_PARAMETERS ProcessParameters, wow64::PPROCESS_CREATE_INFO /*CreateInfo*/,
                                                      wow64::PPROCESS_ATTRIBUTE_LIST /*AttributeList*/)
    {
        const auto io = memory::make_io(core_, proc_);

        // TODO get _RTL_USER_PROCESS_PARAMETERS from pdb
        // fields ImagePathName and CommandLine
        // 32 bits unicode string
        const auto image_pathname = wow64::read_unicode_string(io, ProcessParameters + 0x38);
        if(!image_pathname)
            return 1;

        const auto command_line = wow64::read_unicode_string(io, ProcessParameters + 0x40);
        if(!command_line)
            return 1;

        args_[nb_triggers_]["ImagePathName"] = image_pathname->data();
        args_[nb_triggers_]["CommandLine"]   = command_line->data();
        return 0;
    });

    syscalls_.register_NtCreateFile(proc_, [&](wow64::PHANDLE /*FileHandle*/, wow64::ACCESS_MASK DesiredAccess, wow64::POBJECT_ATTRIBUTES ObjectAttributes,
                                               wow64::PIO_STATUS_BLOCK /*IoStatusBlock*/, wow64::PLARGE_INTEGER /*AllocationSize*/, wow64::ULONG /*FileAttributes*/,
                                               wow64::ULONG /*ShareAccess*/, wow64::ULONG /*CreateDisposition*/, wow64::ULONG /*CreateOptions*/, wow64::PVOID /*EaBuffer*/,
                                               wow64::ULONG /*EaLength*/)
    {
        wow64::_OBJECT_ATTRIBUTES attr;
        const auto                io = memory::make_io(core_, proc_);
        const auto                ok = io.read_all(&attr, ObjectAttributes, sizeof attr);
        if(!ok)
            return 1;

        if(!attr.ObjectName)
            return 1;

        const auto object_name = wow64::read_unicode_string(io, attr.ObjectName);
        if(!object_name)
            return 1;

        const auto access = nt::access_mask_all(static_cast<uint32_t>(DesiredAccess));

        args_[nb_triggers_]["FileName"] = object_name->data();
        args_[nb_triggers_]["Access"]   = access;
        return 0;
    });

    syscalls_.register_all(proc_, [&](const auto& /*callcfg*/)
    {
        private_get_callstack(*this);
        nb_triggers_++;
        return 0;
    });

    return true;
}

bool plugins::Syscalls32::generate(const fs::path& file_name)
{
    auto&      d      = *d_;
    const auto output = create_calltree(d.core_, d.triggers_, d.args_, d.proc_, d.callsteps_);
    const auto dump   = output.dump();
    const auto ok     = file::write(file_name, dump.data(), dump.size());
    return !!ok;
}
