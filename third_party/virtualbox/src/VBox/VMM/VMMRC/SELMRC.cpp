/* $Id: SELMRC.cpp $ */
/** @file
 * SELM - The Selector Manager, Guest Context.
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
#define LOG_GROUP LOG_GROUP_SELM
#include <VBox/vmm/selm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/trpm.h>
#include "SELMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/vmm.h>
#include <VBox/vmm/pgm.h>

#include <VBox/param.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/asm.h>

#include "SELMInline.h"


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
#ifdef LOG_ENABLED
/** Segment register names. */
static char const g_aszSRegNms[X86_SREG_COUNT][4] = { "ES", "CS", "SS", "DS", "FS", "GS" };
#endif


#ifdef SELM_TRACK_GUEST_GDT_CHANGES

/**
 * Synchronizes one GDT entry (guest -> shadow).
 *
 * @returns VBox strict status code (appropriate for trap handling and GC
 *          return).
 * @retval  VINF_SUCCESS
 * @retval  VINF_EM_RAW_EMULATE_INSTR_GDT_FAULT
 * @retval  VINF_SELM_SYNC_GDT
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pCtx        CPU context for the current CPU.
 * @param   iGDTEntry   The GDT entry to sync.
 *
 * @remarks Caller checks that this isn't the LDT entry!
 */
