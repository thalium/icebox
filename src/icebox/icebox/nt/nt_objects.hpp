#pragma once

#include "icebox/types.hpp"
#include "nt.hpp"

#include <memory>
#include <string_view>

namespace core { struct Core; }

namespace objects
{
    struct obj_t
    {
        uint64_t id;
    };

    struct file_t
    {
        uint64_t id;
    };

    struct device_t
    {
        uint64_t id;
    };

    struct driver_t
    {
        uint64_t id;
    };

    struct section_t
    {
        uint64_t id;
    };

    struct control_area_t
    {
        uint64_t id;
    };

    struct segment_t
    {
        uint64_t id;
    };

    struct Data;
    using Handler = std::shared_ptr<Data>;

    Handler             make                (core::Core& core, proc_t proc);
    opt<obj_t>          read                (Data& data, nt::HANDLE handle);
    opt<std::string>    name                (Data& data, obj_t obj);
    opt<std::string>    type                (Data& data, obj_t obj);
    opt<file_t>         file_read           (Data& data, nt::HANDLE handle);
    opt<std::string>    file_name           (Data& data, file_t file);
    opt<device_t>       file_device         (Data& data, file_t file);
    opt<driver_t>       device_driver       (Data& data, device_t device);
    opt<std::string>    driver_name         (Data& data, driver_t driver);
    opt<obj_t>          find                (Data& data, nt::ptr_t root, std::string_view path);
    opt<section_t>      as_section          (obj_t obj);
    opt<control_area_t> section_control_area(Data& data, section_t section);
    opt<segment_t>      control_area_segment(Data& data, control_area_t control_area);
    opt<span_t>         segment_span        (Data& data, segment_t segment);

} // namespace objects
