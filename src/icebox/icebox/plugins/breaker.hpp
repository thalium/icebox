#pragma once

#include "icebox/types.hpp"

#include <functional>
#include <memory>

namespace core { struct Core; }

namespace state
{
    using Task = std::function<void(void)>;

    struct Breaker
    {
         Breaker(core::Core& core, proc_t target);
        ~Breaker();

        bool break_return(std::string_view name, const Task& task);

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace state
