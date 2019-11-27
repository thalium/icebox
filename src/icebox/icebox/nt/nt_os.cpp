#include "os.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "nt_os"
#include "core.hpp"
#include "core/core_private.hpp"
#include "interfaces/if_os.hpp"
#include "interfaces/if_symbols.hpp"
#include "log.hpp"
#include "nt.hpp"
#include "nt_objects.hpp"
#include "reader.hpp"
#include "utils/hex.hpp"
#include "utils/path.hpp"
#include "utils/pe.hpp"
#include "utils/utf8.hpp"
#include "utils/utils.hpp"
#include "wow64.hpp"

#include <array>
#include <map>

namespace
{
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
        OBJECT_NAME_INFORMATION_Name,
        PEB_Ldr,
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
        {cat_e::REQUIRED,   OBJECT_NAME_INFORMATION_Name,                 "nt", "_OBJECT_NAME_INFORMATION",         "Name"},
        {cat_e::REQUIRED,   PEB_Ldr,                                      "nt", "_PEB",                             "Ldr"},
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
        PsActiveProcessHead,
        PsLoadedModuleList,
        PspInsertThread,
        PspExitProcess,
        PspExitThread,
        MiProcessLoaderEntry,
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
        {cat_e::REQUIRED, PsActiveProcessHead,                 "nt", "PsActiveProcessHead"},
        {cat_e::REQUIRED, PsLoadedModuleList,                  "nt", "PsLoadedModuleList"},
        {cat_e::REQUIRED, PspInsertThread,                     "nt", "PspInsertThread"},
        {cat_e::REQUIRED, PspExitProcess,                      "nt", "PspExitProcess"},
        {cat_e::REQUIRED, PspExitThread,                       "nt", "PspExitThread"},
        {cat_e::REQUIRED, MiProcessLoaderEntry,                "nt", "MiProcessLoaderEntry"},
    };
    // clang-format on
    static_assert(COUNT_OF(g_symbols) == SYMBOL_COUNT, "invalid symbols");

    using NtOffsets = std::array<uint64_t, OFFSET_COUNT>;
    using NtSymbols = std::array<uint64_t, SYMBOL_COUNT>;

    using bpid_t      = os::bpid_t;
    using Breakpoints = std::multimap<bpid_t, state::Breakpoint>;

    struct NtOs
        : public os::Module
    {
        NtOs(core::Core& core);

        // os::IModule
        bool    setup               () override;
        bool    is_kernel_address   (uint64_t ptr) override;
        bool    can_inject_fault    (uint64_t ptr) override;
        bool    reader_setup        (reader::Reader& reader, opt<proc_t> proc) override;

        bool                proc_list       (process::on_proc_fn on_process) override;
        opt<proc_t>         proc_current    () override;
        opt<proc_t>         proc_find       (std::string_view name, flags_t flags) override;
        opt<proc_t>         proc_find       (uint64_t pid) override;
        opt<std::string>    proc_name       (proc_t proc) override;
        bool                proc_is_valid   (proc_t proc) override;
        flags_t             proc_flags      (proc_t proc) override;
        uint64_t            proc_id         (proc_t proc) override;
        void                proc_join       (proc_t proc, mode_e mode) override;
        opt<phy_t>          proc_resolve    (proc_t proc, uint64_t ptr) override;
        opt<proc_t>         proc_select     (proc_t proc, uint64_t ptr) override;
        opt<proc_t>         proc_parent     (proc_t proc) override;

        bool            thread_list     (proc_t proc, threads::on_thread_fn on_thread) override;
        opt<thread_t>   thread_current  () override;
        opt<proc_t>     thread_proc     (thread_t thread) override;
        opt<uint64_t>   thread_pc       (proc_t proc, thread_t thread) override;
        uint64_t        thread_id       (proc_t proc, thread_t thread) override;

        bool                mod_list(proc_t proc, modules::on_mod_fn on_module) override;
        opt<std::string>    mod_name(proc_t proc, mod_t mod) override;
        opt<span_t>         mod_span(proc_t proc, mod_t mod) override;
        opt<mod_t>          mod_find(proc_t proc, uint64_t addr) override;

        bool                vm_area_list    (proc_t proc, vm_area::on_vm_area_fn on_vm_area) override;
        opt<vm_area_t>      vm_area_find    (proc_t proc, uint64_t addr) override;
        opt<span_t>         vm_area_span    (proc_t proc, vm_area_t vm_area) override;
        vma_access_e        vm_area_access  (proc_t proc, vm_area_t vm_area) override;
        vma_type_e          vm_area_type    (proc_t proc, vm_area_t vm_area) override;
        opt<std::string>    vm_area_name    (proc_t proc, vm_area_t vm_area) override;

        bool                driver_list (drivers::on_driver_fn on_driver) override;
        opt<driver_t>       driver_find (uint64_t addr) override;
        opt<std::string>    driver_name (driver_t drv) override;
        opt<span_t>         driver_span (driver_t drv) override;

        opt<bpid_t> listen_proc_create  (const process::on_event_fn& on_proc_event) override;
        opt<bpid_t> listen_proc_delete  (const process::on_event_fn& on_proc_event) override;
        opt<bpid_t> listen_thread_create(const threads::on_event_fn& on_thread_event) override;
        opt<bpid_t> listen_thread_delete(const threads::on_event_fn& on_thread_event) override;
        opt<bpid_t> listen_mod_create   (proc_t proc, flags_t flags, const modules::on_event_fn& on_load) override;
        opt<bpid_t> listen_drv_create   (const drivers::on_event_fn& on_drv) override;
        size_t      unlisten            (bpid_t bpid) override;

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
        bpid_t         last_bpid_;
        Breakpoints    breakpoints_;
        phy_t          LdrpInitializeProcess_;
        phy_t          LdrpProcessMappedModule_;
    };
}

