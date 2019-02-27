#include "waiter.hpp"

#define FDP_MODULE "waiter"
#include "log.hpp"
#include "os.hpp"
#include "utils/fnview.hpp"
#include "utils/path.hpp"

#ifdef _MSC_VER
#    define stricmp _stricmp
#else
#    include <cstring>
#    define stricmp strcasecmp
#endif

namespace
{
    static opt<mod_t> search_mod(core::Core& core, proc_t proc, std::string_view mod_name, flags_e flags)
    {
        opt<mod_t> found = {};
        core.os->mod_list(proc, [&](mod_t mod)
        {
            const auto name = core.os->mod_name(proc, mod);
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
    auto found = core.os->proc_find(proc_name, flags);
    if(found)
    {
        core.os->proc_join(*found, os::JOIN_ANY_MODE);
        return found;
    }

    core.os->proc_listen_create([&](proc_t /*parent_proc*/, proc_t proc)
    {
        const auto new_flags = core.os->proc_flags(proc);
        if(flags && !(new_flags & flags))
            return;

        const auto name = core.os->proc_name(proc);
        if(!name)
            return;

        LOG(INFO, "New proc {}", name->data());

        if(*name == proc_name)
            found = proc;
    });

    while(!found)
    {
        core.state.resume();
        core.state.wait();
    }

    core.os->proc_join(*found, os::JOIN_ANY_MODE);
    return found;
}

opt<mod_t> waiter::mod_wait(core::Core& core, proc_t proc, std::string_view mod_name, opt<span_t>& mod_span, flags_e flags)
{
    auto found = search_mod(core, proc, mod_name, flags);
    if(found)
    {
        mod_span = core.os->mod_span(proc, *found);
        return found;
    }

    core.os->mod_listen_load([&](proc_t proc_loading, const std::string& name, span_t span)
    {
        if(proc_loading.id != proc.id)
            return;

        if(flags & FLAGS_32BIT)
            if(span.addr > 0x100000000)
                return;

        if(stricmp(path::filename(name).generic_string().data(), mod_name.data()))
            return;

        mod_span = span;
        found    = {};
    });

    while(!mod_span)
    {
        core.state.resume();
        core.state.wait();
    }
    return found;
}
