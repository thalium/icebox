#include "bindings.hpp"

namespace
{
    template <bool (*Op)(core::Core&)>
    PyObject* core_exec(PyObject* self, PyObject* /*args*/)
    {
        auto core = py::from_self(self);
        if(!core)
            return nullptr;

        const auto ok = Op(*core);
        if(!ok)
            return py::fail_with(nullptr, PyExc_RuntimeError, "error");

        Py_RETURN_NONE;
    }
}

PyObject* py::state::pause(PyObject* self, PyObject* args)
{
    return core_exec<&::state::pause>(self, args);
}

PyObject* py::state::resume(PyObject* self, PyObject* args)
{
    return core_exec<&::state::resume>(self, args);
}

PyObject* py::state::single_step(PyObject* self, PyObject* args)
{
    return core_exec<&::state::single_step>(self, args);
}

PyObject* py::state::wait(PyObject* self, PyObject* args)
{
    return core_exec<&::state::wait>(self, args);
}
