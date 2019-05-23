/* $Id: tstRTAvl.cpp $ */
/** @file
 * IPRT Testcase - AVL trees.
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
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/avl.h>

#include <iprt/asm.h>
#include <iprt/initterm.h>
#include <iprt/mem.h>
#include <iprt/rand.h>
#include <iprt/stdarg.h>
#include <iprt/string.h>
#include <iprt/test.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
typedef struct TRACKER
{
    /** The max key value (exclusive). */
    uint32_t    MaxKey;
    /** The last allocated key. */
    uint32_t    LastAllocatedKey;
    /** The number of set bits in the bitmap. */
    uint32_t    cSetBits;
    /** The bitmap size. */
    uint32_t    cbBitmap;
    /** Bitmap containing the allocated nodes. */
    uint8_t     abBitmap[1];
} TRACKER, *PTRACKER;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static RTTEST g_hTest;
static RTRAND g_hRand;


/**
 * Creates a new tracker.
 *
 * @returns Pointer to the new tracker.
 * @param   MaxKey      The max key value for the tracker. (exclusive)
 */
static PTRACKER TrackerCreate(uint32_t MaxKey)
{
    uint32_t cbBitmap = (MaxKey + sizeof(uint32_t) * sizeof(uint8_t) - 1) / sizeof(uint8_t);
    PTRACKER pTracker = (PTRACKER)RTMemAllocZ(RT_OFFSETOF(TRACKER, abBitmap[cbBitmap]));
    if (pTracker)
    {
        pTracker->MaxKey = MaxKey;
        pTracker->LastAllocatedKey = MaxKey;
        pTracker->cbBitmap = cbBitmap;
        Assert(pTracker->cSetBits == 0);
    }
    return pTracker;
}


/**
 * Destroys a tracker.
 *
 * @param   pTracker        The tracker.
 */
static void TrackerDestroy(PTRACKER pTracker)
{
    RTMemFree(pTracker);
}


/**
 * Inserts a key range into the tracker.
 *
 * @returns success indicator.
 * @param   pTracker    The tracker.
 * @param   Key         The first key in the range.
 * @param   KeyLast     The last key in the range. (inclusive)
 */
static bool TrackerInsert(PTRACKER pTracker, uint32_t Key, uint32_t KeyLast)
{
    bool fRc = !ASMBitTestAndSet(pTracker->abBitmap, Key);
    if (fRc)
        pTracker->cSetBits++;
    while (KeyLast != Key)
    {
        if (!ASMBitTestAndSet(pTracker->abBitmap, KeyLast))
            pTracker->cSetBits++;
        else
            fRc = false;
        KeyLast--;
    }
    return fRc;
}


/**
 * Removes a key range from the tracker.
 *
 * @returns success indicator.
 * @param   pTracker    The tracker.
 * @param   Key         The first key in the range.
 * @param   KeyLast     The last key in the range. (inclusive)
 */
static bool TrackerRemove(PTRACKER pTracker, uint32_t Key, uint32_t KeyLast)
{
    bool fRc = ASMBitTestAndClear(pTracker->abBitmap, Key);
    if (fRc)
        pTracker->cSetBits--;
    while (KeyLast != Key)
    {
        if (ASMBitTestAndClear(pTracker->abBitmap, KeyLast))
            pTracker->cSetBits--;
        else
            fRc = false;
        KeyLast--;
    }
    return fRc;
}


/**
 * Random key range allocation.
 *
 * @returns success indicator.
 * @param   pTracker    The tracker.
 * @param   pKey        Where to store the first key in the allocated range.
 * @param   pKeyLast    Where to store the first key in the allocated range.
 * @param   cMaxKey     The max range length.
 * @remark  The caller has to call TrackerInsert.
 */
