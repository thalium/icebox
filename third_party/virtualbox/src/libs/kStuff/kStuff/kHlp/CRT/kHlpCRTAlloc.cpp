/* $Id: kHlpCRTAlloc.cpp 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kHlpAlloc - Memory Allocation, CRT based implementation.
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

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <k/kHlpAlloc.h>
#include <stdlib.h>
#include <string.h>


KHLP_DECL(void *) kHlpAlloc(KSIZE cb)
{
    return malloc(cb);
}


KHLP_DECL(void *) kHlpAllocZ(KSIZE cb)
{
    return calloc(1, cb);
}


KHLP_DECL(void *) kHlpDup(const void *pv, KSIZE cb)
{
    void *pvDup = kHlpAlloc(cb);
    if (pvDup)
        return memcpy(pvDup, pv, cb);
    return NULL;
}


KHLP_DECL(char *) kHlpStrDup(const char *psz)
{
    size_t cb = strlen(psz) + 1;
    return (char *)kHlpDup(psz, cb);
}


KHLP_DECL(void *) kHlpRealloc(void *pv, KSIZE cb)
{
    return realloc(pv, cb);
}


KHLP_DECL(void) kHlpFree(void *pv)
{
    if (pv)
        free(pv);
}

