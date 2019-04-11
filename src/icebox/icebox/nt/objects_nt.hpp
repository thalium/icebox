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
         ObjectNt(core::Core& core);
        ~ObjectNt();

        opt<obj_t>          obj_read(proc_t proc, nt::HANDLE handle);
        opt<std::string>    obj_type(proc_t proc, obj_t obj);

        opt<file_t>         file_read   (proc_t proc, nt::HANDLE handle);
        opt<std::string>    file_name   (proc_t proc, file_t file);
        opt<device_t>       file_device (proc_t proc, file_t file);

        opt<driver_t> device_driver(proc_t proc, device_t device);

        opt<std::string> driver_name(proc_t proc, driver_t driver);

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace nt
