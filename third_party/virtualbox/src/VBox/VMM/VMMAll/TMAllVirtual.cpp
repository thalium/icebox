/* $Id: TMAllVirtual.cpp $ */
/** @file
 * TM - Timeout Manager, Virtual Time, All Contexts.
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
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_TM
#include <VBox/vmm/tm.h>
#include <VBox/vmm/dbgftrace.h>
#ifdef IN_RING3
# ifdef VBOX_WITH_REM
#  include <VBox/vmm/rem.h>
# endif
# include <iprt/thread.h>
#endif
#include "TMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/vmm.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <VBox/sup.h>

#include <iprt/time.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/asm-math.h>



/**
 * @interface_method_impl{RTTIMENANOTSDATA,pfnBad}
 */
DECLCALLBACK(DECLEXPORT(void)) tmVirtualNanoTSBad(PRTTIMENANOTSDATA pData, uint64_t u64NanoTS, uint64_t u64DeltaPrev,
                                                  uint64_t u64PrevNanoTS)
{
    PVM pVM = RT_FROM_MEMBER(pData, VM, CTX_SUFF(tm.s.VirtualGetRawData));
    pData->cBadPrev++;
    if ((int64_t)u64DeltaPrev < 0)
        LogRel(("TM: u64DeltaPrev=%RI64 u64PrevNanoTS=0x%016RX64 u64NanoTS=0x%016RX64 pVM=%p\n",
                u64DeltaPrev, u64PrevNanoTS, u64NanoTS, pVM));
    else
        Log(("TM: u64DeltaPrev=%RI64 u64PrevNanoTS=0x%016RX64 u64NanoTS=0x%016RX64 pVM=%p (debugging?)\n",
             u64DeltaPrev, u64PrevNanoTS, u64NanoTS, pVM));
}


/**
 * @interface_method_impl{RTTIMENANOTSDATA,pfnRediscover}
 *
 * This is the initial worker, so the first call in each context ends up here.
 * It is also used should the delta rating of the host CPUs change or if the
 * fGetGipCpu feature the current worker relies upon becomes unavailable.  The
 * last two events may occur as CPUs are taken online.
 */
DECLCALLBACK(DECLEXPORT(uint64_t)) tmVirtualNanoTSRediscover(PRTTIMENANOTSDATA pData)
{
    PVM pVM = RT_FROM_MEMBER(pData, VM, CTX_SUFF(tm.s.VirtualGetRawData));

    /*
     * We require a valid GIP for the selection below.  Invalid GIP is fatal.
     */
    PSUPGLOBALINFOPAGE pGip = g_pSUPGlobalInfoPage;
    AssertFatalMsg(RT_VALID_PTR(pGip), ("pVM=%p pGip=%p\n", pVM, pGip));
    AssertFatalMsg(pGip->u32Magic == SUPGLOBALINFOPAGE_MAGIC, ("pVM=%p pGip=%p u32Magic=%#x\n", pVM, pGip, pGip->u32Magic));
    AssertFatalMsg(pGip->u32Mode > SUPGIPMODE_INVALID && pGip->u32Mode < SUPGIPMODE_END,
                   ("pVM=%p pGip=%p u32Mode=%#x\n", pVM, pGip, pGip->u32Mode));

    /*
     * Determine the new worker.
     */
    PFNTIMENANOTSINTERNAL   pfnWorker;
    bool const              fLFence = RT_BOOL(ASMCpuId_EDX(1) & X86_CPUID_FEATURE_EDX_SSE2);
    switch (pGip->u32Mode)
    {
        case SUPGIPMODE_SYNC_TSC:
        case SUPGIPMODE_INVARIANT_TSC:
#if defined(IN_RC) || defined(IN_RING0)
            if (pGip->enmUseTscDelta <= SUPGIPUSETSCDELTA_ROUGHLY_ZERO)
                pfnWorker = fLFence ? RTTimeNanoTSLFenceSyncInvarNoDelta    : RTTimeNanoTSLegacySyncInvarNoDelta;
            else
                pfnWorker = fLFence ? RTTimeNanoTSLFenceSyncInvarWithDelta  : RTTimeNanoTSLegacySyncInvarWithDelta;
#else
            if (pGip->fGetGipCpu & SUPGIPGETCPU_IDTR_LIMIT_MASK_MAX_SET_CPUS)
                pfnWorker = pGip->enmUseTscDelta <= SUPGIPUSETSCDELTA_PRACTICALLY_ZERO
                          ? fLFence ? RTTimeNanoTSLFenceSyncInvarNoDelta    : RTTimeNanoTSLegacySyncInvarNoDelta
                          : fLFence ? RTTimeNanoTSLFenceSyncInvarWithDeltaUseIdtrLim : RTTimeNanoTSLegacySyncInvarWithDeltaUseIdtrLim;
            else if (pGip->fGetGipCpu & SUPGIPGETCPU_RDTSCP_MASK_MAX_SET_CPUS)
                pfnWorker = pGip->enmUseTscDelta <= SUPGIPUSETSCDELTA_PRACTICALLY_ZERO
                          ? fLFence ? RTTimeNanoTSLFenceSyncInvarNoDelta    : RTTimeNanoTSLegacySyncInvarNoDelta
                          : fLFence ? RTTimeNanoTSLFenceSyncInvarWithDeltaUseRdtscp : RTTimeNanoTSLegacySyncInvarWithDeltaUseRdtscp;
            else
                pfnWorker = pGip->enmUseTscDelta <= SUPGIPUSETSCDELTA_ROUGHLY_ZERO
                          ? fLFence ? RTTimeNanoTSLFenceSyncInvarNoDelta    : RTTimeNanoTSLegacySyncInvarNoDelta
                          : fLFence ? RTTimeNanoTSLFenceSyncInvarWithDeltaUseApicId : RTTimeNanoTSLegacySyncInvarWithDeltaUseApicId;
#endif
            break;

        case SUPGIPMODE_ASYNC_TSC:
#if defined(IN_RC) || defined(IN_RING0)
            pfnWorker = fLFence ? RTTimeNanoTSLFenceAsync : RTTimeNanoTSLegacyAsync;
#else
            if (pGip->fGetGipCpu & SUPGIPGETCPU_IDTR_LIMIT_MASK_MAX_SET_CPUS)
                pfnWorker = fLFence ? RTTimeNanoTSLFenceAsyncUseIdtrLim     : RTTimeNanoTSLegacyAsyncUseIdtrLim;
            else if (pGip->fGetGipCpu & SUPGIPGETCPU_RDTSCP_MASK_MAX_SET_CPUS)
                pfnWorker = fLFence ? RTTimeNanoTSLFenceAsyncUseRdtscp      : RTTimeNanoTSLegacyAsyncUseRdtscp;
            else if (pGip->fGetGipCpu & SUPGIPGETCPU_RDTSCP_GROUP_IN_CH_NUMBER_IN_CL)
                pfnWorker = fLFence ? RTTimeNanoTSLFenceAsyncUseRdtscpGroupChNumCl : RTTimeNanoTSLegacyAsyncUseRdtscpGroupChNumCl;
            else
                pfnWorker = fLFence ? RTTimeNanoTSLFenceAsyncUseApicId      : RTTimeNanoTSLegacyAsyncUseApicId;
#endif
            break;

        default:
            AssertFatalMsgFailed(("pVM=%p pGip=%p u32Mode=%#x\n", pVM, pGip, pGip->u32Mode));
    }

    /*
     * Update the pfnVirtualGetRaw pointer and call the worker we selected.
     */
    ASMAtomicWritePtr((void * volatile *)&CTX_SUFF(pVM->tm.s.pfnVirtualGetRaw), (void *)(uintptr_t)pfnWorker);
    return pfnWorker(pData);
}


