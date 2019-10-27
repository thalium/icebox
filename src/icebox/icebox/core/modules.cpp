#include "modules.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "modules"
#include "core.hpp"
#include "core_private.hpp"
#include "interfaces/if_os.hpp"
#include "utils/path.hpp"

#ifdef _MSC_VER
#    define stricmp _stricmp
#else
#    include <strings.h>
#    define stricmp strcasecmp
#endif

bool modules::list(core::Core& core, proc_t proc, modules::on_mod_fn on_mod)
{
    return core.os_->mod_list(proc, std::move(on_mod));
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

namespace
{
    static opt<mod_t> search_mod(core::Core& core, proc_t proc, std::string_view mod_name, flags_e flags)
    {
        opt<mod_t> found;
        modules::list(core, proc, [&](mod_t mod)
        {
            const auto name = modules::name(core, proc, mod);
            if(!name)
                return walk_e::next;

            if(flags && !(mod.flags & flags))
                return walk_e::next;

            if(stricmp(path::filename(*name).generic_string().data(), mod_name.data()))
                return walk_e::next;

            found = mod;
            return walk_e::stop;
        });

        return found;
    }
}

opt<mod_t> modules::wait(core::Core& core, proc_t proc, std::string_view mod_name, flags_e flags)
{
    const auto mod = search_mod(core, proc, mod_name, flags);
    if(mod)
    {
        process::join(core, proc, process::JOIN_USER_MODE);
        return *mod;
    }

    opt<mod_t> found;
    const auto bpid = modules::listen_create(core, [&](proc_t proc_loading, mod_t mod)
    {
        if(proc_loading.id != proc.id)
            return;

        if(flags && !(mod.flags & flags))
            return;

        const auto name = modules::name(core, proc_loading, mod);
        if(!name)
            return;

        if(stricmp(path::filename(*name).generic_string().data(), mod_name.data()))
            return;

        found = mod;
    });
    if(!bpid)
        return {};

    while(!found)
    {
        state::resume(core);
        state::wait(core);
    }
    os::unlisten(core, *bpid);
    return found;
}
