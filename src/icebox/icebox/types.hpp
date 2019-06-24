#pragma once

#include <inttypes.h>
#include <stdint.h>

#ifdef _MSC_VER
#    include <filesystem>
#    include <optional>
template <typename T>
using opt     = std::optional<T>;
namespace ext = std;
namespace fs  = std::filesystem;
#else
#    include <experimental/filesystem>
#    include <experimental/optional>
template <typename T>
using opt     = std::experimental::optional<T>;
namespace ext = std::experimental;
namespace fs  = std::experimental::filesystem;
#endif

#include "enums.hpp"

struct dtb_t
{
    uint64_t val;
};

struct phy_t
{
    uint64_t val;
};

struct proc_t
{
    uint64_t id;
    dtb_t    dtb;
};

struct thread_t
{
    uint64_t id;
};

struct mod_t
{
    uint64_t id;
    flags_e  flags;
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

struct arg_t
{
    uint64_t val;
};

struct vm_area_t
{
    uint64_t id;
};

namespace fn
{
    template <typename T>
    struct view;
}
