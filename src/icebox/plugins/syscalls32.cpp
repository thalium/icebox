#include "syscalls.hpp"

#define FDP_MODULE "syscall_tracer"
#include "log.hpp"

#include "callstack.hpp"
#include "endian.hpp"
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

    using json        = nlohmann::json;
    using Callsteps   = std::vector<callstack::callstep_t>;
    using Triggers    = std::vector<bp_trigger_info_t>;
    using SyscallData = plugins::Syscalls32::Data;
    using Callstack   = std::shared_ptr<callstack::ICallstack>;
    using ObjectsNt   = std::shared_ptr<nt::ObjectNt>;
}

struct plugins::Syscalls32::Data
{
    Data(core::Core& core);

    core::Core&        core_;
    tracer::syscalls32 syscalls_;
    ObjectsNt          objects_;
    Callstack          callstack_;
    Callsteps          callsteps_;
    Triggers           triggers_;
    json               args_;
    proc_t             target_;
    uint64_t           nb_triggers_;
};

plugins::Syscalls32::Data::Data(core::Core& core)
    : core_(core)
    , syscalls_(core, "ntdll32")
    , target_()
    , nb_triggers_()
{
}

plugins::Syscalls32::Syscalls32(core::Core& core)
    : d_(std::make_unique<Data>(core))
{
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
        d.callstack_->get_callstack(d.target_, [&](callstack::callstep_t cstep)
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

namespace
{
    opt<std::string> read_unicode_string(const reader::Reader& reader, uint64_t unicode_string)
    {
        using UnicodeString = struct
        {
            uint16_t length;
            uint16_t max_length;
            uint32_t buffer;
        };
        UnicodeString us;
        auto ok = reader.read(&us, unicode_string, sizeof us);
        if(!ok)
            return FAIL(ext::nullopt, "unable to read UNICODE_STRING");

        us.length     = read_le16(&us.length);
        us.max_length = read_le16(&us.max_length);
        us.buffer     = read_le32(&us.buffer);

        if(us.length > us.max_length)
            return FAIL(ext::nullopt, "corrupted UNICODE_STRING");

        std::vector<uint8_t> buffer(us.length);
        ok = reader.read(&buffer[0], us.buffer, us.length);
        if(!ok)
            return FAIL(ext::nullopt, "unable to read UNICODE_STRING.buffer");

        const auto p = &buffer[0];
        return utf8::convert(p, &p[us.length]);
    }
}

bool plugins::Syscalls32::setup(proc_t target)
{
    d_->target_      = target;
    d_->nb_triggers_ = 0;

    d_->callstack_ = callstack::make_callstack_nt(d_->core_);
    if(!d_->callstack_)
        return FAIL(false, "unable to create callstack object");

    d_->objects_ = nt::make_objectnt(d_->core_);
    if(!d_->objects_)
        return FAIL(false, "Unable to create ObjectNt object");

    // Horrible workaround : Windows's 32 bit version on ntdll patches 4 bits of this function. Setting a breakpoint on this page with FDP
    // creates a glitch on the instruction that reads the patched bytes. Single stepping this instruction "simplify" the page resolution
    // and does not trigger the bug...
    d_->syscalls_.register_ZwQueryInformationProcess(target, [=](nt32::HANDLE, nt32::PROCESSINFOCLASS, nt32::PVOID, nt32::ULONG, nt32::PULONG)
    {
        const auto nbr = 4;
        for(auto i = 0; i < nbr; i++)
        {
            const auto ok = d_->core_.state.single_step();
            if(!ok)
                FAIL(1, "Unable to single step");
        }
        return 0;
    });

    d_->syscalls_.register_NtWriteFile(target, [=](nt32::HANDLE FileHandle, nt32::HANDLE /*Event*/, nt32::PIO_APC_ROUTINE /*ApcRoutine*/, nt32::PVOID /*ApcContext*/,
                                                   nt32::PIO_STATUS_BLOCK /*IoStatusBlock*/, nt32::PVOID Buffer, nt32::ULONG Length,
                                                   nt32::PLARGE_INTEGER /*ByteOffsetm*/, nt32::PULONG /*Key*/)
    {
        const auto proc = d_->target_;
        std::vector<char> buf(Length);
        const auto reader = reader::make(d_->core_, proc);
        const auto ok     = reader.read(&buf[0], Buffer, Length);
        if(!ok)
            return 1;

        buf[Length - 1] = 0;
        const auto obj  = d_->objects_->get_object_ref(proc, FileHandle);
        if(!obj)
            return 1;

        const auto obj_filename = d_->objects_->fileobj_filename(proc, *obj);
        if(!obj_filename)
            return 1;

        const auto device_obj = d_->objects_->fileobj_deviceobject(proc, *obj);
        if(!device_obj)
            return 1;

        const auto driver_obj = d_->objects_->deviceobj_driverobject(proc, *device_obj);
        if(!driver_obj)
            return 1;

        const auto driver_name = d_->objects_->driverobj_drivername(proc, *driver_obj);
        if(!driver_name)
            return 1;

        LOG(INFO, " File handle; {:#x}, filename : {}, driver_name {}", FileHandle, obj_filename->data(), driver_name->data());

        d_->args_[d_->nb_triggers_]["FileName"]    = obj_filename->data();
        d_->args_[d_->nb_triggers_]["Driver Name"] = driver_name->data();
        d_->args_[d_->nb_triggers_]["Buffer"]      = buf;
        return 0;
    });

    d_->syscalls_.register_ZwDeviceIoControlFile(target, [=](nt32::HANDLE FileHandle, nt32::HANDLE /*Event*/, nt32::PIO_APC_ROUTINE /*ApcRoutine*/,
                                                             nt32::PVOID /*ApcContext*/, nt32::PIO_STATUS_BLOCK /*IoStatusBlock*/, nt32::ULONG IoControlCode,
                                                             nt32::PVOID /*InputBuffer*/, nt32::ULONG /*InputBufferLength*/, nt32::PVOID /*OutputBuffer*/,
                                                             nt32::ULONG /*OutputBufferLength*/)
    {
        const auto proc = d_->target_;
        const auto obj  = d_->objects_->get_object_ref(proc, FileHandle);
        if(!obj)
            return 1;

        const auto device_obj = d_->objects_->fileobj_deviceobject(proc, *obj);
        if(!device_obj)
            return 1;

        const auto driver_obj = d_->objects_->deviceobj_driverobject(proc, *device_obj);
        if(!driver_obj)
            return 1;

        const auto driver_name = d_->objects_->driverobj_drivername(proc, *driver_obj);
        if(!driver_name)
            return 1;

        const auto ioctl_code = nt32::afd_status_dump(IoControlCode);

        d_->args_[d_->nb_triggers_]["Driver Name"]   = driver_name->data();
        d_->args_[d_->nb_triggers_]["IoControlCode"] = ioctl_code.data();
        return 0;
    });

    d_->syscalls_.register_NtOpenFile(target, [=](nt32::PHANDLE /*FileHandle*/, nt32::ACCESS_MASK /*DesiredAccess*/, nt32::POBJECT_ATTRIBUTES ObjectAttributes,
                                                  nt32::PIO_STATUS_BLOCK /*IoStatusBlock*/, nt32::ULONG /*ShareAccess*/, nt32::ULONG /*OpenOptions*/)
    {
        const auto proc        = d_->target_;
        const auto object_name = d_->objects_->objattribute_objectname(proc, ObjectAttributes);
        if(!object_name)
            return 1;

        d_->args_[d_->nb_triggers_]["FileName"] = object_name->data();
        return 0;
    });

    d_->syscalls_.register_NtCreateUserProcess(target, [&](nt32::PHANDLE /*ProcessHandle*/, nt32::PHANDLE /*ThreadHandle*/, nt32::ACCESS_MASK /*ProcessDesiredAccess*/,
                                                           nt32::ACCESS_MASK /*ThreadDesiredAccess*/, nt32::POBJECT_ATTRIBUTES /*ProcessObjectAttributes*/,
                                                           nt32::POBJECT_ATTRIBUTES /*ThreadObjectAttributes*/, nt32::ULONG /*ProcessFlags*/, nt32::ULONG /*ThreadFlags*/,
                                                           nt32::PRTL_USER_PROCESS_PARAMETERS ProcessParameters, nt32::PPROCESS_CREATE_INFO /*CreateInfo*/,
                                                           nt32::PPROCESS_ATTRIBUTE_LIST /*AttributeList*/)
    {
        const auto proc   = d_->target_;
        const auto reader = reader::make(d_->core_, proc);

        // TODO get _RTL_USER_PROCESS_PARAMETERS from pdb
        // fields ImagePathName and CommandLine
        // 32 bits unicode string
        const auto image_pathname = read_unicode_string(reader, ProcessParameters + 0x38);
        if(!image_pathname)
            return 1;

        const auto command_line = read_unicode_string(reader, ProcessParameters + 0x40);
        if(!command_line)
            return 1;

        d_->args_[d_->nb_triggers_]["ImagePathName"] = image_pathname->data();
        d_->args_[d_->nb_triggers_]["CommandLine"]   = command_line->data();
        return 0;
    });

    d_->syscalls_.register_NtCreateFile(target, [&](nt32::PHANDLE /*FileHandle*/, nt32::ACCESS_MASK DesiredAccess, nt32::POBJECT_ATTRIBUTES ObjectAttributes,
                                                    nt32::PIO_STATUS_BLOCK /*IoStatusBlock*/, nt32::PLARGE_INTEGER /*AllocationSize*/, nt32::ULONG /*FileAttributes*/,
                                                    nt32::ULONG /*ShareAccess*/, nt32::ULONG /*CreateDisposition*/, nt32::ULONG /*CreateOptions*/, nt32::PVOID /*EaBuffer*/,
                                                    nt32::ULONG /*EaLength*/)
    {
        const auto proc        = d_->target_;
        const auto object_name = d_->objects_->objattribute_objectname(proc, ObjectAttributes);
        if(!object_name)
            return 1;

        const auto access = nt32::access_mask_dump(DesiredAccess);

        d_->args_[d_->nb_triggers_]["FileName"] = object_name->data();
        d_->args_[d_->nb_triggers_]["Access"]   = access;
        return 0;
    });

    d_->syscalls_.register_all(target, [&]
    {
        private_get_callstack(*d_);
        d_->nb_triggers_++;
        return 0;
    });

    return true;
}

bool plugins::Syscalls32::generate(const fs::path& file_name)
{
    const auto output = create_calltree(d_->core_, d_->triggers_, d_->args_, d_->target_, d_->callsteps_);
    const auto dump   = output.dump();
    const auto ok     = file::write(file_name, dump.data(), dump.size());
    return !!ok;
}
