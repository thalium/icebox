/* $Id: kHlpBareHeap.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kHlpBare - Heap.
 */

/*
 * Copyright (c) 2006-2007 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#define KHLPHEAP_STRICT

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <k/kHlpAlloc.h>
#include <k/kHlpString.h>
#include <k/kHlpAssert.h>

#if K_OS == K_OS_OS2
# define INCL_BASE
# define INCL_ERRORS
# include <os2.h>

#elif  K_OS == K_OS_WINDOWS
# include <Windows.h>

#else
# include <k/kHlpAlloc.h>
#endif


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * A heap block.
 */
typedef struct KHLPHEAPBLOCK
{
    /** Next block in the global list. */
    struct KHLPHEAPBLOCK   *pNext;
    /** Previous block in the global list. */
    struct KHLPHEAPBLOCK   *pPrev;
    /** The size of this block including this header. */
    KSIZE                   cb;
    /** The flags. */
    KSIZE                   fFlags;
} KHLPHEAPBLOCK, *PKHLPHEAPBLOCK;

/** Indicates whether the block is free (set) or allocated (clear). */
#define KHLPHEAPBLOCK_FLAG_FREE     ((KSIZE)1)
/** Valid flag mask. */
#define KHLPHEAPBLOCK_FLAG_MASK     ((KSIZE)1)

/** Checks if the block is freed. */
#define KHLPHEAPBLOCK_IS_FREE(pB)       ( (pB)->fFlags & KHLPHEAPBLOCK_FLAG_FREE )
/** Check if the block is allocated. */
#define KHLPHEAPBLOCK_IS_ALLOCATED(pB)  !KHLPHEAPBLOCK_IS_FREE(pB)
/** Checks if the two blocks are adjacent.
 * Assumes pB1 < pB2. */
#define KHLPHEAPBLOCK_IS_ADJACENT(pB1, pB2) \
    ( ((KUPTR)(pB1) + (pB1)->cb) == (KUPTR)(pB2) )

/** The block alignment. */
#define KHLPHEAPBLOCK_ALIGNMENT     sizeof(KHLPHEAPBLOCK)

/** @def KHLPHEAP_ASSERT
 * Heap assertion. */
/** @def KHLPHEAP_ASSERT_BLOCK
 * Assert that a heap block is valid. */
/** @def KHLPHEAP_ASSERT_FREE
 * Assert that a heap free block is valid. */
#ifdef KHLPHEAP_STRICT
# define KHLPHEAP_ASSERT(expr)      kHlpAssert(expr)

# define KHLPHEAP_ASSERT_BLOCK(pHeap, pBlock) \
    do { \
        KHLPHEAP_ASSERT(!((pBlock)->fFlags & ~KHLPHEAPBLOCK_FLAG_MASK)); \
        KHLPHEAP_ASSERT(!((pBlock)->cb & (KHLPHEAPBLOCK_ALIGNMENT - 1))); \
        KHLPHEAP_ASSERT((KUPTR)(pBlock)->pPrev < (KUPTR)(pBlock)); \
        KHLPHEAP_ASSERT((KUPTR)(pBlock)->pNext > (KUPTR)(pBlock) || !(pBlock)->pNext); \
    } while (0)

# define KHLPHEAP_ASSERT_FREE(pHeap, pFree) \
    do { \
        KHLPHEAP_ASSERT_BLOCK(pHeap, &(pFree)->Core); \
        KHLPHEAP_ASSERT((KUPTR)(pFree)->pPrev < (KUPTR)(pFree)); \
        KHLPHEAP_ASSERT((KUPTR)(pFree)->pNext > (KUPTR)(pFree) || !(pFree)->pNext); \
    } while (0)

#else
# define KHLPHEAP_ASSERT(expr)          do { } while (0)
# define KHLPHEAP_ASSERT_BLOCK(pH, pB)  do { } while (0)
# define KHLPHEAP_ASSERT_FREE(pH, pF)   do { } while (0)
#endif


/**
 * A free heap block.
 */
typedef struct KHLPHEAPFREE
{
    /** The core bit which we have in common with used blocks. */
    KHLPHEAPBLOCK           Core;
    /** The next free block. */
    struct KHLPHEAPFREE    *pNext;
    /** The previous free block. */
    struct KHLPHEAPFREE    *pPrev;
} KHLPHEAPFREE, *PKHLPHEAPFREE;


