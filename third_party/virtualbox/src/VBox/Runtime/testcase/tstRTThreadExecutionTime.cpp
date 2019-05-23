/* $Id: tstRTThreadExecutionTime.cpp $ */
/** @file
 * IPRT Testcase - RTThreadGetExecution.
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

#include <iprt/asm.h>
#include <iprt/test.h>
#include <iprt/string.h>
#include <iprt/stream.h>
#include <iprt/time.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static RTTEST g_hTest;
static volatile uint64_t g_kernel, g_user;


static DECLCALLBACK(int) testThread(RTTHREAD hSelf, void *pvUser)
{
    RT_NOREF_PV(hSelf); RT_NOREF_PV(pvUser);

    uint64_t u64Now = RTTimeMilliTS();
    uint64_t kernel, kernelStart, user, userStart;
    RTThreadGetExecutionTimeMilli(&kernelStart, &userStart);
    while (RTTimeMilliTS() < u64Now + 1000)
        ;
    RTThreadGetExecutionTimeMilli(&kernel, &user);
    RTPrintf("kernel = %4lldms, user = %4lldms\n", kernel - kernelStart, user - userStart);
    ASMAtomicAddU64(&g_kernel, kernel);
    ASMAtomicAddU64(&g_user, user);

    return VINF_SUCCESS;
}


static void test1(void)
{
    RTTestSub(g_hTest, "Interrupt RTThreadSleep");
    RTTHREAD hThread[16];
    RTMSINTERVAL msWait = 1000;
    for (unsigned i = 0; i < RT_ELEMENTS(hThread); i++)
    {
        RTTESTI_CHECK_RC_RETV(RTThreadCreate(&hThread[i], testThread, NULL, 0, RTTHREADTYPE_DEFAULT,
                              RTTHREADFLAGS_WAITABLE, "test"), VINF_SUCCESS);
    }
    RTThreadSleep(500);
    RTPrintf("Waiting for %dms ...\n", msWait);
    RTThreadSleep(msWait);
    for (unsigned i = 0; i < RT_ELEMENTS(hThread); i++)
        RTTESTI_CHECK_RC(RTThreadWait(hThread[i], RT_INDEFINITE_WAIT, NULL), VINF_SUCCESS);

    RTPrintf("sum kernel = %lldms, sum user = %lldms\n", g_kernel, g_user);
}


int main()
{
    RTEXITCODE rcExit = RTTestInitAndCreate("tstRTThreadExecutionTime", &g_hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;
    test1();

    return RTTestSummaryAndDestroy(g_hTest);
}
