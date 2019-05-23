/* $Id: semeventmulti-linux.cpp $ */
/** @file
 * IPRT - Multiple Release Event Semaphore, Linux (2.6.x+).
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
 * linux specific event semaphores code in order to work around the bug. As it
 * turns out, this code seems to have an unresolved issue (@bugref{2599}), so we'll
 * fall back on the pthread based implementation if glibc is known to contain
 * the bug fix.
 *
 * The external reference to epoll_pwait is a hack which prevents that we link
 * against glibc < 2.6.
 */
#include "../posix/semeventmulti-posix.cpp"
asm volatile (".global epoll_pwait");

#else /* glibc < 2.6 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/semaphore.h>
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/err.h>
#include <iprt/lockvalidator.h>
#include <iprt/mem.h>
#include <iprt/time.h>
#include "internal/magics.h"
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
 * Linux multiple wakup event semaphore.
 */
struct RTSEMEVENTMULTIINTERNAL
{
    /** Magic value. */
    uint32_t volatile   u32Magic;
    /** The futex state variable.
     * -1 means signaled.
     *  0 means not signaled, no waiters.
     *  1 means not signaled and that someone is waiting.
     */
    int32_t volatile    iState;
#ifdef RTSEMEVENTMULTI_STRICT
    /** Signallers. */
    RTLOCKVALRECSHRD    Signallers;
    /** Indicates that lock validation should be performed. */
    bool volatile       fEverHadSignallers;
#endif
};


/**
 * Wrapper for the futex syscall.
 */
static long sys_futex(int32_t volatile *uaddr, int op, int val, struct timespec *utime, int32_t *uaddr2, int val3)
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


RTDECL(int)  RTSemEventMultiCreate(PRTSEMEVENTMULTI phEventMultiSem)
{
    return RTSemEventMultiCreateEx(phEventMultiSem, 0 /*fFlags*/, NIL_RTLOCKVALCLASS, NULL);
}


RTDECL(int)  RTSemEventMultiCreateEx(PRTSEMEVENTMULTI phEventMultiSem, uint32_t fFlags, RTLOCKVALCLASS hClass,
                                     const char *pszNameFmt, ...)
{
    AssertReturn(!(fFlags & ~RTSEMEVENTMULTI_FLAGS_NO_LOCK_VAL), VERR_INVALID_PARAMETER);

    /*
     * Allocate semaphore handle.
     */
    struct RTSEMEVENTMULTIINTERNAL *pThis = (struct RTSEMEVENTMULTIINTERNAL *)RTMemAlloc(sizeof(struct RTSEMEVENTMULTIINTERNAL));
    if (pThis)
    {
        pThis->u32Magic = RTSEMEVENTMULTI_MAGIC;
        pThis->iState   = 0;
#ifdef RTSEMEVENTMULTI_STRICT
        if (!pszNameFmt)
        {
            static uint32_t volatile s_iSemEventMultiAnon = 0;
            RTLockValidatorRecSharedInit(&pThis->Signallers, hClass, RTLOCKVAL_SUB_CLASS_ANY, pThis,
                                         true /*fSignaller*/, !(fFlags & RTSEMEVENTMULTI_FLAGS_NO_LOCK_VAL),
                                         "RTSemEventMulti-%u", ASMAtomicIncU32(&s_iSemEventMultiAnon) - 1);
        }
        else
        {
            va_list va;
            va_start(va, pszNameFmt);
            RTLockValidatorRecSharedInitV(&pThis->Signallers, hClass, RTLOCKVAL_SUB_CLASS_ANY, pThis,
                                          true /*fSignaller*/, !(fFlags & RTSEMEVENTMULTI_FLAGS_NO_LOCK_VAL),
                                          pszNameFmt, va);
            va_end(va);
        }
        pThis->fEverHadSignallers = false;
#else
        RT_NOREF(hClass, pszNameFmt);
#endif

        *phEventMultiSem = pThis;
        return VINF_SUCCESS;
    }
    return  VERR_NO_MEMORY;
}


