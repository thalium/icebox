/* $Id: thread-r0drv-darwin.cpp $ */
/** @file
 * IPRT - Threads, Ring-0 Driver, Darwin.
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
#include "the-darwin-kernel.h"
#include "internal/iprt.h"
#include <iprt/thread.h>

#include <iprt/assert.h>
#include <iprt/err.h>



RTDECL(RTNATIVETHREAD) RTThreadNativeSelf(void)
{
    return (RTNATIVETHREAD)current_thread();
}


static int rtR0ThreadDarwinSleepCommon(RTMSINTERVAL cMillies)
{
    RT_ASSERT_PREEMPTIBLE();
    IPRT_DARWIN_SAVE_EFL_AC();

    uint64_t u64Deadline;
    clock_interval_to_deadline(cMillies, kMillisecondScale, &u64Deadline);
    clock_delay_until(u64Deadline);

    IPRT_DARWIN_RESTORE_EFL_AC();
    return VINF_SUCCESS;
}


RTDECL(int) RTThreadSleep(RTMSINTERVAL cMillies)
{
    return rtR0ThreadDarwinSleepCommon(cMillies);
}


RTDECL(int) RTThreadSleepNoLog(RTMSINTERVAL cMillies)
{
    return rtR0ThreadDarwinSleepCommon(cMillies);
}


RTDECL(bool) RTThreadYield(void)
{
    RT_ASSERT_PREEMPTIBLE();
    IPRT_DARWIN_SAVE_EFL_AC();

    thread_block(THREAD_CONTINUE_NULL);

    IPRT_DARWIN_RESTORE_EFL_AC();
    return true; /* this is fishy */
}

