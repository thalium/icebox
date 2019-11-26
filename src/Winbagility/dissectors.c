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
#include "winbagility.h"
#include "kdserver.h"
#include "dissectors.h"
#include "mmu.h"
#include "utils.h"
#include "FDP.h"
#include "symbols.h"

#define KDBG_HEADER_OWNER_TAG 0x4742444b

WINDOWS_PROFIL_T WindowsProfils[] = {
    {
        "3844DBB920174967BE7AA4A2C20430FA2",
        "7601.17514.amd64fre.win7sp1_rtm.101119-1850",
        0x00000000002B11E0,
        0x00000000002B1108,
        0x00000000002194B2,
        0x00000000001F10A0,
        0x00000000001F1068,
        0x000000000007D3C0,
        0x0000000000000000,
        0x0000000000000000,
        true,
        false
    },
    {
        "C68EE22FDCF6477895C54A862BE165671",
        "10240.16384.amd64fre.th1.150709-1700",
        0x00000000003c5630,
        0x00000000003c54a8,
        0x0000000000340f68,
        0x0000000000309b20,
        0x0000000000309e80,
        0x0000000000154800,
        0x0000000000000000,
        0x0000000000000000,
        false,
        false
    },
    {
        "3BAEE2762F6442089EF8B926DDC8DBA61",
        "9600.17736.amd64fre.winblue_r9.150322-1500",
        0x0000000000363A30,
        0x0000000000363538,
        0x00000000002B2211,
        0x00000000002A6530,
        0x00000000002A6E60,
        0x0000000000159C00,
        0x0000000000000000,
        0x0000000000000000,
        false,
        false
    },
    {
        "DD08DD42692B43F199A079D60E79D2171",
        "14393.0.amd64fre.rs1_release.160715-1616",
        0x3a8648,
        0x3a84c0,
        0x340110,
        0x2f0500,
        0x2f2cf8,
        0x152100,
        0x319aa0,
        false,
        false
    },
    {
        "D788F72ABE964EFCACAAD0276DAAE6CB1",
        "15063.0.amd64fre.rs2_release.170317-1834",
        0x3e23f0,
        0x3e2260,
        0x36b470,
        0x3384f0,
        0x33aff0,
        0x174300,
        0x362140,
        false,
        false
    }
    
};


