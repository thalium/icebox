#pragma once

#ifndef PRIVATE_CORE__
#    error do not include this header directly
#endif

namespace fdp { struct shm; }

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

namespace core
{
    using Memory = std::shared_ptr<memory::Memory>;
    using State  = std::shared_ptr<state::State>;

    struct Data
    {
        Data(std::string_view name);

        const std::string name_;
        fdp::shm*         shm_;
        Memory            mem_;
        State             state_;
    };
} // namespace core
