/* $Id: PATMInternal.h $ */
/** @file
 * PATM - Internal header file.
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

#ifndef ___PATMInternal_h
#define ___PATMInternal_h

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/vmm/patm.h>
#include <VBox/vmm/stam.h>
#include <VBox/dis.h>
#include <VBox/vmm/pgm.h>
#include <iprt/avl.h>
#include <iprt/param.h>
#include <VBox/log.h>


/** @name Saved state version numbers.
 * @{ */
/** New concept of helper code (for CPUID). */
#define PATM_SAVED_STATE_VERSION                    58
/** New fixup type FIXUP_ABSOLUTE_IN_PATCH_ASM_TMPL. */
#define PATM_SAVED_STATE_VERSION_FORGET_THIS_ONE    57
/** Uses normal structure serialization with markers and everything. */
#define PATM_SAVED_STATE_VERSION_NO_RAW_MEM         56
/** Last version which saves structures as raw memory. */
#define PATM_SAVED_STATE_VERSION_MEM                55
#define PATM_SAVED_STATE_VERSION_FIXUP_HACK         54
#define PATM_SAVED_STATE_VERSION_FIXUP_HACK         54
#define PATM_SAVED_STATE_VERSION_VER16              53
/** @}  */

/* Enable for call patching. */
#define PATM_ENABLE_CALL
#define PATCH_MEMORY_SIZE                  (2*1024*1024)
#define MAX_PATCH_SIZE                     (1024*4)

/*
 * Internal patch type flags (starts at RT_BIT(11))
 */

#define PATMFL_CHECK_SIZE                   RT_BIT_64(11)
#define PATMFL_FOUND_PATCHEND               RT_BIT_64(12)
#define PATMFL_SINGLE_INSTRUCTION           RT_BIT_64(13)
#define PATMFL_SYSENTER_XP                  RT_BIT_64(14)
#define PATMFL_JUMP_CONFLICT                RT_BIT_64(15)
#define PATMFL_READ_ORIGINAL_BYTES          RT_BIT_64(16) /** opcode might have already been patched */
#define PATMFL_INT3_REPLACEMENT             RT_BIT_64(17)
#define PATMFL_SUPPORT_CALLS                RT_BIT_64(18)
#define PATMFL_SUPPORT_INDIRECT_CALLS       RT_BIT_64(19)
#define PATMFL_IDTHANDLER_WITHOUT_ENTRYPOINT RT_BIT_64(20) /** internal flag to avoid duplicate entrypoints */
#define PATMFL_INHIBIT_IRQS                 RT_BIT_64(21) /** temporary internal flag */
#define PATMFL_GENERATE_JUMPTOGUEST         RT_BIT_64(22) /** temporary internal flag */
#define PATMFL_RECOMPILE_NEXT               RT_BIT_64(23) /** for recompilation of the next instruction */
#define PATMFL_CODE_MONITORED               RT_BIT_64(24) /** code pages of guest monitored for self-modifying code. */
#define PATMFL_CALLABLE_AS_FUNCTION         RT_BIT_64(25) /** cli and pushf blocks can be used as callable functions. */
#define PATMFL_GLOBAL_FUNCTIONS             RT_BIT_64(26) /** fake patch for global patm functions. */
#define PATMFL_TRAMPOLINE                   RT_BIT_64(27) /** trampoline patch that clears PATM_ASMFIX_INTERRUPTFLAG and jumps to patch destination */
#define PATMFL_GENERATE_SETPIF              RT_BIT_64(28) /** generate set PIF for the next instruction */
#define PATMFL_INSTR_HINT                   RT_BIT_64(29) /** Generate patch, but don't activate it. */
#define PATMFL_PATCHED_GUEST_CODE           RT_BIT_64(30) /** Patched guest code. */
#define PATMFL_MUST_INSTALL_PATCHJMP        RT_BIT_64(31) /** Need to patch guest code in order to activate patch. */
#define PATMFL_INT3_REPLACEMENT_BLOCK       RT_BIT_64(32) /** int 3 replacement block */
#define PATMFL_EXTERNAL_JUMP_INSIDE         RT_BIT_64(33) /** A trampoline patch was created that jumps to an instruction in the patch block */
#define PATMFL_CODE_REFERENCED              RT_BIT_64(34) /** patch block referenced (called, jumped to) by another patch. */