RTDECL(int)  RTSemEventMultiDestroy(RTSEMEVENTMULTI hEventMultiSem)
{
    /*
     * Validate input.
     */
    struct RTSEMEVENTMULTIINTERNAL *pThis = hEventMultiSem;
    if (pThis == NIL_RTSEMEVENTMULTI)
        return VINF_SUCCESS;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTSEMEVENTMULTI_MAGIC, VERR_INVALID_HANDLE);

    /*
     * Invalidate the semaphore and wake up anyone waiting on it.
     */
    ASMAtomicWriteU32(&pThis->u32Magic, RTSEMEVENTMULTI_MAGIC + 1);
    if (ASMAtomicXchgS32(&pThis->iState, -1) == 1)
    {
        sys_futex(&pThis->iState, FUTEX_WAKE, INT_MAX, NULL, NULL, 0);
        usleep(1000);
    }

    /*
     * Free the semaphore memory and be gone.
     */
#ifdef RTSEMEVENTMULTI_STRICT
    RTLockValidatorRecSharedDelete(&pThis->Signallers);
#endif
    RTMemFree(pThis);
    return VINF_SUCCESS;
}


RTDECL(int)  RTSemEventMultiSignal(RTSEMEVENTMULTI hEventMultiSem)
{
    /*
     * Validate input.
     */
    struct RTSEMEVENTMULTIINTERNAL *pThis = hEventMultiSem;
    AssertReturn(VALID_PTR(pThis) && pThis->u32Magic == RTSEMEVENTMULTI_MAGIC,
                 VERR_INVALID_HANDLE);

#ifdef RTSEMEVENTMULTI_STRICT
    if (pThis->fEverHadSignallers)
    {
        int rc9 = RTLockValidatorRecSharedCheckSignaller(&pThis->Signallers, NIL_RTTHREAD);
        if (RT_FAILURE(rc9))
            return rc9;
    }
#endif


    /*
     * Signal it.
     */
    int32_t iOld = ASMAtomicXchgS32(&pThis->iState, -1);
    if (iOld > 0)
    {
        /* wake up sleeping threads. */
        long cWoken = sys_futex(&pThis->iState, FUTEX_WAKE, INT_MAX, NULL, NULL, 0);
        AssertMsg(cWoken >= 0, ("%ld\n", cWoken)); NOREF(cWoken);
    }
    Assert(iOld == 0 || iOld == -1 || iOld == 1);
    return VINF_SUCCESS;
}


RTDECL(int)  RTSemEventMultiReset(RTSEMEVENTMULTI hEventMultiSem)
{
    /*
     * Validate input.
     */
    struct RTSEMEVENTMULTIINTERNAL *pThis = hEventMultiSem;
    AssertReturn(VALID_PTR(pThis) && pThis->u32Magic == RTSEMEVENTMULTI_MAGIC,
                 VERR_INVALID_HANDLE);
#ifdef RT_STRICT
    int32_t i = pThis->iState;
    Assert(i == 0 || i == -1 || i == 1);
#endif

    /*
     * Reset it.
     */
    ASMAtomicCmpXchgS32(&pThis->iState, 0, -1);
    return VINF_SUCCESS;
}



