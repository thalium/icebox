#pragma once

#include <inttypes.h>
#include <stdint.h>
#include <inttypes.h>

#ifdef _MSC_VER
#include <optional>
template<typename T>
using opt = std::optional<T>;
namespace ext = std;
#else
#include <experimental/optional>
template<typename T>
using opt = std::experimental::optional<T>;
namespace ext = std::experimental;
#endif

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
