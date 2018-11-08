#pragma once

#include "types.hpp"
#include "core.hpp"
#include "winscproto.hpp"
#include "monitor_helpers.hpp"
#include "syscalls.hpp"
#include "nt/nt.hpp"

#include <functional>
#include <vector>

namespace syscall_mon
{
    struct SyscallMonitor
    {
         SyscallMonitor(core::Core& core);
        ~SyscallMonitor();

        using on_param_fn = std::function<void(std::vector<arg_t>&)>;

        bool             setup               (proc_t proc);
        opt<std::string> find                (uint64_t addr);
        bool             generic_handler     (uint64_t rip, const on_param_fn& on_param);
        void             dispatcher          ();
        void             register_NtWriteFile(const on_NtWriteFile& on_ntwritefile);
        bool             On_NtWriteFile      (const std::vector<arg_t>& args);

        struct Data;
        std::unique_ptr<Data> d_;

        core::Core&             core_;
        winscproto::WinScProto  protos_;
    };
}
