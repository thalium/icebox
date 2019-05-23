/** @file
 * EM - Execution Monitor.
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

#ifndef ___VBox_vmm_em_h
#define ___VBox_vmm_em_h

#include <VBox/types.h>
#include <VBox/vmm/trpm.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_em        The Execution Monitor / Manager API
 * @ingroup grp_vmm
 * @{
 */

/** Enable to allow V86 code to run in raw mode. */
#define VBOX_RAW_V86

/**
 * The Execution Manager State.
 *
 * @remarks This is used in the saved state!
 */
typedef enum EMSTATE
{
    /** Not yet started. */
    EMSTATE_NONE = 1,
    /** Raw-mode execution. */
    EMSTATE_RAW,
    /** Hardware accelerated raw-mode execution. */
    EMSTATE_HM,
    /** Executing in IEM. */
    EMSTATE_IEM,
    /** Recompiled mode execution. */
    EMSTATE_REM,
    /** Execution is halted. (waiting for interrupt) */
    EMSTATE_HALTED,
    /** Application processor execution is halted. (waiting for startup IPI (SIPI)) */
    EMSTATE_WAIT_SIPI,
    /** Execution is suspended. */
    EMSTATE_SUSPENDED,
    /** The VM is terminating. */
    EMSTATE_TERMINATING,
    /** Guest debug event from raw-mode is being processed. */
    EMSTATE_DEBUG_GUEST_RAW,
    /** Guest debug event from hardware accelerated mode is being processed. */
    EMSTATE_DEBUG_GUEST_HM,
    /** Guest debug event from interpreted execution mode is being processed. */
    EMSTATE_DEBUG_GUEST_IEM,
    /** Guest debug event from recompiled-mode is being processed. */
    EMSTATE_DEBUG_GUEST_REM,
    /** Hypervisor debug event being processed. */
    EMSTATE_DEBUG_HYPER,
    /** The VM has encountered a fatal error. (And everyone is panicing....) */
    EMSTATE_GURU_MEDITATION,
    /** Executing in IEM, falling back on REM if we cannot switch back to HM or
     * RAW after a short while. */
    EMSTATE_IEM_THEN_REM,
    /** Just a hack to ensure that we get a 32-bit integer. */
    EMSTATE_MAKE_32BIT_HACK = 0x7fffffff
} EMSTATE;


/**
 * EMInterpretInstructionCPU execution modes.
 */
typedef enum
{
    /** Only supervisor code (CPL=0). */
    EMCODETYPE_SUPERVISOR,
    /** User-level code only. */
    EMCODETYPE_USER,
    /** Supervisor and user-level code (use with great care!). */
    EMCODETYPE_ALL,
    /** Just a hack to ensure that we get a 32-bit integer. */
    EMCODETYPE_32BIT_HACK = 0x7fffffff
} EMCODETYPE;

VMM_INT_DECL(EMSTATE)           EMGetState(PVMCPU pVCpu);
VMM_INT_DECL(void)              EMSetState(PVMCPU pVCpu, EMSTATE enmNewState);

/** @name Callback handlers for instruction emulation functions.
 * These are placed here because IOM wants to use them as well.
 * @{
 */
typedef DECLCALLBACK(uint32_t)  FNEMULATEPARAM2UINT32(void *pvParam1, uint64_t val2);
typedef FNEMULATEPARAM2UINT32  *PFNEMULATEPARAM2UINT32;
typedef DECLCALLBACK(uint32_t)  FNEMULATEPARAM2(void *pvParam1, size_t val2);
typedef FNEMULATEPARAM2        *PFNEMULATEPARAM2;
typedef DECLCALLBACK(uint32_t)  FNEMULATEPARAM3(void *pvParam1, uint64_t val2, size_t val3);
typedef FNEMULATEPARAM3        *PFNEMULATEPARAM3;
typedef DECLCALLBACK(int)       FNEMULATELOCKPARAM2(void *pvParam1, uint64_t val2, RTGCUINTREG32 *pf);
typedef FNEMULATELOCKPARAM2    *PFNEMULATELOCKPARAM2;
typedef DECLCALLBACK(int)       FNEMULATELOCKPARAM3(void *pvParam1, uint64_t val2, size_t cb, RTGCUINTREG32 *pf);
typedef FNEMULATELOCKPARAM3    *PFNEMULATELOCKPARAM3;
/** @}  */


