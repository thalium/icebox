/* $Id: IEMAllCImplSvmInstr.cpp.h $ */
/** @file
 * IEM - AMD-V (Secure Virtual Machine) instruction implementation.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/**
 * Converts an IEM exception event type to an SVM event type.
 *
 * @returns The SVM event type.
 * @retval  UINT8_MAX if the specified type of event isn't among the set
 *          of recognized IEM event types.
 *
 * @param   uVector         The vector of the event.
 * @param   fIemXcptFlags   The IEM exception / interrupt flags.
 */
IEM_STATIC uint8_t iemGetSvmEventType(uint32_t uVector, uint32_t fIemXcptFlags)
{
    if (fIemXcptFlags & IEM_XCPT_FLAGS_T_CPU_XCPT)
    {
        if (uVector != X86_XCPT_NMI)
            return SVM_EVENT_EXCEPTION;
        return SVM_EVENT_NMI;
    }

    /* See AMD spec. Table 15-1. "Guest Exception or Interrupt Types". */
    if (fIemXcptFlags & (IEM_XCPT_FLAGS_BP_INSTR | IEM_XCPT_FLAGS_ICEBP_INSTR | IEM_XCPT_FLAGS_OF_INSTR))
        return SVM_EVENT_EXCEPTION;

    if (fIemXcptFlags & IEM_XCPT_FLAGS_T_EXT_INT)
        return SVM_EVENT_EXTERNAL_IRQ;

    if (fIemXcptFlags & IEM_XCPT_FLAGS_T_SOFT_INT)
        return SVM_EVENT_SOFTWARE_INT;

    AssertMsgFailed(("iemGetSvmEventType: Invalid IEM xcpt/int. type %#x, uVector=%#x\n", fIemXcptFlags, uVector));
    return UINT8_MAX;
}


/**
 * Performs an SVM world-switch (VMRUN, \#VMEXIT) updating PGM and IEM internals.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        The guest-CPU context.
 */
DECLINLINE(VBOXSTRICTRC) iemSvmWorldSwitch(PVMCPU pVCpu, PCPUMCTX pCtx)
{
    /*
     * Flush the TLB with new CR3. This is required in case the PGM mode change
     * below doesn't actually change anything.
     */
    PGMFlushTLB(pVCpu, pCtx->cr3, true);

    /*
     * Inform PGM about paging mode changes.
     * We include X86_CR0_PE because PGM doesn't handle paged-real mode yet,
     * see comment in iemMemPageTranslateAndCheckAccess().
     */
    int rc = PGMChangeMode(pVCpu, pCtx->cr0 | X86_CR0_PE, pCtx->cr4, pCtx->msrEFER);
#ifdef IN_RING3
    Assert(rc != VINF_PGM_CHANGE_MODE);
#endif
    AssertRCReturn(rc, rc);

    /* Inform CPUM (recompiler), can later be removed. */
    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_ALL);

    /* Re-initialize IEM cache/state after the drastic mode switch. */
    iemReInitExec(pVCpu);
    return rc;
}


/**
 * SVM \#VMEXIT handler.
 *
 * @returns Strict VBox status code.
 * @retval VINF_SVM_VMEXIT when the \#VMEXIT is successful.
 * @retval VERR_SVM_VMEXIT_FAILED when the \#VMEXIT failed restoring the guest's
 *         "host state" and a shutdown is required.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        The guest-CPU context.
 * @param   uExitCode   The exit code.
 * @param   uExitInfo1  The exit info. 1 field.
 * @param   uExitInfo2  The exit info. 2 field.
 */
IEM_STATIC VBOXSTRICTRC iemSvmVmexit(PVMCPU pVCpu, PCPUMCTX pCtx, uint64_t uExitCode, uint64_t uExitInfo1, uint64_t uExitInfo2)
{
    if (   CPUMIsGuestInSvmNestedHwVirtMode(pCtx)
        || uExitCode == SVM_EXIT_INVALID)
    {
        LogFlow(("iemSvmVmexit: CS:RIP=%04x:%08RX64 uExitCode=%#RX64 uExitInfo1=%#RX64 uExitInfo2=%#RX64\n", pCtx->cs.Sel,
                 pCtx->rip, uExitCode, uExitInfo1, uExitInfo2));

        /*
         * Disable the global interrupt flag to prevent interrupts during the 'atomic' world switch.
         */
        pCtx->hwvirt.svm.fGif = false;

        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->es));
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->cs));
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->ss));
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->ds));

        /*
         * Save the nested-guest state into the VMCB state-save area.
         */
        PSVMVMCB           pVmcbNstGst      = pCtx->hwvirt.svm.CTX_SUFF(pVmcb);
        PSVMVMCBCTRL       pVmcbNstGstCtrl  = &pVmcbNstGst->ctrl;
        PSVMVMCBSTATESAVE  pVmcbNstGstState = &pVmcbNstGst->guest;

        HMSVM_SEG_REG_COPY_TO_VMCB(pCtx, pVmcbNstGstState, ES, es);
        HMSVM_SEG_REG_COPY_TO_VMCB(pCtx, pVmcbNstGstState, CS, cs);
        HMSVM_SEG_REG_COPY_TO_VMCB(pCtx, pVmcbNstGstState, SS, ss);
        HMSVM_SEG_REG_COPY_TO_VMCB(pCtx, pVmcbNstGstState, DS, ds);
        pVmcbNstGstState->GDTR.u32Limit = pCtx->gdtr.cbGdt;
        pVmcbNstGstState->GDTR.u64Base  = pCtx->gdtr.pGdt;
        pVmcbNstGstState->IDTR.u32Limit = pCtx->idtr.cbIdt;
        pVmcbNstGstState->IDTR.u64Base  = pCtx->idtr.pIdt;
        pVmcbNstGstState->u64EFER       = pCtx->msrEFER;
        pVmcbNstGstState->u64CR4        = pCtx->cr4;
        pVmcbNstGstState->u64CR3        = pCtx->cr3;
        pVmcbNstGstState->u64CR2        = pCtx->cr2;
        pVmcbNstGstState->u64CR0        = pCtx->cr0;
        /** @todo Nested paging. */
        pVmcbNstGstState->u64RFlags     = pCtx->rflags.u64;
        pVmcbNstGstState->u64RIP        = pCtx->rip;
        pVmcbNstGstState->u64RSP        = pCtx->rsp;
        pVmcbNstGstState->u64RAX        = pCtx->rax;
        pVmcbNstGstState->u64DR7        = pCtx->dr[7];
        pVmcbNstGstState->u64DR6        = pCtx->dr[6];
        pVmcbNstGstState->u8CPL         = pCtx->ss.Attr.n.u2Dpl;   /* See comment in CPUMGetGuestCPL(). */
        Assert(CPUMGetGuestCPL(pVCpu) == pCtx->ss.Attr.n.u2Dpl);

        PSVMVMCBCTRL pVmcbCtrl = &pCtx->hwvirt.svm.CTX_SUFF(pVmcb)->ctrl;
        /* Save interrupt shadow of the nested-guest instruction if any. */
        if (   VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS)
            && EMGetInhibitInterruptsPC(pVCpu) == pCtx->rip)
        {
            LogFlow(("iemSvmVmexit: Interrupt shadow till %#RX64\n", pCtx->rip));
            pVmcbCtrl->u64IntShadow |= SVM_INTERRUPT_SHADOW_ACTIVE;
        }

        /*
         * Save additional state and intercept information.
         */
        if (VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_INTERRUPT_NESTED_GUEST))
        {
            Assert(pVmcbCtrl->IntCtrl.n.u1VIrqPending);
            VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INTERRUPT_NESTED_GUEST);
        }
        else
            pVmcbCtrl->IntCtrl.n.u1VIrqPending = 0;

        /** @todo Save V_TPR, V_IRQ. */
        /** @todo NRIP. */

        /* Save exit information. */
        pVmcbCtrl->u64ExitCode  = uExitCode;
        pVmcbCtrl->u64ExitInfo1 = uExitInfo1;
        pVmcbCtrl->u64ExitInfo2 = uExitInfo2;

        /*
         * Update the exit interrupt information field if this #VMEXIT happened as a result
         * of delivering an event.
         */
        {
            uint8_t  uExitIntVector;
            uint32_t uExitIntErr;
            uint32_t fExitIntFlags;
            bool const fRaisingEvent = IEMGetCurrentXcpt(pVCpu, &uExitIntVector, &fExitIntFlags, &uExitIntErr,
                                                         NULL /* uExitIntCr2 */);
            pVmcbCtrl->ExitIntInfo.n.u1Valid = fRaisingEvent;
            if (fRaisingEvent)
            {
                pVmcbCtrl->ExitIntInfo.n.u8Vector = uExitIntVector;
                pVmcbCtrl->ExitIntInfo.n.u3Type   = iemGetSvmEventType(uExitIntVector, fExitIntFlags);
                if (fExitIntFlags & IEM_XCPT_FLAGS_ERR)
                {
                    pVmcbCtrl->ExitIntInfo.n.u1ErrorCodeValid = true;
                    pVmcbCtrl->ExitIntInfo.n.u32ErrorCode     = uExitIntErr;
                }
            }
        }

        /*
         * Clear event injection in the VMCB.
         */
        pVmcbCtrl->EventInject.n.u1Valid = 0;

        /*
         * Notify HM in case the nested-guest was executed using hardware-assisted SVM (which
         * would have modified some VMCB state) that need to be restored on #VMEXIT before
         * writing the VMCB back to guest memory.
         */
        HMSvmNstGstVmExitNotify(pVCpu, pCtx);

        /*
         * Write back the nested-guest's VMCB to its guest physical memory location.
         */
        VBOXSTRICTRC rcStrict = PGMPhysSimpleWriteGCPhys(pVCpu->CTX_SUFF(pVM), pCtx->hwvirt.svm.GCPhysVmcb, pVmcbNstGst,
                                                         sizeof(*pVmcbNstGst));
        /*
         * Prepare for guest's "host mode" by clearing internal processor state bits.
         *
         * We don't need to zero out the state-save area, just the controls should be
         * sufficient because it has the critical bit of indicating whether we're inside
         * the nested-guest or not.
         */
        memset(pVmcbNstGstCtrl, 0, sizeof(*pVmcbNstGstCtrl));
        Assert(!CPUMIsGuestInSvmNestedHwVirtMode(pCtx));

        /*
         * Restore the subset of force-flags that were preserved.
         */
        if (pCtx->hwvirt.fLocalForcedActions)
        {
            VMCPU_FF_SET(pVCpu, pCtx->hwvirt.fLocalForcedActions);
            pCtx->hwvirt.fLocalForcedActions = 0;
        }

        if (RT_SUCCESS(rcStrict))
        {
            /** @todo Nested paging. */
            /** @todo ASID. */

            /*
             * Reload the guest's "host state".
             */
            CPUMSvmVmExitRestoreHostState(pVCpu, pCtx);

            /*
             * Update PGM, IEM and others of a world-switch.
             */
            rcStrict = iemSvmWorldSwitch(pVCpu, pCtx);
            if (rcStrict == VINF_SUCCESS)
                return VINF_SVM_VMEXIT;

            if (RT_SUCCESS(rcStrict))
            {
                LogFlow(("iemSvmVmexit: Setting passup status from iemSvmWorldSwitch %Rrc\n", rcStrict));
                iemSetPassUpStatus(pVCpu, rcStrict);
                return VINF_SVM_VMEXIT;
            }

            LogFlow(("iemSvmVmexit: iemSvmWorldSwitch unexpected failure. rc=%Rrc\n", VBOXSTRICTRC_VAL(rcStrict)));
        }
        else
            LogFlow(("iemSvmVmexit: Writing VMCB at %#RGp failed. rc=%Rrc\n", pCtx->hwvirt.svm.GCPhysVmcb,
                     VBOXSTRICTRC_VAL(rcStrict)));

        return VERR_SVM_VMEXIT_FAILED;
    }

    Log(("iemSvmVmexit: Not in SVM guest mode! uExitCode=%#RX64 uExitInfo1=%#RX64 uExitInfo2=%#RX64\n", uExitCode,
         uExitInfo1, uExitInfo2));
    AssertMsgFailed(("iemSvmVmexit: Unexpected SVM-exit failure uExitCode=%#RX64\n", uExitCode));
    return VERR_SVM_IPE_5;
}


