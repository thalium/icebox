/* $Id: PDMNetShaper.cpp $ */
/** @file
 * PDM Network Shaper - Limit network traffic according to bandwidth group settings.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_NET_SHAPER
#include "PDMInternal.h"
#include <VBox/vmm/pdm.h>
#include <VBox/vmm/mm.h>
#ifdef VBOX_WITH_REM
# include <VBox/vmm/rem.h>
#endif
#include <VBox/vmm/vm.h>
#include <VBox/vmm/uvm.h>
#include <VBox/err.h>

#include <VBox/log.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/thread.h>
#include <iprt/mem.h>
#include <iprt/critsect.h>
#include <iprt/tcp.h>
#include <iprt/path.h>
#include <iprt/string.h>

#include <VBox/vmm/pdmnetshaper.h>
#include "PDMNetShaperInternal.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * Network shaper data. One instance per VM.
 */
typedef struct PDMNETSHAPER
{
    /** Pointer to the VM. */
    PVM                      pVM;
    /** Critical section protecting all members below. */
    RTCRITSECT               Lock;
    /** Pending TX thread. */
    PPDMTHREAD               pTxThread;
    /** Pointer to the first bandwidth group. */
    PPDMNSBWGROUP            pBwGroupsHead;
} PDMNETSHAPER;


/** Takes the shaper lock (asserts but doesn't return or anything on
 *  failure). */
#define LOCK_NETSHAPER(a_pShaper) do { int rcShaper = RTCritSectEnter(&(a_pShaper)->Lock); AssertRC(rcShaper); } while (0)

/** Takes the shaper lock, returns + asserts on failure. */
#define LOCK_NETSHAPER_RETURN(a_pShaper) \
    do { int rcShaper = RTCritSectEnter(&(a_pShaper)->Lock); AssertRCReturn(rcShaper, rcShaper); } while (0)

/** Releases the shaper lock (asserts on failure). */
#define UNLOCK_NETSHAPER(a_pShaper) do { int rcShaper = RTCritSectLeave(&(a_pShaper)->Lock); AssertRC(rcShaper); } while (0)




static PPDMNSBWGROUP pdmNsBwGroupFindById(PPDMNETSHAPER pShaper, const char *pszId)
{
    PPDMNSBWGROUP pBwGroup = NULL;

    if (RT_VALID_PTR(pszId))
    {
        LOCK_NETSHAPER(pShaper);

        pBwGroup = pShaper->pBwGroupsHead;
        while (   pBwGroup
               && RTStrCmp(pBwGroup->pszNameR3, pszId))
            pBwGroup = pBwGroup->pNextR3;

        UNLOCK_NETSHAPER(pShaper);
    }

    return pBwGroup;
}


static void pdmNsBwGroupLink(PPDMNSBWGROUP pBwGroup)
{
    PPDMNETSHAPER pShaper = pBwGroup->pShaperR3;
    LOCK_NETSHAPER(pShaper);

    pBwGroup->pNextR3 = pShaper->pBwGroupsHead;
    pShaper->pBwGroupsHead = pBwGroup;

    UNLOCK_NETSHAPER(pShaper);
}


#if 0
static void pdmNsBwGroupUnlink(PPDMNSBWGROUP pBwGroup)
{
    PPDMNETSHAPER pShaper = pBwGroup->pShaper;
    LOCK_NETSHAPER(pShaper);

    if (pBwGroup == pShaper->pBwGroupsHead)
        pShaper->pBwGroupsHead = pBwGroup->pNext;
    else
    {
        PPDMNSBWGROUP pPrev = pShaper->pBwGroupsHead;
        while (   pPrev
               && pPrev->pNext != pBwGroup)
            pPrev = pPrev->pNext;

        AssertPtr(pPrev);
        pPrev->pNext = pBwGroup->pNext;
    }

    UNLOCK_NETSHAPER(pShaper);
}
#endif


static void pdmNsBwGroupSetLimit(PPDMNSBWGROUP pBwGroup, uint64_t cbPerSecMax)
{
    pBwGroup->cbPerSecMax = cbPerSecMax;
    pBwGroup->cbBucket    = RT_MAX(PDM_NETSHAPER_MIN_BUCKET_SIZE, cbPerSecMax * PDM_NETSHAPER_MAX_LATENCY / 1000);
    LogFlow(("pdmNsBwGroupSetLimit: New rate limit is %llu bytes per second, adjusted bucket size to %u bytes\n",
             pBwGroup->cbPerSecMax, pBwGroup->cbBucket));
}


