#pragma once

#include "callstacks.hpp"

namespace callstacks
{
    struct Module
    {
        virtual ~Module() = default;

        virtual size_t  read        (caller_t* callers, size_t num_callers, proc_t proc) = 0;
        virtual size_t  read_from   (caller_t* callers, size_t num_callers, proc_t proc, const context_t& where) = 0;
        virtual bool    preload     (proc_t proc, const std::string& name, span_t span) = 0;
    };

    std::unique_ptr<Module> make_nt(core::Core& core);
} // namespace callstacks
