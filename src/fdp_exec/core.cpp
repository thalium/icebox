#include "core.hpp"

#include "os.hpp"
#include "FDP.h"

#define FDP_MODULE "core"
#include "log.hpp"

#include <vector>

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

    struct Core
        : public ICore
    {
        Core(const std::string_view& name);

        bool setup();

        // ICore methods
        os::IHelper&            os() override;
        std::optional<uint64_t> read_msr(msr_e reg) override;
        bool                    read_mem(void* dst, addr_t src, size_t size) override;

        const std::string               name_;
        std::unique_ptr<FDP_SHM>        shm_;
        std::unique_ptr<os::IHelper>    os_;
    };
}

Core::Core(const std::string_view& name)
    : name_(name)
{
}

std::unique_ptr<ICore> make_core(const std::string_view& name)
{
    auto core = std::make_unique<Core>(name);
    const auto ok = core->setup();
    if(!ok)
        return std::nullptr_t();

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

    // register os helpers
    for(const auto& h : os::helpers)
    {
        os_ = h.make(*this);
        if(os_)
            break;
    }
    return true;
}

os::IHelper& Core::os()
{
    return *os_;
}

std::optional<uint64_t> Core::read_msr(msr_e reg)
{
    uint64_t value = 0;
    const auto ok = FDP_ReadMsr(&*shm_, CPU_ID, reg, &value);
    if(!ok)
        FAIL(std::nullopt, "unable to read msr[%d] 0x%x", CPU_ID, reg);

    return value;
}

bool Core::read_mem(void* dst, addr_t src, size_t size)
{
    const auto ok = FDP_ReadVirtualMemory(&*shm_, CPU_ID, reinterpret_cast<uint8_t*>(dst), static_cast<uint32_t>(size), src);
    if(ok)
        return true;

    LOG(ERROR, "unable to read memory 0x%llx - 0x%llx", src, src + size);
    memset(dst, 0, size);
    return false;
}

