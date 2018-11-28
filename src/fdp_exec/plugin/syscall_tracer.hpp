#pragma once

#include "core.hpp"
#include "utils/pe.hpp"

#include "callstack.hpp"
#include "monitor/generic_mon.hpp"
#include "nt/objects_nt.hpp"

#include <memory>

namespace syscall_tracer
{
    struct SyscallPlugin
    {
         SyscallPlugin(core::Core& core, pe::Pe& pe);
        ~SyscallPlugin();

        bool    setup                   (proc_t target);
        bool    private_get_callstack   ();
        bool    produce_output          (std::string file_name);

        struct Data;
        std::unique_ptr<Data> d_;

        core::Core&                            core_;
        pe::Pe&                                pe_;
        monitor::GenericMonitor                generic_monitor_;
        std::shared_ptr<callstack::ICallstack> callstack_;
        std::shared_ptr<nt::ObjectNt>          objects_nt_;
    };
} // namespace syscall_tracer