/*
* Print a KDBG
*/
void printKDBG(KDDEBUGGER_DATA64 *tmpKDBG)
{
    printf("-------------KDBG-----------------------\n");
    printf("OwnerTag : %04x\n", tmpKDBG->Header.OwnerTag);
    printf("Size : %04x\n", tmpKDBG->Header.Size);
    printf("KernBase : %llx\n", tmpKDBG->KernBase);
    printf("BreakpointWithStatus : %llx\n", tmpKDBG->BreakpointWithStatus);
    printf("SavedContext : %llx\n", tmpKDBG->SavedContext);
    printf("ThCallbackStack : %04x\n", tmpKDBG->ThCallbackStack);
    printf("NextCallback : %04x\n", tmpKDBG->NextCallback);
    printf("FramePointer : %04x\n", tmpKDBG->FramePointer);
    printf("PaeEnabled : %04x\n", tmpKDBG->PaeEnabled);
    printf("KiCallUserMode : %llx\n", tmpKDBG->KiCallUserMode);
    printf("KeUserCallbackDispatcher : %llx\n", tmpKDBG->KeUserCallbackDispatcher);
    printf("PsLoadedModuleList : %llx\n", tmpKDBG->PsLoadedModuleList);
    printf("PsActiveProcessHead : %llx\n", tmpKDBG->PsActiveProcessHead);
    printf("PspCidTable : %llx\n", tmpKDBG->PspCidTable);
    printf("ExpSystemResourcesList : %llx\n", tmpKDBG->ExpSystemResourcesList);
    printf("ExpPagedPoolDescriptor : %llx\n", tmpKDBG->ExpPagedPoolDescriptor);
    printf("ExpNumberOfPagedPools : %llx\n", tmpKDBG->ExpNumberOfPagedPools);
    printf("KeTimeIncrement : %llx\n", tmpKDBG->KeTimeIncrement);
    printf("KeBugCheckCallbackListHead : %llx\n", tmpKDBG->KeBugCheckCallbackListHead);
    printf("KiBugcheckData : %llx\n", tmpKDBG->KiBugcheckData);
    printf("IopErrorLogListHead : %llx\n", tmpKDBG->IopErrorLogListHead);
    printf("ObpRootDirectoryObject : %llx\n", tmpKDBG->ObpRootDirectoryObject);
    printf("ObpTypeObjectType : %llx\n", tmpKDBG->ObpTypeObjectType);
    printf("MmSystemCacheStart : %llx\n", tmpKDBG->MmSystemCacheStart);
    printf("MmSystemCacheEnd : %llx\n", tmpKDBG->MmSystemCacheEnd);
    printf("MmSystemCacheWs : %llx\n", tmpKDBG->MmSystemCacheWs);
    printf("MmPfnDatabase : %llx\n", tmpKDBG->MmPfnDatabase);
    printf("MmSystemPtesStart : %llx\n", tmpKDBG->MmSystemPtesStart);
    printf("MmSystemPtesEnd : %llx\n", tmpKDBG->MmSystemPtesEnd);
    printf("MmSubsectionBase : %llx\n", tmpKDBG->MmSubsectionBase);
    printf("MmNumberOfPagingFiles : %llx\n", tmpKDBG->MmNumberOfPagingFiles);
    printf("MmLowestPhysicalPage : %llx\n", tmpKDBG->MmLowestPhysicalPage);
    printf("MmHighestPhysicalPage : %llx\n", tmpKDBG->MmHighestPhysicalPage);
    printf("MmNumberOfPhysicalPages : %llx\n", tmpKDBG->MmNumberOfPhysicalPages);
    printf("MmMaximumNonPagedPoolInBytes : %llx\n", tmpKDBG->MmMaximumNonPagedPoolInBytes);
    printf("MmNonPagedSystemStart : %llx\n", tmpKDBG->MmNonPagedSystemStart);
    printf("MmNonPagedPoolStart : %llx\n", tmpKDBG->MmNonPagedPoolStart);
    printf("MmNonPagedPoolEnd : %llx\n", tmpKDBG->MmNonPagedPoolEnd);
    printf("MmPagedPoolStart : %llx\n", tmpKDBG->MmPagedPoolStart);
    printf("MmPagedPoolEnd : %llx\n", tmpKDBG->MmPagedPoolEnd);
    printf("MmPagedPoolInformation : %llx\n", tmpKDBG->MmPagedPoolInformation);
    printf("MmPageSize : %llx\n", tmpKDBG->MmPageSize);
    printf("MmSizeOfPagedPoolInBytes : %llx\n", tmpKDBG->MmSizeOfPagedPoolInBytes);
    printf("MmTotalCommitLimit : %llx\n", tmpKDBG->MmTotalCommitLimit);
    printf("MmTotalCommittedPages : %llx\n", tmpKDBG->MmTotalCommittedPages);
    printf("MmSharedCommit : %llx\n", tmpKDBG->MmSharedCommit);
    printf("MmDriverCommit : %llx\n", tmpKDBG->MmDriverCommit);
    printf("MmProcessCommit : %llx\n", tmpKDBG->MmProcessCommit);
    printf("MmPagedPoolCommit : %llx\n", tmpKDBG->MmPagedPoolCommit);
    printf("MmExtendedCommit : %llx\n", tmpKDBG->MmExtendedCommit);
    printf("MmZeroedPageListHead : %llx\n", tmpKDBG->MmZeroedPageListHead);
    printf("MmFreePageListHead : %llx\n", tmpKDBG->MmFreePageListHead);
    printf("MmStandbyPageListHead : %llx\n", tmpKDBG->MmStandbyPageListHead);
    printf("MmModifiedPageListHead : %llx\n", tmpKDBG->MmModifiedPageListHead);
    printf("MmModifiedNoWritePageListHead : %llx\n", tmpKDBG->MmModifiedNoWritePageListHead);
    printf("MmAvailablePages : %llx\n", tmpKDBG->MmAvailablePages);
    printf("MmResidentAvailablePages : %llx\n", tmpKDBG->MmResidentAvailablePages);
    printf("PoolTrackTable : %llx\n", tmpKDBG->PoolTrackTable);
    printf("NonPagedPoolDescriptor : %llx\n", tmpKDBG->NonPagedPoolDescriptor);
    printf("MmHighestUserAddress : %llx\n", tmpKDBG->MmHighestUserAddress);
    printf("MmSystemRangeStart : %llx\n", tmpKDBG->MmSystemRangeStart);
    printf("MmUserProbeAddress : %llx\n", tmpKDBG->MmUserProbeAddress);
    printf("KdPrintCircularBuffer : %llx\n", tmpKDBG->KdPrintCircularBuffer);
    printf("KdPrintCircularBufferEnd : %llx\n", tmpKDBG->KdPrintCircularBufferEnd);
    printf("KdPrintWritePointer : %llx\n", tmpKDBG->KdPrintWritePointer);
    printf("KdPrintRolloverCount : %llx\n", tmpKDBG->KdPrintRolloverCount);
    printf("MmLoadedUserImageList : %llx\n", tmpKDBG->MmLoadedUserImageList);
    printf("NtBuildLab : %llx\n", tmpKDBG->NtBuildLab);
    printf("KiNormalSystemCall : %llx\n", tmpKDBG->KiNormalSystemCall);
    printf("KiProcessorBlock : %llx\n", tmpKDBG->KiProcessorBlock);
    printf("MmUnloadedDrivers : %llx\n", tmpKDBG->MmUnloadedDrivers);
    printf("MmLastUnloadedDriver : %llx\n", tmpKDBG->MmLastUnloadedDriver);
    printf("MmTriageActionTaken : %llx\n", tmpKDBG->MmTriageActionTaken);
    printf("MmSpecialPoolTag : %llx\n", tmpKDBG->MmSpecialPoolTag);
    printf("KernelVerifier : %llx\n", tmpKDBG->KernelVerifier);
    printf("MmVerifierData : %llx\n", tmpKDBG->MmVerifierData);
    printf("MmAllocatedNonPagedPool : %llx\n", tmpKDBG->MmAllocatedNonPagedPool);
    printf("MmPeakCommitment : %llx\n", tmpKDBG->MmPeakCommitment);
    printf("MmTotalCommitLimitMaximum : %llx\n", tmpKDBG->MmTotalCommitLimitMaximum);
    printf("CmNtCSDVersion : %llx\n", tmpKDBG->CmNtCSDVersion);
    printf("MmPhysicalMemoryBlock : %llx\n", tmpKDBG->MmPhysicalMemoryBlock);
    printf("MmSessionBase : %llx\n", tmpKDBG->MmSessionBase);
    printf("MmSessionSize : %llx\n", tmpKDBG->MmSessionSize);
    printf("MmSystemParentTablePage : %llx\n", tmpKDBG->MmSystemParentTablePage);
    printf("MmVirtualTranslationBase : %llx\n", tmpKDBG->MmVirtualTranslationBase);
    printf("OffsetKThreadNextProcessor : %04x\n", tmpKDBG->OffsetKThreadNextProcessor);
    printf("OffsetKThreadTeb : %04x\n", tmpKDBG->OffsetKThreadTeb);
    printf("OffsetKThreadKernelStack : %04x\n", tmpKDBG->OffsetKThreadKernelStack);
    printf("OffsetKThreadInitialStack : %04x\n", tmpKDBG->OffsetKThreadInitialStack);
    printf("OffsetKThreadApcProcess : %04x\n", tmpKDBG->OffsetKThreadApcProcess);
    printf("OffsetKThreadState : %04x\n", tmpKDBG->OffsetKThreadState);
    printf("OffsetKThreadBStore : %04x\n", tmpKDBG->OffsetKThreadBStore);
    printf("OffsetKThreadBStoreLimit : %04x\n", tmpKDBG->OffsetKThreadBStoreLimit);
    printf("SizeEProcess : %04x\n", tmpKDBG->SizeEProcess);
    printf("OffsetEprocessPeb : %04x\n", tmpKDBG->OffsetEprocessPeb);
    printf("OffsetEprocessParentCID : %04x\n", tmpKDBG->OffsetEprocessParentCID);
    printf("OffsetEprocessDirectoryTableBase : %04x\n", tmpKDBG->OffsetEprocessDirectoryTableBase);
    printf("SizePrcb : %04x\n", tmpKDBG->SizePrcb);
    printf("OffsetPrcbDpcRoutine : %04x\n", tmpKDBG->OffsetPrcbDpcRoutine);
    printf("OffsetPrcbCurrentThread : %04x\n", tmpKDBG->OffsetPrcbCurrentThread);
    printf("OffsetPrcbMhz : %04x\n", tmpKDBG->OffsetPrcbMhz);
    printf("OffsetPrcbCpuType : %04x\n", tmpKDBG->OffsetPrcbCpuType);
    printf("OffsetPrcbVendorString : %04x\n", tmpKDBG->OffsetPrcbVendorString);
    printf("OffsetPrcbProcStateContext : %04x\n", tmpKDBG->OffsetPrcbProcStateContext);
    printf("OffsetPrcbNumber : %04x\n", tmpKDBG->OffsetPrcbNumber);
    printf("SizeEThread : %04x\n", tmpKDBG->SizeEThread);
    printf("KdPrintCircularBufferPtr : %llx\n", tmpKDBG->KdPrintCircularBufferPtr);
    printf("KdPrintBufferSize : %llx\n", tmpKDBG->KdPrintBufferSize);
    printf("KeLoaderBlock : %llx\n", tmpKDBG->KeLoaderBlock);
    printf("SizePcr : %04x\n", tmpKDBG->SizePcr);
    printf("OffsetPcrSelfPcr : %04x\n", tmpKDBG->OffsetPcrSelfPcr);
    printf("OffsetPcrCurrentPrcb : %04x\n", tmpKDBG->OffsetPcrCurrentPrcb);
    printf("OffsetPcrContainedPrcb : %04x\n", tmpKDBG->OffsetPcrContainedPrcb);
    printf("OffsetPcrInitialBStore : %04x\n", tmpKDBG->OffsetPcrInitialBStore);
    printf("OffsetPcrBStoreLimit : %04x\n", tmpKDBG->OffsetPcrBStoreLimit);
    printf("OffsetPcrInitialStack : %04x\n", tmpKDBG->OffsetPcrInitialStack);
    printf("OffsetPcrStackLimit : %04x\n", tmpKDBG->OffsetPcrStackLimit);
    printf("OffsetPrcbPcrPage : %04x\n", tmpKDBG->OffsetPrcbPcrPage);
    printf("OffsetPrcbProcStateSpecialReg : %04x\n", tmpKDBG->OffsetPrcbProcStateSpecialReg);
    printf("GdtR0Code : %04x\n", tmpKDBG->GdtR0Code);
    printf("GdtR0Data : %04x\n", tmpKDBG->GdtR0Data);
    printf("GdtR0Pcr : %04x\n", tmpKDBG->GdtR0Pcr);
    printf("GdtR3Code : %04x\n", tmpKDBG->GdtR3Code);
    printf("GdtR3Data : %04x\n", tmpKDBG->GdtR3Data);
    printf("GdtR3Teb : %04x\n", tmpKDBG->GdtR3Teb);
    printf("GdtLdt : %04x\n", tmpKDBG->GdtLdt);
    printf("GdtTss : %04x\n", tmpKDBG->GdtTss);
    printf("Gdt64R3CmCode : %04x\n", tmpKDBG->Gdt64R3CmCode);
    printf("Gdt64R3CmTeb : %04x\n", tmpKDBG->Gdt64R3CmTeb);
    printf("IopNumTriageDumpDataBlocks : %llx\n", tmpKDBG->IopNumTriageDumpDataBlocks);
    printf("IopTriageDumpDataBlocks : %llx\n", tmpKDBG->IopTriageDumpDataBlocks);
    printf("VfCrashDataBlock : %llx\n", tmpKDBG->VfCrashDataBlock);
    printf("----------------------------------------\n");
}

