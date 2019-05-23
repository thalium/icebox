/* $Id: TRPMAll.cpp $ */
/** @file
 * TRPM - Trap Monitor - Any Context.
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
#define LOG_GROUP LOG_GROUP_TRPM
#include <VBox/vmm/trpm.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/patm.h>
#include <VBox/vmm/selm.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/dbgf.h>
#include "TRPMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/err.h>
#include <VBox/vmm/em.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/asm-amd64-x86.h>
#include <iprt/param.h>
#include <iprt/x86.h>



#if defined(TRPM_TRACK_GUEST_IDT_CHANGES) && !defined(IN_RING0)
/**
 * @callback_method_impl{FNPGMVIRTPFHANDLER,
 * \#PF Handler callback for virtual access handler ranges.}
 *
 * Important to realize that a physical page in a range can have aliases, and
 * for ALL and WRITE handlers these will also trigger.
 */
PGM_ALL_CB2_DECL(VBOXSTRICTRC)
trpmGuestIDTWriteHandler(PVM pVM, PVMCPU pVCpu, RTGCPTR GCPtr, void *pvPtr, void *pvBuf, size_t cbBuf,
                         PGMACCESSTYPE enmAccessType, PGMACCESSORIGIN enmOrigin, void *pvUser)
{
    Assert(enmAccessType == PGMACCESSTYPE_WRITE); NOREF(enmAccessType);
    Log(("trpmGuestIDTWriteHandler: write to %RGv size %d\n", GCPtr, cbBuf)); NOREF(GCPtr); NOREF(cbBuf);
    NOREF(pvPtr); NOREF(pvUser); NOREF(pvBuf); NOREF(enmOrigin); NOREF(pvUser); RT_NOREF_PV(pVM);
    Assert(!HMIsEnabled(pVM));

    /** @todo Check which IDT entry and keep the update cost low in TRPMR3SyncIDT() and CSAMCheckGates(). */
    VMCPU_FF_SET(pVCpu, VMCPU_FF_TRPM_SYNC_IDT);
# ifdef IN_RC
    STAM_COUNTER_INC(&pVM->trpm.s.StatRCWriteGuestIDTFault);
# endif
    return VINF_PGM_HANDLER_DO_DEFAULT;
}
#endif /* TRPM_TRACK_GUEST_IDT_CHANGES && !IN_RING0 */


/**
 * Query info about the current active trap/interrupt.
 * If no trap is active active an error code is returned.
 *
 * @returns VBox status code.
 * @param   pVCpu                   The cross context virtual CPU structure.
 * @param   pu8TrapNo               Where to store the trap number.
 * @param   penmType                Where to store the trap type
 */
VMMDECL(int) TRPMQueryTrap(PVMCPU pVCpu, uint8_t *pu8TrapNo, TRPMEVENT *penmType)
{
    /*
     * Check if we have a trap at present.
     */
    if (pVCpu->trpm.s.uActiveVector != ~0U)
    {
        if (pu8TrapNo)
            *pu8TrapNo = (uint8_t)pVCpu->trpm.s.uActiveVector;
        if (penmType)
            *penmType = pVCpu->trpm.s.enmActiveType;
        return VINF_SUCCESS;
    }

    return VERR_TRPM_NO_ACTIVE_TRAP;
}


/**
 * Gets the trap number for the current trap.
 *
 * The caller is responsible for making sure there is an active trap which
 * takes an error code when making this request.
 *
 * @returns The current trap number.
 * @param   pVCpu                   The cross context virtual CPU structure.
 */
VMMDECL(uint8_t) TRPMGetTrapNo(PVMCPU pVCpu)
{
    AssertMsg(pVCpu->trpm.s.uActiveVector != ~0U, ("No active trap!\n"));
    return (uint8_t)pVCpu->trpm.s.uActiveVector;
}


/**
 * Gets the error code for the current trap.
 *
 * The caller is responsible for making sure there is an active trap which
 * takes an error code when making this request.
 *
 * @returns Error code.
 * @param   pVCpu                   The cross context virtual CPU structure.
 */
VMMDECL(RTGCUINT) TRPMGetErrorCode(PVMCPU pVCpu)
{
    AssertMsg(pVCpu->trpm.s.uActiveVector != ~0U, ("No active trap!\n"));
#ifdef VBOX_STRICT
    switch (pVCpu->trpm.s.uActiveVector)
    {
        case X86_XCPT_TS:
        case X86_XCPT_NP:
        case X86_XCPT_SS:
        case X86_XCPT_GP:
        case X86_XCPT_PF:
        case X86_XCPT_AC:
        case X86_XCPT_DF:
            break;
        default:
            AssertMsgFailed(("This trap (%#x) doesn't have any error code\n", pVCpu->trpm.s.uActiveVector));
            break;
    }
#endif
    return pVCpu->trpm.s.uActiveErrorCode;
}


/**
 * Gets the fault address for the current trap.
 *
 * The caller is responsible for making sure there is an active trap 0x0e when
 * making this request.
 *
 * @returns Fault address associated with the trap.
 * @param   pVCpu                   The cross context virtual CPU structure.
 */
VMMDECL(RTGCUINTPTR) TRPMGetFaultAddress(PVMCPU pVCpu)
{
    AssertMsg(pVCpu->trpm.s.uActiveVector != ~0U, ("No active trap!\n"));
    AssertMsg(pVCpu->trpm.s.uActiveVector == X86_XCPT_PF, ("Not page-fault trap!\n"));
    return pVCpu->trpm.s.uActiveCR2;
}


/**
 * Gets the instruction-length for the current trap (only relevant for software
 * interrupts and software exceptions \#BP and \#OF).
 *
 * The caller is responsible for making sure there is an active trap 0x0e when
 * making this request.
 *
 * @returns Fault address associated with the trap.
 * @param   pVCpu                   The cross context virtual CPU structure.
 */
