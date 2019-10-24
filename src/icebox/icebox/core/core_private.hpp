#pragma once

#include "interfaces/if_callstacks.hpp"

#ifndef PRIVATE_CORE__
#    error do not include this header directly
#endif

namespace fdp
{
    struct shm;
    shm* setup(const std::string& name);
} // namespace fdp

namespace memory
{
    struct Memory;
    std::shared_ptr<Memory> setup();
} // namespace memory

namespace state
{
    struct State;
    std::shared_ptr<State> setup();
} // namespace state

namespace functions
{
    struct Data;
    std::shared_ptr<Data> setup();
} // namespace functions

namespace os { struct IModule; }

namespace core
{
    using Memory     = std::shared_ptr<memory::Memory>;
    using State      = std::shared_ptr<state::State>;
    using Os         = std::unique_ptr<os::IModule>;
    using Functions  = std::shared_ptr<functions::Data>;
    using Callstacks = std::unique_ptr<callstacks::Module>;

    struct Core
    {
        Core(std::string name);

        const std::string name_;
        fdp::shm*         shm_;
        Memory            mem_;
        State             state_;
        Functions         func_;
        Os                os_;
        Callstacks        callstacks_;
    };
} // namespace core
