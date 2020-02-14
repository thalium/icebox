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
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <Windows.h>

#include "WindowsProfils.h"
#include "FDP.h"
#include "kd.h"

#include "WindowsProfils.h"
#include "winbagility.h"
#include "kdserver.h"
#include "mmu.h"
#include "utils.h"
#include "dissectors.h"
#include "FDP.h"
#include "GDB.h"
#include "winbagility.h"

//Status
#define STATUS_SUCCESS        0x00000000
#define STATUS_UNSUCCESSFUL    0xc0000001

#define INITIAL_KD_SEQUENCE_NUMBER 0x80800800

//0x80000000 Unknown, no stop of WinDbg
//0x80000001 Guard page violation
//0x80000002 Data misaligned
//0x80000003 Break instruction exception 
//0x80000004 Single step exception
//0x80000005 Unknown, not stop of WinDbg
//0x80000006 Unknown, no stop of WinDbg
//0x80000007 ???
#define BREAK_INSTRUCTION_EXCEPTION_CODE    0x80000003
#define UNKNOWN_EXCEPTION_CODE              0x80000007
#define SINGLESTEP_EXCEPTION_CODE           0x80000004




//Simple Generic ReadRegister
uint64_t WDBG_ReadRegister(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, uint32_t CpuId, FDP_Register RegisterId){
    uint64_t RegisterValue;
    pWinbagilityCtx->pfnReadRegister(pWinbagilityCtx->pUserHandle, CpuId, RegisterId, &RegisterValue);
    return RegisterValue;
}


//TODO Move this !
/*
*@brief get the CpuId of cpu who break on a breakpoint
*/
uint32_t WDBG_GetBreakInCpu(WINBAGILITY_CONTEXT_T *pWinbagilityCtx)
{
    if (pWinbagilityCtx->CurrentMode == StubTypeFdp){
        FDP_State state;
        FDP_GetState((FDP_SHM*)pWinbagilityCtx->pUserHandle, &state);
        if (state | FDP_STATE_BREAKPOINT_HIT){
            for (uint32_t c = 0; c < pWinbagilityCtx->CpuCount; c++){
                FDP_State CpuState;
                FDP_GetCpuState((FDP_SHM*)pWinbagilityCtx->pUserHandle, c, &CpuState);
                if (CpuState & FDP_STATE_BREAKPOINT_HIT){
                    return c;
                }
            }
        }
    }
    return pWinbagilityCtx->CurrentCpuId;
}


/*
* This function ackowledges a specific KD packet
*
* @param pReqKdPkt KD packet to ackowledge
*
* @return sucess
*/
BOOL AckKDPkt(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    KD_PACKET_T *pRespKdPkt = (KD_PACKET_T*)pWinbagilityCtx->TmpOuputBuffer;

    pRespKdPkt->Leader = KD_CONTROL_PACKET;
    pRespKdPkt->Type = KD_PACKET_TYPE_ACK;
    pRespKdPkt->Length = 0x00;
    pRespKdPkt->Id = pReqKdPkt->Id;

    SendKDPacket(pWinbagilityCtx->hWinDbgPipe, pRespKdPkt);
    return true;
}


#define KPRCB_CURRENTTHREAD_OFFSET_X86 0x04
#define KPRCB_CURRENTTHREAD_OFFSET_X64 0x08
/*
* @brief Handle a Break packet
* The KD server has to reply with a KD_PACKET_TYPE_STATE_CHANGE
*
* @return success
*/
BOOL HandleBreakPkt(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, bool bIsBreak, bool bIsForcedBreak)
{
    Log1("[BREAK]\n");
    LogFlow();

    KD_PACKET_T *pRespKdPkt = (KD_PACKET_T*)pWinbagilityCtx->TmpOuputBuffer;
    uint32_t CurrentCpuId;
    FDP_State CpuState = (FDP_State)0;

    //Pause the VM
    pWinbagilityCtx->pfnPause(pWinbagilityCtx->pUserHandle);
    //Get the breakin Cpu
    CurrentCpuId = WDBG_GetBreakInCpu(pWinbagilityCtx);
    //Select the breakin Cpu
    pWinbagilityCtx->CurrentCpuId = CurrentCpuId;
    //Create ExceptionStateChange Pkt
    pRespKdPkt->Leader = KD_DATA_PACKET;
    pRespKdPkt->Type = KD_PACKET_TYPE_STATE_CHANGE;
    //TODO: sizeof !
    pRespKdPkt->Length = 244;
    pRespKdPkt->Id = INITIAL_KD_SEQUENCE_NUMBER;
    pRespKdPkt->StateChange.ApiNumber = DbgKdExceptionStateChange;
    pRespKdPkt->StateChange.NewState = 0x0006;
    pRespKdPkt->StateChange.ProcessorId = CurrentCpuId;
    pRespKdPkt->StateChange.ProcessorLevel = pWinbagilityCtx->CpuCount;
    pRespKdPkt->StateChange.Processor = 0;
    pRespKdPkt->StateChange.NumberProcessors = 1;

    //Read current thread address form KPRCB
    if (pWinbagilityCtx->pCurrentWindowsProfil->bIsX86 == true) {
        pWinbagilityCtx->pfnReadVirtualMemory(pWinbagilityCtx->pUserHandle,
            0,
            (uint8_t*)&pRespKdPkt->StateChange.Thread,
            sizeof(uint64_t),
            pWinbagilityCtx->v_KPRCB[0] + KPRCB_CURRENTTHREAD_OFFSET_X86);
        pRespKdPkt->StateChange.Thread |= 0xFFFFFFFF00000000;
    }
    else {
        pWinbagilityCtx->pfnReadVirtualMemory(pWinbagilityCtx->pUserHandle,
            0,
            (uint8_t*)&pRespKdPkt->StateChange.Thread,
            sizeof(uint64_t),
            pWinbagilityCtx->v_KPRCB[0] + KPRCB_CURRENTTHREAD_OFFSET_X64);
    }

    //Get ProgramCounter from Rip
    uint64_t ProgramCounter = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RIP_REGISTER);
    if (pWinbagilityCtx->pCurrentWindowsProfil->bIsX86 == true) {
        ProgramCounter |= 0xFFFFFFFF00000000;
    }
    pRespKdPkt->StateChange.ProgramCounter = ProgramCounter;


    //Get the CpuState from Stub
    pWinbagilityCtx->pfnGetCpuState(pWinbagilityCtx->pUserHandle, pWinbagilityCtx->CurrentCpuId, &CpuState);

    memset(&pRespKdPkt->StateChange.Exception, 0, sizeof(DBGKM_EXCEPTION64));

    //Compute the ExceptionCode
    pRespKdPkt->StateChange.Exception.ExceptionRecord.ExceptionCode = BREAK_INSTRUCTION_EXCEPTION_CODE;
    if (bIsForcedBreak == false){
        if (bIsForcedBreak == false &&
           ((CpuState & FDP_STATE_BREAKPOINT_HIT) || (bIsBreak == true))){
            pRespKdPkt->StateChange.Exception.ExceptionRecord.ExceptionCode = BREAK_INSTRUCTION_EXCEPTION_CODE;
            if ((CpuState & FDP_STATE_HARD_BREAKPOINT_HIT)){
                pRespKdPkt->StateChange.Exception.ExceptionRecord.ExceptionCode = SINGLESTEP_EXCEPTION_CODE;
            }
        }
    }

    //TODO: Generate good ExceptionRecord
    pRespKdPkt->StateChange.Exception.FirstChance = 0x00000001;

    pRespKdPkt->StateChange.Exception.ExceptionRecord.NumberParameters = 0x00000003;
    pRespKdPkt->StateChange.Exception.ExceptionRecord.ExceptionAddress = ProgramCounter;

    //Set minimal exception
    pRespKdPkt->StateChange.Exception.ExceptionRecord.ExceptionInformation[0] = 0;
    pRespKdPkt->StateChange.Exception.ExceptionRecord.ExceptionInformation[1] = pRespKdPkt->StateChange.Thread;
    pRespKdPkt->StateChange.Exception.ExceptionRecord.ExceptionInformation[2] = 0;

    pRespKdPkt->StateChange.ControlReport.Dr6 = (uint32_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DR6_REGISTER);
    pRespKdPkt->StateChange.ControlReport.Dr7 = (uint32_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DR7_REGISTER);
    pRespKdPkt->StateChange.ControlReport.EFlags = (ULONG)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RFLAGS_REGISTER);
    //Read following instructions
    pRespKdPkt->StateChange.ControlReport.InstructionCount = DBGKD_MAXSTREAM;
    pWinbagilityCtx->pfnReadVirtualMemory(pWinbagilityCtx->pUserHandle,
        0,
        pRespKdPkt->StateChange.ControlReport.InstructionStream,
        pRespKdPkt->StateChange.ControlReport.InstructionCount,
        ProgramCounter);
    pRespKdPkt->StateChange.ControlReport.ReportFlags = 0x0003;

    pRespKdPkt->StateChange.ControlReport.SegCs = (uint16_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_CS_REGISTER);
    pRespKdPkt->StateChange.ControlReport.SegDs = (uint16_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DS_REGISTER);
    pRespKdPkt->StateChange.ControlReport.SegEs = (uint16_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_ES_REGISTER);
    pRespKdPkt->StateChange.ControlReport.SegFs = (uint16_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_FS_REGISTER);

    //Send the packet
    SendKDPacket(pWinbagilityCtx->hWinDbgPipe, pRespKdPkt);

    return true;
}

