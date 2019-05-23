/* $Id: VBoxRecompiler.c $ */
/** @file
 * VBox Recompiler - QEMU.
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

/** @page pg_rem REM - Recompiled Execution Manager.
 *
 * The recompiled exeuction manager (REM) serves the final fallback for guest
 * execution, after HM / raw-mode and IEM have given up.
 *
 * The REM is qemu with a whole bunch of VBox specific customization for
 * interfacing with PATM, CSAM, PGM and other components.
 *
 * @sa @ref grp_rem
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_REM
#include <stdio.h>      /* FILE */
#include "osdep.h"
#include "config.h"
#include "cpu.h"
#include "exec-all.h"
#include "ioport.h"

#include <VBox/vmm/rem.h>
#include <VBox/vmm/vmapi.h>
#include <VBox/vmm/tm.h>
#include <VBox/vmm/ssm.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/iem.h>
#include <VBox/vmm/trpm.h>
#include <VBox/vmm/iom.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/pdm.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/dbg.h>
#include <VBox/vmm/apic.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/patm.h>
#include <VBox/vmm/csam.h>
#include "REMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/uvm.h>
#include <VBox/param.h>
#include <VBox/err.h>

#include <VBox/log.h>
#include <iprt/alloca.h>
#include <iprt/semaphore.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/thread.h>
#include <iprt/string.h>

/* Don't wanna include everything. */
extern void cpu_exec_init_all(uintptr_t tb_size);
extern void cpu_x86_update_cr3(CPUX86State *env, target_ulong new_cr3);
extern void cpu_x86_update_cr0(CPUX86State *env, uint32_t new_cr0);
extern void cpu_x86_update_cr4(CPUX86State *env, uint32_t new_cr4);
extern void tlb_flush_page(CPUX86State *env, target_ulong addr);
extern void tlb_flush(CPUX86State *env, int flush_global);
extern void sync_seg(CPUX86State *env1, int seg_reg, int selector);
extern void sync_ldtr(CPUX86State *env1, int selector);

#ifdef VBOX_STRICT
ram_addr_t get_phys_page_offset(target_ulong addr);
#endif


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/

/** Copy 80-bit fpu register at pSrc to pDst.
 * This is probably faster than *calling* memcpy.
 */
#define REM_COPY_FPU_REG(pDst, pSrc) \
    do { *(PX86FPUMMX)(pDst) = *(const X86FPUMMX *)(pSrc); } while (0)

/** How remR3RunLoggingStep operates. */
#define REM_USE_QEMU_SINGLE_STEP_FOR_LOGGING


/** Selector flag shift between qemu and VBox.
 * VBox shifts the qemu bits to the right. */
#define SEL_FLAGS_SHIFT     (8)
/** Mask applied to the shifted qemu selector flags to get the attributes VBox
 * (VT-x) needs. */
#define SEL_FLAGS_SMASK     UINT32_C(0x1F0FF)


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static DECLCALLBACK(int) remR3Save(PVM pVM, PSSMHANDLE pSSM);
static DECLCALLBACK(int) remR3Load(PVM pVM, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass);
static DECLCALLBACK(int) remR3LoadDone(PVM pVM, PSSMHANDLE pSSM);
static void     remR3StateUpdate(PVM pVM, PVMCPU pVCpu);
static int      remR3InitPhysRamSizeAndDirtyMap(PVM pVM, bool fGuarded);

static uint32_t remR3MMIOReadU8(void *pvEnv, target_phys_addr_t GCPhys);
static uint32_t remR3MMIOReadU16(void *pvEnv, target_phys_addr_t GCPhys);
static uint32_t remR3MMIOReadU32(void *pvEnv, target_phys_addr_t GCPhys);
static void     remR3MMIOWriteU8(void *pvEnv, target_phys_addr_t GCPhys, uint32_t u32);
static void     remR3MMIOWriteU16(void *pvEnv, target_phys_addr_t GCPhys, uint32_t u32);
static void     remR3MMIOWriteU32(void *pvEnv, target_phys_addr_t GCPhys, uint32_t u32);

static uint32_t remR3HandlerReadU8(void *pvVM, target_phys_addr_t GCPhys);
static uint32_t remR3HandlerReadU16(void *pvVM, target_phys_addr_t GCPhys);
static uint32_t remR3HandlerReadU32(void *pvVM, target_phys_addr_t GCPhys);
static void     remR3HandlerWriteU8(void *pvVM, target_phys_addr_t GCPhys, uint32_t u32);
static void     remR3HandlerWriteU16(void *pvVM, target_phys_addr_t GCPhys, uint32_t u32);
static void     remR3HandlerWriteU32(void *pvVM, target_phys_addr_t GCPhys, uint32_t u32);

static void remR3NotifyHandlerPhysicalDeregister(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb, bool fHasHCHandler, bool fRestoreAsRAM);
static void remR3NotifyHandlerPhysicalRegister(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb, bool fHasHCHandler);
static void remR3NotifyHandlerPhysicalModify(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhysOld, RTGCPHYS GCPhysNew, RTGCPHYS cb, bool fHasHCHandler, bool fRestoreAsRAM);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/

/** @todo Move stats to REM::s some rainy day we have nothing do to. */
#ifdef VBOX_WITH_STATISTICS
static STAMPROFILEADV gStatExecuteSingleInstr;
static STAMPROFILEADV gStatCompilationQEmu;
static STAMPROFILEADV gStatRunCodeQEmu;
static STAMPROFILEADV gStatTotalTimeQEmu;
static STAMPROFILEADV gStatTimers;
static STAMPROFILEADV gStatTBLookup;
static STAMPROFILEADV gStatIRQ;
static STAMPROFILEADV gStatRawCheck;
static STAMPROFILEADV gStatMemRead;
static STAMPROFILEADV gStatMemWrite;
static STAMPROFILE    gStatGCPhys2HCVirt;
static STAMCOUNTER    gStatCpuGetTSC;
static STAMCOUNTER    gStatRefuseTFInhibit;
static STAMCOUNTER    gStatRefuseVM86;
static STAMCOUNTER    gStatRefusePaging;
static STAMCOUNTER    gStatRefusePAE;
static STAMCOUNTER    gStatRefuseIOPLNot0;
static STAMCOUNTER    gStatRefuseIF0;
static STAMCOUNTER    gStatRefuseCode16;
static STAMCOUNTER    gStatRefuseWP0;
static STAMCOUNTER    gStatRefuseRing1or2;
static STAMCOUNTER    gStatRefuseCanExecute;
static STAMCOUNTER    gaStatRefuseStale[6];
static STAMCOUNTER    gStatREMGDTChange;
static STAMCOUNTER    gStatREMIDTChange;
static STAMCOUNTER    gStatREMLDTRChange;
static STAMCOUNTER    gStatREMTRChange;
static STAMCOUNTER    gStatSelOutOfSync[6];
static STAMCOUNTER    gStatSelOutOfSyncStateBack[6];
static STAMCOUNTER    gStatFlushTBs;
#endif
/* in exec.c */
extern uint32_t       tlb_flush_count;
extern uint32_t       tb_flush_count;
extern uint32_t       tb_phys_invalidate_count;

/*
 * Global stuff.
 */

/** MMIO read callbacks. */
CPUReadMemoryFunc  *g_apfnMMIORead[3] =
{
    remR3MMIOReadU8,
    remR3MMIOReadU16,
    remR3MMIOReadU32
};

/** MMIO write callbacks. */
CPUWriteMemoryFunc *g_apfnMMIOWrite[3] =
{
    remR3MMIOWriteU8,
    remR3MMIOWriteU16,
    remR3MMIOWriteU32
};

/** Handler read callbacks. */
CPUReadMemoryFunc  *g_apfnHandlerRead[3] =
{
    remR3HandlerReadU8,
    remR3HandlerReadU16,
    remR3HandlerReadU32
};

/** Handler write callbacks. */
CPUWriteMemoryFunc *g_apfnHandlerWrite[3] =
{
    remR3HandlerWriteU8,
    remR3HandlerWriteU16,
    remR3HandlerWriteU32
};


#ifdef VBOX_WITH_DEBUGGER
/*
 * Debugger commands.
 */
static FNDBGCCMD remR3CmdDisasEnableStepping;;

/** '.remstep' arguments. */
static const DBGCVARDESC    g_aArgRemStep[] =
{
    /* cTimesMin,   cTimesMax,  enmCategory,            fFlags,                         pszName,        pszDescription */
    {  0,           ~0U,        DBGCVAR_CAT_NUMBER,     0,                              "on/off",       "Boolean value/mnemonic indicating the new state." },
};

/** Command descriptors. */
static const DBGCCMD    g_aCmds[] =
{
    {
        .pszCmd ="remstep",
        .cArgsMin = 0,
        .cArgsMax = 1,
        .paArgDescs = &g_aArgRemStep[0],
        .cArgDescs = RT_ELEMENTS(g_aArgRemStep),
        .fFlags = 0,
        .pfnHandler = remR3CmdDisasEnableStepping,
        .pszSyntax = "[on/off]",
        .pszDescription = "Enable or disable the single stepping with logged disassembly. "
                          "If no arguments show the current state."
    }
};
#endif

/** Prologue code, must be in lower 4G to simplify jumps to/from generated code.
 * @todo huh??? That cannot be the case on the mac... So, this
 *       point is probably not valid any longer. */
uint8_t *code_gen_prologue;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
void remAbort(int rc, const char *pszTip);
extern int testmath(void);

/* Put them here to avoid unused variable warning. */
AssertCompile(RT_SIZEOFMEMB(VM, rem.padding) >= RT_SIZEOFMEMB(VM, rem.s));
#if !defined(IPRT_NO_CRT) && (defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_WINDOWS))
//AssertCompileMemberSize(REM, Env, REM_ENV_SIZE);
/* Why did this have to be identical?? */
AssertCompile(RT_SIZEOFMEMB(REM, Env) <= REM_ENV_SIZE);
#else
AssertCompile(RT_SIZEOFMEMB(REM, Env) <= REM_ENV_SIZE);
#endif


/**
 * Initializes the REM.
 *
 * @returns VBox status code.
 * @param   pVM         The VM to operate on.
 */
REMR3DECL(int) REMR3Init(PVM pVM)
{
    PREMHANDLERNOTIFICATION pCur;
    uint32_t                u32Dummy;
    int                     rc;
    unsigned                i;

#ifdef VBOX_ENABLE_VBOXREM64
    LogRel(("Using 64-bit aware REM\n"));
#endif

    /*
     * Assert sanity.
     */
    AssertReleaseMsg(sizeof(pVM->rem.padding) >= sizeof(pVM->rem.s), ("%#x >= %#x; sizeof(Env)=%#x\n", sizeof(pVM->rem.padding), sizeof(pVM->rem.s), sizeof(pVM->rem.s.Env)));
    AssertReleaseMsg(sizeof(pVM->rem.s.Env) <= REM_ENV_SIZE, ("%#x == %#x\n", sizeof(pVM->rem.s.Env), REM_ENV_SIZE));
    AssertReleaseMsg(!(RT_OFFSETOF(VM, rem) & 31), ("off=%#x\n", RT_OFFSETOF(VM, rem)));
#if 0 /* just an annoyance at the moment. */
#if defined(DEBUG) && !defined(RT_OS_SOLARIS) && !defined(RT_OS_FREEBSD) /// @todo fix the solaris and freebsd math stuff.
    Assert(!testmath());
#endif
#endif

    /*
     * Init some internal data members.
     */
    pVM->rem.s.offVM = RT_OFFSETOF(VM, rem.s);
    pVM->rem.s.Env.pVM = pVM;
#ifdef CPU_RAW_MODE_INIT
    pVM->rem.s.state |= CPU_RAW_MODE_INIT;
#endif

    /*
     * Initialize the REM critical section.
     *
     * Note: This is not a 100% safe solution as updating the internal memory state while another VCPU
     *       is executing code could be dangerous. Taking the REM lock is not an option due to the danger of
     *       deadlocks. (mostly pgm vs rem locking)
     */
    rc = PDMR3CritSectInit(pVM, &pVM->rem.s.CritSectRegister, RT_SRC_POS, "REM-Register");
    AssertRCReturn(rc, rc);

    /* ctx. */
    pVM->rem.s.pCtx = NULL;     /* set when executing code. */
    AssertMsg(MMR3PhysGetRamSize(pVM) == 0, ("Init order has changed! REM depends on notification about ALL physical memory registrations\n"));

    /* ignore all notifications */
    ASMAtomicIncU32(&pVM->rem.s.cIgnoreAll);

    code_gen_prologue = RTMemExecAlloc(_1K);
    AssertLogRelReturn(code_gen_prologue, VERR_NO_MEMORY);

    cpu_exec_init_all(0);

    /*
     * Init the recompiler.
     */
    if (!cpu_x86_init(&pVM->rem.s.Env, "vbox"))
    {
        AssertMsgFailed(("cpu_x86_init failed - impossible!\n"));
        return VERR_GENERAL_FAILURE;
    }
    PVMCPU pVCpu = VMMGetCpu(pVM);
    CPUMGetGuestCpuId(pVCpu,          1, 0, &u32Dummy, &u32Dummy, &pVM->rem.s.Env.cpuid_ext_features, &pVM->rem.s.Env.cpuid_features);
    CPUMGetGuestCpuId(pVCpu, 0x80000001, 0, &u32Dummy, &u32Dummy, &pVM->rem.s.Env.cpuid_ext3_features, &pVM->rem.s.Env.cpuid_ext2_features);

    EMRemLock(pVM);
    cpu_reset(&pVM->rem.s.Env);
    EMRemUnlock(pVM);

    /* allocate code buffer for single instruction emulation. */
    pVM->rem.s.Env.cbCodeBuffer = 4096;
    pVM->rem.s.Env.pvCodeBuffer = RTMemExecAlloc(pVM->rem.s.Env.cbCodeBuffer);
    AssertMsgReturn(pVM->rem.s.Env.pvCodeBuffer, ("Failed to allocate code buffer!\n"), VERR_NO_MEMORY);

    /* Finally, set the cpu_single_env global. */
    cpu_single_env = &pVM->rem.s.Env;

    /* Nothing is pending by default */
    pVM->rem.s.uStateLoadPendingInterrupt = REM_NO_PENDING_IRQ;

    /*
     * Register ram types.
     */
    pVM->rem.s.iMMIOMemType    = cpu_register_io_memory(g_apfnMMIORead, g_apfnMMIOWrite, &pVM->rem.s.Env);
    AssertReleaseMsg(pVM->rem.s.iMMIOMemType >= 0, ("pVM->rem.s.iMMIOMemType=%d\n", pVM->rem.s.iMMIOMemType));
    pVM->rem.s.iHandlerMemType = cpu_register_io_memory(g_apfnHandlerRead, g_apfnHandlerWrite, pVM);
    AssertReleaseMsg(pVM->rem.s.iHandlerMemType >= 0, ("pVM->rem.s.iHandlerMemType=%d\n", pVM->rem.s.iHandlerMemType));
    Log2(("REM: iMMIOMemType=%d iHandlerMemType=%d\n", pVM->rem.s.iMMIOMemType, pVM->rem.s.iHandlerMemType));

    /* stop ignoring. */
    ASMAtomicDecU32(&pVM->rem.s.cIgnoreAll);

    /*
     * Register the saved state data unit.
     */
    rc = SSMR3RegisterInternal(pVM, "rem", 1, REM_SAVED_STATE_VERSION, sizeof(uint32_t) * 10,
                               NULL, NULL, NULL,
                               NULL, remR3Save, NULL,
                               NULL, remR3Load, remR3LoadDone);
    if (RT_FAILURE(rc))
        return rc;

#ifdef VBOX_WITH_DEBUGGER
    /*
     * Debugger commands.
     */
    static bool fRegisteredCmds = false;
    if (!fRegisteredCmds)
    {
        int rc = DBGCRegisterCommands(&g_aCmds[0], RT_ELEMENTS(g_aCmds));
        if (RT_SUCCESS(rc))
            fRegisteredCmds = true;
    }
#endif

#ifdef VBOX_WITH_STATISTICS
    /*
     * Statistics.
     */
    STAM_REG(pVM, &gStatExecuteSingleInstr, STAMTYPE_PROFILE, "/PROF/REM/SingleInstr",STAMUNIT_TICKS_PER_CALL, "Profiling single instruction emulation.");
    STAM_REG(pVM, &gStatCompilationQEmu,    STAMTYPE_PROFILE, "/PROF/REM/Compile",    STAMUNIT_TICKS_PER_CALL, "Profiling QEmu compilation.");
    STAM_REG(pVM, &gStatRunCodeQEmu,        STAMTYPE_PROFILE, "/PROF/REM/Runcode",    STAMUNIT_TICKS_PER_CALL, "Profiling QEmu code execution.");
    STAM_REG(pVM, &gStatTotalTimeQEmu,      STAMTYPE_PROFILE, "/PROF/REM/Emulate",    STAMUNIT_TICKS_PER_CALL, "Profiling code emulation.");
    STAM_REG(pVM, &gStatTimers,             STAMTYPE_PROFILE, "/PROF/REM/Timers",     STAMUNIT_TICKS_PER_CALL, "Profiling timer queue processing.");
    STAM_REG(pVM, &gStatTBLookup,           STAMTYPE_PROFILE, "/PROF/REM/TBLookup",   STAMUNIT_TICKS_PER_CALL, "Profiling translation block lookup.");
    STAM_REG(pVM, &gStatIRQ,                STAMTYPE_PROFILE, "/PROF/REM/IRQ",        STAMUNIT_TICKS_PER_CALL, "Profiling IRQ delivery.");
    STAM_REG(pVM, &gStatRawCheck,           STAMTYPE_PROFILE, "/PROF/REM/RawCheck",   STAMUNIT_TICKS_PER_CALL, "Profiling remR3CanExecuteRaw calls.");
    STAM_REG(pVM, &gStatMemRead,            STAMTYPE_PROFILE, "/PROF/REM/MemRead",    STAMUNIT_TICKS_PER_CALL, "Profiling memory access.");
    STAM_REG(pVM, &gStatMemWrite,           STAMTYPE_PROFILE, "/PROF/REM/MemWrite",   STAMUNIT_TICKS_PER_CALL, "Profiling memory access.");
    STAM_REG(pVM, &gStatGCPhys2HCVirt,      STAMTYPE_PROFILE, "/PROF/REM/GCPhys2HCVirt", STAMUNIT_TICKS_PER_CALL, "Profiling memory conversion (PGMR3PhysTlbGCPhys2Ptr).");

    STAM_REG(pVM, &gStatCpuGetTSC,          STAMTYPE_COUNTER, "/REM/CpuGetTSC",         STAMUNIT_OCCURENCES,     "cpu_get_tsc calls");

    STAM_REG(pVM, &gStatRefuseTFInhibit,    STAMTYPE_COUNTER, "/REM/Refuse/TFInibit", STAMUNIT_OCCURENCES,     "Raw mode refused because of TF or irq inhibit");
    STAM_REG(pVM, &gStatRefuseVM86,         STAMTYPE_COUNTER, "/REM/Refuse/VM86",     STAMUNIT_OCCURENCES,     "Raw mode refused because of VM86");
    STAM_REG(pVM, &gStatRefusePaging,       STAMTYPE_COUNTER, "/REM/Refuse/Paging",   STAMUNIT_OCCURENCES,     "Raw mode refused because of disabled paging/pm");
    STAM_REG(pVM, &gStatRefusePAE,          STAMTYPE_COUNTER, "/REM/Refuse/PAE",      STAMUNIT_OCCURENCES,     "Raw mode refused because of PAE");
    STAM_REG(pVM, &gStatRefuseIOPLNot0,     STAMTYPE_COUNTER, "/REM/Refuse/IOPLNot0", STAMUNIT_OCCURENCES,     "Raw mode refused because of IOPL != 0");
    STAM_REG(pVM, &gStatRefuseIF0,          STAMTYPE_COUNTER, "/REM/Refuse/IF0",      STAMUNIT_OCCURENCES,     "Raw mode refused because of IF=0");
    STAM_REG(pVM, &gStatRefuseCode16,       STAMTYPE_COUNTER, "/REM/Refuse/Code16",   STAMUNIT_OCCURENCES,     "Raw mode refused because of 16 bit code");
    STAM_REG(pVM, &gStatRefuseWP0,          STAMTYPE_COUNTER, "/REM/Refuse/WP0",      STAMUNIT_OCCURENCES,     "Raw mode refused because of WP=0");
    STAM_REG(pVM, &gStatRefuseRing1or2,     STAMTYPE_COUNTER, "/REM/Refuse/Ring1or2", STAMUNIT_OCCURENCES,     "Raw mode refused because of ring 1/2 execution");
    STAM_REG(pVM, &gStatRefuseCanExecute,   STAMTYPE_COUNTER, "/REM/Refuse/CanExecuteRaw", STAMUNIT_OCCURENCES,     "Raw mode refused because of cCanExecuteRaw");
    STAM_REG(pVM, &gaStatRefuseStale[R_ES], STAMTYPE_COUNTER, "/REM/Refuse/StaleES",  STAMUNIT_OCCURENCES,     "Raw mode refused because of stale ES");
    STAM_REG(pVM, &gaStatRefuseStale[R_CS], STAMTYPE_COUNTER, "/REM/Refuse/StaleCS",  STAMUNIT_OCCURENCES,     "Raw mode refused because of stale CS");
    STAM_REG(pVM, &gaStatRefuseStale[R_SS], STAMTYPE_COUNTER, "/REM/Refuse/StaleSS",  STAMUNIT_OCCURENCES,     "Raw mode refused because of stale SS");
    STAM_REG(pVM, &gaStatRefuseStale[R_DS], STAMTYPE_COUNTER, "/REM/Refuse/StaleDS",  STAMUNIT_OCCURENCES,     "Raw mode refused because of stale DS");
    STAM_REG(pVM, &gaStatRefuseStale[R_FS], STAMTYPE_COUNTER, "/REM/Refuse/StaleFS",  STAMUNIT_OCCURENCES,     "Raw mode refused because of stale FS");
    STAM_REG(pVM, &gaStatRefuseStale[R_GS], STAMTYPE_COUNTER, "/REM/Refuse/StaleGS",  STAMUNIT_OCCURENCES,     "Raw mode refused because of stale GS");
    STAM_REG(pVM, &gStatFlushTBs,           STAMTYPE_COUNTER, "/REM/FlushTB",         STAMUNIT_OCCURENCES,     "Number of TB flushes");

    STAM_REG(pVM, &gStatREMGDTChange,       STAMTYPE_COUNTER, "/REM/Change/GDTBase",   STAMUNIT_OCCURENCES,     "GDT base changes");
    STAM_REG(pVM, &gStatREMLDTRChange,      STAMTYPE_COUNTER, "/REM/Change/LDTR",      STAMUNIT_OCCURENCES,     "LDTR changes");
    STAM_REG(pVM, &gStatREMIDTChange,       STAMTYPE_COUNTER, "/REM/Change/IDTBase",   STAMUNIT_OCCURENCES,     "IDT base changes");
    STAM_REG(pVM, &gStatREMTRChange,        STAMTYPE_COUNTER, "/REM/Change/TR",        STAMUNIT_OCCURENCES,     "TR selector changes");

    STAM_REG(pVM, &gStatSelOutOfSync[0],    STAMTYPE_COUNTER, "/REM/State/SelOutOfSync/ES",        STAMUNIT_OCCURENCES,     "ES out of sync");
    STAM_REG(pVM, &gStatSelOutOfSync[1],    STAMTYPE_COUNTER, "/REM/State/SelOutOfSync/CS",        STAMUNIT_OCCURENCES,     "CS out of sync");
    STAM_REG(pVM, &gStatSelOutOfSync[2],    STAMTYPE_COUNTER, "/REM/State/SelOutOfSync/SS",        STAMUNIT_OCCURENCES,     "SS out of sync");
    STAM_REG(pVM, &gStatSelOutOfSync[3],    STAMTYPE_COUNTER, "/REM/State/SelOutOfSync/DS",        STAMUNIT_OCCURENCES,     "DS out of sync");
    STAM_REG(pVM, &gStatSelOutOfSync[4],    STAMTYPE_COUNTER, "/REM/State/SelOutOfSync/FS",        STAMUNIT_OCCURENCES,     "FS out of sync");
    STAM_REG(pVM, &gStatSelOutOfSync[5],    STAMTYPE_COUNTER, "/REM/State/SelOutOfSync/GS",        STAMUNIT_OCCURENCES,     "GS out of sync");

    STAM_REG(pVM, &gStatSelOutOfSyncStateBack[0],   STAMTYPE_COUNTER,   "/REM/StateBack/SelOutOfSync/ES",   STAMUNIT_OCCURENCES, "ES out of sync");
    STAM_REG(pVM, &gStatSelOutOfSyncStateBack[1],   STAMTYPE_COUNTER,   "/REM/StateBack/SelOutOfSync/CS",   STAMUNIT_OCCURENCES, "CS out of sync");
    STAM_REG(pVM, &gStatSelOutOfSyncStateBack[2],   STAMTYPE_COUNTER,   "/REM/StateBack/SelOutOfSync/SS",   STAMUNIT_OCCURENCES, "SS out of sync");
    STAM_REG(pVM, &gStatSelOutOfSyncStateBack[3],   STAMTYPE_COUNTER,   "/REM/StateBack/SelOutOfSync/DS",   STAMUNIT_OCCURENCES, "DS out of sync");
    STAM_REG(pVM, &gStatSelOutOfSyncStateBack[4],   STAMTYPE_COUNTER,   "/REM/StateBack/SelOutOfSync/FS",   STAMUNIT_OCCURENCES, "FS out of sync");
    STAM_REG(pVM, &gStatSelOutOfSyncStateBack[5],   STAMTYPE_COUNTER,   "/REM/StateBack/SelOutOfSync/GS",   STAMUNIT_OCCURENCES, "GS out of sync");

    STAM_REG(pVM, &pVM->rem.s.Env.StatTbFlush,      STAMTYPE_PROFILE,   "/REM/TbFlush",     STAMUNIT_TICKS_PER_CALL, "profiling tb_flush().");
#endif /* VBOX_WITH_STATISTICS */
    AssertCompileMemberAlignment(CPUX86State, StatTbFlush, 4);
    AssertCompileMemberAlignment(CPUX86State, StatTbFlush, 8);

    STAM_REL_REG(pVM, &tb_flush_count,              STAMTYPE_U32_RESET, "/REM/TbFlushCount",                STAMUNIT_OCCURENCES, "tb_flush() calls");
    STAM_REL_REG(pVM, &tb_phys_invalidate_count,    STAMTYPE_U32_RESET, "/REM/TbPhysInvldCount",            STAMUNIT_OCCURENCES, "tb_phys_invalidate() calls");
    STAM_REL_REG(pVM, &tlb_flush_count,             STAMTYPE_U32_RESET, "/REM/TlbFlushCount",               STAMUNIT_OCCURENCES, "tlb_flush() calls");


#ifdef DEBUG_ALL_LOGGING
    loglevel = ~0;
#endif

    /*
     * Init the handler notification lists.
     */
    pVM->rem.s.idxPendingList = UINT32_MAX;
    pVM->rem.s.idxFreeList    = 0;

    for (i = 0 ; i < RT_ELEMENTS(pVM->rem.s.aHandlerNotifications); i++)
    {
        pCur = &pVM->rem.s.aHandlerNotifications[i];
        pCur->idxNext = i + 1;
        pCur->idxSelf = i;
    }
    pCur->idxNext = UINT32_MAX;         /* the last record. */

    return rc;
}


/**
 * Finalizes the REM initialization.
 *
 * This is called after all components, devices and drivers has
 * been initialized. Its main purpose it to finish the RAM related
 * initialization.
 *
 * @returns VBox status code.
 *
 * @param   pVM         The VM handle.
 */
REMR3DECL(int) REMR3InitFinalize(PVM pVM)
{
    int rc;

    /*
     * Ram size & dirty bit map.
     */
    Assert(!pVM->rem.s.fGCPhysLastRamFixed);
    pVM->rem.s.fGCPhysLastRamFixed = true;
#ifdef RT_STRICT
    rc = remR3InitPhysRamSizeAndDirtyMap(pVM, true /* fGuarded */);
#else
    rc = remR3InitPhysRamSizeAndDirtyMap(pVM, false /* fGuarded */);
#endif
    return rc;
}

/**
 * Initializes ram_list.phys_dirty and ram_list.phys_dirty_size.
 *
 * @returns VBox status code.
 * @param   pVM         The VM handle.
 * @param   fGuarded    Whether to guard the map.
 */