static bool TrackerNewRandomEx(PTRACKER pTracker, uint32_t *pKey, uint32_t *pKeyLast, uint32_t cMaxKeys)
{
    /*
     * Find a key.
     */
    uint32_t Key = RTRandAdvU32Ex(g_hRand, 0, pTracker->MaxKey - 1);
    if (ASMBitTest(pTracker->abBitmap, Key))
    {
        if (pTracker->cSetBits >= pTracker->MaxKey)
            return false;

        int Key2 = ASMBitNextClear(pTracker->abBitmap, pTracker->MaxKey, Key);
        if (Key2 > 0)
            Key = Key2;
        else
        {
            /* we're missing a ASMBitPrevClear function, so just try another, lower, value.*/
            for (;;)
            {
                const uint32_t KeyPrev = Key;
                Key = RTRandAdvU32Ex(g_hRand, 0, KeyPrev - 1);
                if (!ASMBitTest(pTracker->abBitmap, Key))
                    break;
                Key2 = ASMBitNextClear(pTracker->abBitmap, RT_ALIGN_32(KeyPrev, 32), Key);
                if (Key2 > 0)
                {
                    Key = Key2;
                    break;
                }
            }
        }
    }

    /*
     * Determine the range.
     */
    uint32_t KeyLast;
    if (cMaxKeys == 1 || !pKeyLast)
        KeyLast = Key;
    else
    {
        uint32_t cKeys = RTRandAdvU32Ex(g_hRand, 0, RT_MIN(pTracker->MaxKey - Key, cMaxKeys - 1));
        KeyLast = Key + cKeys;
        int Key2 = ASMBitNextSet(pTracker->abBitmap, RT_ALIGN_32(KeyLast, 32), Key);
        if (    Key2 > 0
            &&  (unsigned)Key2 <= KeyLast)
            KeyLast = Key2 - 1;
    }

    /*
     * Return.
     */
    *pKey = Key;
    if (pKeyLast)
        *pKeyLast = KeyLast;
    return true;
}


/**
 * Random single key allocation.
 *
 * @returns success indicator.
 * @param   pTracker    The tracker.
 * @param   pKey        Where to store the allocated key.
 * @remark  The caller has to call TrackerInsert.
 */
static bool TrackerNewRandom(PTRACKER pTracker, uint32_t *pKey)
{
    return TrackerNewRandomEx(pTracker, pKey, NULL, 1);
}


/**
 * Random single key 'lookup'.
 *
 * @returns success indicator.
 * @param   pTracker    The tracker.
 * @param   pKey        Where to store the allocated key.
 * @remark  The caller has to call TrackerRemove.
 */
static bool TrackerFindRandom(PTRACKER pTracker, uint32_t *pKey)
{
    uint32_t Key = RTRandAdvU32Ex(g_hRand, 0, pTracker->MaxKey - 1);
    if (!ASMBitTest(pTracker->abBitmap, Key))
    {
        if (!pTracker->cSetBits)
            return false;

        int Key2 = ASMBitNextSet(pTracker->abBitmap, pTracker->MaxKey, Key);
        if (Key2 > 0)
            Key = Key2;
        else
        {
            /* we're missing a ASMBitPrevSet function, so here's a quick replacement hack. */
            uint32_t *pu32Start = (uint32_t *)&pTracker->abBitmap[0];
            uint32_t *pu32Cur   = (uint32_t *)&pTracker->abBitmap[Key >> 8];
            while (pu32Cur >= pu32Start)
            {
                if (*pu32Cur)
                {
                    *pKey = ASMBitLastSetU32(*pu32Cur) - 1 + (uint32_t)((pu32Cur - pu32Start) * 32);
                    return true;
                }
                pu32Cur--;
            }
            Key2 = ASMBitFirstSet(pTracker->abBitmap, pTracker->MaxKey);
            if (Key2 == -1)
            {
                RTTestIFailed("cSetBits=%u - but ASMBitFirstSet failed to find any", pTracker->cSetBits);
                return false;
            }
            Key = Key2;
        }
    }

    *pKey = Key;
    return true;
}


/*
bool TrackerAllocSeq(PTRACKER pTracker, uint32_t *pKey, uint32_t *pKeyLast, uint32_t cMaxKeys)
{
    return false;
}*/


/**
 * Prints an unbuffered char.
 * @param   ch  The char.
 */
static void ProgressChar(char ch)
{
    //RTTestIPrintf(RTTESTLVL_INFO, "%c", ch);
    RTTestIPrintf(RTTESTLVL_SUB_TEST, "%c", ch);
}

/**
 * Prints a progress indicator label.
 * @param   cMax        The max number of operations (exclusive).
 * @param   pszFormat   The format string.
 * @param   ...         The arguments to the format string.
 */