static VBOXSTRICTRC selmRCSyncGDTEntry(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, unsigned iGDTEntry)
{
    Log2(("GDT %04X LDTR=%04X\n", iGDTEntry, CPUMGetGuestLDTR(pVCpu)));

    /*
     * Validate the offset.
     */
    VBOXGDTR GdtrGuest;
    CPUMGetGuestGDTR(pVCpu, &GdtrGuest);
    unsigned offEntry = iGDTEntry * sizeof(X86DESC);
    if (    iGDTEntry >= SELM_GDT_ELEMENTS
        ||  offEntry  >  GdtrGuest.cbGdt)
        return VINF_SUCCESS; /* ignore */

    /*
     * Read the guest descriptor.
     */
    X86DESC Desc;
    int rc = MMGCRamRead(pVM, &Desc, (uint8_t *)(uintptr_t)GdtrGuest.pGdt + offEntry, sizeof(X86DESC));
    if (RT_FAILURE(rc))
    {
        rc = PGMPhysSimpleReadGCPtr(pVCpu, &Desc, (uintptr_t)GdtrGuest.pGdt + offEntry, sizeof(X86DESC));
        if (RT_FAILURE(rc))
        {
            VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_GDT);
            VMCPU_FF_SET(pVCpu, VMCPU_FF_TO_R3); /* paranoia */
            /* return VINF_EM_RESCHEDULE_REM; - bad idea if we're in a patch. */
            return VINF_EM_RAW_EMULATE_INSTR_GDT_FAULT;
        }
    }

    /*
     * Check for conflicts.
     */
    RTSEL   Sel = iGDTEntry << X86_SEL_SHIFT;
    Assert(   !(pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS]         & ~X86_SEL_MASK_OFF_RPL)
           && !(pVM->selm.s.aHyperSel[SELM_HYPER_SEL_DS]         & ~X86_SEL_MASK_OFF_RPL)
           && !(pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS64]       & ~X86_SEL_MASK_OFF_RPL)
           && !(pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS]        & ~X86_SEL_MASK_OFF_RPL)
           && !(pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS_TRAP08] & ~X86_SEL_MASK_OFF_RPL));
    if (    pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS]         == Sel
        ||  pVM->selm.s.aHyperSel[SELM_HYPER_SEL_DS]         == Sel
        ||  pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS64]       == Sel
        ||  pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS]        == Sel
        ||  pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS_TRAP08] == Sel)
    {
        if (Desc.Gen.u1Present)
        {
            Log(("selmRCSyncGDTEntry: Sel=%d Desc=%.8Rhxs: detected conflict!!\n", Sel, &Desc));
            VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_GDT);
            VMCPU_FF_SET(pVCpu, VMCPU_FF_TO_R3);
            return VINF_SELM_SYNC_GDT;  /** @todo this status code is ignored, unfortunately. */
        }
        Log(("selmRCSyncGDTEntry: Sel=%d Desc=%.8Rhxs: potential conflict (still not present)!\n", Sel, &Desc));

        /* Note: we can't continue below or else we'll change the shadow descriptor!! */
        /* When the guest makes the selector present, then we'll do a GDT sync. */
        return VINF_SUCCESS;
    }

    /*
     * Convert the guest selector to a shadow selector and update the shadow GDT.
     */
    selmGuestToShadowDesc(pVM, &Desc);
    PX86DESC pShwDescr = &pVM->selm.s.paGdtRC[iGDTEntry];
    //Log(("O: base=%08X limit=%08X attr=%04X\n", X86DESC_BASE(*pShwDescr)), X86DESC_LIMIT(*pShwDescr), (pShwDescr->au32[1] >> 8) & 0xFFFF ));
    //Log(("N: base=%08X limit=%08X attr=%04X\n", X86DESC_BASE(Desc)), X86DESC_LIMIT(Desc), (Desc.au32[1] >> 8) & 0xFFFF ));
    *pShwDescr = Desc;

    /*
     * Detect and mark stale registers.
     */
    VBOXSTRICTRC rcStrict = VINF_SUCCESS;
    PCPUMSELREG  paSReg   = CPUMCTX_FIRST_SREG(pCtx);
    for (unsigned iSReg = 0; iSReg <= X86_SREG_COUNT; iSReg++)
    {
        if (Sel == (paSReg[iSReg].Sel & X86_SEL_MASK_OFF_RPL))
        {
            if (CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &paSReg[iSReg]))
            {
                if (selmIsSRegStale32(&paSReg[iSReg], &Desc, iSReg))
                {
                    Log(("GDT write to selector in %s register %04X (now stale)\n", g_aszSRegNms[iSReg], paSReg[iSReg].Sel));
                    paSReg[iSReg].fFlags |= CPUMSELREG_FLAGS_STALE;
                    VMCPU_FF_SET(pVCpu, VMCPU_FF_TO_R3); /* paranoia */
                    /* rcStrict = VINF_EM_RESCHEDULE_REM; - bad idea if we're in a patch. */
                    rcStrict = VINF_EM_RAW_EMULATE_INSTR_GDT_FAULT;
                }
                else if (paSReg[iSReg].fFlags & CPUMSELREG_FLAGS_STALE)
                {
                    Log(("GDT write to selector in %s register %04X (no longer stale)\n", g_aszSRegNms[iSReg], paSReg[iSReg].Sel));
                    paSReg[iSReg].fFlags &= ~CPUMSELREG_FLAGS_STALE;
                }
                else
                    Log(("GDT write to selector in %s register %04X (no important change)\n", g_aszSRegNms[iSReg], paSReg[iSReg].Sel));
            }
            else
                Log(("GDT write to selector in %s register %04X (out of sync)\n", g_aszSRegNms[iSReg], paSReg[iSReg].Sel));
        }
    }

    /** @todo Detect stale LDTR as well? */

    return rcStrict;
}


/**
 * Synchronizes any segment registers refering to the given GDT entry.
 *
 * This is called before any changes performed and shadowed, so it's possible to
 * look in both the shadow and guest descriptor table entries for hidden
 * register content.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pCtx        The CPU context.
 * @param   iGDTEntry   The GDT entry to sync.
 */
