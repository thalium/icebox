/* $Id: rtProcInitExePath-linux.cpp $ */
/** @file
 * IPRT - rtProcInitName, Linux.
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
#define LOG_GROUP RTLOGGROUP_PROCESS
#include <unistd.h>
#include <errno.h>

#include <iprt/string.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/path.h>
#include "internal/process.h"
#include "internal/path.h"


DECLHIDDEN(int) rtProcInitExePath(char *pszPath, size_t cchPath)
{
    /*
     * Read the /proc/self/exe link, convert to native and return it.
     */
    int cchLink = readlink("/proc/self/exe", pszPath, cchPath - 1);
    if (cchLink > 0 && (size_t)cchLink <= cchPath - 1)
    {
        pszPath[cchLink] = '\0';

        char const *pszTmp;
        int rc = rtPathFromNative(&pszTmp, pszPath, NULL);
        AssertMsgRCReturn(rc, ("rc=%Rrc pszLink=\"%s\"\nhex: %.*Rhxs\n", rc, pszPath, cchLink, pszPath), rc);
        if (pszTmp != pszPath)
        {
            rc = RTStrCopy(pszPath, cchPath, pszTmp);
            rtPathFreeIprt(pszTmp, pszPath);
        }
        return rc;
    }

    int err = errno;
    int rc = RTErrConvertFromErrno(err);
    AssertMsgFailed(("rc=%Rrc err=%d cchLink=%d\n", rc, err, cchLink));
    return rc;
}

