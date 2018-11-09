#pragma once

#include "core.hpp"
#include "endian.hpp"
#include "log.hpp"

// uint helpers
template<typename T, T(*read)(const void*)>
opt<T> read_mem(core::Core& core, uint64_t ptr)
{
    T value;
    const auto ok = core.mem.virtual_read(&value, ptr, sizeof value);
    if(!ok)
        FAIL({}, "unable to read %zd bits at 0x%" PRIx64 "", 8 * sizeof value, ptr);

    return read(&value);
}

namespace core
{
    static inline auto read_byte(Core& core, uint64_t ptr) { return read_mem<uint8_t, &::read_byte>(core, ptr); }
    static inline auto read_le16(Core& core, uint64_t ptr) { return read_mem<uint16_t, &::read_le16>(core, ptr); }
    static inline auto read_le32(Core& core, uint64_t ptr) { return read_mem<uint32_t, &::read_le32>(core, ptr); }
    static inline auto read_le64(Core& core, uint64_t ptr) { return read_mem<uint64_t, &::read_le64>(core, ptr); }
    static inline auto read_be16(Core& core, uint64_t ptr) { return read_mem<uint16_t, &::read_be16>(core, ptr); }
    static inline auto read_be32(Core& core, uint64_t ptr) { return read_mem<uint32_t, &::read_be32>(core, ptr); }
    static inline auto read_be64(Core& core, uint64_t ptr) { return read_mem<uint64_t, &::read_be64>(core, ptr); }
    static inline auto read_ptr (Core& core, uint64_t ptr) { return is_little_endian ?
                                                                    read_mem<uint64_t, &::read_le64>(core, ptr)
                                                                  : read_mem<uint64_t, &::read_be64>(core, ptr); }
}