DECLINLINE(void) ProgressPrintf(unsigned cMax, const char *pszFormat, ...)
{
    if (cMax < 10000)
        return;

    va_list va;
    va_start(va, pszFormat);
    //RTTestIPrintfV(RTTESTLVL_INFO, pszFormat, va);
    RTTestIPrintfV(RTTESTLVL_SUB_TEST, pszFormat, va);
    va_end(va);
}


/**
 * Prints a progress indicator dot.
 * @param   iCur    The current operation. (can be descending too)
 * @param   cMax    The max number of operations (exclusive).
 */
DECLINLINE(void) Progress(unsigned iCur, unsigned cMax)
{
    if (cMax < 10000)
        return;
    if (!(iCur % (cMax / 20)))
        ProgressChar('.');
}


static int avlogcphys(unsigned cMax)
{
    /*
     * Simple linear insert and remove.
     */
    if (cMax >= 10000)
        RTTestISubF("oGCPhys(%d): linear left", cMax);
    PAVLOGCPHYSTREE pTree = (PAVLOGCPHYSTREE)RTMemAllocZ(sizeof(*pTree));
    unsigned i;
    for (i = 0; i < cMax; i++)
    {
        Progress(i, cMax);
        PAVLOGCPHYSNODECORE pNode = (PAVLOGCPHYSNODECORE)RTMemAlloc(sizeof(*pNode));
        pNode->Key = i;
        if (!RTAvloGCPhysInsert(pTree, pNode))
        {
            RTTestIFailed("linear left insert i=%d\n", i);
            return 1;
        }
        /* negative. */
        AVLOGCPHYSNODECORE Node = *pNode;
        if (RTAvloGCPhysInsert(pTree, &Node))
        {
            RTTestIFailed("linear left negative insert i=%d\n", i);
            return 1;
        }
    }

    ProgressPrintf(cMax, "~");
    for (i = 0; i < cMax; i++)
    {
        Progress(i, cMax);
        PAVLOGCPHYSNODECORE pNode = RTAvloGCPhysRemove(pTree, i);
        if (!pNode)
        {
            RTTestIFailed("linear left remove i=%d\n", i);
            return 1;
        }
        memset(pNode, 0xcc, sizeof(*pNode));
        RTMemFree(pNode);

        /* negative */
        pNode = RTAvloGCPhysRemove(pTree, i);
        if (pNode)
        {
            RTTestIFailed("linear left negative remove i=%d\n", i);
            return 1;
        }
    }

    /*
     * Simple linear insert and remove from the right.
     */
    if (cMax >= 10000)
        RTTestISubF("oGCPhys(%d): linear right", cMax);
    for (i = 0; i < cMax; i++)
    {
        Progress(i, cMax);
        PAVLOGCPHYSNODECORE pNode = (PAVLOGCPHYSNODECORE)RTMemAlloc(sizeof(*pNode));
        pNode->Key = i;
        if (!RTAvloGCPhysInsert(pTree, pNode))
        {
            RTTestIFailed("linear right insert i=%d\n", i);
            return 1;
        }
        /* negative. */
        AVLOGCPHYSNODECORE Node = *pNode;
        if (RTAvloGCPhysInsert(pTree, &Node))
        {
            RTTestIFailed("linear right negative insert i=%d\n", i);
            return 1;
        }
    }

    ProgressPrintf(cMax, "~");
    while (i-- > 0)
    {
        Progress(i, cMax);
        PAVLOGCPHYSNODECORE pNode = RTAvloGCPhysRemove(pTree, i);
        if (!pNode)
        {
            RTTestIFailed("linear right remove i=%d\n", i);
            return 1;
        }
        memset(pNode, 0xcc, sizeof(*pNode));
        RTMemFree(pNode);

        /* negative */
        pNode = RTAvloGCPhysRemove(pTree, i);
        if (pNode)
        {
            RTTestIFailed("linear right negative remove i=%d\n", i);
            return 1;
        }
    }

    /*
     * Linear insert but root based removal.
     */
    if (cMax >= 10000)
        RTTestISubF("oGCPhys(%d): linear root", cMax);
    for (i = 0; i < cMax; i++)
    {
        Progress(i, cMax);
        PAVLOGCPHYSNODECORE pNode = (PAVLOGCPHYSNODECORE)RTMemAlloc(sizeof(*pNode));
        pNode->Key = i;
        if (!RTAvloGCPhysInsert(pTree, pNode))
        {
            RTTestIFailed("linear root insert i=%d\n", i);
            return 1;
        }
        /* negative. */
        AVLOGCPHYSNODECORE Node = *pNode;
        if (RTAvloGCPhysInsert(pTree, &Node))
        {
            RTTestIFailed("linear root negative insert i=%d\n", i);
            return 1;
        }
    }

    ProgressPrintf(cMax, "~");
    while (i-- > 0)
    {
        Progress(i, cMax);
        PAVLOGCPHYSNODECORE pNode = (PAVLOGCPHYSNODECORE)((intptr_t)pTree + *pTree);
        RTGCPHYS Key = pNode->Key;
        pNode = RTAvloGCPhysRemove(pTree, Key);
        if (!pNode)
        {
            RTTestIFailed("linear root remove i=%d Key=%d\n", i, (unsigned)Key);
            return 1;
        }
        memset(pNode, 0xcc, sizeof(*pNode));
        RTMemFree(pNode);

        /* negative */
        pNode = RTAvloGCPhysRemove(pTree, Key);
        if (pNode)
        {
            RTTestIFailed("linear root negative remove i=%d Key=%d\n", i, (unsigned)Key);
            return 1;
        }
    }
    if (*pTree)
    {
        RTTestIFailed("sparse remove didn't remove it all!\n");
        return 1;
    }

    /*
     * Make a sparsely populated tree and remove the nodes using best fit in 5 cycles.
     */
    const unsigned cMaxSparse = RT_ALIGN(cMax, 32);
    if (cMaxSparse >= 10000)
        RTTestISubF("oGCPhys(%d): sparse", cMax);
    for (i = 0; i < cMaxSparse; i += 8)
    {
        Progress(i, cMaxSparse);
        PAVLOGCPHYSNODECORE pNode = (PAVLOGCPHYSNODECORE)RTMemAlloc(sizeof(*pNode));
        pNode->Key = i;
        if (!RTAvloGCPhysInsert(pTree, pNode))
        {
            RTTestIFailed("sparse insert i=%d\n", i);
            return 1;
        }
        /* negative. */
        AVLOGCPHYSNODECORE Node = *pNode;
        if (RTAvloGCPhysInsert(pTree, &Node))
        {
            RTTestIFailed("sparse negative insert i=%d\n", i);
            return 1;
        }
    }

    /* Remove using best fit in 5 cycles. */
    ProgressPrintf(cMaxSparse, "~");
    unsigned j;
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < cMaxSparse; i += 8 * 4)
        {
            Progress(i, cMax); // good enough
            PAVLOGCPHYSNODECORE pNode = RTAvloGCPhysRemoveBestFit(pTree, i, true);
            if (!pNode)
            {
                RTTestIFailed("sparse remove i=%d j=%d\n", i, j);
                return 1;
            }
            if (pNode->Key - (unsigned long)i >= 8 * 4)
            {
                RTTestIFailed("sparse remove i=%d j=%d Key=%d\n", i, j, (unsigned)pNode->Key);
                return 1;
            }
            memset(pNode, 0xdd, sizeof(*pNode));
            RTMemFree(pNode);
        }
    }
    if (*pTree)
    {
        RTTestIFailed("sparse remove didn't remove it all!\n");
        return 1;
    }
    RTMemFree(pTree);
    ProgressPrintf(cMaxSparse, "\n");
    return 0;
}