#define SIZEOF_NEARJUMP8                   2 //opcode byte + 1 byte relative offset
#define SIZEOF_NEARJUMP16                  3 //opcode byte + 2 byte relative offset
#define SIZEOF_NEARJUMP32                  5 //opcode byte + 4 byte relative offset
#define SIZEOF_NEAR_COND_JUMP32            6 //0xF + opcode byte + 4 byte relative offset

#define MAX_INSTR_SIZE                     16

/* Patch states */
#define PATCH_REFUSED                     1
#define PATCH_DISABLED                    2
#define PATCH_ENABLED                     4
#define PATCH_UNUSABLE                    8
#define PATCH_DIRTY                       16
#define PATCH_DISABLE_PENDING             32


#define MAX_PATCH_TRAPS                    4
#define PATM_MAX_CALL_DEPTH                32
/* Maximum nr of writes before a patch is marked dirty. (disabled) */
#define PATM_MAX_CODE_WRITES               32
/* Maximum nr of invalid writes before a patch is disabled. */
#define PATM_MAX_INVALID_WRITES            16384

/** @name FIXUP_XXX - RELOCREC::uType values.
 * @{ */
/** Absolute fixup.  With one exception (MMIO cache), this does not take any
 * source or destination.  @sa FIXUP_ABSOLUTE_ASM.  */
#define FIXUP_ABSOLUTE                     0
#define FIXUP_REL_JMPTOPATCH               1
#define FIXUP_REL_JMPTOGUEST               2
/** Absolute fixup in patch assembly code template.
 *
 * The source and desination addresses both set to the patch fixup type (see
 * PATM_IS_ASMFIX and friends in PATMA.h).  This is recent addition (CPUID
 * subleaf code), so when loading older saved states this is usally represented
 * as FIXUP_ABSOLUTE. */
#define FIXUP_ABSOLUTE_IN_PATCH_ASM_TMPL   3
/** Constant value that only needs fixing up when loading state.  Structure
 * size, member offset, or similar.  The source and destination address are set
 * like for FIXUP_ABSOLUTE_IN_PATCH_ASM_TMPL.  */
#define FIXUP_CONSTANT_IN_PATCH_ASM_TMPL   4
/** Relative call to a patch helper routine in VMMRC.  The source and destination
 * address are set like for FIXUP_ABSOLUTE_IN_PATCH_ASM_TMPL.  */
#define FIXUP_REL_HELPER_IN_PATCH_ASM_TMPL 5
/** @} */


#define PATM_ILLEGAL_DESTINATION           0xDEADBEEF

/** Size of the instruction that's used for requests from patch code (currently only call) */
#define PATM_ILLEGAL_INSTR_SIZE            2


/** No statistics counter index allocated just yet */
#define PATM_STAT_INDEX_NONE                (uint32_t)-1
/** Dummy counter to handle overflows */
#define PATM_STAT_INDEX_DUMMY               0
#define PATM_STAT_INDEX_IS_VALID(a)         (a != PATM_STAT_INDEX_DUMMY && a != PATM_STAT_INDEX_NONE)

#ifdef VBOX_WITH_STATISTICS
#define PATM_STAT_RUN_INC(pPatch)                                             \
        if (PATM_STAT_INDEX_IS_VALID((pPatch)->uPatchIdx))                \
            CTXSUFF(pVM->patm.s.pStats)[(pPatch)->uPatchIdx].u32A++;
#define PATM_STAT_FAULT_INC(pPatch)                                           \
        if (PATM_STAT_INDEX_IS_VALID((pPatch)->uPatchIdx))                \
            CTXSUFF(pVM->patm.s.pStats)[(pPatch)->uPatchIdx].u32B++;
#else
#define PATM_STAT_RUN_INC(pPatch)           do { } while (0)
#define PATM_STAT_FAULT_INC(pPatch)         do { } while (0)
#endif

/** Maximum number of stat counters. */
#define PATM_STAT_MAX_COUNTERS              1024
/** Size of memory allocated for patch statistics. */
#define PATM_STAT_MEMSIZE                   (PATM_STAT_MAX_COUNTERS*sizeof(STAMRATIOU32))

