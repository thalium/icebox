/* $Id: MMUkHeap.cpp $ */
/** @file
 * MM - Memory Manager - Ring-3 Heap with kernel accessible mapping.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_MM_HEAP
#include <VBox/vmm/mm.h>
#include <VBox/vmm/stam.h>
#include "MMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/uvm.h>
#include <VBox/err.h>
#include <VBox/param.h>
#include <VBox/log.h>

#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/heap.h>


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static void *mmR3UkHeapAlloc(PMMUKHEAP pHeap, MMTAG enmTag, size_t cb, bool fZero, PRTR0PTR pR0Ptr);



/**
 * Create a User-kernel heap.
 *
 * This does not require SUPLib to be initialized as we'll lazily allocate the
 * kernel accessible memory on the first alloc call.
 *
 * @returns VBox status code.
 * @param   pUVM    Pointer to the user mode VM structure.
 * @param   ppHeap  Where to store the heap pointer.
 */
int mmR3UkHeapCreateU(PUVM pUVM, PMMUKHEAP *ppHeap)
{
    PMMUKHEAP pHeap = (PMMUKHEAP)MMR3HeapAllocZU(pUVM, MM_TAG_MM, sizeof(MMUKHEAP));
    if (pHeap)
    {
        int rc = RTCritSectInit(&pHeap->Lock);
        if (RT_SUCCESS(rc))
        {
            /*
             * Initialize the global stat record.
             */
            pHeap->pUVM = pUVM;
#ifdef MMUKHEAP_WITH_STATISTICS
            PMMUKHEAPSTAT pStat = &pHeap->Stat;
            STAMR3RegisterU(pUVM, &pStat->cAllocations,   STAMTYPE_U64, STAMVISIBILITY_ALWAYS, "/MM/UkHeap/cAllocations",     STAMUNIT_CALLS, "Number or MMR3UkHeapAlloc() calls.");
            STAMR3RegisterU(pUVM, &pStat->cReallocations, STAMTYPE_U64, STAMVISIBILITY_ALWAYS, "/MM/UkHeap/cReallocations",   STAMUNIT_CALLS, "Number of MMR3UkHeapRealloc() calls.");
            STAMR3RegisterU(pUVM, &pStat->cFrees,         STAMTYPE_U64, STAMVISIBILITY_ALWAYS, "/MM/UkHeap/cFrees",           STAMUNIT_CALLS, "Number of MMR3UkHeapFree() calls.");
            STAMR3RegisterU(pUVM, &pStat->cFailures,      STAMTYPE_U64, STAMVISIBILITY_ALWAYS, "/MM/UkHeap/cFailures",        STAMUNIT_COUNT, "Number of failures.");
            STAMR3RegisterU(pUVM, &pStat->cbCurAllocated, sizeof(pStat->cbCurAllocated) == sizeof(uint32_t) ? STAMTYPE_U32 : STAMTYPE_U64,
                                                                        STAMVISIBILITY_ALWAYS, "/MM/UkHeap/cbCurAllocated",   STAMUNIT_BYTES, "Number of bytes currently allocated.");
            STAMR3RegisterU(pUVM, &pStat->cbAllocated,    STAMTYPE_U64, STAMVISIBILITY_ALWAYS, "/MM/UkHeap/cbAllocated",      STAMUNIT_BYTES, "Total number of bytes allocated.");
            STAMR3RegisterU(pUVM, &pStat->cbFreed,        STAMTYPE_U64, STAMVISIBILITY_ALWAYS, "/MM/UkHeap/cbFreed",          STAMUNIT_BYTES, "Total number of bytes freed.");
#endif
            *ppHeap = pHeap;
            return VINF_SUCCESS;
        }
        AssertRC(rc);
        MMR3HeapFree(pHeap);
    }
    AssertMsgFailed(("failed to allocate heap structure\n"));
    return VERR_NO_MEMORY;
}


