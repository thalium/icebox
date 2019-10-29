#include "nt_objects.hpp"

#define FDP_MODULE "objects_nt"
#include "core.hpp"
#include "endian.hpp"
#include "log.hpp"
#include "os.hpp"
#include "reader.hpp"
#include "utils/utf8.hpp"
#include "utils/utils.hpp"
#include "wow64.hpp"

#include <array>

namespace
{
    enum member_offset_e
    {
        EPROCESS_ObjectTable,
        HANDLE_TABLE_TableCode,
        KPCR_Prcb,
        OBJECT_HEADER_Body,
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
        ObpKernelHandleTable,
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
        {ObpKernelHandleTable,            "nt", "ObpKernelHandleTable"},
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
};

std::shared_ptr<objects::Data> objects::make(core::Core& core, proc_t proc)
{
    const auto ptr = std::make_shared<Data>(core, proc);
    bool fail      = false;
    for(size_t i = 0; i < SYMBOL_OFFSET_COUNT; ++i)
    {
        const auto& sym = g_symbol_offsets[i];
        const auto addr = symbols::symbol(core, symbols::kernel, sym.module, sym.name);
        if(!addr)
        {
            fail = true;
            LOG(INFO, "unable to read %s!%s symbol offset", sym.module, sym.name);
            continue;
        }

        ptr->symbols[i] = *addr;
    }
    for(size_t i = 0; i < MEMBER_OFFSET_COUNT; ++i)
    {
        const auto& mb    = g_member_offsets[i];
        const auto offset = symbols::struc_offset(core, symbols::kernel, mb.module, mb.struc, mb.member);
        if(!offset)
        {
            fail = true;
            LOG(INFO, "unable to read %s!%s.%s member offset", mb.module, mb.struc, mb.member);
            continue;
        }
        ptr->members[i] = *offset;
    }
    if(fail)
        return {};

    return ptr;
}

namespace
{
    using Data = objects::Data;

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
        const auto type_idx           = *encoded_type_idx ^ *header_cookie ^ obj_addr_cookie;
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
