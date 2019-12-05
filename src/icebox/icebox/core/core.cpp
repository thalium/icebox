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
{
}

namespace
{
    struct interfaces_t
    {
        const char* name;
        std::unique_ptr<os::Module>         (*make)             (core::Core& core);
        std::unique_ptr<callstacks::Module> (*make_callstacks)  (core::Core& core);
    };
    const interfaces_t g_interfaces[] =
    {
            {
                "nt",
                &os::make_nt,
                &callstacks::make_nt,
            },
            {
                "linux",
                &os::make_linux,
                nullptr,
            },
    };

    auto setup(core::Core& core, const std::string& name)
    {
        core.shm_ = fdp::setup(name);
        if(!core.shm_)
            return FAIL(false, "unable to init shm");

        fdp::reset(core);
        core.mem_     = memory::setup();
        core.state_   = state::setup(core);
        core.func_    = functions::setup();
        core.symbols_ = std::make_unique<symbols::Modules>(core);

        // register os helpers
        auto interfaces = static_cast<const interfaces_t*>(nullptr);
        core.os_.reset();
        for(const auto& h : g_interfaces)
        {
            core.os_ = h.make(core);
            if(!core.os_)
                continue;

            const auto ok = core.os_->setup();
            interfaces    = &h;
            if(ok)
                break;

            core.os_.reset();
        }
        if(!core.os_)
        {
            state::resume(core);
            return false;
        }

        if(interfaces->make_callstacks)
            core.callstacks_ = interfaces->make_callstacks(core);

        return true;
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
