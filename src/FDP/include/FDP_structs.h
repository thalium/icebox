/*
    MIT License

    Copyright (c) 2015 Nicolas Couffin ncouffin@gmail.com

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
#ifndef FDP_STRUCTS_H__
#define FDP_STRUCTS_H__

#ifdef _MSC_VER
#    define ALIGNED_(X) _declspec(align(X))
#else
#    define ALIGNED_(X) __attribute__((aligned(X)))
#endif

#include "FDP.h"

#pragma pack(push, 1)
typedef struct FDP_CPU_CTX_
{
    uint64_t rip;

    uint64_t rax;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rbx;

    uint64_t rsp;
    uint64_t rbp;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

    uint64_t es;
    uint64_t cs;
    uint64_t ss;
    uint64_t ds;
    uint64_t fs;
    uint64_t gs;

    uint64_t rflags;

    uint64_t cr0;
    uint64_t cr2;
    uint64_t cr3;
    uint64_t cr4;
} FDP_CPU_CTX;
#pragma pack(pop)

enum
{
    FDPCMD_INIT,
    FDPCMD_PHYSICAL_VIRTUAL,
    FDPCMD_READ_PHYSICAL,
    FDPCMD_READ_REGISTER,
    FDPCMD_READ_MSR,
    FDPCMD_WRITE_MSR,
    FDPCMD_GET_MEMORYSIZE,
    FDPCMD_PAUSE_VM,
    FDPCMD_RESUME_VM,
    FDPCMD_SEARCH_PHYSICAL_MEMORY,
    FDPCMD_SEARCH_VIRTUAL_MEMORY,
    FDPCMD_UNSET_BP,
    FDPCMD_SET_BP,
    FDPCMD_VIRTUAL_PHYSICAL,
    FDPCMD_WRITE_PHYSICAL,
    FDPCMD_WRITE_VIRTUAL,
    FDPCMD_GET_STATE,
    FDPCMD_READ_VIRTUAL,
    FDPCMD_WRITE_REGISTER,
    FDPCMD_GET_FXSTATE,
    FDPCMD_SET_FXSTATE,
    FDPCMD_SINGLE_STEP,
    FDPCMD_GET_CPU_COUNT,
    FDPCMD_GET_CPU_STATE,
    FDPCMD_GET_CURRENT_CPU,
    FDPCMD_SWITCH_CPU,
    FDPCMD_REBOOT,
    FDPCMD_SAVE,
    FDPCMD_RESTORE,
    FDPCMD_INJECT_INTERRUPT,
    FDPCMD_TEST
};

typedef struct _FDP_UnsetBreakpoint_req
{
    uint8_t cmdType;
    int     breakPointId;
} FDP_UnsetBreakpoint_req;

typedef struct _FDP_SetBreakpoint_req
{
    uint8_t  cmdType;
    int      breakPointId;
    uint64_t breakAddress;
} FDP_SetBreakpoint_req;

#ifdef _MSC_VER
#    pragma pack(push, 1)
#    pragma warning(disable : 4200)
#endif

#define FDP_1M            1024 * 1024
#define FDP_MAX_DATA_SIZE 10 * FDP_1M

#ifdef FDP_INTERNAL_ONLY

#    include <atomic>

typedef struct FDP_SHM_CANAL_
{
    volatile uint8_t  data[FDP_MAX_DATA_SIZE];
    volatile uint32_t dataSize;
    std::atomic_bool  lock;         // Per channel lock
    std::atomic_bool  bDataPresent; // is data present
    volatile bool     bStatus;
    uint8_t           _; // padding
} FDP_SHM_CANAL;

typedef struct FDP_SHM_SHARED_
{
    std::atomic_bool lock; // General lock for the whole FDP_SHM_SHARED
    std::atomic_bool stateChangedLock;
    volatile bool    stateChanged;
    uint8_t          _; // padding
    FDP_SHM_CANAL    ClientToServer;
    FDP_SHM_CANAL    ServerToClient;
} FDP_SHM_SHARED;

struct ALIGNED_(1) FDP_SHM_
{
    FDP_SHM_SHARED* pSharedFDPSHM;                   // Shared part of the FDP SHM
    uint8_t         InputBuffer[FDP_MAX_DATA_SIZE];  // Used as temporary input buffer
    uint8_t         OutputBuffer[FDP_MAX_DATA_SIZE]; // Used as temporary output buffer

    FDP_SERVER_INTERFACE_T* pFdpServer;
    FDP_CPU_CTX*            pCpuShm;
};

#    define FDP_SHM_SHARED_SIZE sizeof(FDP_SHM_SHARED)
#endif

typedef struct FDP_SIMPLE_PKT_REQ_
{
    uint8_t Type;
} FDP_SIMPLE_PKT_REQ;

typedef struct FDP_READ_PHYSICAL_MEMORY_PKT_REQ_
{
    uint8_t  Type;
    uint32_t CpuId;
    uint64_t PhysicalAddress;
    uint32_t ReadSize;
} FDP_READ_PHYSICAL_MEMORY_PKT_REQ;

typedef struct FDP_READ_VIRTUAL_MEMORY_PKT_REQ_
{
    uint8_t  Type;
    uint32_t CpuId;
    uint64_t VirtualAddress;
    uint32_t ReadSize;
} FDP_READ_VIRTUAL_MEMORY_PKT_REQ;

typedef struct FDP_WRITE_PHYSICAL_MEMORY_PKT_REQ_
{
    uint8_t  Type;
    uint64_t PhysicalAddress;
    uint32_t WriteSize;
    uint8_t  Data[];
} FDP_WRITE_PHYSICAL_MEMORY_PKT_REQ;

typedef struct FDP_WRITE_VIRTUAL_MEMORY_PKT_REQ_
{
    uint8_t  Type;
    uint32_t CpuId;
    uint64_t VirtualAddress;
    uint32_t WriteSize;
    uint8_t  Data[];
} FDP_WRITE_VIRTUAL_MEMORY_PKT_REQ;

typedef struct FDP_SEARCH_PHYSICAL_MEMORY_PKT_REQ_
{
    uint8_t  Type;
    uint32_t CpuId;
    uint32_t PatternSize;
    uint64_t StartOffset;
    uint8_t  PatternData[];
} FDP_SEARCH_PHYSICAL_MEMORY_PKT_REQ;

typedef struct FDP_SEARCH_VIRTUAL_MEMORY_PKT_REQ_
{
    uint8_t  Type;
    uint32_t CpuId;
    uint32_t PatternSize;
    uint64_t StartOffset;
    uint8_t  PatternData[];
} FDP_SEARCH_VIRTUAL_MEMORY_PKT_REQ;

typedef struct FDP_READ_REGISTER_PKT_REQ_
{
    uint8_t      Type;
    uint32_t     CpuId;
    FDP_Register RegisterId;
} FDP_READ_REGISTER_PKT_REQ;

typedef struct FDP_READ_MSR_PKT_REQ_
{
    uint8_t  Type;
    uint32_t CpuId;
    uint64_t MsrId;
} FDP_READ_MSR_PKT_REQ;

typedef struct FDP_WRITE_MSR_PKT_REQ_
{
    uint8_t  Type;
    uint32_t CpuId;
    uint64_t MsrId;
    uint64_t MsrValue;
} FDP_WRITE_MSR_PKT_REQ;

typedef struct FDP_WRITE_REGISTER_PKT_REQ_
{
    uint8_t      Type;
    uint32_t     CpuId;
    FDP_Register RegisterId;
    uint64_t     RegisterValue;
} FDP_WRITE_REGISTER_PKT_REQ;

typedef struct FDP_VIRTUAL_PHYSICAL_PKT_REQ
{
    uint8_t  Type;
    uint32_t CpuId;
    uint64_t VirtualAddress;
} FDP_VIRTUAL_PHYSICAL_PKT_REQ;

typedef struct FDP_GET_STATE_PKT_REQ_
{
    uint8_t  Type;
    uint32_t CpuId;
} FDP_GET_STATE_PKT_REQ;

typedef struct FDP_SINGLE_STEP_PKT_REQ_
{
    uint8_t  Type;
    uint32_t CpuId;
} FDP_SINGLE_STEP_PKT_REQ;

typedef struct FDP_GET_CPU_STATE_PKT_REQ_
{
    uint8_t  Type;
    uint32_t CpuId;
} FDP_GET_CPU_STATE_PKT_REQ;

typedef struct FDP_UNSET_BREAKPOINT_PKT_REQ
{
    uint8_t Type;
    int     BreakpointId;
} FDP_CLEAR_BREAKPOINT_PKT_REQ;

typedef struct FDP_SET_BREAKPOINT_PKT_REQ_
{
    uint8_t            Type;
    uint32_t           CpuId;
    FDP_BreakpointType BreakpointType;
    int                BreakpointId;
    FDP_Access         BreakpointAccessType;
    FDP_AddressType    BreakpointAddressType;
    uint64_t           BreakpointAddress;
    uint64_t           BreakpointLength;
    uint64_t           BreakpointCr3;
} FDP_SET_BREAKPOINT_PKT_REQ;

typedef struct FDP_INJECT_INTERRUPT_PKT_REQ_
{
    uint8_t  Type;
    uint32_t CpuId;
    uint32_t InterruptionCode;
    uint32_t ErrorCode;
    uint64_t Cr2Value;
} FDP_INJECT_INTERRUPT_PKT_REQ;

typedef struct FDP_SET_FX_STATE_REQ_
{
    uint8_t              Type;
    uint32_t             CpuId;
    FDP_XSAVE_FORMAT64_T FxState64;
} FDP_SET_FX_STATE_REQ;

#ifdef _MSC_VER
#    pragma pack(pop)
#endif

#endif
