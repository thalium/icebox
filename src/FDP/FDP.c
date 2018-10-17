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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifndef __cplusplus
    #include <stdbool.h>
#endif

#include "include/FDP.h"
#include "include/FDP_structs.h"

#ifdef  __linux
//Linux
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define SLEEP(X) (usleep(X*1000))

#elif   _WIN32
#include <Windows.h>
#include <intrin.h>

#define SLEEP(X) (Sleep(X))
#endif


#define MIN(a,b) (((a)<(b))?(a):(b))

#define FDP_POWER_SAVE 1

__inline static void ttas_spinlock_lock(volatile bool* lock)
{
    uint16_t test_counter = 0;

    while (true)
    {
#ifdef  __linux
        if (__sync_lock_test_and_set(lock, 1) == 0)
        {
            test_counter = 0;
            return;
        }
#elif   _WIN32
        if (*lock == 0)
        {
            test_counter = 0;
            if (_InterlockedCompareExchange8((volatile char*)lock, 1, 0) == 0)
            {
                return;
            }
        }
#endif
        else
        {
            if ((test_counter & 0xFFFF) == 0xFFFF)
            {
#if FDP_POWER_SAVE == 1
                SLEEP(10);
#endif
            }
            else
            {
                test_counter++;
            }
        }
    }
}

__inline static void ttas_spinlock_unlock(volatile bool* lock)
{
#ifdef  __linux
    __sync_lock_release(lock);
#elif   _WIN32
    *lock = 0;
#endif

    return;
}

__inline static void LockSHM(FDP_SHM_SHARED* FDPShm)
{
    ttas_spinlock_lock(&FDPShm->lock);
}

__inline static void UnlockSHM(FDP_SHM_SHARED* FDPShm)
{
    ttas_spinlock_unlock(&FDPShm->lock);
}

static bool WriteFDPDataWithStatus(FDP_SHM_CANAL* pFDPCanal, uint8_t* pData, uint32_t DataSize, bool bStatus)
{
    bool dataWritten = false;
    if (DataSize > FDP_MAX_DATA_SIZE)
    {
        return false;
    }
    do
    {
        ttas_spinlock_lock(&pFDPCanal->lock);
        if (pFDPCanal->bDataPresent == false)
        {
            memcpy((char*)pFDPCanal->data, pData, DataSize);
            pFDPCanal->bDataPresent = true;
            pFDPCanal->dataSize = DataSize;
            pFDPCanal->bStatus = bStatus;
            dataWritten = true;
        }
        ttas_spinlock_unlock(&pFDPCanal->lock);
    }
    while (dataWritten == false);
    return true;
}

static bool WriteFDPData(FDP_SHM_CANAL* pFDPCanal, uint8_t* pData, uint32_t DataSize)
{
    return WriteFDPDataWithStatus(pFDPCanal, pData, DataSize, true);
}

static uint32_t ReadFDPDataWithStatus(FDP_SHM_CANAL* pFDPCanal, uint8_t* buffer, bool* pbStatus)
{
    bool dataRead = false;
    uint32_t dataReadSize = 0;
    uint32_t readTry = 0;
    do
    {
        if (pFDPCanal->bDataPresent)
        {
            ttas_spinlock_lock(&pFDPCanal->lock);
            if (pFDPCanal->bDataPresent)  //Verification
            {
                if (pFDPCanal->dataSize < FDP_MAX_DATA_SIZE)
                {
                    memcpy(buffer, (char*)pFDPCanal->data, pFDPCanal->dataSize);
                }
                pFDPCanal->bDataPresent = false; //All data is read !
                dataRead = true;
                dataReadSize = pFDPCanal->dataSize;
                *pbStatus = pFDPCanal->bStatus;
                readTry = 0;
            }
            ttas_spinlock_unlock(&pFDPCanal->lock);
        }
        if ((readTry & 0xFFFFFF) == 0xFFFFFF)
        {
#if FDP_POWER_SAVE == 1
            SLEEP(10);
#endif
        }
        else
        {
            readTry++;
        }
    }
    while (dataRead == false);
    return dataReadSize;
}

__inline static uint32_t ReadFDPData(FDP_SHM_CANAL* pFDPCanal, uint8_t* buffer)
{
    bool bIsSuccess;
    return ReadFDPDataWithStatus(pFDPCanal, buffer, &bIsSuccess);
}

FDP_EXPORTED
FDP_SHM* FDP_CreateSHM(char* shmName)
{
    void* pBuf;

#ifdef  __linux
    uint32_t fdSHM;

    /* create the shared memory segment */
    fdSHM = shm_open(shmName, O_CREAT | O_RDWR, 0666);
    if (fdSHM == NULL)
    {
        return NULL;
    }

    /* configure the size of the shared memory segment */
    ftruncate(fdSHM,FDP_SHM_SHARED_SIZE);

    /* now map the shared memory segment in the address space of the process */
    pBuf = mmap(0,FDP_SHM_SHARED_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fdSHM, 0);
    if (pBuf == NULL)
    {
        shm_unlink(shmName);
        return NULL;
    }
#elif   _WIN32
    HANDLE hMapFile;
    void* pBuf;
    hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE,
                                  NULL,
                                  PAGE_READWRITE,
                                  0,
                                  FDP_SHM_SHARED_SIZE,
                                  shmName);
    if (hMapFile == NULL)
    {
        return NULL;
    }
    pBuf = MapViewOfFile(hMapFile,
                         FILE_MAP_ALL_ACCESS,
                         0,
                         0,
                         FDP_SHM_SHARED_SIZE);
    if (pBuf == NULL)
    {
        CloseHandle(hMapFile);
        return NULL;
    }
#endif

    //Clear SHM
    memset(pBuf, 0, FDP_SHM_SHARED_SIZE);
    FDP_SHM* pFDPSHM = (FDP_SHM*)malloc(sizeof(FDP_SHM));
    //TODO: check !
    pFDPSHM->pSharedFDPSHM = (FDP_SHM_SHARED*)pBuf;
    return pFDPSHM;
}


void* OpenShm(const char* pShmName, size_t szShmSize)
{
    void* pBuf;

#ifdef  __linux
    uint32_t fdSHM;

    /* open the shared memory segment */
    fdSHM = shm_open(pShmName, O_RDWR, 0666);
    if (fdSHM == NULL)
    {
        return NULL;
    }

    /* now map the shared memory segment in the address space of the process */
    pBuf = mmap(0, szShmSize, PROT_READ | PROT_WRITE, MAP_SHARED, fdSHM, 0);
    if (pBuf == MAP_FAILED) {
        shm_unlink(pShmName);
        return NULL;
    }
#elif   _WIN32
    HANDLE hMapFile;
    hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS,
                                FALSE,
                                pShmName);
    if (hMapFile == NULL)
    {
        //printf("Failed to OpenFIle... %d\n", GetLastError());
        return NULL;
    }
    pBuf = (LPSTR)MapViewOfFile(hMapFile,
                                FILE_MAP_ALL_ACCESS,
                                0,
                                0,
                                szShmSize);
    if (pBuf == NULL)
    {
        //printf("Failed to MapFileOfFile... %d\n", GetLastError());
        CloseHandle(hMapFile);
        return NULL;
    }
