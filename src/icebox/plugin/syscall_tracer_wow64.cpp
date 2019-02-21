#include "syscall_tracer.hpp"

#define FDP_MODULE "syscall_tracer"
#include "log.hpp"
#include "os.hpp"

#include "callstack.hpp"
#include "endian.hpp"
#include "monitor/syscallswow64.gen.hpp"
#include "nt/objects_nt.hpp"
#include "os.hpp"
#include "reader.hpp"
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
    using SyscallData = syscall_tracer::SyscallPluginWow64::Data;
}

struct syscall_tracer::SyscallPluginWow64::Data
{
    Data(core::Core& core);

    core::Core&                            core_;
    monitor::syscallswow64                 syscalls_;
    std::shared_ptr<nt::ObjectNt>          objects_;
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

namespace
{
    opt<std::string> read_unicode_string(const reader::Reader& reader, uint64_t unicode_string)
    {
        using UnicodeString = struct
        {
            uint16_t length;
            uint16_t max_length;
            uint32_t _; // padding
            uint64_t buffer;
        };
        UnicodeString us;
        auto ok = reader.read(&us, unicode_string, sizeof us);
        if(!ok)
            FAIL({}, "unable to read UNICODE_STRING");

        us.length     = read_le16(&us.length);
        us.max_length = read_le16(&us.max_length);
        us.buffer     = read_le64(&us.buffer);

        if(us.length > us.max_length)
            FAIL({}, "corrupted UNICODE_STRING");

        std::vector<uint8_t> buffer(us.length);
        ok = reader.read(&buffer[0], us.buffer, us.length);
        if(!ok)
            FAIL({}, "unable to read UNICODE_STRING.buffer");

        const auto p = &buffer[0];
        return utf8::convert(p, &p[us.length]);
    }

    namespace nt32
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
                FAIL({}, "unable to read UNICODE_STRING");

            us.length     = read_le16(&us.length);
            us.max_length = read_le16(&us.max_length);
            us.buffer     = read_le32(&us.buffer);

            if(us.length > us.max_length)
                FAIL({}, "corrupted UNICODE_STRING");

            std::vector<uint8_t> buffer(us.length);
            ok = reader.read(&buffer[0], us.buffer, us.length);
            if(!ok)
                FAIL({}, "unable to read UNICODE_STRING.buffer");

            const auto p = &buffer[0];
            return utf8::convert(p, &p[us.length]);
        }
    } // namespace nt32
}

