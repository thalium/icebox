#include "state.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "state"
#include "core.hpp"
#include "core_private.hpp"
#include "fdp.hpp"
#include "interfaces/if_os.hpp"
#include "log.hpp"

#include <libco.h>

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

    struct BreakState
    {
        proc_t   proc;
        thread_t thread;
        uint64_t rip;
        dtb_t    dtb;
        phy_t    phy;
    };

    using check_fn = std::function<bool()>;
    using Waiters  = std::vector<check_fn>;

    using BufferType = std::vector<uint8_t>;

    template <typename T>
    struct Pool
    {
        using allocate_fn = std::function<T()>;
        using buffers_t   = std::vector<T>;

        Pool(size_t max, const allocate_fn& allocate)
            : max(max)
            , allocate(allocate)
        {
        }

        T acquire()
        {
            if(buffers.empty())
                return allocate();

            auto ret = std::move(buffers.back());
            buffers.pop_back();
            return ret;
        }

        void release(T arg)
        {
            if(buffers.size() < max)
                buffers.emplace_back(std::move(arg));
        }

        size_t      max;
        allocate_fn allocate;
        buffers_t   buffers;
    };

    using Buffer = std::unique_ptr<BufferType>;

    struct Worker
    {
         Worker(state::State& state, const state::Task& task);
        ~Worker();

        state::State& state;
        state::Task   task;
        Buffer        buffer;
        cothread_t co_thread = nullptr;
        uint64_t seq_id      = 0;     // current sequence id
        bool finished        = false; // worker thread is dead
    };

    using Workers    = std::vector<std::shared_ptr<Worker>>;
    using BufferPool = Pool<Buffer>;

    constexpr auto g_stack_size = 0x400000; // 4mb stack size
}

struct state::State
{
    State(core::Core& core)
        : core(core)
        , breakstate{}
        , co_main(co_active())
        , stacks(16, []
        {
            return std::make_unique<BufferType>(g_stack_size);
        })
    {
    }

    core::Core& core;
    Breakers    targets;
    Observers   observers;
    BreakState  breakstate;
    Waiters     waiters;
    cothread_t  co_main;
    Workers     workers;
    BufferPool  stacks;
};

std::shared_ptr<state::State> state::setup(core::Core& core)
{
    return std::make_shared<state::State>(core);
}

namespace
{
    using Data = state::State;

    Worker::Worker(Data& d, const state::Task& task)
        : state(d)
        , task(task)
        , buffer(d.stacks.acquire())
    {
    }

    Worker::~Worker()
    {
        state.stacks.release(std::move(buffer));
    }

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

    // must have global scope...
    Data* g_data = nullptr;

    void start_worker(Data& d, const state::Task& task)
    {
        d.workers.emplace_back(std::make_shared<Worker>(d, task));
        const auto& w          = d.workers.back();
        g_data                 = &d;
        const auto next_thread = co_derive(w->buffer->data(), g_stack_size, []
        {
            {
                // save a reference from shared_ptr
                // which is guaranteed to be alive
                // until this task is finished
                auto& w     = *g_data->workers.back();
                w.co_thread = co_active();
                w.task();
                w.finished = true;
            }
            // we must unwind everything before last switch
            co_switch(g_data->co_main);
        });
        co_switch(next_thread);
    }

    template <typename T, typename U>
    void remove_erase_if(T& vec, U predicate)
    {
        vec.erase(std::remove_if(vec.begin(), vec.end(), predicate), vec.end());
    }

    void discard_dead_workers(Data& d)
    {
        remove_erase_if(d.workers, [](const auto& h)
        {
            return h->finished;
        });
    }

