/* $Id: CSAMAll.cpp $ */
/** @file
 * CSAM - Guest OS Code Scanning and Analysis Manager - Any Context
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
#define LOG_GROUP LOG_GROUP_CSAM
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/patm.h>
#include <VBox/vmm/csam.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/mm.h>
#ifdef VBOX_WITH_REM
# include <VBox/vmm/rem.h>
#endif
#include <VBox/sup.h>
#include <VBox/vmm/mm.h>
#include <VBox/param.h>
#include <iprt/avl.h>
#include "CSAMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/vmm.h>
#include <VBox/dbg.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <VBox/dis.h>
#include <VBox/disopcode.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/string.h>

#ifdef IN_RING0
# error "IN_RING3 & IN_RC only!"
#endif


/**
 * @callback_method_impl{FNPGMVIRTHANDLER,
 * Access handler callback for virtual access handler ranges.}
 */
PGM_ALL_CB2_DECL(VBOXSTRICTRC)
csamCodePageWriteHandler(PVM pVM, PVMCPU pVCpu, RTGCPTR GCPtr, void *pvPtr, void *pvBuf, size_t cbBuf,
                         PGMACCESSTYPE enmAccessType, PGMACCESSORIGIN enmOrigin, void *pvUser)
{
    Log(("csamCodePageWriteHandler: write to %RGv LB %zu\n", GCPtr, cbBuf));
    Assert(enmAccessType == PGMACCESSTYPE_WRITE); NOREF(enmAccessType);
    Assert(VMCPU_IS_EMT(pVCpu));
    RT_NOREF_PV(pvUser);
    RT_NOREF_PV(enmOrigin);

    /*
     * Check if it's a dummy write that doesn't change anything.
     */
    if (   PAGE_ADDRESS(pvPtr) == PAGE_ADDRESS((uintptr_t)pvPtr + cbBuf - 1)
        && !memcmp(pvPtr, pvBuf, cbBuf))
    {
        Log(("csamCodePageWriteHandler: dummy write -> ignore\n"));
        return VINF_PGM_HANDLER_DO_DEFAULT;
    }

#ifdef IN_RING3
    /*
     * Ring-3: Do proper handling.
     */
    int rc = PATMR3PatchWrite(pVM, GCPtr, (uint32_t)cbBuf);
    AssertRC(rc);
    RT_NOREF_PV(pVCpu);
    return VINF_PGM_HANDLER_DO_DEFAULT;

#else
    /*
     * Raw-mode: Try avoid needing to go to ring-3 (same as csamRCCodePageWritePfHandler).
     */
    uint32_t     const cpl            = CPUMGetGuestCPL(pVCpu);
    bool         const fPatchCode     = PATMIsPatchGCAddr(pVM, CPUMGetGuestRIP(pVCpu));
    PPATMGCSTATE       pPATMGCState   = PATMGetGCState(pVM);

    Assert(pVM->csam.s.cDirtyPages < CSAM_MAX_DIRTY_PAGES);
    Assert(pPATMGCState);
    Assert(pPATMGCState->fPIF || fPatchCode);

# ifdef VBOX_WITH_REM
    /* Flush the recompilers translation block cache as the guest seems to be modifying instructions. */
    /** @todo a bit overkill?? */
    REMFlushTBs(pVM);
# endif

    /*
     * When patch code is executing instructions that must complete, then we
     * must *never* interrupt it.
     */
    if (!pPATMGCState->fPIF && fPatchCode)
    {
        Log(("csamRCCodePageWriteHandler: fPIF=0 -> stack fault in patch generated code at %08RX32!\n", CPUMGetGuestRIP(pVCpu)));
        return VINF_PGM_HANDLER_DO_DEFAULT;
    }

    Log(("csamRCCodePageWriteHandler: code page write at %RGv (cpl=%d)\n", GCPtr, cpl));

    /*
     * If user code is modifying one of our monitored pages, then we can safely
     * write to it as it's no longer being used for supervisor code.
     */
    if (cpl != 3)
    {
        VBOXSTRICTRC rcStrict = PATMRCHandleWriteToPatchPage(pVM, NULL /* pRegFrame = no interpret */, (RTRCPTR)GCPtr, cbBuf);
        if (   rcStrict == VINF_PGM_HANDLER_DO_DEFAULT
            || rcStrict == VINF_SUCCESS)
            return rcStrict;
        if (rcStrict == VINF_EM_RAW_EMULATE_INSTR)
        {
            STAM_COUNTER_INC(&pVM->csam.s.StatDangerousWrite);
            return VINF_EM_RAW_EMULATE_INSTR;
        }
        Assert(rcStrict == VERR_PATCH_NOT_FOUND);
    }

    /*
     * Schedule ring-3 activity.
     * Note that GCPtr might be a different address in case of aliases.  So,
     * take down both alternatives.
     */
    VMCPU_FF_SET(pVCpu, VMCPU_FF_CSAM_PENDING_ACTION);
    pVM->csam.s.pvDirtyBasePage[pVM->csam.s.cDirtyPages]  = (RTRCPTR)GCPtr;
    pVM->csam.s.pvDirtyFaultPage[pVM->csam.s.cDirtyPages] = (RTRCPTR)GCPtr;
    if (++pVM->csam.s.cDirtyPages == CSAM_MAX_DIRTY_PAGES)
        return VINF_CSAM_PENDING_ACTION;

    /*
     * Continue with the write. The VM_FF_CSAM_FLUSH_DIRTY_PAGE handler will reset it to readonly again.
     */
    Log(("csamRCCodePageWriteHandler: enabled r/w for page %RGv (%RGv)\n", GCPtr, GCPtr));
    STAM_COUNTER_INC(&pVM->csam.s.StatCodePageModified);
    return VINF_PGM_HANDLER_DO_DEFAULT;
#endif
}


