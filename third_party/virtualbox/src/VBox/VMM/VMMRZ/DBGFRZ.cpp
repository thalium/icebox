/* $Id: DBGFRZ.cpp $ */
/** @file
 * DBGF - Debugger Facility, RZ part.
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
#define LOG_GROUP LOG_GROUP_DBGF
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/selm.h>
#ifdef IN_RC
# include <VBox/vmm/trpm.h>
#endif
#include <VBox/log.h>
#include "DBGFInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/err.h>
#include <iprt/assert.h>

#ifdef IN_RC
DECLASM(void) TRPMRCHandlerAsmTrap03(void);
#endif


/**
 * \#DB (Debug event) handler.
 *
 * @returns VBox status code.
 *          VINF_SUCCESS means we completely handled this trap,
 *          other codes are passed execution to host context.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pRegFrame       Pointer to the register frame for the trap.
 * @param   uDr6            The DR6 hypervisor register value.
 * @param   fAltStepping    Alternative stepping indicator.
 */
VMMRZ_INT_DECL(int) DBGFRZTrap01Handler(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, RTGCUINTREG uDr6, bool fAltStepping)
{
#ifdef IN_RC
    const bool fInHyper = !(pRegFrame->ss.Sel & X86_SEL_RPL) && !pRegFrame->eflags.Bits.u1VM;
#else
    NOREF(pRegFrame);
    const bool fInHyper = false;
#endif

    /** @todo Intel docs say that X86_DR6_BS has the highest priority... */
    /*
     * A breakpoint?
     */
    AssertCompile(X86_DR6_B0 == 1 && X86_DR6_B1 == 2 && X86_DR6_B2 == 4 && X86_DR6_B3 == 8);
    if (   (uDr6 & (X86_DR6_B0 | X86_DR6_B1 | X86_DR6_B2 | X86_DR6_B3))
        && pVM->dbgf.s.cEnabledHwBreakpoints > 0)
    {
        for (unsigned iBp = 0; iBp < RT_ELEMENTS(pVM->dbgf.s.aHwBreakpoints); iBp++)
        {
            if (    ((uint32_t)uDr6 & RT_BIT_32(iBp))
                &&  pVM->dbgf.s.aHwBreakpoints[iBp].enmType == DBGFBPTYPE_REG)
            {
                pVCpu->dbgf.s.iActiveBp = pVM->dbgf.s.aHwBreakpoints[iBp].iBp;
                pVCpu->dbgf.s.fSingleSteppingRaw = false;
                LogFlow(("DBGFRZTrap03Handler: hit hw breakpoint %d at %04x:%RGv\n",
                         pVM->dbgf.s.aHwBreakpoints[iBp].iBp, pRegFrame->cs.Sel, pRegFrame->rip));

                return fInHyper ? VINF_EM_DBG_HYPER_BREAKPOINT : VINF_EM_DBG_BREAKPOINT;
            }
        }
    }

    /*
     * Single step?
     * Are we single stepping or is it the guest?
     */
    if (    (uDr6 & X86_DR6_BS)
        &&  (fInHyper || pVCpu->dbgf.s.fSingleSteppingRaw || fAltStepping))
    {
        pVCpu->dbgf.s.fSingleSteppingRaw = false;
        LogFlow(("DBGFRZTrap01Handler: single step at %04x:%RGv\n", pRegFrame->cs.Sel, pRegFrame->rip));
        return fInHyper ? VINF_EM_DBG_HYPER_STEPPED : VINF_EM_DBG_STEPPED;
    }

#ifdef IN_RC
    /*
     * Either an ICEBP in hypervisor code or a guest related debug exception
     * of sorts.
     */
    if (RT_UNLIKELY(fInHyper))
    {
        /*
         * Is this a guest debug event that was delayed past a ring transition?
         *
         * Since we do no allow sysenter/syscall in raw-mode, the  only
         * non-trap/fault type transitions that can occur are thru interrupt gates.
         * Of those, only INT3 (#BP) has a DPL other than 0 with a CS.RPL of 0.
         * See bugref:9171 and bs3-cpu-weird-1 for more details.
         *
         * We need to reconstruct the guest register state from the hypervisor one
         * here, so here is the layout of the IRET frame on the stack:
         *    20:[8] GS          (V86 only)
         *    1C:[7] FS          (V86 only)
         *    18:[6] DS          (V86 only)
         *    14:[5] ES          (V86 only)
         *    10:[4] SS
         *    0c:[3] ESP
         *    08:[2] EFLAGS
         *    04:[1] CS
         *    00:[0] EIP
         */
        if (pRegFrame->rip == (uintptr_t)TRPMRCHandlerAsmTrap03)
        {
            uint32_t const *pu32Stack = (uint32_t const *)pRegFrame->esp;
            if (   (pu32Stack[2] & X86_EFL_VM)
                || (pu32Stack[1] & X86_SEL_RPL))
            {
                LogFlow(("DBGFRZTrap01Handler: Detected guest #DB delayed past ring transition %04x:%RX32 %#x\n",
                         pu32Stack[1] & 0xffff, pu32Stack[0], pu32Stack[2]));
                PCPUMCTX pGstCtx = CPUMQueryGuestCtxPtr(pVCpu);
                pGstCtx->rip      = pu32Stack[0];
                pGstCtx->cs.Sel   = pu32Stack[1];
                pGstCtx->eflags.u = pu32Stack[2];
                pGstCtx->rsp      = pu32Stack[3];
                pGstCtx->ss.Sel   = pu32Stack[4];
                if (pu32Stack[2] & X86_EFL_VM)
                {
                    pGstCtx->es.Sel = pu32Stack[5];
                    pGstCtx->ds.Sel = pu32Stack[6];
                    pGstCtx->fs.Sel = pu32Stack[7];
                    pGstCtx->gs.Sel = pu32Stack[8];
                }
                else
                {
                    pGstCtx->es.Sel = pRegFrame->es.Sel;
                    pGstCtx->ds.Sel = pRegFrame->ds.Sel;
                    pGstCtx->fs.Sel = pRegFrame->fs.Sel;
                    pGstCtx->gs.Sel = pRegFrame->gs.Sel;
                }
                pGstCtx->rax      = pRegFrame->rax;
                pGstCtx->rcx      = pRegFrame->rcx;
                pGstCtx->rdx      = pRegFrame->rdx;
                pGstCtx->rbx      = pRegFrame->rbx;
                pGstCtx->rsi      = pRegFrame->rsi;
                pGstCtx->rdi      = pRegFrame->rdi;
                pGstCtx->rbp      = pRegFrame->rbp;

                /*
                 * We should assert a #BP followed by a #DB here, but TRPM cannot
                 * do that.  So, we'll just assert the #BP and ignore the #DB, even
                 * if that isn't strictly correct.
                 */
                TRPMResetTrap(pVCpu);
                TRPMAssertTrap(pVCpu, X86_XCPT_BP, TRPM_SOFTWARE_INT);
                return VINF_EM_RAW_GUEST_TRAP;
            }
        }

        LogFlow(("DBGFRZTrap01Handler: Unknown bp at %04x:%RGv\n", pRegFrame->cs.Sel, pRegFrame->rip));
        return VERR_DBGF_HYPER_DB_XCPT;
    }
#endif

    LogFlow(("DBGFRZTrap01Handler: guest debug event %#x at %04x:%RGv!\n", (uint32_t)uDr6, pRegFrame->cs.Sel, pRegFrame->rip));
    return VINF_EM_RAW_GUEST_TRAP;
}


