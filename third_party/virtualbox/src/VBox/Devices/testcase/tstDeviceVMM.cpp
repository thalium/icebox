/* $Id: tstDeviceVMM.cpp $ */
/** @file
 * tstDevice - Test framework for PDM devices/drivers, VMM callbacks implementation.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEFAULT /** @todo */
#include <VBox/types.h>
#include <VBox/version.h>
#include <VBox/vmm/pdmpci.h>

#include <iprt/mem.h>

#include "tstDeviceInternal.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/

/** Frequency of the real clock. */
#define TMCLOCK_FREQ_REAL       UINT32_C(1000)
/** Frequency of the virtual clock. */
#define TMCLOCK_FREQ_VIRTUAL    UINT32_C(1000000000)


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/



/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/



/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/

/**
 * Resolves a path reference to a configuration item.
 *
 * @returns VBox status code.
 * @param   paDevCfg        The array of config items.
 * @param   pszName         Name of a byte string value.
 * @param   ppItem          Where to store the pointer to the item.
 */
static int tstDev_CfgmR3ResolveItem(PCTSTDEVCFGITEM paDevCfg, const char *pszName, PCTSTDEVCFGITEM *ppItem)
{
    *ppItem = NULL;
    if (!paDevCfg)
        return VERR_CFGM_VALUE_NOT_FOUND;

    size_t          cchName = strlen(pszName);
    PCTSTDEVCFGITEM pDevCfgItem = paDevCfg;
    while (pDevCfgItem->pszKey != NULL)
    {
        size_t cchKey = strlen(pDevCfgItem->pszKey);
        if (cchName == cchKey)
        {
            int iDiff = memcmp(pszName, pDevCfgItem->pszKey, cchName);
            if (iDiff <= 0)
            {
                if (iDiff != 0)
                    break;
                *ppItem = pDevCfgItem;
                return VINF_SUCCESS;
            }
        }

        /* next */
        pDevCfgItem++;
    }
    return VERR_CFGM_VALUE_NOT_FOUND;
}


static DECLCALLBACK(bool) tstDevVmm_CFGMR3AreValuesValid(PCFGMNODE pNode, const char *pszzValid)
{
    if (pNode && pNode->pDut->pTestcaseReg->paDevCfg)
    {
        PCTSTDEVCFGITEM pDevCfgItem = pNode->pDut->pTestcaseReg->paDevCfg;
        while (pDevCfgItem->pszKey != NULL)
        {
            size_t cchKey = strlen(pDevCfgItem->pszKey);

            /* search pszzValid for the name */
            const char *psz = pszzValid;
            while (*psz)
            {
                size_t cch = strlen(psz);
                if (    cch == cchKey
                    &&  !memcmp(psz, pDevCfgItem->pszKey, cch))
                    break;

                /* next */
                psz += cch + 1;
            }

            /* if at end of pszzValid we didn't find it => failure */
            if (!*psz)
                return false;

            pDevCfgItem++;
        }
    }

    return true;
}


static DECLCALLBACK(void) tstDevVmm_CFGMR3Dump(PCFGMNODE pRoot)
{
    RT_NOREF(pRoot);
    AssertFailed();
}


static DECLCALLBACK(PCFGMNODE) tstDevVmm_CFGMR3GetChild(PCFGMNODE pNode, const char *pszPath)
{
    RT_NOREF(pNode, pszPath);
    AssertFailed();
    return NULL;
}


static DECLCALLBACK(PCFGMNODE) tstDevVmm_CFGMR3GetChildFV(PCFGMNODE pNode, const char *pszPathFormat, va_list Args)
{
    RT_NOREF(pNode, pszPathFormat, Args);
    AssertFailed();
    return NULL;
}


static DECLCALLBACK(PCFGMNODE) tstDevVmm_CFGMR3GetFirstChild(PCFGMNODE pNode)
{
    RT_NOREF(pNode);
    AssertFailed();
    return NULL;
}


