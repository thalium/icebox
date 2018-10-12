#include "core.hpp"

#include "FDP.h"

#define FDP_MODULE "core"
#include "os.hpp"
#include "utils.hpp"
#include "log.hpp"
#include "endian.hpp"
#include "mmu.hpp"

#include <algorithm>
#include <thread>

// custom deleters
namespace std
{
    template<> struct default_delete<FDP_SHM> { static const bool marker = true; void operator()(FDP_SHM* /*ptr*/) {} }; // FIXME no deleter
}

struct ICore::Ctx
{
    Ctx(opt<proc_t>& core, proc_t current)
        : backup_(core)
        , current_(core)
    {
        current_ = current;
    }

    ~Ctx()
    {
        current_ = backup_;
    }

    opt<proc_t>     backup_;
    opt<proc_t>&    current_;
};

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
}

namespace impl // we need to specify a namespace name due to an msvc bug on 15.8.7
{
    struct Core
        : public ICore
    {
        Core(const std::string_view& name);

        bool setup();

        // ICore methods
        os::IHelper&            os          () override;
        bool                    pause       () override;
        bool                    resume      () override;
        bool                    wait_break  ();

        std::optional<uint64_t> read_msr        (msr_e reg) override;
        bool                    write_msr       (msr_e reg, uint64_t value) override;
        std::optional<uint64_t> read_reg        (reg_e reg) override;
        bool                    write_reg       (reg_e reg, uint64_t value) override;
        bool                    read_mem        (void* dst, uint64_t src, size_t size) override;
        bool                    write_mem       (uint64_t dst, const void* src, size_t size) override;

        std::shared_ptr<ICore::Ctx> switch_context(proc_t proc) override;

        const std::string               name_;
        std::unique_ptr<FDP_SHM>        shm_;
        std::unique_ptr<os::IHelper>    os_;

        std::optional<proc_t>           current_;
        std::optional<proc_t>           ctx_;
    };
}

impl::Core::Core(const std::string_view& name)
    : name_(name)
{
}

std::unique_ptr<ICore> make_core(const std::string_view& name)
{
    auto core = std::make_unique<impl::Core>(name);
    const auto ok = core->setup();
    if(!ok)
        return std::nullptr_t();

    return core;
}

bool impl::Core::setup()
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

os::IHelper& impl::Core::os()
{
    return *os_;
}

bool impl::Core::pause()
{
    const auto ok = FDP_Pause(&*shm_);
    if(!ok)
    {
        LOG(ERROR, "unable to pause");
        return false;
    }

    current_ = os_->get_current_proc();
    return true;
}

bool impl::Core::resume()
{
    const auto ok = FDP_Resume(&*shm_);
    if(!ok)
    {
        LOG(ERROR, "unable to resume");
        return false;
    }

    return true;
}

bool impl::Core::wait_break()
{
    while(true)
    {
        //std::this_thread::yield();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        FDP_State state = FDP_STATE_NULL;
        auto ok = FDP_GetStateChanged(&*shm_);
        if(!ok)
            continue;

        ok = FDP_GetState(&*shm_, &state);
        if(ok && (state & FDP_STATE_BREAKPOINT_HIT))
            return true;
    }
}

namespace
{
    uint64_t mask(int bits)
    {
        return ~(~uint64_t(0) << bits);
    }

    opt<uint64_t> virtual_to_physical(impl::Core& core, uint64_t ptr, uint64_t cr3)
    {
        const virt_t virt = {read_le64(&ptr)};
        const auto pml4e_base = cr3 & (mask(40) << 12);
        const auto pml4e_ptr = pml4e_base + virt.u.f.pml4 * 8;
        entry_t pml4e = {0};
        auto ok = FDP_ReadPhysicalMemory(&*core.shm_, reinterpret_cast<uint8_t*>(&pml4e), sizeof pml4e, pml4e_ptr);
        if(!ok)
            return std::nullopt;

        if(!pml4e.u.f.can_read)
            return std::nullopt;

        const auto pdpe_ptr = pml4e.u.f.page_frame_number * PAGE_SIZE + virt.u.f.pdp * 8;
        entry_t pdpe = {0};
        ok = FDP_ReadPhysicalMemory(&*core.shm_, reinterpret_cast<uint8_t*>(&pdpe), sizeof pdpe, pdpe_ptr);
        if(!ok)
            return std::nullopt;

        if(!pdpe.u.f.can_read)
            return std::nullopt;

        // 1g page
        if(pdpe.u.f.large_page)
        {
            const auto offset = ptr & mask(30);
            const auto phy = (pdpe.u.value & (mask(22) << 30)) + offset;
            return phy;
        }

        const auto pde_ptr = pdpe.u.f.page_frame_number * PAGE_SIZE + virt.u.f.pd * 8;
        entry_t pde = {0};
        ok = FDP_ReadPhysicalMemory(&*core.shm_, reinterpret_cast<uint8_t*>(&pde), sizeof pde, pde_ptr);
        if(!ok)
            return std::nullopt;

        if(!pde.u.f.can_read)
            return std::nullopt;

        // 2mb page
        if(pde.u.f.large_page)
        {
            const auto offset = ptr & mask(21);
            const auto phy = (pde.u.value & (mask(31) << 21)) + offset;
            return phy;
        }

        const auto pte_ptr = pde.u.f.page_frame_number * PAGE_SIZE + virt.u.f.pt * 8;
        entry_t pte = {0};
        ok = FDP_ReadPhysicalMemory(&*core.shm_, reinterpret_cast<uint8_t*>(&pte), sizeof pte, pte_ptr);
        if(!ok)
            return std::nullopt;

        // ignore can_read flag

        const auto phy = pte.u.f.page_frame_number * PAGE_SIZE + virt.u.f.offset;
        return phy;
    }
}

