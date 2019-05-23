/* $Id: timer-r0drv-nt.cpp $ */
/** @file
 * IPRT - Timers, Ring-0 Driver, NT.
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
#include "the-nt-kernel.h"

#include <iprt/timer.h>
#include <iprt/mp.h>
#include <iprt/cpuset.h>
#include <iprt/err.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/mem.h>
#include <iprt/thread.h>

#include "internal-r0drv-nt.h"
#include "internal/magics.h"

/** This seems to provide better accuracy. */
#define RTR0TIMER_NT_MANUAL_RE_ARM 1


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * A sub timer structure.
 *
 * This is used for keeping the per-cpu tick and DPC object.
 */
typedef struct RTTIMERNTSUBTIMER
{
    /** The tick counter. */
    uint64_t                iTick;
    /** Pointer to the parent timer. */
    PRTTIMER                pParent;
    /** Thread active executing the worker function, NIL if inactive. */
    RTNATIVETHREAD volatile hActiveThread;
    /** The NT DPC object. */
    KDPC                    NtDpc;
} RTTIMERNTSUBTIMER;
/** Pointer to a NT sub-timer structure. */
typedef RTTIMERNTSUBTIMER *PRTTIMERNTSUBTIMER;

/**
 * The internal representation of an Linux timer handle.
 */
typedef struct RTTIMER
{
    /** Magic.
     * This is RTTIMER_MAGIC, but changes to something else before the timer
     * is destroyed to indicate clearly that thread should exit. */
    uint32_t volatile       u32Magic;
    /** Suspend count down for single shot omnit timers. */
    int32_t volatile        cOmniSuspendCountDown;
    /** Flag indicating the timer is suspended. */
    bool volatile           fSuspended;
    /** Whether the timer must run on one specific CPU or not. */
    bool                    fSpecificCpu;
    /** Whether the timer must run on all CPUs or not. */
    bool                    fOmniTimer;
    /** The CPU it must run on if fSpecificCpu is set.
     * The master CPU for an omni-timer. */
    RTCPUID                 idCpu;
    /** Callback. */
    PFNRTTIMER              pfnTimer;
    /** User argument. */
    void                   *pvUser;
    /** The timer interval. 0 if one-shot. */
    uint64_t                u64NanoInterval;
#ifdef RTR0TIMER_NT_MANUAL_RE_ARM
    /** The desired NT time of the first tick. */
    uint64_t                uNtStartTime;
#endif
    /** The Nt timer object. */
    KTIMER                  NtTimer;
    /** The number of sub-timers. */
    RTCPUID                 cSubTimers;
    /** Sub-timers.
     * Normally there is just one, but for RTTIMER_FLAGS_CPU_ALL this will contain
     * an entry for all possible cpus. In that case the index will be the same as
     * for the RTCpuSet. */
    RTTIMERNTSUBTIMER       aSubTimers[1];
} RTTIMER;


#ifdef RTR0TIMER_NT_MANUAL_RE_ARM
/**
 * Get current NT interrupt time.
 * @return NT interrupt time
 */
static uint64_t rtTimerNtQueryInterruptTime(void)
{
# ifdef RT_ARCH_AMD64
    return KeQueryInterruptTime(); /* macro */
# else
    if (g_pfnrtKeQueryInterruptTime)
        return g_pfnrtKeQueryInterruptTime();

    /* NT4 */
    ULARGE_INTEGER InterruptTime;
    do
    {
        InterruptTime.HighPart = ((KUSER_SHARED_DATA volatile *)SharedUserData)->InterruptTime.High1Time;
        InterruptTime.LowPart  = ((KUSER_SHARED_DATA volatile *)SharedUserData)->InterruptTime.LowPart;
    } while (((KUSER_SHARED_DATA volatile *)SharedUserData)->InterruptTime.High2Time != (LONG)InterruptTime.HighPart);
    return InterruptTime.QuadPart;
# endif
}
#endif /* RTR0TIMER_NT_MANUAL_RE_ARM */


/**
 * Manually re-arms an internval timer.
 *
 * Turns out NT doesn't necessarily do a very good job at re-arming timers
 * accurately.
 *
 * @param   pTimer              The timer.
 * @param   iTick               The current timer tick.
 * @param   pMasterDpc          The master DPC.
 */