/**
 * @interface_method_impl{RTTIMENANOTSDATA,pfnBadCpuIndex}
 */
DECLEXPORT(uint64_t) tmVirtualNanoTSBadCpuIndex(PRTTIMENANOTSDATA pData, uint16_t idApic, uint16_t iCpuSet, uint16_t iGipCpu)
{
    PVM pVM = RT_FROM_MEMBER(pData, VM, CTX_SUFF(tm.s.VirtualGetRawData));
    AssertFatalMsgFailed(("pVM=%p idApic=%#x iCpuSet=%#x iGipCpu=%#x\n", pVM, idApic, iCpuSet, iGipCpu));
#ifndef _MSC_VER
    return UINT64_MAX;
#endif
}


/**
 * Wrapper around the IPRT GIP time methods.
 */
DECLINLINE(uint64_t) tmVirtualGetRawNanoTS(PVM pVM)
{
# ifdef IN_RING3
    uint64_t u64 = CTXALLSUFF(pVM->tm.s.pfnVirtualGetRaw)(&CTXALLSUFF(pVM->tm.s.VirtualGetRawData));
# else  /* !IN_RING3 */
    uint32_t cPrevSteps = pVM->tm.s.CTX_SUFF(VirtualGetRawData).c1nsSteps;
    uint64_t u64 = pVM->tm.s.CTX_SUFF(pfnVirtualGetRaw)(&pVM->tm.s.CTX_SUFF(VirtualGetRawData));
    if (cPrevSteps != pVM->tm.s.CTX_SUFF(VirtualGetRawData).c1nsSteps)
        VMCPU_FF_SET(VMMGetCpu(pVM), VMCPU_FF_TO_R3);
# endif /* !IN_RING3 */
    /*DBGFTRACE_POS_U64(pVM, u64);*/
    return u64;
}


/**
 * Get the time when we're not running at 100%
 *
 * @returns The timestamp.
 * @param   pVM     The cross context VM structure.
 */
static uint64_t tmVirtualGetRawNonNormal(PVM pVM)
{
    /*
     * Recalculate the RTTimeNanoTS() value for the period where
     * warp drive has been enabled.
     */
    uint64_t u64 = tmVirtualGetRawNanoTS(pVM);
    u64 -= pVM->tm.s.u64VirtualWarpDriveStart;
    u64 *= pVM->tm.s.u32VirtualWarpDrivePercentage;
    u64 /= 100;
    u64 += pVM->tm.s.u64VirtualWarpDriveStart;

    /*
     * Now we apply the virtual time offset.
     * (Which is the negated tmVirtualGetRawNanoTS() value for when the virtual
     * machine started if it had been running continuously without any suspends.)
     */
    u64 -= pVM->tm.s.u64VirtualOffset;
    return u64;
}


/**
 * Get the raw virtual time.
 *
 * @returns The current time stamp.
 * @param   pVM     The cross context VM structure.
 */
DECLINLINE(uint64_t) tmVirtualGetRaw(PVM pVM)
{
    if (RT_LIKELY(!pVM->tm.s.fVirtualWarpDrive))
        return tmVirtualGetRawNanoTS(pVM) - pVM->tm.s.u64VirtualOffset;
    return tmVirtualGetRawNonNormal(pVM);
}


/**
 * Inlined version of tmVirtualGetEx.
 */
