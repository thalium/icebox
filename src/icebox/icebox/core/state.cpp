#include "state.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "state"
#include "core.hpp"
#include "core_private.hpp"
#include "fdp.hpp"
#include "log.hpp"
#include "reader.hpp"
#include "utils/fnview.hpp"
#include "utils/utils.hpp"

#include <cstring>
#include <map>
#include <thread>
#include <unordered_map>
#include <unordered_set>

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
}

struct state::State
{
    Breakers   targets;
    Observers  observers;
    BreakState breakstate;
};

std::shared_ptr<state::State> state::setup()
{
    return std::make_shared<state::State>();
}

namespace
{
    static bool update_break_state(core::Core& core)
    {
        auto& d = *core.state_;
        memset(&d.breakstate, 0, sizeof d.breakstate);
        const auto thread = os::thread_current(core);
        if(!thread)
            return FAIL(false, "unable to get current thread");

        const auto proc = os::thread_proc(core, *thread);
        if(!proc)
            return FAIL(false, "unable to get current proc");

        const auto rip = registers::read(core, FDP_RIP_REGISTER);
        const auto dtb = dtb_t{registers::read(core, FDP_CR3_REGISTER)};
        const auto phy = memory::virtual_to_physical(core, rip, dtb);
        if(!phy)
            return FAIL(false, "unable to get current physical address");

        d.breakstate.thread = *thread;
        d.breakstate.proc   = *proc;
        d.breakstate.rip    = rip;
        d.breakstate.dtb    = dtb;
        d.breakstate.phy    = *phy;
        os::debug_print(core);
        return true;
    }

    static bool try_pause(core::Core& core)
    {
        const auto state = fdp::state(core);
        if(!state)
            return false;

        if(*state & FDP_STATE_PAUSED)
            return true;

        const auto ok = fdp::pause(core);
        if(!ok)
            return FAIL(false, "unable to pause");

        const auto updated = update_break_state(core);
        return updated;
    }

    static bool try_single_step(core::Core& core)
    {
        return fdp::step_once(core);
    }

    static bool try_resume(core::Core& core)
    {
        const auto state = fdp::state(core);
        if(!state)
            return false;

        if(!(*state & FDP_STATE_PAUSED))
            return true;

        if(*state & (FDP_STATE_BREAKPOINT_HIT | FDP_STATE_HARD_BREAKPOINT_HIT))
            if(!try_single_step(core))
                return false;

        const auto resumed = fdp::resume(core);
        if(!resumed)
            return FAIL(false, "unable to resume");

        return true;
    }
}

bool state::pause(core::Core& core)
{
    return try_pause(core);
}

bool state::resume(core::Core& core)
{
    return try_resume(core);
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
            if(operand(it++) == walk_e::WALK_STOP)
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
                return WALK_STOP;

            return WALK_NEXT;
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
    static void check_breakpoints(core::Core& core)
    {
        const auto state = fdp::state(core);
        if(!state)
            return;

        if(!(*state & FDP_STATE_BREAKPOINT_HIT))
            return;

        auto& d = *core.state_;
        std::vector<Observer> observers;
        lookup_observers(d.observers, d.breakstate.phy, [&](auto it)
        {
            const auto& bp = *it->second;
            if(bp.proc && bp.proc != d.breakstate.proc)
                return WALK_NEXT;

            if(bp.thread && bp.thread != d.breakstate.thread)
                return WALK_NEXT;

            if(bp.task)
                observers.push_back(it->second);

            return WALK_NEXT;
        });
        for(const auto& it : observers)
            it->task();
    }

    enum check_e
    {
        CHECK_BREAKPOINTS,
        SKIP_BREAKPOINTS
    };

    static bool try_wait(core::Core& core, check_e check)
    {
        while(true)
        {
            std::this_thread::yield();
            const auto ok = fdp::state_changed(core);
            if(!ok)
                continue;

            update_break_state(core);
            if(check == CHECK_BREAKPOINTS)
                check_breakpoints(core);
            return true;
        }
    }
}

bool state::wait(core::Core& core)
{
    return try_wait(core, CHECK_BREAKPOINTS);
}

namespace
{
    static opt<dtb_t> get_dtb_filter(core::Core& core, const BreakpointObserver& bp)
    {
        if(bp.proc)
            return bp.proc->dtb;

        if(!bp.thread)
            return {};

        const auto proc = os::thread_proc(core, *bp.thread);
        if(!proc)
            return {};

        return proc->dtb;
    }

    static int try_add_breakpoint(core::Core& core, phy_t phy, const BreakpointObserver& bp)
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
                return -1;

