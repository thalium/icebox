#include "state.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "state"
#include "core.hpp"
#include "fdp.hpp"
#include "log.hpp"
#include "os.hpp"
#include "private.hpp"
#include "reader.hpp"
#include "utils/fnview.hpp"
#include "utils/utils.hpp"

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
        BreakpointObserver(core::Task task, std::string_view name, phy_t phy, opt<proc_t> proc, opt<thread_t> thread)
            : task(std::move(task))
            , name(name)
            , phy(phy)
            , proc(std::move(proc))
            , thread(std::move(thread))
            , bpid(-1)
        {
        }

        core::Task    task;
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

struct core::State::Data
{
    Data(fdp::shm& shm, Core& core)
        : shm(shm)
        , core(core)
    {
    }

    fdp::shm&  shm;
    Core&      core;
    Breakers   targets;
    Observers  observers;
    BreakState breakstate;
};

using StateData = core::State::Data;

core::State::State()  = default;
core::State::~State() = default;

void core::setup(State& state, fdp::shm& shm, Core& core)
{
    state.d_ = std::make_unique<core::State::Data>(shm, core);
}

namespace
{
    static bool update_break_state(StateData& d)
    {
        memset(&d.breakstate, 0, sizeof d.breakstate);
        const auto thread = d.core.os->thread_current();
        if(!thread)
            return FAIL(false, "unable to get current thread");

        const auto proc = d.core.os->thread_proc(*thread);
        if(!proc)
            return FAIL(false, "unable to get current proc");

        const auto rip = d.core.regs.read(FDP_RIP_REGISTER);
        const auto dtb = dtb_t{d.core.regs.read(FDP_CR3_REGISTER)};
        const auto phy = d.core.mem.virtual_to_physical(rip, dtb);
        if(!phy)
            return FAIL(false, "unable to get current physical address");

        d.breakstate.thread = *thread;
        d.breakstate.proc   = *proc;
        d.breakstate.rip    = rip;
        d.breakstate.dtb    = dtb;
        d.breakstate.phy    = *phy;
        d.core.os->debug_print();
        return true;
    }

    static bool try_pause(StateData& d)
    {
        const auto state = fdp::state(d.shm);
        if(!state)
            return false;

        if(*state & FDP_STATE_PAUSED)
            return true;

        const auto ok = fdp::pause(d.shm);
        if(!ok)
            return FAIL(false, "unable to pause");

        const auto updated = update_break_state(d);
        return updated;
    }

    static bool try_single_step(StateData& d)
    {
        return fdp::step_once(d.shm);
    }

    static bool try_resume(StateData& d)
    {
        const auto state = fdp::state(d.shm);
        if(!state)
            return false;

        if(!(*state & FDP_STATE_PAUSED))
            return true;

        if(*state & (FDP_STATE_BREAKPOINT_HIT | FDP_STATE_HARD_BREAKPOINT_HIT))
            if(!try_single_step(d))
                return false;

        const auto resumed = fdp::resume(d.shm);
        if(!resumed)
            return FAIL(false, "unable to resume");

        return true;
    }
}

bool core::State::pause()
{
    return try_pause(*d_);
}

bool core::State::resume()
{
    return try_resume(*d_);
}