DECLINLINE(uint64_t) tmVirtualGet(PVM pVM, bool fCheckTimers)
{
    uint64_t u64;
    if (RT_LIKELY(pVM->tm.s.cVirtualTicking))
    {
        STAM_COUNTER_INC(&pVM->tm.s.StatVirtualGet);
        u64 = tmVirtualGetRaw(pVM);

        /*
         * Use the chance to check for expired timers.
         */
        if (fCheckTimers)
        {
            PVMCPU pVCpuDst = &pVM->aCpus[pVM->tm.s.idTimerCpu];
            if (    !VMCPU_FF_IS_SET(pVCpuDst, VMCPU_FF_TIMER)
                &&  !pVM->tm.s.fRunningQueues
                &&  (   pVM->tm.s.CTX_SUFF(paTimerQueues)[TMCLOCK_VIRTUAL].u64Expire <= u64
                     || (   pVM->tm.s.fVirtualSyncTicking
                         && pVM->tm.s.CTX_SUFF(paTimerQueues)[TMCLOCK_VIRTUAL_SYNC].u64Expire <= u64 - pVM->tm.s.offVirtualSync
                        )
                    )
                &&  !pVM->tm.s.fRunningQueues
               )
            {
                STAM_COUNTER_INC(&pVM->tm.s.StatVirtualGetSetFF);
                Log5(("TMAllVirtual(%u): FF: %d -> 1\n", __LINE__, VMCPU_FF_IS_PENDING(pVCpuDst, VMCPU_FF_TIMER)));
                VMCPU_FF_SET(pVCpuDst, VMCPU_FF_TIMER);
#ifdef IN_RING3
# ifdef VBOX_WITH_REM
                REMR3NotifyTimerPending(pVM, pVCpuDst);
# endif
                VMR3NotifyCpuFFU(pVCpuDst->pUVCpu, VMNOTIFYFF_FLAGS_DONE_REM);
#endif
            }
        }
    }
    else
        u64 = pVM->tm.s.u64Virtual;
    return u64;
}


/**
 * Gets the current TMCLOCK_VIRTUAL time
 *
 * @returns The timestamp.
 * @param   pVM     The cross context VM structure.
 *
 * @remark  While the flow of time will never go backwards, the speed of the
 *          progress varies due to inaccurate RTTimeNanoTS and TSC. The latter can be
 *          influenced by power saving (SpeedStep, PowerNow!), while the former
 *          makes use of TSC and kernel timers.
 */
VMM_INT_DECL(uint64_t) TMVirtualGet(PVM pVM)
{
    return tmVirtualGet(pVM, true /*fCheckTimers*/);
}


/**
 * Gets the current TMCLOCK_VIRTUAL time without checking
 * timers or anything.
 *
 * Meaning, this has no side effect on FFs like TMVirtualGet may have.
 *
 * @returns The timestamp.
 * @param   pVM     The cross context VM structure.
 *
 * @remarks See TMVirtualGet.
 */
VMM_INT_DECL(uint64_t) TMVirtualGetNoCheck(PVM pVM)
{
    return tmVirtualGet(pVM, false /*fCheckTimers*/);
}


/**
 * Converts the dead line interval from TMCLOCK_VIRTUAL to host nano seconds.
 *
 * @returns Host nano second count.
 * @param   pVM                     The cross context VM structure.
 * @param   cVirtTicksToDeadline    The TMCLOCK_VIRTUAL interval.
 */
DECLINLINE(uint64_t) tmVirtualVirtToNsDeadline(PVM pVM, uint64_t cVirtTicksToDeadline)
{
    if (RT_UNLIKELY(pVM->tm.s.fVirtualWarpDrive))
        return ASMMultU64ByU32DivByU32(cVirtTicksToDeadline, 100, pVM->tm.s.u32VirtualWarpDrivePercentage);
    return cVirtTicksToDeadline;
}


/**
 * tmVirtualSyncGetLocked worker for handling catch-up when owning the lock.
 *
 * @returns The timestamp.
 * @param   pVM                 The cross context VM structure.
 * @param   u64                 raw virtual time.
 * @param   off                 offVirtualSync.
 * @param   pcNsToDeadline      Where to return the number of nano seconds to
 *                              the next virtual sync timer deadline. Can be
 *                              NULL.
 */
