#include "registers.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "reg"
#include "log.hpp"
#include "core.hpp"
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

namespace
{
    const char* msr_to_str(msr_e reg)
    {
        switch(reg)
        {
            case MSR_LSTAR:             return "lstar";
            case MSR_GS_BASE:           return "gs_base";
            case MSR_KERNEL_GS_BASE:    return "kernel_gs_base";
        }
        return "unknown";
    }

    const char* reg_to_str(reg_e reg)
    {
        switch(reg)
        {
            case FDP_RAX_REGISTER:          return "rax";
            case FDP_RBX_REGISTER:          return "rbx";
            case FDP_RCX_REGISTER:          return "rcx";
            case FDP_RDX_REGISTER:          return "rdx";
            case FDP_R8_REGISTER:           return "r8";
            case FDP_R9_REGISTER:           return "r9";
            case FDP_R10_REGISTER:          return "r10";
            case FDP_R11_REGISTER:          return "r11";
            case FDP_R12_REGISTER:          return "r12";
            case FDP_R13_REGISTER:          return "r13";
            case FDP_R14_REGISTER:          return "r14";
            case FDP_R15_REGISTER:          return "r15";
            case FDP_RSP_REGISTER:          return "rsp";
            case FDP_RBP_REGISTER:          return "rbp";
            case FDP_RSI_REGISTER:          return "rsi";
            case FDP_RDI_REGISTER:          return "rdi";
            case FDP_RIP_REGISTER:          return "rip";
            case FDP_DR0_REGISTER:          return "dr0";
            case FDP_DR1_REGISTER:          return "dr1";
            case FDP_DR2_REGISTER:          return "dr2";
            case FDP_DR3_REGISTER:          return "dr3";
            case FDP_DR6_REGISTER:          return "dr6";
            case FDP_DR7_REGISTER:          return "dr7";
            case FDP_VDR0_REGISTER:         return "vdr0";
            case FDP_VDR1_REGISTER:         return "vdr1";
            case FDP_VDR2_REGISTER:         return "vdr2";
            case FDP_VDR3_REGISTER:         return "vdr3";
            case FDP_VDR6_REGISTER:         return "vdr6";
            case FDP_VDR7_REGISTER:         return "vdr7";
            case FDP_CS_REGISTER:           return "cs";
            case FDP_DS_REGISTER:           return "ds";
            case FDP_ES_REGISTER:           return "es";
            case FDP_FS_REGISTER:           return "fs";
            case FDP_GS_REGISTER:           return "gs";
            case FDP_SS_REGISTER:           return "ss";
            case FDP_RFLAGS_REGISTER:       return "rflags";
            case FDP_MXCSR_REGISTER:        return "mxcsr";
            case FDP_GDTRB_REGISTER:        return "gdtrb";
            case FDP_GDTRL_REGISTER:        return "gdtrl";
            case FDP_IDTRB_REGISTER:        return "idtrb";
            case FDP_IDTRL_REGISTER:        return "idtrl";
            case FDP_CR0_REGISTER:          return "cr0";
            case FDP_CR2_REGISTER:          return "cr2";
            case FDP_CR3_REGISTER:          return "cr3";
            case FDP_CR4_REGISTER:          return "cr4";
            case FDP_CR8_REGISTER:          return "cr8";
            case FDP_LDTR_REGISTER:         return "ldtr";
            case FDP_LDTRB_REGISTER:        return "ldtrb";
            case FDP_LDTRL_REGISTER:        return "ldtrl";
            case FDP_TR_REGISTER:           return "tr";
            case FDP_REGISTER_UINT16_TRICK: return "trick";
        }
        return "unknown";
    }
}

opt<uint64_t> core::Registers::read(reg_e reg)
{
    uint64_t value;
    const auto ok = FDP_ReadRegister(&d_->shm_, 0, reg, &value);
    if(!ok)
        FAIL(exp::nullopt, "unable to read reg %s 0x%x", reg_to_str(reg), reg);

    return value;
}

bool core::Registers::write(reg_e reg, uint64_t value)
{
    const auto ok = FDP_WriteRegister(&d_->shm_, 0, reg, value);
    if(!ok)
        FAIL(false, "unable to write 0x%" PRIx64 " to reg %s 0x%x", value, reg_to_str(reg), reg);

    return true;
}

opt<uint64_t> core::Registers::read(msr_e reg)
{
    uint64_t value = 0;
    const auto ok = FDP_ReadMsr(&d_->shm_, 0, reg, &value);
    if(!ok)
        FAIL(exp::nullopt, "unable to read msr %s 0x%x", msr_to_str(reg), reg);

    return value;
}

bool core::Registers::write(msr_e reg, uint64_t value)
{
    const auto ok = FDP_WriteMsr(&d_->shm_, 0, reg, value);
    if(!ok)
        FAIL(false, "unable to write 0x%" PRIx64 " to msr %s 0x%x", value, msr_to_str(reg), reg);

    return true;
}
