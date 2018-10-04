#pragma once

#include "enums.hpp"

#include <functional>
#include <memory>
#include <string_view>

struct ICore;

namespace os
{
    using proc_t = uint64_t;

    struct IHelper
    {
        virtual ~IHelper() = default;

        virtual std::string_view name() const = 0;

        using on_process_fn = std::function<walk_e(proc_t)>;
        virtual bool                list_procs(const on_process_fn& on_process) = 0;
        virtual proc_t              get_current_proc() = 0;
        virtual proc_t              get_proc(const std::string& name) = 0;
        virtual std::string_view    get_name(proc_t proc) = 0;
    };

    std::unique_ptr<IHelper> make_nt(ICore& core);

    struct Helper
    {
        std::unique_ptr<IHelper> (*make)(ICore& core);
        const std::string         name;
    };
    static const Helper helpers[] =
    {
        {&make_nt, "windows_nt"},
    };
}