DECLINLINE(uint64_t) tmVirtualSyncGetHandleCatchUpLocked(PVM pVM, uint64_t u64, uint64_t off, uint64_t *pcNsToDeadline)
{
    STAM_COUNTER_INC(&pVM->tm.s.StatVirtualSyncGetLocked);

    /*
     * Don't make updates until we've check the timer queue.
     */
    bool            fUpdatePrev = true;
    bool            fUpdateOff  = true;
    bool            fStop       = false;
    const uint64_t  u64Prev     = pVM->tm.s.u64VirtualSyncCatchUpPrev;
    uint64_t        u64Delta    = u64 - u64Prev;
    if (RT_LIKELY(!(u64Delta >> 32)))
    {
        uint64_t u64Sub = ASMMultU64ByU32DivByU32(u64Delta, pVM->tm.s.u32VirtualSyncCatchUpPercentage, 100);
        if (off > u64Sub + pVM->tm.s.offVirtualSyncGivenUp)
        {
            off -= u64Sub;
            Log4(("TM: %'RU64/-%'8RU64: sub %RU32 [vsghcul]\n", u64 - off, off - pVM->tm.s.offVirtualSyncGivenUp, u64Sub));
        }
        else
        {
            /* we've completely caught up. */
            STAM_PROFILE_ADV_STOP(&pVM->tm.s.StatVirtualSyncCatchup, c);
            off = pVM->tm.s.offVirtualSyncGivenUp;
            fStop = true;
            Log4(("TM: %'RU64/0: caught up [vsghcul]\n", u64));
        }
    }
    else
    {
        /* More than 4 seconds since last time (or negative), ignore it. */
        fUpdateOff = false;
        fUpdatePrev = !(u64Delta & RT_BIT_64(63));
        Log(("TMVirtualGetSync: u64Delta=%RX64\n", u64Delta));
    }

    /*
     * Complete the calculation of the current TMCLOCK_VIRTUAL_SYNC time. The current
     * approach is to never pass the head timer. So, when we do stop the clock and
     * set the timer pending flag.
     */
    u64 -= off;

    uint64_t u64Last = ASMAtomicUoReadU64(&pVM->tm.s.u64VirtualSync);
    if (u64Last > u64)
    {
        u64 = u64Last + 1;
        STAM_COUNTER_INC(&pVM->tm.s.StatVirtualSyncGetAdjLast);
    }

    uint64_t u64Expire = ASMAtomicReadU64(&pVM->tm.s.CTX_SUFF(paTimerQueues)[TMCLOCK_VIRTUAL_SYNC].u64Expire);
    if (u64 < u64Expire)
    {
        ASMAtomicWriteU64(&pVM->tm.s.u64VirtualSync, u64);
        if (fUpdateOff)
            ASMAtomicWriteU64(&pVM->tm.s.offVirtualSync, off);
        if (fStop)
            ASMAtomicWriteBool(&pVM->tm.s.fVirtualSyncCatchUp, false);
        if (fUpdatePrev)
            ASMAtomicWriteU64(&pVM->tm.s.u64VirtualSyncCatchUpPrev, u64);
        if (pcNsToDeadline)
        {
            uint64_t cNsToDeadline = u64Expire - u64;
            if (pVM->tm.s.fVirtualSyncCatchUp)
                cNsToDeadline = ASMMultU64ByU32DivByU32(cNsToDeadline, 100,
                                                        pVM->tm.s.u32VirtualSyncCatchUpPercentage + 100);
            *pcNsToDeadline = tmVirtualVirtToNsDeadline(pVM, cNsToDeadline);
        }
        PDMCritSectLeave(&pVM->tm.s.VirtualSyncLock);
    }
    else
    {
        u64 = u64Expire;
        ASMAtomicWriteU64(&pVM->tm.s.u64VirtualSync, u64);
        ASMAtomicWriteBool(&pVM->tm.s.fVirtualSyncTicking, false);

        VM_FF_SET(pVM, VM_FF_TM_VIRTUAL_SYNC);
        PVMCPU pVCpuDst = &pVM->aCpus[pVM->tm.s.idTimerCpu];
        VMCPU_FF_SET(pVCpuDst, VMCPU_FF_TIMER);
        Log5(("TMAllVirtual(%u): FF: %d -> 1\n", __LINE__, VMCPU_FF_IS_PENDING(pVCpuDst, VMCPU_FF_TIMER)));
        Log4(("TM: %'RU64/-%'8RU64: exp tmr=>ff [vsghcul]\n", u64, pVM->tm.s.offVirtualSync - pVM->tm.s.offVirtualSyncGivenUp));
        PDMCritSectLeave(&pVM->tm.s.VirtualSyncLock);

        if (pcNsToDeadline)
            *pcNsToDeadline = 0;
#ifdef IN_RING3
# ifdef VBOX_WITH_REM
        REMR3NotifyTimerPending(pVM, pVCpuDst);
# endif
        VMR3NotifyCpuFFU(pVCpuDst->pUVCpu, VMNOTIFYFF_FLAGS_DONE_REM);
#endif
        STAM_COUNTER_INC(&pVM->tm.s.StatVirtualSyncGetSetFF);
        STAM_COUNTER_INC(&pVM->tm.s.StatVirtualSyncGetExpired);
    }
    STAM_COUNTER_INC(&pVM->tm.s.StatVirtualSyncGetLocked);

    Log6(("tmVirtualSyncGetHandleCatchUpLocked -> %'RU64\n", u64));
    DBGFTRACE_U64_TAG(pVM, u64, "tmVirtualSyncGetHandleCatchUpLocked");
    return u64;
}


/**
 * tmVirtualSyncGetEx worker for when we get the lock.
 *
 * @returns timesamp.
 * @param   pVM                 The cross context VM structure.
 * @param   u64                 The virtual clock timestamp.
 * @param   pcNsToDeadline      Where to return the number of nano seconds to
 *                              the next virtual sync timer deadline.  Can be
 *                              NULL.
 */
DECLINLINE(uint64_t) tmVirtualSyncGetLocked(PVM pVM, uint64_t u64, uint64_t *pcNsToDeadline)
{
    /*
     * Not ticking?
     */
    if (!pVM->tm.s.fVirtualSyncTicking)
    {
        u64 = ASMAtomicUoReadU64(&pVM->tm.s.u64VirtualSync);
        PDMCritSectLeave(&pVM->tm.s.VirtualSyncLock);
        if (pcNsToDeadline)
            *pcNsToDeadline = 0;
        STAM_COUNTER_INC(&pVM->tm.s.StatVirtualSyncGetLocked);
        Log6(("tmVirtualSyncGetLocked -> %'RU64 [stopped]\n", u64));
        DBGFTRACE_U64_TAG(pVM, u64, "tmVirtualSyncGetLocked-stopped");
        return u64;
    }

    /*
     * Handle catch up in a separate function.
     */
    uint64_t off = ASMAtomicUoReadU64(&pVM->tm.s.offVirtualSync);
    if (ASMAtomicUoReadBool(&pVM->tm.s.fVirtualSyncCatchUp))
        return tmVirtualSyncGetHandleCatchUpLocked(pVM, u64, off, pcNsToDeadline);

    /*
     * Complete the calculation of the current TMCLOCK_VIRTUAL_SYNC time. The current
     * approach is to never pass the head timer. So, when we do stop the clock and
     * set the timer pending flag.
     */
    u64 -= off;

    uint64_t u64Last = ASMAtomicUoReadU64(&pVM->tm.s.u64VirtualSync);
    if (u64Last > u64)
    {
        u64 = u64Last + 1;
        STAM_COUNTER_INC(&pVM->tm.s.StatVirtualSyncGetAdjLast);
    }

    uint64_t u64Expire = ASMAtomicReadU64(&pVM->tm.s.CTX_SUFF(paTimerQueues)[TMCLOCK_VIRTUAL_SYNC].u64Expire);
    if (u64 < u64Expire)
    {
        ASMAtomicWriteU64(&pVM->tm.s.u64VirtualSync, u64);
        PDMCritSectLeave(&pVM->tm.s.VirtualSyncLock);
        if (pcNsToDeadline)
            *pcNsToDeadline = tmVirtualVirtToNsDeadline(pVM, u64Expire - u64);
    }
    else
    {
        u64 = u64Expire;
        ASMAtomicWriteU64(&pVM->tm.s.u64VirtualSync, u64);
        ASMAtomicWriteBool(&pVM->tm.s.fVirtualSyncTicking, false);

        VM_FF_SET(pVM, VM_FF_TM_VIRTUAL_SYNC);
        PVMCPU pVCpuDst = &pVM->aCpus[pVM->tm.s.idTimerCpu];
        VMCPU_FF_SET(pVCpuDst, VMCPU_FF_TIMER);
        Log5(("TMAllVirtual(%u): FF: %d -> 1\n", __LINE__, !!VMCPU_FF_IS_PENDING(pVCpuDst, VMCPU_FF_TIMER)));
        Log4(("TM: %'RU64/-%'8RU64: exp tmr=>ff [vsgl]\n", u64, pVM->tm.s.offVirtualSync - pVM->tm.s.offVirtualSyncGivenUp));
        PDMCritSectLeave(&pVM->tm.s.VirtualSyncLock);

#ifdef IN_RING3
# ifdef VBOX_WITH_REM
        REMR3NotifyTimerPending(pVM, pVCpuDst);
# endif
        VMR3NotifyCpuFFU(pVCpuDst->pUVCpu, VMNOTIFYFF_FLAGS_DONE_REM);
#endif
        if (pcNsToDeadline)
            *pcNsToDeadline = 0;
        STAM_COUNTER_INC(&pVM->tm.s.StatVirtualSyncGetSetFF);
        STAM_COUNTER_INC(&pVM->tm.s.StatVirtualSyncGetExpired);
    }
    STAM_COUNTER_INC(&pVM->tm.s.StatVirtualSyncGetLocked);
    Log6(("tmVirtualSyncGetLocked -> %'RU64\n", u64));
    DBGFTRACE_U64_TAG(pVM, u64, "tmVirtualSyncGetLocked");
    return u64;
}


