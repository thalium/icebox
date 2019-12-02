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

    Handle* handle_from_py(PyObject* obj)
    {
        if(!obj)
            return nullptr;

        const auto ptr = PyCapsule_GetPointer(obj, nullptr);
        return static_cast<Handle*>(ptr);
    }

    void delete_handle(PyObject* obj)
    {
        auto handle = handle_from_py(obj);
        if(!handle)
            return;

        handle->~Handle();
        delete handle;
    }

    PyObject* core_attach(PyObject* /*self*/, PyObject* args)
    {
        auto name     = static_cast<const char*>(nullptr);
        const auto ok = PyArg_ParseTuple(args, "s", &name);
        if(!ok)
            return nullptr;

        const auto core = core::attach(name ? name : "");
        if(!core)
        {
            PyErr_SetString(PyExc_ConnectionError, "unable to attach");
            return nullptr;
        }

        auto handle = static_cast<Handle*>(malloc(sizeof(Handle)));
        if(!handle)
            return PyErr_NoMemory();

        new(handle) Handle{core};
        return PyCapsule_New(handle, nullptr, &delete_handle);
    }
}

PyMODINIT_FUNC PyInit_icebox()
{
    auto args       = std::array<char*, 2>{"icebox", nullptr};
    const auto argc = static_cast<int>(args.size());
    logg::init(argc - 1, &args[0]);

    static auto ice_methods = std::array<PyMethodDef, 2>{
        PyMethodDef{"attach", &core_attach, METH_VARARGS, "attach vm <name>"},
        PyMethodDef{nullptr, nullptr, 0, nullptr},
    };
    static auto ice_module = PyModuleDef{
        PyModuleDef_HEAD_INIT,
        "icebox",                                  // module name
        "Python interface for the icebox library", // module documentation
        -1,                                        // sizeof per-interpreter module state
        &ice_methods[0],
    };
    return PyModule_Create(&ice_module);
}