DECLINLINE(int) rtSemEventLnxMultiWait(struct RTSEMEVENTMULTIINTERNAL *pThis, uint32_t fFlags, uint64_t uTimeout,
                                       PCRTLOCKVALSRCPOS pSrcPos)
{
    RT_NOREF(pSrcPos);

    /*
     * Validate input.
     */
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTSEMEVENTMULTI_MAGIC, VERR_INVALID_HANDLE);
    AssertReturn(RTSEMWAIT_FLAGS_ARE_VALID(fFlags), VERR_INVALID_PARAMETER);

    /*
     * Quickly check whether it's signaled.
     */
    int32_t iCur = ASMAtomicUoReadS32(&pThis->iState);
    Assert(iCur == 0 || iCur == -1 || iCur == 1);
    if (iCur == -1)
        return VINF_SUCCESS;

    /*
     * Check and convert the timeout value.
     */
    struct timespec ts;
    struct timespec *pTimeout = NULL;
    uint64_t u64Deadline = 0; /* shut up gcc */
    if (!(fFlags & RTSEMWAIT_FLAGS_INDEFINITE))
    {
        /* If the timeout is zero, then we're done. */
        if (!uTimeout)
            return VERR_TIMEOUT;

        /* Convert it to a deadline + interval timespec. */
        if (fFlags & RTSEMWAIT_FLAGS_MILLISECS)
            uTimeout = uTimeout < UINT64_MAX / UINT32_C(1000000) * UINT32_C(1000000)
                     ? uTimeout * UINT32_C(1000000)
                     : UINT64_MAX;
        if (uTimeout != UINT64_MAX) /* unofficial way of indicating an indefinite wait */
        {
            if (fFlags & RTSEMWAIT_FLAGS_RELATIVE)
                u64Deadline = RTTimeSystemNanoTS() + uTimeout;
            else
            {
                uint64_t u64Now = RTTimeSystemNanoTS();
                if (uTimeout <= u64Now)
                    return VERR_TIMEOUT;
                u64Deadline = uTimeout;
                uTimeout   -= u64Now;
            }
            if (   sizeof(ts.tv_sec) >= sizeof(uint64_t)
                || uTimeout <= UINT64_C(1000000000) * UINT32_MAX)
            {
                ts.tv_nsec = uTimeout % UINT32_C(1000000000);
                ts.tv_sec  = uTimeout / UINT32_C(1000000000);
                pTimeout = &ts;
            }
        }
    }

    /*
     * The wait loop.
     */
#ifdef RTSEMEVENTMULTI_STRICT
    RTTHREAD hThreadSelf = RTThreadSelfAutoAdopt();
#else
    RTTHREAD hThreadSelf = RTThreadSelf();
#endif
    for (unsigned i = 0;; i++)
    {
        /*
         * Start waiting. We only account for there being or having been
         * threads waiting on the semaphore to keep things simple.
         */
        iCur = ASMAtomicUoReadS32(&pThis->iState);
        Assert(iCur == 0 || iCur == -1 || iCur == 1);
        if (    iCur == 1
            ||  ASMAtomicCmpXchgS32(&pThis->iState, 1, 0))
        {
            /* adjust the relative timeout */
            if (pTimeout)
            {
                int64_t i64Diff = u64Deadline - RTTimeSystemNanoTS();
                if (i64Diff < 1000)
                    return VERR_TIMEOUT;
                ts.tv_sec  = (uint64_t)i64Diff / UINT32_C(1000000000);
                ts.tv_nsec = (uint64_t)i64Diff % UINT32_C(1000000000);
            }
#ifdef RTSEMEVENTMULTI_STRICT
            if (pThis->fEverHadSignallers)
            {
                int rc9 = RTLockValidatorRecSharedCheckBlocking(&pThis->Signallers, hThreadSelf, pSrcPos, false,
                                                                uTimeout / UINT32_C(1000000), RTTHREADSTATE_EVENT_MULTI, true);
                if (RT_FAILURE(rc9))
                    return rc9;
            }
#endif
            RTThreadBlocking(hThreadSelf, RTTHREADSTATE_EVENT_MULTI, true);
            long rc = sys_futex(&pThis->iState, FUTEX_WAIT, 1, pTimeout, NULL, 0);
            RTThreadUnblocked(hThreadSelf, RTTHREADSTATE_EVENT_MULTI);
            if (RT_UNLIKELY(pThis->u32Magic != RTSEMEVENTMULTI_MAGIC))
                return VERR_SEM_DESTROYED;
            if (rc == 0)
                return VINF_SUCCESS;

            /*
             * Act on the wakup code.
             */
            if (rc == -ETIMEDOUT)
            {
/** @todo something is broken here. shows up every now and again in the ata
 *        code. Should try to run the timeout against RTTimeMilliTS to
 *        check that it's doing the right thing... */
                Assert(pTimeout);
                return VERR_TIMEOUT;
            }
            if (rc == -EWOULDBLOCK)
                /* retry, the value changed. */;
            else if (rc == -EINTR)
            {
                if (fFlags & RTSEMWAIT_FLAGS_NORESUME)
                    return VERR_INTERRUPTED;
            }
            else
            {
                /* this shouldn't happen! */
                AssertMsgFailed(("rc=%ld errno=%d\n", rc, errno));
                return RTErrConvertFromErrno(rc);
            }
        }
        else if (iCur == -1)
            return VINF_SUCCESS;
    }
}


