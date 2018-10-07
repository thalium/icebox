#include "core.hpp"

#include "os.hpp"
#include "FDP.h"

#define FDP_MODULE "core"
#include "log.hpp"

#include <vector>

// custom deleters
namespace std
{
    template<> struct default_delete<FDP_SHM> { static const bool marker = true; void operator()(FDP_SHM* /*ptr*/) {} }; // FIXME no deleter
}

namespace
{
    static const int CPU_ID = 0;

    template<typename T>
    std::unique_ptr<T> make_unique(T* ptr)
    {
        // check whether we correctly defined a custom deleter
        static_assert(std::default_delete<T>::marker == true, "missing custom marker");
        return std::unique_ptr<T>(ptr);
    }

    struct Core
        : public ICore
    {
        Core(const std::string_view& name);

        bool setup();

        // ICore methods
        os::IHelper&            os      () override;
        bool                    pause   () override;
        bool                    resume  () override;

        std::optional<uint64_t> read_msr    (msr_e reg) override;
        bool                    write_msr   (msr_e reg, uint64_t value) override;
        std::optional<uint64_t> read_reg    (reg_e reg) override;
        bool                    write_reg   (reg_e reg, uint64_t value) override;
        bool                    read_mem    (void* dst, uint64_t src, size_t size) override;
        bool                    write_mem   (uint64_t dst, const void* src, size_t size) override;

        const std::string               name_;
        std::unique_ptr<FDP_SHM>        shm_;
        std::unique_ptr<os::IHelper>    os_;
    };
}

Core::Core(const std::string_view& name)
    : name_(name)
{
}

std::unique_ptr<ICore> make_core(const std::string_view& name)
{
    auto core = std::make_unique<Core>(name);
    const auto ok = core->setup();
    if(!ok)
        return std::nullptr_t();

    return core;
}

bool Core::setup()
{
    auto ptr_shm = FDP_OpenSHM(name_.data());
    if(!ptr_shm)
        FAIL(false, "unable to open shm");

    shm_ = make_unique(ptr_shm);
    auto ok = FDP_Init(ptr_shm);
    if(!ok)
        FAIL(false, "unable to init shm");

    // register os helpers
    for(const auto& h : os::helpers)
    {
        os_ = h.make(*this);
        if(os_)
            break;
    }
    if(!os_)
        return false;

    return true;
}

os::IHelper& Core::os()
{
    return *os_;
}

bool Core::pause()
{
    const auto ok = FDP_Pause(&*shm_);
    if(!ok)
    {
        LOG(ERROR, "unable to pause");
        return false;
    }

    return true;
}

bool Core::resume()
{
    const auto ok = FDP_Resume(&*shm_);
    if(!ok)
    {
        LOG(ERROR, "unable to resume");
        return false;
    }

    return true;
}

bool Core::read_mem(void* dst, uint64_t src, size_t size)
{
    const auto ok = FDP_ReadVirtualMemory(&*shm_, CPU_ID, reinterpret_cast<uint8_t*>(dst), static_cast<uint32_t>(size), src);
    if(!ok)
    {
        LOG(ERROR, "unable to read memory 0x%llx-0x%llx %zd byte(s)", src, src + size, size);
        memset(dst, 0, size);
        return false;
    }

    return true;
}

bool Core::write_mem(uint64_t dst, const void* src, size_t size)
{
    const auto ok = FDP_WriteVirtualMemory(&*shm_, CPU_ID, reinterpret_cast<uint8_t*>(const_cast<void*>(src)), static_cast<uint32_t>(size), dst);
    if(!ok)
    {
        LOG(ERROR, "unable to write memory 0x%llx-0x%llx %zd byte(s)", dst, dst + size, size);
        return false;
    }

    return true;
}

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

std::optional<uint64_t> Core::read_msr(msr_e reg)
{
    uint64_t value = 0;
    const auto ok = FDP_ReadMsr(&*shm_, CPU_ID, reg, &value);
    if(!ok)
        FAIL(std::nullopt, "unable to read msr %s 0x%x", msr_to_str(reg), reg);

    return value;
}

bool Core::write_msr(msr_e reg, uint64_t value)
{
    const auto ok = FDP_WriteMsr(&*shm_, CPU_ID, reg, value);
    if(!ok)
        FAIL(false, "unable to write 0x%llx to msr %s 0x%x", value, msr_to_str(reg), reg);

    return true;
}

std::optional<uint64_t> Core::read_reg(reg_e reg)
{
    uint64_t value;
    const auto ok = FDP_ReadRegister(&*shm_, CPU_ID, reg, &value);
    if(!ok)
        FAIL(std::nullopt, "unable to read reg %s 0x%x", reg_to_str(reg), reg);

    return value;
}

bool Core::write_reg(reg_e reg, uint64_t value)
{
    const auto ok = FDP_WriteRegister(&*shm_, CPU_ID, reg, value);
    if(!ok)
        FAIL(false, "unable to write 0x%llx to reg %s 0x%x", value, reg_to_str(reg), reg);

    return true;
}
