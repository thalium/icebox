#pragma once

#include "icebox/sym.hpp"

namespace sym
{
    struct Map
        : public sym::IMod
    {
        Map(fs::path path);

        // methods
        bool            setup   ();
        bool            set_aslr(const std::string& symbol, const uint64_t addr);
        opt<uint64_t>   get_aslr();

        // IModule methods
        span_t              span        () override;
        opt<uint64_t>       symbol      (const std::string& symbol) override;
        opt<uint64_t>       struc_offset(const std::string& struc, const std::string& member) override;
        opt<size_t>         struc_size  (const std::string& struc) override;
        opt<sym::ModCursor> symbol      (uint64_t addr) override;
        bool                sym_list    (sym::on_sym_fn on_sym) override;
    };

    std::unique_ptr<sym::Map> make_map(span_t span, const std::string& module, const std::string& guid);

} // namespace sym
