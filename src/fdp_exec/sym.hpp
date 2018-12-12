#pragma once

#include "enums.hpp"
#include "types.hpp"

namespace sym
{
    struct ModCursor
    {
        std::string symbol;
        uint64_t    offset;
    };

    using on_sym_fn = fn::view<walk_e(std::string, uint64_t)>;

    struct IMod
    {
        virtual ~IMod() = default;

        virtual span_t          span        () = 0;
        virtual opt<uint64_t>   symbol      (const std::string& symbol) = 0;
        virtual opt<uint64_t>   struc_offset(const std::string& struc, const std::string& member) = 0;
        virtual opt<size_t>     struc_size  (const std::string& struc) = 0;
        virtual opt<ModCursor>  symbol      (uint64_t addr) = 0;
        virtual bool            sym_list    (const on_sym_fn& on_sym) = 0;
    };

    std::unique_ptr<IMod>   make_pdb    (span_t span, const std::string& module, const std::string& guid);
    std::unique_ptr<IMod>   make_pdb    (span_t span, const void* data, const size_t data_size);
    std::unique_ptr<IMod>   make_empty  (span_t span, const void* data, const size_t data_size);

    struct Cursor
    {
        std::string module;
        std::string symbol;
        uint64_t    offset;
    };

    std::string to_string(const Cursor& cursor);

    struct Symbols
    {
         Symbols();
        ~Symbols();

        using on_module_fn = fn::view<walk_e(const IMod& module)>;

        bool            insert      (const std::string& name, std::unique_ptr<IMod>& module);
        bool            insert      (const std::string& name, span_t module, const void* data, const size_t data_size);
        bool            remove      (const std::string& name);
        bool            list        (const on_module_fn& on_module);
        IMod*           find        (const std::string& name);
        opt<uint64_t>   symbol      (const std::string& module, const std::string& symbol);
        opt<uint64_t>   struc_offset(const std::string& module, const std::string& struc, const std::string& member);
        opt<size_t>     struc_size  (const std::string& module, const std::string& struc);
        opt<Cursor>     find        (uint64_t addr);

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace sym
