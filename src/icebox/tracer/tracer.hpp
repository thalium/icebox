#pragma once

#include <cstdint>

namespace core { struct Core; }

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

    void log_call(core::Core& core, const callcfg_t& call);
} // namespace tracer
