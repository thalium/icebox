#pragma once

#include "core.hpp"
#include "nt.hpp"

#include <memory>

namespace nt
{
    struct obj_t
    {
        uint64_t id;
    };

    struct ObjectNt
    {
         ObjectNt(core::Core& core);
        ~ObjectNt();

        bool                setup           ();
        opt<obj_t>          get_object_ref  (proc_t proc, HANDLE handle);
        opt<std::string>    obj_typename    (proc_t proc, obj_t obj);

        opt<std::string>    fileobj_filename        (proc_t proc, obj_t obj);
        opt<obj_t>          fileobj_deviceobject    (proc_t proc, obj_t obj);
        opt<obj_t>          deviceobj_driverobject  (proc_t proc, obj_t obj);
        opt<std::string>    driverobj_drivername    (proc_t proc, obj_t obj);

        struct Data;
        std::unique_ptr<Data> d_;
    };

    std::shared_ptr<nt::ObjectNt> make_objectnt(core::Core& core);
} // namespace nt
