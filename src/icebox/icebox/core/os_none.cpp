#include "os.hpp"

#define FDP_MODULE "none"
#include "core.hpp"
#include "interfaces/if_os.hpp"
#include "log.hpp"

namespace
{
    struct None
        : public os::Module
    {
        // os::IModule
        bool        setup               () override;
        bool        is_kernel_address   (uint64_t ptr) override;
        bool        read_page           (void* dst, uint64_t ptr, proc_t* proc, dtb_t dtb) override;
        bool        write_page          (uint64_t ptr, const void* src, proc_t* proc, dtb_t dtb) override;
        opt<phy_t>  virtual_to_physical (proc_t* proc, dtb_t dtb, uint64_t ptr) override;
        dtb_t       kernel_dtb          () override;

        bool                proc_list       (process::on_proc_fn on_process) override;
        opt<proc_t>         proc_current    () override;
        opt<proc_t>         proc_find       (std::string_view name, flags_t flags) override;
        opt<proc_t>         proc_find       (uint64_t pid) override;
        opt<std::string>    proc_name       (proc_t proc) override;
        bool                proc_is_valid   (proc_t proc) override;
        uint64_t            proc_id         (proc_t proc) override;
        flags_t             proc_flags      (proc_t proc) override;
        void                proc_join       (proc_t proc, mode_e mode) override;
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
        opt<std::string>    driver_name (driver_t drv) override;
        opt<span_t>         driver_span (driver_t drv) override;

        opt<bpid_t> listen_proc_create  (const process::on_event_fn& on_create) override;
        opt<bpid_t> listen_proc_delete  (const process::on_event_fn& on_delete) override;
        opt<bpid_t> listen_thread_create(const threads::on_event_fn& on_create) override;
        opt<bpid_t> listen_thread_delete(const threads::on_event_fn& on_delete) override;
        opt<bpid_t> listen_mod_create   (proc_t proc, flags_t flags, const modules::on_event_fn& on_create) override;
        opt<bpid_t> listen_drv_create   (const drivers::on_event_fn& on_drv) override;

        opt<arg_t>  read_stack  (size_t index) override;
        opt<arg_t>  read_arg    (size_t index) override;
        bool        write_arg   (size_t index, arg_t arg) override;

        void debug_print() override;
    };
}

bool None::setup()
{
    return true;
}

std::unique_ptr<os::Module> os::make_none()
{
    return std::make_unique<None>();
}

bool None::proc_list(process::on_proc_fn /*on_process*/)
{
    return true;
}

opt<proc_t> None::proc_current()
{
    return {};
}

opt<proc_t> None::proc_find(std::string_view /*name*/, flags_t /*flags*/)
{
    return {};
}

opt<proc_t> None::proc_find(uint64_t /*pid*/)
{
    return {};
}

opt<std::string> None::proc_name(proc_t /*proc*/)
{
    return {};
}

uint64_t None::proc_id(proc_t /*proc*/)
{
    return {};
}

bool None::proc_is_valid(proc_t /*proc*/)
{
    return false;
}

bool None::is_kernel_address(uint64_t /*ptr*/)
{
    return true;
}

opt<phy_t> None::virtual_to_physical(proc_t* /*proc*/, dtb_t /*dtb*/, uint64_t /*ptr*/)
{
    return {};
}

bool None::read_page(void* /*dst*/, uint64_t /*ptr*/, proc_t* /*proc*/, dtb_t /*dtb*/)
{
    return false;
}

bool None::write_page(uint64_t /*ptr*/, const void* /*src*/, proc_t* /*proc*/, dtb_t /*dtb*/)
{
    return false;
}

flags_t None::proc_flags(proc_t /*proc*/)
{
    return {};
}

void None::proc_join(proc_t /*proc*/, mode_e /*mode*/)
{
}

opt<proc_t> None::proc_parent(proc_t /*proc*/)
{
    return {};
}

dtb_t None::kernel_dtb()
{
    return {};
}

bool None::thread_list(proc_t /*proc*/, threads::on_thread_fn /*on_thread*/)
{
    return true;
}

opt<thread_t> None::thread_current()
{
    return {};
}

opt<proc_t> None::thread_proc(thread_t /*thread*/)
{
    return {};
}

opt<uint64_t> None::thread_pc(proc_t /*proc*/, thread_t /*thread*/)
{
    return {};
}

uint64_t None::thread_id(proc_t /*proc*/, thread_t /*thread*/)
{
    return {};
}

bool None::mod_list(proc_t /*proc*/, modules::on_mod_fn /*on_module*/)
{
    return true;
}

opt<std::string> None::mod_name(proc_t /*proc*/, mod_t /*mod*/)
{
    return {};
}

opt<span_t> None::mod_span(proc_t /*proc*/, mod_t /*mod*/)
{
    return {};
}

opt<mod_t> None::mod_find(proc_t /*proc*/, uint64_t /*addr*/)
{
    return {};
}

bool None::vm_area_list(proc_t /*proc*/, vm_area::on_vm_area_fn /*on_vm_area*/)
{
    return true;
}

opt<vm_area_t> None::vm_area_find(proc_t /*proc*/, uint64_t /*addr*/)
{
    return {};
}

opt<span_t> None::vm_area_span(proc_t /*proc*/, vm_area_t /*vm_area*/)
{
    return {};
}

vma_access_e None::vm_area_access(proc_t /*proc*/, vm_area_t /*vm_area*/)
{
    return {};
}

vma_type_e None::vm_area_type(proc_t /*proc*/, vm_area_t /*vm_area*/)
{
    return vma_type_e::other;
}

opt<std::string> None::vm_area_name(proc_t /*proc*/, vm_area_t /*vm_area*/)
{
    return {};
}

bool None::driver_list(drivers::on_driver_fn /*on_driver*/)
{
    return true;
}

opt<std::string> None::driver_name(driver_t /*drv*/)
{
    return {};
}

opt<span_t> None::driver_span(driver_t /*drv*/)
{
    return {};
}

opt<arg_t> None::read_stack(size_t /*index*/)
{
    return {};
}

opt<arg_t> None::read_arg(size_t /*index*/)
{
    return {};
}

bool None::write_arg(size_t /*index*/, arg_t /*arg*/)
{
    return false;
}

void None::debug_print()
{
}

opt<bpid_t> None::listen_proc_create(const process::on_event_fn& /*on_create*/)
{
    return {};
}

opt<bpid_t> None::listen_proc_delete(const process::on_event_fn& /*on_delete*/)
{
    return {};
}

opt<bpid_t> None::listen_thread_create(const threads::on_event_fn& /*on_create*/)
{
    return {};
}

opt<bpid_t> None::listen_thread_delete(const threads::on_event_fn& /*on_remove*/)
{
    return {};
}

opt<bpid_t> None::listen_mod_create(proc_t /*proc*/, flags_t /*flags*/, const modules::on_event_fn& /*on_create*/)
{
    return {};
}

opt<bpid_t> None::listen_drv_create(const drivers::on_event_fn& /*on_load*/)
{
    return {};
}
