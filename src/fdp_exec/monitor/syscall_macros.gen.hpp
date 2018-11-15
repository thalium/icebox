#pragma once

#include "nt/nt.hpp"


#define DECLARE_CALLBACK_PROTOS\
    using on_NtWriteFile      = std::function<nt::NTSTATUS(nt::HANDLE,nt::HANDLE,nt::PIO_APC_ROUTINE,nt::PVOID,nt::PIO_STATUS_BLOCK,nt::PVOID,nt::ULONG,nt::PLARGE_INTEGER,nt::PULONG)>;\
    using on_NtClose          = std::function<nt::NTSTATUS(nt::HANDLE)>;

#define DECLARE_OBSERVERS\
    std::vector<on_NtWriteFile> NtWriteFile_observers;\
    std::vector<on_NtClose> NtClose_observers;

#define DECLARE_HANDLERS\
    {"NtWriteFile", &syscall_mon::SyscallMonitor::On_NtWriteFile},\
    {"NtClose", &syscall_mon::SyscallMonitor::On_NtClose},
