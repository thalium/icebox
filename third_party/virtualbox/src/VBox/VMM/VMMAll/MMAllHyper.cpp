/* $Id: MMAllHyper.cpp $ */
/** @file
 * MM - Memory Manager - Hypervisor Memory Area, All Contexts.
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
#define LOG_GROUP LOG_GROUP_MM_HYPER_HEAP
#include <VBox/vmm/mm.h>
#include <VBox/vmm/stam.h>
#include "MMInternal.h"
#include <VBox/vmm/vm.h>

#include <VBox/err.h>
#include <VBox/param.h>
#include <iprt/assert.h>
#include <VBox/log.h>
#include <iprt/asm.h>
#include <iprt/string.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#define ASSERT_L(u1, u2)    AssertMsg((u1) <  (u2), ("u1=%#x u2=%#x\n", u1, u2))
#define ASSERT_LE(u1, u2)   AssertMsg((u1) <= (u2), ("u1=%#x u2=%#x\n", u1, u2))
#define ASSERT_GE(u1, u2)   AssertMsg((u1) >= (u2), ("u1=%#x u2=%#x\n", u1, u2))
#define ASSERT_ALIGN(u1)    AssertMsg(!((u1) & (MMHYPER_HEAP_ALIGN_MIN - 1)), ("u1=%#x (%d)\n", u1, u1))

#define ASSERT_OFFPREV(pHeap, pChunk)  \
    do { Assert(MMHYPERCHUNK_GET_OFFPREV(pChunk) <= 0); \
         Assert(MMHYPERCHUNK_GET_OFFPREV(pChunk) >= (intptr_t)(pHeap)->CTX_SUFF(pbHeap) - (intptr_t)(pChunk)); \
         AssertMsg(     MMHYPERCHUNK_GET_OFFPREV(pChunk) != 0 \
                   ||   (uint8_t *)(pChunk) == (pHeap)->CTX_SUFF(pbHeap), \
                   ("pChunk=%p pvHyperHeap=%p\n", (pChunk), (pHeap)->CTX_SUFF(pbHeap))); \
    } while (0)

#define ASSERT_OFFNEXT(pHeap, pChunk) \
    do { ASSERT_ALIGN((pChunk)->offNext); \
         ASSERT_L((pChunk)->offNext, (uintptr_t)(pHeap)->CTX_SUFF(pbHeap) + (pHeap)->offPageAligned - (uintptr_t)(pChunk)); \
    } while (0)

#define ASSERT_OFFHEAP(pHeap, pChunk) \
    do { Assert((pChunk)->offHeap); \
         AssertMsg((PMMHYPERHEAP)((pChunk)->offHeap + (uintptr_t)pChunk) == (pHeap), \
                   ("offHeap=%RX32 pChunk=%p pHeap=%p\n", (pChunk)->offHeap, (pChunk), (pHeap))); \
         Assert((pHeap)->u32Magic == MMHYPERHEAP_MAGIC); \
    } while (0)

#ifdef VBOX_WITH_STATISTICS
#define ASSERT_OFFSTAT(pHeap, pChunk) \
    do { if (MMHYPERCHUNK_ISFREE(pChunk)) \
             Assert(!(pChunk)->offStat); \
         else if ((pChunk)->offStat) \
         { \
             Assert((pChunk)->offStat); \
             AssertMsg(!((pChunk)->offStat & (MMHYPER_HEAP_ALIGN_MIN - 1)), ("offStat=%RX32\n", (pChunk)->offStat)); \
             uintptr_t uPtr = (uintptr_t)(pChunk)->offStat + (uintptr_t)pChunk; NOREF(uPtr); \
             AssertMsg(uPtr - (uintptr_t)(pHeap)->CTX_SUFF(pbHeap) < (pHeap)->offPageAligned, \
                       ("%p - %p < %RX32\n", uPtr, (pHeap)->CTX_SUFF(pbHeap), (pHeap)->offPageAligned)); \
         } \
    } while (0)
#else
#define ASSERT_OFFSTAT(pHeap, pChunk) \
    do { Assert(!(pChunk)->offStat); \
    } while (0)
#endif

#define ASSERT_CHUNK(pHeap, pChunk) \
    do { ASSERT_OFFNEXT(pHeap, pChunk); \
         ASSERT_OFFPREV(pHeap, pChunk); \
         ASSERT_OFFHEAP(pHeap, pChunk); \
         ASSERT_OFFSTAT(pHeap, pChunk); \
    } while (0)
#define ASSERT_CHUNK_USED(pHeap, pChunk) \
    do { ASSERT_OFFNEXT(pHeap, pChunk); \
         ASSERT_OFFPREV(pHeap, pChunk); \
         Assert(MMHYPERCHUNK_ISUSED(pChunk)); \
    } while (0)

#define ASSERT_FREE_OFFPREV(pHeap, pChunk)  \
    do { ASSERT_ALIGN((pChunk)->offPrev); \
         ASSERT_GE(((pChunk)->offPrev  & (MMHYPER_HEAP_ALIGN_MIN - 1)), (intptr_t)(pHeap)->CTX_SUFF(pbHeap) - (intptr_t)(pChunk)); \
         Assert((pChunk)->offPrev != MMHYPERCHUNK_GET_OFFPREV(&(pChunk)->core) || !(pChunk)->offPrev); \
         AssertMsg(    (pChunk)->offPrev \
                  ||   (uintptr_t)(pChunk) - (uintptr_t)(pHeap)->CTX_SUFF(pbHeap) == (pHeap)->offFreeHead, \
                  ("pChunk=%p offChunk=%#x offFreeHead=%#x\n", (pChunk), (uintptr_t)(pChunk) - (uintptr_t)(pHeap)->CTX_SUFF(pbHeap),\
                  (pHeap)->offFreeHead)); \
    } while (0)

#define ASSERT_FREE_OFFNEXT(pHeap, pChunk) \
    do { ASSERT_ALIGN((pChunk)->offNext); \
         ASSERT_L((pChunk)->offNext, (uintptr_t)(pHeap)->CTX_SUFF(pbHeap) + (pHeap)->offPageAligned - (uintptr_t)(pChunk)); \
         Assert((pChunk)->offNext != (pChunk)->core.offNext || !(pChunk)->offNext); \
         AssertMsg(     (pChunk)->offNext \
                   ||   (uintptr_t)(pChunk) - (uintptr_t)(pHeap)->CTX_SUFF(pbHeap) == (pHeap)->offFreeTail, \
                   ("pChunk=%p offChunk=%#x offFreeTail=%#x\n", (pChunk),  (uintptr_t)(pChunk) - (uintptr_t)(pHeap)->CTX_SUFF(pbHeap), \
                    (pHeap)->offFreeTail)); \
    } while (0)

#define ASSERT_FREE_CB(pHeap, pChunk) \
    do { ASSERT_ALIGN((pChunk)->cb); \
         Assert((pChunk)->cb > 0); \
         if ((pChunk)->core.offNext) \
             AssertMsg((pChunk)->cb == ((pChunk)->core.offNext - sizeof(MMHYPERCHUNK)), \
                       ("cb=%d offNext=%d\n", (pChunk)->cb, (pChunk)->core.offNext)); \
         else \
             ASSERT_LE((pChunk)->cb, (uintptr_t)(pHeap)->CTX_SUFF(pbHeap) + (pHeap)->offPageAligned - (uintptr_t)(pChunk)); \
    } while (0)

#define ASSERT_CHUNK_FREE(pHeap, pChunk) \
    do { ASSERT_CHUNK(pHeap, &(pChunk)->core); \
         Assert(MMHYPERCHUNK_ISFREE(pChunk)); \
         ASSERT_FREE_OFFNEXT(pHeap, pChunk); \
         ASSERT_FREE_OFFPREV(pHeap, pChunk); \
         ASSERT_FREE_CB(pHeap, pChunk); \
    } while (0)


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static PMMHYPERCHUNK mmHyperAllocChunk(PMMHYPERHEAP pHeap, uint32_t cb, unsigned uAlignment);
static void *mmHyperAllocPages(PMMHYPERHEAP pHeap, uint32_t cb);
#ifdef VBOX_WITH_STATISTICS
static PMMHYPERSTAT mmHyperStat(PMMHYPERHEAP pHeap, MMTAG enmTag);
#ifdef IN_RING3
static void mmR3HyperStatRegisterOne(PVM pVM, PMMHYPERSTAT pStat);
#endif
#endif
static int mmHyperFree(PMMHYPERHEAP pHeap, PMMHYPERCHUNK pChunk);
#ifdef MMHYPER_HEAP_STRICT
static void mmHyperHeapCheck(PMMHYPERHEAP pHeap);
#endif



/**
 * Locks the hypervisor heap.
 * This might call back to Ring-3 in order to deal with lock contention in GC and R3.
 *
 * @param   pVM     The cross context VM structure.
 */