void selmRCSyncGdtSegRegs(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, unsigned iGDTEntry)
{
    /*
     * Validate the offset.
     */
    VBOXGDTR GdtrGuest;
    CPUMGetGuestGDTR(pVCpu, &GdtrGuest);
    unsigned offEntry = iGDTEntry * sizeof(X86DESC);
    if (    iGDTEntry >= SELM_GDT_ELEMENTS
        ||  offEntry  >  GdtrGuest.cbGdt)
        return;

    /*
     * Sync outdated segment registers using this entry.
     */
    PCX86DESC       pDesc    = &pVM->selm.s.CTX_SUFF(paGdt)[iGDTEntry];
    uint32_t        uCpl     = CPUMGetGuestCPL(pVCpu);
    PCPUMSELREG     paSReg   = CPUMCTX_FIRST_SREG(pCtx);
    for (unsigned iSReg = 0; iSReg <= X86_SREG_COUNT; iSReg++)
    {
        if (iGDTEntry == (paSReg[iSReg].Sel & X86_SEL_MASK_OFF_RPL))
        {
            if (!CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &paSReg[iSReg]))
            {
                if (selmIsShwDescGoodForSReg(&paSReg[iSReg], pDesc, iSReg, uCpl))
                {
                    selmLoadHiddenSRegFromShadowDesc(&paSReg[iSReg], pDesc);
                    Log(("selmRCSyncGDTSegRegs: Updated %s\n", g_aszSRegNms[iSReg]));
                }
                else
                    Log(("selmRCSyncGDTSegRegs: Bad shadow descriptor %#x (for %s): %.8Rhxs \n",
                         iGDTEntry, g_aszSRegNms[iSReg], pDesc));
            }
        }
    }
}


/**
 * Syncs hidden selector register parts before emulating a GDT change.
 *
 * This is shared between the selmRCGuestGDTWritePfHandler and
 * selmGuestGDTWriteHandler.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   offGuestTss     The offset into the TSS of the write that was made.
 * @param   cbWrite         The number of bytes written.
 * @param   pCtx            The current CPU context.
 */
void selmRCGuestGdtPreWriteCheck(PVM pVM, PVMCPU pVCpu, uint32_t offGuestGdt, uint32_t cbWrite, PCPUMCTX pCtx)
{
    uint32_t       iGdt      = offGuestGdt >> X86_SEL_SHIFT;
    uint32_t const iGdtLast  = (offGuestGdt + cbWrite - 1) >> X86_SEL_SHIFT;
    do
    {
        selmRCSyncGdtSegRegs(pVM, pVCpu, pCtx, iGdt);
        iGdt++;
    } while (iGdt <= iGdtLast);
}


/**
 * Checks the guest GDT for changes after a write has been emulated.
 *
 *
 * This is shared between the selmRCGuestGDTWritePfHandler and
 * selmGuestGDTWriteHandler.
 *
 * @retval  VINF_SUCCESS
 * @retval  VINF_SELM_SYNC_GDT
 * @retval  VINF_EM_RAW_EMULATE_INSTR_GDT_FAULT
 *
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   offGuestTss     The offset into the TSS of the write that was made.
 * @param   cbWrite         The number of bytes written.
 * @param   pCtx            The current CPU context.
 */
