#pragma once

#include "enums.hpp"
#include "types.hpp"

namespace core { struct Core; }

namespace function
{
    opt<arg_t>  read_stack  (core::Core&, size_t index);
    opt<arg_t>  read_arg    (core::Core&, size_t index);
    bool        write_arg   (core::Core&, size_t index, arg_t arg);
} // namespace function
