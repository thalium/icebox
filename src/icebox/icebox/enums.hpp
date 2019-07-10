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
    MSR_LSTAR          = 0xC0000082,
    MSR_FS_BASE        = 0xC0000100,
    MSR_GS_BASE        = 0xC0000101,
    MSR_KERNEL_GS_BASE = 0xC0000102,
};

enum constants_e
{
    PAGE_SIZE  = 0x1000,
    PAGE_FAULT = 0xE,
};

enum flags_e
{
    FLAGS_NONE  = 0,
    FLAGS_32BIT = 1 << 0,
};

enum vma_access_e
{
    VMA_ACCESS_NONE   = 0,
    VMA_ACCESS_READ   = 1 << 0,
    VMA_ACCESS_WRITE  = 1 << 1,
    VMA_ACCESS_EXEC   = 1 << 2,
    VMA_ACCESS_SHARED = 1 << 3,
};

enum class vma_type_e
{
    none,
    main_binary,
    heap,
    stack,
    module,
    specific_os,
};