/*
* Handle a Reset packet
* The KD server has to reply with a KD_PACKET_TYPE_RESET
*
* @return success
*/
BOOL HandleResetPkt(WINBAGILITY_CONTEXT_T *pWinbagilityCtx)
{
    Log1("[RESET]\n");
    LogFlow();

    KD_PACKET_T *pRespKdPkt = (KD_PACKET_T*)pWinbagilityCtx->TmpOuputBuffer;

    //TODO: Reset all server state !
    pRespKdPkt->Leader = KD_CONTROL_PACKET;
    pRespKdPkt->Type = KD_PACKET_TYPE_RESET;
    pRespKdPkt->Length = SIZEOF_KD_PACKET_HEADER;
    pRespKdPkt->Id = INITIAL_KD_SEQUENCE_NUMBER;

    if (pWinbagilityCtx->bIsConnectionEstablished == false){
        //Send Reset Ack while connection isn't established
        SendKDPacket(pWinbagilityCtx->hWinDbgPipe, pRespKdPkt);
    }
    return true;
}


/*
* Handle a Get Version packet
* The KD server has to reply with a KD_PACKET_TYPE_MANIP
*
* @return success
*/
BOOL HandleDbgKdGetVersionApiPkt(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    LogFlow();

    KD_PACKET_T *pRespKdPkt = (KD_PACKET_T*)pWinbagilityCtx->TmpOuputBuffer;

    uint32_t CurrentCpuId = pReqKdPkt->ManipulateState64.Processor;

    pRespKdPkt->Leader = KD_DATA_PACKET;
    pRespKdPkt->Type = KD_PACKET_TYPE_MANIP;
    pRespKdPkt->Length = SIZEOF_KD_PACKET_HEADER + sizeof(DBGKD_GET_VERSION_API64);
    pRespKdPkt->Id = pReqKdPkt->Id ^ 0x1;
    pRespKdPkt->ManipulateState64.ApiNumber = DbgKdGetVersionApi;
    pRespKdPkt->ManipulateState64.Processor = CurrentCpuId;
    pRespKdPkt->ManipulateState64.ProcessorLevel = pWinbagilityCtx->CpuCount;
    pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_SUCCESS;

    uint64_t v_KdVersionBlock = pWinbagilityCtx->v_KernBase + pWinbagilityCtx->pCurrentWindowsProfil->KdVersionBlockOffset;
    //Read nt!KdVersionBlock
    if (pWinbagilityCtx->pfnReadVirtualMemory(pWinbagilityCtx->pUserHandle,
        0,
        (uint8_t*)&pRespKdPkt->ManipulateState64.DbgGetVersion,
        sizeof(DBGKD_GET_VERSION_API64),
        v_KdVersionBlock) == false){
        //TODO: Create one from scratch...
    }
    
    SendKDPacket(pWinbagilityCtx->hWinDbgPipe, pRespKdPkt);

    return true;
}

//This value is invalid !!!!
#define SIZEOF_KDBG 0x370
BOOL HandleDbgKdReadVirtualMemoryApiPkt(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    LogFlow();

    uint32_t CurrentCpuId = pReqKdPkt->ManipulateState64.Processor;

    KD_PACKET_T *pRespKdPkt = (KD_PACKET_T*)pWinbagilityCtx->TmpOuputBuffer;

    pRespKdPkt->Leader = KD_DATA_PACKET;
    pRespKdPkt->Type = KD_PACKET_TYPE_MANIP;
    pRespKdPkt->Length = SIZEOF_KD_PACKET_HEADER + pReqKdPkt->ManipulateState64.ReadMemory.TransferCount;
    pRespKdPkt->Id = pReqKdPkt->Id ^ 0x1;
    pRespKdPkt->ManipulateState64.ApiNumber = DbgKdReadVirtualMemoryApi;
    pRespKdPkt->ManipulateState64.Processor = CurrentCpuId;
    pRespKdPkt->ManipulateState64.ProcessorLevel = pWinbagilityCtx->CpuCount;
    pRespKdPkt->ManipulateState64.ReadMemory.TargetBaseAddress = pReqKdPkt->ManipulateState64.ReadMemory.TargetBaseAddress;
    pRespKdPkt->ManipulateState64.ReadMemory.TransferCount = pReqKdPkt->ManipulateState64.ReadMemory.TransferCount;

    uint64_t TargetBaseAddress = pReqKdPkt->ManipulateState64.ReadMemory.TargetBaseAddress;
    if (pWinbagilityCtx->pCurrentWindowsProfil->bIsX86 == true) {
        TargetBaseAddress = TargetBaseAddress & 0xFFFFFFFF;
    }

    bool bIsReadSuccess = false;
    Log1("TargetbaseAddress %llx TransfertCount: %d\n", TargetBaseAddress, pReqKdPkt->ManipulateState64.ReadMemory.TransferCount);

    //Lot of possibilities here... 
    //Windbg read @v_KDBG or @v_KDBG+sizeof(DBGKD_DEBUG_DATA_HEADER64)
    //But what do I have to print when user want to read v_KDBG ? (Ciphered or Unciphered) ?
    if (pWinbagilityCtx->pCurrentWindowsProfil->bClearKdDebuggerDataBlock == false &&
        TargetBaseAddress >= pWinbagilityCtx->v_KDBG &&
        TargetBaseAddress < (pWinbagilityCtx->v_KDBG + sizeof(KDDEBUGGER_DATA64))
        //(TargetBaseAddress + pRespKdPkt->ManipulateState64.ReadMemory.TransferCount) <= (pWinbagilityCtx->v_KDBG + sizeof(KDDEBUGGER_DATA64))
        ){
        size_t u64OffsetInKDBG = (size_t)(TargetBaseAddress - pWinbagilityCtx->v_KDBG);
        //Check for Overflow
        if (u64OffsetInKDBG + pReqKdPkt->ManipulateState64.ReadMemory.TransferCount > sizeof(KDDEBUGGER_DATA64)){
            bIsReadSuccess = true;
            size_t uKDBGToRead = sizeof(KDDEBUGGER_DATA64)-u64OffsetInKDBG;
            //Read the end of KdDebuggerDataBlock
            memcpy(pRespKdPkt->ManipulateState64.ReadMemory.Data, ((uint8_t*)(&pWinbagilityCtx->KDBG)) + u64OffsetInKDBG, uKDBGToRead);
            //Read the end
            bIsReadSuccess = pWinbagilityCtx->pfnReadVirtualMemory(pWinbagilityCtx->pUserHandle,
                    0,
                    (uint8_t*)&pRespKdPkt->ManipulateState64.ReadMemory.Data + (sizeof(KDDEBUGGER_DATA64)-u64OffsetInKDBG),
                    (pReqKdPkt->ManipulateState64.ReadMemory.TransferCount - (sizeof(KDDEBUGGER_DATA64)-u64OffsetInKDBG)) & 0xFFFFFFFF,
                    TargetBaseAddress + (sizeof(KDDEBUGGER_DATA64)-u64OffsetInKDBG));
        }
        else{
            memcpy(pRespKdPkt->ManipulateState64.ReadMemory.Data, ((uint8_t*)(&pWinbagilityCtx->KDBG)) + u64OffsetInKDBG, pReqKdPkt->ManipulateState64.ReadMemory.TransferCount);
            bIsReadSuccess = true;
        }
    }
    else{

        if (pWinbagilityCtx->pCurrentWindowsProfil->bIsX86 == true &&
            TargetBaseAddress == pWinbagilityCtx->KdpDebuggerDataListHead &&
            pReqKdPkt->ManipulateState64.ReadMemory.TransferCount == 4) {
            //This address is not mapped when no /DEBUG in x86
            memcpy(pRespKdPkt->ManipulateState64.ReadMemory.Data, (uint8_t*)&pWinbagilityCtx->v_KDBG, sizeof(uint32_t));
            bIsReadSuccess = true;
        }
        else {
            //Move this !
            switch (TargetBaseAddress) {
            case FDP2KD_GET_VMNAME_MAGIC:
                //Get the name of the FDP Channel, used by extensions
                memset(pRespKdPkt->ManipulateState64.ReadMemory.Data, 0, pReqKdPkt->ManipulateState64.ReadMemory.TransferCount);
                memcpy(pRespKdPkt->ManipulateState64.ReadMemory.Data, pWinbagilityCtx->pStubOpenArg, strlen(pWinbagilityCtx->pStubOpenArg));
                bIsReadSuccess = true;
                break;
            case FDP2KD_FAKE_SINGLE_STEP_MAGIC:
                //Enable fake single-step, used by extensions
                pWinbagilityCtx->bFakeSingleStepEnabled = true;
                pRespKdPkt->ManipulateState64.ReadMemory.Data[0] = 0x01;
                bIsReadSuccess = true;
                break;
            default:
                //Normal memory read
                bIsReadSuccess = pWinbagilityCtx->pfnReadVirtualMemory(pWinbagilityCtx->pUserHandle,
                    0,
                    (uint8_t*)&pRespKdPkt->ManipulateState64.ReadMemory.Data,
                    pReqKdPkt->ManipulateState64.ReadMemory.TransferCount,
                    TargetBaseAddress);
                break;
            }
        }
    }

    if (bIsReadSuccess){
        pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_SUCCESS;
        pRespKdPkt->ManipulateState64.ReadMemory.ActualBytesRead = pReqKdPkt->ManipulateState64.ReadMemory.TransferCount;
    }
    else{
        pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_UNSUCCESSFUL;
        pRespKdPkt->ManipulateState64.ReadMemory.ActualBytesRead = 0;
    }

    SendKDPacket(pWinbagilityCtx->hWinDbgPipe, pRespKdPkt);

    return true;
}

