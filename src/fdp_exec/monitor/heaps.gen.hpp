#pragma once

#include "core.hpp"
#include "types.hpp"

#include "nt/nt.hpp"

#include <functional>

namespace monitor
{
    using on_RtlpAllocateHeapInternal_fn   = std::function<nt::PVOID(nt::PVOID, nt::SIZE_T)>;
    using on_RtlFreeHeap_fn                = std::function<nt::BOOLEAN(nt::PVOID, nt::ULONG, nt::PVOID)>;
    using on_RtlpReAllocateHeapInternal_fn = std::function<nt::PVOID(nt::PVOID, nt::ULONG, nt::PVOID, nt::ULONG)>;
    using on_RtlSizeHeap_fn                = std::function<nt::SIZE_T(nt::PVOID, nt::ULONG, nt::PVOID)>;
    using on_RtlSetUserValueHeap_fn        = std::function<nt::BOOLEAN(nt::PVOID, nt::ULONG, nt::PVOID, nt::PVOID)>;
    using on_RtlGetUserInfoHeap_fn         = std::function<nt::BOOLEAN(nt::PVOID, nt::ULONG, nt::PVOID, nt::PVOID, nt::PULONG)>;

    struct heaps
    {
         heaps(core::Core& core, const std::string& module);
        ~heaps();

        // register generic callback with process filtering
        using on_call_fn = std::function<void()>;
        bool register_all(proc_t proc, const on_call_fn& on_call);

        bool register_RtlpAllocateHeapInternal  (proc_t proc, const on_RtlpAllocateHeapInternal_fn& on_func);
        bool register_RtlFreeHeap               (proc_t proc, const on_RtlFreeHeap_fn& on_func);
        bool register_RtlpReAllocateHeapInternal(proc_t proc, const on_RtlpReAllocateHeapInternal_fn& on_func);
        bool register_RtlSizeHeap               (proc_t proc, const on_RtlSizeHeap_fn& on_func);
        bool register_RtlSetUserValueHeap       (proc_t proc, const on_RtlSetUserValueHeap_fn& on_func);
        bool register_RtlGetUserInfoHeap        (proc_t proc, const on_RtlGetUserInfoHeap_fn& on_func);

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace monitor