/**
 * A heap segment.
 */
typedef struct KHLPHEAPSEG
{
    /** The base address of the segment. */
    void                   *pvBase;
    /** The length of the segment (in bytes). */
    KSIZE                   cb;
} KHLPHEAPSEG, *PKHLPHEAPSEG;

/**
 * Bundle of heap segments.
 */
typedef struct KHLPHEAPSEGS
{
    /** Pointer to the next segment bundle. */
    struct KHLPHEAPSEGS    *pNext;
    /** The number of segments used. */
    KU32                    cSegs;
    /** Array of chunks. */
    KHLPHEAPSEG             aSegs[64];
} KHLPHEAPSEGS, *PKHLPHEAPSEGS;


/**
 * Heap anchor block.
 */
typedef struct KHLPHEAPANCHOR
{
    /** Head of the block list. */
    PKHLPHEAPBLOCK          pHead;
    /** Tail of the block list. */
    PKHLPHEAPBLOCK          pTail;
    /** Head of the free list. */
    PKHLPHEAPFREE           pFreeHead;
    /** Head segment bundle.
     * The order of this list is important, but a bit peculiar.
     * Logically, SegsHead::pNext is the tail pointer. */
    KHLPHEAPSEGS            SegsHead;
} KHLPHEAPANCHOR, *PKHLPHEAPANCHOR;



/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** The heap anchor block. */
static KHLPHEAPANCHOR       g_Heap;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int      khlpHeapInit(PKHLPHEAPANCHOR pHeap);
static void     khlpHeapDelete(PKHLPHEAPANCHOR pHeap);
static void *   khlpHeapAlloc(PKHLPHEAPANCHOR pHeap, KSIZE cb);
static void     khlpHeapFree(PKHLPHEAPANCHOR pHeap, void *pv);
static KSIZE    khlpHeapBlockSize(PKHLPHEAPANCHOR pHeap, void *pv);
static void     khlpHeapDonate(PKHLPHEAPANCHOR pHeap, void *pv, KSIZE cb);
static int      khlpHeapSegAlloc(PKHLPHEAPSEG pSeg, KSIZE cb);
static void     khlpHeapSegFree(PKHLPHEAPSEG pSeg);


/**
 * Initializes the kLdr heap.
 *
 * @returns 0 on success, non-zero OS specific status code on failure.
 */
KHLP_DECL(int) kHlpHeapInit(void)
{
    return khlpHeapInit(&g_Heap);
}


/**
 * Terminates the kLdr heap.
 */
KHLP_DECL(void) kHlpHeapTerm(void)
{
    khlpHeapDelete(&g_Heap);
}


KHLP_DECL(void *) kHlpAlloc(KSIZE cb)
{
    return khlpHeapAlloc(&g_Heap, cb);
}


KHLP_DECL(void *) kHlpAllocZ(KSIZE cb)
{
    void *pv = khlpHeapAlloc(&g_Heap, cb);
    if (pv)
        kHlpMemSet(pv, 0, cb);
    return pv;
}


KHLP_DECL(void *) kHlpDup(const void *pv, KSIZE cb)
{
    void *pvNew = khlpHeapAlloc(&g_Heap, cb);
    if (pvNew)
        kHlpMemCopy(pvNew, pv, cb);
    return pvNew;
}


KHLP_DECL(char *) kHlpStrDup(const char *psz)
{
    return (char *)kHlpDup(psz, kHlpStrLen(psz) + 1);
}


KHLP_DECL(void *) kHlpRealloc(void *pv, KSIZE cb)
{
    void *pvNew;
    if (!cb)
    {
        kHlpFree(pv);
        pvNew = NULL;
    }
    else if (!pv)
        pvNew = khlpHeapAlloc(&g_Heap, cb);
    else
    {
        KSIZE cbToCopy = khlpHeapBlockSize(&g_Heap, pv);
        pvNew = khlpHeapAlloc(&g_Heap, cb);
        if (pvNew)
        {
            kHlpMemCopy(pvNew, pv, cb);
            kHlpFree(pv);
        }
    }
    return pvNew;
}


