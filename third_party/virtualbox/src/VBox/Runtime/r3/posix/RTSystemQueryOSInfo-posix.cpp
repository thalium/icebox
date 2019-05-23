/* $Id: RTSystemQueryOSInfo-posix.cpp $ */
/** @file
 * IPRT - RTSystemQueryOSInfo, POSIX implementation.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
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
#include <iprt/system.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/err.h>

#include <errno.h>
#include <sys/utsname.h>


RTDECL(int) RTSystemQueryOSInfo(RTSYSOSINFO enmInfo, char *pszInfo, size_t cchInfo)
{
    /*
     * Quick validation.
     */
    AssertReturn(enmInfo > RTSYSOSINFO_INVALID && enmInfo < RTSYSOSINFO_END, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszInfo, VERR_INVALID_POINTER);
    if (!cchInfo)
        return VERR_BUFFER_OVERFLOW;

    /*
     * Handle the request.
     */
    switch (enmInfo)
    {
        case RTSYSOSINFO_PRODUCT:
        case RTSYSOSINFO_RELEASE:
        case RTSYSOSINFO_VERSION:
        {
            struct utsname UtsInfo;
            if (uname(&UtsInfo) < 0)
                return RTErrConvertFromErrno(errno);
            const char *pszSrc;
            switch (enmInfo)
            {
                case RTSYSOSINFO_PRODUCT:   pszSrc = UtsInfo.sysname; break;
                case RTSYSOSINFO_RELEASE:   pszSrc = UtsInfo.release; break;
                case RTSYSOSINFO_VERSION:   pszSrc = UtsInfo.version; break;
                default: AssertFatalFailed(); /* screw gcc */
            }
            size_t cch = strlen(pszSrc);
            if (cch < cchInfo)
            {
                memcpy(pszInfo, pszSrc, cch + 1);
                return VINF_SUCCESS;
            }
            memcpy(pszInfo, pszSrc, cchInfo - 1);
            pszInfo[cchInfo - 1] = '\0';
            return VERR_BUFFER_OVERFLOW;
        }


        case RTSYSOSINFO_SERVICE_PACK:
        default:
            *pszInfo = '\0';
            return VERR_NOT_SUPPORTED;
    }

    return VINF_SUCCESS;
}

