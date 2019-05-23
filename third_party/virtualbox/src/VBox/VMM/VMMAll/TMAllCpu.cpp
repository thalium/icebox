/* $Id: TMAllCpu.cpp $ */
/** @file
 * TM - Timeout Manager, CPU Time, All Contexts.
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
#include <iprt/asm-amd64-x86.h> /* for SUPGetCpuHzFromGIP */
#include "TMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/gim.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/sup.h>

#include <VBox/param.h>
#include <VBox/err.h>
#include <iprt/asm-math.h>
#include <iprt/assert.h>
#include <VBox/log.h>


/**
 * Gets the raw cpu tick from current virtual time.
 *
 * @param   pVM             The cross context VM structure.
 * @param   fCheckTimers    Whether to check timers.
 */
DECLINLINE(uint64_t) tmCpuTickGetRawVirtual(PVM pVM, bool fCheckTimers)
{
    uint64_t u64;
    if (fCheckTimers)
        u64 = TMVirtualSyncGet(pVM);
    else
        u64 = TMVirtualSyncGetNoCheck(pVM);
    return ASMMultU64ByU32DivByU32(u64, pVM->tm.s.cTSCTicksPerSecond, TMCLOCK_FREQ_VIRTUAL);
}


#ifdef IN_RING3
/**
 * Used by tmR3CpuTickParavirtEnable and tmR3CpuTickParavirtDisable.
 *
 * @param   pVM     The cross context VM structure.
 */
uint64_t tmR3CpuTickGetRawVirtualNoCheck(PVM pVM)
{
    return tmCpuTickGetRawVirtual(pVM, false /*fCheckTimers*/);
}
#endif


/**
 * Resumes the CPU timestamp counter ticking.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @internal
 */
int tmCpuTickResume(PVM pVM, PVMCPU pVCpu)
{
    if (!pVCpu->tm.s.fTSCTicking)
    {
        pVCpu->tm.s.fTSCTicking = true;

        /** @todo Test that pausing and resuming doesn't cause lag! (I.e. that we're
         *        unpaused before the virtual time and stopped after it. */
        if (pVM->tm.s.enmTSCMode == TMTSCMODE_REAL_TSC_OFFSET)
            pVCpu->tm.s.offTSCRawSrc = SUPReadTsc() - pVCpu->tm.s.u64TSC;
        else
            pVCpu->tm.s.offTSCRawSrc = tmCpuTickGetRawVirtual(pVM, false /* don't check for pending timers */)
                                     - pVCpu->tm.s.u64TSC;
        return VINF_SUCCESS;
    }
    AssertFailed();
    return VERR_TM_TSC_ALREADY_TICKING;
}


/**
 * Resumes the CPU timestamp counter ticking.
 *
 * @returns VINF_SUCCESS or VERR_TM_VIRTUAL_TICKING_IPE (asserted).
 * @param   pVM     The cross context VM structure.
 * @param   pVCpu   The cross context virtual CPU structure.
 */
int tmCpuTickResumeLocked(PVM pVM, PVMCPU pVCpu)
{
    if (!pVCpu->tm.s.fTSCTicking)
    {
        /* TSC must be ticking before calling tmCpuTickGetRawVirtual()! */
        pVCpu->tm.s.fTSCTicking = true;
        uint32_t c = ASMAtomicIncU32(&pVM->tm.s.cTSCsTicking);
        AssertMsgReturn(c <= pVM->cCpus, ("%u vs %u\n", c, pVM->cCpus), VERR_TM_VIRTUAL_TICKING_IPE);
        if (c == 1)
        {
            /* The first VCPU to resume. */
            uint64_t    offTSCRawSrcOld = pVCpu->tm.s.offTSCRawSrc;

            STAM_COUNTER_INC(&pVM->tm.s.StatTSCResume);

            /* When resuming, use the TSC value of the last stopped VCPU to avoid the TSC going back. */
            if (pVM->tm.s.enmTSCMode == TMTSCMODE_REAL_TSC_OFFSET)
                pVCpu->tm.s.offTSCRawSrc = SUPReadTsc() - pVM->tm.s.u64LastPausedTSC;
            else
                pVCpu->tm.s.offTSCRawSrc = tmCpuTickGetRawVirtual(pVM, false /* don't check for pending timers */)
                                         - pVM->tm.s.u64LastPausedTSC;

            /* Calculate the offset for other VCPUs to use. */
            pVM->tm.s.offTSCPause = pVCpu->tm.s.offTSCRawSrc - offTSCRawSrcOld;
        }
        else
        {
            /* All other VCPUs (if any). */
            pVCpu->tm.s.offTSCRawSrc += pVM->tm.s.offTSCPause;
        }
    }
    return VINF_SUCCESS;
}