/** aCpus[0].fLocalForcedActions fixup (must be uneven to avoid theoretical clashes with valid pointers) */
#define PATM_FIXUP_CPU_FF_ACTION            0xffffff01
/** default cpuid pointer fixup */
#define PATM_FIXUP_CPUID_DEFAULT            0xffffff03
/** standard cpuid pointer fixup */
#define PATM_FIXUP_CPUID_STANDARD           0xffffff05
/** extended cpuid pointer fixup */
#define PATM_FIXUP_CPUID_EXTENDED           0xffffff07
/** centaur cpuid pointer fixup */
#define PATM_FIXUP_CPUID_CENTAUR            0xffffff09

typedef struct
{
    /** The key is a HC virtual address. */
    AVLPVNODECORE               Core;

    uint32_t                    uType;
    R3PTRTYPE(uint8_t *)        pRelocPos;
    RTRCPTR                     pSource;
    RTRCPTR                     pDest;
} RELOCREC, *PRELOCREC;

/* Cache record for guest to host pointer conversions. */
typedef struct
{
    R3PTRTYPE(uint8_t *)        pPageLocStartHC;
    RCPTRTYPE(uint8_t *)        pGuestLoc;
    R3PTRTYPE(void *)           pPatch;
    PGMPAGEMAPLOCK              Lock;
} PATMP2GLOOKUPREC, *PPATMP2GLOOKUPREC;

/* Obsolete; do not use. */
typedef struct
{
    R3PTRTYPE(uint8_t *)        pPatchLocStartHC;
    R3PTRTYPE(uint8_t *)        pPatchLocEndHC;
    RCPTRTYPE(uint8_t *)        pGuestLoc;
    uint32_t                    opsize;
} PATMP2GLOOKUPREC_OBSOLETE;

typedef struct
{
    /** The key is a pointer to a JUMPREC structure. */
    AVLPVNODECORE               Core;

    R3PTRTYPE(uint8_t *)        pJumpHC;
    RCPTRTYPE(uint8_t *)        pTargetGC;
    uint32_t                    offDispl;
    uint32_t                    opcode;
} JUMPREC, *PJUMPREC;

/**
 * Patch to guest lookup type (single or both direction)
 */
typedef enum
{
    /** patch to guest */
    PATM_LOOKUP_PATCH2GUEST,
    /** guest to patch + patch to guest */
    PATM_LOOKUP_BOTHDIR
} PATM_LOOKUP_TYPE;

/**
 * Patch to guest address lookup record.
 */
typedef struct RECPATCHTOGUEST
{
    /** The key is an offset inside the patch memory block. */
    AVLU32NODECORE              Core;
    /** GC address of the guest instruction this record is for. */
    RTRCPTR                     pOrgInstrGC;
    /** Patch to guest lookup type. */
    PATM_LOOKUP_TYPE            enmType;
    /** Flag whether the original instruction was changed by the guest. */
    bool                        fDirty;
    /** Flag whether this guest instruction is a jump target from
     * a trampoline patch. */
    bool                        fJumpTarget;
    /** Original opcode before writing 0xCC there to mark it dirty. */
    uint8_t                     u8DirtyOpcode;
} RECPATCHTOGUEST, *PRECPATCHTOGUEST;

/**
 * Guest to patch address lookup record
 */
typedef struct RECGUESTTOPATCH
{
    /** The key is a GC virtual address. */
    AVLU32NODECORE              Core;
    /** Patch offset (relative to PATM::pPatchMemGC / PATM::pPatchMemHC). */
    uint32_t                    PatchOffset;
} RECGUESTTOPATCH, *PRECGUESTTOPATCH;

/**
 * Temporary information used in ring 3 only; no need to waste memory in the patch record itself.
 */
typedef struct
{
    /* Temporary tree for storing the addresses of illegal instructions. */
    R3PTRTYPE(PAVLPVNODECORE)   IllegalInstrTree;
    uint32_t                    nrIllegalInstr;

    int32_t                     nrJumps;
    uint32_t                    nrRetInstr;

    /* Temporary tree of encountered jumps. (debug only) */
    R3PTRTYPE(PAVLPVNODECORE)   DisasmJumpTree;

    int32_t                     nrCalls;

    /** Last original guest instruction pointer; used for disassembly log. */
    RTRCPTR                     pLastDisasmInstrGC;

    /** Keeping track of multiple ret instructions. */
    RTRCPTR                     pPatchRetInstrGC;
    uint32_t                    uPatchRetParam1;
} PATCHINFOTEMP, *PPATCHINFOTEMP;

