/* $Id: VUSBUrbPool.cpp $ */
/** @file
 * Virtual USB - URB pool.
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_DRV_VUSB
#include <VBox/log.h>
#include <VBox/err.h>
#include <iprt/mem.h>
#include <iprt/critsect.h>

#include "VUSBInternal.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/

/** Maximum age for one URB. */
#define VUSBURB_AGE_MAX 10

/** Convert from an URB to the URB header. */
#define VUSBURBPOOL_URB_2_URBHDR(a_pUrb) RT_FROM_MEMBER(a_pUrb, VUSBURBHDR, Urb);


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * URB header not visible to the caller allocating an URB
 * and only for internal tracking.
 */
typedef struct VUSBURBHDR
{
    /** List node for keeping the URB in the free list. */
    RTLISTNODE  NdFree;
    /** Size of the data allocated for the URB (Only the variable part including the
     * HCI and TDs). */
    size_t      cbAllocated;
    /** Age of the URB waiting on the list, if it is waiting for too long without being used
     * again it will be freed. */
    uint32_t    cAge;
#if HC_ARCH_BITS == 64
    uint32_t    u32Alignment0;
#endif
    /** The embedded URB. */
    VUSBURB     Urb;
} VUSBURBHDR;
/** Pointer to a URB header. */
typedef VUSBURBHDR *PVUSBURBHDR;

AssertCompileSizeAlignment(VUSBURBHDR, 8);


/*********************************************************************************************************************************
*   Static Variables                                                                                                             *
*********************************************************************************************************************************/


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/

DECLHIDDEN(int) vusbUrbPoolInit(PVUSBURBPOOL pUrbPool)
{
    int rc = RTCritSectInit(&pUrbPool->CritSectPool);
    if (RT_SUCCESS(rc))
    {
        pUrbPool->cUrbsInPool = 0;
        for (unsigned i = 0; i < RT_ELEMENTS(pUrbPool->aLstFreeUrbs); i++)
            RTListInit(&pUrbPool->aLstFreeUrbs[i]);
    }

    return rc;
}


DECLHIDDEN(void) vusbUrbPoolDestroy(PVUSBURBPOOL pUrbPool)
{
    RTCritSectEnter(&pUrbPool->CritSectPool);
    for (unsigned i = 0; i < RT_ELEMENTS(pUrbPool->aLstFreeUrbs); i++)
    {
        PVUSBURBHDR pHdr, pHdrNext;
        RTListForEachSafe(&pUrbPool->aLstFreeUrbs[i], pHdr, pHdrNext, VUSBURBHDR, NdFree)
        {
            RTListNodeRemove(&pHdr->NdFree);

            pHdr->cbAllocated  = 0;
            pHdr->Urb.u32Magic = 0;
            pHdr->Urb.enmState = VUSBURBSTATE_INVALID;
            RTMemFree(pHdr);
        }
    }
    RTCritSectLeave(&pUrbPool->CritSectPool);
    RTCritSectDelete(&pUrbPool->CritSectPool);
}


