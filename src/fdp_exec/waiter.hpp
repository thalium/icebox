#pragma once

#include "core.hpp"

#include <functional>
#include <memory>

namespace core
{
    struct Waiter
    {
         Waiter(Core& core);
        ~Waiter();

        using task_proc = std::function<void(proc_t)>;
        using task_mod  = std::function<void(proc_t, const std::string&, span_t)>;

        void    proc_wait   (const std::string& proc_name, const task_proc& task);
        void    mod_wait    (const std::string& mod_name, bool wow64, const task_mod& task);
        void    mod_wait    (const std::string& proc_name, const std::string& mod_name, bool wow64, const task_mod& task);
        void    mod_wait    (proc_t proc, const std::string& mod_name, bool wow64, const task_mod& task);

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace core
