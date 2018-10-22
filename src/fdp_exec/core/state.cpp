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
#include <thread>

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
        const auto current = d.core.os->proc_current();
        if(!current)
            FAIL(false, "unable to get current process & update break state");

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

        const auto paused = FDP_Pause(&d.shm);
        if(!ok)
            FAIL(false, "unable to pause");

        const auto updated = update_break_state(d);
        return updated;
    }

    bool try_resume(StateData& d)
    {
        FDP_State state = FDP_STATE_NULL;
        const auto ok = FDP_GetState(&d.shm, &state);
        if(!ok)
            return false;

        if(!(state & FDP_STATE_PAUSED))
            return true;

        if(false)
            if(state & FDP_STATE_BREAKPOINT_HIT)
                FDP_SingleStep(&d.shm, 0);

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
}

bool core::State::wait()
{
    auto& shm = d_->shm;
    while(true)
    {
        std::this_thread::yield();
        auto ok = FDP_GetStateChanged(&shm);
        if(!ok)
            continue;

        update_break_state(*d_);
        FDP_State state = FDP_STATE_NULL;
        ok = FDP_GetState(&shm, &state);
        if(!ok)
            return false;

        check_breakpoints(*d_, state);
        return true;
    }
}

namespace
{
    int try_add_breakpoint(StateData& d, uint64_t phy, const BreakpointObserver& bp)
    {
        auto& targets = d.breakpoints.targets_;
        auto dtb = bp.filter == core::FILTER_CR3 ? std::make_optional(bp.proc.dtb) : std::nullopt;
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
            dtb = std::nullopt;
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
