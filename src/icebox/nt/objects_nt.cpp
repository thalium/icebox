#include "objects_nt.hpp"

#define FDP_MODULE "objects_nt"
#include "log.hpp"

#include "endian.hpp"
#include "os.hpp"
#include "reader.hpp"
#include "utils/utf8.hpp"
#include "utils/utils.hpp"

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

struct nt::ObjectNt::Data
{
    Data(core::Core& core)
        : core_(core)
    {
    }

    core::Core&   core_;
    MemberOffsets members_;
    SymbolOffsets symbols_;
};

using Data = nt::ObjectNt::Data;

nt::ObjectNt::ObjectNt(core::Core& core)
    : d_(std::make_unique<Data>(core))
{
}

nt::ObjectNt::~ObjectNt() = default;

namespace
{
    bool setup(Data& d)
    {
        bool fail = false;
        for(size_t i = 0; i < SYMBOL_OFFSET_COUNT; ++i)
        {
            const auto addr = d.core_.sym.symbol(g_symbol_offsets[i].module, g_symbol_offsets[i].name);
            if(!addr)
            {
                fail = true;
                LOG(INFO, "unable to read {}!{} symbol offset", g_symbol_offsets[i].module, g_symbol_offsets[i].name);
                continue;
            }

            d.symbols_[i] = *addr;
        }
        for(size_t i = 0; i < MEMBER_OFFSET_COUNT; ++i)
        {
            const auto offset = d.core_.sym.struc_offset(g_member_offsets[i].module, g_member_offsets[i].struc, g_member_offsets[i].member);
            if(!offset)
            {
                fail = true;
                LOG(INFO, "unable to read {}!{}.{} member offset", g_member_offsets[i].module, g_member_offsets[i].struc, g_member_offsets[i].member);
                continue;
            }
            d.members_[i] = *offset;
        }
        return !fail;
    }
}

bool nt::ObjectNt::setup()
{
    return ::setup(*d_);
}

std::shared_ptr<nt::ObjectNt> nt::make_objectnt(core::Core& core)
{
    auto obj_nt = std::make_shared<nt::ObjectNt>(core);
    if(!obj_nt)
        return nullptr;

    const auto ok = obj_nt->setup();
    if(!ok)
        return nullptr;

    return obj_nt;
}

opt<nt::obj_t> nt::ObjectNt::get_object_ref(proc_t proc, nt::HANDLE handle)
{
    // Is kernel handle
    const auto reader            = reader::make(d_->core_, proc);
    const auto handle_table_addr = (handle & 0x80000000) ? d_->symbols_[ObpKernelHandleTable] : proc.id + d_->members_[EPROCESS_ObjectTable];
    if(handle & 0x80000000)
        handle = (((handle << 32) >> 32) & ~0xffffffff80000000);

    const auto handle_table = reader.read(handle_table_addr);
    if(!handle_table)
        return FAIL(ext::nullopt, "unable to read handle table");

    auto handle_table_code = reader.read(*handle_table + d_->members_[HANDLE_TABLE_TableCode]);
    if(!handle_table_code)
        return FAIL(ext::nullopt, "unable to read handle table code");

    const auto handle_table_level = *handle_table_code & 3;
    *handle_table_code &= ~3;

    const auto HANDLE_VALUE_INC        = 0x04; // Amount to increment the Value to get to the next handle
    const auto HANDLE_TABLE_ENTRY_SIZE = 0x10;
    const auto PAGE_SIZE               = 4096;
    const auto POINTER_SIZE            = 8;

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

            handle_table_code = reader.read(*handle_table_code + j);
            // handle_table_code = *handle_table_code + j;
            break;

        case 2:
            i = handle % ((PAGE_SIZE / HANDLE_TABLE_ENTRY_SIZE) * HANDLE_VALUE_INC);
            handle -= i;
            k = handle / (((PAGE_SIZE / HANDLE_TABLE_ENTRY_SIZE) * HANDLE_VALUE_INC) / POINTER_SIZE);
            j = k % PAGE_SIZE;
            k -= j;
            k /= (PAGE_SIZE / POINTER_SIZE);

            handle_table_code = reader.read(*handle_table_code + k);
            handle_table_code = reader.read(*handle_table_code + j);
            break;

        default:
            return FAIL(ext::nullopt, "Unknown table level");
    }

    const auto handle_table_entry = reader.read(*handle_table_code + i * (HANDLE_TABLE_ENTRY_SIZE / HANDLE_VALUE_INC));
    if(!handle_table_entry)
        return FAIL(ext::nullopt, "unable to read table entry");

    // TODO deal with theses shifts on x32
    uint64_t p                = 0xffff;
    const uint64_t obj_header = (((*handle_table_entry >> 16) | (p << 48)) >> 4) << 4;

    const auto obj_body = obj_header + d_->members_[OBJECT_HEADER_Body];
    return obj_t{obj_body};
}

