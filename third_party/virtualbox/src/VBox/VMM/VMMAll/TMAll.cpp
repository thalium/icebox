/* $Id: TMAll.cpp $ */
/** @file
 * TM - Timeout Manager, all contexts.
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
#include <VBox/vmm/mm.h>
#include <VBox/vmm/dbgftrace.h>
#ifdef IN_RING3
# ifdef VBOX_WITH_REM
#  include <VBox/vmm/rem.h>
# endif
#endif
#include "TMInternal.h"
#include <VBox/vmm/vm.h>

#include <VBox/param.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <VBox/sup.h>
#include <iprt/time.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/asm-math.h>
#ifdef IN_RING3
# include <iprt/thread.h>
#endif

#include "TMInline.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** @def TMTIMER_ASSERT_CRITSECT
 * Checks that the caller owns the critical section if one is associated with
 * the timer. */
#ifdef VBOX_STRICT
# define TMTIMER_ASSERT_CRITSECT(pTimer) \
    do { \
        if ((pTimer)->pCritSect) \
        { \
            VMSTATE      enmState; \
            PPDMCRITSECT pCritSect = (PPDMCRITSECT)MMHyperR3ToCC((pTimer)->CTX_SUFF(pVM), (pTimer)->pCritSect); \
            AssertMsg(   pCritSect \
                      && (   PDMCritSectIsOwner(pCritSect) \
                          || (enmState = (pTimer)->CTX_SUFF(pVM)->enmVMState) == VMSTATE_CREATING \
                          || enmState == VMSTATE_RESETTING \
                          || enmState == VMSTATE_RESETTING_LS ),\
                      ("pTimer=%p (%s) pCritSect=%p (%s)\n", pTimer, R3STRING(pTimer->pszDesc), \
                       (pTimer)->pCritSect, R3STRING(PDMR3CritSectName((pTimer)->pCritSect)) )); \
        } \
    } while (0)
#else
# define TMTIMER_ASSERT_CRITSECT(pTimer) do { } while (0)
#endif

/** @def TMTIMER_ASSERT_SYNC_CRITSECT_ORDER
 * Checks for lock order trouble between the timer critsect and the critical
 * section critsect.  The virtual sync critsect must always be entered before
 * the one associated with the timer (see TMR3TimerQueuesDo).  It is OK if there
 * isn't any critical section associated with the timer or if the calling thread
 * doesn't own it, ASSUMING of course that the thread using this macro is going
 * to enter the virtual sync critical section anyway.
 *
 * @remarks This is a sligtly relaxed timer locking attitude compared to
 *          TMTIMER_ASSERT_CRITSECT, however, the calling device/whatever code
 *          should know what it's doing if it's stopping or starting a timer
 *          without taking the device lock.
 */
#ifdef VBOX_STRICT
# define TMTIMER_ASSERT_SYNC_CRITSECT_ORDER(pVM, pTimer) \
    do { \
        if ((pTimer)->pCritSect) \
        { \
            VMSTATE      enmState; \
            PPDMCRITSECT pCritSect = (PPDMCRITSECT)MMHyperR3ToCC(pVM, (pTimer)->pCritSect); \
            AssertMsg(   pCritSect \
                      && (   !PDMCritSectIsOwner(pCritSect) \
                          || PDMCritSectIsOwner(&pVM->tm.s.VirtualSyncLock) \
                          || (enmState = (pVM)->enmVMState) == VMSTATE_CREATING \
                          || enmState == VMSTATE_RESETTING \
                          || enmState == VMSTATE_RESETTING_LS ),\
                      ("pTimer=%p (%s) pCritSect=%p (%s)\n", pTimer, R3STRING(pTimer->pszDesc), \
                       (pTimer)->pCritSect, R3STRING(PDMR3CritSectName((pTimer)->pCritSect)) )); \
        } \
    } while (0)
#else
# define TMTIMER_ASSERT_SYNC_CRITSECT_ORDER(pVM, pTimer) do { } while (0)
#endif


/**
 * Notification that execution is about to start.
 *
 * This call must always be paired with a TMNotifyEndOfExecution call.
 *
 * The function may, depending on the configuration, resume the TSC and future
 * clocks that only ticks when we're executing guest code.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(void) TMNotifyStartOfExecution(PVMCPU pVCpu)
{
    PVM pVM = pVCpu->CTX_SUFF(pVM);

#ifndef VBOX_WITHOUT_NS_ACCOUNTING
    pVCpu->tm.s.u64NsTsStartExecuting = RTTimeNanoTS();
#endif
    if (pVM->tm.s.fTSCTiedToExecution)
        tmCpuTickResume(pVM, pVCpu);
}


/**
 * Notification that execution has ended.
 *
 * This call must always be paired with a TMNotifyStartOfExecution call.
 *
 * The function may, depending on the configuration, suspend the TSC and future
 * clocks that only ticks when we're executing guest code.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(void) TMNotifyEndOfExecution(PVMCPU pVCpu)
{
    PVM pVM = pVCpu->CTX_SUFF(pVM);

    if (pVM->tm.s.fTSCTiedToExecution)
        tmCpuTickPause(pVCpu);

#ifndef VBOX_WITHOUT_NS_ACCOUNTING
    uint64_t const u64NsTs           = RTTimeNanoTS();
    uint64_t const cNsTotalNew       = u64NsTs - pVCpu->tm.s.u64NsTsStartTotal;
    uint64_t const cNsExecutingDelta = u64NsTs - pVCpu->tm.s.u64NsTsStartExecuting;
    uint64_t const cNsExecutingNew   = pVCpu->tm.s.cNsExecuting + cNsExecutingDelta;
    uint64_t const cNsOtherNew       = cNsTotalNew - cNsExecutingNew - pVCpu->tm.s.cNsHalted;

# if defined(VBOX_WITH_STATISTICS) || defined(VBOX_WITH_NS_ACCOUNTING_STATS)
    STAM_REL_PROFILE_ADD_PERIOD(&pVCpu->tm.s.StatNsExecuting, cNsExecutingDelta);
    if (cNsExecutingDelta < 5000)
        STAM_REL_PROFILE_ADD_PERIOD(&pVCpu->tm.s.StatNsExecTiny, cNsExecutingDelta);
    else if (cNsExecutingDelta < 50000)
        STAM_REL_PROFILE_ADD_PERIOD(&pVCpu->tm.s.StatNsExecShort, cNsExecutingDelta);
    else
        STAM_REL_PROFILE_ADD_PERIOD(&pVCpu->tm.s.StatNsExecLong, cNsExecutingDelta);
    STAM_REL_COUNTER_ADD(&pVCpu->tm.s.StatNsTotal, cNsTotalNew - pVCpu->tm.s.cNsTotal);
    int64_t  const cNsOtherNewDelta  = cNsOtherNew - pVCpu->tm.s.cNsOther;
    if (cNsOtherNewDelta > 0)
        STAM_REL_PROFILE_ADD_PERIOD(&pVCpu->tm.s.StatNsOther, cNsOtherNewDelta); /* (the period before execution) */
# endif

    uint32_t uGen = ASMAtomicIncU32(&pVCpu->tm.s.uTimesGen); Assert(uGen & 1);
    pVCpu->tm.s.cNsExecuting = cNsExecutingNew;
    pVCpu->tm.s.cNsTotal     = cNsTotalNew;
    pVCpu->tm.s.cNsOther     = cNsOtherNew;
    pVCpu->tm.s.cPeriodsExecuting++;
    ASMAtomicWriteU32(&pVCpu->tm.s.uTimesGen, (uGen | 1) + 1);
#endif
}


/**
 * Notification that the cpu is entering the halt state
 *
 * This call must always be paired with a TMNotifyEndOfExecution call.
 *
 * The function may, depending on the configuration, resume the TSC and future
 * clocks that only ticks when we're halted.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMM_INT_DECL(void) TMNotifyStartOfHalt(PVMCPU pVCpu)
{
    PVM pVM = pVCpu->CTX_SUFF(pVM);

#ifndef VBOX_WITHOUT_NS_ACCOUNTING
    pVCpu->tm.s.u64NsTsStartHalting = RTTimeNanoTS();
#endif

    if (    pVM->tm.s.fTSCTiedToExecution
        &&  !pVM->tm.s.fTSCNotTiedToHalt)
        tmCpuTickResume(pVM, pVCpu);
}


/**
 * Notification that the cpu is leaving the halt state
 *
 * This call must always be paired with a TMNotifyStartOfHalt call.
 *
 * The function may, depending on the configuration, suspend the TSC and future
 * clocks that only ticks when we're halted.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMM_INT_DECL(void) TMNotifyEndOfHalt(PVMCPU pVCpu)
{
    PVM pVM = pVCpu->CTX_SUFF(pVM);

    if (    pVM->tm.s.fTSCTiedToExecution
        &&  !pVM->tm.s.fTSCNotTiedToHalt)
        tmCpuTickPause(pVCpu);

#ifndef VBOX_WITHOUT_NS_ACCOUNTING
    uint64_t const u64NsTs        = RTTimeNanoTS();
    uint64_t const cNsTotalNew    = u64NsTs - pVCpu->tm.s.u64NsTsStartTotal;
    uint64_t const cNsHaltedDelta = u64NsTs - pVCpu->tm.s.u64NsTsStartHalting;
    uint64_t const cNsHaltedNew   = pVCpu->tm.s.cNsHalted + cNsHaltedDelta;
    uint64_t const cNsOtherNew    = cNsTotalNew - pVCpu->tm.s.cNsExecuting - cNsHaltedNew;

# if defined(VBOX_WITH_STATISTICS) || defined(VBOX_WITH_NS_ACCOUNTING_STATS)
    STAM_REL_PROFILE_ADD_PERIOD(&pVCpu->tm.s.StatNsHalted, cNsHaltedDelta);
    STAM_REL_COUNTER_ADD(&pVCpu->tm.s.StatNsTotal, cNsTotalNew - pVCpu->tm.s.cNsTotal);
    int64_t  const cNsOtherNewDelta  = cNsOtherNew - pVCpu->tm.s.cNsOther;
    if (cNsOtherNewDelta > 0)
        STAM_REL_PROFILE_ADD_PERIOD(&pVCpu->tm.s.StatNsOther, cNsOtherNewDelta); /* (the period before halting) */
# endif

    uint32_t uGen = ASMAtomicIncU32(&pVCpu->tm.s.uTimesGen); Assert(uGen & 1);
    pVCpu->tm.s.cNsHalted = cNsHaltedNew;
    pVCpu->tm.s.cNsTotal  = cNsTotalNew;
    pVCpu->tm.s.cNsOther  = cNsOtherNew;
    pVCpu->tm.s.cPeriodsHalted++;
    ASMAtomicWriteU32(&pVCpu->tm.s.uTimesGen, (uGen | 1) + 1);
#endif
}


/**
 * Raise the timer force action flag and notify the dedicated timer EMT.
 *
 * @param   pVM         The cross context VM structure.
 */
DECLINLINE(void) tmScheduleNotify(PVM pVM)
{
    PVMCPU pVCpuDst = &pVM->aCpus[pVM->tm.s.idTimerCpu];
    if (!VMCPU_FF_IS_SET(pVCpuDst, VMCPU_FF_TIMER))
    {
        Log5(("TMAll(%u): FF: 0 -> 1\n", __LINE__));
        VMCPU_FF_SET(pVCpuDst, VMCPU_FF_TIMER);
#ifdef IN_RING3
# ifdef VBOX_WITH_REM
        REMR3NotifyTimerPending(pVM, pVCpuDst);
# endif
        VMR3NotifyCpuFFU(pVCpuDst->pUVCpu, VMNOTIFYFF_FLAGS_DONE_REM);
#endif
        STAM_COUNTER_INC(&pVM->tm.s.StatScheduleSetFF);
    }
}


/**
 * Schedule the queue which was changed.
 */
DECLINLINE(void) tmSchedule(PTMTIMER pTimer)
{
    PVM pVM = pTimer->CTX_SUFF(pVM);
    if (    VM_IS_EMT(pVM)
        &&  RT_SUCCESS(TM_TRY_LOCK_TIMERS(pVM)))
    {
        STAM_PROFILE_START(&pVM->tm.s.CTX_SUFF_Z(StatScheduleOne), a);
        Log3(("tmSchedule: tmTimerQueueSchedule\n"));
        tmTimerQueueSchedule(pVM, &pVM->tm.s.CTX_SUFF(paTimerQueues)[pTimer->enmClock]);
#ifdef VBOX_STRICT
        tmTimerQueuesSanityChecks(pVM, "tmSchedule");
#endif
        STAM_PROFILE_STOP(&pVM->tm.s.CTX_SUFF_Z(StatScheduleOne), a);
        TM_UNLOCK_TIMERS(pVM);
    }
    else
    {
        TMTIMERSTATE enmState = pTimer->enmState;
        if (TMTIMERSTATE_IS_PENDING_SCHEDULING(enmState))
            tmScheduleNotify(pVM);
    }
}


/**
 * Try change the state to enmStateNew from enmStateOld
 * and link the timer into the scheduling queue.
 *
 * @returns Success indicator.
 * @param   pTimer          Timer in question.
 * @param   enmStateNew     The new timer state.
 * @param   enmStateOld     The old timer state.
 */
DECLINLINE(bool) tmTimerTry(PTMTIMER pTimer, TMTIMERSTATE enmStateNew, TMTIMERSTATE enmStateOld)
{
    /*
     * Attempt state change.
     */
    bool fRc;
    TM_TRY_SET_STATE(pTimer, enmStateNew, enmStateOld, fRc);
    return fRc;
}


/**
 * Links the timer onto the scheduling queue.
 *
 * @param   pQueue  The timer queue the timer belongs to.
 * @param   pTimer  The timer.
 *
 * @todo    FIXME: Look into potential race with the thread running the queues
 *          and stuff.
 */
