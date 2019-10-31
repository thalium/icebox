#pragma once

#include <stdint.h>

enum class reg_e
{
    cs,
    rsp,
    rip,
    rbp,
    cr3,
    rax,
    rbx,
    rcx,
    rdx,
    rsi,
    rdi,
    r8,
    r9,
    r10,
    r11,
    r12,
    r13,
    r14,
    r15,
};

enum class walk_e
{
    stop,
    next,
};

enum class msr_e : uint32_t
{
    lstar          = 0xC0000082,
    fs_base        = 0xC0000100,
    gs_base        = 0xC0000101,
    kernel_gs_base = 0xC0000102,
};

enum class mode_e
{
    kernel,
    user,
};

enum constants_e
{
    PAGE_SIZE  = 0x1000,
    PAGE_FAULT = 0xE,
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
    binary,
    heap,
    stack,
    module,
    other,
};
