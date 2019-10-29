#pragma once

#include "icebox/types.hpp"
#include "nt.hpp"

#include <memory>

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

    struct Data;
    using Handler = std::shared_ptr<Data>;

    Handler             make            (core::Core& core, proc_t proc);
    opt<obj_t>          read            (Data& data, nt::HANDLE handle);
    opt<std::string>    type            (Data& data, obj_t obj);
    opt<file_t>         file_read       (Data& data, nt::HANDLE handle);
    opt<std::string>    file_name       (Data& data, file_t file);
    opt<device_t>       file_device     (Data& data, file_t file);
    opt<driver_t>       device_driver   (Data& data, device_t device);
    opt<std::string>    driver_name     (Data& data, driver_t driver);
} // namespace objects
