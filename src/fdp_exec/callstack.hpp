#pragma once

#include "enums.hpp"
#include "types.hpp"

namespace core { struct Core; }
namespace pe { struct Pe; }

namespace callstack
{
    struct callstep_t
    {
        uint64_t addr;
    };
    using on_callstep_fn = fn::view<walk_e(callstep_t)>;

    struct context_t
    {
        uint64_t rip;
        uint64_t rsp;
        uint64_t rbp;
        bool     is_x64;
    };

    struct ICallstack
    {
        virtual ~ICallstack() = default;

        virtual bool get_callstack(proc_t proc, context_t ctx, const on_callstep_fn& on_callstep) = 0;
    };

    std::shared_ptr<ICallstack> make_callstack_nt(core::Core& core);
} // namespace callstack
