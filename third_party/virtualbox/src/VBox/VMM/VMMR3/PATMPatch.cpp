/* $Id: PATMPatch.cpp $ */
/** @file
 * PATMPatch - Dynamic Guest OS Instruction patches
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
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/trpm.h>
#include <VBox/vmm/csam.h>
#include "PATMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/param.h>

#include <VBox/err.h>
#include <VBox/log.h>
#include <VBox/dis.h>
#include <VBox/disopcode.h>

#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/string.h>

#include "PATMA.h"
#include "PATMPatch.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Internal structure for passing more information about call fixups to
 * patmPatchGenCode.
 */
typedef struct
{
    RTRCPTR   pTargetGC;
    RTRCPTR   pCurInstrGC;
    RTRCPTR   pNextInstrGC;
    RTRCPTR   pReturnGC;
} PATMCALLINFO, *PPATMCALLINFO;


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** Value to use when not sure about the patch size. */
#define PATCHGEN_DEF_SIZE   256

#define PATCHGEN_PROLOG_NODEF(pVM, pPatch, a_cbMaxEmit) \
    do { \
        cbGivenPatchSize = (a_cbMaxEmit) + 16U /*jmp++*/; \
        if (RT_LIKELY((pPatch)->pPatchBlockOffset + pPatch->uCurPatchOffset + cbGivenPatchSize < pVM->patm.s.cbPatchMem)) \
            pPB = PATCHCODE_PTR_HC(pPatch) + pPatch->uCurPatchOffset; \
        else \
        { \
            pVM->patm.s.fOutOfMemory = true; \
            AssertMsgFailed(("offPatch=%#x + offEmit=%#x + a_cbMaxEmit=%#x + jmp -->  cbTotalWithFudge=%#x >= cbPatchMem=%#x", \
                             (pPatch)->pPatchBlockOffset, pPatch->uCurPatchOffset, a_cbMaxEmit, \
                             (pPatch)->pPatchBlockOffset + pPatch->uCurPatchOffset + cbGivenPatchSize, pVM->patm.s.cbPatchMem)); \
            return VERR_NO_MEMORY; \
        } \
    } while (0)

#define PATCHGEN_PROLOG(pVM, pPatch, a_cbMaxEmit) \
    uint8_t *pPB; \
    uint32_t cbGivenPatchSize; \
    PATCHGEN_PROLOG_NODEF(pVM, pPatch, a_cbMaxEmit)

#define PATCHGEN_EPILOG(pPatch, a_cbActual) \
    do { \
        AssertMsg((a_cbActual) <= cbGivenPatchSize, ("a_cbActual=%#x cbGivenPatchSize=%#x\n", a_cbActual, cbGivenPatchSize)); \
        Assert((a_cbActual) <= 640); \
        pPatch->uCurPatchOffset += (a_cbActual); \
    } while (0)




int patmPatchAddReloc32(PVM pVM, PPATCHINFO pPatch, uint8_t *pRelocHC, uint32_t uType,
                        RTRCPTR pSource /*= 0*/, RTRCPTR pDest /*= 0*/)
{
    PRELOCREC pRec;

    Assert(   uType == FIXUP_ABSOLUTE
           || (   (   uType == FIXUP_ABSOLUTE_IN_PATCH_ASM_TMPL
                   || uType == FIXUP_CONSTANT_IN_PATCH_ASM_TMPL
                   || uType == FIXUP_REL_HELPER_IN_PATCH_ASM_TMPL)
               && pSource == pDest
               && PATM_IS_ASMFIX(pSource))
           || ((uType == FIXUP_REL_JMPTOPATCH || uType == FIXUP_REL_JMPTOGUEST) && pSource && pDest));

    LogFlow(("patmPatchAddReloc32 type=%d pRelocGC=%RRv source=%RRv dest=%RRv\n", uType, pRelocHC - pVM->patm.s.pPatchMemGC + pVM->patm.s.pPatchMemGC , pSource, pDest));

    pRec = (PRELOCREC)MMR3HeapAllocZ(pVM, MM_TAG_PATM_PATCH, sizeof(*pRec));
    Assert(pRec);
    pRec->Core.Key  = (AVLPVKEY)pRelocHC;
    pRec->pRelocPos = pRelocHC; /** @todo redundant. */
    pRec->pSource   = pSource;
    pRec->pDest     = pDest;
    pRec->uType     = uType;

    bool ret = RTAvlPVInsert(&pPatch->FixupTree, &pRec->Core);
    Assert(ret); NOREF(ret);
    pPatch->nrFixups++;

    return VINF_SUCCESS;
}

int patmPatchAddJump(PVM pVM, PPATCHINFO pPatch, uint8_t *pJumpHC, uint32_t offset, RTRCPTR pTargetGC, uint32_t opcode)
{
    PJUMPREC pRec;

    pRec = (PJUMPREC)MMR3HeapAllocZ(pVM, MM_TAG_PATM_PATCH, sizeof(*pRec));
    Assert(pRec);

    pRec->Core.Key  = (AVLPVKEY)pJumpHC;
    pRec->pJumpHC   = pJumpHC; /** @todo redundant. */
    pRec->offDispl  = offset;
    pRec->pTargetGC = pTargetGC;
    pRec->opcode    = opcode;

    bool ret = RTAvlPVInsert(&pPatch->JumpTree, &pRec->Core);
    Assert(ret); NOREF(ret);
    pPatch->nrJumpRecs++;

    return VINF_SUCCESS;
}

