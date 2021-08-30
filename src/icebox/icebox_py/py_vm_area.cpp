#include "bindings.hpp"

PyObject* py::vm_area::list(core::Core& core, PyObject* args)
{
    auto*      obj = static_cast<PyObject*>(nullptr);
    const auto ok  = PyArg_ParseTuple(args, "S", &obj);
    if(!ok)
        return nullptr;

    const auto opt_proc = py::from_bytes<proc_t>(obj);
    if(!opt_proc)
        return nullptr;

    auto* list = PyList_New(0);
    if(!list)
        return nullptr;

    PY_DEFER_DECREF(list);
    ::vm_area::list(core, *opt_proc, [&](vm_area_t vma)
    {
        auto* item = py::to_bytes(vma);
        if(!item)
            return walk_e::stop;

        PY_DEFER_DECREF(item);
        const auto err = PyList_Append(list, item);
        if(err)
            return walk_e::stop;

        return walk_e::next;
    });
    Py_INCREF(list);
    return list;
}

PyObject* py::vm_area::span(core::Core& core, PyObject* args)
{
    auto* py_proc = static_cast<PyObject*>(nullptr);
    auto* py_vma  = static_cast<PyObject*>(nullptr);
    auto  ok      = PyArg_ParseTuple(args, "SS", &py_proc, &py_vma);
    if(!ok)
        return nullptr;

    const auto opt_proc = py::from_bytes<proc_t>(py_proc);
    if(!py_proc)
        return nullptr;

    const auto opt_vma = py::from_bytes<vm_area_t>(py_vma);
    if(!opt_vma)
        return nullptr;

    const auto opt_span = ::vm_area::span(core, *opt_proc, *opt_vma);
    if(!opt_span)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to read vm area span");

    return Py_BuildValue("(KK)", opt_span->addr, opt_span->size);
}
