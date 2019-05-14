#include "map.hpp"

#define FDP_MODULE "map"
#include "log.hpp"
#include "utils/fnview.hpp"

#include <fstream>
#include <sstream>

struct sym::Map::Data
{
    // members
    fs::path      filename;
    std::ifstream filestream;
    bool settedup = false;
};

sym::Map::Map(fs::path filename)
    : d_(std::make_unique<Data>())
{
    d_->filename = std::move(filename);
}

sym::Map::~Map()
{
    d_->filestream.close();
}

bool sym::Map::setup()
{
    auto& d = *d_;

    d.filestream = std::ifstream(d.filename.generic_string().data());
    if(!d.filestream)
        return FAIL(false, "unable to open {}", d.filename.generic_string());

    d.settedup = true;
    return true;
}

std::unique_ptr<sym::IMod> sym::make_map(span_t /*span*/, const std::string& module, const std::string& guid)
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
    auto& d = *d_;
    if(!d.settedup)
        return FAIL(false, "map parser has not been set up");

    std::string    row;
    std::string    str_offset;
    sym::ModCursor cursor;
    char           type;

    d.filestream.clear();
    d.filestream.seekg(0, std::ios::beg);
    while(std::getline(d.filestream, row))
    {
        if(!(std::istringstream(row) >> str_offset >> type >> cursor.symbol))
            return FAIL(false, "unable to parse row '{}' in file {}", row, d.filename.generic_string());

        std::istringstream iss_offset(str_offset);
        iss_offset >> std::hex;
        if(!(iss_offset >> cursor.offset))
            return FAIL(false, "unable to parse hex '{}' in file {}", str_offset, d.filename.generic_string());

        const auto ret = on_sym(cursor.symbol, cursor.offset);
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