BOOL HandleDbgKdWriteVirtualMemoryApiPkt(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    LogFlow();

    uint32_t CurrentCpuId = pReqKdPkt->ManipulateState64.Processor;

    KD_PACKET_T *pRespKdPkt = (KD_PACKET_T*)pWinbagilityCtx->TmpOuputBuffer;

    pRespKdPkt->Leader = KD_DATA_PACKET;
    pRespKdPkt->Type = KD_PACKET_TYPE_MANIP;
    pRespKdPkt->Length = SIZEOF_KD_PACKET_HEADER + pReqKdPkt->ManipulateState64.ReadMemory.TransferCount;
    pRespKdPkt->Id = pReqKdPkt->Id ^ 0x1;
    pRespKdPkt->ManipulateState64.ApiNumber = DbgKdWriteVirtualMemoryApi;
    pRespKdPkt->ManipulateState64.Processor = CurrentCpuId;
    pRespKdPkt->ManipulateState64.ProcessorLevel = pWinbagilityCtx->CpuCount;
    pRespKdPkt->ManipulateState64.WriteMemory.TargetBaseAddress = pReqKdPkt->ManipulateState64.WriteMemory.TargetBaseAddress;
    pRespKdPkt->ManipulateState64.WriteMemory.TransferCount = pReqKdPkt->ManipulateState64.WriteMemory.TransferCount;
    pRespKdPkt->ManipulateState64.WriteMemory.ActualBytesWritten = pReqKdPkt->ManipulateState64.WriteMemory.TransferCount;

    Log1("TargetbaseAddress %llx TransfertCount: %d\n", pRespKdPkt->ManipulateState64.WriteMemory.TargetBaseAddress, pReqKdPkt->ManipulateState64.WriteMemory.TransferCount);

    bool bIsWriteSucess = pWinbagilityCtx->pfnWriteVirtualMemory(pWinbagilityCtx->pUserHandle,
        0,
        pReqKdPkt->ManipulateState64.WriteMemory.Data,
        pReqKdPkt->ManipulateState64.WriteMemory.TransferCount,
        pReqKdPkt->ManipulateState64.WriteMemory.TargetBaseAddress);

    if (bIsWriteSucess){
        pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_SUCCESS;
        pRespKdPkt->ManipulateState64.WriteMemory.ActualBytesWritten = pReqKdPkt->ManipulateState64.WriteMemory.TransferCount;
    }
    else{
        pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_UNSUCCESSFUL;
        pRespKdPkt->ManipulateState64.WriteMemory.ActualBytesWritten = 0;
    }

    SendKDPacket(pWinbagilityCtx->hWinDbgPipe, pRespKdPkt);

    return true;
}

BOOL HandleDbgKdReadPhysicalMemoryApiPkt(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    LogFlow();

    uint32_t CurrentCpuId = pReqKdPkt->ManipulateState64.Processor;

    KD_PACKET_T *pRespKdPkt = (KD_PACKET_T*)pWinbagilityCtx->TmpOuputBuffer;

    pRespKdPkt->Leader = KD_DATA_PACKET;
    pRespKdPkt->Type = KD_PACKET_TYPE_MANIP;
    pRespKdPkt->Length = SIZEOF_KD_PACKET_HEADER + pReqKdPkt->ManipulateState64.ReadMemory.TransferCount;
    pRespKdPkt->Id = pReqKdPkt->Id ^ 0x1;
    pRespKdPkt->ManipulateState64.ApiNumber = DbgKdReadPhysicalMemoryApi;
    pRespKdPkt->ManipulateState64.Processor = CurrentCpuId;
    pRespKdPkt->ManipulateState64.ProcessorLevel = pWinbagilityCtx->CpuCount;
    pRespKdPkt->ManipulateState64.ReadMemory.TargetBaseAddress = pReqKdPkt->ManipulateState64.ReadMemory.TargetBaseAddress;
    pRespKdPkt->ManipulateState64.ReadMemory.TransferCount = pReqKdPkt->ManipulateState64.ReadMemory.TransferCount;
    pRespKdPkt->ManipulateState64.ReadMemory.ActualBytesRead = pReqKdPkt->ManipulateState64.ReadMemory.TransferCount;
    pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_SUCCESS;

    //TODO: handle read on p_KDBG, cipher or unciphered ?
    bool bResult = pWinbagilityCtx->pfnReadPhysicalMemory(pWinbagilityCtx->pUserHandle,
        pRespKdPkt->ManipulateState64.ReadMemory.Data,
        pReqKdPkt->ManipulateState64.ReadMemory.TransferCount,
        pReqKdPkt->ManipulateState64.ReadMemory.TargetBaseAddress);
    if (bResult == false){
        pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_UNSUCCESSFUL;
    }
    SendKDPacket(pWinbagilityCtx->hWinDbgPipe, pRespKdPkt);

    return true;
}

BOOL HandleDbgKdWritePhysicalMemoryApiPkt(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    LogFlow();

    KD_PACKET_T *pRespKdPkt = (KD_PACKET_T*)pWinbagilityCtx->TmpOuputBuffer;

    uint32_t CurrentCpuId = pReqKdPkt->ManipulateState64.Processor;

    pRespKdPkt->Leader = KD_DATA_PACKET;
    pRespKdPkt->Type = KD_PACKET_TYPE_MANIP;
    pRespKdPkt->Length = SIZEOF_KD_PACKET_HEADER + pReqKdPkt->ManipulateState64.ReadMemory.TransferCount;
    pRespKdPkt->Id = pReqKdPkt->Id ^ 0x1;
    pRespKdPkt->ManipulateState64.ApiNumber = DbgKdWritePhysicalMemoryApi;
    pRespKdPkt->ManipulateState64.Processor = CurrentCpuId;
    pRespKdPkt->ManipulateState64.ProcessorLevel = pWinbagilityCtx->CpuCount;
    pRespKdPkt->ManipulateState64.WriteMemory.TargetBaseAddress = pReqKdPkt->ManipulateState64.WriteMemory.TargetBaseAddress;
    pRespKdPkt->ManipulateState64.WriteMemory.TransferCount = pReqKdPkt->ManipulateState64.WriteMemory.TransferCount;
    pRespKdPkt->ManipulateState64.WriteMemory.ActualBytesWritten = pReqKdPkt->ManipulateState64.WriteMemory.TransferCount;
    pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_SUCCESS;

    bool bResult = pWinbagilityCtx->pfnWritePhysicalMemory(pWinbagilityCtx->pUserHandle,
        pReqKdPkt->ManipulateState64.WriteMemory.Data,
        pReqKdPkt->ManipulateState64.WriteMemory.TransferCount,
        pReqKdPkt->ManipulateState64.WriteMemory.TargetBaseAddress);

    if (bResult == false){
        pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_UNSUCCESSFUL;
    }
    SendKDPacket(pWinbagilityCtx->hWinDbgPipe, pRespKdPkt);

    return true;
}


