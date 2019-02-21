#include "os.hpp"

#define FDP_MODULE "os_nt"
#include "core.hpp"
#include "log.hpp"
#include "reader.hpp"
#include "utils/fnview.hpp"
#include "utils/hex.hpp"
#include "utils/path.hpp"
#include "utils/pe.hpp"
#include "utils/utf8.hpp"
#include "utils/utils.hpp"

#include <array>

namespace
{
    namespace nt32
    {
        struct _LIST_ENTRY
        {
            uint32_t flink;
            uint32_t blink;
        };
        STATIC_ASSERT_EQ(8, sizeof(_LIST_ENTRY));

        struct _PEB_LDR_DATA
        {
            uint32_t    Length;
            uint8_t     Initialized;
            uint32_t    SsHandle;
            _LIST_ENTRY InLoadOrderModuleList;
            _LIST_ENTRY InMemoryOrderModuleList;
            _LIST_ENTRY InInitializationOrderModuleList;
            uint32_t    EntryInProgress;
            uint8_t     ShutdownInProgress;
            uint32_t    ShutdownThreadId;
        };
        STATIC_ASSERT_EQ(48, sizeof(_PEB_LDR_DATA));

        using _UNICODE_STRING = struct
        {
            uint16_t length;
            uint16_t max_length;
            uint32_t buffer;
        };
        STATIC_ASSERT_EQ(8, sizeof(_UNICODE_STRING));

        struct _LDR_DATA_TABLE_ENTRY
        {
            _LIST_ENTRY     InLoadOrderLinks;
            _LIST_ENTRY     InMemoryOrderLinks;
            _LIST_ENTRY     InInitializationOrderLinks;
            uint32_t        DllBase;
            uint32_t        EntryPoint;
            uint32_t        SizeOfImage;
            _UNICODE_STRING FullDllName;
            _UNICODE_STRING BaseDllName;
        };
        STATIC_ASSERT_EQ(52, sizeof(_LDR_DATA_TABLE_ENTRY));
    } // namespace nt32

    enum class cat_e
    {
        REQUIRED,
        OPTIONAL,
    };

    enum nt_e
    {
        CLIENT_ID_UniqueThread,
        EPROCESS_ActiveProcessLinks,
        EPROCESS_ImageFileName,
        EPROCESS_Pcb,
        EPROCESS_Peb,
        EPROCESS_SeAuditProcessCreationInfo,
        EPROCESS_ThreadListHead,
        EPROCESS_UniqueProcessId,
        EPROCESS_VadRoot,
        EPROCESS_Wow64Process,
        ETHREAD_Cid,
        ETHREAD_Tcb,
        ETHREAD_ThreadListEntry,
        KPCR_Irql,
        KPCR_Prcb,
        KPRCB_CurrentThread,
        KPRCB_KernelDirectoryTableBase,
        KPROCESS_DirectoryTableBase,
        KPROCESS_UserDirectoryTableBase,
        KTHREAD_Process,
        KTHREAD_TrapFrame,
        KTRAP_FRAME_Rip,
        TEB_NtTib,
        NT_TIB_StackBase,
        NT_TIB_StackLimit,
        LDR_DATA_TABLE_ENTRY_DllBase,
        LDR_DATA_TABLE_ENTRY_FullDllName,
        LDR_DATA_TABLE_ENTRY_InLoadOrderLinks,
        LDR_DATA_TABLE_ENTRY_SizeOfImage,
        OBJECT_NAME_INFORMATION_Name,
        PEB_Ldr,
        PEB_LDR_DATA_InLoadOrderModuleList,
        PEB_ProcessParameters,
        PEB32_Ldr,
        RTL_USER_PROCESS_PARAMETERS_ImagePathName,
        SE_AUDIT_PROCESS_CREATION_INFO_ImageFileName,
        EWOW64PROCESS_Peb,
        EWOW64PROCESS_NtdllType,
        NT_COUNT,
    };