static int remR3InitPhysRamSizeAndDirtyMap(PVM pVM, bool fGuarded)
{
    int      rc = VINF_SUCCESS;
    RTGCPHYS cb;

    AssertLogRelReturn(QLIST_EMPTY(&ram_list.blocks), VERR_INTERNAL_ERROR_2);

    cb = pVM->rem.s.GCPhysLastRam + 1;
    AssertLogRelMsgReturn(cb > pVM->rem.s.GCPhysLastRam,
                          ("GCPhysLastRam=%RGp - out of range\n", pVM->rem.s.GCPhysLastRam),
                          VERR_OUT_OF_RANGE);

    ram_list.phys_dirty_size = cb >> PAGE_SHIFT;
    AssertMsg(((RTGCPHYS)ram_list.phys_dirty_size << PAGE_SHIFT) == cb, ("%RGp\n", cb));

    if (!fGuarded)
    {
        ram_list.phys_dirty = MMR3HeapAlloc(pVM, MM_TAG_REM, ram_list.phys_dirty_size);
        AssertLogRelMsgReturn(ram_list.phys_dirty, ("Failed to allocate %u bytes of dirty page map bytes\n", ram_list.phys_dirty_size), VERR_NO_MEMORY);
    }
    else
    {
        /*
         * Fill it up the nearest 4GB RAM and leave at least _64KB of guard after it.
         */
        uint32_t cbBitmapAligned = RT_ALIGN_32(ram_list.phys_dirty_size, PAGE_SIZE);
        uint32_t cbBitmapFull    = RT_ALIGN_32(ram_list.phys_dirty_size, (_4G >> PAGE_SHIFT));
        if (cbBitmapFull == cbBitmapAligned)
            cbBitmapFull += _4G >> PAGE_SHIFT;
        else if (cbBitmapFull - cbBitmapAligned < _64K)
            cbBitmapFull += _64K;

        ram_list.phys_dirty = RTMemPageAlloc(cbBitmapFull);
        AssertLogRelMsgReturn(ram_list.phys_dirty, ("Failed to allocate %u bytes of dirty page map bytes\n", cbBitmapFull), VERR_NO_MEMORY);

        rc = RTMemProtect(ram_list.phys_dirty + cbBitmapAligned, cbBitmapFull - cbBitmapAligned, RTMEM_PROT_NONE);
        if (RT_FAILURE(rc))
        {
            RTMemPageFree(ram_list.phys_dirty, cbBitmapFull);
            AssertLogRelRCReturn(rc, rc);
        }

        ram_list.phys_dirty += cbBitmapAligned - ram_list.phys_dirty_size;
    }

    /* initialize it. */
    memset(ram_list.phys_dirty, 0xff, ram_list.phys_dirty_size);
    return rc;
}


/**
 * Terminates the REM.
 *
 * Termination means cleaning up and freeing all resources,
 * the VM it self is at this point powered off or suspended.
 *
 * @returns VBox status code.
 * @param   pVM         The VM to operate on.
 */
REMR3DECL(int) REMR3Term(PVM pVM)
{
    /*
     * Statistics.
     */
    STAMR3Deregister(pVM->pUVM, "/PROF/REM/*");
    STAMR3Deregister(pVM->pUVM, "/REM/*");

    return VINF_SUCCESS;
}


/**
 * The VM is being reset.
 *
 * For the REM component this means to call the cpu_reset() and
 * reinitialize some state variables.
 *
 * @param   pVM     VM handle.
 */
REMR3DECL(void) REMR3Reset(PVM pVM)
{
    EMRemLock(pVM); /* Only pro forma, we're in a rendezvous. */

    /*
     * Reset the REM cpu.
     */
    Assert(pVM->rem.s.cIgnoreAll == 0);
    ASMAtomicIncU32(&pVM->rem.s.cIgnoreAll);
    cpu_reset(&pVM->rem.s.Env);
    pVM->rem.s.cInvalidatedPages = 0;
    ASMAtomicDecU32(&pVM->rem.s.cIgnoreAll);
    Assert(pVM->rem.s.cIgnoreAll == 0);

    /* Clear raw ring 0 init state */
    pVM->rem.s.Env.state &= ~CPU_RAW_RING0;

    /* Flush the TBs the next time we execute code here. */
    pVM->rem.s.fFlushTBs = true;

    EMRemUnlock(pVM);
}


/**
 * Execute state save operation.
 *
 * @returns VBox status code.
 * @param   pVM             VM Handle.
 * @param   pSSM            SSM operation handle.
 */
static DECLCALLBACK(int) remR3Save(PVM pVM, PSSMHANDLE pSSM)
{
    PREM pRem = &pVM->rem.s;

    /*
     * Save the required CPU Env bits.
     * (Not much because we're never in REM when doing the save.)
     */
    LogFlow(("remR3Save:\n"));
    Assert(!pRem->fInREM);
    SSMR3PutU32(pSSM,   pRem->Env.hflags);
    SSMR3PutU32(pSSM,   ~0);            /* separator */

    /* Remember if we've entered raw mode (vital for ring 1 checks in e.g. iret emulation). */
    SSMR3PutU32(pSSM, !!(pRem->Env.state & CPU_RAW_RING0));
    SSMR3PutU32(pSSM, REM_NO_PENDING_IRQ);

    return SSMR3PutU32(pSSM, ~0);       /* terminator */
}


/**
 * Execute state load operation.
 *
 * @returns VBox status code.
 * @param   pVM             VM Handle.
 * @param   pSSM            SSM operation handle.
 * @param   uVersion        Data layout version.
 * @param   uPass           The data pass.
 */
static DECLCALLBACK(int) remR3Load(PVM pVM, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    uint32_t u32Dummy;
    uint32_t fRawRing0 = false;
    uint32_t u32Sep;
    uint32_t i;
    int rc;
    PREM pRem;

    LogFlow(("remR3Load:\n"));
    Assert(uPass == SSM_PASS_FINAL); NOREF(uPass);

    /*
     * Validate version.
     */
    if (    uVersion != REM_SAVED_STATE_VERSION
        &&  uVersion != REM_SAVED_STATE_VERSION_VER1_6)
    {
        AssertMsgFailed(("remR3Load: Invalid version uVersion=%d!\n", uVersion));
        return VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;
    }

    /*
     * Do a reset to be on the safe side...
     */
    REMR3Reset(pVM);

    /*
     * Ignore all ignorable notifications.
     * (Not doing this will cause serious trouble.)
     */
    ASMAtomicIncU32(&pVM->rem.s.cIgnoreAll);

    /*
     * Load the required CPU Env bits.
     * (Not much because we're never in REM when doing the save.)
     */
    pRem = &pVM->rem.s;
    Assert(!pRem->fInREM);
    SSMR3GetU32(pSSM,   &pRem->Env.hflags);
    if (uVersion == REM_SAVED_STATE_VERSION_VER1_6)
    {
        /* Redundant REM CPU state has to be loaded, but can be ignored. */
        CPUX86State_Ver16 temp;
        SSMR3GetMem(pSSM,   &temp, RT_OFFSETOF(CPUX86State_Ver16, jmp_env));
    }

    rc = SSMR3GetU32(pSSM, &u32Sep);            /* separator */
    if (RT_FAILURE(rc))
        return rc;
    if (u32Sep != ~0U)
    {
        AssertMsgFailed(("u32Sep=%#x\n", u32Sep));
        return VERR_SSM_DATA_UNIT_FORMAT_CHANGED;
    }

    /* Remember if we've entered raw mode (vital for ring 1 checks in e.g. iret emulation). */
    SSMR3GetUInt(pSSM, &fRawRing0);
    if (fRawRing0)
        pRem->Env.state |= CPU_RAW_RING0;

    if (uVersion == REM_SAVED_STATE_VERSION_VER1_6)
    {
        /*
         * Load the REM stuff.
         */
        /** @todo r=bird: We should just drop all these items, restoring doesn't make
         *        sense. */
        rc = SSMR3GetU32(pSSM, (uint32_t *)&pRem->cInvalidatedPages);
        if (RT_FAILURE(rc))
            return rc;
        if (pRem->cInvalidatedPages > RT_ELEMENTS(pRem->aGCPtrInvalidatedPages))
        {
            AssertMsgFailed(("cInvalidatedPages=%#x\n", pRem->cInvalidatedPages));
            return VERR_SSM_DATA_UNIT_FORMAT_CHANGED;
        }
        for (i = 0; i < pRem->cInvalidatedPages; i++)
            SSMR3GetGCPtr(pSSM, &pRem->aGCPtrInvalidatedPages[i]);
    }

    rc = SSMR3GetUInt(pSSM, &pVM->rem.s.uStateLoadPendingInterrupt);
    AssertRCReturn(rc, rc);
    AssertLogRelMsgReturn(   pVM->rem.s.uStateLoadPendingInterrupt == REM_NO_PENDING_IRQ
                          || pVM->rem.s.uStateLoadPendingInterrupt < 256,
                          ("uStateLoadPendingInterrupt=%#x\n", pVM->rem.s.uStateLoadPendingInterrupt),
                          VERR_SSM_UNEXPECTED_DATA);

    /* check the terminator. */
    rc = SSMR3GetU32(pSSM, &u32Sep);
    if (RT_FAILURE(rc))
        return rc;
    if (u32Sep != ~0U)
    {
        AssertMsgFailed(("u32Sep=%#x (term)\n", u32Sep));
        return VERR_SSM_DATA_UNIT_FORMAT_CHANGED;
    }

    /*
     * Get the CPUID features.
     */
    PVMCPU pVCpu = VMMGetCpu(pVM);
    CPUMGetGuestCpuId(pVCpu,          1, 0, &u32Dummy, &u32Dummy, &pVM->rem.s.Env.cpuid_ext_features, &pVM->rem.s.Env.cpuid_features);
    CPUMGetGuestCpuId(pVCpu, 0x80000001, 0, &u32Dummy, &u32Dummy, &u32Dummy, &pVM->rem.s.Env.cpuid_ext2_features);

    /*
     * Stop ignoring ignorable notifications.
     */
    ASMAtomicDecU32(&pVM->rem.s.cIgnoreAll);

    /*
     * Sync the whole CPU state when executing code in the recompiler.
     */
    for (i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU pVCpu = &pVM->aCpus[i];
        CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_ALL);
    }
    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{FNSSMINTLOADDONE,
 *    For pushing misdesigned pending-interrupt mess to TRPM where it belongs. }
 */
static DECLCALLBACK(int) remR3LoadDone(PVM pVM, PSSMHANDLE pSSM)
{
    if (pVM->rem.s.uStateLoadPendingInterrupt != REM_NO_PENDING_IRQ)
    {
        int rc = TRPMAssertTrap(&pVM->aCpus[0], pVM->rem.s.uStateLoadPendingInterrupt, TRPM_HARDWARE_INT);
        AssertLogRelMsgReturn(rc, ("uStateLoadPendingInterrupt=%#x rc=%Rrc\n", pVM->rem.s.uStateLoadPendingInterrupt, rc), rc);
        pVM->rem.s.uStateLoadPendingInterrupt = REM_NO_PENDING_IRQ;
    }
    return VINF_SUCCESS;
}


#undef LOG_GROUP
#define LOG_GROUP LOG_GROUP_REM_RUN

/**
 * Single steps an instruction in recompiled mode.
 *
 * Before calling this function the REM state needs to be in sync with
 * the VM. Call REMR3State() to perform the sync. It's only necessary
 * (and permitted) to sync at the first call to REMR3Step()/REMR3Run()
 * and after calling REMR3StateBack().
 *
 * @returns VBox status code.
 *
 * @param   pVM         VM Handle.
 * @param   pVCpu       VMCPU Handle.
 */
REMR3DECL(int) REMR3Step(PVM pVM, PVMCPU pVCpu)
{
    int         rc, interrupt_request;
    RTGCPTR     GCPtrPC;
    bool        fBp;

    /*
     * Lock the REM - we don't wanna have anyone interrupting us
     * while stepping - and enabled single stepping. We also ignore
     * pending interrupts and suchlike.
     */
    interrupt_request = pVM->rem.s.Env.interrupt_request;
    Assert(!(interrupt_request & ~(CPU_INTERRUPT_HARD | CPU_INTERRUPT_EXITTB | CPU_INTERRUPT_TIMER | CPU_INTERRUPT_EXTERNAL_HARD | CPU_INTERRUPT_EXTERNAL_EXIT | CPU_INTERRUPT_EXTERNAL_FLUSH_TLB | CPU_INTERRUPT_EXTERNAL_TIMER)));
    pVM->rem.s.Env.interrupt_request = 0;
    cpu_single_step(&pVM->rem.s.Env, 1);

    /*
     * If we're standing at a breakpoint, that have to be disabled before we start stepping.
     */
    GCPtrPC = pVM->rem.s.Env.eip + pVM->rem.s.Env.segs[R_CS].base;
    fBp = !cpu_breakpoint_remove(&pVM->rem.s.Env, GCPtrPC, BP_GDB);

    /*
     * Execute and handle the return code.
     * We execute without enabling the cpu tick, so on success we'll
     * just flip it on and off to make sure it moves
     */
    rc = cpu_exec(&pVM->rem.s.Env);
    if (rc == EXCP_DEBUG)
    {
        TMR3NotifyResume(pVM, pVCpu);
        TMR3NotifySuspend(pVM, pVCpu);
        rc = VINF_EM_DBG_STEPPED;
    }
    else
    {
        switch (rc)
        {
            case EXCP_INTERRUPT:    rc = VINF_SUCCESS; break;
            case EXCP_HLT:
            case EXCP_HALTED:       rc = VINF_EM_HALT; break;
            case EXCP_RC:
                rc = pVM->rem.s.rc;
                pVM->rem.s.rc = VERR_INTERNAL_ERROR;
                break;
            case EXCP_EXECUTE_RAW:
            case EXCP_EXECUTE_HM:
                /** @todo is it correct? No! */
                rc = VINF_SUCCESS;
                break;
            default:
                AssertReleaseMsgFailed(("This really shouldn't happen, rc=%d!\n", rc));
                rc = VERR_INTERNAL_ERROR;
                break;
        }
    }

    /*
     * Restore the stuff we changed to prevent interruption.
     * Unlock the REM.
     */
    if (fBp)
    {
        int rc2 = cpu_breakpoint_insert(&pVM->rem.s.Env, GCPtrPC, BP_GDB, NULL);
        Assert(rc2 == 0); NOREF(rc2);
    }
    cpu_single_step(&pVM->rem.s.Env, 0);
    pVM->rem.s.Env.interrupt_request = interrupt_request;

    return rc;
}


/**
 * Set a breakpoint using the REM facilities.
 *
 * @returns VBox status code.
 * @param   pVM             The VM handle.
 * @param   Address         The breakpoint address.
 * @thread  The emulation thread.
 */
REMR3DECL(int) REMR3BreakpointSet(PVM pVM, RTGCUINTPTR Address)
{
    VM_ASSERT_EMT(pVM);
    if (!cpu_breakpoint_insert(&pVM->rem.s.Env, Address, BP_GDB, NULL))
    {
        LogFlow(("REMR3BreakpointSet: Address=%RGv\n", Address));
        return VINF_SUCCESS;
    }
    LogFlow(("REMR3BreakpointSet: Address=%RGv - failed!\n", Address));
    return VERR_REM_NO_MORE_BP_SLOTS;
}


/**
 * Clears a breakpoint set by REMR3BreakpointSet().
 *
 * @returns VBox status code.
 * @param   pVM             The VM handle.
 * @param   Address         The breakpoint address.
 * @thread  The emulation thread.
 */
REMR3DECL(int) REMR3BreakpointClear(PVM pVM, RTGCUINTPTR Address)
{
    VM_ASSERT_EMT(pVM);
    if (!cpu_breakpoint_remove(&pVM->rem.s.Env, Address, BP_GDB))
    {
        LogFlow(("REMR3BreakpointClear: Address=%RGv\n", Address));
        return VINF_SUCCESS;
    }
    LogFlow(("REMR3BreakpointClear: Address=%RGv - not found!\n", Address));
    return VERR_REM_BP_NOT_FOUND;
}


/**
 * Emulate an instruction.
 *
 * This function executes one instruction without letting anyone
 * interrupt it. This is intended for being called while being in
 * raw mode and thus will take care of all the state syncing between
 * REM and the rest.
 *
 * @returns VBox status code.
 * @param   pVM         VM handle.
 * @param   pVCpu       VMCPU Handle.
 */
REMR3DECL(int) REMR3EmulateInstruction(PVM pVM, PVMCPU pVCpu)
{
    bool fFlushTBs;

    int rc, rc2;
    Log2(("REMR3EmulateInstruction: (cs:eip=%04x:%08x)\n", CPUMGetGuestCS(pVCpu), CPUMGetGuestEIP(pVCpu)));

    /* Make sure this flag is set; we might never execute remR3CanExecuteRaw in the AMD-V case.
     * CPU_RAW_HM makes sure we never execute interrupt handlers in the recompiler.
     */
    if (HMIsEnabled(pVM))
        pVM->rem.s.Env.state |= CPU_RAW_HM;

    /* Skip the TB flush as that's rather expensive and not necessary for single instruction emulation. */
    fFlushTBs = pVM->rem.s.fFlushTBs;
    pVM->rem.s.fFlushTBs = false;

    /*
     * Sync the state and enable single instruction / single stepping.
     */
    rc = REMR3State(pVM, pVCpu);
    pVM->rem.s.fFlushTBs = fFlushTBs;
    if (RT_SUCCESS(rc))
    {
        int interrupt_request = pVM->rem.s.Env.interrupt_request;
        Assert(!(  interrupt_request
                 & ~(CPU_INTERRUPT_HARD | CPU_INTERRUPT_EXITTB | CPU_INTERRUPT_TIMER | CPU_INTERRUPT_EXTERNAL_HARD
                     | CPU_INTERRUPT_EXTERNAL_EXIT | CPU_INTERRUPT_EXTERNAL_FLUSH_TLB | CPU_INTERRUPT_EXTERNAL_TIMER
                     | CPU_INTERRUPT_EXTERNAL_DMA)));
#ifdef REM_USE_QEMU_SINGLE_STEP_FOR_LOGGING
        cpu_single_step(&pVM->rem.s.Env, 0);
#endif
        Assert(!pVM->rem.s.Env.singlestep_enabled);

        /*
         * Now we set the execute single instruction flag and enter the cpu_exec loop.
         */
        TMNotifyStartOfExecution(pVCpu);
        pVM->rem.s.Env.interrupt_request = CPU_INTERRUPT_SINGLE_INSTR;
        rc = cpu_exec(&pVM->rem.s.Env);
        TMNotifyEndOfExecution(pVCpu);
        switch (rc)
        {
            /*
             * Executed without anything out of the way happening.
             */
            case EXCP_SINGLE_INSTR:
                rc = VINF_EM_RESCHEDULE;
                Log2(("REMR3EmulateInstruction: cpu_exec -> EXCP_SINGLE_INSTR\n"));
                break;

            /*
             * If we take a trap or start servicing a pending interrupt, we might end up here.
             * (Timer thread or some other thread wishing EMT's attention.)
             */
            case EXCP_INTERRUPT:
                Log2(("REMR3EmulateInstruction: cpu_exec -> EXCP_INTERRUPT\n"));
                rc = VINF_EM_RESCHEDULE;
                break;

            /*
             * Single step, we assume!
             * If there was a breakpoint there we're fucked now.
             */
            case EXCP_DEBUG:
                if (pVM->rem.s.Env.watchpoint_hit)
                {
                    /** @todo deal with watchpoints */
                    Log2(("REMR3EmulateInstruction: cpu_exec -> EXCP_DEBUG rc=%Rrc !watchpoint_hit!\n", rc));
                    rc = VINF_EM_DBG_BREAKPOINT;
                }
                else
                {
                    CPUBreakpoint  *pBP;
                    RTGCPTR         GCPtrPC = pVM->rem.s.Env.eip + pVM->rem.s.Env.segs[R_CS].base;
                    QTAILQ_FOREACH(pBP, &pVM->rem.s.Env.breakpoints, entry)
                        if (pBP->pc == GCPtrPC)
                            break;
                    rc = pBP ? VINF_EM_DBG_BREAKPOINT : VINF_EM_DBG_STEPPED;
                    Log2(("REMR3EmulateInstruction: cpu_exec -> EXCP_DEBUG rc=%Rrc pBP=%p GCPtrPC=%RGv\n", rc, pBP, GCPtrPC));
                }
                break;

            /*
             * hlt instruction.
             */
            case EXCP_HLT:
                Log2(("REMR3EmulateInstruction: cpu_exec -> EXCP_HLT\n"));
                rc = VINF_EM_HALT;
                break;

            /*
             * The VM has halted.
             */
            case EXCP_HALTED:
                Log2(("REMR3EmulateInstruction: cpu_exec -> EXCP_HALTED\n"));
                rc = VINF_EM_HALT;
                break;

            /*
             * Switch to RAW-mode.
             */
            case EXCP_EXECUTE_RAW:
                Log2(("REMR3EmulateInstruction: cpu_exec -> EXCP_EXECUTE_RAW\n"));
                rc = VINF_EM_RESCHEDULE_RAW;
                break;

            /*
             * Switch to hardware accelerated RAW-mode.
             */
            case EXCP_EXECUTE_HM:
                Log2(("REMR3EmulateInstruction: cpu_exec -> EXCP_EXECUTE_HM\n"));
                rc = VINF_EM_RESCHEDULE_HM;
                break;

            /*
             * An EM RC was raised (VMR3Reset/Suspend/PowerOff/some-fatal-error).
             */
            case EXCP_RC:
                Log2(("REMR3EmulateInstruction: cpu_exec -> EXCP_RC\n"));
                rc = pVM->rem.s.rc;
                pVM->rem.s.rc = VERR_INTERNAL_ERROR;
                break;

            /*
             * Figure out the rest when they arrive....
             */
            default:
                AssertMsgFailed(("rc=%d\n", rc));
                Log2(("REMR3EmulateInstruction: cpu_exec -> %d\n", rc));
                rc = VINF_EM_RESCHEDULE;
                break;
        }

        /*
         * Switch back the state.
         */
        pVM->rem.s.Env.interrupt_request = interrupt_request;
        rc2 = REMR3StateBack(pVM, pVCpu);
        AssertRC(rc2);
    }

    Log2(("REMR3EmulateInstruction: returns %Rrc (cs:eip=%04x:%RGv)\n",
          rc, pVM->rem.s.Env.segs[R_CS].selector, (RTGCPTR)pVM->rem.s.Env.eip));
    return rc;
}


/**
 * Used by REMR3Run to handle the case where CPU_EMULATE_SINGLE_STEP is set.
 *
 * @returns VBox status code.
 *
 * @param   pVM                 The VM handle.
 * @param   pVCpu               The Virtual CPU handle.
 */
static int remR3RunLoggingStep(PVM pVM, PVMCPU pVCpu)
{
    int rc;

    Assert(pVM->rem.s.fInREM);
#ifdef REM_USE_QEMU_SINGLE_STEP_FOR_LOGGING
    cpu_single_step(&pVM->rem.s.Env, 1);
#else
    Assert(!pVM->rem.s.Env.singlestep_enabled);
#endif

    /*
     * Now we set the execute single instruction flag and enter the cpu_exec loop.
     */
    for (;;)
    {
        char szBuf[256];

        /*
         * Log the current registers state and instruction.
         */
        remR3StateUpdate(pVM, pVCpu);
        DBGFR3Info(pVM->pUVM, "cpumguest", NULL, NULL);
        szBuf[0] = '\0';
        rc = DBGFR3DisasInstrEx(pVM->pUVM,
                                pVCpu->idCpu,
                                0, /* Sel */  0, /* GCPtr */
                                DBGF_DISAS_FLAGS_CURRENT_GUEST | DBGF_DISAS_FLAGS_DEFAULT_MODE,
                                szBuf,
                                sizeof(szBuf),
                                NULL);
        if (RT_FAILURE(rc))
            RTStrPrintf(szBuf, sizeof(szBuf), "DBGFR3DisasInstrEx failed with rc=%Rrc\n", rc);
        RTLogPrintf("CPU%d: %s\n", pVCpu->idCpu, szBuf);

        /*
         * Execute the instruction.
         */
        TMNotifyStartOfExecution(pVCpu);

        if (   pVM->rem.s.Env.exception_index < 0
            || pVM->rem.s.Env.exception_index > 256)
            pVM->rem.s.Env.exception_index = -1; /** @todo We need to do similar stuff elsewhere, I think. */

#ifdef REM_USE_QEMU_SINGLE_STEP_FOR_LOGGING
        pVM->rem.s.Env.interrupt_request = 0;
#else
        pVM->rem.s.Env.interrupt_request = CPU_INTERRUPT_SINGLE_INSTR;
#endif
        if (VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_UPDATE_APIC | VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC))
            pVM->rem.s.Env.interrupt_request |= CPU_INTERRUPT_HARD;
        RTLogPrintf("remR3RunLoggingStep: interrupt_request=%#x halted=%d exception_index=%#x\n",
                    pVM->rem.s.Env.interrupt_request,
                    pVM->rem.s.Env.halted,
                    pVM->rem.s.Env.exception_index
                    );

        rc = cpu_exec(&pVM->rem.s.Env);

        RTLogPrintf("remR3RunLoggingStep: cpu_exec -> %#x interrupt_request=%#x halted=%d exception_index=%#x\n", rc,
                    pVM->rem.s.Env.interrupt_request,
                    pVM->rem.s.Env.halted,
                    pVM->rem.s.Env.exception_index
                    );

        TMNotifyEndOfExecution(pVCpu);

        switch (rc)
        {
#ifndef REM_USE_QEMU_SINGLE_STEP_FOR_LOGGING
            /*
             * The normal exit.
             */
            case EXCP_SINGLE_INSTR:
                if (   !VM_FF_IS_PENDING(pVM, VM_FF_ALL_REM_MASK)
                    && !VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_ALL_REM_MASK))
                    continue;
                RTLogPrintf("remR3RunLoggingStep: rc=VINF_SUCCESS w/ FFs (%#x/%#x)\n",
                            pVM->fGlobalForcedActions, pVCpu->fLocalForcedActions);
                rc = VINF_SUCCESS;
                break;

#else
            /*
             * The normal exit, check for breakpoints at PC just to be sure.
             */
#endif
            case EXCP_DEBUG:
                if (pVM->rem.s.Env.watchpoint_hit)
                {
                    /** @todo deal with watchpoints */
                    Log2(("remR3RunLoggingStep: cpu_exec -> EXCP_DEBUG rc=%Rrc !watchpoint_hit!\n", rc));
                    rc = VINF_EM_DBG_BREAKPOINT;
                }
                else
                {
                    CPUBreakpoint  *pBP;
                    RTGCPTR         GCPtrPC = pVM->rem.s.Env.eip + pVM->rem.s.Env.segs[R_CS].base;
                    QTAILQ_FOREACH(pBP, &pVM->rem.s.Env.breakpoints, entry)
                        if (pBP->pc == GCPtrPC)
                            break;
                    rc = pBP ? VINF_EM_DBG_BREAKPOINT : VINF_EM_DBG_STEPPED;
                    Log2(("remR3RunLoggingStep: cpu_exec -> EXCP_DEBUG rc=%Rrc pBP=%p GCPtrPC=%RGv\n", rc, pBP, GCPtrPC));
                }
#ifdef REM_USE_QEMU_SINGLE_STEP_FOR_LOGGING
                if (rc == VINF_EM_DBG_STEPPED)
                {
                    if (   !VM_FF_IS_PENDING(pVM, VM_FF_ALL_REM_MASK)
                        && !VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_ALL_REM_MASK))
                        continue;

                    RTLogPrintf("remR3RunLoggingStep: rc=VINF_SUCCESS w/ FFs (%#x/%#x)\n",
                                pVM->fGlobalForcedActions, pVCpu->fLocalForcedActions);
                    rc = VINF_SUCCESS;
                }
#endif
                break;

            /*
             * If we take a trap or start servicing a pending interrupt, we might end up here.
             * (Timer thread or some other thread wishing EMT's attention.)
             */
            case EXCP_INTERRUPT:
                RTLogPrintf("remR3RunLoggingStep: cpu_exec -> EXCP_INTERRUPT rc=VINF_SUCCESS\n");
                rc = VINF_SUCCESS;
                break;

            /*
             * hlt instruction.
             */
            case EXCP_HLT:
                RTLogPrintf("remR3RunLoggingStep: cpu_exec -> EXCP_HLT rc=VINF_EM_HALT\n");
                rc = VINF_EM_HALT;
                break;

            /*
             * The VM has halted.
             */
            case EXCP_HALTED:
                RTLogPrintf("remR3RunLoggingStep: cpu_exec -> EXCP_HALTED rc=VINF_EM_HALT\n");
                rc = VINF_EM_HALT;
                break;

            /*
             * Switch to RAW-mode.
             */
            case EXCP_EXECUTE_RAW:
                RTLogPrintf("remR3RunLoggingStep: cpu_exec -> EXCP_EXECUTE_RAW rc=VINF_EM_RESCHEDULE_RAW\n");
                rc = VINF_EM_RESCHEDULE_RAW;
                break;

            /*
             * Switch to hardware accelerated RAW-mode.
             */
            case EXCP_EXECUTE_HM:
                RTLogPrintf("remR3RunLoggingStep: cpu_exec -> EXCP_EXECUTE_HM rc=VINF_EM_RESCHEDULE_HM\n");
                rc = VINF_EM_RESCHEDULE_HM;
                break;

            /*
             * An EM RC was raised (VMR3Reset/Suspend/PowerOff/some-fatal-error).
             */
            case EXCP_RC:
                RTLogPrintf("remR3RunLoggingStep: cpu_exec -> EXCP_RC rc=%Rrc\n", pVM->rem.s.rc);
                rc = pVM->rem.s.rc;
                pVM->rem.s.rc = VERR_INTERNAL_ERROR;
                break;

            /*
             * Figure out the rest when they arrive....
             */
            default:
                AssertMsgFailed(("rc=%d\n", rc));
                RTLogPrintf("remR3RunLoggingStep: cpu_exec -> %d rc=VINF_EM_RESCHEDULE\n", rc);
                rc = VINF_EM_RESCHEDULE;
                break;
        }
        break;
    }

#ifdef REM_USE_QEMU_SINGLE_STEP_FOR_LOGGING
//    cpu_single_step(&pVM->rem.s.Env, 0);
#else
    pVM->rem.s.Env.interrupt_request &= ~(CPU_INTERRUPT_SINGLE_INSTR | CPU_INTERRUPT_SINGLE_INSTR_IN_FLIGHT);
#endif
    return rc;
}