NtOs::NtOs(core::Core& core)
    : core_(core)
    , kpcr_(0)
    , reader_(reader::make(core))
    , last_bpid_(0)
    , LdrpInitializeProcess_{0}
    , LdrpProcessMappedModule_{0}
{
}

namespace
{
    static bool is_kernel(uint64_t ptr)
    {
        return !!(ptr & 0xFFF0000000000000);
    }

    static opt<span_t> find_kernel_at(core::Core& core, uint64_t needle)
    {
        uint8_t buf[PAGE_SIZE];
        for(auto ptr = utils::align<PAGE_SIZE>(needle); ptr < needle; ptr -= PAGE_SIZE)
        {
            auto ok = memory::read_virtual(core, buf, ptr, sizeof buf);
            if(!ok)
                return {};

            const auto size = pe::read_image_size(buf, sizeof buf);
            if(!size)
                continue;

            return span_t{ptr, *size};
        }

        return {};
    }

    static opt<span_t> find_kernel(NtOs& os, core::Core& core)
    {
        // try until we get a CR3 able to read kernel base address & size
        const auto lstar = registers::read_msr(core, msr_e::lstar);
        for(size_t i = 0; i < 64; ++i)
        {
            if(const auto kernel = find_kernel_at(core, lstar))
                return kernel;

            const auto ok = state::run_fast_to_cr3_write(core);
            if(!ok)
                break;

            os.reader_.kdtb_.val = registers::read(core, reg_e::cr3);
        }
        return {};
    }

    static bool read_phy_symbol(NtOs& os, phy_t& dst, proc_t proc, const char* module, const char* name)
    {
        const auto where = symbols::symbol(os.core_, symbols::kernel, module, name);
        if(!where)
            return false;

        const auto phy = memory::virtual_to_physical(os.core_, *where, proc.dtb);
        if(!phy)
            return false;

        dst = *phy;
        return true;
    }

    static bool try_load_ntdll(NtOs& os, core::Core& core)
    {
        const auto sysret_exit = os.symbols_[KiKernelSysretExit] ? os.symbols_[KiKernelSysretExit] : registers::read_msr(core, msr_e::lstar);
        auto bp                = state::break_on(core, "KiKernelSysretExit", sysret_exit, {});

        state::resume(core);
        state::wait(core);
        const auto ret_addr = registers::read(core, reg_e::rcx);
        bp                  = state::break_on(core, "KiKernelSysretExit return", ret_addr, {});

        state::resume(core);
        state::wait(core);
        const auto proc = os.proc_current();
        if(!proc)
            return false;

        const auto ntdll = modules::find_name(core, *proc, "ntdll.dll", flags::x64);
        if(!ntdll)
            return false;

        const auto span = modules::span(core, *proc, *ntdll);
        if(!span)
            return false;

        const auto reader = reader::make(core, *proc);
        const auto opt_id = symbols::identify_pdb(*span, reader);
        if(!opt_id)
            return false;

        const auto pdb = symbols::make_pdb(opt_id->name, opt_id->id);
        if(!pdb)
            return false;

        auto ok = core.symbols_->insert(symbols::kernel, "ntdll", *span, pdb);
        if(!ok)
            return false;

        ok = read_phy_symbol(os, os.LdrpInitializeProcess_, *proc, "ntdll", "LdrpInitializeProcess");
        if(!ok)
            return false;

        ok = read_phy_symbol(os, os.LdrpProcessMappedModule_, *proc, "ntdll", "LdrpProcessMappedModule");
        if(!ok)
            return false;

        return true;
    }

    bool update_kernel_dtb(NtOs& os)
    {
        auto gdtb = dtb_t{registers::read(os.core_, reg_e::cr3)};
        if(os.offsets_[KPRCB_KernelDirectoryTableBase])
        {
            const auto ok = memory::read_virtual(os.core_, &gdtb, os.kpcr_ + os.offsets_[KPCR_Prcb] + os.offsets_[KPRCB_KernelDirectoryTableBase], sizeof gdtb);
            if(!ok)
                return FAIL(false, "unable to read KPRCB.KernelDirectoryTableBase");
        }
        os.reader_.kdtb_ = gdtb;
        return true;
    }
}