    struct NtOffset
    {
        cat_e      e_cat;
        nt_e       e_id;
        const char module[16];
        const char struc[32];
        const char member[32];
    };
    // clang-format off
    const NtOffset g_nt_offsets[] =
    {
        {cat_e::REQUIRED,   CLIENT_ID_UniqueThread,                       "nt", "_CLIENT_ID",                       "UniqueThread"},
        {cat_e::REQUIRED,   EPROCESS_ActiveProcessLinks,                  "nt", "_EPROCESS",                        "ActiveProcessLinks"},
        {cat_e::REQUIRED,   EPROCESS_ImageFileName,                       "nt", "_EPROCESS",                        "ImageFileName"},
        {cat_e::REQUIRED,   EPROCESS_Pcb,                                 "nt", "_EPROCESS",                        "Pcb"},
        {cat_e::REQUIRED,   EPROCESS_Peb,                                 "nt", "_EPROCESS",                        "Peb"},
        {cat_e::REQUIRED,   EPROCESS_SeAuditProcessCreationInfo,          "nt", "_EPROCESS",                        "SeAuditProcessCreationInfo"},
        {cat_e::REQUIRED,   EPROCESS_ThreadListHead,                      "nt", "_EPROCESS",                        "ThreadListHead"},
        {cat_e::REQUIRED,   EPROCESS_UniqueProcessId,                     "nt", "_EPROCESS",                        "UniqueProcessId"},
        {cat_e::REQUIRED,   EPROCESS_VadRoot,                             "nt", "_EPROCESS",                        "VadRoot"},
        {cat_e::REQUIRED,   EPROCESS_Wow64Process,                        "nt", "_EPROCESS",                        "Wow64Process"},
        {cat_e::REQUIRED,   ETHREAD_Cid,                                  "nt", "_ETHREAD",                         "Cid"},
        {cat_e::REQUIRED,   ETHREAD_Tcb,                                  "nt", "_ETHREAD",                         "Tcb"},
        {cat_e::REQUIRED,   ETHREAD_ThreadListEntry,                      "nt", "_ETHREAD",                         "ThreadListEntry"},
        {cat_e::REQUIRED,   KPCR_Irql,                                    "nt", "_KPCR",                            "Irql"},
        {cat_e::REQUIRED,   KPCR_Prcb,                                    "nt", "_KPCR",                            "Prcb"},
        {cat_e::REQUIRED,   KPRCB_CurrentThread,                          "nt", "_KPRCB",                           "CurrentThread"},
        {cat_e::OPTIONAL,   KPRCB_KernelDirectoryTableBase,               "nt", "_KPRCB",                           "KernelDirectoryTableBase"},
        {cat_e::REQUIRED,   KPROCESS_DirectoryTableBase,                  "nt", "_KPROCESS",                        "DirectoryTableBase"},
        {cat_e::OPTIONAL,   KPROCESS_UserDirectoryTableBase,              "nt", "_KPROCESS",                        "UserDirectoryTableBase"},
        {cat_e::REQUIRED,   KTHREAD_Process,                              "nt", "_KTHREAD",                         "Process"},
        {cat_e::REQUIRED,   KTHREAD_TrapFrame,                            "nt", "_KTHREAD",                         "TrapFrame"},
        {cat_e::REQUIRED,   KTRAP_FRAME_Rip,                              "nt", "_KTRAP_FRAME",                     "Rip"},
        {cat_e::REQUIRED,   TEB_NtTib,                                    "nt", "_TEB",                             "NtTib"},
        {cat_e::REQUIRED,   NT_TIB_StackBase,                             "nt", "_NT_TIB",                          "StackBase"},
        {cat_e::REQUIRED,   NT_TIB_StackLimit,                            "nt", "_NT_TIB",                          "StackLimit"},
        {cat_e::REQUIRED,   LDR_DATA_TABLE_ENTRY_DllBase,                 "nt", "_LDR_DATA_TABLE_ENTRY",            "DllBase"},
        {cat_e::REQUIRED,   LDR_DATA_TABLE_ENTRY_FullDllName,             "nt", "_LDR_DATA_TABLE_ENTRY",            "FullDllName"},
        {cat_e::REQUIRED,   LDR_DATA_TABLE_ENTRY_InLoadOrderLinks,        "nt", "_LDR_DATA_TABLE_ENTRY",            "InLoadOrderLinks"},
        {cat_e::REQUIRED,   LDR_DATA_TABLE_ENTRY_SizeOfImage,             "nt", "_LDR_DATA_TABLE_ENTRY",            "SizeOfImage"},
        {cat_e::REQUIRED,   OBJECT_NAME_INFORMATION_Name,                 "nt", "_OBJECT_NAME_INFORMATION",         "Name"},
        {cat_e::REQUIRED,   PEB_Ldr,                                      "nt", "_PEB",                             "Ldr"},
        {cat_e::REQUIRED,   PEB_LDR_DATA_InLoadOrderModuleList,           "nt", "_PEB_LDR_DATA",                    "InLoadOrderModuleList"},
        {cat_e::REQUIRED,   PEB_ProcessParameters,                        "nt", "_PEB",                             "ProcessParameters"},
        {cat_e::REQUIRED,   PEB32_Ldr,                                    "nt", "_PEB32",                           "Ldr"},
        {cat_e::REQUIRED,   RTL_USER_PROCESS_PARAMETERS_ImagePathName,    "nt", "_RTL_USER_PROCESS_PARAMETERS",     "ImagePathName"},
        {cat_e::REQUIRED,   SE_AUDIT_PROCESS_CREATION_INFO_ImageFileName, "nt", "_SE_AUDIT_PROCESS_CREATION_INFO",  "ImageFileName"},
        {cat_e::OPTIONAL,   EWOW64PROCESS_Peb,                            "nt", "_EWOW64PROCESS",                   "Peb"},
        {cat_e::OPTIONAL,   EWOW64PROCESS_NtdllType,                      "nt", "_EWOW64PROCESS",                   "NtdllType"},
    };
    // clang-format on
    STATIC_ASSERT_EQ(COUNT_OF(g_nt_offsets), NT_COUNT);

