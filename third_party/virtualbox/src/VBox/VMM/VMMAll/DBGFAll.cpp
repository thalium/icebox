/* $Id: DBGFAll.cpp $ */
/** @file
 * DBGF - Debugger Facility, All Context Code.
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
#include "DBGFInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/err.h>
#include <iprt/assert.h>
#include <iprt/asm.h>


/*
 * Check the read-only VM members.
 */
AssertCompileMembersSameSizeAndOffset(VM, dbgf.s.bmSoftIntBreakpoints,  VM, dbgf.ro.bmSoftIntBreakpoints);
AssertCompileMembersSameSizeAndOffset(VM, dbgf.s.bmHardIntBreakpoints,  VM, dbgf.ro.bmHardIntBreakpoints);
AssertCompileMembersSameSizeAndOffset(VM, dbgf.s.bmSelectedEvents,      VM, dbgf.ro.bmSelectedEvents);
AssertCompileMembersSameSizeAndOffset(VM, dbgf.s.cHardIntBreakpoints,   VM, dbgf.ro.cHardIntBreakpoints);
AssertCompileMembersSameSizeAndOffset(VM, dbgf.s.cSoftIntBreakpoints,   VM, dbgf.ro.cSoftIntBreakpoints);
AssertCompileMembersSameSizeAndOffset(VM, dbgf.s.cSelectedEvents,       VM, dbgf.ro.cSelectedEvents);


/**
 * Gets the hardware breakpoint configuration as DR7.
 *
 * @returns DR7 from the DBGF point of view.
 * @param   pVM         The cross context VM structure.
 */
VMM_INT_DECL(RTGCUINTREG) DBGFBpGetDR7(PVM pVM)
{
    RTGCUINTREG uDr7 = X86_DR7_GD | X86_DR7_GE | X86_DR7_LE | X86_DR7_RA1_MASK;
    PDBGFBP     pBp = &pVM->dbgf.s.aHwBreakpoints[0];
    unsigned    cLeft = RT_ELEMENTS(pVM->dbgf.s.aHwBreakpoints);
    while (cLeft-- > 0)
    {
        if (    pBp->enmType == DBGFBPTYPE_REG
            &&  pBp->fEnabled)
        {
            static const uint8_t s_au8Sizes[8] =
            {
                X86_DR7_LEN_BYTE, X86_DR7_LEN_BYTE, X86_DR7_LEN_WORD, X86_DR7_LEN_BYTE,
                X86_DR7_LEN_DWORD,X86_DR7_LEN_BYTE, X86_DR7_LEN_BYTE, X86_DR7_LEN_QWORD
            };
            uDr7 |= X86_DR7_G(pBp->u.Reg.iReg)
                 |  X86_DR7_RW(pBp->u.Reg.iReg, pBp->u.Reg.fType)
                 |  X86_DR7_LEN(pBp->u.Reg.iReg, s_au8Sizes[pBp->u.Reg.cb]);
        }
        pBp++;
    }
    return uDr7;
}


/**
 * Gets the address of the hardware breakpoint number 0.
 *
 * @returns DR0 from the DBGF point of view.
 * @param   pVM         The cross context VM structure.
 */
VMM_INT_DECL(RTGCUINTREG) DBGFBpGetDR0(PVM pVM)
{
    PCDBGFBP    pBp = &pVM->dbgf.s.aHwBreakpoints[0];
    Assert(pBp->u.Reg.iReg == 0);
    return pBp->u.Reg.GCPtr;
}


/**
 * Gets the address of the hardware breakpoint number 1.
 *
 * @returns DR1 from the DBGF point of view.
 * @param   pVM         The cross context VM structure.
 */
VMM_INT_DECL(RTGCUINTREG) DBGFBpGetDR1(PVM pVM)
{
    PCDBGFBP    pBp = &pVM->dbgf.s.aHwBreakpoints[1];
    Assert(pBp->u.Reg.iReg == 1);
    return pBp->u.Reg.GCPtr;
}


/**
 * Gets the address of the hardware breakpoint number 2.
 *
 * @returns DR2 from the DBGF point of view.
 * @param   pVM         The cross context VM structure.
 */
