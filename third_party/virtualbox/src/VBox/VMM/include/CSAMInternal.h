/* $Id: CSAMInternal.h $ */
/** @file
 * CSAM - Internal header file.
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

#ifndef ___CSAMInternal_h
#define ___CSAMInternal_h

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/vmm/csam.h>
#include <VBox/dis.h>
#include <VBox/log.h>



/** @name Page flags.
 * These are placed in the three bits available for system programs in
 * the page entries.
 * @{ */
#ifndef PGM_PTFLAGS_CSAM_VALIDATED
/** Scanned and approved by CSAM (tm). */
/** NOTE: Must be identical to the one defined in PGMInternal.h!! */
#define PGM_PTFLAGS_CSAM_VALIDATED              RT_BIT_64(11)
#endif

/** @} */

#define CSAM_SAVED_STATE_VERSION                CSAM_SAVED_STATE_VERSION_PUT_STRUCT
#define CSAM_SAVED_STATE_VERSION_PUT_STRUCT     15
#define CSAM_SAVED_STATE_VERSION_PUT_MEM        14

#define CSAM_PGDIRBMP_CHUNKS                    1024

#define CSAM_PAGE_BITMAP_SIZE                   (PAGE_SIZE/(sizeof(uint8_t)*8))

/* Maximum nr of dirty page that are cached. */
#define CSAM_MAX_DIRTY_PAGES                    32

/* Maximum number of cached addresses of dangerous instructions that have been scanned before. */
#define CSAM_MAX_DANGR_INSTR                    16 /* power of two! */
#define CSAM_MAX_DANGR_INSTR_MASK               (CSAM_MAX_DANGR_INSTR-1)

/* Maximum number of possible dangerous code pages that we'll flush after a world switch */
#define CSAM_MAX_CODE_PAGES_FLUSH               32

#define CSAM_MAX_CALLEXIT_RET                   16

/* copy from PATMInternal.h */
#define SIZEOF_NEARJUMP32                       5 //opcode byte + 4 byte relative offset

typedef struct
{
  RTRCPTR         pInstrAfterRetGC[CSAM_MAX_CALLEXIT_RET];
  uint32_t          cInstrAfterRet;
} CSAMCALLEXITREC, *PCSAMCALLEXITREC;

typedef struct
{
    R3PTRTYPE(uint8_t *) pPageLocStartHC;
    R3PTRTYPE(uint8_t *) pPageLocEndHC;
    RCPTRTYPE(uint8_t *) pGuestLoc;
    uint32_t             depth;  //call/jump depth

    PCSAMCALLEXITREC     pCallExitRec;

    PGMPAGEMAPLOCK       Lock;
} CSAMP2GLOOKUPREC, *PCSAMP2GLOOKUPREC;

typedef struct
{
    RTRCPTR       pPageGC;
    RTGCPHYS        GCPhys;
    uint64_t        fFlags;
    uint32_t        uSize;

    uint8_t        *pBitmap;

    bool            fCode32;
    bool            fMonitorActive;
    bool            fMonitorInvalidation;

    CSAMTAG         enmTag;

    uint64_t        u64Hash;
} CSAMPAGE, *PCSAMPAGE;

typedef struct
{
    // GC Patch pointer
    RTRCPTR       pInstrGC;

    // Disassembly state for original instruction
    DISCPUSTATE     cpu;

    uint32_t        uState;

    PCSAMPAGE       pPage;
} CSAMPATCH, *PCSAMPATCH;

/**
 * Lookup record for CSAM pages
 */
typedef struct CSAMPAGEREC
{
    /** The key is a GC virtual address. */
    AVLPVNODECORE   Core;
    CSAMPAGE        page;

} CSAMPAGEREC, *PCSAMPAGEREC;

/**
 * Lookup record for patches
 */
typedef struct CSAMPATCHREC
{
    /** The key is a GC virtual address. */
    AVLPVNODECORE   Core;
    CSAMPATCH       patch;

} CSAMPATCHREC, *PCSAMPATCHREC;


/**
 * CSAM VM Instance data.
 * Changes to this must checked against the padding of the CSAM union in VM!
 * @note change SSM version when changing it!!
 */
