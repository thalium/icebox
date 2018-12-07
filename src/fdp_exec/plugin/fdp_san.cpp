#include "fdp_san.hpp"

#define FDP_MODULE "fdp_san"
#include "log.hpp"

#include "core/helpers.hpp"
#include "monitor/heaps.gen.hpp"
#include "nt/nt.hpp"
#include "os.hpp"
#include "utils/utils.hpp"

#include <map>
#include <unordered_map>
#include <unordered_set>

namespace
{

    struct heap_ctx_t
    {
        core::Breakpoint bp;
        thread_t         thread;
    };

    struct heap_data_t
    {
        nt::PVOID  heap_handle;
        nt::PVOID  addr;
        nt::SIZE_T size;
        uint64_t   thread_id;
    };

    using HeapCtx    = std::multimap<uint64_t, heap_ctx_t>;
    using HeapData   = std::unordered_map<nt::PVOID, std::vector<heap_data_t>>;
    using FdpSanData = plugin::FdpSan::Data;

    static const uint64_t add_size = 0x20;
    static const uint64_t half_add_size = add_size / 2;
}

struct plugin::FdpSan::Data
{
    Data(core::Core& core, pe::Pe& pe);

    core::Core&                            core_;
    pe::Pe&                                pe_;
    monitor::heaps                         heaps_;
    std::shared_ptr<callstack::ICallstack> callstack_;

    std::unordered_set<uint64_t> threads_allocating;
    std::unordered_set<uint64_t> threads_rellocating;

    HeapData heap_datas;
    HeapCtx  heap_ctxs;
    proc_t   target_;
};

plugin::FdpSan::Data::Data(core::Core& core, pe::Pe& pe)
    : core_(core)
    , pe_(pe)
    , heaps_(core, "ntdll")
    , target_()
{
}

plugin::FdpSan::FdpSan(core::Core& core, pe::Pe& pe)
    : d_(std::make_unique<Data>(core, pe))
{
}

plugin::FdpSan::~FdpSan()
{
}

