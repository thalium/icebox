#include "syscalls.hpp"

#define FDP_MODULE "syscall_tracer"
#include "log.hpp"

#include "callstack.hpp"
#include "endian.hpp"
#include "nt/nt.hpp"
#include "nt/objects_nt.hpp"
#include "reader.hpp"
#include "tracer/syscalls32.gen.hpp"
#include "utils/file.hpp"
#include "utils/fnview.hpp"
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
    using Callsteps = std::vector<callstack::callstep_t>;
    using Triggers  = std::vector<bp_trigger_info_t>;
    using Callstack = std::shared_ptr<callstack::ICallstack>;
    using Data      = plugins::Syscalls32::Data;
}

struct plugins::Syscalls32::Data
{
    Data(core::Core& core, sym::Symbols& syms, proc_t proc);

    bool setup();

    core::Core&       core_;
    sym::Symbols&     syms_;
    proc_t            proc_;
    wow64::syscalls32 syscalls_;
    nt::ObjectNt      objects_;
    Callstack         callstack_;
    Callsteps         callsteps_;
    Triggers          triggers_;
    json              args_;
    uint64_t          nb_triggers_;
};

plugins::Syscalls32::Data::Data(core::Core& core, sym::Symbols& syms, proc_t proc)
    : core_(core)
    , syms_(syms)
    , proc_(proc)
    , syscalls_(core, syms, "ntdll")
    , objects_(core, proc)
    , nb_triggers_()
{
}

plugins::Syscalls32::Syscalls32(core::Core& core, sym::Symbols& syms, proc_t proc)
    : d_(std::make_unique<Data>(core, syms, proc))
{
    d_->setup();
}

plugins::Syscalls32::~Syscalls32() = default;

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

bool Data::setup()
{
    nb_triggers_ = 0;

    callstack_ = callstack::make_callstack_nt(core_);
    if(!callstack_)
        return FAIL(false, "unable to create callstack object");

    // Horrible workaround : Windows's 32 bit version on ntdll patches 4 bits of this function. Setting a breakpoint on this page with FDP
    // creates a glitch on the instruction that reads the patched bytes. Single stepping this instruction "simplify" the page resolution
    // and does not trigger the bug...
    syscalls_.register_ZwQueryInformationProcess(proc_, [=](wow64::HANDLE, wow64::PROCESSINFOCLASS, wow64::PVOID, wow64::ULONG, wow64::PULONG)
    {
        const auto nbr = 4;
        for(auto i = 0; i < nbr; i++)
        {
            const auto ok = core_.state.single_step();
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
        const auto reader = reader::make(core_, proc_);
        const auto ok     = reader.read(&buf[0], Buffer, Length);
        if(!ok)
            return 1;

        buf[Length - 1] = 0;
        const auto file = objects_.file_read(FileHandle);
        if(!file)
            return 1;

        const auto obj_filename = objects_.file_name(*file);
        if(!obj_filename)
            return 1;

        const auto device_obj = objects_.file_device(*file);
        if(!device_obj)
            return 1;

        const auto driver_obj = objects_.device_driver(*device_obj);
        if(!driver_obj)
            return 1;

        const auto driver_name = objects_.driver_name(*driver_obj);
        if(!driver_name)
            return 1;

        LOG(INFO, " File handle; {:#x}, filename : {}, driver_name {}", FileHandle, obj_filename->data(), driver_name->data());

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
        const auto obj = objects_.file_read(FileHandle);
        if(!obj)
            return 1;

        const auto device_obj = objects_.file_device(*obj);
        if(!device_obj)
            return 1;

        const auto driver_obj = objects_.device_driver(*device_obj);
        if(!driver_obj)
            return 1;

        const auto driver_name = objects_.driver_name(*driver_obj);
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
        const auto reader = reader::make(core_, proc_);
        const auto ok     = reader.read(&attr, ObjectAttributes, sizeof attr);
        if(!ok)
            return 1;

        if(!attr.ObjectName)
            return 1;

        const auto object_name = wow64::read_unicode_string(reader, attr.ObjectName);
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
        const auto reader = reader::make(core_, proc_);

        // TODO get _RTL_USER_PROCESS_PARAMETERS from pdb
        // fields ImagePathName and CommandLine
        // 32 bits unicode string
        const auto image_pathname = wow64::read_unicode_string(reader, ProcessParameters + 0x38);
        if(!image_pathname)
            return 1;

        const auto command_line = wow64::read_unicode_string(reader, ProcessParameters + 0x40);
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
        const auto reader = reader::make(core_, proc_);
        const auto ok     = reader.read(&attr, ObjectAttributes, sizeof attr);
        if(!ok)
            return 1;

        if(!attr.ObjectName)
            return 1;

        const auto object_name = wow64::read_unicode_string(reader, attr.ObjectName);
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
    auto& d           = *d_;
    const auto output = create_calltree(d.core_, d.syms_, d.triggers_, d.args_, d.proc_, d.callsteps_);
    const auto dump   = output.dump();
    const auto ok     = file::write(file_name, dump.data(), dump.size());
    return !!ok;
}
