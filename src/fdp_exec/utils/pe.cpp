#include "pe.hpp"

#define FDP_MODULE "pe"
#include "core.hpp"
#include "endian.hpp"
#include "log.hpp"
#include "reader.hpp"
#include "utils/utils.hpp"

#include <array>

namespace
{
    enum class cat_e
    {
        REQUIRED,
        REQUIRED32,
        OPTIONAL,
    };

    enum member_offset_e
    {
        IMAGE_DOS_HEADER_e_lfanew,
        IMAGE_NT_HEADERS64_FileHeader,
        IMAGE_NT_HEADERS64_OptionalHeader,
        IMAGE_FILE_HEADER_SizeOfOptionalHeader,
        IMAGE_FILE_HEADER_NumberOfSections,
        IMAGE_OPTIONAL_HEADER_DataDirectory,
        IMAGE_OPTIONAL_HEADER64_DataDirectory,
        IMAGE_OPTIONAL_HEADER64_NumberOfRvaAndSizes,
        IMAGE_DATA_DIRECTORY_VirtualAddress,
        IMAGE_DATA_DIRECTORY_Size,
        IMAGE_DEBUG_DIRECTORY_Type,
        IMAGE_DEBUG_DIRECTORY_SizeOfData,
        IMAGE_DEBUG_DIRECTORY_AddressOfRawData,
        IMAGE_SECTION_HEADER_Name,
        IMAGE_SECTION_HEADER_Misc,
        IMAGE_SECTION_HEADER_VirtualAddress,
        MEMBER_OFFSET_COUNT,
    };
    struct MemberOffset
    {
        cat_e           e_cat;
        member_offset_e e_id;
        const char      module[16];
        const char      struc[32];
        const char      member[32];
    };
    // clang-format off
    const MemberOffset g_member_offsets[] =
    {
        {cat_e::REQUIRED,   IMAGE_DOS_HEADER_e_lfanew,                     "nt",     "_IMAGE_DOS_HEADER",            "e_lfanew"},
        {cat_e::REQUIRED,   IMAGE_NT_HEADERS64_FileHeader,                 "nt",     "_IMAGE_NT_HEADERS64",          "FileHeader"},
        {cat_e::REQUIRED,   IMAGE_NT_HEADERS64_OptionalHeader,             "nt",     "_IMAGE_NT_HEADERS64",          "OptionalHeader"},
        {cat_e::REQUIRED,   IMAGE_FILE_HEADER_SizeOfOptionalHeader,        "nt",     "_IMAGE_FILE_HEADER",           "SizeOfOptionalHeader"},
        {cat_e::REQUIRED,   IMAGE_FILE_HEADER_NumberOfSections,            "nt",     "_IMAGE_FILE_HEADER",           "NumberOfSections"},
        {cat_e::REQUIRED32, IMAGE_OPTIONAL_HEADER_DataDirectory,           "wntdll", "_IMAGE_OPTIONAL_HEADER",       "DataDirectory"},
        {cat_e::REQUIRED,   IMAGE_OPTIONAL_HEADER64_DataDirectory,         "nt",     "_IMAGE_OPTIONAL_HEADER64",     "DataDirectory"},
        {cat_e::REQUIRED,   IMAGE_OPTIONAL_HEADER64_NumberOfRvaAndSizes,   "nt",     "_IMAGE_OPTIONAL_HEADER64",     "NumberOfRvaAndSizes"},
        {cat_e::REQUIRED,   IMAGE_DATA_DIRECTORY_VirtualAddress,           "nt",     "_IMAGE_DATA_DIRECTORY",        "VirtualAddress"},
        {cat_e::REQUIRED,   IMAGE_DATA_DIRECTORY_Size,                     "nt",     "_IMAGE_DATA_DIRECTORY",        "Size"},
        {cat_e::REQUIRED,   IMAGE_DEBUG_DIRECTORY_Type,                    "nt",     "_IMAGE_DEBUG_DIRECTORY",       "Type"},
        {cat_e::REQUIRED,   IMAGE_DEBUG_DIRECTORY_SizeOfData,              "nt",     "_IMAGE_DEBUG_DIRECTORY",       "SizeOfData"},
        {cat_e::REQUIRED,   IMAGE_DEBUG_DIRECTORY_AddressOfRawData,        "nt",     "_IMAGE_DEBUG_DIRECTORY",       "AddressOfRawData"},
        {cat_e::REQUIRED,   IMAGE_SECTION_HEADER_Name,                     "nt",     "_IMAGE_SECTION_HEADER",        "Name"},
        {cat_e::REQUIRED,   IMAGE_SECTION_HEADER_Misc,                     "nt",     "_IMAGE_SECTION_HEADER",        "Misc"},
        {cat_e::REQUIRED,   IMAGE_SECTION_HEADER_VirtualAddress,           "nt",     "_IMAGE_SECTION_HEADER",        "VirtualAddress"},
    };
    // clang-format on
    static_assert(COUNT_OF(g_member_offsets) == MEMBER_OFFSET_COUNT, "invalid members");

