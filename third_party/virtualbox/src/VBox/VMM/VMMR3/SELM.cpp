/* $Id: SELM.cpp $ */
/** @file
 * SELM - The Selector Manager.
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

/** @page pg_selm   SELM - The Selector Manager
 *
 * SELM takes care of GDT, LDT and TSS shadowing in raw-mode, and the injection
 * of a few hyper selector for the raw-mode context.  In the hardware assisted
 * virtualization mode its only task is to decode entries in the guest GDT or
 * LDT once in a while.
 *
 * @see grp_selm
 *
 *
 * @section seg_selm_shadowing   Shadowing
 *
 * SELMR3UpdateFromCPUM() and SELMR3SyncTSS() does the bulk synchronization
 * work.  The three structures (GDT, LDT, TSS) are all shadowed wholesale atm.
 * The idea is to do it in a more on-demand fashion when we get time.  There
 * also a whole bunch of issues with the current synchronization of all three
 * tables, see notes and todos in the code.
 *
 * When the guest makes changes to the GDT we will try update the shadow copy
 * without involving SELMR3UpdateFromCPUM(), see selmGCSyncGDTEntry().
 *
 * When the guest make LDT changes we'll trigger a full resync of the LDT
 * (SELMR3UpdateFromCPUM()), which, needless to say, isn't optimal.
 *
 * The TSS shadowing is limited to the fields we need to care about, namely SS0
 * and ESP0.  The Patch Manager makes use of these.  We monitor updates to the
 * guest TSS and will try keep our SS0 and ESP0 copies up to date this way
 * rather than go the SELMR3SyncTSS() route.
 *
 * When in raw-mode SELM also injects a few extra GDT selectors which are used
 * by the raw-mode (hyper) context.  These start their life at the high end of
 * the table and will be relocated when the guest tries to make use of them...
 * Well, that was that idea at least, only the code isn't quite there yet which
 * is why we have trouble with guests which actually have a full sized GDT.
 *
 * So, the summary of the current GDT, LDT and TSS shadowing is that there is a
 * lot of relatively simple and enjoyable work to be done, see @bugref{3267}.
 *
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_SELM
#include <VBox/vmm/selm.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/ssm.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/trpm.h>
#include <VBox/vmm/dbgf.h>
#include "SELMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/err.h>
#include <VBox/param.h>

#include <iprt/assert.h>
#include <VBox/log.h>
#include <iprt/asm.h>
#include <iprt/string.h>
#include <iprt/thread.h>
#include <iprt/string.h>

#include "SELMInline.h"


/** SELM saved state version. */
#define SELM_SAVED_STATE_VERSION    5


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static DECLCALLBACK(int)  selmR3Save(PVM pVM, PSSMHANDLE pSSM);
static DECLCALLBACK(int)  selmR3Load(PVM pVM, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass);
static DECLCALLBACK(int)  selmR3LoadDone(PVM pVM, PSSMHANDLE pSSM);
static DECLCALLBACK(void) selmR3InfoGdt(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs);
static DECLCALLBACK(void) selmR3InfoGdtGuest(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs);
static DECLCALLBACK(void) selmR3InfoLdt(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs);
static DECLCALLBACK(void) selmR3InfoLdtGuest(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs);
//static DECLCALLBACK(void) selmR3InfoTss(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs);
//static DECLCALLBACK(void) selmR3InfoTssGuest(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
#if defined(VBOX_WITH_RAW_MODE) && defined(LOG_ENABLED)
/** Segment register names. */
static char const g_aszSRegNms[X86_SREG_COUNT][4] = { "ES", "CS", "SS", "DS", "FS", "GS" };
#endif


/**
 * Initializes the SELM.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3DECL(int) SELMR3Init(PVM pVM)
{
    int rc;
    LogFlow(("SELMR3Init\n"));

    /*
     * Assert alignment and sizes.
     * (The TSS block requires contiguous back.)
     */
    AssertCompile(sizeof(pVM->selm.s) <= sizeof(pVM->selm.padding));    AssertRelease(sizeof(pVM->selm.s) <= sizeof(pVM->selm.padding));
    AssertCompileMemberAlignment(VM, selm.s, 32);                       AssertRelease(!(RT_OFFSETOF(VM, selm.s) & 31));
#if 0 /* doesn't work */
    AssertCompile((RT_OFFSETOF(VM, selm.s.Tss)       & PAGE_OFFSET_MASK) <= PAGE_SIZE - sizeof(pVM->selm.s.Tss));
    AssertCompile((RT_OFFSETOF(VM, selm.s.TssTrap08) & PAGE_OFFSET_MASK) <= PAGE_SIZE - sizeof(pVM->selm.s.TssTrap08));
#endif
    AssertRelease((RT_OFFSETOF(VM, selm.s.Tss)       & PAGE_OFFSET_MASK) <= PAGE_SIZE - sizeof(pVM->selm.s.Tss));
    AssertRelease((RT_OFFSETOF(VM, selm.s.TssTrap08) & PAGE_OFFSET_MASK) <= PAGE_SIZE - sizeof(pVM->selm.s.TssTrap08));
    AssertRelease(sizeof(pVM->selm.s.Tss.IntRedirBitmap) == 0x20);

    /*
     * Init the structure.
     */
    pVM->selm.s.offVM                                = RT_OFFSETOF(VM, selm);
    pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS]         = (SELM_GDT_ELEMENTS - 0x1) << 3;
    pVM->selm.s.aHyperSel[SELM_HYPER_SEL_DS]         = (SELM_GDT_ELEMENTS - 0x2) << 3;
    pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS64]       = (SELM_GDT_ELEMENTS - 0x3) << 3;
    pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS]        = (SELM_GDT_ELEMENTS - 0x4) << 3;
    pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS_TRAP08] = (SELM_GDT_ELEMENTS - 0x5) << 3;

    if (HMIsRawModeCtxNeeded(pVM))
    {
        /*
         * Allocate GDT table.
         */
        rc = MMR3HyperAllocOnceNoRel(pVM, sizeof(pVM->selm.s.paGdtR3[0]) * SELM_GDT_ELEMENTS,
                                     PAGE_SIZE, MM_TAG_SELM, (void **)&pVM->selm.s.paGdtR3);
        AssertRCReturn(rc, rc);

        /*
         * Allocate LDT area.
         */
        rc = MMR3HyperAllocOnceNoRel(pVM, _64K + PAGE_SIZE, PAGE_SIZE, MM_TAG_SELM, &pVM->selm.s.pvLdtR3);
        AssertRCReturn(rc, rc);
    }

    /*
     * Init Guest's and Shadow GDT, LDT, TSS changes control variables.
     */
    pVM->selm.s.cbEffGuestGdtLimit = 0;
    pVM->selm.s.GuestGdtr.pGdt     = RTRCPTR_MAX;
    pVM->selm.s.GCPtrGuestLdt      = RTRCPTR_MAX;
    pVM->selm.s.GCPtrGuestTss      = RTRCPTR_MAX;

    pVM->selm.s.paGdtRC            = NIL_RTRCPTR; /* Must be set in SELMR3Relocate because of monitoring. */
    pVM->selm.s.pvLdtRC            = RTRCPTR_MAX;
    pVM->selm.s.pvMonShwTssRC      = RTRCPTR_MAX;
    pVM->selm.s.GCSelTss           = RTSEL_MAX;

    pVM->selm.s.fSyncTSSRing0Stack = false;

    /* The I/O bitmap starts right after the virtual interrupt redirection
       bitmap. Outside the TSS on purpose; the CPU will not check it for
       I/O operations. */
    pVM->selm.s.Tss.offIoBitmap = sizeof(VBOXTSS);
    /* bit set to 1 means no redirection */
    memset(pVM->selm.s.Tss.IntRedirBitmap, 0xff, sizeof(pVM->selm.s.Tss.IntRedirBitmap));

    /*
     * Register the virtual access handlers.
     */
    pVM->selm.s.hShadowGdtWriteHandlerType = NIL_PGMVIRTHANDLERTYPE;
    pVM->selm.s.hShadowLdtWriteHandlerType = NIL_PGMVIRTHANDLERTYPE;
    pVM->selm.s.hShadowTssWriteHandlerType = NIL_PGMVIRTHANDLERTYPE;
    pVM->selm.s.hGuestGdtWriteHandlerType  = NIL_PGMVIRTHANDLERTYPE;
    pVM->selm.s.hGuestLdtWriteHandlerType  = NIL_PGMVIRTHANDLERTYPE;
    pVM->selm.s.hGuestTssWriteHandlerType  = NIL_PGMVIRTHANDLERTYPE;
#ifdef VBOX_WITH_RAW_MODE
    if (!HMIsEnabled(pVM))
    {
# ifdef SELM_TRACK_SHADOW_GDT_CHANGES
        rc = PGMR3HandlerVirtualTypeRegister(pVM, PGMVIRTHANDLERKIND_HYPERVISOR, false /*fRelocUserRC*/,
                                             NULL /*pfnInvalidateR3*/, NULL /*pfnHandlerR3*/,
                                             NULL /*pszHandlerRC*/, "selmRCShadowGDTWritePfHandler",
                                             "Shadow GDT write access handler", &pVM->selm.s.hShadowGdtWriteHandlerType);
        AssertRCReturn(rc, rc);
# endif
# ifdef SELM_TRACK_SHADOW_TSS_CHANGES
        rc = PGMR3HandlerVirtualTypeRegister(pVM, PGMVIRTHANDLERKIND_HYPERVISOR, false /*fRelocUserRC*/,
                                             NULL /*pfnInvalidateR3*/, NULL /*pfnHandlerR3*/,
                                             NULL /*pszHandlerRC*/, "selmRCShadowTSSWritePfHandler",
                                             "Shadow TSS write access handler", &pVM->selm.s.hShadowTssWriteHandlerType);
        AssertRCReturn(rc, rc);
# endif
# ifdef SELM_TRACK_SHADOW_LDT_CHANGES
        rc = PGMR3HandlerVirtualTypeRegister(pVM, PGMVIRTHANDLERKIND_HYPERVISOR, false /*fRelocUserRC*/,
                                             NULL /*pfnInvalidateR3*/, NULL /*pfnHandlerR3*/,
                                             NULL /*pszHandlerRC*/, "selmRCShadowLDTWritePfHandler",
                                             "Shadow LDT write access handler", &pVM->selm.s.hShadowLdtWriteHandlerType);
        AssertRCReturn(rc, rc);
# endif
        rc = PGMR3HandlerVirtualTypeRegister(pVM, PGMVIRTHANDLERKIND_WRITE, false /*fRelocUserRC*/,
                                             NULL /*pfnInvalidateR3*/, selmGuestGDTWriteHandler,
                                             "selmGuestGDTWriteHandler", "selmRCGuestGDTWritePfHandler",
                                             "Guest GDT write access handler", &pVM->selm.s.hGuestGdtWriteHandlerType);
        AssertRCReturn(rc, rc);
        rc = PGMR3HandlerVirtualTypeRegister(pVM, PGMVIRTHANDLERKIND_WRITE, false /*fRelocUserRC*/,
                                             NULL /*pfnInvalidateR3*/, selmGuestLDTWriteHandler,
                                             "selmGuestLDTWriteHandler", "selmRCGuestLDTWritePfHandler",
                                             "Guest LDT write access handler", &pVM->selm.s.hGuestLdtWriteHandlerType);
        AssertRCReturn(rc, rc);
        rc = PGMR3HandlerVirtualTypeRegister(pVM, PGMVIRTHANDLERKIND_WRITE, false /*fRelocUserRC*/,
                                             NULL /*pfnInvalidateR3*/, selmGuestTSSWriteHandler,
                                             "selmGuestTSSWriteHandler", "selmRCGuestTSSWritePfHandler",
                                             "Guest TSS write access handler", &pVM->selm.s.hGuestTssWriteHandlerType);
        AssertRCReturn(rc, rc);
    }