bool NtOs::setup()
{
    kpcr_ = registers::read_msr(core_, msr_e::gs_base);
    if(!is_kernel(kpcr_))
        kpcr_ = registers::read_msr(core_, msr_e::kernel_gs_base);
    if(!is_kernel(kpcr_))
        return FAIL(false, "unable to read KPCR");

    const auto kernel = find_kernel(*this, core_);
    if(!kernel)
        return FAIL(false, "unable to find kernel");

    LOG(INFO, "kernel: 0x%" PRIx64 " - 0x%" PRIx64 " (%zu 0x%" PRIx64 ")",
        kernel->addr, kernel->addr + kernel->size, kernel->size, kernel->size);
    auto ok = symbols::load_module_at(core_, symbols::kernel, "nt", *kernel);
    if(!ok)
        return FAIL(false, "unable to load symbols from kernel module");

    bool fail = false;
    int i     = -1;
    memset(&symbols_[0], 0, sizeof symbols_);
    for(const auto& sym : g_symbols)
    {
        fail |= sym.e_id != ++i;
        const auto addr = symbols::symbol(core_, symbols::kernel, sym.module, sym.name);
        if(!addr)
        {
            fail |= sym.e_cat == cat_e::REQUIRED;
            if(sym.e_cat == cat_e::REQUIRED)
                LOG(ERROR, "unable to read %s!%s symbol offset", sym.module, sym.name);
            else
                LOG(WARNING, "unable to read optional %s!%s symbol offset", sym.module, sym.name);
            continue;
        }
        symbols_[i] = *addr;
    }

    i = -1;
    memset(&offsets_[0], 0, sizeof offsets_);
    for(const auto& off : g_offsets)
    {
        fail |= off.e_id != ++i;
        const auto offset = symbols::struc_offset(core_, symbols::kernel, off.module, off.struc, off.member);
        if(!offset)
        {
            fail |= off.e_cat == cat_e::REQUIRED;
            if(off.e_cat == cat_e::REQUIRED)
                LOG(ERROR, "unable to read %s!%s.%s member offset", off.module, off.struc, off.member);
            else
                LOG(WARNING, "unable to read optional %s!%s.%s member offset", off.module, off.struc, off.member);
            continue;
        }
        offsets_[i] = *offset;
    }
    if(fail)
        return false;

    // cr3 is same in user & kernel mode
    if(!offsets_[KPROCESS_UserDirectoryTableBase])
        offsets_[KPROCESS_UserDirectoryTableBase] = offsets_[KPROCESS_DirectoryTableBase];

    // read current kernel dtb, just to have a good one to join system process
    ok = update_kernel_dtb(*this);
    if(!ok)
        return false;

    // join system process kernel side
    constexpr auto system_pid = 4;
    const auto proc           = proc_find(system_pid);
    if(!proc)
        return FAIL(false, "unable to find system process");

    proc_join(*proc, mode_e::kernel);

    // now update kernel dtb with the one read from system process
    ok = update_kernel_dtb(*this);
    if(!ok)
        return false;

    LOG(WARNING, "kernel: kpcr: 0x%" PRIx64 " kdtb: 0x%" PRIx64, kpcr_, reader_.kdtb_.val);
    return try_load_ntdll(*this, core_);
}

std::unique_ptr<os::Module> os::make_nt(core::Core& core)
{
    return std::make_unique<NtOs>(core);
}

namespace
{
    static opt<dtb_t> get_user_dtb(NtOs& os, uint64_t kprocess)
    {
        const auto dtb = os.reader_.read(kprocess + os.offsets_[KPROCESS_UserDirectoryTableBase]);
        if(dtb && *dtb != 0 && *dtb != 1)
            return dtb_t{*dtb};

        if(os.offsets_[KPROCESS_DirectoryTableBase] == os.offsets_[KPROCESS_UserDirectoryTableBase])
            return {};

        const auto kdtb = os.reader_.read(kprocess + os.offsets_[KPROCESS_DirectoryTableBase]);
        if(!kdtb)
            return {};

        return dtb_t{*kdtb};
    }

    static opt<proc_t> make_proc(NtOs& os, uint64_t eproc)
    {
        const auto dtb = get_user_dtb(os, eproc + os.offsets_[EPROCESS_Pcb]);
        if(!dtb)
        {
            LOG(ERROR, "unable to read dtb from 0x%" PRIx64, eproc);
            return {};
        }

        return proc_t{eproc, *dtb};
    }
}