/**
 * Destroy a User-kernel heap.
 *
 * @param   pHeap   Heap handle.
 */
void mmR3UkHeapDestroy(PMMUKHEAP pHeap)
{
    /*
     * Start by deleting the lock, that'll trap anyone
     * attempting to use the heap.
     */
    RTCritSectDelete(&pHeap->Lock);

    /*
     * Walk the sub-heaps and free them.
     */
    while (pHeap->pSubHeapHead)
    {
        PMMUKHEAPSUB pSubHeap = pHeap->pSubHeapHead;
        pHeap->pSubHeapHead = pSubHeap->pNext;
        SUPR3PageFreeEx(pSubHeap->pv, pSubHeap->cb >> PAGE_SHIFT);
        //MMR3HeapFree(pSubHeap); - rely on the automatic cleanup.
    }
    //MMR3HeapFree(pHeap->stats);
    //MMR3HeapFree(pHeap);
}


/**
 * Allocate memory associating it with the VM for collective cleanup.
 *
 * The memory will be allocated from the default heap but a header
 * is added in which we keep track of which VM it belongs to and chain
 * all the allocations together so they can be freed in one go.
 *
 * This interface is typically used for memory block which will not be
 * freed during the life of the VM.
 *
 * @returns Pointer to allocated memory.
 * @param   pVM         The cross context VM structure.
 * @param   enmTag      Statistics tag. Statistics are collected on a per tag
 *                      basis in addition to a global one. Thus we can easily
 *                      identify how memory is used by the VM.
 * @param   cbSize      Size of the block.
 * @param   pR0Ptr      Where to return the ring-0 address of the memory.
 */
VMMR3DECL(void *) MMR3UkHeapAlloc(PVM pVM, MMTAG enmTag, size_t cbSize, PRTR0PTR pR0Ptr)
{
    return mmR3UkHeapAlloc(pVM->pUVM->mm.s.pUkHeap, enmTag, cbSize, false, pR0Ptr);
}


/**
 * Same as MMR3UkHeapAlloc().
 *
 * @returns Pointer to allocated memory.
 * @param   pVM         The cross context VM structure.
 * @param   enmTag      Statistics tag. Statistics are collected on a per tag
 *                      basis in addition to a global one. Thus we can easily
 *                      identify how memory is used by the VM.
 * @param   cbSize      Size of the block.
 * @param   ppv         Where to store the pointer to the allocated memory on success.
 * @param   pR0Ptr      Where to return the ring-0 address of the memory.
 */
VMMR3DECL(int) MMR3UkHeapAllocEx(PVM pVM, MMTAG enmTag, size_t cbSize, void **ppv, PRTR0PTR pR0Ptr)
{
    void *pv = mmR3UkHeapAlloc(pVM->pUVM->mm.s.pUkHeap, enmTag, cbSize, false, pR0Ptr);
    if (pv)
    {
        *ppv = pv;
        return VINF_SUCCESS;
    }
    return VERR_NO_MEMORY;
}


/**
 * Same as MMR3UkHeapAlloc() only the memory is zeroed.
 *
 * @returns Pointer to allocated memory.
 * @param   pVM         The cross context VM structure.
 * @param   enmTag      Statistics tag. Statistics are collected on a per tag
 *                      basis in addition to a global one. Thus we can easily
 *                      identify how memory is used by the VM.
 * @param   cbSize      Size of the block.
 * @param   pR0Ptr      Where to return the ring-0 address of the memory.
 */
VMMR3DECL(void *) MMR3UkHeapAllocZ(PVM pVM, MMTAG enmTag, size_t cbSize, PRTR0PTR pR0Ptr)
{
    return mmR3UkHeapAlloc(pVM->pUVM->mm.s.pUkHeap, enmTag, cbSize, true, pR0Ptr);
}


