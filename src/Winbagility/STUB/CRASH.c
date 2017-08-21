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
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "FDP.h"
#include "Windows.h"
#include "Windows_structs.h"
#include "CRASH.h"
#include "utils.h"


#define SYSTEM_PROCESS_ID 0x4

CRASH_TYPE_T* CRASH_Open(char *pFileName)
{
    HANDLE    hFile = CreateFileA(pFileName,
        GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING,
        FILE_FLAG_RANDOM_ACCESS,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE){
        printf("Failed to CreateFile\n");
        return NULL;
    }

    HANDLE map_handle = CreateFileMapping(hFile, NULL, PAGE_READWRITE | SEC_RESERVE, 0, 0, 0);
    if (map_handle == NULL) {
        printf("Failed to CreateFileMapping\n");
        CloseHandle(hFile);
        return NULL;
    }

    uint8_t *pPhysicalMemory = NULL;
    pPhysicalMemory = (uint8_t*)MapViewOfFile(map_handle, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, 0);
    if (pPhysicalMemory == NULL)
    {
        printf("Failed to MapViewOfFile\n");
        CloseHandle(map_handle);
        CloseHandle(hFile);
        return NULL;
    }

    LARGE_INTEGER liTempValue;
    GetFileSizeEx(hFile, &liTempValue);

    CRASH_TYPE_T* pCRASH = (CRASH_TYPE_T*)malloc(sizeof(CRASH_TYPE_T));
    if (pCRASH == NULL){
        return NULL;
    }
    pCRASH->hFile = hFile;
    pCRASH->uFileSize = liTempValue.QuadPart;
    pCRASH->pFileData = pPhysicalMemory;
 
    return pCRASH;
}

bool CRASH_ReadPhysicalMemory(CRASH_TYPE_T* pUserHandle, uint8_t *pDstBuffer, uint32_t ReadSize, uint64_t PhysicalAddress)
{
    if (PhysicalAddress+ReadSize > pUserHandle->uFileSize){
        return false;
    }
    memcpy(pDstBuffer, pUserHandle->pFileData + PhysicalAddress, ReadSize);
    return true;
}

//TODO !
bool FindCr3Fast(CRASH_TYPE_T *pUserHandle, uint64_t *pCr3Value)
{
    *pCr3Value = 0x1aa000;
    //TODO Verify...
    return false;
}



