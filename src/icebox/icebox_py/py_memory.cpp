#include "bindings.hpp"

PyObject* py::memory::virtual_to_physical(core::Core& core, PyObject* args)
{
    auto obj      = static_cast<PyObject*>(nullptr);
    auto ptr      = uint64_t{};
    const auto ok = PyArg_ParseTuple(args, "OK", &obj, &ptr);
    if(!ok)
        return nullptr;

    const auto opt_proc = py::from_bytes<proc_t>(obj);
    if(!opt_proc)
        return nullptr;

    const auto dtb     = os::is_kernel_address(core, ptr) ? opt_proc->kdtb : opt_proc->udtb;
    const auto opt_phy = ::memory::virtual_to_physical(core, ptr, dtb);
    if(!opt_phy)
        Py_RETURN_NONE;

    return PyLong_FromUnsignedLongLong(opt_phy->val);
}

namespace
{
    bool check_buffer(Py_buffer& buf)
    {
        if(buf.readonly)
            return py::fail_with(false, PyExc_RuntimeError, "output buffer is readonly");

        if(buf.ndim != 1)
            return py::fail_with(false, PyExc_RuntimeError, "output buffer is multi-dimensional");

        if(!PyBuffer_IsContiguous(&buf, 'C'))
            return py::fail_with(false, PyExc_RuntimeError, "output buffer is not contiguous");

        return true;
    }
}

PyObject* py::memory::read_virtual(core::Core& core, PyObject* args)
{
    auto buf = Py_buffer{};
    auto src = uint64_t{};
    auto ok  = PyArg_ParseTuple(args, "y*K", &buf, &src);
    if(!ok)
        return nullptr;

    ok = check_buffer(buf)
         && ::memory::read_virtual(core, buf.buf, src, buf.len);
    PyBuffer_Release(&buf);
    if(!ok)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to read virtual memory");

    Py_RETURN_NONE;
}

PyObject* py::memory::read_virtual_with_dtb(core::Core& core, PyObject* args)
{
    auto buf = Py_buffer{};
    auto dtb = uint64_t{};
    auto src = uint64_t{};
    auto ok  = PyArg_ParseTuple(args, "y*KK", &buf, &dtb, &src);
    if(!ok)
        return nullptr;

    ok = check_buffer(buf)
         && ::memory::read_virtual_with_dtb(core, buf.buf, dtb_t{dtb}, src, buf.len);
    PyBuffer_Release(&buf);
    if(!ok)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to read virtual memory");

    Py_RETURN_NONE;
}

PyObject* py::memory::read_physical(core::Core& core, PyObject* args)
{
    auto buf = Py_buffer{};
    auto src = uint64_t{};
    auto ok  = PyArg_ParseTuple(args, "y*K", &buf, &src);
    if(!ok)
        return nullptr;

    ok = check_buffer(buf)
         && ::memory::read_physical(core, buf.buf, src, buf.len);
    PyBuffer_Release(&buf);
    if(!ok)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to read virtual memory");

    Py_RETURN_NONE;
}
