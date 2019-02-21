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
         FdpSan(core::Core& core);
        ~FdpSan();

        bool setup(proc_t target);

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace plugins