VMM_INT_DECL(RTGCUINTREG) DBGFBpGetDR2(PVM pVM)
{
    PCDBGFBP    pBp = &pVM->dbgf.s.aHwBreakpoints[2];
    Assert(pBp->u.Reg.iReg == 2);
    return pBp->u.Reg.GCPtr;
}


/**
 * Gets the address of the hardware breakpoint number 3.
 *
 * @returns DR3 from the DBGF point of view.
 * @param   pVM         The cross context VM structure.
 */
VMM_INT_DECL(RTGCUINTREG) DBGFBpGetDR3(PVM pVM)
{
    PCDBGFBP    pBp = &pVM->dbgf.s.aHwBreakpoints[3];
    Assert(pBp->u.Reg.iReg == 3);
    return pBp->u.Reg.GCPtr;
}


/**
 * Checks if any of the hardware breakpoints are armed.
 *
 * @returns true if armed, false if not.
 * @param   pVM        The cross context VM structure.
 * @remarks Don't call this from CPUMRecalcHyperDRx!
 */
VMM_INT_DECL(bool) DBGFBpIsHwArmed(PVM pVM)
{
    return pVM->dbgf.s.cEnabledHwBreakpoints > 0;
}


/**
 * Checks if any of the hardware I/O breakpoints are armed.
 *
 * @returns true if armed, false if not.
 * @param   pVM        The cross context VM structure.
 * @remarks Don't call this from CPUMRecalcHyperDRx!
 */
VMM_INT_DECL(bool) DBGFBpIsHwIoArmed(PVM pVM)
{
    return pVM->dbgf.s.cEnabledHwIoBreakpoints > 0;
}


/**
 * Checks if any INT3 breakpoints are armed.
 *
 * @returns true if armed, false if not.
 * @param   pVM        The cross context VM structure.
 * @remarks Don't call this from CPUMRecalcHyperDRx!
 */
VMM_INT_DECL(bool) DBGFBpIsInt3Armed(PVM pVM)
{
    return pVM->dbgf.s.cEnabledInt3Breakpoints > 0;
}


/**
 * Checks I/O access for guest or hypervisor breakpoints.
 *
 * @returns Strict VBox status code
 * @retval  VINF_SUCCESS no breakpoint.
 * @retval  VINF_EM_DBG_BREAKPOINT hypervisor breakpoint triggered.
 * @retval  VINF_EM_RAW_GUEST_TRAP guest breakpoint triggered, DR6 and DR7 have
 *          been updated appropriately.
 *
 * @param   pVM        The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pCtx        The CPU context for the calling EMT.
 * @param   uIoPort     The I/O port being accessed.
 * @param   cbValue     The size/width of the access, in bytes.
 */
