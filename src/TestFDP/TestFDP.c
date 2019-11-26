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
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <inttypes.h>

#ifdef  _MSC_VER
#include <Windows.h>

#define SLEEP(X) do { Sleep(X); } while(0)
#define THREAD_EXIT(X) do { TerminateThread((X), 0); } while(0)
#else
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define SLEEP(X) do { usleep(X*1000); } while(0)
#define THREAD_EXIT(X) do { pthread_exit(&(X)); } while(0)
#endif

#include "utils.h"
#include "FDP.h"

int iTimerDelay = 2;
bool TimerGo = false;
bool TimerOut = false;

void TimerSetDelay(int iNewTimerDelay)
{
    iTimerDelay = iNewTimerDelay;
}

int TimerGetDelay()
{
    return iTimerDelay;
}

#ifdef  _MSC_VER
DWORD WINAPI TimerRoutine(LPVOID lpParam)
#else
void * TimerRoutine(void *lpParam)
#endif
{
    while (true){
        while (TimerGo == false){
            SLEEP(10);
        }
        TimerGo = false;
        SLEEP(iTimerDelay*1000);
        TimerOut = true;
    }
}

#define TEST_MSR_VALUE 0xFFFFFFFFFFFFFFFF
bool testReadWriteMSR(FDP_SHM* pFDP)
{
    printf("%s ...", __FUNCTION__);
    FDP_Pause(pFDP);
    uint64_t originalMSRValue;
    uint64_t modMSRValue;
    if (FDP_ReadMsr(pFDP, 0, MSR_LSTAR, &originalMSRValue) == false){
        printf("Failed to read MSRValue !\n");
        return false;
    }

    if (FDP_WriteMsr(pFDP, 0, MSR_LSTAR, TEST_MSR_VALUE) == false){
        printf("Failed to write MSRValue !\n");
        return false;
    }

    if (FDP_ReadMsr(pFDP, 0, MSR_LSTAR, &modMSRValue) == false){
        printf("Failed to read MSRValue !\n");
        return false;
    }

    if (modMSRValue != TEST_MSR_VALUE){
        printf("MSRValue doesn't match !\n");
        return false;
    }

    if (FDP_WriteMsr(pFDP, 0, MSR_LSTAR, originalMSRValue) == false){
        printf("Failed to write MSRValue !\n");
        return false;
    }

    if (FDP_ReadMsr(pFDP, 0, MSR_LSTAR, &modMSRValue) == false){
        printf("Failed to read MSRValue !\n");
        return false;
    }

    if (modMSRValue != originalMSRValue){
        printf("MSRValue doesn't match !\n");
        return false;
    }
    FDP_Resume(pFDP);
    printf("[OK]\n");
    return true;
}

#define TEST_REGISTER_VALUE 0xDEADBEEFDEADBEEF
bool testReadWriteRegister(FDP_SHM* pFDP){
    printf("%s ...", __FUNCTION__);

    if (FDP_Pause(pFDP) == false){
        return false;
    }

    uint64_t originalRAXValue;
    if (FDP_ReadRegister(pFDP, 0, FDP_RAX_REGISTER, &originalRAXValue) == false){
        printf("Failed to read RegisterValue !\n");
        return false;
    }

    if (FDP_WriteRegister(pFDP, 0, FDP_RAX_REGISTER, TEST_REGISTER_VALUE) == false){
        printf("Failed to write RegisterValue !\n");
        return false;
    }

    uint64_t modRAXValue;
    if (FDP_ReadRegister(pFDP, 0, FDP_RAX_REGISTER, &modRAXValue) == false){
        printf("Failed to read RegisterValue !\n");
        return false;
    }

    if (modRAXValue != TEST_REGISTER_VALUE){
        printf("RegisterValue doesn't match %" PRIx64 "x != %" PRIx64 "!\n", modRAXValue, TEST_REGISTER_VALUE);
        return false;
    }

    if (FDP_WriteRegister(pFDP, 0, FDP_RAX_REGISTER, originalRAXValue) == false){
        printf("Failed to write RegisterValue !\n");
        return false;
    }
    printf("[OK]\n");
    return true;
}


bool testReadWritePhysicalMemory(FDP_SHM* pFDP){
    printf("%s ...", __FUNCTION__);
    uint8_t originalPage[4096];
    if (FDP_ReadPhysicalMemory(pFDP, originalPage, 4096, 4096 * 12) == false){
        printf("Failed to read PhysicalMemory !\n");
        return false;
    }

    uint8_t garbagePage[4096];
    memset(garbagePage, 0xCA, 4096);
    if (FDP_WritePhysicalMemory(pFDP, garbagePage, 4096, 4096 * 12) == false){
        printf("Failed to write PhysicalMemory !\n");
        return false;
    }

    uint8_t modPage[4096];
    if (FDP_ReadPhysicalMemory(pFDP, modPage, 4096, 4096 * 12) == false){
        printf("Failed to read PhysicalMemory !\n");
        return false;
    }

    if (memcmp(garbagePage, modPage, 4096) != 0){
        printf("Failed to compare garbagePage and modPage !\n");
        return false;
    }

    if (FDP_WritePhysicalMemory(pFDP, originalPage, 4096, 4096 * 12) == false){
        printf("Failed to write PhysicalMemory !\n");
        return false;
    }
    printf("[OK]\n");
    return true;
}

bool testReadWriteVirtualMemorySpeed(FDP_SHM* pFDP){
    printf("%s ...", __FUNCTION__);

    uint64_t tempVirtualAddress = 0;
    uint64_t originalMSRValue;
    if (FDP_ReadMsr(pFDP, 0, MSR_LSTAR, &originalMSRValue) == false){
        printf("Failed to read MSRValue !\n");
        return false;
    }
    tempVirtualAddress = originalMSRValue & 0xFFFFFFFFFFFFF000;
    TimerOut = false;
    TimerGo = true;
    uint64_t ReadCount = 0;
    while (TimerOut == false){
        uint8_t OriginalPage[4096];
        if (FDP_ReadVirtualMemory(pFDP, 0, OriginalPage, 4096, tempVirtualAddress) == false){
            printf("Failed to read VirtualMemory !\n");
            return false;
        }
        ReadCount++;
    }

    int ReadCountPerSecond = (int)ReadCount / iTimerDelay;
    if (ReadCountPerSecond < 400000){
        printf("Too Slow !\n");
        return false;
    }

    printf("[OK] %d/s\n", ReadCountPerSecond);
    return true;
}

