#pragma once

#include "types.hpp"

#include <memory>

namespace core { struct Core; }

namespace plugins
{
    struct Syscalls
    {
         Syscalls(core::Core& core);
        ~Syscalls();

        bool    setup   (proc_t target);
        bool    generate(const fs::path& file_name);

        struct Data;
        std::unique_ptr<Data> d_;
    };

    struct Syscalls32
    {
         Syscalls32(core::Core& core);
        ~Syscalls32();

        bool    setup   (proc_t target);
        bool    generate(const fs::path& file_name);

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace plugins
