/* $Id: PATMR3Dbg.cpp $ */
/** @file
 * PATM - Dynamic Guest OS Patching Manager, Debugger Related Parts.
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
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/hm.h>
#include "PATMInternal.h"
#include "PATMA.h"
#include <VBox/vmm/vm.h>
#include <VBox/err.h>
#include <VBox/log.h>

#include <iprt/assert.h>
#include <iprt/dbg.h>
#include <iprt/string.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** Adds a structure member to a debug (pseudo) module as a symbol. */
#define ADD_MEMBER(a_hDbgMod, a_Struct, a_Member, a_pszName) \
        do { \
            rc = RTDbgModSymbolAdd(hDbgMod, a_pszName, 0 /*iSeg*/, RT_OFFSETOF(a_Struct, a_Member),  \
                                   RT_SIZEOFMEMB(a_Struct, a_Member), 0 /*fFlags*/, NULL /*piOrdinal*/); \
            AssertRC(rc); \
        } while (0)

/** Adds a structure member to a debug (pseudo) module as a symbol. */
#define ADD_FUNC(a_hDbgMod, a_BaseRCPtr, a_FuncRCPtr, a_cbFunc, a_pszName) \
        do { \
            int rcAddFunc = RTDbgModSymbolAdd(hDbgMod, a_pszName, 0 /*iSeg*/, \
                                              (RTRCUINTPTR)a_FuncRCPtr - (RTRCUINTPTR)(a_BaseRCPtr),  \
                                              a_cbFunc, 0 /*fFlags*/, NULL /*piOrdinal*/); \
            AssertRC(rcAddFunc); \
        } while (0)



/**
 * Called by PATMR3Init.
 *
 * @param   pVM         The cross context VM structure.
 */
void patmR3DbgInit(PVM pVM)
{
    pVM->patm.s.hDbgModPatchMem = NIL_RTDBGMOD;
}


/**
 * Called by PATMR3Term.
 *
 * @param   pVM         The cross context VM structure.
 */
void patmR3DbgTerm(PVM pVM)
{
    if (pVM->patm.s.hDbgModPatchMem != NIL_RTDBGMOD)
    {
        RTDbgModRelease(pVM->patm.s.hDbgModPatchMem);
        pVM->patm.s.hDbgModPatchMem = NIL_RTDBGMOD;
    }
}


/**
 * Called by when the patch memory is reinitialized.
 *
 * @param   pVM         The cross context VM structure.
 */
void patmR3DbgReset(PVM pVM)
{
    if (pVM->patm.s.hDbgModPatchMem != NIL_RTDBGMOD)
    {
        RTDbgModRemoveAll(pVM->patm.s.hDbgModPatchMem, true);
    }
}


static size_t patmR3DbgDescribePatchAsSymbol(PPATMPATCHREC pPatchRec, char *pszName, size_t cbLeft)
{
    char * const pszNameStart = pszName;
#define ADD_SZ(a_sz) \
        do { \
            if (cbLeft >= sizeof(a_sz)) \
            { \
                memcpy(pszName, a_sz, sizeof(a_sz)); \
                pszName += sizeof(a_sz) - 1; \
                cbLeft -= sizeof(a_sz) - 1;\
            }\
        } while (0)

    /* Start the name off with the address of the guest code. */
    size_t cch = RTStrPrintf(pszName, cbLeft, "Patch_%#08x", pPatchRec->patch.pPrivInstrGC);
    cbLeft  -= cch;
    pszName += cch;

    /* Append flags. */
    uint64_t fFlags = pPatchRec->patch.flags;
    if (fFlags & PATMFL_INTHANDLER)
        ADD_SZ("_IntHandler");
    if (fFlags & PATMFL_SYSENTER)
        ADD_SZ("_SysEnter");
    if (fFlags & PATMFL_GUEST_SPECIFIC)
        ADD_SZ("_GuestSpecific");
    if (fFlags & PATMFL_USER_MODE)
        ADD_SZ("_UserMode");
    if (fFlags & PATMFL_IDTHANDLER)
        ADD_SZ("_IdtHnd");
    if (fFlags & PATMFL_TRAPHANDLER)
        ADD_SZ("_TrapHnd");
    if (fFlags & PATMFL_DUPLICATE_FUNCTION)
        ADD_SZ("_DupFunc");
    if (fFlags & PATMFL_REPLACE_FUNCTION_CALL)
        ADD_SZ("_ReplFunc");
    if (fFlags & PATMFL_TRAPHANDLER_WITH_ERRORCODE)
        ADD_SZ("_TrapHndErrCd");
    if (fFlags & PATMFL_MMIO_ACCESS)
        ADD_SZ("_MmioAccess");
    if (fFlags & PATMFL_SYSENTER_XP)
        ADD_SZ("_SysEnterXP");
    if (fFlags & PATMFL_INT3_REPLACEMENT)
        ADD_SZ("_Int3Repl");
    if (fFlags & PATMFL_SUPPORT_CALLS)
        ADD_SZ("_SupCalls");
    if (fFlags & PATMFL_SUPPORT_INDIRECT_CALLS)
        ADD_SZ("_SupIndirCalls");
    if (fFlags & PATMFL_IDTHANDLER_WITHOUT_ENTRYPOINT)
        ADD_SZ("_IdtHandlerWE");
    if (fFlags & PATMFL_INHIBIT_IRQS)
        ADD_SZ("_InhibitIrqs");
    if (fFlags & PATMFL_RECOMPILE_NEXT)
        ADD_SZ("_RecompileNext");
    if (fFlags & PATMFL_CALLABLE_AS_FUNCTION)
        ADD_SZ("_Callable");
    if (fFlags & PATMFL_TRAMPOLINE)
        ADD_SZ("_Trampoline");
    if (fFlags & PATMFL_PATCHED_GUEST_CODE)
        ADD_SZ("_PatchedGuestCode");
    if (fFlags & PATMFL_MUST_INSTALL_PATCHJMP)
        ADD_SZ("_MustInstallPatchJmp");
    if (fFlags & PATMFL_INT3_REPLACEMENT_BLOCK)
        ADD_SZ("_Int3ReplBlock");
    if (fFlags & PATMFL_EXTERNAL_JUMP_INSIDE)
        ADD_SZ("_ExtJmp");
    if (fFlags & PATMFL_CODE_REFERENCED)
        ADD_SZ("_CodeRefed");

    return pszName - pszNameStart;
}


