#include "core.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "core"
#include "log.hpp"
#include "private.hpp"

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

namespace
{
    struct Core
        : public core::IHandler
    {
        Core(const std::string& name)
            : name_(name)
        {
        }

        bool setup();

        // os::IHandler
        bool                list_procs      (const on_proc_fn& on_proc) override;
        opt<proc_t>         get_current_proc() override;
        opt<proc_t>         get_proc        (const std::string& name) override;
        opt<std::string>    get_proc_name   (proc_t proc) override;
        bool                list_mods       (proc_t proc, const on_mod_fn& on_mod) override;
        opt<std::string>    get_mod_name    (proc_t proc, mod_t mod) override;
        opt<span_t>         get_mod_span    (proc_t proc, mod_t mod) override;
        bool                has_virtual     (proc_t proc) override;

        // members
        const std::string               name_;
        std::unique_ptr<FDP_SHM>        shm_;
        std::unique_ptr<os::IHandler>   os_;
    };
}

std::unique_ptr<core::IHandler> core::make_core(const std::string& name)
{
    auto core = std::make_unique<Core>(name);
    const auto ok = core->setup();
    if(!ok)
        return nullptr;

    return core;
}

bool Core::setup()
{
    auto ptr_shm = FDP_OpenSHM(name_.data());
    if(!ptr_shm)
        FAIL(false, "unable to open shm");

    shm_ = make_unique(ptr_shm);
    auto ok = FDP_Init(ptr_shm);
    if(!ok)
        FAIL(false, "unable to init shm");

    core::setup(regs, *ptr_shm);
    core::setup(mem, *ptr_shm);
    core::setup(state, *ptr_shm, *this);

    // register os helpers
    for(const auto& h : os::g_helpers)
    {
        os_ = h.make(*this);
        if(os_)
            break;
    }
    if(!os_)
        return false;

    return true;
}

bool Core::list_procs(const on_proc_fn& on_proc)
{
    return os_->list_procs(on_proc);
}

opt<proc_t> Core::get_current_proc()
{
    return os_->get_current_proc();
}

opt<proc_t> Core::get_proc(const std::string& name)
{
    return os_->get_proc(name);
}

opt<std::string> Core::get_proc_name(proc_t proc)
{
    return os_->get_proc_name(proc);
}

bool Core::list_mods(proc_t proc, const on_mod_fn& on_mod)
{
    return os_->list_mods(proc, on_mod);
}

opt<std::string> Core::get_mod_name(proc_t proc, mod_t mod)
{
    return os_->get_mod_name(proc, mod);
}

opt<span_t> Core::get_mod_span(proc_t proc, mod_t mod)
{
    return os_->get_mod_span(proc, mod);
}

bool Core::has_virtual(proc_t proc)
{
    return os_->has_virtual(proc);
}
