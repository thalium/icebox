#pragma once

#include "enums.hpp"
#include "types.hpp"

namespace core { struct Core; }

namespace callstack
{
    struct context_t
    {
        uint64_t ip; // instruction pointer
        uint64_t sp; // stack pointer
        uint64_t bp; // base pointer
        uint64_t cs; // code segment
    };

    struct callstep_t
    {
        uint64_t addr;
    };
    using on_callstep_fn = fn::view<walk_e(callstep_t)>;

    struct ICallstack
    {
        virtual ~ICallstack() = default;

        virtual bool    get_callstack               (proc_t proc, on_callstep_fn on_callstep) = 0;
        virtual bool    get_callstack_from_context  (proc_t proc, const context_t& first, flags_e flag, on_callstep_fn on_callstep) = 0;
    };

    std::shared_ptr<ICallstack> make_callstack_nt(core::Core& core);
} // namespace callstack
