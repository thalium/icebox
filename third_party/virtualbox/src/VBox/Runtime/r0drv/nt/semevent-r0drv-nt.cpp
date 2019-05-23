/* $Id: semevent-r0drv-nt.cpp $ */
/** @file
 * IPRT -  Single Release Event Semaphores, Ring-0 Driver, NT.
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
#define RTSEMEVENT_WITHOUT_REMAPPING
#include "the-nt-kernel.h"
#include <iprt/semaphore.h>

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/lockvalidator.h>
#include <iprt/mem.h>
#include <iprt/time.h>
#include <iprt/timer.h>

#include "internal/magics.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * NT event semaphore.
 */
typedef struct RTSEMEVENTINTERNAL
{
    /** Magic value (RTSEMEVENT_MAGIC). */
    uint32_t volatile   u32Magic;
    /** Reference counter. */
    uint32_t volatile   cRefs;
    /** The NT Event object. */
    KEVENT              Event;
} RTSEMEVENTINTERNAL, *PRTSEMEVENTINTERNAL;


RTDECL(int)  RTSemEventCreate(PRTSEMEVENT phEventSem)
{
    return RTSemEventCreateEx(phEventSem, 0 /*fFlags*/, NIL_RTLOCKVALCLASS, NULL);
}


RTDECL(int)  RTSemEventCreateEx(PRTSEMEVENT phEventSem, uint32_t fFlags, RTLOCKVALCLASS hClass, const char *pszNameFmt, ...)
{
    AssertReturn(!(fFlags & ~(RTSEMEVENT_FLAGS_NO_LOCK_VAL | RTSEMEVENT_FLAGS_BOOTSTRAP_HACK)), VERR_INVALID_PARAMETER);
    Assert(!(fFlags & RTSEMEVENT_FLAGS_BOOTSTRAP_HACK) || (fFlags & RTSEMEVENT_FLAGS_NO_LOCK_VAL));
    AssertCompile(sizeof(RTSEMEVENTINTERNAL) > sizeof(void *));
    RT_NOREF2(hClass, pszNameFmt);

    PRTSEMEVENTINTERNAL pThis = (PRTSEMEVENTINTERNAL)RTMemAlloc(sizeof(*pThis));
    if (pThis)
    {
        pThis->u32Magic = RTSEMEVENT_MAGIC;
        pThis->cRefs    = 1;
        KeInitializeEvent(&pThis->Event, SynchronizationEvent, FALSE /* not signalled */);

        *phEventSem = pThis;
        return VINF_SUCCESS;
    }
    return VERR_NO_MEMORY;
}


/**
 * Retains a reference to the semaphore.
 *
 * @param   pThis       The semaphore to retain.
 */
DECLINLINE(void) rtR0SemEventNtRetain(PRTSEMEVENTINTERNAL pThis)
{
    uint32_t cRefs = ASMAtomicIncU32(&pThis->cRefs);
    Assert(cRefs < 100000); NOREF(cRefs);
}


/**
 * Releases a reference to the semaphore.
 *
 * @param   pThis       The semaphore to release
 */
DECLINLINE(void) rtR0SemEventNtRelease(PRTSEMEVENTINTERNAL pThis)
{
    if (ASMAtomicDecU32(&pThis->cRefs) == 0)
        RTMemFree(pThis);
}


RTDECL(int)  RTSemEventDestroy(RTSEMEVENT hEventSem)
{
    /*
     * Validate input.
     */
    PRTSEMEVENTINTERNAL pThis = hEventSem;
    if (pThis == NIL_RTSEMEVENT)
        return VINF_SUCCESS;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertMsgReturn(pThis->u32Magic == RTSEMEVENT_MAGIC, ("pThis->u32Magic=%RX32 pThis=%p\n", pThis->u32Magic, pThis), VERR_INVALID_HANDLE);

    /*
     * Invalidate it and signal the object just in case.
     */
    ASMAtomicIncU32(&pThis->u32Magic);
    KeSetEvent(&pThis->Event, 0xfff, FALSE);
    rtR0SemEventNtRelease(pThis);
    return VINF_SUCCESS;
}


RTDECL(int) RTSemEventSignal(RTSEMEVENT hEventSem)
{
    /*
     * Validate input.
     */
    PRTSEMEVENTINTERNAL pThis = (PRTSEMEVENTINTERNAL)hEventSem;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertMsgReturn(pThis->u32Magic == RTSEMEVENT_MAGIC, ("pThis->u32Magic=%RX32 pThis=%p\n", pThis->u32Magic, pThis), VERR_INVALID_HANDLE);
    rtR0SemEventNtRetain(pThis);

    /*
     * Signal the event object.
     */
    KeSetEvent(&pThis->Event, 1, FALSE);

    rtR0SemEventNtRelease(pThis);
    return VINF_SUCCESS;
}



