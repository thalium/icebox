#pragma once

#include "enums.hpp"
#include "types.hpp"

namespace core { struct Core; }

namespace registers
{
    uint64_t    read        (core::Core& core, reg_e reg);
    bool        write       (core::Core& core, reg_e reg, uint64_t value);
    uint64_t    read_msr    (core::Core& core, msr_e reg);
    bool        write_msr   (core::Core& core, msr_e reg, uint64_t value);
}; // namespace registers
