/* $Id: RTPathAbsExDup.cpp $ */
/** @file
 * IPRT - RTPathAbsExDup
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
#include <iprt/err.h>
#include <iprt/param.h>
#include <iprt/string.h>



/**
 * Same as RTPathAbsEx only the result is RTStrDup()'ed.
 *
 * @returns Pointer to the absolute path. Use RTStrFree() to free this string.
 * @returns NULL if RTPathAbsEx() or RTStrDup() fails.
 * @param   pszBase         The base path to act like a current directory.
 *                          When NULL, the actual cwd is used (i.e. the call
 *                          is equivalent to RTPathAbs(pszPath, ...).
 * @param   pszPath         The path to resolve.
 */
RTDECL(char *) RTPathAbsExDup(const char *pszBase, const char *pszPath)
{
    char szPath[RTPATH_MAX];
    int rc = RTPathAbsEx(pszBase, pszPath, szPath, sizeof(szPath));
    if (RT_SUCCESS(rc))
        return RTStrDup(szPath);
    return NULL;
}