typedef struct CSAM
{
    /** Offset to the VM structure.
     * See CSAM2VM(). */
    RTINT               offVM;
#if HC_ARCH_BITS == 64
    RTINT               Alignment0;     /**< Align pPageTree correctly. */
#endif

    R3PTRTYPE(PAVLPVNODECORE) pPageTree;

    /* Array to store previously scanned dangerous instructions, so we don't need to
     * switch back to ring 3 each time we encounter them in GC.
     */
    RTRCPTR           aDangerousInstr[CSAM_MAX_DANGR_INSTR];
    uint32_t            cDangerousInstr;
    uint32_t            iDangerousInstr;

    RCPTRTYPE(RTRCPTR *)  pPDBitmapGC;
    RCPTRTYPE(RTHCPTR *)  pPDHCBitmapGC;
    R3PTRTYPE(uint8_t **) pPDBitmapHC;
    R3PTRTYPE(RTRCPTR  *) pPDGCBitmapHC;

    /* Temporary storage during load/save state */
    struct
    {
        R3PTRTYPE(PSSMHANDLE) pSSM;
        uint32_t        cPageRecords;
        uint32_t        cPatchPageRecords;
    } savedstate;

    /* To keep track of dirty pages */
    uint32_t            cDirtyPages;
    RTRCPTR             pvDirtyBasePage[CSAM_MAX_DIRTY_PAGES];
    RTRCPTR             pvDirtyFaultPage[CSAM_MAX_DIRTY_PAGES];

    /* To keep track of possible code pages */
    uint32_t            cPossibleCodePages;
    RTRCPTR             pvPossibleCodePage[CSAM_MAX_CODE_PAGES_FLUSH];

    /* call addresses reported by the recompiler */
    RTRCPTR             pvCallInstruction[16];
    uint32_t            iCallInstruction;

    /** Code page write access handler type. */
    PGMVIRTHANDLERTYPE  hCodePageWriteType;
    /** Code page write & invalidation access handler type. */
    PGMVIRTHANDLERTYPE  hCodePageWriteAndInvPgType;

    /* Set when scanning has started. */
    bool                fScanningStarted;

    /* Set when the IDT gates have been checked for the first time. */
    bool                fGatesChecked;
    bool                Alignment1[HC_ARCH_BITS == 32 ? 6 : 2]; /**< Align the stats on an 8-byte boundary. */

    STAMCOUNTER         StatNrTraps;
    STAMCOUNTER         StatNrPages;
    STAMCOUNTER         StatNrPagesInv;
    STAMCOUNTER         StatNrRemovedPages;
    STAMCOUNTER         StatNrPatchPages;
    STAMCOUNTER         StatNrPageNPHC;
    STAMCOUNTER         StatNrPageNPGC;
    STAMCOUNTER         StatNrFlushes;
    STAMCOUNTER         StatNrFlushesSkipped;
    STAMCOUNTER         StatNrKnownPagesHC;
    STAMCOUNTER         StatNrKnownPagesGC;
    STAMCOUNTER         StatNrInstr;
    STAMCOUNTER         StatNrBytesRead;
    STAMCOUNTER         StatNrOpcodeRead;
    STAMPROFILE         StatTime;
    STAMPROFILE         StatTimeCheckAddr;
    STAMPROFILE         StatTimeAddrConv;
    STAMPROFILE         StatTimeFlushPage;
    STAMPROFILE         StatTimeDisasm;
    STAMPROFILE         StatFlushDirtyPages;
    STAMPROFILE         StatCheckGates;
    STAMCOUNTER         StatCodePageModified;
    STAMCOUNTER         StatDangerousWrite;

    STAMCOUNTER         StatInstrCacheHit;
    STAMCOUNTER         StatInstrCacheMiss;

    STAMCOUNTER         StatPagePATM;
    STAMCOUNTER         StatPageCSAM;
    STAMCOUNTER         StatPageREM;
    STAMCOUNTER         StatNrUserPages;
    STAMCOUNTER         StatPageMonitor;
    STAMCOUNTER         StatPageRemoveREMFlush;

    STAMCOUNTER         StatBitmapAlloc;

    STAMCOUNTER         StatScanNextFunction;
    STAMCOUNTER         StatScanNextFunctionFailed;
} CSAM, *PCSAM;

/**
 * Call for analyzing the instructions following the privileged instr. for compliance with our heuristics
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pCpu        CPU disassembly state
 * @param   pInstrHC    Guest context pointer to privileged instruction
 * @param   pCurInstrGC Guest context pointer to current instruction
 * @param   pUserData   User pointer
 *
 */
typedef int (VBOXCALL *PFN_CSAMR3ANALYSE)(PVM pVM, DISCPUSTATE *pCpu, RCPTRTYPE(uint8_t *) pInstrGC, RCPTRTYPE(uint8_t *) pCurInstrGC, PCSAMP2GLOOKUPREC pCacheRec, void *pUserData);

/**
 * Calculate the branch destination
 *
 * @returns branch destination or 0 if failed
 * @param   pCpu            Disassembly state of instruction.
 * @param   pBranchInstrGC  GC pointer of branch instruction
 */
inline RTRCPTR CSAMResolveBranch(PDISCPUSTATE pCpu, RTRCPTR pBranchInstrGC)
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

PGM_ALL_CB2_PROTO(FNPGMVIRTHANDLER) csamCodePageWriteHandler;
RT_C_DECLS_BEGIN
DECLEXPORT(FNPGMRCVIRTPFHANDLER)    csamRCCodePageWritePfHandler;
RT_C_DECLS_END

#endif
