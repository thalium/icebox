#pragma once

#include "types.hpp"

#include <memory>

namespace core { struct Core; }
namespace pe { struct Pe; }

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

    struct SyscallPluginWow64
    {
         SyscallPluginWow64(core::Core& core, pe::Pe& pe);
        ~SyscallPluginWow64();

        bool    setup   (proc_t target);
        bool    generate(const fs::path& file_name);

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace syscall_tracer