    void exec_breakpoints(Data& d, const std::vector<Observer>& observers)
    {
        const auto resumed = run_workers(d);
        if(!resumed)
            for(const auto& it : observers)
                start_worker(d, it->task);
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

    static bool update_break_state(Data& d)
    {
        memset(&d.breakstate, 0, sizeof d.breakstate);
        const auto thread = threads::current(d.core);
        if(!thread)
            return FAIL(false, "unable to get current thread");

        const auto proc = threads::process(d.core, *thread);
        if(!proc)
            return FAIL(false, "unable to get current proc");

        const auto rip = registers::read(d.core, reg_e::rip);
        const auto dtb = dtb_t{registers::read(d.core, reg_e::cr3)};
        const auto phy = memory::virtual_to_physical(d.core, rip, dtb);
        if(!phy)
            return FAIL(false, "unable to get current physical address");

        d.breakstate.thread = *thread;
        d.breakstate.proc   = *proc;
        d.breakstate.rip    = rip;
        d.breakstate.dtb    = dtb;
        d.breakstate.phy    = *phy;
        os::debug_print(d.core);
        return true;
    }

    static bool try_pause(Data& d)
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

    static bool try_single_step(core::Core& core)
    {
        return fdp::step_once(core);
    }

    static bool try_resume(Data& d)
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
    static void lookup_observers(Observers& observers, phy_t phy, T operand)
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
    static void check_breakpoints(Data& d)
    {
        const auto state = fdp::state(d.core);
        if(!state)
            return;

        if(!(*state & FDP_STATE_BREAKPOINT_HIT))
            return;

        auto observers = std::vector<Observer>{};
        lookup_observers(d.observers, d.breakstate.phy, [&](auto it)
        {
            const auto& bp = *it->second;
            if(bp.proc && bp.proc != d.breakstate.proc)
                return walk_e::next;

            if(bp.thread && bp.thread != d.breakstate.thread)
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

    static bool try_wait(Data& d, state_e state, breakpoints_e check)
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

void state::wait_for(core::Core& core, int timeout_ms)
{
    const auto now      = std::chrono::steady_clock::now();
    const auto deadline = now + std::chrono::milliseconds(timeout_ms);
    while(std::chrono::steady_clock::now() < deadline)
    {
        state::resume(core);
        state::wait(core);
    }
}

namespace
{
    static opt<dtb_t> get_dtb_filter(core::Core& core, const BreakpointObserver& bp)
    {
        if(bp.proc)
            return bp.proc->dtb;

        if(!bp.thread)
            return {};

        const auto proc = threads::process(core, *bp.thread);
        if(!proc)
            return {};

        return proc->dtb;
    }

    static opt<int> try_add_breakpoint(core::Core& core, std::string_view name, phy_t phy, const BreakpointObserver& bp)
    {
        auto& d       = *core.state_;
        auto& targets = d.targets;
        auto dtb      = get_dtb_filter(core, bp);
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
                return {};

            // add new breakpoint without filtering
            dtb = {};
        }

        const auto dtb_val = dtb ? dtb->val : 0;
        const auto bpid    = fdp::set_breakpoint(core, FDP_SOFTHBP, 0, FDP_EXECUTE_BP, FDP_PHYSICAL_ADDRESS, phy.val, 1, dtb_val);
        if(bpid < 0)
            return FAIL(ext::nullopt, "unable to set breakpoint %s phy:0x%" PRIx64 " dtb:0x%" PRIx64, std::string{name}.data(), phy.val, dtb_val);

        targets.emplace(phy, Breakpoint{dtb, bpid});
        return bpid;
    }

    static state::Breakpoint set_physical_breakpoint(core::Core& core, std::string_view name, phy_t phy, const opt<proc_t>& proc, const opt<thread_t>& thread, const state::Task& task)
    {
        auto& d       = *core.state_;
        const auto bp = std::make_shared<BreakpointObserver>(task, name, phy, proc, thread);
        d.observers.emplace(phy, bp);
        const auto bpid = try_add_breakpoint(core, std::string{name}, phy, *bp);
        if(!bpid)
            return {};

        // update all observers breakpoint id
        lookup_observers(d.observers, phy, [&](auto it)
        {
            it->second->bpid = *bpid;
            return walk_e::next;
        });
        return std::make_shared<state::BreakpointPrivate>(core, bp);
    }

    static opt<phy_t> to_phy(core::Core& core, uint64_t ptr, const opt<proc_t>& proc)
    {
        const auto current = proc ? proc : process::current(core);
        if(!current)
            return {};

        return core.os_->proc_resolve(*current, ptr);
    }

    static state::Breakpoint set_virtual_breakpoint(core::Core& core, std::string_view name, uint64_t ptr, const opt<proc_t>& proc, const opt<thread_t>& thread, const state::Task& task)
    {
        const auto target = proc ? core.os_->proc_select(*proc, ptr) : ext::nullopt;
        const auto phy    = to_phy(core, ptr, target);
        if(!phy)
            return nullptr;

        return set_physical_breakpoint(core, name, *phy, target, thread, task);
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
    return set_physical_breakpoint(core, name, phy, {}, {}, task);
}

state::Breakpoint state::break_on_physical_process(core::Core& core, std::string_view name, proc_t proc, phy_t phy, const state::Task& task)
{
    return set_physical_breakpoint(core, name, phy, proc, {}, task);
}

namespace
{
    template <typename T>
    static void run_until(Data& d, const T& predicate)
    {
        const auto ok = yield_until(d, predicate);
        if(ok)
            return;

        auto is_end = false;
        d.waiters.emplace_back([&]
        {
            is_end = is_end || predicate();
            return is_end;
        });
        while(!is_end)
        {
            try_resume(d);
            try_wait(d, state_e::update, breakpoints_e::skip);

            // check & remove every ended predicates
            auto any_end = false;
            remove_erase_if(d.waiters, [&](const auto& check)
            {
                const auto ended = check();
                any_end          = any_end || ended;
                return ended;
            });
            // we can return directly if our predicate has ended
            if(is_end)
                return;

            // on other predicates, ignore spurious breakpoints & continue
            if(any_end)
                continue;

            check_breakpoints(d);
        }
    }
}

bool state::run_fast_to_cr3_write(core::Core& core)
{
    const auto bpid = fdp::set_breakpoint(core, FDP_CRHBP, 0, FDP_WRITE_BP, FDP_VIRTUAL_ADDRESS, 3, 1, 0);
    if(bpid < 0)
        return false;

    auto& d = *core.state_;
    try_resume(d);
    try_wait(d, state_e::skip, breakpoints_e::skip);
    fdp::unset_breakpoint(core, bpid);
    return true;
}

void state::run_to_proc(core::Core& core, std::string_view /*name*/, proc_t proc)
{
    const auto bpid = fdp::set_breakpoint(core, FDP_CRHBP, 0, FDP_WRITE_BP, FDP_VIRTUAL_ADDRESS, 3, 1, 0);
    if(bpid < 0)
        return;

    auto& d = *core.state_;
    run_until(d, [&]
    {
        return d.breakstate.proc == proc;
    });
    fdp::unset_breakpoint(core, bpid);
}

void state::run_to_proc_at(core::Core& core, std::string_view name, proc_t proc, uint64_t ptr)
{
    auto& d       = *core.state_;
    const auto bp = set_virtual_breakpoint(core, name, ptr, proc, {}, {});
    run_until(d, [&]
    {
        return d.breakstate.rip == ptr;
    });
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
        const auto got_rsp = registers::read(core, reg_e::rsp);
        return d.breakstate.rip == rip
               && got_rsp == rsp;
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
        uint64_t new_cr3 = registers::read(core, reg_e::cr3);

        if(!ptrs.count(d.breakstate.rip) && (bp_cr3 == BP_CR3_NONE || cr3 == new_cr3))
            return false;

        if(bp_cr3 == BP_CR3_ON_WRITINGS && cr3 != new_cr3)
            cr3 = new_cr3;

        return on_bp(d.breakstate.proc, d.breakstate.thread) == walk_e::stop;
    });

    if(bp_cr3 == BP_CR3_ON_WRITINGS)
        fdp::unset_breakpoint(core, bpid);
}
