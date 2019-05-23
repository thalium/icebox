/* $Id: tstkLdrHeap.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kLdr - Heap testcase.
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
#include <k/kLdr.h>
#include <k/kHlp.h>

#include <stdio.h>
#include <stdlib.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#define CHECK_FATAL(expr) \
    do { if (!(expr)) { printf("tstkLdrHeap(%d): FATAL FAILURE - %s\n", __LINE__, #expr); return 1; } \
    } while (0)

#define CHECK(expr) \
    do { if (!(expr)) { printf("tstkLdrHeap(%d): ERROR - %s\n", __LINE__, #expr); cErrors++; kHlpAssertBreakpoint();} \
    } while (0)


/**
 * Get a random size.
 * @returns random size.
 */
static unsigned RandSize(void)
{
    unsigned i = (unsigned)rand() % (256*1024 - 1);
    return i ? i : 1;
}

/**
 * Get a random index.
 * @returns random index.
 * @param   cEntries    The number of entries in the table.
 */
static unsigned RandIdx(unsigned cEntries)
{
    unsigned i = (unsigned)rand();
    while (i >= cEntries)
        i >>= 1;
    return i;
}

#if 0
# define kHlpAlloc(a) malloc(a)
# define kHlpFree(a) free(a)
#endif

int main()
{
    int cErrors = 0;
    int rc;
#define MAX_ALLOCS 256
    static struct
    {
        void *pv;
        unsigned cb;
    } s_aAllocs[MAX_ALLOCS];
    unsigned cAllocs;
    unsigned i;
    unsigned j;

    /*
     * Some simple init / term.
     */
    rc = kHlpHeapInit();
    CHECK_FATAL(!rc);
    kHlpHeapTerm();

    rc = kHlpHeapInit();
    CHECK_FATAL(!rc);
    kHlpHeapTerm();


    /*
     * Simple alloc all, free all in FIFO order.
     */
    rc = kHlpHeapInit();
    CHECK_FATAL(!rc);

    /* 1. allocate all slots. */
    for (i = 0; i < MAX_ALLOCS; i++)
    {
        s_aAllocs[i].cb = RandSize();
        s_aAllocs[i].pv = kHlpAlloc(s_aAllocs[i].cb);
        CHECK(s_aAllocs[i].pv);
    }

    /* 2. free all slots. */
    for (i = 0; i < MAX_ALLOCS; i++)
        kHlpFree(s_aAllocs[i].pv);

    /* terminate */
    kHlpHeapTerm();


    /*
     * Simple alloc all, free all in LIFO order.
     */
    rc = kHlpHeapInit();
    CHECK_FATAL(!rc);

    /* 1. allocate all slots. */
    for (i = 0; i < MAX_ALLOCS; i++)
    {
        s_aAllocs[i].cb = RandSize();
        s_aAllocs[i].pv = kHlpAlloc(s_aAllocs[i].cb);
        CHECK(s_aAllocs[i].pv);
    }

    /* 2. free all slots. */
    i = MAX_ALLOCS;
    while (i-- > 0)
        kHlpFree(s_aAllocs[i].pv);

    /* terminate */
    kHlpHeapTerm();


    /*
     * Bunch of allocations, free half, allocate and free in pairs, free all.
     */
    rc = kHlpHeapInit();
    CHECK_FATAL(!rc);

    /* 1. allocate all slots. */
    for (i = 0; i < MAX_ALLOCS; i++)
    {
        s_aAllocs[i].cb = RandSize();
        s_aAllocs[i].pv = kHlpAlloc(s_aAllocs[i].cb);
        CHECK(s_aAllocs[i].pv);
    }
    cAllocs = MAX_ALLOCS;

    /* 2. free half (random order). */
    while (cAllocs > MAX_ALLOCS / 2)
    {
        i = RandIdx(cAllocs);
        kHlpFree(s_aAllocs[i].pv);
        cAllocs--;
        if (i != cAllocs)
            s_aAllocs[i] = s_aAllocs[cAllocs];
    }

    /* 3. lots of alloc and free activity. */
    for (j = 0; j < MAX_ALLOCS * 32; j++)
    {
        /* allocate */
        unsigned cMax = RandIdx(MAX_ALLOCS / 4) + 1;
        while (cAllocs < MAX_ALLOCS && cMax-- > 0)
        {
            i = cAllocs;
            s_aAllocs[i].cb = RandSize();
            s_aAllocs[i].pv = kHlpAlloc(s_aAllocs[i].cb);
            CHECK(s_aAllocs[i].pv);
            cAllocs++;
        }

        /* free */
        cMax = RandIdx(MAX_ALLOCS / 4) + 1;
        while (cAllocs > MAX_ALLOCS / 2 && cMax-- > 0)
        {
            i = RandIdx(cAllocs);
            kHlpFree(s_aAllocs[i].pv);
            cAllocs--;
            if (i != cAllocs)
                s_aAllocs[i] = s_aAllocs[cAllocs];
        }
    }

    /* 4. free all */
    while (cAllocs > 0)
    {
        i = RandIdx(cAllocs);
        kHlpFree(s_aAllocs[i].pv);
        cAllocs--;
        if (i != cAllocs)
            s_aAllocs[i] = s_aAllocs[cAllocs];
    }

    /* terminate */
    kHlpHeapTerm();


    /* summary */
    if (!cErrors)
        printf("tstkLdrHeap: SUCCESS\n");
    else
        printf("tstkLdrHeap: FAILURE - %d errors\n", cErrors);
    return !!cErrors;
}