VBOXSTRICTRC selmRCGuestGdtPostWriteCheck(PVM pVM, PVMCPU pVCpu, uint32_t offGuestGdt, uint32_t cbWrite, PCPUMCTX pCtx)
{
    VBOXSTRICTRC rcStrict = VINF_SUCCESS;

    /* Check if the LDT was in any way affected.  Do not sync the
       shadow GDT if that's the case or we might have trouble in
       the world switcher (or so they say). */
    uint32_t const iGdtFirst = offGuestGdt >> X86_SEL_SHIFT;
    uint32_t const iGdtLast  = (offGuestGdt + cbWrite - 1) >> X86_SEL_SHIFT;
    uint32_t const iLdt      = CPUMGetGuestLDTR(pVCpu) >> X86_SEL_SHIFT;
    if (iGdtFirst <= iLdt && iGdtLast >= iLdt)
    {
        Log(("LDTR selector change -> fall back to HC!!\n"));
        VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_GDT);
        rcStrict = VINF_SELM_SYNC_GDT;
        /** @todo Implement correct stale LDT handling.  */
    }
    else
    {
        /* Sync the shadow GDT and continue provided the update didn't
           cause any segment registers to go stale in any way. */
        uint32_t iGdt = iGdtFirst;
        do
        {
            VBOXSTRICTRC rcStrict2 = selmRCSyncGDTEntry(pVM, pVCpu, pCtx, iGdt);
            Assert(rcStrict2 == VINF_SUCCESS || rcStrict2 == VINF_EM_RAW_EMULATE_INSTR_GDT_FAULT || rcStrict2 == VINF_SELM_SYNC_GDT);
            if (rcStrict == VINF_SUCCESS)
                rcStrict = rcStrict2;
            iGdt++;
        } while (   iGdt <= iGdtLast
                 && (rcStrict == VINF_SUCCESS || rcStrict == VINF_EM_RAW_EMULATE_INSTR_GDT_FAULT));
        if (rcStrict == VINF_SUCCESS || rcStrict == VINF_EM_RAW_EMULATE_INSTR_GDT_FAULT)
            STAM_COUNTER_INC(&pVM->selm.s.StatRCWriteGuestGDTHandled);
    }
    return rcStrict;
}


/**
 * @callback_method_impl{FNPGMVIRTHANDLER, Guest GDT write access \#PF handler }
 */
DECLEXPORT(VBOXSTRICTRC) selmRCGuestGDTWritePfHandler(PVM pVM, PVMCPU pVCpu, RTGCUINT uErrorCode, PCPUMCTXCORE pRegFrame,
                                                      RTGCPTR pvFault, RTGCPTR pvRange, uintptr_t offRange, void *pvUser)
{
    LogFlow(("selmRCGuestGDTWritePfHandler errcode=%x fault=%RGv offRange=%08x\n", (uint32_t)uErrorCode, pvFault, offRange));
    NOREF(pvRange); NOREF(pvUser); RT_NOREF_PV(uErrorCode);

    /*
     * Check if any selectors might be affected.
     */
    selmRCGuestGdtPreWriteCheck(pVM, pVCpu, offRange, 8 /*cbWrite*/, CPUMCTX_FROM_CORE(pRegFrame));

    /*
     * Attempt to emulate the instruction and sync the affected entries.
     */
    uint32_t cb;
    VBOXSTRICTRC rcStrict = EMInterpretInstructionEx(pVCpu, pRegFrame, (RTGCPTR)(RTRCUINTPTR)pvFault, &cb);
    if (RT_SUCCESS(rcStrict) && cb)
        rcStrict = selmRCGuestGdtPostWriteCheck(pVM, pVCpu, offRange, cb, CPUMCTX_FROM_CORE(pRegFrame));
    else
    {
        Assert(RT_FAILURE(rcStrict));
        if (rcStrict == VERR_EM_INTERPRETER)
            rcStrict = VINF_EM_RAW_EMULATE_INSTR; /* No, not VINF_EM_RAW_EMULATE_INSTR_GDT_FAULT, see PGM_PHYS_RW_IS_SUCCESS.  */
        VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_GDT);
    }

    if (!VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_SELM_SYNC_GDT))
        STAM_COUNTER_INC(&pVM->selm.s.StatRCWriteGuestGDTHandled);
    else
        STAM_COUNTER_INC(&pVM->selm.s.StatRCWriteGuestGDTUnhandled);
    return rcStrict;
}

#endif /* SELM_TRACK_GUEST_GDT_CHANGES */

#ifdef SELM_TRACK_GUEST_LDT_CHANGES
/**
 * @callback_method_impl{FNPGMVIRTHANDLER, Guest LDT write access \#PF handler }
 */