static int pdmNsBwGroupCreate(PPDMNETSHAPER pShaper, const char *pszBwGroup, uint64_t cbPerSecMax)
{
    LogFlow(("pdmNsBwGroupCreate: pShaper=%#p pszBwGroup=%#p{%s} cbPerSecMax=%llu\n", pShaper, pszBwGroup, pszBwGroup, cbPerSecMax));

    AssertPtrReturn(pShaper, VERR_INVALID_POINTER);
    AssertPtrReturn(pszBwGroup, VERR_INVALID_POINTER);
    AssertReturn(*pszBwGroup != '\0', VERR_INVALID_PARAMETER);

    int         rc;
    PPDMNSBWGROUP pBwGroup = pdmNsBwGroupFindById(pShaper, pszBwGroup);
    if (!pBwGroup)
    {
        rc = MMHyperAlloc(pShaper->pVM, sizeof(PDMNSBWGROUP), 64,
                          MM_TAG_PDM_NET_SHAPER, (void **)&pBwGroup);
        if (RT_SUCCESS(rc))
        {
            rc = PDMR3CritSectInit(pShaper->pVM, &pBwGroup->Lock, RT_SRC_POS, "BWGRP-%s", pszBwGroup);
            if (RT_SUCCESS(rc))
            {
                pBwGroup->pszNameR3 = MMR3HeapStrDup(pShaper->pVM, MM_TAG_PDM_NET_SHAPER, pszBwGroup);
                if (pBwGroup->pszNameR3)
                {
                    pBwGroup->pShaperR3             = pShaper;
                    pBwGroup->cRefs                 = 0;

                    pdmNsBwGroupSetLimit(pBwGroup, cbPerSecMax);

                    pBwGroup->cbTokensLast          = pBwGroup->cbBucket;
                    pBwGroup->tsUpdatedLast         = RTTimeSystemNanoTS();

                    LogFlowFunc(("pszBwGroup={%s} cbBucket=%u\n",
                                 pszBwGroup, pBwGroup->cbBucket));
                    pdmNsBwGroupLink(pBwGroup);
                    return VINF_SUCCESS;
                }
                PDMR3CritSectDelete(&pBwGroup->Lock);
            }
            MMHyperFree(pShaper->pVM, pBwGroup);
        }
        else
            rc = VERR_NO_MEMORY;
    }
    else
        rc = VERR_ALREADY_EXISTS;

    LogFlowFunc(("returns rc=%Rrc\n", rc));
    return rc;
}


static void pdmNsBwGroupTerminate(PPDMNSBWGROUP pBwGroup)
{
    Assert(pBwGroup->cRefs == 0);
    if (PDMCritSectIsInitialized(&pBwGroup->Lock))
        PDMR3CritSectDelete(&pBwGroup->Lock);
}


DECLINLINE(void) pdmNsBwGroupRef(PPDMNSBWGROUP pBwGroup)
{
    ASMAtomicIncU32(&pBwGroup->cRefs);
}


DECLINLINE(void) pdmNsBwGroupUnref(PPDMNSBWGROUP pBwGroup)
{
    Assert(pBwGroup->cRefs > 0);
    ASMAtomicDecU32(&pBwGroup->cRefs);
}


static void pdmNsBwGroupXmitPending(PPDMNSBWGROUP pBwGroup)
{
    /*
     * We don't need to hold the bandwidth group lock to iterate over the list
     * of filters since the filters are removed while the shaper lock is being
     * held.
     */
    AssertPtr(pBwGroup);
    AssertPtr(pBwGroup->pShaperR3);
    Assert(RTCritSectIsOwner(&pBwGroup->pShaperR3->Lock));
    //LOCK_NETSHAPER(pShaper);

    /* Check if the group is disabled. */
    if (pBwGroup->cbPerSecMax == 0)
        return;

    PPDMNSFILTER pFilter = pBwGroup->pFiltersHeadR3;
    while (pFilter)
    {
        bool fChoked = ASMAtomicXchgBool(&pFilter->fChoked, false);
        Log3((LOG_FN_FMT ": pFilter=%#p fChoked=%RTbool\n", __PRETTY_FUNCTION__, pFilter, fChoked));
        if (fChoked && pFilter->pIDrvNetR3)
        {
            LogFlowFunc(("Calling pfnXmitPending for pFilter=%#p\n", pFilter));
            pFilter->pIDrvNetR3->pfnXmitPending(pFilter->pIDrvNetR3);
        }

        pFilter = pFilter->pNextR3;
    }

    //UNLOCK_NETSHAPER(pShaper);
}


