/* $Id: EMAll.cpp $ */
/** @file
 * EM - Execution Monitor(/Manager) - All contexts
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
#define VBOX_WITH_IEM
#define LOG_GROUP LOG_GROUP_EM
#include <VBox/vmm/em.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/selm.h>
#include <VBox/vmm/patm.h>
#include <VBox/vmm/csam.h>
#include <VBox/vmm/pgm.h>
#ifdef VBOX_WITH_IEM
# include <VBox/vmm/iem.h>
#endif
#include <VBox/vmm/iom.h>
#include <VBox/vmm/stam.h>
#include "EMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/vmm.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/tm.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/param.h>
#include <VBox/err.h>
#include <VBox/dis.h>
#include <VBox/disopcode.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/string.h>

#ifdef VBOX_WITH_IEM
//# define VBOX_COMPARE_IEM_AND_EM /* debugging... */
//# define VBOX_SAME_AS_EM
//# define VBOX_COMPARE_IEM_LAST
#endif

#ifdef VBOX_WITH_RAW_RING1
# define EM_EMULATE_SMSW
#endif


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** @def EM_ASSERT_FAULT_RETURN
 * Safety check.
 *
 * Could in theory misfire on a cross page boundary access...
 *
 * Currently disabled because the CSAM (+ PATM) patch monitoring occasionally
 * turns up an alias page instead of the original faulting one and annoying the
 * heck out of anyone running a debug build. See @bugref{2609} and @bugref{1931}.
 */
#if 0
# define EM_ASSERT_FAULT_RETURN(expr, rc) AssertReturn(expr, rc)
#else
# define EM_ASSERT_FAULT_RETURN(expr, rc) do { } while (0)
#endif


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
#if !defined(VBOX_WITH_IEM) || defined(VBOX_COMPARE_IEM_AND_EM)
DECLINLINE(VBOXSTRICTRC) emInterpretInstructionCPUOuter(PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame,
                                                        RTGCPTR pvFault, EMCODETYPE enmCodeType, uint32_t *pcbSize);
#endif


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
#ifdef VBOX_COMPARE_IEM_AND_EM
static const uint32_t g_fInterestingFFs = VMCPU_FF_TO_R3
    | VMCPU_FF_CSAM_PENDING_ACTION | VMCPU_FF_CSAM_SCAN_PAGE | VMCPU_FF_INHIBIT_INTERRUPTS
    | VMCPU_FF_SELM_SYNC_LDT | VMCPU_FF_SELM_SYNC_GDT | VMCPU_FF_SELM_SYNC_TSS | VMCPU_FF_TRPM_SYNC_IDT
    | VMCPU_FF_TLB_FLUSH | VMCPU_FF_PGM_SYNC_CR3 | VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL;
static uint32_t g_fIncomingFFs;
static CPUMCTX g_IncomingCtx;
static bool    g_fIgnoreRaxRdx = false;

static uint32_t g_fEmFFs;
static CPUMCTX g_EmCtx;
static uint8_t g_abEmWrote[256];
static size_t  g_cbEmWrote;

static uint32_t g_fIemFFs;
static CPUMCTX g_IemCtx;
extern uint8_t g_abIemWrote[256];
#if defined(VBOX_COMPARE_IEM_FIRST) || defined(VBOX_COMPARE_IEM_LAST)
extern size_t  g_cbIemWrote;
#else
static size_t  g_cbIemWrote;
#endif
#endif


/**
 * Get the current execution manager status.
 *
 * @returns Current status.
 * @param   pVCpu         The cross context virtual CPU structure.
 */
VMM_INT_DECL(EMSTATE) EMGetState(PVMCPU pVCpu)
{
    return pVCpu->em.s.enmState;
}


/**
 * Sets the current execution manager status. (use only when you know what you're doing!)
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   enmNewState The new state, EMSTATE_WAIT_SIPI or EMSTATE_HALTED.
 */
VMM_INT_DECL(void)    EMSetState(PVMCPU pVCpu, EMSTATE enmNewState)
{
    /* Only allowed combination: */
    Assert(pVCpu->em.s.enmState == EMSTATE_WAIT_SIPI && enmNewState == EMSTATE_HALTED);
    pVCpu->em.s.enmState = enmNewState;
}


/**
 * Sets the PC for which interrupts should be inhibited.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   PC          The PC.
 */
VMMDECL(void) EMSetInhibitInterruptsPC(PVMCPU pVCpu, RTGCUINTPTR PC)
{
    pVCpu->em.s.GCPtrInhibitInterrupts = PC;
    VMCPU_FF_SET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS);
}


/**
 * Gets the PC for which interrupts should be inhibited.
 *
 * There are a few instructions which inhibits or delays interrupts
 * for the instruction following them. These instructions are:
 *      - STI
 *      - MOV SS, r/m16
 *      - POP SS
 *
 * @returns The PC for which interrupts should be inhibited.
 * @param   pVCpu       The cross context virtual CPU structure.
 *
 */
VMMDECL(RTGCUINTPTR) EMGetInhibitInterruptsPC(PVMCPU pVCpu)
{
    return pVCpu->em.s.GCPtrInhibitInterrupts;
}


/**
 * Prepare an MWAIT - essentials of the MONITOR instruction.
 *
 * @returns VINF_SUCCESS
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 * @param   rax                 The content of RAX.
 * @param   rcx                 The content of RCX.
 * @param   rdx                 The content of RDX.
 * @param   GCPhys              The physical address corresponding to rax.
 */
VMM_INT_DECL(int) EMMonitorWaitPrepare(PVMCPU pVCpu, uint64_t rax, uint64_t rcx, uint64_t rdx, RTGCPHYS GCPhys)
{
    pVCpu->em.s.MWait.uMonitorRAX = rax;
    pVCpu->em.s.MWait.uMonitorRCX = rcx;
    pVCpu->em.s.MWait.uMonitorRDX = rdx;
    pVCpu->em.s.MWait.fWait |= EMMWAIT_FLAG_MONITOR_ACTIVE;
    /** @todo Make use of GCPhys. */
    NOREF(GCPhys);
    /** @todo Complete MONITOR implementation.  */
    return VINF_SUCCESS;
}


/**
 * Checks if the monitor hardware is armed / active.
 *
 * @returns true if armed, false otherwise.
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 */
VMM_INT_DECL(bool) EMMonitorIsArmed(PVMCPU pVCpu)
{
    return RT_BOOL(pVCpu->em.s.MWait.fWait & EMMWAIT_FLAG_MONITOR_ACTIVE);
}


/**
 * Performs an MWAIT.
 *
 * @returns VINF_SUCCESS
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 * @param   rax                 The content of RAX.
 * @param   rcx                 The content of RCX.
 */
VMM_INT_DECL(int) EMMonitorWaitPerform(PVMCPU pVCpu, uint64_t rax, uint64_t rcx)
{
    pVCpu->em.s.MWait.uMWaitRAX = rax;
    pVCpu->em.s.MWait.uMWaitRCX = rcx;
    pVCpu->em.s.MWait.fWait |= EMMWAIT_FLAG_ACTIVE;
    if (rcx)
        pVCpu->em.s.MWait.fWait |= EMMWAIT_FLAG_BREAKIRQIF0;
    else
        pVCpu->em.s.MWait.fWait &= ~EMMWAIT_FLAG_BREAKIRQIF0;
    /** @todo not completely correct?? */
    return VINF_EM_HALT;
}



/**
 * Determine if we should continue execution in HM after encountering an mwait
 * instruction.
 *
 * Clears MWAIT flags if returning @c true.
 *
 * @returns true if we should continue, false if we should halt.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pCtx            Current CPU context.
 */
VMM_INT_DECL(bool) EMMonitorWaitShouldContinue(PVMCPU pVCpu, PCPUMCTX pCtx)
{
    if (   pCtx->eflags.Bits.u1IF
        || (   (pVCpu->em.s.MWait.fWait & (EMMWAIT_FLAG_ACTIVE | EMMWAIT_FLAG_BREAKIRQIF0))
            ==                            (EMMWAIT_FLAG_ACTIVE | EMMWAIT_FLAG_BREAKIRQIF0)) )
    {
        if (VMCPU_FF_IS_PENDING(pVCpu, (VMCPU_FF_UPDATE_APIC | VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC)))
        {
            pVCpu->em.s.MWait.fWait &= ~(EMMWAIT_FLAG_ACTIVE | EMMWAIT_FLAG_BREAKIRQIF0);
            return true;
        }
    }

    return false;
}


/**
 * Determine if we should continue execution in HM after encountering a hlt
 * instruction.
 *
 * @returns true if we should continue, false if we should halt.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pCtx            Current CPU context.
 */
VMM_INT_DECL(bool) EMShouldContinueAfterHalt(PVMCPU pVCpu, PCPUMCTX pCtx)
{
    if (pCtx->eflags.Bits.u1IF)
        return !!VMCPU_FF_IS_PENDING(pVCpu, (VMCPU_FF_UPDATE_APIC | VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC));
    return false;
}


/**
 * Unhalts and wakes up the given CPU.
 *
 * This is an API for assisting the KVM hypercall API in implementing KICK_CPU.
 * It sets VMCPU_FF_UNHALT for @a pVCpuDst and makes sure it is woken up.   If
 * the CPU isn't currently in a halt, the next HLT instruction it executes will
 * be affected.
 *
 * @returns GVMMR0SchedWakeUpEx result or VINF_SUCCESS depending on context.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpuDst        The cross context virtual CPU structure of the
 *                          CPU to unhalt and wake up.  This is usually not the
 *                          same as the caller.
 * @thread  EMT
 */
VMM_INT_DECL(int) EMUnhaltAndWakeUp(PVM pVM, PVMCPU pVCpuDst)
{
    /*
     * Flag the current(/next) HLT to unhalt immediately.
     */
    VMCPU_FF_SET(pVCpuDst, VMCPU_FF_UNHALT);

    /*
     * Wake up the EMT (technically should be abstracted by VMM/VMEmt, but
     * just do it here for now).
     */
#ifdef IN_RING0
    /* We might be here with preemption disabled or enabled (i.e. depending on
       thread-context hooks being used), so don't try obtaining the GVMMR0 used
       lock here. See @bugref{7270#c148}. */
    int rc = GVMMR0SchedWakeUpNoGVMNoLock(pVM, pVCpuDst->idCpu);
    AssertRC(rc);

#elif defined(IN_RING3)
    int rc = SUPR3CallVMMR0(pVM->pVMR0, pVCpuDst->idCpu, VMMR0_DO_GVMM_SCHED_WAKE_UP, NULL /* pvArg */);
    AssertRC(rc);

#else
    /* Nothing to do for raw-mode, shouldn't really be used by raw-mode guests anyway. */
    Assert(pVM->cCpus == 1); NOREF(pVM);
    int rc = VINF_SUCCESS;
#endif
    return rc;
}


/**
 * Locks REM execution to a single VCPU.
 *
 * @param   pVM         The cross context VM structure.
 */
VMMDECL(void) EMRemLock(PVM pVM)
{
#ifdef VBOX_WITH_REM
    if (!PDMCritSectIsInitialized(&pVM->em.s.CritSectREM))
        return;     /* early init */

    Assert(!PGMIsLockOwner(pVM));
    Assert(!IOMIsLockWriteOwner(pVM));
    int rc = PDMCritSectEnter(&pVM->em.s.CritSectREM, VERR_SEM_BUSY);
    AssertRCSuccess(rc);
#else
    RT_NOREF(pVM);
#endif
}


/**
 * Unlocks REM execution
 *
 * @param   pVM         The cross context VM structure.
 */
VMMDECL(void) EMRemUnlock(PVM pVM)
{
#ifdef VBOX_WITH_REM
    if (!PDMCritSectIsInitialized(&pVM->em.s.CritSectREM))
        return;     /* early init */

    PDMCritSectLeave(&pVM->em.s.CritSectREM);
#else
    RT_NOREF(pVM);
#endif
}


/**
 * Check if this VCPU currently owns the REM lock.
 *
 * @returns bool owner/not owner
 * @param   pVM         The cross context VM structure.
 */
VMMDECL(bool) EMRemIsLockOwner(PVM pVM)
{
#ifdef VBOX_WITH_REM
    if (!PDMCritSectIsInitialized(&pVM->em.s.CritSectREM))
        return true;   /* early init */

    return PDMCritSectIsOwner(&pVM->em.s.CritSectREM);
#else
    RT_NOREF(pVM);
    return true;
#endif
}


/**
 * Try to acquire the REM lock.
 *
 * @returns VBox status code
 * @param   pVM         The cross context VM structure.
 */
VMM_INT_DECL(int) EMRemTryLock(PVM pVM)
{
#ifdef VBOX_WITH_REM
    if (!PDMCritSectIsInitialized(&pVM->em.s.CritSectREM))
        return VINF_SUCCESS; /* early init */

    return PDMCritSectTryEnter(&pVM->em.s.CritSectREM);
#else
    RT_NOREF(pVM);
    return VINF_SUCCESS;
#endif
}


/**
 * @callback_method_impl{FNDISREADBYTES}
 */
static DECLCALLBACK(int) emReadBytes(PDISCPUSTATE pDis, uint8_t offInstr, uint8_t cbMinRead, uint8_t cbMaxRead)
{
    PVMCPU      pVCpu    = (PVMCPU)pDis->pvUser;
#if defined(VBOX_WITH_RAW_MODE) && (defined(IN_RC) || defined(IN_RING3))
    PVM         pVM      = pVCpu->CTX_SUFF(pVM);
#endif
    RTUINTPTR   uSrcAddr = pDis->uInstrAddr + offInstr;
    int         rc;

    /*
     * Figure how much we can or must read.
     */
    size_t      cbToRead = PAGE_SIZE - (uSrcAddr & PAGE_OFFSET_MASK);
    if (cbToRead > cbMaxRead)
        cbToRead = cbMaxRead;
    else if (cbToRead < cbMinRead)
        cbToRead = cbMinRead;

#if defined(VBOX_WITH_RAW_MODE) && (defined(IN_RC) || defined(IN_RING3))
    /*
     * We might be called upon to interpret an instruction in a patch.
     */
    if (PATMIsPatchGCAddr(pVM, uSrcAddr))
    {
# ifdef IN_RC
        memcpy(&pDis->abInstr[offInstr], (void *)(uintptr_t)uSrcAddr, cbToRead);
# else
        memcpy(&pDis->abInstr[offInstr], PATMR3GCPtrToHCPtr(pVM, uSrcAddr), cbToRead);
# endif
        rc = VINF_SUCCESS;
    }
    else
#endif
    {
# ifdef IN_RC
        /*
         * Try access it thru the shadow page tables first. Fall back on the
         * slower PGM method if it fails because the TLB or page table was
         * modified recently.
         */
        rc = MMGCRamRead(pVCpu->pVMRC, &pDis->abInstr[offInstr], (void *)(uintptr_t)uSrcAddr, cbToRead);
        if (rc == VERR_ACCESS_DENIED && cbToRead > cbMinRead)
        {
            cbToRead = cbMinRead;
            rc = MMGCRamRead(pVCpu->pVMRC, &pDis->abInstr[offInstr], (void *)(uintptr_t)uSrcAddr, cbToRead);
        }
        if (rc == VERR_ACCESS_DENIED)
#endif
        {
            rc = PGMPhysSimpleReadGCPtr(pVCpu, &pDis->abInstr[offInstr], uSrcAddr, cbToRead);
            if (RT_FAILURE(rc))
            {
                if (cbToRead > cbMinRead)
                {
                    cbToRead = cbMinRead;
                    rc = PGMPhysSimpleReadGCPtr(pVCpu, &pDis->abInstr[offInstr], uSrcAddr, cbToRead);
                }
                if (RT_FAILURE(rc))
                {
#ifndef IN_RC
                    /*
                     * If we fail to find the page via the guest's page tables
                     * we invalidate the page in the host TLB (pertaining to
                     * the guest in the NestedPaging case). See @bugref{6043}.
                     */
                    if (rc == VERR_PAGE_TABLE_NOT_PRESENT || rc == VERR_PAGE_NOT_PRESENT)
                    {
                        HMInvalidatePage(pVCpu, uSrcAddr);
                        if (((uSrcAddr + cbToRead - 1) >> PAGE_SHIFT) !=  (uSrcAddr >> PAGE_SHIFT))
                            HMInvalidatePage(pVCpu, uSrcAddr + cbToRead - 1);
                    }
#endif
                }
            }
        }
    }

    pDis->cbCachedInstr = offInstr + (uint8_t)cbToRead;
    return rc;
}


#if !defined(VBOX_WITH_IEM) || defined(VBOX_COMPARE_IEM_AND_EM)
DECLINLINE(int) emDisCoreOne(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, RTGCUINTPTR InstrGC, uint32_t *pOpsize)
{
    NOREF(pVM);
    return DISInstrWithReader(InstrGC, (DISCPUMODE)pDis->uCpuMode, emReadBytes, pVCpu, pDis, pOpsize);
}
#endif


/**
 * Disassembles the current instruction.
 *
 * @returns VBox status code, see SELMToFlatEx and EMInterpretDisasOneEx for
 *          details.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pDis            Where to return the parsed instruction info.
 * @param   pcbInstr        Where to return the instruction size. (optional)
 */
VMM_INT_DECL(int) EMInterpretDisasCurrent(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, unsigned *pcbInstr)
{
    PCPUMCTXCORE pCtxCore = CPUMCTX2CORE(CPUMQueryGuestCtxPtr(pVCpu));
    RTGCPTR GCPtrInstr;
#if 0
    int rc = SELMToFlatEx(pVCpu, DISSELREG_CS, pCtxCore, pCtxCore->rip, 0, &GCPtrInstr);
#else
/** @todo Get the CPU mode as well while we're at it! */
    int rc = SELMValidateAndConvertCSAddr(pVCpu, pCtxCore->eflags, pCtxCore->ss.Sel, pCtxCore->cs.Sel, &pCtxCore->cs,
                                          pCtxCore->rip, &GCPtrInstr);
#endif
    if (RT_FAILURE(rc))
    {
        Log(("EMInterpretDisasOne: Failed to convert %RTsel:%RGv (cpl=%d) - rc=%Rrc !!\n",
             pCtxCore->cs.Sel, (RTGCPTR)pCtxCore->rip, pCtxCore->ss.Sel & X86_SEL_RPL, rc));
        return rc;
    }
    return EMInterpretDisasOneEx(pVM, pVCpu, (RTGCUINTPTR)GCPtrInstr, pCtxCore, pDis, pcbInstr);
}


/**
 * Disassembles one instruction.
 *
 * This is used by internally by the interpreter and by trap/access handlers.
 *
 * @returns VBox status code.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   GCPtrInstr      The flat address of the instruction.
 * @param   pCtxCore        The context core (used to determine the cpu mode).
 * @param   pDis            Where to return the parsed instruction info.
 * @param   pcbInstr        Where to return the instruction size. (optional)
 */
VMM_INT_DECL(int) EMInterpretDisasOneEx(PVM pVM, PVMCPU pVCpu, RTGCUINTPTR GCPtrInstr, PCCPUMCTXCORE pCtxCore,
                                        PDISCPUSTATE pDis, unsigned *pcbInstr)
{
    NOREF(pVM);
    Assert(pCtxCore == CPUMGetGuestCtxCore(pVCpu)); NOREF(pCtxCore);
    DISCPUMODE enmCpuMode = CPUMGetGuestDisMode(pVCpu);
    /** @todo Deal with too long instruction (=> \#GP), opcode read errors (=>
     *        \#PF, \#GP, \#??), undefined opcodes (=> \#UD), and such. */
    int rc = DISInstrWithReader(GCPtrInstr, enmCpuMode, emReadBytes, pVCpu, pDis, pcbInstr);
    if (RT_SUCCESS(rc))
        return VINF_SUCCESS;
    AssertMsg(rc == VERR_PAGE_NOT_PRESENT || rc == VERR_PAGE_TABLE_NOT_PRESENT, ("DISCoreOne failed to GCPtrInstr=%RGv rc=%Rrc\n", GCPtrInstr, rc));
    return rc;
}