/**
 * Same as MMR3UkHeapAllocZ().
 *
 * @returns Pointer to allocated memory.
 * @param   pVM         The cross context VM structure.
 * @param   enmTag      Statistics tag. Statistics are collected on a per tag
 *                      basis in addition to a global one. Thus we can easily
 *                      identify how memory is used by the VM.
 * @param   cbSize      Size of the block.
 * @param   ppv         Where to store the pointer to the allocated memory on success.
 * @param   pR0Ptr      Where to return the ring-0 address of the memory.
 */
VMMR3DECL(int) MMR3UkHeapAllocZEx(PVM pVM, MMTAG enmTag, size_t cbSize, void **ppv, PRTR0PTR pR0Ptr)
{
    void *pv = mmR3UkHeapAlloc(pVM->pUVM->mm.s.pUkHeap, enmTag, cbSize, true, pR0Ptr);
    if (pv)
    {
        *ppv = pv;
        return VINF_SUCCESS;
    }
    return VERR_NO_MEMORY;
}


/***
 * Worker for mmR3UkHeapAlloc that creates and adds a new sub-heap.
 *
 * @returns Pointer to the new sub-heap.
 * @param   pHeap       The heap
 * @param   cbSubHeap   The size of the sub-heap.
 */
static PMMUKHEAPSUB mmR3UkHeapAddSubHeap(PMMUKHEAP pHeap, size_t cbSubHeap)
{
    PMMUKHEAPSUB pSubHeap = (PMMUKHEAPSUB)MMR3HeapAllocU(pHeap->pUVM, MM_TAG_MM/*_UK_HEAP*/, sizeof(*pSubHeap));
    if (pSubHeap)
    {
        pSubHeap->cb = cbSubHeap;
        int rc = SUPR3PageAllocEx(pSubHeap->cb >> PAGE_SHIFT, 0, &pSubHeap->pv, &pSubHeap->pvR0, NULL);
        if (RT_SUCCESS(rc))
        {
            rc = RTHeapSimpleInit(&pSubHeap->hSimple, pSubHeap->pv, pSubHeap->cb);
            if (RT_SUCCESS(rc))
            {
                pSubHeap->pNext = pHeap->pSubHeapHead;
                pHeap->pSubHeapHead = pSubHeap;
                return pSubHeap;
            }

            /* bail out */
            SUPR3PageFreeEx(pSubHeap->pv, pSubHeap->cb >> PAGE_SHIFT);
        }
        MMR3HeapFree(pSubHeap);
    }
    return NULL;
}


/**
 * Allocate memory from the heap.
 *
 * @returns Pointer to allocated memory.
 * @param   pHeap       Heap handle.
 * @param   enmTag      Statistics tag. Statistics are collected on a per tag
 *                      basis in addition to a global one. Thus we can easily
 *                      identify how memory is used by the VM.
 * @param   cb          Size of the block.
 * @param   fZero       Whether or not to zero the memory block.
 * @param   pR0Ptr      Where to return the ring-0 pointer.
 */
