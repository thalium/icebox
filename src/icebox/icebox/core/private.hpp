#pragma once

#ifndef PRIVATE_CORE__
#    error "do not include this private header"
#endif

#include "types.hpp"

namespace fdp { struct shm; }
namespace core { struct Registers; }
namespace core { struct Memory; }
namespace core { struct State; }
namespace core { struct Core; }

namespace core
{
    void    setup   (Memory& mem, fdp::shm& shm, Core& core);
    void    setup   (State& mem, fdp::shm& shm, Core& core);
} // namespace core
