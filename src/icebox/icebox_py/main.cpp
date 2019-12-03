#define PY_SSIZE_T_CLEAN
#ifndef DEBUG
#    define Py_LIMITED_API 0x03030000
#endif
// must be included first
#include <python.h>

#define FDP_MODULE "py"
#include <icebox/core.hpp>
#include <icebox/log.hpp>

#include <array>

namespace
{
    struct Handle
    {
        std::shared_ptr<core::Core> core;
    };

    Handle* handle_from_self(PyObject* self)
    {
        if(self)
            if(const auto ptr = PyModule_GetState(self))
                return static_cast<Handle*>(ptr);

        PyErr_SetString(PyExc_RuntimeError, "missing module handle");
        return nullptr;
    }

    void delete_handle(void* ptr)
    {
        if(!ptr)
            return;

        auto handle = static_cast<Handle*>(ptr);
        handle->~Handle();
    }

    template <typename T>
    T fail_with(T ret, PyObject* err, const char* msg)
    {
        PyErr_SetString(err, msg);
        return ret;
    }

    PyObject* core_attach(PyObject* self, PyObject* args)
    {
        auto name     = static_cast<const char*>(nullptr);
        const auto ok = PyArg_ParseTuple(args, "s", &name);
        if(!ok)
            return nullptr;

        auto handle = handle_from_self(self);
        if(!handle)
            return nullptr;

        if(handle->core)
            return fail_with(nullptr, PyExc_RuntimeError, "already attached");

        const auto core = core::attach(name ? name : "");
        if(!core)
            return fail_with(nullptr, PyExc_RuntimeError, "unable to attach");

        new(handle) Handle{core};
        Py_RETURN_NONE;
    }

    PyObject* core_detach(PyObject* self, PyObject* /*args*/)
    {
        auto handle = handle_from_self(self);
        if(!handle)
            return nullptr;

        handle->core.reset();
        Py_RETURN_NONE;
    }

    core::Core* core_from_self(PyObject* self)
    {
        const auto handle = handle_from_self(self);
        if(!handle)
            return nullptr;

        if(!handle->core)
            return fail_with(nullptr, PyExc_RuntimeError, "not attached to any vm");

        return handle->core.get();
    }

    template <bool (*Op)(core::Core&)>
    PyObject* exec_core(PyObject* self, PyObject* /*args*/)
    {
        auto core = core_from_self(self);
        if(!core)
            return nullptr;

        const auto ok = Op(*core);
        if(!ok)
            return fail_with(nullptr, PyExc_RuntimeError, "error");

        Py_RETURN_NONE;
    }
}

PyMODINIT_FUNC PyInit_icebox()
{
    auto args       = std::array<char*, 2>{"icebox", nullptr};
    const auto argc = static_cast<int>(args.size());
    logg::init(argc - 1, &args[0]);

    static auto ice_methods = std::array<PyMethodDef, 7>{{
        {"attach", &core_attach, METH_VARARGS, "attach vm <name>"},
        {"detach", &core_detach, METH_NOARGS, "detach from vm"},
        {"pause", &exec_core<&state::pause>, METH_NOARGS, "pause vm"},
        {"resume", &exec_core<&state::resume>, METH_NOARGS, "resume vm"},
        {"single_step", &exec_core<&state::single_step>, METH_NOARGS, "execute a single instruction"},
        {"wait", &exec_core<&state::wait>, METH_NOARGS, "wait vm"},
        {nullptr, nullptr, 0, nullptr},
    }};
    static auto ice_module  = PyModuleDef{
        PyModuleDef_HEAD_INIT,                     // m_base
        "icebox",                                  // m_name
        "Python interface for the icebox library", // m_doc
        sizeof(Handle),                            // m_size
        &ice_methods[0],                           // m_methods
        nullptr,                                   // m_slots
        nullptr,                                   // m_traverse
        nullptr,                                   // m_clear
        &delete_handle,                            // m_free
    };
    const auto ptr = PyModule_Create(&ice_module);
    if(!ptr)
        return nullptr;

    auto handle = handle_from_self(ptr);
    if(!handle)
        return nullptr;

    new(handle) Handle{{}};
    return ptr;
}
