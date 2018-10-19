#pragma once

#include <stdint.h>

namespace
{
    static constexpr bool is_little_endian = true;
}

#ifdef _MSC_VER
#include <stdlib.h>
#define bswap16 _byteswap_ushort
#define bswap32 _byteswap_ulong
#define bswap64 _byteswap_uint64
#else
#include <byteswap.h>
#define bswap16 __builtin_bswap16
#define bswap32 __builtin_bswap32
#define bswap64 __builtin_bswap64
#endif

static inline uint16_t read_le16(const void* vptr)
{
    const auto value = *reinterpret_cast<const uint16_t*>(vptr);
    return is_little_endian ? value : bswap16(value);
}

static inline uint32_t read_le32(const void* vptr)
{
    const auto value = *reinterpret_cast<const uint32_t*>(vptr);
    return is_little_endian ? value : bswap32(value);
}

static inline uint64_t read_le64(const void* vptr)
{
    const auto value = *reinterpret_cast<const uint64_t*>(vptr);
    return is_little_endian ? value : bswap64(value);
}

static inline void write_le16(void* vptr, uint16_t value)
{
    *reinterpret_cast<uint16_t*>(vptr) = is_little_endian ? value : bswap16(value);
}

static inline void write_le32(void* vptr, uint32_t value)
{
    *reinterpret_cast<uint32_t*>(vptr) = is_little_endian ? value : bswap32(value);
}

static inline void write_le64(void* vptr, uint64_t value)
{
    *reinterpret_cast<uint64_t*>(vptr) = is_little_endian ? value: bswap64(value);
}

static inline uint16_t read_be16(const void* vptr)
{
    const auto value = *reinterpret_cast<const uint16_t*>(vptr);
    return is_little_endian ? bswap16(value) : value;
}

static inline uint32_t read_be32(const void* vptr)
{
    const auto value = *reinterpret_cast<const uint32_t*>(vptr);
    return is_little_endian ? bswap32(value) : value;
}

static inline uint64_t read_be64(const void* vptr)
{
    const auto value = *reinterpret_cast<const uint64_t*>(vptr);
    return is_little_endian ? bswap64(value) : value;
}

static inline void write_be16(void* vptr, uint16_t value)
{
    *reinterpret_cast<uint16_t*>(vptr) = is_little_endian ? bswap16(value) : value;
}

static inline void write_be32(void* vptr, uint32_t value)
{
    *reinterpret_cast<uint32_t*>(vptr) = is_little_endian ? bswap32(value) : value;
}

static inline void write_be64(void* vptr, uint64_t value)
{
    *reinterpret_cast<uint64_t*>(vptr) = is_little_endian ? bswap64(value) : value;
}
