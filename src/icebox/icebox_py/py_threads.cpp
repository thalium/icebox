#include "bindings.hpp"

PyObject* py::threads::list(core::Core& core, PyObject* args)
{
    auto* py_proc = static_cast<PyObject*>(nullptr);
    auto  ok      = PyArg_ParseTuple(args, "S", &py_proc);
    if(!ok)
        return nullptr;

    const auto opt_proc = py::from_bytes<proc_t>(py_proc);
    if(!opt_proc)
        return nullptr;

    auto* py_list = PyList_New(0);
    if(!py_list)
        return nullptr;

    PY_DEFER_DECREF(py_list);
    ok = ::threads::list(core, *opt_proc, [&](thread_t thread)
    {
        auto* item = py::to_bytes(thread);
        if(!item)
            return walk_e::stop;

        PY_DEFER_DECREF(item);
        const auto err = PyList_Append(py_list, item);
        if(err)
            return walk_e::stop;

        return walk_e::next;
    });
    if(!ok)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to list threads");

    Py_INCREF(py_list);
    return py_list;
}

PyObject* py::threads::current(core::Core& core, PyObject* /*args*/)
{
    const auto opt_thread = ::threads::current(core);
    if(!opt_thread)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to read current thread");

    return py::to_bytes(*opt_thread);
}

PyObject* py::threads::process(core::Core& core, PyObject* args)
{
    auto* py_thread = static_cast<PyObject*>(nullptr);
    auto  ok        = PyArg_ParseTuple(args, "S", &py_thread);
    if(!ok)
        return nullptr;

    const auto opt_thread = py::from_bytes<thread_t>(py_thread);
    if(!opt_thread)
        return nullptr;

    const auto opt_proc = ::threads::process(core, *opt_thread);
    if(!opt_proc)
        Py_RETURN_NONE;

    return py::to_bytes(*opt_proc);
}

PyObject* py::threads::program_counter(core::Core& core, PyObject* args)
{
    auto* py_thread = static_cast<PyObject*>(nullptr);
    auto  ok        = PyArg_ParseTuple(args, "S", &py_thread);
    if(!ok)
        return nullptr;

    const auto opt_thread = py::from_bytes<thread_t>(py_thread);
    if(!opt_thread)
        return nullptr;

    const auto opt_proc = ::threads::process(core, *opt_thread);
    if(!opt_proc)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to read thread process");

    const auto opt_pc = ::threads::program_counter(core, *opt_proc, *opt_thread);
    if(!opt_pc)
        Py_RETURN_NONE;

    return PyLong_FromUnsignedLongLong(*opt_pc);
}

PyObject* py::threads::tid(core::Core& core, PyObject* args)
{
    auto* py_thread = static_cast<PyObject*>(nullptr);
    auto  ok        = PyArg_ParseTuple(args, "S", &py_thread);
    if(!ok)
        return nullptr;

    const auto opt_thread = py::from_bytes<thread_t>(py_thread);
    if(!opt_thread)
        return nullptr;

    const auto opt_proc = ::threads::process(core, *opt_thread);
    if(!opt_proc)
        return nullptr;

    const auto tid = ::threads::tid(core, *opt_proc, *opt_thread);
    return PyLong_FromUnsignedLongLong(tid);
}

namespace
{
    template <typename T>
    PyObject* on_listen(core::Core& core, PyObject* args, const T& operand)
    {
        auto*      py_func = static_cast<PyObject*>(nullptr);
        const auto ok      = PyArg_ParseTuple(args, "O", &py_func);
        if(!ok)
            return nullptr;

        if(!PyCallable_Check(py_func))
            return py::fail_with(nullptr, PyExc_TypeError, "arg must be callable");

        const auto opt_bpid = operand(core, [=](thread_t thread)
        {
            auto* py_thread = py::to_bytes(thread);
            auto* args      = Py_BuildValue("(O)", py_thread);
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
}

PyObject* py::threads::listen_create(core::Core& core, PyObject* args)
{
    return on_listen(core, args, &::threads::listen_create);
}

PyObject* py::threads::listen_delete(core::Core& core, PyObject* args)
{
    return on_listen(core, args, &::threads::listen_delete);
}
