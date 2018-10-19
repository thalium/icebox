#pragma once

#include "enums.hpp"
#include "os.hpp"
#include "sym.hpp"
#include "types.hpp"

#include <memory>

namespace core
{
    struct Registers
    {
         Registers();
        ~Registers();

        opt<uint64_t>   read    (reg_e reg);
        bool            write   (reg_e reg, uint64_t value);
        opt<uint64_t>   read    (msr_e reg);
        bool            write   (msr_e reg, uint64_t value);

        struct Data;
        std::unique_ptr<Data> d_;
    };

    // auto-managed process object
    struct ProcessContextPrivate;
    using ProcessContext = std::shared_ptr<ProcessContextPrivate>;

    // private break state
    struct BreakState;

    struct Memory
    {
         Memory();
        ~Memory();

        void            update              (const BreakState& state);
        opt<uint64_t>   virtual_to_physical (uint64_t ptr, uint64_t dtb);
        ProcessContext  switch_process      (proc_t proc);
        bool            virtual_read        (void* dst, uint64_t src, size_t size);

        struct Data;
        std::unique_ptr<Data> d_;
    };

    // auto-managed breakpoint object
    struct BreakpointPrivate;
    using Breakpoint = std::shared_ptr<BreakpointPrivate>;

    // generic functor object
    using Task = std::function<void(void)>;

    // whether to filter breakpoint on cr3
    enum filter_e { FILTER_CR3, ANY_CR3 };

    struct State
    {
         State();
        ~State();

        bool        pause           ();
        bool        resume          ();
        bool        wait            ();
        Breakpoint  set_breakpoint  (uint64_t ptr, proc_t proc, filter_e filter, const Task& task);

        // private data
        struct Data;
        std::unique_ptr<Data> d_;
    };

    struct Core
    {
         Core();
        ~Core();

        // members
        Registers                       regs;
        Memory                          mem;
        State                           state;
        sym::Symbols                    sym;
        std::unique_ptr<os::IHandler>   os;

        // private data
        struct Data;
        std::unique_ptr<Data> d_;


    };

    bool setup(Core& core, const std::string& name);
}