#endif

    return pBuf;
}

FDP_EXPORTED FDP_SHM* FDP_OpenSHM(const char* pShmName)
{
    void* pSharedFDPSHM = OpenShm(pShmName, FDP_SHM_SHARED_SIZE);
    if (pSharedFDPSHM == NULL)
    {
        return NULL;
    }
    //TODO : !
    char aCpuShmName[512];
    strncpy(aCpuShmName, "CPU_", sizeof(aCpuShmName) - 1);
    strncat(aCpuShmName, pShmName, sizeof(aCpuShmName) - strlen(aCpuShmName) - 1);
    
    void* pCpuShm = OpenShm(aCpuShmName, sizeof(FDP_CPU_CTX));
    if (pCpuShm == NULL)
    {
        printf("Failed to OpenShm(%s)\n", aCpuShmName);
        return NULL;
    }
    FDP_SHM* pFDPSHM = (FDP_SHM*)malloc(sizeof(FDP_SHM));
    if (pFDPSHM == NULL)
    {
        //TODO : CloseShm
        return NULL;
    }
    pFDPSHM->pSharedFDPSHM = (FDP_SHM_SHARED*)pSharedFDPSHM;
    pFDPSHM->pCpuShm = (FDP_CPU_CTX*)pCpuShm;
    return pFDPSHM;
}


FDP_EXPORTED
bool FDP_Pause(FDP_SHM* pFDP)
{
    if (pFDP == NULL)
    {
        return false;
    }
    bool bReturnValue = false;
    uint32_t InputBufferSize = 0;
    FDP_SIMPLE_PKT_REQ TempPkt;
    TempPkt.Type = FDPCMD_PAUSE_VM;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, (uint8_t*)&TempPkt, sizeof(TempPkt));
        InputBufferSize = ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)&bReturnValue);
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return bReturnValue;
}

FDP_EXPORTED
bool FDP_Resume(FDP_SHM* pFDP)
{
    if (pFDP == NULL)
    {
        return false;
    }
    bool bReturnValue = false;
    uint32_t InputBufferSize = 0;
    FDP_SIMPLE_PKT_REQ TempPkt;
    TempPkt.Type = FDPCMD_RESUME_VM;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, (uint8_t*)&TempPkt, sizeof(TempPkt));
        InputBufferSize = ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)&bReturnValue);
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return bReturnValue;
}

FDP_EXPORTED
bool FDP_Reboot(FDP_SHM* pFDP)
{
    if (pFDP == NULL)
    {
        return false;
    }
    bool bReturnValue = false;
    uint32_t InputBufferSize = 0;
    FDP_SIMPLE_PKT_REQ TempPkt;
    TempPkt.Type = FDPCMD_REBOOT;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, (uint8_t*)&TempPkt, sizeof(TempPkt));
        InputBufferSize = ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)&bReturnValue);
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return bReturnValue;
}

bool FDP_ReadPhysicalMemoryInternal(FDP_SHM* pFDP, uint8_t* pDstBuffer, uint32_t ReadSize, uint64_t PhysicalAddress)
{
    uint64_t ReceivedSize = 0;
    bool bReturnCode = false;
    FDP_READ_PHYSICAL_MEMORY_PKT_REQ tmpPkt;
    tmpPkt.Type = FDPCMD_READ_PHYSICAL;
    tmpPkt.PhysicalAddress = PhysicalAddress;
    tmpPkt.ReadSize = ReadSize;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, (uint8_t*)&tmpPkt, sizeof(FDP_READ_PHYSICAL_MEMORY_PKT_REQ));
        ReceivedSize = ReadFDPDataWithStatus(&pFDP->pSharedFDPSHM->ServerToClient, pDstBuffer, &bReturnCode);
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return bReturnCode;
}

FDP_EXPORTED
bool FDP_ReadPhysicalMemory(FDP_SHM* pFDP, uint8_t* pDstBuffer, uint32_t ReadSize, uint64_t PhysicalAddress)
{
    if (pFDP == NULL)
    {
        return false;
    }
    uint32_t CurrentOffset = 0;
    do
    {
        uint32_t CurrentReadSize = MIN(ReadSize, FDP_MAX_DATA_SIZE - 1);
        if (FDP_ReadPhysicalMemoryInternal(pFDP, pDstBuffer + CurrentOffset, CurrentReadSize,
                                           PhysicalAddress + CurrentOffset) == false)
        {
            return false;
        }
        CurrentOffset += CurrentReadSize;
    }
    while (CurrentOffset < ReadSize);
    return true;
}

bool FDP_ReadVirtualMemoryInternal(FDP_SHM* pFDP, uint32_t CpuId, uint8_t* pDstBuffer, uint32_t ReadSize,
                                   uint64_t VirtualAddress)
{
    uint32_t ReceivedSize = 0;
    bool bReturnCode = false;
    FDP_READ_VIRTUAL_MEMORY_PKT_REQ tmpPkt;
    tmpPkt.Type = FDPCMD_READ_VIRTUAL;
    tmpPkt.CpuId = CpuId;
    tmpPkt.VirtualAddress = VirtualAddress;
    tmpPkt.ReadSize = ReadSize;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, (uint8_t*)&tmpPkt, sizeof(FDP_READ_PHYSICAL_MEMORY_PKT_REQ));
        ReceivedSize = ReadFDPDataWithStatus(&pFDP->pSharedFDPSHM->ServerToClient, pDstBuffer, &bReturnCode);
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return bReturnCode;
}

FDP_EXPORTED
bool FDP_ReadVirtualMemory(FDP_SHM* pFDP, uint32_t CpuId, uint8_t* pDstBuffer, uint32_t ReadSize,
                           uint64_t VirtualAddress)
{
    if (pFDP == NULL || ReadSize <= 0)
    {
        return false;
    }
    uint32_t CurrentOffset = 0;
    do
    {
        uint32_t CurrentReadSize = MIN(ReadSize, FDP_MAX_DATA_SIZE - 1);
        if (FDP_ReadVirtualMemoryInternal(pFDP, CpuId, pDstBuffer + CurrentOffset, CurrentReadSize,
                                          VirtualAddress + CurrentOffset) == false)
        {
            return false;
        }
        CurrentOffset += CurrentReadSize;
    }
    while (CurrentOffset < ReadSize);
    return true;
}

