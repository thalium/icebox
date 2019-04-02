#pragma once

#include "types.hpp"

#include <memory>

namespace core
{
    struct Core;
};

namespace plugins
{
    struct FdpSan
    {
         FdpSan(core::Core& core, proc_t target);
        ~FdpSan();

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace plugins
