/* $Id: CPUMRC.cpp $ */
/** @file
 * CPUM - Raw-mode Context Code.
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
#define LOG_GROUP LOG_GROUP_CPUM
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/vmm.h>
#include <VBox/vmm/patm.h>
#include <VBox/vmm/trpm.h>
#include <VBox/vmm/em.h>
#include "CPUMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/err.h>
#include <iprt/assert.h>
#include <VBox/log.h>
#include <iprt/asm-amd64-x86.h>


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
RT_C_DECLS_BEGIN /* addressed from asm (not called so no DECLASM). */
DECLCALLBACK(int) cpumRCHandleNPAndGP(PVM pVM, PCPUMCTXCORE pRegFrame, uintptr_t uUser);
RT_C_DECLS_END


/**
 * Deal with traps occurring during segment loading and IRET when resuming guest
 * context execution.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pRegFrame   The register frame.
 * @param   uUser       User argument. In this case a combination of the
 *                      CPUM_HANDLER_* \#defines.
 */
DECLCALLBACK(int) cpumRCHandleNPAndGP(PVM pVM, PCPUMCTXCORE pRegFrame, uintptr_t uUser)
{
    Log(("********************************************************\n"));
    Log(("cpumRCHandleNPAndGP: eip=%RX32 uUser=%#x\n", pRegFrame->eip, uUser));
    Log(("********************************************************\n"));

    /*
     * Take action based on what's happened.
     */
    switch (uUser & CPUM_HANDLER_TYPEMASK)
    {
        case CPUM_HANDLER_GS:
        case CPUM_HANDLER_DS:
        case CPUM_HANDLER_ES:
        case CPUM_HANDLER_FS:
            TRPMGCHyperReturnToHost(pVM, VINF_EM_RAW_STALE_SELECTOR);
            break;

        case CPUM_HANDLER_IRET:
            TRPMGCHyperReturnToHost(pVM, VINF_EM_RAW_IRET_TRAP);
            break;
    }

    AssertMsgFailed(("uUser=%#x eip=%#x\n", uUser, pRegFrame->eip)); RT_NOREF_PV(pRegFrame);
    return VERR_TRPM_DONT_PANIC;
}


/**
 * Called by TRPM and CPUM assembly code to make sure the guest state is
 * ready for execution.
 *
 * @param   pVM                 The cross context VM structure.
 */