static void *mmR3UkHeapAlloc(PMMUKHEAP pHeap, MMTAG enmTag, size_t cb, bool fZero, PRTR0PTR pR0Ptr)
{
    if (pR0Ptr)
        *pR0Ptr = NIL_RTR0PTR;
    RTCritSectEnter(&pHeap->Lock);

#ifdef MMUKHEAP_WITH_STATISTICS
    /*
     * Find/alloc statistics nodes.
     */
    pHeap->Stat.cAllocations++;
    PMMUKHEAPSTAT pStat = (PMMUKHEAPSTAT)RTAvlULGet(&pHeap->pStatTree, (AVLULKEY)enmTag);
    if (pStat)
        pStat->cAllocations++;
    else
    {
        pStat = (PMMUKHEAPSTAT)MMR3HeapAllocZU(pHeap->pUVM, MM_TAG_MM, sizeof(MMUKHEAPSTAT));
        if (!pStat)
        {
            pHeap->Stat.cFailures++;
            AssertMsgFailed(("Failed to allocate heap stat record.\n"));
            RTCritSectLeave(&pHeap->Lock);
            return NULL;
        }
        pStat->Core.Key = (AVLULKEY)enmTag;
        RTAvlULInsert(&pHeap->pStatTree, &pStat->Core);

        pStat->cAllocations++;

        /* register the statistics */
        PUVM pUVM = pHeap->pUVM;
        const char *pszTag = mmGetTagName(enmTag);
        STAMR3RegisterFU(pUVM, &pStat->cbCurAllocated, STAMTYPE_U32, STAMVISIBILITY_ALWAYS,  STAMUNIT_BYTES, "Number of bytes currently allocated.",    "/MM/UkHeap/%s", pszTag);
        STAMR3RegisterFU(pUVM, &pStat->cAllocations,   STAMTYPE_U64, STAMVISIBILITY_ALWAYS,  STAMUNIT_CALLS, "Number or MMR3UkHeapAlloc() calls.",      "/MM/UkHeap/%s/cAllocations", pszTag);
        STAMR3RegisterFU(pUVM, &pStat->cReallocations, STAMTYPE_U64, STAMVISIBILITY_ALWAYS,  STAMUNIT_CALLS, "Number of MMR3UkHeapRealloc() calls.",    "/MM/UkHeap/%s/cReallocations", pszTag);
        STAMR3RegisterFU(pUVM, &pStat->cFrees,         STAMTYPE_U64, STAMVISIBILITY_ALWAYS,  STAMUNIT_CALLS, "Number of MMR3UkHeapFree() calls.",       "/MM/UkHeap/%s/cFrees", pszTag);
        STAMR3RegisterFU(pUVM, &pStat->cFailures,      STAMTYPE_U64, STAMVISIBILITY_ALWAYS,  STAMUNIT_COUNT, "Number of failures.",                     "/MM/UkHeap/%s/cFailures", pszTag);
        STAMR3RegisterFU(pUVM, &pStat->cbAllocated,    STAMTYPE_U64, STAMVISIBILITY_ALWAYS,  STAMUNIT_BYTES, "Total number of bytes allocated.",        "/MM/UkHeap/%s/cbAllocated", pszTag);
        STAMR3RegisterFU(pUVM, &pStat->cbFreed,        STAMTYPE_U64, STAMVISIBILITY_ALWAYS,  STAMUNIT_BYTES, "Total number of bytes freed.",            "/MM/UkHeap/%s/cbFreed", pszTag);
    }
#else
    RT_NOREF_PV(enmTag);
#endif

    /*
     * Validate input.
     */
    if (cb == 0)
    {
#ifdef MMUKHEAP_WITH_STATISTICS
        pStat->cFailures++;
        pHeap->Stat.cFailures++;
#endif
        RTCritSectLeave(&pHeap->Lock);
        return NULL;
    }

    /*
     * Allocate heap block.
     */
    cb = RT_ALIGN_Z(cb, MMUKHEAP_SIZE_ALIGNMENT);
    void *pv = NULL;
    PMMUKHEAPSUB pSubHeapPrev = NULL;
    PMMUKHEAPSUB pSubHeap = pHeap->pSubHeapHead;
    while (pSubHeap)
    {
        if (fZero)
            pv = RTHeapSimpleAllocZ(pSubHeap->hSimple, cb, MMUKHEAP_SIZE_ALIGNMENT);
        else
            pv = RTHeapSimpleAlloc(pSubHeap->hSimple, cb, MMUKHEAP_SIZE_ALIGNMENT);
        if (pv)
        {
            /* Move the sub-heap with free memory to the head. */
            if (pSubHeapPrev)
            {
                pSubHeapPrev->pNext = pSubHeap->pNext;
                pSubHeap->pNext = pHeap->pSubHeapHead;
                pHeap->pSubHeapHead = pSubHeap;
            }
            break;
        }
        pSubHeapPrev = pSubHeap;
        pSubHeap = pSubHeap->pNext;
    }
    if (RT_UNLIKELY(!pv))
    {
        /*
         * Add another sub-heap.
         */
        pSubHeap = mmR3UkHeapAddSubHeap(pHeap, RT_MAX(RT_ALIGN_Z(cb, PAGE_SIZE) + PAGE_SIZE * 16, _256K));
        if (pSubHeap)
        {
            if (fZero)
                pv = RTHeapSimpleAllocZ(pSubHeap->hSimple, cb, MMUKHEAP_SIZE_ALIGNMENT);
            else
                pv = RTHeapSimpleAlloc(pSubHeap->hSimple, cb, MMUKHEAP_SIZE_ALIGNMENT);
        }
        if (RT_UNLIKELY(!pv))
        {
            AssertMsgFailed(("Failed to allocate heap block %d, enmTag=%x(%.4s).\n", cb, enmTag, &enmTag));
#ifdef MMUKHEAP_WITH_STATISTICS
            pStat->cFailures++;
            pHeap->Stat.cFailures++;
#endif
            RTCritSectLeave(&pHeap->Lock);
            return NULL;
        }
    }

    /*
     * Update statistics
     */
#ifdef MMUKHEAP_WITH_STATISTICS
    size_t cbActual = RTHeapSimpleSize(pSubHeap->hSimple, pv);
    pStat->cbAllocated          += cbActual;
    pStat->cbCurAllocated       += cbActual;
    pHeap->Stat.cbAllocated     += cbActual;
    pHeap->Stat.cbCurAllocated  += cbActual;
#endif

    if (pR0Ptr)
        *pR0Ptr = (uintptr_t)pv - (uintptr_t)pSubHeap->pv + pSubHeap->pvR0;
    RTCritSectLeave(&pHeap->Lock);
    return pv;
}