bool NtOs::proc_list(process::on_proc_fn on_process)
{
    const auto head = symbols_[PsActiveProcessHead];
    for(auto link = reader_.read(head); link != head; link = reader_.read(*link))
    {
        const auto eproc = *link - offsets_[EPROCESS_ActiveProcessLinks];
        const auto proc  = make_proc(*this, eproc);
        if(!proc)
            continue;

        const auto err = on_process(*proc);
        if(err == walk_e::stop)
            break;
    }
    return true;
}

opt<proc_t> NtOs::proc_current()
{
    const auto current = thread_current();
    if(!current)
        return FAIL(ext::nullopt, "unable to get current thread");

    return thread_proc(*current);
}

opt<proc_t> NtOs::proc_find(std::string_view name, flags_t flags)
{
    opt<proc_t> found;
    proc_list([&](proc_t proc)
    {
        const auto got = proc_name(proc);
        if(*got != name)
            return walk_e::next;

        const auto f = proc_flags(proc);
        if(!os::check_flags(f, flags))
            return walk_e::next;

        found = proc;
        return walk_e::stop;
    });
    return found;
}

opt<proc_t> NtOs::proc_find(uint64_t pid)
{
    opt<proc_t> found;
    proc_list([&](proc_t proc)
    {
        const auto got = proc_id(proc);
        if(got != pid)
            return walk_e::next;

        found = proc;
        return walk_e::stop;
    });
    return found;
}

namespace
{
    static opt<uint64_t> read_wow64_peb(const NtOs& os, const reader::Reader& reader, proc_t proc)
    {
        const auto wowp = reader.read(proc.id + os.offsets_[EPROCESS_Wow64Process]);
        if(!wowp)
            return FAIL(ext::nullopt, "unable to read EPROCESS.Wow64Process");

        if(!*wowp)
            return {};

        if(!os.offsets_[EWOW64PROCESS_NtdllType])
            return wowp;

        const auto peb32 = reader.read(*wowp + os.offsets_[EWOW64PROCESS_Peb]);
        if(!peb32)
            return FAIL(ext::nullopt, "unable to read EWOW64PROCESS.Peb");

        return *peb32;
    }

#define offsetof32(x, y) static_cast<uint32_t>(offsetof(x, y))
}

opt<std::string> NtOs::proc_name(proc_t proc)
{
    // EPROCESS.ImageFileName is 16 bytes, but only 14 are actually used
    char buffer[14 + 1];
    const auto ok             = reader_.read_all(buffer, proc.id + offsets_[EPROCESS_ImageFileName], sizeof buffer);
    buffer[sizeof buffer - 1] = 0;
    if(!ok)
        return {};

    const auto name = std::string{buffer};
    if(name.size() < sizeof buffer - 1)
        return name;

    const auto image_file_name = reader_.read(proc.id + offsets_[EPROCESS_SeAuditProcessCreationInfo] + offsets_[SE_AUDIT_PROCESS_CREATION_INFO_ImageFileName]);
    if(!image_file_name)
        return name;

    const auto path = nt::read_unicode_string(reader_, *image_file_name + offsets_[OBJECT_NAME_INFORMATION_Name]);
    if(!path)
        return name;

    return path::filename(*path).generic_string();
}

uint64_t NtOs::proc_id(proc_t proc)
{
    const auto pid = reader_.read(proc.id + offsets_[EPROCESS_UniqueProcessId]);
    if(!pid)
        return 0;

    return *pid;
}

namespace
{
    template <typename T, typename U, typename V>
    static opt<bpid_t> listen_to(NtOs& os, bpid_t bpid, std::string_view name, T addr, const U& on_value, V callback)
    {
        const auto osptr = &os;
        const auto bp    = state::break_on(os.core_, name, addr, [=]
        {
            callback(*osptr, bpid, on_value);
        });
        if(!bp)
            return {};

        os.breakpoints_.emplace(bpid, bp);
        return bpid;
    }

    template <typename T>
    static opt<bpid_t> register_listener(NtOs& os, std::string_view name, uint64_t addr, const T& on_value, void (*callback)(NtOs&, bpid_t, const T&))
    {
        return listen_to(os, ++os.last_bpid_, name, addr, on_value, callback);
    }

    static void on_PspInsertThread(NtOs& os, bpid_t, const threads::on_event_fn& on_thread)
    {
        const auto thread = registers::read(os.core_, reg_e::rcx);
        on_thread({thread});
    }

    static void on_PspExitProcess(NtOs& os, bpid_t, const process::on_event_fn& on_proc)
    {
        const auto eproc = registers::read(os.core_, reg_e::rdx);
        if(const auto proc = make_proc(os, eproc))
            on_proc(*proc);
    }

    static void on_PspExitThread(NtOs& os, bpid_t, const threads::on_event_fn& on_thread)
    {
        if(const auto thread = os.thread_current())
            on_thread(*thread);
    }
}

