#include "os.hpp"

#define FDP_MODULE "os_nt"
#include "log.hpp"
#include "core.hpp"
#include "core/helpers.hpp"
#include "utils/utils.hpp"
#include "utils/utf8.hpp"
#include "utils/pe.hpp"
#include "utils/hex.hpp"

#include <array>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

namespace
{
    enum member_offset_e
    {
        EPROCESS_ActiveProcessLinks,
        EPROCESS_ImageFileName,
        EPROCESS_Pcb,
        EPROCESS_Peb,
        EPROCESS_SeAuditProcessCreationInfo,
        EPROCESS_ThreadListHead,
        EPROCESS_VadRoot,
        ETHREAD_Tcb,
        ETHREAD_ThreadListEntry,
        KPCR_Irql,
        KPCR_Prcb,
        KPRCB_CurrentThread,
        KPROCESS_DirectoryTableBase,
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
        PEB_ProcessParameters,
        RTL_USER_PROCESS_PARAMETERS_ImagePathName,
        SE_AUDIT_PROCESS_CREATION_INFO_ImageFileName,
        MEMBER_OFFSET_COUNT,
    };

    struct MemberOffset
    {
        member_offset_e e_id;
        const char      module[16];
        const char      struc[32];
        const char      member[32];
    };
    const MemberOffset g_member_offsets[] =
    {
        {EPROCESS_ActiveProcessLinks,                   "nt", "_EPROCESS",                        "ActiveProcessLinks"},
        {EPROCESS_ImageFileName,                        "nt", "_EPROCESS",                        "ImageFileName"},
        {EPROCESS_Pcb,                                  "nt", "_EPROCESS",                        "Pcb"},
        {EPROCESS_Peb,                                  "nt", "_EPROCESS",                        "Peb"},
        {EPROCESS_SeAuditProcessCreationInfo,           "nt", "_EPROCESS",                        "SeAuditProcessCreationInfo"},
        {EPROCESS_ThreadListHead,                       "nt", "_EPROCESS",                        "ThreadListHead"},
        {EPROCESS_VadRoot,                              "nt", "_EPROCESS",                        "VadRoot"},
        {ETHREAD_Tcb,                                   "nt", "_ETHREAD",                         "Tcb"},
        {ETHREAD_ThreadListEntry,                       "nt", "_ETHREAD",                         "ThreadListEntry"},
        {KPCR_Irql,                                     "nt", "_KPCR",                            "Irql"},
        {KPCR_Prcb,                                     "nt", "_KPCR",                            "Prcb"},
        {KPRCB_CurrentThread,                           "nt", "_KPRCB",                           "CurrentThread"},
        {KPROCESS_DirectoryTableBase,                   "nt", "_KPROCESS",                        "DirectoryTableBase"},
        {KTHREAD_Process,                               "nt", "_KTHREAD",                         "Process"},
        {KTHREAD_TrapFrame,                             "nt", "_KTHREAD",                         "TrapFrame"},
        {KTRAP_FRAME_Rip,                               "nt", "_KTRAP_FRAME",                     "Rip"},
        {LDR_DATA_TABLE_ENTRY_DllBase,                  "nt", "_LDR_DATA_TABLE_ENTRY",            "DllBase"},
        {LDR_DATA_TABLE_ENTRY_FullDllName,              "nt", "_LDR_DATA_TABLE_ENTRY",            "FullDllName"},
        {LDR_DATA_TABLE_ENTRY_InLoadOrderLinks,         "nt", "_LDR_DATA_TABLE_ENTRY",            "InLoadOrderLinks"},
        {LDR_DATA_TABLE_ENTRY_SizeOfImage,              "nt", "_LDR_DATA_TABLE_ENTRY",            "SizeOfImage"},
        {OBJECT_NAME_INFORMATION_Name,                  "nt", "_OBJECT_NAME_INFORMATION",         "Name"},
        {PEB_Ldr,                                       "nt", "_PEB",                             "Ldr"},
        {PEB_LDR_DATA_InLoadOrderModuleList,            "nt", "_PEB_LDR_DATA",                    "InLoadOrderModuleList"},
        {PEB_ProcessParameters,                         "nt", "_PEB",                             "ProcessParameters"},
        {RTL_USER_PROCESS_PARAMETERS_ImagePathName,     "nt", "_RTL_USER_PROCESS_PARAMETERS",     "ImagePathName"},
        {SE_AUDIT_PROCESS_CREATION_INFO_ImageFileName,  "nt", "_SE_AUDIT_PROCESS_CREATION_INFO",  "ImageFileName"},
    };
    static_assert(COUNT_OF(g_member_offsets) == MEMBER_OFFSET_COUNT, "invalid members");

