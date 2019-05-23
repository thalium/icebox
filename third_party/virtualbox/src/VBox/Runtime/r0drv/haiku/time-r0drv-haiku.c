/* $Id: time-r0drv-haiku.c $ */
/** @file
 * IPRT - Time, Ring-0 Driver, Haiku.
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
#define LOG_GROUP RTLOGGROUP_TIME
#include "the-haiku-kernel.h"
#include "internal/iprt.h"
#include <iprt/time.h>

#include <iprt/asm.h>


DECLINLINE(uint64_t) rtTimeGetSystemNanoTS(void)
{
    return system_time() * RT_NS_1US;
}


DECLINLINE(uint64_t) rtTimeGetSystemMilliTS(void)
{
    return system_time() / RT_NS_1US;
}


RTDECL(uint64_t) RTTimeNanoTS(void)
{
    return rtTimeGetSystemNanoTS();
}


RTDECL(uint64_t) RTTimeMilliTS(void)
{
    return rtTimeGetSystemMilliTS();
}


RTDECL(uint64_t) RTTimeSystemNanoTS(void)
{
    return rtTimeGetSystemNanoTS();
}


RTDECL(uint64_t) RTTimeSystemMilliTS(void)
{
    return rtTimeGetSystemMilliTS();
}


RTDECL(PRTTIMESPEC) RTTimeNow(PRTTIMESPEC pTime)
{
    return RTTimeSpecSetNano(pTime, real_time_clock_usecs() * RT_NS_1US);
}

