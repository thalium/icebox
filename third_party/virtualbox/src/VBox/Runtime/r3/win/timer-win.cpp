/* $Id: timer-win.cpp $ */
/** @file
 * IPRT - Timer.
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


/* Which code to use is determined here...
 *
 * The default is to use wait on NT timers directly with no APC since this
 * is supposed to give the shortest kernel code paths.
 *
 * The USE_APC variation will do as above except that an APC routine is
 * handling the callback action.
 *
 * The USE_WINMM version will use the NT timer wrappers in WinMM which may
 * result in some 0.1% better correctness in number of delivered ticks. However,
 * this codepath have more overhead (it uses APC among other things), and I'm not
 * quite sure if it's actually any more correct.
 *
 * The USE_CATCH_UP will play catch up when the timer lags behind. However this
 * requires a monotonous time source.
 *
 * The default mode which we are using is using relative periods of time and thus
 * will never suffer from errors in the time source. Neither will it try catch up
 * missed ticks. This suits our current purposes best I'd say.
 */
#undef USE_APC
#undef USE_WINMM
#undef USE_CATCH_UP


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP RTLOGGROUP_TIMER
#define _WIN32_WINNT 0x0500
#include <iprt/win/windows.h>

#include <iprt/timer.h>
#ifdef USE_CATCH_UP
# include <iprt/time.h>
#endif
#include <iprt/alloc.h>
#include <iprt/assert.h>
#include <iprt/thread.h>
#include <iprt/log.h>
#include <iprt/asm.h>
#include <iprt/semaphore.h>
#include <iprt/err.h>
#include "internal/magics.h"

RT_C_DECLS_BEGIN
/* from sysinternals. */
NTSYSAPI LONG NTAPI NtSetTimerResolution(IN ULONG DesiredResolution, IN BOOLEAN SetResolution, OUT PULONG CurrentResolution);
NTSYSAPI LONG NTAPI NtQueryTimerResolution(OUT PULONG MaximumResolution, OUT PULONG MinimumResolution, OUT PULONG CurrentResolution);
RT_C_DECLS_END


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * The internal representation of a timer handle.
 */
typedef struct RTTIMER
{
    /** Magic.
     * This is RTTIMER_MAGIC, but changes to something else before the timer
     * is destroyed to indicate clearly that thread should exit. */
    volatile uint32_t       u32Magic;
    /** User argument. */
    void                   *pvUser;
    /** Callback. */
    PFNRTTIMER              pfnTimer;
    /** The current tick. */
    uint64_t                iTick;
    /** The interval. */
    unsigned                uMilliesInterval;
#ifdef USE_WINMM
    /** Win32 timer id. */
    UINT                    TimerId;
#else
    /** Time handle. */
    HANDLE                  hTimer;
# ifdef USE_APC
    /** Handle to wait on. */
    HANDLE                  hevWait;
# endif
    /** USE_CATCH_UP: ns time of the next tick.
     * !USE_CATCH_UP: -uMilliesInterval * 10000 */
    LARGE_INTEGER           llNext;
    /** The thread handle of the timer thread. */
    RTTHREAD                Thread;
    /** The error/status of the timer.
     * Initially -1, set to 0 when the timer have been successfully started, and
     * to errno on failure in starting the timer. */
    volatile int            iError;
#endif
} RTTIMER;



#ifdef USE_WINMM
/**
 * Win32 callback wrapper.
 */
static void CALLBACK rttimerCallback(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
    PRTTIMER pTimer = (PRTTIMER)(void *)dwUser;
    Assert(pTimer->TimerId == uTimerID);
    pTimer->pfnTimer(pTimer, pTimer->pvUser, ++pTimer->iTick);
    NOREF(uMsg); NOREF(dw1); NOREF(dw2); NOREF(uTimerID);
}
#else /* !USE_WINMM */

#ifdef USE_APC
/**
 * Async callback.
 *
 * @param   lpArgToCompletionRoutine    Pointer to our timer structure.
 */
