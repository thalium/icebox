#pragma once

#include "types.hpp"
#include <functional>

namespace core { struct Core; }
namespace memory { struct Io; }

namespace symbols
{
    using on_name_fn = std::function<void(std::string_view)>;

    struct Symbol
    {
        std::string module;
        std::string symbol;
        uint64_t    offset;
    };

    constexpr auto kernel = proc_t{~0ull, {~0ull}, {~0ull}};

    bool        load_module_memory  (core::Core& core, proc_t proc, const memory::Io& io, span_t span);
    bool        load_module         (core::Core& core, proc_t proc, const std::string& name);
    bool        load_modules        (core::Core& core, proc_t proc);
    opt<bpid_t> autoload_modules    (core::Core& core, proc_t proc);
    bool        load_driver_memory  (core::Core& core, span_t span);
    bool        load_driver         (core::Core& core, driver_t driver);
    bool        load_drivers        (core::Core& core);
    bool        unload              (core::Core& core, proc_t proc, const std::string& module);

    opt<uint64_t>   address         (core::Core& core, proc_t proc, const std::string& module, const std::string& symbol);
    void            struc_names     (core::Core& core, proc_t proc, const std::string& module, const on_name_fn& on_struc);
    opt<size_t>     struc_size      (core::Core& core, proc_t proc, const std::string& module, const std::string& struc);
    void            struc_members   (core::Core& core, proc_t proc, const std::string& module, const std::string& struc, const on_name_fn& on_member);
    opt<uint64_t>   member_offset   (core::Core& core, proc_t proc, const std::string& module, const std::string& struc, const std::string& member);
    Symbol          find            (core::Core& core, proc_t proc, uint64_t addr);
    std::string     to_string       (const Symbol& symbol);
} // namespace symbols