/** Forward declaration for a pointer to a trampoline patch record. */
typedef struct TRAMPREC *PTRAMPREC;

/**
 * Patch information.
 */
typedef struct PATCHINFO
{
    /** Current patch state (enabled, disabled, etc.). */
    uint32_t                    uState;
    /** Previous patch state. Used when enabling a disabled patch. */
    uint32_t                    uOldState;
    /** CPU mode (16bit or 32bit). */
    DISCPUMODE                  uOpMode;
    /** GC pointer of privileged instruction */
    RCPTRTYPE(uint8_t *)        pPrivInstrGC;
    /** @todo: Can't remove due to structure size dependencies in saved states. */
    R3PTRTYPE(uint8_t *)        unusedHC;
    /** Original privileged guest instructions overwritten by the jump patch. */
    uint8_t                     aPrivInstr[MAX_INSTR_SIZE];
    /** Number of valid bytes in the instruction buffer. */
    uint32_t                    cbPrivInstr;
    /** Opcode for priv instr (OP_*). */
    uint32_t                    opcode;
    /** Size of the patch jump in the guest code. */
    uint32_t                    cbPatchJump;
    /** Only valid for PATMFL_JUMP_CONFLICT patches */
    RTRCPTR                     pPatchJumpDestGC;
    /** Offset of the patch code from the beginning of the patch memory area. */
    RTGCUINTPTR32               pPatchBlockOffset;
    /** Size of the patch code in bytes. */
    uint32_t                    cbPatchBlockSize;
    /** Current offset of the patch starting from pPatchBlockOffset.
     * Used during patch creation. */
    uint32_t                    uCurPatchOffset;
#if HC_ARCH_BITS == 64
    uint32_t                    Alignment0;         /**< Align flags correctly. */
#endif
    /** PATM flags (see PATMFL_*). */
    uint64_t                    flags;
    /**
     * Lowest and highest patched GC instruction address. To optimize searches.
     */
    RTRCPTR                     pInstrGCLowest;
    RTRCPTR                     pInstrGCHighest;
    /* Tree of fixup records for the patch. */
    R3PTRTYPE(PAVLPVNODECORE)   FixupTree;
    uint32_t                    nrFixups;
    /* Tree of jumps inside the generated patch code. */
    uint32_t                    nrJumpRecs;
    R3PTRTYPE(PAVLPVNODECORE)   JumpTree;
    /**
     * Lookup trees for determining the corresponding guest address of an
     * instruction in the patch block.
     */
    R3PTRTYPE(PAVLU32NODECORE)  Patch2GuestAddrTree;
    R3PTRTYPE(PAVLU32NODECORE)  Guest2PatchAddrTree;
    uint32_t                    nrPatch2GuestRecs;
#if HC_ARCH_BITS == 64
    uint32_t                    Alignment1;
#endif
    /** Unused, but can't remove due to structure size dependencies in the saved state. */
    PATMP2GLOOKUPREC_OBSOLETE   unused;
    /** Temporary information during patch creation. Don't waste hypervisor memory for this. */
    R3PTRTYPE(PPATCHINFOTEMP)   pTempInfo;
    /** List of trampoline patches referencing this patch.
     * Used when refreshing the patch. (Only for function duplicates) */
    R3PTRTYPE(PTRAMPREC)        pTrampolinePatchesHead;
    /** Count the number of writes to the corresponding guest code. */
    uint32_t                    cCodeWrites;
    /** Some statistics to determine if we should keep this patch activated. */
    uint32_t                    cTraps;
    /** Count the number of invalid writes to pages monitored for the patch. */
    uint32_t                    cInvalidWrites;
    /** Index into the uPatchRun and uPatchTrap arrays (0..MAX_PATCHES-1) */
    uint32_t                    uPatchIdx;
    /** First opcode byte, that's overwritten when a patch is marked dirty. */
    uint8_t                     bDirtyOpcode;
    /** Align the structure size on a 8-byte boundary. */
    uint8_t                     Alignment2[HC_ARCH_BITS == 64 ? 7 : 3];
} PATCHINFO, *PPATCHINFO;

