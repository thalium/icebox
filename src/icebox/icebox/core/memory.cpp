#include "memory.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "mem"
#include "core.hpp"
#include "core_private.hpp"
#include "endian.hpp"
#include "fdp.hpp"
#include "log.hpp"
#include "mmu.hpp"
#include "utils/utils.hpp"

#include <array>

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

    opt<phy_t> slow_virtual_to_physical(core::Core& core, uint64_t ptr, dtb_t dtb)
    {
        const virt_t virt     = {read_le64(&ptr)};
        const auto pml4e_base = dtb.val & (mask(40) << 12);
        const auto pml4e_ptr  = pml4e_base + virt.u.f.pml4 * 8;
        entry_t pml4e         = {0};
        auto ok               = fdp::read_physical(core, &pml4e, phy_t{pml4e_ptr}, sizeof pml4e);
        if(!ok)
            return {};

        if(!pml4e.u.f.can_read)
            return {};

        const auto pdpe_ptr = pml4e.u.f.page_frame_number * PAGE_SIZE + virt.u.f.pdp * 8;
        entry_t pdpe        = {0};
        ok                  = fdp::read_physical(core, &pdpe, phy_t{pdpe_ptr}, sizeof pdpe);
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
        ok                 = fdp::read_physical(core, &pde, phy_t{pde_ptr}, sizeof pde);
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
        ok                 = fdp::read_physical(core, &pte, phy_t{pte_ptr}, sizeof pte);
        if(!ok)
            return {};

        if(!pte.u.f.can_read)
            return {};

        const auto phy = pte.u.f.page_frame_number * PAGE_SIZE + virt.u.f.offset;
        return phy_t{phy};
    }

    bool inject_page_fault(core::Core& core, dtb_t /*dtb*/, uint64_t src, bool user_mode)
    {
        const auto code     = user_mode ? 1 << 2 : 0;
        const auto injected = fdp::inject_interrupt(core, PAGE_FAULT, code, src);
        if(!injected)
            return FAIL(false, "unable to inject page fault");

        state::run_to_current(core, "inject_pf");
        return true;
    }

    opt<phy_t> try_virtual_to_physical(core::Core& core, uint64_t ptr, dtb_t dtb)
    {
        const auto ret = fdp::virtual_to_physical(core, dtb, ptr);
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

    if(!os::can_inject_fault(core, ptr))
        return {};

    auto& mem = *core.mem_;
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
    bool read_pages(const char* where, uint8_t* dst, uint64_t src, size_t size, const T& operand)
    {
        auto buffer = std::array<uint8_t, PAGE_SIZE>{};
        auto fill   = size_t{};
        auto ptr    = utils::align<PAGE_SIZE>(src);
        auto skip   = src - ptr;
        while(fill < size)
        {
            const auto ok = operand(&buffer[0], ptr, sizeof buffer);
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

    bool read_virtual_page(core::Core& core, uint8_t* pgdst, uint64_t pgsrc, dtb_t dtb, size_t pgsize)
    {
        auto ok = fdp::read_virtual(core, pgdst, pgsrc, dtb, pgsize);
        if(ok)
            return true;

        ok = inject_page_fault(core, dtb, pgsrc, true);
        if(!ok)
            return false;

        return fdp::read_virtual(core, pgdst, pgsrc, dtb, pgsize);
    }

    bool read_virtual(core::Core& core, uint8_t* dst, dtb_t dtb, uint64_t src, uint32_t size)
    {
        if(!size)
            return true;

        const auto full = fdp::read_virtual(core, dst, src, dtb, size);
        if(full)
            return true;

        if(!os::can_inject_fault(core, src))
            return false;

        return read_pages("virtual", dst, src, size, [&](uint8_t* pgdst, uint64_t pgsrc, uint32_t pgsize)
        {
            return read_virtual_page(core, pgdst, pgsrc, dtb, pgsize);
        });
    }

    bool read_physical(core::Core& core, uint8_t* dst, uint64_t src, size_t size)
    {
        if(!size)
            return true;

        return read_pages("physical", dst, src, size, [&](uint8_t* pgdst, uint64_t pgsrc, uint32_t pgsize)
        {
            return fdp::read_physical(core, pgdst, phy_t{pgsrc}, pgsize);
        });
    }

    template <typename T, typename U>
    bool write_pages(const char* where, uint64_t dst, const uint8_t* src, size_t size, const T& read, const U& write)
    {
        auto buffer = std::array<uint8_t, PAGE_SIZE>{};
        auto fill   = size_t{};
        auto ptr    = utils::align<PAGE_SIZE>(dst);
        auto skip   = dst - ptr;
        while(fill < size)
        {
            // we want to write only PAGE_SIZE buffers
            // so read back missing leading & trailing bytes if any
            const auto chunk = std::min(size - fill, sizeof buffer - skip);
            if(chunk != PAGE_SIZE)
            {
                const auto ok = read(&buffer[0], ptr, sizeof buffer);
                if(!ok)
                    return FAIL(false, "unable to read %s mem 0x%" PRIx64 "-0x%" PRIx64 " (%zu 0x%zx bytes)", where, ptr, ptr + sizeof buffer, sizeof buffer, sizeof buffer);
            }

            // overwrite with input data
            memcpy(&buffer[skip], &src[fill], chunk);
            const auto ok = write(ptr, &buffer[0], sizeof buffer);
            if(!ok)
                return FAIL(false, "unable to write %s mem 0x%" PRIx64 "-0x%" PRIx64 " (%zu 0x%zx bytes)", where, ptr, ptr + sizeof buffer, sizeof buffer, sizeof buffer);

            fill += chunk;
            skip = 0;
            ptr += sizeof buffer;
        }
        return true;
    }

    bool write_virtual(core::Core& core, uint64_t dst, dtb_t dtb, const uint8_t* src, uint32_t size)
    {
        if(!size)
            return true;

        const auto full = fdp::write_virtual(core, dst, dtb, src, size);
        if(full)
            return true;

        if(!os::can_inject_fault(core, dst))
            return false;

        const auto read_virtual = [&](uint8_t* pgdst, uint64_t pgsrc, size_t pgsize)
        {
            return read_virtual_page(core, pgdst, pgsrc, dtb, pgsize);
        };
        const auto write_virtual = [&](uint64_t pgdst, const uint8_t* pgsrc, size_t pgsize)
        {
            auto ok = fdp::write_virtual(core, pgdst, dtb, pgsrc, pgsize);
            if(ok)
                return true;

            ok = inject_page_fault(core, dtb, pgdst, true);
            if(!ok)
                return false;

            return fdp::write_virtual(core, pgdst, dtb, pgsrc, pgsize);
        };
        return write_pages("virtual", dst, src, size, read_virtual, write_virtual);
    }

    bool write_physical(core::Core& core, uint64_t dst, const uint8_t* src, size_t size)
    {
        if(!size)
            return true;

        const auto read_physical = [&](uint8_t* pgdst, uint64_t pgsrc, size_t pgsize)
        {
            return fdp::read_physical(core, pgdst, phy_t{pgsrc}, pgsize);
        };
        const auto write_physical = [&](uint64_t pgdst, const uint8_t* pgsrc, size_t pgsize)
        {
            return fdp::write_physical(core, phy_t{pgdst}, pgsrc, pgsize);
        };
        return write_pages("physical", dst, src, size, read_physical, write_physical);
    }
}

bool memory::read_virtual(core::Core& core, void* vdst, uint64_t src, size_t size)
{
    const auto dst   = reinterpret_cast<uint8_t*>(vdst);
    const auto usize = static_cast<uint32_t>(size);
    const auto dtb   = dtb_t{registers::read(core, reg_e::cr3)};
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

bool memory::write_virtual(core::Core& core, uint64_t dst, const void* vsrc, size_t size)
{
    const auto src   = reinterpret_cast<const uint8_t*>(vsrc);
    const auto usize = static_cast<uint32_t>(size);
    const auto dtb   = dtb_t{registers::read(core, reg_e::cr3)};
    return ::write_virtual(core, dst, dtb, src, usize);
}

bool memory::write_virtual_with_dtb(core::Core& core, uint64_t dst, dtb_t dtb, const void* vsrc, size_t size)
{
    const auto src   = reinterpret_cast<const uint8_t*>(vsrc);
    const auto usize = static_cast<uint32_t>(size);
    return ::write_virtual(core, dst, dtb, src, usize);
}

bool memory::write_physical(core::Core& core, uint64_t dst, const void* vsrc, size_t size)
{
    const auto src = reinterpret_cast<const uint8_t*>(vsrc);
    return ::write_physical(core, dst, src, size);
}
