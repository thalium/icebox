/* $Id: PATMAll.cpp $ */
/** @file
 * PATM - The Patch Manager, all contexts.
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
#define LOG_GROUP LOG_GROUP_PATM
#include <VBox/vmm/patm.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/selm.h>
#include <VBox/vmm/mm.h>
#include "PATMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/vmm.h>
#include "PATMA.h"

#include <VBox/dis.h>
#include <VBox/disopcode.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/string.h>


/**
 * @callback_method_impl{FNPGMPHYSHANDLER, PATM all access handler callback.}
 *
 * @remarks The @a pvUser argument is the base address of the page being
 *          monitored.
 */
PGM_ALL_CB2_DECL(VBOXSTRICTRC)
patmVirtPageHandler(PVM pVM, PVMCPU pVCpu, RTGCPTR GCPtr, void *pvPtr, void *pvBuf, size_t cbBuf,
                    PGMACCESSTYPE enmAccessType, PGMACCESSORIGIN enmOrigin, void *pvUser)
{
    Assert(enmAccessType == PGMACCESSTYPE_WRITE); NOREF(enmAccessType);
    NOREF(pvPtr); NOREF(pvBuf); NOREF(cbBuf); NOREF(enmOrigin); NOREF(pvUser);
    RT_NOREF_PV(pVCpu);

    Assert(pvUser);
    Assert(!((uintptr_t)pvUser & PAGE_OFFSET_MASK));
    Assert(((uintptr_t)pvUser + (GCPtr & PAGE_OFFSET_MASK)) == GCPtr);

    pVM->patm.s.pvFaultMonitor = (RTRCPTR)GCPtr;
#ifdef IN_RING3
    PATMR3HandleMonitoredPage(pVM);
    return VINF_PGM_HANDLER_DO_DEFAULT;
#else
    /* RC: Go handle this in ring-3. */
    return VINF_PATM_CHECK_PATCH_PAGE;
#endif
}


/**
 * Load virtualized flags.
 *
 * This function is called from CPUMRawEnter(). It doesn't have to update the
 * IF and IOPL eflags bits, the caller will enforce those to set and 0 respectively.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pCtx        The cpu context.
 * @see     pg_raw
 */
VMM_INT_DECL(void) PATMRawEnter(PVM pVM, PCPUMCTX pCtx)
{
    Assert(!HMIsEnabled(pVM));

    /*
     * Currently we don't bother to check whether PATM is enabled or not.
     * For all cases where it isn't, IOPL will be safe and IF will be set.
     */
    uint32_t efl = pCtx->eflags.u32;
    CTXSUFF(pVM->patm.s.pGCState)->uVMFlags = efl & PATM_VIRTUAL_FLAGS_MASK;

    AssertMsg((efl & X86_EFL_IF) || PATMShouldUseRawMode(pVM, (RTRCPTR)pCtx->eip),
              ("X86_EFL_IF is clear and PATM is disabled! (eip=%RRv eflags=%08x fPATM=%d pPATMGC=%RRv-%RRv\n",
               pCtx->eip, pCtx->eflags.u32, PATMIsEnabled(pVM), pVM->patm.s.pPatchMemGC,
               pVM->patm.s.pPatchMemGC + pVM->patm.s.cbPatchMem));

    AssertReleaseMsg(CTXSUFF(pVM->patm.s.pGCState)->fPIF || PATMIsPatchGCAddr(pVM, pCtx->eip),
                     ("fPIF=%d eip=%RRv\n", pVM->patm.s.CTXSUFF(pGCState)->fPIF, pCtx->eip));

    efl &= ~PATM_VIRTUAL_FLAGS_MASK;
    efl |= X86_EFL_IF;
    pCtx->eflags.u32 = efl;

#ifdef IN_RING3
# ifdef PATM_EMULATE_SYSENTER
    PCPUMCTX pCtx;

    /* Check if the sysenter handler has changed. */
    pCtx = CPUMQueryGuestCtxPtr(pVM);
    if (   pCtx->SysEnter.cs  != 0
        && pCtx->SysEnter.eip != 0
       )
    {
        if (pVM->patm.s.pfnSysEnterGC != (RTRCPTR)pCtx->SysEnter.eip)
        {
            pVM->patm.s.pfnSysEnterPatchGC = 0;
            pVM->patm.s.pfnSysEnterGC = 0;

            Log2(("PATMRawEnter: installing sysenter patch for %RRv\n", pCtx->SysEnter.eip));
            pVM->patm.s.pfnSysEnterPatchGC = PATMR3QueryPatchGCPtr(pVM, pCtx->SysEnter.eip);
            if (pVM->patm.s.pfnSysEnterPatchGC == 0)
            {
                rc = PATMR3InstallPatch(pVM, pCtx->SysEnter.eip, PATMFL_SYSENTER | PATMFL_CODE32);
                if (rc == VINF_SUCCESS)
                {
                    pVM->patm.s.pfnSysEnterPatchGC  = PATMR3QueryPatchGCPtr(pVM, pCtx->SysEnter.eip);
                    pVM->patm.s.pfnSysEnterGC       = (RTRCPTR)pCtx->SysEnter.eip;
                    Assert(pVM->patm.s.pfnSysEnterPatchGC);
                }
            }
            else
                pVM->patm.s.pfnSysEnterGC = (RTRCPTR)pCtx->SysEnter.eip;
        }
    }
    else
    {
        pVM->patm.s.pfnSysEnterPatchGC = 0;
        pVM->patm.s.pfnSysEnterGC = 0;
    }
# endif /* PATM_EMULATE_SYSENTER */
#endif
}


