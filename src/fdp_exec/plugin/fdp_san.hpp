#pragma once

#include "types.hpp"
#include "core.hpp"
#include "utils/pe.hpp"

#include "callstack.hpp"
#include "monitor/monitor.hpp"

#include <memory>

namespace plugin
{
    struct FdpSan
    {
         FdpSan(core::Core& core, pe::Pe& pe);
        ~FdpSan();

        bool setup(proc_t target);

        struct Data;
        std::unique_ptr<Data> d_;
    };
}
