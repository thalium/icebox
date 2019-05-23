/* $Id: semevent-posix.cpp $ */
/** @file
 * IPRT - Event Semaphore, POSIX.
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

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/mem.h>
#include <iprt/lockvalidator.h>

#include "internal/mem.h"
#include "internal/strict.h"

#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#ifdef RT_OS_DARWIN
# define pthread_yield() pthread_yield_np()
#endif

#if defined(RT_OS_SOLARIS) || defined(RT_OS_HAIKU) || defined(RT_OS_FREEBSD) || defined(RT_OS_NETBSD)
# include <sched.h>
# define pthread_yield() sched_yield()
#endif


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/** Internal representation of the POSIX implementation of an Event semaphore.
 * The POSIX implementation uses a mutex and a condition variable to implement
 * the automatic reset event semaphore semantics.
 */
struct RTSEMEVENTINTERNAL
{
    /** pthread condition. */
    pthread_cond_t      Cond;
    /** pthread mutex which protects the condition and the event state. */
    pthread_mutex_t     Mutex;
    /** The state of the semaphore.
     * This is operated while owning mutex and using atomic updating. */
    volatile uint32_t   u32State;
    /** Number of waiters. */
    volatile uint32_t   cWaiters;
#ifdef RTSEMEVENT_STRICT
    /** Signallers. */
    RTLOCKVALRECSHRD    Signallers;
    /** Indicates that lock validation should be performed. */
    bool volatile       fEverHadSignallers;
#endif
    /** The creation flags. */
    uint32_t            fFlags;
};

/** The values of the u32State variable in a RTSEMEVENTINTERNAL.
 * @{ */
/** The object isn't initialized. */
#define EVENT_STATE_UNINITIALIZED   0
/** The semaphore is signaled. */
#define EVENT_STATE_SIGNALED        0xff00ff00
/** The semaphore is not signaled. */
#define EVENT_STATE_NOT_SIGNALED    0x00ff00ff
/** @} */


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
    int rc;
    struct RTSEMEVENTINTERNAL *pThis;
    if (!(fFlags & RTSEMEVENT_FLAGS_BOOTSTRAP_HACK))
        pThis = (struct RTSEMEVENTINTERNAL *)RTMemAlloc(sizeof(*pThis));
    else
        pThis = (struct RTSEMEVENTINTERNAL *)rtMemBaseAlloc(sizeof(*pThis));
    if (pThis)
    {
        /*
         * Create the condition variable.
         */
        rc = pthread_cond_init(&pThis->Cond, NULL);
        if (!rc)
        {
            /*
             * Create the semaphore.
             */
            rc = pthread_mutex_init(&pThis->Mutex, NULL);
            if (!rc)
            {
                ASMAtomicWriteU32(&pThis->u32State, EVENT_STATE_NOT_SIGNALED);
                ASMAtomicWriteU32(&pThis->cWaiters, 0);
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
                RT_NOREF_PV(hClass); RT_NOREF_PV(pszNameFmt);
#endif

                *phEventSem = pThis;
                return VINF_SUCCESS;
            }
            pthread_cond_destroy(&pThis->Cond);
        }

        rc = RTErrConvertFromErrno(rc);
        if (!(fFlags & RTSEMEVENT_FLAGS_BOOTSTRAP_HACK))
            RTMemFree(pThis);
        else
            rtMemBaseFree(pThis);
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}


