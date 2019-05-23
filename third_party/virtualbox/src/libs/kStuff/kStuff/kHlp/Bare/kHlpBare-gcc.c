/* $Id: kHlpBare-gcc.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kHlpBare - The Dynamic Loader, Helper Functions for GCC.
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

#include <k/kLdr.h>
#include "kHlp.h"


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/


void *memchr(const void *pv, int ch, KSIZE cb)
{
    const char *pb = pv;
    while (cb-- > 0)
    {
        if (*pb == ch)
            return (void *)pb;
        pb++;
    }
    return 0;
}


int memcmp(const void *pv1, const void *pv2, KSIZE cb)
{
    /*
     * Pointer size pointer size.
     */
    if (    cb > 16
        &&  !((KUPTR)pv1 & (sizeof(void *) - 1))
        &&  !((KUPTR)pv2 & (sizeof(void *) - 1)) )
    {
        const KUPTR *pu1 = pv1;
        const KUPTR *pu2 = pv2;
        while (cb >= sizeof(KUPTR))
        {
            const KUPTR u1 = *pu1++;
            const KUPTR u2 = *pu2++;
            if (u1 != u2)
                return u1 > u2 ? 1 : -1;
            cb -= sizeof(KUPTR);
        }
        if (!cb)
            return 0;
        pv1 = (const void *)pu1;
        pv2 = (const void *)pu2;
    }

    /*
     * Byte by byte.
     */
    if (cb)
    {
        const unsigned char *pb1 = pv1;
        const unsigned char *pb2 = pv2;
        while (cb-- > 0)
        {
            const unsigned char b1 = *pb1++;
            const unsigned char b2 = *pb2++;
            if (b1 != b2)
                return b1 > b2 ? 1 : -1;
        }
    }
    return 0;
}


void *memcpy(void *pv1, const void *pv2, KSIZE cb)
{
    void *pv1Start = pv1;

    /*
     * Pointer size pointer size.
     */
    if (    cb > 16
        &&  !((KUPTR)pv1 & (sizeof(void *) - 1))
        &&  !((KUPTR)pv2 & (sizeof(void *) - 1)) )
    {
        KUPTR       *pu1 = pv1;
        const KUPTR *pu2 = pv2;
        while (cb >= sizeof(KUPTR))
        {
            cb -= sizeof(KUPTR);
            *pu1++ = *pu2++;
        }
        if (!cb)
            return 0;
        pv1 = (void *)pu1;
        pv2 = (const void *)pu2;
    }

    /*
     * byte by byte
     */
    if (cb)
    {
        unsigned char       *pb1 = pv1;
        const unsigned char *pb2 = pv2;
        while (cb-- > 0)
            *pb1++ = *pb2++;
    }

    return pv1Start;
}

void *memset(void *pv, int ch, KSIZE cb)
{
    void *pvStart = pv;

    /*
     * Pointer size pointer size.
     */
    if (    cb > 16
        &&  !((KUPTR)pv & (sizeof(void *) - 1)))
    {
        KUPTR  *pu = pv;
        KUPTR   u = ch | (ch << 8);
        u |= u << 16;
#if K_ARCH_BITS >= 64
        u |= u << 32;
#endif
#if K_ARCH_BITS >= 128
        u |= u << 64;
#endif

        while (cb >= sizeof(KUPTR))
        {
            cb -= sizeof(KUPTR);
            *pu++ = u;
        }
    }

    /*
     * Byte by byte
     */
    if (cb)
    {
        unsigned char *pb = pv;
        while (cb-- > 0)
            *pb++ = ch;
    }
    return pvStart;
}


int strcmp(const char *psz1, const char *psz2)
{
    for (;;)
    {
        const char ch1 = *psz1++;
        const char ch2 = *psz2++;
        if (ch1 != ch2)
            return (int)ch1 - (int)ch2;
        if (!ch1)
            return 0;
    }
}


int strncmp(const char *psz1, const char *psz2, KSIZE cch)
{
    while (cch-- > 0)
    {
        const char ch1 = *psz1++;
        const char ch2 = *psz2++;
        if (ch1 != ch2)
            return (int)ch1 - (int)ch2;
        if (!ch1)
            break;
    }
    return 0;
}

char *strchr(const char *psz, int ch)
{
    for (;;)
    {
        const char chCur = *psz;
        if (chCur == ch)
            return (char *)psz;
        if (!chCur)
            return 0;
        psz++;
    }
}

KSIZE strlen(const char *psz)
{
    const char *pszStart = psz;
    while (*psz)
        psz++;
    return psz - pszStart;
}

