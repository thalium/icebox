#include "heaps.gen.hpp"

#define FDP_MODULE "heaps"
#include "log.hpp"
#include "os.hpp"

#include <map>

namespace
{
	constexpr bool g_debug = false;

	static const tracer::callcfg_t g_callcfgs[] =
	{
        {"RtlpAllocateHeapInternal", 2, {{"PVOID", "HeapHandle", sizeof(nt64::PVOID)}, {"SIZE_T", "Size", sizeof(nt64::SIZE_T)}}},
        {"RtlFreeHeap", 3, {{"PVOID", "HeapHandle", sizeof(nt64::PVOID)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"PVOID", "BaseAddress", sizeof(nt64::PVOID)}}},
        {"RtlpReAllocateHeapInternal", 4, {{"PVOID", "HeapHandle", sizeof(nt64::PVOID)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"PVOID", "BaseAddress", sizeof(nt64::PVOID)}, {"ULONG", "Size", sizeof(nt64::ULONG)}}},
        {"RtlSizeHeap", 3, {{"PVOID", "HeapHandle", sizeof(nt64::PVOID)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"PVOID", "BaseAddress", sizeof(nt64::PVOID)}}},
        {"RtlSetUserValueHeap", 4, {{"PVOID", "HeapHandle", sizeof(nt64::PVOID)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"PVOID", "BaseAddress", sizeof(nt64::PVOID)}, {"PVOID", "UserValue", sizeof(nt64::PVOID)}}},
        {"RtlGetUserInfoHeap", 5, {{"PVOID", "HeapHandle", sizeof(nt64::PVOID)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"PVOID", "BaseAddress", sizeof(nt64::PVOID)}, {"PVOID", "UserValue", sizeof(nt64::PVOID)}, {"PULONG", "UserFlags", sizeof(nt64::PULONG)}}},
	};

    using id_t      = nt64::heaps::id_t;
    using Listeners = std::multimap<id_t, core::Breakpoint>;
}

struct nt64::heaps::Data
{
    Data(core::Core& core, std::string_view module);

    core::Core& core;
    std::string module;
    Listeners   listeners;
    uint64_t    last_id;
};

nt64::heaps::Data::Data(core::Core& core, std::string_view module)
    : core(core)
    , module(module)
    , last_id(0)
{
}

nt64::heaps::heaps(core::Core& core, std::string_view module)
    : d_(std::make_unique<Data>(core, module))
{
}

nt64::heaps::~heaps() = default;

namespace
{
    using Data = nt64::heaps::Data;

    static opt<id_t> register_callback(Data& d, id_t id, proc_t proc, const char* name, const core::Task& on_call)
    {
        const auto addr = d.core.sym.symbol(d.module, name);
        if(!addr)
            return FAIL(ext::nullopt, "unable to find symbole {}!{}", d.module.data(), name);

        const auto bp = d.core.state.set_breakpoint(*addr, proc, on_call);
        if(!bp)
            return FAIL(ext::nullopt, "unable to set breakpoint");

        d.listeners.emplace(id, bp);
        return id;
    }

    template <typename T>
    static T arg(core::Core& core, size_t i)
    {
        const auto arg = core.os->read_arg(i);
        if(!arg)
            return {};

        return nt64::cast_to<T>(*arg);
    }
}

opt<id_t> nt64::heaps::register_RtlpAllocateHeapInternal(proc_t proc, const on_RtlpAllocateHeapInternal_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "RtlpAllocateHeapInternal", [=]
    {
        auto& core = d_->core;
        
        const auto HeapHandle = arg<nt64::PVOID>(core, 0);
        const auto Size       = arg<nt64::SIZE_T>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[0]);

        on_func(HeapHandle, Size);
    });
}

opt<id_t> nt64::heaps::register_RtlFreeHeap(proc_t proc, const on_RtlFreeHeap_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "RtlFreeHeap", [=]
    {
        auto& core = d_->core;
        
        const auto HeapHandle  = arg<nt64::PVOID>(core, 0);
        const auto Flags       = arg<nt64::ULONG>(core, 1);
        const auto BaseAddress = arg<nt64::PVOID>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[1]);

        on_func(HeapHandle, Flags, BaseAddress);
    });
}

opt<id_t> nt64::heaps::register_RtlpReAllocateHeapInternal(proc_t proc, const on_RtlpReAllocateHeapInternal_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "RtlpReAllocateHeapInternal", [=]
    {
        auto& core = d_->core;
        
        const auto HeapHandle  = arg<nt64::PVOID>(core, 0);
        const auto Flags       = arg<nt64::ULONG>(core, 1);
        const auto BaseAddress = arg<nt64::PVOID>(core, 2);
        const auto Size        = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[2]);

        on_func(HeapHandle, Flags, BaseAddress, Size);
    });
}

opt<id_t> nt64::heaps::register_RtlSizeHeap(proc_t proc, const on_RtlSizeHeap_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "RtlSizeHeap", [=]
    {
        auto& core = d_->core;
        
        const auto HeapHandle  = arg<nt64::PVOID>(core, 0);
        const auto Flags       = arg<nt64::ULONG>(core, 1);
        const auto BaseAddress = arg<nt64::PVOID>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[3]);

        on_func(HeapHandle, Flags, BaseAddress);
    });
}

opt<id_t> nt64::heaps::register_RtlSetUserValueHeap(proc_t proc, const on_RtlSetUserValueHeap_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "RtlSetUserValueHeap", [=]
    {
        auto& core = d_->core;
        
        const auto HeapHandle  = arg<nt64::PVOID>(core, 0);
        const auto Flags       = arg<nt64::ULONG>(core, 1);
        const auto BaseAddress = arg<nt64::PVOID>(core, 2);
        const auto UserValue   = arg<nt64::PVOID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[4]);

        on_func(HeapHandle, Flags, BaseAddress, UserValue);
    });
}

opt<id_t> nt64::heaps::register_RtlGetUserInfoHeap(proc_t proc, const on_RtlGetUserInfoHeap_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "RtlGetUserInfoHeap", [=]
    {
        auto& core = d_->core;
        
        const auto HeapHandle  = arg<nt64::PVOID>(core, 0);
        const auto Flags       = arg<nt64::ULONG>(core, 1);
        const auto BaseAddress = arg<nt64::PVOID>(core, 2);
        const auto UserValue   = arg<nt64::PVOID>(core, 3);
        const auto UserFlags   = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[5]);

        on_func(HeapHandle, Flags, BaseAddress, UserValue, UserFlags);
    });
}

opt<id_t> nt64::heaps::register_all(proc_t proc, const nt64::heaps::on_call_fn& on_call)
{
    const auto id   = ++d_->last_id;
    const auto size = d_->listeners.size();
    for(const auto cfg : g_callcfgs)
        register_callback(*d_, id, proc, cfg.name, [=]{ on_call(cfg); });

    if(size == d_->listeners.size())
        return {};

    return id;
}

bool nt64::heaps::unregister(id_t id)
{
    return d_->listeners.erase(id) > 0;
}