#if defined(VBOX_COMPARE_IEM_FIRST) || defined(VBOX_COMPARE_IEM_LAST)
static void emCompareWithIem(PVMCPU pVCpu, PCCPUMCTX pEmCtx, PCCPUMCTX pIemCtx,
                             VBOXSTRICTRC rcEm, VBOXSTRICTRC rcIem,
                             uint32_t cbEm, uint32_t cbIem)
{
    /* Quick compare. */
    if (   rcEm == rcIem
        && cbEm == cbIem
        && g_cbEmWrote == g_cbIemWrote
        && memcmp(g_abIemWrote, g_abEmWrote, g_cbIemWrote) == 0
        && memcmp(pIemCtx, pEmCtx, sizeof(*pIemCtx)) == 0
        && (g_fEmFFs & g_fInterestingFFs) == (g_fIemFFs & g_fInterestingFFs)
       )
        return;

    /* Report exact differences. */
    RTLogPrintf("! EM and IEM differs at %04x:%08RGv !\n", g_IncomingCtx.cs.Sel, g_IncomingCtx.rip);
    if (rcEm != rcIem)
        RTLogPrintf(" * rcIem=%Rrc rcEm=%Rrc\n", VBOXSTRICTRC_VAL(rcIem), VBOXSTRICTRC_VAL(rcEm));
    else if (cbEm != cbIem)
        RTLogPrintf(" * cbIem=%#x cbEm=%#x\n", cbIem, cbEm);

    if (RT_SUCCESS(rcEm) && RT_SUCCESS(rcIem))
    {
        if (g_cbIemWrote != g_cbEmWrote)
            RTLogPrintf("!! g_cbIemWrote=%#x g_cbEmWrote=%#x\n", g_cbIemWrote, g_cbEmWrote);
        else if (memcmp(g_abIemWrote, g_abEmWrote, g_cbIemWrote))
        {
            RTLogPrintf("!! IemWrote %.*Rhxs\n", RT_MIN(RT_MAX(1, g_cbIemWrote), 64), g_abIemWrote);
            RTLogPrintf("!! EemWrote  %.*Rhxs\n", RT_MIN(RT_MAX(1, g_cbIemWrote), 64), g_abIemWrote);
        }

        if ((g_fEmFFs & g_fInterestingFFs) != (g_fIemFFs & g_fInterestingFFs))
            RTLogPrintf("!! g_fIemFFs=%#x  g_fEmFFs=%#x (diff=%#x)\n", g_fIemFFs & g_fInterestingFFs,
                        g_fEmFFs & g_fInterestingFFs, (g_fIemFFs ^ g_fEmFFs) & g_fInterestingFFs);

#  define CHECK_FIELD(a_Field) \
        do \
        { \
            if (pEmCtx->a_Field != pIemCtx->a_Field) \
            { \
                switch (sizeof(pEmCtx->a_Field)) \
                { \
                    case 1: RTLogPrintf("!! %8s differs - iem=%02x - em=%02x\n", #a_Field, pIemCtx->a_Field, pEmCtx->a_Field); break; \
                    case 2: RTLogPrintf("!! %8s differs - iem=%04x - em=%04x\n", #a_Field, pIemCtx->a_Field, pEmCtx->a_Field); break; \
                    case 4: RTLogPrintf("!! %8s differs - iem=%08x - em=%08x\n", #a_Field, pIemCtx->a_Field, pEmCtx->a_Field); break; \
                    case 8: RTLogPrintf("!! %8s differs - iem=%016llx - em=%016llx\n", #a_Field, pIemCtx->a_Field, pEmCtx->a_Field); break; \
                    default: RTLogPrintf("!!  %8s differs\n", #a_Field); break; \
                } \
                cDiffs++; \
            } \
        } while (0)

#  define CHECK_BIT_FIELD(a_Field) \
        do \
        { \
            if (pEmCtx->a_Field != pIemCtx->a_Field) \
            { \
                RTLogPrintf("!! %8s differs - iem=%02x - em=%02x\n", #a_Field, pIemCtx->a_Field, pEmCtx->a_Field); \
                cDiffs++; \
            } \
        } while (0)

#  define CHECK_SEL(a_Sel) \
        do \
        { \
            CHECK_FIELD(a_Sel.Sel); \
            CHECK_FIELD(a_Sel.Attr.u); \
            CHECK_FIELD(a_Sel.u64Base); \
            CHECK_FIELD(a_Sel.u32Limit); \
            CHECK_FIELD(a_Sel.fFlags); \
        } while (0)

        unsigned cDiffs = 0;
        if (memcmp(&pEmCtx->fpu, &pIemCtx->fpu, sizeof(pIemCtx->fpu)))
        {
            RTLogPrintf("  the FPU state differs\n");
            cDiffs++;
            CHECK_FIELD(fpu.FCW);
            CHECK_FIELD(fpu.FSW);
            CHECK_FIELD(fpu.FTW);
            CHECK_FIELD(fpu.FOP);
            CHECK_FIELD(fpu.FPUIP);
            CHECK_FIELD(fpu.CS);
            CHECK_FIELD(fpu.Rsrvd1);
            CHECK_FIELD(fpu.FPUDP);
            CHECK_FIELD(fpu.DS);
            CHECK_FIELD(fpu.Rsrvd2);
            CHECK_FIELD(fpu.MXCSR);
            CHECK_FIELD(fpu.MXCSR_MASK);
            CHECK_FIELD(fpu.aRegs[0].au64[0]); CHECK_FIELD(fpu.aRegs[0].au64[1]);
            CHECK_FIELD(fpu.aRegs[1].au64[0]); CHECK_FIELD(fpu.aRegs[1].au64[1]);
            CHECK_FIELD(fpu.aRegs[2].au64[0]); CHECK_FIELD(fpu.aRegs[2].au64[1]);
            CHECK_FIELD(fpu.aRegs[3].au64[0]); CHECK_FIELD(fpu.aRegs[3].au64[1]);
            CHECK_FIELD(fpu.aRegs[4].au64[0]); CHECK_FIELD(fpu.aRegs[4].au64[1]);
            CHECK_FIELD(fpu.aRegs[5].au64[0]); CHECK_FIELD(fpu.aRegs[5].au64[1]);
            CHECK_FIELD(fpu.aRegs[6].au64[0]); CHECK_FIELD(fpu.aRegs[6].au64[1]);
            CHECK_FIELD(fpu.aRegs[7].au64[0]); CHECK_FIELD(fpu.aRegs[7].au64[1]);
            CHECK_FIELD(fpu.aXMM[ 0].au64[0]);  CHECK_FIELD(fpu.aXMM[ 0].au64[1]);
            CHECK_FIELD(fpu.aXMM[ 1].au64[0]);  CHECK_FIELD(fpu.aXMM[ 1].au64[1]);
            CHECK_FIELD(fpu.aXMM[ 2].au64[0]);  CHECK_FIELD(fpu.aXMM[ 2].au64[1]);
            CHECK_FIELD(fpu.aXMM[ 3].au64[0]);  CHECK_FIELD(fpu.aXMM[ 3].au64[1]);
            CHECK_FIELD(fpu.aXMM[ 4].au64[0]);  CHECK_FIELD(fpu.aXMM[ 4].au64[1]);
            CHECK_FIELD(fpu.aXMM[ 5].au64[0]);  CHECK_FIELD(fpu.aXMM[ 5].au64[1]);
            CHECK_FIELD(fpu.aXMM[ 6].au64[0]);  CHECK_FIELD(fpu.aXMM[ 6].au64[1]);
            CHECK_FIELD(fpu.aXMM[ 7].au64[0]);  CHECK_FIELD(fpu.aXMM[ 7].au64[1]);
            CHECK_FIELD(fpu.aXMM[ 8].au64[0]);  CHECK_FIELD(fpu.aXMM[ 8].au64[1]);
            CHECK_FIELD(fpu.aXMM[ 9].au64[0]);  CHECK_FIELD(fpu.aXMM[ 9].au64[1]);
            CHECK_FIELD(fpu.aXMM[10].au64[0]);  CHECK_FIELD(fpu.aXMM[10].au64[1]);
            CHECK_FIELD(fpu.aXMM[11].au64[0]);  CHECK_FIELD(fpu.aXMM[11].au64[1]);
            CHECK_FIELD(fpu.aXMM[12].au64[0]);  CHECK_FIELD(fpu.aXMM[12].au64[1]);
            CHECK_FIELD(fpu.aXMM[13].au64[0]);  CHECK_FIELD(fpu.aXMM[13].au64[1]);
            CHECK_FIELD(fpu.aXMM[14].au64[0]);  CHECK_FIELD(fpu.aXMM[14].au64[1]);
            CHECK_FIELD(fpu.aXMM[15].au64[0]);  CHECK_FIELD(fpu.aXMM[15].au64[1]);
            for (unsigned i = 0; i < RT_ELEMENTS(pEmCtx->fpu.au32RsrvdRest); i++)
                CHECK_FIELD(fpu.au32RsrvdRest[i]);
        }
        CHECK_FIELD(rip);
        if (pEmCtx->rflags.u != pIemCtx->rflags.u)
        {
            RTLogPrintf("!! rflags differs - iem=%08llx em=%08llx\n", pIemCtx->rflags.u, pEmCtx->rflags.u);
            CHECK_BIT_FIELD(rflags.Bits.u1CF);
            CHECK_BIT_FIELD(rflags.Bits.u1Reserved0);
            CHECK_BIT_FIELD(rflags.Bits.u1PF);
            CHECK_BIT_FIELD(rflags.Bits.u1Reserved1);
            CHECK_BIT_FIELD(rflags.Bits.u1AF);
            CHECK_BIT_FIELD(rflags.Bits.u1Reserved2);
            CHECK_BIT_FIELD(rflags.Bits.u1ZF);
            CHECK_BIT_FIELD(rflags.Bits.u1SF);
            CHECK_BIT_FIELD(rflags.Bits.u1TF);
            CHECK_BIT_FIELD(rflags.Bits.u1IF);
            CHECK_BIT_FIELD(rflags.Bits.u1DF);
            CHECK_BIT_FIELD(rflags.Bits.u1OF);
            CHECK_BIT_FIELD(rflags.Bits.u2IOPL);
            CHECK_BIT_FIELD(rflags.Bits.u1NT);
            CHECK_BIT_FIELD(rflags.Bits.u1Reserved3);
            CHECK_BIT_FIELD(rflags.Bits.u1RF);
            CHECK_BIT_FIELD(rflags.Bits.u1VM);
            CHECK_BIT_FIELD(rflags.Bits.u1AC);
            CHECK_BIT_FIELD(rflags.Bits.u1VIF);
            CHECK_BIT_FIELD(rflags.Bits.u1VIP);
            CHECK_BIT_FIELD(rflags.Bits.u1ID);
        }

        if (!g_fIgnoreRaxRdx)
            CHECK_FIELD(rax);
        CHECK_FIELD(rcx);
        if (!g_fIgnoreRaxRdx)
            CHECK_FIELD(rdx);
        CHECK_FIELD(rbx);
        CHECK_FIELD(rsp);
        CHECK_FIELD(rbp);
        CHECK_FIELD(rsi);
        CHECK_FIELD(rdi);
        CHECK_FIELD(r8);
        CHECK_FIELD(r9);
        CHECK_FIELD(r10);
        CHECK_FIELD(r11);
        CHECK_FIELD(r12);
        CHECK_FIELD(r13);
        CHECK_SEL(cs);
        CHECK_SEL(ss);
        CHECK_SEL(ds);
        CHECK_SEL(es);
        CHECK_SEL(fs);
        CHECK_SEL(gs);
        CHECK_FIELD(cr0);
        CHECK_FIELD(cr2);
        CHECK_FIELD(cr3);
        CHECK_FIELD(cr4);
        CHECK_FIELD(dr[0]);
        CHECK_FIELD(dr[1]);
        CHECK_FIELD(dr[2]);
        CHECK_FIELD(dr[3]);
        CHECK_FIELD(dr[6]);
        CHECK_FIELD(dr[7]);
        CHECK_FIELD(gdtr.cbGdt);
        CHECK_FIELD(gdtr.pGdt);
        CHECK_FIELD(idtr.cbIdt);
        CHECK_FIELD(idtr.pIdt);
        CHECK_SEL(ldtr);
        CHECK_SEL(tr);
        CHECK_FIELD(SysEnter.cs);
        CHECK_FIELD(SysEnter.eip);
        CHECK_FIELD(SysEnter.esp);
        CHECK_FIELD(msrEFER);
        CHECK_FIELD(msrSTAR);
        CHECK_FIELD(msrPAT);
        CHECK_FIELD(msrLSTAR);
        CHECK_FIELD(msrCSTAR);
        CHECK_FIELD(msrSFMASK);
        CHECK_FIELD(msrKERNELGSBASE);

#  undef CHECK_FIELD
#  undef CHECK_BIT_FIELD
    }
}
#endif /* VBOX_COMPARE_IEM_AND_EM */


/**
 * Interprets the current instruction.
 *
 * @returns VBox status code.
 * @retval  VINF_*                  Scheduling instructions.
 * @retval  VERR_EM_INTERPRETER     Something we can't cope with.
 * @retval  VERR_*                  Fatal errors.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pRegFrame   The register frame.
 *                      Updates the EIP if an instruction was executed successfully.
 * @param   pvFault     The fault address (CR2).
 *
 * @remark  Invalid opcode exceptions have a higher priority than GP (see Intel
 *          Architecture System Developers Manual, Vol 3, 5.5) so we don't need
 *          to worry about e.g. invalid modrm combinations (!)
 */
VMM_INT_DECL(VBOXSTRICTRC) EMInterpretInstruction(PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault)
{
    Assert(pRegFrame == CPUMGetGuestCtxCore(pVCpu));
    LogFlow(("EMInterpretInstruction %RGv fault %RGv\n", (RTGCPTR)pRegFrame->rip, pvFault));
#ifdef VBOX_WITH_IEM
    NOREF(pvFault);

# ifdef VBOX_COMPARE_IEM_AND_EM
    PCPUMCTX pCtx = CPUMQueryGuestCtxPtr(pVCpu);
    g_IncomingCtx = *pCtx;
    g_fIncomingFFs = pVCpu->fLocalForcedActions;
    g_cbEmWrote = g_cbIemWrote = 0;

#  ifdef VBOX_COMPARE_IEM_FIRST
    /* IEM */
    VBOXSTRICTRC rcIem = IEMExecOneBypassEx(pVCpu, pRegFrame, NULL);
    if (RT_UNLIKELY(   rcIem == VERR_IEM_ASPECT_NOT_IMPLEMENTED
                    || rcIem == VERR_IEM_INSTR_NOT_IMPLEMENTED))
        rcIem = VERR_EM_INTERPRETER;
    g_IemCtx = *pCtx;
    g_fIemFFs = pVCpu->fLocalForcedActions;
    pVCpu->fLocalForcedActions = (pVCpu->fLocalForcedActions & ~g_fInterestingFFs) | (g_fIncomingFFs & g_fInterestingFFs);
    *pCtx = g_IncomingCtx;
#  endif

    /* EM */
    RTGCPTR pbCode;
    VBOXSTRICTRC rcEm = SELMToFlatEx(pVCpu, DISSELREG_CS, pRegFrame, pRegFrame->rip, 0, &pbCode);
    if (RT_SUCCESS(rcEm))
    {
        uint32_t     cbOp;
        PDISCPUSTATE pDis = &pVCpu->em.s.DisState;
        pDis->uCpuMode = CPUMGetGuestDisMode(pVCpu);
        rcEm = emDisCoreOne(pVCpu->CTX_SUFF(pVM), pVCpu, pDis, (RTGCUINTPTR)pbCode, &cbOp);
        if (RT_SUCCESS(rcEm))
        {
            Assert(cbOp == pDis->cbInstr);
            uint32_t cbIgnored;
            rcEm = emInterpretInstructionCPUOuter(pVCpu, pDis, pRegFrame, pvFault, EMCODETYPE_SUPERVISOR, &cbIgnored);
            if (RT_SUCCESS(rcEm))
                pRegFrame->rip += cbOp; /* Move on to the next instruction. */

        }
        rcEm = VERR_EM_INTERPRETER;
    }
    else
        rcEm = VERR_EM_INTERPRETER;
#  ifdef VBOX_SAME_AS_EM
    if (rcEm == VERR_EM_INTERPRETER)
    {
        Log(("EMInterpretInstruction: returns %Rrc\n", VBOXSTRICTRC_VAL(rcEm)));
        return rcEm;
    }
#  endif
    g_EmCtx = *pCtx;
    g_fEmFFs = pVCpu->fLocalForcedActions;
    VBOXSTRICTRC rc = rcEm;

#  ifdef VBOX_COMPARE_IEM_LAST
    /* IEM */
    pVCpu->fLocalForcedActions = (pVCpu->fLocalForcedActions & ~g_fInterestingFFs) | (g_fIncomingFFs & g_fInterestingFFs);
    *pCtx = g_IncomingCtx;
    VBOXSTRICTRC rcIem = IEMExecOneBypassEx(pVCpu, pRegFrame, NULL);
    if (RT_UNLIKELY(   rcIem == VERR_IEM_ASPECT_NOT_IMPLEMENTED
                    || rcIem == VERR_IEM_INSTR_NOT_IMPLEMENTED))
        rcIem = VERR_EM_INTERPRETER;
    g_IemCtx = *pCtx;
    g_fIemFFs = pVCpu->fLocalForcedActions;
    rc = rcIem;
#  endif

#  if defined(VBOX_COMPARE_IEM_LAST) || defined(VBOX_COMPARE_IEM_FIRST)
    emCompareWithIem(pVCpu, &g_EmCtx, &g_IemCtx, rcEm, rcIem, 0, 0);
#  endif

# else
    VBOXSTRICTRC rc = IEMExecOneBypassEx(pVCpu, pRegFrame, NULL);
    if (RT_UNLIKELY(   rc == VERR_IEM_ASPECT_NOT_IMPLEMENTED
                    || rc == VERR_IEM_INSTR_NOT_IMPLEMENTED))
        rc = VERR_EM_INTERPRETER;
# endif
    if (rc != VINF_SUCCESS)
        Log(("EMInterpretInstruction: returns %Rrc\n", VBOXSTRICTRC_VAL(rc)));

    return rc;
#else
    RTGCPTR pbCode;
    VBOXSTRICTRC rc = SELMToFlatEx(pVCpu, DISSELREG_CS, pRegFrame, pRegFrame->rip, 0, &pbCode);
    if (RT_SUCCESS(rc))
    {
        uint32_t     cbOp;
        PDISCPUSTATE pDis = &pVCpu->em.s.DisState;
        pDis->uCpuMode = CPUMGetGuestDisMode(pVCpu);
        rc = emDisCoreOne(pVCpu->CTX_SUFF(pVM), pVCpu, pDis, (RTGCUINTPTR)pbCode, &cbOp);
        if (RT_SUCCESS(rc))
        {
            Assert(cbOp == pDis->cbInstr);
            uint32_t cbIgnored;
            rc = emInterpretInstructionCPUOuter(pVCpu, pDis, pRegFrame, pvFault, EMCODETYPE_SUPERVISOR, &cbIgnored);
            if (RT_SUCCESS(rc))
                pRegFrame->rip += cbOp; /* Move on to the next instruction. */

            return rc;
        }
    }
    return VERR_EM_INTERPRETER;
#endif
}


/**
 * Interprets the current instruction.
 *
 * @returns VBox status code.
 * @retval  VINF_*                  Scheduling instructions.
 * @retval  VERR_EM_INTERPRETER     Something we can't cope with.
 * @retval  VERR_*                  Fatal errors.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pRegFrame   The register frame.
 *                      Updates the EIP if an instruction was executed successfully.
 * @param   pvFault     The fault address (CR2).
 * @param   pcbWritten  Size of the write (if applicable).
 *
 * @remark  Invalid opcode exceptions have a higher priority than GP (see Intel
 *          Architecture System Developers Manual, Vol 3, 5.5) so we don't need
 *          to worry about e.g. invalid modrm combinations (!)
 */
VMM_INT_DECL(VBOXSTRICTRC) EMInterpretInstructionEx(PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbWritten)
{
    LogFlow(("EMInterpretInstructionEx %RGv fault %RGv\n", (RTGCPTR)pRegFrame->rip, pvFault));
    Assert(pRegFrame == CPUMGetGuestCtxCore(pVCpu));
#ifdef VBOX_WITH_IEM
    NOREF(pvFault);

# ifdef VBOX_COMPARE_IEM_AND_EM
    PCPUMCTX pCtx = CPUMQueryGuestCtxPtr(pVCpu);
    g_IncomingCtx = *pCtx;
    g_fIncomingFFs = pVCpu->fLocalForcedActions;
    g_cbEmWrote = g_cbIemWrote = 0;

#  ifdef VBOX_COMPARE_IEM_FIRST
    /* IEM */
    uint32_t cbIemWritten = 0;
    VBOXSTRICTRC rcIem = IEMExecOneBypassEx(pVCpu, pRegFrame, &cbIemWritten);
    if (RT_UNLIKELY(   rcIem == VERR_IEM_ASPECT_NOT_IMPLEMENTED
                    || rcIem == VERR_IEM_INSTR_NOT_IMPLEMENTED))
        rcIem = VERR_EM_INTERPRETER;
    g_IemCtx = *pCtx;
    g_fIemFFs = pVCpu->fLocalForcedActions;
    pVCpu->fLocalForcedActions = (pVCpu->fLocalForcedActions & ~g_fInterestingFFs) | (g_fIncomingFFs & g_fInterestingFFs);
    *pCtx = g_IncomingCtx;
#  endif

    /* EM */
    uint32_t cbEmWritten = 0;
    RTGCPTR pbCode;
    VBOXSTRICTRC rcEm = SELMToFlatEx(pVCpu, DISSELREG_CS, pRegFrame, pRegFrame->rip, 0, &pbCode);
    if (RT_SUCCESS(rcEm))
    {
        uint32_t     cbOp;
        PDISCPUSTATE pDis = &pVCpu->em.s.DisState;
        pDis->uCpuMode = CPUMGetGuestDisMode(pVCpu);
        rcEm = emDisCoreOne(pVCpu->CTX_SUFF(pVM), pVCpu, pDis, (RTGCUINTPTR)pbCode, &cbOp);
        if (RT_SUCCESS(rcEm))
        {
            Assert(cbOp == pDis->cbInstr);
            rcEm = emInterpretInstructionCPUOuter(pVCpu, pDis, pRegFrame, pvFault, EMCODETYPE_SUPERVISOR, &cbEmWritten);
            if (RT_SUCCESS(rcEm))
                pRegFrame->rip += cbOp; /* Move on to the next instruction. */

        }
        else
            rcEm = VERR_EM_INTERPRETER;
    }
    else
        rcEm = VERR_EM_INTERPRETER;
#  ifdef VBOX_SAME_AS_EM
    if (rcEm == VERR_EM_INTERPRETER)
    {
        Log(("EMInterpretInstruction: returns %Rrc\n", VBOXSTRICTRC_VAL(rcEm)));
        return rcEm;
    }
#  endif
    g_EmCtx = *pCtx;
    g_fEmFFs = pVCpu->fLocalForcedActions;
    *pcbWritten = cbEmWritten;
    VBOXSTRICTRC rc = rcEm;

#  ifdef VBOX_COMPARE_IEM_LAST
    /* IEM */
    pVCpu->fLocalForcedActions = (pVCpu->fLocalForcedActions & ~g_fInterestingFFs) | (g_fIncomingFFs & g_fInterestingFFs);
    *pCtx = g_IncomingCtx;
    uint32_t cbIemWritten = 0;
    VBOXSTRICTRC rcIem = IEMExecOneBypassEx(pVCpu, pRegFrame, &cbIemWritten);
    if (RT_UNLIKELY(   rcIem == VERR_IEM_ASPECT_NOT_IMPLEMENTED
                    || rcIem == VERR_IEM_INSTR_NOT_IMPLEMENTED))
        rcIem = VERR_EM_INTERPRETER;
    g_IemCtx = *pCtx;
    g_fIemFFs = pVCpu->fLocalForcedActions;
    *pcbWritten = cbIemWritten;
    rc = rcIem;
#  endif

#  if defined(VBOX_COMPARE_IEM_LAST) || defined(VBOX_COMPARE_IEM_FIRST)
    emCompareWithIem(pVCpu, &g_EmCtx, &g_IemCtx, rcEm, rcIem, cbEmWritten, cbIemWritten);
#  endif

# else
    VBOXSTRICTRC rc = IEMExecOneBypassEx(pVCpu, pRegFrame, pcbWritten);
    if (RT_UNLIKELY(   rc == VERR_IEM_ASPECT_NOT_IMPLEMENTED
                    || rc == VERR_IEM_INSTR_NOT_IMPLEMENTED))
        rc = VERR_EM_INTERPRETER;
# endif
    if (rc != VINF_SUCCESS)
        Log(("EMInterpretInstructionEx: returns %Rrc\n", VBOXSTRICTRC_VAL(rc)));

    return rc;
#else
    RTGCPTR pbCode;
    VBOXSTRICTRC rc = SELMToFlatEx(pVCpu, DISSELREG_CS, pRegFrame, pRegFrame->rip, 0, &pbCode);
    if (RT_SUCCESS(rc))
    {
        uint32_t     cbOp;
        PDISCPUSTATE pDis = &pVCpu->em.s.DisState;
        pDis->uCpuMode = CPUMGetGuestDisMode(pVCpu);
        rc = emDisCoreOne(pVCpu->CTX_SUFF(pVM), pVCpu, pDis, (RTGCUINTPTR)pbCode, &cbOp);
        if (RT_SUCCESS(rc))
        {
            Assert(cbOp == pDis->cbInstr);
            rc = emInterpretInstructionCPUOuter(pVCpu, pDis, pRegFrame, pvFault, EMCODETYPE_SUPERVISOR, pcbWritten);
            if (RT_SUCCESS(rc))
                pRegFrame->rip += cbOp; /* Move on to the next instruction. */

            return rc;
        }
    }
    return VERR_EM_INTERPRETER;
#endif
}


/**
 * Interprets the current instruction using the supplied DISCPUSTATE structure.
 *
 * IP/EIP/RIP *IS* updated!
 *
 * @returns VBox strict status code.
 * @retval  VINF_*                  Scheduling instructions. When these are returned, it
 *                                  starts to get a bit tricky to know whether code was
 *                                  executed or not... We'll address this when it becomes a problem.
 * @retval  VERR_EM_INTERPRETER     Something we can't cope with.
 * @retval  VERR_*                  Fatal errors.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pDis        The disassembler cpu state for the instruction to be
 *                      interpreted.
 * @param   pRegFrame   The register frame. IP/EIP/RIP *IS* changed!
 * @param   pvFault     The fault address (CR2).
 * @param   enmCodeType Code type (user/supervisor)
 *
 * @remark  Invalid opcode exceptions have a higher priority than GP (see Intel
 *          Architecture System Developers Manual, Vol 3, 5.5) so we don't need
 *          to worry about e.g. invalid modrm combinations (!)
 *
 * @todo    At this time we do NOT check if the instruction overwrites vital information.
 *          Make sure this can't happen!! (will add some assertions/checks later)
 */
VMM_INT_DECL(VBOXSTRICTRC) EMInterpretInstructionDisasState(PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame,
                                                            RTGCPTR pvFault, EMCODETYPE enmCodeType)
{
    LogFlow(("EMInterpretInstructionDisasState %RGv fault %RGv\n", (RTGCPTR)pRegFrame->rip, pvFault));
    Assert(pRegFrame == CPUMGetGuestCtxCore(pVCpu));
#ifdef VBOX_WITH_IEM
    NOREF(pDis); NOREF(pvFault); NOREF(enmCodeType);

# ifdef VBOX_COMPARE_IEM_AND_EM
    PCPUMCTX pCtx = CPUMQueryGuestCtxPtr(pVCpu);
    g_IncomingCtx = *pCtx;
    g_fIncomingFFs = pVCpu->fLocalForcedActions;
    g_cbEmWrote = g_cbIemWrote = 0;

#  ifdef VBOX_COMPARE_IEM_FIRST
    VBOXSTRICTRC rcIem = IEMExecOneBypassWithPrefetchedByPC(pVCpu, pRegFrame, pRegFrame->rip, pDis->abInstr, pDis->cbCachedInstr);
    if (RT_UNLIKELY(   rcIem == VERR_IEM_ASPECT_NOT_IMPLEMENTED
                    || rcIem == VERR_IEM_INSTR_NOT_IMPLEMENTED))
        rcIem = VERR_EM_INTERPRETER;
    g_IemCtx = *pCtx;
    g_fIemFFs = pVCpu->fLocalForcedActions;
    pVCpu->fLocalForcedActions = (pVCpu->fLocalForcedActions & ~g_fInterestingFFs) | (g_fIncomingFFs & g_fInterestingFFs);
    *pCtx = g_IncomingCtx;
#  endif

    /* EM */
    uint32_t cbIgnored;
    VBOXSTRICTRC rcEm = emInterpretInstructionCPUOuter(pVCpu, pDis, pRegFrame, pvFault, enmCodeType, &cbIgnored);
    if (RT_SUCCESS(rcEm))
        pRegFrame->rip += pDis->cbInstr; /* Move on to the next instruction. */
#  ifdef VBOX_SAME_AS_EM
    if (rcEm == VERR_EM_INTERPRETER)
    {
        Log(("EMInterpretInstruction: returns %Rrc\n", VBOXSTRICTRC_VAL(rcEm)));
        return rcEm;
    }
#  endif
    g_EmCtx = *pCtx;
    g_fEmFFs = pVCpu->fLocalForcedActions;
    VBOXSTRICTRC rc = rcEm;

#  ifdef VBOX_COMPARE_IEM_LAST
    /* IEM */
    pVCpu->fLocalForcedActions = (pVCpu->fLocalForcedActions & ~g_fInterestingFFs) | (g_fIncomingFFs & g_fInterestingFFs);
    *pCtx = g_IncomingCtx;
    VBOXSTRICTRC rcIem = IEMExecOneBypassWithPrefetchedByPC(pVCpu, pRegFrame, pRegFrame->rip, pDis->abInstr, pDis->cbCachedInstr);
    if (RT_UNLIKELY(   rcIem == VERR_IEM_ASPECT_NOT_IMPLEMENTED
                    || rcIem == VERR_IEM_INSTR_NOT_IMPLEMENTED))
        rcIem = VERR_EM_INTERPRETER;
    g_IemCtx = *pCtx;
    g_fIemFFs = pVCpu->fLocalForcedActions;
    rc = rcIem;
#  endif

#  if defined(VBOX_COMPARE_IEM_LAST) || defined(VBOX_COMPARE_IEM_FIRST)
    emCompareWithIem(pVCpu, &g_EmCtx, &g_IemCtx, rcEm, rcIem, 0, 0);
#  endif

# else
    VBOXSTRICTRC rc = IEMExecOneBypassWithPrefetchedByPC(pVCpu, pRegFrame, pRegFrame->rip, pDis->abInstr, pDis->cbCachedInstr);
    if (RT_UNLIKELY(   rc == VERR_IEM_ASPECT_NOT_IMPLEMENTED
                    || rc == VERR_IEM_INSTR_NOT_IMPLEMENTED))
        rc = VERR_EM_INTERPRETER;
# endif

    if (rc != VINF_SUCCESS)
        Log(("EMInterpretInstructionDisasState: returns %Rrc\n", VBOXSTRICTRC_VAL(rc)));

    return rc;
#else
    uint32_t cbIgnored;
    VBOXSTRICTRC rc = emInterpretInstructionCPUOuter(pVCpu, pDis, pRegFrame, pvFault, enmCodeType, &cbIgnored);
    if (RT_SUCCESS(rc))
        pRegFrame->rip += pDis->cbInstr; /* Move on to the next instruction. */
    return rc;
#endif
}

#ifdef IN_RC

DECLINLINE(int) emRCStackRead(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pCtxCore, void *pvDst, RTGCPTR GCPtrSrc, uint32_t cb)
{
    int rc = MMGCRamRead(pVM, pvDst, (void *)(uintptr_t)GCPtrSrc, cb);
    if (RT_LIKELY(rc != VERR_ACCESS_DENIED))
        return rc;
    return PGMPhysInterpretedReadNoHandlers(pVCpu, pCtxCore, pvDst, GCPtrSrc, cb, /*fMayTrap*/ false);
}


/**
 * Interpret IRET (currently only to V86 code) - PATM only.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pRegFrame   The register frame.
 *
 */
VMM_INT_DECL(int) EMInterpretIretV86ForPatm(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame)
{
    RTGCUINTPTR pIretStack = (RTGCUINTPTR)pRegFrame->esp;
    RTGCUINTPTR eip, cs, esp, ss, eflags, ds, es, fs, gs, uMask;
    int         rc;

    Assert(pRegFrame == CPUMGetGuestCtxCore(pVCpu));
    Assert(!CPUMIsGuestIn64BitCode(pVCpu));
    /** @todo Rainy day: Test what happens when VERR_EM_INTERPRETER is returned by
     *        this function.  Fear that it may guru on us, thus not converted to
     *        IEM. */

    rc  = emRCStackRead(pVM, pVCpu, pRegFrame, &eip,      (RTGCPTR)pIretStack      , 4);
    rc |= emRCStackRead(pVM, pVCpu, pRegFrame, &cs,       (RTGCPTR)(pIretStack + 4), 4);
    rc |= emRCStackRead(pVM, pVCpu, pRegFrame, &eflags,   (RTGCPTR)(pIretStack + 8), 4);
    AssertRCReturn(rc, VERR_EM_INTERPRETER);
    AssertReturn(eflags & X86_EFL_VM, VERR_EM_INTERPRETER);

    rc |= emRCStackRead(pVM, pVCpu, pRegFrame, &esp,      (RTGCPTR)(pIretStack + 12), 4);
    rc |= emRCStackRead(pVM, pVCpu, pRegFrame, &ss,       (RTGCPTR)(pIretStack + 16), 4);
    rc |= emRCStackRead(pVM, pVCpu, pRegFrame, &es,       (RTGCPTR)(pIretStack + 20), 4);
    rc |= emRCStackRead(pVM, pVCpu, pRegFrame, &ds,       (RTGCPTR)(pIretStack + 24), 4);
    rc |= emRCStackRead(pVM, pVCpu, pRegFrame, &fs,       (RTGCPTR)(pIretStack + 28), 4);
    rc |= emRCStackRead(pVM, pVCpu, pRegFrame, &gs,       (RTGCPTR)(pIretStack + 32), 4);
    AssertRCReturn(rc, VERR_EM_INTERPRETER);

    pRegFrame->eip    = eip & 0xffff;
    pRegFrame->cs.Sel = cs;

    /* Mask away all reserved bits */
    uMask = X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF | X86_EFL_TF | X86_EFL_IF | X86_EFL_DF | X86_EFL_OF | X86_EFL_IOPL | X86_EFL_NT | X86_EFL_RF | X86_EFL_VM | X86_EFL_AC | X86_EFL_VIF | X86_EFL_VIP | X86_EFL_ID;
    eflags &= uMask;

    CPUMRawSetEFlags(pVCpu, eflags);
    Assert((pRegFrame->eflags.u32 & (X86_EFL_IF|X86_EFL_IOPL)) == X86_EFL_IF);

    pRegFrame->esp      = esp;
    pRegFrame->ss.Sel   = ss;
    pRegFrame->ds.Sel   = ds;
    pRegFrame->es.Sel   = es;
    pRegFrame->fs.Sel   = fs;
    pRegFrame->gs.Sel   = gs;

    return VINF_SUCCESS;
}

# ifndef VBOX_WITH_IEM
/**
 * IRET Emulation.
 */
static int emInterpretIret(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
#ifdef VBOX_WITH_RAW_RING1
    NOREF(pvFault); NOREF(pcbSize); NOREF(pDis);
    if (EMIsRawRing1Enabled(pVM))
    {
        RTGCUINTPTR pIretStack = (RTGCUINTPTR)pRegFrame->esp;
        RTGCUINTPTR eip, cs, esp, ss, eflags, uMask;
        int         rc;
        uint32_t    cpl, rpl;

        /* We only execute 32-bits protected mode code in raw mode, so no need to bother to check for 16-bits code here. */
        /** @todo we don't verify all the edge cases that generate #GP faults */

        Assert(pRegFrame == CPUMGetGuestCtxCore(pVCpu));
        Assert(!CPUMIsGuestIn64BitCode(pVCpu));
        /** @todo Rainy day: Test what happens when VERR_EM_INTERPRETER is returned by
         *        this function.  Fear that it may guru on us, thus not converted to
         *        IEM. */

        rc  = emRCStackRead(pVM, pVCpu, pRegFrame, &eip,      (RTGCPTR)pIretStack      , 4);
        rc |= emRCStackRead(pVM, pVCpu, pRegFrame, &cs,       (RTGCPTR)(pIretStack + 4), 4);
        rc |= emRCStackRead(pVM, pVCpu, pRegFrame, &eflags,   (RTGCPTR)(pIretStack + 8), 4);
        AssertRCReturn(rc, VERR_EM_INTERPRETER);
        AssertReturn(eflags & X86_EFL_VM, VERR_EM_INTERPRETER);

        /* Deal with V86 above. */
        if (eflags & X86_EFL_VM)
            return EMInterpretIretV86ForPatm(pVM, pVCpu, pRegFrame);

        cpl = CPUMRCGetGuestCPL(pVCpu, pRegFrame);
        rpl = cs & X86_SEL_RPL;

        Log(("emInterpretIret: iret to CS:EIP=%04X:%08X eflags=%x\n", cs, eip, eflags));
        if (rpl != cpl)
        {
            rc |= emRCStackRead(pVM, pVCpu, pRegFrame, &esp,      (RTGCPTR)(pIretStack + 12), 4);
            rc |= emRCStackRead(pVM, pVCpu, pRegFrame, &ss,       (RTGCPTR)(pIretStack + 16), 4);
            AssertRCReturn(rc, VERR_EM_INTERPRETER);
            Log(("emInterpretIret: return to different privilege level (rpl=%d cpl=%d)\n", rpl, cpl));
            Log(("emInterpretIret: SS:ESP=%04x:%08x\n", ss, esp));
            pRegFrame->ss.Sel = ss;
            pRegFrame->esp    = esp;
        }
        pRegFrame->cs.Sel = cs;
        pRegFrame->eip    = eip;

        /* Adjust CS & SS as required. */
        CPUMRCRecheckRawState(pVCpu, pRegFrame);

        /* Mask away all reserved bits */
        uMask = X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF | X86_EFL_TF | X86_EFL_IF | X86_EFL_DF | X86_EFL_OF | X86_EFL_IOPL | X86_EFL_NT | X86_EFL_RF | X86_EFL_VM | X86_EFL_AC | X86_EFL_VIF | X86_EFL_VIP | X86_EFL_ID;
        eflags &= uMask;

        CPUMRawSetEFlags(pVCpu, eflags);
        Assert((pRegFrame->eflags.u32 & (X86_EFL_IF|X86_EFL_IOPL)) == X86_EFL_IF);
        return VINF_SUCCESS;
    }
#else
    NOREF(pVM); NOREF(pVCpu); NOREF(pDis); NOREF(pRegFrame); NOREF(pvFault); NOREF(pcbSize);
#endif
    return VERR_EM_INTERPRETER;
}
# endif /* !VBOX_WITH_IEM */

#endif /* IN_RC */



/*
 *
 * Old interpreter primitives used by HM, move/eliminate later.
 * Old interpreter primitives used by HM, move/eliminate later.
 * Old interpreter primitives used by HM, move/eliminate later.
 * Old interpreter primitives used by HM, move/eliminate later.
 * Old interpreter primitives used by HM, move/eliminate later.
 *
 */


/**
 * Interpret CPUID given the parameters in the CPU context.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pRegFrame   The register frame.
 *
 */
VMM_INT_DECL(int) EMInterpretCpuId(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame)
{
    Assert(pRegFrame == CPUMGetGuestCtxCore(pVCpu));
    uint32_t iLeaf    = pRegFrame->eax;
    uint32_t iSubLeaf = pRegFrame->ecx;
    NOREF(pVM);

    /* cpuid clears the high dwords of the affected 64 bits registers. */
    pRegFrame->rax = 0;
    pRegFrame->rbx = 0;
    pRegFrame->rcx = 0;
    pRegFrame->rdx = 0;

    /* Note: operates the same in 64 and non-64 bits mode. */
    CPUMGetGuestCpuId(pVCpu, iLeaf, iSubLeaf, &pRegFrame->eax, &pRegFrame->ebx, &pRegFrame->ecx, &pRegFrame->edx);
    Log(("Emulate: CPUID %x/%x -> %08x %08x %08x %08x\n", iLeaf, iSubLeaf, pRegFrame->eax, pRegFrame->ebx, pRegFrame->ecx, pRegFrame->edx));
    return VINF_SUCCESS;
}


/**
 * Interpret RDTSC.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pRegFrame   The register frame.
 *
 */
VMM_INT_DECL(int) EMInterpretRdtsc(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame)
{
    Assert(pRegFrame == CPUMGetGuestCtxCore(pVCpu));
    unsigned uCR4 = CPUMGetGuestCR4(pVCpu);

    if (uCR4 & X86_CR4_TSD)
        return VERR_EM_INTERPRETER; /* genuine #GP */

    uint64_t uTicks = TMCpuTickGet(pVCpu);

    /* Same behaviour in 32 & 64 bits mode */
    pRegFrame->rax = RT_LO_U32(uTicks);
    pRegFrame->rdx = RT_HI_U32(uTicks);
#ifdef VBOX_COMPARE_IEM_AND_EM
    g_fIgnoreRaxRdx = true;
#endif

    NOREF(pVM);
    return VINF_SUCCESS;
}

/**
 * Interpret RDTSCP.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        The CPU context.
 *
 */
VMM_INT_DECL(int) EMInterpretRdtscp(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx)
{
    Assert(pCtx == CPUMQueryGuestCtxPtr(pVCpu));
    uint32_t uCR4 = CPUMGetGuestCR4(pVCpu);

    if (!pVM->cpum.ro.GuestFeatures.fRdTscP)
    {
        AssertFailed();
        return VERR_EM_INTERPRETER; /* genuine #UD */
    }

    if (uCR4 & X86_CR4_TSD)
        return VERR_EM_INTERPRETER; /* genuine #GP */

    uint64_t uTicks = TMCpuTickGet(pVCpu);

    /* Same behaviour in 32 & 64 bits mode */
    pCtx->rax = RT_LO_U32(uTicks);
    pCtx->rdx = RT_HI_U32(uTicks);
#ifdef VBOX_COMPARE_IEM_AND_EM
    g_fIgnoreRaxRdx = true;
#endif
    /* Low dword of the TSC_AUX msr only. */
    VBOXSTRICTRC rc2 = CPUMQueryGuestMsr(pVCpu, MSR_K8_TSC_AUX, &pCtx->rcx); Assert(rc2 == VINF_SUCCESS); NOREF(rc2);
    pCtx->rcx &= UINT32_C(0xffffffff);

    return VINF_SUCCESS;
}

/**
 * Interpret RDPMC.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pRegFrame   The register frame.
 *
 */
VMM_INT_DECL(int) EMInterpretRdpmc(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame)
{
    Assert(pRegFrame == CPUMGetGuestCtxCore(pVCpu));
    uint32_t uCR4 = CPUMGetGuestCR4(pVCpu);

    /* If X86_CR4_PCE is not set, then CPL must be zero. */
    if (    !(uCR4 & X86_CR4_PCE)
        &&  CPUMGetGuestCPL(pVCpu) != 0)
    {
        Assert(CPUMGetGuestCR0(pVCpu) & X86_CR0_PE);
        return VERR_EM_INTERPRETER; /* genuine #GP */
    }

    /* Just return zero here; rather tricky to properly emulate this, especially as the specs are a mess. */
    pRegFrame->rax = 0;
    pRegFrame->rdx = 0;
    /** @todo We should trigger a \#GP here if the CPU doesn't support the index in
     *        ecx but see @bugref{3472}! */

    NOREF(pVM);
    return VINF_SUCCESS;
}


/**
 * MWAIT Emulation.
 */
VMM_INT_DECL(VBOXSTRICTRC) EMInterpretMWait(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame)
{
    Assert(pRegFrame == CPUMGetGuestCtxCore(pVCpu));
    uint32_t u32Dummy, u32ExtFeatures, cpl, u32MWaitFeatures;
    NOREF(pVM);

    /* Get the current privilege level. */
    cpl = CPUMGetGuestCPL(pVCpu);
    if (cpl != 0)
        return VERR_EM_INTERPRETER; /* supervisor only */

    CPUMGetGuestCpuId(pVCpu, 1, 0, &u32Dummy, &u32Dummy, &u32ExtFeatures, &u32Dummy);
    if (!(u32ExtFeatures & X86_CPUID_FEATURE_ECX_MONITOR))
        return VERR_EM_INTERPRETER; /* not supported */

    /*
     * CPUID.05H.ECX[0] defines support for power management extensions (eax)
     * CPUID.05H.ECX[1] defines support for interrupts as break events for mwait even when IF=0
     */
    CPUMGetGuestCpuId(pVCpu, 5, 0, &u32Dummy, &u32Dummy, &u32MWaitFeatures, &u32Dummy);
    if (pRegFrame->ecx > 1)
    {
        Log(("EMInterpretMWait: unexpected ecx value %x -> recompiler\n", pRegFrame->ecx));
        return VERR_EM_INTERPRETER; /* illegal value. */
    }

    if (pRegFrame->ecx && !(u32MWaitFeatures & X86_CPUID_MWAIT_ECX_BREAKIRQIF0))
    {
        Log(("EMInterpretMWait: unsupported X86_CPUID_MWAIT_ECX_BREAKIRQIF0 -> recompiler\n"));
        return VERR_EM_INTERPRETER; /* illegal value. */
    }

    return EMMonitorWaitPerform(pVCpu, pRegFrame->rax, pRegFrame->rcx);
}


/**
 * MONITOR Emulation.
 */
VMM_INT_DECL(int) EMInterpretMonitor(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame)
{
    uint32_t u32Dummy, u32ExtFeatures, cpl;
    Assert(pRegFrame == CPUMGetGuestCtxCore(pVCpu));
    NOREF(pVM);

    if (pRegFrame->ecx != 0)
    {
        Log(("emInterpretMonitor: unexpected ecx=%x -> recompiler!!\n", pRegFrame->ecx));
        return VERR_EM_INTERPRETER; /* illegal value. */
    }

    /* Get the current privilege level. */
    cpl = CPUMGetGuestCPL(pVCpu);
    if (cpl != 0)
        return VERR_EM_INTERPRETER; /* supervisor only */

    CPUMGetGuestCpuId(pVCpu, 1, 0, &u32Dummy, &u32Dummy, &u32ExtFeatures, &u32Dummy);
    if (!(u32ExtFeatures & X86_CPUID_FEATURE_ECX_MONITOR))
        return VERR_EM_INTERPRETER; /* not supported */

    EMMonitorWaitPrepare(pVCpu, pRegFrame->rax, pRegFrame->rcx, pRegFrame->rdx, NIL_RTGCPHYS);
    return VINF_SUCCESS;
}


/* VT-x only: */

/**
 * Interpret INVLPG.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pRegFrame   The register frame.
 * @param   pAddrGC     Operand address.
 *
 */
VMM_INT_DECL(VBOXSTRICTRC) EMInterpretInvlpg(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, RTGCPTR pAddrGC)
{
    /** @todo is addr always a flat linear address or ds based
     * (in absence of segment override prefixes)????
     */
    Assert(pRegFrame == CPUMGetGuestCtxCore(pVCpu));
    NOREF(pVM); NOREF(pRegFrame);
#ifdef IN_RC
    LogFlow(("RC: EMULATE: invlpg %RGv\n", pAddrGC));
#endif
    VBOXSTRICTRC rc = PGMInvalidatePage(pVCpu, pAddrGC);
    if (    rc == VINF_SUCCESS
        ||  rc == VINF_PGM_SYNC_CR3 /* we can rely on the FF */)
        return VINF_SUCCESS;
    AssertMsgReturn(rc == VINF_EM_RAW_EMULATE_INSTR,
                    ("%Rrc addr=%RGv\n", VBOXSTRICTRC_VAL(rc), pAddrGC),
                    VERR_EM_INTERPRETER);
    return rc;
}


#ifdef LOG_ENABLED
static const char *emMSRtoString(uint32_t uMsr)
{
    switch (uMsr)
    {
        case MSR_IA32_APICBASE:             return "MSR_IA32_APICBASE";
        case MSR_IA32_CR_PAT:               return "MSR_IA32_CR_PAT";
        case MSR_IA32_SYSENTER_CS:          return "MSR_IA32_SYSENTER_CS";
        case MSR_IA32_SYSENTER_EIP:         return "MSR_IA32_SYSENTER_EIP";
        case MSR_IA32_SYSENTER_ESP:         return "MSR_IA32_SYSENTER_ESP";
        case MSR_K6_EFER:                   return "MSR_K6_EFER";
        case MSR_K8_SF_MASK:                return "MSR_K8_SF_MASK";
        case MSR_K6_STAR:                   return "MSR_K6_STAR";
        case MSR_K8_LSTAR:                  return "MSR_K8_LSTAR";
        case MSR_K8_CSTAR:                  return "MSR_K8_CSTAR";
        case MSR_K8_FS_BASE:                return "MSR_K8_FS_BASE";
        case MSR_K8_GS_BASE:                return "MSR_K8_GS_BASE";
        case MSR_K8_KERNEL_GS_BASE:         return "MSR_K8_KERNEL_GS_BASE";
        case MSR_K8_TSC_AUX:                return "MSR_K8_TSC_AUX";
        case MSR_IA32_BIOS_SIGN_ID:         return "Unsupported MSR_IA32_BIOS_SIGN_ID";
        case MSR_IA32_PLATFORM_ID:          return "Unsupported MSR_IA32_PLATFORM_ID";
        case MSR_IA32_BIOS_UPDT_TRIG:       return "Unsupported MSR_IA32_BIOS_UPDT_TRIG";
        case MSR_IA32_TSC:                  return "MSR_IA32_TSC";
        case MSR_IA32_MISC_ENABLE:          return "MSR_IA32_MISC_ENABLE";
        case MSR_IA32_MTRR_CAP:             return "MSR_IA32_MTRR_CAP";
        case MSR_IA32_MCG_CAP:              return "Unsupported MSR_IA32_MCG_CAP";
        case MSR_IA32_MCG_STATUS:           return "Unsupported MSR_IA32_MCG_STATUS";
        case MSR_IA32_MCG_CTRL:             return "Unsupported MSR_IA32_MCG_CTRL";
        case MSR_IA32_MTRR_DEF_TYPE:        return "MSR_IA32_MTRR_DEF_TYPE";
        case MSR_K7_EVNTSEL0:               return "Unsupported MSR_K7_EVNTSEL0";
        case MSR_K7_EVNTSEL1:               return "Unsupported MSR_K7_EVNTSEL1";
        case MSR_K7_EVNTSEL2:               return "Unsupported MSR_K7_EVNTSEL2";
        case MSR_K7_EVNTSEL3:               return "Unsupported MSR_K7_EVNTSEL3";
        case MSR_IA32_MC0_CTL:              return "Unsupported MSR_IA32_MC0_CTL";
        case MSR_IA32_MC0_STATUS:           return "Unsupported MSR_IA32_MC0_STATUS";
        case MSR_IA32_PERFEVTSEL0:          return "Unsupported MSR_IA32_PERFEVTSEL0";
        case MSR_IA32_PERFEVTSEL1:          return "Unsupported MSR_IA32_PERFEVTSEL1";
        case MSR_IA32_PERF_STATUS:          return "MSR_IA32_PERF_STATUS";
        case MSR_IA32_PLATFORM_INFO:        return "MSR_IA32_PLATFORM_INFO";
        case MSR_IA32_PERF_CTL:             return "Unsupported MSR_IA32_PERF_CTL";
        case MSR_K7_PERFCTR0:               return "Unsupported MSR_K7_PERFCTR0";
        case MSR_K7_PERFCTR1:               return "Unsupported MSR_K7_PERFCTR1";
        case MSR_K7_PERFCTR2:               return "Unsupported MSR_K7_PERFCTR2";
        case MSR_K7_PERFCTR3:               return "Unsupported MSR_K7_PERFCTR3";
        case MSR_IA32_PMC0:                 return "Unsupported MSR_IA32_PMC0";
        case MSR_IA32_PMC1:                 return "Unsupported MSR_IA32_PMC1";
        case MSR_IA32_PMC2:                 return "Unsupported MSR_IA32_PMC2";
        case MSR_IA32_PMC3:                 return "Unsupported MSR_IA32_PMC3";
    }
    return "Unknown MSR";
}
#endif /* LOG_ENABLED */


/**
 * Interpret RDMSR
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pRegFrame   The register frame.
 */
VMM_INT_DECL(int) EMInterpretRdmsr(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame)
{
    NOREF(pVM);

    /* Get the current privilege level. */
    if (CPUMGetGuestCPL(pVCpu) != 0)
    {
        Log4(("EM: Refuse RDMSR: CPL != 0\n"));
        return VERR_EM_INTERPRETER; /* supervisor only */
    }

    uint64_t uValue;
    VBOXSTRICTRC rcStrict = CPUMQueryGuestMsr(pVCpu, pRegFrame->ecx, &uValue);
    if (RT_UNLIKELY(rcStrict != VINF_SUCCESS))
    {
        Log4(("EM: Refuse RDMSR: rc=%Rrc\n", VBOXSTRICTRC_VAL(rcStrict)));
        Assert(rcStrict == VERR_CPUM_RAISE_GP_0 || rcStrict == VERR_EM_INTERPRETER || rcStrict == VINF_CPUM_R3_MSR_READ);
        return VERR_EM_INTERPRETER;
    }
    pRegFrame->rax = RT_LO_U32(uValue);
    pRegFrame->rdx = RT_HI_U32(uValue);
    LogFlow(("EMInterpretRdmsr %s (%x) -> %RX64\n", emMSRtoString(pRegFrame->ecx), pRegFrame->ecx, uValue));
    return VINF_SUCCESS;
}


/**
 * Interpret WRMSR
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pRegFrame   The register frame.
 */
VMM_INT_DECL(int) EMInterpretWrmsr(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame)
{
    Assert(pRegFrame == CPUMGetGuestCtxCore(pVCpu));

    /* Check the current privilege level, this instruction is supervisor only. */
    if (CPUMGetGuestCPL(pVCpu) != 0)
    {
        Log4(("EM: Refuse WRMSR: CPL != 0\n"));
        return VERR_EM_INTERPRETER; /** @todo raise \#GP(0) */
    }

    VBOXSTRICTRC rcStrict = CPUMSetGuestMsr(pVCpu, pRegFrame->ecx, RT_MAKE_U64(pRegFrame->eax, pRegFrame->edx));
    if (rcStrict != VINF_SUCCESS)
    {
        Log4(("EM: Refuse WRMSR: CPUMSetGuestMsr returned %Rrc\n",  VBOXSTRICTRC_VAL(rcStrict)));
        Assert(rcStrict == VERR_CPUM_RAISE_GP_0 || rcStrict == VERR_EM_INTERPRETER || rcStrict == VINF_CPUM_R3_MSR_WRITE);
        return VERR_EM_INTERPRETER;
    }
    LogFlow(("EMInterpretWrmsr %s (%x) val=%RX64\n", emMSRtoString(pRegFrame->ecx), pRegFrame->ecx,
             RT_MAKE_U64(pRegFrame->eax, pRegFrame->edx)));
    NOREF(pVM);
    return VINF_SUCCESS;
}


/**
 * Interpret DRx write.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pRegFrame   The register frame.
 * @param   DestRegDrx  DRx register index (USE_REG_DR*)
 * @param   SrcRegGen   General purpose register index (USE_REG_E**))
 *
 */
VMM_INT_DECL(int) EMInterpretDRxWrite(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, uint32_t DestRegDrx, uint32_t SrcRegGen)
{
    Assert(pRegFrame == CPUMGetGuestCtxCore(pVCpu));
    uint64_t uNewDrX;
    int      rc;
    NOREF(pVM);

    if (CPUMIsGuestIn64BitCode(pVCpu))
        rc = DISFetchReg64(pRegFrame, SrcRegGen, &uNewDrX);
    else
    {
        uint32_t val32;
        rc = DISFetchReg32(pRegFrame, SrcRegGen, &val32);
        uNewDrX = val32;
    }

    if (RT_SUCCESS(rc))
    {
        if (DestRegDrx == 6)
        {
            uNewDrX |= X86_DR6_RA1_MASK;
            uNewDrX &= ~X86_DR6_RAZ_MASK;
        }
        else if (DestRegDrx == 7)
        {
            uNewDrX |= X86_DR7_RA1_MASK;
            uNewDrX &= ~X86_DR7_RAZ_MASK;
        }

        /** @todo we don't fail if illegal bits are set/cleared for e.g. dr7 */
        rc = CPUMSetGuestDRx(pVCpu, DestRegDrx, uNewDrX);
        if (RT_SUCCESS(rc))
            return rc;
        AssertMsgFailed(("CPUMSetGuestDRx %d failed\n", DestRegDrx));
    }
    return VERR_EM_INTERPRETER;
}


/**
 * Interpret DRx read.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pRegFrame   The register frame.
 * @param   DestRegGen  General purpose register index (USE_REG_E**))
 * @param   SrcRegDrx   DRx register index (USE_REG_DR*)
 */
VMM_INT_DECL(int) EMInterpretDRxRead(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, uint32_t DestRegGen, uint32_t SrcRegDrx)
{
    uint64_t val64;
    Assert(pRegFrame == CPUMGetGuestCtxCore(pVCpu));
    NOREF(pVM);

    int rc = CPUMGetGuestDRx(pVCpu, SrcRegDrx, &val64);
    AssertMsgRCReturn(rc, ("CPUMGetGuestDRx %d failed\n", SrcRegDrx), VERR_EM_INTERPRETER);
    if (CPUMIsGuestIn64BitCode(pVCpu))
        rc = DISWriteReg64(pRegFrame, DestRegGen, val64);
    else
        rc = DISWriteReg32(pRegFrame, DestRegGen, (uint32_t)val64);

    if (RT_SUCCESS(rc))
        return VINF_SUCCESS;

    return VERR_EM_INTERPRETER;
}


#if !defined(VBOX_WITH_IEM) || defined(VBOX_COMPARE_IEM_AND_EM)






/*
 *
 * The old interpreter.
 * The old interpreter.
 * The old interpreter.
 * The old interpreter.
 * The old interpreter.
 *
 */

DECLINLINE(int) emRamRead(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pCtxCore, void *pvDst, RTGCPTR GCPtrSrc, uint32_t cb)
{
#ifdef IN_RC
    int rc = MMGCRamRead(pVM, pvDst, (void *)(uintptr_t)GCPtrSrc, cb);
    if (RT_LIKELY(rc != VERR_ACCESS_DENIED))
        return rc;
    /*
     * The page pool cache may end up here in some cases because it
     * flushed one of the shadow mappings used by the trapping
     * instruction and it either flushed the TLB or the CPU reused it.
     */
#else
    NOREF(pVM);
#endif
    return PGMPhysInterpretedReadNoHandlers(pVCpu, pCtxCore, pvDst, GCPtrSrc, cb, /*fMayTrap*/ false);
}


DECLINLINE(int) emRamWrite(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pCtxCore, RTGCPTR GCPtrDst, const void *pvSrc, uint32_t cb)
{
    /* Don't use MMGCRamWrite here as it does not respect zero pages, shared
       pages or write monitored pages. */
    NOREF(pVM);
#if !defined(VBOX_COMPARE_IEM_AND_EM) || !defined(VBOX_COMPARE_IEM_LAST)
    int rc = PGMPhysInterpretedWriteNoHandlers(pVCpu, pCtxCore, GCPtrDst, pvSrc, cb, /*fMayTrap*/ false);
#else
    int rc = VINF_SUCCESS;
#endif
#ifdef VBOX_COMPARE_IEM_AND_EM
    Log(("EM Wrote: %RGv %.*Rhxs rc=%Rrc\n", GCPtrDst, RT_MAX(RT_MIN(cb, 64), 1), pvSrc, rc));
    g_cbEmWrote = cb;
    memcpy(g_abEmWrote, pvSrc, RT_MIN(cb, sizeof(g_abEmWrote)));
#endif
    return rc;
}


/** Convert sel:addr to a flat GC address. */
DECLINLINE(RTGCPTR) emConvertToFlatAddr(PVM pVM, PCPUMCTXCORE pRegFrame, PDISCPUSTATE pDis, PDISOPPARAM pParam, RTGCPTR pvAddr)
{
    DISSELREG enmPrefixSeg = DISDetectSegReg(pDis, pParam);
    return SELMToFlat(pVM, enmPrefixSeg, pRegFrame, pvAddr);
}


#if defined(VBOX_STRICT) || defined(LOG_ENABLED)
/**
 * Get the mnemonic for the disassembled instruction.
 *
 * GC/R0 doesn't include the strings in the DIS tables because
 * of limited space.
 */
static const char *emGetMnemonic(PDISCPUSTATE pDis)
{
    switch (pDis->pCurInstr->uOpcode)
    {
        case OP_XCHG:       return "Xchg";
        case OP_DEC:        return "Dec";
        case OP_INC:        return "Inc";
        case OP_POP:        return "Pop";
        case OP_OR:         return "Or";
        case OP_AND:        return "And";
        case OP_MOV:        return "Mov";
        case OP_INVLPG:     return "InvlPg";
        case OP_CPUID:      return "CpuId";
        case OP_MOV_CR:     return "MovCRx";
        case OP_MOV_DR:     return "MovDRx";
        case OP_LLDT:       return "LLdt";
        case OP_LGDT:       return "LGdt";
        case OP_LIDT:       return "LIdt";
        case OP_CLTS:       return "Clts";
        case OP_MONITOR:    return "Monitor";
        case OP_MWAIT:      return "MWait";
        case OP_RDMSR:      return "Rdmsr";
        case OP_WRMSR:      return "Wrmsr";
        case OP_ADD:        return "Add";
        case OP_ADC:        return "Adc";
        case OP_SUB:        return "Sub";
        case OP_SBB:        return "Sbb";
        case OP_RDTSC:      return "Rdtsc";
        case OP_STI:        return "Sti";
        case OP_CLI:        return "Cli";
        case OP_XADD:       return "XAdd";
        case OP_HLT:        return "Hlt";
        case OP_IRET:       return "Iret";
        case OP_MOVNTPS:    return "MovNTPS";
        case OP_STOSWD:     return "StosWD";
        case OP_WBINVD:     return "WbInvd";
        case OP_XOR:        return "Xor";
        case OP_BTR:        return "Btr";
        case OP_BTS:        return "Bts";
        case OP_BTC:        return "Btc";
        case OP_LMSW:       return "Lmsw";
        case OP_SMSW:       return "Smsw";
        case OP_CMPXCHG:    return pDis->fPrefix & DISPREFIX_LOCK ? "Lock CmpXchg"   : "CmpXchg";
        case OP_CMPXCHG8B:  return pDis->fPrefix & DISPREFIX_LOCK ? "Lock CmpXchg8b" : "CmpXchg8b";

        default:
            Log(("Unknown opcode %d\n", pDis->pCurInstr->uOpcode));
            return "???";
    }
}
#endif /* VBOX_STRICT || LOG_ENABLED */


/**
 * XCHG instruction emulation.
 */
static int emInterpretXchg(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    DISQPVPARAMVAL param1, param2;
    NOREF(pvFault);

    /* Source to make DISQueryParamVal read the register value - ugly hack */
    int rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param1, &param1, DISQPVWHICH_SRC);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

    rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param2, &param2, DISQPVWHICH_SRC);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

#ifdef IN_RC
    if (TRPMHasTrap(pVCpu))
    {
        if (TRPMGetErrorCode(pVCpu) & X86_TRAP_PF_RW)
        {
#endif
            RTGCPTR pParam1 = 0, pParam2 = 0;
            uint64_t valpar1, valpar2;

            AssertReturn(pDis->Param1.cb == pDis->Param2.cb, VERR_EM_INTERPRETER);
            switch(param1.type)
            {
            case DISQPV_TYPE_IMMEDIATE: /* register type is translated to this one too */
                valpar1 = param1.val.val64;
                break;

            case DISQPV_TYPE_ADDRESS:
                pParam1 = (RTGCPTR)param1.val.val64;
                pParam1 = emConvertToFlatAddr(pVM, pRegFrame, pDis, &pDis->Param1, pParam1);
                EM_ASSERT_FAULT_RETURN(pParam1 == pvFault, VERR_EM_INTERPRETER);
                rc = emRamRead(pVM, pVCpu, pRegFrame, &valpar1, pParam1, param1.size);
                if (RT_FAILURE(rc))
                {
                    AssertMsgFailed(("MMGCRamRead %RGv size=%d failed with %Rrc\n", pParam1, param1.size, rc));
                    return VERR_EM_INTERPRETER;
                }
                break;

            default:
                AssertFailed();
                return VERR_EM_INTERPRETER;
            }

            switch(param2.type)
            {
            case DISQPV_TYPE_ADDRESS:
                pParam2 = (RTGCPTR)param2.val.val64;
                pParam2 = emConvertToFlatAddr(pVM, pRegFrame, pDis, &pDis->Param2, pParam2);
                EM_ASSERT_FAULT_RETURN(pParam2 == pvFault, VERR_EM_INTERPRETER);
                rc = emRamRead(pVM, pVCpu, pRegFrame, &valpar2, pParam2, param2.size);
                if (RT_FAILURE(rc))
                {
                    AssertMsgFailed(("MMGCRamRead %RGv size=%d failed with %Rrc\n", pParam1, param1.size, rc));
                }
                break;

            case DISQPV_TYPE_IMMEDIATE:
                valpar2 = param2.val.val64;
                break;

            default:
                AssertFailed();
                return VERR_EM_INTERPRETER;
            }

            /* Write value of parameter 2 to parameter 1 (reg or memory address) */
            if (pParam1 == 0)
            {
                Assert(param1.type == DISQPV_TYPE_IMMEDIATE); /* register actually */
                switch(param1.size)
                {
                case 1: //special case for AH etc
                        rc = DISWriteReg8(pRegFrame, pDis->Param1.Base.idxGenReg,  (uint8_t )valpar2); break;
                case 2: rc = DISWriteReg16(pRegFrame, pDis->Param1.Base.idxGenReg, (uint16_t)valpar2); break;
                case 4: rc = DISWriteReg32(pRegFrame, pDis->Param1.Base.idxGenReg, (uint32_t)valpar2); break;
                case 8: rc = DISWriteReg64(pRegFrame, pDis->Param1.Base.idxGenReg, valpar2); break;
                default: AssertFailedReturn(VERR_EM_INTERPRETER);
                }
                if (RT_FAILURE(rc))
                    return VERR_EM_INTERPRETER;
            }
            else
            {
                rc = emRamWrite(pVM, pVCpu, pRegFrame, pParam1, &valpar2, param1.size);
                if (RT_FAILURE(rc))
                {
                    AssertMsgFailed(("emRamWrite %RGv size=%d failed with %Rrc\n", pParam1, param1.size, rc));
                    return VERR_EM_INTERPRETER;
                }
            }

            /* Write value of parameter 1 to parameter 2 (reg or memory address) */
            if (pParam2 == 0)
            {
                Assert(param2.type == DISQPV_TYPE_IMMEDIATE); /* register actually */
                switch(param2.size)
                {
                case 1: //special case for AH etc
                        rc = DISWriteReg8(pRegFrame, pDis->Param2.Base.idxGenReg,  (uint8_t )valpar1);    break;
                case 2: rc = DISWriteReg16(pRegFrame, pDis->Param2.Base.idxGenReg, (uint16_t)valpar1);    break;
                case 4: rc = DISWriteReg32(pRegFrame, pDis->Param2.Base.idxGenReg, (uint32_t)valpar1);    break;
                case 8: rc = DISWriteReg64(pRegFrame, pDis->Param2.Base.idxGenReg, valpar1);              break;
                default: AssertFailedReturn(VERR_EM_INTERPRETER);
                }
                if (RT_FAILURE(rc))
                    return VERR_EM_INTERPRETER;
            }
            else
            {
                rc = emRamWrite(pVM, pVCpu, pRegFrame, pParam2, &valpar1, param2.size);
                if (RT_FAILURE(rc))
                {
                    AssertMsgFailed(("emRamWrite %RGv size=%d failed with %Rrc\n", pParam1, param1.size, rc));
                    return VERR_EM_INTERPRETER;
                }
            }

            *pcbSize = param2.size;
            return VINF_SUCCESS;
#ifdef IN_RC
        }
    }
    return VERR_EM_INTERPRETER;
#endif
}


/**
 * INC and DEC emulation.
 */
static int emInterpretIncDec(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize,
                             PFNEMULATEPARAM2 pfnEmulate)
{
    DISQPVPARAMVAL param1;
    NOREF(pvFault);

    int rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param1, &param1, DISQPVWHICH_DST);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

#ifdef IN_RC
    if (TRPMHasTrap(pVCpu))
    {
        if (TRPMGetErrorCode(pVCpu) & X86_TRAP_PF_RW)
        {
#endif
            RTGCPTR pParam1 = 0;
            uint64_t valpar1;

            if (param1.type == DISQPV_TYPE_ADDRESS)
            {
                pParam1 = (RTGCPTR)param1.val.val64;
                pParam1 = emConvertToFlatAddr(pVM, pRegFrame, pDis, &pDis->Param1, pParam1);
#ifdef IN_RC
                /* Safety check (in theory it could cross a page boundary and fault there though) */
                EM_ASSERT_FAULT_RETURN(pParam1 == pvFault, VERR_EM_INTERPRETER);
#endif
                rc = emRamRead(pVM, pVCpu, pRegFrame,  &valpar1, pParam1, param1.size);
                if (RT_FAILURE(rc))
                {
                    AssertMsgFailed(("emRamRead %RGv size=%d failed with %Rrc\n", pParam1, param1.size, rc));
                    return VERR_EM_INTERPRETER;
                }
            }
            else
            {
                AssertFailed();
                return VERR_EM_INTERPRETER;
            }

            uint32_t eflags;

            eflags = pfnEmulate(&valpar1, param1.size);

            /* Write result back */
            rc = emRamWrite(pVM, pVCpu, pRegFrame, pParam1, &valpar1, param1.size);
            if (RT_FAILURE(rc))
            {
                AssertMsgFailed(("emRamWrite %RGv size=%d failed with %Rrc\n", pParam1, param1.size, rc));
                return VERR_EM_INTERPRETER;
            }

            /* Update guest's eflags and finish. */
            pRegFrame->eflags.u32 = (pRegFrame->eflags.u32   & ~(X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF | X86_EFL_OF))
                                  | (eflags & (X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF | X86_EFL_OF));

            /* All done! */
            *pcbSize = param1.size;
            return VINF_SUCCESS;
#ifdef IN_RC
        }
    }
    return VERR_EM_INTERPRETER;
#endif
}


/**
 * POP Emulation.
 */
static int emInterpretPop(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    Assert(pDis->uCpuMode != DISCPUMODE_64BIT);    /** @todo check */
    DISQPVPARAMVAL param1;
    NOREF(pvFault);

    int rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param1, &param1, DISQPVWHICH_DST);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

#ifdef IN_RC
    if (TRPMHasTrap(pVCpu))
    {
        if (TRPMGetErrorCode(pVCpu) & X86_TRAP_PF_RW)
        {
#endif
            RTGCPTR pParam1 = 0;
            uint32_t valpar1;
            RTGCPTR pStackVal;

            /* Read stack value first */
            if (CPUMGetGuestCodeBits(pVCpu) == 16)
                return VERR_EM_INTERPRETER; /* No legacy 16 bits stuff here, please. */

            /* Convert address; don't bother checking limits etc, as we only read here */
            pStackVal = SELMToFlat(pVM, DISSELREG_SS, pRegFrame, (RTGCPTR)pRegFrame->esp);
            if (pStackVal == 0)
                return VERR_EM_INTERPRETER;

            rc = emRamRead(pVM, pVCpu, pRegFrame, &valpar1, pStackVal, param1.size);
            if (RT_FAILURE(rc))
            {
                AssertMsgFailed(("emRamRead %RGv size=%d failed with %Rrc\n", pParam1, param1.size, rc));
                return VERR_EM_INTERPRETER;
            }

            if (param1.type == DISQPV_TYPE_ADDRESS)
            {
                pParam1 = (RTGCPTR)param1.val.val64;

                /* pop [esp+xx] uses esp after the actual pop! */
                AssertCompile(DISGREG_ESP == DISGREG_SP);
                if (    (pDis->Param1.fUse & DISUSE_BASE)
                    &&  (pDis->Param1.fUse & (DISUSE_REG_GEN16|DISUSE_REG_GEN32))
                    &&  pDis->Param1.Base.idxGenReg == DISGREG_ESP
                   )
                   pParam1 = (RTGCPTR)((RTGCUINTPTR)pParam1 + param1.size);

                pParam1 = emConvertToFlatAddr(pVM, pRegFrame, pDis, &pDis->Param1, pParam1);
                EM_ASSERT_FAULT_RETURN(pParam1 == pvFault || (RTGCPTR)pRegFrame->esp == pvFault, VERR_EM_INTERPRETER);
                rc = emRamWrite(pVM, pVCpu, pRegFrame, pParam1, &valpar1, param1.size);
                if (RT_FAILURE(rc))
                {
                    AssertMsgFailed(("emRamWrite %RGv size=%d failed with %Rrc\n", pParam1, param1.size, rc));
                    return VERR_EM_INTERPRETER;
                }

                /* Update ESP as the last step */
                pRegFrame->esp += param1.size;
            }
            else
            {
#ifndef DEBUG_bird // annoying assertion.
                AssertFailed();
#endif
                return VERR_EM_INTERPRETER;
            }

            /* All done! */
            *pcbSize = param1.size;
            return VINF_SUCCESS;
#ifdef IN_RC
        }
    }
    return VERR_EM_INTERPRETER;
#endif
}


/**
 * XOR/OR/AND Emulation.
 */
static int emInterpretOrXorAnd(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize,
                               PFNEMULATEPARAM3 pfnEmulate)
{
    DISQPVPARAMVAL param1, param2;
    NOREF(pvFault);

    int rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param1, &param1, DISQPVWHICH_DST);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

    rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param2, &param2, DISQPVWHICH_SRC);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

#ifdef IN_RC
    if (TRPMHasTrap(pVCpu))
    {
        if (TRPMGetErrorCode(pVCpu) & X86_TRAP_PF_RW)
        {
#endif
            RTGCPTR  pParam1;
            uint64_t valpar1, valpar2;

            if (pDis->Param1.cb != pDis->Param2.cb)
            {
                if (pDis->Param1.cb < pDis->Param2.cb)
                {
                    AssertMsgFailed(("%s at %RGv parameter mismatch %d vs %d!!\n", emGetMnemonic(pDis), (RTGCPTR)pRegFrame->rip, pDis->Param1.cb, pDis->Param2.cb)); /* should never happen! */
                    return VERR_EM_INTERPRETER;
                }
                /* Or %Ev, Ib -> just a hack to save some space; the data width of the 1st parameter determines the real width */
                pDis->Param2.cb = pDis->Param1.cb;
                param2.size     = param1.size;
            }

            /* The destination is always a virtual address */
            if (param1.type == DISQPV_TYPE_ADDRESS)
            {
                pParam1 = (RTGCPTR)param1.val.val64;
                pParam1 = emConvertToFlatAddr(pVM, pRegFrame, pDis, &pDis->Param1, pParam1);
                EM_ASSERT_FAULT_RETURN(pParam1 == pvFault, VERR_EM_INTERPRETER);
                rc = emRamRead(pVM, pVCpu, pRegFrame, &valpar1, pParam1, param1.size);
                if (RT_FAILURE(rc))
                {
                    AssertMsgFailed(("emRamRead %RGv size=%d failed with %Rrc\n", pParam1, param1.size, rc));
                    return VERR_EM_INTERPRETER;
                }
            }
            else
            {
                AssertFailed();
                return VERR_EM_INTERPRETER;
            }

            /* Register or immediate data */
            switch(param2.type)
            {
            case DISQPV_TYPE_IMMEDIATE:    /* both immediate data and register (ugly) */
                valpar2 = param2.val.val64;
                break;

            default:
                AssertFailed();
                return VERR_EM_INTERPRETER;
            }

            LogFlow(("emInterpretOrXorAnd %s %RGv %RX64 - %RX64 size %d (%d)\n", emGetMnemonic(pDis), pParam1, valpar1, valpar2, param2.size, param1.size));

            /* Data read, emulate instruction. */
            uint32_t eflags = pfnEmulate(&valpar1, valpar2, param2.size);

            LogFlow(("emInterpretOrXorAnd %s result %RX64\n", emGetMnemonic(pDis), valpar1));

            /* Update guest's eflags and finish. */
            pRegFrame->eflags.u32 = (pRegFrame->eflags.u32 & ~(X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF | X86_EFL_OF))
                                  | (eflags                &  (X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF | X86_EFL_OF));

            /* And write it back */
            rc = emRamWrite(pVM, pVCpu, pRegFrame, pParam1, &valpar1, param1.size);
            if (RT_SUCCESS(rc))
            {
                /* All done! */
                *pcbSize = param2.size;
                return VINF_SUCCESS;
            }
#ifdef IN_RC
        }
    }
#endif
    return VERR_EM_INTERPRETER;
}


