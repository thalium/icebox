#pragma once

#define UNUSED(x)   ((void) (x))
#define COUNT_OF(X) (sizeof(X) / sizeof *(X))

namespace utils
{
    constexpr bool is_power_of_2(int n)
    {
        return n && !(n & (n - 1));
    }

    template <int n, typename T>
    T align(T x)
    {
        static_assert(is_power_of_2(n), "alignment must be power of two");
        return x & ~(n - 1);
    }

    template <int64_t WANT, int64_t GOT>
    struct expect_eq
    {
        static_assert(WANT == GOT, "size mismatch");
        static constexpr bool ok = true;
    };
#define STATIC_ASSERT_EQ(A, B) static_assert(!!utils::expect_eq<A, B>::ok);
} // namespace utils
