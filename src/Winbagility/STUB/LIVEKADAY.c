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
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <Windows.h>

#include "FDP_enum.h"
#include "Windows_structs.h"
#include "LIVEKADAY.h"
#include "utils.h"

LIVEKADAY_TYPE_T* LIVEKADAY_Open(char *pFileName)
{
    HANDLE hDevice = CreateFile("\\\\.\\pmem",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hDevice == INVALID_HANDLE_VALUE){
        printf("Failed to CreateFile\n");
        return NULL;
    }

    LIVEKADAY_TYPE_T* pLIVEKADAY = (LIVEKADAY_TYPE_T*)malloc(sizeof(LIVEKADAY_TYPE_T));
    if (pLIVEKADAY == NULL){
        return NULL;
    }
    pLIVEKADAY->hDevice = hDevice;

    return pLIVEKADAY;
}

bool LIVEKADAY_ReadPhysicalMemory(LIVEKADAY_TYPE_T* pUserHandle, uint8_t *pDstBuffer, uint32_t ReadSize, uint64_t PhysicalAddress)
{
    DWORD dwNumberOfBytesRead;
    LARGE_INTEGER liPhysicalAddress;
    liPhysicalAddress.QuadPart = PhysicalAddress;

    if (SetFilePointerEx(pUserHandle->hDevice, liPhysicalAddress, NULL, FILE_BEGIN) != -1) {
        if (ReadFile(pUserHandle->hDevice, pDstBuffer, ReadSize, &dwNumberOfBytesRead, NULL)){
            if (ReadSize == dwNumberOfBytesRead){
                return true;
            }
        }
    }
    return false;
}

bool LIVEKADAY_ReadRegister(LIVEKADAY_TYPE_T *pUserHandle, uint32_t CpuId, uint16_t RegisterId, uint64_t *pRegisterValue)
{
    switch (RegisterId){
    case FDP_CR3_REGISTER: *pRegisterValue = pUserHandle->Cr3Value; break;

    case FDP_RIP_REGISTER: *pRegisterValue = pUserHandle->TrapFrame.Rip; break;
    case FDP_CS_REGISTER: *pRegisterValue = pUserHandle->TrapFrame.SegCs; break;
    case FDP_SS_REGISTER: *pRegisterValue = pUserHandle->TrapFrame.SegSs; break;
    case FDP_DS_REGISTER: *pRegisterValue = pUserHandle->TrapFrame.SegDs; break;
    case FDP_ES_REGISTER: *pRegisterValue = pUserHandle->TrapFrame.SegEs; break;
    case FDP_FS_REGISTER: *pRegisterValue = pUserHandle->TrapFrame.SegFs; break;
    case FDP_GS_REGISTER: *pRegisterValue = pUserHandle->TrapFrame.SegGs; break;

    case FDP_RAX_REGISTER: *pRegisterValue = pUserHandle->TrapFrame.Rax; break;
    case FDP_RBX_REGISTER: *pRegisterValue = pUserHandle->TrapFrame.Rbx; break;
    case FDP_RCX_REGISTER: *pRegisterValue = pUserHandle->TrapFrame.Rcx; break;
    case FDP_RDX_REGISTER: *pRegisterValue = pUserHandle->TrapFrame.Rdx; break;
    case FDP_RSI_REGISTER: *pRegisterValue = pUserHandle->TrapFrame.Rsi; break;
    case FDP_RDI_REGISTER: *pRegisterValue = pUserHandle->TrapFrame.Rdi; break;
    case FDP_RSP_REGISTER: *pRegisterValue = pUserHandle->TrapFrame.Rsp; break;
    case FDP_RBP_REGISTER: *pRegisterValue = pUserHandle->TrapFrame.Rbp; break;
    case FDP_R10_REGISTER: *pRegisterValue = pUserHandle->TrapFrame.R10; break;
    case FDP_R11_REGISTER: *pRegisterValue = pUserHandle->TrapFrame.R11; break;

    case FDP_IDTRB_REGISTER: *pRegisterValue = pUserHandle->Idtr; break;
    case FDP_IDTRL_REGISTER: *pRegisterValue = 0xff; break;
    case FDP_GDTRB_REGISTER: *pRegisterValue = pUserHandle->Gdtr; break;
    case FDP_GDTRL_REGISTER: *pRegisterValue = 0xffff; break;

    default: *pRegisterValue = 0; break;
    }
    return true;
}