bool testReadWritePhysicalMemorySpeed(FDP_SHM* pFDP)
{
    printf("%s ...", __FUNCTION__);

    TimerOut = false;
    TimerGo = true;
    uint64_t ReadCount = 0;
    while (TimerOut == false) {
        uint8_t OriginalPage[4096];
        if (FDP_ReadPhysicalMemory(pFDP, OriginalPage, sizeof(OriginalPage), 0) == false) {
            printf("Failed to read VirtualMemory !\n");
            return false;
        }
        ReadCount++;
    }

    int ReadCountPerSecond = (int)ReadCount / iTimerDelay;
    if (ReadCountPerSecond < 400000) {
        printf("Too Slow !\n");
        return false;
    }

    printf("[OK] %d/s\n", ReadCountPerSecond);
    return true;
}

bool testReadLargePhysicalMemory(FDP_SHM*  pFDP)
{
    printf("%s ...", __FUNCTION__);

    uint8_t *pBuffer = (uint8_t*)malloc((50 * _1M));
    if (pBuffer == NULL) {
        printf("Failed to malloc\n");
        return false;
    }

    uint64_t Cr3;
    FDP_ReadRegister(pFDP, 0, FDP_CR3_REGISTER, &Cr3);

    if (FDP_ReadPhysicalMemory(pFDP, pBuffer, 49 * _1M, Cr3) == false) {
        printf("Failed to FDP_ReadPhysicalMemory\n");
        free(pBuffer);
        return false;
    }

    free(pBuffer);
    printf("[OK]\n");
    return true;
}

//TODO: find contig virtual memory...
bool testReadLargeVirtualMemory(FDP_SHM*  pFDP)
{
    printf("%s ...", __FUNCTION__);

    uint8_t *pBuffer = (uint8_t*)malloc(50 * _1M);
    uint64_t originalMSRValue;
    if (FDP_ReadMsr(pFDP, 0, MSR_LSTAR, &originalMSRValue) == false) {
        printf("Failed to read MSRValue !\n");
        free(pBuffer);
        return false;
    }

    if (FDP_ReadVirtualMemory(pFDP, 0, pBuffer, 1 * _1M, originalMSRValue) == false) {
        free(pBuffer);
        return false;
    }

    free(pBuffer);
    printf("[OK]\n");
    return true;
}

bool testReadWriteVirtualMemory(FDP_SHM* pFDP)
{
    printf("%s ...", __FUNCTION__);

    uint64_t tempVirtualAddress = 0;

    uint64_t originalMSRValue;
    if (FDP_ReadMsr(pFDP, 0, MSR_LSTAR, &originalMSRValue) == false){
        printf("Failed to read MSRValue !\n");
        return false;
    }
    tempVirtualAddress = originalMSRValue & 0xFFFFFFFFFFFFF000;

    uint8_t originalPage[4096];
    if (FDP_ReadVirtualMemory(pFDP, 0, originalPage, 4096, tempVirtualAddress) == false){
        printf("Failed to read VirtualMemory !\n");
        return false;
    }

    uint8_t garbagePage[4096];
    memset(garbagePage, 0xCA, 4096);
    for (int i = 0; i <= 4096; i++){
        if (FDP_WriteVirtualMemory(pFDP, 0, garbagePage, i, tempVirtualAddress) == false){
            printf("Failed to write PhysicalMemory !\n");
            return false;
        }
    }

    uint8_t modPage[4096];
    if (FDP_ReadVirtualMemory(pFDP, 0, modPage, 4096, tempVirtualAddress) == false){
        printf("Failed to read VirtualMemory !\n");
        return false;
    }

    if (memcmp(garbagePage, modPage, 4096) != 0){
        printf("Failed to compare garbagePage and modPage !\n");
        return false;
    }

    for (int i = 0; i <= 4096; i++){
        if (FDP_WriteVirtualMemory(pFDP, 0, originalPage, i, tempVirtualAddress) == false){
            printf("Failed to write PhysicalMemory !\n");
            return false;
        }
    }
    printf("[OK]\n");
    return true;
}

bool testGetStatePerformance(FDP_SHM* pFDP)
{
    printf("%s ...", __FUNCTION__);

    TimerOut = false;
    TimerGo = true;
    uint64_t ReadCount = 0;
    while (TimerOut == false){
        FDP_State state;
        if (FDP_GetState(pFDP, &state) == false){
            printf("Failed to get state !\n");
            return false;
        }
        ReadCount++;
    }

    int ReadPerSecond = (int)(ReadCount / TimerGetDelay());
    printf("[OK] %d/s\n", ReadPerSecond);
    return true;
}

