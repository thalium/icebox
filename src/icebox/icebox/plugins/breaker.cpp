#include "breaker.hpp"

#include "core.hpp"
#include "os.hpp"
#include "reader.hpp"
#include "state.hpp"

#include <unordered_map>

namespace
{
    using Returns = std::unordered_map<uint64_t, core::Breakpoint>;
}

struct state::Breaker::Data
{
    Data(core::Core& core, proc_t proc)
        : core(core)
        , proc(proc)
        , reader(reader::make(core, proc))
        , ptr_size(core.os->proc_flags(proc) & flags_e::FLAGS_32BIT ? 4 : 8)
    {
    }

    core::Core&    core;
    proc_t         proc;
    reader::Reader reader;
    Returns        returns;
    size_t         ptr_size;
};

state::Breaker::Breaker(core::Core& core, proc_t target)
    : d_(std::make_unique<Data>(core, target))
{
}

state::Breaker::~Breaker() = default;

bool state::Breaker::break_return(std::string_view name, const state::Task& task)
{
    auto& d                = *d_;
    const auto thread      = d.core.os->thread_current();
    const auto want_rsp    = d.core.regs.read(FDP_RSP_REGISTER);
    const auto return_addr = d.reader.read(want_rsp);
    if(!thread || !return_addr)
        return false;

    struct Private
    {
        Data&    d;
        uint64_t rsp;
        Task     task;
    } ctx = {d, want_rsp, task};

    const auto bp = d.core.state.set_breakpoint(name, *return_addr, *thread, [=]
    {
        const auto rsp = ctx.d.core.regs.read(FDP_RSP_REGISTER) - ctx.d.ptr_size;
        auto it        = ctx.d.returns.find(rsp);
        if(it == ctx.d.returns.end())
            return;

        ctx.task();
        ctx.d.returns.erase(it);
    });
    d.returns.emplace(want_rsp, bp);
    return true;
}
