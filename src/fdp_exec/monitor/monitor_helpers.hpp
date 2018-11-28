#pragma once

#include "core.hpp"
#include "types.hpp"

namespace monitor
{
    return_t<arg_t>     get_arg_by_index    (core::Core& core, size_t index);
    status_t            set_arg_by_index    (core::Core& core, size_t index, uint64_t value);
    return_t<uint64_t>  get_stack_by_index  (core::Core& core, size_t index);
    status_t            set_stack_by_index  (core::Core& core, size_t index, uint64_t value);
    return_t<uint64_t>  get_return_value    (core::Core& core, proc_t proc);
    status_t            set_return_value    (core::Core& core, uint64_t value);
} // namespace monitor
