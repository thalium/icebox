#include "state.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "state"
#include "log.hpp"
#include "utils/utils.hpp"
#include "core.hpp"
#include "private.hpp"
#include "os.hpp"

#include <FDP.h>

#include <map>
#include <unordered_map>
#include <thread>
#include <unordered_set>

namespace
{
    struct Breakpoint
    {
        opt<uint64_t> dtb;
        int           id;
    };

    struct BreakpointObserver
    {
        BreakpointObserver(const core::Task& task, uint64_t phy, proc_t proc, core::filter_e filter)
            : task(task)
            , phy(phy)
            , proc(proc)
            , filter(filter)
            , bpid(-1)
        {
        }

        core::Task      task;
        uint64_t        phy;
        proc_t          proc;
        core::filter_e  filter;
        int             bpid;

    };

    using Observer = std::shared_ptr<BreakpointObserver>;

    using Breakpoints = struct
    {
        std::unordered_map<uint64_t, Breakpoint>    targets_;
        std::multimap<uint64_t, Observer>           observers_;
    };
}

struct core::State::Data
{
    Data(FDP_SHM& shm, Core& core)
        : shm(shm)
        , core(core)
    {
    }

    FDP_SHM&    shm;
    Core&       core;
    Breakpoints breakpoints;
    proc_t      current;
};

using StateData = core::State::Data;

core::State::State()
{
}

core::State::~State()
{
}

void core::setup(State& mem, FDP_SHM& shm, Core& core)
{
    mem.d_ = std::make_unique<core::State::Data>(shm, core);
}

namespace
{
    bool update_break_state(StateData& d)
    {
        d.core.os->debug_print();
        const auto current = d.core.os->proc_current();
        if(!current)
            FAIL(false, "unable to get current process & update break state");

        d.current = *current;
        d.core.mem.update(*current);
        return true;
    }

    bool try_pause(StateData& d)
    {
        FDP_State state = FDP_STATE_NULL;
        const auto ok = FDP_GetState(&d.shm, &state);
        if(!ok)
            return false;

        if(state & FDP_STATE_PAUSED)
            return true;

        FDP_Pause(&d.shm);
        if(!ok)
            FAIL(false, "unable to pause");

        const auto updated = update_break_state(d);
        return updated;
    }

    bool try_single_step(StateData& d)
    {
        return FDP_SingleStep(&d.shm, 0);
    }