KHLP_DECL(void) kHlpFree(void *pv)
{
    khlpHeapFree(&g_Heap, pv);
}


/**
 * Donates memory to the heap.
 *
 * @param   pv      The address of the memory.
 * @param   cb      The amount of memory.
 */
KHLP_DECL(void) kHlpHeapDonate(void *pv, KSIZE cb)
{
    khlpHeapDonate(&g_Heap, pv, cb);
}



/**
 * Initializes the heap anchor.
 *
 * @returns 0 on success, non-zero on failure.
 * @param   pHeap   The heap anchor to be initialized.
 */
static int khlpHeapInit(PKHLPHEAPANCHOR pHeap)
{
    pHeap->pHead = NULL;
    pHeap->pTail = NULL;
    pHeap->pFreeHead = NULL;
    pHeap->SegsHead.pNext = NULL;
    pHeap->SegsHead.cSegs = 0;
    return 0;
}


/**
 * Deletes a heap.
 * This will free all resources (memory) associated with the heap.
 *
 * @param   pHeap   The heap to be deleted.
 */
static void khlpHeapDelete(PKHLPHEAPANCHOR pHeap)
{
    /*
     * Free the segments, LIFO order.
     * The head element is the last to be free, while the
     * head.pNext is really the tail pointer - neat or what?
     */
    while (     pHeap->SegsHead.cSegs
           ||   pHeap->SegsHead.pNext)
    {
        /* find the tail. */
        KU32            iSeg;
        PKHLPHEAPSEGS   pSegs = pHeap->SegsHead.pNext;
        if (!pSegs)
            pSegs = &pHeap->SegsHead;
        else
        {
            pHeap->SegsHead.pNext = pSegs->pNext;
            pSegs->pNext = NULL;
        }

        /* free the segments */
        iSeg = pSegs->cSegs;
        while (iSeg-- > 0)
            khlpHeapSegFree(&pSegs->aSegs[iSeg]);
        pSegs->cSegs = 0;
    }

    /* Zap the anchor. */
    pHeap->pHead = NULL;
    pHeap->pTail = NULL;
    pHeap->pFreeHead = NULL;
    pHeap->SegsHead.pNext = NULL;
    pHeap->SegsHead.cSegs = 0;
}


/**
 * Internal heap block allocator.
 */
static void *   kldrHeapAllocSub(PKHLPHEAPANCHOR pHeap, KSIZE cb)
{
    /*
     * Find a fitting free block.
     */
    const KSIZE     cbReq = K_ALIGN_Z(cb + sizeof(KHLPHEAPBLOCK), KHLPHEAPBLOCK_ALIGNMENT);
    PKHLPHEAPFREE   pCur = pHeap->pFreeHead;
    while (pCur)
    {
        if (pCur->Core.cb >= cbReq)
        {
            if (pCur->Core.cb != cbReq)
            {
                /* check and see if there is a better match close by. */
                PKHLPHEAPFREE pCur2 = pCur->pNext;
                unsigned i = 16;
                while (i-- > 0 && pCur2)
                {
                    if (pCur2->Core.cb >= cbReq)
                    {
                        if (pCur2->Core.cb == cbReq)
                        {
                            pCur = pCur2;
                            break;
                        }
                        if (pCur2->Core.cb < pCur->Core.cb)
                            pCur = pCur2;
                    }

                    /* next */
                    KHLPHEAP_ASSERT_FREE(pHeap, pCur2);
                    pCur2 = pCur2->pNext;
                }
            }
            break;
        }

        /* next */
        KHLPHEAP_ASSERT_FREE(pHeap, pCur);
        pCur = pCur->pNext;
    }
    if (!pCur)
        return NULL;
    KHLPHEAP_ASSERT_FREE(pHeap, pCur);

    /*
     * Do we need to split out a block?
     */
    if (pCur->Core.cb - cbReq >= KHLPHEAPBLOCK_ALIGNMENT * 2)
    {
        PKHLPHEAPBLOCK pNew;

        pCur->Core.cb -= cbReq;

        pNew = (PKHLPHEAPBLOCK)((KUPTR)pCur + pCur->Core.cb);
        pNew->fFlags = 0;
        pNew->cb = cbReq;
        pNew->pNext = pCur->Core.pNext;
        if (pNew->pNext)
            pNew->pNext->pPrev = pNew;
        else
            pHeap->pTail = pNew;
        pNew->pPrev = &pCur->Core;
        pCur->Core.pNext = pNew;

        KHLPHEAP_ASSERT_FREE(pHeap, pCur);
        KHLPHEAP_ASSERT_BLOCK(pHeap, pNew);
        return pNew + 1;
    }

    /*
     * No, just unlink it from the free list and return.
     */
    if (pCur->pNext)
        pCur->pNext->pPrev = pCur->pPrev;
    if (pCur->pPrev)
        pCur->pPrev->pNext = pCur->pNext;
    else
        pHeap->pFreeHead = pCur->pNext;
    pCur->Core.fFlags &= ~KHLPHEAPBLOCK_FLAG_FREE;

    KHLPHEAP_ASSERT_BLOCK(pHeap, &pCur->Core);
    return &pCur->Core + 1;
}


