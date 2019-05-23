/* $Id: thread2-win.cpp $ */
/** @file
 * IPRT - Threads part 2, Windows.
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
#define LOG_GROUP RTLOGGROUP_THREAD
#include <iprt/win/windows.h>

#include <iprt/thread.h>
#include "internal/iprt.h"

#include <iprt/asm-amd64-x86.h>
#include <iprt/err.h>
#include <iprt/log.h>
#include "internal/thread.h"


RTDECL(RTNATIVETHREAD) RTThreadNativeSelf(void)
{
    return (RTNATIVETHREAD)GetCurrentThreadId();
}


RTR3DECL(int)   RTThreadSleep(RTMSINTERVAL cMillies)
{
    LogFlow(("RTThreadSleep: cMillies=%d\n", cMillies));
    Sleep(cMillies);
    LogFlow(("RTThreadSleep: returning %Rrc (cMillies=%d)\n", VINF_SUCCESS, cMillies));
    return VINF_SUCCESS;
}


RTR3DECL(int)   RTThreadSleepNoLog(RTMSINTERVAL cMillies)
{
    Sleep(cMillies);
    return VINF_SUCCESS;
}


RTR3DECL(bool) RTThreadYield(void)
{
    uint64_t u64TS = ASMReadTSC();
    Sleep(0);
    u64TS = ASMReadTSC() - u64TS;
    bool fRc = u64TS > 1500;
    LogFlow(("RTThreadYield: returning %d (%llu ticks)\n", fRc, u64TS));
    return fRc;
}

