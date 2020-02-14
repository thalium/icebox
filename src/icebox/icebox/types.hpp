#pragma once

#include <inttypes.h>
#include <optional>
#include <stdint.h>

#ifdef _MSC_VER
#    include <filesystem>
namespace fs = std::filesystem;
#else
#    include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#include "enums.hpp"

template <typename T>
using opt = std::optional<T>;

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
    dtb_t    kdtb;
    dtb_t    udtb;
};

struct thread_t
{
    uint64_t id;
};

struct flags_t
{
    uint8_t is_x64  : 1;
    uint8_t is_x86  : 1;
};

namespace flags
{
    constexpr auto x64 = flags_t{1, 0};
    constexpr auto x86 = flags_t{0, 1};
} // namespace flags

struct mod_t
{
    uint64_t id;
    flags_t  flags;
    uint8_t  _[3];
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

struct bpid_t
{
    uint64_t id;
};
