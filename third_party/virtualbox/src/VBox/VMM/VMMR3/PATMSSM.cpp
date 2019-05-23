/* $Id: PATMSSM.cpp $ */
/** @file
 * PATMSSM - Dynamic Guest OS Patching Manager; Save and load state
 *
 * NOTE: CSAM assumes patch memory is never reused!!
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
#define LOG_GROUP LOG_GROUP_PATM
#include <VBox/vmm/patm.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/cpumctx-v1_6.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/ssm.h>
#include <VBox/param.h>
#include <iprt/avl.h>
#include "PATMInternal.h"
#include "PATMPatch.h"
#include "PATMA.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/csam.h>
#include <VBox/dbg.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/string.h>
#include <VBox/dis.h>
#include <VBox/disopcode.h>
#include <VBox/version.h>

/**
 * Patch information - SSM version.
 *
 * the difference is the missing pTrampolinePatchesHead member
 * to avoid changing the saved state version for now (will come later).
 */
typedef struct PATCHINFOSSM
{
    uint32_t              uState;
    uint32_t              uOldState;
    DISCPUMODE            uOpMode;

    /* GC pointer of privileged instruction */
    RCPTRTYPE(uint8_t *)  pPrivInstrGC;
    R3PTRTYPE(uint8_t *)  unusedHC;                             /** @todo Can't remove due to structure size dependencies in saved states. */
    uint8_t               aPrivInstr[MAX_INSTR_SIZE];
    uint32_t              cbPrivInstr;
    uint32_t              opcode;      //opcode for priv instr (OP_*)
    uint32_t              cbPatchJump; //patch jump size

    /* Only valid for PATMFL_JUMP_CONFLICT patches */
    RTRCPTR               pPatchJumpDestGC;

    RTGCUINTPTR32         pPatchBlockOffset;
    uint32_t              cbPatchBlockSize;
    uint32_t              uCurPatchOffset;
#if HC_ARCH_BITS == 64
    uint32_t              Alignment0;         /**< Align flags correctly. */
#endif

    uint64_t              flags;

    /**
     * Lowest and highest patched GC instruction address. To optimize searches.
     */
    RTRCPTR               pInstrGCLowest;
    RTRCPTR               pInstrGCHighest;

    /* Tree of fixup records for the patch. */
    R3PTRTYPE(PAVLPVNODECORE) FixupTree;
    uint32_t                  nrFixups;

    /* Tree of jumps inside the generated patch code. */
    uint32_t                  nrJumpRecs;
    R3PTRTYPE(PAVLPVNODECORE) JumpTree;

    /**
     * Lookup trees for determining the corresponding guest address of an
     * instruction in the patch block.
     */
    R3PTRTYPE(PAVLU32NODECORE) Patch2GuestAddrTree;
    R3PTRTYPE(PAVLU32NODECORE) Guest2PatchAddrTree;
    uint32_t                  nrPatch2GuestRecs;
#if HC_ARCH_BITS == 64
    uint32_t        Alignment1;
#endif

    /* Unused, but can't remove due to structure size dependencies in the saved state. */
    PATMP2GLOOKUPREC_OBSOLETE    unused;

    /* Temporary information during patch creation. Don't waste hypervisor memory for this. */
    R3PTRTYPE(PPATCHINFOTEMP) pTempInfo;

    /* Count the number of writes to the corresponding guest code. */
    uint32_t        cCodeWrites;

    /* Count the number of invalid writes to pages monitored for the patch. */
    //some statistics to determine if we should keep this patch activated
    uint32_t        cTraps;

    uint32_t        cInvalidWrites;

    // Index into the uPatchRun and uPatchTrap arrays (0..MAX_PATCHES-1)
    uint32_t        uPatchIdx;

    /* First opcode byte, that's overwritten when a patch is marked dirty. */
    uint8_t         bDirtyOpcode;
    uint8_t         Alignment2[7];      /**< Align the structure size on a 8-byte boundary. */
} PATCHINFOSSM, *PPATCHINFOSSM;

/**
 * Lookup record for patches - SSM version.
 */
typedef struct PATMPATCHRECSSM
{
    /** The key is a GC virtual address. */
    AVLOU32NODECORE  Core;
    /** The key is a patch offset. */
    AVLOU32NODECORE  CoreOffset;

    PATCHINFOSSM     patch;
} PATMPATCHRECSSM, *PPATMPATCHRECSSM;


/**
 * Callback arguments.
 */
typedef struct PATMCALLBACKARGS
{
    PVM             pVM;
    PSSMHANDLE      pSSM;
    PPATMPATCHREC   pPatchRec;
} PATMCALLBACKARGS;
typedef PATMCALLBACKARGS *PPATMCALLBACKARGS;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int patmCorrectFixup(PVM pVM, unsigned ulSSMVersion, PATM &patmInfo, PPATCHINFO pPatch, PRELOCREC pRec,
                            int32_t offset, RTRCPTR *pFixup);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/**
 * SSM descriptor table for the PATM structure.
 */
static SSMFIELD const g_aPatmFields[] =
{
    /** @todo there are a bunch more fields here which can be marked as ignored. */
    SSMFIELD_ENTRY_IGNORE(          PATM, offVM),
    SSMFIELD_ENTRY_RCPTR(           PATM, pPatchMemGC),
    SSMFIELD_ENTRY_IGN_HCPTR(       PATM, pPatchMemHC),
    SSMFIELD_ENTRY(                 PATM, cbPatchMem),
    SSMFIELD_ENTRY(                 PATM, offPatchMem),
    SSMFIELD_ENTRY(                 PATM, fOutOfMemory),
    SSMFIELD_ENTRY_PAD_HC_AUTO(     3, 3),
    SSMFIELD_ENTRY(                 PATM, deltaReloc),
    SSMFIELD_ENTRY_IGN_HCPTR(       PATM, pGCStateHC),
    SSMFIELD_ENTRY_RCPTR(           PATM, pGCStateGC),
    SSMFIELD_ENTRY_RCPTR(           PATM, pGCStackGC),
    SSMFIELD_ENTRY_IGN_HCPTR(       PATM, pGCStackHC),
    SSMFIELD_ENTRY_RCPTR(           PATM, pCPUMCtxGC),
    SSMFIELD_ENTRY_RCPTR(           PATM, pStatsGC),
    SSMFIELD_ENTRY_IGN_HCPTR(       PATM, pStatsHC),
    SSMFIELD_ENTRY(                 PATM, uCurrentPatchIdx),
    SSMFIELD_ENTRY(                 PATM, ulCallDepth),
    SSMFIELD_ENTRY(                 PATM, cPageRecords),
    SSMFIELD_ENTRY_RCPTR(           PATM, pPatchedInstrGCLowest),
    SSMFIELD_ENTRY_RCPTR(           PATM, pPatchedInstrGCHighest),
    SSMFIELD_ENTRY_RCPTR(           PATM, PatchLookupTreeGC),
    SSMFIELD_ENTRY_IGN_HCPTR(       PATM, PatchLookupTreeHC),
    SSMFIELD_ENTRY_RCPTR(           PATM, pfnHelperCallGC),
    SSMFIELD_ENTRY_RCPTR(           PATM, pfnHelperRetGC),
    SSMFIELD_ENTRY_RCPTR(           PATM, pfnHelperJumpGC),
    SSMFIELD_ENTRY_RCPTR(           PATM, pfnHelperIretGC),
    SSMFIELD_ENTRY_IGN_HCPTR(       PATM, pGlobalPatchRec),
    SSMFIELD_ENTRY_RCPTR(           PATM, pfnSysEnterGC),
    SSMFIELD_ENTRY_RCPTR(           PATM, pfnSysEnterPatchGC),
    SSMFIELD_ENTRY(                 PATM, uSysEnterPatchIdx),
    SSMFIELD_ENTRY_RCPTR(           PATM, pvFaultMonitor),
    SSMFIELD_ENTRY_GCPHYS(          PATM, mmio.GCPhys),
    SSMFIELD_ENTRY_RCPTR(           PATM, mmio.pCachedData),
    SSMFIELD_ENTRY_IGN_RCPTR(       PATM, mmio.Alignment0),
    SSMFIELD_ENTRY_IGN_HCPTR(       PATM, savedstate.pSSM),
    SSMFIELD_ENTRY(                 PATM, savedstate.cPatches),
    SSMFIELD_ENTRY_PAD_HC64(        PATM, savedstate.Alignment0, sizeof(uint32_t)),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatNrOpcodeRead),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatDisabled),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatUnusable),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatEnabled),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatInstalled),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatInstalledFunctionPatches),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatInstalledTrampoline),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatInstalledJump),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatInt3Callable),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatInt3BlockRun),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatOverwritten),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatFixedConflicts),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatFlushed),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPageBoundaryCrossed),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatMonitored),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatHandleTrap),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatSwitchBack),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatSwitchBackFail),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPATMMemoryUsed),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatDuplicateREQSuccess),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatDuplicateREQFailed),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatDuplicateUseExisting),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatFunctionFound),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatFunctionNotFound),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPatchWrite),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPatchWriteDetect),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatDirty),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPushTrap),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPatchWriteInterpreted),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPatchWriteInterpretedFailed),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatSysEnter),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatSysExit),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatEmulIret),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatEmulIretFailed),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatInstrDirty),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatInstrDirtyGood),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatInstrDirtyBad),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPatchPageInserted),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPatchPageRemoved),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPatchRefreshSuccess),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPatchRefreshFailed),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatGenRet),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatGenRetReused),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatGenJump),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatGenCall),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatGenPopf),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatCheckPendingIRQ),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatFunctionLookupReplace),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatFunctionLookupInsert),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatU32FunctionMaxSlotsUsed),
    SSMFIELD_ENTRY_IGNORE(          PATM, Alignment0),
    SSMFIELD_ENTRY_TERM()
};

/**
 * SSM descriptor table for the PATM structure starting with r86139.
 */
