/* $Id: prfcorefunction.cpp.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kProfiler Mark 2 - Core NewFunction Code Template.
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


/**
 * Creates a new function.
 *
 * @returns Pointer to the new function.
 * @returns NULL if we're out of space.
 */
static KPRF_TYPE(P,FUNC) KPRF_NAME(NewFunction)(KPRF_TYPE(P,HDR) pHdr,KPRF_TYPE(,UPTR) uPC)
{
    /*
     * First find the position of the function (it might actually have been inserted by someone else by now too).
     */
    KPRF_FUNCS_WRITE_LOCK();

    KPRF_TYPE(P,FUNC) paFunctions = KPRF_OFF2PTR(P,FUNC, pHdr->offFunctions, pHdr);
    KI32 iStart = 0;
    KI32 iLast  = pHdr->cFunctions - 1;
    KI32 i      = iLast / 2;
    for (;;)
    {
        KU32 iFunction = pHdr->aiFunctions[i];
        KPRF_TYPE(,IPTR) iDiff = uPC - paFunctions[iFunction].uEntryPtr;
        if (!iDiff)
        {
            KPRF_FUNCS_WRITE_UNLOCK();
            return &paFunctions[iFunction];
        }
        if (iLast == iStart)
            break;
        if (iDiff < 0)
            iLast = i - 1;
        else
            iStart = i + 1;
        if (iLast < iStart)
            break;
        i = iStart + (iLast - iStart) / 2;
    }

    /*
     * Adjust the index so we're exactly in the right spot.
     * (I've too much of a headache to figure out if the above loop leaves us where we should be.)
     */
    const KI32 iNew = pHdr->cFunctions;
    if (paFunctions[pHdr->aiFunctions[i]].uEntryPtr > uPC)
    {
        while (     i > 0
               &&   paFunctions[pHdr->aiFunctions[i - 1]].uEntryPtr > uPC)
            i--;
    }
    else
    {
        while (     i < iNew
               &&   paFunctions[pHdr->aiFunctions[i]].uEntryPtr < uPC)
            i++;
    }

    /*
     * Ensure that there still is space for the function.
     */
    if (iNew >= (KI32)pHdr->cMaxFunctions)
    {
        KPRF_FUNCS_WRITE_UNLOCK();
        return NULL;
    }
    pHdr->cFunctions++;
    KPRF_TYPE(P,FUNC) pNew = &paFunctions[iNew];

    /* init the new function entry */
    pNew->uEntryPtr  = uPC;
    pNew->offModSeg  = 0;
    pNew->cOnStack   = 0;
    pNew->cCalls     = 0;
    pNew->OnStack.MinTicks      = ~(KU64)0;
    pNew->OnStack.MaxTicks      = 0;
    pNew->OnStack.SumTicks      = 0;
    pNew->OnTopOfStack.MinTicks = ~(KU64)0;
    pNew->OnTopOfStack.MaxTicks = 0;
    pNew->OnTopOfStack.SumTicks = 0;

    /* shift the function index array and insert the new one. */
    KI32 j = iNew;
    while (j > i)
    {
        pHdr->aiFunctions[j] = pHdr->aiFunctions[j - 1];
        j--;
    }
    pHdr->aiFunctions[i] = iNew;
    KPRF_FUNCS_WRITE_UNLOCK();

    /*
     * Record the module segment (i.e. add it if it's new).
     */
    pNew->offModSeg = KPRF_NAME(RecordModSeg)(pHdr, uPC);

    return pNew;
}

