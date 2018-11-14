#pragma once

#include "core.hpp"
#include "utils/pe.hpp"

#include "callstack.hpp"
#include "monitor/syscall_mon.hpp"

#include <memory>

namespace syscall_tracer
{
    struct SyscallPlugin
    {
         SyscallPlugin(core::Core& core, pe::Pe& pe);
        ~SyscallPlugin();

        bool setup(proc_t target);
        bool private_get_callstack();
        bool produce_output(std::string file_name);

        struct Data;
        std::unique_ptr<Data> d_;

        core::Core&                            core_;
        pe::Pe&                                pe_;
        syscall_mon::SyscallMonitor            syscall_monitor_;
        std::unique_ptr<callstack::ICallstack> callstack_;
    };
}