/*
*
* Uncipher 64 bits of the KDBG with 3 keys
*
* @param data data to uncipher
* @param KiWaitNever one key
* @param KiWaitAlways one key
* @param KdpDataBlockEncoded one key
* @return data data unciphered
*/
__inline uint64_t uncipherData(uint64_t Data, uint64_t KiWaitNever, uint64_t KiWaitAlways, uint64_t KdpDataBlockEncoded)
{
    Data = Data^KiWaitNever;
    Data = _rotl64(Data, KiWaitNever & 0xFF);
    Data = Data^KdpDataBlockEncoded;
    Data = _byteswap_uint64(Data);
    Data = Data^KiWaitAlways;
    return Data;
}


bool FindKDBG(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, WINDOWS_PROFIL_T *pWindowsProfil)
{
    //Compute KiDivideErrorFault address
    uint64_t v_Idtrb;
    uint16_t Offset;
    uint64_t Base;
    uint64_t v_KdDebuggerDataBlock;
    pWinbagilityCtx->pfnReadRegister(pWinbagilityCtx->pUserHandle, 0, FDP_IDTRB_REGISTER, &v_Idtrb);
    printf("Idtrb %llx\n", v_Idtrb);
    if (v_Idtrb & 0xFFFFFFFF00000000) {
        //x64
        pWinbagilityCtx->pfnReadVirtualMemory(pWinbagilityCtx->pUserHandle, 0, (uint8_t*)&Offset, sizeof(Offset), v_Idtrb);
        pWinbagilityCtx->pfnReadVirtualMemory(pWinbagilityCtx->pUserHandle, 0, (uint8_t*)&Base, sizeof(Base), v_Idtrb + 4);
        uint64_t v_KiDivideErrorFault = (Base & 0xFFFFFFFFFFFF0000) | Offset;
        printf("v_KiDivideErrorFault : %llx\n", v_KiDivideErrorFault);

        uint64_t v_KernBase = v_KiDivideErrorFault - pWindowsProfil->KiDivideErrorFaultOffset;
        printf("v_KernBase : %llx\n", v_KernBase);
        uint64_t v_KiWaitAlways = v_KernBase + pWindowsProfil->KiWaitAlwaysOffset;
        printf("v_KiWaitAlways : %llx\n", v_KiWaitAlways);
        uint64_t v_KiWaitNever = v_KernBase + pWindowsProfil->KiWaitNeverOffset;
        printf("v_KiWaitNever : %llx\n", v_KiWaitNever);
        uint64_t v_KdpDataBlockEncoded = v_KernBase + pWindowsProfil->KdpDataBlockEncodedOffset;
        printf("v_KdpDataBlockEncoded : %llx\n", v_KdpDataBlockEncoded);
        v_KdDebuggerDataBlock = v_KernBase + pWindowsProfil->KdDebuggerDataBlockOffset;
        printf("v_KdDebuggerDataBlock : %llx\n", v_KdDebuggerDataBlock);

        //Retrieve keys value
        if (pWinbagilityCtx->pfnReadVirtualMemory(pWinbagilityCtx->pUserHandle,
            0,
            (uint8_t*)&pWinbagilityCtx->KiWaitNever,
            sizeof(pWinbagilityCtx->KiWaitNever),
            v_KiWaitNever) == false) {
            return false;
        }
        printf("keys->KiWaitNever : %llx\n", pWinbagilityCtx->KiWaitNever);

        if (pWinbagilityCtx->pfnReadVirtualMemory(pWinbagilityCtx->pUserHandle,
            0,
            (uint8_t*)&pWinbagilityCtx->KiWaitAlways,
            sizeof(pWinbagilityCtx->KiWaitAlways),
            v_KiWaitAlways) == false) {
            return false;
        }
        printf("keys->KiWaitAlways : %llx\n", pWinbagilityCtx->KiWaitAlways);
        pWinbagilityCtx->KdpDataBlockEncoded = v_KdpDataBlockEncoded;
        printf("keys->KdpDataBlockEncoded : %llx\n", pWinbagilityCtx->KdpDataBlockEncoded);
        pWinbagilityCtx->v_KDBG = v_KdDebuggerDataBlock;
    }
    else {
        //x86
        pWinbagilityCtx->pfnReadVirtualMemory(pWinbagilityCtx->pUserHandle, 0, (uint8_t*)&Offset, sizeof(Offset), v_Idtrb);
        pWinbagilityCtx->pfnReadVirtualMemory(pWinbagilityCtx->pUserHandle, 0, (uint8_t*)&Base, sizeof(Base), v_Idtrb + 4);
        uint64_t v_KiTrap00 = (Base & 0xFFFF0000) | Offset;
        printf("v_KiTrap00 : %llx\n", v_KiTrap00);

        printf("%llx\n", pWindowsProfil->KiTrap00Offset);
        uint64_t v_KernBase = v_KiTrap00 - pWindowsProfil->KiTrap00Offset;
        printf("v_KernBase : %llx\n", v_KernBase);
        pWinbagilityCtx->KdpDebuggerDataListHead = v_KernBase + pWindowsProfil->KdpDebuggerDataListHeadOffset;
        v_KdDebuggerDataBlock = 
        pWinbagilityCtx->v_KDBG = v_KernBase + pWindowsProfil->KdDebuggerDataBlockOffset;
        printf("v_KdDebuggerDataBlock : %llx\n", pWinbagilityCtx->v_KDBG);

        pWindowsProfil->bClearKdDebuggerDataBlock = true;
        pWindowsProfil->bIsX86 = true;
    }

    
    //Retrieve KDBG
    if (pWinbagilityCtx->pfnReadVirtualMemory(pWinbagilityCtx->pUserHandle,
        0,
        (uint8_t*)&pWinbagilityCtx->EncodedKDBG,
        sizeof(KDDEBUGGER_DATA64),
        v_KdDebuggerDataBlock) == false){
        return false;
    }

    if (pWindowsProfil->bClearKdDebuggerDataBlock == true){ //Windows < 8 doesn't encode KDBG
        //Nothing to do, just copy...
        memcpy(&pWinbagilityCtx->KDBG, &pWinbagilityCtx->EncodedKDBG, sizeof(KDDEBUGGER_DATA64));
    }
    else{
        //Uncipher KDBG
        for (int i = 0; i < sizeof(KDDEBUGGER_DATA64) / sizeof(uint64_t); i++) {
            uint64_t tmpEncodedData = ((uint64_t*)&pWinbagilityCtx->EncodedKDBG)[i];
            ((uint64_t*)&pWinbagilityCtx->KDBG)[i] = uncipherData(tmpEncodedData, pWinbagilityCtx->KiWaitNever, pWinbagilityCtx->KiWaitAlways, pWinbagilityCtx->KdpDataBlockEncoded);
        }
    }

    //Check if KDBG is correct
    if (pWinbagilityCtx->KDBG.Header.OwnerTag == KDBG_HEADER_OWNER_TAG){
        printKDBG((KDDEBUGGER_DATA64*)&pWinbagilityCtx->KDBG);
        return true;
    }

    return false;
}

