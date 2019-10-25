#include "map.hpp"

#define FDP_MODULE "map"
#include "log.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>

namespace
{
    fs::path                     filename;
    std::vector<symbols::Offset> cursors_by_address;
    std::vector<symbols::Offset> cursors_by_name;
    opt<uint64_t>                aslr;
}

symbols::Map::Map(fs::path path)
{
    filename = std::move(path);
}

namespace
{
    const auto ModCursor_order_address = [](const symbols::Offset& a, const symbols::Offset& b)
    {
        return a.offset < b.offset;
    };

    const auto ModCursor_order_name = [](const symbols::Offset& a, const symbols::Offset& b)
    {
        return a.symbol < b.symbol;
    };
}

bool symbols::Map::setup()
{
    cursors_by_address.clear();
    cursors_by_name.clear();
    aslr = {};

    std::ifstream filestream;

    filestream = std::ifstream(filename.generic_string().data());
    if(!filestream)
        return FAIL(false, "unable to open %s", filename.generic_string().data());

    std::string     row;
    std::string     str_offset;
    symbols::Offset cursor;
    char            type;

    while(std::getline(filestream, row))
    {
        if(!(std::istringstream(row) >> str_offset >> type >> cursor.symbol))
            return FAIL(false, "unable to parse row '%s' in file %s", row.data(), filename.generic_string().data());

        std::istringstream iss_offset(str_offset);
        iss_offset >> std::hex;
        if(!(iss_offset >> cursor.offset))
            return FAIL(false, "unable to parse hex '%s' in file %s", str_offset.data(), filename.generic_string().data());

        cursors_by_address.push_back(cursor);
        cursors_by_name.push_back(cursor);
    }

    filestream.close();

    std::sort(cursors_by_address.begin(), cursors_by_address.end(), ModCursor_order_address);
    std::sort(cursors_by_name.begin(), cursors_by_name.end(), ModCursor_order_name);

    return true;
}

bool symbols::Map::set_aslr(const std::string& strSymbol, const uint64_t addr)
{
    aslr                  = 0;
    const auto addrSymbol = symbol(strSymbol);
    if(!addrSymbol)
    {
        aslr = {};
        return FAIL(false, "unable to find symbol %s", strSymbol.data());
    }

    aslr = addr - *addrSymbol;
    return true;
}

opt<uint64_t> symbols::Map::get_aslr()
{
    return aslr;
}

std::unique_ptr<symbols::Map> symbols::make_map(span_t, const std::string& module, const std::string& guid)
{
    const auto path = getenv("_LINUX_SYMBOL_PATH");
    if(!path)
        return nullptr;

    auto ptr = std::make_unique<Map>(fs::path(path) / module / guid / "System.map");
    if(!ptr->setup())
        return nullptr;

    return ptr;
}

span_t symbols::Map::span()
{
    return {};
}

namespace
{
    bool check_setup()
    {
        if(cursors_by_address.empty() | cursors_by_name.empty())
            return FAIL(false, "map parser has not been set up");

        if(!aslr)
            return FAIL(false, "Symbol address is required whereas ASLR was not yet setted");

        return true;
    }
}

opt<uint64_t> symbols::Map::symbol(const std::string& symbol)
{
    if(!check_setup())
        return {};

    Offset target;
    target.symbol = symbol;

    const auto itrCursor = std::equal_range(cursors_by_name.begin(), cursors_by_name.end(), target, ModCursor_order_name);

    if(itrCursor.first == cursors_by_name.end() || itrCursor.first == itrCursor.second)
        return {};

    if(itrCursor.first != (itrCursor.second - 1))
        LOG(WARNING, "The symbol %s has been found multiple times", symbol.data());

    return itrCursor.first->offset + *aslr;
}

bool symbols::Map::sym_list(symbols::on_symbol_fn on_sym)
{
    if(!check_setup())
        return false;

    for(const auto& cursor : cursors_by_address)
        if(on_sym(cursor.symbol, cursor.offset + *aslr) == WALK_STOP)
            return true;

    return true;
}

opt<uint64_t> symbols::Map::struc_offset(const std::string&, const std::string&)
{
    return {};
}

opt<size_t> symbols::Map::struc_size(const std::string&)
{
    return {};
}

opt<symbols::Offset> symbols::Map::symbol(uint64_t addr)
{
    if(!check_setup())
        return {};

    Offset target;
    target.offset            = addr - *aslr;
    const auto first_address = cursors_by_address.begin()->offset, last_address = (cursors_by_address.end() - 1)->offset;

    if(target.offset < first_address || target.offset > last_address)
        return {};

    Offset cursor;
    if(target.offset != last_address)
    {
        const auto itrCursor = std::upper_bound(cursors_by_address.begin(), cursors_by_address.end(), target, ModCursor_order_address);
        cursor               = *(itrCursor - 1);
    }
    else
        cursor = *(cursors_by_address.end() - 1);

    cursor.offset += *aslr;
    return cursor;
}
