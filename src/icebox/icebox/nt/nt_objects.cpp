#include "nt_objects.hpp"

#define FDP_MODULE "nt_objects"
#include "core.hpp"
#include "endian.hpp"
#include "log.hpp"
#include "os.hpp"
#include "reader.hpp"
#include "utils/utf8.hpp"
#include "utils/utils.hpp"
#include "wow64.hpp"

#include <array>

#ifdef _MSC_VER
#    include <string.h>
#    define strncasecmp _strnicmp
#else
#    include <strings.h>
#endif

namespace
{
    enum member_offset_e
    {
        EPROCESS_ObjectTable,
        HANDLE_TABLE_TableCode,
        KPCR_Prcb,
        OBJECT_HEADER_Body,
        OBJECT_HEADER_InfoMask,
        OBJECT_HEADER_TypeIndex,
        OBJECT_TYPE_Name,
        OBJECT_ATTRIBUTES_ObjectName,
        FILE_OBJECT_FileName,
        FILE_OBJECT_DeviceObject,
        DEVICE_OBJECT_DriverObject,
        DRIVER_OBJECT_DriverName,
        MEMBER_OFFSET_COUNT,
    };
    struct MemberOffset
    {
        member_offset_e e_id;
        const char      module[16];
        const char      struc[32];
        const char      member[32];
    };
    // clang-format off
    const MemberOffset g_member_offsets[] =
    {
        {EPROCESS_ObjectTable,                         "nt", "_EPROCESS",                          "ObjectTable"},
        {HANDLE_TABLE_TableCode,                       "nt", "_HANDLE_TABLE",                      "TableCode"},
        {KPCR_Prcb,                                    "nt", "_KPCR",                              "Prcb"},
        {OBJECT_HEADER_Body,                           "nt", "_OBJECT_HEADER",                     "Body"},
        {OBJECT_HEADER_InfoMask,                       "nt", "_OBJECT_HEADER",                     "InfoMask"},
        {OBJECT_HEADER_TypeIndex,                      "nt", "_OBJECT_HEADER",                     "TypeIndex"},
        {OBJECT_TYPE_Name,                             "nt", "_OBJECT_TYPE",                       "Name"},
        {OBJECT_ATTRIBUTES_ObjectName,                 "nt", "_OBJECT_ATTRIBUTES",                 "ObjectName"},
        {FILE_OBJECT_FileName,                         "nt", "_FILE_OBJECT",                       "FileName"},
        {FILE_OBJECT_DeviceObject,                     "nt", "_FILE_OBJECT",                       "DeviceObject"},
        {DEVICE_OBJECT_DriverObject,                   "nt", "_DEVICE_OBJECT",                     "DriverObject"},
        {DRIVER_OBJECT_DriverName,                     "nt", "_DRIVER_OBJECT",                     "DriverName"},
    };
    // clang-format on
    static_assert(COUNT_OF(g_member_offsets) == MEMBER_OFFSET_COUNT, "invalid members");

    enum symbol_offset_e
    {
        ObpInfoMaskToOffset,
        ObpKernelHandleTable,
        ObpRootDirectoryObject,
        ObTypeIndexTable,
        ObHeaderCookie,
        SYMBOL_OFFSET_COUNT,
    };

    struct SymbolOffset
    {
        symbol_offset_e e_id;
        const char      module[16];
        const char      name[32];
    };
    // clang-format off
    const SymbolOffset g_symbol_offsets[] =
    {
        {ObpInfoMaskToOffset,             "nt", "ObpInfoMaskToOffset"},
        {ObpKernelHandleTable,            "nt", "ObpKernelHandleTable"},
        {ObpRootDirectoryObject,          "nt", "ObpRootDirectoryObject"},
        {ObTypeIndexTable,                "nt", "ObTypeIndexTable"},
        {ObHeaderCookie,                  "nt", "ObHeaderCookie"},
    };
    // clang-format on
    static_assert(COUNT_OF(g_symbol_offsets) == SYMBOL_OFFSET_COUNT, "invalid symbols");

    using MemberOffsets = std::array<uint64_t, MEMBER_OFFSET_COUNT>;
    using SymbolOffsets = std::array<uint64_t, SYMBOL_OFFSET_COUNT>;
}

