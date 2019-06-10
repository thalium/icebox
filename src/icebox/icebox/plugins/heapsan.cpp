#include "heapsan.hpp"

#define FDP_MODULE "heapsan"
#include "log.hpp"

#include "breaker.hpp"
#include "callstack.hpp"
#include "nt/nt.hpp"
#include "os.hpp"
#include "reader.hpp"
#include "tracer/heaps.gen.hpp"
#include "utils/fnview.hpp"
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

    static inline bool operator==(const realloc_t& a, const realloc_t& b)
    {
        return a.thread.id == b.thread.id
               && a.HeapHandle == b.HeapHandle;
    }

    struct heap_t
    {
        nt::PVOID HeapHandle;
        nt::PVOID BaseAddress;
    };

    static inline bool operator==(const heap_t& a, const heap_t& b)
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
    using Reallocs  = std::unordered_set<realloc_t>;
    using Heaps     = std::unordered_set<heap_t>;
    using Data      = plugins::HeapSan::Data;
    using Callstack = std::shared_ptr<callstack::ICallstack>;

    constexpr size_t ptr_prolog = 0x20;
    constexpr size_t ptr_epilog = 0x20;
}

struct plugins::HeapSan::Data
{
    Data(core::Core& core, sym::Symbols& syms, proc_t target);

    core::Core&    core_;
    sym::Symbols&  symbols_;
    nt::heaps      tracer_;
    Callstack      callstack_;
    Reallocs       reallocs_;
    Heaps          heaps_;
    proc_t         target_;
    state::Breaker breaker_;
};

Data::Data(core::Core& core, sym::Symbols& syms, proc_t target)
    : core_(core)
    , symbols_(syms)
    , tracer_(core, syms, "ntdll")
    , target_(target)
    , breaker_(core, target)
{
}

plugins::HeapSan::~HeapSan() = default;

namespace
{
    static void on_RtlpAllocateHeapInternal(Data& d, nt::PVOID HeapHandle, nt::SIZE_T Size)
    {
        const auto thread = d.core_.os->thread_current();
        if(!thread)
            return;

        const auto it = d.reallocs_.find(realloc_t{*thread, HeapHandle});
        if(it != d.reallocs_.end())
            return;

        const auto ok = d.core_.os->write_arg(1, {ptr_prolog + Size + ptr_epilog});
        if(!ok)
            return;

        const auto pdata = &d;
        d.breaker_.break_return("return RtlpAllocateHeapInternal", [=]
        {
            auto& d        = *pdata;
            const auto ptr = d.core_.regs.read(FDP_RAX_REGISTER);
            if(!ptr)
                return;

            const auto new_ptr = ptr_prolog + ptr;
            const auto ok      = d.core_.regs.write(FDP_RAX_REGISTER, new_ptr);
            if(!ok)
                return;

            d.heaps_.emplace(heap_t{HeapHandle, new_ptr});
        });
    }

    static void on_RtlFreeHeap(Data& d, nt::PVOID HeapHandle, nt::ULONG /*Flags*/, nt::PVOID BaseAddress)
    {
        const auto it = d.heaps_.find(heap_t{HeapHandle, BaseAddress});
        if(it == d.heaps_.end())
            return;

        d.core_.os->write_arg(2, {BaseAddress - ptr_prolog});
        d.heaps_.erase(it);
    }

    static void on_RtlSizeHeap(Data& d, nt::PVOID HeapHandle, nt::ULONG /*Flags*/, nt::PVOID BaseAddress)
    {
        const auto it = d.heaps_.find(heap_t{HeapHandle, BaseAddress});
        if(it == d.heaps_.end())
            return;

        const auto ok = d.core_.os->write_arg(2, {BaseAddress - ptr_prolog});
        if(!ok)
            return;

        const auto pdata = &d;
        d.breaker_.break_return("return RtlSizeHeap", [=]
        {
            auto& d         = *pdata;
            const auto size = d.core_.regs.read(FDP_RAX_REGISTER);
            if(!size)
                return;

            d.core_.regs.write(FDP_RAX_REGISTER, size - ptr_prolog - ptr_epilog);
        });
    }

