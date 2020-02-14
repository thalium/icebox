#include "state.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "state"
#include "core.hpp"
#include "core_private.hpp"
#include "fdp.hpp"
#include "interfaces/if_os.hpp"
#include "log.hpp"

#include <libco.h>

#include <cstring>
#include <deque>
#include <map>
#include <thread>

namespace std
{
    template <>
    struct hash<phy_t>
    {
        size_t operator()(const phy_t& arg) const
        {
            return hash<uint64_t>()(arg.val);
        }
    };
} // namespace std

bool operator==(phy_t a, phy_t b) { return a.val == b.val; }
bool operator<(phy_t a, phy_t b) { return a.val < b.val; }
bool operator==(dtb_t a, dtb_t b) { return a.val == b.val; }
bool operator!=(dtb_t a, dtb_t b) { return a.val != b.val; }
bool operator==(proc_t a, proc_t b) { return a.id == b.id; }
bool operator!=(proc_t a, proc_t b) { return a.id != b.id; }
bool operator==(thread_t a, thread_t b) { return a.id == b.id; }
bool operator!=(thread_t a, thread_t b) { return a.id != b.id; }

namespace
{
    struct Breakpoint
    {
        opt<dtb_t> dtb;
        int        id;
    };

    struct BreakpointObserver
    {
        BreakpointObserver(state::Task task, std::string_view name, phy_t phy, opt<proc_t> proc, opt<thread_t> thread)
            : task(std::move(task))
            , name(name)
            , phy(phy)
            , proc(std::move(proc))
            , thread(std::move(thread))
            , bpid(-1)
        {
        }

        state::Task   task;
        std::string   name;
        phy_t         phy;
        opt<proc_t>   proc;
        opt<thread_t> thread;
        int           bpid;
    };

    using Breakers  = std::unordered_map<phy_t, Breakpoint>;
    using Observer  = std::shared_ptr<BreakpointObserver>;
    using Observers = std::multimap<phy_t, Observer>;
    using Buffer    = std::vector<uint8_t>;

    template <typename T>
    struct Pool
    {
        using pointer_t = std::unique_ptr<T>;
        using buffers_t = std::vector<pointer_t>;

        Pool(size_t max)
            : max(max)
        {
        }

        pointer_t acquire()
        {
            if(buffers.empty())
                return std::make_unique<T>();

            auto ret = std::move(buffers.back());
            buffers.pop_back();
            return ret;
        }

        void release(pointer_t arg)
        {
            if(buffers.size() < max)
                buffers.emplace_back(std::move(arg));
        }

        size_t    max;
        buffers_t buffers;
    };

    constexpr auto g_stack_size = 0x400000; // 4mb stack size

    struct Worker
    {
        Buffer buffer        = Buffer(g_stack_size);
        cothread_t co_thread = nullptr;
        uint64_t seq_id      = 0;     // current sequence id
        bool finished        = false; // worker thread is dead
    };

    using WorkerPool  = Pool<Worker>;
    using Workers     = std::vector<std::unique_ptr<Worker>>;
    using Breakpoints = std::multimap<uint64_t, state::Breakpoint>;
}

struct state::State
{
    State(core::Core& core)
        : core(core)
        , last_bpid{}
        , breakphy{}
        , co_main(co_active())
        , pool(16)
    {
    }

    core::Core& core;
    Breakers    targets;
    Observers   observers;
    Breakpoints breakpoints;
    bpid_t      last_bpid;
    phy_t       breakphy;
    cothread_t  co_main;
    WorkerPool  pool;
    Workers     workers;
};

std::shared_ptr<state::State> state::setup(core::Core& core)
{
    return std::make_shared<state::State>(core);
}

namespace
{
    using Data = state::State;

    bool run_workers(Data& d)
    {
        auto resumed = false;
        for(const auto& h : d.workers)
        {
            const auto seq_id = h->seq_id;
            co_switch(h->co_thread);
            resumed = resumed || seq_id != h->seq_id;
        }
        return resumed;
    }

    struct co_ctx_t
    {
        Data*    pstate;
        Observer observer;
    };

