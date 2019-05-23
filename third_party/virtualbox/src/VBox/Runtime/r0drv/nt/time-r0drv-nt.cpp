/* $Id: time-r0drv-nt.cpp $ */
/** @file
 * IPRT - Time, Ring-0 Driver, Nt.
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
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
#define LOG_GROUP RTLOGGROUP_TIME
#include "the-nt-kernel.h"
#include "internal-r0drv-nt.h"
#include <iprt/time.h>


DECLINLINE(uint64_t) rtTimeGetSystemNanoTS(void)
{
    /*
     * Note! The time source we use here must be exactly the same as in
     *       the ring-3 code!
     *
     * Using interrupt time is the simplest and requires the least calculation.
     * It is also accounting for suspended time. Unfortuantely, there is no
     * ring-3 for reading it... but that won't stop us.
     *
     * Using the tick count is problematic in ring-3 on older windows version
     * as we can only get the 32-bit tick value, i.e. we'll roll over sooner or
     * later.
     */
#if 1
    /* Interrupt time. */
    LARGE_INTEGER InterruptTime;
    if (g_pfnrtKeQueryInterruptTimePrecise)
    {
        ULONG64 QpcTsIgnored;
        InterruptTime.QuadPart = g_pfnrtKeQueryInterruptTimePrecise(&QpcTsIgnored);
    }
# ifdef RT_ARCH_AMD64
    else
        InterruptTime.QuadPart = KeQueryInterruptTime(); /* macro */
# else
    else if (g_pfnrtKeQueryInterruptTime)
        InterruptTime.QuadPart = g_pfnrtKeQueryInterruptTime();
    else
    {
        /* NT4 (no API) and pre-init fallback. */
        do
        {
            InterruptTime.HighPart = ((KUSER_SHARED_DATA volatile *)SharedUserData)->InterruptTime.High1Time;
            InterruptTime.LowPart = ((KUSER_SHARED_DATA volatile *)SharedUserData)->InterruptTime.LowPart;
        } while (((KUSER_SHARED_DATA volatile *)SharedUserData)->InterruptTime.High2Time != InterruptTime.HighPart);
    }
# endif
    return (uint64_t)InterruptTime.QuadPart * 100;
#else
    /* Tick Count (NT4 SP1 has these APIs, haven't got SP0 to check). */
    LARGE_INTEGER Tick;
    KeQueryTickCount(&Tick);
    return (uint64_t)Tick.QuadPart * KeQueryTimeIncrement() * 100;
#endif
}


RTDECL(uint64_t) RTTimeNanoTS(void)
{
    return rtTimeGetSystemNanoTS();
}


RTDECL(uint64_t) RTTimeMilliTS(void)
{
    return rtTimeGetSystemNanoTS() / RT_NS_1MS;
}


RTDECL(uint64_t) RTTimeSystemNanoTS(void)
{
    return rtTimeGetSystemNanoTS();
}


RTDECL(uint64_t) RTTimeSystemMilliTS(void)
{
    return rtTimeGetSystemNanoTS() / RT_NS_1MS;
}


RTDECL(PRTTIMESPEC) RTTimeNow(PRTTIMESPEC pTime)
{
    LARGE_INTEGER SystemTime;
    if (g_pfnrtKeQuerySystemTimePrecise)
        g_pfnrtKeQuerySystemTimePrecise(&SystemTime);
#ifdef RT_ARCH_AMD64
    else
        KeQuerySystemTime(&SystemTime); /* macro */
#else
    else if (g_pfnrtKeQuerySystemTime)
        g_pfnrtKeQuerySystemTime(&SystemTime);
    else
    {
        do
        {
            SystemTime.HighPart = ((KUSER_SHARED_DATA volatile *)SharedUserData)->SystemTime.High1Time;
            SystemTime.LowPart = ((KUSER_SHARED_DATA volatile *)SharedUserData)->SystemTime.LowPart;
        } while (((KUSER_SHARED_DATA volatile *)SharedUserData)->SystemTime.High2Time != SystemTime.HighPart);
    }
#endif
    return RTTimeSpecSetNtTime(pTime, SystemTime.QuadPart);
}

