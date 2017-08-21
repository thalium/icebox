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


#include "FDP.h"
#include "WindowsProfils.h"
#include "kdserver.h"
#include "kd.h"
#include "mmu.h"
#include "utils.h"

//TODO: configuration file
uint8_t DEBUG_PKT = 0;

/*
 * Compute the Checksum of a KD packet
 *
 * @param pkt the KD packet
 * @return Checksum of the packet
 */
uint32_t ChecksumKD(KD_PACKET_T *pKdPkt)
{
    uint32_t Checksum = 0;
    for (int i = 0; i<pKdPkt->Length; i++){
        Checksum = Checksum + pKdPkt->data[i];
    }
    return Checksum;
}



#define BREAKIN_SHORTLEADER 0x62
#define TYPE1_SHORTLEADER   0x69
#define TYPE2_SHORTLEADER   0x30
#define TYPE1_LONGLEADER    0x69696969
#define TYPE2_LONGLEADER    0x30303030

/*
 * Read a pkt from a Named Pipe
 *
 * @param hPipe the Named Pipe to read from
 * @param pktBuffer the destination buffer, the pkt will be in this buffer
 *
 * @return Type of pkt (ERR, KD_PKT, FASTBREAK_PKT)
 */
KdPacketType ReadKDPipe(HANDLE hPipe, KD_PACKET_T *pktBuffer)
{
    DWORD numBytesRead = 0;
    BOOL result;
    UINT8 firstCMD = 0x00;
    do{
        bool dataRead = GetPipeTry(hPipe, &firstCMD, sizeof(uint8_t), true);
        if (dataRead == false){
            return KdNoPacket;
        }
    } while (firstCMD != TYPE1_SHORTLEADER && firstCMD != TYPE2_SHORTLEADER && firstCMD != BREAKIN_SHORTLEADER);

    if (firstCMD == BREAKIN_SHORTLEADER){
        //This is a BreakIn short packet
        return KdBreakinPacket;
    }

    //Read the end of the Leader
    uint32_t LeaderEnd = 0;
    GetPipe(hPipe, (uint8_t*)&LeaderEnd, 3);
    uint32_t u32Leader = (firstCMD << 24) | LeaderEnd;
    if (u32Leader == TYPE1_LONGLEADER
        || u32Leader == TYPE2_LONGLEADER){

        pktBuffer->Leader = u32Leader;

        //Read header
        GetPipe(hPipe, (uint8_t*)&(pktBuffer->Type), 12);

        //TODO: function !
        uint32_t bytesToRead = pktBuffer->Length;
        uint32_t bytesAlreadyRead = 0;
        while (bytesToRead > 0){
            result = ReadFile(hPipe, pktBuffer->data + bytesAlreadyRead, bytesToRead, &numBytesRead, NULL);
            bytesToRead = bytesToRead - numBytesRead;
            bytesAlreadyRead = bytesAlreadyRead + numBytesRead;
        }

        //END_OF_DATA
        if (pktBuffer->Length > 0){
            //Trick to avoid segfault on Windows 7 ???
            uint8_t tmpBuffer[1024];
            DWORD test = 0;
            if (ReadFile(hPipe, tmpBuffer, 1, &test, NULL) == false){
                printf("Error while reading file !\n");
            }
        }
        if (DEBUG_PKT){
            ParseKDPkt(pktBuffer);
        }
        return KdKdPacket;
    }else{
        UINT16 type = Get16Pipe(hPipe);
        printf("Unknown Leader %08x\n", u32Leader);
        printf("type: %04x\n", type);
    }

    return KdErrorPacket;
}

/*
 * Write a KD packet on a Named Pipe
 *
 * @param hPipe the Named Pipe to write on
 * @prama pkt the KD packet to send
 *
 * @return number of bytes written on the Named Pipe
 */
DWORD WriteKDPipe(HANDLE hPipe, KD_PACKET_T *pkt)
{
    DWORD numBytesWritten = 0;
    BOOL result = WriteFile(hPipe, pkt, pkt->Length + 16, &numBytesWritten, NULL);
    if (result == false){
        return 0;
    }

    uint8_t tmpBuffer[1024];
    tmpBuffer[0] = 0xAA;
    //END_OF_DATA
    if (pkt->Length > 0){
        DWORD test = 0;
        WriteFile(hPipe, tmpBuffer, 1, &test, NULL);
    }

    FlushFileBuffers(hPipe);

    return numBytesWritten;
}

