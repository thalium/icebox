#pragma once

#include "types.hpp"
#include "core.hpp"

#include "syscalls.hpp"

#include <functional>
#include <vector>

namespace syscall_mon
{
    struct SyscallMonitor
    {
         SyscallMonitor(core::Core& core);
        ~SyscallMonitor();

        using on_param_fn = std::function<walk_e(arg_t)>;

        bool             setup               (proc_t proc);
        opt<std::string> find                (uint64_t addr);
        bool             get_raw_args        (size_t nargs, const on_param_fn& on_param);
        void             register_NtWriteFile(const on_NtWriteFile& on_ntwritefile);
        void             On_NtWriteFile      ();

        struct Data;
        std::unique_ptr<Data> d_;

        core::Core&             core_;
    };
}