/**
 * Pauses the CPU timestamp counter ticking.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @internal
 */
int tmCpuTickPause(PVMCPU pVCpu)
{
    if (pVCpu->tm.s.fTSCTicking)
    {
        pVCpu->tm.s.u64TSC = TMCpuTickGetNoCheck(pVCpu);
        pVCpu->tm.s.fTSCTicking = false;
        return VINF_SUCCESS;
    }
    AssertFailed();
    return VERR_TM_TSC_ALREADY_PAUSED;
}


/**
 * Pauses the CPU timestamp counter ticking.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @internal
 */
int tmCpuTickPauseLocked(PVM pVM, PVMCPU pVCpu)
{
    if (pVCpu->tm.s.fTSCTicking)
    {
        pVCpu->tm.s.u64TSC = TMCpuTickGetNoCheck(pVCpu);
        pVCpu->tm.s.fTSCTicking = false;

        uint32_t c = ASMAtomicDecU32(&pVM->tm.s.cTSCsTicking);
        AssertMsgReturn(c < pVM->cCpus, ("%u vs %u\n", c, pVM->cCpus), VERR_TM_VIRTUAL_TICKING_IPE);
        if (c == 0)
        {
            /* When the last TSC stops, remember the value. */
            STAM_COUNTER_INC(&pVM->tm.s.StatTSCPause);
            pVM->tm.s.u64LastPausedTSC = pVCpu->tm.s.u64TSC;
        }
        return VINF_SUCCESS;
    }
    AssertFailed();
    return VERR_TM_TSC_ALREADY_PAUSED;
}


#ifdef VBOX_WITH_STATISTICS
/**
 * Record why we refused to use offsetted TSC.
 *
 * Used by TMCpuTickCanUseRealTSC() and TMCpuTickGetDeadlineAndTscOffset().
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 */
DECLINLINE(void) tmCpuTickRecordOffsettedTscRefusal(PVM pVM, PVMCPU pVCpu)
{
    /* Sample the reason for refusing. */
    if (pVM->tm.s.enmTSCMode != TMTSCMODE_DYNAMIC)
       STAM_COUNTER_INC(&pVM->tm.s.StatTSCNotFixed);
    else if (!pVCpu->tm.s.fTSCTicking)
       STAM_COUNTER_INC(&pVM->tm.s.StatTSCNotTicking);
    else if (pVM->tm.s.enmTSCMode != TMTSCMODE_REAL_TSC_OFFSET)
    {
        if (pVM->tm.s.fVirtualSyncCatchUp)
        {
           if (pVM->tm.s.u32VirtualSyncCatchUpPercentage <= 10)
               STAM_COUNTER_INC(&pVM->tm.s.StatTSCCatchupLE010);
           else if (pVM->tm.s.u32VirtualSyncCatchUpPercentage <= 25)
               STAM_COUNTER_INC(&pVM->tm.s.StatTSCCatchupLE025);
           else if (pVM->tm.s.u32VirtualSyncCatchUpPercentage <= 100)
               STAM_COUNTER_INC(&pVM->tm.s.StatTSCCatchupLE100);
           else
               STAM_COUNTER_INC(&pVM->tm.s.StatTSCCatchupOther);
        }
        else if (!pVM->tm.s.fVirtualSyncTicking)
           STAM_COUNTER_INC(&pVM->tm.s.StatTSCSyncNotTicking);
        else if (pVM->tm.s.fVirtualWarpDrive)
           STAM_COUNTER_INC(&pVM->tm.s.StatTSCWarp);
    }
}
#endif /* VBOX_WITH_STATISTICS */


/**
 * Checks if AMD-V / VT-x can use an offsetted hardware TSC or not.
 *
 * @returns true/false accordingly.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   poffRealTsc     The offset against the TSC of the current host CPU,
 *                          if pfOffsettedTsc is set to true.
 * @param   pfParavirtTsc   Where to return whether paravirt TSC is enabled.
 *
 * @thread  EMT(pVCpu).
 * @see     TMCpuTickGetDeadlineAndTscOffset().
 */