VOID CALLBACK rttimerAPCProc(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue)
{
    PRTTIMER pTimer = (PRTTIMER)lpArgToCompletionRoutine;

    /*
     * Check if we're begin destroyed.
     */
    if (pTimer->u32Magic != RTTIMER_MAGIC)
        return;

    /*
     * Callback the handler.
     */
    pTimer->pfnTimer(pTimer, pTimer->pvUser, ++pTimer->iTick);

    /*
     * Rearm the timer handler.
     */
#ifdef USE_CATCH_UP
    pTimer->llNext.QuadPart += (int64_t)pTimer->uMilliesInterval * 10000;
    LARGE_INTEGER ll;
    ll.QuadPart = RTTimeNanoTS() - pTimer->llNext.QuadPart;
    if (ll.QuadPart < -500000)
        ll.QuadPart = ll.QuadPart / 100;
    else
        ll.QuadPart = -500000 / 100; /* need to catch up, do a minimum wait of 0.5ms. */
#else
    LARGE_INTEGER ll = pTimer->llNext;
#endif
    BOOL frc = SetWaitableTimer(pTimer->hTimer, &ll, 0, rttimerAPCProc, pTimer, FALSE);
    AssertMsg(frc || pTimer->u32Magic != RTTIMER_MAGIC, ("last error %d\n", GetLastError()));
}
#endif /* USE_APC */

/**
 * Timer thread.
 */
static DECLCALLBACK(int) rttimerCallback(RTTHREAD Thread, void *pvArg)
{
    PRTTIMER pTimer = (PRTTIMER)(void *)pvArg;
    Assert(pTimer->u32Magic == RTTIMER_MAGIC);

    /*
     * Bounce our priority up quite a bit.
     */
    if (    !SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL)
        /*&&  !SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST)*/)
    {
        int rc = GetLastError();
        AssertMsgFailed(("Failed to set priority class lasterror %d.\n", rc));
        pTimer->iError = RTErrConvertFromWin32(rc);
        return rc;
    }

    /*
     * Start the waitable timer.
     */

#ifdef USE_CATCH_UP
    const int64_t NSInterval = (int64_t)pTimer->uMilliesInterval * 1000000;
    pTimer->llNext.QuadPart = RTTimeNanoTS() + NSInterval;
#else
    pTimer->llNext.QuadPart = -(int64_t)pTimer->uMilliesInterval * 10000;
#endif
    LARGE_INTEGER ll;
    ll.QuadPart = -(int64_t)pTimer->uMilliesInterval * 10000;
#ifdef USE_APC
    if (!SetWaitableTimer(pTimer->hTimer, &ll, 0, rttimerAPCProc, pTimer, FALSE))
#else
    if (!SetWaitableTimer(pTimer->hTimer, &ll, 0, NULL, NULL, FALSE))
#endif
    {
        int rc = GetLastError();
        AssertMsgFailed(("Failed to set timer, lasterr %d.\n", rc));
        pTimer->iError = RTErrConvertFromWin32(rc);
        RTThreadUserSignal(Thread);
        return rc;
    }

    /*
     * Wait for the semaphore to be posted.
     */
    RTThreadUserSignal(Thread);
    for (;pTimer->u32Magic == RTTIMER_MAGIC;)
    {
#ifdef USE_APC
        int rc = WaitForSingleObjectEx(pTimer->hevWait, INFINITE, TRUE);
        if (rc != WAIT_OBJECT_0 && rc != WAIT_IO_COMPLETION)
#else
        int rc = WaitForSingleObjectEx(pTimer->hTimer, INFINITE, FALSE);
        if (pTimer->u32Magic != RTTIMER_MAGIC)
            break;
        if (rc == WAIT_OBJECT_0)
        {
            /*
             * Callback the handler.
             */
            pTimer->pfnTimer(pTimer, pTimer->pvUser, ++pTimer->iTick);

            /*
             * Rearm the timer handler.
             */
# ifdef USE_CATCH_UP
            pTimer->llNext.QuadPart += NSInterval;
            LARGE_INTEGER ll;
            ll.QuadPart = RTTimeNanoTS() - pTimer->llNext.QuadPart;
            if (ll.QuadPart < -500000)
                ll.QuadPart = ll.QuadPart / 100;
            else
                ll.QuadPart = -500000 / 100; /* need to catch up, do a minimum wait of 0.5ms. */
# else
            LARGE_INTEGER ll = pTimer->llNext;
# endif
            BOOL fRc = SetWaitableTimer(pTimer->hTimer, &ll, 0, NULL, NULL, FALSE);
            AssertMsg(fRc || pTimer->u32Magic != RTTIMER_MAGIC, ("last error %d\n", GetLastError())); NOREF(fRc);
        }
        else
#endif
        {
            /*
             * We failed during wait, so just signal the destructor and exit.
             */
            int rc2 = GetLastError();
            RTThreadUserSignal(Thread);
            AssertMsgFailed(("Wait on hTimer failed, rc=%d lasterr=%d\n", rc, rc2)); NOREF(rc2);
            return -1;
        }
    }

    /*
     * Exit.
     */
    RTThreadUserSignal(Thread);
    return 0;
}
#endif /* !USE_WINMM */


