#pragma once

#include <inttypes.h>
#include <stdint.h>
#include <inttypes.h>

#include <experimental/optional>

template<typename T>
using opt = std::experimental::optional<T>;
namespace exp = std::experimental;

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
