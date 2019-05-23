/* $Id: semspinmutex-r3-generic.cpp $ */
/** @file
 * IPRT - Spinning Mutex Semaphores, Ring-3, Generic.
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
#include <iprt/semaphore.h>
#include "internal/iprt.h"

#include <iprt/alloc.h>
#include <iprt/err.h>
#include <iprt/assert.h>
#include <iprt/critsect.h>



RTDECL(int) RTSemSpinMutexCreate(PRTSEMSPINMUTEX phSpinMtx, uint32_t fFlags)
{
    AssertReturn(!(fFlags & ~RTSEMSPINMUTEX_FLAGS_VALID_MASK), VERR_INVALID_PARAMETER);
    AssertPtr(phSpinMtx);

    PRTCRITSECT pCritSect = (PRTCRITSECT)RTMemAlloc(sizeof(RTCRITSECT));
    if (!pCritSect)
        return VERR_NO_MEMORY;
    int rc = RTCritSectInitEx(pCritSect, RTCRITSECT_FLAGS_NO_NESTING | RTCRITSECT_FLAGS_NO_LOCK_VAL,
                              NIL_RTLOCKVALCLASS, RTLOCKVAL_SUB_CLASS_NONE, "RTSemSpinMutex");
    if (RT_SUCCESS(rc))
        *phSpinMtx = (RTSEMSPINMUTEX)pCritSect;
    else
        RTMemFree(pCritSect);
    return rc;
}
RT_EXPORT_SYMBOL(RTSemSpinMutexCreate);


RTDECL(int) RTSemSpinMutexDestroy(RTSEMSPINMUTEX hSpinMtx)
{
    if (hSpinMtx == NIL_RTSEMSPINMUTEX)
        return VERR_INVALID_PARAMETER;
    PRTCRITSECT pCritSect = (PRTCRITSECT)hSpinMtx;
    int rc = RTCritSectDelete(pCritSect);
    if (RT_SUCCESS(rc))
        RTMemFree(pCritSect);
    return rc;
}
RT_EXPORT_SYMBOL(RTSemSpinMutexDestroy);


RTDECL(int) RTSemSpinMutexTryRequest(RTSEMSPINMUTEX hSpinMtx)
{
    return RTCritSectTryEnter((PRTCRITSECT)hSpinMtx);

}
RT_EXPORT_SYMBOL(RTSemSpinMutexTryRequest);


RTDECL(int) RTSemSpinMutexRequest(RTSEMSPINMUTEX hSpinMtx)
{
    return RTCritSectEnter((PRTCRITSECT)hSpinMtx);
}
RT_EXPORT_SYMBOL(RTSemSpinMutexRequest);


RTDECL(int) RTSemSpinMutexRelease(RTSEMSPINMUTEX hSpinMtx)
{
    return RTCritSectLeave((PRTCRITSECT)hSpinMtx);
}
RT_EXPORT_SYMBOL(RTSemSpinMutexRelease);

