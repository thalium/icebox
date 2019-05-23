/* $Id: RTPathJoinA.cpp $ */
/** @file
 * IPRT - RTPathJoinA.
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
#include <iprt/path.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/string.h>




RTDECL(char *) RTPathJoinA(const char *pszPathSrc, const char *pszAppend)
{
    AssertPtr(pszAppend);
    AssertPtr(pszPathSrc);

    /*
     * The easy way: Allocate a buffer and call RTPathAppend till it succeeds.
     */
    size_t cchPathSrc = strlen(pszPathSrc);
    size_t cchAppend  = strlen(pszAppend);
    size_t cbPathDst  = cchPathSrc + cchAppend + 4;
    char *pszPathDst  = RTStrAlloc(cbPathDst);
    if (pszPathDst)
    {
        memcpy(pszPathDst, pszPathSrc, cchPathSrc + 1);
        int rc = RTPathAppend(pszPathDst, cbPathDst, pszAppend);
        if (RT_FAILURE(rc))
        {
            /* This shouldn't happen, but if it does try again with a larger buffer... */
            AssertRC(rc);

            rc = RTStrRealloc(&pszPathDst, cbPathDst * 2);
            if (RT_SUCCESS(rc))
                rc = RTPathAppend(pszPathDst, cbPathDst, pszAppend);
            if (RT_FAILURE(rc))
            {
                RTStrFree(pszPathDst);
                pszPathDst = NULL;
            }
        }
    }
    return pszPathDst;
}