            // add new breakpoint without filtering
            dtb = {};
        }

        const auto bpid = fdp::set_breakpoint(core, FDP_SOFTHBP, 0, FDP_EXECUTE_BP, FDP_PHYSICAL_ADDRESS, phy.val, 1, dtb ? dtb->val : 0);
        if(bpid < 0)
            return -1;

        targets.emplace(phy, Breakpoint{dtb, bpid});
        return bpid;
    }

    static state::Breakpoint set_breakpoint(core::Core& core, std::string_view name, phy_t phy, const opt<proc_t>& proc, const opt<thread_t>& thread, const state::Task& task)
    {
        auto& d       = *core.state_;
        const auto bp = std::make_shared<BreakpointObserver>(task, name, phy, proc, thread);
        d.observers.emplace(phy, bp);
        const auto bpid = try_add_breakpoint(core, phy, *bp);

        // update all observers breakpoint id
        lookup_observers(d.observers, phy, [&](auto it)
        {
            it->second->bpid = bpid;
            return WALK_NEXT;
        });
        return std::make_shared<state::BreakpointPrivate>(core, bp);
    }

    static opt<phy_t> to_phy(core::Core& core, uint64_t ptr, const opt<proc_t>& proc)
    {
        const auto current = proc ? proc : os::proc_current(core);
        if(!current)
            return {};

        return os::proc_resolve(core, *current, ptr);
    }

    static state::Breakpoint set_breakpoint(core::Core& core, std::string_view name, uint64_t ptr, const opt<proc_t>& proc, const opt<thread_t>& thread, const state::Task& task)
    {
        const auto target = proc ? os::proc_select(core, *proc, ptr) : ext::nullopt;
        const auto phy    = to_phy(core, ptr, target);
        if(!phy)
            return nullptr;

        return set_breakpoint(core, name, *phy, target, thread, task);
    }
}

state::Breakpoint state::set_breakpoint(core::Core& core, std::string_view name, uint64_t ptr, const state::Task& task)
{
    return ::set_breakpoint(core, name, ptr, {}, {}, task);
}

state::Breakpoint state::set_breakpoint(core::Core& core, std::string_view name, uint64_t ptr, proc_t proc, const state::Task& task)
{
    return ::set_breakpoint(core, name, ptr, proc, {}, task);
}

state::Breakpoint state::set_breakpoint(core::Core& core, std::string_view name, uint64_t ptr, thread_t thread, const state::Task& task)
{
    return ::set_breakpoint(core, name, ptr, {}, thread, task);
}

state::Breakpoint state::set_breakpoint(core::Core& core, std::string_view name, phy_t phy, const state::Task& task)
{
    return ::set_breakpoint(core, name, phy, {}, {}, task);
}

state::Breakpoint state::set_breakpoint(core::Core& core, std::string_view name, phy_t phy, proc_t proc, const state::Task& task)
{
    return ::set_breakpoint(core, name, phy, proc, {}, task);
}

state::Breakpoint state::set_breakpoint(core::Core& core, std::string_view name, phy_t phy, thread_t thread, const state::Task& task)
{
    return ::set_breakpoint(core, name, phy, {}, thread, task);
}

namespace
{
    template <typename T>
    static void run_until(core::Core& core, const T& predicate)
    {
        while(true)
        {
            try_resume(core);
            try_wait(core, SKIP_BREAKPOINTS);

            if(predicate())
                return;

            check_breakpoints(core);
        }
    }
}

void state::run_to_proc(core::Core& core, std::string_view /*name*/, proc_t proc)
{
    const auto bpid = fdp::set_breakpoint(core, FDP_CRHBP, 0, FDP_WRITE_BP, FDP_VIRTUAL_ADDRESS, 3, 1, 0);
    if(bpid < 0)
        return;

    auto& d = *core.state_;
    run_until(core, [&]
    {
        return d.breakstate.proc == proc;
    });
    fdp::unset_breakpoint(core, bpid);
}

void state::run_to_proc(core::Core& core, std::string_view name, proc_t proc, uint64_t ptr)
{
    auto& d       = *core.state_;
    const auto bp = ::set_breakpoint(core, name, ptr, proc, {}, {});
    run_until(core, [&]
    {
        return d.breakstate.rip == ptr;
    });
}

void state::run_to_current(core::Core& core, std::string_view name)
{
    auto& d           = *core.state_;
    const auto thread = os::thread_current(core);
    const auto rsp    = registers::read(core, FDP_RSP_REGISTER);
    const auto rip    = registers::read(core, FDP_RIP_REGISTER);
    const auto bp     = ::set_breakpoint(core, name, rip, {}, *thread, {});
    run_until(core, [&]
    {
        const auto got_rsp = registers::read(core, FDP_RSP_REGISTER);
        return d.breakstate.rip == rip
               && got_rsp == rsp;
    });
}

void state::run_to(core::Core& core, std::string_view name, std::unordered_set<uint64_t> ptrs, bp_cr3_e bp_cr3, fn::view<walk_e(proc_t, thread_t)> on_bp)
{
    if((bp_cr3 == BP_CR3_NONE) & ptrs.empty())
        return;

    auto bps = std::vector<state::Breakpoint>{};
    bps.reserve(ptrs.size());
    for(const uint64_t& ptr : ptrs)
        bps.push_back(::set_breakpoint(core, name, ptr, {}, {}, {}));

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

        cr3 = registers::read(core, FDP_CR3_REGISTER);
    }

    auto& d = *core.state_;
    run_until(core, [&]
    {
        uint64_t new_cr3 = registers::read(core, FDP_CR3_REGISTER);

        if(!ptrs.count(d.breakstate.rip) & (bp_cr3 == BP_CR3_NONE || cr3 == new_cr3))
            return false; // WALK_NEXT

        if(bp_cr3 == BP_CR3_ON_WRITINGS && cr3 != new_cr3)
            cr3 = new_cr3;

        return (on_bp(d.breakstate.proc, d.breakstate.thread) == WALK_STOP); // WALK_STOP
    });

    if(bp_cr3 == BP_CR3_ON_WRITINGS)
        fdp::unset_breakpoint(core, bpid);
}
