#include "pe.hpp"

#define FDP_MODULE "pe"
#include "core.hpp"
#include "endian.hpp"
#include "log.hpp"
#include "utils/utils.hpp"

#include <array>

namespace nt
{
    struct IMAGE_DOS_HEADER
    {
        uint16_t e_magic;
        uint16_t e_cblp;
        uint16_t e_cp;
        uint16_t e_crlc;
        uint16_t e_cparhdr;
        uint16_t e_minalloc;
        uint16_t e_maxalloc;
        uint16_t e_ss;
        uint16_t e_sp;
        uint16_t e_csum;
        uint16_t e_ip;
        uint16_t e_cs;
        uint16_t e_lfarlc;
        uint16_t e_ovno;
        uint16_t e_res[4];
        uint16_t e_oemid;
        uint16_t e_oeminfo;
        uint16_t e_res2[10];
        uint32_t e_lfanew;
    };
    STATIC_ASSERT_EQ(64, sizeof(IMAGE_DOS_HEADER));

    struct IMAGE_FILE_HEADER
    {
        uint16_t Machine;
        uint16_t NumberOfSections;
        uint32_t TimeDateStamp;
        uint32_t PointerToSymbolTable;
        uint32_t NumberOfSymbols;
        uint16_t SizeOfOptionalHeader;
        uint16_t Characteristics;
    };
    STATIC_ASSERT_EQ(20, sizeof(IMAGE_FILE_HEADER));

    struct IMAGE_DATA_DIRECTORY
    {
        uint32_t VirtualAddress;
        uint32_t Size;
    };
    STATIC_ASSERT_EQ(8, sizeof(IMAGE_DATA_DIRECTORY));

    struct IMAGE_OPTIONAL_HEADER32
    {
        uint16_t             Magic;
        uint8_t              MajorLinkerVersion;
        uint8_t              MinorLinkerVersion;
        uint32_t             SizeOfCode;
        uint32_t             SizeOfInitializedData;
        uint32_t             SizeOfUninitializedData;
        uint32_t             AddressOfEntryPoint;
        uint32_t             BaseOfCode;
        uint32_t             BaseOfData;
        uint32_t             ImageBase;
        uint32_t             SectionAlignment;
        uint32_t             FileAlignment;
        uint16_t             MajorOperatingSystemVersion;
        uint16_t             MinorOperatingSystemVersion;
        uint16_t             MajorImageVersion;
        uint16_t             MinorImageVersion;
        uint16_t             MajorSubsystemVersion;
        uint16_t             MinorSubsystemVersion;
        uint32_t             Win32VersionValue;
        uint32_t             SizeOfImage;
        uint32_t             SizeOfHeaders;
        uint32_t             CheckSum;
        uint16_t             Subsystem;
        uint16_t             DllCharacteristics;
        uint32_t             SizeOfStackReserve;
        uint32_t             SizeOfStackCommit;
        uint32_t             SizeOfHeapReserve;
        uint32_t             SizeOfHeapCommit;
        uint32_t             LoaderFlags;
        uint32_t             NumberOfRvaAndSizes;
        IMAGE_DATA_DIRECTORY DataDirectory[16];
    };
    STATIC_ASSERT_EQ(224, sizeof(IMAGE_OPTIONAL_HEADER32));

    struct IMAGE_OPTIONAL_HEADER64
    {
        uint16_t             Magic;
        uint8_t              MajorLinkerVersion;
        uint8_t              MinorLinkerVersion;
        uint32_t             SizeOfCode;
        uint32_t             SizeOfInitializedData;
        uint32_t             SizeOfUninitializedData;
        uint32_t             AddressOfEntryPoint;
        uint32_t             BaseOfCode;
        uint64_t             ImageBase;
        uint32_t             SectionAlignment;
        uint32_t             FileAlignment;
        uint16_t             MajorOperatingSystemVersion;
        uint16_t             MinorOperatingSystemVersion;
        uint16_t             MajorImageVersion;
        uint16_t             MinorImageVersion;
        uint16_t             MajorSubsystemVersion;
        uint16_t             MinorSubsystemVersion;
        uint32_t             Win32VersionValue;
        uint32_t             SizeOfImage;
        uint32_t             SizeOfHeaders;
        uint32_t             CheckSum;
        uint16_t             Subsystem;
        uint16_t             DllCharacteristics;
        uint64_t             SizeOfStackReserve;
        uint64_t             SizeOfStackCommit;
        uint64_t             SizeOfHeapReserve;
        uint64_t             SizeOfHeapCommit;
        uint32_t             LoaderFlags;
        uint32_t             NumberOfRvaAndSizes;
        IMAGE_DATA_DIRECTORY DataDirectory[16];
    };
    STATIC_ASSERT_EQ(240, sizeof(IMAGE_OPTIONAL_HEADER64));