DECLINLINE(void) tmTimerLinkSchedule(PTMTIMERQUEUE pQueue, PTMTIMER pTimer)
{
    Assert(!pTimer->offScheduleNext);
    const int32_t offHeadNew = (intptr_t)pTimer - (intptr_t)pQueue;
    int32_t offHead;
    do
    {
        offHead = pQueue->offSchedule;
        if (offHead)
            pTimer->offScheduleNext = ((intptr_t)pQueue + offHead) - (intptr_t)pTimer;
        else
            pTimer->offScheduleNext = 0;
    } while (!ASMAtomicCmpXchgS32(&pQueue->offSchedule, offHeadNew, offHead));
}


/**
 * Try change the state to enmStateNew from enmStateOld
 * and link the timer into the scheduling queue.
 *
 * @returns Success indicator.
 * @param   pTimer          Timer in question.
 * @param   enmStateNew     The new timer state.
 * @param   enmStateOld     The old timer state.
 */
DECLINLINE(bool) tmTimerTryWithLink(PTMTIMER pTimer, TMTIMERSTATE enmStateNew, TMTIMERSTATE enmStateOld)
{
    if (tmTimerTry(pTimer, enmStateNew, enmStateOld))
    {
        tmTimerLinkSchedule(&pTimer->CTX_SUFF(pVM)->tm.s.CTX_SUFF(paTimerQueues)[pTimer->enmClock], pTimer);
        return true;
    }
    return false;
}


/**
 * Links a timer into the active list of a timer queue.
 *
 * @param   pQueue          The queue.
 * @param   pTimer          The timer.
 * @param   u64Expire       The timer expiration time.
 *
 * @remarks Called while owning the relevant queue lock.
 */
DECL_FORCE_INLINE(void) tmTimerQueueLinkActive(PTMTIMERQUEUE pQueue, PTMTIMER pTimer, uint64_t u64Expire)
{
    Assert(!pTimer->offNext);
    Assert(!pTimer->offPrev);
    Assert(pTimer->enmState == TMTIMERSTATE_ACTIVE || pTimer->enmClock != TMCLOCK_VIRTUAL_SYNC); /* (active is not a stable state) */

    PTMTIMER pCur = TMTIMER_GET_HEAD(pQueue);
    if (pCur)
    {
        for (;; pCur = TMTIMER_GET_NEXT(pCur))
        {
            if (pCur->u64Expire > u64Expire)
            {
                const PTMTIMER pPrev = TMTIMER_GET_PREV(pCur);
                TMTIMER_SET_NEXT(pTimer, pCur);
                TMTIMER_SET_PREV(pTimer, pPrev);
                if (pPrev)
                    TMTIMER_SET_NEXT(pPrev, pTimer);
                else
                {
                    TMTIMER_SET_HEAD(pQueue, pTimer);
                    ASMAtomicWriteU64(&pQueue->u64Expire, u64Expire);
                    DBGFTRACE_U64_TAG2(pTimer->CTX_SUFF(pVM), u64Expire, "tmTimerQueueLinkActive head", R3STRING(pTimer->pszDesc));
                }
                TMTIMER_SET_PREV(pCur, pTimer);
                return;
            }
            if (!pCur->offNext)
            {
                TMTIMER_SET_NEXT(pCur, pTimer);
                TMTIMER_SET_PREV(pTimer, pCur);
                DBGFTRACE_U64_TAG2(pTimer->CTX_SUFF(pVM), u64Expire, "tmTimerQueueLinkActive tail", R3STRING(pTimer->pszDesc));
                return;
            }
        }
    }
    else
    {
        TMTIMER_SET_HEAD(pQueue, pTimer);
        ASMAtomicWriteU64(&pQueue->u64Expire, u64Expire);
        DBGFTRACE_U64_TAG2(pTimer->CTX_SUFF(pVM), u64Expire, "tmTimerQueueLinkActive empty", R3STRING(pTimer->pszDesc));
    }
}



/**
 * Schedules the given timer on the given queue.
 *
 * @param   pQueue      The timer queue.
 * @param   pTimer      The timer that needs scheduling.
 *
 * @remarks Called while owning the lock.
 */
DECLINLINE(void) tmTimerQueueScheduleOne(PTMTIMERQUEUE pQueue, PTMTIMER pTimer)
{
    Assert(pQueue->enmClock != TMCLOCK_VIRTUAL_SYNC);

    /*
     * Processing.
     */
    unsigned cRetries = 2;
    do
    {
        TMTIMERSTATE enmState = pTimer->enmState;
        switch (enmState)
        {
            /*
             * Reschedule timer (in the active list).
             */
            case TMTIMERSTATE_PENDING_RESCHEDULE:
                if (RT_UNLIKELY(!tmTimerTry(pTimer, TMTIMERSTATE_PENDING_SCHEDULE, TMTIMERSTATE_PENDING_RESCHEDULE)))
                    break; /* retry */
                tmTimerQueueUnlinkActive(pQueue, pTimer);
                RT_FALL_THRU();

            /*
             * Schedule timer (insert into the active list).
             */
            case TMTIMERSTATE_PENDING_SCHEDULE:
                Assert(!pTimer->offNext); Assert(!pTimer->offPrev);
                if (RT_UNLIKELY(!tmTimerTry(pTimer, TMTIMERSTATE_ACTIVE, TMTIMERSTATE_PENDING_SCHEDULE)))
                    break; /* retry */
                tmTimerQueueLinkActive(pQueue, pTimer, pTimer->u64Expire);
                return;

            /*
             * Stop the timer in active list.
             */
            case TMTIMERSTATE_PENDING_STOP:
                if (RT_UNLIKELY(!tmTimerTry(pTimer, TMTIMERSTATE_PENDING_STOP_SCHEDULE, TMTIMERSTATE_PENDING_STOP)))
                    break; /* retry */
                tmTimerQueueUnlinkActive(pQueue, pTimer);
                RT_FALL_THRU();

            /*
             * Stop the timer (not on the active list).
             */
            case TMTIMERSTATE_PENDING_STOP_SCHEDULE:
                Assert(!pTimer->offNext); Assert(!pTimer->offPrev);
                if (RT_UNLIKELY(!tmTimerTry(pTimer, TMTIMERSTATE_STOPPED, TMTIMERSTATE_PENDING_STOP_SCHEDULE)))
                    break;
                return;

            /*
             * The timer is pending destruction by TMR3TimerDestroy, our caller.
             * Nothing to do here.
             */
            case TMTIMERSTATE_DESTROY:
                break;

            /*
             * Postpone these until they get into the right state.
             */
            case TMTIMERSTATE_PENDING_RESCHEDULE_SET_EXPIRE:
            case TMTIMERSTATE_PENDING_SCHEDULE_SET_EXPIRE:
                tmTimerLinkSchedule(pQueue, pTimer);
                STAM_COUNTER_INC(&pTimer->CTX_SUFF(pVM)->tm.s.CTX_SUFF_Z(StatPostponed));
                return;

            /*
             * None of these can be in the schedule.
             */
            case TMTIMERSTATE_FREE:
            case TMTIMERSTATE_STOPPED:
            case TMTIMERSTATE_ACTIVE:
            case TMTIMERSTATE_EXPIRED_GET_UNLINK:
            case TMTIMERSTATE_EXPIRED_DELIVER:
            default:
                AssertMsgFailed(("Timer (%p) in the scheduling list has an invalid state %s (%d)!",
                                 pTimer, tmTimerState(pTimer->enmState), pTimer->enmState));
                return;
        }
    } while (cRetries-- > 0);
}


/**
 * Schedules the specified timer queue.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pQueue          The queue to schedule.
 *
 * @remarks Called while owning the lock.
 */
void tmTimerQueueSchedule(PVM pVM, PTMTIMERQUEUE pQueue)
{
    TM_ASSERT_TIMER_LOCK_OWNERSHIP(pVM);
    NOREF(pVM);

    /*
     * Dequeue the scheduling list and iterate it.
     */
    int32_t offNext = ASMAtomicXchgS32(&pQueue->offSchedule, 0);
    Log2(("tmTimerQueueSchedule: pQueue=%p:{.enmClock=%d, offNext=%RI32, .u64Expired=%'RU64}\n", pQueue, pQueue->enmClock, offNext, pQueue->u64Expire));
    if (!offNext)
        return;
    PTMTIMER pNext = (PTMTIMER)((intptr_t)pQueue + offNext);
    while (pNext)
    {
        /*
         * Unlink the head timer and find the next one.
         */
        PTMTIMER pTimer = pNext;
        pNext = pNext->offScheduleNext ? (PTMTIMER)((intptr_t)pNext + pNext->offScheduleNext) : NULL;
        pTimer->offScheduleNext = 0;

        /*
         * Do the scheduling.
         */
        Log2(("tmTimerQueueSchedule: %p:{.enmState=%s, .enmClock=%d, .enmType=%d, .pszDesc=%s}\n",
              pTimer, tmTimerState(pTimer->enmState), pTimer->enmClock, pTimer->enmType, R3STRING(pTimer->pszDesc)));
        tmTimerQueueScheduleOne(pQueue, pTimer);
        Log2(("tmTimerQueueSchedule: %p: new %s\n", pTimer, tmTimerState(pTimer->enmState)));
    } /* foreach timer in current schedule batch. */
    Log2(("tmTimerQueueSchedule: u64Expired=%'RU64\n", pQueue->u64Expire));
}


#ifdef VBOX_STRICT
/**
 * Checks that the timer queues are sane.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pszWhere    Caller location clue.
 *
 * @remarks Called while owning the lock.
 */
void tmTimerQueuesSanityChecks(PVM pVM, const char *pszWhere)
{
    TM_ASSERT_TIMER_LOCK_OWNERSHIP(pVM);

    /*
     * Check the linking of the active lists.
     */
    bool fHaveVirtualSyncLock = false;
    for (int i = 0; i < TMCLOCK_MAX; i++)
    {
        PTMTIMERQUEUE pQueue = &pVM->tm.s.CTX_SUFF(paTimerQueues)[i];
        Assert((int)pQueue->enmClock == i);
        if (pQueue->enmClock == TMCLOCK_VIRTUAL_SYNC)
        {
            if (PDMCritSectTryEnter(&pVM->tm.s.VirtualSyncLock) != VINF_SUCCESS)
                continue;
            fHaveVirtualSyncLock = true;
        }
        PTMTIMER pPrev = NULL;
        for (PTMTIMER pCur = TMTIMER_GET_HEAD(pQueue); pCur; pPrev = pCur, pCur = TMTIMER_GET_NEXT(pCur))
        {
            AssertMsg((int)pCur->enmClock == i, ("%s: %d != %d\n", pszWhere, pCur->enmClock, i));
            AssertMsg(TMTIMER_GET_PREV(pCur) == pPrev, ("%s: %p != %p\n", pszWhere, TMTIMER_GET_PREV(pCur), pPrev));
            TMTIMERSTATE enmState = pCur->enmState;
            switch (enmState)
            {
                case TMTIMERSTATE_ACTIVE:
                    AssertMsg(  !pCur->offScheduleNext
                              || pCur->enmState != TMTIMERSTATE_ACTIVE,
                              ("%s: %RI32\n", pszWhere, pCur->offScheduleNext));
                    break;
                case TMTIMERSTATE_PENDING_STOP:
                case TMTIMERSTATE_PENDING_RESCHEDULE:
                case TMTIMERSTATE_PENDING_RESCHEDULE_SET_EXPIRE:
                    break;
                default:
                    AssertMsgFailed(("%s: Invalid state enmState=%d %s\n", pszWhere, enmState, tmTimerState(enmState)));
                    break;
            }
        }
    }


# ifdef IN_RING3
    /*
     * Do the big list and check that active timers all are in the active lists.
     */
    PTMTIMERR3 pPrev = NULL;
    for (PTMTIMERR3 pCur = pVM->tm.s.pCreated; pCur; pPrev = pCur, pCur = pCur->pBigNext)
    {
        Assert(pCur->pBigPrev == pPrev);
        Assert((unsigned)pCur->enmClock < (unsigned)TMCLOCK_MAX);

        TMTIMERSTATE enmState = pCur->enmState;
        switch (enmState)
        {
            case TMTIMERSTATE_ACTIVE:
            case TMTIMERSTATE_PENDING_STOP:
            case TMTIMERSTATE_PENDING_RESCHEDULE:
            case TMTIMERSTATE_PENDING_RESCHEDULE_SET_EXPIRE:
                if (fHaveVirtualSyncLock || pCur->enmClock != TMCLOCK_VIRTUAL_SYNC)
                {
                    PTMTIMERR3 pCurAct = TMTIMER_GET_HEAD(&pVM->tm.s.CTX_SUFF(paTimerQueues)[pCur->enmClock]);
                    Assert(pCur->offPrev || pCur == pCurAct);
                    while (pCurAct && pCurAct != pCur)
                        pCurAct = TMTIMER_GET_NEXT(pCurAct);
                    Assert(pCurAct == pCur);
                }
                break;

            case TMTIMERSTATE_PENDING_SCHEDULE:
            case TMTIMERSTATE_PENDING_STOP_SCHEDULE:
            case TMTIMERSTATE_STOPPED:
            case TMTIMERSTATE_EXPIRED_DELIVER:
                if (fHaveVirtualSyncLock || pCur->enmClock != TMCLOCK_VIRTUAL_SYNC)
                {
                    Assert(!pCur->offNext);
                    Assert(!pCur->offPrev);
                    for (PTMTIMERR3 pCurAct = TMTIMER_GET_HEAD(&pVM->tm.s.CTX_SUFF(paTimerQueues)[pCur->enmClock]);
                          pCurAct;
                          pCurAct = TMTIMER_GET_NEXT(pCurAct))
                    {
                        Assert(pCurAct != pCur);
                        Assert(TMTIMER_GET_NEXT(pCurAct) != pCur);
                        Assert(TMTIMER_GET_PREV(pCurAct) != pCur);
                    }
                }
                break;

            /* ignore */
            case TMTIMERSTATE_PENDING_SCHEDULE_SET_EXPIRE:
                break;

            /* shouldn't get here! */
            case TMTIMERSTATE_EXPIRED_GET_UNLINK:
            case TMTIMERSTATE_DESTROY:
            default:
                AssertMsgFailed(("Invalid state enmState=%d %s\n", enmState, tmTimerState(enmState)));
                break;
        }
    }
# endif /* IN_RING3 */

    if (fHaveVirtualSyncLock)
        PDMCritSectLeave(&pVM->tm.s.VirtualSyncLock);
}
#endif /* !VBOX_STRICT */