/**
 * Gets the current TMCLOCK_VIRTUAL_SYNC time.
 *
 * @returns The timestamp.
 * @param   pVM                 The cross context VM structure.
 * @param   fCheckTimers        Check timers or not
 * @param   pcNsToDeadline      Where to return the number of nano seconds to
 *                              the next virtual sync timer deadline.  Can be
 *                              NULL.
 * @thread  EMT.
 */
DECLINLINE(uint64_t) tmVirtualSyncGetEx(PVM pVM, bool fCheckTimers, uint64_t *pcNsToDeadline)
{
    STAM_COUNTER_INC(&pVM->tm.s.StatVirtualSyncGet);

    uint64_t u64;
    if (!pVM->tm.s.fVirtualSyncTicking)
    {
        if (pcNsToDeadline)
            *pcNsToDeadline = 0;
        u64 = pVM->tm.s.u64VirtualSync;
        DBGFTRACE_U64_TAG(pVM, u64, "tmVirtualSyncGetEx-stopped1");
        return u64;
    }

    /*
     * Query the virtual clock and do the usual expired timer check.
     */
    Assert(pVM->tm.s.cVirtualTicking);
    u64 = tmVirtualGetRaw(pVM);
    if (fCheckTimers)
    {
        PVMCPU pVCpuDst = &pVM->aCpus[pVM->tm.s.idTimerCpu];
        if (    !VMCPU_FF_IS_SET(pVCpuDst, VMCPU_FF_TIMER)
            &&  pVM->tm.s.CTX_SUFF(paTimerQueues)[TMCLOCK_VIRTUAL].u64Expire <= u64)
        {
            Log5(("TMAllVirtual(%u): FF: 0 -> 1\n", __LINE__));
            VMCPU_FF_SET(pVCpuDst, VMCPU_FF_TIMER);
#ifdef IN_RING3
# ifdef VBOX_WITH_REM
            REMR3NotifyTimerPending(pVM, pVCpuDst);
# endif
            VMR3NotifyCpuFFU(pVCpuDst->pUVCpu, VMNOTIFYFF_FLAGS_DONE_REM /** @todo |VMNOTIFYFF_FLAGS_POKE*/);
#endif
            STAM_COUNTER_INC(&pVM->tm.s.StatVirtualSyncGetSetFF);
        }
    }

    /*
     * If we can get the lock, get it.  The result is much more reliable.
     *
     * Note! This is where all clock source devices branch off because they
     *       will be owning the lock already.  The 'else' is taken by code
     *       which is less picky or hasn't been adjusted yet
     */
    if (PDMCritSectTryEnter(&pVM->tm.s.VirtualSyncLock) == VINF_SUCCESS)
        return tmVirtualSyncGetLocked(pVM, u64, pcNsToDeadline);

    /*
     * When the clock is ticking, not doing catch ups and not running into an
     * expired time, we can get away without locking. Try this first.
     */
    uint64_t off;
    if (ASMAtomicUoReadBool(&pVM->tm.s.fVirtualSyncTicking))
    {
        if (!ASMAtomicUoReadBool(&pVM->tm.s.fVirtualSyncCatchUp))
        {
            off = ASMAtomicReadU64(&pVM->tm.s.offVirtualSync);
            if (RT_LIKELY(   ASMAtomicUoReadBool(&pVM->tm.s.fVirtualSyncTicking)
                          && !ASMAtomicUoReadBool(&pVM->tm.s.fVirtualSyncCatchUp)
                          && off == ASMAtomicReadU64(&pVM->tm.s.offVirtualSync)))
            {
                off = u64 - off;
                uint64_t const u64Expire = ASMAtomicReadU64(&pVM->tm.s.CTX_SUFF(paTimerQueues)[TMCLOCK_VIRTUAL_SYNC].u64Expire);
                if (off < u64Expire)
                {
                    if (pcNsToDeadline)
                        *pcNsToDeadline = tmVirtualVirtToNsDeadline(pVM, u64Expire - off);
                    STAM_COUNTER_INC(&pVM->tm.s.StatVirtualSyncGetLockless);
                    Log6(("tmVirtualSyncGetEx -> %'RU64 [lockless]\n", off));
                    DBGFTRACE_U64_TAG(pVM, off, "tmVirtualSyncGetEx-lockless");
                    return off;
                }
            }
        }
    }
    else
    {
        off = ASMAtomicReadU64(&pVM->tm.s.u64VirtualSync);
        if (RT_LIKELY(!ASMAtomicReadBool(&pVM->tm.s.fVirtualSyncTicking)))
        {
            if (pcNsToDeadline)
                *pcNsToDeadline = 0;
            STAM_COUNTER_INC(&pVM->tm.s.StatVirtualSyncGetLockless);
            Log6(("tmVirtualSyncGetEx -> %'RU64 [lockless/stopped]\n", off));
            DBGFTRACE_U64_TAG(pVM, off, "tmVirtualSyncGetEx-stopped2");
            return off;
        }
    }

    /*
     * Read the offset and adjust if we're playing catch-up.
     *
     * The catch-up adjusting work by us decrementing the offset by a percentage of
     * the time elapsed since the previous TMVirtualGetSync call.
     *
     * It's possible to get a very long or even negative interval between two read
     * for the following reasons:
     *  - Someone might have suspended the process execution, frequently the case when
     *    debugging the process.
     *  - We might be on a different CPU which TSC isn't quite in sync with the
     *    other CPUs in the system.
     *  - Another thread is racing us and we might have been preempted while inside
     *    this function.
     *
     * Assuming nano second virtual time, we can simply ignore any intervals which has
     * any of the upper 32 bits set.
     */
    AssertCompile(TMCLOCK_FREQ_VIRTUAL == 1000000000);
    int cOuterTries = 42;
    for (;; cOuterTries--)
    {
        /* Try grab the lock, things get simpler when owning the lock. */
        int rcLock = PDMCritSectTryEnter(&pVM->tm.s.VirtualSyncLock);
        if (RT_SUCCESS_NP(rcLock))
            return tmVirtualSyncGetLocked(pVM, u64, pcNsToDeadline);

        /* Re-check the ticking flag. */
        if (!ASMAtomicReadBool(&pVM->tm.s.fVirtualSyncTicking))
        {
            off = ASMAtomicReadU64(&pVM->tm.s.u64VirtualSync);
            if (   ASMAtomicReadBool(&pVM->tm.s.fVirtualSyncTicking)
                && cOuterTries > 0)
                continue;
            if (pcNsToDeadline)
                *pcNsToDeadline = 0;
            Log6(("tmVirtualSyncGetEx -> %'RU64 [stopped]\n", off));
            DBGFTRACE_U64_TAG(pVM, off, "tmVirtualSyncGetEx-stopped3");
            return off;
        }

        off = ASMAtomicReadU64(&pVM->tm.s.offVirtualSync);
        if (ASMAtomicReadBool(&pVM->tm.s.fVirtualSyncCatchUp))
        {
            /* No changes allowed, try get a consistent set of parameters. */
            uint64_t const u64Prev    = ASMAtomicReadU64(&pVM->tm.s.u64VirtualSyncCatchUpPrev);
            uint64_t const offGivenUp = ASMAtomicReadU64(&pVM->tm.s.offVirtualSyncGivenUp);
            uint32_t const u32Pct     = ASMAtomicReadU32(&pVM->tm.s.u32VirtualSyncCatchUpPercentage);
            if (    (   u64Prev    == ASMAtomicReadU64(&pVM->tm.s.u64VirtualSyncCatchUpPrev)
                     && offGivenUp == ASMAtomicReadU64(&pVM->tm.s.offVirtualSyncGivenUp)
                     && u32Pct     == ASMAtomicReadU32(&pVM->tm.s.u32VirtualSyncCatchUpPercentage)
                     && ASMAtomicReadBool(&pVM->tm.s.fVirtualSyncCatchUp))
                ||  cOuterTries <= 0)
            {
                uint64_t u64Delta = u64 - u64Prev;
                if (RT_LIKELY(!(u64Delta >> 32)))
                {
                    uint64_t u64Sub = ASMMultU64ByU32DivByU32(u64Delta, u32Pct, 100);
                    if (off > u64Sub + offGivenUp)
                    {
                        off -= u64Sub;
                        Log4(("TM: %'RU64/-%'8RU64: sub %RU32 [NoLock]\n", u64 - off, pVM->tm.s.offVirtualSync - offGivenUp, u64Sub));
                    }
                    else
                    {
                        /* we've completely caught up. */
                        STAM_PROFILE_ADV_STOP(&pVM->tm.s.StatVirtualSyncCatchup, c);
                        off = offGivenUp;
                        Log4(("TM: %'RU64/0: caught up [NoLock]\n", u64));
                    }
                }
                else
                    /* More than 4 seconds since last time (or negative), ignore it. */
                    Log(("TMVirtualGetSync: u64Delta=%RX64 (NoLock)\n", u64Delta));

                /* Check that we're still running and in catch up. */
                if (    ASMAtomicUoReadBool(&pVM->tm.s.fVirtualSyncTicking)
                    &&  ASMAtomicReadBool(&pVM->tm.s.fVirtualSyncCatchUp))
                    break;
                if (cOuterTries <= 0)
                    break; /* enough */
            }
        }
        else if (   off == ASMAtomicReadU64(&pVM->tm.s.offVirtualSync)
                 && !ASMAtomicReadBool(&pVM->tm.s.fVirtualSyncCatchUp))
            break; /* Got an consistent offset */
        else if (cOuterTries <= 0)
            break; /* enough */
    }
    if (cOuterTries <= 0)
        STAM_COUNTER_INC(&pVM->tm.s.StatVirtualSyncGetELoop);

    /*
     * Complete the calculation of the current TMCLOCK_VIRTUAL_SYNC time. The current
     * approach is to never pass the head timer. So, when we do stop the clock and
     * set the timer pending flag.
     */
    u64 -= off;
/** @todo u64VirtualSyncLast */
    uint64_t u64Expire = ASMAtomicReadU64(&pVM->tm.s.CTX_SUFF(paTimerQueues)[TMCLOCK_VIRTUAL_SYNC].u64Expire);
    if (u64 >= u64Expire)
    {
        PVMCPU pVCpuDst = &pVM->aCpus[pVM->tm.s.idTimerCpu];
        if (!VMCPU_FF_IS_SET(pVCpuDst, VMCPU_FF_TIMER))
        {
            Log5(("TMAllVirtual(%u): FF: %d -> 1 (NoLock)\n", __LINE__, VMCPU_FF_IS_PENDING(pVCpuDst, VMCPU_FF_TIMER)));
            VM_FF_SET(pVM, VM_FF_TM_VIRTUAL_SYNC); /* Hmm? */
            VMCPU_FF_SET(pVCpuDst, VMCPU_FF_TIMER);
#ifdef IN_RING3
# ifdef VBOX_WITH_REM
            REMR3NotifyTimerPending(pVM, pVCpuDst);
# endif
            VMR3NotifyCpuFFU(pVCpuDst->pUVCpu, VMNOTIFYFF_FLAGS_DONE_REM);
#endif
            STAM_COUNTER_INC(&pVM->tm.s.StatVirtualSyncGetSetFF);
            Log4(("TM: %'RU64/-%'8RU64: exp tmr=>ff [NoLock]\n", u64, pVM->tm.s.offVirtualSync - pVM->tm.s.offVirtualSyncGivenUp));
        }
        else
            Log4(("TM: %'RU64/-%'8RU64: exp tmr [NoLock]\n", u64, pVM->tm.s.offVirtualSync - pVM->tm.s.offVirtualSyncGivenUp));
        if (pcNsToDeadline)
            *pcNsToDeadline = 0;
        STAM_COUNTER_INC(&pVM->tm.s.StatVirtualSyncGetExpired);
    }
    else if (pcNsToDeadline)
    {
        uint64_t cNsToDeadline = u64Expire - u64;
        if (ASMAtomicReadBool(&pVM->tm.s.fVirtualSyncCatchUp))
            cNsToDeadline = ASMMultU64ByU32DivByU32(cNsToDeadline, 100,
                                                    ASMAtomicReadU32(&pVM->tm.s.u32VirtualSyncCatchUpPercentage) + 100);
        *pcNsToDeadline = tmVirtualVirtToNsDeadline(pVM, cNsToDeadline);
    }

    Log6(("tmVirtualSyncGetEx -> %'RU64\n", u64));
    DBGFTRACE_U64_TAG(pVM, u64, "tmVirtualSyncGetEx-nolock");
    return u64;
}


