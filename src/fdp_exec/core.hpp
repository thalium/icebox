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

    struct IMemory
    {
        virtual ~IMemory() = default;

        virtual void            update              (const BreakState& state) = 0;
        virtual opt<uint64_t>   virtual_to_physical (uint64_t ptr, uint64_t dtb) = 0;
        virtual ProcessContext  switch_process      (proc_t proc) = 0;
        virtual bool            read                (void* dst, uint64_t src, size_t size) = 0;
    };

    // auto-managed breakpoint object
    struct BreakpointPrivate;
    using Breakpoint = std::shared_ptr<BreakpointPrivate>;

    // generic functor object
    using Task = std::function<void(void)>;

    // whether to filter breakpoint on cr3
    enum filter_e { FILTER_CR3, ANY_CR3 };

    struct IState
    {
        virtual ~IState() = default;

        virtual bool        pause           () = 0;
        virtual bool        resume          () = 0;
        virtual bool        wait            () = 0;
        virtual Breakpoint  set_breakpoint  (uint64_t ptr, proc_t proc, filter_e filter, const Task& task) = 0;
    };

    struct IHandler
        : public IMemory
        , public IState
        , public os::IHandler
    {
        virtual ~IHandler() = default;

        Registers       regs;
        sym::Symbols    sym;
    };
    std::unique_ptr<IHandler> make_core(const std::string& name);
}