/**
 * Restores virtualized flags.
 *
 * This function is called from CPUMRawLeave(). It will update the eflags register.
 *
 ** @note Only here we are allowed to switch back to guest code (without a special reason such as a trap in patch code)!!
 *
 * @param   pVM         The cross context VM structure.
 * @param   pCtx        The cpu context.
 * @param   rawRC       Raw mode return code
 * @see     @ref pg_raw
 */
VMM_INT_DECL(void) PATMRawLeave(PVM pVM, PCPUMCTX pCtx, int rawRC)
{
    Assert(!HMIsEnabled(pVM));
    bool fPatchCode = PATMIsPatchGCAddr(pVM, pCtx->eip);

    /*
     * We will only be called if PATMRawEnter was previously called.
     */
    uint32_t efl = pCtx->eflags.u32;
    efl = (efl & ~PATM_VIRTUAL_FLAGS_MASK) | (CTXSUFF(pVM->patm.s.pGCState)->uVMFlags & PATM_VIRTUAL_FLAGS_MASK);
    pCtx->eflags.u32 = efl;
    CTXSUFF(pVM->patm.s.pGCState)->uVMFlags = X86_EFL_IF;

#ifdef IN_RING3
    AssertReleaseMsg((efl & X86_EFL_IF) || fPatchCode   || rawRC == VINF_PATM_PENDING_IRQ_AFTER_IRET
                     || rawRC == VINF_EM_RESCHEDULE     || rawRC == VINF_EM_RESCHEDULE_REM
                     || rawRC == VINF_EM_RAW_GUEST_TRAP || RT_FAILURE(rawRC),
                     ("Inconsistent state at %RRv rc=%Rrc\n", pCtx->eip, rawRC));
    AssertReleaseMsg(CTXSUFF(pVM->patm.s.pGCState)->fPIF || fPatchCode || RT_FAILURE(rawRC), ("fPIF=%d eip=%RRv rc=%Rrc\n", CTXSUFF(pVM->patm.s.pGCState)->fPIF, pCtx->eip, rawRC));
    if (   (efl & X86_EFL_IF)
        && fPatchCode)
    {
        if (   rawRC < VINF_PATM_LEAVE_RC_FIRST
            || rawRC > VINF_PATM_LEAVE_RC_LAST)
        {
            /*
             * Golden rules:
             * - Don't interrupt special patch streams that replace special instructions
             * - Don't break instruction fusing (sti, pop ss, mov ss)
             * - Don't go back to an instruction that has been overwritten by a patch jump
             * - Don't interrupt an idt handler on entry (1st instruction); technically incorrect
             *
             */
            if (CTXSUFF(pVM->patm.s.pGCState)->fPIF == 1)            /* consistent patch instruction state */
            {
                PATMTRANSSTATE  enmState;
                RTRCPTR         pOrgInstrGC = PATMR3PatchToGCPtr(pVM, pCtx->eip, &enmState);

                AssertRelease(pOrgInstrGC);

                Assert(enmState != PATMTRANS_OVERWRITTEN);
                if (enmState == PATMTRANS_SAFE)
                {
                    Assert(!patmFindActivePatchByEntrypoint(pVM, pOrgInstrGC));
                    Log(("Switchback from %RRv to %RRv (Psp=%x)\n", pCtx->eip, pOrgInstrGC, CTXSUFF(pVM->patm.s.pGCState)->Psp));
                    STAM_COUNTER_INC(&pVM->patm.s.StatSwitchBack);
                    pCtx->eip = pOrgInstrGC;
                    fPatchCode = false; /* to reset the stack ptr */

                    CTXSUFF(pVM->patm.s.pGCState)->GCPtrInhibitInterrupts = 0;   /* reset this pointer; safe otherwise the state would be PATMTRANS_INHIBITIRQ */
                }
                else
                {
                    LogFlow(("Patch address %RRv can't be interrupted (state=%d)!\n",  pCtx->eip, enmState));
                    STAM_COUNTER_INC(&pVM->patm.s.StatSwitchBackFail);
                }
            }
            else
            {
                LogFlow(("Patch address %RRv can't be interrupted (fPIF=%d)!\n",  pCtx->eip, CTXSUFF(pVM->patm.s.pGCState)->fPIF));
                STAM_COUNTER_INC(&pVM->patm.s.StatSwitchBackFail);
            }
        }
    }
#else  /* !IN_RING3 */
    /*
     * When leaving raw-mode state while IN_RC, it's generally for interpreting
     * a single original guest instruction.
     */
    AssertMsg(!fPatchCode, ("eip=%RRv\n", pCtx->eip));
    AssertReleaseMsg((efl & X86_EFL_IF) || fPatchCode || rawRC == VINF_PATM_PENDING_IRQ_AFTER_IRET || RT_FAILURE(rawRC), ("Inconsistent state at %RRv rc=%Rrc\n", pCtx->eip, rawRC));
    AssertReleaseMsg(CTXSUFF(pVM->patm.s.pGCState)->fPIF || fPatchCode || RT_FAILURE(rawRC), ("fPIF=%d eip=%RRv rc=%Rrc\n", CTXSUFF(pVM->patm.s.pGCState)->fPIF, pCtx->eip, rawRC));
#endif /* !IN_RING3 */

    if (!fPatchCode)
    {
        if (CTXSUFF(pVM->patm.s.pGCState)->GCPtrInhibitInterrupts == (RTRCPTR)pCtx->eip)
        {
            EMSetInhibitInterruptsPC(VMMGetCpu0(pVM), pCtx->eip);
        }
        CTXSUFF(pVM->patm.s.pGCState)->GCPtrInhibitInterrupts = 0;

        /* Reset the stack pointer to the top of the stack. */
#ifdef DEBUG
        if (CTXSUFF(pVM->patm.s.pGCState)->Psp != PATM_STACK_SIZE)
        {
            LogFlow(("PATMRawLeave: Reset PATM stack (Psp = %x)\n", CTXSUFF(pVM->patm.s.pGCState)->Psp));
        }
#endif
        CTXSUFF(pVM->patm.s.pGCState)->Psp = PATM_STACK_SIZE;
    }
}