bool testVirtualSyscallBP(FDP_SHM* pFDP, FDP_BreakpointType BreakpointType)
{
    printf("%s ...", __FUNCTION__);

    FDP_Pause(pFDP);

    uint32_t CPUCount;
    if (FDP_GetCpuCount(pFDP, &CPUCount) == false){
        printf("Failed to get CPU count !\n");
        return false;
    }

    uint64_t originalMSRValue;
    if (FDP_ReadMsr(pFDP, 0, MSR_LSTAR, &originalMSRValue) == false){
        printf("Failed to read MSRValue !\n");
        return false;
    }

    int breakpointId = FDP_SetBreakpoint(pFDP, 0, BreakpointType, -1, FDP_EXECUTE_BP, FDP_VIRTUAL_ADDRESS, originalMSRValue, 1, FDP_NO_CR3);
    if (breakpointId < 0){
        printf("Failed to insert page breakpoint !\n");
        return false;
    }

    if (FDP_Resume(pFDP) == false){
        printf("Failed to resume !\n");
        return false;
    }

    int i = 0;
    TimerOut = false;
    TimerGo = true;
    TimerSetDelay(3);
    while (TimerOut == false){
        if (FDP_GetStateChanged(pFDP)){
            FDP_State state;

            if (FDP_GetState(pFDP, &state) == false){
                printf("Failed to get state !\n");
                return false;
            }
            if (state & FDP_STATE_PAUSED
                && state & FDP_STATE_BREAKPOINT_HIT
                && !(state & FDP_STATE_DEBUGGER_ALERTED)){
                i++;

                if (FDP_GetState(pFDP, &state) == false){
                    printf("Failed to get state !\n");
                    return false;
                }

                if (!(state & FDP_STATE_DEBUGGER_ALERTED)){
                    printf("!(state & FDP_STATE_DEBUGGER_ALERTED) (state %02x ) !!!\n", state);
                }

                if (FDP_UnsetBreakpoint(pFDP, breakpointId) == false){
                    printf("Failed to remove page breakpoint !\n");
                    return false;
                }

                for (uint32_t c = 0; c < CPUCount; c++){
                    if (FDP_SingleStep(pFDP, c) == false){
                        printf("Failed to single step !\n");
                        return false;
                    }
                }

                breakpointId = FDP_SetBreakpoint(pFDP, 0, BreakpointType, -1, FDP_EXECUTE_BP, FDP_VIRTUAL_ADDRESS, originalMSRValue, 1, FDP_NO_CR3);
                if (breakpointId < 0){
                    printf("Failed to insert page breakpoint !\n");
                    return false;
                }

                if (FDP_Resume(pFDP) == false){
                    printf("Failed to resume !\n");
                    return false;
                }
            }
        }
    }

    if (FDP_Pause(pFDP) == false){
        printf("Failed to pause !\n");
    }

    if (FDP_UnsetBreakpoint(pFDP, breakpointId) == false){
        printf("Failed to remove page breakpoint !\n");
        return false;
    }

    printf("[OK] %d/s\n", (i / TimerGetDelay()));
    return true;
}

bool testPhysicalSyscallBP(FDP_SHM* pFDP, FDP_BreakpointType BreakpointType){
    printf("%s ...", __FUNCTION__);

    uint64_t originalMSRValue;
    if (FDP_ReadMsr(pFDP, 0, MSR_LSTAR, &originalMSRValue) == false){
        printf("Failed to read MSRValue !\n");
        return false;
    }

    uint64_t physicalLSTAR;
    if (FDP_VirtualToPhysical(pFDP, 0, originalMSRValue, &physicalLSTAR) == false){
        printf("Failed to convert virtual to physical !\n");
        return false;
    }


    int breakpointId = FDP_SetBreakpoint(pFDP, 0, BreakpointType, -1, FDP_EXECUTE_BP, FDP_PHYSICAL_ADDRESS, physicalLSTAR, 1, FDP_NO_CR3);
    if (breakpointId < 0){
        printf("Failed to insert page breakpoint !\n");
        return false;
    }

    if (FDP_Resume(pFDP) == false){
        printf("Failed to resume !\n");
        return false;
    }

    int i = 0;
    TimerOut = false;
    TimerGo = true;
    while (TimerOut == false){
        if (FDP_GetStateChanged(pFDP)){
            FDP_State state;
            if (FDP_GetState(pFDP, &state) == false){
                printf("Failed to get state !\n");
                return false;
            }
            if (state & FDP_STATE_BREAKPOINT_HIT
                && !(state & FDP_STATE_DEBUGGER_ALERTED)){
                i++;
                if (FDP_UnsetBreakpoint(pFDP, breakpointId) == false){
                    printf("Failed to remove page breakpoint !\n");
                    return false;
                }

                if (FDP_SingleStep(pFDP, 0) == false){
                    printf("Failed to single step !\n");
                }

                breakpointId = FDP_SetBreakpoint(pFDP, 0, BreakpointType, -1, FDP_EXECUTE_BP, FDP_PHYSICAL_ADDRESS, physicalLSTAR, 1, FDP_NO_CR3);
                if (breakpointId < 0){
                    printf("Failed to insert page breakpoint !\n");
                    return false;
                }

                if (FDP_Resume(pFDP) == false){
                    printf("Failed to resume !\n");
                    return false;
                }
            }
        }
    }

    if (FDP_Pause(pFDP) == false){
        printf("Failed to pause !\n");
    }

    if (FDP_UnsetBreakpoint(pFDP, breakpointId) == false){
        printf("Failed to remove page breakpoint !\n");
        return false;
    }

    printf("[OK] %d/s\n", (i / TimerGetDelay()));
    return true;
}


bool testMultiplePhysicalSyscallBP(FDP_SHM* pFDP, FDP_BreakpointType BreakpointType){
    printf("%s ...", __FUNCTION__);

    uint64_t originalMSRValue;
    if (FDP_ReadMsr(pFDP, 0, MSR_LSTAR, &originalMSRValue) == false){
        printf("Failed to read MSRValue !\n");
        return false;
    }

    uint64_t physicalLSTAR;
    if (FDP_VirtualToPhysical(pFDP, 0, originalMSRValue, &physicalLSTAR) == false){
        printf("Failed to convert virtual to physical !\n");
        return false;
    }

    int breakpointId[10];
    for (int j = 0; j < 10; j++){
        breakpointId[j] = FDP_SetBreakpoint(pFDP, 0, BreakpointType, -1, FDP_EXECUTE_BP, FDP_PHYSICAL_ADDRESS, physicalLSTAR + j, 1, FDP_NO_CR3);
        if (breakpointId[j] < 0){
            printf("Failed to insert page breakpoint !\n");
            return false;
        }
    }

    if (FDP_Resume(pFDP) == false){
        printf("Failed to resume !\n");
        return false;
    }

    int i = 0;
    while (i < 10){
        FDP_State state;
        if (FDP_GetState(pFDP, &state) == false){
            printf("Failed to get state !\n");
            return false;
        }
        if (state & FDP_STATE_BREAKPOINT_HIT
            && !(state & FDP_STATE_DEBUGGER_ALERTED)){
            i++;

            for (int j = 0; j < 10; j++){
                if (FDP_UnsetBreakpoint(pFDP, breakpointId[j]) == false){
                    printf("Failed to remove page breakpoint !\n");
                    return false;
                }
            }

            if (FDP_SingleStep(pFDP, 0) == false){
                printf("Failed to single step !\n");
            }

            for (int j = 0; j < 10; j++){
                breakpointId[j] = FDP_SetBreakpoint(pFDP, 0, BreakpointType, -1, FDP_EXECUTE_BP, FDP_PHYSICAL_ADDRESS, physicalLSTAR + j, 1, FDP_NO_CR3);
                if (breakpointId[j] < 0){
                    printf("Failed to insert page breakpoint !\n");
                    return false;
                }
            }

            if (FDP_Resume(pFDP) == false){
                printf("Failed to resume !\n");
                return false;
            }
        }
    }

    if (FDP_Pause(pFDP) == false){
        printf("Failed to pause !\n");
    }

    for (int j = 0; j < 10; j++){
        if (FDP_UnsetBreakpoint(pFDP, breakpointId[j]) == false){
            printf("Failed to remove page breakpoint !\n");
            return false;
        }
    }

    printf("[OK]\n");
    return true;
}