static int mmHyperLock(PVM pVM)
{
    PMMHYPERHEAP pHeap = pVM->mm.s.CTX_SUFF(pHyperHeap);

#ifdef IN_RING3
    if (!PDMCritSectIsInitialized(&pHeap->Lock))
        return VINF_SUCCESS;     /* early init */
#else
    Assert(PDMCritSectIsInitialized(&pHeap->Lock));
#endif
    int rc = PDMCritSectEnter(&pHeap->Lock, VERR_SEM_BUSY);
#if defined(IN_RC) || defined(IN_RING0)
    if (rc == VERR_SEM_BUSY)
        rc = VMMRZCallRing3NoCpu(pVM, VMMCALLRING3_MMHYPER_LOCK, 0);
#endif
    AssertRC(rc);
    return rc;
}


/**
 * Unlocks the hypervisor heap.
 *
 * @param   pVM     The cross context VM structure.
 */
static void mmHyperUnlock(PVM pVM)
{
    PMMHYPERHEAP pHeap = pVM->mm.s.CTX_SUFF(pHyperHeap);

#ifdef IN_RING3
    if (!PDMCritSectIsInitialized(&pHeap->Lock))
        return;     /* early init */
#endif
    Assert(PDMCritSectIsInitialized(&pHeap->Lock));
    PDMCritSectLeave(&pHeap->Lock);
}

/**
 * Allocates memory in the Hypervisor (RC VMM) area.
 * The returned memory is of course zeroed.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   cb          Number of bytes to allocate.
 * @param   uAlignment  Required memory alignment in bytes.
 *                      Values are 0,8,16,32,64 and PAGE_SIZE.
 *                      0 -> default alignment, i.e. 8 bytes.
 * @param   enmTag      The statistics tag.
 * @param   ppv         Where to store the address to the allocated
 *                      memory.
 */
static int mmHyperAllocInternal(PVM pVM, size_t cb, unsigned uAlignment, MMTAG enmTag, void **ppv)
{
    AssertMsg(cb >= 8, ("Hey! Do you really mean to allocate less than 8 bytes?! cb=%d\n", cb));

    /*
     * Validate input and adjust it to reasonable values.
     */
    if (!uAlignment || uAlignment < MMHYPER_HEAP_ALIGN_MIN)
        uAlignment = MMHYPER_HEAP_ALIGN_MIN;
    uint32_t cbAligned;
    switch (uAlignment)
    {
        case 8:
        case 16:
        case 32:
        case 64:
            cbAligned = RT_ALIGN_32(cb, MMHYPER_HEAP_ALIGN_MIN);
            if (!cbAligned || cbAligned < cb)
            {
                Log2(("MMHyperAlloc: cb=%#x uAlignment=%#x returns VERR_INVALID_PARAMETER\n", cb, uAlignment));
                AssertMsgFailed(("Nice try.\n"));
                return VERR_INVALID_PARAMETER;
            }
            break;

        case PAGE_SIZE:
            AssertMsg(RT_ALIGN_32(cb, PAGE_SIZE) == cb, ("The size isn't page aligned. (cb=%#x)\n", cb));
            cbAligned = RT_ALIGN_32(cb, PAGE_SIZE);
            if (!cbAligned)
            {
                Log2(("MMHyperAlloc: cb=%#x uAlignment=%#x returns VERR_INVALID_PARAMETER\n", cb, uAlignment));
                AssertMsgFailed(("Nice try.\n"));
                return VERR_INVALID_PARAMETER;
            }
            break;

        default:
            Log2(("MMHyperAlloc: cb=%#x uAlignment=%#x returns VERR_INVALID_PARAMETER\n", cb, uAlignment));
            AssertMsgFailed(("Invalid alignment %u\n", uAlignment));
            return VERR_INVALID_PARAMETER;
    }


    /*
     * Get heap and statisticsStatistics.
     */
    PMMHYPERHEAP pHeap = pVM->mm.s.CTX_SUFF(pHyperHeap);
#ifdef VBOX_WITH_STATISTICS
    PMMHYPERSTAT pStat = mmHyperStat(pHeap, enmTag);
    if (!pStat)
    {
        Log2(("MMHyperAlloc: cb=%#x uAlignment=%#x returns VERR_MM_HYPER_NO_MEMORY\n", cb, uAlignment));
        AssertMsgFailed(("Failed to allocate statistics!\n"));
        return VERR_MM_HYPER_NO_MEMORY;
    }
#else
    NOREF(enmTag);
#endif
    if (uAlignment < PAGE_SIZE)
    {
        /*
         * Allocate a chunk.
         */
        PMMHYPERCHUNK pChunk = mmHyperAllocChunk(pHeap, cbAligned, uAlignment);
        if (pChunk)
        {
#ifdef VBOX_WITH_STATISTICS
            const uint32_t cbChunk = pChunk->offNext
                ? pChunk->offNext
                : pHeap->CTX_SUFF(pbHeap) + pHeap->offPageAligned - (uint8_t *)pChunk;
            pStat->cbAllocated += (uint32_t)cbChunk;
            pStat->cbCurAllocated += (uint32_t)cbChunk;
            if (pStat->cbCurAllocated > pStat->cbMaxAllocated)
                pStat->cbMaxAllocated = pStat->cbCurAllocated;
            pStat->cAllocations++;
            pChunk->offStat = (uintptr_t)pStat - (uintptr_t)pChunk;
#else
            pChunk->offStat = 0;
#endif
            void *pv = pChunk + 1;
            *ppv = pv;
            ASMMemZero32(pv, cbAligned);
            Log2(("MMHyperAlloc: cb=%#x uAlignment=%#x returns VINF_SUCCESS and *ppv=%p\n", cb, uAlignment, pv));
            return VINF_SUCCESS;
        }
    }
    else
    {
        /*
         * Allocate page aligned memory.
         */
        void *pv = mmHyperAllocPages(pHeap, cbAligned);
        if (pv)
        {
#ifdef VBOX_WITH_STATISTICS
            pStat->cbAllocated += cbAligned;
            pStat->cbCurAllocated += cbAligned;
            if (pStat->cbCurAllocated > pStat->cbMaxAllocated)
                pStat->cbMaxAllocated = pStat->cbCurAllocated;
            pStat->cAllocations++;
#endif
            *ppv = pv;
            /* ASMMemZero32(pv, cbAligned); - not required since memory is alloc-only and SUPR3PageAlloc zeros it. */
            Log2(("MMHyperAlloc: cb=%#x uAlignment=%#x returns VINF_SUCCESS and *ppv=%p\n", cb, uAlignment, ppv));
            return VINF_SUCCESS;
        }
    }

#ifdef VBOX_WITH_STATISTICS
    pStat->cAllocations++;
    pStat->cFailures++;
#endif
    Log2(("MMHyperAlloc: cb=%#x uAlignment=%#x returns VERR_MM_HYPER_NO_MEMORY\n", cb, uAlignment));
    AssertMsgFailed(("Failed to allocate %d bytes!\n", cb));
    return VERR_MM_HYPER_NO_MEMORY;
}


