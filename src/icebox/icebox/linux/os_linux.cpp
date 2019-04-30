#include "os.hpp"

#define FDP_MODULE "os_linux"
#include "core.hpp"
#include "log.hpp"
#include "reader.hpp"
#include "utils/fnview.hpp"
#include "utils/utils.hpp"

#include "map.hpp"

#include <array>

namespace
{
    enum class cat_e
    {
        REQUIRED,
        OPTIONAL,
    };

    enum offset_e
    {
        TASKSTRUCT_COMM,
        TASKSTRUCT_PID,
        TASKSTRUCT_GROUPLEADER,
        TASKSTRUCT_TASKS,
        TASKSTRUCT_MM,
        TASKSTRUCT_ACTIVEMM,
        MMSTRUCT_PGD,
        CRED_UID,
        OFFSET_COUNT,
    };

    struct LinuxOffset
    {
        cat_e      e_cat;
        offset_e   e_id;
        const char module[16];
        const char struc[32];
        const char member[32];
    };
    // clang-format off
    const LinuxOffset g_offsets[] =
    {
            {cat_e::REQUIRED,	TASKSTRUCT_COMM,			"dwarf",	"task_struct",		"comm"},
            {cat_e::REQUIRED,	TASKSTRUCT_PID,				"dwarf",	"task_struct",		"pid"},
            {cat_e::REQUIRED,	TASKSTRUCT_GROUPLEADER,		"dwarf",	"task_struct",		"group_leader"},
            {cat_e::REQUIRED,	TASKSTRUCT_TASKS,			"dwarf",	"task_struct",		"tasks"},
            {cat_e::REQUIRED,	TASKSTRUCT_MM,				"dwarf",	"task_struct",		"mm"},
            {cat_e::REQUIRED,	TASKSTRUCT_ACTIVEMM,		"dwarf",	"task_struct",		"active_mm"},
            {cat_e::REQUIRED,	MMSTRUCT_PGD,				"dwarf",	"mm_struct",		"pgd"},
            {cat_e::REQUIRED,	CRED_UID,					"dwarf",	"cred",				"uid"},
    };
    // clang-format on
    static_assert(COUNT_OF(g_offsets) == OFFSET_COUNT, "invalid offsets");

    enum symbol_e
    {
        PER_CPU_START,
        CURRENT_TASK,
        // STARTUP_64,
        // INIT_TASK,
        SYMBOL_COUNT,
    };

    struct LinuxSymbol
    {
        cat_e      e_cat;
        symbol_e   e_id;
        const char module[16];
        const char name[32];
    };
    // clang-format off
    const LinuxSymbol g_symbols[] =
    {
            {cat_e::REQUIRED,	PER_CPU_START,				"system_map",	"__per_cpu_start"},
			{cat_e::REQUIRED,	CURRENT_TASK,				"system_map",	"current_task"},
			// {cat_e::REQUIRED,	STARTUP_64,					"system_map",	"startup_64"},
			// {cat_e::REQUIRED,	INIT_TASK,					"system_map",	"init_task"},
    };
    // clang-format on
    static_assert(COUNT_OF(g_symbols) == SYMBOL_COUNT, "invalid symbols");

    using LinuxOffsets = std::array<uint64_t, OFFSET_COUNT>;
    using LinuxSymbols = std::array<uint64_t, SYMBOL_COUNT>;

    struct KernelRandomization
    {
        uint64_t per_cpu;
        uint64_t kaslr;
        uint64_t kernel;
        uint64_t kpgd;
    };