DECLEXPORT(VBOXSTRICTRC) selmRCGuestLDTWritePfHandler(PVM pVM, PVMCPU pVCpu, RTGCUINT uErrorCode, PCPUMCTXCORE pRegFrame,
                                                      RTGCPTR pvFault, RTGCPTR pvRange, uintptr_t offRange, void *pvUser)
{
    /** @todo To be implemented... or not. */
    ////LogCom(("selmRCGuestLDTWriteHandler: eip=%08X pvFault=%RGv pvRange=%RGv\r\n", pRegFrame->eip, pvFault, pvRange));
    NOREF(uErrorCode); NOREF(pRegFrame); NOREF(pvFault); NOREF(pvRange); NOREF(offRange); NOREF(pvUser);

    VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_LDT);
    STAM_COUNTER_INC(&pVM->selm.s.StatRCWriteGuestLDT); RT_NOREF_PV(pVM);
    return VINF_EM_RAW_EMULATE_INSTR_LDT_FAULT;
}
#endif


#ifdef SELM_TRACK_GUEST_TSS_CHANGES

/**
 * Read wrapper used by selmRCGuestTSSWriteHandler.
 * @returns VBox status code (appropriate for trap handling and GC return).
 * @param   pVM         The cross context VM structure.
 * @param   pvDst       Where to put the bits we read.
 * @param   pvSrc       Guest address to read from.
 * @param   cb          The number of bytes to read.
 */
DECLINLINE(int) selmRCReadTssBits(PVM pVM, PVMCPU pVCpu, void *pvDst, void const *pvSrc, size_t cb)
{
    int rc = MMGCRamRead(pVM, pvDst, (void *)pvSrc, cb);
    if (RT_SUCCESS(rc))
        return VINF_SUCCESS;

    /** @todo use different fallback?    */
    rc = PGMPrefetchPage(pVCpu, (uintptr_t)pvSrc);
    AssertMsg(rc == VINF_SUCCESS, ("PGMPrefetchPage %p failed with %Rrc\n", &pvSrc, rc));
    if (rc == VINF_SUCCESS)
    {
        rc = MMGCRamRead(pVM, pvDst, (void *)pvSrc, cb);
        AssertMsg(rc == VINF_SUCCESS, ("MMGCRamRead %p failed with %Rrc\n", &pvSrc, rc));
    }
    return rc;
}


/**
 * Checks the guest TSS for changes after a write has been emulated.
 *
 * This is shared between the
 *
 * @returns Strict VBox status code appropriate for raw-mode returns.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   offGuestTss     The offset into the TSS of the write that was made.
 * @param   cbWrite         The number of bytes written.
 */
