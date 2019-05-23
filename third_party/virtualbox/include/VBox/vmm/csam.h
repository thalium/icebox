/** @file
 * CSAM - Guest OS Code Scanning and Analyis Manager.
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
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef ___VBox_vmm_csam_h
#define ___VBox_vmm_csam_h

#include <VBox/types.h>

#if defined(VBOX_WITH_RAW_MODE) || defined(DOXYGEN_RUNNING)

/** @defgroup grp_csam      The Code Scanning and Analysis API
 * @ingroup grp_vmm
 * @{
 */

/**
 * CSAM monitoring tag.
 * For use with CSAMR3MonitorPage
 */
typedef enum CSAMTAG
{
    CSAM_TAG_INVALID = 0,
    CSAM_TAG_REM,
    CSAM_TAG_PATM,
    CSAM_TAG_CSAM,
    CSAM_TAG_32BIT_HACK = 0x7fffffff
} CSAMTAG;


RT_C_DECLS_BEGIN


/**
 * Query CSAM state (enabled/disabled)
 *
 * @returns true / false.
 * @param   a_pVM         The shared VM handle.
 * @internal
 */
#define CSAMIsEnabled(a_pVM) ((a_pVM)->fCSAMEnabled && EMIsRawRing0Enabled(a_pVM))

VMM_INT_DECL(bool)      CSAMDoesPageNeedScanning(PVM pVM, RTRCUINTPTR GCPtr);
VMM_INT_DECL(bool)      CSAMIsPageScanned(PVM pVM, RTRCPTR pPage);
VMM_INT_DECL(int)       CSAMMarkPage(PVM pVM, RTRCUINTPTR pPage, bool fScanned);
VMM_INT_DECL(void)      CSAMMarkPossibleCodePage(PVM pVM, RTRCPTR GCPtr);
VMM_INT_DECL(int)       CSAMEnableScanning(PVM pVM);
VMM_INT_DECL(int)       CSAMDisableScanning(PVM pVM);
VMM_INT_DECL(int)       CSAMExecFault(PVM pVM, RTRCPTR pvFault);
VMM_INT_DECL(bool)      CSAMIsKnownDangerousInstr(PVM pVM, RTRCUINTPTR GCPtr);


#ifdef IN_RING3
/** @defgroup grp_csam_r3      The CSAM ring-3 Context API
 * @{
 */

VMMR3DECL(bool)         CSAMR3IsEnabled(PUVM pUVM);
VMMR3DECL(int)          CSAMR3SetScanningEnabled(PUVM pUVM, bool fEnabled);

VMMR3_INT_DECL(int)     CSAMR3Init(PVM pVM);
VMMR3_INT_DECL(void)    CSAMR3Relocate(PVM pVM, RTGCINTPTR offDelta);
VMMR3_INT_DECL(int)     CSAMR3Term(PVM pVM);
VMMR3_INT_DECL(int)     CSAMR3Reset(PVM pVM);

VMMR3_INT_DECL(int)     CSAMR3FlushPage(PVM pVM, RTRCPTR addr);
VMMR3_INT_DECL(int)     CSAMR3RemovePage(PVM pVM, RTRCPTR addr);
VMMR3_INT_DECL(int)     CSAMR3CheckCode(PVM pVM, RTRCPTR pInstrGC);
VMMR3_INT_DECL(int)     CSAMR3CheckCodeEx(PVM pVM, PCPUMCTX pCtx, RTRCPTR pInstrGC);
VMMR3_INT_DECL(int)     CSAMR3MarkCode(PVM pVM, RTRCPTR pInstr, uint32_t cbInstr, bool fScanned);
VMMR3_INT_DECL(int)     CSAMR3DoPendingAction(PVM pVM, PVMCPU pVCpu);
VMMR3_INT_DECL(int)     CSAMR3CheckGates(PVM pVM, uint32_t iGate, uint32_t cGates);

VMMR3DECL(int)          CSAMR3MonitorPage(PVM pVM, RTRCPTR pPageAddrGC, CSAMTAG enmTag);
VMMR3DECL(int)          CSAMR3UnmonitorPage(PVM pVM, RTRCPTR pPageAddrGC, CSAMTAG enmTag);
VMMR3DECL(int)          CSAMR3RecordCallAddress(PVM pVM, RTRCPTR GCPtrCall);

/** @} */
#endif


/** @} */
RT_C_DECLS_END

#endif /* VBOX_WITH_RAW_MODE */

#endif

