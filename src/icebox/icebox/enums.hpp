#pragma once

#include <stdint.h>

enum class reg_e
{
    rip,
    rsp,
    rbp,
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
    // segment registers
    cs,
    // control registers
    cr3,
    cr8,
    last = cr8, // must be last
};

enum class walk_e
{
    stop,
    next,
};

enum class msr_e : uint32_t
{
    lstar,
    fs_base,
    gs_base,
    kernel_gs_base,
    last = kernel_gs_base, // must be last
};

enum constants_e : uint64_t
{
    PAGE_SIZE  = 0x1000,
    PAGE_FAULT = 0xE,
};

enum vma_access_e
{
    VMA_ACCESS_NONE          = 0,
    VMA_ACCESS_READ          = 1 << 0,
    VMA_ACCESS_WRITE         = 1 << 1,
    VMA_ACCESS_EXEC          = 1 << 2,
    VMA_ACCESS_SHARED        = 1 << 3,
    VMA_ACCESS_COPY_ON_WRITE = 1 << 4,
};

enum class vma_type_e
{
    none,
    heap,
    stack,
    module,
    other,
};