/**
 * Called when a new patch is added or when first populating the address space.
 *
 * @param   pVM                The cross context VM structure.
 * @param   pPatchRec           The patch record.
 */
void patmR3DbgAddPatch(PVM pVM, PPATMPATCHREC pPatchRec)
{
    if (   pVM->patm.s.hDbgModPatchMem != NIL_RTDBGMOD
        && pPatchRec->patch.pPatchBlockOffset > 0
        && !(pPatchRec->patch.flags & PATMFL_GLOBAL_FUNCTIONS))
    {
        /** @todo find a cheap way of checking whether we've already added the patch.
         *        Using a flag would be nice, except I don't want to consider saved
         *        state considerations right now (I don't recall if we're still
         *        depending on structure layout there or not). */
        char   szName[256];
        size_t off = patmR3DbgDescribePatchAsSymbol(pPatchRec, szName, sizeof(szName));

        /* If we have a symbol near the guest address, append that. */
        if (off + 8 <= sizeof(szName))
        {
            RTDBGSYMBOL Symbol;
            RTGCINTPTR  offDisp;
            DBGFADDRESS Addr;

            int rc = DBGFR3AsSymbolByAddr(pVM->pUVM, DBGF_AS_GLOBAL,
                                          DBGFR3AddrFromFlat(pVM->pUVM, &Addr, pPatchRec->patch.pPrivInstrGC),
                                          RTDBGSYMADDR_FLAGS_LESS_OR_EQUAL,
                                          &offDisp, &Symbol, NULL /*phMod*/);
            if (RT_SUCCESS(rc))
            {
                szName[off++] = '_';
                szName[off++] = '_';
                RTStrCopy(&szName[off], sizeof(szName) - off, Symbol.szName);
            }
        }

        /* Add it (may fail due to enable/disable patches). */
        RTDbgModSymbolAdd(pVM->patm.s.hDbgModPatchMem, szName, 0 /*iSeg*/,
                          pPatchRec->patch.pPatchBlockOffset,
                          pPatchRec->patch.cbPatchBlockSize,
                          0 /*fFlags*/, NULL /*piOrdinal*/);
    }
}


/**
 * Enumeration callback used by patmR3DbgAddPatches
 *
 * @returns 0 (continue enum)
 * @param   pNode       The patch record node.
 * @param   pvUser      The cross context VM structure.
 */
static DECLCALLBACK(int) patmR3DbgAddPatchCallback(PAVLOU32NODECORE pNode, void *pvUser)
{
    patmR3DbgAddPatch((PVM)pvUser, (PPATMPATCHREC)pNode);
    return 0;
}


/**
 * Populates an empty "patches" (hDbgModPatchMem) module with patch symbols.
 *
 * @param   pVM         The cross context VM structure.
 * @param   hDbgMod     The debug module handle.
 */