opt<bpid_t> NtOs::listen_proc_create(const process::on_event_fn& on_create)
{
    const auto bpid = ++last_bpid_;
    const auto bp   = state::break_on_physical(core_, "LdrpInitializeProcess", LdrpInitializeProcess_, [=]
    {
        const auto proc = process::current(core_);
        if(!proc)
            return;

        on_create(*proc);
    });
    breakpoints_.emplace(bpid, bp);
    return bpid;
}

opt<bpid_t> NtOs::listen_proc_delete(const process::on_event_fn& on_delete)
{
    return register_listener(*this, "PspExitProcess", symbols_[PspExitProcess], on_delete, &on_PspExitProcess);
}

opt<bpid_t> NtOs::listen_thread_create(const threads::on_event_fn& on_create)
{
    return register_listener(*this, "PspInsertThread", symbols_[PspInsertThread], on_create, &on_PspInsertThread);
}

opt<bpid_t> NtOs::listen_thread_delete(const threads::on_event_fn& on_delete)
{
    return register_listener(*this, "PspExitThread", symbols_[PspExitThread], on_delete, &on_PspExitThread);
}

namespace
{
    constexpr auto x86_cs = 0x23;

    static void on_LdrpInsertDataTableEntry(NtOs& os, bpid_t, const modules::on_event_fn& on_mod)
    {
        const auto cs       = registers::read(os.core_, reg_e::cs);
        const auto is_32bit = cs == x86_cs;

        // LdrpInsertDataTableEntry has a fastcall calling convention whether it's in ntdll or ntdll32
        const auto rcx      = registers::read(os.core_, reg_e::rcx);
        const auto mod_addr = is_32bit ? static_cast<uint32_t>(rcx) : rcx;
        const auto flags    = is_32bit ? flags::x86 : flags::x64;
        on_mod({mod_addr, flags});
    }

    static opt<bpid_t> replace_bp(NtOs& os, bpid_t bpid, const state::Breakpoint& bp)
    {
        os.breakpoints_.erase(bpid);
        if(!bp)
            return {};

        os.breakpoints_.emplace(bpid, bp);
        return bpid;
    }

    static opt<bpid_t> try_on_LdrpProcessMappedModule(NtOs& os, proc_t proc, bpid_t bpid, const modules::on_event_fn& on_mod)
    {
        const auto where = symbols::symbol(os.core_, proc, "wntdll", "_LdrpProcessMappedModule@16");
        if(!where)
            return {};

        const auto ptr = &os;
        const auto bp  = state::break_on_process(os.core_, "wntdll!_LdrpProcessMappedModule@16", proc, *where, [=]
        {
            on_LdrpInsertDataTableEntry(*ptr, bpid, on_mod);
        });
        return replace_bp(os, bpid, bp);
    }

    static void on_LdrpInsertDataTableEntry_wow64(NtOs& os, proc_t proc, bpid_t bpid, const modules::on_event_fn& on_mod)
    {
        const auto objects = objects::make(os.core_, proc);
        if(!objects)
            return;

        const auto entry = objects::find(*objects, 0, "\\KnownDlls32\\ntdll.dll");
        if(!entry)
            return;

        const auto section = objects::as_section(*entry);
        if(!section)
            return;

        const auto control_area = objects::section_control_area(*objects, *section);
        if(!control_area)
            return;

        const auto segment = objects::control_area_segment(*objects, *control_area);
        if(!segment)
            return;

        const auto span = objects::segment_span(*objects, *segment);
        if(!span)
            return;

        const auto inserted = symbols::load_module_at(os.core_, symbols::kernel, "wntdll", *span);
        if(!inserted)
            return;

        try_on_LdrpProcessMappedModule(os, proc, bpid, on_mod);
    }
}

opt<bpid_t> NtOs::listen_mod_create(proc_t proc, flags_t flags, const modules::on_event_fn& on_mod)
{
    const auto bpid = ++last_bpid_;
    const auto name = "ntdll!LdrpProcessMappedModule";
    if(flags.is_x86)
    {
        const auto opt_bpid = try_on_LdrpProcessMappedModule(*this, proc, bpid, on_mod);
        if(opt_bpid)
            return opt_bpid;

        const auto bp = state::break_on_physical_process(core_, name, proc, LdrpProcessMappedModule_, [=]
        {
            on_LdrpInsertDataTableEntry_wow64(*this, proc, bpid, on_mod);
        });
        return replace_bp(*this, bpid, bp);
    }

    const auto bp = state::break_on_physical_process(core_, name, proc, LdrpProcessMappedModule_, [=]
    {
        on_LdrpInsertDataTableEntry(*this, bpid, on_mod);
    });
    return replace_bp(*this, bpid, bp);
}

opt<bpid_t> NtOs::listen_drv_create(const drivers::on_event_fn& on_drv)
{
    const auto bpid = ++last_bpid_;
    const auto ok   = listen_to(*this, bpid, "MiProcessLoaderEntry", symbols_[MiProcessLoaderEntry], on_drv, [](NtOs& os, bpid_t /*bpid*/, const auto& on_drv)
    {
        const auto drv_addr   = registers::read(os.core_, reg_e::rcx);
        const auto drv_loaded = registers::read(os.core_, reg_e::rdx);
        on_drv({drv_addr}, drv_loaded);
    });
    if(!ok)
        return {};

    return bpid;
}