/**
 * Get the EFLAGS.
 * This is a worker for CPUMRawGetEFlags().
 *
 * @returns The eflags.
 * @param   pVM         The cross context VM structure.
 * @param   pCtx        The guest cpu context.
 */
VMM_INT_DECL(uint32_t) PATMRawGetEFlags(PVM pVM, PCCPUMCTX pCtx)
{
    Assert(!HMIsEnabled(pVM));
    uint32_t efl = pCtx->eflags.u32;
    efl &= ~PATM_VIRTUAL_FLAGS_MASK;
    efl |= pVM->patm.s.CTXSUFF(pGCState)->uVMFlags & PATM_VIRTUAL_FLAGS_MASK;
    return efl;
}

/**
 * Updates the EFLAGS.
 * This is a worker for CPUMRawSetEFlags().
 *
 * @param   pVM         The cross context VM structure.
 * @param   pCtx        The guest cpu context.
 * @param   efl         The new EFLAGS value.
 */
VMM_INT_DECL(void) PATMRawSetEFlags(PVM pVM, PCPUMCTX pCtx, uint32_t efl)
{
    Assert(!HMIsEnabled(pVM));
    pVM->patm.s.CTXSUFF(pGCState)->uVMFlags = efl & PATM_VIRTUAL_FLAGS_MASK;
    efl &= ~PATM_VIRTUAL_FLAGS_MASK;
    efl |= X86_EFL_IF;
    pCtx->eflags.u32 = efl;
}