WINDOWS_PROFIL_T* CreateWindowsProfileFromPdbFile(char *pPdbFilePath)
{
    PDB_PARSER_T PdbParserHandle;

    if (PdbOpenPdb(&PdbParserHandle, pPdbFilePath)){
        uint64_t off_KiWaitAlways = 0;
        uint64_t off_KiWaitNever = 0;
        uint64_t off_KdpDataBlockEncoded = 0;
        uint64_t off_KdDebuggerDataBlock = 0;
        uint64_t off_KdVersionBlock = 0;
        uint64_t off_KiDivideErrorFault = 0;
        uint64_t off_KiTrap00 = 0;
        uint64_t off_KdpDebuggerDataListHead = 0;

        PdbGetSymbolsRVA(&PdbParserHandle, "KiWaitAlways", &off_KiWaitAlways);
        PdbGetSymbolsRVA(&PdbParserHandle, "KiWaitNever", &off_KiWaitNever);
        PdbGetSymbolsRVA(&PdbParserHandle, "KdpDataBlockEncoded", &off_KdpDataBlockEncoded);
        PdbGetSymbolsRVA(&PdbParserHandle, "KdDebuggerDataBlock", &off_KdDebuggerDataBlock);
        PdbGetSymbolsRVA(&PdbParserHandle, "KdVersionBlock", &off_KdVersionBlock);
        if(!PdbGetSymbolsRVA(&PdbParserHandle, "KiDivideErrorFaultShadow", &off_KiDivideErrorFault))
        {
            PdbGetSymbolsRVA(&PdbParserHandle, "KiDivideErrorFault", &off_KiDivideErrorFault);
        }
        PdbGetSymbolsRVA(&PdbParserHandle, "KiTrap00", &off_KiTrap00);
        PdbGetSymbolsRVA(&PdbParserHandle, "KdpDebuggerDataListHead", &off_KdpDebuggerDataListHead);

        //todo remove malloc !
        WINDOWS_PROFIL_T *pCurrentWindowsProfil = (WINDOWS_PROFIL_T*)malloc(sizeof(WINDOWS_PROFIL_T));

        pCurrentWindowsProfil->pGUID = "{TODO}";
        pCurrentWindowsProfil->pVersionName = "TODO"; //nt!NtBuildLabEx
        pCurrentWindowsProfil->KiWaitAlwaysOffset = off_KiWaitAlways;
        pCurrentWindowsProfil->KiWaitNeverOffset = off_KiWaitNever;
        pCurrentWindowsProfil->KdpDataBlockEncodedOffset = off_KdpDataBlockEncoded;
        pCurrentWindowsProfil->KdDebuggerDataBlockOffset = off_KdDebuggerDataBlock;
        pCurrentWindowsProfil->KdVersionBlockOffset = off_KdVersionBlock;
        pCurrentWindowsProfil->KiDivideErrorFaultOffset = off_KiDivideErrorFault;
        pCurrentWindowsProfil->KiTrap00Offset = off_KiTrap00;
        pCurrentWindowsProfil->KdpDebuggerDataListHeadOffset = off_KdpDebuggerDataListHead;
        pCurrentWindowsProfil->bClearKdDebuggerDataBlock = true;

        PdbClose(&PdbParserHandle);

        return pCurrentWindowsProfil;
    }
    else{
        printf("Failed to open ntkrnlmp.pdb\n");
    }

    return NULL;
}