    bool try_resume(StateData& d)
    {
        FDP_State state = FDP_STATE_NULL;
        const auto ok = FDP_GetState(&d.shm, &state);
        if(!ok)
            return false;

        if(!(state & FDP_STATE_PAUSED))
            return true;

        if(state & (FDP_STATE_BREAKPOINT_HIT | FDP_STATE_HARD_BREAKPOINT_HIT))
            if(!try_single_step(d))
                return false;

        const auto resumed = FDP_Resume(&d.shm);
        if(!resumed)
            FAIL(false, "unable to resume");

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

struct core::BreakpointPrivate
{
    BreakpointPrivate(StateData& data, const Observer& observer)
        : data_(data)
        , observer_(observer)
    {
    }

    ~BreakpointPrivate()
    {
        utils::erase_if(data_.breakpoints.observers_, [&](auto bp)
        {
            return observer_ == bp;
        });
        const auto range = data_.breakpoints.observers_.equal_range(observer_->phy);
        const auto empty = range.first == range.second;
        if(!empty)
            return;

        const auto ok = FDP_UnsetBreakpoint(&data_.shm, observer_->bpid);
        if(!ok)
            LOG(ERROR, "unable to remove breakpoint %d", observer_->bpid);

        data_.breakpoints.targets_.erase(observer_->phy);
    }

    StateData&  data_;
    Observer    observer_;
};

namespace
{
    void check_breakpoints(StateData& d, FDP_State state)
    {
        if(!(state & FDP_STATE_BREAKPOINT_HIT))
            return;

        if(d.breakpoints.observers_.empty())
            return;

        const auto rip = d.core.regs.read(FDP_RIP_REGISTER);
        if(!rip)
            return;

        const auto cr3 = d.core.regs.read(FDP_CR3_REGISTER);
        if(!cr3)
            return;

        uint64_t phy = 0;
        const auto ok = FDP_VirtualToPhysical(&d.shm, 0, *rip, &phy);
        if(!ok)
            return;

        const auto range = d.breakpoints.observers_.equal_range(phy);
        for(auto it = range.first; it != range.second; ++it)
        {
            const auto& bp = *it->second;
            if(bp.filter == core::FILTER_CR3 && bp.proc.dtb != *cr3)
                continue;

            if(bp.task)
                bp.task();
        }
    }

    bool try_wait(StateData& d)
    {
        while(true)
        {
            std::this_thread::yield();
            auto ok = FDP_GetStateChanged(&d.shm);
            if(!ok)
                continue;

            update_break_state(d);
            FDP_State state = FDP_STATE_NULL;
            ok = FDP_GetState(&d.shm, &state);
            if(!ok)
                return false;

            check_breakpoints(d, state);
            return true;
        }
    }
}

bool core::State::wait()
{
    return try_wait(*d_);
}

namespace
{
    int try_add_breakpoint(StateData& d, uint64_t phy, const BreakpointObserver& bp)
    {
        auto& targets = d.breakpoints.targets_;
        auto dtb = bp.filter == core::FILTER_CR3 ? ext::make_optional(bp.proc.dtb) : ext::nullopt;
        const auto it = targets.find(phy);
        if(it != targets.end())
        {
            // keep using found breakpoint if filtering rules are compatible
            const auto bp_dtb = it->second.dtb;
            if(!bp_dtb || bp_dtb == dtb)
                return it->second.id;

            // filtering rules are too restrictive, remove old breakpoint & add an unfiltered breakpoint
            const auto ok = FDP_UnsetBreakpoint(&d.shm, it->second.id);
            targets.erase(it);
            if(!ok)
                return -1;

            // add new breakpoint without filtering
            dtb = ext::nullopt;
        }

        const auto bpid = FDP_SetBreakpoint(&d.shm, 0, FDP_SOFTHBP, 0, FDP_EXECUTE_BP, FDP_PHYSICAL_ADDRESS, phy, 1, dtb ? *dtb : FDP_NO_CR3);
        if(bpid < 0)
            return -1;

        targets.emplace(phy, Breakpoint{dtb, bpid});
        return bpid;
    }

    core::Breakpoint set_breakpoint(StateData& d, uint64_t ptr, proc_t proc, core::filter_e filter, const core::Task& task)
    {
        const auto phy = d.core.mem.virtual_to_physical(ptr, proc.dtb);
        if(!phy)
            return nullptr;

        const auto bp = std::make_shared<BreakpointObserver>(task, *phy, proc, filter);
        d.breakpoints.observers_.emplace(*phy, bp);
        const auto bpid = try_add_breakpoint(d, *phy, *bp);

        // update all observers breakpoint id
        const auto range = d.breakpoints.observers_.equal_range(*phy);
        for(auto it = range.first; it != range.second; ++it)
            it->second->bpid = bpid;

        return std::make_shared<core::BreakpointPrivate>(d, bp);
    }
}

core::Breakpoint core::State::set_breakpoint(uint64_t ptr, proc_t proc, core::filter_e filter, const core::Task& task)
{
    return ::set_breakpoint(*d_, ptr, proc, filter, task);
}

core::Breakpoint core::State::set_breakpoint(uint64_t ptr, proc_t proc, filter_e filter)
{
    return ::set_breakpoint(*d_, ptr, proc, filter, {});
}

namespace
{
    bool try_proc_join(StateData& d, proc_t proc, core::join_e join)
    {
        // set breakpoint on CR3 changes
        const auto bpid = FDP_SetBreakpoint(&d.shm, 0, FDP_CRHBP, 0, FDP_WRITE_BP, FDP_VIRTUAL_ADDRESS, 3, 1, FDP_NO_CR3);
        if(bpid < 0)
            return false;

        std::vector<core::Breakpoint> thread_rips;
        const auto set_thread_rips = [&]
        {
            std::unordered_set<uint64_t> rips;
            thread_rips.clear();
            d.core.os->thread_list(proc, [&](thread_t thread)
            {
                const auto rip = d.core.os->thread_pc(proc, thread);
                if(!rip)
                    return WALK_NEXT;

                const auto inserted = rips.emplace(*rip).second;
                if(!inserted)
                    return WALK_NEXT;

                auto bp = ::set_breakpoint(d, *rip, proc, core::FILTER_CR3, {});
                if(!bp)
                    return WALK_NEXT;

                thread_rips.emplace_back(bp);
                return WALK_NEXT;
            });
        };

        set_thread_rips();
        bool found = false;
        while(true)
        {
            if(!try_resume(d))
                break;

            if(!try_wait(d))
                break;

            const auto same_proc = proc.id == d.current.id && proc.dtb == d.current.dtb;
            if(!same_proc)
                continue;

            found = join == core::JOIN_ANY_MODE;
            if(found)
                break;

            const auto cs = d.core.regs.read(FDP_CS_REGISTER);
            found = cs && !!(*cs & 3);
            if(found)
                break;

            set_thread_rips();
        }
        FDP_UnsetBreakpoint(&d.shm, bpid);
        return found;
    }
}

bool core::State::proc_join(proc_t proc, join_e join)
{
    return try_proc_join(*d_, proc, join);
}