bool LIVEKADAY_Resume(LIVEKADAY_TYPE_T *pUserHandle)
{
    return true;
}

bool LIVEKADAY_SingleStep(LIVEKADAY_TYPE_T *pUserHandle, uint32_t CpuId)
{
    return true;
}

bool LIVEKADAY_Pause(LIVEKADAY_TYPE_T *pUserHandle)
{
    return true;
}

bool LIVEKADAY_WriteRegister(LIVEKADAY_TYPE_T *pUserHandle, uint32_t CpuId, uint16_t RegisterId, uint64_t RegisterValue)
{
    return true;
}

bool LIVEKADAY_UnsetBreakpoint(LIVEKADAY_TYPE_T *pUserHandle, uint8_t BreakPointId)
{
    return true;
}

bool LIVEKADAY_WritePhysicalMemory(LIVEKADAY_TYPE_T* pUserHandle, uint8_t *pSrcBuffer, uint32_t WriteSize, uint64_t PhysicalAddress)
{
    return false;
}

//TODO: bool !
uint64_t LIVEKADAY_SearchPhysicalMemory(LIVEKADAY_TYPE_T* pUserHandle, uint8_t *pPatternData, uint32_t PatternSize, uint64_t StartOffset)
{
    return false;
}

bool LIVEKADAY_WriteVirtuallMemory(LIVEKADAY_TYPE_T* pUserHandle, uint32_t CpuId, uint8_t *pSrcBuffer, uint32_t WriteSize, uint64_t VirtualAddr)
{
    return false;
}

bool LIVEKADAY_ReadMsr(LIVEKADAY_TYPE_T *pUserHandle, uint32_t CpuId, uint32_t MsrId, uint64_t *pMSRValue)
{
    return true;
}

bool LIVEKADAY_GetPhysicalMemorySize(LIVEKADAY_TYPE_T *pUserHandle, uint64_t *pPhysicalMemorySize)
{
    *pPhysicalMemorySize = pUserHandle->uFileSize;
    return true;
}

//Easy to Use ReadPhysical
uint64_t ReadPhysical64(LIVEKADAY_TYPE_T *pUserHandle, uint64_t PhysicalAddress)
{
    uint64_t Result;
    LIVEKADAY_ReadPhysicalMemory(pUserHandle, (uint8_t*)&Result, sizeof(Result), PhysicalAddress);
    return Result;
}

uint64_t VirtualToPhysical(LIVEKADAY_TYPE_T *pUserHandle, uint64_t VirtualAddress, uint64_t *pPhysicalAddress)
{
    uint64_t PML4E_index = (VirtualAddress & 0x0000FF8000000000) >> (9 + 9 + 9 + 12);
    uint64_t PDPE_index = (VirtualAddress & 0x0000007FC0000000) >> (9 + 9 + 12);
    uint64_t PDE_index = (VirtualAddress & 0x000000003FE00000) >> (9 + 12);
    uint64_t PTE_index = (VirtualAddress & 0x00000000001FF000) >> (12);
    uint64_t P_offset = (VirtualAddress & 0x0000000000000FFF);

    uint64_t PDPE_base = ReadPhysical64(pUserHandle, pUserHandle->Cr3Value + (PML4E_index * 8)) & 0x0000FFFFFFFFF000;
    if (PDPE_base == 0
        || PDPE_base > pUserHandle->uFileSize - _4K){
        return 0;
    }

    uint64_t tmp = ReadPhysical64(pUserHandle, PDPE_base + (PDPE_index * 8));
    if (tmp & 0x80){//This pas is a huge one (1G) !
        printf("TODO !!  HUGE !!!\n");
        return 0;
    }
    uint64_t PDE_base = tmp & 0x0000FFFFFFFFF000;
    if (PDE_base == 0
        || PDE_base > pUserHandle->uFileSize - _4K){
        return 0;
    }

    tmp = ReadPhysical64(pUserHandle, PDE_base + (PDE_index * 8));
    //Is Valid ?
    if ((tmp & 0x1) == 0){
        return 0;
    }
    if (tmp & 0x0000000000000080){ //This page is a large one (2M) !
        uint64_t tmpPhysical = ((tmp & 0xfffffffe00000) | (VirtualAddress & 0x00000000001FFFFF));
        if (tmpPhysical == 0
            || tmpPhysical > (pUserHandle->uFileSize - _2M)){
            return 0;
        }
        *pPhysicalAddress = tmpPhysical;
        return _2M;
    }
    uint64_t PTE_base = tmp & 0x0000FFFFFFFFF000;
    if (PTE_base == 0
        || PTE_base > pUserHandle->uFileSize - _4K){
        return 0;
    }

    //Is Valid ?
    tmp = ReadPhysical64(pUserHandle, PTE_base + (PTE_index * 8));
    if ((tmp & 0x1) == 0){
        return 0;
    }
    uint64_t P_base = tmp & 0x0000FFFFFFFFF000;
    if (P_base == 0
        || P_base > pUserHandle->uFileSize - _4K){
        return 0;
    }

    *pPhysicalAddress = (P_base | P_offset);
    return _4K;
}

