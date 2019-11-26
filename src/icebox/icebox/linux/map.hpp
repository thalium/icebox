#pragma once

#include "interfaces/if_symbols.hpp"

namespace symbols
{
    std::shared_ptr<symbols::Module> make_map(const std::string& module, const std::string& guid);

} // namespace symbols
