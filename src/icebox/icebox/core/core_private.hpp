#pragma once

#include "core.hpp"

#ifndef PRIVATE_CORE__
#    error do not include this header directly
#endif

namespace fdp { struct shm; }

namespace memory
{
    struct Memory;
    std::shared_ptr<Memory> setup(core::Core& core);
} // namespace memory

namespace core
{
    using Memory = std::shared_ptr<memory::Memory>;

    struct Core::Data
    {
        Data(std::string_view name);

        const std::string name_;
        fdp::shm*         shm_;
        Memory            mem_;
    };
} // namespace core
