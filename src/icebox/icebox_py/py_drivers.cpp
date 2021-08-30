#include "bindings.hpp"

PyObject* py::drivers::list(core::Core& core, PyObject* /*args*/)
{
    auto* py_list = PyList_New(0);
    if(!py_list)
        return nullptr;

    PY_DEFER_DECREF(py_list);
    ::drivers::list(core, [&](driver_t drv)
    {
        auto* item = py::to_bytes(drv);
        if(!item)
            return walk_e::stop;

        PY_DEFER_DECREF(item);
        const auto err = PyList_Append(py_list, item);
        if(err)
            return walk_e::stop;

        return walk_e::next;
    });
    Py_INCREF(py_list);
    return py_list;
}

PyObject* py::drivers::name(core::Core& core, PyObject* args)
{
    auto* py_drv = static_cast<PyObject*>(nullptr);
    auto  ok     = PyArg_ParseTuple(args, "S", &py_drv);
    if(!ok)
        return nullptr;

    const auto opt_drv = py::from_bytes<driver_t>(py_drv);
    if(!opt_drv)
        return nullptr;

    const auto opt_name = ::drivers::name(core, *opt_drv);
    if(!opt_name)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to read drvule name");

    return py_to_string(opt_name->data(), opt_name->size());
}

PyObject* py::drivers::span(core::Core& core, PyObject* args)
{
    auto* py_drv = static_cast<PyObject*>(nullptr);
    auto  ok     = PyArg_ParseTuple(args, "S", &py_drv);
    if(!ok)
        return nullptr;

    const auto opt_drv = py::from_bytes<driver_t>(py_drv);
    if(!opt_drv)
        return nullptr;

    const auto opt_span = ::drivers::span(core, *opt_drv);
    if(!opt_span)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to read drvule name");

    return Py_BuildValue("(KK)", opt_span->addr, opt_span->size);
}

PyObject* py::drivers::find(core::Core& core, PyObject* args)
{
    auto       ptr = uint64_t{};
    const auto ok  = PyArg_ParseTuple(args, "K", &ptr);
    if(!ok)
        return nullptr;

    const auto opt_drv = ::drivers::find(core, ptr);
    if(!opt_drv)
        Py_RETURN_NONE;

    return py::to_bytes(*opt_drv);
}

PyObject* py::drivers::listen(core::Core& core, PyObject* args)
{
    auto*      py_func = static_cast<PyObject*>(nullptr);
    const auto ok      = PyArg_ParseTuple(args, "O", &py_func);
    if(!ok)
        return nullptr;

    if(!PyCallable_Check(py_func))
        return py::fail_with(nullptr, PyExc_TypeError, "arg must be callable");

    const auto opt_bpid = ::drivers::listen_create(core, [=](driver_t drv, bool load)
    {
        const auto* py_drv = py::to_bytes(drv);
        auto*       args   = Py_BuildValue("(OO)", py_drv, load ? Py_True : Py_False);
        if(!args)
            return;

        PY_DEFER_DECREF(args);
        auto* ret = PyObject_Call(py_func, args, nullptr);
        if(ret)
            PY_DEFER_DECREF(ret);
    });
    if(!opt_bpid)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to listen");

    return py::to_bytes(*opt_bpid);
}