/**
 * Check if we must use raw mode (patch code being executed)
 *
 * @param   pVM         The cross context VM structure.
 * @param   pAddrGC     Guest context address
 */
VMM_INT_DECL(bool) PATMShouldUseRawMode(PVM pVM, RTRCPTR pAddrGC)
{
    return   PATMIsEnabled(pVM)
          && (   (RTRCUINTPTR)pAddrGC - (RTRCUINTPTR)pVM->patm.s.pPatchMemGC      < pVM->patm.s.cbPatchMem
              || (RTRCUINTPTR)pAddrGC - (RTRCUINTPTR)pVM->patm.s.pbPatchHelpersRC < pVM->patm.s.cbPatchHelpers);
}

/**
 * Returns the guest context pointer and size of the GC context structure
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMM_INT_DECL(RCPTRTYPE(PPATMGCSTATE)) PATMGetGCState(PVM pVM)
{
    AssertReturn(!HMIsEnabled(pVM), NIL_RTRCPTR);
    return pVM->patm.s.pGCStateGC;
}

/**
 * Checks whether the GC address is part of our patch or helper regions.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   uGCAddr     Guest context address.
 * @internal
 */
VMMDECL(bool) PATMIsPatchGCAddr(PVM pVM, RTRCUINTPTR uGCAddr)
{
    return PATMIsEnabled(pVM)
        && (   uGCAddr - (RTRCUINTPTR)pVM->patm.s.pPatchMemGC      < pVM->patm.s.cbPatchMem
            || uGCAddr - (RTRCUINTPTR)pVM->patm.s.pbPatchHelpersRC < pVM->patm.s.cbPatchHelpers);
}

/**
 * Checks whether the GC address is part of our patch region.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   uGCAddr     Guest context address.
 * @internal
 */
VMMDECL(bool) PATMIsPatchGCAddrExclHelpers(PVM pVM, RTRCUINTPTR uGCAddr)
{
    return PATMIsEnabled(pVM)
        && uGCAddr - (RTRCUINTPTR)pVM->patm.s.pPatchMemGC < pVM->patm.s.cbPatchMem;
}

/**
 * Reads patch code.
 *
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_PATCH_NOT_FOUND if the request is entirely outside the patch
 *          code.
 *
 * @param   pVM            The cross context VM structure.
 * @param   GCPtrPatchCode  The patch address to start reading at.
 * @param   pvDst           Where to return the patch code.
 * @param   cbToRead        Number of bytes to read.
 * @param   pcbRead         Where to return the actual number of bytes we've
 *                          read. Optional.
 */
