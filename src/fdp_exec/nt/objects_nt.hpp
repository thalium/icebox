#pragma once

#include "core.hpp"
#include "nt.hpp"
// #include "types.hpp"

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
        opt<std::string>    obj_typename    (obj_t obj);

        opt<std::string>    fileobj_filename        (obj_t obj);
        opt<obj_t>          fileobj_deviceobject    (obj_t obj);
        opt<obj_t>          deviceobj_driverobject  (obj_t obj);
        opt<std::string>    driverobj_drivername    (obj_t obj);

        struct Data;
        std::unique_ptr<Data> d_;

        core::Core& core_;
    };

    std::shared_ptr<nt::ObjectNt> make_objectnt(core::Core& core);
} // namespace nt