#ifndef VBOX_COMPARE_IEM_AND_EM
/**
 * LOCK XOR/OR/AND Emulation.
 */
static int emInterpretLockOrXorAnd(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault,
                                   uint32_t *pcbSize, PFNEMULATELOCKPARAM3 pfnEmulate)
{
    void *pvParam1;
    DISQPVPARAMVAL param1, param2;
    NOREF(pvFault);

#if HC_ARCH_BITS == 32
    Assert(pDis->Param1.cb <= 4);
#endif

    int rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param1, &param1, DISQPVWHICH_DST);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

    rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param2, &param2, DISQPVWHICH_SRC);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

    if (pDis->Param1.cb != pDis->Param2.cb)
    {
        AssertMsgReturn(pDis->Param1.cb >= pDis->Param2.cb, /* should never happen! */
                        ("%s at %RGv parameter mismatch %d vs %d!!\n", emGetMnemonic(pDis), (RTGCPTR)pRegFrame->rip, pDis->Param1.cb, pDis->Param2.cb),
                        VERR_EM_INTERPRETER);

        /* Or %Ev, Ib -> just a hack to save some space; the data width of the 1st parameter determines the real width */
        pDis->Param2.cb = pDis->Param1.cb;
        param2.size       = param1.size;
    }

#ifdef IN_RC
    /* Safety check (in theory it could cross a page boundary and fault there though) */
    Assert(   TRPMHasTrap(pVCpu)
           && (TRPMGetErrorCode(pVCpu) & X86_TRAP_PF_RW));
    EM_ASSERT_FAULT_RETURN(GCPtrPar1 == pvFault, VERR_EM_INTERPRETER);