DECLINLINE(void) rtTimerNtRearmInternval(PRTTIMER pTimer, uint64_t iTick, PKDPC pMasterDpc)
{
#ifdef RTR0TIMER_NT_MANUAL_RE_ARM
    Assert(pTimer->u64NanoInterval);
    RT_NOREF1(pMasterDpc);

    uint64_t uNtNext = (iTick * pTimer->u64NanoInterval) / 100 - 10; /* 1us fudge */
    LARGE_INTEGER DueTime;
    DueTime.QuadPart = rtTimerNtQueryInterruptTime() - pTimer->uNtStartTime;
    if (DueTime.QuadPart < 0)
        DueTime.QuadPart = 0;
    if ((uint64_t)DueTime.QuadPart < uNtNext)
        DueTime.QuadPart -= uNtNext;
    else
        DueTime.QuadPart = -2500; /* 0.25ms */

    KeSetTimerEx(&pTimer->NtTimer, DueTime, 0, &pTimer->aSubTimers[0].NtDpc);
#else
    RT_NOREF3(pTimer, iTick, pMasterDpc);
#endif
}


/**
 * Timer callback function for the non-omni timers.
 *
 * @returns HRTIMER_NORESTART or HRTIMER_RESTART depending on whether it's a one-shot or interval timer.
 * @param   pDpc                Pointer to the DPC.
 * @param   pvUser              Pointer to our internal timer structure.
 * @param   SystemArgument1     Some system argument.
 * @param   SystemArgument2     Some system argument.
 */