#ifdef VBOX_HIGH_RES_TIMERS_HACK

/**
 * Worker for tmTimerPollInternal that handles misses when the dedicated timer
 * EMT is polling.
 *
 * @returns See tmTimerPollInternal.
 * @param   pVM                 The cross context VM structure.
 * @param   u64Now              Current virtual clock timestamp.
 * @param   u64Delta            The delta to the next even in ticks of the
 *                              virtual clock.
 * @param   pu64Delta           Where to return the delta.
 */
DECLINLINE(uint64_t) tmTimerPollReturnMiss(PVM pVM, uint64_t u64Now, uint64_t u64Delta, uint64_t *pu64Delta)
{
    Assert(!(u64Delta & RT_BIT_64(63)));

    if (!pVM->tm.s.fVirtualWarpDrive)
    {
        *pu64Delta = u64Delta;
        return u64Delta + u64Now + pVM->tm.s.u64VirtualOffset;
    }

    /*
     * Warp drive adjustments - this is the reverse of what tmVirtualGetRaw is doing.
     */
    uint64_t const u64Start = pVM->tm.s.u64VirtualWarpDriveStart;
    uint32_t const u32Pct   = pVM->tm.s.u32VirtualWarpDrivePercentage;

    uint64_t u64GipTime = u64Delta + u64Now + pVM->tm.s.u64VirtualOffset;
    u64GipTime -= u64Start; /* the start is GIP time. */
    if (u64GipTime >= u64Delta)
    {
        ASMMultU64ByU32DivByU32(u64GipTime, 100, u32Pct);
        ASMMultU64ByU32DivByU32(u64Delta, 100, u32Pct);
    }
    else
    {
        u64Delta -= u64GipTime;
        ASMMultU64ByU32DivByU32(u64GipTime, 100, u32Pct);
        u64Delta += u64GipTime;
    }
    *pu64Delta = u64Delta;
    u64GipTime += u64Start;
    return u64GipTime;
}


/**
 * Worker for tmTimerPollInternal dealing with returns on virtual CPUs other
 * than the one dedicated to timer work.
 *
 * @returns See tmTimerPollInternal.
 * @param   pVM                 The cross context VM structure.
 * @param   u64Now              Current virtual clock timestamp.
 * @param   pu64Delta           Where to return the delta.
 */
DECL_FORCE_INLINE(uint64_t) tmTimerPollReturnOtherCpu(PVM pVM, uint64_t u64Now, uint64_t *pu64Delta)
{
    static const uint64_t s_u64OtherRet = 500000000; /* 500 ms for non-timer EMTs. */
    *pu64Delta = s_u64OtherRet;
    return u64Now + pVM->tm.s.u64VirtualOffset + s_u64OtherRet;
}


/**
 * Worker for tmTimerPollInternal.
 *
 * @returns See tmTimerPollInternal.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pVCpuDst    The cross context virtual CPU structure of the dedicated
 *                      timer EMT.
 * @param   u64Now      Current virtual clock timestamp.
 * @param   pu64Delta   Where to return the delta.
 * @param   pCounter    The statistics counter to update.
 */
DECL_FORCE_INLINE(uint64_t) tmTimerPollReturnHit(PVM pVM, PVMCPU pVCpu, PVMCPU pVCpuDst, uint64_t u64Now,
                                                 uint64_t *pu64Delta, PSTAMCOUNTER pCounter)
{
    STAM_COUNTER_INC(pCounter); NOREF(pCounter);
    if (pVCpuDst != pVCpu)
        return tmTimerPollReturnOtherCpu(pVM, u64Now, pu64Delta);
    *pu64Delta = 0;
    return 0;
}

/**
 * Common worker for TMTimerPollGIP and TMTimerPoll.
 *
 * This function is called before FFs are checked in the inner execution EM loops.
 *
 * @returns The GIP timestamp of the next event.
 *          0 if the next event has already expired.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pu64Delta   Where to store the delta.
 *
 * @thread  The emulation thread.
 *
 * @remarks GIP uses ns ticks.
 */
DECL_FORCE_INLINE(uint64_t) tmTimerPollInternal(PVM pVM, PVMCPU pVCpu, uint64_t *pu64Delta)
{
    PVMCPU                  pVCpuDst      = &pVM->aCpus[pVM->tm.s.idTimerCpu];
    const uint64_t          u64Now        = TMVirtualGetNoCheck(pVM);
    STAM_COUNTER_INC(&pVM->tm.s.StatPoll);

    /*
     * Return straight away if the timer FF is already set ...
     */
    if (VMCPU_FF_IS_SET(pVCpuDst, VMCPU_FF_TIMER))
        return tmTimerPollReturnHit(pVM, pVCpu, pVCpuDst, u64Now, pu64Delta, &pVM->tm.s.StatPollAlreadySet);

    /*
     * ... or if timers are being run.
     */
    if (ASMAtomicReadBool(&pVM->tm.s.fRunningQueues))
    {
        STAM_COUNTER_INC(&pVM->tm.s.StatPollRunning);
        return tmTimerPollReturnOtherCpu(pVM, u64Now, pu64Delta);
    }

    /*
     * Check for TMCLOCK_VIRTUAL expiration.
     */
    const uint64_t  u64Expire1 = ASMAtomicReadU64(&pVM->tm.s.CTX_SUFF(paTimerQueues)[TMCLOCK_VIRTUAL].u64Expire);
    const int64_t   i64Delta1  = u64Expire1 - u64Now;
    if (i64Delta1 <= 0)
    {
        if (!VMCPU_FF_IS_SET(pVCpuDst, VMCPU_FF_TIMER))
        {
            Log5(("TMAll(%u): FF: %d -> 1\n", __LINE__, VMCPU_FF_IS_PENDING(pVCpuDst, VMCPU_FF_TIMER)));
            VMCPU_FF_SET(pVCpuDst, VMCPU_FF_TIMER);
#if defined(IN_RING3) && defined(VBOX_WITH_REM)
            REMR3NotifyTimerPending(pVM, pVCpuDst);
#endif
        }
        LogFlow(("TMTimerPoll: expire1=%'RU64 <= now=%'RU64\n", u64Expire1, u64Now));
        return tmTimerPollReturnHit(pVM, pVCpu, pVCpuDst, u64Now, pu64Delta, &pVM->tm.s.StatPollVirtual);
    }

    /*
     * Check for TMCLOCK_VIRTUAL_SYNC expiration.
     * This isn't quite as straight forward if in a catch-up, not only do
     * we have to adjust the 'now' but when have to adjust the delta as well.
     */

    /*
     * Optimistic lockless approach.
     */
    uint64_t u64VirtualSyncNow;
    uint64_t u64Expire2 = ASMAtomicUoReadU64(&pVM->tm.s.CTX_SUFF(paTimerQueues)[TMCLOCK_VIRTUAL_SYNC].u64Expire);
    if (ASMAtomicUoReadBool(&pVM->tm.s.fVirtualSyncTicking))
    {
        if (!ASMAtomicUoReadBool(&pVM->tm.s.fVirtualSyncCatchUp))
        {
            u64VirtualSyncNow = ASMAtomicReadU64(&pVM->tm.s.offVirtualSync);
            if (RT_LIKELY(   ASMAtomicUoReadBool(&pVM->tm.s.fVirtualSyncTicking)
                          && !ASMAtomicUoReadBool(&pVM->tm.s.fVirtualSyncCatchUp)
                          && u64VirtualSyncNow == ASMAtomicReadU64(&pVM->tm.s.offVirtualSync)
                          && u64Expire2 == ASMAtomicUoReadU64(&pVM->tm.s.CTX_SUFF(paTimerQueues)[TMCLOCK_VIRTUAL_SYNC].u64Expire)))
            {
                u64VirtualSyncNow = u64Now - u64VirtualSyncNow;
                int64_t i64Delta2 = u64Expire2 - u64VirtualSyncNow;
                if (i64Delta2 > 0)
                {
                    STAM_COUNTER_INC(&pVM->tm.s.StatPollSimple);
                    STAM_COUNTER_INC(&pVM->tm.s.StatPollMiss);

                    if (pVCpu == pVCpuDst)
                        return tmTimerPollReturnMiss(pVM, u64Now, RT_MIN(i64Delta1, i64Delta2), pu64Delta);
                    return tmTimerPollReturnOtherCpu(pVM, u64Now, pu64Delta);
                }

                if (    !pVM->tm.s.fRunningQueues
                    &&  !VMCPU_FF_IS_SET(pVCpuDst, VMCPU_FF_TIMER))
                {
                    Log5(("TMAll(%u): FF: %d -> 1\n", __LINE__, VMCPU_FF_IS_PENDING(pVCpuDst, VMCPU_FF_TIMER)));
                    VMCPU_FF_SET(pVCpuDst, VMCPU_FF_TIMER);
#if defined(IN_RING3) && defined(VBOX_WITH_REM)
                    REMR3NotifyTimerPending(pVM, pVCpuDst);
#endif
                }

                STAM_COUNTER_INC(&pVM->tm.s.StatPollSimple);
                LogFlow(("TMTimerPoll: expire2=%'RU64 <= now=%'RU64\n", u64Expire2, u64Now));
                return tmTimerPollReturnHit(pVM, pVCpu, pVCpuDst, u64Now, pu64Delta, &pVM->tm.s.StatPollVirtualSync);
            }
        }
    }
    else
    {
        STAM_COUNTER_INC(&pVM->tm.s.StatPollSimple);
        LogFlow(("TMTimerPoll: stopped\n"));
        return tmTimerPollReturnHit(pVM, pVCpu, pVCpuDst, u64Now, pu64Delta, &pVM->tm.s.StatPollVirtualSync);
    }

    /*
     * Complicated lockless approach.
     */
    uint64_t    off;
    uint32_t    u32Pct = 0;
    bool        fCatchUp;
    int         cOuterTries = 42;
    for (;; cOuterTries--)
    {
        fCatchUp   = ASMAtomicReadBool(&pVM->tm.s.fVirtualSyncCatchUp);
        off        = ASMAtomicReadU64(&pVM->tm.s.offVirtualSync);
        u64Expire2 = ASMAtomicReadU64(&pVM->tm.s.CTX_SUFF(paTimerQueues)[TMCLOCK_VIRTUAL_SYNC].u64Expire);
        if (fCatchUp)
        {
            /* No changes allowed, try get a consistent set of parameters. */
            uint64_t const u64Prev    = ASMAtomicReadU64(&pVM->tm.s.u64VirtualSyncCatchUpPrev);
            uint64_t const offGivenUp = ASMAtomicReadU64(&pVM->tm.s.offVirtualSyncGivenUp);
            u32Pct                    = ASMAtomicReadU32(&pVM->tm.s.u32VirtualSyncCatchUpPercentage);
            if (    (   u64Prev    == ASMAtomicReadU64(&pVM->tm.s.u64VirtualSyncCatchUpPrev)
                     && offGivenUp == ASMAtomicReadU64(&pVM->tm.s.offVirtualSyncGivenUp)
                     && u32Pct     == ASMAtomicReadU32(&pVM->tm.s.u32VirtualSyncCatchUpPercentage)
                     && off        == ASMAtomicReadU64(&pVM->tm.s.offVirtualSync)
                     && u64Expire2 == ASMAtomicReadU64(&pVM->tm.s.CTX_SUFF(paTimerQueues)[TMCLOCK_VIRTUAL_SYNC].u64Expire)
                     && ASMAtomicReadBool(&pVM->tm.s.fVirtualSyncCatchUp)
                     && ASMAtomicReadBool(&pVM->tm.s.fVirtualSyncTicking))
                ||  cOuterTries <= 0)
            {
                uint64_t u64Delta = u64Now - u64Prev;
                if (RT_LIKELY(!(u64Delta >> 32)))
                {
                    uint64_t u64Sub = ASMMultU64ByU32DivByU32(u64Delta, u32Pct, 100);
                    if (off > u64Sub + offGivenUp)
                        off -= u64Sub;
                    else /* we've completely caught up. */
                        off = offGivenUp;
                }
                else
                    /* More than 4 seconds since last time (or negative), ignore it. */
                    Log(("TMVirtualGetSync: u64Delta=%RX64 (NoLock)\n", u64Delta));

                /* Check that we're still running and in catch up. */
                if (    ASMAtomicUoReadBool(&pVM->tm.s.fVirtualSyncTicking)
                    &&  ASMAtomicReadBool(&pVM->tm.s.fVirtualSyncCatchUp))
                    break;
            }
        }
        else if (   off        == ASMAtomicReadU64(&pVM->tm.s.offVirtualSync)
                 && u64Expire2 == ASMAtomicReadU64(&pVM->tm.s.CTX_SUFF(paTimerQueues)[TMCLOCK_VIRTUAL_SYNC].u64Expire)
                 && !ASMAtomicReadBool(&pVM->tm.s.fVirtualSyncCatchUp)
                 && ASMAtomicReadBool(&pVM->tm.s.fVirtualSyncTicking))
            break; /* Got an consistent offset */

        /* Repeat the initial checks before iterating. */
        if (VMCPU_FF_IS_SET(pVCpuDst, VMCPU_FF_TIMER))
            return tmTimerPollReturnHit(pVM, pVCpu, pVCpuDst, u64Now, pu64Delta, &pVM->tm.s.StatPollAlreadySet);
        if (ASMAtomicUoReadBool(&pVM->tm.s.fRunningQueues))
        {
            STAM_COUNTER_INC(&pVM->tm.s.StatPollRunning);
            return tmTimerPollReturnOtherCpu(pVM, u64Now, pu64Delta);
        }
        if (!ASMAtomicUoReadBool(&pVM->tm.s.fVirtualSyncTicking))
        {
            LogFlow(("TMTimerPoll: stopped\n"));
            return tmTimerPollReturnHit(pVM, pVCpu, pVCpuDst, u64Now, pu64Delta, &pVM->tm.s.StatPollVirtualSync);
        }
        if (cOuterTries <= 0)
            break; /* that's enough */
    }
    if (cOuterTries <= 0)
        STAM_COUNTER_INC(&pVM->tm.s.StatPollELoop);
    u64VirtualSyncNow = u64Now - off;

    /* Calc delta and see if we've got a virtual sync hit. */
    int64_t i64Delta2 = u64Expire2 - u64VirtualSyncNow;
    if (i64Delta2 <= 0)
    {
        if (    !pVM->tm.s.fRunningQueues
            &&  !VMCPU_FF_IS_SET(pVCpuDst, VMCPU_FF_TIMER))
        {
            Log5(("TMAll(%u): FF: %d -> 1\n", __LINE__, VMCPU_FF_IS_PENDING(pVCpuDst, VMCPU_FF_TIMER)));
            VMCPU_FF_SET(pVCpuDst, VMCPU_FF_TIMER);
#if defined(IN_RING3) && defined(VBOX_WITH_REM)
            REMR3NotifyTimerPending(pVM, pVCpuDst);
#endif
        }
        STAM_COUNTER_INC(&pVM->tm.s.StatPollVirtualSync);
        LogFlow(("TMTimerPoll: expire2=%'RU64 <= now=%'RU64\n", u64Expire2, u64Now));
        return tmTimerPollReturnHit(pVM, pVCpu, pVCpuDst, u64Now, pu64Delta, &pVM->tm.s.StatPollVirtualSync);
    }

    /*
     * Return the time left to the next event.
     */
    STAM_COUNTER_INC(&pVM->tm.s.StatPollMiss);
    if (pVCpu == pVCpuDst)
    {
        if (fCatchUp)
            i64Delta2 = ASMMultU64ByU32DivByU32(i64Delta2, 100, u32Pct + 100);
        return tmTimerPollReturnMiss(pVM, u64Now, RT_MIN(i64Delta1, i64Delta2), pu64Delta);
    }
    return tmTimerPollReturnOtherCpu(pVM, u64Now, pu64Delta);
}


