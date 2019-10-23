#include "core.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "core"
#include "core_private.hpp"
#include "fdp.hpp"
#include "log.hpp"
#include "os.hpp"
#include "os_private.hpp"
#include "private.hpp"

#include <chrono>
#include <thread>

namespace
{
    using Data = core::Data;
}

Data::Data(std::string_view name)
    : name_(name)
    , shm_(nullptr)
{
}

core::Core::Core()  = default;
core::Core::~Core() = default;

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
        core.d_->shm_ = fdp::setup(name);
        if(!core.d_->shm_)
            return FAIL(false, "unable to init shm");

        fdp::reset(core);
        core.d_->mem_   = memory::setup();
        core.d_->state_ = state::setup();

        // register os helpers
        for(const auto& h : g_os_modules)
        {
            core.d_->os_ = h.make(core);
            if(!core.d_->os_)
                continue;

            const auto ok = core.d_->os_->setup();
            if(ok)
                break;

            core.d_->os_.reset();
        }
        if(core.d_->os_)
            return true;

        state::resume(core);
        return false;
    }
}

bool core::Core::setup(const std::string& name)
{
    d_ = std::make_unique<core::Data>(name);

    // try to connect multiple times
    const auto now = std::chrono::high_resolution_clock::now();
    const auto end = now + std::chrono::seconds(2);
    int n_ms       = 10;
    while(std::chrono::high_resolution_clock::now() < end)
    {
        if(::setup(*this, name))
            return true;

        std::this_thread::sleep_for(std::chrono::milliseconds(n_ms));
        n_ms = std::min(n_ms * 2, 400);
    }

    return false;
}
