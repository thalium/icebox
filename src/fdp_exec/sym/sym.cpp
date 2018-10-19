#include "sym.hpp"

#include <unordered_map>

namespace
{
    using Modules = std::unordered_map<std::string, std::unique_ptr<sym::IMod>>;
}

namespace sym
{
    struct Symbols::Data
    {
        Modules mods_;
    };
}

sym::Symbols::Symbols()
    : d_(std::make_unique<Data>())
{
}

sym::Symbols::~Symbols()
{
}

bool sym::Symbols::insert(const std::string& name, std::unique_ptr<sym::IMod>& module)
{
    const auto ret = d_->mods_.emplace(name, std::move(module));
    return ret.second;
}

namespace
{
    static const char pdb[] = "pdb";
    static const struct
    {
        std::unique_ptr<sym::IMod>(*make)(span_t, const void*);
        const std::string name;
    } g_helpers[] =
    {
        {&sym::make_pdb, pdb},
    };
}

bool sym::Symbols::insert(const std::string& name, span_t module, const void* data)
{
    for(const auto& h : g_helpers)
    {
        auto mod = h.make(module, data);
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

bool sym::Symbols::list(const on_module_fn& on_module)
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
        return std::nullopt;

    return mod->symbol(symbol);
}

opt<uint64_t> sym::Symbols::struc_offset(const std::string& module, const std::string& struc, const std::string& member)
{
    const auto mod = find(module);
    if(!mod)
        return std::nullopt;

    return mod->struc_offset(struc, member);
}
