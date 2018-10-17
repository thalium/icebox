#define PRIVATE_CORE__
#include "private.hpp"

#define FDP_MODULE "mem"
#include "log.hpp"
#include "utils.hpp"
#include "mmu.hpp"
#include "endian.hpp"
#include "core.hpp"

#include <FDP.h>
#include <algorithm>

namespace
{
    struct Mem
        : public core::IMemory
    {
        Mem(FDP_SHM& shm)
            : shm_(shm)
        {
        }

        // core::IMemory methods
        void                    update              (const core::BreakState& state) override;
        opt<uint64_t>           virtual_to_physical (uint64_t ptr, uint64_t dtb) override;
        core::ProcessContext    switch_process      (proc_t proc) override;
        bool                    read                (void* dst, uint64_t src, size_t size) override;

        // members
        FDP_SHM&    shm_;
        proc_t      current_;
        opt<proc_t> context_;
    };
}

std::unique_ptr<core::IMemory> core::make_memory(FDP_SHM& shm)
{
    return std::make_unique<Mem>(shm);
}

namespace core
{
    struct ProcessContextPrivate
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
}

void Mem::update(const core::BreakState& state)
{
    current_ = state.proc;
}

core::ProcessContext Mem::switch_process(proc_t proc)
{
    return std::make_shared<core::ProcessContextPrivate>(context_, proc);
}

namespace
{
    uint64_t mask(int bits)
    {
        return ~(~uint64_t(0) << bits);
    }
}

opt<uint64_t> Mem::virtual_to_physical(uint64_t ptr, uint64_t dtb)
{
    const virt_t virt = {read_le64(&ptr)};
    const auto pml4e_base = dtb & (mask(40) << 12);
    const auto pml4e_ptr = pml4e_base + virt.u.f.pml4 * 8;
    entry_t pml4e = {0};
    auto ok = FDP_ReadPhysicalMemory(&shm_, reinterpret_cast<uint8_t*>(&pml4e), sizeof pml4e, pml4e_ptr);
    if(!ok)
        return std::nullopt;

    if(!pml4e.u.f.can_read)
        return std::nullopt;

    const auto pdpe_ptr = pml4e.u.f.page_frame_number * PAGE_SIZE + virt.u.f.pdp * 8;
    entry_t pdpe = {0};
    ok = FDP_ReadPhysicalMemory(&shm_, reinterpret_cast<uint8_t*>(&pdpe), sizeof pdpe, pdpe_ptr);
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
    ok = FDP_ReadPhysicalMemory(&shm_, reinterpret_cast<uint8_t*>(&pde), sizeof pde, pde_ptr);
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
    ok = FDP_ReadPhysicalMemory(&shm_, reinterpret_cast<uint8_t*>(&pte), sizeof pte, pte_ptr);
    if(!ok)
        return std::nullopt;

    // FIXME ignore can_read flag?
    const auto phy = pte.u.f.page_frame_number * PAGE_SIZE + virt.u.f.offset;
    return phy;
}

namespace
{
    bool read_virtual(Mem& mem, uint8_t* dst, uint64_t src, uint32_t size)
    {
        const auto ok = FDP_ReadVirtualMemory(&mem.shm_, 0, dst, size, src);
        if(!ok)
            FAIL(false, "unable to read mem 0x%llx-0x%llx (%u 0x%x bytes)", src, src + size, size, size);

        return true;
    }

    bool try_read_mem(Mem& m, uint8_t* dst, uint64_t src, uint32_t size)
    {
        if(!m.context_ || m.current_.dtb == m.context_->dtb)
            return read_virtual(m, dst, src, size);

        uint8_t buffer[PAGE_SIZE];
        size_t fill = 0;
        auto ptr = utils::align<PAGE_SIZE>(src);
        size_t skip = src - ptr;
        while(fill < size)
        {
            auto phy = m.virtual_to_physical(ptr, m.context_->dtb);
            if(!phy)
                FAIL(false, "unable to convert virtual address 0x%llx to physical after page fault injection: dtb = 0x%llx", ptr, m.context_->dtb);

            const auto ok = FDP_ReadPhysicalMemory(&m.shm_, buffer, sizeof buffer, *phy);
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

bool Mem::read(void* vdst, uint64_t src, size_t size)
{
    const auto dst = reinterpret_cast<uint8_t*>(vdst);
    const auto usize = static_cast<uint32_t>(size);
    if(size < PAGE_SIZE)
        return try_read_mem(*this, dst, src, usize);

    // FIXME check if we can read bigger than PAGE_SIZE at once
    uint8_t buffer[PAGE_SIZE];
    uint32_t read = 0;
    while(read < usize)
    {
        const auto chunk = std::min<uint32_t>(sizeof buffer, usize - read);
        const auto ok = try_read_mem(*this, &dst[read], src + read, chunk);
        if(!ok)
            return false;

        read += chunk;
    }
    return true;
}
