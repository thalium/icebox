/* $Id: time2-win.cpp $ */
/** @file
 * IPRT - Time, Windows.
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
#include <iprt/win/windows.h>

#include <iprt/time.h>
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/err.h>
#include "internal/time.h"



RTDECL(int) RTTimeSet(PCRTTIMESPEC pTime)
{
    FILETIME    FileTime;
    SYSTEMTIME  SysTime;
    if (FileTimeToSystemTime(RTTimeSpecGetNtFileTime(pTime, &FileTime), &SysTime))
    {
        if (SetSystemTime(&SysTime))
            return VINF_SUCCESS;
    }
    return RTErrConvertFromWin32(GetLastError());
}


RTDECL(PRTTIME) RTTimeLocalExplode(PRTTIME pTime, PCRTTIMESPEC pTimeSpec)
{
    /*
     * FileTimeToLocalFileTime does not do the right thing, so we'll have
     * to convert to system time and SystemTimeToTzSpecificLocalTime instead.
     */
    RTTIMESPEC LocalTime;
    SYSTEMTIME SystemTimeIn;
    FILETIME FileTime;
    if (FileTimeToSystemTime(RTTimeSpecGetNtFileTime(pTimeSpec, &FileTime), &SystemTimeIn))
    {
        SYSTEMTIME SystemTimeOut;
        if (SystemTimeToTzSpecificLocalTime(NULL /* use current TZI */,
                                            &SystemTimeIn,
                                            &SystemTimeOut))
        {
            if (SystemTimeToFileTime(&SystemTimeOut, &FileTime))
            {
                RTTimeSpecSetNtFileTime(&LocalTime, &FileTime);
                pTime = RTTimeExplode(pTime, &LocalTime);
                if (pTime)
                    pTime->fFlags = (pTime->fFlags & ~RTTIME_FLAGS_TYPE_MASK) | RTTIME_FLAGS_TYPE_LOCAL;
                return pTime;
            }
        }
    }

    /*
     * The fallback is to use the current offset.
     * (A better fallback would be to use the offset of the same time of the year.)
     */
    LocalTime = *pTimeSpec;
    RTTimeSpecAddNano(&LocalTime, RTTimeLocalDeltaNano());
    pTime = RTTimeExplode(pTime, &LocalTime);
    if (pTime)
        pTime->fFlags = (pTime->fFlags & ~RTTIME_FLAGS_TYPE_MASK) | RTTIME_FLAGS_TYPE_LOCAL;
    return pTime;
}