static uint32_t patmPatchGenCode(PVM pVM, PPATCHINFO pPatch, uint8_t *pPB, PCPATCHASMRECORD pAsmRecord,
                                 RCPTRTYPE(uint8_t *) pReturnAddrGC, bool fGenJump,
                                 PPATMCALLINFO pCallInfo = 0)
{
    Assert(fGenJump == false || pReturnAddrGC);
    Assert(fGenJump == false || pAsmRecord->offJump);
    Assert(pAsmRecord);
    Assert(pAsmRecord->cbFunction > sizeof(pAsmRecord->aRelocs[0].uType) * pAsmRecord->cRelocs);

    // Copy the code block
    memcpy(pPB, pAsmRecord->pbFunction, pAsmRecord->cbFunction);

    // Process all fixups
    uint32_t i, j;
    for (j = 0, i = 0; i < pAsmRecord->cRelocs; i++)
    {
        for (; j < pAsmRecord->cbFunction; j++)
        {
            if (*(uint32_t*)&pPB[j] == pAsmRecord->aRelocs[i].uType)
            {
                RCPTRTYPE(uint32_t *) dest;

#ifdef VBOX_STRICT
                if (pAsmRecord->aRelocs[i].uType == PATM_ASMFIX_FIXUP)
                    Assert(pAsmRecord->aRelocs[i].uInfo != 0);
                else
                    Assert(pAsmRecord->aRelocs[i].uInfo == 0);
#endif

                /*
                 * BE VERY CAREFUL WITH THESE FIXUPS. TAKE INTO ACCOUNT THAT PROBLEMS MAY ARISE WHEN RESTORING
                 * A SAVED STATE WITH A DIFFERENT HYPERVISOR LAYOUT.
                 */
                uint32_t uRelocType = FIXUP_ABSOLUTE_IN_PATCH_ASM_TMPL;
                switch (pAsmRecord->aRelocs[i].uType)
                {
                    /*
                     * PATMGCSTATE member fixups.
                     */
                    case PATM_ASMFIX_VMFLAGS:
                        dest = pVM->patm.s.pGCStateGC + RT_OFFSETOF(PATMGCSTATE, uVMFlags);
                        break;
                    case PATM_ASMFIX_PENDINGACTION:
                        dest = pVM->patm.s.pGCStateGC + RT_OFFSETOF(PATMGCSTATE, uPendingAction);
                        break;
                    case PATM_ASMFIX_STACKPTR:
                        dest = pVM->patm.s.pGCStateGC + RT_OFFSETOF(PATMGCSTATE, Psp);
                        break;
                    case PATM_ASMFIX_INTERRUPTFLAG:
                        dest = pVM->patm.s.pGCStateGC + RT_OFFSETOF(PATMGCSTATE, fPIF);
                        break;
                    case PATM_ASMFIX_INHIBITIRQADDR:
                        dest = pVM->patm.s.pGCStateGC + RT_OFFSETOF(PATMGCSTATE, GCPtrInhibitInterrupts);
                        break;
                    case PATM_ASMFIX_TEMP_EAX:
                        dest = pVM->patm.s.pGCStateGC + RT_OFFSETOF(PATMGCSTATE, Restore.uEAX);
                        break;
                    case PATM_ASMFIX_TEMP_ECX:
                        dest = pVM->patm.s.pGCStateGC + RT_OFFSETOF(PATMGCSTATE, Restore.uECX);
                        break;
                    case PATM_ASMFIX_TEMP_EDI:
                        dest = pVM->patm.s.pGCStateGC + RT_OFFSETOF(PATMGCSTATE, Restore.uEDI);
                        break;
                    case PATM_ASMFIX_TEMP_EFLAGS:
                        dest = pVM->patm.s.pGCStateGC + RT_OFFSETOF(PATMGCSTATE, Restore.eFlags);
                        break;
                    case PATM_ASMFIX_TEMP_RESTORE_FLAGS:
                        dest = pVM->patm.s.pGCStateGC + RT_OFFSETOF(PATMGCSTATE, Restore.uFlags);
                        break;
                    case PATM_ASMFIX_CALL_PATCH_TARGET_ADDR:
                        dest = pVM->patm.s.pGCStateGC + RT_OFFSETOF(PATMGCSTATE, GCCallPatchTargetAddr);
                        break;
                    case PATM_ASMFIX_CALL_RETURN_ADDR:
                        dest = pVM->patm.s.pGCStateGC + RT_OFFSETOF(PATMGCSTATE, GCCallReturnAddr);
                        break;
#ifdef VBOX_WITH_STATISTICS
                    case PATM_ASMFIX_ALLPATCHCALLS:
                        dest = pVM->patm.s.pGCStateGC + RT_OFFSETOF(PATMGCSTATE, uPatchCalls);
                        break;
                    case PATM_ASMFIX_IRETEFLAGS:
                        dest = pVM->patm.s.pGCStateGC + RT_OFFSETOF(PATMGCSTATE, uIretEFlags);
                        break;
                    case PATM_ASMFIX_IRETCS:
                        dest = pVM->patm.s.pGCStateGC + RT_OFFSETOF(PATMGCSTATE, uIretCS);
                        break;
                    case PATM_ASMFIX_IRETEIP:
                        dest = pVM->patm.s.pGCStateGC + RT_OFFSETOF(PATMGCSTATE, uIretEIP);
                        break;
#endif


                    case PATM_ASMFIX_FIXUP:
                        /* Offset in aRelocs[i].uInfo is from the base of the function. */
                        dest = (RTGCUINTPTR32)pVM->patm.s.pPatchMemGC + pAsmRecord->aRelocs[i].uInfo
                             + (RTGCUINTPTR32)(pPB - pVM->patm.s.pPatchMemHC);
                        break;

#ifdef VBOX_WITH_STATISTICS
                    case PATM_ASMFIX_PERPATCHCALLS:
                        dest = patmPatchQueryStatAddress(pVM, pPatch);
                        break;
#endif

                    /* The first part of our PATM stack is used to store offsets of patch return addresses; the 2nd
                     * part to store the original return addresses.
                     */
                    case PATM_ASMFIX_STACKBASE:
                        dest = pVM->patm.s.pGCStackGC;
                        break;

                    case PATM_ASMFIX_STACKBASE_GUEST:
                        dest = pVM->patm.s.pGCStackGC + PATM_STACK_SIZE;
                        break;

                    case PATM_ASMFIX_RETURNADDR:   /* absolute guest address; no fixup required */
                        Assert(pCallInfo && pAsmRecord->aRelocs[i].uType >= PATM_ASMFIX_NO_FIXUP);
                        dest = pCallInfo->pReturnGC;
                        break;

                    case PATM_ASMFIX_PATCHNEXTBLOCK:  /* relative address of instruction following this block */
                        Assert(pCallInfo && pAsmRecord->aRelocs[i].uType >= PATM_ASMFIX_NO_FIXUP);

                        /** @note hardcoded assumption that we must return to the instruction following this block */
                        dest = (uintptr_t)pPB - (uintptr_t)pVM->patm.s.pPatchMemHC + pAsmRecord->cbFunction;
                        break;

                    case PATM_ASMFIX_CALLTARGET:   /* relative to patch address; no fixup required */
                        Assert(pCallInfo && pAsmRecord->aRelocs[i].uType >= PATM_ASMFIX_NO_FIXUP);

                        /* Address must be filled in later. (see patmr3SetBranchTargets)  */
                        patmPatchAddJump(pVM, pPatch, &pPB[j-1], 1, pCallInfo->pTargetGC, OP_CALL);
                        dest = PATM_ILLEGAL_DESTINATION;
                        break;

                    case PATM_ASMFIX_PATCHBASE:    /* Patch GC base address */
                        dest = pVM->patm.s.pPatchMemGC;
                        break;

                    case PATM_ASMFIX_NEXTINSTRADDR:
                        Assert(pCallInfo);
                        /* pNextInstrGC can be 0 if several instructions, that inhibit irqs, follow each other */
                        dest = pCallInfo->pNextInstrGC;
                        break;

                    case PATM_ASMFIX_CURINSTRADDR:
                        Assert(pCallInfo);
                        dest = pCallInfo->pCurInstrGC;
                        break;

                    /* Relative address of global patm lookup and call function. */
                    case PATM_ASMFIX_LOOKUP_AND_CALL_FUNCTION:
                    {
                        RTRCPTR pInstrAfterCall = pVM->patm.s.pPatchMemGC
                                                + (RTGCUINTPTR32)(&pPB[j] + sizeof(RTRCPTR) - pVM->patm.s.pPatchMemHC);
                        Assert(pVM->patm.s.pfnHelperCallGC);
                        Assert(sizeof(uint32_t) == sizeof(RTRCPTR));

                        /* Relative value is target minus address of instruction after the actual call instruction. */
                        dest = pVM->patm.s.pfnHelperCallGC - pInstrAfterCall;
                        break;
                    }

                    case PATM_ASMFIX_RETURN_FUNCTION:
                    {
                        RTRCPTR pInstrAfterCall = pVM->patm.s.pPatchMemGC
                                                + (RTGCUINTPTR32)(&pPB[j] + sizeof(RTRCPTR) - pVM->patm.s.pPatchMemHC);
                        Assert(pVM->patm.s.pfnHelperRetGC);
                        Assert(sizeof(uint32_t) == sizeof(RTRCPTR));

                        /* Relative value is target minus address of instruction after the actual call instruction. */
                        dest = pVM->patm.s.pfnHelperRetGC - pInstrAfterCall;
                        break;
                    }

                    case PATM_ASMFIX_IRET_FUNCTION:
                    {
                        RTRCPTR pInstrAfterCall = pVM->patm.s.pPatchMemGC
                                                + (RTGCUINTPTR32)(&pPB[j] + sizeof(RTRCPTR) - pVM->patm.s.pPatchMemHC);
                        Assert(pVM->patm.s.pfnHelperIretGC);
                        Assert(sizeof(uint32_t) == sizeof(RTRCPTR));

                        /* Relative value is target minus address of instruction after the actual call instruction. */
                        dest = pVM->patm.s.pfnHelperIretGC - pInstrAfterCall;
                        break;
                    }

                    case PATM_ASMFIX_LOOKUP_AND_JUMP_FUNCTION:
                    {
                        RTRCPTR pInstrAfterCall = pVM->patm.s.pPatchMemGC
                                                + (RTGCUINTPTR32)(&pPB[j] + sizeof(RTRCPTR) - pVM->patm.s.pPatchMemHC);
                        Assert(pVM->patm.s.pfnHelperJumpGC);
                        Assert(sizeof(uint32_t) == sizeof(RTRCPTR));

                        /* Relative value is target minus address of instruction after the actual call instruction. */
                        dest = pVM->patm.s.pfnHelperJumpGC - pInstrAfterCall;
                        break;
                    }

                    case PATM_ASMFIX_CPUID_STD_MAX: /* saved state only */
                        dest = CPUMR3GetGuestCpuIdPatmStdMax(pVM);
                        break;
                    case PATM_ASMFIX_CPUID_EXT_MAX: /* saved state only */
                        dest = CPUMR3GetGuestCpuIdPatmExtMax(pVM);
                        break;
                    case PATM_ASMFIX_CPUID_CENTAUR_MAX: /* saved state only */
                        dest = CPUMR3GetGuestCpuIdPatmCentaurMax(pVM);
                        break;

                    /*
                     * The following fixups needs to be recalculated when loading saved state
                     * Note! Earlier saved state versions had different hacks for detecting some of these.
                     */
                    case PATM_ASMFIX_VM_FORCEDACTIONS:
                        dest = pVM->pVMRC + RT_OFFSETOF(VM, aCpus[0].fLocalForcedActions);
                        break;

                    case PATM_ASMFIX_CPUID_DEF_PTR: /* saved state only */
                        dest = CPUMR3GetGuestCpuIdPatmDefRCPtr(pVM);
                        break;
                    case PATM_ASMFIX_CPUID_STD_PTR: /* saved state only */
                        dest = CPUMR3GetGuestCpuIdPatmStdRCPtr(pVM);
                        break;
                    case PATM_ASMFIX_CPUID_EXT_PTR: /* saved state only */
                        dest = CPUMR3GetGuestCpuIdPatmExtRCPtr(pVM);
                        break;
                    case PATM_ASMFIX_CPUID_CENTAUR_PTR: /* saved state only */
                        dest = CPUMR3GetGuestCpuIdPatmCentaurRCPtr(pVM);
                        break;

                    /*
                     * The following fixups are constants and helper code calls that only
                     * needs to be corrected when loading saved state.
                     */
                    case PATM_ASMFIX_HELPER_CPUM_CPUID:
                    {
                        int rc = PDMR3LdrGetSymbolRC(pVM, NULL, "CPUMPatchHlpCpuId", &dest);
                        AssertReleaseRCBreakStmt(rc, dest = PATM_ILLEGAL_DESTINATION);
                        uRelocType = FIXUP_REL_HELPER_IN_PATCH_ASM_TMPL;
                        break;
                    }

                    /*
                     * Unknown fixup.
                     */
                    case PATM_ASMFIX_REUSE_LATER_0:
                    case PATM_ASMFIX_REUSE_LATER_1:
                    case PATM_ASMFIX_REUSE_LATER_2:
                    case PATM_ASMFIX_REUSE_LATER_3:
                    default:
                        AssertReleaseMsgFailed(("Unknown fixup: %#x\n", pAsmRecord->aRelocs[i].uType));
                        dest = PATM_ILLEGAL_DESTINATION;
                        break;
                }

                if (uRelocType == FIXUP_REL_HELPER_IN_PATCH_ASM_TMPL)
                {
                    RTRCUINTPTR RCPtrAfter = pVM->patm.s.pPatchMemGC
                                           + (RTRCUINTPTR)(&pPB[j + sizeof(RTRCPTR)] - pVM->patm.s.pPatchMemHC);
                    dest -= RCPtrAfter;
                }

                *(PRTRCPTR)&pPB[j] = dest;

                if (pAsmRecord->aRelocs[i].uType < PATM_ASMFIX_NO_FIXUP)
                {
                    patmPatchAddReloc32(pVM, pPatch, &pPB[j], uRelocType,
                                        pAsmRecord->aRelocs[i].uType /*pSources*/, pAsmRecord->aRelocs[i].uType /*pDest*/);
                }
                break;
            }
        }
        Assert(j < pAsmRecord->cbFunction);
    }
    Assert(pAsmRecord->aRelocs[i].uInfo == 0xffffffff);

    /* Add the jump back to guest code (if required) */
    if (fGenJump)
    {
        int32_t displ = pReturnAddrGC - (PATCHCODE_PTR_GC(pPatch) + pPatch->uCurPatchOffset + pAsmRecord->offJump - 1 + SIZEOF_NEARJUMP32);

        /* Add lookup record for patch to guest address translation */
        Assert(pPB[pAsmRecord->offJump - 1] == 0xE9);
        patmR3AddP2GLookupRecord(pVM, pPatch, &pPB[pAsmRecord->offJump - 1], pReturnAddrGC, PATM_LOOKUP_PATCH2GUEST);

        *(uint32_t *)&pPB[pAsmRecord->offJump] = displ;
        patmPatchAddReloc32(pVM, pPatch, &pPB[pAsmRecord->offJump], FIXUP_REL_JMPTOGUEST,
                            PATCHCODE_PTR_GC(pPatch) + pPatch->uCurPatchOffset + pAsmRecord->offJump - 1 + SIZEOF_NEARJUMP32,
                            pReturnAddrGC);
    }

    // Calculate the right size of this patch block
    if ((fGenJump && pAsmRecord->offJump) || (!fGenJump && !pAsmRecord->offJump))
        return pAsmRecord->cbFunction;
    // if a jump instruction is present and we don't want one, then subtract SIZEOF_NEARJUMP32
    return pAsmRecord->cbFunction - SIZEOF_NEARJUMP32;
}