bool testMultipleVirtualSyscallBP(FDP_SHM* pFDP, FDP_BreakpointType BreakpointType){
    printf("%s ...", __FUNCTION__);

    uint32_t CPUCount;
    if (FDP_GetCpuCount(pFDP, &CPUCount) == false){
        printf("Failed to get CPU count !\n");
        return false;
    }

    uint64_t originalMSRValue;
    if (FDP_ReadMsr(pFDP, 0, MSR_LSTAR, &originalMSRValue) == false){
        printf("Failed to read MSRValue !\n");
        return false;
    }

    int breakpointId[10];
    for (int j = 0; j < 10; j++){
        breakpointId[j] = FDP_SetBreakpoint(pFDP, 0, BreakpointType, -1, FDP_EXECUTE_BP, FDP_VIRTUAL_ADDRESS, originalMSRValue + j, 1, FDP_NO_CR3);
        if (breakpointId[j] < 0){
            printf("Failed to insert page breakpoint !\n");
            return false;
        }
    }

    if (FDP_Resume(pFDP) == false){
        printf("Failed to resume !\n");
        return false;
    }

    int i = 0;
    while (i < 10){
        FDP_State state;
        if (FDP_GetState(pFDP, &state) == false){
            printf("Failed to get state !\n");
            return false;
        }
        if (state & FDP_STATE_BREAKPOINT_HIT
            && !(state & FDP_STATE_DEBUGGER_ALERTED)){
            i++;

            for (int j = 0; j < 10; j++){
                if (FDP_UnsetBreakpoint(pFDP, breakpointId[j]) == false){
                    printf("Failed to remove page breakpoint !\n");
                    return false;
                }
            }

            for (uint32_t c = 0; c < CPUCount; c++){
                if (FDP_SingleStep(pFDP, c) == false){
                    printf("Failed to single step !\n");
                    return false;
                }
            }

            for (int j = 0; j < 10; j++){
                breakpointId[j] = FDP_SetBreakpoint(pFDP, 0, BreakpointType, -1, FDP_EXECUTE_BP, FDP_VIRTUAL_ADDRESS, originalMSRValue + j, 1, FDP_NO_CR3);
                if (breakpointId[j] < 0){
                    printf("Failed to insert page breakpoint !\n");
                    return false;
                }
            }

            if (FDP_Resume(pFDP) == false){
                printf("Failed to resume !\n");
                return false;
            }
        }
    }

    if (FDP_Pause(pFDP) == false){
        printf("Failed to pause !\n");
    }

    for (int j = 0; j < 10; j++){
        if (FDP_UnsetBreakpoint(pFDP, breakpointId[j]) == false){
            printf("Failed to remove page breakpoint !\n");
            return false;
        }
    }

    printf("[OK]\n");
    return true;
}



bool testLargeVirtualPageSyscallBP(FDP_SHM* pFDP){
    printf("%s ...", __FUNCTION__);

    if (FDP_Pause(pFDP) == false){
        printf("Failed to pause !\n");
        return false;
    }
    uint64_t originalMSRValue;
    if (FDP_ReadMsr(pFDP, 0, MSR_LSTAR, &originalMSRValue) == false){
        printf("Failed to read MSRValue !\n");
        return false;
    }

    int breakpointId = FDP_SetBreakpoint(pFDP, 0, FDP_PAGEHBP, -1, FDP_EXECUTE_BP, FDP_VIRTUAL_ADDRESS, originalMSRValue, 4096*30, FDP_NO_CR3);
    if (breakpointId < 0){
        printf("Failed to insert page breakpoint !\n");
        return false;
    }

    if (FDP_Resume(pFDP) == false){
        printf("Failed to resume !\n");
        return false;
    }

    int i = 0;
    while (i < 10){
        FDP_State state;
        if (FDP_GetState(pFDP, &state) == false){
            printf("Failed to get state !\n");
            return false;
        }
        if (state & FDP_STATE_BREAKPOINT_HIT
            && !(state & FDP_STATE_DEBUGGER_ALERTED)){
            i++;
            //FDP_State state;// = 0;
            printf(".");
            if (FDP_UnsetBreakpoint(pFDP, breakpointId) == false){
                printf("Failed to remove page breakpoint !\n");
                return false;
            }

            if (FDP_SingleStep(pFDP, 0) == false){
                printf("Failed to single step !\n");
            }

            breakpointId = FDP_SetBreakpoint(pFDP, 0, FDP_PAGEHBP, -1, FDP_EXECUTE_BP, FDP_VIRTUAL_ADDRESS, originalMSRValue, 4096 * 30, FDP_NO_CR3);
            if (breakpointId < 0){
                printf("Failed to insert page breakpoint !\n");
                return false;
            }

            if (FDP_Resume(pFDP) == false){
                printf("Failed to resume !\n");
                return false;
            }
        }
    }

    if (FDP_Pause(pFDP) == false){
        printf("Failed to pause !\n");
    }

    if (FDP_UnsetBreakpoint(pFDP, breakpointId) == false){
        printf("Failed to remove page breakpoint !\n");
        return false;
    }

    if (FDP_Resume(pFDP) == false){
        printf("Failed to resume !\n");
        return false;
    }

    printf("[OK]\n");
    return true;
}


