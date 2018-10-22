#pragma once

#include <inttypes.h>
#include <stdint.h>

#include <optional>

template<typename T>
using opt = std::optional<T>;

struct proc_t
{
    uint64_t id;
    uint64_t dtb;
};

struct thread_t
{
    uint64_t id;
};

struct mod_t
{
    uint64_t id;
};

struct driver_t
{
    uint64_t id;
};

struct span_t
{
    uint64_t addr;
    size_t   size;
};