/**
 * Set FF if we've passed the next virtual event.
 *
 * This function is called before FFs are checked in the inner execution EM loops.
 *
 * @returns true if timers are pending, false if not.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @thread  The emulation thread.
 */
VMMDECL(bool) TMTimerPollBool(PVM pVM, PVMCPU pVCpu)
{
    AssertCompile(TMCLOCK_FREQ_VIRTUAL == 1000000000);
    uint64_t off = 0;
    tmTimerPollInternal(pVM, pVCpu, &off);
    return off == 0;
}


/**
 * Set FF if we've passed the next virtual event.
 *
 * This function is called before FFs are checked in the inner execution EM loops.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @thread  The emulation thread.
 */
VMM_INT_DECL(void) TMTimerPollVoid(PVM pVM, PVMCPU pVCpu)
{
    uint64_t off;
    tmTimerPollInternal(pVM, pVCpu, &off);
}


/**
 * Set FF if we've passed the next virtual event.
 *
 * This function is called before FFs are checked in the inner execution EM loops.
 *
 * @returns The GIP timestamp of the next event.
 *          0 if the next event has already expired.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pu64Delta   Where to store the delta.
 * @thread  The emulation thread.
 */
VMM_INT_DECL(uint64_t) TMTimerPollGIP(PVM pVM, PVMCPU pVCpu, uint64_t *pu64Delta)
{
    return tmTimerPollInternal(pVM, pVCpu, pu64Delta);
}

#endif /* VBOX_HIGH_RES_TIMERS_HACK */

/**
 * Gets the host context ring-3 pointer of the timer.
 *
 * @returns HC R3 pointer.
 * @param   pTimer      Timer handle as returned by one of the create functions.
 */
VMMDECL(PTMTIMERR3) TMTimerR3Ptr(PTMTIMER pTimer)
{
    return (PTMTIMERR3)MMHyperCCToR3(pTimer->CTX_SUFF(pVM), pTimer);
}


/**
 * Gets the host context ring-0 pointer of the timer.
 *
 * @returns HC R0 pointer.
 * @param   pTimer      Timer handle as returned by one of the create functions.
 */
VMMDECL(PTMTIMERR0) TMTimerR0Ptr(PTMTIMER pTimer)
{
    return (PTMTIMERR0)MMHyperCCToR0(pTimer->CTX_SUFF(pVM), pTimer);
}


/**
 * Gets the RC pointer of the timer.
 *
 * @returns RC pointer.
 * @param   pTimer      Timer handle as returned by one of the create functions.
 */
VMMDECL(PTMTIMERRC) TMTimerRCPtr(PTMTIMER pTimer)
{
    return (PTMTIMERRC)MMHyperCCToRC(pTimer->CTX_SUFF(pVM), pTimer);
}


/**
 * Locks the timer clock.
 *
 * @returns VINF_SUCCESS on success, @a rcBusy if busy, and VERR_NOT_SUPPORTED
 *          if the clock does not have a lock.
 * @param   pTimer              The timer which clock lock we wish to take.
 * @param   rcBusy              What to return in ring-0 and raw-mode context
 *                              if the lock is busy.  Pass VINF_SUCCESS to
 *                              acquired the critical section thru a ring-3
                                call if necessary.
 *
 * @remarks Currently only supported on timers using the virtual sync clock.
 */
VMMDECL(int) TMTimerLock(PTMTIMER pTimer, int rcBusy)
{
    AssertPtr(pTimer);
    AssertReturn(pTimer->enmClock == TMCLOCK_VIRTUAL_SYNC, VERR_NOT_SUPPORTED);
    return PDMCritSectEnter(&pTimer->CTX_SUFF(pVM)->tm.s.VirtualSyncLock, rcBusy);
}


/**
 * Unlocks a timer clock locked by TMTimerLock.
 *
 * @param   pTimer              The timer which clock to unlock.
 */
VMMDECL(void) TMTimerUnlock(PTMTIMER pTimer)
{
    AssertPtr(pTimer);
    AssertReturnVoid(pTimer->enmClock == TMCLOCK_VIRTUAL_SYNC);
    PDMCritSectLeave(&pTimer->CTX_SUFF(pVM)->tm.s.VirtualSyncLock);
}


/**
 * Checks if the current thread owns the timer clock lock.
 *
 * @returns @c true if its the owner, @c false if not.
 * @param   pTimer              The timer handle.
 */
VMMDECL(bool) TMTimerIsLockOwner(PTMTIMER pTimer)
{
    AssertPtr(pTimer);
    AssertReturn(pTimer->enmClock == TMCLOCK_VIRTUAL_SYNC, false);
    return PDMCritSectIsOwner(&pTimer->CTX_SUFF(pVM)->tm.s.VirtualSyncLock);
}


/**
 * Optimized TMTimerSet code path for starting an inactive timer.
 *
 * @returns VBox status code.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pTimer          The timer handle.
 * @param   u64Expire       The new expire time.
 */
static int tmTimerSetOptimizedStart(PVM pVM, PTMTIMER pTimer, uint64_t u64Expire)
{
    Assert(!pTimer->offPrev);
    Assert(!pTimer->offNext);
    Assert(pTimer->enmState == TMTIMERSTATE_ACTIVE);

    TMCLOCK const enmClock = pTimer->enmClock;

    /*
     * Calculate and set the expiration time.
     */
    if (enmClock == TMCLOCK_VIRTUAL_SYNC)
    {
        uint64_t u64Last = ASMAtomicReadU64(&pVM->tm.s.u64VirtualSync);
        AssertMsgStmt(u64Expire >= u64Last,
                      ("exp=%#llx last=%#llx\n", u64Expire, u64Last),
                      u64Expire = u64Last);
    }
    ASMAtomicWriteU64(&pTimer->u64Expire, u64Expire);
    Log2(("tmTimerSetOptimizedStart: %p:{.pszDesc='%s', .u64Expire=%'RU64}\n", pTimer, R3STRING(pTimer->pszDesc), u64Expire));

    /*
     * Link the timer into the active list.
     */
    tmTimerQueueLinkActive(&pVM->tm.s.CTX_SUFF(paTimerQueues)[enmClock], pTimer, u64Expire);

    STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetOpt);
    TM_UNLOCK_TIMERS(pVM);
    return VINF_SUCCESS;
}


/**
 * TMTimerSet for the virtual sync timer queue.
 *
 * This employs a greatly simplified state machine by always acquiring the
 * queue lock and bypassing the scheduling list.
 *
 * @returns VBox status code
 * @param   pVM                 The cross context VM structure.
 * @param   pTimer              The timer handle.
 * @param   u64Expire           The expiration time.
 */
static int tmTimerVirtualSyncSet(PVM pVM, PTMTIMER pTimer, uint64_t u64Expire)
{
    STAM_PROFILE_START(&pVM->tm.s.CTX_SUFF_Z(StatTimerSetVs), a);
    VM_ASSERT_EMT(pVM);
    TMTIMER_ASSERT_SYNC_CRITSECT_ORDER(pVM, pTimer);
    int rc = PDMCritSectEnter(&pVM->tm.s.VirtualSyncLock, VINF_SUCCESS);
    AssertRCReturn(rc, rc);

    PTMTIMERQUEUE   pQueue   = &pVM->tm.s.CTX_SUFF(paTimerQueues)[TMCLOCK_VIRTUAL_SYNC];
    TMTIMERSTATE    enmState = pTimer->enmState;
    switch (enmState)
    {
        case TMTIMERSTATE_EXPIRED_DELIVER:
        case TMTIMERSTATE_STOPPED:
            if (enmState == TMTIMERSTATE_EXPIRED_DELIVER)
                STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetVsStExpDeliver);
            else
                STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetVsStStopped);

            AssertMsg(u64Expire >= pVM->tm.s.u64VirtualSync,
                      ("%'RU64 < %'RU64 %s\n", u64Expire, pVM->tm.s.u64VirtualSync, R3STRING(pTimer->pszDesc)));
            pTimer->u64Expire = u64Expire;
            TM_SET_STATE(pTimer, TMTIMERSTATE_ACTIVE);
            tmTimerQueueLinkActive(pQueue, pTimer, u64Expire);
            rc = VINF_SUCCESS;
            break;

        case TMTIMERSTATE_ACTIVE:
            STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetVsStActive);
            tmTimerQueueUnlinkActive(pQueue, pTimer);
            pTimer->u64Expire = u64Expire;
            tmTimerQueueLinkActive(pQueue, pTimer, u64Expire);
            rc = VINF_SUCCESS;
            break;

        case TMTIMERSTATE_PENDING_RESCHEDULE:
        case TMTIMERSTATE_PENDING_STOP:
        case TMTIMERSTATE_PENDING_SCHEDULE:
        case TMTIMERSTATE_PENDING_STOP_SCHEDULE:
        case TMTIMERSTATE_EXPIRED_GET_UNLINK:
        case TMTIMERSTATE_PENDING_SCHEDULE_SET_EXPIRE:
        case TMTIMERSTATE_PENDING_RESCHEDULE_SET_EXPIRE:
        case TMTIMERSTATE_DESTROY:
        case TMTIMERSTATE_FREE:
            AssertLogRelMsgFailed(("Invalid timer state %s: %s\n", tmTimerState(enmState), R3STRING(pTimer->pszDesc)));
            rc = VERR_TM_INVALID_STATE;
            break;

        default:
            AssertMsgFailed(("Unknown timer state %d: %s\n", enmState, R3STRING(pTimer->pszDesc)));
            rc = VERR_TM_UNKNOWN_STATE;
            break;
    }

    STAM_PROFILE_STOP(&pVM->tm.s.CTX_SUFF_Z(StatTimerSetVs), a);
    PDMCritSectLeave(&pVM->tm.s.VirtualSyncLock);
    return rc;
}


/**
 * Arm a timer with a (new) expire time.
 *
 * @returns VBox status code.
 * @param   pTimer          Timer handle as returned by one of the create functions.
 * @param   u64Expire       New expire time.
 */
VMMDECL(int) TMTimerSet(PTMTIMER pTimer, uint64_t u64Expire)
{
    PVM pVM = pTimer->CTX_SUFF(pVM);

    /* Treat virtual sync timers specially. */
    if (pTimer->enmClock == TMCLOCK_VIRTUAL_SYNC)
        return tmTimerVirtualSyncSet(pVM, pTimer, u64Expire);

    STAM_PROFILE_START(&pVM->tm.s.CTX_SUFF_Z(StatTimerSet), a);
    TMTIMER_ASSERT_CRITSECT(pTimer);

    DBGFTRACE_U64_TAG2(pVM, u64Expire, "TMTimerSet", R3STRING(pTimer->pszDesc));

#ifdef VBOX_WITH_STATISTICS
    /*
     * Gather optimization info.
     */
    STAM_COUNTER_INC(&pVM->tm.s.StatTimerSet);
    TMTIMERSTATE enmOrgState = pTimer->enmState;
    switch (enmOrgState)
    {
        case TMTIMERSTATE_STOPPED:                  STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetStStopped); break;
        case TMTIMERSTATE_EXPIRED_DELIVER:          STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetStExpDeliver); break;
        case TMTIMERSTATE_ACTIVE:                   STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetStActive); break;
        case TMTIMERSTATE_PENDING_STOP:             STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetStPendStop); break;
        case TMTIMERSTATE_PENDING_STOP_SCHEDULE:    STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetStPendStopSched); break;
        case TMTIMERSTATE_PENDING_SCHEDULE:         STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetStPendSched); break;
        case TMTIMERSTATE_PENDING_RESCHEDULE:       STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetStPendResched); break;
        default:                                    STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetStOther); break;
    }
