/* $Id: RTPathCopyComponents.cpp $ */
/** @file
 * IPRT - RTPathCountComponents
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
#include "internal/path.h"


RTDECL(int) RTPathCopyComponents(char *pszDst, size_t cbDst, const char *pszSrc, size_t cComponents)
{
    /*
     * Quick input validation.
     */
    AssertPtr(pszDst);
    AssertPtr(pszSrc);
    if (cbDst == 0)
        return VERR_BUFFER_OVERFLOW;

    /*
     * Fend of the simple case where nothing is wanted.
     */
    if (cComponents == 0)
    {
        *pszDst = '\0';
        return VINF_SUCCESS;
    }

    /*
     * Parse into the path until we've counted the desired number of objects
     * or hit the end.
     */
    size_t off = rtPathRootSpecLen(pszSrc);
    size_t c   = off != 0;
    while (c < cComponents && pszSrc[off])
    {
        c++;
        while (!RTPATH_IS_SLASH(pszSrc[off]) && pszSrc[off])
            off++;
        while (RTPATH_IS_SLASH(pszSrc[off]))
            off++;
    }

    /*
     * Copy up to but not including 'off'.
     */
    if (off >= cbDst)
        return VERR_BUFFER_OVERFLOW;

    memcpy(pszDst, pszSrc, off);
    pszDst[off] = '\0';
    return VINF_SUCCESS;
}

