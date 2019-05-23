/* $Id: RTSystemShutdown-linux.cpp $ */
/** @file
 * IPRT - RTSystemShutdown, linux implementation.
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
#include <iprt/env.h>
#include <iprt/err.h>
#include <iprt/process.h>
#include <iprt/string.h>


RTDECL(int) RTSystemShutdown(RTMSINTERVAL cMsDelay, uint32_t fFlags, const char *pszLogMsg)
{
    AssertPtrReturn(pszLogMsg, VERR_INVALID_POINTER);
    AssertReturn(!(fFlags & ~RTSYSTEM_SHUTDOWN_VALID_MASK), VERR_INVALID_PARAMETER);

    /*
     * Assemble the argument vector.
     */
    int         iArg = 0;
    const char *apszArgs[6];

    RT_BZERO(apszArgs, sizeof(apszArgs));

    apszArgs[iArg++] = "/sbin/shutdown";
    switch (fFlags & RTSYSTEM_SHUTDOWN_ACTION_MASK)
    {
        case RTSYSTEM_SHUTDOWN_HALT:
            apszArgs[iArg++] = "-h";
            apszArgs[iArg++] = "-H";
            break;
        case RTSYSTEM_SHUTDOWN_REBOOT:
            apszArgs[iArg++] = "-r";
            break;
        case RTSYSTEM_SHUTDOWN_POWER_OFF:
        case RTSYSTEM_SHUTDOWN_POWER_OFF_HALT:
            apszArgs[iArg++] = "-h";
            apszArgs[iArg++] = "-P";
            break;
    }

    char szWhen[80];
    if (cMsDelay < 500)
        strcpy(szWhen, "now");
    else
        RTStrPrintf(szWhen, sizeof(szWhen), "%u", (unsigned)((cMsDelay + 499) / 1000));
    apszArgs[iArg++] = szWhen;

    apszArgs[iArg++] = pszLogMsg;


    /*
     * Start the shutdown process and wait for it to complete.
     */
    RTPROCESS hProc;
    int rc = RTProcCreate(apszArgs[0], apszArgs, RTENV_DEFAULT, 0 /*fFlags*/, &hProc);
    if (RT_FAILURE(rc))
        return rc;

    RTPROCSTATUS ProcStatus;
    rc = RTProcWait(hProc, RTPROCWAIT_FLAGS_BLOCK, &ProcStatus);
    if (RT_SUCCESS(rc))
    {
        if (   ProcStatus.enmReason != RTPROCEXITREASON_NORMAL
            || ProcStatus.iStatus   != 0)
            rc = VERR_SYS_SHUTDOWN_FAILED;
    }

    return rc;
}
RT_EXPORT_SYMBOL(RTSystemShutdown);

