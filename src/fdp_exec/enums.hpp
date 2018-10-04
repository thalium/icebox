#pragma once

enum walk_e
{
    WALK_STOP,
    WALK_NEXT,
};

enum msr_e
{
    MSR_LSTAR = 0xC0000082,
};

enum constants_e
{
    PAGE_SIZE = 0x1000,
};