size_t NtOs::unlisten(bpid_t bpid)
{
    return breakpoints_.erase(bpid);
}

namespace
{
    static opt<walk_e> mod_list_64(const NtOs& os, proc_t proc, const reader::Reader& reader, const modules::on_mod_fn& on_mod)
    {
        const auto peb = reader.read(proc.id + os.offsets_[EPROCESS_Peb]);
        if(!peb)
            return FAIL(ext::nullopt, "unable to read EPROCESS.Peb");

        // no PEB on system process
        if(!*peb)
            return walk_e::next;

        const auto ldr = reader.read(*peb + os.offsets_[PEB_Ldr]);
        if(!ldr)
            return FAIL(ext::nullopt, "unable to read PEB.Ldr");

        // Ldr = 0 before the process loads it's first module
        if(!*ldr)
            return walk_e::next;

        const auto head = *ldr + offsetof(nt::_PEB_LDR_DATA, InLoadOrderModuleList);
        for(auto link = reader.read(head); link && link != head; link = reader.read(*link))
        {
            const auto ret = on_mod({*link - offsetof(nt::_LDR_DATA_TABLE_ENTRY, InLoadOrderLinks), flags::x64});
            if(ret == walk_e::stop)
                return ret;
        }

        return walk_e::next;
    }

    static opt<walk_e> mod_list_32(const NtOs& os, proc_t proc, const reader::Reader& reader, const modules::on_mod_fn& on_mod)
    {
        const auto peb32 = read_wow64_peb(os, reader, proc);
        if(!peb32)
            return {};

        // no PEB on system process
        if(!*peb32)
            return walk_e::next;

        const auto ldr32 = reader.le32(*peb32 + os.offsets_[PEB32_Ldr]);
        if(!ldr32)
            return FAIL(ext::nullopt, "unable to read PEB32.Ldr");

        // Ldr = 0 before the process loads it's first module
        if(!*ldr32)
            return walk_e::next;

        const auto head = *ldr32 + offsetof32(wow64::_PEB_LDR_DATA, InLoadOrderModuleList);
        for(auto link = reader.le32(head); link && link != head; link = reader.le32(*link))
        {
            const auto ret = on_mod({*link - offsetof32(wow64::_LDR_DATA_TABLE_ENTRY, InLoadOrderLinks), flags::x86});
            if(ret == walk_e::stop)
                return ret;
        }

        return walk_e::next;
    }
}

bool NtOs::mod_list(proc_t proc, modules::on_mod_fn on_mod)
{
    const auto reader = reader::make(core_, proc);
    auto ret          = mod_list_64(*this, proc, reader, on_mod);
    if(!ret)
        return false;
    if(*ret == walk_e::stop)
        return true;

    ret = mod_list_32(*this, proc, reader, on_mod);
    return !!ret;
}

opt<std::string> NtOs::mod_name(proc_t proc, mod_t mod)
{
    const auto reader = reader::make(core_, proc);
    if(mod.flags.is_x86)
        return wow64::read_unicode_string(reader, mod.id + offsetof32(wow64::_LDR_DATA_TABLE_ENTRY, FullDllName));

    return nt::read_unicode_string(reader, mod.id + offsetof(nt::_LDR_DATA_TABLE_ENTRY, FullDllName));
}

opt<mod_t> NtOs::mod_find(proc_t proc, uint64_t addr)
{
    opt<mod_t> found = {};
    mod_list(proc, [&](mod_t mod)
    {
        const auto span = mod_span(proc, mod);
        if(!span)
            return walk_e::next;

        if(!(span->addr <= addr && addr < span->addr + span->size))
            return walk_e::next;

        found = mod;
        return walk_e::stop;
    });
    return found;
}

bool NtOs::proc_is_valid(proc_t proc)
{
    const auto vad_root = reader_.read(proc.id + offsets_[EPROCESS_VadRoot]);
    return vad_root && *vad_root;
}

flags_t NtOs::proc_flags(proc_t proc)
{
    const auto reader = reader::make(core_, proc);
    auto flags        = flags_t{};
    const auto wow64  = reader.read(proc.id + offsets_[EPROCESS_Wow64Process]);
    if(*wow64)
        flags.is_x86 = true;
    else
        flags.is_x64 = true;
    return flags;
}

namespace
{
    template <typename T>
    static opt<span_t> read_ldr_span(const reader::Reader& reader, uint64_t ptr)
    {
        auto entry    = T{};
        const auto ok = reader.read_all(&entry, ptr, sizeof entry);
        if(!ok)
            return {};

        return span_t{entry.DllBase, entry.SizeOfImage};
    }
}