static void pdmNsFilterLink(PPDMNSFILTER pFilter)
{
    PPDMNSBWGROUP pBwGroup = pFilter->pBwGroupR3;
    int rc = PDMCritSectEnter(&pBwGroup->Lock, VERR_SEM_BUSY); AssertRC(rc);

    pFilter->pNextR3 = pBwGroup->pFiltersHeadR3;
    pBwGroup->pFiltersHeadR3 = pFilter;

    rc = PDMCritSectLeave(&pBwGroup->Lock); AssertRC(rc);
}


static void pdmNsFilterUnlink(PPDMNSFILTER pFilter)
{
    PPDMNSBWGROUP pBwGroup = pFilter->pBwGroupR3;
    /*
     * We need to make sure we hold the shaper lock since pdmNsBwGroupXmitPending()
     * does not hold the bandwidth group lock while iterating over the list
     * of group's filters.
     */
    AssertPtr(pBwGroup);
    AssertPtr(pBwGroup->pShaperR3);
    Assert(RTCritSectIsOwner(&pBwGroup->pShaperR3->Lock));
    int rc = PDMCritSectEnter(&pBwGroup->Lock, VERR_SEM_BUSY); AssertRC(rc);

    if (pFilter == pBwGroup->pFiltersHeadR3)
        pBwGroup->pFiltersHeadR3 = pFilter->pNextR3;
    else
    {
        PPDMNSFILTER pPrev = pBwGroup->pFiltersHeadR3;
        while (   pPrev
               && pPrev->pNextR3 != pFilter)
            pPrev = pPrev->pNextR3;

        AssertPtr(pPrev);
        pPrev->pNextR3 = pFilter->pNextR3;
    }

    rc = PDMCritSectLeave(&pBwGroup->Lock); AssertRC(rc);
}


/**
 * Attach network filter driver from bandwidth group.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM structure.
 * @param   pDrvIns     The driver instance.
 * @param   pszBwGroup  Name of the bandwidth group to attach to.
 * @param   pFilter     Pointer to the filter we attach.
 */
VMMR3_INT_DECL(int) PDMR3NsAttach(PUVM pUVM, PPDMDRVINS pDrvIns, const char *pszBwGroup, PPDMNSFILTER pFilter)
{
    VM_ASSERT_EMT(pUVM->pVM);
    AssertPtrReturn(pFilter, VERR_INVALID_POINTER);
    AssertReturn(pFilter->pBwGroupR3 == NULL, VERR_ALREADY_EXISTS);
    RT_NOREF_PV(pDrvIns);

    PPDMNETSHAPER pShaper = pUVM->pdm.s.pNetShaper;
    LOCK_NETSHAPER_RETURN(pShaper);

    int             rc          = VINF_SUCCESS;
    PPDMNSBWGROUP   pBwGroupNew = NULL;
    if (pszBwGroup)
    {
        pBwGroupNew = pdmNsBwGroupFindById(pShaper, pszBwGroup);
        if (pBwGroupNew)
            pdmNsBwGroupRef(pBwGroupNew);
        else
            rc = VERR_NOT_FOUND;
    }

    if (RT_SUCCESS(rc))
    {
        PPDMNSBWGROUP pBwGroupOld = ASMAtomicXchgPtrT(&pFilter->pBwGroupR3, pBwGroupNew, PPDMNSBWGROUP);
        ASMAtomicWritePtr(&pFilter->pBwGroupR0, MMHyperR3ToR0(pUVM->pVM, pBwGroupNew));
        if (pBwGroupOld)
            pdmNsBwGroupUnref(pBwGroupOld);
        pdmNsFilterLink(pFilter);
    }

    UNLOCK_NETSHAPER(pShaper);
    return rc;
}


/**
 * Detach network filter driver from bandwidth group.
 *
 * @returns VBox status code.
 * @param   pUVM            The user mode VM handle.
 * @param   pDrvIns         The driver instance.
 * @param   pFilter         Pointer to the filter we detach.
 */