FDP_EXPORTED
bool FDP_WritePhysicalMemory(FDP_SHM* pFDP, uint8_t* pSrcBuffer, uint32_t WriteSize, uint64_t PhysicalAddress)
{
    if (pFDP == NULL)
    {
        return false;
    }
    uint32_t InputBufferSize = 0;
    bool bReturnValue = false;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        FDP_WRITE_PHYSICAL_MEMORY_PKT_REQ* TempPkt = (FDP_WRITE_PHYSICAL_MEMORY_PKT_REQ*)pFDP->OutputBuffer;
        TempPkt->Type = FDPCMD_WRITE_PHYSICAL;
        TempPkt->PhysicalAddress = PhysicalAddress;
        TempPkt->WriteSize = WriteSize;
        if (WriteSize < FDP_MAX_DATA_SIZE - sizeof(*TempPkt))
        {
            memcpy(TempPkt->Data, pSrcBuffer, WriteSize);
            WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, (uint8_t*)TempPkt,
                         sizeof(FDP_READ_PHYSICAL_MEMORY_PKT_REQ) + WriteSize);
            InputBufferSize = ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)&bReturnValue);
        }
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return bReturnValue;
}


FDP_EXPORTED
bool FDP_WriteVirtualMemory(FDP_SHM* pFDP, uint32_t CpuId, uint8_t* pSrcBuffer, uint32_t WriteSize,
                            uint64_t VirtualAddress)
{
    if (pFDP == NULL)
    {
        return false;
    }
    bool bReturnValue = false;
    uint32_t InputBufferSize = 0;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        FDP_WRITE_VIRTUAL_MEMORY_PKT_REQ* TempPkt = (FDP_WRITE_VIRTUAL_MEMORY_PKT_REQ*)pFDP->OutputBuffer;
        TempPkt->Type = FDPCMD_WRITE_VIRTUAL;
        TempPkt->CpuId = CpuId;
        TempPkt->VirtualAddress = VirtualAddress;
        TempPkt->WriteSize = WriteSize;
        if (WriteSize < FDP_MAX_DATA_SIZE - sizeof(*TempPkt))
        {
            memcpy(TempPkt->Data, pSrcBuffer, WriteSize);
            WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, pFDP->OutputBuffer,
                         sizeof(FDP_WRITE_VIRTUAL_MEMORY_PKT_REQ) + WriteSize);
            InputBufferSize = ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)&bReturnValue);
        }
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return bReturnValue;
}

FDP_EXPORTED
uint64_t FDP_SearchPhysicalMemory(FDP_SHM* pFDP, const void* pPatternData, uint32_t PatternSize, uint64_t StartOffset)
{
    if (pFDP == NULL)
    {
        return false;
    }
    uint64_t FoundAddress = 0x0;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        FDP_SEARCH_PHYSICAL_MEMORY_PKT_REQ* TempPkt = (FDP_SEARCH_PHYSICAL_MEMORY_PKT_REQ*)pFDP->OutputBuffer;
        TempPkt->Type = FDPCMD_SEARCH_PHYSICAL_MEMORY;
        TempPkt->PatternSize = PatternSize;
        TempPkt->StartOffset = StartOffset;
        memcpy(TempPkt->PatternData, pPatternData, PatternSize);
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, pFDP->OutputBuffer,
                     sizeof(FDP_SEARCH_PHYSICAL_MEMORY_PKT_REQ) + PatternSize);
        ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)&FoundAddress); //TODO: return success/fail !
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return FoundAddress;
}

//TODO !
FDP_EXPORTED
bool FDP_SearchVirtualMemory(FDP_SHM* pFDP, uint32_t CpuId, const void* pPatternData, uint32_t PatternSize,
                             uint64_t StartOffset)
{
    if (pFDP == NULL)
    {
        return false;
    }
    uint64_t FoundAddress = 0x0;
    bool bReturnCode = false;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        FDP_SEARCH_VIRTUAL_MEMORY_PKT_REQ* TempPkt = (FDP_SEARCH_VIRTUAL_MEMORY_PKT_REQ*)pFDP->OutputBuffer;
        TempPkt->Type = FDPCMD_SEARCH_VIRTUAL_MEMORY;
        TempPkt->CpuId = CpuId;
        TempPkt->PatternSize = PatternSize;
        TempPkt->StartOffset = StartOffset;
        memcpy(TempPkt->PatternData, pPatternData, PatternSize);
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, pFDP->OutputBuffer,
                     sizeof(FDP_SEARCH_VIRTUAL_MEMORY_PKT_REQ) + PatternSize);
        ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)&FoundAddress); //TODO: return success/fail !
        bReturnCode = true;
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return bReturnCode;
}

FDP_EXPORTED
bool FDP_ReadRegister(FDP_SHM* pFDP, uint32_t CpuId, FDP_Register RegisterId, uint64_t* pRegisterValue)
{
    if (pFDP == NULL)
    {
        return false;
    }
    //Fast way...
    switch (RegisterId)
    {
    case FDP_RIP_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->rip;
        return true;
    case FDP_RAX_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->rax;
        return true;
    case FDP_RCX_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->rcx;
        return true;
    case FDP_RDX_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->rdx;
        return true;
    case FDP_RBX_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->rbx;
        return true;
    case FDP_RSP_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->rsp;
        return true;
    case FDP_RBP_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->rbp;
        return true;
    case FDP_RSI_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->rsi;
        return true;
    case FDP_RDI_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->rdi;
        return true;
    case FDP_R8_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->r8;
        return true;
    case FDP_R9_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->r9;
        return true;
    case FDP_R10_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->r10;
        return true;
    case FDP_R11_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->r11;
        return true;
    case FDP_R12_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->r12;
        return true;
    case FDP_R13_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->r13;
        return true;
    case FDP_R14_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->r14;
        return true;
    case FDP_R15_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->r15;
        return true;
    case FDP_CR0_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->cr0;
        return true;
    case FDP_CR2_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->cr2;
        return true;
    case FDP_CR3_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->cr3;
        return true;
    case FDP_CR4_REGISTER:
        *pRegisterValue = pFDP->pCpuShm->cr4;
        return true;
    default:
        break;
    }
    //Old version => low performance
    FDP_READ_REGISTER_PKT_REQ TempPkt;
    TempPkt.Type = FDPCMD_READ_REGISTER;
    TempPkt.CpuId = CpuId;
    TempPkt.RegisterId = RegisterId;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, (uint8_t*)&TempPkt, sizeof(FDP_READ_REGISTER_PKT_REQ));
        ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)pRegisterValue); //TODO: return success/fail !
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return true;
}

