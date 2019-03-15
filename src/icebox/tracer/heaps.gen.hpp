#pragma once

#include "core.hpp"
#include "tracer.hpp"
#include "types.hpp"
#include "nt/nt.hpp"
#include "nt/nt.hpp"

#include <functional>

namespace nt
{
    using on_RtlpAllocateHeapInternal_fn   = std::function<PVOID(PVOID, SIZE_T)>;
    using on_RtlFreeHeap_fn                = std::function<BOOLEAN(PVOID, ULONG, PVOID)>;
    using on_RtlpReAllocateHeapInternal_fn = std::function<PVOID(PVOID, ULONG, PVOID, ULONG)>;
    using on_RtlSizeHeap_fn                = std::function<SIZE_T(PVOID, ULONG, PVOID)>;
    using on_RtlSetUserValueHeap_fn        = std::function<BOOLEAN(PVOID, ULONG, PVOID, PVOID)>;
    using on_RtlGetUserInfoHeap_fn         = std::function<BOOLEAN(PVOID, ULONG, PVOID, PVOID, PULONG)>;

    struct heaps
    {
         heaps(core::Core& core, std::string_view module);
        ~heaps();

        using on_call_fn = std::function<void(const tracer::callcfg_t& callcfg)>;
        using id_t       = uint64_t;

        opt<id_t>   register_all(proc_t proc, const on_call_fn& on_call);
        bool        unregister  (id_t id);

        opt<id_t> register_RtlpAllocateHeapInternal  (proc_t proc, const on_RtlpAllocateHeapInternal_fn& on_func);
        opt<id_t> register_RtlFreeHeap               (proc_t proc, const on_RtlFreeHeap_fn& on_func);
        opt<id_t> register_RtlpReAllocateHeapInternal(proc_t proc, const on_RtlpReAllocateHeapInternal_fn& on_func);
        opt<id_t> register_RtlSizeHeap               (proc_t proc, const on_RtlSizeHeap_fn& on_func);
        opt<id_t> register_RtlSetUserValueHeap       (proc_t proc, const on_RtlSetUserValueHeap_fn& on_func);
        opt<id_t> register_RtlGetUserInfoHeap        (proc_t proc, const on_RtlGetUserInfoHeap_fn& on_func);

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace nt
