#include "map.hpp"

#define FDP_MODULE "map"
#include "log.hpp"
#include "utils/fnview.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>

namespace
{
    fs::path                                        filename;
    std::vector<sym::ModCursor> cursors_by_address, cursors_by_name;

    opt<uint64_t> aslr;
}

sym::Map::Map(fs::path path)
{
    filename = std::move(path);
}

namespace
{
    const auto ModCursor_order_address = [](const sym::ModCursor& a, const sym::ModCursor& b)
    {
        return a.offset < b.offset;
    };

    const auto ModCursor_order_name = [](const sym::ModCursor& a, const sym::ModCursor& b)
    {
        return a.symbol < b.symbol;
    };
}

bool sym::Map::setup()
{
    cursors_by_address.clear();
    cursors_by_name.clear();
    aslr = {};

    std::ifstream filestream;

    filestream = std::ifstream(filename.generic_string().data());
    if(!filestream)
        return FAIL(false, "unable to open {}", filename.generic_string());

    std::string    row;
    std::string    str_offset;
    sym::ModCursor cursor;
    char           type;

    while(std::getline(filestream, row))
    {
        if(!(std::istringstream(row) >> str_offset >> type >> cursor.symbol))
            return FAIL(false, "unable to parse row '{}' in file {}", row, filename.generic_string());

        std::istringstream iss_offset(str_offset);
        iss_offset >> std::hex;
        if(!(iss_offset >> cursor.offset))
            return FAIL(false, "unable to parse hex '{}' in file {}", str_offset, filename.generic_string());

        cursors_by_address.push_back(cursor);
        cursors_by_name.push_back(cursor);
    }

    filestream.close();

    std::sort(cursors_by_address.begin(), cursors_by_address.end(), ModCursor_order_address);
    std::sort(cursors_by_name.begin(), cursors_by_name.end(), ModCursor_order_name);

    return true;
}

bool sym::Map::set_aslr(const std::string& strSymbol, const uint64_t addr)
{
    aslr                  = 0;
    const auto addrSymbol = symbol(strSymbol);
    if(!addrSymbol)
    {
        aslr = {};
        return FAIL(false, "unable to find symbol {}", strSymbol);
    }

    aslr = addr - *addrSymbol;
    return true;
}

opt<uint64_t> sym::Map::get_aslr()
{
    return aslr;
}

std::unique_ptr<sym::Map> sym::make_map(span_t, const std::string& module, const std::string& guid)
{
    const auto path = getenv("_LINUX_SYMBOL_PATH");
    if(!path)
        return nullptr;

    auto ptr = std::make_unique<Map>(fs::path(path) / module / guid / "System.map");
    if(!ptr->setup())
        return nullptr;

    return ptr;
}

span_t sym::Map::span()
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

opt<uint64_t> sym::Map::symbol(const std::string& symbol)
{
    if(!check_setup())
        return {};

    ModCursor target;
    target.symbol = symbol;

    const auto itrCursor = std::equal_range(cursors_by_name.begin(), cursors_by_name.end(), target, ModCursor_order_name);

    if(itrCursor.first == cursors_by_name.end() || itrCursor.first == itrCursor.second)
        return {};

    if(itrCursor.first != (itrCursor.second - 1))
        LOG(WARNING, "The symbol {} has been found multiple times", symbol);

    return itrCursor.first->offset + *aslr;
}

bool sym::Map::sym_list(sym::on_sym_fn on_sym)
{
    if(!check_setup())
        return false;

    for(const auto& cursor : cursors_by_address)
        if(on_sym(cursor.symbol, cursor.offset + *aslr) == WALK_STOP)
            return true;

    return true;
}

opt<uint64_t> sym::Map::struc_offset(const std::string&, const std::string&)
{
    return {};
}

opt<size_t> sym::Map::struc_size(const std::string&)
{
    return {};
}

opt<sym::ModCursor> sym::Map::symbol(uint64_t addr)
{
    if(!check_setup())
        return {};

    ModCursor target;
    target.offset            = addr - *aslr;
    const auto first_address = cursors_by_address.begin()->offset, last_address = (cursors_by_address.end() - 1)->offset;

    if(target.offset < first_address || target.offset > last_address)
        return {};

    ModCursor cursor;
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
