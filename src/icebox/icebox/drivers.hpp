#pragma once

#include "enums.hpp"
#include "types.hpp"

#include <functional>

namespace core { struct Core; }

namespace drivers
{
    using on_driver_fn = std::function<walk_e(driver_t)>;

    bool                list(core::Core&, on_driver_fn on_driver);
    opt<driver_t>       find(core::Core&, uint64_t addr);
    opt<std::string>    name(core::Core&, driver_t drv);
    opt<span_t>         span(core::Core&, driver_t drv);
} // namespace drivers