    enum symbol_offset_e
    {
        KiKernelSysretExit,
        KiSystemCall64,
        PsActiveProcessHead,
        PsInitialSystemProcess,
        PsLoadedModuleList,
        PspCreateProcessNotifyRoutine,
        PspCreateProcessNotifyRoutineCount,
        SYMBOL_OFFSET_COUNT,
    };

    struct SymbolOffset
    {
        cat_e           e_cat;
        symbol_offset_e e_id;
        const char      module[16];
        const char      name[64];
    };
    // clang-format off
    const SymbolOffset g_symbol_offsets[] =
    {
        {cat_e::OPTIONAL, KiKernelSysretExit,                  "nt", "KiKernelSysretExit"},
        {cat_e::REQUIRED, KiSystemCall64,                      "nt", "KiSystemCall64"},
        {cat_e::REQUIRED, PsActiveProcessHead,                 "nt", "PsActiveProcessHead"},
        {cat_e::REQUIRED, PsInitialSystemProcess,              "nt", "PsInitialSystemProcess"},
        {cat_e::REQUIRED, PsLoadedModuleList,                  "nt", "PsLoadedModuleList"},
        {cat_e::REQUIRED, PspCreateProcessNotifyRoutine,       "nt", "PspCreateProcessNotifyRoutine"},
        {cat_e::REQUIRED, PspCreateProcessNotifyRoutineCount , "nt", "PspCreateProcessNotifyRoutineCount"},
    };
    // clang-format on
    static_assert(COUNT_OF(g_symbol_offsets) == SYMBOL_OFFSET_COUNT, "invalid symbols");

    using NtOffsets     = std::array<uint64_t, NT_COUNT>;
    using SymbolOffsets = std::array<uint64_t, SYMBOL_OFFSET_COUNT>;

    struct OsNt
        : public os::IModule
    {
        OsNt(core::Core& core);

        // methods
        bool setup();

        // os::IModule
        bool    is_kernel   (uint64_t ptr) override;
        bool    reader_setup(reader::Reader& reader, proc_t proc) override;

        bool                proc_list           (const on_proc_fn& on_process) override;
        opt<proc_t>         proc_current        () override;
        opt<proc_t>         proc_find           (const std::string& name) override;
        opt<proc_t>         proc_find           (uint64_t pid) override;
        opt<std::string>    proc_name           (proc_t proc) override;
        bool                proc_is_valid       (proc_t proc) override;
        flags_e             proc_flags          (proc_t proc) override;
        uint64_t            proc_id             (proc_t proc) override;
        bool                proc_ctx_is_x64     () override;
        void                proc_join           (proc_t proc, os::join_e join) override;
        opt<phy_t>          proc_resolve        (proc_t proc, uint64_t ptr) override;
        opt<proc_t>         proc_select         (proc_t proc, uint64_t ptr) override;
        bool                proc_listen_create  (const on_proc_event_fn& on_proc_event) override;
        bool                proc_listen_delete  (const on_proc_event_fn& on_proc_event) override;

        bool            thread_list     (proc_t proc, const on_thread_fn& on_thread) override;
        opt<thread_t>   thread_current  () override;
        opt<proc_t>     thread_proc     (thread_t thread) override;
        opt<uint64_t>   thread_pc       (proc_t proc, thread_t thread) override;
        uint64_t        thread_id       (proc_t proc, thread_t thread) override;

        bool                mod_list(proc_t proc, const on_mod_fn& on_module) override;
        opt<std::string>    mod_name(proc_t proc, mod_t mod) override;
        opt<span_t>         mod_span(proc_t proc, mod_t mod) override;
        opt<mod_t>          mod_find(proc_t proc, uint64_t addr) override;

        bool                driver_list (const on_driver_fn& on_driver) override;
        opt<driver_t>       driver_find (const std::string& name) override;
        opt<std::string>    driver_name (driver_t drv) override;
        opt<span_t>         driver_span (driver_t drv) override;

        void debug_print() override;

        // members
        core::Core&    core_;
        NtOffsets      offsets_;
        SymbolOffsets  symbols_;
        std::string    last_dump_;
        uint64_t       kpcr_;
        reader::Reader reader_;

        using Breakpoints  = std::vector<core::Breakpoint>;
        using ObsProcEvent = std::vector<OsNt::on_proc_event_fn>;

        Breakpoints  breakpoints_;
        ObsProcEvent observers_proc_create_;
        ObsProcEvent observers_proc_remove_;
    };
}

OsNt::OsNt(core::Core& core)
    : core_(core)
    , reader_(reader::make(core))
{
}

namespace
{
    opt<span_t> find_kernel(core::Memory& mem, uint64_t lstar)
    {
        uint8_t buf[PAGE_SIZE];
        for(auto ptr = utils::align<PAGE_SIZE>(lstar); ptr < lstar; ptr -= PAGE_SIZE)
        {
            auto ok = mem.read_virtual(buf, ptr, sizeof buf);
            if(!ok)
                return {};

            const auto size = pe::read_image_size(buf, sizeof buf);
            if(!size)
                continue;

            return span_t{ptr, *size};
        }

        return {};
    }
}