RTDECL(int) RTTimerCreate(PRTTIMER *ppTimer, unsigned uMilliesInterval, PFNRTTIMER pfnTimer, void *pvUser)
{
#ifndef USE_WINMM
    /*
     * On windows we'll have to set the timer resolution before
     * we start the timer.
     */
    ULONG ulMax = UINT32_MAX;
    ULONG ulMin = UINT32_MAX;
    ULONG ulCur = UINT32_MAX;
    NtQueryTimerResolution(&ulMax, &ulMin, &ulCur);
    Log(("NtQueryTimerResolution -> ulMax=%lu00ns ulMin=%lu00ns ulCur=%lu00ns\n", ulMax, ulMin, ulCur));
    if (ulCur > ulMin && ulCur > 10000 /* = 1ms */)
    {
        if (NtSetTimerResolution(10000, TRUE, &ulCur) >= 0)
            Log(("Changed timer resolution to 1ms.\n"));
        else if (NtSetTimerResolution(20000, TRUE, &ulCur) >= 0)
            Log(("Changed timer resolution to 2ms.\n"));
        else if (NtSetTimerResolution(40000, TRUE, &ulCur) >= 0)
            Log(("Changed timer resolution to 4ms.\n"));
        else if (ulMin <= 50000 && NtSetTimerResolution(ulMin, TRUE, &ulCur) >= 0)
            Log(("Changed timer resolution to %lu *100ns.\n", ulMin));
        else
        {
            AssertMsgFailed(("Failed to configure timer resolution!\n"));
            return VERR_INTERNAL_ERROR;
        }
    }
#endif /* !USE_WINN */

    /*
     * Create new timer.
     */
    int rc = VERR_IPE_UNINITIALIZED_STATUS;
    PRTTIMER pTimer = (PRTTIMER)RTMemAlloc(sizeof(*pTimer));
    if (pTimer)
    {
        pTimer->u32Magic    = RTTIMER_MAGIC;
        pTimer->pvUser      = pvUser;
        pTimer->pfnTimer    = pfnTimer;
        pTimer->iTick       = 0;
        pTimer->uMilliesInterval = uMilliesInterval;
#ifdef USE_WINMM
        /* sync kill doesn't work. */
        pTimer->TimerId     = timeSetEvent(uMilliesInterval, 0, rttimerCallback, (DWORD_PTR)pTimer, TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
        if (pTimer->TimerId)
        {
            ULONG ulMax = UINT32_MAX;
            ULONG ulMin = UINT32_MAX;
            ULONG ulCur = UINT32_MAX;
            NtQueryTimerResolution(&ulMax, &ulMin, &ulCur);
            Log(("NtQueryTimerResolution -> ulMax=%lu00ns ulMin=%lu00ns ulCur=%lu00ns\n", ulMax, ulMin, ulCur));

            *ppTimer = pTimer;
            return VINF_SUCCESS;
        }
        rc = VERR_INVALID_PARAMETER;

#else /* !USE_WINMM */

        /*
         * Create Win32 event semaphore.
         */
        pTimer->iError = 0;
        pTimer->hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
        if (pTimer->hTimer)
        {
#ifdef USE_APC
            /*
             * Create wait semaphore.
             */
            pTimer->hevWait = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (pTimer->hevWait)
#endif
            {
                /*
                 * Kick off the timer thread.
                 */
                rc = RTThreadCreate(&pTimer->Thread, rttimerCallback, pTimer, 0, RTTHREADTYPE_TIMER, RTTHREADFLAGS_WAITABLE, "Timer");
                if (RT_SUCCESS(rc))
                {
                    /*
                     * Wait for the timer to successfully create the timer
                     * If we don't get a response in 10 secs, then we assume we're screwed.
                     */
                    rc = RTThreadUserWait(pTimer->Thread, 10000);
                    if (RT_SUCCESS(rc))
                    {
                        rc = pTimer->iError;
                        if (RT_SUCCESS(rc))
                        {
                            *ppTimer = pTimer;
                            return VINF_SUCCESS;
                        }
                    }
                    ASMAtomicXchgU32(&pTimer->u32Magic, RTTIMER_MAGIC + 1);
                    RTThreadWait(pTimer->Thread, 250, NULL);
                    CancelWaitableTimer(pTimer->hTimer);
                }
#ifdef USE_APC
                CloseHandle(pTimer->hevWait);
#endif
            }
            CloseHandle(pTimer->hTimer);
        }
#endif /* !USE_WINMM */

        AssertMsgFailed(("Failed to create timer uMilliesInterval=%d. rc=%d\n", uMilliesInterval, rc));
        RTMemFree(pTimer);
    }
    else
        rc = VERR_NO_MEMORY;
    return rc;
}


RTR3DECL(int)     RTTimerDestroy(PRTTIMER pTimer)
{
    /* NULL is ok. */
    if (!pTimer)
        return VINF_SUCCESS;

    /*
     * Validate handle first.
     */
    int rc;
    if (    VALID_PTR(pTimer)
        &&  pTimer->u32Magic == RTTIMER_MAGIC)
    {
#ifdef USE_WINMM
        /*
         * Kill the timer and exit.
         */
        rc = timeKillEvent(pTimer->TimerId);
        AssertMsg(rc == TIMERR_NOERROR, ("timeKillEvent -> %d\n", rc));
        ASMAtomicXchgU32(&pTimer->u32Magic, RTTIMER_MAGIC + 1);
        RTThreadSleep(1);

#else /* !USE_WINMM */

        /*
         * Signal that we want the thread to exit.
         */
        ASMAtomicXchgU32(&pTimer->u32Magic, RTTIMER_MAGIC + 1);
#ifdef USE_APC
        SetEvent(pTimer->hevWait);
        CloseHandle(pTimer->hevWait);
        rc = CancelWaitableTimer(pTimer->hTimer);
        AssertMsg(rc, ("CancelWaitableTimer lasterr=%d\n", GetLastError()));
#else
        LARGE_INTEGER ll = {0};
        ll.LowPart = 100;
        rc = SetWaitableTimer(pTimer->hTimer, &ll, 0, NULL, NULL, FALSE);
        AssertMsg(rc, ("CancelWaitableTimer lasterr=%d\n", GetLastError()));
#endif

        /*
         * Wait for the thread to exit.
         * And if it don't wanna exit, we'll get kill it.
         */
        rc = RTThreadWait(pTimer->Thread, 1000, NULL);
        if (RT_FAILURE(rc))
            TerminateThread((HANDLE)RTThreadGetNative(pTimer->Thread), UINT32_MAX);

        /*
         * Free resource.
         */
        rc = CloseHandle(pTimer->hTimer);
        AssertMsg(rc, ("CloseHandle lasterr=%d\n", GetLastError()));

#endif /* !USE_WINMM */
        RTMemFree(pTimer);
        return rc;
    }

    rc = VERR_INVALID_HANDLE;
    AssertMsgFailed(("Failed to destroy timer %p. rc=%d\n", pTimer, rc));
    return rc;
}

