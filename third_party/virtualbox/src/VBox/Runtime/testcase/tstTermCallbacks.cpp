/* $Id: tstTermCallbacks.cpp $ */
/** @file
 * IPRT Testcase - Termination Callbacks.
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
#include <iprt/initterm.h>

#include <iprt/test.h>
#include <iprt/stream.h>
#include <iprt/err.h>
#include <iprt/initterm.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static uint32_t g_cCalls;
static uint32_t g_fCalled;


static DECLCALLBACK(void) tstTermCallback0(RTTERMREASON enmReason, int32_t iStatus, void *pvUser)
{
    RT_NOREF_PV(enmReason); RT_NOREF_PV(iStatus);
    RTTESTI_CHECK(pvUser == (void *)0);
    g_cCalls++;
    g_fCalled |= RT_BIT_32(0);
}


static DECLCALLBACK(void) tstTermCallback1(RTTERMREASON enmReason, int32_t iStatus, void *pvUser)
{
    RT_NOREF_PV(enmReason); RT_NOREF_PV(iStatus);
    RTTESTI_CHECK(pvUser == (void *)1);
    g_cCalls++;
    g_fCalled |= RT_BIT_32(1);
}


static DECLCALLBACK(void) tstTermCallback2(RTTERMREASON enmReason, int32_t iStatus, void *pvUser)
{
    RT_NOREF_PV(enmReason); RT_NOREF_PV(iStatus);
    RTTESTI_CHECK(pvUser == (void *)2);
    g_cCalls++;
    g_fCalled |= RT_BIT_32(2);
}


static DECLCALLBACK(void) tstTermCallback3(RTTERMREASON enmReason, int32_t iStatus, void *pvUser)
{
    RT_NOREF_PV(enmReason); RT_NOREF_PV(iStatus);
    RTTESTI_CHECK(pvUser == (void *)3);
    g_cCalls++;
    g_fCalled |= RT_BIT_32(3);
}


int main()
{
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstTermCallback", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);

    /*
     * Just some basics.
     */
    RTTestSub(hTest, "Uninitialized");
    RTTESTI_CHECK_RC(RTTermDeregisterCallback(tstTermCallback1, (void *)1), VERR_NOT_FOUND);
    RTTermRunCallbacks(RTTERMREASON_UNLOAD, 0);

    RTTestSub(hTest, "One callback");
    g_cCalls = g_fCalled = 0;
    RTTESTI_CHECK_RC(RTTermRegisterCallback(tstTermCallback0, (void *)0), VINF_SUCCESS);
    RTTermRunCallbacks(RTTERMREASON_UNLOAD, 0);
    RTTESTI_CHECK(g_cCalls == 1);
    RTTESTI_CHECK(g_fCalled == RT_BIT_32(0));

    RTTestSub(hTest, "Two callbacks");
    g_cCalls = g_fCalled = 0;
    RTTESTI_CHECK_RC(RTTermRegisterCallback(tstTermCallback0, (void *)0), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTTermRegisterCallback(tstTermCallback1, (void *)1), VINF_SUCCESS);
    RTTermRunCallbacks(RTTERMREASON_UNLOAD, 0);
    RTTESTI_CHECK(g_cCalls == 2);
    RTTESTI_CHECK(g_fCalled == (RT_BIT_32(0) | RT_BIT_32(1)));

    RTTestSub(hTest, "Three callbacks");
    g_cCalls = g_fCalled = 0;
    RTTESTI_CHECK_RC(RTTermRegisterCallback(tstTermCallback0, (void *)0), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTTermRegisterCallback(tstTermCallback1, (void *)1), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTTermRegisterCallback(tstTermCallback2, (void *)2), VINF_SUCCESS);
    RTTermRunCallbacks(RTTERMREASON_UNLOAD, 0);
    RTTESTI_CHECK(g_cCalls == 3);
    RTTESTI_CHECK(g_fCalled == (RT_BIT_32(0) | RT_BIT_32(1) | RT_BIT_32(2)));

    RTTestSub(hTest, "Four callbacks");
    g_cCalls = g_fCalled = 0;
    RTTESTI_CHECK_RC(RTTermRegisterCallback(tstTermCallback0, (void *)0), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTTermRegisterCallback(tstTermCallback1, (void *)1), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTTermRegisterCallback(tstTermCallback2, (void *)2), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTTermRegisterCallback(tstTermCallback3, (void *)3), VINF_SUCCESS);
    RTTermRunCallbacks(RTTERMREASON_UNLOAD, 0);
    RTTESTI_CHECK(g_cCalls == 4);
    RTTESTI_CHECK(g_fCalled == (RT_BIT_32(0) | RT_BIT_32(1) | RT_BIT_32(2) | RT_BIT_32(3)));

    RTTestSub(hTest, "Callback deregistration");
    g_cCalls = g_fCalled = 0;
    RTTESTI_CHECK_RC(RTTermRegisterCallback(tstTermCallback0, (void *)1), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTTermRegisterCallback(tstTermCallback0, (void *)1), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTTermRegisterCallback(tstTermCallback0, (void *)0), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTTermRegisterCallback(tstTermCallback1, (void *)1), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTTermRegisterCallback(tstTermCallback1, (void *)0), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTTermDeregisterCallback(tstTermCallback0, (void *)1), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTTermDeregisterCallback(tstTermCallback0, (void *)1), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTTermDeregisterCallback(tstTermCallback0, (void *)1), VERR_NOT_FOUND);
    RTTESTI_CHECK_RC(RTTermDeregisterCallback(tstTermCallback1, (void *)0), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTTermDeregisterCallback(tstTermCallback1, (void *)0), VERR_NOT_FOUND);
    RTTermRunCallbacks(RTTERMREASON_UNLOAD, 0);
    RTTESTI_CHECK(g_cCalls == 2);
    RTTESTI_CHECK(g_fCalled == (RT_BIT_32(0) | RT_BIT_32(1)));

    /*
     * Summary.
     */
    return RTTestSummaryAndDestroy(hTest);
}

