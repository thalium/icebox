/* $Id: tstDeviceVMMInternal.h $ */
/** @file
 * tstDevice - Test framework for PDM devices/drivers, definitions of VMM internal types.
 */

/*
 * Copyright (C) 2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#ifndef ___tstDeviceVMMInternal_h
#define ___tstDeviceVMMInternal_h

#include <VBox/types.h>
#include <VBox/vmm/tm.h>
#include <iprt/critsect.h>
#include <iprt/list.h>

RT_C_DECLS_BEGIN

/** Pointer to a VMM callback table. */
typedef struct TSTDEVVMMCALLBACKS *PTSTDEVVMMCALLBACKS;
/** Pointer to a constant VMM callback table. */
typedef const struct TSTDEVVMMCALLBACKS *PCTSTDEVVMMCALLBACKS;

/** Pointer to the internal device under test instance. */
typedef struct TSTDEVDUTINT *PTSTDEVDUTINT;
/** Pointer to a constant device under test instance. */
typedef const struct TSTDEVDUTINT *PCTSTDEVDUTINT;

/**
 * Private device instance data.
 */
typedef struct PDMDEVINSINT
{
    /** Pointer to the callback table. */
    PCTSTDEVVMMCALLBACKS            pVmmCallbacks;
    /** Pointer to the device under test the PDM device instance is for. */
    PTSTDEVDUTINT                   pDut;
} PDMDEVINSINT;
AssertCompile(sizeof(PDMDEVINSINT) <= (HC_ARCH_BITS == 32 ? 72 : 112 + 0x28));

/**
 * CFGM node structure.
 */
typedef struct CFGMNODE
{
    /** Pointer to the callback table. */
    PCTSTDEVVMMCALLBACKS pVmmCallbacks;
    /** Device under test this CFGM node is for. */
    PTSTDEVDUTINT        pDut;
    /** @todo: */
} CFGMNODE;

/**
 * PDM queue structure.
 */
typedef struct PDMQUEUE
{
    /** Pointer to the callback table. */
    PCTSTDEVVMMCALLBACKS pVmmCallbacks;
    /** @todo: */
} PDMQUEUE;

/**
 * TM timer structure.
 */
typedef struct TMTIMER
{
    /** Pointer to the callback table. */
    PCTSTDEVVMMCALLBACKS pVmmCallbacks;
    /** List of timers created by the device. */
    RTLISTNODE           NdDevTimers;
    /** Clock this timer belongs to. */
    TMCLOCK              enmClock;
    /** Callback to call when the timer expires. */
    PFNTMTIMERDEV        pfnCallbackDev;
    /** Opaque user data to pass to the callback. */
    void                 *pvUser;
    /** Flags. */
    uint32_t             fFlags;
    /** @todo: */
} TMTIMER;

/**
 * Internal VMM structure of VMCPU
 */
typedef struct VMMCPU
{
    /** Pointer to the callback table. */
    PCTSTDEVVMMCALLBACKS pVmmCallbacks;
    /** @todo: */
} VMMCPU;
AssertCompile(sizeof(VMMCPU) <= 704);

/**
 * Internal VMM structure of VM
 */
typedef struct VMM
{
    /** Pointer to the callback table. */
    PCTSTDEVVMMCALLBACKS pVmmCallbacks;
    /** @todo: */
} VMM;
AssertCompile(sizeof(VMM) <= 1600);

/**
 * Internal VM structure of VM
 */
typedef struct VMINT
{
    /** Pointer to the callback table. */
    PCTSTDEVVMMCALLBACKS pVmmCallbacks;
    /** @todo: */
} VMINT;
AssertCompile(sizeof(VMINT) <= 24);

/**
 * Internal per vCPU user VM structure.
 */
typedef struct VMINTUSERPERVMCPU
{
    /** Pointer to the callback table. */
    PCTSTDEVVMMCALLBACKS pVmmCallbacks;
    /** @todo: */
} VMINTUSERPERVMCPU;
AssertCompile(sizeof(VMINTUSERPERVMCPU) <= 512);

