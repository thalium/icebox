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

std::shared_ptr<symbols::Map> symbols::make_map(const std::string& module, const std::string& guid)
{
    const auto path = getenv("_LINUX_SYMBOL_PATH");
    if(!path)
        return nullptr;

    const auto ptr = std::make_shared<Map>(fs::path(path) / module / guid / "System.map");
    if(!ptr->setup())
        return nullptr;

    return ptr;
}

namespace
{
    bool check_setup()
    {
        if(cursors_by_address.empty() | cursors_by_name.empty())
            return FAIL(false, "map parser has not been set up");

        return true;
    }
}

opt<size_t> symbols::Map::symbol(const std::string& symbol)
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

    return itrCursor.first->offset;
}

bool symbols::Map::sym_list(symbols::on_symbol_fn on_sym)
{
    if(!check_setup())
        return false;

    for(const auto& cursor : cursors_by_address)
        if(on_sym(cursor.symbol, cursor.offset) == walk_e::stop)
            return true;

    return true;
}

opt<size_t> symbols::Map::struc_offset(const std::string&, const std::string&)
{
    return {};
}

opt<size_t> symbols::Map::struc_size(const std::string&)
{
    return {};
}

opt<symbols::Offset> symbols::Map::symbol(size_t offset)
{
    if(!check_setup())
        return {};

    Offset target;
    target.offset            = offset;
    const auto first_address = cursors_by_address.front().offset;
    const auto last_address  = cursors_by_address.back().offset;

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

    return cursor;
}