/**
 * Gets the current TMCLOCK_VIRTUAL_SYNC time.
 *
 * @returns The timestamp.
 * @param   pVM             The cross context VM structure.
 * @thread  EMT.
 * @remarks May set the timer and virtual sync FFs.
 */
VMM_INT_DECL(uint64_t) TMVirtualSyncGet(PVM pVM)
{
    return tmVirtualSyncGetEx(pVM, true /*fCheckTimers*/, NULL /*pcNsToDeadline*/);
}


/**
 * Gets the current TMCLOCK_VIRTUAL_SYNC time without checking timers running on
 * TMCLOCK_VIRTUAL.
 *
 * @returns The timestamp.
 * @param   pVM             The cross context VM structure.
 * @thread  EMT.
 * @remarks May set the timer and virtual sync FFs.
 */
VMM_INT_DECL(uint64_t) TMVirtualSyncGetNoCheck(PVM pVM)
{
    return tmVirtualSyncGetEx(pVM, false /*fCheckTimers*/, NULL /*pcNsToDeadline*/);
}


/**
 * Gets the current TMCLOCK_VIRTUAL_SYNC time.
 *
 * @returns The timestamp.
 * @param   pVM     The cross context VM structure.
 * @param   fCheckTimers    Check timers on the virtual clock or not.
 * @thread  EMT.
 * @remarks May set the timer and virtual sync FFs.
 */
