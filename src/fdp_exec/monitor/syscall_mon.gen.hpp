#pragma once

#include "syscall_mon.hpp"


void syscall_mon::SyscallMonitor::register_NtWriteFile(const on_NtWriteFile& on_ntwritefile)
{
    d_->NtWriteFile_observers.push_back(on_ntwritefile);
}

void syscall_mon::SyscallMonitor::On_NtWriteFile()
{
    const auto nargs = 9;

    std::vector<arg_t> args;
    get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Event           = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto ApcRoutine      = nt::cast_to<nt::PIO_APC_ROUTINE>    (args[2]);
    const auto ApcContext      = nt::cast_to<nt::PVOID>              (args[3]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[4]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[5]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[6]);
    const auto ByteOffset      = nt::cast_to<nt::PLARGE_INTEGER>     (args[7]);
    const auto Key             = nt::cast_to<nt::PULONG>             (args[8]);

    for(const auto it : d_->NtWriteFile_observers)
    {
        it(FileHandle,Event,ApcRoutine,ApcContext,IoStatusBlock,Buffer,Length,ByteOffset,Key);
    }
}

void syscall_mon::SyscallMonitor::register_NtClose(const on_NtClose& on_ntclose)
{
    d_->NtClose_observers.push_back(on_ntclose);
}

void syscall_mon::SyscallMonitor::On_NtClose()
{
    const auto nargs = 1;

    std::vector<arg_t> args;
    get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Handle          = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto it : d_->NtClose_observers)
    {
        it(Handle);
    }
}
