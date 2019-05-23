/* $Id: RTSystemShutdown-win.cpp $ */
/** @file
 * IPRT - RTSystemShutdown, Windows.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
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
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/string.h>

#include <iprt/win/windows.h>


RTDECL(int) RTSystemShutdown(RTMSINTERVAL cMsDelay, uint32_t fFlags, const char *pszLogMsg)
{
    AssertPtrReturn(pszLogMsg, VERR_INVALID_POINTER);
    AssertReturn(!(fFlags & ~RTSYSTEM_SHUTDOWN_VALID_MASK), VERR_INVALID_PARAMETER);

    PRTUTF16 pwszLogMsg;
    int rc = RTStrToUtf16(pszLogMsg, &pwszLogMsg);
    if (RT_FAILURE(rc))
        return rc;
    DWORD cSecsTimeout = (cMsDelay + 499) / 1000;
    BOOL  fRebootAfterShutdown = (fFlags & RTSYSTEM_SHUTDOWN_ACTION_MASK) == RTSYSTEM_SHUTDOWN_REBOOT
                               ? TRUE : FALSE;
    BOOL  fForceAppsClosed = fFlags & RTSYSTEM_SHUTDOWN_FORCE ? TRUE : FALSE;

    /*
     * Do the
     */
    if (InitiateSystemShutdownW(NULL /*pwszMachineName = NULL = localhost*/,
                                pwszLogMsg,
                                cSecsTimeout,
                                fForceAppsClosed,
                                fRebootAfterShutdown))
        rc = (fFlags & RTSYSTEM_SHUTDOWN_ACTION_MASK) == RTSYSTEM_SHUTDOWN_HALT ? VINF_SYS_MAY_POWER_OFF : VINF_SUCCESS;
    else
    {
        /* If we failed because of missing privileges, try get the right to
           shut down the system and call the api again. */
        DWORD dwErr = GetLastError();
        rc = RTErrConvertFromWin32(dwErr);
        if (dwErr == ERROR_ACCESS_DENIED)
        {
            HANDLE hToken = NULL;
            if (OpenThreadToken(GetCurrentThread(),
                                TOKEN_ADJUST_PRIVILEGES,
                                TRUE /*OpenAsSelf*/,
                                &hToken))
                dwErr = NO_ERROR;
            else
            {
                dwErr  = GetLastError();
                if (dwErr == ERROR_NO_TOKEN)
                {
                    if (OpenProcessToken(GetCurrentProcess(),
                                         TOKEN_ADJUST_PRIVILEGES,
                                         &hToken))
                        dwErr = NO_ERROR;
                    else
                        dwErr = GetLastError();
                }
            }

            if (dwErr == NO_ERROR)
            {
                union
                {
                    TOKEN_PRIVILEGES TokenPriv;
                    char ab[sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES)];
                } u;
                u.TokenPriv.PrivilegeCount = 1;
                u.TokenPriv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
                if (LookupPrivilegeValue(NULL /*localhost*/, SE_SHUTDOWN_NAME, &u.TokenPriv.Privileges[0].Luid))
                {
                    if (AdjustTokenPrivileges(hToken,
                                              FALSE /*DisableAllPrivileges*/,
                                              &u.TokenPriv,
                                              RT_OFFSETOF(TOKEN_PRIVILEGES, Privileges[1]),
                                              NULL,
                                              NULL) )
                    {
                        if (InitiateSystemShutdownW(NULL /*pwszMachineName = NULL = localhost*/,
                                                    pwszLogMsg,
                                                    cSecsTimeout,
                                                    fForceAppsClosed,
                                                    fRebootAfterShutdown))
                            rc = (fFlags & RTSYSTEM_SHUTDOWN_ACTION_MASK) == RTSYSTEM_SHUTDOWN_HALT ? VINF_SYS_MAY_POWER_OFF : VINF_SUCCESS;
                        else
                        {
                            dwErr = GetLastError();
                            rc = RTErrConvertFromWin32(dwErr);
                        }
                    }
                    CloseHandle(hToken);
                }
            }
        }
    }

    RTUtf16Free(pwszLogMsg);
    return rc;
}
RT_EXPORT_SYMBOL(RTSystemShutdown);

