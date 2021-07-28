#include "process.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "process"
#include "core.hpp"
#include "core_private.hpp"
#include "interfaces/if_os.hpp"
#include "log.hpp"

bool process::list(core::Core& core, process::on_proc_fn on_proc)
{
    return core.os_->proc_list(std::move(on_proc));
}

opt<proc_t> process::current(core::Core& core)
{
    return core.os_->proc_current();
}

opt<proc_t> process::find_name(core::Core& core, std::string_view name, flags_t flags)
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

flags_t process::flags(core::Core& core, proc_t proc)
{
    return core.os_->proc_flags(proc);
}

opt<proc_t> process::parent(core::Core& core, proc_t proc)
{
    return core.os_->proc_parent(proc);
}

opt<bpid_t> process::listen_create(core::Core& core, const on_event_fn& on_proc_event)
{
    return core.os_->listen_proc_create(on_proc_event);
}

opt<bpid_t> process::listen_delete(core::Core& core, const on_event_fn& on_proc_event)
{
    return core.os_->listen_proc_delete(on_proc_event);
}

opt<proc_t> process::wait(core::Core& core, std::string_view proc_name, flags_t flags)
{
    const auto proc = process::find_name(core, proc_name, flags);
    if(proc)
        return *proc;

    auto found      = opt<proc_t>{};
    const auto bpid = process::listen_create(core, [&](proc_t proc)
    {
        const auto new_flags = process::flags(core, proc);
        if(!os::check_flags(new_flags, flags))
            return;

        const auto name = process::name(core, proc);
        if(!name)
            return;

        LOG(INFO, "proc started: %s", name->data());
        if(*name == proc_name)
            found = proc;
    });
    if(!bpid)
        return {};

    while(!found)
        state::exec(core);

    state::drop_breakpoint(core, *bpid);
    return found;
}