VMM_INT_DECL(uint64_t) TMVirtualSyncGetEx(PVM pVM, bool fCheckTimers)
{
    return tmVirtualSyncGetEx(pVM, fCheckTimers, NULL /*pcNsToDeadline*/);
}


/**
 * Gets the current TMCLOCK_VIRTUAL_SYNC time and ticks to the next deadline
 * without checking timers running on TMCLOCK_VIRTUAL.
 *
 * @returns The timestamp.
 * @param   pVM                 The cross context VM structure.
 * @param   pcNsToDeadline      Where to return the number of nano seconds to
 *                              the next virtual sync timer deadline.
 * @thread  EMT.
 * @remarks May set the timer and virtual sync FFs.
 */
VMM_INT_DECL(uint64_t) TMVirtualSyncGetWithDeadlineNoCheck(PVM pVM, uint64_t *pcNsToDeadline)
{
    uint64_t cNsToDeadlineTmp;       /* try convince the compiler to skip the if tests. */
    uint64_t u64Now = tmVirtualSyncGetEx(pVM, false /*fCheckTimers*/, &cNsToDeadlineTmp);
    *pcNsToDeadline = cNsToDeadlineTmp;
    return u64Now;
}


/**
 * Gets the number of nano seconds to the next virtual sync deadline.
 *
 * @returns The number of TMCLOCK_VIRTUAL ticks.
 * @param   pVM                 The cross context VM structure.
 * @thread  EMT.
 * @remarks May set the timer and virtual sync FFs.
 */
VMMDECL(uint64_t) TMVirtualSyncGetNsToDeadline(PVM pVM)
{
    uint64_t cNsToDeadline;
    tmVirtualSyncGetEx(pVM, false /*fCheckTimers*/, &cNsToDeadline);
    return cNsToDeadline;
}


/**
 * Gets the current lag of the synchronous virtual clock (relative to the virtual clock).
 *
 * @return  The current lag.
 * @param   pVM     The cross context VM structure.
 */
VMM_INT_DECL(uint64_t) TMVirtualSyncGetLag(PVM pVM)
{
    return pVM->tm.s.offVirtualSync - pVM->tm.s.offVirtualSyncGivenUp;
}


/**
 * Get the current catch-up percent.
 *
 * @return  The current catch0up percent. 0 means running at the same speed as the virtual clock.
 * @param   pVM     The cross context VM structure.
 */
VMM_INT_DECL(uint32_t) TMVirtualSyncGetCatchUpPct(PVM pVM)
{
    if (pVM->tm.s.fVirtualSyncCatchUp)
        return pVM->tm.s.u32VirtualSyncCatchUpPercentage;
    return 0;
}