    struct IMAGE_NT_HEADERS64
    {
        uint32_t                Signature;
        IMAGE_FILE_HEADER       FileHeader;
        IMAGE_OPTIONAL_HEADER64 OptionalHeader;
    };
    STATIC_ASSERT_EQ(264, sizeof(IMAGE_NT_HEADERS64));

    struct IMAGE_DEBUG_DIRECTORY
    {
        uint32_t Characteristics;
        uint32_t TimeDateStamp;
        uint16_t MajorVersion;
        uint16_t MinorVersion;
        uint32_t Type;
        uint32_t SizeOfData;
        uint32_t AddressOfRawData;
        uint32_t PointerToRawData;
    };
    STATIC_ASSERT_EQ(28, sizeof(IMAGE_DEBUG_DIRECTORY));

    struct IMAGE_SECTION_HEADER
    {
        uint8_t Name[8];
        union
        {
            uint32_t PhysicalAddress;
            uint32_t VirtualSize;
        } Misc;
        uint32_t VirtualAddress;
        uint32_t SizeOfRawData;
        uint32_t PointerToRawData;
        uint32_t PointerToRelocations;
        uint32_t PointerToLinenumbers;
        uint16_t NumberOfRelocations;
        uint16_t NumberOfLinenumbers;
        uint32_t Characteristics;
    };
    STATIC_ASSERT_EQ(40, sizeof(IMAGE_SECTION_HEADER));

    constexpr auto image_file_machine_amd64      = uint16_t(0x8664);
    constexpr auto image_dos_signature           = uint16_t(0x4D5A);       // MZ
    constexpr auto image_nt_signature            = uint32_t(0X5045) << 16; // PE
    constexpr auto image_nt_optional_hdr64_magic = uint16_t(0x20B);

} // namespace nt

opt<bool> pe::is_pe64(const memory::Io& io, const uint64_t image_file_header)
{
    const auto machine = io.le16(image_file_header + offsetof(nt::IMAGE_FILE_HEADER, Machine));
    if(!machine)
        return FAIL(std::nullopt, "unable to read IMAGE_FILE_HEADER.Machine");

    return *machine == nt::image_file_machine_amd64;
}

opt<span_t> pe::find_image_directory(const memory::Io& io, const span_t span, const image_directory_entry_e id)
{
    const auto e_lfanew = io.le32(span.addr + offsetof(nt::IMAGE_DOS_HEADER, e_lfanew));
    if(!e_lfanew)
        return FAIL(std::nullopt, "unable to read e_lfanew");

    const auto image_nt_header       = span.addr + *e_lfanew;
    const auto image_file_header     = image_nt_header + offsetof(nt::IMAGE_NT_HEADERS64, FileHeader);
    const auto image_optional_header = image_nt_header + offsetof(nt::IMAGE_NT_HEADERS64, OptionalHeader);

    const auto pe64 = is_pe64(io, image_file_header);
    if(!pe64)
        return FAIL(std::nullopt, "unable to read get pe machine type");

    const auto offset          = *pe64 ? offsetof(nt::IMAGE_OPTIONAL_HEADER64, DataDirectory) : offsetof(nt::IMAGE_OPTIONAL_HEADER32, DataDirectory);
    const auto data_directory  = image_optional_header + offset + id * sizeof(nt::IMAGE_DATA_DIRECTORY);
    const auto virtual_address = io.le32(data_directory + offsetof(nt::IMAGE_DATA_DIRECTORY, VirtualAddress));
    if(!virtual_address || !*virtual_address)
        return FAIL(std::nullopt, "unable to read DataDirectory.VirtualAddress");

    const auto size = io.le32(data_directory + offsetof(nt::IMAGE_DATA_DIRECTORY, Size));
    if(!size)
        return FAIL(std::nullopt, "unable to read DataDirectory.Size");

    // LOG(INFO, "exception_dir addr {:#x} section size {:#x}", span.addr + *data_directory_virtual_address, *data_directory_size);
    return span_t{span.addr + *virtual_address, *size};
}

