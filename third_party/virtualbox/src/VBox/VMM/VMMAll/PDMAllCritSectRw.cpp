/* $Id: PDMAllCritSectRw.cpp $ */
/** @file
 * IPRT - Read/Write Critical Section, Generic.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_PDM//_CRITSECT
#include "PDMInternal.h"
#include <VBox/vmm/pdmcritsectrw.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/vmm.h>
#include <VBox/vmm/vm.h>
#include <VBox/err.h>
#include <VBox/vmm/hm.h>

#include <VBox/log.h>
#include <iprt/asm.h>
#include <iprt/asm-amd64-x86.h>
#include <iprt/assert.h>
#ifdef IN_RING3
# include <iprt/lockvalidator.h>
# include <iprt/semaphore.h>
#endif
#if defined(IN_RING3) || defined(IN_RING0)
# include <iprt/thread.h>
#endif


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** The number loops to spin for shared access in ring-3. */
#define PDMCRITSECTRW_SHRD_SPIN_COUNT_R3       20
/** The number loops to spin for shared access in ring-0. */
#define PDMCRITSECTRW_SHRD_SPIN_COUNT_R0       128
/** The number loops to spin for shared access in the raw-mode context. */
#define PDMCRITSECTRW_SHRD_SPIN_COUNT_RC       128

/** The number loops to spin for exclusive access in ring-3. */
#define PDMCRITSECTRW_EXCL_SPIN_COUNT_R3       20
/** The number loops to spin for exclusive access in ring-0. */
#define PDMCRITSECTRW_EXCL_SPIN_COUNT_R0       256
/** The number loops to spin for exclusive access in the raw-mode context. */
#define PDMCRITSECTRW_EXCL_SPIN_COUNT_RC       256


/* Undefine the automatic VBOX_STRICT API mappings. */
#undef PDMCritSectRwEnterExcl
#undef PDMCritSectRwTryEnterExcl
#undef PDMCritSectRwEnterShared
#undef PDMCritSectRwTryEnterShared


/**
 * Gets the ring-3 native thread handle of the calling thread.
 *
 * @returns native thread handle (ring-3).
 * @param   pThis       The read/write critical section.  This is only used in
 *                      R0 and RC.
 */
DECL_FORCE_INLINE(RTNATIVETHREAD) pdmCritSectRwGetNativeSelf(PCPDMCRITSECTRW pThis)
{
#ifdef IN_RING3
    NOREF(pThis);
    RTNATIVETHREAD  hNativeSelf = RTThreadNativeSelf();
#else
    AssertMsgReturn(pThis->s.Core.u32Magic == RTCRITSECTRW_MAGIC, ("%RX32\n", pThis->s.Core.u32Magic),
                    NIL_RTNATIVETHREAD);
    PVM             pVM         = pThis->s.CTX_SUFF(pVM);   AssertPtr(pVM);
    PVMCPU          pVCpu       = VMMGetCpu(pVM);           AssertPtr(pVCpu);
    RTNATIVETHREAD  hNativeSelf = pVCpu->hNativeThread;     Assert(hNativeSelf != NIL_RTNATIVETHREAD);
#endif
    return hNativeSelf;
}





#ifdef IN_RING3
/**
 * Changes the lock validator sub-class of the read/write critical section.
 *
 * It is recommended to try make sure that nobody is using this critical section
 * while changing the value.
 *
 * @returns The old sub-class.  RTLOCKVAL_SUB_CLASS_INVALID is returns if the
 *          lock validator isn't compiled in or either of the parameters are
 *          invalid.
 * @param   pThis           Pointer to the read/write critical section.
 * @param   uSubClass       The new sub-class value.
 */
VMMDECL(uint32_t) PDMR3CritSectRwSetSubClass(PPDMCRITSECTRW pThis, uint32_t uSubClass)
{
    AssertPtrReturn(pThis, RTLOCKVAL_SUB_CLASS_INVALID);
    AssertReturn(pThis->s.Core.u32Magic == RTCRITSECTRW_MAGIC, RTLOCKVAL_SUB_CLASS_INVALID);
# if defined(PDMCRITSECTRW_STRICT) && defined(IN_RING3)
    AssertReturn(!(pThis->s.Core.fFlags & RTCRITSECT_FLAGS_NOP), RTLOCKVAL_SUB_CLASS_INVALID);

    RTLockValidatorRecSharedSetSubClass(pThis->s.Core.pValidatorRead, uSubClass);
    return RTLockValidatorRecExclSetSubClass(pThis->s.Core.pValidatorWrite, uSubClass);
# else
    NOREF(uSubClass);
    return RTLOCKVAL_SUB_CLASS_INVALID;
# endif
}
#endif /* IN_RING3 */


#ifdef IN_RING0
/**
 * Go back to ring-3 so the kernel can do signals, APCs and other fun things.
 *
 * @param   pThis       Pointer to the read/write critical section.
 */
static void pdmR0CritSectRwYieldToRing3(PPDMCRITSECTRW pThis)
{
    PVM     pVM   = pThis->s.CTX_SUFF(pVM);     AssertPtr(pVM);
    PVMCPU  pVCpu = VMMGetCpu(pVM);             AssertPtr(pVCpu);
    int rc = VMMRZCallRing3(pVM, pVCpu, VMMCALLRING3_VM_R0_PREEMPT, NULL);
    AssertRC(rc);
}
#endif /* IN_RING0 */


/**
 * Worker that enters a read/write critical section with shard access.
 *
 * @returns VBox status code.
 * @param   pThis       Pointer to the read/write critical section.
 * @param   rcBusy      The busy return code for ring-0 and ring-3.
 * @param   fTryOnly    Only try enter it, don't wait.
 * @param   pSrcPos     The source position. (Can be NULL.)
 * @param   fNoVal      No validation records.
 */
