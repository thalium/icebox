#include "reader.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "reader"
#include "core.hpp"
#include "core_private.hpp"
#include "endian.hpp"
#include "interfaces/if_os.hpp"
#include "os.hpp"

namespace
{
    dtb_t dtb_select(const reader::Reader& r, uint64_t ptr)
    {
        return os::is_kernel_address(r.core, ptr) ? r.kdtb : r.udtb;
    }

    template <typename T, T (*read)(const void*)>
    opt<T> read_mem(const reader::Reader& r, uint64_t ptr)
    {
        auto value     = T{};
        const auto dtb = dtb_select(r, ptr);
        const auto ok  = memory::read_virtual_with_dtb(r.core, &value, dtb, ptr, sizeof value);
        if(!ok)
            return {};

        return read(&value);
    }

    reader::Reader make_reader_with(core::Core& core, const opt<proc_t>& proc)
    {
        const auto cr3 = registers::read(core, reg_e::cr3);
        auto reader    = reader::Reader{core, {cr3}, {cr3}};
        if(core.os_)
            core.os_->reader_setup(reader, proc);
        return reader;
    }
}

reader::Reader reader::make(core::Core& core)
{
    return make_reader_with(core, {});
}

reader::Reader reader::make(core::Core& core, proc_t proc)
{
    return make_reader_with(core, proc);
}

opt<uint8_t> reader::Reader::byte(uint64_t ptr) const
{
    return read_mem<uint8_t, &::read_byte>(*this, ptr);
}

opt<uint16_t> reader::Reader::le16(uint64_t ptr) const
{
    return read_mem<uint16_t, &::read_le16>(*this, ptr);
}

opt<uint32_t> reader::Reader::le32(uint64_t ptr) const
{
    return read_mem<uint32_t, &::read_le32>(*this, ptr);
}

opt<uint64_t> reader::Reader::le64(uint64_t ptr) const
{
    return read_mem<uint64_t, &::read_le64>(*this, ptr);
}

opt<uint16_t> reader::Reader::be16(uint64_t ptr) const
{
    return read_mem<uint16_t, &::read_be16>(*this, ptr);
}

opt<uint32_t> reader::Reader::be32(uint64_t ptr) const
{
    return read_mem<uint32_t, &::read_be32>(*this, ptr);
}

opt<uint64_t> reader::Reader::be64(uint64_t ptr) const
{
    return read_mem<uint64_t, &::read_be64>(*this, ptr);
}

opt<uint64_t> reader::Reader::read(uint64_t ptr) const
{
    constexpr auto rfunc = is_little_endian ? ::read_le64 : ::read_be64;
    return read_mem<uint64_t, rfunc>(*this, ptr);
}

bool reader::Reader::read_all(void* dst, uint64_t ptr, size_t size) const
{
    const auto dtb = dtb_select(*this, ptr);
    return memory::read_virtual_with_dtb(core, dst, dtb, ptr, size);
}

opt<phy_t> reader::Reader::physical(uint64_t ptr) const
{
    const auto dtb = dtb_select(*this, ptr);
    const auto phy = memory::virtual_to_physical(core, ptr, dtb);
    if(!phy)
        return {};

    return *phy;
}
