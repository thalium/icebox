#pragma once

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

namespace py
{
    core::Core* from_self(PyObject* self);

    namespace state
    {
        PyObject*   pause       (PyObject* self, PyObject* args);
        PyObject*   resume      (PyObject* self, PyObject* args);
        PyObject*   single_step (PyObject* self, PyObject* args);
        PyObject*   wait        (PyObject* self, PyObject* args);
    } // namespace state

    namespace registers
    {
        PyObject*   list        (PyObject* self, PyObject* args);
        PyObject*   msr_list    (PyObject* self, PyObject* args);
        PyObject*   read        (PyObject* self, PyObject* args);
        PyObject*   write       (PyObject* self, PyObject* args);
        PyObject*   msr_read    (PyObject* self, PyObject* args);
        PyObject*   msr_write   (PyObject* self, PyObject* args);
    } // namespace registers

    namespace process
    {
        PyObject*   current         (PyObject* self, PyObject* args);
        PyObject*   name            (PyObject* self, PyObject* args);
        PyObject*   is_valid        (PyObject* self, PyObject* args);
        PyObject*   pid             (PyObject* self, PyObject* args);
        PyObject*   flags           (PyObject* self, PyObject* args);
        PyObject*   join            (PyObject* self, PyObject* args);
        PyObject*   parent          (PyObject* self, PyObject* args);
        PyObject*   list            (PyObject* self, PyObject* args);
        PyObject*   wait            (PyObject* self, PyObject* args);
        PyObject*   listen_create   (PyObject* self, PyObject* args);
        PyObject*   listen_delete   (PyObject* self, PyObject* args);
    } // namespace process

    namespace symbols
    {
        PyObject*   address     (PyObject* self, PyObject* args);
        PyObject*   struc_offset(PyObject* self, PyObject* args);
        PyObject*   struc_size  (PyObject* self, PyObject* args);
        PyObject*   string      (PyObject* self, PyObject* args);
    } // namespace symbols

    template <typename T>
    T fail_with(T ret, PyObject* err, const char* msg)
    {
        PyErr_SetString(err, msg);
        return ret;
    }

    PyObject* to_bytes(const char* ptr, size_t size);

    template <typename T>
    PyObject* to_bytes(const T& value)
    {
        const auto ptr = reinterpret_cast<const char*>(&value);
        return to_bytes(ptr, sizeof value);
    }

    const char* from_bytes(PyObject* self, size_t size);

    template <typename T>
    opt<T> from_bytes(PyObject* self)
    {
        auto ret       = T{};
        const auto ptr = from_bytes(self, sizeof(T));
        if(!ptr)
            return {};

        memcpy(&ret, ptr, sizeof ret);
        return ret;
    }
} // namespace py
