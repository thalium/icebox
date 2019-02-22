#pragma once

#include "core.hpp"

namespace waiter
{
    opt<proc_t> proc_wait   (core::Core& core, std::string_view proc_name);
    opt<mod_t>  mod_wait    (core::Core& core, proc_t proc, std::string_view mod_name, opt<span_t>& mod_span);
} // namespace waiter
