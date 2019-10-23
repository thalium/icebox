#pragma once

#include "enums.hpp"
#include "types.hpp"

#include <functional>

namespace core { struct Core; }
namespace sym { struct Symbols; }
namespace reader { struct Reader; }

namespace os
{
    using bpid_t = uint64_t;

    bool            is_kernel_address   (core::Core&, uint64_t ptr);
    bool            can_inject_fault    (core::Core&, uint64_t ptr);
    bool            reader_setup        (core::Core&, reader::Reader& reader, opt<proc_t> proc);
    sym::Symbols&   kernel_symbols      (core::Core&);

    size_t unlisten(core::Core&, bpid_t bpid);

    opt<arg_t>  read_stack  (core::Core&, size_t index);
    opt<arg_t>  read_arg    (core::Core&, size_t index);
    bool        write_arg   (core::Core&, size_t index, arg_t arg);

    void debug_print(core::Core&);
} // namespace os
