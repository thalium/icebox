/* $Id: PATM.cpp $ */
/** @file
 * PATM - Dynamic Guest OS Patching Manager
 *
 * @note Never ever reuse patch memory!!
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

/** @page pg_patm   PATM - Patch Manager
 *
 * The patch manager (PATM) patches privileged guest code to allow it to execute
 * directly in raw-mode.
 *
 * The PATM works closely together with the @ref pg_csam "CSAM" detect code
 * needing patching and detected changes to the patch.  It also interfaces with
 * other components, like @ref pg_trpm "TRPM" and @ref pg_rem "REM", for these
 * purposes.
 *
 * @sa @ref grp_patm
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_PATM
#include <VBox/vmm/patm.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/cpumdis.h>
#include <VBox/vmm/iom.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/ssm.h>
#include <VBox/vmm/trpm.h>
#include <VBox/vmm/cfgm.h>
#include <VBox/param.h>
#include <VBox/vmm/selm.h>
#include <VBox/vmm/csam.h>
#include <iprt/avl.h>
#include "PATMInternal.h"
#include "PATMPatch.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/uvm.h>
#include <VBox/dbg.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <VBox/dis.h>
#include <VBox/disopcode.h>

#include <iprt/string.h>
#include "PATMA.h"

//#define PATM_REMOVE_PATCH_ON_TOO_MANY_TRAPS
//#define PATM_DISABLE_ALL

/**
 * Refresh trampoline patch state.
 */
typedef struct PATMREFRESHPATCH
{
    /** Pointer to the VM structure. */
    PVM        pVM;
    /** The trampoline patch record. */
    PPATCHINFO pPatchTrampoline;
    /** The new patch we want to jump to. */
    PPATCHINFO pPatchRec;
} PATMREFRESHPATCH, *PPATMREFRESHPATCH;


#define PATMREAD_RAWCODE        1  /* read code as-is */
#define PATMREAD_ORGCODE        2  /* read original guest opcode bytes; not the patched bytes */
#define PATMREAD_NOCHECK        4  /* don't check for patch conflicts */

/*
 * Private structure used during disassembly
 */
typedef struct
{
    PVM                  pVM;
    PPATCHINFO           pPatchInfo;
    R3PTRTYPE(uint8_t *) pbInstrHC;
    RTRCPTR              pInstrGC;
    uint32_t             fReadFlags;
} PATMDISASM, *PPATMDISASM;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int          patmDisableUnusablePatch(PVM pVM, RTRCPTR pInstrGC, RTRCPTR pConflictAddr, PPATCHINFO pPatch);
static int          patmActivateInt3Patch(PVM pVM, PPATCHINFO pPatch);
static int          patmDeactivateInt3Patch(PVM pVM, PPATCHINFO pPatch);

#ifdef LOG_ENABLED // keep gcc quiet
static bool         patmIsCommonIDTHandlerPatch(PVM pVM, RTRCPTR pInstrGC);
#endif
#ifdef VBOX_WITH_STATISTICS
static const char  *PATMPatchType(PVM pVM, PPATCHINFO pPatch);
static void         patmResetStat(PVM pVM, void *pvSample);
static void         patmPrintStat(PVM pVM, void *pvSample, char *pszBuf, size_t cchBuf);
#endif

#define             patmPatchHCPtr2PatchGCPtr(pVM, pHC)      (pVM->patm.s.pPatchMemGC + (pHC - pVM->patm.s.pPatchMemHC))
#define             patmPatchGCPtr2PatchHCPtr(pVM, pGC)      (pVM->patm.s.pPatchMemHC + (pGC - pVM->patm.s.pPatchMemGC))

static int               patmReinit(PVM pVM);
static DECLCALLBACK(int) patmR3RelocatePatches(PAVLOU32NODECORE pNode, void *pParam);
#ifdef PATM_RESOLVE_CONFLICTS_WITH_JUMP_PATCHES
static RTRCPTR      patmR3GuestGCPtrToPatchGCPtrSimple(PVM pVM, RCPTRTYPE(uint8_t*) pInstrGC);
#endif
static int          patmR3MarkDirtyPatch(PVM pVM, PPATCHINFO pPatch);

#ifdef VBOX_WITH_DEBUGGER
static DECLCALLBACK(int) DisableAllPatches(PAVLOU32NODECORE pNode, void *pVM);
static FNDBGCCMD    patmr3CmdOn;
static FNDBGCCMD    patmr3CmdOff;

/** Command descriptors. */
static const DBGCCMD    g_aCmds[] =
{
    /* pszCmd,      cArgsMin, cArgsMax, paArgDesc, cArgDescs, fFlags, pfnHandler    pszSyntax, ....pszDescription */
    { "patmon",     0,        0,        NULL,      0,         0,      patmr3CmdOn,  "",        "Enable patching."  },
    { "patmoff",    0,        0,        NULL,      0,         0,      patmr3CmdOff, "",        "Disable patching." },
};
#endif

/* Don't want to break saved states, so put it here as a global variable. */
static unsigned int cIDTHandlersDisabled = 0;

/**
 * Initializes the PATM.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3_INT_DECL(int) PATMR3Init(PVM pVM)
{
    int rc;

    /*
     * We only need a saved state dummy loader if HM is enabled.
     */
    if (HMIsEnabled(pVM))
    {
        pVM->fPATMEnabled = false;
        return SSMR3RegisterStub(pVM, "PATM", 0);
    }

    /*
     * Raw-mode.
     */
    Log(("PATMR3Init: Patch record size %d\n", sizeof(PATCHINFO)));

    /* These values can't change as they are hardcoded in patch code (old saved states!) */
    AssertCompile(VMCPU_FF_TIMER == RT_BIT_32(2));
    AssertCompile(VM_FF_REQUEST == VMCPU_FF_REQUEST);
    AssertCompile(VMCPU_FF_INTERRUPT_APIC == RT_BIT_32(0));
    AssertCompile(VMCPU_FF_INTERRUPT_PIC == RT_BIT_32(1));

    AssertReleaseMsg(g_fPatmInterruptFlag == (VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC | VMCPU_FF_TIMER | VMCPU_FF_REQUEST),
                     ("Interrupt flags out of sync!! g_fPatmInterruptFlag=%#x expected %#x. broken assembler?\n", g_fPatmInterruptFlag, VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC | VMCPU_FF_TIMER | VMCPU_FF_REQUEST));

    /* Allocate patch memory and GC patch state memory. */
    pVM->patm.s.cbPatchMem = PATCH_MEMORY_SIZE;
    /* Add another page in case the generated code is much larger than expected. */
    /** @todo bad safety precaution */
    rc = MMR3HyperAllocOnceNoRel(pVM, PATCH_MEMORY_SIZE + PAGE_SIZE + PATM_STACK_TOTAL_SIZE + PAGE_SIZE + PATM_STAT_MEMSIZE, PAGE_SIZE, MM_TAG_PATM, (void **)&pVM->patm.s.pPatchMemHC);
    if (RT_FAILURE(rc))
    {
        Log(("MMHyperAlloc failed with %Rrc\n", rc));
        return rc;
    }
    pVM->patm.s.pPatchMemGC = MMHyperR3ToRC(pVM, pVM->patm.s.pPatchMemHC);

    /* PATM stack page for call instruction execution. (2 parts: one for our private stack and one to store the original return address */
    pVM->patm.s.pGCStackHC  = (RTRCPTR *)(pVM->patm.s.pPatchMemHC + PATCH_MEMORY_SIZE + PAGE_SIZE);
    pVM->patm.s.pGCStackGC  = MMHyperR3ToRC(pVM, pVM->patm.s.pGCStackHC);

    patmR3DbgInit(pVM);

    /*
     * Hypervisor memory for GC status data (read/write)
     *
     * Note1: This is non-critical data; if trashed by the guest, then it will only cause problems for itself
     * Note2: This doesn't really belong here, but we need access to it for relocation purposes
     *
     */
    Assert(sizeof(PATMGCSTATE) < PAGE_SIZE);    /* Note: hardcoded dependencies on this exist. */
    pVM->patm.s.pGCStateHC  = (PPATMGCSTATE)((uint8_t *)pVM->patm.s.pGCStackHC + PATM_STACK_TOTAL_SIZE);
    pVM->patm.s.pGCStateGC  = MMHyperR3ToRC(pVM, pVM->patm.s.pGCStateHC);

    /* Hypervisor memory for patch statistics */
    pVM->patm.s.pStatsHC  = (PSTAMRATIOU32)((uint8_t *)pVM->patm.s.pGCStateHC + PAGE_SIZE);
    pVM->patm.s.pStatsGC  = MMHyperR3ToRC(pVM, pVM->patm.s.pStatsHC);

    /* Memory for patch lookup trees. */
    rc = MMHyperAlloc(pVM, sizeof(*pVM->patm.s.PatchLookupTreeHC), 0, MM_TAG_PATM, (void **)&pVM->patm.s.PatchLookupTreeHC);
    AssertRCReturn(rc, rc);
    pVM->patm.s.PatchLookupTreeGC = MMHyperR3ToRC(pVM, pVM->patm.s.PatchLookupTreeHC);

#ifdef RT_ARCH_AMD64 /* see patmReinit(). */
    /* Check CFGM option. */
    rc = CFGMR3QueryBool(CFGMR3GetRoot(pVM), "PATMEnabled", &pVM->fPATMEnabled);
    if (RT_FAILURE(rc))
# ifdef PATM_DISABLE_ALL
        pVM->fPATMEnabled = false;
# else
        pVM->fPATMEnabled = true;
# endif
#endif

    rc = patmReinit(pVM);
    AssertRC(rc);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Register the virtual page access handler type.
     */
    rc = PGMR3HandlerVirtualTypeRegister(pVM, PGMVIRTHANDLERKIND_ALL, false /*fRelocUserRC*/,
                                         NULL /*pfnInvalidateR3*/,
                                         patmVirtPageHandler,
                                         "patmVirtPageHandler", "patmRCVirtPagePfHandler",
                                         "PATMMonitorPatchJump", &pVM->patm.s.hMonitorPageType);
    AssertRCReturn(rc, rc);

    /*
     * Register save and load state notifiers.
     */
    rc = SSMR3RegisterInternal(pVM, "PATM", 0, PATM_SAVED_STATE_VERSION, sizeof(pVM->patm.s) + PATCH_MEMORY_SIZE  + PAGE_SIZE + PATM_STACK_TOTAL_SIZE + PAGE_SIZE,
                               NULL, NULL, NULL,
                               NULL, patmR3Save, NULL,
                               NULL, patmR3Load, NULL);
    AssertRCReturn(rc, rc);

#ifdef VBOX_WITH_DEBUGGER
    /*
     * Debugger commands.
     */
    static bool s_fRegisteredCmds = false;
    if (!s_fRegisteredCmds)
    {
        int rc2 = DBGCRegisterCommands(&g_aCmds[0], RT_ELEMENTS(g_aCmds));
        if (RT_SUCCESS(rc2))
            s_fRegisteredCmds = true;
    }
#endif

#ifdef VBOX_WITH_STATISTICS
    STAM_REG(pVM, &pVM->patm.s.StatNrOpcodeRead,  STAMTYPE_COUNTER, "/PATM/OpcodeBytesRead",      STAMUNIT_OCCURENCES,     "The number of opcode bytes read by the recompiler.");
    STAM_REG(pVM, &pVM->patm.s.StatPATMMemoryUsed,STAMTYPE_COUNTER, "/PATM/MemoryUsed",           STAMUNIT_OCCURENCES,     "The amount of hypervisor heap used for patches.");
    STAM_REG(pVM, &pVM->patm.s.StatDisabled,      STAMTYPE_COUNTER, "/PATM/Patch/Disabled",        STAMUNIT_OCCURENCES,     "Number of times patches were disabled.");
    STAM_REG(pVM, &pVM->patm.s.StatEnabled,       STAMTYPE_COUNTER, "/PATM/Patch/Enabled",         STAMUNIT_OCCURENCES,     "Number of times patches were enabled.");
    STAM_REG(pVM, &pVM->patm.s.StatDirty,         STAMTYPE_COUNTER, "/PATM/Patch/Dirty",           STAMUNIT_OCCURENCES,     "Number of times patches were marked dirty.");
    STAM_REG(pVM, &pVM->patm.s.StatUnusable,      STAMTYPE_COUNTER, "/PATM/Patch/Unusable",        STAMUNIT_OCCURENCES,     "Number of unusable patches (conflicts).");
    STAM_REG(pVM, &pVM->patm.s.StatInstalled,     STAMTYPE_COUNTER, "/PATM/Patch/Installed",       STAMUNIT_OCCURENCES,     "Number of installed patches.");
    STAM_REG(pVM, &pVM->patm.s.StatInt3Callable,  STAMTYPE_COUNTER, "/PATM/Patch/Int3Callable",    STAMUNIT_OCCURENCES,     "Number of cli patches turned into int3 patches.");

    STAM_REG(pVM, &pVM->patm.s.StatInt3BlockRun,  STAMTYPE_COUNTER, "/PATM/Patch/Run/Int3",        STAMUNIT_OCCURENCES,     "Number of times an int3 block patch was executed.");
    STAMR3RegisterF(pVM, &pVM->patm.s.pGCStateHC->uPatchCalls, STAMTYPE_U32, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, NULL, "/PATM/Patch/Run/Normal");

    STAM_REG(pVM, &pVM->patm.s.StatInstalledFunctionPatches,   STAMTYPE_COUNTER, "/PATM/Patch/Installed/Function",       STAMUNIT_OCCURENCES,     "Number of installed function duplication patches.");
    STAM_REG(pVM, &pVM->patm.s.StatInstalledTrampoline,   STAMTYPE_COUNTER, "/PATM/Patch/Installed/Trampoline",          STAMUNIT_OCCURENCES,     "Number of installed trampoline patches.");
    STAM_REG(pVM, &pVM->patm.s.StatInstalledJump, STAMTYPE_COUNTER, "/PATM/Patch/Installed/Jump",  STAMUNIT_OCCURENCES,     "Number of installed jump patches.");

    STAM_REG(pVM, &pVM->patm.s.StatOverwritten,   STAMTYPE_COUNTER, "/PATM/Patch/Overwritten",     STAMUNIT_OCCURENCES,     "Number of overwritten patches.");
    STAM_REG(pVM, &pVM->patm.s.StatFixedConflicts,STAMTYPE_COUNTER, "/PATM/Patch/ConflictFixed",   STAMUNIT_OCCURENCES,     "Number of fixed conflicts.");
    STAM_REG(pVM, &pVM->patm.s.StatFlushed,       STAMTYPE_COUNTER, "/PATM/Patch/Flushed",         STAMUNIT_OCCURENCES,     "Number of flushes of pages with patch jumps.");
    STAM_REG(pVM, &pVM->patm.s.StatMonitored,     STAMTYPE_COUNTER, "/PATM/Patch/Monitored",       STAMUNIT_OCCURENCES,     "Number of patches in monitored patch pages.");
    STAM_REG(pVM, &pVM->patm.s.StatPageBoundaryCrossed, STAMTYPE_COUNTER, "/PATM/Patch/BoundaryCross", STAMUNIT_OCCURENCES,     "Number of refused patches due to patch jump crossing page boundary.");

    STAM_REG(pVM, &pVM->patm.s.StatHandleTrap,    STAMTYPE_PROFILE, "/PATM/HandleTrap",           STAMUNIT_TICKS_PER_CALL, "Profiling of PATMR3HandleTrap");
    STAM_REG(pVM, &pVM->patm.s.StatPushTrap,      STAMTYPE_COUNTER, "/PATM/HandleTrap/PushWP",    STAMUNIT_OCCURENCES,     "Number of traps due to monitored stack pages.");

    STAM_REG(pVM, &pVM->patm.s.StatSwitchBack,    STAMTYPE_COUNTER, "/PATM/SwitchBack",           STAMUNIT_OCCURENCES,     "Switch back to original guest code when IF=1 & executing PATM instructions");
    STAM_REG(pVM, &pVM->patm.s.StatSwitchBackFail,STAMTYPE_COUNTER, "/PATM/SwitchBackFail",       STAMUNIT_OCCURENCES,     "Failed switch back to original guest code when IF=1 & executing PATM instructions");

    STAM_REG(pVM, &pVM->patm.s.StatDuplicateREQFailed,  STAMTYPE_COUNTER,      "/PATM/Function/DupREQ/Failed",     STAMUNIT_OCCURENCES,    "Nr of failed PATMR3DuplicateFunctionRequest calls");
    STAM_REG(pVM, &pVM->patm.s.StatDuplicateREQSuccess, STAMTYPE_COUNTER,      "/PATM/Function/DupREQ/Success",    STAMUNIT_OCCURENCES,    "Nr of successful PATMR3DuplicateFunctionRequest calls");
    STAM_REG(pVM, &pVM->patm.s.StatDuplicateUseExisting,STAMTYPE_COUNTER,      "/PATM/Function/DupREQ/UseExist",   STAMUNIT_OCCURENCES,    "Nr of successful PATMR3DuplicateFunctionRequest calls when using an existing patch");

    STAM_REG(pVM, &pVM->patm.s.StatFunctionLookupInsert,  STAMTYPE_COUNTER,    "/PATM/Function/Lookup/Insert",     STAMUNIT_OCCURENCES,    "Nr of successful function address insertions");
    STAM_REG(pVM, &pVM->patm.s.StatFunctionLookupReplace, STAMTYPE_COUNTER,    "/PATM/Function/Lookup/Replace",    STAMUNIT_OCCURENCES,    "Nr of successful function address replacements");
    STAM_REG(pVM, &pVM->patm.s.StatU32FunctionMaxSlotsUsed, STAMTYPE_U32_RESET,"/PATM/Function/Lookup/MaxSlots",   STAMUNIT_OCCURENCES,    "Maximum nr of lookup slots used in all call patches");

    STAM_REG(pVM, &pVM->patm.s.StatFunctionFound,     STAMTYPE_COUNTER, "/PATM/Function/Found",     STAMUNIT_OCCURENCES,     "Nr of successful function patch lookups in GC");
    STAM_REG(pVM, &pVM->patm.s.StatFunctionNotFound,  STAMTYPE_COUNTER, "/PATM/Function/NotFound",  STAMUNIT_OCCURENCES,     "Nr of failed function patch lookups in GC");

    STAM_REG(pVM, &pVM->patm.s.StatPatchWrite,        STAMTYPE_PROFILE, "/PATM/Write/Handle",       STAMUNIT_TICKS_PER_CALL, "Profiling of PATMR3PatchWrite");
    STAM_REG(pVM, &pVM->patm.s.StatPatchWriteDetect,  STAMTYPE_PROFILE, "/PATM/Write/Detect",       STAMUNIT_TICKS_PER_CALL, "Profiling of PATMIsWriteToPatchPage");
    STAM_REG(pVM, &pVM->patm.s.StatPatchWriteInterpreted, STAMTYPE_COUNTER, "/PATM/Write/Interpreted/Success", STAMUNIT_OCCURENCES,  "Nr of interpreted patch writes.");
    STAM_REG(pVM, &pVM->patm.s.StatPatchWriteInterpretedFailed, STAMTYPE_COUNTER, "/PATM/Write/Interpreted/Failed", STAMUNIT_OCCURENCES,  "Nr of failed interpreted patch writes.");

    STAM_REG(pVM, &pVM->patm.s.StatPatchRefreshSuccess, STAMTYPE_COUNTER, "/PATM/Refresh/Success",    STAMUNIT_OCCURENCES, "Successful patch refreshes");
    STAM_REG(pVM, &pVM->patm.s.StatPatchRefreshFailed,  STAMTYPE_COUNTER, "/PATM/Refresh/Failure",    STAMUNIT_OCCURENCES, "Failed patch refreshes");

    STAM_REG(pVM, &pVM->patm.s.StatPatchPageInserted, STAMTYPE_COUNTER, "/PATM/Page/Inserted",      STAMUNIT_OCCURENCES,     "Nr of inserted guest pages that were patched");
    STAM_REG(pVM, &pVM->patm.s.StatPatchPageRemoved,  STAMTYPE_COUNTER, "/PATM/Page/Removed",       STAMUNIT_OCCURENCES,     "Nr of removed guest pages that were patched");

    STAM_REG(pVM, &pVM->patm.s.StatInstrDirty,        STAMTYPE_COUNTER, "/PATM/Instr/Dirty/Detected",  STAMUNIT_OCCURENCES,     "Number of times instructions were marked dirty.");
    STAM_REG(pVM, &pVM->patm.s.StatInstrDirtyGood,    STAMTYPE_COUNTER, "/PATM/Instr/Dirty/Corrected", STAMUNIT_OCCURENCES,     "Number of times instructions were marked dirty and corrected later on.");
    STAM_REG(pVM, &pVM->patm.s.StatInstrDirtyBad,     STAMTYPE_COUNTER, "/PATM/Instr/Dirty/Failed",    STAMUNIT_OCCURENCES,     "Number of times instructions were marked dirty and we were not able to correct them.");

    STAM_REG(pVM, &pVM->patm.s.StatSysEnter,          STAMTYPE_COUNTER, "/PATM/Emul/SysEnter",         STAMUNIT_OCCURENCES,     "Number of times sysenter was emulated.");
    STAM_REG(pVM, &pVM->patm.s.StatSysExit,           STAMTYPE_COUNTER, "/PATM/Emul/SysExit" ,         STAMUNIT_OCCURENCES,     "Number of times sysexit was emulated.");
    STAM_REG(pVM, &pVM->patm.s.StatEmulIret,          STAMTYPE_COUNTER, "/PATM/Emul/Iret/Success",     STAMUNIT_OCCURENCES,     "Number of times iret was emulated.");
    STAM_REG(pVM, &pVM->patm.s.StatEmulIretFailed,    STAMTYPE_COUNTER, "/PATM/Emul/Iret/Failed",      STAMUNIT_OCCURENCES,     "Number of times iret was emulated.");

    STAM_REG(pVM, &pVM->patm.s.StatGenRet,            STAMTYPE_COUNTER, "/PATM/Gen/Ret" ,         STAMUNIT_OCCURENCES,     "Number of generated ret instructions.");
    STAM_REG(pVM, &pVM->patm.s.StatGenRetReused,      STAMTYPE_COUNTER, "/PATM/Gen/RetReused" ,   STAMUNIT_OCCURENCES,     "Number of reused ret instructions.");
    STAM_REG(pVM, &pVM->patm.s.StatGenCall,           STAMTYPE_COUNTER, "/PATM/Gen/Call",         STAMUNIT_OCCURENCES,     "Number of generated call instructions.");
    STAM_REG(pVM, &pVM->patm.s.StatGenJump,           STAMTYPE_COUNTER, "/PATM/Gen/Jmp" ,         STAMUNIT_OCCURENCES,     "Number of generated indirect jump instructions.");
    STAM_REG(pVM, &pVM->patm.s.StatGenPopf,           STAMTYPE_COUNTER, "/PATM/Gen/Popf" ,        STAMUNIT_OCCURENCES,     "Number of generated popf instructions.");

    STAM_REG(pVM, &pVM->patm.s.StatCheckPendingIRQ,   STAMTYPE_COUNTER, "/PATM/GC/CheckIRQ" ,        STAMUNIT_OCCURENCES,     "Number of traps that ask to check for pending irqs.");
#endif /* VBOX_WITH_STATISTICS */

    Log(("g_patmCallRecord.cbFunction           %u\n", g_patmCallRecord.cbFunction));
    Log(("g_patmCallIndirectRecord.cbFunction   %u\n", g_patmCallIndirectRecord.cbFunction));
    Log(("g_patmRetRecord.cbFunction            %u\n", g_patmRetRecord.cbFunction));
    Log(("g_patmJumpIndirectRecord.cbFunction   %u\n", g_patmJumpIndirectRecord.cbFunction));
    Log(("g_patmPopf32Record.cbFunction         %u\n", g_patmPopf32Record.cbFunction));
    Log(("g_patmIretRecord.cbFunction           %u\n", g_patmIretRecord.cbFunction));
    Log(("g_patmStiRecord.cbFunction            %u\n", g_patmStiRecord.cbFunction));
    Log(("g_patmCheckIFRecord.cbFunction        %u\n", g_patmCheckIFRecord.cbFunction));

    return rc;
}

/**
 * Finalizes HMA page attributes.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 */
VMMR3_INT_DECL(int) PATMR3InitFinalize(PVM pVM)
{
    if (HMIsEnabled(pVM))
        return VINF_SUCCESS;

    /*
     * The GC state, stack and statistics must be read/write for the guest
     * (supervisor only of course).
     *
     * Remember, we run guest code at ring-1 and ring-2 levels, which are
     * considered supervisor levels by the paging  structures.  We run the VMM
     * in ring-0 with CR0.WP=0 and mapping all VMM structures as read-only
     * pages.  The following structures are exceptions and must be mapped with
     * write access so the ring-1 and ring-2 code can modify them.
     */
    int rc = PGMMapSetPage(pVM, pVM->patm.s.pGCStateGC, PAGE_SIZE, X86_PTE_P | X86_PTE_A | X86_PTE_D | X86_PTE_RW);
    AssertLogRelMsgReturn(RT_SUCCESS(rc), ("Failed to make the GCState accessible to ring-1 and ring-2 code: %Rrc\n", rc), rc);

    rc = PGMMapSetPage(pVM, pVM->patm.s.pGCStackGC, PATM_STACK_TOTAL_SIZE, X86_PTE_P | X86_PTE_A | X86_PTE_D | X86_PTE_RW);
    AssertLogRelMsgReturn(RT_SUCCESS(rc), ("Failed to make the GCStack accessible to ring-1 and ring-2 code: %Rrc\n", rc), rc);

    rc = PGMMapSetPage(pVM, pVM->patm.s.pStatsGC, PATM_STAT_MEMSIZE, X86_PTE_P | X86_PTE_A | X86_PTE_D | X86_PTE_RW);
    AssertLogRelMsgReturn(RT_SUCCESS(rc), ("Failed to make the stats struct accessible to ring-1 and ring-2 code: %Rrc\n", rc), rc);

    /*
     * Find the patch helper segment so we can identify code running there as patch code.
     */
    rc = PDMR3LdrGetSymbolRC(pVM, NULL, "g_PatchHlpBegin", &pVM->patm.s.pbPatchHelpersRC);
    AssertLogRelMsgReturn(RT_SUCCESS(rc), ("Failed to resolve g_PatchHlpBegin: %Rrc\n", rc), rc);
    pVM->patm.s.pbPatchHelpersR3 = (uint8_t *)MMHyperRCToR3(pVM, pVM->patm.s.pbPatchHelpersRC);
    AssertLogRelReturn(pVM->patm.s.pbPatchHelpersR3 != NULL, VERR_INTERNAL_ERROR_3);

    RTRCPTR RCPtrEnd;
    rc = PDMR3LdrGetSymbolRC(pVM, NULL, "g_PatchHlpEnd", &RCPtrEnd);
    AssertLogRelMsgReturn(RT_SUCCESS(rc), ("Failed to resolve g_PatchHlpEnd: %Rrc\n", rc), rc);

    pVM->patm.s.cbPatchHelpers = RCPtrEnd - pVM->patm.s.pbPatchHelpersRC;
    AssertLogRelMsgReturn(pVM->patm.s.cbPatchHelpers < _128K,
                          ("%RRv-%RRv => %#x\n", pVM->patm.s.pbPatchHelpersRC, RCPtrEnd, pVM->patm.s.cbPatchHelpers),
                          VERR_INTERNAL_ERROR_4);


    return VINF_SUCCESS;
}

/**
 * (Re)initializes PATM
 *
 * @param   pVM     The cross context VM structure.
 */
static int patmReinit(PVM pVM)
{
    int rc;

    /*
     * Assert alignment and sizes.
     */
    AssertRelease(!(RT_OFFSETOF(VM, patm.s) & 31));
    AssertRelease(sizeof(pVM->patm.s) <= sizeof(pVM->patm.padding));

    /*
     * Setup any fixed pointers and offsets.
     */
    pVM->patm.s.offVM = RT_OFFSETOF(VM, patm);

#ifndef RT_ARCH_AMD64 /* would be nice if this was changed everywhere. was driving me crazy on AMD64. */
#ifndef PATM_DISABLE_ALL
    pVM->fPATMEnabled = true;
#endif
#endif

    Assert(pVM->patm.s.pGCStateHC);
    memset(pVM->patm.s.pGCStateHC, 0, PAGE_SIZE);
    AssertReleaseMsg(pVM->patm.s.pGCStateGC, ("Impossible! MMHyperHC2GC(%p) failed!\n", pVM->patm.s.pGCStateGC));

    Log(("Patch memory allocated at %p - %RRv\n", pVM->patm.s.pPatchMemHC, pVM->patm.s.pPatchMemGC));
    pVM->patm.s.pGCStateHC->uVMFlags = X86_EFL_IF;

    Assert(pVM->patm.s.pGCStackHC);
    memset(pVM->patm.s.pGCStackHC, 0, PAGE_SIZE);
    AssertReleaseMsg(pVM->patm.s.pGCStackGC, ("Impossible! MMHyperHC2GC(%p) failed!\n", pVM->patm.s.pGCStackGC));
    pVM->patm.s.pGCStateHC->Psp = PATM_STACK_SIZE;
    pVM->patm.s.pGCStateHC->fPIF = 1;   /* PATM Interrupt Flag */

    Assert(pVM->patm.s.pStatsHC);
    memset(pVM->patm.s.pStatsHC, 0, PATM_STAT_MEMSIZE);
    AssertReleaseMsg(pVM->patm.s.pStatsGC, ("Impossible! MMHyperHC2GC(%p) failed!\n", pVM->patm.s.pStatsGC));

    Assert(pVM->patm.s.pPatchMemHC);
    Assert(pVM->patm.s.pPatchMemGC == MMHyperR3ToRC(pVM, pVM->patm.s.pPatchMemHC));
    memset(pVM->patm.s.pPatchMemHC, 0, PATCH_MEMORY_SIZE);
    AssertReleaseMsg(pVM->patm.s.pPatchMemGC, ("Impossible! MMHyperHC2GC(%p) failed!\n", pVM->patm.s.pPatchMemHC));

    /* Needed for future patching of sldt/sgdt/sidt/str etc. */
    pVM->patm.s.pCPUMCtxGC = VM_RC_ADDR(pVM, CPUMQueryGuestCtxPtr(VMMGetCpu(pVM)));

    Assert(pVM->patm.s.PatchLookupTreeHC);
    Assert(pVM->patm.s.PatchLookupTreeGC == MMHyperR3ToRC(pVM, pVM->patm.s.PatchLookupTreeHC));

    /*
     * (Re)Initialize PATM structure
     */
    Assert(!pVM->patm.s.PatchLookupTreeHC->PatchTree);
    Assert(!pVM->patm.s.PatchLookupTreeHC->PatchTreeByPatchAddr);
    Assert(!pVM->patm.s.PatchLookupTreeHC->PatchTreeByPage);
    pVM->patm.s.offPatchMem          = 16;  /* don't start with zero here */
    pVM->patm.s.uCurrentPatchIdx     = 1;   /* Index zero is a dummy */
    pVM->patm.s.pvFaultMonitor       = 0;
    pVM->patm.s.deltaReloc           = 0;

    /* Lowest and highest patched instruction */
    pVM->patm.s.pPatchedInstrGCLowest     = RTRCPTR_MAX;
    pVM->patm.s.pPatchedInstrGCHighest    = 0;

    pVM->patm.s.PatchLookupTreeHC->PatchTree            = 0;
    pVM->patm.s.PatchLookupTreeHC->PatchTreeByPatchAddr = 0;
    pVM->patm.s.PatchLookupTreeHC->PatchTreeByPage      = 0;

    pVM->patm.s.pfnSysEnterPatchGC = 0;
    pVM->patm.s.pfnSysEnterGC = 0;

    pVM->patm.s.fOutOfMemory = false;

    pVM->patm.s.pfnHelperCallGC = 0;
    patmR3DbgReset(pVM);

    /* Generate all global functions to be used by future patches. */
    /* We generate a fake patch in order to use the existing code for relocation. */
    rc = MMHyperAlloc(pVM, sizeof(PATMPATCHREC), 0, MM_TAG_PATM_PATCH, (void **)&pVM->patm.s.pGlobalPatchRec);
    if (RT_FAILURE(rc))
    {
        Log(("Out of memory!!!!\n"));
        return VERR_NO_MEMORY;
    }
    pVM->patm.s.pGlobalPatchRec->patch.flags             = PATMFL_GLOBAL_FUNCTIONS;
    pVM->patm.s.pGlobalPatchRec->patch.uState            = PATCH_ENABLED;
    pVM->patm.s.pGlobalPatchRec->patch.pPatchBlockOffset = pVM->patm.s.offPatchMem;

    rc = patmPatchGenGlobalFunctions(pVM, &pVM->patm.s.pGlobalPatchRec->patch);
    AssertRC(rc);

    /* Update free pointer in patch memory. */
    pVM->patm.s.offPatchMem += pVM->patm.s.pGlobalPatchRec->patch.uCurPatchOffset;
    /* Round to next 8 byte boundary. */
    pVM->patm.s.offPatchMem = RT_ALIGN_32(pVM->patm.s.offPatchMem, 8);


    return rc;
}


/**
 * Applies relocations to data and code managed by this
 * component. This function will be called at init and
 * whenever the VMM need to relocate it self inside the GC.
 *
 * The PATM will update the addresses used by the switcher.
 *
 * @param   pVM         The cross context VM structure.
 * @param   offDelta    The relocation delta.
 */
VMMR3_INT_DECL(void) PATMR3Relocate(PVM pVM, RTRCINTPTR offDelta)
{
    if (HMIsEnabled(pVM))
        return;

    RTRCPTR     GCPtrNew = MMHyperR3ToRC(pVM, pVM->patm.s.pGCStateHC);
    Assert((RTRCINTPTR)(GCPtrNew - pVM->patm.s.pGCStateGC) == offDelta);

    Log(("PATMR3Relocate from %RRv to %RRv - delta %08X\n", pVM->patm.s.pGCStateGC, GCPtrNew, offDelta));
    if (offDelta)
    {
        PCPUMCTX pCtx;

        /* Update CPUMCTX guest context pointer. */
        pVM->patm.s.pCPUMCtxGC += offDelta;

        pVM->patm.s.deltaReloc = offDelta;
        RTAvloU32DoWithAll(&pVM->patm.s.PatchLookupTreeHC->PatchTree, true, patmR3RelocatePatches, (void *)pVM);

        pVM->patm.s.pGCStateGC        = GCPtrNew;
        pVM->patm.s.pPatchMemGC       = MMHyperR3ToRC(pVM, pVM->patm.s.pPatchMemHC);
        pVM->patm.s.pGCStackGC        = MMHyperR3ToRC(pVM, pVM->patm.s.pGCStackHC);
        pVM->patm.s.pStatsGC          = MMHyperR3ToRC(pVM, pVM->patm.s.pStatsHC);
        pVM->patm.s.PatchLookupTreeGC = MMHyperR3ToRC(pVM, pVM->patm.s.PatchLookupTreeHC);

        if (pVM->patm.s.pfnSysEnterPatchGC)
            pVM->patm.s.pfnSysEnterPatchGC += offDelta;

        /* If we are running patch code right now, then also adjust EIP. */
        pCtx = CPUMQueryGuestCtxPtr(VMMGetCpu(pVM));
        if (PATMIsPatchGCAddr(pVM, pCtx->eip))
            pCtx->eip += offDelta;

        /* Deal with the global patch functions. */
        pVM->patm.s.pfnHelperCallGC += offDelta;
        pVM->patm.s.pfnHelperRetGC  += offDelta;
        pVM->patm.s.pfnHelperIretGC += offDelta;
        pVM->patm.s.pfnHelperJumpGC += offDelta;

        pVM->patm.s.pbPatchHelpersRC += offDelta;

        patmR3RelocatePatches(&pVM->patm.s.pGlobalPatchRec->Core, (void *)pVM);
    }
}


/**
 * Terminates the PATM.
 *
 * Termination means cleaning up and freeing all resources,
 * the VM it self is at this point powered off or suspended.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3_INT_DECL(int) PATMR3Term(PVM pVM)
{
    if (HMIsEnabled(pVM))
        return VINF_SUCCESS;

    patmR3DbgTerm(pVM);

    /* Memory was all allocated from the two MM heaps and requires no freeing. */
    return VINF_SUCCESS;
}


/**
 * PATM reset callback.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 */
VMMR3_INT_DECL(int) PATMR3Reset(PVM pVM)
{
    Log(("PATMR3Reset\n"));
    if (HMIsEnabled(pVM))
        return VINF_SUCCESS;

    /* Free all patches. */
    for (;;)
    {
        PPATMPATCHREC pPatchRec = (PPATMPATCHREC)RTAvloU32RemoveBestFit(&pVM->patm.s.PatchLookupTreeHC->PatchTree, 0, true);
        if (pPatchRec)
            patmR3RemovePatch(pVM, pPatchRec, true);
        else
            break;
    }
    Assert(!pVM->patm.s.PatchLookupTreeHC->PatchTreeByPage);
    Assert(!pVM->patm.s.PatchLookupTreeHC->PatchTree);
    pVM->patm.s.PatchLookupTreeHC->PatchTreeByPatchAddr = 0;
    pVM->patm.s.PatchLookupTreeHC->PatchTreeByPage      = 0;

    int rc = patmReinit(pVM);
    if (RT_SUCCESS(rc))
        rc = PATMR3InitFinalize(pVM); /* paranoia */

    return rc;
}

/**
 * @callback_method_impl{FNDISREADBYTES}
 */
static DECLCALLBACK(int) patmReadBytes(PDISCPUSTATE pDis, uint8_t offInstr, uint8_t cbMinRead, uint8_t cbMaxRead)
{
    PATMDISASM   *pDisInfo = (PATMDISASM *)pDis->pvUser;

/** @todo change this to read more! */
    /*
     * Trap/interrupt handler typically call common code on entry. Which might already have patches inserted.
     * As we currently don't support calling patch code from patch code, we'll let it read the original opcode bytes instead.
     */
    /** @todo could change in the future! */
    if (pDisInfo->fReadFlags & PATMREAD_ORGCODE)
    {
        size_t      cbRead   = cbMaxRead;
        RTUINTPTR   uSrcAddr = pDis->uInstrAddr + offInstr;
        int rc = PATMR3ReadOrgInstr(pDisInfo->pVM, pDis->uInstrAddr + offInstr, &pDis->abInstr[offInstr], cbRead, &cbRead);
        if (RT_SUCCESS(rc))
        {
            if (cbRead >= cbMinRead)
            {
                pDis->cbCachedInstr = offInstr + (uint8_t)cbRead;
                return VINF_SUCCESS;
            }

            cbMinRead -= (uint8_t)cbRead;
            cbMaxRead -= (uint8_t)cbRead;
            offInstr  += (uint8_t)cbRead;
            uSrcAddr  += cbRead;
        }

#ifdef VBOX_STRICT
        if (   !(pDisInfo->pPatchInfo->flags & (PATMFL_DUPLICATE_FUNCTION|PATMFL_IDTHANDLER))
            && !(pDisInfo->fReadFlags & PATMREAD_NOCHECK))
        {
            Assert(PATMR3IsInsidePatchJump(pDisInfo->pVM, pDis->uInstrAddr + offInstr, NULL) == false);
            Assert(PATMR3IsInsidePatchJump(pDisInfo->pVM, pDis->uInstrAddr + offInstr + cbMinRead-1, NULL) == false);
        }
#endif
    }

    int       rc       = VINF_SUCCESS;
    RTGCPTR32 uSrcAddr = (RTGCPTR32)pDis->uInstrAddr + offInstr;
    if (   !pDisInfo->pbInstrHC
        || (   PAGE_ADDRESS(pDisInfo->pInstrGC) != PAGE_ADDRESS(uSrcAddr + cbMinRead - 1)
            && !PATMIsPatchGCAddr(pDisInfo->pVM, uSrcAddr)))
    {
        Assert(!PATMIsPatchGCAddr(pDisInfo->pVM, uSrcAddr));
        rc = PGMPhysSimpleReadGCPtr(&pDisInfo->pVM->aCpus[0], &pDis->abInstr[offInstr], uSrcAddr, cbMinRead);
        offInstr += cbMinRead;
    }
    else
    {
        /*
         * pbInstrHC is the base address; adjust according to the GC pointer.
         *
         * Try read the max number of bytes here.  Since the disassembler only
         * ever uses these bytes for the current instruction, it doesn't matter
         * much if we accidentally read the start of the next instruction even
         * if it happens to be a patch jump or int3.
         */
        uint8_t const *pbInstrHC = pDisInfo->pbInstrHC; AssertPtr(pbInstrHC);
        pbInstrHC += uSrcAddr - pDisInfo->pInstrGC;

        size_t cbMaxRead1 = PAGE_SIZE - (uSrcAddr & PAGE_OFFSET_MASK);
        size_t cbMaxRead2 = PAGE_SIZE - ((uintptr_t)pbInstrHC & PAGE_OFFSET_MASK);
        size_t cbToRead   = RT_MIN(cbMaxRead1, RT_MAX(cbMaxRead2, cbMinRead));
        if (cbToRead > cbMaxRead)
            cbToRead = cbMaxRead;

        memcpy(&pDis->abInstr[offInstr], pbInstrHC, cbToRead);
        offInstr += (uint8_t)cbToRead;
    }

    pDis->cbCachedInstr = offInstr;
    return rc;
}


DECLINLINE(bool) patmR3DisInstrToStr(PVM pVM, PPATCHINFO pPatch, RTGCPTR32 InstrGCPtr32, uint8_t *pbInstrHC, uint32_t fReadFlags,
                                     PDISCPUSTATE pCpu, uint32_t *pcbInstr, char *pszOutput, size_t cbOutput)
{
    PATMDISASM disinfo;
    disinfo.pVM         = pVM;
    disinfo.pPatchInfo  = pPatch;
    disinfo.pbInstrHC   = pbInstrHC;
    disinfo.pInstrGC    = InstrGCPtr32;
    disinfo.fReadFlags  = fReadFlags;
    return RT_SUCCESS(DISInstrToStrWithReader(InstrGCPtr32,
                                              (pPatch->flags & PATMFL_CODE32) ? DISCPUMODE_32BIT : DISCPUMODE_16BIT,
                                              patmReadBytes, &disinfo,
                                              pCpu, pcbInstr, pszOutput, cbOutput));
}


DECLINLINE(bool) patmR3DisInstr(PVM pVM, PPATCHINFO pPatch, RTGCPTR32 InstrGCPtr32, uint8_t *pbInstrHC, uint32_t fReadFlags,
                                PDISCPUSTATE pCpu, uint32_t *pcbInstr)
{
    PATMDISASM disinfo;
    disinfo.pVM         = pVM;
    disinfo.pPatchInfo  = pPatch;
    disinfo.pbInstrHC   = pbInstrHC;
    disinfo.pInstrGC    = InstrGCPtr32;
    disinfo.fReadFlags  = fReadFlags;
    return RT_SUCCESS(DISInstrWithReader(InstrGCPtr32,
                                         (pPatch->flags & PATMFL_CODE32) ? DISCPUMODE_32BIT : DISCPUMODE_16BIT,
                                         patmReadBytes, &disinfo,
                                         pCpu, pcbInstr));
}


DECLINLINE(bool) patmR3DisInstrNoStrOpMode(PVM pVM, PPATCHINFO pPatch, RTGCPTR32 InstrGCPtr32, uint8_t *pbInstrHC,
                                           uint32_t fReadFlags,
                                           PDISCPUSTATE pCpu, uint32_t *pcbInstr)
{
    PATMDISASM disinfo;
    disinfo.pVM         = pVM;
    disinfo.pPatchInfo  = pPatch;
    disinfo.pbInstrHC   = pbInstrHC;
    disinfo.pInstrGC    = InstrGCPtr32;
    disinfo.fReadFlags  = fReadFlags;
    return RT_SUCCESS(DISInstrWithReader(InstrGCPtr32, pPatch->uOpMode, patmReadBytes, &disinfo,
                                         pCpu, pcbInstr));
}

#ifdef LOG_ENABLED
# define PATM_LOG_ORG_PATCH_INSTR(a_pVM, a_pPatch, a_szComment) \
    PATM_LOG_PATCH_INSTR(a_pVM, a_pPatch, PATMREAD_ORGCODE, a_szComment, " patch:")
# define PATM_LOG_RAW_PATCH_INSTR(a_pVM, a_pPatch, a_szComment) \
    PATM_LOG_PATCH_INSTR(a_pVM, a_pPatch, PATMREAD_RAWCODE, a_szComment, " patch:")

# define PATM_LOG_PATCH_INSTR(a_pVM, a_pPatch, a_fFlags, a_szComment1, a_szComment2) \
    do { \
        if (LogIsEnabled()) \
            patmLogRawPatchInstr(a_pVM, a_pPatch, a_fFlags, a_szComment1, a_szComment2); \
    } while (0)

static void patmLogRawPatchInstr(PVM pVM, PPATCHINFO pPatch, uint32_t fFlags,
                                 const char *pszComment1, const char *pszComment2)
{
    DISCPUSTATE DisState;
    char szOutput[128];
    szOutput[0] = '\0';
    patmR3DisInstrToStr(pVM, pPatch, pPatch->pPrivInstrGC, NULL, fFlags,
                        &DisState, NULL, szOutput, sizeof(szOutput));
    Log(("%s%s %s", pszComment1, pszComment2, szOutput));
}

#else
# define PATM_LOG_ORG_PATCH_INSTR(a_pVM, a_pPatch, a_szComment)                         do { } while (0)
# define PATM_LOG_RAW_PATCH_INSTR(a_pVM, a_pPatch, a_szComment)                         do { } while (0)
# define PATM_LOG_PATCH_INSTR(a_pVM, a_pPatch, a_fFlags, a_szComment1, a_szComment2)    do { } while (0)
#endif


/**
 * Callback function for RTAvloU32DoWithAll
 *
 * Updates all fixups in the patches
 *
 * @returns VBox status code.
 * @param   pNode       Current node
 * @param   pParam      Pointer to the VM.
 */
static DECLCALLBACK(int) patmR3RelocatePatches(PAVLOU32NODECORE pNode, void *pParam)
{
    PPATMPATCHREC   pPatch = (PPATMPATCHREC)pNode;
    PVM             pVM = (PVM)pParam;
    RTRCINTPTR      delta;
    int             rc;

    /* Nothing to do if the patch is not active. */
    if (pPatch->patch.uState == PATCH_REFUSED)
        return 0;

    if (pPatch->patch.flags & PATMFL_PATCHED_GUEST_CODE)
        PATM_LOG_PATCH_INSTR(pVM, &pPatch->patch, PATMREAD_RAWCODE, "Org patch jump:", "");

    Log(("Nr of fixups %d\n", pPatch->patch.nrFixups));
    delta = (RTRCINTPTR)pVM->patm.s.deltaReloc;

    /*
     * Apply fixups.
     */
    AVLPVKEY key = NULL;
    for (;;)
    {
        /* Get the record that's closest from above (after or equal to key). */
        PRELOCREC pRec = (PRELOCREC)RTAvlPVGetBestFit(&pPatch->patch.FixupTree, key, true);
        if (!pRec)
            break;

        key = (uint8_t *)pRec->Core.Key + 1;   /* search for the next record during the next round. */

        switch (pRec->uType)
        {
        case FIXUP_ABSOLUTE_IN_PATCH_ASM_TMPL:
            Assert(pRec->pDest == pRec->pSource); Assert(PATM_IS_ASMFIX(pRec->pSource));
            Log(("Absolute patch template fixup type %#x at %RHv -> %RHv at %RRv\n", pRec->pSource, *(RTRCUINTPTR *)pRec->pRelocPos, *(RTRCINTPTR*)pRec->pRelocPos + delta, pRec->pRelocPos));
            *(RTRCUINTPTR *)pRec->pRelocPos += delta;
            break;

        case FIXUP_ABSOLUTE:
            Log(("Absolute fixup at %RRv %RHv -> %RHv at %RRv\n", pRec->pSource, *(RTRCUINTPTR *)pRec->pRelocPos, *(RTRCINTPTR*)pRec->pRelocPos + delta, pRec->pRelocPos));
            if (    !pRec->pSource
                ||  PATMIsPatchGCAddr(pVM, pRec->pSource))
            {
                *(RTRCUINTPTR *)pRec->pRelocPos += delta;
            }
            else
            {
                uint8_t curInstr[15];
                uint8_t oldInstr[15];
                Assert(pRec->pSource && pPatch->patch.cbPrivInstr <= 15);

                Assert(!(pPatch->patch.flags & PATMFL_GLOBAL_FUNCTIONS));

                memcpy(oldInstr, pPatch->patch.aPrivInstr, pPatch->patch.cbPrivInstr);
                *(RTRCPTR *)&oldInstr[pPatch->patch.cbPrivInstr - sizeof(RTRCPTR)] = pRec->pDest;

                rc = PGMPhysSimpleReadGCPtr(VMMGetCpu0(pVM), curInstr, pPatch->patch.pPrivInstrGC, pPatch->patch.cbPrivInstr);
                Assert(RT_SUCCESS(rc) || rc == VERR_PAGE_NOT_PRESENT || rc == VERR_PAGE_TABLE_NOT_PRESENT);

                pRec->pDest = (RTRCPTR)((RTRCUINTPTR)pRec->pDest + delta);

                if (    rc == VERR_PAGE_NOT_PRESENT
                    ||  rc == VERR_PAGE_TABLE_NOT_PRESENT)
                {
                    RTRCPTR pPage = pPatch->patch.pPrivInstrGC & PAGE_BASE_GC_MASK;

                    Log(("PATM: Patch page not present -> check later!\n"));
                    rc = PGMR3HandlerVirtualRegister(pVM, VMMGetCpu(pVM), pVM->patm.s.hMonitorPageType,
                                                     pPage,
                                                     pPage + (PAGE_SIZE - 1) /* inclusive! */,
                                                     (void *)(uintptr_t)pPage, pPage, NULL /*pszDesc*/);
                    Assert(RT_SUCCESS(rc) || rc == VERR_PGM_HANDLER_VIRTUAL_CONFLICT);
                }
                else
                if (memcmp(curInstr, oldInstr, pPatch->patch.cbPrivInstr))
                {
                    Log(("PATM: Patch was overwritten -> disabling patch!!\n"));
                    /*
                     * Disable patch; this is not a good solution
                     */
                     /** @todo hopefully it was completely overwritten (if the read was successful)!!!! */
                    pPatch->patch.uState = PATCH_DISABLED;
                }
                else
                if (RT_SUCCESS(rc))
                {
                    *(RTRCPTR *)&curInstr[pPatch->patch.cbPrivInstr - sizeof(RTRCPTR)] = pRec->pDest;
                    rc = PGMPhysSimpleDirtyWriteGCPtr(VMMGetCpu0(pVM), pRec->pSource, curInstr, pPatch->patch.cbPrivInstr);
                    AssertRC(rc);
                }
            }
            break;

        case FIXUP_REL_JMPTOPATCH:
        {
            RTRCPTR pTarget = (RTRCPTR)((RTRCINTPTR)pRec->pDest + delta);

            if (    pPatch->patch.uState == PATCH_ENABLED
                &&  (pPatch->patch.flags & PATMFL_PATCHED_GUEST_CODE))
            {
                uint8_t    oldJump[SIZEOF_NEAR_COND_JUMP32];
                uint8_t    temp[SIZEOF_NEAR_COND_JUMP32];
                RTRCPTR    pJumpOffGC;
                RTRCINTPTR displ   = (RTRCINTPTR)pTarget - (RTRCINTPTR)pRec->pSource;
                RTRCINTPTR displOld= (RTRCINTPTR)pRec->pDest - (RTRCINTPTR)pRec->pSource;

#if 0 /** @todo '*(int32_t*)pRec->pRelocPos' crashes on restore of an XP VM here. pRelocPos=0x8000dbe2180a (bird) */
                Log(("Relative fixup (g2p) %08X -> %08X at %08X (source=%08x, target=%08x)\n", *(int32_t*)pRec->pRelocPos, displ, pRec->pRelocPos, pRec->pSource, pRec->pDest));
#else
                Log(("Relative fixup (g2p) ???????? -> %08X at %08X (source=%08x, target=%08x)\n", displ, pRec->pRelocPos, pRec->pSource, pRec->pDest));
#endif

                Assert(pRec->pSource - pPatch->patch.cbPatchJump == pPatch->patch.pPrivInstrGC);
#ifdef PATM_RESOLVE_CONFLICTS_WITH_JUMP_PATCHES
                if (pPatch->patch.cbPatchJump == SIZEOF_NEAR_COND_JUMP32)
                {
                    Assert(pPatch->patch.flags & PATMFL_JUMP_CONFLICT);

                    pJumpOffGC = pPatch->patch.pPrivInstrGC + 2;    //two byte opcode
                    oldJump[0] = pPatch->patch.aPrivInstr[0];
                    oldJump[1] = pPatch->patch.aPrivInstr[1];
                    *(RTRCUINTPTR *)&oldJump[2] = displOld;
                }
                else
#endif
                if (pPatch->patch.cbPatchJump == SIZEOF_NEARJUMP32)
                {
                    pJumpOffGC = pPatch->patch.pPrivInstrGC + 1;    //one byte opcode
                    oldJump[0] = 0xE9;
                    *(RTRCUINTPTR *)&oldJump[1] = displOld;
                }
                else
                {
                    AssertMsgFailed(("Invalid patch jump size %d\n", pPatch->patch.cbPatchJump));
                    continue;   //this should never happen!!
                }
                Assert(pPatch->patch.cbPatchJump <= sizeof(temp));

                /*
                 * Read old patch jump and compare it to the one we previously installed
                 */
                rc = PGMPhysSimpleReadGCPtr(VMMGetCpu0(pVM), temp, pPatch->patch.pPrivInstrGC, pPatch->patch.cbPatchJump);
                Assert(RT_SUCCESS(rc) || rc == VERR_PAGE_NOT_PRESENT || rc == VERR_PAGE_TABLE_NOT_PRESENT);

                if (    rc == VERR_PAGE_NOT_PRESENT
                    ||  rc == VERR_PAGE_TABLE_NOT_PRESENT)
                {
                    RTRCPTR pPage = pPatch->patch.pPrivInstrGC & PAGE_BASE_GC_MASK;
                    Log(("PATM: Patch page not present -> check later!\n"));
                    rc = PGMR3HandlerVirtualRegister(pVM, VMMGetCpu(pVM), pVM->patm.s.hMonitorPageType,
                                                     pPage,
                                                     pPage + (PAGE_SIZE - 1) /* inclusive! */,
                                                     (void *)(uintptr_t)pPage, pPage, NULL /*pszDesc*/);
                    Assert(RT_SUCCESS(rc) || rc == VERR_PGM_HANDLER_VIRTUAL_CONFLICT);
                }
                else
                if (memcmp(temp, oldJump, pPatch->patch.cbPatchJump))
                {
                    Log(("PATM: Patch jump was overwritten -> disabling patch!!\n"));
                    /*
                     * Disable patch; this is not a good solution
                     */
                     /** @todo hopefully it was completely overwritten (if the read was successful)!!!! */
                    pPatch->patch.uState = PATCH_DISABLED;
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
                Log(("Skip the guest jump to patch code for this disabled patch %RGv - %08X\n", pPatch->patch.pPrivInstrGC, pRec->pRelocPos));

            pRec->pDest = pTarget;
            break;
        }

        case FIXUP_REL_JMPTOGUEST:
        {
            RTRCPTR    pSource = (RTRCPTR)((RTRCINTPTR)pRec->pSource + delta);
            RTRCINTPTR displ   = (RTRCINTPTR)pRec->pDest - (RTRCINTPTR)pSource;

            Assert(!(pPatch->patch.flags & PATMFL_GLOBAL_FUNCTIONS));
            Log(("Relative fixup (p2g) %08X -> %08X at %08X (source=%08x, target=%08x)\n", *(int32_t*)pRec->pRelocPos, displ, pRec->pRelocPos, pRec->pSource, pRec->pDest));
            *(RTRCUINTPTR *)pRec->pRelocPos = displ;
            pRec->pSource = pSource;
            break;
        }

        case FIXUP_REL_HELPER_IN_PATCH_ASM_TMPL:
        case FIXUP_CONSTANT_IN_PATCH_ASM_TMPL:
            /* Only applicable when loading state. */
            Assert(pRec->pDest == pRec->pSource);
            Assert(PATM_IS_ASMFIX(pRec->pSource));
            break;

        default:
            AssertMsg(0, ("Invalid fixup type!!\n"));
            return VERR_INVALID_PARAMETER;
        }
    }

    if (pPatch->patch.flags & PATMFL_PATCHED_GUEST_CODE)
        PATM_LOG_PATCH_INSTR(pVM, &pPatch->patch, PATMREAD_RAWCODE, "Rel patch jump:", "");
    return 0;
}

#ifdef VBOX_WITH_DEBUGGER

/**
 * Callback function for RTAvloU32DoWithAll
 *
 * Enables the patch that's being enumerated
 *
 * @returns 0 (continue enumeration).
 * @param   pNode       Current node
 * @param   pVM         The cross context VM structure.
 */
static DECLCALLBACK(int) EnableAllPatches(PAVLOU32NODECORE pNode, void *pVM)
{
    PPATMPATCHREC pPatch = (PPATMPATCHREC)pNode;

    PATMR3EnablePatch((PVM)pVM, (RTRCPTR)pPatch->Core.Key);
    return 0;
}


/**
 * Callback function for RTAvloU32DoWithAll
 *
 * Disables the patch that's being enumerated
 *
 * @returns 0 (continue enumeration).
 * @param   pNode       Current node
 * @param   pVM         The cross context VM structure.
 */
static DECLCALLBACK(int) DisableAllPatches(PAVLOU32NODECORE pNode, void *pVM)
{
    PPATMPATCHREC pPatch = (PPATMPATCHREC)pNode;

    PATMR3DisablePatch((PVM)pVM, (RTRCPTR)pPatch->Core.Key);
    return 0;
}

#endif /* VBOX_WITH_DEBUGGER */

/**
 * Returns the host context pointer of the GC context structure
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3_INT_DECL(PPATMGCSTATE) PATMR3QueryGCStateHC(PVM pVM)
{
    AssertReturn(!HMIsEnabled(pVM), NULL);
    return pVM->patm.s.pGCStateHC;
}


/**
 * Allows or disallow patching of privileged instructions executed by the guest OS
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   fAllowPatching  Allow/disallow patching
 */
VMMR3DECL(int) PATMR3AllowPatching(PUVM pUVM, bool fAllowPatching)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);

    if (!HMIsEnabled(pVM))
        pVM->fPATMEnabled = fAllowPatching;
    else
        Assert(!pVM->fPATMEnabled);
    return VINF_SUCCESS;
}


/**
 * Checks if the patch manager is enabled or not.
 *
 * @returns true if enabled, false if not (or if invalid handle).
 * @param   pUVM        The user mode VM handle.
 */
VMMR3DECL(bool) PATMR3IsEnabled(PUVM pUVM)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, false);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, false);
    return PATMIsEnabled(pVM);
}


/**
 * Convert a GC patch block pointer to a HC patch pointer
 *
 * @returns HC pointer or NULL if it's not a GC patch pointer
 * @param   pVM         The cross context VM structure.
 * @param   pAddrGC     GC pointer
 */
VMMR3_INT_DECL(void *) PATMR3GCPtrToHCPtr(PVM pVM, RTRCPTR pAddrGC)
{
    AssertReturn(!HMIsEnabled(pVM), NULL);
    RTRCUINTPTR offPatch = (RTRCUINTPTR)pAddrGC - (RTRCUINTPTR)pVM->patm.s.pPatchMemGC;
    if (offPatch >= pVM->patm.s.cbPatchMem)
    {
        offPatch = (RTRCUINTPTR)pAddrGC - (RTRCUINTPTR)pVM->patm.s.pbPatchHelpersRC;
        if (offPatch >= pVM->patm.s.cbPatchHelpers)
            return NULL;
        return pVM->patm.s.pbPatchHelpersR3 + offPatch;
    }
    return pVM->patm.s.pPatchMemHC + offPatch;
}


/**
 * Convert guest context address to host context pointer
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pCacheRec   Address conversion cache record
 * @param   pGCPtr      Guest context pointer
 *
 * @returns             Host context pointer or NULL in case of an error
 *
 */
R3PTRTYPE(uint8_t *) patmR3GCVirtToHCVirt(PVM pVM, PPATMP2GLOOKUPREC pCacheRec, RCPTRTYPE(uint8_t *) pGCPtr)
{
    int rc;
    R3PTRTYPE(uint8_t *) pHCPtr;
    uint32_t offset;

    offset = (RTRCUINTPTR)pGCPtr - (RTRCUINTPTR)pVM->patm.s.pPatchMemGC;
    if (offset < pVM->patm.s.cbPatchMem)
    {
#ifdef VBOX_STRICT
        PPATCHINFO pPatch = (PPATCHINFO)pCacheRec->pPatch;
        Assert(pPatch); Assert(offset - pPatch->pPatchBlockOffset < pPatch->cbPatchBlockSize);
#endif
        return pVM->patm.s.pPatchMemHC + offset;
    }
    /* Note! We're _not_ including the patch helpers here. */

    offset = pGCPtr & PAGE_OFFSET_MASK;
    if (pCacheRec->pGuestLoc == (pGCPtr & PAGE_BASE_GC_MASK))
        return pCacheRec->pPageLocStartHC + offset;

    /* Release previous lock if any. */
    if (pCacheRec->Lock.pvMap)
    {
        PGMPhysReleasePageMappingLock(pVM, &pCacheRec->Lock);
        pCacheRec->Lock.pvMap = NULL;
    }

    rc = PGMPhysGCPtr2CCPtrReadOnly(VMMGetCpu(pVM), pGCPtr, (const void **)&pHCPtr, &pCacheRec->Lock);
    if (rc != VINF_SUCCESS)
    {
        AssertMsg(rc == VINF_SUCCESS || rc == VERR_PAGE_NOT_PRESENT || rc == VERR_PAGE_TABLE_NOT_PRESENT, ("MMR3PhysGCVirt2HCVirtEx failed for %08X\n", pGCPtr));
        return NULL;
    }
    pCacheRec->pPageLocStartHC = (R3PTRTYPE(uint8_t*))((RTHCUINTPTR)pHCPtr & PAGE_BASE_HC_MASK);
    pCacheRec->pGuestLoc        = pGCPtr & PAGE_BASE_GC_MASK;
    return pHCPtr;
}


/**
 * Calculates and fills in all branch targets
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Current patch block pointer
 *
 */
static int patmr3SetBranchTargets(PVM pVM, PPATCHINFO pPatch)
{
    int32_t displ;

    PJUMPREC pRec = 0;
    unsigned nrJumpRecs = 0;

    /*
     * Set all branch targets inside the patch block.
     * We remove all jump records as they are no longer needed afterwards.
     */
    while (true)
    {
        RCPTRTYPE(uint8_t *) pInstrGC;
        RCPTRTYPE(uint8_t *) pBranchTargetGC = 0;

        pRec = (PJUMPREC)RTAvlPVRemoveBestFit(&pPatch->JumpTree, 0, true);
        if (pRec == 0)
            break;

        nrJumpRecs++;

        /* HC in patch block to GC in patch block. */
        pInstrGC = patmPatchHCPtr2PatchGCPtr(pVM, pRec->pJumpHC);

        if (pRec->opcode == OP_CALL)
        {
            /* Special case: call function replacement patch from this patch block.
             */
            PPATMPATCHREC pFunctionRec = patmQueryFunctionPatch(pVM, pRec->pTargetGC);
            if (!pFunctionRec)
            {
                int rc;

                if (PATMR3HasBeenPatched(pVM, pRec->pTargetGC) == false)
                    rc = PATMR3InstallPatch(pVM, pRec->pTargetGC, PATMFL_CODE32 | PATMFL_DUPLICATE_FUNCTION);
                else
                    rc = VERR_PATCHING_REFUSED;    /* exists as a normal patch; can't use it */

                if (RT_FAILURE(rc))
                {
                    uint8_t *pPatchHC;
                    RTRCPTR  pPatchGC;
                    RTRCPTR  pOrgInstrGC;

                    pOrgInstrGC = PATMR3PatchToGCPtr(pVM, pInstrGC, 0);
                    Assert(pOrgInstrGC);

                    /* Failure for some reason -> mark exit point with int 3. */
                    Log(("Failed to install function replacement patch (at %x) for reason %Rrc\n", pOrgInstrGC, rc));

                    pPatchGC = patmGuestGCPtrToPatchGCPtr(pVM, pPatch, pOrgInstrGC);
                    Assert(pPatchGC);

                    pPatchHC = pVM->patm.s.pPatchMemHC + (pPatchGC - pVM->patm.s.pPatchMemGC);

                    /* Set a breakpoint at the very beginning of the recompiled instruction */
                    *pPatchHC = 0xCC;

                    continue;
                }
            }
            else
            {
                Log(("Patch block %RRv called as function\n", pFunctionRec->patch.pPrivInstrGC));
                pFunctionRec->patch.flags |= PATMFL_CODE_REFERENCED;
            }

            pBranchTargetGC = PATMR3QueryPatchGCPtr(pVM, pRec->pTargetGC);
        }
        else
            pBranchTargetGC = patmGuestGCPtrToPatchGCPtr(pVM, pPatch, pRec->pTargetGC);

        if (pBranchTargetGC == 0)
        {
            AssertMsgFailed(("patmr3SetBranchTargets: patmGuestGCPtrToPatchGCPtr failed for %08X\n", pRec->pTargetGC));
            return VERR_PATCHING_REFUSED;
        }
        /* Our jumps *always* have a dword displacement (to make things easier). */
        Assert(sizeof(uint32_t) == sizeof(RTRCPTR));
        displ =  pBranchTargetGC - (pInstrGC + pRec->offDispl + sizeof(RTRCPTR));
        *(RTRCPTR *)(pRec->pJumpHC + pRec->offDispl) = displ;
        Log(("Set branch target %d to %08X : %08x - (%08x + %d + %d)\n", nrJumpRecs, displ, pBranchTargetGC, pInstrGC, pRec->offDispl, sizeof(RTRCPTR)));
    }
    Assert(nrJumpRecs == pPatch->nrJumpRecs);
    Assert(pPatch->JumpTree == 0);
    return VINF_SUCCESS;
}

/**
 * Add an illegal instruction record
 *
 * @param   pVM             The cross context VM structure.
 * @param   pPatch          Patch structure ptr
 * @param   pInstrGC        Guest context pointer to privileged instruction
 *
 */
static void patmAddIllegalInstrRecord(PVM pVM, PPATCHINFO pPatch, RTRCPTR pInstrGC)
{
    PAVLPVNODECORE pRec;

    pRec = (PAVLPVNODECORE)MMR3HeapAllocZ(pVM, MM_TAG_PATM_PATCH, sizeof(*pRec));
    Assert(pRec);
    pRec->Key = (AVLPVKEY)(uintptr_t)pInstrGC;

    bool ret = RTAvlPVInsert(&pPatch->pTempInfo->IllegalInstrTree, pRec);
    Assert(ret); NOREF(ret);
    pPatch->pTempInfo->nrIllegalInstr++;
}

static bool patmIsIllegalInstr(PPATCHINFO pPatch, RTRCPTR pInstrGC)
{
    PAVLPVNODECORE pRec;

    pRec = RTAvlPVGet(&pPatch->pTempInfo->IllegalInstrTree, (AVLPVKEY)(uintptr_t)pInstrGC);
    if (pRec)
        return true;
    else
        return false;
}

/**
 * Add a patch to guest lookup record
 *
 * @param   pVM             The cross context VM structure.
 * @param   pPatch          Patch structure ptr
 * @param   pPatchInstrHC   Guest context pointer to patch block
 * @param   pInstrGC        Guest context pointer to privileged instruction
 * @param   enmType         Lookup type
 * @param   fDirty          Dirty flag
 *
 * @note Be extremely careful with this function. Make absolutely sure the guest
 *       address is correct! (to avoid executing instructions twice!)
 */
void patmR3AddP2GLookupRecord(PVM pVM, PPATCHINFO pPatch, uint8_t *pPatchInstrHC, RTRCPTR pInstrGC, PATM_LOOKUP_TYPE enmType, bool fDirty)
{
    bool ret;
    PRECPATCHTOGUEST pPatchToGuestRec;
    PRECGUESTTOPATCH pGuestToPatchRec;
    uint32_t PatchOffset = pPatchInstrHC - pVM->patm.s.pPatchMemHC;  /* Offset in memory reserved for PATM. */

    LogFlowFunc(("pVM=%#p pPatch=%#p pPatchInstrHC=%#p pInstrGC=%#x enmType=%d fDirty=%RTbool\n",
                 pVM, pPatch, pPatchInstrHC, pInstrGC, enmType, fDirty));

    if (enmType == PATM_LOOKUP_PATCH2GUEST)
    {
        pPatchToGuestRec = (PRECPATCHTOGUEST)RTAvlU32Get(&pPatch->Patch2GuestAddrTree, PatchOffset);
        if (pPatchToGuestRec && pPatchToGuestRec->Core.Key == PatchOffset)
            return; /* already there */

        Assert(!pPatchToGuestRec);
    }
#ifdef VBOX_STRICT
    else
    {
        pPatchToGuestRec = (PRECPATCHTOGUEST)RTAvlU32Get(&pPatch->Patch2GuestAddrTree, PatchOffset);
        Assert(!pPatchToGuestRec);
    }
#endif

    pPatchToGuestRec = (PRECPATCHTOGUEST)MMR3HeapAllocZ(pVM, MM_TAG_PATM_PATCH, sizeof(RECPATCHTOGUEST) + sizeof(RECGUESTTOPATCH));
    Assert(pPatchToGuestRec);
    pPatchToGuestRec->Core.Key    = PatchOffset;
    pPatchToGuestRec->pOrgInstrGC = pInstrGC;
    pPatchToGuestRec->enmType     = enmType;
    pPatchToGuestRec->fDirty     = fDirty;

    ret = RTAvlU32Insert(&pPatch->Patch2GuestAddrTree, &pPatchToGuestRec->Core);
    Assert(ret);

    /* GC to patch address */
    if (enmType == PATM_LOOKUP_BOTHDIR)
    {
        pGuestToPatchRec = (PRECGUESTTOPATCH)RTAvlU32Get(&pPatch->Guest2PatchAddrTree, pInstrGC);
        if (!pGuestToPatchRec)
        {
            pGuestToPatchRec = (PRECGUESTTOPATCH)(pPatchToGuestRec+1);
            pGuestToPatchRec->Core.Key    = pInstrGC;
            pGuestToPatchRec->PatchOffset = PatchOffset;

            ret = RTAvlU32Insert(&pPatch->Guest2PatchAddrTree, &pGuestToPatchRec->Core);
            Assert(ret);
        }
    }

    pPatch->nrPatch2GuestRecs++;
}


/**
 * Removes a patch to guest lookup record
 *
 * @param   pVM             The cross context VM structure.
 * @param   pPatch          Patch structure ptr
 * @param   pPatchInstrGC   Guest context pointer to patch block
 */
void patmr3RemoveP2GLookupRecord(PVM pVM, PPATCHINFO pPatch, RTRCPTR pPatchInstrGC)
{
    PAVLU32NODECORE     pNode;
    PAVLU32NODECORE     pNode2;
    PRECPATCHTOGUEST    pPatchToGuestRec;
    uint32_t            PatchOffset = pPatchInstrGC - pVM->patm.s.pPatchMemGC;  /* Offset in memory reserved for PATM. */

    pPatchToGuestRec = (PRECPATCHTOGUEST)RTAvlU32Get(&pPatch->Patch2GuestAddrTree, PatchOffset);
    Assert(pPatchToGuestRec);
    if (pPatchToGuestRec)
    {
        if (pPatchToGuestRec->enmType == PATM_LOOKUP_BOTHDIR)
        {
            PRECGUESTTOPATCH pGuestToPatchRec = (PRECGUESTTOPATCH)(pPatchToGuestRec+1);

            Assert(pGuestToPatchRec->Core.Key);
            pNode2 = RTAvlU32Remove(&pPatch->Guest2PatchAddrTree, pGuestToPatchRec->Core.Key);
            Assert(pNode2);
        }
        pNode = RTAvlU32Remove(&pPatch->Patch2GuestAddrTree, pPatchToGuestRec->Core.Key);
        Assert(pNode);

        MMR3HeapFree(pPatchToGuestRec);
        pPatch->nrPatch2GuestRecs--;
    }
}


/**
 * RTAvlPVDestroy callback.
 */
static DECLCALLBACK(int) patmEmptyTreePVCallback(PAVLPVNODECORE pNode, void *)
{
    MMR3HeapFree(pNode);
    return 0;
}

/**
 * Empty the specified tree (PV tree, MMR3 heap)
 *
 * @param   pVM             The cross context VM structure.
 * @param   ppTree          Tree to empty
 */
static void patmEmptyTree(PVM pVM, PAVLPVNODECORE *ppTree)
{
    NOREF(pVM);
    RTAvlPVDestroy(ppTree, patmEmptyTreePVCallback, NULL);
}


/**
 * RTAvlU32Destroy callback.
 */
static DECLCALLBACK(int) patmEmptyTreeU32Callback(PAVLU32NODECORE pNode, void *)
{
    MMR3HeapFree(pNode);
    return 0;
}

/**
 * Empty the specified tree (U32 tree, MMR3 heap)
 *
 * @param   pVM             The cross context VM structure.
 * @param   ppTree          Tree to empty
 */
static void patmEmptyTreeU32(PVM pVM, PPAVLU32NODECORE ppTree)
{
    NOREF(pVM);
    RTAvlU32Destroy(ppTree, patmEmptyTreeU32Callback, NULL);
}


/**
 * Analyses the instructions following the cli for compliance with our heuristics for cli & pushf
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pCpu        CPU disassembly state
 * @param   pInstrGC    Guest context pointer to privileged instruction
 * @param   pCurInstrGC Guest context pointer to the current instruction
 * @param   pCacheRec   Cache record ptr
 *
 */
static int patmAnalyseBlockCallback(PVM pVM, DISCPUSTATE *pCpu, RCPTRTYPE(uint8_t *) pInstrGC, RCPTRTYPE(uint8_t *) pCurInstrGC, PPATMP2GLOOKUPREC pCacheRec)
{
    PPATCHINFO pPatch = (PPATCHINFO)pCacheRec->pPatch;
    bool       fIllegalInstr = false;

    /*
     *  Preliminary heuristics:
     *- no call instructions without a fixed displacement between cli and sti/popf
     *- no jumps in the instructions following cli (4+ bytes; enough for the replacement jump (5 bytes))
     *- no nested pushf/cli
     *- sti/popf should be the (eventual) target of all branches
     *- no near or far returns; no int xx, no into
     *
     * Note: Later on we can impose less stricter guidelines if the need arises
     */

    /* Bail out if the patch gets too big. */
    if (pPatch->cbPatchBlockSize >= MAX_PATCH_SIZE)
    {
        Log(("Code block too big (%x) for patch at %RRv!!\n", pPatch->cbPatchBlockSize, pCurInstrGC));
        fIllegalInstr = true;
        patmAddIllegalInstrRecord(pVM, pPatch, pCurInstrGC);
    }
    else
    {
        /* No unconditional jumps or calls without fixed displacements. */
        if (    (pCpu->pCurInstr->fOpType & DISOPTYPE_CONTROLFLOW)
             && (pCpu->pCurInstr->uOpcode == OP_JMP || pCpu->pCurInstr->uOpcode == OP_CALL)
           )
        {
            Assert(pCpu->Param1.cb <= 4 || pCpu->Param1.cb == 6);
            if (    pCpu->Param1.cb == 6 /* far call/jmp */
                ||  (pCpu->pCurInstr->uOpcode == OP_CALL && !(pPatch->flags & PATMFL_SUPPORT_CALLS))
                ||  (OP_PARM_VTYPE(pCpu->pCurInstr->fParam1) != OP_PARM_J && !(pPatch->flags & PATMFL_SUPPORT_INDIRECT_CALLS))
               )
            {
                fIllegalInstr = true;
                patmAddIllegalInstrRecord(pVM, pPatch, pCurInstrGC);
            }
        }

        /* An unconditional (short) jump right after a cli is a potential problem; we will overwrite whichever function comes afterwards */
        if (pPatch->opcode == OP_CLI && pCpu->pCurInstr->uOpcode == OP_JMP)
        {
            if (   pCurInstrGC > pPatch->pPrivInstrGC
                && pCurInstrGC + pCpu->cbInstr < pPatch->pPrivInstrGC + SIZEOF_NEARJUMP32) /* hardcoded patch jump size; cbPatchJump is still zero */
            {
                Log(("Dangerous unconditional jump ends in our generated patch jump!! (%x vs %x)\n", pCurInstrGC, pPatch->pPrivInstrGC));
                /* We turn this one into a int 3 callable patch. */
                pPatch->flags |= PATMFL_INT3_REPLACEMENT_BLOCK;
            }
        }
        else
        /* no nested pushfs just yet; nested cli is allowed for cli patches though. */
        if (pPatch->opcode == OP_PUSHF)
        {
            if (pCurInstrGC != pInstrGC && pCpu->pCurInstr->uOpcode == OP_PUSHF)
            {
                fIllegalInstr = true;
                patmAddIllegalInstrRecord(pVM, pPatch, pCurInstrGC);
            }
        }

        /* no far returns */
        if (pCpu->pCurInstr->uOpcode == OP_RETF)
        {
            pPatch->pTempInfo->nrRetInstr++;
            fIllegalInstr = true;
            patmAddIllegalInstrRecord(pVM, pPatch, pCurInstrGC);
        }
        else if (   pCpu->pCurInstr->uOpcode == OP_INT3
                 || pCpu->pCurInstr->uOpcode == OP_INT
                 || pCpu->pCurInstr->uOpcode == OP_INTO)
        {
            /* No int xx or into either. */
            fIllegalInstr = true;
            patmAddIllegalInstrRecord(pVM, pPatch, pCurInstrGC);
        }
    }

    pPatch->cbPatchBlockSize += pCpu->cbInstr;

    /* Illegal instruction -> end of analysis phase for this code block */
    if (fIllegalInstr || patmIsIllegalInstr(pPatch, pCurInstrGC))
        return VINF_SUCCESS;

    /* Check for exit points. */
    switch (pCpu->pCurInstr->uOpcode)
    {
    case OP_SYSEXIT:
        return VINF_SUCCESS; /* duplicate it; will fault or emulated in GC. */

    case OP_SYSENTER:
    case OP_ILLUD2:
        /* This appears to be some kind of kernel panic in Linux 2.4; no point to analyse more. */
        Log(("Illegal opcode (0xf 0xb) -> return here\n"));
        return VINF_SUCCESS;

    case OP_STI:
    case OP_POPF:
        Assert(!(pPatch->flags & (PATMFL_DUPLICATE_FUNCTION)));
        /* If out exit point lies within the generated patch jump, then we have to refuse!! */
        if (pCurInstrGC > pPatch->pPrivInstrGC && pCurInstrGC < pPatch->pPrivInstrGC + SIZEOF_NEARJUMP32) /* hardcoded patch jump size; cbPatchJump is still zero */
        {
            Log(("Exit point within patch jump itself!! (%x vs %x)\n", pCurInstrGC, pPatch->pPrivInstrGC));
            return VERR_PATCHING_REFUSED;
        }
        if (pPatch->opcode == OP_PUSHF)
        {
            if (pCpu->pCurInstr->uOpcode == OP_POPF)
            {
                if (pPatch->cbPatchBlockSize >= SIZEOF_NEARJUMP32)
                    return VINF_SUCCESS;

                /* Or else we need to duplicate more instructions, because we can't jump back yet! */
                Log(("WARNING: End of block reached, but we need to duplicate some extra instruction to avoid a conflict with the patch jump\n"));
                pPatch->flags |= PATMFL_CHECK_SIZE;
            }
            break;  /* sti doesn't mark the end of a pushf block; only popf does. */
        }
        RT_FALL_THRU();
    case OP_RETN: /* exit point for function replacement */
        return VINF_SUCCESS;

    case OP_IRET:
        return VINF_SUCCESS;    /* exitpoint */

    case OP_CPUID:
    case OP_CALL:
    case OP_JMP:
        break;

#ifdef VBOX_WITH_SAFE_STR   /** @todo remove DISOPTYPE_PRIVILEGED_NOTRAP from disasm table */
    case OP_STR:
        break;
#endif

    default:
        if (pCpu->pCurInstr->fOpType & (DISOPTYPE_PRIVILEGED_NOTRAP))
        {
            patmAddIllegalInstrRecord(pVM, pPatch, pCurInstrGC);
            return VINF_SUCCESS;    /* exit point */
        }
        break;
    }

    /* If single instruction patch, we've copied enough instructions *and* the current instruction is not a relative jump. */
    if ((pPatch->flags & PATMFL_CHECK_SIZE) && pPatch->cbPatchBlockSize > SIZEOF_NEARJUMP32 && !(pCpu->pCurInstr->fOpType & DISOPTYPE_RELATIVE_CONTROLFLOW))
    {
        /* The end marker for this kind of patch is any instruction at a location outside our patch jump. */
        Log(("End of block at %RRv size %d\n", pCurInstrGC, pCpu->cbInstr));
        return VINF_SUCCESS;
    }

    return VWRN_CONTINUE_ANALYSIS;
}

/**
 * Analyses the instructions inside a function for compliance
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pCpu        CPU disassembly state
 * @param   pInstrGC    Guest context pointer to privileged instruction
 * @param   pCurInstrGC Guest context pointer to the current instruction
 * @param   pCacheRec   Cache record ptr
 *
 */
static int patmAnalyseFunctionCallback(PVM pVM, DISCPUSTATE *pCpu, RCPTRTYPE(uint8_t *) pInstrGC, RCPTRTYPE(uint8_t *) pCurInstrGC, PPATMP2GLOOKUPREC pCacheRec)
{
    PPATCHINFO pPatch = (PPATCHINFO)pCacheRec->pPatch;
    bool       fIllegalInstr = false;
    NOREF(pInstrGC);

    //Preliminary heuristics:
    //- no call instructions
    //- ret ends a block

    Assert(pPatch->flags & (PATMFL_DUPLICATE_FUNCTION));

    // bail out if the patch gets too big
    if (pPatch->cbPatchBlockSize >= MAX_PATCH_SIZE)
    {
        Log(("Code block too big (%x) for function patch at %RRv!!\n", pPatch->cbPatchBlockSize, pCurInstrGC));
        fIllegalInstr = true;
        patmAddIllegalInstrRecord(pVM, pPatch, pCurInstrGC);
    }
    else
    {
        // no unconditional jumps or calls without fixed displacements
        if (    (pCpu->pCurInstr->fOpType & DISOPTYPE_CONTROLFLOW)
             && (pCpu->pCurInstr->uOpcode == OP_JMP || pCpu->pCurInstr->uOpcode == OP_CALL)
           )
        {
            Assert(pCpu->Param1.cb <= 4 || pCpu->Param1.cb == 6);
            if (    pCpu->Param1.cb == 6 /* far call/jmp */
                ||  (pCpu->pCurInstr->uOpcode == OP_CALL && !(pPatch->flags & PATMFL_SUPPORT_CALLS))
                ||  (OP_PARM_VTYPE(pCpu->pCurInstr->fParam1) != OP_PARM_J && !(pPatch->flags & PATMFL_SUPPORT_INDIRECT_CALLS))
               )
            {
                fIllegalInstr = true;
                patmAddIllegalInstrRecord(pVM, pPatch, pCurInstrGC);
            }
        }
        else /* no far returns */
        if (pCpu->pCurInstr->uOpcode == OP_RETF)
        {
            patmAddIllegalInstrRecord(pVM, pPatch, pCurInstrGC);
            fIllegalInstr = true;
        }
        else /* no int xx or into either */
        if (pCpu->pCurInstr->uOpcode == OP_INT3 || pCpu->pCurInstr->uOpcode == OP_INT || pCpu->pCurInstr->uOpcode == OP_INTO)
        {
            patmAddIllegalInstrRecord(pVM, pPatch, pCurInstrGC);
            fIllegalInstr = true;
        }

    #if 0
        /// @todo we can handle certain in/out and privileged instructions in the guest context
        if (pCpu->pCurInstr->fOpType & DISOPTYPE_PRIVILEGED && pCpu->pCurInstr->uOpcode != OP_STI)
        {
            Log(("Illegal instructions for function patch!!\n"));
            return VERR_PATCHING_REFUSED;
        }
    #endif
    }

    pPatch->cbPatchBlockSize += pCpu->cbInstr;

    /* Illegal instruction -> end of analysis phase for this code block */
    if (fIllegalInstr || patmIsIllegalInstr(pPatch, pCurInstrGC))
    {
        return VINF_SUCCESS;
    }

    // Check for exit points
    switch (pCpu->pCurInstr->uOpcode)
    {
    case OP_ILLUD2:
        //This appears to be some kind of kernel panic in Linux 2.4; no point to analyse more
        Log(("Illegal opcode (0xf 0xb) -> return here\n"));
        return VINF_SUCCESS;

    case OP_IRET:
    case OP_SYSEXIT: /* will fault or emulated in GC */
    case OP_RETN:
        return VINF_SUCCESS;

#ifdef VBOX_WITH_SAFE_STR   /** @todo remove DISOPTYPE_PRIVILEGED_NOTRAP from disasm table */
    case OP_STR:
        break;
#endif

    case OP_POPF:
    case OP_STI:
        return VWRN_CONTINUE_ANALYSIS;
    default:
        if (pCpu->pCurInstr->fOpType & (DISOPTYPE_PRIVILEGED_NOTRAP))
        {
            patmAddIllegalInstrRecord(pVM, pPatch, pCurInstrGC);
            return VINF_SUCCESS;    /* exit point */
        }
        return VWRN_CONTINUE_ANALYSIS;
    }

    return VWRN_CONTINUE_ANALYSIS;
}

/**
 * Recompiles the instructions in a code block
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pCpu        CPU disassembly state
 * @param   pInstrGC    Guest context pointer to privileged instruction
 * @param   pCurInstrGC Guest context pointer to the current instruction
 * @param   pCacheRec   Cache record ptr
 *
 */
static DECLCALLBACK(int) patmRecompileCallback(PVM pVM, DISCPUSTATE *pCpu, RCPTRTYPE(uint8_t *) pInstrGC, RCPTRTYPE(uint8_t *) pCurInstrGC, PPATMP2GLOOKUPREC pCacheRec)
{
    PPATCHINFO pPatch = (PPATCHINFO)pCacheRec->pPatch;
    int        rc     = VINF_SUCCESS;
    bool       fInhibitIRQInstr = false;  /* did the instruction cause PATMFL_INHIBITIRQS to be set? */

    LogFlow(("patmRecompileCallback %RRv %RRv\n", pInstrGC, pCurInstrGC));

    if (    patmGuestGCPtrToPatchGCPtr(pVM, pPatch, pCurInstrGC) != 0
        &&  !(pPatch->flags & PATMFL_RECOMPILE_NEXT)) /* do not do this when the next instruction *must* be executed! */
    {
        /*
         * Been there, done that; so insert a jump (we don't want to duplicate code)
         * no need to record this instruction as it's glue code that never crashes (it had better not!)
         */
        Log(("patmRecompileCallback: jump to code we've recompiled before %RRv!\n", pCurInstrGC));
        return patmPatchGenRelJump(pVM, pPatch, pCurInstrGC, OP_JMP, !!(pCpu->fPrefix & DISPREFIX_OPSIZE));
    }

    if (pPatch->flags & (PATMFL_DUPLICATE_FUNCTION))
    {
        rc = patmAnalyseFunctionCallback(pVM, pCpu, pInstrGC, pCurInstrGC, pCacheRec);
    }
    else
        rc = patmAnalyseBlockCallback(pVM, pCpu, pInstrGC, pCurInstrGC, pCacheRec);

    if (RT_FAILURE(rc))
        return rc;

    /* Note: Never do a direct return unless a failure is encountered! */

    /* Clear recompilation of next instruction flag; we are doing that right here. */
    if (pPatch->flags & PATMFL_RECOMPILE_NEXT)
        pPatch->flags &= ~PATMFL_RECOMPILE_NEXT;

    /* Add lookup record for patch to guest address translation */
    patmR3AddP2GLookupRecord(pVM, pPatch, PATCHCODE_PTR_HC(pPatch) + pPatch->uCurPatchOffset, pCurInstrGC, PATM_LOOKUP_BOTHDIR);

    /* Update lowest and highest instruction address for this patch */
    if (pCurInstrGC < pPatch->pInstrGCLowest)
        pPatch->pInstrGCLowest = pCurInstrGC;
    else
    if (pCurInstrGC > pPatch->pInstrGCHighest)
        pPatch->pInstrGCHighest = pCurInstrGC + pCpu->cbInstr;

    /* Illegal instruction -> end of recompile phase for this code block. */
    if (patmIsIllegalInstr(pPatch, pCurInstrGC))
    {
        Log(("Illegal instruction at %RRv -> mark with int 3\n", pCurInstrGC));
        rc = patmPatchGenIllegalInstr(pVM, pPatch);
        goto end;
    }

    /* For our first attempt, we'll handle only simple relative jumps (immediate offset coded in instruction).
     * Indirect calls are handled below.
     */
    if (   (pCpu->pCurInstr->fOpType & DISOPTYPE_CONTROLFLOW)
        && (pCpu->pCurInstr->uOpcode != OP_CALL || (pPatch->flags & PATMFL_SUPPORT_CALLS))
        && (OP_PARM_VTYPE(pCpu->pCurInstr->fParam1) == OP_PARM_J))
    {
        RCPTRTYPE(uint8_t *) pTargetGC = PATMResolveBranch(pCpu, pCurInstrGC);
        if (pTargetGC == 0)
        {
            Log(("We don't support far jumps here!! (%08X)\n", pCpu->Param1.fUse));
            return VERR_PATCHING_REFUSED;
        }

        if (pCpu->pCurInstr->uOpcode == OP_CALL)
        {
            Assert(!PATMIsPatchGCAddr(pVM, pTargetGC));
            rc = patmPatchGenCall(pVM, pPatch, pCpu, pCurInstrGC, pTargetGC, false);
            if (RT_FAILURE(rc))
                goto end;
        }
        else
            rc = patmPatchGenRelJump(pVM, pPatch, pTargetGC, pCpu->pCurInstr->uOpcode, !!(pCpu->fPrefix & DISPREFIX_OPSIZE));

        if (RT_SUCCESS(rc))
            rc = VWRN_CONTINUE_RECOMPILE;

        goto end;
    }

    switch (pCpu->pCurInstr->uOpcode)
    {
    case OP_CLI:
    {
        /* If a cli is found while duplicating instructions for another patch, then it's of vital importance to continue
         * until we've found the proper exit point(s).
         */
        if (    pCurInstrGC != pInstrGC
            && !(pPatch->flags & (PATMFL_DUPLICATE_FUNCTION))
           )
        {
            Log(("cli instruction found in other instruction patch block; force it to continue & find an exit point\n"));
            pPatch->flags &= ~(PATMFL_CHECK_SIZE | PATMFL_SINGLE_INSTRUCTION);
        }
        /* Set by irq inhibition; no longer valid now. */
        pPatch->flags &= ~PATMFL_GENERATE_JUMPTOGUEST;

        rc = patmPatchGenCli(pVM, pPatch);
        if (RT_SUCCESS(rc))
            rc = VWRN_CONTINUE_RECOMPILE;
        break;
    }

    case OP_MOV:
        if (pCpu->pCurInstr->fOpType & DISOPTYPE_POTENTIALLY_DANGEROUS)
        {
            /* mov ss, src? */
            if (    (pCpu->Param1.fUse & DISUSE_REG_SEG)
                &&  (pCpu->Param1.Base.idxSegReg == DISSELREG_SS))
            {
                Log(("Force recompilation of next instruction for OP_MOV at %RRv\n", pCurInstrGC));
                pPatch->flags |= PATMFL_RECOMPILE_NEXT;
                /** @todo this could cause a fault (ring 0 selector being loaded in ring 1) */
            }
#if 0 /* necessary for Haiku */
            else
            if (    (pCpu->Param2.fUse & DISUSE_REG_SEG)
                &&  (pCpu->Param2.Base.idxSegReg == USE_REG_SS)
                &&  (pCpu->Param1.fUse & (DISUSE_REG_GEN32|DISUSE_REG_GEN16)))     /** @todo memory operand must in theory be handled too */
            {
                /* mov GPR, ss */
                rc = patmPatchGenMovFromSS(pVM, pPatch, pCpu, pCurInstrGC);
                if (RT_SUCCESS(rc))
                    rc = VWRN_CONTINUE_RECOMPILE;
                break;
            }
#endif
        }
        goto duplicate_instr;

    case OP_POP:
        /** @todo broken comparison!! should be if ((pCpu->Param1.fUse & DISUSE_REG_SEG) &&  (pCpu->Param1.Base.idxSegReg == DISSELREG_SS)) */
        if (pCpu->pCurInstr->fParam1 == OP_PARM_REG_SS)
        {
            Assert(pCpu->pCurInstr->fOpType & DISOPTYPE_INHIBIT_IRQS);

            Log(("Force recompilation of next instruction for OP_MOV at %RRv\n", pCurInstrGC));
            pPatch->flags |= PATMFL_RECOMPILE_NEXT;
        }
        goto duplicate_instr;

    case OP_STI:
    {
        RTRCPTR pNextInstrGC = 0;   /* by default no inhibit irq */

        /* In a sequence of instructions that inhibit irqs, only the first one actually inhibits irqs. */
        if (!(pPatch->flags & PATMFL_INHIBIT_IRQS))
        {
            pPatch->flags   |= PATMFL_INHIBIT_IRQS | PATMFL_GENERATE_JUMPTOGUEST;
            fInhibitIRQInstr = true;
            pNextInstrGC     = pCurInstrGC + pCpu->cbInstr;
            Log(("Inhibit irqs for instruction OP_STI at %RRv\n", pCurInstrGC));
        }
        rc = patmPatchGenSti(pVM, pPatch, pCurInstrGC, pNextInstrGC);

        if (RT_SUCCESS(rc))
        {
            DISCPUSTATE cpu = *pCpu;
            unsigned    cbInstr;
            int         disret;
            RCPTRTYPE(uint8_t *) pReturnInstrGC;

            pPatch->flags |= PATMFL_FOUND_PATCHEND;

            pNextInstrGC = pCurInstrGC + pCpu->cbInstr;
            {   /* Force pNextInstrHC out of scope after using it */
                uint8_t *pNextInstrHC = patmR3GCVirtToHCVirt(pVM, pCacheRec, pNextInstrGC);
                if (pNextInstrHC == NULL)
                {
                    AssertFailed();
                    return VERR_PATCHING_REFUSED;
                }

                // Disassemble the next instruction
                disret = patmR3DisInstr(pVM, pPatch, pNextInstrGC, pNextInstrHC, PATMREAD_ORGCODE, &cpu, &cbInstr);
            }
            if (disret == false)
            {
                AssertMsgFailed(("STI: Disassembly failed (probably page not present) -> return to caller\n"));
                return VERR_PATCHING_REFUSED;
            }
            pReturnInstrGC = pNextInstrGC + cbInstr;

            if (   (pPatch->flags & (PATMFL_DUPLICATE_FUNCTION))
                ||  pReturnInstrGC <= pInstrGC
                ||  pReturnInstrGC - pInstrGC >= SIZEOF_NEARJUMP32
               )
            {
                /* Not an exit point for function duplication patches */
                if (    (pPatch->flags & PATMFL_DUPLICATE_FUNCTION)
                    &&  RT_SUCCESS(rc))
                {
                    pPatch->flags &= ~PATMFL_GENERATE_JUMPTOGUEST;  /* Don't generate a jump back */
                    rc = VWRN_CONTINUE_RECOMPILE;
                }
                else
                    rc = VINF_SUCCESS;  //exit point
            }
            else {
                Log(("PATM: sti occurred too soon; refusing patch!\n"));
                rc = VERR_PATCHING_REFUSED; //not allowed!!
            }
        }
        break;
    }

    case OP_POPF:
    {
        bool fGenerateJmpBack = (pCurInstrGC + pCpu->cbInstr - pInstrGC >= SIZEOF_NEARJUMP32);

        /* Not an exit point for IDT handler or function replacement patches */
        /* Note: keep IOPL in mind when changing any of this!! (see comments in PATMA.asm, PATMPopf32Replacement) */
        if (pPatch->flags & (PATMFL_IDTHANDLER|PATMFL_DUPLICATE_FUNCTION))
            fGenerateJmpBack = false;

        rc = patmPatchGenPopf(pVM, pPatch, pCurInstrGC + pCpu->cbInstr, !!(pCpu->fPrefix & DISPREFIX_OPSIZE), fGenerateJmpBack);
        if (RT_SUCCESS(rc))
        {
            if (fGenerateJmpBack == false)
            {
                /* Not an exit point for IDT handler or function replacement patches */
                rc = VWRN_CONTINUE_RECOMPILE;
            }
            else
            {
                pPatch->flags |= PATMFL_FOUND_PATCHEND;
                rc = VINF_SUCCESS;  /* exit point! */
            }
        }
        break;
    }

    case OP_PUSHF:
        rc = patmPatchGenPushf(pVM, pPatch, !!(pCpu->fPrefix & DISPREFIX_OPSIZE));
        if (RT_SUCCESS(rc))
            rc = VWRN_CONTINUE_RECOMPILE;
        break;

    case OP_PUSH:
        /** @todo broken comparison!! should be if ((pCpu->Param1.fUse & DISUSE_REG_SEG) &&  (pCpu->Param1.Base.idxSegReg == DISSELREG_SS)) */
        if (pCpu->pCurInstr->fParam1 == OP_PARM_REG_CS)
        {
            rc = patmPatchGenPushCS(pVM, pPatch);
            if (RT_SUCCESS(rc))
                rc = VWRN_CONTINUE_RECOMPILE;
            break;
        }
        goto duplicate_instr;

    case OP_IRET:
        Log(("IRET at %RRv\n", pCurInstrGC));
        rc = patmPatchGenIret(pVM, pPatch, pCurInstrGC, !!(pCpu->fPrefix & DISPREFIX_OPSIZE));
        if (RT_SUCCESS(rc))
        {
            pPatch->flags |= PATMFL_FOUND_PATCHEND;
            rc = VINF_SUCCESS;  /* exit point by definition */
        }
        break;

    case OP_ILLUD2:
        /* This appears to be some kind of kernel panic in Linux 2.4; no point to continue */
        rc = patmPatchGenIllegalInstr(pVM, pPatch);
        if (RT_SUCCESS(rc))
            rc = VINF_SUCCESS;  /* exit point by definition */
        Log(("Illegal opcode (0xf 0xb)\n"));
        break;

    case OP_CPUID:
        rc = patmPatchGenCpuid(pVM, pPatch, pCurInstrGC);
        if (RT_SUCCESS(rc))
            rc = VWRN_CONTINUE_RECOMPILE;
        break;

    case OP_STR:
#ifdef VBOX_WITH_SAFE_STR   /** @todo remove DISOPTYPE_PRIVILEGED_NOTRAP from disasm table and move OP_STR into ifndef */
        /* Now safe because our shadow TR entry is identical to the guest's. */
        goto duplicate_instr;
#endif
    case OP_SLDT:
        rc = patmPatchGenSldtStr(pVM, pPatch, pCpu, pCurInstrGC);
        if (RT_SUCCESS(rc))
            rc = VWRN_CONTINUE_RECOMPILE;
        break;

    case OP_SGDT:
    case OP_SIDT:
        rc = patmPatchGenSxDT(pVM, pPatch, pCpu, pCurInstrGC);
        if (RT_SUCCESS(rc))
            rc = VWRN_CONTINUE_RECOMPILE;
        break;

    case OP_RETN:
        /* retn is an exit point for function patches */
        rc = patmPatchGenRet(pVM, pPatch, pCpu, pCurInstrGC);
        if (RT_SUCCESS(rc))
            rc = VINF_SUCCESS;  /* exit point by definition */
        break;

    case OP_SYSEXIT:
        /* Duplicate it, so it can be emulated in GC (or fault). */
        rc = patmPatchGenDuplicate(pVM, pPatch, pCpu, pCurInstrGC);
        if (RT_SUCCESS(rc))
            rc = VINF_SUCCESS;  /* exit point by definition */
        break;

    case OP_CALL:
        Assert(pPatch->flags & PATMFL_SUPPORT_INDIRECT_CALLS);
        /* In interrupt gate handlers it's possible to encounter jumps or calls when IF has been enabled again.
         * In that case we'll jump to the original instruction and continue from there. Otherwise an int 3 is executed.
         */
        Assert(pCpu->Param1.cb == 4 || pCpu->Param1.cb == 6);
        if (pPatch->flags & PATMFL_SUPPORT_INDIRECT_CALLS && pCpu->Param1.cb == 4 /* no far calls! */)
        {
            rc = patmPatchGenCall(pVM, pPatch, pCpu, pCurInstrGC, (RTRCPTR)0xDEADBEEF, true);
            if (RT_SUCCESS(rc))
            {
                rc = VWRN_CONTINUE_RECOMPILE;
            }
            break;
        }
        goto gen_illegal_instr;

    case OP_JMP:
        Assert(pPatch->flags & PATMFL_SUPPORT_INDIRECT_CALLS);
        /* In interrupt gate handlers it's possible to encounter jumps or calls when IF has been enabled again.
         * In that case we'll jump to the original instruction and continue from there. Otherwise an int 3 is executed.
         */
        Assert(pCpu->Param1.cb == 4 || pCpu->Param1.cb == 6);
        if (pPatch->flags & PATMFL_SUPPORT_INDIRECT_CALLS && pCpu->Param1.cb == 4 /* no far jumps! */)
        {
            rc = patmPatchGenJump(pVM, pPatch, pCpu, pCurInstrGC);
            if (RT_SUCCESS(rc))
                rc = VINF_SUCCESS;  /* end of branch */
            break;
        }
        goto gen_illegal_instr;

    case OP_INT3:
    case OP_INT:
    case OP_INTO:
        goto gen_illegal_instr;

    case OP_MOV_DR:
        /* Note: currently we let DRx writes cause a trap d; our trap handler will decide to interpret it or not. */
        if (pCpu->pCurInstr->fParam2 == OP_PARM_Dd)
        {
            rc = patmPatchGenMovDebug(pVM, pPatch, pCpu);
            if (RT_SUCCESS(rc))
                rc = VWRN_CONTINUE_RECOMPILE;
            break;
        }
        goto duplicate_instr;

    case OP_MOV_CR:
        /* Note: currently we let CRx writes cause a trap d; our trap handler will decide to interpret it or not. */
        if (pCpu->pCurInstr->fParam2 == OP_PARM_Cd)
        {
            rc = patmPatchGenMovControl(pVM, pPatch, pCpu);
            if (RT_SUCCESS(rc))
                rc = VWRN_CONTINUE_RECOMPILE;
            break;
        }
        goto duplicate_instr;

    default:
        if (pCpu->pCurInstr->fOpType & (DISOPTYPE_CONTROLFLOW | DISOPTYPE_PRIVILEGED_NOTRAP))
        {
gen_illegal_instr:
            rc = patmPatchGenIllegalInstr(pVM, pPatch);
            if (RT_SUCCESS(rc))
                rc = VINF_SUCCESS;  /* exit point by definition */
        }
        else
        {
duplicate_instr:
            Log(("patmPatchGenDuplicate\n"));
            rc = patmPatchGenDuplicate(pVM, pPatch, pCpu, pCurInstrGC);
            if (RT_SUCCESS(rc))
                rc = VWRN_CONTINUE_RECOMPILE;
        }
        break;
    }

end:

    if (    !fInhibitIRQInstr
        && (pPatch->flags & PATMFL_INHIBIT_IRQS))
    {
        int     rc2;
        RTRCPTR pNextInstrGC = pCurInstrGC + pCpu->cbInstr;

        pPatch->flags &= ~PATMFL_INHIBIT_IRQS;
        Log(("Clear inhibit IRQ flag at %RRv\n", pCurInstrGC));
        if (pPatch->flags & PATMFL_GENERATE_JUMPTOGUEST)
        {
            Log(("patmRecompileCallback: generate jump back to guest (%RRv) after fused instruction\n", pNextInstrGC));

            rc2 = patmPatchGenJumpToGuest(pVM, pPatch, pNextInstrGC, true /* clear inhibit irq flag */);
            pPatch->flags &= ~PATMFL_GENERATE_JUMPTOGUEST;
            rc = VINF_SUCCESS; /* end of the line */
        }
        else
        {
            rc2 = patmPatchGenClearInhibitIRQ(pVM, pPatch, pNextInstrGC);
        }
        if (RT_FAILURE(rc2))
            rc = rc2;
    }

    if (RT_SUCCESS(rc))
    {
        // If single instruction patch, we've copied enough instructions *and* the current instruction is not a relative jump
        if (    (pPatch->flags & PATMFL_CHECK_SIZE)
             &&  pCurInstrGC + pCpu->cbInstr - pInstrGC >= SIZEOF_NEARJUMP32
             &&  !(pCpu->pCurInstr->fOpType & DISOPTYPE_RELATIVE_CONTROLFLOW)
             &&  !(pPatch->flags & PATMFL_RECOMPILE_NEXT) /* do not do this when the next instruction *must* be executed! */
           )
        {
            RTRCPTR pNextInstrGC = pCurInstrGC + pCpu->cbInstr;

            // The end marker for this kind of patch is any instruction at a location outside our patch jump
            Log(("patmRecompileCallback: end found for single instruction patch at %RRv cbInstr %d\n", pNextInstrGC, pCpu->cbInstr));

            rc = patmPatchGenJumpToGuest(pVM, pPatch, pNextInstrGC);
            AssertRC(rc);
        }
    }
    return rc;
}


#ifdef LOG_ENABLED

/**
 * Add a disasm jump record (temporary for prevent duplicate analysis)
 *
 * @param   pVM             The cross context VM structure.
 * @param   pPatch          Patch structure ptr
 * @param   pInstrGC        Guest context pointer to privileged instruction
 *
 */
static void patmPatchAddDisasmJump(PVM pVM, PPATCHINFO pPatch, RTRCPTR pInstrGC)
{
    PAVLPVNODECORE pRec;

    pRec = (PAVLPVNODECORE)MMR3HeapAllocZ(pVM, MM_TAG_PATM_PATCH, sizeof(*pRec));
    Assert(pRec);
    pRec->Key = (AVLPVKEY)(uintptr_t)pInstrGC;

    int ret = RTAvlPVInsert(&pPatch->pTempInfo->DisasmJumpTree, pRec);
    Assert(ret);
}

/**
 * Checks if jump target has been analysed before.
 *
 * @returns VBox status code.
 * @param   pPatch      Patch struct
 * @param   pInstrGC    Jump target
 *
 */
static bool patmIsKnownDisasmJump(PPATCHINFO pPatch, RTRCPTR pInstrGC)
{
    PAVLPVNODECORE pRec;

    pRec = RTAvlPVGet(&pPatch->pTempInfo->DisasmJumpTree, (AVLPVKEY)(uintptr_t)pInstrGC);
    if (pRec)
        return true;
    return false;
}

/**
 * For proper disassembly of the final patch block
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pCpu        CPU disassembly state
 * @param   pInstrGC    Guest context pointer to privileged instruction
 * @param   pCurInstrGC Guest context pointer to the current instruction
 * @param   pCacheRec   Cache record ptr
 *
 */
DECLCALLBACK(int) patmR3DisasmCallback(PVM pVM, DISCPUSTATE *pCpu, RCPTRTYPE(uint8_t *) pInstrGC,
                                       RCPTRTYPE(uint8_t *) pCurInstrGC, PPATMP2GLOOKUPREC pCacheRec)
{
    PPATCHINFO pPatch = (PPATCHINFO)pCacheRec->pPatch;
    NOREF(pInstrGC);

    if (pCpu->pCurInstr->uOpcode == OP_INT3)
    {
        /* Could be an int3 inserted in a call patch. Check to be sure */
        DISCPUSTATE cpu;
        RTRCPTR     pOrgJumpGC;

        pOrgJumpGC = patmPatchGCPtr2GuestGCPtr(pVM, pPatch, pCurInstrGC);

        {   /* Force pOrgJumpHC out of scope after using it */
            uint8_t *pOrgJumpHC = patmR3GCVirtToHCVirt(pVM, pCacheRec, pOrgJumpGC);

            bool disret = patmR3DisInstr(pVM, pPatch, pOrgJumpGC, pOrgJumpHC, PATMREAD_ORGCODE, &cpu, NULL);
            if (!disret || cpu.pCurInstr->uOpcode != OP_CALL || cpu.Param1.cb != 4 /* only near calls */)
                return VINF_SUCCESS;
        }
        return VWRN_CONTINUE_ANALYSIS;
    }

    if (    pCpu->pCurInstr->uOpcode == OP_ILLUD2
        &&  PATMIsPatchGCAddr(pVM, pCurInstrGC))
    {
        /* the indirect call patch contains an 0xF/0xB illegal instr to call for assistance; check for this and continue */
        return VWRN_CONTINUE_ANALYSIS;
    }

    if (   (pCpu->pCurInstr->uOpcode == OP_CALL && !(pPatch->flags & PATMFL_SUPPORT_CALLS))
        || pCpu->pCurInstr->uOpcode == OP_INT
        || pCpu->pCurInstr->uOpcode == OP_IRET
        || pCpu->pCurInstr->uOpcode == OP_RETN
        || pCpu->pCurInstr->uOpcode == OP_RETF
       )
    {
        return VINF_SUCCESS;
    }

    if (pCpu->pCurInstr->uOpcode == OP_ILLUD2)
        return VINF_SUCCESS;

    return VWRN_CONTINUE_ANALYSIS;
}


/**
 * Disassembles the code stream until the callback function detects a failure or decides everything is acceptable
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context pointer to the initial privileged instruction
 * @param   pCurInstrGC Guest context pointer to the current instruction
 * @param   pfnPATMR3Disasm Callback for testing the disassembled instruction
 * @param   pCacheRec   Cache record ptr
 *
 */
int patmr3DisasmCode(PVM pVM, RCPTRTYPE(uint8_t *) pInstrGC, RCPTRTYPE(uint8_t *) pCurInstrGC, PFN_PATMR3ANALYSE pfnPATMR3Disasm, PPATMP2GLOOKUPREC pCacheRec)
{
    DISCPUSTATE cpu;
    PPATCHINFO pPatch = (PPATCHINFO)pCacheRec->pPatch;
    int rc = VWRN_CONTINUE_ANALYSIS;
    uint32_t cbInstr, delta;
    R3PTRTYPE(uint8_t *) pCurInstrHC = 0;
    bool disret;
    char szOutput[256];

    Assert(pCurInstrHC != PATCHCODE_PTR_HC(pPatch) || pPatch->pTempInfo->DisasmJumpTree == 0);

    /* We need this to determine branch targets (and for disassembling). */
    delta = pVM->patm.s.pPatchMemGC - (uintptr_t)pVM->patm.s.pPatchMemHC;

    while (rc == VWRN_CONTINUE_ANALYSIS)
    {
        pCurInstrHC = patmR3GCVirtToHCVirt(pVM, pCacheRec, pCurInstrGC);
        if (pCurInstrHC == NULL)
        {
            rc = VERR_PATCHING_REFUSED;
            goto end;
        }

        disret = patmR3DisInstrToStr(pVM, pPatch, pCurInstrGC, pCurInstrHC, PATMREAD_RAWCODE,
                                     &cpu, &cbInstr, szOutput, sizeof(szOutput));
        if (PATMIsPatchGCAddr(pVM, pCurInstrGC))
        {
            RTRCPTR pOrgInstrGC = patmPatchGCPtr2GuestGCPtr(pVM, pPatch, pCurInstrGC);

            if (pOrgInstrGC != pPatch->pTempInfo->pLastDisasmInstrGC)
                Log(("DIS %RRv<-%s", pOrgInstrGC, szOutput));
            else
                Log(("DIS           %s", szOutput));

            pPatch->pTempInfo->pLastDisasmInstrGC = pOrgInstrGC;
            if (patmIsIllegalInstr(pPatch, pOrgInstrGC))
            {
                rc = VINF_SUCCESS;
                goto end;
            }
        }
        else
            Log(("DIS: %s", szOutput));

        if (disret == false)
        {
            Log(("Disassembly failed (probably page not present) -> return to caller\n"));
            rc = VINF_SUCCESS;
            goto end;
        }

        rc = pfnPATMR3Disasm(pVM, &cpu, pInstrGC, pCurInstrGC, pCacheRec);
        if (rc != VWRN_CONTINUE_ANALYSIS) {
            break; //done!
        }

        /* For our first attempt, we'll handle only simple relative jumps and calls (immediate offset coded in instruction) */
        if (   (cpu.pCurInstr->fOpType & DISOPTYPE_CONTROLFLOW)
            && (OP_PARM_VTYPE(cpu.pCurInstr->fParam1) == OP_PARM_J)
            &&  cpu.pCurInstr->uOpcode != OP_CALL /* complete functions are replaced; don't bother here. */
           )
        {
            RTRCPTR pTargetGC = PATMResolveBranch(&cpu, pCurInstrGC);
            RTRCPTR pOrgTargetGC;

            if (pTargetGC == 0)
            {
                Log(("We don't support far jumps here!! (%08X)\n", cpu.Param1.fUse));
                rc = VERR_PATCHING_REFUSED;
                break;
            }

            if (!PATMIsPatchGCAddr(pVM, pTargetGC))
            {
                //jump back to guest code
                rc = VINF_SUCCESS;
                goto end;
            }
            pOrgTargetGC = PATMR3PatchToGCPtr(pVM, pTargetGC, 0);

            if (patmIsCommonIDTHandlerPatch(pVM, pOrgTargetGC))
            {
                rc = VINF_SUCCESS;
                goto end;
            }

            if (patmIsKnownDisasmJump(pPatch, pTargetGC) == false)
            {
                /* New jump, let's check it. */
                patmPatchAddDisasmJump(pVM, pPatch, pTargetGC);

                if (cpu.pCurInstr->uOpcode == OP_CALL)  pPatch->pTempInfo->nrCalls++;
                rc = patmr3DisasmCode(pVM, pInstrGC, pTargetGC, pfnPATMR3Disasm, pCacheRec);
                if (cpu.pCurInstr->uOpcode == OP_CALL)  pPatch->pTempInfo->nrCalls--;

                if (rc != VINF_SUCCESS) {
                    break; //done!
                }
            }
            if (cpu.pCurInstr->uOpcode == OP_JMP)
            {
                /* Unconditional jump; return to caller. */
                rc = VINF_SUCCESS;
                goto end;
            }

            rc = VWRN_CONTINUE_ANALYSIS;
        }
        pCurInstrGC += cbInstr;
    }
end:
    return rc;
}

/**
 * Disassembles the code stream until the callback function detects a failure or decides everything is acceptable
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context pointer to the initial privileged instruction
 * @param   pCurInstrGC Guest context pointer to the current instruction
 * @param   pfnPATMR3Disasm Callback for testing the disassembled instruction
 * @param   pCacheRec   Cache record ptr
 *
 */
int patmr3DisasmCodeStream(PVM pVM, RCPTRTYPE(uint8_t *) pInstrGC, RCPTRTYPE(uint8_t *) pCurInstrGC, PFN_PATMR3ANALYSE pfnPATMR3Disasm, PPATMP2GLOOKUPREC pCacheRec)
{
    PPATCHINFO pPatch = (PPATCHINFO)pCacheRec->pPatch;

    int rc = patmr3DisasmCode(pVM, pInstrGC, pCurInstrGC, pfnPATMR3Disasm, pCacheRec);
    /* Free all disasm jump records. */
    patmEmptyTree(pVM, &pPatch->pTempInfo->DisasmJumpTree);
    return rc;
}

#endif /* LOG_ENABLED */

/**
 * Detects it the specified address falls within a 5 byte jump generated for an active patch.
 * If so, this patch is permanently disabled.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context pointer to instruction
 * @param   pConflictGC Guest context pointer to check
 *
 * @note also checks for patch hints to make sure they can never be enabled if a conflict is present.
 *
 */
VMMR3_INT_DECL(int) PATMR3DetectConflict(PVM pVM, RTRCPTR pInstrGC, RTRCPTR pConflictGC)
{
    AssertReturn(!HMIsEnabled(pVM), VERR_PATCH_NO_CONFLICT);
    PPATCHINFO pTargetPatch = patmFindActivePatchByEntrypoint(pVM, pConflictGC, true /* include patch hints */);
    if (pTargetPatch)
        return patmDisableUnusablePatch(pVM, pInstrGC, pConflictGC, pTargetPatch);
    return VERR_PATCH_NO_CONFLICT;
}

/**
 * Recompile the code stream until the callback function detects a failure or decides everything is acceptable
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context pointer to privileged instruction
 * @param   pCurInstrGC Guest context pointer to the current instruction
 * @param   pfnPATMR3Recompile Callback for testing the disassembled instruction
 * @param   pCacheRec   Cache record ptr
 *
 */
static int patmRecompileCodeStream(PVM pVM, RCPTRTYPE(uint8_t *) pInstrGC, RCPTRTYPE(uint8_t *) pCurInstrGC, PFN_PATMR3ANALYSE pfnPATMR3Recompile, PPATMP2GLOOKUPREC pCacheRec)
{
    DISCPUSTATE cpu;
    PPATCHINFO pPatch = (PPATCHINFO)pCacheRec->pPatch;
    int rc = VWRN_CONTINUE_ANALYSIS;
    uint32_t cbInstr;
    R3PTRTYPE(uint8_t *) pCurInstrHC = 0;
    bool disret;
#ifdef LOG_ENABLED
    char szOutput[256];
#endif

    while (rc == VWRN_CONTINUE_RECOMPILE)
    {
        pCurInstrHC = patmR3GCVirtToHCVirt(pVM, pCacheRec, pCurInstrGC);
        if (pCurInstrHC == NULL)
        {
            rc = VERR_PATCHING_REFUSED;   /* fatal in this case */
            goto end;
        }
#ifdef LOG_ENABLED
        disret = patmR3DisInstrToStr(pVM, pPatch, pCurInstrGC, pCurInstrHC, PATMREAD_ORGCODE,
                                     &cpu, &cbInstr, szOutput, sizeof(szOutput));
        Log(("Recompile: %s", szOutput));
#else
        disret = patmR3DisInstr(pVM, pPatch, pCurInstrGC, pCurInstrHC, PATMREAD_ORGCODE, &cpu, &cbInstr);
#endif
        if (disret == false)
        {
            Log(("Disassembly failed (probably page not present) -> return to caller\n"));

            /* Add lookup record for patch to guest address translation */
            patmR3AddP2GLookupRecord(pVM, pPatch, PATCHCODE_PTR_HC(pPatch) + pPatch->uCurPatchOffset, pCurInstrGC, PATM_LOOKUP_BOTHDIR);
            patmPatchGenIllegalInstr(pVM, pPatch);
            rc = VINF_SUCCESS;   /* Note: don't fail here; we might refuse an important patch!! */
            goto end;
        }

        rc = pfnPATMR3Recompile(pVM, &cpu, pInstrGC, pCurInstrGC, pCacheRec);
        if (rc != VWRN_CONTINUE_RECOMPILE)
        {
            /* If irqs are inhibited because of the current instruction, then we must make sure the next one is executed! */
            if (    rc == VINF_SUCCESS
                && (pPatch->flags & PATMFL_INHIBIT_IRQS))
            {
                DISCPUSTATE cpunext;
                uint32_t    opsizenext;
                uint8_t *pNextInstrHC;
                RTRCPTR  pNextInstrGC = pCurInstrGC + cbInstr;

                Log(("patmRecompileCodeStream: irqs inhibited by instruction %RRv\n", pNextInstrGC));

                /* Certain instructions (e.g. sti) force the next instruction to be executed before any interrupts can occur.
                 * Recompile the next instruction as well
                 */
                pNextInstrHC = patmR3GCVirtToHCVirt(pVM, pCacheRec, pNextInstrGC);
                if (pNextInstrHC == NULL)
                {
                    rc = VERR_PATCHING_REFUSED;   /* fatal in this case */
                    goto end;
                }
                disret = patmR3DisInstr(pVM, pPatch, pNextInstrGC, pNextInstrHC, PATMREAD_ORGCODE, &cpunext, &opsizenext);
                if (disret == false)
                {
                    rc = VERR_PATCHING_REFUSED;   /* fatal in this case */
                    goto end;
                }
                switch(cpunext.pCurInstr->uOpcode)
                {
                case OP_IRET:       /* inhibit cleared in generated code */
                case OP_SYSEXIT:    /* faults; inhibit should be cleared in HC handling */
                case OP_HLT:
                    break; /* recompile these */

                default:
                    if (cpunext.pCurInstr->fOpType & DISOPTYPE_CONTROLFLOW)
                    {
                        Log(("Unexpected control flow instruction after inhibit irq instruction\n"));

                        rc = patmPatchGenJumpToGuest(pVM, pPatch, pNextInstrGC, true /* clear inhibit irq flag */);
                        AssertRC(rc);
                        pPatch->flags &= ~PATMFL_INHIBIT_IRQS;
                        goto end;       /** @todo should be ok to ignore instruction fusing in this case */
                    }
                    break;
                }

                /* Note: after a cli we must continue to a proper exit point */
                if (cpunext.pCurInstr->uOpcode != OP_CLI)
                {
                    rc = pfnPATMR3Recompile(pVM, &cpunext, pInstrGC, pNextInstrGC, pCacheRec);
                    if (RT_SUCCESS(rc))
                    {
                        rc = VINF_SUCCESS;
                        goto end;
                    }
                    break;
                }
                else
                    rc = VWRN_CONTINUE_RECOMPILE;
            }
            else
                break; /* done! */
        }

        /** @todo continue with the instructions following the jump and then recompile the jump target code */


        /* For our first attempt, we'll handle only simple relative jumps and calls (immediate offset coded in instruction). */
        if (   (cpu.pCurInstr->fOpType & DISOPTYPE_CONTROLFLOW)
            && (OP_PARM_VTYPE(cpu.pCurInstr->fParam1) == OP_PARM_J)
            &&  cpu.pCurInstr->uOpcode != OP_CALL /* complete functions are replaced; don't bother here. */
           )
        {
            RCPTRTYPE(uint8_t *) addr = PATMResolveBranch(&cpu, pCurInstrGC);
            if (addr == 0)
            {
                Log(("We don't support far jumps here!! (%08X)\n", cpu.Param1.fUse));
                rc = VERR_PATCHING_REFUSED;
                break;
            }

            Log(("Jump encountered target %RRv\n", addr));

            /* We don't check if the branch target lies in a valid page as we've already done that in the analysis phase. */
            if (!(cpu.pCurInstr->fOpType & DISOPTYPE_UNCOND_CONTROLFLOW))
            {
                Log(("patmRecompileCodeStream continue passed conditional jump\n"));
                /* First we need to finish this linear code stream until the next exit point. */
                rc = patmRecompileCodeStream(pVM, pInstrGC, pCurInstrGC+cbInstr, pfnPATMR3Recompile, pCacheRec);
                if (RT_FAILURE(rc))
                {
                    Log(("patmRecompileCodeStream fatal error %d\n", rc));
                    break; //fatal error
                }
            }

            if (patmGuestGCPtrToPatchGCPtr(pVM, pPatch, addr) == 0)
            {
                /* New code; let's recompile it. */
                Log(("patmRecompileCodeStream continue with jump\n"));

                 /*
                  * If we are jumping to an existing patch (or within 5 bytes of the entrypoint), then we must temporarily disable
                  * this patch so we can continue our analysis
                  *
                  * We rely on CSAM to detect and resolve conflicts
                  */
                PPATCHINFO pTargetPatch = patmFindActivePatchByEntrypoint(pVM, addr);
                if(pTargetPatch)
                {
                    Log(("Found active patch at target %RRv (%RRv) -> temporarily disabling it!!\n", addr, pTargetPatch->pPrivInstrGC));
                    PATMR3DisablePatch(pVM, pTargetPatch->pPrivInstrGC);
                }

                if (cpu.pCurInstr->uOpcode == OP_CALL)  pPatch->pTempInfo->nrCalls++;
                rc = patmRecompileCodeStream(pVM, pInstrGC, addr, pfnPATMR3Recompile, pCacheRec);
                if (cpu.pCurInstr->uOpcode == OP_CALL)  pPatch->pTempInfo->nrCalls--;

                if(pTargetPatch)
                    PATMR3EnablePatch(pVM, pTargetPatch->pPrivInstrGC);

                if (RT_FAILURE(rc))
                {
                    Log(("patmRecompileCodeStream fatal error %d\n", rc));
                    break; //done!
                }
            }
            /* Always return to caller here; we're done! */
            rc = VINF_SUCCESS;
            goto end;
        }
        else
        if (cpu.pCurInstr->fOpType & DISOPTYPE_UNCOND_CONTROLFLOW)
        {
            rc = VINF_SUCCESS;
            goto end;
        }
        pCurInstrGC += cbInstr;
    }
end:
    Assert(!(pPatch->flags & PATMFL_RECOMPILE_NEXT));
    return rc;
}


/**
 * Generate the jump from guest to patch code
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch record
 * @param   pCacheRec   Guest translation lookup cache record
 * @param   fAddFixup   Whether to add a fixup record.
 */
static int patmGenJumpToPatch(PVM pVM, PPATCHINFO pPatch, PPATMP2GLOOKUPREC pCacheRec, bool fAddFixup = true)
{
    uint8_t  temp[8];
    uint8_t *pPB;
    int      rc;

    Assert(pPatch->cbPatchJump <= sizeof(temp));
    Assert(!(pPatch->flags & PATMFL_PATCHED_GUEST_CODE));

    pPB = patmR3GCVirtToHCVirt(pVM, pCacheRec, pPatch->pPrivInstrGC);
    Assert(pPB);

#ifdef PATM_RESOLVE_CONFLICTS_WITH_JUMP_PATCHES
    if (pPatch->flags & PATMFL_JUMP_CONFLICT)
    {
        Assert(pPatch->pPatchJumpDestGC);

        if (pPatch->cbPatchJump == SIZEOF_NEARJUMP32)
        {
            // jmp [PatchCode]
            if (fAddFixup)
            {
                if (patmPatchAddReloc32(pVM, pPatch, &pPB[1], FIXUP_REL_JMPTOPATCH, pPatch->pPrivInstrGC + pPatch->cbPatchJump,
                                        pPatch->pPatchJumpDestGC) != VINF_SUCCESS)
                {
                    Log(("Relocation failed for the jump in the guest code!!\n"));
                    return VERR_PATCHING_REFUSED;
                }
            }

            temp[0] = pPatch->aPrivInstr[0];  //jump opcode copied from original instruction
            *(uint32_t *)&temp[1] = (uint32_t)pPatch->pPatchJumpDestGC - ((uint32_t)pPatch->pPrivInstrGC + pPatch->cbPatchJump);    //return address
        }
        else
        if (pPatch->cbPatchJump == SIZEOF_NEAR_COND_JUMP32)
        {
            // jmp [PatchCode]
            if (fAddFixup)
            {
                if (patmPatchAddReloc32(pVM, pPatch, &pPB[2], FIXUP_REL_JMPTOPATCH, pPatch->pPrivInstrGC + pPatch->cbPatchJump,
                                        pPatch->pPatchJumpDestGC) != VINF_SUCCESS)
                {
                    Log(("Relocation failed for the jump in the guest code!!\n"));
                    return VERR_PATCHING_REFUSED;
                }
            }

            temp[0] = pPatch->aPrivInstr[0];  //jump opcode copied from original instruction
            temp[1] = pPatch->aPrivInstr[1];  //jump opcode copied from original instruction
            *(uint32_t *)&temp[2] = (uint32_t)pPatch->pPatchJumpDestGC - ((uint32_t)pPatch->pPrivInstrGC + pPatch->cbPatchJump);    //return address
        }
        else
        {
            Assert(0);
            return VERR_PATCHING_REFUSED;
        }
    }
    else
#endif
    {
        Assert(pPatch->cbPatchJump == SIZEOF_NEARJUMP32);

        // jmp [PatchCode]
        if (fAddFixup)
        {
            if (patmPatchAddReloc32(pVM, pPatch, &pPB[1], FIXUP_REL_JMPTOPATCH, pPatch->pPrivInstrGC + SIZEOF_NEARJUMP32,
                                    PATCHCODE_PTR_GC(pPatch)) != VINF_SUCCESS)
            {
                Log(("Relocation failed for the jump in the guest code!!\n"));
                return VERR_PATCHING_REFUSED;
            }
        }
        temp[0] = 0xE9;  //jmp
        *(uint32_t *)&temp[1] = (RTRCUINTPTR)PATCHCODE_PTR_GC(pPatch) - ((RTRCUINTPTR)pPatch->pPrivInstrGC + SIZEOF_NEARJUMP32);    //return address
    }
    rc = PGMPhysSimpleDirtyWriteGCPtr(VMMGetCpu0(pVM), pPatch->pPrivInstrGC, temp, pPatch->cbPatchJump);
    AssertRC(rc);

    if (rc == VINF_SUCCESS)
        pPatch->flags |= PATMFL_PATCHED_GUEST_CODE;

    return rc;
}

/**
 * Remove the jump from guest to patch code
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch record
 */
static int patmRemoveJumpToPatch(PVM pVM, PPATCHINFO pPatch)
{
#ifdef DEBUG
    DISCPUSTATE cpu;
    char szOutput[256];
    uint32_t cbInstr, i = 0;
    bool disret;

    while (i < pPatch->cbPrivInstr)
    {
        disret = patmR3DisInstrToStr(pVM, pPatch, pPatch->pPrivInstrGC + i, NULL, PATMREAD_ORGCODE,
                                     &cpu, &cbInstr, szOutput, sizeof(szOutput));
        if (disret == false)
            break;

        Log(("Org patch jump: %s", szOutput));
        Assert(cbInstr);
        i += cbInstr;
    }
#endif

    /* Restore original code (privileged instruction + following instructions that were overwritten because of the 5/6 byte jmp). */
    int rc = PGMPhysSimpleDirtyWriteGCPtr(VMMGetCpu0(pVM), pPatch->pPrivInstrGC, pPatch->aPrivInstr, pPatch->cbPatchJump);
#ifdef DEBUG
    if (rc == VINF_SUCCESS)
    {
        i = 0;
        while (i < pPatch->cbPrivInstr)
        {
            disret = patmR3DisInstrToStr(pVM, pPatch, pPatch->pPrivInstrGC + i, NULL, PATMREAD_ORGCODE,
                                         &cpu, &cbInstr, szOutput, sizeof(szOutput));
            if (disret == false)
                break;

            Log(("Org instr: %s", szOutput));
            Assert(cbInstr);
            i += cbInstr;
        }
    }
#endif
    pPatch->flags &= ~PATMFL_PATCHED_GUEST_CODE;
    return rc;
}

/**
 * Generate the call from guest to patch code
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch record
 * @param   pTargetGC   The target of the fixup (i.e. the patch code we're
 *                      calling into).
 * @param   pCacheRec   Guest translation cache record
 * @param   fAddFixup   Whether to add a fixup record.
 */
static int patmGenCallToPatch(PVM pVM, PPATCHINFO pPatch, RTRCPTR pTargetGC, PPATMP2GLOOKUPREC pCacheRec, bool fAddFixup = true)
{
    uint8_t  temp[8];
    uint8_t *pPB;
    int      rc;

    Assert(pPatch->cbPatchJump <= sizeof(temp));

    pPB = patmR3GCVirtToHCVirt(pVM, pCacheRec, pPatch->pPrivInstrGC);
    Assert(pPB);

    Assert(pPatch->cbPatchJump == SIZEOF_NEARJUMP32);

    // jmp [PatchCode]
    if (fAddFixup)
    {
        if (patmPatchAddReloc32(pVM, pPatch, &pPB[1], FIXUP_REL_JMPTOPATCH,
                                pPatch->pPrivInstrGC + SIZEOF_NEARJUMP32, pTargetGC) != VINF_SUCCESS)
        {
            Log(("Relocation failed for the jump in the guest code!!\n"));
            return VERR_PATCHING_REFUSED;
        }
    }

    Assert(pPatch->aPrivInstr[0] == 0xE8 || pPatch->aPrivInstr[0] == 0xE9); /* call or jmp */
    temp[0] = pPatch->aPrivInstr[0];
    *(uint32_t *)&temp[1] = (uint32_t)pTargetGC - ((uint32_t)pPatch->pPrivInstrGC + SIZEOF_NEARJUMP32);    //return address

    rc = PGMPhysSimpleDirtyWriteGCPtr(VMMGetCpu0(pVM), pPatch->pPrivInstrGC, temp, pPatch->cbPatchJump);
    AssertRC(rc);

    return rc;
}


/**
 * Patch cli/sti pushf/popf instruction block at specified location
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context point to privileged instruction
 * @param   pInstrHC    Host context point to privileged instruction
 * @param   uOpcode     Instruction opcode
 * @param   uOpSize     Size of starting instruction
 * @param   pPatchRec   Patch record
 *
 * @note    returns failure if patching is not allowed or possible
 *
 */
static int patmR3PatchBlock(PVM pVM, RTRCPTR pInstrGC, R3PTRTYPE(uint8_t *) pInstrHC,
                            uint32_t uOpcode, uint32_t uOpSize, PPATMPATCHREC pPatchRec)
{
    PPATCHINFO pPatch = &pPatchRec->patch;
    int rc = VERR_PATCHING_REFUSED;
    uint32_t orgOffsetPatchMem = UINT32_MAX;
    RTRCPTR pInstrStart;
    bool fInserted;
    NOREF(pInstrHC); NOREF(uOpSize);

    /* Save original offset (in case of failures later on) */
    /** @todo use the hypervisor heap (that has quite a few consequences for save/restore though) */
    orgOffsetPatchMem = pVM->patm.s.offPatchMem;

    Assert(!(pPatch->flags & (PATMFL_GUEST_SPECIFIC|PATMFL_USER_MODE|PATMFL_TRAPHANDLER)));
    switch (uOpcode)
    {
    case OP_MOV:
        break;

    case OP_CLI:
    case OP_PUSHF:
        /* We can 'call' a cli or pushf patch. It will either return to the original guest code when IF is set again, or fault. */
        /* Note: special precautions are taken when disabling and enabling such patches. */
        pPatch->flags |= PATMFL_CALLABLE_AS_FUNCTION;
        break;

    default:
        if (!(pPatch->flags & PATMFL_IDTHANDLER))
        {
            AssertMsg(0, ("patmR3PatchBlock: Invalid opcode %x\n", uOpcode));
            return VERR_INVALID_PARAMETER;
        }
    }

    if (!(pPatch->flags & (PATMFL_IDTHANDLER|PATMFL_IDTHANDLER_WITHOUT_ENTRYPOINT|PATMFL_SYSENTER|PATMFL_INT3_REPLACEMENT_BLOCK)))
        pPatch->flags |= PATMFL_MUST_INSTALL_PATCHJMP;

    /* If we're going to insert a patch jump, then the jump itself is not allowed to cross a page boundary. */
    if (     (pPatch->flags & PATMFL_MUST_INSTALL_PATCHJMP)
        &&   PAGE_ADDRESS(pInstrGC) != PAGE_ADDRESS(pInstrGC + SIZEOF_NEARJUMP32)
       )
    {
        STAM_COUNTER_INC(&pVM->patm.s.StatPageBoundaryCrossed);
        Log(("Patch jump would cross page boundary -> refuse!!\n"));
        rc = VERR_PATCHING_REFUSED;
        goto failure;
    }

    pPatch->nrPatch2GuestRecs = 0;
    pInstrStart = pInstrGC;

#ifdef PATM_ENABLE_CALL
    pPatch->flags |= PATMFL_SUPPORT_CALLS | PATMFL_SUPPORT_INDIRECT_CALLS;
#endif

    pPatch->pPatchBlockOffset = pVM->patm.s.offPatchMem;
    pPatch->uCurPatchOffset = 0;

    if ((pPatch->flags & (PATMFL_IDTHANDLER|PATMFL_IDTHANDLER_WITHOUT_ENTRYPOINT|PATMFL_SYSENTER)) == PATMFL_IDTHANDLER)
    {
        Assert(pPatch->flags & PATMFL_INTHANDLER);

        /* Install fake cli patch (to clear the virtual IF and check int xx parameters) */
        rc = patmPatchGenIntEntry(pVM, pPatch, pInstrGC);
        if (RT_FAILURE(rc))
            goto failure;
    }

    /***************************************************************************************************************************/
    /* Note: We can't insert *any* code before a sysenter handler; some linux guests have an invalid stack at this point!!!!!  */
    /***************************************************************************************************************************/
#ifdef VBOX_WITH_STATISTICS
    if (!(pPatch->flags & PATMFL_SYSENTER))
    {
        rc = patmPatchGenStats(pVM, pPatch, pInstrGC);
        if (RT_FAILURE(rc))
            goto failure;
    }
#endif

    PATMP2GLOOKUPREC cacheRec;
    RT_ZERO(cacheRec);
    cacheRec.pPatch = pPatch;

    rc = patmRecompileCodeStream(pVM, pInstrGC, pInstrGC, patmRecompileCallback, &cacheRec);
    /* Free leftover lock if any. */
    if (cacheRec.Lock.pvMap)
    {
        PGMPhysReleasePageMappingLock(pVM, &cacheRec.Lock);
        cacheRec.Lock.pvMap = NULL;
    }
    if (rc != VINF_SUCCESS)
    {
        Log(("PATMR3PatchCli: patmRecompileCodeStream failed with %d\n", rc));
        goto failure;
    }

    /* Calculated during analysis. */
    if (pPatch->cbPatchBlockSize < SIZEOF_NEARJUMP32)
    {
        /* Most likely cause: we encountered an illegal instruction very early on. */
        /** @todo could turn it into an int3 callable patch. */
        Log(("patmR3PatchBlock: patch block too small -> refuse\n"));
        rc = VERR_PATCHING_REFUSED;
        goto failure;
    }

    /* size of patch block */
    pPatch->cbPatchBlockSize = pPatch->uCurPatchOffset;


    /* Update free pointer in patch memory. */
    pVM->patm.s.offPatchMem += pPatch->cbPatchBlockSize;
    /* Round to next 8 byte boundary. */
    pVM->patm.s.offPatchMem = RT_ALIGN_32(pVM->patm.s.offPatchMem, 8);

    /*
     * Insert into patch to guest lookup tree
     */
    LogFlow(("Insert %RRv patch offset %RRv\n", pPatchRec->patch.pPrivInstrGC, pPatch->pPatchBlockOffset));
    pPatchRec->CoreOffset.Key = pPatch->pPatchBlockOffset;
    fInserted = RTAvloU32Insert(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPatchAddr, &pPatchRec->CoreOffset);
    AssertMsg(fInserted, ("RTAvlULInsert failed for %x\n", pPatchRec->CoreOffset.Key));
    if (!fInserted)
    {
        rc = VERR_PATCHING_REFUSED;
        goto failure;
    }

    /* Note that patmr3SetBranchTargets can install additional patches!! */
    rc = patmr3SetBranchTargets(pVM, pPatch);
    if (rc != VINF_SUCCESS)
    {
        Log(("PATMR3PatchCli: patmr3SetBranchTargets failed with %d\n", rc));
        goto failure;
    }

#ifdef LOG_ENABLED
    Log(("Patch code ----------------------------------------------------------\n"));
    patmr3DisasmCodeStream(pVM, PATCHCODE_PTR_GC(pPatch), PATCHCODE_PTR_GC(pPatch), patmR3DisasmCallback, &cacheRec);
    /* Free leftover lock if any. */
    if (cacheRec.Lock.pvMap)
    {
        PGMPhysReleasePageMappingLock(pVM, &cacheRec.Lock);
        cacheRec.Lock.pvMap = NULL;
    }
    Log(("Patch code ends -----------------------------------------------------\n"));
#endif

    /* make a copy of the guest code bytes that will be overwritten */
    pPatch->cbPatchJump = SIZEOF_NEARJUMP32;

    rc = PGMPhysSimpleReadGCPtr(VMMGetCpu0(pVM), pPatch->aPrivInstr, pPatch->pPrivInstrGC, pPatch->cbPatchJump);
    AssertRC(rc);

    if (pPatch->flags & PATMFL_INT3_REPLACEMENT_BLOCK)
    {
        /*uint8_t bASMInt3 = 0xCC; - unused */

        Log(("patmR3PatchBlock %RRv -> int 3 callable patch.\n", pPatch->pPrivInstrGC));
        /* Replace first opcode byte with 'int 3'. */
        rc = patmActivateInt3Patch(pVM, pPatch);
        if (RT_FAILURE(rc))
            goto failure;

        /* normal patch can be turned into an int3 patch -> clear patch jump installation flag. */
        pPatch->flags &= ~PATMFL_MUST_INSTALL_PATCHJMP;

        pPatch->flags &= ~PATMFL_INSTR_HINT;
        STAM_COUNTER_INC(&pVM->patm.s.StatInt3Callable);
    }
    else
    if (pPatch->flags & PATMFL_MUST_INSTALL_PATCHJMP)
    {
        Assert(!(pPatch->flags & (PATMFL_IDTHANDLER|PATMFL_IDTHANDLER_WITHOUT_ENTRYPOINT|PATMFL_SYSENTER|PATMFL_INT3_REPLACEMENT_BLOCK)));
        /* now insert a jump in the guest code */
        rc = patmGenJumpToPatch(pVM, pPatch, &cacheRec, true);
        AssertRC(rc);
        if (RT_FAILURE(rc))
            goto failure;

    }

    patmR3DbgAddPatch(pVM, pPatchRec);

    PATM_LOG_RAW_PATCH_INSTR(pVM, pPatch, patmGetInstructionString(pPatch->opcode, pPatch->flags));

    patmEmptyTree(pVM, &pPatch->pTempInfo->IllegalInstrTree);
    pPatch->pTempInfo->nrIllegalInstr = 0;

    Log(("Successfully installed %s patch at %RRv\n", patmGetInstructionString(pPatch->opcode, pPatch->flags), pInstrGC));

    pPatch->uState = PATCH_ENABLED;
    return VINF_SUCCESS;

failure:
    if (pPatchRec->CoreOffset.Key)
        RTAvloU32Remove(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPatchAddr, pPatchRec->CoreOffset.Key);

    patmEmptyTree(pVM, &pPatch->FixupTree);
    pPatch->nrFixups = 0;

    patmEmptyTree(pVM, &pPatch->JumpTree);
    pPatch->nrJumpRecs = 0;

    patmEmptyTree(pVM, &pPatch->pTempInfo->IllegalInstrTree);
    pPatch->pTempInfo->nrIllegalInstr = 0;

    /* Turn this cli patch into a dummy. */
    pPatch->uState = PATCH_REFUSED;
    pPatch->pPatchBlockOffset = 0;

    // Give back the patch memory we no longer need
    Assert(orgOffsetPatchMem != (uint32_t)~0);
    pVM->patm.s.offPatchMem = orgOffsetPatchMem;

    return rc;
}

/**
 * Patch IDT handler
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context point to privileged instruction
 * @param   uOpSize     Size of starting instruction
 * @param   pPatchRec   Patch record
 * @param   pCacheRec   Cache record ptr
 *
 * @note    returns failure if patching is not allowed or possible
 *
 */
static int patmIdtHandler(PVM pVM, RTRCPTR pInstrGC, uint32_t uOpSize, PPATMPATCHREC pPatchRec, PPATMP2GLOOKUPREC pCacheRec)
{
    PPATCHINFO pPatch = &pPatchRec->patch;
    bool disret;
    DISCPUSTATE cpuPush, cpuJmp;
    uint32_t cbInstr;
    RTRCPTR  pCurInstrGC = pInstrGC;
    uint8_t *pCurInstrHC, *pInstrHC;
    uint32_t orgOffsetPatchMem = UINT32_MAX;

    pInstrHC = pCurInstrHC = patmR3GCVirtToHCVirt(pVM, pCacheRec, pCurInstrGC);
    AssertReturn(pCurInstrHC, VERR_PAGE_NOT_PRESENT);

    /*
     * In Linux it's often the case that many interrupt handlers push a predefined value onto the stack
     * and then jump to a common entrypoint. In order not to waste a lot of memory, we will check for this
     * condition here and only patch the common entypoint once.
     */
    disret = patmR3DisInstr(pVM, pPatch, pCurInstrGC, pCurInstrHC, PATMREAD_ORGCODE, &cpuPush, &cbInstr);
    Assert(disret);
    if (disret && cpuPush.pCurInstr->uOpcode == OP_PUSH)
    {
        RTRCPTR  pJmpInstrGC;
        int      rc;
        pCurInstrGC += cbInstr;

        disret = patmR3DisInstr(pVM, pPatch, pCurInstrGC, pCurInstrHC, PATMREAD_ORGCODE, &cpuJmp, &cbInstr);
        if (   disret
            && cpuJmp.pCurInstr->uOpcode == OP_JMP
            && (pJmpInstrGC = PATMResolveBranch(&cpuJmp, pCurInstrGC))
           )
        {
            bool fInserted;
            PPATMPATCHREC pJmpPatch = (PPATMPATCHREC)RTAvloU32Get(&pVM->patm.s.PatchLookupTreeHC->PatchTree, pJmpInstrGC);
            if (pJmpPatch == 0)
            {
                /* Patch it first! */
                rc = PATMR3InstallPatch(pVM, pJmpInstrGC, pPatch->flags | PATMFL_IDTHANDLER_WITHOUT_ENTRYPOINT);
                if (rc != VINF_SUCCESS)
                    goto failure;
                pJmpPatch = (PPATMPATCHREC)RTAvloU32Get(&pVM->patm.s.PatchLookupTreeHC->PatchTree, pJmpInstrGC);
                Assert(pJmpPatch);
            }
            if (pJmpPatch->patch.uState != PATCH_ENABLED)
                goto failure;

            /* save original offset (in case of failures later on) */
            orgOffsetPatchMem = pVM->patm.s.offPatchMem;

            pPatch->pPatchBlockOffset = pVM->patm.s.offPatchMem;
            pPatch->uCurPatchOffset   = 0;
            pPatch->nrPatch2GuestRecs = 0;

#ifdef VBOX_WITH_STATISTICS
            rc = patmPatchGenStats(pVM, pPatch, pInstrGC);
            if (RT_FAILURE(rc))
                goto failure;
#endif

            /* Install fake cli patch (to clear the virtual IF) */
            rc = patmPatchGenIntEntry(pVM, pPatch, pInstrGC);
            if (RT_FAILURE(rc))
                goto failure;

            /* Add lookup record for patch to guest address translation (for the push) */
            patmR3AddP2GLookupRecord(pVM, pPatch, PATCHCODE_PTR_HC(pPatch) + pPatch->uCurPatchOffset, pInstrGC, PATM_LOOKUP_BOTHDIR);

            /* Duplicate push. */
            rc = patmPatchGenDuplicate(pVM, pPatch, &cpuPush, pInstrGC);
            if (RT_FAILURE(rc))
                goto failure;

            /* Generate jump to common entrypoint. */
            rc = patmPatchGenPatchJump(pVM, pPatch, pCurInstrGC, PATCHCODE_PTR_GC(&pJmpPatch->patch));
            if (RT_FAILURE(rc))
                goto failure;

            /* size of patch block */
            pPatch->cbPatchBlockSize = pPatch->uCurPatchOffset;

            /* Update free pointer in patch memory. */
            pVM->patm.s.offPatchMem += pPatch->cbPatchBlockSize;
            /* Round to next 8 byte boundary */
            pVM->patm.s.offPatchMem = RT_ALIGN_32(pVM->patm.s.offPatchMem, 8);

            /* There's no jump from guest to patch code. */
            pPatch->cbPatchJump = 0;


#ifdef LOG_ENABLED
            Log(("Patch code ----------------------------------------------------------\n"));
            patmr3DisasmCodeStream(pVM, PATCHCODE_PTR_GC(pPatch), PATCHCODE_PTR_GC(pPatch), patmR3DisasmCallback, pCacheRec);
            Log(("Patch code ends -----------------------------------------------------\n"));
#endif
            Log(("Successfully installed IDT handler patch at %RRv\n", pInstrGC));

            /*
             * Insert into patch to guest lookup tree
             */
            LogFlow(("Insert %RRv patch offset %RRv\n", pPatchRec->patch.pPrivInstrGC, pPatch->pPatchBlockOffset));
            pPatchRec->CoreOffset.Key = pPatch->pPatchBlockOffset;
            fInserted = RTAvloU32Insert(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPatchAddr, &pPatchRec->CoreOffset);
            AssertMsg(fInserted, ("RTAvlULInsert failed for %x\n", pPatchRec->CoreOffset.Key));
            patmR3DbgAddPatch(pVM, pPatchRec);

            pPatch->uState = PATCH_ENABLED;

            return VINF_SUCCESS;
        }
    }
failure:
    /* Give back the patch memory we no longer need */
    if (orgOffsetPatchMem != (uint32_t)~0)
        pVM->patm.s.offPatchMem = orgOffsetPatchMem;

    return patmR3PatchBlock(pVM, pInstrGC, pInstrHC, OP_CLI, uOpSize, pPatchRec);
}

/**
 * Install a trampoline to call a guest trap handler directly
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context point to privileged instruction
 * @param   pPatchRec   Patch record
 * @param   pCacheRec   Cache record ptr
 *
 */
static int patmInstallTrapTrampoline(PVM pVM, RTRCPTR pInstrGC, PPATMPATCHREC pPatchRec, PPATMP2GLOOKUPREC pCacheRec)
{
    PPATCHINFO pPatch = &pPatchRec->patch;
    int rc = VERR_PATCHING_REFUSED;
    uint32_t orgOffsetPatchMem = UINT32_MAX;
    bool fInserted;

    // save original offset (in case of failures later on)
    orgOffsetPatchMem = pVM->patm.s.offPatchMem;

    pPatch->pPatchBlockOffset = pVM->patm.s.offPatchMem;
    pPatch->uCurPatchOffset   = 0;
    pPatch->nrPatch2GuestRecs = 0;

#ifdef VBOX_WITH_STATISTICS
    rc = patmPatchGenStats(pVM, pPatch, pInstrGC);
    if (RT_FAILURE(rc))
        goto failure;
#endif

    rc = patmPatchGenTrapEntry(pVM, pPatch, pInstrGC);
    if (RT_FAILURE(rc))
        goto failure;

    /* size of patch block */
    pPatch->cbPatchBlockSize = pPatch->uCurPatchOffset;

    /* Update free pointer in patch memory. */
    pVM->patm.s.offPatchMem += pPatch->cbPatchBlockSize;
    /* Round to next 8 byte boundary */
    pVM->patm.s.offPatchMem = RT_ALIGN_32(pVM->patm.s.offPatchMem, 8);

    /* There's no jump from guest to patch code. */
    pPatch->cbPatchJump = 0;

#ifdef LOG_ENABLED
    Log(("Patch code ----------------------------------------------------------\n"));
    patmr3DisasmCodeStream(pVM, PATCHCODE_PTR_GC(pPatch), PATCHCODE_PTR_GC(pPatch), patmR3DisasmCallback, pCacheRec);
    Log(("Patch code ends -----------------------------------------------------\n"));
#else
    RT_NOREF_PV(pCacheRec);
#endif
    PATM_LOG_ORG_PATCH_INSTR(pVM, pPatch, "TRAP handler");
    Log(("Successfully installed Trap Trampoline patch at %RRv\n", pInstrGC));

    /*
     * Insert into patch to guest lookup tree
     */
    LogFlow(("Insert %RRv patch offset %RRv\n", pPatchRec->patch.pPrivInstrGC, pPatch->pPatchBlockOffset));
    pPatchRec->CoreOffset.Key = pPatch->pPatchBlockOffset;
    fInserted = RTAvloU32Insert(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPatchAddr, &pPatchRec->CoreOffset);
    AssertMsg(fInserted, ("RTAvlULInsert failed for %x\n", pPatchRec->CoreOffset.Key));
    patmR3DbgAddPatch(pVM, pPatchRec);

    pPatch->uState = PATCH_ENABLED;
    return VINF_SUCCESS;

failure:
    AssertMsgFailed(("Failed to install trap handler trampoline!!\n"));

    /* Turn this cli patch into a dummy. */
    pPatch->uState = PATCH_REFUSED;
    pPatch->pPatchBlockOffset = 0;

    /* Give back the patch memory we no longer need */
    Assert(orgOffsetPatchMem != (uint32_t)~0);
    pVM->patm.s.offPatchMem = orgOffsetPatchMem;

    return rc;
}


#ifdef LOG_ENABLED
/**
 * Check if the instruction is patched as a common idt handler
 *
 * @returns true or false
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context point to the instruction
 *
 */
static bool patmIsCommonIDTHandlerPatch(PVM pVM, RTRCPTR pInstrGC)
{
    PPATMPATCHREC pRec;

    pRec = (PPATMPATCHREC)RTAvloU32Get(&pVM->patm.s.PatchLookupTreeHC->PatchTree, pInstrGC);
    if (pRec && pRec->patch.flags & PATMFL_IDTHANDLER_WITHOUT_ENTRYPOINT)
        return true;
    return false;
}
#endif //DEBUG


/**
 * Duplicates a complete function
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context point to privileged instruction
 * @param   pPatchRec   Patch record
 * @param   pCacheRec   Cache record ptr
 *
 */
static int patmDuplicateFunction(PVM pVM, RTRCPTR pInstrGC, PPATMPATCHREC pPatchRec, PPATMP2GLOOKUPREC pCacheRec)
{
    PPATCHINFO pPatch = &pPatchRec->patch;
    int rc = VERR_PATCHING_REFUSED;
    uint32_t orgOffsetPatchMem = UINT32_MAX;
    bool fInserted;

    Log(("patmDuplicateFunction %RRv\n", pInstrGC));
    /* Save original offset (in case of failures later on). */
    orgOffsetPatchMem = pVM->patm.s.offPatchMem;

    /* We will not go on indefinitely with call instruction handling. */
    if (pVM->patm.s.ulCallDepth > PATM_MAX_CALL_DEPTH)
    {
        Log(("patmDuplicateFunction: maximum callback depth reached!!\n"));
        return VERR_PATCHING_REFUSED;
    }

    pVM->patm.s.ulCallDepth++;

#ifdef PATM_ENABLE_CALL
    pPatch->flags |= PATMFL_SUPPORT_CALLS | PATMFL_SUPPORT_INDIRECT_CALLS;
#endif

    Assert(pPatch->flags & (PATMFL_DUPLICATE_FUNCTION));

    pPatch->nrPatch2GuestRecs = 0;
    pPatch->pPatchBlockOffset = pVM->patm.s.offPatchMem;
    pPatch->uCurPatchOffset   = 0;

    /* Note: Set the PATM interrupt flag here; it was cleared before the patched call. (!!!) */
    rc = patmPatchGenSetPIF(pVM, pPatch, pInstrGC);
    if (RT_FAILURE(rc))
        goto failure;

#ifdef VBOX_WITH_STATISTICS
    rc = patmPatchGenStats(pVM, pPatch, pInstrGC);
    if (RT_FAILURE(rc))
        goto failure;
#endif

    rc = patmRecompileCodeStream(pVM, pInstrGC, pInstrGC, patmRecompileCallback, pCacheRec);
    if (rc != VINF_SUCCESS)
    {
        Log(("PATMR3PatchCli: patmRecompileCodeStream failed with %d\n", rc));
        goto failure;
    }

    //size of patch block
    pPatch->cbPatchBlockSize = pPatch->uCurPatchOffset;

    //update free pointer in patch memory
    pVM->patm.s.offPatchMem += pPatch->cbPatchBlockSize;
    /* Round to next 8 byte boundary. */
    pVM->patm.s.offPatchMem = RT_ALIGN_32(pVM->patm.s.offPatchMem, 8);

    pPatch->uState = PATCH_ENABLED;

    /*
     * Insert into patch to guest lookup tree
     */
    LogFlow(("Insert %RRv patch offset %RRv\n", pPatchRec->patch.pPrivInstrGC, pPatch->pPatchBlockOffset));
    pPatchRec->CoreOffset.Key = pPatch->pPatchBlockOffset;
    fInserted = RTAvloU32Insert(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPatchAddr, &pPatchRec->CoreOffset);
    AssertMsg(fInserted, ("RTAvloU32Insert failed for %x\n", pPatchRec->CoreOffset.Key));
    if (!fInserted)
    {
        rc = VERR_PATCHING_REFUSED;
        goto failure;
    }

    /* Note that patmr3SetBranchTargets can install additional patches!! */
    rc = patmr3SetBranchTargets(pVM, pPatch);
    if (rc != VINF_SUCCESS)
    {
        Log(("PATMR3PatchCli: patmr3SetBranchTargets failed with %d\n", rc));
        goto failure;
    }

    patmR3DbgAddPatch(pVM, pPatchRec);

#ifdef LOG_ENABLED
    Log(("Patch code ----------------------------------------------------------\n"));
    patmr3DisasmCodeStream(pVM, PATCHCODE_PTR_GC(pPatch), PATCHCODE_PTR_GC(pPatch), patmR3DisasmCallback, pCacheRec);
    Log(("Patch code ends -----------------------------------------------------\n"));
#endif

    Log(("Successfully installed function duplication patch at %RRv\n", pInstrGC));

    patmEmptyTree(pVM, &pPatch->pTempInfo->IllegalInstrTree);
    pPatch->pTempInfo->nrIllegalInstr = 0;

    pVM->patm.s.ulCallDepth--;
    STAM_COUNTER_INC(&pVM->patm.s.StatInstalledFunctionPatches);
    return VINF_SUCCESS;

failure:
    if (pPatchRec->CoreOffset.Key)
        RTAvloU32Remove(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPatchAddr, pPatchRec->CoreOffset.Key);

    patmEmptyTree(pVM, &pPatch->FixupTree);
    pPatch->nrFixups = 0;

    patmEmptyTree(pVM, &pPatch->JumpTree);
    pPatch->nrJumpRecs = 0;

    patmEmptyTree(pVM, &pPatch->pTempInfo->IllegalInstrTree);
    pPatch->pTempInfo->nrIllegalInstr = 0;

    /* Turn this cli patch into a dummy. */
    pPatch->uState = PATCH_REFUSED;
    pPatch->pPatchBlockOffset = 0;

    // Give back the patch memory we no longer need
    Assert(orgOffsetPatchMem != (uint32_t)~0);
    pVM->patm.s.offPatchMem = orgOffsetPatchMem;

    pVM->patm.s.ulCallDepth--;
    Log(("patmDupicateFunction %RRv failed!!\n", pInstrGC));
    return rc;
}

/**
 * Creates trampoline code to jump inside an existing patch
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context point to privileged instruction
 * @param   pPatchRec   Patch record
 *
 */
static int patmCreateTrampoline(PVM pVM, RTRCPTR pInstrGC, PPATMPATCHREC pPatchRec)
{
    PPATCHINFO  pPatch = &pPatchRec->patch;
    RTRCPTR     pPage, pPatchTargetGC = 0;
    uint32_t    orgOffsetPatchMem = UINT32_MAX;
    int         rc = VERR_PATCHING_REFUSED;
    PPATCHINFO  pPatchToJmp = NULL; /**< Patch the trampoline jumps to. */
    PTRAMPREC   pTrampRec = NULL; /**< Trampoline record used to find the patch. */
    bool        fInserted = false;

    Log(("patmCreateTrampoline %RRv\n", pInstrGC));
    /* Save original offset (in case of failures later on). */
    orgOffsetPatchMem = pVM->patm.s.offPatchMem;

    /* First we check if the duplicate function target lies in some existing function patch already. Will save some space. */
    /** @todo we already checked this before */
    pPage = pInstrGC & PAGE_BASE_GC_MASK;

    PPATMPATCHPAGE pPatchPage = (PPATMPATCHPAGE)RTAvloU32Get(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPage, (RTRCPTR)pPage);
    if (pPatchPage)
    {
        uint32_t i;

        for (i=0;i<pPatchPage->cCount;i++)
        {
            if (pPatchPage->papPatch[i])
            {
                pPatchToJmp = pPatchPage->papPatch[i];

                if (    (pPatchToJmp->flags & PATMFL_DUPLICATE_FUNCTION)
                    &&  pPatchToJmp->uState == PATCH_ENABLED)
                {
                    pPatchTargetGC = patmGuestGCPtrToPatchGCPtr(pVM, pPatchToJmp, pInstrGC);
                    if (pPatchTargetGC)
                    {
                        uint32_t         offsetPatch      = pPatchTargetGC - pVM->patm.s.pPatchMemGC;
                        PRECPATCHTOGUEST pPatchToGuestRec = (PRECPATCHTOGUEST)RTAvlU32GetBestFit(&pPatchToJmp->Patch2GuestAddrTree, offsetPatch, false);
                        Assert(pPatchToGuestRec);

                        pPatchToGuestRec->fJumpTarget = true;
                        Assert(pPatchTargetGC != pPatchToJmp->pPrivInstrGC);
                        Log(("patmCreateTrampoline: generating jump to code inside patch at %RRv (patch target %RRv)\n", pPatchToJmp->pPrivInstrGC, pPatchTargetGC));
                        break;
                    }
                }
            }
        }
    }
    AssertReturn(pPatchPage && pPatchTargetGC && pPatchToJmp, VERR_PATCHING_REFUSED);

    /*
     * Only record the trampoline patch if this is the first patch to the target
     * or we recorded other patches already.
     * The goal is to refuse refreshing function duplicates if the guest
     * modifies code after a saved state was loaded because it is not possible
     * to save the relation between trampoline and target without changing the
     * saved satte version.
     */
    if (   !(pPatchToJmp->flags & PATMFL_EXTERNAL_JUMP_INSIDE)
        || pPatchToJmp->pTrampolinePatchesHead)
    {
        pPatchToJmp->flags |= PATMFL_EXTERNAL_JUMP_INSIDE;
        pTrampRec = (PTRAMPREC)MMR3HeapAllocZ(pVM, MM_TAG_PATM_PATCH, sizeof(*pTrampRec));
        if (!pTrampRec)
            return VERR_NO_MEMORY; /* or better return VERR_PATCHING_REFUSED to let the VM continue? */

        pTrampRec->pPatchTrampoline = pPatchRec;
    }

    pPatch->nrPatch2GuestRecs = 0;
    pPatch->pPatchBlockOffset = pVM->patm.s.offPatchMem;
    pPatch->uCurPatchOffset   = 0;

    /* Note: Set the PATM interrupt flag here; it was cleared before the patched call. (!!!) */
    rc = patmPatchGenSetPIF(pVM, pPatch, pInstrGC);
    if (RT_FAILURE(rc))
        goto failure;

#ifdef VBOX_WITH_STATISTICS
    rc = patmPatchGenStats(pVM, pPatch, pInstrGC);
    if (RT_FAILURE(rc))
        goto failure;
#endif

    rc = patmPatchGenPatchJump(pVM, pPatch, pInstrGC, pPatchTargetGC);
    if (RT_FAILURE(rc))
        goto failure;

    /*
     * Insert into patch to guest lookup tree
     */
    LogFlow(("Insert %RRv patch offset %RRv\n", pPatchRec->patch.pPrivInstrGC, pPatch->pPatchBlockOffset));
    pPatchRec->CoreOffset.Key = pPatch->pPatchBlockOffset;
    fInserted = RTAvloU32Insert(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPatchAddr, &pPatchRec->CoreOffset);
    AssertMsg(fInserted, ("RTAvloU32Insert failed for %x\n", pPatchRec->CoreOffset.Key));
    if (!fInserted)
    {
        rc = VERR_PATCHING_REFUSED;
        goto failure;
    }
    patmR3DbgAddPatch(pVM, pPatchRec);

    /* size of patch block */
    pPatch->cbPatchBlockSize = pPatch->uCurPatchOffset;

    /* Update free pointer in patch memory. */
    pVM->patm.s.offPatchMem += pPatch->cbPatchBlockSize;
    /* Round to next 8 byte boundary */
    pVM->patm.s.offPatchMem = RT_ALIGN_32(pVM->patm.s.offPatchMem, 8);

    /* There's no jump from guest to patch code. */
    pPatch->cbPatchJump = 0;

    /* Enable the patch. */
    pPatch->uState = PATCH_ENABLED;
    /* We allow this patch to be called as a function. */
    pPatch->flags |= PATMFL_CALLABLE_AS_FUNCTION;

    if (pTrampRec)
    {
        pTrampRec->pNext = pPatchToJmp->pTrampolinePatchesHead;
        pPatchToJmp->pTrampolinePatchesHead = pTrampRec;
    }
    STAM_COUNTER_INC(&pVM->patm.s.StatInstalledTrampoline);
    return VINF_SUCCESS;

failure:
    if (pPatchRec->CoreOffset.Key)
        RTAvloU32Remove(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPatchAddr, pPatchRec->CoreOffset.Key);

    patmEmptyTree(pVM, &pPatch->FixupTree);
    pPatch->nrFixups = 0;

    patmEmptyTree(pVM, &pPatch->JumpTree);
    pPatch->nrJumpRecs = 0;

    patmEmptyTree(pVM, &pPatch->pTempInfo->IllegalInstrTree);
    pPatch->pTempInfo->nrIllegalInstr = 0;

    /* Turn this cli patch into a dummy. */
    pPatch->uState = PATCH_REFUSED;
    pPatch->pPatchBlockOffset = 0;

    // Give back the patch memory we no longer need
    Assert(orgOffsetPatchMem != (uint32_t)~0);
    pVM->patm.s.offPatchMem = orgOffsetPatchMem;

    if (pTrampRec)
        MMR3HeapFree(pTrampRec);

    return rc;
}


/**
 * Patch branch target function for call/jump at specified location.
 * (in responds to a VINF_PATM_DUPLICATE_FUNCTION GC exit reason)
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pCtx        Pointer to the guest CPU context.
 *
 */
VMMR3_INT_DECL(int) PATMR3DuplicateFunctionRequest(PVM pVM, PCPUMCTX pCtx)
{
    RTRCPTR     pBranchTarget, pPage;
    int         rc;
    RTRCPTR     pPatchTargetGC = 0;
    AssertReturn(!HMIsEnabled(pVM), VERR_PATM_HM_IPE);

    pBranchTarget = pCtx->edx;
    pBranchTarget = SELMToFlat(pVM, DISSELREG_CS, CPUMCTX2CORE(pCtx), pBranchTarget);

    /* First we check if the duplicate function target lies in some existing function patch already. Will save some space. */
    pPage = pBranchTarget & PAGE_BASE_GC_MASK;

    PPATMPATCHPAGE pPatchPage = (PPATMPATCHPAGE)RTAvloU32Get(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPage, (RTRCPTR)pPage);
    if (pPatchPage)
    {
        uint32_t i;

        for (i=0;i<pPatchPage->cCount;i++)
        {
            if (pPatchPage->papPatch[i])
            {
                PPATCHINFO pPatch = pPatchPage->papPatch[i];

                if (    (pPatch->flags & PATMFL_DUPLICATE_FUNCTION)
                    &&  pPatch->uState == PATCH_ENABLED)
                {
                    pPatchTargetGC = patmGuestGCPtrToPatchGCPtr(pVM, pPatch, pBranchTarget);
                    if (pPatchTargetGC)
                    {
                        STAM_COUNTER_INC(&pVM->patm.s.StatDuplicateUseExisting);
                        break;
                    }
                }
            }
        }
    }

    if (pPatchTargetGC)
    {
        /* Create a trampoline that also sets PATM_ASMFIX_INTERRUPTFLAG. */
        rc = PATMR3InstallPatch(pVM, pBranchTarget, PATMFL_CODE32 | PATMFL_TRAMPOLINE);
    }
    else
    {
        rc = PATMR3InstallPatch(pVM, pBranchTarget, PATMFL_CODE32 | PATMFL_DUPLICATE_FUNCTION);
    }

    if (rc == VINF_SUCCESS)
    {
        pPatchTargetGC = PATMR3QueryPatchGCPtr(pVM, pBranchTarget);
        Assert(pPatchTargetGC);
    }

    if (pPatchTargetGC)
    {
        pCtx->eax = pPatchTargetGC;
        pCtx->eax = pCtx->eax - (RTRCUINTPTR)pVM->patm.s.pPatchMemGC;   /* make it relative */
    }
    else
    {
        /* We add a dummy entry into the lookup cache so we won't get bombarded with the same requests over and over again. */
        pCtx->eax = 0;
        STAM_COUNTER_INC(&pVM->patm.s.StatDuplicateREQFailed);
    }
    Assert(PATMIsPatchGCAddr(pVM, pCtx->edi));
    rc = patmAddBranchToLookupCache(pVM, pCtx->edi, pBranchTarget, pCtx->eax);
    AssertRC(rc);

    pCtx->eip += PATM_ILLEGAL_INSTR_SIZE;
    STAM_COUNTER_INC(&pVM->patm.s.StatDuplicateREQSuccess);
    return VINF_SUCCESS;
}

/**
 * Replaces a function call by a call to an existing function duplicate (or jmp -> jmp)
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pCpu        Disassembly CPU structure ptr
 * @param   pInstrGC    Guest context point to privileged instruction
 * @param   pCacheRec   Cache record ptr
 *
 */
static int patmReplaceFunctionCall(PVM pVM, DISCPUSTATE *pCpu, RTRCPTR pInstrGC, PPATMP2GLOOKUPREC pCacheRec)
{
    PPATCHINFO    pPatch = (PPATCHINFO)pCacheRec->pPatch;
    int           rc = VERR_PATCHING_REFUSED;
    DISCPUSTATE   cpu;
    RTRCPTR       pTargetGC;
    PPATMPATCHREC pPatchFunction;
    uint32_t      cbInstr;
    bool          disret;

    Assert(pPatch->flags & PATMFL_REPLACE_FUNCTION_CALL);
    Assert((pCpu->pCurInstr->uOpcode == OP_CALL || pCpu->pCurInstr->uOpcode == OP_JMP) && pCpu->cbInstr == SIZEOF_NEARJUMP32);

    if ((pCpu->pCurInstr->uOpcode != OP_CALL && pCpu->pCurInstr->uOpcode != OP_JMP) || pCpu->cbInstr != SIZEOF_NEARJUMP32)
    {
        rc = VERR_PATCHING_REFUSED;
        goto failure;
    }

    pTargetGC = PATMResolveBranch(pCpu, pInstrGC);
    if (pTargetGC == 0)
    {
        Log(("We don't support far jumps here!! (%08X)\n", pCpu->Param1.fUse));
        rc = VERR_PATCHING_REFUSED;
        goto failure;
    }

    pPatchFunction = (PPATMPATCHREC)RTAvloU32Get(&pVM->patm.s.PatchLookupTreeHC->PatchTree, pTargetGC);
    if (pPatchFunction == NULL)
    {
        for(;;)
        {
            /* It could be an indirect call (call -> jmp dest).
             * Note that it's dangerous to assume the jump will never change...
             */
            uint8_t *pTmpInstrHC;

            pTmpInstrHC = patmR3GCVirtToHCVirt(pVM, pCacheRec, pTargetGC);
            Assert(pTmpInstrHC);
            if (pTmpInstrHC == 0)
                break;

            disret = patmR3DisInstr(pVM, pPatch, pTargetGC, pTmpInstrHC, PATMREAD_ORGCODE, &cpu, &cbInstr);
            if (disret == false || cpu.pCurInstr->uOpcode != OP_JMP)
                break;

            pTargetGC = PATMResolveBranch(&cpu, pTargetGC);
            if (pTargetGC == 0)
            {
                break;
            }

            pPatchFunction = (PPATMPATCHREC)RTAvloU32Get(&pVM->patm.s.PatchLookupTreeHC->PatchTree, pTargetGC);
            break;
        }
        if (pPatchFunction == 0)
        {
            AssertMsgFailed(("Unable to find duplicate function %RRv\n", pTargetGC));
            rc = VERR_PATCHING_REFUSED;
            goto failure;
        }
    }

    // make a copy of the guest code bytes that will be overwritten
    pPatch->cbPatchJump = SIZEOF_NEARJUMP32;

    rc = PGMPhysSimpleReadGCPtr(VMMGetCpu0(pVM), pPatch->aPrivInstr, pPatch->pPrivInstrGC, pPatch->cbPatchJump);
    AssertRC(rc);

    /* Now replace the original call in the guest code */
    rc = patmGenCallToPatch(pVM, pPatch, PATCHCODE_PTR_GC(&pPatchFunction->patch), pCacheRec, true);
    AssertRC(rc);
    if (RT_FAILURE(rc))
        goto failure;

    /* Lowest and highest address for write monitoring. */
    pPatch->pInstrGCLowest  = pInstrGC;
    pPatch->pInstrGCHighest = pInstrGC + pCpu->cbInstr;
    PATM_LOG_ORG_PATCH_INSTR(pVM, pPatch, "Call");

    Log(("Successfully installed function replacement patch at %RRv\n", pInstrGC));

    pPatch->uState = PATCH_ENABLED;
    return VINF_SUCCESS;

failure:
    /* Turn this patch into a dummy. */
    pPatch->uState = PATCH_REFUSED;

    return rc;
}

/**
 * Replace the address in an MMIO instruction with the cached version.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context point to privileged instruction
 * @param   pCpu        Disassembly CPU structure ptr
 * @param   pCacheRec   Cache record ptr
 *
 * @note    returns failure if patching is not allowed or possible
 *
 */
static int patmPatchMMIOInstr(PVM pVM, RTRCPTR pInstrGC, DISCPUSTATE *pCpu, PPATMP2GLOOKUPREC pCacheRec)
{
    PPATCHINFO    pPatch = (PPATCHINFO)pCacheRec->pPatch;
    uint8_t      *pPB;
    int           rc = VERR_PATCHING_REFUSED;

    Assert(pVM->patm.s.mmio.pCachedData);
    if (!pVM->patm.s.mmio.pCachedData)
        goto failure;

    if (pCpu->Param2.fUse != DISUSE_DISPLACEMENT32)
        goto failure;

    pPB = patmR3GCVirtToHCVirt(pVM, pCacheRec, pPatch->pPrivInstrGC);
    if (pPB == 0)
        goto failure;

    /* Add relocation record for cached data access. */
    if (patmPatchAddReloc32(pVM, pPatch, &pPB[pCpu->cbInstr - sizeof(RTRCPTR)], FIXUP_ABSOLUTE, pPatch->pPrivInstrGC,
                            pVM->patm.s.mmio.pCachedData) != VINF_SUCCESS)
    {
        Log(("Relocation failed for cached mmio address!!\n"));
        return VERR_PATCHING_REFUSED;
    }
    PATM_LOG_PATCH_INSTR(pVM, pPatch, PATMREAD_ORGCODE, "MMIO patch old instruction:", "");

    /* Save original instruction. */
    rc = PGMPhysSimpleReadGCPtr(VMMGetCpu0(pVM), pPatch->aPrivInstr, pPatch->pPrivInstrGC, pPatch->cbPrivInstr);
    AssertRC(rc);

    pPatch->cbPatchJump = pPatch->cbPrivInstr;  /* bit of a misnomer in this case; size of replacement instruction. */

    /* Replace address with that of the cached item. */
    rc = PGMPhysSimpleDirtyWriteGCPtr(VMMGetCpu0(pVM), pInstrGC + pCpu->cbInstr - sizeof(RTRCPTR),
                                      &pVM->patm.s.mmio.pCachedData, sizeof(RTRCPTR));
    AssertRC(rc);
    if (RT_FAILURE(rc))
    {
        goto failure;
    }

    PATM_LOG_ORG_PATCH_INSTR(pVM, pPatch, "MMIO");
    pVM->patm.s.mmio.pCachedData = 0;
    pVM->patm.s.mmio.GCPhys = 0;
    pPatch->uState = PATCH_ENABLED;
    return VINF_SUCCESS;

failure:
    /* Turn this patch into a dummy. */
    pPatch->uState = PATCH_REFUSED;

    return rc;
}


/**
 * Replace the address in an MMIO instruction with the cached version. (instruction is part of an existing patch)
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context point to privileged instruction
 * @param   pPatch      Patch record
 *
 * @note    returns failure if patching is not allowed or possible
 *
 */
static int patmPatchPATMMMIOInstr(PVM pVM, RTRCPTR pInstrGC, PPATCHINFO pPatch)
{
    DISCPUSTATE   cpu;
    uint32_t      cbInstr;
    bool          disret;
    uint8_t      *pInstrHC;

    AssertReturn(pVM->patm.s.mmio.pCachedData, VERR_INVALID_PARAMETER);

    /* Convert GC to HC address. */
    pInstrHC = patmPatchGCPtr2PatchHCPtr(pVM, pInstrGC);
    AssertReturn(pInstrHC, VERR_PATCHING_REFUSED);

    /* Disassemble mmio instruction. */
    disret = patmR3DisInstrNoStrOpMode(pVM, pPatch, pInstrGC, pInstrHC, PATMREAD_ORGCODE,
                                       &cpu, &cbInstr);
    if (disret == false)
    {
        Log(("Disassembly failed (probably page not present) -> return to caller\n"));
        return VERR_PATCHING_REFUSED;
    }

    AssertMsg(cbInstr <= MAX_INSTR_SIZE, ("privileged instruction too big %d!!\n", cbInstr));
    if (cbInstr > MAX_INSTR_SIZE)
        return VERR_PATCHING_REFUSED;
    if (cpu.Param2.fUse != DISUSE_DISPLACEMENT32)
        return VERR_PATCHING_REFUSED;

    /* Add relocation record for cached data access. */
    if (patmPatchAddReloc32(pVM, pPatch, &pInstrHC[cpu.cbInstr - sizeof(RTRCPTR)], FIXUP_ABSOLUTE) != VINF_SUCCESS)
    {
        Log(("Relocation failed for cached mmio address!!\n"));
        return VERR_PATCHING_REFUSED;
    }
    /* Replace address with that of the cached item. */
    *(RTRCPTR *)&pInstrHC[cpu.cbInstr - sizeof(RTRCPTR)] = pVM->patm.s.mmio.pCachedData;

    /* Lowest and highest address for write monitoring. */
    pPatch->pInstrGCLowest  = pInstrGC;
    pPatch->pInstrGCHighest = pInstrGC + cpu.cbInstr;

    PATM_LOG_ORG_PATCH_INSTR(pVM, pPatch, "MMIO");
    pVM->patm.s.mmio.pCachedData = 0;
    pVM->patm.s.mmio.GCPhys = 0;
    return VINF_SUCCESS;
}

/**
 * Activates an int3 patch
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch record
 */
static int patmActivateInt3Patch(PVM pVM, PPATCHINFO pPatch)
{
    uint8_t bASMInt3 = 0xCC;
    int     rc;

    Assert(pPatch->flags & (PATMFL_INT3_REPLACEMENT|PATMFL_INT3_REPLACEMENT_BLOCK));
    Assert(pPatch->uState != PATCH_ENABLED);

    /* Replace first opcode byte with 'int 3'. */
    rc = PGMPhysSimpleDirtyWriteGCPtr(VMMGetCpu0(pVM), pPatch->pPrivInstrGC, &bASMInt3, sizeof(bASMInt3));
    AssertRC(rc);

    pPatch->cbPatchJump = sizeof(bASMInt3);

    return rc;
}

/**
 * Deactivates an int3 patch
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch record
 */
static int patmDeactivateInt3Patch(PVM pVM, PPATCHINFO pPatch)
{
    uint8_t cbASMInt3 = 1;
    int     rc;

    Assert(pPatch->flags & (PATMFL_INT3_REPLACEMENT|PATMFL_INT3_REPLACEMENT_BLOCK));
    Assert(pPatch->uState == PATCH_ENABLED || pPatch->uState == PATCH_DIRTY);

    /* Restore first opcode byte. */
    rc = PGMPhysSimpleDirtyWriteGCPtr(VMMGetCpu0(pVM), pPatch->pPrivInstrGC, pPatch->aPrivInstr, cbASMInt3);
    AssertRC(rc);
    return rc;
}

/**
 * Replace an instruction with a breakpoint (0xCC), that is handled dynamically
 * in the raw-mode context.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context point to privileged instruction
 * @param   pInstrHC    Host context point to privileged instruction
 * @param   pCpu        Disassembly CPU structure ptr
 * @param   pPatch      Patch record
 *
 * @note    returns failure if patching is not allowed or possible
 *
 */
int patmR3PatchInstrInt3(PVM pVM, RTRCPTR pInstrGC, R3PTRTYPE(uint8_t *) pInstrHC, DISCPUSTATE *pCpu, PPATCHINFO pPatch)
{
    uint8_t cbASMInt3 = 1;
    int rc;
    RT_NOREF_PV(pInstrHC);

    /* Note: Do not use patch memory here! It might called during patch installation too. */
    PATM_LOG_PATCH_INSTR(pVM, pPatch, PATMREAD_ORGCODE, "patmR3PatchInstrInt3:", "");

    /* Save the original instruction. */
    rc = PGMPhysSimpleReadGCPtr(VMMGetCpu0(pVM), pPatch->aPrivInstr, pPatch->pPrivInstrGC, pPatch->cbPrivInstr);
    AssertRC(rc);
    pPatch->cbPatchJump = cbASMInt3;  /* bit of a misnomer in this case; size of replacement instruction. */

    pPatch->flags |= PATMFL_INT3_REPLACEMENT;

    /* Replace first opcode byte with 'int 3'. */
    rc = patmActivateInt3Patch(pVM, pPatch);
    if (RT_FAILURE(rc))
        goto failure;

    /* Lowest and highest address for write monitoring. */
    pPatch->pInstrGCLowest  = pInstrGC;
    pPatch->pInstrGCHighest = pInstrGC + pCpu->cbInstr;

    pPatch->uState = PATCH_ENABLED;
    return VINF_SUCCESS;

failure:
    /* Turn this patch into a dummy. */
    return VERR_PATCHING_REFUSED;
}

#ifdef PATM_RESOLVE_CONFLICTS_WITH_JUMP_PATCHES
/**
 * Patch a jump instruction at specified location
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context point to privileged instruction
 * @param   pInstrHC    Host context point to privileged instruction
 * @param   pCpu        Disassembly CPU structure ptr
 * @param   pPatchRec   Patch record
 *
 * @note    returns failure if patching is not allowed or possible
 *
 */
int patmPatchJump(PVM pVM, RTRCPTR pInstrGC, R3PTRTYPE(uint8_t *) pInstrHC, DISCPUSTATE *pCpu, PPATMPATCHREC pPatchRec)
{
    PPATCHINFO pPatch = &pPatchRec->patch;
    int rc = VERR_PATCHING_REFUSED;

    pPatch->pPatchBlockOffset = 0;  /* doesn't use patch memory */
    pPatch->uCurPatchOffset   = 0;
    pPatch->cbPatchBlockSize  = 0;
    pPatch->flags            |= PATMFL_SINGLE_INSTRUCTION;

    /*
     * Instruction replacements such as these should never be interrupted. I've added code to EM.cpp to
     * make sure this never happens. (unless a trap is triggered (intentionally or not))
     */
    switch (pCpu->pCurInstr->uOpcode)
    {
    case OP_JO:
    case OP_JNO:
    case OP_JC:
    case OP_JNC:
    case OP_JE:
    case OP_JNE:
    case OP_JBE:
    case OP_JNBE:
    case OP_JS:
    case OP_JNS:
    case OP_JP:
    case OP_JNP:
    case OP_JL:
    case OP_JNL:
    case OP_JLE:
    case OP_JNLE:
    case OP_JMP:
        Assert(pPatch->flags & PATMFL_JUMP_CONFLICT);
        Assert(pCpu->Param1.fUse & DISUSE_IMMEDIATE32_REL);
        if (!(pCpu->Param1.fUse & DISUSE_IMMEDIATE32_REL))
            goto failure;

        Assert(pCpu->cbInstr == SIZEOF_NEARJUMP32 || pCpu->cbInstr == SIZEOF_NEAR_COND_JUMP32);
        if (pCpu->cbInstr != SIZEOF_NEARJUMP32 && pCpu->cbInstr != SIZEOF_NEAR_COND_JUMP32)
            goto failure;

        if (PAGE_ADDRESS(pInstrGC) != PAGE_ADDRESS(pInstrGC + pCpu->cbInstr))
        {
            STAM_COUNTER_INC(&pVM->patm.s.StatPageBoundaryCrossed);
            AssertMsgFailed(("Patch jump would cross page boundary -> refuse!!\n"));
            rc = VERR_PATCHING_REFUSED;
            goto failure;
        }

        break;

    default:
        goto failure;
    }

    // make a copy of the guest code bytes that will be overwritten
    Assert(pCpu->cbInstr <= sizeof(pPatch->aPrivInstr));
    Assert(pCpu->cbInstr >= SIZEOF_NEARJUMP32);
    pPatch->cbPatchJump = pCpu->cbInstr;

    rc = PGMPhysSimpleReadGCPtr(VMMGetCpu0(pVM), pPatch->aPrivInstr, pPatch->pPrivInstrGC, pPatch->cbPatchJump);
    AssertRC(rc);

    /* Now insert a jump in the guest code. */
    /*
     * A conflict jump patch needs to be treated differently; we'll just replace the relative jump address with one that
     * references the target instruction in the conflict patch.
     */
    RTRCPTR pJmpDest = patmR3GuestGCPtrToPatchGCPtrSimple(pVM, pInstrGC + pCpu->cbInstr + (int32_t)pCpu->Param1.uValue);

    AssertMsg(pJmpDest, ("patmR3GuestGCPtrToPatchGCPtrSimple failed for %RRv\n", pInstrGC + pCpu->cbInstr + (int32_t)pCpu->Param1.uValue));
    pPatch->pPatchJumpDestGC = pJmpDest;

    PATMP2GLOOKUPREC cacheRec;
    RT_ZERO(cacheRec);
    cacheRec.pPatch = pPatch;

    rc = patmGenJumpToPatch(pVM, pPatch, &cacherec, true);
    /* Free leftover lock if any. */
    if (cacheRec.Lock.pvMap)
    {
        PGMPhysReleasePageMappingLock(pVM, &cacheRec.Lock);
        cacheRec.Lock.pvMap = NULL;
    }
    AssertRC(rc);
    if (RT_FAILURE(rc))
        goto failure;

    pPatch->flags |= PATMFL_MUST_INSTALL_PATCHJMP;

    PATM_LOG_ORG_PATCH_INSTR(pVM, pPatch, patmGetInstructionString(pPatch->opcode, pPatch->flags));
    Log(("Successfully installed %s patch at %RRv\n", patmGetInstructionString(pPatch->opcode, pPatch->flags), pInstrGC));

    STAM_COUNTER_INC(&pVM->patm.s.StatInstalledJump);

    /* Lowest and highest address for write monitoring. */
    pPatch->pInstrGCLowest  = pInstrGC;
    pPatch->pInstrGCHighest = pInstrGC + pPatch->cbPatchJump;

    pPatch->uState = PATCH_ENABLED;
    return VINF_SUCCESS;

failure:
    /* Turn this cli patch into a dummy. */
    pPatch->uState = PATCH_REFUSED;

    return rc;
}
#endif /* PATM_RESOLVE_CONFLICTS_WITH_JUMP_PATCHES */


/**
 * Gives hint to PATM about supervisor guest instructions
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context point to privileged instruction
 * @param   flags       Patch flags
 */
VMMR3_INT_DECL(int) PATMR3AddHint(PVM pVM, RTRCPTR pInstrGC, uint32_t flags)
{
    Assert(pInstrGC);
    Assert(flags == PATMFL_CODE32); RT_NOREF_PV(flags);

    Log(("PATMR3AddHint %RRv\n", pInstrGC));
    return PATMR3InstallPatch(pVM, pInstrGC, PATMFL_CODE32 | PATMFL_INSTR_HINT);
}

/**
 * Patch privileged instruction at specified location
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context point to privileged instruction (0:32 flat
 *                      address)
 * @param   flags       Patch flags
 *
 * @note    returns failure if patching is not allowed or possible
 */
VMMR3_INT_DECL(int) PATMR3InstallPatch(PVM pVM, RTRCPTR pInstrGC, uint64_t flags)
{
    DISCPUSTATE cpu;
    R3PTRTYPE(uint8_t *) pInstrHC;
    uint32_t cbInstr;
    PPATMPATCHREC pPatchRec;
    PCPUMCTX pCtx = 0;
    bool disret;
    int rc;
    PVMCPU pVCpu = VMMGetCpu0(pVM);
    LogFlow(("PATMR3InstallPatch: %08x (%#llx)\n", pInstrGC, flags));

    AssertReturn(!HMIsEnabled(pVM), VERR_PATM_HM_IPE);

    if (    !pVM
        ||  pInstrGC == 0
        || (flags & ~(PATMFL_CODE32|PATMFL_IDTHANDLER|PATMFL_INTHANDLER|PATMFL_SYSENTER|PATMFL_TRAPHANDLER|PATMFL_DUPLICATE_FUNCTION|PATMFL_REPLACE_FUNCTION_CALL|PATMFL_GUEST_SPECIFIC|PATMFL_INT3_REPLACEMENT|PATMFL_TRAPHANDLER_WITH_ERRORCODE|PATMFL_IDTHANDLER_WITHOUT_ENTRYPOINT|PATMFL_MMIO_ACCESS|PATMFL_TRAMPOLINE|PATMFL_INSTR_HINT|PATMFL_JUMP_CONFLICT)))
    {
        AssertFailed();
        return VERR_INVALID_PARAMETER;
    }

    if (PATMIsEnabled(pVM) == false)
        return VERR_PATCHING_REFUSED;

    /* Test for patch conflict only with patches that actually change guest code. */
    if (!(flags & (PATMFL_GUEST_SPECIFIC|PATMFL_IDTHANDLER|PATMFL_INTHANDLER|PATMFL_TRAMPOLINE)))
    {
        PPATCHINFO pConflictPatch = patmFindActivePatchByEntrypoint(pVM, pInstrGC);
        AssertReleaseMsg(pConflictPatch == 0, ("Unable to patch overwritten instruction at %RRv (%RRv)\n", pInstrGC, pConflictPatch->pPrivInstrGC));
        if (pConflictPatch != 0)
            return VERR_PATCHING_REFUSED;
    }

    if (!(flags & PATMFL_CODE32))
    {
        /** @todo Only 32 bits code right now */
        AssertMsgFailed(("PATMR3InstallPatch: We don't support 16 bits code at this moment!!\n"));
        return VERR_NOT_IMPLEMENTED;
    }

    /* We ran out of patch memory; don't bother anymore. */
    if (pVM->patm.s.fOutOfMemory == true)
        return VERR_PATCHING_REFUSED;

#if 1 /* DONT COMMIT ENABLED! */
    /* Blacklisted NT4SP1 areas - debugging why we sometimes crash early on, */
    if (  0
        //|| (pInstrGC - 0x80010000U) < 0x10000U // NT4SP1 HAL
        //|| (pInstrGC - 0x80010000U) < 0x5000U // NT4SP1 HAL
        //|| (pInstrGC - 0x80013000U) < 0x2000U // NT4SP1 HAL
        //|| (pInstrGC - 0x80014000U) < 0x1000U // NT4SP1 HAL
        //|| (pInstrGC - 0x80014000U) < 0x800U // NT4SP1 HAL
        //|| (pInstrGC - 0x80014400U) < 0x400U // NT4SP1 HAL
        //|| (pInstrGC - 0x80014400U) < 0x200U // NT4SP1 HAL
        //|| (pInstrGC - 0x80014400U) < 0x100U // NT4SP1 HAL
        //|| (pInstrGC - 0x80014500U) < 0x100U // NT4SP1 HAL - negative
        //|| (pInstrGC - 0x80014400U) < 0x80U // NT4SP1 HAL
        //|| (pInstrGC - 0x80014400U) < 0x80U // NT4SP1 HAL
        //|| (pInstrGC - 0x80014440U) < 0x40U // NT4SP1 HAL
        //|| (pInstrGC - 0x80014440U) < 0x20U // NT4SP1 HAL
        || pInstrGC == 0x80014447       /* KfLowerIrql */
        || 0)
    {
        Log(("PATMR3InstallPatch: %08x is blacklisted\n", pInstrGC));
        return VERR_PATCHING_REFUSED;
    }
#endif

    /* Make sure the code selector is wide open; otherwise refuse. */
    pCtx = CPUMQueryGuestCtxPtr(pVCpu);
    if (CPUMGetGuestCPL(pVCpu) == 0)
    {
        RTRCPTR pInstrGCFlat = SELMToFlat(pVM, DISSELREG_CS, CPUMCTX2CORE(pCtx), pInstrGC);
        if (pInstrGCFlat != pInstrGC)
        {
            Log(("PATMR3InstallPatch: code selector not wide open: %04x:%RRv != %RRv eflags=%08x\n", pCtx->cs.Sel, pInstrGCFlat, pInstrGC, pCtx->eflags.u32));
            return VERR_PATCHING_REFUSED;
        }
    }

    /* Note: the OpenBSD specific check will break if we allow additional patches to be installed (int 3)) */
    if (!(flags & PATMFL_GUEST_SPECIFIC))
    {
        /* New code. Make sure CSAM has a go at it first. */
        CSAMR3CheckCode(pVM, pInstrGC);
    }

    /* Note: obsolete */
    if (    PATMIsPatchGCAddr(pVM, pInstrGC)
        && (flags & PATMFL_MMIO_ACCESS))
    {
        RTRCUINTPTR offset;
        void         *pvPatchCoreOffset;

        /* Find the patch record. */
        offset = pInstrGC - pVM->patm.s.pPatchMemGC;
        pvPatchCoreOffset = RTAvloU32GetBestFit(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPatchAddr, offset, false);
        if (pvPatchCoreOffset == NULL)
        {
            AssertMsgFailed(("PATMR3InstallPatch: patch not found at address %RRv!!\n", pInstrGC));
            return VERR_PATCH_NOT_FOUND;    //fatal error
        }
        pPatchRec = PATM_PATCHREC_FROM_COREOFFSET(pvPatchCoreOffset);

        return patmPatchPATMMMIOInstr(pVM, pInstrGC, &pPatchRec->patch);
    }

    AssertReturn(!PATMIsPatchGCAddr(pVM, pInstrGC), VERR_PATCHING_REFUSED);

    pPatchRec = (PPATMPATCHREC)RTAvloU32Get(&pVM->patm.s.PatchLookupTreeHC->PatchTree, pInstrGC);
    if (pPatchRec)
    {
        Assert(!(flags & PATMFL_TRAMPOLINE));

        /* Hints about existing patches are ignored. */
        if (flags & PATMFL_INSTR_HINT)
            return VERR_PATCHING_REFUSED;

        if (pPatchRec->patch.uState == PATCH_DISABLE_PENDING)
        {
            Log(("PATMR3InstallPatch: disable operation is pending for patch at %RRv\n", pPatchRec->patch.pPrivInstrGC));
            PATMR3DisablePatch(pVM, pPatchRec->patch.pPrivInstrGC);
            Assert(pPatchRec->patch.uState == PATCH_DISABLED);
        }

        if (pPatchRec->patch.uState == PATCH_DISABLED)
        {
            /* A patch, for which we previously received a hint, will be enabled and turned into a normal patch. */
            if (pPatchRec->patch.flags & PATMFL_INSTR_HINT)
            {
                Log(("Enabling HINTED patch %RRv\n", pInstrGC));
                pPatchRec->patch.flags &= ~PATMFL_INSTR_HINT;
            }
            else
                Log(("Enabling patch %RRv again\n", pInstrGC));

            /** @todo we shouldn't disable and enable patches too often (it's relatively cheap, but pointless if it always happens) */
            rc = PATMR3EnablePatch(pVM, pInstrGC);
            if (RT_SUCCESS(rc))
                return VWRN_PATCH_ENABLED;

            return rc;
        }
        if (    pPatchRec->patch.uState == PATCH_ENABLED
            ||  pPatchRec->patch.uState == PATCH_DIRTY)
        {
            /*
             * The patch might have been overwritten.
             */
            STAM_COUNTER_INC(&pVM->patm.s.StatOverwritten);
            if (pPatchRec->patch.uState != PATCH_REFUSED && pPatchRec->patch.uState != PATCH_UNUSABLE)
            {
                /* Patch must have been overwritten; remove it and pretend nothing happened. */
                Log(("Patch an existing patched instruction?!? (%RRv)\n", pInstrGC));
                if (pPatchRec->patch.flags & (PATMFL_DUPLICATE_FUNCTION|PATMFL_IDTHANDLER|PATMFL_MMIO_ACCESS|PATMFL_INT3_REPLACEMENT|PATMFL_INT3_REPLACEMENT_BLOCK))
                {
                    if (flags & PATMFL_IDTHANDLER)
                        pPatchRec->patch.flags |= (flags & (PATMFL_IDTHANDLER|PATMFL_TRAPHANDLER|PATMFL_INTHANDLER)); /* update the type */

                    return VERR_PATM_ALREADY_PATCHED;    /* already done once */
                }
            }
            rc = PATMR3RemovePatch(pVM, pInstrGC);
            if (RT_FAILURE(rc))
                return VERR_PATCHING_REFUSED;
        }
        else
        {
            AssertMsg(pPatchRec->patch.uState == PATCH_REFUSED || pPatchRec->patch.uState == PATCH_UNUSABLE, ("Patch an existing patched instruction?!? (%RRv, state=%d)\n", pInstrGC, pPatchRec->patch.uState));
            /* already tried it once! */
            return VERR_PATCHING_REFUSED;
        }
    }

    RTGCPHYS GCPhys;
    rc = PGMGstGetPage(pVCpu, pInstrGC, NULL, &GCPhys);
    if (rc != VINF_SUCCESS)
    {
        Log(("PGMGstGetPage failed with %Rrc\n", rc));
        return rc;
    }
    /* Disallow patching instructions inside ROM code; complete function duplication is allowed though. */
    if (    !(flags & (PATMFL_DUPLICATE_FUNCTION|PATMFL_TRAMPOLINE))
        &&  !PGMPhysIsGCPhysNormal(pVM, GCPhys))
    {
        Log(("Code at %RGv (phys %RGp) is in a ROM, MMIO or invalid page - refused\n", pInstrGC, GCPhys));
        return VERR_PATCHING_REFUSED;
    }

    /* Initialize cache record for guest address translations. */
    bool fInserted;
    PATMP2GLOOKUPREC cacheRec;
    RT_ZERO(cacheRec);

    pInstrHC = patmR3GCVirtToHCVirt(pVM, &cacheRec, pInstrGC);
    AssertReturn(pInstrHC, VERR_PATCHING_REFUSED);

    /* Allocate patch record. */
    rc = MMHyperAlloc(pVM, sizeof(PATMPATCHREC), 0, MM_TAG_PATM_PATCH, (void **)&pPatchRec);
    if (RT_FAILURE(rc))
    {
        Log(("Out of memory!!!!\n"));
        return VERR_NO_MEMORY;
    }
    pPatchRec->Core.Key = pInstrGC;
    pPatchRec->patch.uState = PATCH_REFUSED;   /* default value */
    /* Insert patch record into the lookup tree. */
    fInserted = RTAvloU32Insert(&pVM->patm.s.PatchLookupTreeHC->PatchTree, &pPatchRec->Core);
    Assert(fInserted);

    pPatchRec->patch.pPrivInstrGC = pInstrGC;
    pPatchRec->patch.flags   = flags;
    pPatchRec->patch.uOpMode = (flags & PATMFL_CODE32) ? DISCPUMODE_32BIT : DISCPUMODE_16BIT;
    pPatchRec->patch.pTrampolinePatchesHead = NULL;

    pPatchRec->patch.pInstrGCLowest  = pInstrGC;
    pPatchRec->patch.pInstrGCHighest = pInstrGC;

    if (!(pPatchRec->patch.flags & (PATMFL_DUPLICATE_FUNCTION | PATMFL_IDTHANDLER | PATMFL_SYSENTER | PATMFL_TRAMPOLINE)))
    {
        /*
         * Close proximity to an unusable patch is a possible hint that this patch would turn out to be dangerous too!
         */
        PPATMPATCHREC pPatchNear = (PPATMPATCHREC)RTAvloU32GetBestFit(&pVM->patm.s.PatchLookupTreeHC->PatchTree, (pInstrGC + SIZEOF_NEARJUMP32 - 1), false);
        if (pPatchNear)
        {
            if (pPatchNear->patch.uState == PATCH_UNUSABLE && pInstrGC < pPatchNear->patch.pPrivInstrGC && pInstrGC + SIZEOF_NEARJUMP32 > pPatchNear->patch.pPrivInstrGC)
            {
                Log(("Dangerous patch; would overwrite the unusable patch at %RRv\n", pPatchNear->patch.pPrivInstrGC));

                pPatchRec->patch.uState = PATCH_UNUSABLE;
                /*
                 * Leave the new patch active as it's marked unusable; to prevent us from checking it over and over again
                 */
                return VERR_PATCHING_REFUSED;
            }
        }
    }

    pPatchRec->patch.pTempInfo = (PPATCHINFOTEMP)MMR3HeapAllocZ(pVM, MM_TAG_PATM_PATCH, sizeof(PATCHINFOTEMP));
    if (pPatchRec->patch.pTempInfo == 0)
    {
        Log(("Out of memory!!!!\n"));
        return VERR_NO_MEMORY;
    }

    disret = patmR3DisInstrNoStrOpMode(pVM, &pPatchRec->patch, pInstrGC, NULL, PATMREAD_ORGCODE, &cpu, &cbInstr);
    if (disret == false)
    {
        Log(("Disassembly failed (probably page not present) -> return to caller\n"));
        return VERR_PATCHING_REFUSED;
    }

    AssertMsg(cbInstr <= MAX_INSTR_SIZE, ("privileged instruction too big %d!!\n", cbInstr));
    if (cbInstr > MAX_INSTR_SIZE)
        return VERR_PATCHING_REFUSED;

    pPatchRec->patch.cbPrivInstr = cbInstr;
    pPatchRec->patch.opcode      = cpu.pCurInstr->uOpcode;

    /* Restricted hinting for now. */
    Assert(!(flags & PATMFL_INSTR_HINT) || cpu.pCurInstr->uOpcode == OP_CLI);

    /* Initialize cache record patch pointer. */
    cacheRec.pPatch = &pPatchRec->patch;

    /* Allocate statistics slot */
    if (pVM->patm.s.uCurrentPatchIdx < PATM_STAT_MAX_COUNTERS)
    {
        pPatchRec->patch.uPatchIdx = pVM->patm.s.uCurrentPatchIdx++;
    }
    else
    {
        Log(("WARNING: Patch index wrap around!!\n"));
        pPatchRec->patch.uPatchIdx = PATM_STAT_INDEX_DUMMY;
    }

    if (pPatchRec->patch.flags & PATMFL_TRAPHANDLER)
    {
        rc = patmInstallTrapTrampoline(pVM, pInstrGC, pPatchRec, &cacheRec);
    }
    else
    if (pPatchRec->patch.flags & (PATMFL_DUPLICATE_FUNCTION ))
    {
        rc = patmDuplicateFunction(pVM, pInstrGC, pPatchRec, &cacheRec);
    }
    else
    if (pPatchRec->patch.flags & PATMFL_TRAMPOLINE)
    {
        rc = patmCreateTrampoline(pVM, pInstrGC, pPatchRec);
    }
    else
    if (pPatchRec->patch.flags & PATMFL_REPLACE_FUNCTION_CALL)
    {
        rc = patmReplaceFunctionCall(pVM, &cpu, pInstrGC, &cacheRec);
    }
    else
    if (pPatchRec->patch.flags & PATMFL_INT3_REPLACEMENT)
    {
        rc = patmR3PatchInstrInt3(pVM, pInstrGC, pInstrHC, &cpu, &pPatchRec->patch);
    }
    else
    if (pPatchRec->patch.flags & PATMFL_MMIO_ACCESS)
    {
        rc = patmPatchMMIOInstr(pVM, pInstrGC, &cpu, &cacheRec);
    }
    else
    if (pPatchRec->patch.flags & (PATMFL_IDTHANDLER|PATMFL_SYSENTER))
    {
        if (pPatchRec->patch.flags & PATMFL_SYSENTER)
            pPatchRec->patch.flags |= PATMFL_IDTHANDLER;    /* we treat a sysenter handler as an IDT handler */

        rc = patmIdtHandler(pVM, pInstrGC, cbInstr, pPatchRec, &cacheRec);
#ifdef VBOX_WITH_STATISTICS
        if (    rc == VINF_SUCCESS
            &&  (pPatchRec->patch.flags & PATMFL_SYSENTER))
        {
            pVM->patm.s.uSysEnterPatchIdx = pPatchRec->patch.uPatchIdx;
        }
#endif
    }
    else
    if (pPatchRec->patch.flags & PATMFL_GUEST_SPECIFIC)
    {
        switch (cpu.pCurInstr->uOpcode)
        {
        case OP_SYSENTER:
        case OP_PUSH:
            rc = patmR3InstallGuestSpecificPatch(pVM, &cpu, pInstrGC, pInstrHC, pPatchRec);
            if (rc == VINF_SUCCESS)
            {
                if (rc == VINF_SUCCESS)
                    Log(("PATMR3InstallPatch GUEST: %s %RRv code32=%d\n", patmGetInstructionString(pPatchRec->patch.opcode, pPatchRec->patch.flags), pInstrGC, (flags & PATMFL_CODE32) ? 1 : 0));
                return rc;
            }
            break;

        default:
            rc = VERR_NOT_IMPLEMENTED;
            break;
        }
    }
    else
    {
        switch (cpu.pCurInstr->uOpcode)
        {
        case OP_SYSENTER:
            rc = patmR3InstallGuestSpecificPatch(pVM, &cpu, pInstrGC, pInstrHC, pPatchRec);
            if (rc == VINF_SUCCESS)
            {
                Log(("PATMR3InstallPatch GUEST: %s %RRv code32=%d\n", patmGetInstructionString(pPatchRec->patch.opcode, pPatchRec->patch.flags), pInstrGC, (flags & PATMFL_CODE32) ? 1 : 0));
                return VINF_SUCCESS;
            }
            break;

#ifdef PATM_RESOLVE_CONFLICTS_WITH_JUMP_PATCHES
        case OP_JO:
        case OP_JNO:
        case OP_JC:
        case OP_JNC:
        case OP_JE:
        case OP_JNE:
        case OP_JBE:
        case OP_JNBE:
        case OP_JS:
        case OP_JNS:
        case OP_JP:
        case OP_JNP:
        case OP_JL:
        case OP_JNL:
        case OP_JLE:
        case OP_JNLE:
        case OP_JECXZ:
        case OP_LOOP:
        case OP_LOOPNE:
        case OP_LOOPE:
        case OP_JMP:
            if (pPatchRec->patch.flags & PATMFL_JUMP_CONFLICT)
            {
                rc = patmPatchJump(pVM, pInstrGC, pInstrHC, &cpu, pPatchRec);
                break;
            }
            return VERR_NOT_IMPLEMENTED;
#endif

        case OP_PUSHF:
        case OP_CLI:
            Log(("PATMR3InstallPatch %s %RRv code32=%d\n", patmGetInstructionString(pPatchRec->patch.opcode, pPatchRec->patch.flags), pInstrGC, (flags & PATMFL_CODE32) ? 1 : 0));
            rc = patmR3PatchBlock(pVM, pInstrGC, pInstrHC, cpu.pCurInstr->uOpcode, cbInstr, pPatchRec);
            break;

#ifndef VBOX_WITH_SAFE_STR
        case OP_STR:
#endif
        case OP_SGDT:
        case OP_SLDT:
        case OP_SIDT:
        case OP_CPUID:
        case OP_LSL:
        case OP_LAR:
        case OP_SMSW:
        case OP_VERW:
        case OP_VERR:
        case OP_IRET:
#ifdef VBOX_WITH_RAW_RING1
        case OP_MOV:
#endif
            rc = patmR3PatchInstrInt3(pVM, pInstrGC, pInstrHC, &cpu, &pPatchRec->patch);
            break;

        default:
            return VERR_NOT_IMPLEMENTED;
        }
    }

    if (rc != VINF_SUCCESS)
    {
        if (pPatchRec && pPatchRec->patch.nrPatch2GuestRecs)
        {
            patmEmptyTreeU32(pVM, &pPatchRec->patch.Patch2GuestAddrTree);
            pPatchRec->patch.nrPatch2GuestRecs = 0;
        }
        pVM->patm.s.uCurrentPatchIdx--;
    }
    else
    {
        rc = patmInsertPatchPages(pVM, &pPatchRec->patch);
        AssertRCReturn(rc, rc);

        /* Keep track upper and lower boundaries of patched instructions */
        if (pPatchRec->patch.pInstrGCLowest < pVM->patm.s.pPatchedInstrGCLowest)
            pVM->patm.s.pPatchedInstrGCLowest = pPatchRec->patch.pInstrGCLowest;
        if (pPatchRec->patch.pInstrGCHighest > pVM->patm.s.pPatchedInstrGCHighest)
            pVM->patm.s.pPatchedInstrGCHighest = pPatchRec->patch.pInstrGCHighest;

        Log(("Patch  lowest %RRv highest %RRv\n", pPatchRec->patch.pInstrGCLowest, pPatchRec->patch.pInstrGCHighest));
        Log(("Global lowest %RRv highest %RRv\n", pVM->patm.s.pPatchedInstrGCLowest, pVM->patm.s.pPatchedInstrGCHighest));

        STAM_COUNTER_ADD(&pVM->patm.s.StatInstalled, 1);
        STAM_COUNTER_ADD(&pVM->patm.s.StatPATMMemoryUsed, pPatchRec->patch.cbPatchBlockSize);

        rc = VINF_SUCCESS;

        /* Patch hints are not enabled by default. Only when the are actually encountered. */
        if (pPatchRec->patch.flags & PATMFL_INSTR_HINT)
        {
            rc = PATMR3DisablePatch(pVM, pInstrGC);
            AssertRCReturn(rc, rc);
        }

#ifdef VBOX_WITH_STATISTICS
        /* Register statistics counter */
        if (PATM_STAT_INDEX_IS_VALID(pPatchRec->patch.uPatchIdx))
        {
            STAMR3RegisterCallback(pVM, &pPatchRec->patch, STAMVISIBILITY_NOT_GUI, STAMUNIT_GOOD_BAD, patmResetStat, patmPrintStat, "Patch statistics",
                                   "/PATM/Stats/Patch/0x%RRv", pPatchRec->patch.pPrivInstrGC);
#ifndef DEBUG_sandervl
            /* Full breakdown for the GUI. */
            STAMR3RegisterF(pVM, &pVM->patm.s.pStatsHC[pPatchRec->patch.uPatchIdx], STAMTYPE_RATIO_U32, STAMVISIBILITY_ALWAYS, STAMUNIT_GOOD_BAD, PATMPatchType(pVM, &pPatchRec->patch),
                            "/PATM/PatchBD/0x%RRv", pPatchRec->patch.pPrivInstrGC);
            STAMR3RegisterF(pVM, &pPatchRec->patch.pPatchBlockOffset,STAMTYPE_X32, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES,     NULL, "/PATM/PatchBD/0x%RRv/offPatchBlock", pPatchRec->patch.pPrivInstrGC);
            STAMR3RegisterF(pVM, &pPatchRec->patch.cbPatchBlockSize,STAMTYPE_U32, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES,      NULL, "/PATM/PatchBD/0x%RRv/cbPatchBlockSize", pPatchRec->patch.pPrivInstrGC);
            STAMR3RegisterF(pVM, &pPatchRec->patch.cbPatchJump,     STAMTYPE_U32, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES,      NULL, "/PATM/PatchBD/0x%RRv/cbPatchJump", pPatchRec->patch.pPrivInstrGC);
            STAMR3RegisterF(pVM, &pPatchRec->patch.cbPrivInstr,     STAMTYPE_U32, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES,      NULL, "/PATM/PatchBD/0x%RRv/cbPrivInstr", pPatchRec->patch.pPrivInstrGC);
            STAMR3RegisterF(pVM, &pPatchRec->patch.cCodeWrites,     STAMTYPE_U32, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, NULL, "/PATM/PatchBD/0x%RRv/cCodeWrites", pPatchRec->patch.pPrivInstrGC);
            STAMR3RegisterF(pVM, &pPatchRec->patch.cInvalidWrites,  STAMTYPE_U32, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, NULL, "/PATM/PatchBD/0x%RRv/cInvalidWrites", pPatchRec->patch.pPrivInstrGC);
            STAMR3RegisterF(pVM, &pPatchRec->patch.cTraps,          STAMTYPE_U32, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, NULL, "/PATM/PatchBD/0x%RRv/cTraps", pPatchRec->patch.pPrivInstrGC);
            STAMR3RegisterF(pVM, &pPatchRec->patch.flags,           STAMTYPE_X64, STAMVISIBILITY_ALWAYS, STAMUNIT_NONE,       NULL, "/PATM/PatchBD/0x%RRv/flags", pPatchRec->patch.pPrivInstrGC);
            STAMR3RegisterF(pVM, &pPatchRec->patch.nrJumpRecs,      STAMTYPE_U32, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, NULL, "/PATM/PatchBD/0x%RRv/nrJumpRecs", pPatchRec->patch.pPrivInstrGC);
            STAMR3RegisterF(pVM, &pPatchRec->patch.nrFixups,        STAMTYPE_U32, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, NULL, "/PATM/PatchBD/0x%RRv/nrFixups", pPatchRec->patch.pPrivInstrGC);
            STAMR3RegisterF(pVM, &pPatchRec->patch.opcode,          STAMTYPE_U32, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, NULL, "/PATM/PatchBD/0x%RRv/opcode", pPatchRec->patch.pPrivInstrGC);
            STAMR3RegisterF(pVM, &pPatchRec->patch.uOldState,       STAMTYPE_U32, STAMVISIBILITY_ALWAYS, STAMUNIT_NONE,       NULL, "/PATM/PatchBD/0x%RRv/uOldState", pPatchRec->patch.pPrivInstrGC);
            STAMR3RegisterF(pVM, &pPatchRec->patch.uOpMode,         STAMTYPE_U32, STAMVISIBILITY_ALWAYS, STAMUNIT_NONE,       NULL, "/PATM/PatchBD/0x%RRv/uOpMode", pPatchRec->patch.pPrivInstrGC);
            /// @todo change the state to be a callback so we can get a state mnemonic instead.
            STAMR3RegisterF(pVM, &pPatchRec->patch.uState,          STAMTYPE_U32, STAMVISIBILITY_ALWAYS, STAMUNIT_NONE,       NULL, "/PATM/PatchBD/0x%RRv/uState", pPatchRec->patch.pPrivInstrGC);
#endif
        }
#endif

        /* Add debug symbol. */
        patmR3DbgAddPatch(pVM, pPatchRec);
    }
    /* Free leftover lock if any. */
    if (cacheRec.Lock.pvMap)
        PGMPhysReleasePageMappingLock(pVM, &cacheRec.Lock);
    return rc;
}

/**
 * Query instruction size
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch record
 * @param   pInstrGC    Instruction address
 */
static uint32_t patmGetInstrSize(PVM pVM, PPATCHINFO pPatch, RTRCPTR pInstrGC)
{
    uint8_t       *pInstrHC;
    PGMPAGEMAPLOCK Lock;

    int rc = PGMPhysGCPtr2CCPtrReadOnly(VMMGetCpu(pVM), pInstrGC, (const void **)&pInstrHC, &Lock);
    if (rc == VINF_SUCCESS)
    {
        DISCPUSTATE cpu;
        bool        disret;
        uint32_t    cbInstr;

        disret = patmR3DisInstr(pVM, pPatch, pInstrGC, pInstrHC, PATMREAD_ORGCODE | PATMREAD_NOCHECK, &cpu, &cbInstr);
        PGMPhysReleasePageMappingLock(pVM, &Lock);
        if (disret)
            return cbInstr;
    }
    return 0;
}

/**
 * Add patch to page record
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPage       Page address
 * @param   pPatch      Patch record
 */
int patmAddPatchToPage(PVM pVM, RTRCUINTPTR pPage, PPATCHINFO pPatch)
{
    PPATMPATCHPAGE pPatchPage;
    int            rc;

    Log(("patmAddPatchToPage: insert patch %RHv to page %RRv\n", pPatch, pPage));

    pPatchPage = (PPATMPATCHPAGE)RTAvloU32Get(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPage, pPage);
    if (pPatchPage)
    {
        Assert(pPatchPage->cCount <= pPatchPage->cMaxPatches);
        if (pPatchPage->cCount == pPatchPage->cMaxPatches)
        {
            uint32_t    cMaxPatchesOld = pPatchPage->cMaxPatches;
            PPATCHINFO *papPatchOld    = pPatchPage->papPatch;

            pPatchPage->cMaxPatches += PATMPATCHPAGE_PREALLOC_INCREMENT;
            rc = MMHyperAlloc(pVM, sizeof(pPatchPage->papPatch[0]) * pPatchPage->cMaxPatches, 0, MM_TAG_PATM_PATCH,
                              (void **)&pPatchPage->papPatch);
            if (RT_FAILURE(rc))
            {
                Log(("Out of memory!!!!\n"));
                return VERR_NO_MEMORY;
            }
            memcpy(pPatchPage->papPatch, papPatchOld, cMaxPatchesOld * sizeof(pPatchPage->papPatch[0]));
            MMHyperFree(pVM, papPatchOld);
        }
        pPatchPage->papPatch[pPatchPage->cCount] = pPatch;
        pPatchPage->cCount++;
    }
    else
    {
        bool fInserted;

        rc = MMHyperAlloc(pVM, sizeof(PATMPATCHPAGE), 0, MM_TAG_PATM_PATCH, (void **)&pPatchPage);
        if (RT_FAILURE(rc))
        {
            Log(("Out of memory!!!!\n"));
            return VERR_NO_MEMORY;
        }
        pPatchPage->Core.Key    = pPage;
        pPatchPage->cCount      = 1;
        pPatchPage->cMaxPatches = PATMPATCHPAGE_PREALLOC_INCREMENT;

        rc = MMHyperAlloc(pVM, sizeof(pPatchPage->papPatch[0]) * PATMPATCHPAGE_PREALLOC_INCREMENT, 0, MM_TAG_PATM_PATCH,
                          (void **)&pPatchPage->papPatch);
        if (RT_FAILURE(rc))
        {
            Log(("Out of memory!!!!\n"));
            MMHyperFree(pVM, pPatchPage);
            return VERR_NO_MEMORY;
        }
        pPatchPage->papPatch[0] = pPatch;

        fInserted = RTAvloU32Insert(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPage, &pPatchPage->Core);
        Assert(fInserted);
        pVM->patm.s.cPageRecords++;

        STAM_COUNTER_INC(&pVM->patm.s.StatPatchPageInserted);
    }
    CSAMR3MonitorPage(pVM, pPage, CSAM_TAG_PATM);

    /* Get the closest guest instruction (from below) */
    PRECGUESTTOPATCH pGuestToPatchRec = (PRECGUESTTOPATCH)RTAvlU32GetBestFit(&pPatch->Guest2PatchAddrTree, pPage, true);
    Assert(pGuestToPatchRec);
    if (pGuestToPatchRec)
    {
        LogFlow(("patmAddPatchToPage: lowest patch page address %RRv current lowest %RRv\n", pGuestToPatchRec->Core.Key, pPatchPage->pLowestAddrGC));
        if (    pPatchPage->pLowestAddrGC == 0
            ||  pPatchPage->pLowestAddrGC > (RTRCPTR)pGuestToPatchRec->Core.Key)
        {
            RTRCUINTPTR offset;

            pPatchPage->pLowestAddrGC = (RTRCPTR)pGuestToPatchRec->Core.Key;

            offset = pPatchPage->pLowestAddrGC & PAGE_OFFSET_MASK;
            /* If we're too close to the page boundary, then make sure an
               instruction from the previous page doesn't cross the
               boundary itself. */
            if (offset && offset < MAX_INSTR_SIZE)
            {
                /* Get the closest guest instruction (from above) */
                pGuestToPatchRec = (PRECGUESTTOPATCH)RTAvlU32GetBestFit(&pPatch->Guest2PatchAddrTree, pPage-1, false);

                if (pGuestToPatchRec)
                {
                    uint32_t size = patmGetInstrSize(pVM, pPatch, (RTRCPTR)pGuestToPatchRec->Core.Key);
                    if ((RTRCUINTPTR)pGuestToPatchRec->Core.Key + size  > pPage)
                    {
                        pPatchPage->pLowestAddrGC = pPage;
                        LogFlow(("patmAddPatchToPage: new lowest %RRv\n", pPatchPage->pLowestAddrGC));
                    }
                }
            }
        }
    }

    /* Get the closest guest instruction (from above) */
    pGuestToPatchRec = (PRECGUESTTOPATCH)RTAvlU32GetBestFit(&pPatch->Guest2PatchAddrTree, pPage+PAGE_SIZE-1, false);
    Assert(pGuestToPatchRec);
    if (pGuestToPatchRec)
    {
        LogFlow(("patmAddPatchToPage: highest patch page address %RRv current highest %RRv\n", pGuestToPatchRec->Core.Key, pPatchPage->pHighestAddrGC));
        if (    pPatchPage->pHighestAddrGC == 0
            ||  pPatchPage->pHighestAddrGC <= (RTRCPTR)pGuestToPatchRec->Core.Key)
        {
            pPatchPage->pHighestAddrGC = (RTRCPTR)pGuestToPatchRec->Core.Key;
            /* Increase by instruction size. */
            uint32_t size = patmGetInstrSize(pVM, pPatch, pPatchPage->pHighestAddrGC);
////            Assert(size);
            pPatchPage->pHighestAddrGC += size;
            LogFlow(("patmAddPatchToPage: new highest %RRv\n", pPatchPage->pHighestAddrGC));
        }
    }

    return VINF_SUCCESS;
}

/**
 * Remove patch from page record
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPage       Page address
 * @param   pPatch      Patch record
 */
int patmRemovePatchFromPage(PVM pVM, RTRCUINTPTR pPage, PPATCHINFO pPatch)
{
    PPATMPATCHPAGE pPatchPage;
    int            rc;

    pPatchPage = (PPATMPATCHPAGE)RTAvloU32Get(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPage, pPage);
    Assert(pPatchPage);

    if (!pPatchPage)
        return VERR_INVALID_PARAMETER;

    Assert(pPatchPage->cCount <= pPatchPage->cMaxPatches);

    Log(("patmRemovePatchPage: remove patch %RHv from page %RRv\n", pPatch, pPage));
    if (pPatchPage->cCount > 1)
    {
        uint32_t i;

        /* Used by multiple patches */
        for (i = 0; i < pPatchPage->cCount; i++)
        {
            if (pPatchPage->papPatch[i] == pPatch)
            {
                /* close the gap between the remaining pointers. */
                uint32_t cNew = --pPatchPage->cCount;
                if (i < cNew)
                    pPatchPage->papPatch[i] = pPatchPage->papPatch[cNew];
                pPatchPage->papPatch[cNew] = NULL;
                return VINF_SUCCESS;
            }
        }
        AssertMsgFailed(("Unable to find patch %RHv in page %RRv\n", pPatch, pPage));
    }
    else
    {
        PPATMPATCHPAGE pPatchNode;

        Log(("patmRemovePatchFromPage %RRv\n", pPage));

        STAM_COUNTER_INC(&pVM->patm.s.StatPatchPageRemoved);
        pPatchNode = (PPATMPATCHPAGE)RTAvloU32Remove(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPage, pPage);
        Assert(pPatchNode && pPatchNode == pPatchPage);

        Assert(pPatchPage->papPatch);
        rc = MMHyperFree(pVM, pPatchPage->papPatch);
        AssertRC(rc);
        rc = MMHyperFree(pVM, pPatchPage);
        AssertRC(rc);
        pVM->patm.s.cPageRecords--;
    }
    return VINF_SUCCESS;
}

/**
 * Insert page records for all guest pages that contain instructions that were recompiled for this patch
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch record
 */
int patmInsertPatchPages(PVM pVM, PPATCHINFO pPatch)
{
    int           rc;
    RTRCUINTPTR pPatchPageStart, pPatchPageEnd, pPage;

    /* Insert the pages that contain patched instructions into a lookup tree for detecting self-modifying code. */
    pPatchPageStart = (RTRCUINTPTR)pPatch->pInstrGCLowest  & PAGE_BASE_GC_MASK;
    pPatchPageEnd   = (RTRCUINTPTR)pPatch->pInstrGCHighest & PAGE_BASE_GC_MASK;

    /** @todo optimize better (large gaps between current and next used page) */
    for(pPage = pPatchPageStart; pPage <= pPatchPageEnd; pPage += PAGE_SIZE)
    {
        /* Get the closest guest instruction (from above) */
        PRECGUESTTOPATCH pGuestToPatchRec = (PRECGUESTTOPATCH)RTAvlU32GetBestFit(&pPatch->Guest2PatchAddrTree, pPage, true);
        if (    pGuestToPatchRec
            &&  PAGE_ADDRESS(pGuestToPatchRec->Core.Key) == PAGE_ADDRESS(pPage)
           )
        {
            /* Code in page really patched -> add record */
            rc = patmAddPatchToPage(pVM, pPage, pPatch);
            AssertRC(rc);
        }
    }
    pPatch->flags |= PATMFL_CODE_MONITORED;
    return VINF_SUCCESS;
}

/**
 * Remove page records for all guest pages that contain instructions that were recompiled for this patch
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch record
 */
static int patmRemovePatchPages(PVM pVM, PPATCHINFO pPatch)
{
    int         rc;
    RTRCUINTPTR pPatchPageStart, pPatchPageEnd, pPage;

    /* Insert the pages that contain patched instructions into a lookup tree for detecting self-modifying code. */
    pPatchPageStart = (RTRCUINTPTR)pPatch->pInstrGCLowest  & PAGE_BASE_GC_MASK;
    pPatchPageEnd   = (RTRCUINTPTR)pPatch->pInstrGCHighest & PAGE_BASE_GC_MASK;

    for(pPage = pPatchPageStart; pPage <= pPatchPageEnd; pPage += PAGE_SIZE)
    {
        /* Get the closest guest instruction (from above) */
        PRECGUESTTOPATCH pGuestToPatchRec = (PRECGUESTTOPATCH)RTAvlU32GetBestFit(&pPatch->Guest2PatchAddrTree, pPage, true);
        if (    pGuestToPatchRec
            &&  PAGE_ADDRESS(pGuestToPatchRec->Core.Key) == PAGE_ADDRESS(pPage) /** @todo bird: PAGE_ADDRESS is for the current context really. check out these. */
           )
        {
            /* Code in page really patched -> remove record */
            rc = patmRemovePatchFromPage(pVM, pPage, pPatch);
            AssertRC(rc);
        }
    }
    pPatch->flags &= ~PATMFL_CODE_MONITORED;
    return VINF_SUCCESS;
}

/**
 * Notifies PATM about a (potential) write to code that has been patched.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   GCPtr       GC pointer to write address
 * @param   cbWrite     Nr of bytes to write
 *
 */
VMMR3_INT_DECL(int) PATMR3PatchWrite(PVM pVM, RTRCPTR GCPtr, uint32_t cbWrite)
{
    RTRCUINTPTR          pWritePageStart, pWritePageEnd, pPage;

    Log(("PATMR3PatchWrite %RRv %x\n", GCPtr, cbWrite));

    Assert(VM_IS_EMT(pVM));
    AssertReturn(!HMIsEnabled(pVM), VERR_PATM_HM_IPE);

    /* Quick boundary check */
    if (    GCPtr < pVM->patm.s.pPatchedInstrGCLowest
        ||  GCPtr > pVM->patm.s.pPatchedInstrGCHighest
       )
       return VINF_SUCCESS;

    STAM_PROFILE_ADV_START(&pVM->patm.s.StatPatchWrite, a);

    pWritePageStart =  (RTRCUINTPTR)GCPtr & PAGE_BASE_GC_MASK;
    pWritePageEnd   = ((RTRCUINTPTR)GCPtr + cbWrite - 1) & PAGE_BASE_GC_MASK;

    for (pPage = pWritePageStart; pPage <= pWritePageEnd; pPage += PAGE_SIZE)
    {
loop_start:
        PPATMPATCHPAGE pPatchPage = (PPATMPATCHPAGE)RTAvloU32Get(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPage, (RTRCPTR)pPage);
        if (pPatchPage)
        {
            uint32_t i;
            bool fValidPatchWrite = false;

            /* Quick check to see if the write is in the patched part of the page */
            if (    pPatchPage->pLowestAddrGC  > (RTRCPTR)((RTRCUINTPTR)GCPtr + cbWrite - 1)
                ||  pPatchPage->pHighestAddrGC < GCPtr)
            {
                break;
            }

            for (i=0;i<pPatchPage->cCount;i++)
            {
                if (pPatchPage->papPatch[i])
                {
                    PPATCHINFO pPatch = pPatchPage->papPatch[i];
                    RTRCPTR pPatchInstrGC;
                    //unused: bool    fForceBreak = false;

                    Assert(pPatchPage->papPatch[i]->flags & PATMFL_CODE_MONITORED);
                    /** @todo inefficient and includes redundant checks for multiple pages. */
                    for (uint32_t j=0; j<cbWrite; j++)
                    {
                        RTRCPTR pGuestPtrGC = (RTRCPTR)((RTRCUINTPTR)GCPtr + j);

                        if (    pPatch->cbPatchJump
                            &&  pGuestPtrGC >= pPatch->pPrivInstrGC
                            &&  pGuestPtrGC < pPatch->pPrivInstrGC + pPatch->cbPatchJump)
                        {
                            /* The guest is about to overwrite the 5 byte jump to patch code. Remove the patch. */
                            Log(("PATMR3PatchWrite: overwriting jump to patch code -> remove patch.\n"));
                            int rc = PATMR3RemovePatch(pVM, pPatch->pPrivInstrGC);
                            if (rc == VINF_SUCCESS)
                                /* Note: jump back to the start as the pPatchPage has been deleted or changed */
                                goto loop_start;

                            continue;
                        }

                        /* Find the closest instruction from below; the above quick check ensured that we are indeed in patched code */
                        pPatchInstrGC = patmGuestGCPtrToPatchGCPtr(pVM, pPatch, pGuestPtrGC);
                        if (!pPatchInstrGC)
                        {
                            RTRCPTR  pClosestInstrGC;
                            uint32_t size;

                            pPatchInstrGC   = patmGuestGCPtrToClosestPatchGCPtr(pVM, pPatch, pGuestPtrGC);
                            if (pPatchInstrGC)
                            {
                                pClosestInstrGC = patmPatchGCPtr2GuestGCPtr(pVM, pPatch, pPatchInstrGC);
                                Assert(pClosestInstrGC <= pGuestPtrGC);
                                size            = patmGetInstrSize(pVM, pPatch, pClosestInstrGC);
                                /* Check if this is not a write into a gap between two patches */
                                if (pClosestInstrGC + size - 1 < pGuestPtrGC)
                                    pPatchInstrGC = 0;
                            }
                        }
                        if (pPatchInstrGC)
                        {
                            uint32_t PatchOffset = pPatchInstrGC - pVM->patm.s.pPatchMemGC;  /* Offset in memory reserved for PATM. */

                            fValidPatchWrite = true;

                            PRECPATCHTOGUEST pPatchToGuestRec = (PRECPATCHTOGUEST)RTAvlU32Get(&pPatch->Patch2GuestAddrTree, PatchOffset);
                            Assert(pPatchToGuestRec);
                            if (pPatchToGuestRec && !pPatchToGuestRec->fDirty)
                            {
                                Log(("PATMR3PatchWrite: Found patched instruction %RRv -> %RRv\n", pGuestPtrGC, pPatchInstrGC));

                                if (++pPatch->cCodeWrites > PATM_MAX_CODE_WRITES)
                                {
                                    LogRel(("PATM: Disable block at %RRv - write %RRv-%RRv\n", pPatch->pPrivInstrGC, pGuestPtrGC, pGuestPtrGC+cbWrite));

                                    patmR3MarkDirtyPatch(pVM, pPatch);

                                    /* Note: jump back to the start as the pPatchPage has been deleted or changed */
                                    goto loop_start;
                                }
                                else
                                {
                                    /* Replace the patch instruction with a breakpoint; when it's hit, then we'll attempt to recompile the instruction again. */
                                    uint8_t *pInstrHC = patmPatchGCPtr2PatchHCPtr(pVM, pPatchInstrGC);

                                    pPatchToGuestRec->u8DirtyOpcode = *pInstrHC;
                                    pPatchToGuestRec->fDirty   = true;

                                    *pInstrHC = 0xCC;

                                    STAM_COUNTER_INC(&pVM->patm.s.StatInstrDirty);
                                }
                            }
                            /* else already marked dirty */
                        }
                    }
                }
            } /* for each patch */

            if (fValidPatchWrite == false)
            {
                /* Write to a part of the page that either:
                 * - doesn't contain any code (shared code/data); rather unlikely
                 * - old code page that's no longer in active use.
                 */
invalid_write_loop_start:
                pPatchPage = (PPATMPATCHPAGE)RTAvloU32Get(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPage, (RTRCPTR)pPage);

                if (pPatchPage)
                {
                    for (i=0;i<pPatchPage->cCount;i++)
                    {
                        PPATCHINFO pPatch = pPatchPage->papPatch[i];

                        if (pPatch->cInvalidWrites > PATM_MAX_INVALID_WRITES)
                        {
                            /* Note: possibly dangerous assumption that all future writes will be harmless. */
                            if (pPatch->flags & PATMFL_IDTHANDLER)
                            {
                                LogRel(("PATM: Stop monitoring IDT handler pages at %RRv - invalid write %RRv-%RRv (this is not a fatal error)\n", pPatch->pPrivInstrGC, GCPtr, GCPtr+cbWrite));

                                Assert(pPatch->flags & PATMFL_CODE_MONITORED);
                                int rc = patmRemovePatchPages(pVM, pPatch);
                                AssertRC(rc);
                            }
                            else
                            {
                                LogRel(("PATM: Disable block at %RRv - invalid write %RRv-%RRv \n", pPatch->pPrivInstrGC, GCPtr, GCPtr+cbWrite));
                                patmR3MarkDirtyPatch(pVM, pPatch);
                            }
                            /* Note: jump back to the start as the pPatchPage has been deleted or changed */
                            goto invalid_write_loop_start;
                        }
                    } /* for */
                }
            }
        }
    }
    STAM_PROFILE_ADV_STOP(&pVM->patm.s.StatPatchWrite, a);
    return VINF_SUCCESS;

}

/**
 * Disable all patches in a flushed page
 *
 * @returns VBox status code
 * @param   pVM         The cross context VM structure.
 * @param   addr        GC address of the page to flush
 * @note    Currently only called by CSAMR3FlushPage; optimization to avoid
 *          having to double check if the physical address has changed
 */
VMMR3_INT_DECL(int) PATMR3FlushPage(PVM pVM, RTRCPTR addr)
{
    AssertReturn(!HMIsEnabled(pVM), VERR_PATM_HM_IPE);

    addr &= PAGE_BASE_GC_MASK;

    PPATMPATCHPAGE pPatchPage = (PPATMPATCHPAGE)RTAvloU32Get(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPage, addr);
    if (pPatchPage)
    {
        int i;

        /* From top to bottom as the array is modified by PATMR3MarkDirtyPatch. */
        for (i=(int)pPatchPage->cCount-1;i>=0;i--)
        {
            if (pPatchPage->papPatch[i])
            {
                PPATCHINFO pPatch = pPatchPage->papPatch[i];

                Log(("PATMR3FlushPage %RRv remove patch at %RRv\n", addr, pPatch->pPrivInstrGC));
                patmR3MarkDirtyPatch(pVM, pPatch);
            }
        }
        STAM_COUNTER_INC(&pVM->patm.s.StatFlushed);
    }
    return VINF_SUCCESS;
}

/**
 * Checks if the instructions at the specified address has been patched already.
 *
 * @returns boolean, patched or not
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context pointer to instruction
 */
VMMR3_INT_DECL(bool) PATMR3HasBeenPatched(PVM pVM, RTRCPTR pInstrGC)
{
    Assert(!HMIsEnabled(pVM));
    PPATMPATCHREC pPatchRec;
    pPatchRec = (PPATMPATCHREC)RTAvloU32Get(&pVM->patm.s.PatchLookupTreeHC->PatchTree, pInstrGC);
    if (pPatchRec && pPatchRec->patch.uState == PATCH_ENABLED)
        return true;
    return false;
}

/**
 * Query the opcode of the original code that was overwritten by the 5 bytes patch jump
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    GC address of instr
 * @param   pByte       opcode byte pointer (OUT)
 *
 */
VMMR3DECL(int) PATMR3QueryOpcode(PVM pVM, RTRCPTR pInstrGC, uint8_t *pByte)
{
    PPATMPATCHREC pPatchRec;

    /** @todo this will not work for aliased pages! (never has, but so far not a problem for us) */

    /* Shortcut. */
    if (!PATMIsEnabled(pVM))
        return VERR_PATCH_NOT_FOUND;
    Assert(!HMIsEnabled(pVM));
    if (   pInstrGC < pVM->patm.s.pPatchedInstrGCLowest
        || pInstrGC > pVM->patm.s.pPatchedInstrGCHighest)
        return VERR_PATCH_NOT_FOUND;

    pPatchRec = (PPATMPATCHREC)RTAvloU32GetBestFit(&pVM->patm.s.PatchLookupTreeHC->PatchTree, pInstrGC, false);
    // if the patch is enabled and the pointer lies within 5 bytes of this priv instr ptr, then we've got a hit!
    if (    pPatchRec
        &&  pPatchRec->patch.uState == PATCH_ENABLED
        &&  pInstrGC >= pPatchRec->patch.pPrivInstrGC
        &&  pInstrGC < pPatchRec->patch.pPrivInstrGC + pPatchRec->patch.cbPatchJump)
    {
        RTRCPTR offset = pInstrGC - pPatchRec->patch.pPrivInstrGC;
        *pByte = pPatchRec->patch.aPrivInstr[offset];

        if (pPatchRec->patch.cbPatchJump == 1)
        {
            Log(("PATMR3QueryOpcode: returning opcode %2X for instruction at %RRv\n", *pByte, pInstrGC));
        }
        STAM_COUNTER_ADD(&pVM->patm.s.StatNrOpcodeRead, 1);
        return VINF_SUCCESS;
    }
    return VERR_PATCH_NOT_FOUND;
}

/**
 * Read instruction bytes of the original code that was overwritten by the 5
 * bytes patch jump.
 *
 * @returns VINF_SUCCESS or VERR_PATCH_NOT_FOUND.
 * @param   pVM         The cross context VM structure.
 * @param   GCPtrInstr  GC address of instr
 * @param   pbDst       The output buffer.
 * @param   cbToRead    The maximum number bytes to read.
 * @param   pcbRead     Where to return the acutal number of bytes read.
 */
VMMR3_INT_DECL(int) PATMR3ReadOrgInstr(PVM pVM, RTGCPTR32 GCPtrInstr, uint8_t *pbDst, size_t cbToRead, size_t *pcbRead)
{
    /* Shortcut. */
    if (!PATMIsEnabled(pVM))
        return VERR_PATCH_NOT_FOUND;
    Assert(!HMIsEnabled(pVM));
    if (   GCPtrInstr < pVM->patm.s.pPatchedInstrGCLowest
        || GCPtrInstr > pVM->patm.s.pPatchedInstrGCHighest)
        return VERR_PATCH_NOT_FOUND;

    /** @todo this will not work for aliased pages! (never has, but so far not a problem for us) */

    /*
     * If the patch is enabled and the pointer lies within 5 bytes of this
     * priv instr ptr, then we've got a hit!
     */
    RTGCPTR32     off;
    PPATMPATCHREC pPatchRec = (PPATMPATCHREC)RTAvloU32GetBestFit(&pVM->patm.s.PatchLookupTreeHC->PatchTree,
                                                                 GCPtrInstr, false /*fAbove*/);
    if (   pPatchRec
        && pPatchRec->patch.uState == PATCH_ENABLED
        && (off = GCPtrInstr - pPatchRec->patch.pPrivInstrGC) < pPatchRec->patch.cbPatchJump)
    {
        uint8_t const  *pbSrc = &pPatchRec->patch.aPrivInstr[off];
        uint32_t const  cbMax = pPatchRec->patch.cbPatchJump - off;
        if (cbToRead > cbMax)
            cbToRead = cbMax;
        switch (cbToRead)
        {
            case 5: pbDst[4] = pbSrc[4]; RT_FALL_THRU();
            case 4: pbDst[3] = pbSrc[3]; RT_FALL_THRU();
            case 3: pbDst[2] = pbSrc[2]; RT_FALL_THRU();
            case 2: pbDst[1] = pbSrc[1]; RT_FALL_THRU();
            case 1: pbDst[0] = pbSrc[0];
                break;
            default:
                memcpy(pbDst, pbSrc, cbToRead);
        }
        *pcbRead = cbToRead;

        if (pPatchRec->patch.cbPatchJump == 1)
            Log(("PATMR3ReadOrgInstr: returning opcode %.*Rhxs for instruction at %RX32\n", cbToRead, pbSrc, GCPtrInstr));
        STAM_COUNTER_ADD(&pVM->patm.s.StatNrOpcodeRead, 1);
        return VINF_SUCCESS;
    }

    return VERR_PATCH_NOT_FOUND;
}

/**
 * Disable patch for privileged instruction at specified location
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context point to privileged instruction
 *
 * @note    returns failure if patching is not allowed or possible
 *
 */
VMMR3_INT_DECL(int) PATMR3DisablePatch(PVM pVM, RTRCPTR pInstrGC)
{
    PPATMPATCHREC pPatchRec;
    PPATCHINFO    pPatch;

    Log(("PATMR3DisablePatch: %RRv\n", pInstrGC));
    AssertReturn(!HMIsEnabled(pVM), VERR_PATM_HM_IPE);
    pPatchRec = (PPATMPATCHREC)RTAvloU32Get(&pVM->patm.s.PatchLookupTreeHC->PatchTree, pInstrGC);
    if (pPatchRec)
    {
        int rc = VINF_SUCCESS;

        pPatch = &pPatchRec->patch;

        /* Already disabled? */
        if (pPatch->uState == PATCH_DISABLED)
            return VINF_SUCCESS;

        /* Clear the IDT entries for the patch we're disabling. */
        /* Note: very important as we clear IF in the patch itself */
        /** @todo this needs to be changed */
        if (pPatch->flags & PATMFL_IDTHANDLER)
        {
            uint32_t iGate;

            iGate = TRPMR3QueryGateByHandler(pVM, PATCHCODE_PTR_GC(pPatch));
            if (iGate != (uint32_t)~0)
            {
                TRPMR3SetGuestTrapHandler(pVM, iGate, TRPM_INVALID_HANDLER);
                if (++cIDTHandlersDisabled < 256)
                    LogRel(("PATM: Disabling IDT %x patch handler %RRv\n", iGate, pInstrGC));
            }
        }

        /* Mark the entry with a breakpoint in case somebody else calls it later on (cli patch used as a function, function, trampoline or idt patches) */
        if (    pPatch->pPatchBlockOffset
            &&  pPatch->uState == PATCH_ENABLED)
        {
            Log(("Invalidate patch at %RRv (HC=%RRv)\n", PATCHCODE_PTR_GC(pPatch), PATCHCODE_PTR_HC(pPatch)));
            pPatch->bDirtyOpcode   = *PATCHCODE_PTR_HC(pPatch);
            *PATCHCODE_PTR_HC(pPatch) = 0xCC;
        }

        /* IDT or function patches haven't changed any guest code.  */
        if (pPatch->flags & PATMFL_PATCHED_GUEST_CODE)
        {
            Assert(pPatch->flags & PATMFL_MUST_INSTALL_PATCHJMP);
            Assert(!(pPatch->flags & (PATMFL_DUPLICATE_FUNCTION|PATMFL_IDTHANDLER|PATMFL_TRAMPOLINE|PATMFL_INT3_REPLACEMENT|PATMFL_INT3_REPLACEMENT_BLOCK)));

            if (pPatch->uState != PATCH_REFUSED)
            {
                uint8_t temp[16];

                Assert(pPatch->cbPatchJump < sizeof(temp));

                /* Let's first check if the guest code is still the same. */
                rc = PGMPhysSimpleReadGCPtr(VMMGetCpu0(pVM), temp, pPatch->pPrivInstrGC, pPatch->cbPatchJump);
                Assert(rc == VINF_SUCCESS || rc == VERR_PAGE_TABLE_NOT_PRESENT || rc == VERR_PAGE_NOT_PRESENT);
                if (rc == VINF_SUCCESS)
                {
                    RTRCINTPTR displ = (RTRCUINTPTR)PATCHCODE_PTR_GC(pPatch) - ((RTRCUINTPTR)pPatch->pPrivInstrGC + SIZEOF_NEARJUMP32);

                    if (    temp[0] != 0xE9 /* jmp opcode */
                        ||  *(RTRCINTPTR *)(&temp[1]) != displ
                        )
                    {
                        Log(("PATMR3DisablePatch: Can't disable a patch who's guest code has changed!!\n"));
                        STAM_COUNTER_INC(&pVM->patm.s.StatOverwritten);
                        /* Remove it completely */
                        pPatch->uState = PATCH_DISABLED;    /* don't call PATMR3DisablePatch again */
                        rc = PATMR3RemovePatch(pVM, pInstrGC);
                        AssertRC(rc);
                        return VWRN_PATCH_REMOVED;
                    }
                    patmRemoveJumpToPatch(pVM, pPatch);
                }
                else
                {
                    Log(("PATMR3DisablePatch: unable to disable patch -> mark PATCH_DISABLE_PENDING\n"));
                    pPatch->uState = PATCH_DISABLE_PENDING;
                }
            }
            else
            {
                AssertMsgFailed(("Patch was refused!\n"));
                return VERR_PATCH_ALREADY_DISABLED;
            }
        }
        else
        if (pPatch->flags & (PATMFL_INT3_REPLACEMENT|PATMFL_INT3_REPLACEMENT_BLOCK))
        {
            uint8_t temp[16];

            Assert(pPatch->cbPatchJump < sizeof(temp));

            /* Let's first check if the guest code is still the same. */
            rc = PGMPhysSimpleReadGCPtr(VMMGetCpu0(pVM), temp, pPatch->pPrivInstrGC, pPatch->cbPatchJump);
            Assert(rc == VINF_SUCCESS || rc == VERR_PAGE_TABLE_NOT_PRESENT || rc == VERR_PAGE_NOT_PRESENT);
            if (rc == VINF_SUCCESS)
            {
                if (temp[0] != 0xCC)
                {
                    Log(("PATMR3DisablePatch: Can't disable a patch who's guest code has changed!!\n"));
                    STAM_COUNTER_INC(&pVM->patm.s.StatOverwritten);
                    /* Remove it completely */
                    pPatch->uState = PATCH_DISABLED;    /* don't call PATMR3DisablePatch again */
                    rc = PATMR3RemovePatch(pVM, pInstrGC);
                    AssertRC(rc);
                    return VWRN_PATCH_REMOVED;
                }
                patmDeactivateInt3Patch(pVM, pPatch);
            }
        }

        if (rc == VINF_SUCCESS)
        {
            /* Save old state and mark this one as disabled (so it can be enabled later on). */
            if (pPatch->uState == PATCH_DISABLE_PENDING)
            {
                /* Just to be safe, let's make sure this one can never be reused; the patch might be marked dirty already (int3 at start) */
                pPatch->uState    = PATCH_UNUSABLE;
            }
            else
            if (pPatch->uState != PATCH_DIRTY)
            {
                pPatch->uOldState = pPatch->uState;
                pPatch->uState    = PATCH_DISABLED;
            }
            STAM_COUNTER_ADD(&pVM->patm.s.StatDisabled, 1);
        }

        Log(("PATMR3DisablePatch: disabled patch at %RRv\n", pInstrGC));
        return VINF_SUCCESS;
    }
    Log(("Patch not found!\n"));
    return VERR_PATCH_NOT_FOUND;
}

/**
 * Permanently disable patch for privileged instruction at specified location
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context instruction pointer
 * @param   pConflictAddr  Guest context pointer which conflicts with specified patch
 * @param   pConflictPatch Conflicting patch
 *
 */
static int patmDisableUnusablePatch(PVM pVM, RTRCPTR pInstrGC, RTRCPTR pConflictAddr, PPATCHINFO pConflictPatch)
{
    NOREF(pConflictAddr);
#ifdef PATM_RESOLVE_CONFLICTS_WITH_JUMP_PATCHES
    PATCHINFO            patch;
    DISCPUSTATE          cpu;
    R3PTRTYPE(uint8_t *) pInstrHC;
    uint32_t             cbInstr;
    bool                 disret;
    int                  rc;

    RT_ZERO(patch);
    pInstrHC = patmR3GCVirtToHCVirt(pVM, &patch, pInstrGC);
    disret = patmR3DisInstr(pVM, &patch, pInstrGC, pInstrHC, PATMREAD_ORGCODE, &cpu, &cbInstr);
    /*
     * If it's a 5 byte relative jump, then we can work around the problem by replacing the 32 bits relative offset
     * with one that jumps right into the conflict patch.
     * Otherwise we must disable the conflicting patch to avoid serious problems.
     */
    if (    disret == true
        && (pConflictPatch->flags & PATMFL_CODE32)
        && (cpu.pCurInstr->uOpcode == OP_JMP || (cpu.pCurInstr->fOpType & DISOPTYPE_COND_CONTROLFLOW))
        && (cpu.Param1.fUse & DISUSE_IMMEDIATE32_REL))
    {
        /* Hint patches must be enabled first. */
        if (pConflictPatch->flags & PATMFL_INSTR_HINT)
        {
            Log(("Enabling HINTED patch %RRv\n", pConflictPatch->pPrivInstrGC));
            pConflictPatch->flags &= ~PATMFL_INSTR_HINT;
            rc = PATMR3EnablePatch(pVM, pConflictPatch->pPrivInstrGC);
            Assert(rc == VINF_SUCCESS || rc == VERR_PATCH_NOT_FOUND);
            /* Enabling might fail if the patched code has changed in the meantime. */
            if (rc != VINF_SUCCESS)
                return rc;
        }

        rc = PATMR3InstallPatch(pVM, pInstrGC, PATMFL_CODE32 | PATMFL_JUMP_CONFLICT);
        if (RT_SUCCESS(rc))
        {
            Log(("PATM -> CONFLICT: Installed JMP patch for patch conflict at %RRv\n", pInstrGC));
            STAM_COUNTER_INC(&pVM->patm.s.StatFixedConflicts);
            return VINF_SUCCESS;
        }
    }
#else
    RT_NOREF_PV(pInstrGC);
#endif

    if (pConflictPatch->opcode == OP_CLI)
    {
        /* Turn it into an int3 patch; our GC trap handler will call the generated code manually. */
        Log(("PATM -> CONFLICT: Found active patch at instruction %RRv with target %RRv -> turn into int 3 patch!!\n", pInstrGC, pConflictPatch->pPrivInstrGC));
        int rc =  PATMR3DisablePatch(pVM, pConflictPatch->pPrivInstrGC);
        if (rc == VWRN_PATCH_REMOVED)
            return VINF_SUCCESS;
        if (RT_SUCCESS(rc))
        {
            pConflictPatch->flags &= ~(PATMFL_MUST_INSTALL_PATCHJMP|PATMFL_INSTR_HINT);
            pConflictPatch->flags |= PATMFL_INT3_REPLACEMENT_BLOCK;
            rc = PATMR3EnablePatch(pVM, pConflictPatch->pPrivInstrGC);
            if (rc == VERR_PATCH_NOT_FOUND)
                return VINF_SUCCESS;    /* removed already */

            AssertRC(rc);
            if (RT_SUCCESS(rc))
            {
                STAM_COUNTER_INC(&pVM->patm.s.StatInt3Callable);
                return VINF_SUCCESS;
            }
        }
        /* else turned into unusable patch (see below) */
    }
    else
    {
        Log(("PATM -> CONFLICT: Found active patch at instruction %RRv with target %RRv -> DISABLING it!!\n", pInstrGC, pConflictPatch->pPrivInstrGC));
        int rc = PATMR3DisablePatch(pVM, pConflictPatch->pPrivInstrGC);
        if (rc == VWRN_PATCH_REMOVED)
            return VINF_SUCCESS;
    }

    /* No need to monitor the code anymore. */
    if (pConflictPatch->flags & PATMFL_CODE_MONITORED)
    {
        int rc = patmRemovePatchPages(pVM, pConflictPatch);
        AssertRC(rc);
    }
    pConflictPatch->uState = PATCH_UNUSABLE;
    STAM_COUNTER_INC(&pVM->patm.s.StatUnusable);
    return VERR_PATCH_DISABLED;
}

/**
 * Enable patch for privileged instruction at specified location
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context point to privileged instruction
 *
 * @note    returns failure if patching is not allowed or possible
 *
 */
VMMR3_INT_DECL(int) PATMR3EnablePatch(PVM pVM, RTRCPTR pInstrGC)
{
    PPATMPATCHREC pPatchRec;
    PPATCHINFO    pPatch;

    Log(("PATMR3EnablePatch %RRv\n", pInstrGC));
    AssertReturn(!HMIsEnabled(pVM), VERR_PATM_HM_IPE);
    pPatchRec = (PPATMPATCHREC)RTAvloU32Get(&pVM->patm.s.PatchLookupTreeHC->PatchTree, pInstrGC);
    if (pPatchRec)
    {
        int rc = VINF_SUCCESS;

        pPatch = &pPatchRec->patch;

        if (pPatch->uState == PATCH_DISABLED)
        {
            if (pPatch->flags & PATMFL_MUST_INSTALL_PATCHJMP)
            {
                Assert(!(pPatch->flags & PATMFL_PATCHED_GUEST_CODE));
                uint8_t temp[16];

                Assert(pPatch->cbPatchJump < sizeof(temp));

                /* Let's first check if the guest code is still the same. */
                int rc2 = PGMPhysSimpleReadGCPtr(VMMGetCpu0(pVM), temp, pPatch->pPrivInstrGC, pPatch->cbPatchJump);
                AssertRC(rc2);
                if (rc2 == VINF_SUCCESS)
                {
                    if (memcmp(temp, pPatch->aPrivInstr, pPatch->cbPatchJump))
                    {
                        Log(("PATMR3EnablePatch: Can't enable a patch who's guest code has changed!!\n"));
                        STAM_COUNTER_INC(&pVM->patm.s.StatOverwritten);
                        /* Remove it completely */
                        rc = PATMR3RemovePatch(pVM, pInstrGC);
                        AssertRC(rc);
                        return VERR_PATCH_NOT_FOUND;
                    }

                    PATMP2GLOOKUPREC cacheRec;
                    RT_ZERO(cacheRec);
                    cacheRec.pPatch = pPatch;

                    rc2 = patmGenJumpToPatch(pVM, pPatch, &cacheRec, false);
                    /* Free leftover lock if any. */
                    if (cacheRec.Lock.pvMap)
                    {
                        PGMPhysReleasePageMappingLock(pVM, &cacheRec.Lock);
                        cacheRec.Lock.pvMap = NULL;
                    }
                    AssertRC(rc2);
                    if (RT_FAILURE(rc2))
                        return rc2;

#ifdef DEBUG
                    {
                        DISCPUSTATE cpu;
                        char szOutput[256];
                        uint32_t cbInstr;
                        uint32_t i = 0;
                        bool disret;
                        while(i < pPatch->cbPatchJump)
                        {
                            disret = patmR3DisInstrToStr(pVM, pPatch, pPatch->pPrivInstrGC + i, NULL, PATMREAD_ORGCODE,
                                                         &cpu, &cbInstr, szOutput, sizeof(szOutput));
                            Log(("Renewed patch instr: %s", szOutput));
                            i += cbInstr;
                        }
                    }
#endif
                }
            }
            else
            if (pPatch->flags & (PATMFL_INT3_REPLACEMENT|PATMFL_INT3_REPLACEMENT_BLOCK))
            {
                uint8_t temp[16];

                Assert(pPatch->cbPatchJump < sizeof(temp));

                /* Let's first check if the guest code is still the same. */
                int rc2 = PGMPhysSimpleReadGCPtr(VMMGetCpu0(pVM), temp, pPatch->pPrivInstrGC, pPatch->cbPatchJump);
                AssertRC(rc2);

                if (memcmp(temp, pPatch->aPrivInstr, pPatch->cbPatchJump))
                {
                    Log(("PATMR3EnablePatch: Can't enable a patch who's guest code has changed!!\n"));
                    STAM_COUNTER_INC(&pVM->patm.s.StatOverwritten);
                    rc = PATMR3RemovePatch(pVM, pInstrGC);
                    AssertRC(rc);
                    return VERR_PATCH_NOT_FOUND;
                }

                rc2 = patmActivateInt3Patch(pVM, pPatch);
                if (RT_FAILURE(rc2))
                    return rc2;
            }

            pPatch->uState = pPatch->uOldState; //restore state

            /* Restore the entry breakpoint with the original opcode (see PATMR3DisablePatch). */
            if (pPatch->pPatchBlockOffset)
                *PATCHCODE_PTR_HC(pPatch) = pPatch->bDirtyOpcode;

            STAM_COUNTER_ADD(&pVM->patm.s.StatEnabled, 1);
        }
        else
            Log(("PATMR3EnablePatch: Unable to enable patch %RRv with state %d\n", pInstrGC, pPatch->uState));

        return rc;
    }
    return VERR_PATCH_NOT_FOUND;
}

/**
 * Remove patch for privileged instruction at specified location
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pPatchRec       Patch record
 * @param   fForceRemove    Remove *all* patches
 */
int patmR3RemovePatch(PVM pVM, PPATMPATCHREC pPatchRec, bool fForceRemove)
{
    PPATCHINFO    pPatch;

    pPatch = &pPatchRec->patch;

    /* Strictly forbidden to remove such patches. There can be dependencies!! */
    if (!fForceRemove && (pPatch->flags & (PATMFL_DUPLICATE_FUNCTION|PATMFL_CODE_REFERENCED)))
    {
        Log(("PATMRemovePatch %RRv REFUSED!\n", pPatch->pPrivInstrGC));
        return VERR_ACCESS_DENIED;
    }
    Log(("PATMRemovePatch %RRv\n", pPatch->pPrivInstrGC));

    /* Note: NEVER EVER REUSE PATCH MEMORY */
    /* Note: PATMR3DisablePatch puts a breakpoint (0xCC) at the entry of this patch */

    if (pPatchRec->patch.pPatchBlockOffset)
    {
        PAVLOU32NODECORE pNode;

        pNode = RTAvloU32Remove(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPatchAddr, pPatchRec->patch.pPatchBlockOffset);
        Assert(pNode);
    }

    if (pPatchRec->patch.flags & PATMFL_CODE_MONITORED)
    {
        int rc = patmRemovePatchPages(pVM, &pPatchRec->patch);
        AssertRC(rc);
    }

#ifdef VBOX_WITH_STATISTICS
    if (PATM_STAT_INDEX_IS_VALID(pPatchRec->patch.uPatchIdx))
    {
        STAMR3DeregisterF(pVM->pUVM, "/PATM/Stats/Patch/0x%RRv", pPatchRec->patch.pPrivInstrGC);
        STAMR3DeregisterF(pVM->pUVM, "/PATM/PatchBD/0x%RRv*", pPatchRec->patch.pPrivInstrGC);
    }
#endif

    /* Note: no need to free Guest2PatchAddrTree as those records share memory with Patch2GuestAddrTree records. */
    patmEmptyTreeU32(pVM, &pPatch->Patch2GuestAddrTree);
    pPatch->nrPatch2GuestRecs = 0;
    Assert(pPatch->Patch2GuestAddrTree == 0);

    patmEmptyTree(pVM, &pPatch->FixupTree);
    pPatch->nrFixups = 0;
    Assert(pPatch->FixupTree == 0);

    if (pPatchRec->patch.pTempInfo)
        MMR3HeapFree(pPatchRec->patch.pTempInfo);

    /* Note: might fail, because it has already been removed (e.g. during reset). */
    RTAvloU32Remove(&pVM->patm.s.PatchLookupTreeHC->PatchTree, pPatchRec->Core.Key);

    /* Free the patch record */
    MMHyperFree(pVM, pPatchRec);
    return VINF_SUCCESS;
}

/**
 * RTAvlU32DoWithAll() worker.
 * Checks whether the current trampoline instruction is the jump to the target patch
 * and updates the displacement to jump to the new target.
 *
 * @returns VBox status code.
 * @retval  VERR_ALREADY_EXISTS if the jump was found.
 * @param   pNode    The current patch to guest record to check.
 * @param   pvUser   The refresh state.
 */
static DECLCALLBACK(int) patmR3PatchRefreshFindTrampolinePatch(PAVLU32NODECORE pNode, void *pvUser)
{
    PRECPATCHTOGUEST  pPatch2GuestRec = (PRECPATCHTOGUEST)pNode;
    PPATMREFRESHPATCH pRefreshPatchState = (PPATMREFRESHPATCH)pvUser;
    PVM               pVM = pRefreshPatchState->pVM;

    uint8_t *pPatchInstr = (uint8_t *)(pVM->patm.s.pPatchMemHC + pPatch2GuestRec->Core.Key);

    /*
     * Check if the patch instruction starts with a jump.
     * ASSUMES that there is no other patch to guest record that starts
     * with a jump.
     */
    if (*pPatchInstr == 0xE9)
    {
        /* Jump found, update the displacement. */
        RTRCPTR pPatchTargetGC = patmGuestGCPtrToPatchGCPtr(pVM, pRefreshPatchState->pPatchRec,
                                                            pRefreshPatchState->pPatchTrampoline->pPrivInstrGC);
        int32_t displ =  pPatchTargetGC - (pVM->patm.s.pPatchMemGC + pPatch2GuestRec->Core.Key + SIZEOF_NEARJUMP32);

        LogFlow(("Updating trampoline patch new patch target %RRv, new displacment %d (old was %d)\n",
                 pPatchTargetGC, displ, *(uint32_t *)&pPatchInstr[1]));

        *(uint32_t *)&pPatchInstr[1] = displ;
        return VERR_ALREADY_EXISTS; /** @todo better return code */
    }

    return VINF_SUCCESS;
}

/**
 * Attempt to refresh the patch by recompiling its entire code block
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pPatchRec       Patch record
 */
int patmR3RefreshPatch(PVM pVM, PPATMPATCHREC pPatchRec)
{
    PPATCHINFO  pPatch;
    int         rc;
    RTRCPTR     pInstrGC = pPatchRec->patch.pPrivInstrGC;
    PTRAMPREC   pTrampolinePatchesHead = NULL;

    Log(("patmR3RefreshPatch: attempt to refresh patch at %RRv\n", pInstrGC));

    pPatch = &pPatchRec->patch;
    AssertReturn(pPatch->flags & (PATMFL_DUPLICATE_FUNCTION|PATMFL_IDTHANDLER|PATMFL_TRAPHANDLER), VERR_PATCHING_REFUSED);
    if (pPatch->flags & PATMFL_EXTERNAL_JUMP_INSIDE)
    {
        if (!pPatch->pTrampolinePatchesHead)
        {
            /*
             * It is sometimes possible that there are trampoline patches to this patch
             * but they are not recorded (after a saved state load for example).
             * Refuse to refresh those patches.
             * Can hurt performance in theory if the patched code is modified by the guest
             * and is executed often. However most of the time states are saved after the guest
             * code was modified and is not updated anymore afterwards so this shouldn't be a
             * big problem.
             */
            Log(("patmR3RefreshPatch: refused because external jumps to this patch exist but the jumps are not recorded\n"));
            return VERR_PATCHING_REFUSED;
        }
        Log(("patmR3RefreshPatch: external jumps to this patch exist, updating\n"));
        pTrampolinePatchesHead = pPatch->pTrampolinePatchesHead;
    }

    /* Note: quite ugly to enable/disable/remove/insert old and new patches, but there's no easy way around it. */

    rc = PATMR3DisablePatch(pVM, pInstrGC);
    AssertRC(rc);

    /* Kick it out of the lookup tree to make sure PATMR3InstallPatch doesn't fail (hack alert) */
    RTAvloU32Remove(&pVM->patm.s.PatchLookupTreeHC->PatchTree, pPatchRec->Core.Key);
#ifdef VBOX_WITH_STATISTICS
    if (PATM_STAT_INDEX_IS_VALID(pPatchRec->patch.uPatchIdx))
    {
        STAMR3DeregisterF(pVM->pUVM, "/PATM/Stats/Patch/0x%RRv", pPatchRec->patch.pPrivInstrGC);
        STAMR3DeregisterF(pVM->pUVM, "/PATM/PatchBD/0x%RRv*", pPatchRec->patch.pPrivInstrGC);
    }
#endif

    /** Note: We don't attempt to reuse patch memory here as it's quite common that the new code block requires more memory. */

    /* Attempt to install a new patch. */
    rc = PATMR3InstallPatch(pVM, pInstrGC, pPatch->flags & (PATMFL_CODE32|PATMFL_IDTHANDLER|PATMFL_INTHANDLER|PATMFL_TRAPHANDLER|PATMFL_DUPLICATE_FUNCTION|PATMFL_TRAPHANDLER_WITH_ERRORCODE|PATMFL_IDTHANDLER_WITHOUT_ENTRYPOINT));
    if (RT_SUCCESS(rc))
    {
        RTRCPTR         pPatchTargetGC;
        PPATMPATCHREC   pNewPatchRec;

        /* Determine target address in new patch */
        pPatchTargetGC = PATMR3QueryPatchGCPtr(pVM, pInstrGC);
        Assert(pPatchTargetGC);
        if (!pPatchTargetGC)
        {
            rc = VERR_PATCHING_REFUSED;
            goto failure;
        }

        /* Reset offset into patch memory to put the next code blocks right at the beginning. */
        pPatch->uCurPatchOffset   = 0;

        /* insert jump to new patch in old patch block */
        rc = patmPatchGenPatchJump(pVM, pPatch, pInstrGC, pPatchTargetGC, false /* no lookup record */);
        if (RT_FAILURE(rc))
            goto failure;

        pNewPatchRec = (PPATMPATCHREC)RTAvloU32Get(&pVM->patm.s.PatchLookupTreeHC->PatchTree, pInstrGC);
        Assert(pNewPatchRec); /* can't fail */

        /* Remove old patch (only do that when everything is finished) */
        int rc2 = patmR3RemovePatch(pVM, pPatchRec, true /* force removal */);
        AssertRC(rc2);

        /* Put the new patch back into the tree, because removing the old one kicked this one out. (hack alert) */
        bool fInserted = RTAvloU32Insert(&pVM->patm.s.PatchLookupTreeHC->PatchTree, &pNewPatchRec->Core);
        Assert(fInserted); NOREF(fInserted);

        Log(("PATM: patmR3RefreshPatch: succeeded to refresh patch at %RRv \n", pInstrGC));
        STAM_COUNTER_INC(&pVM->patm.s.StatPatchRefreshSuccess);

        /* Used by another patch, so don't remove it! */
        pNewPatchRec->patch.flags |= PATMFL_CODE_REFERENCED;

        if (pTrampolinePatchesHead)
        {
            /* Update all trampoline patches to jump to the new patch. */
            PTRAMPREC pTrampRec = NULL;
            PATMREFRESHPATCH RefreshPatch;

            RefreshPatch.pVM = pVM;
            RefreshPatch.pPatchRec = &pNewPatchRec->patch;

            pTrampRec = pTrampolinePatchesHead;

            while (pTrampRec)
            {
                PPATCHINFO pPatchTrampoline = &pTrampRec->pPatchTrampoline->patch;

                RefreshPatch.pPatchTrampoline = pPatchTrampoline;
                /*
                 * We have to find the right patch2guest record because there might be others
                 * for statistics.
                 */
                rc = RTAvlU32DoWithAll(&pPatchTrampoline->Patch2GuestAddrTree, true,
                                       patmR3PatchRefreshFindTrampolinePatch, &RefreshPatch);
                Assert(rc == VERR_ALREADY_EXISTS);
                rc = VINF_SUCCESS;
                pTrampRec = pTrampRec->pNext;
            }
            pNewPatchRec->patch.pTrampolinePatchesHead = pTrampolinePatchesHead;
            pNewPatchRec->patch.flags                 |= PATMFL_EXTERNAL_JUMP_INSIDE;
            /* Clear the list of trampoline patches for the old patch (safety precaution). */
            pPatchRec->patch.pTrampolinePatchesHead = NULL;
        }
    }

failure:
    if (RT_FAILURE(rc))
    {
        LogRel(("PATM: patmR3RefreshPatch: failed to refresh patch at %RRv. Reactiving old one. \n", pInstrGC));

        /* Remove the new inactive patch */
        rc = PATMR3RemovePatch(pVM, pInstrGC);
        AssertRC(rc);

        /* Put the old patch back into the tree (or else it won't be saved) (hack alert) */
        bool fInserted = RTAvloU32Insert(&pVM->patm.s.PatchLookupTreeHC->PatchTree, &pPatchRec->Core);
        Assert(fInserted); NOREF(fInserted);

        /* Enable again in case the dirty instruction is near the end and there are safe code paths. */
        int rc2 = PATMR3EnablePatch(pVM, pInstrGC);
        AssertRC(rc2);

        STAM_COUNTER_INC(&pVM->patm.s.StatPatchRefreshFailed);
    }
    return rc;
}

/**
 * Find patch for privileged instruction at specified location
 *
 * @returns Patch structure pointer if found; else NULL
 * @param   pVM           The cross context VM structure.
 * @param   pInstrGC      Guest context point to instruction that might lie
 *                        within 5 bytes of an existing patch jump
 * @param   fIncludeHints Include hinted patches or not
 */
PPATCHINFO patmFindActivePatchByEntrypoint(PVM pVM, RTRCPTR pInstrGC, bool fIncludeHints)
{
    PPATMPATCHREC pPatchRec = (PPATMPATCHREC)RTAvloU32GetBestFit(&pVM->patm.s.PatchLookupTreeHC->PatchTree, pInstrGC, false);
    /* if the patch is enabled, the pointer is not identical to the privileged patch ptr and it lies within 5 bytes of this priv instr ptr, then we've got a hit! */
    if (pPatchRec)
    {
        if (    pPatchRec->patch.uState == PATCH_ENABLED
            && (pPatchRec->patch.flags & PATMFL_PATCHED_GUEST_CODE)
            &&  pInstrGC > pPatchRec->patch.pPrivInstrGC
            &&  pInstrGC < pPatchRec->patch.pPrivInstrGC + pPatchRec->patch.cbPatchJump)
        {
            Log(("Found active patch at %RRv (org %RRv)\n", pInstrGC, pPatchRec->patch.pPrivInstrGC));
            return &pPatchRec->patch;
        }
        else
        if (    fIncludeHints
            &&  pPatchRec->patch.uState == PATCH_DISABLED
            &&  (pPatchRec->patch.flags & PATMFL_INSTR_HINT)
            &&  pInstrGC > pPatchRec->patch.pPrivInstrGC
            &&  pInstrGC < pPatchRec->patch.pPrivInstrGC + pPatchRec->patch.cbPatchJump)
        {
            Log(("Found HINT patch at %RRv (org %RRv)\n", pInstrGC, pPatchRec->patch.pPrivInstrGC));
            return &pPatchRec->patch;
        }
    }
    return NULL;
}

/**
 * Checks whether the GC address is inside a generated patch jump
 *
 * @returns true -> yes, false -> no
 * @param   pVM         The cross context VM structure.
 * @param   pAddr       Guest context address.
 * @param   pPatchAddr  Guest context patch address (if true).
 */
VMMR3_INT_DECL(bool) PATMR3IsInsidePatchJump(PVM pVM, RTRCPTR pAddr, PRTGCPTR32 pPatchAddr)
{
    RTRCPTR addr;
    PPATCHINFO pPatch;

    Assert(!HMIsEnabled(pVM));
    if (PATMIsEnabled(pVM) == false)
        return false;

    if (pPatchAddr == NULL)
        pPatchAddr = &addr;

    *pPatchAddr = 0;

    pPatch = patmFindActivePatchByEntrypoint(pVM, pAddr);
    if (pPatch)
        *pPatchAddr = pPatch->pPrivInstrGC;

    return *pPatchAddr == 0 ? false : true;
}

/**
 * Remove patch for privileged instruction at specified location
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context point to privileged instruction
 *
 * @note    returns failure if patching is not allowed or possible
 *
 */
VMMR3_INT_DECL(int) PATMR3RemovePatch(PVM pVM, RTRCPTR pInstrGC)
{
    PPATMPATCHREC pPatchRec;

    AssertReturn(!HMIsEnabled(pVM), VERR_PATM_HM_IPE);
    pPatchRec = (PPATMPATCHREC)RTAvloU32Get(&pVM->patm.s.PatchLookupTreeHC->PatchTree, pInstrGC);
    if (pPatchRec)
    {
        int rc = PATMR3DisablePatch(pVM, pInstrGC);
        if (rc == VWRN_PATCH_REMOVED)
            return VINF_SUCCESS;

        return patmR3RemovePatch(pVM, pPatchRec, false);
    }
    AssertFailed();
    return VERR_PATCH_NOT_FOUND;
}

/**
 * Mark patch as dirty
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch record
 *
 * @note    returns failure if patching is not allowed or possible
 *
 */
static int patmR3MarkDirtyPatch(PVM pVM, PPATCHINFO pPatch)
{
    if (pPatch->pPatchBlockOffset)
    {
        Log(("Invalidate patch at %RRv (HC=%RRv)\n", PATCHCODE_PTR_GC(pPatch), PATCHCODE_PTR_HC(pPatch)));
        pPatch->bDirtyOpcode   = *PATCHCODE_PTR_HC(pPatch);
        *PATCHCODE_PTR_HC(pPatch) = 0xCC;
    }

    STAM_COUNTER_INC(&pVM->patm.s.StatDirty);
    /* Put back the replaced instruction. */
    int rc = PATMR3DisablePatch(pVM, pPatch->pPrivInstrGC);
    if (rc == VWRN_PATCH_REMOVED)
        return VINF_SUCCESS;

    /* Note: we don't restore patch pages for patches that are not enabled! */
    /* Note: be careful when changing this behaviour!! */

    /* The patch pages are no longer marked for self-modifying code detection */
    if (pPatch->flags & PATMFL_CODE_MONITORED)
    {
        rc = patmRemovePatchPages(pVM, pPatch);
        AssertRCReturn(rc, rc);
    }
    pPatch->uState = PATCH_DIRTY;

    /* Paranoia; make sure this patch is not somewhere in the callchain, so prevent ret instructions from succeeding. */
    CTXSUFF(pVM->patm.s.pGCState)->Psp = PATM_STACK_SIZE;

    return VINF_SUCCESS;
}

/**
 * Query the corresponding GC instruction pointer from a pointer inside the patch block itself
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch block structure pointer
 * @param   pPatchGC    GC address in patch block
 */
RTRCPTR patmPatchGCPtr2GuestGCPtr(PVM pVM, PPATCHINFO pPatch, RCPTRTYPE(uint8_t *) pPatchGC)
{
    Assert(pPatch->Patch2GuestAddrTree);
    /* Get the closest record from below. */
    PRECPATCHTOGUEST pPatchToGuestRec = (PRECPATCHTOGUEST)RTAvlU32GetBestFit(&pPatch->Patch2GuestAddrTree, pPatchGC - pVM->patm.s.pPatchMemGC, false);
    if (pPatchToGuestRec)
        return pPatchToGuestRec->pOrgInstrGC;

    return 0;
}

/**
 * Converts Guest code GC ptr to Patch code GC ptr (if found)
 *
 * @returns corresponding GC pointer in patch block
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Current patch block pointer
 * @param   pInstrGC    Guest context pointer to privileged instruction
 *
 */
RTRCPTR patmGuestGCPtrToPatchGCPtr(PVM pVM, PPATCHINFO pPatch, RCPTRTYPE(uint8_t*) pInstrGC)
{
    if (pPatch->Guest2PatchAddrTree)
    {
        PRECGUESTTOPATCH pGuestToPatchRec = (PRECGUESTTOPATCH)RTAvlU32Get(&pPatch->Guest2PatchAddrTree, pInstrGC);
        if (pGuestToPatchRec)
            return pVM->patm.s.pPatchMemGC + pGuestToPatchRec->PatchOffset;
    }

    return 0;
}

#ifdef PATM_RESOLVE_CONFLICTS_WITH_JUMP_PATCHES
/**
 * Converts Guest code GC ptr to Patch code GC ptr (if found)
 *
 * @returns corresponding GC pointer in patch block
 * @param   pVM         The cross context VM structure.
 * @param   pInstrGC    Guest context pointer to privileged instruction
 */
static RTRCPTR patmR3GuestGCPtrToPatchGCPtrSimple(PVM pVM, RCPTRTYPE(uint8_t*) pInstrGC)
{
    PPATMPATCHREC pPatchRec = (PPATMPATCHREC)RTAvloU32GetBestFit(&pVM->patm.s.PatchLookupTreeHC->PatchTree, pInstrGC, false);
    if (pPatchRec && pPatchRec->patch.uState == PATCH_ENABLED && pInstrGC >= pPatchRec->patch.pPrivInstrGC)
        return patmGuestGCPtrToPatchGCPtr(pVM, &pPatchRec->patch, pInstrGC);
    return NIL_RTRCPTR;
}
#endif

/**
 * Converts Guest code GC ptr to Patch code GC ptr (or nearest from below if no
 * identical match)
 *
 * @returns corresponding GC pointer in patch block
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Current patch block pointer
 * @param   pInstrGC    Guest context pointer to privileged instruction
 *
 */
RTRCPTR patmGuestGCPtrToClosestPatchGCPtr(PVM pVM, PPATCHINFO pPatch, RCPTRTYPE(uint8_t*) pInstrGC)
{
    PRECGUESTTOPATCH pGuestToPatchRec = (PRECGUESTTOPATCH)RTAvlU32GetBestFit(&pPatch->Guest2PatchAddrTree, pInstrGC, false);
    if (pGuestToPatchRec)
        return pVM->patm.s.pPatchMemGC + pGuestToPatchRec->PatchOffset;
    return NIL_RTRCPTR;
}

/**
 * Query the corresponding GC instruction pointer from a pointer inside the patch block itself
 *
 * @returns original GC instruction pointer or 0 if not found
 * @param   pVM         The cross context VM structure.
 * @param   pPatchGC    GC address in patch block
 * @param   pEnmState   State of the translated address (out)
 *
 */
VMMR3_INT_DECL(RTRCPTR) PATMR3PatchToGCPtr(PVM pVM, RTRCPTR pPatchGC, PATMTRANSSTATE *pEnmState)
{
    PPATMPATCHREC pPatchRec;
    void         *pvPatchCoreOffset;
    RTRCPTR       pPrivInstrGC;

    Assert(PATMIsPatchGCAddr(pVM, pPatchGC));
    Assert(!HMIsEnabled(pVM));
    pvPatchCoreOffset = RTAvloU32GetBestFit(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPatchAddr, pPatchGC - pVM->patm.s.pPatchMemGC, false);
    if (pvPatchCoreOffset == 0)
    {
        Log(("PATMR3PatchToGCPtr failed for %RRv offset %x\n", pPatchGC, pPatchGC - pVM->patm.s.pPatchMemGC));
        return 0;
    }
    pPatchRec = PATM_PATCHREC_FROM_COREOFFSET(pvPatchCoreOffset);
    pPrivInstrGC = patmPatchGCPtr2GuestGCPtr(pVM, &pPatchRec->patch, pPatchGC);
    if (pEnmState)
    {
        AssertMsg(pPrivInstrGC && (     pPatchRec->patch.uState == PATCH_ENABLED
                                    ||  pPatchRec->patch.uState == PATCH_DIRTY
                                    ||  pPatchRec->patch.uState == PATCH_DISABLE_PENDING
                                    ||  pPatchRec->patch.uState == PATCH_UNUSABLE),
                  ("pPrivInstrGC=%RRv uState=%d\n", pPrivInstrGC, pPatchRec->patch.uState));

        if (    !pPrivInstrGC
            ||   pPatchRec->patch.uState == PATCH_UNUSABLE
            ||   pPatchRec->patch.uState == PATCH_REFUSED)
        {
            pPrivInstrGC = 0;
            *pEnmState = PATMTRANS_FAILED;
        }
        else
        if (pVM->patm.s.pGCStateHC->GCPtrInhibitInterrupts == pPrivInstrGC)
        {
            *pEnmState = PATMTRANS_INHIBITIRQ;
        }
        else
        if (    pPatchRec->patch.uState == PATCH_ENABLED
            && !(pPatchRec->patch.flags & (PATMFL_DUPLICATE_FUNCTION|PATMFL_IDTHANDLER|PATMFL_TRAMPOLINE))
            &&  pPrivInstrGC > pPatchRec->patch.pPrivInstrGC
            &&  pPrivInstrGC < pPatchRec->patch.pPrivInstrGC + pPatchRec->patch.cbPatchJump)
        {
            *pEnmState = PATMTRANS_OVERWRITTEN;
        }
        else
        if (patmFindActivePatchByEntrypoint(pVM, pPrivInstrGC))
        {
            *pEnmState = PATMTRANS_OVERWRITTEN;
        }
        else
        if (pPrivInstrGC == pPatchRec->patch.pPrivInstrGC)
        {
            *pEnmState = PATMTRANS_PATCHSTART;
        }
        else
            *pEnmState = PATMTRANS_SAFE;
    }
    return pPrivInstrGC;
}

/**
 * Returns the GC pointer of the patch for the specified GC address
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pAddrGC     Guest context address
 */
VMMR3_INT_DECL(RTRCPTR) PATMR3QueryPatchGCPtr(PVM pVM, RTRCPTR pAddrGC)
{
    PPATMPATCHREC pPatchRec;

    Assert(!HMIsEnabled(pVM));

    /* Find the patch record. */
    pPatchRec = (PPATMPATCHREC)RTAvloU32Get(&pVM->patm.s.PatchLookupTreeHC->PatchTree, pAddrGC);
    /** @todo we should only use patches that are enabled! always did this, but it's incorrect! */
    if (pPatchRec && (pPatchRec->patch.uState == PATCH_ENABLED || pPatchRec->patch.uState == PATCH_DIRTY))
        return PATCHCODE_PTR_GC(&pPatchRec->patch);
    return NIL_RTRCPTR;
}

/**
 * Attempt to recover dirty instructions
 *
 * @returns VBox status code.
 * @param   pVM                 The cross context VM structure.
 * @param   pCtx                Pointer to the guest CPU context.
 * @param   pPatch              Patch record.
 * @param   pPatchToGuestRec    Patch to guest address record.
 * @param   pEip                GC pointer of trapping instruction.
 */
static int patmR3HandleDirtyInstr(PVM pVM, PCPUMCTX pCtx, PPATMPATCHREC pPatch, PRECPATCHTOGUEST pPatchToGuestRec, RTRCPTR pEip)
{
    DISCPUSTATE  CpuOld, CpuNew;
    uint8_t     *pPatchInstrHC, *pCurPatchInstrHC;
    int          rc;
    RTRCPTR      pCurInstrGC, pCurPatchInstrGC;
    uint32_t     cbDirty;
    PRECPATCHTOGUEST pRec;
    RTRCPTR const pOrgInstrGC = pPatchToGuestRec->pOrgInstrGC;
    PVMCPU       pVCpu = VMMGetCpu0(pVM);
    Log(("patmR3HandleDirtyInstr: dirty instruction at %RRv (%RRv)\n", pEip, pOrgInstrGC));

    pRec             = pPatchToGuestRec;
    pCurInstrGC      = pOrgInstrGC;
    pCurPatchInstrGC = pEip;
    cbDirty          = 0;
    pPatchInstrHC    = patmPatchGCPtr2PatchHCPtr(pVM, pCurPatchInstrGC);

    /* Find all adjacent dirty instructions */
    while (true)
    {
        if (pRec->fJumpTarget)
        {
            LogRel(("PATM: patmR3HandleDirtyInstr: dirty instruction at %RRv (%RRv) ignored, because instruction in function was reused as target of jump\n", pEip, pOrgInstrGC));
            pRec->fDirty = false;
            return VERR_PATCHING_REFUSED;
        }

        /* Restore original instruction opcode byte so we can check if the write was indeed safe. */
        pCurPatchInstrHC  = patmPatchGCPtr2PatchHCPtr(pVM, pCurPatchInstrGC);
        *pCurPatchInstrHC = pRec->u8DirtyOpcode;

        /* Only harmless instructions are acceptable. */
        rc = CPUMR3DisasmInstrCPU(pVM, pVCpu, pCtx, pCurPatchInstrGC, &CpuOld, 0);
        if (    RT_FAILURE(rc)
            ||  !(CpuOld.pCurInstr->fOpType & DISOPTYPE_HARMLESS))
        {
            if (RT_SUCCESS(rc))
                cbDirty += CpuOld.cbInstr;
            else
            if (!cbDirty)
                cbDirty = 1;
            break;
        }

#ifdef DEBUG
        char szBuf[256];
        DBGFR3DisasInstrEx(pVM->pUVM, pVCpu->idCpu, pCtx->cs.Sel, pCurPatchInstrGC, DBGF_DISAS_FLAGS_DEFAULT_MODE,
                           szBuf, sizeof(szBuf), NULL);
        Log(("DIRTY: %s\n", szBuf));
#endif
        /* Mark as clean; if we fail we'll let it always fault. */
        pRec->fDirty      = false;

        /* Remove old lookup record. */
        patmr3RemoveP2GLookupRecord(pVM, &pPatch->patch, pCurPatchInstrGC);
        pPatchToGuestRec = NULL;

        pCurPatchInstrGC += CpuOld.cbInstr;
        cbDirty          += CpuOld.cbInstr;

        /* Let's see if there's another dirty instruction right after. */
        pRec = (PRECPATCHTOGUEST)RTAvlU32GetBestFit(&pPatch->patch.Patch2GuestAddrTree, pCurPatchInstrGC - pVM->patm.s.pPatchMemGC, true);
        if (!pRec || !pRec->fDirty)
            break;  /* no more dirty instructions */

        /* In case of complex instructions the next guest instruction could be quite far off. */
        pCurPatchInstrGC = pRec->Core.Key + pVM->patm.s.pPatchMemGC;
    }

    if (    RT_SUCCESS(rc)
        &&  (CpuOld.pCurInstr->fOpType & DISOPTYPE_HARMLESS)
       )
    {
        uint32_t cbLeft;

        pCurPatchInstrHC = pPatchInstrHC;
        pCurPatchInstrGC = pEip;
        cbLeft           = cbDirty;

        while (cbLeft && RT_SUCCESS(rc))
        {
            bool fValidInstr;

            rc = CPUMR3DisasmInstrCPU(pVM, pVCpu, pCtx, pCurInstrGC, &CpuNew, 0);

            fValidInstr = !!(CpuNew.pCurInstr->fOpType & DISOPTYPE_HARMLESS);
            if (    !fValidInstr
                &&  (CpuNew.pCurInstr->fOpType & DISOPTYPE_RELATIVE_CONTROLFLOW)
               )
            {
                RTRCPTR pTargetGC = PATMResolveBranch(&CpuNew, pCurInstrGC);

                if (    pTargetGC >= pOrgInstrGC
                    &&  pTargetGC <= pOrgInstrGC + cbDirty
                   )
                {
                    /* A relative jump to an instruction inside or to the end of the dirty block is acceptable. */
                    fValidInstr = true;
                }
            }

            /* If the instruction is completely harmless (which implies a 1:1 patch copy). */
            if (    rc == VINF_SUCCESS
                &&  CpuNew.cbInstr <= cbLeft /* must still fit */
                &&  fValidInstr
               )
            {
#ifdef DEBUG
                char szBuf[256];
                DBGFR3DisasInstrEx(pVM->pUVM, pVCpu->idCpu, pCtx->cs.Sel, pCurInstrGC, DBGF_DISAS_FLAGS_DEFAULT_MODE,
                                   szBuf, sizeof(szBuf), NULL);
                Log(("NEW:   %s\n", szBuf));
#endif

                /* Copy the new instruction. */
                rc = PGMPhysSimpleReadGCPtr(VMMGetCpu0(pVM), pCurPatchInstrHC, pCurInstrGC, CpuNew.cbInstr);
                AssertRC(rc);

                /* Add a new lookup record for the duplicated instruction. */
                patmR3AddP2GLookupRecord(pVM, &pPatch->patch, pCurPatchInstrHC, pCurInstrGC, PATM_LOOKUP_BOTHDIR);
            }
            else
            {
#ifdef DEBUG
                char szBuf[256];
                DBGFR3DisasInstrEx(pVM->pUVM, pVCpu->idCpu, pCtx->cs.Sel, pCurInstrGC, DBGF_DISAS_FLAGS_DEFAULT_MODE,
                                   szBuf, sizeof(szBuf), NULL);
                Log(("NEW:   %s (FAILED)\n", szBuf));
#endif
                /* Restore the old lookup record for the duplicated instruction. */
                patmR3AddP2GLookupRecord(pVM, &pPatch->patch, pCurPatchInstrHC, pCurInstrGC, PATM_LOOKUP_BOTHDIR);

                /** @todo in theory we need to restore the lookup records for the remaining dirty instructions too! */
                rc = VERR_PATCHING_REFUSED;
                break;
            }
            pCurInstrGC      += CpuNew.cbInstr;
            pCurPatchInstrHC += CpuNew.cbInstr;
            pCurPatchInstrGC += CpuNew.cbInstr;
            cbLeft           -= CpuNew.cbInstr;

            /* Check if we expanded a complex guest instruction into a patch stream (e.g. call) */
            if (!cbLeft)
            {
                /* If the next patch instruction doesn't correspond to the next guest instruction, then we have some extra room to fill. */
                if (RTAvlU32Get(&pPatch->patch.Patch2GuestAddrTree, pCurPatchInstrGC - pVM->patm.s.pPatchMemGC) == NULL)
                {
                    pRec = (PRECPATCHTOGUEST)RTAvlU32GetBestFit(&pPatch->patch.Patch2GuestAddrTree, pCurPatchInstrGC - pVM->patm.s.pPatchMemGC, true);
                    if (pRec)
                    {
                        unsigned cbFiller  = pRec->Core.Key + pVM->patm.s.pPatchMemGC - pCurPatchInstrGC;
                        uint8_t *pPatchFillHC = patmPatchGCPtr2PatchHCPtr(pVM, pCurPatchInstrGC);

                        Assert(!pRec->fDirty);

                        Log(("Room left in patched instruction stream (%d bytes)\n", cbFiller));
                        if (cbFiller >= SIZEOF_NEARJUMP32)
                        {
                            pPatchFillHC[0] = 0xE9;
                             *(uint32_t *)&pPatchFillHC[1] = cbFiller - SIZEOF_NEARJUMP32;
#ifdef DEBUG
                            char szBuf[256];
                            DBGFR3DisasInstrEx(pVM->pUVM, pVCpu->idCpu, pCtx->cs.Sel, pCurPatchInstrGC,
                                               DBGF_DISAS_FLAGS_DEFAULT_MODE, szBuf, sizeof(szBuf), NULL);
                            Log(("FILL:  %s\n", szBuf));
#endif
                        }
                        else
                        {
                            for (unsigned i = 0; i < cbFiller; i++)
                            {
                                pPatchFillHC[i] = 0x90; /* NOP */
#ifdef DEBUG
                                char szBuf[256];
                                DBGFR3DisasInstrEx(pVM->pUVM, pVCpu->idCpu, pCtx->cs.Sel, pCurPatchInstrGC + i,
                                                   DBGF_DISAS_FLAGS_DEFAULT_MODE, szBuf, sizeof(szBuf), NULL);
                                Log(("FILL:  %s\n", szBuf));
#endif
                            }
                        }
                    }
                }
            }
        }
    }
    else
        rc = VERR_PATCHING_REFUSED;

    if (RT_SUCCESS(rc))
    {
        STAM_COUNTER_INC(&pVM->patm.s.StatInstrDirtyGood);
    }
    else
    {
        STAM_COUNTER_INC(&pVM->patm.s.StatInstrDirtyBad);
        Assert(cbDirty);

        /* Mark the whole instruction stream with breakpoints. */
        if (cbDirty)
            memset(pPatchInstrHC, 0xCC, cbDirty);

        if (    pVM->patm.s.fOutOfMemory == false
            &&  (pPatch->patch.flags & (PATMFL_DUPLICATE_FUNCTION|PATMFL_IDTHANDLER|PATMFL_TRAPHANDLER)))
        {
            rc = patmR3RefreshPatch(pVM, pPatch);
            if (RT_FAILURE(rc))
            {
                LogRel(("PATM: Failed to refresh dirty patch at %RRv. Disabling it.\n", pPatch->patch.pPrivInstrGC));
            }
            /* Even if we succeed, we must go back to the original instruction as the patched one could be invalid. */
            rc = VERR_PATCHING_REFUSED;
        }
    }
    return rc;
}

/**
 * Handle trap inside patch code
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pCtx        Pointer to the guest CPU context.
 * @param   pEip        GC pointer of trapping instruction.
 * @param   ppNewEip    GC pointer to new instruction.
 */
VMMR3_INT_DECL(int) PATMR3HandleTrap(PVM pVM, PCPUMCTX pCtx, RTRCPTR pEip, RTGCPTR *ppNewEip)
{
    PPATMPATCHREC    pPatch = 0;
    void            *pvPatchCoreOffset;
    RTRCUINTPTR      offset;
    RTRCPTR          pNewEip;
    int              rc ;
    PRECPATCHTOGUEST pPatchToGuestRec = 0;
    PVMCPU           pVCpu = VMMGetCpu0(pVM);

    AssertReturn(!HMIsEnabled(pVM), VERR_PATM_HM_IPE);
    Assert(pVM->cCpus == 1);

    pNewEip   = 0;
    *ppNewEip = 0;

    STAM_PROFILE_ADV_START(&pVM->patm.s.StatHandleTrap, a);

    /* Find the patch record. */
    /* Note: there might not be a patch to guest translation record (global function) */
    offset = pEip - pVM->patm.s.pPatchMemGC;
    pvPatchCoreOffset = RTAvloU32GetBestFit(&pVM->patm.s.PatchLookupTreeHC->PatchTreeByPatchAddr, offset, false);
    if (pvPatchCoreOffset)
    {
        pPatch = PATM_PATCHREC_FROM_COREOFFSET(pvPatchCoreOffset);

        Assert(offset >= pPatch->patch.pPatchBlockOffset && offset < pPatch->patch.pPatchBlockOffset + pPatch->patch.cbPatchBlockSize);

        if (pPatch->patch.uState == PATCH_DIRTY)
        {
            Log(("PATMR3HandleTrap: trap in dirty patch at %RRv\n", pEip));
            if (pPatch->patch.flags & (PATMFL_DUPLICATE_FUNCTION|PATMFL_CODE_REFERENCED))
            {
                /* Function duplication patches set fPIF to 1 on entry */
                pVM->patm.s.pGCStateHC->fPIF = 1;
            }
        }
        else
        if (pPatch->patch.uState == PATCH_DISABLED)
        {
            Log(("PATMR3HandleTrap: trap in disabled patch at %RRv\n", pEip));
            if (pPatch->patch.flags & (PATMFL_DUPLICATE_FUNCTION|PATMFL_CODE_REFERENCED))
            {
                /* Function duplication patches set fPIF to 1 on entry */
                pVM->patm.s.pGCStateHC->fPIF = 1;
            }
        }
        else
        if (pPatch->patch.uState == PATCH_DISABLE_PENDING)
        {
            RTRCPTR pPrivInstrGC = pPatch->patch.pPrivInstrGC;

            Log(("PATMR3HandleTrap: disable operation is pending for patch at %RRv\n", pPatch->patch.pPrivInstrGC));
            rc = PATMR3DisablePatch(pVM, pPatch->patch.pPrivInstrGC);
            AssertReleaseMsg(rc != VWRN_PATCH_REMOVED, ("PATMR3DisablePatch removed patch at %RRv\n", pPrivInstrGC));
            AssertMsg(pPatch->patch.uState == PATCH_DISABLED || pPatch->patch.uState == PATCH_UNUSABLE, ("Unexpected failure to disable patch state=%d rc=%Rrc\n", pPatch->patch.uState, rc));
        }

        pPatchToGuestRec = (PRECPATCHTOGUEST)RTAvlU32GetBestFit(&pPatch->patch.Patch2GuestAddrTree, offset, false);
        AssertReleaseMsg(pPatchToGuestRec, ("PATMR3HandleTrap: Unable to find corresponding guest address for %RRv (offset %x)\n", pEip, offset));

        pNewEip = pPatchToGuestRec->pOrgInstrGC;
        pPatch->patch.cTraps++;
        PATM_STAT_FAULT_INC(&pPatch->patch);
    }
    else
        AssertReleaseMsg(pVM->patm.s.pGCStateHC->fPIF == 0, ("PATMR3HandleTrap: Unable to find translation record for %RRv (PIF=0)\n", pEip));

    /* Check if we were interrupted in PATM generated instruction code. */
    if (pVM->patm.s.pGCStateHC->fPIF == 0)
    {
        DISCPUSTATE Cpu;
        rc = CPUMR3DisasmInstrCPU(pVM, pVCpu, pCtx, pEip, &Cpu, "PIF Trap: ");
        AssertRC(rc);

        if (    rc == VINF_SUCCESS
            &&  (   Cpu.pCurInstr->uOpcode == OP_PUSHF
                 || Cpu.pCurInstr->uOpcode == OP_PUSH
                 || Cpu.pCurInstr->uOpcode == OP_CALL)
           )
        {
            uint64_t fFlags;

            STAM_COUNTER_INC(&pVM->patm.s.StatPushTrap);

            if (Cpu.pCurInstr->uOpcode == OP_PUSH)
            {
                rc = PGMShwGetPage(pVCpu, pCtx->esp, &fFlags, NULL);
                if (    rc == VINF_SUCCESS
                    &&  ((fFlags & (X86_PTE_P|X86_PTE_RW)) == (X86_PTE_P|X86_PTE_RW)) )
                {
                    /* The stack address is fine, so the push argument is a pointer -> emulate this instruction */

                    /* Reset the PATM stack. */
                    CTXSUFF(pVM->patm.s.pGCState)->Psp = PATM_STACK_SIZE;

                    pVM->patm.s.pGCStateHC->fPIF = 1;

                    Log(("Faulting push -> go back to the original instruction\n"));

                    /* continue at the original instruction */
                    *ppNewEip = pNewEip - SELMToFlat(pVM, DISSELREG_CS, CPUMCTX2CORE(pCtx), 0);
                    STAM_PROFILE_ADV_STOP(&pVM->patm.s.StatHandleTrap, a);
                    return VINF_SUCCESS;
                }
            }

            /* Typical pushf (most patches)/push (call patch) trap because of a monitored page. */
            rc = PGMShwMakePageWritable(pVCpu, pCtx->esp, 0 /*fFlags*/);
            AssertMsgRC(rc, ("PGMShwModifyPage -> rc=%Rrc\n", rc));
            if (rc == VINF_SUCCESS)
            {
                /* The guest page *must* be present. */
                rc = PGMGstGetPage(pVCpu, pCtx->esp, &fFlags, NULL);
                if (    rc == VINF_SUCCESS
                    &&  (fFlags & X86_PTE_P))
                {
                    STAM_PROFILE_ADV_STOP(&pVM->patm.s.StatHandleTrap, a);
                    return VINF_PATCH_CONTINUE;
                }
            }
        }
        else
        if (pPatch->patch.pPrivInstrGC == pNewEip)
        {
            /* Invalidated patch or first instruction overwritten.
             * We can ignore the fPIF state in this case.
             */
            /* Reset the PATM stack. */
            CTXSUFF(pVM->patm.s.pGCState)->Psp = PATM_STACK_SIZE;

            Log(("Call to invalidated patch -> go back to the original instruction\n"));

            pVM->patm.s.pGCStateHC->fPIF = 1;

            /* continue at the original instruction */
            *ppNewEip = pNewEip - SELMToFlat(pVM, DISSELREG_CS, CPUMCTX2CORE(pCtx), 0);
            STAM_PROFILE_ADV_STOP(&pVM->patm.s.StatHandleTrap, a);
            return VINF_SUCCESS;
        }

        char szBuf[256];
        DBGFR3DisasInstrEx(pVM->pUVM, pVCpu->idCpu, pCtx->cs.Sel, pEip, DBGF_DISAS_FLAGS_DEFAULT_MODE, szBuf, sizeof(szBuf), NULL);

        /* Very bad. We crashed in emitted code. Probably stack? */
        if (pPatch)
        {
            AssertLogRelMsg(pVM->patm.s.pGCStateHC->fPIF == 1,
                            ("Crash in patch code %RRv (%RRv) esp=%RX32\nPatch state=%x flags=%RX64 fDirty=%d\n%s\n",
                             pEip, pNewEip, CPUMGetGuestESP(pVCpu), pPatch->patch.uState, pPatch->patch.flags,
                             pPatchToGuestRec->fDirty, szBuf));
        }
        else
            AssertLogRelMsg(pVM->patm.s.pGCStateHC->fPIF == 1,
                            ("Crash in patch code %RRv (%RRv) esp=%RX32\n%s\n", pEip, pNewEip, CPUMGetGuestESP(pVCpu), szBuf));
        EMR3FatalError(pVCpu, VERR_PATM_IPE_TRAP_IN_PATCH_CODE);
    }

    /* From here on, we must have a valid patch to guest translation. */
    if (pvPatchCoreOffset == 0)
    {
        STAM_PROFILE_ADV_STOP(&pVM->patm.s.StatHandleTrap, a);
        AssertMsgFailed(("PATMR3HandleTrap: patch not found at address %RRv!!\n", pEip));
        return VERR_PATCH_NOT_FOUND;
    }

    /* Take care of dirty/changed instructions. */
    if (pPatchToGuestRec->fDirty)
    {
        Assert(pPatchToGuestRec->Core.Key == offset);
        Assert(pVM->patm.s.pGCStateHC->fPIF == 1);

        rc = patmR3HandleDirtyInstr(pVM, pCtx, pPatch, pPatchToGuestRec, pEip);
        if (RT_SUCCESS(rc))
        {
            /* Retry the current instruction. */
            pNewEip = pEip;
            rc = VINF_PATCH_CONTINUE;   /* Continue at current patch instruction. */
        }
        else
        {
            /* Reset the PATM stack. */
            CTXSUFF(pVM->patm.s.pGCState)->Psp = PATM_STACK_SIZE;

            rc = VINF_SUCCESS;  /* Continue at original instruction. */
        }

        *ppNewEip = pNewEip - SELMToFlat(pVM, DISSELREG_CS, CPUMCTX2CORE(pCtx), 0);
        STAM_PROFILE_ADV_STOP(&pVM->patm.s.StatHandleTrap, a);
        return rc;
    }

#ifdef VBOX_STRICT
    if (pPatch->patch.flags & PATMFL_DUPLICATE_FUNCTION)
    {
        DISCPUSTATE cpu;
        bool        disret;
        uint32_t    cbInstr;
        PATMP2GLOOKUPREC cacheRec;
        RT_ZERO(cacheRec);
        cacheRec.pPatch = &pPatch->patch;

        disret = patmR3DisInstr(pVM, &pPatch->patch, pNewEip, patmR3GCVirtToHCVirt(pVM, &cacheRec, pNewEip), PATMREAD_RAWCODE,
                                &cpu, &cbInstr);
        if (cacheRec.Lock.pvMap)
            PGMPhysReleasePageMappingLock(pVM, &cacheRec.Lock);

        if (disret && cpu.pCurInstr->uOpcode == OP_RETN)
        {
            RTRCPTR retaddr;
            PCPUMCTX pCtx2;

            pCtx2 = CPUMQueryGuestCtxPtr(pVCpu);

            rc = PGMPhysSimpleReadGCPtr(pVCpu,  &retaddr, pCtx2->esp, sizeof(retaddr));
            AssertRC(rc);

            Log(("Return failed at %RRv (%RRv)\n", pEip, pNewEip));
            Log(("Expected return address %RRv found address %RRv Psp=%x\n", pVM->patm.s.pGCStackHC[(pVM->patm.s.pGCStateHC->Psp+PATM_STACK_SIZE)/sizeof(RTRCPTR)], retaddr, pVM->patm.s.pGCStateHC->Psp));
        }
    }
#endif

    /* Return original address, correct by subtracting the CS base address. */
    *ppNewEip = pNewEip - SELMToFlat(pVM, DISSELREG_CS, CPUMCTX2CORE(pCtx), 0);

    /* Reset the PATM stack. */
    CTXSUFF(pVM->patm.s.pGCState)->Psp = PATM_STACK_SIZE;

    if (pVM->patm.s.pGCStateHC->GCPtrInhibitInterrupts == pNewEip)
    {
        /* Must be a faulting instruction after sti; currently only sysexit, hlt or iret */
        Log(("PATMR3HandleTrap %RRv -> inhibit irqs set!\n", pEip));
#ifdef VBOX_STRICT
        DISCPUSTATE cpu;
        bool        disret;
        uint32_t    cbInstr;
        PATMP2GLOOKUPREC cacheRec;
        RT_ZERO(cacheRec);
        cacheRec.pPatch = &pPatch->patch;

        disret = patmR3DisInstr(pVM, &pPatch->patch, pNewEip, patmR3GCVirtToHCVirt(pVM, &cacheRec, pNewEip), PATMREAD_ORGCODE,
                                &cpu, &cbInstr);
        if (cacheRec.Lock.pvMap)
            PGMPhysReleasePageMappingLock(pVM, &cacheRec.Lock);

        if (disret && (cpu.pCurInstr->uOpcode == OP_SYSEXIT || cpu.pCurInstr->uOpcode == OP_HLT || cpu.pCurInstr->uOpcode == OP_INT3))
        {
            disret = patmR3DisInstr(pVM, &pPatch->patch, pNewEip, patmR3GCVirtToHCVirt(pVM, &cacheRec, pNewEip), PATMREAD_RAWCODE,
                                    &cpu, &cbInstr);
            if (cacheRec.Lock.pvMap)
                PGMPhysReleasePageMappingLock(pVM, &cacheRec.Lock);

            Assert(cpu.pCurInstr->uOpcode == OP_SYSEXIT || cpu.pCurInstr->uOpcode == OP_HLT || cpu.pCurInstr->uOpcode == OP_IRET);
        }
#endif
        EMSetInhibitInterruptsPC(pVCpu, pNewEip);
        pVM->patm.s.pGCStateHC->GCPtrInhibitInterrupts = 0;
    }

    Log2(("pPatchBlockGC %RRv - pEip %RRv corresponding GC address %RRv\n", PATCHCODE_PTR_GC(&pPatch->patch), pEip, pNewEip));
    DBGFR3_DISAS_INSTR_LOG(pVCpu, pCtx->cs.Sel, pNewEip, "PATCHRET: ");
    if (pNewEip >= pPatch->patch.pPrivInstrGC && pNewEip < pPatch->patch.pPrivInstrGC + pPatch->patch.cbPatchJump)
    {
        /* We can't jump back to code that we've overwritten with a 5 byte jump! */
        Log(("Disabling patch at location %RRv due to trap too close to the privileged instruction \n", pPatch->patch.pPrivInstrGC));
        PATMR3DisablePatch(pVM, pPatch->patch.pPrivInstrGC);
        STAM_PROFILE_ADV_STOP(&pVM->patm.s.StatHandleTrap, a);
        return VERR_PATCH_DISABLED;
    }

#ifdef PATM_REMOVE_PATCH_ON_TOO_MANY_TRAPS
    /** @todo compare to nr of successful runs. add some aging algorithm and determine the best time to disable the patch */
    if (pPatch->patch.cTraps > MAX_PATCH_TRAPS)
    {
        Log(("Disabling patch at location %RRv due to too many traps inside patch code\n", pPatch->patch.pPrivInstrGC));
        //we are only wasting time, back out the patch
        PATMR3DisablePatch(pVM, pPatch->patch.pPrivInstrGC);
        pTrapRec->pNextPatchInstr = 0;
        STAM_PROFILE_ADV_STOP(&pVM->patm.s.StatHandleTrap, a);
        return VERR_PATCH_DISABLED;
    }
#endif

    STAM_PROFILE_ADV_STOP(&pVM->patm.s.StatHandleTrap, a);
    return VINF_SUCCESS;
}


/**
 * Handle page-fault in monitored page
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3_INT_DECL(int) PATMR3HandleMonitoredPage(PVM pVM)
{
    AssertReturn(!HMIsEnabled(pVM), VERR_PATM_HM_IPE);
    PVMCPU pVCpu = VMMGetCpu0(pVM);

    RTRCPTR addr = pVM->patm.s.pvFaultMonitor;
    addr &= PAGE_BASE_GC_MASK;

    int rc = PGMHandlerVirtualDeregister(pVM, pVCpu, addr, false /*fHypervisor*/);
    AssertRC(rc); NOREF(rc);

    PPATMPATCHREC pPatchRec = (PPATMPATCHREC)RTAvloU32GetBestFit(&pVM->patm.s.PatchLookupTreeHC->PatchTree, addr, false);
    if (pPatchRec && pPatchRec->patch.uState == PATCH_ENABLED && PAGE_ADDRESS(pPatchRec->patch.pPrivInstrGC) == PAGE_ADDRESS(addr))
    {
        STAM_COUNTER_INC(&pVM->patm.s.StatMonitored);
        Log(("Renewing patch at %RRv\n", pPatchRec->patch.pPrivInstrGC));
        rc = PATMR3DisablePatch(pVM, pPatchRec->patch.pPrivInstrGC);
        if (rc == VWRN_PATCH_REMOVED)
            return VINF_SUCCESS;

        PATMR3EnablePatch(pVM, pPatchRec->patch.pPrivInstrGC);

        if (addr == pPatchRec->patch.pPrivInstrGC)
            addr++;
    }

    for(;;)
    {
        pPatchRec = (PPATMPATCHREC)RTAvloU32GetBestFit(&pVM->patm.s.PatchLookupTreeHC->PatchTree, addr, true);

        if (!pPatchRec || PAGE_ADDRESS(pPatchRec->patch.pPrivInstrGC) != PAGE_ADDRESS(addr))
            break;

        if (pPatchRec && pPatchRec->patch.uState == PATCH_ENABLED)
        {
            STAM_COUNTER_INC(&pVM->patm.s.StatMonitored);
            Log(("Renewing patch at %RRv\n", pPatchRec->patch.pPrivInstrGC));
            PATMR3DisablePatch(pVM, pPatchRec->patch.pPrivInstrGC);
            PATMR3EnablePatch(pVM, pPatchRec->patch.pPrivInstrGC);
        }
        addr = pPatchRec->patch.pPrivInstrGC + 1;
    }

    pVM->patm.s.pvFaultMonitor = 0;
    return VINF_SUCCESS;
}


#ifdef VBOX_WITH_STATISTICS

static const char *PATMPatchType(PVM pVM, PPATCHINFO pPatch)
{
    if (pPatch->flags & PATMFL_SYSENTER)
    {
        return "SYSENT";
    }
    else
    if (pPatch->flags & (PATMFL_TRAPHANDLER|PATMFL_INTHANDLER))
    {
        static char szTrap[16];
        uint32_t iGate;

        iGate = TRPMR3QueryGateByHandler(pVM, PATCHCODE_PTR_GC(pPatch));
        if (iGate < 256)
            RTStrPrintf(szTrap, sizeof(szTrap), (pPatch->flags & PATMFL_INTHANDLER) ? "INT-%2X" : "TRAP-%2X", iGate);
        else
            RTStrPrintf(szTrap, sizeof(szTrap), (pPatch->flags & PATMFL_INTHANDLER) ? "INT-??" : "TRAP-??");
        return szTrap;
    }
    else
    if (pPatch->flags & (PATMFL_DUPLICATE_FUNCTION))
        return "DUPFUNC";
    else
    if (pPatch->flags & PATMFL_REPLACE_FUNCTION_CALL)
        return "FUNCCALL";
    else
    if (pPatch->flags & PATMFL_TRAMPOLINE)
        return "TRAMP";
    else
        return patmGetInstructionString(pPatch->opcode, pPatch->flags);
}

static const char *PATMPatchState(PVM pVM, PPATCHINFO pPatch)
{
    NOREF(pVM);
    switch(pPatch->uState)
    {
    case PATCH_ENABLED:
        return "ENA";
    case PATCH_DISABLED:
        return "DIS";
    case PATCH_DIRTY:
        return "DIR";
    case PATCH_UNUSABLE:
        return "UNU";
    case PATCH_REFUSED:
        return "REF";
    case PATCH_DISABLE_PENDING:
        return "DIP";
    default:
        AssertFailed();
        return "   ";
    }
}

/**
 * Resets the sample.
 * @param   pVM         The cross context VM structure.
 * @param   pvSample    The sample registered using STAMR3RegisterCallback.
 */
static void patmResetStat(PVM pVM, void *pvSample)
{
    PPATCHINFO pPatch = (PPATCHINFO)pvSample;
    Assert(pPatch);

    pVM->patm.s.pStatsHC[pPatch->uPatchIdx].u32A = 0;
    pVM->patm.s.pStatsHC[pPatch->uPatchIdx].u32B = 0;
}

/**
 * Prints the sample into the buffer.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pvSample    The sample registered using STAMR3RegisterCallback.
 * @param   pszBuf      The buffer to print into.
 * @param   cchBuf      The size of the buffer.
 */
static void patmPrintStat(PVM pVM, void *pvSample, char *pszBuf, size_t cchBuf)
{
    PPATCHINFO pPatch = (PPATCHINFO)pvSample;
    Assert(pPatch);

    Assert(pPatch->uState != PATCH_REFUSED);
    Assert(!(pPatch->flags & (PATMFL_REPLACE_FUNCTION_CALL|PATMFL_MMIO_ACCESS)));

    RTStrPrintf(pszBuf, cchBuf, "size %04x ->%3s %8s - %08d - %08d",
                pPatch->cbPatchBlockSize, PATMPatchState(pVM, pPatch), PATMPatchType(pVM, pPatch),
                pVM->patm.s.pStatsHC[pPatch->uPatchIdx].u32A, pVM->patm.s.pStatsHC[pPatch->uPatchIdx].u32B);
}

/**
 * Returns the GC address of the corresponding patch statistics counter
 *
 * @returns Stat address
 * @param   pVM         The cross context VM structure.
 * @param   pPatch      Patch structure
 */
RTRCPTR patmPatchQueryStatAddress(PVM pVM, PPATCHINFO pPatch)
{
    Assert(pPatch->uPatchIdx != PATM_STAT_INDEX_NONE);
    return pVM->patm.s.pStatsGC + sizeof(STAMRATIOU32) * pPatch->uPatchIdx + RT_OFFSETOF(STAMRATIOU32, u32A);
}

#endif /* VBOX_WITH_STATISTICS */
#ifdef VBOX_WITH_DEBUGGER

/**
 * @callback_method_impl{FNDBGCCMD, The '.patmoff' command.}
 */
static DECLCALLBACK(int) patmr3CmdOff(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PUVM pUVM, PCDBGCVAR paArgs, unsigned cArgs)
{
    /*
     * Validate input.
     */
    NOREF(cArgs); NOREF(paArgs);
    DBGC_CMDHLP_REQ_UVM_RET(pCmdHlp, pCmd, pUVM);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);

    if (HMIsEnabled(pVM))
        return DBGCCmdHlpPrintf(pCmdHlp, "PATM is permanently disabled by HM.\n");

    RTAvloU32DoWithAll(&pVM->patm.s.PatchLookupTreeHC->PatchTree, true, DisableAllPatches, pVM);
    PATMR3AllowPatching(pVM->pUVM, false);
    return pCmdHlp->pfnPrintf(pCmdHlp, NULL, "Patching disabled\n");
}

/**
 * @callback_method_impl{FNDBGCCMD, The '.patmon' command.}
 */
static DECLCALLBACK(int) patmr3CmdOn(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PUVM pUVM, PCDBGCVAR paArgs, unsigned cArgs)
{
    /*
     * Validate input.
     */
    NOREF(cArgs); NOREF(paArgs);
    DBGC_CMDHLP_REQ_UVM_RET(pCmdHlp, pCmd, pUVM);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);

    if (HMIsEnabled(pVM))
        return DBGCCmdHlpPrintf(pCmdHlp, "PATM is permanently disabled by HM.\n");

    PATMR3AllowPatching(pVM->pUVM, true);
    RTAvloU32DoWithAll(&pVM->patm.s.PatchLookupTreeHC->PatchTree, true, EnableAllPatches, pVM);
    return pCmdHlp->pfnPrintf(pCmdHlp, NULL, "Patching enabled\n");
}

#endif /* VBOX_WITH_DEBUGGER */