#define PATCHCODE_PTR_GC(pPatch)    (RTRCPTR)  (pVM->patm.s.pPatchMemGC + (pPatch)->pPatchBlockOffset)
#define PATCHCODE_PTR_HC(pPatch)    (uint8_t *)(pVM->patm.s.pPatchMemHC + (pPatch)->pPatchBlockOffset)

/**
 * Lookup record for patches
 */
typedef struct PATMPATCHREC
{
    /** The key is a GC virtual address. */
    AVLOU32NODECORE             Core;
    /** The key is a patch offset. */
    AVLOU32NODECORE             CoreOffset;
    /** The patch information. */
    PATCHINFO                   patch;
} PATMPATCHREC, *PPATMPATCHREC;

/**
 * Record for a trampoline patch.
 */
typedef struct TRAMPREC
{
    /** Pointer to the next trampoline patch. */
    struct TRAMPREC            *pNext;
    /** Pointer to the trampoline patch record. */
    PPATMPATCHREC               pPatchTrampoline;
} TRAMPREC;

/** Increment for allocating room for pointer array */
#define PATMPATCHPAGE_PREALLOC_INCREMENT        16

/**
 * Lookup record for patch pages
 */
typedef struct PATMPATCHPAGE
{
    /** The key is a GC virtual address. */
    AVLOU32NODECORE             Core;
    /** Region to monitor. */
    RTRCPTR                     pLowestAddrGC;
    RTRCPTR                     pHighestAddrGC;
    /** Number of patches for this page. */
    uint32_t                    cCount;
    /** Maximum nr of pointers in the array. */
    uint32_t                    cMaxPatches;
    /** Array of patch pointers for this page. */
    R3PTRTYPE(PPATCHINFO *)     papPatch;
} PATMPATCHPAGE, *PPATMPATCHPAGE;

#define PATM_PATCHREC_FROM_COREOFFSET(a)  (PPATMPATCHREC)((uintptr_t)a - RT_OFFSETOF(PATMPATCHREC, CoreOffset))
#define PATM_PATCHREC_FROM_PATCHINFO(a)   (PPATMPATCHREC)((uintptr_t)a - RT_OFFSETOF(PATMPATCHREC, patch))

/**
 * AVL trees used by PATM.
 */
typedef struct PATMTREES
{
    /**
     * AVL tree with all patches (active or disabled) sorted by guest instruction address
     */
    AVLOU32TREE                 PatchTree;

    /**
     * AVL tree with all patches sorted by patch address (offset actually)
     */
    AVLOU32TREE                 PatchTreeByPatchAddr;

    /**
     * AVL tree with all pages which were (partly) patched
     */
    AVLOU32TREE                 PatchTreeByPage;

    uint32_t                    align[1];
} PATMTREES, *PPATMTREES;

/**
 * PATM VM Instance data.
 * Changes to this must checked against the padding of the patm union in VM!
 */
