#pragma once

#include "types.hpp"

namespace core { struct Core; }

namespace memory
{
    opt<phy_t>  virtual_to_physical     (core::Core& core, uint64_t ptr, dtb_t dtb);
    bool        read_virtual            (core::Core& core, void* dst, uint64_t src, size_t size);
    bool        read_virtual_with_dtb   (core::Core& core, void* dst, dtb_t dtb, uint64_t src, size_t size);
    bool        read_physical           (core::Core& core, void* dst, uint64_t src, size_t size);
} // namespace memory
