/* $Id: semevent-linux.cpp $ */
/** @file
 * IPRT - Event Semaphore, Linux (2.6.x+).
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

#include <features.h>
#if __GLIBC_PREREQ(2,6) && !defined(IPRT_WITH_FUTEX_BASED_SEMS)

/*
 * glibc 2.6 fixed a serious bug in the mutex implementation. We wrote this
 * linux specific event semaphores code in order to work around the bug. We
 * will fall back on the pthread-based implementation if glibc is known to
 * contain the bug fix.
 *
 * The external reference to epoll_pwait is a hack which prevents that we link
 * against glibc < 2.6.
 */
#include "../posix/semevent-posix.cpp"
asm volatile (".global epoll_pwait");

#else /* glibc < 2.6 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/semaphore.h>
#include "internal/iprt.h"

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/lockvalidator.h>
#include <iprt/mem.h>
#include <iprt/time.h>
#include "internal/magics.h"
#include "internal/mem.h"
#include "internal/strict.h"

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/syscall.h>
#if 0 /* With 2.6.17 futex.h has become C++ unfriendly. */
# include <linux/futex.h>
#else
# define FUTEX_WAIT 0
# define FUTEX_WAKE 1
#endif


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Linux (single wakup) event semaphore.
 */
struct RTSEMEVENTINTERNAL
{
    /** Magic value. */
    intptr_t volatile   iMagic;
    /** The futex state variable.
     * 0 means not signalled.
       1 means signalled. */
    uint32_t volatile   fSignalled;
    /** The number of waiting threads */
    int32_t volatile    cWaiters;
#ifdef RTSEMEVENT_STRICT
    /** Signallers. */
    RTLOCKVALRECSHRD    Signallers;
    /** Indicates that lock validation should be performed. */
    bool volatile       fEverHadSignallers;
#endif
    /** The creation flags. */
    uint32_t            fFlags;
};


/**
 * Wrapper for the futex syscall.
 */
static long sys_futex(uint32_t volatile *uaddr, int op, int val, struct timespec *utime, int32_t *uaddr2, int val3)
{
    errno = 0;
    long rc = syscall(__NR_futex, uaddr, op, val, utime, uaddr2, val3);
    if (rc < 0)
    {
        Assert(rc == -1);
        rc = -errno;
    }
    return rc;
}



RTDECL(int)  RTSemEventCreate(PRTSEMEVENT phEventSem)
{
    return RTSemEventCreateEx(phEventSem, 0 /*fFlags*/, NIL_RTLOCKVALCLASS, NULL);
}


RTDECL(int)  RTSemEventCreateEx(PRTSEMEVENT phEventSem, uint32_t fFlags, RTLOCKVALCLASS hClass, const char *pszNameFmt, ...)
{
    AssertReturn(!(fFlags & ~(RTSEMEVENT_FLAGS_NO_LOCK_VAL | RTSEMEVENT_FLAGS_BOOTSTRAP_HACK)), VERR_INVALID_PARAMETER);
    Assert(!(fFlags & RTSEMEVENT_FLAGS_BOOTSTRAP_HACK) || (fFlags & RTSEMEVENT_FLAGS_NO_LOCK_VAL));

    /*
     * Allocate semaphore handle.
     */
    struct RTSEMEVENTINTERNAL *pThis;
    if (!(fFlags & RTSEMEVENT_FLAGS_BOOTSTRAP_HACK))
        pThis = (struct RTSEMEVENTINTERNAL *)RTMemAlloc(sizeof(struct RTSEMEVENTINTERNAL));
    else
        pThis = (struct RTSEMEVENTINTERNAL *)rtMemBaseAlloc(sizeof(struct RTSEMEVENTINTERNAL));
    if (pThis)
    {
        pThis->iMagic = RTSEMEVENT_MAGIC;
        pThis->cWaiters = 0;
        pThis->fSignalled = 0;
        pThis->fFlags = fFlags;
#ifdef RTSEMEVENT_STRICT
        if (!pszNameFmt)
        {
            static uint32_t volatile s_iSemEventAnon = 0;
            RTLockValidatorRecSharedInit(&pThis->Signallers, hClass, RTLOCKVAL_SUB_CLASS_ANY, pThis,
                                         true /*fSignaller*/, !(fFlags & RTSEMEVENT_FLAGS_NO_LOCK_VAL),
                                         "RTSemEvent-%u", ASMAtomicIncU32(&s_iSemEventAnon) - 1);
        }
        else
        {
            va_list va;
            va_start(va, pszNameFmt);
            RTLockValidatorRecSharedInitV(&pThis->Signallers, hClass, RTLOCKVAL_SUB_CLASS_ANY, pThis,
                                          true /*fSignaller*/, !(fFlags & RTSEMEVENT_FLAGS_NO_LOCK_VAL),
                                          pszNameFmt, va);
            va_end(va);
        }
        pThis->fEverHadSignallers = false;
#else
        RT_NOREF(hClass, pszNameFmt);
#endif

        *phEventSem = pThis;
        return VINF_SUCCESS;
    }
    return  VERR_NO_MEMORY;
}