typedef struct PATM
{
    /** Offset to the VM structure.
     * See PATM2VM(). */
    RTINT                       offVM;
    /** Pointer to the patch memory area (GC) */
    RCPTRTYPE(uint8_t *)        pPatchMemGC;
    /** Pointer to the patch memory area (HC) */
    R3PTRTYPE(uint8_t *)        pPatchMemHC;
    /** Size of the patch memory area in bytes. */
    uint32_t                    cbPatchMem;
    /** Relative offset to the next free byte starting from the start of the region. */
    uint32_t                    offPatchMem;
    /** Flag whether PATM ran out of patch memory. */
    bool                        fOutOfMemory;
    /** Delta to the new relocated HMA area.
     * Used only during PATMR3Relocate(). */
    int32_t                     deltaReloc;

    /** The ring-3 address of the PatchHlp segment (for PATMReadPatchCode). */
    R3PTRTYPE(uint8_t *)        pbPatchHelpersR3;
    /** The raw-mode address of the PatchHlp segment. */
    RCPTRTYPE(uint8_t *)        pbPatchHelpersRC;
    /** Size of the PatchHlp segment containing the callable helper code.   */
    uint32_t                    cbPatchHelpers;

    /** GC PATM state pointer - HC pointer. */
    R3PTRTYPE(PPATMGCSTATE)     pGCStateHC;
    /** GC PATM state pointer - RC pointer. */
    RCPTRTYPE(PPATMGCSTATE)     pGCStateGC;

    /** PATM stack page for call instruction execution.
     * 2 parts: one for our private stack and one to store the original return
     * address. */
    RCPTRTYPE(RTRCPTR *)        pGCStackGC;
    /** HC pointer of the PATM stack page. */
    R3PTRTYPE(RTRCPTR *)        pGCStackHC;
    /** GC pointer to CPUMCTX structure. */
    RCPTRTYPE(PCPUMCTX)         pCPUMCtxGC;

    /** GC statistics pointer. */
    RCPTRTYPE(PSTAMRATIOU32)    pStatsGC;
    /** HC statistics pointer. */
    R3PTRTYPE(PSTAMRATIOU32)    pStatsHC;

    /** Current free index value (uPatchRun/uPatchTrap arrays). */
    uint32_t                    uCurrentPatchIdx;
    /** Temporary counter for patch installation call depth. (in order not to go on forever) */
    uint32_t                    ulCallDepth;
    /** Number of page lookup records. */
    uint32_t                    cPageRecords;
    /** Lowest and highest patched GC instruction addresses. To optimize searches. */
    RTRCPTR                     pPatchedInstrGCLowest;
    RTRCPTR                     pPatchedInstrGCHighest;
    /** Pointer to the patch tree for instructions replaced by 'int 3'. */
    RCPTRTYPE(PPATMTREES)       PatchLookupTreeGC;
    R3PTRTYPE(PPATMTREES)       PatchLookupTreeHC;
    /** Global PATM lookup and call function (used by call patches). */
    RTRCPTR                     pfnHelperCallGC;
    /** Global PATM return function (used by ret patches). */
    RTRCPTR                     pfnHelperRetGC;
    /** Global PATM jump function (used by indirect jmp patches). */
    RTRCPTR                     pfnHelperJumpGC;
    /** Global PATM return function (used by iret patches). */
    RTRCPTR                     pfnHelperIretGC;
    /** Fake patch record for global functions. */
    R3PTRTYPE(PPATMPATCHREC)    pGlobalPatchRec;
    /** Pointer to original sysenter handler */
    RTRCPTR                     pfnSysEnterGC;
    /** Pointer to sysenter handler trampoline */
    RTRCPTR                     pfnSysEnterPatchGC;
    /** Sysenter patch index (for stats only) */
    uint32_t                    uSysEnterPatchIdx;
    /** GC address of fault in monitored page (set by PATMGCMonitorPage, used by PATMR3HandleMonitoredPage)- */
    RTRCPTR                     pvFaultMonitor;
    /** Temporary information for pending MMIO patch. Set in GC or R0 context. */
    struct
    {
        RTGCPHYS                GCPhys;
        RTRCPTR                 pCachedData;
        RTRCPTR                 Alignment0; /**< Align the structure size on a 8-byte boundary. */
    } mmio;
    /** Temporary storage during load/save state */
    struct
    {
        R3PTRTYPE(PSSMHANDLE)   pSSM;
        uint32_t                cPatches;
#if HC_ARCH_BITS == 64
        uint32_t                Alignment0; /**< Align the structure size on a 8-byte boundary. */
#endif
    } savedstate;

    /** Debug module for the patch memory. */
    RTDBGMOD                    hDbgModPatchMem;

    /** Virtual page access handler type (patmVirtPageHandler,
     * PATMGCMonitorPage). */
    PGMVIRTHANDLERTYPE          hMonitorPageType;

    /** Align statistics on a 8 byte boundary. */
    uint32_t                    u32Alignment1;

    STAMCOUNTER                 StatNrOpcodeRead;
    STAMCOUNTER                 StatDisabled;
    STAMCOUNTER                 StatUnusable;
    STAMCOUNTER                 StatEnabled;
    STAMCOUNTER                 StatInstalled;
    STAMCOUNTER                 StatInstalledFunctionPatches;
    STAMCOUNTER                 StatInstalledTrampoline;
    STAMCOUNTER                 StatInstalledJump;
    STAMCOUNTER                 StatInt3Callable;
    STAMCOUNTER                 StatInt3BlockRun;
    STAMCOUNTER                 StatOverwritten;
    STAMCOUNTER                 StatFixedConflicts;
    STAMCOUNTER                 StatFlushed;
    STAMCOUNTER                 StatPageBoundaryCrossed;
    STAMCOUNTER                 StatMonitored;
    STAMPROFILEADV              StatHandleTrap;
    STAMCOUNTER                 StatSwitchBack;
    STAMCOUNTER                 StatSwitchBackFail;
    STAMCOUNTER                 StatPATMMemoryUsed;
    STAMCOUNTER                 StatDuplicateREQSuccess;
    STAMCOUNTER                 StatDuplicateREQFailed;
    STAMCOUNTER                 StatDuplicateUseExisting;
    STAMCOUNTER                 StatFunctionFound;
    STAMCOUNTER                 StatFunctionNotFound;
    STAMPROFILEADV              StatPatchWrite;
    STAMPROFILEADV              StatPatchWriteDetect;
    STAMCOUNTER                 StatDirty;
    STAMCOUNTER                 StatPushTrap;
    STAMCOUNTER                 StatPatchWriteInterpreted;
    STAMCOUNTER                 StatPatchWriteInterpretedFailed;

    STAMCOUNTER                 StatSysEnter;
    STAMCOUNTER                 StatSysExit;
    STAMCOUNTER                 StatEmulIret;
    STAMCOUNTER                 StatEmulIretFailed;

    STAMCOUNTER                 StatInstrDirty;
    STAMCOUNTER                 StatInstrDirtyGood;
    STAMCOUNTER                 StatInstrDirtyBad;

    STAMCOUNTER                 StatPatchPageInserted;
    STAMCOUNTER                 StatPatchPageRemoved;

    STAMCOUNTER                 StatPatchRefreshSuccess;
    STAMCOUNTER                 StatPatchRefreshFailed;

    STAMCOUNTER                 StatGenRet;
    STAMCOUNTER                 StatGenRetReused;
    STAMCOUNTER                 StatGenJump;
    STAMCOUNTER                 StatGenCall;
    STAMCOUNTER                 StatGenPopf;

    STAMCOUNTER                 StatCheckPendingIRQ;

    STAMCOUNTER                 StatFunctionLookupReplace;
    STAMCOUNTER                 StatFunctionLookupInsert;
    uint32_t                    StatU32FunctionMaxSlotsUsed;
    uint32_t                    Alignment0; /**< Align the structure size on a 8-byte boundary. */
} PATM, *PPATM;