/**
 * Performs the operations necessary that are part of the vmrun instruction
 * execution in the guest.
 *
 * @returns Strict VBox status code (i.e. informational status codes too).
 * @retval  VINF_SUCCESS successully executed VMRUN and entered nested-guest
 *          code execution.
 * @retval  VINF_SVM_VMEXIT when executing VMRUN causes a \#VMEXIT
 *          (SVM_EXIT_INVALID most likely).
 *
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   pCtx                Pointer to the guest-CPU context.
 * @param   cbInstr             The length of the VMRUN instruction.
 * @param   GCPhysVmcb          Guest physical address of the VMCB to run.
 */
IEM_STATIC VBOXSTRICTRC iemSvmVmrun(PVMCPU pVCpu, PCPUMCTX pCtx, uint8_t cbInstr, RTGCPHYS GCPhysVmcb)
{
    LogFlow(("iemSvmVmrun\n"));

#ifdef IN_RING0
    /*
     * Until PGM can handle switching the guest paging mode in ring-0,
     * there's no point in trying to emulate VMRUN in ring-0 as we have
     * to go back to ring-3 anyway, see @bugref{7243#c48}.
     */
    RT_NOREF(pVCpu, pCtx, cbInstr, GCPhysVmcb);
    return VERR_IEM_ASPECT_NOT_IMPLEMENTED;
#else

    /*
     * Cache the physical address of the VMCB for #VMEXIT exceptions.
     */
    pCtx->hwvirt.svm.GCPhysVmcb = GCPhysVmcb;

    /*
     * Save the host state.
     */
    CPUMSvmVmRunSaveHostState(pCtx, cbInstr);

    /*
     * Read the guest VMCB state.
     */
    PVM pVM = pVCpu->CTX_SUFF(pVM);
    int rc = PGMPhysSimpleReadGCPhys(pVM, pCtx->hwvirt.svm.CTX_SUFF(pVmcb), GCPhysVmcb, sizeof(SVMVMCB));
    if (RT_SUCCESS(rc))
    {
        PSVMVMCBCTRL      pVmcbCtrl   = &pCtx->hwvirt.svm.CTX_SUFF(pVmcb)->ctrl;
        PSVMVMCBSTATESAVE pVmcbNstGst = &pCtx->hwvirt.svm.CTX_SUFF(pVmcb)->guest;

        /*
         * Validate guest-state and controls.
         */
        /* VMRUN must always be intercepted. */
        if (!CPUMIsGuestSvmCtrlInterceptSet(pCtx, SVM_CTRL_INTERCEPT_VMRUN))
        {
            Log(("iemSvmVmrun: VMRUN instruction not intercepted -> #VMEXIT\n"));
            return iemSvmVmexit(pVCpu, pCtx, SVM_EXIT_INVALID, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
        }

        /* Nested paging. */
        if (    pVmcbCtrl->NestedPaging.n.u1NestedPaging
            && !pVM->cpum.ro.GuestFeatures.fSvmNestedPaging)
        {
            Log(("iemSvmVmrun: Nested paging not supported -> #VMEXIT\n"));
            return iemSvmVmexit(pVCpu, pCtx, SVM_EXIT_INVALID, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
        }

        /* AVIC. */
        if (    pVmcbCtrl->IntCtrl.n.u1AvicEnable
            && !pVM->cpum.ro.GuestFeatures.fSvmAvic)
        {
            Log(("iemSvmVmrun: AVIC not supported -> #VMEXIT\n"));
            return iemSvmVmexit(pVCpu, pCtx, SVM_EXIT_INVALID, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
        }

        /* Last branch record (LBR) virtualization. */
        if (    (pVmcbCtrl->u64LBRVirt & SVM_LBR_VIRT_ENABLE)
            && !pVM->cpum.ro.GuestFeatures.fSvmLbrVirt)
        {
            Log(("iemSvmVmrun: LBR virtualization not supported -> #VMEXIT\n"));
            return iemSvmVmexit(pVCpu, pCtx, SVM_EXIT_INVALID, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
        }

        /* Guest ASID. */
        if (!pVmcbCtrl->TLBCtrl.n.u32ASID)
        {
            Log(("iemSvmVmrun: Guest ASID is invalid -> #VMEXIT\n"));
            return iemSvmVmexit(pVCpu, pCtx, SVM_EXIT_INVALID, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
        }

        /* IO permission bitmap. */
        RTGCPHYS const GCPhysIOBitmap = pVmcbCtrl->u64IOPMPhysAddr;
        if (   (GCPhysIOBitmap & X86_PAGE_4K_OFFSET_MASK)
            || !PGMPhysIsGCPhysNormal(pVM, GCPhysIOBitmap)
            || !PGMPhysIsGCPhysNormal(pVM, GCPhysIOBitmap + X86_PAGE_4K_SIZE)
            || !PGMPhysIsGCPhysNormal(pVM, GCPhysIOBitmap + (X86_PAGE_4K_SIZE << 1)))
        {
            Log(("iemSvmVmrun: IO bitmap physaddr invalid. GCPhysIOBitmap=%#RX64 -> #VMEXIT\n", GCPhysIOBitmap));
            return iemSvmVmexit(pVCpu, pCtx, SVM_EXIT_INVALID, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
        }

        /* MSR permission bitmap. */
        RTGCPHYS const GCPhysMsrBitmap = pVmcbCtrl->u64MSRPMPhysAddr;
        if (   (GCPhysMsrBitmap & X86_PAGE_4K_OFFSET_MASK)
            || !PGMPhysIsGCPhysNormal(pVM, GCPhysMsrBitmap)
            || !PGMPhysIsGCPhysNormal(pVM, GCPhysMsrBitmap + X86_PAGE_4K_SIZE))
        {
            Log(("iemSvmVmrun: MSR bitmap physaddr invalid. GCPhysMsrBitmap=%#RX64 -> #VMEXIT\n", GCPhysMsrBitmap));
            return iemSvmVmexit(pVCpu, pCtx, SVM_EXIT_INVALID, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
        }

        /* CR0. */
        if (   !(pVmcbNstGst->u64CR0 & X86_CR0_CD)
            &&  (pVmcbNstGst->u64CR0 & X86_CR0_NW))
        {
            Log(("iemSvmVmrun: CR0 no-write through with cache disabled. CR0=%#RX64 -> #VMEXIT\n", pVmcbNstGst->u64CR0));
            return iemSvmVmexit(pVCpu, pCtx, SVM_EXIT_INVALID, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
        }
        if (pVmcbNstGst->u64CR0 >> 32)
        {
            Log(("iemSvmVmrun: CR0 reserved bits set. CR0=%#RX64 -> #VMEXIT\n", pVmcbNstGst->u64CR0));
            return iemSvmVmexit(pVCpu, pCtx, SVM_EXIT_INVALID, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
        }
        /** @todo Implement all reserved bits/illegal combinations for CR3, CR4. */

        /* DR6 and DR7. */
        if (   pVmcbNstGst->u64DR6 >> 32
            || pVmcbNstGst->u64DR7 >> 32)
        {
            Log(("iemSvmVmrun: DR6 and/or DR7 reserved bits set. DR6=%#RX64 DR7=%#RX64 -> #VMEXIT\n", pVmcbNstGst->u64DR6,
                 pVmcbNstGst->u64DR6));
            return iemSvmVmexit(pVCpu, pCtx, SVM_EXIT_INVALID, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
        }

        /** @todo gPAT MSR validation? */

        /*
         * Copy the IO permission bitmap into the cache.
         */
        Assert(pCtx->hwvirt.svm.CTX_SUFF(pvIoBitmap));
        rc = PGMPhysSimpleReadGCPhys(pVM, pCtx->hwvirt.svm.CTX_SUFF(pvIoBitmap), GCPhysIOBitmap,
                                     SVM_IOPM_PAGES * X86_PAGE_4K_SIZE);
        if (RT_FAILURE(rc))
        {
            Log(("iemSvmVmrun: Failed reading the IO permission bitmap at %#RGp. rc=%Rrc\n", GCPhysIOBitmap, rc));
            return iemSvmVmexit(pVCpu, pCtx, SVM_EXIT_INVALID, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
        }

        /*
         * Copy the MSR permission bitmap into the cache.
         */
        Assert(pCtx->hwvirt.svm.CTX_SUFF(pvMsrBitmap));
        rc = PGMPhysSimpleReadGCPhys(pVM, pCtx->hwvirt.svm.CTX_SUFF(pvMsrBitmap), GCPhysMsrBitmap,
                                     SVM_MSRPM_PAGES * X86_PAGE_4K_SIZE);
        if (RT_FAILURE(rc))
        {
            Log(("iemSvmVmrun: Failed reading the MSR permission bitmap at %#RGp. rc=%Rrc\n", GCPhysMsrBitmap, rc));
            return iemSvmVmexit(pVCpu, pCtx, SVM_EXIT_INVALID, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
        }

        /*
         * Copy segments from nested-guest VMCB state to the guest-CPU state.
         *
         * We do this here as we need to use the CS attributes and it's easier this way
         * then using the VMCB format selectors. It doesn't really matter where we copy
         * the state, we restore the guest-CPU context state on the \#VMEXIT anyway.
         */
        HMSVM_SEG_REG_COPY_FROM_VMCB(pCtx, pVmcbNstGst, ES, es);
        HMSVM_SEG_REG_COPY_FROM_VMCB(pCtx, pVmcbNstGst, CS, cs);
        HMSVM_SEG_REG_COPY_FROM_VMCB(pCtx, pVmcbNstGst, SS, ss);
        HMSVM_SEG_REG_COPY_FROM_VMCB(pCtx, pVmcbNstGst, DS, ds);

        /** @todo Segment attribute overrides by VMRUN. */

        /*
         * CPL adjustments and overrides.
         *
         * SS.DPL is apparently the CPU's CPL, see comment in CPUMGetGuestCPL().
         * We shall thus adjust both CS.DPL and SS.DPL here.
         */
        pCtx->cs.Attr.n.u2Dpl = pCtx->ss.Attr.n.u2Dpl = pVmcbNstGst->u8CPL;
        if (CPUMIsGuestInV86ModeEx(pCtx))
            pCtx->cs.Attr.n.u2Dpl = pCtx->ss.Attr.n.u2Dpl = 3;
        if (CPUMIsGuestInRealModeEx(pCtx))
            pCtx->cs.Attr.n.u2Dpl = pCtx->ss.Attr.n.u2Dpl = 0;

        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->ss));

        /*
         * Continue validating guest-state and controls.
         *
         * We pass CR0 as 0 to CPUMQueryValidatedGuestEfer below to skip the illegal
         * EFER.LME bit transition check. We pass the nested-guest's EFER as both the
         * old and new EFER value to not have any guest EFER bits influence the new
         * nested-guest EFER.
         */
        uint64_t uValidEfer;
        rc = CPUMQueryValidatedGuestEfer(pVM, 0 /* CR0 */, pVmcbNstGst->u64EFER, pVmcbNstGst->u64EFER, &uValidEfer);
        if (RT_FAILURE(rc))
        {
            Log(("iemSvmVmrun: EFER invalid uOldEfer=%#RX64 -> #VMEXIT\n", pVmcbNstGst->u64EFER));
            return iemSvmVmexit(pVCpu, pCtx, SVM_EXIT_INVALID, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
        }

        /* Validate paging and CPU mode bits. */
        bool const fSvm                     = RT_BOOL(uValidEfer & MSR_K6_EFER_SVME);
        bool const fLongModeSupported       = RT_BOOL(pVM->cpum.ro.GuestFeatures.fLongMode);
        bool const fLongModeEnabled         = RT_BOOL(uValidEfer & MSR_K6_EFER_LME);
        bool const fPaging                  = RT_BOOL(pVmcbNstGst->u64CR0 & X86_CR0_PG);
        bool const fPae                     = RT_BOOL(pVmcbNstGst->u64CR4 & X86_CR4_PAE);
        bool const fProtMode                = RT_BOOL(pVmcbNstGst->u64CR0 & X86_CR0_PE);
        bool const fLongModeWithPaging      = fLongModeEnabled && fPaging;
        bool const fLongModeConformCS       = pCtx->cs.Attr.n.u1Long && pCtx->cs.Attr.n.u1DefBig;
        /* Adjust EFER.LMA (this is normally done by the CPU when system software writes CR0). */
        if (fLongModeWithPaging)
            uValidEfer |= MSR_K6_EFER_LMA;
        bool const fLongModeActiveOrEnabled = RT_BOOL(uValidEfer & (MSR_K6_EFER_LME | MSR_K6_EFER_LMA));
        if (   !fSvm
            || (!fLongModeSupported && fLongModeActiveOrEnabled)
            || (fLongModeWithPaging && !fPae)
            || (fLongModeWithPaging && !fProtMode)
            || (   fLongModeEnabled
                && fPaging
                && fPae
                && fLongModeConformCS))
        {
            Log(("iemSvmVmrun: EFER invalid. uValidEfer=%#RX64 -> #VMEXIT\n", uValidEfer));
            return iemSvmVmexit(pVCpu, pCtx, SVM_EXIT_INVALID, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
        }

        /*
         * Preserve the required force-flags.
         *
         * We only preserve the force-flags that would affect the execution of the
         * nested-guest (or the guest).
         *
         *   - VMCPU_FF_INHIBIT_INTERRUPTS need -not- be preserved as it's for a single
         *     instruction which is this VMRUN instruction itself.
         *
         *   - VMCPU_FF_BLOCK_NMIS needs to be preserved as it blocks NMI until the
         *     execution of a subsequent IRET instruction in the guest.
         *
         *   - The remaining FFs (e.g. timers) can stay in place so that we will be
         *     able to generate interrupts that should cause #VMEXITs for the
         *     nested-guest.
         */
        pCtx->hwvirt.fLocalForcedActions = pVCpu->fLocalForcedActions & VMCPU_FF_BLOCK_NMIS;
        VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_BLOCK_NMIS);

        /*
         * Interrupt shadow.
         */
        if (pVmcbCtrl->u64IntShadow & SVM_INTERRUPT_SHADOW_ACTIVE)
        {
            LogFlow(("iemSvmVmrun: setting interrupt shadow. inhibit PC=%#RX64\n", pVmcbNstGst->u64RIP));
            /** @todo will this cause trouble if the nested-guest is 64-bit but the guest is 32-bit? */
            EMSetInhibitInterruptsPC(pVCpu, pVmcbNstGst->u64RIP);
        }

        /*
         * TLB flush control.
         * Currently disabled since it's redundant as we unconditionally flush the TLB
         * in iemSvmWorldSwitch() below.
         */
#if 0
        /** @todo @bugref{7243}: ASID based PGM TLB flushes. */
        if (   pVmcbCtrl->TLBCtrl.n.u8TLBFlush == SVM_TLB_FLUSH_ENTIRE
            || pVmcbCtrl->TLBCtrl.n.u8TLBFlush == SVM_TLB_FLUSH_SINGLE_CONTEXT
            || pVmcbCtrl->TLBCtrl.n.u8TLBFlush == SVM_TLB_FLUSH_SINGLE_CONTEXT_RETAIN_GLOBALS)
            PGMFlushTLB(pVCpu, pVmcbNstGst->u64CR3, true /* fGlobal */);
#endif

        /** @todo @bugref{7243}: SVM TSC offset, see tmCpuTickGetInternal. */

        /*
         * Copy the remaining guest state from the VMCB to the guest-CPU context.
         */
        pCtx->gdtr.cbGdt = pVmcbNstGst->GDTR.u32Limit;
        pCtx->gdtr.pGdt  = pVmcbNstGst->GDTR.u64Base;
        pCtx->idtr.cbIdt = pVmcbNstGst->IDTR.u32Limit;
        pCtx->idtr.pIdt  = pVmcbNstGst->IDTR.u64Base;
        CPUMSetGuestCR0(pVCpu, pVmcbNstGst->u64CR0);
        CPUMSetGuestCR4(pVCpu, pVmcbNstGst->u64CR4);
        pCtx->cr3        = pVmcbNstGst->u64CR3;
        pCtx->cr2        = pVmcbNstGst->u64CR2;
        pCtx->dr[6]      = pVmcbNstGst->u64DR6;
        pCtx->dr[7]      = pVmcbNstGst->u64DR7;
        pCtx->rflags.u64 = pVmcbNstGst->u64RFlags;
        pCtx->rax        = pVmcbNstGst->u64RAX;
        pCtx->rsp        = pVmcbNstGst->u64RSP;
        pCtx->rip        = pVmcbNstGst->u64RIP;
        CPUMSetGuestMsrEferNoCheck(pVCpu, pCtx->msrEFER, uValidEfer);

        /* Mask DR6, DR7 bits mandatory set/clear bits. */
        pCtx->dr[6] &= ~(X86_DR6_RAZ_MASK | X86_DR6_MBZ_MASK);
        pCtx->dr[6] |= X86_DR6_RA1_MASK;
        pCtx->dr[7] &= ~(X86_DR7_RAZ_MASK | X86_DR7_MBZ_MASK);
        pCtx->dr[7] |= X86_DR7_RA1_MASK;

        /*
         * Check for pending virtual interrupts.
         */
        if (pVmcbCtrl->IntCtrl.n.u1VIrqPending)
            VMCPU_FF_SET(pVCpu, VMCPU_FF_INTERRUPT_NESTED_GUEST);
        else
            Assert(!VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_INTERRUPT_NESTED_GUEST));

        /*
         * Update PGM, IEM and others of a world-switch.
         */
        VBOXSTRICTRC rcStrict = iemSvmWorldSwitch(pVCpu, pCtx);
        if (rcStrict == VINF_SUCCESS)
        { /* likely */ }
        else if (RT_SUCCESS(rcStrict))
        {
            LogFlow(("iemSvmVmrun: iemSvmWorldSwitch returned %Rrc, setting passup status\n", VBOXSTRICTRC_VAL(rcStrict)));
            rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
        }
        else
        {
            LogFlow(("iemSvmVmrun: iemSvmWorldSwitch unexpected failure. rc=%Rrc\n", VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }

        /*
         * Clear global interrupt flags to allow interrupts in the guest.
         */
        pCtx->hwvirt.svm.fGif = true;

        /*
         * Event injection.
         */
        PCSVMEVENT pEventInject = &pVmcbCtrl->EventInject;
        pCtx->hwvirt.svm.fInterceptEvents = !pEventInject->n.u1Valid;
        if (pEventInject->n.u1Valid)
        {
            uint8_t   const uVector    = pEventInject->n.u8Vector;
            TRPMEVENT const enmType    = HMSvmEventToTrpmEventType(pEventInject);
            uint16_t  const uErrorCode = pEventInject->n.u1ErrorCodeValid ? pEventInject->n.u32ErrorCode : 0;

            /* Validate vectors for hardware exceptions, see AMD spec. 15.20 "Event Injection". */
            if (RT_UNLIKELY(enmType == TRPM_32BIT_HACK))
            {
                Log(("iemSvmVmrun: Invalid event type =%#x -> #VMEXIT\n", (uint8_t)pEventInject->n.u3Type));
                return iemSvmVmexit(pVCpu, pCtx, SVM_EXIT_INVALID, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
            }
            if (pEventInject->n.u3Type == SVM_EVENT_EXCEPTION)
            {
                if (   uVector == X86_XCPT_NMI
                    || uVector > X86_XCPT_LAST)
                {
                    Log(("iemSvmVmrun: Invalid vector for hardware exception. uVector=%#x -> #VMEXIT\n", uVector));
                    return iemSvmVmexit(pVCpu, pCtx, SVM_EXIT_INVALID, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
                }
                if (   uVector == X86_XCPT_BR
                    && CPUMIsGuestInLongModeEx(pCtx))
                {
                    Log(("iemSvmVmrun: Cannot inject #BR when not in long mode -> #VMEXIT\n"));
                    return iemSvmVmexit(pVCpu, pCtx, SVM_EXIT_INVALID, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
                }
                /** @todo any others? */
            }

            /*
             * Update the exit interruption info field so that if an exception occurs
             * while delivering the event causing a #VMEXIT, we only need to update
             * the valid bit while the rest is already in place.
             */
            pVmcbCtrl->ExitIntInfo.u = pVmcbCtrl->EventInject.u;
            pVmcbCtrl->ExitIntInfo.n.u1Valid = 0;

            /** @todo NRIP: Software interrupts can only be pushed properly if we support
             *        NRIP for the nested-guest to calculate the instruction length
             *        below. */
            LogFlow(("iemSvmVmrun: Injecting event: %04x:%08RX64 uVector=%#x enmType=%d uErrorCode=%u cr2=%#RX64 efer=%#RX64\n",
                     pCtx->cs.Sel, pCtx->rip, uVector, enmType, uErrorCode, pCtx->cr2, pCtx->msrEFER));
            rcStrict = IEMInjectTrap(pVCpu, uVector, enmType, uErrorCode, pCtx->cr2, 0 /* cbInstr */);
        }
        else
            LogFlow(("iemSvmVmrun: Entering nested-guest: %04x:%08RX64 cr0=%#RX64 cr3=%#RX64 cr4=%#RX64 efer=%#RX64 efl=%#x\n",
                     pCtx->cs.Sel, pCtx->rip, pCtx->cr0, pCtx->cr3, pCtx->cr4, pCtx->msrEFER, pCtx->rflags.u64));

        LogFlow(("iemSvmVmrun: returns %d\n", VBOXSTRICTRC_VAL(rcStrict)));
        return rcStrict;
    }

    /* Shouldn't really happen as the caller should've validated the physical address already. */
    Log(("iemSvmVmrun: Failed to read nested-guest VMCB at %#RGp (rc=%Rrc) -> #VMEXIT\n", GCPhysVmcb, rc));
    return rc;
#endif
}


#if 0
/**
 * Handles nested-guest SVM control intercepts and performs the \#VMEXIT if the
 * intercept is active.
 *
 * @returns Strict VBox status code.
 * @retval  VINF_SVM_INTERCEPT_NOT_ACTIVE if the intercept is not active or
 *          we're not executing a nested-guest.
 * @retval  VINF_SVM_VMEXIT if the intercept is active and the \#VMEXIT occurred
 *          successfully.
 * @retval  VERR_SVM_VMEXIT_FAILED if the intercept is active and the \#VMEXIT
 *          failed and a shutdown needs to be initiated for the geust.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        The guest-CPU context.
 * @param   uExitCode   The SVM exit code (see SVM_EXIT_XXX).
 * @param   uExitInfo1  The exit info. 1 field.
 * @param   uExitInfo2  The exit info. 2 field.
 */
VMM_INT_DECL(VBOXSTRICTRC) HMSvmNstGstHandleCtrlIntercept(PVMCPU pVCpu, PCPUMCTX pCtx, uint64_t uExitCode, uint64_t uExitInfo1,
                                                          uint64_t uExitInfo2)
{
#define HMSVM_CTRL_INTERCEPT_VMEXIT(a_Intercept) \
    do { \
        if (CPUMIsGuestSvmCtrlInterceptSet(pCtx, (a_Intercept))) \
            return iemSvmVmexit(pVCpu, pCtx, uExitCode, uExitInfo1, uExitInfo2); \
        break; \
    } while (0)

    if (!CPUMIsGuestInSvmNestedHwVirtMode(pCtx))
        return VINF_HM_INTERCEPT_NOT_ACTIVE;

    switch (uExitCode)
    {
        case SVM_EXIT_EXCEPTION_0:  case SVM_EXIT_EXCEPTION_1:  case SVM_EXIT_EXCEPTION_2:  case SVM_EXIT_EXCEPTION_3:
        case SVM_EXIT_EXCEPTION_4:  case SVM_EXIT_EXCEPTION_5:  case SVM_EXIT_EXCEPTION_6:  case SVM_EXIT_EXCEPTION_7:
        case SVM_EXIT_EXCEPTION_8:  case SVM_EXIT_EXCEPTION_9:  case SVM_EXIT_EXCEPTION_10: case SVM_EXIT_EXCEPTION_11:
        case SVM_EXIT_EXCEPTION_12: case SVM_EXIT_EXCEPTION_13: case SVM_EXIT_EXCEPTION_14: case SVM_EXIT_EXCEPTION_15:
        case SVM_EXIT_EXCEPTION_16: case SVM_EXIT_EXCEPTION_17: case SVM_EXIT_EXCEPTION_18: case SVM_EXIT_EXCEPTION_19:
        case SVM_EXIT_EXCEPTION_20: case SVM_EXIT_EXCEPTION_21: case SVM_EXIT_EXCEPTION_22: case SVM_EXIT_EXCEPTION_23:
        case SVM_EXIT_EXCEPTION_24: case SVM_EXIT_EXCEPTION_25: case SVM_EXIT_EXCEPTION_26: case SVM_EXIT_EXCEPTION_27:
        case SVM_EXIT_EXCEPTION_28: case SVM_EXIT_EXCEPTION_29: case SVM_EXIT_EXCEPTION_30: case SVM_EXIT_EXCEPTION_31:
        {
            if (CPUMIsGuestSvmXcptInterceptSet(pCtx, (X86XCPT)(uExitCode - SVM_EXIT_EXCEPTION_0)))
                    return iemSvmVmexit(pVCpu, pCtx, uExitCode, uExitInfo1, uExitInfo2);
                break;
        }

        case SVM_EXIT_WRITE_CR0:  case SVM_EXIT_WRITE_CR1:  case SVM_EXIT_WRITE_CR2:  case SVM_EXIT_WRITE_CR3:
        case SVM_EXIT_WRITE_CR4:  case SVM_EXIT_WRITE_CR5:  case SVM_EXIT_WRITE_CR6:  case SVM_EXIT_WRITE_CR7:
        case SVM_EXIT_WRITE_CR8:  case SVM_EXIT_WRITE_CR9:  case SVM_EXIT_WRITE_CR10: case SVM_EXIT_WRITE_CR11:
        case SVM_EXIT_WRITE_CR12: case SVM_EXIT_WRITE_CR13: case SVM_EXIT_WRITE_CR14: case SVM_EXIT_WRITE_CR15:
        {
            if (CPUMIsGuestSvmWriteCRxInterceptSet(pCtx, uExitCode - SVM_EXIT_WRITE_CR0))
                return iemSvmVmexit(pVCpu, pCtx, uExitCode, uExitInfo1, uExitInfo2);
            break;
        }

        case SVM_EXIT_READ_CR0:  case SVM_EXIT_READ_CR1:  case SVM_EXIT_READ_CR2:  case SVM_EXIT_READ_CR3:
        case SVM_EXIT_READ_CR4:  case SVM_EXIT_READ_CR5:  case SVM_EXIT_READ_CR6:  case SVM_EXIT_READ_CR7:
        case SVM_EXIT_READ_CR8:  case SVM_EXIT_READ_CR9:  case SVM_EXIT_READ_CR10: case SVM_EXIT_READ_CR11:
        case SVM_EXIT_READ_CR12: case SVM_EXIT_READ_CR13: case SVM_EXIT_READ_CR14: case SVM_EXIT_READ_CR15:
        {
            if (CPUMIsGuestSvmReadCRxInterceptSet(pCtx, uExitCode - SVM_EXIT_READ_CR0))
                return iemSvmVmexit(pVCpu, pCtx, uExitCode, uExitInfo1, uExitInfo2);
            break;
        }

        case SVM_EXIT_READ_DR0:  case SVM_EXIT_READ_DR1:  case SVM_EXIT_READ_DR2:  case SVM_EXIT_READ_DR3:
        case SVM_EXIT_READ_DR4:  case SVM_EXIT_READ_DR5:  case SVM_EXIT_READ_DR6:  case SVM_EXIT_READ_DR7:
        case SVM_EXIT_READ_DR8:  case SVM_EXIT_READ_DR9:  case SVM_EXIT_READ_DR10: case SVM_EXIT_READ_DR11:
        case SVM_EXIT_READ_DR12: case SVM_EXIT_READ_DR13: case SVM_EXIT_READ_DR14: case SVM_EXIT_READ_DR15:
        {
            if (CPUMIsGuestSvmReadDRxInterceptSet(pCtx, uExitCode - SVM_EXIT_READ_DR0))
                return iemSvmVmexit(pVCpu, pCtx, uExitCode, uExitInfo1, uExitInfo2);
            break;
        }

        case SVM_EXIT_WRITE_DR0:  case SVM_EXIT_WRITE_DR1:  case SVM_EXIT_WRITE_DR2:  case SVM_EXIT_WRITE_DR3:
        case SVM_EXIT_WRITE_DR4:  case SVM_EXIT_WRITE_DR5:  case SVM_EXIT_WRITE_DR6:  case SVM_EXIT_WRITE_DR7:
        case SVM_EXIT_WRITE_DR8:  case SVM_EXIT_WRITE_DR9:  case SVM_EXIT_WRITE_DR10: case SVM_EXIT_WRITE_DR11:
        case SVM_EXIT_WRITE_DR12: case SVM_EXIT_WRITE_DR13: case SVM_EXIT_WRITE_DR14: case SVM_EXIT_WRITE_DR15:
        {
            if (CPUMIsGuestSvmWriteDRxInterceptSet(pCtx, uExitCode - SVM_EXIT_WRITE_DR0))
                return iemSvmVmexit(pVCpu, pCtx, uExitCode, uExitInfo1, uExitInfo2);
            break;
        }

        case SVM_EXIT_INTR:                HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_INTR);
        case SVM_EXIT_NMI:                 HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_NMI);
        case SVM_EXIT_SMI:                 HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_SMI);
        case SVM_EXIT_INIT:                HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_INIT);
        case SVM_EXIT_VINTR:               HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_VINTR);
        case SVM_EXIT_CR0_SEL_WRITE:       HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_CR0_SEL_WRITES);
        case SVM_EXIT_IDTR_READ:           HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_IDTR_READS);
        case SVM_EXIT_GDTR_READ:           HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_GDTR_READS);
        case SVM_EXIT_LDTR_READ:           HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_LDTR_READS);
        case SVM_EXIT_TR_READ:             HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_TR_READS);
        case SVM_EXIT_IDTR_WRITE:          HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_IDTR_WRITES);
        case SVM_EXIT_GDTR_WRITE:          HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_GDTR_WRITES);
        case SVM_EXIT_LDTR_WRITE:          HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_LDTR_WRITES);
        case SVM_EXIT_TR_WRITE:            HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_TR_WRITES);
        case SVM_EXIT_RDTSC:               HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_RDTSC);
        case SVM_EXIT_RDPMC:               HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_RDPMC);
        case SVM_EXIT_PUSHF:               HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_PUSHF);
        case SVM_EXIT_POPF:                HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_POPF);
        case SVM_EXIT_CPUID:               HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_CPUID);
        case SVM_EXIT_RSM:                 HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_RSM);
        case SVM_EXIT_IRET:                HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_IRET);
        case SVM_EXIT_SWINT:               HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_INTN);
        case SVM_EXIT_INVD:                HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_INVD);
        case SVM_EXIT_PAUSE:               HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_PAUSE);
        case SVM_EXIT_HLT:                 HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_HLT);
        case SVM_EXIT_INVLPG:              HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_INVLPG);
        case SVM_EXIT_INVLPGA:             HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_INVLPGA);
        case SVM_EXIT_TASK_SWITCH:         HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_TASK_SWITCH);
        case SVM_EXIT_FERR_FREEZE:         HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_FERR_FREEZE);
        case SVM_EXIT_SHUTDOWN:            HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_SHUTDOWN);
        case SVM_EXIT_VMRUN:               HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_VMRUN);
        case SVM_EXIT_VMMCALL:             HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_VMMCALL);
        case SVM_EXIT_VMLOAD:              HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_VMLOAD);
        case SVM_EXIT_VMSAVE:              HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_VMSAVE);
        case SVM_EXIT_STGI:                HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_STGI);
        case SVM_EXIT_CLGI:                HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_CLGI);
        case SVM_EXIT_SKINIT:              HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_SKINIT);
        case SVM_EXIT_RDTSCP:              HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_RDTSCP);
        case SVM_EXIT_ICEBP:               HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_ICEBP);
        case SVM_EXIT_WBINVD:              HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_WBINVD);
        case SVM_EXIT_MONITOR:             HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_MONITOR);
        case SVM_EXIT_MWAIT:               HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_MWAIT);
        case SVM_EXIT_MWAIT_ARMED:         HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_MWAIT_ARMED);
        case SVM_EXIT_XSETBV:              HMSVM_CTRL_INTERCEPT_VMEXIT(SVM_CTRL_INTERCEPT_XSETBV);

        case SVM_EXIT_IOIO:
            AssertMsgFailed(("Use HMSvmNstGstHandleMsrIntercept!\n"));
            return VERR_SVM_IPE_1;

        case SVM_EXIT_MSR:
            AssertMsgFailed(("Use HMSvmNstGstHandleMsrIntercept!\n"));
            return VERR_SVM_IPE_1;

        case SVM_EXIT_NPF:
        case SVM_EXIT_AVIC_INCOMPLETE_IPI:
        case SVM_EXIT_AVIC_NOACCEL:
            AssertMsgFailed(("Todo Implement.\n"));
            return VERR_SVM_IPE_1;

        default:
            AssertMsgFailed(("Unsupported SVM exit code %#RX64\n", uExitCode));
            return VERR_SVM_IPE_1;
    }

    return VINF_HM_INTERCEPT_NOT_ACTIVE;