static SSMFIELD const g_aPatmFields86139[] =
{
    /** @todo there are a bunch more fields here which can be marked as ignored. */
    SSMFIELD_ENTRY_IGNORE(          PATM, offVM),
    SSMFIELD_ENTRY_RCPTR(           PATM, pPatchMemGC),
    SSMFIELD_ENTRY_IGN_HCPTR(       PATM, pPatchMemHC),
    SSMFIELD_ENTRY(                 PATM, cbPatchMem),
    SSMFIELD_ENTRY(                 PATM, offPatchMem),
    SSMFIELD_ENTRY(                 PATM, fOutOfMemory),
    SSMFIELD_ENTRY_PAD_HC_AUTO(     3, 3),
    SSMFIELD_ENTRY(                 PATM, deltaReloc),
    SSMFIELD_ENTRY_IGN_HCPTR(       PATM, pGCStateHC),
    SSMFIELD_ENTRY_RCPTR(           PATM, pGCStateGC),
    SSMFIELD_ENTRY_RCPTR(           PATM, pGCStackGC),
    SSMFIELD_ENTRY_IGN_HCPTR(       PATM, pGCStackHC),
    SSMFIELD_ENTRY_RCPTR(           PATM, pCPUMCtxGC),
    SSMFIELD_ENTRY_RCPTR(           PATM, pStatsGC),
    SSMFIELD_ENTRY_IGN_HCPTR(       PATM, pStatsHC),
    SSMFIELD_ENTRY(                 PATM, uCurrentPatchIdx),
    SSMFIELD_ENTRY(                 PATM, ulCallDepth),
    SSMFIELD_ENTRY(                 PATM, cPageRecords),
    SSMFIELD_ENTRY_RCPTR(           PATM, pPatchedInstrGCLowest),
    SSMFIELD_ENTRY_RCPTR(           PATM, pPatchedInstrGCHighest),
    SSMFIELD_ENTRY_RCPTR(           PATM, PatchLookupTreeGC),
    SSMFIELD_ENTRY_IGN_HCPTR(       PATM, PatchLookupTreeHC),
    SSMFIELD_ENTRY_RCPTR(           PATM, pfnHelperCallGC),
    SSMFIELD_ENTRY_RCPTR(           PATM, pfnHelperRetGC),
    SSMFIELD_ENTRY_RCPTR(           PATM, pfnHelperJumpGC),
    SSMFIELD_ENTRY_RCPTR(           PATM, pfnHelperIretGC),
    SSMFIELD_ENTRY_IGN_HCPTR(       PATM, pGlobalPatchRec),
    SSMFIELD_ENTRY_RCPTR(           PATM, pfnSysEnterGC),
    SSMFIELD_ENTRY_RCPTR(           PATM, pfnSysEnterPatchGC),
    SSMFIELD_ENTRY(                 PATM, uSysEnterPatchIdx),
    SSMFIELD_ENTRY_RCPTR(           PATM, pvFaultMonitor),
    SSMFIELD_ENTRY_GCPHYS(          PATM, mmio.GCPhys),
    SSMFIELD_ENTRY_RCPTR(           PATM, mmio.pCachedData),
    SSMFIELD_ENTRY_IGN_RCPTR(       PATM, mmio.Alignment0),
    SSMFIELD_ENTRY_IGN_HCPTR(       PATM, savedstate.pSSM),
    SSMFIELD_ENTRY(                 PATM, savedstate.cPatches),
    SSMFIELD_ENTRY_PAD_HC64(        PATM, savedstate.Alignment0, sizeof(uint32_t)),
    SSMFIELD_ENTRY_IGN_HCPTR(       PATM, hDbgModPatchMem),
    SSMFIELD_ENTRY_PAD_HC32(        PATM, Alignment0, sizeof(uint32_t)),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatNrOpcodeRead),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatDisabled),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatUnusable),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatEnabled),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatInstalled),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatInstalledFunctionPatches),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatInstalledTrampoline),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatInstalledJump),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatInt3Callable),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatInt3BlockRun),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatOverwritten),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatFixedConflicts),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatFlushed),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPageBoundaryCrossed),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatMonitored),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatHandleTrap),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatSwitchBack),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatSwitchBackFail),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPATMMemoryUsed),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatDuplicateREQSuccess),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatDuplicateREQFailed),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatDuplicateUseExisting),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatFunctionFound),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatFunctionNotFound),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPatchWrite),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPatchWriteDetect),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatDirty),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPushTrap),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPatchWriteInterpreted),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPatchWriteInterpretedFailed),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatSysEnter),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatSysExit),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatEmulIret),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatEmulIretFailed),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatInstrDirty),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatInstrDirtyGood),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatInstrDirtyBad),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPatchPageInserted),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPatchPageRemoved),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPatchRefreshSuccess),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatPatchRefreshFailed),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatGenRet),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatGenRetReused),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatGenJump),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatGenCall),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatGenPopf),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatCheckPendingIRQ),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatFunctionLookupReplace),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatFunctionLookupInsert),
    SSMFIELD_ENTRY_IGNORE(          PATM, StatU32FunctionMaxSlotsUsed),
    SSMFIELD_ENTRY_IGNORE(          PATM, Alignment0),
    SSMFIELD_ENTRY_TERM()
};

/**
 * SSM descriptor table for the PATMGCSTATE structure.
 */
static SSMFIELD const g_aPatmGCStateFields[] =
{
    SSMFIELD_ENTRY(                 PATMGCSTATE, uVMFlags),
    SSMFIELD_ENTRY(                 PATMGCSTATE, uPendingAction),
    SSMFIELD_ENTRY(                 PATMGCSTATE, uPatchCalls),
    SSMFIELD_ENTRY(                 PATMGCSTATE, uScratch),
    SSMFIELD_ENTRY(                 PATMGCSTATE, uIretEFlags),
    SSMFIELD_ENTRY(                 PATMGCSTATE, uIretCS),
    SSMFIELD_ENTRY(                 PATMGCSTATE, uIretEIP),
    SSMFIELD_ENTRY(                 PATMGCSTATE, Psp),
    SSMFIELD_ENTRY(                 PATMGCSTATE, fPIF),
    SSMFIELD_ENTRY_RCPTR(           PATMGCSTATE, GCPtrInhibitInterrupts),
    SSMFIELD_ENTRY_RCPTR(           PATMGCSTATE, GCCallPatchTargetAddr),
    SSMFIELD_ENTRY_RCPTR(           PATMGCSTATE, GCCallReturnAddr),
    SSMFIELD_ENTRY(                 PATMGCSTATE, Restore.uEAX),
    SSMFIELD_ENTRY(                 PATMGCSTATE, Restore.uECX),
    SSMFIELD_ENTRY(                 PATMGCSTATE, Restore.uEDI),
    SSMFIELD_ENTRY(                 PATMGCSTATE, Restore.eFlags),
    SSMFIELD_ENTRY(                 PATMGCSTATE, Restore.uFlags),
    SSMFIELD_ENTRY_TERM()
};

/**
 * SSM descriptor table for the PATMPATCHREC structure.
 */
static SSMFIELD const g_aPatmPatchRecFields[] =
{
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, Core.Key),
    SSMFIELD_ENTRY_IGNORE(          PATMPATCHRECSSM, Core.pLeft),
    SSMFIELD_ENTRY_IGNORE(          PATMPATCHRECSSM, Core.pRight),
    SSMFIELD_ENTRY_IGNORE(          PATMPATCHRECSSM, Core.uchHeight),
    SSMFIELD_ENTRY_PAD_HC_AUTO(     3, 3),
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, CoreOffset.Key),
    SSMFIELD_ENTRY_IGNORE(          PATMPATCHRECSSM, CoreOffset.pLeft),
    SSMFIELD_ENTRY_IGNORE(          PATMPATCHRECSSM, CoreOffset.pRight),
    SSMFIELD_ENTRY_IGNORE(          PATMPATCHRECSSM, CoreOffset.uchHeight),
    SSMFIELD_ENTRY_PAD_HC_AUTO(     3, 3),
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, patch.uState),
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, patch.uOldState),
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, patch.uOpMode),
    SSMFIELD_ENTRY_RCPTR(           PATMPATCHRECSSM, patch.pPrivInstrGC),
    SSMFIELD_ENTRY_IGN_HCPTR(       PATMPATCHRECSSM, patch.unusedHC),
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, patch.aPrivInstr),
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, patch.cbPrivInstr),
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, patch.opcode),
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, patch.cbPatchJump),
    SSMFIELD_ENTRY_RCPTR(           PATMPATCHRECSSM, patch.pPatchJumpDestGC),
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, patch.pPatchBlockOffset),
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, patch.cbPatchBlockSize),
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, patch.uCurPatchOffset),
    SSMFIELD_ENTRY_PAD_HC64(        PATMPATCHRECSSM, patch.Alignment0, sizeof(uint32_t)),
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, patch.flags),
    SSMFIELD_ENTRY_RCPTR(           PATMPATCHRECSSM, patch.pInstrGCLowest),
    SSMFIELD_ENTRY_RCPTR(           PATMPATCHRECSSM, patch.pInstrGCHighest),
    SSMFIELD_ENTRY_IGN_HCPTR(       PATMPATCHRECSSM, patch.FixupTree),
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, patch.nrFixups),
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, patch.nrJumpRecs), // should be zero?
    SSMFIELD_ENTRY_IGN_HCPTR(       PATMPATCHRECSSM, patch.JumpTree),
    SSMFIELD_ENTRY_IGN_HCPTR(       PATMPATCHRECSSM, patch.Patch2GuestAddrTree),
    SSMFIELD_ENTRY_IGN_HCPTR(       PATMPATCHRECSSM, patch.Guest2PatchAddrTree),
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, patch.nrPatch2GuestRecs),
    SSMFIELD_ENTRY_PAD_HC64(        PATMPATCHRECSSM, patch.Alignment1, sizeof(uint32_t)),
    SSMFIELD_ENTRY_IGN_HCPTR(       PATMPATCHRECSSM, patch.unused.pPatchLocStartHC), // saved as zero
    SSMFIELD_ENTRY_IGN_HCPTR(       PATMPATCHRECSSM, patch.unused.pPatchLocEndHC),   // ditto
    SSMFIELD_ENTRY_IGN_RCPTR(       PATMPATCHRECSSM, patch.unused.pGuestLoc),        // ditto
    SSMFIELD_ENTRY_IGNORE(          PATMPATCHRECSSM, patch.unused.opsize),           // ditto
    SSMFIELD_ENTRY_IGN_HCPTR(       PATMPATCHRECSSM, patch.pTempInfo),
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, patch.cCodeWrites),
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, patch.cTraps),
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, patch.cInvalidWrites),
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, patch.uPatchIdx),
    SSMFIELD_ENTRY(                 PATMPATCHRECSSM, patch.bDirtyOpcode),
    SSMFIELD_ENTRY_IGNORE(          PATMPATCHRECSSM, patch.Alignment2),
    SSMFIELD_ENTRY_TERM()
};

/**
 * SSM descriptor table for the RELOCREC structure.
 */
