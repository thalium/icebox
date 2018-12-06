#include "memory.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "mem"
#include "core.hpp"
#include "core/helpers.hpp"
#include "endian.hpp"
#include "log.hpp"
#include "mmu.hpp"
#include "private.hpp"
#include "utils/utils.hpp"

#include <FDP.h>
#include <algorithm>
#include <string.h>

struct core::Memory::Data
{
    Data(FDP_SHM& shm, Core& core)
        : shm(shm)
        , core(core)
    {
        memset(&current, 0, sizeof current);
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
    constexpr uint64_t mask(int bits)
    {
        return ~(~uint64_t(0) << bits);
    }

    opt<uint64_t> virtual_to_physical(MemData& m, uint64_t ptr, uint64_t dtb)
    {
        const virt_t virt = {read_le64(&ptr)};
        const auto pml4e_base = dtb & (mask(40) << 12);
        const auto pml4e_ptr  = pml4e_base + virt.u.f.pml4 * 8;
        entry_t pml4e = {0};
        auto ok = FDP_ReadPhysicalMemory(&m.shm, reinterpret_cast<uint8_t*>(&pml4e), sizeof pml4e, pml4e_ptr);
        if(!ok)
            return {};

        if(!pml4e.u.f.can_read)
            return {};

        const auto pdpe_ptr = pml4e.u.f.page_frame_number * PAGE_SIZE + virt.u.f.pdp * 8;
        entry_t pdpe = {0};
        ok = FDP_ReadPhysicalMemory(&m.shm, reinterpret_cast<uint8_t*>(&pdpe), sizeof pdpe, pdpe_ptr);
        if(!ok)
            return {};

        if(!pdpe.u.f.can_read)
            return {};

        // 1g page
        if(pdpe.u.f.large_page)
        {
            const auto offset = ptr & mask(30);
            const auto phy    = (pdpe.u.value & (mask(22) << 30)) + offset;
            return phy;
        }

        const auto pde_ptr = pdpe.u.f.page_frame_number * PAGE_SIZE + virt.u.f.pd * 8;
        entry_t pde = {0};
        ok = FDP_ReadPhysicalMemory(&m.shm, reinterpret_cast<uint8_t*>(&pde), sizeof pde, pde_ptr);
        if(!ok)
            return {};

        if(!pde.u.f.can_read)
            return {};

        // 2mb page
        if(pde.u.f.large_page)
        {
            const auto offset = ptr & mask(21);
            const auto phy    = (pde.u.value & (mask(31) << 21)) + offset;
            return phy;
        }

        const auto pte_ptr = pde.u.f.page_frame_number * PAGE_SIZE + virt.u.f.pt * 8;
        entry_t pte = {0};
        ok = FDP_ReadPhysicalMemory(&m.shm, reinterpret_cast<uint8_t*>(&pte), sizeof pte, pte_ptr);
        if(!ok)
            return {};

        if(!pte.u.f.can_read)
            return {};

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
    bool is_user_mode(MemData& m)
    {
        const auto cs = m.core.regs.read(FDP_CS_REGISTER);
        return !!(cs & 3);
    }

    bool is_kernel_page(uint64_t ptr)
    {
        return !!(ptr & 0xFFF0000000000000);
    }

    struct Cr3Swap
    {
        Cr3Swap(MemData& m, proc_t want)
            : m_(m)
            , current_(m.current)
            , want_(want)
        {
        }

        bool setup()
        {
            if(want_.dtb == current_.dtb)
                return true;

            const auto ok = m_.core.regs.write(FDP_CR3_REGISTER, want_.dtb);
            if(!ok)
                LOG(ERROR, "unable to set CR3 to %" PRIx64 " for context swap", want_.dtb);

            return true;
        }

        ~Cr3Swap()
        {
            if(want_.dtb == current_.dtb)
                return;

            const auto ok = m_.core.regs.write(FDP_CR3_REGISTER, current_.dtb);
            if(!ok)
                LOG(ERROR, "unable to restore CR3 to %" PRIx64 " from %" PRIx64, current_.dtb, want_.dtb);
        }

        MemData& m_;
        proc_t   current_;
        proc_t   want_;
    };

    opt<Cr3Swap> swap_context(MemData& m, proc_t want)
    {
        Cr3Swap swap(m, want);
        const auto ok = swap.setup();
        if(!ok)
            return {};

        return swap;
    }

    bool read_virtual_from_proc(MemData& m, uint8_t* dst, uint64_t src, uint32_t size, proc_t proc)
    {
        const auto swap = swap_context(m, proc);
        if(!swap)
            return false;

        return FDP_ReadVirtualMemory(&m.shm, 0, dst, size, src);
    }

    bool try_read_virtual_page(MemData& m, uint8_t* dst, uint64_t src, uint32_t size, proc_t proc, bool user_mode)
    {
        const auto read = read_virtual_from_proc(m, dst, src, size, proc);
        if(read)
            return true;

        const auto rip  = m.core.regs.read(FDP_RIP_REGISTER);
        const auto swap = swap_context(m, proc);
        if(!swap)
            FAIL(false, "unable to swap context");

        const auto code     = user_mode ? 1 << 2 : 0;
        const auto injected = FDP_InjectInterrupt(&m.shm, 0, PAGE_FAULT, code, src);
        if(!injected)
            FAIL(false, "unable to inject page fault");

        m.core.state.run_to(proc, rip);
        const auto ok = FDP_ReadVirtualMemory(&m.shm, 0, dst, size, src);
        return ok;
    }

    bool try_read_virtual_only(MemData& d, uint8_t* dst, uint64_t src, uint32_t size)
    {
        const auto proc = d.context ? *d.context : d.current;
        const auto full = read_virtual_from_proc(d, dst, src, size, proc);
        if(full)
            return true;

        const auto user_mode = is_user_mode(d);
        if(!user_mode || is_kernel_page(src))
            return false;

        uint8_t buffer[PAGE_SIZE];
        size_t fill = 0;
        auto ptr = utils::align<PAGE_SIZE>(src);
        size_t skip = src - ptr;
        while(fill < size)
        {
            const auto ok = try_read_virtual_page(d, buffer, ptr, sizeof buffer, proc, user_mode);
            if(!ok)
                FAIL(false, "unable to read virtual mem 0x%" PRIx64 "-0x%" PRIx64 " (%zd 0x%zx bytes)", ptr, ptr + sizeof buffer, sizeof buffer, sizeof buffer);

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
    const auto dst   = reinterpret_cast<uint8_t*>(vdst);
    const auto usize = static_cast<uint32_t>(size);
    return try_read_virtual_only(*d_, dst, src, usize);
}
