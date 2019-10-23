#include "memory.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "mem"
#include "core.hpp"
#include "core_private.hpp"
#include "endian.hpp"
#include "fdp.hpp"
#include "log.hpp"
#include "mmu.hpp"
#include "os.hpp"
#include "private.hpp"
#include "utils/utils.hpp"

struct memory::Memory
{
    int depth = 0;
};

std::shared_ptr<memory::Memory> memory::setup()
{
    return std::make_shared<Memory>();
}

namespace
{
    constexpr uint64_t mask(int bits)
    {
        return ~(~uint64_t(0) << bits);
    }

    static opt<phy_t> slow_virtual_to_physical(core::Core& core, uint64_t ptr, dtb_t dtb)
    {
        const virt_t virt     = {read_le64(&ptr)};
        const auto pml4e_base = dtb.val & (mask(40) << 12);
        const auto pml4e_ptr  = pml4e_base + virt.u.f.pml4 * 8;
        entry_t pml4e         = {0};
        auto ok               = fdp::read_physical(*core.d_->shm_, &pml4e, sizeof pml4e, phy_t{pml4e_ptr});
        if(!ok)
            return {};

        if(!pml4e.u.f.can_read)
            return {};

        const auto pdpe_ptr = pml4e.u.f.page_frame_number * PAGE_SIZE + virt.u.f.pdp * 8;
        entry_t pdpe        = {0};
        ok                  = fdp::read_physical(*core.d_->shm_, &pdpe, sizeof pdpe, phy_t{pdpe_ptr});
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
        ok                 = fdp::read_physical(*core.d_->shm_, &pde, sizeof pde, phy_t{pde_ptr});
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
        ok                 = fdp::read_physical(*core.d_->shm_, &pte, sizeof pte, phy_t{pte_ptr});
        if(!ok)
            return {};

        if(!pte.u.f.can_read)
            return {};

        const auto phy = pte.u.f.page_frame_number * PAGE_SIZE + virt.u.f.offset;
        return phy_t{phy};
    }

    static bool inject_page_fault(core::Core& core, dtb_t /*dtb*/, uint64_t src, bool user_mode)
    {
        const auto code     = user_mode ? 1 << 2 : 0;
        const auto injected = fdp::inject_interrupt(*core.d_->shm_, PAGE_FAULT, code, src);
        if(!injected)
            return FAIL(false, "unable to inject page fault");

        state::run_to_current(core, "inject_pf");
        return true;
    }

    static opt<phy_t> try_virtual_to_physical(core::Core& core, uint64_t ptr, dtb_t dtb)
    {
        const auto ret = fdp::virtual_to_physical(*core.d_->shm_, dtb, ptr);
        if(ret && ret->val)
            return ret;

        return slow_virtual_to_physical(core, ptr, dtb);
    }
}

opt<phy_t> memory::virtual_to_physical(core::Core& core, uint64_t ptr, dtb_t dtb)
{
    const auto ret = try_virtual_to_physical(core, ptr, dtb);
    if(ret)
        return ret;

    if(!core.os || !core.os->can_inject_fault(ptr))
        return {};

    auto& mem = *core.d_->mem_;
    mem.depth++;
    if(mem.depth > 1)
        return {};

    const auto ok = inject_page_fault(core, dtb, ptr, true);
    mem.depth--;
    if(!ok)
        return {};

    return try_virtual_to_physical(core, ptr, dtb);
}

namespace
{
    template <typename T>
    static bool read_pages(const char* where, uint8_t* dst, uint64_t src, size_t size, const T& operand)
    {
        uint8_t buffer[PAGE_SIZE];
        size_t fill = 0;
        auto ptr    = utils::align<PAGE_SIZE>(src);
        size_t skip = src - ptr;
        while(fill < size)
        {
            const auto ok = operand(buffer, ptr, sizeof buffer);
            if(!ok)
                return FAIL(false, "unable to read %s mem 0x%" PRIx64 "-0x%" PRIx64 " (%zu 0x%zx bytes)", where, ptr, ptr + sizeof buffer, sizeof buffer, sizeof buffer);

            const auto chunk = std::min(size - fill, sizeof buffer - skip);
            memcpy(&dst[fill], &buffer[skip], chunk);
            fill += chunk;
            skip = 0;
            ptr += sizeof buffer;
        }
        return true;
    }

    static bool read_virtual(core::Core& core, uint8_t* dst, dtb_t dtb, uint64_t src, uint32_t size)
    {
        const auto full = fdp::read_virtual(*core.d_->shm_, dst, size, dtb, src);
        if(full)
            return true;

        if(!core.os || !core.os->can_inject_fault(src))
            return false;

        return read_pages("virtual", dst, src, size, [&](uint8_t* pgdst, uint64_t pgsrc, uint32_t pgsize)
        {
            auto ok = fdp::read_virtual(*core.d_->shm_, pgdst, pgsize, dtb, pgsrc);
            if(ok)
                return true;

            ok = inject_page_fault(core, dtb, pgsrc, true);
            if(!ok)
                return false;

            return fdp::read_virtual(*core.d_->shm_, pgdst, pgsize, dtb, pgsrc);
        });
    }

    static bool read_physical(core::Core& core, uint8_t* dst, uint64_t src, size_t size)
    {
        return read_pages("physical", dst, src, size, [&](uint8_t* pgdst, uint64_t pgsrc, uint32_t pgsize)
        {
            return fdp::read_physical(*core.d_->shm_, pgdst, pgsize, phy_t{pgsrc});
        });
    }
}

bool memory::read_virtual(core::Core& core, void* vdst, uint64_t src, size_t size)
{
    const auto dst   = reinterpret_cast<uint8_t*>(vdst);
    const auto usize = static_cast<uint32_t>(size);
    const auto dtb   = dtb_t{registers::read(core, FDP_CR3_REGISTER)};
    return ::read_virtual(core, dst, dtb, src, usize);
}

bool memory::read_virtual_with_dtb(core::Core& core, void* vdst, dtb_t dtb, uint64_t src, size_t size)
{
    const auto dst   = reinterpret_cast<uint8_t*>(vdst);
    const auto usize = static_cast<uint32_t>(size);
    return ::read_virtual(core, dst, dtb, src, usize);
}

bool memory::read_physical(core::Core& core, void* vdst, uint64_t src, size_t size)
{
    const auto dst = reinterpret_cast<uint8_t*>(vdst);
    return ::read_physical(core, dst, src, size);
}