    // must have global scope...
    co_ctx_t g_co_ctx = {nullptr, nullptr};

    void start_worker(Data& d, const Observer& observer)
    {
        d.workers.emplace_back(d.pool.acquire());
        auto& w            = *d.workers.back();
        g_co_ctx           = {&d, observer};
        const auto co_next = co_derive(w.buffer.data(), g_stack_size, []
        {
            const auto co_main = g_co_ctx.pstate->co_main;
            {
                // save a copy of global context & reset it
                const auto s = g_co_ctx;
                g_co_ctx     = {};
                // save a reference from shared_ptr
                // which is guaranteed to be alive
                // until this task is finished
                auto& w     = *s.pstate->workers.back();
                w.co_thread = co_active();
                w.finished  = false;
                s.observer->task();
                w.finished = true;
            }
            // we must unwind everything before last switch
            co_switch(co_main);
        });
        co_switch(co_next);
    }

    template <typename T, typename U>
    void remove_erase_if(T& vec, U predicate)
    {
        vec.erase(std::remove_if(vec.begin(), vec.end(), predicate), vec.end());
    }

    void discard_dead_workers(Data& d)
    {
        remove_erase_if(d.workers, [&](auto& h)
        {
            if(!h->finished)
                return false;

            d.pool.release(std::move(h));
            return true;
        });
    }

    void exec_breakpoints(Data& d, const std::vector<Observer>& observers)
    {
        const auto resumed = run_workers(d);
        if(!resumed)
            for(const auto& it : observers)
                start_worker(d, it);
        discard_dead_workers(d);
    }

    Worker* current_worker(Data& d, cothread_t thread)
    {
        for(const auto& w : d.workers)
            if(w->co_thread == thread)
                return &*w;

        return nullptr;
    }

    template <typename T>
    bool yield_until(Data& d, const T& predicate)
    {
        const auto current = co_active();
        const auto pw      = current_worker(d, current);
        if(!pw)
            return false;

        // predicate must run *after* pausing first
        // ex: after page fault injection
        do
        {
            co_switch(d.co_main);
        } while(!predicate());

        pw->seq_id++;
        return true;
    }

    bool update_break_state(Data& d)
    {
        d.breakphy     = {};
        const auto rip = registers::read(d.core, reg_e::rip);
        const auto dtb = dtb_t{registers::read(d.core, reg_e::cr3)};
        const auto phy = memory::virtual_to_physical_with_dtb(d.core, dtb, rip);
        if(!phy)
            return FAIL(false, "unable to get current physical address");

        d.breakphy = *phy;
        if(false)
            os::debug_print(d.core);
        return true;
    }

    bool try_pause(Data& d)
    {
        const auto state = fdp::state(d.core);
        if(!state)
            return false;

        if(*state & FDP_STATE_PAUSED)
            return true;

        const auto ok = fdp::pause(d.core);
        if(!ok)
            return FAIL(false, "unable to pause");

        const auto updated = update_break_state(d);
        return updated;
    }

    bool try_single_step(core::Core& core)
    {
        return fdp::step_once(core);
    }

    bool try_resume(Data& d)
    {
        const auto state = fdp::state(d.core);
        if(!state)
            return false;

        if(!(*state & FDP_STATE_PAUSED))
            return true;

        if(*state & (FDP_STATE_BREAKPOINT_HIT | FDP_STATE_HARD_BREAKPOINT_HIT))
            if(!try_single_step(d.core))
                return false;

        const auto resumed = fdp::resume(d.core);
        if(!resumed)
            return FAIL(false, "unable to resume");

        return true;
    }
}

bool state::pause(core::Core& core)
{
    return try_pause(*core.state_);
}

bool state::resume(core::Core& core)
{
    return try_resume(*core.state_);
}

bool state::single_step(core::Core& core)
{
    return try_single_step(core);
}

namespace
{
    template <typename T>
    void lookup_observers(Observers& observers, phy_t phy, T operand)
    {
        // fast observer lookup while allowing current iterator deletion
        const auto end = observers.end();
        for(auto it = observers.lower_bound(phy); it != end && it->first == phy; /**/)
            if(operand(it++) == walk_e::stop)
                break;
    }
}

