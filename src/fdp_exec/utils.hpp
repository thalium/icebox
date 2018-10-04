#pragma once

#define UNUSED(x)

constexpr bool is_power_of_2(int n)
{
    return n && !(n & (n - 1));
}

template<int n, typename T>
T align(T x)
{
    static_assert(is_power_of_2(n), "alignment must be power of two");
    return x & ~(n - 1);
}