#undef HMSVM_CTRL_INTERCEPT_VMEXIT
}
#endif


/**
 * Checks if the event intercepts and performs the \#VMEXIT if the corresponding
 * intercept is active.
 *
 * @returns Strict VBox status code.
 * @retval  VINF_HM_INTERCEPT_NOT_ACTIVE if the intercept is not active or
 *          we're not executing a nested-guest.
 * @retval  VINF_SVM_VMEXIT if the intercept is active and the \#VMEXIT occurred
 *          successfully.
 * @retval  VERR_SVM_VMEXIT_FAILED if the intercept is active and the \#VMEXIT
 *          failed and a shutdown needs to be initiated for the geust.
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   u16Port         The IO port being accessed.
 * @param   enmIoType       The type of IO access.
 * @param   cbReg           The IO operand size in bytes.
 * @param   cAddrSizeBits   The address size bits (for 16, 32 or 64).
 * @param   iEffSeg         The effective segment number.
 * @param   fRep            Whether this is a repeating IO instruction (REP prefix).
 * @param   fStrIo          Whether this is a string IO instruction.
 * @param   cbInstr         The length of the IO instruction in bytes.
 */
IEM_STATIC VBOXSTRICTRC iemHandleSvmEventIntercept(PVMCPU pVCpu, PCPUMCTX pCtx, uint8_t u8Vector, uint32_t fFlags, uint32_t uErr,
                                                   uint64_t uCr2)
{
    Assert(CPUMIsGuestInSvmNestedHwVirtMode(pCtx));

    /*
     * Handle SVM exception and software interrupt intercepts, see AMD spec. 15.12 "Exception Intercepts".
     *
     *   - NMI intercepts have their own exit code and do not cause SVM_EXIT_EXCEPTION_2 #VMEXITs.
     *   - External interrupts and software interrupts (INTn instruction) do not check the exception intercepts
     *     even when they use a vector in the range 0 to 31.
     *   - ICEBP should not trigger #DB intercept, but its own intercept.
     *   - For #PF exceptions, its intercept is checked before CR2 is written by the exception.
     */
    /* Check NMI intercept */
    if (   u8Vector == X86_XCPT_NMI
        && (fFlags & IEM_XCPT_FLAGS_T_CPU_XCPT)
        && IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_NMI))
    {
        Log2(("iemHandleSvmNstGstEventIntercept: NMI intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_NMI, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    /* Check ICEBP intercept. */
    if (   (fFlags & IEM_XCPT_FLAGS_ICEBP_INSTR)
        && IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_ICEBP))
    {
        Log2(("iemHandleSvmNstGstEventIntercept: ICEBP intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_ICEBP, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    /* Check CPU exception intercepts. */
    if (   (fFlags & IEM_XCPT_FLAGS_T_CPU_XCPT)
        && IEM_IS_SVM_XCPT_INTERCEPT_SET(pVCpu, u8Vector))
    {
        Assert(u8Vector <= X86_XCPT_LAST);
        uint64_t const uExitInfo1 = fFlags & IEM_XCPT_FLAGS_ERR ? uErr : 0;
        uint64_t const uExitInfo2 = fFlags & IEM_XCPT_FLAGS_CR2 ? uCr2 : 0;
        if (   IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fSvmDecodeAssist
            && u8Vector == X86_XCPT_PF
            && !(uErr & X86_TRAP_PF_ID))
        {
            /** @todo Nested-guest SVM - figure out fetching op-code bytes from IEM. */
#ifdef IEM_WITH_CODE_TLB
            AssertReleaseFailedReturn(VERR_IEM_IPE_5);
#else
            PSVMVMCBCTRL  pVmcbCtrl = &pCtx->hwvirt.svm.CTX_SUFF(pVmcb)->ctrl;
            uint8_t const offOpCode = pVCpu->iem.s.offOpcode;
            uint8_t const cbCurrent = pVCpu->iem.s.cbOpcode - pVCpu->iem.s.offOpcode;
            if (   cbCurrent > 0
                && cbCurrent < sizeof(pVmcbCtrl->abInstr))
            {
                Assert(cbCurrent <= sizeof(pVCpu->iem.s.abOpcode));
                memcpy(&pVmcbCtrl->abInstr[0], &pVCpu->iem.s.abOpcode[offOpCode], cbCurrent);
            }
#endif
        }
        Log2(("iemHandleSvmNstGstEventIntercept: Xcpt intercept u32InterceptXcpt=%#RX32 u8Vector=%#x "
              "uExitInfo1=%#RX64 uExitInfo2=%#RX64 -> #VMEXIT\n", pCtx->hwvirt.svm.CTX_SUFF(pVmcb)->ctrl.u32InterceptXcpt,
              u8Vector, uExitInfo1, uExitInfo2));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_EXCEPTION_0 + u8Vector, uExitInfo1, uExitInfo2);
    }

    /* Check software interrupt (INTn) intercepts. */
    if (   (fFlags & (  IEM_XCPT_FLAGS_T_SOFT_INT
                      | IEM_XCPT_FLAGS_BP_INSTR
                      | IEM_XCPT_FLAGS_ICEBP_INSTR
                      | IEM_XCPT_FLAGS_OF_INSTR)) == IEM_XCPT_FLAGS_T_SOFT_INT
        && IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_INTN))
    {
        uint64_t const uExitInfo1 = IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fSvmDecodeAssist ? u8Vector : 0;
        Log2(("iemHandleSvmNstGstEventIntercept: Software INT intercept (u8Vector=%#x) -> #VMEXIT\n", u8Vector));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_SWINT, uExitInfo1, 0 /* uExitInfo2 */);
    }

    return VINF_HM_INTERCEPT_NOT_ACTIVE;
}


/**
 * Checks the SVM IO permission bitmap and performs the \#VMEXIT if the
 * corresponding intercept is active.
 *
 * @returns Strict VBox status code.
 * @retval  VINF_HM_INTERCEPT_NOT_ACTIVE if the intercept is not active or
 *          we're not executing a nested-guest.
 * @retval  VINF_SVM_VMEXIT if the intercept is active and the \#VMEXIT occurred
 *          successfully.
 * @retval  VERR_SVM_VMEXIT_FAILED if the intercept is active and the \#VMEXIT
 *          failed and a shutdown needs to be initiated for the geust.
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   u16Port         The IO port being accessed.
 * @param   enmIoType       The type of IO access.
 * @param   cbReg           The IO operand size in bytes.
 * @param   cAddrSizeBits   The address size bits (for 16, 32 or 64).
 * @param   iEffSeg         The effective segment number.
 * @param   fRep            Whether this is a repeating IO instruction (REP prefix).
 * @param   fStrIo          Whether this is a string IO instruction.
 * @param   cbInstr         The length of the IO instruction in bytes.
 */
IEM_STATIC VBOXSTRICTRC iemSvmHandleIOIntercept(PVMCPU pVCpu, uint16_t u16Port, SVMIOIOTYPE enmIoType, uint8_t cbReg,
                                                uint8_t cAddrSizeBits, uint8_t iEffSeg, bool fRep, bool fStrIo, uint8_t cbInstr)
{
    Assert(IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_IOIO_PROT));
    Assert(cAddrSizeBits == 0 || cAddrSizeBits == 16 || cAddrSizeBits == 32 || cAddrSizeBits == 64);
    Assert(cbReg == 1 || cbReg == 2 || cbReg == 4 || cbReg == 8);

    Log3(("iemSvmHandleIOIntercept: u16Port=%#x (%u)\n", u16Port, u16Port));

    SVMIOIOEXITINFO IoExitInfo;
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    void *pvIoBitmap = pCtx->hwvirt.svm.CTX_SUFF(pvIoBitmap);
    bool const fIntercept = HMSvmIsIOInterceptActive(pvIoBitmap, u16Port, enmIoType, cbReg, cAddrSizeBits, iEffSeg, fRep, fStrIo,
                                                     &IoExitInfo);
    if (fIntercept)
    {
        Log3(("iemSvmHandleIOIntercept: u16Port=%#x (%u) -> #VMEXIT\n", u16Port, u16Port));
        return iemSvmVmexit(pVCpu, pCtx, SVM_EXIT_IOIO, IoExitInfo.u, pCtx->rip + cbInstr);
    }

    /** @todo remove later (for debugging as VirtualBox always traps all IO
     *        intercepts). */
    AssertMsgFailed(("iemSvmHandleIOIntercept: We expect an IO intercept here!\n"));
    return VINF_HM_INTERCEPT_NOT_ACTIVE;
}


