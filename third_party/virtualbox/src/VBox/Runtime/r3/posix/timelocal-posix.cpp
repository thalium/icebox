/* $Id $ */
/** @file
 * IPRT - Local Time, Posix.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
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
#define RTTIME_INCL_TIMEVAL
#include <iprt/types.h>
#include <sys/time.h>
#include <time.h>

#include <iprt/time.h>


/**
 * This tries to find the UTC offset for a given timespec.
 *
 * It does probably not take into account changes in daylight
 * saving over the years or similar stuff.
 *
 * @returns UTC offset in nanoseconds.
 * @param   pTime           The time.
 * @param   fCurrentTime    Whether the input is current time or not.
 *                          This is for avoid infinit recursion on errors in the fallback path.
 */
static int64_t rtTimeLocalUTCOffset(PCRTTIMESPEC pTime, bool fCurrentTime)
{
    RTTIMESPEC Fallback;

    /*
     * Convert to time_t.
     */
    int64_t i64UnixTime = RTTimeSpecGetSeconds(pTime);
    time_t UnixTime = i64UnixTime;
    if (UnixTime != i64UnixTime)
        return fCurrentTime ? 0 : rtTimeLocalUTCOffset(RTTimeNow(&Fallback), true);

    /*
     * Explode it as both local and uct time.
     */
    struct tm TmLocal;
    if (    !localtime_r(&UnixTime, &TmLocal)
        ||  !TmLocal.tm_year)
        return fCurrentTime ? 0 : rtTimeLocalUTCOffset(RTTimeNow(&Fallback), true);
    struct tm TmUct;
    if (!gmtime_r(&UnixTime, &TmUct))
        return fCurrentTime ? 0 : rtTimeLocalUTCOffset(RTTimeNow(&Fallback), true);

    /*
     * Calc the difference (if any).
     * We ASSUME that the difference is less that 24 hours.
     */
    if (    TmLocal.tm_hour == TmUct.tm_hour
        &&  TmLocal.tm_min  == TmUct.tm_min
        &&  TmLocal.tm_sec  == TmUct.tm_sec
        &&  TmLocal.tm_mday == TmUct.tm_mday)
        return 0;

    int LocalSecs = TmLocal.tm_hour * 3600
                  + TmLocal.tm_min * 60
                  + TmLocal.tm_sec;
    int UctSecs   = TmUct.tm_hour * 3600
                  + TmUct.tm_min * 60
                  + TmUct.tm_sec;
    if (TmLocal.tm_mday != TmUct.tm_mday)
    {
        if (    (   TmLocal.tm_mday > TmUct.tm_mday
                 && TmUct.tm_mday != 1)
            ||  TmLocal.tm_mday == 1)
            LocalSecs += 24*60*60;
        else
            UctSecs   += 24*60*60;
    }

    return (LocalSecs - UctSecs) * INT64_C(1000000000);
}


/**
 * Gets the delta between UTC and local time.
 *
 * @code
 *      RTTIMESPEC LocalTime;
 *      RTTimeSpecAddNano(RTTimeNow(&LocalTime), RTTimeLocalDeltaNano());
 * @endcode
 *
 * @returns Returns the nanosecond delta between UTC and local time.
 */
RTDECL(int64_t) RTTimeLocalDeltaNano(void)
{
    RTTIMESPEC Time;
    return rtTimeLocalUTCOffset(RTTimeNow(&Time), true /* current time, skip fallback */);
}


/**
 * Explodes a time spec to the localized timezone.
 *
 * @returns pTime.
 * @param   pTime       Where to store the exploded time.
 * @param   pTimeSpec   The time spec to exploded. (UTC)
 */
RTDECL(PRTTIME) RTTimeLocalExplode(PRTTIME pTime, PCRTTIMESPEC pTimeSpec)
{
    RTTIMESPEC LocalTime = *pTimeSpec;
    RTTimeSpecAddNano(&LocalTime, rtTimeLocalUTCOffset(&LocalTime, true /* current time, skip fallback */));
    pTime = RTTimeExplode(pTime, &LocalTime);
    if (pTime)
        pTime->fFlags = (pTime->fFlags & ~RTTIME_FLAGS_TYPE_MASK) | RTTIME_FLAGS_TYPE_LOCAL;
    return pTime;
}

