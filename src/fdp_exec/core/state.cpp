#define PRIVATE_CORE__
#include "private.hpp"

#define FDP_MODULE "state"
#include "log.hpp"
#include "utils.hpp"
#include "core.hpp"

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

    struct Stater
        : public core::IState
    {
        Stater(FDP_SHM& shm, core::IHandler& core)
            : shm_(shm)
            , core_(core)
        {
        }

        bool                pause           () override;
        bool                resume          () override;
        bool                wait            () override;
        core::Breakpoint    set_breakpoint  (uint64_t ptr, proc_t proc, core::filter_e filter, const core::Task& task) override;

        // members
        FDP_SHM&        shm_;
        core::IHandler& core_;
        Breakpoints     breakpoints_;
    };
}

namespace core
{
    std::unique_ptr<IState> make_state(FDP_SHM& shm, IHandler& core)
    {
        return std::make_unique<Stater>(shm, core);
    }
}

namespace
{
    bool update_break_state(Stater& s)
    {
        const auto current = s.core_.get_current_proc();
        if(!current)
            FAIL(false, "unable to get current process & update break state");

        s.core_.update({*current});
        return true;
    }
}

bool Stater::pause()
{
    const auto ok = FDP_Pause(&shm_);
    if(!ok)
        FAIL(false, "unable to pause");

    const auto updated = update_break_state(*this);
    return updated;
}

bool Stater::resume()
{
    FDP_State state = FDP_STATE_NULL;
    auto ok = FDP_GetState(&shm_, &state);
    if(ok)
        if(state & FDP_STATE_BREAKPOINT_HIT)
            FDP_SingleStep(&shm_, 0);

    ok = FDP_Resume(&shm_);
    if(!ok)
        FAIL(false, "unable to resume");

    return true;
}

struct core::BreakpointPrivate
{
    BreakpointPrivate(Stater& core, const Observer& observer)
        : core_(core)
        , observer_(observer)
    {
    }

    ~BreakpointPrivate()
    {
        utils::erase_if(core_.breakpoints_.observers_, [&](auto bp)
        {
            return observer_ == bp;
        });
        const auto range = core_.breakpoints_.observers_.equal_range(observer_->phy);
        const auto empty = range.first == range.second;
        if(!empty)
            return;

        const auto ok = FDP_UnsetBreakpoint(&core_.shm_, static_cast<uint8_t>(observer_->bpid));
        if(!ok)
            LOG(ERROR, "unable to remove breakpoint %d", observer_->bpid);

        core_.breakpoints_.targets_.erase(observer_->phy);
    }

    Stater&     core_;
    Observer    observer_;
};

namespace
{
    void check_breakpoints(Stater& s, FDP_State state)
    {
        if(!(state & FDP_STATE_BREAKPOINT_HIT))
            return;

        if(s.breakpoints_.observers_.empty())
            return;

        const auto rip = s.core_.read_reg(FDP_RIP_REGISTER);
        if(!rip)
            return;

        const auto cr3 = s.core_.read_reg(FDP_CR3_REGISTER);
        if(!cr3)
            return;

        const auto phy = s.core_.virtual_to_physical(*rip, *cr3);
        if(!phy)
            return;

        const auto range = s.breakpoints_.observers_.equal_range(*phy);
        for(auto it = range.first; it != range.second; ++it)
        {
            const auto& bp = *it->second;
            if(bp.filter == core::FILTER_CR3 && bp.proc.dtb != *cr3)
                continue;

            bp.task();
        }
    }
}

bool Stater::wait()
{
    while(true)
    {
        std::this_thread::yield();
        auto ok = FDP_GetStateChanged(&shm_);
        if(!ok)
            continue;

        FDP_State state = FDP_STATE_NULL;
        ok = FDP_GetState(&shm_, &state);
        if(!ok)
            return false;

        check_breakpoints(*this, state);
        return true;
    }
}

namespace
{
    int try_add_breakpoint(Stater& s, uint64_t phy, const BreakpointObserver& bp)
    {
        auto& targets = s.breakpoints_.targets_;
        auto dtb = bp.filter == core::FILTER_CR3 ? std::make_optional(bp.proc.dtb) : std::nullopt;
        const auto it = targets.find(phy);
        if(it != targets.end())
        {
            // keep using found breakpoint if filtering rules are compatible
            const auto bp_dtb = it->second.dtb;
            if(!bp_dtb || bp_dtb == dtb)
                return it->second.id;

            // filtering rules are too restrictive, remove old breakpoint & add an unfiltered breakpoint
            const auto ok = FDP_UnsetBreakpoint(&s.shm_, static_cast<uint8_t>(it->second.id));
            targets.erase(it);
            if(!ok)
                return -1;

            // add new breakpoint without filtering
            dtb = std::nullopt;
        }

        const auto bpid = FDP_SetBreakpoint(&s.shm_, 0, FDP_SOFTHBP, 0, FDP_EXECUTE_BP, FDP_PHYSICAL_ADDRESS, phy, 1, dtb ? *dtb : FDP_NO_CR3);
        if(bpid < 0)
            return -1;

        targets.emplace(phy, Breakpoint{dtb, bpid});
        return bpid;
    }
}

core::Breakpoint Stater::set_breakpoint(uint64_t ptr, proc_t proc, core::filter_e filter, const core::Task& task)
{
    const auto phy = core_.virtual_to_physical(ptr, proc.dtb);
    if(!phy)
        return nullptr;

    const auto bp = std::make_shared<BreakpointObserver>(task, *phy, proc, filter);
    breakpoints_.observers_.emplace(*phy, bp);
    const auto bpid = try_add_breakpoint(*this, *phy, *bp);

    // update all observers breakpoint id
    const auto range = breakpoints_.observers_.equal_range(*phy);
    for(auto it = range.first; it != range.second; ++it)
        it->second->bpid = bpid;

    return std::make_shared<core::BreakpointPrivate>(*this, bp);
}