#endif

    /*
     * The most common case is setting the timer again during the callback.
     * The second most common case is starting a timer at some other time.
     */
#if 1
    TMTIMERSTATE enmState1 = pTimer->enmState;
    if (    enmState1 == TMTIMERSTATE_EXPIRED_DELIVER
        ||  (   enmState1 == TMTIMERSTATE_STOPPED
             && pTimer->pCritSect))
    {
        /* Try take the TM lock and check the state again. */
        if (RT_SUCCESS_NP(TM_TRY_LOCK_TIMERS(pVM)))
        {
            if (RT_LIKELY(tmTimerTry(pTimer, TMTIMERSTATE_ACTIVE, enmState1)))
            {
                tmTimerSetOptimizedStart(pVM, pTimer, u64Expire);
                STAM_PROFILE_STOP(&pVM->tm.s.CTX_SUFF_Z(StatTimerSet), a);
                return VINF_SUCCESS;
            }
            TM_UNLOCK_TIMERS(pVM);
        }
    }
#endif

    /*
     * Unoptimized code path.
     */
    int cRetries = 1000;
    do
    {
        /*
         * Change to any of the SET_EXPIRE states if valid and then to SCHEDULE or RESCHEDULE.
         */
        TMTIMERSTATE enmState = pTimer->enmState;
        Log2(("TMTimerSet: %p:{.enmState=%s, .pszDesc='%s'} cRetries=%d u64Expire=%'RU64\n",
              pTimer, tmTimerState(enmState), R3STRING(pTimer->pszDesc), cRetries, u64Expire));
        switch (enmState)
        {
            case TMTIMERSTATE_EXPIRED_DELIVER:
            case TMTIMERSTATE_STOPPED:
                if (tmTimerTryWithLink(pTimer, TMTIMERSTATE_PENDING_SCHEDULE_SET_EXPIRE, enmState))
                {
                    Assert(!pTimer->offPrev);
                    Assert(!pTimer->offNext);
                    pTimer->u64Expire = u64Expire;
                    TM_SET_STATE(pTimer, TMTIMERSTATE_PENDING_SCHEDULE);
                    tmSchedule(pTimer);
                    STAM_PROFILE_STOP(&pVM->tm.s.CTX_SUFF_Z(StatTimerSet), a);
                    return VINF_SUCCESS;
                }
                break;

            case TMTIMERSTATE_PENDING_SCHEDULE:
            case TMTIMERSTATE_PENDING_STOP_SCHEDULE:
                if (tmTimerTry(pTimer, TMTIMERSTATE_PENDING_SCHEDULE_SET_EXPIRE, enmState))
                {
                    pTimer->u64Expire = u64Expire;
                    TM_SET_STATE(pTimer, TMTIMERSTATE_PENDING_SCHEDULE);
                    tmSchedule(pTimer);
                    STAM_PROFILE_STOP(&pVM->tm.s.CTX_SUFF_Z(StatTimerSet), a);
                    return VINF_SUCCESS;
                }
                break;


            case TMTIMERSTATE_ACTIVE:
                if (tmTimerTryWithLink(pTimer, TMTIMERSTATE_PENDING_RESCHEDULE_SET_EXPIRE, enmState))
                {
                    pTimer->u64Expire = u64Expire;
                    TM_SET_STATE(pTimer, TMTIMERSTATE_PENDING_RESCHEDULE);
                    tmSchedule(pTimer);
                    STAM_PROFILE_STOP(&pVM->tm.s.CTX_SUFF_Z(StatTimerSet), a);
                    return VINF_SUCCESS;
                }
                break;

            case TMTIMERSTATE_PENDING_RESCHEDULE:
            case TMTIMERSTATE_PENDING_STOP:
                if (tmTimerTry(pTimer, TMTIMERSTATE_PENDING_RESCHEDULE_SET_EXPIRE, enmState))
                {
                    pTimer->u64Expire = u64Expire;
                    TM_SET_STATE(pTimer, TMTIMERSTATE_PENDING_RESCHEDULE);
                    tmSchedule(pTimer);
                    STAM_PROFILE_STOP(&pVM->tm.s.CTX_SUFF_Z(StatTimerSet), a);
                    return VINF_SUCCESS;
                }
                break;


            case TMTIMERSTATE_EXPIRED_GET_UNLINK:
            case TMTIMERSTATE_PENDING_SCHEDULE_SET_EXPIRE:
            case TMTIMERSTATE_PENDING_RESCHEDULE_SET_EXPIRE:
#ifdef IN_RING3
                if (!RTThreadYield())
                    RTThreadSleep(1);
#else
/** @todo call host context and yield after a couple of iterations */
#endif
                break;

            /*
             * Invalid states.
             */
            case TMTIMERSTATE_DESTROY:
            case TMTIMERSTATE_FREE:
                AssertMsgFailed(("Invalid timer state %d (%s)\n", enmState, R3STRING(pTimer->pszDesc)));
                return VERR_TM_INVALID_STATE;
            default:
                AssertMsgFailed(("Unknown timer state %d (%s)\n", enmState, R3STRING(pTimer->pszDesc)));
                return VERR_TM_UNKNOWN_STATE;
        }
    } while (cRetries-- > 0);

    AssertMsgFailed(("Failed waiting for stable state. state=%d (%s)\n", pTimer->enmState, R3STRING(pTimer->pszDesc)));
    STAM_PROFILE_STOP(&pVM->tm.s.CTX_SUFF_Z(StatTimerSet), a);
    return VERR_TM_TIMER_UNSTABLE_STATE;
}


/**
 * Return the current time for the specified clock, setting pu64Now if not NULL.
 *
 * @returns Current time.
 * @param   pVM             The cross context VM structure.
 * @param   enmClock        The clock to query.
 * @param   pu64Now         Optional pointer where to store the return time
 */
DECL_FORCE_INLINE(uint64_t) tmTimerSetRelativeNowWorker(PVM pVM, TMCLOCK enmClock, uint64_t *pu64Now)
{
    uint64_t u64Now;
    switch (enmClock)
    {
        case TMCLOCK_VIRTUAL_SYNC:
            u64Now = TMVirtualSyncGet(pVM);
            break;
        case TMCLOCK_VIRTUAL:
            u64Now = TMVirtualGet(pVM);
            break;
        case TMCLOCK_REAL:
            u64Now = TMRealGet(pVM);
            break;
        default:
            AssertFatalMsgFailed(("%d\n", enmClock));
    }

    if (pu64Now)
        *pu64Now = u64Now;
    return u64Now;
}


/**
 * Optimized TMTimerSetRelative code path.
 *
 * @returns VBox status code.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pTimer          The timer handle.
 * @param   cTicksToNext    Clock ticks until the next time expiration.
 * @param   pu64Now         Where to return the current time stamp used.
 *                          Optional.
 */
static int tmTimerSetRelativeOptimizedStart(PVM pVM, PTMTIMER pTimer, uint64_t cTicksToNext, uint64_t *pu64Now)
{
    Assert(!pTimer->offPrev);
    Assert(!pTimer->offNext);
    Assert(pTimer->enmState == TMTIMERSTATE_ACTIVE);

    /*
     * Calculate and set the expiration time.
     */
    TMCLOCK const   enmClock  = pTimer->enmClock;
    uint64_t const  u64Expire = cTicksToNext + tmTimerSetRelativeNowWorker(pVM, enmClock, pu64Now);
    pTimer->u64Expire         = u64Expire;
    Log2(("tmTimerSetRelativeOptimizedStart: %p:{.pszDesc='%s', .u64Expire=%'RU64} cTicksToNext=%'RU64\n", pTimer, R3STRING(pTimer->pszDesc), u64Expire, cTicksToNext));

    /*
     * Link the timer into the active list.
     */
    DBGFTRACE_U64_TAG2(pVM, u64Expire, "tmTimerSetRelativeOptimizedStart", R3STRING(pTimer->pszDesc));
    tmTimerQueueLinkActive(&pVM->tm.s.CTX_SUFF(paTimerQueues)[enmClock], pTimer, u64Expire);

    STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetRelativeOpt);
    TM_UNLOCK_TIMERS(pVM);
    return VINF_SUCCESS;
}


/**
 * TMTimerSetRelative for the virtual sync timer queue.
 *
 * This employs a greatly simplified state machine by always acquiring the
 * queue lock and bypassing the scheduling list.
 *
 * @returns VBox status code
 * @param   pVM                 The cross context VM structure.
 * @param   pTimer              The timer to (re-)arm.
 * @param   cTicksToNext        Clock ticks until the next time expiration.
 * @param   pu64Now             Where to return the current time stamp used.
 *                              Optional.
 */
static int tmTimerVirtualSyncSetRelative(PVM pVM, PTMTIMER pTimer, uint64_t cTicksToNext, uint64_t *pu64Now)
{
    STAM_PROFILE_START(pVM->tm.s.CTX_SUFF_Z(StatTimerSetRelativeVs), a);
    VM_ASSERT_EMT(pVM);
    TMTIMER_ASSERT_SYNC_CRITSECT_ORDER(pVM, pTimer);
    int rc = PDMCritSectEnter(&pVM->tm.s.VirtualSyncLock, VINF_SUCCESS);
    AssertRCReturn(rc, rc);

    /* Calculate the expiration tick. */
    uint64_t u64Expire = TMVirtualSyncGetNoCheck(pVM);
    if (pu64Now)
        *pu64Now = u64Expire;
    u64Expire += cTicksToNext;

    /* Update the timer. */
    PTMTIMERQUEUE   pQueue    = &pVM->tm.s.CTX_SUFF(paTimerQueues)[TMCLOCK_VIRTUAL_SYNC];
    TMTIMERSTATE    enmState  = pTimer->enmState;
    switch (enmState)
    {
        case TMTIMERSTATE_EXPIRED_DELIVER:
        case TMTIMERSTATE_STOPPED:
            if (enmState == TMTIMERSTATE_EXPIRED_DELIVER)
                STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetRelativeVsStExpDeliver);
            else
                STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetRelativeVsStStopped);
            pTimer->u64Expire = u64Expire;
            TM_SET_STATE(pTimer, TMTIMERSTATE_ACTIVE);
            tmTimerQueueLinkActive(pQueue, pTimer, u64Expire);
            rc = VINF_SUCCESS;
            break;

        case TMTIMERSTATE_ACTIVE:
            STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetRelativeVsStActive);
            tmTimerQueueUnlinkActive(pQueue, pTimer);
            pTimer->u64Expire = u64Expire;
            tmTimerQueueLinkActive(pQueue, pTimer, u64Expire);
            rc = VINF_SUCCESS;
            break;

        case TMTIMERSTATE_PENDING_RESCHEDULE:
        case TMTIMERSTATE_PENDING_STOP:
        case TMTIMERSTATE_PENDING_SCHEDULE:
        case TMTIMERSTATE_PENDING_STOP_SCHEDULE:
        case TMTIMERSTATE_EXPIRED_GET_UNLINK:
        case TMTIMERSTATE_PENDING_SCHEDULE_SET_EXPIRE:
        case TMTIMERSTATE_PENDING_RESCHEDULE_SET_EXPIRE:
        case TMTIMERSTATE_DESTROY:
        case TMTIMERSTATE_FREE:
            AssertLogRelMsgFailed(("Invalid timer state %s: %s\n", tmTimerState(enmState), R3STRING(pTimer->pszDesc)));
            rc = VERR_TM_INVALID_STATE;
            break;

        default:
            AssertMsgFailed(("Unknown timer state %d: %s\n", enmState, R3STRING(pTimer->pszDesc)));
            rc = VERR_TM_UNKNOWN_STATE;
            break;
    }

    STAM_PROFILE_STOP(&pVM->tm.s.CTX_SUFF_Z(StatTimerSetRelativeVs), a);
    PDMCritSectLeave(&pVM->tm.s.VirtualSyncLock);
    return rc;
}


/**
 * Arm a timer with a expire time relative to the current time.
 *
 * @returns VBox status code.
 * @param   pTimer          Timer handle as returned by one of the create functions.
 * @param   cTicksToNext    Clock ticks until the next time expiration.
 * @param   pu64Now         Where to return the current time stamp used.
 *                          Optional.
 */
VMMDECL(int) TMTimerSetRelative(PTMTIMER pTimer, uint64_t cTicksToNext, uint64_t *pu64Now)
{
    PVM pVM = pTimer->CTX_SUFF(pVM);

    /* Treat virtual sync timers specially. */
    if (pTimer->enmClock == TMCLOCK_VIRTUAL_SYNC)
        return tmTimerVirtualSyncSetRelative(pVM, pTimer, cTicksToNext, pu64Now);

    STAM_PROFILE_START(&pVM->tm.s.CTX_SUFF_Z(StatTimerSetRelative), a);
    TMTIMER_ASSERT_CRITSECT(pTimer);

    DBGFTRACE_U64_TAG2(pVM, cTicksToNext, "TMTimerSetRelative", R3STRING(pTimer->pszDesc));

#ifdef VBOX_WITH_STATISTICS
    /*
     * Gather optimization info.
     */
    STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetRelative);
    TMTIMERSTATE enmOrgState = pTimer->enmState;
    switch (enmOrgState)
    {
        case TMTIMERSTATE_STOPPED:                  STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetRelativeStStopped); break;
        case TMTIMERSTATE_EXPIRED_DELIVER:          STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetRelativeStExpDeliver); break;
        case TMTIMERSTATE_ACTIVE:                   STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetRelativeStActive); break;
        case TMTIMERSTATE_PENDING_STOP:             STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetRelativeStPendStop); break;
        case TMTIMERSTATE_PENDING_STOP_SCHEDULE:    STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetRelativeStPendStopSched); break;
        case TMTIMERSTATE_PENDING_SCHEDULE:         STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetRelativeStPendSched); break;
        case TMTIMERSTATE_PENDING_RESCHEDULE:       STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetRelativeStPendResched); break;
        default:                                    STAM_COUNTER_INC(&pVM->tm.s.StatTimerSetRelativeStOther); break;
    }
