#include "reader.hpp"

#include "core.hpp"
#include "endian.hpp"
#include "os.hpp"

namespace
{
    template <typename T, T (*read)(const void*)>
    static opt<T> read_mem(const reader::Reader& r, uint64_t ptr)
    {
        T value;
        const auto dtb = os::is_kernel_address(r.core_, ptr) ? r.kdtb_ : r.udtb_;
        const auto ok  = memory::read_virtual_with_dtb(r.core_, &value, dtb, ptr, sizeof value);
        if(!ok)
            return {};

        return read(&value);
    }

    static reader::Reader make_reader_with(core::Core& core, const opt<proc_t>& proc)
    {
        const auto cr3 = registers::read(core, reg_e::cr3);
        auto reader    = reader::Reader{core, {cr3}, {cr3}};
        os::reader_setup(core, reader, proc);
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
    if constexpr(is_little_endian)
        return read_mem<uint64_t, ::read_le64>(*this, ptr);
    else
        return read_mem<uint64_t, ::read_be64>(*this, ptr);
}

bool reader::Reader::read(void* dst, uint64_t ptr, size_t size) const
{
    const auto dtb = os::is_kernel_address(core_, ptr) ? kdtb_ : udtb_;
    return memory::read_virtual_with_dtb(core_, dst, dtb, ptr, size);
}

opt<phy_t> reader::Reader::physical(uint64_t ptr) const
{
    const auto dtb = os::is_kernel_address(core_, ptr) ? kdtb_ : udtb_;
    const auto phy = memory::virtual_to_physical(core_, ptr, dtb);
    if(!phy)
        return {};

    return *phy;
}
