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
#ifndef __KDSERVER_H__
#define __KDSERVER_H__


//Move to FDP.cpp ???
#define FDP2KD_GET_VMNAME_MAGIC 0xDEADCACABABECAFE
#define FDP2KD_FAKE_SINGLE_STEP_MAGIC 0xDEADCACABABEFACE

#define SIZEOF_KD_PACKET_HEADER 56




#pragma pack(push)
typedef struct _DBGKD_DEBUG_DATA_HEADER64 {
    uint64_t List[2];
    uint32_t OwnerTag;
    uint32_t Size;
}DBGKD_DEBUG_DATA_HEADER64;

typedef struct _KDDEBUGGER_DATA64 {
    DBGKD_DEBUG_DATA_HEADER64 Header;
    uint64_t KernBase;
    uint64_t BreakpointWithStatus;
    uint64_t SavedContext;
    uint16_t ThCallbackStack;
    uint16_t NextCallback;
    uint16_t FramePointer;
    uint16_t PaeEnabled;
    uint64_t KiCallUserMode;
    uint64_t KeUserCallbackDispatcher;
    uint64_t PsLoadedModuleList;
    uint64_t PsActiveProcessHead;
    uint64_t PspCidTable;
    uint64_t ExpSystemResourcesList;
    uint64_t ExpPagedPoolDescriptor;
    uint64_t ExpNumberOfPagedPools;
    uint64_t KeTimeIncrement;
    uint64_t KeBugCheckCallbackListHead;
    uint64_t KiBugcheckData;
    uint64_t IopErrorLogListHead;
    uint64_t ObpRootDirectoryObject;
    uint64_t ObpTypeObjectType;
    uint64_t MmSystemCacheStart;
    uint64_t MmSystemCacheEnd;
    uint64_t MmSystemCacheWs;
    uint64_t MmPfnDatabase;
    uint64_t MmSystemPtesStart;
    uint64_t MmSystemPtesEnd;
    uint64_t MmSubsectionBase;
    uint64_t MmNumberOfPagingFiles;
    uint64_t MmLowestPhysicalPage;
    uint64_t MmHighestPhysicalPage;
    uint64_t MmNumberOfPhysicalPages;
    uint64_t MmMaximumNonPagedPoolInBytes;
    uint64_t MmNonPagedSystemStart;
    uint64_t MmNonPagedPoolStart;
    uint64_t MmNonPagedPoolEnd;
    uint64_t MmPagedPoolStart;
    uint64_t MmPagedPoolEnd;
    uint64_t MmPagedPoolInformation;
    uint64_t MmPageSize;
    uint64_t MmSizeOfPagedPoolInBytes;
    uint64_t MmTotalCommitLimit;
    uint64_t MmTotalCommittedPages;
    uint64_t MmSharedCommit;
    uint64_t MmDriverCommit;
    uint64_t MmProcessCommit;
    uint64_t MmPagedPoolCommit;
    uint64_t MmExtendedCommit;
    uint64_t MmZeroedPageListHead;
    uint64_t MmFreePageListHead;
    uint64_t MmStandbyPageListHead;
    uint64_t MmModifiedPageListHead;
    uint64_t MmModifiedNoWritePageListHead;
    uint64_t MmAvailablePages;
    uint64_t MmResidentAvailablePages;
    uint64_t PoolTrackTable;
    uint64_t NonPagedPoolDescriptor;
    uint64_t MmHighestUserAddress;
    uint64_t MmSystemRangeStart;
    uint64_t MmUserProbeAddress;
    uint64_t KdPrintCircularBuffer;
    uint64_t KdPrintCircularBufferEnd;
    uint64_t KdPrintWritePointer;
    uint64_t KdPrintRolloverCount;
    uint64_t MmLoadedUserImageList;
    uint64_t NtBuildLab;
    uint64_t KiNormalSystemCall;
    uint64_t KiProcessorBlock;
    uint64_t MmUnloadedDrivers;
    uint64_t MmLastUnloadedDriver;
    uint64_t MmTriageActionTaken;
    uint64_t MmSpecialPoolTag;
    uint64_t KernelVerifier;
    uint64_t MmVerifierData;
    uint64_t MmAllocatedNonPagedPool;
    uint64_t MmPeakCommitment;
    uint64_t MmTotalCommitLimitMaximum;
    uint64_t CmNtCSDVersion;
    uint64_t MmPhysicalMemoryBlock;
    uint64_t MmSessionBase;
    uint64_t MmSessionSize;
    uint64_t MmSystemParentTablePage;
    uint64_t MmVirtualTranslationBase;
    uint16_t OffsetKThreadNextProcessor;
    uint16_t OffsetKThreadTeb;
    uint16_t OffsetKThreadKernelStack;
    uint16_t OffsetKThreadInitialStack;
    uint16_t OffsetKThreadApcProcess;
    uint16_t OffsetKThreadState;
    uint16_t OffsetKThreadBStore;
    uint16_t OffsetKThreadBStoreLimit;
    uint16_t SizeEProcess;
    uint16_t OffsetEprocessPeb;
    uint16_t OffsetEprocessParentCID;
    uint16_t OffsetEprocessDirectoryTableBase;
    uint16_t SizePrcb;
    uint16_t OffsetPrcbDpcRoutine;
    uint16_t OffsetPrcbCurrentThread;
    uint16_t OffsetPrcbMhz;
    uint16_t OffsetPrcbCpuType;
    uint16_t OffsetPrcbVendorString;
    uint16_t OffsetPrcbProcStateContext;
    uint16_t OffsetPrcbNumber;
    uint16_t SizeEThread;
    uint64_t KdPrintCircularBufferPtr;
    uint64_t KdPrintBufferSize;
    uint64_t KeLoaderBlock;
    uint16_t SizePcr;
    uint16_t OffsetPcrSelfPcr;
    uint16_t OffsetPcrCurrentPrcb;
    uint16_t OffsetPcrContainedPrcb;
    uint16_t OffsetPcrInitialBStore;
    uint16_t OffsetPcrBStoreLimit;
    uint16_t OffsetPcrInitialStack;
    uint16_t OffsetPcrStackLimit;
    uint16_t OffsetPrcbPcrPage;
    uint16_t OffsetPrcbProcStateSpecialReg;
    uint16_t GdtR0Code;
    uint16_t GdtR0Data;
    uint16_t GdtR0Pcr;
    uint16_t GdtR3Code;
    uint16_t GdtR3Data;
    uint16_t GdtR3Teb;
    uint16_t GdtLdt;
    uint16_t GdtTss;
    uint16_t Gdt64R3CmCode;
    uint16_t Gdt64R3CmTeb;
    uint64_t IopNumTriageDumpDataBlocks;
    uint64_t IopTriageDumpDataBlocks;
    uint64_t VfCrashDataBlock;
    uint64_t u[8];
} KDDEBUGGER_DATA64;
#pragma pack(pop)