#undef RTSemEventMultiWaitEx
RTDECL(int)  RTSemEventMultiWaitEx(RTSEMEVENTMULTI hEventMultiSem, uint32_t fFlags, uint64_t uTimeout)
{
#ifndef RTSEMEVENT_STRICT
    return rtSemEventLnxMultiWait(hEventMultiSem, fFlags, uTimeout, NULL);
#else
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_NORMAL_API();
    return rtSemEventLnxMultiWait(hEventMultiSem, fFlags, uTimeout, &SrcPos);
#endif
}


RTDECL(int)  RTSemEventMultiWaitExDebug(RTSEMEVENTMULTI hEventMultiSem, uint32_t fFlags, uint64_t uTimeout,
                                        RTHCUINTPTR uId, RT_SRC_POS_DECL)
{
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_DEBUG_API();
    return rtSemEventLnxMultiWait(hEventMultiSem, fFlags, uTimeout, &SrcPos);
}


RTDECL(void) RTSemEventMultiSetSignaller(RTSEMEVENTMULTI hEventMultiSem, RTTHREAD hThread)
{
#ifdef RTSEMEVENTMULTI_STRICT
    struct RTSEMEVENTMULTIINTERNAL *pThis = hEventMultiSem;
    AssertPtrReturnVoid(pThis);
    AssertReturnVoid(pThis->u32Magic == RTSEMEVENTMULTI_MAGIC);

    ASMAtomicWriteBool(&pThis->fEverHadSignallers, true);
    RTLockValidatorRecSharedResetOwner(&pThis->Signallers, hThread, NULL);
#else
    RT_NOREF(hEventMultiSem, hThread);
#endif
}


RTDECL(void) RTSemEventMultiAddSignaller(RTSEMEVENTMULTI hEventMultiSem, RTTHREAD hThread)
{
#ifdef RTSEMEVENTMULTI_STRICT
    struct RTSEMEVENTMULTIINTERNAL *pThis = hEventMultiSem;
    AssertPtrReturnVoid(pThis);
    AssertReturnVoid(pThis->u32Magic == RTSEMEVENTMULTI_MAGIC);

    ASMAtomicWriteBool(&pThis->fEverHadSignallers, true);
    RTLockValidatorRecSharedAddOwner(&pThis->Signallers, hThread, NULL);
#else
    RT_NOREF(hEventMultiSem, hThread);
#endif
}


RTDECL(void) RTSemEventMultiRemoveSignaller(RTSEMEVENTMULTI hEventMultiSem, RTTHREAD hThread)
{
#ifdef RTSEMEVENTMULTI_STRICT
    struct RTSEMEVENTMULTIINTERNAL *pThis = hEventMultiSem;
    AssertPtrReturnVoid(pThis);
    AssertReturnVoid(pThis->u32Magic == RTSEMEVENTMULTI_MAGIC);

    RTLockValidatorRecSharedRemoveOwner(&pThis->Signallers, hThread);
#else
    RT_NOREF(hEventMultiSem, hThread);
#endif
}

#endif /* glibc < 2.6 || IPRT_WITH_FUTEX_BASED_SEMS */

