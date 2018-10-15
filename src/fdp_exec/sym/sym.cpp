#include "sym.hpp"

#include <unordered_map>

namespace impl // cannot use anonymous namespace due to visual studio bug
{
    using Modules = std::unordered_map<std::string, std::unique_ptr<sym::IModule>>;

    struct Handler
        : public sym::IHandler
    {
        // IHandler methods
        bool            register_module     (const std::string& name, std::unique_ptr<sym::IModule>& module) override;
        bool            unregister_module   (const std::string& name) override;
        bool            list_modules        (const on_module_fn& on_module) override;
        opt<uint64_t>   get_symbol          (const std::string& mod, const std::string& symbol) override;
        opt<uint64_t>   get_struc_offset    (const std::string& mod, const std::string& struc, const std::string& member) override;

        Modules mods_;
    };
}

std::unique_ptr<sym::IHandler> sym::make_sym()
{
    return std::make_unique<impl::Handler>();
}

bool impl::Handler::register_module(const std::string& name, std::unique_ptr<sym::IModule>& module)
{
    const auto ret = mods_.emplace(name, std::move(module));
    return ret.second;
}

bool impl::Handler::unregister_module(const std::string& name)
{
    const auto ret = mods_.erase(name);
    return !!ret;
}

bool impl::Handler::list_modules(const on_module_fn& on_module)
{
    for(const auto& m : mods_)
        if(on_module(*m.second) == WALK_STOP)
            break;
    return true;
}

opt<uint64_t> impl::Handler::get_symbol(const std::string& module, const std::string& symbol)
{
    const auto it = mods_.find(module);
    if(it == mods_.end())
        return std::nullopt;

    return it->second->get_symbol(symbol);
}

opt<uint64_t> impl::Handler::get_struc_offset(const std::string& module, const std::string& struc, const std::string& member)
{
    const auto it = mods_.find(module);
    if(it == mods_.end())
        return std::nullopt;

    return it->second->get_struc_offset(struc, member);
}