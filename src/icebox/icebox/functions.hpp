#pragma once

#include "enums.hpp"
#include "types.hpp"

#include <functional>

namespace core { struct Core; }

namespace functions
{
    using bpid_t       = uint64_t;
    using on_return_fn = std::function<void(void)>;

    opt<arg_t>  read_stack      (core::Core&, size_t index);
    opt<arg_t>  read_arg        (core::Core&, size_t index);
    bool        write_arg       (core::Core&, size_t index, arg_t arg);
    bool        break_on_return (core::Core&, std::string_view name, const on_return_fn& on_return);
} // namespace functions