    enum struc_size_e
    {
        IMAGE_DATA_DIRECTORY,
        IMAGE_DEBUG_DIRECTORY,
        STRUC_SIZE_COUNT,
    };
    struct StrucSize
    {
        struc_size_e e_id;
        const char   module[16];
        const char   struc[32];
    };
    // clang-format off
    const StrucSize g_struc_sizes[] =
    {
        {IMAGE_DATA_DIRECTORY,  "nt",   "_IMAGE_DATA_DIRECTORY"},
        {IMAGE_DEBUG_DIRECTORY, "nt",   "_IMAGE_DEBUG_DIRECTORY"},
    };
    // clang-format on
    static_assert(COUNT_OF(g_struc_sizes) == STRUC_SIZE_COUNT, "invalid struc sizes");

    using MemberOffsets = std::array<uint64_t, MEMBER_OFFSET_COUNT>;
    using StrucSizes    = std::array<uint64_t, STRUC_SIZE_COUNT>;
}

struct pe::Pe::Data
{
    MemberOffsets members_;
    StrucSizes    sizes_;
};

pe::Pe::Pe()
    : d_(std::make_unique<Data>())
{
}

pe::Pe::~Pe()
{
}

bool pe::Pe::setup(core::Core& core)
{
    bool fail = false;
    for(size_t i = 0; i < MEMBER_OFFSET_COUNT; ++i)
    {
        if(g_member_offsets[i].e_cat == cat_e::REQUIRED32)
            continue;

        const auto offset = core.sym.struc_offset(g_member_offsets[i].module, g_member_offsets[i].struc, g_member_offsets[i].member);
        if(!offset)
        {
            fail = true;
            LOG(ERROR, "unable to read {}!{}.{} member offset", g_member_offsets[i].module, g_member_offsets[i].struc, g_member_offsets[i].member);
            continue;
        }
        d_->members_[i] = *offset;
    }
    for(size_t i = 0; i < STRUC_SIZE_COUNT; ++i)
    {
        const auto offset = core.sym.struc_size(g_struc_sizes[i].module, g_struc_sizes[i].struc);
        if(!offset)
        {
            fail = true;
            LOG(ERROR, "unable to read {}!{} struc size", g_struc_sizes[i].module, g_struc_sizes[i].struc);
            continue;
        }
        d_->sizes_[i] = *offset;
    }
    return !fail;
}

bool pe::Pe::setup_wow64(core::Core& core)
{
    bool fail = false;
    for(size_t i = 0; i < MEMBER_OFFSET_COUNT; ++i)
    {
        if(g_member_offsets[i].e_cat != cat_e::REQUIRED32)
            continue;

        const auto offset = core.sym.struc_offset(g_member_offsets[i].module, g_member_offsets[i].struc, g_member_offsets[i].member);
        if(!offset)
        {
            fail = true;
            LOG(ERROR, "unable to read {}!{}.{} member offset", g_member_offsets[i].module, g_member_offsets[i].struc, g_member_offsets[i].member);
            continue;
        }

        d_->members_[i] = *offset;
    }
    return !fail;
}

opt<bool> pe::Pe::is_pe64(const reader::Reader& reader, const uint64_t image_file_header)
{
    const auto machine = reader.le16(image_file_header);
    if(!machine)
        FAIL({}, "unable to read IMAGE_FILE_HEADER.Machine");

    static const uint16_t image_file_machine_amd64 = 0x8664;
    const auto is_pe64                             = *machine == image_file_machine_amd64 ? true : false;

    return is_pe64;
}