VMMR3_INT_DECL(int) PDMR3NsDetach(PUVM pUVM, PPDMDRVINS pDrvIns, PPDMNSFILTER pFilter)
{
    RT_NOREF_PV(pDrvIns);
    VM_ASSERT_EMT(pUVM->pVM);
    AssertPtrReturn(pFilter, VERR_INVALID_POINTER);

    /* Now, return quietly if the filter isn't attached since driver/device
       destructors are called on constructor failure. */
    if (!pFilter->pBwGroupR3)
        return VINF_SUCCESS;
    AssertPtrReturn(pFilter->pBwGroupR3, VERR_INVALID_POINTER);

    PPDMNETSHAPER pShaper = pUVM->pdm.s.pNetShaper;
    LOCK_NETSHAPER_RETURN(pShaper);

    pdmNsFilterUnlink(pFilter);
    PPDMNSBWGROUP pBwGroup = ASMAtomicXchgPtrT(&pFilter->pBwGroupR3, NULL, PPDMNSBWGROUP);
    if (pBwGroup)
        pdmNsBwGroupUnref(pBwGroup);

    UNLOCK_NETSHAPER(pShaper);
    return VINF_SUCCESS;
}


/**
 * Adjusts the maximum rate for the bandwidth group.
 *
 * @returns VBox status code.
 * @param   pUVM            The user mode VM handle.
 * @param   pszBwGroup      Name of the bandwidth group to attach to.
 * @param   cbPerSecMax     Maximum number of bytes per second to be transmitted.
 */
VMMR3DECL(int) PDMR3NsBwGroupSetLimit(PUVM pUVM, const char *pszBwGroup, uint64_t cbPerSecMax)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    PPDMNETSHAPER pShaper = pUVM->pdm.s.pNetShaper;
    LOCK_NETSHAPER_RETURN(pShaper);

    int           rc;
    PPDMNSBWGROUP pBwGroup = pdmNsBwGroupFindById(pShaper, pszBwGroup);
    if (pBwGroup)
    {
        rc = PDMCritSectEnter(&pBwGroup->Lock, VERR_SEM_BUSY); AssertRC(rc);
        if (RT_SUCCESS(rc))
        {
            pdmNsBwGroupSetLimit(pBwGroup, cbPerSecMax);

            /* Drop extra tokens */
            if (pBwGroup->cbTokensLast > pBwGroup->cbBucket)
                pBwGroup->cbTokensLast = pBwGroup->cbBucket;

            int rc2 = PDMCritSectLeave(&pBwGroup->Lock); AssertRC(rc2);
        }
    }
    else
        rc = VERR_NOT_FOUND;

    UNLOCK_NETSHAPER(pShaper);
    return rc;
}


/**
 * I/O thread for pending TX.
 *
 * @returns VINF_SUCCESS (ignored).
 * @param   pVM         The cross context VM structure.
 * @param   pThread     The PDM thread data.
 */
static DECLCALLBACK(int) pdmR3NsTxThread(PVM pVM, PPDMTHREAD pThread)
{
    RT_NOREF_PV(pVM);

    PPDMNETSHAPER pShaper = (PPDMNETSHAPER)pThread->pvUser;
    LogFlow(("pdmR3NsTxThread: pShaper=%p\n", pShaper));
    while (pThread->enmState == PDMTHREADSTATE_RUNNING)
    {
        RTThreadSleep(PDM_NETSHAPER_MAX_LATENCY);

        /* Go over all bandwidth groups/filters calling pfnXmitPending */
        LOCK_NETSHAPER(pShaper);
        PPDMNSBWGROUP pBwGroup = pShaper->pBwGroupsHead;
        while (pBwGroup)
        {
            pdmNsBwGroupXmitPending(pBwGroup);
            pBwGroup = pBwGroup->pNextR3;
        }
        UNLOCK_NETSHAPER(pShaper);
    }
    return VINF_SUCCESS;
}


/**
 * @copydoc FNPDMTHREADWAKEUPINT
 */
static DECLCALLBACK(int) pdmR3NsTxWakeUp(PVM pVM, PPDMTHREAD pThread)
{
    RT_NOREF2(pVM, pThread);
    LogFlow(("pdmR3NsTxWakeUp: pShaper=%p\n", pThread->pvUser));
    /* Nothing to do */
    return VINF_SUCCESS;
}