static int pdmCritSectRwEnterShared(PPDMCRITSECTRW pThis, int rcBusy, bool fTryOnly, PCRTLOCKVALSRCPOS pSrcPos, bool fNoVal)
{
    /*
     * Validate input.
     */
    AssertPtr(pThis);
    AssertReturn(pThis->s.Core.u32Magic == RTCRITSECTRW_MAGIC, VERR_SEM_DESTROYED);

#if !defined(PDMCRITSECTRW_STRICT) || !defined(IN_RING3)
    NOREF(pSrcPos);
    NOREF(fNoVal);
#endif
#ifdef IN_RING3
    NOREF(rcBusy);
#endif

#if defined(PDMCRITSECTRW_STRICT) && defined(IN_RING3)
    RTTHREAD hThreadSelf = RTThreadSelfAutoAdopt();
    if (!fTryOnly)
    {
        int            rc9;
        RTNATIVETHREAD hNativeWriter;
        ASMAtomicUoReadHandle(&pThis->s.Core.hNativeWriter, &hNativeWriter);
        if (hNativeWriter != NIL_RTTHREAD && hNativeWriter == pdmCritSectRwGetNativeSelf(pThis))
            rc9 = RTLockValidatorRecExclCheckOrder(pThis->s.Core.pValidatorWrite, hThreadSelf, pSrcPos, RT_INDEFINITE_WAIT);
        else
            rc9 = RTLockValidatorRecSharedCheckOrder(pThis->s.Core.pValidatorRead, hThreadSelf, pSrcPos, RT_INDEFINITE_WAIT);
        if (RT_FAILURE(rc9))
            return rc9;
    }
#endif

    /*
     * Get cracking...
     */
    uint64_t u64State    = ASMAtomicReadU64(&pThis->s.Core.u64State);
    uint64_t u64OldState = u64State;

    for (;;)
    {
        if ((u64State & RTCSRW_DIR_MASK) == (RTCSRW_DIR_READ << RTCSRW_DIR_SHIFT))
        {
            /* It flows in the right direction, try follow it before it changes. */
            uint64_t c = (u64State & RTCSRW_CNT_RD_MASK) >> RTCSRW_CNT_RD_SHIFT;
            c++;
            Assert(c < RTCSRW_CNT_MASK / 2);
            u64State &= ~RTCSRW_CNT_RD_MASK;
            u64State |= c << RTCSRW_CNT_RD_SHIFT;
            if (ASMAtomicCmpXchgU64(&pThis->s.Core.u64State, u64State, u64OldState))
            {
#if defined(PDMCRITSECTRW_STRICT) && defined(IN_RING3)
                if (!fNoVal)
                    RTLockValidatorRecSharedAddOwner(pThis->s.Core.pValidatorRead, hThreadSelf, pSrcPos);
#endif
                break;
            }
        }
        else if ((u64State & (RTCSRW_CNT_RD_MASK | RTCSRW_CNT_WR_MASK)) == 0)
        {
            /* Wrong direction, but we're alone here and can simply try switch the direction. */
            u64State &= ~(RTCSRW_CNT_RD_MASK | RTCSRW_CNT_WR_MASK | RTCSRW_DIR_MASK);
            u64State |= (UINT64_C(1) << RTCSRW_CNT_RD_SHIFT) | (RTCSRW_DIR_READ << RTCSRW_DIR_SHIFT);
            if (ASMAtomicCmpXchgU64(&pThis->s.Core.u64State, u64State, u64OldState))
            {
                Assert(!pThis->s.Core.fNeedReset);
#if defined(PDMCRITSECTRW_STRICT) && defined(IN_RING3)
                if (!fNoVal)
                    RTLockValidatorRecSharedAddOwner(pThis->s.Core.pValidatorRead, hThreadSelf, pSrcPos);
#endif
                break;
            }
        }
        else
        {
            /* Is the writer perhaps doing a read recursion? */
            RTNATIVETHREAD hNativeSelf = pdmCritSectRwGetNativeSelf(pThis);
            RTNATIVETHREAD hNativeWriter;
            ASMAtomicUoReadHandle(&pThis->s.Core.hNativeWriter, &hNativeWriter);
            if (hNativeSelf == hNativeWriter)
            {
#if defined(PDMCRITSECTRW_STRICT) && defined(IN_RING3)
                if (!fNoVal)
                {
                    int rc9 = RTLockValidatorRecExclRecursionMixed(pThis->s.Core.pValidatorWrite, &pThis->s.Core.pValidatorRead->Core, pSrcPos);
                    if (RT_FAILURE(rc9))
                        return rc9;
                }
#endif
                Assert(pThis->s.Core.cWriterReads < UINT32_MAX / 2);
                ASMAtomicIncU32(&pThis->s.Core.cWriterReads);
                STAM_REL_COUNTER_INC(&pThis->s.CTX_MID_Z(Stat,EnterShared));
                return VINF_SUCCESS; /* don't break! */
            }

            /*
             * If we're only trying, return already.
             */
            if (fTryOnly)
            {
                STAM_REL_COUNTER_INC(&pThis->s.CTX_MID_Z(StatContention,EnterShared));
                return VERR_SEM_BUSY;
            }

#if defined(IN_RING3) || defined(IN_RING0)
# ifdef IN_RING0
            if (   RTThreadPreemptIsEnabled(NIL_RTTHREAD)
                && ASMIntAreEnabled())
# endif
            {
                /*
                 * Add ourselves to the queue and wait for the direction to change.
                 */
                uint64_t c = (u64State & RTCSRW_CNT_RD_MASK) >> RTCSRW_CNT_RD_SHIFT;
                c++;
                Assert(c < RTCSRW_CNT_MASK / 2);

                uint64_t cWait = (u64State & RTCSRW_WAIT_CNT_RD_MASK) >> RTCSRW_WAIT_CNT_RD_SHIFT;
                cWait++;
                Assert(cWait <= c);
                Assert(cWait < RTCSRW_CNT_MASK / 2);

                u64State &= ~(RTCSRW_CNT_RD_MASK | RTCSRW_WAIT_CNT_RD_MASK);
                u64State |= (c << RTCSRW_CNT_RD_SHIFT) | (cWait << RTCSRW_WAIT_CNT_RD_SHIFT);

                if (ASMAtomicCmpXchgU64(&pThis->s.Core.u64State, u64State, u64OldState))
                {
                    for (uint32_t iLoop = 0; ; iLoop++)
                    {
                        int rc;
# ifdef IN_RING3
#  if defined(PDMCRITSECTRW_STRICT) && defined(IN_RING3)
                        rc = RTLockValidatorRecSharedCheckBlocking(pThis->s.Core.pValidatorRead, hThreadSelf, pSrcPos, true,
                                                                   RT_INDEFINITE_WAIT, RTTHREADSTATE_RW_READ, false);
                        if (RT_SUCCESS(rc))
#  else
                        RTTHREAD hThreadSelf = RTThreadSelf();
                        RTThreadBlocking(hThreadSelf, RTTHREADSTATE_RW_READ, false);
#  endif
# endif
                        {
                            for (;;)
                            {
                                rc = SUPSemEventMultiWaitNoResume(pThis->s.CTX_SUFF(pVM)->pSession,
                                                                  (SUPSEMEVENTMULTI)pThis->s.Core.hEvtRead,
                                                                  RT_INDEFINITE_WAIT);
                                if (   rc != VERR_INTERRUPTED
                                    || pThis->s.Core.u32Magic != RTCRITSECTRW_MAGIC)
                                    break;
# ifdef IN_RING0
                                pdmR0CritSectRwYieldToRing3(pThis);
# endif
                            }
# ifdef IN_RING3
                            RTThreadUnblocked(hThreadSelf, RTTHREADSTATE_RW_READ);
# endif
                            if (pThis->s.Core.u32Magic != RTCRITSECTRW_MAGIC)
                                return VERR_SEM_DESTROYED;
                        }
                        if (RT_FAILURE(rc))
                        {
                            /* Decrement the counts and return the error. */
                            for (;;)
                            {
                                u64OldState = u64State = ASMAtomicReadU64(&pThis->s.Core.u64State);
                                c = (u64State & RTCSRW_CNT_RD_MASK) >> RTCSRW_CNT_RD_SHIFT; Assert(c > 0);
                                c--;
                                cWait = (u64State & RTCSRW_WAIT_CNT_RD_MASK) >> RTCSRW_WAIT_CNT_RD_SHIFT; Assert(cWait > 0);
                                cWait--;
                                u64State &= ~(RTCSRW_CNT_RD_MASK | RTCSRW_WAIT_CNT_RD_MASK);
                                u64State |= (c << RTCSRW_CNT_RD_SHIFT) | (cWait << RTCSRW_WAIT_CNT_RD_SHIFT);
                                if (ASMAtomicCmpXchgU64(&pThis->s.Core.u64State, u64State, u64OldState))
                                    break;
                            }
                            return rc;
                        }

                        Assert(pThis->s.Core.fNeedReset);
                        u64State = ASMAtomicReadU64(&pThis->s.Core.u64State);
                        if ((u64State & RTCSRW_DIR_MASK) == (RTCSRW_DIR_READ << RTCSRW_DIR_SHIFT))
                            break;
                        AssertMsg(iLoop < 1, ("%u\n", iLoop));
                    }

                    /* Decrement the wait count and maybe reset the semaphore (if we're last). */
                    for (;;)
                    {
                        u64OldState = u64State;

                        cWait = (u64State & RTCSRW_WAIT_CNT_RD_MASK) >> RTCSRW_WAIT_CNT_RD_SHIFT;
                        Assert(cWait > 0);
                        cWait--;
                        u64State &= ~RTCSRW_WAIT_CNT_RD_MASK;
                        u64State |= cWait << RTCSRW_WAIT_CNT_RD_SHIFT;

                        if (ASMAtomicCmpXchgU64(&pThis->s.Core.u64State, u64State, u64OldState))
                        {
                            if (cWait == 0)
                            {
                                if (ASMAtomicXchgBool(&pThis->s.Core.fNeedReset, false))
                                {
                                    int rc = SUPSemEventMultiReset(pThis->s.CTX_SUFF(pVM)->pSession,
                                                                   (SUPSEMEVENTMULTI)pThis->s.Core.hEvtRead);
                                    AssertRCReturn(rc, rc);
                                }
                            }
                            break;
                        }
                        u64State = ASMAtomicReadU64(&pThis->s.Core.u64State);
                    }

# if defined(PDMCRITSECTRW_STRICT) && defined(IN_RING3)
                    if (!fNoVal)
                        RTLockValidatorRecSharedAddOwner(pThis->s.Core.pValidatorRead, hThreadSelf, pSrcPos);
# endif
                    break;
                }
            }
#endif /* IN_RING3 || IN_RING3 */
#ifndef IN_RING3
# ifdef IN_RING0
            else
# endif
            {
                /*
                 * We cannot call SUPSemEventMultiWaitNoResume in this context. Go
                 * back to ring-3 and do it there or return rcBusy.
                 */
                STAM_REL_COUNTER_INC(&pThis->s.CTX_MID_Z(StatContention,EnterShared));
                if (rcBusy == VINF_SUCCESS)
                {
                    PVM     pVM   = pThis->s.CTX_SUFF(pVM);     AssertPtr(pVM);
                    PVMCPU  pVCpu = VMMGetCpu(pVM);             AssertPtr(pVCpu);
                    /** @todo Should actually do this in via VMMR0.cpp instead of going all the way
                     *        back to ring-3. Goes for both kind of crit sects. */
                    return VMMRZCallRing3(pVM, pVCpu, VMMCALLRING3_PDM_CRIT_SECT_RW_ENTER_SHARED, MMHyperCCToR3(pVM, pThis));
                }
                return rcBusy;
            }
#endif /* !IN_RING3 */
        }

        if (pThis->s.Core.u32Magic != RTCRITSECTRW_MAGIC)
            return VERR_SEM_DESTROYED;

        ASMNopPause();
        u64State = ASMAtomicReadU64(&pThis->s.Core.u64State);
        u64OldState = u64State;
    }

    /* got it! */
    STAM_REL_COUNTER_INC(&pThis->s.CTX_MID_Z(Stat,EnterShared));
    Assert((ASMAtomicReadU64(&pThis->s.Core.u64State) & RTCSRW_DIR_MASK) == (RTCSRW_DIR_READ << RTCSRW_DIR_SHIFT));
    return VINF_SUCCESS;

}


