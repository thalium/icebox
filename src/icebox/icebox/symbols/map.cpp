#include "symbols.hpp"

#define FDP_MODULE "map"
#include "indexer.hpp"
#include "interfaces/if_symbols.hpp"
#include "log.hpp"

#include <fstream>
#include <sstream>

namespace
{
    bool setup(symbols::Indexer& indexer, const fs::path& filename)
    {
        auto filestream = std::ifstream(filename);
        if(!filestream)
            return FAIL(false, "unable to open %s", filename.generic_string().data());

        auto row    = std::string{};
        auto offset = uint64_t{};
        auto type   = char{};
        auto symbol = std::string{};
        while(std::getline(filestream, row))
        {
            auto ok = !!(std::istringstream{row} >> std::hex >> offset >> std::dec >> type >> symbol);
            if(!ok)
                return FAIL(false, "unable to parse row '%s' in file %s", row.data(), filename.generic_string().data());

            indexer.add_symbol(symbol, offset);
        }
        return true;
    }
}

std::shared_ptr<symbols::Module> symbols::make_map(const std::string& module, const std::string& guid)
{
    const auto path = getenv("_LINUX_SYMBOL_PATH");
    if(!path)
        return nullptr;

    auto indexer = symbols::make_indexer(guid);
    if(!indexer)
        return nullptr;

    const auto ok = setup(*indexer, fs::path(path) / module / guid / "System.map");
    if(!ok)
        return nullptr;

    indexer->finalize();
    return indexer;
}
