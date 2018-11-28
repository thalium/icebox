#pragma once

#include "types.hpp"

#include <memory>

namespace core
{
    // auto-managed process object
    struct ProcessContextPrivate;
    using ProcessContext = std::shared_ptr<ProcessContextPrivate>;

    // private break state
    struct BreakState;

    struct Memory
    {
         Memory();
        ~Memory();

        void            update              (proc_t proc);
        opt<uint64_t>   virtual_to_physical (uint64_t ptr, uint64_t dtb);
        ProcessContext  switch_process      (proc_t proc);
        bool            virtual_read        (void* dst, uint64_t src, size_t size);

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace core