    struct OsLinux
        : public os::IModule
    {
        OsLinux(core::Core& core);

        // methods
        bool            find_kernel ();
        opt<thread_t>   thread_find (uint32_t pid);

        // os::IModule
        bool            setup               () override;
        bool            is_kernel_address   (uint64_t ptr) override;
        bool            can_inject_fault    (uint64_t ptr) override;
        bool            reader_setup        (reader::Reader& reader, opt<proc_t> proc) override;
        sym::Symbols&   kernel_symbols      () override;

        bool                proc_list       (on_proc_fn on_process) override;
        opt<proc_t>         proc_current    () override;
        opt<proc_t>         proc_find       (std::string_view name, flags_e flags) override;
        opt<proc_t>         proc_find       (uint64_t pid) override;
        opt<std::string>    proc_name       (proc_t proc) override;
        bool                proc_is_valid   (proc_t proc) override;
        uint64_t            proc_id         (proc_t proc) override;
        flags_e             proc_flags      (proc_t proc) override;
        void                proc_join       (proc_t proc, os::join_e join) override;
        opt<phy_t>          proc_resolve    (proc_t proc, uint64_t ptr) override;
        opt<proc_t>         proc_select     (proc_t proc, uint64_t ptr) override;
        opt<proc_t>         proc_parent     (proc_t proc) override;

        bool            thread_list     (proc_t proc, on_thread_fn on_thread) override;
        opt<thread_t>   thread_current  () override;
        opt<proc_t>     thread_proc     (thread_t thread) override;
        opt<uint64_t>   thread_pc       (proc_t proc, thread_t thread) override;
        uint64_t        thread_id       (proc_t proc, thread_t thread) override;

        bool                mod_list(proc_t proc, on_mod_fn on_module) override;
        opt<std::string>    mod_name(proc_t proc, mod_t mod) override;
        opt<span_t>         mod_span(proc_t proc, mod_t mod) override;
        opt<mod_t>          mod_find(proc_t proc, uint64_t addr) override;

        bool                vm_area_list    (proc_t proc, on_vm_area_fn on_vm_area) override;
        opt<vm_area_t>      vm_area_find    (proc_t proc, uint64_t addr) override;
        opt<span_t>         vm_area_span    (proc_t proc, vm_area_t vm_area) override;
        vma_access_e        vm_area_access  (proc_t proc, vm_area_t vm_area) override;
        vma_type_e          vm_area_type    (proc_t proc, vm_area_t vm_area) override;
        opt<std::string>    vm_area_name    (proc_t proc, vm_area_t vm_area) override;

        bool                driver_list (on_driver_fn on_driver) override;
        opt<driver_t>       driver_find (uint64_t addr) override;
        opt<std::string>    driver_name (driver_t drv) override;
        opt<span_t>         driver_span (driver_t drv) override;

        opt<bpid_t> listen_proc_create  (const on_proc_event_fn& on_create) override;
        opt<bpid_t> listen_proc_delete  (const on_proc_event_fn& on_delete) override;
        opt<bpid_t> listen_thread_create(const on_thread_event_fn& on_create) override;
        opt<bpid_t> listen_thread_delete(const on_thread_event_fn& on_delete) override;
        opt<bpid_t> listen_mod_create   (const on_mod_event_fn& on_create) override;
        opt<bpid_t> listen_drv_create   (const on_drv_event_fn& on_drv) override;
        size_t      unlisten            (bpid_t bpid) override;

        opt<arg_t>  read_stack  (size_t index) override;
        opt<arg_t>  read_arg    (size_t index) override;
        bool        write_arg   (size_t index, arg_t arg) override;

        void debug_print() override;

        // members
        core::Core&         core_;
        sym::Symbols        dwarf_;
        sym::Map            sysmap_;
        reader::Reader      reader_;
        LinuxOffsets        offsets_;
        LinuxSymbols        symbols_;
        KernelRandomization kernel_rand_;
    };
}

OsLinux::OsLinux(core::Core& core)
    : core_(core)
    , reader_(reader::make(core))
    , sysmap_()
{
}

bool OsLinux::setup()
{
    if(!dwarf_.insert("dwarf", {}, {}, {}))
        return FAIL(false, "unable to read dwarf file");

    if(!sysmap_.setup())
        return FAIL(false, "unable to read System.map file");

    bool success = true;
    int i        = -1;
    memset(&symbols_[0], 0, sizeof symbols_);
    for(const auto& sym : g_symbols)
    {
        success &= sym.e_id == ++i;
        const auto addr = sysmap_.symbol(sym.name);
        if(!addr)
        {
            success &= sym.e_cat != cat_e::REQUIRED;
            if(sym.e_cat == cat_e::REQUIRED)
                LOG(ERROR, "unable to read {}!{} symbol offset", sym.module, sym.name);
            else
                LOG(WARNING, "unable to read optional {}!{} symbol offset", sym.module, sym.name);
            continue;
        }
        symbols_[i] = *addr;
    }

    i = -1;
    memset(&offsets_[0], 0, sizeof offsets_);
    for(const auto& off : g_offsets)
    {
        success &= off.e_id == ++i;
        const auto offset = dwarf_.struc_offset(off.module, off.struc, off.member);
        if(!offset)
        {
            success &= off.e_cat != cat_e::REQUIRED;
            if(off.e_cat == cat_e::REQUIRED)
                LOG(ERROR, "unable to read {}!{}.{} member offset", off.module, off.struc, off.member);
            else
                LOG(WARNING, "unable to read optional {}!{}.{} member offset", off.module, off.struc, off.member);
            continue;
        }
        offsets_[i] = *offset;
    }

    if(!find_kernel())
        return FAIL(false, "unable to find kernel addresses");

    return success;
}

