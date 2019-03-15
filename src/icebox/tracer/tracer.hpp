#pragma once

#include <cstdint>

namespace core { struct Core; }

namespace tracer
{
    struct argcfg_t
    {
        const char* type;
        const char* name;
        size_t      size;
    };

    struct callcfg_t
    {
        const char* name;
        size_t      argc;
        argcfg_t    args[20];
    };

    void log_call(core::Core& core, const callcfg_t& call);
} // namespace tracer
