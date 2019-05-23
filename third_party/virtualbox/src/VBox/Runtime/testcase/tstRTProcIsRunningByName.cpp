/* $Id: tstRTProcIsRunningByName.cpp $ */
/** @file
 * IPRT Testcase - RTProcIsRunningByName
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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
#include <iprt/process.h>
#include <iprt/initterm.h>
#include <iprt/stream.h>
#include <iprt/err.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/string.h>



int main(int argc, char **argv)
{
    int cErrors = 0;

    RTR3InitExe(argc, &argv, 0);
    RTPrintf("tstRTPRocIsRunningByName: TESTING...\n");

    /*
     * Test 1: Check for a definitely not running process.
     */
    char szExecPath[RTPATH_MAX] = { "vbox-5b05e1ff-6ae2-4d10-885a-7d25018c4c5b" };
    if (!RTProcIsRunningByName(szExecPath))
        RTPrintf("tstRTProcIsRunningByName: Process '%s' is not running (expected).\n", szExecPath);
    else
    {
        RTPrintf("tstRTProcIsRunningByName: FAILURE - '%s' is running! (test 1)\n", szExecPath);
        cErrors++;
    }

    /*
     * Test 2: Check for a definitely not running process.
     */
    strcpy(szExecPath, "/bin/vbox-5b05e1ff-6ae2-4d10-885a-7d25018c4c5b");
    if (!RTProcIsRunningByName(szExecPath))
        RTPrintf("tstRTProcIsRunningByName: Process '%s' is not running (expected).\n", szExecPath);
    else
    {
        RTPrintf("tstRTProcIsRunningByName: FAILURE - '%s' is running! (test 1)\n", szExecPath);
        cErrors++;
    }

    /*
     * Test 3: Check for our own process, filename only.
     */
    if (RTProcGetExecutablePath(szExecPath, RTPATH_MAX))
    {
        /* Strip any path components */
        char *pszFilename = RTPathFilename(szExecPath);
        if (pszFilename)
        {
            if (RTProcIsRunningByName(pszFilename))
                RTPrintf("tstRTProcIsRunningByName: Process '%s' (self) is running\n", pszFilename);
            else
            {
                RTPrintf("tstRTProcIsRunningByName: FAILURE - Process '%s' (self) is not running!\n", pszFilename);
                cErrors++;
            }
        }
        else
        {
            RTPrintf("tstRTProcIsRunningByName: FAILURE - RTPathFilename failed!\n");
            cErrors++;
        }

        /*
         * Test 4: Check for our own process, full path.
         */
        if (RTProcIsRunningByName(szExecPath))
            RTPrintf("tstRTProcIsRunningByName: Process '%s' (self) is running\n", szExecPath);
        else
        {
            RTPrintf("tstRTProcIsRunningByName: FAILURE - Process '%s' (self) is not running!\n", szExecPath);
            cErrors++;
        }
    }
    else
    {
        RTPrintf("tstRTProcIsRunningByName: FAILURE - RTProcGetExecutablePath failed!\n");
        cErrors++;
    }


    /*
     * Summary.
     */
    if (!cErrors)
        RTPrintf("tstRTProcIsRunningByName: SUCCESS\n");
    else
        RTPrintf("tstRTProcIsRunningByName: FAILURE - %d errors\n", cErrors);
    return cErrors ? 1 : 0;
}
