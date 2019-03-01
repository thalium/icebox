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

    enum offset_e
    {
        CLIENT_ID_UniqueThread,
        EPROCESS_ActiveProcessLinks,
        EPROCESS_ImageFileName,
        EPROCESS_Pcb,
        EPROCESS_Peb,
        EPROCESS_SeAuditProcessCreationInfo,
        EPROCESS_ThreadListHead,
        EPROCESS_ActiveThreads,
        EPROCESS_UniqueProcessId,
        EPROCESS_InheritedFromUniqueProcessId,
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
        LDR_DATA_TABLE_ENTRY_DllBase,
        LDR_DATA_TABLE_ENTRY_FullDllName,
        LDR_DATA_TABLE_ENTRY_InLoadOrderLinks,
        LDR_DATA_TABLE_ENTRY_SizeOfImage,
        OBJECT_NAME_INFORMATION_Name,
        PEB_Ldr,
        PEB_LDR_DATA_InLoadOrderModuleList,
        PEB32_Ldr,
        SE_AUDIT_PROCESS_CREATION_INFO_ImageFileName,
        EWOW64PROCESS_Peb,
        EWOW64PROCESS_NtdllType,
        OFFSET_COUNT,
    };

    struct NtOffset
    {
        cat_e      e_cat;
        offset_e   e_id;
        const char module[16];
        const char struc[32];
        const char member[32];
    };
    // clang-format off
    const NtOffset g_offsets[] =
    {
        {cat_e::REQUIRED,   CLIENT_ID_UniqueThread,                       "nt", "_CLIENT_ID",                       "UniqueThread"},
        {cat_e::REQUIRED,   EPROCESS_ActiveProcessLinks,                  "nt", "_EPROCESS",                        "ActiveProcessLinks"},
        {cat_e::REQUIRED,   EPROCESS_ImageFileName,                       "nt", "_EPROCESS",                        "ImageFileName"},
        {cat_e::REQUIRED,   EPROCESS_Pcb,                                 "nt", "_EPROCESS",                        "Pcb"},
        {cat_e::REQUIRED,   EPROCESS_Peb,                                 "nt", "_EPROCESS",                        "Peb"},
        {cat_e::REQUIRED,   EPROCESS_SeAuditProcessCreationInfo,          "nt", "_EPROCESS",                        "SeAuditProcessCreationInfo"},
        {cat_e::REQUIRED,   EPROCESS_ThreadListHead,                      "nt", "_EPROCESS",                        "ThreadListHead"},
        {cat_e::REQUIRED,   EPROCESS_ActiveThreads,                       "nt", "_EPROCESS",                        "ActiveThreads"},
        {cat_e::REQUIRED,   EPROCESS_UniqueProcessId,                     "nt", "_EPROCESS",                        "UniqueProcessId"},
        {cat_e::REQUIRED,   EPROCESS_InheritedFromUniqueProcessId,        "nt", "_EPROCESS",                        "InheritedFromUniqueProcessId"},
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
        {cat_e::REQUIRED,   LDR_DATA_TABLE_ENTRY_DllBase,                 "nt", "_LDR_DATA_TABLE_ENTRY",            "DllBase"},
        {cat_e::REQUIRED,   LDR_DATA_TABLE_ENTRY_FullDllName,             "nt", "_LDR_DATA_TABLE_ENTRY",            "FullDllName"},
        {cat_e::REQUIRED,   LDR_DATA_TABLE_ENTRY_InLoadOrderLinks,        "nt", "_LDR_DATA_TABLE_ENTRY",            "InLoadOrderLinks"},
        {cat_e::REQUIRED,   LDR_DATA_TABLE_ENTRY_SizeOfImage,             "nt", "_LDR_DATA_TABLE_ENTRY",            "SizeOfImage"},
        {cat_e::REQUIRED,   OBJECT_NAME_INFORMATION_Name,                 "nt", "_OBJECT_NAME_INFORMATION",         "Name"},
        {cat_e::REQUIRED,   PEB_Ldr,                                      "nt", "_PEB",                             "Ldr"},
        {cat_e::REQUIRED,   PEB_LDR_DATA_InLoadOrderModuleList,           "nt", "_PEB_LDR_DATA",                    "InLoadOrderModuleList"},
        {cat_e::REQUIRED,   PEB32_Ldr,                                    "nt", "_PEB32",                           "Ldr"},
        {cat_e::REQUIRED,   SE_AUDIT_PROCESS_CREATION_INFO_ImageFileName, "nt", "_SE_AUDIT_PROCESS_CREATION_INFO",  "ImageFileName"},
        {cat_e::OPTIONAL,   EWOW64PROCESS_Peb,                            "nt", "_EWOW64PROCESS",                   "Peb"},
        {cat_e::OPTIONAL,   EWOW64PROCESS_NtdllType,                      "nt", "_EWOW64PROCESS",                   "NtdllType"},
    };
    // clang-format on
    STATIC_ASSERT_EQ(COUNT_OF(g_offsets), OFFSET_COUNT);

    enum symbol_e
    {
        KiKernelSysretExit,
        KiSystemCall64,
        PsActiveProcessHead,
        PsInitialSystemProcess,
        PsLoadedModuleList,
        PsCallImageNotifyRoutines,
        PspInsertThread,
        PspExitProcess,
        PspExitThread,
        SYMBOL_COUNT,
    };

    struct NtSymbol
    {
        cat_e      e_cat;
        symbol_e   e_id;
        const char module[16];
        const char name[32];
    };
    // clang-format off
    const NtSymbol g_symbols[] =
    {
        {cat_e::OPTIONAL, KiKernelSysretExit,                  "nt", "KiKernelSysretExit"},
        {cat_e::REQUIRED, KiSystemCall64,                      "nt", "KiSystemCall64"},
        {cat_e::REQUIRED, PsActiveProcessHead,                 "nt", "PsActiveProcessHead"},
        {cat_e::REQUIRED, PsInitialSystemProcess,              "nt", "PsInitialSystemProcess"},
        {cat_e::REQUIRED, PsLoadedModuleList,                  "nt", "PsLoadedModuleList"},
        {cat_e::REQUIRED, PsCallImageNotifyRoutines,           "nt", "PsCallImageNotifyRoutines"},
        {cat_e::REQUIRED, PspInsertThread,                     "nt", "PspInsertThread"},
        {cat_e::REQUIRED, PspExitProcess,                      "nt", "PspExitProcess"},
        {cat_e::REQUIRED, PspExitThread,                       "nt", "PspExitThread"},
    };
    // clang-format on
    static_assert(COUNT_OF(g_symbols) == SYMBOL_COUNT, "invalid symbols");

    using NtOffsets = std::array<uint64_t, OFFSET_COUNT>;
    using NtSymbols = std::array<uint64_t, SYMBOL_COUNT>;

    struct OsNt
        : public os::IModule
    {
        OsNt(core::Core& core);

        // methods
        bool setup();

        // os::IModule
        bool    is_kernel_address   (uint64_t ptr) override;
        bool    can_inject_fault    (uint64_t ptr) override;
        bool    reader_setup        (reader::Reader& reader, proc_t proc) override;

        bool                proc_list           (on_proc_fn on_process) override;
        opt<proc_t>         proc_current        () override;
        opt<proc_t>         proc_find           (std::string_view name, flags_e flags) override;
        opt<proc_t>         proc_find           (uint64_t pid) override;
        opt<std::string>    proc_name           (proc_t proc) override;
        bool                proc_is_valid       (proc_t proc) override;
        flags_e             proc_flags          (proc_t proc) override;
        uint64_t            proc_id             (proc_t proc) override;
        void                proc_join           (proc_t proc, os::join_e join) override;
        opt<phy_t>          proc_resolve        (proc_t proc, uint64_t ptr) override;
        opt<proc_t>         proc_select         (proc_t proc, uint64_t ptr) override;
        opt<proc_t>         proc_parent         (proc_t proc) override;
        bool                proc_listen_create  (const on_proc_event_fn& on_proc_event) override;
        bool                proc_listen_delete  (const on_proc_event_fn& on_proc_event) override;

        bool            thread_list         (proc_t proc, on_thread_fn on_thread) override;
        opt<thread_t>   thread_current      () override;
        opt<proc_t>     thread_proc         (thread_t thread) override;
        opt<uint64_t>   thread_pc           (proc_t proc, thread_t thread) override;
        uint64_t        thread_id           (proc_t proc, thread_t thread) override;
        bool            thread_listen_create(const on_thread_event_fn& on_thread_event) override;
        bool            thread_listen_delete(const on_thread_event_fn& on_thread_event) override;

        bool                mod_list        (proc_t proc, on_mod_fn on_module) override;
        opt<std::string>    mod_name        (proc_t proc, mod_t mod) override;
        opt<span_t>         mod_span        (proc_t proc, mod_t mod) override;
        opt<mod_t>          mod_find        (proc_t proc, uint64_t addr) override;
        bool                mod_listen_load (const on_mod_event_fn& on_load) override;

        bool                driver_list (on_driver_fn on_driver) override;
        opt<driver_t>       driver_find (std::string_view name) override;
        opt<std::string>    driver_name (driver_t drv) override;
        opt<span_t>         driver_span (driver_t drv) override;

        opt<arg_t>  read_stack  (size_t index) override;
        opt<arg_t>  read_arg    (size_t index) override;
        bool        write_arg   (size_t index, arg_t arg) override;

        void debug_print() override;

        // members
        core::Core&    core_;
        NtOffsets      offsets_;
        NtSymbols      symbols_;
        std::string    last_dump_;
        uint64_t       kpcr_;
        reader::Reader reader_;

        using Breakpoints     = std::vector<core::Breakpoint>;
        using ProcListeners   = std::vector<on_proc_event_fn>;
        using ThreadListeners = std::vector<on_thread_event_fn>;
        using ModListeners    = std::vector<on_mod_event_fn>;

        Breakpoints     breakpoints_;
        ProcListeners   observers_proc_create_;
        ProcListeners   observers_proc_delete_;
        ThreadListeners observers_thread_create_;
        ThreadListeners observers_thread_delete_;
        ModListeners    observers_mod_load_;
        opt<phy_t>      LdrpInsertDataTableEntry;
        opt<phy_t>      LdrpInsertDataTableEntry32;
    };
}