struct objects::Data
{
    Data(core::Core& core, proc_t proc)
        : core(core)
        , proc(proc)
        , reader(reader::make(core, proc))
    {
    }

    core::Core&    core;
    proc_t         proc;
    reader::Reader reader;
    MemberOffsets  members;
    SymbolOffsets  symbols;
    obj_t          root;
    uint8_t        masks[16];
};

namespace
{
    using Data = objects::Data;

    bool setup(Data& d)
    {
        bool fail = false;
        for(size_t i = 0; i < SYMBOL_OFFSET_COUNT; ++i)
        {
            const auto& sym = g_symbol_offsets[i];
            const auto addr = symbols::address(d.core, symbols::kernel, sym.module, sym.name);
            if(!addr)
            {
                fail = true;
                LOG(INFO, "unable to read %s!%s symbol offset", sym.module, sym.name);
                continue;
            }

            d.symbols[i] = *addr;
        }
        for(size_t i = 0; i < MEMBER_OFFSET_COUNT; ++i)
        {
            const auto& mb    = g_member_offsets[i];
            const auto offset = symbols::struc_offset(d.core, symbols::kernel, mb.module, mb.struc, mb.member);
            if(!offset)
            {
                fail = true;
                LOG(INFO, "unable to read %s!%s.%s member offset", mb.module, mb.struc, mb.member);
                continue;
            }
            d.members[i] = *offset;
        }
        if(fail)
            return false;

        auto ok = d.reader.read_all(&d.root.id, d.symbols[ObpRootDirectoryObject], sizeof d.root.id);
        if(!ok)
            return false;

        return d.reader.read_all(&d.masks, d.symbols[ObpInfoMaskToOffset], sizeof d.masks);
    }
}

std::shared_ptr<objects::Data> objects::make(core::Core& core, proc_t proc)
{
    const auto ptr = std::make_shared<Data>(core, proc);
    const auto ok  = setup(*ptr);
    if(!ok)
        return {};

    return ptr;
}

namespace
{
    static opt<objects::obj_t> object_read(const Data& d, nt::HANDLE handle)
    {
        // Is kernel handle
        const auto handle_table_addr = handle & 0x80000000 ? d.symbols[ObpKernelHandleTable] : d.proc.id + d.members[EPROCESS_ObjectTable];
        if(handle & 0x80000000)
            handle = ((handle << 32) >> 32) & ~0xffffffff80000000;

        const auto handle_table = d.reader.read(handle_table_addr);
        if(!handle_table)
            return FAIL(ext::nullopt, "unable to read handle table");

        auto handle_table_code = d.reader.read(*handle_table + d.members[HANDLE_TABLE_TableCode]);
        if(!handle_table_code)
            return FAIL(ext::nullopt, "unable to read handle table code");

        const auto handle_table_level = *handle_table_code & 3;
        *handle_table_code &= ~3;

        constexpr auto HANDLE_VALUE_INC        = 0x04; // Amount to increment the Value to get to the next handle
        constexpr auto HANDLE_TABLE_ENTRY_SIZE = 0x10;
        constexpr auto PAGE_SIZE               = 4096;
        constexpr auto POINTER_SIZE            = 8;

        uint64_t i, j, k;
        switch(handle_table_level)
        {
            case 0:
                i = handle;
                break;

            case 1:
                i = handle % ((PAGE_SIZE / HANDLE_TABLE_ENTRY_SIZE) * HANDLE_VALUE_INC);
                handle -= i;
                j = handle / (((PAGE_SIZE / HANDLE_TABLE_ENTRY_SIZE) * HANDLE_VALUE_INC) / POINTER_SIZE);

                handle_table_code = d.reader.read(*handle_table_code + j);
                // handle_table_code = *handle_table_code + j;
                break;

            case 2:
                i = handle % ((PAGE_SIZE / HANDLE_TABLE_ENTRY_SIZE) * HANDLE_VALUE_INC);
                handle -= i;
                k = handle / (((PAGE_SIZE / HANDLE_TABLE_ENTRY_SIZE) * HANDLE_VALUE_INC) / POINTER_SIZE);
                j = k % PAGE_SIZE;
                k -= j;
                k /= (PAGE_SIZE / POINTER_SIZE);

                handle_table_code = d.reader.read(*handle_table_code + k);
                handle_table_code = d.reader.read(*handle_table_code + j);
                break;

            default:
                return FAIL(ext::nullopt, "Unknown table level");
        }
        if(!*handle_table_code)
            return {};

        const auto handle_table_entry = d.reader.read(*handle_table_code + i * (HANDLE_TABLE_ENTRY_SIZE / HANDLE_VALUE_INC));
        if(!handle_table_entry)
            return FAIL(ext::nullopt, "unable to read table entry");

        // TODO deal with theses shifts on x32
        uint64_t p                = 0xffff;
        const uint64_t obj_header = (((*handle_table_entry >> 16) | (p << 48)) >> 4) << 4;
        const auto obj_body       = obj_header + d.members[OBJECT_HEADER_Body];
        return objects::obj_t{obj_body};
    }
}

