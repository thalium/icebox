/* $Id: RTShutdown.cpp $ */
/** @file
 * IPRT Testcase - System Shutdown.
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

#include <iprt/buildconfig.h>
#include <iprt/err.h>
#include <iprt/getopt.h>
#include <iprt/initterm.h>
#include <iprt/message.h>
#include <iprt/stream.h>
#include <iprt/string.h>


int main(int argc, char **argv)
{
    int rc = RTR3InitExe(argc, &argv, 0);
    if (RT_FAILURE(rc))
        return RTMsgInitFailure(rc);

    /*
     * Parse the command line.
     */
    static const RTGETOPTDEF s_aOptions[] =
    {
        { "--halt",         'H', RTGETOPT_REQ_NOTHING },
        { "--poweroff",     'p', RTGETOPT_REQ_NOTHING },
        { "--reboot",       'r', RTGETOPT_REQ_NOTHING },
        { "--force",        'f', RTGETOPT_REQ_NOTHING },
        { "--delay",        'd', RTGETOPT_REQ_UINT32  },
        { "--message",      'm', RTGETOPT_REQ_STRING  }
    };

    const char     *pszMsg   = "RTShutdown";
    RTMSINTERVAL    cMsDelay = 0;
    uint32_t        fFlags   = RTSYSTEM_SHUTDOWN_POWER_OFF | RTSYSTEM_SHUTDOWN_PLANNED;

    RTGETOPTSTATE   GetState;
    rc = RTGetOptInit(&GetState, argc, argv, s_aOptions, RT_ELEMENTS(s_aOptions), 1,
                      RTGETOPTINIT_FLAGS_OPTS_FIRST);
    for (;;)
    {
        RTGETOPTUNION ValueUnion;
        rc = RTGetOpt(&GetState, &ValueUnion);
        if (rc == 0)
            break;
        switch (rc)
        {
            case 'H': fFlags = (fFlags & ~RTSYSTEM_SHUTDOWN_ACTION_MASK) | RTSYSTEM_SHUTDOWN_HALT; break;
            case 'p': fFlags = (fFlags & ~RTSYSTEM_SHUTDOWN_ACTION_MASK) | RTSYSTEM_SHUTDOWN_POWER_OFF_HALT; break;
            case 'r': fFlags = (fFlags & ~RTSYSTEM_SHUTDOWN_ACTION_MASK) | RTSYSTEM_SHUTDOWN_REBOOT; break;
            case 'f': fFlags |= RTSYSTEM_SHUTDOWN_FORCE; break;
            case 'd': cMsDelay = ValueUnion.u32; break;
            case 'm': pszMsg = ValueUnion.psz; break;

            case 'h':
                RTPrintf("Usage: RTShutdown [-H|-p|-r] [-f] [-d <milliseconds>] [-m <msg>]\n");
                return RTEXITCODE_SUCCESS;

            case 'V':
                RTPrintf("%sr%d\n", RTBldCfgVersion(), RTBldCfgRevision());
                return RTEXITCODE_SUCCESS;

            default:
                return RTGetOptPrintError(rc, &ValueUnion);
        }
    }

    /*
     * Do the deed.
     */
    rc = RTSystemShutdown(cMsDelay, fFlags, pszMsg);
    if (RT_FAILURE(rc))
        return RTMsgErrorExit(RTEXITCODE_FAILURE, "RTSystemShutdown(%u, %#x, \"%s\") returned %Rrc\n", cMsDelay, fFlags, pszMsg, rc);
    return RTEXITCODE_SUCCESS;
}