VMMDECL(uint8_t) TRPMGetInstrLength(PVMCPU pVCpu)
{
    AssertMsg(pVCpu->trpm.s.uActiveVector != ~0U, ("No active trap!\n"));
    return pVCpu->trpm.s.cbInstr;
}


/**
 * Clears the current active trap/exception/interrupt.
 *
 * The caller is responsible for making sure there is an active trap
 * when making this request.
 *
 * @returns VBox status code.
 * @param   pVCpu                   The cross context virtual CPU structure.
 */
VMMDECL(int) TRPMResetTrap(PVMCPU pVCpu)
{
    /*
     * Cannot reset non-existing trap!
     */
    if (pVCpu->trpm.s.uActiveVector == ~0U)
    {
        AssertMsgFailed(("No active trap!\n"));
        return VERR_TRPM_NO_ACTIVE_TRAP;
    }

    /*
     * Reset it.
     */
    pVCpu->trpm.s.uActiveVector = ~0U;
    return VINF_SUCCESS;
}


/**
 * Assert trap/exception/interrupt.
 *
 * The caller is responsible for making sure there is no active trap
 * when making this request.
 *
 * @returns VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   u8TrapNo            The trap vector to assert.
 * @param   enmType             Trap type.
 */
VMMDECL(int) TRPMAssertTrap(PVMCPU pVCpu, uint8_t u8TrapNo, TRPMEVENT enmType)
{
    Log2(("TRPMAssertTrap: u8TrapNo=%02x type=%d\n", u8TrapNo, enmType));

    /*
     * Cannot assert a trap when one is already active.
     */
    if (pVCpu->trpm.s.uActiveVector != ~0U)
    {
        AssertMsgFailed(("CPU%d: Active trap %#x\n", pVCpu->idCpu, pVCpu->trpm.s.uActiveVector));
        return VERR_TRPM_ACTIVE_TRAP;
    }

    pVCpu->trpm.s.uActiveVector               = u8TrapNo;
    pVCpu->trpm.s.enmActiveType               = enmType;
    pVCpu->trpm.s.uActiveErrorCode            = ~(RTGCUINT)0;
    pVCpu->trpm.s.uActiveCR2                  = 0xdeadface;
    pVCpu->trpm.s.cbInstr                     = UINT8_MAX;
    return VINF_SUCCESS;
}


/**
 * Assert a page-fault exception.
 *
 * The caller is responsible for making sure there is no active trap
 * when making this request.
 *
 * @returns VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   uCR2                The new fault address.
 * @param   uErrorCode          The error code for the page-fault.
 */
VMMDECL(int) TRPMAssertXcptPF(PVMCPU pVCpu, RTGCUINTPTR uCR2, RTGCUINT uErrorCode)
{
    Log2(("TRPMAssertXcptPF: uCR2=%RGv uErrorCode=%RGv\n", uCR2, uErrorCode)); /** @todo RTGCUINT to be fixed. */

    /*
     * Cannot assert a trap when one is already active.
     */
    if (pVCpu->trpm.s.uActiveVector != ~0U)
    {
        AssertMsgFailed(("CPU%d: Active trap %#x\n", pVCpu->idCpu, pVCpu->trpm.s.uActiveVector));
        return VERR_TRPM_ACTIVE_TRAP;
    }

    pVCpu->trpm.s.uActiveVector               = X86_XCPT_PF;
    pVCpu->trpm.s.enmActiveType               = TRPM_TRAP;
    pVCpu->trpm.s.uActiveErrorCode            = uErrorCode;
    pVCpu->trpm.s.uActiveCR2                  = uCR2;
    pVCpu->trpm.s.cbInstr                     = UINT8_MAX;
    return VINF_SUCCESS;
}


/**
 * Sets the error code of the current trap.
 * (This function is for use in trap handlers and such.)
 *
 * The caller is responsible for making sure there is an active trap
 * which takes an errorcode when making this request.
 *
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   uErrorCode          The new error code.
 */
VMMDECL(void) TRPMSetErrorCode(PVMCPU pVCpu, RTGCUINT uErrorCode)
{
    Log2(("TRPMSetErrorCode: uErrorCode=%RGv\n", uErrorCode)); /** @todo RTGCUINT mess! */
    AssertMsg(pVCpu->trpm.s.uActiveVector != ~0U, ("No active trap!\n"));
    pVCpu->trpm.s.uActiveErrorCode = uErrorCode;
#ifdef VBOX_STRICT
    switch (pVCpu->trpm.s.uActiveVector)
    {
        case X86_XCPT_TS: case X86_XCPT_NP: case X86_XCPT_SS: case X86_XCPT_GP: case X86_XCPT_PF:
            AssertMsg(uErrorCode != ~(RTGCUINT)0, ("Invalid uErrorCode=%#x u8TrapNo=%d\n", uErrorCode, pVCpu->trpm.s.uActiveVector));
            break;
        case X86_XCPT_AC: case X86_XCPT_DF:
            AssertMsg(uErrorCode == 0,            ("Invalid uErrorCode=%#x u8TrapNo=%d\n", uErrorCode, pVCpu->trpm.s.uActiveVector));
            break;
        default:
            AssertMsg(uErrorCode == ~(RTGCUINT)0, ("Invalid uErrorCode=%#x u8TrapNo=%d\n", uErrorCode, pVCpu->trpm.s.uActiveVector));
            break;
    }
#endif
}


/**
 * Sets the fault address of the current \#PF trap. (This function is for use in
 * trap handlers and such.)
 *
 * The caller is responsible for making sure there is an active trap 0e
 * when making this request.
 *
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   uCR2                The new fault address (cr2 register).
 */
VMMDECL(void) TRPMSetFaultAddress(PVMCPU pVCpu, RTGCUINTPTR uCR2)
{
    Log2(("TRPMSetFaultAddress: uCR2=%RGv\n", uCR2));
    AssertMsg(pVCpu->trpm.s.uActiveVector != ~0U, ("No active trap!\n"));
    AssertMsg(pVCpu->trpm.s.uActiveVector == X86_XCPT_PF, ("Not trap 0e!\n"));
    pVCpu->trpm.s.uActiveCR2 = uCR2;
}