opt<span_t> pe::find_debug_codeview(const memory::Io& io, span_t span)
{
    const auto directory = find_image_directory(io, span, pe::IMAGE_DIRECTORY_ENTRY_DEBUG);
    if(!directory)
        return {};

    const auto type = io.le32(directory->addr + offsetof(nt::IMAGE_DEBUG_DIRECTORY, Type));
    if(!type)
        return {};

    if(*type != 2)
        return FAIL(std::nullopt, "invalid IMAGE_DEBUG_TYPE, want IMAGE_DEBUG_TYPE_CODEVIEW = 2, got %d", *type);

    const auto addr = io.le32(directory->addr + offsetof(nt::IMAGE_DEBUG_DIRECTORY, AddressOfRawData));
    const auto size = io.le32(directory->addr + offsetof(nt::IMAGE_DEBUG_DIRECTORY, SizeOfData));
    if(!addr || !size)
        return {};

    return span_t{span.addr + *addr, *size};
}

opt<size_t> pe::read_image_size(const void* vsrc, size_t size)
{
    const auto src = reinterpret_cast<const uint8_t*>(vsrc);

    const auto e_magic = read_be16(&src[0]);
    if(e_magic != nt::image_dos_signature)
        return {};

    size_t idx = offsetof(nt::IMAGE_DOS_HEADER, e_lfanew);
    if(idx + 4 > size)
        return {};

    const auto e_lfanew = read_le32(&src[idx]);
    idx                 = e_lfanew;
    if(idx + sizeof nt::image_nt_signature > size)
        return {};

    const auto signature = read_be32(&src[idx]);
    if(signature != nt::image_nt_signature)
        return {};

    idx += sizeof signature;
    if(idx + sizeof nt::image_file_machine_amd64 > size)
        return {};

    const auto machine = read_le16(&src[idx]);
    if(machine != nt::image_file_machine_amd64)
        return {};

    idx += sizeof(nt::IMAGE_FILE_HEADER);
    if(idx + sizeof nt::image_nt_optional_hdr64_magic > size)
        return {};

    const auto magic = read_le16(&src[idx]);
    if(magic != nt::image_nt_optional_hdr64_magic)
        return {};

    static const auto size_of_image_offset = offsetof(nt::IMAGE_OPTIONAL_HEADER64, SizeOfImage);
    idx += size_of_image_offset;
    if(idx + 4 > size)
        return {};

    return read_le32(&src[idx]);
}

#ifdef _MSC_VER
#    include <windows.h>
STATIC_ASSERT_EQ(sizeof(IMAGE_DOS_HEADER), sizeof(nt::IMAGE_DOS_HEADER));
STATIC_ASSERT_EQ(sizeof(IMAGE_FILE_HEADER), sizeof(nt::IMAGE_FILE_HEADER));
STATIC_ASSERT_EQ(sizeof(IMAGE_DATA_DIRECTORY), sizeof(nt::IMAGE_DATA_DIRECTORY));
STATIC_ASSERT_EQ(sizeof(IMAGE_OPTIONAL_HEADER32), sizeof(nt::IMAGE_OPTIONAL_HEADER32));
STATIC_ASSERT_EQ(sizeof(IMAGE_OPTIONAL_HEADER64), sizeof(nt::IMAGE_OPTIONAL_HEADER64));
STATIC_ASSERT_EQ(sizeof(IMAGE_NT_HEADERS64), sizeof(nt::IMAGE_NT_HEADERS64));
STATIC_ASSERT_EQ(sizeof(IMAGE_DEBUG_DIRECTORY), sizeof(nt::IMAGE_DEBUG_DIRECTORY));
STATIC_ASSERT_EQ(sizeof(IMAGE_SECTION_HEADER), sizeof(nt::IMAGE_SECTION_HEADER));
#endif