bool syscall_tracer::SyscallPluginWow64::setup(proc_t target)
{
    d_->target_      = target;
    d_->nb_triggers_ = 0;

    d_->callstack_ = callstack::make_callstack_nt(d_->core_);
    if(!d_->callstack_)
        FAIL(false, "Unable to create callstack object");

    d_->objects_ = nt::make_objectnt(d_->core_);
    if(!d_->objects_)
        FAIL(false, "Unable to create ObjectNt object");

    // Horrible workaround
    d_->syscalls_.register_ZwQueryInformationProcess(target, [=](wntdll::HANDLE, wntdll::PROCESSINFOCLASS, wntdll::PVOID, wntdll::ULONG, wntdll::PULONG)
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

    d_->syscalls_.register_NtWriteFile(target, [=](nt::HANDLE FileHandle, nt::HANDLE /*Event*/, nt::PIO_APC_ROUTINE /*ApcRoutine*/, nt::PVOID /*ApcContext*/,
                                                   nt::PIO_STATUS_BLOCK /*IoStatusBlock*/, nt::PVOID Buffer, nt::ULONG Length,
                                                   nt::PLARGE_INTEGER /*ByteOffsetm*/, nt::PULONG /*Key*/)
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

    d_->syscalls_.register_ZwDeviceIoControlFile(target, [=](nt::HANDLE FileHandle, nt::HANDLE /*Event*/, nt::PIO_APC_ROUTINE /*ApcRoutine*/,
                                                             nt::PVOID /*ApcContext*/, nt::PIO_STATUS_BLOCK /*IoStatusBlock*/, nt::ULONG IoControlCode,
                                                             nt::PVOID /*InputBuffer*/, nt::ULONG /*InputBufferLength*/, nt::PVOID /*OutputBuffer*/,
                                                             nt::ULONG /*OutputBufferLength*/)
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

        opt<std::string> ioctrole_code = {};
        for(size_t i = 0; i < COUNT_OF(nt::g_nt_afd_status); i++)
        {
            if(IoControlCode == nt::g_nt_afd_status[i].status)
            {
                ioctrole_code = nt::g_nt_afd_status[i].status_name;
                break;
            }
        }
        if(!ioctrole_code)
            ioctrole_code = std::to_string(IoControlCode);

        d_->args_[d_->nb_triggers_]["Driver Name"]   = driver_name->data();
        d_->args_[d_->nb_triggers_]["IoControlCode"] = ioctrole_code->data();
        return 0;
    });

    d_->syscalls_.register_NtOpenFile(target, [=](nt::PHANDLE /*FileHandle*/, nt::ACCESS_MASK /*DesiredAccess*/, nt::POBJECT_ATTRIBUTES ObjectAttributes,
                                                  nt::PIO_STATUS_BLOCK /*IoStatusBlock*/, nt::ULONG /*ShareAccess*/, nt::ULONG /*OpenOptions*/)
    {
        const auto proc        = d_->target_;
        const auto object_name = d_->objects_->objattribute_objectname(proc, ObjectAttributes);
        if(!object_name)
            return 1;

        d_->args_[d_->nb_triggers_]["FileName"] = object_name->data();
        return 0;
    });

    d_->syscalls_.register_NtCreateUserProcess(target, [&](nt::PHANDLE /*ProcessHandle*/, nt::PHANDLE /*ThreadHandle*/, nt::ACCESS_MASK /*ProcessDesiredAccess*/,
                                                           nt::ACCESS_MASK /*ThreadDesiredAccess*/, nt::POBJECT_ATTRIBUTES /*ProcessObjectAttributes*/,
                                                           nt::POBJECT_ATTRIBUTES /*ThreadObjectAttributes*/, nt::ULONG /*ProcessFlags*/, nt::ULONG /*ThreadFlags*/,
                                                           nt::PRTL_USER_PROCESS_PARAMETERS ProcessParameters, nt::PPROCESS_CREATE_INFO /*CreateInfo*/,
                                                           nt::PPROCESS_ATTRIBUTE_LIST /*AttributeList*/)
    {
        const auto proc   = d_->target_;
        const auto reader = reader::make(d_->core_, proc);

        // TODO get _RTL_USER_PROCESS_PARAMETERS from pdb
        // fields ImagePathName and CommandLine
        opt<std::string> image_pathname;
        opt<std::string> command_line;
        if(d_->core_.os->proc_ctx_is_x64())
        {
            image_pathname = read_unicode_string(reader, ProcessParameters + 0x60);
            command_line   = read_unicode_string(reader, ProcessParameters + 0x70);
        }
        else
        {
            image_pathname = nt32::read_unicode_string(reader, ProcessParameters + 0x38);
            command_line   = nt32::read_unicode_string(reader, ProcessParameters + 0x40);
        }

        if(!command_line || !image_pathname)
            return 1;

        d_->args_[d_->nb_triggers_]["ImagePathName"] = image_pathname->data();
        d_->args_[d_->nb_triggers_]["CommandLine"]   = command_line->data();
        return 0;
    });

    d_->syscalls_.register_NtCreateFile(target, [&](nt::PHANDLE /*FileHandle*/, nt::ACCESS_MASK DesiredAccess, nt::POBJECT_ATTRIBUTES ObjectAttributes,
                                                    nt::PIO_STATUS_BLOCK /*IoStatusBlock*/, nt::PLARGE_INTEGER /*AllocationSize*/, nt::ULONG /*FileAttributes*/,
                                                    nt::ULONG /*ShareAccess*/, nt::ULONG /*CreateDisposition*/, nt::ULONG /*CreateOptions*/, nt::PVOID /*EaBuffer*/,
                                                    nt::ULONG /*EaLength*/)
    {
        const auto proc        = d_->target_;
        const auto object_name = d_->objects_->objattribute_objectname(proc, ObjectAttributes);
        if(!object_name)
            return 1;

        std::vector<std::string> access;
        for(size_t i = 0; i < COUNT_OF(nt::g_nt_access_mask); i++)
            if(DesiredAccess & nt::g_nt_access_mask[i].mask)
                access.push_back(nt::g_nt_access_mask[i].mask_name);

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

bool syscall_tracer::SyscallPluginWow64::generate(const fs::path& file_name)
{
    const auto output = create_calltree(d_->core_, d_->triggers_, d_->args_, d_->target_, d_->callsteps_);
    const auto dump   = output.dump();
    const auto ok     = file::write(file_name, dump.data(), dump.size());
    return !!ok;
}
