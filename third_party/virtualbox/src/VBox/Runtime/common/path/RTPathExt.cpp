/* $Id: RTPathExt.cpp $ */
/** @file
 * IPRT - RTPathExt
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


RTDECL(char *) RTPathSuffix(const char *pszPath)
{
    const char *psz = pszPath;
    const char *pszExt = NULL;

    for (;; psz++)
    {
        switch (*psz)
        {
            /* handle separators. */
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
            case ':':
                pszExt = NULL;
                break;

            case '\\':
#endif
            case '/':
                pszExt = NULL;
                break;
            case '.':
                pszExt = psz;
                break;

            /* the end */
            case '\0':
                if (pszExt && pszExt != pszPath && pszExt[1])
                    return (char *)(void *)pszExt;
                return NULL;
        }
    }

    /* not reached */
}