FDP_EXPORTED
bool FDP_ReadMsr(FDP_SHM* pFDP, uint32_t CpuId, uint64_t MsrId, uint64_t* pMsrValue)
{
    if (pFDP == NULL)
    {
        return false;
    }
    FDP_READ_MSR_PKT_REQ TempPkt;
    TempPkt.Type = FDPCMD_READ_MSR;
    TempPkt.CpuId = CpuId;
    TempPkt.MsrId = MsrId;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, (uint8_t*)&TempPkt, sizeof(FDP_READ_MSR_PKT_REQ));
        ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)pMsrValue); //TODO: return success/fail !
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return true;
}

FDP_EXPORTED
bool FDP_WriteMsr(FDP_SHM* pFDP, uint32_t CpuId, uint64_t MsrId, uint64_t MsrValue)
{
    if (pFDP == NULL)
    {
        return false;
    }
    bool bReturnValue = false;
    uint32_t InputBufferSize = 0;
    FDP_WRITE_MSR_PKT_REQ TempPkt;
    TempPkt.Type = FDPCMD_WRITE_MSR;
    TempPkt.CpuId = CpuId;
    TempPkt.MsrId = MsrId;
    TempPkt.MsrValue = MsrValue;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, (uint8_t*)&TempPkt, sizeof(FDP_WRITE_MSR_PKT_REQ));
        InputBufferSize = ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)&bReturnValue);
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return bReturnValue;
}

FDP_EXPORTED
bool FDP_WriteRegister(FDP_SHM* pFDP, uint32_t CpuId, FDP_Register RegisterId, uint64_t RegisterValue)
{
    if (pFDP == NULL)
    {
        return false;
    }
    uint32_t InputBufferSize = 0;
    bool bReturnValue = false;
    FDP_WRITE_REGISTER_PKT_REQ TempPkt;
    TempPkt.Type = FDPCMD_WRITE_REGISTER;
    TempPkt.CpuId = CpuId;
    TempPkt.RegisterId = RegisterId;
    TempPkt.RegisterValue = RegisterValue;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, (uint8_t*)&TempPkt, sizeof(FDP_WRITE_REGISTER_PKT_REQ));
        InputBufferSize = ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)&bReturnValue);
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return bReturnValue;
}


FDP_EXPORTED
bool FDP_UnsetBreakpoint(FDP_SHM* pFDP, uint8_t BreakpointId)
{
    if (pFDP == NULL)
    {
        return false;
    }
    uint32_t InputBufferSize = 0;
    bool bReturnValue = false;
    FDP_CLEAR_BREAKPOINT_PKT_REQ TempPkt;
    TempPkt.Type = FDPCMD_UNSET_BP;
    TempPkt.BreakpointId = BreakpointId;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, (uint8_t*)&TempPkt, sizeof(FDP_CLEAR_BREAKPOINT_PKT_REQ));
        InputBufferSize = ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)&bReturnValue);
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return bReturnValue;
}


FDP_EXPORTED
int FDP_SetBreakpoint(
    FDP_SHM*             pFDP,
    uint32_t            CpuId,
    FDP_BreakpointType  BreakpointType,
    uint8_t             BreakpointId,
    FDP_Access          BreakpointAccessType,
    FDP_AddressType     BreakpointAddressType,
    uint64_t            BreakpointAddress,
    uint64_t            BreakpointLength,
    uint64_t            BreakpointCr3)
{
    if (pFDP == NULL)
    {
        return false;
    }
    uint32_t InputBufferSize = 0;
    int iReturnedBreakpointId;
    FDP_SET_BREAKPOINT_PKT_REQ TempPkt;
    TempPkt.Type = FDPCMD_SET_BP;
    TempPkt.CpuId = CpuId;
    TempPkt.BreakpointType = BreakpointType;
    TempPkt.BreakpointId = BreakpointId;
    TempPkt.BreakpointAccessType = BreakpointAccessType;
    TempPkt.BreakpointAddressType = BreakpointAddressType;
    TempPkt.BreakpointAddress = BreakpointAddress;
    TempPkt.BreakpointLength = BreakpointLength;
    TempPkt.BreakpointCr3 = BreakpointCr3;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, (uint8_t*)&TempPkt, sizeof(FDP_SET_BREAKPOINT_PKT_REQ));
        InputBufferSize = ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)&iReturnedBreakpointId);
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return iReturnedBreakpointId;
}

FDP_EXPORTED
bool FDP_VirtualToPhysical(FDP_SHM* pFDP, uint32_t CpuId, uint64_t VirtualAddress, uint64_t* PhysicalAddress)
{
    if (pFDP == NULL)
    {
        return false;
    }
    FDP_VIRTUAL_PHYSICAL_PKT_REQ TempPkt;
    TempPkt.Type = FDPCMD_VIRTUAL_PHYSICAL;
    TempPkt.CpuId = CpuId;
    TempPkt.VirtualAddress = VirtualAddress;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, (uint8_t*)&TempPkt, sizeof(FDP_VIRTUAL_PHYSICAL_PKT_REQ));
        ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)PhysicalAddress); //TODO: return success/fail !
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return true;
}

FDP_EXPORTED
bool FDP_GetState(FDP_SHM* pFDP, FDP_State* DebuggeeState)
{
    if (pFDP == NULL)
    {
        return false;
    }
    FDP_GET_STATE_PKT_REQ TempPkt;
    TempPkt.Type = FDPCMD_GET_STATE;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, (uint8_t*)&TempPkt, sizeof(FDP_GET_STATE_PKT_REQ));
        ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)DebuggeeState); //TODO: return success/fail !
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return true;
}

FDP_EXPORTED
bool FDP_WaitForStateChanged(FDP_SHM *pFDP, FDP_State *DebuggeeState)
{
    if (pFDP == NULL)
    {
        return false;
    }
    while (true)
    {
        SLEEP(0);
        if (FDP_GetStateChanged(pFDP) == true)
        {
            return FDP_GetState(pFDP, DebuggeeState);
        }
    }
    return true;
}

FDP_EXPORTED
bool FDP_Init(FDP_SHM* pFDP)
{
    if (pFDP == NULL)
    {
        return false;
    }
    bool bReturnValue = true;
    memset(pFDP->pSharedFDPSHM, 0, FDP_SHM_SHARED_SIZE);
    return bReturnValue;
}

