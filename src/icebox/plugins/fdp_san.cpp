#include "fdp_san.hpp"

#define FDP_MODULE "fdp_san"
#include "log.hpp"

#include "callstack.hpp"
#include "nt/nt_types.hpp"
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
    struct ctx_t
    {
        uint64_t addr;
        thread_t thread;
    };

    struct heap_ctx_t
    {
        nt::PVOID HeapHandle;
        ctx_t     ctx;
    };

    static inline bool operator==(const ctx_t& a, const ctx_t& b)
    {
        return a.addr == b.addr
               && a.thread.id == b.thread.id;
    }

    static inline bool operator==(const heap_ctx_t& a, const heap_ctx_t& b)
    {
        return a.HeapHandle == b.HeapHandle
               && a.ctx.addr == b.ctx.addr
               && a.ctx.thread.id == b.ctx.thread.id;
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
    struct hash<ctx_t>
    {
        size_t operator()(const ctx_t& arg) const
        {
            size_t seed = 0;
            hash_combine(seed, arg.addr, arg.thread.id);
            return seed;
        }
    };

    template <>
    struct hash<heap_ctx_t>
    {
        size_t operator()(const heap_ctx_t& arg) const
        {
            size_t seed = 0;
            hash_combine(seed, arg.HeapHandle, arg.ctx);
            return seed;
        }
    };
} // namespace std

namespace
{
    using RetCtx     = std::unordered_map<ctx_t, core::Breakpoint>;
    using HeapCtx    = std::unordered_map<heap_ctx_t, nt::SIZE_T>;
    using FdpSanData = plugins::FdpSan::Data;
    using Callstack  = std::shared_ptr<callstack::ICallstack>;

    constexpr uint64_t add_size      = 0x20;
    constexpr uint64_t half_add_size = add_size / 2;
}

struct plugins::FdpSan::Data
{
    Data(core::Core& core, proc_t target);

    core::Core& core_;
    nt::heaps   heaps_;
    Callstack   callstack_;

    std::unordered_set<uint64_t> threads_allocating;
    std::unordered_set<uint64_t> threads_reallocating;

    HeapCtx heap_datas;
    RetCtx  ret_ctxs;
    proc_t  target_;
};

plugins::FdpSan::Data::Data(core::Core& core, proc_t target)
    : core_(core)
    , heaps_(core, "ntdll")
    , target_(target)
{
}

plugins::FdpSan::~FdpSan() = default;

namespace
{
    static bool is_present(std::unordered_set<uint64_t>& u, thread_t thread)
    {
        const auto it = u.find(thread.id);
        return it != u.end();
    }

    static opt<uint64_t> add_to_ret_val(FdpSanData& d, int size)
    {
        const auto ret = d.core_.regs.read(FDP_RAX_REGISTER);
        if(!ret)
            return {};

        const auto ok = d.core_.regs.write(FDP_RAX_REGISTER, ret + size);
        if(!ok)
            return {};

        return ret + size;
    }

    static opt<uint64_t> get_return_address(FdpSanData& d, const reader::Reader& reader)
    {
        const auto rsp = d.core_.regs.read(FDP_RSP_REGISTER);
        if(!rsp)
            return {};

        return reader.read(rsp);
    }

    static bool is_addr_tracked(FdpSanData& d, const heap_ctx_t& heap_ctx)
    {
        return d.heap_datas.find(heap_ctx) != d.heap_datas.end();
    }

    static void on_return_RtlpAllocateHeapInternal(FdpSanData& d, uint64_t addr, thread_t thread, nt::PVOID HeapHandle, nt::SIZE_T Size)
    {
        const auto ret_ctx = ctx_t{addr, thread};
        auto it            = d.ret_ctxs.find(ret_ctx);
        if(it == d.ret_ctxs.end())
            return;

        const auto alloc_addr = add_to_ret_val(d, half_add_size);
        if(!alloc_addr)
            return;

        const auto alloc_ctx = heap_ctx_t{HeapHandle, *alloc_addr, thread};
        d.heap_datas.emplace(alloc_ctx, Size);

        // Remove BP on return address
        d.ret_ctxs.erase(ret_ctx);
    }

    static void on_return_RtlSizeHeap(FdpSanData& d, uint64_t addr, thread_t thread)
    {
        const auto ret_ctx = ctx_t{addr, thread};
        if(d.ret_ctxs.find(ret_ctx) == d.ret_ctxs.end())
            return;

        add_to_ret_val(d, -static_cast<int>(add_size));

        // Remove BP on return address
        d.ret_ctxs.erase(ret_ctx);
    }

    static core::Breakpoint set_callback_on_return(FdpSanData& d, uint64_t addr, thread_t thread, const core::Task& on_ret)
    {
        return d.core_.state.set_breakpoint(addr, thread, on_ret);
    }