/* Read bytes and check for overwritten instructions. */
static int patmPatchReadBytes(PVM pVM, uint8_t *pDest, RTRCPTR pSrc, uint32_t cb)
{
    int rc = PGMPhysSimpleReadGCPtr(&pVM->aCpus[0], pDest, pSrc, cb);
    AssertRCReturn(rc, rc);
    /*
     * Could be patched already; make sure this is checked!
     */
    for (uint32_t i=0;i<cb;i++)
    {
        uint8_t temp;

        int rc2 = PATMR3QueryOpcode(pVM, pSrc+i, &temp);
        if (RT_SUCCESS(rc2))
        {
            pDest[i] = temp;
        }
        else
            break;  /* no more */
    }
    return VINF_SUCCESS;
}

int patmPatchGenDuplicate(PVM pVM, PPATCHINFO pPatch, DISCPUSTATE *pCpu, RCPTRTYPE(uint8_t *) pCurInstrGC)
{
    uint32_t const cbInstrShutUpGcc = pCpu->cbInstr;
    PATCHGEN_PROLOG(pVM, pPatch, cbInstrShutUpGcc);

    int rc = patmPatchReadBytes(pVM, pPB, pCurInstrGC, cbInstrShutUpGcc);
    AssertRC(rc);
    PATCHGEN_EPILOG(pPatch, cbInstrShutUpGcc);
    return rc;
}

int patmPatchGenIret(PVM pVM, PPATCHINFO pPatch, RTRCPTR pCurInstrGC, bool fSizeOverride)
{
    uint32_t size;
    PATMCALLINFO callInfo;
    PCPATCHASMRECORD pPatchAsmRec = EMIsRawRing1Enabled(pVM) ? &g_patmIretRing1Record : &g_patmIretRecord;

    PATCHGEN_PROLOG(pVM, pPatch, pPatchAsmRec->cbFunction);

    AssertMsg(fSizeOverride == false, ("operand size override!!\n")); RT_NOREF_PV(fSizeOverride);
    callInfo.pCurInstrGC = pCurInstrGC;

    size = patmPatchGenCode(pVM, pPatch, pPB, pPatchAsmRec, 0, false, &callInfo);

    PATCHGEN_EPILOG(pPatch, size);
    return VINF_SUCCESS;
}

int patmPatchGenCli(PVM pVM, PPATCHINFO pPatch)
{
    uint32_t size;
    PATCHGEN_PROLOG(pVM, pPatch, g_patmCliRecord.cbFunction);

    size = patmPatchGenCode(pVM, pPatch, pPB, &g_patmCliRecord, 0, false);

    PATCHGEN_EPILOG(pPatch, size);
    return VINF_SUCCESS;
}

/*
 * Generate an STI patch
 */
int patmPatchGenSti(PVM pVM, PPATCHINFO pPatch, RTRCPTR pCurInstrGC, RTRCPTR pNextInstrGC)
{
    PATMCALLINFO callInfo;
    uint32_t     size;

    Log(("patmPatchGenSti at %RRv; next %RRv\n", pCurInstrGC, pNextInstrGC)); RT_NOREF_PV(pCurInstrGC);
    PATCHGEN_PROLOG(pVM, pPatch, g_patmStiRecord.cbFunction);
    callInfo.pNextInstrGC = pNextInstrGC;
    size = patmPatchGenCode(pVM, pPatch, pPB, &g_patmStiRecord, 0, false, &callInfo);
    PATCHGEN_EPILOG(pPatch, size);

    return VINF_SUCCESS;
}


int patmPatchGenPopf(PVM pVM, PPATCHINFO pPatch, RCPTRTYPE(uint8_t *) pReturnAddrGC, bool fSizeOverride, bool fGenJumpBack)
{
    uint32_t        size;
    PATMCALLINFO    callInfo;
    PCPATCHASMRECORD pPatchAsmRec;
    if (fSizeOverride == true)
        pPatchAsmRec = fGenJumpBack ? &g_patmPopf16Record : &g_patmPopf16Record_NoExit;
    else
        pPatchAsmRec = fGenJumpBack ? &g_patmPopf32Record : &g_patmPopf32Record_NoExit;

    PATCHGEN_PROLOG(pVM, pPatch, pPatchAsmRec->cbFunction);

    callInfo.pNextInstrGC = pReturnAddrGC;

    Log(("patmPatchGenPopf at %RRv\n", pReturnAddrGC));

    /* Note: keep IOPL in mind when changing any of this!! (see comments in PATMA.asm, PATMPopf32Replacement) */
    if (fSizeOverride == true)
        Log(("operand size override!!\n"));
    size = patmPatchGenCode(pVM, pPatch, pPB, pPatchAsmRec, pReturnAddrGC, fGenJumpBack, &callInfo);

    PATCHGEN_EPILOG(pPatch, size);
    STAM_COUNTER_INC(&pVM->patm.s.StatGenPopf);
    return VINF_SUCCESS;
}

int patmPatchGenPushf(PVM pVM, PPATCHINFO pPatch, bool fSizeOverride)
{
    uint32_t size;
    PCPATCHASMRECORD pPatchAsmRec = fSizeOverride == true ?  &g_patmPushf16Record : &g_patmPushf32Record;
    PATCHGEN_PROLOG(pVM, pPatch, pPatchAsmRec->cbFunction);

    size = patmPatchGenCode(pVM, pPatch, pPB, pPatchAsmRec, 0, false);

    PATCHGEN_EPILOG(pPatch, size);
    return VINF_SUCCESS;
}

int patmPatchGenPushCS(PVM pVM, PPATCHINFO pPatch)
{
    uint32_t size;
    PATCHGEN_PROLOG(pVM, pPatch, g_patmPushCSRecord.cbFunction);
    size = patmPatchGenCode(pVM, pPatch, pPB, &g_patmPushCSRecord, 0, false);
    PATCHGEN_EPILOG(pPatch, size);
    return VINF_SUCCESS;
}

