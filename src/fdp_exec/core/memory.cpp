#include "memory.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "mem"
#include "log.hpp"
#include "utils/utils.hpp"
#include "mmu.hpp"
#include "endian.hpp"
#include "core.hpp"
#include "private.hpp"

#include <FDP.h>
#include <algorithm>

struct core::Memory::Data
{
    Data(FDP_SHM& shm, Core& core)
        : shm(shm)
        , core(core)
    {
    }

    // members
    FDP_SHM&    shm;
    Core&       core;
    proc_t      current;
    opt<proc_t> context;
};

using MemData = core::Memory::Data;

core::Memory::Memory()
{
}

core::Memory::~Memory()
{
}

void core::setup(Memory& mem, FDP_SHM& shm, Core& core)
{
    mem.d_ = std::make_unique<core::Memory::Data>(shm, core);
}

struct core::ProcessContextPrivate
{
    ProcessContextPrivate(opt<proc_t>& target, proc_t proc)
        : target_(target) // save reference first
        , backup_(target)
    {
        target_ = proc; // then update it
    }

    ~ProcessContextPrivate()
    {
        target_ = backup_; // restore previous value
    }

    opt<proc_t>& target_;
    opt<proc_t>  backup_;
};

void core::Memory::update(proc_t current)
{
    d_->current = current;
}

core::ProcessContext core::Memory::switch_process(proc_t proc)
{
    return std::make_shared<core::ProcessContextPrivate>(d_->context, proc);
}

namespace
{
    uint64_t mask(int bits)
    {
        return ~(~uint64_t(0) << bits);
    }

    opt<uint64_t> virtual_to_physical(MemData& m, uint64_t ptr, uint64_t dtb)
    {
        const virt_t virt = {read_le64(&ptr)};
        const auto pml4e_base = dtb & (mask(40) << 12);
        const auto pml4e_ptr = pml4e_base + virt.u.f.pml4 * 8;
        entry_t pml4e = {0};
        auto ok = FDP_ReadPhysicalMemory(&m.shm, reinterpret_cast<uint8_t*>(&pml4e), sizeof pml4e, pml4e_ptr);
        if(!ok)
            return std::nullopt;

        if(!pml4e.u.f.can_read)
            return std::nullopt;

        const auto pdpe_ptr = pml4e.u.f.page_frame_number * PAGE_SIZE + virt.u.f.pdp * 8;
        entry_t pdpe = {0};
        ok = FDP_ReadPhysicalMemory(&m.shm, reinterpret_cast<uint8_t*>(&pdpe), sizeof pdpe, pdpe_ptr);
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
        ok = FDP_ReadPhysicalMemory(&m.shm, reinterpret_cast<uint8_t*>(&pde), sizeof pde, pde_ptr);
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
        ok = FDP_ReadPhysicalMemory(&m.shm, reinterpret_cast<uint8_t*>(&pte), sizeof pte, pte_ptr);
        if(!ok)
            return std::nullopt;

        if(!pte.u.f.can_read)
            return std::nullopt;

        const auto phy = pte.u.f.page_frame_number * PAGE_SIZE + virt.u.f.offset;
        return phy;
    }
}

opt<uint64_t> core::Memory::virtual_to_physical(uint64_t ptr, uint64_t dtb)
{
    return ::virtual_to_physical(*d_, ptr, dtb);
}


namespace
{
    bool read_virtual(MemData& m, uint8_t* dst, uint64_t src, uint32_t size)
    {
        auto ok = FDP_ReadVirtualMemory(&m.shm, 0, dst, size, src);
        if(ok)
            return true;

        const auto rip = m.core.regs.read(FDP_RIP_REGISTER);
        if(!rip)
            FAIL(false, "unable to read rip for page fault injection");

        ok = FDP_InjectInterrupt(&m.shm, 0, PAGE_FAULT, 0, src);
        if(!ok)
            FAIL(false, "unable to inject page fault at %" PRIx64, src);

        auto bp = m.core.state.set_breakpoint(*rip, m.current, core::FILTER_CR3);
        m.core.state.resume();
        m.core.state.wait();
        bp.reset();

        ok = FDP_ReadVirtualMemory(&m.shm, 0, dst, size, src);
        if(!ok)
            FAIL(false, "unable to read mem %" PRIx64 "-%" PRIx64 " after page fault injection", src, src + size);

        return ok;
    }

    bool try_read_mem(MemData& m, uint8_t* dst, uint64_t src, uint32_t size)
    {
        if(!m.context || m.current.dtb == m.context->dtb)
            return read_virtual(m, dst, src, size);

        uint8_t buffer[PAGE_SIZE];
        size_t fill = 0;
        auto ptr = utils::align<PAGE_SIZE>(src);
        size_t skip = src - ptr;
        while(fill < size)
        {
            auto phy = virtual_to_physical(m, ptr, m.context->dtb);
            if(!phy)
                FAIL(false, "unable to convert virtual address 0x%llx to physical after page fault injection: dtb = 0x%llx", ptr, m.context->dtb);

            const auto ok = FDP_ReadPhysicalMemory(&m.shm, buffer, sizeof buffer, *phy);
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
}

bool core::Memory::virtual_read(void* vdst, uint64_t src, size_t size)
{
    const auto dst = reinterpret_cast<uint8_t*>(vdst);
    const auto usize = static_cast<uint32_t>(size);
    if(size < PAGE_SIZE)
        return try_read_mem(*d_, dst, src, usize);

    // FIXME check if we can read bigger than PAGE_SIZE at once
    uint8_t buffer[PAGE_SIZE];
    uint32_t read = 0;
    while(read < usize)
    {
        const auto chunk = std::min<uint32_t>(sizeof buffer, usize - read);
        const auto ok = try_read_mem(*d_, &dst[read], src + read, chunk);
        if(!ok)
            return false;

        read += chunk;
    }
    return true;
}
