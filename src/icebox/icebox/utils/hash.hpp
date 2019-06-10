#pragma once

#include <cstddef>

namespace hash
{
    static inline void combine(std::size_t& /*seed*/) {}

    template <typename T, typename... Rest>
    static inline void combine(std::size_t& seed, const T& v, Rest... rest)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9E3779B97F4A7C15 + (seed << 6) + (seed >> 2);
        combine(seed, rest...);
    }
} // namespace hash
