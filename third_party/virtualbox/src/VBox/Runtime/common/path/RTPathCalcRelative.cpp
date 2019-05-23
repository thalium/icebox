/* $Id: RTPathCalcRelative.cpp $ */
/** @file
 * IPRT - RTPathCreateRelative.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
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
#include "internal/path.h"



RTDECL(int) RTPathCalcRelative(char *pszPathDst, size_t cbPathDst,
                               const char *pszPathFrom,
                               const char *pszPathTo)
{
    int rc = VINF_SUCCESS;

    AssertPtrReturn(pszPathDst, VERR_INVALID_POINTER);
    AssertReturn(cbPathDst, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszPathFrom, VERR_INVALID_POINTER);
    AssertPtrReturn(pszPathTo, VERR_INVALID_POINTER);
    AssertReturn(RTPathStartsWithRoot(pszPathFrom), VERR_INVALID_PARAMETER);
    AssertReturn(RTPathStartsWithRoot(pszPathTo), VERR_INVALID_PARAMETER);
    AssertReturn(RTStrCmp(pszPathFrom, pszPathTo), VERR_INVALID_PARAMETER);

    /*
     * Check for different root specifiers (drive letters), creating a relative path doesn't work here.
     * @todo: How to handle case insensitive root specifiers correctly?
     */
    size_t offRootFrom = rtPathRootSpecLen(pszPathFrom);
    size_t offRootTo   = rtPathRootSpecLen(pszPathTo);

    if (   offRootFrom != offRootTo
        || RTStrNCmp(pszPathFrom, pszPathTo, offRootFrom))
        return VERR_NOT_SUPPORTED;

    /* Filter out the parent path which is equal to both paths. */
    while (   *pszPathFrom == *pszPathTo
           && *pszPathFrom != '\0'
           && *pszPathTo != '\0')
    {
        pszPathFrom++;
        pszPathTo++;
    }

    /*
     * Because path components can start with an equal string but differ afterwards we
     * need to go back to the beginning of the current component.
     */
    while (!RTPATH_IS_SEP(*pszPathFrom))
        pszPathFrom--;

    pszPathFrom++; /* Skip path separator. */

    while (!RTPATH_IS_SEP(*pszPathTo))
        pszPathTo--;

    pszPathTo++; /* Skip path separator. */

    /* Paths point to the first non equal component now. */
    char aszPathTmp[RTPATH_MAX + 1];
    unsigned offPathTmp = 0;

    /* Create the part to go up from pszPathFrom. */
    while (*pszPathFrom != '\0')
    {
        while (   !RTPATH_IS_SEP(*pszPathFrom)
               && *pszPathFrom != '\0')
            pszPathFrom++;

        if (RTPATH_IS_SEP(*pszPathFrom))
        {
            if (offPathTmp + 3 >= sizeof(aszPathTmp) - 1)
                return VERR_FILENAME_TOO_LONG;
            aszPathTmp[offPathTmp++] = '.';
            aszPathTmp[offPathTmp++] = '.';
            aszPathTmp[offPathTmp++] = RTPATH_SLASH;
            pszPathFrom++;
        }
    }

    aszPathTmp[offPathTmp] = '\0';

    /* Now append the rest of pszPathTo to the final path. */
    char *pszPathTmp = &aszPathTmp[offPathTmp];
    size_t cbPathTmp = sizeof(aszPathTmp) - offPathTmp - 1;
    rc = RTStrCatP(&pszPathTmp, &cbPathTmp, pszPathTo);
    if (RT_SUCCESS(rc))
    {
        *pszPathTmp = '\0';

        size_t cchPathTmp = strlen(aszPathTmp);
        if (cchPathTmp >= cbPathDst)
           return VERR_BUFFER_OVERFLOW;
        memcpy(pszPathDst, aszPathTmp, cchPathTmp + 1);
    }
    else
        rc = VERR_FILENAME_TOO_LONG;

    return rc;
}