/**
 * Gets the current TMCLOCK_VIRTUAL frequency.
 *
 * @returns The frequency.
 * @param   pVM     The cross context VM structure.
 */
VMM_INT_DECL(uint64_t) TMVirtualGetFreq(PVM pVM)
{
    NOREF(pVM);
    return TMCLOCK_FREQ_VIRTUAL;
}


/**
 * Worker for TMR3PauseClocks.
 *
 * @returns VINF_SUCCESS or VERR_TM_VIRTUAL_TICKING_IPE (asserted).
 * @param   pVM     The cross context VM structure.
 */
int tmVirtualPauseLocked(PVM pVM)
{
    uint32_t c = ASMAtomicDecU32(&pVM->tm.s.cVirtualTicking);
    AssertMsgReturn(c < pVM->cCpus, ("%u vs %u\n", c, pVM->cCpus), VERR_TM_VIRTUAL_TICKING_IPE);
    if (c == 0)
    {
        STAM_COUNTER_INC(&pVM->tm.s.StatVirtualPause);
        pVM->tm.s.u64Virtual = tmVirtualGetRaw(pVM);
        ASMAtomicWriteBool(&pVM->tm.s.fVirtualSyncTicking, false);
    }
    return VINF_SUCCESS;
}


/**
 * Worker for TMR3ResumeClocks.
 *
 * @returns VINF_SUCCESS or VERR_TM_VIRTUAL_TICKING_IPE (asserted).
 * @param   pVM     The cross context VM structure.
 */
int tmVirtualResumeLocked(PVM pVM)
{
    uint32_t c = ASMAtomicIncU32(&pVM->tm.s.cVirtualTicking);
    AssertMsgReturn(c <= pVM->cCpus, ("%u vs %u\n", c, pVM->cCpus), VERR_TM_VIRTUAL_TICKING_IPE);
    if (c == 1)
    {
        STAM_COUNTER_INC(&pVM->tm.s.StatVirtualResume);
        pVM->tm.s.u64VirtualRawPrev         = 0;
        pVM->tm.s.u64VirtualWarpDriveStart  = tmVirtualGetRawNanoTS(pVM);
        pVM->tm.s.u64VirtualOffset          = pVM->tm.s.u64VirtualWarpDriveStart - pVM->tm.s.u64Virtual;
        ASMAtomicWriteBool(&pVM->tm.s.fVirtualSyncTicking, true);
    }
    return VINF_SUCCESS;
}


/**
 * Converts from virtual ticks to nanoseconds.
 *
 * @returns nanoseconds.
 * @param   pVM             The cross context VM structure.
 * @param   u64VirtualTicks The virtual ticks to convert.
 * @remark  There could be rounding errors here. We just do a simple integer divide
 *          without any adjustments.
 */
VMM_INT_DECL(uint64_t) TMVirtualToNano(PVM pVM, uint64_t u64VirtualTicks)
{
    NOREF(pVM);
    AssertCompile(TMCLOCK_FREQ_VIRTUAL == 1000000000);
    return u64VirtualTicks;
}


/**
 * Converts from virtual ticks to microseconds.
 *
 * @returns microseconds.
 * @param   pVM             The cross context VM structure.
 * @param   u64VirtualTicks The virtual ticks to convert.
 * @remark  There could be rounding errors here. We just do a simple integer divide
 *          without any adjustments.
 */
VMM_INT_DECL(uint64_t) TMVirtualToMicro(PVM pVM, uint64_t u64VirtualTicks)
{
    NOREF(pVM);
    AssertCompile(TMCLOCK_FREQ_VIRTUAL == 1000000000);
    return u64VirtualTicks / 1000;
}


/**
 * Converts from virtual ticks to milliseconds.
 *
 * @returns milliseconds.
 * @param   pVM             The cross context VM structure.
 * @param   u64VirtualTicks The virtual ticks to convert.
 * @remark  There could be rounding errors here. We just do a simple integer divide
 *          without any adjustments.
 */
VMM_INT_DECL(uint64_t) TMVirtualToMilli(PVM pVM, uint64_t u64VirtualTicks)
{
    NOREF(pVM);
    AssertCompile(TMCLOCK_FREQ_VIRTUAL == 1000000000);
    return u64VirtualTicks / 1000000;
}


/**
 * Converts from nanoseconds to virtual ticks.
 *
 * @returns virtual ticks.
 * @param   pVM             The cross context VM structure.
 * @param   u64NanoTS       The nanosecond value ticks to convert.
 * @remark  There could be rounding and overflow errors here.
 */
VMM_INT_DECL(uint64_t) TMVirtualFromNano(PVM pVM, uint64_t u64NanoTS)
{
    NOREF(pVM);
    AssertCompile(TMCLOCK_FREQ_VIRTUAL == 1000000000);
    return u64NanoTS;
}


/**
 * Converts from microseconds to virtual ticks.
 *
 * @returns virtual ticks.
 * @param   pVM             The cross context VM structure.
 * @param   u64MicroTS      The microsecond value ticks to convert.
 * @remark  There could be rounding and overflow errors here.
 */
VMM_INT_DECL(uint64_t) TMVirtualFromMicro(PVM pVM, uint64_t u64MicroTS)
{
    NOREF(pVM);
    AssertCompile(TMCLOCK_FREQ_VIRTUAL == 1000000000);
    return u64MicroTS * 1000;
}


/**
 * Converts from milliseconds to virtual ticks.
 *
 * @returns virtual ticks.
 * @param   pVM             The cross context VM structure.
 * @param   u64MilliTS      The millisecond value ticks to convert.
 * @remark  There could be rounding and overflow errors here.
 */
VMM_INT_DECL(uint64_t) TMVirtualFromMilli(PVM pVM, uint64_t u64MilliTS)
{
    NOREF(pVM);
    AssertCompile(TMCLOCK_FREQ_VIRTUAL == 1000000000);
    return u64MilliTS * 1000000;
}

