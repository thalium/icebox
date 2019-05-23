/** @file
 * PATM - Dynamic Guest OS Patching Manager.
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

#ifndef ___VBox_vmm_patm_h
#define ___VBox_vmm_patm_h

#include <VBox/types.h>
#include <VBox/dis.h>

#if defined(VBOX_WITH_RAW_MODE) || defined(DOXYGEN_RUNNING)

RT_C_DECLS_BEGIN

/** @defgroup grp_patm      The Patch Manager API
 * @ingroup grp_vmm
 * @{
 */
#define MAX_PATCHES          512

/**
 * Flags for specifying the type of patch to install with PATMR3InstallPatch
 * @{
 */
#define PATMFL_CODE32                       RT_BIT_64(0)
#define PATMFL_INTHANDLER                   RT_BIT_64(1)
#define PATMFL_SYSENTER                     RT_BIT_64(2)
#define PATMFL_GUEST_SPECIFIC               RT_BIT_64(3)
#define PATMFL_USER_MODE                    RT_BIT_64(4)
#define PATMFL_IDTHANDLER                   RT_BIT_64(5)
#define PATMFL_TRAPHANDLER                  RT_BIT_64(6)
#define PATMFL_DUPLICATE_FUNCTION           RT_BIT_64(7)
#define PATMFL_REPLACE_FUNCTION_CALL        RT_BIT_64(8)
#define PATMFL_TRAPHANDLER_WITH_ERRORCODE   RT_BIT_64(9)
#define PATMFL_INTHANDLER_WITH_ERRORCODE    (PATMFL_TRAPHANDLER_WITH_ERRORCODE)
#define PATMFL_MMIO_ACCESS                  RT_BIT_64(10)
/* no more room -> change PATMInternal.h if more is needed!! */

/*
 * Flags above 1024 are reserved for internal use!
 */
/** @} */

/** Enable to activate sysenter emulation in GC. */
/* #define PATM_EMULATE_SYSENTER */

/**
 * Maximum number of cached VGA writes
 */
#define MAX_VGA_WRITE_CACHE    64

typedef struct PATMGCSTATE
{
    /** Virtual Flags register (IF + more later on) */
    uint32_t  uVMFlags;

    /** Pending PATM actions (internal use only) */
    uint32_t  uPendingAction;

    /** Records the number of times all patches are called (indicating how many exceptions we managed to avoid) */
    uint32_t  uPatchCalls;
    /** Scratchpad dword */
    uint32_t  uScratch;
    /** Debugging info */
    uint32_t  uIretEFlags, uIretCS, uIretEIP;

    /** PATM stack pointer */
    uint32_t  Psp;

    /** PATM interrupt flag */
    uint32_t  fPIF;
    /** PATM inhibit irq address (used by sti) */
    RTRCPTR   GCPtrInhibitInterrupts;

    /** Scratch room for call patch */
    RTRCPTR   GCCallPatchTargetAddr;
    RTRCPTR   GCCallReturnAddr;

    /** Temporary storage for guest registers. */
    struct
    {
        uint32_t    uEAX;
        uint32_t    uECX;
        uint32_t    uEDI;
        uint32_t    eFlags;
        uint32_t    uFlags;
    } Restore;
} PATMGCSTATE, *PPATMGCSTATE;

typedef struct PATMTRAPREC
{
    /** pointer to original guest code instruction (for emulation) */
    RTRCPTR pNewEIP;
    /** pointer to the next guest code instruction */
    RTRCPTR pNextInstr;
    /** pointer to the corresponding next instruction in the patch block */
    RTRCPTR pNextPatchInstr;
} PATMTRAPREC, *PPATMTRAPREC;


/**
 * Translation state (currently patch to GC ptr)
 */
typedef enum
{
    PATMTRANS_FAILED,
    PATMTRANS_SAFE,          /**< Safe translation */
    PATMTRANS_PATCHSTART,    /**< Instruction starts a patch block */
    PATMTRANS_OVERWRITTEN,   /**< Instruction overwritten by patchjump */
    PATMTRANS_INHIBITIRQ     /**< Instruction must be executed due to instruction fusing */
} PATMTRANSSTATE;


/**
 * Query PATM state (enabled/disabled)
 *
 * @returns 0 - disabled, 1 - enabled
 * @param   a_pVM       The VM to operate on.
 * @internal
 */
#define PATMIsEnabled(a_pVM)    ((a_pVM)->fPATMEnabled)

VMMDECL(bool)           PATMIsPatchGCAddr(PVM pVM, RTRCUINTPTR uGCAddr);
VMMDECL(bool)           PATMIsPatchGCAddrExclHelpers(PVM pVM, RTRCUINTPTR uGCAddr);
VMM_INT_DECL(int)       PATMReadPatchCode(PVM pVM, RTGCPTR GCPtrPatchCode, void *pvDst, size_t cbToRead, size_t *pcbRead);

VMM_INT_DECL(void)      PATMRawEnter(PVM pVM, PCPUMCTX pCtx);
VMM_INT_DECL(void)      PATMRawLeave(PVM pVM, PCPUMCTX pCtx, int rawRC);
VMM_INT_DECL(uint32_t)  PATMRawGetEFlags(PVM pVM, PCCPUMCTX pCtx);
VMM_INT_DECL(void)      PATMRawSetEFlags(PVM pVM, PCPUMCTX pCtx, uint32_t efl);
VMM_INT_DECL(RCPTRTYPE(PPATMGCSTATE)) PATMGetGCState(PVM pVM);
VMM_INT_DECL(bool)      PATMShouldUseRawMode(PVM pVM, RTRCPTR pAddrGC);
VMM_INT_DECL(int)       PATMSetMMIOPatchInfo(PVM pVM, RTGCPHYS GCPhys, RTRCPTR pCachedData);