VBOXSTRICTRC selmRCGuestTssPostWriteCheck(PVM pVM, PVMCPU pVCpu, uint32_t offGuestTss, uint32_t cbWrite)
{
    VBOXSTRICTRC rcStrict = VINF_SUCCESS;

    /*
     * If it's on the same page as the esp0 and ss0 fields or actually one of them,
     * then check if any of these has changed.
     */
/** @todo just read the darn fields and put them on the stack. */
    PCVBOXTSS pGuestTss = (PVBOXTSS)(uintptr_t)pVM->selm.s.GCPtrGuestTss;
    if (   PAGE_ADDRESS(&pGuestTss->esp0) == PAGE_ADDRESS(&pGuestTss->padding_ss0)
        && PAGE_ADDRESS(&pGuestTss->esp0) == PAGE_ADDRESS((uint8_t *)pGuestTss + offGuestTss)
        && (   pGuestTss->esp0 !=  pVM->selm.s.Tss.esp1
            || pGuestTss->ss0  != (pVM->selm.s.Tss.ss1 & ~1)) /* undo raw-r0 */
       )
    {
        Log(("selmRCGuestTSSWritePfHandler: R0 stack: %RTsel:%RGv -> %RTsel:%RGv\n",
             (RTSEL)(pVM->selm.s.Tss.ss1 & ~1), (RTGCPTR)pVM->selm.s.Tss.esp1, (RTSEL)pGuestTss->ss0, (RTGCPTR)pGuestTss->esp0));
        pVM->selm.s.Tss.esp1 = pGuestTss->esp0;
        pVM->selm.s.Tss.ss1  = pGuestTss->ss0 | 1;
        STAM_COUNTER_INC(&pVM->selm.s.StatRCWriteGuestTSSHandledChanged);
    }
# ifdef VBOX_WITH_RAW_RING1
    else if (   EMIsRawRing1Enabled(pVM)
             && PAGE_ADDRESS(&pGuestTss->esp1) == PAGE_ADDRESS(&pGuestTss->padding_ss1)
             && PAGE_ADDRESS(&pGuestTss->esp1) == PAGE_ADDRESS((uint8_t *)pGuestTss + offGuestTss)
             && (   pGuestTss->esp1 !=  pVM->selm.s.Tss.esp2
                 || pGuestTss->ss1  != ((pVM->selm.s.Tss.ss2 & ~2) | 1)) /* undo raw-r1 */
            )
    {
        Log(("selmRCGuestTSSWritePfHandler: R1 stack: %RTsel:%RGv -> %RTsel:%RGv\n",
             (RTSEL)((pVM->selm.s.Tss.ss2 & ~2) | 1), (RTGCPTR)pVM->selm.s.Tss.esp2, (RTSEL)pGuestTss->ss1, (RTGCPTR)pGuestTss->esp1));
        pVM->selm.s.Tss.esp2 = pGuestTss->esp1;
        pVM->selm.s.Tss.ss2  = (pGuestTss->ss1 & ~1) | 2;
        STAM_COUNTER_INC(&pVM->selm.s.StatRCWriteGuestTSSHandledChanged);
    }
# endif
    /* Handle misaligned TSS in a safe manner (just in case). */
    else if (   offGuestTss >= RT_UOFFSETOF(VBOXTSS, esp0)
             && offGuestTss < RT_UOFFSETOF(VBOXTSS, padding_ss0))
    {
        struct
        {
            uint32_t esp0;
            uint16_t ss0;
            uint16_t padding_ss0;
        } s;
        AssertCompileSize(s, 8);
        rcStrict = selmRCReadTssBits(pVM, pVCpu, &s, &pGuestTss->esp0, sizeof(s));
        if (   rcStrict == VINF_SUCCESS
            && (    s.esp0 !=  pVM->selm.s.Tss.esp1
                ||  s.ss0  != (pVM->selm.s.Tss.ss1 & ~1)) /* undo raw-r0 */
           )
        {
            Log(("selmRCGuestTSSWritePfHandler: R0 stack: %RTsel:%RGv -> %RTsel:%RGv [x-page]\n",
                 (RTSEL)(pVM->selm.s.Tss.ss1 & ~1), (RTGCPTR)pVM->selm.s.Tss.esp1, (RTSEL)s.ss0, (RTGCPTR)s.esp0));
            pVM->selm.s.Tss.esp1 = s.esp0;
            pVM->selm.s.Tss.ss1  = s.ss0 | 1;
            STAM_COUNTER_INC(&pVM->selm.s.StatRCWriteGuestTSSHandledChanged);
        }
    }

    /*
     * If VME is enabled we need to check if the interrupt redirection bitmap
     * needs updating.
     */
    if (   offGuestTss >= RT_UOFFSETOF(VBOXTSS, offIoBitmap)
        && (CPUMGetGuestCR4(pVCpu) & X86_CR4_VME))
    {
        if (offGuestTss - RT_UOFFSETOF(VBOXTSS, offIoBitmap) < sizeof(pGuestTss->offIoBitmap))
        {
            uint16_t offIoBitmap = pGuestTss->offIoBitmap;
            if (offIoBitmap != pVM->selm.s.offGuestIoBitmap)
            {
                Log(("TSS offIoBitmap changed: old=%#x new=%#x -> resync in ring-3\n", pVM->selm.s.offGuestIoBitmap, offIoBitmap));
                VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_TSS);
                VMCPU_FF_SET(pVCpu, VMCPU_FF_TO_R3);
            }
            else
                Log(("TSS offIoBitmap: old=%#x new=%#x [unchanged]\n", pVM->selm.s.offGuestIoBitmap, offIoBitmap));
        }
        else
        {
            /** @todo not sure how the partial case is handled; probably not allowed */
            uint32_t offIntRedirBitmap = pVM->selm.s.offGuestIoBitmap - sizeof(pVM->selm.s.Tss.IntRedirBitmap);
            if (   offIntRedirBitmap <= offGuestTss
                && offIntRedirBitmap + sizeof(pVM->selm.s.Tss.IntRedirBitmap) >= offGuestTss + cbWrite
                && offIntRedirBitmap + sizeof(pVM->selm.s.Tss.IntRedirBitmap) <= pVM->selm.s.cbGuestTss)
            {
                Log(("TSS IntRedirBitmap Changed: offIoBitmap=%x offIntRedirBitmap=%x cbTSS=%x offGuestTss=%x cbWrite=%x\n",
                     pVM->selm.s.offGuestIoBitmap, offIntRedirBitmap, pVM->selm.s.cbGuestTss, offGuestTss, cbWrite));

                /** @todo only update the changed part. */
                for (uint32_t i = 0; rcStrict == VINF_SUCCESS && i < sizeof(pVM->selm.s.Tss.IntRedirBitmap) / 8; i++)
                    rcStrict = selmRCReadTssBits(pVM, pVCpu, &pVM->selm.s.Tss.IntRedirBitmap[i * 8],
                                                 (uint8_t *)pGuestTss + offIntRedirBitmap + i * 8, 8);
                STAM_COUNTER_INC(&pVM->selm.s.StatRCWriteGuestTSSRedir);
            }
        }
    }

    /*
     * Return to ring-3 for a full resync if any of the above fails... (?)
     */
    if (rcStrict != VINF_SUCCESS)
    {
        VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_TSS);
        VMCPU_FF_SET(pVCpu, VMCPU_FF_TO_R3);
        if (RT_SUCCESS(rcStrict))
            rcStrict = VINF_SUCCESS;
    }

    STAM_COUNTER_INC(&pVM->selm.s.StatRCWriteGuestTSSHandled);
    return rcStrict;
}