struct state::BreakpointPrivate
{
    BreakpointPrivate(core::Core& core, Observer observer)
        : core_(core)
        , observer_(std::move(observer))
    {
    }

    ~BreakpointPrivate()
    {
        auto& d              = *core_.state_;
        bool unique_observer = true;
        opt<Observers::iterator> target;
        lookup_observers(d.observers, observer_->phy, [&](auto it)
        {
            if(observer_ == it->second)
                target = it;
            else
                unique_observer = false;
            if(target && !unique_observer)
                return walk_e::stop;

            return walk_e::next;
        });
        if(!target)
            return;

        d.observers.erase(*target);
        if(!unique_observer)
            return;

        const auto ok = fdp::unset_breakpoint(core_, observer_->bpid);
        if(!ok)
            LOG(ERROR, "unable to remove breakpoint %d", observer_->bpid);

        d.targets.erase(observer_->phy);
    }

    core::Core& core_;
    Observer    observer_;
};

namespace
{
    void check_breakpoints(Data& d)
    {
        const auto state = fdp::state(d.core);
        if(!state)
            return;

        if(!(*state & FDP_STATE_BREAKPOINT_HIT))
            return;

        auto observers  = std::vector<Observer>{};
        auto opt_thread = opt<thread_t>{};
        auto opt_proc   = opt<proc_t>{};
        lookup_observers(d.observers, d.breakphy, [&](auto it)
        {
            const auto& bp        = *it->second;
            const auto has_filter = bp.thread || bp.proc;
            if(has_filter && !opt_thread)
                opt_thread = threads::current(d.core);

            if(bp.thread && bp.thread != opt_thread)
                return walk_e::next;

            if(bp.proc && !opt_proc)
                opt_proc = threads::process(d.core, *opt_thread);

            if(bp.proc && bp.proc != opt_proc)
                return walk_e::next;

            if(bp.task)
                observers.push_back(it->second);

            return walk_e::next;
        });
        exec_breakpoints(d, observers);
    }

    enum class breakpoints_e
    {
        update,
        skip,
    };

    enum class state_e
    {
        update,
        skip,
    };

    bool try_wait(Data& d, state_e state, breakpoints_e check)
    {
        while(true)
        {
            std::this_thread::yield();
            const auto ok = fdp::state_changed(d.core);
            if(!ok)
                continue;

            if(state == state_e::update)
                update_break_state(d);
            if(check == breakpoints_e::update)
                check_breakpoints(d);
            return true;
        }
    }
}

bool state::wait(core::Core& core)
{
    auto& d = *core.state_;
    return try_wait(d, state_e::update, breakpoints_e::update);
}

void state::exec(core::Core& core)
{
    state::resume(core);
    state::wait(core);
}

void state::wait_for(core::Core& core, int timeout_ms)
{
    const auto now      = std::chrono::steady_clock::now();
    const auto deadline = now + std::chrono::milliseconds(timeout_ms);
    while(std::chrono::steady_clock::now() < deadline)
        state::exec(core);
}

namespace
{
    opt<int> try_add_breakpoint(core::Core& core, std::string_view name, phy_t phy, opt<dtb_t> dtb)
    {
        auto& d       = *core.state_;
        auto& targets = d.targets;
        const auto it = targets.find(phy);
        if(it != targets.end())
        {
            // keep using found breakpoint if filtering rules are compatible
            const auto bp_dtb = it->second.dtb;
            if(!bp_dtb || bp_dtb == dtb)
                return it->second.id;

            // filtering rules are too restrictive, remove old breakpoint & add an unfiltered breakpoint
            const auto ok = fdp::unset_breakpoint(core, it->second.id);
            targets.erase(it);
            if(!ok)
                return std::nullopt;

            // add new breakpoint without filtering
            dtb = {};
        }

        const auto dtb_val = dtb ? dtb->val : 0;
        const auto bpid    = fdp::set_breakpoint(core, FDP_SOFTHBP, 0, FDP_EXECUTE_BP, FDP_PHYSICAL_ADDRESS, phy.val, 1, dtb_val);
        if(bpid < 0)
            return FAIL(std::nullopt, "unable to set breakpoint %s phy:0x%" PRIx64 " dtb:0x%" PRIx64, std::string{name}.data(), phy.val, dtb_val);

        targets.emplace(phy, Breakpoint{dtb, bpid});
        return bpid;
    }

