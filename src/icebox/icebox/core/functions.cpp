#include "functions.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "function"
#include "core.hpp"
#include "core_private.hpp"
#include "interfaces/if_os.hpp"

#include <unordered_map>

namespace
{
    using Returns = std::unordered_map<uint64_t, state::Breakpoint>;
}

struct functions::Data
{
    Returns returns;
};

std::shared_ptr<functions::Data> functions::setup()
{
    return std::make_shared<functions::Data>();
}

bool functions::break_on_return(core::Core& core, std::string_view name, const functions::on_return_fn& on_return)
{
    const auto thread = threads::current(core);
    if(!thread)
        return false;

    const auto proc = process::current(core);
    if(!proc)
        return false;

    const auto ptr_size    = process::flags(core, *proc).is_x86 ? 4 : 8;
    const auto reader      = reader::make(core, *proc);
    const auto want_rsp    = registers::read(core, reg_e::rsp);
    const auto return_addr = reader.read(want_rsp);
    if(!return_addr)
        return false;

    struct Private
    {
        core::Core& core;
    } ctx = {core};

    const auto bp = state::break_on_thread(core, name, *thread, *return_addr, [=]
    {
        auto& d        = *ctx.core.func_;
        const auto rsp = registers::read(ctx.core, reg_e::rsp) - ptr_size;
        auto it        = d.returns.find(rsp);
        if(it == d.returns.end())
            return;

        on_return();
        d.returns.erase(it);
    });
    core.func_->returns.emplace(want_rsp, bp);
    return true;
}

opt<arg_t> functions::read_stack(core::Core& core, size_t index)
{
    return core.os_->read_stack(index);
}

opt<arg_t> functions::read_arg(core::Core& core, size_t index)
{
    return core.os_->read_arg(index);
}

bool functions::write_arg(core::Core& core, size_t index, arg_t arg)
{
    return core.os_->write_arg(index, arg);
}
