#pragma once

#ifndef PRIVATE_CORE__
#error "do not include this private header"
#endif

#include "types.hpp"

#include <memory>

typedef struct FDP_SHM_ FDP_SHM;
namespace core { struct Registers; }
namespace core { struct Memory; }
namespace core { struct IState; }
namespace core { struct IHandler; }

namespace core
{
    struct BreakState
    {
        proc_t proc;
    };

    void setup(Registers& regs, FDP_SHM& shm);
    void setup(Memory& mem, FDP_SHM& shm);
    std::unique_ptr<IState>     make_state(FDP_SHM& shm, IHandler& mem);
}