/**
 * Enter a critical section with shared (read) access.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  rcBusy if in ring-0 or raw-mode context and it is busy.
 * @retval  VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 *
 * @param   pThis       Pointer to the read/write critical section.
 * @param   rcBusy      The status code to return when we're in RC or R0 and the
 *                      section is busy.   Pass VINF_SUCCESS to acquired the
 *                      critical section thru a ring-3 call if necessary.
 * @sa      PDMCritSectRwEnterSharedDebug, PDMCritSectRwTryEnterShared,
 *          PDMCritSectRwTryEnterSharedDebug, PDMCritSectRwLeaveShared,
 *          RTCritSectRwEnterShared.
 */
VMMDECL(int) PDMCritSectRwEnterShared(PPDMCRITSECTRW pThis, int rcBusy)
{
#if !defined(PDMCRITSECTRW_STRICT) || !defined(IN_RING3)
    return pdmCritSectRwEnterShared(pThis, rcBusy, false /*fTryOnly*/, NULL,    false /*fNoVal*/);
#else
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_NORMAL_API();
    return pdmCritSectRwEnterShared(pThis, rcBusy, false /*fTryOnly*/, &SrcPos, false /*fNoVal*/);
#endif
}


/**
 * Enter a critical section with shared (read) access.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  rcBusy if in ring-0 or raw-mode context and it is busy.
 * @retval  VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 *
 * @param   pThis       Pointer to the read/write critical section.
 * @param   rcBusy      The status code to return when we're in RC or R0 and the
 *                      section is busy.   Pass VINF_SUCCESS to acquired the
 *                      critical section thru a ring-3 call if necessary.
 * @param   uId         Where we're entering the section.
 * @param   SRC_POS     The source position.
 * @sa      PDMCritSectRwEnterShared, PDMCritSectRwTryEnterShared,
 *          PDMCritSectRwTryEnterSharedDebug, PDMCritSectRwLeaveShared,
 *          RTCritSectRwEnterSharedDebug.
 */
VMMDECL(int) PDMCritSectRwEnterSharedDebug(PPDMCRITSECTRW pThis, int rcBusy, RTHCUINTPTR uId, RT_SRC_POS_DECL)
{
    NOREF(uId); NOREF(pszFile); NOREF(iLine); NOREF(pszFunction);
#if !defined(PDMCRITSECTRW_STRICT) || !defined(IN_RING3)
    return pdmCritSectRwEnterShared(pThis, rcBusy, false /*fTryOnly*/, NULL,    false /*fNoVal*/);
#else
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_DEBUG_API();
    return pdmCritSectRwEnterShared(pThis, rcBusy, false /*fTryOnly*/, &SrcPos, false /*fNoVal*/);
#endif
}


/**
 * Try enter a critical section with shared (read) access.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_SEM_BUSY if the critsect was owned.
 * @retval  VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 *
 * @param   pThis       Pointer to the read/write critical section.
 * @sa      PDMCritSectRwTryEnterSharedDebug, PDMCritSectRwEnterShared,
 *          PDMCritSectRwEnterSharedDebug, PDMCritSectRwLeaveShared,
 *          RTCritSectRwTryEnterShared.
 */
VMMDECL(int) PDMCritSectRwTryEnterShared(PPDMCRITSECTRW pThis)
{
#if !defined(PDMCRITSECTRW_STRICT) || !defined(IN_RING3)
    return pdmCritSectRwEnterShared(pThis, VERR_SEM_BUSY, true /*fTryOnly*/, NULL,    false /*fNoVal*/);
#else
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_NORMAL_API();
    return pdmCritSectRwEnterShared(pThis, VERR_SEM_BUSY, true /*fTryOnly*/, &SrcPos, false /*fNoVal*/);
#endif
}


/**
 * Try enter a critical section with shared (read) access.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_SEM_BUSY if the critsect was owned.
 * @retval  VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 *
 * @param   pThis       Pointer to the read/write critical section.
 * @param   uId         Where we're entering the section.
 * @param   SRC_POS     The source position.
 * @sa      PDMCritSectRwTryEnterShared, PDMCritSectRwEnterShared,
 *          PDMCritSectRwEnterSharedDebug, PDMCritSectRwLeaveShared,
 *          RTCritSectRwTryEnterSharedDebug.
 */
VMMDECL(int) PDMCritSectRwTryEnterSharedDebug(PPDMCRITSECTRW pThis, RTHCUINTPTR uId, RT_SRC_POS_DECL)
{
    NOREF(uId); NOREF(pszFile); NOREF(iLine); NOREF(pszFunction);
#if !defined(PDMCRITSECTRW_STRICT) || !defined(IN_RING3)
    return pdmCritSectRwEnterShared(pThis, VERR_SEM_BUSY, true /*fTryOnly*/, NULL,    false /*fNoVal*/);
#else
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_DEBUG_API();
    return pdmCritSectRwEnterShared(pThis, VERR_SEM_BUSY, true /*fTryOnly*/, &SrcPos, false /*fNoVal*/);
#endif
}


#ifdef IN_RING3
/**
 * Enters a PDM read/write critical section with shared (read) access.
 *
 * @returns VINF_SUCCESS if entered successfully.
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 *
 * @param   pThis       Pointer to the read/write critical section.
 * @param   fCallRing3  Whether this is a VMMRZCallRing3()request.
 */
