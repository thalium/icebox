#include "sym.hpp"

#include "icebox/utils/fnview.hpp"
#include "icebox/utils/hex.hpp"

#include <unordered_map>

#ifdef _MSC_VER
#    include <algorithm>
#    include <functional>
#    define search                          std::search
#    define boyer_moore_horspool_searcher   std::boyer_moore_horspool_searcher
#else
#    include <experimental/algorithm>
#    include <experimental/functional>
#    define search                          std::experimental::search
#    define boyer_moore_horspool_searcher   std::experimental::make_boyer_moore_horspool_searcher
#endif

namespace
{
    using Modules = std::unordered_map<std::string, std::unique_ptr<sym::IMod>>;
}

struct sym::Symbols::Data
{
    Modules mods_;
};

using SymData = sym::Symbols::Data;

sym::Symbols::Symbols()
    : d_(std::make_unique<Data>())
{
}

sym::Symbols::~Symbols() = default;

bool sym::Symbols::insert(const std::string& name, std::unique_ptr<sym::IMod>& module)
{
    const auto ret = d_->mods_.emplace(name, std::move(module));
    return ret.second;
}

namespace
{
    static const char pdb[]   = "pdb";
    static const char dwarf[] = "dwarf";
    static const char empty[] = "empty";
    static const struct
    {
        std::unique_ptr<sym::IMod> (*make)(span_t, const void*, const size_t);
        const std::string name;
    } g_helpers[] =
    {
            {&sym::make_pdb, pdb},
            {&sym::make_dwarf, dwarf},
            {&sym::make_empty, empty},
    };
}

bool sym::Symbols::insert(const std::string& name, span_t module, const void* data, const size_t data_size)
{
    for(const auto& h : g_helpers)
    {
        auto mod = h.make(module, data, data_size);
        if(!mod)
            continue;

        return insert(name, mod);
    }
    return false;
}

bool sym::Symbols::remove(const std::string& name)
{
    const auto ret = d_->mods_.erase(name);
    return !!ret;
}

bool sym::Symbols::list(on_module_fn on_module)
{
    for(const auto& m : d_->mods_)
        if(on_module(*m.second) == WALK_STOP)
            break;
    return true;
}

sym::IMod* sym::Symbols::find(const std::string& name)
{
    const auto it = d_->mods_.find(name);
    if(it == d_->mods_.end())
        return nullptr;

    return it->second.get();
}

opt<uint64_t> sym::Symbols::symbol(const std::string& module, const std::string& symbol)
{
    const auto mod = find(module);
    if(!mod)
        return {};

    return mod->symbol(symbol);
}

opt<uint64_t> sym::Symbols::struc_offset(const std::string& module, const std::string& struc, const std::string& member)
{
    const auto mod = find(module);
    if(!mod)
        return {};

    return mod->struc_offset(struc, member);
}

opt<size_t> sym::Symbols::struc_size(const std::string& module, const std::string& struc)
{
    const auto mod = find(module);
    if(!mod)
        return {};

    return mod->struc_size(struc);
}

namespace
{
    struct ModPair
    {
        std::string name;
        sym::IMod&  mod;
    };

    static opt<ModPair> find(SymData& s, uint64_t addr)
    {
        for(const auto& m : s.mods_)
        {
            const auto span = m.second->span();
            if(span.addr <= addr && addr < span.addr + span.size)
                return ModPair{m.first, *m.second};
        }

        return {};
    }
}

opt<sym::Cursor> sym::Symbols::find(uint64_t addr)
{
    auto p = ::find(*d_, addr);
    if(!p)
        return {};

    const auto cur = p->mod.symbol(addr);
    if(!cur)
        return {};

    return Cursor{p->name.data(), cur->symbol, cur->offset};
}

std::string sym::to_string(const sym::Cursor& cursor)
{
    char dst[2 + 8 * 2 + 1];
    const char* offset = hex::convert<hex::RemovePadding | hex::HexaPrefix>(dst, cursor.offset);
    return cursor.module + '!' + cursor.symbol + '+' + offset;
}
