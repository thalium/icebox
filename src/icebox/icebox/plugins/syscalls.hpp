#pragma once

#include "icebox/types.hpp"

#include <memory>

namespace core { struct Core; }
namespace sym { struct Symbols; }

namespace plugins
{
    struct Syscalls
    {
         Syscalls(core::Core& core, sym::Symbols& syms, proc_t proc);
        ~Syscalls();

        bool generate(const fs::path& file_name);

        struct Data;
        std::unique_ptr<Data> d_;
    };

    struct Syscalls32
    {
         Syscalls32(core::Core& core, sym::Symbols& syms, proc_t proc);
        ~Syscalls32();

        bool generate(const fs::path& file_name);

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace plugins
