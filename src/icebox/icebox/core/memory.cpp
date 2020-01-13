#include "memory.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "mem"
#include "core.hpp"
#include "core_private.hpp"
#include "endian.hpp"
#include "fdp.hpp"
#include "log.hpp"
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
    dtb_t dtb_select(core::Core& core, proc_t proc, uint64_t ptr)
    {
        return os::is_kernel_address(core, ptr) ? proc.kdtb : proc.udtb;
    }

    opt<phy_t> virtual_to_physical(core::Core& core, proc_t* proc, dtb_t dtb, uint64_t ptr)
    {
        const auto ret = fdp::virtual_to_physical(core, dtb, ptr);
        if(ret && ret->val)
            return ret;

        return os::virtual_to_physical(core, proc, dtb, ptr);
    }
}

opt<phy_t> memory::virtual_to_physical(core::Core& core, proc_t proc, uint64_t ptr)
{
    const auto dtb = dtb_select(core, proc, ptr);
    return ::virtual_to_physical(core, &proc, dtb, ptr);
}

opt<phy_t> memory::virtual_to_physical_with_dtb(core::Core& core, dtb_t dtb, uint64_t ptr)
{
    return ::virtual_to_physical(core, nullptr, dtb, ptr);
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

    bool read_virtual_page(core::Core& core, uint8_t* pgdst, uint64_t pgsrc, proc_t* proc, dtb_t dtb, size_t pgsize)
    {
        auto ok = fdp::read_virtual(core, pgdst, pgsrc, dtb, pgsize);
        if(ok)
            return true;

        return os::read_page(core, pgdst, pgsrc, proc, dtb);
    }

    bool read_virtual(core::Core& core, proc_t* proc, dtb_t dtb, uint8_t* dst, uint64_t src, uint32_t size)
    {
        if(!size)
            return true;

        const auto full = fdp::read_virtual(core, dst, src, dtb, size);
        if(full)
            return true;

        return read_pages("virtual", dst, src, size, [&](uint8_t* pgdst, uint64_t pgsrc, uint32_t pgsize)
        {
            return read_virtual_page(core, pgdst, pgsrc, proc, dtb, pgsize);
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

    bool write_virtual(core::Core& core, proc_t* proc, dtb_t dtb, uint64_t dst, const uint8_t* src, uint32_t size)
    {
        if(!size)
            return true;

        const auto full = fdp::write_virtual(core, dst, dtb, src, size);
        if(full)
            return true;

        const auto read_virtual = [&](uint8_t* pgdst, uint64_t pgsrc, size_t pgsize)
        {
            return read_virtual_page(core, pgdst, pgsrc, proc, dtb, pgsize);
        };
        const auto write_virtual = [&](uint64_t pgdst, const uint8_t* pgsrc, size_t pgsize)
        {
            auto ok = fdp::write_virtual(core, pgdst, dtb, pgsrc, pgsize);
            if(ok)
                return true;

            return os::write_page(core, pgdst, pgsrc, proc, dtb);
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

bool memory::read_virtual(core::Core& core, proc_t proc, void* vdst, uint64_t src, size_t size)
{
    const auto dst   = reinterpret_cast<uint8_t*>(vdst);
    const auto usize = static_cast<uint32_t>(size);
    const auto dtb   = dtb_select(core, proc, src);
    return ::read_virtual(core, &proc, dtb, dst, src, usize);
}

bool memory::read_virtual_with_dtb(core::Core& core, dtb_t dtb, void* vdst, uint64_t src, size_t size)
{
    const auto dst   = reinterpret_cast<uint8_t*>(vdst);
    const auto usize = static_cast<uint32_t>(size);
    return ::read_virtual(core, nullptr, dtb, dst, src, usize);
}

bool memory::read_physical(core::Core& core, void* vdst, uint64_t src, size_t size)
{
    const auto dst = reinterpret_cast<uint8_t*>(vdst);
    return ::read_physical(core, dst, src, size);
}

bool memory::write_virtual(core::Core& core, proc_t proc, uint64_t dst, const void* vsrc, size_t size)
{
    const auto src   = reinterpret_cast<const uint8_t*>(vsrc);
    const auto usize = static_cast<uint32_t>(size);
    const auto dtb   = dtb_select(core, proc, dst);
    return ::write_virtual(core, &proc, dtb, dst, src, usize);
}

bool memory::write_virtual_with_dtb(core::Core& core, dtb_t dtb, uint64_t dst, const void* vsrc, size_t size)
{
    const auto src   = reinterpret_cast<const uint8_t*>(vsrc);
    const auto usize = static_cast<uint32_t>(size);
    return ::write_virtual(core, nullptr, dtb, dst, src, usize);
}

bool memory::write_physical(core::Core& core, uint64_t dst, const void* vsrc, size_t size)
{
    const auto src = reinterpret_cast<const uint8_t*>(vsrc);
    return ::write_physical(core, dst, src, size);
}
