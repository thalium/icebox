#include "os.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "core"
#include "core.hpp"
#include "core_private.hpp"
#include "interfaces/if_os.hpp"

bool os::is_kernel_address(core::Core& core, uint64_t ptr)
{
    return core.os_->is_kernel_address(ptr);
}

bool os::can_inject_fault(core::Core& core, uint64_t ptr)
{
    if(!core.os_)
        return false;

    return core.os_->can_inject_fault(ptr);
}

bool os::reader_setup(core::Core& core, reader::Reader& reader, opt<proc_t> proc)
{
    if(!core.os_)
        return true;

    return core.os_->reader_setup(reader, std::move(proc));
}

size_t os::unlisten(core::Core& core, bpid_t bpid)
{
    return core.os_->unlisten(bpid);
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