/**
 * Worker for RTSemEventWaitEx and RTSemEventWaitExDebug.
 *
 * @returns VBox status code.
 * @param   pThis           The event semaphore.
 * @param   fFlags          See RTSemEventWaitEx.
 * @param   uTimeout        See RTSemEventWaitEx.
 * @param   pSrcPos         The source code position of the wait.
 */
DECLINLINE(int) rtR0SemEventNtWait(PRTSEMEVENTINTERNAL pThis, uint32_t fFlags, uint64_t uTimeout,
                                   PCRTLOCKVALSRCPOS pSrcPos)
{
    /*
     * Validate input.
     */
    if (!pThis)
        return VERR_INVALID_PARAMETER;
    AssertPtrReturn(pThis, VERR_INVALID_PARAMETER);
    AssertMsgReturn(pThis->u32Magic == RTSEMEVENT_MAGIC, ("%p u32Magic=%RX32\n", pThis, pThis->u32Magic), VERR_INVALID_PARAMETER);
    AssertReturn(RTSEMWAIT_FLAGS_ARE_VALID(fFlags), VERR_INVALID_PARAMETER);
    NOREF(pSrcPos);

    rtR0SemEventNtRetain(pThis);

    /*
     * Convert the timeout to a relative one because KeWaitForSingleObject
     * takes system time instead of interrupt time as input for absolute
     * timeout specifications.  So, we're best of by giving it relative time.
     *
     * Lazy bird converts uTimeout to relative nanoseconds and then to Nt time.
     */
    if (!(fFlags & RTSEMWAIT_FLAGS_INDEFINITE))
    {
        if (fFlags & RTSEMWAIT_FLAGS_MILLISECS)
            uTimeout = uTimeout < UINT64_MAX / UINT32_C(1000000) * UINT32_C(1000000)
                     ? uTimeout * UINT32_C(1000000)
                     : UINT64_MAX;
        if (uTimeout == UINT64_MAX)
            fFlags |= RTSEMWAIT_FLAGS_INDEFINITE;
        else
        {
            if (fFlags & RTSEMWAIT_FLAGS_ABSOLUTE)
            {
                uint64_t u64Now = RTTimeSystemNanoTS();
                uTimeout = u64Now < uTimeout
                         ? uTimeout - u64Now
                         : 0;
            }
        }
    }

    /*
     * Wait for it.
     * We're assuming interruptible waits should happen at UserMode level.
     */
    NTSTATUS        rcNt;
    BOOLEAN         fInterruptible = !!(fFlags & RTSEMWAIT_FLAGS_INTERRUPTIBLE);
    KPROCESSOR_MODE WaitMode   = fInterruptible ? UserMode : KernelMode;
    if (fFlags & RTSEMWAIT_FLAGS_INDEFINITE)
        rcNt = KeWaitForSingleObject(&pThis->Event, Executive, WaitMode, fInterruptible, NULL);
    else
    {
        LARGE_INTEGER Timeout;
        Timeout.QuadPart = -(int64_t)(uTimeout / 100);
        rcNt = KeWaitForSingleObject(&pThis->Event, Executive, WaitMode, fInterruptible, &Timeout);
    }
    int rc;
    if (pThis->u32Magic == RTSEMEVENT_MAGIC)
    {
        switch (rcNt)
        {
            case STATUS_SUCCESS:
                rc = VINF_SUCCESS;
                break;
            case STATUS_ALERTED:
                rc = VERR_INTERRUPTED;
                break;
            case STATUS_USER_APC:
                rc = VERR_INTERRUPTED;
                break;
            case STATUS_TIMEOUT:
                rc = VERR_TIMEOUT;
                break;
            default:
                AssertMsgFailed(("pThis->u32Magic=%RX32 pThis=%p: wait returned %lx!\n",
                                 pThis->u32Magic, pThis, (long)rcNt));
                rc = VERR_INTERNAL_ERROR_4;
                break;
        }
    }
    else
        rc = VERR_SEM_DESTROYED;

    rtR0SemEventNtRelease(pThis);
    return rc;
}


RTDECL(int)  RTSemEventWaitEx(RTSEMEVENT hEventSem, uint32_t fFlags, uint64_t uTimeout)
{
#ifndef RTSEMEVENT_STRICT
    return rtR0SemEventNtWait(hEventSem, fFlags, uTimeout, NULL);
#else
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_NORMAL_API();
    return rtR0SemEventNtWait(hEventSem, fFlags, uTimeout, &SrcPos);
#endif
}


RTDECL(int)  RTSemEventWaitExDebug(RTSEMEVENT hEventSem, uint32_t fFlags, uint64_t uTimeout,
                                   RTHCUINTPTR uId, RT_SRC_POS_DECL)
{
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_DEBUG_API();
    return rtR0SemEventNtWait(hEventSem, fFlags, uTimeout, &SrcPos);
}


RTDECL(uint32_t) RTSemEventGetResolution(void)
{
    return RTTimerGetSystemGranularity();
}