OsNt::OsNt(core::Core& core)
    : core_(core)
    , reader_(reader::make(core))
{
}

namespace
{
    static bool is_kernel(uint64_t ptr)
    {
        return !!(ptr & 0xFFF0000000000000);
    }

    static opt<span_t> find_kernel(core::Memory& mem, uint64_t lstar)
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
        return FAIL(false, "unable to find kernel");

    LOG(INFO, "kernel: {:#x} - {:#x} ({} {:#x})", kernel->addr, kernel->addr + kernel->size, kernel->size, kernel->size);
    std::vector<uint8_t> buffer(kernel->size);
    auto ok = core_.mem.read_virtual(&buffer[0], kernel->addr, kernel->size);
    if(!ok)
        return FAIL(false, "unable to read kernel module");

    ok = core_.sym.insert("nt", *kernel, &buffer[0], buffer.size());
    if(!ok)
        return FAIL(false, "unable to load symbols from kernel module");

    bool fail = false;
    int i     = -1;
    memset(&symbols_[0], 0, sizeof symbols_);
    for(const auto& sym : g_symbols)
    {
        fail |= sym.e_id != ++i;
        const auto addr = core_.sym.symbol(sym.module, sym.name);
        if(!addr)
        {
            fail |= sym.e_cat == cat_e::REQUIRED;
            if(sym.e_cat == cat_e::REQUIRED)
                LOG(ERROR, "unable to read {}!{} symbol offset", sym.module, sym.name);
            else
                LOG(WARNING, "unable to read {}!{} symbol offset", sym.module, sym.name);
            continue;
        }

        symbols_[i] = *addr;
    }