VMM_INT_DECL(VBOXSTRICTRC)  DBGFBpCheckIo(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, RTIOPORT uIoPort, uint8_t cbValue)
{
    uint32_t const uIoPortFirst = uIoPort;
    uint32_t const uIoPortLast  = uIoPortFirst + cbValue - 1;


    /*
     * Check hyper breakpoints first as the VMM debugger has priority over
     * the guest.
     */
    if (pVM->dbgf.s.cEnabledHwIoBreakpoints > 0)
    {
        for (unsigned iBp = 0; iBp < RT_ELEMENTS(pVM->dbgf.s.aHwBreakpoints); iBp++)
        {
            if (   pVM->dbgf.s.aHwBreakpoints[iBp].u.Reg.fType == X86_DR7_RW_IO
                && pVM->dbgf.s.aHwBreakpoints[iBp].fEnabled
                && pVM->dbgf.s.aHwBreakpoints[iBp].enmType     == DBGFBPTYPE_REG )
            {
                uint8_t  cbReg      = pVM->dbgf.s.aHwBreakpoints[iBp].u.Reg.cb; Assert(RT_IS_POWER_OF_TWO(cbReg));
                uint64_t uDrXFirst  = pVM->dbgf.s.aHwBreakpoints[iBp].u.Reg.GCPtr & ~(uint64_t)(cbReg - 1);
                uint64_t uDrXLast   = uDrXFirst + cbReg - 1;
                if (uDrXFirst <= uIoPortLast && uDrXLast >= uIoPortFirst)
                {
                    /* (See also DBGFRZTrap01Handler.) */
                    pVCpu->dbgf.s.iActiveBp = pVM->dbgf.s.aHwBreakpoints[iBp].iBp;
                    pVCpu->dbgf.s.fSingleSteppingRaw = false;

                    LogFlow(("DBGFBpCheckIo: hit hw breakpoint %d at %04x:%RGv (iop %#x)\n",
                             pVM->dbgf.s.aHwBreakpoints[iBp].iBp, pCtx->cs.Sel, pCtx->rip, uIoPort));
                    return VINF_EM_DBG_BREAKPOINT;
                }
            }
        }
    }

    /*
     * Check the guest.
     */
    uint32_t const uDr7 = pCtx->dr[7];
    if (   (uDr7 & X86_DR7_ENABLED_MASK)
        && X86_DR7_ANY_RW_IO(uDr7)
        && (pCtx->cr4 & X86_CR4_DE) )
    {
        for (unsigned iBp = 0; iBp < 4; iBp++)
        {
            if (   (uDr7 & X86_DR7_L_G(iBp))
                && X86_DR7_GET_RW(uDr7, iBp) == X86_DR7_RW_IO)
            {
                /* ASSUME the breakpoint and the I/O width qualifier uses the same encoding (1 2 x 4). */
                static uint8_t const s_abInvAlign[4] = { 0, 1, 7, 3 };
                uint8_t  cbInvAlign = s_abInvAlign[X86_DR7_GET_LEN(uDr7, iBp)];
                uint64_t uDrXFirst  = pCtx->dr[iBp] & ~(uint64_t)cbInvAlign;
                uint64_t uDrXLast   = uDrXFirst + cbInvAlign;

                if (uDrXFirst <= uIoPortLast && uDrXLast >= uIoPortFirst)
                {
                    /*
                     * Update DR6 and DR7.
                     *
                     * See "AMD64 Architecture Programmer's Manual Volume 2",
                     * chapter 13.1.1.3 for details on DR6 bits.  The basics is
                     * that the B0..B3 bits are always cleared while the others
                     * must be cleared by software.
                     *
                     * The following sub chapters says the GD bit is always
                     * cleared when generating a #DB so the handler can safely
                     * access the debug registers.
                     */
                    pCtx->dr[6] &= ~X86_DR6_B_MASK;
                    pCtx->dr[6] |= X86_DR6_B(iBp);
                    pCtx->dr[7] &= ~X86_DR7_GD;
                    LogFlow(("DBGFBpCheckIo: hit hw breakpoint %d at %04x:%RGv (iop %#x)\n",
                             pVM->dbgf.s.aHwBreakpoints[iBp].iBp, pCtx->cs.Sel, pCtx->rip, uIoPort));
                    return VINF_EM_RAW_GUEST_TRAP;
                }
            }
        }
    }
    return VINF_SUCCESS;
}


/**
 * Returns the single stepping state for a virtual CPU.
 *
 * @returns stepping (true) or not (false).
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMM_INT_DECL(bool) DBGFIsStepping(PVMCPU pVCpu)
{
    return pVCpu->dbgf.s.fSingleSteppingRaw;
}


/**
 * Checks if the specified generic event is enabled or not.
 *
 * @returns true / false.
 * @param   pVM                 The cross context VM structure.
 * @param   enmEvent            The generic event being raised.
 * @param   uEventArg           The argument of that event.
 */
DECLINLINE(bool) dbgfEventIsGenericWithArgEnabled(PVM pVM, DBGFEVENTTYPE enmEvent, uint64_t uEventArg)
{
    if (DBGF_IS_EVENT_ENABLED(pVM, enmEvent))
    {
        switch (enmEvent)
        {
            case DBGFEVENT_INTERRUPT_HARDWARE:
                AssertReturn(uEventArg < 256, false);
                return ASMBitTest(pVM->dbgf.s.bmHardIntBreakpoints, (uint32_t)uEventArg);

            case DBGFEVENT_INTERRUPT_SOFTWARE:
                AssertReturn(uEventArg < 256, false);
                return ASMBitTest(pVM->dbgf.s.bmSoftIntBreakpoints, (uint32_t)uEventArg);

            default:
                return true;

        }
    }
    return false;
}