/**
 * Runs code in recompiled mode.
 *
 * Before calling this function the REM state needs to be in sync with
 * the VM. Call REMR3State() to perform the sync. It's only necessary
 * (and permitted) to sync at the first call to REMR3Step()/REMR3Run()
 * and after calling REMR3StateBack().
 *
 * @returns VBox status code.
 *
 * @param   pVM         VM Handle.
 * @param   pVCpu       VMCPU Handle.
 */
REMR3DECL(int) REMR3Run(PVM pVM, PVMCPU pVCpu)
{
    int rc;

    if (RT_UNLIKELY(pVM->rem.s.Env.state & CPU_EMULATE_SINGLE_STEP))
        return remR3RunLoggingStep(pVM, pVCpu);

    Assert(pVM->rem.s.fInREM);
    Log2(("REMR3Run: (cs:eip=%04x:%RGv)\n", pVM->rem.s.Env.segs[R_CS].selector, (RTGCPTR)pVM->rem.s.Env.eip));

    TMNotifyStartOfExecution(pVCpu);
    rc = cpu_exec(&pVM->rem.s.Env);
    TMNotifyEndOfExecution(pVCpu);
    switch (rc)
    {
        /*
         * This happens when the execution was interrupted
         * by an external event, like pending timers.
         */
        case EXCP_INTERRUPT:
            Log2(("REMR3Run: cpu_exec -> EXCP_INTERRUPT\n"));
            rc = VINF_SUCCESS;
            break;

        /*
         * hlt instruction.
         */
        case EXCP_HLT:
            Log2(("REMR3Run: cpu_exec -> EXCP_HLT\n"));
            rc = VINF_EM_HALT;
            break;

        /*
         * The VM has halted.
         */
        case EXCP_HALTED:
            Log2(("REMR3Run: cpu_exec -> EXCP_HALTED\n"));
            rc = VINF_EM_HALT;
            break;

        /*
         * Breakpoint/single step.
         */
        case EXCP_DEBUG:
            if (pVM->rem.s.Env.watchpoint_hit)
            {
                /** @todo deal with watchpoints */
                Log2(("REMR3Run: cpu_exec -> EXCP_DEBUG rc=%Rrc !watchpoint_hit!\n", rc));
                rc = VINF_EM_DBG_BREAKPOINT;
            }
            else
            {
                CPUBreakpoint  *pBP;
                RTGCPTR         GCPtrPC = pVM->rem.s.Env.eip + pVM->rem.s.Env.segs[R_CS].base;
                QTAILQ_FOREACH(pBP, &pVM->rem.s.Env.breakpoints, entry)
                    if (pBP->pc == GCPtrPC)
                        break;
                rc = pBP ? VINF_EM_DBG_BREAKPOINT : VINF_EM_DBG_STEPPED;
                Log2(("REMR3Run: cpu_exec -> EXCP_DEBUG rc=%Rrc pBP=%p GCPtrPC=%RGv\n", rc, pBP, GCPtrPC));
            }
            break;

        /*
         * Switch to RAW-mode.
         */
        case EXCP_EXECUTE_RAW:
            Log2(("REMR3Run: cpu_exec -> EXCP_EXECUTE_RAW pc=%RGv\n", pVM->rem.s.Env.eip));
            rc = VINF_EM_RESCHEDULE_RAW;
            break;

        /*
         * Switch to hardware accelerated RAW-mode.
         */
        case EXCP_EXECUTE_HM:
            Log2(("REMR3Run: cpu_exec -> EXCP_EXECUTE_HM\n"));
            rc = VINF_EM_RESCHEDULE_HM;
            break;

        /*
         * An EM RC was raised (VMR3Reset/Suspend/PowerOff/some-fatal-error).
         */
        case EXCP_RC:
            Log2(("REMR3Run: cpu_exec -> EXCP_RC rc=%Rrc\n", pVM->rem.s.rc));
            rc = pVM->rem.s.rc;
            pVM->rem.s.rc = VERR_INTERNAL_ERROR;
            break;

        /*
         * Figure out the rest when they arrive....
         */
        default:
            AssertMsgFailed(("rc=%d\n", rc));
            Log2(("REMR3Run: cpu_exec -> %d\n", rc));
            rc = VINF_SUCCESS;
            break;
    }

    Log2(("REMR3Run: returns %Rrc (cs:eip=%04x:%RGv)\n", rc, pVM->rem.s.Env.segs[R_CS].selector, (RTGCPTR)pVM->rem.s.Env.eip));
    return rc;
}


/**
 * Check if the cpu state is suitable for Raw execution.
 *
 * @returns true if RAW/HWACC mode is ok, false if we should stay in REM.
 *
 * @param   env         The CPU env struct.
 * @param   eip         The EIP to check this for (might differ from env->eip).
 * @param   fFlags      hflags OR'ed with IOPL, TF and VM from eflags.
 * @param   piException Stores EXCP_EXECUTE_RAW/HWACC in case raw mode is supported in this context
 *
 * @remark  This function must be kept in perfect sync with the scheduler in EM.cpp!
 */
bool remR3CanExecuteRaw(CPUX86State *env, RTGCPTR eip, unsigned fFlags, int *piException)
{
    /* !!! THIS MUST BE IN SYNC WITH emR3Reschedule !!! */
    /* !!! THIS MUST BE IN SYNC WITH emR3Reschedule !!! */
    /* !!! THIS MUST BE IN SYNC WITH emR3Reschedule !!! */
    uint32_t u32CR0;

#ifdef IEM_VERIFICATION_MODE
    return false;
#endif

    /* Update counter. */
    env->pVM->rem.s.cCanExecuteRaw++;

    /* Never when single stepping+logging guest code. */
    if (env->state & CPU_EMULATE_SINGLE_STEP)
        return false;

    if (HMIsEnabled(env->pVM))
    {
#ifdef RT_OS_WINDOWS
        PCPUMCTX pCtx = alloca(sizeof(*pCtx));
#else
        CPUMCTX Ctx;
        PCPUMCTX pCtx = &Ctx;
#endif

        env->state |= CPU_RAW_HM;

        /*
         * The simple check first...
         */
        if (!EMIsHwVirtExecutionEnabled(env->pVM))
            return false;

        /*
         * Create partial context for HMR3CanExecuteGuest
         */
        pCtx->cr0            = env->cr[0];
        pCtx->cr3            = env->cr[3];
        pCtx->cr4            = env->cr[4];

        pCtx->tr.Sel         = env->tr.selector;
        pCtx->tr.ValidSel    = env->tr.selector;
        pCtx->tr.fFlags      = CPUMSELREG_FLAGS_VALID;
        pCtx->tr.u64Base     = env->tr.base;
        pCtx->tr.u32Limit    = env->tr.limit;
        pCtx->tr.Attr.u      = (env->tr.flags >> SEL_FLAGS_SHIFT) & SEL_FLAGS_SMASK;

        pCtx->ldtr.Sel       = env->ldt.selector;
        pCtx->ldtr.ValidSel  = env->ldt.selector;
        pCtx->ldtr.fFlags    = CPUMSELREG_FLAGS_VALID;
        pCtx->ldtr.u64Base   = env->ldt.base;
        pCtx->ldtr.u32Limit  = env->ldt.limit;
        pCtx->ldtr.Attr.u    = (env->ldt.flags >> SEL_FLAGS_SHIFT) & SEL_FLAGS_SMASK;

        pCtx->idtr.cbIdt     = env->idt.limit;
        pCtx->idtr.pIdt      = env->idt.base;

        pCtx->gdtr.cbGdt     = env->gdt.limit;
        pCtx->gdtr.pGdt      = env->gdt.base;

        pCtx->rsp            = env->regs[R_ESP];
        pCtx->rip            = env->eip;

        pCtx->eflags.u32     = env->eflags;

        pCtx->cs.Sel         = env->segs[R_CS].selector;
        pCtx->cs.ValidSel    = env->segs[R_CS].selector;
        pCtx->cs.fFlags      = CPUMSELREG_FLAGS_VALID;
        pCtx->cs.u64Base     = env->segs[R_CS].base;
        pCtx->cs.u32Limit    = env->segs[R_CS].limit;
        pCtx->cs.Attr.u      = (env->segs[R_CS].flags >> SEL_FLAGS_SHIFT) & SEL_FLAGS_SMASK;

        pCtx->ds.Sel         = env->segs[R_DS].selector;
        pCtx->ds.ValidSel    = env->segs[R_DS].selector;
        pCtx->ds.fFlags      = CPUMSELREG_FLAGS_VALID;
        pCtx->ds.u64Base     = env->segs[R_DS].base;
        pCtx->ds.u32Limit    = env->segs[R_DS].limit;
        pCtx->ds.Attr.u      = (env->segs[R_DS].flags >> SEL_FLAGS_SHIFT) & SEL_FLAGS_SMASK;

        pCtx->es.Sel         = env->segs[R_ES].selector;
        pCtx->es.ValidSel    = env->segs[R_ES].selector;
        pCtx->es.fFlags      = CPUMSELREG_FLAGS_VALID;
        pCtx->es.u64Base     = env->segs[R_ES].base;
        pCtx->es.u32Limit    = env->segs[R_ES].limit;
        pCtx->es.Attr.u      = (env->segs[R_ES].flags >> SEL_FLAGS_SHIFT) & SEL_FLAGS_SMASK;

        pCtx->fs.Sel         = env->segs[R_FS].selector;
        pCtx->fs.ValidSel    = env->segs[R_FS].selector;
        pCtx->fs.fFlags      = CPUMSELREG_FLAGS_VALID;
        pCtx->fs.u64Base     = env->segs[R_FS].base;
        pCtx->fs.u32Limit    = env->segs[R_FS].limit;
        pCtx->fs.Attr.u      = (env->segs[R_FS].flags >> SEL_FLAGS_SHIFT) & SEL_FLAGS_SMASK;

        pCtx->gs.Sel         = env->segs[R_GS].selector;
        pCtx->gs.ValidSel    = env->segs[R_GS].selector;
        pCtx->gs.fFlags      = CPUMSELREG_FLAGS_VALID;
        pCtx->gs.u64Base     = env->segs[R_GS].base;
        pCtx->gs.u32Limit    = env->segs[R_GS].limit;
        pCtx->gs.Attr.u      = (env->segs[R_GS].flags >> SEL_FLAGS_SHIFT) & SEL_FLAGS_SMASK;

        pCtx->ss.Sel         = env->segs[R_SS].selector;
        pCtx->ss.ValidSel    = env->segs[R_SS].selector;
        pCtx->ss.fFlags      = CPUMSELREG_FLAGS_VALID;
        pCtx->ss.u64Base     = env->segs[R_SS].base;
        pCtx->ss.u32Limit    = env->segs[R_SS].limit;
        pCtx->ss.Attr.u      = (env->segs[R_SS].flags >> SEL_FLAGS_SHIFT) & SEL_FLAGS_SMASK;

        pCtx->msrEFER        = env->efer;

        /* Hardware accelerated raw-mode:
         *
         * Typically only 32-bits protected mode, with paging enabled, code is allowed here.
         */
        if (HMR3CanExecuteGuest(env->pVM, pCtx) == true)
        {
            *piException = EXCP_EXECUTE_HM;
            return true;
        }
        return false;
    }

    /*
     * Here we only support 16 & 32 bits protected mode ring 3 code that has no IO privileges
     * or 32 bits protected mode ring 0 code
     *
     * The tests are ordered by the likelihood of being true during normal execution.
     */
    if (fFlags & (HF_TF_MASK | HF_INHIBIT_IRQ_MASK))
    {
        STAM_COUNTER_INC(&gStatRefuseTFInhibit);
        Log2(("raw mode refused: fFlags=%#x\n", fFlags));
        return false;
    }

#ifndef VBOX_RAW_V86
    if (fFlags & VM_MASK) {
        STAM_COUNTER_INC(&gStatRefuseVM86);
        Log2(("raw mode refused: VM_MASK\n"));
        return false;
    }
#endif

    if (env->state & CPU_EMULATE_SINGLE_INSTR)
    {
#ifndef DEBUG_bird
        Log2(("raw mode refused: CPU_EMULATE_SINGLE_INSTR\n"));
#endif
        return false;
    }

    if (env->singlestep_enabled)
    {
        //Log2(("raw mode refused: Single step\n"));
        return false;
    }

    if (!QTAILQ_EMPTY(&env->breakpoints))
    {
        //Log2(("raw mode refused: Breakpoints\n"));
        return false;
    }

    if (!QTAILQ_EMPTY(&env->watchpoints))
    {
        //Log2(("raw mode refused: Watchpoints\n"));
        return false;
    }

    u32CR0 = env->cr[0];
    if ((u32CR0 & (X86_CR0_PG | X86_CR0_PE)) != (X86_CR0_PG | X86_CR0_PE))
    {
        STAM_COUNTER_INC(&gStatRefusePaging);
        //Log2(("raw mode refused: %s%s%s\n", (u32CR0 & X86_CR0_PG) ? "" : " !PG", (u32CR0 & X86_CR0_PE) ? "" : " !PE", (u32CR0 & X86_CR0_AM) ? "" : " !AM"));
        return false;
    }

    if (env->cr[4] & CR4_PAE_MASK)
    {
        if (!(env->cpuid_features & X86_CPUID_FEATURE_EDX_PAE))
        {
            STAM_COUNTER_INC(&gStatRefusePAE);
            return false;
        }
    }

    if (((fFlags >> HF_CPL_SHIFT) & 3) == 3)
    {
        if (!EMIsRawRing3Enabled(env->pVM))
            return false;

        if (!(env->eflags & IF_MASK))
        {
            STAM_COUNTER_INC(&gStatRefuseIF0);
            Log2(("raw mode refused: IF (RawR3)\n"));
            return false;
        }

        if (!(u32CR0 & CR0_WP_MASK) && EMIsRawRing0Enabled(env->pVM))
        {
            STAM_COUNTER_INC(&gStatRefuseWP0);
            Log2(("raw mode refused: CR0.WP + RawR0\n"));
            return false;
        }
    }
    else
    {
        if (!EMIsRawRing0Enabled(env->pVM))
            return false;

        // Let's start with pure 32 bits ring 0 code first
        if ((fFlags & (HF_SS32_MASK | HF_CS32_MASK)) != (HF_SS32_MASK | HF_CS32_MASK))
        {
            STAM_COUNTER_INC(&gStatRefuseCode16);
            Log2(("raw r0 mode refused: HF_[S|C]S32_MASK fFlags=%#x\n", fFlags));
            return false;
        }

        if (EMIsRawRing1Enabled(env->pVM))
        {
            /* Only ring 0 and 1 supervisor code. */
            if (((fFlags >> HF_CPL_SHIFT) & 3) == 2) /* ring 1 code is moved into ring 2, so we can't support ring-2 in that case. */
            {
                Log2(("raw r0 mode refused: CPL %d\n", (fFlags >> HF_CPL_SHIFT) & 3));
                return false;
            }
        }
        /* Only R0. */
        else if (((fFlags >> HF_CPL_SHIFT) & 3) != 0)
        {
            STAM_COUNTER_INC(&gStatRefuseRing1or2);
            Log2(("raw r0 mode refused: CPL %d\n", ((fFlags >> HF_CPL_SHIFT) & 3) ));
            return false;
        }

        if (!(u32CR0 & CR0_WP_MASK))
        {
            STAM_COUNTER_INC(&gStatRefuseWP0);
            Log2(("raw r0 mode refused: CR0.WP=0!\n"));
            return false;
        }

#ifdef VBOX_WITH_RAW_MODE
        if (PATMIsPatchGCAddr(env->pVM, eip))
        {
            Log2(("raw r0 mode forced: patch code\n"));
            *piException = EXCP_EXECUTE_RAW;
            return true;
        }
#endif

#if !defined(VBOX_ALLOW_IF0) && !defined(VBOX_RUN_INTERRUPT_GATE_HANDLERS)
        if (!(env->eflags & IF_MASK))
        {
            STAM_COUNTER_INC(&gStatRefuseIF0);
            ////Log2(("R0: IF=0 VIF=%d %08X\n", eip, *env->pVMeflags));
            //Log2(("RR0: Interrupts turned off; fall back to emulation\n"));
            return false;
        }
#endif

#ifndef VBOX_WITH_RAW_RING1
        if (((env->eflags >> IOPL_SHIFT) & 3) != 0)
        {
            Log2(("raw r0 mode refused: IOPL %d\n", ((env->eflags >> IOPL_SHIFT) & 3)));
            return false;
        }
#endif
        env->state |= CPU_RAW_RING0;
    }

    /*
     * Don't reschedule the first time we're called, because there might be
     * special reasons why we're here that is not covered by the above checks.
     */
    if (env->pVM->rem.s.cCanExecuteRaw == 1)
    {
        Log2(("raw mode refused: first scheduling\n"));
        STAM_COUNTER_INC(&gStatRefuseCanExecute);
        return false;
    }

    /*
     * Stale hidden selectors means raw-mode is unsafe (being very careful).
     */
    if (env->segs[R_CS].fVBoxFlags & CPUMSELREG_FLAGS_STALE)
    {
        Log2(("raw mode refused: stale CS (%#x)\n", env->segs[R_CS].selector));
        STAM_COUNTER_INC(&gaStatRefuseStale[R_CS]);
        return false;
    }
    if (env->segs[R_SS].fVBoxFlags & CPUMSELREG_FLAGS_STALE)
    {
        Log2(("raw mode refused: stale SS (%#x)\n", env->segs[R_SS].selector));
        STAM_COUNTER_INC(&gaStatRefuseStale[R_SS]);
        return false;
    }
    if (env->segs[R_DS].fVBoxFlags & CPUMSELREG_FLAGS_STALE)
    {
        Log2(("raw mode refused: stale DS (%#x)\n", env->segs[R_DS].selector));
        STAM_COUNTER_INC(&gaStatRefuseStale[R_DS]);
        return false;
    }
    if (env->segs[R_ES].fVBoxFlags & CPUMSELREG_FLAGS_STALE)
    {
        Log2(("raw mode refused: stale ES (%#x)\n", env->segs[R_ES].selector));
        STAM_COUNTER_INC(&gaStatRefuseStale[R_ES]);
        return false;
    }
    if (env->segs[R_FS].fVBoxFlags & CPUMSELREG_FLAGS_STALE)
    {
        Log2(("raw mode refused: stale FS (%#x)\n", env->segs[R_FS].selector));
        STAM_COUNTER_INC(&gaStatRefuseStale[R_FS]);
        return false;
    }
    if (env->segs[R_GS].fVBoxFlags & CPUMSELREG_FLAGS_STALE)
    {
        Log2(("raw mode refused: stale GS (%#x)\n", env->segs[R_GS].selector));
        STAM_COUNTER_INC(&gaStatRefuseStale[R_GS]);
        return false;
    }

/*    Assert(env->pVCpu && PGMPhysIsA20Enabled(env->pVCpu));*/
    *piException = EXCP_EXECUTE_RAW;
    return true;
}


#ifdef VBOX_WITH_RAW_MODE
/**
 * Fetches a code byte.
 *
 * @returns Success indicator (bool) for ease of use.
 * @param   env         The CPU environment structure.
 * @param   GCPtrInstr  Where to fetch code.
 * @param   pu8Byte     Where to store the byte on success
 */
bool remR3GetOpcode(CPUX86State *env, RTGCPTR GCPtrInstr, uint8_t *pu8Byte)
{
    int rc = PATMR3QueryOpcode(env->pVM, GCPtrInstr, pu8Byte);
    if (RT_SUCCESS(rc))
        return true;
    return false;
}
#endif /* VBOX_WITH_RAW_MODE */


/**
 * Flush (or invalidate if you like) page table/dir entry.
 *
 * (invlpg instruction; tlb_flush_page)
 *
 * @param   env         Pointer to cpu environment.
 * @param   GCPtr       The virtual address which page table/dir entry should be invalidated.
 */
void remR3FlushPage(CPUX86State *env, RTGCPTR GCPtr)
{
    PVM pVM = env->pVM;
    PCPUMCTX pCtx;
    int rc;

    Assert(EMRemIsLockOwner(env->pVM));

    /*
     * When we're replaying invlpg instructions or restoring a saved
     * state we disable this path.
     */
    if (pVM->rem.s.fIgnoreInvlPg || pVM->rem.s.cIgnoreAll)
        return;
    LogFlow(("remR3FlushPage: GCPtr=%RGv\n", GCPtr));
    Assert(pVM->rem.s.fInREM || pVM->rem.s.fInStateSync);

    //RAWEx_ProfileStop(env, STATS_QEMU_TOTAL);

    /*
     * Update the control registers before calling PGMFlushPage.
     */
    pCtx = (PCPUMCTX)pVM->rem.s.pCtx;
    Assert(pCtx);
    pCtx->cr0 = env->cr[0];
    pCtx->cr3 = env->cr[3];
#ifdef VBOX_WITH_RAW_MODE
    if (((env->cr[4] ^ pCtx->cr4) & X86_CR4_VME) && !HMIsEnabled(pVM))
        VMCPU_FF_SET(env->pVCpu, VMCPU_FF_SELM_SYNC_TSS);
#endif
    pCtx->cr4 = env->cr[4];

    /*
     * Let PGM do the rest.
     */
    Assert(env->pVCpu);
    rc = PGMInvalidatePage(env->pVCpu, GCPtr);
    if (RT_FAILURE(rc))
    {
        AssertMsgFailed(("remR3FlushPage %RGv failed with %d!!\n", GCPtr, rc));
        VMCPU_FF_SET(env->pVCpu, VMCPU_FF_PGM_SYNC_CR3);
    }
    //RAWEx_ProfileStart(env, STATS_QEMU_TOTAL);
}


#ifndef REM_PHYS_ADDR_IN_TLB
/** Wrapper for PGMR3PhysTlbGCPhys2Ptr. */
void *remR3TlbGCPhys2Ptr(CPUX86State *env1, target_ulong physAddr, int fWritable)
{
    void *pv;
    int rc;


    /* Address must be aligned enough to fiddle with lower bits */
    Assert((physAddr & 0x3) == 0);
    /*AssertMsg((env1->a20_mask & physAddr) == physAddr, ("%llx\n", (uint64_t)physAddr));*/

    STAM_PROFILE_START(&gStatGCPhys2HCVirt, a);
    rc = PGMR3PhysTlbGCPhys2Ptr(env1->pVM, physAddr, true /*fWritable*/, &pv);
    STAM_PROFILE_STOP(&gStatGCPhys2HCVirt, a);
    Assert(   rc == VINF_SUCCESS
           || rc == VINF_PGM_PHYS_TLB_CATCH_WRITE
           || rc == VERR_PGM_PHYS_TLB_CATCH_ALL
           || rc == VERR_PGM_PHYS_TLB_UNASSIGNED);
    if (RT_FAILURE(rc))
        return (void *)1;
    if (rc == VINF_PGM_PHYS_TLB_CATCH_WRITE)
        return (void *)((uintptr_t)pv | 2);
    return pv;
}
#endif /* REM_PHYS_ADDR_IN_TLB */


/**
 * Called from tlb_protect_code in order to write monitor a code page.
 *
 * @param   env             Pointer to the CPU environment.
 * @param   GCPtr           Code page to monitor
 */
void remR3ProtectCode(CPUX86State *env, RTGCPTR GCPtr)
{
#ifdef VBOX_REM_PROTECT_PAGES_FROM_SMC
    Assert(env->pVM->rem.s.fInREM);
    if (     (env->cr[0] & X86_CR0_PG)                      /* paging must be enabled */
        &&  !(env->state & CPU_EMULATE_SINGLE_INSTR)        /* ignore during single instruction execution */
        &&   (((env->hflags >> HF_CPL_SHIFT) & 3) == 0)     /* supervisor mode only */
        &&  !(env->eflags & VM_MASK)                        /* no V86 mode */
        &&  !HMIsEnabled(env->pVM))
        CSAMR3MonitorPage(env->pVM, GCPtr, CSAM_TAG_REM);
#endif
}


/**
 * Called from tlb_unprotect_code in order to clear write monitoring for a code page.
 *
 * @param   env             Pointer to the CPU environment.
 * @param   GCPtr           Code page to monitor
 */
void remR3UnprotectCode(CPUX86State *env, RTGCPTR GCPtr)
{
    Assert(env->pVM->rem.s.fInREM);
#ifdef VBOX_REM_PROTECT_PAGES_FROM_SMC
    if (     (env->cr[0] & X86_CR0_PG)                      /* paging must be enabled */
        &&  !(env->state & CPU_EMULATE_SINGLE_INSTR)        /* ignore during single instruction execution */
        &&   (((env->hflags >> HF_CPL_SHIFT) & 3) == 0)     /* supervisor mode only */
        &&  !(env->eflags & VM_MASK)                        /* no V86 mode */
        &&  !HMIsEnabled(env->pVM))
        CSAMR3UnmonitorPage(env->pVM, GCPtr, CSAM_TAG_REM);
#endif
}


/**
 * Called when the CPU is initialized, any of the CRx registers are changed or
 * when the A20 line is modified.
 *
 * @param   env             Pointer to the CPU environment.
 * @param   fGlobal         Set if the flush is global.
 */
void remR3FlushTLB(CPUX86State *env, bool fGlobal)
{
    PVM      pVM = env->pVM;
    PCPUMCTX pCtx;
    Assert(EMRemIsLockOwner(pVM));

    /*
     * When we're replaying invlpg instructions or restoring a saved
     * state we disable this path.
     */
    if (pVM->rem.s.fIgnoreCR3Load || pVM->rem.s.cIgnoreAll)
        return;
    Assert(pVM->rem.s.fInREM);

    /*
     * The caller doesn't check cr4, so we have to do that for ourselves.
     */
    if (!fGlobal && !(env->cr[4] & X86_CR4_PGE))
        fGlobal = true;
    Log(("remR3FlushTLB: CR0=%08RX64 CR3=%08RX64 CR4=%08RX64 %s\n", (uint64_t)env->cr[0], (uint64_t)env->cr[3], (uint64_t)env->cr[4], fGlobal ? " global" : ""));

    /*
     * Update the control registers before calling PGMR3FlushTLB.
     */
    pCtx = (PCPUMCTX)pVM->rem.s.pCtx;
    Assert(pCtx);
    pCtx->cr0 = env->cr[0];
    pCtx->cr3 = env->cr[3];
#ifdef VBOX_WITH_RAW_MODE
    if (((env->cr[4] ^ pCtx->cr4) & X86_CR4_VME) && !HMIsEnabled(pVM))
        VMCPU_FF_SET(env->pVCpu, VMCPU_FF_SELM_SYNC_TSS);
#endif
    pCtx->cr4 = env->cr[4];

    /*
     * Let PGM do the rest.
     */
    Assert(env->pVCpu);
    PGMFlushTLB(env->pVCpu, env->cr[3], fGlobal);
}


/**
 * Called when any of the cr0, cr4 or efer registers is updated.
 *
 * @param   env     Pointer to the CPU environment.
 */
void remR3ChangeCpuMode(CPUX86State *env)
{
    PVM         pVM = env->pVM;
    uint64_t    efer;
    PCPUMCTX    pCtx;
    int         rc;

    /*
     * When we're replaying loads or restoring a saved
     * state this path is disabled.
     */
    if (pVM->rem.s.fIgnoreCpuMode || pVM->rem.s.cIgnoreAll)
        return;
    Assert(pVM->rem.s.fInREM);

    pCtx = (PCPUMCTX)pVM->rem.s.pCtx;
    Assert(pCtx);

    /*
     * Notify PGM about WP0 being enabled (like CPUSetGuestCR0 does).
     */
    if (((env->cr[0] ^ pCtx->cr0) & X86_CR0_WP) && (env->cr[0] & X86_CR0_WP))
        PGMCr0WpEnabled(env->pVCpu);

    /*
     * Update the control registers before calling PGMChangeMode()
     * as it may need to map whatever cr3 is pointing to.
     */
    pCtx->cr0 = env->cr[0];
    pCtx->cr3 = env->cr[3];
#ifdef VBOX_WITH_RAW_MODE
    if (((env->cr[4] ^ pCtx->cr4) & X86_CR4_VME) && !HMIsEnabled(pVM))
        VMCPU_FF_SET(env->pVCpu, VMCPU_FF_SELM_SYNC_TSS);
#endif
    pCtx->cr4 = env->cr[4];
#ifdef TARGET_X86_64
    efer = env->efer;
    pCtx->msrEFER = efer;
#else
    efer = 0;
#endif
    Assert(env->pVCpu);
    rc = PGMChangeMode(env->pVCpu, env->cr[0], env->cr[4], efer);
    if (rc != VINF_SUCCESS)
    {
        if (rc >= VINF_EM_FIRST && rc <= VINF_EM_LAST)
        {
            Log(("PGMChangeMode(, %RX64, %RX64, %RX64) -> %Rrc -> remR3RaiseRC\n", env->cr[0], env->cr[4], efer, rc));
            remR3RaiseRC(env->pVM, rc);
        }
        else
            cpu_abort(env, "PGMChangeMode(, %RX64, %RX64, %RX64) -> %Rrc\n", env->cr[0], env->cr[4], efer, rc);
    }
}


/**
 * Called from compiled code to run dma.
 *
 * @param   env             Pointer to the CPU environment.
 */
void remR3DmaRun(CPUX86State *env)
{
    remR3ProfileStop(STATS_QEMU_RUN_EMULATED_CODE);
    PDMR3DmaRun(env->pVM);
    remR3ProfileStart(STATS_QEMU_RUN_EMULATED_CODE);
}


/**
 * Called from compiled code to schedule pending timers in VMM
 *
 * @param   env             Pointer to the CPU environment.
 */
