#include "registers.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "reg"
#include "core.hpp"
#include "fdp.hpp"
#include "log.hpp"
#include "private.hpp"

struct core::Registers::Data
{
    Data(fdp::shm& shm)
        : shm_(shm)
    {
    }

    fdp::shm& shm_;
};

core::Registers::Registers()  = default;
core::Registers::~Registers() = default;

void core::setup(Registers& regs, fdp::shm& shm)
{
    regs.d_ = std::make_unique<core::Registers::Data>(shm);
}

uint64_t core::Registers::read(reg_e reg)
{
    const auto ret = fdp::read_register(d_->shm_, reg);
    return ret ? *ret : 0;
}

bool core::Registers::write(reg_e reg, uint64_t value)
{
    return fdp::write_register(d_->shm_, reg, value);
}

uint64_t core::Registers::read(msr_e reg)
{
    const auto ret = fdp::read_msr_register(d_->shm_, reg);
    return ret ? *ret : 0;
}

bool core::Registers::write(msr_e reg, uint64_t value)
{
    return fdp::write_msr_register(d_->shm_, reg, value);
}