typedef enum StubType{
    StubTypeCrash,
    StubTypeFdp,
    StubTypeGdb,
    StubTypeLiveKaDay
} StubType;



#pragma pack(push)
typedef void*(*pfnOpen_t)                   (char *);
typedef bool(*pfnReadVirtualMemory_t)       (void*, uint32_t, uint8_t *, uint32_t, uint64_t);
typedef bool(*pfnReadPhysicalMemory_t)      (void*, uint8_t *, uint32_t, uint64_t);
typedef bool(*pfnReadRegister_t)            (void*, uint32_t, uint16_t, uint64_t *);
typedef bool(*pfnResume_t)                  (void *);
typedef bool(*pfnSingleStep_t)              (void *, uint32_t);
typedef bool(*pfnPause_t)                   (void*);
typedef bool(*pfnWriteRegister_t)           (void *, uint32_t, uint16_t, uint64_t);
typedef bool(*pfnUnsetBreakpoint_t)         (void *, uint32_t);
typedef bool(*pfnVirtualToPhysical_t)       (void*, uint32_t, uint64_t, uint64_t *);
typedef bool(*pfnWritePhysicalMemory_t)     (void*, const void *, uint32_t, uint64_t);
typedef bool(*pfnWriteVirtualMemory_t)      (void*, uint32_t, const void *, uint32_t, uint64_t);
typedef bool(*pfnSearchPhysicalMemory_t)    (void*, const void*, uint32_t, uint64_t);
typedef bool(*pfnSetBreakpoint_t)           (void*, uint32_t, uint16_t, uint8_t, uint16_t, uint16_t, uint64_t, uint64_t, uint64_t);
typedef bool(*pfnGetCpuState_t)             (void*, uint32_t, void *);
typedef bool(*pfnGetFxState64_t)            (void*, uint32_t, void *);
typedef bool(*pfnSetFxState64_t)            (void*, uint32_t, void *);
typedef bool(*pfnReadMsr_t)                 (void*, uint32_t , uint64_t , uint64_t *);
typedef bool(*pfnInit_t)                    (void*);
typedef bool(*pfnGetPhysicalMemorySize_t)   (void*, uint64_t*);
typedef bool(*pfnWriteMsr_t)                (void*, uint32_t, uint64_t, uint64_t);
typedef bool(*pfnReboot_t)                  (void*);

