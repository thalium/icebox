#pragma once

#ifndef PRIVATE_CORE__
#    error do not include this header directly
#endif

namespace fdp
{
    struct shm;
    std::shared_ptr<shm> setup(const std::string& name);
} // namespace fdp

namespace memory
{
    struct Memory;
    std::shared_ptr<Memory> setup();
} // namespace memory

namespace state
{
    struct State;
    std::shared_ptr<State> setup(core::Core& core);
} // namespace state

namespace functions
{
    struct Data;
    std::shared_ptr<Data> setup();
} // namespace functions

namespace os { struct Module; }
namespace callstacks { struct Module; }
namespace symbols { struct Modules; }
namespace nt { struct Os; }

namespace core
{
    using Shm        = std::shared_ptr<fdp::shm>;
    using Memory     = std::shared_ptr<memory::Memory>;
    using State      = std::shared_ptr<state::State>;
    using Functions  = std::shared_ptr<functions::Data>;
    using Callstacks = std::unique_ptr<callstacks::Module>;
    using Symbols    = std::unique_ptr<symbols::Modules>;
    using Nt         = std::shared_ptr<nt::Os>;
    using Linux      = std::unique_ptr<os::Module>;

    struct Core
    {
         Core(std::string name);
        ~Core();

        const std::string name_;
        Shm               shm_;
        Memory            mem_;
        State             state_;
        Functions         func_;
        Nt                nt_;
        Linux             linux_;
        os::Module*       os_;
        Callstacks        callstacks_;
        Symbols           symbols_;
    };
} // namespace core