VMM_INT_DECL(int) PATMReadPatchCode(PVM pVM, RTGCPTR GCPtrPatchCode, void *pvDst, size_t cbToRead, size_t *pcbRead)
{
    /* Shortcut. */
    if (!PATMIsEnabled(pVM))
        return VERR_PATCH_NOT_FOUND;
    Assert(!HMIsEnabled(pVM));

    /*
     * Check patch code and patch helper code.  We assume the requested bytes
     * are not in either.
     */
    RTGCPTR offPatchCode = GCPtrPatchCode - (RTGCPTR32)pVM->patm.s.pPatchMemGC;
    if (offPatchCode >= pVM->patm.s.cbPatchMem)
    {
        offPatchCode = GCPtrPatchCode - (RTGCPTR32)pVM->patm.s.pbPatchHelpersRC;
        if (offPatchCode >= pVM->patm.s.cbPatchHelpers)
            return VERR_PATCH_NOT_FOUND;

        /*
         * Patch helper memory.
         */
        uint32_t cbMaxRead = pVM->patm.s.cbPatchHelpers - (uint32_t)offPatchCode;
        if (cbToRead > cbMaxRead)
            cbToRead = cbMaxRead;
#ifdef IN_RC
        memcpy(pvDst, pVM->patm.s.pbPatchHelpersRC + (uint32_t)offPatchCode, cbToRead);
#else
        memcpy(pvDst, pVM->patm.s.pbPatchHelpersR3 + (uint32_t)offPatchCode, cbToRead);
#endif
    }
    else
    {
        /*
         * Patch memory.
         */
        uint32_t cbMaxRead = pVM->patm.s.cbPatchMem - (uint32_t)offPatchCode;
        if (cbToRead > cbMaxRead)
            cbToRead = cbMaxRead;
#ifdef IN_RC
        memcpy(pvDst, pVM->patm.s.pPatchMemGC + (uint32_t)offPatchCode, cbToRead);
#else
        memcpy(pvDst, pVM->patm.s.pPatchMemHC + (uint32_t)offPatchCode, cbToRead);
#endif
    }

    if (pcbRead)
        *pcbRead = cbToRead;
    return VINF_SUCCESS;
}

/**
 * Set parameters for pending MMIO patch operation
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   GCPhys      MMIO physical address.
 * @param   pCachedData RC pointer to cached data.
 */
VMM_INT_DECL(int) PATMSetMMIOPatchInfo(PVM pVM, RTGCPHYS GCPhys, RTRCPTR pCachedData)
{
    if (!HMIsEnabled(pVM))
    {
        pVM->patm.s.mmio.GCPhys = GCPhys;
        pVM->patm.s.mmio.pCachedData = (RTRCPTR)pCachedData;
    }

    return VINF_SUCCESS;
}

/**
 * Checks if the interrupt flag is enabled or not.
 *
 * @returns true if it's enabled.
 * @returns false if it's disabled.
 *
 * @param   pVM         The cross context VM structure.
 * @todo CPUM should wrap this, EM.cpp shouldn't call us.
 */
VMM_INT_DECL(bool) PATMAreInterruptsEnabled(PVM pVM)
{
    PCPUMCTX pCtx = CPUMQueryGuestCtxPtr(VMMGetCpu(pVM));

    return PATMAreInterruptsEnabledByCtx(pVM, pCtx);
}

/**
 * Checks if the interrupt flag is enabled or not.
 *
 * @returns true if it's enabled.
 * @returns false if it's disabled.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pCtx        The guest CPU context.
 * @todo CPUM should wrap this, EM.cpp shouldn't call us.
 */
VMM_INT_DECL(bool) PATMAreInterruptsEnabledByCtx(PVM pVM, PCPUMCTX pCtx)
{
    if (PATMIsEnabled(pVM))
    {
        Assert(!HMIsEnabled(pVM));
        if (PATMIsPatchGCAddr(pVM, pCtx->eip))
            return false;
    }
    return !!(pCtx->eflags.u32 & X86_EFL_IF);
}

/**
 * Check if the instruction is patched as a duplicated function
 *
 * @returns patch record
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context point to the instruction
 *
 */
