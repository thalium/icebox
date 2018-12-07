#pragma once

#include "errors.hpp"
#include "types.hpp"

namespace core
{
    struct Core;
};

namespace reader
{
    struct Reader
    {
        ~Reader() = default;

        return_t<uint8_t>   byte    (uint64_t ptr) const;
        return_t<uint16_t>  le16    (uint64_t ptr) const;
        return_t<uint32_t>  le32    (uint64_t ptr) const;
        return_t<uint64_t>  le64    (uint64_t ptr) const;
        return_t<uint16_t>  be16    (uint64_t ptr) const;
        return_t<uint32_t>  be32    (uint64_t ptr) const;
        return_t<uint64_t>  be64    (uint64_t ptr) const;
        return_t<uint64_t>  read    (uint64_t ptr) const;
        status_t            read    (void* dst, uint64_t ptr, size_t size) const;
        return_t<phy_t>     physical(uint64_t ptr) const;

        core::Core& core_;
        dtb_t       udtb_;
        dtb_t       kdtb_;
    };

    Reader  make(core::Core& core);
    Reader  make(core::Core& core, proc_t proc);

} // namespace reader
