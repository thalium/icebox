/* $Id: prfcoreterm.cpp.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kProfiler Mark 2 - Core Termination Code Template.
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
 * Unwinds and terminates all the threads, and frees the stack space.
 *
 * @returns The new data set size. (pHdr->cb)
 * @param   pHdr        The profiler data set header.
 */
KPRF_DECL_FUNC(KU32, TerminateAll)(KPRF_TYPE(P,HDR) pHdr)
{
    KU64 TS = KPRF_NOW();
    if (!pHdr)
        return 0;

    /*
     * Iterate the threads and terminate all which are non-terminated.
     */
    KPRF_TYPE(P,THREAD) paThread = KPRF_OFF2PTR(P,THREAD, pHdr->offThreads, pHdr);
    for (KU32 i = 0; i < pHdr->cThreads; i++)
    {
        KPRF_TYPE(P,THREAD) pCur = &paThread[i];
        switch (pCur->enmState)
        {
            /* these states needs no work. */
            case KPRF_TYPE(,THREADSTATE_TERMINATED):
            case KPRF_TYPE(,THREADSTATE_UNUSED):
            default:
                break;

            /* these are active and requires unwinding.*/
            case KPRF_TYPE(,THREADSTATE_ACTIVE):
            case KPRF_TYPE(,THREADSTATE_SUSPENDED):
            case KPRF_TYPE(,THREADSTATE_OVERFLOWED):
                KPRF_NAME(TerminateThread)(pHdr, pCur, TS);
                break;
        }
    }


    /*
     * Free the stacks.
     */
    if (pHdr->offStacks)
    {
        /* only if the stack is at the end of the data set. */
        const KU32 cbStacks = KPRF_ALIGN(pHdr->cMaxStacks * pHdr->cbStack, 32);
        if (pHdr->offStacks + cbStacks == pHdr->cb)
            pHdr->cb -= cbStacks;
        pHdr->offStacks = 0;
    }

    return pHdr->cb;
}


/**
 * Sets the commandline.
 *
 * This is typically done after TerminateAll, when the stacks has
 * been freed up and there is plenty free space.
 *
 * @returns The new data set size. (pHdr->cb)
 * @param   pHdr        The profiler data set header.
 * @param   cArgs       The number of arguments in the array.
 * @param   papszArgs   Pointer to an array of arguments.
 */
KPRF_DECL_FUNC(KU32, SetCommandLine)(KPRF_TYPE(P,HDR) pHdr, unsigned cArgs, const char * const *papszArgs)
{
    if (!pHdr)
        return 0;

    /*
     * Any space at all?
     */
    if (pHdr->cb + 16 > pHdr->cbAllocated) /* 16 bytes min */
        return pHdr->cb;

    /*
     * Encode untill we run out of space.
     */
    pHdr->offCommandLine = pHdr->cb;
    char *psz = (char *)pHdr + pHdr->cb;
    char *pszMax = (char *)pHdr + pHdr->cbAllocated - 1;
    for (unsigned i = 0; i < cArgs && psz + 7 < pszMax; i++)
    {
        if (i > 0)
            *psz++ = ' ';
        *psz++ = '\'';
        const char *pszArg = papszArgs[i];
        while (psz < pszMax)
        {
            char ch = *pszArg++;
            if (!ch)
                break;
            if (ch == '\'')
            {
                if (psz + 1 >= pszMax)
                    break;
                *psz++ = '\\';
            }
            *psz++ = ch;
        }
        if (psz < pszMax)
            *psz++ = '\'';
    }
    *psz++ = '\0';
    pHdr->cb = psz - (char *)pHdr;
    pHdr->cchCommandLine = pHdr->cb - pHdr->offCommandLine - 1;

    return pHdr->cb;
}


