#include "heapsan.hpp"

#define FDP_MODULE "heapsan"
#include "core.hpp"
#include "log.hpp"
#include "nt/nt.hpp"
#include "tracer/heaps.gen.hpp"
#include "utils/hash.hpp"
#include "utils/utils.hpp"

#include <map>
#include <unordered_map>
#include <unordered_set>

namespace
{
    struct realloc_t
    {
        thread_t  thread;
        nt::PVOID HeapHandle;
    };

    inline bool operator==(const realloc_t& a, const realloc_t& b)
    {
        return a.thread.id == b.thread.id
               && a.HeapHandle == b.HeapHandle;
    }

    struct heap_t
    {
        nt::PVOID HeapHandle;
        nt::PVOID BaseAddress;
    };

    inline bool operator==(const heap_t& a, const heap_t& b)
    {
        return a.HeapHandle == b.HeapHandle
               && a.BaseAddress == b.BaseAddress;
    }
}

namespace std
{
    template <>
    struct hash<realloc_t>
    {
        size_t operator()(const realloc_t& arg) const
        {
            size_t seed = 0;
            ::hash::combine(seed, arg.thread.id, arg.HeapHandle);
            return seed;
        }
    };

    template <>
    struct hash<heap_t>
    {
        size_t operator()(const heap_t& arg) const
        {
            size_t seed = 0;
            ::hash::combine(seed, arg.HeapHandle, arg.BaseAddress);
            return seed;
        }
    };
} // namespace std

namespace
{
    using Reallocs = std::unordered_set<realloc_t>;
    using Heaps    = std::unordered_set<heap_t>;
    using Data     = plugins::HeapSan::Data;

    constexpr size_t ptr_prolog = 0x20;
    constexpr size_t ptr_epilog = 0x20;
}

struct plugins::HeapSan::Data
{
    Data(core::Core& core, proc_t target);

    core::Core& core_;
    nt::heaps   tracer_;
    Reallocs    reallocs_;
    Heaps       heaps_;
    proc_t      target_;
};

Data::Data(core::Core& core, proc_t target)
    : core_(core)
    , tracer_(core, "ntdll")
    , target_(target)
{
}

plugins::HeapSan::~HeapSan() = default;

namespace
{
    void on_RtlpAllocateHeapInternal(Data& d, nt::PVOID HeapHandle, nt::SIZE_T Size)
    {
        const auto thread = threads::current(d.core_);
        if(!thread)
            return;

        const auto it = d.reallocs_.find(realloc_t{*thread, HeapHandle});
        if(it != d.reallocs_.end())
            return;

        const auto ok = functions::write_arg(d.core_, 1, {ptr_prolog + Size + ptr_epilog});
        if(!ok)
            return;

        const auto pdata = &d;
        functions::break_on_return(d.core_, "return RtlpAllocateHeapInternal", [=]
        {
            auto& d        = *pdata;
            const auto ptr = registers::read(d.core_, reg_e::rax);
            if(!ptr)
                return;

            const auto new_ptr = ptr_prolog + ptr;
            const auto ok      = registers::write(d.core_, reg_e::rax, new_ptr);
            if(!ok)
                return;

            d.heaps_.emplace(heap_t{HeapHandle, new_ptr});
        });
    }

    void on_RtlFreeHeap(Data& d, nt::PVOID HeapHandle, nt::ULONG /*Flags*/, nt::PVOID BaseAddress)
    {
        const auto it = d.heaps_.find(heap_t{HeapHandle, BaseAddress});
        if(it == d.heaps_.end())
            return;

        functions::write_arg(d.core_, 2, {BaseAddress - ptr_prolog});
        d.heaps_.erase(it);
    }

    void on_RtlSizeHeap(Data& d, nt::PVOID HeapHandle, nt::ULONG /*Flags*/, nt::PVOID BaseAddress)
    {
        const auto it = d.heaps_.find(heap_t{HeapHandle, BaseAddress});
        if(it == d.heaps_.end())
            return;

        const auto ok = functions::write_arg(d.core_, 2, {BaseAddress - ptr_prolog});
        if(!ok)
            return;

        const auto pdata = &d;
        functions::break_on_return(d.core_, "return RtlSizeHeap", [=]
        {
            auto& d         = *pdata;
            const auto size = registers::read(d.core_, reg_e::rax);
            if(!size)
                return;

            registers::write(d.core_, reg_e::rax, size - ptr_prolog - ptr_epilog);
        });
    }

    void on_RtlGetUserInfoHeap(Data& d, nt::PVOID HeapHandle, nt::ULONG /*Flags*/, nt::PVOID BaseAddress)
    {
        const auto it = d.heaps_.find(heap_t{HeapHandle, BaseAddress});
        if(it == d.heaps_.end())
            return;

        functions::write_arg(d.core_, 2, {BaseAddress - ptr_prolog});
    }

    void on_RtlSetUserValueHeap(Data& d, nt::PVOID HeapHandle, nt::ULONG /*Flags*/, nt::PVOID BaseAddress)
    {
        const auto it = d.heaps_.find(heap_t{HeapHandle, BaseAddress});
        if(it == d.heaps_.end())
            return;

        functions::write_arg(d.core_, 2, {BaseAddress - ptr_prolog});
    }