/*
* Send a pkt through a Named Pipe
*
* @param hPipe the Named Pipe
* @param toSendKDPkt ...
*
* @return bool True if the sent success
*/
bool SendKDPacket(HANDLE hPipe, KD_PACKET_T* pKdPkt)
{
    //Compute Checksum before sending pkt
    pKdPkt->Checksum = ChecksumKD(pKdPkt);
    DWORD numBytesWritten = WriteKDPipe(hPipe, pKdPkt);
    if (numBytesWritten == 0){
        return false;
    }
    if (DEBUG_PKT){
        ParseKDPkt(pKdPkt);
    }
    return true;
}


bool ParseKDPkt(KD_PACKET_T* pkt)
{
    printf("------------RAW--------------\n");
    dumpHexData((char*)pkt, pkt->Length + 16);
    printf("-----------------------------\n");
    printf("---------KD_HEADER-----------\n");
    printf("Leader: %08x\n", pkt->Leader);
    printf("PacketType: %04x\n", pkt->Type);
    printf("DataSize: %d\n", pkt->Length);
    printf("PacketID: %08x\n", pkt->Id);
    printf("Checksum: %08x\n", pkt->Checksum);
    printf("Checksum(check): %08x\n", ChecksumKD(pkt));
    if (pkt->Length > 0){
        printf("\t---------KD_CONTENT-----------\n");
        printf("\tApiNumber %08x\n", pkt->ApiNumber);
        if (pkt->Type == KD_PACKET_TYPE_MANIP){
            printf("\t\t---------KD_MANIP-----------\n");
            printf("\t\tProcessorLevel: %04x\n", pkt->ManipulateState64.ProcessorLevel);
            printf("\t\tProcessor: %04x\n", pkt->ManipulateState64.Processor);
            printf("\t\tProcessor: %04x\n", pkt->ManipulateState64.Processor);
            printf("\t\tReturnStatus: %08x\n", pkt->ManipulateState64.ReturnStatus);
            dumpHexData((char*)pkt->ManipulateState64.data, pkt->Length - 12);
            printf("\t\t----------------------------\n");
        }
        switch (pkt->ApiNumber){
        case DbgKdGetVersionApi:
            printf("\t[DbgKdGetVersionApi]\n");
            printf("\tMajorVersion %04x\n", pkt->ManipulateState64.DbgGetVersion.MajorVersion);
            printf("\tMinorVersion %04x\n", pkt->ManipulateState64.DbgGetVersion.MinorVersion);
            printf("\tProtocolVersion %04x\n", pkt->ManipulateState64.DbgGetVersion.ProtocolVersion);
            printf("\tFlags %04x\n", pkt->ManipulateState64.DbgGetVersion.Flags);
            printf("\tMachineType %04x\n", pkt->ManipulateState64.DbgGetVersion.MachineType);
            printf("\tMaxPacketType %02x\n", pkt->ManipulateState64.DbgGetVersion.MaxPacketType);
            printf("\tMaxStateChange %02x\n", pkt->ManipulateState64.DbgGetVersion.MaxStateChange);
            printf("\tMaxManipulate %02x\n", pkt->ManipulateState64.DbgGetVersion.MaxManipulate);
            printf("\tSimulation %02x\n", pkt->ManipulateState64.DbgGetVersion.Simulation);
            printf("\tUnknown1 %04x\n", pkt->ManipulateState64.DbgGetVersion.Unknown1);
            printf("\tKernelImageBase %llx\n", pkt->ManipulateState64.DbgGetVersion.KernelImageBase);
            printf("\tPsLoadedModuleList %llx\n", pkt->ManipulateState64.DbgGetVersion.PsLoadedModuleList);
            printf("\tDebuggerDataList %llx\n", pkt->ManipulateState64.DbgGetVersion.DebuggerDataList);
            printf("\tUnknown2 %llx\n", pkt->ManipulateState64.DbgGetVersion.Unknown2);
            printf("\tUnknown3 %llx\n", pkt->ManipulateState64.DbgGetVersion.Unknown3);
            break;
        case DbgKdReadVirtualMemoryApi:
            printf("\t[DbgKdReadVirtualMemoryApi]\n");
            printf("\tTargetBaseAddress %llx\n", pkt->ManipulateState64.ReadMemory.TargetBaseAddress);
            printf("\tTransferCount %08x\n", pkt->ManipulateState64.ReadMemory.TransferCount);
            printf("\tActualBytesRead %08x\n", pkt->ManipulateState64.ReadMemory.ActualBytesRead);
            //printf("\tUnknown1 %llx\n", pkt->ManipulateState64.ReadMemory.Unknown1);
            //printf("\tUnknown2 %llx\n", pkt->ManipulateState64.ReadMemory.Unknown2);
            //printf("\tUnknown3 %llx\n", pkt->ManipulateState64.ReadMemory.Unknown3);
            if (pkt->Length > 56){
                printHexData((char*)pkt->ManipulateState64.ReadMemory.Data, pkt->ManipulateState64.ReadMemory.TransferCount);
            }
            break;
        case DbgKdWriteVirtualMemoryApi:
            printf("\t[DbgKdWriteVirtualMemoryApi]\n");
            printf("\tTargetBaseAddress %llx\n", pkt->ManipulateState64.WriteMemory.TargetBaseAddress);
            printf("\tTransferCount %08x\n", pkt->ManipulateState64.WriteMemory.TransferCount);
            printf("\tActualBytesWritten %08x\n", pkt->ManipulateState64.WriteMemory.ActualBytesWritten);
            //printf("\tUnknown1 %llx\n", pkt->ManipulateState64.WriteMemory.Unknown1);
            //printf("\tUnknown2 %llx\n", pkt->ManipulateState64.WriteMemory.Unknown2);
            //printf("\tUnknown3 %llx\n", pkt->ManipulateState64.WriteMemory.Unknown3);

            if (pkt->Length > 56){
                printHexData((char*)pkt->ManipulateState64.WriteMemory.Data, pkt->ManipulateState64.WriteMemory.TransferCount);
            }
            break;
        case DbgKdReadPhysicalMemoryApi:
            printf("\t[DbgKdReadPhysicalMemoryApi]\n");
            printf("\tTargetBaseAddress %llx\n", pkt->ManipulateState64.ReadMemory.TargetBaseAddress);
            printf("\tTransferCount %08x\n", pkt->ManipulateState64.ReadMemory.TransferCount);
            printf("\tActualBytesRead %08x\n", pkt->ManipulateState64.ReadMemory.ActualBytesRead);
            //printf("\tUnknown1 %llx\n", pkt->ManipulateState64.ReadMemory.Unknown1);
            //printf("\tUnknown2 %llx\n", pkt->ManipulateState64.ReadMemory.Unknown2);
            //printf("\tUnknown3 %llx\n", pkt->ManipulateState64.ReadMemory.Unknown3);
            if (pkt->Length > 56){
                printHexData((char*)pkt->ManipulateState64.ReadMemory.Data, pkt->ManipulateState64.ReadMemory.TransferCount);
            }
            break;
        case DbgKdWritePhysicalMemoryApi:
            printf("\t[DbgKdWritePhysicalMemoryApi]\n");
            printf("\tTargetBaseAddress %llx\n", pkt->ManipulateState64.WriteMemory.TargetBaseAddress);
            printf("\tTransferCount %08x\n", pkt->ManipulateState64.WriteMemory.TransferCount);
            printf("\tActualBytesWritten %08x\n", pkt->ManipulateState64.WriteMemory.ActualBytesWritten);
            //printf("\tUnknown1 %llx\n", pkt->ManipulateState64.WriteMemory.Unknown1);
            //printf("\tUnknown2 %llx\n", pkt->ManipulateState64.WriteMemory.Unknown2);
            //printf("\tUnknown3 %llx\n", pkt->ManipulateState64.WriteMemory.Unknown3);
            
            if (pkt->Length > 56){
                printHexData((char*)pkt->ManipulateState64.WriteMemory.Data, pkt->ManipulateState64.WriteMemory.TransferCount);
            }
            break;
        case DbgKdReadControlSpaceApi:
            printf("\t[DbgKdReadControlSpaceApi]\n");
            //TODO: 0 @KPCR, 1 @KPRCB, 2 @SpecialReagister, 3 @KTHREAD
            printf("\tTargetBaseAddress(index) %llx\n", pkt->ManipulateState64.ReadMemory.TargetBaseAddress);
            printf("\tTransferCount %08x\n", pkt->ManipulateState64.ReadMemory.TransferCount);
            printf("\tActualBytesRead %08x\n", pkt->ManipulateState64.ReadMemory.ActualBytesRead);
            //printf("\tUnknown1 %llx\n", pkt->ManipulateState64.ReadMemory.Unknown1);
            //printf("\tUnknown2 %llx\n", pkt->ManipulateState64.ReadMemory.Unknown2);
            //printf("\tUnknown3 %llx\n", pkt->ManipulateState64.ReadMemory.Unknown3);
            if (pkt->Length > 56){
                switch (pkt->ManipulateState64.ReadMemory.TargetBaseAddress){
                case 0: //@v_KPCR
                    break;
                case 1: //@v_KPRCB
                    break;
                case 2:{ //@SpecialRegisters
                    KSPECIAL_REGISTERS64_T *tmpSpecialRegisters = (KSPECIAL_REGISTERS64_T*)pkt->ManipulateState64.WriteMemory.Data;
                    printf("\tKernelDr0 : 0x%llx\n", tmpSpecialRegisters->KernelDr0);
                    printf("\tKernelDr1 : 0x%llx\n", tmpSpecialRegisters->KernelDr1);
                    printf("\tKernelDr2 : 0x%llx\n", tmpSpecialRegisters->KernelDr2);
                    printf("\tKernelDr3 : 0x%llx\n", tmpSpecialRegisters->KernelDr3);
                    printf("\tKernelDr6 : 0x%llx\n", tmpSpecialRegisters->KernelDr6);
                    printf("\tKernelDr7 : 0x%llx\n", tmpSpecialRegisters->KernelDr7);
                    printf("\tGdtr.Limit : 0x%04x\n", tmpSpecialRegisters->Gdtr.Limit);
                    printf("\tGdtr.Base : 0x%llx\n", tmpSpecialRegisters->Gdtr.Base);
                    printf("\tIdtr.Limit : 0x%04x\n", tmpSpecialRegisters->Idtr.Limit);
                    printf("\tIdtr.Base : 0x%llx\n", tmpSpecialRegisters->Idtr.Base);
                    printf("\tTr : 0x%04x\n", tmpSpecialRegisters->Tr);
                    printf("\tLdtr : 0x%04x\n", tmpSpecialRegisters->Ldtr);
                    printf("\tMxCsr : 0x%08x\n", tmpSpecialRegisters->MxCsr);
                    printf("\tDebugControl : 0x%llx\n", tmpSpecialRegisters->DebugControl);
                    printf("\tLastBranchToRip : 0x%llx\n", tmpSpecialRegisters->LastBranchToRip);
                    printf("\tLastBranchFromRip : 0x%llx\n", tmpSpecialRegisters->LastBranchFromRip);
                    printf("\tLastExceptionToRip : 0x%llx\n", tmpSpecialRegisters->LastExceptionToRip);
                    printf("\tLastExceptionFromRip : 0x%llx\n", tmpSpecialRegisters->LastExceptionFromRip);
                    printf("\tCr8 : 0x%llx\n", tmpSpecialRegisters->Cr8);
                    printf("\tMsrGsBase : 0x%llx\n", tmpSpecialRegisters->MsrGsBase);
                    printf("\tMsrGsSwap : 0x%llx\n", tmpSpecialRegisters->MsrGsSwap);
                    printf("\tMsrStar : 0x%llx\n", tmpSpecialRegisters->MsrStar);
                    printf("\tMsrLStar : 0x%llx\n", tmpSpecialRegisters->MsrLStar);
                    printf("\tMsrCStar : 0x%llx\n", tmpSpecialRegisters->MsrCStar);
                    printf("\tMsrSyscallMask : 0x%llx\n", tmpSpecialRegisters->MsrSyscallMask);
                    printf("\tXcr0 : 0x%llx\n", tmpSpecialRegisters->Xcr0);
                    break;
                }
                case 3: //@v_KTHREAD
                    break;
                default:
                    break;
                };
            }
            break;
        case DbgKdWriteControlSpaceApi:
            printf("\t[DbgKdWriteControlSpaceApi]\n");
            printf("\tTargetBaseAddress(index) %llx\n", pkt->ManipulateState64.WriteMemory.TargetBaseAddress);
            printf("\tTransferCount %08x\n", pkt->ManipulateState64.WriteMemory.TransferCount);
            printf("\tActualBytesWritten %08x\n", pkt->ManipulateState64.WriteMemory.ActualBytesWritten);
            switch (pkt->ManipulateState64.ReadMemory.TargetBaseAddress){
            case 0: //@v_KPCR
                break;
            case 1: //@v_KPRCB
                break;
            case 2:{ //@SpecialRegisters
                KSPECIAL_REGISTERS64_T *tmpSpecialRegisters = (KSPECIAL_REGISTERS64_T*)pkt->ManipulateState64.WriteMemory.Data;
                printf("\tKernelDr0 : 0x%llx\n", tmpSpecialRegisters->KernelDr0);
                printf("\tKernelDr1 : 0x%llx\n", tmpSpecialRegisters->KernelDr1);
                printf("\tKernelDr2 : 0x%llx\n", tmpSpecialRegisters->KernelDr2);
                printf("\tKernelDr3 : 0x%llx\n", tmpSpecialRegisters->KernelDr3);
                printf("\tKernelDr6 : 0x%llx\n", tmpSpecialRegisters->KernelDr6);
                printf("\tKernelDr7 : 0x%llx\n", tmpSpecialRegisters->KernelDr7);
                printf("\tGdtr.Limit : 0x%04x\n", tmpSpecialRegisters->Gdtr.Limit);
                printf("\tGdtr.Base : 0x%llx\n", tmpSpecialRegisters->Gdtr.Base);
                printf("\tIdtr.Limit : 0x%04x\n", tmpSpecialRegisters->Idtr.Limit);
                printf("\tIdtr.Base : 0x%llx\n", tmpSpecialRegisters->Idtr.Base);
                printf("\tTr : 0x%04x\n", tmpSpecialRegisters->Tr);
                printf("\tLdtr : 0x%04x\n", tmpSpecialRegisters->Ldtr);
                printf("\tMxCsr : 0x%08x\n", tmpSpecialRegisters->MxCsr);
                printf("\tDebugControl : 0x%llx\n", tmpSpecialRegisters->DebugControl);
                printf("\tLastBranchToRip : 0x%llx\n", tmpSpecialRegisters->LastBranchToRip);
                printf("\tLastBranchFromRip : 0x%llx\n", tmpSpecialRegisters->LastBranchFromRip);
                printf("\tLastExceptionToRip : 0x%llx\n", tmpSpecialRegisters->LastExceptionToRip);
                printf("\tLastExceptionFromRip : 0x%llx\n", tmpSpecialRegisters->LastExceptionFromRip);
                printf("\tCr8 : 0x%llx\n", tmpSpecialRegisters->Cr8);
                printf("\tMsrGsBase : 0x%llx\n", tmpSpecialRegisters->MsrGsBase);
                printf("\tMsrGsSwap : 0x%llx\n", tmpSpecialRegisters->MsrGsSwap);
                printf("\tMsrStar : 0x%llx\n", tmpSpecialRegisters->MsrStar);
                printf("\tMsrLStar : 0x%llx\n", tmpSpecialRegisters->MsrLStar);
                printf("\tMsrCStar : 0x%llx\n", tmpSpecialRegisters->MsrCStar);
                printf("\tMsrSyscallMask : 0x%llx\n", tmpSpecialRegisters->MsrSyscallMask);
                printf("\tXcr0 : 0x%llx\n", tmpSpecialRegisters->Xcr0);
            }
            case 3: //@v_KTHREAD
                break;
            default:
                break;
            };
            break;
        case DbgKdRestoreBreakPointApi:
            printf("\t[DbgKdRestoreBreakPointApi]\n");
            printf("\tBreakPointHandle %08x\n", pkt->ManipulateState64.RestoreBreakPoint.BreakPointHandle);
            break;
        case DbgKdClearAllInternalBreakpointsApi:
            printf("\t[DbgKdClearAllInternalBreakpointsApi]\n");
            break;
        case DbgKdWriteBreakPointApi:
            printf("\t[DbgKdWriteBreakPointApi]\n");
            printf("\tBreakPointAddress %llx\n", pkt->ManipulateState64.WriteBreakPoint.BreakPointAddress);
            printf("\tBreakPointHandle %08x\n", pkt->ManipulateState64.WriteBreakPoint.BreakPointHandle);
            break;
        case DbgKdGetRegisterApi:
            printf("\t[DbgKdGetRegister]\n");

            for (int i = 0; i < 12; i++){
                printf("pkt->ManipulateState64.GetRegisters.u[%d] = 0x%llx;\n", i, pkt->ManipulateState64.GetRegisters.u[i]);
            }
            if (pkt->Length > 56){
                printf("SegCs %04x\n", pkt->ManipulateState64.GetRegisters.SegCs);
                printf("SegDs %04x\n", pkt->ManipulateState64.GetRegisters.SegDs);
                printf("SegEs %04x\n", pkt->ManipulateState64.GetRegisters.SegEs);
                printf("SegFs %04x\n", pkt->ManipulateState64.GetRegisters.SegFs);
                printf("SegGs %04x\n", pkt->ManipulateState64.GetRegisters.SegGs);
                printf("SegSs %04x\n", pkt->ManipulateState64.GetRegisters.SegSs);
                printf("EFlags %08x\n", pkt->ManipulateState64.GetRegisters.EFlags);

                printf("Dr0 %llx\n", pkt->ManipulateState64.GetRegisters.Dr0);
                printf("Dr1 %llx\n", pkt->ManipulateState64.GetRegisters.Dr1);
                printf("Dr2 %llx\n", pkt->ManipulateState64.GetRegisters.Dr2);
                printf("Dr3 %llx\n", pkt->ManipulateState64.GetRegisters.Dr3);
                printf("Dr6 %llx\n", pkt->ManipulateState64.GetRegisters.Dr6);
                printf("Dr7 %llx\n", pkt->ManipulateState64.GetRegisters.Dr7);

                printf("Rax %llx\n", pkt->ManipulateState64.GetRegisters.Rax);
                printf("Rcx %llx\n", pkt->ManipulateState64.GetRegisters.Rcx);
                printf("Rdx %llx\n", pkt->ManipulateState64.GetRegisters.Rdx);
                printf("Rbx %llx\n", pkt->ManipulateState64.GetRegisters.Rbx);
                printf("Rsp %llx\n", pkt->ManipulateState64.GetRegisters.Rsp);
                printf("Rbp %llx\n", pkt->ManipulateState64.GetRegisters.Rbp);
                printf("Rsi %llx\n", pkt->ManipulateState64.GetRegisters.Rsi);
                printf("Rdi %llx\n", pkt->ManipulateState64.GetRegisters.Rdi);
                printf("R8 %llx\n", pkt->ManipulateState64.GetRegisters.R8);
                printf("R9 %llx\n", pkt->ManipulateState64.GetRegisters.R9);
                printf("R10 %llx\n", pkt->ManipulateState64.GetRegisters.R10);
                printf("R11 %llx\n", pkt->ManipulateState64.GetRegisters.R11);
                printf("R12 %llx\n", pkt->ManipulateState64.GetRegisters.R12);
                printf("R13 %llx\n", pkt->ManipulateState64.GetRegisters.R13);
                printf("R14 %llx\n", pkt->ManipulateState64.GetRegisters.R14);
                printf("R15 %llx\n", pkt->ManipulateState64.GetRegisters.R15);

                printf("Rip %llx\n", pkt->ManipulateState64.GetRegisters.Rip);

                for (int i = 0; i < 122; i++){
                    //printf("tmpKDRespPkt->ManipulateState64.GetRegisters.fpu.DATA[%d] = 0x%llx;\n", i, pkt->ManipulateState64.GetRegisters.fpu.DATA[i]);
                }
            }
            break;
        case DbgKdGetContextApi:
            printf("\t[DbgKdGetContextApi]\n");
            printf("CS %04x\n", pkt->ManipulateState64.GetContext.Context.SegCs);
            printf("DS %04x\n", pkt->ManipulateState64.GetContext.Context.SegDs);
            printf("ES %04x\n", pkt->ManipulateState64.GetContext.Context.SegEs);
            printf("FS %04x\n", pkt->ManipulateState64.GetContext.Context.SegFs);
            printf("Gs %04x\n", pkt->ManipulateState64.GetContext.Context.SegGs);
            printf("Ss %04x\n", pkt->ManipulateState64.GetContext.Context.SegSs);
            printf("Rip %llx\n", pkt->ManipulateState64.GetContext.Context.Rip);
            printf("Rbp %llx\n", pkt->ManipulateState64.GetContext.Context.Rbp);
            printf("Rsp %llx\n", pkt->ManipulateState64.GetContext.Context.Rsp);
            printf("Rax %llx\n", pkt->ManipulateState64.GetContext.Context.Rax);
            printf("Rbx %llx\n", pkt->ManipulateState64.GetContext.Context.Rbx);
            printf("Rcx %llx\n", pkt->ManipulateState64.GetContext.Context.Rcx);
            printf("Rdx %llx\n", pkt->ManipulateState64.GetContext.Context.Rdx);
            printf("Rsi %llx\n", pkt->ManipulateState64.GetContext.Context.Rsi);
            printf("Rdi %llx\n", pkt->ManipulateState64.GetContext.Context.Rdi);
            printf("R8 %llx\n", pkt->ManipulateState64.GetContext.Context.R8);
            printf("R9 %llx\n", pkt->ManipulateState64.GetContext.Context.R9);
            printf("R10 %llx\n", pkt->ManipulateState64.GetContext.Context.R10);
            printf("R11 %llx\n", pkt->ManipulateState64.GetContext.Context.R11);
            printf("R12 %llx\n", pkt->ManipulateState64.GetContext.Context.R12);
            printf("R13 %llx\n", pkt->ManipulateState64.GetContext.Context.R13);
            printf("R14 %llx\n", pkt->ManipulateState64.GetContext.Context.R14);
            printf("R15 %llx\n", pkt->ManipulateState64.GetContext.Context.R15);
            printf("EFlags %08x\n", pkt->ManipulateState64.GetContext.Context.EFlags);
            printf("Dr0 %llx\n", pkt->ManipulateState64.GetContext.Context.Dr0);
            printf("Dr1 %llx\n", pkt->ManipulateState64.GetContext.Context.Dr1);
            printf("Dr2 %llx\n", pkt->ManipulateState64.GetContext.Context.Dr2);
            printf("Dr3 %llx\n", pkt->ManipulateState64.GetContext.Context.Dr3);
            printf("Dr6 %llx\n", pkt->ManipulateState64.GetContext.Context.Dr6);
            printf("Dr7 %llx\n", pkt->ManipulateState64.GetContext.Context.Dr7);
            break;
        case DbgKdSetContextApi: 
            printf("\t[DbgKdSetContextApi]\n");
            printf("CS %04x\n", pkt->ManipulateState64.GetContext.Context.SegCs);
            printf("DS %04x\n", pkt->ManipulateState64.GetContext.Context.SegDs);
            printf("ES %04x\n", pkt->ManipulateState64.GetContext.Context.SegEs);
            printf("FS %04x\n", pkt->ManipulateState64.GetContext.Context.SegFs);
            printf("Gs %04x\n", pkt->ManipulateState64.GetContext.Context.SegGs);
            printf("Ss %04x\n", pkt->ManipulateState64.GetContext.Context.SegSs);
            printf("Rip %llx\n", pkt->ManipulateState64.GetContext.Context.Rip);
            printf("Rbp %llx\n", pkt->ManipulateState64.GetContext.Context.Rbp);
            printf("Rsp %llx\n", pkt->ManipulateState64.GetContext.Context.Rsp);
            printf("Rax %llx\n", pkt->ManipulateState64.GetContext.Context.Rax);
            printf("Rbx %llx\n", pkt->ManipulateState64.GetContext.Context.Rbx);
            printf("Rcx %llx\n", pkt->ManipulateState64.GetContext.Context.Rcx);
            printf("Rdx %llx\n", pkt->ManipulateState64.GetContext.Context.Rdx);
            printf("Rsi %llx\n", pkt->ManipulateState64.GetContext.Context.Rsi);
            printf("Rdi %llx\n", pkt->ManipulateState64.GetContext.Context.Rdi);
            printf("R8 %llx\n", pkt->ManipulateState64.GetContext.Context.R8);
            printf("R9 %llx\n", pkt->ManipulateState64.GetContext.Context.R9);
            printf("R10 %llx\n", pkt->ManipulateState64.GetContext.Context.R10);
            printf("R11 %llx\n", pkt->ManipulateState64.GetContext.Context.R11);
            printf("R12 %llx\n", pkt->ManipulateState64.GetContext.Context.R12);
            printf("R13 %llx\n", pkt->ManipulateState64.GetContext.Context.R13);
            printf("R14 %llx\n", pkt->ManipulateState64.GetContext.Context.R14);
            printf("R15 %llx\n", pkt->ManipulateState64.GetContext.Context.R15);
            printf("EFlags %08x\n", pkt->ManipulateState64.GetContext.Context.EFlags);
            printf("Dr0 %llx\n", pkt->ManipulateState64.GetContext.Context.Dr0);
            printf("Dr1 %llx\n", pkt->ManipulateState64.GetContext.Context.Dr1);
            printf("Dr2 %llx\n", pkt->ManipulateState64.GetContext.Context.Dr2);
            printf("Dr3 %llx\n", pkt->ManipulateState64.GetContext.Context.Dr3);
            printf("Dr6 %llx\n", pkt->ManipulateState64.GetContext.Context.Dr6);
            printf("Dr7 %llx\n", pkt->ManipulateState64.GetContext.Context.Dr7);
            break;
        case DbgKdContinueApi:
            printf("\t[DbgKdContinueApi]\n");
            break;
        case DbgKdContinueApi2: //Go !
            printf("\t[DbgKdContinueApi2]\n");
            printf("\tContinueStatus %08x\n", pkt->ManipulateState64.Continue2.ContinueStatus);
            printf("\tTraceFlag %08x\n", pkt->ManipulateState64.Continue2.TraceFlag);
            printf("\tDr7 %llx\n", pkt->ManipulateState64.Continue2.Dr7);
            printf("\tCurrentSymbolStart %llx\n", pkt->ManipulateState64.Continue2.CurrentSymbolStart);
            printf("\tCurrentSymbolEnd %llx\n", pkt->ManipulateState64.Continue2.CurrentSymbolEnd);
            break;
            //VM->Windbg
        case DbgKdExceptionStateChange:
            printf("\t[DbgKdExceptionStateChange]\n");
            printf("\tNewState %08x\n", pkt->StateChange.NewState);
            printf("\tProcessorLevel %04x\n", pkt->StateChange.ProcessorLevel);
            printf("\tProcessor %04x\n", pkt->StateChange.Processor);
            printf("\tNumberProcessors %08x\n", pkt->StateChange.NumberProcessors);
            printf("\tThread %llx\n", pkt->StateChange.Thread);
            printf("\tProgramCounter %llx\n", pkt->StateChange.ProgramCounter);

            //TODO: printExceptionRecord            
            printf("ExceptionCode %08x\n", pkt->StateChange.Exception.ExceptionRecord.ExceptionCode);
            printf("ExceptionFlags %08x\n", pkt->StateChange.Exception.ExceptionRecord.ExceptionFlags);
            printf("ExceptionRecord %llx\n", pkt->StateChange.Exception.ExceptionRecord.ExceptionRecord);
            printf("ExceptionAddress %llx\n", pkt->StateChange.Exception.ExceptionRecord.ExceptionAddress);
            printf("NumberParameters %08x\n", pkt->StateChange.Exception.ExceptionRecord.NumberParameters);
            printf("u1 %08x\n", pkt->StateChange.Exception.ExceptionRecord.u1);
            for (int i = 0; i<EXCEPTION_MAXIMUM_PARAMETERS; i++){
                printf("ExceptionInformation[%d] %llx\n", i, pkt->StateChange.Exception.ExceptionRecord.ExceptionInformation[i]);
            }
            printf("FirstChance %08x\n", pkt->StateChange.Exception.FirstChance);

            printf("\tDR6 %llx\n", pkt->StateChange.ControlReport.Dr6);
            printf("\tDR7 %llx\n", pkt->StateChange.ControlReport.Dr7);
            printf("\tEFlags %08x\n", pkt->StateChange.ControlReport.EFlags);
            printf("\tInstructionCount %04x\n", pkt->StateChange.ControlReport.InstructionCount);
            printf("\tReportFlags %04x\n", pkt->StateChange.ControlReport.ReportFlags);
            for (int i = 0; i<min(DBGKD_MAXSTREAM, pkt->StateChange.ControlReport.InstructionCount); i++){
                printf("\tInstructionStream[%d] %02x\n", i, pkt->StateChange.ControlReport.InstructionStream[i]);
            }
            printf("\tSegCs %04x\n", pkt->StateChange.ControlReport.SegCs);
            printf("\tSegDs %04x\n", pkt->StateChange.ControlReport.SegDs);
            printf("\tSegEs %04x\n", pkt->StateChange.ControlReport.SegEs);
            printf("\tSegFs %04x\n", pkt->StateChange.ControlReport.SegFs);
            printf("\tSegSs %04x\n", pkt->StateChange.ControlReport.SegSs);
            break;
        case DbgKdLoadSymbolsStateChange:
            printf("\t[DbgKdLoadSymbolsStateChange]\n");
            //THE FUCK ?
            break;
        case DbgKdSwitchProcessor:
            printf("\t[DbgKdSwitchProcessor]\n");
            break;
        case DbgKdQueryMemoryApi:
            printf("\t[DbgKdQueryMemoryApi]\n");
            printf("\tAddress 0x%llx\n", pkt->ManipulateState64.QueryMemory.Address);
            printf("\tReserved 0x%llx\n", pkt->ManipulateState64.QueryMemory.Reserved);
            printf("\tAddressSpace 0x%08X\n", pkt->ManipulateState64.QueryMemory.AddressSpace);
            printf("\tFlags 0x%08X\n", pkt->ManipulateState64.QueryMemory.Flags);
            break;
        case DbgKdSearchMemoryApi:
            printf("\t[DbgKdSearchMemoryApi]\n");
            printf("\tSearchAddress 0x%llx\n", pkt->ManipulateState64.SearchMemory.SearchAddress);
            printf("\tSearchLength 0x%llx\n", pkt->ManipulateState64.SearchMemory.SearchLength);
            printf("\tPatternLength 0x%08x\n", pkt->ManipulateState64.SearchMemory.PatternLength);
            if (pkt->Length > 56){
                printf("\tData :\n");
                printHexData((char*)pkt->ManipulateState64.SearchMemory.Data, pkt->ManipulateState64.SearchMemory.PatternLength);
            }
            break;
        case DbgKdReadMachineSpecificRegister:
            printf("\t[DbgKdReadMachineSpecificRegister]\n");
            printf("\tMsr %08x\n", pkt->ManipulateState64.ReadWriteMsr.Msr);
            printf("\tDataValueHigh %08x\n", pkt->ManipulateState64.ReadWriteMsr.DataValueHigh);
            printf("\tDataValueLow %08x\n", pkt->ManipulateState64.ReadWriteMsr.DataValueLow);
            break;
        default: //Stop ALL !
            printf("\t[UNKNOWN]\n");
            //stopKDServer();
            //printHexData((char*)pkt->data, pkt->Length);
            //system("pause");
        }
        printf("\t---------KD_CONTENT-----------\n");
    }
    printf("---------KD_HEADER-----------\n");
    printf("\n\n");
    return true;
}