VMM_INT_DECL(bool)      PATMIsInt3Patch(PVM pVM, RTRCPTR pInstrGC, uint32_t *pOpcode, uint32_t *pSize);
VMM_INT_DECL(bool)      PATMAreInterruptsEnabled(PVM pVM);
VMM_INT_DECL(bool)      PATMAreInterruptsEnabledByCtx(PVM pVM, PCPUMCTX pCtx);
#ifdef PATM_EMULATE_SYSENTER
VMM_INT_DECL(int)       PATMSysCall(PVM pVM, PCPUMCTX pCtx, PDISCPUSTATE pCpu);
#endif

#ifdef IN_RC
/** @defgroup grp_patm_rc    The Patch Manager Raw-mode Context API
 * @{
 */

VMMRC_INT_DECL(int)             PATMRCHandleInt3PatchTrap(PVM pVM, PCPUMCTXCORE pRegFrame);
VMMRC_INT_DECL(VBOXSTRICTRC)    PATMRCHandleWriteToPatchPage(PVM pVM, PCPUMCTXCORE pRegFrame, RTRCPTR GCPtr, uint32_t cbWrite);
VMMRC_INT_DECL(int)             PATMRCHandleIllegalInstrTrap(PVM pVM, PCPUMCTXCORE pRegFrame);

/** @} */

#endif

#ifdef IN_RING3
/** @defgroup grp_patm_r3    The Patch Manager Host Ring-3 Context API
 * @{
 */

VMMR3DECL(int)                  PATMR3AllowPatching(PUVM pUVM, bool fAllowPatching);
VMMR3DECL(bool)                 PATMR3IsEnabled(PUVM pUVM);

VMMR3_INT_DECL(int)             PATMR3Init(PVM pVM);
VMMR3_INT_DECL(int)             PATMR3InitFinalize(PVM pVM);
VMMR3_INT_DECL(void)            PATMR3Relocate(PVM pVM, RTRCINTPTR offDelta);
VMMR3_INT_DECL(int)             PATMR3Term(PVM pVM);
VMMR3_INT_DECL(int)             PATMR3Reset(PVM pVM);

VMMR3_INT_DECL(bool)            PATMR3IsInsidePatchJump(PVM pVM, RTRCPTR pAddr, PRTGCPTR32 pPatchAddr);
VMMR3_INT_DECL(RTRCPTR)         PATMR3QueryPatchGCPtr(PVM pVM, RTRCPTR pAddrGC);
VMMR3_INT_DECL(void *)          PATMR3GCPtrToHCPtr(PVM pVM, RTRCPTR pAddrGC);
VMMR3_INT_DECL(PPATMGCSTATE)    PATMR3QueryGCStateHC(PVM pVM);
VMMR3_INT_DECL(int)             PATMR3HandleTrap(PVM pVM, PCPUMCTX pCtx, RTRCPTR pEip, RTGCPTR *ppNewEip);
VMMR3_INT_DECL(int)             PATMR3HandleMonitoredPage(PVM pVM);
VMMR3_INT_DECL(int)             PATMR3PatchWrite(PVM pVM, RTRCPTR GCPtr, uint32_t cbWrite);
VMMR3_INT_DECL(int)             PATMR3FlushPage(PVM pVM, RTRCPTR addr);
VMMR3_INT_DECL(int)             PATMR3InstallPatch(PVM pVM, RTRCPTR pInstrGC, uint64_t flags);
VMMR3_INT_DECL(int)             PATMR3AddHint(PVM pVM, RTRCPTR pInstrGC, uint32_t flags);
VMMR3_INT_DECL(int)             PATMR3DuplicateFunctionRequest(PVM pVM, PCPUMCTX pCtx);
VMMR3_INT_DECL(RTRCPTR)         PATMR3PatchToGCPtr(PVM pVM, RTRCPTR pPatchGC, PATMTRANSSTATE *pEnmState);
VMMR3DECL(int)                  PATMR3QueryOpcode(PVM pVM, RTRCPTR pInstrGC, uint8_t *pByte);
VMMR3_INT_DECL(int)             PATMR3ReadOrgInstr(PVM pVM, RTGCPTR32 GCPtrInstr, uint8_t *pbDst, size_t cbToRead, size_t *pcbRead);
VMMR3_INT_DECL(int)             PATMR3DisablePatch(PVM pVM, RTRCPTR pInstrGC);
VMMR3_INT_DECL(int)             PATMR3EnablePatch(PVM pVM, RTRCPTR pInstrGC);
VMMR3_INT_DECL(int)             PATMR3RemovePatch(PVM pVM, RTRCPTR pInstrGC);
VMMR3_INT_DECL(int)             PATMR3DetectConflict(PVM pVM, RTRCPTR pInstrGC, RTRCPTR pConflictGC);
VMMR3_INT_DECL(bool)            PATMR3HasBeenPatched(PVM pVM, RTRCPTR pInstrGC);

VMMR3_INT_DECL(void)            PATMR3DbgPopulateAddrSpace(PVM pVM, RTDBGAS hDbgAs);
VMMR3_INT_DECL(void)            PATMR3DbgAnnotatePatchedInstruction(PVM pVM, RTRCPTR RCPtr, uint8_t cbInstr,
                                                                    char *pszBuf, size_t cbBuf);

/** @} */
#endif


/** @} */
RT_C_DECLS_END

#endif /* VBOX_WITH_RAW_MODE */

#endif