void remR3TimersRun(CPUX86State *env)
{
    LogFlow(("remR3TimersRun:\n"));
    LogIt(RTLOGGRPFLAGS_LEVEL_5, LOG_GROUP_TM, ("remR3TimersRun\n"));
    remR3ProfileStop(STATS_QEMU_RUN_EMULATED_CODE);
    remR3ProfileStart(STATS_QEMU_RUN_TIMERS);
    TMR3TimerQueuesDo(env->pVM);
    remR3ProfileStop(STATS_QEMU_RUN_TIMERS);
    remR3ProfileStart(STATS_QEMU_RUN_EMULATED_CODE);
}


/**
 * Record trap occurrence
 *
 * @returns VBox status code
 * @param   env             Pointer to the CPU environment.
 * @param   uTrap           Trap nr
 * @param   uErrorCode      Error code
 * @param   pvNextEIP       Next EIP
 */
int remR3NotifyTrap(CPUX86State *env, uint32_t uTrap, uint32_t uErrorCode, RTGCPTR pvNextEIP)
{
    PVM pVM = env->pVM;
#ifdef VBOX_WITH_STATISTICS
    static STAMCOUNTER s_aStatTrap[255];
    static bool        s_aRegisters[RT_ELEMENTS(s_aStatTrap)];
#endif

#ifdef VBOX_WITH_STATISTICS
    if (uTrap < 255)
    {
        if (!s_aRegisters[uTrap])
        {
            char szStatName[64];
            s_aRegisters[uTrap] = true;
            RTStrPrintf(szStatName, sizeof(szStatName), "/REM/Trap/0x%02X", uTrap);
            STAM_REG(env->pVM, &s_aStatTrap[uTrap], STAMTYPE_COUNTER, szStatName, STAMUNIT_OCCURENCES, "Trap stats.");
        }
        STAM_COUNTER_INC(&s_aStatTrap[uTrap]);
    }
#endif
    Log(("remR3NotifyTrap: uTrap=%x error=%x next_eip=%RGv eip=%RGv cr2=%RGv\n", uTrap, uErrorCode, pvNextEIP, (RTGCPTR)env->eip, (RTGCPTR)env->cr[2]));
    if(   uTrap < 0x20
       && (env->cr[0] & X86_CR0_PE)
       && !(env->eflags & X86_EFL_VM))
    {
#ifdef DEBUG
        remR3DisasInstr(env, 1, "remR3NotifyTrap: ");
#endif
        if(pVM->rem.s.uPendingException == uTrap && ++pVM->rem.s.cPendingExceptions > 512)
        {
            LogRel(("VERR_REM_TOO_MANY_TRAPS -> uTrap=%x error=%x next_eip=%RGv eip=%RGv cr2=%RGv\n", uTrap, uErrorCode, pvNextEIP, (RTGCPTR)env->eip, (RTGCPTR)env->cr[2]));
            remR3RaiseRC(env->pVM, VERR_REM_TOO_MANY_TRAPS);
            return VERR_REM_TOO_MANY_TRAPS;
        }
        if(pVM->rem.s.uPendingException != uTrap || pVM->rem.s.uPendingExcptEIP != env->eip || pVM->rem.s.uPendingExcptCR2 != env->cr[2])
        {
            Log(("remR3NotifyTrap: uTrap=%#x set as pending\n", uTrap));
            pVM->rem.s.cPendingExceptions = 1;
        }
        pVM->rem.s.uPendingException = uTrap;
        pVM->rem.s.uPendingExcptEIP  = env->eip;
        pVM->rem.s.uPendingExcptCR2  = env->cr[2];
    }
    else
    {
        pVM->rem.s.cPendingExceptions = 0;
        pVM->rem.s.uPendingException = uTrap;
        pVM->rem.s.uPendingExcptEIP  = env->eip;
        pVM->rem.s.uPendingExcptCR2  = env->cr[2];
    }
    return VINF_SUCCESS;
}


/*
 * Clear current active trap
 *
 * @param   pVM         VM Handle.
 */
void remR3TrapClear(PVM pVM)
{
    pVM->rem.s.cPendingExceptions = 0;
    pVM->rem.s.uPendingException  = 0;
    pVM->rem.s.uPendingExcptEIP   = 0;
    pVM->rem.s.uPendingExcptCR2   = 0;
}


/*
 * Record previous call instruction addresses
 *
 * @param   env             Pointer to the CPU environment.
 */
void remR3RecordCall(CPUX86State *env)
{
#ifdef VBOX_WITH_RAW_MODE
    CSAMR3RecordCallAddress(env->pVM, env->eip);
#endif
}


/**
 * Syncs the internal REM state with the VM.
 *
 * This must be called before REMR3Run() is invoked whenever when the REM
 * state is not up to date. Calling it several times in a row is not
 * permitted.
 *
 * @returns VBox status code.
 *
 * @param   pVM         VM Handle.
 * @param   pVCpu       VMCPU Handle.
 *
 * @remark  The caller has to check for important FFs before calling REMR3Run. REMR3State will
 *          no do this since the majority of the callers don't want any unnecessary of events
 *          pending that would immediately interrupt execution.
 */
REMR3DECL(int)  REMR3State(PVM pVM, PVMCPU pVCpu)
{
    register const CPUMCTX *pCtx;
    register unsigned       fFlags;
    unsigned                i;
    TRPMEVENT               enmType;
    uint8_t                 u8TrapNo;
    uint32_t                uCpl;
    int                     rc;

    STAM_PROFILE_START(&pVM->rem.s.StatsState, a);
    Log2(("REMR3State:\n"));

    pVM->rem.s.Env.pVCpu = pVCpu;
    pCtx = pVM->rem.s.pCtx = CPUMQueryGuestCtxPtr(pVCpu);

    Assert(pCtx);
    if (CPUMIsGuestInNestedHwVirtMode(pCtx))
    {
        AssertMsgFailed(("Bad scheduling - can't exec. nested-guest in REM!\n"));
        return VERR_EM_CANNOT_EXEC_GUEST;
    }

    Assert(!pVM->rem.s.fInREM);
    pVM->rem.s.fInStateSync = true;

    /*
     * If we have to flush TBs, do that immediately.
     */
    if (pVM->rem.s.fFlushTBs)
    {
        STAM_COUNTER_INC(&gStatFlushTBs);
        tb_flush(&pVM->rem.s.Env);
        pVM->rem.s.fFlushTBs = false;
    }

    /*
     * Copy the registers which require no special handling.
     */
#ifdef TARGET_X86_64
    /* Note that the high dwords of 64 bits registers are undefined in 32 bits mode and are undefined after a mode change. */
    Assert(R_EAX == 0);
    pVM->rem.s.Env.regs[R_EAX]  = pCtx->rax;
    Assert(R_ECX == 1);
    pVM->rem.s.Env.regs[R_ECX]  = pCtx->rcx;
    Assert(R_EDX == 2);
    pVM->rem.s.Env.regs[R_EDX]  = pCtx->rdx;
    Assert(R_EBX == 3);
    pVM->rem.s.Env.regs[R_EBX]  = pCtx->rbx;
    Assert(R_ESP == 4);
    pVM->rem.s.Env.regs[R_ESP]  = pCtx->rsp;
    Assert(R_EBP == 5);
    pVM->rem.s.Env.regs[R_EBP]  = pCtx->rbp;
    Assert(R_ESI == 6);
    pVM->rem.s.Env.regs[R_ESI]  = pCtx->rsi;
    Assert(R_EDI == 7);
    pVM->rem.s.Env.regs[R_EDI]  = pCtx->rdi;
    pVM->rem.s.Env.regs[8]      = pCtx->r8;
    pVM->rem.s.Env.regs[9]      = pCtx->r9;
    pVM->rem.s.Env.regs[10]     = pCtx->r10;
    pVM->rem.s.Env.regs[11]     = pCtx->r11;
    pVM->rem.s.Env.regs[12]     = pCtx->r12;
    pVM->rem.s.Env.regs[13]     = pCtx->r13;
    pVM->rem.s.Env.regs[14]     = pCtx->r14;
    pVM->rem.s.Env.regs[15]     = pCtx->r15;

    pVM->rem.s.Env.eip          = pCtx->rip;

    pVM->rem.s.Env.eflags       = pCtx->rflags.u64;
#else
    Assert(R_EAX == 0);
    pVM->rem.s.Env.regs[R_EAX]  = pCtx->eax;
    Assert(R_ECX == 1);
    pVM->rem.s.Env.regs[R_ECX]  = pCtx->ecx;
    Assert(R_EDX == 2);
    pVM->rem.s.Env.regs[R_EDX]  = pCtx->edx;
    Assert(R_EBX == 3);
    pVM->rem.s.Env.regs[R_EBX]  = pCtx->ebx;
    Assert(R_ESP == 4);
    pVM->rem.s.Env.regs[R_ESP]  = pCtx->esp;
    Assert(R_EBP == 5);
    pVM->rem.s.Env.regs[R_EBP]  = pCtx->ebp;
    Assert(R_ESI == 6);
    pVM->rem.s.Env.regs[R_ESI]  = pCtx->esi;
    Assert(R_EDI == 7);
    pVM->rem.s.Env.regs[R_EDI]  = pCtx->edi;
    pVM->rem.s.Env.eip          = pCtx->eip;

    pVM->rem.s.Env.eflags       = pCtx->eflags.u32;
#endif

    pVM->rem.s.Env.cr[2]        = pCtx->cr2;

    /** @todo we could probably benefit from using a CPUM_CHANGED_DRx flag too! */
    for (i=0;i<8;i++)
        pVM->rem.s.Env.dr[i] = pCtx->dr[i];

#ifdef HF_HALTED_MASK /** @todo remove me when we're up to date again. */
    /*
     * Clear the halted hidden flag (the interrupt waking up the CPU can
     * have been dispatched in raw mode).
     */
    pVM->rem.s.Env.hflags      &= ~HF_HALTED_MASK;
#endif

    /*
     * Replay invlpg?  Only if we're not flushing the TLB.
     */
    fFlags = CPUMR3RemEnter(pVCpu, &uCpl);
    LogFlow(("CPUMR3RemEnter %x %x\n", fFlags, uCpl));
    if (pVM->rem.s.cInvalidatedPages)
    {
        if (!(fFlags & CPUM_CHANGED_GLOBAL_TLB_FLUSH))
        {
            RTUINT i;

            pVM->rem.s.fIgnoreCR3Load = true;
            pVM->rem.s.fIgnoreInvlPg  = true;
            for (i = 0; i < pVM->rem.s.cInvalidatedPages; i++)
            {
                Log2(("REMR3State: invlpg %RGv\n", pVM->rem.s.aGCPtrInvalidatedPages[i]));
                tlb_flush_page(&pVM->rem.s.Env, pVM->rem.s.aGCPtrInvalidatedPages[i]);
            }
            pVM->rem.s.fIgnoreInvlPg  = false;
            pVM->rem.s.fIgnoreCR3Load = false;
        }
        pVM->rem.s.cInvalidatedPages = 0;
    }

    /* Replay notification changes. */
    REMR3ReplayHandlerNotifications(pVM);

    /* Update MSRs; before CRx registers! */
    pVM->rem.s.Env.efer         = pCtx->msrEFER;
    pVM->rem.s.Env.star         = pCtx->msrSTAR;
    pVM->rem.s.Env.pat          = pCtx->msrPAT;
#ifdef TARGET_X86_64
    pVM->rem.s.Env.lstar        = pCtx->msrLSTAR;
    pVM->rem.s.Env.cstar        = pCtx->msrCSTAR;
    pVM->rem.s.Env.fmask        = pCtx->msrSFMASK;
    pVM->rem.s.Env.kernelgsbase = pCtx->msrKERNELGSBASE;

    /* Update the internal long mode activate flag according to the new EFER value. */
    if (pCtx->msrEFER & MSR_K6_EFER_LMA)
        pVM->rem.s.Env.hflags |= HF_LMA_MASK;
    else
        pVM->rem.s.Env.hflags &= ~(HF_LMA_MASK | HF_CS64_MASK);
#endif

    /* Update the inhibit IRQ mask. */
    pVM->rem.s.Env.hflags &= ~HF_INHIBIT_IRQ_MASK;
    if (VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS))
    {
        RTGCPTR InhibitPC = EMGetInhibitInterruptsPC(pVCpu);
        if (InhibitPC == pCtx->rip)
            pVM->rem.s.Env.hflags |= HF_INHIBIT_IRQ_MASK;
        else
        {
            Log(("Clearing VMCPU_FF_INHIBIT_INTERRUPTS at %RGv - successor %RGv (REM#1)\n", (RTGCPTR)pCtx->rip, InhibitPC));
            VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS);
        }
    }

    /* Update the inhibit NMI mask. */
    pVM->rem.s.Env.hflags2 &= ~HF2_NMI_MASK;
    if (VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_BLOCK_NMIS))
        pVM->rem.s.Env.hflags2 |= HF2_NMI_MASK;

    /*
     * Sync the A20 gate.
     */
    bool fA20State = PGMPhysIsA20Enabled(pVCpu);
    if (fA20State != RT_BOOL(pVM->rem.s.Env.a20_mask & RT_BIT(20)))
    {
        ASMAtomicIncU32(&pVM->rem.s.cIgnoreAll);
        cpu_x86_set_a20(&pVM->rem.s.Env, fA20State);
        ASMAtomicDecU32(&pVM->rem.s.cIgnoreAll);
    }

    /*
     * Registers which are rarely changed and require special handling / order when changed.
     */
    if (fFlags & (  CPUM_CHANGED_GLOBAL_TLB_FLUSH
                  | CPUM_CHANGED_CR4
                  | CPUM_CHANGED_CR0
                  | CPUM_CHANGED_CR3
                  | CPUM_CHANGED_GDTR
                  | CPUM_CHANGED_IDTR
                  | CPUM_CHANGED_SYSENTER_MSR
                  | CPUM_CHANGED_LDTR
                  | CPUM_CHANGED_CPUID
                  | CPUM_CHANGED_FPU_REM
                 )
        )
    {
        if (fFlags & CPUM_CHANGED_GLOBAL_TLB_FLUSH)
        {
            pVM->rem.s.fIgnoreCR3Load = true;
            tlb_flush(&pVM->rem.s.Env, true);
            pVM->rem.s.fIgnoreCR3Load = false;
        }

        /* CR4 before CR0! */
        if (fFlags & CPUM_CHANGED_CR4)
        {
            pVM->rem.s.fIgnoreCR3Load = true;
            pVM->rem.s.fIgnoreCpuMode = true;
            cpu_x86_update_cr4(&pVM->rem.s.Env, pCtx->cr4);
            pVM->rem.s.fIgnoreCpuMode = false;
            pVM->rem.s.fIgnoreCR3Load = false;
        }

        if (fFlags & CPUM_CHANGED_CR0)
        {
            pVM->rem.s.fIgnoreCR3Load = true;
            pVM->rem.s.fIgnoreCpuMode = true;
            cpu_x86_update_cr0(&pVM->rem.s.Env, pCtx->cr0);
            pVM->rem.s.fIgnoreCpuMode = false;
            pVM->rem.s.fIgnoreCR3Load = false;
        }

        if (fFlags & CPUM_CHANGED_CR3)
        {
            pVM->rem.s.fIgnoreCR3Load = true;
            cpu_x86_update_cr3(&pVM->rem.s.Env, pCtx->cr3);
            pVM->rem.s.fIgnoreCR3Load = false;
        }

        if (fFlags & CPUM_CHANGED_GDTR)
        {
            pVM->rem.s.Env.gdt.base     = pCtx->gdtr.pGdt;
            pVM->rem.s.Env.gdt.limit    = pCtx->gdtr.cbGdt;
        }

        if (fFlags & CPUM_CHANGED_IDTR)
        {
            pVM->rem.s.Env.idt.base     = pCtx->idtr.pIdt;
            pVM->rem.s.Env.idt.limit    = pCtx->idtr.cbIdt;
        }

        if (fFlags & CPUM_CHANGED_SYSENTER_MSR)
        {
            pVM->rem.s.Env.sysenter_cs  = pCtx->SysEnter.cs;
            pVM->rem.s.Env.sysenter_eip = pCtx->SysEnter.eip;
            pVM->rem.s.Env.sysenter_esp = pCtx->SysEnter.esp;
        }

        if (fFlags & CPUM_CHANGED_LDTR)
        {
            if (pCtx->ldtr.fFlags & CPUMSELREG_FLAGS_VALID)
            {
                pVM->rem.s.Env.ldt.selector    = pCtx->ldtr.Sel;
                pVM->rem.s.Env.ldt.newselector = 0;
                pVM->rem.s.Env.ldt.fVBoxFlags  = pCtx->ldtr.fFlags;
                pVM->rem.s.Env.ldt.base        = pCtx->ldtr.u64Base;
                pVM->rem.s.Env.ldt.limit       = pCtx->ldtr.u32Limit;
                pVM->rem.s.Env.ldt.flags       = (pCtx->ldtr.Attr.u & SEL_FLAGS_SMASK) << SEL_FLAGS_SHIFT;
            }
            else
            {
                AssertFailed(); /* Shouldn't happen, see cpumR3LoadExec. */
                sync_ldtr(&pVM->rem.s.Env, pCtx->ldtr.Sel);
            }
        }

        if (fFlags & CPUM_CHANGED_CPUID)
        {
            uint32_t u32Dummy;

            /*
             * Get the CPUID features.
             */
            CPUMGetGuestCpuId(pVCpu,          1, 0, &u32Dummy, &u32Dummy, &pVM->rem.s.Env.cpuid_ext_features, &pVM->rem.s.Env.cpuid_features);
            CPUMGetGuestCpuId(pVCpu, 0x80000001, 0, &u32Dummy, &u32Dummy, &u32Dummy, &pVM->rem.s.Env.cpuid_ext2_features);
        }

        /* Sync FPU state after CR4, CPUID and EFER (!). */
        if (fFlags & CPUM_CHANGED_FPU_REM)
            save_raw_fp_state(&pVM->rem.s.Env, (uint8_t *)&pCtx->pXStateR3->x87); /* 'save' is an excellent name. */
    }

    /*
     * Sync TR unconditionally to make life simpler.
     */
    pVM->rem.s.Env.tr.selector    = pCtx->tr.Sel;
    pVM->rem.s.Env.tr.newselector = 0;
    pVM->rem.s.Env.tr.fVBoxFlags  = pCtx->tr.fFlags;
    pVM->rem.s.Env.tr.base        = pCtx->tr.u64Base;
    pVM->rem.s.Env.tr.limit       = pCtx->tr.u32Limit;
    pVM->rem.s.Env.tr.flags       = (pCtx->tr.Attr.u & SEL_FLAGS_SMASK) << SEL_FLAGS_SHIFT;

    /*
     * Update selector registers.
     *
     * This must be done *after* we've synced gdt, ldt and crX registers
     * since we're reading the GDT/LDT om sync_seg. This will happen with
     * saved state which takes a quick dip into rawmode for instance.
     *
     * CPL/Stack; Note first check this one as the CPL might have changed.
     * The wrong CPL can cause QEmu to raise an exception in sync_seg!!
     */
    cpu_x86_set_cpl(&pVM->rem.s.Env, uCpl);
    /* Note! QEmu saves the 2nd dword of the descriptor; we should convert the attribute word back! */
#define SYNC_IN_SREG(a_pEnv, a_SReg, a_pRemSReg, a_pVBoxSReg) \
        do \
        { \
            if (CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, a_pVBoxSReg)) \
            { \
                cpu_x86_load_seg_cache(a_pEnv, R_##a_SReg, \
                                       (a_pVBoxSReg)->Sel, \
                                       (a_pVBoxSReg)->u64Base, \
                                       (a_pVBoxSReg)->u32Limit, \
                                       ((a_pVBoxSReg)->Attr.u & SEL_FLAGS_SMASK) << SEL_FLAGS_SHIFT); \
                (a_pRemSReg)->fVBoxFlags = (a_pVBoxSReg)->fFlags; \
            } \
            /* This only-reload-if-changed stuff is the old approach, we should ditch it. */ \
            else if ((a_pRemSReg)->selector != (a_pVBoxSReg)->Sel) \
            { \
                Log2(("REMR3State: " #a_SReg " changed from %04x to %04x!\n", \
                      (a_pRemSReg)->selector, (a_pVBoxSReg)->Sel)); \
                sync_seg(a_pEnv, R_##a_SReg, (a_pVBoxSReg)->Sel); \
                if ((a_pRemSReg)->newselector) \
                    STAM_COUNTER_INC(&gStatSelOutOfSync[R_##a_SReg]); \
            } \
            else \
                (a_pRemSReg)->newselector = 0; \
        } while (0)

    SYNC_IN_SREG(&pVM->rem.s.Env, CS, &pVM->rem.s.Env.segs[R_CS], &pCtx->cs);
    SYNC_IN_SREG(&pVM->rem.s.Env, SS, &pVM->rem.s.Env.segs[R_SS], &pCtx->ss);
    SYNC_IN_SREG(&pVM->rem.s.Env, DS, &pVM->rem.s.Env.segs[R_DS], &pCtx->ds);
    SYNC_IN_SREG(&pVM->rem.s.Env, ES, &pVM->rem.s.Env.segs[R_ES], &pCtx->es);
    SYNC_IN_SREG(&pVM->rem.s.Env, FS, &pVM->rem.s.Env.segs[R_FS], &pCtx->fs);
    SYNC_IN_SREG(&pVM->rem.s.Env, GS, &pVM->rem.s.Env.segs[R_GS], &pCtx->gs);
    /** @todo need to find a way to communicate potential GDT/LDT changes and thread switches. The selector might
     * be the same but not the base/limit. */

    /*
     * Check for traps.
     */
    pVM->rem.s.Env.exception_index = -1; /** @todo this won't work :/ */
    rc = TRPMQueryTrap(pVCpu, &u8TrapNo, &enmType);
    if (RT_SUCCESS(rc))
    {
#ifdef DEBUG
        if (u8TrapNo == 0x80)
        {
            remR3DumpLnxSyscall(pVCpu);
            remR3DumpOBsdSyscall(pVCpu);
        }
#endif

        pVM->rem.s.Env.exception_index = u8TrapNo;
        if (enmType != TRPM_SOFTWARE_INT)
        {
            pVM->rem.s.Env.exception_is_int     = enmType == TRPM_HARDWARE_INT
                                                ? EXCEPTION_IS_INT_VALUE_HARDWARE_IRQ : 0; /* HACK ALERT! */
            pVM->rem.s.Env.exception_next_eip   = pVM->rem.s.Env.eip;
        }
        else
        {
            /*
             * The there are two 1 byte opcodes and one 2 byte opcode for software interrupts.
             * We ASSUME that there are no prefixes and sets the default to 2 byte, and checks
             * for int03 and into.
             */
            pVM->rem.s.Env.exception_is_int     = 1;
            pVM->rem.s.Env.exception_next_eip   = pCtx->rip + 2;
            /* int 3 may be generated by one-byte 0xcc */
            if (u8TrapNo == 3)
            {
                if (read_byte(&pVM->rem.s.Env, pVM->rem.s.Env.segs[R_CS].base + pCtx->rip) == 0xcc)
                    pVM->rem.s.Env.exception_next_eip = pCtx->rip + 1;
            }
            /* int 4 may be generated by one-byte 0xce */
            else if (u8TrapNo == 4)
            {
                if (read_byte(&pVM->rem.s.Env, pVM->rem.s.Env.segs[R_CS].base + pCtx->rip) == 0xce)
                    pVM->rem.s.Env.exception_next_eip = pCtx->rip + 1;
            }
        }

        /* get error code and cr2 if needed. */
        if (enmType == TRPM_TRAP)
        {
            switch (u8TrapNo)
            {
                case X86_XCPT_PF:
                    pVM->rem.s.Env.cr[2] = TRPMGetFaultAddress(pVCpu);
                    /* fallthru */
                case X86_XCPT_TS: case X86_XCPT_NP: case X86_XCPT_SS: case X86_XCPT_GP:
                    pVM->rem.s.Env.error_code = TRPMGetErrorCode(pVCpu);
                    break;

                case X86_XCPT_AC: case X86_XCPT_DF:
                default:
                    pVM->rem.s.Env.error_code = 0;
                    break;
            }
        }
        else
            pVM->rem.s.Env.error_code = 0;

        /*
         * We can now reset the active trap since the recompiler is gonna have a go at it.
         */
        rc = TRPMResetTrap(pVCpu);
        AssertRC(rc);
        Log2(("REMR3State: trap=%02x errcd=%RGv cr2=%RGv nexteip=%RGv%s\n", pVM->rem.s.Env.exception_index, (RTGCPTR)pVM->rem.s.Env.error_code,
              (RTGCPTR)pVM->rem.s.Env.cr[2], (RTGCPTR)pVM->rem.s.Env.exception_next_eip, pVM->rem.s.Env.exception_is_int ? " software" : ""));
    }

    /*
     * Clear old interrupt request flags; Check for pending hardware interrupts.
     * (See @remark for why we don't check for other FFs.)
     */
    pVM->rem.s.Env.interrupt_request &= ~(CPU_INTERRUPT_HARD | CPU_INTERRUPT_EXITTB | CPU_INTERRUPT_TIMER);
    if (VMCPU_FF_TEST_AND_CLEAR(pVCpu, VMCPU_FF_UPDATE_APIC))
        APICUpdatePendingInterrupts(pVCpu);
    if (VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC))
        pVM->rem.s.Env.interrupt_request |= CPU_INTERRUPT_HARD;

    /*
     * We're now in REM mode.
     */
    VMCPU_SET_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC_REM);
    pVM->rem.s.fInREM = true;
    pVM->rem.s.fInStateSync = false;
    pVM->rem.s.cCanExecuteRaw = 0;
    STAM_PROFILE_STOP(&pVM->rem.s.StatsState, a);
    Log2(("REMR3State: returns VINF_SUCCESS\n"));
    return VINF_SUCCESS;
}


/**
 * Syncs back changes in the REM state to the VM state.
 *
 * This must be called after invoking REMR3Run().
 * Calling it several times in a row is not permitted.
 *
 * @returns VBox status code.
 *
 * @param   pVM         VM Handle.
 * @param   pVCpu       VMCPU Handle.
 */