/**
 * Sets the instruction-length of the current trap (relevant for software
 * interrupts and software exceptions like \#BP, \#OF).
 *
 * The caller is responsible for making sure there is an active trap 0e
 * when making this request.
 *
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   cbInstr             The instruction length.
 */
VMMDECL(void) TRPMSetInstrLength(PVMCPU pVCpu, uint8_t cbInstr)
{
    Log2(("TRPMSetInstrLength: cbInstr=%u\n", cbInstr));
    AssertMsg(pVCpu->trpm.s.uActiveVector != ~0U, ("No active trap!\n"));
    AssertMsg(   pVCpu->trpm.s.enmActiveType == TRPM_SOFTWARE_INT
              || (   pVCpu->trpm.s.enmActiveType == TRPM_TRAP
                  && (   pVCpu->trpm.s.uActiveVector == X86_XCPT_BP
                      || pVCpu->trpm.s.uActiveVector == X86_XCPT_OF)),
              ("Invalid trap type %#x\n", pVCpu->trpm.s.enmActiveType));
    pVCpu->trpm.s.cbInstr = cbInstr;
}


/**
 * Checks if the current active trap/interrupt/exception/fault/whatever is a software
 * interrupt or not.
 *
 * The caller is responsible for making sure there is an active trap
 * when making this request.
 *
 * @returns true if software interrupt, false if not.
 *
 * @param   pVCpu               The cross context virtual CPU structure.
 */
VMMDECL(bool) TRPMIsSoftwareInterrupt(PVMCPU pVCpu)
{
    AssertMsg(pVCpu->trpm.s.uActiveVector != ~0U, ("No active trap!\n"));
    return (pVCpu->trpm.s.enmActiveType == TRPM_SOFTWARE_INT);
}


/**
 * Check if there is an active trap.
 *
 * @returns true if trap active, false if not.
 * @param   pVCpu               The cross context virtual CPU structure.
 */
VMMDECL(bool) TRPMHasTrap(PVMCPU pVCpu)
{
    return pVCpu->trpm.s.uActiveVector != ~0U;
}


/**
 * Query all info about the current active trap/interrupt.
 * If no trap is active active an error code is returned.
 *
 * @returns VBox status code.
 * @param   pVCpu                   The cross context virtual CPU structure.
 * @param   pu8TrapNo               Where to store the trap number.
 * @param   pEnmType                Where to store the trap type
 * @param   puErrorCode             Where to store the error code associated with some traps.
 *                                  ~0U is stored if the trap has no error code.
 * @param   puCR2                   Where to store the CR2 associated with a trap 0E.
 * @param   pcbInstr                Where to store the instruction-length
 *                                  associated with some traps.
 */
VMMDECL(int) TRPMQueryTrapAll(PVMCPU pVCpu, uint8_t *pu8TrapNo, TRPMEVENT *pEnmType, PRTGCUINT puErrorCode, PRTGCUINTPTR puCR2,
                               uint8_t *pcbInstr)
{
    /*
     * Check if we have a trap at present.
     */
    if (pVCpu->trpm.s.uActiveVector == ~0U)
        return VERR_TRPM_NO_ACTIVE_TRAP;

    if (pu8TrapNo)
        *pu8TrapNo      = (uint8_t)pVCpu->trpm.s.uActiveVector;
    if (pEnmType)
        *pEnmType       = pVCpu->trpm.s.enmActiveType;
    if (puErrorCode)
        *puErrorCode    = pVCpu->trpm.s.uActiveErrorCode;
    if (puCR2)
        *puCR2          = pVCpu->trpm.s.uActiveCR2;
    if (pcbInstr)
        *pcbInstr       = pVCpu->trpm.s.cbInstr;
    return VINF_SUCCESS;
}


/**
 * Save the active trap.
 *
 * This routine useful when doing try/catch in the hypervisor.
 * Any function which uses temporary trap handlers should
 * probably also use this facility to save the original trap.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(void) TRPMSaveTrap(PVMCPU pVCpu)
{
    pVCpu->trpm.s.uSavedVector        = pVCpu->trpm.s.uActiveVector;
    pVCpu->trpm.s.enmSavedType        = pVCpu->trpm.s.enmActiveType;
    pVCpu->trpm.s.uSavedErrorCode     = pVCpu->trpm.s.uActiveErrorCode;
    pVCpu->trpm.s.uSavedCR2           = pVCpu->trpm.s.uActiveCR2;
    pVCpu->trpm.s.cbSavedInstr        = pVCpu->trpm.s.cbInstr;
}


/**
 * Restore a saved trap.
 *
 * Multiple restores of a saved trap is possible.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(void) TRPMRestoreTrap(PVMCPU pVCpu)
{
    pVCpu->trpm.s.uActiveVector       = pVCpu->trpm.s.uSavedVector;
    pVCpu->trpm.s.enmActiveType       = pVCpu->trpm.s.enmSavedType;
    pVCpu->trpm.s.uActiveErrorCode    = pVCpu->trpm.s.uSavedErrorCode;
    pVCpu->trpm.s.uActiveCR2          = pVCpu->trpm.s.uSavedCR2;
    pVCpu->trpm.s.cbInstr             = pVCpu->trpm.s.cbSavedInstr;
}


#ifdef VBOX_WITH_RAW_MODE_NOT_R0
/**
 * Forward trap or interrupt to the guest's handler
 *
 *
 * @returns VBox status code.
 *  or does not return at all (when the trap is actually forwarded)
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pRegFrame   Pointer to the register frame for the trap.
 * @param   iGate       Trap or interrupt gate number
 * @param   cbInstr     Instruction size (only relevant for software interrupts)
 * @param   enmError    TRPM_TRAP_HAS_ERRORCODE or TRPM_TRAP_NO_ERRORCODE.
 * @param   enmType     TRPM event type
 * @param   iOrgTrap    The original trap.
 * @internal
 */
