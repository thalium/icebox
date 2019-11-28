#pragma once

#include "enums.hpp"
#include "types.hpp"

#include <functional>

namespace core { struct Core; }

namespace drivers
{
    using bpid_t       = uint64_t;
    using on_driver_fn = std::function<walk_e(driver_t)>;
    using on_event_fn  = std::function<void(driver_t, bool load)>;

    bool                list            (core::Core&, on_driver_fn on_driver);
    opt<driver_t>       find            (core::Core&, uint64_t addr);
    opt<driver_t>       find_name       (core::Core&, std::string_view name);
    opt<std::string>    name            (core::Core&, driver_t drv);
    opt<span_t>         span            (core::Core&, driver_t drv);
    opt<bpid_t>         listen_create   (core::Core&, const on_event_fn& on_load);
} // namespace drivers
