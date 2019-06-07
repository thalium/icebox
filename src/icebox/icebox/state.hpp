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

    struct State
    {
         State();
        ~State();

        bool        pause           ();
        bool        resume          ();
        bool        single_step     ();
        bool        wait            ();
        Breakpoint  set_breakpoint  (std::string_view name, uint64_t ptr, const Task& task);
        Breakpoint  set_breakpoint  (std::string_view name, uint64_t ptr, proc_t proc, const Task& task);
        Breakpoint  set_breakpoint  (std::string_view name, uint64_t ptr, thread_t thread, const Task& task);
        Breakpoint  set_breakpoint  (std::string_view name, phy_t phy, const Task& task);
        Breakpoint  set_breakpoint  (std::string_view name, phy_t phy, proc_t proc, const Task& task);
        Breakpoint  set_breakpoint  (std::string_view name, phy_t phy, thread_t thread, const Task& task);
        void        run_to_proc     (std::string_view name, proc_t proc);
        void        run_to_proc     (std::string_view name, proc_t proc, uint64_t ptr);
        void        run_to_current  (std::string_view name);

        // private data
        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace core