/**
 * @callback_method_impl{FNPGMVIRTHANDLER, Guest TSS write access \#PF handler}
 */
DECLEXPORT(VBOXSTRICTRC) selmRCGuestTSSWritePfHandler(PVM pVM, PVMCPU pVCpu, RTGCUINT uErrorCode, PCPUMCTXCORE pRegFrame,
                                                      RTGCPTR pvFault, RTGCPTR pvRange, uintptr_t offRange, void *pvUser)
{
    LogFlow(("selmRCGuestTSSWritePfHandler errcode=%x fault=%RGv offRange=%08x\n", (uint32_t)uErrorCode, pvFault, offRange));
    NOREF(pvRange); NOREF(pvUser); RT_NOREF_PV(uErrorCode);

    /*
     * Try emulate the access.
     */
    uint32_t cb;
    VBOXSTRICTRC rcStrict = EMInterpretInstructionEx(pVCpu, pRegFrame, (RTGCPTR)(RTRCUINTPTR)pvFault, &cb);
    if (   RT_SUCCESS(rcStrict)
        && cb)
        rcStrict = selmRCGuestTssPostWriteCheck(pVM, pVCpu, offRange, cb);
    else
    {
        AssertMsg(RT_FAILURE(rcStrict), ("cb=%u rcStrict=%#x\n", cb, VBOXSTRICTRC_VAL(rcStrict)));
        VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_TSS);
        STAM_COUNTER_INC(&pVM->selm.s.StatRCWriteGuestTSSUnhandled);
        if (rcStrict == VERR_EM_INTERPRETER)
            rcStrict = VINF_EM_RAW_EMULATE_INSTR_TSS_FAULT;
    }
    return rcStrict;
}