#define _1M 1 * 1024 * 1024
#define WINBAGILITY_TEMP_BUFFER_SIZE _1M

typedef struct WINBAGILITY_CONTEXT_T_{
    //Is the Server have to run ?
    bool                ServerRunning;
    //Current Windows profil
    WINDOWS_PROFIL_T    *pCurrentWindowsProfil;
    //
    char                *pNamedPipePath;
    HANDLE                hWinDbgPipe;
    //
    HANDLE                hMainLoopThread;

    uint64_t            StartIdtrb;
    uint32_t            CurrentCpuId;
    uint32_t            CpuCount;
    uint64_t            v_KiProcessBlock;
    uint64_t            v_KPCR[64];
    uint64_t            v_KPRCB[64];
    uint64_t            v_KDBG;
    uint64_t            v_KernBase;
    //Encoded version of KDBG
    KDDEBUGGER_DATA64    EncodedKDBG;
    //Decoded version of KDBG
    KDDEBUGGER_DATA64    KDBG;
    //Keys for KDBG Decoding
    uint64_t            KdDebuggerDataBlock;
    uint64_t            KdpDebuggerDataListHead;
    uint64_t            KiWaitNever;
    uint64_t            KiWaitAlways;
    uint64_t            KdpDataBlockEncoded;
    char                TmpInputBuffer[WINBAGILITY_TEMP_BUFFER_SIZE]; //Used as a buffer to temporary Input KD packet in kdserver.cpp
    char                TmpOuputBuffer[WINBAGILITY_TEMP_BUFFER_SIZE]; //Used as a buffer to temporary Ouput KD packet in kdserver.cpp
    //Is Single step needed ?
    bool                bSingleStep;
    //Is Connection to WinDbg OK ?
    bool                bIsConnectionEstablished;
    //If true KdSingleStep will not call STUB_SingleStep, used to refresh, Windbg don't have refresh command
    bool                        bFakeSingleStepEnabled;
    //StubArguments
    StubType                    CurrentMode;
    char                        *pStubName;
    char                        *pStubOpenArg;
    //StubInterface
    void*                       pUserHandle;
    pfnOpen_t                   pfnOpen;
    pfnReadVirtualMemory_t      pfnReadVirtualMemory;
    pfnReadRegister_t           pfnReadRegister;
    pfnWriteRegister_t          pfnWriteRegister;
    pfnPause_t                  pfnPause;
    pfnResume_t                 pfnResume;
    pfnSingleStep_t             pfnSingleStep;
    pfnUnsetBreakpoint_t        pfnUnsetBreakpoint;
    pfnReadPhysicalMemory_t     pfnReadPhysicalMemory;
    pfnVirtualToPhysical_t      pfnVirtualToPhysical;
    pfnWritePhysicalMemory_t    pfnWritePhysicalMemory;
    pfnWriteVirtualMemory_t     pfnWriteVirtualMemory;
    pfnSearchPhysicalMemory_t   pfnSearchPhysicalMemory;
    pfnSetBreakpoint_t          pfnSetBreakpoint;
    pfnGetCpuState_t            pfnGetCpuState;
    pfnGetFxState64_t           pfnGetFxState64;
    pfnSetFxState64_t           pfnSetFxState64;
    pfnReadMsr_t                pfnReadMsr;
    pfnInit_t                   pfnInit;
    pfnGetPhysicalMemorySize_t  pfnGetPhysicalMemorySize;
    pfnWriteMsr_t               pfnWriteMsr;
    pfnReboot_t                 pfnReboot;
}WINBAGILITY_CONTEXT_T;
#pragma pack(pop)

//functions 
bool StartKdServer(WINBAGILITY_CONTEXT_T *curContext);
void StopKdServer(WINBAGILITY_CONTEXT_T *curContext);

#endif //__KDSERVER_H__
