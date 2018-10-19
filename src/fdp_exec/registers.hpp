#pragma once

#include "types.hpp"
#include "enums.hpp"

#include <memory>

namespace core
{
    struct Registers
    {
         Registers();
        ~Registers();

        opt<uint64_t>   read    (reg_e reg);
        bool            write   (reg_e reg, uint64_t value);
        opt<uint64_t>   read    (msr_e reg);
        bool            write   (msr_e reg, uint64_t value);

        struct Data;
        std::unique_ptr<Data> d_;
    };
}
