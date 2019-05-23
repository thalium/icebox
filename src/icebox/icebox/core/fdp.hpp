#pragma once

#include "types.hpp"

extern "C"
{
#include <FDP_enum.h>
}

namespace fdp
{
    struct shm;

    shm*    open(const char* name);
    bool    init(shm& shm);

    void            unset_all_breakpoints   (shm& shm);
    opt<FDP_State>  state                   (shm& shm);
    bool            state_changed           (shm& shm);
    bool            pause                   (shm& shm);
    bool            resume                  (shm& shm);
    bool            step_once               (shm& shm);
    bool            unset_breakpoint        (shm& shm, int bpid);
    int             set_breakpoint          (shm& shm, FDP_BreakpointType type, int bpid, FDP_Access access, FDP_AddressType ptrtype, uint64_t ptr, uint64_t len, uint64_t cr3);
    bool            read_physical           (shm& shm, void* dst, size_t size, phy_t phy);
    bool            read_virtual            (shm& shm, void* dst, size_t size, uint64_t ptr);
    opt<phy_t>      virtual_to_physical     (shm& shm, uint64_t ptr);
    bool            inject_interrupt        (shm& shm, uint32_t code, uint32_t error, uint64_t cr2);
    opt<uint64_t>   read_register           (shm& shm, reg_e reg);
    opt<uint64_t>   read_msr_register       (shm& shm, msr_e msr);
    bool            write_register          (shm& shm, reg_e reg, uint64_t value);
    bool            write_msr_register      (shm& shm, msr_e msr, uint64_t value);
} // namespace fdp
