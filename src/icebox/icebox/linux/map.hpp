#pragma once

#include "interfaces/if_symbols.hpp"

namespace symbols
{
    struct Map
        : public symbols::Module
    {
        Map(fs::path path);

        // methods
        bool setup();

        // IModule methods
        opt<size_t>             symbol      (const std::string& symbol) override;
        opt<size_t>             struc_offset(const std::string& struc, const std::string& member) override;
        opt<size_t>             struc_size  (const std::string& struc) override;
        opt<symbols::Offset>    symbol      (size_t offset) override;
        bool                    sym_list    (symbols::on_symbol_fn on_sym) override;
    };

    std::shared_ptr<symbols::Map> make_map(const std::string& module, const std::string& guid);

} // namespace symbols
