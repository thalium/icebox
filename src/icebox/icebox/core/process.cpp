#include "process.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "process"
#include "core.hpp"
#include "core_private.hpp"
#include "os_private.hpp"

bool process::list(core::Core& core, process::on_proc_fn on_proc)
{
    return core.os_->proc_list(on_proc);
}

opt<proc_t> process::current(core::Core& core)
{
    return core.os_->proc_current();
}

opt<proc_t> process::find_name(core::Core& core, std::string_view name, flags_e flags)
{
    return core.os_->proc_find(name, flags);
}

opt<proc_t> process::find_pid(core::Core& core, uint64_t pid)
{
    return core.os_->proc_find(pid);
}

opt<std::string> process::name(core::Core& core, proc_t proc)
{
    return core.os_->proc_name(proc);
}

bool process::is_valid(core::Core& core, proc_t proc)
{
    return core.os_->proc_is_valid(proc);
}

uint64_t process::pid(core::Core& core, proc_t proc)
{
    return core.os_->proc_id(proc);
}

flags_e process::flags(core::Core& core, proc_t proc)
{
    return core.os_->proc_flags(proc);
}

void process::join(core::Core& core, proc_t proc, join_e join)
{
    return core.os_->proc_join(proc, join);
}

opt<phy_t> process::resolve(core::Core& core, proc_t proc, uint64_t ptr)
{
    return core.os_->proc_resolve(proc, ptr);
}

opt<proc_t> process::select(core::Core& core, proc_t proc, uint64_t ptr)
{
    return core.os_->proc_select(proc, ptr);
}

opt<proc_t> process::parent(core::Core& core, proc_t proc)
{
    return core.os_->proc_parent(proc);
}

opt<os::bpid_t> process::listen_create(core::Core& core, const on_event_fn& on_proc_event)
{
    return core.os_->listen_proc_create(on_proc_event);
}

opt<os::bpid_t> process::listen_delete(core::Core& core, const on_event_fn& on_proc_event)
{
    return core.os_->listen_proc_delete(on_proc_event);
}