VMMDECL(int) TRPMForwardTrap(PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, uint32_t iGate, uint32_t cbInstr,
                             TRPMERRORCODE enmError, TRPMEVENT enmType, int32_t iOrgTrap)
{
    AssertReturn(!HMIsEnabled(pVCpu->CTX_SUFF(pVM)), VERR_TRPM_HM_IPE);
#ifdef TRPM_FORWARD_TRAPS_IN_GC
    PVM pVM = pVCpu->CTX_SUFF(pVM);
    X86EFLAGS eflags;
    Assert(pVM->cCpus == 1);

    STAM_PROFILE_ADV_START(&pVM->trpm.s.CTX_SUFF_Z(StatForwardProf), a);

# if defined(VBOX_STRICT) || defined(LOG_ENABLED)
    if (pRegFrame->eflags.Bits.u1VM)
        Log(("TRPMForwardTrap-VM: eip=%04X:%04X iGate=%d\n", pRegFrame->cs.Sel, pRegFrame->eip, iGate));
    else
        Log(("TRPMForwardTrap: eip=%04X:%08X iGate=%d\n", pRegFrame->cs.Sel, pRegFrame->eip, iGate));

    switch (iGate) {
    case X86_XCPT_PF:
        if (pRegFrame->eip == pVCpu->trpm.s.uActiveCR2)
        {
            RTGCPTR pCallerGC;
#  ifdef IN_RC
            int rc = MMGCRamRead(pVM, &pCallerGC, (void *)pRegFrame->esp, sizeof(pCallerGC));
#  else
            int rc = PGMPhysSimpleReadGCPtr(pVCpu, &pCallerGC, (RTGCPTR)pRegFrame->esp, sizeof(pCallerGC));
#  endif
            if (RT_SUCCESS(rc))
                Log(("TRPMForwardTrap: caller=%RGv\n", pCallerGC));
        }
        RT_FALL_THRU();
    case X86_XCPT_DF:
    case X86_XCPT_TS:
    case X86_XCPT_NP:
    case X86_XCPT_SS:
    case X86_XCPT_GP:
    case X86_XCPT_AC:
        Assert(enmError == TRPM_TRAP_HAS_ERRORCODE || enmType == TRPM_SOFTWARE_INT);
        break;

    default:
        Assert(enmError == TRPM_TRAP_NO_ERRORCODE);
        break;
    }
# endif /* VBOX_STRICT || LOG_ENABLED */
#ifdef IN_RC
    AssertReturn(CPUMIsGuestInRawMode(pVCpu), VINF_EM_RESCHEDULE);
#endif

    /* Retrieve the eflags including the virtualized bits. */
    /* Note: hackish as the cpumctxcore structure doesn't contain the right value */
    eflags.u32 = CPUMRawGetEFlags(pVCpu);

    /* VMCPU_FF_INHIBIT_INTERRUPTS should be cleared upfront or don't call this function at all for dispatching hardware interrupts. */
    Assert(enmType != TRPM_HARDWARE_INT || !VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS));

    /*
     * If it's a real guest trap and the guest's page fault handler is marked as safe for GC execution, then we call it directly.
     * Well, only if the IF flag is set.
     */
    /** @todo if the trap handler was modified and marked invalid, then we should *now* go back to the host context and install a new patch. */
    if (    pVM->trpm.s.aGuestTrapHandler[iGate]
        && (eflags.Bits.u1IF)
#ifndef VBOX_RAW_V86
        && !(eflags.Bits.u1VM) /** @todo implement when needed (illegal for same privilege level transfers). */
#endif
        && !PATMIsPatchGCAddr(pVM, pRegFrame->eip)
       )
    {
        uint16_t    cbIDT;
        RTGCPTR     GCPtrIDT = (RTGCPTR)CPUMGetGuestIDTR(pVCpu, &cbIDT);
        uint32_t    cpl;
        VBOXIDTE    GuestIdte;
        RTGCPTR     pIDTEntry;
        int         rc;

        Assert(PATMAreInterruptsEnabledByCtx(pVM, CPUMCTX_FROM_CORE(pRegFrame)));
        Assert(!VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_SELM_SYNC_GDT | VMCPU_FF_SELM_SYNC_LDT | VMCPU_FF_TRPM_SYNC_IDT | VMCPU_FF_SELM_SYNC_TSS));

        if (GCPtrIDT && iGate * sizeof(VBOXIDTE) >= cbIDT)
            goto failure;

        /* Get the current privilege level. */
        cpl = CPUMGetGuestCPL(pVCpu);

        /*
         * BIG TODO: The checks are not complete. see trap and interrupt dispatching section in Intel docs for details
         *           All very obscure, but still necessary.
         *           Currently only some CS & TSS selector checks are missing.
         *
         */
        pIDTEntry = (RTGCPTR)((RTGCUINTPTR)GCPtrIDT + sizeof(VBOXIDTE) * iGate);
#ifdef IN_RC
        rc = MMGCRamRead(pVM, &GuestIdte, (void *)(uintptr_t)pIDTEntry, sizeof(GuestIdte));
#else
        rc = PGMPhysSimpleReadGCPtr(pVCpu, &GuestIdte, pIDTEntry, sizeof(GuestIdte));
