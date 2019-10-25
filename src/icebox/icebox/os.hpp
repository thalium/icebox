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

    bool    is_kernel_address   (core::Core&, uint64_t ptr);
    bool    can_inject_fault    (core::Core&, uint64_t ptr);
    bool    reader_setup        (core::Core&, reader::Reader& reader, opt<proc_t> proc);
    size_t  unlisten            (core::Core&, bpid_t bpid);
    void    debug_print         (core::Core&);
} // namespace os
