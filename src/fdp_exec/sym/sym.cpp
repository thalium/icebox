#include "sym.hpp"

#include <unordered_map>

namespace
{
    using Modules = std::unordered_map<std::string, std::unique_ptr<sym::IModule>>;

    struct Handler
        : public sym::IHandler
    {
        // IHandler methods
        bool            register_module     (const std::string& name, std::unique_ptr<sym::IModule>& module) override;
        bool            unregister_module   (const std::string& name) override;
        bool            list_modules        (const on_module_fn& on_module) override;
        sym::IModule*   get_module          (const std::string& name) override;
        opt<uint64_t>   get_symbol          (const std::string& mod, const std::string& symbol) override;
        opt<uint64_t>   get_struc_offset    (const std::string& mod, const std::string& struc, const std::string& member) override;

        Modules mods_;
    };
}

std::unique_ptr<sym::IHandler> sym::make_sym()
{
    return std::make_unique<Handler>();
}

bool Handler::register_module(const std::string& name, std::unique_ptr<sym::IModule>& module)
{
    const auto ret = mods_.emplace(name, std::move(module));
    return ret.second;
}

bool Handler::unregister_module(const std::string& name)
{
    const auto ret = mods_.erase(name);
    return !!ret;
}

bool Handler::list_modules(const on_module_fn& on_module)
{
    for(const auto& m : mods_)
        if(on_module(*m.second) == WALK_STOP)
            break;
    return true;
}

sym::IModule* Handler::get_module(const std::string& name)
{
    const auto it = mods_.find(name);
    if(it == mods_.end())
        return nullptr;

    return it->second.get();
}

opt<uint64_t> Handler::get_symbol(const std::string& module, const std::string& symbol)
{
    const auto mod = get_module(module);
    if(!mod)
        return std::nullopt;

    return mod->get_symbol(symbol);
}

opt<uint64_t> Handler::get_struc_offset(const std::string& module, const std::string& struc, const std::string& member)
{
    const auto mod = get_module(module);
    if(!mod)
        return std::nullopt;

    return mod->get_struc_offset(struc, member);
}
