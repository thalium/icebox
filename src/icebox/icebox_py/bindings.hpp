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
    namespace state
    {
        PyObject*   pause       (core::Core& core, PyObject* args);
        PyObject*   resume      (core::Core& core, PyObject* args);
        PyObject*   single_step (core::Core& core, PyObject* args);
        PyObject*   wait        (core::Core& core, PyObject* args);
    } // namespace state

    namespace registers
    {
        PyObject*   list        (core::Core& core, PyObject* args);
        PyObject*   msr_list    (core::Core& core, PyObject* args);
        PyObject*   read        (core::Core& core, PyObject* args);
        PyObject*   write       (core::Core& core, PyObject* args);
        PyObject*   msr_read    (core::Core& core, PyObject* args);
        PyObject*   msr_write   (core::Core& core, PyObject* args);
    } // namespace registers

    namespace process
    {
        PyObject*   current         (core::Core& core, PyObject* args);
        PyObject*   name            (core::Core& core, PyObject* args);
        PyObject*   is_valid        (core::Core& core, PyObject* args);
        PyObject*   pid             (core::Core& core, PyObject* args);
        PyObject*   flags           (core::Core& core, PyObject* args);
        PyObject*   join            (core::Core& core, PyObject* args);
        PyObject*   parent          (core::Core& core, PyObject* args);
        PyObject*   list            (core::Core& core, PyObject* args);
        PyObject*   wait            (core::Core& core, PyObject* args);
        PyObject*   listen_create   (core::Core& core, PyObject* args);
        PyObject*   listen_delete   (core::Core& core, PyObject* args);
    } // namespace process

    namespace symbols
    {
        PyObject*   address     (core::Core& core, PyObject* args);
        PyObject*   struc_offset(core::Core& core, PyObject* args);
        PyObject*   struc_size  (core::Core& core, PyObject* args);
        PyObject*   string      (core::Core& core, PyObject* args);
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
