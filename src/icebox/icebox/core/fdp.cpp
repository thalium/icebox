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
    fdp::shm* cast(FDP_SHM* shm)
    {
        return reinterpret_cast<fdp::shm*>(shm);
    }

    FDP_SHM* cast(fdp::shm* shm)
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
    auto ptr = cast(core.shm_);
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
    const auto ok   = FDP_GetState(cast(core.shm_), &value);
    if(!ok)
        return {};

    return value;
}

bool fdp::state_changed(core::Core& core)
{
    return FDP_GetStateChanged(cast(core.shm_));
}

bool fdp::pause(core::Core& core)
{
    return FDP_Pause(cast(core.shm_));
}

bool fdp::resume(core::Core& core)
{
    return FDP_Resume(cast(core.shm_));
}

bool fdp::step_once(core::Core& core)
{
    return FDP_SingleStep(cast(core.shm_), 0);
}

bool fdp::unset_breakpoint(core::Core& core, int bpid)
{
    return FDP_UnsetBreakpoint(cast(core.shm_), bpid);
}

int fdp::set_breakpoint(core::Core& core, FDP_BreakpointType type, int bpid, FDP_Access access, FDP_AddressType ptrtype, uint64_t ptr, uint64_t len, uint64_t cr3)
{
    return FDP_SetBreakpoint(cast(core.shm_), 0, type, bpid, access, ptrtype, ptr, len, cr3);
}

bool fdp::read_physical(core::Core& core, void* vdst, phy_t src, size_t size)
{
    const auto dst   = reinterpret_cast<uint8_t*>(vdst);
    const auto usize = static_cast<uint32_t>(size);
    return FDP_ReadPhysicalMemory(cast(core.shm_), dst, usize, src.val);
}

bool fdp::write_physical(core::Core& core, phy_t dst, const void* vsrc, size_t size)
{
    const auto src   = reinterpret_cast<uint8_t*>(const_cast<void*>(vsrc));
    const auto usize = static_cast<uint32_t>(size);
    return FDP_WritePhysicalMemory(cast(core.shm_), src, usize, dst.val);
}

namespace
{
    template <typename T>
    auto switch_dtb(core::Core& core, dtb_t dtb, T operand)
    {
        const auto backup      = fdp::read_register(core, reg_e::cr3);
        const auto need_switch = backup && *backup != dtb.val;
        if(need_switch)
            fdp::write_register(core, reg_e::cr3, dtb.val);
        const auto ret = operand();
        if(need_switch)
            fdp::write_register(core, reg_e::cr3, *backup);
        return ret;
    }
}

bool fdp::read_virtual(core::Core& core, void* vdst, uint64_t src, dtb_t dtb, size_t size)
{
    const auto dst   = reinterpret_cast<uint8_t*>(vdst);
    const auto usize = static_cast<uint32_t>(size);
    return switch_dtb(core, dtb, [&]
    {
        return FDP_ReadVirtualMemory(cast(core.shm_), 0, dst, usize, src);
    });
}

bool fdp::write_virtual(core::Core& core, uint64_t dst, dtb_t dtb, const void* vsrc, size_t size)
{
    const auto src   = reinterpret_cast<uint8_t*>(const_cast<void*>(vsrc));
    const auto usize = static_cast<uint32_t>(size);
    return switch_dtb(core, dtb, [&]
    {
        return FDP_WriteVirtualMemory(cast(core.shm_), 0, src, usize, dst);
    });
}

opt<phy_t> fdp::virtual_to_physical(core::Core& core, dtb_t dtb, uint64_t ptr)
{
    uint64_t phy  = 0;
    const auto ok = switch_dtb(core, dtb, [&]
    {
        return FDP_VirtualToPhysical(cast(core.shm_), 0, ptr, &phy);
    });
    if(!ok)
        return {};

    return phy_t{phy};
}

bool fdp::inject_interrupt(core::Core& core, uint32_t code, uint32_t error, uint64_t cr2)
{
    return FDP_InjectInterrupt(cast(core.shm_), 0, code, error, cr2);
}

namespace
{
    FDP_Register cast(reg_e arg)
    {
        switch(arg)
        {
            case reg_e::cs:     return FDP_CS_REGISTER;
            case reg_e::rsp:    return FDP_RSP_REGISTER;
            case reg_e::rip:    return FDP_RIP_REGISTER;
            case reg_e::rbp:    return FDP_RBP_REGISTER;
            case reg_e::cr3:    return FDP_CR3_REGISTER;
            case reg_e::cr8:    return FDP_CR8_REGISTER;
            case reg_e::rax:    return FDP_RAX_REGISTER;
            case reg_e::rbx:    return FDP_RBX_REGISTER;
            case reg_e::rcx:    return FDP_RCX_REGISTER;
            case reg_e::rdx:    return FDP_RDX_REGISTER;
            case reg_e::rsi:    return FDP_RSI_REGISTER;
            case reg_e::rdi:    return FDP_RDI_REGISTER;
            case reg_e::r8:     return FDP_R8_REGISTER;
            case reg_e::r9:     return FDP_R9_REGISTER;
            case reg_e::r10:    return FDP_R10_REGISTER;
            case reg_e::r11:    return FDP_R11_REGISTER;
            case reg_e::r12:    return FDP_R12_REGISTER;
            case reg_e::r13:    return FDP_R13_REGISTER;
            case reg_e::r14:    return FDP_R14_REGISTER;
            case reg_e::r15:    return FDP_R15_REGISTER;
        }
        return FDP_REGISTER_UINT16_TRICK;
    }
}

opt<uint64_t> fdp::read_register(core::Core& core, reg_e reg)
{
    uint64_t value = 0;
    const auto ok  = FDP_ReadRegister(cast(core.shm_), 0, cast(reg), &value);
    if(!ok)
        return {};

    return value;
}

namespace
{
    uint64_t cast(msr_e reg)
    {
        switch(reg)
        {
            case msr_e::lstar:          return 0xC0000082;
            case msr_e::fs_base:        return 0xC0000100;
            case msr_e::gs_base:        return 0xC0000101;
            case msr_e::kernel_gs_base: return 0xC0000102;
        }
        return ~0ULL;
    }
}

opt<uint64_t> fdp::read_msr_register(core::Core& core, msr_e msr)
{
    uint64_t value = 0;
    const auto ok  = FDP_ReadMsr(cast(core.shm_), 0, cast(msr), &value);
    if(!ok)
        return {};

    return value;
}

bool fdp::write_register(core::Core& core, reg_e reg, uint64_t value)
{
    return FDP_WriteRegister(cast(core.shm_), 0, cast(reg), value);
}

bool fdp::write_msr_register(core::Core& core, msr_e msr, uint64_t value)
{
    return FDP_WriteMsr(cast(core.shm_), 0, cast(msr), value);
}

bool fdp::save(core::Core& core)
{
    return FDP_Save(cast(core.shm_));
}

bool fdp::restore(core::Core& core)
{
    return FDP_Restore(cast(core.shm_));
}