/**
 * Raises a generic debug event if enabled and not being ignored.
 *
 * @returns Strict VBox status code.
 * @retval  VINF_EM_DBG_EVENT if the event was raised and the caller should
 *          return ASAP to the debugger (via EM).  We set VMCPU_FF_DBGF so, it
 *          is okay not to pass this along in some situations  .
 * @retval  VINF_SUCCESS if the event was disabled or ignored.
 *
 * @param   pVM                 The cross context VM structure.
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   enmEvent            The generic event being raised.
 * @param   uEventArg           The argument of that event.
 * @param   enmCtx              The context in which this event is being raised.
 *
 * @thread  EMT(pVCpu)
 */
VMM_INT_DECL(VBOXSTRICTRC) DBGFEventGenericWithArg(PVM pVM, PVMCPU pVCpu, DBGFEVENTTYPE enmEvent, uint64_t uEventArg,
                                                   DBGFEVENTCTX enmCtx)
{
    /*
     * Is it enabled.
     */
    if (dbgfEventIsGenericWithArgEnabled(pVM, enmEvent, uEventArg))
    {
        /*
         * Any events on the stack. Should the incoming event be ignored?
         */
        uint64_t const rip = CPUMGetGuestRIP(pVCpu);
        uint32_t i = pVCpu->dbgf.s.cEvents;
        if (i > 0)
        {
            while (i-- > 0)
            {
                if (   pVCpu->dbgf.s.aEvents[i].Event.enmType   == enmEvent
                    && pVCpu->dbgf.s.aEvents[i].enmState        == DBGFEVENTSTATE_IGNORE
                    && pVCpu->dbgf.s.aEvents[i].rip             == rip)
                {
                    pVCpu->dbgf.s.aEvents[i].enmState = DBGFEVENTSTATE_RESTORABLE;
                    return VINF_SUCCESS;
                }
                Assert(pVCpu->dbgf.s.aEvents[i].enmState != DBGFEVENTSTATE_CURRENT);
            }

            /*
             * Trim the event stack.
             */
            i = pVCpu->dbgf.s.cEvents;
            while (i-- > 0)
            {
                if (   pVCpu->dbgf.s.aEvents[i].rip == rip
                    && (   pVCpu->dbgf.s.aEvents[i].enmState == DBGFEVENTSTATE_RESTORABLE
                        || pVCpu->dbgf.s.aEvents[i].enmState == DBGFEVENTSTATE_IGNORE) )
                    pVCpu->dbgf.s.aEvents[i].enmState = DBGFEVENTSTATE_IGNORE;
                else
                {
                    if (i + 1 != pVCpu->dbgf.s.cEvents)
                        memmove(&pVCpu->dbgf.s.aEvents[i], &pVCpu->dbgf.s.aEvents[i + 1],
                                (pVCpu->dbgf.s.cEvents - i) * sizeof(pVCpu->dbgf.s.aEvents));
                    pVCpu->dbgf.s.cEvents--;
                }
            }

            i = pVCpu->dbgf.s.cEvents;
            AssertStmt(i < RT_ELEMENTS(pVCpu->dbgf.s.aEvents), i = RT_ELEMENTS(pVCpu->dbgf.s.aEvents) - 1);
        }

        /*
         * Push the event.
         */
        pVCpu->dbgf.s.aEvents[i].enmState               = DBGFEVENTSTATE_CURRENT;
        pVCpu->dbgf.s.aEvents[i].rip                    = rip;
        pVCpu->dbgf.s.aEvents[i].Event.enmType          = enmEvent;
        pVCpu->dbgf.s.aEvents[i].Event.enmCtx           = enmCtx;
        pVCpu->dbgf.s.aEvents[i].Event.u.Generic.uArg   = uEventArg;
        pVCpu->dbgf.s.cEvents = i + 1;

        VMCPU_FF_SET(pVCpu, VMCPU_FF_DBGF);
        return VINF_EM_DBG_EVENT;
    }

    return VINF_SUCCESS;
}

