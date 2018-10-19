#pragma once

#include "enums.hpp"
#include "types.hpp"

#include <functional>
#include <memory>

namespace core { struct Core; }

namespace os
{
    struct IHandler
    {
        virtual ~IHandler() = default;

        using on_proc_fn    = std::function<walk_e(proc_t)>;
        using on_mod_fn     = std::function<walk_e(mod_t)>;

        virtual bool                list_procs      (const on_proc_fn& on_proc) = 0;
        virtual opt<proc_t>         get_current_proc() = 0;
        virtual opt<proc_t>         get_proc        (const std::string& name) = 0;
        virtual opt<std::string>    get_proc_name   (proc_t proc) = 0;
        virtual bool                list_mods       (proc_t proc, const on_mod_fn& on_mod) = 0;
        virtual opt<std::string>    get_mod_name    (proc_t proc, mod_t mod) = 0;
        virtual opt<span_t>         get_mod_span    (proc_t proc, mod_t mod) = 0;
        virtual bool                has_virtual     (proc_t proc) = 0;
    };

    std::unique_ptr<IHandler> make_nt(core::Core& core);
    static const char windows_nt[] = "windows_nt";
    static const struct
    {
        std::unique_ptr<IHandler>(*make)(core::Core& core);
        const std::string         name;
    } g_helpers[] =
    {
        {&make_nt, windows_nt},
    };
}
