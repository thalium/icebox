#pragma once

#include <stdint.h>

#include <optional>

template<typename T>
using opt = std::optional<T>;

using proc_t = struct
{
    uint64_t id;
    uint64_t ctx;
};

using mod_t = uint64_t;