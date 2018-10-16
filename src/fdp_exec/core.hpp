#pragma once

#include "enums.hpp"
#include "os.hpp"
#include "sym.hpp"
#include "types.hpp"

#include <memory>

namespace core
{
    // auto-managed breakpoint object
    struct BreakpointPrivate;
    using Breakpoint = std::shared_ptr<BreakpointPrivate>;

    // auto-managed process object
    struct ProcessContextPrivate;
    using ProcessContext = std::shared_ptr<ProcessContextPrivate>;

    // generic functor object
    using Task = std::function<void(void)>;

    // whether to filter breakpoint on cr3
    enum filter_e { FILTER_CR3, ANY_CR3 };

    struct IHandler
    {
        virtual ~IHandler() = default;

        virtual os::IHandler&   os() = 0;
        virtual sym::IHandler&  sym() = 0;

        // state functions
        virtual bool pause() = 0;
        virtual bool resume() = 0;
        virtual bool wait() = 0;

        // breakpoint functions
        virtual Breakpoint set_breakpoint(uint64_t ptr, proc_t proc, filter_e filter, const Task& task) = 0;

        // register functions
        virtual opt<uint64_t>   read_msr    (msr_e reg) = 0;
        virtual bool            write_msr   (msr_e reg, uint64_t value) = 0;
        virtual opt<uint64_t>   read_reg    (reg_e reg) = 0;
        virtual bool            write_reg   (reg_e reg, uint64_t value) = 0;

        // memory functions
        virtual ProcessContext  switch_context  (proc_t proc) = 0;
        virtual bool            read_mem        (void* dst, uint64_t src, size_t size) = 0;
        virtual bool            write_mem       (uint64_t dst, const void* src, size_t size) = 0;
    };

    std::unique_ptr<IHandler> make_core(const std::string& name);
}
