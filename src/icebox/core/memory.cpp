#include "memory.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "mem"
#include "core.hpp"
#include "endian.hpp"
#include "log.hpp"
#include "mmu.hpp"
#include "os.hpp"
#include "private.hpp"
#include "utils/utils.hpp"

#include <FDP.h>

struct core::Memory::Data
{
    Data(FDP_SHM& shm, Core& core)
        : shm(shm)
        , core(core)
    {
    }

    // members
    FDP_SHM& shm;
    Core&    core;
};

using MemData = core::Memory::Data;

core::Memory::Memory()  = default;
core::Memory::~Memory() = default;

void core::setup(Memory& mem, FDP_SHM& shm, Core& core)
{
    mem.d_ = std::make_unique<core::Memory::Data>(shm, core);
}

namespace
{
    constexpr uint64_t mask(int bits)
    {
        return ~(~uint64_t(0) << bits);
    }

    static opt<phy_t> slow_virtual_to_physical(MemData& m, uint64_t ptr, dtb_t dtb)
    {
        const virt_t virt     = {read_le64(&ptr)};
        const auto pml4e_base = dtb.val & (mask(40) << 12);
        const auto pml4e_ptr  = pml4e_base + virt.u.f.pml4 * 8;
        entry_t pml4e         = {0};
        auto ok               = FDP_ReadPhysicalMemory(&m.shm, reinterpret_cast<uint8_t*>(&pml4e), sizeof pml4e, pml4e_ptr);
        if(!ok)
            return {};

        if(!pml4e.u.f.can_read)
            return {};

        const auto pdpe_ptr = pml4e.u.f.page_frame_number * PAGE_SIZE + virt.u.f.pdp * 8;
        entry_t pdpe        = {0};
        ok                  = FDP_ReadPhysicalMemory(&m.shm, reinterpret_cast<uint8_t*>(&pdpe), sizeof pdpe, pdpe_ptr);
        if(!ok)
            return {};

        if(!pdpe.u.f.can_read)
            return {};

        // 1g page
        if(pdpe.u.f.large_page)
        {
            const auto offset = ptr & mask(30);
            const auto phy    = (pdpe.u.value & (mask(22) << 30)) + offset;
            return phy_t{phy};
        }

        const auto pde_ptr = pdpe.u.f.page_frame_number * PAGE_SIZE + virt.u.f.pd * 8;
        entry_t pde        = {0};
        ok                 = FDP_ReadPhysicalMemory(&m.shm, reinterpret_cast<uint8_t*>(&pde), sizeof pde, pde_ptr);
        if(!ok)
            return {};

        if(!pde.u.f.can_read)
            return {};

        // 2mb page
        if(pde.u.f.large_page)
        {
            const auto offset = ptr & mask(21);
            const auto phy    = (pde.u.value & (mask(31) << 21)) + offset;
            return phy_t{phy};
        }

        const auto pte_ptr = pde.u.f.page_frame_number * PAGE_SIZE + virt.u.f.pt * 8;
        entry_t pte        = {0};
        ok                 = FDP_ReadPhysicalMemory(&m.shm, reinterpret_cast<uint8_t*>(&pte), sizeof pte, pte_ptr);
        if(!ok)
            return {};

        if(!pte.u.f.can_read)
            return {};

        const auto phy = pte.u.f.page_frame_number * PAGE_SIZE + virt.u.f.offset;
        return phy_t{phy};
    }

    static opt<phy_t> fast_virtual_to_physical(MemData& d, uint64_t ptr, dtb_t dtb)
    {
        const auto backup = d.core.regs.read(FDP_CR3_REGISTER);
        d.core.regs.write(FDP_CR3_REGISTER, dtb.val);
        phy_t phy;
        const auto ok = FDP_VirtualToPhysical(&d.shm, 0, ptr, &phy.val);
        d.core.regs.write(FDP_CR3_REGISTER, backup);
        if(!ok)
            return {};

        return phy;
    }

    static bool inject_page_fault(MemData& d, dtb_t dtb, uint64_t src, bool user_mode)
    {
        const auto rip      = d.core.regs.read(FDP_RIP_REGISTER);
        const auto code     = user_mode ? 1 << 2 : 0;
        const auto injected = FDP_InjectInterrupt(&d.shm, 0, PAGE_FAULT, code, src);
        if(!injected)
            return FAIL(false, "unable to inject page fault");

        d.core.state.run_to(dtb, rip);
        return true;
    }

