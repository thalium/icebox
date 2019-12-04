#pragma once

namespace defer
{
    template <typename T>
    struct defer_t
    {
        defer_t(T op)
            : op(op)
        {
        }

        ~defer_t()
        {
            op();
        }

        T op;
    };

    template <typename T>
    defer_t<T> make(T op)
    {
        return defer_t<T>{op};
    }
} // namespace defer

#define CONCAT__(X, Y)  Y
#define CONCAT_(X, Y)   CONCAT__(~, X##Y)
#define CONCAT(X, Y)    CONCAT_(X, Y)

#define DEFER(X)    const auto CONCAT(defer_, __COUNTER__) = defer::make(X)
#define PYREF(X)    DEFER([=] { Py_DECREF(X); })
