#include "core.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "core"
#include "core_private.hpp"
#include "fdp.hpp"
#include "interfaces/if_callstacks.hpp"
#include "interfaces/if_os.hpp"
#include "interfaces/if_symbols.hpp"
#include "log.hpp"

#include <chrono>
#include <thread>

core::Core::Core(std::string name)
    : name_(std::move(name))
    , shm_(nullptr)
    , os_(nullptr)
{
}

core::Core::~Core()
{
    fdp::exit(*this);
}

namespace
{
    bool setup_nt(core::Core& core)
    {
        core.nt_ = os::make_nt(core);
        if(!core.nt_)
            return false;

        os::attach(core, *core.nt_);
        return true;
    }

    bool finalize_nt(core::Core& core)
    {
        core.callstacks_ = callstacks::make_nt(core);
        if(core.callstacks_)
            return true;

        core.nt_.reset();
        return false;
    }

    bool setup_linux(core::Core& core)
    {
        core.linux_ = os::make_linux(core);
        if(!core.linux_)
            return false;

        core.os_ = core.linux_.get();
        return true;
    }

    bool finalize_linux(core::Core& /*core*/)
    {
        return true;
    }

    struct interfaces_t
    {
        const char* name;
        bool    (*make)     (core::Core& core);
        bool    (*finalize) (core::Core& core);
    };
    const interfaces_t g_interfaces[] =
    {
            {
                "nt",
                &setup_nt,
                &finalize_nt,
            },
            {
                "linux",
                &setup_linux,
                &finalize_linux,
            },
    };

    bool try_load_os(core::Core& core)
    {
        for(const auto& h : g_interfaces)
        {
            core.os_ = nullptr;
            auto ok  = h.make(core);
            if(!ok)
                continue;

            ok = core.os_->setup();
            if(!ok)
                continue;

            ok = h.finalize(core);
            if(!ok)
                continue;

            return true;
        }
        return false;
    }

    auto setup(core::Core& core, const std::string& name)
    {
        LOG(INFO, "waiting for shm...");
        while(!core.shm_)
        {
            core.shm_ = fdp::setup(name);
            std::this_thread::yield();
        }
        LOG(INFO, "attached");

        fdp::reset(core);
        core.mem_     = memory::setup();
        core.state_   = state::setup(core);
        core.func_    = functions::setup();
        core.symbols_ = std::make_unique<symbols::Modules>(core);

        const auto loaded = try_load_os(core);
        if(loaded)
            return true;

        state::resume(core);
        return false;
    }
}

std::shared_ptr<core::Core> core::attach(const std::string& name)
{
    auto ptr = std::make_shared<core::Core>(name);
    if(!ptr)
        return {};

    if(!::setup(*ptr, name))
        return {};

    return ptr;
}