#endif

    /* Register and immediate data == DISQPV_TYPE_IMMEDIATE */
    AssertReturn(param2.type == DISQPV_TYPE_IMMEDIATE, VERR_EM_INTERPRETER);
    RTGCUINTREG ValPar2 = param2.val.val64;

    /* The destination is always a virtual address */
    AssertReturn(param1.type == DISQPV_TYPE_ADDRESS, VERR_EM_INTERPRETER);

    RTGCPTR GCPtrPar1 = param1.val.val64;
    GCPtrPar1 = emConvertToFlatAddr(pVM, pRegFrame, pDis, &pDis->Param1, GCPtrPar1);
    PGMPAGEMAPLOCK Lock;
    rc = PGMPhysGCPtr2CCPtr(pVCpu, GCPtrPar1, &pvParam1, &Lock);
    AssertRCReturn(rc, VERR_EM_INTERPRETER);

    /* Try emulate it with a one-shot #PF handler in place. (RC) */
    Log2(("%s %RGv imm%d=%RX64\n", emGetMnemonic(pDis), GCPtrPar1, pDis->Param2.cb*8, ValPar2));

    RTGCUINTREG32 eflags = 0;
    rc = pfnEmulate(pvParam1, ValPar2, pDis->Param2.cb, &eflags);
    PGMPhysReleasePageMappingLock(pVM, &Lock);
    if (RT_FAILURE(rc))
    {
        Log(("%s %RGv imm%d=%RX64-> emulation failed due to page fault!\n", emGetMnemonic(pDis), GCPtrPar1, pDis->Param2.cb*8, ValPar2));
        return VERR_EM_INTERPRETER;
    }

    /* Update guest's eflags and finish. */
    pRegFrame->eflags.u32 = (pRegFrame->eflags.u32 & ~(X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF | X86_EFL_OF))
                          | (eflags                &  (X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF | X86_EFL_OF));

    *pcbSize = param2.size;
    return VINF_SUCCESS;
}
#endif /* !VBOX_COMPARE_IEM_AND_EM */


