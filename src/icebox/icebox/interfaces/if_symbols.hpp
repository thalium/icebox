#pragma once

#include "symbols.hpp"

#include <functional>

namespace reader { struct Reader; }

namespace symbols
{
    struct Offset
    {
        std::string symbol;
        size_t      offset;
    };

    using on_symbol_fn = std::function<walk_e(const std::string&, size_t)>;

    struct Module
    {
        virtual ~Module() = default;

        virtual std::string_view    id          () = 0;
        virtual opt<size_t>         symbol      (const std::string& symbol) = 0;
        virtual opt<size_t>         struc_offset(const std::string& struc, const std::string& member) = 0;
        virtual opt<size_t>         struc_size  (const std::string& struc) = 0;
        virtual opt<Offset>         symbol      (size_t offset) = 0;
        virtual bool                sym_list    (on_symbol_fn on_symbol) = 0;
    };

    std::shared_ptr<Module> make_pdb    (const std::string& module, const std::string& guid);
    std::shared_ptr<Module> make_pdb    (span_t span, const reader::Reader& reader);
    std::shared_ptr<Module> make_dwarf  (const std::string& module, const std::string& guid);
    std::shared_ptr<Module> make_dwarf  (span_t span, const reader::Reader& reader);

    struct Modules
    {
         Modules(core::Core& core);
        ~Modules();

        using on_module_fn = std::function<walk_e(span_t span, const Module& module)>;

        bool    insert  (proc_t proc, const std::string& module, span_t span, const std::shared_ptr<Module>& symbols);
        bool    insert  (proc_t proc, const std::string& module, span_t span);
        bool    remove  (proc_t proc, const std::string& module);

        bool            list        (proc_t proc, const on_module_fn& on_module);
        Module*         find        (proc_t proc, const std::string& name);
        opt<uint64_t>   symbol      (proc_t proc, const std::string& module, const std::string& symbol);
        opt<size_t>     struc_offset(proc_t proc, const std::string& module, const std::string& struc, const std::string& member);
        opt<size_t>     struc_size  (proc_t proc, const std::string& module, const std::string& struc);
        Symbol          find        (proc_t proc, uint64_t addr);

        // remove me
        static Modules& modules(core::Core& core);

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace symbols