/**
 * Allocate a heap block.
 *
 * @returns Pointer to the allocated heap block on success. On failure NULL is returned.
 * @param   pHeap   The heap.
 * @param   cb      The requested heap block size.
 */
static void *   khlpHeapAlloc(PKHLPHEAPANCHOR pHeap, KSIZE cb)
{
    void *pv;

    /* adjust the requested block size. */
    cb = K_ALIGN_Z(cb, KHLPHEAPBLOCK_ALIGNMENT);
    if (!cb)
        cb = KHLPHEAPBLOCK_ALIGNMENT;

    /* try allocate the block. */
    pv = kldrHeapAllocSub(pHeap, cb);
    if (!pv)
    {
        /*
         * Failed, add another segment and try again.
         */
        KHLPHEAPSEG Seg;
        if (khlpHeapSegAlloc(&Seg, cb + sizeof(KHLPHEAPSEGS) + sizeof(KHLPHEAPBLOCK) * 16))
            return NULL;

        /* donate before insterting the segment, this makes sure we got heap to expand the segment list. */
        khlpHeapDonate(pHeap, Seg.pvBase, Seg.cb);

        /* insert the segment. */
        if (pHeap->SegsHead.cSegs < sizeof(pHeap->SegsHead.aSegs) / sizeof(pHeap->SegsHead.aSegs[0]))
            pHeap->SegsHead.aSegs[pHeap->SegsHead.cSegs++] = Seg;
        else if (   pHeap->SegsHead.pNext
                 && pHeap->SegsHead.pNext->cSegs < sizeof(pHeap->SegsHead.aSegs) / sizeof(pHeap->SegsHead.aSegs[0]))
            pHeap->SegsHead.pNext->aSegs[pHeap->SegsHead.pNext->cSegs++] = Seg;
        else
        {
            PKHLPHEAPSEGS pSegs = (PKHLPHEAPSEGS)kldrHeapAllocSub(pHeap, sizeof(*pSegs));
            KHLPHEAP_ASSERT(pSegs);
            pSegs->pNext = pHeap->SegsHead.pNext;
            pHeap->SegsHead.pNext = pSegs;
            pSegs->aSegs[0] = Seg;
            pSegs->cSegs = 1;
        }

        /* retry (should succeed) */
        pv = kldrHeapAllocSub(pHeap, cb);
        KHLPHEAP_ASSERT(pv);
    }

    return pv;
}


/**
 * Frees a heap block.
 *
 * @param   pHeap   The heap.
 * @param   pv      The pointer returned by khlpHeapAlloc().
 */