static SSMFIELD const g_aPatmRelocRec[] =
{
    SSMFIELD_ENTRY_HCPTR_HACK_U32(  RELOCREC, Core.Key),        // Used to store the relocation type
    SSMFIELD_ENTRY_IGN_HCPTR(       RELOCREC, Core.pLeft),
    SSMFIELD_ENTRY_IGN_HCPTR(       RELOCREC, Core.pRight),
    SSMFIELD_ENTRY_IGNORE(          RELOCREC, Core.uchHeight),
    SSMFIELD_ENTRY_PAD_HC_AUTO(     3, 7),
    SSMFIELD_ENTRY(                 RELOCREC, uType),
    SSMFIELD_ENTRY_PAD_HC_AUTO(     0, 4),
    SSMFIELD_ENTRY_HCPTR_HACK_U32(  RELOCREC, pRelocPos),       // converted to a patch member offset.
    SSMFIELD_ENTRY_RCPTR(           RELOCREC, pSource),
    SSMFIELD_ENTRY_RCPTR(           RELOCREC, pDest),
    SSMFIELD_ENTRY_TERM()
};

/**
 * SSM descriptor table for the RECPATCHTOGUEST structure.
 */
static SSMFIELD const g_aPatmRecPatchToGuest[] =
{
    SSMFIELD_ENTRY(                 RECPATCHTOGUEST, Core.Key),
    SSMFIELD_ENTRY_PAD_HC_AUTO(     0, 4),
    SSMFIELD_ENTRY_IGN_HCPTR(       RECPATCHTOGUEST, Core.pLeft),
    SSMFIELD_ENTRY_IGN_HCPTR(       RECPATCHTOGUEST, Core.pRight),
    SSMFIELD_ENTRY_IGNORE(          RECPATCHTOGUEST, Core.uchHeight),
    SSMFIELD_ENTRY_PAD_HC_AUTO(     3, 7),
    SSMFIELD_ENTRY_RCPTR(           RECPATCHTOGUEST, pOrgInstrGC),
    SSMFIELD_ENTRY(                 RECPATCHTOGUEST, enmType),
    SSMFIELD_ENTRY(                 RECPATCHTOGUEST, fDirty),
    SSMFIELD_ENTRY(                 RECPATCHTOGUEST, fJumpTarget),
    SSMFIELD_ENTRY(                 RECPATCHTOGUEST, u8DirtyOpcode),
    SSMFIELD_ENTRY_PAD_HC_AUTO(     1, 5),
    SSMFIELD_ENTRY_TERM()
};

#ifdef VBOX_STRICT

/**
 * Callback function for RTAvlPVDoWithAll
 *
 * Counts the number of patches in the tree
 *
 * @returns VBox status code.
 * @param   pNode           Current node
 * @param   pcPatches       Pointer to patch counter (uint32_t)
 */
static DECLCALLBACK(int) patmCountLeafPV(PAVLPVNODECORE pNode, void *pcPatches)
{
    NOREF(pNode);
    *(uint32_t *)pcPatches = *(uint32_t *)pcPatches + 1;
    return VINF_SUCCESS;
}

/**
 * Callback function for RTAvlU32DoWithAll
 *
 * Counts the number of patches in the tree
 *
 * @returns VBox status code.
 * @param   pNode           Current node
 * @param   pcPatches       Pointer to patch counter (uint32_t)
 */
static DECLCALLBACK(int) patmCountLeaf(PAVLU32NODECORE pNode, void *pcPatches)
{
    NOREF(pNode);
    *(uint32_t *)pcPatches = *(uint32_t *)pcPatches + 1;
    return VINF_SUCCESS;
}

#endif /* VBOX_STRICT */

/**
 * Callback function for RTAvloU32DoWithAll
 *
 * Counts the number of patches in the tree
 *
 * @returns VBox status code.
 * @param   pNode           Current node
 * @param   pcPatches       Pointer to patch counter
 */
static DECLCALLBACK(int) patmCountPatch(PAVLOU32NODECORE pNode, void *pcPatches)
{
    NOREF(pNode);
    *(uint32_t *)pcPatches = *(uint32_t *)pcPatches + 1;
    return VINF_SUCCESS;
}

/**
 * Callback function for RTAvlU32DoWithAll
 *
 * Saves all patch to guest lookup records.
 *
 * @returns VBox status code.
 * @param   pNode           Current node
 * @param   pvUser          Pointer to PATMCALLBACKARGS.
 */
static DECLCALLBACK(int) patmSaveP2GLookupRecords(PAVLU32NODECORE pNode, void *pvUser)
{
    PPATMCALLBACKARGS   pArgs = (PPATMCALLBACKARGS)pvUser;
    PRECPATCHTOGUEST    pPatchToGuestRec = (PRECPATCHTOGUEST)pNode;

    /* Save the lookup record. */
    int rc = SSMR3PutStructEx(pArgs->pSSM, pPatchToGuestRec, sizeof(RECPATCHTOGUEST), 0 /*fFlags*/,
                              &g_aPatmRecPatchToGuest[0], NULL);
    AssertRCReturn(rc, rc);

    return VINF_SUCCESS;
}

/**
 * Callback function for RTAvlPVDoWithAll
 *
 * Saves all patch to guest lookup records.
 *
 * @returns VBox status code.
 * @param   pNode           Current node
 * @param   pvUser          Pointer to PATMCALLBACKARGS.
 */
static DECLCALLBACK(int) patmSaveFixupRecords(PAVLPVNODECORE pNode, void *pvUser)
{
    PPATMCALLBACKARGS   pArgs = (PPATMCALLBACKARGS)pvUser;
    RELOCREC            rec  = *(PRELOCREC)pNode;

    /* Convert pointer to an offset into patch memory.  May not be applicable
       to all fixup types, thus the UINT32_MAX. */
    AssertMsg(   rec.pRelocPos
              || (   rec.uType == FIXUP_REL_JMPTOPATCH
                  && !(pArgs->pPatchRec->patch.flags & PATMFL_PATCHED_GUEST_CODE)),
             ("uState=%#x uType=%#x flags=%#RX64\n", pArgs->pPatchRec->patch.uState, rec.uType, pArgs->pPatchRec->patch.flags));
    uintptr_t offRelocPos = (uintptr_t)rec.pRelocPos - (uintptr_t)pArgs->pVM->patm.s.pPatchMemHC;
    if (offRelocPos > pArgs->pVM->patm.s.cbPatchMem)
        offRelocPos = UINT32_MAX;
    rec.pRelocPos = (uint8_t *)offRelocPos;

    /* Zero rec.Core.Key since it's unused and may trigger SSM check due to the hack below. */
    rec.Core.Key = 0;

    /* Save the lookup record. */
    int rc = SSMR3PutStructEx(pArgs->pSSM, &rec, sizeof(rec), 0 /*fFlags*/, &g_aPatmRelocRec[0], NULL);
    AssertRCReturn(rc, rc);

    return VINF_SUCCESS;
}

/**
 * Converts a saved state patch record to the memory record.
 *
 * @returns nothing.
 * @param   pPatch       The memory record.
 * @param   pPatchSSM    The SSM version of the patch record.
 */
static void patmR3PatchConvertSSM2Mem(PPATMPATCHREC pPatch, PPATMPATCHRECSSM pPatchSSM)
{
    /*
     * Only restore the patch part of the tree record; not the internal data (except the key of course)
     */
    pPatch->Core.Key                  = pPatchSSM->Core.Key;
    pPatch->CoreOffset.Key            = pPatchSSM->CoreOffset.Key;
    pPatch->patch.uState              = pPatchSSM->patch.uState;
    pPatch->patch.uOldState           = pPatchSSM->patch.uOldState;
    pPatch->patch.uOpMode             = pPatchSSM->patch.uOpMode;
    pPatch->patch.pPrivInstrGC        = pPatchSSM->patch.pPrivInstrGC;
    pPatch->patch.unusedHC            = pPatchSSM->patch.unusedHC;
    memcpy(&pPatch->patch.aPrivInstr[0], &pPatchSSM->patch.aPrivInstr[0], MAX_INSTR_SIZE);
    pPatch->patch.cbPrivInstr         = pPatchSSM->patch.cbPrivInstr;
    pPatch->patch.opcode              = pPatchSSM->patch.opcode;
    pPatch->patch.cbPatchJump         = pPatchSSM->patch.cbPatchJump;
    pPatch->patch.pPatchJumpDestGC    = pPatchSSM->patch.pPatchJumpDestGC;
    pPatch->patch.pPatchBlockOffset   = pPatchSSM->patch.pPatchBlockOffset;
    pPatch->patch.cbPatchBlockSize    = pPatchSSM->patch.cbPatchBlockSize;
    pPatch->patch.uCurPatchOffset     = pPatchSSM->patch.uCurPatchOffset;
    pPatch->patch.flags               = pPatchSSM->patch.flags;
    pPatch->patch.pInstrGCLowest      = pPatchSSM->patch.pInstrGCLowest;
    pPatch->patch.pInstrGCHighest     = pPatchSSM->patch.pInstrGCHighest;
    pPatch->patch.FixupTree           = pPatchSSM->patch.FixupTree;
    pPatch->patch.nrFixups            = pPatchSSM->patch.nrFixups;
    pPatch->patch.nrJumpRecs          = pPatchSSM->patch.nrJumpRecs;
    pPatch->patch.JumpTree            = pPatchSSM->patch.JumpTree;
    pPatch->patch.Patch2GuestAddrTree = pPatchSSM->patch.Patch2GuestAddrTree;
    pPatch->patch.Guest2PatchAddrTree = pPatchSSM->patch.Guest2PatchAddrTree;
    pPatch->patch.nrPatch2GuestRecs   = pPatchSSM->patch.nrPatch2GuestRecs;
    pPatch->patch.unused              = pPatchSSM->patch.unused;
    pPatch->patch.pTempInfo           = pPatchSSM->patch.pTempInfo;
    pPatch->patch.cCodeWrites         = pPatchSSM->patch.cCodeWrites;
    pPatch->patch.cTraps              = pPatchSSM->patch.cTraps;
    pPatch->patch.cInvalidWrites      = pPatchSSM->patch.cInvalidWrites;
    pPatch->patch.uPatchIdx           = pPatchSSM->patch.uPatchIdx;
    pPatch->patch.bDirtyOpcode        = pPatchSSM->patch.bDirtyOpcode;
    pPatch->patch.pTrampolinePatchesHead = NULL;
}

/**
 * Converts a memory patch record to the saved state version.
 *
 * @returns nothing.
 * @param   pPatchSSM    The saved state record.
 * @param   pPatch       The memory version to save.
 */