bool OsNt::setup()
{
    const auto lstar  = core_.regs.read(MSR_LSTAR);
    const auto kernel = find_kernel(core_.mem, lstar);
    if(!kernel)
        FAIL(false, "unable to find kernel");

    LOG(INFO, "kernel: {:#x} - {:#x} ({} {:#x})", kernel->addr, kernel->addr + kernel->size, kernel->size, kernel->size);
    std::vector<uint8_t> buffer(kernel->size);
    auto ok = core_.mem.read_virtual(&buffer[0], kernel->addr, kernel->size);
    if(!ok)
        FAIL(false, "unable to read kernel module");

    ok = core_.sym.insert("nt", *kernel, &buffer[0], buffer.size());
    if(!ok)
        FAIL(false, "unable to load symbols from kernel module");

    bool fail = false;
    memset(&symbols_[0], 0, sizeof symbols_);
    for(size_t i = 0; i < SYMBOL_OFFSET_COUNT; ++i)
    {
        const auto addr = core_.sym.symbol(g_symbol_offsets[i].module, g_symbol_offsets[i].name);
        if(!addr)
        {
            fail |= g_symbol_offsets[i].e_cat == cat_e::REQUIRED;
            if(g_symbol_offsets[i].e_cat == cat_e::REQUIRED)
                LOG(ERROR, "unable to read {}!{} symbol offset", g_symbol_offsets[i].module, g_symbol_offsets[i].name);
            else
                LOG(WARNING, "unable to read {}!{} symbol offset", g_symbol_offsets[i].module, g_symbol_offsets[i].name);
            continue;
        }

        symbols_[i] = *addr;
    }

    memset(&offsets_[0], 0, sizeof offsets_);
    for(size_t i = 0; i < NT_COUNT; ++i)
    {
        fail |= g_nt_offsets[i].e_id != i;
        const auto offset = core_.sym.struc_offset(g_nt_offsets[i].module, g_nt_offsets[i].struc, g_nt_offsets[i].member);
        if(!offset)
        {
            fail |= g_nt_offsets[i].e_cat == cat_e::REQUIRED;
            if(g_nt_offsets[i].e_cat == cat_e::REQUIRED)
                LOG(ERROR, "unable to read {}!{}.{} member offset", g_nt_offsets[i].module, g_nt_offsets[i].struc, g_nt_offsets[i].member);
            else
                LOG(WARNING, "unable to read {}!{}.{} member offset", g_nt_offsets[i].module, g_nt_offsets[i].struc, g_nt_offsets[i].member);
            continue;
        }
        offsets_[i] = *offset;
    }
    if(fail)
        return false;

    kpcr_ = core_.regs.read(MSR_GS_BASE);
    if(!(kpcr_ & 0xFFF0000000000000))
        kpcr_ = core_.regs.read(MSR_KERNEL_GS_BASE);
    if(!(kpcr_ & 0xFFF0000000000000))
        FAIL(false, "unable to read KPCR");

    auto gdtb = dtb_t{core_.regs.read(FDP_CR3_REGISTER)};
    if(offsets_[KPRCB_KernelDirectoryTableBase])
    {
        ok = core_.mem.read_virtual(&gdtb, kpcr_ + offsets_[KPCR_Prcb] + offsets_[KPRCB_KernelDirectoryTableBase], sizeof gdtb);
        if(!ok)
            FAIL(false, "unable to read KPRCB.KernelDirectoryTableBase");
    }

    // cr3 is same in user & kernel mode
    if(!offsets_[KPROCESS_UserDirectoryTableBase])
        offsets_[KPROCESS_UserDirectoryTableBase] = offsets_[KPROCESS_DirectoryTableBase];

    reader_.kdtb_ = gdtb;
    LOG(WARNING, "kernel: kpcr: {:#x} kdtb: {:#x}", kpcr_, gdtb.val);
    return true;
}

std::unique_ptr<os::IModule> os::make_nt(core::Core& core)
{
    auto nt = std::make_unique<OsNt>(core);
    if(!nt)
        return nullptr;

    const auto ok = nt->setup();
    if(!ok)
        return nullptr;

    return nt;
}

bool OsNt::proc_list(const on_proc_fn& on_process)
{
    const auto head = symbols_[PsActiveProcessHead];
    for(auto link = reader_.read(head); link != head; link = reader_.read(*link))
    {
        const auto eproc = *link - offsets_[EPROCESS_ActiveProcessLinks];
        const auto dtb   = reader_.read(eproc + offsets_[EPROCESS_Pcb] + offsets_[KPROCESS_UserDirectoryTableBase]);
        if(!dtb)
        {
            LOG(ERROR, "unable to read KPROCESS.DirectoryTableBase from {:#x}", eproc);
            continue;
        }

        const auto err = on_process({eproc, {*dtb}});
        if(err == WALK_STOP)
            break;
    }
    return true;
}

opt<proc_t> OsNt::proc_current()
{
    const auto current = thread_current();
    if(!current)
        FAIL({}, "unable to get current thread");

    return thread_proc(*current);
}