    state::Breakpoint set_physical_breakpoint(core::Core& core, std::string_view name, phy_t phy, const opt<dtb_t>& dtb, opt<proc_t> proc, const opt<thread_t>& thread, const state::Task& task)
    {
        auto& d = *core.state_;
        if(thread && !proc)
            proc = threads::process(core, *thread);

        const auto bp   = std::make_shared<BreakpointObserver>(task, name, phy, proc, thread);
        const auto bpid = try_add_breakpoint(core, std::string{name}, phy, dtb);
        if(!bpid)
            return {};

        // update all observers breakpoint id
        d.observers.emplace(phy, bp);
        lookup_observers(d.observers, phy, [&](auto it)
        {
            it->second->bpid = *bpid;
            return walk_e::next;
        });
        return std::make_shared<state::BreakpointPrivate>(core, bp);
    }

    dtb_t dtb_select(core::Core& core, proc_t proc, uint64_t ptr)
    {
        return core.os_->is_kernel_address(ptr) ? proc.kdtb : proc.udtb;
    }

    state::Breakpoint set_virtual_breakpoint(core::Core& core, std::string_view name, uint64_t ptr, const opt<proc_t>& proc, const opt<thread_t>& thread, const state::Task& task)
    {
        const auto opt_proc = proc ? proc : thread ? threads::process(core, *thread) : process::current(core);
        if(!opt_proc)
            return {};

        const auto opt_phy = memory::virtual_to_physical(core, *opt_proc, ptr);
        if(!opt_phy)
            return nullptr;

        const auto dtb     = dtb_select(core, *opt_proc, ptr);
        const auto opt_dtb = proc || thread ? std::make_optional(dtb) : std::nullopt;
        return set_physical_breakpoint(core, name, *opt_phy, opt_dtb, proc, thread, task);
    }
}

state::Breakpoint state::break_on(core::Core& core, std::string_view name, uint64_t ptr, const state::Task& task)
{
    return set_virtual_breakpoint(core, name, ptr, {}, {}, task);
}

state::Breakpoint state::break_on_process(core::Core& core, std::string_view name, proc_t proc, uint64_t ptr, const state::Task& task)
{
    return set_virtual_breakpoint(core, name, ptr, proc, {}, task);
}

state::Breakpoint state::break_on_thread(core::Core& core, std::string_view name, thread_t thread, uint64_t ptr, const state::Task& task)
{
    return set_virtual_breakpoint(core, name, ptr, {}, thread, task);
}

state::Breakpoint state::break_on_physical(core::Core& core, std::string_view name, phy_t phy, const state::Task& task)
{
    return set_physical_breakpoint(core, name, phy, {}, {}, {}, task);
}

state::Breakpoint state::break_on_physical_process(core::Core& core, std::string_view name, dtb_t dtb, phy_t phy, const state::Task& task)
{
    return set_physical_breakpoint(core, name, phy, dtb, {}, {}, task);
}

namespace
{
    template <typename T>
    void run_until(Data& d, const T& predicate)
    {
        const auto ok = yield_until(d, predicate);
        if(ok)
            return;

        while(true)
        {
            try_resume(d);
            try_wait(d, state_e::update, breakpoints_e::skip);

            if(predicate())
                return;

            check_breakpoints(d);
        }
    }
}

bool state::run_to_cr_write(core::Core& core, reg_e reg)
{
    int reg_value = 0;
    switch(reg)
    {
        case reg_e::cr3:    reg_value = 3; break;
        case reg_e::cr8:    reg_value = 8; break;
        default:            return false;
    }

    const auto bpid = fdp::set_breakpoint(core, FDP_CRHBP, 0, FDP_WRITE_BP, FDP_VIRTUAL_ADDRESS, reg_value, 1, 0);
    if(bpid < 0)
        return false;

    auto& d = *core.state_;
    try_resume(d);
    try_wait(d, state_e::skip, breakpoints_e::skip);
    fdp::unset_breakpoint(core, bpid);
    return true;
}