bool testLargePhysicalPageSyscallBP(FDP_SHM* pFDP)
{
    printf("%s ...", __FUNCTION__);

    uint64_t originalMSRValue;
    if (FDP_ReadMsr(pFDP, 0, MSR_LSTAR, &originalMSRValue) == false){
        printf("Failed to read MSRValue !\n");
        return false;
    }

    uint64_t physicalLSTAR;
    if (FDP_VirtualToPhysical(pFDP, 0, originalMSRValue, &physicalLSTAR) == false){
        printf("Failed to convert virtual to physical !\n");
        return false;
    }

    int breakpointId = FDP_SetBreakpoint(pFDP, 0, FDP_PAGEHBP, -1, FDP_EXECUTE_BP, FDP_PHYSICAL_ADDRESS, physicalLSTAR, 4096 * 30, FDP_NO_CR3);
    if (breakpointId < 0){
        printf("Failed to insert page breakpoint !\n");
        return false;
    }

    if (FDP_Resume(pFDP) == false){
        printf("Failed to resume !\n");
        return false;
    }

    int i = 0;
    while (i < 10){
        if (FDP_GetStateChanged(pFDP) == true){
            FDP_State state;
            if (FDP_GetState(pFDP, &state) == false){
                printf("Failed to get state !\n");
                return false;
            }
            if (state & FDP_STATE_BREAKPOINT_HIT
                && !(state & FDP_STATE_DEBUGGER_ALERTED)){
                i++;
                //FDP_State state = 0;
                if (FDP_UnsetBreakpoint(pFDP, breakpointId) == false){
                    printf("Failed to remove page breakpoint !\n");
                    return false;
                }

                if (FDP_SingleStep(pFDP, 0) == false){
                    printf("Failed to single step !\n");
                }

                breakpointId = FDP_SetBreakpoint(pFDP, 0, FDP_PAGEHBP, -1, FDP_EXECUTE_BP, FDP_PHYSICAL_ADDRESS, physicalLSTAR, 4096 * 30, FDP_NO_CR3);
                if (breakpointId < 0){
                    printf("Failed to insert page breakpoint !\n");
                    return false;
                }

                if (FDP_Resume(pFDP) == false){
                    printf("Failed to resume !\n");
                    return false;
                }
            }
        }
    }

    if (FDP_Pause(pFDP) == false){
        printf("Failed to pause !\n");
    }

    if (FDP_UnsetBreakpoint(pFDP, breakpointId) == false){
        printf("Failed to remove page breakpoint !\n");
        return false;
    }

    if (FDP_Resume(pFDP) == false){
        printf("Failed to resume !\n");
        return false;
    }

    printf("[OK]\n");
    return true;
}


bool testMultiCpu(FDP_SHM* pFDP)
{
    printf("%s ...", __FUNCTION__);

    uint32_t CPUCount;
    if (FDP_GetCpuCount(pFDP, &CPUCount) == false){
        printf("Failed to get CPU count !\n");
        return false;
    }

    uint64_t oldRAXValue = 0;
    for (uint32_t c = 0; c < CPUCount; c++){

        uint64_t RAXValue;
        if (FDP_ReadRegister(pFDP, c, FDP_RAX_REGISTER, &RAXValue) == false){
            printf("Failed to read register value!\n");
            return false;
        }

        if (oldRAXValue == RAXValue){
            //printf("Failed to switch CPU!\n");
            //return false;
        }
    }


    printf("[OK]\n");
    return true;
}

static void wait_input()
{
    char line[256];
    char* rpy = fgets(line, (int) sizeof line, stdin);
    (void) rpy;
}



bool testState(FDP_SHM* pFDP)
{
    printf("%s ...", __FUNCTION__);
    if (FDP_Resume(pFDP) == false){
        printf("Failed to resume!\n");
        wait_input();;
        return false;
    }
    if (FDP_Pause(pFDP) == false){
        printf("Failed to pause !\n");
        wait_input();
        return false;
    }

    FDP_State state;
    if (FDP_GetState(pFDP, &state) == false){
        printf("Failed to get state !\n");
        wait_input();
        return false;
    }

    if (!(state & FDP_STATE_PAUSED)){
        printf("1. State !=  STATE_PAUSED (state %02x)!\n", state);
        wait_input();
        return false;
    }

    if (FDP_Pause(pFDP) == false){
        printf("Failed to pause !\n");
        wait_input();
        return false;
    }

    if (FDP_Resume(pFDP) == false){
        printf("Failed to pause !\n");
        wait_input();
        return false;
    }

    if (FDP_Resume(pFDP) == false){
        printf("Failed to pause !\n");
        wait_input();
        return false;
    }

    if (FDP_Pause(pFDP) == false){
        printf("Failed to pause !\n");
        wait_input();
        return false;
    }

    if (FDP_GetState(pFDP, &state) == false){
        printf("Failed to get state !\n");
        wait_input();
        return false;
    }

    if (!(state & FDP_STATE_PAUSED)){
        printf("3. State !=  STATE_PAUSED (state %02x)!\n", state);
        wait_input();
        return false;
    }

    uint64_t originalMSRValue;
    if (FDP_ReadMsr(pFDP, 0, MSR_LSTAR, &originalMSRValue) == false){
        printf("Failed to read MSRValue !\n");
        return false;
    }

    int breakpointId = FDP_SetBreakpoint(pFDP, 0, FDP_SOFTHBP, -1, FDP_EXECUTE_BP, FDP_VIRTUAL_ADDRESS, originalMSRValue, 1, FDP_NO_CR3);
    if (breakpointId < 0){
        printf("Failed to insert page breakpoint !\n");
        return false;
    }

    if (FDP_Resume(pFDP) == false){
        printf("Failed to resume");
        return false;
    }

    while (true){
        FDP_State state;
        if (FDP_GetState(pFDP, &state) == false){
            printf("Failed to get state !\n");
            return false;
        }

        if (state & FDP_STATE_BREAKPOINT_HIT
            && !(state & FDP_STATE_DEBUGGER_ALERTED)){
            break;
        }
        else{
            //printf("state %02x\n", state);
        }
        SLEEP(100);
    }

    if (FDP_UnsetBreakpoint(pFDP, breakpointId) == false){
        printf("Failed to clear breakpoint !\n");
        return false;
    }

    if (FDP_Resume(pFDP) == false){
        printf("Failed to resume !\n");
        return false;
    }

    printf("[OK]\n");
    return true;
}

