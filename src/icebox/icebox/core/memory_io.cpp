#include "memory.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "io"
#include "core.hpp"
#include "core_private.hpp"
#include "endian.hpp"
#include "interfaces/if_os.hpp"
#include "os.hpp"

namespace
{
    template <typename T, T (*read)(const void*)>
    opt<T> read_mem(const memory::Io& io, uint64_t src)
    {
        auto value    = T{};
        const auto ok = io.read_all(&value, src, sizeof value);
        if(!ok)
            return std::nullopt;

        return read(&value);
    }

    template <typename T, void (*write)(void*, T)>
    bool write_mem(const memory::Io& io, uint64_t dst, T arg)
    {
        auto value = T{};
        write(&value, arg);
        return io.write_all(dst, &value, sizeof value);
    }
}

memory::Io memory::make_io_kernel(core::Core& core)
{
    const auto dtb = core.os_->kernel_dtb();
    return memory::Io{core, {}, dtb};
}

memory::Io memory::make_io_current(core::Core& core)
{
    const auto dtb = dtb_t{registers::read(core, reg_e::cr3)};
    return memory::Io{core, {}, dtb};
}

memory::Io memory::make_io(core::Core& core, proc_t proc)
{
    return memory::Io{core, proc, proc.udtb};
}

opt<uint8_t> memory::Io::byte(uint64_t ptr) const
{
    return read_mem<uint8_t, &::read_byte>(*this, ptr);
}

opt<uint16_t> memory::Io::le16(uint64_t ptr) const
{
    return read_mem<uint16_t, &::read_le16>(*this, ptr);
}

opt<uint32_t> memory::Io::le32(uint64_t ptr) const
{
    return read_mem<uint32_t, &::read_le32>(*this, ptr);
}

opt<uint64_t> memory::Io::le64(uint64_t ptr) const
{
    return read_mem<uint64_t, &::read_le64>(*this, ptr);
}

opt<uint16_t> memory::Io::be16(uint64_t ptr) const
{
    return read_mem<uint16_t, &::read_be16>(*this, ptr);
}

opt<uint32_t> memory::Io::be32(uint64_t ptr) const
{
    return read_mem<uint32_t, &::read_be32>(*this, ptr);
}

opt<uint64_t> memory::Io::be64(uint64_t ptr) const
{
    return read_mem<uint64_t, &::read_be64>(*this, ptr);
}

opt<uint64_t> memory::Io::read(uint64_t ptr) const
{
    constexpr auto rfunc = is_little_endian ? ::read_le64 : ::read_be64;
    return read_mem<uint64_t, rfunc>(*this, ptr);
}

bool memory::Io::read_all(void* dst, uint64_t ptr, size_t size) const
{
    if(proc)
        return memory::read_virtual(core, *proc, dst, ptr, size);

    return memory::read_virtual_with_dtb(core, dtb, dst, ptr, size);
}

opt<phy_t> memory::Io::physical(uint64_t ptr) const
{
    if(proc)
        return memory::virtual_to_physical(core, *proc, ptr);

    return memory::virtual_to_physical_with_dtb(core, dtb, ptr);
}

bool memory::Io::write_byte(uint64_t dst, uint8_t arg) const
{
    return write_mem<uint8_t, &::write_byte>(*this, dst, arg);
}

bool memory::Io::write_le16(uint64_t dst, uint16_t arg) const
{
    return write_mem<uint16_t, &::write_le16>(*this, dst, arg);
}

bool memory::Io::write_le32(uint64_t dst, uint32_t arg) const
{
    return write_mem<uint32_t, &::write_le32>(*this, dst, arg);
}

bool memory::Io::write_le64(uint64_t dst, uint64_t arg) const
{
    return write_mem<uint64_t, &::write_le64>(*this, dst, arg);
}

bool memory::Io::write_be16(uint64_t dst, uint16_t arg) const
{
    return write_mem<uint16_t, &::write_be16>(*this, dst, arg);
}

bool memory::Io::write_be32(uint64_t dst, uint32_t arg) const
{
    return write_mem<uint32_t, &::write_be32>(*this, dst, arg);
}

bool memory::Io::write_be64(uint64_t dst, uint64_t arg) const
{
    return write_mem<uint64_t, &::write_be64>(*this, dst, arg);
}

bool memory::Io::write(uint64_t dst, uint64_t arg) const
{
    constexpr auto rfunc = is_little_endian ? ::write_le64 : ::write_be64;
    return write_mem<uint64_t, rfunc>(*this, dst, arg);
}

bool memory::Io::write_all(uint64_t dst, const void* src, size_t size) const
{
    if(proc)
        return memory::write_virtual(core, *proc, dst, src, size);

    return memory::write_virtual_with_dtb(core, dtb, dst, src, size);
}