/**
 * Internal user VM structure.
 */
typedef struct VMINTUSERPERVM
{
    /** Pointer to the callback table. */
    PCTSTDEVVMMCALLBACKS pVmmCallbacks;
    /** @todo: */
} VMINTUSERPERVM;
AssertCompile(sizeof(VMINTUSERPERVM) <= 512);

/**
 * Internal PDM critical section structure.
 */
typedef struct PDMCRITSECTINT
{
    /** Pointer to the callback table. */
    PCTSTDEVVMMCALLBACKS pVmmCallbacks;
    /** The actual critical section used for emulation. */
    RTCRITSECT           CritSect;
} PDMCRITSECTINT;
AssertCompile(sizeof(PDMCRITSECTINT) <= (HC_ARCH_BITS == 32 ? 0x80 : 0xc0));

/**
 * Internal PDM thread instance data.
 */
typedef struct PDMTHREADINT
{
    /** Pointer to the callback table. */
    PCTSTDEVVMMCALLBACKS            pVmmCallbacks;
    /** Pointer to the device under test. */
    PTSTDEVDUTINT                   pDut;
    /** Node for the list of PDM threads. */
    RTLISTNODE                      NdPdmThreads;
} PDMTHREADINT;
AssertCompile(sizeof(PDMTHREADINT) <= 64);

/**
 * MM Heap allocation.
 */
typedef struct TSTDEVMMHEAPALLOC
{
    /** Node for the list of allocations. */
    RTLISTNODE                      NdMmHeap;
    /** Pointer to the callback table. */
    PCTSTDEVVMMCALLBACKS            pVmmCallbacks;
    /** Pointer to the device under test the allocation was made for. */
    PTSTDEVDUTINT                   pDut;
    /** Size of the allocation. */
    size_t                          cbAlloc;
    /** Align the next member on a 16byte boundary. */
    size_t                          uAlignment0;
    /** Start of the real allocation. */
    uint8_t                         abAlloc[RT_FLEXIBLE_ARRAY];
} TSTDEVMMHEAPALLOC;
/** Pointer to a MM Heap allocation. */
typedef TSTDEVMMHEAPALLOC *PTSTDEVMMHEAPALLOC;
/** Pointer to a const MM Heap allocation. */
typedef const TSTDEVMMHEAPALLOC *PCTSTDEVMMHEAPALLOC;

AssertCompileMemberAlignment(TSTDEVMMHEAPALLOC, abAlloc, HC_ARCH_BITS == 64 ? 16 : 8);

#if 0
/**
 * Internal PDM netshaper filter instance data.
 */
typedef struct PDMNSFILTER
{
    /** Pointer to the callback table. */
    PCTSTDEVVMMCALLBACKS pVmmCallbacks;
    /** @todo: */
} PDMNSFILTER;
#endif

#define PDMCRITSECTINT_DECLARED
#define PDMTHREADINT_DECLARED
#define PDMDEVINSINT_DECLARED
#define ___VMInternal_h
#define ___VMMInternal_h
RT_C_DECLS_END
#include <VBox/vmm/pdmcritsect.h>
#include <VBox/vmm/vm.h>
#include <VBox/vmm/uvm.h>
#include <VBox/vmm/cfgm.h>
#include <VBox/vmm/tm.h>
#include <VBox/vmm/pdmblkcache.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmqueue.h>
#include <VBox/vmm/pdmthread.h>
#include <VBox/vmm/cfgm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/pdmasynccompletion.h>
#include <VBox/vmm/pdmnetshaper.h>
#include <VBox/vmm/pgm.h>
RT_C_DECLS_BEGIN


/**
 * Callback table from the VMM shim library to the PDM test framework.
 */
