#include "waiter.hpp"

#define FDP_MODULE "waiter"
#include "log.hpp"
#include "os.hpp"
#include "utils/fnview.hpp"
#include "utils/path.hpp"

struct core::Waiter::Data
{
    Data(Core& core);
    Core& core;
};

core::Waiter::Data::Data(core::Core& core)
    : core(core)
{
}

using WaiterData = core::Waiter::Data;

core::Waiter::Waiter(core::Core& core)
    : d_(std::make_unique<Data>(core))
{
}

core::Waiter::~Waiter() = default;

namespace
{
    bool search_mod(WaiterData& d, proc_t proc, const std::string& mod_name, const core::Waiter::task_mod& task)
    {
        opt<mod_t> found = {};
        d.core.os->mod_list(proc, [&](mod_t mod)
        {
            const auto name = d.core.os->mod_name(proc, mod);
            if(!name)
                return WALK_NEXT;

            if(_stricmp(path::filename(*name).generic_string().data(), mod_name.data()))
                return WALK_NEXT;

            found = mod;
            return WALK_STOP;
        });

        if(!found)
            return false;

        const auto name = d.core.os->mod_name(proc, *found);
        if(!name)
            return false;

        const auto span = d.core.os->mod_span(proc, *found);
        if(!span)
            return false;

        task(proc, *name, *span);
        return true;
    }
}

void core::Waiter::proc_wait(const std::string& proc_name, const core::Waiter::task_proc& task)
{
    const auto proc_f = d_->core.os->proc_find(proc_name);
    if(proc_f)
        return task(*proc_f);

    d_->core.os->proc_listen_create([=](proc_t /*parent_proc*/, proc_t proc)
    {
        const auto name = d_->core.os->proc_name(proc);
        if(!name)
            return;

        if(name == proc_name)
            task(proc);
    });
}

void core::Waiter::mod_wait(const std::string& mod_name, const core::Waiter::task_mod& task)
{
    auto ok = false;
    d_->core.os->proc_list([&](proc_t proc)
    {
        ok = search_mod(*d_, proc, mod_name, task);
        if(ok)
            return WALK_STOP;

        return WALK_NEXT;
    });

    if(ok)
        return;

    d_->core.os->mod_listen_load([=](proc_t proc_loading, const std::string& name, span_t span)
    {
        if(_stricmp(path::filename(name).generic_string().data(), mod_name.data()))
            return;

        task(proc_loading, name, span);
    });
}

void core::Waiter::mod_wait(proc_t proc, const std::string& mod_name, const core::Waiter::task_mod& task)
{
    if(search_mod(*d_, proc, mod_name, task))
        return;

    d_->core.os->mod_listen_load([=](proc_t proc_loading, const std::string& name, span_t span)
    {
        if(proc_loading.id != proc.id)
            return;

        if(_stricmp(path::filename(name).generic_string().data(), mod_name.data()))
            return;

        task(proc, name, span);
    });
}

void core::Waiter::mod_wait(const std::string& proc_name, const std::string& mod_name, const core::Waiter::task_mod& task)
{
    proc_wait(proc_name, [=](proc_t proc)
    {
        mod_wait(proc, mod_name, task);
    });
}
