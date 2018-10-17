#include "pe.hpp"

#define FDP_MODULE "pe"
#include "log.hpp"
#include "endian.hpp"

opt<size_t> pe::read_image_size(const void* vsrc, size_t size)
{
    const auto src = reinterpret_cast<const uint8_t*>(vsrc);

    const auto e_magic = read_be16(&src[0]);
    static const auto image_dos_signature = 'MZ';
    if(e_magic != image_dos_signature)
        return std::nullopt;

    static const auto e_lfanew_offset = 0x3C;
    int idx = e_lfanew_offset;
    if(idx + 4 > size)
        return std::nullopt;

    const auto e_lfanew = read_le32(&src[idx]);
    idx = e_lfanew;

    static const uint32_t image_nt_signature = 'PE' << 16;
    if(idx + sizeof image_nt_signature > size)
        return std::nullopt;

    const auto signature = read_be32(&src[idx]);
    if(signature != image_nt_signature)
        return std::nullopt;

    static const uint16_t image_file_machine_amd64 = 0x8664;
    idx += sizeof signature;
    if(idx + sizeof image_file_machine_amd64 > size)
        return std::nullopt;

    const auto machine = read_le16(&src[idx]);
    if(machine != image_file_machine_amd64)
        return std::nullopt;

    static const int image_file_header_size = 20;
    static const uint16_t image_nt_optional_hdr64_magic = 0x20B;
    idx += image_file_header_size;
    if(idx + sizeof image_nt_optional_hdr64_magic > size)
        return std::nullopt;

    const auto magic = read_le16(&src[idx]);
    if(magic != image_nt_optional_hdr64_magic)
        return std::nullopt;

    static const auto size_of_image_offset = 14 * 4;
    idx += size_of_image_offset;
    if(idx + 4 > size)
        return std::nullopt;

    return read_le32(&src[idx]);
}
