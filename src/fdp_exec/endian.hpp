#pragma once

#include <stdint.h>
#include <string.h>

namespace
{
    static constexpr bool is_little_endian = true;
}

#ifdef _MSC_VER
#    include <stdlib.h>
#    define bswap16 _byteswap_ushort
#    define bswap32 _byteswap_ulong
#    define bswap64 _byteswap_uint64
#else
#    include <byteswap.h>
#    define bswap16 __builtin_bswap16
#    define bswap32 __builtin_bswap32
#    define bswap64 __builtin_bswap64
#endif

namespace endian
{
    static inline uint8_t bswap(uint8_t x) { return x; }
    static inline uint16_t bswap(uint16_t x) { return bswap16(x); }
    static inline uint32_t bswap(uint32_t x) { return bswap32(x); }
    static inline uint64_t bswap(uint64_t x) { return bswap64(x); }

    template <typename T, bool swap>
    static inline T read_bits(const void* ptr)
    {
        T value;
        memcpy(&value, ptr, sizeof value);
        return swap ? bswap(value) : value;
    }

    template <bool swap, typename T>
    static inline void write_bits(void* ptr, T value)
    {
        value = swap ? bswap(value) : value;
        memcpy(ptr, &value, sizeof value);
    }
} // namespace endian

static inline uint8_t read_byte(const void* ptr) { return endian::read_bits<uint8_t, !is_little_endian>(ptr); }
static inline uint16_t read_le16(const void* ptr) { return endian::read_bits<uint16_t, !is_little_endian>(ptr); }
static inline uint32_t read_le32(const void* ptr) { return endian::read_bits<uint32_t, !is_little_endian>(ptr); }
static inline uint64_t read_le64(const void* ptr) { return endian::read_bits<uint64_t, !is_little_endian>(ptr); }
static inline uint16_t read_be16(const void* ptr) { return endian::read_bits<uint16_t, is_little_endian>(ptr); }
static inline uint32_t read_be32(const void* ptr) { return endian::read_bits<uint32_t, is_little_endian>(ptr); }
static inline uint64_t read_be64(const void* ptr) { return endian::read_bits<uint64_t, is_little_endian>(ptr); }

static inline void write_byte(void* ptr, uint8_t x) { endian::write_bits<!is_little_endian>(ptr, x); }
static inline void write_le16(void* ptr, uint16_t x) { endian::write_bits<!is_little_endian>(ptr, x); }
static inline void write_le32(void* ptr, uint32_t x) { endian::write_bits<!is_little_endian>(ptr, x); }
static inline void write_le64(void* ptr, uint64_t x) { endian::write_bits<!is_little_endian>(ptr, x); }
static inline void write_be16(void* ptr, uint16_t x) { endian::write_bits<is_little_endian>(ptr, x); }
static inline void write_be32(void* ptr, uint32_t x) { endian::write_bits<is_little_endian>(ptr, x); }
static inline void write_be64(void* ptr, uint64_t x) { endian::write_bits<is_little_endian>(ptr, x); }