DECLASM(void) CPUMRCAssertPreExecutionSanity(PVM pVM)
{
#ifdef VBOX_STRICT
    /*
     * Check some important assumptions before resuming guest execution.
     */
    PVMCPU         pVCpu     = VMMGetCpu0(pVM);
    PCCPUMCTX      pCtx      = &pVCpu->cpum.s.Guest;
    uint8_t  const uRawCpl   = CPUMGetGuestCPL(pVCpu);
    uint32_t const u32EFlags = CPUMRawGetEFlags(pVCpu);
    bool     const fPatch    = PATMIsPatchGCAddr(pVM, pCtx->eip);
    AssertMsg(pCtx->eflags.Bits.u1IF,                ("cs:eip=%04x:%08x ss:esp=%04x:%08x cpl=%u raw/efl=%#x/%#x%s\n", pCtx->cs.Sel, pCtx->eip, pCtx->ss.Sel, pCtx->esp, uRawCpl, u32EFlags, pCtx->eflags.u, fPatch ? " patch" : ""));
    AssertMsg(pCtx->eflags.Bits.u2IOPL < RT_MAX(uRawCpl, 1U),
                                                     ("cs:eip=%04x:%08x ss:esp=%04x:%08x cpl=%u raw/efl=%#x/%#x%s\n", pCtx->cs.Sel, pCtx->eip, pCtx->ss.Sel, pCtx->esp, uRawCpl, u32EFlags, pCtx->eflags.u, fPatch ? " patch" : ""));
    if (!(u32EFlags & X86_EFL_VM))
    {
        AssertMsg((u32EFlags & X86_EFL_IF) || fPatch,("cs:eip=%04x:%08x ss:esp=%04x:%08x cpl=%u raw/efl=%#x/%#x%s\n", pCtx->cs.Sel, pCtx->eip, pCtx->ss.Sel, pCtx->esp, uRawCpl, u32EFlags, pCtx->eflags.u, fPatch ? " patch" : ""));
        AssertMsg((pCtx->cs.Sel & X86_SEL_RPL) > 0,  ("cs:eip=%04x:%08x ss:esp=%04x:%08x cpl=%u raw/efl=%#x/%#x%s\n", pCtx->cs.Sel, pCtx->eip, pCtx->ss.Sel, pCtx->esp, uRawCpl, u32EFlags, pCtx->eflags.u, fPatch ? " patch" : ""));
        AssertMsg((pCtx->ss.Sel & X86_SEL_RPL) > 0,  ("cs:eip=%04x:%08x ss:esp=%04x:%08x cpl=%u raw/efl=%#x/%#x%s\n", pCtx->cs.Sel, pCtx->eip, pCtx->ss.Sel, pCtx->esp, uRawCpl, u32EFlags, pCtx->eflags.u, fPatch ? " patch" : ""));
    }
    AssertMsg(CPUMIsGuestInRawMode(pVCpu),           ("cs:eip=%04x:%08x ss:esp=%04x:%08x cpl=%u raw/efl=%#x/%#x%s\n", pCtx->cs.Sel, pCtx->eip, pCtx->ss.Sel, pCtx->esp, uRawCpl, u32EFlags, pCtx->eflags.u, fPatch ? " patch" : ""));
    //Log2(("cs:eip=%04x:%08x ss:esp=%04x:%08x cpl=%u raw/efl=%#x/%#x%s\n", pCtx->cs.Sel, pCtx->eip, pCtx->ss.Sel, pCtx->esp, uRawCpl, u32EFlags, pCtx->eflags.u, fPatch ? " patch" : ""));
#else
    RT_NOREF_PV(pVM);
#endif
}


/**
 * Get the current privilege level of the guest.
 *
 * @returns CPL
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pRegFrame   Pointer to the register frame.
 *
 * @todo    r=bird: This is very similar to CPUMGetGuestCPL and I cannot quite
 *          see why this variant of the code is necessary.
 */
VMMDECL(uint32_t) CPUMRCGetGuestCPL(PVMCPU pVCpu, PCPUMCTXCORE pRegFrame)
{
    /*
     * CPL can reliably be found in SS.DPL (hidden regs valid) or SS if not.
     *
     * Note! We used to check CS.DPL here, assuming it was always equal to
     * CPL even if a conforming segment was loaded.  But this truned out to
     * only apply to older AMD-V.  With VT-x we had an ACP2 regression
     * during install after a far call to ring 2 with VT-x.  Then on newer
     * AMD-V CPUs we have to move the VMCB.guest.u8CPL into cs.Attr.n.u2Dpl
     * as well as ss.Attr.n.u2Dpl to make this (and other) code work right.
     *
     * So, forget CS.DPL, always use SS.DPL.
     *
     * Note! The SS RPL is always equal to the CPL, while the CS RPL
     * isn't necessarily equal if the segment is conforming.
     * See section 4.11.1 in the AMD manual.
     */
    uint32_t uCpl;
    if (!pRegFrame->eflags.Bits.u1VM)
    {
        uCpl = (pRegFrame->ss.Sel & X86_SEL_RPL);
#ifdef VBOX_WITH_RAW_MODE_NOT_R0
# ifdef VBOX_WITH_RAW_RING1
        if (pVCpu->cpum.s.fRawEntered)
        {
            if (   uCpl == 2
                && EMIsRawRing1Enabled(pVCpu->CTX_SUFF(pVM)) )
                uCpl = 1;
            else if (uCpl == 1)
                uCpl = 0;
        }
        Assert(uCpl != 2);  /* ring 2 support not allowed anymore. */
# else
        if (uCpl == 1)
            uCpl = 0;
# endif
#endif
    }
    else
        uCpl = 3; /* V86 has CPL=3; REM doesn't set DPL=3 in V8086 mode. See @bugref{5130}. */

    return uCpl;
}