#endif /* VBOX_WITH_RAW_MODE */

    /*
     * Register the saved state data unit.
     */
    rc = SSMR3RegisterInternal(pVM, "selm", 1, SELM_SAVED_STATE_VERSION, sizeof(SELM),
                               NULL, NULL, NULL,
                               NULL, selmR3Save, NULL,
                               NULL, selmR3Load, selmR3LoadDone);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Statistics.
     */
    if (!HMIsEnabled(pVM))
    {
        STAM_REG(pVM, &pVM->selm.s.StatRCWriteGuestGDTHandled,     STAMTYPE_COUNTER, "/SELM/GC/Write/Guest/GDTInt",  STAMUNIT_OCCURENCES,     "The number of handled writes to the Guest GDT.");
        STAM_REG(pVM, &pVM->selm.s.StatRCWriteGuestGDTUnhandled,   STAMTYPE_COUNTER, "/SELM/GC/Write/Guest/GDTEmu",  STAMUNIT_OCCURENCES,     "The number of unhandled writes to the Guest GDT.");
        STAM_REG(pVM, &pVM->selm.s.StatRCWriteGuestLDT,            STAMTYPE_COUNTER, "/SELM/GC/Write/Guest/LDT",     STAMUNIT_OCCURENCES,     "The number of writes to the Guest LDT was detected.");
        STAM_REG(pVM, &pVM->selm.s.StatRCWriteGuestTSSHandled,     STAMTYPE_COUNTER, "/SELM/GC/Write/Guest/TSSInt",  STAMUNIT_OCCURENCES,     "The number of handled writes to the Guest TSS.");
        STAM_REG(pVM, &pVM->selm.s.StatRCWriteGuestTSSRedir,       STAMTYPE_COUNTER, "/SELM/GC/Write/Guest/TSSRedir",STAMUNIT_OCCURENCES,     "The number of handled redir bitmap writes to the Guest TSS.");
        STAM_REG(pVM, &pVM->selm.s.StatRCWriteGuestTSSHandledChanged,STAMTYPE_COUNTER, "/SELM/GC/Write/Guest/TSSIntChg", STAMUNIT_OCCURENCES, "The number of handled writes to the Guest TSS where the R0 stack changed.");
        STAM_REG(pVM, &pVM->selm.s.StatRCWriteGuestTSSUnhandled,   STAMTYPE_COUNTER, "/SELM/GC/Write/Guest/TSSEmu",  STAMUNIT_OCCURENCES,     "The number of unhandled writes to the Guest TSS.");
        STAM_REG(pVM, &pVM->selm.s.StatTSSSync,                    STAMTYPE_PROFILE, "/PROF/SELM/TSSSync",           STAMUNIT_TICKS_PER_CALL, "Profiling of the SELMR3SyncTSS() body.");
        STAM_REG(pVM, &pVM->selm.s.StatUpdateFromCPUM,             STAMTYPE_PROFILE, "/PROF/SELM/UpdateFromCPUM",    STAMUNIT_TICKS_PER_CALL, "Profiling of the SELMR3UpdateFromCPUM() body.");

        STAM_REL_REG(pVM, &pVM->selm.s.StatHyperSelsChanged,       STAMTYPE_COUNTER, "/SELM/HyperSels/Changed",      STAMUNIT_OCCURENCES,     "The number of times we had to relocate our hypervisor selectors.");
        STAM_REL_REG(pVM, &pVM->selm.s.StatScanForHyperSels,       STAMTYPE_COUNTER, "/SELM/HyperSels/Scan",         STAMUNIT_OCCURENCES,     "The number of times we had find free hypervisor selectors.");

        STAM_REL_REG(pVM, &pVM->selm.s.aStatDetectedStaleSReg[X86_SREG_ES], STAMTYPE_COUNTER, "/SELM/UpdateFromCPUM/DetectedStaleES", STAMUNIT_OCCURENCES, "Stale ES was detected in UpdateFromCPUM.");
        STAM_REL_REG(pVM, &pVM->selm.s.aStatDetectedStaleSReg[X86_SREG_CS], STAMTYPE_COUNTER, "/SELM/UpdateFromCPUM/DetectedStaleCS", STAMUNIT_OCCURENCES, "Stale CS was detected in UpdateFromCPUM.");
        STAM_REL_REG(pVM, &pVM->selm.s.aStatDetectedStaleSReg[X86_SREG_SS], STAMTYPE_COUNTER, "/SELM/UpdateFromCPUM/DetectedStaleSS", STAMUNIT_OCCURENCES, "Stale SS was detected in UpdateFromCPUM.");
        STAM_REL_REG(pVM, &pVM->selm.s.aStatDetectedStaleSReg[X86_SREG_DS], STAMTYPE_COUNTER, "/SELM/UpdateFromCPUM/DetectedStaleDS", STAMUNIT_OCCURENCES, "Stale DS was detected in UpdateFromCPUM.");
        STAM_REL_REG(pVM, &pVM->selm.s.aStatDetectedStaleSReg[X86_SREG_FS], STAMTYPE_COUNTER, "/SELM/UpdateFromCPUM/DetectedStaleFS", STAMUNIT_OCCURENCES, "Stale FS was detected in UpdateFromCPUM.");
        STAM_REL_REG(pVM, &pVM->selm.s.aStatDetectedStaleSReg[X86_SREG_GS], STAMTYPE_COUNTER, "/SELM/UpdateFromCPUM/DetectedStaleGS", STAMUNIT_OCCURENCES, "Stale GS was detected in UpdateFromCPUM.");

        STAM_REL_REG(pVM, &pVM->selm.s.aStatAlreadyStaleSReg[X86_SREG_ES],  STAMTYPE_COUNTER, "/SELM/UpdateFromCPUM/AlreadyStaleES", STAMUNIT_OCCURENCES, "Already stale ES in UpdateFromCPUM.");
        STAM_REL_REG(pVM, &pVM->selm.s.aStatAlreadyStaleSReg[X86_SREG_CS],  STAMTYPE_COUNTER, "/SELM/UpdateFromCPUM/AlreadyStaleCS", STAMUNIT_OCCURENCES, "Already stale CS in UpdateFromCPUM.");
        STAM_REL_REG(pVM, &pVM->selm.s.aStatAlreadyStaleSReg[X86_SREG_SS],  STAMTYPE_COUNTER, "/SELM/UpdateFromCPUM/AlreadyStaleSS", STAMUNIT_OCCURENCES, "Already stale SS in UpdateFromCPUM.");
        STAM_REL_REG(pVM, &pVM->selm.s.aStatAlreadyStaleSReg[X86_SREG_DS],  STAMTYPE_COUNTER, "/SELM/UpdateFromCPUM/AlreadyStaleDS", STAMUNIT_OCCURENCES, "Already stale DS in UpdateFromCPUM.");
        STAM_REL_REG(pVM, &pVM->selm.s.aStatAlreadyStaleSReg[X86_SREG_FS],  STAMTYPE_COUNTER, "/SELM/UpdateFromCPUM/AlreadyStaleFS", STAMUNIT_OCCURENCES, "Already stale FS in UpdateFromCPUM.");
        STAM_REL_REG(pVM, &pVM->selm.s.aStatAlreadyStaleSReg[X86_SREG_GS],  STAMTYPE_COUNTER, "/SELM/UpdateFromCPUM/AlreadyStaleGS", STAMUNIT_OCCURENCES, "Already stale GS in UpdateFromCPUM.");

        STAM_REL_REG(pVM, &pVM->selm.s.StatStaleToUnstaleSReg,              STAMTYPE_COUNTER, "/SELM/UpdateFromCPUM/StaleToUnstale", STAMUNIT_OCCURENCES, "Transitions from stale to unstale UpdateFromCPUM.");

        STAM_REG(    pVM, &pVM->selm.s.aStatUpdatedSReg[X86_SREG_ES],  STAMTYPE_COUNTER, "/SELM/UpdateFromCPUM/UpdatedES", STAMUNIT_OCCURENCES, "Updated hidden ES values in UpdateFromCPUM.");
        STAM_REG(    pVM, &pVM->selm.s.aStatUpdatedSReg[X86_SREG_CS],  STAMTYPE_COUNTER, "/SELM/UpdateFromCPUM/UpdatedCS", STAMUNIT_OCCURENCES, "Updated hidden CS values in UpdateFromCPUM.");
        STAM_REG(    pVM, &pVM->selm.s.aStatUpdatedSReg[X86_SREG_SS],  STAMTYPE_COUNTER, "/SELM/UpdateFromCPUM/UpdatedSS", STAMUNIT_OCCURENCES, "Updated hidden SS values in UpdateFromCPUM.");
        STAM_REG(    pVM, &pVM->selm.s.aStatUpdatedSReg[X86_SREG_DS],  STAMTYPE_COUNTER, "/SELM/UpdateFromCPUM/UpdatedDS", STAMUNIT_OCCURENCES, "Updated hidden DS values in UpdateFromCPUM.");
        STAM_REG(    pVM, &pVM->selm.s.aStatUpdatedSReg[X86_SREG_FS],  STAMTYPE_COUNTER, "/SELM/UpdateFromCPUM/UpdatedFS", STAMUNIT_OCCURENCES, "Updated hidden FS values in UpdateFromCPUM.");
        STAM_REG(    pVM, &pVM->selm.s.aStatUpdatedSReg[X86_SREG_GS],  STAMTYPE_COUNTER, "/SELM/UpdateFromCPUM/UpdatedGS", STAMUNIT_OCCURENCES, "Updated hidden GS values in UpdateFromCPUM.");
    }

    STAM_REG(    pVM, &pVM->selm.s.StatLoadHidSelGst,              STAMTYPE_COUNTER, "/SELM/LoadHidSel/LoadedGuest",   STAMUNIT_OCCURENCES, "SELMLoadHiddenSelectorReg: Loaded from guest tables.");
    STAM_REG(    pVM, &pVM->selm.s.StatLoadHidSelShw,              STAMTYPE_COUNTER, "/SELM/LoadHidSel/LoadedShadow",  STAMUNIT_OCCURENCES, "SELMLoadHiddenSelectorReg: Loaded from shadow tables.");
    STAM_REL_REG(pVM, &pVM->selm.s.StatLoadHidSelReadErrors,       STAMTYPE_COUNTER, "/SELM/LoadHidSel/GstReadErrors", STAMUNIT_OCCURENCES, "SELMLoadHiddenSelectorReg: Guest table read errors.");
    STAM_REL_REG(pVM, &pVM->selm.s.StatLoadHidSelGstNoGood,        STAMTYPE_COUNTER, "/SELM/LoadHidSel/NoGoodGuest",   STAMUNIT_OCCURENCES, "SELMLoadHiddenSelectorReg: No good guest table entry.");

#ifdef VBOX_WITH_RAW_MODE
    /*
     * Default action when entering raw mode for the first time
     */
    if (!HMIsEnabled(pVM))
    {
        PVMCPU pVCpu = &pVM->aCpus[0];  /* raw mode implies on VCPU */
        VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_TSS);
        VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_GDT);
        VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_LDT);
    }
#endif

    /*
     * Register info handlers.
     */
    if (HMIsRawModeCtxNeeded(pVM))
    {
        DBGFR3InfoRegisterInternal(pVM, "gdt",      "Displays the shadow GDT. No arguments.",   &selmR3InfoGdt);
        DBGFR3InfoRegisterInternal(pVM, "ldt",      "Displays the shadow LDT. No arguments.",   &selmR3InfoLdt);
        //DBGFR3InfoRegisterInternal(pVM, "tss",      "Displays the shadow TSS. No arguments.",   &selmR3InfoTss);
    }
    DBGFR3InfoRegisterInternal(pVM, "gdtguest", "Displays the guest GDT. No arguments.",    &selmR3InfoGdtGuest);
    DBGFR3InfoRegisterInternal(pVM, "ldtguest", "Displays the guest LDT. No arguments.",    &selmR3InfoLdtGuest);
    //DBGFR3InfoRegisterInternal(pVM, "tssguest", "Displays the guest TSS. No arguments.",    &selmR3InfoTssGuest);

    return rc;
}


/**
 * Finalizes HMA page attributes.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 */
VMMR3DECL(int) SELMR3InitFinalize(PVM pVM)
{
#ifdef VBOX_WITH_RAW_MODE
    /** @cfgm{/DoubleFault,bool,false}
     * Enables catching of double faults in the raw-mode context VMM code.  This can
     * be used when the triple faults or hangs occur and one suspect an unhandled
     * double fault.  This is not enabled by default because it means making the
     * hyper selectors writeable for all supervisor code, including the guest's.
     * The double fault is a task switch and thus requires write access to the GDT
     * of the TSS (to set it busy), to the old TSS (to store state), and to the Trap
     * 8 TSS for the back link.
     */
    bool f;
# if defined(DEBUG_bird)
    int rc = CFGMR3QueryBoolDef(CFGMR3GetRoot(pVM), "DoubleFault", &f, true);
# else
    int rc = CFGMR3QueryBoolDef(CFGMR3GetRoot(pVM), "DoubleFault", &f, false);
# endif
    AssertLogRelRCReturn(rc, rc);
    if (f && HMIsRawModeCtxNeeded(pVM))
    {
        PX86DESC paGdt = pVM->selm.s.paGdtR3;
        rc = PGMMapSetPage(pVM, MMHyperR3ToRC(pVM, &paGdt[pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS_TRAP08] >> 3]), sizeof(paGdt[0]),
                           X86_PTE_RW | X86_PTE_P | X86_PTE_A | X86_PTE_D);
        AssertRC(rc);
        rc = PGMMapSetPage(pVM, MMHyperR3ToRC(pVM, &paGdt[pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS] >> 3]), sizeof(paGdt[0]),
                           X86_PTE_RW | X86_PTE_P | X86_PTE_A | X86_PTE_D);
        AssertRC(rc);
        rc = PGMMapSetPage(pVM, VM_RC_ADDR(pVM, &pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS]), sizeof(pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS]),
                           X86_PTE_RW | X86_PTE_P | X86_PTE_A | X86_PTE_D);
        AssertRC(rc);
        rc = PGMMapSetPage(pVM, VM_RC_ADDR(pVM, &pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS_TRAP08]), sizeof(pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS_TRAP08]),
                           X86_PTE_RW | X86_PTE_P | X86_PTE_A | X86_PTE_D);
        AssertRC(rc);
    }
#else  /* !VBOX_WITH_RAW_MODE */
    RT_NOREF(pVM);
#endif /* !VBOX_WITH_RAW_MODE */
    return VINF_SUCCESS;
}


/**
 * Setup the hypervisor GDT selectors in our shadow table
 *
 * @param   pVM     The cross context VM structure.
 */
static void selmR3SetupHyperGDTSelectors(PVM pVM)
{
    PX86DESC paGdt = pVM->selm.s.paGdtR3;

    /*
     * Set up global code and data descriptors for use in the guest context.
     * Both are wide open (base 0, limit 4GB)
     */
    PX86DESC   pDesc = &paGdt[pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS] >> 3];
    pDesc->Gen.u16LimitLow      = 0xffff;
    pDesc->Gen.u4LimitHigh      = 0xf;
    pDesc->Gen.u16BaseLow       = 0;
    pDesc->Gen.u8BaseHigh1      = 0;
    pDesc->Gen.u8BaseHigh2      = 0;
    pDesc->Gen.u4Type           = X86_SEL_TYPE_ER_ACC;
    pDesc->Gen.u1DescType       = 1; /* not system, but code/data */
    pDesc->Gen.u2Dpl            = 0; /* supervisor */
    pDesc->Gen.u1Present        = 1;
    pDesc->Gen.u1Available      = 0;
    pDesc->Gen.u1Long           = 0;
    pDesc->Gen.u1DefBig         = 1; /* def 32 bit */
    pDesc->Gen.u1Granularity    = 1; /* 4KB limit */

    /* data */
    pDesc = &paGdt[pVM->selm.s.aHyperSel[SELM_HYPER_SEL_DS] >> 3];
    pDesc->Gen.u16LimitLow      = 0xffff;
    pDesc->Gen.u4LimitHigh      = 0xf;
    pDesc->Gen.u16BaseLow       = 0;
    pDesc->Gen.u8BaseHigh1      = 0;
    pDesc->Gen.u8BaseHigh2      = 0;
    pDesc->Gen.u4Type           = X86_SEL_TYPE_RW_ACC;
    pDesc->Gen.u1DescType       = 1; /* not system, but code/data */
    pDesc->Gen.u2Dpl            = 0; /* supervisor */
    pDesc->Gen.u1Present        = 1;
    pDesc->Gen.u1Available      = 0;
    pDesc->Gen.u1Long           = 0;
    pDesc->Gen.u1DefBig         = 1; /* big */
    pDesc->Gen.u1Granularity    = 1; /* 4KB limit */

    /* 64-bit mode code (& data?) */
    pDesc = &paGdt[pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS64] >> 3];
    pDesc->Gen.u16LimitLow      = 0xffff;
    pDesc->Gen.u4LimitHigh      = 0xf;
    pDesc->Gen.u16BaseLow       = 0;
    pDesc->Gen.u8BaseHigh1      = 0;
    pDesc->Gen.u8BaseHigh2      = 0;
    pDesc->Gen.u4Type           = X86_SEL_TYPE_ER_ACC;
    pDesc->Gen.u1DescType       = 1; /* not system, but code/data */
    pDesc->Gen.u2Dpl            = 0; /* supervisor */
    pDesc->Gen.u1Present        = 1;
    pDesc->Gen.u1Available      = 0;
    pDesc->Gen.u1Long           = 1; /* The Long (L) attribute bit. */
    pDesc->Gen.u1DefBig         = 0; /* With L=1 this must be 0. */
    pDesc->Gen.u1Granularity    = 1; /* 4KB limit */

    /*
     * TSS descriptor
     */
    pDesc = &paGdt[pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS] >> 3];
    RTRCPTR RCPtrTSS = VM_RC_ADDR(pVM, &pVM->selm.s.Tss);
    pDesc->Gen.u16BaseLow       = RT_LOWORD(RCPtrTSS);
    pDesc->Gen.u8BaseHigh1      = RT_BYTE3(RCPtrTSS);
    pDesc->Gen.u8BaseHigh2      = RT_BYTE4(RCPtrTSS);
    pDesc->Gen.u16LimitLow      = sizeof(VBOXTSS) - 1;
    pDesc->Gen.u4LimitHigh      = 0;
    pDesc->Gen.u4Type           = X86_SEL_TYPE_SYS_386_TSS_AVAIL;
    pDesc->Gen.u1DescType       = 0; /* system */
    pDesc->Gen.u2Dpl            = 0; /* supervisor */
    pDesc->Gen.u1Present        = 1;
    pDesc->Gen.u1Available      = 0;
    pDesc->Gen.u1Long           = 0;
    pDesc->Gen.u1DefBig         = 0;
    pDesc->Gen.u1Granularity    = 0; /* byte limit */

    /*
     * TSS descriptor for trap 08
     */
    pDesc = &paGdt[pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS_TRAP08] >> 3];
    pDesc->Gen.u16LimitLow      = sizeof(VBOXTSS) - 1;
    pDesc->Gen.u4LimitHigh      = 0;
    RCPtrTSS = VM_RC_ADDR(pVM, &pVM->selm.s.TssTrap08);
    pDesc->Gen.u16BaseLow       = RT_LOWORD(RCPtrTSS);
    pDesc->Gen.u8BaseHigh1      = RT_BYTE3(RCPtrTSS);
    pDesc->Gen.u8BaseHigh2      = RT_BYTE4(RCPtrTSS);
    pDesc->Gen.u4Type           = X86_SEL_TYPE_SYS_386_TSS_AVAIL;
    pDesc->Gen.u1DescType       = 0; /* system */
    pDesc->Gen.u2Dpl            = 0; /* supervisor */
    pDesc->Gen.u1Present        = 1;
    pDesc->Gen.u1Available      = 0;
    pDesc->Gen.u1Long           = 0;
    pDesc->Gen.u1DefBig         = 0;
    pDesc->Gen.u1Granularity    = 0; /* byte limit */
}

/**
 * Applies relocations to data and code managed by this
 * component. This function will be called at init and
 * whenever the VMM need to relocate it self inside the GC.
 *
 * @param   pVM     The cross context VM structure.
 */
VMMR3DECL(void) SELMR3Relocate(PVM pVM)
{
    PX86DESC paGdt = pVM->selm.s.paGdtR3;
    LogFlow(("SELMR3Relocate\n"));

    if (HMIsRawModeCtxNeeded(pVM))
    {
        for (VMCPUID i = 0; i < pVM->cCpus; i++)
        {
            PVMCPU pVCpu = &pVM->aCpus[i];

            /*
             * Update GDTR and selector.
             */
            CPUMSetHyperGDTR(pVCpu, MMHyperR3ToRC(pVM, paGdt), SELM_GDT_ELEMENTS * sizeof(paGdt[0]) - 1);

            /** @todo selector relocations should be a separate operation? */
            CPUMSetHyperCS(pVCpu, pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS]);
            CPUMSetHyperDS(pVCpu, pVM->selm.s.aHyperSel[SELM_HYPER_SEL_DS]);
            CPUMSetHyperES(pVCpu, pVM->selm.s.aHyperSel[SELM_HYPER_SEL_DS]);
            CPUMSetHyperSS(pVCpu, pVM->selm.s.aHyperSel[SELM_HYPER_SEL_DS]);
            CPUMSetHyperTR(pVCpu, pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS]);
        }

        selmR3SetupHyperGDTSelectors(pVM);