bool testDebugRegisters(FDP_SHM* pFDP){
    printf("%s ...", __FUNCTION__);

    if (FDP_Pause(pFDP) == false){
        printf("Failed to FDP_Pause !\n");
        return false;
    }

    uint64_t oldDR0Value;
    uint64_t oldDR7Value;
    if (FDP_ReadRegister(pFDP, 0, FDP_DR0_REGISTER, &oldDR0Value) == false){
        return false;
    }
    if (FDP_ReadRegister(pFDP, 0, FDP_DR7_REGISTER, &oldDR7Value) == false){
        return false;
    }

    uint64_t LSTARValue;
    if (FDP_ReadMsr(pFDP, 0, MSR_LSTAR, &LSTARValue) == false){
        printf("Failed to read FDP_ReadMsr !\n");
        return false;
    }
    if (FDP_WriteRegister(pFDP, 0, FDP_DR0_REGISTER, LSTARValue) == false){
        printf("Failed to write FDP_WriteRegister!\n");
        return false;
    }

    if (FDP_WriteRegister(pFDP, 0, FDP_DR7_REGISTER, 0x0000000000000403) == false){
        printf("Failed to write FDP_WriteRegister!\n");
        return false;
    }

    if (FDP_Resume(pFDP) == 0){
        return false;
    }

    TimerOut = false;
    TimerGo = true;
    TimerSetDelay(5);
    uint64_t i = 0;
    while (TimerOut == false){
        if (FDP_GetStateChanged(pFDP) == true){
            FDP_State state;
            if (FDP_GetState(pFDP, &state) == false){
                printf("Failed to FDP_GetState !\n");
                return false;
            }
            if (state & FDP_STATE_BREAKPOINT_HIT){
                i++;
                if (FDP_SingleStep(pFDP, 0) == false){
                    return false;
                }
                if (FDP_Resume(pFDP) == false){
                    return false;
                }
            }
        }
    }

    if (FDP_WriteRegister(pFDP, 0, FDP_DR0_REGISTER, oldDR0Value) == false){
        return false;
    }

    if (FDP_WriteRegister(pFDP, 0, FDP_DR7_REGISTER, oldDR7Value) == false){
        return false;
    }

    if (FDP_Resume(pFDP) == false){
        return false;
    }

    int BreakpointPerSecond = (int)i / TimerGetDelay();
    printf("[OK] %d/s\n", BreakpointPerSecond);
    return true;
}

bool threadRunning;

#ifdef  _MSC_VER
DWORD WINAPI testStateThread(LPVOID lpParam)
#else
void * testStateThread(void *lpParam)
#endif
{
    FDP_SHM* pFDP = (FDP_SHM*)lpParam;
    while (threadRunning){
        FDP_State state;
        FDP_GetState(pFDP, &state);
    }
    return 0;
}

#ifdef  _MSC_VER
DWORD WINAPI testReadRegisterThread(LPVOID lpParam)
#else
void * testReadRegisterThread(void *lpParam)
#endif
{
    FDP_SHM* pFDP = (FDP_SHM*)lpParam;
    while (threadRunning){
        uint64_t RegisterValue;
        FDP_ReadRegister(pFDP, 0, FDP_RAX_REGISTER, &RegisterValue);
    }
    return 0;
}

#ifdef  _MSC_VER
DWORD WINAPI testReadMemoryThread(LPVOID lpParam)
#else
void * testReadMemoryThread(void *lpParam)
#endif
{
    FDP_SHM* pFDP = (FDP_SHM*)lpParam;
    while (threadRunning){
        uint8_t TempBuffer[1024];
        FDP_ReadPhysicalMemory(pFDP, TempBuffer, 1024, 0);
    }
    return 0;
}

bool testMultiThread(FDP_SHM* pFDP)
{
    printf("%s ...", __FUNCTION__);

    threadRunning = true;

#ifdef  _MSC_VER
    HANDLE thread1 = CreateThread(NULL, 0, testStateThread, pFDP, 0, NULL);
    HANDLE thread2 = CreateThread(NULL, 0, testReadRegisterThread, pFDP, 0, NULL);
    HANDLE thread3 = CreateThread(NULL, 0, testReadMemoryThread, pFDP, 0, NULL);
#else
    pthread_t thread1;
    pthread_t thread2;
    pthread_t thread3;

    pthread_create(&thread1, NULL, testStateThread, pFDP);
    pthread_create(&thread2, NULL, testReadRegisterThread, pFDP);
    pthread_create(&thread3, NULL, testReadMemoryThread, pFDP);
#endif

    SLEEP(2000);

    threadRunning = false;

    SLEEP(100);

    THREAD_EXIT(thread1);
    THREAD_EXIT(thread2);
    THREAD_EXIT(thread3);

    printf("[OK]\n");
    return true;

}

bool testUnsetBreakpoint(FDP_SHM* pFDP)
{
    printf("%s ...", __FUNCTION__);

    if (FDP_Pause(pFDP) == false){
        return false;
    }

    for (int b = 0; b < FDP_MAX_BREAKPOINT; b++){
        FDP_UnsetBreakpoint(pFDP, b);
    }

    if (FDP_Resume(pFDP) == 0){
        return false;
    }

    printf("[OK]\n");
    return true;
}

bool testSingleStep(FDP_SHM* pFDP){
    printf("%s ...", __FUNCTION__);

    if (FDP_Pause(pFDP) == false){
        printf("Failed to pause !\n");
        return false;
    }
    uint64_t SingleStepCount = 0;
    TimerSetDelay(30);
    TimerOut = false;
    TimerGo = true;
    while (TimerOut == false){
        FDP_SingleStep(pFDP, 0);
        SingleStepCount++;
    }
    FDP_Resume(pFDP);

    printf("[OK]\n");
    return true;
}

bool testSingleStepSpeed(FDP_SHM* pFDP){
    printf("%s ...", __FUNCTION__);

    FDP_Pause(pFDP);

    uint64_t SingleStepCount = 0;
    TimerSetDelay(3);
    TimerOut = false;
    TimerGo = true;
    while (TimerOut == false){
        FDP_SingleStep(pFDP, 0);
        SingleStepCount++;
    }

    int SingleStepCountPerSecond = (int)SingleStepCount / TimerGetDelay();

    // if (SingleStepCountPerSecond < 50000){
    //     printf("Too slow !\n");
    //     return false;
    // }
    FDP_Resume(pFDP);
    printf("[OK] %d/s\n", SingleStepCountPerSecond);
    return true;
}