/*
* @brief : This function looks for a valid DTB
*
* @param context Analysis context
* @return value a DTB
*/
//TODO Generic function with profil and PDB brute force
#define WIN7_KPROCESS_INAGEFILENAME_OFF 0x2E0
#define WIN7_KPROCESS_UNIQUEPROCESSID_OFF 0x180
#define WIN7_KPROCESS_DIRECTORYTABLEBASE_OFF 0x28
bool FindCr3Windows7(CRASH_TYPE_T *pUserHandle, uint64_t *pCr3Value)
{
    const char SystemImageFileName[] = { 'S', 'y', 's', 't', 'e', 'm', 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uint64_t p_KPROCESS;
    for (p_KPROCESS = 0x00; p_KPROCESS<pUserHandle->uFileSize - _4K; p_KPROCESS++){
        //Is ImageFileName == "System" ?
        if (memcmp(pUserHandle->pFileData + p_KPROCESS + WIN7_KPROCESS_INAGEFILENAME_OFF, SystemImageFileName, sizeof(SystemImageFileName)) == 0){
            uint64_t uPid = 0;
            //Is PID == 4 ?
            if (CRASH_ReadPhysicalMemory(pUserHandle, (uint8_t*)&uPid, sizeof(uPid), p_KPROCESS + WIN7_KPROCESS_UNIQUEPROCESSID_OFF) == true
                && uPid == SYSTEM_PROCESS_ID){
                LIST_ENTRY64 ProcessListEntry;
                if (CRASH_ReadPhysicalMemory(pUserHandle, (uint8_t*)&ProcessListEntry, sizeof(ProcessListEntry), p_KPROCESS + 0xe0)
                    && ProcessListEntry.Blink > KERNEL_SPACE_START
                    && ProcessListEntry.Flink > KERNEL_SPACE_START){
                    printf("Physical System KPROCESS : 0x%llx\n", p_KPROCESS);
                    uint64_t DirectoryTableBase = 0;
                    CRASH_ReadPhysicalMemory(pUserHandle, (uint8_t*)&DirectoryTableBase, sizeof(DirectoryTableBase), p_KPROCESS + WIN7_KPROCESS_DIRECTORYTABLEBASE_OFF);
                    *pCr3Value = DirectoryTableBase;
                    return true;
                }
            }
        }
    }
    return false;
}

#define WIN81_KPROCESS_INAGEFILENAME_OFF 0x438
#define WIN81_KPROCESS_UNIQUEPROCESSID_OFF 0x2e0
#define WIN81_KPROCESS_DIRECTORYTABLEBASE_OFF 0x28
#define SYSTEM_PROCESS_ID 0x4
bool FindCr3Windows81(CRASH_TYPE_T *pUserHandle, uint64_t *pCr3Value)
{
    const char SystemImageFileName[] = { 'S', 'y', 's', 't', 'e', 'm', 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uint64_t p_KPROCESS;
    for (p_KPROCESS = 0x00; p_KPROCESS<pUserHandle->uFileSize - _4K; p_KPROCESS++){
        //Is ImageFileName == "System" ?
        if (memcmp(pUserHandle->pFileData + p_KPROCESS + WIN81_KPROCESS_INAGEFILENAME_OFF, SystemImageFileName, sizeof(SystemImageFileName)) == 0){
            uint64_t uPid = 0;
            if (CRASH_ReadPhysicalMemory(pUserHandle, (uint8_t*)&uPid, sizeof(uPid), p_KPROCESS + WIN81_KPROCESS_UNIQUEPROCESSID_OFF) == true
                && uPid == SYSTEM_PROCESS_ID){
                //LIST_ENTRY64 ProcessListEntry;
                //if (CRASH_ReadPhysicalMemory(pUserHandle, (uint8_t*)&ProcessListEntry, sizeof(ProcessListEntry), p_KPROCESS + 0xe0)
                //    && ProcessListEntry.Blink > KERNEL_SPACE_START
                //    && ProcessListEntry.Flink > KERNEL_SPACE_START){
                    printf("Physical System KPROCESS : 0x%llx\n", p_KPROCESS);
                    uint64_t DirectoryTableBase = 0;
                    CRASH_ReadPhysicalMemory(pUserHandle, (uint8_t*)&DirectoryTableBase, sizeof(DirectoryTableBase), p_KPROCESS + WIN81_KPROCESS_DIRECTORYTABLEBASE_OFF);
                    *pCr3Value = DirectoryTableBase;
                    return true;
                //}
            }
        }
    }
    return false;
}

bool FindCr3Windows10(CRASH_TYPE_T *pUserHandle, uint64_t *pCr3Value)
{
    const char SystemImageFileName[] = { 'S', 'y', 's', 't', 'e', 'm', 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uint64_t p_KPROCESS;
    for (p_KPROCESS = 0x00; p_KPROCESS<pUserHandle->uFileSize - _4K; p_KPROCESS++){
        //Is ImageFileName == "System" ?
        if (memcmp(pUserHandle->pFileData + p_KPROCESS + 0x450, SystemImageFileName, sizeof(SystemImageFileName)) == 0){
            uint64_t uPid = 0;
            if (CRASH_ReadPhysicalMemory(pUserHandle, (uint8_t*)&uPid, sizeof(uPid), p_KPROCESS + 0x2e8) == true){
                //Is PID == 4 ?
                if (uPid == SYSTEM_PROCESS_ID){
                    printf("Physical System KPROCESS : 0x%llx\n", p_KPROCESS);
                    uint64_t DirectoryTableBase = 0;
                    CRASH_ReadPhysicalMemory(pUserHandle, (uint8_t*)&DirectoryTableBase, sizeof(DirectoryTableBase), p_KPROCESS + 0x28);
                    *pCr3Value = DirectoryTableBase;
                    return true;
                }
            }
        }
    }
    return false;
}



#define WIN7_KPROCESS_DIRECTORYTABLEBASE_OFF_X86    0x018
#define WIN7_KPROCESS_UNIQUEPROCESSID_OFF_X86        0x0B4
#define WIN7_KPROCESS_INAGEFILENAME_OFF_X86            0x16C
bool FindCr3Windows7_x86(CRASH_TYPE_T *pUserHandle, uint64_t *pCr3Value)
{
    const char SystemImageFileName[] = { 'S', 'y', 's', 't', 'e', 'm', 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uint64_t p_KPROCESS;
    for (p_KPROCESS = 0x00; p_KPROCESS<pUserHandle->uFileSize - WIN7_KPROCESS_INAGEFILENAME_OFF_X86 - sizeof(SystemImageFileName); p_KPROCESS++) {
        //Is ImageFileName == "System" ?
        if (memcmp(pUserHandle->pFileData + p_KPROCESS + WIN7_KPROCESS_INAGEFILENAME_OFF_X86, SystemImageFileName, sizeof(SystemImageFileName)) == 0) {
            uint32_t uPid = 0;
            //Is PID == 4 ?
            if (CRASH_ReadPhysicalMemory(pUserHandle, (uint8_t*)&uPid, sizeof(uPid), p_KPROCESS + WIN7_KPROCESS_UNIQUEPROCESSID_OFF_X86) == true
                && uPid == SYSTEM_PROCESS_ID) {
                LIST_ENTRY32 ProcessListEntry;
                if (CRASH_ReadPhysicalMemory(pUserHandle, (uint8_t*)&ProcessListEntry, sizeof(ProcessListEntry), p_KPROCESS + 0xB8)
                    && ProcessListEntry.Blink > KERNEL_SPACE_START_X86
                    && ProcessListEntry.Flink > KERNEL_SPACE_START_X86) {
                    printf("Physical System KPROCESS : 0x%llx\n", p_KPROCESS);
                    uint32_t DirectoryTableBase = 0;
                    CRASH_ReadPhysicalMemory(pUserHandle, (uint8_t*)&DirectoryTableBase, sizeof(DirectoryTableBase), p_KPROCESS + WIN7_KPROCESS_DIRECTORYTABLEBASE_OFF_X86);
                    *pCr3Value = DirectoryTableBase;
                    return true;
                }
            }
        }
    }
    return false;
}


/*
* @brief : This function looks for a valid DTB
*
* @param context Analysis context
* @return value a DTB
*/
bool FindCr3(CRASH_TYPE_T *pUserHandle, uint64_t *pCr3Value)
{
    if (FindCr3Fast(pUserHandle, pCr3Value) == true){
        //TODO Check Cr3
        return true;
    }
    if (FindCr3Windows7(pUserHandle, pCr3Value) == true){
        //TODO Check Cr3
        return true;
    }
    if (FindCr3Windows81(pUserHandle, pCr3Value) == true){
        //TODO Check Cr3
        return true;
    }
    if (FindCr3Windows10(pUserHandle, pCr3Value) == true){
        //TODO Check Cr3
        return true;
    }
    if (FindCr3Windows7_x86(pUserHandle, pCr3Value) == true) {
        //TODO Check Cr3
        return true;
    }
    return false;
}


/*+0x000 GdtBase          : 0xfffff801`d0aa9000 _KGDTENTRY64
+ 0x008 TssBase          : 0xfffff801`d0aaa080 _KTSS64
+ 0x010 UserRsp          : 0x0000008a`7d36f568
+ 0x018 Self             : 0xfffff801`cf312000 _KPCR
+ 0x020 CurrentPrcb      : 0xfffff801`cf312180 _KPRCB
+ 0x028 LockArray        : 0xfffff801`cf3127f0 _KSPIN_LOCK_QUEUE
+ 0x030 Used_Self        : 0x00007ff6`e4e6c000 Void
+ 0x038 IdtBase          : 0xfffff801`d0aa9080 _KIDTENTRY64*/

bool CRASH_ReadRegister(CRASH_TYPE_T *pUserHandle, uint32_t CpuId, uint16_t RegisterId, uint64_t *pRegisterValue)
{
    //Physical Memory Dump...
    switch (RegisterId){
    case FDP_CR3_REGISTER: *pRegisterValue = pUserHandle->Cr3Value; break;
    case FDP_RIP_REGISTER: *pRegisterValue = 0x00; break; //TODO: from KPCR->CurrentThread->TrapFrame
    case FDP_CS_REGISTER: *pRegisterValue = 0x0010; break;
    case FDP_SS_REGISTER: *pRegisterValue = 0x0018; break;
    case FDP_DS_REGISTER: *pRegisterValue = 0x002b; break;
    case FDP_ES_REGISTER: *pRegisterValue = 0x002b; break;
    case FDP_FS_REGISTER: *pRegisterValue = 0x0053; break;
    case FDP_GS_REGISTER: *pRegisterValue = 0x002B; break;
    case FDP_IDTRB_REGISTER: *pRegisterValue = pUserHandle->IDTRB; break;
    case FDP_IDTRL_REGISTER: *pRegisterValue = 0xff; break;
    case FDP_GDTRB_REGISTER: *pRegisterValue = pUserHandle->GDTRB; break;
    case FDP_GDTRL_REGISTER: *pRegisterValue = 0xffff; break;

    default: *pRegisterValue = 0; break;
    }
    return true;
}

bool CRASH_Resume(CRASH_TYPE_T *pUserHandle)
{
    return true;
}

bool CRASH_SingleStep(CRASH_TYPE_T *pUserHandle, uint32_t CpuId)
{
    return true;
}

bool CRASH_Pause(CRASH_TYPE_T *pUserHandle)
{
    return true;
}

bool CRASH_WriteRegister(CRASH_TYPE_T *pUserHandle, uint32_t CpuId, uint16_t RegisterId, uint64_t RegisterValue)
{
    return true;
}

bool CRASH_UnsetBreakpoint(CRASH_TYPE_T *pUserHandle, uint8_t BreakPointId)
{
    return true;
}

bool CRASH_WritePhysicalMemory(CRASH_TYPE_T* pUserHandle, uint8_t *pSrcBuffer, uint32_t WriteSize, uint64_t PhysicalAddress)
{
    if (PhysicalAddress > pUserHandle->uFileSize){
        return false;
    }
#if 0
    //Do we really want this on a mapped file
    memcpy((void*)(context->physicalMemory + physicalAddress), srcBuffer, size);
#endif
    return true;
}

//TODO: bool !
uint64_t CRASH_SearchPhysicalMemory(CRASH_TYPE_T* pUserHandle, uint8_t *pPatternData, uint32_t PatternSize, uint64_t StartOffset)
{
    for (uint64_t i = StartOffset; i < pUserHandle->uFileSize - PatternSize; i++){
        if (memcmp(pUserHandle->pFileData + i, pPatternData, PatternSize) == 0){
            return i;
        }
    }
    return 0;
}

bool CRASH_WriteVirtuallMemory(CRASH_TYPE_T* pUserHandle, uint32_t CpuId, uint8_t *pSrcBuffer, uint32_t WriteSize, uint64_t VirtualAddr)
{
    return false;
}

//TODO !
bool CRASH_ReadMsr(CRASH_TYPE_T *pUserHandle, uint32_t CpuId, uint32_t MsrId, uint64_t *pMSRValue)
{
    return false;
}

//Easy to Use ReadPhysical
uint64_t CRASH_ReadPhysical64(CRASH_TYPE_T *pUserHandle, uint64_t PhysicalAddress)
{
    uint64_t Result;
    CRASH_ReadPhysicalMemory(pUserHandle, (uint8_t*)&Result, sizeof(Result), PhysicalAddress);
    return Result;
}

uint32_t CRASH_ReadPhysical32(CRASH_TYPE_T *pUserHandle, uint32_t PhysicalAddress)
{
    uint32_t Result;
    CRASH_ReadPhysicalMemory(pUserHandle, (uint8_t*)&Result, sizeof(Result), PhysicalAddress);
    return Result;
}

bool CRASH_GetPhysicalMemorySize(CRASH_TYPE_T *pUserHandle, uint64_t *pPhysicalMemorySize)
{
    *pPhysicalMemorySize = pUserHandle->uFileSize;
    return true;
}

/*x86*/
uint32_t CRASH_VirtualToPhysical32(CRASH_TYPE_T *pUserHandle, uint32_t VirtualAddress, uint32_t *pPhysicalAddress)
{
    uint32_t PD_index = (VirtualAddress >> 22);
    uint32_t PT_index = (VirtualAddress >> 12) & 0x3FF;
    *pPhysicalAddress = 0;

    uint32_t tmp = CRASH_ReadPhysical32(pUserHandle, (pUserHandle->Cr3Value & 0xFFFFFF) + (PD_index * 4));
    if ((tmp & 0x1) == false) {
        return 0;
    }
    uint32_t PD_Base = tmp & 0xFFFFFF000;
    if (tmp == 0 || tmp > pUserHandle->uFileSize - _4K) {
        return 0;
    }
    tmp = CRASH_ReadPhysical32(pUserHandle, PD_Base + (PT_index * 4));
    if ((tmp & 0x1) == false) {
        return 0;
    }
    uint32_t P_Base = tmp & 0xFFFFFF000;
    if (P_Base == 0 || P_Base > pUserHandle->uFileSize - _4K) {
        return 0;
    }

    *pPhysicalAddress = (P_Base | (VirtualAddress & 0xFFF));
    return _4K;
}

//TODO ! Finish it !
/*x86PAE*/
uint32_t CRASH_VirtualToPhysical32PAE(CRASH_TYPE_T *pUserHandle, uint32_t VirtualAddress, uint32_t *pPhysicalAddress)
{
    uint32_t PDP_index =    (VirtualAddress >> 30);
    uint32_t PD_index =        (VirtualAddress >> 21) & 0x1FF;
    uint32_t PT_index =        (VirtualAddress >> 12) & 0x1FF;
    *pPhysicalAddress = 0;

    uint64_t tmp = CRASH_ReadPhysical64(pUserHandle, pUserHandle->Cr3Value + (PDP_index * 8));
    uint32_t PD_Base = (tmp & 0xFFFFF00000000000 >> 36);
    tmp = CRASH_ReadPhysical64(pUserHandle, PD_Base + (PD_index * 8));
    if ((tmp & 0x1) == false) {
        return 0;
    }

    if ((tmp & 0x80) == true) {
        //2M Page !
        uint32_t P_Base = (uint32_t)(tmp & 0xFFFE00000);
        if (P_Base == 0 || P_Base > pUserHandle->uFileSize - _2M) {
            return 0;
        }

        *pPhysicalAddress = (P_Base | (VirtualAddress & 0x1FFFFF));
        return _2M;
    }
    uint32_t PT_Base = (tmp & 0xFFFFF00000000000 >> 36);
    if (tmp == 0 || tmp > pUserHandle->uFileSize - _4K) {
        return 0;
    }

    tmp = CRASH_ReadPhysical64(pUserHandle, PT_Base + (PT_index *8));
    if ((tmp & 0x1) == false) {
        return 0;
    }

    uint32_t P_Base = (tmp & 0xFFFFF00000000000 >> 36);
    if (P_Base == 0 || P_Base > pUserHandle->uFileSize - _4K) {
        return 0;
    }

    *pPhysicalAddress = (P_Base | (VirtualAddress & 0xFFF));
    return _4K;
}

//TODO: Generic function, LIVEKADAY is using this function too...
uint64_t CRASH_VirtualToPhysical(CRASH_TYPE_T *pUserHandle, uint64_t VirtualAddress, uint64_t *pPhysicalAddress)
{
    uint64_t PML4E_index = (VirtualAddress & 0x0000FF8000000000) >> (9 + 9 + 9 + 12);
    uint64_t PDPE_index = (VirtualAddress & 0x0000007FC0000000) >> (9 + 9 + 12);
    uint64_t PDE_index = (VirtualAddress & 0x000000003FE00000) >> (9 + 12);
    uint64_t PTE_index = (VirtualAddress & 0x00000000001FF000) >> (12);
    uint64_t P_offset = (VirtualAddress & 0x0000000000000FFF);

    uint64_t PDPE_base = CRASH_ReadPhysical64(pUserHandle, pUserHandle->Cr3Value + (PML4E_index * 8)) & 0x0000FFFFFFFFF000;
    if (PDPE_base == 0
        || PDPE_base > pUserHandle->uFileSize - _4K){
        return 0;
    }

    uint64_t tmp = CRASH_ReadPhysical64(pUserHandle, PDPE_base + (PDPE_index * 8));
    if (tmp & 0x80){//This pas is a huge one (1G) !
        printf("TODO !!  HUGE !!!\n");
        return 0;
    }
    uint64_t PDE_base = tmp & 0x0000FFFFFFFFF000;
    if (PDE_base == 0
        || PDE_base > pUserHandle->uFileSize - _4K){
        return 0;
    }

    tmp = CRASH_ReadPhysical64(pUserHandle, PDE_base + (PDE_index * 8));
    //Is Valid ?
    if ((tmp & 0x1) == 0){
        return 0;
    }
    if (tmp & 0x0000000000000080){ //This page is a large one (2M) !
        uint64_t tmpPhysical = ((tmp & 0xfffffffe00000) | (VirtualAddress & 0x00000000001FFFFF));
        if (tmpPhysical == 0
            || tmpPhysical > (pUserHandle->uFileSize - (2 * 1024 * 1024))){
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
    tmp = CRASH_ReadPhysical64(pUserHandle, PTE_base + (PTE_index * 8));
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


bool CRASH_ReadVirtualMemory(CRASH_TYPE_T *pUserHandle, uint32_t CpuId, uint8_t *pDstBuffer, uint32_t ReadSize, uint64_t VirtualAddress)
{
    uint64_t PhysicalAddress = 0;
    int64_t LeftToRead = ReadSize;
    uint64_t CurrentVirtualOffset = 0;
    while (LeftToRead > 0){
        uint64_t CurrentPageSize;
        CurrentPageSize = CRASH_VirtualToPhysical(pUserHandle, VirtualAddress + CurrentVirtualOffset, &PhysicalAddress);
        if (CurrentPageSize == 0){
            return false;
        }
        uint64_t PageBase = PhysicalAddress & ~(CurrentPageSize - 1);
        uint64_t PageEnd = PageBase + CurrentPageSize;
        uint32_t LeftOnPage = (uint32_t)(PageEnd - PhysicalAddress);
        uint32_t ByteToRead = (uint32_t)(MIN(LeftOnPage, LeftToRead));
        if (CRASH_ReadPhysicalMemory(pUserHandle, pDstBuffer + CurrentVirtualOffset, ByteToRead, PhysicalAddress) == false){
            return false;
        }
        LeftToRead = LeftToRead - ByteToRead;
        CurrentVirtualOffset = CurrentVirtualOffset + ByteToRead;
    }
    return true;
}


bool CRASH_GetFxState64(CRASH_TYPE_T *pUserHandle, uint32_t CpuId, void *pFxState64)
{
    return true;
}

bool CRASH_SetFxState64(CRASH_TYPE_T *pUserHandle, uint32_t CpuId, void *pFxState64)
{
    return true;
}

int CRASH_SetBreakpoint(CRASH_TYPE_T* pUserHandle,
    uint32_t CpuId,
    FDP_BreakpointType BreakpointType,
    uint8_t BreakpointId,
    FDP_Access BreakpointAccessType,
    FDP_AddressType BreakpointAddressType,
    uint64_t BreakpointAddress, uint64_t BreakpointLength)
{
    return true;
}


/*
KPCR is aligned on 0x100

nt!_KPCR
+0x000 NtTib            : _NT_TIB
+0x000 GdtBase          : Ptr64 _KGDTENTRY64
+0x008 TssBase          : Ptr64 _KTSS64
+0x010 UserRsp          : Uint8B
+0x018 Self             : Ptr64 _KPCR
+0x020 CurrentPrcb      : Ptr64 _KPRCB
+0x028 LockArray        : Ptr64 _KSPIN_LOCK_QUEUE
+0x030 Used_Self        : Ptr64 Void
+0x038 IdtBase          : Ptr64 _KIDTENTRY64
+0x040 Unused           : [2] Uint8B
+0x050 Irql             : UChar
+0x051 SecondLevelCacheAssociativity : UChar
+0x052 ObsoleteNumber   : UChar
+0x053 Fill0            : UChar
+0x054 Unused0          : [3] Uint4B
+0x060 MajorVersion     : Uint2B
+0x062 MinorVersion     : Uint2B
+0x064 StallScaleFactor : Uint4B
+0x068 Unused1          : [3] Ptr64 Void
+0x080 KernelReserved   : [15] Uint4B
+0x0bc SecondLevelCacheSize : Uint4B
+0x0c0 HalReserved      : [16] Uint4B
+0x100 Unused2          : Uint4B
+0x108 KdVersionBlock   : Ptr64 Void
+0x110 Unused3          : Ptr64 Void
+0x118 PcrAlign1        : [24] Uint4B
+0x180 Prcb             : _KPRCB
*/
#define KPCR_ALIGN                0x100
#define KPCR_GDTRB_OFF          0x00
#define KPCR_IDTRB_OFF          0x38
#define KPCR_SELF_OFF           0x18
#define KPCR_CURRENTPRCB_OFF    0x20
#define KPCR_PRCB_OFF           0x180
bool FindKPCR(CRASH_TYPE_T *pUserHandle, uint64_t *pKPCR)
{
    //Iterate over all physical pages
    for (uint64_t PageBase = 0; PageBase < pUserHandle->uFileSize; PageBase += KPCR_ALIGN){
        //Read a possible value of v_KPCR
        uint64_t v_KPCR = CRASH_ReadPhysical64(pUserHandle, PageBase + KPCR_SELF_OFF);
        //printf("v_KPCR %p\n", v_KPCR);
        //Check if the possible v_KPCR is in KernelSpace and page aligned
        if (v_KPCR > KERNEL_SPACE_START /*&& ((v_KPCR & (KPCR_ALIGN-1)) == 0)*/){
            uint64_t p_KPCR;
            //Convert v_KPCR to p_KPCR and check if the page is valid,
            if (CRASH_VirtualToPhysical(pUserHandle, v_KPCR, &p_KPCR) > 0){
                //Check if p_KPCR is at PageBase, Is this the hardest test...
                if (p_KPCR == PageBase){
                    //We passed the MMU test !!
                    //Get the possible v_KPRCB
                    uint64_t v_KPRCB = CRASH_ReadPhysical64(pUserHandle, p_KPCR + KPCR_CURRENTPRCB_OFF);
                    //Paranoid check !
                    if (v_KPRCB == (v_KPCR + KPCR_PRCB_OFF)){
                        uint64_t p_KPRCB;
                        if (CRASH_VirtualToPhysical(pUserHandle, v_KPRCB, &p_KPRCB) > 0){
                            if (p_KPRCB == p_KPCR + KPCR_PRCB_OFF) {
                                *pKPCR = v_KPCR;
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

#define KPCR_ALIGN_X86        0x100
#define KPCR_SELF_OFF_X86    0x1C
bool FindKPCRx86(CRASH_TYPE_T *pUserHandle, uint64_t *pKPCR)
{
    *pKPCR = 0x8273bc00;
    return true;

    //Iterate over all physical pages
    for (uint32_t PageBase = 0; PageBase < pUserHandle->uFileSize; PageBase += KPCR_ALIGN_X86) {
        //Read a possible value of v_KPCR
        uint32_t v_KPCR = CRASH_ReadPhysical32(pUserHandle, PageBase + KPCR_SELF_OFF_X86);
        //Check if the possible v_KPCR is in KernelSpace and page aligned
        if (v_KPCR > KERNEL_SPACE_START_X86 /*&& ((v_KPCR & (KPCR_ALIGN-1)) == 0)*/) {
            uint32_t p_KPCR = 0;
            //Convert v_KPCR to p_KPCR and check if the page is valid,
            if (CRASH_VirtualToPhysical32(pUserHandle, v_KPCR, &p_KPCR) > 0) {
                //Check if p_KPCR is at PageBase, Is this the hardest test...
                if (p_KPCR == PageBase) {
                    //We passed the MMU test !!
                    //Get the possible v_KPRCB


                    printf("%08x %08x\n", p_KPCR, PageBase);

                    *pKPCR = v_KPCR;
                    return true;
                }
            }
        }
    }
    return false;
}

uint32_t CRASH_ReadVirtual32(CRASH_TYPE_T *pUserHandle, uint32_t VirtualAddress)
{
    uint32_t Result = 0;
    CRASH_ReadVirtualMemory(pUserHandle, 0, (uint8_t*)&Result, sizeof(Result), VirtualAddress);
    return Result;
}

bool CRASH_GetCpuState(CRASH_TYPE_T *pUserHandle, uint32_t CpuId, uint16_t *pState)
{
    return false;
}

bool CRASH_Init(CRASH_TYPE_T *pUserHandle)
{
    if (FindCr3(pUserHandle, &pUserHandle->Cr3Value) == false){
        printf("Failed to FindCr3\n");
        return false;
    }

    if (FindKPCRx86(pUserHandle, &pUserHandle->v_KPCR) == false){
        printf("Failed to FindKPCR\n");
        return false;
    }

    if (FindKPCR(pUserHandle, &pUserHandle->v_KPCR) == false){
        printf("Failed to FindKPCR\n");
        return false;
    }

    //Read the IDTRB
    if (CRASH_ReadVirtualMemory(pUserHandle, 0, (uint8_t*)&pUserHandle->IDTRB, sizeof(pUserHandle->IDTRB), pUserHandle->v_KPCR + KPCR_IDTRB_OFF) == false){
        return false;
    }
    if (pUserHandle->IDTRB < KERNEL_SPACE_START){
        return false;
    }

    //Read the GDTRB
    if (CRASH_ReadVirtualMemory(pUserHandle, 0, (uint8_t*)&pUserHandle->GDTRB, sizeof(pUserHandle->GDTRB), pUserHandle->v_KPCR + KPCR_GDTRB_OFF) == false){
        return false;
    }
    if (pUserHandle->GDTRB < KERNEL_SPACE_START){
        return false;
    }


    return true;
}