int avlogcphysRand(unsigned cMax, unsigned cMax2)
{
    PAVLOGCPHYSTREE pTree = (PAVLOGCPHYSTREE)RTMemAllocZ(sizeof(*pTree));
    unsigned i;

    /*
     * Random tree.
     */
    if (cMax >= 10000)
        RTTestISubF("oGCPhys(%d, %d): random", cMax, cMax2);
    PTRACKER pTracker = TrackerCreate(cMax2);
    if (!pTracker)
    {
        RTTestIFailed("failed to create %d tracker!\n", cMax2);
        return 1;
    }

    /* Insert a number of nodes in random order. */
    for (i = 0; i < cMax; i++)
    {
        Progress(i, cMax);
        uint32_t Key;
        if (!TrackerNewRandom(pTracker, &Key))
        {
            RTTestIFailed("failed to allocate node no. %d\n", i);
            TrackerDestroy(pTracker);
            return 1;
        }
        PAVLOGCPHYSNODECORE pNode = (PAVLOGCPHYSNODECORE)RTMemAlloc(sizeof(*pNode));
        pNode->Key = Key;
        if (!RTAvloGCPhysInsert(pTree, pNode))
        {
            RTTestIFailed("random insert i=%d Key=%#x\n", i, Key);
            return 1;
        }
        /* negative. */
        AVLOGCPHYSNODECORE Node = *pNode;
        if (RTAvloGCPhysInsert(pTree, &Node))
        {
            RTTestIFailed("linear negative insert i=%d Key=%#x\n", i, Key);
            return 1;
        }
        TrackerInsert(pTracker, Key, Key);
    }


    /* delete the nodes in random order. */
    ProgressPrintf(cMax, "~");
    while (i-- > 0)
    {
        Progress(i, cMax);
        uint32_t Key;
        if (!TrackerFindRandom(pTracker, &Key))
        {
            RTTestIFailed("failed to find free node no. %d\n", i);
            TrackerDestroy(pTracker);
            return 1;
        }

        PAVLOGCPHYSNODECORE pNode = RTAvloGCPhysRemove(pTree, Key);
        if (!pNode)
        {
            RTTestIFailed("random remove i=%d Key=%#x\n", i, Key);
            return 1;
        }
        if (pNode->Key != Key)
        {
            RTTestIFailed("random remove i=%d Key=%#x pNode->Key=%#x\n", i, Key, (unsigned)pNode->Key);
            return 1;
        }
        TrackerRemove(pTracker, Key, Key);
        memset(pNode, 0xdd, sizeof(*pNode));
        RTMemFree(pNode);
    }
    if (*pTree)
    {
        RTTestIFailed("random remove didn't remove it all!\n");
        return 1;
    }
    ProgressPrintf(cMax, "\n");
    TrackerDestroy(pTracker);
    RTMemFree(pTree);
    return 0;
}



