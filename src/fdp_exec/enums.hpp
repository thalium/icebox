#pragma once

#include <stdint.h>

extern "C"
{
#include <FDP_enum.h>
}

using reg_e = FDP_Register;

enum walk_e
{
    WALK_STOP,
    WALK_NEXT,
};

enum msr_e
{
    MSR_LSTAR           = 0xC0000082,
    MSR_GS_BASE         = 0xC0000101,
    MSR_KERNEL_GS_BASE  = 0xC0000102,
};

enum constants_e
{
    PAGE_SIZE = 0x1000,
};
