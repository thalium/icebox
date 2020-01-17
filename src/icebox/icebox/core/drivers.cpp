#include "drivers.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "drivers"
#include "core.hpp"
#include "core_private.hpp"
#include "interfaces/if_os.hpp"
#include "log.hpp"
#include "utils/path.hpp"

#ifdef _MSC_VER
#    define stricmp _stricmp
#else
#    include <strings.h>
#    define stricmp strcasecmp
#endif

bool drivers::list(core::Core& core, on_driver_fn on_driver)
{
    return core.os_->driver_list(std::move(on_driver));
}

opt<driver_t> drivers::find(core::Core& core, uint64_t addr)
{
    auto found = opt<driver_t>{};
    drivers::list(core, [&](driver_t drv)
    {
        const auto span = drivers::span(core, drv);
        if(!span)
            return walk_e::next;

        if(span->addr <= addr && addr < span->addr + span->size)
            found = drv;

        return found ? walk_e::stop : walk_e::next;
    });
    return found;
}

opt<driver_t> drivers::find_name(core::Core& core, std::string_view name)
{
    auto found = opt<driver_t>{};
    drivers::list(core, [&](driver_t drv)
    {
        const auto drv_path = drivers::name(core, drv);
        if(!drv_path)
            return walk_e::next;

        const auto drv_name = path::filename(*drv_path);
        if(stricmp(drv_name.generic_string().data(), name.data()) != 0)
            return walk_e::next;

        found = drv;
        return walk_e::stop;
    });
    return found;
}

opt<std::string> drivers::name(core::Core& core, driver_t drv)
{
    return core.os_->driver_name(drv);
}

opt<span_t> drivers::span(core::Core& core, driver_t drv)
{
    return core.os_->driver_span(drv);
}

opt<bpid_t> drivers::listen_create(core::Core& core, const on_event_fn& on_load)
{
    return core.os_->listen_drv_create(on_load);
}
