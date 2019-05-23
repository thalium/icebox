/* $Id: time-r0drv-solaris.c $ */
/** @file
 * IPRT - Time, Ring-0 Driver, Solaris.
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
#define RTTIME_INCL_TIMESPEC
#include "the-solaris-kernel.h"
#include "internal/iprt.h"
#include <iprt/time.h>


RTDECL(uint64_t) RTTimeNanoTS(void)
{
    return (uint64_t)gethrtime();
}


RTDECL(uint64_t) RTTimeMilliTS(void)
{
    return RTTimeNanoTS() / RT_NS_1MS;
}


RTDECL(uint64_t) RTTimeSystemNanoTS(void)
{
    return RTTimeNanoTS();
}


RTDECL(uint64_t) RTTimeSystemMilliTS(void)
{
    return RTTimeNanoTS() / RT_NS_1MS;
}


RTDECL(PRTTIMESPEC) RTTimeNow(PRTTIMESPEC pTime)
{
    timestruc_t TimeSpec;

    mutex_enter(&tod_lock);
    TimeSpec = tod_get();
    mutex_exit(&tod_lock);
    return RTTimeSpecSetNano(pTime, (uint64_t)TimeSpec.tv_sec * RT_NS_1SEC + TimeSpec.tv_nsec);
}

