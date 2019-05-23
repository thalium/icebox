/* $Id: HMSVMR0.h $ */
/** @file
 * HM SVM (AMD-V) - Internal header file.
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

#ifndef ___HMSVMR0_h
#define ___HMSVMR0_h

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/stam.h>
#include <VBox/dis.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/hm_svm.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_svm_int  Internal
 * @ingroup grp_svm
 * @internal
 * @{
 */

#ifdef IN_RING0

VMMR0DECL(int)  SVMR0GlobalInit(void);
VMMR0DECL(void) SVMR0GlobalTerm(void);
VMMR0DECL(int)  SVMR0Enter(PVM pVM, PVMCPU pVCpu, PHMGLOBALCPUINFO pCpu);
VMMR0DECL(void) SVMR0ThreadCtxCallback(RTTHREADCTXEVENT enmEvent, PVMCPU pVCpu, bool fGlobalInit);
VMMR0DECL(int)  SVMR0EnableCpu(PHMGLOBALCPUINFO pCpu, PVM pVM, void *pvPageCpu, RTHCPHYS HCPhysCpuPage, bool fEnabledBySystem,
                               void *pvArg);
VMMR0DECL(int)  SVMR0DisableCpu(PHMGLOBALCPUINFO pCpu, void *pvPageCpu, RTHCPHYS pPageCpuPhys);
VMMR0DECL(int)  SVMR0InitVM(PVM pVM);
VMMR0DECL(int)  SVMR0TermVM(PVM pVM);
VMMR0DECL(int)  SVMR0SetupVM(PVM pVM);
VMMR0DECL(VBOXSTRICTRC) SVMR0RunGuestCode(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx);
VMMR0DECL(int)  SVMR0SaveHostState(PVM pVM, PVMCPU pVCpu);

#if HC_ARCH_BITS == 32 && defined(VBOX_WITH_64_BITS_GUESTS)
DECLASM(int)   SVMR0VMSwitcherRun64(RTHCPHYS pVMCBHostPhys, RTHCPHYS pVMCBPhys, PCPUMCTX pCtx, PVM pVM, PVMCPU pVCpu);
VMMR0DECL(int) SVMR0Execute64BitsHandler(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, HM64ON32OP enmOp, uint32_t cbParam,
                                         uint32_t *paParam);
#endif /* HC_ARCH_BITS == 32 && defined(VBOX_WITH_64_BITS_GUESTS) */

/**
 * Prepares for and executes VMRUN (32-bit guests).
 *
 * @returns VBox status code.
 * @param   pVMCBHostPhys   Physical address of host VMCB.
 * @param   pVMCBPhys       Physical address of the VMCB.
 * @param   pCtx            Pointer to the guest CPU context.
 * @param   pVM             The cross context VM structure. (Not used.)
 * @param   pVCpu           The cross context virtual CPU structure. (Not used.)
 */
DECLASM(int) SVMR0VMRun(RTHCPHYS pVMCBHostPhys, RTHCPHYS pVMCBPhys, PCPUMCTX pCtx, PVM pVM, PVMCPU pVCpu);


/**
 * Prepares for and executes VMRUN (64-bit guests).
 *
 * @returns VBox status code.
 * @param   pVMCBHostPhys   Physical address of host VMCB.
 * @param   pVMCBPhys       Physical address of the VMCB.
 * @param   pCtx            Pointer to the guest CPU context.
 * @param   pVM             The cross context VM structure. (Not used.)
 * @param   pVCpu           The cross context virtual CPU structure. (Not used.)
 */
DECLASM(int) SVMR0VMRun64(RTHCPHYS pVMCBHostPhys, RTHCPHYS pVMCBPhys, PCPUMCTX pCtx, PVM pVM, PVMCPU pVCpu);

/**
 * Executes INVLPGA.
 *
 * @param   pPageGC         Virtual page to invalidate.
 * @param   u32ASID         Tagged TLB id.
 */
DECLASM(void) SVMR0InvlpgA(RTGCPTR pPageGC, uint32_t u32ASID);

#endif /* IN_RING0 */

/** @} */

RT_C_DECLS_END

#endif /* !___HMSVMR0_h */