PPATMPATCHREC patmQueryFunctionPatch(PVM pVM, RTRCPTR pInstrGC)
{
    PPATMPATCHREC pRec;

    AssertCompile(sizeof(AVLOU32KEY) == sizeof(pInstrGC));
    pRec = (PPATMPATCHREC)RTAvloU32Get(&CTXSUFF(pVM->patm.s.PatchLookupTree)->PatchTree, (AVLOU32KEY)pInstrGC);
    if (    pRec
        && (pRec->patch.uState == PATCH_ENABLED)
        && (pRec->patch.flags & (PATMFL_DUPLICATE_FUNCTION|PATMFL_CALLABLE_AS_FUNCTION))
       )
        return pRec;
    return 0;
}

/**
 * Checks if the int 3 was caused by a patched instruction
 *
 * @returns VBox status
 *
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Instruction pointer
 * @param   pOpcode     Original instruction opcode (out, optional)
 * @param   pSize       Original instruction size (out, optional)
 */
VMM_INT_DECL(bool) PATMIsInt3Patch(PVM pVM, RTRCPTR pInstrGC, uint32_t *pOpcode, uint32_t *pSize)
{
    PPATMPATCHREC pRec;
    Assert(!HMIsEnabled(pVM));

    pRec = (PPATMPATCHREC)RTAvloU32Get(&CTXSUFF(pVM->patm.s.PatchLookupTree)->PatchTree, (AVLOU32KEY)pInstrGC);
    if (    pRec
        && (pRec->patch.uState == PATCH_ENABLED)
        && (pRec->patch.flags & (PATMFL_INT3_REPLACEMENT|PATMFL_INT3_REPLACEMENT_BLOCK))
       )
    {
        if (pOpcode) *pOpcode = pRec->patch.opcode;
        if (pSize)   *pSize   = pRec->patch.cbPrivInstr;
        return true;
    }
    return false;
}

/**
 * Emulate sysenter, sysexit and syscall instructions
 *
 * @returns VBox status
 *
 * @param   pVM         The cross context VM structure.
 * @param   pCtx        The relevant guest cpu context.
 * @param   pCpu        Disassembly state.
 */
VMMDECL(int) PATMSysCall(PVM pVM, PCPUMCTX pCtx, PDISCPUSTATE pCpu)
{
    Assert(CPUMQueryGuestCtxPtr(VMMGetCpu0(pVM)) == pCtx);
    AssertReturn(!HMIsEnabled(pVM), VERR_PATM_HM_IPE);

    if (pCpu->pCurInstr->uOpcode == OP_SYSENTER)
    {
        if (    pCtx->SysEnter.cs == 0
            ||  pCtx->eflags.Bits.u1VM
            ||  (pCtx->cs.Sel & X86_SEL_RPL) != 3
            ||  pVM->patm.s.pfnSysEnterPatchGC == 0
            ||  pVM->patm.s.pfnSysEnterGC != (RTRCPTR)(RTRCUINTPTR)pCtx->SysEnter.eip
            ||  !(PATMRawGetEFlags(pVM, pCtx) & X86_EFL_IF))
            goto end;

        Log2(("PATMSysCall: sysenter from %RRv to %RRv\n", pCtx->eip, pVM->patm.s.pfnSysEnterPatchGC));
        /** @todo the base and limit are forced to 0 & 4G-1 resp. We assume the selector is wide open here. */
        /** @note The Intel manual suggests that the OS is responsible for this. */
        pCtx->cs.Sel      = (pCtx->SysEnter.cs & ~X86_SEL_RPL) | 1;
        pCtx->eip         = /** @todo ugly conversion! */(uint32_t)pVM->patm.s.pfnSysEnterPatchGC;
        pCtx->ss.Sel      = pCtx->cs.Sel + 8;     /* SysEnter.cs + 8 */
        pCtx->esp         = pCtx->SysEnter.esp;
        pCtx->eflags.u32 &= ~(X86_EFL_VM | X86_EFL_RF);
        pCtx->eflags.u32 |= X86_EFL_IF;

        /* Turn off interrupts. */
        pVM->patm.s.CTXSUFF(pGCState)->uVMFlags &= ~X86_EFL_IF;

        STAM_COUNTER_INC(&pVM->patm.s.StatSysEnter);

        return VINF_SUCCESS;
    }
    if (pCpu->pCurInstr->uOpcode == OP_SYSEXIT)
    {
        if (    pCtx->SysEnter.cs == 0
            ||  (pCtx->cs.Sel & X86_SEL_RPL) != 1
            ||  pCtx->eflags.Bits.u1VM
            ||  !(PATMRawGetEFlags(pVM, pCtx) & X86_EFL_IF))
            goto end;

        Log2(("PATMSysCall: sysexit from %RRv to %RRv\n", pCtx->eip, pCtx->edx));

        pCtx->cs.Sel      = ((pCtx->SysEnter.cs + 16) & ~X86_SEL_RPL) | 3;
        pCtx->eip         = pCtx->edx;
        pCtx->ss.Sel      = pCtx->cs.Sel + 8;  /* SysEnter.cs + 24 */
        pCtx->esp         = pCtx->ecx;

        STAM_COUNTER_INC(&pVM->patm.s.StatSysExit);

        return VINF_SUCCESS;
    }
    if (pCpu->pCurInstr->uOpcode == OP_SYSCALL)
    {
        /** @todo implement syscall */
    }
    else
    if (pCpu->pCurInstr->uOpcode == OP_SYSRET)
    {
        /** @todo implement sysret */
    }

end:
    return VINF_EM_RAW_RING_SWITCH;
}

