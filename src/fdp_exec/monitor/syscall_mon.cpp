#include "syscall_mon.hpp"

#define FDP_MODULE "syscall_mon"
#include "log.hpp"
#include "monitor_helpers.hpp"
#include "nt/nt.hpp"

#include <unordered_map>

namespace
{
    static const std::string syscall_dll = "ntdll";

    struct OnSyscall
    {
        char name[64];
        void (syscall_mon::SyscallMonitor::*on_syscall)();
    };

    static const OnSyscall syscalls[] =
    {
        DECLARE_HANDLERS
    };
}

struct syscall_mon::SyscallMonitor::Data
{
    std::vector<core::Breakpoint> bps;

    DECLARE_OBSERVERS
};

syscall_mon::SyscallMonitor::SyscallMonitor(core::Core& core)
    : d_(std::make_unique<Data>())
    , core_(core)
{
}

syscall_mon::SyscallMonitor::~SyscallMonitor()
{
}

bool syscall_mon::SyscallMonitor::setup(proc_t proc)
{
    //TODO Load pdb to be sure that it is loaded ?

    for(const auto& s : syscalls)
    {
        const auto syscall_addr = core_.sym.symbol(syscall_dll, s.name);
        if (!syscall_addr)
        {
            // FAIL(false, "Unable to find symbol %s", s.name);
            LOG(ERROR, "Unable to find symbol %s", s.name);
            continue;
        }

        const auto b = core_.state.set_breakpoint(*syscall_addr, proc, core::FILTER_CR3, [&]()
        {
            ((this)->*(s.on_syscall))();
        });

        d_->bps.push_back(b);
    }

    return true;
}

bool syscall_mon::SyscallMonitor::get_raw_args(size_t nargs, const on_param_fn& on_param)
{
    for (size_t j=0; j < nargs; j++)
    {
        const auto param = monitor_helpers::get_param_by_index(core_, j);
        if (on_param(*param) == WALK_STOP)
            break;
    }

    return true;
}

#include "syscall_mon_private.gen.hpp"
