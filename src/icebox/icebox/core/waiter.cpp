#include "waiter.hpp"

#define FDP_MODULE "waiter"
#include "core.hpp"
#include "log.hpp"
#include "utils/fnview.hpp"
#include "utils/path.hpp"

#ifdef _MSC_VER
#    define stricmp _stricmp
#else
#    include <strings.h>
#    define stricmp strcasecmp
#endif

namespace
{
    static opt<mod_t> search_mod(core::Core& core, proc_t proc, std::string_view mod_name, flags_e flags)
    {
        opt<mod_t> found;
        modules::list(core, proc, [&](mod_t mod)
        {
            const auto name = modules::name(core, proc, mod);
            if(!name)
                return WALK_NEXT;

            if(flags && !(mod.flags & flags))
                return WALK_NEXT;

            if(stricmp(path::filename(*name).generic_string().data(), mod_name.data()))
                return WALK_NEXT;

            found = mod;
            return WALK_STOP;
        });

        return found;
    }
}

opt<proc_t> waiter::proc_wait(core::Core& core, std::string_view proc_name, flags_e flags)
{
    const auto proc = process::find_name(core, proc_name, flags);
    if(proc)
    {
        process::join(core, *proc, process::JOIN_ANY_MODE);
        return *proc;
    }

    opt<proc_t> found;
    const auto bpid = process::listen_create(core, [&](proc_t proc)
    {
        const auto new_flags = process::flags(core, proc);
        if(flags && !(new_flags & flags))
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
    {
        state::resume(core);
        state::wait(core);
    }
    os::unlisten(core, *bpid);
    return found;
}

opt<mod_t> waiter::mod_wait(core::Core& core, proc_t proc, std::string_view mod_name, flags_e flags)
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