BOOL HandleDbgKdWriteBreakPointApi(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    LogFlow();

    KD_PACKET_T *pRespKdPkt = (KD_PACKET_T*)pWinbagilityCtx->TmpOuputBuffer;

    uint32_t CurrentCpuId = pReqKdPkt->ManipulateState64.Processor;

    pRespKdPkt->Leader = KD_DATA_PACKET;
    pRespKdPkt->Type = KD_PACKET_TYPE_MANIP;
    pRespKdPkt->Length = SIZEOF_KD_PACKET_HEADER;
    pRespKdPkt->Id = pReqKdPkt->Id ^ 0x1;
    pRespKdPkt->ManipulateState64.ApiNumber = DbgKdWriteBreakPointApi;
    pRespKdPkt->ManipulateState64.Processor = CurrentCpuId;
    pRespKdPkt->ManipulateState64.ProcessorLevel = pWinbagilityCtx->CpuCount;
    pRespKdPkt->ManipulateState64.WriteBreakPoint.BreakPointAddress = pReqKdPkt->ManipulateState64.WriteBreakPoint.BreakPointAddress;
    pRespKdPkt->ManipulateState64.WriteBreakPoint.BreakPointHandle = 0;
    pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_UNSUCCESSFUL;
    for (int i = 0; i <= 32 - 5; i++){ //TODO: what da fuq ?
        pRespKdPkt->ManipulateState64.WriteBreakPoint.u[i] = pReqKdPkt->ManipulateState64.WriteBreakPoint.u[i];
    }

    if (pWinbagilityCtx->pCurrentWindowsProfil->bIsX86 == true) {
        pRespKdPkt->ManipulateState64.WriteBreakPoint.BreakPointAddress &= 0xFFFFFFFF;
    }

    int tmpBreakpointHandle = pWinbagilityCtx->pfnSetBreakpoint(pWinbagilityCtx->pUserHandle,
        CurrentCpuId,
        FDP_SOFTHBP,
        -1,
        FDP_EXECUTE_BP,
        FDP_VIRTUAL_ADDRESS,
        pRespKdPkt->ManipulateState64.WriteBreakPoint.BreakPointAddress,
        0x1,
        FDP_NO_CR3);
    if (tmpBreakpointHandle >= 0){
        //Breakpoint install success 
        pRespKdPkt->ManipulateState64.WriteBreakPoint.BreakPointHandle = tmpBreakpointHandle; //TODO dont use FDP handle here !
        pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_SUCCESS;
    }
    else{
        printf("Failed to SetBreakpoint\n");
    }

    Log1("BreakPointAddress %llx\n", pRespKdPkt->ManipulateState64.WriteBreakPoint.BreakPointAddress);

    SendKDPacket(pWinbagilityCtx->hWinDbgPipe, pRespKdPkt);
    return true;
}

BOOL HandleDbgKdRestoreBreakPointApi(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    LogFlow();

    KD_PACKET_T *pRespKdPkt = (KD_PACKET_T*)pWinbagilityCtx->TmpOuputBuffer;

    uint32_t CurrentCpuId = pReqKdPkt->ManipulateState64.Processor;

    pRespKdPkt->Leader = KD_DATA_PACKET;
    pRespKdPkt->Type = KD_PACKET_TYPE_MANIP;
    pRespKdPkt->Length = SIZEOF_KD_PACKET_HEADER;
    pRespKdPkt->Id = pReqKdPkt->Id ^ 0x1;
    pRespKdPkt->ManipulateState64.ApiNumber = DbgKdRestoreBreakPointApi;
    pRespKdPkt->ManipulateState64.Processor = CurrentCpuId;
    pRespKdPkt->ManipulateState64.ProcessorLevel = pWinbagilityCtx->CpuCount;
    pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_SUCCESS;

    //Unset the breakpoint !
    pWinbagilityCtx->pfnUnsetBreakpoint(pWinbagilityCtx->pUserHandle, pReqKdPkt->ManipulateState64.RestoreBreakPoint.BreakPointHandle);

    SendKDPacket(pWinbagilityCtx->hWinDbgPipe, pRespKdPkt);
    return true;
}

BOOL HandleDbgKdGetRegisterApi(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    LogFlow();

    KD_PACKET_T *pRespKdPkt = (KD_PACKET_T*)pWinbagilityCtx->TmpOuputBuffer;

    //XXX
    //TODO :
    //if(pWinbagilityCtx->pCurrentProfil->MajorVersion <= 7){
        //pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_SUCCESS;
    //else{
        pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_UNSUCCESSFUL;
    //}

    uint32_t CurrentCpuId = pReqKdPkt->ManipulateState64.Processor;

    pRespKdPkt->Leader = KD_DATA_PACKET;
    pRespKdPkt->Type = KD_PACKET_TYPE_MANIP;
    pRespKdPkt->Length = 1288;
    pRespKdPkt->Id = pReqKdPkt->Id ^ 0x1;
    pRespKdPkt->ManipulateState64.ApiNumber = DbgKdGetRegisterApi;
    pRespKdPkt->ManipulateState64.Processor = CurrentCpuId;
    pRespKdPkt->ManipulateState64.ProcessorLevel = pWinbagilityCtx->CpuCount;

    //XXX: Do not remove this ! Even with STATUS_UNSUCCESSFUL
    if (pWinbagilityCtx->pCurrentWindowsProfil->bIsX86 == true) {
        //TODO !
    }
    else {

        //TODO: What those values are ?
        pRespKdPkt->ManipulateState64.GetRegisters.u[0] = pReqKdPkt->ManipulateState64.GetRegisters.u[0];
        pRespKdPkt->ManipulateState64.GetRegisters.u[1] = pReqKdPkt->ManipulateState64.GetRegisters.u[1] + 0x4D0;
        pRespKdPkt->ManipulateState64.GetRegisters.u[2] = pReqKdPkt->ManipulateState64.GetRegisters.u[2];
        pRespKdPkt->ManipulateState64.GetRegisters.u[3] = pReqKdPkt->ManipulateState64.GetRegisters.u[3];
        pRespKdPkt->ManipulateState64.GetRegisters.u[4] = pReqKdPkt->ManipulateState64.GetRegisters.u[4];
        //What is this ?
        pRespKdPkt->ManipulateState64.GetRegisters.u[5] = 0x0000000000000000;
        pRespKdPkt->ManipulateState64.GetRegisters.u[6] = 0x0000000000000000;
        pRespKdPkt->ManipulateState64.GetRegisters.u[7] = 0x0000000000000000;
        pRespKdPkt->ManipulateState64.GetRegisters.u[8] = 0x0000000000000000;
        pRespKdPkt->ManipulateState64.GetRegisters.u[9] = 0x0000000000000000;
        pRespKdPkt->ManipulateState64.GetRegisters.u[10] = 0x0000000000000000;
        pRespKdPkt->ManipulateState64.GetRegisters.u[11] = 0x00001F800010001F;

        pRespKdPkt->ManipulateState64.GetRegisters.SegCs = (uint16_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_CS_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.SegDs = (uint16_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DS_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.SegEs = (uint16_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_ES_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.SegFs = (uint16_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_FS_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.SegGs = (uint16_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_GS_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.SegSs = (uint16_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_SS_REGISTER);

        pRespKdPkt->ManipulateState64.GetRegisters.Rip = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RIP_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.Rbp = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RBP_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.Rsp = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RSP_REGISTER);

        pRespKdPkt->ManipulateState64.GetRegisters.Rax = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RAX_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.Rbx = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RBX_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.Rcx = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RCX_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.Rdx = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RDX_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.Rsi = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RSI_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.Rdi = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RDI_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.R8 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_R8_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.R9 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_R9_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.R10 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_R10_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.R11 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_R11_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.R12 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_R12_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.R13 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_R13_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.R14 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_R14_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.R15 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_R15_REGISTER);

        pRespKdPkt->ManipulateState64.GetRegisters.EFlags = (uint32_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RFLAGS_REGISTER);

        pRespKdPkt->ManipulateState64.GetRegisters.Dr0 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DR0_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.Dr1 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DR1_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.Dr2 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DR2_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.Dr3 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DR3_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.Dr6 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DR6_REGISTER);
        pRespKdPkt->ManipulateState64.GetRegisters.Dr7 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DR7_REGISTER);

        pWinbagilityCtx->pfnGetFxState64(pWinbagilityCtx->pUserHandle, CurrentCpuId, (XSAVE_FORMAT64*)&pRespKdPkt->ManipulateState64.GetRegisters.FltSave);
    }

    SendKDPacket(pWinbagilityCtx->hWinDbgPipe, pRespKdPkt);
    return true;
}

BOOL HandleDbgKdSwitchProcessor(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    LogFlow();

    pWinbagilityCtx->CurrentCpuId = pReqKdPkt->ManipulateState64.Processor;

    HandleBreakPkt(pWinbagilityCtx, false, false);

    return true;
}

bool HandleDbgKdSetContextApi(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    LogFlow();

    KD_PACKET_T *pRespKdPkt = (KD_PACKET_T*)pWinbagilityCtx->TmpOuputBuffer;

    uint32_t CurrentCpuId = pReqKdPkt->ManipulateState64.Processor;

    pRespKdPkt->Leader = KD_DATA_PACKET;
    pRespKdPkt->Type = KD_PACKET_TYPE_MANIP;
    pRespKdPkt->Length = SIZEOF_KD_PACKET_HEADER;
    pRespKdPkt->Id = pReqKdPkt->Id ^ 0x1;
    pRespKdPkt->ApiNumber = DbgKdSetContextApi;
    pRespKdPkt->ManipulateState64.Processor = CurrentCpuId;
    pRespKdPkt->ManipulateState64.ProcessorLevel = pWinbagilityCtx->CpuCount;
    pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_SUCCESS;

    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_CS_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.SegCs);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_DS_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.SegDs);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_ES_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.SegEs);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_FS_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.SegFs);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_GS_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.SegGs);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_SS_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.SegSs);

    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_RIP_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.Rip);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_RBP_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.Rbp);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_RSP_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.Rsp);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_RAX_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.Rax);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_RBX_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.Rbx);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_RCX_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.Rcx);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_RDX_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.Rdx);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_RSI_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.Rsi);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_RDI_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.Rdi);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_R8_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.R8);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_R9_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.R9);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_R10_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.R10);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_R11_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.R11);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_R12_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.R12);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_R13_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.R13);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_R14_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.R14);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_R15_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.R15);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_RFLAGS_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.EFlags);

    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_DR0_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.Dr0);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_DR1_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.Dr1);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_DR2_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.Dr2);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_DR3_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.Dr3);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_DR6_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.Dr6);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_DR7_REGISTER, pReqKdPkt->ManipulateState64.GetContext.Context.Dr7);

    //TODO !
    //pWinbagilityCtx->pfnSetFxState64(pWinbagilityCtx->pUserHandle, CurrentCpuId, &pReqKdPkt->ManipulateState64.GetContext.Context.FltSave);

    SendKDPacket(pWinbagilityCtx->hWinDbgPipe, pRespKdPkt);
    return true;
}

