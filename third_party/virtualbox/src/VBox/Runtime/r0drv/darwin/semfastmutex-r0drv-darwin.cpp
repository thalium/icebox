/* $Id: semfastmutex-r0drv-darwin.cpp $ */
/** @file
 * IPRT - Fast Mutex Semaphores, Ring-0 Driver, Darwin.
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
#include <iprt/semaphore.h>

#include <iprt/assert.h>
#include <iprt/asm.h>
#if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
# include <iprt/asm-amd64-x86.h>
#endif
#include <iprt/err.h>
#include <iprt/mem.h>
#include <iprt/mp.h>
#include <iprt/thread.h>

#include "internal/magics.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Wrapper for the darwin semaphore structure.
 */
typedef struct RTSEMFASTMUTEXINTERNAL
{
    /** Magic value (RTSEMFASTMUTEX_MAGIC). */
    uint32_t            u32Magic;
    /** The mutex. */
    lck_mtx_t          *pMtx;
} RTSEMFASTMUTEXINTERNAL, *PRTSEMFASTMUTEXINTERNAL;



RTDECL(int)  RTSemFastMutexCreate(PRTSEMFASTMUTEX phFastMtx)
{
    AssertCompile(sizeof(RTSEMFASTMUTEXINTERNAL) > sizeof(void *));
    AssertPtrReturn(phFastMtx, VERR_INVALID_POINTER);
    RT_ASSERT_PREEMPTIBLE();
    IPRT_DARWIN_SAVE_EFL_AC();

    PRTSEMFASTMUTEXINTERNAL pThis = (PRTSEMFASTMUTEXINTERNAL)RTMemAlloc(sizeof(*pThis));
    if (pThis)
    {
        pThis->u32Magic = RTSEMFASTMUTEX_MAGIC;
        Assert(g_pDarwinLockGroup);
        pThis->pMtx = lck_mtx_alloc_init(g_pDarwinLockGroup, LCK_ATTR_NULL);
        if (pThis->pMtx)
        {
            *phFastMtx = pThis;
            IPRT_DARWIN_RESTORE_EFL_AC();
            return VINF_SUCCESS;
        }

        RTMemFree(pThis);
    }
    IPRT_DARWIN_RESTORE_EFL_AC();
    return VERR_NO_MEMORY;
}


RTDECL(int)  RTSemFastMutexDestroy(RTSEMFASTMUTEX hFastMtx)
{
    PRTSEMFASTMUTEXINTERNAL pThis = hFastMtx;
    if (pThis == NIL_RTSEMFASTMUTEX)
        return VINF_SUCCESS;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertMsgReturn(pThis->u32Magic == RTSEMFASTMUTEX_MAGIC, ("%p: u32Magic=%RX32\n", pThis, pThis->u32Magic), VERR_INVALID_HANDLE);
    RT_ASSERT_INTS_ON();
    IPRT_DARWIN_SAVE_EFL_AC();

    ASMAtomicWriteU32(&pThis->u32Magic, RTSEMFASTMUTEX_MAGIC_DEAD);
    Assert(g_pDarwinLockGroup);
    lck_mtx_free(pThis->pMtx, g_pDarwinLockGroup);
    pThis->pMtx = NULL;
    RTMemFree(pThis);

    IPRT_DARWIN_RESTORE_EFL_AC();
    return VINF_SUCCESS;
}


RTDECL(int)  RTSemFastMutexRequest(RTSEMFASTMUTEX hFastMtx)
{
    PRTSEMFASTMUTEXINTERNAL pThis = hFastMtx;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertMsgReturn(pThis->u32Magic == RTSEMFASTMUTEX_MAGIC, ("%p: u32Magic=%RX32\n", pThis, pThis->u32Magic), VERR_INVALID_HANDLE);
    RT_ASSERT_PREEMPTIBLE();
    IPRT_DARWIN_SAVE_EFL_AC();

    lck_mtx_lock(pThis->pMtx);

    IPRT_DARWIN_RESTORE_EFL_ONLY_AC();
    return VINF_SUCCESS;
}


RTDECL(int)  RTSemFastMutexRelease(RTSEMFASTMUTEX hFastMtx)
{
    PRTSEMFASTMUTEXINTERNAL pThis = hFastMtx;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertMsgReturn(pThis->u32Magic == RTSEMFASTMUTEX_MAGIC, ("%p: u32Magic=%RX32\n", pThis, pThis->u32Magic), VERR_INVALID_HANDLE);
    RT_ASSERT_PREEMPTIBLE();
    IPRT_DARWIN_SAVE_EFL_AC();

    lck_mtx_unlock(pThis->pMtx);

    IPRT_DARWIN_RESTORE_EFL_ONLY_AC();
    return VINF_SUCCESS;
}