#endif
        if (RT_FAILURE(rc))
        {
            /* The page might be out of sync. */ /** @todo might cross a page boundary) */
            Log(("Page %RGv out of sync -> prefetch and try again\n", pIDTEntry));
            rc = PGMPrefetchPage(pVCpu, pIDTEntry); /** @todo r=bird: rainy day: this isn't entirely safe because of access bit virtualiziation and CSAM. */
            if (rc != VINF_SUCCESS)
            {
                Log(("TRPMForwardTrap: PGMPrefetchPage failed with rc=%Rrc\n", rc));
                goto failure;
            }
#ifdef IN_RC
            rc = MMGCRamRead(pVM, &GuestIdte, (void *)(uintptr_t)pIDTEntry, sizeof(GuestIdte));
#else
            rc = PGMPhysSimpleReadGCPtr(pVCpu, &GuestIdte, pIDTEntry, sizeof(GuestIdte));
#endif
        }
        if (    RT_SUCCESS(rc)
            &&  GuestIdte.Gen.u1Present
            &&  (GuestIdte.Gen.u5Type2 == VBOX_IDTE_TYPE2_TRAP_32 || GuestIdte.Gen.u5Type2 == VBOX_IDTE_TYPE2_INT_32)
            &&  (GuestIdte.Gen.u2DPL == 3 || GuestIdte.Gen.u2DPL == 0)
            &&  (GuestIdte.Gen.u16SegSel & 0xfffc)  /* must not be zero */
            &&  (enmType == TRPM_TRAP || enmType == TRPM_HARDWARE_INT || cpl <= GuestIdte.Gen.u2DPL)  /* CPL <= DPL if software int */
           )
        {
            RTGCPTR pHandler, dummy;
            RTGCPTR pTrapStackGC;

            pHandler = (RTGCPTR)VBOXIDTE_OFFSET(GuestIdte);

            /* Note: SELMValidateAndConvertCSAddr checks for code type, memory type, selector validity. */
            /** @todo dpl <= cpl else GPF */

            /* Note: don't use current eflags as we might be in V86 mode and the IDT always contains protected mode selectors */
            X86EFLAGS fakeflags;
            fakeflags.u32 = 0;

            rc = SELMValidateAndConvertCSAddr(pVCpu, fakeflags, 0, GuestIdte.Gen.u16SegSel, NULL, pHandler, &dummy);
            if (rc == VINF_SUCCESS)
            {
                VBOXGDTR gdtr = {0, 0};
                bool     fConforming = false;
                int      idx = 0;
                uint32_t dpl;
                uint32_t ss_r0;
                uint32_t esp_r0;
                X86DESC  Desc;
                RTGCPTR  pGdtEntry;

                CPUMGetGuestGDTR(pVCpu, &gdtr);
                Assert(gdtr.pGdt && gdtr.cbGdt > GuestIdte.Gen.u16SegSel);

                if (!gdtr.pGdt)
                    goto failure;

                pGdtEntry = gdtr.pGdt + (GuestIdte.Gen.u16SegSel >> X86_SEL_SHIFT) * sizeof(X86DESC);
#ifdef IN_RC
                rc = MMGCRamRead(pVM, &Desc, (void *)(uintptr_t)pGdtEntry, sizeof(Desc));
#else
                rc = PGMPhysSimpleReadGCPtr(pVCpu, &Desc, pGdtEntry, sizeof(Desc));
#endif
                if (RT_FAILURE(rc))
                {
                    /* The page might be out of sync. */ /** @todo might cross a page boundary) */
                    Log(("Page %RGv out of sync -> prefetch and try again\n", pGdtEntry));
                    rc = PGMPrefetchPage(pVCpu, pGdtEntry);  /** @todo r=bird: rainy day: this isn't entirely safe because of access bit virtualiziation and CSAM. */
                    if (rc != VINF_SUCCESS)
                    {
                        Log(("PGMPrefetchPage failed with rc=%Rrc\n", rc));
                        goto failure;
                    }
#ifdef IN_RC
                    rc = MMGCRamRead(pVM, &Desc, (void *)(uintptr_t)pGdtEntry, sizeof(Desc));
#else
                    rc = PGMPhysSimpleReadGCPtr(pVCpu, &Desc, pGdtEntry, sizeof(Desc));
#endif
                    if (RT_FAILURE(rc))
                    {
                        Log(("MMGCRamRead failed with %Rrc\n", rc));
                        goto failure;
                    }
                }

                if (Desc.Gen.u4Type & X86_SEL_TYPE_CONF)
                {
                    Log(("Conforming code selector\n"));
                    fConforming = true;
                }
                /** @todo check descriptor type!! */

                dpl = Desc.Gen.u2Dpl;

                if (!fConforming && dpl < cpl)  /* to inner privilege level */
                {
                    rc = SELMGetRing1Stack(pVM, &ss_r0, &esp_r0);
                    if (RT_FAILURE(rc))
                        goto failure;

                    Assert((ss_r0 & X86_SEL_RPL) == 1);

                    if (   !esp_r0
                        || !ss_r0
                        || (ss_r0 & X86_SEL_RPL) != ((dpl == 0) ? 1 : dpl)
                        || SELMToFlatBySelEx(pVCpu, fakeflags, ss_r0, (RTGCPTR)esp_r0, SELMTOFLAT_FLAGS_CPL1,
                                             (PRTGCPTR)&pTrapStackGC, NULL) != VINF_SUCCESS
                       )
                    {
                        Log(("Invalid ring 0 stack %04X:%08RX32\n", ss_r0, esp_r0));
                        goto failure;
                    }
                }
                else
                if (fConforming || dpl == cpl)  /* to the same privilege level */
                {
                    ss_r0  = pRegFrame->ss.Sel;
                    esp_r0 = pRegFrame->esp;

                    if (    eflags.Bits.u1VM    /* illegal */
                        ||  SELMToFlatBySelEx(pVCpu, fakeflags, ss_r0, (RTGCPTR)esp_r0, SELMTOFLAT_FLAGS_CPL1,
                                              (PRTGCPTR)&pTrapStackGC, NULL) != VINF_SUCCESS)
                    {
                        AssertMsgFailed(("Invalid stack %04X:%08RX32??? (VM=%d)\n", ss_r0, esp_r0, eflags.Bits.u1VM));
                        goto failure;
                    }
                }
                else
                {
                    Log(("Invalid cpl-dpl combo %d vs %d\n", cpl, dpl));
                    goto failure;
                }
                /*
                 * Build trap stack frame on guest handler's stack
                 */
                uint32_t *pTrapStack;
#ifdef IN_RC
                Assert(eflags.Bits.u1VM || (pRegFrame->ss.Sel & X86_SEL_RPL) != 0);
                /* Check maximum amount we need (10 when executing in V86 mode) */
                rc = PGMVerifyAccess(pVCpu, (RTGCUINTPTR)pTrapStackGC - 10*sizeof(uint32_t), 10 * sizeof(uint32_t), X86_PTE_RW);
                pTrapStack = (uint32_t *)(uintptr_t)pTrapStackGC;
#else
                Assert(eflags.Bits.u1VM || (pRegFrame->ss.Sel & X86_SEL_RPL) == 0 || (pRegFrame->ss.Sel & X86_SEL_RPL) == 3 || (EMIsRawRing1Enabled(pVM) && (pRegFrame->ss.Sel & X86_SEL_RPL) == 1));
                /* Check maximum amount we need (10 when executing in V86 mode) */
                if ((pTrapStackGC >> PAGE_SHIFT) != ((pTrapStackGC - 10*sizeof(uint32_t)) >> PAGE_SHIFT)) /* fail if we cross a page boundary */
                    goto failure;
                PGMPAGEMAPLOCK PageMappingLock;
                rc = PGMPhysGCPtr2CCPtr(pVCpu, pTrapStackGC, (void **)&pTrapStack, &PageMappingLock);
                if (RT_FAILURE(rc))
                {
                    AssertRC(rc);
                    goto failure;
                }
#endif
                if (rc == VINF_SUCCESS)
                {
                    /** if eflags.Bits.u1VM then push gs, fs, ds, es */
                    if (eflags.Bits.u1VM)
                    {
                        Log(("TRAP%02X: (VM) Handler %04X:%RGv Stack %04X:%08X RPL=%d CR2=%08X\n", iGate, GuestIdte.Gen.u16SegSel, pHandler, ss_r0, esp_r0, (pRegFrame->ss.Sel & X86_SEL_RPL), pVCpu->trpm.s.uActiveCR2));
                        pTrapStack[--idx] = pRegFrame->gs.Sel;
                        pTrapStack[--idx] = pRegFrame->fs.Sel;
                        pTrapStack[--idx] = pRegFrame->ds.Sel;
                        pTrapStack[--idx] = pRegFrame->es.Sel;

                        /* clear ds, es, fs & gs in current context */
                        pRegFrame->ds.Sel = pRegFrame->es.Sel = pRegFrame->fs.Sel = pRegFrame->gs.Sel = 0;
                    }
                    else
                        Log(("TRAP%02X: Handler %04X:%RGv Stack %04X:%08X RPL=%d CR2=%08X\n", iGate, GuestIdte.Gen.u16SegSel, pHandler, ss_r0, esp_r0, (pRegFrame->ss.Sel & X86_SEL_RPL), pVCpu->trpm.s.uActiveCR2));

                    if (!fConforming && dpl < cpl)
                    {
#ifdef IN_RC /* Only in RC we still see tracing of our ring modifications. */
                        if (    (pRegFrame->ss.Sel & X86_SEL_RPL) == 1
                            &&  !eflags.Bits.u1VM)
                            pTrapStack[--idx] = pRegFrame->ss.Sel & ~1;         /* Mask away traces of raw ring 0 execution (ring 1). */
                        else if (   EMIsRawRing1Enabled(pVM)
                                 && (pRegFrame->ss.Sel & X86_SEL_RPL) == 2)
                            pTrapStack[--idx] = (pRegFrame->ss.Sel & ~2) | 1;   /* Mask away traces of raw ring 1 execution (ring 2). */
                        else
#endif  /* IN_RC */
                            pTrapStack[--idx] = pRegFrame->ss.Sel;

                        pTrapStack[--idx] = pRegFrame->esp;
                    }

                    /* Note: We use the eflags copy, that includes the virtualized bits! */
                    /* Note: Not really necessary as we grab include those bits in the trap/irq handler trampoline */
                    pTrapStack[--idx] = eflags.u32;

#ifdef IN_RC /* Only in RC mode we still see tracing of our ring modifications */
                    if (    (pRegFrame->cs.Sel & X86_SEL_RPL) == 1
                        &&  !eflags.Bits.u1VM)
                        pTrapStack[--idx] = pRegFrame->cs.Sel & ~1;         /* Mask away traces of raw ring execution (ring 1). */
                    else if (   EMIsRawRing1Enabled(pVM)
                             && (pRegFrame->cs.Sel & X86_SEL_RPL) == 2)
                        pTrapStack[--idx] = (pRegFrame->cs.Sel & ~2) | 1;   /* Mask away traces of raw ring 1 execution (ring 2). */
                    else
#endif  /* IN_RC */
                        pTrapStack[--idx] = pRegFrame->cs.Sel;

                    if (enmType == TRPM_SOFTWARE_INT)
                    {
                        Assert(cbInstr);
                        pTrapStack[--idx] = pRegFrame->eip + cbInstr;    /* return address = next instruction */
                    }
                    else
                        pTrapStack[--idx] = pRegFrame->eip;

                    if (enmError == TRPM_TRAP_HAS_ERRORCODE)
                    {
                        pTrapStack[--idx] = pVCpu->trpm.s.uActiveErrorCode;
                    }

                    Assert(esp_r0 > -idx*sizeof(uint32_t));
                    /* Adjust ESP accordingly */
                    esp_r0 +=  idx*sizeof(uint32_t);

                    /* Mask away dangerous flags for the trap/interrupt handler. */
                    eflags.u32 &= ~(X86_EFL_TF | X86_EFL_VM | X86_EFL_RF | X86_EFL_NT);
#ifdef DEBUG
                    if (DBGFIsStepping(pVCpu))
                        eflags.u32 |= X86_EFL_TF;
#endif

                    /* Turn off interrupts for interrupt gates. */
                    if (GuestIdte.Gen.u5Type2 == VBOX_IDTE_TYPE2_INT_32)
                        eflags.Bits.u1IF = 0;

                    CPUMRawSetEFlags(pVCpu, eflags.u32);

#ifdef DEBUG
                    for (int j = idx; j < 0; j++)
                        LogFlow(("Stack %RRv pos %02d: %08x\n", &pTrapStack[j], j, pTrapStack[j]));

                    Log4(("eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x\n"
                          "eip=%08x esp=%08x ebp=%08x iopl=%d\n"
                          "cs=%04x ds=%04x es=%04x fs=%04x gs=%04x                       eflags=%08x\n",
                          pRegFrame->eax, pRegFrame->ebx, pRegFrame->ecx, pRegFrame->edx, pRegFrame->esi, pRegFrame->edi,
                          pRegFrame->eip, pRegFrame->esp, pRegFrame->ebp, eflags.Bits.u2IOPL,
                          pRegFrame->cs.Sel, pRegFrame->ds.Sel, pRegFrame->es.Sel,
                          pRegFrame->fs.Sel, pRegFrame->gs.Sel, eflags.u32));
#endif

                    Log(("TRPM: PATM Handler %RRv Adjusted stack %08X new EFLAGS=%08X/%08x idx=%d dpl=%d cpl=%d\n",
                         pVM->trpm.s.aGuestTrapHandler[iGate], esp_r0, eflags.u32, CPUMRawGetEFlags(pVCpu), idx, dpl, cpl));

                    /* Make sure the internal guest context structure is up-to-date. */
                    if (iGate == X86_XCPT_PF)
                        CPUMSetGuestCR2(pVCpu, pVCpu->trpm.s.uActiveCR2);

#ifdef IN_RC
                    /* paranoia */
                    Assert(pRegFrame->eflags.Bits.u1IF == 1);
                    eflags.Bits.u1IF   = 1;
                    Assert(pRegFrame->eflags.Bits.u2IOPL == 0);
                    eflags.Bits.u2IOPL = 0;

                    Assert(eflags.Bits.u1IF);
                    Assert(eflags.Bits.u2IOPL == 0);
                    STAM_COUNTER_INC(&pVM->trpm.s.CTX_SUFF(paStatForwardedIRQ)[iGate]);
                    STAM_PROFILE_ADV_STOP(&pVM->trpm.s.CTX_SUFF_Z(StatForwardProf), a);
                    if (iOrgTrap >= 0 && iOrgTrap < (int)RT_ELEMENTS(pVM->trpm.s.aStatGCTraps))
                        STAM_PROFILE_ADV_STOP(&pVM->trpm.s.aStatGCTraps[iOrgTrap], o);

                    PGMRZDynMapReleaseAutoSet(pVCpu);
                    CPUMGCCallGuestTrapHandler(pRegFrame, GuestIdte.Gen.u16SegSel | 1, pVM->trpm.s.aGuestTrapHandler[iGate],
                                               eflags.u32, ss_r0, (RTRCPTR)esp_r0);
                    /* does not return */
#else

                    Assert(!CPUMIsGuestInRawMode(pVCpu));
                    pRegFrame->eflags.u32 = eflags.u32;
                    pRegFrame->eip        = pVM->trpm.s.aGuestTrapHandler[iGate];
                    pRegFrame->cs.Sel     = GuestIdte.Gen.u16SegSel;
                    pRegFrame->esp        = esp_r0;
                    pRegFrame->ss.Sel     = ss_r0 & ~X86_SEL_RPL;     /* set rpl to ring 0 */
                    STAM_PROFILE_ADV_STOP(&pVM->trpm.s.CTX_SUFF_Z(StatForwardProf), a);
                    PGMPhysReleasePageMappingLock(pVM, &PageMappingLock);
                    NOREF(iOrgTrap);
                    return VINF_SUCCESS;
#endif
                }
                else
                    Log(("TRAP%02X: PGMVerifyAccess %RGv failed with %Rrc -> forward to REM\n", iGate, pTrapStackGC, rc));
            }
            else
                Log(("SELMValidateAndConvertCSAddr failed with %Rrc\n", rc));
        }
        else
            Log(("MMRamRead %RGv size %d failed with %Rrc\n", (RTGCUINTPTR)GCPtrIDT + sizeof(VBOXIDTE) * iGate, sizeof(GuestIdte), rc));
    }
    else
    {
        Log(("Refused to forward trap: eflags=%08x IF=%d\n", eflags.u32, eflags.Bits.u1IF));
#ifdef VBOX_WITH_STATISTICS
        if (pVM->trpm.s.aGuestTrapHandler[iGate] == TRPM_INVALID_HANDLER)
            STAM_COUNTER_INC(&pVM->trpm.s.StatForwardFailNoHandler);
        else if (PATMIsPatchGCAddr(pVM, pRegFrame->eip))
            STAM_COUNTER_INC(&pVM->trpm.s.StatForwardFailPatchAddr);
#endif
    }