int avlrogcphys(void)
{
    unsigned            i;
    unsigned            j;
    unsigned            k;
    PAVLROGCPHYSTREE    pTree = (PAVLROGCPHYSTREE)RTMemAllocZ(sizeof(*pTree));

    AssertCompileSize(AVLOGCPHYSNODECORE, 24);
    AssertCompileSize(AVLROGCPHYSNODECORE, 32);

    RTTestISubF("RTAvlroGCPhys");

    /*
     * Simple linear insert, get and remove.
     */
    /* insert */
    for (i = 0; i < 65536; i += 4)
    {
        PAVLROGCPHYSNODECORE pNode = (PAVLROGCPHYSNODECORE)RTMemAlloc(sizeof(*pNode));
        pNode->Key = i;
        pNode->KeyLast = i + 3;
        if (!RTAvlroGCPhysInsert(pTree, pNode))
        {
            RTTestIFailed("linear insert i=%d\n", (unsigned)i);
            return 1;
        }

        /* negative. */
        AVLROGCPHYSNODECORE Node = *pNode;
        for (j = i + 3; j > i - 32; j--)
        {
            for (k = i; k < i + 32; k++)
            {
                Node.Key = RT_MIN(j, k);
                Node.KeyLast = RT_MAX(k, j);
                if (RTAvlroGCPhysInsert(pTree, &Node))
                {
                    RTTestIFailed("linear negative insert i=%d j=%d k=%d\n", i, j, k);
                    return 1;
                }
            }
        }
    }

    /* do gets. */
    for (i = 0; i < 65536; i += 4)
    {
        PAVLROGCPHYSNODECORE pNode = RTAvlroGCPhysGet(pTree, i);
        if (!pNode)
        {
            RTTestIFailed("linear get i=%d\n", i);
            return 1;
        }
        if (pNode->Key > i || pNode->KeyLast < i)
        {
            RTTestIFailed("linear get i=%d Key=%d KeyLast=%d\n", i, (unsigned)pNode->Key, (unsigned)pNode->KeyLast);
            return 1;
        }

        for (j = 0; j < 4; j++)
        {
            if (RTAvlroGCPhysRangeGet(pTree, i + j) != pNode)
            {
                RTTestIFailed("linear range get i=%d j=%d\n", i, j);
                return 1;
            }
        }

        /* negative. */
        if (    RTAvlroGCPhysGet(pTree, i + 1)
            ||  RTAvlroGCPhysGet(pTree, i + 2)
            ||  RTAvlroGCPhysGet(pTree, i + 3))
        {
            RTTestIFailed("linear negative get i=%d + n\n", i);
            return 1;
        }

    }

    /* remove */
    for (i = 0; i < 65536; i += 4)
    {
        PAVLROGCPHYSNODECORE pNode = RTAvlroGCPhysRemove(pTree, i);
        if (!pNode)
        {
            RTTestIFailed("linear remove i=%d\n", i);
            return 1;
        }
        memset(pNode, 0xcc, sizeof(*pNode));
        RTMemFree(pNode);

        /* negative */
        if (    RTAvlroGCPhysRemove(pTree, i)
            ||  RTAvlroGCPhysRemove(pTree, i + 1)
            ||  RTAvlroGCPhysRemove(pTree, i + 2)
            ||  RTAvlroGCPhysRemove(pTree, i + 3))
        {
            RTTestIFailed("linear negative remove i=%d + n\n", i);
            return 1;
        }
    }

    /*
     * Make a sparsely populated tree.
     */
    for (i = 0; i < 65536; i += 8)
    {
        PAVLROGCPHYSNODECORE pNode = (PAVLROGCPHYSNODECORE)RTMemAlloc(sizeof(*pNode));
        pNode->Key = i;
        pNode->KeyLast = i + 3;
        if (!RTAvlroGCPhysInsert(pTree, pNode))
        {
            RTTestIFailed("sparse insert i=%d\n", i);
            return 1;
        }
        /* negative. */
        AVLROGCPHYSNODECORE Node = *pNode;
        const RTGCPHYS jMin = i > 32 ? i - 32 : 1;
        const RTGCPHYS kMax = i + 32;
        for (j = pNode->KeyLast; j >= jMin; j--)
        {
            for (k = pNode->Key; k < kMax; k++)
            {
                Node.Key = RT_MIN(j, k);
                Node.KeyLast = RT_MAX(k, j);
                if (RTAvlroGCPhysInsert(pTree, &Node))
                {
                    RTTestIFailed("sparse negative insert i=%d j=%d k=%d\n", i, j, k);
                    return 1;
                }
            }
        }
    }

    /*
     * Get and Remove using range matching in 5 cycles.
     */
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 65536; i += 8 * 4)
        {
            /* gets */
            RTGCPHYS  KeyBase = i + j * 8;
            PAVLROGCPHYSNODECORE pNode = RTAvlroGCPhysGet(pTree, KeyBase);
            if (!pNode)
            {
                RTTestIFailed("sparse get i=%d j=%d KeyBase=%d\n", i, j, (unsigned)KeyBase);
                return 1;
            }
            if (pNode->Key > KeyBase || pNode->KeyLast < KeyBase)
            {
                RTTestIFailed("sparse get i=%d j=%d KeyBase=%d pNode->Key=%d\n", i, j, (unsigned)KeyBase, (unsigned)pNode->Key);
                return 1;
            }
            for (k = KeyBase; k < KeyBase + 4; k++)
            {
                if (RTAvlroGCPhysRangeGet(pTree, k) != pNode)
                {
                    RTTestIFailed("sparse range get i=%d j=%d k=%d\n", i, j, k);
                    return 1;
                }
            }

            /* negative gets */
            for (k = i + j; k < KeyBase + 8; k++)
            {
                if (    k != KeyBase
                    &&  RTAvlroGCPhysGet(pTree, k))
                {
                    RTTestIFailed("sparse negative get i=%d j=%d k=%d\n", i, j, k);
                    return 1;
                }
            }
            for (k = i + j; k < KeyBase; k++)
            {
                if (RTAvlroGCPhysRangeGet(pTree, k))
                {
                    RTTestIFailed("sparse negative range get i=%d j=%d k=%d\n", i, j, k);
                    return 1;
                }
            }
            for (k = KeyBase + 4; k < KeyBase + 8; k++)
            {
                if (RTAvlroGCPhysRangeGet(pTree, k))
                {
                    RTTestIFailed("sparse negative range get i=%d j=%d k=%d\n", i, j, k);
                    return 1;
                }
            }

            /* remove */
            RTGCPHYS Key = KeyBase + ((i / 19) % 4);
            if (RTAvlroGCPhysRangeRemove(pTree, Key) != pNode)
            {
                RTTestIFailed("sparse remove i=%d j=%d Key=%d\n", i, j, (unsigned)Key);
                return 1;
            }
            memset(pNode, 0xdd, sizeof(*pNode));
            RTMemFree(pNode);
        }
    }
    if (*pTree)
    {
        RTTestIFailed("sparse remove didn't remove it all!\n");
        return 1;
    }


    /*
     * Realworld testcase.
     */
    struct
    {
        AVLROGCPHYSTREE     Tree;
        AVLROGCPHYSNODECORE aNode[4];
    }   s1, s2, s3;
    RT_ZERO(s1);
    RT_ZERO(s2);
    RT_ZERO(s3);

    s1.aNode[0].Key        = 0x00030000;
    s1.aNode[0].KeyLast    = 0x00030fff;
    s1.aNode[1].Key        = 0x000a0000;
    s1.aNode[1].KeyLast    = 0x000bffff;
    s1.aNode[2].Key        = 0xe0000000;
    s1.aNode[2].KeyLast    = 0xe03fffff;
    s1.aNode[3].Key        = 0xfffe0000;
    s1.aNode[3].KeyLast    = 0xfffe0ffe;
    for (i = 0; i < RT_ELEMENTS(s1.aNode); i++)
    {
        PAVLROGCPHYSNODECORE pNode = &s1.aNode[i];
        if (!RTAvlroGCPhysInsert(&s1.Tree, pNode))
        {
            RTTestIFailed("real insert i=%d\n", i);
            return 1;
        }
        if (RTAvlroGCPhysInsert(&s1.Tree, pNode))
        {
            RTTestIFailed("real negative insert i=%d\n", i);
            return 1;
        }
        if (RTAvlroGCPhysGet(&s1.Tree, pNode->Key) != pNode)
        {
            RTTestIFailed("real get (1) i=%d\n", i);
            return 1;
        }
        if (RTAvlroGCPhysGet(&s1.Tree, pNode->KeyLast) != NULL)
        {
            RTTestIFailed("real negative get (2) i=%d\n", i);
            return 1;
        }
        if (RTAvlroGCPhysRangeGet(&s1.Tree, pNode->Key) != pNode)
        {
            RTTestIFailed("real range get (1) i=%d\n", i);
            return 1;
        }
        if (RTAvlroGCPhysRangeGet(&s1.Tree, pNode->Key + 1) != pNode)
        {
            RTTestIFailed("real range get (2) i=%d\n", i);
            return 1;
        }
        if (RTAvlroGCPhysRangeGet(&s1.Tree, pNode->KeyLast) != pNode)
        {
            RTTestIFailed("real range get (3) i=%d\n", i);
            return 1;
        }
    }

    s3 = s1;
    s1 = s2;
    for (i = 0; i < RT_ELEMENTS(s3.aNode); i++)
    {
        PAVLROGCPHYSNODECORE pNode = &s3.aNode[i];
        if (RTAvlroGCPhysGet(&s3.Tree, pNode->Key) != pNode)
        {
            RTTestIFailed("real get (10) i=%d\n", i);
            return 1;
        }
        if (RTAvlroGCPhysRangeGet(&s3.Tree, pNode->Key) != pNode)
        {
            RTTestIFailed("real range get (10) i=%d\n", i);
            return 1;
        }

        j = pNode->Key + 1;
        do
        {
            if (RTAvlroGCPhysGet(&s3.Tree, j) != NULL)
            {
                RTTestIFailed("real negative get (11) i=%d j=%#x\n", i, j);
                return 1;
            }
            if (RTAvlroGCPhysRangeGet(&s3.Tree, j) != pNode)
            {
                RTTestIFailed("real range get (11) i=%d j=%#x\n", i, j);
                return 1;
            }
        } while (j++ < pNode->KeyLast);
    }

    return 0;
}


