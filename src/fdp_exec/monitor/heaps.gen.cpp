#include "heaps.gen.hpp"

#define FDP_MODULE "heaps"
#include "log.hpp"
#include "monitor.hpp"

namespace
{
	constexpr bool g_debug = true;
}

struct monitor::heaps::Data
{
    Data(core::Core& core, const std::string& module);

    using Breakpoints = std::vector<core::Breakpoint>;
    core::Core& core;
    std::string module;
    Breakpoints breakpoints;

    std::vector<on_RtlpAllocateHeapInternal_fn>   observers_RtlpAllocateHeapInternal;
    std::vector<on_RtlFreeHeap_fn>                observers_RtlFreeHeap;
    std::vector<on_RtlpReAllocateHeapInternal_fn> observers_RtlpReAllocateHeapInternal;
    std::vector<on_RtlSizeHeap_fn>                observers_RtlSizeHeap;
    std::vector<on_RtlSetUserValueHeap_fn>        observers_RtlSetUserValueHeap;
    std::vector<on_RtlGetUserInfoHeap_fn>         observers_RtlGetUserInfoHeap;
};

monitor::heaps::Data::Data(core::Core& core, const std::string& module)
    : core(core)
    , module(module)
{
}

monitor::heaps::heaps(core::Core& core, const std::string& module)
    : d_(std::make_unique<Data>(core, module))
{
}

monitor::heaps::~heaps()
{
}

namespace
{
    using Data = monitor::heaps::Data;

    static core::Breakpoint register_callback(Data& d, proc_t proc, const char* name, const monitor::heaps::on_call_fn& on_call)
    {
        const auto addr = d.core.sym.symbol(d.module, name);
        if(!addr)
            FAIL(nullptr, "unable to find symbole {}!{}", d.module.data(), name);

        return d.core.state.set_breakpoint(*addr, proc, on_call);
    }

    static bool register_callback_with(Data& d, proc_t proc, const char* name, void (*callback)(Data&))
    {
        const auto dptr = &d;
        const auto bp = register_callback(d, proc, name, [=]
        {
            callback(*dptr);
        });
        if(!bp)
            return false;

        d.breakpoints.emplace_back(bp);
        return true;
    }

    template <typename T>
    static T arg(core::Core& core, size_t i)
    {
        const auto arg = monitor::get_arg_by_index(core, i);
        if(!arg)
            return {};

        return nt::cast_to<T>(*arg);
    }

    static void on_RtlpAllocateHeapInternal(monitor::heaps::Data& d)
    {
        const auto HeapHandle = arg<nt::PVOID>(d.core, 0);
        const auto Size       = arg<nt::SIZE_T>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "RtlpAllocateHeapInternal(HeapHandle:{:#x}, Size:{:#x})", HeapHandle, Size);

        for(const auto& it : d.observers_RtlpAllocateHeapInternal)
            it(HeapHandle, Size);
    }

    static void on_RtlFreeHeap(monitor::heaps::Data& d)
    {
        const auto HeapHandle  = arg<nt::PVOID>(d.core, 0);
        const auto Flags       = arg<nt::ULONG>(d.core, 1);
        const auto BaseAddress = arg<nt::PVOID>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "RtlFreeHeap(HeapHandle:{:#x}, Flags:{:#x}, BaseAddress:{:#x})", HeapHandle, Flags, BaseAddress);

        for(const auto& it : d.observers_RtlFreeHeap)
            it(HeapHandle, Flags, BaseAddress);
    }

    static void on_RtlpReAllocateHeapInternal(monitor::heaps::Data& d)
    {
        const auto HeapHandle  = arg<nt::PVOID>(d.core, 0);
        const auto Flags       = arg<nt::ULONG>(d.core, 1);
        const auto BaseAddress = arg<nt::PVOID>(d.core, 2);
        const auto Size        = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "RtlpReAllocateHeapInternal(HeapHandle:{:#x}, Flags:{:#x}, BaseAddress:{:#x}, Size:{:#x})", HeapHandle, Flags, BaseAddress, Size);

        for(const auto& it : d.observers_RtlpReAllocateHeapInternal)
            it(HeapHandle, Flags, BaseAddress, Size);
    }

    static void on_RtlSizeHeap(monitor::heaps::Data& d)
    {
        const auto HeapHandle  = arg<nt::PVOID>(d.core, 0);
        const auto Flags       = arg<nt::ULONG>(d.core, 1);
        const auto BaseAddress = arg<nt::PVOID>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "RtlSizeHeap(HeapHandle:{:#x}, Flags:{:#x}, BaseAddress:{:#x})", HeapHandle, Flags, BaseAddress);

        for(const auto& it : d.observers_RtlSizeHeap)
            it(HeapHandle, Flags, BaseAddress);
    }

    static void on_RtlSetUserValueHeap(monitor::heaps::Data& d)
    {
        const auto HeapHandle  = arg<nt::PVOID>(d.core, 0);
        const auto Flags       = arg<nt::ULONG>(d.core, 1);
        const auto BaseAddress = arg<nt::PVOID>(d.core, 2);
        const auto UserValue   = arg<nt::PVOID>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "RtlSetUserValueHeap(HeapHandle:{:#x}, Flags:{:#x}, BaseAddress:{:#x}, UserValue:{:#x})", HeapHandle, Flags, BaseAddress, UserValue);

        for(const auto& it : d.observers_RtlSetUserValueHeap)
            it(HeapHandle, Flags, BaseAddress, UserValue);
    }

    static void on_RtlGetUserInfoHeap(monitor::heaps::Data& d)
    {
        const auto HeapHandle  = arg<nt::PVOID>(d.core, 0);
        const auto Flags       = arg<nt::ULONG>(d.core, 1);
        const auto BaseAddress = arg<nt::PVOID>(d.core, 2);
        const auto UserValue   = arg<nt::PVOID>(d.core, 3);
        const auto UserFlags   = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "RtlGetUserInfoHeap(HeapHandle:{:#x}, Flags:{:#x}, BaseAddress:{:#x}, UserValue:{:#x}, UserFlags:{:#x})", HeapHandle, Flags, BaseAddress, UserValue, UserFlags);

        for(const auto& it : d.observers_RtlGetUserInfoHeap)
            it(HeapHandle, Flags, BaseAddress, UserValue, UserFlags);
    }

}