VMMR3DECL(int) PDMR3CritSectRwEnterSharedEx(PPDMCRITSECTRW pThis, bool fCallRing3)
{
    return pdmCritSectRwEnterShared(pThis, VERR_SEM_BUSY, false /*fTryAgain*/, NULL, fCallRing3);
}
#endif


/**
 * Leave a critical section held with shared access.
 *
 * @returns VBox status code.
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 * @param   pThis       Pointer to the read/write critical section.
 * @param   fNoVal      No validation records (i.e. queued release).
 * @sa      PDMCritSectRwEnterShared, PDMCritSectRwTryEnterShared,
 *          PDMCritSectRwEnterSharedDebug, PDMCritSectRwTryEnterSharedDebug,
 *          PDMCritSectRwLeaveExcl, RTCritSectRwLeaveShared.
 */
static int pdmCritSectRwLeaveSharedWorker(PPDMCRITSECTRW pThis, bool fNoVal)
{
    /*
     * Validate handle.
     */
    AssertPtr(pThis);
    AssertReturn(pThis->s.Core.u32Magic == RTCRITSECTRW_MAGIC, VERR_SEM_DESTROYED);

#if !defined(PDMCRITSECTRW_STRICT) || !defined(IN_RING3)
    NOREF(fNoVal);
#endif

    /*
     * Check the direction and take action accordingly.
     */
    uint64_t u64State    = ASMAtomicReadU64(&pThis->s.Core.u64State);
    uint64_t u64OldState = u64State;
    if ((u64State & RTCSRW_DIR_MASK) == (RTCSRW_DIR_READ << RTCSRW_DIR_SHIFT))
    {
#if defined(PDMCRITSECTRW_STRICT) && defined(IN_RING3)
        if (fNoVal)
            Assert(!RTLockValidatorRecSharedIsOwner(pThis->s.Core.pValidatorRead, NIL_RTTHREAD));
        else
        {
            int rc9 = RTLockValidatorRecSharedCheckAndRelease(pThis->s.Core.pValidatorRead, NIL_RTTHREAD);
            if (RT_FAILURE(rc9))
                return rc9;
        }
#endif
        for (;;)
        {
            uint64_t c = (u64State & RTCSRW_CNT_RD_MASK) >> RTCSRW_CNT_RD_SHIFT;
            AssertReturn(c > 0, VERR_NOT_OWNER);
            c--;

            if (   c > 0
                || (u64State & RTCSRW_CNT_WR_MASK) == 0)
            {
                /* Don't change the direction. */
                u64State &= ~RTCSRW_CNT_RD_MASK;
                u64State |= c << RTCSRW_CNT_RD_SHIFT;
                if (ASMAtomicCmpXchgU64(&pThis->s.Core.u64State, u64State, u64OldState))
                    break;
            }
            else
            {
#if defined(IN_RING3) || defined(IN_RING0)
# ifdef IN_RING0
                if (   RTThreadPreemptIsEnabled(NIL_RTTHREAD)
                    && ASMIntAreEnabled())
# endif
                {
                    /* Reverse the direction and signal the writer threads. */
                    u64State &= ~(RTCSRW_CNT_RD_MASK | RTCSRW_DIR_MASK);
                    u64State |= RTCSRW_DIR_WRITE << RTCSRW_DIR_SHIFT;
                    if (ASMAtomicCmpXchgU64(&pThis->s.Core.u64State, u64State, u64OldState))
                    {
                        int rc = SUPSemEventSignal(pThis->s.CTX_SUFF(pVM)->pSession, (SUPSEMEVENT)pThis->s.Core.hEvtWrite);
                        AssertRC(rc);
                        break;
                    }
                }
#endif /* IN_RING3 || IN_RING0 */
#ifndef IN_RING3
# ifdef IN_RING0
                else
# endif
                {
                    /* Queue the exit request (ring-3). */
                    PVM         pVM   = pThis->s.CTX_SUFF(pVM);         AssertPtr(pVM);
                    PVMCPU      pVCpu = VMMGetCpu(pVM);                 AssertPtr(pVCpu);
                    uint32_t    i     = pVCpu->pdm.s.cQueuedCritSectRwShrdLeaves++;
                    LogFlow(("PDMCritSectRwLeaveShared: [%d]=%p => R3 c=%d (%#llx)\n", i, pThis, c, u64State));
                    AssertFatal(i < RT_ELEMENTS(pVCpu->pdm.s.apQueuedCritSectRwShrdLeaves));
                    pVCpu->pdm.s.apQueuedCritSectRwShrdLeaves[i] = MMHyperCCToR3(pVM, pThis);
                    VMCPU_FF_SET(pVCpu, VMCPU_FF_PDM_CRITSECT);
                    VMCPU_FF_SET(pVCpu, VMCPU_FF_TO_R3);
                    STAM_REL_COUNTER_INC(&pVM->pdm.s.StatQueuedCritSectLeaves);
                    STAM_REL_COUNTER_INC(&pThis->s.StatContentionRZLeaveShared);
                    break;
                }
#endif
            }

            ASMNopPause();
            u64State = ASMAtomicReadU64(&pThis->s.Core.u64State);
            u64OldState = u64State;
        }
    }
    else
    {
        RTNATIVETHREAD hNativeSelf = pdmCritSectRwGetNativeSelf(pThis);
        RTNATIVETHREAD hNativeWriter;
        ASMAtomicUoReadHandle(&pThis->s.Core.hNativeWriter, &hNativeWriter);
        AssertReturn(hNativeSelf == hNativeWriter, VERR_NOT_OWNER);
        AssertReturn(pThis->s.Core.cWriterReads > 0, VERR_NOT_OWNER);
#if defined(PDMCRITSECTRW_STRICT) && defined(IN_RING3)
        if (!fNoVal)
        {
            int rc = RTLockValidatorRecExclUnwindMixed(pThis->s.Core.pValidatorWrite, &pThis->s.Core.pValidatorRead->Core);
            if (RT_FAILURE(rc))
                return rc;
        }
#endif
        ASMAtomicDecU32(&pThis->s.Core.cWriterReads);
    }

    return VINF_SUCCESS;
}

/**
 * Leave a critical section held with shared access.
 *
 * @returns VBox status code.
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 * @param   pThis       Pointer to the read/write critical section.
 * @sa      PDMCritSectRwEnterShared, PDMCritSectRwTryEnterShared,
 *          PDMCritSectRwEnterSharedDebug, PDMCritSectRwTryEnterSharedDebug,
 *          PDMCritSectRwLeaveExcl, RTCritSectRwLeaveShared.
 */
VMMDECL(int) PDMCritSectRwLeaveShared(PPDMCRITSECTRW pThis)
{
    return pdmCritSectRwLeaveSharedWorker(pThis, false /*fNoVal*/);
}


#if defined(IN_RING3) || defined(IN_RING0)
/**
 * PDMCritSectBothFF interface.
 *
 * @param   pThis       Pointer to the read/write critical section.
 */
void pdmCritSectRwLeaveSharedQueued(PPDMCRITSECTRW pThis)
{
    pdmCritSectRwLeaveSharedWorker(pThis, true /*fNoVal*/);
}
#endif


/**
 * Worker that enters a read/write critical section with exclusive access.
 *
 * @returns VBox status code.
 * @param   pThis       Pointer to the read/write critical section.
 * @param   rcBusy      The busy return code for ring-0 and ring-3.
 * @param   fTryOnly    Only try enter it, don't wait.
 * @param   pSrcPos     The source position. (Can be NULL.)
 * @param   fNoVal      No validation records.
 */