static DECLCALLBACK(int) tstDevVmm_CFGMR3GetName(PCFGMNODE pCur, char *pszName, size_t cchName)
{
    RT_NOREF(pCur, pszName, cchName);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(PCFGMNODE) tstDevVmm_CFGMR3GetNextChild(PCFGMNODE pCur)
{
    RT_NOREF(pCur);
    AssertFailed();
    return NULL;
}


static DECLCALLBACK(PCFGMNODE) tstDevVmm_CFGMR3GetParent(PCFGMNODE pNode)
{
    RT_NOREF(pNode);
    AssertFailed();
    return NULL;
}


static DECLCALLBACK(PCFGMNODE) tstDevVmm_CFGMR3GetRoot(PVM pVM)
{
    RT_NOREF(pVM);
    AssertFailed();
    return NULL;
}


static DECLCALLBACK(int) tstDevVmm_CFGMR3InsertNode(PCFGMNODE pNode, const char *pszName, PCFGMNODE *ppChild)
{
    RT_NOREF(pNode, pszName, ppChild);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_CFGMR3InsertNodeFV(PCFGMNODE pNode, PCFGMNODE *ppChild,
                                                      const char *pszNameFormat, va_list Args)
{
    RT_NOREF(pNode, ppChild, pszNameFormat, Args);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_CFGMR3InsertString(PCFGMNODE pNode, const char *pszName, const char *pszString)
{
    RT_NOREF(pNode, pszName, pszString);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_CFGMR3QueryBytes(PCFGMNODE pNode, const char *pszName, void *pvData, size_t cbData)
{
    RT_NOREF(pNode, pszName, pvData, cbData);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_CFGMR3QueryInteger(PCFGMNODE pNode, const char *pszName, uint64_t *pu64)
{
    if (!pNode)
        return VERR_CFGM_NO_PARENT;

    PCTSTDEVCFGITEM pCfgItem;
    int rc = tstDev_CfgmR3ResolveItem(pNode->pDut->pTestcaseReg->paDevCfg, pszName, &pCfgItem);
    if (RT_SUCCESS(rc))
    {
        if (pCfgItem->enmType == TSTDEVCFGITEMTYPE_INTEGER)
            *pu64 = RTStrToUInt64(pCfgItem->pszVal);
        else
            rc = VERR_CFGM_NOT_INTEGER;
    }

    return rc;
}


static DECLCALLBACK(int) tstDevVmm_CFGMR3QuerySize(PCFGMNODE pNode, const char *pszName, size_t *pcb)
{
    if (!pNode)
        return VERR_CFGM_NO_PARENT;

    PCTSTDEVCFGITEM pCfgItem;
    int rc = tstDev_CfgmR3ResolveItem(pNode->pDut->pTestcaseReg->paDevCfg, pszName, &pCfgItem);
    if (RT_SUCCESS(rc))
    {
        switch (pCfgItem->enmType)
        {
            case TSTDEVCFGITEMTYPE_INTEGER:
                *pcb = sizeof(uint64_t);
                break;

            case TSTDEVCFGITEMTYPE_STRING:
                *pcb = strlen(pCfgItem->pszVal) + 1;
                break;

            case TSTDEVCFGITEMTYPE_BYTES:
                AssertFailed();
                break;

            default:
                rc = VERR_CFGM_IPE_1;
                AssertMsgFailed(("Invalid value type %d\n", pCfgItem->enmType));
                break;
        }
    }
    return rc;
}


static DECLCALLBACK(int) tstDevVmm_CFGMR3QueryString(PCFGMNODE pNode, const char *pszName, char *pszString, size_t cchString)
{
    if (!pNode)
        return VERR_CFGM_NO_PARENT;

    PCTSTDEVCFGITEM pCfgItem;
    int rc = tstDev_CfgmR3ResolveItem(pNode->pDut->pTestcaseReg->paDevCfg, pszName, &pCfgItem);
    if (RT_SUCCESS(rc))
    {
        switch (pCfgItem->enmType)
        {
            case TSTDEVCFGITEMTYPE_STRING:
            {
                size_t cchVal = strlen(pCfgItem->pszVal);
                if (cchString <= cchVal + 1)
                    memcpy(pszString, pCfgItem->pszVal, cchVal);
                else
                    rc = VERR_CFGM_NOT_ENOUGH_SPACE;
                break;
            }
            case TSTDEVCFGITEMTYPE_INTEGER:
            case TSTDEVCFGITEMTYPE_BYTES:
            default:
                rc = VERR_CFGM_IPE_1;
                AssertMsgFailed(("Invalid value type %d\n", pCfgItem->enmType));
                break;
        }
    }
    else
        rc = VERR_CFGM_VALUE_NOT_FOUND;

    return rc;
}


static DECLCALLBACK(int) tstDevVmm_CFGMR3QueryStringAlloc(PCFGMNODE pNode, const char *pszName, char **ppszString)
{
    RT_NOREF(pNode, pszName, ppszString);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_CFGMR3QueryStringAllocDef(PCFGMNODE pNode, const char *pszName, char **ppszString, const char *pszDef)
{
    RT_NOREF(pNode, pszName, ppszString, pszDef);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(void) tstDevVmm_CFGMR3RemoveNode(PCFGMNODE pNode)
{
    RT_NOREF(pNode);
    AssertFailed();
}


static DECLCALLBACK(int) tstDevVmm_CFGMR3ValidateConfig(PCFGMNODE pNode, const char *pszNode,
                                                        const char *pszValidValues, const char *pszValidNodes,
                                                        const char *pszWho, uint32_t uInstance)
{
    RT_NOREF(pNode, pszNode, pszValidValues, pszValidNodes, pszWho, uInstance);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_IOMIOPortWrite(PVM pVM, PVMCPU pVCpu, RTIOPORT Port, uint32_t u32Value, size_t cbValue)
{
    RT_NOREF(pVM, pVCpu, Port, u32Value, cbValue);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_IOMMMIOMapMMIO2Page(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS GCPhysRemapped, uint64_t fPageFlags)
{
    RT_NOREF(pVM, GCPhys, GCPhysRemapped, fPageFlags);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_IOMMMIOResetRegion(PVM pVM, RTGCPHYS GCPhys)
{
    RT_NOREF(pVM, GCPhys);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_MMHyperAlloc(PVM pVM, size_t cb, uint32_t uAlignment, MMTAG enmTag, void **ppv)
{
    RT_NOREF(pVM, cb, uAlignment, enmTag, ppv);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_MMHyperFree(PVM pVM, void *pv)
{
    RT_NOREF(pVM, pv);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(RTR0PTR) tstDevVmm_MMHyperR3ToR0(PVM pVM, RTR3PTR R3Ptr)
{
    RT_NOREF(pVM, R3Ptr);
    AssertFailed();
    return NIL_RTR0PTR;
}


static DECLCALLBACK(RTRCPTR) tstDevVmm_MMHyperR3ToRC(PVM pVM, RTR3PTR R3Ptr)
{
    RT_NOREF(pVM, R3Ptr);
    AssertFailed();
    return NIL_RTRCPTR;
}


static DECLCALLBACK(void) tstDevVmm_MMR3HeapFree(void *pv)
{
    PTSTDEVMMHEAPALLOC pHeapAlloc = (PTSTDEVMMHEAPALLOC)((uint8_t *)pv - RT_OFFSETOF(TSTDEVMMHEAPALLOC, abAlloc[0]));
    PTSTDEVDUTINT pThis = pHeapAlloc->pDut;

    tstDevDutLockExcl(pThis);
    RTListNodeRemove(&pHeapAlloc->NdMmHeap);
    tstDevDutUnlockExcl(pThis);

    /* Poison */
    memset(&pHeapAlloc->abAlloc[0], 0xfc, pHeapAlloc->cbAlloc);
    RTMemFree(pHeapAlloc);
}


static DECLCALLBACK(int) tstDevVmm_MMR3HyperAllocOnceNoRel(PVM pVM, size_t cb, uint32_t uAlignment, MMTAG enmTag, void **ppv)
{
    RT_NOREF(pVM, cb, uAlignment, enmTag, ppv);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(uint64_t) tstDevVmm_MMR3PhysGetRamSize(PVM pVM)
{
    RT_NOREF(pVM);
    AssertFailed();
    return 0;
}


static DECLCALLBACK(uint64_t) tstDevVmm_MMR3PhysGetRamSizeAbove4GB(PVM pVM)
{
    RT_NOREF(pVM);
    AssertFailed();
    return 0;
}


static DECLCALLBACK(uint32_t) tstDevVmm_MMR3PhysGetRamSizeBelow4GB(PVM pVM)
{
    RT_NOREF(pVM);
    AssertFailed();
    return 0;
}


static DECLCALLBACK(int) tstDevVmm_PDMCritSectEnterDebug(PPDMCRITSECT pCritSect, int rcBusy, RTHCUINTPTR uId, RT_SRC_POS_DECL)
{
    RT_NOREF(rcBusy, uId, RT_SRC_POS_ARGS);
    return RTCritSectEnter(&pCritSect->s.CritSect);
}


static DECLCALLBACK(bool) tstDevVmm_PDMCritSectIsInitialized(PCPDMCRITSECT pCritSect)
{
    RT_NOREF(pCritSect);
    AssertFailed();
    return false;
}


static DECLCALLBACK(bool) tstDevVmm_PDMCritSectIsOwner(PCPDMCRITSECT pCritSect)
{
    return RTCritSectIsOwner(&pCritSect->s.CritSect);
}


static DECLCALLBACK(int) tstDevVmm_PDMCritSectLeave(PPDMCRITSECT pCritSect)
{
    return RTCritSectLeave(&pCritSect->s.CritSect);
}


static DECLCALLBACK(int) tstDevVmm_PDMCritSectTryEnterDebug(PPDMCRITSECT pCritSect, RTHCUINTPTR uId, RT_SRC_POS_DECL)
{
    RT_NOREF(pCritSect, uId, RT_SRC_POS_ARGS);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMHCCritSectScheduleExitEvent(PPDMCRITSECT pCritSect, SUPSEMEVENT hEventToSignal)
{
    RT_NOREF(pCritSect, hEventToSignal);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(bool) tstDevVmm_PDMNsAllocateBandwidth(PPDMNSFILTER pFilter, size_t cbTransfer)
{
    RT_NOREF(pFilter, cbTransfer);
    AssertFailed();
    return false;
}


static DECLCALLBACK(PPDMQUEUEITEMCORE) tstDevVmm_PDMQueueAlloc(PPDMQUEUE pQueue)
{
    RT_NOREF(pQueue);
    AssertFailed();
    return NULL;
}


static DECLCALLBACK(bool) tstDevVmm_PDMQueueFlushIfNecessary(PPDMQUEUE pQueue)
{
    RT_NOREF(pQueue);
    AssertFailed();
    return false;
}


static DECLCALLBACK(void) tstDevVmm_PDMQueueInsert(PPDMQUEUE pQueue, PPDMQUEUEITEMCORE pItem)
{
    RT_NOREF(pQueue, pItem);
    AssertFailed();
}


static DECLCALLBACK(R0PTRTYPE(PPDMQUEUE)) tstDevVmm_PDMQueueR0Ptr(PPDMQUEUE pQueue)
{
    RT_NOREF(pQueue);
    AssertFailed();
    return NIL_RTR0PTR;
}


static DECLCALLBACK(RCPTRTYPE(PPDMQUEUE)) tstDevVmm_PDMQueueRCPtr(PPDMQUEUE pQueue)
{
    RT_NOREF(pQueue);
    AssertFailed();
    return NIL_RTRCPTR;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3AsyncCompletionEpClose(PPDMASYNCCOMPLETIONENDPOINT pEndpoint)
{
    RT_NOREF(pEndpoint);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3AsyncCompletionEpCreateForFile(PPPDMASYNCCOMPLETIONENDPOINT ppEndpoint,
                                                                       const char *pszFilename, uint32_t fFlags,
                                                                       PPDMASYNCCOMPLETIONTEMPLATE pTemplate)
{
    RT_NOREF(ppEndpoint, pszFilename, fFlags, pTemplate);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3AsyncCompletionEpFlush(PPDMASYNCCOMPLETIONENDPOINT pEndpoint, void *pvUser,
                                                               PPPDMASYNCCOMPLETIONTASK ppTask)
{
    RT_NOREF(pEndpoint, pvUser, ppTask);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3AsyncCompletionEpGetSize(PPDMASYNCCOMPLETIONENDPOINT pEndpoint, uint64_t *pcbSize)
{
    RT_NOREF(pEndpoint, pcbSize);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3AsyncCompletionEpRead(PPDMASYNCCOMPLETIONENDPOINT pEndpoint, RTFOFF off,
                                                              PCRTSGSEG paSegments, unsigned cSegments,
                                                              size_t cbRead, void *pvUser,
                                                              PPPDMASYNCCOMPLETIONTASK ppTask)
{
    RT_NOREF(pEndpoint, off, paSegments, cSegments, cbRead, pvUser, ppTask);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3AsyncCompletionEpSetBwMgr(PPDMASYNCCOMPLETIONENDPOINT pEndpoint, const char *pszBwMgr)
{
    RT_NOREF(pEndpoint, pszBwMgr);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3AsyncCompletionEpSetSize(PPDMASYNCCOMPLETIONENDPOINT pEndpoint, uint64_t cbSize)
{
    RT_NOREF(pEndpoint, cbSize);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3AsyncCompletionEpWrite(PPDMASYNCCOMPLETIONENDPOINT pEndpoint, RTFOFF off,
                                                               PCRTSGSEG paSegments, unsigned cSegments,
                                                               size_t cbWrite, void *pvUser,
                                                               PPPDMASYNCCOMPLETIONTASK ppTask)
{
    RT_NOREF(pEndpoint, off, paSegments, cSegments, cbWrite, pvUser, ppTask);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3AsyncCompletionTemplateDestroy(PPDMASYNCCOMPLETIONTEMPLATE pTemplate)
{
    RT_NOREF(pTemplate);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3BlkCacheClear(PPDMBLKCACHE pBlkCache)
{
    RT_NOREF(pBlkCache);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3BlkCacheDiscard(PPDMBLKCACHE pBlkCache, PCRTRANGE paRanges,
                                                        unsigned cRanges, void *pvUser)
{
    RT_NOREF(pBlkCache, paRanges, cRanges, pvUser);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3BlkCacheFlush(PPDMBLKCACHE pBlkCache, void *pvUser)
{
    RT_NOREF(pBlkCache, pvUser);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3BlkCacheIoXferComplete(PPDMBLKCACHE pBlkCache, PPDMBLKCACHEIOXFER hIoXfer, int rcIoXfer)
{
    RT_NOREF(pBlkCache, hIoXfer, rcIoXfer);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3BlkCacheRead(PPDMBLKCACHE pBlkCache, uint64_t off, PCRTSGBUF pSgBuf, size_t cbRead, void *pvUser)
{
    RT_NOREF(pBlkCache, off, pSgBuf, cbRead, pvUser);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3BlkCacheRelease(PPDMBLKCACHE pBlkCache)
{
    RT_NOREF(pBlkCache);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3BlkCacheResume(PPDMBLKCACHE pBlkCache)
{
    RT_NOREF(pBlkCache);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3BlkCacheSuspend(PPDMBLKCACHE pBlkCache)
{
    RT_NOREF(pBlkCache);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3BlkCacheWrite(PPDMBLKCACHE pBlkCache, uint64_t off, PCRTSGBUF pSgBuf, size_t cbWrite, void *pvUser)
{
    RT_NOREF(pBlkCache, off, pSgBuf, cbWrite, pvUser);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3CritSectDelete(PPDMCRITSECT pCritSect)
{
    RT_NOREF(pCritSect);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3QueryLun(PUVM pUVM, const char *pszDevice, unsigned iInstance, unsigned iLun, PPPDMIBASE ppBase)
{
    RT_NOREF(pUVM, pszDevice, iInstance, iLun, ppBase);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3ThreadDestroy(PPDMTHREAD pThread, int *pRcThread)
{
    RT_NOREF(pThread, pRcThread);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3ThreadResume(PPDMTHREAD pThread)
{
    RT_NOREF(pThread);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3ThreadSleep(PPDMTHREAD pThread, RTMSINTERVAL cMillies)
{
    RT_NOREF(pThread, cMillies);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_PDMR3ThreadSuspend(PPDMTHREAD pThread)
{
    RT_NOREF(pThread);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(uint64_t) tstDevVmm_TMCpuTicksPerSecond(PVM pVM)
{
    RT_NOREF(pVM);
    AssertFailed();
    return 0;
}


static DECLCALLBACK(int) tstDevVmm_TMR3TimerDestroy(PTMTIMER pTimer)
{
    RT_NOREF(pTimer);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_TMR3TimerLoad(PTMTIMERR3 pTimer, PSSMHANDLE pSSM)
{
    RT_NOREF(pTimer, pSSM);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_TMR3TimerSave(PTMTIMERR3 pTimer, PSSMHANDLE pSSM)
{
    RT_NOREF(pTimer, pSSM);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_TMR3TimerSetCritSect(PTMTIMERR3 pTimer, PPDMCRITSECT pCritSect)
{
    RT_NOREF(pTimer, pCritSect);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(uint64_t) tstDevVmm_TMTimerFromMilli(PTMTIMER pTimer, uint64_t cMilliSecs)
{
    RT_NOREF(pTimer, cMilliSecs);
    AssertFailed();
    return 0;
}


static DECLCALLBACK(uint64_t) tstDevVmm_TMTimerFromNano(PTMTIMER pTimer, uint64_t cNanoSecs)
{
    RT_NOREF(pTimer, cNanoSecs);
    AssertFailed();
    return 0;
}


static DECLCALLBACK(uint64_t) tstDevVmm_TMTimerGet(PTMTIMER pTimer)
{
    RT_NOREF(pTimer);
    AssertFailed();
    return 0;
}


static DECLCALLBACK(uint64_t) tstDevVmm_TMTimerGetFreq(PTMTIMER pTimer)
{
    switch (pTimer->enmClock)
    {
        case TMCLOCK_VIRTUAL:
        case TMCLOCK_VIRTUAL_SYNC:
            return TMCLOCK_FREQ_VIRTUAL;

        case TMCLOCK_REAL:
            return TMCLOCK_FREQ_REAL;

        default:
            AssertMsgFailed(("Invalid enmClock=%d\n", pTimer->enmClock));
            return 0;
    }
}


static DECLCALLBACK(uint64_t) tstDevVmm_TMTimerGetNano(PTMTIMER pTimer)
{
    RT_NOREF(pTimer);
    AssertFailed();
    return 0;
}


static DECLCALLBACK(bool) tstDevVmm_TMTimerIsActive(PTMTIMER pTimer)
{
    RT_NOREF(pTimer);
    AssertFailed();
    return false;
}


static DECLCALLBACK(bool) tstDevVmm_TMTimerIsLockOwner(PTMTIMER pTimer)
{
    RT_NOREF(pTimer);
    AssertFailed();
    return false;
}


static DECLCALLBACK(int) tstDevVmm_TMTimerLock(PTMTIMER pTimer, int rcBusy)
{
    RT_NOREF(pTimer, rcBusy);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(PTMTIMERR0) tstDevVmm_TMTimerR0Ptr(PTMTIMER pTimer)
{
    return (PTMTIMERR0)pTimer;
}


static DECLCALLBACK(PTMTIMERRC) tstDevVmm_TMTimerRCPtr(PTMTIMER pTimer)
{
    RT_NOREF(pTimer);
    /* Impossible to implement RC modules at the moment. */
    return NIL_RTRCPTR;
}


static DECLCALLBACK(int) tstDevVmm_TMTimerSet(PTMTIMER pTimer, uint64_t u64Expire)
{
    RT_NOREF(pTimer, u64Expire);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_TMTimerSetFrequencyHint(PTMTIMER pTimer, uint32_t uHz)
{
    RT_NOREF(pTimer, uHz);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_TMTimerSetMicro(PTMTIMER pTimer, uint64_t cMicrosToNext)
{
    RT_NOREF(pTimer, cMicrosToNext);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_TMTimerSetMillies(PTMTIMER pTimer, uint32_t cMilliesToNext)
{
    RT_NOREF(pTimer, cMilliesToNext);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_TMTimerSetNano(PTMTIMER pTimer, uint64_t cNanosToNext)
{
    RT_NOREF(pTimer, cNanosToNext);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_TMTimerStop(PTMTIMER pTimer)
{
    RT_NOREF(pTimer);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(void) tstDevVmm_TMTimerUnlock(PTMTIMER pTimer)
{
    RT_NOREF(pTimer);
    AssertFailed();
}


static DECLCALLBACK(PVMCPU) tstDevVmm_VMMGetCpu(PVM pVM)
{
    RT_NOREF(pVM);
    AssertFailed();
    return NULL;
}


static DECLCALLBACK(VMCPUID) tstDevVmm_VMMGetCpuId(PVM pVM)
{
    RT_NOREF(pVM);
    AssertFailed();
    return NIL_VMCPUID;
}


static DECLCALLBACK(int) tstDevVmm_VMMR3DeregisterPatchMemory(PVM pVM, RTGCPTR pPatchMem, unsigned cbPatchMem)
{
    RT_NOREF(pVM, pPatchMem, cbPatchMem);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_VMMR3RegisterPatchMemory(PVM pVM, RTGCPTR pPatchMem, unsigned cbPatchMem)
{
    RT_NOREF(pVM, pPatchMem, cbPatchMem);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(RTNATIVETHREAD) tstDevVmm_VMR3GetVMCPUNativeThread(PVM pVM)
{
    RT_NOREF(pVM);
    AssertFailed();
    return NIL_RTNATIVETHREAD;
}


static DECLCALLBACK(int) tstDevVmm_VMR3NotifyCpuDeviceReady(PVM pVM, VMCPUID idCpu)
{
    RT_NOREF(pVM, idCpu);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_VMR3ReqCallNoWait(PVM pVM, VMCPUID idDstCpu, PFNRT pfnFunction, unsigned cArgs, ...)
{
    RT_NOREF(pVM, idDstCpu, pfnFunction, cArgs);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_VMR3ReqCallVoidNoWait(PVM pVM, VMCPUID idDstCpu, PFNRT pfnFunction, unsigned cArgs, ...)
{
    RT_NOREF(pVM, idDstCpu, pfnFunction, cArgs);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_VMR3ReqPriorityCallWait(PVM pVM, VMCPUID idDstCpu, PFNRT pfnFunction, unsigned cArgs, ...)
{
    RT_NOREF(pVM, idDstCpu, pfnFunction, cArgs);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(int) tstDevVmm_VMR3WaitForDeviceReady(PVM pVM, VMCPUID idCpu)
{
    RT_NOREF(pVM, idCpu);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}



const TSTDEVVMMCALLBACKS g_tstDevVmmCallbacks =
{
    tstDevVmm_CFGMR3AreValuesValid,
    tstDevVmm_CFGMR3Dump,
    tstDevVmm_CFGMR3GetChild,
    tstDevVmm_CFGMR3GetChildFV,
    tstDevVmm_CFGMR3GetFirstChild,
    tstDevVmm_CFGMR3GetName,
    tstDevVmm_CFGMR3GetNextChild,
    tstDevVmm_CFGMR3GetParent,
    tstDevVmm_CFGMR3GetRoot,
    tstDevVmm_CFGMR3InsertNode,
    tstDevVmm_CFGMR3InsertNodeFV,
    tstDevVmm_CFGMR3InsertString,
    tstDevVmm_CFGMR3QueryBytes,
    tstDevVmm_CFGMR3QueryInteger,
    tstDevVmm_CFGMR3QuerySize,
    tstDevVmm_CFGMR3QueryString,
    tstDevVmm_CFGMR3QueryStringAlloc,
    tstDevVmm_CFGMR3QueryStringAllocDef,
    tstDevVmm_CFGMR3RemoveNode,
    tstDevVmm_CFGMR3ValidateConfig,
    tstDevVmm_IOMIOPortWrite,
    tstDevVmm_IOMMMIOMapMMIO2Page,
    tstDevVmm_IOMMMIOResetRegion,
    tstDevVmm_MMHyperAlloc,
    tstDevVmm_MMHyperFree,
    tstDevVmm_MMHyperR3ToR0,
    tstDevVmm_MMHyperR3ToRC,
    tstDevVmm_MMR3HeapFree,
    tstDevVmm_MMR3HyperAllocOnceNoRel,
    tstDevVmm_MMR3PhysGetRamSize,
    tstDevVmm_MMR3PhysGetRamSizeAbove4GB,
    tstDevVmm_MMR3PhysGetRamSizeBelow4GB,
    tstDevVmm_PDMCritSectEnterDebug,
    tstDevVmm_PDMCritSectIsInitialized,
    tstDevVmm_PDMCritSectIsOwner,
    tstDevVmm_PDMCritSectLeave,
    tstDevVmm_PDMCritSectTryEnterDebug,
    tstDevVmm_PDMHCCritSectScheduleExitEvent,
    tstDevVmm_PDMNsAllocateBandwidth,
    tstDevVmm_PDMQueueAlloc,
    tstDevVmm_PDMQueueFlushIfNecessary,
    tstDevVmm_PDMQueueInsert,
    tstDevVmm_PDMQueueR0Ptr,
    tstDevVmm_PDMQueueRCPtr,
    tstDevVmm_PDMR3AsyncCompletionEpClose,
    tstDevVmm_PDMR3AsyncCompletionEpCreateForFile,
    tstDevVmm_PDMR3AsyncCompletionEpFlush,
    tstDevVmm_PDMR3AsyncCompletionEpGetSize,
    tstDevVmm_PDMR3AsyncCompletionEpRead,
    tstDevVmm_PDMR3AsyncCompletionEpSetBwMgr,
    tstDevVmm_PDMR3AsyncCompletionEpSetSize,
    tstDevVmm_PDMR3AsyncCompletionEpWrite,
    tstDevVmm_PDMR3AsyncCompletionTemplateDestroy,
    tstDevVmm_PDMR3BlkCacheClear,
    tstDevVmm_PDMR3BlkCacheDiscard,
    tstDevVmm_PDMR3BlkCacheFlush,
    tstDevVmm_PDMR3BlkCacheIoXferComplete,
    tstDevVmm_PDMR3BlkCacheRead,
    tstDevVmm_PDMR3BlkCacheRelease,
    tstDevVmm_PDMR3BlkCacheResume,
    tstDevVmm_PDMR3BlkCacheSuspend,
    tstDevVmm_PDMR3BlkCacheWrite,
    tstDevVmm_PDMR3CritSectDelete,
    tstDevVmm_PDMR3QueryLun,
    tstDevVmm_PDMR3ThreadDestroy,
    tstDevVmm_PDMR3ThreadResume,
    tstDevVmm_PDMR3ThreadSleep,
    tstDevVmm_PDMR3ThreadSuspend,
    tstDevVmm_TMCpuTicksPerSecond,
    tstDevVmm_TMR3TimerDestroy,
    tstDevVmm_TMR3TimerLoad,
    tstDevVmm_TMR3TimerSave,
    tstDevVmm_TMR3TimerSetCritSect,
    tstDevVmm_TMTimerFromMilli,
    tstDevVmm_TMTimerFromNano,
    tstDevVmm_TMTimerGet,
    tstDevVmm_TMTimerGetFreq,
    tstDevVmm_TMTimerGetNano,
    tstDevVmm_TMTimerIsActive,
    tstDevVmm_TMTimerIsLockOwner,
    tstDevVmm_TMTimerLock,
    tstDevVmm_TMTimerR0Ptr,
    tstDevVmm_TMTimerRCPtr,
    tstDevVmm_TMTimerSet,
    tstDevVmm_TMTimerSetFrequencyHint,
    tstDevVmm_TMTimerSetMicro,
    tstDevVmm_TMTimerSetMillies,
    tstDevVmm_TMTimerSetNano,
    tstDevVmm_TMTimerStop,
    tstDevVmm_TMTimerUnlock,
    tstDevVmm_VMMGetCpu,
    tstDevVmm_VMMGetCpuId,
    tstDevVmm_VMMR3DeregisterPatchMemory,
    tstDevVmm_VMMR3RegisterPatchMemory,
    tstDevVmm_VMR3GetVMCPUNativeThread,
    tstDevVmm_VMR3NotifyCpuDeviceReady,
    tstDevVmm_VMR3ReqCallNoWait,
    tstDevVmm_VMR3ReqCallVoidNoWait,
    tstDevVmm_VMR3ReqPriorityCallWait,
    tstDevVmm_VMR3WaitForDeviceReady
};