/**
 * ADD, ADC & SUB Emulation.
 */
static int emInterpretAddSub(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize,
                             PFNEMULATEPARAM3 pfnEmulate)
{
    NOREF(pvFault);
    DISQPVPARAMVAL param1, param2;
    int rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param1, &param1, DISQPVWHICH_DST);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

    rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param2, &param2, DISQPVWHICH_SRC);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

#ifdef IN_RC
    if (TRPMHasTrap(pVCpu))
    {
        if (TRPMGetErrorCode(pVCpu) & X86_TRAP_PF_RW)
        {
#endif
            RTGCPTR  pParam1;
            uint64_t valpar1, valpar2;

            if (pDis->Param1.cb != pDis->Param2.cb)
            {
                if (pDis->Param1.cb < pDis->Param2.cb)
                {
                    AssertMsgFailed(("%s at %RGv parameter mismatch %d vs %d!!\n", emGetMnemonic(pDis), (RTGCPTR)pRegFrame->rip, pDis->Param1.cb, pDis->Param2.cb)); /* should never happen! */
                    return VERR_EM_INTERPRETER;
                }
                /* Or %Ev, Ib -> just a hack to save some space; the data width of the 1st parameter determines the real width */
                pDis->Param2.cb = pDis->Param1.cb;
                param2.size     = param1.size;
            }

            /* The destination is always a virtual address */
            if (param1.type == DISQPV_TYPE_ADDRESS)
            {
                pParam1 = (RTGCPTR)param1.val.val64;
                pParam1 = emConvertToFlatAddr(pVM, pRegFrame, pDis, &pDis->Param1, pParam1);
                EM_ASSERT_FAULT_RETURN(pParam1 == pvFault, VERR_EM_INTERPRETER);
                rc = emRamRead(pVM, pVCpu, pRegFrame, &valpar1, pParam1, param1.size);
                if (RT_FAILURE(rc))
                {
                    AssertMsgFailed(("emRamRead %RGv size=%d failed with %Rrc\n", pParam1, param1.size, rc));
                    return VERR_EM_INTERPRETER;
                }
            }
            else
            {
#ifndef DEBUG_bird
                AssertFailed();
#endif
                return VERR_EM_INTERPRETER;
            }

            /* Register or immediate data */
            switch(param2.type)
            {
            case DISQPV_TYPE_IMMEDIATE:    /* both immediate data and register (ugly) */
                valpar2 = param2.val.val64;
                break;

            default:
                AssertFailed();
                return VERR_EM_INTERPRETER;
            }

            /* Data read, emulate instruction. */
            uint32_t eflags = pfnEmulate(&valpar1, valpar2, param2.size);

            /* Update guest's eflags and finish. */
            pRegFrame->eflags.u32 = (pRegFrame->eflags.u32 & ~(X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF | X86_EFL_OF))
                                  | (eflags                &  (X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF | X86_EFL_OF));

            /* And write it back */
            rc = emRamWrite(pVM, pVCpu, pRegFrame, pParam1, &valpar1, param1.size);
            if (RT_SUCCESS(rc))
            {
                /* All done! */
                *pcbSize = param2.size;
                return VINF_SUCCESS;
            }
#ifdef IN_RC
        }
    }
#endif
    return VERR_EM_INTERPRETER;
}


/**
 * ADC Emulation.
 */
static int emInterpretAdc(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    if (pRegFrame->eflags.Bits.u1CF)
        return emInterpretAddSub(pVM, pVCpu, pDis, pRegFrame, pvFault, pcbSize, EMEmulateAdcWithCarrySet);
    else
        return emInterpretAddSub(pVM, pVCpu, pDis, pRegFrame, pvFault, pcbSize, EMEmulateAdd);
}


/**
 * BTR/C/S Emulation.
 */
static int emInterpretBitTest(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize,
                              PFNEMULATEPARAM2UINT32 pfnEmulate)
{
    DISQPVPARAMVAL param1, param2;
    int rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param1, &param1, DISQPVWHICH_DST);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

    rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param2, &param2, DISQPVWHICH_SRC);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

#ifdef IN_RC
    if (TRPMHasTrap(pVCpu))
    {
        if (TRPMGetErrorCode(pVCpu) & X86_TRAP_PF_RW)
        {
#endif
            RTGCPTR  pParam1;
            uint64_t valpar1 = 0, valpar2;
            uint32_t eflags;

            /* The destination is always a virtual address */
            if (param1.type != DISQPV_TYPE_ADDRESS)
                return VERR_EM_INTERPRETER;

            pParam1 = (RTGCPTR)param1.val.val64;
            pParam1 = emConvertToFlatAddr(pVM, pRegFrame, pDis, &pDis->Param1, pParam1);

            /* Register or immediate data */
            switch(param2.type)
            {
            case DISQPV_TYPE_IMMEDIATE:    /* both immediate data and register (ugly) */
                valpar2 = param2.val.val64;
                break;

            default:
                AssertFailed();
                return VERR_EM_INTERPRETER;
            }

            Log2(("emInterpret%s: pvFault=%RGv pParam1=%RGv val2=%x\n", emGetMnemonic(pDis), pvFault, pParam1, valpar2));
            pParam1 = (RTGCPTR)((RTGCUINTPTR)pParam1 + valpar2/8);
            EM_ASSERT_FAULT_RETURN((RTGCPTR)((RTGCUINTPTR)pParam1 & ~3) == pvFault, VERR_EM_INTERPRETER); NOREF(pvFault);
            rc = emRamRead(pVM, pVCpu, pRegFrame, &valpar1, pParam1, 1);
            if (RT_FAILURE(rc))
            {
                AssertMsgFailed(("emRamRead %RGv size=%d failed with %Rrc\n", pParam1, param1.size, rc));
                return VERR_EM_INTERPRETER;
            }

            Log2(("emInterpretBtx: val=%x\n", valpar1));
            /* Data read, emulate bit test instruction. */
            eflags = pfnEmulate(&valpar1, valpar2 & 0x7);

            Log2(("emInterpretBtx: val=%x CF=%d\n", valpar1, !!(eflags & X86_EFL_CF)));

            /* Update guest's eflags and finish. */
            pRegFrame->eflags.u32 = (pRegFrame->eflags.u32 & ~(X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF | X86_EFL_OF))
                                  | (eflags                &  (X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF | X86_EFL_OF));

            /* And write it back */
            rc = emRamWrite(pVM, pVCpu, pRegFrame, pParam1, &valpar1, 1);
            if (RT_SUCCESS(rc))
            {
                /* All done! */
                *pcbSize = 1;
                return VINF_SUCCESS;
            }
#ifdef IN_RC
        }
    }
#endif
    return VERR_EM_INTERPRETER;
}


#ifndef VBOX_COMPARE_IEM_AND_EM
/**
 * LOCK BTR/C/S Emulation.
 */
