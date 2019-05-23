/* $Id: alloc-1.c $ */
/** @file
 * Allocate lots of memory, portable ANSI C code.
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
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
#include <stdlib.h>
#include <stdio.h>


int main(int argc, char **argv)
{
    unsigned      uPct;
    unsigned long cbDone;
    unsigned long cMBs = 1024;
    unsigned long cb;

    /*
     * Some quick and dirty argument parsing.
     */
    if (argc == 2)
        cMBs = strtoul(argv[1], 0, 0);
    if (!cMBs || argc > 2)
    {
        printf("usage: alloc-1 [MBs]\n");
        return 1;
    }
    cb = cMBs * 1024 * 1024;
    if (cb / (1024 * 1024) != cMBs)
        cb = ~(unsigned long)0 / (1024 * 1024) * (1024 * 1024);
    printf("alloc-1: allocating %lu MB (%lu bytes)\n", cb/1024/1024, cb);

    /*
     * The allocation loop.
     */
    printf("alloc-1: 0%%");
    fflush(stdout);
    cbDone = 0;
    uPct = 0;
    while (cbDone < cb)
    {
        unsigned      uPctNow;
        unsigned long cbThis = cb > 10*1024*1024 ? 10*1024*1024 : cb;
        char         *pb = malloc(cbThis);
        if (!pb)
        {
            printf("\nalloc-1: calloc failed, cbDone=%lu MB (%lu bytes)\n",
                   cbDone/1024/1024, cbDone);
            return 1;
        }
        cbDone += cbThis;

        /* touch the memory. */
        while (cbThis >= 0x1000)
        {
            *pb = (char)cbThis;
            pb += 0x1000;
            cbThis -= 0x1000;
        }

        /* progress */
        uPctNow = 100.0 * cbDone / cb;
        if (uPctNow != uPct && !(uPctNow & 1))
        {
            if (!(uPctNow % 10))
                printf("%u%%", uPctNow);
            else
                printf(".");
            fflush(stdout);
        }
        uPct = uPctNow;
    }

    printf("\nalloc-1: done\n");
    return 0;
}