int patmPatchGenLoop(PVM pVM, PPATCHINFO pPatch, RCPTRTYPE(uint8_t *) pTargetGC, uint32_t opcode, bool fSizeOverride)
{
    uint32_t size = 0;
    PCPATCHASMRECORD pPatchAsmRec;
    switch (opcode)
    {
    case OP_LOOP:
        pPatchAsmRec = &g_patmLoopRecord;
        break;
    case OP_LOOPNE:
        pPatchAsmRec = &g_patmLoopNZRecord;
        break;
    case OP_LOOPE:
        pPatchAsmRec = &g_patmLoopZRecord;
        break;
    case OP_JECXZ:
        pPatchAsmRec = &g_patmJEcxRecord;
        break;
    default:
        AssertMsgFailed(("PatchGenLoop: invalid opcode %d\n", opcode));
        return VERR_INVALID_PARAMETER;
    }
    Assert(pPatchAsmRec->offSizeOverride && pPatchAsmRec->offRelJump);

    PATCHGEN_PROLOG(pVM, pPatch, pPatchAsmRec->cbFunction);
    Log(("PatchGenLoop %d jump %d to %08x offrel=%d\n", opcode, pPatch->nrJumpRecs, pTargetGC, pPatchAsmRec->offRelJump));

    // Generate the patch code
    size = patmPatchGenCode(pVM, pPatch, pPB, pPatchAsmRec, 0, false);

    if (fSizeOverride)
    {
        pPB[pPatchAsmRec->offSizeOverride] = 0x66;  // ecx -> cx or vice versa
    }

    *(RTRCPTR *)&pPB[pPatchAsmRec->offRelJump] = 0xDEADBEEF;

    patmPatchAddJump(pVM, pPatch, &pPB[pPatchAsmRec->offRelJump - 1], 1, pTargetGC, opcode);

    PATCHGEN_EPILOG(pPatch, size);
    return VINF_SUCCESS;
}

int patmPatchGenRelJump(PVM pVM, PPATCHINFO pPatch, RCPTRTYPE(uint8_t *) pTargetGC, uint32_t opcode, bool fSizeOverride)
{
    uint32_t offset = 0;
    PATCHGEN_PROLOG(pVM, pPatch, PATCHGEN_DEF_SIZE);

    // internal relative jumps from patch code to patch code; no relocation record required

    Assert(PATMIsPatchGCAddr(pVM, pTargetGC) == false);

    switch (opcode)
    {
    case OP_JO:
        pPB[1] = 0x80;
        break;
    case OP_JNO:
        pPB[1] = 0x81;
        break;
    case OP_JC:
        pPB[1] = 0x82;
        break;
    case OP_JNC:
        pPB[1] = 0x83;
        break;
    case OP_JE:
        pPB[1] = 0x84;
        break;
    case OP_JNE:
        pPB[1] = 0x85;
        break;
    case OP_JBE:
        pPB[1] = 0x86;
        break;
    case OP_JNBE:
        pPB[1] = 0x87;
        break;
    case OP_JS:
        pPB[1] = 0x88;
        break;
    case OP_JNS:
        pPB[1] = 0x89;
        break;
    case OP_JP:
        pPB[1] = 0x8A;
        break;
    case OP_JNP:
        pPB[1] = 0x8B;
        break;
    case OP_JL:
        pPB[1] = 0x8C;
        break;
    case OP_JNL:
        pPB[1] = 0x8D;
        break;
    case OP_JLE:
        pPB[1] = 0x8E;
        break;
    case OP_JNLE:
        pPB[1] = 0x8F;
        break;

    case OP_JMP:
        /* If interrupted here, then jump to the target instruction. Used by PATM.cpp for jumping to known instructions. */
        /* Add lookup record for patch to guest address translation */
        patmR3AddP2GLookupRecord(pVM, pPatch, pPB, pTargetGC, PATM_LOOKUP_PATCH2GUEST);

        pPB[0] = 0xE9;
        break;

    case OP_JECXZ:
    case OP_LOOP:
    case OP_LOOPNE:
    case OP_LOOPE:
        return patmPatchGenLoop(pVM, pPatch, pTargetGC, opcode, fSizeOverride);

    default:
        AssertMsg(0, ("Invalid jump opcode %d\n", opcode));
        return VERR_PATCHING_REFUSED;
    }
    if (opcode != OP_JMP)
    {
         pPB[0] = 0xF;
         offset += 2;
    }
    else offset++;

    *(RTRCPTR *)&pPB[offset] = 0xDEADBEEF;

    patmPatchAddJump(pVM, pPatch, pPB, offset, pTargetGC, opcode);

    offset += sizeof(RTRCPTR);

    PATCHGEN_EPILOG(pPatch, offset);
    return VINF_SUCCESS;
}

/*
 * Rewrite call to dynamic or currently unknown function (on-demand patching of function)
 */
int patmPatchGenCall(PVM pVM, PPATCHINFO pPatch, DISCPUSTATE *pCpu, RTRCPTR pCurInstrGC, RTRCPTR pTargetGC, bool fIndirect)
{
    PATMCALLINFO        callInfo;
    uint32_t            offset;
    uint32_t            i, size;
    int                 rc;

    /** @note Don't check for IF=1 here. The ret instruction will do this.  */
    /** @note It's dangerous to do this for 'normal' patches. the jump target might be inside the generated patch jump. (seen this!) */

    /* 1: Clear PATM interrupt flag on entry. */
    rc = patmPatchGenClearPIF(pVM, pPatch, pCurInstrGC);
    if (rc == VERR_NO_MEMORY)
        return rc;
    AssertRCReturn(rc, rc);

    PATCHGEN_PROLOG(pVM, pPatch, PATCHGEN_DEF_SIZE);
    /* 2: We must push the target address onto the stack before appending the indirect call code. */

    if (fIndirect)
    {
        Log(("patmPatchGenIndirectCall\n"));
        Assert(pCpu->Param1.cb == 4);
        Assert(OP_PARM_VTYPE(pCpu->pCurInstr->fParam1) != OP_PARM_J);

        /* We push it onto the stack here, so the guest's context isn't ruined when this happens to cause
         * a page fault. The assembly code restores the stack afterwards.
         */
        offset = 0;
        /* include prefix byte to make sure we don't use the incorrect selector register. */
        if (pCpu->fPrefix & DISPREFIX_SEG)
            pPB[offset++] = DISQuerySegPrefixByte(pCpu);
        pPB[offset++] = 0xFF;              // push r/m32
        pPB[offset++] = MAKE_MODRM(pCpu->ModRM.Bits.Mod, 6 /* group 5 */, pCpu->ModRM.Bits.Rm);
        i = 2;  /* standard offset of modrm bytes */
        if (pCpu->fPrefix & DISPREFIX_OPSIZE)
            i++;    //skip operand prefix
        if (pCpu->fPrefix & DISPREFIX_SEG)
            i++;    //skip segment prefix

        rc = patmPatchReadBytes(pVM, &pPB[offset], (RTRCPTR)((RTGCUINTPTR32)pCurInstrGC + i), pCpu->cbInstr - i);
        AssertRCReturn(rc, rc);
        offset += (pCpu->cbInstr - i);
    }
    else
    {
        AssertMsg(PATMIsPatchGCAddr(pVM, pTargetGC) == false, ("Target is already a patch address (%RRv)?!?\n", pTargetGC));
        Assert(pTargetGC);
        Assert(OP_PARM_VTYPE(pCpu->pCurInstr->fParam1) == OP_PARM_J);

        /** @todo wasting memory as the complex search is overkill and we need only one lookup slot... */

        /* Relative call to patch code (patch to patch -> no fixup). */
        Log(("PatchGenCall from %RRv (next=%RRv) to %RRv\n", pCurInstrGC, pCurInstrGC + pCpu->cbInstr, pTargetGC));

        /* We push it onto the stack here, so the guest's context isn't ruined when this happens to cause
         * a page fault. The assembly code restores the stack afterwards.
         */
        offset = 0;
        pPB[offset++] = 0x68;              // push %Iv
        *(RTRCPTR *)&pPB[offset] = pTargetGC;
        offset += sizeof(RTRCPTR);
    }

    /* align this block properly to make sure the jump table will not be misaligned. */
    size = (RTHCUINTPTR)&pPB[offset] & 3;
    if (size)
        size = 4 - size;

    for (i=0;i<size;i++)
    {
        pPB[offset++] = 0x90;   /* nop */
    }
    PATCHGEN_EPILOG(pPatch, offset);

    /* 3: Generate code to lookup address in our local cache; call hypervisor PATM code if it can't be located. */
    PCPATCHASMRECORD pPatchAsmRec = fIndirect ? &g_patmCallIndirectRecord : &g_patmCallRecord;
    PATCHGEN_PROLOG_NODEF(pVM, pPatch, pPatchAsmRec->cbFunction);
    callInfo.pReturnGC      = pCurInstrGC + pCpu->cbInstr;
    callInfo.pTargetGC      = (fIndirect) ? 0xDEADBEEF : pTargetGC;
    size = patmPatchGenCode(pVM, pPatch, pPB, pPatchAsmRec, 0, false, &callInfo);
    PATCHGEN_EPILOG(pPatch, size);

    /* Need to set PATM_ASMFIX_INTERRUPTFLAG after the patched ret returns here. */
    rc = patmPatchGenSetPIF(pVM, pPatch, pCurInstrGC);
    if (rc == VERR_NO_MEMORY)
        return rc;
    AssertRCReturn(rc, rc);

    STAM_COUNTER_INC(&pVM->patm.s.StatGenCall);
    return VINF_SUCCESS;
}

