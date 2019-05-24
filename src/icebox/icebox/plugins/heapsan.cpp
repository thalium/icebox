#include "heapsan.hpp"

#define FDP_MODULE "heapsan"
#include "log.hpp"

#include "callstack.hpp"
#include "nt/nt.hpp"
#include "os.hpp"
#include "reader.hpp"
#include "tracer/heaps.gen.hpp"
#include "utils/fnview.hpp"
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

    struct return_t
    {
        thread_t thread;
        uint64_t rsp;
    };

    static inline bool operator==(const return_t& a, const return_t& b)
    {
        return a.thread.id == b.thread.id
               && a.rsp == b.rsp;
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

    static inline void hash_combine(std::size_t& /*seed*/) {}

    template <typename T, typename... Rest>
    static inline void hash_combine(std::size_t& seed, const T& v, Rest... rest)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9E3779B97F4A7C15 + (seed << 6) + (seed >> 2);
        hash_combine(seed, rest...);
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
            hash_combine(seed, arg.thread.id, arg.HeapHandle);
            return seed;
        }
    };

    template <>
    struct hash<return_t>
    {
        size_t operator()(const return_t& arg) const
        {
            size_t seed = 0;
            hash_combine(seed, arg.thread.id, arg.rsp);
            return seed;
        }
    };

    template <>
    struct hash<heap_t>
    {
        size_t operator()(const heap_t& arg) const
        {
            size_t seed = 0;
            hash_combine(seed, arg.HeapHandle, arg.BaseAddress);
            return seed;
        }
    };
} // namespace std

namespace
{
    using Reallocs  = std::unordered_set<realloc_t>;
    using Returns   = std::unordered_map<return_t, core::Breakpoint>;
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
    Returns        returns_;
    Heaps          heaps_;
    proc_t         target_;
    reader::Reader reader_;
    size_t         ptr_size_;
};

Data::Data(core::Core& core, sym::Symbols& syms, proc_t target)
    : core_(core)
    , symbols_(syms)
    , tracer_(core, syms, "ntdll")
    , target_(target)
    , reader_(reader::make(core, target))
    , ptr_size_(core.os->proc_flags(target) & flags_e::FLAGS_32BIT ? 4 : 8)
{
}

plugins::HeapSan::~HeapSan() = default;

namespace
{
    template <typename T>
    static void break_on_return(Data& d, std::string_view name, T operand)
    {
        const auto thread      = d.core_.os->thread_current();
        const auto rsp         = d.core_.regs.read(FDP_RSP_REGISTER);
        const auto return_addr = d.reader_.read(rsp);
        if(!thread || !return_addr)
            return;

        struct Private
        {
            Data&    d;
            thread_t thread;
            T        operand;
        } p = {d, *thread, operand};

        const auto bp = d.core_.state.set_breakpoint(name, *return_addr, *thread, [=]
        {
            const auto rsp = p.d.core_.regs.read(FDP_RSP_REGISTER) - p.d.ptr_size_;
            auto it        = p.d.returns_.find(return_t{p.thread, rsp});
            if(it == p.d.returns_.end())
                return;

            p.operand(p.d);
            p.d.returns_.erase(it);
        });
        d.returns_.emplace(return_t{*thread, rsp}, bp);
    }

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

        break_on_return(d, "return RtlpAllocateHeapInternal", [=](Data& d)
        {
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

        break_on_return(d, "return RtlSizeHeap", [=](Data& d)
        {
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
        break_on_return(d, "return RtlpReAllocateHeapInternal unknown", [=](Data& d)
        {
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
        break_on_return(d, "return RtlpReAllocateHeapInternal known", [=](Data& d)
        {
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
