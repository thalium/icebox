#include "bindings.hpp"

PyObject* py::modules::list(core::Core& core, PyObject* args)
{
    auto       py_proc = static_cast<PyObject*>(nullptr);
    const auto ok      = PyArg_ParseTuple(args, "S", &py_proc);
    if(!ok)
        return nullptr;

    const auto opt_proc = py::from_bytes<proc_t>(py_proc);
    if(!opt_proc)
        return nullptr;

    auto py_list = PyList_New(0);
    if(!py_list)
        return nullptr;

    PY_DEFER_DECREF(py_list);
    ::modules::list(core, *opt_proc, [&](mod_t mod)
    {
        auto item = py::to_bytes(mod);
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

PyObject* py::modules::name(core::Core& core, PyObject* args)
{
    auto py_proc = static_cast<PyObject*>(nullptr);
    auto py_mod  = static_cast<PyObject*>(nullptr);
    auto ok      = PyArg_ParseTuple(args, "SS", &py_proc, &py_mod);
    if(!ok)
        return nullptr;

    const auto opt_proc = py::from_bytes<proc_t>(py_proc);
    if(!py_proc)
        return nullptr;

    const auto opt_mod = py::from_bytes<mod_t>(py_mod);
    if(!opt_mod)
        return nullptr;

    const auto opt_name = ::modules::name(core, *opt_proc, *opt_mod);
    if(!opt_name)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to read module name");

    return PyUnicode_FromStringAndSize(opt_name->data(), opt_name->size());
}

PyObject* py::modules::span(core::Core& core, PyObject* args)
{
    auto py_proc = static_cast<PyObject*>(nullptr);
    auto py_mod  = static_cast<PyObject*>(nullptr);
    auto ok      = PyArg_ParseTuple(args, "SS", &py_proc, &py_mod);
    if(!ok)
        return nullptr;

    const auto opt_proc = py::from_bytes<proc_t>(py_proc);
    if(!py_proc)
        return nullptr;

    const auto opt_mod = py::from_bytes<mod_t>(py_mod);
    if(!opt_mod)
        return nullptr;

    const auto opt_span = ::modules::span(core, *opt_proc, *opt_mod);
    if(!opt_span)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to read module name");

    return Py_BuildValue("(KK)", opt_span->addr, opt_span->size);
}

PyObject* py::modules::flags(core::Core& /*core*/, PyObject* args)
{
    auto py_mod = static_cast<PyObject*>(nullptr);
    auto ok     = PyArg_ParseTuple(args, "S", &py_mod);
    if(!ok)
        return nullptr;

    const auto opt_mod = py::from_bytes<mod_t>(py_mod);
    if(!opt_mod)
        return nullptr;

    return py::flags::from(opt_mod->flags);
}

PyObject* py::modules::find(core::Core& core, PyObject* args)
{
    auto       py_proc = static_cast<PyObject*>(nullptr);
    auto       ptr     = uint64_t{};
    const auto ok      = PyArg_ParseTuple(args, "OK", &py_proc, &ptr);
    if(!ok)
        return nullptr;

    const auto opt_proc = py::from_bytes<proc_t>(py_proc);
    if(!opt_proc)
        return nullptr;

    const auto opt_mod = ::modules::find(core, *opt_proc, ptr);
    if(!opt_mod)
        Py_RETURN_NONE;

    return py::to_bytes(*opt_mod);
}

PyObject* py::modules::find_name(core::Core& core, PyObject* args)
{
    auto       py_proc  = static_cast<PyObject*>(nullptr);
    auto       name     = static_cast<const char*>(nullptr);
    auto       py_flags = static_cast<PyObject*>(nullptr);
    const auto ok       = PyArg_ParseTuple(args, "OsO", &py_proc, &name, &py_flags);
    if(!ok)
        return nullptr;

    const auto opt_proc = py::from_bytes<proc_t>(py_proc);
    if(!opt_proc)
        return nullptr;

    const auto opt_flags = py::flags::to(py_flags);
    if(!opt_flags)
        return nullptr;

    const auto opt_mod = ::modules::find_name(core, *opt_proc, name, *opt_flags);
    if(!opt_mod)
        Py_RETURN_NONE;

    return py::to_bytes(*opt_mod);
}

PyObject* py::modules::listen_create(core::Core& core, PyObject* args)
{
    auto       py_proc  = static_cast<PyObject*>(nullptr);
    auto       py_flags = static_cast<PyObject*>(nullptr);
    auto       py_func  = static_cast<PyObject*>(nullptr);
    const auto ok       = PyArg_ParseTuple(args, "OOO", &py_proc, &py_flags, &py_func);
    if(!ok)
        return nullptr;

    const auto opt_proc = py::from_bytes<proc_t>(py_proc);
    if(!opt_proc)
        return nullptr;

    const auto opt_flags = py::flags::to(py_flags);
    if(!opt_flags)
        return nullptr;

    if(!PyCallable_Check(py_func))
        return py::fail_with(nullptr, PyExc_TypeError, "arg must be callable");

    const auto opt_bpid = ::modules::listen_create(core, *opt_proc, *opt_flags, [=](mod_t mod)
    {
        const auto py_mod = py::to_bytes(mod);
        const auto args   = Py_BuildValue("(O)", py_mod);
        if(!args)
            return;

        PY_DEFER_DECREF(args);
        const auto ret = PyObject_Call(py_func, args, nullptr);
        if(ret)
            PY_DEFER_DECREF(ret);
    });
    if(!opt_bpid)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to listen");

    return py::to_bytes(*opt_bpid);
}