/**
 * Adds branch pair to the lookup cache of the particular branch instruction
 *
 * @returns VBox status
 * @param   pVM                 The cross context VM structure.
 * @param   pJumpTableGC        Pointer to branch instruction lookup cache
 * @param   pBranchTarget       Original branch target
 * @param   pRelBranchPatch     Relative duplicated function address
 */
int patmAddBranchToLookupCache(PVM pVM, RTRCPTR pJumpTableGC, RTRCPTR pBranchTarget, RTRCUINTPTR pRelBranchPatch)
{
    PPATCHJUMPTABLE pJumpTable;

    Log(("PATMAddBranchToLookupCache: Adding (%RRv->%RRv (%RRv)) to table %RRv\n", pBranchTarget, pRelBranchPatch + pVM->patm.s.pPatchMemGC, pRelBranchPatch, pJumpTableGC));

    AssertReturn(PATMIsPatchGCAddr(pVM, (RTRCUINTPTR)pJumpTableGC), VERR_INVALID_PARAMETER);

#ifdef IN_RC
    pJumpTable = (PPATCHJUMPTABLE) pJumpTableGC;
#else
    pJumpTable = (PPATCHJUMPTABLE) (pJumpTableGC - pVM->patm.s.pPatchMemGC + pVM->patm.s.pPatchMemHC);
#endif
    Log(("Nr addresses = %d, insert pos = %d\n", pJumpTable->cAddresses, pJumpTable->ulInsertPos));
    if (pJumpTable->cAddresses < pJumpTable->nrSlots)
    {
        uint32_t i;

        for (i=0;i<pJumpTable->nrSlots;i++)
        {
            if (pJumpTable->Slot[i].pInstrGC == 0)
            {
                pJumpTable->Slot[i].pInstrGC    = pBranchTarget;
                /* Relative address - eases relocation */
                pJumpTable->Slot[i].pRelPatchGC = pRelBranchPatch;
                pJumpTable->cAddresses++;
                break;
            }
        }
        AssertReturn(i < pJumpTable->nrSlots, VERR_INTERNAL_ERROR);
#ifdef VBOX_WITH_STATISTICS
        STAM_COUNTER_INC(&pVM->patm.s.StatFunctionLookupInsert);
        if (pVM->patm.s.StatU32FunctionMaxSlotsUsed < i)
            pVM->patm.s.StatU32FunctionMaxSlotsUsed = i + 1;
#endif
    }
    else
    {
        /* Replace an old entry. */
        /** @todo replacement strategy isn't really bright. change to something better if required. */
        Assert(pJumpTable->ulInsertPos < pJumpTable->nrSlots);
        Assert((pJumpTable->nrSlots & 1) == 0);

        pJumpTable->ulInsertPos &= (pJumpTable->nrSlots-1);
        pJumpTable->Slot[pJumpTable->ulInsertPos].pInstrGC    = pBranchTarget;
        /* Relative address - eases relocation */
        pJumpTable->Slot[pJumpTable->ulInsertPos].pRelPatchGC = pRelBranchPatch;

        pJumpTable->ulInsertPos = (pJumpTable->ulInsertPos+1) & (pJumpTable->nrSlots-1);

        STAM_COUNTER_INC(&pVM->patm.s.StatFunctionLookupReplace);
    }

    return VINF_SUCCESS;
}


