#include "core.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "core"
#include "core_private.hpp"
#include "fdp.hpp"
#include "log.hpp"
#include "os_private.hpp"

#include <chrono>
#include <thread>

core::Core::Core(const std::string& name)
    : name_(name)
    , shm_(nullptr)
{
}

namespace
{
    static const struct
    {
        std::unique_ptr<os::IModule> (*make)(core::Core& core);
        const char name[32];
    } g_os_modules[] =
    {
            {&os::make_nt, "windows_nt"},
            {&os::make_linux, "linux_nt"},
    };

    static auto setup(core::Core& core, const std::string& name)
    {
        core.shm_ = fdp::setup(name);
        if(!core.shm_)
            return FAIL(false, "unable to init shm");

        fdp::reset(core);
        core.mem_   = memory::setup();
        core.state_ = state::setup();

        // register os helpers
        for(const auto& h : g_os_modules)
        {
            core.os_ = h.make(core);
            if(!core.os_)
                continue;

            const auto ok = core.os_->setup();
            if(ok)
                break;

            core.os_.reset();
        }
        if(core.os_)
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

    // try to connect multiple times
    const auto now = std::chrono::high_resolution_clock::now();
    const auto end = now + std::chrono::seconds(2);
    int n_ms       = 10;
    while(std::chrono::high_resolution_clock::now() < end)
    {
        if(::setup(*ptr, name))
            return ptr;

        std::this_thread::sleep_for(std::chrono::milliseconds(n_ms));
        n_ms = std::min(n_ms * 2, 400);
    }

    return {};
}
