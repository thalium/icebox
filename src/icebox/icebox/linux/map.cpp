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
    struct Map
        : public symbols::Module
    {
        Map(fs::path path, std::string guid);

        // methods
        bool setup();

        // IModule methods
        std::string_view        id              () override;
        opt<size_t>             symbol_offset   (const std::string& symbol) override;
        void                    struc_names     (const symbols::on_name_fn& on_struc) override;
        opt<size_t>             struc_size      (const std::string& struc) override;
        void                    struc_members   (const std::string& struc, const symbols::on_name_fn& on_member) override;
        opt<size_t>             member_offset   (const std::string& struc, const std::string& member) override;
        opt<symbols::Offset>    find_symbol     (size_t offset) override;
        bool                    list_symbols    (symbols::on_symbol_fn on_sym) override;

        const fs::path               filename;
        const std::string            guid;
        std::vector<symbols::Offset> cursors_by_address;
        std::vector<symbols::Offset> cursors_by_name;
    };

}

Map::Map(fs::path path, std::string guid)
    : filename(std::move(path))
    , guid(std::move(guid))
{
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

bool Map::setup()
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

std::shared_ptr<symbols::Module> symbols::make_map(const std::string& module, const std::string& guid)
{
    const auto path = getenv("_LINUX_SYMBOL_PATH");
    if(!path)
        return nullptr;

    const auto ptr = std::make_shared<Map>(fs::path(path) / module / guid / "System.map", guid);
    if(!ptr->setup())
        return nullptr;

    return ptr;
}

std::string_view Map::id()
{
    return guid;
}

opt<size_t> Map::symbol_offset(const std::string& symbol)
{
    auto target          = symbols::Offset{};
    target.symbol        = symbol;
    const auto itrCursor = std::equal_range(cursors_by_name.begin(), cursors_by_name.end(), target, ModCursor_order_name);

    if(itrCursor.first == cursors_by_name.end() || itrCursor.first == itrCursor.second)
        return {};

    if(itrCursor.first != (itrCursor.second - 1))
        LOG(WARNING, "The symbol %s has been found multiple times", symbol.data());

    return itrCursor.first->offset;
}

bool Map::list_symbols(symbols::on_symbol_fn on_sym)
{
    for(const auto& cursor : cursors_by_address)
        if(on_sym(cursor.symbol, cursor.offset) == walk_e::stop)
            return true;

    return true;
}

void Map::struc_names(const symbols::on_name_fn& /*on_struc*/)
{
}

opt<size_t> Map::struc_size(const std::string& /*struc*/)
{
    return {};
}

void Map::struc_members(const std::string& /*struc*/, const symbols::on_name_fn& /*on_member*/)
{
}

opt<size_t> Map::member_offset(const std::string& /*struc*/, const std::string& /*member*/)
{
    return {};
}

opt<symbols::Offset> Map::find_symbol(size_t offset)
{
    auto target              = symbols::Offset{};
    target.offset            = offset;
    const auto first_address = cursors_by_address.front().offset;
    const auto last_address  = cursors_by_address.back().offset;

    if(target.offset < first_address || target.offset > last_address)
        return {};

    auto cursor = symbols::Offset{};
    if(target.offset != last_address)
    {
        const auto itrCursor = std::upper_bound(cursors_by_address.begin(), cursors_by_address.end(), target, ModCursor_order_address);
        cursor               = *(itrCursor - 1);
    }
    else
        cursor = *(cursors_by_address.end() - 1);

    return cursor;
}