/**
 * Checks the SVM MSR permission bitmap and performs the \#VMEXIT if the
 * corresponding intercept is active.
 *
 * @returns Strict VBox status code.
 * @retval  VINF_HM_INTERCEPT_NOT_ACTIVE if the MSR permission bitmap does not
 *          specify interception of the accessed MSR @a idMsr.
 * @retval  VINF_SVM_VMEXIT if the intercept is active and the \#VMEXIT occurred
 *          successfully.
 * @retval  VERR_SVM_VMEXIT_FAILED if the intercept is active and the \#VMEXIT
 *          failed and a shutdown needs to be initiated for the geust.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        The guest-CPU context.
 * @param   idMsr       The MSR being accessed in the nested-guest.
 * @param   fWrite      Whether this is an MSR write access, @c false implies an
 *                      MSR read.
 */
IEM_STATIC VBOXSTRICTRC iemSvmHandleMsrIntercept(PVMCPU pVCpu, PCPUMCTX pCtx, uint32_t idMsr, bool fWrite)
{
    /*
     * Check if any MSRs are being intercepted.
     */
    Assert(CPUMIsGuestSvmCtrlInterceptSet(pCtx, SVM_CTRL_INTERCEPT_MSR_PROT));
    Assert(CPUMIsGuestInSvmNestedHwVirtMode(pCtx));

    uint64_t const uExitInfo1 = fWrite ? SVM_EXIT1_MSR_WRITE : SVM_EXIT1_MSR_READ;

    /*
     * Get the byte and bit offset of the permission bits corresponding to the MSR.
     */
    uint16_t offMsrpm;
    uint32_t uMsrpmBit;
    int rc = HMSvmGetMsrpmOffsetAndBit(idMsr, &offMsrpm, &uMsrpmBit);
    if (RT_SUCCESS(rc))
    {
        Assert(uMsrpmBit < 0x3fff);
        Assert(offMsrpm < SVM_MSRPM_PAGES << X86_PAGE_4K_SHIFT);
        if (fWrite)
            ++uMsrpmBit;

        /*
         * Check if the bit is set, if so, trigger a #VMEXIT.
         */
        uint8_t *pbMsrpm = (uint8_t *)pCtx->hwvirt.svm.CTX_SUFF(pvMsrBitmap);
        pbMsrpm += offMsrpm;
        if (ASMBitTest(pbMsrpm, uMsrpmBit))
            return iemSvmVmexit(pVCpu, pCtx, SVM_EXIT_MSR, uExitInfo1, 0 /* uExitInfo2 */);
    }
    else
    {
        /*
         * This shouldn't happen, but if it does, cause a #VMEXIT and let the "host" (guest hypervisor) deal with it.
         */
        Log(("iemSvmHandleMsrIntercept: Invalid/out-of-range MSR %#RX32 fWrite=%RTbool -> #VMEXIT\n", idMsr, fWrite));
        return iemSvmVmexit(pVCpu, pCtx, SVM_EXIT_MSR, uExitInfo1, 0 /* uExitInfo2 */);
    }
    return VINF_HM_INTERCEPT_NOT_ACTIVE;
}



