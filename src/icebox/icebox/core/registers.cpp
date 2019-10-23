#include "registers.hpp"

#define PRIVATE_CORE__
#include "fdp.hpp"

uint64_t registers::read(core::Core& core, reg_e reg)
{
    const auto ret = fdp::read_register(core, reg);
    return ret ? *ret : 0;
}

bool registers::write(core::Core& core, reg_e reg, uint64_t value)
{
    return fdp::write_register(core, reg, value);
}

uint64_t registers::read_msr(core::Core& core, msr_e reg)
{
    const auto ret = fdp::read_msr_register(core, reg);
    return ret ? *ret : 0;
}

bool registers::write_msr(core::Core& core, msr_e reg, uint64_t value)
{
    return fdp::write_msr_register(core, reg, value);
}