bool monitor::heaps::register_RtlpAllocateHeapInternal(proc_t proc, const on_RtlpAllocateHeapInternal_fn& on_func)
{
    if(d_->observers_RtlpAllocateHeapInternal.empty())
        if(!register_callback_with(*d_, proc, "RtlpAllocateHeapInternal", &on_RtlpAllocateHeapInternal))
            return false;

    d_->observers_RtlpAllocateHeapInternal.push_back(on_func);
    return true;
}

bool monitor::heaps::register_RtlFreeHeap(proc_t proc, const on_RtlFreeHeap_fn& on_func)
{
    if(d_->observers_RtlFreeHeap.empty())
        if(!register_callback_with(*d_, proc, "RtlFreeHeap", &on_RtlFreeHeap))
            return false;

    d_->observers_RtlFreeHeap.push_back(on_func);
    return true;
}

bool monitor::heaps::register_RtlpReAllocateHeapInternal(proc_t proc, const on_RtlpReAllocateHeapInternal_fn& on_func)
{
    if(d_->observers_RtlpReAllocateHeapInternal.empty())
        if(!register_callback_with(*d_, proc, "RtlpReAllocateHeapInternal", &on_RtlpReAllocateHeapInternal))
            return false;

    d_->observers_RtlpReAllocateHeapInternal.push_back(on_func);
    return true;
}

bool monitor::heaps::register_RtlSizeHeap(proc_t proc, const on_RtlSizeHeap_fn& on_func)
{
    if(d_->observers_RtlSizeHeap.empty())
        if(!register_callback_with(*d_, proc, "RtlSizeHeap", &on_RtlSizeHeap))
            return false;

    d_->observers_RtlSizeHeap.push_back(on_func);
    return true;
}

bool monitor::heaps::register_RtlSetUserValueHeap(proc_t proc, const on_RtlSetUserValueHeap_fn& on_func)
{
    if(d_->observers_RtlSetUserValueHeap.empty())
        if(!register_callback_with(*d_, proc, "RtlSetUserValueHeap", &on_RtlSetUserValueHeap))
            return false;

    d_->observers_RtlSetUserValueHeap.push_back(on_func);
    return true;
}

bool monitor::heaps::register_RtlGetUserInfoHeap(proc_t proc, const on_RtlGetUserInfoHeap_fn& on_func)
{
    if(d_->observers_RtlGetUserInfoHeap.empty())
        if(!register_callback_with(*d_, proc, "RtlGetUserInfoHeap", &on_RtlGetUserInfoHeap))
            return false;

    d_->observers_RtlGetUserInfoHeap.push_back(on_func);
    return true;
}

namespace
{
    static const char g_names[][64] =
    {
      "RtlpAllocateHeapInternal",
      "RtlFreeHeap",
      "RtlpReAllocateHeapInternal",
      "RtlSizeHeap",
      "RtlSetUserValueHeap",
      "RtlGetUserInfoHeap",
    };
}

bool monitor::heaps::register_all(proc_t proc, const monitor::heaps::on_call_fn& on_call)
{
    Data::Breakpoints breakpoints;
    for(const auto it : g_names)
    {
        const auto bp = register_callback(*d_, proc, it, on_call);
        if(!bp)
            return false;

        breakpoints.emplace_back(bp);
    }

    d_->breakpoints.insert(d_->breakpoints.end(), breakpoints.begin(), breakpoints.end());
    return true;
}
