/* $Id: RTPathTraverseList.cpp $ */
/** @file
 * IPRT - RTPathTraverseList
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "internal/iprt.h"
#include <iprt/path.h>

#include <iprt/assert.h>
#include <iprt/ctype.h>
#include <iprt/err.h>
#include <iprt/string.h>


RTDECL(int) RTPathTraverseList(const char *pszPathList, char chSep, PFNRTPATHTRAVERSER pfnCallback, void *pvUser1, void *pvUser2)
{
    AssertPtrNull(pszPathList);
    Assert((unsigned int)chSep <= 127);

    if (!pszPathList)
        return VERR_END_OF_STRING;

    /*
     * Walk the path list.
     */
    const char *psz = pszPathList;
    while (*psz)
    {
        /* Skip leading blanks - no directories with leading spaces, thank you. */
        while (RT_C_IS_BLANK(*psz))
            psz++;

        /* Find the end of this element. */
        const char *pszNext;
        const char *pszEnd = strchr(psz, chSep);
        if (!pszEnd)
            pszEnd = pszNext = strchr(psz, '\0');
        else
            pszNext = pszEnd + 1;
        if (pszEnd != psz)
        {
            size_t const cch = pszEnd - psz;
            int rc = pfnCallback(psz, cch, pvUser1, pvUser2);
            if (rc != VERR_TRY_AGAIN)
                return rc;
        }

        /* advance */
        psz = pszNext;
    }

    return VERR_END_OF_STRING;
}
RT_EXPORT_SYMBOL(RTPathTraverseList);

