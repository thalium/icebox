/* $Id: RTSemMutexRequest-generic.cpp $ */
/** @file
 * IPRT - RTSemMutexRequest, generic RTSemMutexRequestNoResume wrapper.
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
#define RTSEMMUTEX_WITHOUT_REMAPPING
#define LOG_GROUP RTLOGGROUP_SEM
#include <iprt/semaphore.h>
#include "internal/iprt.h"

#include <iprt/time.h>
#include <iprt/err.h>
#include <iprt/assert.h>


RTDECL(int) RTSemMutexRequest(RTSEMMUTEX Mutex, RTMSINTERVAL cMillies)
{
    int rc;
    if (cMillies == RT_INDEFINITE_WAIT)
    {
        do rc = RTSemMutexRequestNoResume(Mutex, cMillies);
        while (rc == VERR_INTERRUPTED);
    }
    else
    {
        const uint64_t u64Start = RTTimeMilliTS();
        rc = RTSemMutexRequestNoResume(Mutex, cMillies);
        if (rc == VERR_INTERRUPTED)
        {
            do
            {
                uint64_t u64Elapsed = RTTimeMilliTS() - u64Start;
                if (u64Elapsed >= cMillies)
                    return VERR_TIMEOUT;
                rc = RTSemMutexRequestNoResume(Mutex, cMillies - (RTMSINTERVAL)u64Elapsed);
            } while (rc == VERR_INTERRUPTED);
        }
    }
    return rc;
}
RT_EXPORT_SYMBOL(RTSemMutexRequest);

