/* $Id: RTPathGetCurrentDrive-generic.cpp $ */
/** @file
 * IPRT - RTPathGetCurrentDrive, generic implementation.
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
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
#define LOG_GROUP RTLOGGROUP_PATH
#include <iprt/path.h>
#include "internal/iprt.h"

#include <iprt/ctype.h>
#include <iprt/err.h>
#include <iprt/string.h>
#include "internal/path.h"


RTDECL(int) RTPathGetCurrentDrive(char *pszPath, size_t cbPath)
{
#ifdef HAVE_DRIVE
    /*
     * Query the current directroy and extract the wanted information from it.
     */
    int rc = RTPathGetCurrent(pszPath, cbPath);
    if (RT_SUCCESS(rc))
    {
        /*
         * Drive letter? Chop off at root slash.
         */
        if (pszPath[0] && RTPATH_IS_VOLSEP(pszPath[1]))
        {
            pszPath[2] = '\0';
            return rc;
        }

        /*
         * UNC? Chop off after share.
         */
        if (   RTPATH_IS_SLASH(pszPath[0])
            && RTPATH_IS_SLASH(pszPath[1])
            && !RTPATH_IS_SLASH(pszPath[2])
            && pszPath[2])
        {
            /* Work thru the server name. */
            size_t off = 3;
            while (!RTPATH_IS_SLASH(pszPath[off]) && pszPath[off])
                off++;
            size_t offServerSlash = off;

            /* Is there a share name? */
            if (RTPATH_IS_SLASH(pszPath[off]))
            {
                while (RTPATH_IS_SLASH(pszPath[off]))
                    off++;
                if (pszPath[off])
                {
                    /* Work thru the share name. */
                    while (!RTPATH_IS_SLASH(pszPath[off]) && pszPath[off])
                        off++;
                }
                /* No share name, chop at server name. */
                else
                    off = offServerSlash;
            }
        }
        return VERR_INTERNAL_ERROR_4;
    }
    return rc;

#else  /* !HAVE_DRIVE */
    /*
     * No drive letters on this system, return empty string.
     */
    if (cbPath > 0)
    {
        *pszPath = '\0';
        return VINF_SUCCESS;
    }
    return VERR_BUFFER_OVERFLOW;
#endif /* !HAVE_DRIVE */
}
RT_EXPORT_SYMBOL(RTPathGetCurrentDrive);