static int pdmCritSectRwEnterExcl(PPDMCRITSECTRW pThis, int rcBusy, bool fTryOnly, PCRTLOCKVALSRCPOS pSrcPos, bool fNoVal)
{
    /*
     * Validate input.
     */
    AssertPtr(pThis);
    AssertReturn(pThis->s.Core.u32Magic == RTCRITSECTRW_MAGIC, VERR_SEM_DESTROYED);

#if !defined(PDMCRITSECTRW_STRICT) || !defined(IN_RING3)
    NOREF(pSrcPos);
    NOREF(fNoVal);
#endif
#ifdef IN_RING3
    NOREF(rcBusy);
#endif

#if defined(PDMCRITSECTRW_STRICT) && defined(IN_RING3)
    RTTHREAD hThreadSelf = NIL_RTTHREAD;
    if (!fTryOnly)
    {
        hThreadSelf = RTThreadSelfAutoAdopt();
        int rc9 = RTLockValidatorRecExclCheckOrder(pThis->s.Core.pValidatorWrite, hThreadSelf, pSrcPos, RT_INDEFINITE_WAIT);
        if (RT_FAILURE(rc9))
            return rc9;
    }
#endif

    /*
     * Check if we're already the owner and just recursing.
     */
    RTNATIVETHREAD hNativeSelf = pdmCritSectRwGetNativeSelf(pThis);
    RTNATIVETHREAD hNativeWriter;
    ASMAtomicUoReadHandle(&pThis->s.Core.hNativeWriter, &hNativeWriter);
    if (hNativeSelf == hNativeWriter)
    {
        Assert((ASMAtomicReadU64(&pThis->s.Core.u64State) & RTCSRW_DIR_MASK) == (RTCSRW_DIR_WRITE << RTCSRW_DIR_SHIFT));
#if defined(PDMCRITSECTRW_STRICT) && defined(IN_RING3)
        if (!fNoVal)
        {
            int rc9 = RTLockValidatorRecExclRecursion(pThis->s.Core.pValidatorWrite, pSrcPos);
            if (RT_FAILURE(rc9))
                return rc9;
        }
#endif
        Assert(pThis->s.Core.cWriteRecursions < UINT32_MAX / 2);
        STAM_REL_COUNTER_INC(&pThis->s.CTX_MID_Z(Stat,EnterExcl));
        ASMAtomicIncU32(&pThis->s.Core.cWriteRecursions);
        return VINF_SUCCESS;
    }

    /*
     * Get cracking.
     */
    uint64_t u64State    = ASMAtomicReadU64(&pThis->s.Core.u64State);
    uint64_t u64OldState = u64State;

    for (;;)
    {
        if (   (u64State & RTCSRW_DIR_MASK) == (RTCSRW_DIR_WRITE << RTCSRW_DIR_SHIFT)
            || (u64State & (RTCSRW_CNT_RD_MASK | RTCSRW_CNT_WR_MASK)) != 0)
        {
            /* It flows in the right direction, try follow it before it changes. */
            uint64_t c = (u64State & RTCSRW_CNT_WR_MASK) >> RTCSRW_CNT_WR_SHIFT;
            c++;
            Assert(c < RTCSRW_CNT_MASK / 2);
            u64State &= ~RTCSRW_CNT_WR_MASK;
            u64State |= c << RTCSRW_CNT_WR_SHIFT;
            if (ASMAtomicCmpXchgU64(&pThis->s.Core.u64State, u64State, u64OldState))
                break;
        }
        else if ((u64State & (RTCSRW_CNT_RD_MASK | RTCSRW_CNT_WR_MASK)) == 0)
        {
            /* Wrong direction, but we're alone here and can simply try switch the direction. */
            u64State &= ~(RTCSRW_CNT_RD_MASK | RTCSRW_CNT_WR_MASK | RTCSRW_DIR_MASK);
            u64State |= (UINT64_C(1) << RTCSRW_CNT_WR_SHIFT) | (RTCSRW_DIR_WRITE << RTCSRW_DIR_SHIFT);
            if (ASMAtomicCmpXchgU64(&pThis->s.Core.u64State, u64State, u64OldState))
                break;
        }
        else if (fTryOnly)
        {
            /* Wrong direction and we're not supposed to wait, just return. */
            STAM_REL_COUNTER_INC(&pThis->s.CTX_MID_Z(StatContention,EnterExcl));
            return VERR_SEM_BUSY;
        }
        else
        {
            /* Add ourselves to the write count and break out to do the wait. */
            uint64_t c = (u64State & RTCSRW_CNT_WR_MASK) >> RTCSRW_CNT_WR_SHIFT;
            c++;
            Assert(c < RTCSRW_CNT_MASK / 2);
            u64State &= ~RTCSRW_CNT_WR_MASK;
            u64State |= c << RTCSRW_CNT_WR_SHIFT;
            if (ASMAtomicCmpXchgU64(&pThis->s.Core.u64State, u64State, u64OldState))
                break;
        }

        if (pThis->s.Core.u32Magic != RTCRITSECTRW_MAGIC)
            return VERR_SEM_DESTROYED;

        ASMNopPause();
        u64State = ASMAtomicReadU64(&pThis->s.Core.u64State);
        u64OldState = u64State;
    }

    /*
     * If we're in write mode now try grab the ownership. Play fair if there
     * are threads already waiting.
     */
    bool fDone = (u64State & RTCSRW_DIR_MASK) == (RTCSRW_DIR_WRITE << RTCSRW_DIR_SHIFT)
#if defined(IN_RING3)
              && (  ((u64State & RTCSRW_CNT_WR_MASK) >> RTCSRW_CNT_WR_SHIFT) == 1
                  || fTryOnly)
#endif
               ;
    if (fDone)
        ASMAtomicCmpXchgHandle(&pThis->s.Core.hNativeWriter, hNativeSelf, NIL_RTNATIVETHREAD, fDone);
    if (!fDone)
    {
        STAM_REL_COUNTER_INC(&pThis->s.CTX_MID_Z(StatContention,EnterExcl));

#if defined(IN_RING3) || defined(IN_RING0)
        if (   !fTryOnly
# ifdef IN_RING0
            && RTThreadPreemptIsEnabled(NIL_RTTHREAD)
            && ASMIntAreEnabled()
# endif
           )
        {

            /*
             * Wait for our turn.
             */
            for (uint32_t iLoop = 0; ; iLoop++)
            {
                int rc;
# ifdef IN_RING3
#  ifdef PDMCRITSECTRW_STRICT
                if (hThreadSelf == NIL_RTTHREAD)
                    hThreadSelf = RTThreadSelfAutoAdopt();
                rc = RTLockValidatorRecExclCheckBlocking(pThis->s.Core.pValidatorWrite, hThreadSelf, pSrcPos, true,
                                                         RT_INDEFINITE_WAIT, RTTHREADSTATE_RW_WRITE, false);
                if (RT_SUCCESS(rc))
#  else
                RTTHREAD hThreadSelf = RTThreadSelf();
                RTThreadBlocking(hThreadSelf, RTTHREADSTATE_RW_WRITE, false);
#  endif
# endif
                {
                    for (;;)
                    {
                        rc = SUPSemEventWaitNoResume(pThis->s.CTX_SUFF(pVM)->pSession,
                                                     (SUPSEMEVENT)pThis->s.Core.hEvtWrite,
                                                     RT_INDEFINITE_WAIT);
                        if (   rc != VERR_INTERRUPTED
                            || pThis->s.Core.u32Magic != RTCRITSECTRW_MAGIC)
                            break;
# ifdef IN_RING0
                        pdmR0CritSectRwYieldToRing3(pThis);
# endif
                    }
# ifdef IN_RING3
                    RTThreadUnblocked(hThreadSelf, RTTHREADSTATE_RW_WRITE);
# endif
                    if (pThis->s.Core.u32Magic != RTCRITSECTRW_MAGIC)
                        return VERR_SEM_DESTROYED;
                }
                if (RT_FAILURE(rc))
                {
                    /* Decrement the counts and return the error. */
                    for (;;)
                    {
                        u64OldState = u64State = ASMAtomicReadU64(&pThis->s.Core.u64State);
                        uint64_t c = (u64State & RTCSRW_CNT_WR_MASK) >> RTCSRW_CNT_WR_SHIFT; Assert(c > 0);
                        c--;
                        u64State &= ~RTCSRW_CNT_WR_MASK;
                        u64State |= c << RTCSRW_CNT_WR_SHIFT;
                        if (ASMAtomicCmpXchgU64(&pThis->s.Core.u64State, u64State, u64OldState))
                            break;
                    }
                    return rc;
                }

                u64State = ASMAtomicReadU64(&pThis->s.Core.u64State);
                if ((u64State & RTCSRW_DIR_MASK) == (RTCSRW_DIR_WRITE << RTCSRW_DIR_SHIFT))
                {
                    ASMAtomicCmpXchgHandle(&pThis->s.Core.hNativeWriter, hNativeSelf, NIL_RTNATIVETHREAD, fDone);
                    if (fDone)
                        break;
                }
                AssertMsg(iLoop < 1000, ("%u\n", iLoop)); /* may loop a few times here... */
            }

        }
        else
#endif /* IN_RING3 || IN_RING0 */
        {
#ifdef IN_RING3
            /* TryEnter call - decrement the number of (waiting) writers.  */
#else
            /* We cannot call SUPSemEventWaitNoResume in this context. Go back to
               ring-3 and do it there or return rcBusy. */
#endif

            for (;;)
            {
                u64OldState = u64State = ASMAtomicReadU64(&pThis->s.Core.u64State);
                uint64_t c = (u64State & RTCSRW_CNT_WR_MASK) >> RTCSRW_CNT_WR_SHIFT; Assert(c > 0);
                c--;
                u64State &= ~RTCSRW_CNT_WR_MASK;
                u64State |= c << RTCSRW_CNT_WR_SHIFT;
                if (ASMAtomicCmpXchgU64(&pThis->s.Core.u64State, u64State, u64OldState))
                    break;
            }

#ifdef IN_RING3
            return VERR_SEM_BUSY;
#else
            if (rcBusy == VINF_SUCCESS)
            {
                Assert(!fTryOnly);
                PVM     pVM   = pThis->s.CTX_SUFF(pVM);     AssertPtr(pVM);
                PVMCPU  pVCpu = VMMGetCpu(pVM);             AssertPtr(pVCpu);
                /** @todo Should actually do this in via VMMR0.cpp instead of going all the way
                 *        back to ring-3. Goes for both kind of crit sects. */
                return VMMRZCallRing3(pVM, pVCpu, VMMCALLRING3_PDM_CRIT_SECT_RW_ENTER_EXCL, MMHyperCCToR3(pVM, pThis));
            }
            return rcBusy;
#endif
        }
    }

    /*
     * Got it!
     */
    Assert((ASMAtomicReadU64(&pThis->s.Core.u64State) & RTCSRW_DIR_MASK) == (RTCSRW_DIR_WRITE << RTCSRW_DIR_SHIFT));
    ASMAtomicWriteU32(&pThis->s.Core.cWriteRecursions, 1);
    Assert(pThis->s.Core.cWriterReads == 0);
#if defined(PDMCRITSECTRW_STRICT) && defined(IN_RING3)
    if (!fNoVal)
        RTLockValidatorRecExclSetOwner(pThis->s.Core.pValidatorWrite, hThreadSelf, pSrcPos, true);
#endif
    STAM_REL_COUNTER_INC(&pThis->s.CTX_MID_Z(Stat,EnterExcl));
    STAM_PROFILE_ADV_START(&pThis->s.StatWriteLocked, swl);

    return VINF_SUCCESS;
}


