#include "fdp.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "fdp"
#include "core.hpp"
#include "core_private.hpp"
#include "log.hpp"

extern "C"
{
#include <FDP.h>
}

namespace
{
    static fdp::shm* cast(FDP_SHM* shm)
    {
        return reinterpret_cast<fdp::shm*>(shm);
    }

    static FDP_SHM* cast(fdp::shm* shm)
    {
        return reinterpret_cast<FDP_SHM*>(shm);
    }
}

fdp::shm* fdp::setup(const std::string& name)
{
    const auto ptr = FDP_OpenSHM(name.data());
    if(!ptr)
        return nullptr;

    const auto ok = FDP_Init(ptr);
    if(!ok)
        return nullptr;

    return cast(ptr);
}

void fdp::reset(core::Core& core)
{
    auto ptr = cast(core.d_->shm_);
    FDP_Pause(ptr);

    for(int bpid = 0; bpid < FDP_MAX_BREAKPOINT; bpid++)
        FDP_UnsetBreakpoint(ptr, bpid);

    FDP_WriteRegister(ptr, 0, FDP_DR0_REGISTER, 0);
    FDP_WriteRegister(ptr, 0, FDP_DR1_REGISTER, 0);
    FDP_WriteRegister(ptr, 0, FDP_DR2_REGISTER, 0);
    FDP_WriteRegister(ptr, 0, FDP_DR3_REGISTER, 0);
    FDP_WriteRegister(ptr, 0, FDP_DR6_REGISTER, 0);
    FDP_WriteRegister(ptr, 0, FDP_DR7_REGISTER, 0);
}

opt<FDP_State> fdp::state(core::Core& core)
{
    FDP_State value = 0;
    const auto ok   = FDP_GetState(cast(core.d_->shm_), &value);
    if(!ok)
        return {};

    return value;
}

bool fdp::state_changed(core::Core& core)
{
    return FDP_GetStateChanged(cast(core.d_->shm_));
}

bool fdp::pause(core::Core& core)
{
    return FDP_Pause(cast(core.d_->shm_));
}

bool fdp::resume(core::Core& core)
{
    return FDP_Resume(cast(core.d_->shm_));
}

bool fdp::step_once(core::Core& core)
{
    return FDP_SingleStep(cast(core.d_->shm_), 0);
}

bool fdp::unset_breakpoint(core::Core& core, int bpid)
{
    return FDP_UnsetBreakpoint(cast(core.d_->shm_), bpid);
}

int fdp::set_breakpoint(core::Core& core, FDP_BreakpointType type, int bpid, FDP_Access access, FDP_AddressType ptrtype, uint64_t ptr, uint64_t len, uint64_t cr3)
{
    return FDP_SetBreakpoint(cast(core.d_->shm_), 0, type, bpid, access, ptrtype, ptr, len, cr3);
}

bool fdp::read_physical(core::Core& core, void* vdst, size_t size, phy_t phy)
{
    const auto dst   = reinterpret_cast<uint8_t*>(vdst);
    const auto usize = static_cast<uint32_t>(size);
    return FDP_ReadPhysicalMemory(cast(core.d_->shm_), dst, usize, phy.val);
}

namespace
{
    template <typename T>
    static auto switch_dtb(core::Core& core, dtb_t dtb, T operand)
    {
        const auto backup      = fdp::read_register(core, FDP_CR3_REGISTER);
        const auto need_switch = backup && *backup != dtb.val;
        if(need_switch)
            fdp::write_register(core, FDP_CR3_REGISTER, dtb.val);
        const auto ret = operand();
        if(need_switch)
            fdp::write_register(core, FDP_CR3_REGISTER, *backup);
        return ret;
    }
}

bool fdp::read_virtual(core::Core& core, void* vdst, size_t size, dtb_t dtb, uint64_t ptr)
{
    const auto dst   = reinterpret_cast<uint8_t*>(vdst);
    const auto usize = static_cast<uint32_t>(size);
    return switch_dtb(core, dtb, [&]
    {
        return FDP_ReadVirtualMemory(cast(core.d_->shm_), 0, dst, usize, ptr);
    });
}

opt<phy_t> fdp::virtual_to_physical(core::Core& core, dtb_t dtb, uint64_t ptr)
{
    uint64_t phy  = 0;
    const auto ok = switch_dtb(core, dtb, [&]
    {
        return FDP_VirtualToPhysical(cast(core.d_->shm_), 0, ptr, &phy);
    });
    if(!ok)
        return {};

    return phy_t{phy};
}

bool fdp::inject_interrupt(core::Core& core, uint32_t code, uint32_t error, uint64_t cr2)
{
    return FDP_InjectInterrupt(cast(core.d_->shm_), 0, code, error, cr2);
}

opt<uint64_t> fdp::read_register(core::Core& core, reg_e reg)
{
    uint64_t value = 0;
    const auto ok  = FDP_ReadRegister(cast(core.d_->shm_), 0, reg, &value);
    if(!ok)
        return {};

    return value;
}

opt<uint64_t> fdp::read_msr_register(core::Core& core, msr_e msr)
{
    uint64_t value = 0;
    const auto ok  = FDP_ReadMsr(cast(core.d_->shm_), 0, msr, &value);
    if(!ok)
        return {};

    return value;
}

bool fdp::write_register(core::Core& core, reg_e reg, uint64_t value)
{
    return FDP_WriteRegister(cast(core.d_->shm_), 0, reg, value);
}

bool fdp::write_msr_register(core::Core& core, msr_e msr, uint64_t value)
{
    return FDP_WriteMsr(cast(core.d_->shm_), 0, msr, value);
}
