#pragma once

#include "interfaces/if_symbols.hpp"

namespace symbols
{
    struct Map
        : public symbols::Module
    {
        Map(fs::path path);

        // methods
        bool            setup   ();
        bool            set_aslr(const std::string& symbol, const uint64_t addr);
        opt<uint64_t>   get_aslr();

        // IModule methods
        span_t                  span        () override;
        opt<uint64_t>           symbol      (const std::string& symbol) override;
        opt<uint64_t>           struc_offset(const std::string& struc, const std::string& member) override;
        opt<size_t>             struc_size  (const std::string& struc) override;
        opt<symbols::Offset>    symbol      (uint64_t addr) override;
        bool                    sym_list    (symbols::on_symbol_fn on_sym) override;
    };

    std::shared_ptr<symbols::Map> make_map(span_t span, const std::string& module, const std::string& guid);

} // namespace symbols