typedef struct SEARCH_WINDOWS_PROFIL_T_{
    WINBAGILITY_CONTEXT_T *pWinbagilityCtx;
    WINDOWS_PROFIL_T *pCurrentWindowsProfil;
}SEARCH_WINDOWS_PROFIL_T;

bool TestPdbFile(void *pUserHandle, char *pFilePath)
{
    SEARCH_WINDOWS_PROFIL_T* pSearchWindowsProfil = (SEARCH_WINDOWS_PROFIL_T*)pUserHandle;

    if (strlen(pFilePath) < 3) {
        return false;
    }

    //Check file extension
    char *pExtension = pFilePath + strlen(pFilePath) - 3;
    if (strcmp(pExtension, "pdb") != 0) {
        return false;
    }

    printf("######################################\n");
    printf("Trying Windows profil from : %s\n", pFilePath);

    pSearchWindowsProfil->pCurrentWindowsProfil = CreateWindowsProfileFromPdbFile(pFilePath);
    if (pSearchWindowsProfil->pCurrentWindowsProfil != NULL){
        //Trying with encodeded KDBG...
        pSearchWindowsProfil->pCurrentWindowsProfil->bClearKdDebuggerDataBlock = false;
        if (FindKDBG(pSearchWindowsProfil->pWinbagilityCtx, pSearchWindowsProfil->pCurrentWindowsProfil) == true){
            return true;
        }
        
        //Trying with clear KDBG...
        pSearchWindowsProfil->pCurrentWindowsProfil->bClearKdDebuggerDataBlock = true;
        if (FindKDBG(pSearchWindowsProfil->pWinbagilityCtx, pSearchWindowsProfil->pCurrentWindowsProfil) == true){
            return true;
        }
        free(pSearchWindowsProfil->pCurrentWindowsProfil);
        pSearchWindowsProfil->pCurrentWindowsProfil = NULL;
    }

    return false;
}