void state::run_to_current(core::Core& core, std::string_view name)
{
    auto& d           = *core.state_;
    const auto thread = threads::current(core);
    const auto rsp    = registers::read(core, reg_e::rsp);
    const auto rip    = registers::read(core, reg_e::rip);
    const auto bp     = set_virtual_breakpoint(core, name, rip, {}, *thread, {});
    run_until(d, [&]
    {
        const auto got_rip = registers::read(core, reg_e::rip);
        const auto got_rsp = registers::read(core, reg_e::rsp);
        return got_rip == rip && got_rsp == rsp;
    });
}

void state::run_to(core::Core& core, std::string_view name, std::unordered_set<uint64_t> ptrs, bp_cr3_e bp_cr3, std::function<walk_e(proc_t, thread_t)> on_bp)
{
    if((bp_cr3 == BP_CR3_NONE) & ptrs.empty())
        return;

    auto bps = std::vector<state::Breakpoint>{};
    bps.reserve(ptrs.size());
    for(const uint64_t& ptr : ptrs)
        bps.push_back(set_virtual_breakpoint(core, name, ptr, {}, {}, {}));

    int bpid     = -1;
    uint64_t cr3 = 0;
    if(bp_cr3 == BP_CR3_ON_WRITINGS)
    {
        bpid = fdp::set_breakpoint(core, FDP_CRHBP, 0, FDP_WRITE_BP, FDP_VIRTUAL_ADDRESS, 3, 1, 0);
        if(bpid < 0)
        {
            LOG(ERROR, "unable to set a breakpoint on CR3 writes");
            return;
        }

        cr3 = registers::read(core, reg_e::cr3);
    }

    auto& d = *core.state_;
    run_until(d, [&]
    {
        const auto got_cr3 = registers::read(core, reg_e::cr3);
        const auto rip     = registers::read(core, reg_e::rip);
        if(!ptrs.count(rip) && (bp_cr3 == BP_CR3_NONE || cr3 == got_cr3))
            return false;

        if(bp_cr3 == BP_CR3_ON_WRITINGS && cr3 != got_cr3)
            cr3 = got_cr3;

        const auto thread = threads::current(core);
        if(!thread)
            return false;

        const auto proc = threads::process(core, *thread);
        if(!proc)
            return false;

        return on_bp(*proc, *thread) == walk_e::stop;
    });

    if(bp_cr3 == BP_CR3_ON_WRITINGS)
        fdp::unset_breakpoint(core, bpid);
}

bool state::save(core::Core& core)
{
    return fdp::save(core);
}

bool state::restore(core::Core& core)
{
    return fdp::restore(core);
}

bool state::inject_interrupt(core::Core& core, uint32_t code, uint32_t error, uint64_t cr2)
{
    return fdp::inject_interrupt(core, code, error, cr2);
}

namespace
{
    bpid_t acquire_bpid(Data& d)
    {
        ++d.last_bpid.id;
        return d.last_bpid;
    }

    bpid_t save_bpid(Data& d, bpid_t bpid, const state::Breakpoint& bp)
    {
        d.breakpoints.emplace(bpid.id, bp);
        return bpid;
    }
}

bpid_t state::save_breakpoint(core::Core& core, const Breakpoint& bp)
{
    if(!bp)
        return {};

    auto& d = *core.state_;
    return save_bpid(d, acquire_bpid(d), bp);
}

bpid_t state::acquire_breakpoint_id(core::Core& core)
{
    auto& d = *core.state_;
    return acquire_bpid(d);
}

bpid_t state::save_breakpoint_with(core::Core& core, bpid_t bpid, const Breakpoint& bp)
{
    if(!bp)
        return bpid;

    auto& d = *core.state_;
    return save_bpid(d, bpid, bp);
}

void state::drop_breakpoint(core::Core& core, bpid_t bpid)
{
    auto& d = *core.state_;
    d.breakpoints.erase(bpid.id);
}