DECLCALLBACK(int) patmR3Save(PVM pVM, PSSMHANDLE pSSM);
DECLCALLBACK(int) patmR3Load(PVM pVM, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass);

#ifdef IN_RING3
RTRCPTR         patmPatchGCPtr2GuestGCPtr(PVM pVM, PPATCHINFO pPatch, RCPTRTYPE(uint8_t *) pPatchGC);
RTRCPTR         patmGuestGCPtrToPatchGCPtr(PVM pVM, PPATCHINFO pPatch, RCPTRTYPE(uint8_t*) pInstrGC);
RTRCPTR         patmGuestGCPtrToClosestPatchGCPtr(PVM pVM, PPATCHINFO pPatch, RCPTRTYPE(uint8_t*) pInstrGC);
#endif

void            patmR3AddP2GLookupRecord(PVM pVM, PPATCHINFO pPatch, uint8_t *pPatchInstrHC, RTRCPTR pInstrGC,
                                         PATM_LOOKUP_TYPE enmType, bool fDirty = false);
int             patmInsertPatchPages(PVM pVM, PPATCHINFO pPatch);
RTRCPTR         patmPatchQueryStatAddress(PVM pVM, PPATCHINFO pPatch);
int             patmR3RemovePatch(PVM pVM, PPATMPATCHREC pPatchRec, bool fForceRemove);

