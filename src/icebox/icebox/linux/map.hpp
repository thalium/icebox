#pragma once

#include "icebox/sym.hpp"

namespace sym
{
    struct Map
        : public sym::IMod
    {
         Map();
        ~Map();

        // methods
        bool    setup       ();
        bool    check_file  ();

        // IModule methods
        span_t              span        () override;
        opt<uint64_t>       symbol      (const std::string& symbol) override;
        opt<uint64_t>       struc_offset(const std::string& struc, const std::string& member) override;
        opt<size_t>         struc_size  (const std::string& struc) override;
        opt<sym::ModCursor> symbol      (uint64_t addr) override;
        bool                sym_list    (sym::on_sym_fn on_sym) override;

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace sym
