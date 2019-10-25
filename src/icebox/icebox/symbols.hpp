#pragma once

#include "types.hpp"

#include <functional>

namespace core { struct Core; }

namespace symbols
{
    using bpid_t              = uint64_t;
    using module_predicate_fn = std::function<bool(mod_t, const std::string&)>;

    struct Symbol
    {
        std::string module;
        std::string symbol;
        uint64_t    offset;
    };

    constexpr auto kernel = proc_t{~0ull, {~0ull}};

    bool        load_module_at  (core::Core& core, proc_t proc, const std::string& module, span_t span);
    bool        load_module     (core::Core& core, proc_t proc, mod_t mod);
    bool        load_modules    (core::Core& core, proc_t proc);
    opt<bpid_t> listen_and_load (core::Core& core, proc_t proc, const module_predicate_fn& predicate);
    void        unlisten        (core::Core& core, bpid_t bpid);
    bool        load_driver_at  (core::Core& core, const std::string& driver, span_t span);
    bool        load_driver     (core::Core& core, driver_t driver);
    bool        load_drivers    (core::Core& core);
    bool        unload          (core::Core& core, proc_t proc, const std::string& module);

    opt<uint64_t>   symbol      (core::Core& core, proc_t proc, const std::string& module, const std::string& symbol);
    opt<uint64_t>   struc_offset(core::Core& core, proc_t proc, const std::string& module, const std::string& struc, const std::string& member);
    opt<size_t>     struc_size  (core::Core& core, proc_t proc, const std::string& module, const std::string& struc);
    Symbol          find        (core::Core& core, proc_t proc, uint64_t addr);
    std::string     to_string   (const Symbol& symbol);
} // namespace symbols
