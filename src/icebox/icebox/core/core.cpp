#include "core.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "core"
#include "fdp.hpp"
#include "log.hpp"
#include "os.hpp"
#include "private.hpp"

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
}

bool core::Core::setup(std::string_view name)
{
    d_           = std::make_unique<core::Core::Data>(name);
    auto ptr_shm = fdp::open(name.data());
    if(!ptr_shm)
        return FAIL(false, "unable to open shm");

    d_->shm_ = ptr_shm;
    auto ok  = fdp::init(*ptr_shm);
    if(!ok)
        return FAIL(false, "unable to init shm");

    fdp::reset(*ptr_shm);

    core::setup(regs, *ptr_shm);
    core::setup(mem, *ptr_shm, *this);
    core::setup(state, *ptr_shm, *this);

    // register os helpers
    for(const auto& h : g_os_modules)
    {
        os = h.make(*this);
        if(!os)
            continue;

        ok = os->setup();
        if(ok)
            break;

        os.reset();
    }
    if(!os)
        return false;

    return true;
}
