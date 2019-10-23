#pragma once

#include "enums.hpp"
#include "types.hpp"

#include <functional>

namespace core { struct Core; }

namespace threads
{
    using on_thread_fn = std::function<walk_e(thread_t)>;

    bool            list            (core::Core&, proc_t proc, on_thread_fn on_thread);
    opt<thread_t>   current         (core::Core&);
    opt<proc_t>     process         (core::Core&, thread_t thread);
    opt<uint64_t>   program_counter (core::Core&, proc_t proc, thread_t thread);
    uint64_t        tid             (core::Core&, proc_t proc, thread_t thread);
} // namespace threads