static void patmR3PatchConvertMem2SSM(PPATMPATCHRECSSM pPatchSSM, PPATMPATCHREC pPatch)
{
    pPatchSSM->Core                      = pPatch->Core;
    pPatchSSM->CoreOffset                = pPatch->CoreOffset;
    pPatchSSM->patch.uState              = pPatch->patch.uState;
    pPatchSSM->patch.uOldState           = pPatch->patch.uOldState;
    pPatchSSM->patch.uOpMode             = pPatch->patch.uOpMode;
    pPatchSSM->patch.pPrivInstrGC        = pPatch->patch.pPrivInstrGC;
    pPatchSSM->patch.unusedHC            = pPatch->patch.unusedHC;
    memcpy(&pPatchSSM->patch.aPrivInstr[0], &pPatch->patch.aPrivInstr[0], MAX_INSTR_SIZE);
    pPatchSSM->patch.cbPrivInstr         = pPatch->patch.cbPrivInstr;
    pPatchSSM->patch.opcode              = pPatch->patch.opcode;
    pPatchSSM->patch.cbPatchJump         = pPatch->patch.cbPatchJump;
    pPatchSSM->patch.pPatchJumpDestGC    = pPatch->patch.pPatchJumpDestGC;
    pPatchSSM->patch.pPatchBlockOffset   = pPatch->patch.pPatchBlockOffset;
    pPatchSSM->patch.cbPatchBlockSize    = pPatch->patch.cbPatchBlockSize;
    pPatchSSM->patch.uCurPatchOffset     = pPatch->patch.uCurPatchOffset;
    pPatchSSM->patch.flags               = pPatch->patch.flags;
    pPatchSSM->patch.pInstrGCLowest      = pPatch->patch.pInstrGCLowest;
    pPatchSSM->patch.pInstrGCHighest     = pPatch->patch.pInstrGCHighest;
    pPatchSSM->patch.FixupTree           = pPatch->patch.FixupTree;
    pPatchSSM->patch.nrFixups            = pPatch->patch.nrFixups;
    pPatchSSM->patch.nrJumpRecs          = pPatch->patch.nrJumpRecs;
    pPatchSSM->patch.JumpTree            = pPatch->patch.JumpTree;
    pPatchSSM->patch.Patch2GuestAddrTree = pPatch->patch.Patch2GuestAddrTree;
    pPatchSSM->patch.Guest2PatchAddrTree = pPatch->patch.Guest2PatchAddrTree;
    pPatchSSM->patch.nrPatch2GuestRecs   = pPatch->patch.nrPatch2GuestRecs;
    pPatchSSM->patch.unused              = pPatch->patch.unused;
    pPatchSSM->patch.pTempInfo           = pPatch->patch.pTempInfo;
    pPatchSSM->patch.cCodeWrites         = pPatch->patch.cCodeWrites;
    pPatchSSM->patch.cTraps              = pPatch->patch.cTraps;
    pPatchSSM->patch.cInvalidWrites      = pPatch->patch.cInvalidWrites;
    pPatchSSM->patch.uPatchIdx           = pPatch->patch.uPatchIdx;
    pPatchSSM->patch.bDirtyOpcode        = pPatch->patch.bDirtyOpcode;
}

/**
 * Callback function for RTAvloU32DoWithAll
 *
 * Saves the state of the patch that's being enumerated
 *
 * @returns VBox status code.
 * @param   pNode           Current node
 * @param   pvUser          Pointer to PATMCALLBACKARGS.
 */
static DECLCALLBACK(int) patmSavePatchState(PAVLOU32NODECORE pNode, void *pvUser)
{
    PPATMCALLBACKARGS   pArgs  = (PPATMCALLBACKARGS)pvUser;
    PPATMPATCHREC       pPatch = (PPATMPATCHREC)pNode;
    PATMPATCHRECSSM     patch;
    int                 rc;

    pArgs->pPatchRec = pPatch;
    Assert(!(pPatch->patch.flags & PATMFL_GLOBAL_FUNCTIONS));

    patmR3PatchConvertMem2SSM(&patch, pPatch);
    Log4(("patmSavePatchState: cbPatchJump=%u uCurPathOffset=%#x pInstrGCLowest/Higest=%#x/%#x nrFixups=%#x nrJumpRecs=%#x\n",
          patch.patch.cbPatchJump, patch.patch.uCurPatchOffset, patch.patch.pInstrGCLowest, patch.patch.pInstrGCHighest,
          patch.patch.nrFixups, patch.patch.nrJumpRecs));

    /*
     * Reset HC pointers that need to be recalculated when loading the state
     */
#ifdef VBOX_STRICT
    PVM pVM = pArgs->pVM; /* For PATCHCODE_PTR_HC. */
    AssertMsg(patch.patch.uState == PATCH_REFUSED || (patch.patch.pPatchBlockOffset || (patch.patch.flags & (PATMFL_SYSENTER_XP|PATMFL_INT3_REPLACEMENT))),
              ("State = %x pPatchBlockHC=%08x flags=%x\n", patch.patch.uState, PATCHCODE_PTR_HC(&patch.patch), patch.patch.flags));
#endif
    Assert(pPatch->patch.JumpTree == 0);
    Assert(!pPatch->patch.pTempInfo || pPatch->patch.pTempInfo->DisasmJumpTree == 0);
    Assert(!pPatch->patch.pTempInfo || pPatch->patch.pTempInfo->IllegalInstrTree == 0);

    /* Save the patch record itself */
    rc = SSMR3PutStructEx(pArgs->pSSM, &patch, sizeof(patch), 0 /*fFlags*/, &g_aPatmPatchRecFields[0], NULL);
    AssertRCReturn(rc, rc);

    /*
     * Reset HC pointers in fixup records and save them.
     */
#ifdef VBOX_STRICT
    uint32_t nrFixupRecs = 0;
    RTAvlPVDoWithAll(&pPatch->patch.FixupTree, true, patmCountLeafPV, &nrFixupRecs);
    AssertMsg(nrFixupRecs == pPatch->patch.nrFixups, ("Fixup inconsistency! counted %d vs %d\n", nrFixupRecs, pPatch->patch.nrFixups));
#endif
    rc = RTAvlPVDoWithAll(&pPatch->patch.FixupTree, true, patmSaveFixupRecords, pArgs);
    AssertRCReturn(rc, rc);

#ifdef VBOX_STRICT
    uint32_t nrLookupRecords = 0;
    RTAvlU32DoWithAll(&pPatch->patch.Patch2GuestAddrTree, true, patmCountLeaf, &nrLookupRecords);
    Assert(nrLookupRecords == pPatch->patch.nrPatch2GuestRecs);
#endif

    rc = RTAvlU32DoWithAll(&pPatch->patch.Patch2GuestAddrTree, true, patmSaveP2GLookupRecords, pArgs);
    AssertRCReturn(rc, rc);

    pArgs->pPatchRec = NULL;
    return VINF_SUCCESS;
}

/**
 * Execute state save operation.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pSSM            SSM operation handle.
 */
DECLCALLBACK(int) patmR3Save(PVM pVM, PSSMHANDLE pSSM)
{
    PATM patmInfo = pVM->patm.s;
    int  rc;

    pVM->patm.s.savedstate.pSSM = pSSM;

    /*
     * Reset HC pointers that need to be recalculated when loading the state
     */
    patmInfo.pPatchMemHC = NULL;
    patmInfo.pGCStateHC  = 0;
    patmInfo.pvFaultMonitor = 0;

    Assert(patmInfo.ulCallDepth == 0);

    /*
     * Count the number of patches in the tree (feeling lazy)
     */
    patmInfo.savedstate.cPatches = 0;
    RTAvloU32DoWithAll(&pVM->patm.s.PatchLookupTreeHC->PatchTree, true, patmCountPatch, &patmInfo.savedstate.cPatches);

    /*
     * Save PATM structure
     */
    rc = SSMR3PutStructEx(pSSM, &patmInfo, sizeof(patmInfo), 0 /*fFlags*/, &g_aPatmFields[0], NULL);
    AssertRCReturn(rc, rc);

    /*
     * Save patch memory contents
     */
    rc = SSMR3PutMem(pSSM, pVM->patm.s.pPatchMemHC, pVM->patm.s.cbPatchMem);
    AssertRCReturn(rc, rc);

    /*
     * Save GC state memory
     */
    rc = SSMR3PutStructEx(pSSM, pVM->patm.s.pGCStateHC, sizeof(PATMGCSTATE), 0 /*fFlags*/, &g_aPatmGCStateFields[0], NULL);
    AssertRCReturn(rc, rc);

    /*
     * Save PATM stack page
     */
    SSMR3PutU32(pSSM, PATM_STACK_TOTAL_SIZE);
    rc = SSMR3PutMem(pSSM, pVM->patm.s.pGCStackHC, PATM_STACK_TOTAL_SIZE);
    AssertRCReturn(rc, rc);

    /*
     * Save all patches
     */
    PATMCALLBACKARGS Args;
    Args.pVM = pVM;
    Args.pSSM = pSSM;
    rc = RTAvloU32DoWithAll(&pVM->patm.s.PatchLookupTreeHC->PatchTree, true, patmSavePatchState, &Args);
    AssertRCReturn(rc, rc);

    /* Note! Patch statistics are not saved. */

    return VINF_SUCCESS;
}


/**
 * Execute state load operation.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pSSM            SSM operation handle.
 * @param   uVersion        Data layout version.
 * @param   uPass           The data pass.
 */