/**
 * Generate indirect jump to unknown destination
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch record
 * @param   pCpu        Disassembly state
 * @param   pCurInstrGC Current instruction address
 */
int patmPatchGenJump(PVM pVM, PPATCHINFO pPatch, DISCPUSTATE *pCpu, RTRCPTR pCurInstrGC)
{
    PATMCALLINFO        callInfo;
    uint32_t            offset;
    uint32_t            i, size;
    int                 rc;

    /* 1: Clear PATM interrupt flag on entry. */
    rc = patmPatchGenClearPIF(pVM, pPatch, pCurInstrGC);
    if (rc == VERR_NO_MEMORY)
        return rc;
    AssertRCReturn(rc, rc);

    PATCHGEN_PROLOG(pVM, pPatch, PATCHGEN_DEF_SIZE);
    /* 2: We must push the target address onto the stack before appending the indirect call code. */

    Log(("patmPatchGenIndirectJump\n"));
    Assert(pCpu->Param1.cb == 4);
    Assert(OP_PARM_VTYPE(pCpu->pCurInstr->fParam1) != OP_PARM_J);

    /* We push it onto the stack here, so the guest's context isn't ruined when this happens to cause
     * a page fault. The assembly code restores the stack afterwards.
     */
    offset = 0;
    /* include prefix byte to make sure we don't use the incorrect selector register. */
    if (pCpu->fPrefix & DISPREFIX_SEG)
        pPB[offset++] = DISQuerySegPrefixByte(pCpu);

    pPB[offset++] = 0xFF;              // push r/m32
    pPB[offset++] = MAKE_MODRM(pCpu->ModRM.Bits.Mod, 6 /* group 5 */, pCpu->ModRM.Bits.Rm);
    i = 2;  /* standard offset of modrm bytes */
    if (pCpu->fPrefix & DISPREFIX_OPSIZE)
        i++;    //skip operand prefix
    if (pCpu->fPrefix & DISPREFIX_SEG)
        i++;    //skip segment prefix

    rc = patmPatchReadBytes(pVM, &pPB[offset], (RTRCPTR)((RTGCUINTPTR32)pCurInstrGC + i), pCpu->cbInstr - i);
    AssertRCReturn(rc, rc);
    offset += (pCpu->cbInstr - i);

    /* align this block properly to make sure the jump table will not be misaligned. */
    size = (RTHCUINTPTR)&pPB[offset] & 3;
    if (size)
        size = 4 - size;

    for (i=0;i<size;i++)
    {
        pPB[offset++] = 0x90;   /* nop */
    }
    PATCHGEN_EPILOG(pPatch, offset);

    /* 3: Generate code to lookup address in our local cache; call hypervisor PATM code if it can't be located. */
    PATCHGEN_PROLOG_NODEF(pVM, pPatch, g_patmJumpIndirectRecord.cbFunction);
    callInfo.pReturnGC      = pCurInstrGC + pCpu->cbInstr;
    callInfo.pTargetGC      = 0xDEADBEEF;
    size = patmPatchGenCode(pVM, pPatch, pPB, &g_patmJumpIndirectRecord, 0, false, &callInfo);
    PATCHGEN_EPILOG(pPatch, size);

    STAM_COUNTER_INC(&pVM->patm.s.StatGenJump);
    return VINF_SUCCESS;
}

/**
 * Generate return instruction
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch structure
 * @param   pCpu        Disassembly struct
 * @param   pCurInstrGC Current instruction pointer
 *
 */
int patmPatchGenRet(PVM pVM, PPATCHINFO pPatch, DISCPUSTATE *pCpu, RCPTRTYPE(uint8_t *) pCurInstrGC)
{
    RTRCPTR pPatchRetInstrGC;

    /* Remember start of this patch for below. */
    pPatchRetInstrGC = PATCHCODE_PTR_GC(pPatch) + pPatch->uCurPatchOffset;

    Log(("patmPatchGenRet %RRv\n", pCurInstrGC));

    /** @note optimization: multiple identical ret instruction in a single patch can share a single patched ret. */
    if (    pPatch->pTempInfo->pPatchRetInstrGC
        &&  pPatch->pTempInfo->uPatchRetParam1 == (uint32_t)pCpu->Param1.uValue) /* nr of bytes popped off the stack should be identical of course! */
    {
        Assert(pCpu->pCurInstr->uOpcode == OP_RETN);
        STAM_COUNTER_INC(&pVM->patm.s.StatGenRetReused);

        return patmPatchGenPatchJump(pVM, pPatch, pCurInstrGC, pPatch->pTempInfo->pPatchRetInstrGC);
    }

    /* Jump back to the original instruction if IF is set again. */
    Assert(!patmFindActivePatchByEntrypoint(pVM, pCurInstrGC));
    int rc = patmPatchGenCheckIF(pVM, pPatch, pCurInstrGC);
    AssertRCReturn(rc, rc);

    /* align this block properly to make sure the jump table will not be misaligned. */
    PATCHGEN_PROLOG(pVM, pPatch, 4);
    uint32_t size = (RTHCUINTPTR)pPB & 3;
    if (size)
        size = 4 - size;

    for (uint32_t i = 0; i < size; i++)
        pPB[i] = 0x90;   /* nop */
    PATCHGEN_EPILOG(pPatch, size);

    PATCHGEN_PROLOG_NODEF(pVM, pPatch, g_patmRetRecord.cbFunction);
    size = patmPatchGenCode(pVM, pPatch, pPB, &g_patmRetRecord, 0, false);
    PATCHGEN_EPILOG(pPatch, size);

    STAM_COUNTER_INC(&pVM->patm.s.StatGenRet);
    /* Duplicate the ret or ret n instruction; it will use the PATM return address */
    rc = patmPatchGenDuplicate(pVM, pPatch, pCpu, pCurInstrGC);

    if (rc == VINF_SUCCESS)
    {
        pPatch->pTempInfo->pPatchRetInstrGC = pPatchRetInstrGC;
        pPatch->pTempInfo->uPatchRetParam1  = pCpu->Param1.uValue;
    }
    return rc;
}

/**
 * Generate all global patm functions
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch structure
 *
 */
int patmPatchGenGlobalFunctions(PVM pVM, PPATCHINFO pPatch)
{
    pVM->patm.s.pfnHelperCallGC = PATCHCODE_PTR_GC(pPatch) + pPatch->uCurPatchOffset;
    PATCHGEN_PROLOG(pVM, pPatch, g_patmLookupAndCallRecord.cbFunction);
    uint32_t size = patmPatchGenCode(pVM, pPatch, pPB, &g_patmLookupAndCallRecord, 0, false);
    PATCHGEN_EPILOG(pPatch, size);

    /* Round to next 8 byte boundary. */
    pPatch->uCurPatchOffset = RT_ALIGN_32(pPatch->uCurPatchOffset, 8);

    pVM->patm.s.pfnHelperRetGC = PATCHCODE_PTR_GC(pPatch) + pPatch->uCurPatchOffset;
    PATCHGEN_PROLOG_NODEF(pVM, pPatch, g_patmRetFunctionRecord.cbFunction);
    size = patmPatchGenCode(pVM, pPatch, pPB, &g_patmRetFunctionRecord, 0, false);
    PATCHGEN_EPILOG(pPatch, size);

    /* Round to next 8 byte boundary. */
    pPatch->uCurPatchOffset = RT_ALIGN_32(pPatch->uCurPatchOffset, 8);

    pVM->patm.s.pfnHelperJumpGC = PATCHCODE_PTR_GC(pPatch) + pPatch->uCurPatchOffset;
    PATCHGEN_PROLOG_NODEF(pVM, pPatch, g_patmLookupAndJumpRecord.cbFunction);
    size = patmPatchGenCode(pVM, pPatch, pPB, &g_patmLookupAndJumpRecord, 0, false);
    PATCHGEN_EPILOG(pPatch, size);

    /* Round to next 8 byte boundary. */
    pPatch->uCurPatchOffset = RT_ALIGN_32(pPatch->uCurPatchOffset, 8);

    pVM->patm.s.pfnHelperIretGC = PATCHCODE_PTR_GC(pPatch) + pPatch->uCurPatchOffset;
    PATCHGEN_PROLOG_NODEF(pVM, pPatch, g_patmIretFunctionRecord.cbFunction);
    size = patmPatchGenCode(pVM, pPatch, pPB, &g_patmIretFunctionRecord, 0, false);
    PATCHGEN_EPILOG(pPatch, size);

    Log(("pfnHelperCallGC %RRv\n", pVM->patm.s.pfnHelperCallGC));
    Log(("pfnHelperRetGC  %RRv\n", pVM->patm.s.pfnHelperRetGC));
    Log(("pfnHelperJumpGC %RRv\n", pVM->patm.s.pfnHelperJumpGC));
    Log(("pfnHelperIretGC %RRv\n", pVM->patm.s.pfnHelperIretGC));

    return VINF_SUCCESS;
}

