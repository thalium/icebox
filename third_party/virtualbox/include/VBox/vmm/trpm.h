/** @file
 * TRPM - The Trap Monitor.
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

#ifndef ___VBox_vmm_trpm_h
#define ___VBox_vmm_trpm_h

#include <VBox/types.h>
#include <iprt/x86.h>


RT_C_DECLS_BEGIN
/** @defgroup grp_trpm The Trap Monitor API
 * @ingroup grp_vmm
 * @{
 */

/**
 * Trap: error code present or not
 */
typedef enum
{
    TRPM_TRAP_HAS_ERRORCODE = 0,
    TRPM_TRAP_NO_ERRORCODE,
    /** The usual 32-bit paranoia. */
    TRPM_TRAP_32BIT_HACK = 0x7fffffff
} TRPMERRORCODE;

/**
 * TRPM event type
 */
/** Note: must match trpm.mac! */
typedef enum
{
    TRPM_TRAP         = 0,
    TRPM_HARDWARE_INT = 1,
    TRPM_SOFTWARE_INT = 2,
    /** The usual 32-bit paranoia. */
    TRPM_32BIT_HACK   = 0x7fffffff
} TRPMEVENT;
/** Pointer to a TRPM event type. */
typedef TRPMEVENT *PTRPMEVENT;
/** Pointer to a const TRPM event type. */
typedef TRPMEVENT const *PCTRPMEVENT;

/**
 * Invalid trap handler for trampoline calls
 */
#define TRPM_INVALID_HANDLER        0

VMMDECL(int)        TRPMQueryTrap(PVMCPU pVCpu, uint8_t *pu8TrapNo, PTRPMEVENT penmType);
VMMDECL(uint8_t)    TRPMGetTrapNo(PVMCPU pVCpu);
VMMDECL(RTGCUINT)   TRPMGetErrorCode(PVMCPU pVCpu);
VMMDECL(RTGCUINTPTR) TRPMGetFaultAddress(PVMCPU pVCpu);
VMMDECL(uint8_t)    TRPMGetInstrLength(PVMCPU pVCpu);
VMMDECL(int)        TRPMResetTrap(PVMCPU pVCpu);
VMMDECL(int)        TRPMAssertTrap(PVMCPU pVCpu, uint8_t u8TrapNo, TRPMEVENT enmType);
VMMDECL(int)        TRPMAssertXcptPF(PVMCPU pVCpu, RTGCUINTPTR uCR2, RTGCUINT uErrorCode);
VMMDECL(void)       TRPMSetErrorCode(PVMCPU pVCpu, RTGCUINT uErrorCode);
VMMDECL(void)       TRPMSetFaultAddress(PVMCPU pVCpu, RTGCUINTPTR uCR2);
VMMDECL(void)       TRPMSetInstrLength(PVMCPU pVCpu, uint8_t cbInstr);
VMMDECL(bool)       TRPMIsSoftwareInterrupt(PVMCPU pVCpu);
VMMDECL(bool)       TRPMHasTrap(PVMCPU pVCpu);
VMMDECL(int)        TRPMQueryTrapAll(PVMCPU pVCpu, uint8_t *pu8TrapNo, PTRPMEVENT pEnmType, PRTGCUINT puErrorCode, PRTGCUINTPTR puCR2, uint8_t *pcbInstr);
VMMDECL(void)       TRPMSaveTrap(PVMCPU pVCpu);
VMMDECL(void)       TRPMRestoreTrap(PVMCPU pVCpu);
VMMDECL(int)        TRPMForwardTrap(PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, uint32_t iGate, uint32_t cbInstr, TRPMERRORCODE enmError, TRPMEVENT enmType, int32_t iOrgTrap);
VMMDECL(int)        TRPMRaiseXcpt(PVMCPU pVCpu, PCPUMCTXCORE pCtxCore, X86XCPT enmXcpt);
VMMDECL(int)        TRPMRaiseXcptErr(PVMCPU pVCpu, PCPUMCTXCORE pCtxCore, X86XCPT enmXcpt, uint32_t uErr);
VMMDECL(int)        TRPMRaiseXcptErrCR2(PVMCPU pVCpu, PCPUMCTXCORE pCtxCore, X86XCPT enmXcpt, uint32_t uErr, RTGCUINTPTR uCR2);


#ifdef IN_RING3
/** @defgroup grp_trpm_r3   TRPM Host Context Ring 3 API
 * @{
 */
VMMR3DECL(int)      TRPMR3Init(PVM pVM);
VMMR3DECL(void)     TRPMR3Relocate(PVM pVM, RTGCINTPTR offDelta);
VMMR3DECL(void)     TRPMR3ResetCpu(PVMCPU pVCpu);
VMMR3DECL(void)     TRPMR3Reset(PVM pVM);
VMMR3DECL(int)      TRPMR3Term(PVM pVM);
VMMR3DECL(int)      TRPMR3InjectEvent(PVM pVM, PVMCPU pVCpu, TRPMEVENT enmEvent);
# ifdef VBOX_WITH_RAW_MODE
VMMR3_INT_DECL(int) TRPMR3GetImportRC(PVM pVM, const char *pszSymbol, PRTRCPTR pRCPtrValue);
VMMR3DECL(int)      TRPMR3SyncIDT(PVM pVM, PVMCPU pVCpu);
VMMR3DECL(bool)     TRPMR3IsGateHandler(PVM pVM, RTRCPTR GCPtr);
VMMR3DECL(uint32_t) TRPMR3QueryGateByHandler(PVM pVM, RTRCPTR GCPtr);
VMMR3DECL(int)      TRPMR3EnableGuestTrapHandler(PVM pVM, unsigned iTrap);
VMMR3DECL(int)      TRPMR3SetGuestTrapHandler(PVM pVM, unsigned iTrap, RTRCPTR pHandler);
VMMR3DECL(RTRCPTR)  TRPMR3GetGuestTrapHandler(PVM pVM, unsigned iTrap);
# endif
/** @} */
#endif


#ifdef IN_RC
/** @defgroup grp_trpm_rc    The TRPM Raw-mode Context API
 * @{
 */

/**
 * Guest Context temporary trap handler
 *
 * @returns VBox status code (appropriate for GC return).
 *          In this context VINF_SUCCESS means to restart the instruction.
 * @param   pVM         The cross context VM structure.
 * @param   pRegFrame   Trap register frame.
 */
typedef DECLCALLBACK(int) FNTRPMGCTRAPHANDLER(PVM pVM, PCPUMCTXCORE pRegFrame);
/** Pointer to a TRPMGCTRAPHANDLER() function. */
typedef FNTRPMGCTRAPHANDLER *PFNTRPMGCTRAPHANDLER;

VMMRCDECL(int)      TRPMGCSetTempHandler(PVM pVM, unsigned iTrap, PFNTRPMGCTRAPHANDLER pfnHandler);
VMMRCDECL(void)     TRPMGCHyperReturnToHost(PVM pVM, int rc);
/** @} */
#endif


#ifdef IN_RING0
/** @defgroup grp_trpm_r0   TRPM Host Context Ring 0 API
 * @{
 */
VMMR0DECL(void)     TRPMR0DispatchHostInterrupt(PVM pVM);
VMMR0DECL(void)     TRPMR0SetupInterruptDispatcherFrame(PVM pVM, void *pvRet);
/** @} */
#endif

/** @} */
RT_C_DECLS_END

#endif