static void _stdcall rtTimerNtSimpleCallback(IN PKDPC pDpc, IN PVOID pvUser, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
{
    PRTTIMER pTimer = (PRTTIMER)pvUser;
    AssertPtr(pTimer);
#ifdef RT_STRICT
    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
        RTAssertMsg2Weak("rtTimerNtSimpleCallback: Irql=%d expected >=%d\n", KeGetCurrentIrql(), DISPATCH_LEVEL);
#endif

    /*
     * Check that we haven't been suspended before doing the callout.
     */
    if (    !ASMAtomicUoReadBool(&pTimer->fSuspended)
        &&  pTimer->u32Magic == RTTIMER_MAGIC)
    {
        ASMAtomicWriteHandle(&pTimer->aSubTimers[0].hActiveThread, RTThreadNativeSelf());

        if (!pTimer->u64NanoInterval)
            ASMAtomicWriteBool(&pTimer->fSuspended, true);
        uint64_t iTick = ++pTimer->aSubTimers[0].iTick;
        if (pTimer->u64NanoInterval)
            rtTimerNtRearmInternval(pTimer, iTick, &pTimer->aSubTimers[0].NtDpc);
        pTimer->pfnTimer(pTimer, pTimer->pvUser, iTick);

        ASMAtomicWriteHandle(&pTimer->aSubTimers[0].hActiveThread, NIL_RTNATIVETHREAD);
    }

    NOREF(pDpc); NOREF(SystemArgument1); NOREF(SystemArgument2);
}


/**
 * The slave DPC callback for an omni timer.
 *
 * @param   pDpc                The DPC object.
 * @param   pvUser              Pointer to the sub-timer.
 * @param   SystemArgument1     Some system stuff.
 * @param   SystemArgument2     Some system stuff.
 */
static void _stdcall rtTimerNtOmniSlaveCallback(IN PKDPC pDpc, IN PVOID pvUser, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
{
    PRTTIMERNTSUBTIMER pSubTimer = (PRTTIMERNTSUBTIMER)pvUser;
    PRTTIMER pTimer = pSubTimer->pParent;

    AssertPtr(pTimer);
#ifdef RT_STRICT
    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
        RTAssertMsg2Weak("rtTimerNtOmniSlaveCallback: Irql=%d expected >=%d\n", KeGetCurrentIrql(), DISPATCH_LEVEL);
    int iCpuSelf = RTMpCpuIdToSetIndex(RTMpCpuId());
    if (pSubTimer - &pTimer->aSubTimers[0] != iCpuSelf)
        RTAssertMsg2Weak("rtTimerNtOmniSlaveCallback: iCpuSelf=%d pSubTimer=%p / %d\n", iCpuSelf, pSubTimer, pSubTimer - &pTimer->aSubTimers[0]);
#endif

    /*
     * Check that we haven't been suspended before doing the callout.
     */
    if (    !ASMAtomicUoReadBool(&pTimer->fSuspended)
        &&  pTimer->u32Magic == RTTIMER_MAGIC)
    {
        ASMAtomicWriteHandle(&pSubTimer->hActiveThread, RTThreadNativeSelf());

        if (!pTimer->u64NanoInterval)
            if (ASMAtomicDecS32(&pTimer->cOmniSuspendCountDown) <= 0)
                ASMAtomicWriteBool(&pTimer->fSuspended, true);

        pTimer->pfnTimer(pTimer, pTimer->pvUser, ++pSubTimer->iTick);

        ASMAtomicWriteHandle(&pSubTimer->hActiveThread, NIL_RTNATIVETHREAD);
    }

    NOREF(pDpc); NOREF(SystemArgument1); NOREF(SystemArgument2);
}


/**
 * The timer callback for an omni-timer.
 *
 * This is responsible for queueing the DPCs for the other CPUs and
 * perform the callback on the CPU on which it is called.
 *
 * @param   pDpc                The DPC object.
 * @param   pvUser              Pointer to the sub-timer.
 * @param   SystemArgument1     Some system stuff.
 * @param   SystemArgument2     Some system stuff.
 */
static void _stdcall rtTimerNtOmniMasterCallback(IN PKDPC pDpc, IN PVOID pvUser, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
{
    PRTTIMERNTSUBTIMER pSubTimer = (PRTTIMERNTSUBTIMER)pvUser;
    PRTTIMER pTimer = pSubTimer->pParent;
    int iCpuSelf = RTMpCpuIdToSetIndex(RTMpCpuId());

    AssertPtr(pTimer);
#ifdef RT_STRICT
    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
        RTAssertMsg2Weak("rtTimerNtOmniMasterCallback: Irql=%d expected >=%d\n", KeGetCurrentIrql(), DISPATCH_LEVEL);
    if (pSubTimer - &pTimer->aSubTimers[0] != iCpuSelf)
        RTAssertMsg2Weak("rtTimerNtOmniMasterCallback: iCpuSelf=%d pSubTimer=%p / %d\n", iCpuSelf, pSubTimer, pSubTimer - &pTimer->aSubTimers[0]);
#endif

    /*
     * Check that we haven't been suspended before scheduling the other DPCs
     * and doing the callout.
     */
    if (    !ASMAtomicUoReadBool(&pTimer->fSuspended)
        &&  pTimer->u32Magic == RTTIMER_MAGIC)
    {
        RTCPUSET    OnlineSet;
        RTMpGetOnlineSet(&OnlineSet);

        ASMAtomicWriteHandle(&pSubTimer->hActiveThread, RTThreadNativeSelf());

        if (pTimer->u64NanoInterval)
        {
            /*
             * Recurring timer.
             */
            for (int iCpu = 0; iCpu < RTCPUSET_MAX_CPUS; iCpu++)
                if (    RTCpuSetIsMemberByIndex(&OnlineSet, iCpu)
                    &&  iCpuSelf != iCpu)
                    KeInsertQueueDpc(&pTimer->aSubTimers[iCpu].NtDpc, 0, 0);

            uint64_t iTick = ++pSubTimer->iTick;
            rtTimerNtRearmInternval(pTimer, iTick, &pTimer->aSubTimers[RTMpCpuIdToSetIndex(pTimer->idCpu)].NtDpc);
            pTimer->pfnTimer(pTimer, pTimer->pvUser, iTick);
        }
        else
        {
            /*
             * Single shot timers gets complicated wrt to fSuspended maintance.
             */
            uint32_t cCpus = 0;
            for (int iCpu = 0; iCpu < RTCPUSET_MAX_CPUS; iCpu++)
                if (RTCpuSetIsMemberByIndex(&OnlineSet, iCpu))
                    cCpus++;
            ASMAtomicAddS32(&pTimer->cOmniSuspendCountDown, cCpus);

            for (int iCpu = 0; iCpu < RTCPUSET_MAX_CPUS; iCpu++)
                if (    RTCpuSetIsMemberByIndex(&OnlineSet, iCpu)
                    &&  iCpuSelf != iCpu)
                    if (!KeInsertQueueDpc(&pTimer->aSubTimers[iCpu].NtDpc, 0, 0))
                        ASMAtomicDecS32(&pTimer->cOmniSuspendCountDown); /* already queued and counted. */

            if (ASMAtomicDecS32(&pTimer->cOmniSuspendCountDown) <= 0)
                ASMAtomicWriteBool(&pTimer->fSuspended, true);

            pTimer->pfnTimer(pTimer, pTimer->pvUser, ++pSubTimer->iTick);
        }

        ASMAtomicWriteHandle(&pSubTimer->hActiveThread, NIL_RTNATIVETHREAD);
    }

    NOREF(pDpc); NOREF(SystemArgument1); NOREF(SystemArgument2);
}



RTDECL(int) RTTimerStart(PRTTIMER pTimer, uint64_t u64First)
{
    /*
     * Validate.
     */
    AssertPtrReturn(pTimer, VERR_INVALID_HANDLE);
    AssertReturn(pTimer->u32Magic == RTTIMER_MAGIC, VERR_INVALID_HANDLE);

    if (!ASMAtomicUoReadBool(&pTimer->fSuspended))
        return VERR_TIMER_ACTIVE;
    if (   pTimer->fSpecificCpu
        && !RTMpIsCpuOnline(pTimer->idCpu))
        return VERR_CPU_OFFLINE;

    /*
     * Start the timer.
     */
    PKDPC pMasterDpc = pTimer->fOmniTimer
                     ? &pTimer->aSubTimers[RTMpCpuIdToSetIndex(pTimer->idCpu)].NtDpc
                     : &pTimer->aSubTimers[0].NtDpc;

#ifndef RTR0TIMER_NT_MANUAL_RE_ARM
    uint64_t u64Interval = pTimer->u64NanoInterval / 1000000; /* This is ms, believe it or not. */
    ULONG ulInterval = (ULONG)u64Interval;
    if (ulInterval != u64Interval)
        ulInterval = MAXLONG;
    else if (!ulInterval && pTimer->u64NanoInterval)
        ulInterval = 1;
#endif

    LARGE_INTEGER DueTime;
    DueTime.QuadPart = -(int64_t)(u64First / 100); /* Relative, NT time. */
    if (!DueTime.QuadPart)
        DueTime.QuadPart = -1;

    unsigned cSubTimers = pTimer->fOmniTimer ? pTimer->cSubTimers : 1;
    for (unsigned iCpu = 0; iCpu < cSubTimers; iCpu++)
        pTimer->aSubTimers[iCpu].iTick = 0;
    ASMAtomicWriteS32(&pTimer->cOmniSuspendCountDown, 0);
    ASMAtomicWriteBool(&pTimer->fSuspended, false);
#ifdef RTR0TIMER_NT_MANUAL_RE_ARM
    pTimer->uNtStartTime = rtTimerNtQueryInterruptTime() + u64First / 100;
    KeSetTimerEx(&pTimer->NtTimer, DueTime, 0, pMasterDpc);
#else
    KeSetTimerEx(&pTimer->NtTimer, DueTime, ulInterval, pMasterDpc);
#endif
    return VINF_SUCCESS;
}


/**
 * Worker function that stops an active timer.
 *
 * Shared by RTTimerStop and RTTimerDestroy.
 *
 * @param   pTimer      The active timer.
 */
static void rtTimerNtStopWorker(PRTTIMER pTimer)
{
    /*
     * Just cancel the timer, dequeue the DPCs and flush them (if this is supported).
     */
    ASMAtomicWriteBool(&pTimer->fSuspended, true);

    KeCancelTimer(&pTimer->NtTimer);

    for (RTCPUID iCpu = 0; iCpu < pTimer->cSubTimers; iCpu++)
        KeRemoveQueueDpc(&pTimer->aSubTimers[iCpu].NtDpc);
}


RTDECL(int) RTTimerStop(PRTTIMER pTimer)
{
    /*
     * Validate.
     */
    AssertPtrReturn(pTimer, VERR_INVALID_HANDLE);
    AssertReturn(pTimer->u32Magic == RTTIMER_MAGIC, VERR_INVALID_HANDLE);

    if (ASMAtomicUoReadBool(&pTimer->fSuspended))
        return VERR_TIMER_SUSPENDED;

    /*
     * Call the worker we share with RTTimerDestroy.
     */
    rtTimerNtStopWorker(pTimer);
    return VINF_SUCCESS;
}


RTDECL(int) RTTimerChangeInterval(PRTTIMER pTimer, uint64_t u64NanoInterval)
{
    AssertPtrReturn(pTimer, VERR_INVALID_HANDLE);
    AssertReturn(pTimer->u32Magic == RTTIMER_MAGIC, VERR_INVALID_HANDLE);
    RT_NOREF1(u64NanoInterval);

    return VERR_NOT_SUPPORTED;
}


RTDECL(int) RTTimerDestroy(PRTTIMER pTimer)
{
    /* It's ok to pass NULL pointer. */
    if (pTimer == /*NIL_RTTIMER*/ NULL)
        return VINF_SUCCESS;
    AssertPtrReturn(pTimer, VERR_INVALID_HANDLE);
    AssertReturn(pTimer->u32Magic == RTTIMER_MAGIC, VERR_INVALID_HANDLE);

    /*
     * We do not support destroying a timer from the callback because it is
     * not 101% safe since we cannot flush DPCs.  Solaris has the same restriction.
     */
    AssertReturn(KeGetCurrentIrql() == PASSIVE_LEVEL, VERR_INVALID_CONTEXT);

    /*
     * Invalidate the timer, stop it if it's running and finally
     * free up the memory.
     */
    ASMAtomicWriteU32(&pTimer->u32Magic, ~RTTIMER_MAGIC);
    if (!ASMAtomicUoReadBool(&pTimer->fSuspended))
        rtTimerNtStopWorker(pTimer);

    /*
     * Flush DPCs to be on the safe side.
     */
    if (g_pfnrtNtKeFlushQueuedDpcs)
        g_pfnrtNtKeFlushQueuedDpcs();

    RTMemFree(pTimer);

    return VINF_SUCCESS;
}


RTDECL(int) RTTimerCreateEx(PRTTIMER *ppTimer, uint64_t u64NanoInterval, uint32_t fFlags, PFNRTTIMER pfnTimer, void *pvUser)
{
    *ppTimer = NULL;

    /*
     * Validate flags.
     */
    if (!RTTIMER_FLAGS_ARE_VALID(fFlags))
        return VERR_INVALID_PARAMETER;
    if (    (fFlags & RTTIMER_FLAGS_CPU_SPECIFIC)
        &&  (fFlags & RTTIMER_FLAGS_CPU_ALL) != RTTIMER_FLAGS_CPU_ALL
        &&  !RTMpIsCpuPossible(RTMpCpuIdFromSetIndex(fFlags & RTTIMER_FLAGS_CPU_MASK)))
        return VERR_CPU_NOT_FOUND;

    /*
     * Allocate the timer handler.
     */
    RTCPUID cSubTimers = 1;
    if ((fFlags & RTTIMER_FLAGS_CPU_ALL) == RTTIMER_FLAGS_CPU_ALL)
    {
        cSubTimers = RTMpGetMaxCpuId() + 1;
        Assert(cSubTimers <= RTCPUSET_MAX_CPUS); /* On Windows we have a 1:1 relationship between cpuid and set index. */
    }

    PRTTIMER pTimer = (PRTTIMER)RTMemAllocZ(RT_OFFSETOF(RTTIMER, aSubTimers[cSubTimers]));
    if (!pTimer)
        return VERR_NO_MEMORY;

    /*
     * Initialize it.
     */
    pTimer->u32Magic = RTTIMER_MAGIC;
    pTimer->cOmniSuspendCountDown = 0;
    pTimer->fSuspended = true;
    pTimer->fSpecificCpu = (fFlags & RTTIMER_FLAGS_CPU_SPECIFIC) && (fFlags & RTTIMER_FLAGS_CPU_ALL) != RTTIMER_FLAGS_CPU_ALL;
    pTimer->fOmniTimer = (fFlags & RTTIMER_FLAGS_CPU_ALL) == RTTIMER_FLAGS_CPU_ALL;
    pTimer->idCpu = pTimer->fSpecificCpu ? RTMpCpuIdFromSetIndex(fFlags & RTTIMER_FLAGS_CPU_MASK) : NIL_RTCPUID;
    pTimer->cSubTimers = cSubTimers;
    pTimer->pfnTimer = pfnTimer;
    pTimer->pvUser = pvUser;
    pTimer->u64NanoInterval = u64NanoInterval;
    KeInitializeTimerEx(&pTimer->NtTimer, SynchronizationTimer);
    int rc = VINF_SUCCESS;
    if (pTimer->fOmniTimer)
    {
        /*
         * Initialize the per-cpu "sub-timers", select the first online cpu
         * to be the master.
         * ASSUMES that no cpus will ever go offline.
         */
        pTimer->idCpu = NIL_RTCPUID;
        for (unsigned iCpu = 0; iCpu < cSubTimers && RT_SUCCESS(rc); iCpu++)
        {
            pTimer->aSubTimers[iCpu].iTick = 0;
            pTimer->aSubTimers[iCpu].pParent = pTimer;

            if (    pTimer->idCpu == NIL_RTCPUID
                &&  RTMpIsCpuOnline(RTMpCpuIdFromSetIndex(iCpu)))
            {
                pTimer->idCpu = RTMpCpuIdFromSetIndex(iCpu);
                KeInitializeDpc(&pTimer->aSubTimers[iCpu].NtDpc, rtTimerNtOmniMasterCallback, &pTimer->aSubTimers[iCpu]);
            }
            else
                KeInitializeDpc(&pTimer->aSubTimers[iCpu].NtDpc, rtTimerNtOmniSlaveCallback, &pTimer->aSubTimers[iCpu]);
            KeSetImportanceDpc(&pTimer->aSubTimers[iCpu].NtDpc, HighImportance);
            rc = rtMpNtSetTargetProcessorDpc(&pTimer->aSubTimers[iCpu].NtDpc, iCpu);
        }
        Assert(pTimer->idCpu != NIL_RTCPUID);
    }
    else
    {
        /*
         * Initialize the first "sub-timer", target the DPC on a specific processor
         * if requested to do so.
         */
        pTimer->aSubTimers[0].iTick = 0;
        pTimer->aSubTimers[0].pParent = pTimer;

        KeInitializeDpc(&pTimer->aSubTimers[0].NtDpc, rtTimerNtSimpleCallback, pTimer);
        KeSetImportanceDpc(&pTimer->aSubTimers[0].NtDpc, HighImportance);
        if (pTimer->fSpecificCpu)
            rc = rtMpNtSetTargetProcessorDpc(&pTimer->aSubTimers[0].NtDpc, (int)pTimer->idCpu);
    }
    if (RT_SUCCESS(rc))
    {
        *ppTimer = pTimer;
        return VINF_SUCCESS;
    }

    RTMemFree(pTimer);
    return rc;
}


RTDECL(int) RTTimerRequestSystemGranularity(uint32_t u32Request, uint32_t *pu32Granted)
{
    if (!g_pfnrtNtExSetTimerResolution)
        return VERR_NOT_SUPPORTED;

    ULONG ulGranted = g_pfnrtNtExSetTimerResolution(u32Request / 100, TRUE);
    if (pu32Granted)
        *pu32Granted = ulGranted * 100; /* NT -> ns */
    return VINF_SUCCESS;
}


RTDECL(int) RTTimerReleaseSystemGranularity(uint32_t u32Granted)
{
    if (!g_pfnrtNtExSetTimerResolution)
        return VERR_NOT_SUPPORTED;

    g_pfnrtNtExSetTimerResolution(0 /* ignored */, FALSE);
    NOREF(u32Granted);
    return VINF_SUCCESS;
}


RTDECL(bool) RTTimerCanDoHighResolution(void)
{
    return false;
}

