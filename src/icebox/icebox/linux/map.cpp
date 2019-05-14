#include "map.hpp"

#define FDP_MODULE "map"
#include "log.hpp"
#include "utils/fnview.hpp"

#include <fstream>
#include <sstream>

namespace
{
    fs::path      filename;
    std::ifstream filestream;
    bool settedup = false;

    uint64_t aslr = 0;
    bool is_aslr  = false;
}

sym::Map::Map(fs::path path)
{
    filename = std::move(path);
}

sym::Map::~Map()
{
    filestream.close();
}

bool sym::Map::setup()
{
    filestream = std::ifstream(filename.generic_string().data());
    if(!filestream)
        return FAIL(false, "unable to open {}", filename.generic_string());

    settedup = true;
    return true;
}

bool sym::Map::set_aslr(const std::string& strSymbol, const uint64_t addr)
{
    is_aslr               = true;
    const auto addrSymbol = symbol(strSymbol);
    if(!addrSymbol)
    {
        is_aslr = false;
        return FAIL(false, "unable to find symbol {}", strSymbol);
    }

    aslr = addr - *addrSymbol;
    return true;
}

opt<uint64_t> sym::Map::get_aslr()
{
    if(is_aslr)
        return aslr;
    else
        return {};
}

std::unique_ptr<sym::Map> sym::make_map(span_t /*span*/, const std::string& module, const std::string& guid)
{
    const auto path = getenv("_LINUX_SYMBOL_PATH");
    if(!path)
        return nullptr;

    auto ptr = std::make_unique<sym::Map>(fs::path(path) / module / guid / "System.map");
    if(!ptr->setup())
        return nullptr;

    return ptr;
}

span_t sym::Map::span()
{
    return {};
}

opt<uint64_t> sym::Map::symbol(const std::string& symbol)
{
    opt<uint64_t> symbol_offset = {};

    sym_list([&](std::string name, uint64_t offset)
    {
        if(name == symbol)
        {
            symbol_offset = offset;
            return WALK_STOP;
        }
        return WALK_NEXT;
    });

    return symbol_offset;
}

bool sym::Map::sym_list(sym::on_sym_fn on_sym)
{
    if(!settedup)
        return FAIL(false, "map parser has not been set up");

    if(!is_aslr)
        LOG(WARNING, "Symbol address is required whereas ASLR was not setted");

    std::string    row;
    std::string    str_offset;
    sym::ModCursor cursor;
    char           type;

    filestream.clear();
    filestream.seekg(0, std::ios::beg);
    while(std::getline(filestream, row))
    {
        if(!(std::istringstream(row) >> str_offset >> type >> cursor.symbol))
            return FAIL(false, "unable to parse row '{}' in file {}", row, filename.generic_string());

        std::istringstream iss_offset(str_offset);
        iss_offset >> std::hex;
        if(!(iss_offset >> cursor.offset))
            return FAIL(false, "unable to parse hex '{}' in file {}", str_offset, filename.generic_string());

        const auto ret = on_sym(cursor.symbol, cursor.offset + aslr);
        if(ret == WALK_STOP)
            return true;
    }

    return true;
}

opt<uint64_t> sym::Map::struc_offset(const std::string& /*struc*/, const std::string& /*member*/)
{
    return {};
}

opt<size_t> sym::Map::struc_size(const std::string& /*struc*/)
{
    return {};
}

opt<sym::ModCursor> sym::Map::symbol(uint64_t addr)
{
    opt<sym::ModCursor> cursor = {};

    sym_list([&](std::string name, uint64_t offset)
    {
        if(offset == addr)
        {
            cursor         = sym::ModCursor();
            cursor->symbol = name;
            cursor->offset = offset;
            return WALK_STOP;
        }
        return WALK_NEXT;
    });

    return cursor;
}
