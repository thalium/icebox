#define PY_SSIZE_T_CLEAN
#ifndef DEBUG
#    define Py_LIMITED_API 0x03030000
#endif
// must be included first
#include <python.h>

#include "defer.hpp"

#define FDP_MODULE "py"
#include <icebox/core.hpp>
#include <icebox/log.hpp>

#include <array>

namespace
{
    struct Handle
    {
        std::shared_ptr<core::Core> core;
    };

    Handle* handle_from_self(PyObject* self)
    {
        if(self)
            if(const auto ptr = PyModule_GetState(self))
                return static_cast<Handle*>(ptr);

        PyErr_SetString(PyExc_RuntimeError, "missing module handle");
        return nullptr;
    }

    void delete_handle(void* ptr)
    {
        if(!ptr)
            return;

        auto handle = static_cast<Handle*>(ptr);
        handle->~Handle();
    }

    template <typename T>
    T fail_with(T ret, PyObject* err, const char* msg)
    {
        PyErr_SetString(err, msg);
        return ret;
    }

    PyObject* core_attach(PyObject* self, PyObject* args)
    {
        auto name     = static_cast<const char*>(nullptr);
        const auto ok = PyArg_ParseTuple(args, "s", &name);
        if(!ok)
            return nullptr;

        auto handle = handle_from_self(self);
        if(!handle)
            return nullptr;

        if(handle->core)
            return fail_with(nullptr, PyExc_RuntimeError, "already attached");

        const auto core = core::attach(name ? name : "");
        if(!core)
            return fail_with(nullptr, PyExc_RuntimeError, "unable to attach");

        new(handle) Handle{core};
        Py_RETURN_NONE;
    }

    PyObject* core_detach(PyObject* self, PyObject* /*args*/)
    {
        auto handle = handle_from_self(self);
        if(!handle)
            return nullptr;

        handle->core.reset();
        Py_RETURN_NONE;
    }

    core::Core* core_from_self(PyObject* self)
    {
        const auto handle = handle_from_self(self);
        if(!handle)
            return nullptr;

        if(!handle->core)
            return fail_with(nullptr, PyExc_RuntimeError, "not attached to any vm");

        return handle->core.get();
    }

    template <bool (*Op)(core::Core&)>
    PyObject* core_exec(PyObject* self, PyObject* /*args*/)
    {
        auto core = core_from_self(self);
        if(!core)
            return nullptr;

        const auto ok = Op(*core);
        if(!ok)
            return fail_with(nullptr, PyExc_RuntimeError, "error");

        Py_RETURN_NONE;
    }

