#pragma once

#define PY_SSIZE_T_CLEAN
// must be included first
#include <Python.h>

#include "defer.hpp"

#define FDP_MODULE "py"
#include <icebox/core.hpp>
#include <icebox/log.hpp>

namespace py
{
    namespace state
    {
        PyObject*   pause                       (core::Core& core, PyObject* args);
        PyObject*   resume                      (core::Core& core, PyObject* args);
        PyObject*   single_step                 (core::Core& core, PyObject* args);
        PyObject*   wait                        (core::Core& core, PyObject* args);
        PyObject*   break_on                    (core::Core& core, PyObject* args);
        PyObject*   break_on_process            (core::Core& core, PyObject* args);
        PyObject*   break_on_thread             (core::Core& core, PyObject* args);
        PyObject*   break_on_physical           (core::Core& core, PyObject* args);
        PyObject*   break_on_physical_process   (core::Core& core, PyObject* args);
        PyObject*   drop_breakpoint             (core::Core& core, PyObject* args);
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

    namespace memory
    {
        PyObject*   virtual_to_physical         (core::Core& core, PyObject* args);
        PyObject*   virtual_to_physical_with_dtb(core::Core& core, PyObject* args);
        PyObject*   read_virtual                (core::Core& core, PyObject* args);
        PyObject*   read_virtual_with_dtb       (core::Core& core, PyObject* args);
        PyObject*   read_physical               (core::Core& core, PyObject* args);
        PyObject*   write_virtual               (core::Core& core, PyObject* args);
        PyObject*   write_virtual_with_dtb      (core::Core& core, PyObject* args);
        PyObject*   write_physical              (core::Core& core, PyObject* args);
    } // namespace memory

    namespace process
    {
        PyObject*   current         (core::Core& core, PyObject* args);
        PyObject*   native          (core::Core& core, PyObject* args);
        PyObject*   kdtb            (core::Core& core, PyObject* args);
        PyObject*   udtb            (core::Core& core, PyObject* args);
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

    namespace threads
    {
        PyObject*   list            (core::Core& core, PyObject* args);
        PyObject*   current         (core::Core& core, PyObject* args);
        PyObject*   process         (core::Core& core, PyObject* args);
        PyObject*   program_counter (core::Core& core, PyObject* args);
        PyObject*   tid             (core::Core& core, PyObject* args);
        PyObject*   listen_create   (core::Core& core, PyObject* args);
        PyObject*   listen_delete   (core::Core& core, PyObject* args);
    } // namespace threads

    namespace modules
    {
        PyObject*   list            (core::Core& core, PyObject* args);
        PyObject*   name            (core::Core& core, PyObject* args);
        PyObject*   span            (core::Core& core, PyObject* args);
        PyObject*   flags           (core::Core& core, PyObject* args);
        PyObject*   find            (core::Core& core, PyObject* args);
        PyObject*   find_name       (core::Core& core, PyObject* args);
        PyObject*   listen_create   (core::Core& core, PyObject* args);
    } // namespace modules

    namespace drivers
    {
        PyObject*   list    (core::Core& core, PyObject* args);
        PyObject*   name    (core::Core& core, PyObject* args);
        PyObject*   span    (core::Core& core, PyObject* args);
        PyObject*   find    (core::Core& core, PyObject* args);
        PyObject*   listen  (core::Core& core, PyObject* args);
    }; // namespace drivers

    namespace symbols
    {
        PyObject*   load_module_memory  (core::Core& core, PyObject* args);
        PyObject*   load_module         (core::Core& core, PyObject* args);
        PyObject*   load_modules        (core::Core& core, PyObject* args);
        PyObject*   load_driver_memory  (core::Core& core, PyObject* args);
        PyObject*   load_driver         (core::Core& core, PyObject* args);
        PyObject*   load_drivers        (core::Core& core, PyObject* args);
        PyObject*   autoload_modules    (core::Core& core, PyObject* args);
        PyObject*   address             (core::Core& core, PyObject* args);
        PyObject*   list_strucs         (core::Core& core, PyObject* args);
        PyObject*   read_struc          (core::Core& core, PyObject* args);
        PyObject*   string              (core::Core& core, PyObject* args);
    } // namespace symbols

    namespace functions
    {
        PyObject*   read_stack      (core::Core& core, PyObject* args);
        PyObject*   read_arg        (core::Core& core, PyObject* args);
        PyObject*   write_arg       (core::Core& core, PyObject* args);
        PyObject*   break_on_return (core::Core& core, PyObject* args);
    } // namespace functions

    namespace callstacks
    {
        PyObject*   read            (core::Core& core, PyObject* args);
        PyObject*   load_module     (core::Core& core, PyObject* args);
        PyObject*   load_driver     (core::Core& core, PyObject* args);
        PyObject*   autoload_modules(core::Core& core, PyObject* args);
    } // namespace callstacks

    namespace vm_area
    {
        PyObject*   list(core::Core&, PyObject* args);
        PyObject*   span(core::Core&, PyObject* args);
    } // namespace vm_area

    namespace flags
    {
        PyObject*       from(flags_t flags);
        opt<flags_t>    to  (PyObject* arg);
    } // namespace flags

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
