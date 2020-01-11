/* $Id: RTPathParseSimple.cpp $ */
/** @file
 * IPRT - RTPathParseSimple
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


/**
 * Parses a path.
 *
 * It figures the length of the directory component, the offset of
 * the file name and the location of the suffix dot.
 *
 * @returns The path length.
 *
 * @param   pszPath     Path to find filename in.
 * @param   pcchDir     Where to put the length of the directory component. If
 *                      no directory, this will be 0. Optional.
 * @param   poffName    Where to store the filename offset.
 *                      If empty string or if it's ending with a slash this
 *                      will be set to -1. Optional.
 * @param   poffSuff    Where to store the suffix offset (the last dot).
 *                      If empty string or if it's ending with a slash this
 *                      will be set to -1. Optional.
 */
RTDECL(size_t) RTPathParseSimple(const char *pszPath, size_t *pcchDir, ssize_t *poffName, ssize_t *poffSuff)
{
    /*
     * First deal with the root as it is always more fun that you'd think.
     */
    const char *psz     = pszPath;
    size_t      cchRoot = 0;

#if RTPATH_STYLE == RTPATH_STR_F_STYLE_DOS
    if (RT_C_IS_ALPHA(*psz) && RTPATH_IS_VOLSEP(psz[1]))
    {
        /* Volume specifier. */
        cchRoot = 2;
        psz    += 2;
    }
    else if (RTPATH_IS_SLASH(*psz) && RTPATH_IS_SLASH(psz[1]))
    {
        /* UNC - there are exactly two prefix slashes followed by a namespace
           or computer name, which can be empty on windows.  */
        cchRoot = 2;
        psz += 2;
        while (!RTPATH_IS_SLASH(*psz) && *psz)
        {
            cchRoot++;
            psz++;
        }
    }
#endif
    while (RTPATH_IS_SLASH(*psz))
    {
        cchRoot++;
        psz++;
    }

    /*
     * Do the remainder.
     */
    const char *pszName = psz;
    const char *pszLastDot = NULL;
    for (;; psz++)
    {
        switch (*psz)
        {
            default:
                break;

            /* handle separators. */
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
            case '\\':
#endif
            case '/':
                pszName = psz + 1;
                pszLastDot = NULL;
                break;

            case '.':
                pszLastDot = psz;
                break;

            /*
             * The end. Complete the results.
             */
            case '\0':
            {
                ssize_t offName = *pszName != '\0' ? pszName - pszPath : -1;
                if (poffName)
                    *poffName = offName;

                if (poffSuff)
                {
                    ssize_t offSuff = -1;
                    if (   pszLastDot
                        && pszLastDot != pszName
                        && pszLastDot[1] != '\0')
                    {
                        offSuff = pszLastDot - pszPath;
                        Assert(offSuff > offName);
                    }
                    *poffSuff = offSuff;
                }

                if (pcchDir)
                {
                    size_t cch = offName < 0 ? psz - pszPath : offName - 1 < (ssize_t)cchRoot ? cchRoot : offName - 1;
                    while (cch > cchRoot && RTPATH_IS_SLASH(pszPath[cch - 1]))
                        cch--;
                    *pcchDir = cch;
                }

                return psz - pszPath;
            }
        }
    }

    /* will never get here */
}