opt<proc_t> OsNt::proc_find(const std::string& name)
{
    opt<proc_t> found;
    proc_list([&](proc_t proc)
    {
        const auto got = proc_name(proc);
        if(got != name)
            return WALK_NEXT;

        found = proc;
        return WALK_STOP;
    });
    return found;
}

opt<proc_t> OsNt::proc_find(uint64_t pid)
{
    opt<proc_t> found;
    proc_list([&](proc_t proc)
    {
        const auto got = proc_id(proc);
        if(got != pid)
            return WALK_NEXT;

        found = proc;
        return WALK_STOP;
    });
    return found;
}

namespace
{
    opt<std::string> read_unicode_string(const reader::Reader& reader, uint64_t unicode_string)
    {
        using _UNICODE_STRING = struct
        {
            uint16_t length;
            uint16_t max_length;
            uint32_t _; // padding
            uint64_t buffer;
        };
        _UNICODE_STRING us;
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
            nt32::_UNICODE_STRING us;
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

    static opt<uint64_t> read_wow64_peb(const OsNt& os, const reader::Reader& reader, proc_t proc)
    {
        const auto wowp = reader.read(proc.id + os.offsets_[EPROCESS_Wow64Process]);
        if(!wowp)
            FAIL(false, "unable to read EPROCESS.Wow64Process");

        if(!os.offsets_[EWOW64PROCESS_NtdllType])
            return wowp;

        const auto peb32 = reader.read(*wowp + os.offsets_[EWOW64PROCESS_Peb]);
        if(!peb32)
            FAIL(false, "unable to read EWOW64PROCESS.Peb");

        return *peb32;
    }

#define offsetof32(x, y) static_cast<uint32_t>(offsetof(x, y))
}

opt<std::string> OsNt::proc_name(proc_t proc)
{
    // EPROCESS.ImageFileName is 16 bytes, but only 14 are actually used
    char buffer[14 + 1];
    const auto ok             = reader_.read(buffer, proc.id + offsets_[EPROCESS_ImageFileName], sizeof buffer);
    buffer[sizeof buffer - 1] = 0;
    if(!ok)
        return {};

    const auto name = std::string{buffer};
    if(name.size() < sizeof buffer - 1)
        return name;

    const auto image_file_name = reader_.read(proc.id + offsets_[EPROCESS_SeAuditProcessCreationInfo] + offsets_[SE_AUDIT_PROCESS_CREATION_INFO_ImageFileName]);
    if(!image_file_name)
        return name;

    const auto path = read_unicode_string(reader_, *image_file_name + offsets_[OBJECT_NAME_INFORMATION_Name]);
    if(!path)
        return name;

    return path::filename(*path).generic_string();
}

uint64_t OsNt::proc_id(proc_t proc)
{
    const auto pid = reader_.read(proc.id + offsets_[EPROCESS_UniqueProcessId]);
    if(!pid)
        return 0;

    return *pid;
}

bool OsNt::proc_ctx_is_x64()
{
    const auto cs         = core_.regs.read(FDP_CS_REGISTER);
    const auto WOW64_CS32 = 0x23;
    return cs != WOW64_CS32;
}

namespace
{
    opt<uint64_t> get_pspnotifyroutine_address(core::Core& core, uint64_t routine_addr, uint64_t count_addr)
    {
        struct _EX_CALLBACK_ROUTINE_BLOCK
        {
            uint64_t RundownProtect;
            uint64_t Function;
            uint64_t Context;
        };

        const auto reader = reader::make(core);
        const auto count  = reader.le32(count_addr);
        if(!count)
            FAIL({}, "unable to read pspnotifyroutinecount");

        if(*count == 0)
            FAIL({}, "no entry to pspnotifyroutine...");

        // For each routine type there is a global array of callbacks that contains up to 64 entries
        size_t max_entries  = 64;
        uint64_t ex_fastref = 0;
        for(size_t i = 0; i < max_entries; ++i)
        {
            const auto m = reader.read(routine_addr);
            if(!m)
                FAIL({}, "unable to read ex_callback_routine_block");

            if(*m == 0)
                continue;

            ex_fastref = *m;
            break;
        }

        // Check if we found a correct registered routine
        if(ex_fastref == 0)
            FAIL({}, "unable to find correct psnotifyroutine");

        const auto ex_callback_routine_block = ex_fastref & 0xFFFFFFFFFFFFFFF0;
        return reader.read(ex_callback_routine_block + offsetof(_EX_CALLBACK_ROUTINE_BLOCK, Function));
    }