    static opt<phy_t> try_virtual_to_physical(MemData& d, uint64_t ptr, dtb_t dtb)
    {
        auto ret = fast_virtual_to_physical(d, ptr, dtb);
        if(ret)
            if(ret->val)
                return ret;

        return slow_virtual_to_physical(d, ptr, dtb);
    }
}

opt<phy_t> core::Memory::virtual_to_physical(uint64_t ptr, dtb_t dtb)
{
    auto ret = try_virtual_to_physical(*d_, ptr, dtb);
    if(ret)
        return ret;

    if(!d_->core.os || !d_->core.os->can_inject_fault(ptr))
        return {};

    const auto ok = inject_page_fault(*d_, dtb, ptr, true);
    if(!ok)
        return {};

    return try_virtual_to_physical(*d_, ptr, dtb);
}

namespace
{
    template <typename T>
    bool read_pages(const char* where, uint8_t* dst, uint64_t src, size_t size, const T& operand)
    {
        uint8_t buffer[PAGE_SIZE];
        size_t fill = 0;
        auto ptr    = utils::align<PAGE_SIZE>(src);
        size_t skip = src - ptr;
        while(fill < size)
        {
            const auto ok = operand(buffer, ptr, sizeof buffer);
            if(!ok)
                return FAIL(false, "unable to read {} mem {:#x}-{:#x} ({} {:#x} bytes)", where, ptr, ptr + sizeof buffer, sizeof buffer, sizeof buffer);

            const auto chunk = std::min(size - fill, sizeof buffer - skip);
            memcpy(&dst[fill], &buffer[skip], chunk);
            fill += chunk;
            skip = 0;
            ptr += sizeof buffer;
        }
        return true;
    }

    bool read_virtual(MemData& d, uint8_t* dst, dtb_t dtb, uint64_t src, uint32_t size)
    {
        const auto full = FDP_ReadVirtualMemory(&d.shm, 0, dst, size, src);
        if(full)
            return true;

        if(!d.core.os || !d.core.os->can_inject_fault(src))
            return false;

        return read_pages("virtual", dst, src, size, [&](uint8_t* pgdst, uint64_t pgsrc, uint32_t pgsize)
        {
            const auto ok = inject_page_fault(d, dtb, pgsrc, true);
            if(!ok)
                return false;

            return FDP_ReadVirtualMemory(&d.shm, 0, pgdst, pgsize, pgsrc);
        });
    }

    bool read_physical(MemData& d, uint8_t* dst, uint64_t src, size_t size)
    {
        return read_pages("physical", dst, src, size, [&](uint8_t* pgdst, uint64_t pgsrc, uint32_t pgsize)
        {
            return FDP_ReadPhysicalMemory(&d.shm, pgdst, pgsize, pgsrc);
        });
    }
}

bool core::Memory::read_virtual(void* vdst, uint64_t src, size_t size)
{
    const auto dst   = reinterpret_cast<uint8_t*>(vdst);
    const auto usize = static_cast<uint32_t>(size);
    const auto dtb   = dtb_t{d_->core.regs.read(FDP_CR3_REGISTER)};
    return ::read_virtual(*d_, dst, dtb, src, usize);
}

bool core::Memory::read_virtual(void* vdst, dtb_t dtb, uint64_t src, size_t size)
{
    const auto dst    = reinterpret_cast<uint8_t*>(vdst);
    const auto usize  = static_cast<uint32_t>(size);
    const auto backup = d_->core.regs.read(FDP_CR3_REGISTER);
    d_->core.regs.write(FDP_CR3_REGISTER, dtb.val);
    const auto ok = ::read_virtual(*d_, dst, dtb, src, usize);
    d_->core.regs.write(FDP_CR3_REGISTER, backup);
    return ok;
}

bool core::Memory::read_physical(void* vdst, uint64_t src, size_t size)
{
    const auto dst = reinterpret_cast<uint8_t*>(vdst);
    return ::read_physical(*d_, dst, src, size);
}