bool core::State::single_step()
{
    return try_single_step(*d_);
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

struct core::BreakpointPrivate
{
    BreakpointPrivate(StateData& data, Observer observer)
        : data_(data)
        , observer_(std::move(observer))
    {
    }

    ~BreakpointPrivate()
    {
        bool unique_observer = true;
        opt<Observers::iterator> target;
        lookup_observers(data_.observers, observer_->phy, [&](auto it)
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

        data_.observers.erase(*target);
        if(!unique_observer)
            return;

        const auto ok = fdp::unset_breakpoint(data_.shm, observer_->bpid);
        if(!ok)
            LOG(ERROR, "unable to remove breakpoint {}", observer_->bpid);

        data_.targets.erase(observer_->phy);
    }

    StateData& data_;
    Observer   observer_;
};

namespace
{
    static void check_breakpoints(StateData& d)
    {
        const auto state = fdp::state(d.shm);
        if(!state)
            return;

        if(!(*state & FDP_STATE_BREAKPOINT_HIT))
            return;

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

    static bool try_wait(StateData& d, check_e check)
    {
        while(true)
        {
            std::this_thread::yield();
            const auto ok = fdp::state_changed(d.shm);
            if(!ok)
                continue;

            update_break_state(d);
            if(check == CHECK_BREAKPOINTS)
                check_breakpoints(d);
            return true;
        }
    }
}

bool core::State::wait()
{
    return try_wait(*d_, CHECK_BREAKPOINTS);
}

namespace
{
    static opt<dtb_t> get_dtb_filter(StateData& d, const BreakpointObserver& bp)
    {
        if(bp.proc)
            return bp.proc->dtb;

        if(!bp.thread)
            return {};

        const auto proc = d.core.os->thread_proc(*bp.thread);
        if(!proc)
            return {};

        return proc->dtb;
    }

    static int try_add_breakpoint(StateData& d, phy_t phy, const BreakpointObserver& bp)
    {
        auto& targets = d.targets;
        auto dtb      = get_dtb_filter(d, bp);
        const auto it = targets.find(phy);
        if(it != targets.end())
        {
            // keep using found breakpoint if filtering rules are compatible
            const auto bp_dtb = it->second.dtb;
            if(!bp_dtb || bp_dtb == dtb)
                return it->second.id;

            // filtering rules are too restrictive, remove old breakpoint & add an unfiltered breakpoint
            const auto ok = fdp::unset_breakpoint(d.shm, it->second.id);
            targets.erase(it);
            if(!ok)
                return -1;

            // add new breakpoint without filtering
            dtb = {};
        }

        const auto bpid = fdp::set_breakpoint(d.shm, FDP_SOFTHBP, 0, FDP_EXECUTE_BP, FDP_PHYSICAL_ADDRESS, phy.val, 1, dtb ? dtb->val : 0);
        if(bpid < 0)
            return -1;

        targets.emplace(phy, Breakpoint{dtb, bpid});
        return bpid;
    }

    static core::Breakpoint set_breakpoint(StateData& d, std::string_view name, phy_t phy, const opt<proc_t>& proc, const opt<thread_t>& thread, const core::Task& task)
    {
        const auto bp = std::make_shared<BreakpointObserver>(task, name, phy, proc, thread);
        d.observers.emplace(phy, bp);
        const auto bpid = try_add_breakpoint(d, phy, *bp);

        // update all observers breakpoint id
        lookup_observers(d.observers, phy, [&](auto it)
        {
            it->second->bpid = bpid;
            return WALK_NEXT;
        });
        return std::make_shared<core::BreakpointPrivate>(d, bp);
    }

    static opt<phy_t> to_phy(StateData& d, uint64_t ptr, const opt<proc_t>& proc)
    {
        const auto current = proc ? proc : d.core.os->proc_current();
        if(!current)
            return {};

        return d.core.os->proc_resolve(*current, ptr);
    }

    static core::Breakpoint set_breakpoint(StateData& d, std::string_view name, uint64_t ptr, const opt<proc_t>& proc, const opt<thread_t>& thread, const core::Task& task)
    {
        const auto target = proc ? d.core.os->proc_select(*proc, ptr) : ext::nullopt;
        const auto phy    = to_phy(d, ptr, target);
        if(!phy)
            return nullptr;

        return set_breakpoint(d, name, *phy, target, thread, task);
    }
}

core::Breakpoint core::State::set_breakpoint(std::string_view name, uint64_t ptr, const core::Task& task)
{
    return ::set_breakpoint(*d_, name, ptr, {}, {}, task);
}

core::Breakpoint core::State::set_breakpoint(std::string_view name, uint64_t ptr, proc_t proc, const core::Task& task)
{
    return ::set_breakpoint(*d_, name, ptr, proc, {}, task);
}

core::Breakpoint core::State::set_breakpoint(std::string_view name, uint64_t ptr, thread_t thread, const core::Task& task)
{
    return ::set_breakpoint(*d_, name, ptr, {}, thread, task);
}

core::Breakpoint core::State::set_breakpoint(std::string_view name, phy_t phy, const core::Task& task)
{
    return ::set_breakpoint(*d_, name, phy, {}, {}, task);
}

core::Breakpoint core::State::set_breakpoint(std::string_view name, phy_t phy, proc_t proc, const core::Task& task)
{
    return ::set_breakpoint(*d_, name, phy, proc, {}, task);
}

core::Breakpoint core::State::set_breakpoint(std::string_view name, phy_t phy, thread_t thread, const core::Task& task)
{
    return ::set_breakpoint(*d_, name, phy, {}, thread, task);
}

namespace
{
    template <typename T>
    static void run_until(StateData& d, const T& predicate)
    {
        while(true)
        {
            try_resume(d);
            try_wait(d, SKIP_BREAKPOINTS);

            if(predicate())
                return;

            check_breakpoints(d);
        }
    }
}

void core::State::run_to_proc(std::string_view /*name*/, proc_t proc)
{
    auto d          = *d_;
    const auto bpid = fdp::set_breakpoint(d.shm, FDP_CRHBP, 0, FDP_WRITE_BP, FDP_VIRTUAL_ADDRESS, 3, 1, 0);
    if(bpid < 0)
        return;

    run_until(d, [&]
    {
        return d.breakstate.proc == proc;
    });
    fdp::unset_breakpoint(d.shm, bpid);
}

void core::State::run_to_proc(std::string_view name, proc_t proc, uint64_t ptr)
{
    auto& d       = *d_;
    const auto bp = ::set_breakpoint(d, name, ptr, proc, {}, {});
    run_until(d, [&]
    {
        return d.breakstate.rip == ptr;
    });
}

void core::State::run_to_current(std::string_view name)
{
    auto& d           = *d_;
    const auto thread = d.core.os->thread_current();
    const auto rsp    = d.core.regs.read(FDP_RSP_REGISTER);
    const auto rip    = d.core.regs.read(FDP_RIP_REGISTER);
    const auto bp     = ::set_breakpoint(d, name, rip, {}, *thread, {});
    run_until(d, [&]
    {
        const auto got_rsp = d.core.regs.read(FDP_RSP_REGISTER);
        return d.breakstate.rip == rip
               && got_rsp == rsp;
    });
}

void core::State::run_to(std::string_view name, std::unordered_set<uint64_t> ptrs, bp_cr3_e bp_cr3, fn::view<walk_e(proc_t, thread_t)> on_bp)
{
    auto& d = *d_;

    if((bp_cr3 == BP_CR3_NONE) & ptrs.empty())
        return;

    std::vector<core::Breakpoint> bp;
    for(const uint64_t& ptr : ptrs)
        bp.push_back(::set_breakpoint(d, name, ptr, {}, {}, {}));

    int bpid = -1;
    uint64_t cr3;
    if(bp_cr3 == BP_CR3_ON_WRITINGS)
    {
        bpid = fdp::set_breakpoint(d.shm, FDP_CRHBP, 0, FDP_WRITE_BP, FDP_VIRTUAL_ADDRESS, 3, 1, 0);
        if(bpid < 0)
        {
            LOG(ERROR, "unable to set a breakpoint on CR3 writes");
            return;
        }

        cr3 = d.core.regs.read(FDP_CR3_REGISTER);
    }

    run_until(d, [&]
    {
        uint64_t new_cr3 = d.core.regs.read(FDP_CR3_REGISTER);

        if(!ptrs.count(d.breakstate.rip) & (bp_cr3 == BP_CR3_NONE || cr3 == new_cr3))
            return false; // WALK_NEXT

        if(bp_cr3 == BP_CR3_ON_WRITINGS && cr3 != new_cr3)
            cr3 = new_cr3;

        return (on_bp(d.breakstate.proc, d.breakstate.thread) == WALK_STOP); // WALK_STOP
    });

    if(bp_cr3 == BP_CR3_ON_WRITINGS)
        fdp::unset_breakpoint(d.shm, bpid);
}
