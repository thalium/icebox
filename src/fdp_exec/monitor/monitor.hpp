#pragma once

#include "types.hpp"

namespace core { struct Core; }

namespace monitor
{
    opt<arg_t>      get_arg_by_index    (core::Core& core, size_t index);
    bool            set_arg_by_index    (core::Core& core, size_t index, uint64_t value);
    opt<uint64_t>   get_stack_by_index  (core::Core& core, size_t index);
    bool            set_stack_by_index  (core::Core& core, size_t index, uint64_t value);
    opt<uint64_t>   get_return_value    (core::Core& core, proc_t proc);
    bool            set_return_value    (core::Core& core, uint64_t value);
} // namespace monitor
