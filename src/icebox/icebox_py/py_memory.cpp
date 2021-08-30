#include "bindings.hpp"

PyObject* py::memory::virtual_to_physical(core::Core& core, PyObject* args)
{
    auto*      py_proc = static_cast<PyObject*>(nullptr);
    auto       ptr     = uint64_t{};
    const auto ok      = PyArg_ParseTuple(args, "OK", &py_proc, &ptr);
    if(!ok)
        return nullptr;

    const auto opt_proc = py::from_bytes<proc_t>(py_proc);
    if(!opt_proc)
        return nullptr;

    const auto opt_phy = ::memory::virtual_to_physical(core, *opt_proc, ptr);
    if(!opt_phy)
        Py_RETURN_NONE;

    return PyLong_FromUnsignedLongLong(opt_phy->val);
}

PyObject* py::memory::virtual_to_physical_with_dtb(core::Core& core, PyObject* args)
{
    auto       dtb = uint64_t{};
    auto       ptr = uint64_t{};
    const auto ok  = PyArg_ParseTuple(args, "KK", &dtb, &ptr);
    if(!ok)
        return nullptr;

    const auto opt_phy = ::memory::virtual_to_physical_with_dtb(core, dtb_t{dtb}, ptr);
    if(!opt_phy)
        Py_RETURN_NONE;

    return PyLong_FromUnsignedLongLong(opt_phy->val);
}

namespace
{
    enum class access_e
    {
        need_read,
        need_write
    };

    bool check_buffer(Py_buffer& buf, access_e eaccess)
    {
        if(eaccess == access_e::need_write)
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
    auto  buf     = Py_buffer{};
    auto* py_proc = static_cast<PyObject*>(nullptr);
    auto  src     = uint64_t{};
    auto  ok      = PyArg_ParseTuple(args, "y*OK", &buf, &py_proc, &src);
    if(!ok)
        return nullptr;

    DEFER([&] { PyBuffer_Release(&buf); });
    if(!check_buffer(buf, access_e::need_write))
        return nullptr;

    const auto opt_proc = py::from_bytes<proc_t>(py_proc);
    if(!opt_proc)
        return nullptr;

    ok = ::memory::read_virtual(core, *opt_proc, buf.buf, src, buf.len);
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

    DEFER([&] { PyBuffer_Release(&buf); });
    if(!check_buffer(buf, access_e::need_write))
        return nullptr;

    ok = ::memory::read_virtual_with_dtb(core, dtb_t{dtb}, buf.buf, src, buf.len);
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

    DEFER([&] { PyBuffer_Release(&buf); });
    if(!check_buffer(buf, access_e::need_write))
        return nullptr;

    ok = ::memory::read_physical(core, buf.buf, src, buf.len);
    if(!ok)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to read physical memory");

    Py_RETURN_NONE;
}

PyObject* py::memory::write_virtual(core::Core& core, PyObject* args)
{
    auto  buf     = Py_buffer{};
    auto* py_proc = static_cast<PyObject*>(nullptr);
    auto  dst     = uint64_t{};
    auto  ok      = PyArg_ParseTuple(args, "y*OK", &buf, &py_proc, &dst);
    if(!ok)
        return nullptr;

    DEFER([&] { PyBuffer_Release(&buf); });
    if(!check_buffer(buf, access_e::need_read))
        return nullptr;

    const auto opt_proc = py::from_bytes<proc_t>(py_proc);
    if(!opt_proc)
        return nullptr;

    ok = ::memory::write_virtual(core, *opt_proc, dst, buf.buf, buf.len);
    if(!ok)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to write virtual memory");

    Py_RETURN_NONE;
}

PyObject* py::memory::write_virtual_with_dtb(core::Core& core, PyObject* args)
{
    auto buf = Py_buffer{};
    auto dtb = uint64_t{};
    auto dst = uint64_t{};
    auto ok  = PyArg_ParseTuple(args, "y*OK", &buf, &dtb, &dst);
    if(!ok)
        return nullptr;

    DEFER([&] { PyBuffer_Release(&buf); });
    if(!check_buffer(buf, access_e::need_read))
        return nullptr;

    ok = ::memory::write_virtual_with_dtb(core, dtb_t{dtb}, dst, buf.buf, buf.len);
    if(!ok)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to write virtual memory");

    Py_RETURN_NONE;
}

PyObject* py::memory::write_physical(core::Core& core, PyObject* args)
{
    auto buf = Py_buffer{};
    auto dst = uint64_t{};
    auto ok  = PyArg_ParseTuple(args, "y*K", &buf, &dst);
    if(!ok)
        return nullptr;

    DEFER([&] { PyBuffer_Release(&buf); });
    if(!check_buffer(buf, access_e::need_read))
        return nullptr;

    ok = ::memory::write_physical(core, dst, buf.buf, buf.len);
    if(!ok)
        return py::fail_with(nullptr, PyExc_RuntimeError, "unable to write virtual memory");

    Py_RETURN_NONE;
}