/**
 * Implements 'VMRUN'.
 */
IEM_CIMPL_DEF_0(iemCImpl_vmrun)
{
#if defined(VBOX_WITH_NESTED_HWVIRT_ONLY_IN_IEM) && !defined(IN_RING3)
    RT_NOREF2(pVCpu, cbInstr);
    return VINF_EM_RAW_EMULATE_INSTR;
#else
    LogFlow(("iemCImpl_vmrun\n"));
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    IEM_SVM_INSTR_COMMON_CHECKS(pVCpu, vmrun);

    /** @todo Check effective address size using address size prefix. */
    RTGCPHYS const GCPhysVmcb = pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT ? pCtx->rax : pCtx->eax;
    if (   (GCPhysVmcb & X86_PAGE_4K_OFFSET_MASK)
        || !PGMPhysIsGCPhysNormal(pVCpu->CTX_SUFF(pVM), GCPhysVmcb))
    {
        Log(("vmrun: VMCB physaddr (%#RGp) not valid -> #GP(0)\n", GCPhysVmcb));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_VMRUN))
    {
        Log(("vmrun: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_VMRUN, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    VBOXSTRICTRC rcStrict = iemSvmVmrun(pVCpu, pCtx, cbInstr, GCPhysVmcb);
    if (rcStrict == VERR_SVM_VMEXIT_FAILED)
    {
        Assert(!CPUMIsGuestInSvmNestedHwVirtMode(pCtx));
        rcStrict = VINF_EM_TRIPLE_FAULT;
    }
    return rcStrict;
#endif
}


/**
 * Implements 'VMMCALL'.
 */
IEM_CIMPL_DEF_0(iemCImpl_vmmcall)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_VMMCALL))
    {
        Log(("vmmcall: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_VMMCALL, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    bool fUpdatedRipAndRF;
    VBOXSTRICTRC rcStrict = HMSvmVmmcall(pVCpu, pCtx, &fUpdatedRipAndRF);
    if (RT_SUCCESS(rcStrict))
    {
        if (!fUpdatedRipAndRF)
            iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        return rcStrict;
    }

    return iemRaiseUndefinedOpcode(pVCpu);
}


/**
 * Implements 'VMLOAD'.
 */
IEM_CIMPL_DEF_0(iemCImpl_vmload)
{
#if defined(VBOX_WITH_NESTED_HWVIRT_ONLY_IN_IEM) && !defined(IN_RING3)
    RT_NOREF2(pVCpu, cbInstr);
    return VINF_EM_RAW_EMULATE_INSTR;
#else
    LogFlow(("iemCImpl_vmload\n"));
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    IEM_SVM_INSTR_COMMON_CHECKS(pVCpu, vmload);

    /** @todo Check effective address size using address size prefix. */
    RTGCPHYS const GCPhysVmcb = pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT ? pCtx->rax : pCtx->eax;
    if (   (GCPhysVmcb & X86_PAGE_4K_OFFSET_MASK)
        || !PGMPhysIsGCPhysNormal(pVCpu->CTX_SUFF(pVM), GCPhysVmcb))
    {
        Log(("vmload: VMCB physaddr (%#RGp) not valid -> #GP(0)\n", GCPhysVmcb));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_VMLOAD))
    {
        Log(("vmload: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_VMLOAD, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    SVMVMCBSTATESAVE VmcbNstGst;
    VBOXSTRICTRC rcStrict = PGMPhysSimpleReadGCPhys(pVCpu->CTX_SUFF(pVM), &VmcbNstGst, GCPhysVmcb + RT_OFFSETOF(SVMVMCB, guest),
                                                    sizeof(SVMVMCBSTATESAVE));
    if (rcStrict == VINF_SUCCESS)
    {
        LogFlow(("vmload: Loading VMCB at %#RGp enmEffAddrMode=%d\n", GCPhysVmcb, pVCpu->iem.s.enmEffAddrMode));
        HMSVM_SEG_REG_COPY_FROM_VMCB(pCtx, &VmcbNstGst, FS, fs);
        HMSVM_SEG_REG_COPY_FROM_VMCB(pCtx, &VmcbNstGst, GS, gs);
        HMSVM_SEG_REG_COPY_FROM_VMCB(pCtx, &VmcbNstGst, TR, tr);
        HMSVM_SEG_REG_COPY_FROM_VMCB(pCtx, &VmcbNstGst, LDTR, ldtr);

        pCtx->msrKERNELGSBASE = VmcbNstGst.u64KernelGSBase;
        pCtx->msrSTAR         = VmcbNstGst.u64STAR;
        pCtx->msrLSTAR        = VmcbNstGst.u64LSTAR;
        pCtx->msrCSTAR        = VmcbNstGst.u64CSTAR;
        pCtx->msrSFMASK       = VmcbNstGst.u64SFMASK;

        pCtx->SysEnter.cs     = VmcbNstGst.u64SysEnterCS;
        pCtx->SysEnter.esp    = VmcbNstGst.u64SysEnterESP;
        pCtx->SysEnter.eip    = VmcbNstGst.u64SysEnterEIP;

        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    }
    return rcStrict;
#endif
}


/**
 * Implements 'VMSAVE'.
 */
IEM_CIMPL_DEF_0(iemCImpl_vmsave)
{
#if defined(VBOX_WITH_NESTED_HWVIRT_ONLY_IN_IEM) && !defined(IN_RING3)
    RT_NOREF2(pVCpu, cbInstr);
    return VINF_EM_RAW_EMULATE_INSTR;
#else
    LogFlow(("iemCImpl_vmsave\n"));
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    IEM_SVM_INSTR_COMMON_CHECKS(pVCpu, vmsave);

    /** @todo Check effective address size using address size prefix. */
    RTGCPHYS const GCPhysVmcb = pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT ? pCtx->rax : pCtx->eax;
    if (   (GCPhysVmcb & X86_PAGE_4K_OFFSET_MASK)
        || !PGMPhysIsGCPhysNormal(pVCpu->CTX_SUFF(pVM), GCPhysVmcb))
    {
        Log(("vmsave: VMCB physaddr (%#RGp) not valid -> #GP(0)\n", GCPhysVmcb));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_VMSAVE))
    {
        Log(("vmsave: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_VMSAVE, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    SVMVMCBSTATESAVE VmcbNstGst;
    VBOXSTRICTRC rcStrict = PGMPhysSimpleReadGCPhys(pVCpu->CTX_SUFF(pVM), &VmcbNstGst, GCPhysVmcb + RT_OFFSETOF(SVMVMCB, guest),
                                                    sizeof(SVMVMCBSTATESAVE));
    if (rcStrict == VINF_SUCCESS)
    {
        LogFlow(("vmsave: Saving VMCB at %#RGp enmEffAddrMode=%d\n", GCPhysVmcb, pVCpu->iem.s.enmEffAddrMode));
        HMSVM_SEG_REG_COPY_TO_VMCB(pCtx, &VmcbNstGst, FS, fs);
        HMSVM_SEG_REG_COPY_TO_VMCB(pCtx, &VmcbNstGst, GS, gs);
        HMSVM_SEG_REG_COPY_TO_VMCB(pCtx, &VmcbNstGst, TR, tr);
        HMSVM_SEG_REG_COPY_TO_VMCB(pCtx, &VmcbNstGst, LDTR, ldtr);

        VmcbNstGst.u64KernelGSBase  = pCtx->msrKERNELGSBASE;
        VmcbNstGst.u64STAR          = pCtx->msrSTAR;
        VmcbNstGst.u64LSTAR         = pCtx->msrLSTAR;
        VmcbNstGst.u64CSTAR         = pCtx->msrCSTAR;
        VmcbNstGst.u64SFMASK        = pCtx->msrSFMASK;

        VmcbNstGst.u64SysEnterCS    = pCtx->SysEnter.cs;
        VmcbNstGst.u64SysEnterESP   = pCtx->SysEnter.esp;
        VmcbNstGst.u64SysEnterEIP   = pCtx->SysEnter.eip;

        rcStrict = PGMPhysSimpleWriteGCPhys(pVCpu->CTX_SUFF(pVM), GCPhysVmcb + RT_OFFSETOF(SVMVMCB, guest), &VmcbNstGst,
                                            sizeof(SVMVMCBSTATESAVE));
        if (rcStrict == VINF_SUCCESS)
            iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    }
    return rcStrict;
#endif
}


/**
 * Implements 'CLGI'.
 */
IEM_CIMPL_DEF_0(iemCImpl_clgi)
{
#if defined(VBOX_WITH_NESTED_HWVIRT_ONLY_IN_IEM) && !defined(IN_RING3)
    RT_NOREF2(pVCpu, cbInstr);
    return VINF_EM_RAW_EMULATE_INSTR;
#else
    LogFlow(("iemCImpl_clgi\n"));
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    IEM_SVM_INSTR_COMMON_CHECKS(pVCpu, clgi);
    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_CLGI))
    {
        Log(("clgi: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_CLGI, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    pCtx->hwvirt.svm.fGif = false;
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);

# if defined(VBOX_WITH_NESTED_HWVIRT_ONLY_IN_IEM) && defined(IN_RING3)
    return EMR3SetExecutionPolicy(pVCpu->CTX_SUFF(pVM)->pUVM, EMEXECPOLICY_IEM_ALL, true);
# else
    return VINF_SUCCESS;
# endif
#endif
}


/**
 * Implements 'STGI'.
 */
IEM_CIMPL_DEF_0(iemCImpl_stgi)
{
#if defined(VBOX_WITH_NESTED_HWVIRT_ONLY_IN_IEM) && !defined(IN_RING3)
    RT_NOREF2(pVCpu, cbInstr);
    return VINF_EM_RAW_EMULATE_INSTR;
#else
    LogFlow(("iemCImpl_stgi\n"));
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    IEM_SVM_INSTR_COMMON_CHECKS(pVCpu, stgi);
    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_STGI))
    {
        Log2(("stgi: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_STGI, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    pCtx->hwvirt.svm.fGif = true;
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);

# if defined(VBOX_WITH_NESTED_HWVIRT_ONLY_IN_IEM) && defined(IN_RING3)
    return EMR3SetExecutionPolicy(pVCpu->CTX_SUFF(pVM)->pUVM, EMEXECPOLICY_IEM_ALL, false);
# else
    return VINF_SUCCESS;
# endif
#endif
}


/**
 * Implements 'INVLPGA'.
 */
IEM_CIMPL_DEF_0(iemCImpl_invlpga)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    /** @todo Check effective address size using address size prefix. */
    RTGCPTR  const GCPtrPage = pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT ? pCtx->rax : pCtx->eax;
    /** @todo PGM needs virtual ASID support. */
#if 0
    uint32_t const uAsid     = pCtx->ecx;
#endif

    IEM_SVM_INSTR_COMMON_CHECKS(pVCpu, invlpga);
    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_INVLPGA))
    {
        Log2(("invlpga: Guest intercept (%RGp) -> #VMEXIT\n", GCPtrPage));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_INVLPGA, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    PGMInvalidatePage(pVCpu, GCPtrPage);
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'SKINIT'.
 */
IEM_CIMPL_DEF_0(iemCImpl_skinit)
{
    IEM_SVM_INSTR_COMMON_CHECKS(pVCpu, invlpga);

    uint32_t uIgnore;
    uint32_t fFeaturesECX;
    CPUMGetGuestCpuId(pVCpu, 0x80000001, 0 /* iSubLeaf */, &uIgnore, &uIgnore, &fFeaturesECX, &uIgnore);
    if (!(fFeaturesECX & X86_CPUID_AMD_FEATURE_ECX_SKINIT))
        return iemRaiseUndefinedOpcode(pVCpu);

    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_SKINIT))
    {
        Log2(("skinit: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_SKINIT, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    RT_NOREF(cbInstr);
    return VERR_IEM_INSTR_NOT_IMPLEMENTED;
}