REMR3DECL(int) REMR3StateBack(PVM pVM, PVMCPU pVCpu)
{
    register PCPUMCTX pCtx = pVM->rem.s.pCtx;
    Assert(pCtx);
    unsigned          i;

    STAM_PROFILE_START(&pVM->rem.s.StatsStateBack, a);
    Log2(("REMR3StateBack:\n"));
    Assert(pVM->rem.s.fInREM);

    /*
     * Copy back the registers.
     * This is done in the order they are declared in the CPUMCTX structure.
     */

    /** @todo FOP */
    /** @todo FPUIP */
    /** @todo CS */
    /** @todo FPUDP */
    /** @todo DS */

    /** @todo check if FPU/XMM was actually used in the recompiler */
    restore_raw_fp_state(&pVM->rem.s.Env, (uint8_t *)&pCtx->pXStateR3->x87);
////    dprintf2(("FPU state CW=%04X TT=%04X SW=%04X (%04X)\n", env->fpuc, env->fpstt, env->fpus, pVMCtx->fpu.FSW));

#ifdef TARGET_X86_64
    /* Note that the high dwords of 64 bits registers are undefined in 32 bits mode and are undefined after a mode change. */
    pCtx->rdi           = pVM->rem.s.Env.regs[R_EDI];
    pCtx->rsi           = pVM->rem.s.Env.regs[R_ESI];
    pCtx->rbp           = pVM->rem.s.Env.regs[R_EBP];
    pCtx->rax           = pVM->rem.s.Env.regs[R_EAX];
    pCtx->rbx           = pVM->rem.s.Env.regs[R_EBX];
    pCtx->rdx           = pVM->rem.s.Env.regs[R_EDX];
    pCtx->rcx           = pVM->rem.s.Env.regs[R_ECX];
    pCtx->r8            = pVM->rem.s.Env.regs[8];
    pCtx->r9            = pVM->rem.s.Env.regs[9];
    pCtx->r10           = pVM->rem.s.Env.regs[10];
    pCtx->r11           = pVM->rem.s.Env.regs[11];
    pCtx->r12           = pVM->rem.s.Env.regs[12];
    pCtx->r13           = pVM->rem.s.Env.regs[13];
    pCtx->r14           = pVM->rem.s.Env.regs[14];
    pCtx->r15           = pVM->rem.s.Env.regs[15];

    pCtx->rsp           = pVM->rem.s.Env.regs[R_ESP];

#else
    pCtx->edi           = pVM->rem.s.Env.regs[R_EDI];
    pCtx->esi           = pVM->rem.s.Env.regs[R_ESI];
    pCtx->ebp           = pVM->rem.s.Env.regs[R_EBP];
    pCtx->eax           = pVM->rem.s.Env.regs[R_EAX];
    pCtx->ebx           = pVM->rem.s.Env.regs[R_EBX];
    pCtx->edx           = pVM->rem.s.Env.regs[R_EDX];
    pCtx->ecx           = pVM->rem.s.Env.regs[R_ECX];

    pCtx->esp           = pVM->rem.s.Env.regs[R_ESP];
#endif

#define SYNC_BACK_SREG(a_sreg, a_SREG) \
        do \
        { \
            pCtx->a_sreg.Sel = pVM->rem.s.Env.segs[R_##a_SREG].selector; \
            if (!pVM->rem.s.Env.segs[R_SS].newselector) \
            { \
                pCtx->a_sreg.ValidSel = pVM->rem.s.Env.segs[R_##a_SREG].selector; \
                pCtx->a_sreg.fFlags   = CPUMSELREG_FLAGS_VALID; \
                pCtx->a_sreg.u64Base  = pVM->rem.s.Env.segs[R_##a_SREG].base; \
                pCtx->a_sreg.u32Limit = pVM->rem.s.Env.segs[R_##a_SREG].limit; \
                /* Note! QEmu saves the 2nd dword of the descriptor; we (VT-x/AMD-V) keep only the attributes! */ \
                pCtx->a_sreg.Attr.u   = (pVM->rem.s.Env.segs[R_##a_SREG].flags >> SEL_FLAGS_SHIFT) & SEL_FLAGS_SMASK; \
            } \
            else \
            { \
                pCtx->a_sreg.fFlags = 0; \
                STAM_COUNTER_INC(&gStatSelOutOfSyncStateBack[R_##a_SREG]); \
            } \
        } while (0)

    SYNC_BACK_SREG(es, ES);
    SYNC_BACK_SREG(cs, CS);
    SYNC_BACK_SREG(ss, SS);
    SYNC_BACK_SREG(ds, DS);
    SYNC_BACK_SREG(fs, FS);
    SYNC_BACK_SREG(gs, GS);

#ifdef TARGET_X86_64
    pCtx->rip           = pVM->rem.s.Env.eip;
    pCtx->rflags.u64    = pVM->rem.s.Env.eflags;
#else
    pCtx->eip           = pVM->rem.s.Env.eip;
    pCtx->eflags.u32    = pVM->rem.s.Env.eflags;
#endif

    pCtx->cr0           = pVM->rem.s.Env.cr[0];
    pCtx->cr2           = pVM->rem.s.Env.cr[2];
    pCtx->cr3           = pVM->rem.s.Env.cr[3];
#ifdef VBOX_WITH_RAW_MODE
    if (((pVM->rem.s.Env.cr[4] ^ pCtx->cr4) & X86_CR4_VME) && !HMIsEnabled(pVM))
        VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_TSS);
#endif
    pCtx->cr4           = pVM->rem.s.Env.cr[4];

    for (i = 0; i < 8; i++)
        pCtx->dr[i] = pVM->rem.s.Env.dr[i];

    pCtx->gdtr.cbGdt    = pVM->rem.s.Env.gdt.limit;
    if (pCtx->gdtr.pGdt != pVM->rem.s.Env.gdt.base)
    {
        pCtx->gdtr.pGdt = pVM->rem.s.Env.gdt.base;
        STAM_COUNTER_INC(&gStatREMGDTChange);
#ifdef VBOX_WITH_RAW_MODE
        if (!HMIsEnabled(pVM))
            VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_GDT);
#endif
    }

    pCtx->idtr.cbIdt    = pVM->rem.s.Env.idt.limit;
    if (pCtx->idtr.pIdt != pVM->rem.s.Env.idt.base)
    {
        pCtx->idtr.pIdt = pVM->rem.s.Env.idt.base;
        STAM_COUNTER_INC(&gStatREMIDTChange);
#ifdef VBOX_WITH_RAW_MODE
        if (!HMIsEnabled(pVM))
            VMCPU_FF_SET(pVCpu, VMCPU_FF_TRPM_SYNC_IDT);
#endif
    }

    if (    pCtx->ldtr.Sel      != pVM->rem.s.Env.ldt.selector
        ||  pCtx->ldtr.ValidSel != pVM->rem.s.Env.ldt.selector
        ||  pCtx->ldtr.u64Base  != pVM->rem.s.Env.ldt.base
        ||  pCtx->ldtr.u32Limit != pVM->rem.s.Env.ldt.limit
        ||  pCtx->ldtr.Attr.u   != ((pVM->rem.s.Env.ldt.flags >> SEL_FLAGS_SHIFT) & SEL_FLAGS_SMASK)
        ||  !(pCtx->ldtr.fFlags & CPUMSELREG_FLAGS_VALID)
       )
    {
        pCtx->ldtr.Sel      = pVM->rem.s.Env.ldt.selector;
        pCtx->ldtr.ValidSel = pVM->rem.s.Env.ldt.selector;
        pCtx->ldtr.fFlags   = CPUMSELREG_FLAGS_VALID;
        pCtx->ldtr.u64Base  = pVM->rem.s.Env.ldt.base;
        pCtx->ldtr.u32Limit = pVM->rem.s.Env.ldt.limit;
        pCtx->ldtr.Attr.u   = (pVM->rem.s.Env.ldt.flags >> SEL_FLAGS_SHIFT) & SEL_FLAGS_SMASK;
        STAM_COUNTER_INC(&gStatREMLDTRChange);
#ifdef VBOX_WITH_RAW_MODE
        if (!HMIsEnabled(pVM))
            VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_LDT);
#endif
    }

    if (    pCtx->tr.Sel      != pVM->rem.s.Env.tr.selector
        ||  pCtx->tr.ValidSel != pVM->rem.s.Env.tr.selector
        ||  pCtx->tr.u64Base  != pVM->rem.s.Env.tr.base
        ||  pCtx->tr.u32Limit != pVM->rem.s.Env.tr.limit
        ||  pCtx->tr.Attr.u   != ((pVM->rem.s.Env.tr.flags >> SEL_FLAGS_SHIFT) & SEL_FLAGS_SMASK)
        ||  !(pCtx->tr.fFlags & CPUMSELREG_FLAGS_VALID)
       )
    {
        Log(("REM: TR changed! %#x{%#llx,%#x,%#x} -> %#x{%llx,%#x,%#x}\n",
             pCtx->tr.Sel, pCtx->tr.u64Base, pCtx->tr.u32Limit, pCtx->tr.Attr.u,
             pVM->rem.s.Env.tr.selector, (uint64_t)pVM->rem.s.Env.tr.base, pVM->rem.s.Env.tr.limit,
             pVM->rem.s.Env.tr.flags >> SEL_FLAGS_SHIFT));
        pCtx->tr.Sel        = pVM->rem.s.Env.tr.selector;
        pCtx->tr.ValidSel   = pVM->rem.s.Env.tr.selector;
        pCtx->tr.fFlags     = CPUMSELREG_FLAGS_VALID;
        pCtx->tr.u64Base    = pVM->rem.s.Env.tr.base;
        pCtx->tr.u32Limit   = pVM->rem.s.Env.tr.limit;
        pCtx->tr.Attr.u     = (pVM->rem.s.Env.tr.flags >> SEL_FLAGS_SHIFT) & SEL_FLAGS_SMASK;
        Assert(pCtx->tr.Attr.u & ~DESC_INTEL_UNUSABLE);
        STAM_COUNTER_INC(&gStatREMTRChange);
#ifdef VBOX_WITH_RAW_MODE
        if (!HMIsEnabled(pVM))
            VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_TSS);
#endif
    }

    /* Sysenter MSR */
    pCtx->SysEnter.cs      = pVM->rem.s.Env.sysenter_cs;
    pCtx->SysEnter.eip     = pVM->rem.s.Env.sysenter_eip;
    pCtx->SysEnter.esp     = pVM->rem.s.Env.sysenter_esp;

    /* System MSRs. */
    pCtx->msrEFER          = pVM->rem.s.Env.efer;
    pCtx->msrSTAR          = pVM->rem.s.Env.star;
    pCtx->msrPAT           = pVM->rem.s.Env.pat;
#ifdef TARGET_X86_64
    pCtx->msrLSTAR         = pVM->rem.s.Env.lstar;
    pCtx->msrCSTAR         = pVM->rem.s.Env.cstar;
    pCtx->msrSFMASK        = pVM->rem.s.Env.fmask;
    pCtx->msrKERNELGSBASE  = pVM->rem.s.Env.kernelgsbase;
#endif

    /* Inhibit interrupt flag. */
    if (pVM->rem.s.Env.hflags & HF_INHIBIT_IRQ_MASK)
    {
        Log(("Settings VMCPU_FF_INHIBIT_INTERRUPTS at %RGv (REM)\n", (RTGCPTR)pCtx->rip));
        EMSetInhibitInterruptsPC(pVCpu, pCtx->rip);
        VMCPU_FF_SET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS);
    }
    else if (VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS))
    {
        Log(("Clearing VMCPU_FF_INHIBIT_INTERRUPTS at %RGv - successor %RGv (REM#2)\n", (RTGCPTR)pCtx->rip, EMGetInhibitInterruptsPC(pVCpu)));
        VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS);
    }

    /* Inhibit NMI flag. */
    if (pVM->rem.s.Env.hflags2 & HF2_NMI_MASK)
    {
        Log(("Settings VMCPU_FF_BLOCK_NMIS at %RGv (REM)\n", (RTGCPTR)pCtx->rip));
        VMCPU_FF_SET(pVCpu, VMCPU_FF_BLOCK_NMIS);
    }
    else if (VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_BLOCK_NMIS))
    {
        Log(("Clearing VMCPU_FF_BLOCK_NMIS at %RGv (REM)\n", (RTGCPTR)pCtx->rip));
        VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_BLOCK_NMIS);
    }

    remR3TrapClear(pVM);

    /*
     * Check for traps.
     */
    if (    pVM->rem.s.Env.exception_index >= 0
        &&  pVM->rem.s.Env.exception_index < 256)
    {
        /* This cannot be a hardware-interrupt because exception_index < EXCP_INTERRUPT. */
        int rc;

        Log(("REMR3StateBack: Pending trap %x %d\n", pVM->rem.s.Env.exception_index, pVM->rem.s.Env.exception_is_int));
        TRPMEVENT enmType = pVM->rem.s.Env.exception_is_int == 0                                    ? TRPM_TRAP
                          : pVM->rem.s.Env.exception_is_int == EXCEPTION_IS_INT_VALUE_HARDWARE_IRQ  ? TRPM_HARDWARE_INT
                          :                                                                           TRPM_SOFTWARE_INT;
        rc = TRPMAssertTrap(pVCpu, pVM->rem.s.Env.exception_index, enmType);
        AssertRC(rc);
        if (enmType == TRPM_TRAP)
        {
            switch (pVM->rem.s.Env.exception_index)
            {
                case X86_XCPT_PF:
                    TRPMSetFaultAddress(pVCpu, pCtx->cr2);
                    /* fallthru */
                case X86_XCPT_TS: case X86_XCPT_NP: case X86_XCPT_SS: case X86_XCPT_GP:
                case X86_XCPT_AC: case X86_XCPT_DF: /* 0 */
                    TRPMSetErrorCode(pVCpu, pVM->rem.s.Env.error_code);
                    break;
            }
        }
    }

    /*
     * We're not longer in REM mode.
     */
    CPUMR3RemLeave(pVCpu,
                      HMIsEnabled(pVM)
                   || (  pVM->rem.s.Env.segs[R_SS].newselector
                       | pVM->rem.s.Env.segs[R_GS].newselector
                       | pVM->rem.s.Env.segs[R_FS].newselector
                       | pVM->rem.s.Env.segs[R_ES].newselector
                       | pVM->rem.s.Env.segs[R_DS].newselector
                       | pVM->rem.s.Env.segs[R_CS].newselector) == 0
                   );
    VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED, VMCPUSTATE_STARTED_EXEC_REM);
    pVM->rem.s.fInREM    = false;
    pVM->rem.s.pCtx      = NULL;
    pVM->rem.s.Env.pVCpu = NULL;
    STAM_PROFILE_STOP(&pVM->rem.s.StatsStateBack, a);
    Log2(("REMR3StateBack: returns VINF_SUCCESS\n"));
    return VINF_SUCCESS;
}


/**
 * This is called by the disassembler when it wants to update the cpu state
 * before for instance doing a register dump.
 */
static void remR3StateUpdate(PVM pVM, PVMCPU pVCpu)
{
    register PCPUMCTX pCtx = pVM->rem.s.pCtx;
    unsigned          i;

    Assert(pVM->rem.s.fInREM);

    /*
     * Copy back the registers.
     * This is done in the order they are declared in the CPUMCTX structure.
     */

    PX86FXSTATE pFpuCtx = &pCtx->pXStateR3->x87;
    /** @todo FOP */
    /** @todo FPUIP */
    /** @todo CS */
    /** @todo FPUDP */
    /** @todo DS */
    /** @todo Fix MXCSR support in QEMU so we don't overwrite MXCSR with 0 when we shouldn't! */
    pFpuCtx->MXCSR       = 0;
    pFpuCtx->MXCSR_MASK  = 0;

    /** @todo check if FPU/XMM was actually used in the recompiler */
    restore_raw_fp_state(&pVM->rem.s.Env, (uint8_t *)pFpuCtx);
////    dprintf2(("FPU state CW=%04X TT=%04X SW=%04X (%04X)\n", env->fpuc, env->fpstt, env->fpus, pVMCtx->fpu.FSW));

#ifdef TARGET_X86_64
    pCtx->rdi           = pVM->rem.s.Env.regs[R_EDI];
    pCtx->rsi           = pVM->rem.s.Env.regs[R_ESI];
    pCtx->rbp           = pVM->rem.s.Env.regs[R_EBP];
    pCtx->rax           = pVM->rem.s.Env.regs[R_EAX];
    pCtx->rbx           = pVM->rem.s.Env.regs[R_EBX];
    pCtx->rdx           = pVM->rem.s.Env.regs[R_EDX];
    pCtx->rcx           = pVM->rem.s.Env.regs[R_ECX];
    pCtx->r8            = pVM->rem.s.Env.regs[8];
    pCtx->r9            = pVM->rem.s.Env.regs[9];
    pCtx->r10           = pVM->rem.s.Env.regs[10];
    pCtx->r11           = pVM->rem.s.Env.regs[11];
    pCtx->r12           = pVM->rem.s.Env.regs[12];
    pCtx->r13           = pVM->rem.s.Env.regs[13];
    pCtx->r14           = pVM->rem.s.Env.regs[14];
    pCtx->r15           = pVM->rem.s.Env.regs[15];

    pCtx->rsp           = pVM->rem.s.Env.regs[R_ESP];
#else
    pCtx->edi           = pVM->rem.s.Env.regs[R_EDI];
    pCtx->esi           = pVM->rem.s.Env.regs[R_ESI];
    pCtx->ebp           = pVM->rem.s.Env.regs[R_EBP];
    pCtx->eax           = pVM->rem.s.Env.regs[R_EAX];
    pCtx->ebx           = pVM->rem.s.Env.regs[R_EBX];
    pCtx->edx           = pVM->rem.s.Env.regs[R_EDX];
    pCtx->ecx           = pVM->rem.s.Env.regs[R_ECX];

    pCtx->esp           = pVM->rem.s.Env.regs[R_ESP];
#endif

    SYNC_BACK_SREG(es, ES);
    SYNC_BACK_SREG(cs, CS);
    SYNC_BACK_SREG(ss, SS);
    SYNC_BACK_SREG(ds, DS);
    SYNC_BACK_SREG(fs, FS);
    SYNC_BACK_SREG(gs, GS);

#ifdef TARGET_X86_64
    pCtx->rip           = pVM->rem.s.Env.eip;
    pCtx->rflags.u64    = pVM->rem.s.Env.eflags;
#else
    pCtx->eip           = pVM->rem.s.Env.eip;
    pCtx->eflags.u32    = pVM->rem.s.Env.eflags;
#endif

    pCtx->cr0           = pVM->rem.s.Env.cr[0];
    pCtx->cr2           = pVM->rem.s.Env.cr[2];
    pCtx->cr3           = pVM->rem.s.Env.cr[3];
#ifdef VBOX_WITH_RAW_MODE
    if (((pVM->rem.s.Env.cr[4] ^ pCtx->cr4) & X86_CR4_VME) && !HMIsEnabled(pVM))
        VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_TSS);
#endif
    pCtx->cr4           = pVM->rem.s.Env.cr[4];

    for (i = 0; i < 8; i++)
        pCtx->dr[i] = pVM->rem.s.Env.dr[i];

    pCtx->gdtr.cbGdt    = pVM->rem.s.Env.gdt.limit;
    if (pCtx->gdtr.pGdt != (RTGCPTR)pVM->rem.s.Env.gdt.base)
    {
        pCtx->gdtr.pGdt     = (RTGCPTR)pVM->rem.s.Env.gdt.base;
        STAM_COUNTER_INC(&gStatREMGDTChange);
#ifdef VBOX_WITH_RAW_MODE
        if (!HMIsEnabled(pVM))
            VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_GDT);
#endif
    }

    pCtx->idtr.cbIdt    = pVM->rem.s.Env.idt.limit;
    if (pCtx->idtr.pIdt != (RTGCPTR)pVM->rem.s.Env.idt.base)
    {
        pCtx->idtr.pIdt     = (RTGCPTR)pVM->rem.s.Env.idt.base;
        STAM_COUNTER_INC(&gStatREMIDTChange);
#ifdef VBOX_WITH_RAW_MODE
        if (!HMIsEnabled(pVM))
            VMCPU_FF_SET(pVCpu, VMCPU_FF_TRPM_SYNC_IDT);
#endif
    }

    if (    pCtx->ldtr.Sel      != pVM->rem.s.Env.ldt.selector
        ||  pCtx->ldtr.ValidSel != pVM->rem.s.Env.ldt.selector
        ||  pCtx->ldtr.u64Base  != pVM->rem.s.Env.ldt.base
        ||  pCtx->ldtr.u32Limit != pVM->rem.s.Env.ldt.limit
        ||  pCtx->ldtr.Attr.u   != ((pVM->rem.s.Env.ldt.flags >> SEL_FLAGS_SHIFT) & SEL_FLAGS_SMASK)
        ||  !(pCtx->ldtr.fFlags & CPUMSELREG_FLAGS_VALID)
       )
    {
        pCtx->ldtr.Sel      = pVM->rem.s.Env.ldt.selector;
        pCtx->ldtr.ValidSel = pVM->rem.s.Env.ldt.selector;
        pCtx->ldtr.fFlags   = CPUMSELREG_FLAGS_VALID;
        pCtx->ldtr.u64Base  = pVM->rem.s.Env.ldt.base;
        pCtx->ldtr.u32Limit = pVM->rem.s.Env.ldt.limit;
        pCtx->ldtr.Attr.u   = (pVM->rem.s.Env.ldt.flags >> SEL_FLAGS_SHIFT) & SEL_FLAGS_SMASK;
        STAM_COUNTER_INC(&gStatREMLDTRChange);
#ifdef VBOX_WITH_RAW_MODE
        if (!HMIsEnabled(pVM))
            VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_LDT);
#endif
    }

    if (    pCtx->tr.Sel      != pVM->rem.s.Env.tr.selector
        ||  pCtx->tr.ValidSel != pVM->rem.s.Env.tr.selector
        ||  pCtx->tr.u64Base  != pVM->rem.s.Env.tr.base
        ||  pCtx->tr.u32Limit != pVM->rem.s.Env.tr.limit
        ||  pCtx->tr.Attr.u   != ((pVM->rem.s.Env.tr.flags >> SEL_FLAGS_SHIFT) & SEL_FLAGS_SMASK)
        ||  !(pCtx->tr.fFlags & CPUMSELREG_FLAGS_VALID)
       )
    {
        Log(("REM: TR changed! %#x{%#llx,%#x,%#x} -> %#x{%llx,%#x,%#x}\n",
             pCtx->tr.Sel, pCtx->tr.u64Base, pCtx->tr.u32Limit, pCtx->tr.Attr.u,
             pVM->rem.s.Env.tr.selector, (uint64_t)pVM->rem.s.Env.tr.base, pVM->rem.s.Env.tr.limit,
             pVM->rem.s.Env.tr.flags >> SEL_FLAGS_SHIFT));
        pCtx->tr.Sel        = pVM->rem.s.Env.tr.selector;
        pCtx->tr.ValidSel   = pVM->rem.s.Env.tr.selector;
        pCtx->tr.fFlags     = CPUMSELREG_FLAGS_VALID;
        pCtx->tr.u64Base    = pVM->rem.s.Env.tr.base;
        pCtx->tr.u32Limit   = pVM->rem.s.Env.tr.limit;
        pCtx->tr.Attr.u     = (pVM->rem.s.Env.tr.flags >> SEL_FLAGS_SHIFT) & SEL_FLAGS_SMASK;
        Assert(pCtx->tr.Attr.u & ~DESC_INTEL_UNUSABLE);
        STAM_COUNTER_INC(&gStatREMTRChange);
#ifdef VBOX_WITH_RAW_MODE
        if (!HMIsEnabled(pVM))
            VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_TSS);
#endif
    }

    /* Sysenter MSR */
    pCtx->SysEnter.cs      = pVM->rem.s.Env.sysenter_cs;
    pCtx->SysEnter.eip     = pVM->rem.s.Env.sysenter_eip;
    pCtx->SysEnter.esp     = pVM->rem.s.Env.sysenter_esp;

    /* System MSRs. */
    pCtx->msrEFER          = pVM->rem.s.Env.efer;
    pCtx->msrSTAR          = pVM->rem.s.Env.star;
    pCtx->msrPAT           = pVM->rem.s.Env.pat;
#ifdef TARGET_X86_64
    pCtx->msrLSTAR         = pVM->rem.s.Env.lstar;
    pCtx->msrCSTAR         = pVM->rem.s.Env.cstar;
    pCtx->msrSFMASK        = pVM->rem.s.Env.fmask;
    pCtx->msrKERNELGSBASE  = pVM->rem.s.Env.kernelgsbase;
#endif

}


/**
 * Update the VMM state information if we're currently in REM.
 *
 * This method is used by the DBGF and PDMDevice when there is any uncertainty of whether
 * we're currently executing in REM and the VMM state is invalid. This method will of
 * course check that we're executing in REM before syncing any data over to the VMM.
 *
 * @param   pVM         The VM handle.
 * @param   pVCpu       The VMCPU handle.
 */
REMR3DECL(void) REMR3StateUpdate(PVM pVM, PVMCPU pVCpu)
{
    if (pVM->rem.s.fInREM)
        remR3StateUpdate(pVM, pVCpu);
}


#undef LOG_GROUP
#define LOG_GROUP LOG_GROUP_REM


/**
 * Notify the recompiler about Address Gate 20 state change.
 *
 * This notification is required since A20 gate changes are
 * initialized from a device driver and the VM might just as
 * well be in REM mode as in RAW mode.
 *
 * @param   pVM         VM handle.
 * @param   pVCpu       VMCPU handle.
 * @param   fEnable     True if the gate should be enabled.
 *                      False if the gate should be disabled.
 */
REMR3DECL(void) REMR3A20Set(PVM pVM, PVMCPU pVCpu, bool fEnable)
{
    LogFlow(("REMR3A20Set: fEnable=%d\n", fEnable));
    VM_ASSERT_EMT(pVM);

    /** @todo SMP and the A20 gate... */
    if (pVM->rem.s.Env.pVCpu == pVCpu)
    {
        ASMAtomicIncU32(&pVM->rem.s.cIgnoreAll);
        cpu_x86_set_a20(&pVM->rem.s.Env, fEnable);
        ASMAtomicDecU32(&pVM->rem.s.cIgnoreAll);
    }
}


/**
 * Replays the handler notification changes
 * Called in response to VM_FF_REM_HANDLER_NOTIFY from the RAW execution loop.
 *
 * @param   pVM         VM handle.
 */
REMR3DECL(void) REMR3ReplayHandlerNotifications(PVM pVM)
{
    /*
     * Replay the flushes.
     */
    LogFlow(("REMR3ReplayHandlerNotifications:\n"));
    VM_ASSERT_EMT(pVM);

    /** @todo this isn't ensuring correct replay order. */
    if (VM_FF_TEST_AND_CLEAR(pVM, VM_FF_REM_HANDLER_NOTIFY))
    {
        uint32_t    idxNext;
        uint32_t    idxRevHead;
        uint32_t    idxHead;
#ifdef VBOX_STRICT
        int32_t     c = 0;
#endif

        /* Lockless purging of pending notifications. */
        idxHead = ASMAtomicXchgU32(&pVM->rem.s.idxPendingList, UINT32_MAX);
        if (idxHead == UINT32_MAX)
            return;
        Assert(idxHead < RT_ELEMENTS(pVM->rem.s.aHandlerNotifications));

        /*
         * Reverse the list to process it in FIFO order.
         */
        idxRevHead = UINT32_MAX;
        do
        {
            /* Save the index of the next rec. */
            idxNext    = pVM->rem.s.aHandlerNotifications[idxHead].idxNext;
            Assert(idxNext < RT_ELEMENTS(pVM->rem.s.aHandlerNotifications) || idxNext == UINT32_MAX);
            /* Push the record onto the reversed list. */
            pVM->rem.s.aHandlerNotifications[idxHead].idxNext = idxRevHead;
            idxRevHead = idxHead;
            Assert(++c <= RT_ELEMENTS(pVM->rem.s.aHandlerNotifications));
            /* Advance. */
            idxHead    = idxNext;
        } while (idxHead != UINT32_MAX);

        /*
         * Loop thru the list, reinserting the record into the free list as they are
         * processed to avoid having other EMTs running out of entries while we're flushing.
         */
        idxHead = idxRevHead;
        do
        {
            PREMHANDLERNOTIFICATION pCur = &pVM->rem.s.aHandlerNotifications[idxHead];
            uint32_t                idxCur;
            Assert(--c >= 0);

            switch (pCur->enmKind)
            {
                case REMHANDLERNOTIFICATIONKIND_PHYSICAL_REGISTER:
                    remR3NotifyHandlerPhysicalRegister(pVM,
                                                       pCur->u.PhysicalRegister.enmKind,
                                                       pCur->u.PhysicalRegister.GCPhys,
                                                       pCur->u.PhysicalRegister.cb,
                                                       pCur->u.PhysicalRegister.fHasHCHandler);
                    break;

                case REMHANDLERNOTIFICATIONKIND_PHYSICAL_DEREGISTER:
                    remR3NotifyHandlerPhysicalDeregister(pVM,
                                                         pCur->u.PhysicalDeregister.enmKind,
                                                         pCur->u.PhysicalDeregister.GCPhys,
                                                         pCur->u.PhysicalDeregister.cb,
                                                         pCur->u.PhysicalDeregister.fHasHCHandler,
                                                         pCur->u.PhysicalDeregister.fRestoreAsRAM);
                    break;

                case REMHANDLERNOTIFICATIONKIND_PHYSICAL_MODIFY:
                    remR3NotifyHandlerPhysicalModify(pVM,
                                                     pCur->u.PhysicalModify.enmKind,
                                                     pCur->u.PhysicalModify.GCPhysOld,
                                                     pCur->u.PhysicalModify.GCPhysNew,
                                                     pCur->u.PhysicalModify.cb,
                                                     pCur->u.PhysicalModify.fHasHCHandler,
                                                     pCur->u.PhysicalModify.fRestoreAsRAM);
                    break;

                default:
                    AssertReleaseMsgFailed(("enmKind=%d\n", pCur->enmKind));
                    break;
            }

            /*
             * Advance idxHead.
             */
            idxCur  = idxHead;
            idxHead = pCur->idxNext;
            Assert(idxHead < RT_ELEMENTS(pVM->rem.s.aHandlerNotifications) || (idxHead == UINT32_MAX && c == 0));

            /*
             * Put the record back into the free list.
             */
            do
            {
                idxNext = ASMAtomicUoReadU32(&pVM->rem.s.idxFreeList);
                ASMAtomicWriteU32(&pCur->idxNext, idxNext);
                ASMCompilerBarrier();
            } while (!ASMAtomicCmpXchgU32(&pVM->rem.s.idxFreeList, idxCur, idxNext));
        } while (idxHead != UINT32_MAX);

#ifdef VBOX_STRICT
        if (pVM->cCpus == 1)
        {
            unsigned c;
            /* Check that all records are now on the free list. */
            for (c = 0, idxNext = pVM->rem.s.idxFreeList; idxNext != UINT32_MAX;
                 idxNext = pVM->rem.s.aHandlerNotifications[idxNext].idxNext)
                c++;
            AssertReleaseMsg(c == RT_ELEMENTS(pVM->rem.s.aHandlerNotifications), ("%#x != %#x, idxFreeList=%#x\n", c, RT_ELEMENTS(pVM->rem.s.aHandlerNotifications), pVM->rem.s.idxFreeList));
        }
#endif
    }
}


/**
 * Notify REM about changed code page.
 *
 * @returns VBox status code.
 * @param   pVM         VM handle.
 * @param   pVCpu       VMCPU handle.
 * @param   pvCodePage  Code page address
 */
REMR3DECL(int) REMR3NotifyCodePageChanged(PVM pVM, PVMCPU pVCpu, RTGCPTR pvCodePage)
{
#ifdef VBOX_REM_PROTECT_PAGES_FROM_SMC
    int      rc;
    RTGCPHYS PhysGC;
    uint64_t flags;

    VM_ASSERT_EMT(pVM);

    /*
     * Get the physical page address.
     */
    rc = PGMGstGetPage(pVM, pvCodePage, &flags, &PhysGC);
    if (rc == VINF_SUCCESS)
    {
        /*
         * Sync the required registers and flush the whole page.
         * (Easier to do the whole page than notifying it about each physical
         * byte that was changed.
         */
        pVM->rem.s.Env.cr[0] = pVM->rem.s.pCtx->cr0;
        pVM->rem.s.Env.cr[2] = pVM->rem.s.pCtx->cr2;
        pVM->rem.s.Env.cr[3] = pVM->rem.s.pCtx->cr3;
        pVM->rem.s.Env.cr[4] = pVM->rem.s.pCtx->cr4;

        tb_invalidate_phys_page_range(PhysGC, PhysGC + PAGE_SIZE - 1, 0);
    }
#endif
    return VINF_SUCCESS;
}


/**
 * Notification about a successful MMR3PhysRegister() call.
 *
 * @param   pVM         VM handle.
 * @param   GCPhys      The physical address the RAM.
 * @param   cb          Size of the memory.
 * @param   fFlags      Flags of the REM_NOTIFY_PHYS_RAM_FLAGS_* defines.
 */