#endif /* SELM_TRACK_GUEST_TSS_CHANGES */

#ifdef SELM_TRACK_SHADOW_GDT_CHANGES
/**
 * @callback_method_impl{FNPGMRCVIRTPFHANDLER,
 * \#PF Virtual Handler callback for Guest write access to the VBox shadow GDT.}
 */
DECLEXPORT(VBOXSTRICTRC) selmRCShadowGDTWritePfHandler(PVM pVM, PVMCPU pVCpu, RTGCUINT uErrorCode, PCPUMCTXCORE pRegFrame,
                                                       RTGCPTR pvFault, RTGCPTR pvRange, uintptr_t offRange, void *pvUser)
{
    LogRel(("FATAL ERROR: selmRCShadowGDTWritePfHandler: eip=%08X pvFault=%RGv pvRange=%RGv\r\n", pRegFrame->eip, pvFault, pvRange));
    NOREF(pVM); NOREF(pVCpu); NOREF(uErrorCode); NOREF(pRegFrame); NOREF(pvFault); NOREF(pvRange); NOREF(offRange); NOREF(pvUser);
    return VERR_SELM_SHADOW_GDT_WRITE;
}
#endif


#ifdef SELM_TRACK_SHADOW_LDT_CHANGES
/**
 * @callback_method_impl{FNPGMRCVIRTPFHANDLER,
 * \#PF Virtual Handler callback for Guest write access to the VBox shadow LDT.}
 */
DECLEXPORT(VBOXSTRICTRC) selmRCShadowLDTWritePfHandler(PVM pVM, PVMCPU pVCpu, RTGCUINT uErrorCode, PCPUMCTXCORE pRegFrame,
                                                       RTGCPTR pvFault, RTGCPTR pvRange, uintptr_t offRange, void *pvUser)
{
    LogRel(("FATAL ERROR: selmRCShadowLDTWritePfHandler: eip=%08X pvFault=%RGv pvRange=%RGv\r\n", pRegFrame->eip, pvFault, pvRange));
    Assert(pvFault - (uintptr_t)pVM->selm.s.pvLdtRC < (unsigned)(65536U + PAGE_SIZE));
    NOREF(pVM); NOREF(pVCpu); NOREF(uErrorCode); NOREF(pRegFrame); NOREF(pvFault); NOREF(pvRange); NOREF(offRange); NOREF(pvUser);
    return VERR_SELM_SHADOW_LDT_WRITE;
}
#endif


#ifdef SELM_TRACK_SHADOW_TSS_CHANGES
/**
 * @callback_method_impl{FNPGMRCVIRTPFHANDLER,
 * \#PF Virtual Handler callback for Guest write access to the VBox shadow TSS.}
 */
DECLEXPORT(VBOXSTRICTRC) selmRCShadowTSSWritePfHandler(PVM pVM, PVMCPU pVCpu, RTGCUINT uErrorCode, PCPUMCTXCORE pRegFrame,
                                                       RTGCPTR pvFault, RTGCPTR pvRange, uintptr_t offRange, void *pvUser)
{
    LogRel(("FATAL ERROR: selmRCShadowTSSWritePfHandler: eip=%08X pvFault=%RGv pvRange=%RGv\r\n", pRegFrame->eip, pvFault, pvRange));
    NOREF(pVM); NOREF(pVCpu); NOREF(uErrorCode); NOREF(pRegFrame); NOREF(pvFault); NOREF(pvRange); NOREF(offRange); NOREF(pvUser);
    return VERR_SELM_SHADOW_TSS_WRITE;
}
#endif