    PyObject* register_list(PyObject* /*self*/, PyObject* /*args*/)
    {
        auto list = PyList_New(0);
        if(!list)
            return nullptr;

        PYREF(list);
        for(auto i = 0; i <= static_cast<size_t>(reg_e::last); ++i)
        {
            const auto arg     = static_cast<reg_e>(i);
            const auto strname = registers::to_string(arg);
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

    PyObject* msr_list(PyObject* /*self*/, PyObject* /*args*/)
    {
        auto list = PyList_New(0);
        if(!list)
            return nullptr;

        PYREF(list);
        for(auto i = 0; i <= static_cast<size_t>(msr_e::last); ++i)
        {
            const auto arg     = static_cast<msr_e>(i);
            const auto strname = registers::to_string(arg);
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

    template <typename T, uint64_t (*Op)(core::Core&, T)>
    PyObject* reg_read(PyObject* self, PyObject* args)
    {
        auto core = core_from_self(self);
        if(!core)
            return nullptr;

        auto reg_id   = int{};
        const auto ok = PyArg_ParseTuple(args, "i", &reg_id);
        if(!ok)
            return nullptr;

        const auto reg = static_cast<T>(reg_id);
        const auto ret = Op(*core, reg);
        return PyLong_FromUnsignedLongLong(ret);
    }

    template <typename T, bool (*Op)(core::Core&, T, uint64_t)>
    PyObject* reg_write(PyObject* self, PyObject* args)
    {
        auto core = core_from_self(self);
        if(!core)
            return nullptr;

        auto reg_id = int{};
        auto value  = uint64_t{};
        auto ok     = PyArg_ParseTuple(args, "iK", &reg_id, &value);
        if(!ok)
            return nullptr;

        const auto reg = static_cast<T>(reg_id);
        ok             = Op(*core, reg, value);
        if(!ok)
            return fail_with(nullptr, PyExc_RuntimeError, "unable to write register");

        Py_RETURN_NONE;
    }

    PyObject* register_read(PyObject* self, PyObject* args)
    {
        return reg_read<reg_e, &registers::read>(self, args);
    }

    PyObject* register_write(PyObject* self, PyObject* args)
    {
        return reg_write<reg_e, &registers::write>(self, args);
    }

    PyObject* msr_read(PyObject* self, PyObject* args)
    {
        return reg_read<msr_e, &registers::read_msr>(self, args);
    }

    PyObject* msr_write(PyObject* self, PyObject* args)
    {
        return reg_write<msr_e, &registers::write_msr>(self, args);
    }

    template <typename T>
    PyObject* to_bytes(const T& value)
    {
        const auto ptr = reinterpret_cast<const char*>(&value);
        const auto obj = PyBytes_FromStringAndSize(ptr, sizeof value);
        return obj;
    }

    template <typename T>
    opt<T> from_bytes(PyObject* self)
    {
        if(!PyBytes_CheckExact(self))
            return fail_with(ext::nullopt, PyExc_RuntimeError, "invalid argument");

        auto ptr       = static_cast<char*>(nullptr);
        auto len       = ssize_t{};
        const auto err = PyBytes_AsStringAndSize(self, &ptr, &len);
        if(err)
            return fail_with(ext::nullopt, PyExc_RuntimeError, "invalid argument");

        if(len != sizeof(T))
            return fail_with(ext::nullopt, PyExc_RuntimeError, "invalid argument");

        auto ret = T{};
        memcpy(&ret, ptr, sizeof ret);
        return ret;
    }

    PyObject* process_current(PyObject* self, PyObject* /*args*/)
    {
        auto core = core_from_self(self);
        if(!core)
            return nullptr;

        const auto opt_proc = process::current(*core);
        if(!opt_proc)
            return fail_with(nullptr, PyExc_RuntimeError, "unable to read current process");

        return to_bytes(*opt_proc);
    }

    PyObject* process_name(PyObject* self, PyObject* args)
    {
        auto core = core_from_self(self);
        if(!core)
            return nullptr;

        auto obj = static_cast<PyObject*>(nullptr);
        auto ok  = PyArg_ParseTuple(args, "S", &obj);
        if(!ok)
            return nullptr;

        const auto opt_proc = from_bytes<proc_t>(obj);
        if(!opt_proc)
            return nullptr;

        const auto opt_name = process::name(*core, *opt_proc);
        if(!opt_name)
            return fail_with(nullptr, PyExc_RuntimeError, "unable to read process name");

        return PyUnicode_FromStringAndSize(opt_name->data(), opt_name->size());
    }

    PyObject* process_is_valid(PyObject* self, PyObject* args)
    {
        auto core = core_from_self(self);
        if(!core)
            return nullptr;

        auto obj = static_cast<PyObject*>(nullptr);
        auto ok  = PyArg_ParseTuple(args, "S", &obj);
        if(!ok)
            return nullptr;

        const auto opt_proc = from_bytes<proc_t>(obj);
        if(!opt_proc)
            return nullptr;

        ok = process::is_valid(*core, *opt_proc);
        if(!ok)
            Py_RETURN_FALSE;

        Py_RETURN_TRUE;
    }

    PyObject* process_pid(PyObject* self, PyObject* args)
    {
        auto core = core_from_self(self);
        if(!core)
            return nullptr;

        auto obj = static_cast<PyObject*>(nullptr);
        auto ok  = PyArg_ParseTuple(args, "S", &obj);
        if(!ok)
            return nullptr;

        const auto opt_proc = from_bytes<proc_t>(obj);
        if(!opt_proc)
            return nullptr;

        const auto pid = process::pid(*core, *opt_proc);
        return PyLong_FromUnsignedLongLong(pid);
    }

    PyObject* process_flags(PyObject* self, PyObject* args)
    {
        auto core = core_from_self(self);
        if(!core)
            return nullptr;

        auto obj = static_cast<PyObject*>(nullptr);
        auto ok  = PyArg_ParseTuple(args, "S", &obj);
        if(!ok)
            return nullptr;

        const auto opt_proc = from_bytes<proc_t>(obj);
        if(!opt_proc)
            return nullptr;

        const auto flags = process::flags(*core, *opt_proc);
        return Py_BuildValue("{s:O,s:O}",
                             "is_x86", flags.is_x86 ? Py_True : Py_False,
                             "is_x64", flags.is_x64 ? Py_True : Py_False);
    }

    PyObject* process_join(PyObject* self, PyObject* args)
    {
        auto core = core_from_self(self);
        if(!core)
            return nullptr;

        auto obj     = static_cast<PyObject*>(nullptr);
        auto strmode = static_cast<const char*>(nullptr);
        auto ok      = PyArg_ParseTuple(args, "Ss", &obj, &strmode);
        if(!ok)
            return nullptr;

        const auto opt_proc = from_bytes<proc_t>(obj);
        if(!opt_proc)
            return nullptr;

        const auto mode = strmode && strmode == std::string_view{"kernel"} ? mode_e::kernel : mode_e::user;
        process::join(*core, *opt_proc, mode);
        Py_RETURN_NONE;
    }

    PyObject* process_parent(PyObject* self, PyObject* args)
    {
        auto core = core_from_self(self);
        if(!core)
            return nullptr;

        auto obj = static_cast<PyObject*>(nullptr);
        auto ok  = PyArg_ParseTuple(args, "S", &obj);
        if(!ok)
            return nullptr;

        const auto opt_proc = from_bytes<proc_t>(obj);
        if(!opt_proc)
            return nullptr;

        const auto opt_parent = process::parent(*core, *opt_proc);
        if(!opt_parent)
            Py_RETURN_NONE;

        return to_bytes(*opt_parent);
    }

    PyObject* process_list(PyObject* self, PyObject* /*args*/)
    {
        auto core = core_from_self(self);
        if(!core)
            return nullptr;

        auto list = PyList_New(0);
        if(!list)
            return nullptr;

        PYREF(list);
        const auto ok = process::list(*core, [&](proc_t proc)
        {
            auto item = to_bytes(proc);
            if(!item)
                return walk_e::stop;

            PYREF(item);
            const auto err = PyList_Append(list, item);
            if(err)
                return walk_e::stop;

            return walk_e::next;
        });
        if(!ok)
            return fail_with(nullptr, PyExc_RuntimeError, "unable to list processes");

        Py_INCREF(list);
        return list;
    }

    opt<flags_t> to_flags(PyObject* arg)
    {
        auto flags = flags_t{};
        auto attr  = PyObject_GetAttrString(arg, "is_x64");
        if(!attr || !PyBool_Check(attr))
            return fail_with(ext::nullopt, PyExc_RuntimeError, "missing or invalid is_x64 attribute");

        flags.is_x64 = PyLong_AsLong(attr);
        attr         = PyObject_GetAttrString(arg, "is_x86");
        if(!attr || !PyBool_Check(attr))
            return fail_with(ext::nullopt, PyExc_RuntimeError, "missing or invalid is_x86 attribute");

        flags.is_x86 = PyLong_AsLong(attr);
        return flags;
    }

    PyObject* process_wait(PyObject* self, PyObject* args)
    {
        auto core = core_from_self(self);
        if(!core)
            return nullptr;

        auto name     = static_cast<const char*>(nullptr);
        auto py_flags = static_cast<PyObject*>(nullptr);
        const auto ok = PyArg_ParseTuple(args, "sO", &name, &py_flags);
        if(!ok)
            return nullptr;

        name                 = name ? name : "";
        const auto opt_flags = to_flags(py_flags);
        const auto opt_proc  = process::wait(*core, name, *opt_flags);
        if(!opt_proc)
            return fail_with(nullptr, PyExc_RuntimeError, "unable to wait for process");

        return to_bytes(*opt_proc);
    }
}

PyMODINIT_FUNC PyInit__icebox()
{
    auto args       = std::array<char*, 2>{"_icebox", nullptr};
    const auto argc = static_cast<int>(args.size());
    logg::init(argc - 1, &args[0]);

    static auto ice_methods = std::array<PyMethodDef, 32>{{
        {"attach", &core_attach, METH_VARARGS, "attach vm <name>"},
        {"detach", &core_detach, METH_NOARGS, "detach from vm"},
        {"pause", &core_exec<&state::pause>, METH_NOARGS, "pause vm"},
        {"resume", &core_exec<&state::resume>, METH_NOARGS, "resume vm"},
        {"single_step", &core_exec<&state::single_step>, METH_NOARGS, "execute a single instruction"},
        {"wait", &core_exec<&state::wait>, METH_NOARGS, "wait vm"},
        {"register_list", &register_list, METH_NOARGS, "list available registers"},
        {"register_read", &register_read, METH_VARARGS, "read register"},
        {"register_write", &register_write, METH_VARARGS, "write register"},
        {"msr_read", &msr_read, METH_VARARGS, "read msr register"},
        {"msr_write", &msr_write, METH_VARARGS, "write msr register"},
        {"msr_list", &msr_list, METH_NOARGS, "list available msr registers"},
        {"process_current", &process_current, METH_NOARGS, "read current process"},
        {"process_name", &process_name, METH_VARARGS, "read process name"},
        {"process_is_valid", &process_is_valid, METH_VARARGS, "check if process is valid"},
        {"process_pid", &process_pid, METH_VARARGS, "read process pid"},
        {"process_flags", &process_flags, METH_VARARGS, "read process flags"},
        {"process_join", &process_join, METH_VARARGS, "join process"},
        {"process_parent", &process_parent, METH_VARARGS, "read process parent, if any"},
        {"process_list", &process_list, METH_NOARGS, "list available processes"},
        {"process_wait", &process_wait, METH_VARARGS, "wait for process"},
        {nullptr, nullptr, 0, nullptr},
    }};
    static auto ice_module  = PyModuleDef{
        PyModuleDef_HEAD_INIT,                     // m_base
        "_icebox",                                 // m_name
        "Python interface for the icebox library", // m_doc
        sizeof(Handle),                            // m_size
        &ice_methods[0],                           // m_methods
        nullptr,                                   // m_slots
        nullptr,                                   // m_traverse
        nullptr,                                   // m_clear
        &delete_handle,                            // m_free
    };
    const auto ptr = PyModule_Create(&ice_module);
    if(!ptr)
        return nullptr;

    auto handle = handle_from_self(ptr);
    if(!handle)
        return nullptr;

    new(handle) Handle{{}};
    return ptr;
}
