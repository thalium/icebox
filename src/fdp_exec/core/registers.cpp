#include "registers.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "reg"
#include "core.hpp"
#include "log.hpp"
#include "private.hpp"

#include <FDP.h>

struct core::Registers::Data
{
    Data(FDP_SHM& shm)
        : shm_(shm)
    {
    }

    FDP_SHM& shm_;
};

core::Registers::Registers()
{
}

core::Registers::~Registers()
{
}

void core::setup(Registers& regs, FDP_SHM& shm)
{
    regs.d_ = std::make_unique<core::Registers::Data>(shm);
}

uint64_t core::Registers::read(reg_e reg)
{
    uint64_t value;
    const auto ok = FDP_ReadRegister(&d_->shm_, 0, reg, &value);
    if(!ok)
        return 0;

    return value;
}

bool core::Registers::write(reg_e reg, uint64_t value)
{
    const auto ok = FDP_WriteRegister(&d_->shm_, 0, reg, value);
    return ok;
}

uint64_t core::Registers::read(msr_e reg)
{
    uint64_t value = 0;
    const auto ok  = FDP_ReadMsr(&d_->shm_, 0, reg, &value);
    if(!ok)
        return 0;

    return value;
}

bool core::Registers::write(msr_e reg, uint64_t value)
{
    const auto ok = FDP_WriteMsr(&d_->shm_, 0, reg, value);
    return ok;
}