    static void get_callstack(FdpSanData& d)
    {
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
    d_->callstack_    = callstack::make_callstack_nt(d_->core_);
    const auto reader = reader::make(d_->core_, target);

    d_->heaps_.register_RtlpAllocateHeapInternal(d_->target_, [&](nt::PVOID HeapHandle, nt::SIZE_T Size)
    {
        const auto thread = d_->core_.os->thread_current();
        if(!thread)
            return 0;

        if(is_present(d_->threads_allocating, *thread))
        {
            LOG(WARNING, "Reentering RtlAllocateHeap !");
            get_callstack(*d_);
            return 0;
        }

        if(is_present(d_->threads_reallocating, *thread))
            return 0;

        d_->threads_allocating.emplace(thread->id);

        const auto ok = d_->core_.os->write_arg(1, {Size + add_size});
        if(!ok)
            return 0;

        const auto return_addr = get_return_address(*d_, reader);
        if(!return_addr)
            return 0;

        const auto bp = set_callback_on_return(*d_, *return_addr, *thread, [=]()
        {
            d_->threads_allocating.erase(thread->id);
            on_return_RtlpAllocateHeapInternal(*d_, *return_addr, *thread, HeapHandle, Size);
        });

        d_->ret_ctxs.emplace(ctx_t{*return_addr, *thread}, bp);
        return 0;
    });

    d_->heaps_.register_RtlpReAllocateHeapInternal(d_->target_, [&](nt::PVOID HeapHandle, nt::ULONG /*Flags*/, nt::PVOID BaseAddress, nt::SIZE_T Size)
    {
        const auto thread = d_->core_.os->thread_current();
        if(!thread)
            return 0;

        d_->threads_reallocating.emplace(thread->id);

        const auto ctx = heap_ctx_t{HeapHandle, BaseAddress, *thread};
        if(!is_addr_tracked(*d_, ctx))
        {
            const auto ok = d_->core_.os->write_arg(2, {BaseAddress - half_add_size});
            if(!ok)
                return 0;

            d_->heap_datas.erase(ctx);
        }

        const auto ok = d_->core_.os->write_arg(3, {Size + add_size});
        if(!ok)
            return 0;

        const auto return_addr = get_return_address(*d_, reader);
        if(!return_addr)
            return 0;

        const auto bp = set_callback_on_return(*d_, *return_addr, *thread, [=]()
        {
            d_->threads_reallocating.erase(thread->id);
            on_return_RtlpAllocateHeapInternal(*d_, *return_addr, *thread, HeapHandle, Size);
        });

        d_->ret_ctxs.emplace(ctx_t{*return_addr, *thread}, bp);
        return 0;
    });

    d_->heaps_.register_RtlFreeHeap(d_->target_, [&](nt::PVOID HeapHandle, nt::ULONG /*Flags*/, nt::PVOID BaseAddress)
    {
        const auto thread = d_->core_.os->thread_current();
        if(!thread)
            return false;

        const auto ctx = heap_ctx_t{HeapHandle, BaseAddress, *thread};
        if(!is_addr_tracked(*d_, ctx))
            return true;

        const auto ok = d_->core_.os->write_arg(2, {BaseAddress - half_add_size});
        if(!ok)
            return false;

        d_->heap_datas.erase(ctx);
        return true;
    });

    d_->heaps_.register_RtlSizeHeap(d_->target_, [&](nt::PVOID HeapHandle, nt::ULONG /*Flags*/, nt::PVOID BaseAddress)
    {
        const auto thread = d_->core_.os->thread_current();
        if(!thread)
            return 0;

        const auto ctx = heap_ctx_t{HeapHandle, BaseAddress, *thread};
        if(!is_addr_tracked(*d_, ctx))
            return 0;

        const auto ok = d_->core_.os->write_arg(2, {BaseAddress - half_add_size});
        if(!ok)
            return 0;

        const auto return_addr = get_return_address(*d_, reader);
        if(!return_addr)
            return 0;

        const auto bp = set_callback_on_return(*d_, *return_addr, *thread, [=]()
        {
            on_return_RtlSizeHeap(*d_, *return_addr, *thread);
        });

        d_->ret_ctxs.emplace(ctx_t{*return_addr, *thread}, bp);
        return 1;
    });

    d_->heaps_.register_RtlSetUserValueHeap(d_->target_, [&](nt::PVOID HeapHandle, nt::ULONG /*Flags*/, nt::PVOID BaseAddress, nt::PVOID /*UserValue*/)
    {
        const auto thread = d_->core_.os->thread_current();
        if(!thread)
            return false;

        const auto ctx = heap_ctx_t{HeapHandle, BaseAddress, *thread};
        if(!is_addr_tracked(*d_, ctx))
            return false;

        return d_->core_.os->write_arg(2, {BaseAddress - half_add_size});
    });

    d_->heaps_.register_RtlGetUserInfoHeap(d_->target_, [&](nt::PVOID HeapHandle, nt::ULONG /*Flags*/, nt::PVOID BaseAddress, nt::PVOID /*UserValue*/, nt::PULONG /*UserFlags*/)
    {
        const auto thread = d_->core_.os->thread_current();
        if(!thread)
            return false;

        const auto ctx = heap_ctx_t{HeapHandle, BaseAddress, *thread};
        if(!is_addr_tracked(*d_, ctx))
            return false;

        return d_->core_.os->write_arg(2, {BaseAddress - half_add_size});
    });
}