#ifdef VBOX_WITH_RAW_RING1
/**
 * Transforms the guest CPU state to raw-ring mode.
 *
 * This function will change the any of the cs and ss register with DPL=0 to DPL=1.
 *
 * Used by emInterpretIret() after the new state has been loaded.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtxCore    The context core (for trap usage).
 * @see     @ref pg_raw
 * @remarks Will be probably obsoleted by #5653 (it will leave and reenter raw
 *          mode instead, I think).
 */
VMMDECL(void) CPUMRCRecheckRawState(PVMCPU pVCpu, PCPUMCTXCORE pCtxCore)
{
    /*
     * Are we in Ring-0?
     */
    if (    pCtxCore->ss.Sel
        &&  (pCtxCore->ss.Sel & X86_SEL_RPL) == 0
        &&  !pCtxCore->eflags.Bits.u1VM)
    {
        /*
         * Set CPL to Ring-1.
         */
        pCtxCore->ss.Sel |= 1;
        if (    pCtxCore->cs.Sel
            &&  (pCtxCore->cs.Sel & X86_SEL_RPL) == 0)
            pCtxCore->cs.Sel |= 1;
    }
    else
    {
        if (    EMIsRawRing1Enabled(pVCpu->CTX_SUFF(pVM))
            &&  !pCtxCore->eflags.Bits.u1VM
            &&  (pCtxCore->ss.Sel & X86_SEL_RPL) == 1)
        {
            /* Set CPL to Ring-2. */
            pCtxCore->ss.Sel = (pCtxCore->ss.Sel & ~X86_SEL_RPL) | 2;
            if (pCtxCore->cs.Sel && (pCtxCore->cs.Sel & X86_SEL_RPL) == 1)
                pCtxCore->cs.Sel = (pCtxCore->cs.Sel & ~X86_SEL_RPL) | 2;
        }
    }

    /*
     * Assert sanity.
     */
    AssertMsg((pCtxCore->eflags.u32 & X86_EFL_IF), ("X86_EFL_IF is clear\n"));
    AssertReleaseMsg(pCtxCore->eflags.Bits.u2IOPL == 0,
                     ("X86_EFL_IOPL=%d CPL=%d\n", pCtxCore->eflags.Bits.u2IOPL, pCtxCore->ss.Sel & X86_SEL_RPL));

    pCtxCore->eflags.u32        |= X86_EFL_IF; /* paranoia */
}
#endif /* VBOX_WITH_RAW_RING1 */


/**
 * Called by trpmGCExitTrap when VMCPU_FF_CPUM is set (by CPUMRZ.cpp).
 *
 * We can be called unecessarily here if we returned to ring-3 for some other
 * reason before we tried to resume executed guest code.  This is detected and
 * ignored.
 *
 * @param   pVCpu   The cross context CPU structure for the calling EMT.
 */
VMMRCDECL(void) CPUMRCProcessForceFlag(PVMCPU pVCpu)
{
    /* Only modify CR0 if we're in the post IEM state (host state saved, guest no longer active). */
    if ((pVCpu->cpum.s.fUseFlags & (CPUM_USED_FPU_GUEST | CPUM_USED_FPU_HOST)) == CPUM_USED_FPU_HOST)
    {
        /*
         * Doing the same CR0 calculation as in AMD64andLegacy.mac so that we'll
         * catch guest FPU accesses and load the FPU/SSE/AVX register state as needed.
         */
        uint32_t cr0 = ASMGetCR0();
        cr0 |= pVCpu->cpum.s.Guest.cr0 & X86_CR0_EM;
        cr0 |= X86_CR0_TS | X86_CR0_MP;
        ASMSetCR0(cr0);
        Log6(("CPUMRCProcessForceFlag: cr0=%#x\n", cr0));
    }
    else
        Log6(("CPUMRCProcessForceFlag: no change -  cr0=%#x\n", ASMGetCR0()));
}