REMR3DECL(void) REMR3NotifyPhysRamRegister(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, unsigned fFlags)
{
    Log(("REMR3NotifyPhysRamRegister: GCPhys=%RGp cb=%RGp fFlags=%#x\n", GCPhys, cb, fFlags));
    VM_ASSERT_EMT(pVM);

    /*
     * Validate input - we trust the caller.
     */
    Assert(RT_ALIGN_T(GCPhys, PAGE_SIZE, RTGCPHYS) == GCPhys);
    Assert(cb);
    Assert(RT_ALIGN_Z(cb, PAGE_SIZE) == cb);
    AssertMsg(fFlags == REM_NOTIFY_PHYS_RAM_FLAGS_RAM || fFlags == REM_NOTIFY_PHYS_RAM_FLAGS_MMIO2, ("%#x\n", fFlags));

    /*
     * Base ram? Update GCPhysLastRam.
     */
    if (fFlags & REM_NOTIFY_PHYS_RAM_FLAGS_RAM)
    {
        if (GCPhys + (cb - 1) > pVM->rem.s.GCPhysLastRam)
        {
            AssertReleaseMsg(!pVM->rem.s.fGCPhysLastRamFixed, ("GCPhys=%RGp cb=%RGp\n", GCPhys, cb));
            pVM->rem.s.GCPhysLastRam = GCPhys + (cb - 1);
        }
    }

    /*
     * Register the ram.
     */
    ASMAtomicIncU32(&pVM->rem.s.cIgnoreAll);

    PDMCritSectEnter(&pVM->rem.s.CritSectRegister, VERR_SEM_BUSY);
    cpu_register_physical_memory_offset(GCPhys, cb, GCPhys, GCPhys);
    PDMCritSectLeave(&pVM->rem.s.CritSectRegister);

    ASMAtomicDecU32(&pVM->rem.s.cIgnoreAll);
}


/**
 * Notification about a successful MMR3PhysRomRegister() call.
 *
 * @param   pVM         VM handle.
 * @param   GCPhys      The physical address of the ROM.
 * @param   cb          The size of the ROM.
 * @param   pvCopy      Pointer to the ROM copy.
 * @param   fShadow     Whether it's currently writable shadow ROM or normal readonly ROM.
 *                      This function will be called when ever the protection of the
 *                      shadow ROM changes (at reset and end of POST).
 */
REMR3DECL(void) REMR3NotifyPhysRomRegister(PVM pVM, RTGCPHYS GCPhys, RTUINT cb, void *pvCopy, bool fShadow)
{
    Log(("REMR3NotifyPhysRomRegister: GCPhys=%RGp cb=%d fShadow=%RTbool\n", GCPhys, cb, fShadow));
    VM_ASSERT_EMT(pVM);

    /*
     * Validate input - we trust the caller.
     */
    Assert(RT_ALIGN_T(GCPhys, PAGE_SIZE, RTGCPHYS) == GCPhys);
    Assert(cb);
    Assert(RT_ALIGN_Z(cb, PAGE_SIZE) == cb);

    /*
     * Register the rom.
     */
    ASMAtomicIncU32(&pVM->rem.s.cIgnoreAll);

    PDMCritSectEnter(&pVM->rem.s.CritSectRegister, VERR_SEM_BUSY);
    cpu_register_physical_memory_offset(GCPhys, cb, GCPhys | (fShadow ? 0 : IO_MEM_ROM), GCPhys);
    PDMCritSectLeave(&pVM->rem.s.CritSectRegister);

    ASMAtomicDecU32(&pVM->rem.s.cIgnoreAll);
}


/**
 * Notification about a successful memory deregistration or reservation.
 *
 * @param   pVM         VM Handle.
 * @param   GCPhys      Start physical address.
 * @param   cb          The size of the range.
 */
REMR3DECL(void) REMR3NotifyPhysRamDeregister(PVM pVM, RTGCPHYS GCPhys, RTUINT cb)
{
    Log(("REMR3NotifyPhysRamDeregister: GCPhys=%RGp cb=%d\n", GCPhys, cb));
    VM_ASSERT_EMT(pVM);

    /*
     * Validate input - we trust the caller.
     */
    Assert(RT_ALIGN_T(GCPhys, PAGE_SIZE, RTGCPHYS) == GCPhys);
    Assert(cb);
    Assert(RT_ALIGN_Z(cb, PAGE_SIZE) == cb);

    /*
     * Unassigning the memory.
     */
    ASMAtomicIncU32(&pVM->rem.s.cIgnoreAll);

    PDMCritSectEnter(&pVM->rem.s.CritSectRegister, VERR_SEM_BUSY);
    cpu_register_physical_memory_offset(GCPhys, cb, IO_MEM_UNASSIGNED, GCPhys);
    PDMCritSectLeave(&pVM->rem.s.CritSectRegister);

    ASMAtomicDecU32(&pVM->rem.s.cIgnoreAll);
}


/**
 * Notification about a successful PGMR3HandlerPhysicalRegister() call.
 *
 * @param   pVM             VM Handle.
 * @param   enmKind         Kind of access handler.
 * @param   GCPhys          Handler range address.
 * @param   cb              Size of the handler range.
 * @param   fHasHCHandler   Set if the handler has a HC callback function.
 *
 * @remark  MMR3PhysRomRegister assumes that this function will not apply the
 *          Handler memory type to memory which has no HC handler.
 */
static void remR3NotifyHandlerPhysicalRegister(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb,
                                               bool fHasHCHandler)
{
    Log(("REMR3NotifyHandlerPhysicalRegister: enmKind=%d GCPhys=%RGp cb=%RGp fHasHCHandler=%d\n",
         enmKind, GCPhys, cb, fHasHCHandler));

    VM_ASSERT_EMT(pVM);
    Assert(RT_ALIGN_T(GCPhys, PAGE_SIZE, RTGCPHYS) == GCPhys);
    Assert(RT_ALIGN_T(cb, PAGE_SIZE, RTGCPHYS) == cb);


    ASMAtomicIncU32(&pVM->rem.s.cIgnoreAll);

    PDMCritSectEnter(&pVM->rem.s.CritSectRegister, VERR_SEM_BUSY);
    if (enmKind == PGMPHYSHANDLERKIND_MMIO)
        cpu_register_physical_memory_offset(GCPhys, cb, pVM->rem.s.iMMIOMemType, GCPhys);
    else if (fHasHCHandler)
        cpu_register_physical_memory_offset(GCPhys, cb, pVM->rem.s.iHandlerMemType, GCPhys);
    PDMCritSectLeave(&pVM->rem.s.CritSectRegister);

    ASMAtomicDecU32(&pVM->rem.s.cIgnoreAll);
}

/**
 * Notification about a successful PGMR3HandlerPhysicalRegister() call.
 *
 * @param   pVM             VM Handle.
 * @param   enmKind         Kind of access handler.
 * @param   GCPhys          Handler range address.
 * @param   cb              Size of the handler range.
 * @param   fHasHCHandler   Set if the handler has a HC callback function.
 *
 * @remark  MMR3PhysRomRegister assumes that this function will not apply the
 *          Handler memory type to memory which has no HC handler.
 */
REMR3DECL(void) REMR3NotifyHandlerPhysicalRegister(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb,
                                                   bool fHasHCHandler)
{
    REMR3ReplayHandlerNotifications(pVM);

    remR3NotifyHandlerPhysicalRegister(pVM, enmKind, GCPhys, cb, fHasHCHandler);
}

/**
 * Notification about a successful PGMR3HandlerPhysicalDeregister() operation.
 *
 * @param   pVM             VM Handle.
 * @param   enmKind         Kind of access handler.
 * @param   GCPhys          Handler range address.
 * @param   cb              Size of the handler range.
 * @param   fHasHCHandler   Set if the handler has a HC callback function.
 * @param   fRestoreAsRAM   Whether the to restore it as normal RAM or as unassigned memory.
 */
static void remR3NotifyHandlerPhysicalDeregister(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb,
                                                 bool fHasHCHandler, bool fRestoreAsRAM)
{
    Log(("REMR3NotifyHandlerPhysicalDeregister: enmKind=%d GCPhys=%RGp cb=%RGp fHasHCHandler=%RTbool fRestoreAsRAM=%RTbool RAM=%08x\n",
         enmKind, GCPhys, cb, fHasHCHandler, fRestoreAsRAM, MMR3PhysGetRamSize(pVM)));
    VM_ASSERT_EMT(pVM);


    ASMAtomicIncU32(&pVM->rem.s.cIgnoreAll);

    PDMCritSectEnter(&pVM->rem.s.CritSectRegister, VERR_SEM_BUSY);
    /** @todo this isn't right, MMIO can (in theory) be restored as RAM. */
    if (enmKind == PGMPHYSHANDLERKIND_MMIO)
        cpu_register_physical_memory_offset(GCPhys, cb, IO_MEM_UNASSIGNED, GCPhys);
    else if (fHasHCHandler)
    {
        if (!fRestoreAsRAM)
        {
            Assert(GCPhys > MMR3PhysGetRamSize(pVM));
            cpu_register_physical_memory_offset(GCPhys, cb, IO_MEM_UNASSIGNED, GCPhys);
        }
        else
        {
            Assert(RT_ALIGN_T(GCPhys, PAGE_SIZE, RTGCPHYS) == GCPhys);
            Assert(RT_ALIGN_T(cb, PAGE_SIZE, RTGCPHYS) == cb);
            cpu_register_physical_memory_offset(GCPhys, cb, GCPhys, GCPhys);
        }
    }
    PDMCritSectLeave(&pVM->rem.s.CritSectRegister);

    ASMAtomicDecU32(&pVM->rem.s.cIgnoreAll);
}

/**
 * Notification about a successful PGMR3HandlerPhysicalDeregister() operation.
 *
 * @param   pVM             VM Handle.
 * @param   enmKind         Kind of access handler.
 * @param   GCPhys          Handler range address.
 * @param   cb              Size of the handler range.
 * @param   fHasHCHandler   Set if the handler has a HC callback function.
 * @param   fRestoreAsRAM   Whether the to restore it as normal RAM or as unassigned memory.
 */
REMR3DECL(void) REMR3NotifyHandlerPhysicalDeregister(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb, bool fHasHCHandler, bool fRestoreAsRAM)
{
    REMR3ReplayHandlerNotifications(pVM);
    remR3NotifyHandlerPhysicalDeregister(pVM, enmKind, GCPhys, cb, fHasHCHandler, fRestoreAsRAM);
}


/**
 * Notification about a successful PGMR3HandlerPhysicalModify() call.
 *
 * @param   pVM             VM Handle.
 * @param   enmKind         Kind of access handler.
 * @param   GCPhysOld       Old handler range address.
 * @param   GCPhysNew       New handler range address.
 * @param   cb              Size of the handler range.
 * @param   fHasHCHandler   Set if the handler has a HC callback function.
 * @param   fRestoreAsRAM   Whether the to restore it as normal RAM or as unassigned memory.
 */
static void remR3NotifyHandlerPhysicalModify(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhysOld, RTGCPHYS GCPhysNew, RTGCPHYS cb, bool fHasHCHandler, bool fRestoreAsRAM)
{
    Log(("REMR3NotifyHandlerPhysicalModify: enmKind=%d GCPhysOld=%RGp GCPhysNew=%RGp cb=%RGp fHasHCHandler=%RTbool fRestoreAsRAM=%RTbool\n",
         enmKind, GCPhysOld, GCPhysNew, cb, fHasHCHandler, fRestoreAsRAM));
    VM_ASSERT_EMT(pVM);
    AssertReleaseMsg(enmKind != PGMPHYSHANDLERKIND_MMIO, ("enmKind=%d\n", enmKind));

    if (fHasHCHandler)
    {
        ASMAtomicIncU32(&pVM->rem.s.cIgnoreAll);

        /*
         * Reset the old page.
         */
        PDMCritSectEnter(&pVM->rem.s.CritSectRegister, VERR_SEM_BUSY);
        if (!fRestoreAsRAM)
            cpu_register_physical_memory_offset(GCPhysOld, cb, IO_MEM_UNASSIGNED, GCPhysOld);
        else
        {
            /* This is not perfect, but it'll do for PD monitoring... */
            Assert(cb == PAGE_SIZE);
            Assert(RT_ALIGN_T(GCPhysOld, PAGE_SIZE, RTGCPHYS) == GCPhysOld);
            cpu_register_physical_memory_offset(GCPhysOld, cb, GCPhysOld, GCPhysOld);
        }

        /*
         * Update the new page.
         */
        Assert(RT_ALIGN_T(GCPhysNew, PAGE_SIZE, RTGCPHYS) == GCPhysNew);
        Assert(RT_ALIGN_T(cb, PAGE_SIZE, RTGCPHYS) == cb);
        cpu_register_physical_memory_offset(GCPhysNew, cb, pVM->rem.s.iHandlerMemType, GCPhysNew);
        PDMCritSectLeave(&pVM->rem.s.CritSectRegister);

        ASMAtomicDecU32(&pVM->rem.s.cIgnoreAll);
    }
}

/**
 * Notification about a successful PGMR3HandlerPhysicalModify() call.
 *
 * @param   pVM             VM Handle.
 * @param   enmKind         Kind of access handler.
 * @param   GCPhysOld       Old handler range address.
 * @param   GCPhysNew       New handler range address.
 * @param   cb              Size of the handler range.
 * @param   fHasHCHandler   Set if the handler has a HC callback function.
 * @param   fRestoreAsRAM   Whether the to restore it as normal RAM or as unassigned memory.
 */
REMR3DECL(void) REMR3NotifyHandlerPhysicalModify(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhysOld, RTGCPHYS GCPhysNew, RTGCPHYS cb, bool fHasHCHandler, bool fRestoreAsRAM)
{
    REMR3ReplayHandlerNotifications(pVM);

    remR3NotifyHandlerPhysicalModify(pVM, enmKind, GCPhysOld, GCPhysNew, cb, fHasHCHandler, fRestoreAsRAM);
}

/**
 * Checks if we're handling access to this page or not.
 *
 * @returns true if we're trapping access.
 * @returns false if we aren't.
 * @param   pVM         The VM handle.
 * @param   GCPhys      The physical address.
 *
 * @remark  This function will only work correctly in VBOX_STRICT builds!
 */
REMR3DECL(bool) REMR3IsPageAccessHandled(PVM pVM, RTGCPHYS GCPhys)
{
#ifdef VBOX_STRICT
    ram_addr_t off;
    REMR3ReplayHandlerNotifications(pVM);

    off = get_phys_page_offset(GCPhys);
    return (off & PAGE_OFFSET_MASK) == pVM->rem.s.iHandlerMemType
        || (off & PAGE_OFFSET_MASK) == pVM->rem.s.iMMIOMemType
        || (off & PAGE_OFFSET_MASK) == IO_MEM_ROM;
#else
    return false;
#endif
}


/**
 * Deals with a rare case in get_phys_addr_code where the code
 * is being monitored.
 *
 * It could also be an MMIO page, in which case we will raise a fatal error.
 *
 * @returns The physical address corresponding to addr.
 * @param   env         The cpu environment.
 * @param   addr        The virtual address.
 * @param   pTLBEntry   The TLB entry.
 * @param   IoTlbEntry  The I/O TLB entry address.
 */
target_ulong remR3PhysGetPhysicalAddressCode(CPUX86State       *env,
                                             target_ulong       addr,
                                             CPUTLBEntry       *pTLBEntry,
                                             target_phys_addr_t IoTlbEntry)
{
    PVM pVM = env->pVM;

    if ((IoTlbEntry & ~TARGET_PAGE_MASK) == pVM->rem.s.iHandlerMemType)
    {
        /* If code memory is being monitored, appropriate IOTLB entry will have
           handler IO type, and addend will provide real physical address, no
           matter if we store VA in TLB or not, as handlers are always passed PA */
        target_ulong ret = (IoTlbEntry & TARGET_PAGE_MASK) + addr;
        return ret;
    }
    LogRel(("\nTrying to execute code with memory type addr_code=%RGv addend=%RGp at %RGv! (iHandlerMemType=%#x iMMIOMemType=%#x IOTLB=%RGp)\n"
            "*** handlers\n",
            (RTGCPTR)pTLBEntry->addr_code, (RTGCPHYS)pTLBEntry->addend, (RTGCPTR)addr, pVM->rem.s.iHandlerMemType, pVM->rem.s.iMMIOMemType, (RTGCPHYS)IoTlbEntry));
    DBGFR3Info(pVM->pUVM, "handlers", NULL, DBGFR3InfoLogRelHlp());
    LogRel(("*** mmio\n"));
    DBGFR3Info(pVM->pUVM, "mmio", NULL, DBGFR3InfoLogRelHlp());
    LogRel(("*** phys\n"));
    DBGFR3Info(pVM->pUVM, "phys", NULL, DBGFR3InfoLogRelHlp());
    cpu_abort(env, "Trying to execute code with memory type addr_code=%RGv addend=%RGp at %RGv. (iHandlerMemType=%#x iMMIOMemType=%#x)\n",
              (RTGCPTR)pTLBEntry->addr_code, (RTGCPHYS)pTLBEntry->addend, (RTGCPTR)addr, pVM->rem.s.iHandlerMemType, pVM->rem.s.iMMIOMemType);
    AssertFatalFailed();
}

/**
 * Read guest RAM and ROM.
 *
 * @param   SrcGCPhys       The source address (guest physical).
 * @param   pvDst           The destination address.
 * @param   cb              Number of bytes
 */
void remR3PhysRead(RTGCPHYS SrcGCPhys, void *pvDst, unsigned cb)
{
    STAM_PROFILE_ADV_START(&gStatMemRead, a);
    VBOX_CHECK_ADDR(SrcGCPhys);
    VBOXSTRICTRC rcStrict = PGMPhysRead(cpu_single_env->pVM, SrcGCPhys, pvDst, cb, PGMACCESSORIGIN_REM);
    AssertMsg(rcStrict == VINF_SUCCESS, ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict))); NOREF(rcStrict);
#ifdef VBOX_DEBUG_PHYS
    LogRel(("read(%d): %08x\n", cb, (uint32_t)SrcGCPhys));
#endif
    STAM_PROFILE_ADV_STOP(&gStatMemRead, a);
}


/**
 * Read guest RAM and ROM, unsigned 8-bit.
 *
 * @param   SrcGCPhys       The source address (guest physical).
 */
RTCCUINTREG remR3PhysReadU8(RTGCPHYS SrcGCPhys)
{
    uint8_t val;
    STAM_PROFILE_ADV_START(&gStatMemRead, a);
    VBOX_CHECK_ADDR(SrcGCPhys);
    val = PGMR3PhysReadU8(cpu_single_env->pVM, SrcGCPhys, PGMACCESSORIGIN_REM);
    STAM_PROFILE_ADV_STOP(&gStatMemRead, a);
#ifdef VBOX_DEBUG_PHYS
    LogRel(("readu8: %x <- %08x\n", val, (uint32_t)SrcGCPhys));
#endif
    return val;
}


/**
 * Read guest RAM and ROM, signed 8-bit.
 *
 * @param   SrcGCPhys       The source address (guest physical).
 */
RTCCINTREG remR3PhysReadS8(RTGCPHYS SrcGCPhys)
{
    int8_t val;
    STAM_PROFILE_ADV_START(&gStatMemRead, a);
    VBOX_CHECK_ADDR(SrcGCPhys);
    val = PGMR3PhysReadU8(cpu_single_env->pVM, SrcGCPhys, PGMACCESSORIGIN_REM);
    STAM_PROFILE_ADV_STOP(&gStatMemRead, a);
#ifdef VBOX_DEBUG_PHYS
    LogRel(("reads8: %x <- %08x\n", val, (uint32_t)SrcGCPhys));
#endif
    return val;
}


/**
 * Read guest RAM and ROM, unsigned 16-bit.
 *
 * @param   SrcGCPhys       The source address (guest physical).
 */
RTCCUINTREG remR3PhysReadU16(RTGCPHYS SrcGCPhys)
{
    uint16_t val;
    STAM_PROFILE_ADV_START(&gStatMemRead, a);
    VBOX_CHECK_ADDR(SrcGCPhys);
    val = PGMR3PhysReadU16(cpu_single_env->pVM, SrcGCPhys, PGMACCESSORIGIN_REM);
    STAM_PROFILE_ADV_STOP(&gStatMemRead, a);
#ifdef VBOX_DEBUG_PHYS
    LogRel(("readu16: %x <- %08x\n", val, (uint32_t)SrcGCPhys));
#endif
    return val;
}


/**
 * Read guest RAM and ROM, signed 16-bit.
 *
 * @param   SrcGCPhys       The source address (guest physical).
 */
RTCCINTREG remR3PhysReadS16(RTGCPHYS SrcGCPhys)
{
    int16_t val;
    STAM_PROFILE_ADV_START(&gStatMemRead, a);
    VBOX_CHECK_ADDR(SrcGCPhys);
    val = PGMR3PhysReadU16(cpu_single_env->pVM, SrcGCPhys, PGMACCESSORIGIN_REM);
    STAM_PROFILE_ADV_STOP(&gStatMemRead, a);
#ifdef VBOX_DEBUG_PHYS
    LogRel(("reads16: %x <- %08x\n", (uint16_t)val, (uint32_t)SrcGCPhys));
#endif
    return val;
}


/**
 * Read guest RAM and ROM, unsigned 32-bit.
 *
 * @param   SrcGCPhys       The source address (guest physical).
 */
RTCCUINTREG remR3PhysReadU32(RTGCPHYS SrcGCPhys)
{
    uint32_t val;
    STAM_PROFILE_ADV_START(&gStatMemRead, a);
    VBOX_CHECK_ADDR(SrcGCPhys);
    val = PGMR3PhysReadU32(cpu_single_env->pVM, SrcGCPhys, PGMACCESSORIGIN_REM);
    STAM_PROFILE_ADV_STOP(&gStatMemRead, a);
#ifdef VBOX_DEBUG_PHYS
    LogRel(("readu32: %x <- %08x\n", val, (uint32_t)SrcGCPhys));
#endif
    return val;
}


/**
 * Read guest RAM and ROM, signed 32-bit.
 *
 * @param   SrcGCPhys       The source address (guest physical).
 */
RTCCINTREG remR3PhysReadS32(RTGCPHYS SrcGCPhys)
{
    int32_t val;
    STAM_PROFILE_ADV_START(&gStatMemRead, a);
    VBOX_CHECK_ADDR(SrcGCPhys);
    val = PGMR3PhysReadU32(cpu_single_env->pVM, SrcGCPhys, PGMACCESSORIGIN_REM);
    STAM_PROFILE_ADV_STOP(&gStatMemRead, a);
#ifdef VBOX_DEBUG_PHYS
    LogRel(("reads32: %x <- %08x\n", val, (uint32_t)SrcGCPhys));
#endif
    return val;
}


/**
 * Read guest RAM and ROM, unsigned 64-bit.
 *
 * @param   SrcGCPhys       The source address (guest physical).
 */
uint64_t remR3PhysReadU64(RTGCPHYS SrcGCPhys)
{
    uint64_t val;
    STAM_PROFILE_ADV_START(&gStatMemRead, a);
    VBOX_CHECK_ADDR(SrcGCPhys);
    val = PGMR3PhysReadU64(cpu_single_env->pVM, SrcGCPhys, PGMACCESSORIGIN_REM);
    STAM_PROFILE_ADV_STOP(&gStatMemRead, a);
#ifdef VBOX_DEBUG_PHYS
    LogRel(("readu64: %llx <- %08x\n", val, (uint32_t)SrcGCPhys));
#endif
    return val;
}


/**
 * Read guest RAM and ROM, signed 64-bit.
 *
 * @param   SrcGCPhys       The source address (guest physical).
 */
int64_t remR3PhysReadS64(RTGCPHYS SrcGCPhys)
{
    int64_t val;
    STAM_PROFILE_ADV_START(&gStatMemRead, a);
    VBOX_CHECK_ADDR(SrcGCPhys);
    val = PGMR3PhysReadU64(cpu_single_env->pVM, SrcGCPhys, PGMACCESSORIGIN_REM);
    STAM_PROFILE_ADV_STOP(&gStatMemRead, a);
#ifdef VBOX_DEBUG_PHYS
    LogRel(("reads64: %llx <- %08x\n", val, (uint32_t)SrcGCPhys));
#endif
    return val;
}


/**
 * Write guest RAM.
 *
 * @param   DstGCPhys       The destination address (guest physical).
 * @param   pvSrc           The source address.
 * @param   cb              Number of bytes to write
 */
void remR3PhysWrite(RTGCPHYS DstGCPhys, const void *pvSrc, unsigned cb)
{
    STAM_PROFILE_ADV_START(&gStatMemWrite, a);
    VBOX_CHECK_ADDR(DstGCPhys);
    VBOXSTRICTRC rcStrict = PGMPhysWrite(cpu_single_env->pVM, DstGCPhys, pvSrc, cb, PGMACCESSORIGIN_REM);
    AssertMsg(rcStrict == VINF_SUCCESS, ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict))); NOREF(rcStrict);
    STAM_PROFILE_ADV_STOP(&gStatMemWrite, a);
#ifdef VBOX_DEBUG_PHYS
    LogRel(("write(%d): %08x\n", cb, (uint32_t)DstGCPhys));
#endif
}


/**
 * Write guest RAM, unsigned 8-bit.
 *
 * @param   DstGCPhys       The destination address (guest physical).
 * @param   val             Value
 */
void remR3PhysWriteU8(RTGCPHYS DstGCPhys, uint8_t val)
{
    STAM_PROFILE_ADV_START(&gStatMemWrite, a);
    VBOX_CHECK_ADDR(DstGCPhys);
    PGMR3PhysWriteU8(cpu_single_env->pVM, DstGCPhys, val, PGMACCESSORIGIN_REM);
    STAM_PROFILE_ADV_STOP(&gStatMemWrite, a);
#ifdef VBOX_DEBUG_PHYS
    LogRel(("writeu8: %x -> %08x\n", val, (uint32_t)DstGCPhys));
#endif
}


/**
 * Write guest RAM, unsigned 8-bit.
 *
 * @param   DstGCPhys       The destination address (guest physical).
 * @param   val             Value
 */
void remR3PhysWriteU16(RTGCPHYS DstGCPhys, uint16_t val)
{
    STAM_PROFILE_ADV_START(&gStatMemWrite, a);
    VBOX_CHECK_ADDR(DstGCPhys);
    PGMR3PhysWriteU16(cpu_single_env->pVM, DstGCPhys, val, PGMACCESSORIGIN_REM);
    STAM_PROFILE_ADV_STOP(&gStatMemWrite, a);
#ifdef VBOX_DEBUG_PHYS
    LogRel(("writeu16: %x -> %08x\n", val, (uint32_t)DstGCPhys));
#endif
}


/**
 * Write guest RAM, unsigned 32-bit.
 *
 * @param   DstGCPhys       The destination address (guest physical).
 * @param   val             Value
 */
void remR3PhysWriteU32(RTGCPHYS DstGCPhys, uint32_t val)
{
    STAM_PROFILE_ADV_START(&gStatMemWrite, a);
    VBOX_CHECK_ADDR(DstGCPhys);
    PGMR3PhysWriteU32(cpu_single_env->pVM, DstGCPhys, val, PGMACCESSORIGIN_REM);
    STAM_PROFILE_ADV_STOP(&gStatMemWrite, a);
#ifdef VBOX_DEBUG_PHYS
    LogRel(("writeu32: %x -> %08x\n", val, (uint32_t)DstGCPhys));
#endif
}


/**
 * Write guest RAM, unsigned 64-bit.
 *
 * @param   DstGCPhys       The destination address (guest physical).
 * @param   val             Value
 */
void remR3PhysWriteU64(RTGCPHYS DstGCPhys, uint64_t val)
{
    STAM_PROFILE_ADV_START(&gStatMemWrite, a);
    VBOX_CHECK_ADDR(DstGCPhys);
    PGMR3PhysWriteU64(cpu_single_env->pVM, DstGCPhys, val, PGMACCESSORIGIN_REM);
    STAM_PROFILE_ADV_STOP(&gStatMemWrite, a);
#ifdef VBOX_DEBUG_PHYS
    LogRel(("writeu64: %llx -> %08x\n", val, (uint32_t)DstGCPhys));
#endif
}

#undef LOG_GROUP
#define LOG_GROUP LOG_GROUP_REM_MMIO

/** Read MMIO memory. */
static uint32_t remR3MMIOReadU8(void *pvEnv, target_phys_addr_t GCPhys)
{
    CPUX86State *env = (CPUX86State *)pvEnv;
    uint32_t     u32 = 0;
    int rc = IOMMMIORead(env->pVM, env->pVCpu, GCPhys, &u32, 1);
    AssertMsg(rc == VINF_SUCCESS, ("rc=%Rrc\n", rc)); NOREF(rc);
    Log2(("remR3MMIOReadU8: GCPhys=%RGp -> %02x\n", (RTGCPHYS)GCPhys, u32));
    return u32;
}

/** Read MMIO memory. */
static uint32_t remR3MMIOReadU16(void *pvEnv, target_phys_addr_t GCPhys)
{
    CPUX86State *env = (CPUX86State *)pvEnv;
    uint32_t     u32 = 0;
    int rc = IOMMMIORead(env->pVM, env->pVCpu, GCPhys, &u32, 2);
    AssertMsg(rc == VINF_SUCCESS, ("rc=%Rrc\n", rc)); NOREF(rc);
    Log2(("remR3MMIOReadU16: GCPhys=%RGp -> %04x\n", (RTGCPHYS)GCPhys, u32));
    return u32;
}

