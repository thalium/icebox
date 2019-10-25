#pragma once

#include "icebox/nt/nt.hpp"
#include "icebox/types.hpp"
#include "tracer.hpp"

#include <functional>

namespace core { struct Core; }

namespace nt
{
    using on_RtlpAllocateHeapInternal_fn   = std::function<void(PVOID, SIZE_T)>;
    using on_RtlFreeHeap_fn                = std::function<void(PVOID, ULONG, PVOID)>;
    using on_RtlpReAllocateHeapInternal_fn = std::function<void(PVOID, ULONG, PVOID, ULONG)>;
    using on_RtlSizeHeap_fn                = std::function<void(PVOID, ULONG, PVOID)>;
    using on_RtlSetUserValueHeap_fn        = std::function<void(PVOID, ULONG, PVOID, PVOID)>;
    using on_RtlGetUserInfoHeap_fn         = std::function<void(PVOID, ULONG, PVOID, PVOID, PULONG)>;

    struct heaps
    {
         heaps(core::Core& core, std::string_view module);
        ~heaps();

        using on_call_fn = std::function<void(const tracer::callcfg_t& callcfg)>;
        using bpid_t     = uint64_t;

        opt<bpid_t> register_all(proc_t proc, const on_call_fn& on_call);
        bool        unregister  (bpid_t id);

        opt<bpid_t> register_RtlpAllocateHeapInternal  (proc_t proc, const on_RtlpAllocateHeapInternal_fn& on_func);
        opt<bpid_t> register_RtlFreeHeap               (proc_t proc, const on_RtlFreeHeap_fn& on_func);
        opt<bpid_t> register_RtlpReAllocateHeapInternal(proc_t proc, const on_RtlpReAllocateHeapInternal_fn& on_func);
        opt<bpid_t> register_RtlSizeHeap               (proc_t proc, const on_RtlSizeHeap_fn& on_func);
        opt<bpid_t> register_RtlSetUserValueHeap       (proc_t proc, const on_RtlSetUserValueHeap_fn& on_func);
        opt<bpid_t> register_RtlGetUserInfoHeap        (proc_t proc, const on_RtlGetUserInfoHeap_fn& on_func);

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace nt
