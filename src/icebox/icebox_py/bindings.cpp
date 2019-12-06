#include "bindings.hpp"

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
            return py::fail_with(nullptr, PyExc_RuntimeError, "already attached");

        const auto core = core::attach(name ? name : "");
        if(!core)
            return py::fail_with(nullptr, PyExc_RuntimeError, "unable to attach");

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
}

core::Core* py::from_self(PyObject* self)
{
    const auto handle = handle_from_self(self);
    if(!handle)
        return nullptr;

    if(!handle->core)
        return py::fail_with(nullptr, PyExc_RuntimeError, "not attached to any vm");

    return handle->core.get();
}

PyObject* py::to_bytes(const char* ptr, size_t size)
{
    return PyBytes_FromStringAndSize(ptr, size);
}

const char* py::from_bytes(PyObject* self, size_t size)
{
    if(!PyBytes_CheckExact(self))
        return py::fail_with(nullptr, PyExc_RuntimeError, "invalid argument");

    auto src       = static_cast<char*>(nullptr);
    auto len       = ssize_t{};
    const auto err = PyBytes_AsStringAndSize(self, &src, &len);
    if(err)
        return py::fail_with(nullptr, PyExc_RuntimeError, "invalid argument");

    if(len != static_cast<ssize_t>(size))
        return py::fail_with(nullptr, PyExc_RuntimeError, "invalid argument");

    return src;
}

PyMODINIT_FUNC PyInit__icebox()
{
    auto args       = std::array<char*, 2>{"_icebox", nullptr};
    const auto argc = static_cast<int>(args.size());
    logg::init(argc - 1, &args[0]);

    static auto ice_methods = std::array<PyMethodDef, 32>{{
        {"attach", &core_attach, METH_VARARGS, "attach vm <name>"},
        {"detach", &core_detach, METH_NOARGS, "detach from vm"},
        // state
        {"pause", &py::state::pause, METH_NOARGS, "pause vm"},
        {"resume", &py::state::resume, METH_NOARGS, "resume vm"},
        {"single_step", &py::state::single_step, METH_NOARGS, "execute a single instruction"},
        {"wait", &py::state::wait, METH_NOARGS, "wait vm"},
        // registers
        {"msr_list", &py::registers::msr_list, METH_NOARGS, "list available msr registers"},
        {"msr_read", &py::registers::msr_read, METH_VARARGS, "read msr register"},
        {"msr_write", &py::registers::msr_write, METH_VARARGS, "write msr register"},
        {"register_list", &py::registers::list, METH_NOARGS, "list available registers"},
        {"register_read", &py::registers::read, METH_VARARGS, "read register"},
        {"register_write", &py::registers::write, METH_VARARGS, "write register"},
        // process
        {"process_current", &py::process::current, METH_NOARGS, "read current process"},
        {"process_flags", &py::process::flags, METH_VARARGS, "read process flags"},
        {"process_is_valid", &py::process::is_valid, METH_VARARGS, "check if process is valid"},
        {"process_join", &py::process::join, METH_VARARGS, "join process"},
        {"process_list", &py::process::list, METH_NOARGS, "list available processes"},
        {"process_listen_create", &py::process::listen_create, METH_VARARGS, "listen on process creation"},
        {"process_listen_delete", &py::process::listen_delete, METH_VARARGS, "listen on process deletion"},
        {"process_name", &py::process::name, METH_VARARGS, "read process name"},
        {"process_parent", &py::process::parent, METH_VARARGS, "read process parent, if any"},
        {"process_pid", &py::process::pid, METH_VARARGS, "read process pid"},
        {"process_wait", &py::process::wait, METH_VARARGS, "wait for process"},
        {nullptr, nullptr, 0, nullptr},
    }};
    static auto ice_module  = PyModuleDef{
        PyModuleDef_HEAD_INIT,                     // m_base
        "_icebox",                                 // m_name
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