/**
 * Checks if raw ring-3 execute mode is enabled.
 *
 * @returns true if enabled.
 * @returns false if disabled.
 * @param   pVM         The cross context VM structure.
 */
#define EMIsRawRing3Enabled(pVM)            (!(pVM)->fRecompileUser)

/**
 * Checks if raw ring-0 execute mode is enabled.
 *
 * @returns true if enabled.
 * @returns false if disabled.
 * @param   pVM         The cross context VM structure.
 */
#define EMIsRawRing0Enabled(pVM)            (!(pVM)->fRecompileSupervisor)

#ifdef VBOX_WITH_RAW_RING1
/**
 * Checks if raw ring-1 execute mode is enabled.
 *
 * @returns true if enabled.
 * @returns false if disabled.
 * @param   pVM         The cross context VM structure.
 */
# define EMIsRawRing1Enabled(pVM)           ((pVM)->fRawRing1Enabled)
#else
# define EMIsRawRing1Enabled(pVM)           false
#endif

/**
 * Checks if execution with hardware assisted virtualization is enabled.
 *
 * @returns true if enabled.
 * @returns false if disabled.
 * @param   pVM         The cross context VM structure.
 */
#define EMIsHwVirtExecutionEnabled(pVM)     (!(pVM)->fRecompileSupervisor && !(pVM)->fRecompileUser)

/**
 * Checks if execution of supervisor code should be done in the
 * recompiler or not.
 *
 * @returns true if enabled.
 * @returns false if disabled.
 * @param   pVM         The cross context VM structure.
 */
#define EMIsSupervisorCodeRecompiled(pVM) ((pVM)->fRecompileSupervisor)

VMMDECL(void)                   EMSetInhibitInterruptsPC(PVMCPU pVCpu, RTGCUINTPTR PC);
VMMDECL(RTGCUINTPTR)            EMGetInhibitInterruptsPC(PVMCPU pVCpu);
VMM_INT_DECL(int)               EMInterpretDisasCurrent(PVM pVM, PVMCPU pVCpu, PDISCPUSTATE pCpu, unsigned *pcbInstr);
VMM_INT_DECL(int)               EMInterpretDisasOneEx(PVM pVM, PVMCPU pVCpu, RTGCUINTPTR GCPtrInstr, PCCPUMCTXCORE pCtxCore,
                                                      PDISCPUSTATE pDISState, unsigned *pcbInstr);
VMM_INT_DECL(VBOXSTRICTRC)      EMInterpretInstruction(PVMCPU pVCpu, PCPUMCTXCORE pCoreCtx, RTGCPTR pvFault);
VMM_INT_DECL(VBOXSTRICTRC)      EMInterpretInstructionEx(PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbWritten);
VMM_INT_DECL(VBOXSTRICTRC)      EMInterpretInstructionDisasState(PVMCPU pVCpu, PDISCPUSTATE pDis, PCPUMCTXCORE pCoreCtx,
                                                                 RTGCPTR pvFault, EMCODETYPE enmCodeType);

#ifdef IN_RC
VMM_INT_DECL(int)               EMInterpretIretV86ForPatm(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame);
#endif