VMM_INT_DECL(bool) TMCpuTickCanUseRealTSC(PVM pVM, PVMCPU pVCpu, uint64_t *poffRealTsc, bool *pfParavirtTsc)
{
    Assert(pVCpu->tm.s.fTSCTicking || DBGFIsStepping(pVCpu));

    *pfParavirtTsc = pVM->tm.s.fParavirtTscEnabled;

    /*
     * In real TSC mode it's easy, we just need the delta & offTscRawSrc and
     * the CPU will add them to RDTSC and RDTSCP at runtime.
     *
     * In tmCpuTickGetInternal we do:
     *          SUPReadTsc() - pVCpu->tm.s.offTSCRawSrc;
     * Where SUPReadTsc() does:
     *          ASMReadTSC() - pGipCpu->i64TscDelta;
     * Which means tmCpuTickGetInternal actually does:
     *          ASMReadTSC() - pGipCpu->i64TscDelta - pVCpu->tm.s.offTSCRawSrc;
     * So, the offset to be ADDED to RDTSC[P] is:
     *          offRealTsc = -(pGipCpu->i64TscDelta + pVCpu->tm.s.offTSCRawSrc)
     */
    if (pVM->tm.s.enmTSCMode == TMTSCMODE_REAL_TSC_OFFSET)
    {
        /** @todo We should negate both deltas!  It's soo weird that we do the
         *        exact opposite of what the hardware implements. */
#ifdef IN_RING3
        *poffRealTsc = 0 - pVCpu->tm.s.offTSCRawSrc - SUPGetTscDelta();
#else
        *poffRealTsc = 0 - pVCpu->tm.s.offTSCRawSrc - SUPGetTscDeltaByCpuSetIndex(pVCpu->iHostCpuSet);
#endif
        return true;
    }

    /*
     * We require:
     *     1. A fixed TSC, this is checked at init time.
     *     2. That the TSC is ticking (we shouldn't be here if it isn't)
     *     3. Either that we're using the real TSC as time source or
     *          a) we don't have any lag to catch up, and
     *          b) the virtual sync clock hasn't been halted by an expired timer, and
     *          c) we're not using warp drive (accelerated virtual guest time).
     */
    if (   pVM->tm.s.enmTSCMode == TMTSCMODE_DYNAMIC
        && !pVM->tm.s.fVirtualSyncCatchUp
        && RT_LIKELY(pVM->tm.s.fVirtualSyncTicking)
        && !pVM->tm.s.fVirtualWarpDrive)
    {
        /* The source is the timer synchronous virtual clock. */
        uint64_t u64Now = tmCpuTickGetRawVirtual(pVM, false /* don't check for pending timers */)
                        - pVCpu->tm.s.offTSCRawSrc;
        /** @todo When we start collecting statistics on how much time we spend executing
         * guest code before exiting, we should check this against the next virtual sync
         * timer timeout. If it's lower than the avg. length, we should trap rdtsc to increase
         * the chance that we'll get interrupted right after the timer expired. */
        if (u64Now >= pVCpu->tm.s.u64TSCLastSeen)
        {
            *poffRealTsc = u64Now - ASMReadTSC();
            return true;    /** @todo count this? */
        }
    }

#ifdef VBOX_WITH_STATISTICS
    tmCpuTickRecordOffsettedTscRefusal(pVM, pVCpu);
#endif
    return false;
}


/**
 * Calculates the number of host CPU ticks till the next virtual sync deadline.
 *
 * @note    To save work, this function will not bother calculating the accurate
 *          tick count for deadlines that are more than a second ahead.
 *
 * @returns The number of host cpu ticks to the next deadline.  Max one second.
 * @param   pVCpu           The cross context virtual CPU structure of the calling EMT.
 * @param   cNsToDeadline   The number of nano seconds to the next virtual
 *                          sync deadline.
 */
DECLINLINE(uint64_t) tmCpuCalcTicksToDeadline(PVMCPU pVCpu, uint64_t cNsToDeadline)
{
    AssertCompile(TMCLOCK_FREQ_VIRTUAL <= _4G);
#ifdef IN_RING3
    RT_NOREF_PV(pVCpu);
    uint64_t uCpuHz = SUPGetCpuHzFromGip(g_pSUPGlobalInfoPage);
#else
    uint64_t uCpuHz = SUPGetCpuHzFromGipBySetIndex(g_pSUPGlobalInfoPage, pVCpu->iHostCpuSet);
#endif
    if (RT_UNLIKELY(cNsToDeadline >= TMCLOCK_FREQ_VIRTUAL))
        return uCpuHz;
    uint64_t cTicks = ASMMultU64ByU32DivByU32(uCpuHz, cNsToDeadline, TMCLOCK_FREQ_VIRTUAL);
    if (cTicks > 4000)
        cTicks -= 4000; /* fudge to account for overhead */
    else
        cTicks >>= 1;
    return cTicks;
}


