#include "heaps.gen.hpp"

#define FDP_MODULE "heaps"
#include "log.hpp"
#include "core.hpp"

#include <cstring>

namespace
{
    constexpr bool g_debug = false;

    constexpr nt::heaps::callcfgs_t g_callcfgs =
    {{
        {"RtlFreeHeap", 3, {{"PVOID", "HeapHandle", sizeof(nt::PVOID)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"PVOID", "BaseAddress", sizeof(nt::PVOID)}}},
        {"RtlGetUserInfoHeap", 5, {{"PVOID", "HeapHandle", sizeof(nt::PVOID)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"PVOID", "BaseAddress", sizeof(nt::PVOID)}, {"PVOID", "UserValue", sizeof(nt::PVOID)}, {"PULONG", "UserFlags", sizeof(nt::PULONG)}}},
        {"RtlSetUserValueHeap", 4, {{"PVOID", "HeapHandle", sizeof(nt::PVOID)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"PVOID", "BaseAddress", sizeof(nt::PVOID)}, {"PVOID", "UserValue", sizeof(nt::PVOID)}}},
        {"RtlSizeHeap", 3, {{"PVOID", "HeapHandle", sizeof(nt::PVOID)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"PVOID", "BaseAddress", sizeof(nt::PVOID)}}},
        {"RtlpAllocateHeapInternal", 2, {{"PVOID", "HeapHandle", sizeof(nt::PVOID)}, {"SIZE_T", "Size", sizeof(nt::SIZE_T)}}},
        {"RtlpReAllocateHeapInternal", 4, {{"PVOID", "HeapHandle", sizeof(nt::PVOID)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"PVOID", "BaseAddress", sizeof(nt::PVOID)}, {"ULONG", "Size", sizeof(nt::ULONG)}}},
    }};
}

struct nt::heaps::Data
{
    Data(core::Core& core, std::string_view module);

    core::Core&   core;
    std::string   module;
};

nt::heaps::Data::Data(core::Core& core, std::string_view module)
    : core(core)
    , module(module)
{
}

nt::heaps::heaps(core::Core& core, std::string_view module)
    : d_(std::make_unique<Data>(core, module))
{
}

nt::heaps::~heaps() = default;

const nt::heaps::callcfgs_t& nt::heaps::callcfgs()
{
    return g_callcfgs;
}

namespace
{
    opt<bpid_t> register_callback_with(nt::heaps::Data& d, bpid_t bpid, proc_t proc, const char* name, const state::Task& on_call)
    {
        const auto addr = symbols::address(d.core, proc, d.module, name);
        if(!addr)
            return FAIL(std::nullopt, "unable to find symbole %s!%s", d.module.data(), name);

        const auto bp = state::break_on_process(d.core, name, proc, *addr, on_call);
        if(!bp)
            return FAIL(std::nullopt, "unable to set breakpoint");

        return state::save_breakpoint_with(d.core, bpid, bp);
    }

    opt<bpid_t> register_callback(nt::heaps::Data& d, proc_t proc, const char* name, const state::Task& on_call)
    {
        const auto bpid = state::acquire_breakpoint_id(d.core);
        return register_callback_with(d, bpid, proc, name, on_call);
    }

    template <typename T>
    T arg(core::Core& core, size_t i)
    {
        const auto arg = functions::read_arg(core, i);
        if(!arg)
            return {};

        T value = {};
        static_assert(sizeof value <= sizeof arg->val, "invalid size");
        memcpy(&value, &arg->val, sizeof value);
        return value;
    }
}

opt<bpid_t> nt::heaps::register_RtlFreeHeap(proc_t proc, const on_RtlFreeHeap_fn& on_func)
{
    return register_callback(*d_, proc, "RtlFreeHeap", [=]
    {
        auto& core = d_->core;

        const auto HeapHandle  = arg<nt::PVOID>(core, 0);
        const auto Flags       = arg<nt::ULONG>(core, 1);
        const auto BaseAddress = arg<nt::PVOID>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[0]);

        on_func(HeapHandle, Flags, BaseAddress);
    });
}

opt<bpid_t> nt::heaps::register_RtlGetUserInfoHeap(proc_t proc, const on_RtlGetUserInfoHeap_fn& on_func)
{
    return register_callback(*d_, proc, "RtlGetUserInfoHeap", [=]
    {
        auto& core = d_->core;

        const auto HeapHandle  = arg<nt::PVOID>(core, 0);
        const auto Flags       = arg<nt::ULONG>(core, 1);
        const auto BaseAddress = arg<nt::PVOID>(core, 2);
        const auto UserValue   = arg<nt::PVOID>(core, 3);
        const auto UserFlags   = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[1]);

        on_func(HeapHandle, Flags, BaseAddress, UserValue, UserFlags);
    });
}

opt<bpid_t> nt::heaps::register_RtlSetUserValueHeap(proc_t proc, const on_RtlSetUserValueHeap_fn& on_func)
{
    return register_callback(*d_, proc, "RtlSetUserValueHeap", [=]
    {
        auto& core = d_->core;

        const auto HeapHandle  = arg<nt::PVOID>(core, 0);
        const auto Flags       = arg<nt::ULONG>(core, 1);
        const auto BaseAddress = arg<nt::PVOID>(core, 2);
        const auto UserValue   = arg<nt::PVOID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[2]);

        on_func(HeapHandle, Flags, BaseAddress, UserValue);
    });
}

opt<bpid_t> nt::heaps::register_RtlSizeHeap(proc_t proc, const on_RtlSizeHeap_fn& on_func)
{
    return register_callback(*d_, proc, "RtlSizeHeap", [=]
    {
        auto& core = d_->core;

        const auto HeapHandle  = arg<nt::PVOID>(core, 0);
        const auto Flags       = arg<nt::ULONG>(core, 1);
        const auto BaseAddress = arg<nt::PVOID>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[3]);

        on_func(HeapHandle, Flags, BaseAddress);
    });
}

opt<bpid_t> nt::heaps::register_RtlpAllocateHeapInternal(proc_t proc, const on_RtlpAllocateHeapInternal_fn& on_func)
{
    return register_callback(*d_, proc, "RtlpAllocateHeapInternal", [=]
    {
        auto& core = d_->core;

        const auto HeapHandle = arg<nt::PVOID>(core, 0);
        const auto Size       = arg<nt::SIZE_T>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[4]);

        on_func(HeapHandle, Size);
    });
}

opt<bpid_t> nt::heaps::register_RtlpReAllocateHeapInternal(proc_t proc, const on_RtlpReAllocateHeapInternal_fn& on_func)
{
    return register_callback(*d_, proc, "RtlpReAllocateHeapInternal", [=]
    {
        auto& core = d_->core;

        const auto HeapHandle  = arg<nt::PVOID>(core, 0);
        const auto Flags       = arg<nt::ULONG>(core, 1);
        const auto BaseAddress = arg<nt::PVOID>(core, 2);
        const auto Size        = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[5]);

        on_func(HeapHandle, Flags, BaseAddress, Size);
    });
}

opt<bpid_t> nt::heaps::register_all(proc_t proc, const nt::heaps::on_call_fn& on_call)
{
    auto& d         = *d_;
    const auto bpid = state::acquire_breakpoint_id(d.core);
    for(const auto cfg : g_callcfgs)
        register_callback_with(d, bpid, proc, cfg.name, [=]{ on_call(cfg); });
    return bpid;
}
