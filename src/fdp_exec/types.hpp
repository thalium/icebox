#pragma once

#include <stdint.h>

#include <optional>

template<typename T>
using opt = std::optional<T>;

using proc_t = struct
{
    uint64_t id;
    uint64_t dtb;
};

using mod_t = uint64_t;

using driver_t = uint64_t;

using span_t = struct
{
    uint64_t addr;
    size_t   size;
};
