/* $Id: tstRTMp-1.cpp $ */
/** @file
 * IPRT Testcase - RTMp.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
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
#include <iprt/mp.h>
#include <iprt/cpuset.h>
#include <iprt/err.h>
#include <iprt/string.h>
#include <iprt/test.h>
#ifdef VBOX
# include <VBox/sup.h>
#endif



int main(int argc, char **argv)
{
    RTTEST hTest;
    RTEXITCODE rcExit = RTTestInitAndCreate("tstRTMp-1", &hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;
    RTTestBanner(hTest);

    NOREF(argc); NOREF(argv);
#ifdef VBOX
    if (argc > 1)
        SUPR3Init(NULL);
#endif

    /*
     * Present and possible CPUs.
     */
    RTCPUID cCpus = RTMpGetCount();
    if (cCpus > 0)
        RTTestIPrintf(RTTESTLVL_ALWAYS, "RTMpGetCount -> %u\n", cCpus);
    else
    {
        RTTestIFailed("RTMpGetCount returned zero");
        cCpus = 1;
    }

    RTCPUID cCoreCpus = RTMpGetCoreCount();
    if (cCoreCpus > 0)
        RTTestIPrintf(RTTESTLVL_ALWAYS, "RTMpGetCoreCount -> %d\n", (int)cCoreCpus);
    else
    {
        RTTestIFailed("RTMpGetCoreCount returned zero");
        cCoreCpus = 1;
    }
    RTTESTI_CHECK(cCoreCpus <= cCpus);

    RTCPUSET Set;
    PRTCPUSET pSet = RTMpGetSet(&Set);
    RTTESTI_CHECK(pSet == &Set);
    if (pSet == &Set)
    {
        RTTESTI_CHECK((RTCPUID)RTCpuSetCount(&Set) == cCpus);

        RTTestIPrintf(RTTESTLVL_ALWAYS, "Possible CPU mask:\n");
        for (int iCpu = 0; iCpu < RTCPUSET_MAX_CPUS; iCpu++)
        {
            RTCPUID idCpu = RTMpCpuIdFromSetIndex(iCpu);
            if (RTCpuSetIsMemberByIndex(&Set, iCpu))
            {
                RTTestIPrintf(RTTESTLVL_ALWAYS, "%2d - id %d: %u/%u MHz", iCpu, (int)idCpu,
                              RTMpGetCurFrequency(idCpu), RTMpGetMaxFrequency(idCpu));
                if (RTMpIsCpuPresent(idCpu))
                    RTTestIPrintf(RTTESTLVL_ALWAYS, RTMpIsCpuOnline(idCpu) ? " online\n" : " offline\n");
                else
                {
                    if (!RTMpIsCpuOnline(idCpu))
                        RTTestIPrintf(RTTESTLVL_ALWAYS, " absent\n");
                    else
                    {
                        RTTestIPrintf(RTTESTLVL_ALWAYS, " online but absent!\n");
                        RTTestIFailed("Cpu with index %d is report as !RTIsCpuPresent while RTIsCpuOnline returns true!\n", iCpu);
                    }
                }
                if (!RTMpIsCpuPossible(idCpu))
                    RTTestIFailed("Cpu with index %d is returned by RTCpuSet but not RTMpIsCpuPossible!\n", iCpu);
            }
            else if (RTMpIsCpuPossible(idCpu))
                RTTestIFailed("Cpu with index %d is returned by RTMpIsCpuPossible but not RTCpuSet!\n", iCpu);
            else if (RTMpGetCurFrequency(idCpu) != 0)
                RTTestIFailed("RTMpGetCurFrequency(%d[idx=%d]) didn't return 0 as it should\n", (int)idCpu, iCpu);
            else if (RTMpGetMaxFrequency(idCpu) != 0)
                RTTestIFailed("RTMpGetMaxFrequency(%d[idx=%d]) didn't return 0 as it should\n", (int)idCpu, iCpu);
        }
    }
    else
    {
        RTCpuSetEmpty(&Set);
        RTCpuSetAdd(&Set, RTMpCpuIdFromSetIndex(0));
    }

    /*
     * Online CPUs.
     */
    RTCPUID cCpusOnline = RTMpGetOnlineCount();
    if (cCpusOnline > 0)
    {
        if (cCpusOnline <= cCpus)
            RTTestIPrintf(RTTESTLVL_ALWAYS, "RTMpGetOnlineCount -> %d\n", (int)cCpusOnline);
        else
        {
            RTTestIFailed("RTMpGetOnlineCount -> %d, expected <= %d\n", (int)cCpusOnline, (int)cCpus);
            cCpusOnline = cCpus;
        }
    }
    else
    {
        RTTestIFailed("RTMpGetOnlineCount -> %d\n", (int)cCpusOnline);
        cCpusOnline = 1;
    }

    RTCPUID cCoresOnline = RTMpGetOnlineCoreCount();
    if (cCoresOnline > 0)
        RTTestIPrintf(RTTESTLVL_ALWAYS, "RTMpGetOnlineCoreCount -> %d\n", (int)cCoresOnline);
    else
    {
        RTTestIFailed("RTMpGetOnlineCoreCount -> %d, expected <= %d\n", (int)cCoresOnline, (int)cCpusOnline);
        cCoresOnline = 1;
    }
    RTTESTI_CHECK(cCoresOnline <= cCpusOnline);

    RTCPUSET SetOnline;
    pSet = RTMpGetOnlineSet(&SetOnline);
    if (pSet == &SetOnline)
    {
        if (RTCpuSetCount(&SetOnline) <= 0)
            RTTestIFailed("RTMpGetOnlineSet returned an empty set!\n");
        else if ((RTCPUID)RTCpuSetCount(&SetOnline) > cCpus)
            RTTestIFailed("RTMpGetOnlineSet returned a too high value; %d, expected <= %d\n",
                          RTCpuSetCount(&SetOnline), cCpus);
        RTTestIPrintf(RTTESTLVL_ALWAYS, "Online CPU mask:\n");
        for (int iCpu = 0; iCpu < RTCPUSET_MAX_CPUS; iCpu++)
            if (RTCpuSetIsMemberByIndex(&SetOnline, iCpu))
            {
                RTCPUID idCpu = RTMpCpuIdFromSetIndex(iCpu);
                RTTestIPrintf(RTTESTLVL_ALWAYS, "%2d - id %d: %u/%u MHz %s\n", iCpu, (int)idCpu, RTMpGetCurFrequency(idCpu),
                              RTMpGetMaxFrequency(idCpu), RTMpIsCpuOnline(idCpu) ? "online" : "offline");
                if (!RTCpuSetIsMemberByIndex(&Set, iCpu))
                    RTTestIFailed("online cpu with index %2d is not a member of the possible cpu set!\n", iCpu);
            }

        /* There isn't any sane way of testing RTMpIsCpuOnline really... :-/ */
    }
    else
        RTTestIFailed("RTMpGetOnlineSet -> %p, expected %p\n", pSet, &Set);

    /*
     * Present CPUs.
     */
    RTCPUID cCpusPresent = RTMpGetPresentCount();
    if (cCpusPresent > 0)
    {
        if (    cCpusPresent <= cCpus
            &&  cCpusPresent >= cCpusOnline)
            RTTestIPrintf(RTTESTLVL_ALWAYS, "RTMpGetPresentCount -> %d\n", (int)cCpusPresent);
        else
            RTTestIFailed("RTMpGetPresentCount -> %d, expected <= %d and >= %d\n",
                          (int)cCpusPresent, (int)cCpus, (int)cCpusOnline);
    }
    else
    {
        RTTestIFailed("RTMpGetPresentCount -> %d\n", (int)cCpusPresent);
        cCpusPresent = 1;
    }

    RTCPUSET SetPresent;
    pSet = RTMpGetPresentSet(&SetPresent);
    if (pSet == &SetPresent)
    {
        if (RTCpuSetCount(&SetPresent) <= 0)
            RTTestIFailed("RTMpGetPresentSet returned an empty set!\n");
        else if ((RTCPUID)RTCpuSetCount(&SetPresent) != cCpusPresent)
            RTTestIFailed("RTMpGetPresentSet returned a bad value; %d, expected = %d\n",
                          RTCpuSetCount(&SetPresent), cCpusPresent);
        RTTestIPrintf(RTTESTLVL_ALWAYS, "Present CPU mask:\n");
        for (int iCpu = 0; iCpu < RTCPUSET_MAX_CPUS; iCpu++)
            if (RTCpuSetIsMemberByIndex(&SetPresent, iCpu))
            {
                RTCPUID idCpu = RTMpCpuIdFromSetIndex(iCpu);
                RTTestIPrintf(RTTESTLVL_ALWAYS, "%2d - id %d: %u/%u MHz %s\n", iCpu, (int)idCpu, RTMpGetCurFrequency(idCpu),
                              RTMpGetMaxFrequency(idCpu), RTMpIsCpuPresent(idCpu) ? "present" : "absent");
                if (!RTCpuSetIsMemberByIndex(&Set, iCpu))
                    RTTestIFailed("online cpu with index %2d is not a member of the possible cpu set!\n", iCpu);
            }

        /* There isn't any sane way of testing RTMpIsCpuPresent really... :-/ */
    }
    else
        RTTestIFailed("RTMpGetPresentSet -> %p, expected %p\n", pSet, &Set);


    /* Find an online cpu for the next test. */
    RTCPUID idCpuOnline;
    for (idCpuOnline = 0; idCpuOnline < RTCPUSET_MAX_CPUS; idCpuOnline++)
        if (RTMpIsCpuOnline(idCpuOnline))
            break;

    /*
     * Quick test of RTMpGetDescription.
     */
    char szBuf[64];
    int rc = RTMpGetDescription(idCpuOnline, &szBuf[0], sizeof(szBuf));
    if (RT_SUCCESS(rc))
    {
        RTTestIPrintf(RTTESTLVL_ALWAYS, "RTMpGetDescription -> '%s'\n", szBuf);

        size_t cch = strlen(szBuf);
        rc = RTMpGetDescription(idCpuOnline, &szBuf[0], cch);
        if (rc != VERR_BUFFER_OVERFLOW)
            RTTestIFailed("RTMpGetDescription -> %Rrc, expected VERR_BUFFER_OVERFLOW\n", rc);

        rc = RTMpGetDescription(idCpuOnline, &szBuf[0], cch + 1);
        if (RT_FAILURE(rc))
            RTTestIFailed("RTMpGetDescription -> %Rrc, expected VINF_SUCCESS\n", rc);
    }
    else
        RTTestIFailed("RTMpGetDescription -> %Rrc\n", rc);

    return RTTestSummaryAndDestroy(hTest);
}