bool testSaveRestore(FDP_SHM* pFDP)
{
    printf("%s ...", __FUNCTION__);

    if (FDP_Pause(pFDP) == false){
        printf("Failed to pause !\n");
        wait_input();
        return false;
    }

    if (FDP_Save(pFDP) == false){
        printf("Failed to save !\n");
        return false;
    }

    uint64_t OriginalRipValue;
    if (FDP_ReadRegister(pFDP, 0, FDP_RIP_REGISTER, &OriginalRipValue) == false){
        printf("Failed to FDP_ReadRegister\n");
        return false;
    }

    if (FDP_Resume(pFDP) == false){
        printf("Failed to resume !\n");
        return false;
    }

    for (int i = 0; i < 10; i++){
        //Random Sleep
        srand((unsigned int)time(NULL));
        SLEEP(rand()%3000);


        if (FDP_Restore(pFDP) == false){
            printf("Failed to FDP_Restore !\n");
            return false;
        }

        uint64_t NewRipValue;
        if (FDP_ReadRegister(pFDP, 0, FDP_RIP_REGISTER, &NewRipValue) == false){
            printf("Failed to FDP_ReadRegister\n");
            return false;
        }

        if (OriginalRipValue != NewRipValue){
            printf("OriginalRipValue != NewRipValue\n");
            return false;
        }

        if (FDP_Resume(pFDP) == false){
            printf("Failed to FDP_Resume !\n");
            return false;

        }

        //Wait for Rip change
        uint64_t OldRipValue;
        uint64_t RipValue;
        FDP_ReadRegister(pFDP, 0, FDP_RIP_REGISTER, &OldRipValue);
        while (true){
            FDP_ReadRegister(pFDP, 0, FDP_RIP_REGISTER, &RipValue);
            if (RipValue != OldRipValue){
                break;
            }
        }

    }

    if (FDP_Resume(pFDP) == false){
        printf("Failed to FDP_Resume !\n");
        return false;
    }

    printf("[OK]\n");
    return true;
}

bool testReadAllPhysicalMemory(FDP_SHM* pFDP)
{
    printf("%s ...", __FUNCTION__);

    uint64_t PhysicalAddress = 0;
    uint8_t Buffer[4096];
    uint64_t PhysicalMaxAddress = 0;

    if (FDP_Pause(pFDP) == false){
        printf("Failed to pause !\n");
        return false;
    }
    if (FDP_GetPhysicalMemorySize(pFDP, &PhysicalMaxAddress) == false){
        printf("Failed to FDP_GetPhysicalMemorySize\n");
        return false;
    }
    while (PhysicalAddress < PhysicalMaxAddress){
        if (FDP_ReadPhysicalMemory(pFDP, Buffer, sizeof(Buffer), PhysicalAddress) == false){
            //Some PhysicalAddress aren't readeable
            //printf("Failed to FDP_ReadPhysicalMemory (%p)\n", PhysicalAddress);
            //return false;
        }
        PhysicalAddress += sizeof(Buffer);
    }
    if (FDP_Resume(pFDP) == false){
        printf("Failed to resume !\n");
        return false;
    }

    printf("[OK]\n");
    return true;
}


bool testReadWriteAllPhysicalMemory(FDP_SHM* pFDP)
{
    printf("%s ...", __FUNCTION__);

    uint64_t PhysicalAddress = 0;
    uint8_t Buffer[4096];
    uint64_t PhysicalMaxAddress = 0;

    if (FDP_Pause(pFDP) == false){
        printf("Failed to pause !\n");
        return false;
    }
    if (FDP_GetPhysicalMemorySize(pFDP, &PhysicalMaxAddress) == false){
        printf("Failed to FDP_GetPhysicalMemorySize\n");
        return false;
    }

    while (PhysicalAddress < PhysicalMaxAddress){
        if (FDP_ReadPhysicalMemory(pFDP, Buffer, sizeof(Buffer), PhysicalAddress) == true){
            FDP_WritePhysicalMemory(pFDP, Buffer, sizeof(Buffer), PhysicalAddress);
        }
        PhysicalAddress += sizeof(Buffer);
    }
    if (FDP_Resume(pFDP) == false){
        printf("Failed to resume !\n");
        return false;
    }

    printf("[OK]\n");
    return true;
}

bool testSetCr3(FDP_SHM* pFDP)
{
    printf("%s ...", __FUNCTION__);

    uint64_t u64OldCr3;
    uint64_t OldValue;
    uint64_t Value;

    FDP_Pause(pFDP);
    FDP_ReadRegister(pFDP, 0, FDP_CR3_REGISTER, &u64OldCr3);
    FDP_ReadVirtualMemory(pFDP, 0, (uint8_t*)&OldValue, sizeof(Value), 0xFFFFF6FB7DBEDF68);

    FDP_WriteRegister(pFDP, 0, FDP_CR3_REGISTER, u64OldCr3 + 0x1000);
    FDP_ReadVirtualMemory(pFDP, 0, (uint8_t*)&Value, sizeof(Value), 0xFFFFF6FB7DBEDF68);
    if (Value == OldValue){
        printf("Failed to change Cr3 !\n");
        return false;
    }

    FDP_WriteRegister(pFDP, 0, FDP_CR3_REGISTER, u64OldCr3);
    FDP_ReadVirtualMemory(pFDP, 0, (uint8_t*)&Value, sizeof(Value), 0xFFFFF6FB7DBEDF68);
    if (Value != OldValue){
        printf("Failed to restore Cr3 !");
        return false;
    }

    FDP_Resume(pFDP);

    printf("[OK]\n");
    return true;
}