RTDECL(int)  RTSemEventDestroy(RTSEMEVENT hEventSem)
{
    /*
     * Validate handle.
     */
    struct RTSEMEVENTINTERNAL *pThis = hEventSem;
    if (pThis == NIL_RTSEMEVENT)
        return VINF_SUCCESS;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    uint32_t    u32 = pThis->u32State;
    AssertReturn(u32 == EVENT_STATE_NOT_SIGNALED || u32 == EVENT_STATE_SIGNALED, VERR_INVALID_HANDLE);

    /*
     * Abort all waiters forcing them to return failure.
     */
    int rc;
    for (int i = 30; i > 0; i--)
    {
        ASMAtomicWriteU32(&pThis->u32State, EVENT_STATE_UNINITIALIZED);
        rc = pthread_cond_destroy(&pThis->Cond);
        if (rc != EBUSY)
            break;
        pthread_cond_broadcast(&pThis->Cond);
        usleep(1000);
    }
    if (rc)
    {
        AssertMsgFailed(("Failed to destroy event sem %p, rc=%d.\n", pThis, rc));
        return RTErrConvertFromErrno(rc);
    }

    /*
     * Destroy the semaphore
     * If it's busy we'll wait a bit to give the threads a chance to be scheduled.
     */
    for (int i = 30; i > 0; i--)
    {
        rc = pthread_mutex_destroy(&pThis->Mutex);
        if (rc != EBUSY)
            break;
        usleep(1000);
    }
    if (rc)
    {
        AssertMsgFailed(("Failed to destroy event sem %p, rc=%d. (mutex)\n", pThis, rc));
        return RTErrConvertFromErrno(rc);
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
    uint32_t    u32 = pThis->u32State;
    AssertReturn(u32 == EVENT_STATE_NOT_SIGNALED || u32 == EVENT_STATE_SIGNALED, VERR_INVALID_HANDLE);

#ifdef RTSEMEVENT_STRICT
    if (pThis->fEverHadSignallers)
    {
        int rc9 = RTLockValidatorRecSharedCheckSignaller(&pThis->Signallers, NIL_RTTHREAD);
        if (RT_FAILURE(rc9))
            return rc9;
    }
#endif

    /*
     * Lock the mutex semaphore.
     */
    int rc = pthread_mutex_lock(&pThis->Mutex);
    if (rc)
    {
        AssertMsgFailed(("Failed to lock event sem %p, rc=%d.\n", hEventSem, rc));
        return RTErrConvertFromErrno(rc);
    }

    /*
     * Check the state.
     */
    if (pThis->u32State == EVENT_STATE_NOT_SIGNALED)
    {
        ASMAtomicWriteU32(&pThis->u32State, EVENT_STATE_SIGNALED);
        rc = pthread_cond_signal(&pThis->Cond);
        AssertMsg(!rc, ("Failed to signal event sem %p, rc=%d.\n", hEventSem, rc));
    }
    else if (pThis->u32State == EVENT_STATE_SIGNALED)
    {
        rc = pthread_cond_signal(&pThis->Cond); /* give'm another kick... */
        AssertMsg(!rc, ("Failed to signal event sem %p, rc=%d. (2)\n", hEventSem, rc));
    }
    else
        rc = VERR_SEM_DESTROYED;

    /*
     * Release the mutex and return.
     */
    int rc2 = pthread_mutex_unlock(&pThis->Mutex);
    AssertMsg(!rc2, ("Failed to unlock event sem %p, rc=%d.\n", hEventSem, rc));
    if (rc)
        return RTErrConvertFromErrno(rc);
    if (rc2)
        return RTErrConvertFromErrno(rc2);

    return VINF_SUCCESS;
}


DECL_FORCE_INLINE(int) rtSemEventWait(RTSEMEVENT hEventSem, RTMSINTERVAL cMillies, bool fAutoResume)
{
#ifdef RTSEMEVENT_STRICT
    PCRTLOCKVALSRCPOS  pSrcPos = NULL;
#endif

    /*
     * Validate input.
     */
    struct RTSEMEVENTINTERNAL *pThis = hEventSem;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    uint32_t    u32 = pThis->u32State;
    AssertReturn(u32 == EVENT_STATE_NOT_SIGNALED || u32 == EVENT_STATE_SIGNALED, VERR_INVALID_HANDLE);

    /*
     * Timed or indefinite wait?
     */
    if (cMillies == RT_INDEFINITE_WAIT)
    {
        /* for fairness, yield before going to sleep. */
        if (    ASMAtomicIncU32(&pThis->cWaiters) > 1
            &&  pThis->u32State == EVENT_STATE_SIGNALED)
            pthread_yield();

         /* take mutex */
        int rc = pthread_mutex_lock(&pThis->Mutex);
        if (rc)
        {
            ASMAtomicDecU32(&pThis->cWaiters);
            AssertMsgFailed(("Failed to lock event sem %p, rc=%d.\n", hEventSem, rc));
            return RTErrConvertFromErrno(rc);
        }

        for (;;)
        {
            /* check state. */
            if (pThis->u32State == EVENT_STATE_SIGNALED)
            {
                ASMAtomicWriteU32(&pThis->u32State, EVENT_STATE_NOT_SIGNALED);
                ASMAtomicDecU32(&pThis->cWaiters);
                rc = pthread_mutex_unlock(&pThis->Mutex);
                AssertMsg(!rc, ("Failed to unlock event sem %p, rc=%d.\n", hEventSem, rc)); NOREF(rc);
                return VINF_SUCCESS;
            }
            if (pThis->u32State == EVENT_STATE_UNINITIALIZED)
            {
                rc = pthread_mutex_unlock(&pThis->Mutex);
                AssertMsg(!rc, ("Failed to unlock event sem %p, rc=%d.\n", hEventSem, rc)); NOREF(rc);
                return VERR_SEM_DESTROYED;
            }

            /* wait */
#ifdef RTSEMEVENT_STRICT
            RTTHREAD hThreadSelf = !(pThis->fFlags & RTSEMEVENT_FLAGS_BOOTSTRAP_HACK)
                                 ? RTThreadSelfAutoAdopt()
                                 : RTThreadSelf();
            if (pThis->fEverHadSignallers)
            {
                rc = RTLockValidatorRecSharedCheckBlocking(&pThis->Signallers, hThreadSelf, pSrcPos, false,
                                                           cMillies, RTTHREADSTATE_EVENT, true);
                if (RT_FAILURE(rc))
                {
                    ASMAtomicDecU32(&pThis->cWaiters);
                    pthread_mutex_unlock(&pThis->Mutex);
                    return rc;
                }
            }
#else
            RTTHREAD hThreadSelf = RTThreadSelf();
#endif
            RTThreadBlocking(hThreadSelf, RTTHREADSTATE_EVENT, true);
            rc = pthread_cond_wait(&pThis->Cond, &pThis->Mutex);
            RTThreadUnblocked(hThreadSelf, RTTHREADSTATE_EVENT);
            if (rc)
            {
                AssertMsgFailed(("Failed to wait on event sem %p, rc=%d.\n", hEventSem, rc));
                ASMAtomicDecU32(&pThis->cWaiters);
                int rc2 = pthread_mutex_unlock(&pThis->Mutex);
                AssertMsg(!rc2, ("Failed to unlock event sem %p, rc=%d.\n", hEventSem, rc2)); NOREF(rc2);
                return RTErrConvertFromErrno(rc);
            }
        }
    }
    else
    {
        /*
         * Get current time and calc end of wait time.
         */
        struct timespec     ts = {0,0};
#if defined(RT_OS_DARWIN) || defined(RT_OS_HAIKU)
        struct timeval      tv = {0,0};
        gettimeofday(&tv, NULL);
        ts.tv_sec = tv.tv_sec;
        ts.tv_nsec = tv.tv_usec * 1000;
#else
        clock_gettime(CLOCK_REALTIME, &ts);
#endif
        if (cMillies != 0)
        {
            ts.tv_nsec += (cMillies % 1000) * 1000000;
            ts.tv_sec  += cMillies / 1000;
            if (ts.tv_nsec >= 1000000000)
            {
                ts.tv_nsec -= 1000000000;
                ts.tv_sec++;
            }
        }

        /* for fairness, yield before going to sleep. */
        if (ASMAtomicIncU32(&pThis->cWaiters) > 1 && cMillies)
            pthread_yield();

        /* take mutex */
        int rc = pthread_mutex_lock(&pThis->Mutex);
        if (rc)
        {
            ASMAtomicDecU32(&pThis->cWaiters);
            AssertMsg(rc == ETIMEDOUT, ("Failed to lock event sem %p, rc=%d.\n", hEventSem, rc));
            return RTErrConvertFromErrno(rc);
        }

        for (;;)
        {
            /* check state. */
            if (pThis->u32State == EVENT_STATE_SIGNALED)
            {
                ASMAtomicWriteU32(&pThis->u32State, EVENT_STATE_NOT_SIGNALED);
                ASMAtomicDecU32(&pThis->cWaiters);
                rc = pthread_mutex_unlock(&pThis->Mutex);
                AssertMsg(!rc, ("Failed to unlock event sem %p, rc=%d.\n", hEventSem, rc)); NOREF(rc);
                return VINF_SUCCESS;
            }
            if (pThis->u32State == EVENT_STATE_UNINITIALIZED)
            {
                rc = pthread_mutex_unlock(&pThis->Mutex);
                AssertMsg(!rc, ("Failed to unlock event sem %p, rc=%d.\n", hEventSem, rc)); NOREF(rc);
                return VERR_SEM_DESTROYED;
            }

            /* we're done if the timeout is 0. */
            if (!cMillies)
            {
                ASMAtomicDecU32(&pThis->cWaiters);
                rc = pthread_mutex_unlock(&pThis->Mutex);
                return VERR_TIMEOUT;
            }

            /* wait */
#ifdef RTSEMEVENT_STRICT
            RTTHREAD hThreadSelf = !(pThis->fFlags & RTSEMEVENT_FLAGS_BOOTSTRAP_HACK)
                                 ? RTThreadSelfAutoAdopt()
                                 : RTThreadSelf();
            if (pThis->fEverHadSignallers)
            {
                rc = RTLockValidatorRecSharedCheckBlocking(&pThis->Signallers, hThreadSelf, pSrcPos, false,
                                                           cMillies, RTTHREADSTATE_EVENT, true);
                if (RT_FAILURE(rc))
                {
                    ASMAtomicDecU32(&pThis->cWaiters);
                    pthread_mutex_unlock(&pThis->Mutex);
                    return rc;
                }
            }
#else
            RTTHREAD hThreadSelf = RTThreadSelf();
#endif
            RTThreadBlocking(hThreadSelf, RTTHREADSTATE_EVENT, true);
            rc = pthread_cond_timedwait(&pThis->Cond, &pThis->Mutex, &ts);
            RTThreadUnblocked(hThreadSelf, RTTHREADSTATE_EVENT);
            if (rc && (rc != EINTR || !fAutoResume)) /* according to SuS this function shall not return EINTR, but linux man page says differently. */
            {
                AssertMsg(rc == ETIMEDOUT, ("Failed to wait on event sem %p, rc=%d.\n", hEventSem, rc));
                ASMAtomicDecU32(&pThis->cWaiters);
                int rc2 = pthread_mutex_unlock(&pThis->Mutex);
                AssertMsg(!rc2, ("Failed to unlock event sem %p, rc2=%d.\n", hEventSem, rc2)); NOREF(rc2);
                return RTErrConvertFromErrno(rc);
            }
        } /* for (;;) */
    }
}


RTDECL(int)  RTSemEventWait(RTSEMEVENT hEventSem, RTMSINTERVAL cMillies)
{
    int rc = rtSemEventWait(hEventSem, cMillies, true);
    Assert(rc != VERR_INTERRUPTED);
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
    uint32_t u32 = pThis->u32State;
    AssertReturnVoid(u32 == EVENT_STATE_NOT_SIGNALED || u32 == EVENT_STATE_SIGNALED);

    ASMAtomicWriteBool(&pThis->fEverHadSignallers, true);
    RTLockValidatorRecSharedResetOwner(&pThis->Signallers, hThread, NULL);
#else
    RT_NOREF_PV(hEventSem); RT_NOREF_PV(hThread);
#endif
}


RTDECL(void) RTSemEventAddSignaller(RTSEMEVENT hEventSem, RTTHREAD hThread)
{
#ifdef RTSEMEVENT_STRICT
    struct RTSEMEVENTINTERNAL *pThis = hEventSem;
    AssertPtrReturnVoid(pThis);
    uint32_t u32 = pThis->u32State;
    AssertReturnVoid(u32 == EVENT_STATE_NOT_SIGNALED || u32 == EVENT_STATE_SIGNALED);

    ASMAtomicWriteBool(&pThis->fEverHadSignallers, true);
    RTLockValidatorRecSharedAddOwner(&pThis->Signallers, hThread, NULL);
#else
    RT_NOREF_PV(hEventSem); RT_NOREF_PV(hThread);
#endif
}


RTDECL(void) RTSemEventRemoveSignaller(RTSEMEVENT hEventSem, RTTHREAD hThread)
{
#ifdef RTSEMEVENT_STRICT
    struct RTSEMEVENTINTERNAL *pThis = hEventSem;
    AssertPtrReturnVoid(pThis);
    uint32_t u32 = pThis->u32State;
    AssertReturnVoid(u32 == EVENT_STATE_NOT_SIGNALED || u32 == EVENT_STATE_SIGNALED);

    RTLockValidatorRecSharedRemoveOwner(&pThis->Signallers, hThread);
#else
    RT_NOREF_PV(hEventSem); RT_NOREF_PV(hThread);
#endif
}