    static bool register_callback_notifyroutine(core::Core& core, OsNt::Breakpoints& breakpoints, OsNt::ObsProcEvent& observers_proc_create, OsNt::ObsProcEvent& observers_proc_remove,
                                                uint64_t routine_addr, uint64_t count_addr, void (*callback)(core::Core&, OsNt::ObsProcEvent&, OsNt::ObsProcEvent&))
    {
        const auto addr = get_pspnotifyroutine_address(core, routine_addr, count_addr);
        if(!addr)
            FAIL(nullptr, "unable to find routine address");

        const auto coreptr                  = &core;
        const auto observers_proc_createptr = &observers_proc_create;
        const auto observers_proc_removeptr = &observers_proc_remove;
        const auto bp                       = core.state.set_breakpoint(*addr, [=]()
        {
            callback(*coreptr, *observers_proc_createptr, *observers_proc_removeptr);
        });
        if(!bp)
            return false;

        breakpoints.emplace_back(bp);
        return true;
    }

    void proc_on_event(core::Core& core, OsNt::ObsProcEvent& observers_proc_create, OsNt::ObsProcEvent& observers_proc_remove)
    {
        const auto proc_parent_id = core.regs.read(FDP_RCX_REGISTER);
        const auto proc_id        = core.regs.read(FDP_RDX_REGISTER);
        const auto created        = !!(core.regs.read(FDP_R8_REGISTER)); // boolean

        const auto proc_parent = core.os->proc_find(proc_parent_id);
        if(!proc_parent)
            return;

        const auto proc = core.os->proc_find(proc_id);
        if(!proc)
            return;

        if(created)
            for(const auto& it : observers_proc_create)
                it(*proc_parent, *proc);
        else
            for(const auto& it : observers_proc_remove)
                it(*proc_parent, *proc);
    }
}

bool OsNt::proc_listen_create(const on_proc_event_fn& on_create)
{
    if(observers_proc_create_.empty() && observers_proc_remove_.empty())
        if(!register_callback_notifyroutine(core_, breakpoints_, observers_proc_create_, observers_proc_remove_, symbols_[PspCreateProcessNotifyRoutine], symbols_[PspCreateProcessNotifyRoutine], &proc_on_event))
            return false;

    observers_proc_create_.push_back(on_create);
    return true;
}

bool OsNt::proc_listen_delete(const on_proc_event_fn& on_remove)
{
    if(observers_proc_create_.empty() && observers_proc_remove_.empty())
        if(!register_callback_notifyroutine(core_, breakpoints_, observers_proc_create_, observers_proc_remove_, symbols_[PspCreateProcessNotifyRoutine], symbols_[PspCreateProcessNotifyRoutine], &proc_on_event))
            return false;

    observers_proc_remove_.push_back(on_remove);
    return true;
}

namespace
{
    static opt<walk_e> mod_list_64(const OsNt& os, proc_t proc, const reader::Reader& reader, os::IModule::on_mod_fn on_mod)
    {
        const auto peb = reader.read(proc.id + os.offsets_[EPROCESS_Peb]);
        if(!peb)
            FAIL({}, "unable to read EPROCESS.Peb");

        // no PEB on system process
        if(!*peb)
            return WALK_NEXT;

        const auto ldr = reader.read(*peb + os.offsets_[PEB_Ldr]);
        if(!ldr)
            FAIL({}, "unable to read PEB.Ldr");

        const auto head = *ldr + os.offsets_[PEB_LDR_DATA_InLoadOrderModuleList];
        for(auto link = reader.read(head); link && link != head; link = reader.read(*link))
        {
            const auto ret = on_mod({*link - os.offsets_[LDR_DATA_TABLE_ENTRY_InLoadOrderLinks], FLAGS_NONE});
            if(ret == WALK_STOP)
                return ret;
        }

        return WALK_NEXT;
    }