opt<objects::obj_t> objects::read(Data& d, nt::HANDLE handle)
{
    return object_read(d, handle);
}

namespace
{
    opt<std::string> object_type(const Data& d, objects::obj_t obj)
    {
        const auto POINTER_SIZE     = 8;
        const auto obj_header       = obj.id - d.members[OBJECT_HEADER_Body];
        const auto encoded_type_idx = d.reader.byte(obj_header + d.members[OBJECT_HEADER_TypeIndex]);
        if(!encoded_type_idx)
            return FAIL(ext::nullopt, "unable to read encoded type index");

        const auto header_cookie = d.reader.byte(d.symbols[ObHeaderCookie]);
        if(!header_cookie)
            return FAIL(ext::nullopt, "unable to read ObHeaderCookie");

        const uint8_t obj_addr_cookie = ((obj_header >> 8) & 0xff);
        const auto type_idx           = static_cast<size_t>(*encoded_type_idx ^ *header_cookie ^ obj_addr_cookie);
        const auto obj_type           = d.reader.read(d.symbols[ObTypeIndexTable] + type_idx * POINTER_SIZE);
        if(!obj_type)
            return FAIL(ext::nullopt, "unable to read object type");

        return nt::read_unicode_string(d.reader, *obj_type + d.members[OBJECT_TYPE_Name]);
    }
}

opt<std::string> objects::type(Data& d, obj_t obj)
{
    return object_type(d, obj);
}

namespace
{
    opt<std::string> object_name(Data& d, objects::obj_t obj)
    {
        auto info_mask    = uint8_t{};
        const auto header = obj.id - d.members[OBJECT_HEADER_Body];
        auto ok           = d.reader.read_all(&info_mask, header + d.members[OBJECT_HEADER_InfoMask], sizeof info_mask);
        if(!ok)
            return {};

        auto info = nt::_OBJECT_HEADER_NAME_INFO{};
        ok        = d.reader.read_all(&info, header - d.masks[info_mask & 3], sizeof info);
        if(!ok)
            return {};

        return nt::read_unicode_string(d.reader, header - d.masks[info_mask & 3] + offsetof(nt::_OBJECT_HEADER_NAME_INFO, Name));
    }
}

opt<std::string> objects::name(Data& data, obj_t obj)
{
    return object_name(data, obj);
}

opt<objects::file_t> objects::file_read(Data& d, nt::HANDLE handle)
{
    const auto obj = read(d, handle);
    if(!obj)
        return {};

    const auto type = objects::type(d, *obj);
    if(*type != "File")
        return {};

    return file_t{obj->id};
}

opt<std::string> objects::file_name(Data& d, file_t file)
{
    return nt::read_unicode_string(d.reader, file.id + d.members[FILE_OBJECT_FileName]);
}

opt<objects::device_t> objects::file_device(Data& d, file_t file)
{
    const auto dev = d.reader.read(file.id + d.members[FILE_OBJECT_DeviceObject]);
    if(!dev)
        return {};

    return device_t{*dev};
}

opt<objects::driver_t> objects::device_driver(Data& d, device_t device)
{
    const auto drv = d.reader.read(device.id + d.members[DEVICE_OBJECT_DriverObject]);
    if(!drv)
        return {};

    return driver_t{*drv};
}

opt<std::string> objects::driver_name(Data& d, driver_t driver)
{
    return nt::read_unicode_string(d.reader, driver.id + d.members[DRIVER_OBJECT_DriverName]);
}

