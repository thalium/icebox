#pragma once

#include "enums.hpp"
#include "types.hpp"

#include <functional>

namespace core { struct Core; }

namespace process
{
    enum join_e
    {
        JOIN_ANY_MODE,
        JOIN_USER_MODE
    };

    using on_proc_fn = std::function<walk_e(proc_t)>;

    bool                list        (core::Core&, on_proc_fn on_proc);
    opt<proc_t>         current     (core::Core&);
    opt<proc_t>         find_name   (core::Core&, std::string_view name, flags_e flags);
    opt<proc_t>         find_pid    (core::Core&, uint64_t pid);
    opt<std::string>    name        (core::Core&, proc_t proc);
    bool                is_valid    (core::Core&, proc_t proc);
    uint64_t            pid         (core::Core&, proc_t proc);
    flags_e             flags       (core::Core&, proc_t proc);
    void                join        (core::Core&, proc_t proc, join_e join);
    opt<phy_t>          resolve     (core::Core&, proc_t proc, uint64_t ptr);
    opt<proc_t>         select      (core::Core&, proc_t proc, uint64_t ptr);
    opt<proc_t>         parent      (core::Core&, proc_t proc);
} // namespace process