namespace
{
    opt<std::string> read_unicode_string(const reader::Reader& reader, uint64_t unicode_string)
    {
        using UnicodeString = struct
        {
            uint16_t length;
            uint16_t max_length;
            uint32_t _; // padding
            uint64_t buffer;
        };
        UnicodeString us;
        auto ok = reader.read(&us, unicode_string, sizeof us);
        if(!ok)
            return FAIL(ext::nullopt, "unable to read UNICODE_STRING");

        us.length     = read_le16(&us.length);
        us.max_length = read_le16(&us.max_length);
        us.buffer     = read_le64(&us.buffer);

        if(us.length > us.max_length)
            return FAIL(ext::nullopt, "corrupted UNICODE_STRING");

        std::vector<uint8_t> buffer(us.length);
        ok = reader.read(&buffer[0], us.buffer, us.length);
        if(!ok)
            return FAIL(ext::nullopt, "unable to read UNICODE_STRING.buffer");

        const auto p = &buffer[0];
        return utf8::convert(p, &p[us.length]);
    }

    namespace nt32
    {
        opt<std::string> read_unicode_string(const reader::Reader& reader, uint64_t unicode_string)
        {
            using UnicodeString = struct
            {
                uint16_t length;
                uint16_t max_length;
                uint32_t buffer;
            };
            UnicodeString us;
            auto ok = reader.read(&us, unicode_string, sizeof us);
            if(!ok)
                return FAIL(ext::nullopt, "unable to read UNICODE_STRING");

            us.length     = read_le16(&us.length);
            us.max_length = read_le16(&us.max_length);
            us.buffer     = read_le32(&us.buffer);

            if(us.length > us.max_length)
                return FAIL(ext::nullopt, "corrupted UNICODE_STRING");

            std::vector<uint8_t> buffer(us.length);
            ok = reader.read(&buffer[0], us.buffer, us.length);
            if(!ok)
                return FAIL(ext::nullopt, "unable to read UNICODE_STRING.buffer");

            const auto p = &buffer[0];
            return utf8::convert(p, &p[us.length]);
        }
    } // namespace nt32
}

opt<std::string> nt::ObjectNt::obj_typename(proc_t proc, nt::obj_t obj)
{
    const auto POINTER_SIZE     = 8;
    const auto reader           = reader::make(d_->core_, proc);
    const auto obj_header       = obj.id - d_->members_[OBJECT_HEADER_Body];
    const auto encoded_type_idx = reader.byte(obj_header + d_->members_[OBJECT_HEADER_TypeIndex]);
    if(!encoded_type_idx)
        return FAIL(ext::nullopt, "unable to read encoded type index");

    const auto header_cookie = reader.byte(d_->symbols_[ObHeaderCookie]);
    if(!header_cookie)
        return FAIL(ext::nullopt, "unable to read ObHeaderCookie");

    const uint8_t obj_addr_cookie = ((obj_header >> 8) & 0xff);
    const auto type_idx           = *encoded_type_idx ^ *header_cookie ^ obj_addr_cookie;

    const auto obj_type = reader.read(d_->symbols_[ObTypeIndexTable] + type_idx * POINTER_SIZE);
    if(!obj_type)
        return FAIL(ext::nullopt, "unable to read object type");

    return read_unicode_string(reader, *obj_type + d_->members_[OBJECT_TYPE_Name]);
}

opt<std::string> nt::ObjectNt::fileobj_filename(proc_t proc, nt::obj_t obj)
{
    const auto reader = reader::make(d_->core_, proc);
    return read_unicode_string(reader, obj.id + d_->members_[FILE_OBJECT_FileName]);
}

opt<nt::obj_t> nt::ObjectNt::fileobj_deviceobject(proc_t proc, nt::obj_t obj)
{
    const auto reader     = reader::make(d_->core_, proc);
    const auto device_obj = reader.read(obj.id + d_->members_[FILE_OBJECT_DeviceObject]);
    if(!device_obj)
        return {};

    return obj_t{*device_obj};
}

opt<nt::obj_t> nt::ObjectNt::deviceobj_driverobject(proc_t proc, nt::obj_t obj)
{
    const auto reader     = reader::make(d_->core_, proc);
    const auto driver_obj = reader.read(obj.id + d_->members_[DEVICE_OBJECT_DriverObject]);
    if(!driver_obj)
        return {};

    return obj_t{*driver_obj};
}

opt<std::string> nt::ObjectNt::driverobj_drivername(proc_t proc, nt::obj_t obj)
{
    const auto reader = reader::make(d_->core_, proc);
    return read_unicode_string(reader, obj.id + d_->members_[DRIVER_OBJECT_DriverName]);
}

opt<std::string> nt::ObjectNt::objattribute_objectname(proc_t proc, uint64_t ptr)
{
    const auto reader   = reader::make(d_->core_, proc);
    const auto cs       = d_->core_.regs.read(FDP_CS_REGISTER);
    const auto is_32bit = cs & 0x23;
    if(is_32bit)
    {
        // TODO get offset in 32 bits ntdll version's pdb
        const auto obj_name = reader.le32(ptr + 8);
        if(!obj_name)
            return {};

        return nt32::read_unicode_string(reader, *obj_name);
    }

    const auto obj_name = reader.read(ptr + d_->members_[OBJECT_ATTRIBUTES_ObjectName]);
    if(!obj_name)
        return {};

    return read_unicode_string(reader, *obj_name);
}
