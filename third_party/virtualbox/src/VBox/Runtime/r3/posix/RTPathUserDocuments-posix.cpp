/* $Id: RTPathUserDocuments-posix.cpp $ */
/** @file
 * IPRT - RTPathUserDocuments, posix ring-3.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
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
#include <iprt/path.h>
#include <iprt/err.h>
#include <iprt/assert.h>

RTDECL(int) RTPathUserDocuments(char *pszPath, size_t cchPath)
{
    /*
     * Validate input
     */
    AssertPtrReturn(pszPath, VERR_INVALID_POINTER);
    AssertReturn(cchPath, VERR_INVALID_PARAMETER);

    int rc = RTPathUserHome(pszPath, cchPath);
    if (RT_FAILURE(rc))
        return rc;

    rc = RTPathAppend(pszPath, cchPath, "Documents");
    if (RT_FAILURE(rc))
        *pszPath = '\0';

    return rc;
}