/** @todo SELM must be called when any of the CR3s changes during a cpu mode change. */
/** @todo PGM knows the proper CR3 values these days, not CPUM. */
        /*
         * Update the TSSes.
         */
        /* Only applies to raw mode which supports only 1 VCPU */
        PVMCPU pVCpu = &pVM->aCpus[0];

        /* Current TSS */
        pVM->selm.s.Tss.cr3     = PGMGetHyperCR3(pVCpu);
        pVM->selm.s.Tss.ss0     = pVM->selm.s.aHyperSel[SELM_HYPER_SEL_DS];
        pVM->selm.s.Tss.esp0    = VMMGetStackRC(pVCpu);
        pVM->selm.s.Tss.cs      = pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS];
        pVM->selm.s.Tss.ds      = pVM->selm.s.aHyperSel[SELM_HYPER_SEL_DS];
        pVM->selm.s.Tss.es      = pVM->selm.s.aHyperSel[SELM_HYPER_SEL_DS];
        pVM->selm.s.Tss.offIoBitmap = sizeof(VBOXTSS);

        /* trap 08 */
        pVM->selm.s.TssTrap08.cr3    = PGMGetInterRCCR3(pVM, pVCpu);                   /* this should give use better survival chances. */
        pVM->selm.s.TssTrap08.ss0    = pVM->selm.s.aHyperSel[SELM_HYPER_SEL_DS];
        pVM->selm.s.TssTrap08.ss     = pVM->selm.s.aHyperSel[SELM_HYPER_SEL_DS];
        pVM->selm.s.TssTrap08.esp0   = VMMGetStackRC(pVCpu) - PAGE_SIZE / 2;  /* upper half can be analysed this way. */
        pVM->selm.s.TssTrap08.esp    = pVM->selm.s.TssTrap08.esp0;
        pVM->selm.s.TssTrap08.ebp    = pVM->selm.s.TssTrap08.esp0;
        pVM->selm.s.TssTrap08.cs     = pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS];
        pVM->selm.s.TssTrap08.ds     = pVM->selm.s.aHyperSel[SELM_HYPER_SEL_DS];
        pVM->selm.s.TssTrap08.es     = pVM->selm.s.aHyperSel[SELM_HYPER_SEL_DS];
        pVM->selm.s.TssTrap08.fs     = 0;
        pVM->selm.s.TssTrap08.gs     = 0;
        pVM->selm.s.TssTrap08.selLdt = 0;
        pVM->selm.s.TssTrap08.eflags = 0x2;    /* all cleared */
        pVM->selm.s.TssTrap08.ecx    = VM_RC_ADDR(pVM, &pVM->selm.s.Tss);       /* setup ecx to normal Hypervisor TSS address. */
        pVM->selm.s.TssTrap08.edi    = pVM->selm.s.TssTrap08.ecx;
        pVM->selm.s.TssTrap08.eax    = pVM->selm.s.TssTrap08.ecx;
        pVM->selm.s.TssTrap08.edx    = VM_RC_ADDR(pVM, pVM);                    /* setup edx VM address. */
        pVM->selm.s.TssTrap08.edi    = pVM->selm.s.TssTrap08.edx;
        pVM->selm.s.TssTrap08.ebx    = pVM->selm.s.TssTrap08.edx;
        pVM->selm.s.TssTrap08.offIoBitmap = sizeof(VBOXTSS);
        /* TRPM will be updating the eip */
    }

    if (!HMIsEnabled(pVM))
    {
        /*
         * Update shadow GDT/LDT/TSS write access handlers.
         */
        PVMCPU pVCpu = VMMGetCpu(pVM); NOREF(pVCpu);
        int rc; NOREF(rc);
#ifdef SELM_TRACK_SHADOW_GDT_CHANGES
        if (pVM->selm.s.paGdtRC != NIL_RTRCPTR)
        {
            rc = PGMHandlerVirtualDeregister(pVM, pVCpu, pVM->selm.s.paGdtRC, true /*fHypervisor*/);
            AssertRC(rc);
        }
        pVM->selm.s.paGdtRC = MMHyperR3ToRC(pVM, paGdt);
        rc = PGMR3HandlerVirtualRegister(pVM, pVCpu, pVM->selm.s.hShadowGdtWriteHandlerType,
                                         pVM->selm.s.paGdtRC,
                                         pVM->selm.s.paGdtRC + SELM_GDT_ELEMENTS * sizeof(paGdt[0]) - 1,
                                         NULL /*pvUserR3*/, NIL_RTR0PTR /*pvUserRC*/, NULL /*pszDesc*/);
        AssertRC(rc);
#endif
#ifdef SELM_TRACK_SHADOW_TSS_CHANGES
        if (pVM->selm.s.pvMonShwTssRC != RTRCPTR_MAX)
        {
            rc = PGMHandlerVirtualDeregister(pVM, pVCpu, pVM->selm.s.pvMonShwTssRC, true /*fHypervisor*/);
            AssertRC(rc);
        }
        pVM->selm.s.pvMonShwTssRC = VM_RC_ADDR(pVM, &pVM->selm.s.Tss);
        rc = PGMR3HandlerVirtualRegister(pVM, pVCpu, pVM->selm.s.hShadowTssWriteHandlerType,
                                         pVM->selm.s.pvMonShwTssRC,
                                         pVM->selm.s.pvMonShwTssRC + sizeof(pVM->selm.s.Tss) - 1,
                                         NULL /*pvUserR3*/, NIL_RTR0PTR /*pvUserRC*/, NULL /*pszDesc*/);
        AssertRC(rc);
#endif

        /*
         * Update the GC LDT region handler and address.
         */
#ifdef SELM_TRACK_SHADOW_LDT_CHANGES
        if (pVM->selm.s.pvLdtRC != RTRCPTR_MAX)
        {
            rc = PGMHandlerVirtualDeregister(pVM, pVCpu, pVM->selm.s.pvLdtRC, true /*fHypervisor*/);
            AssertRC(rc);
        }
#endif
        pVM->selm.s.pvLdtRC = MMHyperR3ToRC(pVM, pVM->selm.s.pvLdtR3);
#ifdef SELM_TRACK_SHADOW_LDT_CHANGES
        rc = PGMR3HandlerVirtualRegister(pVM, pVCpu, pVM->selm.s.hShadowLdtWriteHandlerType,
                                         pVM->selm.s.pvLdtRC,
                                         pVM->selm.s.pvLdtRC + _64K + PAGE_SIZE - 1,
                                         NULL /*pvUserR3*/, NIL_RTR0PTR /*pvUserRC*/, NULL /*pszDesc*/);
        AssertRC(rc);
#endif
    }
}


/**
 * Terminates the SELM.
 *
 * Termination means cleaning up and freeing all resources,
 * the VM it self is at this point powered off or suspended.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3DECL(int) SELMR3Term(PVM pVM)
{
    NOREF(pVM);
    return VINF_SUCCESS;
}


/**
 * The VM is being reset.
 *
 * For the SELM component this means that any GDT/LDT/TSS monitors
 * needs to be removed.
 *
 * @param   pVM     The cross context VM structure.
 */
VMMR3DECL(void) SELMR3Reset(PVM pVM)
{
    LogFlow(("SELMR3Reset:\n"));
    VM_ASSERT_EMT(pVM);

    /*
     * Uninstall guest GDT/LDT/TSS write access handlers.
     */
    PVMCPU pVCpu = VMMGetCpu(pVM); NOREF(pVCpu);
    if (pVM->selm.s.GuestGdtr.pGdt != RTRCPTR_MAX && pVM->selm.s.fGDTRangeRegistered)
    {
#ifdef SELM_TRACK_GUEST_GDT_CHANGES
        int rc = PGMHandlerVirtualDeregister(pVM, pVCpu, pVM->selm.s.GuestGdtr.pGdt, false /*fHypervisor*/);
        AssertRC(rc);
#endif
        pVM->selm.s.GuestGdtr.pGdt = RTRCPTR_MAX;
        pVM->selm.s.GuestGdtr.cbGdt = 0;
    }
    pVM->selm.s.fGDTRangeRegistered = false;
    if (pVM->selm.s.GCPtrGuestLdt != RTRCPTR_MAX)
    {
#ifdef SELM_TRACK_GUEST_LDT_CHANGES
        int rc = PGMHandlerVirtualDeregister(pVM, pVCpu, pVM->selm.s.GCPtrGuestLdt, false /*fHypervisor*/);
        AssertRC(rc);
#endif
        pVM->selm.s.GCPtrGuestLdt = RTRCPTR_MAX;
    }
    if (pVM->selm.s.GCPtrGuestTss != RTRCPTR_MAX)
    {
#ifdef SELM_TRACK_GUEST_TSS_CHANGES
        int rc = PGMHandlerVirtualDeregister(pVM, pVCpu, pVM->selm.s.GCPtrGuestTss, false /*fHypervisor*/);
        AssertRC(rc);
#endif
        pVM->selm.s.GCPtrGuestTss = RTRCPTR_MAX;
        pVM->selm.s.GCSelTss      = RTSEL_MAX;
    }

    /*
     * Re-initialize other members.
     */
    pVM->selm.s.cbLdtLimit = 0;
    pVM->selm.s.offLdtHyper = 0;
    pVM->selm.s.cbMonitoredGuestTss = 0;

    pVM->selm.s.fSyncTSSRing0Stack = false;

#ifdef VBOX_WITH_RAW_MODE
    if (!HMIsEnabled(pVM))
    {
        /*
         * Default action when entering raw mode for the first time
         */
        VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_TSS);
        VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_GDT);
        VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_LDT);
    }
#endif
}


/**
 * Execute state save operation.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pSSM            SSM operation handle.
 */
static DECLCALLBACK(int) selmR3Save(PVM pVM, PSSMHANDLE pSSM)
{
    LogFlow(("selmR3Save:\n"));

    /*
     * Save the basic bits - fortunately all the other things can be resynced on load.
     */
    PSELM pSelm = &pVM->selm.s;

    SSMR3PutBool(pSSM, HMIsEnabled(pVM));
    SSMR3PutBool(pSSM, pSelm->fSyncTSSRing0Stack);
    SSMR3PutSel(pSSM, pSelm->aHyperSel[SELM_HYPER_SEL_CS]);
    SSMR3PutSel(pSSM, pSelm->aHyperSel[SELM_HYPER_SEL_DS]);
    SSMR3PutSel(pSSM, pSelm->aHyperSel[SELM_HYPER_SEL_CS64]);
    SSMR3PutSel(pSSM, pSelm->aHyperSel[SELM_HYPER_SEL_CS64]); /* reserved for DS64. */
    SSMR3PutSel(pSSM, pSelm->aHyperSel[SELM_HYPER_SEL_TSS]);
    return SSMR3PutSel(pSSM, pSelm->aHyperSel[SELM_HYPER_SEL_TSS_TRAP08]);
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
static DECLCALLBACK(int) selmR3Load(PVM pVM, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    LogFlow(("selmR3Load:\n"));
    Assert(uPass == SSM_PASS_FINAL); NOREF(uPass);

    /*
     * Validate version.
     */
    if (uVersion != SELM_SAVED_STATE_VERSION)
    {
        AssertMsgFailed(("selmR3Load: Invalid version uVersion=%d!\n", uVersion));
        return VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;
    }

    /*
     * Do a reset.
     */
    SELMR3Reset(pVM);

    /* Get the monitoring flag. */
    bool fIgnored;
    SSMR3GetBool(pSSM, &fIgnored);

    /* Get the TSS state flag. */
    SSMR3GetBool(pSSM, &pVM->selm.s.fSyncTSSRing0Stack);

    /*
     * Get the selectors.
     */
    RTSEL SelCS;
    SSMR3GetSel(pSSM, &SelCS);
    RTSEL SelDS;
    SSMR3GetSel(pSSM, &SelDS);
    RTSEL SelCS64;
    SSMR3GetSel(pSSM, &SelCS64);
    RTSEL SelDS64;
    SSMR3GetSel(pSSM, &SelDS64);
    RTSEL SelTSS;
    SSMR3GetSel(pSSM, &SelTSS);
    RTSEL SelTSSTrap08;
    SSMR3GetSel(pSSM, &SelTSSTrap08);

    /* Copy the selectors; they will be checked during relocation. */
    PSELM pSelm = &pVM->selm.s;
    pSelm->aHyperSel[SELM_HYPER_SEL_CS]         = SelCS;
    pSelm->aHyperSel[SELM_HYPER_SEL_DS]         = SelDS;
    pSelm->aHyperSel[SELM_HYPER_SEL_CS64]       = SelCS64;
    pSelm->aHyperSel[SELM_HYPER_SEL_TSS]        = SelTSS;
    pSelm->aHyperSel[SELM_HYPER_SEL_TSS_TRAP08] = SelTSSTrap08;

    return VINF_SUCCESS;
}


/**
 * Sync the GDT, LDT and TSS after loading the state.
 *
 * Just to play save, we set the FFs to force syncing before
 * executing GC code.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pSSM            SSM operation handle.
 */
static DECLCALLBACK(int) selmR3LoadDone(PVM pVM, PSSMHANDLE pSSM)
{
#ifdef VBOX_WITH_RAW_MODE
    if (!HMIsEnabled(pVM))
    {
        PVMCPU pVCpu = VMMGetCpu(pVM);

        LogFlow(("selmR3LoadDone:\n"));

        /*
         * Don't do anything if it's a load failure.
         */
        int rc = SSMR3HandleGetStatus(pSSM);
        if (RT_FAILURE(rc))
            return VINF_SUCCESS;

        /*
         * Do the syncing if we're in protected mode and using raw-mode.
         */
        if (PGMGetGuestMode(pVCpu) != PGMMODE_REAL)
        {
            VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_GDT);
            VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_LDT);
            VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_TSS);
            SELMR3UpdateFromCPUM(pVM, pVCpu);
        }

        /*
         * Flag everything for resync on next raw mode entry.
         */
        VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_GDT);
        VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_LDT);
        VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_TSS);
    }

#else  /* !VBOX_WITH_RAW_MODE */
    RT_NOREF(pVM, pSSM);
#endif /* !VBOX_WITH_RAW_MODE */
    return VINF_SUCCESS;
}

#ifdef VBOX_WITH_RAW_MODE

