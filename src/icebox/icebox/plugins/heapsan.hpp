#pragma once

#include "icebox/types.hpp"

#include <memory>

namespace core { struct Core; }

namespace plugins
{
    struct HeapSan
    {
        HeapSan(core::Core& core, proc_t target);
        ~HeapSan();

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace plugins
