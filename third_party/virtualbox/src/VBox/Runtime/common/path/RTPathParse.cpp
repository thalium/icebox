/* $Id: RTPathParse.cpp $ */
/** @file
 * IPRT - RTPathParse
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
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

#define RTPATH_TEMPLATE_CPP_H "RTPathParse.cpp.h"
#include "rtpath-expand-template.cpp.h"


RTDECL(int) RTPathParse(const char *pszPath, PRTPATHPARSED pParsed, size_t cbParsed, uint32_t fFlags)
{
    /*
     * Input validation.
     */
    AssertReturn(cbParsed >= RT_UOFFSETOF(RTPATHPARSED, aComps), VERR_INVALID_PARAMETER);
    AssertPtrReturn(pParsed, VERR_INVALID_POINTER);
    AssertPtrReturn(pszPath, VERR_INVALID_POINTER);
    AssertReturn(*pszPath, VERR_PATH_ZERO_LENGTH);
    AssertReturn(RTPATH_STR_F_IS_VALID(fFlags, 0), VERR_INVALID_FLAGS);

    /*
     * Invoke the worker for the selected path style.
     */
    switch (fFlags & RTPATH_STR_F_STYLE_MASK)
    {
#if defined(RT_OS_OS2) || defined(RT_OS_WINDOWS)
        case RTPATH_STR_F_STYLE_HOST:
#endif
        case RTPATH_STR_F_STYLE_DOS:
            return rtPathParseStyleDos(pszPath, pParsed, cbParsed, fFlags);

#if !defined(RT_OS_OS2) && !defined(RT_OS_WINDOWS)
        case RTPATH_STR_F_STYLE_HOST:
#endif
        case RTPATH_STR_F_STYLE_UNIX:
            return rtPathParseStyleUnix(pszPath, pParsed, cbParsed, fFlags);

        default:
            AssertFailedReturn(VERR_INVALID_FLAGS); /* impossible */
    }
}

