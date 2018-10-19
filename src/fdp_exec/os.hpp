#pragma once

#include "enums.hpp"
#include "types.hpp"

#include <functional>
#include <memory>

namespace core { struct Core; }

namespace os
{
    struct IModule
    {
        virtual ~IModule() = default;

        using on_proc_fn    = std::function<walk_e(proc_t)>;
        using on_mod_fn     = std::function<walk_e(mod_t)>;

        virtual bool                proc_list       (const on_proc_fn& on_proc) = 0;
        virtual opt<proc_t>         proc_current    () = 0;
        virtual opt<proc_t>         proc_find       (const std::string& name) = 0;
        virtual opt<std::string>    proc_name       (proc_t proc) = 0;
        virtual bool                proc_is_valid   (proc_t proc) = 0;
        virtual bool                mod_list        (proc_t proc, const on_mod_fn& on_mod) = 0;
        virtual opt<std::string>    mod_name        (proc_t proc, mod_t mod) = 0;
        virtual opt<span_t>         mod_span        (proc_t proc, mod_t mod) = 0;
    };

    std::unique_ptr<IModule> make_nt(core::Core& core);
    static const char windows_nt[] = "windows_nt";
    static const struct
    {
        std::unique_ptr<IModule>(*make)(core::Core& core);
        const std::string         name;
    } g_helpers[] =
    {
        {&make_nt, windows_nt},
    };
}
