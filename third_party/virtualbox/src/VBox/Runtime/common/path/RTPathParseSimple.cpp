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
    const char *psz = pszPath;
    ssize_t     offRoot = 0;
    const char *pszName = pszPath;
    const char *pszLastDot = NULL;

    for (;; psz++)
    {
        switch (*psz)
        {
            /* handle separators. */
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
            case ':':
                pszName = psz + 1;
                offRoot = pszName - psz;
                break;

            case '\\':
#endif
            case '/':
                pszName = psz + 1;
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
                    if (pszLastDot)
                    {
                        offSuff = pszLastDot - pszPath;
                        if (offSuff <= offName)
                            offSuff = -1;
                    }
                    *poffSuff = offSuff;
                }

                if (pcchDir)
                {
                    ssize_t off = offName - 1;
                    while (off >= offRoot && RTPATH_IS_SLASH(pszPath[off]))
                        off--;
                    *pcchDir = RT_MAX(off, offRoot) + 1;
                }

                return psz - pszPath;
            }
        }
    }

    /* will never get here */
}