FDP_EXPORTED
bool FDP_SingleStep(FDP_SHM* pFDP, uint32_t CpuId)
{
    if (pFDP == NULL)
    {
        return false;
    }
    bool bReturnValue = false;
    uint32_t InputBufferSize = 0;
    FDP_SINGLE_STEP_PKT_REQ TempPkt;
    TempPkt.Type = FDPCMD_SINGLE_STEP;
    TempPkt.CpuId = CpuId;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, (uint8_t*)&TempPkt, sizeof(FDP_SINGLE_STEP_PKT_REQ));
        InputBufferSize = ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)&bReturnValue);
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return bReturnValue;
}

uint8_t FDP_Test(FDP_SHM* pFDP)
{
    if (pFDP == NULL)
    {
        return false;
    }
    uint8_t DebuggeState;
    FDP_GET_STATE_PKT_REQ* tmpPkt = (FDP_GET_STATE_PKT_REQ*)pFDP->OutputBuffer;
    tmpPkt->Type = FDPCMD_TEST;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, pFDP->OutputBuffer, sizeof(FDP_GET_STATE_PKT_REQ));
        ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)&DebuggeState); //TODO: return success/fail !
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return DebuggeState;
}


FDP_EXPORTED
bool FDP_GetFxState64(FDP_SHM* pFDP, uint32_t CpuId, FDP_XSAVE_FORMAT64_T* pFxState)
{
    if (pFDP == NULL)
    {
        return false;
    }
    FDP_GET_STATE_PKT_REQ* TempPkt = (FDP_GET_STATE_PKT_REQ*)pFDP->OutputBuffer;
    TempPkt->Type = FDPCMD_GET_FXSTATE;
    TempPkt->CpuId = CpuId;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, pFDP->OutputBuffer, sizeof(FDP_GET_STATE_PKT_REQ));
        ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)pFxState); //TODO: return success/fail !
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return true;
}

FDP_EXPORTED
bool FDP_SetFxState64(FDP_SHM* pFDP, uint32_t CpuId, FDP_XSAVE_FORMAT64_T* pFxState64)
{
    if (pFDP == NULL)
    {
        return false;
    }
    bool bReturnValue = false;
    FDP_SET_FX_STATE_REQ* TempPkt = (FDP_SET_FX_STATE_REQ*)pFDP->OutputBuffer;
    TempPkt->Type = FDPCMD_SET_FXSTATE;
    TempPkt->CpuId = CpuId;
    memcpy(&TempPkt->FxState64, pFxState64, sizeof(FDP_XSAVE_FORMAT64_T));
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, pFDP->OutputBuffer, sizeof(FDP_SET_FX_STATE_REQ));
        ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)&bReturnValue); //TODO: return success/fail !
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return bReturnValue;
}


FDP_EXPORTED
bool FDP_GetPhysicalMemorySize(FDP_SHM* pFDP, uint64_t* PhysicalMemorySize)
{
    if (pFDP == NULL)
    {
        return false;
    }
    bool bReturnValue = true;
    uint32_t InputBufferSize = 0;
    FDP_SIMPLE_PKT_REQ TempPkt;
    TempPkt.Type = FDPCMD_GET_MEMORYSIZE;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, (uint8_t*)&TempPkt, sizeof(TempPkt));
        InputBufferSize = ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)PhysicalMemorySize);
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    //TODO return bool !
    return bReturnValue;
}


FDP_EXPORTED
bool FDP_GetCpuCount(FDP_SHM* pFDP, uint32_t* CPUCount)
{
    if (pFDP == NULL)
    {
        return false;
    }
    bool bReturnValue = true;
    uint32_t InputBufferSize = 0;
    FDP_SIMPLE_PKT_REQ TempPkt;
    TempPkt.Type = FDPCMD_GET_CPU_COUNT;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, (uint8_t*)&TempPkt, sizeof(TempPkt));
        InputBufferSize = ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)CPUCount);
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return bReturnValue;
}

FDP_EXPORTED
bool FDP_GetCpuState(FDP_SHM* pFDP, uint32_t CpuId, FDP_State* pDebuggeeState)
{
    if (pFDP == NULL)
    {
        return false;
    }
    FDP_GET_CPU_STATE_PKT_REQ TempPkt;
    TempPkt.Type = FDPCMD_GET_CPU_STATE;
    TempPkt.CpuId = CpuId;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, (uint8_t*)&TempPkt, sizeof(FDP_GET_CPU_STATE_PKT_REQ));
        ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)pDebuggeeState); //TODO: return success/fail !
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return true;
}

FDP_EXPORTED
bool FDP_Save(FDP_SHM* pFDP)
{
    if (pFDP == NULL)
    {
        return false;
    }
    bool bReturnValue = false;
    FDP_SIMPLE_PKT_REQ TempPkt;
    TempPkt.Type = FDPCMD_SAVE;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, (uint8_t*)&TempPkt, sizeof(TempPkt));
        ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)&bReturnValue); //TODO: return success/fail !
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return bReturnValue;
}

FDP_EXPORTED
bool FDP_Restore(FDP_SHM* pFDP)
{
    if (pFDP == NULL)
    {
        return false;
    }
    bool bReturnValue = false;
    FDP_SIMPLE_PKT_REQ TempPkt;
    TempPkt.Type = FDPCMD_RESTORE;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, (uint8_t*)&TempPkt, sizeof(TempPkt));
        ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)&bReturnValue); //TODO: return success/fail !
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return bReturnValue;
}

FDP_EXPORTED
bool FDP_GetStateChanged(FDP_SHM* pFDP)
{
    if (pFDP == NULL)
    {
        return false;
    }
    bool StateChanged;
    //LockSHM(pFDP->pSharedFDPSHM);
    ttas_spinlock_lock(&pFDP->pSharedFDPSHM->stateChangedLock);
    {
        StateChanged = pFDP->pSharedFDPSHM->stateChanged;
        pFDP->pSharedFDPSHM->stateChanged = false;
    }
    //UnlockSHM(pFDP->pSharedFDPSHM);
    ttas_spinlock_unlock(&pFDP->pSharedFDPSHM->stateChangedLock);
    return StateChanged;
}

FDP_EXPORTED
void FDP_SetStateChanged(FDP_SHM* pFDP)
{
    if (pFDP == NULL)
    {
        return;
    }
    //LockSHM(pFDP->pSharedFDPSHM);
    ttas_spinlock_lock(&pFDP->pSharedFDPSHM->stateChangedLock);
    {
        pFDP->pSharedFDPSHM->stateChanged = true;
    }
    //UnlockSHM(pFDP->pSharedFDPSHM);
    ttas_spinlock_unlock(&pFDP->pSharedFDPSHM->stateChangedLock);
    return;
}