int avlul(void)
{
    RTTestISubF("RTAvlUL");

    /*
     * Simple linear insert and remove.
     */
    PAVLULNODECORE  pTree = 0;
    unsigned i;
    /* insert */
    for (i = 0; i < 65536; i++)
    {
        PAVLULNODECORE pNode = (PAVLULNODECORE)RTMemAlloc(sizeof(*pNode));
        pNode->Key = i;
        if (!RTAvlULInsert(&pTree, pNode))
        {
            RTTestIFailed("linear insert i=%d\n", i);
            return 1;
        }
        /* negative. */
        AVLULNODECORE Node = *pNode;
        if (RTAvlULInsert(&pTree, &Node))
        {
            RTTestIFailed("linear negative insert i=%d\n", i);
            return 1;
        }
    }

    for (i = 0; i < 65536; i++)
    {
        PAVLULNODECORE pNode = RTAvlULRemove(&pTree, i);
        if (!pNode)
        {
            RTTestIFailed("linear remove i=%d\n", i);
            return 1;
        }
        pNode->pLeft     = (PAVLULNODECORE)(uintptr_t)0xaaaaaaaa;
        pNode->pRight    = (PAVLULNODECORE)(uintptr_t)0xbbbbbbbb;
        pNode->uchHeight = 'e';
        RTMemFree(pNode);

        /* negative */
        pNode = RTAvlULRemove(&pTree, i);
        if (pNode)
        {
            RTTestIFailed("linear negative remove i=%d\n", i);
            return 1;
        }
    }

    /*
     * Make a sparsely populated tree.
     */
    for (i = 0; i < 65536; i += 8)
    {
        PAVLULNODECORE pNode = (PAVLULNODECORE)RTMemAlloc(sizeof(*pNode));
        pNode->Key = i;
        if (!RTAvlULInsert(&pTree, pNode))
        {
            RTTestIFailed("linear insert i=%d\n", i);
            return 1;
        }
        /* negative. */
        AVLULNODECORE Node = *pNode;
        if (RTAvlULInsert(&pTree, &Node))
        {
            RTTestIFailed("linear negative insert i=%d\n", i);
            return 1;
        }
    }

    /*
     * Remove using best fit in 5 cycles.
     */
    unsigned j;
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 65536; i += 8 * 4)
        {
            PAVLULNODECORE pNode = RTAvlULRemoveBestFit(&pTree, i, true);
            //PAVLULNODECORE pNode = RTAvlULRemove(&pTree, i + j * 8);
            if (!pNode)
            {
                RTTestIFailed("sparse remove i=%d j=%d\n", i, j);
                return 1;
            }
            pNode->pLeft     = (PAVLULNODECORE)(uintptr_t)0xdddddddd;
            pNode->pRight    = (PAVLULNODECORE)(uintptr_t)0xcccccccc;
            pNode->uchHeight = 'E';
            RTMemFree(pNode);
        }
    }

    return 0;
}


int main()
{
    /*
     * Init.
     */
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstRTAvl", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);
    g_hTest = hTest;

    rc = RTRandAdvCreateParkMiller(&g_hRand);
    if (RT_FAILURE(rc))
    {
        RTTestIFailed("RTRandAdvCreateParkMiller -> %Rrc", rc);
        return RTTestSummaryAndDestroy(hTest);
    }

    /*
     * Testing.
     */
    unsigned i;
    RTTestSub(hTest, "oGCPhys(32..2048)");
    for (i = 32; i < 2048; i++)
        if (avlogcphys(i))
            break;

    avlogcphys(_64K);
    avlogcphys(_512K);
    avlogcphys(_4M);

    RTTestISubF("oGCPhys(32..2048, *1K)");
    for (i = 32; i < 4096; i++)
        if (avlogcphysRand(i, i + _1K))
            break;
    for (; i <= _4M; i *= 2)
        if (avlogcphysRand(i, i * 8))
            break;

    avlrogcphys();
    avlul();

    /*
     * Done.
     */
    return RTTestSummaryAndDestroy(hTest);
}

