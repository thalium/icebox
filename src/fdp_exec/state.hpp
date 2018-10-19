#pragma once

#include "types.hpp"

#include <memory>
#include <functional>

namespace core
{
    // auto-managed breakpoint object
    struct BreakpointPrivate;
    using Breakpoint = std::shared_ptr<BreakpointPrivate>;

    // generic functor object
    using Task = std::function<void(void)>;

    // whether to filter breakpoint on cr3
    enum filter_e { FILTER_CR3, ANY_CR3 };

    struct State
    {
         State();
        ~State();

        bool        pause           ();
        bool        resume          ();
        bool        wait            ();
        Breakpoint  set_breakpoint  (uint64_t ptr, proc_t proc, filter_e filter, const Task& task);

        // private data
        struct Data;
        std::unique_ptr<Data> d_;
    };
}
