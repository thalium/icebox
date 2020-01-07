#pragma once

#include "enums.hpp"
#include "types.hpp"

#include <functional>

namespace core { struct Core; }

namespace modules
{
    using on_mod_fn   = std::function<walk_e(mod_t)>;
    using on_event_fn = std::function<void(mod_t)>;

    bool                list            (core::Core&, proc_t proc, on_mod_fn on_mod);
    opt<std::string>    name            (core::Core&, proc_t proc, mod_t mod);
    bool                is_equal        (core::Core&, proc_t proc, mod_t mod, flags_t flags, std::string_view name);
    opt<span_t>         span            (core::Core&, proc_t proc, mod_t mod);
    opt<mod_t>          find            (core::Core&, proc_t proc, uint64_t addr);
    opt<mod_t>          find_name       (core::Core& core, proc_t proc, std::string_view name, flags_t flags);
    opt<bpid_t>         listen_create   (core::Core& core, proc_t proc, flags_t flags, const on_event_fn& on_load);
} // namespace modules
