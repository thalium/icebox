#include "fdp_san.hpp"

#define FDP_MODULE "fdp_san"
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

namespace
{
    struct return_ctx_t
    {
        thread_t thread;
        uint64_t rsp;
    };

    static inline bool operator==(const return_ctx_t& a, const return_ctx_t& b)
    {
        return a.thread.id == b.thread.id
               && a.rsp == b.rsp;
    }

    struct heap_ctx_t
    {
        nt::PVOID HeapHandle;
        nt::PVOID BaseAddress;
    };

    static inline bool operator==(const heap_ctx_t& a, const heap_ctx_t& b)
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
    struct hash<return_ctx_t>
    {
        size_t operator()(const return_ctx_t& arg) const
        {
            size_t seed = 0;
            hash_combine(seed, arg.thread.id, arg.rsp);
            return seed;
        }
    };

    template <>
    struct hash<heap_ctx_t>
    {
        size_t operator()(const heap_ctx_t& arg) const
        {
            size_t seed = 0;
            hash_combine(seed, arg.HeapHandle, arg.BaseAddress);
            return seed;
        }
    };
} // namespace std

namespace
{
    using ReturnCtx  = std::unordered_map<return_ctx_t, core::Breakpoint>;
    using HeapCtx    = std::unordered_map<heap_ctx_t, nt::SIZE_T>;
    using FdpSanData = plugins::FdpSan::Data;
    using Callstack  = std::shared_ptr<callstack::ICallstack>;

    constexpr size_t ptr_prolog = 0x10;
    constexpr size_t ptr_epilog = 0x10;
}

struct plugins::FdpSan::Data
{
    Data(core::Core& core, proc_t target);

    core::Core& core_;
    nt::heaps   heap_tracer_;
    Callstack   callstack_;

    ReturnCtx      returns_;
    HeapCtx        heap_;
    proc_t         target_;
    reader::Reader reader_;
    size_t         ptr_size_;
};

plugins::FdpSan::Data::Data(core::Core& core, proc_t target)
    : core_(core)
    , heap_tracer_(core, "ntdll")
    , target_(target)
    , reader_(reader::make(core, target))
    , ptr_size_(core.os->proc_flags(target) & flags_e::FLAGS_32BIT ? 4 : 8)
{
}

plugins::FdpSan::~FdpSan() = default;

namespace
{
    template <typename T>
    static void break_on_return(FdpSanData& d, T operand)
    {
        const auto thread      = d.core_.os->thread_current();
        const auto rsp         = d.core_.regs.read(FDP_RSP_REGISTER);
        const auto return_addr = d.reader_.read(rsp);
        if(!thread || !return_addr)
            return;

        struct Private
        {
            FdpSanData& d;
            thread_t    thread;
        } p = {d, *thread};

        const auto bp = d.core_.state.set_breakpoint(*return_addr, *thread, [=]
        {
            const auto rsp = p.d.core_.regs.read(FDP_RSP_REGISTER) - p.d.ptr_size_;
            auto it        = p.d.returns_.find(return_ctx_t{p.thread, rsp});
            if(it == p.d.returns_.end())
                return;

            operand(p.d);
            p.d.returns_.erase(it);
        });
        d.returns_.emplace(return_ctx_t{*thread, rsp}, bp);
    }

    static void on_RtlpAllocateHeapInternal(FdpSanData& d, nt::PVOID HeapHandle, nt::SIZE_T Size)
    {
        const auto ok = d.core_.os->write_arg(1, {ptr_prolog + Size + ptr_epilog});
        if(!ok)
            return;

        break_on_return(d, [=](FdpSanData& d)
        {
            const auto ptr = d.core_.regs.read(FDP_RAX_REGISTER);
            if(!ptr)
                return;

            const auto new_ptr = ptr_prolog + ptr;
            const auto ok      = d.core_.regs.write(FDP_RAX_REGISTER, new_ptr);
            if(!ok)
                return;

            d.heap_.emplace(heap_ctx_t{HeapHandle, new_ptr}, Size);
        });
    }

    static void on_RtlFreeHeap(FdpSanData& d, nt::PVOID HeapHandle, nt::ULONG /*Flags*/, nt::PVOID BaseAddress)
    {
        const auto it = d.heap_.find(heap_ctx_t{HeapHandle, BaseAddress});
        if(it == d.heap_.end())
            return;

        d.core_.os->write_arg(2, {BaseAddress - ptr_prolog});
        d.heap_.erase(it);
    }

    static void on_RtlSizeHeap(FdpSanData& d, nt::PVOID HeapHandle, nt::ULONG /*Flags*/, nt::PVOID BaseAddress)
    {
        const auto it = d.heap_.find(heap_ctx_t{HeapHandle, BaseAddress});
        if(it == d.heap_.end())
            return;

        const auto ok = d.core_.os->write_arg(2, {BaseAddress - ptr_prolog});
        if(!ok)
            return;

        break_on_return(d, [=](FdpSanData& d)
        {
            const auto size = d.core_.regs.read(FDP_RAX_REGISTER);
            if(!size)
                return;

            d.core_.regs.write(FDP_RAX_REGISTER, size - ptr_prolog - ptr_epilog);
        });
    }