/** Read MMIO memory. */
static uint32_t remR3MMIOReadU32(void *pvEnv, target_phys_addr_t GCPhys)
{
    CPUX86State *env = (CPUX86State *)pvEnv;
    uint32_t     u32 = 0;
    int rc = IOMMMIORead(env->pVM, env->pVCpu, GCPhys, &u32, 4);
    AssertMsg(rc == VINF_SUCCESS, ("rc=%Rrc\n", rc)); NOREF(rc);
    Log2(("remR3MMIOReadU32: GCPhys=%RGp -> %08x\n", (RTGCPHYS)GCPhys, u32));
    return u32;
}

/** Write to MMIO memory. */
static void     remR3MMIOWriteU8(void *pvEnv, target_phys_addr_t GCPhys, uint32_t u32)
{
    CPUX86State *env = (CPUX86State *)pvEnv;
    int          rc;
    Log2(("remR3MMIOWriteU8: GCPhys=%RGp u32=%#x\n", (RTGCPHYS)GCPhys, u32));
    rc = IOMMMIOWrite(env->pVM, env->pVCpu, GCPhys, u32, 1);
    AssertMsg(rc == VINF_SUCCESS, ("rc=%Rrc\n", rc)); NOREF(rc);
}

/** Write to MMIO memory. */
static void     remR3MMIOWriteU16(void *pvEnv, target_phys_addr_t GCPhys, uint32_t u32)
{
    CPUX86State *env = (CPUX86State *)pvEnv;
    int          rc;
    Log2(("remR3MMIOWriteU16: GCPhys=%RGp u32=%#x\n", (RTGCPHYS)GCPhys, u32));
    rc = IOMMMIOWrite(env->pVM, env->pVCpu, GCPhys, u32, 2);
    AssertMsg(rc == VINF_SUCCESS, ("rc=%Rrc\n", rc)); NOREF(rc);
}

/** Write to MMIO memory. */
static void     remR3MMIOWriteU32(void *pvEnv, target_phys_addr_t GCPhys, uint32_t u32)
{
    CPUX86State *env = (CPUX86State *)pvEnv;
    int          rc;
    Log2(("remR3MMIOWriteU32: GCPhys=%RGp u32=%#x\n", (RTGCPHYS)GCPhys, u32));
    rc = IOMMMIOWrite(env->pVM, env->pVCpu, GCPhys, u32, 4);
    AssertMsg(rc == VINF_SUCCESS, ("rc=%Rrc\n", rc)); NOREF(rc);
}


#undef LOG_GROUP
#define LOG_GROUP LOG_GROUP_REM_HANDLER

/*  !!!WARNING!!! This is extremely hackish right now, we assume it's only for LFB access!  !!!WARNING!!!  */

static uint32_t remR3HandlerReadU8(void *pvVM, target_phys_addr_t GCPhys)
{
    uint8_t u8;
    Log2(("remR3HandlerReadU8: GCPhys=%RGp\n", (RTGCPHYS)GCPhys));
    VBOXSTRICTRC rcStrict = PGMPhysRead((PVM)pvVM, GCPhys, &u8, sizeof(u8), PGMACCESSORIGIN_REM);
    AssertMsg(rcStrict == VINF_SUCCESS, ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict))); NOREF(rcStrict);
    return u8;
}

static uint32_t remR3HandlerReadU16(void *pvVM, target_phys_addr_t GCPhys)
{
    uint16_t u16;
    Log2(("remR3HandlerReadU16: GCPhys=%RGp\n", (RTGCPHYS)GCPhys));
    VBOXSTRICTRC rcStrict = PGMPhysRead((PVM)pvVM, GCPhys, &u16, sizeof(u16), PGMACCESSORIGIN_REM);
    AssertMsg(rcStrict == VINF_SUCCESS, ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict))); NOREF(rcStrict);
    return u16;
}

static uint32_t remR3HandlerReadU32(void *pvVM, target_phys_addr_t GCPhys)
{
    uint32_t u32;
    Log2(("remR3HandlerReadU32: GCPhys=%RGp\n", (RTGCPHYS)GCPhys));
    VBOXSTRICTRC rcStrict = PGMPhysRead((PVM)pvVM, GCPhys, &u32, sizeof(u32), PGMACCESSORIGIN_REM);
    AssertMsg(rcStrict == VINF_SUCCESS, ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict))); NOREF(rcStrict);
    return u32;
}

static void     remR3HandlerWriteU8(void *pvVM, target_phys_addr_t GCPhys, uint32_t u32)
{
    Log2(("remR3HandlerWriteU8: GCPhys=%RGp u32=%#x\n", (RTGCPHYS)GCPhys, u32));
    VBOXSTRICTRC rcStrict = PGMPhysWrite((PVM)pvVM, GCPhys, &u32, sizeof(uint8_t), PGMACCESSORIGIN_REM);
    AssertMsg(rcStrict == VINF_SUCCESS, ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict))); NOREF(rcStrict);
}

static void     remR3HandlerWriteU16(void *pvVM, target_phys_addr_t GCPhys, uint32_t u32)
{
    Log2(("remR3HandlerWriteU16: GCPhys=%RGp u32=%#x\n", (RTGCPHYS)GCPhys, u32));
    VBOXSTRICTRC rcStrict = PGMPhysWrite((PVM)pvVM, GCPhys, &u32, sizeof(uint16_t), PGMACCESSORIGIN_REM);
    AssertMsg(rcStrict == VINF_SUCCESS, ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict))); NOREF(rcStrict);
}

static void     remR3HandlerWriteU32(void *pvVM, target_phys_addr_t GCPhys, uint32_t u32)
{
    Log2(("remR3HandlerWriteU32: GCPhys=%RGp u32=%#x\n", (RTGCPHYS)GCPhys, u32));
    VBOXSTRICTRC rcStrict = PGMPhysWrite((PVM)pvVM, GCPhys, &u32, sizeof(uint32_t), PGMACCESSORIGIN_REM);
    AssertMsg(rcStrict == VINF_SUCCESS, ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict))); NOREF(rcStrict);
}

/* -+- disassembly -+- */

#undef LOG_GROUP
#define LOG_GROUP LOG_GROUP_REM_DISAS


/**
 * Enables or disables singled stepped disassembly.
 *
 * @returns VBox status code.
 * @param   pVM         VM handle.
 * @param   fEnable     To enable set this flag, to disable clear it.
 */
static DECLCALLBACK(int) remR3DisasEnableStepping(PVM pVM, bool fEnable)
{
    LogFlow(("remR3DisasEnableStepping: fEnable=%d\n", fEnable));
    VM_ASSERT_EMT(pVM);

    if (fEnable)
        pVM->rem.s.Env.state |= CPU_EMULATE_SINGLE_STEP;
    else
        pVM->rem.s.Env.state &= ~CPU_EMULATE_SINGLE_STEP;
#ifdef REM_USE_QEMU_SINGLE_STEP_FOR_LOGGING
    cpu_single_step(&pVM->rem.s.Env, fEnable);
#endif
    return VINF_SUCCESS;
}


/**
 * Enables or disables singled stepped disassembly.
 *
 * @returns VBox status code.
 * @param   pVM         VM handle.
 * @param   fEnable     To enable set this flag, to disable clear it.
 */
REMR3DECL(int) REMR3DisasEnableStepping(PVM pVM, bool fEnable)
{
    int rc;

    LogFlow(("REMR3DisasEnableStepping: fEnable=%d\n", fEnable));
    if (VM_IS_EMT(pVM))
        return remR3DisasEnableStepping(pVM, fEnable);

    rc = VMR3ReqPriorityCallWait(pVM, VMCPUID_ANY, (PFNRT)remR3DisasEnableStepping, 2, pVM, fEnable);
    AssertRC(rc);
    return rc;
}


#ifdef VBOX_WITH_DEBUGGER
/**
 * External Debugger Command: .remstep [on|off|1|0]
 */
static DECLCALLBACK(int) remR3CmdDisasEnableStepping(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PUVM pUVM,
                                                     PCDBGCVAR paArgs, unsigned cArgs)
{
    int rc;
    PVM pVM = pUVM->pVM;

    if (cArgs == 0)
        /*
         * Print the current status.
         */
        rc = DBGCCmdHlpPrintf(pCmdHlp, "DisasStepping is %s\n",
                              pVM->rem.s.Env.state & CPU_EMULATE_SINGLE_STEP ? "enabled" : "disabled");
    else
    {
        /*
         * Convert the argument and change the mode.
         */
        bool fEnable;
        rc = DBGCCmdHlpVarToBool(pCmdHlp, &paArgs[0], &fEnable);
        if (RT_SUCCESS(rc))
        {
            rc = REMR3DisasEnableStepping(pVM, fEnable);
            if (RT_SUCCESS(rc))
                rc = DBGCCmdHlpPrintf(pCmdHlp, "DisasStepping was %s\n", fEnable ? "enabled" : "disabled");
            else
                rc = DBGCCmdHlpFailRc(pCmdHlp, pCmd, rc, "REMR3DisasEnableStepping");
        }
        else
            rc = DBGCCmdHlpFailRc(pCmdHlp, pCmd, rc, "DBGCCmdHlpVarToBool");
    }
    return rc;
}
#endif /* VBOX_WITH_DEBUGGER */


/**
 * Disassembles one instruction and prints it to the log.
 *
 * @returns Success indicator.
 * @param   env         Pointer to the recompiler CPU structure.
 * @param   f32BitCode  Indicates that whether or not the code should
 *                      be disassembled as 16 or 32 bit. If -1 the CS
 *                      selector will be inspected.
 * @param   pszPrefix
 */
bool remR3DisasInstr(CPUX86State *env, int f32BitCode, char *pszPrefix)
{
    PVM pVM = env->pVM;
    const bool fLog = LogIsEnabled();
    const bool fLog2 = LogIs2Enabled();
    int rc = VINF_SUCCESS;

    /*
     * Don't bother if there ain't any log output to do.
     */
    if (!fLog && !fLog2)
        return true;

    /*
     * Update the state so DBGF reads the correct register values.
     */
    remR3StateUpdate(pVM, env->pVCpu);

    /*
     * Log registers if requested.
     */
    if (fLog2)
        DBGFR3_INFO_LOG(pVM, env->pVCpu, "cpumguest", pszPrefix);

    /*
     * Disassemble to log.
     */
    if (fLog)
    {
        PVMCPU  pVCpu = VMMGetCpu(pVM);
        char    szBuf[256];
        szBuf[0] = '\0';
        int rc = DBGFR3DisasInstrEx(pVCpu->pVMR3->pUVM,
                                    pVCpu->idCpu,
                                    0, /* Sel */  0, /* GCPtr */
                                    DBGF_DISAS_FLAGS_CURRENT_GUEST | DBGF_DISAS_FLAGS_DEFAULT_MODE,
                                    szBuf,
                                    sizeof(szBuf),
                                    NULL);
        if (RT_FAILURE(rc))
            RTStrPrintf(szBuf, sizeof(szBuf), "DBGFR3DisasInstrEx failed with rc=%Rrc\n", rc);
        if (pszPrefix && *pszPrefix)
            RTLogPrintf("%s-CPU%d: %s\n", pszPrefix, pVCpu->idCpu, szBuf);
        else
            RTLogPrintf("CPU%d: %s\n", pVCpu->idCpu, szBuf);
    }

    return RT_SUCCESS(rc);
}


/**
 * Disassemble recompiled code.
 *
 * @param   phFileIgnored   Ignored, logfile usually.
 * @param   pvCode          Pointer to the code block.
 * @param   cb              Size of the code block.
 */
void disas(FILE *phFileIgnored, void *pvCode, unsigned long cb)
{
    if (LogIs2Enabled())
    {
        unsigned        off = 0;
        char            szOutput[256];
        DISCPUSTATE     Cpu;
#ifdef RT_ARCH_X86
        DISCPUMODE      enmCpuMode = DISCPUMODE_32BIT;
#else
        DISCPUMODE      enmCpuMode = DISCPUMODE_64BIT;
#endif

        RTLogPrintf("Recompiled Code: %p %#lx (%ld) bytes\n", pvCode, cb, cb);
        while (off < cb)
        {
            uint32_t cbInstr;
            int rc = DISInstrToStr((uint8_t const *)pvCode + off, enmCpuMode,
                                   &Cpu, &cbInstr, szOutput, sizeof(szOutput));
            if (RT_SUCCESS(rc))
                RTLogPrintf("%s", szOutput);
            else
            {
                RTLogPrintf("disas error %Rrc\n", rc);
                cbInstr = 1;
            }
            off += cbInstr;
        }
    }
}


/**
 * Disassemble guest code.
 *
 * @param   phFileIgnored   Ignored, logfile usually.
 * @param   uCode           The guest address of the code to disassemble. (flat?)
 * @param   cb              Number of bytes to disassemble.
 * @param   fFlags          Flags, probably something which tells if this is 16, 32 or 64 bit code.
 */
void target_disas(FILE *phFileIgnored, target_ulong uCode, target_ulong cb, int fFlags)
{
    if (LogIs2Enabled())
    {
        PVM         pVM = cpu_single_env->pVM;
        PVMCPU      pVCpu = cpu_single_env->pVCpu;
        RTSEL       cs;
        RTGCUINTPTR eip;

        Assert(pVCpu);

        /*
         * Update the state so DBGF reads the correct register values (flags).
         */
        remR3StateUpdate(pVM, pVCpu);

        /*
         * Do the disassembling.
         */
        RTLogPrintf("Guest Code: PC=%llx %llx bytes fFlags=%d\n", (uint64_t)uCode, (uint64_t)cb, fFlags);
        cs = cpu_single_env->segs[R_CS].selector;
        eip = uCode - cpu_single_env->segs[R_CS].base;
        for (;;)
        {
            char        szBuf[256];
            uint32_t    cbInstr;
            int rc = DBGFR3DisasInstrEx(pVM->pUVM,
                                        pVCpu->idCpu,
                                        cs,
                                        eip,
                                        DBGF_DISAS_FLAGS_DEFAULT_MODE,
                                        szBuf, sizeof(szBuf),
                                        &cbInstr);
            if (RT_SUCCESS(rc))
                RTLogPrintf("%llx %s\n", (uint64_t)uCode, szBuf);
            else
            {
                RTLogPrintf("%llx %04x:%llx: %s\n", (uint64_t)uCode, cs, (uint64_t)eip, szBuf);
                cbInstr = 1;
            }

            /* next */
            if (cb <= cbInstr)
                break;
            cb -= cbInstr;
            uCode += cbInstr;
            eip += cbInstr;
        }
    }
}


/**
 * Looks up a guest symbol.
 *
 * @returns Pointer to symbol name. This is a static buffer.
 * @param   orig_addr   The address in question.
 */
const char *lookup_symbol(target_ulong orig_addr)
{
    PVM         pVM = cpu_single_env->pVM;
    RTGCINTPTR  off = 0;
    RTDBGSYMBOL Sym;
    DBGFADDRESS Addr;

    int rc = DBGFR3AsSymbolByAddr(pVM->pUVM, DBGF_AS_GLOBAL, DBGFR3AddrFromFlat(pVM->pUVM, &Addr, orig_addr),
                                  RTDBGSYMADDR_FLAGS_LESS_OR_EQUAL, &off, &Sym, NULL /*phMod*/);
    if (RT_SUCCESS(rc))
    {
        static char szSym[sizeof(Sym.szName) + 48];
        if (!off)
            RTStrPrintf(szSym,  sizeof(szSym), "%s\n", Sym.szName);
        else if (off > 0)
            RTStrPrintf(szSym,  sizeof(szSym), "%s+%x\n", Sym.szName,  off);
        else
            RTStrPrintf(szSym,  sizeof(szSym), "%s-%x\n", Sym.szName,  -off);
        return szSym;
    }
    return "<N/A>";
}


#undef LOG_GROUP
#define LOG_GROUP LOG_GROUP_REM


/* -+- FF notifications -+- */

/**
 * Notification about the interrupt FF being set.
 *
 * @param   pVM             VM Handle.
 * @param   pVCpu           VMCPU Handle.
 * @thread  The emulation thread.
 */
REMR3DECL(void) REMR3NotifyInterruptSet(PVM pVM, PVMCPU pVCpu)
{
#ifndef IEM_VERIFICATION_MODE
    LogFlow(("REMR3NotifyInterruptSet: fInRem=%d interrupts %s\n", pVM->rem.s.fInREM,
             (pVM->rem.s.Env.eflags & IF_MASK) && !(pVM->rem.s.Env.hflags & HF_INHIBIT_IRQ_MASK) ? "enabled" : "disabled"));
    if (pVM->rem.s.fInREM)
    {
        ASMAtomicOrS32((int32_t volatile *)&cpu_single_env->interrupt_request,
                       CPU_INTERRUPT_EXTERNAL_HARD);
    }
#endif
}


/**
 * Notification about the interrupt FF being set.
 *
 * @param   pVM             VM Handle.
 * @param   pVCpu           VMCPU Handle.
 * @thread  Any.
 */
REMR3DECL(void) REMR3NotifyInterruptClear(PVM pVM, PVMCPU pVCpu)
{
    LogFlow(("REMR3NotifyInterruptClear:\n"));
    if (pVM->rem.s.fInREM)
        cpu_reset_interrupt(cpu_single_env, CPU_INTERRUPT_HARD);
}


/**
 * Notification about pending timer(s).
 *
 * @param   pVM             VM Handle.
 * @param   pVCpuDst        The target cpu for this notification.
 *                          TM will not broadcast pending timer events, but use
 *                          a dedicated EMT for them. So, only interrupt REM
 *                          execution if the given CPU is executing in REM.
 * @thread  Any.
 */
REMR3DECL(void) REMR3NotifyTimerPending(PVM pVM, PVMCPU pVCpuDst)
{
#ifndef IEM_VERIFICATION_MODE
#ifndef DEBUG_bird
    LogFlow(("REMR3NotifyTimerPending: fInRem=%d\n", pVM->rem.s.fInREM));
#endif
    if (pVM->rem.s.fInREM)
    {
        if (pVM->rem.s.Env.pVCpu == pVCpuDst)
        {
            LogIt(RTLOGGRPFLAGS_LEVEL_5, LOG_GROUP_TM, ("REMR3NotifyTimerPending: setting\n"));
            ASMAtomicOrS32((int32_t volatile *)&pVM->rem.s.Env.interrupt_request,
                           CPU_INTERRUPT_EXTERNAL_TIMER);
        }
        else
            LogIt(RTLOGGRPFLAGS_LEVEL_5, LOG_GROUP_TM, ("REMR3NotifyTimerPending: pVCpu:%p != pVCpuDst:%p\n", pVM->rem.s.Env.pVCpu, pVCpuDst));
    }
    else
        LogIt(RTLOGGRPFLAGS_LEVEL_5, LOG_GROUP_TM, ("REMR3NotifyTimerPending: !fInREM; cpu state=%d\n", VMCPU_GET_STATE(pVCpuDst)));
#endif
}


/**
 * Notification about pending DMA transfers.
 *
 * @param   pVM             VM Handle.
 * @thread  Any.
 */
REMR3DECL(void) REMR3NotifyDmaPending(PVM pVM)
{
#ifndef IEM_VERIFICATION_MODE
    LogFlow(("REMR3NotifyDmaPending: fInRem=%d\n", pVM->rem.s.fInREM));
    if (pVM->rem.s.fInREM)
    {
        ASMAtomicOrS32((int32_t volatile *)&cpu_single_env->interrupt_request,
                       CPU_INTERRUPT_EXTERNAL_DMA);
    }
#endif
}


/**
 * Notification about pending timer(s).
 *
 * @param   pVM             VM Handle.
 * @thread  Any.
 */
REMR3DECL(void) REMR3NotifyQueuePending(PVM pVM)
{
#ifndef IEM_VERIFICATION_MODE
    LogFlow(("REMR3NotifyQueuePending: fInRem=%d\n", pVM->rem.s.fInREM));
    if (pVM->rem.s.fInREM)
    {
        ASMAtomicOrS32((int32_t volatile *)&cpu_single_env->interrupt_request,
                       CPU_INTERRUPT_EXTERNAL_EXIT);
    }
#endif
}


/**
 * Notification about pending FF set by an external thread.
 *
 * @param   pVM             VM handle.
 * @thread  Any.
 */
REMR3DECL(void) REMR3NotifyFF(PVM pVM)
{
#ifndef IEM_VERIFICATION_MODE
    LogFlow(("REMR3NotifyFF: fInRem=%d\n", pVM->rem.s.fInREM));
    if (pVM->rem.s.fInREM)
    {
        ASMAtomicOrS32((int32_t volatile *)&cpu_single_env->interrupt_request,
                       CPU_INTERRUPT_EXTERNAL_EXIT);
    }
#endif
}


#ifdef VBOX_WITH_STATISTICS
void remR3ProfileStart(int statcode)
{
    STAMPROFILEADV *pStat;
    switch(statcode)
    {
    case STATS_EMULATE_SINGLE_INSTR:
        pStat = &gStatExecuteSingleInstr;
        break;
    case STATS_QEMU_COMPILATION:
        pStat = &gStatCompilationQEmu;
        break;
    case STATS_QEMU_RUN_EMULATED_CODE:
        pStat = &gStatRunCodeQEmu;
        break;
    case STATS_QEMU_TOTAL:
        pStat = &gStatTotalTimeQEmu;
        break;
    case STATS_QEMU_RUN_TIMERS:
        pStat = &gStatTimers;
        break;
    case STATS_TLB_LOOKUP:
        pStat= &gStatTBLookup;
        break;
    case STATS_IRQ_HANDLING:
        pStat= &gStatIRQ;
        break;
    case STATS_RAW_CHECK:
        pStat = &gStatRawCheck;
        break;

    default:
        AssertMsgFailed(("unknown stat %d\n", statcode));
        return;
    }
    STAM_PROFILE_ADV_START(pStat, a);
}


void remR3ProfileStop(int statcode)
{
    STAMPROFILEADV *pStat;
    switch(statcode)
    {
    case STATS_EMULATE_SINGLE_INSTR:
        pStat = &gStatExecuteSingleInstr;
        break;
    case STATS_QEMU_COMPILATION:
        pStat = &gStatCompilationQEmu;
        break;
    case STATS_QEMU_RUN_EMULATED_CODE:
        pStat = &gStatRunCodeQEmu;
        break;
    case STATS_QEMU_TOTAL:
        pStat = &gStatTotalTimeQEmu;
        break;
    case STATS_QEMU_RUN_TIMERS:
        pStat = &gStatTimers;
        break;
    case STATS_TLB_LOOKUP:
        pStat= &gStatTBLookup;
        break;
    case STATS_IRQ_HANDLING:
        pStat= &gStatIRQ;
        break;
    case STATS_RAW_CHECK:
        pStat = &gStatRawCheck;
        break;
    default:
        AssertMsgFailed(("unknown stat %d\n", statcode));
        return;
    }
    STAM_PROFILE_ADV_STOP(pStat, a);
}
#endif

/**
 * Raise an RC, force rem exit.
 *
 * @param   pVM     VM handle.
 * @param   rc      The rc.
 */
void remR3RaiseRC(PVM pVM, int rc)
{
    Log(("remR3RaiseRC: rc=%Rrc\n", rc));
    Assert(pVM->rem.s.fInREM);
    VM_ASSERT_EMT(pVM);
    pVM->rem.s.rc = rc;
    cpu_interrupt(&pVM->rem.s.Env, CPU_INTERRUPT_RC);
}


/* -+- timers -+- */

uint64_t cpu_get_tsc(CPUX86State *env)
{
    STAM_COUNTER_INC(&gStatCpuGetTSC);
    return TMCpuTickGet(env->pVCpu);
}


/* -+- interrupts -+- */

void cpu_set_ferr(CPUX86State *env)
{
    int rc = PDMIsaSetIrq(env->pVM, 13, 1, 0 /*uTagSrc*/);
    LogFlow(("cpu_set_ferr: rc=%d\n", rc)); NOREF(rc);
}

int cpu_get_pic_interrupt(CPUX86State *env)
{
    uint8_t u8Interrupt;
    int     rc;

    if (VMCPU_FF_TEST_AND_CLEAR(env->pVCpu, VMCPU_FF_UPDATE_APIC))
        APICUpdatePendingInterrupts(env->pVCpu);

    /* When we fail to forward interrupts directly in raw mode, we fall back to the recompiler.
     * In that case we can't call PDMGetInterrupt anymore, because it has already cleared the interrupt
     * with the (a)pic.
     */
    /* Note! We assume we will go directly to the recompiler to handle the pending interrupt! */
    rc = PDMGetInterrupt(env->pVCpu, &u8Interrupt);
    LogFlow(("cpu_get_pic_interrupt: u8Interrupt=%d rc=%Rrc pc=%04x:%08llx ~flags=%08llx\n",
             u8Interrupt, rc, env->segs[R_CS].selector, (uint64_t)env->eip, (uint64_t)env->eflags));
    if (RT_SUCCESS(rc))
    {
        if (VMCPU_FF_IS_PENDING(env->pVCpu, VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC))
            env->interrupt_request |= CPU_INTERRUPT_HARD;
        return u8Interrupt;
    }
    return -1;
}


/* -+- local apic -+- */

#if 0 /* CPUMSetGuestMsr does this now. */
void cpu_set_apic_base(CPUX86State *env, uint64_t val)
{
    int rc = PDMApicSetBase(env->pVM, val);
    LogFlow(("cpu_set_apic_base: val=%#llx rc=%Rrc\n", val, rc)); NOREF(rc);
}
#endif

uint64_t cpu_get_apic_base(CPUX86State *env)
{
    uint64_t u64;
    VBOXSTRICTRC rcStrict = CPUMQueryGuestMsr(env->pVCpu, MSR_IA32_APICBASE, &u64);
    if (RT_SUCCESS(rcStrict))
    {
        LogFlow(("cpu_get_apic_base: returns %#llx \n", u64));
        return u64;
    }
    LogFlow(("cpu_get_apic_base: returns 0 (rc=%Rrc)\n", VBOXSTRICTRC_VAL(rcStrict)));
    return 0;
}

void cpu_set_apic_tpr(CPUX86State *env, uint8_t val)
{
    int rc = APICSetTpr(env->pVCpu, val << 4);       /* cr8 bits 3-0 correspond to bits 7-4 of the task priority mmio register. */
    LogFlow(("cpu_set_apic_tpr: val=%#x rc=%Rrc\n", val, rc)); NOREF(rc);
}

uint8_t cpu_get_apic_tpr(CPUX86State *env)
{
    uint8_t u8;
    int rc = APICGetTpr(env->pVCpu, &u8, NULL, NULL);
    if (RT_SUCCESS(rc))
    {
        LogFlow(("cpu_get_apic_tpr: returns %#x\n", u8));
        return u8 >> 4;     /* cr8 bits 3-0 correspond to bits 7-4 of the task priority mmio register. */
    }
    LogFlow(("cpu_get_apic_tpr: returns 0 (rc=%Rrc)\n", rc));
    return 0;
}

/**
 * Read an MSR.
 *
 * @retval 0 success.
 * @retval -1 failure, raise \#GP(0).
 * @param   env     The cpu state.
 * @param   idMsr   The MSR to read.
 * @param   puValue Where to return the value.
 */
int cpu_rdmsr(CPUX86State *env, uint32_t idMsr, uint64_t *puValue)
{
    Assert(env->pVCpu);
    return CPUMQueryGuestMsr(env->pVCpu, idMsr, puValue) == VINF_SUCCESS ? 0 : -1;
}

/**
 * Write to an MSR.
 *
 * @retval 0 success.
 * @retval -1 failure, raise \#GP(0).
 * @param   env     The cpu state.
 * @param   idMsr   The MSR to write to.
 * @param   uValue  The value to write.
 */
int cpu_wrmsr(CPUX86State *env, uint32_t idMsr, uint64_t uValue)
{
    Assert(env->pVCpu);
    return CPUMSetGuestMsr(env->pVCpu, idMsr, uValue) == VINF_SUCCESS ? 0 : -1;
}

/* -+- I/O Ports -+- */

#undef LOG_GROUP
#define LOG_GROUP LOG_GROUP_REM_IOPORT

void cpu_outb(CPUX86State *env, pio_addr_t addr, uint8_t val)
{
    int rc;

    if (addr != 0x80 && addr != 0x70 && addr != 0x61)
        Log2(("cpu_outb: addr=%#06x val=%#x\n", addr, val));

    rc = IOMIOPortWrite(env->pVM, env->pVCpu, (RTIOPORT)addr, val, 1);
    if (RT_LIKELY(rc == VINF_SUCCESS))
        return;
    if (rc >= VINF_EM_FIRST && rc <= VINF_EM_LAST)
    {
        Log(("cpu_outb: addr=%#06x val=%#x -> %Rrc\n", addr, val, rc));
        remR3RaiseRC(env->pVM, rc);
        return;
    }
    remAbort(rc, __FUNCTION__);
}

void cpu_outw(CPUX86State *env, pio_addr_t addr, uint16_t val)
{
    //Log2(("cpu_outw: addr=%#06x val=%#x\n", addr, val));
    int rc = IOMIOPortWrite(env->pVM, env->pVCpu, (RTIOPORT)addr, val, 2);
    if (RT_LIKELY(rc == VINF_SUCCESS))
        return;
    if (rc >= VINF_EM_FIRST && rc <= VINF_EM_LAST)
    {
        Log(("cpu_outw: addr=%#06x val=%#x -> %Rrc\n", addr, val, rc));
        remR3RaiseRC(env->pVM, rc);
        return;
    }
    remAbort(rc, __FUNCTION__);
}