    void realloc_unknown_pointer(Data& d, nt::PVOID HeapHandle, nt::ULONG /*Flags*/, nt::PVOID /*BaseAddress*/, nt::ULONG /*Size*/)
    {
        const auto thread = threads::current(d.core_);
        if(!thread)
            return;

        // disable alloc hooks during this call
        d.reallocs_.insert(realloc_t{*thread, HeapHandle});
        const auto pdata = &d;
        functions::break_on_return(d.core_, "return RtlpReAllocateHeapInternal unknown", [=]
        {
            auto& d = *pdata;
            d.reallocs_.erase(realloc_t{*thread, HeapHandle});
        });
    }

    void on_RtlpReAllocateHeapInternal(Data& d, nt::PVOID HeapHandle, nt::ULONG Flags, nt::PVOID BaseAddress, nt::ULONG Size)
    {
        if(!BaseAddress)
            return;

        const auto it = d.heaps_.find(heap_t{HeapHandle, BaseAddress});
        if(it == d.heaps_.end())
            return realloc_unknown_pointer(d, HeapHandle, Flags, BaseAddress, Size);

        const auto thread = threads::current(d.core_);
        if(!thread)
            return;

        // tweak back pointer
        auto ok = functions::write_arg(d.core_, 2, {BaseAddress - ptr_prolog});
        if(!ok)
            return;

        // tweak size up
        ok = functions::write_arg(d.core_, 3, {ptr_prolog + Size + ptr_epilog});
        if(!ok)
            return;

        // disable alloc hooks during this call
        d.reallocs_.insert(realloc_t{*thread, HeapHandle});

        // remove pointer from heap because it can be freed with original value
        d.heaps_.erase(it);
        const auto pdata = &d;
        functions::break_on_return(d.core_, "return RtlpReAllocateHeapInternal known", [=]
        {
            auto& d = *pdata;
            d.reallocs_.erase(realloc_t{*thread, HeapHandle});
            const auto ptr = registers::read(d.core_, reg_e::rax);
            if(!ptr)
                return;

            // store new pointer which always have prolog
            const auto new_ptr = ptr + ptr_prolog;
            d.heaps_.emplace(heap_t{HeapHandle, new_ptr});
            registers::write(d.core_, reg_e::rax, new_ptr);
        });
    }

    void get_callstack(Data& d)
    {
        if(true)
            return;

        auto callers = std::vector<callstacks::caller_t>(128);
        const auto n = callstacks::read(d.core_, &callers[0], callers.size(), d.target_);
        for(size_t i = 0; i < n; ++i)
        {
            const auto addr   = callers[i].addr;
            const auto symbol = symbols::find(d.core_, d.target_, addr);
            LOG(INFO, "%-3" PRIx64 " - 0x%" PRIx64 "- %s", i, addr, symbols::to_string(symbol).data());
        }
    }
}

plugins::HeapSan::HeapSan(core::Core& core, proc_t target)
    : d_(std::make_unique<Data>(core, target))
{
    auto& d = *d_;
    d.tracer_.register_RtlpAllocateHeapInternal(d.target_, [=](nt::PVOID HeapHandle, nt::SIZE_T Size)
    {
        get_callstack(*d_);
        on_RtlpAllocateHeapInternal(*d_, HeapHandle, Size);
        return 0;
    });
    d.tracer_.register_RtlFreeHeap(d.target_, [=](nt::PVOID HeapHandle, nt::ULONG Flags, nt::PVOID BaseAddress)
    {
        get_callstack(*d_);
        on_RtlFreeHeap(*d_, HeapHandle, Flags, BaseAddress);
        return 0;
    });
    d.tracer_.register_RtlSizeHeap(d.target_, [=](nt::PVOID HeapHandle, nt::ULONG Flags, nt::PVOID BaseAddress)
    {
        get_callstack(*d_);
        on_RtlSizeHeap(*d_, HeapHandle, Flags, BaseAddress);
        return 0;
    });
    d.tracer_.register_RtlGetUserInfoHeap(d.target_, [=](nt::PVOID HeapHandle, nt::ULONG Flags, nt::PVOID BaseAddress, nt::PVOID /*UserValue*/, nt::PULONG /*UserFlags*/)
    {
        get_callstack(*d_);
        on_RtlGetUserInfoHeap(*d_, HeapHandle, Flags, BaseAddress);
        return 0;
    });
    d.tracer_.register_RtlSetUserValueHeap(d.target_, [=](nt::PVOID HeapHandle, nt::ULONG Flags, nt::PVOID BaseAddress, nt::PVOID /*UserValue*/)
    {
        get_callstack(*d_);
        on_RtlSetUserValueHeap(*d_, HeapHandle, Flags, BaseAddress);
        return 0;
    });
    d.tracer_.register_RtlpReAllocateHeapInternal(d.target_, [=](nt::PVOID HeapHandle, nt::ULONG Flags, nt::PVOID BaseAddress, nt::ULONG Size)
    {
        get_callstack(*d_);
        on_RtlpReAllocateHeapInternal(*d_, HeapHandle, Flags, BaseAddress, Size);
        return 0;
    });
}
