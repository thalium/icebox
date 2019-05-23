/* $Id: semfastmutex-r0drv-haiku.c $ */
/** @file
 * IPRT - Fast Mutex Semaphores, Ring-0 Driver, Haiku.
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
#include "the-haiku-kernel.h"

#include <iprt/semaphore.h>
#include <iprt/err.h>
#include <iprt/alloc.h>
#include <iprt/assert.h>
#include <iprt/asm.h>

#include "internal/magics.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Wrapper for the Haiku (sleep) mutex.
 */
typedef struct RTSEMFASTMUTEXINTERNAL
{
    /** Magic value (RTSEMFASTMUTEX_MAGIC). */
    uint32_t            u32Magic;
    /** A good old Benaphore. */
    vint32              BenId;
    sem_id              SemId;
} RTSEMFASTMUTEXINTERNAL, *PRTSEMFASTMUTEXINTERNAL;


RTDECL(int)  RTSemFastMutexCreate(PRTSEMFASTMUTEX phFastMtx)
{
    AssertCompile(sizeof(RTSEMFASTMUTEXINTERNAL) > sizeof(void *));
    AssertPtrReturn(phFastMtx, VERR_INVALID_POINTER);

    PRTSEMFASTMUTEXINTERNAL pThis = (PRTSEMFASTMUTEXINTERNAL)RTMemAllocZ(sizeof(*pThis));
    if (RT_UNLIKELY(!pThis))
        return VERR_NO_MEMORY;

    pThis->u32Magic = RTSEMFASTMUTEX_MAGIC;
    pThis->BenId = 0;
    pThis->SemId = create_sem(0, "IPRT Fast Mutex Semaphore");
    if (pThis->SemId >= B_OK)
    {
        *phFastMtx = pThis;
        return VINF_SUCCESS;
    }
    RTMemFree(pThis);
    return VERR_TOO_MANY_SEMAPHORES;     /** @todo r=ramshankar: use RTErrConvertFromHaikuKernReturn */
}


RTDECL(int)  RTSemFastMutexDestroy(RTSEMFASTMUTEX hFastMtx)
{
    PRTSEMFASTMUTEXINTERNAL pThis = hFastMtx;
    if (pThis == NIL_RTSEMFASTMUTEX)
        return VINF_SUCCESS;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertMsgReturn(pThis->u32Magic == RTSEMFASTMUTEX_MAGIC, ("%p: u32Magic=%RX32\n", pThis, pThis->u32Magic), VERR_INVALID_HANDLE);

    ASMAtomicWriteU32(&pThis->u32Magic, RTSEMFASTMUTEX_MAGIC_DEAD);
    delete_sem(pThis->SemId);
    RTMemFree(pThis);

    return VINF_SUCCESS;
}


RTDECL(int)  RTSemFastMutexRequest(RTSEMFASTMUTEX hFastMtx)
{
    PRTSEMFASTMUTEXINTERNAL pThis = hFastMtx;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertMsgReturn(pThis->u32Magic == RTSEMFASTMUTEX_MAGIC, ("%p: u32Magic=%RX32\n", pThis, pThis->u32Magic), VERR_INVALID_HANDLE);

    if (atomic_add(&pThis->BenId, 1) > 0)
        acquire_sem(pThis->SemId);

    return VINF_SUCCESS;
}


RTDECL(int)  RTSemFastMutexRelease(RTSEMFASTMUTEX hFastMtx)
{
    PRTSEMFASTMUTEXINTERNAL pThis = hFastMtx;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertMsgReturn(pThis->u32Magic == RTSEMFASTMUTEX_MAGIC, ("%p: u32Magic=%RX32\n", pThis, pThis->u32Magic), VERR_INVALID_HANDLE);

    if (atomic_add(&pThis->BenId, -1) > 1)
        release_sem(pThis->SemId);

    return VINF_SUCCESS;
}