opt<span_t> NtOs::mod_span(proc_t proc, mod_t mod)
{
    const auto reader = reader::make(core_, proc);
    if(mod.flags.is_x86)
        return read_ldr_span<wow64::_LDR_DATA_TABLE_ENTRY>(reader, mod.id);

    return read_ldr_span<nt::_LDR_DATA_TABLE_ENTRY>(reader, mod.id);
}

bool NtOs::vm_area_list(proc_t /*proc*/, vm_area::on_vm_area_fn /*on_vm_area*/)
{
    return false;
}

opt<vm_area_t> NtOs::vm_area_find(proc_t /*proc*/, uint64_t /*addr*/)
{
    return {};
}

opt<span_t> NtOs::vm_area_span(proc_t /*proc*/, vm_area_t /*vm_area*/)
{
    return {};
}

vma_access_e NtOs::vm_area_access(proc_t /*proc*/, vm_area_t /*vm_area*/)
{
    return VMA_ACCESS_NONE;
}

vma_type_e NtOs::vm_area_type(proc_t /*proc*/, vm_area_t /*vm_area*/)
{
    return vma_type_e::none;
}

opt<std::string> NtOs::vm_area_name(proc_t /*proc*/, vm_area_t /*vm_area*/)
{
    return {};
}

bool NtOs::driver_list(drivers::on_driver_fn on_driver)
{
    const auto head = symbols_[PsLoadedModuleList];
    for(auto link = reader_.read(head); link != head; link = reader_.read(*link))
        if(on_driver({*link - offsetof(nt::_LDR_DATA_TABLE_ENTRY, InLoadOrderLinks)}) == walk_e::stop)
            break;
    return true;
}

opt<driver_t> NtOs::driver_find(uint64_t addr)
{
    opt<driver_t> found;
    driver_list([&](driver_t drv)
    {
        const auto span = driver_span(drv);
        if(!span)
            return walk_e::next;

        if(!(span->addr <= addr && addr < span->addr + span->size))
            return walk_e::next;

        found = drv;
        return walk_e::stop;
    });
    return found;
}

opt<std::string> NtOs::driver_name(driver_t drv)
{
    return nt::read_unicode_string(reader_, drv.id + offsetof(nt::_LDR_DATA_TABLE_ENTRY, FullDllName));
}

opt<span_t> NtOs::driver_span(driver_t drv)
{
    return read_ldr_span<nt::_LDR_DATA_TABLE_ENTRY>(reader_, drv.id);
}

bool NtOs::thread_list(proc_t proc, threads::on_thread_fn on_thread)
{
    const auto head = proc.id + offsets_[EPROCESS_ThreadListHead];
    for(auto link = reader_.read(head); link && link != head; link = reader_.read(*link))
        if(on_thread({*link - offsets_[ETHREAD_ThreadListEntry]}) == walk_e::stop)
            break;

    return true;
}

opt<thread_t> NtOs::thread_current()
{
    const auto thread = reader_.read(kpcr_ + offsets_[KPCR_Prcb] + offsets_[KPRCB_CurrentThread]);
    if(!thread)
        return FAIL(ext::nullopt, "unable to read KPCR.Prcb.CurrentThread");

    return thread_t{*thread};
}

opt<proc_t> NtOs::thread_proc(thread_t thread)
{
    const auto kproc = reader_.read(thread.id + offsets_[KTHREAD_Process]);
    if(!kproc)
        return FAIL(ext::nullopt, "unable to read KTHREAD.Process");

    const auto dtb = get_user_dtb(*this, *kproc);
    if(!dtb)
        return FAIL(ext::nullopt, "unable to read dtb");

    const auto eproc = *kproc - offsets_[EPROCESS_Pcb];
    return proc_t{eproc, *dtb};
}

opt<uint64_t> NtOs::thread_pc(proc_t /*proc*/, thread_t thread)
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

uint64_t NtOs::thread_id(proc_t /*proc*/, thread_t thread)
{
    const auto tid = reader_.read(thread.id + offsets_[ETHREAD_Cid] + offsets_[CLIENT_ID_UniqueThread]);
    if(!tid)
        return 0;

    return *tid;
}

namespace
{
    static void proc_join_kernel(NtOs& os, proc_t proc)
    {
        state::run_to_proc(os.core_, "proc_join_kernel", proc);
    }

    static void proc_join_user(NtOs& os, proc_t proc)
    {
        // if KiKernelSysretExit doesn't exist, KiSystemCall* in lstar has user return address in rcx
        const auto where = os.symbols_[KiKernelSysretExit] ? os.symbols_[KiKernelSysretExit] : registers::read_msr(os.core_, msr_e::lstar);
        state::run_to_proc_at(os.core_, "KiKernelSysretExit", proc, where);
        const auto rip = registers::read(os.core_, reg_e::rcx);
        state::run_to_proc_at(os.core_, "return KiKernelSysretExit", proc, rip);
    }

