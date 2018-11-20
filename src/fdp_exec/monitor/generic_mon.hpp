#pragma once

#include "types.hpp"
#include "core.hpp"

#include "syscall_mon_public.gen.hpp"

#include <functional>
#include <vector>

namespace monitor
{
    DECLARE_SYSCALLS_CALLBACK_PROTOS

    struct GenericMonitor
    {
         GenericMonitor(core::Core& core);
        ~GenericMonitor();

        using on_param_fn = std::function<walk_e(arg_t)>;

        bool             setup               (proc_t proc);
        bool             get_raw_args        (size_t nargs, const on_param_fn& on_param);

        DECLARE_SYSCALLS_FUNCTIONS_PROTOS

        struct Data;
        std::unique_ptr<Data> d_;

        core::Core&             core_;
    };
}
