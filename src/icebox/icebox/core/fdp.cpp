#include "fdp.hpp"

#define PRIVATE_CORE_
#define FDP_MODULE "fdp"
#include "core.hpp"
#include "core_private.hpp"
#include "log.hpp"

extern "C"
{
#include <FDP.h>
}

struct fdp::shm
{
    shm(FDP_SHM* ptr)
        : ptr(ptr)
        , is_running(true)
    {
    }

    ~shm()
    {
        FDP_Resume(ptr);
        FDP_ExitSHM(ptr);
    }

    FDP_SHM* ptr;
    bool     is_running;
};

std::shared_ptr<fdp::shm> fdp::setup(const std::string& name)
{
    const auto ptr = FDP_OpenSHM(name.data());
    if(!ptr)
        return nullptr;

    const auto ok = FDP_Init(ptr);
    if(!ok)
        return nullptr;

    return std::make_shared<fdp::shm>(ptr);
}

namespace
{
    void check_vm(core::Core& core, const char* where)
    {
        if(!core.shm_)
            return;

        if(!core.shm_->is_running)
            return;

        LOG(ERROR, "%s called on is_running vm", where);
    }
}

void fdp::reset(core::Core& core)
{
    fdp::pause(core);

    auto ptr = core.shm_->ptr;
    check_vm(core, "fdp::reset");
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
    // accept is_running FDP_GetState calls
    auto       value = FDP_State{};
    const auto ok    = FDP_GetState(core.shm_->ptr, &value);
    if(!ok)
        return {};

    return value;
}

bool fdp::state_changed(core::Core& core)
{
    const auto ret = FDP_GetStateChanged(core.shm_->ptr);
    if(!ret)
        return false;

    const auto opt_state = fdp::state(core);
    if(!opt_state)
        return false;

    core.shm_->is_running = !(*opt_state & FDP_STATE_PAUSED);
    return true;
}

bool fdp::pause(core::Core& core)
{
    const auto ret        = FDP_Pause(core.shm_->ptr);
    core.shm_->is_running = !ret;
    return ret;
}

bool fdp::resume(core::Core& core)
{
    const auto ret        = FDP_Resume(core.shm_->ptr);
    core.shm_->is_running = ret;
    return ret;
}

bool fdp::step_once(core::Core& core)
{
    check_vm(core, "fdp::step_once");
    return FDP_SingleStep(core.shm_->ptr, 0);
}

bool fdp::unset_breakpoint(core::Core& core, int bpid)
{
    check_vm(core, "fdp::unset_breakpoint");
    return FDP_UnsetBreakpoint(core.shm_->ptr, bpid);
}

int fdp::set_breakpoint(core::Core& core, FDP_BreakpointType type, int bpid, FDP_Access access, FDP_AddressType ptrtype, uint64_t ptr, uint64_t len, uint64_t cr3)
{
    check_vm(core, "fdp::set_breakpoint");
    return FDP_SetBreakpoint(core.shm_->ptr, 0, type, bpid, access, ptrtype, ptr, len, cr3);
}

bool fdp::read_physical(core::Core& core, void* vdst, phy_t src, size_t size)
{
    check_vm(core, "fdp::read_physical");
    const auto dst   = reinterpret_cast<uint8_t*>(vdst);
    const auto usize = static_cast<uint32_t>(size);
    return FDP_ReadPhysicalMemory(core.shm_->ptr, dst, usize, src.val);
}

bool fdp::write_physical(core::Core& core, phy_t dst, const void* vsrc, size_t size)
{
    check_vm(core, "fdp::write_physical");
    const auto src   = reinterpret_cast<uint8_t*>(const_cast<void*>(vsrc));
    const auto usize = static_cast<uint32_t>(size);
    return FDP_WritePhysicalMemory(core.shm_->ptr, src, usize, dst.val);
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
        check_vm(core, "fdp::read_virtual");
        return FDP_ReadVirtualMemory(core.shm_->ptr, 0, dst, usize, src);
    });
}

bool fdp::write_virtual(core::Core& core, uint64_t dst, dtb_t dtb, const void* vsrc, size_t size)
{
    const auto src   = reinterpret_cast<uint8_t*>(const_cast<void*>(vsrc));
    const auto usize = static_cast<uint32_t>(size);
    return switch_dtb(core, dtb, [&]
    {
        check_vm(core, "fdp::write_virtual");
        return FDP_WriteVirtualMemory(core.shm_->ptr, 0, src, usize, dst);
    });
}

opt<phy_t> fdp::virtual_to_physical(core::Core& core, dtb_t dtb, uint64_t ptr)
{
    uint64_t   phy = 0;
    const auto ok  = switch_dtb(core, dtb, [&]
    {
        check_vm(core, "fdp::virtual_to_physical");
        return FDP_VirtualToPhysical(core.shm_->ptr, 0, ptr, &phy);
    });
    if(!ok)
        return {};

    return phy_t{phy};
}

bool fdp::inject_interrupt(core::Core& core, uint32_t code, uint32_t error, uint64_t cr2)
{
    check_vm(core, "fdp::inject_interrupt");
    return FDP_InjectInterrupt(core.shm_->ptr, 0, code, error, cr2);
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
    check_vm(core, "fdp::read_register");
    auto       value = uint64_t{};
    const auto ok    = FDP_ReadRegister(core.shm_->ptr, 0, cast(reg), &value);
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
    check_vm(core, "fdp::read_msr_register");
    auto       value = uint64_t{};
    const auto ok    = FDP_ReadMsr(core.shm_->ptr, 0, cast(msr), &value);
    if(!ok)
        return {};

    return value;
}

bool fdp::write_register(core::Core& core, reg_e reg, uint64_t value)
{
    check_vm(core, "fdp::write_register");
    return FDP_WriteRegister(core.shm_->ptr, 0, cast(reg), value);
}

bool fdp::write_msr_register(core::Core& core, msr_e msr, uint64_t value)
{
    check_vm(core, "fdp::write_msr_register");
    return FDP_WriteMsr(core.shm_->ptr, 0, cast(msr), value);
}

bool fdp::save(core::Core& core)
{
    check_vm(core, "fdp::save");
    return FDP_Save(core.shm_->ptr);
}

bool fdp::restore(core::Core& core)
{
    check_vm(core, "fdp::restore");
    return FDP_Restore(core.shm_->ptr);
}