static void     khlpHeapFree(PKHLPHEAPANCHOR pHeap, void *pv)
{
    PKHLPHEAPFREE pFree, pLeft, pRight;

    /* ignore NULL pointers. */
    if (!pv)
        return;

    pFree = (PKHLPHEAPFREE)((PKHLPHEAPBLOCK)pv - 1);
    KHLPHEAP_ASSERT_BLOCK(pHeap, &pFree->Core);
    KHLPHEAP_ASSERT(KHLPHEAPBLOCK_IS_ALLOCATED(&pFree->Core));

    /*
     * Merge or link with left node?
     */
    pLeft = (PKHLPHEAPFREE)pFree->Core.pPrev;
    if (    pLeft
        &&  KHLPHEAPBLOCK_IS_FREE(&pLeft->Core)
        &&  KHLPHEAPBLOCK_IS_ADJACENT(&pLeft->Core, &pFree->Core)
       )
    {
        /* merge left */
        pLeft->Core.pNext = pFree->Core.pNext;
        if (pFree->Core.pNext)
            pFree->Core.pNext->pPrev = &pLeft->Core;
        else
            pHeap->pTail = &pLeft->Core;

        pLeft->Core.cb += pFree->Core.cb;
        pFree->Core.fFlags = ~0;
        pFree = pLeft;
    }
    else
    {
        /* link left */
        while (pLeft && !KHLPHEAPBLOCK_IS_FREE(&pLeft->Core))
            pLeft = (PKHLPHEAPFREE)pLeft->Core.pPrev;
        if (pLeft)
        {
            pFree->pPrev = pLeft;
            pFree->pNext = pLeft->pNext;
            if (pLeft->pNext)
                pLeft->pNext->pPrev = pFree;
            pLeft->pNext  = pFree;
        }
        else
        {
            pFree->pPrev = NULL;
            pFree->pNext = pHeap->pFreeHead;
            if (pHeap->pFreeHead)
                pHeap->pFreeHead->pPrev = pFree;
            pHeap->pFreeHead = pFree;
        }
        pFree->Core.fFlags |= KHLPHEAPBLOCK_FLAG_FREE;
    }
    KHLPHEAP_ASSERT_FREE(pHeap, pFree);

    /*
     * Merge right?
     */
    pRight = (PKHLPHEAPFREE)pFree->Core.pNext;
    if (    pRight
        &&  KHLPHEAPBLOCK_IS_FREE(&pRight->Core)
        &&  KHLPHEAPBLOCK_IS_ADJACENT(&pFree->Core, pRight)
       )
    {
        /* unlink pRight from the global list. */
        pFree->Core.pNext = pRight->Core.pNext;
        if (pRight->Core.pNext)
            pRight->Core.pNext->pPrev = &pFree->Core;
        else
            pHeap->pTail = &pFree->Core;

        /* unlink pRight from the free list. */
        pFree->pNext = pRight->pNext;
        if (pRight->pNext)
            pRight->pNext->pPrev = pFree;

        /* update size and invalidate pRight. */
        pFree->Core.cb += pRight->Core.cb;
        pRight->Core.fFlags = ~0;
    }
}


/**
 * Calcs the size of a heap block.
 *
 * @returns The block size (in bytes).
 * @param   pHeap       The heap.
 * @param   pv          Pointer to an in-use heap block.
 */
static KSIZE khlpHeapBlockSize(PKHLPHEAPANCHOR pHeap, void *pv)
{
    PKHLPHEAPBLOCK pBlock  = (PKHLPHEAPBLOCK)pv - 1;
    KHLPHEAP_ASSERT_BLOCK(pHeap, pBlock);
    KHLPHEAP_ASSERT(KHLPHEAPBLOCK_IS_ALLOCATED(pBlock));
    return (KU8 *)pBlock->pNext - (KU8 *)pv;
}


/**
 * Donates memory to the heap.
 *
 * The donated memory is returned to the donator when the heap is deleted.
 *
 * @param pHeap The heap
 * @param pv    The pointer to the donated memory.
 * @param cb    Size of the donated memory.
 */