failure:
    STAM_COUNTER_INC(&pVM->trpm.s.CTX_SUFF_Z(StatForwardFail));
    STAM_PROFILE_ADV_STOP(&pVM->trpm.s.CTX_SUFF_Z(StatForwardProf), a);

    Log(("TRAP%02X: forwarding to REM (ss rpl=%d eflags=%08X VMIF=%d handler=%08X\n", iGate, pRegFrame->ss.Sel & X86_SEL_RPL, pRegFrame->eflags.u32, PATMAreInterruptsEnabledByCtx(pVM, CPUMCTX_FROM_CORE(pRegFrame)), pVM->trpm.s.aGuestTrapHandler[iGate]));
#endif
    return VINF_EM_RAW_GUEST_TRAP;
}
#endif /* VBOX_WITH_RAW_MODE_NOT_R0 */


/**
 * Raises a cpu exception which doesn't take an error code.
 *
 * This function may or may not dispatch the exception before returning.
 *
 * @returns VBox status code fit for scheduling.
 * @retval  VINF_EM_RAW_GUEST_TRAP if the exception was left pending.
 * @retval  VINF_TRPM_XCPT_DISPATCHED if the exception was raised and dispatched for raw-mode execution.
 * @retval  VINF_EM_RESCHEDULE_REM if the exception was dispatched and cannot be executed in raw-mode.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pCtxCore    The CPU context core.
 * @param   enmXcpt     The exception.
 */