/**
 * Wrapper for mmHyperAllocInternal
 */
VMMDECL(int) MMHyperAlloc(PVM pVM, size_t cb, unsigned uAlignment, MMTAG enmTag, void **ppv)
{
    int rc = mmHyperLock(pVM);
    AssertRCReturn(rc, rc);

    LogFlow(("MMHyperAlloc %x align=%x tag=%s\n", cb, uAlignment, mmGetTagName(enmTag)));

    rc = mmHyperAllocInternal(pVM, cb, uAlignment, enmTag, ppv);

    mmHyperUnlock(pVM);
    return rc;
}


/**
 * Duplicates a block of memory.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pvSrc       The source memory block to copy from.
 * @param   cb          Size of the source memory block.
 * @param   uAlignment  Required memory alignment in bytes.
 *                      Values are 0,8,16,32,64 and PAGE_SIZE.
 *                      0 -> default alignment, i.e. 8 bytes.
 * @param   enmTag      The statistics tag.
 * @param   ppv         Where to store the address to the allocated
 *                      memory.
 */
VMMDECL(int) MMHyperDupMem(PVM pVM, const void *pvSrc, size_t cb, unsigned uAlignment, MMTAG enmTag, void **ppv)
{
    int rc = MMHyperAlloc(pVM, cb, uAlignment, enmTag, ppv);
    if (RT_SUCCESS(rc))
        memcpy(*ppv, pvSrc, cb);
    return rc;
}


/**
 * Allocates a chunk of memory from the specified heap.
 * The caller validates the parameters of this request.
 *
 * @returns Pointer to the allocated chunk.
 * @returns NULL on failure.
 * @param   pHeap       The heap.
 * @param   cb          Size of the memory block to allocate.
 * @param   uAlignment  The alignment specifications for the allocated block.
 * @internal
 */
