/* $Id: RTProcDaemonize-generic.cpp $ */
/** @file
 * IPRT - RTProcDaemonize, generic implementation.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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
#include <iprt/process.h>
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/env.h>
#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/mem.h>
#include <iprt/string.h>
#include "internal/process.h"


RTR3DECL(int) RTProcDaemonize(const char * const *papszArgs, const char *pszDaemonizedOpt)
{
    /*
     * Get the executable name.
     * If this asserts, it's probably because rtR3Init hasn't been called.
     */
    char szExecPath[RTPATH_MAX];
    AssertReturn(RTProcGetExecutablePath(szExecPath, sizeof(szExecPath)) == szExecPath, VERR_WRONG_ORDER);

    /*
     * Create a copy of the argument list with the daemonized option appended.
     */
    unsigned cArgs = 0;
    while (papszArgs[cArgs])
        cArgs++;

    char const **papszNewArgs = (char const **)RTMemAlloc(sizeof(const char *) * (cArgs + 2));
    if (!papszNewArgs)
        return VERR_NO_MEMORY;
    for (unsigned i = 0; i < cArgs; i++)
        papszNewArgs[i] = papszArgs[i];
    papszNewArgs[cArgs] = pszDaemonizedOpt;
    papszNewArgs[cArgs + 1] = NULL;

    /*
     * Open the bitbucket handles and create the detached process.
     */
    RTHANDLE hStdIn;
    int rc = RTFileOpenBitBucket(&hStdIn.u.hFile, RTFILE_O_READ);
    if (RT_SUCCESS(rc))
    {
        hStdIn.enmType = RTHANDLETYPE_FILE;

        RTHANDLE hStdOutAndErr;
        rc = RTFileOpenBitBucket(&hStdOutAndErr.u.hFile, RTFILE_O_WRITE);
        if (RT_SUCCESS(rc))
        {
            hStdOutAndErr.enmType = RTHANDLETYPE_FILE;

            rc = RTProcCreateEx(szExecPath, papszNewArgs, RTENV_DEFAULT,
                                RTPROC_FLAGS_DETACHED | RTPROC_FLAGS_SAME_CONTRACT,
                                &hStdIn, &hStdOutAndErr, &hStdOutAndErr,
                                NULL /*pszAsUser*/,  NULL /*pszPassword*/, NULL /*phProcess*/);

            RTFileClose(hStdOutAndErr.u.hFile);
        }

        RTFileClose(hStdOutAndErr.u.hFile);
    }
    RTMemFree(papszNewArgs);
    return rc;
}