    i = -1;
    memset(&offsets_[0], 0, sizeof offsets_);
    for(const auto& off : g_offsets)
    {
        fail |= off.e_id != ++i;
        const auto offset = core_.sym.struc_offset(off.module, off.struc, off.member);
        if(!offset)
        {
            fail |= off.e_cat == cat_e::REQUIRED;
            if(off.e_cat == cat_e::REQUIRED)
                LOG(ERROR, "unable to read {}!{}.{} member offset", off.module, off.struc, off.member);
            else
                LOG(WARNING, "unable to read {}!{}.{} member offset", off.module, off.struc, off.member);
            continue;
        }
        offsets_[i] = *offset;
    }
    if(fail)
        return false;

    kpcr_ = core_.regs.read(MSR_GS_BASE);
    if(!is_kernel(kpcr_))
        kpcr_ = core_.regs.read(MSR_KERNEL_GS_BASE);
    if(!is_kernel(kpcr_))
        return FAIL(false, "unable to read KPCR");

    auto gdtb = dtb_t{core_.regs.read(FDP_CR3_REGISTER)};
    if(offsets_[KPRCB_KernelDirectoryTableBase])
    {
        ok = core_.mem.read_virtual(&gdtb, kpcr_ + offsets_[KPCR_Prcb] + offsets_[KPRCB_KernelDirectoryTableBase], sizeof gdtb);
        if(!ok)
            return FAIL(false, "unable to read KPRCB.KernelDirectoryTableBase");
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

namespace
{
    static opt<proc_t> make_proc(OsNt& os, uint64_t eproc)
    {
        const auto dtb = os.reader_.read(eproc + os.offsets_[EPROCESS_Pcb] + os.offsets_[KPROCESS_UserDirectoryTableBase]);
        if(!dtb)
        {
            LOG(ERROR, "unable to read KPROCESS.UserDirectoryTableBase from {:#x}", eproc);
            return {};
        }

        return proc_t{eproc, {*dtb}};
    }
}

bool OsNt::proc_list(on_proc_fn on_process)
{
    const auto head = symbols_[PsActiveProcessHead];
    for(auto link = reader_.read(head); link != head; link = reader_.read(*link))
    {
        const auto eproc = *link - offsets_[EPROCESS_ActiveProcessLinks];
        const auto proc  = make_proc(*this, eproc);
        if(!proc)
            continue;

        const auto err = on_process(*proc);
        if(err == WALK_STOP)
            break;
    }
    return true;
}

opt<proc_t> OsNt::proc_current()
{
    const auto current = thread_current();
    if(!current)
        return FAIL(ext::nullopt, "unable to get current thread");

    return thread_proc(*current);
}

opt<proc_t> OsNt::proc_find(std::string_view name, flags_e flags)
{
    opt<proc_t> found;
    proc_list([&](proc_t proc)
    {
        const auto got = proc_name(proc);
        if(*got != name)
            return WALK_NEXT;

        const auto f = proc_flags(proc);
        if(flags && !(f & flags))
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
    static opt<std::string> read_unicode_string(const reader::Reader& reader, uint64_t unicode_string)
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
            return FAIL(ext::nullopt, "unable to read UNICODE_STRING");

        us.length     = read_le16(&us.length);
        us.max_length = read_le16(&us.max_length);
        us.buffer     = read_le64(&us.buffer);

        if(us.length > us.max_length)
            return FAIL(ext::nullopt, "corrupted UNICODE_STRING");

        std::vector<uint8_t> buffer(us.length);
        ok = reader.read(&buffer[0], us.buffer, us.length);
        if(!ok)
            return FAIL(ext::nullopt, "unable to read UNICODE_STRING.buffer");

        const auto p = &buffer[0];
        return utf8::convert(p, &p[us.length]);
    }

    namespace nt32
    {
        static opt<std::string> read_unicode_string(const reader::Reader& reader, uint64_t unicode_string)
        {
            nt32::_UNICODE_STRING us;
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
    } // namespace nt32

    static opt<uint64_t> read_wow64_peb(const OsNt& os, const reader::Reader& reader, proc_t proc)
    {
        const auto wowp = reader.read(proc.id + os.offsets_[EPROCESS_Wow64Process]);
        if(!wowp)
            return FAIL(false, "unable to read EPROCESS.Wow64Process");

        if(!os.offsets_[EWOW64PROCESS_NtdllType])
            return wowp;

        const auto peb32 = reader.read(*wowp + os.offsets_[EWOW64PROCESS_Peb]);
        if(!peb32)
            return FAIL(false, "unable to read EWOW64PROCESS.Peb");

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

namespace
{
    template <typename T>
    static bool listen_to(OsNt& os, T addr, void (*callback)(OsNt&))
    {
        const auto osptr = &os;
        const auto bp    = os.core_.state.set_breakpoint(addr, [=]
        {
            callback(*osptr);
        });
        if(!bp)
            return false;

        os.breakpoints_.emplace_back(bp);
        return true;
    }

    template <typename T, typename U, typename V>
    static bool register_listener(OsNt& os, T& listeners, U listener, V addr, void (*callback)(OsNt&))
    {
        if(listeners.empty())
            if(!listen_to(os, addr, callback))
                return false;

        listeners.emplace_back(listener);
        return true;
    }

    static void on_PspInsertThread(OsNt& os)
    {
        const auto thread = os.core_.regs.read(FDP_RCX_REGISTER);
        for(const auto& it : os.observers_thread_create_)
            it({thread});

        // Check if it is a CreateProcess (if ActiveThreads = 0)
        const auto eproc          = os.core_.regs.read(FDP_RDX_REGISTER);
        const auto active_threads = os.reader_.le32(eproc + os.offsets_[EPROCESS_ActiveThreads]);
        if(!active_threads)
            return;

        if(*active_threads)
            return;

        const auto proc = make_proc(os, eproc);
        if(!proc)
            return;

        for(const auto& it : os.observers_proc_create_)
            it(*proc);
    }

    static void on_PspExitProcess(OsNt& os)
    {
        const auto eproc = os.core_.regs.read(FDP_RDX_REGISTER);
        const auto proc  = make_proc(os, eproc);
        if(!proc)
            return;

        for(const auto& it : os.observers_proc_delete_)
            it(*proc);
    }

    static void on_PspExitThread(OsNt& os)
    {
        const auto thread = os.thread_current();
        if(!thread)
            return;

        for(const auto& it : os.observers_thread_delete_)
            it(*thread);
    }
}

bool OsNt::proc_listen_create(const on_proc_event_fn& on_create)
{
    return register_listener(*this, observers_proc_create_, on_create,
                             symbols_[PspInsertThread], &on_PspInsertThread);
}

bool OsNt::proc_listen_delete(const on_proc_event_fn& on_delete)
{
    return register_listener(*this, observers_proc_delete_, on_delete,
                             symbols_[PspExitProcess], &on_PspExitProcess);
}

bool OsNt::thread_listen_create(const on_thread_event_fn& on_create)
{
    return register_listener(*this, observers_thread_create_, on_create,
                             symbols_[PspInsertThread], &on_PspInsertThread);
}

bool OsNt::thread_listen_delete(const on_thread_event_fn& on_delete)
{
    return register_listener(*this, observers_thread_delete_, on_delete,
                             symbols_[PspExitThread], &on_PspExitThread);
}

namespace
{
    static std::shared_ptr<sym::Symbols> insert_pdb(const reader::Reader& reader, span_t span, const std::string& name)
    {
        std::shared_ptr<sym::Symbols> sym;
        const auto debug = pe::find_debug_codeview(reader, span);
        if(!debug)
            return sym;

        std::vector<uint8_t> buffer(debug->size);
        const auto ok = reader.read(&buffer[0], debug->addr, debug->size);
        if(!ok)
            return sym;

        auto tmp            = std::make_shared<sym::Symbols>();
        const auto inserted = tmp->insert(name, span, &buffer[0], buffer.size());
        if(!inserted)
            return sym;

        sym = std::move(tmp);
        return sym;
    }

    static opt<span_t> get_ntdll_span(OsNt& os, proc_t proc, bool is_32bit)
    {
        opt<span_t> found;
        os.mod_list(proc, [&](mod_t mod)
        {
            const auto name = os.mod_name(proc, mod);
            if(!name)
                return WALK_NEXT;

            const auto filename = path::filename(*name);
            if(filename != "ntdll.dll")
                return WALK_NEXT;

            const auto is_wow64 = !!(mod.flags & FLAGS_32BIT);
            if(is_32bit && !is_wow64)
                return WALK_NEXT;

            found = os.mod_span(proc, mod);
            return WALK_STOP;
        });
        return found;
    }

    static opt<span_t> get_ntdll_span_on_event(OsNt& os, proc_t proc, bool is_32bit)
    {
        struct _IMAGE_INFO
        {
            uint64_t Properties;
            uint64_t ImageAddressingMode;
            uint64_t ImageBase;
            uint64_t ImageSelector;
            uint64_t ImageSize;
            uint64_t ImageSectionNumber;
        };

        const auto name_addr = os.core_.regs.read(FDP_RCX_REGISTER);

        const auto reader   = reader::make(os.core_, proc);
        const auto mod_name = read_unicode_string(reader, name_addr);
        if(!mod_name)
            return {};

        const auto filename = path::filename(*mod_name);
        if(filename != "ntdll.dll")
            return {};

        const auto image_info_addr = os.core_.regs.read(FDP_R8_REGISTER);
        const auto base            = reader.read(image_info_addr + offsetof(_IMAGE_INFO, ImageBase));
        if(!base)
            return {};

        // ntdll32 is always mapped at an address under the maximum 32 bit address
        const auto max_32bit_addr = 0xffffffff;
        const auto is_wow64       = *base <= max_32bit_addr;
        if((is_32bit && !is_wow64) || (!is_32bit && is_wow64))
            return {};

        const auto size = reader.read(image_info_addr + offsetof(_IMAGE_INFO, ImageSize));
        if(!size)
            return {};

        return span_t{*base, *size};
    }

    static opt<span_t> try_get_ntdll_span(OsNt& os, proc_t proc, bool is_32bit)
    {
        const auto span = get_ntdll_span(os, proc, is_32bit);
        if(!span)
            return get_ntdll_span_on_event(os, proc, is_32bit);

        return span;
    }

    static opt<phy_t> get_phy_from_sym(OsNt& os, proc_t proc, const std::shared_ptr<sym::Symbols>& sym, const std::string& mod_name, const std::string& sym_name)
    {
        const auto addr = sym->symbol(mod_name, sym_name);
        if(!addr)
            return {};

        return os.proc_resolve(proc, *addr);
    }

    constexpr auto x86_cs = 0x23;

    static void on_LdrpInsertDataTableEntry(OsNt& os)
    {
        const auto proc = os.proc_current();
        if(!proc)
            return;

        const auto cs       = os.core_.regs.read(FDP_CS_REGISTER);
        const auto is_32bit = cs == x86_cs;

        // LdrpInsertDataTableEntry has a fastcall calling convention whether it's in ntdll or ntdll32
        const auto rcx      = os.core_.regs.read(FDP_RCX_REGISTER);
        const auto mod_addr = is_32bit ? static_cast<uint32_t>(rcx) : rcx;
        const auto flags    = is_32bit ? FLAGS_32BIT : FLAGS_NONE;

        for(const auto& it : os.observers_mod_load_)
            it(*proc, {mod_addr, flags});
    }

    // Wait for kernel notification
    static void on_PsCallImageNotifyRoutines(OsNt& os)
    {
        if(os.LdrpInsertDataTableEntry && os.LdrpInsertDataTableEntry32)
            return;

        const auto proc = os.proc_current();
        if(!proc)
            return;

        opt<span_t> ntdll;
        opt<span_t> ntdll32;
        if(!os.LdrpInsertDataTableEntry)
            ntdll = try_get_ntdll_span(os, *proc, false);

        const auto flags = os.proc_flags(*proc);
        if(!os.LdrpInsertDataTableEntry32)
            if(flags & FLAGS_32BIT)
                ntdll32 = try_get_ntdll_span(os, *proc, true);

        if(!ntdll && !ntdll32)
            return;

        os.proc_join(*proc, os::JOIN_USER_MODE);

        const auto reader = reader::make(os.core_, *proc);
        if(!os.LdrpInsertDataTableEntry && ntdll)
        {
            const auto sym = insert_pdb(reader, *ntdll, "ntdll");
            if(!sym)
                return;

            os.LdrpInsertDataTableEntry = get_phy_from_sym(os, *proc, sym, "ntdll", "LdrpInsertDataTableEntry");
            if(os.LdrpInsertDataTableEntry)
                if(!listen_to(os, *os.LdrpInsertDataTableEntry, &on_LdrpInsertDataTableEntry))
                    return;
        }
        if(!os.LdrpInsertDataTableEntry32 && ntdll32)
        {
            const auto sym = insert_pdb(reader, *ntdll32, "ntdll");
            if(!sym)
                return;

            os.LdrpInsertDataTableEntry32 = get_phy_from_sym(os, *proc, sym, "ntdll", "_LdrpInsertDataTableEntry@4");
            if(os.LdrpInsertDataTableEntry32)
                if(!listen_to(os, *os.LdrpInsertDataTableEntry32, &on_LdrpInsertDataTableEntry))
                    return;
        }
    }
}

bool OsNt::mod_listen_load(const on_mod_event_fn& on_load)
{
    return register_listener(*this, observers_mod_load_, on_load,
                             symbols_[PsCallImageNotifyRoutines], &on_PsCallImageNotifyRoutines);
}

namespace
{
    static opt<walk_e> mod_list_64(const OsNt& os, proc_t proc, const reader::Reader& reader, os::IModule::on_mod_fn on_mod)
    {
        const auto peb = reader.read(proc.id + os.offsets_[EPROCESS_Peb]);
        if(!peb)
            return FAIL(ext::nullopt, "unable to read EPROCESS.Peb");

        // no PEB on system process
        if(!*peb)
            return WALK_NEXT;

        const auto ldr = reader.read(*peb + os.offsets_[PEB_Ldr]);
        if(!ldr)
            return FAIL(ext::nullopt, "unable to read PEB.Ldr");

        // Ldr = 0 before the process loads it's first module
        if(!*ldr)
            return WALK_NEXT;

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
            return FAIL(ext::nullopt, "unable to read PEB32.Ldr");

        // Ldr = 0 before the process loads it's first module
        if(!*ldr32)
            return WALK_NEXT;

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

bool OsNt::mod_list(proc_t proc, on_mod_fn on_mod)
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
    if(*wow64)
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

bool OsNt::driver_list(on_driver_fn on_driver)
{
    const auto head = symbols_[PsLoadedModuleList];
    for(auto link = reader_.read(head); link != head; link = reader_.read(*link))
        if(on_driver({*link - offsets_[LDR_DATA_TABLE_ENTRY_InLoadOrderLinks]}) == WALK_STOP)
            break;
    return true;
}

opt<driver_t> OsNt::driver_find(std::string_view name)
{
    opt<driver_t> found;
    driver_list([&](driver_t driver)
    {
        const auto got = driver_name(driver);
        if(*got != name)
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

bool OsNt::thread_list(proc_t proc, on_thread_fn on_thread)
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
        return FAIL(ext::nullopt, "unable to read KPCR.Prcb.CurrentThread");

    return thread_t{*thread};
}

opt<proc_t> OsNt::thread_proc(thread_t thread)
{
    const auto kproc = reader_.read(thread.id + offsets_[KTHREAD_Process]);
    if(!kproc)
        return FAIL(ext::nullopt, "unable to read KTHREAD.Process");

    const auto dtb = reader_.read(*kproc + offsets_[KPROCESS_UserDirectoryTableBase]);
    if(!dtb)
        return FAIL(ext::nullopt, "unable to read KPROCESS.DirectoryTableBase");

    const auto eproc = *kproc - offsets_[EPROCESS_Pcb];
    return proc_t{eproc, dtb_t{*dtb}};
}

opt<uint64_t> OsNt::thread_pc(proc_t /*proc*/, thread_t thread)
{
    const auto ktrap_frame = reader_.read(thread.id + offsets_[ETHREAD_Tcb] + offsets_[KTHREAD_TrapFrame]);
    if(!ktrap_frame)
        return FAIL(ext::nullopt, "unable to read KTHREAD.TrapFrame");

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
    static void proc_join_kernel(OsNt& os, proc_t proc)
    {
        os.core_.state.run_to(proc);
    }

    static void proc_join_user(OsNt& os, proc_t proc)
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

namespace
{
    static bool is_user_mode(uint64_t cs)
    {
        return !!(cs & 3);
    }
}

bool OsNt::is_kernel_address(uint64_t ptr)
{
    return is_kernel(ptr);
}

bool OsNt::can_inject_fault(uint64_t ptr)
{
    if(is_kernel_address(ptr))
        return false;

    return is_user_mode(core_.regs.read(FDP_CS_REGISTER));
}

opt<proc_t> OsNt::proc_select(proc_t proc, uint64_t ptr)
{
    if(!is_kernel_address(ptr))
        return proc;

    const auto kdtb = reader_.read(proc.id + offsets_[EPROCESS_Pcb] + offsets_[KPROCESS_DirectoryTableBase]);
    if(!kdtb)
        return {};

    return proc_t{proc.id, dtb_t{*kdtb}};
}

opt<proc_t> OsNt::proc_parent(proc_t proc)
{
    const auto reader     = reader::make(core_, proc);
    const auto parent_pid = reader.read(proc.id + offsets_[EPROCESS_InheritedFromUniqueProcessId]);
    if(!parent_pid)
        return {};

    return proc_find(*parent_pid);
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
    static opt<arg_t> to_arg(opt<uint64_t> arg)
    {
        if(!arg)
            return {};

        return arg_t{*arg};
    }

    static opt<arg_t> read_stack32(const reader::Reader& reader, uint64_t sp, size_t index)
    {
        return to_arg(reader.le32(sp + index * sizeof(uint32_t)));
    }

    static bool write_stack32(core::Core& /*core*/, size_t /*index*/, uint32_t /*arg*/)
    {
        LOG(ERROR, "not implemented");
        return false;
    }

    static opt<arg_t> read_stack64(const reader::Reader& reader, uint64_t sp, size_t index)
    {
        return to_arg(reader.le64(sp + index * sizeof(uint64_t)));
    }

    static bool write_stack64(core::Core& /*core*/, size_t /*index*/, uint64_t /*arg*/)
    {
        LOG(ERROR, "not implemented");
        return false;
    }

    static opt<arg_t> read_arg32(const reader::Reader& reader, uint64_t sp, size_t index)
    {
        return read_stack32(reader, sp, index + 1);
    }

    static bool write_arg32(core::Core& core, size_t index, arg_t arg)
    {
        return write_stack32(core, index + 1, static_cast<uint32_t>(arg.val));
    }

    static opt<arg_t> read_arg64(core::Core& core, const reader::Reader& reader, uint64_t sp, size_t index)
    {
        switch(index)
        {
            case 0:     return to_arg      (core.regs.read(FDP_RCX_REGISTER));
            case 1:     return to_arg      (core.regs.read(FDP_RDX_REGISTER));
            case 2:     return to_arg      (core.regs.read(FDP_R8_REGISTER));
            case 3:     return to_arg      (core.regs.read(FDP_R9_REGISTER));
            default:    return read_stack64(reader, sp, index + 1);
        }
    }

    static bool write_arg64(core::Core& core, size_t index, arg_t arg)
    {
        switch(index)
        {
            case 0:     return core.regs.write(FDP_RCX_REGISTER, arg.val);
            case 1:     return core.regs.write(FDP_RDX_REGISTER, arg.val);
            case 2:     return core.regs.write(FDP_R8_REGISTER, arg.val);
            case 3:     return core.regs.write(FDP_R9_REGISTER, arg.val);
            default:    return write_stack64(core, index + 1, arg.val);
        }
    }
}

opt<arg_t> OsNt::read_stack(size_t index)
{
    const auto cs       = core_.regs.read(FDP_CS_REGISTER);
    const auto is_32bit = cs == x86_cs;
    const auto sp       = core_.regs.read(FDP_RSP_REGISTER);
    const auto reader   = reader::make(core_);
    if(is_32bit)
        return read_stack32(reader, sp, index);

    return read_stack64(reader, sp, index);
}

opt<arg_t> OsNt::read_arg(size_t index)
{
    const auto cs       = core_.regs.read(FDP_CS_REGISTER);
    const auto is_32bit = cs == x86_cs;
    const auto sp       = core_.regs.read(FDP_RSP_REGISTER);
    const auto reader   = reader::make(core_);
    if(is_32bit)
        return read_arg32(reader, sp, index);

    return read_arg64(core_, reader, sp, index);
}

bool OsNt::write_arg(size_t index, arg_t arg)
{
    const auto cs       = core_.regs.read(FDP_CS_REGISTER);
    const auto is_32bit = cs == x86_cs;
    if(is_32bit)
        return write_arg32(core_, index, arg);

    return write_arg64(core_, index, arg);
}

namespace
{
    static const char* irql_to_text(uint8_t value)
    {
        switch(value)
        {
            case 0: return "passive";
            case 1: return "apc";
            case 2: return "dispatch";
        }
        return "?";
    }

    static std::string to_hex(uint64_t x)
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
