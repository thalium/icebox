#include "core.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "core"
#include "log.hpp"
#include "private.hpp"
#include "os.hpp"

#include <FDP.h>

// custom deleters
namespace std
{
    template<> struct default_delete<FDP_SHM> { static const bool marker = true; void operator()(FDP_SHM* /*ptr*/) {} }; // FIXME no deleter
}

namespace
{
    static const int CPU_ID = 0;

    template<typename T>
    std::unique_ptr<T> make_unique(T* ptr)
    {
        // check whether we correctly defined a custom deleter
        static_assert(std::default_delete<T>::marker == true, "missing custom marker");
        return std::unique_ptr<T>(ptr);
    }
}

struct core::Core::Data
{
    Data(const std::string& name)
        : name_(name)
    {
    }

    // members
    const std::string           name_;
    std::unique_ptr<FDP_SHM>    shm_;
};

core::Core::Core()
{
}

core::Core::~Core()
{
}

namespace
{
    static const struct
    {
        std::unique_ptr<os::IModule>(*make)(core::Core& core);
        const char name[32];
    } g_os_modules[] =
    {
        {&os::make_nt, "windows_nt"},
    };
}

bool core::setup(Core& core, const std::string& name)
{
    core.d_ = std::make_unique<core::Core::Data>(name);
    auto ptr_shm = FDP_OpenSHM(name.data());
    if(!ptr_shm)
        FAIL(false, "unable to open shm");

    core.d_->shm_ = make_unique(ptr_shm);
    auto ok = FDP_Init(ptr_shm);
    if(!ok)
        FAIL(false, "unable to init shm");

    for(int i = 0; i < FDP_MAX_BREAKPOINT; ++i)
        FDP_UnsetBreakpoint(ptr_shm, i);

    core::setup(core.regs, *ptr_shm);
    core::setup(core.mem, *ptr_shm);
    core::setup(core.state, *ptr_shm, core);

    // register os helpers
    for(const auto& h : g_os_modules)
    {
        core.os = h.make(core);
        if(core.os)
            break;
    }
    if(!core.os)
        return false;

    return true;
}