void cpu_outl(CPUX86State *env, pio_addr_t addr, uint32_t val)
{
    int rc;
    Log2(("cpu_outl: addr=%#06x val=%#x\n", addr, val));
    rc = IOMIOPortWrite(env->pVM, env->pVCpu, (RTIOPORT)addr, val, 4);
    if (RT_LIKELY(rc == VINF_SUCCESS))
        return;
    if (rc >= VINF_EM_FIRST && rc <= VINF_EM_LAST)
    {
        Log(("cpu_outl: addr=%#06x val=%#x -> %Rrc\n", addr, val, rc));
        remR3RaiseRC(env->pVM, rc);
        return;
    }
    remAbort(rc, __FUNCTION__);
}

uint8_t cpu_inb(CPUX86State *env, pio_addr_t addr)
{
    uint32_t u32 = 0;
    int rc = IOMIOPortRead(env->pVM, env->pVCpu, (RTIOPORT)addr, &u32, 1);
    if (RT_LIKELY(rc == VINF_SUCCESS))
    {
        if (/*addr != 0x61 && */addr != 0x71)
            Log2(("cpu_inb: addr=%#06x -> %#x\n", addr, u32));
        return (uint8_t)u32;
    }
    if (rc >= VINF_EM_FIRST && rc <= VINF_EM_LAST)
    {
        Log(("cpu_inb: addr=%#06x -> %#x rc=%Rrc\n", addr, u32, rc));
        remR3RaiseRC(env->pVM, rc);
        return (uint8_t)u32;
    }
    remAbort(rc, __FUNCTION__);
    return UINT8_C(0xff);
}

uint16_t cpu_inw(CPUX86State *env, pio_addr_t addr)
{
    uint32_t u32 = 0;
    int rc = IOMIOPortRead(env->pVM, env->pVCpu, (RTIOPORT)addr, &u32, 2);
    if (RT_LIKELY(rc == VINF_SUCCESS))
    {
        Log2(("cpu_inw: addr=%#06x -> %#x\n", addr, u32));
        return (uint16_t)u32;
    }
    if (rc >= VINF_EM_FIRST && rc <= VINF_EM_LAST)
    {
        Log(("cpu_inw: addr=%#06x -> %#x rc=%Rrc\n", addr, u32, rc));
        remR3RaiseRC(env->pVM, rc);
        return (uint16_t)u32;
    }
    remAbort(rc, __FUNCTION__);
    return UINT16_C(0xffff);
}

uint32_t cpu_inl(CPUX86State *env, pio_addr_t addr)
{
    uint32_t u32 = 0;
    int rc = IOMIOPortRead(env->pVM, env->pVCpu, (RTIOPORT)addr, &u32, 4);
    if (RT_LIKELY(rc == VINF_SUCCESS))
    {
        Log2(("cpu_inl: addr=%#06x -> %#x\n", addr, u32));
        return u32;
    }
    if (rc >= VINF_EM_FIRST && rc <= VINF_EM_LAST)
    {
        Log(("cpu_inl: addr=%#06x -> %#x rc=%Rrc\n", addr, u32, rc));
        remR3RaiseRC(env->pVM, rc);
        return u32;
    }
    remAbort(rc, __FUNCTION__);
    return UINT32_C(0xffffffff);
}

#undef LOG_GROUP
#define LOG_GROUP LOG_GROUP_REM


/* -+- helpers and misc other interfaces -+- */

/**
 * Perform the CPUID instruction.
 *
 * @param   env         Pointer to the recompiler CPU structure.
 * @param   idx         The CPUID leaf (eax).
 * @param   idxSub      The CPUID sub-leaf (ecx) where applicable.
 * @param   pEAX        Where to store eax.
 * @param   pEBX        Where to store ebx.
 * @param   pECX        Where to store ecx.
 * @param   pEDX        Where to store edx.
 */
void cpu_x86_cpuid(CPUX86State *env, uint32_t idx, uint32_t idxSub,
                   uint32_t *pEAX, uint32_t *pEBX, uint32_t *pECX, uint32_t *pEDX)
{
    NOREF(idxSub);
    CPUMGetGuestCpuId(env->pVCpu, idx, idxSub, pEAX, pEBX, pECX, pEDX);
}


#if 0 /* not used */
/**
 * Interface for qemu hardware to report back fatal errors.
 */
void hw_error(const char *pszFormat, ...)
{
    /*
     * Bitch about it.
     */
    /** @todo Add support for nested arg lists in the LogPrintfV routine! I've code for
     * this in my Odin32 tree at home! */
    va_list args;
    va_start(args, pszFormat);
    RTLogPrintf("fatal error in virtual hardware:");
    RTLogPrintfV(pszFormat, args);
    va_end(args);
    AssertReleaseMsgFailed(("fatal error in virtual hardware: %s\n", pszFormat));

    /*
     * If we're in REM context we'll sync back the state before 'jumping' to
     * the EMs failure handling.
     */
    PVM pVM = cpu_single_env->pVM;
    if (pVM->rem.s.fInREM)
        REMR3StateBack(pVM);
    EMR3FatalError(pVM, VERR_REM_VIRTUAL_HARDWARE_ERROR);
    AssertMsgFailed(("EMR3FatalError returned!\n"));
}
#endif

/**
 * Interface for the qemu cpu to report unhandled situation
 * raising a fatal VM error.
 */
void cpu_abort(CPUX86State *env, const char *pszFormat, ...)
{
    va_list va;
    PVM     pVM;
    PVMCPU  pVCpu;
    char    szMsg[256];

    /*
     * Bitch about it.
     */
    RTLogFlags(NULL, "nodisabled nobuffered");
    RTLogFlush(NULL);

    va_start(va, pszFormat);
#if defined(RT_OS_WINDOWS) && ARCH_BITS == 64
    /* It's a bit complicated when mixing MSC and GCC on AMD64. This is a bit ugly, but it works. */
    unsigned    cArgs     = 0;
    uintptr_t   auArgs[6] = {0,0,0,0,0,0};
    const char *psz       = strchr(pszFormat, '%');
    while (psz && cArgs < 6)
    {
        auArgs[cArgs++] = va_arg(va, uintptr_t);
        psz = strchr(psz + 1, '%');
    }
    switch (cArgs)
    {
        case 1: RTStrPrintf(szMsg, sizeof(szMsg), pszFormat, auArgs[0]); break;
        case 2: RTStrPrintf(szMsg, sizeof(szMsg), pszFormat, auArgs[0], auArgs[1]); break;
        case 3: RTStrPrintf(szMsg, sizeof(szMsg), pszFormat, auArgs[0], auArgs[1], auArgs[2]); break;
        case 4: RTStrPrintf(szMsg, sizeof(szMsg), pszFormat, auArgs[0], auArgs[1], auArgs[2], auArgs[3]); break;
        case 5: RTStrPrintf(szMsg, sizeof(szMsg), pszFormat, auArgs[0], auArgs[1], auArgs[2], auArgs[3], auArgs[4]); break;
        case 6: RTStrPrintf(szMsg, sizeof(szMsg), pszFormat, auArgs[0], auArgs[1], auArgs[2], auArgs[3], auArgs[4], auArgs[5]); break;
        default:
        case 0: RTStrPrintf(szMsg, sizeof(szMsg), "%s", pszFormat); break;
    }
#else
    RTStrPrintfV(szMsg, sizeof(szMsg), pszFormat, va);
#endif
    va_end(va);

    RTLogPrintf("fatal error in recompiler cpu: %s\n", szMsg);
    RTLogRelPrintf("fatal error in recompiler cpu: %s\n", szMsg);

    /*
     * If we're in REM context we'll sync back the state before 'jumping' to
     * the EMs failure handling.
     */
    pVM   = cpu_single_env->pVM;
    pVCpu = cpu_single_env->pVCpu;
    Assert(pVCpu);

    if (pVM->rem.s.fInREM)
        REMR3StateBack(pVM, pVCpu);
    EMR3FatalError(pVCpu, VERR_REM_VIRTUAL_CPU_ERROR);
    AssertMsgFailed(("EMR3FatalError returned!\n"));
}


/**
 * Aborts the VM.
 *
 * @param   rc      VBox error code.
 * @param   pszTip  Hint about why/when this happened.
 */
void remAbort(int rc, const char *pszTip)
{
    PVM     pVM;
    PVMCPU  pVCpu;

    /*
     * Bitch about it.
     */
    RTLogPrintf("internal REM fatal error: rc=%Rrc %s\n", rc, pszTip);
    AssertReleaseMsgFailed(("internal REM fatal error: rc=%Rrc %s\n", rc, pszTip));

    /*
     * Jump back to where we entered the recompiler.
     */
    pVM = cpu_single_env->pVM;
    pVCpu = cpu_single_env->pVCpu;
    Assert(pVCpu);

    if (pVM->rem.s.fInREM)
        REMR3StateBack(pVM, pVCpu);

    EMR3FatalError(pVCpu, rc);
    AssertMsgFailed(("EMR3FatalError returned!\n"));
}


/**
 * Dumps a linux system call.
 * @param   pVCpu     VMCPU handle.
 */
void remR3DumpLnxSyscall(PVMCPU pVCpu)
{
    static const char *apsz[] =
    {
        "sys_restart_syscall",  /* 0 - old "setup()" system call, used for restarting */
        "sys_exit",
        "sys_fork",
        "sys_read",
        "sys_write",
        "sys_open",             /* 5 */
        "sys_close",
        "sys_waitpid",
        "sys_creat",
        "sys_link",
        "sys_unlink",   /* 10 */
        "sys_execve",
        "sys_chdir",
        "sys_time",
        "sys_mknod",
        "sys_chmod",            /* 15 */
        "sys_lchown16",
        "sys_ni_syscall",       /* old break syscall holder */
        "sys_stat",
        "sys_lseek",
        "sys_getpid",   /* 20 */
        "sys_mount",
        "sys_oldumount",
        "sys_setuid16",
        "sys_getuid16",
        "sys_stime",            /* 25 */
        "sys_ptrace",
        "sys_alarm",
        "sys_fstat",
        "sys_pause",
        "sys_utime",            /* 30 */
        "sys_ni_syscall",       /* old stty syscall holder */
        "sys_ni_syscall",       /* old gtty syscall holder */
        "sys_access",
        "sys_nice",
        "sys_ni_syscall",       /* 35 - old ftime syscall holder */
        "sys_sync",
        "sys_kill",
        "sys_rename",
        "sys_mkdir",
        "sys_rmdir",            /* 40 */
        "sys_dup",
        "sys_pipe",
        "sys_times",
        "sys_ni_syscall",       /* old prof syscall holder */
        "sys_brk",              /* 45 */
        "sys_setgid16",
        "sys_getgid16",
        "sys_signal",
        "sys_geteuid16",
        "sys_getegid16",        /* 50 */
        "sys_acct",
        "sys_umount",   /* recycled never used phys() */
        "sys_ni_syscall",       /* old lock syscall holder */
        "sys_ioctl",
        "sys_fcntl",            /* 55 */
        "sys_ni_syscall",       /* old mpx syscall holder */
        "sys_setpgid",
        "sys_ni_syscall",       /* old ulimit syscall holder */
        "sys_olduname",
        "sys_umask",            /* 60 */
        "sys_chroot",
        "sys_ustat",
        "sys_dup2",
        "sys_getppid",
        "sys_getpgrp",  /* 65 */
        "sys_setsid",
        "sys_sigaction",
        "sys_sgetmask",
        "sys_ssetmask",
        "sys_setreuid16",       /* 70 */
        "sys_setregid16",
        "sys_sigsuspend",
        "sys_sigpending",
        "sys_sethostname",
        "sys_setrlimit",        /* 75 */
        "sys_old_getrlimit",
        "sys_getrusage",
        "sys_gettimeofday",
        "sys_settimeofday",
        "sys_getgroups16",      /* 80 */
        "sys_setgroups16",
        "old_select",
        "sys_symlink",
        "sys_lstat",
        "sys_readlink", /* 85 */
        "sys_uselib",
        "sys_swapon",
        "sys_reboot",
        "old_readdir",
        "old_mmap",             /* 90 */
        "sys_munmap",
        "sys_truncate",
        "sys_ftruncate",
        "sys_fchmod",
        "sys_fchown16", /* 95 */
        "sys_getpriority",
        "sys_setpriority",
        "sys_ni_syscall",       /* old profil syscall holder */
        "sys_statfs",
        "sys_fstatfs",  /* 100 */
        "sys_ioperm",
        "sys_socketcall",
        "sys_syslog",
        "sys_setitimer",
        "sys_getitimer",        /* 105 */
        "sys_newstat",
        "sys_newlstat",
        "sys_newfstat",
        "sys_uname",
        "sys_iopl",             /* 110 */
        "sys_vhangup",
        "sys_ni_syscall",       /* old "idle" system call */
        "sys_vm86old",
        "sys_wait4",
        "sys_swapoff",  /* 115 */
        "sys_sysinfo",
        "sys_ipc",
        "sys_fsync",
        "sys_sigreturn",
        "sys_clone",            /* 120 */
        "sys_setdomainname",
        "sys_newuname",
        "sys_modify_ldt",
        "sys_adjtimex",
        "sys_mprotect", /* 125 */
        "sys_sigprocmask",
        "sys_ni_syscall",       /* old "create_module" */
        "sys_init_module",
        "sys_delete_module",
        "sys_ni_syscall",       /* 130: old "get_kernel_syms" */
        "sys_quotactl",
        "sys_getpgid",
        "sys_fchdir",
        "sys_bdflush",
        "sys_sysfs",            /* 135 */
        "sys_personality",
        "sys_ni_syscall",       /* reserved for afs_syscall */
        "sys_setfsuid16",
        "sys_setfsgid16",
        "sys_llseek",   /* 140 */
        "sys_getdents",
        "sys_select",
        "sys_flock",
        "sys_msync",
        "sys_readv",            /* 145 */
        "sys_writev",
        "sys_getsid",
        "sys_fdatasync",
        "sys_sysctl",
        "sys_mlock",            /* 150 */
        "sys_munlock",
        "sys_mlockall",
        "sys_munlockall",
        "sys_sched_setparam",
        "sys_sched_getparam",   /* 155 */
        "sys_sched_setscheduler",
        "sys_sched_getscheduler",
        "sys_sched_yield",
        "sys_sched_get_priority_max",
        "sys_sched_get_priority_min",  /* 160 */
        "sys_sched_rr_get_interval",
        "sys_nanosleep",
        "sys_mremap",
        "sys_setresuid16",
        "sys_getresuid16",      /* 165 */
        "sys_vm86",
        "sys_ni_syscall",       /* Old sys_query_module */
        "sys_poll",
        "sys_nfsservctl",
        "sys_setresgid16",      /* 170 */
        "sys_getresgid16",
        "sys_prctl",
        "sys_rt_sigreturn",
        "sys_rt_sigaction",
        "sys_rt_sigprocmask",   /* 175 */
        "sys_rt_sigpending",
        "sys_rt_sigtimedwait",
        "sys_rt_sigqueueinfo",
        "sys_rt_sigsuspend",
        "sys_pread64",  /* 180 */
        "sys_pwrite64",
        "sys_chown16",
        "sys_getcwd",
        "sys_capget",
        "sys_capset",   /* 185 */
        "sys_sigaltstack",
        "sys_sendfile",
        "sys_ni_syscall",       /* reserved for streams1 */
        "sys_ni_syscall",       /* reserved for streams2 */
        "sys_vfork",            /* 190 */
        "sys_getrlimit",
        "sys_mmap2",
        "sys_truncate64",
        "sys_ftruncate64",
        "sys_stat64",   /* 195 */
        "sys_lstat64",
        "sys_fstat64",
        "sys_lchown",
        "sys_getuid",
        "sys_getgid",   /* 200 */
        "sys_geteuid",
        "sys_getegid",
        "sys_setreuid",
        "sys_setregid",
        "sys_getgroups",        /* 205 */
        "sys_setgroups",
        "sys_fchown",
        "sys_setresuid",
        "sys_getresuid",
        "sys_setresgid",        /* 210 */
        "sys_getresgid",
        "sys_chown",
        "sys_setuid",
        "sys_setgid",
        "sys_setfsuid", /* 215 */
        "sys_setfsgid",
        "sys_pivot_root",
        "sys_mincore",
        "sys_madvise",
        "sys_getdents64",       /* 220 */
        "sys_fcntl64",
        "sys_ni_syscall",       /* reserved for TUX */
        "sys_ni_syscall",
        "sys_gettid",
        "sys_readahead",        /* 225 */
        "sys_setxattr",
        "sys_lsetxattr",
        "sys_fsetxattr",
        "sys_getxattr",
        "sys_lgetxattr",        /* 230 */
        "sys_fgetxattr",
        "sys_listxattr",
        "sys_llistxattr",
        "sys_flistxattr",
        "sys_removexattr",      /* 235 */
        "sys_lremovexattr",
        "sys_fremovexattr",
        "sys_tkill",
        "sys_sendfile64",
        "sys_futex",            /* 240 */
        "sys_sched_setaffinity",
        "sys_sched_getaffinity",
        "sys_set_thread_area",
        "sys_get_thread_area",
        "sys_io_setup", /* 245 */
        "sys_io_destroy",
        "sys_io_getevents",
        "sys_io_submit",
        "sys_io_cancel",
        "sys_fadvise64",        /* 250 */
        "sys_ni_syscall",
        "sys_exit_group",
        "sys_lookup_dcookie",
        "sys_epoll_create",
        "sys_epoll_ctl",        /* 255 */
        "sys_epoll_wait",
        "sys_remap_file_pages",
        "sys_set_tid_address",
        "sys_timer_create",
        "sys_timer_settime",            /* 260 */
        "sys_timer_gettime",
        "sys_timer_getoverrun",
        "sys_timer_delete",
        "sys_clock_settime",
        "sys_clock_gettime",            /* 265 */
        "sys_clock_getres",
        "sys_clock_nanosleep",
        "sys_statfs64",
        "sys_fstatfs64",
        "sys_tgkill",   /* 270 */
        "sys_utimes",
        "sys_fadvise64_64",
        "sys_ni_syscall"        /* sys_vserver */
    };

    uint32_t    uEAX = CPUMGetGuestEAX(pVCpu);
    switch (uEAX)
    {
        default:
            if (uEAX < RT_ELEMENTS(apsz))
                Log(("REM: linux syscall %3d: %s (eip=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x ebp=%08x)\n",
                     uEAX, apsz[uEAX], CPUMGetGuestEIP(pVCpu), CPUMGetGuestEBX(pVCpu), CPUMGetGuestECX(pVCpu),
                     CPUMGetGuestEDX(pVCpu), CPUMGetGuestESI(pVCpu), CPUMGetGuestEDI(pVCpu), CPUMGetGuestEBP(pVCpu)));
            else
                Log(("eip=%08x: linux syscall %d (#%x) unknown\n", CPUMGetGuestEIP(pVCpu), uEAX, uEAX));
            break;

    }
}


/**
 * Dumps an OpenBSD system call.
 * @param   pVCpu     VMCPU handle.
 */
void remR3DumpOBsdSyscall(PVMCPU pVCpu)
{
    static const char *apsz[] =
    {
        "SYS_syscall",          //0
        "SYS_exit",             //1
        "SYS_fork",             //2
        "SYS_read",             //3
        "SYS_write",            //4
        "SYS_open",             //5
        "SYS_close",            //6
        "SYS_wait4",            //7
        "SYS_8",
        "SYS_link",             //9
        "SYS_unlink",           //10
        "SYS_11",
        "SYS_chdir",            //12
        "SYS_fchdir",           //13
        "SYS_mknod",            //14
        "SYS_chmod",            //15
        "SYS_chown",            //16
        "SYS_break",            //17
        "SYS_18",
        "SYS_19",
        "SYS_getpid",           //20
        "SYS_mount",            //21
        "SYS_unmount",          //22
        "SYS_setuid",           //23
        "SYS_getuid",           //24
        "SYS_geteuid",          //25
        "SYS_ptrace",           //26
        "SYS_recvmsg",          //27
        "SYS_sendmsg",          //28
        "SYS_recvfrom",         //29
        "SYS_accept",           //30
        "SYS_getpeername",      //31
        "SYS_getsockname",      //32
        "SYS_access",           //33
        "SYS_chflags",          //34
        "SYS_fchflags",         //35
        "SYS_sync",             //36
        "SYS_kill",             //37
        "SYS_38",
        "SYS_getppid",          //39
        "SYS_40",
        "SYS_dup",              //41
        "SYS_opipe",            //42
        "SYS_getegid",          //43
        "SYS_profil",           //44
        "SYS_ktrace",           //45
        "SYS_sigaction",        //46
        "SYS_getgid",           //47
        "SYS_sigprocmask",      //48
        "SYS_getlogin",         //49
        "SYS_setlogin",         //50
        "SYS_acct",             //51
        "SYS_sigpending",       //52
        "SYS_osigaltstack",     //53
        "SYS_ioctl",            //54
        "SYS_reboot",           //55
        "SYS_revoke",           //56
        "SYS_symlink",          //57
        "SYS_readlink",         //58
        "SYS_execve",           //59
        "SYS_umask",            //60
        "SYS_chroot",           //61
        "SYS_62",
        "SYS_63",
        "SYS_64",
        "SYS_65",
        "SYS_vfork",            //66
        "SYS_67",
        "SYS_68",
        "SYS_sbrk",             //69
        "SYS_sstk",             //70
        "SYS_61",
        "SYS_vadvise",          //72
        "SYS_munmap",           //73
        "SYS_mprotect",         //74
        "SYS_madvise",          //75
        "SYS_76",
        "SYS_77",
        "SYS_mincore",          //78
        "SYS_getgroups",        //79
        "SYS_setgroups",        //80
        "SYS_getpgrp",          //81
        "SYS_setpgid",          //82
        "SYS_setitimer",        //83
        "SYS_84",
        "SYS_85",
        "SYS_getitimer",        //86
        "SYS_87",
        "SYS_88",
        "SYS_89",
        "SYS_dup2",             //90
        "SYS_91",
        "SYS_fcntl",            //92
        "SYS_select",           //93
        "SYS_94",
        "SYS_fsync",            //95
        "SYS_setpriority",      //96
        "SYS_socket",           //97
        "SYS_connect",          //98
        "SYS_99",
        "SYS_getpriority",      //100
        "SYS_101",
        "SYS_102",
        "SYS_sigreturn",        //103
        "SYS_bind",             //104
        "SYS_setsockopt",       //105
        "SYS_listen",           //106
        "SYS_107",
        "SYS_108",
        "SYS_109",
        "SYS_110",
        "SYS_sigsuspend",       //111
        "SYS_112",
        "SYS_113",
        "SYS_114",
        "SYS_115",
        "SYS_gettimeofday",     //116
        "SYS_getrusage",        //117
        "SYS_getsockopt",       //118
        "SYS_119",
        "SYS_readv",            //120
        "SYS_writev",           //121
        "SYS_settimeofday",     //122
        "SYS_fchown",           //123
        "SYS_fchmod",           //124
        "SYS_125",
        "SYS_setreuid",         //126
        "SYS_setregid",         //127
        "SYS_rename",           //128
        "SYS_129",
        "SYS_130",
        "SYS_flock",            //131
        "SYS_mkfifo",           //132
        "SYS_sendto",           //133
        "SYS_shutdown",         //134
        "SYS_socketpair",       //135
        "SYS_mkdir",            //136
        "SYS_rmdir",            //137
        "SYS_utimes",           //138
        "SYS_139",
        "SYS_adjtime",          //140
        "SYS_141",
        "SYS_142",
        "SYS_143",
        "SYS_144",
        "SYS_145",
        "SYS_146",
        "SYS_setsid",           //147
        "SYS_quotactl",         //148
        "SYS_149",
        "SYS_150",
        "SYS_151",
        "SYS_152",
        "SYS_153",
        "SYS_154",
        "SYS_nfssvc",           //155
        "SYS_156",
        "SYS_157",
        "SYS_158",
        "SYS_159",
        "SYS_160",
        "SYS_getfh",            //161
        "SYS_162",
        "SYS_163",
        "SYS_164",
        "SYS_sysarch",          //165
        "SYS_166",
        "SYS_167",
        "SYS_168",
        "SYS_169",
        "SYS_170",
        "SYS_171",
        "SYS_172",
        "SYS_pread",            //173
        "SYS_pwrite",           //174
        "SYS_175",
        "SYS_176",
        "SYS_177",
        "SYS_178",
        "SYS_179",
        "SYS_180",
        "SYS_setgid",           //181
        "SYS_setegid",          //182
        "SYS_seteuid",          //183
        "SYS_lfs_bmapv",        //184
        "SYS_lfs_markv",        //185
        "SYS_lfs_segclean",     //186
        "SYS_lfs_segwait",      //187
        "SYS_188",
        "SYS_189",
        "SYS_190",
        "SYS_pathconf",         //191
        "SYS_fpathconf",        //192
        "SYS_swapctl",          //193
        "SYS_getrlimit",        //194
        "SYS_setrlimit",        //195
        "SYS_getdirentries",    //196
        "SYS_mmap",             //197
        "SYS___syscall",        //198
        "SYS_lseek",            //199
        "SYS_truncate",         //200
        "SYS_ftruncate",        //201
        "SYS___sysctl",         //202
        "SYS_mlock",            //203
        "SYS_munlock",          //204
        "SYS_205",
        "SYS_futimes",          //206
        "SYS_getpgid",          //207
        "SYS_xfspioctl",        //208
        "SYS_209",
        "SYS_210",
        "SYS_211",
        "SYS_212",
        "SYS_213",
        "SYS_214",
        "SYS_215",
        "SYS_216",
        "SYS_217",
        "SYS_218",
        "SYS_219",
        "SYS_220",
        "SYS_semget",           //221
        "SYS_222",
        "SYS_223",
        "SYS_224",
        "SYS_msgget",           //225
        "SYS_msgsnd",           //226
        "SYS_msgrcv",           //227
        "SYS_shmat",            //228
        "SYS_229",
        "SYS_shmdt",            //230
        "SYS_231",
        "SYS_clock_gettime",    //232
        "SYS_clock_settime",    //233
        "SYS_clock_getres",     //234
        "SYS_235",
        "SYS_236",
        "SYS_237",
        "SYS_238",
        "SYS_239",
        "SYS_nanosleep",        //240
        "SYS_241",
        "SYS_242",
        "SYS_243",
        "SYS_244",
        "SYS_245",
        "SYS_246",
        "SYS_247",
        "SYS_248",
        "SYS_249",
        "SYS_minherit",         //250
        "SYS_rfork",            //251
        "SYS_poll",             //252
        "SYS_issetugid",        //253
        "SYS_lchown",           //254
        "SYS_getsid",           //255
        "SYS_msync",            //256
        "SYS_257",
        "SYS_258",
        "SYS_259",
        "SYS_getfsstat",        //260
        "SYS_statfs",           //261
        "SYS_fstatfs",          //262
        "SYS_pipe",             //263
        "SYS_fhopen",           //264
        "SYS_265",
        "SYS_fhstatfs",         //266
        "SYS_preadv",           //267
        "SYS_pwritev",          //268
        "SYS_kqueue",           //269
        "SYS_kevent",           //270
        "SYS_mlockall",         //271
        "SYS_munlockall",       //272
        "SYS_getpeereid",       //273
        "SYS_274",
        "SYS_275",
        "SYS_276",
        "SYS_277",
        "SYS_278",
        "SYS_279",
        "SYS_280",
        "SYS_getresuid",        //281
        "SYS_setresuid",        //282
        "SYS_getresgid",        //283
        "SYS_setresgid",        //284
        "SYS_285",
        "SYS_mquery",           //286
        "SYS_closefrom",        //287
        "SYS_sigaltstack",      //288
        "SYS_shmget",           //289
        "SYS_semop",            //290
        "SYS_stat",             //291
        "SYS_fstat",            //292
        "SYS_lstat",            //293
        "SYS_fhstat",           //294
        "SYS___semctl",         //295
        "SYS_shmctl",           //296
        "SYS_msgctl",           //297
        "SYS_MAXSYSCALL",       //298
        //299
        //300
    };
    uint32_t    uEAX;
    if (!LogIsEnabled())
        return;
    uEAX = CPUMGetGuestEAX(pVCpu);
    switch (uEAX)
    {
        default:
            if (uEAX < RT_ELEMENTS(apsz))
            {
                uint32_t au32Args[8] = {0};
                PGMPhysSimpleReadGCPtr(pVCpu, au32Args, CPUMGetGuestESP(pVCpu), sizeof(au32Args));
                RTLogPrintf("REM: OpenBSD syscall %3d: %s (eip=%08x %08x %08x %08x %08x %08x %08x %08x %08x)\n",
                            uEAX, apsz[uEAX], CPUMGetGuestEIP(pVCpu), au32Args[0], au32Args[1], au32Args[2], au32Args[3],
                            au32Args[4], au32Args[5], au32Args[6], au32Args[7]);
            }
            else
                RTLogPrintf("eip=%08x: OpenBSD syscall %d (#%x) unknown!!\n", CPUMGetGuestEIP(pVCpu), uEAX, uEAX);
            break;
    }
}


#if defined(IPRT_NO_CRT) && defined(RT_OS_WINDOWS) && defined(RT_ARCH_X86)
/**
 * The Dll main entry point (stub).
 */
bool __stdcall _DllMainCRTStartup(void *hModule, uint32_t dwReason, void *pvReserved)
{
    return true;
}

void *memcpy(void *dst, const void *src, size_t size)
{
    uint8_t*pbDst = dst, *pbSrc = src;
    while (size-- > 0)
        *pbDst++ = *pbSrc++;
    return dst;
}

#endif

void cpu_smm_update(CPUX86State *env)
{
}
