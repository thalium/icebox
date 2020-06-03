#include "bindings.hpp"

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
            return py::fail_with(nullptr, PyExc_RuntimeError, "already attached");

        const auto core = core::attach(name ? name : "");
        if(!core)
            return py::fail_with(nullptr, PyExc_RuntimeError, "unable to attach");

        new(handle) Handle{core};
        Py_RETURN_NONE;
    }

    PyObject* core_attach_only(PyObject* self, PyObject* args)
    {
        auto name     = static_cast<const char*>(nullptr);
        const auto ok = PyArg_ParseTuple(args, "s", &name);
        if(!ok)
            return nullptr;

        auto handle = handle_from_self(self);
        if(!handle)
            return nullptr;

        if(handle->core)
            return py::fail_with(nullptr, PyExc_RuntimeError, "already attached");

        const auto core = core::attach_only(name ? name : "");
        if(!core)
            return py::fail_with(nullptr, PyExc_RuntimeError, "unable to attach");

        new(handle) Handle{core};
        Py_RETURN_NONE;
    }

    PyObject* core_detect(PyObject* self, PyObject* /*args*/)
    {
        auto handle = handle_from_self(self);
        if(!handle)
            return nullptr;

        const auto ok = core::detect(*handle->core);
        if(!ok)
            Py_RETURN_FALSE;

        Py_RETURN_TRUE;
    }

    PyObject* core_detach(PyObject* self, PyObject* /*args*/)
    {
        auto handle = handle_from_self(self);
        if(!handle)
            return nullptr;

        handle->core.reset();
        Py_RETURN_NONE;
    }

    template <PyObject* (*Op)(core::Core&, PyObject*)>
    PyObject* core_exec(PyObject* self, PyObject* args)
    {
        const auto handle = handle_from_self(self);
        if(!handle)
            return nullptr;

        if(!handle->core)
            return py::fail_with(nullptr, PyExc_RuntimeError, "not attached to any vm");

        return Op(*handle->core, args);
    }

    PyObject* log_redirect(PyObject* /*self*/, PyObject* args)
    {
        auto py_func = static_cast<PyObject*>(nullptr);
        auto ok      = PyArg_ParseTuple(args, "O", &py_func);
        if(!ok)
            return nullptr;

        if(!PyCallable_Check(py_func))
            return py::fail_with(nullptr, PyExc_TypeError, "arg must be callable");

        Py_INCREF(py_func); // FIXME leak ?
        logg::redirect([=](logg::level_t level, const char* fmt)
        {
            const auto args = Py_BuildValue("(Ks)", (uint64_t) level, fmt);
            if(!args)
                return;

            PY_DEFER_DECREF(args);
            const auto ret = PyEval_CallObject(py_func, args);
            if(ret)
                PY_DEFER_DECREF(ret);
        });
        Py_RETURN_NONE;
    }
}

PyObject* py::to_bytes(const char* ptr, size_t size)
{
    return PyBytes_FromStringAndSize(ptr, size);
}

const char* py::from_bytes(PyObject* self, size_t size)
{
    if(!PyBytes_CheckExact(self))
        return py::fail_with(nullptr, PyExc_RuntimeError, "invalid argument");

    auto src       = static_cast<char*>(nullptr);
    auto len       = ssize_t{};
    const auto err = PyBytes_AsStringAndSize(self, &src, &len);
    if(err)
        return py::fail_with(nullptr, PyExc_RuntimeError, "invalid argument");

    if(len != static_cast<ssize_t>(size))
        return py::fail_with(nullptr, PyExc_RuntimeError, "invalid argument");

    return src;
}