/**
 * Gets the next deadline in host CPU clock ticks and the TSC offset if we can
 * use the raw TSC.
 *
 * @returns The number of host CPU clock ticks to the next timer deadline.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure of the calling EMT.
 * @param   poffRealTsc     The offset against the TSC of the current host CPU,
 *                          if pfOffsettedTsc is set to true.
 * @param   pfOffsettedTsc  Where to return whether TSC offsetting can be used.
 * @param   pfParavirtTsc   Where to return whether paravirt TSC is enabled.
 *
 * @thread  EMT(pVCpu).
 * @see     TMCpuTickCanUseRealTSC().
 */
VMM_INT_DECL(uint64_t) TMCpuTickGetDeadlineAndTscOffset(PVM pVM, PVMCPU pVCpu, uint64_t *poffRealTsc,
                                                        bool *pfOffsettedTsc, bool *pfParavirtTsc)
{
    Assert(pVCpu->tm.s.fTSCTicking || DBGFIsStepping(pVCpu));

    *pfParavirtTsc = pVM->tm.s.fParavirtTscEnabled;

    /*
     * Same logic as in TMCpuTickCanUseRealTSC.
     */
    if (pVM->tm.s.enmTSCMode == TMTSCMODE_REAL_TSC_OFFSET)
    {
        /** @todo We should negate both deltas!  It's soo weird that we do the
         *        exact opposite of what the hardware implements. */
#ifdef IN_RING3
        *poffRealTsc     = 0 - pVCpu->tm.s.offTSCRawSrc - SUPGetTscDelta();
#else
        *poffRealTsc     = 0 - pVCpu->tm.s.offTSCRawSrc - SUPGetTscDeltaByCpuSetIndex(pVCpu->iHostCpuSet);
#endif
        *pfOffsettedTsc  = true;
        return tmCpuCalcTicksToDeadline(pVCpu, TMVirtualSyncGetNsToDeadline(pVM));
    }

    /*
     * Same logic as in TMCpuTickCanUseRealTSC.
     */
    if (   pVM->tm.s.enmTSCMode == TMTSCMODE_DYNAMIC
        && !pVM->tm.s.fVirtualSyncCatchUp
        && RT_LIKELY(pVM->tm.s.fVirtualSyncTicking)
        && !pVM->tm.s.fVirtualWarpDrive)
    {
        /* The source is the timer synchronous virtual clock. */
        uint64_t cNsToDeadline;
        uint64_t u64NowVirtSync = TMVirtualSyncGetWithDeadlineNoCheck(pVM, &cNsToDeadline);
        uint64_t u64Now = ASMMultU64ByU32DivByU32(u64NowVirtSync, pVM->tm.s.cTSCTicksPerSecond, TMCLOCK_FREQ_VIRTUAL);
        u64Now -= pVCpu->tm.s.offTSCRawSrc;
        *poffRealTsc     = u64Now - ASMReadTSC();
        *pfOffsettedTsc  = u64Now >= pVCpu->tm.s.u64TSCLastSeen;
        return tmCpuCalcTicksToDeadline(pVCpu, cNsToDeadline);
    }

#ifdef VBOX_WITH_STATISTICS
    tmCpuTickRecordOffsettedTscRefusal(pVM, pVCpu);
#endif
    *pfOffsettedTsc  = false;
    *poffRealTsc     = 0;
    return tmCpuCalcTicksToDeadline(pVCpu, TMVirtualSyncGetNsToDeadline(pVM));
}


/**
 * Read the current CPU timestamp counter.
 *
 * @returns Gets the CPU tsc.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   fCheckTimers    Whether to check timers.
 */
DECLINLINE(uint64_t) tmCpuTickGetInternal(PVMCPU pVCpu, bool fCheckTimers)
{
    uint64_t u64;

    if (RT_LIKELY(pVCpu->tm.s.fTSCTicking))
    {
        PVM pVM = pVCpu->CTX_SUFF(pVM);
        if (pVM->tm.s.enmTSCMode == TMTSCMODE_REAL_TSC_OFFSET)
            u64 = SUPReadTsc();
        else
            u64 = tmCpuTickGetRawVirtual(pVM, fCheckTimers);
        u64 -= pVCpu->tm.s.offTSCRawSrc;

        /* Always return a value higher than what the guest has already seen. */
        if (RT_LIKELY(u64 > pVCpu->tm.s.u64TSCLastSeen))
            pVCpu->tm.s.u64TSCLastSeen = u64;
        else
        {
            STAM_COUNTER_INC(&pVM->tm.s.StatTSCUnderflow);
            pVCpu->tm.s.u64TSCLastSeen += 64;   /** @todo choose a good increment here */
            u64 = pVCpu->tm.s.u64TSCLastSeen;
        }
    }
    else
        u64 = pVCpu->tm.s.u64TSC;
    /** @todo @bugref{7243}: SVM TSC offset. */
    return u64;
}