static void patmR3DbgAddPatches(PVM pVM, RTDBGMOD hDbgMod)
{
    /*
     * Global functions and a start marker.
     */
    ADD_FUNC(hDbgMod, pVM->patm.s.pPatchMemGC, pVM->patm.s.pfnHelperCallGC, g_patmLookupAndCallRecord.cbFunction, "PATMLookupAndCall");
    ADD_FUNC(hDbgMod, pVM->patm.s.pPatchMemGC, pVM->patm.s.pfnHelperRetGC,  g_patmRetFunctionRecord.cbFunction,   "PATMRetFunction");
    ADD_FUNC(hDbgMod, pVM->patm.s.pPatchMemGC, pVM->patm.s.pfnHelperJumpGC, g_patmLookupAndJumpRecord.cbFunction, "PATMLookupAndJump");
    ADD_FUNC(hDbgMod, pVM->patm.s.pPatchMemGC, pVM->patm.s.pfnHelperIretGC, g_patmIretFunctionRecord.cbFunction,  "PATMIretFunction");

    ADD_FUNC(hDbgMod, pVM->patm.s.pPatchMemGC, pVM->patm.s.pPatchMemGC, 0,  "PatchMemStart");
    ADD_FUNC(hDbgMod, pVM->patm.s.pPatchMemGC, pVM->patm.s.pGCStackGC, PATM_STACK_TOTAL_SIZE, "PATMStack");

    /*
     * The patches.
     */
    RTAvloU32DoWithAll(&pVM->patm.s.PatchLookupTreeHC->PatchTree, true /*fFromLeft*/, patmR3DbgAddPatchCallback, pVM);
}


/**
 * Populate DBGF_AS_RC with PATM symbols.
 *
 * Called by dbgfR3AsLazyPopulate when DBGF_AS_RC or DBGF_AS_RC_AND_GC_GLOBAL is
 * accessed for the first time.
 *
 * @param   pVM         The cross context VM structure.
 * @param   hDbgAs      The DBGF_AS_RC address space handle.
 */
VMMR3_INT_DECL(void) PATMR3DbgPopulateAddrSpace(PVM pVM, RTDBGAS hDbgAs)
{
    AssertReturnVoid(!HMIsEnabled(pVM));

    /*
     * Add a fake debug module for the PATMGCSTATE structure.
     */
    RTDBGMOD hDbgMod;
    int rc = RTDbgModCreate(&hDbgMod, "patmgcstate", sizeof(PATMGCSTATE), 0 /*fFlags*/);
    if (RT_SUCCESS(rc))
    {
        ADD_MEMBER(hDbgMod, PATMGCSTATE, uVMFlags,                  "uVMFlags");
        ADD_MEMBER(hDbgMod, PATMGCSTATE, uPendingAction,            "uPendingAction");
        ADD_MEMBER(hDbgMod, PATMGCSTATE, uPatchCalls,               "uPatchCalls");
        ADD_MEMBER(hDbgMod, PATMGCSTATE, uScratch,                  "uScratch");
        ADD_MEMBER(hDbgMod, PATMGCSTATE, uIretEFlags,               "uIretEFlags");
        ADD_MEMBER(hDbgMod, PATMGCSTATE, uIretCS,                   "uIretCS");
        ADD_MEMBER(hDbgMod, PATMGCSTATE, uIretEIP,                  "uIretEIP");
        ADD_MEMBER(hDbgMod, PATMGCSTATE, Psp,                       "Psp");
        ADD_MEMBER(hDbgMod, PATMGCSTATE, fPIF,                      "fPIF");
        ADD_MEMBER(hDbgMod, PATMGCSTATE, GCPtrInhibitInterrupts,    "GCPtrInhibitInterrupts");
        ADD_MEMBER(hDbgMod, PATMGCSTATE, GCCallPatchTargetAddr,     "GCCallPatchTargetAddr");
        ADD_MEMBER(hDbgMod, PATMGCSTATE, GCCallReturnAddr,          "GCCallReturnAddr");
        ADD_MEMBER(hDbgMod, PATMGCSTATE, Restore.uEAX,              "Restore.uEAX");
        ADD_MEMBER(hDbgMod, PATMGCSTATE, Restore.uECX,              "Restore.uECX");
        ADD_MEMBER(hDbgMod, PATMGCSTATE, Restore.uEDI,              "Restore.uEDI");
        ADD_MEMBER(hDbgMod, PATMGCSTATE, Restore.eFlags,            "Restore.eFlags");
        ADD_MEMBER(hDbgMod, PATMGCSTATE, Restore.uFlags,            "Restore.uFlags");

        rc = RTDbgAsModuleLink(hDbgAs, hDbgMod, pVM->patm.s.pGCStateGC, 0 /*fFlags*/);
        AssertLogRelRC(rc);
        RTDbgModRelease(hDbgMod);
    }

    /*
     * Add something for the stats so we get some kind of symbols for
     * references to them while disassembling patches.
     */
    rc = RTDbgModCreate(&hDbgMod, "patmstats", PATM_STAT_MEMSIZE, 0 /*fFlags*/);
    if (RT_SUCCESS(rc))
    {
        ADD_FUNC(hDbgMod, pVM->patm.s.pStatsGC, pVM->patm.s.pStatsGC, PATM_STAT_MEMSIZE, "PATMMemStatsStart");

        rc = RTDbgAsModuleLink(hDbgAs, hDbgMod, pVM->patm.s.pStatsGC, 0 /*fFlags*/);
        AssertLogRelRC(rc);
        RTDbgModRelease(hDbgMod);
    }

    /*
     * Add a fake debug module for the patches and stack.
     */
    rc = RTDbgModCreate(&hDbgMod, "patches", pVM->patm.s.cbPatchMem + PATM_STACK_TOTAL_SIZE + PAGE_SIZE, 0 /*fFlags*/);
    if (RT_SUCCESS(rc))
    {
        pVM->patm.s.hDbgModPatchMem = hDbgMod;
        patmR3DbgAddPatches(pVM, hDbgMod);

        rc = RTDbgAsModuleLink(hDbgAs, hDbgMod, pVM->patm.s.pPatchMemGC, 0 /*fFlags*/);
        AssertLogRelRC(rc);
    }
}


