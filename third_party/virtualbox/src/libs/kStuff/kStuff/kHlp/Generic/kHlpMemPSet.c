/* $Id: kHlpMemPSet.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kHlpString - kHlpMemPSet.
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
#include <k/kHlpString.h>


KHLP_DECL(void *) kHlpMemPSet(void *pv1, int ch, KSIZE cb)
{
    union
    {
        void *pv;
        KU8 *pb;
        KUPTR *pu;
    } u1;

    u1.pv = pv1;

    if (cb >= 32)
    {
        KUPTR u = ch & 0xff;
#if K_ARCH_BITS > 8
        u |= u << 8;
#endif
#if K_ARCH_BITS > 16
        u |= u << 16;
#endif
#if K_ARCH_BITS > 32
        u |= u << 32;
#endif
#if K_ARCH_BITS > 64
        u |= u << 64;
#endif

        while (cb > sizeof(KUPTR))
        {
            cb -= sizeof(KUPTR);
            *u1.pu++ = u;
        }
    }

    while (cb-- > 0)
        *u1.pb++ = ch;

    return u1.pb;
}