static void     khlpHeapDonate(PKHLPHEAPANCHOR pHeap, void *pv, KSIZE cb)
{
    PKHLPHEAPBLOCK pBlock;

    /*
     * Don't bother with small donations.
     */
    if (cb < KHLPHEAPBLOCK_ALIGNMENT * 4)
        return;

    /*
     * Align the donation on a heap block boundrary.
     */
    if ((KUPTR)pv & (KHLPHEAPBLOCK_ALIGNMENT - 1))
    {
        cb -= (KUPTR)pv & 31;
        pv = K_ALIGN_P(pv, KHLPHEAPBLOCK_ALIGNMENT);
    }
    cb &= ~(KSIZE)(KHLPHEAPBLOCK_ALIGNMENT - 1);

    /*
     * Create an allocated block, link it and free it.
     */
    pBlock = (PKHLPHEAPBLOCK)pv;
    pBlock->pNext = NULL;
    pBlock->pPrev = NULL;
    pBlock->cb = cb;
    pBlock->fFlags = 0;

    /* insert */
    if ((KUPTR)pBlock < (KUPTR)pHeap->pHead)
    {
        /* head */
        pBlock->pNext = pHeap->pHead;
        pHeap->pHead->pPrev = pBlock;
        pHeap->pHead = pBlock;
    }
    else if ((KUPTR)pBlock > (KUPTR)pHeap->pTail)
    {
        if (pHeap->pTail)
        {
            /* tail */
            pBlock->pPrev = pHeap->pTail;
            pHeap->pTail->pNext = pBlock;
            pHeap->pTail = pBlock;
        }
        else
        {
            /* first */
            pHeap->pHead = pBlock;
            pHeap->pTail = pBlock;
        }
    }
    else
    {
        /* in list (unlikely) */
        PKHLPHEAPBLOCK pPrev = pHeap->pHead;
        PKHLPHEAPBLOCK pCur = pPrev->pNext;
        for (;;)
        {
            KHLPHEAP_ASSERT_BLOCK(pHeap, pCur);
            if ((KUPTR)pCur > (KUPTR)pBlock)
                break;
            pPrev = pCur;
            pCur = pCur->pNext;
        }

        pBlock->pNext = pCur;
        pBlock->pPrev = pPrev;
        pPrev->pNext = pBlock;
        pCur->pPrev = pBlock;
    }
    KHLPHEAP_ASSERT_BLOCK(pHeap, pBlock);

    /* free it */
    khlpHeapFree(pHeap, pBlock + 1);
}



/**
 * Allocates a new segment.
 *
 * @returns 0 on success, non-zero OS status code on failure.
 * @param   pSeg    Where to put the info about the allocated segment.
 * @param   cbMin   The minimum segment size.
 */
static int khlpHeapSegAlloc(PKHLPHEAPSEG pSeg, KSIZE cbMin)
{
#if K_OS == K_OS_OS2
    APIRET rc;

    pSeg->cb = (cbMin + 0xffff) & ~(KSIZE)0xffff;
    pSeg->pvBase = NULL;
    rc = DosAllocMem(&pSeg->pvBase, pSeg->cb, PAG_COMMIT | PAG_READ | PAG_WRITE | OBJ_ANY);
    if (rc == ERROR_INVALID_PARAMETER)
        rc = DosAllocMem(&pSeg->pvBase, pSeg->cb, PAG_COMMIT | PAG_READ | PAG_WRITE);
    if (rc)
    {
        pSeg->pvBase = NULL;
        pSeg->cb = 0;
        return rc;
    }

#elif  K_OS == K_OS_WINDOWS
    pSeg->cb = (cbMin + 0xffff) & ~(KSIZE)0xffff;
    pSeg->pvBase = VirtualAlloc(NULL, pSeg->cb, MEM_COMMIT, PAGE_READWRITE);
    if (!pSeg->pvBase)
    {
        pSeg->cb = 0;
        return GetLastError();
    }

#else
    int rc;

    pSeg->cb = (cbMin + 0xffff) & ~(KSIZE)0xffff;
    pSeg->pvBase = NULL;
    rc = kHlpPageAlloc(&pSeg->pvBase, pSeg->cb, KPROT_READWRITE, K_FALSE);
    if (rc)
    {
        pSeg->pvBase = NULL;
        pSeg->cb = 0;
        return rc;
    }

#endif

    return 0;
}


/**
 * Frees a segment.
 *
 * @param   pSeg    The segment to be freed.
 */
static void khlpHeapSegFree(PKHLPHEAPSEG pSeg)
{
#if K_OS == K_OS_OS2
    APIRET rc = DosFreeMem(pSeg->pvBase);
    KHLPHEAP_ASSERT(!rc); (void)rc;

#elif  K_OS == K_OS_WINDOWS
    BOOL fRc = VirtualFree(pSeg->pvBase, 0 /*pSeg->cb*/, MEM_RELEASE);
    KHLPHEAP_ASSERT(fRc); (void)fRc;

#else
    int rc = kHlpPageFree(pSeg->pvBase, pSeg->cb);
    KHLPHEAP_ASSERT(!rc); (void)rc;

#endif
}