    static bool is_user_mode(uint64_t cs)
    {
        return !!(cs & 3);
    }
}

void NtOs::proc_join(proc_t proc, mode_e mode)
{
    const auto current   = proc_current();
    const auto same_proc = current && current->id == proc.id;
    const auto user_mode = is_user_mode(registers::read(core_, reg_e::cs));
    if(mode == mode_e::kernel && same_proc && !user_mode)
        return;

    if(mode == mode_e::kernel)
        return proc_join_kernel(*this, proc);

    if(same_proc && user_mode)
        return;

    return proc_join_user(*this, proc);
}

opt<phy_t> NtOs::proc_resolve(proc_t proc, uint64_t ptr)
{
    const auto phy = memory::virtual_to_physical(core_, ptr, proc.dtb);
    if(phy)
        return phy;

    return memory::virtual_to_physical(core_, ptr, reader_.kdtb_);
}

bool NtOs::is_kernel_address(uint64_t ptr)
{
    return is_kernel(ptr);
}

bool NtOs::can_inject_fault(uint64_t ptr)
{
    if(is_kernel_address(ptr))
        return false;

    return is_user_mode(registers::read(core_, reg_e::cs));
}

opt<proc_t> NtOs::proc_select(proc_t proc, uint64_t ptr)
{
    if(!is_kernel_address(ptr))
        return proc;

    const auto kdtb = reader_.read(proc.id + offsets_[EPROCESS_Pcb] + offsets_[KPROCESS_DirectoryTableBase]);
    if(!kdtb)
        return {};

    return proc_t{proc.id, dtb_t{*kdtb}};
}

opt<proc_t> NtOs::proc_parent(proc_t proc)
{
    const auto reader     = reader::make(core_, proc);
    const auto parent_pid = reader.read(proc.id + offsets_[EPROCESS_InheritedFromUniqueProcessId]);
    if(!parent_pid)
        return {};

    return proc_find(*parent_pid);
}

bool NtOs::reader_setup(reader::Reader& reader, opt<proc_t> proc)
{
    if(!proc)
    {
        reader.kdtb_ = reader_.kdtb_;
        return true;
    }

    const auto dtb = get_user_dtb(*this, proc->id + offsets_[EPROCESS_Pcb]);
    if(!dtb)
        return false;

    const auto kdtb = reader_.read(proc->id + offsets_[EPROCESS_Pcb] + offsets_[KPROCESS_DirectoryTableBase]);
    if(!kdtb)
        return false;

    reader.udtb_ = *dtb;
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
            case 0:     return to_arg      (registers::read(core, reg_e::rcx));
            case 1:     return to_arg      (registers::read(core, reg_e::rdx));
            case 2:     return to_arg      (registers::read(core, reg_e::r8));
            case 3:     return to_arg      (registers::read(core, reg_e::r9));
            default:    return read_stack64(reader, sp, index + 1);
        }
    }

    static bool write_arg64(core::Core& core, size_t index, arg_t arg)
    {
        switch(index)
        {
            case 0:     return registers::write(core, reg_e::rcx, arg.val);
            case 1:     return registers::write(core, reg_e::rdx, arg.val);
            case 2:     return registers::write(core, reg_e::r8, arg.val);
            case 3:     return registers::write(core, reg_e::r9, arg.val);
            default:    return write_stack64(core, index + 1, arg.val);
        }
    }
}

opt<arg_t> NtOs::read_stack(size_t index)
{
    const auto cs       = registers::read(core_, reg_e::cs);
    const auto is_32bit = cs == x86_cs;
    const auto sp       = registers::read(core_, reg_e::rsp);
    const auto reader   = reader::make(core_);
    if(is_32bit)
        return read_stack32(reader, sp, index);

    return read_stack64(reader, sp, index);
}

opt<arg_t> NtOs::read_arg(size_t index)
{
    const auto cs       = registers::read(core_, reg_e::cs);
    const auto is_32bit = cs == x86_cs;
    const auto sp       = registers::read(core_, reg_e::rsp);
    const auto reader   = reader::make(core_);
    if(is_32bit)
        return read_arg32(reader, sp, index);

    return read_arg64(core_, reader, sp, index);
}

bool NtOs::write_arg(size_t index, arg_t arg)
{
    const auto cs       = registers::read(core_, reg_e::cs);
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

void NtOs::debug_print()
{
    if(true)
        return;
    const auto irql   = reader_.byte(kpcr_ + offsets_[KPCR_Irql]);
    const auto cs     = registers::read(core_, reg_e::cs);
    const auto rip    = registers::read(core_, reg_e::rip);
    const auto cr3    = registers::read(core_, reg_e::cr3);
    const auto thread = thread_current();
    const auto proc   = thread_proc(*thread);
    const auto ripcur = symbols::find(core_, *proc, rip);
    const auto ripsym = symbols::to_string(ripcur);
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
        LOG(INFO, "%s", dump.data());
    last_dump_ = dump;
}