VMMDECL(int) TRPMRaiseXcpt(PVMCPU pVCpu, PCPUMCTXCORE pCtxCore, X86XCPT enmXcpt)
{
    LogFlow(("TRPMRaiseXcptErr: cs:eip=%RTsel:%RX32 enmXcpt=%#x\n", pCtxCore->cs.Sel, pCtxCore->eip, enmXcpt));
    NOREF(pCtxCore);
/** @todo dispatch the trap. */
    pVCpu->trpm.s.uActiveVector            = enmXcpt;
    pVCpu->trpm.s.enmActiveType            = TRPM_TRAP;
    pVCpu->trpm.s.uActiveErrorCode         = 0xdeadbeef;
    pVCpu->trpm.s.uActiveCR2               = 0xdeadface;
    pVCpu->trpm.s.cbInstr                  = UINT8_MAX;
    return VINF_EM_RAW_GUEST_TRAP;
}


/**
 * Raises a cpu exception with an errorcode.
 *
 * This function may or may not dispatch the exception before returning.
 *
 * @returns VBox status code fit for scheduling.
 * @retval  VINF_EM_RAW_GUEST_TRAP if the exception was left pending.
 * @retval  VINF_TRPM_XCPT_DISPATCHED if the exception was raised and dispatched for raw-mode execution.
 * @retval  VINF_EM_RESCHEDULE_REM if the exception was dispatched and cannot be executed in raw-mode.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pCtxCore    The CPU context core.
 * @param   enmXcpt     The exception.
 * @param   uErr        The error code.
 */