static int emInterpretLockBitTest(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault,
                                  uint32_t *pcbSize, PFNEMULATELOCKPARAM2 pfnEmulate)
{
    void *pvParam1;

    DISQPVPARAMVAL param1, param2;
    int rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param1, &param1, DISQPVWHICH_DST);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

    rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param2, &param2, DISQPVWHICH_SRC);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

    /* The destination is always a virtual address */
    if (param1.type != DISQPV_TYPE_ADDRESS)
        return VERR_EM_INTERPRETER;

    /* Register and immediate data == DISQPV_TYPE_IMMEDIATE */
    AssertReturn(param2.type == DISQPV_TYPE_IMMEDIATE, VERR_EM_INTERPRETER);
    uint64_t ValPar2 = param2.val.val64;

    /* Adjust the parameters so what we're dealing with is a bit within the byte pointed to. */
    RTGCPTR GCPtrPar1 = param1.val.val64;
    GCPtrPar1 = (GCPtrPar1 + ValPar2 / 8);
    ValPar2 &= 7;

    GCPtrPar1 = emConvertToFlatAddr(pVM, pRegFrame, pDis, &pDis->Param1, GCPtrPar1);
#ifdef IN_RC
    Assert(TRPMHasTrap(pVCpu));
    EM_ASSERT_FAULT_RETURN((RTGCPTR)((RTGCUINTPTR)GCPtrPar1 & ~(RTGCUINTPTR)3) == pvFault, VERR_EM_INTERPRETER);
#endif

    PGMPAGEMAPLOCK Lock;
    rc = PGMPhysGCPtr2CCPtr(pVCpu, GCPtrPar1, &pvParam1, &Lock);
    AssertRCReturn(rc, VERR_EM_INTERPRETER);

    Log2(("emInterpretLockBitTest %s: pvFault=%RGv GCPtrPar1=%RGv imm=%RX64\n", emGetMnemonic(pDis), pvFault, GCPtrPar1, ValPar2));
    NOREF(pvFault);

    /* Try emulate it with a one-shot #PF handler in place. (RC) */
    RTGCUINTREG32 eflags = 0;
    rc = pfnEmulate(pvParam1, ValPar2, &eflags);
    PGMPhysReleasePageMappingLock(pVM, &Lock);
    if (RT_FAILURE(rc))
    {
        Log(("emInterpretLockBitTest %s: %RGv imm%d=%RX64 -> emulation failed due to page fault!\n",
             emGetMnemonic(pDis), GCPtrPar1, pDis->Param2.cb*8, ValPar2));
        return VERR_EM_INTERPRETER;
    }

    Log2(("emInterpretLockBitTest %s: GCPtrPar1=%RGv imm=%RX64 CF=%d\n", emGetMnemonic(pDis), GCPtrPar1, ValPar2, !!(eflags & X86_EFL_CF)));

    /* Update guest's eflags and finish. */
    pRegFrame->eflags.u32 = (pRegFrame->eflags.u32 & ~(X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF | X86_EFL_OF))
                          | (eflags                &  (X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF | X86_EFL_OF));

    *pcbSize = 1;
    return VINF_SUCCESS;
}
#endif /* !VBOX_COMPARE_IEM_AND_EM */


/**
 * MOV emulation.
 */
static int emInterpretMov(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    NOREF(pvFault);
    DISQPVPARAMVAL param1, param2;
    int rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param1, &param1, DISQPVWHICH_DST);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

    rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param2, &param2, DISQPVWHICH_SRC);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

    /* If destination is a segment register, punt. We can't handle it here.
     * NB: Source can be a register and still trigger a #PF!
     */
    if (RT_UNLIKELY(pDis->Param1.fUse == DISUSE_REG_SEG))
        return VERR_EM_INTERPRETER;

    if (param1.type == DISQPV_TYPE_ADDRESS)
    {
        RTGCPTR pDest;
        uint64_t val64;

        switch(param1.type)
        {
        case DISQPV_TYPE_IMMEDIATE:
            if(!(param1.flags  & (DISQPV_FLAG_32|DISQPV_FLAG_64)))
                return VERR_EM_INTERPRETER;
            RT_FALL_THRU();

        case DISQPV_TYPE_ADDRESS:
            pDest = (RTGCPTR)param1.val.val64;
            pDest = emConvertToFlatAddr(pVM, pRegFrame, pDis, &pDis->Param1, pDest);
            break;

        default:
            AssertFailed();
            return VERR_EM_INTERPRETER;
        }

        switch(param2.type)
        {
        case DISQPV_TYPE_IMMEDIATE: /* register type is translated to this one too */
            val64 = param2.val.val64;
            break;

        default:
            Log(("emInterpretMov: unexpected type=%d rip=%RGv\n", param2.type, (RTGCPTR)pRegFrame->rip));
            return VERR_EM_INTERPRETER;
        }
#ifdef LOG_ENABLED
        if (pDis->uCpuMode == DISCPUMODE_64BIT)
            LogFlow(("EMInterpretInstruction at %RGv: OP_MOV %RGv <- %RX64 (%d) &val64=%RHv\n", (RTGCPTR)pRegFrame->rip, pDest, val64, param2.size, &val64));
        else
            LogFlow(("EMInterpretInstruction at %08RX64: OP_MOV %RGv <- %08X  (%d) &val64=%RHv\n", pRegFrame->rip, pDest, (uint32_t)val64, param2.size, &val64));
#endif

        Assert(param2.size <= 8 && param2.size > 0);
        EM_ASSERT_FAULT_RETURN(pDest == pvFault, VERR_EM_INTERPRETER);
        rc = emRamWrite(pVM, pVCpu, pRegFrame, pDest, &val64, param2.size);
        if (RT_FAILURE(rc))
            return VERR_EM_INTERPRETER;

        *pcbSize = param2.size;
    }
#if defined(IN_RC) && defined(VBOX_WITH_RAW_RING1)
    /* mov xx, cs instruction is dangerous in raw mode and replaced by an 'int3' by csam/patm. */
    else if (   param1.type == DISQPV_TYPE_REGISTER
             && param2.type == DISQPV_TYPE_REGISTER)
    {
        AssertReturn((pDis->Param1.fUse & (DISUSE_REG_GEN8|DISUSE_REG_GEN16|DISUSE_REG_GEN32)), VERR_EM_INTERPRETER);
        AssertReturn(pDis->Param2.fUse == DISUSE_REG_SEG, VERR_EM_INTERPRETER);
        AssertReturn(pDis->Param2.Base.idxSegReg == DISSELREG_CS, VERR_EM_INTERPRETER);

        uint32_t u32Cpl = CPUMRCGetGuestCPL(pVCpu, pRegFrame);
        uint32_t uValCS = (pRegFrame->cs.Sel & ~X86_SEL_RPL) | u32Cpl;

        Log(("EMInterpretInstruction: OP_MOV cs=%x->%x\n", pRegFrame->cs.Sel, uValCS));
        switch (param1.size)
        {
        case 1: rc = DISWriteReg8(pRegFrame, pDis->Param1.Base.idxGenReg,  (uint8_t) uValCS); break;
        case 2: rc = DISWriteReg16(pRegFrame, pDis->Param1.Base.idxGenReg, (uint16_t)uValCS); break;
        case 4: rc = DISWriteReg32(pRegFrame, pDis->Param1.Base.idxGenReg, (uint32_t)uValCS); break;
        default:
            AssertFailed();
            return VERR_EM_INTERPRETER;
        }
        AssertRCReturn(rc, rc);
    }
#endif
    else
    { /* read fault */
        RTGCPTR pSrc;
        uint64_t val64;

        /* Source */
        switch(param2.type)
        {
        case DISQPV_TYPE_IMMEDIATE:
            if(!(param2.flags & (DISQPV_FLAG_32|DISQPV_FLAG_64)))
                return VERR_EM_INTERPRETER;
            RT_FALL_THRU();

        case DISQPV_TYPE_ADDRESS:
            pSrc = (RTGCPTR)param2.val.val64;
            pSrc = emConvertToFlatAddr(pVM, pRegFrame, pDis, &pDis->Param2, pSrc);
            break;

        default:
            return VERR_EM_INTERPRETER;
        }

        Assert(param1.size <= 8 && param1.size > 0);
        EM_ASSERT_FAULT_RETURN(pSrc == pvFault, VERR_EM_INTERPRETER);
        rc = emRamRead(pVM, pVCpu, pRegFrame, &val64, pSrc, param1.size);
        if (RT_FAILURE(rc))
            return VERR_EM_INTERPRETER;

        /* Destination */
        switch(param1.type)
        {
        case DISQPV_TYPE_REGISTER:
            switch(param1.size)
            {
            case 1: rc = DISWriteReg8(pRegFrame, pDis->Param1.Base.idxGenReg,  (uint8_t) val64); break;
            case 2: rc = DISWriteReg16(pRegFrame, pDis->Param1.Base.idxGenReg, (uint16_t)val64); break;
            case 4: rc = DISWriteReg32(pRegFrame, pDis->Param1.Base.idxGenReg, (uint32_t)val64); break;
            case 8: rc = DISWriteReg64(pRegFrame, pDis->Param1.Base.idxGenReg, val64); break;
            default:
                return VERR_EM_INTERPRETER;
            }
            if (RT_FAILURE(rc))
                return rc;
            break;

        default:
            return VERR_EM_INTERPRETER;
        }
#ifdef LOG_ENABLED
        if (pDis->uCpuMode == DISCPUMODE_64BIT)
            LogFlow(("EMInterpretInstruction: OP_MOV %RGv -> %RX64 (%d)\n", pSrc, val64, param1.size));
        else
            LogFlow(("EMInterpretInstruction: OP_MOV %RGv -> %08X (%d)\n", pSrc, (uint32_t)val64, param1.size));
#endif
    }
    return VINF_SUCCESS;
}


#ifndef IN_RC
/**
 * [REP] STOSWD emulation
 */
static int emInterpretStosWD(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    int      rc;
    RTGCPTR  GCDest, GCOffset;
    uint32_t cbSize;
    uint64_t cTransfers;
    int      offIncrement;
    NOREF(pvFault);

    /* Don't support any but these three prefix bytes. */
    if ((pDis->fPrefix & ~(DISPREFIX_ADDRSIZE|DISPREFIX_OPSIZE|DISPREFIX_REP|DISPREFIX_REX)))
        return VERR_EM_INTERPRETER;

    switch (pDis->uAddrMode)
    {
    case DISCPUMODE_16BIT:
        GCOffset   = pRegFrame->di;
        cTransfers = pRegFrame->cx;
        break;
    case DISCPUMODE_32BIT:
        GCOffset   = pRegFrame->edi;
        cTransfers = pRegFrame->ecx;
        break;
    case DISCPUMODE_64BIT:
        GCOffset   = pRegFrame->rdi;
        cTransfers = pRegFrame->rcx;
        break;
    default:
        AssertFailed();
        return VERR_EM_INTERPRETER;
    }

    GCDest = SELMToFlat(pVM, DISSELREG_ES, pRegFrame, GCOffset);
    switch (pDis->uOpMode)
    {
    case DISCPUMODE_16BIT:
        cbSize = 2;
        break;
    case DISCPUMODE_32BIT:
        cbSize = 4;
        break;
    case DISCPUMODE_64BIT:
        cbSize = 8;
        break;
    default:
        AssertFailed();
        return VERR_EM_INTERPRETER;
    }

    offIncrement = pRegFrame->eflags.Bits.u1DF ? -(signed)cbSize : (signed)cbSize;

    if (!(pDis->fPrefix & DISPREFIX_REP))
    {
        LogFlow(("emInterpretStosWD dest=%04X:%RGv (%RGv) cbSize=%d\n", pRegFrame->es.Sel, GCOffset, GCDest, cbSize));

        rc = emRamWrite(pVM, pVCpu, pRegFrame, GCDest, &pRegFrame->rax, cbSize);
        if (RT_FAILURE(rc))
            return VERR_EM_INTERPRETER;
        Assert(rc == VINF_SUCCESS);

        /* Update (e/r)di. */
        switch (pDis->uAddrMode)
        {
        case DISCPUMODE_16BIT:
            pRegFrame->di  += offIncrement;
            break;
        case DISCPUMODE_32BIT:
            pRegFrame->edi += offIncrement;
            break;
        case DISCPUMODE_64BIT:
            pRegFrame->rdi += offIncrement;
            break;
        default:
            AssertFailed();
            return VERR_EM_INTERPRETER;
        }

    }
    else
    {
        if (!cTransfers)
            return VINF_SUCCESS;

        /*
         * Do *not* try emulate cross page stuff here because we don't know what might
         * be waiting for us on the subsequent pages. The caller has only asked us to
         * ignore access handlers fro the current page.
         * This also fends off big stores which would quickly kill PGMR0DynMap.
         */
        if (    cbSize > PAGE_SIZE
            ||  cTransfers > PAGE_SIZE
            ||  (GCDest >> PAGE_SHIFT) != ((GCDest + offIncrement * cTransfers) >> PAGE_SHIFT))
        {
            Log(("STOSWD is crosses pages, chicken out to the recompiler; GCDest=%RGv cbSize=%#x offIncrement=%d cTransfers=%#x\n",
                 GCDest, cbSize, offIncrement, cTransfers));
            return VERR_EM_INTERPRETER;
        }

        LogFlow(("emInterpretStosWD dest=%04X:%RGv (%RGv) cbSize=%d cTransfers=%x DF=%d\n", pRegFrame->es.Sel, GCOffset, GCDest, cbSize, cTransfers, pRegFrame->eflags.Bits.u1DF));
        /* Access verification first; we currently can't recover properly from traps inside this instruction */
        rc = PGMVerifyAccess(pVCpu, GCDest - ((offIncrement > 0) ? 0 : ((cTransfers-1) * cbSize)),
                             cTransfers * cbSize,
                             X86_PTE_RW | (CPUMGetGuestCPL(pVCpu) == 3 ? X86_PTE_US : 0));
        if (rc != VINF_SUCCESS)
        {
            Log(("STOSWD will generate a trap -> recompiler, rc=%d\n", rc));
            return VERR_EM_INTERPRETER;
        }

        /* REP case */
        while (cTransfers)
        {
            rc = emRamWrite(pVM, pVCpu, pRegFrame, GCDest, &pRegFrame->rax, cbSize);
            if (RT_FAILURE(rc))
            {
                rc = VERR_EM_INTERPRETER;
                break;
            }

            Assert(rc == VINF_SUCCESS);
            GCOffset += offIncrement;
            GCDest   += offIncrement;
            cTransfers--;
        }

        /* Update the registers. */
        switch (pDis->uAddrMode)
        {
        case DISCPUMODE_16BIT:
            pRegFrame->di = GCOffset;
            pRegFrame->cx = cTransfers;
            break;
        case DISCPUMODE_32BIT:
            pRegFrame->edi = GCOffset;
            pRegFrame->ecx = cTransfers;
            break;
        case DISCPUMODE_64BIT:
            pRegFrame->rdi = GCOffset;
            pRegFrame->rcx = cTransfers;
            break;
        default:
            AssertFailed();
            return VERR_EM_INTERPRETER;
        }
    }

    *pcbSize = cbSize;
    return rc;
}
#endif /* !IN_RC */


/**
 * [LOCK] CMPXCHG emulation.
 */
static int emInterpretCmpXchg(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    DISQPVPARAMVAL param1, param2;
    NOREF(pvFault);

#if HC_ARCH_BITS == 32
    Assert(pDis->Param1.cb <= 4);
#endif

    /* Source to make DISQueryParamVal read the register value - ugly hack */
    int rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param1, &param1, DISQPVWHICH_SRC);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

    rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param2, &param2, DISQPVWHICH_SRC);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

    uint64_t valpar;
    switch(param2.type)
    {
    case DISQPV_TYPE_IMMEDIATE: /* register actually */
        valpar = param2.val.val64;
        break;

    default:
        return VERR_EM_INTERPRETER;
    }

    PGMPAGEMAPLOCK Lock;
    RTGCPTR  GCPtrPar1;
    void    *pvParam1;
    uint64_t eflags;

    AssertReturn(pDis->Param1.cb == pDis->Param2.cb, VERR_EM_INTERPRETER);
    switch(param1.type)
    {
    case DISQPV_TYPE_ADDRESS:
        GCPtrPar1 = param1.val.val64;
        GCPtrPar1 = emConvertToFlatAddr(pVM, pRegFrame, pDis, &pDis->Param1, GCPtrPar1);

        rc = PGMPhysGCPtr2CCPtr(pVCpu, GCPtrPar1, &pvParam1, &Lock);
        AssertRCReturn(rc, VERR_EM_INTERPRETER);
        break;

    default:
        return VERR_EM_INTERPRETER;
    }

    LogFlow(("%s %RGv rax=%RX64 %RX64\n", emGetMnemonic(pDis), GCPtrPar1, pRegFrame->rax, valpar));

#ifndef VBOX_COMPARE_IEM_AND_EM
    if (pDis->fPrefix & DISPREFIX_LOCK)
        eflags = EMEmulateLockCmpXchg(pvParam1, &pRegFrame->rax, valpar, pDis->Param2.cb);
    else
        eflags = EMEmulateCmpXchg(pvParam1, &pRegFrame->rax, valpar, pDis->Param2.cb);
#else  /* VBOX_COMPARE_IEM_AND_EM */
    uint64_t u64;
    switch (pDis->Param2.cb)
    {
        case 1: u64 = *(uint8_t  *)pvParam1; break;
        case 2: u64 = *(uint16_t *)pvParam1; break;
        case 4: u64 = *(uint32_t *)pvParam1; break;
        default:
        case 8: u64 = *(uint64_t *)pvParam1; break;
    }
    eflags = EMEmulateCmpXchg(&u64, &pRegFrame->rax, valpar, pDis->Param2.cb);
    int rc2 = emRamWrite(pVM, pVCpu, pRegFrame, GCPtrPar1, &u64, pDis->Param2.cb); AssertRCSuccess(rc2);
#endif /* VBOX_COMPARE_IEM_AND_EM */

    LogFlow(("%s %RGv rax=%RX64 %RX64 ZF=%d\n", emGetMnemonic(pDis), GCPtrPar1, pRegFrame->rax, valpar, !!(eflags & X86_EFL_ZF)));

    /* Update guest's eflags and finish. */
    pRegFrame->eflags.u32 =   (pRegFrame->eflags.u32 & ~(X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF | X86_EFL_OF))
                            | (eflags                &  (X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF | X86_EFL_OF));

    *pcbSize = param2.size;
    PGMPhysReleasePageMappingLock(pVM, &Lock);
    return VINF_SUCCESS;
}


/**
 * [LOCK] CMPXCHG8B emulation.
 */
static int emInterpretCmpXchg8b(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    DISQPVPARAMVAL param1;
    NOREF(pvFault);

    /* Source to make DISQueryParamVal read the register value - ugly hack */
    int rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param1, &param1, DISQPVWHICH_SRC);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

    RTGCPTR  GCPtrPar1;
    void    *pvParam1;
    uint64_t eflags;
    PGMPAGEMAPLOCK Lock;

    AssertReturn(pDis->Param1.cb == 8, VERR_EM_INTERPRETER);
    switch(param1.type)
    {
    case DISQPV_TYPE_ADDRESS:
        GCPtrPar1 = param1.val.val64;
        GCPtrPar1 = emConvertToFlatAddr(pVM, pRegFrame, pDis, &pDis->Param1, GCPtrPar1);

        rc = PGMPhysGCPtr2CCPtr(pVCpu, GCPtrPar1, &pvParam1, &Lock);
        AssertRCReturn(rc, VERR_EM_INTERPRETER);
        break;

    default:
        return VERR_EM_INTERPRETER;
    }

    LogFlow(("%s %RGv=%p eax=%08x\n", emGetMnemonic(pDis), GCPtrPar1, pvParam1, pRegFrame->eax));

#ifndef VBOX_COMPARE_IEM_AND_EM
    if (pDis->fPrefix & DISPREFIX_LOCK)
        eflags = EMEmulateLockCmpXchg8b(pvParam1, &pRegFrame->eax, &pRegFrame->edx, pRegFrame->ebx, pRegFrame->ecx);
    else
        eflags = EMEmulateCmpXchg8b(pvParam1, &pRegFrame->eax, &pRegFrame->edx, pRegFrame->ebx, pRegFrame->ecx);
#else  /* VBOX_COMPARE_IEM_AND_EM */
    uint64_t u64 = *(uint64_t *)pvParam1;
    eflags = EMEmulateCmpXchg8b(&u64, &pRegFrame->eax, &pRegFrame->edx, pRegFrame->ebx, pRegFrame->ecx);
    int rc2 = emRamWrite(pVM, pVCpu, pRegFrame, GCPtrPar1, &u64, sizeof(u64)); AssertRCSuccess(rc2);
#endif /* VBOX_COMPARE_IEM_AND_EM */

    LogFlow(("%s %RGv=%p eax=%08x ZF=%d\n", emGetMnemonic(pDis), GCPtrPar1, pvParam1, pRegFrame->eax, !!(eflags & X86_EFL_ZF)));

    /* Update guest's eflags and finish; note that *only* ZF is affected. */
    pRegFrame->eflags.u32 =   (pRegFrame->eflags.u32 & ~(X86_EFL_ZF))
                            | (eflags                &  (X86_EFL_ZF));

    *pcbSize = 8;
    PGMPhysReleasePageMappingLock(pVM, &Lock);
    return VINF_SUCCESS;
}


#ifdef IN_RC /** @todo test+enable for HM as well. */
/**
 * [LOCK] XADD emulation.
 */
static int emInterpretXAdd(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    Assert(pDis->uCpuMode != DISCPUMODE_64BIT);    /** @todo check */
    DISQPVPARAMVAL param1;
    void *pvParamReg2;
    size_t cbParamReg2;
    NOREF(pvFault);

    /* Source to make DISQueryParamVal read the register value - ugly hack */
    int rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param1, &param1, DISQPVWHICH_SRC);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

    rc = DISQueryParamRegPtr(pRegFrame, pDis, &pDis->Param2, &pvParamReg2, &cbParamReg2);
    Assert(cbParamReg2 <= 4);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

