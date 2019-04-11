#pragma once

#include "icebox/types.hpp"

#include <memory>

namespace core { struct Core; }
namespace sym { struct Symbols; }

namespace plugins
{
    struct FdpSan
    {
         FdpSan(core::Core& core, sym::Symbols& syms, proc_t target);
        ~FdpSan();

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace plugins
