#include "callstacks.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "callstacks"
#include "core.hpp"
#include "core_private.hpp"
#include "interfaces/if_callstacks.hpp"

size_t callstacks::read(core::Core& core, caller_t* callers, size_t num_callers, proc_t proc)
{
    return core.callstacks_->read(callers, num_callers, proc);
}

size_t callstacks::read_from(core::Core& core, caller_t* callers, size_t num_callers, proc_t proc, const context_t& where)
{
    return core.callstacks_->read_from(callers, num_callers, proc, where);
}

bool callstacks::load_module(core::Core& core, proc_t proc, mod_t mod)
{
    if(mod.flags.is_x86)
        return true;

    const auto opt_name = modules::name(core, proc, mod);
    if(!opt_name)
        return false;

    const auto opt_span = modules::span(core, proc, mod);
    if(!opt_span)
        return false;

    return core.callstacks_->preload(proc, *opt_name, *opt_span);
}

bool callstacks::load_driver(core::Core& core, proc_t proc, driver_t drv)
{
    const auto opt_name = drivers::name(core, drv);
    if(!opt_name)
        return false;

    const auto opt_span = drivers::span(core, drv);
    if(!opt_span)
        return false;

    return core.callstacks_->preload(proc, *opt_name, *opt_span);
}

opt<callstacks::bpid_t> callstacks::autoload_modules(core::Core& core, proc_t proc)
{
    modules::list(core, proc, [&](mod_t mod)
    {
        load_module(core, proc, mod);
        return walk_e::next;
    });
    const auto ptr   = &core;
    const auto flags = process::flags(core, proc);
    const auto bpid  = modules::listen_create(core, proc, flags, [=](mod_t mod)
    {
        load_module(*ptr, proc, mod);
    });
    return bpid;
}
