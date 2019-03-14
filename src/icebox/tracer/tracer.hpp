#pragma once

#include <cstdint>

namespace tracer
{
    struct argcfg_t
    {
        char   type[64];
        char   name[64];
        size_t size;
    };

    struct callcfg_t
    {
        char     name[64];
        size_t   argc;
        argcfg_t args[32];
    };
} // namespace tracer