#endif

    /*
     * Try to take the TM lock and optimize the common cases.
     *
     * With the TM lock we can safely make optimizations like immediate
     * scheduling and we can also be 100% sure that we're not racing the
     * running of the timer queues. As an additional restraint we require the
     * timer to have a critical section associated with to be 100% there aren't
     * concurrent operations on the timer. (This latter isn't necessary any
     * longer as this isn't supported for any timers, critsect or not.)
     *
     * Note! Lock ordering doesn't apply when we only tries to
     *       get the innermost locks.
     */
    bool fOwnTMLock = RT_SUCCESS_NP(TM_TRY_LOCK_TIMERS(pVM));
#if 1
    if (    fOwnTMLock
        &&  pTimer->pCritSect)
    {
        TMTIMERSTATE enmState = pTimer->enmState;
        if (RT_LIKELY(  (   enmState == TMTIMERSTATE_EXPIRED_DELIVER
                         || enmState == TMTIMERSTATE_STOPPED)
                      && tmTimerTry(pTimer, TMTIMERSTATE_ACTIVE, enmState)))
        {
            tmTimerSetRelativeOptimizedStart(pVM, pTimer, cTicksToNext, pu64Now);
            STAM_PROFILE_STOP(&pTimer->CTX_SUFF(pVM)->tm.s.CTX_SUFF_Z(StatTimerSetRelative), a);
            return VINF_SUCCESS;
        }

        /* Optimize other states when it becomes necessary. */
    }
#endif

    /*
     * Unoptimized path.
     */
    int             rc;
    TMCLOCK const   enmClock = pTimer->enmClock;
    for (int cRetries = 1000; ; cRetries--)
    {
        /*
         * Change to any of the SET_EXPIRE states if valid and then to SCHEDULE or RESCHEDULE.
         */
        TMTIMERSTATE enmState = pTimer->enmState;
        switch (enmState)
        {
            case TMTIMERSTATE_STOPPED:
                if (enmClock == TMCLOCK_VIRTUAL_SYNC)
                {
                    /** @todo To fix assertion in tmR3TimerQueueRunVirtualSync:
                     *              Figure a safe way of activating this timer while the queue is
                     *              being run.
                     *        (99.9% sure this that the assertion is caused by DevAPIC.cpp
                     *        re-starting the timer in response to a initial_count write.) */
                }
                RT_FALL_THRU();
            case TMTIMERSTATE_EXPIRED_DELIVER:
                if (tmTimerTryWithLink(pTimer, TMTIMERSTATE_PENDING_SCHEDULE_SET_EXPIRE, enmState))
                {
                    Assert(!pTimer->offPrev);
                    Assert(!pTimer->offNext);
                    pTimer->u64Expire = cTicksToNext + tmTimerSetRelativeNowWorker(pVM, enmClock, pu64Now);
                    Log2(("TMTimerSetRelative: %p:{.enmState=%s, .pszDesc='%s', .u64Expire=%'RU64} cRetries=%d [EXP/STOP]\n",
                          pTimer, tmTimerState(enmState), R3STRING(pTimer->pszDesc), pTimer->u64Expire, cRetries));
                    TM_SET_STATE(pTimer, TMTIMERSTATE_PENDING_SCHEDULE);
                    tmSchedule(pTimer);
                    rc = VINF_SUCCESS;
                    break;
                }
                rc = VERR_TRY_AGAIN;
                break;

            case TMTIMERSTATE_PENDING_SCHEDULE:
            case TMTIMERSTATE_PENDING_STOP_SCHEDULE:
                if (tmTimerTry(pTimer, TMTIMERSTATE_PENDING_SCHEDULE_SET_EXPIRE, enmState))
                {
                    pTimer->u64Expire = cTicksToNext + tmTimerSetRelativeNowWorker(pVM, enmClock, pu64Now);
                    Log2(("TMTimerSetRelative: %p:{.enmState=%s, .pszDesc='%s', .u64Expire=%'RU64} cRetries=%d [PEND_SCHED]\n",
                          pTimer, tmTimerState(enmState), R3STRING(pTimer->pszDesc), pTimer->u64Expire, cRetries));
                    TM_SET_STATE(pTimer, TMTIMERSTATE_PENDING_SCHEDULE);
                    tmSchedule(pTimer);
                    rc = VINF_SUCCESS;
                    break;
                }
                rc = VERR_TRY_AGAIN;
                break;


            case TMTIMERSTATE_ACTIVE:
                if (tmTimerTryWithLink(pTimer, TMTIMERSTATE_PENDING_RESCHEDULE_SET_EXPIRE, enmState))
                {
                    pTimer->u64Expire = cTicksToNext + tmTimerSetRelativeNowWorker(pVM, enmClock, pu64Now);
                    Log2(("TMTimerSetRelative: %p:{.enmState=%s, .pszDesc='%s', .u64Expire=%'RU64} cRetries=%d [ACTIVE]\n",
                          pTimer, tmTimerState(enmState), R3STRING(pTimer->pszDesc), pTimer->u64Expire, cRetries));
                    TM_SET_STATE(pTimer, TMTIMERSTATE_PENDING_RESCHEDULE);
                    tmSchedule(pTimer);
                    rc = VINF_SUCCESS;
                    break;
                }
                rc = VERR_TRY_AGAIN;
                break;

            case TMTIMERSTATE_PENDING_RESCHEDULE:
            case TMTIMERSTATE_PENDING_STOP:
                if (tmTimerTry(pTimer, TMTIMERSTATE_PENDING_RESCHEDULE_SET_EXPIRE, enmState))
                {
                    pTimer->u64Expire = cTicksToNext + tmTimerSetRelativeNowWorker(pVM, enmClock, pu64Now);
                    Log2(("TMTimerSetRelative: %p:{.enmState=%s, .pszDesc='%s', .u64Expire=%'RU64} cRetries=%d [PEND_RESCH/STOP]\n",
                          pTimer, tmTimerState(enmState), R3STRING(pTimer->pszDesc), pTimer->u64Expire, cRetries));
                    TM_SET_STATE(pTimer, TMTIMERSTATE_PENDING_RESCHEDULE);
                    tmSchedule(pTimer);
                    rc = VINF_SUCCESS;
                    break;
                }
                rc = VERR_TRY_AGAIN;
                break;


            case TMTIMERSTATE_EXPIRED_GET_UNLINK:
            case TMTIMERSTATE_PENDING_SCHEDULE_SET_EXPIRE:
            case TMTIMERSTATE_PENDING_RESCHEDULE_SET_EXPIRE:
#ifdef IN_RING3
                if (!RTThreadYield())
                    RTThreadSleep(1);
#else
/** @todo call host context and yield after a couple of iterations */
#endif
                rc = VERR_TRY_AGAIN;
                break;

            /*
             * Invalid states.
             */
            case TMTIMERSTATE_DESTROY:
            case TMTIMERSTATE_FREE:
                AssertMsgFailed(("Invalid timer state %d (%s)\n", enmState, R3STRING(pTimer->pszDesc)));
                rc = VERR_TM_INVALID_STATE;
                break;

            default:
                AssertMsgFailed(("Unknown timer state %d (%s)\n", enmState, R3STRING(pTimer->pszDesc)));
                rc = VERR_TM_UNKNOWN_STATE;
                break;
        }

        /* switch + loop is tedious to break out of. */
        if (rc == VINF_SUCCESS)
            break;

        if (rc != VERR_TRY_AGAIN)
        {
            tmTimerSetRelativeNowWorker(pVM, enmClock, pu64Now);
            break;
        }
        if (cRetries <= 0)
        {
            AssertMsgFailed(("Failed waiting for stable state. state=%d (%s)\n", pTimer->enmState, R3STRING(pTimer->pszDesc)));
            rc = VERR_TM_TIMER_UNSTABLE_STATE;
            tmTimerSetRelativeNowWorker(pVM, enmClock, pu64Now);
            break;
        }

        /*
         * Retry to gain locks.
         */
        if (!fOwnTMLock)
            fOwnTMLock = RT_SUCCESS_NP(TM_TRY_LOCK_TIMERS(pVM));

    } /* for (;;) */

    /*
     * Clean up and return.
     */
    if (fOwnTMLock)
        TM_UNLOCK_TIMERS(pVM);

    STAM_PROFILE_STOP(&pTimer->CTX_SUFF(pVM)->tm.s.CTX_SUFF_Z(StatTimerSetRelative), a);
    return rc;
}


/**
 * Drops a hint about the frequency of the timer.
 *
 * This is used by TM and the VMM to calculate how often guest execution needs
 * to be interrupted.  The hint is automatically cleared by TMTimerStop.
 *
 * @returns VBox status code.
 * @param   pTimer          Timer handle as returned by one of the create
 *                          functions.
 * @param   uHzHint         The frequency hint.  Pass 0 to clear the hint.
 *
 * @remarks We're using an integer hertz value here since anything above 1 HZ
 *          is not going to be any trouble satisfying scheduling wise.  The
 *          range where it makes sense is >= 100 HZ.
 */
VMMDECL(int) TMTimerSetFrequencyHint(PTMTIMER pTimer, uint32_t uHzHint)
{
    TMTIMER_ASSERT_CRITSECT(pTimer);

    uint32_t const uHzOldHint = pTimer->uHzHint;
    pTimer->uHzHint = uHzHint;

    PVM pVM = pTimer->CTX_SUFF(pVM);
    uint32_t const uMaxHzHint = pVM->tm.s.uMaxHzHint;
    if (   uHzHint    >  uMaxHzHint
        || uHzOldHint >= uMaxHzHint)
        ASMAtomicWriteBool(&pVM->tm.s.fHzHintNeedsUpdating, true);

    return VINF_SUCCESS;
}


/**
 * TMTimerStop for the virtual sync timer queue.
 *
 * This employs a greatly simplified state machine by always acquiring the
 * queue lock and bypassing the scheduling list.
 *
 * @returns VBox status code
 * @param   pVM                 The cross context VM structure.
 * @param   pTimer              The timer handle.
 */
static int tmTimerVirtualSyncStop(PVM pVM, PTMTIMER pTimer)
{
    STAM_PROFILE_START(&pVM->tm.s.CTX_SUFF_Z(StatTimerStopVs), a);
    VM_ASSERT_EMT(pVM);
    TMTIMER_ASSERT_SYNC_CRITSECT_ORDER(pVM, pTimer);
    int rc = PDMCritSectEnter(&pVM->tm.s.VirtualSyncLock, VINF_SUCCESS);
    AssertRCReturn(rc, rc);

    /* Reset the HZ hint. */
    if (pTimer->uHzHint)
    {
        if (pTimer->uHzHint >= pVM->tm.s.uMaxHzHint)
            ASMAtomicWriteBool(&pVM->tm.s.fHzHintNeedsUpdating, true);
        pTimer->uHzHint = 0;
    }

    /* Update the timer state. */
    PTMTIMERQUEUE   pQueue   = &pVM->tm.s.CTX_SUFF(paTimerQueues)[TMCLOCK_VIRTUAL_SYNC];
    TMTIMERSTATE    enmState = pTimer->enmState;
    switch (enmState)
    {
        case TMTIMERSTATE_ACTIVE:
            tmTimerQueueUnlinkActive(pQueue, pTimer);
            TM_SET_STATE(pTimer, TMTIMERSTATE_STOPPED);
            rc = VINF_SUCCESS;
            break;

        case TMTIMERSTATE_EXPIRED_DELIVER:
            TM_SET_STATE(pTimer, TMTIMERSTATE_STOPPED);
            rc = VINF_SUCCESS;
            break;

        case TMTIMERSTATE_STOPPED:
            rc = VINF_SUCCESS;
            break;

        case TMTIMERSTATE_PENDING_RESCHEDULE:
        case TMTIMERSTATE_PENDING_STOP:
        case TMTIMERSTATE_PENDING_SCHEDULE:
        case TMTIMERSTATE_PENDING_STOP_SCHEDULE:
        case TMTIMERSTATE_EXPIRED_GET_UNLINK:
        case TMTIMERSTATE_PENDING_SCHEDULE_SET_EXPIRE:
        case TMTIMERSTATE_PENDING_RESCHEDULE_SET_EXPIRE:
        case TMTIMERSTATE_DESTROY:
        case TMTIMERSTATE_FREE:
            AssertLogRelMsgFailed(("Invalid timer state %s: %s\n", tmTimerState(enmState), R3STRING(pTimer->pszDesc)));
            rc = VERR_TM_INVALID_STATE;
            break;

        default:
            AssertMsgFailed(("Unknown timer state %d: %s\n", enmState, R3STRING(pTimer->pszDesc)));
            rc = VERR_TM_UNKNOWN_STATE;
            break;
    }

    STAM_PROFILE_STOP(&pVM->tm.s.CTX_SUFF_Z(StatTimerStopVs), a);
    PDMCritSectLeave(&pVM->tm.s.VirtualSyncLock);
    return rc;
}


/**
 * Stop the timer.
 * Use TMR3TimerArm() to "un-stop" the timer.
 *
 * @returns VBox status code.
 * @param   pTimer          Timer handle as returned by one of the create functions.
 */
