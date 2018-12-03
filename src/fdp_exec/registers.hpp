#pragma once

#include "enums.hpp"
#include "errors.hpp"
#include "types.hpp"

#include <memory>

namespace core
{
    struct Registers
    {
         Registers();
        ~Registers();

        uint64_t    read    (reg_e reg);
        status_t    write   (reg_e reg, uint64_t value);
        uint64_t    read    (msr_e reg);
        status_t    write   (msr_e reg, uint64_t value);

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace core