namespace
{
    void get_callstack(FdpSanData& d)
    {
        uint64_t cs_size = 0;
        uint64_t cs_depth = 15;

        const auto rip = d.core_.regs.read(FDP_RIP_REGISTER);
        const auto rsp = d.core_.regs.read(FDP_RSP_REGISTER);
        const auto rbp = d.core_.regs.read(FDP_RBP_REGISTER);
        d.callstack_->get_callstack(d.target_, {rip, rsp, rbp}, [&](callstack::callstep_t cstep)
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

bool plugin::FdpSan::setup(proc_t target)
{
    d_->target_ = target;

    d_->threads_rellocating.clear();
    d_->threads_allocating.clear();

    d_->callstack_ = callstack::make_callstack_nt(d_->core_, d_->pe_);
    if(!d_->callstack_)
        FAIL(false, "Unable to create callstack object");

    d_->heaps_.register_RtlpAllocateHeapInternal(d_->target_, [&](nt::PVOID HeapHandle, nt::SIZE_T Size)
    {
        const auto thread_curr = d_->core_.os->thread_current();
        if(!thread_curr)
            return 0;

        LOG(INFO, "ALLOC {:#x}, thread {:#x}", Size, thread_curr->id);

        if(false)
        {
            get_callstack(*d_);
        }

        const auto it_thread = d_->threads_rellocating.find(thread_curr->id);
        if(it_thread != d_->threads_rellocating.end())
            return 0;

        const auto it_thread_alloc = d_->threads_allocating.find(thread_curr->id);
        if(it_thread_alloc != d_->threads_allocating.end())
            return 0;

        d_->threads_allocating.emplace(thread_curr->id);

        auto heap_data = heap_data_t{HeapHandle, 0, Size, thread_curr->id};

        const auto ok = monitor::set_arg_by_index(d_->core_, 1, Size + add_size);
        if(!ok)
            return 0;

        const auto rsp = d_->core_.regs.read(FDP_RSP_REGISTER);
        if(!rsp)
            return 0;

        const auto return_addr = core::read_ptr(d_->core_, rsp);
        if(!return_addr)
            return 0;

        const auto bp = d_->core_.state.set_breakpoint(*return_addr, *thread_curr, [=]()
        {
            d_->threads_allocating.erase(thread_curr->id);

            const auto rip = d_->core_.regs.read(FDP_RIP_REGISTER);
            if(!rip)
                return;

            auto saved_it    = d_->heap_ctxs.end();
            const auto range = d_->heap_ctxs.equal_range(rip);
            for(auto it = range.first; it != range.second; ++it)
            {
                if(thread_curr->id != it->second.thread.id)
                    continue;

                saved_it = it;
                break;
            }

            if(saved_it == d_->heap_ctxs.end())
                return;

            const auto ret = d_->core_.regs.read(FDP_RAX_REGISTER);
            if(!ret)
                return;

            const auto ok_ = monitor::set_return_value(d_->core_, ret + half_add_size);
            if(!ok_)
                return;

            auto it = this->d_->heap_datas.find(ret + half_add_size);
            if(it == d_->heap_datas.end())
                it = (d_->heap_datas.emplace(ret + half_add_size, std::vector<heap_data_t>())).first;

            auto hd = heap_data;
            hd.addr = ret + half_add_size;
            it->second.push_back(hd);

            LOG(INFO, "RtlAllocateHeap handle {:#x}, size : {:#x} at addr {:#x}, thread {:#x}", HeapHandle, Size, ret + half_add_size, thread_curr->id);
            d_->heap_ctxs.erase(saved_it);
        });

        d_->heap_ctxs.emplace(*return_addr, heap_ctx_t{bp, *thread_curr});
        return 0;
    });

    d_->heaps_.register_RtlpReAllocateHeapInternal(d_->target_, [&](nt::PVOID HeapHandle, nt::ULONG Flags, nt::PVOID BaseAddress, nt::SIZE_T Size)
    {
        UNUSED(Flags);

        LOG(INFO, "RtlReallocate {:#x}", BaseAddress);

        if(false)
        {
            get_callstack(*d_);
        }

        const auto thread_curr = d_->core_.os->thread_current();
        if(!thread_curr)
            return 0;

        d_->threads_rellocating.emplace(thread_curr->id);

        // Check if the address was previously changed by FdpSan
        auto it = d_->heap_datas.find(BaseAddress);
        bool b = true;
        int i = -1;
        if(it != d_->heap_datas.end())
        {
            for(const auto& it_v : it->second)
            {
                i++;
                if(it_v.heap_handle != HeapHandle && it_v.thread_id != thread_curr->id)
                    continue;

                b = false;
                break;
            }
        }

        if(!b)
            if(!monitor::set_arg_by_index(d_->core_, 2, BaseAddress - half_add_size))
                return 0;

        if(i != -1)
            it->second.erase(it->second.begin() + i);

        const auto ok = monitor::set_arg_by_index(d_->core_, 3, Size + add_size);
        if(!ok)
            return 0;

        const auto rsp = d_->core_.regs.read(FDP_RSP_REGISTER);
        if(!rsp)
            return 0;

        const auto return_addr = core::read_ptr(d_->core_, rsp);
        if(!return_addr)
            return 0;

        const auto heap_data = heap_data_t{HeapHandle, 0, Size, thread_curr->id};
        const auto bp        = d_->core_.state.set_breakpoint(*return_addr, *thread_curr, [=]()
        {
            const auto rip = d_->core_.regs.read(FDP_RIP_REGISTER);
            if(!rip)
                return;

            auto saved_it    = d_->heap_ctxs.end();
            const auto range = d_->heap_ctxs.equal_range(rip);
            for(auto it = range.first; it != range.second; ++it)
            {
                if(thread_curr->id != it->second.thread.id)
                    continue;

                saved_it = it;
                break;
            }

            if(saved_it == d_->heap_ctxs.end())
                return;

            const auto ret = d_->core_.regs.read(FDP_RAX_REGISTER);
            if(!ret)
                return;

            const auto ok_ = monitor::set_return_value(d_->core_, ret + half_add_size);
            if(!ok_)
                return;

            auto it_ = d_->heap_datas.find(ret + half_add_size);
            if(it_ == d_->heap_datas.end())
                it_ = (d_->heap_datas.emplace(ret + half_add_size, std::vector<heap_data_t>())).first;

            auto hd = heap_data;
            hd.addr = ret + half_add_size;
            it_->second.push_back(hd);

            LOG(INFO, "RtlReAllocateHeap handleheap {:#x}, memory pointer {:#x}, size : {:#x} at addr {:#x}", HeapHandle, BaseAddress, Size, ret + half_add_size);
            d_->threads_rellocating.erase(thread_curr->id);
            d_->heap_ctxs.erase(saved_it);
        });

        d_->heap_ctxs.emplace(*return_addr, heap_ctx_t{bp, *thread_curr});
        return 0;
    });

    d_->heaps_.register_RtlFreeHeap(d_->target_, [&](nt::PVOID HeapHandle, nt::ULONG Flags, nt::PVOID BaseAddress)
    {
        UNUSED(Flags);
        LOG(INFO, "FREE {:#x}", BaseAddress);

        if(false)
        {
            get_callstack(*d_);
        }

        const auto thread_curr = d_->core_.os->thread_current();
        if(!thread_curr)
            return false;

        // Check if the address was previously changed by FdpSan
        const auto it = d_->heap_datas.find(BaseAddress);
        if(it == d_->heap_datas.end())
            return false;

        heap_data_t heap_data;
        memset(&heap_data, 0, sizeof heap_data);
        bool found = false;
        int i = -1;
        for(const auto& it_v : it->second)
        {
            i++;
            if(it_v.heap_handle != HeapHandle && it_v.thread_id != thread_curr->id)
                continue;

            heap_data = it_v;
            found     = true;
            break;
        }

        if(!found)
            return false;

        const auto ok = monitor::set_arg_by_index(d_->core_, 2, BaseAddress - half_add_size);
        if(!ok)
            return false;

        LOG(INFO, "RtlFreeHeap handleheap {:#x}, size : {:#x} at addr {:#x}, thread {:#x}", HeapHandle, heap_data.size, BaseAddress, thread_curr->id);
        it->second.erase(it->second.begin() + i);

        return true;
    });

    d_->heaps_.register_RtlSizeHeap(d_->target_, [&](nt::PVOID HeapHandle, nt::ULONG Flags, nt::PVOID BaseAddress)
    {
        UNUSED(Flags);

        LOG(INFO, "RtlSizeHeap");

        if(false)
        {
            get_callstack(*d_);
        }

        const auto thread_curr = d_->core_.os->thread_current();
        if(!thread_curr)
            return 0;

        // Check if the address was previously changed by FdpSan
        const auto it = d_->heap_datas.find(BaseAddress);
        if(it == d_->heap_datas.end())
            return 0;

        bool found = false;
        for(const auto& it_v : it->second)
        {
            if(it_v.heap_handle != HeapHandle && it_v.thread_id != thread_curr->id)
                continue;

            found = true;
            break;
        }

        if(!found)
            return 0;

        const auto ok = monitor::set_arg_by_index(d_->core_, 2, BaseAddress - half_add_size);
        if(!ok)
            return 0;

        const auto rsp = d_->core_.regs.read(FDP_RSP_REGISTER);
        if(!rsp)
            return 0;

        const auto return_addr = core::read_ptr(d_->core_, rsp);
        if(!return_addr)
            return 0;

        const auto bp = d_->core_.state.set_breakpoint(*return_addr, *thread_curr, [=]()
        {
            const auto rip = d_->core_.regs.read(FDP_RIP_REGISTER);
            if(!rip)
                return;

            auto saved_it    = d_->heap_ctxs.end();
            const auto range = d_->heap_ctxs.equal_range(rip);
            for(auto it = range.first; it != range.second; ++it)
            {
                if(thread_curr->id != it->second.thread.id)
                    continue;

                saved_it = it;
                break;
            }

            if(saved_it == d_->heap_ctxs.end())
                return;

            const auto ret = d_->core_.regs.read(FDP_RAX_REGISTER);
            if(!ret)
                return;

            const auto ok_ = monitor::set_return_value(d_->core_, ret - add_size);
            if(!ok_)
                return;

            LOG(INFO, "RtlSizeHeap at addr {:#x} - returning {:#x}", BaseAddress, ret - add_size);
            d_->heap_ctxs.erase(saved_it);
        });

        d_->heap_ctxs.emplace(*return_addr, heap_ctx_t{bp, *thread_curr});
        return 0;
    });

    d_->heaps_.register_RtlSetUserValueHeap(d_->target_, [&](nt::PVOID HeapHandle, nt::ULONG Flags, nt::PVOID BaseAddress, nt::PVOID UserValue)
    {
        UNUSED(Flags);
        UNUSED(UserValue);

        LOG(INFO, "RtlSetUserValueHeap");

        const auto thread_curr = d_->core_.os->thread_current();
        if(!thread_curr)
            return 0;

        // Check if the address was previously changed by FdpSan
        const auto it = d_->heap_datas.find(BaseAddress);
        if(it == d_->heap_datas.end())
            return 0;

        bool found = false;
        for(const auto& it_v : it->second)
        {
            if(it_v.heap_handle != HeapHandle && it_v.thread_id != thread_curr->id)
                continue;

            found = true;
            break;
        }

        if(!found)
            return 0;

        const auto ok = monitor::set_arg_by_index(d_->core_, 2, BaseAddress - half_add_size);
        if(!ok)
            return 0;

        LOG(INFO, "RtlSetUserValueHeap at addr {:#x}", BaseAddress);
        return 0;
    });

    d_->heaps_.register_RtlGetUserInfoHeap(d_->target_, [&](nt::PVOID HeapHandle, nt::ULONG Flags, nt::PVOID BaseAddress, nt::PVOID UserValue, nt::PULONG UserFlags)
    {
        UNUSED(Flags);
        UNUSED(UserValue);
        UNUSED(UserFlags);

        LOG(INFO, "RtlGetUserValueHeap");

        const auto thread_curr = d_->core_.os->thread_current();
        if(!thread_curr)
            return 0;

        // Check if the address was previously changed by FdpSan
        const auto it = d_->heap_datas.find(BaseAddress);
        if(it == d_->heap_datas.end())
            return 0;

        bool found = false;
        for(const auto& it_v : it->second)
        {
            if(it_v.heap_handle != HeapHandle && it_v.thread_id != thread_curr->id)
                continue;

            found = true;
            break;
        }

        if(!found)
            return 0;

        const auto ok = monitor::set_arg_by_index(d_->core_, 2, BaseAddress - half_add_size);
        if(!ok)
            return 0;

        LOG(INFO, "RtlGetUserInfoHeap at addr {:#x}", BaseAddress);
        return 0;
    });

    return true;
}
