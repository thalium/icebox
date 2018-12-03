#pragma once

#include "core.hpp"
#include "types.hpp"

#include "syscalls_public.gen.hpp"

#include <functional>
#include <vector>

namespace monitor
{
    DECLARE_SYSCALLS_CALLBACK_PROTOS

    struct GenericMonitor
    {
         GenericMonitor(core::Core& core);
        ~GenericMonitor();

        using on_function_generic_fn = std::function<void()>;
        using on_arg_fn              = std::function<walk_e(arg_t)>;

        bool    setup_all   (proc_t proc, on_function_generic_fn& on_function_generic);
        bool    setup_func  (proc_t proc, std::string fname);
        bool    get_raw_args(size_t nargs, const on_arg_fn& on_arg);

        DECLARE_SYSCALLS_FUNCTIONS_PROTOS

        struct Data;
        std::unique_ptr<Data> d_;

        core::Core& core_;
    };
} // namespace monitor