typedef struct TSTDEVVMMCALLBACKS
{
    DECLR3CALLBACKMEMBER(bool, pfnCFGMR3AreValuesValid, (PCFGMNODE pNode, const char *pszzValid));
    DECLR3CALLBACKMEMBER(void, pfnCFGMR3Dump, (PCFGMNODE pRoot));
    DECLR3CALLBACKMEMBER(PCFGMNODE, pfnCFGMR3GetChild, (PCFGMNODE pNode, const char *pszPath));
    DECLR3CALLBACKMEMBER(PCFGMNODE, pfnCFGMR3GetChildFV, (PCFGMNODE pNode, const char *pszPathFormat, va_list Args));
    DECLR3CALLBACKMEMBER(PCFGMNODE, pfnCFGMR3GetFirstChild, (PCFGMNODE pNode));
    DECLR3CALLBACKMEMBER(int, pfnCFGMR3GetName, (PCFGMNODE pCur, char *pszName, size_t cchName));
    DECLR3CALLBACKMEMBER(PCFGMNODE, pfnCFGMR3GetNextChild, (PCFGMNODE pCur));
    DECLR3CALLBACKMEMBER(PCFGMNODE, pfnCFGMR3GetParent, (PCFGMNODE pNode));
    DECLR3CALLBACKMEMBER(PCFGMNODE, pfnCFGMR3GetRoot, (PVM pVM));
    DECLR3CALLBACKMEMBER(int, pfnCFGMR3InsertNode, (PCFGMNODE pNode, const char *pszName, PCFGMNODE *ppChild));
    DECLR3CALLBACKMEMBER(int, pfnCFGMR3InsertNodeFV, (PCFGMNODE pNode, PCFGMNODE *ppChild,
                                                     const char *pszNameFormat, va_list Args));
    DECLR3CALLBACKMEMBER(int, pfnCFGMR3InsertString, (PCFGMNODE pNode, const char *pszName, const char *pszString));
    DECLR3CALLBACKMEMBER(int, pfnCFGMR3QueryBytes, (PCFGMNODE pNode, const char *pszName, void *pvData, size_t cbData));
    DECLR3CALLBACKMEMBER(int, pfnCFGMR3QueryInteger, (PCFGMNODE pNode, const char *pszName, uint64_t *pu64));
    DECLR3CALLBACKMEMBER(int, pfnCFGMR3QuerySize, (PCFGMNODE pNode, const char *pszName, size_t *pcb));
    DECLR3CALLBACKMEMBER(int, pfnCFGMR3QueryString, (PCFGMNODE pNode, const char *pszName, char *pszString, size_t cchString));
    DECLR3CALLBACKMEMBER(int, pfnCFGMR3QueryStringAlloc, (PCFGMNODE pNode, const char *pszName, char **ppszString));
    DECLR3CALLBACKMEMBER(int, pfnCFGMR3QueryStringAllocDef, (PCFGMNODE pNode, const char *pszName, char **ppszString, const char *pszDef));
    DECLR3CALLBACKMEMBER(void, pfnCFGMR3RemoveNode, (PCFGMNODE pNode));
    DECLR3CALLBACKMEMBER(int, pfnCFGMR3ValidateConfig, (PCFGMNODE pNode, const char *pszNode,
                                                        const char *pszValidValues, const char *pszValidNodes,
                                                        const char *pszWho, uint32_t uInstance));

    DECLR3CALLBACKMEMBER(int, pfnIOMIOPortWrite, (PVM pVM, PVMCPU pVCpu, RTIOPORT Port, uint32_t u32Value, size_t cbValue));
    DECLR3CALLBACKMEMBER(int, pfnIOMMMIOMapMMIO2Page, (PVM pVM, RTGCPHYS GCPhys, RTGCPHYS GCPhysRemapped, uint64_t fPageFlags));
    DECLR3CALLBACKMEMBER(int, pfnIOMMMIOResetRegion, (PVM pVM, RTGCPHYS GCPhys));
    DECLR3CALLBACKMEMBER(int, pfnMMHyperAlloc, (PVM pVM, size_t cb, uint32_t uAlignment, MMTAG enmTag, void **ppv));
    DECLR3CALLBACKMEMBER(int, pfnMMHyperFree, (PVM pVM, void *pv));
    DECLR3CALLBACKMEMBER(RTR0PTR, pfnMMHyperR3ToR0, (PVM pVM, RTR3PTR R3Ptr));
    DECLR3CALLBACKMEMBER(RTRCPTR, pfnMMHyperR3ToRC, (PVM pVM, RTR3PTR R3Ptr));
    DECLR3CALLBACKMEMBER(void, pfnMMR3HeapFree, (void *pv));
    DECLR3CALLBACKMEMBER(int, pfnMMR3HyperAllocOnceNoRel, (PVM pVM, size_t cb, uint32_t uAlignment, MMTAG enmTag, void **ppv));
    DECLR3CALLBACKMEMBER(uint64_t, pfnMMR3PhysGetRamSize, (PVM pVM));
    DECLR3CALLBACKMEMBER(uint64_t, pfnMMR3PhysGetRamSizeAbove4GB, (PVM pVM));
    DECLR3CALLBACKMEMBER(uint32_t, pfnMMR3PhysGetRamSizeBelow4GB, (PVM pVM));
    DECLR3CALLBACKMEMBER(int, pfnPDMCritSectEnterDebug, (PPDMCRITSECT pCritSect, int rcBusy, RTHCUINTPTR uId, RT_SRC_POS_DECL));
    DECLR3CALLBACKMEMBER(bool, pfnPDMCritSectIsInitialized, (PCPDMCRITSECT pCritSect));
    DECLR3CALLBACKMEMBER(bool, pfnPDMCritSectIsOwner, (PCPDMCRITSECT pCritSect));
    DECLR3CALLBACKMEMBER(int, pfnPDMCritSectLeave, (PPDMCRITSECT pCritSect));
    DECLR3CALLBACKMEMBER(int, pfnPDMCritSectTryEnterDebug, (PPDMCRITSECT pCritSect, RTHCUINTPTR uId, RT_SRC_POS_DECL));
    DECLR3CALLBACKMEMBER(int, pfnPDMHCCritSectScheduleExitEvent, (PPDMCRITSECT pCritSect, SUPSEMEVENT hEventToSignal));
    DECLR3CALLBACKMEMBER(bool, pfnPDMNsAllocateBandwidth, (PPDMNSFILTER pFilter, size_t cbTransfer));
    DECLR3CALLBACKMEMBER(PPDMQUEUEITEMCORE, pfnPDMQueueAlloc, (PPDMQUEUE pQueue));
    DECLR3CALLBACKMEMBER(bool, pfnPDMQueueFlushIfNecessary, (PPDMQUEUE pQueue));
    DECLR3CALLBACKMEMBER(void, pfnPDMQueueInsert, (PPDMQUEUE pQueue, PPDMQUEUEITEMCORE pItem));
    DECLR3CALLBACKMEMBER(R0PTRTYPE(PPDMQUEUE), pfnPDMQueueR0Ptr, (PPDMQUEUE pQueue));
    DECLR3CALLBACKMEMBER(RCPTRTYPE(PPDMQUEUE), pfnPDMQueueRCPtr, (PPDMQUEUE pQueue));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3AsyncCompletionEpClose, (PPDMASYNCCOMPLETIONENDPOINT pEndpoint));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3AsyncCompletionEpCreateForFile, (PPPDMASYNCCOMPLETIONENDPOINT ppEndpoint,
                                                                       const char *pszFilename, uint32_t fFlags,
                                                                       PPDMASYNCCOMPLETIONTEMPLATE pTemplate));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3AsyncCompletionEpFlush, (PPDMASYNCCOMPLETIONENDPOINT pEndpoint, void *pvUser,
                                                               PPPDMASYNCCOMPLETIONTASK ppTask));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3AsyncCompletionEpGetSize, (PPDMASYNCCOMPLETIONENDPOINT pEndpoint, uint64_t *pcbSize));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3AsyncCompletionEpRead, (PPDMASYNCCOMPLETIONENDPOINT pEndpoint, RTFOFF off,
                                                              PCRTSGSEG paSegments, unsigned cSegments,
                                                              size_t cbRead, void *pvUser,
                                                              PPPDMASYNCCOMPLETIONTASK ppTask));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3AsyncCompletionEpSetBwMgr, (PPDMASYNCCOMPLETIONENDPOINT pEndpoint, const char *pszBwMgr));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3AsyncCompletionEpSetSize, (PPDMASYNCCOMPLETIONENDPOINT pEndpoint, uint64_t cbSize));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3AsyncCompletionEpWrite, (PPDMASYNCCOMPLETIONENDPOINT pEndpoint, RTFOFF off,
                                                               PCRTSGSEG paSegments, unsigned cSegments,
                                                               size_t cbWrite, void *pvUser,
                                                               PPPDMASYNCCOMPLETIONTASK ppTask));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3AsyncCompletionTemplateDestroy, (PPDMASYNCCOMPLETIONTEMPLATE pTemplate));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3BlkCacheClear, (PPDMBLKCACHE pBlkCache));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3BlkCacheDiscard, (PPDMBLKCACHE pBlkCache, PCRTRANGE paRanges,
                                                        unsigned cRanges, void *pvUser));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3BlkCacheFlush, (PPDMBLKCACHE pBlkCache, void *pvUser));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3BlkCacheIoXferComplete, (PPDMBLKCACHE pBlkCache, PPDMBLKCACHEIOXFER hIoXfer, int rcIoXfer));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3BlkCacheRead, (PPDMBLKCACHE pBlkCache, uint64_t off, PCRTSGBUF pSgBuf, size_t cbRead, void *pvUser));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3BlkCacheRelease, (PPDMBLKCACHE pBlkCache));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3BlkCacheResume, (PPDMBLKCACHE pBlkCache));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3BlkCacheSuspend, (PPDMBLKCACHE pBlkCache));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3BlkCacheWrite, (PPDMBLKCACHE pBlkCache, uint64_t off, PCRTSGBUF pSgBuf, size_t cbWrite, void *pvUser));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3CritSectDelete, (PPDMCRITSECT pCritSect));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3QueryLun, (PUVM pUVM, const char *pszDevice, unsigned iInstance, unsigned iLun, PPPDMIBASE ppBase));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3ThreadDestroy, (PPDMTHREAD pThread, int *pRcThread));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3ThreadResume, (PPDMTHREAD pThread));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3ThreadSleep, (PPDMTHREAD pThread, RTMSINTERVAL cMillies));
    DECLR3CALLBACKMEMBER(int, pfnPDMR3ThreadSuspend, (PPDMTHREAD pThread));
    DECLR3CALLBACKMEMBER(uint64_t, pfnTMCpuTicksPerSecond, (PVM pVM));
    DECLR3CALLBACKMEMBER(int, pfnTMR3TimerDestroy, (PTMTIMER pTimer));
    DECLR3CALLBACKMEMBER(int, pfnTMR3TimerLoad, (PTMTIMERR3 pTimer, PSSMHANDLE pSSM));
    DECLR3CALLBACKMEMBER(int, pfnTMR3TimerSave, (PTMTIMERR3 pTimer, PSSMHANDLE pSSM));
    DECLR3CALLBACKMEMBER(int, pfnTMR3TimerSetCritSect, (PTMTIMERR3 pTimer, PPDMCRITSECT pCritSect));
    DECLR3CALLBACKMEMBER(uint64_t, pfnTMTimerFromMilli, (PTMTIMER pTimer, uint64_t cMilliSecs));
    DECLR3CALLBACKMEMBER(uint64_t, pfnTMTimerFromNano, (PTMTIMER pTimer, uint64_t cNanoSecs));
    DECLR3CALLBACKMEMBER(uint64_t, pfnTMTimerGet, (PTMTIMER pTimer));
    DECLR3CALLBACKMEMBER(uint64_t, pfnTMTimerGetFreq, (PTMTIMER pTimer));
    DECLR3CALLBACKMEMBER(uint64_t, pfnTMTimerGetNano, (PTMTIMER pTimer));
    DECLR3CALLBACKMEMBER(bool, pfnTMTimerIsActive, (PTMTIMER pTimer));
    DECLR3CALLBACKMEMBER(bool, pfnTMTimerIsLockOwner, (PTMTIMER pTimer));
    DECLR3CALLBACKMEMBER(int, pfnTMTimerLock, (PTMTIMER pTimer, int rcBusy));
    DECLR3CALLBACKMEMBER(PTMTIMERR0, pfnTMTimerR0Ptr, (PTMTIMER pTimer));
    DECLR3CALLBACKMEMBER(PTMTIMERRC, pfnTMTimerRCPtr, (PTMTIMER pTimer));
    DECLR3CALLBACKMEMBER(int, pfnTMTimerSet, (PTMTIMER pTimer, uint64_t u64Expire));
    DECLR3CALLBACKMEMBER(int, pfnTMTimerSetFrequencyHint, (PTMTIMER pTimer, uint32_t uHz));
    DECLR3CALLBACKMEMBER(int, pfnTMTimerSetMicro, (PTMTIMER pTimer, uint64_t cMicrosToNext));
    DECLR3CALLBACKMEMBER(int, pfnTMTimerSetMillies, (PTMTIMER pTimer, uint32_t cMilliesToNext));
    DECLR3CALLBACKMEMBER(int, pfnTMTimerSetNano, (PTMTIMER pTimer, uint64_t cNanosToNext));
    DECLR3CALLBACKMEMBER(int, pfnTMTimerStop, (PTMTIMER pTimer));
    DECLR3CALLBACKMEMBER(void, pfnTMTimerUnlock, (PTMTIMER pTimer));
    DECLR3CALLBACKMEMBER(PVMCPU, pfnVMMGetCpu, (PVM pVM));
    DECLR3CALLBACKMEMBER(VMCPUID, pfnVMMGetCpuId, (PVM pVM));
    DECLR3CALLBACKMEMBER(int, pfnVMMR3DeregisterPatchMemory, (PVM pVM, RTGCPTR pPatchMem, unsigned cbPatchMem));
    DECLR3CALLBACKMEMBER(int, pfnVMMR3RegisterPatchMemory, (PVM pVM, RTGCPTR pPatchMem, unsigned cbPatchMem));
    DECLR3CALLBACKMEMBER(RTNATIVETHREAD, pfnVMR3GetVMCPUNativeThread, (PVM pVM));
    DECLR3CALLBACKMEMBER(int, pfnVMR3NotifyCpuDeviceReady, (PVM pVM, VMCPUID idCpu));
    DECLR3CALLBACKMEMBER(int, pfnVMR3ReqCallNoWait, (PVM pVM, VMCPUID idDstCpu, PFNRT pfnFunction, unsigned cArgs, ...));
    DECLR3CALLBACKMEMBER(int, pfnVMR3ReqCallVoidNoWait, (PVM pVM, VMCPUID idDstCpu, PFNRT pfnFunction, unsigned cArgs, ...));
    DECLR3CALLBACKMEMBER(int, pfnVMR3ReqPriorityCallWait, (PVM pVM, VMCPUID idDstCpu, PFNRT pfnFunction, unsigned cArgs, ...));
    DECLR3CALLBACKMEMBER(int, pfnVMR3WaitForDeviceReady, (PVM pVM, VMCPUID idCpu));
} TSTDEVVMMCALLBACKS;


extern const PDMDEVHLPR3 g_tstDevPdmDevHlpR3;
extern const TSTDEVVMMCALLBACKS g_tstDevVmmCallbacks;

RT_C_DECLS_END

#endif