/**
 * Releases memory allocated with MMR3UkHeapAlloc() and MMR3UkHeapAllocZ()
 *
 * @param   pVM         The cross context VM structure.
 * @param   pv          Pointer to the memory block to free.
 * @param   enmTag      The allocation accounting tag.
 */
VMMR3DECL(void) MMR3UkHeapFree(PVM pVM, void *pv, MMTAG enmTag)
{
    /* Ignore NULL pointers. */
    if (!pv)
        return;

    PMMUKHEAP pHeap = pVM->pUVM->mm.s.pUkHeap;
    RTCritSectEnter(&pHeap->Lock);

    /*
     * Find the sub-heap and block
     */
#ifdef MMUKHEAP_WITH_STATISTICS
    size_t cbActual = 0;
#endif
    PMMUKHEAPSUB pSubHeap = pHeap->pSubHeapHead;
    while (pSubHeap)
    {
        if ((uintptr_t)pv - (uintptr_t)pSubHeap->pv < pSubHeap->cb)
        {
#ifdef MMUKHEAP_WITH_STATISTICS
            cbActual = RTHeapSimpleSize(pSubHeap->hSimple, pv);
            PMMUKHEAPSTAT pStat = (PMMUKHEAPSTAT)RTAvlULGet(&pHeap->pStatTree, (AVLULKEY)enmTag);
            if (pStat)
            {
                pStat->cFrees++;
                pStat->cbCurAllocated     -= cbActual;
                pStat->cbFreed            += cbActual;
            }
            pHeap->Stat.cFrees++;
            pHeap->Stat.cbFreed           += cbActual;
            pHeap->Stat.cbCurAllocated    -= cbActual;
#else
            RT_NOREF_PV(enmTag);
#endif
            RTHeapSimpleFree(pSubHeap->hSimple, pv);

            RTCritSectLeave(&pHeap->Lock);
            return;
        }
    }
    AssertMsgFailed(("pv=%p\n", pv));
}