/**
 * Updates (syncs) the shadow GDT.
 *
 * @returns VBox status code.
 * @param   pVM                 The cross context VM structure.
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 */
static int selmR3UpdateShadowGdt(PVM pVM, PVMCPU pVCpu)
{
    LogFlow(("selmR3UpdateShadowGdt\n"));
    Assert(!HMIsEnabled(pVM));

    /*
     * Always assume the best...
     */
    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_SELM_SYNC_GDT);

    /* If the GDT was changed, then make sure the LDT is checked too */
    /** @todo only do this if the actual ldtr selector was changed; this is a bit excessive */
    VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_LDT);
    /* Same goes for the TSS selector */
    VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_TSS);

    /*
     * Get the GDTR and check if there is anything to do (there usually is).
     */
    VBOXGDTR    GDTR;
    CPUMGetGuestGDTR(pVCpu, &GDTR);
    if (GDTR.cbGdt < sizeof(X86DESC))
    {
        Log(("No GDT entries...\n"));
        return VINF_SUCCESS;
    }

    /*
     * Read the Guest GDT.
     * ASSUMES that the entire GDT is in memory.
     */
    RTUINT      cbEffLimit = GDTR.cbGdt;
    PX86DESC    pGDTE = &pVM->selm.s.paGdtR3[1];
    int rc = PGMPhysSimpleReadGCPtr(pVCpu, pGDTE, GDTR.pGdt + sizeof(X86DESC), cbEffLimit + 1 - sizeof(X86DESC));
    if (RT_FAILURE(rc))
    {
        /*
         * Read it page by page.
         *
         * Keep track of the last valid page and delay memsets and
         * adjust cbEffLimit to reflect the effective size.  The latter
         * is something we do in the belief that the guest will probably
         * never actually commit the last page, thus allowing us to keep
         * our selectors in the high end of the GDT.
         */
        RTUINT  cbLeft         = cbEffLimit + 1 - sizeof(X86DESC);
        RTGCPTR GCPtrSrc       = (RTGCPTR)GDTR.pGdt + sizeof(X86DESC);
        uint8_t *pu8Dst        = (uint8_t *)&pVM->selm.s.paGdtR3[1];
        uint8_t *pu8DstInvalid = pu8Dst;

        while (cbLeft)
        {
            RTUINT cb = PAGE_SIZE - (GCPtrSrc & PAGE_OFFSET_MASK);
            cb = RT_MIN(cb, cbLeft);
            rc = PGMPhysSimpleReadGCPtr(pVCpu, pu8Dst, GCPtrSrc, cb);
            if (RT_SUCCESS(rc))
            {
                if (pu8DstInvalid != pu8Dst)
                    RT_BZERO(pu8DstInvalid, pu8Dst - pu8DstInvalid);
                GCPtrSrc += cb;
                pu8Dst += cb;
                pu8DstInvalid = pu8Dst;
            }
            else if (   rc == VERR_PAGE_NOT_PRESENT
                     || rc == VERR_PAGE_TABLE_NOT_PRESENT)
            {
                GCPtrSrc += cb;
                pu8Dst += cb;
            }
            else
            {
                AssertLogRelMsgFailed(("Couldn't read GDT at %016RX64, rc=%Rrc!\n", GDTR.pGdt, rc));
                return VERR_SELM_GDT_READ_ERROR;
            }
            cbLeft -= cb;
        }

        /* any invalid pages at the end? */
        if (pu8DstInvalid != pu8Dst)
        {
            cbEffLimit = pu8DstInvalid - (uint8_t *)pVM->selm.s.paGdtR3 - 1;
            /* If any GDTEs was invalidated, zero them. */
            if (cbEffLimit < pVM->selm.s.cbEffGuestGdtLimit)
                RT_BZERO(pu8DstInvalid + cbEffLimit + 1, pVM->selm.s.cbEffGuestGdtLimit - cbEffLimit);
        }

        /* keep track of the effective limit. */
        if (cbEffLimit != pVM->selm.s.cbEffGuestGdtLimit)
        {
            Log(("SELMR3UpdateFromCPUM: cbEffGuestGdtLimit=%#x -> %#x (actual %#x)\n",
                 pVM->selm.s.cbEffGuestGdtLimit, cbEffLimit, GDTR.cbGdt));
            pVM->selm.s.cbEffGuestGdtLimit = cbEffLimit;
        }
    }

    /*
     * Check if the Guest GDT intrudes on our GDT entries.
     */
    /** @todo we should try to minimize relocations by making sure our current selectors can be reused. */
    RTSEL aHyperSel[SELM_HYPER_SEL_MAX];
    if (cbEffLimit >= SELM_HYPER_DEFAULT_BASE)
    {
        PX86DESC    pGDTEStart = pVM->selm.s.paGdtR3;
        PX86DESC    pGDTECur   = (PX86DESC)((char *)pGDTEStart + GDTR.cbGdt + 1 - sizeof(X86DESC));
        int         iGDT       = 0;

        Log(("Internal SELM GDT conflict: use non-present entries\n"));
        STAM_REL_COUNTER_INC(&pVM->selm.s.StatScanForHyperSels);
        while ((uintptr_t)pGDTECur > (uintptr_t)pGDTEStart)
        {
            /* We can reuse non-present entries */
            if (!pGDTECur->Gen.u1Present)
            {
                aHyperSel[iGDT] = ((uintptr_t)pGDTECur - (uintptr_t)pVM->selm.s.paGdtR3) / sizeof(X86DESC);
                aHyperSel[iGDT] = aHyperSel[iGDT] << X86_SEL_SHIFT;
                Log(("SELM: Found unused GDT %04X\n", aHyperSel[iGDT]));
                iGDT++;
                if (iGDT >= SELM_HYPER_SEL_MAX)
                    break;
            }

            pGDTECur--;
        }
        if (iGDT != SELM_HYPER_SEL_MAX)
        {
            AssertLogRelMsgFailed(("Internal SELM GDT conflict.\n"));
            return VERR_SELM_GDT_TOO_FULL;
        }
    }
    else
    {
        aHyperSel[SELM_HYPER_SEL_CS]         = SELM_HYPER_DEFAULT_SEL_CS;
        aHyperSel[SELM_HYPER_SEL_DS]         = SELM_HYPER_DEFAULT_SEL_DS;
        aHyperSel[SELM_HYPER_SEL_CS64]       = SELM_HYPER_DEFAULT_SEL_CS64;
        aHyperSel[SELM_HYPER_SEL_TSS]        = SELM_HYPER_DEFAULT_SEL_TSS;
        aHyperSel[SELM_HYPER_SEL_TSS_TRAP08] = SELM_HYPER_DEFAULT_SEL_TSS_TRAP08;
    }

# ifdef VBOX_WITH_SAFE_STR
    /* Use the guest's TR selector to plug the str virtualization hole. */
    if (CPUMGetGuestTR(pVCpu, NULL) != 0)
    {
        Log(("SELM: Use guest TSS selector %x\n", CPUMGetGuestTR(pVCpu, NULL)));
        aHyperSel[SELM_HYPER_SEL_TSS] = CPUMGetGuestTR(pVCpu, NULL);
    }
# endif

    /*
     * Work thru the copied GDT entries adjusting them for correct virtualization.
     */
    PX86DESC pGDTEEnd = (PX86DESC)((char *)pGDTE + cbEffLimit + 1 - sizeof(X86DESC));
    while (pGDTE < pGDTEEnd)
    {
        if (pGDTE->Gen.u1Present)
            selmGuestToShadowDesc(pVM, pGDTE);

        /* Next GDT entry. */
        pGDTE++;
    }

    /*
     * Check if our hypervisor selectors were changed.
     */
    if (    aHyperSel[SELM_HYPER_SEL_CS]         != pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS]
        ||  aHyperSel[SELM_HYPER_SEL_DS]         != pVM->selm.s.aHyperSel[SELM_HYPER_SEL_DS]
        ||  aHyperSel[SELM_HYPER_SEL_CS64]       != pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS64]
        ||  aHyperSel[SELM_HYPER_SEL_TSS]        != pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS]
        ||  aHyperSel[SELM_HYPER_SEL_TSS_TRAP08] != pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS_TRAP08])
    {
        /* Reinitialize our hypervisor GDTs */
        pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS]         = aHyperSel[SELM_HYPER_SEL_CS];
        pVM->selm.s.aHyperSel[SELM_HYPER_SEL_DS]         = aHyperSel[SELM_HYPER_SEL_DS];
        pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS64]       = aHyperSel[SELM_HYPER_SEL_CS64];
        pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS]        = aHyperSel[SELM_HYPER_SEL_TSS];
        pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS_TRAP08] = aHyperSel[SELM_HYPER_SEL_TSS_TRAP08];

        STAM_REL_COUNTER_INC(&pVM->selm.s.StatHyperSelsChanged);

        /*
         * Do the relocation callbacks to let everyone update their hyper selector dependencies.
         * (SELMR3Relocate will call selmR3SetupHyperGDTSelectors() for us.)
         */
        VMR3Relocate(pVM, 0);
    }
# ifdef VBOX_WITH_SAFE_STR
    else if (   cbEffLimit >= SELM_HYPER_DEFAULT_BASE
             || CPUMGetGuestTR(pVCpu, NULL) != 0) /* Our shadow TR entry was overwritten when we synced the guest's GDT. */
# else
    else if (cbEffLimit >= SELM_HYPER_DEFAULT_BASE)
# endif
        /* We overwrote all entries above, so we have to save them again. */
        selmR3SetupHyperGDTSelectors(pVM);

    /*
     * Adjust the cached GDT limit.
     * Any GDT entries which have been removed must be cleared.
     */
    if (pVM->selm.s.GuestGdtr.cbGdt != GDTR.cbGdt)
    {
        if (pVM->selm.s.GuestGdtr.cbGdt > GDTR.cbGdt)
            RT_BZERO(pGDTE, pVM->selm.s.GuestGdtr.cbGdt - GDTR.cbGdt);
    }

    /*
     * Check if Guest's GDTR is changed.
     */
    if (    GDTR.pGdt  != pVM->selm.s.GuestGdtr.pGdt
        ||  GDTR.cbGdt != pVM->selm.s.GuestGdtr.cbGdt)
    {
        Log(("SELMR3UpdateFromCPUM: Guest's GDT is changed to pGdt=%016RX64 cbGdt=%08X\n", GDTR.pGdt, GDTR.cbGdt));

# ifdef SELM_TRACK_GUEST_GDT_CHANGES
        /*
         * [Re]Register write virtual handler for guest's GDT.
         */
        if (pVM->selm.s.GuestGdtr.pGdt != RTRCPTR_MAX && pVM->selm.s.fGDTRangeRegistered)
        {
            rc = PGMHandlerVirtualDeregister(pVM, pVCpu, pVM->selm.s.GuestGdtr.pGdt, false /*fHypervisor*/);
            AssertRC(rc);
        }
        rc = PGMR3HandlerVirtualRegister(pVM, pVCpu, pVM->selm.s.hGuestGdtWriteHandlerType,
                                         GDTR.pGdt, GDTR.pGdt + GDTR.cbGdt /* already inclusive */,
                                         NULL /*pvUserR3*/, NIL_RTR0PTR /*pvUserRC*/, NULL /*pszDesc*/);
#  ifdef VBOX_WITH_RAW_RING1
        /** @todo !HACK ALERT!
         * Some guest OSes (QNX) share code and the GDT on the same page;
         * PGMR3HandlerVirtualRegister doesn't support more than one handler,
         * so we kick out the PATM handler as this one is more important. Fix this
         * properly in PGMR3HandlerVirtualRegister?
         */
        if (rc == VERR_PGM_HANDLER_VIRTUAL_CONFLICT)
        {
            LogRel(("selmR3UpdateShadowGdt: Virtual handler conflict %RGv -> kick out PATM handler for the higher priority GDT page monitor\n", GDTR.pGdt));
            rc = PGMHandlerVirtualDeregister(pVM, pVCpu, GDTR.pGdt & PAGE_BASE_GC_MASK, false /*fHypervisor*/);
            AssertRC(rc);
            rc = PGMR3HandlerVirtualRegister(pVM, pVCpu, pVM->selm.s.hGuestGdtWriteHandlerType,
                                             GDTR.pGdt, GDTR.pGdt + GDTR.cbGdt /* already inclusive */,
                                             NULL /*pvUserR3*/, NIL_RTR0PTR /*pvUserRC*/, NULL /*pszDesc*/);
        }
#  endif
        if (RT_FAILURE(rc))
            return rc;
# endif /* SELM_TRACK_GUEST_GDT_CHANGES */

        /* Update saved Guest GDTR. */
        pVM->selm.s.GuestGdtr = GDTR;
        pVM->selm.s.fGDTRangeRegistered = true;
    }

    return VINF_SUCCESS;
}


/**
 * Updates (syncs) the shadow LDT.
 *
 * @returns VBox status code.
 * @param   pVM                 The cross context VM structure.
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 */
static int selmR3UpdateShadowLdt(PVM pVM, PVMCPU pVCpu)
{
    LogFlow(("selmR3UpdateShadowLdt\n"));
    int rc = VINF_SUCCESS;
    Assert(!HMIsEnabled(pVM));

    /*
     * Always assume the best...
     */
    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_SELM_SYNC_LDT);

    /*
     * LDT handling is done similarly to the GDT handling with a shadow
     * array. However, since the LDT is expected to be swappable (at least
     * some ancient OSes makes it swappable) it must be floating and
     * synced on a per-page basis.
     *
     * Eventually we will change this to be fully on demand. Meaning that
     * we will only sync pages containing LDT selectors actually used and
     * let the #PF handler lazily sync pages as they are used.
     * (This applies to GDT too, when we start making OS/2 fast.)
     */

    /*
     * First, determine the current LDT selector.
     */
    RTSEL SelLdt = CPUMGetGuestLDTR(pVCpu);
    if (!(SelLdt & X86_SEL_MASK_OFF_RPL))
    {
        /* ldtr = 0 - update hyper LDTR and deregister any active handler. */
        CPUMSetHyperLDTR(pVCpu, 0);
        if (pVM->selm.s.GCPtrGuestLdt != RTRCPTR_MAX)
        {
            rc = PGMHandlerVirtualDeregister(pVM, pVCpu, pVM->selm.s.GCPtrGuestLdt, false /*fHypervisor*/);
            AssertRC(rc);
            pVM->selm.s.GCPtrGuestLdt = RTRCPTR_MAX;
        }
        pVM->selm.s.cbLdtLimit = 0;
        return VINF_SUCCESS;
    }

    /*
     * Get the LDT selector.
     */
/** @todo this is wrong, use CPUMGetGuestLdtrEx */
    PX86DESC    pDesc    = &pVM->selm.s.paGdtR3[SelLdt >> X86_SEL_SHIFT];
    RTGCPTR     GCPtrLdt = X86DESC_BASE(pDesc);
    uint32_t    cbLdt    = X86DESC_LIMIT_G(pDesc);

    /*
     * Validate it.
     */
    if (    !cbLdt
        ||  SelLdt >= pVM->selm.s.GuestGdtr.cbGdt
        ||  pDesc->Gen.u1DescType
        ||  pDesc->Gen.u4Type != X86_SEL_TYPE_SYS_LDT)
    {
        AssertMsg(!cbLdt, ("Invalid LDT %04x!\n", SelLdt));

        /* cbLdt > 0:
         * This is quite impossible, so we do as most people do when faced with
         * the impossible, we simply ignore it.
         */
        CPUMSetHyperLDTR(pVCpu, 0);
        if (pVM->selm.s.GCPtrGuestLdt != RTRCPTR_MAX)
        {
            rc = PGMHandlerVirtualDeregister(pVM, pVCpu, pVM->selm.s.GCPtrGuestLdt, false /*fHypervisor*/);
            AssertRC(rc);
            pVM->selm.s.GCPtrGuestLdt = RTRCPTR_MAX;
        }
        return VINF_SUCCESS;
    }
    /** @todo check what intel does about odd limits. */
    AssertMsg(RT_ALIGN(cbLdt + 1, sizeof(X86DESC)) == cbLdt + 1 && cbLdt <= 0xffff, ("cbLdt=%d\n", cbLdt));

    /*
     * Use the cached guest ldt address if the descriptor has already been modified (see below)
     * (this is necessary due to redundant LDT updates; see todo above at GDT sync)
     */
    if (MMHyperIsInsideArea(pVM, GCPtrLdt))
        GCPtrLdt = pVM->selm.s.GCPtrGuestLdt;   /* use the old one */


    /** @todo Handle only present LDT segments. */
//    if (pDesc->Gen.u1Present)
    {
        /*
         * Check if Guest's LDT address/limit is changed.
         */
        if (    GCPtrLdt != pVM->selm.s.GCPtrGuestLdt
            ||  cbLdt != pVM->selm.s.cbLdtLimit)
        {
            Log(("SELMR3UpdateFromCPUM: Guest LDT changed to from %RGv:%04x to %RGv:%04x. (GDTR=%016RX64:%04x)\n",
                 pVM->selm.s.GCPtrGuestLdt, pVM->selm.s.cbLdtLimit, GCPtrLdt, cbLdt, pVM->selm.s.GuestGdtr.pGdt, pVM->selm.s.GuestGdtr.cbGdt));

# ifdef SELM_TRACK_GUEST_LDT_CHANGES
            /*
             * [Re]Register write virtual handler for guest's GDT.
             * In the event of LDT overlapping something, don't install it just assume it's being updated.
             */
            if (pVM->selm.s.GCPtrGuestLdt != RTRCPTR_MAX)
            {
                rc = PGMHandlerVirtualDeregister(pVM, pVCpu, pVM->selm.s.GCPtrGuestLdt, false /*fHypervisor*/);
                AssertRC(rc);
            }
#  ifdef LOG_ENABLED
            if (pDesc->Gen.u1Present)
                Log(("LDT selector marked not present!!\n"));
#  endif
            rc = PGMR3HandlerVirtualRegister(pVM, pVCpu, pVM->selm.s.hGuestLdtWriteHandlerType,
                                             GCPtrLdt, GCPtrLdt + cbLdt /* already inclusive */,
                                             NULL /*pvUserR3*/, NIL_RTR0PTR /*pvUserRC*/, NULL /*pszDesc*/);
            if (rc == VERR_PGM_HANDLER_VIRTUAL_CONFLICT)
            {
                /** @todo investigate the various cases where conflicts happen and try avoid them by enh. the instruction emulation. */
                pVM->selm.s.GCPtrGuestLdt = RTRCPTR_MAX;
                Log(("WARNING: Guest LDT (%RGv:%04x) conflicted with existing access range!! Assumes LDT is begin updated. (GDTR=%016RX64:%04x)\n",
                     GCPtrLdt, cbLdt, pVM->selm.s.GuestGdtr.pGdt, pVM->selm.s.GuestGdtr.cbGdt));
            }
            else if (RT_SUCCESS(rc))
                pVM->selm.s.GCPtrGuestLdt = GCPtrLdt;
            else
            {
                CPUMSetHyperLDTR(pVCpu, 0);
                return rc;
            }
# else
            pVM->selm.s.GCPtrGuestLdt = GCPtrLdt;
# endif
            pVM->selm.s.cbLdtLimit = cbLdt;
        }
    }

    /*
     * Calc Shadow LDT base.
     */
    unsigned    off;
    pVM->selm.s.offLdtHyper = off = (GCPtrLdt & PAGE_OFFSET_MASK);
    RTGCPTR     GCPtrShadowLDT  = (RTGCPTR)((RTGCUINTPTR)pVM->selm.s.pvLdtRC + off);
    PX86DESC    pShadowLDT      = (PX86DESC)((uintptr_t)pVM->selm.s.pvLdtR3 + off);

    /*
     * Enable the LDT selector in the shadow GDT.
     */
    pDesc->Gen.u1Present    = 1;
    pDesc->Gen.u16BaseLow   = RT_LOWORD(GCPtrShadowLDT);
    pDesc->Gen.u8BaseHigh1  = RT_BYTE3(GCPtrShadowLDT);
    pDesc->Gen.u8BaseHigh2  = RT_BYTE4(GCPtrShadowLDT);
    pDesc->Gen.u1Available  = 0;
    pDesc->Gen.u1Long       = 0;
    if (cbLdt > 0xffff)
    {
        cbLdt = 0xffff;
        pDesc->Gen.u4LimitHigh  = 0;
        pDesc->Gen.u16LimitLow  = pDesc->Gen.u1Granularity ? 0xf : 0xffff;
    }

    /*
     * Set Hyper LDTR and notify TRPM.
     */
    CPUMSetHyperLDTR(pVCpu, SelLdt);
    LogFlow(("selmR3UpdateShadowLdt: Hyper LDTR %#x\n", SelLdt));

    /*
     * Loop synchronising the LDT page by page.
     */
    /** @todo investigate how intel handle various operations on half present cross page entries. */
    off = GCPtrLdt & (sizeof(X86DESC) - 1);
    AssertMsg(!off, ("LDT is not aligned on entry size! GCPtrLdt=%08x\n", GCPtrLdt));

    /* Note: Do not skip the first selector; unlike the GDT, a zero LDT selector is perfectly valid. */
    unsigned    cbLeft = cbLdt + 1;
    PX86DESC    pLDTE = pShadowLDT;
    while (cbLeft)
    {
        /*
         * Read a chunk.
         */
        unsigned  cbChunk = PAGE_SIZE - ((RTGCUINTPTR)GCPtrLdt & PAGE_OFFSET_MASK);
        if (cbChunk > cbLeft)
            cbChunk = cbLeft;
        rc = PGMPhysSimpleReadGCPtr(pVCpu, pShadowLDT, GCPtrLdt, cbChunk);
        if (RT_SUCCESS(rc))
        {
            /*
             * Mark page
             */
            rc = PGMMapSetPage(pVM, GCPtrShadowLDT & PAGE_BASE_GC_MASK, PAGE_SIZE, X86_PTE_P | X86_PTE_A | X86_PTE_D);
            AssertRC(rc);

            /*
             * Loop thru the available LDT entries.
             * Figure out where to start and end and the potential cross pageness of
             * things adds a little complexity. pLDTE is updated there and not in the
             * 'next' part of the loop. The pLDTEEnd is inclusive.
             */
            PX86DESC pLDTEEnd = (PX86DESC)((uintptr_t)pShadowLDT + cbChunk) - 1;
            if (pLDTE + 1 < pShadowLDT)
                pLDTE = (PX86DESC)((uintptr_t)pShadowLDT + off);
            while (pLDTE <= pLDTEEnd)
            {
                if (pLDTE->Gen.u1Present)
                    selmGuestToShadowDesc(pVM, pLDTE);

                /* Next LDT entry. */
                pLDTE++;
            }
        }
        else
        {
            RT_BZERO(pShadowLDT, cbChunk);
            AssertMsg(rc == VERR_PAGE_NOT_PRESENT || rc == VERR_PAGE_TABLE_NOT_PRESENT, ("rc=%Rrc\n", rc));
            rc = PGMMapSetPage(pVM, GCPtrShadowLDT & PAGE_BASE_GC_MASK, PAGE_SIZE, 0);
            AssertRC(rc);
        }

        /*
         * Advance to the next page.
         */
        cbLeft          -= cbChunk;
        GCPtrShadowLDT  += cbChunk;
        pShadowLDT       = (PX86DESC)((char *)pShadowLDT + cbChunk);
        GCPtrLdt        += cbChunk;
    }

    return VINF_SUCCESS;
}


