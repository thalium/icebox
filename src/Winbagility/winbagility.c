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
#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

#include "WindowsProfils.h"
#include "FDP.h"
#include "winbagility.h"
#include "kd.h"
#include "kdserver.h"
#include "mmu.h"
#include "dissectors.h"
#include "utils.h"
#include "kdproxy.h"
#include "STUB/windows_structs.h"
#include "STUB/GDB.h"
#include "STUB/CRASH.h"
#include "STUB/LIVEKADAY.h"

//TODO: !
void Usage(){
    printf("Winbagility PipeName StubType StubOpenArgument\n");
    printf("Winbagility \\\\.\\pipe\\client FDP 10_x64\n");
    printf("Winbagility \\\\.\\pipe\\client GDB 127.0.0.1:8864\n");
    printf("Winbagility \\\\.\\pipe\\client CRASH D:\\\\8_1_x64_STOCK.dmp\n");
    printf("Winbagility \\\\.\\pipe\\client LIVEKADAY ?\n");
}

//TODO: Dummy STUB
bool CreateWinbagilityInterface(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, char *pStubName)
{
    if (strcmp(pStubName, "GDB") == 0){
        //Prepare GDB Interface
        pWinbagilityCtx->CurrentMode = StubTypeGdb;
        pWinbagilityCtx->pfnOpen = (pfnOpen_t)GDB_Open;
        pWinbagilityCtx->pfnInit = (pfnInit_t)GDB_Init;
        pWinbagilityCtx->pfnGetPhysicalMemorySize = (pfnGetPhysicalMemorySize_t)GDB_GetPhysicalMemorySize;
        pWinbagilityCtx->pfnReadVirtualMemory = (pfnReadVirtualMemory_t)GDB_ReadVirtualMemory;
        pWinbagilityCtx->pfnReadRegister = (pfnReadRegister_t)GDB_ReadRegister;
        pWinbagilityCtx->pfnReadMsr = (pfnReadMsr_t)GDB_ReadMsr;
        pWinbagilityCtx->pfnResume = (pfnResume_t)GDB_Resume;
        pWinbagilityCtx->pfnSingleStep = (pfnSingleStep_t)GDB_SingleStep;
        pWinbagilityCtx->pfnPause = (pfnPause_t)GDB_Pause;
        pWinbagilityCtx->pfnWriteRegister = (pfnWriteRegister_t)GDB_WriteRegister;
        pWinbagilityCtx->pfnUnsetBreakpoint = (pfnUnsetBreakpoint_t)GDB_UnsetBreakpoint;
        pWinbagilityCtx->pfnGetFxState64 = (pfnGetFxState64_t)GDB_GetFxState64;
        pWinbagilityCtx->pfnSetFxState64 = (pfnSetFxState64_t)GDB_SetFxState64;
        pWinbagilityCtx->pfnReadPhysicalMemory = (pfnReadPhysicalMemory_t)GDB_ReadPhysicalMemory;
        pWinbagilityCtx->pfnGetCpuState = (pfnGetCpuState_t)GDB_GetCpuState;
        pWinbagilityCtx->pfnSetBreakpoint = (pfnSetBreakpoint_t)GDB_SetBreakpoint;
        pWinbagilityCtx->pfnWriteVirtualMemory = (pfnWriteVirtualMemory_t)GDB_WriteVirtualMemory;

        //TODO !
        /*pWinbagilityCtx->pfnVirtualToPhysical = (pfnVirtualToPhysical_t)GDB_VirtualToPhysical;
        pWinbagilityCtx->pfnWritePhysicalMemory = (pfnWritePhysicalMemory_t)GDB_WritePhysicalMemory;
        pWinbagilityCtx->pfnSearchPhysicalMemory = (pfnSearchPhysicalMemory_t)GDB_SearchPhysicalMemory;
        pWinbagilityCtx->pfnWriteMsr = (pfnWriteMsr_t)GDB_WriteMsr;
        pWinbagilityCtx->pfnReboot = (pfnReboot_t)GDB_Reboot;*/
        return true;
    }

    if (strcmp(pStubName, "CRASH") == 0){
        //Prepare CRASH Interface
        pWinbagilityCtx->CurrentMode = StubTypeCrash;
        pWinbagilityCtx->pfnOpen = (pfnOpen_t)CRASH_Open;
        pWinbagilityCtx->pfnInit = (pfnInit_t)CRASH_Init;
        pWinbagilityCtx->pfnGetPhysicalMemorySize = (pfnGetPhysicalMemorySize_t)CRASH_GetPhysicalMemorySize;
        pWinbagilityCtx->pfnReadRegister = (pfnReadRegister_t)CRASH_ReadRegister;
        pWinbagilityCtx->pfnReadMsr = (pfnReadMsr_t)CRASH_ReadMsr;
        pWinbagilityCtx->pfnReadVirtualMemory = (pfnReadVirtualMemory_t)CRASH_ReadVirtualMemory;
        pWinbagilityCtx->pfnReadPhysicalMemory = (pfnReadPhysicalMemory_t)CRASH_ReadPhysicalMemory;
        pWinbagilityCtx->pfnPause = (pfnPause_t)CRASH_Pause;
        pWinbagilityCtx->pfnResume = (pfnResume_t)CRASH_Resume;
        pWinbagilityCtx->pfnUnsetBreakpoint = (pfnUnsetBreakpoint_t)CRASH_UnsetBreakpoint;
        pWinbagilityCtx->pfnGetFxState64 = (pfnGetFxState64_t)CRASH_GetFxState64;
        pWinbagilityCtx->pfnSetFxState64 = (pfnSetFxState64_t)CRASH_SetFxState64;
        pWinbagilityCtx->pfnWriteRegister = (pfnWriteRegister_t)CRASH_WriteRegister;
        pWinbagilityCtx->pfnSingleStep = (pfnSingleStep_t)CRASH_SingleStep;
        pWinbagilityCtx->pfnWritePhysicalMemory = (pfnWritePhysicalMemory_t)CRASH_WritePhysicalMemory;
        pWinbagilityCtx->pfnWriteVirtualMemory = (pfnWriteVirtualMemory_t)CRASH_WriteVirtuallMemory;
        pWinbagilityCtx->pfnSetBreakpoint = (pfnSetBreakpoint_t)CRASH_SetBreakpoint;
        pWinbagilityCtx->pfnGetCpuState = (pfnGetCpuState_t)CRASH_GetCpuState;
        return true;
    }

    if (strcmp(pStubName, "LIVEKADAY") == 0){
        //Prepare LIVEKADAY Interface
        pWinbagilityCtx->CurrentMode = StubTypeLiveKaDay;
        pWinbagilityCtx->pfnOpen = (pfnOpen_t)LIVEKADAY_Open;
        pWinbagilityCtx->pfnInit = (pfnInit_t)LIVEKADAY_Init;
        pWinbagilityCtx->pfnGetPhysicalMemorySize = (pfnGetPhysicalMemorySize_t)LIVEKADAY_GetPhysicalMemorySize;
        pWinbagilityCtx->pfnReadRegister = (pfnReadRegister_t)LIVEKADAY_ReadRegister;
        pWinbagilityCtx->pfnReadMsr = (pfnReadMsr_t)LIVEKADAY_ReadMsr;
        pWinbagilityCtx->pfnReadVirtualMemory = (pfnReadVirtualMemory_t)LIVEKADAY_ReadVirtualMemory;
        pWinbagilityCtx->pfnReadPhysicalMemory = (pfnReadPhysicalMemory_t)LIVEKADAY_ReadPhysicalMemory;
        pWinbagilityCtx->pfnPause = (pfnPause_t)LIVEKADAY_Pause;
        pWinbagilityCtx->pfnResume = (pfnResume_t)LIVEKADAY_Resume;
        pWinbagilityCtx->pfnUnsetBreakpoint = (pfnUnsetBreakpoint_t)LIVEKADAY_UnsetBreakpoint;
        pWinbagilityCtx->pfnGetFxState64 = (pfnGetFxState64_t)LIVEKADAY_GetFxState64;
        pWinbagilityCtx->pfnSetFxState64 = (pfnSetFxState64_t)LIVEKADAY_SetFxState64;
        pWinbagilityCtx->pfnWriteRegister = (pfnWriteRegister_t)LIVEKADAY_WriteRegister;
        pWinbagilityCtx->pfnSingleStep = (pfnSingleStep_t)LIVEKADAY_SingleStep;
        pWinbagilityCtx->pfnWritePhysicalMemory = (pfnWritePhysicalMemory_t)LIVEKADAY_WritePhysicalMemory;
        pWinbagilityCtx->pfnWriteVirtualMemory = (pfnWriteVirtualMemory_t)LIVEKADAY_WriteVirtuallMemory;
        pWinbagilityCtx->pfnSetBreakpoint = (pfnSetBreakpoint_t)LIVEKADAY_SetBreakpoint;
        pWinbagilityCtx->pfnGetCpuState = (pfnGetCpuState_t)LIVEKADAY_GetCpuState;
        return true;
    }

    if (strcmp(pStubName, "FDP") == 0){
        //Prepare FDP Interface !
        pWinbagilityCtx->CurrentMode = StubTypeFdp;
        pWinbagilityCtx->pfnOpen = (pfnOpen_t)FDP_OpenSHM;
        pWinbagilityCtx->pfnInit = (pfnInit_t)FDP_Init;
        pWinbagilityCtx->pfnGetPhysicalMemorySize = (pfnGetPhysicalMemorySize_t)FDP_GetPhysicalMemorySize;
        pWinbagilityCtx->pfnReadVirtualMemory = (pfnReadVirtualMemory_t)FDP_ReadVirtualMemory;
        pWinbagilityCtx->pfnReadRegister = (pfnReadRegister_t)FDP_ReadRegister;
        pWinbagilityCtx->pfnResume = (pfnResume_t)FDP_Resume;
        pWinbagilityCtx->pfnSingleStep = (pfnSingleStep_t)FDP_SingleStep;
        pWinbagilityCtx->pfnPause = (pfnPause_t)FDP_Pause;
        pWinbagilityCtx->pfnWriteRegister = (pfnWriteRegister_t)FDP_WriteRegister;
        pWinbagilityCtx->pfnUnsetBreakpoint = (pfnUnsetBreakpoint_t)FDP_UnsetBreakpoint;
        pWinbagilityCtx->pfnVirtualToPhysical = (pfnVirtualToPhysical_t)FDP_VirtualToPhysical;
        pWinbagilityCtx->pfnWritePhysicalMemory = (pfnWritePhysicalMemory_t)FDP_WritePhysicalMemory;
        pWinbagilityCtx->pfnWriteVirtualMemory = (pfnWriteVirtualMemory_t)FDP_WriteVirtualMemory;
        pWinbagilityCtx->pfnReadPhysicalMemory = (pfnReadPhysicalMemory_t)FDP_ReadPhysicalMemory;
        pWinbagilityCtx->pfnSearchPhysicalMemory = (pfnSearchPhysicalMemory_t)FDP_SearchPhysicalMemory;
        pWinbagilityCtx->pfnSetBreakpoint = (pfnSetBreakpoint_t)FDP_SetBreakpoint;
        pWinbagilityCtx->pfnGetCpuState = (pfnGetCpuState_t)FDP_GetCpuState;
        pWinbagilityCtx->pfnGetFxState64 = (pfnGetFxState64_t)FDP_GetFxState64;
        pWinbagilityCtx->pfnSetFxState64 = (pfnSetFxState64_t)FDP_SetFxState64;
        pWinbagilityCtx->pfnReadMsr = (pfnReadMsr_t)FDP_ReadMsr;
        pWinbagilityCtx->pfnWriteMsr = (pfnWriteMsr_t)FDP_WriteMsr;
        pWinbagilityCtx->pfnReboot = (pfnReboot_t)FDP_Reboot;
        return true;
    }

    return false;
}





