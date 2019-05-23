/* $Id: comparepaths.cpp $ */
/** @file
 * IPRT - Path Comparison.
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
#include <iprt/string.h>
#include <iprt/uni.h>


/**
 * Helper for RTPathCompare() and RTPathStartsWith().
 *
 * @returns similar to strcmp.
 * @param   pszPath1        Path to compare.
 * @param   pszPath2        Path to compare.
 * @param   fLimit          Limit the comparison to the length of \a pszPath2
 * @internal
 */
static int rtPathCompare(const char *pszPath1, const char *pszPath2, bool fLimit)
{
    if (pszPath1 == pszPath2)
        return 0;
    if (!pszPath1)
        return -1;
    if (!pszPath2)
        return 1;

#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
    for (;;)
    {
        RTUNICP uc1;
        int rc = RTStrGetCpEx(&pszPath1, &uc1);
        if (RT_SUCCESS(rc))
        {
            RTUNICP uc2;
            rc = RTStrGetCpEx(&pszPath2, &uc2);
            if (RT_SUCCESS(rc))
            {
                if (uc1 == uc2)
                {
                    if (uc1)
                    { /* likely */ }
                    else
                        return 0;
                }
                else
                {
                    if (uc1 == '\\')
                        uc1 = '/';
                    else
                        uc1 = RTUniCpToUpper(uc1);
                    if (uc2 == '\\')
                        uc2 = '/';
                    else
                        uc2 = RTUniCpToUpper(uc2);
                    if (uc1 != uc2)
                    {
                        if (fLimit && uc2 == '\0')
                            return 0;
                        return uc1 > uc2 ? 1 : -1; /* (overflow/underflow paranoia) */
                    }
                }
            }
            else
                return 1;
        }
        else
            return -1;
    }
#else
    if (!fLimit)
        return strcmp(pszPath1, pszPath2);
    return strncmp(pszPath1, pszPath2, strlen(pszPath2));
#endif
}


/**
 * Compares two paths.
 *
 * The comparison takes platform-dependent details into account,
 * such as:
 * <ul>
 * <li>On DOS-like platforms, both separator chars (|\| and |/|) are considered
 *     to be equal.
 * <li>On platforms with case-insensitive file systems, mismatching characters
 *     are uppercased and compared again.
 * </ul>
 *
 * @returns @< 0 if the first path less than the second path.
 * @returns 0 if the first path identical to the second path.
 * @returns @> 0 if the first path greater than the second path.
 *
 * @param   pszPath1    Path to compare (must be an absolute path).
 * @param   pszPath2    Path to compare (must be an absolute path).
 *
 * @remarks File system details are currently ignored. This means that you won't
 *          get case-insensitive compares on unix systems when a path goes into a
 *          case-insensitive filesystem like FAT, HPFS, HFS, NTFS, JFS, or
 *          similar. For NT, OS/2 and similar you'll won't get case-sensitive
 *          compares on a case-sensitive file system.
 */
RTDECL(int) RTPathCompare(const char *pszPath1, const char *pszPath2)
{
    return rtPathCompare(pszPath1, pszPath2, false /* full path lengths */);
}


/**
 * Checks if a path starts with the given parent path.
 *
 * This means that either the path and the parent path matches completely, or
 * that the path is to some file or directory residing in the tree given by the
 * parent directory.
 *
 * The path comparison takes platform-dependent details into account,
 * see RTPathCompare() for details.
 *
 * @returns |true| when \a pszPath starts with \a pszParentPath (or when they
 *          are identical), or |false| otherwise.
 *
 * @param   pszPath         Path to check, must be an absolute path.
 * @param   pszParentPath   Parent path, must be an absolute path.
 *                          No trailing directory slash!
 */
RTDECL(bool) RTPathStartsWith(const char *pszPath, const char *pszParentPath)
{
    if (pszPath == pszParentPath)
        return true;
    if (!pszPath || !pszParentPath)
        return false;

    if (rtPathCompare(pszPath, pszParentPath, true /* limited by path 2 */) != 0)
        return false;

    const size_t cchParentPath = strlen(pszParentPath);
    if (RTPATH_IS_SLASH(pszPath[cchParentPath]))
        return true;
    if (pszPath[cchParentPath] == '\0')
        return true;

    /* Deal with pszParentPath = root (or having a trailing slash). */
    if (   cchParentPath > 0
        && RTPATH_IS_SLASH(pszParentPath[cchParentPath - 1])
        && RTPATH_IS_SLASH(pszPath[cchParentPath - 1]))
        return true;

    return false;
}