/**
 * Checks and updates segment selector registers.
 *
 * @returns VBox strict status code.
 * @retval  VINF_EM_RESCHEDULE_REM if a stale register was found.
 *
 * @param   pVM                 The cross context VM structure.
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 */
static VBOXSTRICTRC selmR3UpdateSegmentRegisters(PVM pVM, PVMCPU pVCpu)
{
    Assert(CPUMIsGuestInProtectedMode(pVCpu));
    Assert(!HMIsEnabled(pVM));

    /*
     * No stale selectors in V8086 mode.
     */
    PCPUMCTX        pCtx     = CPUMQueryGuestCtxPtr(pVCpu);
    if (pCtx->eflags.Bits.u1VM)
        return VINF_SUCCESS;

    /*
     * Check for stale selectors and load hidden register bits where they
     * are missing.
     */
    uint32_t        uCpl     = CPUMGetGuestCPL(pVCpu);
    VBOXSTRICTRC    rcStrict = VINF_SUCCESS;
    PCPUMSELREG     paSReg   = CPUMCTX_FIRST_SREG(pCtx);
    for (uint32_t iSReg = 0; iSReg < X86_SREG_COUNT; iSReg++)
    {
        RTSEL const Sel = paSReg[iSReg].Sel;
        if (Sel & X86_SEL_MASK_OFF_RPL)
        {
            /* Get the shadow descriptor entry corresponding to this. */
            static X86DESC const s_NotPresentDesc = { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } };
            PCX86DESC pDesc;
            if (!(Sel & X86_SEL_LDT))
            {
                if ((Sel | (sizeof(*pDesc) - 1)) <= pCtx->gdtr.cbGdt)
                    pDesc = &pVM->selm.s.paGdtR3[Sel >> X86_SEL_SHIFT];
                else
                    pDesc = &s_NotPresentDesc;
            }
            else
            {
                if ((Sel | (sizeof(*pDesc) - 1)) <= pVM->selm.s.cbLdtLimit)
                    pDesc = &((PCX86DESC)((uintptr_t)pVM->selm.s.pvLdtR3 + pVM->selm.s.offLdtHyper))[Sel >> X86_SEL_SHIFT];
                else
                    pDesc = &s_NotPresentDesc;
            }

            /* Check the segment register. */
            if (CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &paSReg[iSReg]))
            {
                if (!(paSReg[iSReg].fFlags & CPUMSELREG_FLAGS_STALE))
                {
                    /* Did it go stale? */
                    if (selmIsSRegStale32(&paSReg[iSReg], pDesc, iSReg))
                    {
                        Log2(("SELM: Detected stale %s=%#x (was valid)\n", g_aszSRegNms[iSReg], Sel));
                        STAM_REL_COUNTER_INC(&pVM->selm.s.aStatDetectedStaleSReg[iSReg]);
                        paSReg[iSReg].fFlags |= CPUMSELREG_FLAGS_STALE;
                        rcStrict = VINF_EM_RESCHEDULE_REM;
                    }
                }
                else
                {
                    /* Did it stop being stale? I.e. did the guest change it things
                       back to the way they were? */
                    if (!selmIsSRegStale32(&paSReg[iSReg], pDesc, iSReg))
                    {
                        STAM_REL_COUNTER_INC(&pVM->selm.s.StatStaleToUnstaleSReg);
                        paSReg[iSReg].fFlags &= CPUMSELREG_FLAGS_STALE;
                    }
                    else
                    {
                        Log2(("SELM: Already stale %s=%#x\n", g_aszSRegNms[iSReg], Sel));
                        STAM_REL_COUNTER_INC(&pVM->selm.s.aStatAlreadyStaleSReg[iSReg]);
                        rcStrict = VINF_EM_RESCHEDULE_REM;
                    }
                }
            }
            /* Load the hidden registers if it's a valid descriptor for the
               current segment register. */
            else if (selmIsShwDescGoodForSReg(&paSReg[iSReg], pDesc, iSReg, uCpl))
            {
                selmLoadHiddenSRegFromShadowDesc(&paSReg[iSReg], pDesc);
                STAM_COUNTER_INC(&pVM->selm.s.aStatUpdatedSReg[iSReg]);
            }
            /* It's stale. */
            else
            {
                Log2(("SELM: Detected stale %s=%#x (wasn't valid)\n", g_aszSRegNms[iSReg], Sel));
                STAM_REL_COUNTER_INC(&pVM->selm.s.aStatDetectedStaleSReg[iSReg]);
                paSReg[iSReg].fFlags = CPUMSELREG_FLAGS_STALE;
                rcStrict = VINF_EM_RESCHEDULE_REM;
            }
        }
        /* else: 0 selector, ignore. */
    }

    return rcStrict;
}


/**
 * Updates the Guest GDT & LDT virtualization based on current CPU state.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMR3DECL(VBOXSTRICTRC) SELMR3UpdateFromCPUM(PVM pVM, PVMCPU pVCpu)
{
    STAM_PROFILE_START(&pVM->selm.s.StatUpdateFromCPUM, a);
    AssertReturn(!HMIsEnabled(pVM), VERR_SELM_HM_IPE);

    /*
     * GDT sync
     */
    int rc;
    if (VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_SELM_SYNC_GDT))
    {
        rc = selmR3UpdateShadowGdt(pVM, pVCpu);
        if (RT_FAILURE(rc))
            return rc;                  /* We're toast, so forget the profiling. */
        AssertRCSuccess(rc);
    }

    /*
     * TSS sync
     */
    if (VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_SELM_SYNC_TSS))
    {
        rc = SELMR3SyncTSS(pVM, pVCpu);
        if (RT_FAILURE(rc))
            return rc;
        AssertRCSuccess(rc);
    }

    /*
     * LDT sync
     */
    if (VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_SELM_SYNC_LDT))
    {
        rc = selmR3UpdateShadowLdt(pVM, pVCpu);
        if (RT_FAILURE(rc))
            return rc;
        AssertRCSuccess(rc);
    }

    /*
     * Check selector registers.
     */
    VBOXSTRICTRC rcStrict = selmR3UpdateSegmentRegisters(pVM, pVCpu);

    STAM_PROFILE_STOP(&pVM->selm.s.StatUpdateFromCPUM, a);
    return rcStrict;
}


/**
 * Synchronize the shadowed fields in the TSS.
 *
 * At present we're shadowing the ring-0 stack selector & pointer, and the
 * interrupt redirection bitmap (if present). We take the lazy approach wrt to
 * REM and this function is called both if REM made any changes to the TSS or
 * loaded TR.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMR3DECL(int) SELMR3SyncTSS(PVM pVM, PVMCPU pVCpu)
{
    LogFlow(("SELMR3SyncTSS\n"));
    int rc;
    AssertReturnStmt(!HMIsEnabled(pVM), VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_SELM_SYNC_TSS), VINF_SUCCESS);

    STAM_PROFILE_START(&pVM->selm.s.StatTSSSync, a);
    Assert(VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_SELM_SYNC_TSS));

    /*
     * Get TR and extract and store the basic info.
     *
     * Note! The TSS limit is not checked by the LTR code, so we
     *       have to be a bit careful with it. We make sure cbTss
     *       won't be zero if TR is valid and if it's NULL we'll
     *       make sure cbTss is 0.
     */
/** @todo use the hidden bits, not shadow GDT. */
    CPUMSELREGHID   trHid;
    RTSEL           SelTss   = CPUMGetGuestTR(pVCpu, &trHid);
    RTGCPTR         GCPtrTss = trHid.u64Base;
    uint32_t        cbTss    = trHid.u32Limit;
    Assert(   (SelTss & X86_SEL_MASK_OFF_RPL)
           || (cbTss == 0 && GCPtrTss == 0 && trHid.Attr.u == 0 /* TR=0 */)
           || (cbTss == 0xffff && GCPtrTss == 0 && trHid.Attr.n.u1Present && trHid.Attr.n.u4Type == X86_SEL_TYPE_SYS_386_TSS_BUSY /* RESET */));
    if (SelTss & X86_SEL_MASK_OFF_RPL)
    {
        Assert(!(SelTss & X86_SEL_LDT));
        Assert(trHid.Attr.n.u1DescType == 0);
        Assert(   trHid.Attr.n.u4Type == X86_SEL_TYPE_SYS_286_TSS_BUSY
               || trHid.Attr.n.u4Type == X86_SEL_TYPE_SYS_386_TSS_BUSY);
        if (!++cbTss)
            cbTss = UINT32_MAX;
    }
    else
    {
        Assert(   (cbTss == 0 && GCPtrTss == 0 && trHid.Attr.u == 0 /* TR=0 */)
               || (cbTss == 0xffff && GCPtrTss == 0 && trHid.Attr.n.u1Present && trHid.Attr.n.u4Type == X86_SEL_TYPE_SYS_386_TSS_BUSY /* RESET */));
        cbTss = 0; /* the reset case. */
    }
    pVM->selm.s.cbGuestTss     = cbTss;
    pVM->selm.s.fGuestTss32Bit = trHid.Attr.n.u4Type == X86_SEL_TYPE_SYS_386_TSS_AVAIL
                              || trHid.Attr.n.u4Type == X86_SEL_TYPE_SYS_386_TSS_BUSY;

    /*
     * Figure out the size of what need to monitor.
     */
    /* We're not interested in any 16-bit TSSes. */
    uint32_t cbMonitoredTss = cbTss;
    if (    trHid.Attr.n.u4Type != X86_SEL_TYPE_SYS_386_TSS_AVAIL
        &&  trHid.Attr.n.u4Type != X86_SEL_TYPE_SYS_386_TSS_BUSY)
        cbMonitoredTss = 0;

    pVM->selm.s.offGuestIoBitmap = 0;
    bool fNoRing1Stack = true;
    if (cbMonitoredTss)
    {
        /*
         * 32-bit TSS. What we're really keen on is the SS0 and ESP0 fields.
         * If VME is enabled we also want to keep an eye on the interrupt
         * redirection bitmap.
         */
        VBOXTSS Tss;
        uint32_t cr4 = CPUMGetGuestCR4(pVCpu);
        rc = PGMPhysSimpleReadGCPtr(pVCpu, &Tss, GCPtrTss, RT_OFFSETOF(VBOXTSS, IntRedirBitmap));
        if (    !(cr4 & X86_CR4_VME)
            ||   (  RT_SUCCESS(rc)
                 && Tss.offIoBitmap < sizeof(VBOXTSS) /* too small */
                 && Tss.offIoBitmap > cbTss)          /* beyond the end */ /** @todo not sure how the partial case is handled; probably not allowed. */
           )
            /* No interrupt redirection bitmap, just ESP0 and SS0. */
            cbMonitoredTss = RT_UOFFSETOF(VBOXTSS, padding_ss0);
        else if (RT_SUCCESS(rc))
        {
            /*
             * Everything up to and including the interrupt redirection bitmap. Unfortunately
             * this can be quite a large chunk. We use to skip it earlier and just hope it
             * was kind of static...
             *
             * Update the virtual interrupt redirection bitmap while we're here.
             * (It is located in the 32 bytes before TR:offIoBitmap.)
             */
            cbMonitoredTss = Tss.offIoBitmap;
            pVM->selm.s.offGuestIoBitmap = Tss.offIoBitmap;

            uint32_t offRedirBitmap = Tss.offIoBitmap - sizeof(Tss.IntRedirBitmap);
            rc = PGMPhysSimpleReadGCPtr(pVCpu, &pVM->selm.s.Tss.IntRedirBitmap,
                                        GCPtrTss + offRedirBitmap, sizeof(Tss.IntRedirBitmap));
            AssertRC(rc);
            /** @todo memset the bitmap on failure? */
            Log2(("Redirection bitmap:\n"));
            Log2(("%.*Rhxd\n", sizeof(Tss.IntRedirBitmap), &pVM->selm.s.Tss.IntRedirBitmap));
        }
        else
        {
            cbMonitoredTss = RT_OFFSETOF(VBOXTSS, IntRedirBitmap);
            pVM->selm.s.offGuestIoBitmap = 0;
            /** @todo memset the bitmap? */
        }

        /*
         * Update the ring 0 stack selector and base address.
         */
        if (RT_SUCCESS(rc))
        {
# ifdef LOG_ENABLED
            if (LogIsEnabled())
            {
                uint32_t ssr0, espr0;
                SELMGetRing1Stack(pVM, &ssr0, &espr0);
                if ((ssr0 & ~1) != Tss.ss0 || espr0 != Tss.esp0)
                {
                    RTGCPHYS GCPhys = NIL_RTGCPHYS;
                    rc = PGMGstGetPage(pVCpu, GCPtrTss, NULL, &GCPhys); AssertRC(rc);
                    Log(("SELMR3SyncTSS: Updating TSS ring 0 stack to %04X:%08X from %04X:%08X; TSS Phys=%RGp)\n",
                         Tss.ss0, Tss.esp0, (ssr0 & ~1), espr0,  GCPhys));
                    AssertMsg(ssr0 != Tss.ss0,
                              ("ring-1 leak into TSS.SS0! %04X:%08X from %04X:%08X; TSS Phys=%RGp)\n",
                               Tss.ss0, Tss.esp0, (ssr0 & ~1), espr0,  GCPhys));
                }
                Log(("offIoBitmap=%#x\n", Tss.offIoBitmap));
            }
# endif /* LOG_ENABLED */
            AssertMsg(!(Tss.ss0 & 3), ("ring-1 leak into TSS.SS0? %04X:%08X\n", Tss.ss0, Tss.esp0));

            /* Update our TSS structure for the guest's ring 1 stack */
            selmSetRing1Stack(pVM, Tss.ss0 | 1, Tss.esp0);
            pVM->selm.s.fSyncTSSRing0Stack = fNoRing1Stack = false;

# ifdef VBOX_WITH_RAW_RING1
            /* Update our TSS structure for the guest's ring 2 stack */
            if (EMIsRawRing1Enabled(pVM))
            {
                if (    (pVM->selm.s.Tss.ss2 != ((Tss.ss1 & ~2) | 1))
                    ||  pVM->selm.s.Tss.esp2 != Tss.esp1)
                    Log(("SELMR3SyncTSS: Updating TSS ring 1 stack to %04X:%08X from %04X:%08X\n", Tss.ss1, Tss.esp1, (pVM->selm.s.Tss.ss2 & ~2) | 1, pVM->selm.s.Tss.esp2));
                selmSetRing2Stack(pVM, (Tss.ss1 & ~1) | 2, Tss.esp1);
            }
# endif
        }
    }

    /*
     * Flush the ring-1 stack and the direct syscall dispatching if we
     * cannot obtain SS0:ESP0.
     */
    if (fNoRing1Stack)
    {
        selmSetRing1Stack(pVM, 0 /* invalid SS */, 0);
        pVM->selm.s.fSyncTSSRing0Stack = cbMonitoredTss != 0;

        /** @todo handle these dependencies better! */
        TRPMR3SetGuestTrapHandler(pVM, 0x2E, TRPM_INVALID_HANDLER);
        TRPMR3SetGuestTrapHandler(pVM, 0x80, TRPM_INVALID_HANDLER);
    }

    /*
     * Check for monitor changes and apply them.
     */
    if (    GCPtrTss        != pVM->selm.s.GCPtrGuestTss
        ||  cbMonitoredTss  != pVM->selm.s.cbMonitoredGuestTss)
    {
        Log(("SELMR3SyncTSS: Guest's TSS is changed to pTss=%RGv cbMonitoredTss=%08X cbGuestTss=%#08x\n",
             GCPtrTss, cbMonitoredTss, pVM->selm.s.cbGuestTss));

        /* Release the old range first. */
        if (pVM->selm.s.GCPtrGuestTss != RTRCPTR_MAX)
        {
            rc = PGMHandlerVirtualDeregister(pVM, pVCpu, pVM->selm.s.GCPtrGuestTss, false /*fHypervisor*/);
            AssertRC(rc);
        }

        /* Register the write handler if TS != 0. */
        if (cbMonitoredTss != 0)
        {
# ifdef SELM_TRACK_GUEST_TSS_CHANGES
            rc = PGMR3HandlerVirtualRegister(pVM, pVCpu, pVM->selm.s.hGuestTssWriteHandlerType,
                                             GCPtrTss, GCPtrTss + cbMonitoredTss - 1,
                                             NULL /*pvUserR3*/, NIL_RTR0PTR /*pvUserRC*/, NULL /*pszDesc*/);
            if (RT_FAILURE(rc))
            {
#  ifdef VBOX_WITH_RAW_RING1
                /** @todo !HACK ALERT!
                 * Some guest OSes (QNX) share code and the TSS on the same page;
                 * PGMR3HandlerVirtualRegister doesn't support more than one
                 * handler, so we kick out the PATM handler as this one is more
                 * important. Fix this properly in PGMR3HandlerVirtualRegister?
                 */
                if (rc == VERR_PGM_HANDLER_VIRTUAL_CONFLICT)
                {
                    LogRel(("SELMR3SyncTSS: Virtual handler conflict %RGv -> kick out PATM handler for the higher priority TSS page monitor\n", GCPtrTss));
                    rc = PGMHandlerVirtualDeregister(pVM, pVCpu, GCPtrTss & PAGE_BASE_GC_MASK, false /*fHypervisor*/);
                    AssertRC(rc);

                    rc = PGMR3HandlerVirtualRegister(pVM, pVCpu, pVM->selm.s.hGuestTssWriteHandlerType,
                                                     GCPtrTss, GCPtrTss + cbMonitoredTss - 1,
                                                     NULL /*pvUserR3*/, NIL_RTR0PTR /*pvUserRC*/, NULL /*pszDesc*/);
                    if (RT_FAILURE(rc))
                    {
                        STAM_PROFILE_STOP(&pVM->selm.s.StatUpdateFromCPUM, a);
                        return rc;
                    }
                }
#  else
                STAM_PROFILE_STOP(&pVM->selm.s.StatUpdateFromCPUM, a);
                return rc;
#  endif
           }
# endif /* SELM_TRACK_GUEST_TSS_CHANGES */

            /* Update saved Guest TSS info. */
            pVM->selm.s.GCPtrGuestTss       = GCPtrTss;
            pVM->selm.s.cbMonitoredGuestTss = cbMonitoredTss;
            pVM->selm.s.GCSelTss            = SelTss;
        }
        else
        {
            pVM->selm.s.GCPtrGuestTss       = RTRCPTR_MAX;
            pVM->selm.s.cbMonitoredGuestTss = 0;
            pVM->selm.s.GCSelTss            = 0;
        }
    }

    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_SELM_SYNC_TSS);

    STAM_PROFILE_STOP(&pVM->selm.s.StatTSSSync, a);
    return VINF_SUCCESS;
}


