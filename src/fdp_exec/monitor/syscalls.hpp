#pragma once

#include "nt/nt.hpp"

namespace syscall_mon
{
    using on_NtWriteFile = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::PIO_APC_ROUTINE, nt::PVOID,
                                                      nt::PIO_STATUS_BLOCK, nt::PVOID, nt::ULONG, nt::PLARGE_INTEGER, nt::PULONG)>;

}
