/* $Id: tstRTProcCreatePrf.cpp $ */
/** @file
 * IPRT Testcase - RTProcCreate Profiling.
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
#include <iprt/process.h>
#include <iprt/test.h>
#include <iprt/time.h>
#include <iprt/path.h>
#include <iprt/env.h>
#include <iprt/string.h>


int main(int argc, char **argv)
{
    /* the child response. */
    if (argc != 1)
        return 0;

    RTTEST hTest;
    RTEXITCODE rcExit = RTTestInitAndCreate("tstRTProcCreatePrf", &hTest);
    if (rcExit)
        return rcExit;
    RTTestBanner(hTest);

    char szExecPath[RTPATH_MAX];
    if (!RTProcGetExecutablePath(szExecPath, sizeof(szExecPath)))
        RTStrCopy(szExecPath, sizeof(szExecPath), argv[0]);

    const char *apszArgs[4] = { szExecPath, "child", "process", NULL };

    uint64_t NsStart = RTTimeNanoTS();
    uint32_t i;
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2) || defined(RT_OS_DARWIN)
    for (i = 0; i < 1000; i++)
#else
    for (i = 0; i < 10000; i++)
#endif
    {
        RTPROCESS hProc;
        RTTEST_CHECK_RC_BREAK(hTest, RTProcCreate(szExecPath, apszArgs, RTENV_DEFAULT, 0 /* fFlags*/, &hProc), VINF_SUCCESS);
        RTPROCSTATUS ChildStatus;
        RTTEST_CHECK_RC_BREAK(hTest, RTProcWait(hProc, RTPROCWAIT_FLAGS_BLOCK, &ChildStatus), VINF_SUCCESS);
        RTTEST_CHECK_BREAK(hTest, ChildStatus.enmReason == RTPROCEXITREASON_NORMAL);
        RTTEST_CHECK_BREAK(hTest, ChildStatus.iStatus == 0);
    }
    uint64_t cNsElapsed = RTTimeNanoTS() - NsStart;
    if (i)
    {
        RTTestValue(hTest, "Time per process", cNsElapsed / i, RTTESTUNIT_NS);
    }

    /*
     * Summary.
     */
    return RTTestSummaryAndDestroy(hTest);
}