/**
 * Compares the Guest GDT and LDT with the shadow tables.
 * This is a VBOX_STRICT only function.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3DECL(int) SELMR3DebugCheck(PVM pVM)
{
# ifdef VBOX_STRICT
    PVMCPU pVCpu = VMMGetCpu(pVM);
    AssertReturn(!HMIsEnabled(pVM), VERR_SELM_HM_IPE);

    /*
     * Get GDTR and check for conflict.
     */
    VBOXGDTR  GDTR;
    CPUMGetGuestGDTR(pVCpu, &GDTR);
    if (GDTR.cbGdt == 0)
        return VINF_SUCCESS;

    if (GDTR.cbGdt >= (unsigned)(pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS_TRAP08] >> X86_SEL_SHIFT))
        Log(("SELMR3DebugCheck: guest GDT size forced us to look for unused selectors.\n"));

    if (GDTR.cbGdt != pVM->selm.s.GuestGdtr.cbGdt)
        Log(("SELMR3DebugCheck: limits have changed! new=%d old=%d\n", GDTR.cbGdt, pVM->selm.s.GuestGdtr.cbGdt));

    /*
     * Loop thru the GDT checking each entry.
     */
    RTGCPTR     GCPtrGDTEGuest = GDTR.pGdt;
    PX86DESC    pGDTE = pVM->selm.s.paGdtR3;
    PX86DESC    pGDTEEnd = (PX86DESC)((uintptr_t)pGDTE + GDTR.cbGdt);
    while (pGDTE < pGDTEEnd)
    {
        X86DESC    GDTEGuest;
        int rc = PGMPhysSimpleReadGCPtr(pVCpu, &GDTEGuest, GCPtrGDTEGuest, sizeof(GDTEGuest));
        if (RT_SUCCESS(rc))
        {
            if (pGDTE->Gen.u1DescType || pGDTE->Gen.u4Type != X86_SEL_TYPE_SYS_LDT)
            {
                if (   pGDTE->Gen.u16LimitLow != GDTEGuest.Gen.u16LimitLow
                    || pGDTE->Gen.u4LimitHigh != GDTEGuest.Gen.u4LimitHigh
                    || pGDTE->Gen.u16BaseLow  != GDTEGuest.Gen.u16BaseLow
                    || pGDTE->Gen.u8BaseHigh1 != GDTEGuest.Gen.u8BaseHigh1
                    || pGDTE->Gen.u8BaseHigh2 != GDTEGuest.Gen.u8BaseHigh2
                    || pGDTE->Gen.u1DefBig    != GDTEGuest.Gen.u1DefBig
                    || pGDTE->Gen.u1DescType  != GDTEGuest.Gen.u1DescType)
                {
                    unsigned iGDT = pGDTE - pVM->selm.s.paGdtR3;
                    SELMR3DumpDescriptor(*pGDTE, iGDT << 3, "SELMR3DebugCheck: GDT mismatch, shadow");
                    SELMR3DumpDescriptor(GDTEGuest, iGDT << 3, "SELMR3DebugCheck: GDT mismatch,  guest");
                }
            }
        }

        /* Advance to the next descriptor. */
        GCPtrGDTEGuest += sizeof(X86DESC);
        pGDTE++;
    }


    /*
     * LDT?
     */
    RTSEL SelLdt = CPUMGetGuestLDTR(pVCpu);
    if ((SelLdt & X86_SEL_MASK_OFF_RPL) == 0)
        return VINF_SUCCESS;
    Assert(!(SelLdt & X86_SEL_LDT));
    if (SelLdt > GDTR.cbGdt)
    {
        Log(("SELMR3DebugCheck: ldt is out of bound SelLdt=%#x\n", SelLdt));
        return VERR_SELM_LDT_OUT_OF_BOUNDS;
    }
    X86DESC    LDTDesc;
    int rc = PGMPhysSimpleReadGCPtr(pVCpu, &LDTDesc, GDTR.pGdt + (SelLdt & X86_SEL_MASK), sizeof(LDTDesc));
    if (RT_FAILURE(rc))
    {
        Log(("SELMR3DebugCheck: Failed to read LDT descriptor. rc=%d\n", rc));
        return rc;
    }
    RTGCPTR     GCPtrLDTEGuest = X86DESC_BASE(&LDTDesc);
    uint32_t    cbLdt = X86DESC_LIMIT_G(&LDTDesc);

    /*
     * Validate it.
     */
    if (!cbLdt)
        return VINF_SUCCESS;
    /** @todo check what intel does about odd limits. */
    AssertMsg(RT_ALIGN(cbLdt + 1, sizeof(X86DESC)) == cbLdt + 1 && cbLdt <= 0xffff, ("cbLdt=%d\n", cbLdt));
    if (    LDTDesc.Gen.u1DescType
        ||  LDTDesc.Gen.u4Type != X86_SEL_TYPE_SYS_LDT
        ||  SelLdt >= pVM->selm.s.GuestGdtr.cbGdt)
    {
        Log(("SELmR3DebugCheck: Invalid LDT %04x!\n", SelLdt));
        return VERR_SELM_INVALID_LDT;
    }

    /*
     * Loop thru the LDT checking each entry.
     */
    unsigned    off = (GCPtrLDTEGuest & PAGE_OFFSET_MASK);
    PX86DESC    pLDTE = (PX86DESC)((uintptr_t)pVM->selm.s.pvLdtR3 + off);
    PX86DESC    pLDTEEnd = (PX86DESC)((uintptr_t)pGDTE + cbLdt);
    while (pLDTE < pLDTEEnd)
    {
        X86DESC    LDTEGuest;
        rc = PGMPhysSimpleReadGCPtr(pVCpu, &LDTEGuest, GCPtrLDTEGuest, sizeof(LDTEGuest));
        if (RT_SUCCESS(rc))
        {
            if (   pLDTE->Gen.u16LimitLow != LDTEGuest.Gen.u16LimitLow
                || pLDTE->Gen.u4LimitHigh != LDTEGuest.Gen.u4LimitHigh
                || pLDTE->Gen.u16BaseLow  != LDTEGuest.Gen.u16BaseLow
                || pLDTE->Gen.u8BaseHigh1 != LDTEGuest.Gen.u8BaseHigh1
                || pLDTE->Gen.u8BaseHigh2 != LDTEGuest.Gen.u8BaseHigh2
                || pLDTE->Gen.u1DefBig    != LDTEGuest.Gen.u1DefBig
                || pLDTE->Gen.u1DescType  != LDTEGuest.Gen.u1DescType)
            {
                unsigned iLDT = pLDTE - (PX86DESC)((uintptr_t)pVM->selm.s.pvLdtR3 + off);
                SELMR3DumpDescriptor(*pLDTE, iLDT << 3, "SELMR3DebugCheck: LDT mismatch, shadow");
                SELMR3DumpDescriptor(LDTEGuest, iLDT << 3, "SELMR3DebugCheck: LDT mismatch,  guest");
            }
        }

        /* Advance to the next descriptor. */
        GCPtrLDTEGuest += sizeof(X86DESC);
        pLDTE++;
    }

# else  /* !VBOX_STRICT */
    NOREF(pVM);
# endif /* !VBOX_STRICT */

    return VINF_SUCCESS;
}


/**
 * Validates the RawR0 TSS values against the one in the Guest TSS.
 *
 * @returns true if it matches.
 * @returns false and assertions on mismatch..
 * @param   pVM     The cross context VM structure.
 */
VMMR3DECL(bool) SELMR3CheckTSS(PVM pVM)
{
# if defined(VBOX_STRICT) && defined(SELM_TRACK_GUEST_TSS_CHANGES)
    PVMCPU pVCpu = VMMGetCpu(pVM);

    if (VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_SELM_SYNC_TSS))
        return true;

    /*
     * Get TR and extract the basic info.
     */
    CPUMSELREGHID   trHid;
    RTSEL           SelTss   = CPUMGetGuestTR(pVCpu, &trHid);
    RTGCPTR         GCPtrTss = trHid.u64Base;
    uint32_t        cbTss    = trHid.u32Limit;
    Assert(   (SelTss & X86_SEL_MASK_OFF_RPL)
           || (cbTss == 0 && GCPtrTss == 0 && trHid.Attr.u == 0 /* TR=0 */)
           || (cbTss == 0xffff && GCPtrTss == 0 && trHid.Attr.n.u1Present && trHid.Attr.n.u4Type == X86_SEL_TYPE_SYS_386_TSS_BUSY /* RESET */));
    if (SelTss & X86_SEL_MASK_OFF_RPL)
    {
        AssertReturn(!(SelTss & X86_SEL_LDT), false);
        AssertReturn(trHid.Attr.n.u1DescType == 0, false);
        AssertReturn(   trHid.Attr.n.u4Type == X86_SEL_TYPE_SYS_286_TSS_BUSY
                     || trHid.Attr.n.u4Type == X86_SEL_TYPE_SYS_386_TSS_BUSY,
                     false);
        if (!++cbTss)
            cbTss = UINT32_MAX;
    }
    else
    {
        AssertReturn(   (cbTss == 0 && GCPtrTss == 0 && trHid.Attr.u == 0 /* TR=0 */)
                     || (cbTss == 0xffff && GCPtrTss == 0 && trHid.Attr.n.u1Present && trHid.Attr.n.u4Type == X86_SEL_TYPE_SYS_386_TSS_BUSY /* RESET */),
                     false);
        cbTss = 0; /* the reset case. */
    }
    AssertMsgReturn(pVM->selm.s.cbGuestTss == cbTss, ("%#x %#x\n", pVM->selm.s.cbGuestTss, cbTss), false);
    AssertMsgReturn(pVM->selm.s.fGuestTss32Bit == (   trHid.Attr.n.u4Type == X86_SEL_TYPE_SYS_386_TSS_AVAIL
                                                   || trHid.Attr.n.u4Type == X86_SEL_TYPE_SYS_386_TSS_BUSY),
                    ("%RTbool u4Type=%d\n", pVM->selm.s.fGuestTss32Bit, trHid.Attr.n.u4Type),
                    false);
    AssertMsgReturn(    pVM->selm.s.GCSelTss == SelTss
                    ||  (!pVM->selm.s.GCSelTss && !(SelTss & X86_SEL_LDT)),
                    ("%#x %#x\n", pVM->selm.s.GCSelTss, SelTss),
                    false);
    AssertMsgReturn(    pVM->selm.s.GCPtrGuestTss == GCPtrTss
                    ||  (pVM->selm.s.GCPtrGuestTss == RTRCPTR_MAX && !GCPtrTss),
                    ("%#RGv %#RGv\n", pVM->selm.s.GCPtrGuestTss, GCPtrTss),
                    false);


    /*
     * Figure out the size of what need to monitor.
     */
    /* We're not interested in any 16-bit TSSes. */
    uint32_t cbMonitoredTss = cbTss;
    if (    trHid.Attr.n.u4Type != X86_SEL_TYPE_SYS_386_TSS_AVAIL
        &&  trHid.Attr.n.u4Type != X86_SEL_TYPE_SYS_386_TSS_BUSY)
        cbMonitoredTss = 0;
    if (cbMonitoredTss)
    {
        VBOXTSS Tss;
        uint32_t cr4 = CPUMGetGuestCR4(pVCpu);
        int rc = PGMPhysSimpleReadGCPtr(pVCpu, &Tss, GCPtrTss, RT_OFFSETOF(VBOXTSS, IntRedirBitmap));
        AssertReturn(   rc == VINF_SUCCESS
                        /* Happens early in XP boot during page table switching. */
                     || (   (rc == VERR_PAGE_TABLE_NOT_PRESENT || rc == VERR_PAGE_NOT_PRESENT)
                         && !(CPUMGetGuestEFlags(pVCpu) & X86_EFL_IF)),
                     false);
        if (    !(cr4 & X86_CR4_VME)
            ||   (  RT_SUCCESS(rc)
                 && Tss.offIoBitmap < sizeof(VBOXTSS) /* too small */
                 && Tss.offIoBitmap > cbTss)
           )
            cbMonitoredTss = RT_UOFFSETOF(VBOXTSS, padding_ss0);
        else if (RT_SUCCESS(rc))
        {
            cbMonitoredTss = Tss.offIoBitmap;
            AssertMsgReturn(pVM->selm.s.offGuestIoBitmap == Tss.offIoBitmap,
                            ("%#x %#x\n", pVM->selm.s.offGuestIoBitmap, Tss.offIoBitmap),
                            false);

            /* check the bitmap */
            uint32_t offRedirBitmap = Tss.offIoBitmap - sizeof(Tss.IntRedirBitmap);
            rc = PGMPhysSimpleReadGCPtr(pVCpu, &Tss.IntRedirBitmap,
                                        GCPtrTss + offRedirBitmap, sizeof(Tss.IntRedirBitmap));
            AssertRCReturn(rc, false);
            AssertMsgReturn(!memcmp(&Tss.IntRedirBitmap[0], &pVM->selm.s.Tss.IntRedirBitmap[0], sizeof(Tss.IntRedirBitmap)),
                            ("offIoBitmap=%#x cbTss=%#x\n"
                             " Guest: %.32Rhxs\n"
                             "Shadow: %.32Rhxs\n",
                             Tss.offIoBitmap, cbTss,
                             &Tss.IntRedirBitmap[0],
                             &pVM->selm.s.Tss.IntRedirBitmap[0]),
                            false);
        }
        else
            cbMonitoredTss = RT_OFFSETOF(VBOXTSS, IntRedirBitmap);

        /*
         * Check SS0 and ESP0.
         */
        if (    !pVM->selm.s.fSyncTSSRing0Stack
            &&  RT_SUCCESS(rc))
        {
            if (    Tss.esp0 != pVM->selm.s.Tss.esp1
                ||  Tss.ss0  != (pVM->selm.s.Tss.ss1 & ~1))
            {
                RTGCPHYS GCPhys;
                rc = PGMGstGetPage(pVCpu, GCPtrTss, NULL, &GCPhys); AssertRC(rc);
                AssertMsgFailed(("TSS out of sync!! (%04X:%08X vs %04X:%08X (guest)) Tss=%RGv Phys=%RGp\n",
                                 (pVM->selm.s.Tss.ss1 & ~1), pVM->selm.s.Tss.esp1,
                                 Tss.ss1, Tss.esp1, GCPtrTss, GCPhys));
                return false;
            }
        }
        AssertMsgReturn(pVM->selm.s.cbMonitoredGuestTss == cbMonitoredTss, ("%#x %#x\n", pVM->selm.s.cbMonitoredGuestTss, cbMonitoredTss), false);
    }
    else
    {
        AssertMsgReturn(pVM->selm.s.Tss.ss1 == 0 && pVM->selm.s.Tss.esp1 == 0, ("%04x:%08x\n", pVM->selm.s.Tss.ss1, pVM->selm.s.Tss.esp1), false);
        AssertReturn(!pVM->selm.s.fSyncTSSRing0Stack, false);
        AssertMsgReturn(pVM->selm.s.cbMonitoredGuestTss == cbMonitoredTss, ("%#x %#x\n", pVM->selm.s.cbMonitoredGuestTss, cbMonitoredTss), false);
    }



    return true;

