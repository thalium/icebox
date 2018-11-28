#include "objects_nt.hpp"

#define FDP_MODULE "objects_nt"
#include "log.hpp"

#include "core/helpers.hpp"
#include "utils/utf8.hpp"
#include "utils/utils.hpp"

#include <array>

namespace
{
    enum member_obj_offset_e
    {
        EPROCESS_ObjectTable,
        HANDLE_TABLE_TableCode,
        OBJECT_HEADER_Body,
        OBJECT_HEADER_TypeIndex,
        OBJECT_TYPE_Name,
        FILE_OBJECT_FileName,
        FILE_OBJECT_DeviceObject,
        DEVICE_OBJECT_DriverObject,
        DRIVER_OBJECT_DriverName,
        MEMBER_OBJ_OFFSET_COUNT,
    };
    struct MemberObjOffset
    {
        member_obj_offset_e e_id;
        const char          module[16];
        const char          struc[32];
        const char          member[32];
    };
    // clang-format off
    const MemberObjOffset g_member_obj_offsets[] =
    {
        {EPROCESS_ObjectTable,                         "nt", "_EPROCESS",                          "ObjectTable"},
        {HANDLE_TABLE_TableCode,                       "nt", "_HANDLE_TABLE",                      "TableCode"},
        {OBJECT_HEADER_Body,                           "nt", "_OBJECT_HEADER",                     "Body"},
        {OBJECT_HEADER_TypeIndex,                      "nt", "_OBJECT_HEADER",                     "TypeIndex"},
        {OBJECT_TYPE_Name,                             "nt", "_OBJECT_TYPE",                       "Name"},
        {FILE_OBJECT_FileName,                         "nt", "_FILE_OBJECT",                       "FileName"},
        {FILE_OBJECT_DeviceObject,                     "nt", "_FILE_OBJECT",                       "DeviceObject"},
        {DEVICE_OBJECT_DriverObject,                   "nt", "_DEVICE_OBJECT",                     "DriverObject"},
        {DRIVER_OBJECT_DriverName,                     "nt", "_DRIVER_OBJECT",                     "DriverName"},
    };
    // clang-format on
    static_assert(COUNT_OF(g_member_obj_offsets) == MEMBER_OBJ_OFFSET_COUNT, "invalid members");

    enum symbol_obj_offset_e
    {
        ObpKernelHandleTable,
        ObTypeIndexTable,
        ObHeaderCookie,
        SYMBOL_OBJ_OFFSET_COUNT,
    };

    struct SymbolObjOffset
    {
        symbol_obj_offset_e e_id;
        const char          module[16];
        const char          name[32];
    };
    // clang-format off
    const SymbolObjOffset g_symbol_obj_offsets[] =
    {
        {ObpKernelHandleTable,            "nt", "ObpKernelHandleTable"},
        {ObTypeIndexTable,                "nt", "ObTypeIndexTable"},
        {ObHeaderCookie,                  "nt", "ObHeaderCookie"},
    };
    // clang-format on
    static_assert(COUNT_OF(g_symbol_obj_offsets) == SYMBOL_OBJ_OFFSET_COUNT, "invalid symbols");

    using MemberObjOffsets = std::array<uint64_t, MEMBER_OBJ_OFFSET_COUNT>;
    using SymbolObjOffsets = std::array<uint64_t, SYMBOL_OBJ_OFFSET_COUNT>;
}

struct nt::ObjectNt::Data
{
    MemberObjOffsets members_obj_;
    SymbolObjOffsets symbols_obj_;
};

nt::ObjectNt::ObjectNt(core::Core& core)
    : d_(std::make_unique<Data>())
    , core_(core)
{
}

nt::ObjectNt::~ObjectNt()
{
}