    static void on_RtlGetUserInfoHeap(Data& d, nt::PVOID HeapHandle, nt::ULONG /*Flags*/, nt::PVOID BaseAddress)
    {
        const auto it = d.heaps_.find(heap_t{HeapHandle, BaseAddress});
        if(it == d.heaps_.end())
            return;

        d.core_.os->write_arg(2, {BaseAddress - ptr_prolog});
    }

    static void on_RtlSetUserValueHeap(Data& d, nt::PVOID HeapHandle, nt::ULONG /*Flags*/, nt::PVOID BaseAddress)
    {
        const auto it = d.heaps_.find(heap_t{HeapHandle, BaseAddress});
        if(it == d.heaps_.end())
            return;

        d.core_.os->write_arg(2, {BaseAddress - ptr_prolog});
    }

    static void realloc_unknown_pointer(Data& d, nt::PVOID HeapHandle, nt::ULONG /*Flags*/, nt::PVOID /*BaseAddress*/, nt::ULONG /*Size*/)
    {
        const auto thread = d.core_.os->thread_current();
        if(!thread)
            return;

        // disable alloc hooks during this call
        d.reallocs_.insert(realloc_t{*thread, HeapHandle});
        const auto pdata = &d;
        d.breaker_.break_return("return RtlpReAllocateHeapInternal unknown", [=]
        {
            auto& d = *pdata;
            d.reallocs_.erase(realloc_t{*thread, HeapHandle});
        });
    }

    static void on_RtlpReAllocateHeapInternal(Data& d, nt::PVOID HeapHandle, nt::ULONG Flags, nt::PVOID BaseAddress, nt::ULONG Size)
    {
        if(!BaseAddress)
            return;

        const auto it = d.heaps_.find(heap_t{HeapHandle, BaseAddress});
        if(it == d.heaps_.end())
            return realloc_unknown_pointer(d, HeapHandle, Flags, BaseAddress, Size);

        const auto thread = d.core_.os->thread_current();
        if(!thread)
            return;

        // tweak back pointer
        auto ok = d.core_.os->write_arg(2, {BaseAddress - ptr_prolog});
        if(!ok)
            return;

        // tweak size up
        ok = d.core_.os->write_arg(3, {ptr_prolog + Size + ptr_epilog});
        if(!ok)
            return;

        // disable alloc hooks during this call
        d.reallocs_.insert(realloc_t{*thread, HeapHandle});

        // remove pointer from heap because it can be freed with original value
        d.heaps_.erase(it);
        const auto pdata = &d;
        d.breaker_.break_return("return RtlpReAllocateHeapInternal known", [=]
        {
            auto& d = *pdata;
            d.reallocs_.erase(realloc_t{*thread, HeapHandle});
            const auto ptr = d.core_.regs.read(FDP_RAX_REGISTER);
            if(!ptr)
                return;

            // store new pointer which always have prolog
            const auto new_ptr = ptr + ptr_prolog;
            d.heaps_.emplace(heap_t{HeapHandle, new_ptr});
            d.core_.regs.write(FDP_RAX_REGISTER, new_ptr);
        });
    }

    static void get_callstack(Data& d)
    {
        if(true)
            return;

        uint64_t cs_size  = 0;
        uint64_t cs_depth = 150;
        d.callstack_->get_callstack(d.target_, [&](callstack::callstep_t cstep)
        {
            auto cursor = d.symbols_.find(cstep.addr);
            if(!cursor)
                cursor = sym::Cursor{"_", "_", cstep.addr};

            LOG(INFO, "{:>3} - {:#x}- {}", cs_size, cstep.addr, sym::to_string(*cursor).data());

            cs_size++;
            if(cs_size < cs_depth)
                return WALK_NEXT;

            return WALK_STOP;
        });
    }
}

plugins::HeapSan::HeapSan(core::Core& core, sym::Symbols& syms, proc_t target)
    : d_(std::make_unique<Data>(core, syms, target))
{
    auto& d      = *d_;
    d.callstack_ = callstack::make_callstack_nt(d.core_);
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