    enum symbol_offset_e
    {
        KiSystemCall64,
        PsActiveProcessHead,
        PsInitialSystemProcess,
        PsLoadedModuleList,
        SYMBOL_OFFSET_COUNT,
    };

    struct SymbolOffset
    {
        symbol_offset_e e_id;
        const char      module[16];
        const char      name[32];
    };
    const SymbolOffset g_symbol_offsets[] =
    {
        {KiSystemCall64,            "nt", "KiSystemCall64"},
        {PsActiveProcessHead,       "nt", "PsActiveProcessHead"},
        {PsInitialSystemProcess,    "nt", "PsInitialSystemProcess"},
        {PsLoadedModuleList,        "nt", "PsLoadedModuleList"},
    };
    static_assert(COUNT_OF(g_symbol_offsets) == SYMBOL_OFFSET_COUNT, "invalid symbols");

    using MemberOffsets = std::array<uint64_t, MEMBER_OFFSET_COUNT>;
    using SymbolOffsets = std::array<uint64_t, SYMBOL_OFFSET_COUNT>;

    struct OsNt
        : public os::IModule
    {
        OsNt(core::Core& core);

        // methods
        bool setup();

        // os::IModule
        bool                proc_list       (const on_proc_fn& on_process) override;
        opt<proc_t>         proc_current    () override;
        opt<proc_t>         proc_find       (const std::string& name) override;
        opt<std::string>    proc_name       (proc_t proc) override;
        bool                proc_is_valid   (proc_t proc) override;

        bool                thread_list     (proc_t proc, const on_thread_fn& on_thread) override;
        opt<thread_t>       thread_current  () override;
        opt<proc_t>         thread_proc     (thread_t thread) override;
        opt<uint64_t>       thread_pc       (proc_t proc, thread_t thread) override;

        bool                mod_list        (proc_t proc, const on_mod_fn& on_module) override;
        opt<std::string>    mod_name        (proc_t proc, mod_t mod) override;
        opt<span_t>         mod_span        (proc_t proc, mod_t mod) override;

        bool                driver_list     (const on_driver_fn& on_driver) override;
        opt<driver_t>       driver_find     (const std::string& name) override;
        opt<std::string>    driver_name     (driver_t drv) override;
        opt<span_t>         driver_span     (driver_t drv) override;

        void                debug_print     () override;

        // members
        core::Core&     core_;
        MemberOffsets   members_;
        SymbolOffsets   symbols_;
        std::string     last_dump_;
    };
}

OsNt::OsNt(core::Core& core)
    : core_(core)
{
}

namespace
{
    opt<span_t> find_kernel(core::Core& core, uint64_t lstar)
    {
        uint8_t buf[PAGE_SIZE];
        for(auto ptr = utils::align<PAGE_SIZE>(lstar); ptr < lstar; ptr -= PAGE_SIZE)
        {
            auto ok = core.mem.virtual_read(buf, ptr, sizeof buf);
            if(!ok)
                return std::nullopt;

            const auto size = pe::read_image_size(buf, sizeof buf);
            if(!size)
                continue;

            return span_t{ptr, *size};
        }

        return std::nullopt;
    }
}