DECLCALLBACK(int) patmR3Load(PVM pVM, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    PATM patmInfo;
    int  rc;

    if (    uVersion != PATM_SAVED_STATE_VERSION
        &&  uVersion != PATM_SAVED_STATE_VERSION_NO_RAW_MEM
        &&  uVersion != PATM_SAVED_STATE_VERSION_MEM
        &&  uVersion != PATM_SAVED_STATE_VERSION_FIXUP_HACK
        &&  uVersion != PATM_SAVED_STATE_VERSION_VER16
       )
    {
        AssertMsgFailed(("patmR3Load: Invalid version uVersion=%d!\n", uVersion));
        return VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;
    }
    uint32_t const fStructRestoreFlags = uVersion <= PATM_SAVED_STATE_VERSION_MEM ? SSMSTRUCT_FLAGS_MEM_BAND_AID_RELAXED : 0;
    Assert(uPass == SSM_PASS_FINAL); NOREF(uPass);

    pVM->patm.s.savedstate.pSSM = pSSM;

    /*
     * Restore PATM structure
     */
    RT_ZERO(patmInfo);
    if (   uVersion == PATM_SAVED_STATE_VERSION_MEM
        && SSMR3HandleRevision(pSSM) >= 86139
        && SSMR3HandleVersion(pSSM)  >= VBOX_FULL_VERSION_MAKE(4, 2, 51))
        rc = SSMR3GetStructEx(pSSM, &patmInfo, sizeof(patmInfo), SSMSTRUCT_FLAGS_MEM_BAND_AID_RELAXED,
                              &g_aPatmFields86139[0], NULL);
    else
        rc = SSMR3GetStructEx(pSSM, &patmInfo, sizeof(patmInfo), fStructRestoreFlags, &g_aPatmFields[0], NULL);
    AssertRCReturn(rc, rc);

    /* Relative calls are made to the helper functions. Therefor their relative location must not change! */
    /* Note: we reuse the saved global helpers and assume they are identical, which is kind of dangerous. */
    AssertLogRelReturn((pVM->patm.s.pfnHelperCallGC - pVM->patm.s.pPatchMemGC) == (patmInfo.pfnHelperCallGC  - patmInfo.pPatchMemGC),
                       VERR_SSM_DATA_UNIT_FORMAT_CHANGED);
    AssertLogRelReturn((pVM->patm.s.pfnHelperRetGC  - pVM->patm.s.pPatchMemGC) == (patmInfo.pfnHelperRetGC   - patmInfo.pPatchMemGC),
                       VERR_SSM_DATA_UNIT_FORMAT_CHANGED);
    AssertLogRelReturn((pVM->patm.s.pfnHelperJumpGC - pVM->patm.s.pPatchMemGC) == (patmInfo.pfnHelperJumpGC  - patmInfo.pPatchMemGC),
                       VERR_SSM_DATA_UNIT_FORMAT_CHANGED);
    AssertLogRelReturn((pVM->patm.s.pfnHelperIretGC - pVM->patm.s.pPatchMemGC) == (patmInfo.pfnHelperIretGC  - patmInfo.pPatchMemGC),
                       VERR_SSM_DATA_UNIT_FORMAT_CHANGED);
    AssertLogRelReturn(pVM->patm.s.cbPatchMem == patmInfo.cbPatchMem, VERR_SSM_DATA_UNIT_FORMAT_CHANGED);

    pVM->patm.s.offPatchMem         = patmInfo.offPatchMem;
    pVM->patm.s.deltaReloc          = patmInfo.deltaReloc;
    pVM->patm.s.uCurrentPatchIdx    = patmInfo.uCurrentPatchIdx;
    pVM->patm.s.fOutOfMemory        = patmInfo.fOutOfMemory;

    /* Lowest and highest patched instruction */
    pVM->patm.s.pPatchedInstrGCLowest    = patmInfo.pPatchedInstrGCLowest;
    pVM->patm.s.pPatchedInstrGCHighest   = patmInfo.pPatchedInstrGCHighest;

    /* Sysenter handlers */
    pVM->patm.s.pfnSysEnterGC            = patmInfo.pfnSysEnterGC;
    pVM->patm.s.pfnSysEnterPatchGC       = patmInfo.pfnSysEnterPatchGC;
    pVM->patm.s.uSysEnterPatchIdx        = patmInfo.uSysEnterPatchIdx;

    Assert(patmInfo.ulCallDepth == 0 && pVM->patm.s.ulCallDepth == 0);

    Log(("pPatchMemGC %RRv vs old %RRv\n", pVM->patm.s.pPatchMemGC, patmInfo.pPatchMemGC));
    Log(("pGCStateGC  %RRv vs old %RRv\n", pVM->patm.s.pGCStateGC, patmInfo.pGCStateGC));
    Log(("pGCStackGC  %RRv vs old %RRv\n", pVM->patm.s.pGCStackGC, patmInfo.pGCStackGC));
    Log(("pCPUMCtxGC  %RRv vs old %RRv\n", pVM->patm.s.pCPUMCtxGC, patmInfo.pCPUMCtxGC));


    /** @note patch statistics are not restored. */

    /*
     * Restore patch memory contents
     */
    Log(("Restore patch memory: new %RRv old %RRv\n", pVM->patm.s.pPatchMemGC, patmInfo.pPatchMemGC));
    rc = SSMR3GetMem(pSSM, pVM->patm.s.pPatchMemHC, pVM->patm.s.cbPatchMem);
    AssertRCReturn(rc, rc);

    /*
     * Restore GC state memory
     */
    RT_BZERO(pVM->patm.s.pGCStateHC, sizeof(PATMGCSTATE));
    rc = SSMR3GetStructEx(pSSM, pVM->patm.s.pGCStateHC, sizeof(PATMGCSTATE), fStructRestoreFlags, &g_aPatmGCStateFields[0], NULL);
    AssertRCReturn(rc, rc);

    /*
     * Restore PATM stack page
     */
    uint32_t cbStack = PATM_STACK_TOTAL_SIZE;
    if (uVersion > PATM_SAVED_STATE_VERSION_MEM)
    {
        rc = SSMR3GetU32(pSSM, &cbStack);
        AssertRCReturn(rc, rc);
    }
    AssertCompile(!(PATM_STACK_TOTAL_SIZE & 31));
    AssertLogRelMsgReturn(cbStack > 0 && cbStack <= PATM_STACK_TOTAL_SIZE && !(cbStack & 31),
                          ("cbStack=%#x vs %#x", cbStack, PATM_STACK_TOTAL_SIZE),
                          VERR_SSM_DATA_UNIT_FORMAT_CHANGED);
    rc = SSMR3GetMem(pSSM, pVM->patm.s.pGCStackHC, cbStack);
    AssertRCReturn(rc, rc);
    if (cbStack < PATM_STACK_TOTAL_SIZE)
        memset((uint8_t *)pVM->patm.s.pGCStackHC + cbStack, 0, PATM_STACK_TOTAL_SIZE - cbStack);

    /*
     * Load all patches
     */
    for (unsigned i = 0; i < patmInfo.savedstate.cPatches; i++)
    {
        PATMPATCHRECSSM patch;
        PATMPATCHREC *pPatchRec;

        RT_ZERO(patch);
        rc = SSMR3GetStructEx(pSSM, &patch, sizeof(patch), fStructRestoreFlags, &g_aPatmPatchRecFields[0], NULL);
        AssertRCReturn(rc, rc);
        Log4(("patmR3Load: cbPatchJump=%u uCurPathOffset=%#x pInstrGCLowest/Higest=%#x/%#x nrFixups=%#x nrJumpRecs=%#x\n",
              patch.patch.cbPatchJump, patch.patch.uCurPatchOffset, patch.patch.pInstrGCLowest, patch.patch.pInstrGCHighest,
              patch.patch.nrFixups, patch.patch.nrJumpRecs));

        Assert(!(patch.patch.flags & PATMFL_GLOBAL_FUNCTIONS));

        rc = MMHyperAlloc(pVM, sizeof(PATMPATCHREC), 0, MM_TAG_PATM_PATCH, (void **)&pPatchRec);
        if (RT_FAILURE(rc))
        {
            AssertMsgFailed(("Out of memory!!!!\n"));
            return VERR_NO_MEMORY;
        }

        /* Convert SSM version to memory. */
        patmR3PatchConvertSSM2Mem(pPatchRec, &patch);

        Log(("Restoring patch %RRv -> %RRv state %x\n", pPatchRec->patch.pPrivInstrGC, patmInfo.pPatchMemGC + pPatchRec->patch.pPatchBlockOffset, pPatchRec->patch.uState));
        bool ret = RTAvloU32Insert(&pVM->patm.s.PatchLookupTreeHC->PatchTree, &pPatchRec->Core);
        Assert(ret);
        if (pPatchRec->patch.uState != PATCH_REFUSED)
        {
            if (pPatchRec->patch.pPatchBlockOffset)
            {
                /* We actually generated code for this patch. */
                ret = RTAvloU32Insert(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPatchAddr, &pPatchRec->CoreOffset);
                AssertMsg(ret, ("Inserting patch %RRv offset %08RX32 failed!!\n", pPatchRec->patch.pPrivInstrGC, pPatchRec->CoreOffset.Key));
            }
        }
        /* Set to zero as we don't need it anymore. */
        pPatchRec->patch.pTempInfo = 0;

        PATMP2GLOOKUPREC cacheRec;
        RT_ZERO(cacheRec);
        cacheRec.pPatch = &pPatchRec->patch;

        uint8_t *pPrivInstrHC = patmR3GCVirtToHCVirt(pVM, &cacheRec, pPatchRec->patch.pPrivInstrGC);
        /* Can fail due to page or page table not present. */

        /*
         * Restore fixup records and correct HC pointers in fixup records
         */
        pPatchRec->patch.FixupTree = 0;
        pPatchRec->patch.nrFixups  = 0;    /* increased by patmPatchAddReloc32 */
        for (unsigned j = 0; j < patch.patch.nrFixups; j++)
        {
            RELOCREC rec;
            int32_t offset;
            RTRCPTR *pFixup;

            RT_ZERO(rec);
            rc = SSMR3GetStructEx(pSSM, &rec, sizeof(rec), fStructRestoreFlags, &g_aPatmRelocRec[0], NULL);
            AssertRCReturn(rc, rc);

            if (pPrivInstrHC)
            {
                /* rec.pRelocPos now contains the relative position inside the hypervisor area. */
                offset = (int32_t)(intptr_t)rec.pRelocPos;
                /* Convert to HC pointer again. */
                if ((uintptr_t)rec.pRelocPos < pVM->patm.s.cbPatchMem)
                    rec.pRelocPos = pVM->patm.s.pPatchMemHC + (uintptr_t)rec.pRelocPos;
                else
                    rec.pRelocPos = NULL;
                pFixup = (RTRCPTR *)rec.pRelocPos;

                if (pPatchRec->patch.uState != PATCH_REFUSED)
                {
                    if (    rec.uType == FIXUP_REL_JMPTOPATCH
                        &&  (pPatchRec->patch.flags & PATMFL_PATCHED_GUEST_CODE))
                    {
                        Assert(pPatchRec->patch.cbPatchJump == SIZEOF_NEARJUMP32 || pPatchRec->patch.cbPatchJump == SIZEOF_NEAR_COND_JUMP32);
                        unsigned offset2 = (pPatchRec->patch.cbPatchJump == SIZEOF_NEARJUMP32) ? 1 : 2;

                        rec.pRelocPos = pPrivInstrHC + offset2;
                        pFixup        = (RTRCPTR *)rec.pRelocPos;
                    }

                    rc = patmCorrectFixup(pVM, uVersion, patmInfo, &pPatchRec->patch, &rec, offset, pFixup);
                    AssertRCReturn(rc, rc);
                }

                rc = patmPatchAddReloc32(pVM, &pPatchRec->patch, rec.pRelocPos, rec.uType, rec.pSource, rec.pDest);
                AssertRCReturn(rc, rc);
            }
        }
        /* Release previous lock if any. */
        if (cacheRec.Lock.pvMap)
            PGMPhysReleasePageMappingLock(pVM, &cacheRec.Lock);

        /* And all patch to guest lookup records */
        Assert(pPatchRec->patch.nrPatch2GuestRecs || pPatchRec->patch.uState == PATCH_REFUSED || (pPatchRec->patch.flags & (PATMFL_SYSENTER_XP | PATMFL_IDTHANDLER | PATMFL_TRAPHANDLER | PATMFL_INT3_REPLACEMENT)));

        pPatchRec->patch.Patch2GuestAddrTree = 0;
        pPatchRec->patch.Guest2PatchAddrTree = 0;
        if (pPatchRec->patch.nrPatch2GuestRecs)
        {
            RECPATCHTOGUEST rec;
            uint32_t        nrPatch2GuestRecs = pPatchRec->patch.nrPatch2GuestRecs;

            pPatchRec->patch.nrPatch2GuestRecs = 0;    /* incremented by patmr3AddP2GLookupRecord */
            for (uint32_t j=0;j<nrPatch2GuestRecs;j++)
            {
                RT_ZERO(rec);
                rc = SSMR3GetStructEx(pSSM, &rec, sizeof(rec), fStructRestoreFlags, &g_aPatmRecPatchToGuest[0], NULL);
                AssertRCReturn(rc, rc);

                patmR3AddP2GLookupRecord(pVM, &pPatchRec->patch, (uintptr_t)rec.Core.Key + pVM->patm.s.pPatchMemHC, rec.pOrgInstrGC, rec.enmType, rec.fDirty);
            }
            Assert(pPatchRec->patch.Patch2GuestAddrTree);
        }

        if (pPatchRec->patch.flags & PATMFL_CODE_MONITORED)
        {
            /* Insert the guest page lookup records (for detection self-modifying code) */
            rc = patmInsertPatchPages(pVM, &pPatchRec->patch);
            AssertRCReturn(rc, rc);
        }

#if 0 /* can fail def LOG_ENABLED */
        if (    pPatchRec->patch.uState != PATCH_REFUSED
            &&  !(pPatchRec->patch.flags & PATMFL_INT3_REPLACEMENT))
        {
            pPatchRec->patch.pTempInfo = (PPATCHINFOTEMP)MMR3HeapAllocZ(pVM, MM_TAG_PATM_PATCH, sizeof(PATCHINFOTEMP));
            Log(("Patch code ----------------------------------------------------------\n"));
            patmr3DisasmCodeStream(pVM, PATCHCODE_PTR_GC(&pPatchRec->patch), PATCHCODE_PTR_GC(&pPatchRec->patch), patmr3DisasmCallback, &pPatchRec->patch);
            Log(("Patch code ends -----------------------------------------------------\n"));
            MMR3HeapFree(pPatchRec->patch.pTempInfo);
            pPatchRec->patch.pTempInfo = NULL;
        }
#endif
        /* Remove the patch in case the gc mapping is not present. */
        if (    !pPrivInstrHC
            &&  pPatchRec->patch.uState == PATCH_ENABLED)
        {
            Log(("Remove patch %RGv due to failed HC address translation\n", pPatchRec->patch.pPrivInstrGC));
            PATMR3RemovePatch(pVM, pPatchRec->patch.pPrivInstrGC);
        }
    }

    /*
     * Correct absolute fixups in the global patch. (helper functions)
     * Bit of a mess. Uses the new patch record, but restored patch functions.
     */
    PRELOCREC pRec = 0;
    AVLPVKEY  key  = 0;

    Log(("Correct fixups in global helper functions\n"));
    while (true)
    {
        int32_t offset;
        RTRCPTR *pFixup;

        /* Get the record that's closest from above */
        pRec = (PRELOCREC)RTAvlPVGetBestFit(&pVM->patm.s.pGlobalPatchRec->patch.FixupTree, key, true);
        if (pRec == 0)
            break;

        key = (AVLPVKEY)(pRec->pRelocPos + 1);   /* search for the next record during the next round. */

        /* rec.pRelocPos now contains the relative position inside the hypervisor area. */
        offset = (int32_t)(pRec->pRelocPos - pVM->patm.s.pPatchMemHC);
        pFixup = (RTRCPTR *)pRec->pRelocPos;

        /* Correct fixups that refer to PATM structures in the hypervisor region (their addresses might have changed). */
        rc = patmCorrectFixup(pVM, uVersion, patmInfo, &pVM->patm.s.pGlobalPatchRec->patch, pRec, offset, pFixup);
        AssertRCReturn(rc, rc);
    }

#ifdef VBOX_WITH_STATISTICS
    /*
     * Restore relevant old statistics
     */
    pVM->patm.s.StatDisabled  = patmInfo.StatDisabled;
    pVM->patm.s.StatUnusable  = patmInfo.StatUnusable;
    pVM->patm.s.StatEnabled   = patmInfo.StatEnabled;
    pVM->patm.s.StatInstalled = patmInfo.StatInstalled;
#endif

    return VINF_SUCCESS;
}