FDP_EXPORTED
bool FDP_InjectInterrupt(FDP_SHM* pFDP, uint32_t CpuId, uint32_t uInterruptionCode, uint32_t uErrorCode,
                         uint64_t Cr2Value)
{
    if (pFDP == NULL)
    {
        return false;
    }
    bool bReturnValue = false;
    FDP_INJECT_INTERRUPT_PKT_REQ* tmpPkt = (FDP_INJECT_INTERRUPT_PKT_REQ*)pFDP->OutputBuffer;
    tmpPkt->Type = FDPCMD_INJECT_INTERRUPT;
    tmpPkt->CpuId = CpuId;
    tmpPkt->Cr2Value = Cr2Value;
    tmpPkt->ErrorCode = uErrorCode;
    tmpPkt->InterruptionCode = uInterruptionCode;
    LockSHM(pFDP->pSharedFDPSHM);
    {
        WriteFDPData(&pFDP->pSharedFDPSHM->ClientToServer, pFDP->OutputBuffer, sizeof(FDP_INJECT_INTERRUPT_PKT_REQ));
        ReadFDPData(&pFDP->pSharedFDPSHM->ServerToClient, (uint8_t*)&bReturnValue); //TODO: return success/fail !
    }
    UnlockSHM(pFDP->pSharedFDPSHM);
    return bReturnValue;
}