bool OsNt::setup()
{
    const auto lstar = core_.regs.read(MSR_LSTAR);
    if(!lstar)
        return false;

    const auto kernel = find_kernel(core_, *lstar);
    if(!kernel)
        FAIL(false, "unable to find kernel");

    LOG(INFO, "kernel: 0x%016llx - 0x%016llx (%lld 0x%llx)", kernel->addr, kernel->addr + kernel->size, kernel->size, kernel->size);
    std::vector<uint8_t> buffer(kernel->size);
    auto ok = core_.mem.virtual_read(&buffer[0], kernel->addr, kernel->size);
    if(!ok)
        FAIL(false, "unable to read kernel module");

    ok = core_.sym.insert("nt", *kernel, &buffer[0]);
    if(!ok)
        FAIL(false, "unable to load symbols from kernel module");

    bool fail = false;
    for(size_t i = 0; i < SYMBOL_OFFSET_COUNT; ++i)
    {
        const auto addr = core_.sym.symbol(g_symbol_offsets[i].module, g_symbol_offsets[i].name);
        if(!addr)
        {
            fail = true;
            LOG(ERROR, "unable to read %s!%s symbol offset", g_symbol_offsets[i].module, g_symbol_offsets[i].name);
            continue;
        }

        symbols_[i] = *addr;
    }
    for(size_t i = 0; i < MEMBER_OFFSET_COUNT; ++i)
    {
        const auto offset = core_.sym.struc_offset(g_member_offsets[i].module, g_member_offsets[i].struc, g_member_offsets[i].member);
        if(!offset)
        {
            fail = true;
            LOG(ERROR, "unable to read %s!%s.%s member offset", g_member_offsets[i].module, g_member_offsets[i].struc, g_member_offsets[i].member);
            continue;
        }

        members_[i] = *offset;
    }
    if(fail)
        return false;

    const auto KiSystemCall64 = symbols_[::KiSystemCall64];
    if(*lstar != KiSystemCall64)
        FAIL(false, "PDB mismatch lstar: 0x%llx pdb: 0x%llx\n", *lstar, KiSystemCall64);

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
    for(auto link = core::read_ptr(core_, head); link != head; link = core::read_ptr(core_, *link))
    {
        const auto eproc = *link - members_[EPROCESS_ActiveProcessLinks];
        const auto dtb = core::read_ptr(core_, eproc + members_[EPROCESS_Pcb] + members_[KPROCESS_DirectoryTableBase]);
        if(!dtb)
        {
            LOG(ERROR, "unable to read KPROCESS.DirectoryTableBase from 0x%llx", eproc);
            continue;
        }

        const auto err = on_process({eproc, *dtb});
        if(err == WALK_STOP)
            break;
    }
    return true;
}

namespace
{
    opt<uint64_t> read_gs_base(core::Core& core)
    {
        auto gs = core.regs.read(MSR_GS_BASE);
        if(!gs)
            return std::nullopt;

        if(*gs & 0xFFF0000000000000)
            return gs;

        gs = core.regs.read(MSR_KERNEL_GS_BASE);
        if(!gs)
            return std::nullopt;

        return gs;
    }
}

