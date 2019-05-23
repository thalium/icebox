/* $Id: tstFork.cpp $ */
/** @file
 * IPRT Testcase - fork() issues.
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
#include <iprt/test.h>
#include <iprt/process.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/initterm.h>

#ifndef RT_OS_WINDOWS
# include <unistd.h>
# include <sys/wait.h>
# include <errno.h>
#endif


int main()
{
    /*
     * Init the runtime and stuff.
     */
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstFork", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);

#ifdef RT_OS_WINDOWS
    RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "Skipped\n");
#else
    /*
     * Get values that are supposed to or change across the fork.
     */
    RTPROCESS const ProcBefore = RTProcSelf();

    /*
     * Fork.
     */
    pid_t pid = fork();
    if (pid == 0)
    {
        /*
         * Check that the values has changed.
         */
        rc = 0;
        if (ProcBefore == RTProcSelf())
        {
            RTTestFailed(hTest, "%RTproc == %RTproc [child]", ProcBefore, RTProcSelf());
            rc = 1;
        }
        return rc;
    }
    if (pid != -1)
    {
        /*
         * Check that the values didn't change.
         */
        RTTEST_CHECK(hTest, ProcBefore == RTProcSelf());

        /*
         * Wait for the child.
         */
        rc = 1;
        while (   waitpid(pid, &rc, 0)
               && errno == EINTR)
            rc = 1;
        if (!WIFEXITED(rc) || WEXITSTATUS(rc) != 0)
            RTTestFailed(hTest, "rc=%#x", rc);
    }
    else
        RTTestFailed(hTest, "fork() failed: %d - %s", errno, strerror(errno));
#endif

    /*
     * Summary
     */
    return RTTestSummaryAndDestroy(hTest);
}

