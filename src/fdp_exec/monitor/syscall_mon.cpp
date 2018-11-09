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
        char name[32];
        void (syscall_mon::SyscallMonitor::*on_syscall)();
    };

    static const OnSyscall syscalls[] =
    {
        {"NtWriteFile", &syscall_mon::SyscallMonitor::On_NtWriteFile},
    };
}

struct syscall_mon::SyscallMonitor::Data
{
    std::vector<core::Breakpoint> bps;
    std::unordered_map<uint64_t, std::string> bps_names;

    std::vector<on_NtWriteFile> NtWriteFile_observers;
};

syscall_mon::SyscallMonitor::SyscallMonitor(core::Core& core)
    : d_(std::make_unique<Data>()), core_(core)
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
            FAIL(false, "Unable to find symbol %s", s.name);

        const auto b = core_.state.set_breakpoint(*syscall_addr, proc, core::FILTER_CR3, [&]()
        {
            ((this)->*(s.on_syscall))();
        });

        d_->bps.push_back(b);
        d_->bps_names.emplace(*syscall_addr, s.name);
    }

    LOG(INFO, "Number of breakpoints %" PRIx64, d_->bps_names.size());
    return true;
}


opt<std::string> syscall_mon::SyscallMonitor::find(uint64_t addr)
{
    const auto it = d_->bps_names.find(addr);
    if (it == d_->bps_names.end())
        return ext::nullopt;

    return it->second;
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

void syscall_mon::SyscallMonitor::register_NtWriteFile(const on_NtWriteFile& on_ntwritefile)
{
    d_->NtWriteFile_observers.push_back(on_ntwritefile);
}

void syscall_mon::SyscallMonitor::On_NtWriteFile()
{
    const auto nargs = 9;   //GET THIS FROM GENERATED CODE ?

    std::vector<arg_t> args;
    get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle    = nt::cast_to<nt::HANDLE>          (args[0]);
    const auto Event         = nt::cast_to<nt::HANDLE>          (args[1]);
    const auto ApcRoutine    = nt::cast_to<nt::PIO_APC_ROUTINE> (args[2]);
    const auto ApcContext    = nt::cast_to<nt::PVOID>           (args[3]);
    const auto IoStatusBlock = nt::cast_to<nt::PIO_STATUS_BLOCK>(args[4]);
    const auto Buffer        = nt::cast_to<nt::PVOID>           (args[5]);
    const auto Length        = nt::cast_to<nt::ULONG>           (args[6]);
    const auto ByteOffsetm   = nt::cast_to<nt::PLARGE_INTEGER>  (args[7]);
    const auto Key           = nt::cast_to<nt::PULONG>          (args[8]);

    for(const auto it : d_->NtWriteFile_observers)
    {
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffsetm, Key);
    }
}