RTDECL(int)  RTSemEventDestroy(RTSEMEVENT hEventSem)
{
    /*
     * Validate input.
     */
    struct RTSEMEVENTINTERNAL *pThis = hEventSem;
    if (pThis == NIL_RTSEMEVENT)
        return VINF_SUCCESS;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->iMagic == RTSEMEVENT_MAGIC, VERR_INVALID_HANDLE);

    /*
     * Invalidate the semaphore and wake up anyone waiting on it.
     */
    ASMAtomicXchgSize(&pThis->iMagic, RTSEMEVENT_MAGIC | UINT32_C(0x80000000));
    if (ASMAtomicXchgS32(&pThis->cWaiters, INT32_MIN / 2) > 0)
    {
        sys_futex(&pThis->fSignalled, FUTEX_WAKE, INT_MAX, NULL, NULL, 0);
        usleep(1000);
    }

    /*
     * Free the semaphore memory and be gone.
     */
#ifdef RTSEMEVENT_STRICT
    RTLockValidatorRecSharedDelete(&pThis->Signallers);
#endif
    if (!(pThis->fFlags & RTSEMEVENT_FLAGS_BOOTSTRAP_HACK))
        RTMemFree(pThis);
    else
        rtMemBaseFree(pThis);
    return VINF_SUCCESS;
}


RTDECL(int)  RTSemEventSignal(RTSEMEVENT hEventSem)
{
    /*
     * Validate input.
     */
    struct RTSEMEVENTINTERNAL *pThis = hEventSem;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->iMagic == RTSEMEVENT_MAGIC, VERR_INVALID_HANDLE);

#ifdef RTSEMEVENT_STRICT
    if (pThis->fEverHadSignallers)
    {
        int rc9 = RTLockValidatorRecSharedCheckSignaller(&pThis->Signallers, NIL_RTTHREAD);
        if (RT_FAILURE(rc9))
            return rc9;
    }
#endif

    ASMAtomicWriteU32(&pThis->fSignalled, 1);
    if (ASMAtomicReadS32(&pThis->cWaiters) < 1)
        return VINF_SUCCESS;

    /* somebody is waiting, try wake up one of them. */
    long cWoken = sys_futex(&pThis->fSignalled, FUTEX_WAKE, 1, NULL, NULL, 0);
    if (RT_LIKELY(cWoken >= 0))
        return VINF_SUCCESS;

    if (RT_UNLIKELY(pThis->iMagic != RTSEMEVENT_MAGIC))
        return VERR_SEM_DESTROYED;

    return VERR_INVALID_PARAMETER;
}


static int rtSemEventWait(RTSEMEVENT hEventSem, RTMSINTERVAL cMillies, bool fAutoResume)
{
#ifdef RTSEMEVENT_STRICT
    PCRTLOCKVALSRCPOS pSrcPos = NULL;
#endif

    /*
     * Validate input.
     */
    struct RTSEMEVENTINTERNAL *pThis = hEventSem;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->iMagic == RTSEMEVENT_MAGIC, VERR_INVALID_HANDLE);

    /*
     * Quickly check whether it's signaled.
     */
    /** @todo this isn't fair if someone is already waiting on it.  They should
     *        have the first go at it!
     *  (ASMAtomicReadS32(&pThis->cWaiters) == 0 || !cMillies) && ... */
    if (ASMAtomicCmpXchgU32(&pThis->fSignalled, 0, 1))
        return VINF_SUCCESS;

    /*
     * Convert the timeout value.
     */
    struct timespec ts;
    struct timespec *pTimeout = NULL;
    uint64_t u64End = 0; /* shut up gcc */
    if (cMillies != RT_INDEFINITE_WAIT)
    {
        if (!cMillies)
            return VERR_TIMEOUT;
        ts.tv_sec  = cMillies / 1000;
        ts.tv_nsec = (cMillies % 1000) * UINT32_C(1000000);
        u64End = RTTimeSystemNanoTS() + cMillies * UINT64_C(1000000);
        pTimeout = &ts;
    }

    ASMAtomicIncS32(&pThis->cWaiters);

    /*
     * The wait loop.
     */
#ifdef RTSEMEVENT_STRICT
    RTTHREAD hThreadSelf = !(pThis->fFlags & RTSEMEVENT_FLAGS_BOOTSTRAP_HACK)
                         ? RTThreadSelfAutoAdopt()
                         : RTThreadSelf();
#else
    RTTHREAD hThreadSelf = RTThreadSelf();
