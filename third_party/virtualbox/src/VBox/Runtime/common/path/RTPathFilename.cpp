/* $Id: RTPathFilename.cpp $ */
/** @file
 * IPRT - RTPathFilename
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



RTDECL(char *) RTPathFilename(const char *pszPath)
{
    return RTPathFilenameEx(pszPath, RTPATH_STYLE);
}
RT_EXPORT_SYMBOL(RTPathFilename);


RTDECL(char *) RTPathFilenameEx(const char *pszPath, uint32_t fFlags)
{
    const char *psz = pszPath;
    const char *pszName = pszPath;

    Assert(RTPATH_STR_F_IS_VALID(fFlags, 0 /*no extra flags*/));
    fFlags &= RTPATH_STR_F_STYLE_MASK;
    if (fFlags == RTPATH_STR_F_STYLE_HOST)
        fFlags = RTPATH_STYLE;
    if (fFlags == RTPATH_STR_F_STYLE_DOS)
    {
        for (;; psz++)
        {
            switch (*psz)
            {
                /* handle separators. */
                case ':':
                case '\\':
                case '/':
                    pszName = psz + 1;
                    break;

                /* the end */
                case '\0':
                    if (*pszName)
                        return (char *)(void *)pszName;
                    return NULL;
            }
        }
    }
    else
    {
        Assert(fFlags == RTPATH_STR_F_STYLE_UNIX);
        for (;; psz++)
        {
            switch (*psz)
            {
                /* handle separators. */
                case '/':
                    pszName = psz + 1;
                    break;

                /* the end */
                case '\0':
                    if (*pszName)
                        return (char *)(void *)pszName;
                    return NULL;
            }
        }
    }

    /* not reached */
}
RT_EXPORT_SYMBOL(RTPathFilenameEx);

