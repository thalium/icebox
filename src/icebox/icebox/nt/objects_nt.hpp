#pragma once

#include "icebox/types.hpp"

#include "nt.hpp"

#include <memory>

namespace core { struct Core; }

namespace nt
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

    struct ObjectNt
    {
         ObjectNt(core::Core& core, proc_t proc);
        ~ObjectNt();

        opt<obj_t>          obj_read(nt::HANDLE handle);
        opt<std::string>    obj_type(obj_t obj);

        opt<file_t>         file_read   (nt::HANDLE handle);
        opt<std::string>    file_name   (file_t file);
        opt<device_t>       file_device (file_t file);

        opt<driver_t> device_driver(device_t device);

        opt<std::string> driver_name(driver_t driver);

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace nt