static PMMHYPERCHUNK mmHyperAllocChunk(PMMHYPERHEAP pHeap, uint32_t cb, unsigned uAlignment)
{
    Log3(("mmHyperAllocChunk: Enter cb=%#x uAlignment=%#x\n", cb, uAlignment));
#ifdef MMHYPER_HEAP_STRICT
    mmHyperHeapCheck(pHeap);
#endif
#ifdef MMHYPER_HEAP_STRICT_FENCE
    uint32_t cbFence = RT_MAX(MMHYPER_HEAP_STRICT_FENCE_SIZE, uAlignment);
    cb += cbFence;
#endif

    /*
     * Check if there are any free chunks. (NIL_OFFSET use/not-use forces this check)
     */
    if (pHeap->offFreeHead == NIL_OFFSET)
        return NULL;

    /*
     * Small alignments - from the front of the heap.
     *
     * Must split off free chunks at the end to prevent messing up the
     * last free node which we take the page aligned memory from the top of.
     */
    PMMHYPERCHUNK     pRet = NULL;
    PMMHYPERCHUNKFREE pFree = (PMMHYPERCHUNKFREE)((char *)pHeap->CTX_SUFF(pbHeap) + pHeap->offFreeHead);
    while (pFree)
    {
        ASSERT_CHUNK_FREE(pHeap, pFree);
        if (pFree->cb >= cb)
        {
            unsigned offAlign = (uintptr_t)(&pFree->core + 1) & (uAlignment - 1);
            if (offAlign)
                offAlign = uAlignment - offAlign;
            if (!offAlign || pFree->cb - offAlign >= cb)
            {
                Log3(("mmHyperAllocChunk: Using pFree=%p pFree->cb=%d offAlign=%d\n", pFree, pFree->cb, offAlign));

                /*
                 * Adjust the node in front.
                 * Because of multiple alignments we need to special case allocation of the first block.
                 */
                if (offAlign)
                {
                    MMHYPERCHUNKFREE Free = *pFree;
                    if (MMHYPERCHUNK_GET_OFFPREV(&pFree->core))
                    {
                        /* just add a bit of memory to it. */
                        PMMHYPERCHUNKFREE pPrev = (PMMHYPERCHUNKFREE)((char *)pFree + MMHYPERCHUNK_GET_OFFPREV(&Free.core));
                        pPrev->core.offNext += offAlign;
                        AssertMsg(!MMHYPERCHUNK_ISFREE(&pPrev->core), ("Impossible!\n"));
                        Log3(("mmHyperAllocChunk: Added %d bytes to %p\n", offAlign, pPrev));
                    }
                    else
                    {
                        /* make new head node, mark it USED for simplicity. */
                        PMMHYPERCHUNK pPrev = (PMMHYPERCHUNK)pHeap->CTX_SUFF(pbHeap);
                        Assert(pPrev == &pFree->core);
                        pPrev->offPrev = 0;
                        MMHYPERCHUNK_SET_TYPE(pPrev, MMHYPERCHUNK_FLAGS_USED);
                        pPrev->offNext = offAlign;
                        Log3(("mmHyperAllocChunk: Created new first node of %d bytes\n", offAlign));

                    }
                    Log3(("mmHyperAllocChunk: cbFree %d -> %d (%d)\n", pHeap->cbFree, pHeap->cbFree - offAlign, -(int)offAlign));
                    pHeap->cbFree -= offAlign;

                    /* Recreate pFree node and adjusting everything... */
                    pFree = (PMMHYPERCHUNKFREE)((char *)pFree + offAlign);
                    *pFree = Free;

                    pFree->cb -= offAlign;
                    if (pFree->core.offNext)
                    {
                        pFree->core.offNext -= offAlign;
                        PMMHYPERCHUNK pNext = (PMMHYPERCHUNK)((char *)pFree + pFree->core.offNext);
                        MMHYPERCHUNK_SET_OFFPREV(pNext, -(int32_t)pFree->core.offNext);
                        ASSERT_CHUNK(pHeap, pNext);
                    }
                    if (MMHYPERCHUNK_GET_OFFPREV(&pFree->core))
                        MMHYPERCHUNK_SET_OFFPREV(&pFree->core, MMHYPERCHUNK_GET_OFFPREV(&pFree->core) - offAlign);

                    if (pFree->offNext)
                    {
                        pFree->offNext -= offAlign;
                        PMMHYPERCHUNKFREE pNext = (PMMHYPERCHUNKFREE)((char *)pFree + pFree->offNext);
                        pNext->offPrev = -(int32_t)pFree->offNext;
                        ASSERT_CHUNK_FREE(pHeap, pNext);
                    }
                    else
                        pHeap->offFreeTail += offAlign;
                    if (pFree->offPrev)
                    {
                        pFree->offPrev -= offAlign;
                        PMMHYPERCHUNKFREE pPrev = (PMMHYPERCHUNKFREE)((char *)pFree + pFree->offPrev);
                        pPrev->offNext = -pFree->offPrev;
                        ASSERT_CHUNK_FREE(pHeap, pPrev);
                    }
                    else
                        pHeap->offFreeHead += offAlign;
                    pFree->core.offHeap = (uintptr_t)pHeap - (uintptr_t)pFree;
                    pFree->core.offStat = 0;
                    ASSERT_CHUNK_FREE(pHeap, pFree);
                    Log3(("mmHyperAllocChunk: Realigned pFree=%p\n", pFree));
                }

                /*
                 * Split off a new FREE chunk?
                 */
                if (pFree->cb >= cb + RT_ALIGN(sizeof(MMHYPERCHUNKFREE), MMHYPER_HEAP_ALIGN_MIN))
                {
                    /*
                     * Move the FREE chunk up to make room for the new USED chunk.
                     */
                    const int           off = cb + sizeof(MMHYPERCHUNK);
                    PMMHYPERCHUNKFREE   pNew = (PMMHYPERCHUNKFREE)((char *)&pFree->core + off);
                    *pNew = *pFree;
                    pNew->cb -= off;
                    if (pNew->core.offNext)
                    {
                        pNew->core.offNext -= off;
                        PMMHYPERCHUNK pNext = (PMMHYPERCHUNK)((char *)pNew + pNew->core.offNext);
                        MMHYPERCHUNK_SET_OFFPREV(pNext, -(int32_t)pNew->core.offNext);
                        ASSERT_CHUNK(pHeap, pNext);
                    }
                    pNew->core.offPrev  = -off;
                    MMHYPERCHUNK_SET_TYPE(pNew, MMHYPERCHUNK_FLAGS_FREE);

                    if (pNew->offNext)
                    {
                        pNew->offNext -= off;
                        PMMHYPERCHUNKFREE pNext = (PMMHYPERCHUNKFREE)((char *)pNew + pNew->offNext);
                        pNext->offPrev = -(int32_t)pNew->offNext;
                        ASSERT_CHUNK_FREE(pHeap, pNext);
                    }
                    else
                        pHeap->offFreeTail += off;
                    if (pNew->offPrev)
                    {
                        pNew->offPrev -= off;
                        PMMHYPERCHUNKFREE pPrev = (PMMHYPERCHUNKFREE)((char *)pNew + pNew->offPrev);
                        pPrev->offNext = -pNew->offPrev;
                        ASSERT_CHUNK_FREE(pHeap, pPrev);
                    }
                    else
                        pHeap->offFreeHead += off;
                    pNew->core.offHeap = (uintptr_t)pHeap - (uintptr_t)pNew;
                    pNew->core.offStat = 0;
                    ASSERT_CHUNK_FREE(pHeap, pNew);

                    /*
                     * Update the old FREE node making it a USED node.
                     */
                    pFree->core.offNext = off;
                    MMHYPERCHUNK_SET_TYPE(&pFree->core, MMHYPERCHUNK_FLAGS_USED);


                    Log3(("mmHyperAllocChunk: cbFree %d -> %d (%d)\n", pHeap->cbFree,
                          pHeap->cbFree - (cb + sizeof(MMHYPERCHUNK)), -(int)(cb + sizeof(MMHYPERCHUNK))));
                    pHeap->cbFree -= (uint32_t)(cb + sizeof(MMHYPERCHUNK));
                    pRet = &pFree->core;
                    ASSERT_CHUNK(pHeap, &pFree->core);
                    Log3(("mmHyperAllocChunk: Created free chunk pNew=%p cb=%d\n", pNew, pNew->cb));
                }
                else
                {
                    /*
                     * Link out of free list.
                     */
                    if (pFree->offNext)
                    {
                        PMMHYPERCHUNKFREE pNext = (PMMHYPERCHUNKFREE)((char *)pFree + pFree->offNext);
                        if (pFree->offPrev)
                        {
                            pNext->offPrev += pFree->offPrev;
                            PMMHYPERCHUNKFREE pPrev = (PMMHYPERCHUNKFREE)((char *)pFree + pFree->offPrev);
                            pPrev->offNext += pFree->offNext;
                            ASSERT_CHUNK_FREE(pHeap, pPrev);
                        }
                        else
                        {
                            pHeap->offFreeHead += pFree->offNext;
                            pNext->offPrev = 0;
                        }
                        ASSERT_CHUNK_FREE(pHeap, pNext);
                    }
                    else
                    {
                        if (pFree->offPrev)
                        {
                            pHeap->offFreeTail += pFree->offPrev;
                            PMMHYPERCHUNKFREE pPrev = (PMMHYPERCHUNKFREE)((char *)pFree + pFree->offPrev);
                            pPrev->offNext = 0;
                            ASSERT_CHUNK_FREE(pHeap, pPrev);
                        }
                        else
                        {
                            pHeap->offFreeHead = NIL_OFFSET;
                            pHeap->offFreeTail = NIL_OFFSET;
                        }
                    }

                    Log3(("mmHyperAllocChunk: cbFree %d -> %d (%d)\n", pHeap->cbFree,
                          pHeap->cbFree - pFree->cb, -(int32_t)pFree->cb));
                    pHeap->cbFree -= pFree->cb;
                    MMHYPERCHUNK_SET_TYPE(&pFree->core, MMHYPERCHUNK_FLAGS_USED);
                    pRet = &pFree->core;
                    ASSERT_CHUNK(pHeap, &pFree->core);
                    Log3(("mmHyperAllocChunk: Converted free chunk %p to used chunk.\n", pFree));
                }
                Log3(("mmHyperAllocChunk: Returning %p\n", pRet));
                break;
            }
        }

        /* next */
        pFree = pFree->offNext ? (PMMHYPERCHUNKFREE)((char *)pFree + pFree->offNext) : NULL;
    }

#ifdef MMHYPER_HEAP_STRICT_FENCE
    uint32_t *pu32End = (uint32_t *)((uint8_t *)(pRet + 1) + cb);
    uint32_t *pu32EndReal = pRet->offNext
                          ? (uint32_t *)((uint8_t *)pRet + pRet->offNext)
                          : (uint32_t *)(pHeap->CTX_SUFF(pbHeap) + pHeap->cbHeap);
    cbFence += (uintptr_t)pu32EndReal - (uintptr_t)pu32End; Assert(!(cbFence & 0x3));
    ASMMemFill32((uint8_t *)pu32EndReal - cbFence, cbFence, MMHYPER_HEAP_STRICT_FENCE_U32);
    pu32EndReal[-1] = cbFence;
#endif
#ifdef MMHYPER_HEAP_STRICT
    mmHyperHeapCheck(pHeap);
#endif
    return pRet;
}


/**
 * Allocates one or more pages of memory from the specified heap.
 * The caller validates the parameters of this request.
 *
 * @returns Pointer to the allocated chunk.
 * @returns NULL on failure.
 * @param   pHeap       The heap.
 * @param   cb          Size of the memory block to allocate.
 * @internal
 */