bool LIVEKADAY_ReadVirtualMemory(LIVEKADAY_TYPE_T *pUserHandle, uint32_t CpuId, uint8_t *pDstBuffer, uint32_t ReadSize, uint64_t VirtualAddress)
{
    uint64_t PhysicalAddress;
    int64_t LeftToRead = ReadSize;
    uint64_t CurrentVirtualOffset = 0;
    while (LeftToRead > 0){
        uint64_t CurrentPageSize;
        CurrentPageSize = VirtualToPhysical(pUserHandle, VirtualAddress + CurrentVirtualOffset, &PhysicalAddress);
        if ( CurrentPageSize == 0){
            return false;
        }
        uint64_t PageBase = PhysicalAddress & ~(CurrentPageSize - 1);
        uint64_t PageEnd = PageBase + CurrentPageSize;
        uint32_t LeftOnPage = (uint32_t)(PageEnd - PhysicalAddress);
        uint32_t ByteToRead = (uint32_t)(MIN(LeftOnPage, LeftToRead));
        if (LIVEKADAY_ReadPhysicalMemory(pUserHandle, pDstBuffer + CurrentVirtualOffset, ByteToRead, PhysicalAddress) == false){
            return false;
        }
        LeftToRead = LeftToRead - ByteToRead;
        CurrentVirtualOffset = CurrentVirtualOffset + ByteToRead;
    }
    return true;
}

bool LIVEKADAY_GetFxState64(LIVEKADAY_TYPE_T *pUserHandle, uint32_t CpuId, void *pFxState64)
{
    return true;
}

bool LIVEKADAY_SetFxState64(LIVEKADAY_TYPE_T *pUserHandle, uint32_t CpuId, void *pFxState64)
{
    return true;
}

int LIVEKADAY_SetBreakpoint(LIVEKADAY_TYPE_T* pUserHandle,
    uint32_t CpuId,
    FDP_BreakpointType BreakpointType,
    uint8_t BreakpointId,
    FDP_Access BreakpointAccessType,
    FDP_AddressType BreakpointAddressType,
    uint64_t BreakpointAddress, uint64_t BreakpointLength)
{
    return true;
}

bool LIVEKADAY_GetCpuState(LIVEKADAY_TYPE_T* pUserHandle, uint32_t CpuId, FDP_State *pState){
    return false;
}

#pragma pack(push, 2)
typedef struct pmem_info_runs {
    __int64 start;
    __int64 length;
} PHYSICAL_MEMORY_RANGE;

