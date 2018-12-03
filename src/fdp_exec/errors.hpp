#pragma once

#include <nonstd/expected.hpp>

enum class err_e
{
    _ = 0, // discard 0 value

    cannot_read,
    cannot_write,
    invalid_input,
    input_too_small,
    cannot_open,
};

template <typename T>
using return_t      = nonstd::expected<T, err_e>;
using status_t      = return_t<void>;
using error_unexp_t = nonstd::unexpected_type<err_e>;

namespace err
{
    constexpr error_unexp_t make(err_e err)
    {
        return error_unexp_t{err};
    }

    static const status_t ok;
} // namespace err