#ifdef IN_RC
    if (TRPMHasTrap(pVCpu))
    {
        if (TRPMGetErrorCode(pVCpu) & X86_TRAP_PF_RW)
        {
#endif
            RTGCPTR         GCPtrPar1;
            void           *pvParam1;
            uint32_t        eflags;
            PGMPAGEMAPLOCK  Lock;

            AssertReturn(pDis->Param1.cb == pDis->Param2.cb, VERR_EM_INTERPRETER);
            switch(param1.type)
            {
            case DISQPV_TYPE_ADDRESS:
                GCPtrPar1 = emConvertToFlatAddr(pVM, pRegFrame, pDis, &pDis->Param1, (RTRCUINTPTR)param1.val.val64);
#ifdef IN_RC
                EM_ASSERT_FAULT_RETURN(GCPtrPar1 == pvFault, VERR_EM_INTERPRETER);
#endif

                rc = PGMPhysGCPtr2CCPtr(pVCpu, GCPtrPar1, &pvParam1, &Lock);
                AssertRCReturn(rc, VERR_EM_INTERPRETER);
                break;

            default:
                return VERR_EM_INTERPRETER;
            }

            LogFlow(("XAdd %RGv=%p reg=%08llx\n", GCPtrPar1, pvParam1, *(uint64_t *)pvParamReg2));

#ifndef VBOX_COMPARE_IEM_AND_EM
            if (pDis->fPrefix & DISPREFIX_LOCK)
                eflags = EMEmulateLockXAdd(pvParam1, pvParamReg2, cbParamReg2);
            else
                eflags = EMEmulateXAdd(pvParam1, pvParamReg2, cbParamReg2);
#else  /* VBOX_COMPARE_IEM_AND_EM */
            uint64_t u64;
            switch (cbParamReg2)
            {
                case 1: u64 = *(uint8_t  *)pvParam1; break;
                case 2: u64 = *(uint16_t *)pvParam1; break;
                case 4: u64 = *(uint32_t *)pvParam1; break;
                default:
                case 8: u64 = *(uint64_t *)pvParam1; break;
            }
            eflags = EMEmulateXAdd(&u64, pvParamReg2, cbParamReg2);
            int rc2 = emRamWrite(pVM, pVCpu, pRegFrame, GCPtrPar1, &u64, pDis->Param2.cb); AssertRCSuccess(rc2);
#endif /* VBOX_COMPARE_IEM_AND_EM */

            LogFlow(("XAdd %RGv=%p reg=%08llx ZF=%d\n", GCPtrPar1, pvParam1, *(uint64_t *)pvParamReg2, !!(eflags & X86_EFL_ZF) ));

            /* Update guest's eflags and finish. */
            pRegFrame->eflags.u32 = (pRegFrame->eflags.u32 & ~(X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF | X86_EFL_OF))
                                  | (eflags                &  (X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF | X86_EFL_OF));

            *pcbSize = cbParamReg2;
            PGMPhysReleasePageMappingLock(pVM, &Lock);
            return VINF_SUCCESS;
#ifdef IN_RC
        }
    }

    return VERR_EM_INTERPRETER;
#endif
}
#endif /* IN_RC */


/**
 * WBINVD Emulation.
 */
static int emInterpretWbInvd(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    /* Nothing to do. */
    NOREF(pVM); NOREF(pVCpu); NOREF(pDis); NOREF(pRegFrame); NOREF(pvFault); NOREF(pcbSize);
    return VINF_SUCCESS;
}


/**
 * INVLPG Emulation.
 */
static VBOXSTRICTRC emInterpretInvlPg(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    DISQPVPARAMVAL param1;
    RTGCPTR     addr;
    NOREF(pvFault); NOREF(pVM); NOREF(pcbSize);

    VBOXSTRICTRC rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param1, &param1, DISQPVWHICH_SRC);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

    switch(param1.type)
    {
    case DISQPV_TYPE_IMMEDIATE:
    case DISQPV_TYPE_ADDRESS:
        if(!(param1.flags  & (DISQPV_FLAG_32|DISQPV_FLAG_64)))
            return VERR_EM_INTERPRETER;
        addr = (RTGCPTR)param1.val.val64;
        break;

    default:
        return VERR_EM_INTERPRETER;
    }

    /** @todo is addr always a flat linear address or ds based
     * (in absence of segment override prefixes)????
     */
#ifdef IN_RC
    LogFlow(("RC: EMULATE: invlpg %RGv\n", addr));
#endif
    rc = PGMInvalidatePage(pVCpu, addr);
    if (    rc == VINF_SUCCESS
        ||  rc == VINF_PGM_SYNC_CR3 /* we can rely on the FF */)
        return VINF_SUCCESS;
    AssertMsgReturn(rc == VINF_EM_RAW_EMULATE_INSTR,
                    ("%Rrc addr=%RGv\n", VBOXSTRICTRC_VAL(rc), addr),
                    VERR_EM_INTERPRETER);
    return rc;
}

/** @todo change all these EMInterpretXXX methods to VBOXSTRICTRC. */

/**
 * CPUID Emulation.
 */
static int emInterpretCpuId(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    NOREF(pVM); NOREF(pVCpu); NOREF(pDis); NOREF(pRegFrame); NOREF(pvFault); NOREF(pcbSize);
    int rc = EMInterpretCpuId(pVM, pVCpu, pRegFrame);
    return rc;
}


/**
 * CLTS Emulation.
 */
static int emInterpretClts(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    NOREF(pVM); NOREF(pDis); NOREF(pRegFrame); NOREF(pvFault); NOREF(pcbSize);

    uint64_t cr0 = CPUMGetGuestCR0(pVCpu);
    if (!(cr0 & X86_CR0_TS))
        return VINF_SUCCESS;
    return CPUMSetGuestCR0(pVCpu, cr0 & ~X86_CR0_TS);
}


/**
 * Update CRx.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pRegFrame   The register frame.
 * @param   DestRegCrx  CRx register index (DISUSE_REG_CR*)
 * @param   val         New CRx value
 *
 */
static int emUpdateCRx(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, uint32_t DestRegCrx, uint64_t val)
{
    uint64_t oldval;
    uint64_t msrEFER;
    uint32_t fValid;
    int      rc, rc2;
    NOREF(pVM);

    /** @todo Clean up this mess. */
    LogFlow(("emInterpretCRxWrite at %RGv CR%d <- %RX64\n", (RTGCPTR)pRegFrame->rip, DestRegCrx, val));
    Assert(pRegFrame == CPUMGetGuestCtxCore(pVCpu));
    switch (DestRegCrx)
    {
    case DISCREG_CR0:
        oldval = CPUMGetGuestCR0(pVCpu);
#ifdef IN_RC
        /* CR0.WP and CR0.AM changes require a reschedule run in ring 3. */
        if (    (val    & (X86_CR0_WP | X86_CR0_AM))
            !=  (oldval & (X86_CR0_WP | X86_CR0_AM)))
            return VERR_EM_INTERPRETER;
#endif
        rc = VINF_SUCCESS;
#if !defined(VBOX_COMPARE_IEM_AND_EM) || !defined(VBOX_COMPARE_IEM_LAST)
        CPUMSetGuestCR0(pVCpu, val);
#else
        CPUMQueryGuestCtxPtr(pVCpu)->cr0 = val | X86_CR0_ET;
#endif
        val = CPUMGetGuestCR0(pVCpu);
        if (    (oldval & (X86_CR0_PG | X86_CR0_WP | X86_CR0_PE))
            !=  (val    & (X86_CR0_PG | X86_CR0_WP | X86_CR0_PE)))
        {
            /* global flush */
            rc = PGMFlushTLB(pVCpu, CPUMGetGuestCR3(pVCpu), true /* global */);
            AssertRCReturn(rc, rc);
        }

        /* Deal with long mode enabling/disabling. */
        msrEFER = CPUMGetGuestEFER(pVCpu);
        if (msrEFER & MSR_K6_EFER_LME)
        {
            if (    !(oldval & X86_CR0_PG)
                &&  (val & X86_CR0_PG))
            {
                /* Illegal to have an active 64 bits CS selector (AMD Arch. Programmer's Manual Volume 2: Table 14-5) */
                if (pRegFrame->cs.Attr.n.u1Long)
                {
                    AssertMsgFailed(("Illegal enabling of paging with CS.u1Long = 1!!\n"));
                    return VERR_EM_INTERPRETER; /** @todo generate \#GP(0) */
                }

                /* Illegal to switch to long mode before activating PAE first (AMD Arch. Programmer's Manual Volume 2: Table 14-5) */
                if (!(CPUMGetGuestCR4(pVCpu) & X86_CR4_PAE))
                {
                    AssertMsgFailed(("Illegal enabling of paging with PAE disabled!!\n"));
                    return VERR_EM_INTERPRETER; /** @todo generate \#GP(0) */
                }
                msrEFER |= MSR_K6_EFER_LMA;
            }
            else
            if (    (oldval & X86_CR0_PG)
                &&  !(val & X86_CR0_PG))
            {
                msrEFER &= ~MSR_K6_EFER_LMA;
                /** @todo Do we need to cut off rip here? High dword of rip is undefined, so it shouldn't really matter. */
            }
            CPUMSetGuestEFER(pVCpu, msrEFER);
        }
        rc2 = PGMChangeMode(pVCpu, CPUMGetGuestCR0(pVCpu), CPUMGetGuestCR4(pVCpu), CPUMGetGuestEFER(pVCpu));
        return rc2 == VINF_SUCCESS ? rc : rc2;

    case DISCREG_CR2:
        rc = CPUMSetGuestCR2(pVCpu, val); AssertRC(rc);
        return VINF_SUCCESS;

    case DISCREG_CR3:
        /* Reloading the current CR3 means the guest just wants to flush the TLBs */
        rc = CPUMSetGuestCR3(pVCpu, val); AssertRC(rc);
        if (CPUMGetGuestCR0(pVCpu) & X86_CR0_PG)
        {
            /* flush */
            rc = PGMFlushTLB(pVCpu, val, !(CPUMGetGuestCR4(pVCpu) & X86_CR4_PGE));
            AssertRC(rc);
        }
        return rc;

    case DISCREG_CR4:
        oldval = CPUMGetGuestCR4(pVCpu);
        rc = CPUMSetGuestCR4(pVCpu, val); AssertRC(rc);
        val = CPUMGetGuestCR4(pVCpu);

        /* Illegal to disable PAE when long mode is active. (AMD Arch. Programmer's Manual Volume 2: Table 14-5) */
        msrEFER = CPUMGetGuestEFER(pVCpu);
        if (    (msrEFER & MSR_K6_EFER_LMA)
            &&  (oldval & X86_CR4_PAE)
            &&  !(val & X86_CR4_PAE))
        {
            return VERR_EM_INTERPRETER; /** @todo generate \#GP(0) */
        }

        /* From IEM iemCImpl_load_CrX. */
        /** @todo Check guest CPUID bits for determining corresponding valid bits. */
        fValid = X86_CR4_VME | X86_CR4_PVI
               | X86_CR4_TSD | X86_CR4_DE
               | X86_CR4_PSE | X86_CR4_PAE
               | X86_CR4_MCE | X86_CR4_PGE
               | X86_CR4_PCE | X86_CR4_OSFXSR
               | X86_CR4_OSXMMEEXCPT;
        //if (xxx)
        //    fValid |= X86_CR4_VMXE;
        //if (xxx)
        //    fValid |= X86_CR4_OSXSAVE;
        if (val & ~(uint64_t)fValid)
        {
            Log(("Trying to set reserved CR4 bits: NewCR4=%#llx InvalidBits=%#llx\n", val, val & ~(uint64_t)fValid));
            return VERR_EM_INTERPRETER; /** @todo generate \#GP(0) */
        }

        rc = VINF_SUCCESS;
        if (    (oldval & (X86_CR4_PGE|X86_CR4_PAE|X86_CR4_PSE))
            !=  (val    & (X86_CR4_PGE|X86_CR4_PAE|X86_CR4_PSE)))
        {
            /* global flush */
            rc = PGMFlushTLB(pVCpu, CPUMGetGuestCR3(pVCpu), true /* global */);
            AssertRCReturn(rc, rc);
        }

        /* Feeling extremely lazy. */
# ifdef IN_RC
        if (    (oldval & (X86_CR4_OSFXSR|X86_CR4_OSXMMEEXCPT|X86_CR4_PCE|X86_CR4_MCE|X86_CR4_PAE|X86_CR4_DE|X86_CR4_TSD|X86_CR4_PVI|X86_CR4_VME))
            !=  (val    & (X86_CR4_OSFXSR|X86_CR4_OSXMMEEXCPT|X86_CR4_PCE|X86_CR4_MCE|X86_CR4_PAE|X86_CR4_DE|X86_CR4_TSD|X86_CR4_PVI|X86_CR4_VME)))
        {
            Log(("emInterpretMovCRx: CR4: %#RX64->%#RX64 => R3\n", oldval, val));
            VMCPU_FF_SET(pVCpu, VMCPU_FF_TO_R3);
        }
# endif
# ifdef VBOX_WITH_RAW_MODE
        if (((val ^ oldval) & X86_CR4_VME) && !HMIsEnabled(pVM))
            VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_TSS);
# endif

        rc2 = PGMChangeMode(pVCpu, CPUMGetGuestCR0(pVCpu), CPUMGetGuestCR4(pVCpu), CPUMGetGuestEFER(pVCpu));
        return rc2 == VINF_SUCCESS ? rc : rc2;

    case DISCREG_CR8:
        return APICSetTpr(pVCpu, val << 4);  /* cr8 bits 3-0 correspond to bits 7-4 of the task priority mmio register. */

    default:
        AssertFailed();
    case DISCREG_CR1: /* illegal op */
        break;
    }
    return VERR_EM_INTERPRETER;
}


/**
 * LMSW Emulation.
 */
static int emInterpretLmsw(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    DISQPVPARAMVAL param1;
    uint32_t    val;
    NOREF(pvFault); NOREF(pcbSize);
    Assert(pRegFrame == CPUMGetGuestCtxCore(pVCpu));

    int rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param1, &param1, DISQPVWHICH_SRC);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

    switch(param1.type)
    {
    case DISQPV_TYPE_IMMEDIATE:
    case DISQPV_TYPE_ADDRESS:
        if(!(param1.flags  & DISQPV_FLAG_16))
            return VERR_EM_INTERPRETER;
        val = param1.val.val32;
        break;

    default:
        return VERR_EM_INTERPRETER;
    }

    LogFlow(("emInterpretLmsw %x\n", val));
    uint64_t OldCr0 = CPUMGetGuestCR0(pVCpu);

    /* Only PE, MP, EM and TS can be changed; note that PE can't be cleared by this instruction. */
    uint64_t NewCr0 = ( OldCr0 & ~(             X86_CR0_MP | X86_CR0_EM | X86_CR0_TS))
                    | (val     &  (X86_CR0_PE | X86_CR0_MP | X86_CR0_EM | X86_CR0_TS));

    return emUpdateCRx(pVM, pVCpu, pRegFrame, DISCREG_CR0, NewCr0);

}

#ifdef EM_EMULATE_SMSW
/**
 * SMSW Emulation.
 */
static int emInterpretSmsw(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    NOREF(pvFault); NOREF(pcbSize);
    DISQPVPARAMVAL param1;
    uint64_t    cr0 = CPUMGetGuestCR0(pVCpu);

    int rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param1, &param1, DISQPVWHICH_SRC);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

    switch(param1.type)
    {
    case DISQPV_TYPE_IMMEDIATE:
        if(param1.size != sizeof(uint16_t))
            return VERR_EM_INTERPRETER;
        LogFlow(("emInterpretSmsw %d <- cr0 (%x)\n", pDis->Param1.Base.idxGenReg, cr0));
        rc = DISWriteReg16(pRegFrame, pDis->Param1.Base.idxGenReg, cr0);
        break;

    case DISQPV_TYPE_ADDRESS:
    {
        RTGCPTR pParam1;

        /* Actually forced to 16 bits regardless of the operand size. */
        if(param1.size != sizeof(uint16_t))
            return VERR_EM_INTERPRETER;

        pParam1 = (RTGCPTR)param1.val.val64;
        pParam1 = emConvertToFlatAddr(pVM, pRegFrame, pDis, &pDis->Param1, pParam1);
        LogFlow(("emInterpretSmsw %RGv <- cr0 (%x)\n", pParam1, cr0));

        rc = emRamWrite(pVM, pVCpu, pRegFrame, pParam1, &cr0, sizeof(uint16_t));
        if (RT_FAILURE(rc))
        {
            AssertMsgFailed(("emRamWrite %RGv size=%d failed with %Rrc\n", pParam1, param1.size, rc));
            return VERR_EM_INTERPRETER;
        }
        break;
    }

    default:
        return VERR_EM_INTERPRETER;
    }

    LogFlow(("emInterpretSmsw %x\n", cr0));
    return rc;
}
#endif


/**
 * Interpret CRx read.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pRegFrame   The register frame.
 * @param   DestRegGen  General purpose register index (USE_REG_E**))
 * @param   SrcRegCrx   CRx register index (DISUSE_REG_CR*)
 *
 */
static int emInterpretCRxRead(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, uint32_t DestRegGen, uint32_t SrcRegCrx)
{
    Assert(pRegFrame == CPUMGetGuestCtxCore(pVCpu));
    uint64_t val64;
    int rc = CPUMGetGuestCRx(pVCpu, SrcRegCrx, &val64);
    AssertMsgRCReturn(rc, ("CPUMGetGuestCRx %d failed\n", SrcRegCrx), VERR_EM_INTERPRETER);
    NOREF(pVM);

    if (CPUMIsGuestIn64BitCode(pVCpu))
        rc = DISWriteReg64(pRegFrame, DestRegGen, val64);
    else
        rc = DISWriteReg32(pRegFrame, DestRegGen, val64);

    if (RT_SUCCESS(rc))
    {
        LogFlow(("MOV_CR: gen32=%d CR=%d val=%RX64\n", DestRegGen, SrcRegCrx, val64));
        return VINF_SUCCESS;
    }
    return VERR_EM_INTERPRETER;
}


/**
 * Interpret CRx write.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pRegFrame   The register frame.
 * @param   DestRegCrx  CRx register index (DISUSE_REG_CR*)
 * @param   SrcRegGen   General purpose register index (USE_REG_E**))
 *
 */
static int emInterpretCRxWrite(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, uint32_t DestRegCrx, uint32_t SrcRegGen)
{
    uint64_t val;
    int      rc;
    Assert(pRegFrame == CPUMGetGuestCtxCore(pVCpu));

    if (CPUMIsGuestIn64BitCode(pVCpu))
        rc = DISFetchReg64(pRegFrame, SrcRegGen, &val);
    else
    {
        uint32_t val32;
        rc = DISFetchReg32(pRegFrame, SrcRegGen, &val32);
        val = val32;
    }

    if (RT_SUCCESS(rc))
        return emUpdateCRx(pVM, pVCpu, pRegFrame, DestRegCrx, val);

    return VERR_EM_INTERPRETER;
}


/**
 * MOV CRx
 */
static int emInterpretMovCRx(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    NOREF(pvFault); NOREF(pcbSize);
    if ((pDis->Param1.fUse == DISUSE_REG_GEN32 || pDis->Param1.fUse == DISUSE_REG_GEN64) && pDis->Param2.fUse == DISUSE_REG_CR)
        return emInterpretCRxRead(pVM, pVCpu, pRegFrame, pDis->Param1.Base.idxGenReg, pDis->Param2.Base.idxCtrlReg);

    if (pDis->Param1.fUse == DISUSE_REG_CR && (pDis->Param2.fUse == DISUSE_REG_GEN32 || pDis->Param2.fUse == DISUSE_REG_GEN64))
        return emInterpretCRxWrite(pVM, pVCpu, pRegFrame, pDis->Param1.Base.idxCtrlReg, pDis->Param2.Base.idxGenReg);

    AssertMsgFailedReturn(("Unexpected control register move\n"), VERR_EM_INTERPRETER);
}


/**
 * MOV DRx
 */
static int emInterpretMovDRx(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    int rc = VERR_EM_INTERPRETER;
    NOREF(pvFault); NOREF(pcbSize);

    if((pDis->Param1.fUse == DISUSE_REG_GEN32 || pDis->Param1.fUse == DISUSE_REG_GEN64) && pDis->Param2.fUse == DISUSE_REG_DBG)
    {
        rc = EMInterpretDRxRead(pVM, pVCpu, pRegFrame, pDis->Param1.Base.idxGenReg, pDis->Param2.Base.idxDbgReg);
    }
    else
    if(pDis->Param1.fUse == DISUSE_REG_DBG && (pDis->Param2.fUse == DISUSE_REG_GEN32 || pDis->Param2.fUse == DISUSE_REG_GEN64))
    {
        rc = EMInterpretDRxWrite(pVM, pVCpu, pRegFrame, pDis->Param1.Base.idxDbgReg, pDis->Param2.Base.idxGenReg);
    }
    else
        AssertMsgFailed(("Unexpected debug register move\n"));

    return rc;
}


/**
 * LLDT Emulation.
 */
static int emInterpretLLdt(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    DISQPVPARAMVAL param1;
    RTSEL       sel;
    NOREF(pVM); NOREF(pvFault); NOREF(pcbSize);

    int rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param1, &param1, DISQPVWHICH_SRC);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

    switch(param1.type)
    {
    case DISQPV_TYPE_ADDRESS:
        return VERR_EM_INTERPRETER; //feeling lazy right now

    case DISQPV_TYPE_IMMEDIATE:
        if(!(param1.flags  & DISQPV_FLAG_16))
            return VERR_EM_INTERPRETER;
        sel = (RTSEL)param1.val.val16;
        break;

    default:
        return VERR_EM_INTERPRETER;
    }