# else  /* !VBOX_STRICT */
    NOREF(pVM);
    return true;
# endif /* !VBOX_STRICT */
}


# ifdef VBOX_WITH_SAFE_STR
/**
 * Validates the RawR0 TR shadow GDT entry.
 *
 * @returns true if it matches.
 * @returns false and assertions on mismatch..
 * @param   pVM     The cross context VM structure.
 */
VMMR3DECL(bool) SELMR3CheckShadowTR(PVM pVM)
{
#  ifdef VBOX_STRICT
    PX86DESC paGdt = pVM->selm.s.paGdtR3;

    /*
     * TSS descriptor
     */
    PX86DESC pDesc = &paGdt[pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS] >> 3];
    RTRCPTR RCPtrTSS = VM_RC_ADDR(pVM, &pVM->selm.s.Tss);

    if (    pDesc->Gen.u16BaseLow      != RT_LOWORD(RCPtrTSS)
        ||  pDesc->Gen.u8BaseHigh1     != RT_BYTE3(RCPtrTSS)
        ||  pDesc->Gen.u8BaseHigh2     != RT_BYTE4(RCPtrTSS)
        ||  pDesc->Gen.u16LimitLow     != sizeof(VBOXTSS) - 1
        ||  pDesc->Gen.u4LimitHigh     != 0
        ||  (pDesc->Gen.u4Type         != X86_SEL_TYPE_SYS_386_TSS_AVAIL && pDesc->Gen.u4Type != X86_SEL_TYPE_SYS_386_TSS_BUSY)
        ||  pDesc->Gen.u1DescType      != 0 /* system */
        ||  pDesc->Gen.u2Dpl           != 0 /* supervisor */
        ||  pDesc->Gen.u1Present       != 1
        ||  pDesc->Gen.u1Available     != 0
        ||  pDesc->Gen.u1Long          != 0
        ||  pDesc->Gen.u1DefBig        != 0
        ||  pDesc->Gen.u1Granularity   != 0 /* byte limit */
        )
    {
        AssertFailed();
        return false;
    }
#  else
    RT_NOREF_PV(pVM);
#  endif
    return true;
}
# endif /* VBOX_WITH_SAFE_STR */

#endif /* VBOX_WITH_RAW_MODE */

/**
 * Gets information about a 64-bit selector, SELMR3GetSelectorInfo helper.
 *
 * See SELMR3GetSelectorInfo for details.
 *
 * @returns VBox status code, see SELMR3GetSelectorInfo for details.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   Sel         The selector to get info about.
 * @param   pSelInfo    Where to store the information.
 */
static int selmR3GetSelectorInfo64(PVMCPU pVCpu, RTSEL Sel, PDBGFSELINFO pSelInfo)
{
    /*
     * Read it from the guest descriptor table.
     */
/** @todo this is bogus wrt the LDT/GDT limit on long selectors. */
    X86DESC64   Desc;
    RTGCPTR     GCPtrDesc;
    if (!(Sel & X86_SEL_LDT))
    {
        /* GDT */
        VBOXGDTR Gdtr;
        CPUMGetGuestGDTR(pVCpu, &Gdtr);
        if ((Sel | X86_SEL_RPL_LDT) > Gdtr.cbGdt)
            return VERR_INVALID_SELECTOR;
        GCPtrDesc = Gdtr.pGdt + (Sel & X86_SEL_MASK);
    }
    else
    {
        /* LDT */
        uint64_t GCPtrBase;
        uint32_t cbLimit;
        CPUMGetGuestLdtrEx(pVCpu, &GCPtrBase, &cbLimit);
        if ((Sel | X86_SEL_RPL_LDT) > cbLimit)
            return VERR_INVALID_SELECTOR;

        /* calc the descriptor location. */
        GCPtrDesc = GCPtrBase + (Sel & X86_SEL_MASK);
    }

    /* read the descriptor. */
    int rc = PGMPhysSimpleReadGCPtr(pVCpu, &Desc, GCPtrDesc, sizeof(Desc));
    if (RT_FAILURE(rc))
    {
        rc = PGMPhysSimpleReadGCPtr(pVCpu, &Desc, GCPtrDesc, sizeof(X86DESC));
        if (RT_FAILURE(rc))
            return rc;
        Desc.au64[1] = 0;
    }

    /*
     * Extract the base and limit
     * (We ignore the present bit here, which is probably a bit silly...)
     */
    pSelInfo->Sel     = Sel;
    pSelInfo->fFlags  = DBGFSELINFO_FLAGS_LONG_MODE;
    pSelInfo->u.Raw64 = Desc;
    if (Desc.Gen.u1DescType)
    {
        /*
         * 64-bit code selectors are wide open, it's not possible to detect
         * 64-bit data or stack selectors without also dragging in assumptions
         * about current CS (i.e. that's we're executing in 64-bit mode).  So,
         * the selinfo user needs to deal with this in the context the info is
         * used unfortunately.
         */
        if (    Desc.Gen.u1Long
            &&  !Desc.Gen.u1DefBig
            &&  (Desc.Gen.u4Type & X86_SEL_TYPE_CODE))
        {
            /* Note! We ignore the segment limit hacks that was added by AMD. */
            pSelInfo->GCPtrBase = 0;
            pSelInfo->cbLimit   = ~(RTGCUINTPTR)0;
        }
        else
        {
            pSelInfo->cbLimit   = X86DESC_LIMIT_G(&Desc);
            pSelInfo->GCPtrBase = X86DESC_BASE(&Desc);
        }
        pSelInfo->SelGate = 0;
    }
    else if (   Desc.Gen.u4Type == AMD64_SEL_TYPE_SYS_LDT
             || Desc.Gen.u4Type == AMD64_SEL_TYPE_SYS_TSS_AVAIL
             || Desc.Gen.u4Type == AMD64_SEL_TYPE_SYS_TSS_BUSY)
    {
        /* Note. LDT descriptors are weird in long mode, we ignore the footnote
           in the AMD manual here as a simplification. */
        pSelInfo->GCPtrBase = X86DESC64_BASE(&Desc);
        pSelInfo->cbLimit   = X86DESC_LIMIT_G(&Desc);
        pSelInfo->SelGate   = 0;
    }
    else if (   Desc.Gen.u4Type == AMD64_SEL_TYPE_SYS_CALL_GATE
             || Desc.Gen.u4Type == AMD64_SEL_TYPE_SYS_TRAP_GATE
             || Desc.Gen.u4Type == AMD64_SEL_TYPE_SYS_INT_GATE)
    {
        pSelInfo->cbLimit   = X86DESC64_BASE(&Desc);
        pSelInfo->GCPtrBase = Desc.Gate.u16OffsetLow
                            | ((uint32_t)Desc.Gate.u16OffsetHigh << 16)
                            | ((uint64_t)Desc.Gate.u32OffsetTop << 32);
        pSelInfo->SelGate   = Desc.Gate.u16Sel;
        pSelInfo->fFlags   |= DBGFSELINFO_FLAGS_GATE;
    }
    else
    {
        pSelInfo->cbLimit   = 0;
        pSelInfo->GCPtrBase = 0;
        pSelInfo->SelGate   = 0;
        pSelInfo->fFlags   |= DBGFSELINFO_FLAGS_INVALID;
    }
    if (!Desc.Gen.u1Present)
        pSelInfo->fFlags |= DBGFSELINFO_FLAGS_NOT_PRESENT;

    return VINF_SUCCESS;
}


/**
 * Worker for selmR3GetSelectorInfo32 and SELMR3GetShadowSelectorInfo that
 * interprets a legacy descriptor table entry and fills in the selector info
 * structure from it.
 *
 * @param  pSelInfo     Where to store the selector info. Only the fFlags and
 *                      Sel members have been initialized.
 * @param  pDesc        The legacy descriptor to parse.
 */
DECLINLINE(void) selmR3SelInfoFromDesc32(PDBGFSELINFO pSelInfo, PCX86DESC pDesc)
{
    pSelInfo->u.Raw64.au64[1] = 0;
    pSelInfo->u.Raw = *pDesc;
    if (    pDesc->Gen.u1DescType
        ||  !(pDesc->Gen.u4Type & 4))
    {
        pSelInfo->cbLimit   = X86DESC_LIMIT_G(pDesc);
        pSelInfo->GCPtrBase = X86DESC_BASE(pDesc);
        pSelInfo->SelGate   = 0;
    }
    else if (pDesc->Gen.u4Type != X86_SEL_TYPE_SYS_UNDEFINED4)
    {
        pSelInfo->cbLimit = 0;
        if (pDesc->Gen.u4Type == X86_SEL_TYPE_SYS_TASK_GATE)
            pSelInfo->GCPtrBase = 0;
        else
            pSelInfo->GCPtrBase = pDesc->Gate.u16OffsetLow
                                | (uint32_t)pDesc->Gate.u16OffsetHigh << 16;
        pSelInfo->SelGate = pDesc->Gate.u16Sel;
        pSelInfo->fFlags |= DBGFSELINFO_FLAGS_GATE;
    }
    else
    {
        pSelInfo->cbLimit = 0;
        pSelInfo->GCPtrBase = 0;
        pSelInfo->SelGate = 0;
        pSelInfo->fFlags |= DBGFSELINFO_FLAGS_INVALID;
    }
    if (!pDesc->Gen.u1Present)
        pSelInfo->fFlags |= DBGFSELINFO_FLAGS_NOT_PRESENT;
}


/**
 * Gets information about a 64-bit selector, SELMR3GetSelectorInfo helper.
 *
 * See SELMR3GetSelectorInfo for details.
 *
 * @returns VBox status code, see SELMR3GetSelectorInfo for details.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   Sel         The selector to get info about.
 * @param   pSelInfo    Where to store the information.
 */
static int selmR3GetSelectorInfo32(PVM pVM, PVMCPU pVCpu, RTSEL Sel, PDBGFSELINFO pSelInfo)
{
    /*
     * Read the descriptor entry
     */
    pSelInfo->fFlags = 0;
    X86DESC Desc;
    if (    !(Sel & X86_SEL_LDT)
        && (    pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS]         == (Sel & X86_SEL_RPL_LDT)
            ||  pVM->selm.s.aHyperSel[SELM_HYPER_SEL_DS]         == (Sel & X86_SEL_RPL_LDT)
            ||  pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS64]       == (Sel & X86_SEL_RPL_LDT)
            ||  pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS]        == (Sel & X86_SEL_RPL_LDT)
            ||  pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS_TRAP08] == (Sel & X86_SEL_RPL_LDT))
       )
    {
        /*
         * Hypervisor descriptor.
         */
        pSelInfo->fFlags = DBGFSELINFO_FLAGS_HYPER;
        if (CPUMIsGuestInProtectedMode(pVCpu))
            pSelInfo->fFlags |= DBGFSELINFO_FLAGS_PROT_MODE;
        else
            pSelInfo->fFlags |= DBGFSELINFO_FLAGS_REAL_MODE;

        Desc = pVM->selm.s.paGdtR3[Sel >> X86_SEL_SHIFT];
    }
    else if (CPUMIsGuestInProtectedMode(pVCpu))
    {
        /*
         * Read it from the guest descriptor table.
         */
        pSelInfo->fFlags = DBGFSELINFO_FLAGS_PROT_MODE;

        RTGCPTR     GCPtrDesc;
        if (!(Sel & X86_SEL_LDT))
        {
            /* GDT */
            VBOXGDTR Gdtr;
            CPUMGetGuestGDTR(pVCpu, &Gdtr);
            if ((Sel | X86_SEL_RPL_LDT) > Gdtr.cbGdt)
                return VERR_INVALID_SELECTOR;
            GCPtrDesc = Gdtr.pGdt + (Sel & X86_SEL_MASK);
        }
        else
        {
            /* LDT */
            uint64_t GCPtrBase;
            uint32_t cbLimit;
            CPUMGetGuestLdtrEx(pVCpu, &GCPtrBase, &cbLimit);
            if ((Sel | X86_SEL_RPL_LDT) > cbLimit)
                return VERR_INVALID_SELECTOR;

            /* calc the descriptor location. */
            GCPtrDesc = GCPtrBase + (Sel & X86_SEL_MASK);
        }

        /* read the descriptor. */
        int rc = PGMPhysSimpleReadGCPtr(pVCpu, &Desc, GCPtrDesc, sizeof(Desc));
        if (RT_FAILURE(rc))
            return rc;
    }
    else
    {
        /*
         * We're in real mode.
         */
        pSelInfo->Sel       = Sel;
        pSelInfo->GCPtrBase = Sel << 4;
        pSelInfo->cbLimit   = 0xffff;
        pSelInfo->fFlags    = DBGFSELINFO_FLAGS_REAL_MODE;
        pSelInfo->u.Raw64.au64[0] = 0;
        pSelInfo->u.Raw64.au64[1] = 0;
        pSelInfo->SelGate   = 0;
        return VINF_SUCCESS;
    }

    /*
     * Extract the base and limit or sel:offset for gates.
     */
    pSelInfo->Sel = Sel;
    selmR3SelInfoFromDesc32(pSelInfo, &Desc);

    return VINF_SUCCESS;
}


/**
 * Gets information about a selector.
 *
 * Intended for the debugger mostly and will prefer the guest descriptor tables
 * over the shadow ones.
 *
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_INVALID_SELECTOR if the selector isn't fully inside the
 *          descriptor table.
 * @retval  VERR_SELECTOR_NOT_PRESENT if the LDT is invalid or not present. This
 *          is not returned if the selector itself isn't present, you have to
 *          check that for yourself (see DBGFSELINFO::fFlags).
 * @retval  VERR_PAGE_TABLE_NOT_PRESENT or VERR_PAGE_NOT_PRESENT if the
 *          pagetable or page backing the selector table wasn't present.
 * @returns Other VBox status code on other errors.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   Sel         The selector to get info about.
 * @param   pSelInfo    Where to store the information.
 */
