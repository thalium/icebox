#include "bindings.hpp"

namespace
{
    PyObject* none_or_error(bool ok)
    {
        if(PyErr_Occurred())
            return nullptr;

        if(!ok)
            return py::fail_with(nullptr, PyExc_RuntimeError, "error");

        Py_RETURN_NONE;
    }
}

PyObject* py::state::pause(core::Core& core, PyObject* /*args*/)
{
    const auto ok = ::state::pause(core);
    return none_or_error(ok);
}

PyObject* py::state::resume(core::Core& core, PyObject* /*args*/)
{
    const auto ok = ::state::resume(core);
    return none_or_error(ok);
}

PyObject* py::state::single_step(core::Core& core, PyObject* /*args*/)
{
    const auto ok = ::state::single_step(core);
    return none_or_error(ok);
}

PyObject* py::state::wait(core::Core& core, PyObject* /*args*/)
{
    const auto ok = ::state::wait(core);
    return none_or_error(ok);
}

PyObject* py::state::interrupt(core::Core& core, PyObject* /*args*/)
{
    ::state::interrupt(core);
    Py_RETURN_NONE;
}

namespace
{
    PyObject* from_breakpoint(core::Core& core, const state::Breakpoint& bp)
    {
        const auto bpid = state::save_breakpoint(core, bp);
        if(!bpid.id)
            return py::fail_with(nullptr, PyExc_RuntimeError, "cannot save breakpoint");

        return py::to_bytes(bpid);
    }
}

PyObject* py::state::break_on(core::Core& core, PyObject* args)
{
    auto       name    = static_cast<const char*>(nullptr);
    auto       where   = uint64_t{};
    auto       py_func = static_cast<PyObject*>(nullptr);
    const auto ok      = PyArg_ParseTuple(args, "sKO", &name, &where, &py_func);
    if(!ok)
        return nullptr;

    name = name ? name : "";
    if(!PyCallable_Check(py_func))
        return py::fail_with(nullptr, PyExc_TypeError, "arg must be callable");

    const auto bp = ::state::break_on(core, name, where, [=]
    {
        const auto args = Py_BuildValue("()");
        if(!args)
            return;

        PY_DEFER_DECREF(args);
        const auto ret = PyObject_Call(py_func, args, nullptr);
        if(ret)
            PY_DEFER_DECREF(ret);
    });
    return from_breakpoint(core, bp);
}

PyObject* py::state::break_on_process(core::Core& core, PyObject* args)
{
    auto       name    = static_cast<const char*>(nullptr);
    auto       py_proc = static_cast<PyObject*>(nullptr);
    auto       where   = uint64_t{};
    auto       py_func = static_cast<PyObject*>(nullptr);
    const auto ok      = PyArg_ParseTuple(args, "sSKO", &name, &py_proc, &where, &py_func);
    if(!ok)
        return nullptr;

    const auto opt_proc = py::from_bytes<proc_t>(py_proc);
    if(!opt_proc)
        return nullptr;

    name = name ? name : "";
    if(!PyCallable_Check(py_func))
        return py::fail_with(nullptr, PyExc_TypeError, "arg must be callable");

    const auto bp = ::state::break_on_process(core, name, *opt_proc, where, [=]
    {
        const auto args = Py_BuildValue("()");
        if(!args)
            return;

        PY_DEFER_DECREF(args);
        const auto ret = PyObject_Call(py_func, args, nullptr);
        if(ret)
            PY_DEFER_DECREF(ret);
    });
    return from_breakpoint(core, bp);
}

PyObject* py::state::break_on_thread(core::Core& core, PyObject* args)
{
    auto       name      = static_cast<const char*>(nullptr);
    auto       py_thread = static_cast<PyObject*>(nullptr);
    auto       where     = uint64_t{};
    auto       py_func   = static_cast<PyObject*>(nullptr);
    const auto ok        = PyArg_ParseTuple(args, "sSKO", &name, &py_thread, &where, &py_func);
    if(!ok)
        return nullptr;

    const auto opt_thread = py::from_bytes<thread_t>(py_thread);
    if(!opt_thread)
        return nullptr;

    name = name ? name : "";
    if(!PyCallable_Check(py_func))
        return py::fail_with(nullptr, PyExc_TypeError, "arg must be callable");

    const auto bp = ::state::break_on_thread(core, name, *opt_thread, where, [=]
    {
        const auto args = Py_BuildValue("()");
        if(!args)
            return;

        PY_DEFER_DECREF(args);
        const auto ret = PyObject_Call(py_func, args, nullptr);
        if(ret)
            PY_DEFER_DECREF(ret);
    });
    return from_breakpoint(core, bp);
}

PyObject* py::state::break_on_physical(core::Core& core, PyObject* args)
{
    auto       name    = static_cast<const char*>(nullptr);
    auto       where   = uint64_t{};
    auto       py_func = static_cast<PyObject*>(nullptr);
    const auto ok      = PyArg_ParseTuple(args, "sKO", &name, &where, &py_func);
    if(!ok)
        return nullptr;

    name = name ? name : "";
    if(!PyCallable_Check(py_func))
        return py::fail_with(nullptr, PyExc_TypeError, "arg must be callable");

    const auto bp = ::state::break_on_physical(core, name, phy_t{where}, [=]
    {
        const auto args = Py_BuildValue("()");
        if(!args)
            return;

        PY_DEFER_DECREF(args);
        const auto ret = PyObject_Call(py_func, args, nullptr);
        if(ret)
            PY_DEFER_DECREF(ret);
    });
    return from_breakpoint(core, bp);
}

PyObject* py::state::break_on_physical_process(core::Core& core, PyObject* args)
{
    auto       name    = static_cast<const char*>(nullptr);
    auto       dtb     = uint64_t{};
    auto       where   = uint64_t{};
    auto       py_func = static_cast<PyObject*>(nullptr);
    const auto ok      = PyArg_ParseTuple(args, "sKKO", &name, &dtb, &where, &py_func);
    if(!ok)
        return nullptr;

    name = name ? name : "";
    if(!PyCallable_Check(py_func))
        return py::fail_with(nullptr, PyExc_TypeError, "arg must be callable");

    const auto bp = ::state::break_on_physical_process(core, name, dtb_t{dtb}, phy_t{where}, [=]
    {
        const auto args = Py_BuildValue("()");
        if(!args)
            return;

        PY_DEFER_DECREF(args);
        const auto ret = PyObject_Call(py_func, args, nullptr);
        if(ret)
            PY_DEFER_DECREF(ret);
    });
    return from_breakpoint(core, bp);
}

PyObject* py::state::drop_breakpoint(core::Core& core, PyObject* args)
{
    auto       py_bpid = static_cast<PyObject*>(nullptr);
    const auto ok      = PyArg_ParseTuple(args, "O", &py_bpid);
    if(!ok)
        return nullptr;

    const auto bpid = from_bytes<bpid_t>(py_bpid);
    if(!bpid)
        return nullptr;

    ::state::drop_breakpoint(core, *bpid);
    Py_RETURN_NONE;
}