#define OFFSETOF_KPRCB_KSPECIALREGISTERS64 0x40

BOOL HandleDbgKdReadControlSpaceApi(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    LogFlow();

    KD_PACKET_T *pRespKdPkt = (KD_PACKET_T*)pWinbagilityCtx->TmpOuputBuffer;

    uint32_t CurrentCpuId = pReqKdPkt->ManipulateState64.Processor;

    pRespKdPkt->Leader = KD_DATA_PACKET;
    pRespKdPkt->Type = KD_PACKET_TYPE_MANIP;
    pRespKdPkt->Length = SIZEOF_KD_PACKET_HEADER + pReqKdPkt->ManipulateState64.ReadMemory.TransferCount;
    pRespKdPkt->Id = pReqKdPkt->Id ^ 0x1;
    pRespKdPkt->ManipulateState64.ApiNumber = DbgKdReadControlSpaceApi;

    pRespKdPkt->ManipulateState64.Processor = CurrentCpuId;
    pRespKdPkt->ManipulateState64.ProcessorLevel = pWinbagilityCtx->CpuCount;

    pRespKdPkt->ManipulateState64.ReadMemory.TargetBaseAddress = pReqKdPkt->ManipulateState64.ReadMemory.TargetBaseAddress;

    if (pWinbagilityCtx->pCurrentWindowsProfil->bIsX86 == true) {
        pRespKdPkt->ManipulateState64.ReadMemory.TargetBaseAddress = pRespKdPkt->ManipulateState64.ReadMemory.TargetBaseAddress & 0xFFFFFFFF;
    }
    pRespKdPkt->ManipulateState64.ReadMemory.TransferCount = pReqKdPkt->ManipulateState64.ReadMemory.TransferCount;
    pRespKdPkt->ManipulateState64.ReadMemory.ActualBytesRead = pReqKdPkt->ManipulateState64.ReadMemory.TransferCount;
    pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_SUCCESS;

    Log1("TargetBaseAddress %llx TransfertCount: %d\n", pReqKdPkt->ManipulateState64.ReadMemory.TargetBaseAddress, pReqKdPkt->ManipulateState64.ReadMemory.TransferCount);

    if (pWinbagilityCtx->pCurrentWindowsProfil->bIsX86 == true) {
        pWinbagilityCtx->pfnReadVirtualMemory(pWinbagilityCtx->pUserHandle,
            0,
            pRespKdPkt->ManipulateState64.ReadMemory.Data,
            pReqKdPkt->ManipulateState64.ReadMemory.TransferCount,
            pWinbagilityCtx->v_KPRCB[0] + 0x18 + pRespKdPkt->ManipulateState64.ReadMemory.TargetBaseAddress);
    }
    else {
        switch (pReqKdPkt->ManipulateState64.ReadMemory.TargetBaseAddress){
        case 0: //v_KPCR
        {
            //Get the address of KPCR
            memcpy(pRespKdPkt->ManipulateState64.ReadMemory.Data, &pWinbagilityCtx->v_KPCR, sizeof(uint64_t));
            break;
        }
        case 1: //v_KPRCB
            //Get the address of KPRCB
            memcpy(pRespKdPkt->ManipulateState64.ReadMemory.Data, &pWinbagilityCtx->v_KPRCB[CurrentCpuId], sizeof(uint64_t));
            break;
        case 2: //SpecialRegisters
        {
            //TODO: 
            pWinbagilityCtx->pfnReadVirtualMemory(pWinbagilityCtx->pUserHandle,
                0,
                pRespKdPkt->ManipulateState64.ReadMemory.Data,
                pReqKdPkt->ManipulateState64.ReadMemory.TransferCount,
                pWinbagilityCtx->v_KPRCB[0] + 0x18 + pRespKdPkt->ManipulateState64.ReadMemory.TargetBaseAddress);

            _KPROCESSOR_STATE64 *tmpProcessorState = (_KPROCESSOR_STATE64*)pRespKdPkt->ManipulateState64.ReadMemory.Data;
            tmpProcessorState->SpecialRegisters.KernelDr0 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DR0_REGISTER);
            tmpProcessorState->SpecialRegisters.KernelDr1 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DR1_REGISTER);
            tmpProcessorState->SpecialRegisters.KernelDr2 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DR2_REGISTER);
            tmpProcessorState->SpecialRegisters.KernelDr3 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DR3_REGISTER);
            tmpProcessorState->SpecialRegisters.KernelDr6 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DR6_REGISTER);
            tmpProcessorState->SpecialRegisters.KernelDr7 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DR7_REGISTER);

            tmpProcessorState->SpecialRegisters.Gdtr.Base = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_GDTRB_REGISTER);
            tmpProcessorState->SpecialRegisters.Gdtr.Limit = (uint16_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_GDTRL_REGISTER);
            tmpProcessorState->SpecialRegisters.Idtr.Base = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_IDTRB_REGISTER);
            tmpProcessorState->SpecialRegisters.Idtr.Limit = (uint16_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_IDTRL_REGISTER);
            tmpProcessorState->SpecialRegisters.Tr = (uint16_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_TR_REGISTER);
            tmpProcessorState->SpecialRegisters.Ldtr = (uint16_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_LDTR_REGISTER);

            //tmpProcessorState->SpecialRegisters.MxCsr = 0x00001f80;
            //tmpProcessorState->SpecialRegisters.Xcr0 = 0x7;

            tmpProcessorState->SpecialRegisters.Cr0 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_CR0_REGISTER);
            tmpProcessorState->SpecialRegisters.Cr2 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_CR2_REGISTER);
            tmpProcessorState->SpecialRegisters.Cr3 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_CR3_REGISTER);
            tmpProcessorState->SpecialRegisters.Cr4 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_CR4_REGISTER);
            tmpProcessorState->SpecialRegisters.Cr8 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_CR8_REGISTER);

            break;
        }
         case 3: //v_KTHREAD
         {
             //Copy a pointer to the current thread
             uint64_t CurrentThreadPointer = 0;
             /*//TODO: WTF,Should be that..
             {
                 uint64_t CurrentThreadPointer = (pWinbagilityCtx->v_KPRCB[CurrentCpuId] + 0x8);
                 memcpy(pRespKdPkt->ManipulateState64.ReadMemory.Data, &CurrentThreadPointer, sizeof(CurrentThreadPointer));

                 pRespKdPkt->ManipulateState64.ReadMemory.TransferCount = sizeof(CurrentThreadPointer);
                 pRespKdPkt->ManipulateState64.ReadMemory.ActualBytesRead = sizeof(CurrentThreadPointer);
             }*/
             //But not indeed...
             {
                 pWinbagilityCtx->pfnReadVirtualMemory(pWinbagilityCtx->pUserHandle,
                     0,
                     (uint8_t*)&CurrentThreadPointer,
                     sizeof(CurrentThreadPointer),
                     pWinbagilityCtx->v_KPRCB[CurrentCpuId] + 0x8);
                 memcpy(pRespKdPkt->ManipulateState64.ReadMemory.Data, &CurrentThreadPointer, sizeof(CurrentThreadPointer));

                 CurrentThreadPointer = CurrentThreadPointer - 0x538D + 0x220; //TODO: what the fuck, reverse...

                 pRespKdPkt->ManipulateState64.ReadMemory.TransferCount = sizeof(CurrentThreadPointer);
                 pRespKdPkt->ManipulateState64.ReadMemory.ActualBytesRead = sizeof(CurrentThreadPointer);
             }
             break;
         }
         default:
             ParseKDPkt(pReqKdPkt);
         }
    }

    SendKDPacket(pWinbagilityCtx->hWinDbgPipe, pRespKdPkt);
    return true;
}

