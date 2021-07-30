#include "os.hpp"

#define PRIVATE_CORE_
#define FDP_MODULE "core"
#include "core.hpp"
#include "core_private.hpp"
#include "interfaces/if_os.hpp"

bool os::is_kernel_address(core::Core& core, uint64_t ptr)
{
    return core.os_->is_kernel_address(ptr);
}

bool os::read_page(core::Core& core, void* dst, uint64_t ptr, proc_t* proc, dtb_t dtb)
{
    if(!core.os_)
        return false;

    return core.os_->read_page(dst, ptr, proc, dtb);
}

bool os::write_page(core::Core& core, uint64_t dst, const void* src, proc_t* proc, dtb_t dtb)
{
    if(!core.os_)
        return false;

    return core.os_->write_page(dst, src, proc, dtb);
}

opt<phy_t> os::virtual_to_physical(core::Core& core, proc_t* proc, dtb_t dtb, uint64_t ptr)
{
    if(!core.os_)
        return {};

    return core.os_->virtual_to_physical(proc, dtb, ptr);
}

void os::debug_print(core::Core& core)
{
    return core.os_->debug_print();
}

bool os::check_flags(flags_t got, flags_t want)
{
    if(want.is_x86 && !got.is_x86)
        return false;

    if(want.is_x64 && !got.is_x64)
        return false;

    return true;
}