DECLHIDDEN(PVUSBURB) vusbUrbPoolAlloc(PVUSBURBPOOL pUrbPool, VUSBXFERTYPE enmType,
                                      VUSBDIRECTION enmDir, size_t cbData, size_t cbHci,
                                      size_t cbHciTd, unsigned cTds)
{
    Assert((uint32_t)cbData == cbData);
    Assert((uint32_t)cbHci == cbHci);

    /*
     * Reuse or allocate a new URB.
     */
    /** @todo The allocations should be done by the device, at least as an option, since the devices
     * frequently wish to associate their own stuff with the in-flight URB or need special buffering
     * (isochronous on Darwin for instance). */
    /* Get the required amount of additional memory to allocate the whole state. */
    size_t cbMem = cbData + sizeof(VUSBURBVUSBINT) + cbHci + cTds * cbHciTd;

    AssertReturn(enmType < RT_ELEMENTS(pUrbPool->aLstFreeUrbs), NULL);

    RTCritSectEnter(&pUrbPool->CritSectPool);
    PVUSBURBHDR pHdr = NULL;
    PVUSBURBHDR pIt, pItNext;
    RTListForEachSafe(&pUrbPool->aLstFreeUrbs[enmType], pIt, pItNext, VUSBURBHDR, NdFree)
    {
        if (pIt->cbAllocated >= cbMem)
        {
            RTListNodeRemove(&pIt->NdFree);
            Assert(pIt->Urb.u32Magic == VUSBURB_MAGIC);
            Assert(pIt->Urb.enmState == VUSBURBSTATE_FREE);
            /*
             * If the allocation is far too big we increase the age counter too
             * so we don't waste memory for a lot of small transfers
             */
            if (pIt->cbAllocated >= 2 * cbMem)
                pIt->cAge++;
            else
                pIt->cAge = 0;
            pHdr = pIt;
            break;
        }
        else
        {
            /* Increase age and free if it reached a threshold. */
            pIt->cAge++;
            if (pIt->cAge == VUSBURB_AGE_MAX)
            {
                RTListNodeRemove(&pIt->NdFree);
                ASMAtomicDecU32(&pUrbPool->cUrbsInPool);
                RTMemFree(pIt);
            }
        }
    }

    if (!pHdr)
    {
        /* allocate a new one. */
        size_t cbDataAllocated = cbMem <= _4K  ? RT_ALIGN_32(cbMem, _1K)
                               : cbMem <= _32K ? RT_ALIGN_32(cbMem, _4K)
                                               : RT_ALIGN_32(cbMem, 16*_1K);

        pHdr = (PVUSBURBHDR)RTMemAllocZ(RT_OFFSETOF(VUSBURBHDR, Urb.abData[cbDataAllocated]));
        if (RT_UNLIKELY(!pHdr))
        {
            RTCritSectLeave(&pUrbPool->CritSectPool);
            AssertLogRelFailedReturn(NULL);
        }

        pHdr->cbAllocated = cbDataAllocated;
        pHdr->cAge        = 0;
        ASMAtomicIncU32(&pUrbPool->cUrbsInPool);
    }
    RTCritSectLeave(&pUrbPool->CritSectPool);

    Assert(pHdr->cbAllocated >= cbMem);

    /*
     * (Re)init the URB
     */
    uint32_t offAlloc = (uint32_t)cbData;
    PVUSBURB pUrb = &pHdr->Urb;
    pUrb->u32Magic               = VUSBURB_MAGIC;
    pUrb->enmState               = VUSBURBSTATE_ALLOCATED;
    pUrb->fCompleting            = false;
    pUrb->pszDesc                = NULL;
    pUrb->pVUsb                  = (PVUSBURBVUSB)&pUrb->abData[offAlloc];
    offAlloc += sizeof(VUSBURBVUSBINT);
    pUrb->pVUsb->pUrb            = pUrb;
    pUrb->pVUsb->pvFreeCtx       = NULL;
    pUrb->pVUsb->pfnFree         = NULL;
    pUrb->pVUsb->pCtrlUrb        = NULL;
    pUrb->pVUsb->u64SubmitTS     = 0;
    pUrb->Dev.pvPrivate          = NULL;
    pUrb->Dev.pNext              = NULL;
    pUrb->EndPt                  = UINT8_MAX;
    pUrb->enmType                = enmType;
    pUrb->enmDir                 = enmDir;
    pUrb->fShortNotOk            = false;
    pUrb->enmStatus              = VUSBSTATUS_INVALID;
    pUrb->cbData                 = (uint32_t)cbData;
    pUrb->pHci                   = cbHci ? (PVUSBURBHCI)&pUrb->abData[offAlloc] : NULL;
    offAlloc += (uint32_t)cbHci;
    pUrb->paTds                  = (cbHciTd && cTds) ? (PVUSBURBHCITD)&pUrb->abData[offAlloc] : NULL;

    return pUrb;
}


DECLHIDDEN(void) vusbUrbPoolFree(PVUSBURBPOOL pUrbPool, PVUSBURB pUrb)
{
    PVUSBURBHDR pHdr = VUSBURBPOOL_URB_2_URBHDR(pUrb);

    /* URBs which aged too much because they are too big are freed. */
    if (pHdr->cAge == VUSBURB_AGE_MAX)
    {
        ASMAtomicDecU32(&pUrbPool->cUrbsInPool);
        RTMemFree(pHdr);
    }
    else
    {
        /* Put it into the list of free URBs. */
        VUSBXFERTYPE enmType = pUrb->enmType;
        AssertReturnVoid(enmType < RT_ELEMENTS(pUrbPool->aLstFreeUrbs));
        RTCritSectEnter(&pUrbPool->CritSectPool);
        pUrb->enmState = VUSBURBSTATE_FREE;
        RTListAppend(&pUrbPool->aLstFreeUrbs[enmType], &pHdr->NdFree);
        RTCritSectLeave(&pUrbPool->CritSectPool);
    }
}