namespace
{
    bool raw_read_virtual(impl::Core& core, uint8_t* dst, uint64_t src, uint32_t size)
    {
        const auto ok = FDP_ReadVirtualMemory(&*core.shm_, CPU_ID, dst, size, src);
        if(!ok)
            FAIL(false, "unable to read mem 0x%llx-0x%llx (%u 0x%x bytes)", src, src + size, size, size);

        return true;
    }

    opt<uint64_t> try_page_fault(impl::Core& core, uint64_t ptr, uint64_t cr3_want)
    {
        auto ok = FDP_InjectInterrupt(&*core.shm_, CPU_ID, PAGE_FAULT, 0, ptr);
        if(!ok)
            return std::nullopt;

        const auto rip = core.read_reg(FDP_RIP_REGISTER);
        if(!rip)
            return std::nullopt;

        const auto cr3 = core.read_reg(FDP_CR3_REGISTER);
        if(!cr3)
            return std::nullopt;

        const auto bpid = FDP_SetBreakpoint(&*core.shm_, CPU_ID, FDP_SOFTHBP, 0, FDP_EXECUTE_BP, FDP_VIRTUAL_ADDRESS, *rip, 1, *cr3);
        if(bpid < 0)
            return std::nullopt;

        ok = FDP_Resume(&*core.shm_);
        if(!ok)
            return std::nullopt;

        while(true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ok = FDP_GetStateChanged(&*core.shm_);
            if(!ok)
                continue;

            FDP_State state = FDP_STATE_NULL;
            ok = FDP_GetState(&*core.shm_, &state);
            if(!ok)
                continue;
        }

        return virtual_to_physical(core, ptr, cr3_want);
    }
}

bool impl::Core::read_mem(void* vdst, uint64_t src, size_t size)
{
    const auto dst = reinterpret_cast<uint8_t*>(vdst);
    const auto usize = static_cast<uint32_t>(size);

    if(!ctx_)
        return raw_read_virtual(*this, dst, src, usize);

    if(current_ && current_->ctx == ctx_->ctx)
        return raw_read_virtual(*this, dst, src, usize);

    uint8_t buffer[PAGE_SIZE];
    size_t fill = 0;
    auto ptr = align<PAGE_SIZE>(src);
    size_t skip = src - ptr;
    while(fill < size)
    {
        auto phy = virtual_to_physical(*this, ptr, ctx_->ctx);
        if(!phy)
            FAIL(false, "unable to convert virtual address 0x%llx to physical after page fault injection: cr3 = 0x%llx", ptr, ctx_->ctx);

        const auto ok = FDP_ReadPhysicalMemory(&*shm_, buffer, sizeof buffer, *phy);
        if(!ok)
            FAIL(false, "unable to read phy mem 0x%llx-0x%llx virtual 0x%llx-0x%llx (%zd 0x%zx bytes)",
                 *phy, *phy + sizeof buffer, ptr, ptr + sizeof buffer, sizeof buffer, sizeof buffer);

        const auto chunk = std::min(size - fill, sizeof buffer - skip);
        memcpy(&dst[fill], &buffer[skip], chunk);
        fill += chunk;
        skip = 0;
        ptr += sizeof buffer;
    }

    return true;
}

bool impl::Core::write_mem(uint64_t dst, const void* src, size_t size)
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

std::optional<uint64_t> impl::Core::read_msr(msr_e reg)
{
    uint64_t value = 0;
    const auto ok = FDP_ReadMsr(&*shm_, CPU_ID, reg, &value);
    if(!ok)
        FAIL(std::nullopt, "unable to read msr %s 0x%x", msr_to_str(reg), reg);

    return value;
}

bool impl::Core::write_msr(msr_e reg, uint64_t value)
{
    const auto ok = FDP_WriteMsr(&*shm_, CPU_ID, reg, value);
    if(!ok)
        FAIL(false, "unable to write 0x%llx to msr %s 0x%x", value, msr_to_str(reg), reg);

    return true;
}

std::optional<uint64_t> impl::Core::read_reg(reg_e reg)
{
    uint64_t value;
    const auto ok = FDP_ReadRegister(&*shm_, CPU_ID, reg, &value);
    if(!ok)
        FAIL(std::nullopt, "unable to read reg %s 0x%x", reg_to_str(reg), reg);

    return value;
}

bool impl::Core::write_reg(reg_e reg, uint64_t value)
{
    const auto ok = FDP_WriteRegister(&*shm_, CPU_ID, reg, value);
    if(!ok)
        FAIL(false, "unable to write 0x%llx to reg %s 0x%x", value, reg_to_str(reg), reg);

    return true;
}

std::shared_ptr<ICore::Ctx> impl::Core::switch_context(const proc_t proc)
{
    return std::make_shared<ICore::Ctx>(ctx_, proc);
}