VMM_INT_DECL(int)               EMInterpretCpuId(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame);
VMM_INT_DECL(int)               EMInterpretRdtsc(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame);
VMM_INT_DECL(int)               EMInterpretRdpmc(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame);
VMM_INT_DECL(int)               EMInterpretRdtscp(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx);
VMM_INT_DECL(VBOXSTRICTRC)      EMInterpretInvlpg(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, RTGCPTR pAddrGC);
VMM_INT_DECL(VBOXSTRICTRC)      EMInterpretMWait(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame);
VMM_INT_DECL(int)               EMInterpretMonitor(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame);
VMM_INT_DECL(int)               EMInterpretDRxWrite(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, uint32_t DestRegDrx, uint32_t SrcRegGen);
VMM_INT_DECL(int)               EMInterpretDRxRead(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, uint32_t DestRegGen, uint32_t SrcRegDrx);
VMM_INT_DECL(int)               EMInterpretRdmsr(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame);
VMM_INT_DECL(int)               EMInterpretWrmsr(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame);
VMM_INT_DECL(bool)              EMShouldContinueAfterHalt(PVMCPU pVCpu, PCPUMCTX pCtx);
VMM_INT_DECL(bool)              EMMonitorWaitShouldContinue(PVMCPU pVCpu, PCPUMCTX pCtx);
VMM_INT_DECL(int)               EMMonitorWaitPrepare(PVMCPU pVCpu, uint64_t rax, uint64_t rcx, uint64_t rdx, RTGCPHYS GCPhys);
VMM_INT_DECL(bool)              EMMonitorIsArmed(PVMCPU pVCpu);
VMM_INT_DECL(int)               EMMonitorWaitPerform(PVMCPU pVCpu, uint64_t rax, uint64_t rcx);
VMM_INT_DECL(int)               EMUnhaltAndWakeUp(PVM pVM, PVMCPU pVCpuDst);

/** @name Assembly routines
 * @{ */
VMMDECL(uint32_t)   EMEmulateCmp(uint32_t u32Param1, uint64_t u64Param2, size_t cb);
VMMDECL(uint32_t)   EMEmulateAnd(void *pvParam1, uint64_t u64Param2, size_t cb);
VMMDECL(uint32_t)   EMEmulateInc(void *pvParam1, size_t cb);
VMMDECL(uint32_t)   EMEmulateDec(void *pvParam1, size_t cb);
VMMDECL(uint32_t)   EMEmulateOr(void *pvParam1, uint64_t u64Param2, size_t cb);
VMMDECL(int)        EMEmulateLockOr(void *pvParam1, uint64_t u64Param2, size_t cbSize, RTGCUINTREG32 *pf);
VMMDECL(uint32_t)   EMEmulateXor(void *pvParam1, uint64_t u64Param2, size_t cb);
VMMDECL(int)        EMEmulateLockXor(void *pvParam1, uint64_t u64Param2, size_t cbSize, RTGCUINTREG32 *pf);
VMMDECL(uint32_t)   EMEmulateAdd(void *pvParam1, uint64_t u64Param2, size_t cb);
VMMDECL(int)        EMEmulateLockAnd(void *pvParam1, uint64_t u64Param2, size_t cbSize, RTGCUINTREG32 *pf);
VMMDECL(uint32_t)   EMEmulateSub(void *pvParam1, uint64_t u64Param2, size_t cb);
VMMDECL(uint32_t)   EMEmulateAdcWithCarrySet(void *pvParam1, uint64_t u64Param2, size_t cb);
VMMDECL(uint32_t)   EMEmulateBtr(void *pvParam1, uint64_t u64Param2);
VMMDECL(int)        EMEmulateLockBtr(void *pvParam1, uint64_t u64Param2, RTGCUINTREG32 *pf);
VMMDECL(uint32_t)   EMEmulateBts(void *pvParam1, uint64_t u64Param2);
VMMDECL(uint32_t)   EMEmulateBtc(void *pvParam1, uint64_t u64Param2);
VMMDECL(uint32_t)   EMEmulateCmpXchg(void *pvParam1, uint64_t *pu32Param2, uint64_t u32Param3, size_t cbSize);
VMMDECL(uint32_t)   EMEmulateLockCmpXchg(void *pvParam1, uint64_t *pu64Param2, uint64_t u64Param3, size_t cbSize);
VMMDECL(uint32_t)   EMEmulateCmpXchg8b(void *pu32Param1, uint32_t *pEAX, uint32_t *pEDX, uint32_t uEBX, uint32_t uECX);
VMMDECL(uint32_t)   EMEmulateLockCmpXchg8b(void *pu32Param1, uint32_t *pEAX, uint32_t *pEDX, uint32_t uEBX, uint32_t uECX);
VMMDECL(uint32_t)   EMEmulateXAdd(void *pvParam1, void *pvParam2, size_t cbOp);
VMMDECL(uint32_t)   EMEmulateLockXAdd(void *pvParam1, void *pvParam2, size_t cbOp);
/** @} */