#ifdef IN_RING0
    /* Only for the VT-x real-mode emulation case. */
    AssertReturn(CPUMIsGuestInRealMode(pVCpu), VERR_EM_INTERPRETER);
    CPUMSetGuestLDTR(pVCpu, sel);
    return VINF_SUCCESS;
#else
    if (sel == 0)
    {
        if (CPUMGetHyperLDTR(pVCpu) == 0)
        {
            // this simple case is most frequent in Windows 2000 (31k - boot & shutdown)
            return VINF_SUCCESS;
        }
    }
    //still feeling lazy
    return VERR_EM_INTERPRETER;
#endif
}

#ifdef IN_RING0
/**
 * LIDT/LGDT Emulation.
 */
static int emInterpretLIGdt(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    DISQPVPARAMVAL param1;
    RTGCPTR     pParam1;
    X86XDTR32   dtr32;
    NOREF(pvFault); NOREF(pcbSize);

    Log(("Emulate %s at %RGv\n", emGetMnemonic(pDis), (RTGCPTR)pRegFrame->rip));

    /* Only for the VT-x real-mode emulation case. */
    AssertReturn(CPUMIsGuestInRealMode(pVCpu), VERR_EM_INTERPRETER);

    int rc = DISQueryParamVal(pRegFrame, pDis, &pDis->Param1, &param1, DISQPVWHICH_SRC);
    if(RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

    switch(param1.type)
    {
    case DISQPV_TYPE_ADDRESS:
        pParam1 = emConvertToFlatAddr(pVM, pRegFrame, pDis, &pDis->Param1, param1.val.val16);
        break;

    default:
        return VERR_EM_INTERPRETER;
    }

    rc = emRamRead(pVM, pVCpu, pRegFrame, &dtr32, pParam1, sizeof(dtr32));
    AssertRCReturn(rc, VERR_EM_INTERPRETER);

    if (!(pDis->fPrefix & DISPREFIX_OPSIZE))
        dtr32.uAddr &= 0xffffff; /* 16 bits operand size */

    if (pDis->pCurInstr->uOpcode == OP_LIDT)
        CPUMSetGuestIDTR(pVCpu, dtr32.uAddr, dtr32.cb);
    else
        CPUMSetGuestGDTR(pVCpu, dtr32.uAddr, dtr32.cb);

    return VINF_SUCCESS;
}
#endif


#ifdef IN_RC
/**
 * STI Emulation.
 *
 * @remark the instruction following sti is guaranteed to be executed before any interrupts are dispatched
 */
static int emInterpretSti(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    NOREF(pcbSize);
    PPATMGCSTATE pGCState = PATMGetGCState(pVM);

    if(!pGCState)
    {
        Assert(pGCState);
        return VERR_EM_INTERPRETER;
    }
    pGCState->uVMFlags |= X86_EFL_IF;

    Assert(pRegFrame->eflags.u32 & X86_EFL_IF);
    Assert(pvFault == SELMToFlat(pVM, DISSELREG_CS, pRegFrame, (RTGCPTR)pRegFrame->rip));

    pVCpu->em.s.GCPtrInhibitInterrupts = pRegFrame->eip + pDis->cbInstr;
    VMCPU_FF_SET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS);

    return VINF_SUCCESS;
}
#endif /* IN_RC */


/**
 * HLT Emulation.
 */
static VBOXSTRICTRC
emInterpretHlt(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    NOREF(pVM); NOREF(pVCpu); NOREF(pDis); NOREF(pRegFrame); NOREF(pvFault); NOREF(pcbSize);
    return VINF_EM_HALT;
}


/**
 * RDTSC Emulation.
 */
static int emInterpretRdtsc(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    NOREF(pDis); NOREF(pvFault); NOREF(pcbSize);
    return EMInterpretRdtsc(pVM, pVCpu, pRegFrame);
}

/**
 * RDPMC Emulation
 */
static int emInterpretRdpmc(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    NOREF(pDis); NOREF(pvFault); NOREF(pcbSize);
    return EMInterpretRdpmc(pVM, pVCpu, pRegFrame);
}


static int emInterpretMonitor(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    NOREF(pDis); NOREF(pvFault); NOREF(pcbSize);
    return EMInterpretMonitor(pVM, pVCpu, pRegFrame);
}


static VBOXSTRICTRC emInterpretMWait(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    NOREF(pDis); NOREF(pvFault); NOREF(pcbSize);
    return EMInterpretMWait(pVM, pVCpu, pRegFrame);
}


/**
 * RDMSR Emulation.
 */
static int emInterpretRdmsr(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    /* Note: The Intel manual claims there's a REX version of RDMSR that's slightly
             different, so we play safe by completely disassembling the instruction. */
    Assert(!(pDis->fPrefix & DISPREFIX_REX));
    NOREF(pDis); NOREF(pvFault); NOREF(pcbSize);
    return EMInterpretRdmsr(pVM, pVCpu, pRegFrame);
}


/**
 * WRMSR Emulation.
 */
static int emInterpretWrmsr(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize)
{
    NOREF(pDis); NOREF(pvFault); NOREF(pcbSize);
    return EMInterpretWrmsr(pVM, pVCpu, pRegFrame);
}


/**
 * Internal worker.
 * @copydoc emInterpretInstructionCPUOuter
 * @param   pVM     The cross context VM structure.
 */
DECLINLINE(VBOXSTRICTRC) emInterpretInstructionCPU(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame,
                                                   RTGCPTR pvFault, EMCODETYPE enmCodeType, uint32_t *pcbSize)
{
    Assert(pRegFrame == CPUMGetGuestCtxCore(pVCpu));
    Assert(enmCodeType == EMCODETYPE_SUPERVISOR || enmCodeType == EMCODETYPE_ALL);
    Assert(pcbSize);
    *pcbSize = 0;

    if (enmCodeType == EMCODETYPE_SUPERVISOR)
    {
        /*
         * Only supervisor guest code!!
         * And no complicated prefixes.
         */
        /* Get the current privilege level. */
        uint32_t cpl = CPUMGetGuestCPL(pVCpu);
#ifdef VBOX_WITH_RAW_RING1
        if (   !EMIsRawRing1Enabled(pVM)
            || cpl > 1
            || pRegFrame->eflags.Bits.u2IOPL > cpl
           )
#endif
        {
            if (    cpl != 0
                &&  pDis->pCurInstr->uOpcode != OP_RDTSC)    /* rdtsc requires emulation in ring 3 as well */
            {
                Log(("WARNING: refusing instruction emulation for user-mode code!!\n"));
                STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,FailedUserMode));
                return VERR_EM_INTERPRETER;
            }
        }
    }
    else
        Log2(("emInterpretInstructionCPU allowed to interpret user-level code!!\n"));

#ifdef IN_RC
    if (    (pDis->fPrefix & (DISPREFIX_REPNE | DISPREFIX_REP))
        ||  (   (pDis->fPrefix & DISPREFIX_LOCK)
             && pDis->pCurInstr->uOpcode != OP_CMPXCHG
             && pDis->pCurInstr->uOpcode != OP_CMPXCHG8B
             && pDis->pCurInstr->uOpcode != OP_XADD
             && pDis->pCurInstr->uOpcode != OP_OR
             && pDis->pCurInstr->uOpcode != OP_AND
             && pDis->pCurInstr->uOpcode != OP_XOR
             && pDis->pCurInstr->uOpcode != OP_BTR
            )
       )
#else
    if (    (pDis->fPrefix & DISPREFIX_REPNE)
        ||  (   (pDis->fPrefix & DISPREFIX_REP)
             && pDis->pCurInstr->uOpcode != OP_STOSWD
            )
        ||  (   (pDis->fPrefix & DISPREFIX_LOCK)
             && pDis->pCurInstr->uOpcode != OP_OR
             && pDis->pCurInstr->uOpcode != OP_AND
             && pDis->pCurInstr->uOpcode != OP_XOR
             && pDis->pCurInstr->uOpcode != OP_BTR
             && pDis->pCurInstr->uOpcode != OP_CMPXCHG
             && pDis->pCurInstr->uOpcode != OP_CMPXCHG8B
            )
       )
#endif
    {
        //Log(("EMInterpretInstruction: wrong prefix!!\n"));
        STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,FailedPrefix));
        Log4(("EM: Refuse %u on REP/REPNE/LOCK prefix grounds\n", pDis->pCurInstr->uOpcode));
        return VERR_EM_INTERPRETER;
    }

#if HC_ARCH_BITS == 32
    /*
     * Unable to emulate most >4 bytes accesses in 32 bits mode.
     * Whitelisted instructions are safe.
     */
    if (    pDis->Param1.cb > 4
        &&  CPUMIsGuestIn64BitCode(pVCpu))
    {
        uint32_t uOpCode = pDis->pCurInstr->uOpcode;
        if (    uOpCode != OP_STOSWD
            &&  uOpCode != OP_MOV
            &&  uOpCode != OP_CMPXCHG8B
            &&  uOpCode != OP_XCHG
            &&  uOpCode != OP_BTS
            &&  uOpCode != OP_BTR
            &&  uOpCode != OP_BTC
            )
        {
# ifdef VBOX_WITH_STATISTICS
            switch (pDis->pCurInstr->uOpcode)
            {
#  define INTERPRET_FAILED_CASE(opcode, Instr) \
                case opcode: STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,Failed##Instr)); break;
                INTERPRET_FAILED_CASE(OP_XCHG,Xchg);
                INTERPRET_FAILED_CASE(OP_DEC,Dec);
                INTERPRET_FAILED_CASE(OP_INC,Inc);
                INTERPRET_FAILED_CASE(OP_POP,Pop);
                INTERPRET_FAILED_CASE(OP_OR, Or);
                INTERPRET_FAILED_CASE(OP_XOR,Xor);
                INTERPRET_FAILED_CASE(OP_AND,And);
                INTERPRET_FAILED_CASE(OP_MOV,Mov);
                INTERPRET_FAILED_CASE(OP_STOSWD,StosWD);
                INTERPRET_FAILED_CASE(OP_INVLPG,InvlPg);
                INTERPRET_FAILED_CASE(OP_CPUID,CpuId);
                INTERPRET_FAILED_CASE(OP_MOV_CR,MovCRx);
                INTERPRET_FAILED_CASE(OP_MOV_DR,MovDRx);
                INTERPRET_FAILED_CASE(OP_LLDT,LLdt);
                INTERPRET_FAILED_CASE(OP_LIDT,LIdt);
                INTERPRET_FAILED_CASE(OP_LGDT,LGdt);
                INTERPRET_FAILED_CASE(OP_LMSW,Lmsw);
                INTERPRET_FAILED_CASE(OP_CLTS,Clts);
                INTERPRET_FAILED_CASE(OP_MONITOR,Monitor);
                INTERPRET_FAILED_CASE(OP_MWAIT,MWait);
                INTERPRET_FAILED_CASE(OP_RDMSR,Rdmsr);
                INTERPRET_FAILED_CASE(OP_WRMSR,Wrmsr);
                INTERPRET_FAILED_CASE(OP_ADD,Add);
                INTERPRET_FAILED_CASE(OP_SUB,Sub);
                INTERPRET_FAILED_CASE(OP_ADC,Adc);
                INTERPRET_FAILED_CASE(OP_BTR,Btr);
                INTERPRET_FAILED_CASE(OP_BTS,Bts);
                INTERPRET_FAILED_CASE(OP_BTC,Btc);
                INTERPRET_FAILED_CASE(OP_RDTSC,Rdtsc);
                INTERPRET_FAILED_CASE(OP_CMPXCHG, CmpXchg);
                INTERPRET_FAILED_CASE(OP_STI, Sti);
                INTERPRET_FAILED_CASE(OP_XADD,XAdd);
                INTERPRET_FAILED_CASE(OP_CMPXCHG8B,CmpXchg8b);
                INTERPRET_FAILED_CASE(OP_HLT, Hlt);
                INTERPRET_FAILED_CASE(OP_IRET,Iret);
                INTERPRET_FAILED_CASE(OP_WBINVD,WbInvd);
                INTERPRET_FAILED_CASE(OP_MOVNTPS,MovNTPS);
#  undef INTERPRET_FAILED_CASE
                default:
                    STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,FailedMisc));
                    break;
            }
# endif /* VBOX_WITH_STATISTICS */
            Log4(("EM: Refuse %u on grounds of accessing %u bytes\n", pDis->pCurInstr->uOpcode, pDis->Param1.cb));
            return VERR_EM_INTERPRETER;
        }
    }
#endif

    VBOXSTRICTRC rc;
#if (defined(VBOX_STRICT) || defined(LOG_ENABLED))
    LogFlow(("emInterpretInstructionCPU %s\n", emGetMnemonic(pDis)));
#endif
    switch (pDis->pCurInstr->uOpcode)
    {
        /*
         * Macros for generating the right case statements.
         */
# ifndef VBOX_COMPARE_IEM_AND_EM
#  define INTERPRET_CASE_EX_LOCK_PARAM3(opcode, Instr, InstrFn, pfnEmulate, pfnEmulateLock) \
        case opcode:\
            if (pDis->fPrefix & DISPREFIX_LOCK) \
                rc = emInterpretLock##InstrFn(pVM, pVCpu, pDis, pRegFrame, pvFault, pcbSize, pfnEmulateLock); \
            else \
                rc = emInterpret##InstrFn(pVM, pVCpu, pDis, pRegFrame, pvFault, pcbSize, pfnEmulate); \
            if (RT_SUCCESS(rc)) \
                STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,Instr)); \
            else \
                STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,Failed##Instr)); \
            return rc
# else  /* VBOX_COMPARE_IEM_AND_EM */
#  define INTERPRET_CASE_EX_LOCK_PARAM3(opcode, Instr, InstrFn, pfnEmulate, pfnEmulateLock) \
        case opcode:\
            rc = emInterpret##InstrFn(pVM, pVCpu, pDis, pRegFrame, pvFault, pcbSize, pfnEmulate); \
            if (RT_SUCCESS(rc)) \
                STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,Instr)); \
            else \
                STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,Failed##Instr)); \
            return rc
# endif /* VBOX_COMPARE_IEM_AND_EM */

#define INTERPRET_CASE_EX_PARAM3(opcode, Instr, InstrFn, pfnEmulate) \
        case opcode:\
            rc = emInterpret##InstrFn(pVM, pVCpu, pDis, pRegFrame, pvFault, pcbSize, pfnEmulate); \
            if (RT_SUCCESS(rc)) \
                STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,Instr)); \
            else \
                STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,Failed##Instr)); \
            return rc

#define INTERPRET_CASE_EX_PARAM2(opcode, Instr, InstrFn, pfnEmulate) \
            INTERPRET_CASE_EX_PARAM3(opcode, Instr, InstrFn, pfnEmulate)
#define INTERPRET_CASE_EX_LOCK_PARAM2(opcode, Instr, InstrFn, pfnEmulate, pfnEmulateLock) \
            INTERPRET_CASE_EX_LOCK_PARAM3(opcode, Instr, InstrFn, pfnEmulate, pfnEmulateLock)

#define INTERPRET_CASE(opcode, Instr) \
        case opcode:\
            rc = emInterpret##Instr(pVM, pVCpu, pDis, pRegFrame, pvFault, pcbSize); \
            if (RT_SUCCESS(rc)) \
                STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,Instr)); \
            else \
                STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,Failed##Instr)); \
            return rc

#define INTERPRET_CASE_EX_DUAL_PARAM2(opcode, Instr, InstrFn) \
        case opcode:\
            rc = emInterpret##InstrFn(pVM, pVCpu, pDis, pRegFrame, pvFault, pcbSize); \
            if (RT_SUCCESS(rc)) \
                STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,Instr)); \
            else \
                STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,Failed##Instr)); \
            return rc

#define INTERPRET_STAT_CASE(opcode, Instr) \
        case opcode: STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,Failed##Instr)); return VERR_EM_INTERPRETER;

        /*
         * The actual case statements.
         */
        INTERPRET_CASE(OP_XCHG,Xchg);
        INTERPRET_CASE_EX_PARAM2(OP_DEC,Dec, IncDec, EMEmulateDec);
        INTERPRET_CASE_EX_PARAM2(OP_INC,Inc, IncDec, EMEmulateInc);
        INTERPRET_CASE(OP_POP,Pop);
        INTERPRET_CASE_EX_LOCK_PARAM3(OP_OR, Or,  OrXorAnd, EMEmulateOr, EMEmulateLockOr);
        INTERPRET_CASE_EX_LOCK_PARAM3(OP_XOR,Xor, OrXorAnd, EMEmulateXor, EMEmulateLockXor);
        INTERPRET_CASE_EX_LOCK_PARAM3(OP_AND,And, OrXorAnd, EMEmulateAnd, EMEmulateLockAnd);
        INTERPRET_CASE(OP_MOV,Mov);
#ifndef IN_RC
        INTERPRET_CASE(OP_STOSWD,StosWD);
#endif
        INTERPRET_CASE(OP_INVLPG,InvlPg);
        INTERPRET_CASE(OP_CPUID,CpuId);
        INTERPRET_CASE(OP_MOV_CR,MovCRx);
        INTERPRET_CASE(OP_MOV_DR,MovDRx);
#ifdef IN_RING0
        INTERPRET_CASE_EX_DUAL_PARAM2(OP_LIDT, LIdt, LIGdt);
        INTERPRET_CASE_EX_DUAL_PARAM2(OP_LGDT, LGdt, LIGdt);
#endif
        INTERPRET_CASE(OP_LLDT,LLdt);
        INTERPRET_CASE(OP_LMSW,Lmsw);
#ifdef EM_EMULATE_SMSW
        INTERPRET_CASE(OP_SMSW,Smsw);
#endif
        INTERPRET_CASE(OP_CLTS,Clts);
        INTERPRET_CASE(OP_MONITOR, Monitor);
        INTERPRET_CASE(OP_MWAIT, MWait);
        INTERPRET_CASE(OP_RDMSR, Rdmsr);
        INTERPRET_CASE(OP_WRMSR, Wrmsr);
        INTERPRET_CASE_EX_PARAM3(OP_ADD,Add, AddSub, EMEmulateAdd);
        INTERPRET_CASE_EX_PARAM3(OP_SUB,Sub, AddSub, EMEmulateSub);
        INTERPRET_CASE(OP_ADC,Adc);
        INTERPRET_CASE_EX_LOCK_PARAM2(OP_BTR,Btr, BitTest, EMEmulateBtr, EMEmulateLockBtr);
        INTERPRET_CASE_EX_PARAM2(OP_BTS,Bts, BitTest, EMEmulateBts);
        INTERPRET_CASE_EX_PARAM2(OP_BTC,Btc, BitTest, EMEmulateBtc);
        INTERPRET_CASE(OP_RDPMC,Rdpmc);
        INTERPRET_CASE(OP_RDTSC,Rdtsc);
        INTERPRET_CASE(OP_CMPXCHG, CmpXchg);
#ifdef IN_RC
        INTERPRET_CASE(OP_STI,Sti);
        INTERPRET_CASE(OP_XADD, XAdd);
        INTERPRET_CASE(OP_IRET,Iret);
#endif
        INTERPRET_CASE(OP_CMPXCHG8B, CmpXchg8b);
        INTERPRET_CASE(OP_HLT,Hlt);
        INTERPRET_CASE(OP_WBINVD,WbInvd);
#ifdef VBOX_WITH_STATISTICS
# ifndef IN_RC
        INTERPRET_STAT_CASE(OP_XADD, XAdd);
# endif
        INTERPRET_STAT_CASE(OP_MOVNTPS,MovNTPS);
#endif

        default:
            Log3(("emInterpretInstructionCPU: opcode=%d\n", pDis->pCurInstr->uOpcode));
            STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,FailedMisc));
            return VERR_EM_INTERPRETER;

#undef INTERPRET_CASE_EX_PARAM2
#undef INTERPRET_STAT_CASE
#undef INTERPRET_CASE_EX
#undef INTERPRET_CASE
    } /* switch (opcode) */
    /* not reached */
}

/**
 * Interprets the current instruction using the supplied DISCPUSTATE structure.
 *
 * EIP is *NOT* updated!
 *
 * @returns VBox strict status code.
 * @retval  VINF_*                  Scheduling instructions. When these are returned, it
 *                                  starts to get a bit tricky to know whether code was
 *                                  executed or not... We'll address this when it becomes a problem.
 * @retval  VERR_EM_INTERPRETER     Something we can't cope with.
 * @retval  VERR_*                  Fatal errors.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pDis        The disassembler cpu state for the instruction to be
 *                      interpreted.
 * @param   pRegFrame   The register frame. EIP is *NOT* changed!
 * @param   pvFault     The fault address (CR2).
 * @param   pcbSize     Size of the write (if applicable).
 * @param   enmCodeType Code type (user/supervisor)
 *
 * @remark  Invalid opcode exceptions have a higher priority than GP (see Intel
 *          Architecture System Developers Manual, Vol 3, 5.5) so we don't need
 *          to worry about e.g. invalid modrm combinations (!)
 *
 * @todo    At this time we do NOT check if the instruction overwrites vital information.
 *          Make sure this can't happen!! (will add some assertions/checks later)
 */
DECLINLINE(VBOXSTRICTRC) emInterpretInstructionCPUOuter(PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pRegFrame,
                                                        RTGCPTR pvFault, EMCODETYPE enmCodeType, uint32_t *pcbSize)
{
    STAM_PROFILE_START(&pVCpu->em.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,Emulate), a);
    VBOXSTRICTRC rc = emInterpretInstructionCPU(pVCpu->CTX_SUFF(pVM), pVCpu, pDis, pRegFrame, pvFault, enmCodeType, pcbSize);
    STAM_PROFILE_STOP(&pVCpu->em.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,Emulate), a);
    if (RT_SUCCESS(rc))
        STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,InterpretSucceeded));
    else
        STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,InterpretFailed));
    return rc;
}


#endif /* !VBOX_WITH_IEM */
