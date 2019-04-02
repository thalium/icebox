#pragma once

#include <stdint.h>

#include "endian.hpp"
#include "utils.hpp"

namespace hex
{
    const char chars_upper[] = "0123456789ABCDEF";
    const char chars_lower[] = "0123456789abcdef";

    template <size_t szhex>
    void convert(char* dst, const char (&hexchars)[szhex], const void* vsrc, size_t size)
    {
        const uint8_t* src = static_cast<const uint8_t*>(vsrc);
        for(size_t i = 0; i < size; ++i)
        {
            dst[i * 2 + 0] = hexchars[src[i] >> 4];
            dst[i * 2 + 1] = hexchars[src[i] & 0x0F];
        }
    }

    enum BinHexFlags
    {
        LowerCase     = 1 << 0, // use lower-case instead of upper-case
        RemovePadding = 1 << 1, // do not pad with '0'
        HexaPrefix    = 1 << 3, // add 0x prefix
    };

    template <size_t size, uint32_t flags = 0, size_t szdst>
    const char* convert(char (&dst)[szdst], const void* src)
    {
        STATIC_ASSERT_EQ(szdst, !!(flags & HexaPrefix) * 2 + size * 2 + 1);
        const auto& hexchars = flags & LowerCase ? chars_lower : chars_upper;
        const auto prefix    = flags & HexaPrefix ? 2 : 0;
        convert(&dst[prefix], hexchars, src, size);
        dst[prefix + size * 2] = 0;
        size_t skip            = 0;
        // we need at least one 0
        if constexpr(!!(flags & RemovePadding))
            while(skip + 1 < size * 2 && dst[prefix + skip] == '0')
                skip++;
        if constexpr(!!(flags & HexaPrefix))
        {
            dst[prefix + skip - 2] = '0';
            dst[prefix + skip - 1] = 'x';
        }
        return &dst[skip];
    }

    inline uint8_t swap(uint8_t x) { return x; }
    inline uint16_t swap(uint16_t x) { return bswap16(x); }
    inline uint32_t swap(uint32_t x) { return bswap32(x); }
    inline uint64_t swap(uint64_t x) { return bswap64(x); }

    template <uint32_t flags = 0, size_t szdst, typename T>
    const char* convert(char (&dst)[szdst], T x)
    {
        x = swap(x);
        return convert<sizeof x, flags>(dst, &x);
    }
} // namespace hex