typedef struct PmemMemoryInfo_ {
    LARGE_INTEGER CR3;
    LARGE_INTEGER NtBuildNumber;
    LARGE_INTEGER KernBase;
    LARGE_INTEGER KDBG;
    LARGE_INTEGER KPCR[32];
    LARGE_INTEGER PfnDataBase;
    LARGE_INTEGER PsLoadedModuleList;
    LARGE_INTEGER PsActiveProcessHead;
    LARGE_INTEGER NtBuildNumberAddr;
    LARGE_INTEGER Padding[0xfe];
    LARGE_INTEGER NumberOfRuns;
    PHYSICAL_MEMORY_RANGE Run[100];
}PmemMemoryInfo;
#pragma pack(pop)


/*
+0x000 GdtBase           : 0xfffff801`d0aa9000 _KGDTENTRY64
+ 0x008 TssBase          : 0xfffff801`d0aaa080 _KTSS64
+ 0x010 UserRsp          : 0x0000008a`7d36f568
+ 0x018 Self             : 0xfffff801`cf312000 _KPCR
+ 0x020 CurrentPrcb      : 0xfffff801`cf312180 _KPRCB
+ 0x028 LockArray        : 0xfffff801`cf3127f0 _KSPIN_LOCK_QUEUE
+ 0x030 Used_Self        : 0x00007ff6`e4e6c000 Void
+ 0x038 IdtBase          : 0xfffff801`d0aa9080 _KIDTENTRY64
*/
#define KPCR_GDTRB_OFF          0x00
#define KPCR_IDTRB_OFF          0x38
#define KPCR_PRCB_OFF            0x180
#define KPRCB_CURRENTTHREAD_OFF 0x8
#define KTHREAD_TRAPFRAME_OFF    0x90
bool LIVEKADAY_Init(LIVEKADAY_TYPE_T *pUserHandle)
{
    PmemMemoryInfo PmemInfo;
#define PMEM_INFO_IOCTRL CTL_CODE(0x22, 0x103, 0, 3)
    DWORD dwBytesReturned;
    if (!DeviceIoControl(pUserHandle->hDevice, PMEM_INFO_IOCTRL, NULL, 0, (char *)&PmemInfo,
        sizeof(PmemInfo), &dwBytesReturned, NULL)) {
        return false;
    };

    //We got Cr3, we can read VirtualMemory !
    pUserHandle->Cr3Value = PmemInfo.CR3.QuadPart;
    pUserHandle->uFileSize = PmemInfo.Run[PmemInfo.NumberOfRuns.QuadPart - 1].start + PmemInfo.Run[PmemInfo.NumberOfRuns.QuadPart - 1].length;
    pUserHandle->v_KPCR = PmemInfo.KPCR[0].QuadPart;

    //Get Idtr
    if (LIVEKADAY_ReadVirtualMemory(pUserHandle,
        0,
        (uint8_t*)&pUserHandle->Idtr,
        sizeof(uint64_t), 
        pUserHandle->v_KPCR + KPCR_IDTRB_OFF) == false){
        return false;
    }

    //Get Gdtr
    if (LIVEKADAY_ReadVirtualMemory(pUserHandle,
        0,
        (uint8_t*)&pUserHandle->Gdtr,
        sizeof(uint64_t),
        pUserHandle->v_KPCR + KPCR_GDTRB_OFF) == false){
        return false;
    }


    //Get the TrapFrame
    uint64_t v_KPRCB = pUserHandle->v_KPCR + KPCR_PRCB_OFF;
    uint64_t CurrentThread;
    if (LIVEKADAY_ReadVirtualMemory(pUserHandle, 0, (uint8_t*)&CurrentThread, sizeof(CurrentThread), v_KPRCB + KPRCB_CURRENTTHREAD_OFF) == false){
        return false;
    }
    uint64_t v_TrapFrame;
    if (LIVEKADAY_ReadVirtualMemory(pUserHandle, 0, (uint8_t*)&v_TrapFrame, sizeof(v_TrapFrame), CurrentThread + KTHREAD_TRAPFRAME_OFF) == false){
        return false;
    }
    if (LIVEKADAY_ReadVirtualMemory(pUserHandle, 0, (uint8_t*)&pUserHandle->TrapFrame, sizeof(pUserHandle->TrapFrame), v_TrapFrame) == false){
        return false;
    }
        
    return true;
}