/**
 * Read the current CPU timestamp counter.
 *
 * @returns Gets the CPU tsc.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(uint64_t) TMCpuTickGet(PVMCPU pVCpu)
{
    return tmCpuTickGetInternal(pVCpu, true /* fCheckTimers */);
}


/**
 * Read the current CPU timestamp counter, don't check for expired timers.
 *
 * @returns Gets the CPU tsc.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMM_INT_DECL(uint64_t) TMCpuTickGetNoCheck(PVMCPU pVCpu)
{
    return tmCpuTickGetInternal(pVCpu, false /* fCheckTimers */);
}


/**
 * Sets the current CPU timestamp counter.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   u64Tick     The new timestamp value.
 *
 * @thread  EMT which TSC is to be set.
 */
VMM_INT_DECL(int) TMCpuTickSet(PVM pVM, PVMCPU pVCpu, uint64_t u64Tick)
{
    VMCPU_ASSERT_EMT(pVCpu);
    STAM_COUNTER_INC(&pVM->tm.s.StatTSCSet);

    /*
     * This is easier to do when the TSC is paused since resume will
     * do all the calculations for us. Actually, we don't need to
     * call tmCpuTickPause here since we overwrite u64TSC anyway.
     */
    bool        fTSCTicking    = pVCpu->tm.s.fTSCTicking;
    pVCpu->tm.s.fTSCTicking    = false;
    pVCpu->tm.s.u64TSC         = u64Tick;
    pVCpu->tm.s.u64TSCLastSeen = u64Tick;
    if (fTSCTicking)
        tmCpuTickResume(pVM, pVCpu);
    /** @todo Try help synchronizing it better among the virtual CPUs? */

    return VINF_SUCCESS;
}

/**
 * Sets the last seen CPU timestamp counter.
 *
 * @returns VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   u64LastSeenTick     The last seen timestamp value.
 *
 * @thread  EMT which TSC is to be set.
 */
VMM_INT_DECL(int) TMCpuTickSetLastSeen(PVMCPU pVCpu, uint64_t u64LastSeenTick)
{
    VMCPU_ASSERT_EMT(pVCpu);

    LogFlow(("TMCpuTickSetLastSeen %RX64\n", u64LastSeenTick));
    if (pVCpu->tm.s.u64TSCLastSeen < u64LastSeenTick)
        pVCpu->tm.s.u64TSCLastSeen = u64LastSeenTick;
    return VINF_SUCCESS;
}

/**
 * Gets the last seen CPU timestamp counter of the guest.
 *
 * @returns the last seen TSC.
 * @param   pVCpu               The cross context virtual CPU structure.
 *
 * @thread  EMT(pVCpu).
 */
VMM_INT_DECL(uint64_t) TMCpuTickGetLastSeen(PVMCPU pVCpu)
{
    VMCPU_ASSERT_EMT(pVCpu);

    return pVCpu->tm.s.u64TSCLastSeen;
}


/**
 * Get the timestamp frequency.
 *
 * @returns Number of ticks per second.
 * @param   pVM     The cross context VM structure.
 */
VMMDECL(uint64_t) TMCpuTicksPerSecond(PVM pVM)
{
    if (   pVM->tm.s.enmTSCMode == TMTSCMODE_REAL_TSC_OFFSET
        && g_pSUPGlobalInfoPage->u32Mode != SUPGIPMODE_INVARIANT_TSC)
    {
#ifdef IN_RING3
        uint64_t cTSCTicksPerSecond = SUPGetCpuHzFromGip(g_pSUPGlobalInfoPage);
#elif defined(IN_RING0)
        uint64_t cTSCTicksPerSecond = SUPGetCpuHzFromGipBySetIndex(g_pSUPGlobalInfoPage, RTMpCpuIdToSetIndex(RTMpCpuId()));
#else
        uint64_t cTSCTicksPerSecond = SUPGetCpuHzFromGipBySetIndex(g_pSUPGlobalInfoPage, VMMGetCpu(pVM)->iHostCpuSet);
#endif
        if (RT_LIKELY(cTSCTicksPerSecond != ~(uint64_t)0))
            return cTSCTicksPerSecond;
    }
    return pVM->tm.s.cTSCTicksPerSecond;
}


/**
 * Whether the TSC is ticking for the VCPU.
 *
 * @returns true if ticking, false otherwise.
 * @param   pVCpu           The cross context virtual CPU structure.
 */
VMM_INT_DECL(bool) TMCpuTickIsTicking(PVMCPU pVCpu)
{
    return pVCpu->tm.s.fTSCTicking;
}