bool OsLinux::find_kernel()
{
    // kpgd
    kernel_rand_.kpgd = core_.regs.read(FDP_CR3_REGISTER);
    kernel_rand_.kpgd &= ~0x1fffull;
    reader_setup(reader_, proc_t{NULL, dtb_t{NULL}});

    // per_cpu address
    auto per_cpu = core_.regs.read(MSR_GS_BASE);
    if(!is_kernel_address(per_cpu))
        per_cpu = core_.regs.read(MSR_KERNEL_GS_BASE);
    if(!is_kernel_address(per_cpu))
        return FAIL(false, "unable to find per_cpu address");
    kernel_rand_.per_cpu = per_cpu;

    // test LSTAR -> sys_call_table -> kaslr
    const auto lstar = core_.regs.read(MSR_LSTAR);

    return true;
}

std::unique_ptr<os::IModule> os::make_linux(core::Core& core)
{
    return std::make_unique<OsLinux>(core);
}

bool OsLinux::proc_list(on_proc_fn on_process)
{
    const auto current = proc_current();
    if(!current)
        return false;

    const auto head    = (*current).id + offsets_[TASKSTRUCT_TASKS];
    opt<uint64_t> link = head;
    do
    {
        const auto thread = thread_t{*link - offsets_[TASKSTRUCT_TASKS]};
        const auto proc   = thread_proc(thread);
        if(proc)
            if(on_process(*proc) == WALK_STOP)
                return true;

        link = reader_.read(*link);
        if(!link)
            return FAIL(false, "unable to read next process address");
    } while(link != head);

    return true;
}

opt<proc_t> OsLinux::proc_current()
{
    const auto thread = thread_current();
    if(!thread)
        return {};

    return thread_proc(*thread);
}

opt<proc_t> OsLinux::proc_find(std::string_view name, flags_e /*flags*/)
{
    opt<proc_t> found;
    proc_list([&](proc_t proc)
    {
        const auto got = proc_name(proc);
        if(*got != name)
            return WALK_NEXT;

        found = proc;
        return WALK_STOP;
    });
    return found;
}

opt<proc_t> OsLinux::proc_find(uint64_t pid)
{
    opt<proc_t> ret;
    proc_list([&](proc_t proc)
    {
        const auto got = proc_id(proc);
        if(got > 4194304)
            return FAIL(WALK_NEXT, "unable to find the pid of proc {:#x}", proc.id);

        if(got != pid)
            return WALK_NEXT;

        ret = proc;
        return WALK_STOP;
    });
    return ret;
}

opt<std::string> OsLinux::proc_name(proc_t proc)
{
    char buffer[14 + 1];
    const auto ok             = reader_.read(buffer, proc.id + offsets_[TASKSTRUCT_COMM], sizeof buffer);
    buffer[sizeof buffer - 1] = 0;
    if(!ok)
        return {};

    return std::string{buffer};
}

uint64_t OsLinux::proc_id(proc_t proc)
{
    return thread_id({}, thread_t{proc.id});
}

bool OsLinux::proc_is_valid(proc_t /*proc*/)
{
    return true;
}

bool OsLinux::is_kernel_address(uint64_t ptr)
{
    return (ptr > 0x7fffffffffffffff);
}

bool OsLinux::can_inject_fault(uint64_t /*ptr*/)
{
    return false;
}

flags_e OsLinux::proc_flags(proc_t /*proc*/)
{
    return FLAGS_NONE;
}

void OsLinux::proc_join(proc_t /*proc*/, os::join_e /*join*/)
{
}

opt<phy_t> OsLinux::proc_resolve(proc_t /*proc*/, uint64_t /*ptr*/)
{
    return {};
}

opt<proc_t> OsLinux::proc_select(proc_t proc, uint64_t /*ptr*/)
{
    return proc;
}

opt<proc_t> OsLinux::proc_parent(proc_t /*proc*/)
{
    return {};
}

bool OsLinux::reader_setup(reader::Reader& reader, opt<proc_t> proc)
{
    reader.udtb_ = (proc) ? proc->dtb : dtb_t{0};
    reader.kdtb_ = dtb_t{kernel_rand_.kpgd};
    return true;
}

sym::Symbols& OsLinux::kernel_symbols() // structure either
{
    return dwarf_;
}

bool OsLinux::thread_list(proc_t /*proc*/, on_thread_fn on_thread) // todo
{
    LOG(ERROR, "thread_list unimplemented");

    thread_t dummy_thread = {0};
    on_thread(dummy_thread);
    return true;
}

opt<thread_t> OsLinux::thread_current()
{
    const auto addr = reader_.read(kernel_rand_.per_cpu + symbols_[CURRENT_TASK] - symbols_[PER_CPU_START]);
    if(!addr)
        return FAIL(ext::nullopt, "unable to read current_task in per_cpu area");

    return thread_t{*addr};
}

opt<thread_t> OsLinux::thread_find(uint32_t pid)
{
    opt<thread_t> ret = {};

    thread_list({}, [&](thread_t thread)
    {
        const auto got = thread_id({}, thread);
        if(got > 4194304)
            return FAIL(WALK_NEXT, "unable to find the pid of thread {:#x}", thread.id);

        if(got != pid)
            return WALK_NEXT;

        ret = thread;
        return WALK_STOP;
    });

    return ret;
}

