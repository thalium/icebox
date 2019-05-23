/* $Id: RTPathAbsEx.cpp $ */
/** @file
 * IPRT - RTPathAbsEx
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
#include "internal/path.h"



/**
 * Get the absolute path (no symlinks, no . or .. components), assuming the
 * given base path as the current directory. The resulting path doesn't have
 * to exist.
 *
 * @returns iprt status code.
 * @param   pszBase         The base path to act like a current directory.
 *                          When NULL, the actual cwd is used (i.e. the call
 *                          is equivalent to RTPathAbs(pszPath, ...).
 * @param   pszPath         The path to resolve.
 * @param   pszAbsPath      Where to store the absolute path.
 * @param   cchAbsPath      Size of the buffer.
 */
RTDECL(int) RTPathAbsEx(const char *pszBase, const char *pszPath, char *pszAbsPath, size_t cchAbsPath)
{
    if (    pszBase
        &&  pszPath
        &&  !rtPathVolumeSpecLen(pszPath)
       )
    {
#if defined(RT_OS_WINDOWS)
        /* The format for very long paths is not supported. */
        if (    RTPATH_IS_SLASH(pszBase[0])
            &&  RTPATH_IS_SLASH(pszBase[1])
            &&  pszBase[2] == '?'
            &&  RTPATH_IS_SLASH(pszBase[3])
           )
            return VERR_INVALID_NAME;
#endif

        /** @todo there are a couple of things which isn't 100% correct, although the
         * current code will have to do for now, no time to fix.
         *
         * 1) On Windows & OS/2 we confuse '/' with an abspath spec and will
         *    not necessarily resolve it on the right drive.
         * 2) A trailing slash in the base might cause UNC names to be created.
         */
        size_t const cchPath = strlen(pszPath);
        char         szTmpPath[RTPATH_MAX];
        if (RTPATH_IS_SLASH(pszPath[0]))
        {
            /* join the disk name from base and the path (DOS systems only) */
            size_t const cchVolSpec = rtPathVolumeSpecLen(pszBase);
            if (cchVolSpec + cchPath + 1 > sizeof(szTmpPath))
                return VERR_FILENAME_TOO_LONG;
            memcpy(szTmpPath, pszBase, cchVolSpec);
            memcpy(&szTmpPath[cchVolSpec], pszPath, cchPath + 1);
        }
        else
        {
            /* join the base path and the path */
            size_t const cchBase = strlen(pszBase);
            if (cchBase + 1 + cchPath + 1 > sizeof(szTmpPath))
                return VERR_FILENAME_TOO_LONG;
            memcpy(szTmpPath, pszBase, cchBase);
            szTmpPath[cchBase] = RTPATH_DELIMITER;
            memcpy(&szTmpPath[cchBase + 1], pszPath, cchPath + 1);
        }
        return RTPathAbs(szTmpPath, pszAbsPath, cchAbsPath);
    }

    /* Fallback to the non *Ex version */
    return RTPathAbs(pszPath, pszAbsPath, cchAbsPath);
}