namespace
{
    constexpr auto ice_methods = std::array<PyMethodDef, 128>{{
        {"attach", core_attach, METH_VARARGS, "attach vm <name>"},
        {"attach_only", core_attach_only, METH_VARARGS, "attach_only vm <name>"},
        {"detect", core_detect, METH_NOARGS, "detect os"},
        {"detach", core_detach, METH_NOARGS, "detach from vm"},
        {"log_redirect", log_redirect, METH_VARARGS, "redirect logs"},
        // state
        {"pause", &core_exec<&py::state::pause>, METH_NOARGS, "pause vm"},
        {"resume", &core_exec<&py::state::resume>, METH_NOARGS, "resume vm"},
        {"single_step", &core_exec<&py::state::single_step>, METH_NOARGS, "execute a single instruction"},
        {"wait", &core_exec<&py::state::wait>, METH_NOARGS, "wait vm"},
        {"break_on", &core_exec<&py::state::break_on>, METH_VARARGS, "break on virtual address"},
        {"break_on_process", &core_exec<&py::state::break_on_process>, METH_VARARGS, "break process on address"},
        {"break_on_thread", &core_exec<&py::state::break_on_thread>, METH_VARARGS, "break thread on address"},
        {"break_on_physical", &core_exec<&py::state::break_on_physical>, METH_VARARGS, "break on physical address"},
        {"break_on_physical_process", &core_exec<&py::state::break_on_physical_process>, METH_VARARGS, "break process on physical address"},
        {"drop_breakpoint", &core_exec<&py::state::drop_breakpoint>, METH_VARARGS, "drop breakpoint"},
        // registers
        {"msr_list", &core_exec<&py::registers::msr_list>, METH_NOARGS, "list available msr registers"},
        {"msr_read", &core_exec<&py::registers::msr_read>, METH_VARARGS, "read msr register"},
        {"msr_write", &core_exec<&py::registers::msr_write>, METH_VARARGS, "write msr register"},
        {"register_list", &core_exec<&py::registers::list>, METH_NOARGS, "list available registers"},
        {"register_read", &core_exec<&py::registers::read>, METH_VARARGS, "read register"},
        {"register_write", &core_exec<&py::registers::write>, METH_VARARGS, "write register"},
        // memory
        {"memory_virtual_to_physical", &core_exec<&py::memory::virtual_to_physical>, METH_VARARGS, "convert virtual address to physical"},
        {"memory_virtual_to_physical_with_dtb", &core_exec<&py::memory::virtual_to_physical_with_dtb>, METH_VARARGS, "convert virtual address to physical with dtb"},
        {"memory_read_virtual", &core_exec<&py::memory::read_virtual>, METH_VARARGS, "read virtual memory"},
        {"memory_read_virtual_with_dtb", &core_exec<&py::memory::read_virtual_with_dtb>, METH_VARARGS, "read virtual memory with dtb"},
        {"memory_read_physical", &core_exec<&py::memory::read_physical>, METH_VARARGS, "read physical memory"},
        {"memory_write_virtual", &core_exec<&py::memory::write_virtual>, METH_VARARGS, "write virtual memory"},
        {"memory_write_virtual_with_dtb", &core_exec<&py::memory::write_virtual_with_dtb>, METH_VARARGS, "write virtual memory with dtb"},
        {"memory_write_physical", &core_exec<&py::memory::write_physical>, METH_VARARGS, "write physical memory"},
        // process
        {"process_current", &core_exec<&py::process::current>, METH_NOARGS, "read current process"},
        {"process_flags", &core_exec<&py::process::flags>, METH_VARARGS, "read process flags"},
        {"process_is_valid", &core_exec<&py::process::is_valid>, METH_VARARGS, "check if process is valid"},
        {"process_join", &core_exec<&py::process::join>, METH_VARARGS, "join process"},
        {"process_list", &core_exec<&py::process::list>, METH_NOARGS, "list available processes"},
        {"process_listen_create", &core_exec<&py::process::listen_create>, METH_VARARGS, "listen on process creation"},
        {"process_listen_delete", &core_exec<&py::process::listen_delete>, METH_VARARGS, "listen on process deletion"},
        {"process_native", &core_exec<&py::process::native>, METH_VARARGS, "read native process"},
        {"process_kdtb", &core_exec<&py::process::kdtb>, METH_VARARGS, "read kernel Directory Table Base"},
        {"process_udtb", &core_exec<&py::process::udtb>, METH_VARARGS, "read user Directory Table Base"},
        {"process_name", &core_exec<&py::process::name>, METH_VARARGS, "read process name"},
        {"process_parent", &core_exec<&py::process::parent>, METH_VARARGS, "read process parent, if any"},
        {"process_pid", &core_exec<&py::process::pid>, METH_VARARGS, "read process pid"},
        {"process_wait", &core_exec<&py::process::wait>, METH_VARARGS, "wait for process"},
        // threads
        {"thread_list", &core_exec<&py::threads::list>, METH_VARARGS, "list process threads"},
        {"thread_current", &core_exec<&py::threads::current>, METH_NOARGS, "read current thread"},
        {"thread_process", &core_exec<&py::threads::process>, METH_VARARGS, "read thread process"},
        {"thread_program_counter", &core_exec<&py::threads::program_counter>, METH_VARARGS, "read thread program counter"},
        {"thread_tid", &core_exec<&py::threads::tid>, METH_VARARGS, "read thread tid"},
        {"thread_listen_create", &core_exec<&py::threads::listen_create>, METH_VARARGS, "listen on thread creation"},
        {"thread_listen_delete", &core_exec<&py::threads::listen_delete>, METH_VARARGS, "listen on thread deletion"},
        // modules
        {"modules_list", &core_exec<&py::modules::list>, METH_VARARGS, "list process modules"},
        {"modules_name", &core_exec<&py::modules::name>, METH_VARARGS, "read module name"},
        {"modules_span", &core_exec<&py::modules::span>, METH_VARARGS, "read module span"},
        {"modules_flags", &core_exec<&py::modules::flags>, METH_VARARGS, "read module flags"},
        {"modules_find", &core_exec<&py::modules::find>, METH_VARARGS, "find module from address"},
        {"modules_find_name", &core_exec<&py::modules::find_name>, METH_VARARGS, "find module from address"},
        {"modules_listen_create", &core_exec<&py::modules::listen_create>, METH_VARARGS, "listen on module creation"},
        // drivers
        {"drivers_list", &core_exec<&py::drivers::list>, METH_NOARGS, "list drivers"},
        {"drivers_name", &core_exec<&py::drivers::name>, METH_VARARGS, "read driver name"},
        {"drivers_span", &core_exec<&py::drivers::span>, METH_VARARGS, "read driver span"},
        {"drivers_find", &core_exec<&py::drivers::find>, METH_VARARGS, "find driver from address"},
        {"drivers_listen", &core_exec<&py::drivers::listen>, METH_VARARGS, "listen on driver events"},
        // symbols
        {"symbols_load_module_memory", &core_exec<&py::symbols::load_module_memory>, METH_VARARGS, "load module symbols from memory"},
        {"symbols_load_module", &core_exec<&py::symbols::load_module>, METH_VARARGS, "load module symbols from name"},
        {"symbols_load_modules", &core_exec<&py::symbols::load_modules>, METH_VARARGS, "load all module symbols from process"},
        {"symbols_load_driver_memory", &core_exec<&py::symbols::load_driver_memory>, METH_VARARGS, "load driver symbols from memory"},
        {"symbols_load_driver", &core_exec<&py::symbols::load_driver>, METH_VARARGS, "load driver symbols from name"},
        {"symbols_load_drivers", &core_exec<&py::symbols::load_drivers>, METH_VARARGS, "load all driver symbols"},
        {"symbols_autoload_modules", &core_exec<&py::symbols::autoload_modules>, METH_VARARGS, "auto-load module symbols from process"},
        {"symbols_address", &core_exec<&py::symbols::address>, METH_VARARGS, "read symbols address"},
        {"symbols_list_strucs", &core_exec<&py::symbols::list_strucs>, METH_VARARGS, "list structs"},
        {"symbols_read_struc", &core_exec<&py::symbols::read_struc>, METH_VARARGS, "read struc"},
        {"symbols_string", &core_exec<&py::symbols::string>, METH_VARARGS, "convert address to symbol string"},
        // functions
        {"functions_read_stack", &core_exec<&py::functions::read_stack>, METH_VARARGS, "read stack value"},
        {"functions_read_arg", &core_exec<&py::functions::read_arg>, METH_VARARGS, "read arg value"},
        {"functions_write_arg", &core_exec<&py::functions::write_arg>, METH_VARARGS, "write arg value"},
        {"functions_break_on_return", &core_exec<&py::functions::break_on_return>, METH_VARARGS, "break on function return"},
        // callstacks
        {"callstacks_read", &core_exec<&py::callstacks::read>, METH_VARARGS, "read callstack"},
        {"callstacks_load_module", &core_exec<&py::callstacks::load_module>, METH_VARARGS, "load module unwind data"},
        {"callstacks_load_driver", &core_exec<&py::callstacks::load_driver>, METH_VARARGS, "load driver unwind data"},
        {"callstacks_autoload_modules", &core_exec<&py::callstacks::autoload_modules>, METH_VARARGS, "autoload process module unwind data"},
        // vm_area
        {"vm_area_list", &core_exec<&py::vm_area::list>, METH_VARARGS, "list process vm areas"},
        {"vm_area_span", &core_exec<&py::vm_area::span>, METH_VARARGS, "read vm area span"},
        // null terminated
        {nullptr, nullptr, 0, nullptr},
    }};
}

PyMODINIT_FUNC PyInit__icebox()
{
    char main[]     = "_icebox";
    auto args       = std::array<char*, 2>{main, nullptr};
    const auto argc = static_cast<int>(args.size());
    logg::init(argc - 1, &args[0]);

    static auto ice_module = PyModuleDef{
        PyModuleDef_HEAD_INIT,                     // m_base
        "_icebox",                                 // m_name
        "Python interface for the icebox library", // m_doc
        sizeof(Handle),                            // m_size
        const_cast<PyMethodDef*>(&ice_methods[0]), // m_methods
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