    static opt<walk_e> mod_list_32(const OsNt& os, proc_t proc, const reader::Reader& reader, os::IModule::on_mod_fn on_mod)
    {
        const auto peb32 = read_wow64_peb(os, reader, proc);
        if(!peb32)
            return {};

        // no PEB on system process
        if(!*peb32)
            return WALK_NEXT;

        const auto ldr32 = reader.le32(*peb32 + os.offsets_[PEB32_Ldr]);
        if(!ldr32)
            FAIL({}, "unable to read PEB32.Ldr");

        const auto head = *ldr32 + offsetof32(nt32::_PEB_LDR_DATA, InLoadOrderModuleList);
        for(auto link = reader.le32(head); link && link != head; link = reader.le32(*link))
        {
            const auto ret = on_mod({*link - offsetof32(nt32::_LDR_DATA_TABLE_ENTRY, InLoadOrderLinks), FLAGS_32BIT});
            if(ret == WALK_STOP)
                return ret;
        }

        return WALK_NEXT;
    }
}

bool OsNt::mod_list(proc_t proc, const on_mod_fn& on_mod)
{
    const auto reader = reader::make(core_, proc);
    auto ret          = mod_list_64(*this, proc, reader, on_mod);
    if(!ret)
        return false;
    if(*ret == WALK_STOP)
        return true;

    ret = mod_list_32(*this, proc, reader, on_mod);
    return !!ret;
}

opt<std::string> OsNt::mod_name(proc_t proc, mod_t mod)
{
    const auto reader = reader::make(core_, proc);
    if(mod.flags & FLAGS_32BIT)
        return nt32::read_unicode_string(reader, mod.id + offsetof32(nt32::_LDR_DATA_TABLE_ENTRY, FullDllName));

    return read_unicode_string(reader, mod.id + offsets_[LDR_DATA_TABLE_ENTRY_FullDllName]);
}

opt<mod_t> OsNt::mod_find(proc_t proc, uint64_t addr)
{
    opt<mod_t> found = {};
    mod_list(proc, [&](mod_t mod)
    {
        const auto span = core_.os->mod_span(proc, mod);
        if(!span)
            return WALK_NEXT;

        if(!(span->addr <= addr && addr <= span->addr + span->size))
            return WALK_NEXT;

        found = mod;
        return WALK_STOP;
    });
    return found;
}

bool OsNt::proc_is_valid(proc_t proc)
{
    const auto vad_root = reader_.read(proc.id + offsets_[EPROCESS_VadRoot]);
    return vad_root && *vad_root;
}

flags_e OsNt::proc_flags(proc_t proc)
{
    const auto reader = reader::make(core_, proc);
    int flags         = FLAGS_NONE;
    const auto wow64  = reader.read(proc.id + offsets_[EPROCESS_Wow64Process]);
    if(wow64)
        flags |= FLAGS_32BIT;
    return static_cast<flags_e>(flags);
}

namespace
{
    static opt<span_t> mod_span_64(const OsNt& os, const reader::Reader& reader, mod_t mod)
    {
        const auto base = reader.read(mod.id + os.offsets_[LDR_DATA_TABLE_ENTRY_DllBase]);
        if(!base)
            return {};

        const auto size = reader.read(mod.id + os.offsets_[LDR_DATA_TABLE_ENTRY_SizeOfImage]);
        if(!size)
            return {};

        return span_t{*base, *size};
    }

    static opt<span_t> mod_span_32(const reader::Reader& reader, mod_t mod)
    {
        const auto base = reader.le32(mod.id + offsetof32(nt32::_LDR_DATA_TABLE_ENTRY, DllBase));
        if(!base)
            return {};

        const auto size = reader.le32(mod.id + offsetof32(nt32::_LDR_DATA_TABLE_ENTRY, SizeOfImage));
        if(!size)
            return {};

        return span_t{*base, *size};
    }
}

opt<span_t> OsNt::mod_span(proc_t proc, mod_t mod)
{
    const auto reader = reader::make(core_, proc);
    if(mod.flags & FLAGS_32BIT)
        return mod_span_32(reader, mod);

    return mod_span_64(*this, reader, mod);
}

bool OsNt::driver_list(const on_driver_fn& on_driver)
{
    const auto head = symbols_[PsLoadedModuleList];
    for(auto link = reader_.read(head); link != head; link = reader_.read(*link))
        if(on_driver({*link - offsets_[LDR_DATA_TABLE_ENTRY_InLoadOrderLinks]}) == WALK_STOP)
            break;
    return true;
}

opt<driver_t> OsNt::driver_find(const std::string& name)
{
    opt<driver_t> found;
    driver_list([&](driver_t driver)
    {
        const auto got = driver_name(driver);
        if(got != name)
            return WALK_NEXT;

        found = driver;
        return WALK_STOP;
    });
    return found;
}

opt<std::string> OsNt::driver_name(driver_t drv)
{
    return read_unicode_string(reader_, drv.id + offsets_[LDR_DATA_TABLE_ENTRY_FullDllName]);
}

opt<span_t> OsNt::driver_span(driver_t drv)
{
    const auto base = reader_.read(drv.id + offsets_[LDR_DATA_TABLE_ENTRY_DllBase]);
    if(!base)
        return {};

    const auto size = reader_.read(drv.id + offsets_[LDR_DATA_TABLE_ENTRY_SizeOfImage]);
    if(!size)
        return {};

    return span_t{*base, *size};
}

bool OsNt::thread_list(proc_t proc, const on_thread_fn& on_thread)
{
    const auto head = proc.id + offsets_[EPROCESS_ThreadListHead];
    for(auto link = reader_.read(head); link && link != head; link = reader_.read(*link))
        if(on_thread({*link - offsets_[ETHREAD_ThreadListEntry]}) == WALK_STOP)
            break;

    return true;
}

opt<thread_t> OsNt::thread_current()
{
    const auto thread = reader_.read(kpcr_ + offsets_[KPCR_Prcb] + offsets_[KPRCB_CurrentThread]);
    if(!thread)
        FAIL({}, "unable to read KPCR.Prcb.CurrentThread");

    return thread_t{*thread};
}

opt<proc_t> OsNt::thread_proc(thread_t thread)
{
    const auto kproc = reader_.read(thread.id + offsets_[KTHREAD_Process]);
    if(!kproc)
        FAIL({}, "unable to read KTHREAD.Process");

    const auto dtb = reader_.read(*kproc + offsets_[KPROCESS_UserDirectoryTableBase]);
    if(!dtb)
        FAIL({}, "unable to read KPROCESS.DirectoryTableBase");

    const auto eproc = *kproc - offsets_[EPROCESS_Pcb];
    return proc_t{eproc, dtb_t{*dtb}};
}

opt<uint64_t> OsNt::thread_pc(proc_t /*proc*/, thread_t thread)
{
    const auto ktrap_frame = reader_.read(thread.id + offsets_[ETHREAD_Tcb] + offsets_[KTHREAD_TrapFrame]);
    if(!ktrap_frame)
        FAIL({}, "unable to read KTHREAD.TrapFrame");

    if(!*ktrap_frame)
        return {};

    const auto rip = reader_.read(*ktrap_frame + offsets_[KTRAP_FRAME_Rip]);
    if(!rip)
        return {};

    return *rip; // rip can be null
}

uint64_t OsNt::thread_id(proc_t /*proc*/, thread_t thread)
{
    const auto tid = reader_.read(thread.id + offsets_[ETHREAD_Cid] + offsets_[CLIENT_ID_UniqueThread]);
    if(!tid)
        return 0;

    return *tid;
}

namespace
{
    void proc_join_kernel(OsNt& os, proc_t proc)
    {
        os.core_.state.run_to(proc);
    }