VMMDECL(int) TMTimerStop(PTMTIMER pTimer)
{
    PVM pVM = pTimer->CTX_SUFF(pVM);

    /* Treat virtual sync timers specially. */
    if (pTimer->enmClock == TMCLOCK_VIRTUAL_SYNC)
        return tmTimerVirtualSyncStop(pVM, pTimer);

    STAM_PROFILE_START(&pVM->tm.s.CTX_SUFF_Z(StatTimerStop), a);
    TMTIMER_ASSERT_CRITSECT(pTimer);

    /*
     * Reset the HZ hint.
     */
    if (pTimer->uHzHint)
    {
        if (pTimer->uHzHint >= pVM->tm.s.uMaxHzHint)
            ASMAtomicWriteBool(&pVM->tm.s.fHzHintNeedsUpdating, true);
        pTimer->uHzHint = 0;
    }

    /** @todo see if this function needs optimizing. */
    int cRetries = 1000;
    do
    {
        /*
         * Change to any of the SET_EXPIRE states if valid and then to SCHEDULE or RESCHEDULE.
         */
        TMTIMERSTATE    enmState = pTimer->enmState;
        Log2(("TMTimerStop: %p:{.enmState=%s, .pszDesc='%s'} cRetries=%d\n",
              pTimer, tmTimerState(enmState), R3STRING(pTimer->pszDesc), cRetries));
        switch (enmState)
        {
            case TMTIMERSTATE_EXPIRED_DELIVER:
                //AssertMsgFailed(("You don't stop an expired timer dude!\n"));
                return VERR_INVALID_PARAMETER;

            case TMTIMERSTATE_STOPPED:
            case TMTIMERSTATE_PENDING_STOP:
            case TMTIMERSTATE_PENDING_STOP_SCHEDULE:
                STAM_PROFILE_STOP(&pVM->tm.s.CTX_SUFF_Z(StatTimerStop), a);
                return VINF_SUCCESS;

            case TMTIMERSTATE_PENDING_SCHEDULE:
                if (tmTimerTry(pTimer, TMTIMERSTATE_PENDING_STOP_SCHEDULE, enmState))
                {
                    tmSchedule(pTimer);
                    STAM_PROFILE_STOP(&pVM->tm.s.CTX_SUFF_Z(StatTimerStop), a);
                    return VINF_SUCCESS;
                }
                break;

            case TMTIMERSTATE_PENDING_RESCHEDULE:
                if (tmTimerTry(pTimer, TMTIMERSTATE_PENDING_STOP, enmState))
                {
                    tmSchedule(pTimer);
                    STAM_PROFILE_STOP(&pVM->tm.s.CTX_SUFF_Z(StatTimerStop), a);
                    return VINF_SUCCESS;
                }
                break;

            case TMTIMERSTATE_ACTIVE:
                if (tmTimerTryWithLink(pTimer, TMTIMERSTATE_PENDING_STOP, enmState))
                {
                    tmSchedule(pTimer);
                    STAM_PROFILE_STOP(&pVM->tm.s.CTX_SUFF_Z(StatTimerStop), a);
                    return VINF_SUCCESS;
                }
                break;

            case TMTIMERSTATE_EXPIRED_GET_UNLINK:
            case TMTIMERSTATE_PENDING_SCHEDULE_SET_EXPIRE:
            case TMTIMERSTATE_PENDING_RESCHEDULE_SET_EXPIRE:
#ifdef IN_RING3
                if (!RTThreadYield())
                    RTThreadSleep(1);
#else
/** @todo call host and yield cpu after a while. */
#endif
                break;

            /*
             * Invalid states.
             */
            case TMTIMERSTATE_DESTROY:
            case TMTIMERSTATE_FREE:
                AssertMsgFailed(("Invalid timer state %d (%s)\n", enmState, R3STRING(pTimer->pszDesc)));
                return VERR_TM_INVALID_STATE;
            default:
                AssertMsgFailed(("Unknown timer state %d (%s)\n", enmState, R3STRING(pTimer->pszDesc)));
                return VERR_TM_UNKNOWN_STATE;
        }
    } while (cRetries-- > 0);

    AssertMsgFailed(("Failed waiting for stable state. state=%d (%s)\n", pTimer->enmState, R3STRING(pTimer->pszDesc)));
    STAM_PROFILE_STOP(&pVM->tm.s.CTX_SUFF_Z(StatTimerStop), a);
    return VERR_TM_TIMER_UNSTABLE_STATE;
}


/**
 * Get the current clock time.
 * Handy for calculating the new expire time.
 *
 * @returns Current clock time.
 * @param   pTimer          Timer handle as returned by one of the create functions.
 */
VMMDECL(uint64_t) TMTimerGet(PTMTIMER pTimer)
{
    PVM pVM = pTimer->CTX_SUFF(pVM);

    uint64_t u64;
    switch (pTimer->enmClock)
    {
        case TMCLOCK_VIRTUAL:
            u64 = TMVirtualGet(pVM);
            break;
        case TMCLOCK_VIRTUAL_SYNC:
            u64 = TMVirtualSyncGet(pVM);
            break;
        case TMCLOCK_REAL:
            u64 = TMRealGet(pVM);
            break;
        default:
            AssertMsgFailed(("Invalid enmClock=%d\n", pTimer->enmClock));
            return UINT64_MAX;
    }
    //Log2(("TMTimerGet: returns %'RU64 (pTimer=%p:{.enmState=%s, .pszDesc='%s'})\n",
    //      u64, pTimer, tmTimerState(pTimer->enmState), R3STRING(pTimer->pszDesc)));
    return u64;
}


/**
 * Get the frequency of the timer clock.
 *
 * @returns Clock frequency (as Hz of course).
 * @param   pTimer          Timer handle as returned by one of the create functions.
 */
VMMDECL(uint64_t) TMTimerGetFreq(PTMTIMER pTimer)
{
    switch (pTimer->enmClock)
    {
        case TMCLOCK_VIRTUAL:
        case TMCLOCK_VIRTUAL_SYNC:
            return TMCLOCK_FREQ_VIRTUAL;

        case TMCLOCK_REAL:
            return TMCLOCK_FREQ_REAL;

        default:
            AssertMsgFailed(("Invalid enmClock=%d\n", pTimer->enmClock));
            return 0;
    }
}


/**
 * Get the expire time of the timer.
 * Only valid for active timers.
 *
 * @returns Expire time of the timer.
 * @param   pTimer          Timer handle as returned by one of the create functions.
 */
VMMDECL(uint64_t) TMTimerGetExpire(PTMTIMER pTimer)
{
    TMTIMER_ASSERT_CRITSECT(pTimer);
    int cRetries = 1000;
    do
    {
        TMTIMERSTATE    enmState = pTimer->enmState;
        switch (enmState)
        {
            case TMTIMERSTATE_EXPIRED_GET_UNLINK:
            case TMTIMERSTATE_EXPIRED_DELIVER:
            case TMTIMERSTATE_STOPPED:
            case TMTIMERSTATE_PENDING_STOP:
            case TMTIMERSTATE_PENDING_STOP_SCHEDULE:
                Log2(("TMTimerGetExpire: returns ~0 (pTimer=%p:{.enmState=%s, .pszDesc='%s'})\n",
                      pTimer, tmTimerState(pTimer->enmState), R3STRING(pTimer->pszDesc)));
                return ~(uint64_t)0;

            case TMTIMERSTATE_ACTIVE:
            case TMTIMERSTATE_PENDING_RESCHEDULE:
            case TMTIMERSTATE_PENDING_SCHEDULE:
                Log2(("TMTimerGetExpire: returns %'RU64 (pTimer=%p:{.enmState=%s, .pszDesc='%s'})\n",
                      pTimer->u64Expire, pTimer, tmTimerState(pTimer->enmState), R3STRING(pTimer->pszDesc)));
                return pTimer->u64Expire;

            case TMTIMERSTATE_PENDING_SCHEDULE_SET_EXPIRE:
            case TMTIMERSTATE_PENDING_RESCHEDULE_SET_EXPIRE:
#ifdef IN_RING3
                if (!RTThreadYield())
                    RTThreadSleep(1);
#endif
                break;

            /*
             * Invalid states.
             */
            case TMTIMERSTATE_DESTROY:
            case TMTIMERSTATE_FREE:
                AssertMsgFailed(("Invalid timer state %d (%s)\n", enmState, R3STRING(pTimer->pszDesc)));
                Log2(("TMTimerGetExpire: returns ~0 (pTimer=%p:{.enmState=%s, .pszDesc='%s'})\n",
                      pTimer, tmTimerState(pTimer->enmState), R3STRING(pTimer->pszDesc)));
                return ~(uint64_t)0;
            default:
                AssertMsgFailed(("Unknown timer state %d (%s)\n", enmState, R3STRING(pTimer->pszDesc)));
                return ~(uint64_t)0;
        }
    } while (cRetries-- > 0);

    AssertMsgFailed(("Failed waiting for stable state. state=%d (%s)\n", pTimer->enmState, R3STRING(pTimer->pszDesc)));
    Log2(("TMTimerGetExpire: returns ~0 (pTimer=%p:{.enmState=%s, .pszDesc='%s'})\n",
          pTimer, tmTimerState(pTimer->enmState), R3STRING(pTimer->pszDesc)));
    return ~(uint64_t)0;
}


/**
 * Checks if a timer is active or not.
 *
 * @returns True if active.
 * @returns False if not active.
 * @param   pTimer          Timer handle as returned by one of the create functions.
 */
VMMDECL(bool) TMTimerIsActive(PTMTIMER pTimer)
{
    TMTIMERSTATE enmState = pTimer->enmState;
    switch (enmState)
    {
        case TMTIMERSTATE_STOPPED:
        case TMTIMERSTATE_EXPIRED_GET_UNLINK:
        case TMTIMERSTATE_EXPIRED_DELIVER:
        case TMTIMERSTATE_PENDING_STOP:
        case TMTIMERSTATE_PENDING_STOP_SCHEDULE:
            Log2(("TMTimerIsActive: returns false (pTimer=%p:{.enmState=%s, .pszDesc='%s'})\n",
                  pTimer, tmTimerState(pTimer->enmState), R3STRING(pTimer->pszDesc)));
            return false;

        case TMTIMERSTATE_ACTIVE:
        case TMTIMERSTATE_PENDING_RESCHEDULE:
        case TMTIMERSTATE_PENDING_SCHEDULE:
        case TMTIMERSTATE_PENDING_SCHEDULE_SET_EXPIRE:
        case TMTIMERSTATE_PENDING_RESCHEDULE_SET_EXPIRE:
            Log2(("TMTimerIsActive: returns true (pTimer=%p:{.enmState=%s, .pszDesc='%s'})\n",
                  pTimer, tmTimerState(pTimer->enmState), R3STRING(pTimer->pszDesc)));
            return true;

        /*
         * Invalid states.
         */
        case TMTIMERSTATE_DESTROY:
        case TMTIMERSTATE_FREE:
            AssertMsgFailed(("Invalid timer state %s (%s)\n", tmTimerState(enmState), R3STRING(pTimer->pszDesc)));
            Log2(("TMTimerIsActive: returns false (pTimer=%p:{.enmState=%s, .pszDesc='%s'})\n",
                  pTimer, tmTimerState(pTimer->enmState), R3STRING(pTimer->pszDesc)));
            return false;
        default:
            AssertMsgFailed(("Unknown timer state %d (%s)\n", enmState, R3STRING(pTimer->pszDesc)));
            return false;
    }
}


/* -=-=-=-=-=-=- Convenience APIs -=-=-=-=-=-=- */


/**
 * Arm a timer with a (new) expire time relative to current time.
 *
 * @returns VBox status code.
 * @param   pTimer          Timer handle as returned by one of the create functions.
 * @param   cMilliesToNext  Number of milliseconds to the next tick.
 */
VMMDECL(int) TMTimerSetMillies(PTMTIMER pTimer, uint32_t cMilliesToNext)
{
    switch (pTimer->enmClock)
    {
        case TMCLOCK_VIRTUAL:
            AssertCompile(TMCLOCK_FREQ_VIRTUAL == 1000000000);
            return TMTimerSetRelative(pTimer, cMilliesToNext * UINT64_C(1000000), NULL);

        case TMCLOCK_VIRTUAL_SYNC:
            AssertCompile(TMCLOCK_FREQ_VIRTUAL == 1000000000);
            return TMTimerSetRelative(pTimer, cMilliesToNext * UINT64_C(1000000), NULL);

        case TMCLOCK_REAL:
            AssertCompile(TMCLOCK_FREQ_REAL == 1000);
            return TMTimerSetRelative(pTimer, cMilliesToNext, NULL);

        default:
            AssertMsgFailed(("Invalid enmClock=%d\n", pTimer->enmClock));
            return VERR_TM_TIMER_BAD_CLOCK;
    }
}


/**
 * Arm a timer with a (new) expire time relative to current time.
 *
 * @returns VBox status code.
 * @param   pTimer          Timer handle as returned by one of the create functions.
 * @param   cMicrosToNext   Number of microseconds to the next tick.
 */
VMMDECL(int) TMTimerSetMicro(PTMTIMER pTimer, uint64_t cMicrosToNext)
{
    switch (pTimer->enmClock)
    {
        case TMCLOCK_VIRTUAL:
            AssertCompile(TMCLOCK_FREQ_VIRTUAL == 1000000000);
            return TMTimerSetRelative(pTimer, cMicrosToNext * 1000, NULL);

        case TMCLOCK_VIRTUAL_SYNC:
            AssertCompile(TMCLOCK_FREQ_VIRTUAL == 1000000000);
            return TMTimerSetRelative(pTimer, cMicrosToNext * 1000, NULL);

        case TMCLOCK_REAL:
            AssertCompile(TMCLOCK_FREQ_REAL == 1000);
            return TMTimerSetRelative(pTimer, cMicrosToNext / 1000, NULL);

        default:
            AssertMsgFailed(("Invalid enmClock=%d\n", pTimer->enmClock));
            return VERR_TM_TIMER_BAD_CLOCK;
    }
}


/**
 * Arm a timer with a (new) expire time relative to current time.
 *
 * @returns VBox status code.
 * @param   pTimer          Timer handle as returned by one of the create functions.
 * @param   cNanosToNext    Number of nanoseconds to the next tick.
 */
VMMDECL(int) TMTimerSetNano(PTMTIMER pTimer, uint64_t cNanosToNext)
{
    switch (pTimer->enmClock)
    {
        case TMCLOCK_VIRTUAL:
            AssertCompile(TMCLOCK_FREQ_VIRTUAL == 1000000000);
            return TMTimerSetRelative(pTimer, cNanosToNext, NULL);

        case TMCLOCK_VIRTUAL_SYNC:
            AssertCompile(TMCLOCK_FREQ_VIRTUAL == 1000000000);
            return TMTimerSetRelative(pTimer, cNanosToNext, NULL);

        case TMCLOCK_REAL:
            AssertCompile(TMCLOCK_FREQ_REAL == 1000);
            return TMTimerSetRelative(pTimer, cNanosToNext / 1000000, NULL);

        default:
            AssertMsgFailed(("Invalid enmClock=%d\n", pTimer->enmClock));
            return VERR_TM_TIMER_BAD_CLOCK;
    }
}