/**
 * Generate illegal instruction (int 3)
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch structure
 *
 */
int patmPatchGenIllegalInstr(PVM pVM, PPATCHINFO pPatch)
{
    PATCHGEN_PROLOG(pVM, pPatch, 1);

    pPB[0] = 0xCC;

    PATCHGEN_EPILOG(pPatch, 1);
    return VINF_SUCCESS;
}

/**
 * Check virtual IF flag and jump back to original guest code if set
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch structure
 * @param   pCurInstrGC Guest context pointer to the current instruction
 *
 */
int patmPatchGenCheckIF(PVM pVM, PPATCHINFO pPatch, RTRCPTR pCurInstrGC)
{
    uint32_t size;

    PATCHGEN_PROLOG(pVM, pPatch, g_patmCheckIFRecord.cbFunction);

    /* Add lookup record for patch to guest address translation */
    patmR3AddP2GLookupRecord(pVM, pPatch, pPB, pCurInstrGC, PATM_LOOKUP_PATCH2GUEST);

    /* Generate code to check for IF=1 before executing the call to the duplicated function. */
    size = patmPatchGenCode(pVM, pPatch, pPB, &g_patmCheckIFRecord, pCurInstrGC, true);

    PATCHGEN_EPILOG(pPatch, size);
    return VINF_SUCCESS;
}

/**
 * Set PATM interrupt flag
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch structure
 * @param   pInstrGC    Corresponding guest instruction
 *
 */
int patmPatchGenSetPIF(PVM pVM, PPATCHINFO pPatch, RTRCPTR pInstrGC)
{
    PATCHGEN_PROLOG(pVM, pPatch, g_patmSetPIFRecord.cbFunction);

    /* Add lookup record for patch to guest address translation */
    patmR3AddP2GLookupRecord(pVM, pPatch, pPB, pInstrGC, PATM_LOOKUP_PATCH2GUEST);

    uint32_t size = patmPatchGenCode(pVM, pPatch, pPB, &g_patmSetPIFRecord, 0, false);
    PATCHGEN_EPILOG(pPatch, size);
    return VINF_SUCCESS;
}

/**
 * Clear PATM interrupt flag
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch structure
 * @param   pInstrGC    Corresponding guest instruction
 *
 */
int patmPatchGenClearPIF(PVM pVM, PPATCHINFO pPatch, RTRCPTR pInstrGC)
{
    PATCHGEN_PROLOG(pVM, pPatch, g_patmSetPIFRecord.cbFunction);

    /* Add lookup record for patch to guest address translation */
    patmR3AddP2GLookupRecord(pVM, pPatch, pPB, pInstrGC, PATM_LOOKUP_PATCH2GUEST);

    uint32_t size = patmPatchGenCode(pVM, pPatch, pPB, &g_patmClearPIFRecord, 0, false);
    PATCHGEN_EPILOG(pPatch, size);
    return VINF_SUCCESS;
}


/**
 * Clear PATM inhibit irq flag
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pPatch          Patch structure
 * @param   pNextInstrGC    Next guest instruction
 */
int patmPatchGenClearInhibitIRQ(PVM pVM, PPATCHINFO pPatch, RTRCPTR pNextInstrGC)
{
    PATMCALLINFO callInfo;
    PCPATCHASMRECORD pPatchAsmRec = pPatch->flags & PATMFL_DUPLICATE_FUNCTION
                                  ? &g_patmClearInhibitIRQContIF0Record : &g_patmClearInhibitIRQFaultIF0Record;
    PATCHGEN_PROLOG(pVM, pPatch, pPatchAsmRec->cbFunction);

    Assert((pPatch->flags & (PATMFL_GENERATE_JUMPTOGUEST|PATMFL_DUPLICATE_FUNCTION)) != (PATMFL_GENERATE_JUMPTOGUEST|PATMFL_DUPLICATE_FUNCTION));

    /* Add lookup record for patch to guest address translation */
    patmR3AddP2GLookupRecord(pVM, pPatch, pPB, pNextInstrGC, PATM_LOOKUP_PATCH2GUEST);

    callInfo.pNextInstrGC = pNextInstrGC;

    uint32_t size = patmPatchGenCode(pVM, pPatch, pPB, pPatchAsmRec, 0, false, &callInfo);

    PATCHGEN_EPILOG(pPatch, size);
    return VINF_SUCCESS;
}

/**
 * Generate an interrupt handler entrypoint
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch record
 * @param   pIntHandlerGC IDT handler address
 *
 ** @todo must check if virtual IF is already cleared on entry!!!!!!!!!!!!!!!!!!!!!!!
 */
int patmPatchGenIntEntry(PVM pVM, PPATCHINFO pPatch, RTRCPTR pIntHandlerGC)
{
    int rc = VINF_SUCCESS;

    if (!EMIsRawRing1Enabled(pVM))    /* direct passthru of interrupts is not allowed in the ring-1 support case as we can't
                                         deal with the ring-1/2 ambiguity in the patm asm code and we don't need it either as
                                         TRPMForwardTrap takes care of the details. */
    {
        uint32_t size;
        PCPATCHASMRECORD pPatchAsmRec = pPatch->flags & PATMFL_INTHANDLER_WITH_ERRORCODE
                                      ? &g_patmIntEntryRecordErrorCode : &g_patmIntEntryRecord;
        PATCHGEN_PROLOG(pVM, pPatch, pPatchAsmRec->cbFunction);

        /* Add lookup record for patch to guest address translation */
        patmR3AddP2GLookupRecord(pVM, pPatch, pPB, pIntHandlerGC, PATM_LOOKUP_PATCH2GUEST);

        /* Generate entrypoint for the interrupt handler (correcting CS in the interrupt stack frame) */
        size = patmPatchGenCode(pVM, pPatch, pPB, pPatchAsmRec, 0, false);

        PATCHGEN_EPILOG(pPatch, size);
    }

    // Interrupt gates set IF to 0
    rc = patmPatchGenCli(pVM, pPatch);
    AssertRCReturn(rc, rc);

    return rc;
}

/**
 * Generate a trap handler entrypoint
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch record
 * @param   pTrapHandlerGC  IDT handler address
 */
int patmPatchGenTrapEntry(PVM pVM, PPATCHINFO pPatch, RTRCPTR pTrapHandlerGC)
{
    uint32_t size;
    PCPATCHASMRECORD pPatchAsmRec = (pPatch->flags & PATMFL_TRAPHANDLER_WITH_ERRORCODE)
                                  ? &g_patmTrapEntryRecordErrorCode : &g_patmTrapEntryRecord;

    Assert(!EMIsRawRing1Enabled(pVM));

    PATCHGEN_PROLOG(pVM, pPatch, pPatchAsmRec->cbFunction);

    /* Add lookup record for patch to guest address translation */
    patmR3AddP2GLookupRecord(pVM, pPatch, pPB, pTrapHandlerGC, PATM_LOOKUP_PATCH2GUEST);

    /* Generate entrypoint for the trap handler (correcting CS in the interrupt stack frame) */
    size = patmPatchGenCode(pVM, pPatch, pPB, pPatchAsmRec, pTrapHandlerGC, true);
    PATCHGEN_EPILOG(pPatch, size);

    return VINF_SUCCESS;
}

#ifdef VBOX_WITH_STATISTICS
int patmPatchGenStats(PVM pVM, PPATCHINFO pPatch, RTRCPTR pInstrGC)
{
    uint32_t size;

    PATCHGEN_PROLOG(pVM, pPatch, g_patmStatsRecord.cbFunction);

    /* Add lookup record for stats code -> guest handler. */
    patmR3AddP2GLookupRecord(pVM, pPatch, pPB, pInstrGC, PATM_LOOKUP_PATCH2GUEST);

    /* Generate code to keep calling statistics for this patch */
    size = patmPatchGenCode(pVM, pPatch, pPB, &g_patmStatsRecord, pInstrGC, false);
    PATCHGEN_EPILOG(pPatch, size);

    return VINF_SUCCESS;
}
#endif

/**
 * Debug register moves to or from general purpose registers
 * mov GPR, DRx
 * mov DRx, GPR
 *
 * @todo: if we ever want to support hardware debug registers natively, then
 *        this will need to be changed!
 */
