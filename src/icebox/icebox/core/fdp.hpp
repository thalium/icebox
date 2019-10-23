#pragma once

#include "types.hpp"

extern "C"
{
#include <FDP_enum.h>
}

namespace core { struct Core; }

namespace fdp
{
    void            reset               (core::Core& core);
    opt<FDP_State>  state               (core::Core& core);
    bool            state_changed       (core::Core& core);
    bool            pause               (core::Core& core);
    bool            resume              (core::Core& core);
    bool            step_once           (core::Core& core);
    bool            unset_breakpoint    (core::Core& core, int bpid);
    int             set_breakpoint      (core::Core& core, FDP_BreakpointType type, int bpid, FDP_Access access, FDP_AddressType ptrtype, uint64_t ptr, uint64_t len, uint64_t cr3);
    bool            read_physical       (core::Core& core, void* dst, size_t size, phy_t phy);
    bool            read_virtual        (core::Core& core, void* dst, size_t size, dtb_t dtb, uint64_t ptr);
    opt<phy_t>      virtual_to_physical (core::Core& core, dtb_t dtb, uint64_t ptr);
    bool            inject_interrupt    (core::Core& core, uint32_t code, uint32_t error, uint64_t cr2);
    opt<uint64_t>   read_register       (core::Core& core, reg_e reg);
    opt<uint64_t>   read_msr_register   (core::Core& core, msr_e msr);
    bool            write_register      (core::Core& core, reg_e reg, uint64_t value);
    bool            write_msr_register  (core::Core& core, msr_e msr, uint64_t value);
} // namespace fdp
