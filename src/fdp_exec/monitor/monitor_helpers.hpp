#pragma once

#include "types.hpp"
#include "core.hpp"

namespace monitor_helpers
{
    return_t<arg_t>    get_param_by_index(core::Core& core, int index);
    return_t<uint64_t> get_stack_by_index(core::Core& core, int index);
    return_t<uint64_t> get_return_value  (core::Core& core, proc_t proc);

}
