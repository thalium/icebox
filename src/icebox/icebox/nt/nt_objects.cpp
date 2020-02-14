#include "nt_objects.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "nt_objects"
#include "core.hpp"
#include "core/core_private.hpp"
#include "endian.hpp"
#include "log.hpp"
#include "nt_os.hpp"
#include "utils/utf8.hpp"
#include "utils/utils.hpp"
#include "wow64.hpp"

#ifdef _MSC_VER
#    include <string.h>
#    define strncasecmp _strnicmp
#else
#    include <strings.h>
#endif

struct objects::Data
{
    Data(core::Core& core, proc_t proc)
        : core(core)
        , nt(*core.nt_)
        , proc(proc)
        , io(memory::make_io(core, proc))
    {
    }

    core::Core& core;
    nt::Os&     nt;
    proc_t      proc;
    memory::Io  io;
    obj_t       root;
    uint8_t     masks[16];
};

namespace
{
    using Data = objects::Data;

    bool setup(Data& d)
    {
        const auto ok = d.io.read_all(&d.root.id, *d.nt.symbols_[ObpRootDirectoryObject], sizeof d.root.id);
        if(!ok)
            return false;

        return d.io.read_all(&d.masks, *d.nt.symbols_[ObpInfoMaskToOffset], sizeof d.masks);
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
    opt<objects::obj_t> object_read(const Data& d, nt::HANDLE handle)
    {
        // Is kernel handle
        const auto handle_table_addr = handle & 0x80000000 ? *d.nt.symbols_[ObpKernelHandleTable] : d.proc.id + d.nt.offsets_[EPROCESS_ObjectTable];
        if(handle & 0x80000000)
            handle = ((handle << 32) >> 32) & ~0xffffffff80000000;

        const auto handle_table = d.io.read(handle_table_addr);
        if(!handle_table)
            return FAIL(std::nullopt, "unable to read handle table");

        auto handle_table_code = d.io.read(*handle_table + d.nt.offsets_[HANDLE_TABLE_TableCode]);
        if(!handle_table_code)
            return FAIL(std::nullopt, "unable to read handle table code");

        const auto handle_table_level = *handle_table_code & 3;
        *handle_table_code &= ~3;

        constexpr auto HANDLE_VALUE_INC        = 0x04; // Amount to increment the Value to get to the next handle
        constexpr auto HANDLE_TABLE_ENTRY_SIZE = 0x10;
        constexpr auto PAGE_SIZE               = 4096;
        constexpr auto POINTER_SIZE            = 8;

        auto i = uint64_t{};
        auto j = uint64_t{};
        auto k = uint64_t{};
        switch(handle_table_level)
        {
            case 0:
                i = handle;
                break;

            case 1:
                i = handle % ((PAGE_SIZE / HANDLE_TABLE_ENTRY_SIZE) * HANDLE_VALUE_INC);
                handle -= i;
                j = handle / (((PAGE_SIZE / HANDLE_TABLE_ENTRY_SIZE) * HANDLE_VALUE_INC) / POINTER_SIZE);

                handle_table_code = d.io.read(*handle_table_code + j);
                // handle_table_code = *handle_table_code + j;
                break;

            case 2:
                i = handle % ((PAGE_SIZE / HANDLE_TABLE_ENTRY_SIZE) * HANDLE_VALUE_INC);
                handle -= i;
                k = handle / (((PAGE_SIZE / HANDLE_TABLE_ENTRY_SIZE) * HANDLE_VALUE_INC) / POINTER_SIZE);
                j = k % PAGE_SIZE;
                k -= j;
                k /= (PAGE_SIZE / POINTER_SIZE);

                handle_table_code = d.io.read(*handle_table_code + k);
                handle_table_code = d.io.read(*handle_table_code + j);
                break;

            default:
                return FAIL(std::nullopt, "Unknown table level");
        }
        if(!*handle_table_code)
            return std::nullopt;

        const auto handle_table_entry = d.io.read(*handle_table_code + i * (HANDLE_TABLE_ENTRY_SIZE / HANDLE_VALUE_INC));
        if(!handle_table_entry)
            return FAIL(std::nullopt, "unable to read table entry");

        // TODO deal with theses shifts on x32
        uint64_t p                = 0xffff;
        const uint64_t obj_header = (((*handle_table_entry >> 16) | (p << 48)) >> 4) << 4;
        const auto obj_body       = obj_header + d.nt.offsets_[OBJECT_HEADER_Body];
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
        const auto obj_header       = obj.id - d.nt.offsets_[OBJECT_HEADER_Body];
        const auto encoded_type_idx = d.io.byte(obj_header + d.nt.offsets_[OBJECT_HEADER_TypeIndex]);
        if(!encoded_type_idx)
            return {};

        const uint8_t obj_addr_cookie = ((obj_header >> 8) & 0xff);
        auto type_idx                 = *encoded_type_idx ^ obj_addr_cookie;
        const auto opt_cookie         = d.nt.symbols_[ObHeaderCookie];
        if(opt_cookie)
        {
            const auto header_cookie = d.io.byte(*opt_cookie);
            if(!header_cookie)
                return FAIL(std::nullopt, "unable to read ObHeaderCookie");

            type_idx ^= *header_cookie;
        }

        const auto obj_type = d.io.read(*d.nt.symbols_[ObTypeIndexTable] + static_cast<size_t>(type_idx) * POINTER_SIZE);
        if(!obj_type)
            return FAIL(std::nullopt, "unable to read object type");

        return nt::read_unicode_string(d.io, *obj_type + d.nt.offsets_[OBJECT_TYPE_Name]);
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
        const auto header = obj.id - d.nt.offsets_[OBJECT_HEADER_Body];
        auto ok           = d.io.read_all(&info_mask, header + d.nt.offsets_[OBJECT_HEADER_InfoMask], sizeof info_mask);
        if(!ok)
            return {};

        auto info = nt::_OBJECT_HEADER_NAME_INFO{};
        ok        = d.io.read_all(&info, header - d.masks[info_mask & 3], sizeof info);
        if(!ok)
            return {};

        return nt::read_unicode_string(d.io, header - d.masks[info_mask & 3] + offsetof(nt::_OBJECT_HEADER_NAME_INFO, Name));
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
    return nt::read_unicode_string(d.io, file.id + d.nt.offsets_[FILE_OBJECT_FileName]);
}

opt<objects::device_t> objects::file_device(Data& d, file_t file)
{
    const auto dev = d.io.read(file.id + d.nt.offsets_[FILE_OBJECT_DeviceObject]);
    if(!dev)
        return {};

    return device_t{*dev};
}

opt<objects::driver_t> objects::device_driver(Data& d, device_t device)
{
    const auto drv = d.io.read(device.id + d.nt.offsets_[DEVICE_OBJECT_DriverObject]);
    if(!drv)
        return {};

    return driver_t{*drv};
}

opt<std::string> objects::driver_name(Data& d, driver_t driver)
{
    return nt::read_unicode_string(d.io, driver.id + d.nt.offsets_[DRIVER_OBJECT_DriverName]);
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

    uint32_t hash_name(std::string_view src)
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
            const auto ok = d.io.read_all(&entry, entry.ChainLink, sizeof entry);
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
        const auto ok  = d.io.read_all(&directory, root, sizeof directory);
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
        const auto ok = d.io.read_all(&struc, id, sizeof struc);
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
