/* $Id: tstRTThreadPoke.cpp $ */
/** @file
 * IPRT Testcase - RTThreadPoke.
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
#include <iprt/thread.h>

#include <iprt/test.h>
#include <iprt/string.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static RTTEST g_hTest;


#ifndef RT_OS_WINDOWS

static DECLCALLBACK(int) test1Thread(RTTHREAD hSelf, void *pvUser)
{
    RT_NOREF_PV(hSelf); RT_NOREF_PV(pvUser);
    RTTEST_CHECK_RC(g_hTest, RTThreadSleep(60*1000), VERR_INTERRUPTED);
    return VINF_SUCCESS;
}


static void test1(void)
{
    RTTestSub(g_hTest, "Interrupt RTThreadSleep");
    RTTHREAD hThread;
    RTTESTI_CHECK_RC_RETV(RTThreadCreate(&hThread, test1Thread, NULL, 0, RTTHREADTYPE_DEFAULT, RTTHREADFLAGS_WAITABLE, "test1"),
                          VINF_SUCCESS);
    RTThreadSleep(500); RTThreadSleep(1500); /* fudge */
    RTTESTI_CHECK_RC(RTThreadPoke(hThread), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTThreadWait(hThread, RT_INDEFINITE_WAIT, NULL), VINF_SUCCESS);
}

#endif /* !RT_OS_WINDOWS */


int main()
{
    RTEXITCODE rcExit = RTTestInitAndCreate("tstRTThreadPoke", &g_hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;
#ifdef RT_OS_WINDOWS
    return RTTestSkipAndDestroy(g_hTest, "No RTThreadPoke on Windows");

#else
    test1();

    return RTTestSummaryAndDestroy(g_hTest);
#endif
}