/**
 * Call for analysing the instructions following the privileged instr. for compliance with our heuristics
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pCpu        CPU disassembly state
 * @param   pInstrHC    Guest context pointer to privileged instruction
 * @param   pCurInstrHC Guest context pointer to current instruction
 * @param   pCacheRec   Cache record ptr
 *
 */
typedef int (VBOXCALL *PFN_PATMR3ANALYSE)(PVM pVM, DISCPUSTATE *pCpu, RCPTRTYPE(uint8_t *) pInstrGC, RCPTRTYPE(uint8_t *) pCurInstrGC, PPATMP2GLOOKUPREC pCacheRec);

int             patmR3InstallGuestSpecificPatch(PVM pVM, PDISCPUSTATE pCpu, RTRCPTR pInstrGC, uint8_t *pInstrHC, PPATMPATCHREC pPatchRec);
PPATMPATCHREC   patmQueryFunctionPatch(PVM pVM, RTRCPTR pInstrGC);
const char     *patmGetInstructionString(uint32_t opcode, uint32_t fPatchFlags);

PPATCHINFO      patmFindActivePatchByEntrypoint(PVM pVM, RTRCPTR pInstrGC, bool fIncludeHints = false);
int             patmR3PatchInstrInt3(PVM pVM, RTRCPTR pInstrGC, R3PTRTYPE(uint8_t *) pInstrHC, DISCPUSTATE *pCpu, PPATCHINFO pPatch);
int             patmAddBranchToLookupCache(PVM pVM, RTRCPTR pJumpTableGC, RTRCPTR pBranchTarget, RTRCUINTPTR pRelBranchPatch);
R3PTRTYPE(uint8_t *) patmR3GCVirtToHCVirt(PVM pVM, PPATMP2GLOOKUPREC pCacheRec, RCPTRTYPE(uint8_t *) pGCPtr);

RT_C_DECLS_BEGIN
DECLEXPORT(FNPGMRCVIRTPFHANDLER) patmRCVirtPagePfHandler;
RT_C_DECLS_END

/**
 * Calculate the branch destination
 *
 * @returns branch destination or 0 if failed
 * @param   pCpu            Disassembly state of instruction.
 * @param   pBranchInstrGC  GC pointer of branch instruction
 */
DECLINLINE(RTRCPTR) PATMResolveBranch(PDISCPUSTATE pCpu, RTRCPTR pBranchInstrGC)
{
    uint32_t disp;
    if (pCpu->Param1.fUse & DISUSE_IMMEDIATE8_REL)
    {
        disp = (int32_t)(char)pCpu->Param1.uValue;
    }
    else
    if (pCpu->Param1.fUse & DISUSE_IMMEDIATE16_REL)
    {
        disp = (int32_t)(uint16_t)pCpu->Param1.uValue;
    }
    else
    if (pCpu->Param1.fUse & DISUSE_IMMEDIATE32_REL)
    {
        disp = (int32_t)pCpu->Param1.uValue;
    }
    else
    {
        Log(("We don't support far jumps here!! (%08X)\n", pCpu->Param1.fUse));
        return 0;
    }
#ifdef IN_RC
    return (RTRCPTR)((uint8_t *)pBranchInstrGC + pCpu->cbInstr + disp);
#else
    return pBranchInstrGC + pCpu->cbInstr + disp;
#endif
}

#ifdef LOG_ENABLED
DECLCALLBACK(int) patmR3DisasmCallback(PVM pVM, DISCPUSTATE *pCpu, RCPTRTYPE(uint8_t *) pInstrGC, RCPTRTYPE(uint8_t *) pCurInstrGC, PPATMP2GLOOKUPREC pCacheRec);
int patmr3DisasmCodeStream(PVM pVM, RCPTRTYPE(uint8_t *) pInstrGC, RCPTRTYPE(uint8_t *) pCurInstrGC, PFN_PATMR3ANALYSE pfnPATMR3Analyse, PPATMP2GLOOKUPREC pCacheRec);
#endif


void patmR3DbgInit(PVM pVM);
void patmR3DbgTerm(PVM pVM);
void patmR3DbgReset(PVM pVM);
void patmR3DbgAddPatch(PVM pVM, PPATMPATCHREC pPatchRec);

PGM_ALL_CB2_PROTO(FNPGMVIRTHANDLER) patmVirtPageHandler;

#endif
