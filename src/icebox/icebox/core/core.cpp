#include "core.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "core"
#include "fdp.hpp"
#include "log.hpp"
#include "os.hpp"
#include "private.hpp"

#include <chrono>
#include <thread>

namespace
{
    using Data = core::Core::Data;
}

struct core::Core::Data
{
    Data(std::string_view name)
        : name_(name)
    {
    }

    // members
    const std::string name_;
    fdp::shm*         shm_;
};

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

    static auto setup(core::Core& core, Data& d, std::string_view name)
    {
        auto ptr_shm = fdp::open(name.data());
        if(!ptr_shm)
            return FAIL(false, "unable to open shm");

        d.shm_  = ptr_shm;
        auto ok = fdp::init(*ptr_shm);
        if(!ok)
            return FAIL(false, "unable to init shm");

        fdp::reset(*ptr_shm);

        core::setup(core.regs, *ptr_shm);
        core::setup(core.mem, *ptr_shm, core);
        core::setup(core.state, *ptr_shm, core);

        // register os helpers
        for(const auto& h : g_os_modules)
        {
            core.os = h.make(core);
            if(!core.os)
                continue;

            ok = core.os->setup();
            if(ok)
                break;

            core.os.reset();
        }
        if(core.os)
            return true;

        core.state.resume();
        return false;
    }
}

bool core::Core::setup(std::string_view name)
{
    d_ = std::make_unique<core::Core::Data>(name);

    // try to connect multiple times
    const auto now = std::chrono::high_resolution_clock::now();
    const auto end = now + std::chrono::seconds(2);
    int n_ms       = 10;
    while(std::chrono::high_resolution_clock::now() < end)
    {
        if(::setup(*this, *d_, name))
            return true;

        std::this_thread::sleep_for(std::chrono::milliseconds(n_ms));
        n_ms = std::min(n_ms * 2, 400);
    }

    return false;
}
