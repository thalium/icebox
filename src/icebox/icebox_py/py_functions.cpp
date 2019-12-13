#include "bindings.hpp"

PyObject* py::functions::read_stack(core::Core& core, PyObject* args)
{
    auto idx = uint64_t{};
    auto ok  = PyArg_ParseTuple(args, "K", &idx);
    if(!ok)
        return nullptr;

    const auto opt_arg = ::functions::read_stack(core, idx);
    if(!opt_arg)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to read stack");

    return PyLong_FromUnsignedLongLong(opt_arg->val);
}

PyObject* py::functions::read_arg(core::Core& core, PyObject* args)
{
    auto idx = uint64_t{};
    auto ok  = PyArg_ParseTuple(args, "K", &idx);
    if(!ok)
        return nullptr;

    const auto opt_arg = ::functions::read_arg(core, idx);
    if(!opt_arg)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to read arg");

    return PyLong_FromUnsignedLongLong(opt_arg->val);
}

PyObject* py::functions::write_arg(core::Core& core, PyObject* args)
{
    auto idx   = uint64_t{};
    auto value = uint64_t{};
    auto ok    = PyArg_ParseTuple(args, "KK", &idx, &value);
    if(!ok)
        return nullptr;

    ok = ::functions::write_arg(core, idx, arg_t{value});
    if(!ok)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to write arg");

    Py_RETURN_NONE;
}

PyObject* py::functions::break_on_return(core::Core& core, PyObject* args)
{
    auto name    = static_cast<const char*>(nullptr);
    auto py_func = static_cast<PyObject*>(nullptr);
    auto ok      = PyArg_ParseTuple(args, "sO", &name, &py_func);
    if(!ok)
        return nullptr;

    name = name ? name : "";
    if(!PyCallable_Check(py_func))
        return py::fail_with(nullptr, PyExc_TypeError, "arg must be callable");

    Py_INCREF(py_func);
    ok = ::functions::break_on_return(core, name, [=]
    {
        const auto args = Py_BuildValue("()");
        if(!args)
            return;

        PY_DEFER_DECREF(args);
        PY_DEFER_DECREF(py_func);
        const auto ret = PyEval_CallObject(py_func, args);
        if(ret)
            PY_DEFER_DECREF(ret);
    });
    if(!ok)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to break on return");

    Py_RETURN_NONE;
}
