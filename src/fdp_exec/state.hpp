#pragma once

#include "types.hpp"

#include <functional>
#include <memory>

namespace core
{
    // auto-managed breakpoint object
    struct BreakpointPrivate;
    using Breakpoint = std::shared_ptr<BreakpointPrivate>;

    // generic functor object
    using Task = std::function<void(void)>;

    // whether to filter breakpoint on proc or thread
    enum filter_e
    {
        FILTER_THREAD,
        FILTER_PROC,
        ANY
    };

    enum join_e
    {
        JOIN_ANY_MODE,
        JOIN_USER_MODE
    };

    struct State
    {
         State();
        ~State();

        bool        pause           ();
        bool        resume          ();
        bool        wait            ();
        Breakpoint  set_breakpoint  (uint64_t ptr, proc_t proc, filter_e filter);
        Breakpoint  set_breakpoint  (uint64_t ptr, proc_t proc, filter_e filter, const Task& task);
        Breakpoint  set_breakpoint  (uint64_t ptr, proc_t proc, thread_t thread, filter_e filter);
        Breakpoint  set_breakpoint  (uint64_t ptr, proc_t proc, thread_t thread, filter_e filter, const Task& task);
        bool        proc_join       (proc_t proc, join_e join);

        // private data
        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace core