/** @name REM locking routines
 * @{ */
VMMDECL(void)                   EMRemUnlock(PVM pVM);
VMMDECL(void)                   EMRemLock(PVM pVM);
VMMDECL(bool)                   EMRemIsLockOwner(PVM pVM);
VMM_INT_DECL(int)               EMRemTryLock(PVM pVM);
/** @} */


/** @name EM_ONE_INS_FLAGS_XXX - flags for EMR3HmSingleInstruction (et al).
 * @{ */
/** Return when CS:RIP changes or some other important event happens.
 * This means running whole REP and LOOP $ sequences for instance. */
#define EM_ONE_INS_FLAGS_RIP_CHANGE     RT_BIT_32(0)
/** Mask of valid flags. */
#define EM_ONE_INS_FLAGS_MASK           UINT32_C(0x00000001)
/** @} */


#ifdef IN_RING3
/** @defgroup grp_em_r3     The EM Host Context Ring-3 API
 * @{
 */

/**
 * Command argument for EMR3RawSetMode().
 *
 * It's possible to extend this interface to change several
 * execution modes at once should the need arise.
 */
typedef enum EMEXECPOLICY
{
    /** The customary invalid zero entry. */
    EMEXECPOLICY_INVALID = 0,
    /** Whether to recompile ring-0 code or execute it in raw/hm. */
    EMEXECPOLICY_RECOMPILE_RING0,
    /** Whether to recompile ring-3 code or execute it in raw/hm. */
    EMEXECPOLICY_RECOMPILE_RING3,
    /** Whether to only use IEM for execution. */
    EMEXECPOLICY_IEM_ALL,
    /** End of valid value (not included). */
    EMEXECPOLICY_END,
    /** The customary 32-bit type blowup. */
    EMEXECPOLICY_32BIT_HACK = 0x7fffffff
} EMEXECPOLICY;
VMMR3DECL(int)                  EMR3SetExecutionPolicy(PUVM pUVM, EMEXECPOLICY enmPolicy, bool fEnforce);
VMMR3DECL(int)                  EMR3QueryExecutionPolicy(PUVM pUVM, EMEXECPOLICY enmPolicy, bool *pfEnforced);

VMMR3_INT_DECL(int)             EMR3Init(PVM pVM);
VMMR3_INT_DECL(void)            EMR3Relocate(PVM pVM);
VMMR3_INT_DECL(void)            EMR3ResetCpu(PVMCPU pVCpu);
VMMR3_INT_DECL(void)            EMR3Reset(PVM pVM);
VMMR3_INT_DECL(int)             EMR3Term(PVM pVM);
VMMR3DECL(DECLNORETURN(void))   EMR3FatalError(PVMCPU pVCpu, int rc);
VMMR3_INT_DECL(int)             EMR3ExecuteVM(PVM pVM, PVMCPU pVCpu);
VMMR3_INT_DECL(int)             EMR3CheckRawForcedActions(PVM pVM, PVMCPU pVCpu);
VMMR3_INT_DECL(int)             EMR3NotifyResume(PVM pVM);
VMMR3_INT_DECL(int)             EMR3NotifySuspend(PVM pVM);
VMMR3_INT_DECL(VBOXSTRICTRC)    EMR3HmSingleInstruction(PVM pVM, PVMCPU pVCpu, uint32_t fFlags);

/** @} */
#endif /* IN_RING3 */

/** @} */

RT_C_DECLS_END

#endif