//Server Part
FDP_EXPORTED
bool FDP_ServerLoop(FDP_SHM* pFDP)
{
    if (pFDP == NULL)
    {
        return false;
    }
    uint32_t u32InputBufferSize = 0;
    uint32_t u32OutputBuffersize = 0;
    pFDP->pFdpServer->bIsRunning = true;
    while (pFDP->pFdpServer->bIsRunning)
    {
        bool bStatus = true;
        u32InputBufferSize = ReadFDPData(&pFDP->pSharedFDPSHM->ClientToServer, pFDP->InputBuffer);
        if (u32InputBufferSize == 0)
        {
            return false;
        }
        uint8_t Type = pFDP->InputBuffer[0];
        switch (Type)
        {
        case FDPCMD_TEST:
        {
            pFDP->OutputBuffer[0] = 0; //TODO: true !
            u32OutputBuffersize = 1;
            break;
        }
        case FDPCMD_SAVE:
        {
            pFDP->OutputBuffer[0] = pFDP->pFdpServer->pfnSave(pFDP->pFdpServer->pUserHandle);
            u32OutputBuffersize = 1;
            break;
        }
        case FDPCMD_RESTORE:
        {
            pFDP->OutputBuffer[0] = pFDP->pFdpServer->pfnRestore(pFDP->pFdpServer->pUserHandle);
            u32OutputBuffersize = 1;
            break;
        }
        case FDPCMD_REBOOT:
        {
            pFDP->OutputBuffer[0] = pFDP->pFdpServer->pfnReboot(pFDP->pFdpServer->pUserHandle);
            u32OutputBuffersize = 1;
            break;
        }
        case FDPCMD_GET_CPU_COUNT:
        {
            uint32_t CpuCout;
            pFDP->pFdpServer->pfnGetCpuCount(pFDP->pFdpServer->pUserHandle, &CpuCout);
            ((uint32_t*)pFDP->OutputBuffer)[0] = CpuCout;
            u32OutputBuffersize = sizeof(CpuCout);
            break;
        }
        case FDPCMD_GET_STATE:
        {
            uint8_t CurrentState;
            pFDP->pFdpServer->pfnGetState(pFDP->pFdpServer->pUserHandle, &CurrentState);
            pFDP->OutputBuffer[0] = CurrentState;
            u32OutputBuffersize = sizeof(CurrentState);
            break;
        }
        case FDPCMD_GET_CPU_STATE:
        {
            uint8_t CurrentState = 0;
            FDP_GET_STATE_PKT_REQ* TempPkt = (FDP_GET_STATE_PKT_REQ*)pFDP->InputBuffer;
            pFDP->pFdpServer->pfnGetCpuState(pFDP->pFdpServer->pUserHandle, TempPkt->CpuId, &CurrentState);
            pFDP->OutputBuffer[0] = CurrentState;
            u32OutputBuffersize = sizeof(CurrentState);
            break;
        }
        case FDPCMD_GET_MEMORYSIZE:
        {
            uint64_t u64PhysicalMemorySize;
            pFDP->pFdpServer->pfnGetMemorySize(pFDP->pFdpServer->pUserHandle, &u64PhysicalMemorySize);
            ((uint64_t*)pFDP->OutputBuffer)[0] = u64PhysicalMemorySize;
            u32OutputBuffersize = sizeof(u64PhysicalMemorySize);
            break;
        }
        case FDPCMD_UNSET_BP:
        {
            FDP_CLEAR_BREAKPOINT_PKT_REQ* TempPkt = (FDP_CLEAR_BREAKPOINT_PKT_REQ*)pFDP->InputBuffer;
            pFDP->OutputBuffer[0] = pFDP->pFdpServer->pfnUnsetBreakpoint(pFDP->pFdpServer->pUserHandle, TempPkt->BreakpointId);
            u32OutputBuffersize = 1;
            break;
        }
        case FDPCMD_SET_BP:
        {
            FDP_SET_BREAKPOINT_PKT_REQ* TempPkt = (FDP_SET_BREAKPOINT_PKT_REQ*)pFDP->InputBuffer;
            ((int*)pFDP->OutputBuffer)[0] = pFDP->pFdpServer->pfnSetBreakpoint(pFDP->pFdpServer->pUserHandle,
                                            TempPkt->CpuId,
                                            TempPkt->BreakpointType,
                                            TempPkt->BreakpointId,
                                            TempPkt->BreakpointAccessType,
                                            TempPkt->BreakpointAddressType,
                                            TempPkt->BreakpointAddress,
                                            TempPkt->BreakpointLength,
                                            TempPkt->BreakpointCr3);
            u32OutputBuffersize = sizeof(int);
            break;
        }
        case FDPCMD_VIRTUAL_PHYSICAL:
        {
            uint64_t PhysicalAddress = 0;
            FDP_VIRTUAL_PHYSICAL_PKT_REQ* TempPkt = (FDP_VIRTUAL_PHYSICAL_PKT_REQ*)pFDP->InputBuffer;
            pFDP->pFdpServer->pfnVirtualToPhysical(pFDP->pFdpServer->pUserHandle,
                                                   TempPkt->CpuId,
                                                   TempPkt->VirtualAddress,
                                                   &PhysicalAddress);
            ((uint64_t*)pFDP->OutputBuffer)[0] = PhysicalAddress;
            u32OutputBuffersize = sizeof(PhysicalAddress);
            break;
        }
        case FDPCMD_RESUME_VM:
            pFDP->OutputBuffer[0] = pFDP->pFdpServer->pfnResume(pFDP->pFdpServer->pUserHandle);
            u32OutputBuffersize = sizeof(bool);
            break;
        case FDPCMD_PAUSE_VM:
            pFDP->OutputBuffer[0] = pFDP->pFdpServer->pfnPause(pFDP->pFdpServer->pUserHandle);
            u32OutputBuffersize = sizeof(bool);
            break;
        case FDPCMD_SINGLE_STEP:
        {
            FDP_GET_STATE_PKT_REQ* TempPkt = (FDP_GET_STATE_PKT_REQ*)pFDP->InputBuffer;
            pFDP->OutputBuffer[0] = pFDP->pFdpServer->pfnSingleStep(pFDP->pFdpServer->pUserHandle, TempPkt->CpuId);
            u32OutputBuffersize = sizeof(bool);
            break;
        }
        case FDPCMD_READ_REGISTER:
        {
            uint64_t RegisterValue = 0;
            FDP_READ_REGISTER_PKT_REQ* TempPkt = (FDP_READ_REGISTER_PKT_REQ*)pFDP->InputBuffer;
            pFDP->pFdpServer->pfnReadRegister(pFDP->pFdpServer->pUserHandle,
                                              TempPkt->CpuId,
                                              TempPkt->RegisterId,
                                              &RegisterValue);
            ((uint64_t*)pFDP->OutputBuffer)[0] = RegisterValue;
            u32OutputBuffersize = sizeof(RegisterValue);
            break;
        }
        case FDPCMD_GET_FXSTATE:
        {
            FDP_GET_STATE_PKT_REQ* TempPkt = (FDP_GET_STATE_PKT_REQ*)pFDP->InputBuffer;
            pFDP->pFdpServer->pfnGetFxState64(pFDP->pFdpServer->pUserHandle,
                                              TempPkt->CpuId,
                                              pFDP->OutputBuffer,
                                              &u32OutputBuffersize);
            break;
        }
        case FDPCMD_SET_FXSTATE:
        {
            FDP_SET_FX_STATE_REQ* TempPkt = (FDP_SET_FX_STATE_REQ*)pFDP->InputBuffer;
            pFDP->OutputBuffer[0] = pFDP->pFdpServer->pfnSetFxState64(pFDP->pFdpServer->pUserHandle,
                                    TempPkt->CpuId,
                                    (uint8_t*)&TempPkt->FxState64,
                                    sizeof(FDP_XSAVE_FORMAT64_T));
            u32OutputBuffersize = sizeof(bool);
            break;
        }
        case FDPCMD_READ_MSR:
        {
            uint64_t MsrValue = 0;
            FDP_READ_MSR_PKT_REQ* TempPkt = (FDP_READ_MSR_PKT_REQ*)pFDP->InputBuffer;
            pFDP->pFdpServer->pfnReadMsr(pFDP->pFdpServer->pUserHandle,
                                         TempPkt->CpuId,
                                         TempPkt->MsrId,
                                         &MsrValue);
            ((uint64_t*)pFDP->OutputBuffer)[0] = MsrValue;
            u32OutputBuffersize = sizeof(uint64_t);
            break;
        }
        case FDPCMD_WRITE_MSR:
        {
            FDP_WRITE_MSR_PKT_REQ* TempPkt = (FDP_WRITE_MSR_PKT_REQ*)pFDP->InputBuffer;
            pFDP->OutputBuffer[0] = pFDP->pFdpServer->pfnWriteMsr(pFDP->pFdpServer->pUserHandle,
                                    TempPkt->CpuId,
                                    TempPkt->MsrId,
                                    TempPkt->MsrValue);
            u32OutputBuffersize = sizeof(bool);
            break;
        }
        case FDPCMD_WRITE_REGISTER:
        {
            FDP_WRITE_REGISTER_PKT_REQ* TempPkt = (FDP_WRITE_REGISTER_PKT_REQ*)pFDP->InputBuffer;
            pFDP->OutputBuffer[0] = pFDP->pFdpServer->pfnWriteRegister(pFDP->pFdpServer->pUserHandle,
                                    TempPkt->CpuId,
                                    TempPkt->RegisterId,
                                    TempPkt->RegisterValue);
            u32OutputBuffersize = sizeof(bool);
            break;
        }
        case FDPCMD_READ_PHYSICAL:
        {
            FDP_READ_PHYSICAL_MEMORY_PKT_REQ* TempPkt = (FDP_READ_PHYSICAL_MEMORY_PKT_REQ*)pFDP->InputBuffer;
            if (TempPkt->ReadSize > FDP_MAX_DATA_SIZE)
            {
                bStatus = false;
            }
            else
            {
                bStatus = pFDP->pFdpServer->pfnReadPhysicalMemory(pFDP->pFdpServer->pUserHandle,
                          pFDP->OutputBuffer,
                          TempPkt->PhysicalAddress,
                          TempPkt->ReadSize);
            }
            if (bStatus)
            {
                u32OutputBuffersize = TempPkt->ReadSize;
            }
            else
            {
                u32OutputBuffersize = 1;
            }
            break;
        }
        case FDPCMD_READ_VIRTUAL:
        {
            FDP_READ_VIRTUAL_MEMORY_PKT_REQ* TempPkt = (FDP_READ_VIRTUAL_MEMORY_PKT_REQ*)pFDP->InputBuffer;
            if (TempPkt->ReadSize > FDP_MAX_DATA_SIZE)
            {
                bStatus = false;
            }
            else
            {
                bStatus = pFDP->pFdpServer->pfnReadVirtualMemory(pFDP->pFdpServer->pUserHandle,
                          TempPkt->CpuId,
                          TempPkt->VirtualAddress,
                          TempPkt->ReadSize,
                          pFDP->OutputBuffer);
            }
            if (bStatus)
            {
                u32OutputBuffersize = TempPkt->ReadSize;
            }
            else
            {
                u32OutputBuffersize = 1;
            }
            break;
        }
        case FDPCMD_WRITE_PHYSICAL:
        {
            FDP_WRITE_PHYSICAL_MEMORY_PKT_REQ* TempPkt = (FDP_WRITE_PHYSICAL_MEMORY_PKT_REQ*)pFDP->InputBuffer;
            pFDP->OutputBuffer[0] = pFDP->pFdpServer->pfnWritePhysicalMemory(pFDP->pFdpServer->pUserHandle,
                                    TempPkt->Data,
                                    TempPkt->PhysicalAddress,
                                    TempPkt->WriteSize);
            u32OutputBuffersize = sizeof(bool);
            break;
        }
        case FDPCMD_WRITE_VIRTUAL:
        {
            FDP_WRITE_VIRTUAL_MEMORY_PKT_REQ* TempPkt = (FDP_WRITE_VIRTUAL_MEMORY_PKT_REQ*)pFDP->InputBuffer;
            pFDP->OutputBuffer[0] = pFDP->OutputBuffer[0] = pFDP->pFdpServer->pfnWriteVirtualMemory(
                                        pFDP->pFdpServer->pUserHandle,
                                        TempPkt->CpuId,
                                        TempPkt->Data,
                                        TempPkt->VirtualAddress,
                                        TempPkt->WriteSize);
            u32OutputBuffersize = sizeof(bool);
            break;
        }
        case FDPCMD_INJECT_INTERRUPT:
        {
            FDP_INJECT_INTERRUPT_PKT_REQ* TempPkt = (FDP_INJECT_INTERRUPT_PKT_REQ*)pFDP->InputBuffer;
            pFDP->OutputBuffer[0] = pFDP->OutputBuffer[0] = pFDP->pFdpServer->pfnInjectInterrupt(
                                        pFDP->pFdpServer->pUserHandle,
                                        TempPkt->CpuId,
                                        TempPkt->InterruptionCode,
                                        TempPkt->ErrorCode,
                                        TempPkt->Cr2Value
                                    );
            u32OutputBuffersize = sizeof(bool);
            break;
        }
        //TODO !
        case FDPCMD_SEARCH_PHYSICAL_MEMORY:
        {
            /*FDP_SEARCH_PHYSICAL_MEMORY_PKT_REQ* tmpPkt = (FDP_SEARCH_PHYSICAL_MEMORY_PKT_REQ*)pFDP->InputBuffer;

            ((uint64_t*)pFDP->OutputBuffer)[0] = pFDP->pFdpServer->pfnSear

            ((uint64_t*)myFDPHandle.OutputBuffer)[0] = -1;
            if (tmpPkt->StartOffset < MMR3PhysGetRamSizeU(pUVM)){
            int rc = PGMR3DbgScanPhysicalU(pUVM, tmpPkt->StartOffset, MMR3PhysGetRamSizeU(pUVM) - tmpPkt->StartOffset, 1, tmpPkt->PatternData, tmpPkt->PatternSize, &HitAddress);
            ((uint64_t*)myFDPHandle.OutputBuffer)[0] = HitAddress;
            if (RT_FAILURE(rc)){
            ((uint64_t*)myFDPHandle.OutputBuffer)[0] = -1;

            }

            }
            myFDPHandle.OutputBufferSize = sizeof(uint64_t);
            */
            break;
        }
        default:
            break;
        }
        //There is something to send !
        if (u32OutputBuffersize > 0)
        {
            WriteFDPDataWithStatus(&pFDP->pSharedFDPSHM->ServerToClient, pFDP->OutputBuffer, u32OutputBuffersize, bStatus);
            u32OutputBuffersize = 0;
        }
    }
    return true;
}

