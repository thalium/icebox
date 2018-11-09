#pragma once

#include "types.hpp"
#include "core.hpp"

namespace monitor_helpers
{
    opt<arg_t>    get_param_by_index(core::Core& core, int index);
    opt<uint64_t> get_stack_by_index(core::Core& core, int index);
    opt<uint64_t> get_return_value  (core::Core& core, proc_t proc);

}
