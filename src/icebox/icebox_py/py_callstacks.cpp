#include "bindings.hpp"

PyObject* py::callstacks::read(core::Core& core, PyObject* args)
{
    auto* py_proc = static_cast<PyObject*>(nullptr);
    auto  size    = uint64_t{};
    auto  ok      = PyArg_ParseTuple(args, "SK", &py_proc, &size);
    if(!ok)
        return nullptr;

    const auto opt_proc = py::from_bytes<proc_t>(py_proc);
    if(!opt_proc)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to wait for process");

    auto* py_list = PyList_New(0);
    if(!py_list)
        return nullptr;

    PY_DEFER_DECREF(py_list);
    auto       buf = std::vector<::callstacks::caller_t>(size);
    const auto n   = ::callstacks::read(core, &buf[0], buf.size(), *opt_proc);
    for(size_t i = 0; i < n; ++i)
    {
        auto* item = PyLong_FromUnsignedLongLong(buf[i].addr);
        if(!item)
            return nullptr;

        PY_DEFER_DECREF(item);
        const auto err = PyList_Append(py_list, item);
        if(err)
            return nullptr;
    }

    Py_INCREF(py_list);
    return py_list;
}

PyObject* py::callstacks::load_module(core::Core& core, PyObject* args)
{
    auto* py_proc = static_cast<PyObject*>(nullptr);
    auto* py_mod  = static_cast<PyObject*>(nullptr);
    auto  ok      = PyArg_ParseTuple(args, "SS", &py_proc, &py_mod);
    if(!ok)
        return nullptr;

    const auto opt_proc = py::from_bytes<proc_t>(py_proc);
    if(!opt_proc)
        return nullptr;

    const auto opt_mod = py::from_bytes<mod_t>(py_mod);
    if(!opt_mod)
        return nullptr;

    ok = ::callstacks::load_module(core, *opt_proc, *opt_mod);
    if(!ok)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to load module");

    Py_RETURN_NONE;
}

PyObject* py::callstacks::load_driver(core::Core& core, PyObject* args)
{
    auto* py_proc = static_cast<PyObject*>(nullptr);
    auto* py_drv  = static_cast<PyObject*>(nullptr);
    auto  ok      = PyArg_ParseTuple(args, "SS", &py_proc, &py_drv);
    if(!ok)
        return nullptr;

    const auto opt_proc = py::from_bytes<proc_t>(py_proc);
    if(!opt_proc)
        return nullptr;

    const auto opt_drv = py::from_bytes<driver_t>(py_drv);
    if(!opt_drv)
        return nullptr;

    ok = ::callstacks::load_driver(core, *opt_proc, *opt_drv);
    if(!ok)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to load module");

    Py_RETURN_NONE;
}

PyObject* py::callstacks::autoload_modules(core::Core& core, PyObject* args)
{
    auto* py_proc = static_cast<PyObject*>(nullptr);
    auto  ok      = PyArg_ParseTuple(args, "S", &py_proc);
    if(!ok)
        return nullptr;

    const auto opt_proc = py::from_bytes<proc_t>(py_proc);
    if(!opt_proc)
        return nullptr;

    const auto opt_bp = ::callstacks::autoload_modules(core, *opt_proc);
    if(!opt_bp)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to load modules");

    return py::to_bytes(*opt_bp);
}