/**
 * Correct fixups to predefined hypervisor PATM regions. (their addresses might have changed)
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   uVersion        Saved state version.
 * @param   patmInfo        Saved PATM structure
 * @param   pPatch          Patch record
 * @param   pRec            Relocation record
 * @param   offset          Offset of referenced data/code
 * @param   pFixup          Fixup address
 */
static int patmCorrectFixup(PVM pVM, unsigned uVersion, PATM &patmInfo, PPATCHINFO pPatch, PRELOCREC pRec,
                            int32_t offset, RTRCPTR *pFixup)
{
    int32_t delta = pVM->patm.s.pPatchMemGC - patmInfo.pPatchMemGC;
    RT_NOREF1(offset);

    switch (pRec->uType)
    {
    case FIXUP_ABSOLUTE:
    case FIXUP_ABSOLUTE_IN_PATCH_ASM_TMPL:
    {
        Assert(   pRec->uType != PATM_SAVED_STATE_VERSION_NO_RAW_MEM
               || (pRec->pSource == pRec->pDest && PATM_IS_ASMFIX(pRec->pSource)) );

        /* bird: What is this for exactly?  Only the MMIO fixups used to have pSource set. */
        if (    pRec->pSource
            && !PATMIsPatchGCAddr(pVM, (RTRCUINTPTR)pRec->pSource)
            && pRec->uType != FIXUP_ABSOLUTE_IN_PATCH_ASM_TMPL)
            break;

        RTRCPTR const uFixup = *pFixup;
        if (    uFixup >= patmInfo.pGCStateGC
            &&  uFixup <  patmInfo.pGCStateGC + sizeof(PATMGCSTATE))
        {
            LogFlow(("Changing absolute GCState at %RRv from %RRv to %RRv\n", patmInfo.pPatchMemGC + offset, uFixup, (uFixup - patmInfo.pGCStateGC) + pVM->patm.s.pGCStateGC));
            *pFixup = (uFixup - patmInfo.pGCStateGC) + pVM->patm.s.pGCStateGC;
        }
        else if (   uFixup >= patmInfo.pCPUMCtxGC
                 && uFixup <  patmInfo.pCPUMCtxGC + sizeof(CPUMCTX))
        {
            LogFlow(("Changing absolute CPUMCTX at %RRv from %RRv to %RRv\n", patmInfo.pPatchMemGC + offset, uFixup, (uFixup - patmInfo.pCPUMCtxGC) + pVM->patm.s.pCPUMCtxGC));

            /* The CPUMCTX structure has completely changed, so correct the offsets too. */
            if (uVersion == PATM_SAVED_STATE_VERSION_VER16)
            {
                unsigned offCpumCtx = uFixup - patmInfo.pCPUMCtxGC;

                /* ''case RT_OFFSETOF()'' does not work as gcc refuses to use & as a constant expression.
                 * Defining RT_OFFSETOF as __builtin_offsetof for gcc would make this possible. But this
                 * function is not available in older gcc versions, at least not in gcc-3.3 */
                if (offCpumCtx == (unsigned)RT_OFFSETOF(CPUMCTX_VER1_6, dr0))
                {
                    LogFlow(("Changing dr[0] offset from %x to %x\n", offCpumCtx, RT_OFFSETOF(CPUMCTX, dr[0])));
                    *pFixup = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, dr[0]);
                }
                else if (offCpumCtx == (unsigned)RT_OFFSETOF(CPUMCTX_VER1_6, dr1))
                {
                    LogFlow(("Changing dr[1] offset from %x to %x\n", offCpumCtx, RT_OFFSETOF(CPUMCTX, dr[1])));
                    *pFixup = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, dr[1]);
                }
                else if (offCpumCtx == (unsigned)RT_OFFSETOF(CPUMCTX_VER1_6, dr2))
                {
                    LogFlow(("Changing dr[2] offset from %x to %x\n", offCpumCtx, RT_OFFSETOF(CPUMCTX, dr[2])));
                    *pFixup = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, dr[2]);
                }
                else if (offCpumCtx == (unsigned)RT_OFFSETOF(CPUMCTX_VER1_6, dr3))
                {
                    LogFlow(("Changing dr[3] offset from %x to %x\n", offCpumCtx, RT_OFFSETOF(CPUMCTX, dr[3])));
                    *pFixup = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, dr[3]);
                }
                else if (offCpumCtx == (unsigned)RT_OFFSETOF(CPUMCTX_VER1_6, dr4))
                {
                    LogFlow(("Changing dr[4] offset from %x to %x\n", offCpumCtx, RT_OFFSETOF(CPUMCTX, dr[4])));
                    *pFixup = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, dr[4]);
                }
                else if (offCpumCtx == (unsigned)RT_OFFSETOF(CPUMCTX_VER1_6, dr5))
                {
                    LogFlow(("Changing dr[5] offset from %x to %x\n", offCpumCtx, RT_OFFSETOF(CPUMCTX, dr[5])));
                    *pFixup = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, dr[5]);
                }
                else if (offCpumCtx == (unsigned)RT_OFFSETOF(CPUMCTX_VER1_6, dr6))
                {
                    LogFlow(("Changing dr[6] offset from %x to %x\n", offCpumCtx, RT_OFFSETOF(CPUMCTX, dr[6])));
                    *pFixup = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, dr[6]);
                }
                else if (offCpumCtx == (unsigned)RT_OFFSETOF(CPUMCTX_VER1_6, dr7))
                {
                    LogFlow(("Changing dr[7] offset from %x to %x\n", offCpumCtx, RT_OFFSETOF(CPUMCTX, dr[7])));
                    *pFixup = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, dr[7]);
                }
                else if (offCpumCtx == (unsigned)RT_OFFSETOF(CPUMCTX_VER1_6, cr0))
                {
                    LogFlow(("Changing cr0 offset from %x to %x\n", offCpumCtx, RT_OFFSETOF(CPUMCTX, cr0)));
                    *pFixup = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, cr0);
                }
                else if (offCpumCtx == (unsigned)RT_OFFSETOF(CPUMCTX_VER1_6, cr2))
                {
                    LogFlow(("Changing cr2 offset from %x to %x\n", offCpumCtx, RT_OFFSETOF(CPUMCTX, cr2)));
                    *pFixup = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, cr2);
                }
                else if (offCpumCtx == (unsigned)RT_OFFSETOF(CPUMCTX_VER1_6, cr3))
                {
                    LogFlow(("Changing cr3 offset from %x to %x\n", offCpumCtx, RT_OFFSETOF(CPUMCTX, cr3)));
                    *pFixup = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, cr3);
                }
                else if (offCpumCtx == (unsigned)RT_OFFSETOF(CPUMCTX_VER1_6, cr4))
                {
                    LogFlow(("Changing cr4 offset from %x to %x\n", offCpumCtx, RT_OFFSETOF(CPUMCTX, cr4)));
                    *pFixup = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, cr4);
                }
                else if (offCpumCtx == (unsigned)RT_OFFSETOF(CPUMCTX_VER1_6, tr))
                {
                    LogFlow(("Changing tr offset from %x to %x\n", offCpumCtx, RT_OFFSETOF(CPUMCTX, tr)));
                    *pFixup = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, tr);
                }
                else if (offCpumCtx == (unsigned)RT_OFFSETOF(CPUMCTX_VER1_6, ldtr))
                {
                    LogFlow(("Changing ldtr offset from %x to %x\n", offCpumCtx, RT_OFFSETOF(CPUMCTX, ldtr)));
                    *pFixup = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, ldtr);
                }
                else if (offCpumCtx == (unsigned)RT_OFFSETOF(CPUMCTX_VER1_6, gdtr.pGdt))
                {
                    LogFlow(("Changing pGdt offset from %x to %x\n", offCpumCtx, RT_OFFSETOF(CPUMCTX, gdtr.pGdt)));
                    *pFixup = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, gdtr.pGdt);
                }
                else if (offCpumCtx == (unsigned)RT_OFFSETOF(CPUMCTX_VER1_6, gdtr.cbGdt))
                {
                    LogFlow(("Changing cbGdt offset from %x to %x\n", offCpumCtx, RT_OFFSETOF(CPUMCTX, gdtr.cbGdt)));
                    *pFixup = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, gdtr.cbGdt);
                }
                else if (offCpumCtx == (unsigned)RT_OFFSETOF(CPUMCTX_VER1_6, idtr.pIdt))
                {
                    LogFlow(("Changing pIdt offset from %x to %x\n", offCpumCtx, RT_OFFSETOF(CPUMCTX, idtr.pIdt)));
                    *pFixup = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, idtr.pIdt);
                }
                else if (offCpumCtx == (unsigned)RT_OFFSETOF(CPUMCTX_VER1_6, idtr.cbIdt))
                {
                    LogFlow(("Changing cbIdt offset from %x to %x\n", offCpumCtx, RT_OFFSETOF(CPUMCTX, idtr.cbIdt)));
                    *pFixup = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, idtr.cbIdt);
                }
                else
                    AssertMsgFailed(("Unexpected CPUMCTX offset %x\n", offCpumCtx));
            }
            else
                *pFixup = (uFixup - patmInfo.pCPUMCtxGC) + pVM->patm.s.pCPUMCtxGC;
        }
        else if (   uFixup >= patmInfo.pStatsGC
                 && uFixup <  patmInfo.pStatsGC + PATM_STAT_MEMSIZE)
        {
            LogFlow(("Changing absolute Stats at %RRv from %RRv to %RRv\n", patmInfo.pPatchMemGC + offset, uFixup, (uFixup - patmInfo.pStatsGC) + pVM->patm.s.pStatsGC));
            *pFixup = (uFixup - patmInfo.pStatsGC) + pVM->patm.s.pStatsGC;
        }
        else if (   uFixup >= patmInfo.pGCStackGC
                 && uFixup <  patmInfo.pGCStackGC + PATM_STACK_TOTAL_SIZE)
        {
            LogFlow(("Changing absolute Stack at %RRv from %RRv to %RRv\n", patmInfo.pPatchMemGC + offset, uFixup, (uFixup - patmInfo.pGCStackGC) + pVM->patm.s.pGCStackGC));
            *pFixup = (uFixup - patmInfo.pGCStackGC) + pVM->patm.s.pGCStackGC;
        }
        else if (   uFixup >= patmInfo.pPatchMemGC
                 && uFixup <  patmInfo.pPatchMemGC + patmInfo.cbPatchMem)
        {
            LogFlow(("Changing absolute PatchMem at %RRv from %RRv to %RRv\n", patmInfo.pPatchMemGC + offset, uFixup, (uFixup - patmInfo.pPatchMemGC) + pVM->patm.s.pPatchMemGC));
            *pFixup = (uFixup - patmInfo.pPatchMemGC) + pVM->patm.s.pPatchMemGC;
        }
        /*
         * For PATM_SAVED_STATE_VERSION_FIXUP_HACK and earlier boldly ASSUME:
         * 1. That pCPUMCtxGC is in the VM structure and that its location is
         *    at the first page of the same 4 MB chunk.
         * 2. That the forced actions were in the first 32 bytes of the VM
         *    structure.
         * 3. That the CPUM leaves are less than 8KB into the structure.
         */
        else if (   uVersion <= PATM_SAVED_STATE_VERSION_FIXUP_HACK
                 && uFixup - (patmInfo.pCPUMCtxGC & UINT32_C(0xffc00000)) < UINT32_C(32))
        {
            LogFlow(("Changing fLocalForcedActions fixup from %RRv to %RRv\n", uFixup, pVM->pVMRC + RT_OFFSETOF(VM, aCpus[0].fLocalForcedActions)));
            *pFixup = pVM->pVMRC + RT_OFFSETOF(VM, aCpus[0].fLocalForcedActions);
            pRec->pSource = pRec->pDest = PATM_ASMFIX_VM_FORCEDACTIONS;
            pRec->uType   = FIXUP_ABSOLUTE_IN_PATCH_ASM_TMPL;
        }
        else if (   uVersion <= PATM_SAVED_STATE_VERSION_FIXUP_HACK
                 && uFixup - (patmInfo.pCPUMCtxGC & UINT32_C(0xffc00000)) < UINT32_C(8192))
        {
            static int cCpuidFixup = 0;

            /* Very dirty assumptions about the cpuid patch and cpuid ordering. */
            switch (cCpuidFixup & 3)
            {
            case 0:
                *pFixup = CPUMR3GetGuestCpuIdPatmDefRCPtr(pVM);
                pRec->pSource = pRec->pDest = PATM_ASMFIX_CPUID_DEF_PTR;
                pRec->uType   = FIXUP_ABSOLUTE_IN_PATCH_ASM_TMPL;
                break;
            case 1:
                *pFixup = CPUMR3GetGuestCpuIdPatmStdRCPtr(pVM);
                pRec->pSource = pRec->pDest = PATM_ASMFIX_CPUID_STD_PTR;
                pRec->uType   = FIXUP_ABSOLUTE_IN_PATCH_ASM_TMPL;
                break;
            case 2:
                *pFixup = CPUMR3GetGuestCpuIdPatmExtRCPtr(pVM);
                pRec->pSource = pRec->pDest = PATM_ASMFIX_CPUID_EXT_PTR;
                pRec->uType   = FIXUP_ABSOLUTE_IN_PATCH_ASM_TMPL;
                break;
            case 3:
                *pFixup = CPUMR3GetGuestCpuIdPatmCentaurRCPtr(pVM);
                pRec->pSource = pRec->pDest = PATM_ASMFIX_CPUID_CENTAUR_PTR;
                pRec->uType   = FIXUP_ABSOLUTE_IN_PATCH_ASM_TMPL;
                break;
            }
            LogFlow(("Changing cpuid fixup %d from %RRv to %RRv\n", cCpuidFixup, uFixup, *pFixup));
            cCpuidFixup++;
        }
        /*
         * For PATM_SAVED_STATE_VERSION_MEM thru PATM_SAVED_STATE_VERSION_NO_RAW_MEM
         * we abused Core.Key to store the type for fixups needing correcting on load.
         */
        else if (   uVersion >= PATM_SAVED_STATE_VERSION_MEM
                 && uVersion <= PATM_SAVED_STATE_VERSION_NO_RAW_MEM)
        {
            /* Core.Key abused to store the type of fixup. */
            switch ((uintptr_t)pRec->Core.Key)
            {
            case PATM_FIXUP_CPU_FF_ACTION:
                *pFixup = pVM->pVMRC + RT_OFFSETOF(VM, aCpus[0].fLocalForcedActions);
                pRec->pSource = pRec->pDest = PATM_ASMFIX_VM_FORCEDACTIONS;
                pRec->uType   = FIXUP_ABSOLUTE_IN_PATCH_ASM_TMPL;
                LogFlow(("Changing cpu ff action fixup from %x to %x\n", uFixup, *pFixup));
                break;
            case PATM_FIXUP_CPUID_DEFAULT:
                *pFixup = CPUMR3GetGuestCpuIdPatmDefRCPtr(pVM);
                pRec->pSource = pRec->pDest = PATM_ASMFIX_CPUID_DEF_PTR;
                pRec->uType   = FIXUP_ABSOLUTE_IN_PATCH_ASM_TMPL;
                LogFlow(("Changing cpuid def fixup from %x to %x\n", uFixup, *pFixup));
                break;
            case PATM_FIXUP_CPUID_STANDARD:
                *pFixup = CPUMR3GetGuestCpuIdPatmStdRCPtr(pVM);
                pRec->pSource = pRec->pDest = PATM_ASMFIX_CPUID_STD_PTR;
                pRec->uType   = FIXUP_ABSOLUTE_IN_PATCH_ASM_TMPL;
                LogFlow(("Changing cpuid std fixup from %x to %x\n", uFixup, *pFixup));
                break;
            case PATM_FIXUP_CPUID_EXTENDED:
                *pFixup = CPUMR3GetGuestCpuIdPatmExtRCPtr(pVM);
                pRec->pSource = pRec->pDest = PATM_ASMFIX_CPUID_EXT_PTR;
                pRec->uType   = FIXUP_ABSOLUTE_IN_PATCH_ASM_TMPL;
                LogFlow(("Changing cpuid ext fixup from %x to %x\n", uFixup, *pFixup));
                break;
            case PATM_FIXUP_CPUID_CENTAUR:
                *pFixup = CPUMR3GetGuestCpuIdPatmCentaurRCPtr(pVM);
                pRec->pSource = pRec->pDest = PATM_ASMFIX_CPUID_CENTAUR_PTR;
                pRec->uType   = FIXUP_ABSOLUTE_IN_PATCH_ASM_TMPL;
                LogFlow(("Changing cpuid centaur fixup from %x to %x\n", uFixup, *pFixup));
                break;
            default:
                AssertMsgFailed(("Unexpected fixup value %p\n", (uintptr_t)pRec->Core.Key));
                break;
            }
        }
        /*
         * After PATM_SAVED_STATE_VERSION_NO_RAW_MEM we changed the fixup type
         * and instead put the patch fixup code in the source and target addresses.
         */
        else if (   uVersion > PATM_SAVED_STATE_VERSION_NO_RAW_MEM
                 && pRec->uType == FIXUP_ABSOLUTE_IN_PATCH_ASM_TMPL)
        {
            Assert(pRec->pSource == pRec->pDest); Assert(PATM_IS_ASMFIX(pRec->pSource));
            switch (pRec->pSource)
            {
                case PATM_ASMFIX_VM_FORCEDACTIONS:
                    *pFixup = pVM->pVMRC + RT_OFFSETOF(VM, aCpus[0].fLocalForcedActions);
                    break;
                case PATM_ASMFIX_CPUID_DEF_PTR:
                    *pFixup = CPUMR3GetGuestCpuIdPatmDefRCPtr(pVM);
                    break;
                case PATM_ASMFIX_CPUID_STD_PTR: /* Saved again patches only. */
                    *pFixup = CPUMR3GetGuestCpuIdPatmStdRCPtr(pVM);
                    break;
                case PATM_ASMFIX_CPUID_EXT_PTR: /* Saved again patches only. */
                    *pFixup = CPUMR3GetGuestCpuIdPatmExtRCPtr(pVM);
                    break;
                case PATM_ASMFIX_CPUID_CENTAUR_PTR: /* Saved again patches only. */
                    *pFixup = CPUMR3GetGuestCpuIdPatmCentaurRCPtr(pVM);
                    break;
                case PATM_ASMFIX_REUSE_LATER_0: /* Was only used for a few days. Don't want to keep this legacy around.  */
                case PATM_ASMFIX_REUSE_LATER_1:
                    AssertLogRelMsgFailedReturn(("Unsupported PATM fixup. You have to discard this saved state or snapshot."),
                                                VERR_INTERNAL_ERROR);
                    break;
            }
        }
        /*
         * Constant that may change between VM version needs fixing up.
         */
        else if (pRec->uType == FIXUP_CONSTANT_IN_PATCH_ASM_TMPL)
        {
            AssertLogRelReturn(uVersion > PATM_SAVED_STATE_VERSION_NO_RAW_MEM, VERR_SSM_DATA_UNIT_FORMAT_CHANGED);
            Assert(pRec->pSource == pRec->pDest); Assert(PATM_IS_ASMFIX(pRec->pSource));
            switch (pRec->pSource)
            {
                case PATM_ASMFIX_REUSE_LATER_2: /* Was only used for a few days. Don't want to keep this legacy around.  */
                case PATM_ASMFIX_REUSE_LATER_3:
                    AssertLogRelMsgFailedReturn(("Unsupported PATM fixup. You have to discard this saved state or snapshot."),
                                                VERR_INTERNAL_ERROR);
                    break;
                default:
                    AssertLogRelMsgFailed(("Unknown FIXUP_CONSTANT_IN_PATCH_ASM_TMPL fixup: %#x\n", pRec->pSource));
                    return VERR_SSM_DATA_UNIT_FORMAT_CHANGED;
            }
        }
        /*
         * Relative fixups for calling or jumping to helper functions inside VMMRC.
         * (The distance between the helper function and the patch is subject to
         * new code being added to VMMRC as well as VM configurations influencing
         * heap allocations and so on and so forth.)
         */
        else if (pRec->uType == FIXUP_REL_HELPER_IN_PATCH_ASM_TMPL)
        {
            AssertLogRelReturn(uVersion > PATM_SAVED_STATE_VERSION_NO_RAW_MEM, VERR_SSM_DATA_UNIT_FORMAT_CHANGED);
            Assert(pRec->pSource == pRec->pDest); Assert(PATM_IS_ASMFIX(pRec->pSource));
            int     rc;
            RTRCPTR uRCPtrDest;
            switch (pRec->pSource)
            {
                case PATM_ASMFIX_HELPER_CPUM_CPUID:
                    rc = PDMR3LdrGetSymbolRC(pVM, NULL, "CPUMPatchHlpCpuId", &uRCPtrDest);
                    AssertLogRelRCReturn(rc, rc);
                    break;
                default:
                    AssertLogRelMsgFailed(("Unknown FIXUP_REL_HLP_CALL_IN_PATCH_ASM_TMPL fixup: %#x\n", pRec->pSource));
                    return VERR_SSM_DATA_UNIT_FORMAT_CHANGED;
            }
            RTRCPTR uRCPtrAfter = pVM->patm.s.pPatchMemGC + ((uintptr_t)&pFixup[1] - (uintptr_t)pVM->patm.s.pPatchMemHC);
            *pFixup = uRCPtrDest - uRCPtrAfter;
        }

