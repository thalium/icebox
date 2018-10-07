#pragma once

#include <stdint.h>

static inline uint16_t read_le16(const void* vptr)
{
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(vptr);
    return (((uint16_t) ptr[0]) << 0)
         | (((uint16_t) ptr[1]) << 8);
}

static inline uint32_t read_le32(const void* vptr)
{
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(vptr);
    return (((uint32_t) ptr[0]) <<  0)
         | (((uint32_t) ptr[1]) <<  8)
         | (((uint32_t) ptr[2]) << 16)
         | (((uint32_t) ptr[3]) << 24);
}

static inline uint64_t read_le64(const void* vptr)
{
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(vptr);
    return (((uint64_t) ptr[0]) <<  0)
         | (((uint64_t) ptr[1]) <<  8)
         | (((uint64_t) ptr[2]) << 16)
         | (((uint64_t) ptr[3]) << 24)
         | (((uint64_t) ptr[4]) << 32)
         | (((uint64_t) ptr[5]) << 40)
         | (((uint64_t) ptr[6]) << 48)
         | (((uint64_t) ptr[7]) << 56);
}

static inline void write_le16(void* vptr, uint16_t value)
{
    uint8_t* ptr = reinterpret_cast<uint8_t*>(vptr);
    ptr[0] = (value >> 0) & 0xFF;
    ptr[1] = (value >> 8) & 0xFF;
}

static inline void write_le32(void* vptr, uint32_t value)
{
    uint8_t* ptr = reinterpret_cast<uint8_t*>(vptr);
    ptr[0] = (value >>  0) & 0xFF;
    ptr[1] = (value >>  8) & 0xFF;
    ptr[2] = (value >> 16) & 0xFF;
    ptr[3] = (value >> 24) & 0xFF;
}

static inline void write_le64(void* vptr, uint64_t value)
{
    uint8_t* ptr = reinterpret_cast<uint8_t*>(vptr);
    ptr[0] = (value >>  0) & 0xFF;
    ptr[1] = (value >>  8) & 0xFF;
    ptr[2] = (value >> 16) & 0xFF;
    ptr[3] = (value >> 24) & 0xFF;
    ptr[4] = (value >> 32) & 0xFF;
    ptr[5] = (value >> 40) & 0xFF;
    ptr[6] = (value >> 48) & 0xFF;
    ptr[7] = (value >> 56) & 0xFF;
}

static inline uint16_t read_be16(const void* vptr)
{
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(vptr);
    return (((uint16_t) ptr[0]) << 8)
         | (((uint16_t) ptr[1]) << 0);
}

static inline uint32_t read_be32(const void* vptr)
{
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(vptr);
    return (((uint32_t) ptr[0]) << 24)
         | (((uint32_t) ptr[1]) << 16)
         | (((uint32_t) ptr[2]) <<  8)
         | (((uint32_t) ptr[3]) <<  0);
}

static inline uint64_t read_be64(const void* vptr)
{
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(vptr);
    return (((uint64_t) ptr[0]) << 56)
         | (((uint64_t) ptr[1]) << 48)
         | (((uint64_t) ptr[2]) << 40)
         | (((uint64_t) ptr[3]) << 32)
         | (((uint64_t) ptr[4]) << 24)
         | (((uint64_t) ptr[5]) << 16)
         | (((uint64_t) ptr[6]) <<  8)
         | (((uint64_t) ptr[7]) <<  0);
}

static inline void write_be16(void* vptr, uint16_t value)
{
    uint8_t* ptr = reinterpret_cast<uint8_t*>(vptr);
    ptr[0] = (value >> 8) & 0xFF;
    ptr[1] = (value >> 0) & 0xFF;
}

static inline void write_be32(void* vptr, uint32_t value)
{
    uint8_t* ptr = reinterpret_cast<uint8_t*>(vptr);
    ptr[0] = (value >> 24) & 0xFF;
    ptr[1] = (value >> 16) & 0xFF;
    ptr[2] = (value >>  8) & 0xFF;
    ptr[3] = (value >>  0) & 0xFF;
}

static inline void write_be64(void* vptr, uint64_t value)
{
    uint8_t* ptr = reinterpret_cast<uint8_t*>(vptr);
    ptr[0] = (value >> 56) & 0xFF;
    ptr[1] = (value >> 48) & 0xFF;
    ptr[2] = (value >> 40) & 0xFF;
    ptr[3] = (value >> 32) & 0xFF;
    ptr[4] = (value >> 24) & 0xFF;
    ptr[5] = (value >> 16) & 0xFF;
    ptr[6] = (value >>  8) & 0xFF;
    ptr[7] = (value >>  0) & 0xFF;
}