/*
* This function looks for some "important" struct in the memory of the debuggee
* DTB, KPCR...
*
* @param context of analysis
* @return True if the analysis is a success
*/
bool InitialAnalysis(WINBAGILITY_CONTEXT_T *pWinbagilityCtx)
{
    //Use to detect reboot...
    pWinbagilityCtx->pfnReadRegister(pWinbagilityCtx->pUserHandle, 0, FDP_IDTRB_REGISTER, &pWinbagilityCtx->StartIdtrb);

    uint8_t WindowsProfilCount = sizeof(WindowsProfils) / sizeof(WINDOWS_PROFIL_T);
    WINDOWS_PROFIL_T *pCurrentWindowsProfil = NULL;
    for (int p = 0; p < WindowsProfilCount; p++){
        pCurrentWindowsProfil = &WindowsProfils[p];
        printf("######################################\n");
        printf("Trying Windows profil : \"%s\"\n", pCurrentWindowsProfil->pVersionName);
        if (FindKDBG(pWinbagilityCtx, pCurrentWindowsProfil) == true){
            break;
        }
        pCurrentWindowsProfil = NULL;
    }

    //Try with PDB from _NT_SYMBOLS_PATH
    if (pCurrentWindowsProfil == NULL){
        char *pSymbolsDirectory = NULL;
        //Get Symbol Directory... x64
        pSymbolsDirectory = GetSymbolsDirectory();
        if (pSymbolsDirectory != NULL){
            //Concat 
            char NtkrnlmpSymbolsDirectory[MAX_PATH];
            sprintf_s(NtkrnlmpSymbolsDirectory, sizeof(NtkrnlmpSymbolsDirectory), "%s\\ntkrnlmp.pdb\\", pSymbolsDirectory);

            SEARCH_WINDOWS_PROFIL_T SearchWindowsProfil;
            SearchWindowsProfil.pWinbagilityCtx = pWinbagilityCtx;
            SearchWindowsProfil.pCurrentWindowsProfil = NULL;

            if (ReccurseFile(NtkrnlmpSymbolsDirectory, TestPdbFile, &SearchWindowsProfil, true) == true){
                pCurrentWindowsProfil = SearchWindowsProfil.pCurrentWindowsProfil;
            }
        }

        //Get Symbol Directory... x86
        pSymbolsDirectory = GetSymbolsDirectory();
        if (pSymbolsDirectory != NULL) {
            //Concat 
            char NtkrnlmpSymbolsDirectory[MAX_PATH];
            sprintf_s(NtkrnlmpSymbolsDirectory, sizeof(NtkrnlmpSymbolsDirectory), "%s\\ntkrpamp.pdb\\", pSymbolsDirectory);

            SEARCH_WINDOWS_PROFIL_T SearchWindowsProfil;
            SearchWindowsProfil.pWinbagilityCtx = pWinbagilityCtx;
            SearchWindowsProfil.pCurrentWindowsProfil = NULL;

            if (ReccurseFile(NtkrnlmpSymbolsDirectory, TestPdbFile, &SearchWindowsProfil, true) == true) {
                pCurrentWindowsProfil = SearchWindowsProfil.pCurrentWindowsProfil;
            }
        }
    }

    if (pCurrentWindowsProfil == NULL){
        printf("Unable to find a good windows profil !\n");
        return false;
    }

    pWinbagilityCtx->pCurrentWindowsProfil = pCurrentWindowsProfil;

    //Print the Windows Profil
    printf("{\n");
    printf("\t\"%s\",\n", pCurrentWindowsProfil->pGUID);
    printf("\t\"%s\",\n", pCurrentWindowsProfil->pVersionName);
    printf("\t0x%llx,\n", pCurrentWindowsProfil->KiWaitAlwaysOffset);
    printf("\t0x%llx,\n", pCurrentWindowsProfil->KiWaitNeverOffset);
    printf("\t0x%llx,\n", pCurrentWindowsProfil->KdpDataBlockEncodedOffset);
    printf("\t0x%llx,\n", pCurrentWindowsProfil->KdDebuggerDataBlockOffset);
    printf("\t0x%llx,\n", pCurrentWindowsProfil->KdVersionBlockOffset);
    printf("\t0x%llx,\n", pCurrentWindowsProfil->KiDivideErrorFaultOffset);
    printf("\t0x%llx,\n", pCurrentWindowsProfil->KdpDebuggerDataListHeadOffset);
    printf("\t%s,\n", pCurrentWindowsProfil->bClearKdDebuggerDataBlock ? "true" : "false");
    printf("\t%s\n", pCurrentWindowsProfil->bIsX86 ? "true" : "false");
    printf("}\n");

    printf("v_KDBG : %llx\n", pWinbagilityCtx->v_KDBG);
    pWinbagilityCtx->v_KernBase = pWinbagilityCtx->KDBG.KernBase;
    printf("v_KernBase : %llx\n", pWinbagilityCtx->v_KernBase);

    pWinbagilityCtx->v_KiProcessBlock = pWinbagilityCtx->KDBG.KiProcessorBlock;

    //Retreive KPCR for all CPUs
    uint32_t i = 0;
    do{
        pWinbagilityCtx->pfnReadVirtualMemory(pWinbagilityCtx->pUserHandle,
            0,
            (uint8_t*)&pWinbagilityCtx->v_KPRCB[i],
            sizeof(pWinbagilityCtx->v_KPRCB[i]),
            pWinbagilityCtx->v_KiProcessBlock + (i * 8));
        if (pWinbagilityCtx->v_KPRCB[i] == 0){
             //No more CPU !!
            break;
        }
        printf("v_KPRCB[%d] : %llx\n", i, pWinbagilityCtx->v_KPRCB[i]);
        pWinbagilityCtx->v_KPCR[i] = pWinbagilityCtx->v_KPRCB[i] - pWinbagilityCtx->KDBG.OffsetPcrContainedPrcb;
        printf("v_KPCR[%d] : %llx\n", i, pWinbagilityCtx->v_KPCR[i]);
        i++;
    } while (pWinbagilityCtx->v_KPRCB[i - 1] != 0);
    pWinbagilityCtx->CpuCount = i;

    return true;
}