/**
 * \#BP (Breakpoint) handler.
 *
 * @returns VBox status code.
 *          VINF_SUCCESS means we completely handled this trap,
 *          other codes are passed execution to host context.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pRegFrame   Pointer to the register frame for the trap.
 */
VMMRZ_INT_DECL(int) DBGFRZTrap03Handler(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame)
{
#ifdef IN_RC
    const bool fInHyper = !(pRegFrame->ss.Sel & X86_SEL_RPL) && !pRegFrame->eflags.Bits.u1VM;
#else
    const bool fInHyper = false;
#endif

    /*
     * Get the trap address and look it up in the breakpoint table.
     * Don't bother if we don't have any breakpoints.
     */
    unsigned cToSearch = pVM->dbgf.s.Int3.cToSearch;
    if (cToSearch > 0)
    {
        RTGCPTR pPc;
        int rc = SELMValidateAndConvertCSAddr(pVCpu, pRegFrame->eflags, pRegFrame->ss.Sel, pRegFrame->cs.Sel, &pRegFrame->cs,
#ifdef IN_RC
                                              pRegFrame->eip - 1,
#else
                                              pRegFrame->rip /* no -1 in R0 */,
#endif
                                              &pPc);
        AssertRCReturn(rc, rc);

        unsigned iBp = pVM->dbgf.s.Int3.iStartSearch;
        while (cToSearch-- > 0)
        {
            if (   pVM->dbgf.s.aBreakpoints[iBp].u.GCPtr == (RTGCUINTPTR)pPc
                && pVM->dbgf.s.aBreakpoints[iBp].enmType == DBGFBPTYPE_INT3)
            {
                pVM->dbgf.s.aBreakpoints[iBp].cHits++;
                pVCpu->dbgf.s.iActiveBp = pVM->dbgf.s.aBreakpoints[iBp].iBp;

                LogFlow(("DBGFRZTrap03Handler: hit breakpoint %d at %RGv (%04x:%RGv) cHits=0x%RX64\n",
                         pVM->dbgf.s.aBreakpoints[iBp].iBp, pPc, pRegFrame->cs.Sel, pRegFrame->rip,
                         pVM->dbgf.s.aBreakpoints[iBp].cHits));
                return fInHyper
                     ? VINF_EM_DBG_HYPER_BREAKPOINT
                     : VINF_EM_DBG_BREAKPOINT;
            }
            iBp++;
        }
    }

    return fInHyper
         ? VINF_EM_DBG_HYPER_ASSERTION
         : VINF_EM_RAW_GUEST_TRAP;
}

