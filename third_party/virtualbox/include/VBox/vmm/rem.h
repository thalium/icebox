/** @file
 * REM - The Recompiled Execution Manager.
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

#ifndef ___VBox_vmm_rem_h
#define ___VBox_vmm_rem_h

#include <VBox/types.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/vmapi.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rem      The Recompiled Execution Manager API
 * @ingroup grp_vmm
 * @{
 */

/** No pending interrupt. */
#define REM_NO_PENDING_IRQ          (~(uint32_t)0)


#if defined(IN_RING0) || defined(IN_RC)
VMMDECL(void) REMNotifyInvalidatePage(PVM pVM, RTGCPTR GCPtrPage);
VMMDECL(void) REMNotifyHandlerPhysicalRegister(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb, bool fHasHCHandler);
VMMDECL(void) REMNotifyHandlerPhysicalDeregister(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb, bool fHasHCHandler, bool fRestoreAsRAM);
VMMDECL(void) REMNotifyHandlerPhysicalModify(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhysOld, RTGCPHYS GCPhysNew, RTGCPHYS cb, bool fHasHCHandler, bool fRestoreAsRAM);
#endif /* IN_RING0 || IN_RC */
#ifdef IN_RC
VMMDECL(void) REMNotifyHandlerPhysicalFlushIfAlmostFull(PVM pVM, PVMCPU pVCpu);
#endif
VMMDECL(void) REMFlushTBs(PVM pVM);


#ifdef IN_RING3
/** @defgroup grp_rem_r3   REM Host Context Ring 3 API
 * @{
 */
REMR3DECL(int)  REMR3Init(PVM pVM);
REMR3DECL(int)  REMR3InitFinalize(PVM pVM);
REMR3DECL(int)  REMR3Term(PVM pVM);
REMR3DECL(void) REMR3Reset(PVM pVM);
REMR3DECL(int)  REMR3Run(PVM pVM, PVMCPU pVCpu);
REMR3DECL(int)  REMR3EmulateInstruction(PVM pVM, PVMCPU pVCpu);
REMR3DECL(int)  REMR3Step(PVM pVM, PVMCPU pVCpu);
REMR3DECL(int)  REMR3BreakpointSet(PVM pVM, RTGCUINTPTR Address);
REMR3DECL(int)  REMR3BreakpointClear(PVM pVM, RTGCUINTPTR Address);
REMR3DECL(int)  REMR3State(PVM pVM, PVMCPU pVCpu);
REMR3DECL(int)  REMR3StateBack(PVM pVM, PVMCPU pVCpu);
REMR3DECL(void) REMR3StateUpdate(PVM pVM, PVMCPU pVCpu);
REMR3DECL(void) REMR3A20Set(PVM pVM, PVMCPU pVCpu, bool fEnable);
REMR3DECL(int)  REMR3DisasEnableStepping(PVM pVM, bool fEnable);
REMR3DECL(void) REMR3ReplayHandlerNotifications(PVM pVM);
REMR3DECL(int)  REMR3NotifyCodePageChanged(PVM pVM, PVMCPU pVCpu, RTGCPTR pvCodePage);
REMR3DECL(void) REMR3NotifyPhysRamRegister(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, unsigned fFlags);
/** @name Flags for REMR3NotifyPhysRamRegister.
 * @{ */
#define REM_NOTIFY_PHYS_RAM_FLAGS_RAM    RT_BIT(16)
#define REM_NOTIFY_PHYS_RAM_FLAGS_MMIO2  RT_BIT(17)
/** @} */
REMR3DECL(void) REMR3NotifyPhysRomRegister(PVM pVM, RTGCPHYS GCPhys, RTUINT cb, void *pvCopy, bool fShadow);
REMR3DECL(void) REMR3NotifyPhysRamDeregister(PVM pVM, RTGCPHYS GCPhys, RTUINT cb);
REMR3DECL(void) REMR3NotifyHandlerPhysicalRegister(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb, bool fHasHCHandler);
REMR3DECL(void) REMR3NotifyHandlerPhysicalDeregister(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb, bool fHasHCHandler, bool fRestoreAsRAM);
REMR3DECL(void) REMR3NotifyHandlerPhysicalModify(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhysOld, RTGCPHYS GCPhysNew, RTGCPHYS cb, bool fHasHCHandler, bool fRestoreAsRAM);
REMR3DECL(void) REMR3NotifyInterruptSet(PVM pVM, PVMCPU pVCpu);
REMR3DECL(void) REMR3NotifyInterruptClear(PVM pVM, PVMCPU pVCpu);
REMR3DECL(void) REMR3NotifyTimerPending(PVM pVM, PVMCPU pVCpuDst);
REMR3DECL(void) REMR3NotifyDmaPending(PVM pVM);
REMR3DECL(void) REMR3NotifyQueuePending(PVM pVM);
REMR3DECL(void) REMR3NotifyFF(PVM pVM);
REMR3DECL(bool) REMR3IsPageAccessHandled(PVM pVM, RTGCPHYS GCPhys);
/** @} */
#endif /* IN_RING3 */


/** @} */
RT_C_DECLS_END


#endif