#endif
    int rc = VINF_SUCCESS;
    for (;;)
    {
#ifdef RTSEMEVENT_STRICT
        if (pThis->fEverHadSignallers)
        {
            rc = RTLockValidatorRecSharedCheckBlocking(&pThis->Signallers, hThreadSelf, pSrcPos, false,
                                                       cMillies, RTTHREADSTATE_EVENT, true);
            if (RT_FAILURE(rc))
                break;
        }
#endif
        RTThreadBlocking(hThreadSelf, RTTHREADSTATE_EVENT, true);
        long lrc = sys_futex(&pThis->fSignalled, FUTEX_WAIT, 0, pTimeout, NULL, 0);
        RTThreadUnblocked(hThreadSelf, RTTHREADSTATE_EVENT);
        if (RT_UNLIKELY(pThis->iMagic != RTSEMEVENT_MAGIC))
        {
            rc = VERR_SEM_DESTROYED;
            break;
        }

        if (RT_LIKELY(lrc == 0 || lrc == -EWOULDBLOCK))
        {
            /* successful wakeup or fSignalled > 0 in the meantime */
            if (ASMAtomicCmpXchgU32(&pThis->fSignalled, 0, 1))
                break;
        }
        else if (lrc == -ETIMEDOUT)
        {
            rc = VERR_TIMEOUT;
            break;
        }
        else if (lrc == -EINTR)
        {
            if (!fAutoResume)
            {
                rc = VERR_INTERRUPTED;
                break;
            }
        }
        else
        {
            /* this shouldn't happen! */
            AssertMsgFailed(("rc=%ld errno=%d\n", lrc, errno));
            rc = RTErrConvertFromErrno(lrc);
            break;
        }
        /* adjust the relative timeout */
        if (pTimeout)
        {
            int64_t i64Diff = u64End - RTTimeSystemNanoTS();
            if (i64Diff < 1000)
            {
                rc = VERR_TIMEOUT;
                break;
            }
            ts.tv_sec  = (uint64_t)i64Diff / UINT32_C(1000000000);
            ts.tv_nsec = (uint64_t)i64Diff % UINT32_C(1000000000);
        }
    }

    ASMAtomicDecS32(&pThis->cWaiters);
    return rc;
}


RTDECL(int)  RTSemEventWait(RTSEMEVENT hEventSem, RTMSINTERVAL cMillies)
{
    int rc = rtSemEventWait(hEventSem, cMillies, true);
    Assert(rc != VERR_INTERRUPTED);
    Assert(rc != VERR_TIMEOUT || cMillies != RT_INDEFINITE_WAIT);
    return rc;
}


RTDECL(int)  RTSemEventWaitNoResume(RTSEMEVENT hEventSem, RTMSINTERVAL cMillies)
{
    return rtSemEventWait(hEventSem, cMillies, false);
}


RTDECL(void) RTSemEventSetSignaller(RTSEMEVENT hEventSem, RTTHREAD hThread)
{
#ifdef RTSEMEVENT_STRICT
    struct RTSEMEVENTINTERNAL *pThis = hEventSem;
    AssertPtrReturnVoid(pThis);
    AssertReturnVoid(pThis->iMagic == RTSEMEVENT_MAGIC);

    ASMAtomicWriteBool(&pThis->fEverHadSignallers, true);
    RTLockValidatorRecSharedResetOwner(&pThis->Signallers, hThread, NULL);
#else
    RT_NOREF(hEventSem, hThread);
#endif
}


RTDECL(void) RTSemEventAddSignaller(RTSEMEVENT hEventSem, RTTHREAD hThread)
{
#ifdef RTSEMEVENT_STRICT
    struct RTSEMEVENTINTERNAL *pThis = hEventSem;
    AssertPtrReturnVoid(pThis);
    AssertReturnVoid(pThis->iMagic == RTSEMEVENT_MAGIC);

    ASMAtomicWriteBool(&pThis->fEverHadSignallers, true);
    RTLockValidatorRecSharedAddOwner(&pThis->Signallers, hThread, NULL);
#else
    RT_NOREF(hEventSem, hThread);
#endif
}


RTDECL(void) RTSemEventRemoveSignaller(RTSEMEVENT hEventSem, RTTHREAD hThread)
{
#ifdef RTSEMEVENT_STRICT
    struct RTSEMEVENTINTERNAL *pThis = hEventSem;
    AssertPtrReturnVoid(pThis);
    AssertReturnVoid(pThis->iMagic == RTSEMEVENT_MAGIC);

    RTLockValidatorRecSharedRemoveOwner(&pThis->Signallers, hThread);
#else
    RT_NOREF(hEventSem, hThread);
#endif
}

#endif /* glibc < 2.6 || IPRT_WITH_FUTEX_BASED_SEMS */

