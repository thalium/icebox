#pragma once

#include "core.hpp"
#include "endian.hpp"
#include "log.hpp"
#include "types.hpp"

namespace core
{
    template <typename T, T (*read)(const void*)>
    return_t<T> read_mem(core::Core& core, dtb_t dtb, uint64_t ptr)
    {
        T value;
        const auto ok = core.mem.read_virtual(&value, dtb, ptr, sizeof value);
        if(!ok)
            return {};

        return read(&value);
    }

    static inline auto read_byte(Core& core, dtb_t dtb, uint64_t ptr) { return read_mem<uint8_t, &::read_byte>(core, dtb, ptr); }
    static inline auto read_le16(Core& core, dtb_t dtb, uint64_t ptr) { return read_mem<uint16_t, &::read_le16>(core, dtb, ptr); }
    static inline auto read_le32(Core& core, dtb_t dtb, uint64_t ptr) { return read_mem<uint32_t, &::read_le32>(core, dtb, ptr); }
    static inline auto read_le64(Core& core, dtb_t dtb, uint64_t ptr) { return read_mem<uint64_t, &::read_le64>(core, dtb, ptr); }
    static inline auto read_be16(Core& core, dtb_t dtb, uint64_t ptr) { return read_mem<uint16_t, &::read_be16>(core, dtb, ptr); }
    static inline auto read_be32(Core& core, dtb_t dtb, uint64_t ptr) { return read_mem<uint32_t, &::read_be32>(core, dtb, ptr); }
    static inline auto read_be64(Core& core, dtb_t dtb, uint64_t ptr) { return read_mem<uint64_t, &::read_be64>(core, dtb, ptr); }

    static inline auto read_ptr(Core& core, dtb_t dtb, uint64_t ptr)
    {
        if constexpr(is_little_endian)
            return read_mem<uint64_t, ::read_le64>(core, dtb, ptr);
        else
            return read_mem<uint64_t, ::read_be64>(core, dtb, ptr);
    }

    static inline auto read_ptr(Core& core, uint64_t ptr)
    {
        const auto dtb = dtb_t{core.regs.read(FDP_CR3_REGISTER)};
        return read_ptr(core, dtb, ptr);
    }

} // namespace core