/**
 * Terminate the network shaper.
 *
 * @returns VBox error code.
 * @param   pVM  The cross context VM structure.
 *
 * @remarks This method destroys all bandwidth group objects.
 */
int pdmR3NetShaperTerm(PVM pVM)
{
    PUVM pUVM = pVM->pUVM;
    AssertPtrReturn(pUVM, VERR_INVALID_POINTER);
    PPDMNETSHAPER pShaper = pUVM->pdm.s.pNetShaper;
    AssertPtrReturn(pShaper, VERR_INVALID_POINTER);

    /* Destroy the bandwidth managers. */
    PPDMNSBWGROUP pBwGroup = pShaper->pBwGroupsHead;
    while (pBwGroup)
    {
        PPDMNSBWGROUP pFree = pBwGroup;
        pBwGroup = pBwGroup->pNextR3;
        pdmNsBwGroupTerminate(pFree);
        MMR3HeapFree(pFree->pszNameR3);
        MMHyperFree(pVM, pFree);
    }

    RTCritSectDelete(&pShaper->Lock);
    MMR3HeapFree(pShaper);
    pUVM->pdm.s.pNetShaper = NULL;
    return VINF_SUCCESS;
}


/**
 * Initialize the network shaper.
 *
 * @returns VBox status code
 * @param   pVM The cross context VM structure.
 */
int pdmR3NetShaperInit(PVM pVM)
{
    LogFlow(("pdmR3NetShaperInit: pVM=%p\n", pVM));
    VM_ASSERT_EMT(pVM);
    PUVM pUVM = pVM->pUVM;
    AssertMsgReturn(!pUVM->pdm.s.pNetShaper, ("Network shaper was already initialized\n"), VERR_WRONG_ORDER);

    PPDMNETSHAPER pShaper;
    int rc = MMR3HeapAllocZEx(pVM, MM_TAG_PDM_NET_SHAPER, sizeof(PDMNETSHAPER), (void **)&pShaper);
    if (RT_SUCCESS(rc))
    {
        PCFGMNODE pCfgNetShaper = CFGMR3GetChild(CFGMR3GetChild(CFGMR3GetRoot(pVM), "PDM"), "NetworkShaper");

        pShaper->pVM = pVM;
        rc = RTCritSectInit(&pShaper->Lock);
        if (RT_SUCCESS(rc))
        {
            /* Create all bandwidth groups. */
            PCFGMNODE pCfgBwGrp = CFGMR3GetChild(pCfgNetShaper, "BwGroups");
            if (pCfgBwGrp)
            {
                for (PCFGMNODE pCur = CFGMR3GetFirstChild(pCfgBwGrp); pCur; pCur = CFGMR3GetNextChild(pCur))
                {
                    size_t cbName = CFGMR3GetNameLen(pCur) + 1;
                    char *pszBwGrpId = (char *)RTMemAllocZ(cbName);
                    if (pszBwGrpId)
                    {
                        rc = CFGMR3GetName(pCur, pszBwGrpId, cbName);
                        if (RT_SUCCESS(rc))
                        {
                            uint64_t cbMax;
                            rc = CFGMR3QueryU64(pCur, "Max", &cbMax);
                            if (RT_SUCCESS(rc))
                                rc = pdmNsBwGroupCreate(pShaper, pszBwGrpId, cbMax);
                        }
                        RTMemFree(pszBwGrpId);
                    }
                    else
                        rc = VERR_NO_MEMORY;
                    if (RT_FAILURE(rc))
                        break;
                }
            }

            if (RT_SUCCESS(rc))
            {
                rc = PDMR3ThreadCreate(pVM, &pShaper->pTxThread, pShaper, pdmR3NsTxThread, pdmR3NsTxWakeUp,
                                       0 /*cbStack*/, RTTHREADTYPE_IO, "PDMNsTx");
                if (RT_SUCCESS(rc))
                {
                    pUVM->pdm.s.pNetShaper = pShaper;
                    return VINF_SUCCESS;
                }
            }

            RTCritSectDelete(&pShaper->Lock);
        }

        MMR3HeapFree(pShaper);
    }

    LogFlow(("pdmR3NetShaperInit: pVM=%p rc=%Rrc\n", pVM, rc));
    return rc;
}