bool nt::ObjectNt::setup()
{
    bool fail = false;
    for(size_t i = 0; i < SYMBOL_OBJ_OFFSET_COUNT; ++i)
    {
        const auto addr = core_.sym.symbol(g_symbol_obj_offsets[i].module, g_symbol_obj_offsets[i].name);
        if(!addr)
        {
            fail = true;
            LOG(INFO, "unable to read %s!%s symbol offset", g_symbol_obj_offsets[i].module, g_symbol_obj_offsets[i].name);
            continue;
        }

        d_->symbols_obj_[i] = *addr;
    }
    for(size_t i = 0; i < MEMBER_OBJ_OFFSET_COUNT; ++i)
    {
        const auto offset = core_.sym.struc_offset(g_member_obj_offsets[i].module, g_member_obj_offsets[i].struc, g_member_obj_offsets[i].member);
        if(!offset)
        {
            fail = true;
            LOG(INFO, "unable to read %s!%s.%s member offset", g_member_obj_offsets[i].module, g_member_obj_offsets[i].struc, g_member_obj_offsets[i].member);
            continue;
        }
        d_->members_obj_[i] = *offset;
    }

    if(fail)
        return false;

    return true;
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
    const auto handle_table_addr = (handle & 0x80000000) ? d_->symbols_obj_[ObpKernelHandleTable] : proc.id + d_->members_obj_[EPROCESS_ObjectTable];
    if(handle & 0x80000000)
        handle = (((handle << 32) >> 32) & ~0xffffffff80000000);

    const auto handle_table = core::read_ptr(core_, handle_table_addr);
    if(!handle_table)
        FAIL({}, "Unable to read handle table");

    auto handle_table_code = core::read_ptr(core_, *handle_table + d_->members_obj_[HANDLE_TABLE_TableCode]);
    if(!handle_table_code)
        FAIL({}, "Unable to read handle table code");

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
            j = handle / ((PAGE_SIZE / HANDLE_TABLE_ENTRY_SIZE) * HANDLE_VALUE_INC) / POINTER_SIZE;

            handle_table_code = core::read_ptr(core_, *handle_table_code + j);
            break;

        case 2:
            i = handle % ((PAGE_SIZE / HANDLE_TABLE_ENTRY_SIZE) * HANDLE_VALUE_INC);
            handle -= i;
            k = handle / ((PAGE_SIZE / HANDLE_TABLE_ENTRY_SIZE) * HANDLE_VALUE_INC) / POINTER_SIZE;
            j = k % PAGE_SIZE;
            k -= j;
            k /= (PAGE_SIZE / POINTER_SIZE);

            handle_table_code = core::read_ptr(core_, *handle_table_code + k);
            handle_table_code = core::read_ptr(core_, *handle_table_code + j);
            break;

        default:
            FAIL({}, "Unknown table level");
    }

    const auto handle_table_entry = core::read_ptr(core_, *handle_table_code + handle * (HANDLE_TABLE_ENTRY_SIZE / HANDLE_VALUE_INC));
    if(!handle_table_entry)
        FAIL({}, "Unable to read table entry");

    //TODO deal with theses shifts on x32
    uint64_t p = 0xffff;
    const uint64_t obj_header = (((*handle_table_entry >> 16) | (p << 48)) >> 4) << 4;

    const auto obj_body = obj_header + d_->members_obj_[OBJECT_HEADER_Body];
    return obj_t{obj_body};
}

namespace
{
    opt<std::string> read_unicode_string(core::Core& core, uint64_t unicode_string)
    {
        using UnicodeString = struct
        {
            uint16_t length;
            uint16_t max_length;
            uint32_t _; // padding
            uint64_t buffer;
        };
        UnicodeString us;
        auto ok = core.mem.virtual_read(&us, unicode_string, sizeof us);
        if(!ok)
            FAIL({}, "unable to read UNICODE_STRING");

        us.length     = read_le16(&us.length);
        us.max_length = read_le16(&us.max_length);
        us.buffer     = read_le64(&us.buffer);

        if(us.length > us.max_length)
            FAIL({}, "corrupted UNICODE_STRING");

        std::vector<uint8_t> buffer(us.length);
        ok = core.mem.virtual_read(&buffer[0], us.buffer, us.length);
        if(!ok)
            FAIL({}, "unable to read UNICODE_STRING.buffer");

        const auto p = &buffer[0];
        return utf8::convert(p, &p[us.length]);
    }
}

opt<std::string> nt::ObjectNt::obj_typename(nt::obj_t obj)
{
    const auto POINTER_SIZE = 8;

    const auto obj_header       = obj.id - d_->members_obj_[OBJECT_HEADER_Body];
    const auto encoded_type_idx = core::read_byte(core_, obj_header + d_->members_obj_[OBJECT_HEADER_TypeIndex]);
    if(!encoded_type_idx)
        FAIL({}, "Unable to read encoded type index");

    const auto header_cookie = core::read_byte(core_, d_->symbols_obj_[ObHeaderCookie]);
    if(!header_cookie)
        FAIL({}, "Unable to read ObHeaderCookie");

    const uint8_t obj_addr_cookie = ((obj_header >> 8) & 0xff);
    const auto type_idx = *encoded_type_idx ^ *header_cookie ^ obj_addr_cookie;

    const auto obj_type = core::read_ptr(core_, d_->symbols_obj_[ObTypeIndexTable] + type_idx * POINTER_SIZE);
    if(!obj_type)
        FAIL({}, "Unable to read object type");

    return read_unicode_string(core_, *obj_type + d_->members_obj_[OBJECT_TYPE_Name]);
}

opt<std::string> nt::ObjectNt::fileobj_filename(nt::obj_t obj)
{
    return read_unicode_string(core_, obj.id + d_->members_obj_[FILE_OBJECT_FileName]);
}

opt<nt::obj_t> nt::ObjectNt::fileobj_deviceobject(nt::obj_t obj)
{
    const auto device_obj = core::read_ptr(core_, obj.id + d_->members_obj_[FILE_OBJECT_DeviceObject]);
    if(!device_obj)
        return {};

    return obj_t{*device_obj};
}

opt<nt::obj_t> nt::ObjectNt::deviceobj_driverobject(nt::obj_t obj)
{
    const auto driver_obj = core::read_ptr(core_, obj.id + d_->members_obj_[DEVICE_OBJECT_DriverObject]);
    if(!driver_obj)
        return {};

    return obj_t{*driver_obj};
}

opt<std::string> nt::ObjectNt::driverobj_drivername(nt::obj_t obj)
{
    return read_unicode_string(core_, obj.id + d_->members_obj_[DRIVER_OBJECT_DriverName]);
}