opt<span_t> pe::Pe::find_image_directory(const reader::Reader& reader, const span_t span, const image_directory_entry_e id)
{
    const auto e_lfanew = reader.le32(span.addr + d_->members_[IMAGE_DOS_HEADER_e_lfanew]);
    if(!e_lfanew)
        FAIL({}, "unable to read e_lfanew");

    const auto image_nt_header       = span.addr + *e_lfanew;
    const auto image_file_header     = image_nt_header + d_->members_[IMAGE_NT_HEADERS64_FileHeader];
    const auto image_optional_header = image_nt_header + d_->members_[IMAGE_NT_HEADERS64_OptionalHeader];

    const auto pe64 = is_pe64(reader, image_file_header);
    if(!pe64)
        FAIL({}, "unable to read get pe machine type");

    const auto data_directory_offset = *pe64 ? d_->members_[IMAGE_OPTIONAL_HEADER64_DataDirectory] : d_->members_[IMAGE_OPTIONAL_HEADER_DataDirectory];

    const auto size_image_data_directory      = d_->sizes_[IMAGE_DATA_DIRECTORY];
    const auto data_directory                 = image_optional_header + data_directory_offset + size_image_data_directory * id;
    const auto data_directory_virtual_address = reader.le32(data_directory + d_->members_[IMAGE_DATA_DIRECTORY_VirtualAddress]);
    if(!data_directory_virtual_address)
        FAIL({}, "unable to read DataDirectory.VirtualAddress");

    const auto data_directory_size = reader.le32(data_directory + d_->members_[IMAGE_DATA_DIRECTORY_Size]);
    if(!data_directory_size)
        FAIL({}, "unable to read DataDirectory.Size");

    // LOG(INFO, "exception_dir addr {:#x} section size {:#x}", span.addr + *data_directory_virtual_address, *data_directory_size);
    return span_t{span.addr + *data_directory_virtual_address, *data_directory_size};
}

opt<span_t> pe::Pe::find_debug_codeview(const reader::Reader& reader, span_t module)
{
    const auto directory = find_image_directory(reader, module, pe::IMAGE_DIRECTORY_ENTRY_DEBUG);
    if(!directory)
        return {};

    const auto type = reader.le32(directory->addr + d_->members_[IMAGE_DEBUG_DIRECTORY_Type]);
    if(!type)
        return {};

    if(*type != 2)
        FAIL({}, "invalid IMAGE_DEBUG_TYPE, want IMAGE_DEBUG_TYPE_CODEVIEW = 2, got {}", *type);

    const auto addr = reader.le32(directory->addr + d_->members_[IMAGE_DEBUG_DIRECTORY_AddressOfRawData]);
    const auto size = reader.le32(directory->addr + d_->members_[IMAGE_DEBUG_DIRECTORY_SizeOfData]);
    if(!addr || !size)
        return {};

    return span_t{module.addr + *addr, *size};
}

opt<size_t> pe::read_image_size(const void* vsrc, size_t size)
{
    const auto src = reinterpret_cast<const uint8_t*>(vsrc);

    const auto e_magic                    = read_be16(&src[0]);
    static const auto image_dos_signature = 0x4D5A; // MZ
    if(e_magic != image_dos_signature)
        return {};

    static const auto e_lfanew_offset = 0x3C;
    size_t idx                        = e_lfanew_offset;
    if(idx + 4 > size)
        return {};

    const auto e_lfanew = read_le32(&src[idx]);
    idx                 = e_lfanew;

    static const uint32_t image_nt_signature = 0X5045 << 16; // PE
    if(idx + sizeof image_nt_signature > size)
        return {};

    const auto signature = read_be32(&src[idx]);
    if(signature != image_nt_signature)
        return {};

    static const uint16_t image_file_machine_amd64 = 0x8664;
    idx += sizeof signature;
    if(idx + sizeof image_file_machine_amd64 > size)
        return {};

    const auto machine = read_le16(&src[idx]);
    if(machine != image_file_machine_amd64)
        return {};

    static const int image_file_header_size             = 20;
    static const uint16_t image_nt_optional_hdr64_magic = 0x20B;
    idx += image_file_header_size;
    if(idx + sizeof image_nt_optional_hdr64_magic > size)
        return {};

    const auto magic = read_le16(&src[idx]);
    if(magic != image_nt_optional_hdr64_magic)
        return {};

    static const auto size_of_image_offset = 14 * 4;
    idx += size_of_image_offset;
    if(idx + 4 > size)
        return {};

    return read_le32(&src[idx]);
}
