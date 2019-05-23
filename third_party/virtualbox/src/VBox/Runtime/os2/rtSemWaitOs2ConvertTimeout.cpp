/* $Id: rtSemWaitOs2ConvertTimeout.cpp $ */
/** @file
 * IPRT - RTSemEventMultiWait, implementation based on RTSemEventMultiWaitEx.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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
#include "internal/iprt.h"
#include <iprt/semaphore.h>
#include <iprt/time.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** Too lazy to include the right OS/2 header, duplicating the define we
 * need here. */
#define MY_SEM_INDEFINITE_WAIT UINT32_MAX


/**
 * Converts the timeout to a millisecond value that can be fed to KernBlock.
 *
 * @returns Relative timeout in milliseconds.
 * @param   fFlags      The semaphore wait flags.
 * @param   uTimeout    The timeout value.
 */
uint32_t rtR0SemWaitOs2ConvertTimeout(uint32_t fFlags, uint64_t uTimeout)
{
    /*
     * Simple & common cases.
     */
    if (fFlags & RTSEMWAIT_FLAGS_INDEFINITE)
        return MY_SEM_INDEFINITE_WAIT;

    if (   (fFlags & (RTSEMWAIT_FLAGS_MILLISECS | RTSEMWAIT_FLAGS_ABSOLUTE))
        == RTSEMWAIT_FLAGS_MILLISECS)
    {
        if (uTimeout < UINT32_MAX)
            return (uint32_t)uTimeout;
        return MY_SEM_INDEFINITE_WAIT;
    }

    if (!uTimeout)
        return 0;

    if (uTimeout == UINT64_MAX)
        return MY_SEM_INDEFINITE_WAIT;

    /*
     * For the more complicated cases (nano or/and abs), convert thru nano (lazy bird).
     */
    if (fFlags & RTSEMWAIT_FLAGS_MILLISECS)
    {
        if (uTimeout >= UINT64_MAX / RT_NS_1MS * RT_NS_1MS)
            return MY_SEM_INDEFINITE_WAIT;
        uTimeout = uTimeout * RT_NS_1MS;
    }
    if (fFlags & RTSEMWAIT_FLAGS_ABSOLUTE)
    {
        uint64_t u64Now = RTTimeSystemNanoTS();
        if (u64Now >= uTimeout)
            return 0;
        uTimeout = uTimeout - u64Now;
    }
    uTimeout /= RT_NS_1MS;
    if (uTimeout >= UINT32_MAX)
        return MY_SEM_INDEFINITE_WAIT;
    return (uint32_t)uTimeout;
}

