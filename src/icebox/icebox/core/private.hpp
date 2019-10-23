#pragma once

#ifndef PRIVATE_CORE__
#    error "do not include this private header"
#endif

#include "types.hpp"

namespace fdp { struct shm; }
namespace core { struct Registers; }
namespace core { struct State; }
namespace core { struct Core; }

namespace core
{
    void setup(State& mem, fdp::shm& shm, Core& core);
} // namespace core