opt<proc_t> OsNt::proc_current()
{
    const auto current = thread_current();
    if(!current)
        FAIL(std::nullopt, "unable to get current thread");

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

namespace
{
    opt<std::string> read_unicode_string(core::Core& core, uint64_t unicode_string)
    {
        using UnicodeString = struct
        {
            uint16_t length;
            uint16_t max_length;
            uint32_t _; // padding
            uint64_t buffer;
        };
        UnicodeString us;
        auto ok = core.mem.virtual_read(&us, unicode_string, sizeof us);
        if(!ok)
            FAIL(std::nullopt, "unable to read UNICODE_STRING");

        us.length = read_le16(&us.length);
        us.max_length = read_le16(&us.max_length);
        us.buffer = read_le64(&us.buffer);

        if(us.length > us.max_length)
            FAIL(std::nullopt, "corrupted UNICODE_STRING");

        std::vector<uint8_t> buffer(us.length);
        ok = core.mem.virtual_read(&buffer[0], us.buffer, us.length);
        if(!ok)
            FAIL(std::nullopt, "unable to read UNICODE_STRING.buffer");

        const auto p = &buffer[0];
        return utf8::convert(p, &p[us.length]);
    }
}

opt<std::string> OsNt::proc_name(proc_t proc)
{
    // EPROCESS.ImageFileName is 16 bytes, but only 14 are actually used
    char buffer[14+1];
    const auto ok = core_.mem.virtual_read(buffer, proc.id + members_[EPROCESS_ImageFileName], sizeof buffer);
    buffer[sizeof buffer - 1] = 0;
    if(!ok)
        return std::nullopt;

    const auto name = std::string{buffer};
    if(name.size() < sizeof buffer - 1)
        return name;

    const auto image_file_name = core::read_ptr(core_, proc.id + members_[EPROCESS_SeAuditProcessCreationInfo] + members_[SE_AUDIT_PROCESS_CREATION_INFO_ImageFileName]);
    if(!image_file_name)
        return name;

    const auto path = read_unicode_string(core_, *image_file_name + members_[OBJECT_NAME_INFORMATION_Name]);
    if(!path)
        return name;

    return fs::path(*path).filename().generic_string();
}

bool OsNt::mod_list(proc_t proc, const on_mod_fn& on_mod)
{
    const auto peb = core::read_ptr(core_, proc.id + members_[EPROCESS_Peb]);
    if(!peb)
        FAIL(false, "unable to read EPROCESS.Peb");

    // no PEB on system process
    if(!*peb)
        return true;

    const auto ctx = core_.mem.switch_process(proc);
    const auto ldr = core::read_ptr(core_, *peb + members_[PEB_Ldr]);
    if(!ldr)
        FAIL(false, "unable to read PEB.Ldr");

    const auto head = *ldr + members_[PEB_LDR_DATA_InLoadOrderModuleList];
    for(auto link = core::read_ptr(core_, head); link && link != head; link = core::read_ptr(core_, *link))
        if(on_mod({*link - members_[LDR_DATA_TABLE_ENTRY_InLoadOrderLinks]}) == WALK_STOP)
            break;

    return true;
}

opt<std::string> OsNt::mod_name(proc_t proc, mod_t mod)
{
    const auto ctx = core_.mem.switch_process(proc);
    return read_unicode_string(core_, mod.id + members_[LDR_DATA_TABLE_ENTRY_FullDllName]);
}

bool OsNt::proc_is_valid(proc_t proc)
{
    const auto vad_root = core::read_ptr(core_, proc.id + members_[EPROCESS_VadRoot]);
    return vad_root && *vad_root;
}

opt<span_t> OsNt::mod_span(proc_t proc, mod_t mod)
{
    const auto ctx = core_.mem.switch_process(proc);
    const auto base = core::read_ptr(core_, mod.id + members_[LDR_DATA_TABLE_ENTRY_DllBase]);
    if(!base)
        return std::nullopt;

    const auto size = core::read_ptr(core_, mod.id + members_[LDR_DATA_TABLE_ENTRY_SizeOfImage]);
    if(!size)
        return std::nullopt;

    return span_t{*base, *size};
}

bool OsNt::driver_list(const on_driver_fn& on_driver)
{
    const auto head = symbols_[PsLoadedModuleList];
    for(auto link = core::read_ptr(core_, head); link != head; link = core::read_ptr(core_, *link))
        if(on_driver({*link - members_[LDR_DATA_TABLE_ENTRY_InLoadOrderLinks]}) == WALK_STOP)
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
    return read_unicode_string(core_, drv.id + members_[LDR_DATA_TABLE_ENTRY_FullDllName]);
}

opt<span_t> OsNt::driver_span(driver_t drv)
{
    const auto base = core::read_ptr(core_, drv.id + members_[LDR_DATA_TABLE_ENTRY_DllBase]);
    if(!base)
        return std::nullopt;

    const auto size = core::read_ptr(core_, drv.id + members_[LDR_DATA_TABLE_ENTRY_SizeOfImage]);
    if(!size)
        return std::nullopt;

    return span_t{*base, *size};
}

bool OsNt::thread_list(proc_t proc, const on_thread_fn& on_thread)
{
    const auto head = proc.id + members_[EPROCESS_ThreadListHead];
    for(auto link = core::read_ptr(core_, head); link && link != head; link = core::read_ptr(core_, *link))
        if(on_thread({*link - members_[ETHREAD_ThreadListEntry]}) == WALK_STOP)
            break;

    return true;
}

opt<thread_t> OsNt::thread_current()
{
    const auto kpcr = read_gs_base(core_);
    if(!kpcr)
        return std::nullopt;

    const auto current_thread = core::read_ptr(core_, *kpcr + members_[KPCR_Prcb] + members_[KPRCB_CurrentThread]);
    if(!current_thread)
        FAIL(std::nullopt, "unable to read KPCR.Prcb.CurrentThread");

    return thread_t{*current_thread};
}

opt<proc_t> OsNt::thread_proc(thread_t thread)
{
    const auto kproc = core::read_ptr(core_, thread.id + members_[KTHREAD_Process]);
    if(!kproc)
        FAIL(std::nullopt, "unable to read KTHREAD.Process");

    const auto dtb = core::read_ptr(core_, *kproc + members_[KPROCESS_DirectoryTableBase]);
    if(!dtb)
        FAIL(std::nullopt, "unable to read KPROCESS.DirectoryTableBase");

    const auto eproc = *kproc - members_[EPROCESS_Pcb];
    return proc_t{eproc, *dtb};
}

opt<uint64_t> OsNt::thread_pc(proc_t proc, thread_t thread)
{
    UNUSED(proc);

    const auto ktrap_frame = core::read_ptr(core_, thread.id + members_[ETHREAD_Tcb] + members_[KTHREAD_TrapFrame]);
    if(!ktrap_frame)
        FAIL(std::nullopt, "unable to read KTHREAD.TrapFrame");

    if(!*ktrap_frame)
        return std::nullopt;

    const auto rip = core::read_ptr(core_, *ktrap_frame + members_[KTRAP_FRAME_Rip]);
    return rip; // rip can be null
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
        return hex::convert<hex::LowerCase>(buf, x);
    }
}

void OsNt::debug_print()
{
#ifdef USE_DEBUG_PRINT
    const auto kpcr = read_gs_base(core_);
    if(!kpcr)
        return;

    const auto irql = core::read_byte(core_, *kpcr + members_[KPCR_Irql]);
    const auto cs = core_.regs.read(FDP_CS_REGISTER);
    const auto rip = core_.regs.read(FDP_RIP_REGISTER);
    const auto ripcur = core_.sym.find(*rip);
    const auto ripsym = ripcur ? sym::to_string(*ripcur) : "";
    const auto thread = thread_current();
    const auto proc = thread_proc(*thread);
    const auto name = proc_name(*proc);
    const auto dump = "rip: " + to_hex(rip ? *rip : 0)
                    + ' ' + irql_to_text(irql ? *irql : -1)
                    + ' ' + (cs ? is_user_mode(*cs) ? "user" : "kernel" : "?")
                    + (name ? " " + *name : "")
                    + (ripsym.empty() ? "" : " " + ripsym)
                    + " p:" + to_hex(proc ? proc->id : 0)
                    + " t:" + to_hex(thread ? thread->id : 0);
    if(dump != last_dump_)
        LOG(INFO, "%s", dump.data());
    last_dump_ = dump;
#endif
}
