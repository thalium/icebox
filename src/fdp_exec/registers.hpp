#pragma once

#include "enums.hpp"
#include "types.hpp"

#include <memory>

namespace core
{
    struct Registers
    {
         Registers();
        ~Registers();

        uint64_t    read    (reg_e reg);
        bool        write   (reg_e reg, uint64_t value);
        uint64_t    read    (msr_e reg);
        bool        write   (msr_e reg, uint64_t value);

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace core
