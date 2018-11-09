#pragma once

#include <nonstd/expected.hpp>

enum class err_e
{
    _ = 0, // discard 0 value

    cannot_read,
    cannot_write,
    invalid_input,
    input_too_small,
};

template<typename T>
using return_t = nonstd::expected<T, err_e>;
using status_t = return_t<void>;

namespace err
{
    constexpr nonstd::unexpected_type<err_e> make(err_e err)
    {
        return nonstd::make_unexpected(err);
    }

    static const status_t ok;
}