bool testSingleStepPageBreakpoint(FDP_SHM* pFDP)
{
    printf("%s ...", __FUNCTION__);
    bool bReturnValue = false;

    //Bug single-step PageHyperBreakpoint
    FDP_Pause(pFDP);

    uint64_t originalMSRValue;
    if (FDP_ReadMsr(pFDP, 0, MSR_LSTAR, &originalMSRValue) == false) {
        printf("Failed to read MSRValue !\n");
        return false;
    }

    int BreakpointId = FDP_SetBreakpoint(pFDP, 0, FDP_PAGEHBP, -1, FDP_EXECUTE_BP, FDP_VIRTUAL_ADDRESS, originalMSRValue, 1, FDP_NO_CR3);
    if (BreakpointId < 0) {
        printf("Failed to insert page breakpoint !\n");
        return false;
    }

    if (FDP_Resume(pFDP) == false) {
        printf("Failed to resume !\n");
        return false;
    }

    bool bRunning = true;
    while (bRunning == true) {
        if (FDP_GetStateChanged(pFDP)) {
            FDP_State state;
            if (FDP_GetState(pFDP, &state) == false) {
                printf("Failed to get state !\n");
                return false;
            }
            if (state & FDP_STATE_PAUSED
                && state & FDP_STATE_BREAKPOINT_HIT
                && !(state & FDP_STATE_DEBUGGER_ALERTED)) {
                break;
            }
        }
    }

    uint64_t OldRip;
    FDP_ReadRegister(pFDP, 0, FDP_RIP_REGISTER, &OldRip);
    FDP_SingleStep(pFDP, 0);
    uint64_t NewRip;
    FDP_ReadRegister(pFDP, 0, FDP_RIP_REGISTER, &NewRip);
    if (OldRip == NewRip) {
        bReturnValue = false;
    }
    else {
        bReturnValue = true;
    }

    //FDP_Pause(pFDP);
    FDP_UnsetBreakpoint(pFDP, BreakpointId);
    FDP_Resume(pFDP);
    if (bReturnValue == true) {
        printf("[OK]\n");
    }
    else {
        printf("[FAIL]\n");
    }
    return bReturnValue;
}

bool testSingleStepPause(FDP_SHM* pFDP)
{
    printf("%s ...", __FUNCTION__);

    FDP_Resume(pFDP);
    FDP_SingleStep(pFDP, 0);
    printf("[OK]\n");

    return true;
}




int testFDP(char *pVMName) {
    bool bReturnCode = false;
    FDP_SHM* pFDP = FDP_OpenSHM(pVMName);
    if (pFDP) {
        //Start Timer Thread
#ifdef  _MSC_VER
        CreateThread(NULL, 0, TimerRoutine, NULL, 0, NULL);
#else
        pthread_t timerthread;
        pthread_create(&timerthread, NULL, TimerRoutine, NULL);

#endif

        if (FDP_Init(pFDP) == false) {
            printf("Failed to FDP_Init !\n");
            goto Fail;
        }

        if (testUnsetBreakpoint(pFDP) == false)
            goto Fail;
        //if (testSingleStepPause(pFDP) == false)
        //    goto Fail;
        if (testSingleStepPageBreakpoint(pFDP) == false)
            goto Fail;
        if (testSingleStepSpeed(pFDP) == false)
            goto Fail;
        if (testReadWriteMSR(pFDP) == false)
            goto Fail;
        if (testSetCr3(pFDP) == false)
            goto Fail;
        // if (testMultiThread(pFDP) == false)
        //     goto Fail;
        /*
        if (testState(pFDP) == false)
            goto Fail;
        if (FDP_Pause(pFDP) == false)
            goto Fail;
        */
        if (testVirtualSyscallBP(pFDP, FDP_SOFTHBP) == false)
            goto Fail;
        //if (testMultiCpu(pFDP) == false)
        //    goto Fail;
        if (testReadWriteRegister(pFDP) == false)
            goto Fail;
        if (testReadWritePhysicalMemory(pFDP) == false)
            goto Fail;
        if (testReadWriteVirtualMemory(pFDP) == false)
            goto Fail;
        if (testGetStatePerformance(pFDP) == false)
            goto Fail;
        // if (testDebugRegisters(pFDP) == false)
        //     goto Fail;
        if (testVirtualSyscallBP(pFDP, FDP_PAGEHBP) == false)
            goto Fail;
        if (testVirtualSyscallBP(pFDP, FDP_SOFTHBP) == false)
            goto Fail;
        if (testPhysicalSyscallBP(pFDP, FDP_PAGEHBP) == false)
            goto Fail;
        if (testPhysicalSyscallBP(pFDP, FDP_SOFTHBP) == false)
            goto Fail;
        if (testMultipleVirtualSyscallBP(pFDP, FDP_PAGEHBP) == false)
            goto Fail;
        if (testMultipleVirtualSyscallBP(pFDP, FDP_SOFTHBP) == false)
            goto Fail;
        if (testMultiplePhysicalSyscallBP(pFDP, FDP_PAGEHBP) == false)
            goto Fail;
        if (testMultiplePhysicalSyscallBP(pFDP, FDP_SOFTHBP) == false)
            goto Fail;
        if (testReadAllPhysicalMemory(pFDP) == false)
            goto Fail;
        if (testReadWriteAllPhysicalMemory(pFDP) == false)
            goto Fail;
        if (testLargeVirtualPageSyscallBP(pFDP) == false)
            goto Fail;
        if (testLargePhysicalPageSyscallBP(pFDP) == false)
            goto Fail;
        if (testReadWriteVirtualMemorySpeed(pFDP) == false)
            goto Fail;
        if (testReadWritePhysicalMemorySpeed(pFDP) == false)
            goto Fail;
        if (testReadLargePhysicalMemory(pFDP) == false)
            goto Fail;
        if (testReadLargeVirtualMemory(pFDP) == false)
            goto Fail;
        /*if (testSaveRestore(pFDP) == false)
            goto Fail;*/

        bReturnCode = true;
    Fail:
        testUnsetBreakpoint(pFDP);
        //FDP_Resume(pFDP);
    }

    printf("*********************\n");
    printf("*********************\n");
    if (bReturnCode == false){
        printf("**  TESTS FAILED !  **\n");
    }
    else{
        printf("** TESTS PASSED !  **\n");
    }
    printf("*********************\n");
    printf("*********************\n");
    return bReturnCode;
}


void usage(char * binPath){
    printf("Usage: %s [VM Name]\n", binPath);
}

int main(int argc, char *argv[]){
    if (argc != 2){
        usage(argv[0]);
        return 2;
    }

    if (testFDP(argv[1]))
        return 0;
    else
        return 1;
}