int patmPatchGenMovDebug(PVM pVM, PPATCHINFO pPatch, DISCPUSTATE *pCpu)
{
    int rc = VINF_SUCCESS;
    unsigned reg, mod, rm, dbgreg;
    uint32_t offset;

    PATCHGEN_PROLOG(pVM, pPatch, PATCHGEN_DEF_SIZE);

    mod = 0;            //effective address (only)
    rm  = 5;            //disp32
    if (pCpu->pCurInstr->fParam1 == OP_PARM_Dd)
    {
        Assert(0);  // You not come here. Illegal!

        // mov DRx, GPR
        pPB[0] = 0x89;      //mov disp32, GPR
        Assert(pCpu->Param1.fUse & DISUSE_REG_DBG);
        Assert(pCpu->Param2.fUse & DISUSE_REG_GEN32);

        dbgreg = pCpu->Param1.Base.idxDbgReg;
        reg    = pCpu->Param2.Base.idxGenReg;
    }
    else
    {
        // mov GPR, DRx
        Assert(pCpu->Param1.fUse & DISUSE_REG_GEN32);
        Assert(pCpu->Param2.fUse & DISUSE_REG_DBG);

        pPB[0] = 0x8B;      // mov GPR, disp32
        reg    = pCpu->Param1.Base.idxGenReg;
        dbgreg = pCpu->Param2.Base.idxDbgReg;
    }

    pPB[1] = MAKE_MODRM(mod, reg, rm);

    AssertReturn(dbgreg <= DISDREG_DR7, VERR_INVALID_PARAMETER);
    offset = RT_OFFSETOF(CPUMCTX, dr[dbgreg]);

    *(RTRCPTR *)&pPB[2] = pVM->patm.s.pCPUMCtxGC + offset;
    patmPatchAddReloc32(pVM, pPatch, &pPB[2], FIXUP_ABSOLUTE);

    PATCHGEN_EPILOG(pPatch, 2 + sizeof(RTRCPTR));
    return rc;
}

/*
 * Control register moves to or from general purpose registers
 * mov GPR, CRx
 * mov CRx, GPR
 */
int patmPatchGenMovControl(PVM pVM, PPATCHINFO pPatch, DISCPUSTATE *pCpu)
{
    int rc = VINF_SUCCESS;
    int reg, mod, rm, ctrlreg;
    uint32_t offset;

    PATCHGEN_PROLOG(pVM, pPatch, PATCHGEN_DEF_SIZE);

    mod = 0;            //effective address (only)
    rm  = 5;            //disp32
    if (pCpu->pCurInstr->fParam1 == OP_PARM_Cd)
    {
        Assert(0);  // You not come here. Illegal!

        // mov CRx, GPR
        pPB[0] = 0x89;      //mov disp32, GPR
        ctrlreg = pCpu->Param1.Base.idxCtrlReg;
        reg     = pCpu->Param2.Base.idxGenReg;
        Assert(pCpu->Param1.fUse & DISUSE_REG_CR);
        Assert(pCpu->Param2.fUse & DISUSE_REG_GEN32);
    }
    else
    {
        // mov GPR, CRx
        Assert(pCpu->Param1.fUse & DISUSE_REG_GEN32);
        Assert(pCpu->Param2.fUse & DISUSE_REG_CR);

        pPB[0]  = 0x8B;      // mov GPR, disp32
        reg     = pCpu->Param1.Base.idxGenReg;
        ctrlreg = pCpu->Param2.Base.idxCtrlReg;
    }

    pPB[1] = MAKE_MODRM(mod, reg, rm);

    /// @todo make this an array in the context structure
    switch (ctrlreg)
    {
    case DISCREG_CR0:
        offset = RT_OFFSETOF(CPUMCTX, cr0);
        break;
    case DISCREG_CR2:
        offset = RT_OFFSETOF(CPUMCTX, cr2);
        break;
    case DISCREG_CR3:
        offset = RT_OFFSETOF(CPUMCTX, cr3);
        break;
    case DISCREG_CR4:
        offset = RT_OFFSETOF(CPUMCTX, cr4);
        break;
    default: /* Shut up compiler warning. */
        AssertFailed();
        offset = 0;
        break;
    }
    *(RTRCPTR *)&pPB[2] = pVM->patm.s.pCPUMCtxGC + offset;
    patmPatchAddReloc32(pVM, pPatch, &pPB[2], FIXUP_ABSOLUTE);

    PATCHGEN_EPILOG(pPatch, 2 + sizeof(RTRCPTR));
    return rc;
}

/*
 * mov GPR, SS
 */
int patmPatchGenMovFromSS(PVM pVM, PPATCHINFO pPatch, DISCPUSTATE *pCpu, RTRCPTR pCurInstrGC)
{
    uint32_t size, offset;

    Log(("patmPatchGenMovFromSS %RRv\n", pCurInstrGC)); RT_NOREF_PV(pCurInstrGC);

    Assert(pPatch->flags & PATMFL_CODE32);

    PATCHGEN_PROLOG(pVM, pPatch, g_patmClearPIFRecord.cbFunction + 2 + g_patmMovFromSSRecord.cbFunction + 2 + g_patmSetPIFRecord.cbFunction);
    size = patmPatchGenCode(pVM, pPatch, pPB, &g_patmClearPIFRecord, 0, false);
    PATCHGEN_EPILOG(pPatch, size);

    /* push ss */
    PATCHGEN_PROLOG_NODEF(pVM, pPatch, 2);
    offset = 0;
    if (pCpu->fPrefix & DISPREFIX_OPSIZE)
        pPB[offset++] = 0x66;       /* size override -> 16 bits push */
    pPB[offset++] = 0x16;
    PATCHGEN_EPILOG(pPatch, offset);

    /* checks and corrects RPL of pushed ss*/
    PATCHGEN_PROLOG_NODEF(pVM, pPatch, g_patmMovFromSSRecord.cbFunction);
    size = patmPatchGenCode(pVM, pPatch, pPB, &g_patmMovFromSSRecord, 0, false);
    PATCHGEN_EPILOG(pPatch, size);

    /* pop general purpose register */
    PATCHGEN_PROLOG_NODEF(pVM, pPatch, 2);
    offset = 0;
    if (pCpu->fPrefix & DISPREFIX_OPSIZE)
        pPB[offset++] = 0x66; /* size override -> 16 bits pop */
    pPB[offset++] = 0x58 + pCpu->Param1.Base.idxGenReg;
    PATCHGEN_EPILOG(pPatch, offset);


    PATCHGEN_PROLOG_NODEF(pVM, pPatch, g_patmSetPIFRecord.cbFunction);
    size = patmPatchGenCode(pVM, pPatch, pPB, &g_patmSetPIFRecord, 0, false);
    PATCHGEN_EPILOG(pPatch, size);

    return VINF_SUCCESS;
}


/**
 * Generate an sldt or str patch instruction
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch record
 * @param   pCpu        Disassembly state
 * @param   pCurInstrGC Guest instruction address
 */
int patmPatchGenSldtStr(PVM pVM, PPATCHINFO pPatch, DISCPUSTATE *pCpu, RTRCPTR pCurInstrGC)
{
    // sldt %Ew
    int rc = VINF_SUCCESS;
    uint32_t offset = 0;
    uint32_t i;

    /** @todo segment prefix (untested) */
    Assert(pCpu->fPrefix == DISPREFIX_NONE || pCpu->fPrefix == DISPREFIX_OPSIZE);

    PATCHGEN_PROLOG(pVM, pPatch, PATCHGEN_DEF_SIZE);

    if (pCpu->Param1.fUse == DISUSE_REG_GEN32 || pCpu->Param1.fUse == DISUSE_REG_GEN16)
    {
        /* Register operand */
        // 8B 15 [32 bits addr]   mov edx, CPUMCTX.tr/ldtr

        if (pCpu->fPrefix == DISPREFIX_OPSIZE)
            pPB[offset++] = 0x66;

        pPB[offset++] = 0x8B;              // mov       destreg, CPUMCTX.tr/ldtr
        /* Modify REG part according to destination of original instruction */
        pPB[offset++] = MAKE_MODRM(0, pCpu->Param1.Base.idxGenReg, 5);
        if (pCpu->pCurInstr->uOpcode == OP_STR)
        {
            *(RTRCPTR *)&pPB[offset] = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, tr);
        }
        else
        {
            *(RTRCPTR *)&pPB[offset] = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, ldtr);
        }
        patmPatchAddReloc32(pVM, pPatch, &pPB[offset], FIXUP_ABSOLUTE);
        offset += sizeof(RTRCPTR);
    }
    else
    {
        /* Memory operand */
        //50                   push        eax
        //52                   push        edx
        //8D 15 48 7C 42 00    lea         edx, dword ptr [dest]
        //66 A1 48 7C 42 00    mov         ax, CPUMCTX.tr/ldtr
        //66 89 02             mov         word ptr [edx],ax
        //5A                   pop         edx
        //58                   pop         eax

        pPB[offset++] = 0x50;              // push      eax
        pPB[offset++] = 0x52;              // push      edx

        if (pCpu->fPrefix == DISPREFIX_SEG)
        {
            pPB[offset++] = DISQuerySegPrefixByte(pCpu);
        }
        pPB[offset++] = 0x8D;              // lea       edx, dword ptr [dest]
        // duplicate and modify modrm byte and additional bytes if present (e.g. direct address)
        pPB[offset++] = MAKE_MODRM(pCpu->ModRM.Bits.Mod, DISGREG_EDX , pCpu->ModRM.Bits.Rm);

        i = 3;  /* standard offset of modrm bytes */
        if (pCpu->fPrefix == DISPREFIX_OPSIZE)
            i++;    //skip operand prefix
        if (pCpu->fPrefix == DISPREFIX_SEG)
            i++;    //skip segment prefix

        rc = patmPatchReadBytes(pVM, &pPB[offset], (RTRCPTR)((RTGCUINTPTR32)pCurInstrGC + i), pCpu->cbInstr - i);
        AssertRCReturn(rc, rc);
        offset += (pCpu->cbInstr - i);

        pPB[offset++] = 0x66;              // mov       ax, CPUMCTX.tr/ldtr
        pPB[offset++] = 0xA1;
        if (pCpu->pCurInstr->uOpcode == OP_STR)
        {
            *(RTRCPTR *)&pPB[offset] = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, tr);
        }
        else
        {
            *(RTRCPTR *)&pPB[offset] = pVM->patm.s.pCPUMCtxGC + RT_OFFSETOF(CPUMCTX, ldtr);
        }
        patmPatchAddReloc32(pVM, pPatch, &pPB[offset], FIXUP_ABSOLUTE);
        offset += sizeof(RTRCPTR);

        pPB[offset++] = 0x66;              // mov       word ptr [edx],ax
        pPB[offset++] = 0x89;
        pPB[offset++] = 0x02;

        pPB[offset++] = 0x5A;              // pop       edx
        pPB[offset++] = 0x58;              // pop       eax
    }

    PATCHGEN_EPILOG(pPatch, offset);

    return rc;
}