/**
 * Annotates an instruction if patched.
 *
 * @param   pVM         The cross context VM structure.
 * @param   RCPtr       The instruction address.
 * @param   cbInstr     The instruction length.
 * @param   pszBuf      The output buffer.  This will be an empty string if the
 *                      instruction wasn't patched.  If it's patched, it will
 *                      hold a symbol-like string describing the patch.
 * @param   cbBuf       The size of the output buffer.
 */
VMMR3_INT_DECL(void) PATMR3DbgAnnotatePatchedInstruction(PVM pVM, RTRCPTR RCPtr, uint8_t cbInstr, char *pszBuf, size_t cbBuf)
{
    /*
     * Always zero the buffer.
     */
    AssertReturnVoid(cbBuf > 0);
    *pszBuf = '\0';

    /*
     * Drop out immediately if it cannot be a patched instruction.
     */
    if (!PATMIsEnabled(pVM))
        return;
    if (   RCPtr < pVM->patm.s.pPatchedInstrGCLowest
        || RCPtr > pVM->patm.s.pPatchedInstrGCHighest)
        return;

    /*
     * Look for a patch record covering any part of the instruction.
     *
     * The first query results in a patched less or equal to RCPtr. While the
     * second results in one that's greater than RCPtr.
     */
    PPATMPATCHREC pPatchRec;
    pPatchRec = (PPATMPATCHREC)RTAvloU32GetBestFit(&pVM->patm.s.PatchLookupTreeHC->PatchTree, RCPtr, false /*fFromAbove*/);
    if (   !pPatchRec
        || RCPtr - pPatchRec->patch.pPrivInstrGC > pPatchRec->patch.cbPrivInstr)
    {
        pPatchRec = (PPATMPATCHREC)RTAvloU32GetBestFit(&pVM->patm.s.PatchLookupTreeHC->PatchTree, RCPtr, true /*fFromAbove*/);
        if (   !pPatchRec
            || (RTRCPTR)(RCPtr + cbInstr) < pPatchRec->patch.pPrivInstrGC )
            return;
    }

    /*
     * Lazy bird uses the symbol name generation code for describing the patch.
     */
    size_t off = patmR3DbgDescribePatchAsSymbol(pPatchRec, pszBuf, cbBuf);
    if (off + 1 < cbBuf)
    {
        const char *pszState;
        switch (pPatchRec->patch.uState)
        {
            case PATCH_REFUSED:             pszState = "Refused"; break;
            case PATCH_DISABLED:            pszState = "Disabled"; break;
            case PATCH_ENABLED:             pszState = "Enabled"; break;
            case PATCH_UNUSABLE:            pszState = "Unusable"; break;
            case PATCH_DIRTY:               pszState = "Dirty"; break;
            case PATCH_DISABLE_PENDING:     pszState = "DisablePending"; break;
            default:                        pszState = "State???"; AssertFailed(); break;
        }

        if (pPatchRec->patch.cbPatchBlockSize > 0)
            off += RTStrPrintf(&pszBuf[off], cbBuf - off, " - %s (%u b) - %#x LB %#x",
                               pszState, pPatchRec->patch.cbPatchJump,
                               pPatchRec->patch.pPatchBlockOffset + pVM->patm.s.pPatchMemGC,
                               pPatchRec->patch.cbPatchBlockSize);
        else
            off += RTStrPrintf(&pszBuf[off], cbBuf - off, " - %s (%u b)", pszState, pPatchRec->patch.cbPatchJump);
    }

}

