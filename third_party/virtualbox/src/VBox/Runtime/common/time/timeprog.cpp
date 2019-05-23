/* $Id: timeprog.cpp $ */
/** @file
 * IPRT - Time Relative to Program Start.
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
#include <iprt/time.h>
#include "internal/iprt.h"

#include <iprt/assert.h>
#include "internal/time.h"



/**
 * Get the nanosecond timestamp relative to program startup.
 *
 * @returns Timestamp relative to program startup.
 */
RTDECL(uint64_t)  RTTimeProgramNanoTS(void)
{
    return RTTimeNanoTS() - g_u64ProgramStartNanoTS;
}
RT_EXPORT_SYMBOL(RTTimeProgramNanoTS);


/**
 * Get the microsecond timestamp relative to program startup.
 *
 * @returns Timestamp relative to program startup.
 */
RTDECL(uint64_t)  RTTimeProgramMicroTS(void)
{
    return RTTimeProgramNanoTS() / 1000;
}
RT_EXPORT_SYMBOL(RTTimeProgramMicroTS);


/**
 * Get the millisecond timestamp relative to program startup.
 *
 * @returns Timestamp relative to program startup.
 */
RTDECL(uint64_t)  RTTimeProgramMilliTS(void)
{
    return RTTimeMilliTS() - g_u64ProgramStartMilliTS;
}
RT_EXPORT_SYMBOL(RTTimeProgramMilliTS);


/**
 * Get the second timestamp relative to program startup.
 *
 * @returns Timestamp relative to program startup.
 */
RTDECL(uint32_t)  RTTimeProgramSecTS(void)
{
    AssertMsg(g_u64ProgramStartMilliTS, ("rtR3Init hasn't been called!\n"));
    return (uint32_t)(RTTimeProgramMilliTS() / 1000);
}
RT_EXPORT_SYMBOL(RTTimeProgramSecTS);


/**
 * Get the RTTimeNanoTS() of when the program started.
 *
 * @returns Program startup timestamp.
 */
RTDECL(uint64_t) RTTimeProgramStartNanoTS(void)
{
    return g_u64ProgramStartNanoTS;
}
RT_EXPORT_SYMBOL(RTTimeProgramStartNanoTS);

