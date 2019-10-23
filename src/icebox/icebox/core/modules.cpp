#include "modules.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "modules"
#include "core.hpp"
#include "core_private.hpp"
#include "os_private.hpp"

bool modules::list(core::Core& core, proc_t proc, modules::on_mod_fn on_mod)
{
    return core.os_->mod_list(proc, on_mod);
}

opt<std::string> modules::name(core::Core& core, proc_t proc, mod_t mod)
{
    return core.os_->mod_name(proc, mod);
}

opt<span_t> modules::span(core::Core& core, proc_t proc, mod_t mod)
{
    return core.os_->mod_span(proc, mod);
}

opt<mod_t> modules::find(core::Core& core, proc_t proc, uint64_t addr)
{
    return core.os_->mod_find(proc, addr);
}

opt<os::bpid_t> modules::listen_create(core::Core& core, const on_event_fn& on_load)
{
    return core.os_->listen_mod_create(on_load);
}