/**
 * Try enter a critical section with exclusive (write) access.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  rcBusy if in ring-0 or raw-mode context and it is busy.
 * @retval  VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 *
 * @param   pThis       Pointer to the read/write critical section.
 * @param   rcBusy      The status code to return when we're in RC or R0 and the
 *                      section is busy.   Pass VINF_SUCCESS to acquired the
 *                      critical section thru a ring-3 call if necessary.
 * @sa      PDMCritSectRwEnterExclDebug, PDMCritSectRwTryEnterExcl,
 *          PDMCritSectRwTryEnterExclDebug,
 *          PDMCritSectEnterDebug, PDMCritSectEnter,
 * RTCritSectRwEnterExcl.
 */
VMMDECL(int) PDMCritSectRwEnterExcl(PPDMCRITSECTRW pThis, int rcBusy)
{
#if !defined(PDMCRITSECTRW_STRICT) || !defined(IN_RING3)
    return pdmCritSectRwEnterExcl(pThis, rcBusy, false /*fTryAgain*/, NULL,    false /*fNoVal*/);
#else
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_NORMAL_API();
    return pdmCritSectRwEnterExcl(pThis, rcBusy, false /*fTryAgain*/, &SrcPos, false /*fNoVal*/);
#endif
}


/**
 * Try enter a critical section with exclusive (write) access.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  rcBusy if in ring-0 or raw-mode context and it is busy.
 * @retval  VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 *
 * @param   pThis       Pointer to the read/write critical section.
 * @param   rcBusy      The status code to return when we're in RC or R0 and the
 *                      section is busy.   Pass VINF_SUCCESS to acquired the
 *                      critical section thru a ring-3 call if necessary.
 * @param   uId         Where we're entering the section.
 * @param   SRC_POS     The source position.
 * @sa      PDMCritSectRwEnterExcl, PDMCritSectRwTryEnterExcl,
 *          PDMCritSectRwTryEnterExclDebug,
 *          PDMCritSectEnterDebug, PDMCritSectEnter,
 *          RTCritSectRwEnterExclDebug.
 */
VMMDECL(int) PDMCritSectRwEnterExclDebug(PPDMCRITSECTRW pThis, int rcBusy, RTHCUINTPTR uId, RT_SRC_POS_DECL)
{
    NOREF(uId); NOREF(pszFile); NOREF(iLine); NOREF(pszFunction);
#if !defined(PDMCRITSECTRW_STRICT) || !defined(IN_RING3)
    return pdmCritSectRwEnterExcl(pThis, rcBusy, false /*fTryAgain*/, NULL,    false /*fNoVal*/);
#else
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_DEBUG_API();
    return pdmCritSectRwEnterExcl(pThis, rcBusy, false /*fTryAgain*/, &SrcPos, false /*fNoVal*/);
#endif
}


/**
 * Try enter a critical section with exclusive (write) access.
 *
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_SEM_BUSY if the critsect was owned.
 * @retval  VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 *
 * @param   pThis       Pointer to the read/write critical section.
 * @sa      PDMCritSectRwEnterExcl, PDMCritSectRwTryEnterExclDebug,
 *          PDMCritSectRwEnterExclDebug,
 *          PDMCritSectTryEnter, PDMCritSectTryEnterDebug,
 *          RTCritSectRwTryEnterExcl.
 */
VMMDECL(int) PDMCritSectRwTryEnterExcl(PPDMCRITSECTRW pThis)
{
#if !defined(PDMCRITSECTRW_STRICT) || !defined(IN_RING3)
    return pdmCritSectRwEnterExcl(pThis, VERR_SEM_BUSY, true /*fTryAgain*/, NULL,    false /*fNoVal*/);
#else
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_NORMAL_API();
    return pdmCritSectRwEnterExcl(pThis, VERR_SEM_BUSY, true /*fTryAgain*/, &SrcPos, false /*fNoVal*/);
#endif
}


/**
 * Try enter a critical section with exclusive (write) access.
 *
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_SEM_BUSY if the critsect was owned.
 * @retval  VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 *
 * @param   pThis       Pointer to the read/write critical section.
 * @param   uId         Where we're entering the section.
 * @param   SRC_POS     The source position.
 * @sa      PDMCritSectRwTryEnterExcl, PDMCritSectRwEnterExcl,
 *          PDMCritSectRwEnterExclDebug,
 *          PDMCritSectTryEnterDebug, PDMCritSectTryEnter,
 *          RTCritSectRwTryEnterExclDebug.
 */
