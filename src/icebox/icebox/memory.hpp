#pragma once

#include "types.hpp"

namespace core { struct Core; }

namespace memory
{
    opt<phy_t>  virtual_to_physical         (core::Core& core, proc_t proc, uint64_t ptr);
    opt<phy_t>  virtual_to_physical_with_dtb(core::Core& core, dtb_t dtb, uint64_t ptr);
    bool        read_virtual                (core::Core& core, proc_t proc, void* dst, uint64_t src, size_t size);
    bool        read_virtual_with_dtb       (core::Core& core, dtb_t dtb, void* dst, uint64_t src, size_t size);
    bool        read_physical               (core::Core& core, void* dst, uint64_t src, size_t size);
    bool        write_virtual               (core::Core& core, proc_t proc, uint64_t dst, const void*, size_t size);
    bool        write_virtual_with_dtb      (core::Core& core, dtb_t dtb, uint64_t dst, const void*, size_t size);
    bool        write_physical              (core::Core& core, uint64_t dst, const void* src, size_t size);

    struct Io
    {
        ~Io() = default;

        // read methods
        opt<uint8_t>    byte    (uint64_t ptr) const;
        opt<uint16_t>   le16    (uint64_t ptr) const;
        opt<uint32_t>   le32    (uint64_t ptr) const;
        opt<uint64_t>   le64    (uint64_t ptr) const;
        opt<uint16_t>   be16    (uint64_t ptr) const;
        opt<uint32_t>   be32    (uint64_t ptr) const;
        opt<uint64_t>   be64    (uint64_t ptr) const;
        opt<uint64_t>   read    (uint64_t ptr) const;
        bool            read_all(void* dst, uint64_t ptr, size_t size) const;
        opt<phy_t>      physical(uint64_t ptr) const;

        // write methods
        bool    write_byte  (uint64_t ptr, uint8_t arg) const;
        bool    write_le16  (uint64_t ptr, uint16_t arg) const;
        bool    write_le32  (uint64_t ptr, uint32_t arg) const;
        bool    write_le64  (uint64_t ptr, uint64_t arg) const;
        bool    write_be16  (uint64_t ptr, uint16_t arg) const;
        bool    write_be32  (uint64_t ptr, uint32_t arg) const;
        bool    write_be64  (uint64_t ptr, uint64_t arg) const;
        bool    write       (uint64_t ptr, uint64_t arg) const;
        bool    write_all   (uint64_t ptr, const void* src, size_t size) const;

        core::Core& core;
        opt<proc_t> proc;
        dtb_t       dtb;
    };

    Io  make_io_kernel  (core::Core& core);
    Io  make_io_current (core::Core& core);
    Io  make_io         (core::Core& core, proc_t proc);
} // namespace memory
