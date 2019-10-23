#pragma once

#include "enums.hpp"
#include "types.hpp"

#include <functional>

namespace core { struct Core; }

namespace vm_area
{
    using on_vm_area_fn = std::function<walk_e(vm_area_t)>;

    bool                list    (core::Core&, proc_t proc, on_vm_area_fn on_vm_area);
    opt<vm_area_t>      find    (core::Core&, proc_t proc, uint64_t addr);
    opt<span_t>         span    (core::Core&, proc_t proc, vm_area_t vm_area);
    vma_access_e        access  (core::Core&, proc_t proc, vm_area_t vm_area);
    vma_type_e          type    (core::Core&, proc_t proc, vm_area_t vm_area);
    opt<std::string>    name    (core::Core&, proc_t proc, vm_area_t vm_area);
} // namespace vm_area
