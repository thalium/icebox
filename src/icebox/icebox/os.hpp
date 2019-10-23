#pragma once

#include "enums.hpp"
#include "types.hpp"

#include <functional>

namespace core { struct Core; }
namespace sym { struct Symbols; }
namespace reader { struct Reader; }

namespace os
{
    using on_vm_area_fn = fn::view<walk_e(vm_area_t)>;

    using bpid_t             = uint64_t;
    using on_proc_event_fn   = std::function<void(proc_t)>;
    using on_thread_event_fn = std::function<void(thread_t)>;
    using on_mod_event_fn    = std::function<void(proc_t, mod_t)>;
    using on_drv_event_fn    = std::function<void(driver_t, bool load)>;

    bool            is_kernel_address   (core::Core&, uint64_t ptr);
    bool            can_inject_fault    (core::Core&, uint64_t ptr);
    bool            reader_setup        (core::Core&, reader::Reader& reader, opt<proc_t> proc);
    sym::Symbols&   kernel_symbols      (core::Core&);

    bool                vm_area_list    (core::Core&, proc_t proc, on_vm_area_fn on_vm_area);
    opt<vm_area_t>      vm_area_find    (core::Core&, proc_t proc, uint64_t addr);
    opt<span_t>         vm_area_span    (core::Core&, proc_t proc, vm_area_t vm_area);
    vma_access_e        vm_area_access  (core::Core&, proc_t proc, vm_area_t vm_area);
    vma_type_e          vm_area_type    (core::Core&, proc_t proc, vm_area_t vm_area);
    opt<std::string>    vm_area_name    (core::Core&, proc_t proc, vm_area_t vm_area);

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
