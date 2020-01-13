#pragma once

#include "enums.hpp"
#include "types.hpp"

namespace core { struct Core; }

namespace os
{
    using bpid_t = uint64_t;

    bool        is_kernel_address   (core::Core&, uint64_t ptr);
    bool        read_page           (core::Core& core, void* dst, uint64_t ptr, proc_t* proc, dtb_t dtb);
    bool        write_page          (core::Core& core, uint64_t dst, const void* src, proc_t* proc, dtb_t dtb);
    opt<phy_t>  virtual_to_physical (core::Core& core, proc_t* proc, dtb_t dtb, uint64_t ptr);
    size_t      unlisten            (core::Core&, bpid_t bpid);
    void        debug_print         (core::Core&);
    bool        check_flags         (flags_t got, flags_t want);
} // namespace os
