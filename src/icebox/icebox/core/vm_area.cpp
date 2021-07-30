#include "vm_area.hpp"

#define PRIVATE_CORE_
#define FDP_MODULE "vm_area"
#include "core.hpp"
#include "core_private.hpp"
#include "interfaces/if_os.hpp"

bool vm_area::list(core::Core& core, proc_t proc, on_vm_area_fn on_vm_area)
{
    return core.os_->vm_area_list(proc, std::move(on_vm_area));
}

opt<vm_area_t> vm_area::find(core::Core& core, proc_t proc, uint64_t addr)
{
    return core.os_->vm_area_find(proc, addr);
}

opt<span_t> vm_area::span(core::Core& core, proc_t proc, vm_area_t vm_area)
{
    return core.os_->vm_area_span(proc, vm_area);
}

vma_access_e vm_area::access(core::Core& core, proc_t proc, vm_area_t vm_area)
{
    return core.os_->vm_area_access(proc, vm_area);
}

vma_type_e vm_area::type(core::Core& core, proc_t proc, vm_area_t vm_area)
{
    return core.os_->vm_area_type(proc, vm_area);
}

opt<std::string> vm_area::name(core::Core& core, proc_t proc, vm_area_t vm_area)
{
    return core.os_->vm_area_name(proc, vm_area);
}
