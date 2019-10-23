#pragma once

#include "enums.hpp"
#include "types.hpp"

#include <functional>
#include <memory>

namespace core { struct Core; }
namespace sym { struct Symbols; }
namespace reader { struct Reader; }

namespace os
{
    using on_thread_fn  = fn::view<walk_e(thread_t)>;
    using on_mod_fn     = fn::view<walk_e(mod_t)>;
    using on_vm_area_fn = fn::view<walk_e(vm_area_t)>;
    using on_driver_fn  = fn::view<walk_e(driver_t)>;

    using bpid_t             = uint64_t;
    using on_proc_event_fn   = std::function<void(proc_t)>;
    using on_thread_event_fn = std::function<void(thread_t)>;
    using on_mod_event_fn    = std::function<void(proc_t, mod_t)>;
    using on_drv_event_fn    = std::function<void(driver_t, bool load)>;

    bool            is_kernel_address   (core::Core&, uint64_t ptr);
    bool            can_inject_fault    (core::Core&, uint64_t ptr);
    bool            reader_setup        (core::Core&, reader::Reader& reader, opt<proc_t> proc);
    sym::Symbols&   kernel_symbols      (core::Core&);

    bool            thread_list     (core::Core&, proc_t proc, on_thread_fn on_thread);
    opt<thread_t>   thread_current  (core::Core&);
    opt<proc_t>     thread_proc     (core::Core&, thread_t thread);
    opt<uint64_t>   thread_pc       (core::Core&, proc_t proc, thread_t thread);
    uint64_t        thread_id       (core::Core&, proc_t proc, thread_t thread);

    bool                mod_list(core::Core&, proc_t proc, on_mod_fn on_mod);
    opt<std::string>    mod_name(core::Core&, proc_t proc, mod_t mod);
    opt<span_t>         mod_span(core::Core&, proc_t proc, mod_t mod);
    opt<mod_t>          mod_find(core::Core&, proc_t proc, uint64_t addr);

    bool                vm_area_list    (core::Core&, proc_t proc, on_vm_area_fn on_vm_area);
    opt<vm_area_t>      vm_area_find    (core::Core&, proc_t proc, uint64_t addr);
    opt<span_t>         vm_area_span    (core::Core&, proc_t proc, vm_area_t vm_area);
    vma_access_e        vm_area_access  (core::Core&, proc_t proc, vm_area_t vm_area);
    vma_type_e          vm_area_type    (core::Core&, proc_t proc, vm_area_t vm_area);
    opt<std::string>    vm_area_name    (core::Core&, proc_t proc, vm_area_t vm_area);

    bool                driver_list (core::Core&, on_driver_fn on_driver);
    opt<driver_t>       driver_find (core::Core&, uint64_t addr);
    opt<std::string>    driver_name (core::Core&, driver_t drv);
    opt<span_t>         driver_span (core::Core&, driver_t drv);

    opt<bpid_t> listen_proc_create  (core::Core&, const on_proc_event_fn& on_proc_event);
    opt<bpid_t> listen_proc_delete  (core::Core&, const on_proc_event_fn& on_proc_event);
    opt<bpid_t> listen_thread_create(core::Core&, const on_thread_event_fn& on_thread_event);
    opt<bpid_t> listen_thread_delete(core::Core&, const on_thread_event_fn& on_thread_event);
    opt<bpid_t> listen_mod_create   (core::Core&, const on_mod_event_fn& on_load);
    opt<bpid_t> listen_drv_create   (core::Core&, const on_drv_event_fn& on_load);
    size_t      unlisten            (core::Core&, bpid_t bpid);

    opt<arg_t>  read_stack  (core::Core&, size_t index);
    opt<arg_t>  read_arg    (core::Core&, size_t index);
    bool        write_arg   (core::Core&, size_t index, arg_t arg);

    void debug_print(core::Core&);
} // namespace os