namespace
{
    std::vector<std::string_view> split(std::string_view arg, std::string_view delimiter)
    {
        auto reply = std::vector<std::string_view>{};
        auto prev  = size_t{0};
        while(prev < arg.size())
        {
            auto end = arg.find(delimiter, prev);
            if(end == std::string::npos)
                end = arg.size();

            const auto token = arg.substr(prev, end - prev);
            if(!token.empty())
                reply.emplace_back(token);

            prev = end + delimiter.size();
        }
        return reply;
    }

    static uint32_t hash_name(std::string_view src)
    {
        const auto wname = utf8::to_utf16(src);
        auto hash        = uint32_t{0};
        auto ptr         = wname.data();
        auto size        = wname.size();
        auto chunk_hash  = uint64_t{0};
        auto chunk_idx   = size_t{0};
        for(/**/; chunk_idx + 4 <= size; chunk_idx += 4)
        {
            auto chunk = read_le64(&ptr[chunk_idx]);
            // broken on non-ascii
            chunk &= ~0x0020002000200020;
            chunk_hash += (chunk_hash << 1) + (chunk_hash >> 1);
            chunk_hash += chunk;
        }
        hash = static_cast<uint32_t>(chunk_hash + (chunk_hash >> 32));
        for(auto i = chunk_idx; i < size; ++i)
        {
            const auto wchar = ptr[i];
            hash += (hash << 1) + (hash >> 1);
            if(wchar < 'a')
                hash += wchar;
            else if(wchar > 'z')
                hash += towupper(wchar);
            else
                hash += wchar - ('a' - 'A');
        }
        return hash;
    }

    opt<nt::_OBJECT_DIRECTORY_ENTRY> find_object_in(Data& d, const nt::_OBJECT_DIRECTORY& root, std::string_view name)
    {
        const auto hash   = hash_name(name);
        const auto bucket = hash % std::size(root.HashBuckets);
        auto entry        = nt::_OBJECT_DIRECTORY_ENTRY{};
        entry.ChainLink   = root.HashBuckets[bucket];
        while(entry.ChainLink)
        {
            const auto ok = d.reader.read_all(&entry, entry.ChainLink, sizeof entry);
            if(!ok)
                return {};

            if(entry.HashValue != hash)
                continue;

            const auto obj_name = object_name(d, objects::obj_t{entry.Object});
            if(!obj_name)
                continue;

            if(obj_name->size() != name.size())
                continue;

            if(strncasecmp(obj_name->data(), name.data(), name.size()) != 0)
                continue;

            return entry;
        }
        return {};
    }
}

opt<objects::obj_t> objects::find(Data& d, nt::ptr_t root, std::string_view path)
{
    const auto components = split(path, "\\");
    if(!root)
        root = d.root.id;

    auto entry = nt::_OBJECT_DIRECTORY_ENTRY{};
    for(const auto& token : components)
    {
        auto directory = nt::_OBJECT_DIRECTORY{};
        const auto ok  = d.reader.read_all(&directory, root, sizeof directory);
        if(!ok)
            return {};

        const auto opt_entry = find_object_in(d, directory, token);
        if(!opt_entry)
            return {};

        entry = *opt_entry;
        root  = entry.Object;
    }
    return obj_t{entry.Object};
}

opt<objects::section_t> objects::as_section(obj_t obj)
{
    // TODO check object type?
    return section_t{obj.id};
}

namespace
{
    template <typename T>
    opt<T> read_as(Data& d, uint64_t id)
    {
        auto struc    = T{};
        const auto ok = d.reader.read_all(&struc, id, sizeof struc);
        if(!ok)
            return {};

        return struc;
    }
}

opt<objects::control_area_t> objects::section_control_area(Data& d, section_t section)
{
    const auto sec = read_as<nt::_SECTION>(d, section.id);
    if(!sec)
        return {};

    return control_area_t{sec->u.ControlArea};
}

opt<objects::segment_t> objects::control_area_segment(Data& d, control_area_t control_area)
{
    const auto ctl_area = read_as<nt::_CONTROL_AREA>(d, control_area.id);
    if(!ctl_area)
        return {};

    return segment_t{ctl_area->Segment};
}

opt<span_t> objects::segment_span(Data& d, segment_t segment)
{
    const auto seg = read_as<nt::_SEGMENT>(d, segment.id);
    if(!seg)
        return {};

    return span_t{seg->u.BasedAddress, seg->SizeOfSegment};
}