bool HandleDbgKdWriteControlSpaceApi(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    LogFlow();

    KD_PACKET_T *pRespKdPkt = (KD_PACKET_T*)pWinbagilityCtx->TmpOuputBuffer;

    uint32_t CurrentCpuId = pReqKdPkt->ManipulateState64.Processor;

    pRespKdPkt->Leader = KD_DATA_PACKET;
    pRespKdPkt->Type = KD_PACKET_TYPE_MANIP;
    pRespKdPkt->Length = 280;
    pRespKdPkt->Id = pReqKdPkt->Id ^ 0x1;
    pRespKdPkt->ApiNumber = DbgKdWriteControlSpaceApi;
    pRespKdPkt->ManipulateState64.Processor = CurrentCpuId;
    pRespKdPkt->ManipulateState64.ProcessorLevel = pWinbagilityCtx->CpuCount;

    pRespKdPkt->ManipulateState64.WriteMemory.TargetBaseAddress = pReqKdPkt->ManipulateState64.WriteMemory.TargetBaseAddress;
    pRespKdPkt->ManipulateState64.WriteMemory.TransferCount = pReqKdPkt->ManipulateState64.WriteMemory.TransferCount;
    pRespKdPkt->ManipulateState64.WriteMemory.ActualBytesWritten = pReqKdPkt->ManipulateState64.WriteMemory.TransferCount;
    pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_SUCCESS;

    switch (pReqKdPkt->ManipulateState64.ReadMemory.TargetBaseAddress){
    case 0: //@KPCR
        break;
    case 1: //@KPRCB
        break;
    case 2:{ //@SpecialRegisters
        KSPECIAL_REGISTERS64_T *tmpSpecialRegisters = (KSPECIAL_REGISTERS64_T*)pReqKdPkt->ManipulateState64.WriteMemory.Data;

        pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_DR0_REGISTER, tmpSpecialRegisters->KernelDr0);
        pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_DR1_REGISTER, tmpSpecialRegisters->KernelDr1);
        pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_DR2_REGISTER, tmpSpecialRegisters->KernelDr2);
        pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_DR3_REGISTER, tmpSpecialRegisters->KernelDr3);
        pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_DR6_REGISTER, tmpSpecialRegisters->KernelDr6);
        pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_DR7_REGISTER, tmpSpecialRegisters->KernelDr7);

        pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_CR0_REGISTER, tmpSpecialRegisters->Cr0);
        pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_CR2_REGISTER, tmpSpecialRegisters->Cr2);
        pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_CR3_REGISTER, tmpSpecialRegisters->Cr3);
        pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_CR4_REGISTER, tmpSpecialRegisters->Cr4);
        pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_CR8_REGISTER, tmpSpecialRegisters->Cr8);
        break;
    }
    case 3: //@KTHREAD
        break;
    default:
        ParseKDPkt(pReqKdPkt);
    }

    SendKDPacket(pWinbagilityCtx->hWinDbgPipe, pRespKdPkt);
    return true;
}


bool HandleDbgKdContinueApi(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    LogFlow();

    uint32_t CurrentCpuId = pReqKdPkt->ManipulateState64.Processor;

    printf("HandleDbgKdContinueApi\n");
    if (pWinbagilityCtx->bFakeSingleStepEnabled == true){
        Log1("[DEBUG] Fake Single-Step !\n");
        pWinbagilityCtx->bFakeSingleStepEnabled = false;
        HandleBreakPkt(pWinbagilityCtx, true, false);
    }
    else{
        if (pWinbagilityCtx->bSingleStep == true){
            pWinbagilityCtx->pfnSingleStep(pWinbagilityCtx->pUserHandle, CurrentCpuId);
            HandleBreakPkt(pWinbagilityCtx, false, false);
        }
        else{
            pWinbagilityCtx->pfnResume(pWinbagilityCtx->pUserHandle);
        }
    }

    return true;
}


bool HandleDbgKdContinueApi2(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    LogFlow();

    uint32_t CurrentCpuId = pReqKdPkt->ManipulateState64.Processor;

    pWinbagilityCtx->bSingleStep = false;

    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_DR7_REGISTER, pReqKdPkt->ManipulateState64.Continue2.Dr7);
    pWinbagilityCtx->pfnWriteRegister(pWinbagilityCtx->pUserHandle, CurrentCpuId, FDP_DR6_REGISTER, 0);

    if (pWinbagilityCtx->bFakeSingleStepEnabled == true){
        Log1("Fake a Single-Step !\n");
        pWinbagilityCtx->bFakeSingleStepEnabled = false;
        HandleBreakPkt(pWinbagilityCtx, true, false);
    }
    else{
        //if TraceFlag >= 1 then we single step
        if (pReqKdPkt->ManipulateState64.Continue2.TraceFlag >= 0x1){ //Not a Software Breakpoint !
            pWinbagilityCtx->bSingleStep = true;
            //Single step the current cpu
            pWinbagilityCtx->pfnSingleStep(pWinbagilityCtx->pUserHandle, CurrentCpuId);
            //Alert WinDbg that the single step is done 

            HandleBreakPkt(pWinbagilityCtx, true, false);
            pWinbagilityCtx->bSingleStep = false;
        }
        else{
            pWinbagilityCtx->pfnResume(pWinbagilityCtx->pUserHandle);
        }
    }
    return true;
}



bool HandleDbgKdQueryMemoryApi(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    LogFlow();

    KD_PACKET_T *pRespKdPkt = (KD_PACKET_T*)pWinbagilityCtx->TmpOuputBuffer;

    uint32_t CurrentCpuId = pReqKdPkt->ManipulateState64.Processor;

    pRespKdPkt->Leader = KD_DATA_PACKET;
    pRespKdPkt->Type = KD_PACKET_TYPE_MANIP;
    pRespKdPkt->Length = SIZEOF_KD_PACKET_HEADER;
    pRespKdPkt->Id = pReqKdPkt->Id ^ 0x1;
    pRespKdPkt->ManipulateState64.ApiNumber = DbgKdQueryMemoryApi;
    pRespKdPkt->ManipulateState64.Processor = CurrentCpuId;
    pRespKdPkt->ManipulateState64.ProcessorLevel = pWinbagilityCtx->CpuCount;
    pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_SUCCESS;

    pRespKdPkt->ManipulateState64.QueryMemory.Address = pReqKdPkt->ManipulateState64.QueryMemory.Address;
    pRespKdPkt->ManipulateState64.QueryMemory.AddressSpace = DBGKD_QUERY_MEMORY_KERNEL; //TODO: if K K else S
    //TODO: get real rigths from PTE
    pRespKdPkt->ManipulateState64.QueryMemory.Flags = DBGKD_QUERY_MEMORY_READ | DBGKD_QUERY_MEMORY_WRITE | DBGKD_QUERY_MEMORY_EXECUTE;

    SendKDPacket(pWinbagilityCtx->hWinDbgPipe, pRespKdPkt);
    return true;
}