/**
 * Generate an sgdt or sidt patch instruction
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch record
 * @param   pCpu        Disassembly state
 * @param   pCurInstrGC Guest instruction address
 */
int patmPatchGenSxDT(PVM pVM, PPATCHINFO pPatch, DISCPUSTATE *pCpu, RTRCPTR pCurInstrGC)
{
    int rc = VINF_SUCCESS;
    uint32_t offset = 0, offset_base, offset_limit;
    uint32_t i;

    /** @todo segment prefix (untested) */
    Assert(pCpu->fPrefix == DISPREFIX_NONE);

    // sgdt %Ms
    // sidt %Ms

    switch (pCpu->pCurInstr->uOpcode)
    {
    case OP_SGDT:
        offset_base  = RT_OFFSETOF(CPUMCTX, gdtr.pGdt);
        offset_limit = RT_OFFSETOF(CPUMCTX, gdtr.cbGdt);
        break;

    case OP_SIDT:
        offset_base  = RT_OFFSETOF(CPUMCTX, idtr.pIdt);
        offset_limit = RT_OFFSETOF(CPUMCTX, idtr.cbIdt);
        break;

    default:
        return VERR_INVALID_PARAMETER;
    }

//50                   push        eax
//52                   push        edx
//8D 15 48 7C 42 00    lea         edx, dword ptr [dest]
//66 A1 48 7C 42 00    mov         ax, CPUMCTX.gdtr.limit
//66 89 02             mov         word ptr [edx],ax
//A1 48 7C 42 00       mov         eax, CPUMCTX.gdtr.base
//89 42 02             mov         dword ptr [edx+2],eax
//5A                   pop         edx
//58                   pop         eax

    PATCHGEN_PROLOG(pVM, pPatch, PATCHGEN_DEF_SIZE);
    pPB[offset++] = 0x50;              // push      eax
    pPB[offset++] = 0x52;              // push      edx

    if (pCpu->fPrefix == DISPREFIX_SEG)
    {
        pPB[offset++] = DISQuerySegPrefixByte(pCpu);
    }
    pPB[offset++] = 0x8D;              // lea       edx, dword ptr [dest]
    // duplicate and modify modrm byte and additional bytes if present (e.g. direct address)
    pPB[offset++] = MAKE_MODRM(pCpu->ModRM.Bits.Mod, DISGREG_EDX , pCpu->ModRM.Bits.Rm);

    i = 3;  /* standard offset of modrm bytes */
    if (pCpu->fPrefix == DISPREFIX_OPSIZE)
        i++;    //skip operand prefix
    if (pCpu->fPrefix == DISPREFIX_SEG)
        i++;    //skip segment prefix
    rc = patmPatchReadBytes(pVM, &pPB[offset], (RTRCPTR)((RTGCUINTPTR32)pCurInstrGC + i), pCpu->cbInstr - i);
    AssertRCReturn(rc, rc);
    offset += (pCpu->cbInstr - i);

    pPB[offset++] = 0x66;              // mov       ax, CPUMCTX.gdtr.limit
    pPB[offset++] = 0xA1;
    *(RTRCPTR *)&pPB[offset] = pVM->patm.s.pCPUMCtxGC + offset_limit;
    patmPatchAddReloc32(pVM, pPatch, &pPB[offset], FIXUP_ABSOLUTE);
    offset += sizeof(RTRCPTR);

    pPB[offset++] = 0x66;              // mov       word ptr [edx],ax
    pPB[offset++] = 0x89;
    pPB[offset++] = 0x02;

    pPB[offset++] = 0xA1;              // mov       eax, CPUMCTX.gdtr.base
    *(RTRCPTR *)&pPB[offset] = pVM->patm.s.pCPUMCtxGC + offset_base;
    patmPatchAddReloc32(pVM, pPatch, &pPB[offset], FIXUP_ABSOLUTE);
    offset += sizeof(RTRCPTR);

    pPB[offset++] = 0x89;              // mov       dword ptr [edx+2],eax
    pPB[offset++] = 0x42;
    pPB[offset++] = 0x02;

    pPB[offset++] = 0x5A;              // pop       edx
    pPB[offset++] = 0x58;              // pop       eax

    PATCHGEN_EPILOG(pPatch, offset);

    return rc;
}

/**
 * Generate a cpuid patch instruction
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch record
 * @param   pCurInstrGC Guest instruction address
 */
int patmPatchGenCpuid(PVM pVM, PPATCHINFO pPatch, RTRCPTR pCurInstrGC)
{
    uint32_t size;
    PATCHGEN_PROLOG(pVM, pPatch, g_patmCpuidRecord.cbFunction);

    size = patmPatchGenCode(pVM, pPatch, pPB, &g_patmCpuidRecord, 0, false);

    PATCHGEN_EPILOG(pPatch, size);
    NOREF(pCurInstrGC);
    return VINF_SUCCESS;
}

/**
 * Generate the jump from guest to patch code
 *
 * @returns VBox status code.
 * @param   pVM                 The cross context VM structure.
 * @param   pPatch              Patch record
 * @param   pReturnAddrGC       Guest code target of the jump.
 * @param   fClearInhibitIRQs   Clear inhibit irq flag
 */
int patmPatchGenJumpToGuest(PVM pVM, PPATCHINFO pPatch, RCPTRTYPE(uint8_t *) pReturnAddrGC, bool fClearInhibitIRQs)
{
    int rc = VINF_SUCCESS;
    uint32_t size;

    if (fClearInhibitIRQs)
    {
        rc = patmPatchGenClearInhibitIRQ(pVM, pPatch, pReturnAddrGC);
        if (rc == VERR_NO_MEMORY)
            return rc;
        AssertRCReturn(rc, rc);
    }

    PATCHGEN_PROLOG(pVM, pPatch, PATMJumpToGuest_IF1Record.cbFunction);

    /* Add lookup record for patch to guest address translation */
    patmR3AddP2GLookupRecord(pVM, pPatch, pPB, pReturnAddrGC, PATM_LOOKUP_PATCH2GUEST);

    /* Generate code to jump to guest code if IF=1, else fault. */
    size = patmPatchGenCode(pVM, pPatch, pPB, &PATMJumpToGuest_IF1Record, pReturnAddrGC, true);
    PATCHGEN_EPILOG(pPatch, size);

    return rc;
}

/*
 * Relative jump from patch code to patch code (no fixup required)
 */
int patmPatchGenPatchJump(PVM pVM, PPATCHINFO pPatch, RTRCPTR pCurInstrGC, RCPTRTYPE(uint8_t *) pPatchAddrGC, bool fAddLookupRecord)
{
    int32_t displ;
    int     rc = VINF_SUCCESS;

    Assert(PATMIsPatchGCAddr(pVM, pPatchAddrGC));
    PATCHGEN_PROLOG(pVM, pPatch, SIZEOF_NEARJUMP32);

    if (fAddLookupRecord)
    {
        /* Add lookup record for patch to guest address translation */
        patmR3AddP2GLookupRecord(pVM, pPatch, pPB, pCurInstrGC, PATM_LOOKUP_PATCH2GUEST);
    }

    pPB[0] = 0xE9;  //JMP

    displ = pPatchAddrGC - (PATCHCODE_PTR_GC(pPatch) + pPatch->uCurPatchOffset + SIZEOF_NEARJUMP32);

     *(uint32_t *)&pPB[1] = displ;

    PATCHGEN_EPILOG(pPatch, SIZEOF_NEARJUMP32);

    return rc;
}