VMMDECL(int) PDMCritSectRwTryEnterExclDebug(PPDMCRITSECTRW pThis, RTHCUINTPTR uId, RT_SRC_POS_DECL)
{
    NOREF(uId); NOREF(pszFile); NOREF(iLine); NOREF(pszFunction);
#if !defined(PDMCRITSECTRW_STRICT) || !defined(IN_RING3)
    return pdmCritSectRwEnterExcl(pThis, VERR_SEM_BUSY, true /*fTryAgain*/, NULL,    false /*fNoVal*/);
#else
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_DEBUG_API();
    return pdmCritSectRwEnterExcl(pThis, VERR_SEM_BUSY, true /*fTryAgain*/, &SrcPos, false /*fNoVal*/);
#endif
}


#ifdef IN_RING3
/**
 * Enters a PDM read/write critical section with exclusive (write) access.
 *
 * @returns VINF_SUCCESS if entered successfully.
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 *
 * @param   pThis       Pointer to the read/write critical section.
 * @param   fCallRing3  Whether this is a VMMRZCallRing3()request.
 */
VMMR3DECL(int) PDMR3CritSectRwEnterExclEx(PPDMCRITSECTRW pThis, bool fCallRing3)
{
    return pdmCritSectRwEnterExcl(pThis, VERR_SEM_BUSY, false /*fTryAgain*/, NULL, fCallRing3 /*fNoVal*/);
}
#endif /* IN_RING3 */


/**
 * Leave a critical section held exclusively.
 *
 * @returns VBox status code.
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 * @param   pThis       Pointer to the read/write critical section.
 * @param   fNoVal      No validation records (i.e. queued release).
 * @sa      PDMCritSectRwLeaveShared, RTCritSectRwLeaveExcl.
 */
static int pdmCritSectRwLeaveExclWorker(PPDMCRITSECTRW pThis, bool fNoVal)
{
    /*
     * Validate handle.
     */
    AssertPtr(pThis);
    AssertReturn(pThis->s.Core.u32Magic == RTCRITSECTRW_MAGIC, VERR_SEM_DESTROYED);

#if !defined(PDMCRITSECTRW_STRICT) || !defined(IN_RING3)
    NOREF(fNoVal);
#endif

    RTNATIVETHREAD hNativeSelf = pdmCritSectRwGetNativeSelf(pThis);
    RTNATIVETHREAD hNativeWriter;
    ASMAtomicUoReadHandle(&pThis->s.Core.hNativeWriter, &hNativeWriter);
    AssertReturn(hNativeSelf == hNativeWriter, VERR_NOT_OWNER);

    /*
     * Unwind one recursion. Is it the final one?
     */
    if (pThis->s.Core.cWriteRecursions == 1)
    {
        AssertReturn(pThis->s.Core.cWriterReads == 0, VERR_WRONG_ORDER); /* (must release all read recursions before the final write.) */
#if defined(PDMCRITSECTRW_STRICT) && defined(IN_RING3)
        if (fNoVal)
            Assert(pThis->s.Core.pValidatorWrite->hThread == NIL_RTTHREAD);
        else
        {
            int rc9 = RTLockValidatorRecExclReleaseOwner(pThis->s.Core.pValidatorWrite, true);
            if (RT_FAILURE(rc9))
                return rc9;
        }
#endif
        /*
         * Update the state.
         */
#if defined(IN_RING3) || defined(IN_RING0)
# ifdef IN_RING0
        if (   RTThreadPreemptIsEnabled(NIL_RTTHREAD)
            && ASMIntAreEnabled())
# endif
        {
            ASMAtomicWriteU32(&pThis->s.Core.cWriteRecursions, 0);
            STAM_PROFILE_ADV_STOP(&pThis->s.StatWriteLocked, swl);
            ASMAtomicWriteHandle(&pThis->s.Core.hNativeWriter, NIL_RTNATIVETHREAD);

            for (;;)
            {
                uint64_t u64State    = ASMAtomicReadU64(&pThis->s.Core.u64State);
                uint64_t u64OldState = u64State;

                uint64_t c = (u64State & RTCSRW_CNT_WR_MASK) >> RTCSRW_CNT_WR_SHIFT;
                Assert(c > 0);
                c--;

                if (   c > 0
                    || (u64State & RTCSRW_CNT_RD_MASK) == 0)
                {
                    /* Don't change the direction, wake up the next writer if any. */
                    u64State &= ~RTCSRW_CNT_WR_MASK;
                    u64State |= c << RTCSRW_CNT_WR_SHIFT;
                    if (ASMAtomicCmpXchgU64(&pThis->s.Core.u64State, u64State, u64OldState))
                    {
                        if (c > 0)
                        {
                            int rc = SUPSemEventSignal(pThis->s.CTX_SUFF(pVM)->pSession, (SUPSEMEVENT)pThis->s.Core.hEvtWrite);
                            AssertRC(rc);
                        }
                        break;
                    }
                }
                else
                {
                    /* Reverse the direction and signal the reader threads. */
                    u64State &= ~(RTCSRW_CNT_WR_MASK | RTCSRW_DIR_MASK);
                    u64State |= RTCSRW_DIR_READ << RTCSRW_DIR_SHIFT;
                    if (ASMAtomicCmpXchgU64(&pThis->s.Core.u64State, u64State, u64OldState))
                    {
                        Assert(!pThis->s.Core.fNeedReset);
                        ASMAtomicWriteBool(&pThis->s.Core.fNeedReset, true);
                        int rc = SUPSemEventMultiSignal(pThis->s.CTX_SUFF(pVM)->pSession, (SUPSEMEVENTMULTI)pThis->s.Core.hEvtRead);
                        AssertRC(rc);
                        break;
                    }
                }

                ASMNopPause();
                if (pThis->s.Core.u32Magic != RTCRITSECTRW_MAGIC)
                    return VERR_SEM_DESTROYED;
            }
        }
#endif /* IN_RING3 || IN_RING0 */
#ifndef IN_RING3
# ifdef IN_RING0
        else
# endif
        {
            /*
             * We cannot call neither SUPSemEventSignal nor SUPSemEventMultiSignal,
             * so queue the exit request (ring-3).
             */
            PVM         pVM   = pThis->s.CTX_SUFF(pVM);         AssertPtr(pVM);
            PVMCPU      pVCpu = VMMGetCpu(pVM);                 AssertPtr(pVCpu);
            uint32_t    i     = pVCpu->pdm.s.cQueuedCritSectRwExclLeaves++;
            LogFlow(("PDMCritSectRwLeaveShared: [%d]=%p => R3\n", i, pThis));
            AssertFatal(i < RT_ELEMENTS(pVCpu->pdm.s.apQueuedCritSectLeaves));
            pVCpu->pdm.s.apQueuedCritSectRwExclLeaves[i] = MMHyperCCToR3(pVM, pThis);
            VMCPU_FF_SET(pVCpu, VMCPU_FF_PDM_CRITSECT);
            VMCPU_FF_SET(pVCpu, VMCPU_FF_TO_R3);
            STAM_REL_COUNTER_INC(&pVM->pdm.s.StatQueuedCritSectLeaves);
            STAM_REL_COUNTER_INC(&pThis->s.StatContentionRZLeaveExcl);
        }
#endif
    }
    else
    {
        /*
         * Not the final recursion.
         */
        Assert(pThis->s.Core.cWriteRecursions != 0);
#if defined(PDMCRITSECTRW_STRICT) && defined(IN_RING3)
        if (fNoVal)
            Assert(pThis->s.Core.pValidatorWrite->hThread == NIL_RTTHREAD);
        else
        {
            int rc9 = RTLockValidatorRecExclUnwind(pThis->s.Core.pValidatorWrite);
            if (RT_FAILURE(rc9))
                return rc9;
        }
#endif
        ASMAtomicDecU32(&pThis->s.Core.cWriteRecursions);
    }

    return VINF_SUCCESS;
}