FDP_EXPORTED
bool FDP_SetFDPServer(FDP_SHM* pFDP, FDP_SERVER_INTERFACE_T* pFDPServer)
{
    if (pFDP == NULL)
    {
        return false;
    }
    pFDP->pFdpServer = pFDPServer;
    return true;
}


//TODO ! Unit Tests

bool FDP_DummyReadRegister(void* pUserHandle, uint32_t u32CpuId, FDP_Register u8RegisterId, uint64_t* pRegisterValue)
{
    *pRegisterValue = 0xDEADDEADDEADDEAD;
    return true;
}

bool FDP_DummyWriteRegister(void* pUserHandle, uint32_t u32CpuId, FDP_Register u8RegisterId, uint64_t pRegisterValue)
{
    return true;
}

bool FDP_DummyGetCpuCount(void* pUserHandle, uint32_t* pCpuCount)
{
    *pCpuCount = 0x42;
    return true;
}


#ifdef  __linux
void * FDP_UnitTestClient(void *lpParameter)
#elif   _WIN32
DWORD WINAPI FDP_UnitTestClient(_In_ LPVOID lpParameter)
#endif
{
    FDP_SHM* pFDPClient = (FDP_SHM*)lpParameter;
    //Waiting for FDPServer star
    while (pFDPClient->pFdpServer->bIsRunning == false)
    {
        printf(".");
    }
    uint64_t RaxRegisterValue;
    FDP_ReadRegister(pFDPClient, 0, FDP_RAX_REGISTER, &RaxRegisterValue);
    FDP_WriteRegister(pFDPClient, 0, FDP_RAX_REGISTER, 0xCAFECAFECAFECAFE);
    pFDPClient->pFdpServer->bIsRunning = false;
    uint32_t u32CpuCount;
    FDP_GetCpuCount(pFDPClient, &u32CpuCount);
    return 0;
}

bool FDP_ClientServerTest()
{
    //Building FDP Server Interface
    FDP_SERVER_INTERFACE_T FDPServerInterface;
    //FDPServerInterface.bIsRunning = true;
    FDPServerInterface.pUserHandle = NULL;
    FDPServerInterface.pfnReadRegister = FDP_DummyReadRegister;
    FDPServerInterface.pfnWriteRegister = FDP_DummyWriteRegister;
    FDPServerInterface.pfnGetCpuCount = FDP_DummyGetCpuCount;
    FDP_SHM* pFDPServer = FDP_CreateSHM("FDP_TEST");
    if (pFDPServer == NULL)
    {
        printf("Failed to FDP_CreateSHM\n");
        return false;
    }
    if (FDP_SetFDPServer(pFDPServer, &FDPServerInterface) == false)
    {
        printf("Failed to FDP_SerFDPServer\n");
        return false;
    }

    //Create a fake Client...
#ifdef  __linux
    int rc;
    void *ret;
    pthread_t threadServer;
    if(pthread_create(&threadServer, NULL, FDP_UnitTestClient, pFDPServer)){
         printf("Failed create thread\n");
         return false;
    }

#elif   _WIN32
    HANDLE hThreadServer = INVALID_HANDLE_VALUE;
    hThreadServer = CreateThread(NULL, 0, FDP_UnitTestClient, pFDPServer, 0, 0);
    if (hThreadServer == INVALID_HANDLE_VALUE)
    {
        printf("Failed to CreateThread\n");
        return false;
    }
#endif

    if (FDP_ServerLoop(pFDPServer) == false)
    {
        printf("Failed to FDP_ServerLoop\n");
        return false;
    }
    //Clean:
    //Closing server
    FDPServerInterface.bIsRunning = false;
#ifdef  __linux
    pthread_join(threadServer, &ret);
#elif   _WIN32
    WaitForSingleObject(hThreadServer, INFINITE);
#endif
    return true;
}