VMMDECL(int) TRPMRaiseXcptErr(PVMCPU pVCpu, PCPUMCTXCORE pCtxCore, X86XCPT enmXcpt, uint32_t uErr)
{
    LogFlow(("TRPMRaiseXcptErr: cs:eip=%RTsel:%RX32 enmXcpt=%#x uErr=%RX32\n", pCtxCore->cs.Sel, pCtxCore->eip, enmXcpt, uErr));
    NOREF(pCtxCore);
/** @todo dispatch the trap. */
    pVCpu->trpm.s.uActiveVector            = enmXcpt;
    pVCpu->trpm.s.enmActiveType            = TRPM_TRAP;
    pVCpu->trpm.s.uActiveErrorCode         = uErr;
    pVCpu->trpm.s.uActiveCR2               = 0xdeadface;
    pVCpu->trpm.s.cbInstr                  = UINT8_MAX;
    return VINF_EM_RAW_GUEST_TRAP;
}


/**
 * Raises a cpu exception with an errorcode and CR2.
 *
 * This function may or may not dispatch the exception before returning.
 *
 * @returns VBox status code fit for scheduling.
 * @retval  VINF_EM_RAW_GUEST_TRAP if the exception was left pending.
 * @retval  VINF_TRPM_XCPT_DISPATCHED if the exception was raised and dispatched for raw-mode execution.
 * @retval  VINF_EM_RESCHEDULE_REM if the exception was dispatched and cannot be executed in raw-mode.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pCtxCore    The CPU context core.
 * @param   enmXcpt     The exception.
 * @param   uErr        The error code.
 * @param   uCR2        The CR2 value.
 */
VMMDECL(int) TRPMRaiseXcptErrCR2(PVMCPU pVCpu, PCPUMCTXCORE pCtxCore, X86XCPT enmXcpt, uint32_t uErr, RTGCUINTPTR uCR2)
{
    LogFlow(("TRPMRaiseXcptErr: cs:eip=%RTsel:%RX32 enmXcpt=%#x uErr=%RX32 uCR2=%RGv\n", pCtxCore->cs.Sel, pCtxCore->eip, enmXcpt, uErr, uCR2));
    NOREF(pCtxCore);
/** @todo dispatch the trap. */
    pVCpu->trpm.s.uActiveVector            = enmXcpt;
    pVCpu->trpm.s.enmActiveType            = TRPM_TRAP;
    pVCpu->trpm.s.uActiveErrorCode         = uErr;
    pVCpu->trpm.s.uActiveCR2               = uCR2;
    pVCpu->trpm.s.cbInstr                  = UINT8_MAX;
    return VINF_EM_RAW_GUEST_TRAP;
}


#ifdef VBOX_WITH_RAW_MODE
/**
 * Clear guest trap/interrupt gate handler
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   iTrap       Interrupt/trap number.
 */
VMMDECL(int) trpmClearGuestTrapHandler(PVM pVM, unsigned iTrap)
{
    AssertReturn(!HMIsEnabled(pVM), VERR_TRPM_HM_IPE);
    AssertMsgReturn(iTrap < RT_ELEMENTS(pVM->trpm.s.aIdt), ("Illegal gate number %d!\n", iTrap), VERR_INVALID_PARAMETER);

    if (ASMBitTest(&pVM->trpm.s.au32IdtPatched[0], iTrap))
# ifdef IN_RING3
        trpmR3ClearPassThroughHandler(pVM, iTrap);
# else
        AssertFailed();
# endif

    pVM->trpm.s.aGuestTrapHandler[iTrap] = TRPM_INVALID_HANDLER;
    return VINF_SUCCESS;
}
#endif /* VBOX_WITH_RAW_MODE */

