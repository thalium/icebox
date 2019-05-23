/* $Id: shellsort.cpp $ */
/** @file
 * IPRT - RTSortIsSorted.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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
#include "internal/iprt.h"
#include <iprt/sort.h>

#include <iprt/alloca.h>
#include <iprt/assert.h>
#include <iprt/string.h>



RTDECL(void) RTSortShell(void *pvArray, size_t cElements, size_t cbElement, PFNRTSORTCMP pfnCmp, void *pvUser)
{
    Assert(cbElement <= 32);

    /* Anything worth sorting? */
    if (cElements < 2)
        return;

    uint8_t *pbArray = (uint8_t *)pvArray;
    void    *pvTmp   = alloca(cbElement);
    size_t   cGap    = (cElements + 1) / 2;
    while (cGap > 0)
    {
        size_t i;
        for (i = cGap; i < cElements; i++)
        {
            memcpy(pvTmp, &pbArray[i * cbElement], cbElement);
            size_t  j     = i;
            while (   j >= cGap
                   && pfnCmp(&pbArray[(j - cGap) * cbElement], pvTmp, pvUser) > 0)
            {
                memmove(&pbArray[j * cbElement], &pbArray[(j - cGap) * cbElement], cbElement);
                j -= cGap;
            }
            memcpy(&pbArray[j * cbElement], pvTmp, cbElement);
        }

        /* This does not generate the most optimal gap sequence, but it has the
           advantage of being simple and avoid floating point. */
        cGap /= 2;
    }
}


RTDECL(void) RTSortApvShell(void **papvArray, size_t cElements, PFNRTSORTCMP pfnCmp, void *pvUser)
{
    /* Anything worth sorting? */
    if (cElements < 2)
        return;

    size_t cGap = (cElements + 1) / 2;
    while (cGap > 0)
    {
        size_t i;
        for (i = cGap; i < cElements; i++)
        {
            void   *pvTmp = papvArray[i];
            size_t  j     = i;
            while (   j >= cGap
                   && pfnCmp(papvArray[j - cGap], pvTmp, pvUser) > 0)
            {
                papvArray[j] = papvArray[j - cGap];
                j -= cGap;
            }
            papvArray[j] = pvTmp;
        }

        /* This does not generate the most optimal gap sequence, but it has the
           advantage of being simple and avoid floating point. */
        cGap /= 2;
    }
}