static void *mmHyperAllocPages(PMMHYPERHEAP pHeap, uint32_t cb)
{
    Log3(("mmHyperAllocPages: Enter cb=%#x\n", cb));

#ifdef MMHYPER_HEAP_STRICT
    mmHyperHeapCheck(pHeap);
#endif

    /*
     * Check if there are any free chunks. (NIL_OFFSET use/not-use forces this check)
     */
    if (pHeap->offFreeHead == NIL_OFFSET)
        return NULL;

    /*
     * Page aligned chunks.
     *
     * Page aligned chunks can only be allocated from the last FREE chunk.
     * This is for reasons of simplicity and fragmentation. Page aligned memory
     * must also be allocated in page aligned sizes. Page aligned memory cannot
     * be freed either.
     *
     * So, for this to work, the last FREE chunk needs to end on a page aligned
     * boundary.
     */
    PMMHYPERCHUNKFREE pFree = (PMMHYPERCHUNKFREE)((char *)pHeap->CTX_SUFF(pbHeap) + pHeap->offFreeTail);
    ASSERT_CHUNK_FREE(pHeap, pFree);
    if (    (((uintptr_t)(&pFree->core + 1) + pFree->cb) & (PAGE_OFFSET_MASK - 1))
        ||  pFree->cb + sizeof(MMHYPERCHUNK) < cb)
    {
        Log3(("mmHyperAllocPages: Not enough/no page aligned memory!\n"));
        return NULL;
    }

    void *pvRet;
    if (pFree->cb > cb)
    {
        /*
         * Simple, just cut the top of the free node and return it.
         */
        pFree->cb -= cb;
        pvRet = (char *)(&pFree->core + 1) + pFree->cb;
        AssertMsg(RT_ALIGN_P(pvRet, PAGE_SIZE) == pvRet, ("pvRet=%p cb=%#x pFree=%p pFree->cb=%#x\n", pvRet, cb, pFree, pFree->cb));
        Log3(("mmHyperAllocPages: cbFree %d -> %d (%d)\n", pHeap->cbFree, pHeap->cbFree - cb, -(int)cb));
        pHeap->cbFree -= cb;
        ASSERT_CHUNK_FREE(pHeap, pFree);
        Log3(("mmHyperAllocPages: Allocated from pFree=%p new pFree->cb=%d\n", pFree, pFree->cb));
    }
    else
    {
        /*
         * Unlink the FREE node.
         */
        pvRet = (char *)(&pFree->core + 1) + pFree->cb - cb;
        Log3(("mmHyperAllocPages: cbFree %d -> %d (%d)\n", pHeap->cbFree, pHeap->cbFree - pFree->cb, -(int32_t)pFree->cb));
        pHeap->cbFree -= pFree->cb;

        /* a scrap of spare memory (unlikely)? add it to the sprevious chunk. */
        if (pvRet != (void *)pFree)
        {
            AssertMsg(MMHYPERCHUNK_GET_OFFPREV(&pFree->core), ("How the *beep* did someone manage to allocated up all the heap with page aligned memory?!?\n"));
            PMMHYPERCHUNK pPrev = (PMMHYPERCHUNK)((char *)pFree + MMHYPERCHUNK_GET_OFFPREV(&pFree->core));
            pPrev->offNext += (uintptr_t)pvRet - (uintptr_t)pFree;
            AssertMsg(!MMHYPERCHUNK_ISFREE(pPrev), ("Free bug?\n"));
#ifdef VBOX_WITH_STATISTICS
            PMMHYPERSTAT pStat = (PMMHYPERSTAT)((uintptr_t)pPrev + pPrev->offStat);
            pStat->cbAllocated += (uintptr_t)pvRet - (uintptr_t)pFree;
            pStat->cbCurAllocated += (uintptr_t)pvRet - (uintptr_t)pFree;
#endif
            Log3(("mmHyperAllocPages: Added %d to %p (page align)\n", (uintptr_t)pvRet - (uintptr_t)pFree, pFree));
        }

        /* unlink from FREE chain. */
        if (pFree->offPrev)
        {
            pHeap->offFreeTail += pFree->offPrev;
            ((PMMHYPERCHUNKFREE)((char *)pFree + pFree->offPrev))->offNext = 0;
        }
        else
        {
            pHeap->offFreeTail = NIL_OFFSET;
            pHeap->offFreeHead = NIL_OFFSET;
        }
        Log3(("mmHyperAllocPages: Unlinked pFree=%d\n", pFree));
    }
    pHeap->offPageAligned = (uintptr_t)pvRet - (uintptr_t)pHeap->CTX_SUFF(pbHeap);
    Log3(("mmHyperAllocPages: Returning %p (page aligned)\n", pvRet));

#ifdef MMHYPER_HEAP_STRICT
    mmHyperHeapCheck(pHeap);
#endif
    return pvRet;
}

#ifdef VBOX_WITH_STATISTICS

/**
 * Get the statistic record for a tag.
 *
 * @returns Pointer to a stat record.
 * @returns NULL on failure.
 * @param   pHeap       The heap.
 * @param   enmTag      The tag.
 */
static PMMHYPERSTAT mmHyperStat(PMMHYPERHEAP pHeap, MMTAG enmTag)
{
    /* try look it up first. */
    PMMHYPERSTAT pStat = (PMMHYPERSTAT)RTAvloGCPhysGet(&pHeap->HyperHeapStatTree, enmTag);
    if (!pStat)
    {
        /* try allocate a new one */
        PMMHYPERCHUNK pChunk = mmHyperAllocChunk(pHeap, RT_ALIGN(sizeof(*pStat), MMHYPER_HEAP_ALIGN_MIN), MMHYPER_HEAP_ALIGN_MIN);
        if (!pChunk)
            return NULL;
        pStat = (PMMHYPERSTAT)(pChunk + 1);
        pChunk->offStat = (uintptr_t)pStat - (uintptr_t)pChunk;

        ASMMemZero32(pStat, sizeof(*pStat));
        pStat->Core.Key = enmTag;
        RTAvloGCPhysInsert(&pHeap->HyperHeapStatTree, &pStat->Core);
    }
    if (!pStat->fRegistered)
    {
# ifdef IN_RING3
        mmR3HyperStatRegisterOne(pHeap->pVMR3, pStat);
# else
        /** @todo schedule a R3 action. */
# endif
    }
    return pStat;
}


# ifdef IN_RING3
/**
 * Registers statistics with STAM.
 *
 */