#if defined(VBOX_WITH_STATISTICS) || defined(LOG_ENABLED)
/**
 * Return the name of the patched instruction
 *
 * @returns instruction name
 *
 * @param   opcode      DIS instruction opcode
 * @param   fPatchFlags Patch flags
 */
const char *patmGetInstructionString(uint32_t opcode, uint32_t fPatchFlags)
{
    const char *pszInstr = NULL;

    switch (opcode)
    {
    case OP_CLI:
        pszInstr = "cli";
        break;
    case OP_PUSHF:
        pszInstr = "pushf";
        break;
    case OP_POPF:
        pszInstr = "popf";
        break;
    case OP_STR:
        pszInstr = "str";
        break;
    case OP_LSL:
        pszInstr = "lsl";
        break;
    case OP_LAR:
        pszInstr = "lar";
        break;
    case OP_SGDT:
        pszInstr = "sgdt";
        break;
    case OP_SLDT:
        pszInstr = "sldt";
        break;
    case OP_SIDT:
        pszInstr = "sidt";
        break;
    case OP_SMSW:
        pszInstr = "smsw";
        break;
    case OP_VERW:
        pszInstr = "verw";
        break;
    case OP_VERR:
        pszInstr = "verr";
        break;
    case OP_CPUID:
        pszInstr = "cpuid";
        break;
    case OP_JMP:
        pszInstr = "jmp";
        break;
    case OP_JO:
        pszInstr = "jo";
        break;
    case OP_JNO:
        pszInstr = "jno";
        break;
    case OP_JC:
        pszInstr = "jc";
        break;
    case OP_JNC:
        pszInstr = "jnc";
        break;
    case OP_JE:
        pszInstr = "je";
        break;
    case OP_JNE:
        pszInstr = "jne";
        break;
    case OP_JBE:
        pszInstr = "jbe";
        break;
    case OP_JNBE:
        pszInstr = "jnbe";
        break;
    case OP_JS:
        pszInstr = "js";
        break;
    case OP_JNS:
        pszInstr = "jns";
        break;
    case OP_JP:
        pszInstr = "jp";
        break;
    case OP_JNP:
        pszInstr = "jnp";
        break;
    case OP_JL:
        pszInstr = "jl";
        break;
    case OP_JNL:
        pszInstr = "jnl";
        break;
    case OP_JLE:
        pszInstr = "jle";
        break;
    case OP_JNLE:
        pszInstr = "jnle";
        break;
    case OP_JECXZ:
        pszInstr = "jecxz";
        break;
    case OP_LOOP:
        pszInstr = "loop";
        break;
    case OP_LOOPNE:
        pszInstr = "loopne";
        break;
    case OP_LOOPE:
        pszInstr = "loope";
        break;
    case OP_MOV:
        if (fPatchFlags & PATMFL_IDTHANDLER)
            pszInstr = "mov (Int/Trap Handler)";
        else
            pszInstr = "mov (cs)";
        break;
    case OP_SYSENTER:
        pszInstr = "sysenter";
        break;
    case OP_PUSH:
        pszInstr = "push (cs)";
        break;
    case OP_CALL:
        pszInstr = "call";
        break;
    case OP_IRET:
        pszInstr = "iret";
        break;
    }
    return pszInstr;
}
#endif
