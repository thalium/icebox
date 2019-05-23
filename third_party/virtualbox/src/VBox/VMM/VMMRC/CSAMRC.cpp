/* $Id: CSAMRC.cpp $ */
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
#include <VBox/vmm/mm.h>
#include <VBox/sup.h>
#include <VBox/vmm/mm.h>
#ifdef VBOX_WITH_REM
# include <VBox/vmm/rem.h>
#endif
#include <VBox/param.h>
#include <iprt/avl.h>
#include "CSAMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/dbg.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <VBox/dis.h>
#include <VBox/disopcode.h>
#include <iprt/asm.h>
#include <iprt/asm-amd64-x86.h>
#include <iprt/string.h>



/**
 * @callback_method_impl{FNPGMRCVIRTPFHANDLER,
 * \#PF Handler callback for virtual access handler ranges. (CSAM self-modifying
 * code monitor)}
 *
 * Important to realize that a physical page in a range can have aliases, and
 * for ALL and WRITE handlers these will also trigger.
 */
DECLEXPORT(VBOXSTRICTRC) csamRCCodePageWritePfHandler(PVM pVM, PVMCPU pVCpu, RTGCUINT uErrorCode, PCPUMCTXCORE pRegFrame,
                                                      RTGCPTR pvFault, RTGCPTR pvRange, uintptr_t offRange, void *pvUser)
{
    PPATMGCSTATE pPATMGCState;
    bool         fPatchCode = PATMIsPatchGCAddr(pVM, pRegFrame->eip);
    RT_NOREF_PV(uErrorCode);
    RT_NOREF_PV(pvUser);


    Assert(pVM->csam.s.cDirtyPages < CSAM_MAX_DIRTY_PAGES);

#ifdef VBOX_WITH_REM
    /* Flush the recompilers translation block cache as the guest seems to be modifying instructions. */
    REMFlushTBs(pVM);
#endif

    pPATMGCState = PATMGetGCState(pVM);
    Assert(pPATMGCState);

    Assert(pPATMGCState->fPIF || fPatchCode);
    /* When patch code is executing instructions that must complete, then we must *never* interrupt it. */
    if (!pPATMGCState->fPIF && fPatchCode)
    {
        Log(("csamRCCodePageWriteHandler: fPIF=0 -> stack fault in patch generated code at %08RX32!\n", pRegFrame->eip));
        /** @note there are cases when pages previously used for code are now used for stack; patch generated code will fault (pushf))
         *  Just make the page r/w and continue.
         */
        /*
         * Make this particular page R/W.
         */
        int rc = PGMShwMakePageWritable(pVCpu, pvFault, PGM_MK_PG_IS_WRITE_FAULT);
        AssertMsgRC(rc, ("PGMShwModifyPage -> rc=%Rrc\n", rc));
        ASMInvalidatePage((uintptr_t)pvFault);
        return VINF_SUCCESS;
    }

    uint32_t cpl;

    if (pRegFrame->eflags.Bits.u1VM)
        cpl = 3;
    else
        cpl = (pRegFrame->ss.Sel & X86_SEL_RPL);

    Log(("csamRCCodePageWriteHandler: code page write at %RGv original address %RGv (cpl=%d)\n", pvFault, (RTGCUINTPTR)pvRange + offRange, cpl));

    /* If user code is modifying one of our monitored pages, then we can safely make it r/w as it's no longer being used for supervisor code. */
    if (cpl != 3)
    {
        VBOXSTRICTRC rcStrict = PATMRCHandleWriteToPatchPage(pVM, pRegFrame, (RTRCPTR)((RTRCUINTPTR)pvRange + offRange),
                                                             4 /** @todo */);
        if (rcStrict == VINF_SUCCESS)
            return VBOXSTRICTRC_TODO(rcStrict);
        if (rcStrict == VINF_EM_RAW_EMULATE_INSTR)
        {
            STAM_COUNTER_INC(&pVM->csam.s.StatDangerousWrite);
            return VINF_EM_RAW_EMULATE_INSTR;
        }
        Assert(rcStrict == VERR_PATCH_NOT_FOUND);
    }

    VMCPU_FF_SET(pVCpu, VMCPU_FF_CSAM_PENDING_ACTION);

    /* Note that pvFault might be a different address in case of aliases. So use pvRange + offset instead!. */
    pVM->csam.s.pvDirtyBasePage[pVM->csam.s.cDirtyPages] = (RTRCPTR)((RTRCUINTPTR)pvRange + offRange);
    pVM->csam.s.pvDirtyFaultPage[pVM->csam.s.cDirtyPages] = (RTRCPTR)pvFault;
    if (++pVM->csam.s.cDirtyPages == CSAM_MAX_DIRTY_PAGES)
        return VINF_CSAM_PENDING_ACTION;

    /*
     * Make this particular page R/W. The VM_FF_CSAM_FLUSH_DIRTY_PAGE handler will reset it to readonly again.
     */
    Log(("csamRCCodePageWriteHandler: enabled r/w for page %RGv\n", pvFault));
    int rc = PGMShwMakePageWritable(pVCpu, pvFault, PGM_MK_PG_IS_WRITE_FAULT);
    AssertMsgRC(rc, ("PGMShwModifyPage -> rc=%Rrc\n", rc));
    ASMInvalidatePage((uintptr_t)pvFault);

    STAM_COUNTER_INC(&pVM->csam.s.StatCodePageModified);
    return VINF_SUCCESS;
}