/**
 * Check if this page needs to be analysed by CSAM
 *
 * @returns VBox status code
 * @param   pVM         The cross context VM structure.
 * @param   pvFault     Fault address
 */
VMM_INT_DECL(int) CSAMExecFault(PVM pVM, RTRCPTR pvFault)
{
    Assert(!HMIsEnabled(pVM));
    if (!CSAMIsEnabled(pVM))
        return VINF_SUCCESS;

    LogFlow(("CSAMGCExecFault: for page %08X scanned=%d\n", pvFault, CSAMIsPageScanned(pVM, pvFault)));

    if (CSAMIsPageScanned(pVM, pvFault))
    {
        // Already checked!
        STAM_COUNTER_ADD(&pVM->csam.s.StatNrKnownPagesGC, 1);
        return VINF_SUCCESS;
    }

    STAM_COUNTER_ADD(&pVM->csam.s.StatNrTraps, 1);
    VMCPU_FF_SET(VMMGetCpu0(pVM), VMCPU_FF_CSAM_SCAN_PAGE);
    return VINF_CSAM_PENDING_ACTION;
}


/**
 * Check if this page was previously scanned by CSAM
 *
 * @returns true -> scanned, false -> not scanned
 * @param   pVM         The cross context VM structure.
 * @param   pPage       GC page address
 */
VMM_INT_DECL(bool) CSAMIsPageScanned(PVM pVM, RTRCPTR pPage)
{
    int pgdir, bit;
    uintptr_t page;
    Assert(!HMIsEnabled(pVM));

    page  = (uintptr_t)pPage;
    pgdir = page >> X86_PAGE_4M_SHIFT;
    bit   = (page & X86_PAGE_4M_OFFSET_MASK) >> X86_PAGE_4K_SHIFT;

    Assert(pgdir < CSAM_PGDIRBMP_CHUNKS);
    Assert(bit < PAGE_SIZE);

    return pVM->csam.s.CTXSUFF(pPDBitmap)[pgdir] && ASMBitTest((void *)pVM->csam.s.CTXSUFF(pPDBitmap)[pgdir], bit);
}



/**
 * Mark a page as scanned/not scanned
 *
 * @note: we always mark it as scanned, even if we haven't completely done so
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPage       GC page address (not necessarily aligned)
 * @param   fScanned    Mark as scanned or not scanned
 *
 */
VMM_INT_DECL(int) CSAMMarkPage(PVM pVM, RTRCUINTPTR pPage, bool fScanned)
{
    int pgdir, bit;
    uintptr_t page;

#ifdef LOG_ENABLED
    if (fScanned && !CSAMIsPageScanned(pVM, (RTRCPTR)pPage))
       Log(("CSAMMarkPage %RRv\n", pPage));
#endif

    if (!CSAMIsEnabled(pVM))
        return VINF_SUCCESS;
    Assert(!HMIsEnabled(pVM));

    page  = (uintptr_t)pPage;
    pgdir = page >> X86_PAGE_4M_SHIFT;
    bit   = (page & X86_PAGE_4M_OFFSET_MASK) >> X86_PAGE_4K_SHIFT;

    Assert(pgdir < CSAM_PGDIRBMP_CHUNKS);
    Assert(bit < PAGE_SIZE);

    if(!CTXSUFF(pVM->csam.s.pPDBitmap)[pgdir])
    {
        STAM_COUNTER_INC(&pVM->csam.s.StatBitmapAlloc);
        int rc = MMHyperAlloc(pVM, CSAM_PAGE_BITMAP_SIZE, 0, MM_TAG_CSAM, (void **)&pVM->csam.s.CTXSUFF(pPDBitmap)[pgdir]);
        if (RT_FAILURE(rc))
        {
            Log(("MMHyperAlloc failed with %Rrc\n", rc));
            return rc;
        }
#ifdef IN_RC
        pVM->csam.s.pPDHCBitmapGC[pgdir] = MMHyperRCToR3(pVM, (RCPTRTYPE(void*))pVM->csam.s.pPDBitmapGC[pgdir]);
        if (!pVM->csam.s.pPDHCBitmapGC[pgdir])
        {
            Log(("MMHyperHC2GC failed for %RRv\n", pVM->csam.s.pPDBitmapGC[pgdir]));
            return rc;
        }
#else
        pVM->csam.s.pPDGCBitmapHC[pgdir] = MMHyperR3ToRC(pVM, pVM->csam.s.pPDBitmapHC[pgdir]);
        if (!pVM->csam.s.pPDGCBitmapHC[pgdir])
        {
            Log(("MMHyperHC2GC failed for %RHv\n", pVM->csam.s.pPDBitmapHC[pgdir]));
            return rc;
        }
#endif
    }
    if(fScanned)
        ASMBitSet((void *)pVM->csam.s.CTXSUFF(pPDBitmap)[pgdir], bit);
    else
        ASMBitClear((void *)pVM->csam.s.CTXSUFF(pPDBitmap)[pgdir], bit);

    return VINF_SUCCESS;
}

