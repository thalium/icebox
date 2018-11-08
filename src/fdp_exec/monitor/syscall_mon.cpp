#include "syscall_mon.hpp"

#define FDP_MODULE "syscall_mon"
#include "log.hpp"
#include "utils/json.hpp"
#include "syscalls.hpp"

#include <fstream>

#include <map>

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
    protos_.setup();

    const auto pdb_name = "ntdll";
    const auto imod = core_.sym.find(pdb_name);
    if (!imod)
        FAIL(false, "Unable to find pdb of %s", pdb_name);

    std::map<std::string, uint64_t> syscalls;

    imod->sym_list([&](std::string name, uint64_t offset)
    {
        if (name.find("Nt") != 0 || name.find("Ntdll") != std::string::npos)
            return WALK_NEXT;

        std::string toto = "NtWriteFile";
        if (name.compare(toto) != 0)
            return WALK_NEXT;

        syscalls.emplace(name, offset);
        //LOG(INFO, "FOUND %s - %" PRIx64, name.data(), offset);
        return WALK_NEXT;
    });

    if (syscalls.size() == 0)
        FAIL(false, "Found no symbols that contains Nt");

    d_->bps.resize(syscalls.size());
    for (const auto s : syscalls){
        d_->bps.push_back(core_.state.set_breakpoint(s.second, proc, core::FILTER_CR3, [&]()
        {
            dispatcher();
        }));
        d_->bps_names.emplace(s.second, s.first);
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

void syscall_mon::SyscallMonitor::dispatcher()
{
    const auto rip = core_.regs.read(FDP_RIP_REGISTER);
    //LOOKUP and check if OBSERVER ?

    //GET ARGS (args)
    std::vector<arg_t> args;
    generic_handler(*rip, [&](std::vector<arg_t>& args2)
    {
        args = args2;
    });
    On_NtWriteFile(args);
}

bool syscall_mon::SyscallMonitor::generic_handler(uint64_t rip, const on_param_fn& on_param)
{
    const auto syscall_name = find(rip);
    if(!syscall_name)
        FAIL(false, "No corresponding syscall");

    LOG(INFO, "Syscall %" PRIx64 " - %s", rip, syscall_name->data());

    const auto wsc = protos_.find(*syscall_name);
    const auto nargs = wsc->num_args;

    std::vector<arg_t> params;
    for (size_t j=0; j < nargs; j++)
    {
        const auto param = monitor_helpers::get_param_by_index(core_, j);
        params.push_back(arg_t{*param});
    }

    on_param(params);

    return true;
}

void syscall_mon::SyscallMonitor::register_NtWriteFile(const on_NtWriteFile& on_ntwritefile)
{
    d_->NtWriteFile_observers.push_back(on_ntwritefile);
}

bool syscall_mon::SyscallMonitor::On_NtWriteFile(const std::vector<arg_t>& args)
{
    const auto FileHandle    = nt::cast_to<nt::HANDLE>          (reinterpret_cast<const void*>(&args[0]));
    const auto Event         = nt::cast_to<nt::HANDLE>          (reinterpret_cast<const void*>(&args[1]));
    const auto ApcRoutine    = nt::cast_to<nt::PIO_APC_ROUTINE> (reinterpret_cast<const void*>(&args[2]));
    const auto ApcContext    = nt::cast_to<nt::PVOID>           (reinterpret_cast<const void*>(&args[3]));
    const auto IoStatusBlock = nt::cast_to<nt::PIO_STATUS_BLOCK>(reinterpret_cast<const void*>(&args[4]));
    const auto Buffer        = nt::cast_to<nt::PVOID>           (reinterpret_cast<const void*>(&args[5]));
    const auto Length        = nt::cast_to<nt::ULONG>           (reinterpret_cast<const void*>(&args[6]));
    const auto ByteOffsetm   = nt::cast_to<nt::PLARGE_INTEGER>  (reinterpret_cast<const void*>(&args[7]));
    const auto Key           = nt::cast_to<nt::PULONG>          (reinterpret_cast<const void*>(&args[8]));

    for(const auto it : d_->NtWriteFile_observers)
    {
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffsetm, Key);
    }

    return true;
}
