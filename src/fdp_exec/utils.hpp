#pragma once

#define UNUSED(x) ((void) (x))
#define COUNT_OF(X) (sizeof(X)/sizeof*(X))

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

template<typename T>
struct Defer
{
    Defer(const T& defer)
        : defer_(defer)
    {
    }
    ~Defer()
    {
        defer_();
    }
    const T& defer_;
};

template<typename T>
Defer<T> defer(const T& defer)
{
    return Defer<T>(defer);
}

template<typename T, typename U>
static void erase_if(T& c, const U& pred)
{
    for(auto it = c.begin(), end = c.end(); it != end; /**/)
        if(pred(it->second))
            it = c.erase(it);
        else
            ++it;
}