VMMR3DECL(int) SELMR3GetSelectorInfo(PVM pVM, PVMCPU pVCpu, RTSEL Sel, PDBGFSELINFO pSelInfo)
{
    AssertPtr(pSelInfo);
    if (CPUMIsGuestInLongMode(pVCpu))
        return selmR3GetSelectorInfo64(pVCpu, Sel, pSelInfo);
    return selmR3GetSelectorInfo32(pVM, pVCpu, Sel, pSelInfo);
}


/**
 * Gets information about a selector from the shadow tables.
 *
 * This is intended to be faster than the SELMR3GetSelectorInfo() method, but
 * requires that the caller ensures that the shadow tables are up to date.
 *
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_INVALID_SELECTOR if the selector isn't fully inside the
 *          descriptor table.
 * @retval  VERR_SELECTOR_NOT_PRESENT if the LDT is invalid or not present. This
 *          is not returned if the selector itself isn't present, you have to
 *          check that for yourself (see DBGFSELINFO::fFlags).
 * @retval  VERR_PAGE_TABLE_NOT_PRESENT or VERR_PAGE_NOT_PRESENT if the
 *          pagetable or page backing the selector table wasn't present.
 * @returns Other VBox status code on other errors.
 *
 * @param   pVM         The cross context VM structure.
 * @param   Sel         The selector to get info about.
 * @param   pSelInfo    Where to store the information.
 *
 * @remarks Don't use this when in hardware assisted virtualization mode.
 */
VMMR3DECL(int) SELMR3GetShadowSelectorInfo(PVM pVM, RTSEL Sel, PDBGFSELINFO pSelInfo)
{
    Assert(pSelInfo);

    /*
     * Read the descriptor entry
     */
    X86DESC Desc;
    if (!(Sel & X86_SEL_LDT))
    {
        /*
         * Global descriptor.
         */
        Desc = pVM->selm.s.paGdtR3[Sel >> X86_SEL_SHIFT];
        pSelInfo->fFlags =    pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS]         == (Sel & X86_SEL_MASK_OFF_RPL)
                           || pVM->selm.s.aHyperSel[SELM_HYPER_SEL_DS]         == (Sel & X86_SEL_MASK_OFF_RPL)
                           || pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS64]       == (Sel & X86_SEL_MASK_OFF_RPL)
                           || pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS]        == (Sel & X86_SEL_MASK_OFF_RPL)
                           || pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS_TRAP08] == (Sel & X86_SEL_MASK_OFF_RPL)
                         ? DBGFSELINFO_FLAGS_HYPER
                         : 0;
        /** @todo check that the GDT offset is valid. */
    }
    else
    {
        /*
         * Local Descriptor.
         */
        PX86DESC paLDT = (PX86DESC)((char *)pVM->selm.s.pvLdtR3 + pVM->selm.s.offLdtHyper);
        Desc = paLDT[Sel >> X86_SEL_SHIFT];
        /** @todo check if the LDT page is actually available. */
        /** @todo check that the LDT offset is valid. */
        pSelInfo->fFlags = 0;
    }
    if (CPUMIsGuestInProtectedMode(VMMGetCpu0(pVM)))
        pSelInfo->fFlags |= DBGFSELINFO_FLAGS_PROT_MODE;
    else
        pSelInfo->fFlags |= DBGFSELINFO_FLAGS_REAL_MODE;

    /*
     * Extract the base and limit or sel:offset for gates.
     */
    pSelInfo->Sel = Sel;
    selmR3SelInfoFromDesc32(pSelInfo, &Desc);

    return VINF_SUCCESS;
}


/**
 * Formats a descriptor.
 *
 * @param   Desc        Descriptor to format.
 * @param   Sel         Selector number.
 * @param   pszOutput   Output buffer.
 * @param   cchOutput   Size of output buffer.
 */
static void selmR3FormatDescriptor(X86DESC Desc, RTSEL Sel, char *pszOutput, size_t cchOutput)
{
    /*
     * Make variable description string.
     */
    static struct
    {
        unsigned    cch;
        const char *psz;
    } const aTypes[32] =
    {
#define STRENTRY(str) { sizeof(str) - 1, str }
        /* system */
        STRENTRY("Reserved0 "),                  /* 0x00 */
        STRENTRY("TSS16Avail "),                 /* 0x01 */
        STRENTRY("LDT "),                        /* 0x02 */
        STRENTRY("TSS16Busy "),                  /* 0x03 */
        STRENTRY("Call16 "),                     /* 0x04 */
        STRENTRY("Task "),                       /* 0x05 */
        STRENTRY("Int16 "),                      /* 0x06 */
        STRENTRY("Trap16 "),                     /* 0x07 */
        STRENTRY("Reserved8 "),                  /* 0x08 */
        STRENTRY("TSS32Avail "),                 /* 0x09 */
        STRENTRY("ReservedA "),                  /* 0x0a */
        STRENTRY("TSS32Busy "),                  /* 0x0b */
        STRENTRY("Call32 "),                     /* 0x0c */
        STRENTRY("ReservedD "),                  /* 0x0d */
        STRENTRY("Int32 "),                      /* 0x0e */
        STRENTRY("Trap32 "),                     /* 0x0f */
        /* non system */
        STRENTRY("DataRO "),                     /* 0x10 */
        STRENTRY("DataRO Accessed "),            /* 0x11 */
        STRENTRY("DataRW "),                     /* 0x12 */
        STRENTRY("DataRW Accessed "),            /* 0x13 */
        STRENTRY("DataDownRO "),                 /* 0x14 */
        STRENTRY("DataDownRO Accessed "),        /* 0x15 */
        STRENTRY("DataDownRW "),                 /* 0x16 */
        STRENTRY("DataDownRW Accessed "),        /* 0x17 */
        STRENTRY("CodeEO "),                     /* 0x18 */
        STRENTRY("CodeEO Accessed "),            /* 0x19 */
        STRENTRY("CodeER "),                     /* 0x1a */
        STRENTRY("CodeER Accessed "),            /* 0x1b */
        STRENTRY("CodeConfEO "),                 /* 0x1c */
        STRENTRY("CodeConfEO Accessed "),        /* 0x1d */
        STRENTRY("CodeConfER "),                 /* 0x1e */
        STRENTRY("CodeConfER Accessed ")         /* 0x1f */
#undef SYSENTRY
    };
#define ADD_STR(psz, pszAdd) do { strcpy(psz, pszAdd); psz += strlen(pszAdd); } while (0)
    char        szMsg[128];
    char       *psz = &szMsg[0];
    unsigned    i = Desc.Gen.u1DescType << 4 | Desc.Gen.u4Type;
    memcpy(psz, aTypes[i].psz, aTypes[i].cch);
    psz += aTypes[i].cch;

    if (Desc.Gen.u1Present)
        ADD_STR(psz, "Present ");
    else
        ADD_STR(psz, "Not-Present ");
    if (Desc.Gen.u1Granularity)
        ADD_STR(psz, "Page ");
    if (Desc.Gen.u1DefBig)
        ADD_STR(psz, "32-bit ");
    else
        ADD_STR(psz, "16-bit ");
#undef ADD_STR
    *psz = '\0';

    /*
     * Limit and Base and format the output.
     */
    uint32_t    u32Limit = X86DESC_LIMIT_G(&Desc);
    uint32_t    u32Base  = X86DESC_BASE(&Desc);

    RTStrPrintf(pszOutput, cchOutput, "%04x - %08x %08x - base=%08x limit=%08x dpl=%d %s",
                Sel, Desc.au32[0], Desc.au32[1], u32Base, u32Limit, Desc.Gen.u2Dpl, szMsg);
}


/**
 * Dumps a descriptor.
 *
 * @param   Desc    Descriptor to dump.
 * @param   Sel     Selector number.
 * @param   pszMsg  Message to prepend the log entry with.
 */
VMMR3DECL(void) SELMR3DumpDescriptor(X86DESC Desc, RTSEL Sel, const char *pszMsg)
{
#ifdef LOG_ENABLED
    if (LogIsEnabled())
    {
        char szOutput[128];
        selmR3FormatDescriptor(Desc, Sel, &szOutput[0], sizeof(szOutput));
        Log(("%s: %s\n", pszMsg, szOutput));
    }
#else
    RT_NOREF3(Desc, Sel, pszMsg);
#endif
}


/**
 * Display the shadow gdt.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pHlp        The info helpers.
 * @param   pszArgs     Arguments, ignored.
 */
static DECLCALLBACK(void) selmR3InfoGdt(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    NOREF(pszArgs);
    pHlp->pfnPrintf(pHlp, "Shadow GDT (GCAddr=%RRv):\n", MMHyperR3ToRC(pVM, pVM->selm.s.paGdtR3));
    for (unsigned iGDT = 0; iGDT < SELM_GDT_ELEMENTS; iGDT++)
    {
        if (pVM->selm.s.paGdtR3[iGDT].Gen.u1Present)
        {
            char szOutput[128];
            selmR3FormatDescriptor(pVM->selm.s.paGdtR3[iGDT], iGDT << X86_SEL_SHIFT, &szOutput[0], sizeof(szOutput));
            const char *psz = "";
            if (iGDT == ((unsigned)pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS] >> X86_SEL_SHIFT))
                psz = " HyperCS";
            else if (iGDT == ((unsigned)pVM->selm.s.aHyperSel[SELM_HYPER_SEL_DS] >> X86_SEL_SHIFT))
                psz = " HyperDS";
            else if (iGDT == ((unsigned)pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS64] >> X86_SEL_SHIFT))
                psz = " HyperCS64";
            else if (iGDT == ((unsigned)pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS] >> X86_SEL_SHIFT))
                psz = " HyperTSS";
            else if (iGDT == ((unsigned)pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS_TRAP08] >> X86_SEL_SHIFT))
                psz = " HyperTSSTrap08";
            pHlp->pfnPrintf(pHlp, "%s%s\n", szOutput, psz);
        }
    }
}


/**
 * Display the guest gdt.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pHlp        The info helpers.
 * @param   pszArgs     Arguments, ignored.
 */
static DECLCALLBACK(void) selmR3InfoGdtGuest(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    /** @todo SMP support! */
    PVMCPU      pVCpu = &pVM->aCpus[0];

    VBOXGDTR    GDTR;
    CPUMGetGuestGDTR(pVCpu, &GDTR);
    RTGCPTR     GCPtrGDT = GDTR.pGdt;
    unsigned    cGDTs = ((unsigned)GDTR.cbGdt + 1) / sizeof(X86DESC);

    pHlp->pfnPrintf(pHlp, "Guest GDT (GCAddr=%RGv limit=%x):\n", GCPtrGDT, GDTR.cbGdt);
    for (unsigned iGDT = 0; iGDT < cGDTs; iGDT++, GCPtrGDT += sizeof(X86DESC))
    {
        X86DESC GDTE;
        int rc = PGMPhysSimpleReadGCPtr(pVCpu, &GDTE, GCPtrGDT, sizeof(GDTE));
        if (RT_SUCCESS(rc))
        {
            if (GDTE.Gen.u1Present)
            {
                char szOutput[128];
                selmR3FormatDescriptor(GDTE, iGDT << X86_SEL_SHIFT, &szOutput[0], sizeof(szOutput));
                pHlp->pfnPrintf(pHlp, "%s\n", szOutput);
            }
        }
        else if (rc == VERR_PAGE_NOT_PRESENT)
        {
            if ((GCPtrGDT & PAGE_OFFSET_MASK) + sizeof(X86DESC) - 1 < sizeof(X86DESC))
                pHlp->pfnPrintf(pHlp, "%04x - page not present (GCAddr=%RGv)\n", iGDT << X86_SEL_SHIFT, GCPtrGDT);
        }
        else
            pHlp->pfnPrintf(pHlp, "%04x - read error rc=%Rrc GCAddr=%RGv\n", iGDT << X86_SEL_SHIFT, rc, GCPtrGDT);
    }
    NOREF(pszArgs);
}


/**
 * Display the shadow ldt.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pHlp        The info helpers.
 * @param   pszArgs     Arguments, ignored.
 */
static DECLCALLBACK(void) selmR3InfoLdt(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    unsigned    cLDTs = ((unsigned)pVM->selm.s.cbLdtLimit + 1) >> X86_SEL_SHIFT;
    PX86DESC    paLDT = (PX86DESC)((char *)pVM->selm.s.pvLdtR3 + pVM->selm.s.offLdtHyper);
    pHlp->pfnPrintf(pHlp, "Shadow LDT (GCAddr=%RRv limit=%#x):\n", pVM->selm.s.pvLdtRC + pVM->selm.s.offLdtHyper, pVM->selm.s.cbLdtLimit);
    for (unsigned iLDT = 0; iLDT < cLDTs; iLDT++)
    {
        if (paLDT[iLDT].Gen.u1Present)
        {
            char szOutput[128];
            selmR3FormatDescriptor(paLDT[iLDT], (iLDT << X86_SEL_SHIFT) | X86_SEL_LDT, &szOutput[0], sizeof(szOutput));
            pHlp->pfnPrintf(pHlp, "%s\n", szOutput);
        }
    }
    NOREF(pszArgs);
}


/**
 * Display the guest ldt.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pHlp        The info helpers.
 * @param   pszArgs     Arguments, ignored.
 */
static DECLCALLBACK(void) selmR3InfoLdtGuest(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    /** @todo SMP support! */
    PVMCPU   pVCpu = &pVM->aCpus[0];

    uint64_t GCPtrLdt;
    uint32_t cbLdt;
    RTSEL    SelLdt = CPUMGetGuestLdtrEx(pVCpu, &GCPtrLdt, &cbLdt);
    if (!(SelLdt & X86_SEL_MASK_OFF_RPL))
    {
        pHlp->pfnPrintf(pHlp, "Guest LDT (Sel=%x): Null-Selector\n", SelLdt);
        return;
    }

    pHlp->pfnPrintf(pHlp, "Guest LDT (Sel=%x GCAddr=%RX64 limit=%x):\n", SelLdt, GCPtrLdt, cbLdt);
    unsigned cLdts  = (cbLdt + 1) >> X86_SEL_SHIFT;
    for (unsigned iLdt = 0; iLdt < cLdts; iLdt++, GCPtrLdt += sizeof(X86DESC))
    {
        X86DESC LdtE;
        int rc = PGMPhysSimpleReadGCPtr(pVCpu, &LdtE, GCPtrLdt, sizeof(LdtE));
        if (RT_SUCCESS(rc))
        {
            if (LdtE.Gen.u1Present)
            {
                char szOutput[128];
                selmR3FormatDescriptor(LdtE, (iLdt << X86_SEL_SHIFT) | X86_SEL_LDT, &szOutput[0], sizeof(szOutput));
                pHlp->pfnPrintf(pHlp, "%s\n", szOutput);
            }
        }
        else if (rc == VERR_PAGE_NOT_PRESENT)
        {
            if ((GCPtrLdt & PAGE_OFFSET_MASK) + sizeof(X86DESC) - 1 < sizeof(X86DESC))
                pHlp->pfnPrintf(pHlp, "%04x - page not present (GCAddr=%RGv)\n", (iLdt << X86_SEL_SHIFT) | X86_SEL_LDT, GCPtrLdt);
        }
        else
            pHlp->pfnPrintf(pHlp, "%04x - read error rc=%Rrc GCAddr=%RGv\n", (iLdt << X86_SEL_SHIFT) | X86_SEL_LDT, rc, GCPtrLdt);
    }
    NOREF(pszArgs);
}


/**
 * Dumps the hypervisor GDT
 *
 * @param   pVM     The cross context VM structure.
 */
VMMR3DECL(void) SELMR3DumpHyperGDT(PVM pVM)
{
    DBGFR3Info(pVM->pUVM, "gdt", NULL, NULL);
}


/**
 * Dumps the hypervisor LDT
 *
 * @param   pVM     The cross context VM structure.
 */
VMMR3DECL(void) SELMR3DumpHyperLDT(PVM pVM)
{
    DBGFR3Info(pVM->pUVM, "ldt", NULL, NULL);
}


/**
 * Dumps the guest GDT
 *
 * @param   pVM     The cross context VM structure.
 */
VMMR3DECL(void) SELMR3DumpGuestGDT(PVM pVM)
{
    DBGFR3Info(pVM->pUVM, "gdtguest", NULL, NULL);
}


/**
 * Dumps the guest LDT
 *
 * @param   pVM     The cross context VM structure.
 */
VMMR3DECL(void) SELMR3DumpGuestLDT(PVM pVM)
{
    DBGFR3Info(pVM->pUVM, "ldtguest", NULL, NULL);
}

