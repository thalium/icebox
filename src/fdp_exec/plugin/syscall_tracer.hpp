#pragma once

#include "core.hpp"
#include "utils/pe.hpp"

#include "callstack.hpp"
#include "monitor/generic_mon.hpp"
#include "nt/objects_nt.hpp"

#include <memory>

namespace syscall_tracer
{
    struct SyscallPlugin
    {
         SyscallPlugin(core::Core& core, pe::Pe& pe);
        ~SyscallPlugin();

        bool    setup   (proc_t target);
        bool    generate(const fs::path& file_name);

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace syscall_tracer
