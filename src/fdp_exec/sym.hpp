#pragma once

#include "enums.hpp"
#include "types.hpp"

#include <functional>
#include <memory>
#include <string>

namespace sym
{
    struct IModule
    {
        virtual ~IModule() = default;

        virtual span_t        get_span          () = 0;
        virtual opt<uint64_t> get_symbol        (const std::string& symbol) = 0;
        virtual opt<uint64_t> get_struc_offset  (const std::string& struc, const std::string& member) = 0;
    };

    std::unique_ptr<IModule> make_pdb(span_t span, const std::string& module, const std::string& guid);
    std::unique_ptr<IModule> make_pdb(span_t span, const void* data);

    struct IHandler
    {
        virtual ~IHandler() = default;

        using on_module_fn = std::function<walk_e(const IModule& module)>;

        virtual bool            register_module     (const std::string& name, std::unique_ptr<IModule>& module) = 0;
        virtual bool            register_module     (const std::string& name, span_t module, const void* data) = 0;
        virtual bool            unregister_module   (const std::string& name) = 0;
        virtual bool            list_modules        (const on_module_fn& on_module) = 0;
        virtual IModule*        get_module          (const std::string& name) = 0;
        virtual opt<uint64_t>   get_symbol          (const std::string& module, const std::string& symbol) = 0;
        virtual opt<uint64_t>   get_struc_offset    (const std::string& module, const std::string& struc, const std::string& member) = 0;
    };

    std::unique_ptr<IHandler> make_sym();
}