/**
 * Check if this page needs to be analysed by CSAM.
 *
 * This function should only be called for supervisor pages and
 * only when CSAM is enabled. Leaving these selection criteria
 * to the caller simplifies the interface (PTE passing).
 *
 * Note that the page has not yet been synced, so the TLB trick
 * (which wasn't ever active anyway) cannot be applied.
 *
 * @returns true if the page should be marked not present because
 *          CSAM want need to scan it.
 * @returns false if the page was already scanned.
 * @param   pVM         The cross context VM structure.
 * @param   GCPtr       GC pointer of page
 */
VMM_INT_DECL(bool) CSAMDoesPageNeedScanning(PVM pVM, RTRCUINTPTR GCPtr)
{
    if (!CSAMIsEnabled(pVM))
        return false;
    Assert(!HMIsEnabled(pVM));

    if(CSAMIsPageScanned(pVM, (RTRCPTR)GCPtr))
    {
        /* Already checked! */
        STAM_COUNTER_ADD(&CTXSUFF(pVM->csam.s.StatNrKnownPages), 1);
        return false;
    }
    STAM_COUNTER_ADD(&CTXSUFF(pVM->csam.s.StatNrPageNP), 1);
    return true;
}


/**
 * Remember a possible code page for later inspection
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   GCPtr       GC pointer of page
 */
VMM_INT_DECL(void) CSAMMarkPossibleCodePage(PVM pVM, RTRCPTR GCPtr)
{
    Assert(!HMIsEnabled(pVM));
    if (pVM->csam.s.cPossibleCodePages < RT_ELEMENTS(pVM->csam.s.pvPossibleCodePage))
    {
        pVM->csam.s.pvPossibleCodePage[pVM->csam.s.cPossibleCodePages++] = (RTRCPTR)GCPtr;
        VMCPU_FF_SET(VMMGetCpu0(pVM), VMCPU_FF_CSAM_PENDING_ACTION);
    }
    return;
}


/**
 * Turn on code scanning
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMM_INT_DECL(int) CSAMEnableScanning(PVM pVM)
{
    AssertReturn(!HMIsEnabled(pVM), VERR_CSAM_HM_IPE);
    pVM->fCSAMEnabled = true;
    return VINF_SUCCESS;
}

/**
 * Turn off code scanning
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMM_INT_DECL(int) CSAMDisableScanning(PVM pVM)
{
    pVM->fCSAMEnabled = false;
    return VINF_SUCCESS;
}


/**
 * Check if we've scanned this instruction before. If true, then we can emulate
 * it instead of returning to ring 3.
 *
 * Using a simple array here as there are generally few mov crx instructions and
 * tree lookup is likely to be more expensive. (as it would also have to be offset based)
 *
 * @returns boolean
 * @param   pVM         The cross context VM structure.
 * @param   GCPtr       GC pointer of page table entry
 */
VMM_INT_DECL(bool) CSAMIsKnownDangerousInstr(PVM pVM, RTRCUINTPTR GCPtr)
{
    Assert(!HMIsEnabled(pVM));

    for (uint32_t i=0;i<pVM->csam.s.cDangerousInstr;i++)
    {
        if (pVM->csam.s.aDangerousInstr[i] == (RTRCPTR)GCPtr)
        {
            STAM_COUNTER_INC(&pVM->csam.s.StatInstrCacheHit);
            return true;
        }
    }
    /* Record that we're about to process it in ring 3. */
    pVM->csam.s.aDangerousInstr[pVM->csam.s.iDangerousInstr++] = (RTRCPTR)GCPtr;
    pVM->csam.s.iDangerousInstr &= CSAM_MAX_DANGR_INSTR_MASK;

    if (++pVM->csam.s.cDangerousInstr > CSAM_MAX_DANGR_INSTR)
        pVM->csam.s.cDangerousInstr = CSAM_MAX_DANGR_INSTR;

    STAM_COUNTER_INC(&pVM->csam.s.StatInstrCacheMiss);
    return false;
}