    void proc_join_user(OsNt& os, proc_t proc)
    {
        // if KiKernelSysretExit doesn't exist, KiSystemCall* in lstar has user return address in rcx
        const auto where = os.symbols_[KiKernelSysretExit] ? os.symbols_[KiKernelSysretExit] : os.core_.regs.read(MSR_LSTAR);
        os.core_.state.run_to(proc, where);
        const auto rip = os.core_.regs.read(FDP_RCX_REGISTER);
        os.core_.state.run_to(proc, rip);
    }
}

void OsNt::proc_join(proc_t proc, os::join_e join)
{
    if(join == os::JOIN_ANY_MODE)
        return proc_join_kernel(*this, proc);

    return proc_join_user(*this, proc);
}

opt<phy_t> OsNt::proc_resolve(proc_t proc, uint64_t ptr)
{
    const auto phy = core_.mem.virtual_to_physical(ptr, proc.dtb);
    if(phy)
        return phy;

    return core_.mem.virtual_to_physical(ptr, reader_.kdtb_);
}

bool OsNt::is_kernel(uint64_t ptr)
{
    return !!(ptr & 0xFFF0000000000000);
}

opt<proc_t> OsNt::proc_select(proc_t proc, uint64_t ptr)
{
    if(!is_kernel(ptr))
        return proc;

    const auto kdtb = reader_.read(proc.id + offsets_[EPROCESS_Pcb] + offsets_[KPROCESS_DirectoryTableBase]);
    if(!kdtb)
        return {};

    return proc_t{proc.id, dtb_t{*kdtb}};
}

bool OsNt::reader_setup(reader::Reader& reader, proc_t proc)
{
    const auto dtb = reader_.read(proc.id + offsets_[EPROCESS_Pcb] + offsets_[KPROCESS_UserDirectoryTableBase]);
    if(!dtb)
        return false;

    const auto kdtb = reader_.read(proc.id + offsets_[EPROCESS_Pcb] + offsets_[KPROCESS_DirectoryTableBase]);
    if(!kdtb)
        return false;

    reader.udtb_ = dtb_t{*dtb};
    reader.kdtb_ = dtb_t{*kdtb};
    return true;
}

namespace
{
    const char* irql_to_text(uint8_t value)
    {
        switch(value)
        {
            case 0: return "passive";
            case 1: return "apc";
            case 2: return "dispatch";
        }
        return "?";
    }

    bool is_user_mode(uint64_t cs)
    {
        return !!(cs & 3);
    }

    std::string to_hex(uint64_t x)
    {
        char buf[sizeof x * 2 + 1];
        return hex::convert<hex::LowerCase | hex::RemovePadding>(buf, x);
    }
}

void OsNt::debug_print()
{
    if(true)
        return;
    const auto irql   = reader_.byte(kpcr_ + offsets_[KPCR_Irql]);
    const auto cs     = core_.regs.read(FDP_CS_REGISTER);
    const auto rip    = core_.regs.read(FDP_RIP_REGISTER);
    const auto cr3    = core_.regs.read(FDP_CR3_REGISTER);
    const auto ripcur = core_.sym.find(rip);
    const auto ripsym = ripcur ? sym::to_string(*ripcur) : "";
    const auto thread = thread_current();
    const auto proc   = thread_proc(*thread);
    const auto name   = proc_name(*proc);
    const auto dump   = "rip: " + to_hex(rip)
                      + " cr3:" + to_hex(cr3)
                      + " dtb:" + to_hex(proc ? proc->dtb.val : 0)
                      + ' ' + irql_to_text(irql ? *irql : -1)
                      + ' ' + (is_user_mode(cs) ? "user" : "kernel")
                      + (name ? " " + *name : "")
                      + (ripsym.empty() ? "" : " " + ripsym)
                      + " p:" + to_hex(proc ? proc->id : 0)
                      + " t:" + to_hex(thread ? thread->id : 0);
    if(dump != last_dump_)
        LOG(INFO, "{}", dump.data());
    last_dump_ = dump;
}