int main(int argc, char* argv[], char *env)
{
    //Tests 
    //return startKDProxy();

    /*argc = 4;
    argv[1] = "\\\\.\\pipe\\client";
    argv[2] = "GDB";
    argv[3] = "127.0.0.1:8864";*/

    int iReturnCode = -1;
    WINBAGILITY_CONTEXT_T *pWinbagilityCtx = NULL;
    char *pStubOpenArg = NULL;

    if (argc != 4){
        Usage();
        goto Clean;
    }

    //This structure contains all information needed by WinDbg
    pWinbagilityCtx = (WINBAGILITY_CONTEXT_T*)malloc(sizeof(WINBAGILITY_CONTEXT_T));
    if (pWinbagilityCtx == NULL){
        printf("Failed to allocate pWinbagilityCtx\n");
        goto Clean;
    }

    //Store Named Pipe Name
    pWinbagilityCtx->pNamedPipePath = argv[1];
    pWinbagilityCtx->pStubName = argv[2];
    pWinbagilityCtx->pStubOpenArg = argv[3];

    //Set Cpu0 as default Cpu
    pWinbagilityCtx->CurrentCpuId = 0;
    if (CreateWinbagilityInterface(pWinbagilityCtx, pWinbagilityCtx->pStubName) == false){
        printf("Failed to CreateWinbagilityInterface(%s)\n", pWinbagilityCtx->pStubName);
        goto Clean;
    }

    //Connection to the stub
    pWinbagilityCtx->pUserHandle = NULL;
    pWinbagilityCtx->pUserHandle = pWinbagilityCtx->pfnOpen(pWinbagilityCtx->pStubOpenArg);
    if (pWinbagilityCtx->pUserHandle == NULL){
        printf("Failed to StubOpen(StubName:%s StubOpenArg:%s)\n", pWinbagilityCtx->pStubName, pWinbagilityCtx->pStubOpenArg);
        goto Clean;
    }

    //Init Stub
    if (pWinbagilityCtx->pfnInit(pWinbagilityCtx->pUserHandle) == false){
        printf("Failed to StubInit\n");
        goto Clean;
    }

    //LET'S START !!!
    if (InitialAnalysis(pWinbagilityCtx) == false){
        printf("Unable to initiale analysis !\n");
        goto Clean;
    }

    //Start Kd Server
    if (StartKdServer(pWinbagilityCtx) == false){
        printf("Something failed into Kd Server\n");
    }

    //todo clean & free !
    iReturnCode = 0;

Clean:
    if (iReturnCode == 0){
        StopKdServer(pWinbagilityCtx);
    }

    if (pWinbagilityCtx != NULL){
        free(pWinbagilityCtx);
    }
    getchar();
    return iReturnCode;
}