//Read pWinbagilityCtx from PRCB
bool HandleDbgKdGetContextApi(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    LogFlow();

    KD_PACKET_T *pRespKdPkt = (KD_PACKET_T*)pWinbagilityCtx->TmpOuputBuffer;

    uint32_t CurrentCpuId = pReqKdPkt->ManipulateState64.Processor;

    pRespKdPkt->Leader = KD_DATA_PACKET;
    pRespKdPkt->Type = KD_PACKET_TYPE_MANIP;
    pRespKdPkt->Length = SIZEOF_KD_PACKET_HEADER + sizeof(CONTEXT64_T); //TODO: build specific !
    pRespKdPkt->Id = pReqKdPkt->Id ^ 0x1;
    pRespKdPkt->ManipulateState64.ApiNumber = DbgKdGetContextApi;
    pRespKdPkt->ManipulateState64.Processor = CurrentCpuId;
    pRespKdPkt->ManipulateState64.ProcessorLevel = pWinbagilityCtx->CpuCount;
    pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_SUCCESS;

    if (pWinbagilityCtx->pCurrentWindowsProfil->bIsX86 == true) {
        //TODO: Make a struct !
        uint32_t *pTest = (uint32_t*)&pRespKdPkt->ManipulateState64.GetContext;
        pTest[0x36] = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RAX_REGISTER) & 0xFFFFFFFF;
        pTest[0x33] = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RBX_REGISTER) & 0xFFFFFFFF;
        pTest[0x35] = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RCX_REGISTER) & 0xFFFFFFFF;
        pTest[0x34] = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RDX_REGISTER) & 0xFFFFFFFF;
        pTest[0x32] = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RSI_REGISTER) & 0xFFFFFFFF;
        pTest[0x31] = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RDI_REGISTER) & 0xFFFFFFFF;
        pTest[0x38] = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RIP_REGISTER) & 0xFFFFFFFF;
        pTest[0x3b] = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RSP_REGISTER) & 0xFFFFFFFF;
        pTest[0x37] = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RBP_REGISTER) & 0xFFFFFFFF;
        pTest[0x39] = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_CS_REGISTER) & 0xFFFFFFFF;
        pTest[0x3c] = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_SS_REGISTER) & 0xFFFFFFFF;
        pTest[0x30] = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DS_REGISTER) & 0xFFFFFFFF;
        pTest[0x2f] = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_ES_REGISTER) & 0xFFFFFFFF;
        pTest[0x2e] = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_FS_REGISTER) & 0xFFFFFFFF;
        pTest[0x2d] = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_GS_REGISTER) & 0xFFFFFFFF;
        pTest[0x3a] = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RFLAGS_REGISTER) & 0xFFFFFFFF;
    }
    else {
        pRespKdPkt->ManipulateState64.GetContext.u[0] = 0xDEADCACADEADCACA;
        pRespKdPkt->ManipulateState64.GetContext.u[1] = 0xDEADCACADEADCACA;
        pRespKdPkt->ManipulateState64.GetContext.u[2] = 0xDEADCACADEADCACA;

        pRespKdPkt->ManipulateState64.GetContext.Context.SegCs = (uint16_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_CS_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.SegDs = (uint16_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DS_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.SegEs = (uint16_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_ES_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.SegFs = (uint16_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_FS_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.SegGs = (uint16_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_GS_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.SegSs = (uint16_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_SS_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.Rip = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RIP_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.Rbp = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RBP_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.Rsp = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RSP_REGISTER);

        pRespKdPkt->ManipulateState64.GetContext.Context.Rax = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RAX_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.Rbx = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RBX_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.Rcx = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RCX_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.Rdx = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RDX_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.Rsi = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RSI_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.Rdi = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RDI_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.R8 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_R8_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.R9 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_R9_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.R10 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_R10_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.R11 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_R11_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.R12 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_R12_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.R13 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_R13_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.R14 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_R14_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.R15 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_R15_REGISTER);

        pRespKdPkt->ManipulateState64.GetContext.Context.EFlags = (uint32_t)WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_RFLAGS_REGISTER);

        pRespKdPkt->ManipulateState64.GetContext.Context.Dr0 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DR0_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.Dr1 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DR1_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.Dr2 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DR2_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.Dr3 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DR3_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.Dr6 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DR6_REGISTER);
        pRespKdPkt->ManipulateState64.GetContext.Context.Dr7 = WDBG_ReadRegister(pWinbagilityCtx, CurrentCpuId, FDP_DR7_REGISTER);

        pWinbagilityCtx->pfnGetFxState64(pWinbagilityCtx->pUserHandle, CurrentCpuId, &pRespKdPkt->ManipulateState64.GetContext.Context.FltSave);
    }
    

    SendKDPacket(pWinbagilityCtx->hWinDbgPipe, pRespKdPkt);
    return true;
}

bool HandleDbgKdSearchMemoryApi(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    LogFlow();

    KD_PACKET_T *pRespKdPkt = (KD_PACKET_T*)pWinbagilityCtx->TmpOuputBuffer;

    uint32_t CurrentCpuId = pReqKdPkt->ManipulateState64.Processor;

    pRespKdPkt->Leader = KD_DATA_PACKET;
    pRespKdPkt->Type = KD_PACKET_TYPE_MANIP;
    pRespKdPkt->Length = SIZEOF_KD_PACKET_HEADER;
    pRespKdPkt->Id = pReqKdPkt->Id ^ 0x1;
    pRespKdPkt->ManipulateState64.ApiNumber = DbgKdSearchMemoryApi;
    pRespKdPkt->ManipulateState64.Processor = CurrentCpuId;
    pRespKdPkt->ManipulateState64.ProcessorLevel = pWinbagilityCtx->CpuCount;

    //Effective Search
    uint64_t u64FoundAddress;
    bool bIsFound = WDBG_SearchVirtualMemory(pWinbagilityCtx,
        pReqKdPkt->ManipulateState64.SearchMemory.Data,
        pReqKdPkt->ManipulateState64.SearchMemory.PatternLength,
        pReqKdPkt->ManipulateState64.SearchMemory.SearchAddress,
        pReqKdPkt->ManipulateState64.SearchMemory.SearchLength, &u64FoundAddress);

    if (bIsFound){
        pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_SUCCESS;
        pRespKdPkt->ManipulateState64.SearchMemory.FoundAddress = u64FoundAddress;
    }
    else{
        pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_UNSUCCESSFUL;
        pRespKdPkt->ManipulateState64.SearchMemory.FoundAddress = 0;
    }
    pRespKdPkt->ManipulateState64.SearchMemory.SearchLength = pReqKdPkt->ManipulateState64.SearchMemory.SearchLength;
    pRespKdPkt->ManipulateState64.SearchMemory.PatternLength = pReqKdPkt->ManipulateState64.SearchMemory.PatternLength;

    SendKDPacket(pWinbagilityCtx->hWinDbgPipe, pRespKdPkt);
    return true;
}


bool HandleDbgKdReadMachineSpecificRegister(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    LogFlow();

    KD_PACKET_T *pRespKdPkt = (KD_PACKET_T*)pWinbagilityCtx->TmpOuputBuffer;

    uint32_t CurrentCpuId = 0;
    //TODO: Not Working here why ???
    //CurrentCpuId = pReqKdPkt->ManipulateState64.Processor;

    pRespKdPkt->Leader = KD_DATA_PACKET;
    pRespKdPkt->Type = KD_PACKET_TYPE_MANIP;
    pRespKdPkt->Length = SIZEOF_KD_PACKET_HEADER;
    pRespKdPkt->Id = pReqKdPkt->Id ^ 0x1;
    pRespKdPkt->ManipulateState64.ApiNumber = DbgKdReadMachineSpecificRegister;
    pRespKdPkt->ManipulateState64.Processor = CurrentCpuId;
    pRespKdPkt->ManipulateState64.ProcessorLevel = pWinbagilityCtx->CpuCount;
    pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_SUCCESS;

    memset(pRespKdPkt->ManipulateState64.data, 0, 512);

    pRespKdPkt->ManipulateState64.ReadWriteMsr.Msr = pReqKdPkt->ManipulateState64.ReadWriteMsr.Msr;
    uint64_t MSRValue;
    pWinbagilityCtx->pfnReadMsr(pWinbagilityCtx->pUserHandle, CurrentCpuId, pReqKdPkt->ManipulateState64.ReadWriteMsr.Msr, &MSRValue);

    pRespKdPkt->ManipulateState64.ReadWriteMsr.DataValueHigh = (MSRValue >> 32);
    pRespKdPkt->ManipulateState64.ReadWriteMsr.DataValueLow = (MSRValue & 0xFFFFFFFF);

    SendKDPacket(pWinbagilityCtx->hWinDbgPipe, pRespKdPkt);
    return true;
}

bool HandleDbgKdWriteMachineSpecificRegister(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    LogFlow();

    KD_PACKET_T *pRespKdPkt = (KD_PACKET_T*)pWinbagilityCtx->TmpOuputBuffer;

    uint32_t CurrentCpuId = 0;
    //TODO: Not Working here why ???
    //CurrentCpuId = pReqKdPkt->ManipulateState64.Processor;

    pRespKdPkt->Leader = KD_DATA_PACKET;
    pRespKdPkt->Type = KD_PACKET_TYPE_MANIP;
    pRespKdPkt->Length = SIZEOF_KD_PACKET_HEADER;
    pRespKdPkt->Id = pReqKdPkt->Id ^ 0x1;
    pRespKdPkt->ManipulateState64.ApiNumber = DbgKdWriteMachineSpecificRegister;
    pRespKdPkt->ManipulateState64.Processor = CurrentCpuId;
    pRespKdPkt->ManipulateState64.ProcessorLevel = pWinbagilityCtx->CpuCount;
    pRespKdPkt->ManipulateState64.ReturnStatus = STATUS_SUCCESS;

    memset(pRespKdPkt->ManipulateState64.data, 0, 512);

    pRespKdPkt->ManipulateState64.ReadWriteMsr.Msr = pReqKdPkt->ManipulateState64.ReadWriteMsr.Msr;
    uint64_t MSRValue = _rotl64(pReqKdPkt->ManipulateState64.ReadWriteMsr.DataValueHigh, 32) | pReqKdPkt->ManipulateState64.ReadWriteMsr.DataValueLow;

    pWinbagilityCtx->pfnWriteMsr(pWinbagilityCtx->pUserHandle, CurrentCpuId, pReqKdPkt->ManipulateState64.ReadWriteMsr.Msr, MSRValue);

    SendKDPacket(pWinbagilityCtx->hWinDbgPipe, pRespKdPkt);
    return true;
}

bool HandleDbgKdRebootApi(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    LogFlow();

    pWinbagilityCtx->pfnReboot(pWinbagilityCtx->pUserHandle);

    return false;
}

bool HandleDbgKdTimeout(WINBAGILITY_CONTEXT_T *pWinbagilityCtx)
{
    //Timeout let's poke the VM !
    if (pWinbagilityCtx->CurrentMode == StubTypeFdp){
        if (FDP_GetStateChanged((FDP_SHM*)pWinbagilityCtx->pUserHandle) == true){
            Log1("[DEBUG] State changed !\n");
            //Retreive the state of the machine
            FDP_State currentState;
            FDP_GetState((FDP_SHM*)pWinbagilityCtx->pUserHandle, &currentState);

            //Check for breakpoint
            if ((currentState & FDP_STATE_BREAKPOINT_HIT)
                && !(currentState & FDP_STATE_DEBUGGER_ALERTED)){
                Log1("[DEBUG] Breakpoint Hitted !\n");
                //Need a break !
                HandleBreakPkt(pWinbagilityCtx, true, false);
            }
            else{
                //Check reboot
                uint64_t CurrentIdtrb;
                //TODO: FDP_STATE_REBOOTED
                FDP_ReadRegister((FDP_SHM*)pWinbagilityCtx->pUserHandle, 0, FDP_IDTRB_REGISTER, &CurrentIdtrb);
                //TODO !
                if (CurrentIdtrb != pWinbagilityCtx->StartIdtrb){
                    //TODO: This is shit !
                    Sleep(10000);
                    do{
                        Sleep(1000);
                    } while (InitialAnalysis(pWinbagilityCtx) == false);

                    //Restart a new connection !
                    pWinbagilityCtx->bIsConnectionEstablished = false;
                    HandleResetPkt(pWinbagilityCtx);
                    FDP_Pause((FDP_SHM*)pWinbagilityCtx->pUserHandle);
                    HandleBreakPkt(pWinbagilityCtx, false, false);
                }
            }
        }
    }

    if (pWinbagilityCtx->CurrentMode == StubTypeGdb){
        if (GDB_GetStateChanged((GDB_TYPE_T*)pWinbagilityCtx->pUserHandle) == true){
            printf("Breakpoint Hitted !\n");
            //Need a break !
            HandleBreakPkt(pWinbagilityCtx, true, false);
        }
    }
/*    else {
#if WINBAGILITY_POWER_SAVE 1
        //there is nothing else to do...
        Sleep(5);
#endif
    }*/
    return true;
}

bool DispatchKdKdPacket(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, KD_PACKET_T *pReqKdPkt)
{
    switch (pReqKdPkt->Type){
    case KD_PACKET_TYPE_ACK:
        //Don't need to manage lost packet... We are on a reliable communication !
        break;
    case KD_PACKET_TYPE_RESET:
        HandleResetPkt(pWinbagilityCtx);
        HandleBreakPkt(pWinbagilityCtx, false, false);
        break;
    case KD_PACKET_TYPE_MANIP:
    {
        //We are sur that the connection is established !
        pWinbagilityCtx->bIsConnectionEstablished = true;

        switch (pReqKdPkt->ApiNumber)
        {
        case DbgKdGetVersionApi:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            HandleDbgKdGetVersionApiPkt(pWinbagilityCtx, pReqKdPkt);
            break;
        case DbgKdReadVirtualMemoryApi:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            HandleDbgKdReadVirtualMemoryApiPkt(pWinbagilityCtx, pReqKdPkt);
            break;
        case DbgKdWriteVirtualMemoryApi:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            HandleDbgKdWriteVirtualMemoryApiPkt(pWinbagilityCtx, pReqKdPkt);
            break;
        case DbgKdReadPhysicalMemoryApi:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            HandleDbgKdReadPhysicalMemoryApiPkt(pWinbagilityCtx, pReqKdPkt);
            break;
        case DbgKdWritePhysicalMemoryApi:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            HandleDbgKdWritePhysicalMemoryApiPkt(pWinbagilityCtx, pReqKdPkt);
            break;
        case DbgKdReadControlSpaceApi:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            HandleDbgKdReadControlSpaceApi(pWinbagilityCtx, pReqKdPkt);
            break;
        case DbgKdWriteBreakPointApi:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            HandleDbgKdWriteBreakPointApi(pWinbagilityCtx, pReqKdPkt);
            break;
        case DbgKdRestoreBreakPointApi:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            HandleDbgKdRestoreBreakPointApi(pWinbagilityCtx, pReqKdPkt);
            break;
        case DbgKdClearAllInternalBreakpointsApi:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            //TODO: HandleDbgKdClearAllInternalBreakpointsApi
            break;
        case DbgKdGetRegisterApi:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            HandleDbgKdGetRegisterApi(pWinbagilityCtx, pReqKdPkt);
            break;
        case DbgKdSwitchProcessor:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            HandleDbgKdSwitchProcessor(pWinbagilityCtx, pReqKdPkt);
            break;
        case DbgKdSetContextApi:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            HandleDbgKdSetContextApi(pWinbagilityCtx, pReqKdPkt);
            break;
        case DbgKdWriteControlSpaceApi:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            HandleDbgKdWriteControlSpaceApi(pWinbagilityCtx, pReqKdPkt);
            break;
        case DbgKdContinueApi:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            HandleDbgKdContinueApi(pWinbagilityCtx, pReqKdPkt);
            break;
        case DbgKdContinueApi2:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            HandleDbgKdContinueApi2(pWinbagilityCtx, pReqKdPkt);
            break;
        case DbgKdQueryMemoryApi:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            HandleDbgKdQueryMemoryApi(pWinbagilityCtx, pReqKdPkt);
            break;
        case DbgKdGetContextApi:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            HandleDbgKdGetContextApi(pWinbagilityCtx, pReqKdPkt);
            break;
        case DbgKdSearchMemoryApi:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            HandleDbgKdSearchMemoryApi(pWinbagilityCtx, pReqKdPkt);
            break;
        case DbgKdReadMachineSpecificRegister:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            HandleDbgKdReadMachineSpecificRegister(pWinbagilityCtx, pReqKdPkt);
            break;
        case DbgKdWriteMachineSpecificRegister:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            HandleDbgKdWriteMachineSpecificRegister(pWinbagilityCtx, pReqKdPkt);
            break;
        case DbgKdRebootApi:
            AckKDPkt(pWinbagilityCtx, pReqKdPkt);
            HandleDbgKdRebootApi(pWinbagilityCtx, pReqKdPkt);
            break;
        default:
            printf("[DEBUG] Unknown ApiNumber %08x\n", pReqKdPkt->ApiNumber);
            ParseKDPkt(pReqKdPkt);
            break;
        }
        break;
    }
    default:
        printf("[DEBUG] Unknown Type %d\n", pReqKdPkt->Type);
        ParseKDPkt(pReqKdPkt);
        break;
    }

    return true;
}

DWORD WINAPI KdMainLoop(LPVOID lpParam)
{
    WINBAGILITY_CONTEXT_T *pWinbagilityCtx = (WINBAGILITY_CONTEXT_T *)lpParam;
    KD_PACKET_T *pReqKdPkt = (KD_PACKET_T*)pWinbagilityCtx->TmpInputBuffer;

    //Set the connection as not established !
    pWinbagilityCtx->bIsConnectionEstablished = false;

    printf("Starting Fake-VM KD Server\n");
    uint32_t TimeoutCount = 0;
    while (pWinbagilityCtx->ServerRunning){
        //Read one Kd Packet
        KdPacketType KdPktType = ReadKDPipe(pWinbagilityCtx->hWinDbgPipe, pReqKdPkt);
        switch (KdPktType){
        case KdNoPacket:
            HandleDbgKdTimeout(pWinbagilityCtx);
            if ((TimeoutCount & 0xFFFF) == 0xFFFF){
#ifdef WINBAGILITY_POWER_SAVE
                Sleep(5);
#endif
            }
            else{
                TimeoutCount++;
            }
            break;
        case KdKdPacket:
            TimeoutCount = 0;
            DispatchKdKdPacket(pWinbagilityCtx, pReqKdPkt);
            break;
        case KdBreakinPacket:
            TimeoutCount = 0;
            HandleBreakPkt(pWinbagilityCtx, true, true);
            break;
        case KdErrorPacket:
            break;
        default:
            break;
        }
    }

    return 0;
}




bool StartKdServer(WINBAGILITY_CONTEXT_T *pWinbagilityCtx)
{
    pWinbagilityCtx->ServerRunning = true;
    pWinbagilityCtx->hMainLoopThread = NULL;
    while (pWinbagilityCtx->ServerRunning){
        HANDLE hPipe;
        //Wait for a new client
        CreateConnectNamedPipe(&hPipe, pWinbagilityCtx->pNamedPipePath);
        if (hPipe == INVALID_HANDLE_VALUE){
            printf("Failed to CreateNamedPipe !\n");
            return false;
        }
        if (pWinbagilityCtx->hMainLoopThread){
            //TODO: Terminate the thread ...
            pWinbagilityCtx->ServerRunning = false;
            //TODO WaitForSingleObject(pWinbagilityCtx->hMainLoopThread);
            //DisconnectNamedPipe(pWinbagilityCtx->hWinDbgPipe);
            Sleep(100);
            pWinbagilityCtx->ServerRunning = true;
        }
        pWinbagilityCtx->hWinDbgPipe = hPipe;
        pWinbagilityCtx->hMainLoopThread = CreateThread(NULL, 0, KdMainLoop, pWinbagilityCtx, 0, NULL);
    }
    return true;
}

void StopKdServer(WINBAGILITY_CONTEXT_T *pWinbagilityCtx)
{
    pWinbagilityCtx->ServerRunning = false;

    //TODO WaitForSingleObject(pWinbagilityCtx->hMainLoopThread);
}