#ifdef RT_OS_WINDOWS
        AssertCompile(RT_OFFSETOF(VM, fGlobalForcedActions) < 32);
#endif
        break;
    }

    case FIXUP_REL_JMPTOPATCH:
    {
        RTRCPTR pTarget = (RTRCPTR)((RTRCINTPTR)pRec->pDest + delta);

        if (    pPatch->uState == PATCH_ENABLED
            &&  (pPatch->flags & PATMFL_PATCHED_GUEST_CODE))
        {
            uint8_t    oldJump[SIZEOF_NEAR_COND_JUMP32];
            uint8_t    temp[SIZEOF_NEAR_COND_JUMP32];
            RTRCPTR    pJumpOffGC;
            RTRCINTPTR displ   = (RTRCINTPTR)pTarget - (RTRCINTPTR)pRec->pSource;
            RTRCINTPTR displOld= (RTRCINTPTR)pRec->pDest - (RTRCINTPTR)pRec->pSource;

            Log(("Relative fixup (g2p) %08X -> %08X at %08X (source=%08x, target=%08x)\n", *(int32_t*)pRec->pRelocPos, displ, pRec->pRelocPos, pRec->pSource, pRec->pDest));

            Assert(pRec->pSource - pPatch->cbPatchJump == pPatch->pPrivInstrGC);
#ifdef PATM_RESOLVE_CONFLICTS_WITH_JUMP_PATCHES
            if (pPatch->cbPatchJump == SIZEOF_NEAR_COND_JUMP32)
            {
                Assert(pPatch->flags & PATMFL_JUMP_CONFLICT);

                pJumpOffGC = pPatch->pPrivInstrGC + 2;    //two byte opcode
                oldJump[0] = pPatch->aPrivInstr[0];
                oldJump[1] = pPatch->aPrivInstr[1];
                *(RTRCUINTPTR *)&oldJump[2] = displOld;
            }
            else
#endif
            if (pPatch->cbPatchJump == SIZEOF_NEARJUMP32)
            {
                pJumpOffGC = pPatch->pPrivInstrGC + 1;    //one byte opcode
                oldJump[0] = 0xE9;
                *(RTRCUINTPTR *)&oldJump[1] = displOld;
            }
            else
            {
                AssertMsgFailed(("Invalid patch jump size %d\n", pPatch->cbPatchJump));
                break;
            }
            Assert(pPatch->cbPatchJump <= sizeof(temp));

            /*
             * Read old patch jump and compare it to the one we previously installed
             */
            int rc = PGMPhysSimpleReadGCPtr(VMMGetCpu0(pVM), temp, pPatch->pPrivInstrGC, pPatch->cbPatchJump);
            Assert(RT_SUCCESS(rc) || rc == VERR_PAGE_NOT_PRESENT || rc == VERR_PAGE_TABLE_NOT_PRESENT);

            if (rc == VERR_PAGE_NOT_PRESENT || rc == VERR_PAGE_TABLE_NOT_PRESENT)
            {
                RTRCPTR pPage = pPatch->pPrivInstrGC & PAGE_BASE_GC_MASK;
                rc = PGMR3HandlerVirtualRegister(pVM, VMMGetCpu(pVM), pVM->patm.s.hMonitorPageType,
                                                 pPage,
                                                 pPage + (PAGE_SIZE - 1) /* inclusive! */,
                                                 (void *)(uintptr_t)pPage, pPage, NULL /*pszDesc*/);
                Assert(RT_SUCCESS(rc) || rc == VERR_PGM_HANDLER_VIRTUAL_CONFLICT);
            }
            else
            if (memcmp(temp, oldJump, pPatch->cbPatchJump))
            {
                Log(("PATM: Patch jump was overwritten -> disabling patch!!\n"));
                /*
                 * Disable patch; this is not a good solution
                 */
                /** @todo hopefully it was completely overwritten (if the read was successful)!!!! */
                pPatch->uState = PATCH_DISABLED;
            }
            else
            if (RT_SUCCESS(rc))
            {
                rc = PGMPhysSimpleDirtyWriteGCPtr(VMMGetCpu0(pVM), pJumpOffGC, &displ, sizeof(displ));
                AssertRC(rc);
            }
            else
                AssertMsgFailed(("Unexpected error %d from MMR3PhysReadGCVirt\n", rc));
        }
        else
            Log(("Skip the guest jump to patch code for this disabled patch %08X\n", pRec->pRelocPos));

        pRec->pDest = pTarget;
        break;
    }

    case FIXUP_REL_JMPTOGUEST:
    {
        RTRCPTR    pSource = (RTRCPTR)((RTRCINTPTR)pRec->pSource + delta);
        RTRCINTPTR displ   = (RTRCINTPTR)pRec->pDest - (RTRCINTPTR)pSource;

        Assert(!(pPatch->flags & PATMFL_GLOBAL_FUNCTIONS));
        Log(("Relative fixup (p2g) %08X -> %08X at %08X (source=%08x, target=%08x)\n", *(int32_t*)pRec->pRelocPos, displ, pRec->pRelocPos, pRec->pSource, pRec->pDest));
        *(RTRCUINTPTR *)pRec->pRelocPos = displ;
        pRec->pSource = pSource;
        break;

    }
    }
    return VINF_SUCCESS;
}

