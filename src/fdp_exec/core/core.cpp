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

        // core::IRegisters
        opt<uint64_t>   read_reg    (reg_e reg) override;
        bool            write_reg   (reg_e reg, uint64_t value) override;
        opt<uint64_t>   read_msr    (msr_e reg) override;
        bool            write_msr   (msr_e reg, uint64_t value) override;

        // core::IMemory
        void                    update              (const core::BreakState& state) override;
        opt<uint64_t>           virtual_to_physical (uint64_t ptr, uint64_t dtb) override;
        core::ProcessContext    switch_process      (proc_t proc) override;
        bool                    read                (void* dst, uint64_t src, size_t size) override;

        // core::IState
        bool                pause           () override;
        bool                resume          () override;
        bool                wait            () override;
        core::Breakpoint    set_breakpoint  (uint64_t ptr, proc_t proc, core::filter_e filter, const core::Task& task) override;

        // sym::IHandler
        bool            register_module     (const std::string& name, std::unique_ptr<sym::IModule>& module) override;
        bool            unregister_module   (const std::string& name) override;
        bool            list_modules        (const on_module_fn& on_module) override;
        sym::IModule*   get_module          (const std::string& name) override;
        opt<uint64_t>   get_symbol          (const std::string& module, const std::string& symbol) override;
        opt<uint64_t>   get_struc_offset    (const std::string& module, const std::string& struc, const std::string& member) override;

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
        const std::string                   name_;
        std::unique_ptr<FDP_SHM>            shm_;
        std::unique_ptr<core::IRegisters>   regs_;
        std::unique_ptr<core::IMemory>      mem_;
        std::unique_ptr<core::IState>       state_;
        std::unique_ptr<os::IHandler>       os_;
        std::unique_ptr<sym::IHandler>      sym_;
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

    regs_ = core::make_registers(*ptr_shm);
    if(!regs_)
        FAIL(false, "unable to init register module");

    mem_ = core::make_memory(*ptr_shm);
    if(!mem_)
        FAIL(false, "unable to init memory module");

    state_ = core::make_state(*ptr_shm, *this);
    if(!state_)
        FAIL(false, "unable to init state module");

    sym_ = sym::make_sym();
    if(!sym_)
        FAIL(false, "unable to init sym module");

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

opt<uint64_t> Core::read_reg(reg_e reg)
{
    return regs_->read_reg(reg);
}

bool Core::write_reg(reg_e reg, uint64_t value)
{
    return regs_->write_reg(reg, value);
}

opt<uint64_t> Core::read_msr(msr_e reg)
{
    return regs_->read_msr(reg);
}

bool Core::write_msr(msr_e reg, uint64_t value)
{
    return regs_->write_msr(reg, value);
}

void Core::update(const core::BreakState& state)
{
    mem_->update(state);
}

opt<uint64_t> Core::virtual_to_physical(uint64_t ptr, uint64_t dtb)
{
    return mem_->virtual_to_physical(ptr, dtb);
}

core::ProcessContext Core::switch_process(proc_t proc)
{
    return mem_->switch_process(proc);
}

bool Core::read(void* dst, uint64_t src, size_t size)
{
    return mem_->read(dst, src, size);
}

bool Core::pause()
{
    return state_->pause();
}

bool Core::resume()
{
    return state_->resume();
}

bool Core::wait()
{
    return state_->wait();
}

core::Breakpoint Core::set_breakpoint(uint64_t ptr, proc_t proc, core::filter_e filter, const core::Task& task)
{
    return state_->set_breakpoint(ptr, proc, filter, task);
}

bool Core::register_module(const std::string& name, std::unique_ptr<sym::IModule>& module)
{
    return sym_->register_module(name, module);
}

bool Core::unregister_module(const std::string& name)
{
    return sym_->unregister_module(name);
}

bool Core::list_modules(const on_module_fn& on_module)
{
    return sym_->list_modules(on_module);
}

sym::IModule* Core::get_module(const std::string& name)
{
    return sym_->get_module(name);
}

opt<uint64_t> Core::get_symbol(const std::string& module, const std::string& symbol)
{
    return sym_->get_symbol(module, symbol);
}

opt<uint64_t> Core::get_struc_offset(const std::string& module, const std::string& struc, const std::string& member)
{
    return sym_->get_struc_offset(module, struc, member);
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
