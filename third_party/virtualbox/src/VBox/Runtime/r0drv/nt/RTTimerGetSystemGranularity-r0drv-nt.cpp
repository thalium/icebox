/* $Id: RTTimerGetSystemGranularity-r0drv-nt.cpp $ */
/** @file
 * IPRT - RTTimerGetSystemGranularity, Ring-0 Driver, NT.
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
#include "the-nt-kernel.h"

#include <iprt/timer.h>
#include <iprt/err.h>
#include <iprt/asm.h>
#include <iprt/assert.h>

#include "internal-r0drv-nt.h"
#include "internal/magics.h"


RTDECL(uint32_t) RTTimerGetSystemGranularity(void)
{
    /*
     * Get the default/max timer increment value, return it if ExSetTimerResolution
     * isn't available. According to the sysinternals guys NtQueryTimerResolution
     * is only available in userland and they find it equally annoying.
     */
    ULONG ulTimeInc = KeQueryTimeIncrement();
    if (!g_pfnrtNtExSetTimerResolution)
        return ulTimeInc * 100; /* The value is in 100ns, the funny NT unit. */

    /*
     * Use the value returned by ExSetTimerResolution. Since the kernel is keeping
     * count of these calls, we have to do two calls that cancel each other out.
     */
    g_pfnrtNtExSetTimerResolution(ulTimeInc, TRUE);
    ULONG ulResolution = g_pfnrtNtExSetTimerResolution(0 /*ignored*/, FALSE);
    return ulResolution * 100; /* NT -> ns */
}