opt<proc_t> OsLinux::thread_proc(thread_t thread)
{
    const auto proc_id = reader_.read(thread.id + offsets_[TASKSTRUCT_GROUPLEADER]);
    if(!proc_id)
        return FAIL(ext::nullopt, "unable to find the leader of thread {:#x}", thread.id);

    auto pgd = reader_.read(*proc_id + offsets_[TASKSTRUCT_MM] + offsets_[MMSTRUCT_PGD]);
    if(!pgd || is_kernel_address(*pgd))
        pgd = reader_.read(*proc_id + offsets_[TASKSTRUCT_ACTIVEMM] + offsets_[MMSTRUCT_PGD]); // for kernel threads
    if(!pgd || is_kernel_address(*pgd))
        return FAIL(ext::nullopt, "unable to read pgd in task_struct.mm or task_struct.active_mm for process {:#x}", *proc_id);

    return proc_t{*proc_id, dtb_t{*pgd}};
}

opt<uint64_t> OsLinux::thread_pc(proc_t /*proc*/, thread_t /*thread*/)
{
    return {};
}

uint64_t OsLinux::thread_id(proc_t /*proc*/, thread_t thread) // return opt ?, remove proc ?
{
    const auto pid = reader_.le32(thread.id + offsets_[TASKSTRUCT_PID]);
    if(!pid)
        return 0xffffffffffffffff; // opt<uint64_t> either ? (0 is a valid pid for an iddle task in linux)

    return *pid;
}

bool OsLinux::mod_list(proc_t /*proc*/, on_mod_fn on_module)
{
    mod_t dummy_mod = {0, FLAGS_NONE};
    on_module(dummy_mod);
    return true;
}

opt<std::string> OsLinux::mod_name(proc_t /*proc*/, mod_t /*mod*/)
{
    return {};
}

opt<span_t> OsLinux::mod_span(proc_t /*proc*/, mod_t /*mod*/)
{
    return {};
}

opt<mod_t> OsLinux::mod_find(proc_t /*proc*/, uint64_t /*addr*/)
{
    return {};
}

bool OsLinux::vm_area_list(proc_t /*proc*/, on_vm_area_fn /*on_vm_area*/)
{
    return false;
}

opt<vm_area_t> OsLinux::vm_area_find(proc_t /*proc*/, uint64_t /*addr*/)
{
    return {};
}

opt<span_t> OsLinux::vm_area_span(proc_t /*proc*/, vm_area_t /*vm_area*/)
{
    return {};
}

vma_access_e OsLinux::vm_area_access(proc_t /*proc*/, vm_area_t /*vm_area*/)
{
    return VMA_ACCESS_NONE;
}

vma_type_e OsLinux::vm_area_type(proc_t /*proc*/, vm_area_t /*vm_area*/)
{
    return vma_type_e::none;
}

opt<std::string> OsLinux::vm_area_name(proc_t /*proc*/, vm_area_t /*vm_area*/)
{
    return {};
}

bool OsLinux::driver_list(on_driver_fn on_driver)
{
    driver_t dummy_driver = {0};
    on_driver(dummy_driver);
    return true;
}

opt<driver_t> OsLinux::driver_find(uint64_t /*addr*/)
{
    return {};
}

opt<std::string> OsLinux::driver_name(driver_t /*drv*/)
{
    return {};
}

opt<span_t> OsLinux::driver_span(driver_t /*drv*/)
{
    return {};
}

opt<arg_t> OsLinux::read_stack(size_t /*index*/)
{
    return {};
}

opt<arg_t> OsLinux::read_arg(size_t /*index*/)
{
    return {};
}

bool OsLinux::write_arg(size_t /*index*/, arg_t /*arg*/)
{
    return false;
}

void OsLinux::debug_print()
{
}

opt<OsLinux::bpid_t> OsLinux::listen_proc_create(const on_proc_event_fn& /*on_create*/)
{
    return {};
}

opt<OsLinux::bpid_t> OsLinux::listen_proc_delete(const on_proc_event_fn& /*on_delete*/)
{
    return {};
}

opt<OsLinux::bpid_t> OsLinux::listen_thread_create(const on_thread_event_fn& /*on_create*/)
{
    return {};
}

opt<OsLinux::bpid_t> OsLinux::listen_thread_delete(const on_thread_event_fn& /*on_remove*/)
{
    return {};
}

opt<OsLinux::bpid_t> OsLinux::listen_mod_create(const on_mod_event_fn& /*on_create*/)
{
    return {};
}

opt<OsLinux::bpid_t> OsLinux::listen_drv_create(const on_drv_event_fn&)
{
    return {};
}

size_t OsLinux::unlisten(bpid_t /*bpid*/)
{
    return {};
}