/**
 * Get the current clock time as nanoseconds.
 *
 * @returns The timer clock as nanoseconds.
 * @param   pTimer          Timer handle as returned by one of the create functions.
 */
VMMDECL(uint64_t) TMTimerGetNano(PTMTIMER pTimer)
{
    return TMTimerToNano(pTimer, TMTimerGet(pTimer));
}


/**
 * Get the current clock time as microseconds.
 *
 * @returns The timer clock as microseconds.
 * @param   pTimer          Timer handle as returned by one of the create functions.
 */
VMMDECL(uint64_t) TMTimerGetMicro(PTMTIMER pTimer)
{
    return TMTimerToMicro(pTimer, TMTimerGet(pTimer));
}


/**
 * Get the current clock time as milliseconds.
 *
 * @returns The timer clock as milliseconds.
 * @param   pTimer          Timer handle as returned by one of the create functions.
 */
VMMDECL(uint64_t) TMTimerGetMilli(PTMTIMER pTimer)
{
    return TMTimerToMilli(pTimer, TMTimerGet(pTimer));
}


/**
 * Converts the specified timer clock time to nanoseconds.
 *
 * @returns nanoseconds.
 * @param   pTimer          Timer handle as returned by one of the create functions.
 * @param   u64Ticks        The clock ticks.
 * @remark  There could be rounding errors here. We just do a simple integer divide
 *          without any adjustments.
 */
VMMDECL(uint64_t) TMTimerToNano(PTMTIMER pTimer, uint64_t u64Ticks)
{
    switch (pTimer->enmClock)
    {
        case TMCLOCK_VIRTUAL:
        case TMCLOCK_VIRTUAL_SYNC:
            AssertCompile(TMCLOCK_FREQ_VIRTUAL == 1000000000);
            return u64Ticks;

        case TMCLOCK_REAL:
            AssertCompile(TMCLOCK_FREQ_REAL == 1000);
            return u64Ticks * 1000000;

        default:
            AssertMsgFailed(("Invalid enmClock=%d\n", pTimer->enmClock));
            return 0;
    }
}


/**
 * Converts the specified timer clock time to microseconds.
 *
 * @returns microseconds.
 * @param   pTimer          Timer handle as returned by one of the create functions.
 * @param   u64Ticks        The clock ticks.
 * @remark  There could be rounding errors here. We just do a simple integer divide
 *          without any adjustments.
 */
VMMDECL(uint64_t) TMTimerToMicro(PTMTIMER pTimer, uint64_t u64Ticks)
{
    switch (pTimer->enmClock)
    {
        case TMCLOCK_VIRTUAL:
        case TMCLOCK_VIRTUAL_SYNC:
            AssertCompile(TMCLOCK_FREQ_VIRTUAL == 1000000000);
            return u64Ticks / 1000;

        case TMCLOCK_REAL:
            AssertCompile(TMCLOCK_FREQ_REAL == 1000);
            return u64Ticks * 1000;

        default:
            AssertMsgFailed(("Invalid enmClock=%d\n", pTimer->enmClock));
            return 0;
    }
}


/**
 * Converts the specified timer clock time to milliseconds.
 *
 * @returns milliseconds.
 * @param   pTimer          Timer handle as returned by one of the create functions.
 * @param   u64Ticks        The clock ticks.
 * @remark  There could be rounding errors here. We just do a simple integer divide
 *          without any adjustments.
 */
VMMDECL(uint64_t) TMTimerToMilli(PTMTIMER pTimer, uint64_t u64Ticks)
{
    switch (pTimer->enmClock)
    {
        case TMCLOCK_VIRTUAL:
        case TMCLOCK_VIRTUAL_SYNC:
            AssertCompile(TMCLOCK_FREQ_VIRTUAL == 1000000000);
            return u64Ticks / 1000000;

        case TMCLOCK_REAL:
            AssertCompile(TMCLOCK_FREQ_REAL == 1000);
            return u64Ticks;

        default:
            AssertMsgFailed(("Invalid enmClock=%d\n", pTimer->enmClock));
            return 0;
    }
}


/**
 * Converts the specified nanosecond timestamp to timer clock ticks.
 *
 * @returns timer clock ticks.
 * @param   pTimer          Timer handle as returned by one of the create functions.
 * @param   cNanoSecs       The nanosecond value ticks to convert.
 * @remark  There could be rounding and overflow errors here.
 */
VMMDECL(uint64_t) TMTimerFromNano(PTMTIMER pTimer, uint64_t cNanoSecs)
{
    switch (pTimer->enmClock)
    {
        case TMCLOCK_VIRTUAL:
        case TMCLOCK_VIRTUAL_SYNC:
            AssertCompile(TMCLOCK_FREQ_VIRTUAL == 1000000000);
            return cNanoSecs;

        case TMCLOCK_REAL:
            AssertCompile(TMCLOCK_FREQ_REAL == 1000);
            return cNanoSecs / 1000000;

        default:
            AssertMsgFailed(("Invalid enmClock=%d\n", pTimer->enmClock));
            return 0;
    }
}


/**
 * Converts the specified microsecond timestamp to timer clock ticks.
 *
 * @returns timer clock ticks.
 * @param   pTimer          Timer handle as returned by one of the create functions.
 * @param   cMicroSecs      The microsecond value ticks to convert.
 * @remark  There could be rounding and overflow errors here.
 */
VMMDECL(uint64_t) TMTimerFromMicro(PTMTIMER pTimer, uint64_t cMicroSecs)
{
    switch (pTimer->enmClock)
    {
        case TMCLOCK_VIRTUAL:
        case TMCLOCK_VIRTUAL_SYNC:
            AssertCompile(TMCLOCK_FREQ_VIRTUAL == 1000000000);
            return cMicroSecs * 1000;

        case TMCLOCK_REAL:
            AssertCompile(TMCLOCK_FREQ_REAL == 1000);
            return cMicroSecs / 1000;

        default:
            AssertMsgFailed(("Invalid enmClock=%d\n", pTimer->enmClock));
            return 0;
    }
}


/**
 * Converts the specified millisecond timestamp to timer clock ticks.
 *
 * @returns timer clock ticks.
 * @param   pTimer          Timer handle as returned by one of the create functions.
 * @param   cMilliSecs      The millisecond value ticks to convert.
 * @remark  There could be rounding and overflow errors here.
 */
VMMDECL(uint64_t) TMTimerFromMilli(PTMTIMER pTimer, uint64_t cMilliSecs)
{
    switch (pTimer->enmClock)
    {
        case TMCLOCK_VIRTUAL:
        case TMCLOCK_VIRTUAL_SYNC:
            AssertCompile(TMCLOCK_FREQ_VIRTUAL == 1000000000);
            return cMilliSecs * 1000000;

        case TMCLOCK_REAL:
            AssertCompile(TMCLOCK_FREQ_REAL == 1000);
            return cMilliSecs;

        default:
            AssertMsgFailed(("Invalid enmClock=%d\n", pTimer->enmClock));
            return 0;
    }
}


/**
 * Convert state to string.
 *
 * @returns Readonly status name.
 * @param   enmState    State.
 */
const char *tmTimerState(TMTIMERSTATE enmState)
{
    switch (enmState)
    {
#define CASE(num, state) \
            case TMTIMERSTATE_##state: \
                AssertCompile(TMTIMERSTATE_##state == (num)); \
                return #num "-" #state
        CASE( 1,STOPPED);
        CASE( 2,ACTIVE);
        CASE( 3,EXPIRED_GET_UNLINK);
        CASE( 4,EXPIRED_DELIVER);
        CASE( 5,PENDING_STOP);
        CASE( 6,PENDING_STOP_SCHEDULE);
        CASE( 7,PENDING_SCHEDULE_SET_EXPIRE);
        CASE( 8,PENDING_SCHEDULE);
        CASE( 9,PENDING_RESCHEDULE_SET_EXPIRE);
        CASE(10,PENDING_RESCHEDULE);
        CASE(11,DESTROY);
        CASE(12,FREE);
        default:
            AssertMsgFailed(("Invalid state enmState=%d\n", enmState));
            return "Invalid state!";
#undef CASE
    }
}


/**
 * Gets the highest frequency hint for all the important timers.
 *
 * @returns The highest frequency.  0 if no timers care.
 * @param   pVM         The cross context VM structure.
 */
static uint32_t tmGetFrequencyHint(PVM pVM)
{
    /*
     * Query the value, recalculate it if necessary.
     *
     * The "right" highest frequency value isn't so important that we'll block
     * waiting on the timer semaphore.
     */
    uint32_t uMaxHzHint = ASMAtomicUoReadU32(&pVM->tm.s.uMaxHzHint);
    if (RT_UNLIKELY(ASMAtomicReadBool(&pVM->tm.s.fHzHintNeedsUpdating)))
    {
        if (RT_SUCCESS(TM_TRY_LOCK_TIMERS(pVM)))
        {
            ASMAtomicWriteBool(&pVM->tm.s.fHzHintNeedsUpdating, false);

            /*
             * Loop over the timers associated with each clock.
             */
            uMaxHzHint = 0;
            for (int i = 0; i < TMCLOCK_MAX; i++)
            {
                PTMTIMERQUEUE pQueue = &pVM->tm.s.CTX_SUFF(paTimerQueues)[i];
                for (PTMTIMER pCur = TMTIMER_GET_HEAD(pQueue); pCur; pCur = TMTIMER_GET_NEXT(pCur))
                {
                    uint32_t uHzHint = ASMAtomicUoReadU32(&pCur->uHzHint);
                    if (uHzHint > uMaxHzHint)
                    {
                        switch (pCur->enmState)
                        {
                            case TMTIMERSTATE_ACTIVE:
                            case TMTIMERSTATE_EXPIRED_GET_UNLINK:
                            case TMTIMERSTATE_EXPIRED_DELIVER:
                            case TMTIMERSTATE_PENDING_SCHEDULE_SET_EXPIRE:
                            case TMTIMERSTATE_PENDING_SCHEDULE:
                            case TMTIMERSTATE_PENDING_RESCHEDULE_SET_EXPIRE:
                            case TMTIMERSTATE_PENDING_RESCHEDULE:
                                uMaxHzHint = uHzHint;
                                break;

                            case TMTIMERSTATE_STOPPED:
                            case TMTIMERSTATE_PENDING_STOP:
                            case TMTIMERSTATE_PENDING_STOP_SCHEDULE:
                            case TMTIMERSTATE_DESTROY:
                            case TMTIMERSTATE_FREE:
                                break;
                            /* no default, want gcc warnings when adding more states. */
                        }
                    }
                }
            }
            ASMAtomicWriteU32(&pVM->tm.s.uMaxHzHint, uMaxHzHint);
            Log(("tmGetFrequencyHint: New value %u Hz\n", uMaxHzHint));
            TM_UNLOCK_TIMERS(pVM);
        }
    }
    return uMaxHzHint;
}


/**
 * Calculates a host timer frequency that would be suitable for the current
 * timer load.
 *
 * This will take the highest timer frequency, adjust for catch-up and warp
 * driver, and finally add a little fudge factor.  The caller (VMM) will use
 * the result to adjust the per-cpu preemption timer.
 *
 * @returns The highest frequency.  0 if no important timers around.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 */
VMM_INT_DECL(uint32_t) TMCalcHostTimerFrequency(PVM pVM, PVMCPU pVCpu)
{
    uint32_t uHz = tmGetFrequencyHint(pVM);

    /* Catch up, we have to be more aggressive than the % indicates at the
       beginning of the effort. */
    if (ASMAtomicUoReadBool(&pVM->tm.s.fVirtualSyncCatchUp))
    {
        uint32_t u32Pct = ASMAtomicReadU32(&pVM->tm.s.u32VirtualSyncCatchUpPercentage);
        if (ASMAtomicReadBool(&pVM->tm.s.fVirtualSyncCatchUp))
        {
            if (u32Pct <= 100)
                u32Pct = u32Pct * pVM->tm.s.cPctHostHzFudgeFactorCatchUp100 / 100;
            else if (u32Pct <= 200)
                u32Pct = u32Pct * pVM->tm.s.cPctHostHzFudgeFactorCatchUp200 / 100;
            else if (u32Pct <= 400)
                u32Pct = u32Pct * pVM->tm.s.cPctHostHzFudgeFactorCatchUp400 / 100;
            uHz *= u32Pct + 100;
            uHz /= 100;
        }
    }

    /* Warp drive. */
    if (ASMAtomicUoReadBool(&pVM->tm.s.fVirtualWarpDrive))
    {
        uint32_t u32Pct = ASMAtomicReadU32(&pVM->tm.s.u32VirtualWarpDrivePercentage);
        if (ASMAtomicReadBool(&pVM->tm.s.fVirtualWarpDrive))
        {
            uHz *= u32Pct;
            uHz /= 100;
        }
    }

    /* Fudge factor. */
    if (pVCpu->idCpu == pVM->tm.s.idTimerCpu)
        uHz *= pVM->tm.s.cPctHostHzFudgeFactorTimerCpu;
    else
        uHz *= pVM->tm.s.cPctHostHzFudgeFactorOtherCpu;
    uHz /= 100;

    /* Make sure it isn't too high. */
    if (uHz > pVM->tm.s.cHostHzMax)
        uHz = pVM->tm.s.cHostHzMax;

    return uHz;
}


/**
 * Whether the guest virtual clock is ticking.
 *
 * @returns true if ticking, false otherwise.
 * @param   pVM     The cross context VM structure.
 */
VMM_INT_DECL(bool) TMVirtualIsTicking(PVM pVM)
{
    return RT_BOOL(pVM->tm.s.cVirtualTicking);
}

