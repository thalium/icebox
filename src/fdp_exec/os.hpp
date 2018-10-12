#pragma once

#include "enums.hpp"
#include "types.hpp"

#include <functional>
#include <memory>

struct ICore;

namespace os
{
    struct IHelper
    {
        virtual ~IHelper() = default;

        virtual std::string_view name() const = 0;

        using on_proc_fn = std::function<walk_e(proc_t)>;
        using on_mod_fn = std::function<walk_e(mod_t)>;

        virtual bool                list_procs(const on_proc_fn& on_proc) = 0;
        virtual opt<proc_t>         get_current_proc() = 0;
        virtual opt<proc_t>         get_proc(const std::string& name) = 0;
        virtual opt<std::string>    get_proc_name(proc_t proc) = 0;
        virtual bool                list_mods(proc_t proc, const on_mod_fn& on_mod) = 0;
        virtual opt<std::string>    get_mod_name(proc_t proc, mod_t mod) = 0;
        virtual bool                has_virtual(proc_t proc) = 0;
    };

    std::unique_ptr<IHelper> make_nt(ICore& core);

    struct Helper
    {
        std::unique_ptr<IHelper>(*make)(ICore& core);
        const std::string         name;
    };
    static const Helper helpers[] =
    {
        {&make_nt, "windows_nt"},
    };
}
