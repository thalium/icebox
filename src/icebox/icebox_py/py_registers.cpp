#include "bindings.hpp"

PyObject* py::registers::list(core::Core& /*core*/, PyObject* /*args*/)
{
    auto list = PyList_New(0);
    if(!list)
        return nullptr;

    PYREF(list);
    for(auto i = 0; i <= static_cast<size_t>(reg_e::last); ++i)
    {
        const auto arg     = static_cast<reg_e>(i);
        const auto strname = ::registers::to_string(arg);
        const auto name    = PyUnicode_FromStringAndSize(strname.data(), strname.size());
        if(!name)
            return nullptr;

        PYREF(name);
        const auto idx = PyLong_FromLong(i);
        if(!idx)
            return nullptr;

        PYREF(idx);
        const auto item = Py_BuildValue("(OO)", name, idx);
        if(!item)
            return nullptr;

        PYREF(item);
        const auto err = PyList_Append(list, item);
        if(err)
            return nullptr;
    }
    Py_INCREF(list);
    return list;
}

PyObject* py::registers::msr_list(core::Core& /*core*/, PyObject* /*args*/)
{
    auto list = PyList_New(0);
    if(!list)
        return nullptr;

    PYREF(list);
    for(auto i = 0; i <= static_cast<size_t>(msr_e::last); ++i)
    {
        const auto arg     = static_cast<msr_e>(i);
        const auto strname = ::registers::to_string(arg);
        const auto name    = PyUnicode_FromStringAndSize(strname.data(), strname.size());
        if(!name)
            return nullptr;

        PYREF(name);
        const auto idx = PyLong_FromLong(i);
        if(!idx)
            return nullptr;

        PYREF(idx);
        const auto item = Py_BuildValue("(OO)", name, idx);
        if(!item)
            return nullptr;

        PYREF(item);
        const auto err = PyList_Append(list, item);
        if(err)
            return nullptr;
    }
    Py_INCREF(list);
    return list;
}

namespace
{
    template <typename T, uint64_t (*Op)(core::Core&, T)>
    PyObject* reg_read(core::Core& core, PyObject* args)
    {
        auto reg_id   = int{};
        const auto ok = PyArg_ParseTuple(args, "i", &reg_id);
        if(!ok)
            return nullptr;

        const auto reg = static_cast<T>(reg_id);
        const auto ret = Op(core, reg);
        return PyLong_FromUnsignedLongLong(ret);
    }

    template <typename T, bool (*Op)(core::Core&, T, uint64_t)>
    PyObject* reg_write(core::Core& core, PyObject* args)
    {
        auto reg_id = int{};
        auto value  = uint64_t{};
        auto ok     = PyArg_ParseTuple(args, "iK", &reg_id, &value);
        if(!ok)
            return nullptr;

        const auto reg = static_cast<T>(reg_id);
        ok             = Op(core, reg, value);
        if(!ok)
            return py::fail_with(nullptr, PyExc_RuntimeError, "unable to write register");

        Py_RETURN_NONE;
    }
}

PyObject* py::registers::read(core::Core& core, PyObject* args)
{
    return reg_read<reg_e, &::registers::read>(core, args);
}

PyObject* py::registers::write(core::Core& core, PyObject* args)
{
    return reg_write<reg_e, &::registers::write>(core, args);
}

PyObject* py::registers::msr_read(core::Core& core, PyObject* args)
{
    return reg_read<msr_e, &::registers::read_msr>(core, args);
}

PyObject* py::registers::msr_write(core::Core& core, PyObject* args)
{
    return reg_write<msr_e, &::registers::write_msr>(core, args);
}
