/* $Id: rdtsc.cpp $ */
/** @file
 * rdtsc - Test if three consecutive rdtsc instructions return different values.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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


#include <stdio.h>

int main()
{
    int i;
    int cMatches = 0;
    for (i = 0; i < 10000000; i++)
    {
        int cMatchedNow = 0;
        __asm
        {
            rdtsc
            mov ebx, eax
            mov ecx, edx
            rdtsc
            mov esi, eax
            mov edi, edx
            rdtsc

            cmp eax, ebx
            jnz l_ok1
            cmp edx, ecx
            jnz l_ok1
            inc [cMatchedNow]
        l_ok1:

            cmp eax, esi
            jnz l_ok2
            cmp edx, edi
            jnz l_ok2
            inc [cMatchedNow]
            jmp l_ok3
        l_ok2:

            cmp ebx, esi
            jnz l_ok3
            cmp ecx, edi
            jnz l_ok3
            inc [cMatchedNow]
        l_ok3:
        }
        if (cMatchedNow)
        {
            printf("%08d: %d\n", i, cMatchedNow);
            cMatches += cMatchedNow;
        }
    }
    printf("done: %d matches\n", cMatches);
    return !!cMatches;
}