/**
 * Leave a critical section held exclusively.
 *
 * @returns VBox status code.
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 * @param   pThis       Pointer to the read/write critical section.
 * @sa      PDMCritSectRwLeaveShared, RTCritSectRwLeaveExcl.
 */
VMMDECL(int) PDMCritSectRwLeaveExcl(PPDMCRITSECTRW pThis)
{
    return pdmCritSectRwLeaveExclWorker(pThis, false /*fNoVal*/);
}


#if defined(IN_RING3) || defined(IN_RING0)
/**
 * PDMCritSectBothFF interface.
 *
 * @param   pThis       Pointer to the read/write critical section.
 */
void pdmCritSectRwLeaveExclQueued(PPDMCRITSECTRW pThis)
{
    pdmCritSectRwLeaveExclWorker(pThis, true /*fNoVal*/);
}
#endif


/**
 * Checks the caller is the exclusive (write) owner of the critical section.
 *
 * @retval  true if owner.
 * @retval  false if not owner.
 * @param   pThis       Pointer to the read/write critical section.
 * @sa      PDMCritSectRwIsReadOwner, PDMCritSectIsOwner,
 *          RTCritSectRwIsWriteOwner.
 */
VMMDECL(bool) PDMCritSectRwIsWriteOwner(PPDMCRITSECTRW pThis)
{
    /*
     * Validate handle.
     */
    AssertPtr(pThis);
    AssertReturn(pThis->s.Core.u32Magic == RTCRITSECTRW_MAGIC, false);

    /*
     * Check ownership.
     */
    RTNATIVETHREAD hNativeWriter;
    ASMAtomicUoReadHandle(&pThis->s.Core.hNativeWriter, &hNativeWriter);
    if (hNativeWriter == NIL_RTNATIVETHREAD)
        return false;
    return hNativeWriter == pdmCritSectRwGetNativeSelf(pThis);
}


/**
 * Checks if the caller is one of the read owners of the critical section.
 *
 * @note    !CAUTION!  This API doesn't work reliably if lock validation isn't
 *          enabled. Meaning, the answer is not trustworhty unless
 *          RT_LOCK_STRICT or PDMCRITSECTRW_STRICT was defined at build time.
 *          Also, make sure you do not use RTCRITSECTRW_FLAGS_NO_LOCK_VAL when
 *          creating the semaphore.  And finally, if you used a locking class,
 *          don't disable deadlock detection by setting cMsMinDeadlock to
 *          RT_INDEFINITE_WAIT.
 *
 *          In short, only use this for assertions.
 *
 * @returns @c true if reader, @c false if not.
 * @param   pThis       Pointer to the read/write critical section.
 * @param   fWannaHear  What you'd like to hear when lock validation is not
 *                      available.  (For avoiding asserting all over the place.)
 * @sa      PDMCritSectRwIsWriteOwner, RTCritSectRwIsReadOwner.
 */
VMMDECL(bool) PDMCritSectRwIsReadOwner(PPDMCRITSECTRW pThis, bool fWannaHear)
{
    /*
     * Validate handle.
     */
    AssertPtr(pThis);
    AssertReturn(pThis->s.Core.u32Magic == RTCRITSECTRW_MAGIC, false);

    /*
     * Inspect the state.
     */
    uint64_t u64State = ASMAtomicReadU64(&pThis->s.Core.u64State);
    if ((u64State & RTCSRW_DIR_MASK) == (RTCSRW_DIR_WRITE << RTCSRW_DIR_SHIFT))
    {
        /*
         * It's in write mode, so we can only be a reader if we're also the
         * current writer.
         */
        RTNATIVETHREAD hWriter;
        ASMAtomicUoReadHandle(&pThis->s.Core.hNativeWriter, &hWriter);
        if (hWriter == NIL_RTNATIVETHREAD)
            return false;
        return hWriter == pdmCritSectRwGetNativeSelf(pThis);
    }

    /*
     * Read mode.  If there are no current readers, then we cannot be a reader.
     */
    if (!(u64State & RTCSRW_CNT_RD_MASK))
        return false;

#if defined(PDMCRITSECTRW_STRICT) && defined(IN_RING3)
    /*
     * Ask the lock validator.
     * Note! It doesn't know everything, let's deal with that if it becomes an issue...
     */
    NOREF(fWannaHear);
    return RTLockValidatorRecSharedIsOwner(pThis->s.Core.pValidatorRead, NIL_RTTHREAD);
#else
    /*
     * Ok, we don't know, just tell the caller what he want to hear.
     */
    return fWannaHear;
#endif
}


/**
 * Gets the write recursion count.
 *
 * @returns The write recursion count (0 if bad critsect).
 * @param   pThis       Pointer to the read/write critical section.
 * @sa      PDMCritSectRwGetWriterReadRecursion, PDMCritSectRwGetReadCount,
 *          RTCritSectRwGetWriteRecursion.
 */
VMMDECL(uint32_t) PDMCritSectRwGetWriteRecursion(PPDMCRITSECTRW pThis)
{
    /*
     * Validate handle.
     */
    AssertPtr(pThis);
    AssertReturn(pThis->s.Core.u32Magic == RTCRITSECTRW_MAGIC, 0);

    /*
     * Return the requested data.
     */
    return pThis->s.Core.cWriteRecursions;
}


/**
 * Gets the read recursion count of the current writer.
 *
 * @returns The read recursion count (0 if bad critsect).
 * @param   pThis       Pointer to the read/write critical section.
 * @sa      PDMCritSectRwGetWriteRecursion, PDMCritSectRwGetReadCount,
 *          RTCritSectRwGetWriterReadRecursion.
 */
VMMDECL(uint32_t) PDMCritSectRwGetWriterReadRecursion(PPDMCRITSECTRW pThis)
{
    /*
     * Validate handle.
     */
    AssertPtr(pThis);
    AssertReturn(pThis->s.Core.u32Magic == RTCRITSECTRW_MAGIC, 0);

    /*
     * Return the requested data.
     */
    return pThis->s.Core.cWriterReads;
}


/**
 * Gets the current number of reads.
 *
 * This includes all read recursions, so it might be higher than the number of
 * read owners.  It does not include reads done by the current writer.
 *
 * @returns The read count (0 if bad critsect).
 * @param   pThis       Pointer to the read/write critical section.
 * @sa      PDMCritSectRwGetWriteRecursion, PDMCritSectRwGetWriterReadRecursion,
 *          RTCritSectRwGetReadCount.
 */
VMMDECL(uint32_t) PDMCritSectRwGetReadCount(PPDMCRITSECTRW pThis)
{
    /*
     * Validate input.
     */
    AssertPtr(pThis);
    AssertReturn(pThis->s.Core.u32Magic == RTCRITSECTRW_MAGIC, 0);

    /*
     * Return the requested data.
     */
    uint64_t u64State = ASMAtomicReadU64(&pThis->s.Core.u64State);
    if ((u64State & RTCSRW_DIR_MASK) != (RTCSRW_DIR_READ << RTCSRW_DIR_SHIFT))
        return 0;
    return (u64State & RTCSRW_CNT_RD_MASK) >> RTCSRW_CNT_RD_SHIFT;
}


/**
 * Checks if the read/write critical section is initialized or not.
 *
 * @retval  true if initialized.
 * @retval  false if not initialized.
 * @param   pThis       Pointer to the read/write critical section.
 * @sa      PDMCritSectIsInitialized, RTCritSectRwIsInitialized.
 */
VMMDECL(bool) PDMCritSectRwIsInitialized(PCPDMCRITSECTRW pThis)
{
    AssertPtr(pThis);
    return pThis->s.Core.u32Magic == RTCRITSECTRW_MAGIC;
}

