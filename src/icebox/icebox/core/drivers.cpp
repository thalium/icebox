#include "drivers.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "drivers"
#include "core.hpp"
#include "core_private.hpp"
#include "os_private.hpp"

bool drivers::list(core::Core& core, on_driver_fn on_driver)
{
    return core.os_->driver_list(on_driver);
}

opt<driver_t> drivers::find(core::Core& core, uint64_t addr)
{
    return core.os_->driver_find(addr);
}

opt<std::string> drivers::name(core::Core& core, driver_t drv)
{
    return core.os_->driver_name(drv);
}

opt<span_t> drivers::span(core::Core& core, driver_t drv)
{
    return core.os_->driver_span(drv);
}

opt<os::bpid_t> drivers::listen_create(core::Core& core, const on_event_fn& on_load)
{
    return core.os_->listen_drv_create(on_load);
}
