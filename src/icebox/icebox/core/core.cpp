#include "core.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "core"
#include "log.hpp"
#include "os.hpp"
#include "private.hpp"

extern "C"
{
#include <FDP.h>
}

// custom deleters
namespace std
{
    template <>
    struct default_delete<FDP_SHM>
    {
        static const bool marker = true;
        void operator()(FDP_SHM* /*ptr*/) {}
    }; // FIXME no deleter
} // namespace std

namespace
{
    template <typename T>
    static std::unique_ptr<T> make_unique(T* ptr)
    {
        // check whether we correctly defined a custom deleter
        static_assert(std::default_delete<T>::marker == true, "missing custom marker");
        return std::unique_ptr<T>(ptr);
    }
}

struct core::Core::Data
{
    Data(std::string_view name)
        : name_(name)
    {
    }

    // members
    const std::string        name_;
    std::unique_ptr<FDP_SHM> shm_;
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
    auto ptr_shm = FDP_OpenSHM(name.data());
    if(!ptr_shm)
        return FAIL(false, "unable to open shm");

    d_->shm_ = make_unique(ptr_shm);
    auto ok  = FDP_Init(ptr_shm);
    if(!ok)
        return FAIL(false, "unable to init shm");

    FDP_State status;
    memset(&status, 0, sizeof status);
    ok = FDP_GetState(ptr_shm, &status);
    if(!ok)
        return FAIL(false, "unable to get initial fdp state");

    if(!(status & FDP_STATE_PAUSED))
    {
        ok = FDP_Pause(ptr_shm);
        if(!ok)
            return FAIL(false, "unable to pause fdp");
    }

    for(int i = 0; i < FDP_MAX_BREAKPOINT; ++i)
        FDP_UnsetBreakpoint(ptr_shm, i);

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