static void mmR3HyperStatRegisterOne(PVM pVM, PMMHYPERSTAT pStat)
{
    if (pStat->fRegistered)
        return;
    const char *pszTag = mmGetTagName((MMTAG)pStat->Core.Key);
    STAMR3RegisterF(pVM, &pStat->cbCurAllocated, STAMTYPE_U32, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Number of bytes currently allocated.",           "/MM/HyperHeap/%s", pszTag);
    STAMR3RegisterF(pVM, &pStat->cAllocations,   STAMTYPE_U64, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Number of alloc calls.",                         "/MM/HyperHeap/%s/cAllocations", pszTag);
    STAMR3RegisterF(pVM, &pStat->cFrees,         STAMTYPE_U64, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Number of free calls.",                          "/MM/HyperHeap/%s/cFrees", pszTag);
    STAMR3RegisterF(pVM, &pStat->cFailures,      STAMTYPE_U64, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Number of failures.",                            "/MM/HyperHeap/%s/cFailures", pszTag);
    STAMR3RegisterF(pVM, &pStat->cbAllocated,    STAMTYPE_U64, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Total number of allocated bytes.",               "/MM/HyperHeap/%s/cbAllocated", pszTag);
    STAMR3RegisterF(pVM, &pStat->cbFreed,        STAMTYPE_U64, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Total number of freed bytes.",                   "/MM/HyperHeap/%s/cbFreed", pszTag);
    STAMR3RegisterF(pVM, &pStat->cbMaxAllocated, STAMTYPE_U32, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Max number of bytes allocated at the same time.","/MM/HyperHeap/%s/cbMaxAllocated", pszTag);
    pStat->fRegistered = true;
}
# endif /* IN_RING3 */

#endif /* VBOX_WITH_STATISTICS */


/**
 * Free memory allocated using MMHyperAlloc().
 * The caller validates the parameters of this request.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pv          The memory to free.
 * @remark  Try avoid free hyper memory.
 */
static int mmHyperFreeInternal(PVM pVM, void *pv)
{
    Log2(("MMHyperFree: pv=%p\n", pv));
    if (!pv)
        return VINF_SUCCESS;
    AssertMsgReturn(RT_ALIGN_P(pv, MMHYPER_HEAP_ALIGN_MIN) == pv,
                    ("Invalid pointer %p!\n", pv),
                    VERR_INVALID_POINTER);

    /*
     * Get the heap and stats.
     * Validate the chunk at the same time.
     */
    PMMHYPERCHUNK   pChunk = (PMMHYPERCHUNK)((PMMHYPERCHUNK)pv - 1);

    AssertMsgReturn(    (uintptr_t)pChunk + pChunk->offNext >= (uintptr_t)pChunk
                    ||  RT_ALIGN_32(pChunk->offNext, MMHYPER_HEAP_ALIGN_MIN) != pChunk->offNext,
                    ("%p: offNext=%#RX32\n", pv, pChunk->offNext),
                    VERR_INVALID_POINTER);

    AssertMsgReturn(MMHYPERCHUNK_ISUSED(pChunk),
                    ("%p: Not used!\n", pv),
                    VERR_INVALID_POINTER);

    int32_t offPrev = MMHYPERCHUNK_GET_OFFPREV(pChunk);
    AssertMsgReturn(    (uintptr_t)pChunk + offPrev <= (uintptr_t)pChunk
                    && !((uint32_t)-offPrev & (MMHYPER_HEAP_ALIGN_MIN - 1)),
                    ("%p: offPrev=%#RX32!\n", pv, offPrev),
                    VERR_INVALID_POINTER);

    /* statistics */
#ifdef VBOX_WITH_STATISTICS
    PMMHYPERSTAT    pStat = (PMMHYPERSTAT)((uintptr_t)pChunk + pChunk->offStat);
    AssertMsgReturn(    RT_ALIGN_P(pStat, MMHYPER_HEAP_ALIGN_MIN) == (void *)pStat
                    &&  pChunk->offStat,
                    ("%p: offStat=%#RX32!\n", pv, pChunk->offStat),
                    VERR_INVALID_POINTER);
#else
    AssertMsgReturn(!pChunk->offStat,
                    ("%p: offStat=%#RX32!\n", pv, pChunk->offStat),
                    VERR_INVALID_POINTER);
#endif

    /* The heap structure. */
    PMMHYPERHEAP    pHeap = (PMMHYPERHEAP)((uintptr_t)pChunk + pChunk->offHeap);
    AssertMsgReturn(    !((uintptr_t)pHeap & PAGE_OFFSET_MASK)
                    &&  pChunk->offHeap,
                    ("%p: pHeap=%#x offHeap=%RX32\n", pv, pHeap->u32Magic, pChunk->offHeap),
                    VERR_INVALID_POINTER);

    AssertMsgReturn(pHeap->u32Magic == MMHYPERHEAP_MAGIC,
                    ("%p: u32Magic=%#x\n", pv, pHeap->u32Magic),
                    VERR_INVALID_POINTER);
    Assert(pHeap == pVM->mm.s.CTX_SUFF(pHyperHeap)); NOREF(pVM);

    /* Some more verifications using additional info from pHeap. */
    AssertMsgReturn((uintptr_t)pChunk + offPrev >= (uintptr_t)pHeap->CTX_SUFF(pbHeap),
                    ("%p: offPrev=%#RX32!\n", pv, offPrev),
                    VERR_INVALID_POINTER);

    AssertMsgReturn(pChunk->offNext < pHeap->cbHeap,
                    ("%p: offNext=%#RX32!\n", pv, pChunk->offNext),
                    VERR_INVALID_POINTER);

    AssertMsgReturn(   (uintptr_t)pv - (uintptr_t)pHeap->CTX_SUFF(pbHeap) <= pHeap->offPageAligned,
                    ("Invalid pointer %p! (heap: %p-%p)\n", pv, pHeap->CTX_SUFF(pbHeap),
                    (char *)pHeap->CTX_SUFF(pbHeap) + pHeap->offPageAligned),
                    VERR_INVALID_POINTER);

#ifdef MMHYPER_HEAP_STRICT
    mmHyperHeapCheck(pHeap);
#endif

#if defined(VBOX_WITH_STATISTICS) || defined(MMHYPER_HEAP_FREE_POISON)
    /* calc block size. */
    const uint32_t cbChunk = pChunk->offNext
        ? pChunk->offNext
        : pHeap->CTX_SUFF(pbHeap) + pHeap->offPageAligned - (uint8_t *)pChunk;
#endif
#ifdef MMHYPER_HEAP_FREE_POISON
    /* poison the block */
    memset(pChunk + 1, MMHYPER_HEAP_FREE_POISON, cbChunk - sizeof(*pChunk));
#endif

#ifdef MMHYPER_HEAP_FREE_DELAY
# ifdef MMHYPER_HEAP_FREE_POISON
    /*
     * Check poison.
     */
    unsigned i = RT_ELEMENTS(pHeap->aDelayedFrees);
    while (i-- > 0)
        if (pHeap->aDelayedFrees[i].offChunk)
        {
            PMMHYPERCHUNK pCur = (PMMHYPERCHUNK)((uintptr_t)pHeap + pHeap->aDelayedFrees[i].offChunk);
            const size_t cb = pCur->offNext
                ? pCur->offNext - sizeof(*pCur)
                : pHeap->CTX_SUFF(pbHeap) + pHeap->offPageAligned - (uint8_t *)pCur - sizeof(*pCur);
            uint8_t *pab = (uint8_t *)(pCur + 1);
            for (unsigned off = 0; off < cb; off++)
                AssertReleaseMsg(pab[off] == 0xCB,
                                 ("caller=%RTptr cb=%#zx off=%#x: %.*Rhxs\n",
                                  pHeap->aDelayedFrees[i].uCaller, cb, off, RT_MIN(cb - off, 32), &pab[off]));
        }
# endif /* MMHYPER_HEAP_FREE_POISON */

    /*
     * Delayed freeing.
     */
    int rc = VINF_SUCCESS;
    if (pHeap->aDelayedFrees[pHeap->iDelayedFree].offChunk)
    {
        PMMHYPERCHUNK pChunkFree = (PMMHYPERCHUNK)((uintptr_t)pHeap + pHeap->aDelayedFrees[pHeap->iDelayedFree].offChunk);
        rc = mmHyperFree(pHeap, pChunkFree);
    }
    pHeap->aDelayedFrees[pHeap->iDelayedFree].offChunk = (uintptr_t)pChunk - (uintptr_t)pHeap;
    pHeap->aDelayedFrees[pHeap->iDelayedFree].uCaller = (uintptr_t)ASMReturnAddress();
    pHeap->iDelayedFree = (pHeap->iDelayedFree + 1) % RT_ELEMENTS(pHeap->aDelayedFrees);

#else   /* !MMHYPER_HEAP_FREE_POISON */
    /*
     * Call the worker.
     */
    int rc = mmHyperFree(pHeap, pChunk);
#endif  /* !MMHYPER_HEAP_FREE_POISON */

    /*
     * Update statistics.
     */
#ifdef VBOX_WITH_STATISTICS
    pStat->cFrees++;
    if (RT_SUCCESS(rc))
    {
        pStat->cbFreed        += cbChunk;
        pStat->cbCurAllocated -= cbChunk;
    }
    else
        pStat->cFailures++;
#endif

    return rc;
}


/**
 * Wrapper for mmHyperFreeInternal
 */
VMMDECL(int) MMHyperFree(PVM pVM, void *pv)
{
    int rc;

    rc = mmHyperLock(pVM);
    AssertRCReturn(rc, rc);

    LogFlow(("MMHyperFree %p\n", pv));

    rc = mmHyperFreeInternal(pVM, pv);

    mmHyperUnlock(pVM);
    return rc;
}


/**
 * Free memory a memory chunk.
 *
 * @returns VBox status code.
 * @param   pHeap       The heap.
 * @param   pChunk      The memory chunk to free.
 */
static int mmHyperFree(PMMHYPERHEAP pHeap, PMMHYPERCHUNK pChunk)
{
    Log3(("mmHyperFree: Enter pHeap=%p pChunk=%p\n", pHeap, pChunk));
    PMMHYPERCHUNKFREE   pFree = (PMMHYPERCHUNKFREE)pChunk;

    /*
     * Insert into the free list (which is sorted on address).
     *
     * We'll search towards the end of the heap to locate the
     * closest FREE chunk.
     */
    PMMHYPERCHUNKFREE   pLeft = NULL;
    PMMHYPERCHUNKFREE   pRight = NULL;
    if (pHeap->offFreeTail != NIL_OFFSET)
    {
        if (pFree->core.offNext)
        {
            pRight = (PMMHYPERCHUNKFREE)((char *)pFree + pFree->core.offNext);
            ASSERT_CHUNK(pHeap, &pRight->core);
            while (!MMHYPERCHUNK_ISFREE(&pRight->core))
            {
                if (!pRight->core.offNext)
                {
                    pRight = NULL;
                    break;
                }
                pRight = (PMMHYPERCHUNKFREE)((char *)pRight + pRight->core.offNext);
                ASSERT_CHUNK(pHeap, &pRight->core);
            }
        }
        if (!pRight)
            pRight = (PMMHYPERCHUNKFREE)((char *)pHeap->CTX_SUFF(pbHeap) + pHeap->offFreeTail); /** @todo this can't be correct! 'pLeft = .. ; else' I think */
        if (pRight)
        {
            ASSERT_CHUNK_FREE(pHeap, pRight);
            if (pRight->offPrev)
            {
                pLeft = (PMMHYPERCHUNKFREE)((char *)pRight + pRight->offPrev);
                ASSERT_CHUNK_FREE(pHeap, pLeft);
            }
        }
    }
    if (pLeft == pFree)
    {
        AssertMsgFailed(("Freed twice! pv=%p (pChunk=%p)\n", pChunk + 1, pChunk));
        return VERR_INVALID_POINTER;
    }
    pChunk->offStat = 0;

    /*
     * Head free chunk list?
     */
    if (!pLeft)
    {
        MMHYPERCHUNK_SET_TYPE(&pFree->core, MMHYPERCHUNK_FLAGS_FREE);
        pFree->offPrev = 0;
        pHeap->offFreeHead = (uintptr_t)pFree - (uintptr_t)pHeap->CTX_SUFF(pbHeap);
        if (pRight)
        {
            pFree->offNext = (uintptr_t)pRight - (uintptr_t)pFree;
            pRight->offPrev = -(int32_t)pFree->offNext;
        }
        else
        {
            pFree->offNext = 0;
            pHeap->offFreeTail = pHeap->offFreeHead;
        }
        Log3(("mmHyperFree: Inserted %p at head of free chain.\n", pFree));
    }
    else
    {
        /*
         * Can we merge with left hand free chunk?
         */
        if ((char *)pLeft + pLeft->core.offNext == (char *)pFree)
        {
            if (pFree->core.offNext)
            {
                pLeft->core.offNext = pLeft->core.offNext + pFree->core.offNext;
                MMHYPERCHUNK_SET_OFFPREV(((PMMHYPERCHUNK)((char *)pLeft + pLeft->core.offNext)), -(int32_t)pLeft->core.offNext);
            }
            else
                pLeft->core.offNext = 0;
            pFree = pLeft;
            Log3(("mmHyperFree: cbFree %d -> %d (%d)\n", pHeap->cbFree, pHeap->cbFree - pLeft->cb, -(int32_t)pLeft->cb));
            pHeap->cbFree -= pLeft->cb;
            Log3(("mmHyperFree: Merging %p into %p (cb=%d).\n", pFree, pLeft, pLeft->cb));
        }
        /*
         * No, just link it into the free list then.
         */
        else
        {
            MMHYPERCHUNK_SET_TYPE(&pFree->core, MMHYPERCHUNK_FLAGS_FREE);
            pFree->offPrev = (uintptr_t)pLeft - (uintptr_t)pFree;
            pLeft->offNext = -pFree->offPrev;
            if (pRight)
            {
                pFree->offNext = (uintptr_t)pRight - (uintptr_t)pFree;
                pRight->offPrev = -(int32_t)pFree->offNext;
            }
            else
            {
                pFree->offNext = 0;
                pHeap->offFreeTail = (uintptr_t)pFree - (uintptr_t)pHeap->CTX_SUFF(pbHeap);
            }
            Log3(("mmHyperFree: Inserted %p after %p in free list.\n", pFree, pLeft));
        }
    }

    /*
     * Can we merge with right hand free chunk?
     */
    if (pRight && (char *)pRight == (char *)pFree + pFree->core.offNext)
    {
        /* core */
        if (pRight->core.offNext)
        {
            pFree->core.offNext += pRight->core.offNext;
            PMMHYPERCHUNK pNext = (PMMHYPERCHUNK)((char *)pFree + pFree->core.offNext);
            MMHYPERCHUNK_SET_OFFPREV(pNext, -(int32_t)pFree->core.offNext);
            ASSERT_CHUNK(pHeap, pNext);
        }
        else
            pFree->core.offNext = 0;

        /* free */
        if (pRight->offNext)
        {
            pFree->offNext += pRight->offNext;
            ((PMMHYPERCHUNKFREE)((char *)pFree + pFree->offNext))->offPrev = -(int32_t)pFree->offNext;
        }
        else
        {
            pFree->offNext = 0;
            pHeap->offFreeTail = (uintptr_t)pFree - (uintptr_t)pHeap->CTX_SUFF(pbHeap);
        }
        Log3(("mmHyperFree: cbFree %d -> %d (%d)\n", pHeap->cbFree, pHeap->cbFree - pRight->cb, -(int32_t)pRight->cb));
        pHeap->cbFree -= pRight->cb;
        Log3(("mmHyperFree: Merged %p (cb=%d) into %p.\n", pRight, pRight->cb, pFree));
    }

    /* calculate the size. */
    if (pFree->core.offNext)
        pFree->cb = pFree->core.offNext - sizeof(MMHYPERCHUNK);
    else
        pFree->cb = pHeap->offPageAligned - ((uintptr_t)pFree - (uintptr_t)pHeap->CTX_SUFF(pbHeap)) - sizeof(MMHYPERCHUNK);
    Log3(("mmHyperFree: cbFree %d -> %d (%d)\n", pHeap->cbFree, pHeap->cbFree + pFree->cb, pFree->cb));
    pHeap->cbFree += pFree->cb;
    ASSERT_CHUNK_FREE(pHeap, pFree);

#ifdef MMHYPER_HEAP_STRICT
    mmHyperHeapCheck(pHeap);
#endif
    return VINF_SUCCESS;
}


#if defined(DEBUG) || defined(MMHYPER_HEAP_STRICT_FENCE)
/**
 * Dumps a heap chunk to the log.
 *
 * @param   pHeap       Pointer to the heap.
 * @param   pCur        Pointer to the chunk.
 */
static void mmHyperHeapDumpOne(PMMHYPERHEAP pHeap, PMMHYPERCHUNKFREE pCur)
{
    if (MMHYPERCHUNK_ISUSED(&pCur->core))
    {
        if (pCur->core.offStat)
        {
            PMMHYPERSTAT pStat = (PMMHYPERSTAT)((uintptr_t)pCur + pCur->core.offStat);
            const char *pszSelf = pCur->core.offStat == sizeof(MMHYPERCHUNK) ? " stat record" : "";
#ifdef IN_RING3
            Log(("%p  %06x USED offNext=%06x offPrev=-%06x %s%s\n",
                 pCur, (uintptr_t)pCur - (uintptr_t)pHeap->CTX_SUFF(pbHeap),
                 pCur->core.offNext, -MMHYPERCHUNK_GET_OFFPREV(&pCur->core),
                 mmGetTagName((MMTAG)pStat->Core.Key), pszSelf));
#else
            Log(("%p  %06x USED offNext=%06x offPrev=-%06x %d%s\n",
                 pCur, (uintptr_t)pCur - (uintptr_t)pHeap->CTX_SUFF(pbHeap),
                 pCur->core.offNext, -MMHYPERCHUNK_GET_OFFPREV(&pCur->core),
                 (MMTAG)pStat->Core.Key, pszSelf));
#endif
            NOREF(pStat); NOREF(pszSelf);
        }
        else
            Log(("%p  %06x USED offNext=%06x offPrev=-%06x\n",
                 pCur, (uintptr_t)pCur - (uintptr_t)pHeap->CTX_SUFF(pbHeap),
                 pCur->core.offNext, -MMHYPERCHUNK_GET_OFFPREV(&pCur->core)));
    }
    else
        Log(("%p  %06x FREE offNext=%06x offPrev=-%06x : cb=%06x offNext=%06x offPrev=-%06x\n",
             pCur, (uintptr_t)pCur - (uintptr_t)pHeap->CTX_SUFF(pbHeap),
             pCur->core.offNext, -MMHYPERCHUNK_GET_OFFPREV(&pCur->core), pCur->cb, pCur->offNext, pCur->offPrev));
}
#endif /* DEBUG || MMHYPER_HEAP_STRICT */


#ifdef MMHYPER_HEAP_STRICT
/**
 * Internal consistency check.
 */
static void mmHyperHeapCheck(PMMHYPERHEAP pHeap)
{
    PMMHYPERCHUNKFREE pPrev = NULL;
    PMMHYPERCHUNKFREE pCur = (PMMHYPERCHUNKFREE)pHeap->CTX_SUFF(pbHeap);
    for (;;)
    {
        if (MMHYPERCHUNK_ISUSED(&pCur->core))
            ASSERT_CHUNK_USED(pHeap, &pCur->core);
        else
            ASSERT_CHUNK_FREE(pHeap, pCur);
        if (pPrev)
            AssertMsg((int32_t)pPrev->core.offNext == -MMHYPERCHUNK_GET_OFFPREV(&pCur->core),
                      ("pPrev->core.offNext=%d offPrev=%d\n", pPrev->core.offNext, MMHYPERCHUNK_GET_OFFPREV(&pCur->core)));

# ifdef MMHYPER_HEAP_STRICT_FENCE
        uint32_t off = (uint8_t *)pCur - pHeap->CTX_SUFF(pbHeap);
        if (    MMHYPERCHUNK_ISUSED(&pCur->core)
            &&  off < pHeap->offPageAligned)
        {
            uint32_t cbCur = pCur->core.offNext
                           ? pCur->core.offNext
                           : pHeap->cbHeap - off;
            uint32_t *pu32End = ((uint32_t *)((uint8_t *)pCur + cbCur));
            uint32_t cbFence = pu32End[-1];
            if (RT_UNLIKELY(    cbFence >= cbCur - sizeof(*pCur)
                            ||  cbFence < MMHYPER_HEAP_STRICT_FENCE_SIZE))
            {
                mmHyperHeapDumpOne(pHeap, pCur);
                Assert(cbFence < cbCur - sizeof(*pCur));
                Assert(cbFence >= MMHYPER_HEAP_STRICT_FENCE_SIZE);
            }

            uint32_t *pu32Bad = ASMMemFirstMismatchingU32((uint8_t *)pu32End - cbFence, cbFence - sizeof(uint32_t), MMHYPER_HEAP_STRICT_FENCE_U32);
            if (RT_UNLIKELY(pu32Bad))
            {
                mmHyperHeapDumpOne(pHeap, pCur);
                Assert(!pu32Bad);
            }
        }
# endif

        /* next */
        if (!pCur->core.offNext)
            break;
        pPrev = pCur;
        pCur = (PMMHYPERCHUNKFREE)((char *)pCur + pCur->core.offNext);
    }
}
#endif


/**
 * Performs consistency checks on the heap if MMHYPER_HEAP_STRICT was
 * defined at build time.
 *
 * @param   pVM         The cross context VM structure.
 */
VMMDECL(void) MMHyperHeapCheck(PVM pVM)
{
#ifdef MMHYPER_HEAP_STRICT
    int rc;

    rc = mmHyperLock(pVM);
    AssertRC(rc);
    mmHyperHeapCheck(pVM->mm.s.CTX_SUFF(pHyperHeap));
    mmHyperUnlock(pVM);
#else
    NOREF(pVM);
#endif
}


#ifdef DEBUG
/**
 * Dumps the hypervisor heap to Log.
 * @param   pVM     The cross context VM structure.
 */
VMMDECL(void) MMHyperHeapDump(PVM pVM)
{
    Log(("MMHyperHeapDump: *** heap dump - start ***\n"));
    PMMHYPERHEAP pHeap = pVM->mm.s.CTX_SUFF(pHyperHeap);
    PMMHYPERCHUNKFREE pCur = (PMMHYPERCHUNKFREE)pHeap->CTX_SUFF(pbHeap);
    for (;;)
    {
        mmHyperHeapDumpOne(pHeap, pCur);

        /* next */
        if (!pCur->core.offNext)
            break;
        pCur = (PMMHYPERCHUNKFREE)((char *)pCur + pCur->core.offNext);
    }
    Log(("MMHyperHeapDump: *** heap dump - end ***\n"));
}
#endif


/**
 * Query the amount of free memory in the hypervisor heap.
 *
 * @returns Number of free bytes in the hypervisor heap.
 */
VMMDECL(size_t) MMHyperHeapGetFreeSize(PVM pVM)
{
    return pVM->mm.s.CTX_SUFF(pHyperHeap)->cbFree;
}


/**
 * Query the size the hypervisor heap.
 *
 * @returns The size of the hypervisor heap in bytes.
 */
VMMDECL(size_t) MMHyperHeapGetSize(PVM pVM)
{
    return pVM->mm.s.CTX_SUFF(pHyperHeap)->cbHeap;
}


/**
 * Converts a context neutral heap offset into a pointer.
 *
 * @returns Pointer to hyper heap data.
 * @param   pVM         The cross context VM structure.
 * @param   offHeap     The hyper heap offset.
 */
VMMDECL(void *) MMHyperHeapOffsetToPtr(PVM pVM, uint32_t offHeap)
{
    Assert(offHeap - MMYPERHEAP_HDR_SIZE <= pVM->mm.s.CTX_SUFF(pHyperHeap)->cbHeap);
    return (uint8_t *)pVM->mm.s.CTX_SUFF(pHyperHeap) + offHeap;
}


/**
 * Converts a context specific heap pointer into a neutral heap offset.
 *
 * @returns Heap offset.
 * @param   pVM         The cross context VM structure.
 * @param   pv          Pointer to the heap data.
 */
VMMDECL(uint32_t) MMHyperHeapPtrToOffset(PVM pVM, void *pv)
{
    size_t offHeap = (uint8_t *)pv - (uint8_t *)pVM->mm.s.CTX_SUFF(pHyperHeap);
    Assert(offHeap - MMYPERHEAP_HDR_SIZE <= pVM->mm.s.CTX_SUFF(pHyperHeap)->cbHeap);
    return (uint32_t)offHeap;
}


/**
 * Query the address and size the hypervisor memory area.
 *
 * @returns Base address of the hypervisor area.
 * @param   pVM         The cross context VM structure.
 * @param   pcb         Where to store the size of the hypervisor area. (out)
 */
VMMDECL(RTGCPTR) MMHyperGetArea(PVM pVM, size_t *pcb)
{
    if (pcb)
        *pcb = pVM->mm.s.cbHyperArea;
    return pVM->mm.s.pvHyperAreaGC;
}


/**
 * Checks if an address is within the hypervisor memory area.
 *
 * @returns true if inside.
 * @returns false if outside.
 * @param   pVM         The cross context VM structure.
 * @param   GCPtr       The pointer to check.
 */
VMMDECL(bool) MMHyperIsInsideArea(PVM pVM, RTGCPTR GCPtr)
{
    return (RTGCUINTPTR)GCPtr - (RTGCUINTPTR)pVM->mm.s.pvHyperAreaGC < pVM->mm.s.cbHyperArea;
}