    static void on_RtlGetUserInfoHeap(FdpSanData& d, nt::PVOID HeapHandle, nt::ULONG /*Flags*/, nt::PVOID BaseAddress)
    {
        if(true)
            return;
        const auto it = d.heap_.find(heap_ctx_t{HeapHandle, BaseAddress});
        if(it == d.heap_.end())
            return;

        if(false)
            d.core_.os->write_arg(2, {BaseAddress - ptr_prolog});
    }

    static void on_RtlSetUserValueHeap(FdpSanData& d, nt::PVOID HeapHandle, nt::ULONG /*Flags*/, nt::PVOID BaseAddress)
    {
        const auto it = d.heap_.find(heap_ctx_t{HeapHandle, BaseAddress});
        if(it == d.heap_.end())
            return;

        d.core_.os->write_arg(2, {BaseAddress - ptr_prolog});
    }

    static void on_RtlpReAllocateHeapInternal(FdpSanData& d, nt::PVOID HeapHandle, nt::ULONG /*Flags*/, nt::PVOID BaseAddress, nt::ULONG Size)
    {
        const auto it = d.heap_.find(heap_ctx_t{HeapHandle, BaseAddress});
        if(it != d.heap_.end())
        {
            const auto ok = d.core_.os->write_arg(2, {BaseAddress - ptr_prolog});
            if(!ok)
                return;

            d.heap_.erase(it);
        }

        const auto ok = d.core_.os->write_arg(3, {ptr_prolog + Size + ptr_epilog});
        if(!ok)
            return;

        break_on_return(d, [=](FdpSanData& d)
        {
            const auto ptr = d.core_.regs.read(FDP_RAX_REGISTER);
            if(!ptr)
                return;

            const auto new_ptr = ptr_prolog + ptr;
            d.core_.regs.write(FDP_RAX_REGISTER, new_ptr);
            d.heap_.emplace(heap_ctx_t{HeapHandle, new_ptr}, Size);
        });
    }

    static void get_callstack(FdpSanData& d)
    {
        if(true)
            return;

        uint64_t cs_size  = 0;
        uint64_t cs_depth = 150;
        d.callstack_->get_callstack(d.target_, [&](callstack::callstep_t cstep)
        {
            auto cursor = d.core_.sym.find(cstep.addr);
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

plugins::FdpSan::FdpSan(core::Core& core, proc_t target)
    : d_(std::make_unique<Data>(core, target))
{
    d_->callstack_ = callstack::make_callstack_nt(d_->core_);
    d_->heap_tracer_.register_RtlpAllocateHeapInternal(d_->target_, [=](nt::PVOID HeapHandle, nt::SIZE_T Size)
    {
        get_callstack(*d_);
        on_RtlpAllocateHeapInternal(*d_, HeapHandle, Size);
        return 0;
    });
    d_->heap_tracer_.register_RtlFreeHeap(d_->target_, [=](nt::PVOID HeapHandle, nt::ULONG Flags, nt::PVOID BaseAddress)
    {
        get_callstack(*d_);
        on_RtlFreeHeap(*d_, HeapHandle, Flags, BaseAddress);
        return 0;
    });
    d_->heap_tracer_.register_RtlSizeHeap(d_->target_, [=](nt::PVOID HeapHandle, nt::ULONG Flags, nt::PVOID BaseAddress)
    {
        get_callstack(*d_);
        on_RtlSizeHeap(*d_, HeapHandle, Flags, BaseAddress);
        return 0;
    });
    d_->heap_tracer_.register_RtlGetUserInfoHeap(d_->target_, [=](nt::PVOID HeapHandle, nt::ULONG Flags, nt::PVOID BaseAddress, nt::PVOID /*UserValue*/, nt::PULONG /*UserFlags*/)
    {
        get_callstack(*d_);
        on_RtlGetUserInfoHeap(*d_, HeapHandle, Flags, BaseAddress);
        return 0;
    });
    d_->heap_tracer_.register_RtlSetUserValueHeap(d_->target_, [=](nt::PVOID HeapHandle, nt::ULONG Flags, nt::PVOID BaseAddress, nt::PVOID /*UserValue*/)
    {
        get_callstack(*d_);
        on_RtlSetUserValueHeap(*d_, HeapHandle, Flags, BaseAddress);
        return 0;
    });
    d_->heap_tracer_.register_RtlpReAllocateHeapInternal(d_->target_, [=](nt::PVOID HeapHandle, nt::ULONG Flags, nt::PVOID BaseAddress, nt::ULONG Size)
    {
        get_callstack(*d_);
        on_RtlpReAllocateHeapInternal(*d_, HeapHandle, Flags, BaseAddress, Size);
        return 0;
    });
}
