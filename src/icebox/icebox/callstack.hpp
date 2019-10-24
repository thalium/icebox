#pragma once

#include "enums.hpp"
#include "types.hpp"

namespace core { struct Core; }

namespace callstack
{
    struct context_t
    {
        uint64_t ip;    // instruction pointer
        uint64_t sp;    // stack pointer
        uint64_t bp;    // base pointer
        uint64_t cs;    // code segment
        flags_e  flags; // process flags
    };

    struct caller_t
    {
        uint64_t addr;
    };

    struct ICallstack
    {
        virtual ~ICallstack() = default;

        virtual size_t  read        (caller_t* callers, size_t num_callers, proc_t proc) = 0;
        virtual size_t  read_from   (caller_t* callers, size_t num_callers, proc_t proc, const context_t& where) = 0;
    };

    std::shared_ptr<ICallstack> make_callstack_nt(core::Core& core);
} // namespace callstack
