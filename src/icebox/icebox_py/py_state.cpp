#include "bindings.hpp"

namespace
{
    PyObject* none_or_error(bool ok)
    {
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
