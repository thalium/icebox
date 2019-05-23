/* $Id: CPUM.cpp $ */
/** @file
 * CPUM - CPU Monitor / Manager.
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

/** @page pg_cpum CPUM - CPU Monitor / Manager
 *
 * The CPU Monitor / Manager keeps track of all the CPU registers. It is
 * also responsible for lazy FPU handling and some of the context loading
 * in raw mode.
 *
 * There are three CPU contexts, the most important one is the guest one (GC).
 * When running in raw-mode (RC) there is a special hyper context for the VMM
 * part that floats around inside the guest address space. When running in
 * raw-mode, CPUM also maintains a host context for saving and restoring
 * registers across world switches. This latter is done in cooperation with the
 * world switcher (@see pg_vmm).
 *
 * @see grp_cpum
 *
 * @section sec_cpum_fpu        FPU / SSE / AVX / ++ state.
 *
 * TODO: proper write up, currently just some notes.
 *
 * The ring-0 FPU handling per OS:
 *
 *      - 64-bit Windows uses XMM registers in the kernel as part of the calling
 *        convention (Visual C++ doesn't seem to have a way to disable
 *        generating such code either), so CR0.TS/EM are always zero from what I
 *        can tell.  We are also forced to always load/save the guest XMM0-XMM15
 *        registers when entering/leaving guest context.  Interrupt handlers
 *        using FPU/SSE will offically have call save and restore functions
 *        exported by the kernel, if the really really have to use the state.
 *
 *      - 32-bit windows does lazy FPU handling, I think, probably including
 *        lazying saving.  The Windows Internals book states that it's a bad
 *        idea to use the FPU in kernel space. However, it looks like it will
 *        restore the FPU state of the current thread in case of a kernel \#NM.
 *        Interrupt handlers should be same as for 64-bit.
 *
 *      - Darwin allows taking \#NM in kernel space, restoring current thread's
 *        state if I read the code correctly.  It saves the FPU state of the
 *        outgoing thread, and uses CR0.TS to lazily load the state of the
 *        incoming one.  No idea yet how the FPU is treated by interrupt
 *        handlers, i.e. whether they are allowed to disable the state or
 *        something.
 *
 *      - Linux also allows \#NM in kernel space (don't know since when), and
 *        uses CR0.TS for lazy loading.  Saves outgoing thread's state, lazy
 *        loads the incoming unless configured to agressivly load it.  Interrupt
 *        handlers can ask whether they're allowed to use the FPU, and may
 *        freely trash the state if Linux thinks it has saved the thread's state
 *        already.  This is a problem.
 *
 *      - Solaris will, from what I can tell, panic if it gets an \#NM in kernel
 *        context.  When switching threads, the kernel will save the state of
 *        the outgoing thread and lazy load the incoming one using CR0.TS.
 *        There are a few routines in seeblk.s which uses the SSE unit in ring-0
 *        to do stuff, HAT are among the users.  The routines there will
 *        manually clear CR0.TS and save the XMM registers they use only if
 *        CR0.TS was zero upon entry.  They will skip it when not, because as
 *        mentioned above, the FPU state is saved when switching away from a
 *        thread and CR0.TS set to 1, so when CR0.TS is 1 there is nothing to
 *        preserve.  This is a problem if we restore CR0.TS to 1 after loading
 *        the guest state.
 *
 *      - FreeBSD - no idea yet.
 *
 *      - OS/2 does not allow \#NMs in kernel space IIRC.  Does lazy loading,
 *        possibly also lazy saving.  Interrupts must preserve the CR0.TS+EM &
 *        FPU states.
 *
 * Up to r107425 (2016-05-24) we would only temporarily modify CR0.TS/EM while
 * saving and restoring the host and guest states.  The motivation for this
 * change is that we want to be able to emulate SSE instruction in ring-0 (IEM).
 *
 * Starting with that change, we will leave CR0.TS=EM=0 after saving the host
 * state and only restore it once we've restore the host FPU state. This has the
 * accidental side effect of triggering Solaris to preserve XMM registers in
 * sseblk.s. When CR0 was changed by saving the FPU state, CPUM must now inform
 * the VT-x (HMVMX) code about it as it caches the CR0 value in the VMCS.
 *
 *
 * @section sec_cpum_logging        Logging Level Assignments.
 *
 * Following log level assignments:
 *      - Log6 is used for FPU state management.
 *      - Log7 is used for FPU state actualization.
 *
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_CPUM
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/cpumdis.h>
#include <VBox/vmm/cpumctx-v1_6.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/apic.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/iem.h>
#include <VBox/vmm/selm.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/patm.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/ssm.h>
#include "CPUMInternal.h"
#include <VBox/vmm/vm.h>

#include <VBox/param.h>
#include <VBox/dis.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/asm-amd64-x86.h>
#include <iprt/assert.h>
#include <iprt/cpuset.h>
#include <iprt/mem.h>
#include <iprt/mp.h>
#include <iprt/string.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/**
 * This was used in the saved state up to the early life of version 14.
 *
 * It indicates that we may have some out-of-sync hidden segement registers.
 * It is only relevant for raw-mode.
 */
#define CPUM_CHANGED_HIDDEN_SEL_REGS_INVALID    RT_BIT(12)


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * What kind of cpu info dump to perform.
 */
typedef enum CPUMDUMPTYPE
{
    CPUMDUMPTYPE_TERSE,
    CPUMDUMPTYPE_DEFAULT,
    CPUMDUMPTYPE_VERBOSE
} CPUMDUMPTYPE;
/** Pointer to a cpu info dump type. */
typedef CPUMDUMPTYPE *PCPUMDUMPTYPE;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static DECLCALLBACK(int)  cpumR3LiveExec(PVM pVM, PSSMHANDLE pSSM, uint32_t uPass);
static DECLCALLBACK(int)  cpumR3SaveExec(PVM pVM, PSSMHANDLE pSSM);
static DECLCALLBACK(int)  cpumR3LoadPrep(PVM pVM, PSSMHANDLE pSSM);
static DECLCALLBACK(int)  cpumR3LoadExec(PVM pVM, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass);
static DECLCALLBACK(int)  cpumR3LoadDone(PVM pVM, PSSMHANDLE pSSM);
static DECLCALLBACK(void) cpumR3InfoAll(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs);
static DECLCALLBACK(void) cpumR3InfoGuest(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs);
static DECLCALLBACK(void) cpumR3InfoGuestHwvirt(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs);
static DECLCALLBACK(void) cpumR3InfoGuestInstr(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs);
static DECLCALLBACK(void) cpumR3InfoHyper(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs);
static DECLCALLBACK(void) cpumR3InfoHost(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Saved state field descriptors for CPUMCTX. */
static const SSMFIELD g_aCpumCtxFields[] =
{
    SSMFIELD_ENTRY(         CPUMCTX, rdi),
    SSMFIELD_ENTRY(         CPUMCTX, rsi),
    SSMFIELD_ENTRY(         CPUMCTX, rbp),
    SSMFIELD_ENTRY(         CPUMCTX, rax),
    SSMFIELD_ENTRY(         CPUMCTX, rbx),
    SSMFIELD_ENTRY(         CPUMCTX, rdx),
    SSMFIELD_ENTRY(         CPUMCTX, rcx),
    SSMFIELD_ENTRY(         CPUMCTX, rsp),
    SSMFIELD_ENTRY(         CPUMCTX, rflags),
    SSMFIELD_ENTRY(         CPUMCTX, rip),
    SSMFIELD_ENTRY(         CPUMCTX, r8),
    SSMFIELD_ENTRY(         CPUMCTX, r9),
    SSMFIELD_ENTRY(         CPUMCTX, r10),
    SSMFIELD_ENTRY(         CPUMCTX, r11),
    SSMFIELD_ENTRY(         CPUMCTX, r12),
    SSMFIELD_ENTRY(         CPUMCTX, r13),
    SSMFIELD_ENTRY(         CPUMCTX, r14),
    SSMFIELD_ENTRY(         CPUMCTX, r15),
    SSMFIELD_ENTRY(         CPUMCTX, es.Sel),
    SSMFIELD_ENTRY(         CPUMCTX, es.ValidSel),
    SSMFIELD_ENTRY(         CPUMCTX, es.fFlags),
    SSMFIELD_ENTRY(         CPUMCTX, es.u64Base),
    SSMFIELD_ENTRY(         CPUMCTX, es.u32Limit),
    SSMFIELD_ENTRY(         CPUMCTX, es.Attr),
    SSMFIELD_ENTRY(         CPUMCTX, cs.Sel),
    SSMFIELD_ENTRY(         CPUMCTX, cs.ValidSel),
    SSMFIELD_ENTRY(         CPUMCTX, cs.fFlags),
    SSMFIELD_ENTRY(         CPUMCTX, cs.u64Base),
    SSMFIELD_ENTRY(         CPUMCTX, cs.u32Limit),
    SSMFIELD_ENTRY(         CPUMCTX, cs.Attr),
    SSMFIELD_ENTRY(         CPUMCTX, ss.Sel),
    SSMFIELD_ENTRY(         CPUMCTX, ss.ValidSel),
    SSMFIELD_ENTRY(         CPUMCTX, ss.fFlags),
    SSMFIELD_ENTRY(         CPUMCTX, ss.u64Base),
    SSMFIELD_ENTRY(         CPUMCTX, ss.u32Limit),
    SSMFIELD_ENTRY(         CPUMCTX, ss.Attr),
    SSMFIELD_ENTRY(         CPUMCTX, ds.Sel),
    SSMFIELD_ENTRY(         CPUMCTX, ds.ValidSel),
    SSMFIELD_ENTRY(         CPUMCTX, ds.fFlags),
    SSMFIELD_ENTRY(         CPUMCTX, ds.u64Base),
    SSMFIELD_ENTRY(         CPUMCTX, ds.u32Limit),
    SSMFIELD_ENTRY(         CPUMCTX, ds.Attr),
    SSMFIELD_ENTRY(         CPUMCTX, fs.Sel),
    SSMFIELD_ENTRY(         CPUMCTX, fs.ValidSel),
    SSMFIELD_ENTRY(         CPUMCTX, fs.fFlags),
    SSMFIELD_ENTRY(         CPUMCTX, fs.u64Base),
    SSMFIELD_ENTRY(         CPUMCTX, fs.u32Limit),
    SSMFIELD_ENTRY(         CPUMCTX, fs.Attr),
    SSMFIELD_ENTRY(         CPUMCTX, gs.Sel),
    SSMFIELD_ENTRY(         CPUMCTX, gs.ValidSel),
    SSMFIELD_ENTRY(         CPUMCTX, gs.fFlags),
    SSMFIELD_ENTRY(         CPUMCTX, gs.u64Base),
    SSMFIELD_ENTRY(         CPUMCTX, gs.u32Limit),
    SSMFIELD_ENTRY(         CPUMCTX, gs.Attr),
    SSMFIELD_ENTRY(         CPUMCTX, cr0),
    SSMFIELD_ENTRY(         CPUMCTX, cr2),
    SSMFIELD_ENTRY(         CPUMCTX, cr3),
    SSMFIELD_ENTRY(         CPUMCTX, cr4),
    SSMFIELD_ENTRY(         CPUMCTX, dr[0]),
    SSMFIELD_ENTRY(         CPUMCTX, dr[1]),
    SSMFIELD_ENTRY(         CPUMCTX, dr[2]),
    SSMFIELD_ENTRY(         CPUMCTX, dr[3]),
    SSMFIELD_ENTRY(         CPUMCTX, dr[6]),
    SSMFIELD_ENTRY(         CPUMCTX, dr[7]),
    SSMFIELD_ENTRY(         CPUMCTX, gdtr.cbGdt),
    SSMFIELD_ENTRY(         CPUMCTX, gdtr.pGdt),
    SSMFIELD_ENTRY(         CPUMCTX, idtr.cbIdt),
    SSMFIELD_ENTRY(         CPUMCTX, idtr.pIdt),
    SSMFIELD_ENTRY(         CPUMCTX, SysEnter.cs),
    SSMFIELD_ENTRY(         CPUMCTX, SysEnter.eip),
    SSMFIELD_ENTRY(         CPUMCTX, SysEnter.esp),
    SSMFIELD_ENTRY(         CPUMCTX, msrEFER),
    SSMFIELD_ENTRY(         CPUMCTX, msrSTAR),
    SSMFIELD_ENTRY(         CPUMCTX, msrPAT),
    SSMFIELD_ENTRY(         CPUMCTX, msrLSTAR),
    SSMFIELD_ENTRY(         CPUMCTX, msrCSTAR),
    SSMFIELD_ENTRY(         CPUMCTX, msrSFMASK),
    SSMFIELD_ENTRY(         CPUMCTX, msrKERNELGSBASE),
    SSMFIELD_ENTRY(         CPUMCTX, ldtr.Sel),
    SSMFIELD_ENTRY(         CPUMCTX, ldtr.ValidSel),
    SSMFIELD_ENTRY(         CPUMCTX, ldtr.fFlags),
    SSMFIELD_ENTRY(         CPUMCTX, ldtr.u64Base),
    SSMFIELD_ENTRY(         CPUMCTX, ldtr.u32Limit),
    SSMFIELD_ENTRY(         CPUMCTX, ldtr.Attr),
    SSMFIELD_ENTRY(         CPUMCTX, tr.Sel),
    SSMFIELD_ENTRY(         CPUMCTX, tr.ValidSel),
    SSMFIELD_ENTRY(         CPUMCTX, tr.fFlags),
    SSMFIELD_ENTRY(         CPUMCTX, tr.u64Base),
    SSMFIELD_ENTRY(         CPUMCTX, tr.u32Limit),
    SSMFIELD_ENTRY(         CPUMCTX, tr.Attr),
    SSMFIELD_ENTRY_VER(     CPUMCTX, aXcr[0],                           CPUM_SAVED_STATE_VERSION_XSAVE),
    SSMFIELD_ENTRY_VER(     CPUMCTX, aXcr[1],                           CPUM_SAVED_STATE_VERSION_XSAVE),
    SSMFIELD_ENTRY_VER(     CPUMCTX, fXStateMask,                       CPUM_SAVED_STATE_VERSION_XSAVE),
    SSMFIELD_ENTRY_TERM()
};

/** Saved state field descriptors for CPUMCTX. */
static const SSMFIELD g_aCpumX87Fields[] =
{
    SSMFIELD_ENTRY(         X86FXSTATE, FCW),
    SSMFIELD_ENTRY(         X86FXSTATE, FSW),
    SSMFIELD_ENTRY(         X86FXSTATE, FTW),
    SSMFIELD_ENTRY(         X86FXSTATE, FOP),
    SSMFIELD_ENTRY(         X86FXSTATE, FPUIP),
    SSMFIELD_ENTRY(         X86FXSTATE, CS),
    SSMFIELD_ENTRY(         X86FXSTATE, Rsrvd1),
    SSMFIELD_ENTRY(         X86FXSTATE, FPUDP),
    SSMFIELD_ENTRY(         X86FXSTATE, DS),
    SSMFIELD_ENTRY(         X86FXSTATE, Rsrvd2),
    SSMFIELD_ENTRY(         X86FXSTATE, MXCSR),
    SSMFIELD_ENTRY(         X86FXSTATE, MXCSR_MASK),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[0]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[1]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[2]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[3]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[4]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[5]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[6]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[7]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[0]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[1]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[2]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[3]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[4]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[5]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[6]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[7]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[8]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[9]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[10]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[11]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[12]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[13]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[14]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[15]),
    SSMFIELD_ENTRY_VER(     X86FXSTATE, au32RsrvdForSoftware[0],        CPUM_SAVED_STATE_VERSION_XSAVE), /* 32-bit/64-bit hack */
    SSMFIELD_ENTRY_TERM()
};

/** Saved state field descriptors for X86XSAVEHDR. */
static const SSMFIELD g_aCpumXSaveHdrFields[] =
{
    SSMFIELD_ENTRY(         X86XSAVEHDR,  bmXState),
    SSMFIELD_ENTRY_TERM()
};

/** Saved state field descriptors for X86XSAVEYMMHI. */
static const SSMFIELD g_aCpumYmmHiFields[] =
{
    SSMFIELD_ENTRY(         X86XSAVEYMMHI, aYmmHi[0]),
    SSMFIELD_ENTRY(         X86XSAVEYMMHI, aYmmHi[1]),
    SSMFIELD_ENTRY(         X86XSAVEYMMHI, aYmmHi[2]),
    SSMFIELD_ENTRY(         X86XSAVEYMMHI, aYmmHi[3]),
    SSMFIELD_ENTRY(         X86XSAVEYMMHI, aYmmHi[4]),
    SSMFIELD_ENTRY(         X86XSAVEYMMHI, aYmmHi[5]),
    SSMFIELD_ENTRY(         X86XSAVEYMMHI, aYmmHi[6]),
    SSMFIELD_ENTRY(         X86XSAVEYMMHI, aYmmHi[7]),
    SSMFIELD_ENTRY(         X86XSAVEYMMHI, aYmmHi[8]),
    SSMFIELD_ENTRY(         X86XSAVEYMMHI, aYmmHi[9]),
    SSMFIELD_ENTRY(         X86XSAVEYMMHI, aYmmHi[10]),
    SSMFIELD_ENTRY(         X86XSAVEYMMHI, aYmmHi[11]),
    SSMFIELD_ENTRY(         X86XSAVEYMMHI, aYmmHi[12]),
    SSMFIELD_ENTRY(         X86XSAVEYMMHI, aYmmHi[13]),
    SSMFIELD_ENTRY(         X86XSAVEYMMHI, aYmmHi[14]),
    SSMFIELD_ENTRY(         X86XSAVEYMMHI, aYmmHi[15]),
    SSMFIELD_ENTRY_TERM()
};

/** Saved state field descriptors for X86XSAVEBNDREGS. */
static const SSMFIELD g_aCpumBndRegsFields[] =
{
    SSMFIELD_ENTRY(         X86XSAVEBNDREGS, aRegs[0]),
    SSMFIELD_ENTRY(         X86XSAVEBNDREGS, aRegs[1]),
    SSMFIELD_ENTRY(         X86XSAVEBNDREGS, aRegs[2]),
    SSMFIELD_ENTRY(         X86XSAVEBNDREGS, aRegs[3]),
    SSMFIELD_ENTRY_TERM()
};

/** Saved state field descriptors for X86XSAVEBNDCFG. */
static const SSMFIELD g_aCpumBndCfgFields[] =
{
    SSMFIELD_ENTRY(         X86XSAVEBNDCFG, fConfig),
    SSMFIELD_ENTRY(         X86XSAVEBNDCFG, fStatus),
    SSMFIELD_ENTRY_TERM()
};

#if 0 /** @todo */
/** Saved state field descriptors for X86XSAVEOPMASK. */
static const SSMFIELD g_aCpumOpmaskFields[] =
{
    SSMFIELD_ENTRY(         X86XSAVEOPMASK, aKRegs[0]),
    SSMFIELD_ENTRY(         X86XSAVEOPMASK, aKRegs[1]),
    SSMFIELD_ENTRY(         X86XSAVEOPMASK, aKRegs[2]),
    SSMFIELD_ENTRY(         X86XSAVEOPMASK, aKRegs[3]),
    SSMFIELD_ENTRY(         X86XSAVEOPMASK, aKRegs[4]),
    SSMFIELD_ENTRY(         X86XSAVEOPMASK, aKRegs[5]),
    SSMFIELD_ENTRY(         X86XSAVEOPMASK, aKRegs[6]),
    SSMFIELD_ENTRY(         X86XSAVEOPMASK, aKRegs[7]),
    SSMFIELD_ENTRY_TERM()
};
#endif

/** Saved state field descriptors for X86XSAVEZMMHI256. */
static const SSMFIELD g_aCpumZmmHi256Fields[] =
{
    SSMFIELD_ENTRY(         X86XSAVEZMMHI256, aHi256Regs[0]),
    SSMFIELD_ENTRY(         X86XSAVEZMMHI256, aHi256Regs[1]),
    SSMFIELD_ENTRY(         X86XSAVEZMMHI256, aHi256Regs[2]),
    SSMFIELD_ENTRY(         X86XSAVEZMMHI256, aHi256Regs[3]),
    SSMFIELD_ENTRY(         X86XSAVEZMMHI256, aHi256Regs[4]),
    SSMFIELD_ENTRY(         X86XSAVEZMMHI256, aHi256Regs[5]),
    SSMFIELD_ENTRY(         X86XSAVEZMMHI256, aHi256Regs[6]),
    SSMFIELD_ENTRY(         X86XSAVEZMMHI256, aHi256Regs[7]),
    SSMFIELD_ENTRY(         X86XSAVEZMMHI256, aHi256Regs[8]),
    SSMFIELD_ENTRY(         X86XSAVEZMMHI256, aHi256Regs[9]),
    SSMFIELD_ENTRY(         X86XSAVEZMMHI256, aHi256Regs[10]),
    SSMFIELD_ENTRY(         X86XSAVEZMMHI256, aHi256Regs[11]),
    SSMFIELD_ENTRY(         X86XSAVEZMMHI256, aHi256Regs[12]),
    SSMFIELD_ENTRY(         X86XSAVEZMMHI256, aHi256Regs[13]),
    SSMFIELD_ENTRY(         X86XSAVEZMMHI256, aHi256Regs[14]),
    SSMFIELD_ENTRY(         X86XSAVEZMMHI256, aHi256Regs[15]),
    SSMFIELD_ENTRY_TERM()
};

/** Saved state field descriptors for X86XSAVEZMM16HI. */
static const SSMFIELD g_aCpumZmm16HiFields[] =
{
    SSMFIELD_ENTRY(         X86XSAVEZMM16HI, aRegs[0]),
    SSMFIELD_ENTRY(         X86XSAVEZMM16HI, aRegs[1]),
    SSMFIELD_ENTRY(         X86XSAVEZMM16HI, aRegs[2]),
    SSMFIELD_ENTRY(         X86XSAVEZMM16HI, aRegs[3]),
    SSMFIELD_ENTRY(         X86XSAVEZMM16HI, aRegs[4]),
    SSMFIELD_ENTRY(         X86XSAVEZMM16HI, aRegs[5]),
    SSMFIELD_ENTRY(         X86XSAVEZMM16HI, aRegs[6]),
    SSMFIELD_ENTRY(         X86XSAVEZMM16HI, aRegs[7]),
    SSMFIELD_ENTRY(         X86XSAVEZMM16HI, aRegs[8]),
    SSMFIELD_ENTRY(         X86XSAVEZMM16HI, aRegs[9]),
    SSMFIELD_ENTRY(         X86XSAVEZMM16HI, aRegs[10]),
    SSMFIELD_ENTRY(         X86XSAVEZMM16HI, aRegs[11]),
    SSMFIELD_ENTRY(         X86XSAVEZMM16HI, aRegs[12]),
    SSMFIELD_ENTRY(         X86XSAVEZMM16HI, aRegs[13]),
    SSMFIELD_ENTRY(         X86XSAVEZMM16HI, aRegs[14]),
    SSMFIELD_ENTRY(         X86XSAVEZMM16HI, aRegs[15]),
    SSMFIELD_ENTRY_TERM()
};



/** Saved state field descriptors for CPUMCTX in V4.1 before the hidden selector
 * registeres changed. */
static const SSMFIELD g_aCpumX87FieldsMem[] =
{
    SSMFIELD_ENTRY(         X86FXSTATE, FCW),
    SSMFIELD_ENTRY(         X86FXSTATE, FSW),
    SSMFIELD_ENTRY(         X86FXSTATE, FTW),
    SSMFIELD_ENTRY(         X86FXSTATE, FOP),
    SSMFIELD_ENTRY(         X86FXSTATE, FPUIP),
    SSMFIELD_ENTRY(         X86FXSTATE, CS),
    SSMFIELD_ENTRY(         X86FXSTATE, Rsrvd1),
    SSMFIELD_ENTRY(         X86FXSTATE, FPUDP),
    SSMFIELD_ENTRY(         X86FXSTATE, DS),
    SSMFIELD_ENTRY(         X86FXSTATE, Rsrvd2),
    SSMFIELD_ENTRY(         X86FXSTATE, MXCSR),
    SSMFIELD_ENTRY(         X86FXSTATE, MXCSR_MASK),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[0]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[1]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[2]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[3]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[4]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[5]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[6]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[7]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[0]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[1]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[2]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[3]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[4]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[5]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[6]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[7]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[8]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[9]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[10]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[11]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[12]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[13]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[14]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[15]),
    SSMFIELD_ENTRY_IGNORE(  X86FXSTATE, au32RsrvdRest),
    SSMFIELD_ENTRY_IGNORE(  X86FXSTATE, au32RsrvdForSoftware),
};

/** Saved state field descriptors for CPUMCTX in V4.1 before the hidden selector
 * registeres changed. */
static const SSMFIELD g_aCpumCtxFieldsMem[] =
{
    SSMFIELD_ENTRY(         CPUMCTX, rdi),
    SSMFIELD_ENTRY(         CPUMCTX, rsi),
    SSMFIELD_ENTRY(         CPUMCTX, rbp),
    SSMFIELD_ENTRY(         CPUMCTX, rax),
    SSMFIELD_ENTRY(         CPUMCTX, rbx),
    SSMFIELD_ENTRY(         CPUMCTX, rdx),
    SSMFIELD_ENTRY(         CPUMCTX, rcx),
    SSMFIELD_ENTRY(         CPUMCTX, rsp),
    SSMFIELD_ENTRY_OLD(              lss_esp, sizeof(uint32_t)),
    SSMFIELD_ENTRY(         CPUMCTX, ss.Sel),
    SSMFIELD_ENTRY_OLD(              ssPadding, sizeof(uint16_t)),
    SSMFIELD_ENTRY(         CPUMCTX, gs.Sel),
    SSMFIELD_ENTRY_OLD(              gsPadding, sizeof(uint16_t)),
    SSMFIELD_ENTRY(         CPUMCTX, fs.Sel),
    SSMFIELD_ENTRY_OLD(              fsPadding, sizeof(uint16_t)),
    SSMFIELD_ENTRY(         CPUMCTX, es.Sel),
    SSMFIELD_ENTRY_OLD(              esPadding, sizeof(uint16_t)),
    SSMFIELD_ENTRY(         CPUMCTX, ds.Sel),
    SSMFIELD_ENTRY_OLD(              dsPadding, sizeof(uint16_t)),
    SSMFIELD_ENTRY(         CPUMCTX, cs.Sel),
    SSMFIELD_ENTRY_OLD(              csPadding, sizeof(uint16_t)*3),
    SSMFIELD_ENTRY(         CPUMCTX, rflags),
    SSMFIELD_ENTRY(         CPUMCTX, rip),
    SSMFIELD_ENTRY(         CPUMCTX, r8),
    SSMFIELD_ENTRY(         CPUMCTX, r9),
    SSMFIELD_ENTRY(         CPUMCTX, r10),
    SSMFIELD_ENTRY(         CPUMCTX, r11),
    SSMFIELD_ENTRY(         CPUMCTX, r12),
    SSMFIELD_ENTRY(         CPUMCTX, r13),
    SSMFIELD_ENTRY(         CPUMCTX, r14),
    SSMFIELD_ENTRY(         CPUMCTX, r15),
    SSMFIELD_ENTRY(         CPUMCTX, es.u64Base),
    SSMFIELD_ENTRY(         CPUMCTX, es.u32Limit),
    SSMFIELD_ENTRY(         CPUMCTX, es.Attr),
    SSMFIELD_ENTRY(         CPUMCTX, cs.u64Base),
    SSMFIELD_ENTRY(         CPUMCTX, cs.u32Limit),
    SSMFIELD_ENTRY(         CPUMCTX, cs.Attr),
    SSMFIELD_ENTRY(         CPUMCTX, ss.u64Base),
    SSMFIELD_ENTRY(         CPUMCTX, ss.u32Limit),
    SSMFIELD_ENTRY(         CPUMCTX, ss.Attr),
    SSMFIELD_ENTRY(         CPUMCTX, ds.u64Base),
    SSMFIELD_ENTRY(         CPUMCTX, ds.u32Limit),
    SSMFIELD_ENTRY(         CPUMCTX, ds.Attr),
    SSMFIELD_ENTRY(         CPUMCTX, fs.u64Base),
    SSMFIELD_ENTRY(         CPUMCTX, fs.u32Limit),
    SSMFIELD_ENTRY(         CPUMCTX, fs.Attr),
    SSMFIELD_ENTRY(         CPUMCTX, gs.u64Base),
    SSMFIELD_ENTRY(         CPUMCTX, gs.u32Limit),
    SSMFIELD_ENTRY(         CPUMCTX, gs.Attr),
    SSMFIELD_ENTRY(         CPUMCTX, cr0),
    SSMFIELD_ENTRY(         CPUMCTX, cr2),
    SSMFIELD_ENTRY(         CPUMCTX, cr3),
    SSMFIELD_ENTRY(         CPUMCTX, cr4),
    SSMFIELD_ENTRY(         CPUMCTX, dr[0]),
    SSMFIELD_ENTRY(         CPUMCTX, dr[1]),
    SSMFIELD_ENTRY(         CPUMCTX, dr[2]),
    SSMFIELD_ENTRY(         CPUMCTX, dr[3]),
    SSMFIELD_ENTRY_OLD(              dr[4], sizeof(uint64_t)),
    SSMFIELD_ENTRY_OLD(              dr[5], sizeof(uint64_t)),
    SSMFIELD_ENTRY(         CPUMCTX, dr[6]),
    SSMFIELD_ENTRY(         CPUMCTX, dr[7]),
    SSMFIELD_ENTRY(         CPUMCTX, gdtr.cbGdt),
    SSMFIELD_ENTRY(         CPUMCTX, gdtr.pGdt),
    SSMFIELD_ENTRY_OLD(              gdtrPadding, sizeof(uint16_t)),
    SSMFIELD_ENTRY(         CPUMCTX, idtr.cbIdt),
    SSMFIELD_ENTRY(         CPUMCTX, idtr.pIdt),
    SSMFIELD_ENTRY_OLD(              idtrPadding, sizeof(uint16_t)),
    SSMFIELD_ENTRY(         CPUMCTX, ldtr.Sel),
    SSMFIELD_ENTRY_OLD(              ldtrPadding, sizeof(uint16_t)),
    SSMFIELD_ENTRY(         CPUMCTX, tr.Sel),
    SSMFIELD_ENTRY_OLD(              trPadding, sizeof(uint16_t)),
    SSMFIELD_ENTRY(         CPUMCTX, SysEnter.cs),
    SSMFIELD_ENTRY(         CPUMCTX, SysEnter.eip),
    SSMFIELD_ENTRY(         CPUMCTX, SysEnter.esp),
    SSMFIELD_ENTRY(         CPUMCTX, msrEFER),
    SSMFIELD_ENTRY(         CPUMCTX, msrSTAR),
    SSMFIELD_ENTRY(         CPUMCTX, msrPAT),
    SSMFIELD_ENTRY(         CPUMCTX, msrLSTAR),
    SSMFIELD_ENTRY(         CPUMCTX, msrCSTAR),
    SSMFIELD_ENTRY(         CPUMCTX, msrSFMASK),
    SSMFIELD_ENTRY(         CPUMCTX, msrKERNELGSBASE),
    SSMFIELD_ENTRY(         CPUMCTX, ldtr.u64Base),
    SSMFIELD_ENTRY(         CPUMCTX, ldtr.u32Limit),
    SSMFIELD_ENTRY(         CPUMCTX, ldtr.Attr),
    SSMFIELD_ENTRY(         CPUMCTX, tr.u64Base),
    SSMFIELD_ENTRY(         CPUMCTX, tr.u32Limit),
    SSMFIELD_ENTRY(         CPUMCTX, tr.Attr),
    SSMFIELD_ENTRY_TERM()
};

/** Saved state field descriptors for CPUMCTX_VER1_6. */
static const SSMFIELD g_aCpumX87FieldsV16[] =
{
    SSMFIELD_ENTRY(         X86FXSTATE, FCW),
    SSMFIELD_ENTRY(         X86FXSTATE, FSW),
    SSMFIELD_ENTRY(         X86FXSTATE, FTW),
    SSMFIELD_ENTRY(         X86FXSTATE, FOP),
    SSMFIELD_ENTRY(         X86FXSTATE, FPUIP),
    SSMFIELD_ENTRY(         X86FXSTATE, CS),
    SSMFIELD_ENTRY(         X86FXSTATE, Rsrvd1),
    SSMFIELD_ENTRY(         X86FXSTATE, FPUDP),
    SSMFIELD_ENTRY(         X86FXSTATE, DS),
    SSMFIELD_ENTRY(         X86FXSTATE, Rsrvd2),
    SSMFIELD_ENTRY(         X86FXSTATE, MXCSR),
    SSMFIELD_ENTRY(         X86FXSTATE, MXCSR_MASK),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[0]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[1]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[2]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[3]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[4]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[5]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[6]),
    SSMFIELD_ENTRY(         X86FXSTATE, aRegs[7]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[0]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[1]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[2]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[3]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[4]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[5]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[6]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[7]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[8]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[9]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[10]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[11]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[12]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[13]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[14]),
    SSMFIELD_ENTRY(         X86FXSTATE, aXMM[15]),
    SSMFIELD_ENTRY_IGNORE(  X86FXSTATE, au32RsrvdRest),
    SSMFIELD_ENTRY_IGNORE(  X86FXSTATE, au32RsrvdForSoftware),
    SSMFIELD_ENTRY_TERM()
};

/** Saved state field descriptors for CPUMCTX_VER1_6. */
static const SSMFIELD g_aCpumCtxFieldsV16[] =
{
    SSMFIELD_ENTRY(             CPUMCTX, rdi),
    SSMFIELD_ENTRY(             CPUMCTX, rsi),
    SSMFIELD_ENTRY(             CPUMCTX, rbp),
    SSMFIELD_ENTRY(             CPUMCTX, rax),
    SSMFIELD_ENTRY(             CPUMCTX, rbx),
    SSMFIELD_ENTRY(             CPUMCTX, rdx),
    SSMFIELD_ENTRY(             CPUMCTX, rcx),
    SSMFIELD_ENTRY_U32_ZX_U64(  CPUMCTX, rsp),
    SSMFIELD_ENTRY(             CPUMCTX, ss.Sel),
    SSMFIELD_ENTRY_OLD(                  ssPadding, sizeof(uint16_t)),
    SSMFIELD_ENTRY_OLD(         CPUMCTX, sizeof(uint64_t) /*rsp_notused*/),
    SSMFIELD_ENTRY(             CPUMCTX, gs.Sel),
    SSMFIELD_ENTRY_OLD(                  gsPadding, sizeof(uint16_t)),
    SSMFIELD_ENTRY(             CPUMCTX, fs.Sel),
    SSMFIELD_ENTRY_OLD(                  fsPadding, sizeof(uint16_t)),
    SSMFIELD_ENTRY(             CPUMCTX, es.Sel),
    SSMFIELD_ENTRY_OLD(                  esPadding, sizeof(uint16_t)),
    SSMFIELD_ENTRY(             CPUMCTX, ds.Sel),
    SSMFIELD_ENTRY_OLD(                  dsPadding, sizeof(uint16_t)),
    SSMFIELD_ENTRY(             CPUMCTX, cs.Sel),
    SSMFIELD_ENTRY_OLD(                  csPadding, sizeof(uint16_t)*3),
    SSMFIELD_ENTRY(             CPUMCTX, rflags),
    SSMFIELD_ENTRY(             CPUMCTX, rip),
    SSMFIELD_ENTRY(             CPUMCTX, r8),
    SSMFIELD_ENTRY(             CPUMCTX, r9),
    SSMFIELD_ENTRY(             CPUMCTX, r10),
    SSMFIELD_ENTRY(             CPUMCTX, r11),
    SSMFIELD_ENTRY(             CPUMCTX, r12),
    SSMFIELD_ENTRY(             CPUMCTX, r13),
    SSMFIELD_ENTRY(             CPUMCTX, r14),
    SSMFIELD_ENTRY(             CPUMCTX, r15),
    SSMFIELD_ENTRY_U32_ZX_U64(  CPUMCTX, es.u64Base),
    SSMFIELD_ENTRY(             CPUMCTX, es.u32Limit),
    SSMFIELD_ENTRY(             CPUMCTX, es.Attr),
    SSMFIELD_ENTRY_U32_ZX_U64(  CPUMCTX, cs.u64Base),
    SSMFIELD_ENTRY(             CPUMCTX, cs.u32Limit),
    SSMFIELD_ENTRY(             CPUMCTX, cs.Attr),
    SSMFIELD_ENTRY_U32_ZX_U64(  CPUMCTX, ss.u64Base),
    SSMFIELD_ENTRY(             CPUMCTX, ss.u32Limit),
    SSMFIELD_ENTRY(             CPUMCTX, ss.Attr),
    SSMFIELD_ENTRY_U32_ZX_U64(  CPUMCTX, ds.u64Base),
    SSMFIELD_ENTRY(             CPUMCTX, ds.u32Limit),
    SSMFIELD_ENTRY(             CPUMCTX, ds.Attr),
    SSMFIELD_ENTRY_U32_ZX_U64(  CPUMCTX, fs.u64Base),
    SSMFIELD_ENTRY(             CPUMCTX, fs.u32Limit),
    SSMFIELD_ENTRY(             CPUMCTX, fs.Attr),
    SSMFIELD_ENTRY_U32_ZX_U64(  CPUMCTX, gs.u64Base),
    SSMFIELD_ENTRY(             CPUMCTX, gs.u32Limit),
    SSMFIELD_ENTRY(             CPUMCTX, gs.Attr),
    SSMFIELD_ENTRY(             CPUMCTX, cr0),
    SSMFIELD_ENTRY(             CPUMCTX, cr2),
    SSMFIELD_ENTRY(             CPUMCTX, cr3),
    SSMFIELD_ENTRY(             CPUMCTX, cr4),
    SSMFIELD_ENTRY_OLD(                  cr8, sizeof(uint64_t)),
    SSMFIELD_ENTRY(             CPUMCTX, dr[0]),
    SSMFIELD_ENTRY(             CPUMCTX, dr[1]),
    SSMFIELD_ENTRY(             CPUMCTX, dr[2]),
    SSMFIELD_ENTRY(             CPUMCTX, dr[3]),
    SSMFIELD_ENTRY_OLD(                  dr[4], sizeof(uint64_t)),
    SSMFIELD_ENTRY_OLD(                  dr[5], sizeof(uint64_t)),
    SSMFIELD_ENTRY(             CPUMCTX, dr[6]),
    SSMFIELD_ENTRY(             CPUMCTX, dr[7]),
    SSMFIELD_ENTRY(             CPUMCTX, gdtr.cbGdt),
    SSMFIELD_ENTRY_U32_ZX_U64(  CPUMCTX, gdtr.pGdt),
    SSMFIELD_ENTRY_OLD(                  gdtrPadding, sizeof(uint16_t)),
    SSMFIELD_ENTRY_OLD(                  gdtrPadding64, sizeof(uint64_t)),
    SSMFIELD_ENTRY(             CPUMCTX, idtr.cbIdt),
    SSMFIELD_ENTRY_U32_ZX_U64(  CPUMCTX, idtr.pIdt),
    SSMFIELD_ENTRY_OLD(                  idtrPadding, sizeof(uint16_t)),
    SSMFIELD_ENTRY_OLD(                  idtrPadding64, sizeof(uint64_t)),
    SSMFIELD_ENTRY(             CPUMCTX, ldtr.Sel),
    SSMFIELD_ENTRY_OLD(                  ldtrPadding, sizeof(uint16_t)),
    SSMFIELD_ENTRY(             CPUMCTX, tr.Sel),
    SSMFIELD_ENTRY_OLD(                  trPadding, sizeof(uint16_t)),
    SSMFIELD_ENTRY(             CPUMCTX, SysEnter.cs),
    SSMFIELD_ENTRY(             CPUMCTX, SysEnter.eip),
    SSMFIELD_ENTRY(             CPUMCTX, SysEnter.esp),
    SSMFIELD_ENTRY(             CPUMCTX, msrEFER),
    SSMFIELD_ENTRY(             CPUMCTX, msrSTAR),
    SSMFIELD_ENTRY(             CPUMCTX, msrPAT),
    SSMFIELD_ENTRY(             CPUMCTX, msrLSTAR),
    SSMFIELD_ENTRY(             CPUMCTX, msrCSTAR),
    SSMFIELD_ENTRY(             CPUMCTX, msrSFMASK),
    SSMFIELD_ENTRY_OLD(                  msrFSBASE, sizeof(uint64_t)),
    SSMFIELD_ENTRY_OLD(                  msrGSBASE, sizeof(uint64_t)),
    SSMFIELD_ENTRY(             CPUMCTX, msrKERNELGSBASE),
    SSMFIELD_ENTRY_U32_ZX_U64(  CPUMCTX, ldtr.u64Base),
    SSMFIELD_ENTRY(             CPUMCTX, ldtr.u32Limit),
    SSMFIELD_ENTRY(             CPUMCTX, ldtr.Attr),
    SSMFIELD_ENTRY_U32_ZX_U64(  CPUMCTX, tr.u64Base),
    SSMFIELD_ENTRY(             CPUMCTX, tr.u32Limit),
    SSMFIELD_ENTRY(             CPUMCTX, tr.Attr),
    SSMFIELD_ENTRY_OLD(                  padding, sizeof(uint32_t)*2),
    SSMFIELD_ENTRY_TERM()
};


/**
 * Checks for partial/leaky FXSAVE/FXRSTOR handling on AMD CPUs.
 *
 * AMD K7, K8 and newer AMD CPUs do not save/restore the x87 error pointers
 * (last instruction pointer, last data pointer, last opcode) except when the ES
 * bit (Exception Summary) in x87 FSW (FPU Status Word) is set. Thus if we don't
 * clear these registers there is potential, local FPU leakage from a process
 * using the FPU to another.
 *
 * See AMD Instruction Reference for FXSAVE, FXRSTOR.
 *
 * @param   pVM     The cross context VM structure.
 */
static void cpumR3CheckLeakyFpu(PVM pVM)
{
    uint32_t u32CpuVersion = ASMCpuId_EAX(1);
    uint32_t const u32Family = u32CpuVersion >> 8;
    if (   u32Family >= 6      /* K7 and higher */
        && ASMIsAmdCpu())
    {
        uint32_t cExt = ASMCpuId_EAX(0x80000000);
        if (ASMIsValidExtRange(cExt))
        {
            uint32_t fExtFeaturesEDX = ASMCpuId_EDX(0x80000001);
            if (fExtFeaturesEDX & X86_CPUID_AMD_FEATURE_EDX_FFXSR)
            {
                for (VMCPUID i = 0; i < pVM->cCpus; i++)
                    pVM->aCpus[i].cpum.s.fUseFlags |= CPUM_USE_FFXSR_LEAKY;
                Log(("CPUMR3Init: host CPU has leaky fxsave/fxrstor behaviour\n"));
            }
        }
    }
}


/**
 * Frees memory allocated by cpumR3AllocHwVirtState().
 *
 * @param   pVM     The cross context VM structure.
 */
static void cpumR3FreeHwVirtState(PVM pVM)
{
    Assert(pVM->cpum.ro.GuestFeatures.fSvm);
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU pVCpu = &pVM->aCpus[i];
        if (pVCpu->cpum.s.Guest.hwvirt.svm.pVmcbR3)
        {
            SUPR3PageFreeEx(pVCpu->cpum.s.Guest.hwvirt.svm.pVmcbR3, SVM_VMCB_PAGES);
            pVCpu->cpum.s.Guest.hwvirt.svm.pVmcbR3 = NULL;
        }
        pVCpu->cpum.s.Guest.hwvirt.svm.HCPhysVmcb = NIL_RTHCPHYS;

        if (pVCpu->cpum.s.Guest.hwvirt.svm.pvMsrBitmapR3)
        {
            SUPR3PageFreeEx(pVCpu->cpum.s.Guest.hwvirt.svm.pvMsrBitmapR3, SVM_MSRPM_PAGES);
            pVCpu->cpum.s.Guest.hwvirt.svm.pvMsrBitmapR3 = NULL;
        }

        if (pVCpu->cpum.s.Guest.hwvirt.svm.pvIoBitmapR3)
        {
            SUPR3PageFreeEx(pVCpu->cpum.s.Guest.hwvirt.svm.pvIoBitmapR3, SVM_IOPM_PAGES);
            pVCpu->cpum.s.Guest.hwvirt.svm.pvIoBitmapR3 = NULL;
        }
    }
}


/**
 * Allocates memory required by the hardware virtualization state.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 */
static int cpumR3AllocHwVirtState(PVM pVM)
{
    Assert(pVM->cpum.ro.GuestFeatures.fSvm);

    int rc = VINF_SUCCESS;
    LogRel(("CPUM: Allocating a total of %u pages for the nested-guest SVM MSR and IO permission bitmaps\n",
            pVM->cCpus * (SVM_MSRPM_PAGES + SVM_IOPM_PAGES)));
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU pVCpu = &pVM->aCpus[i];

        /*
         * Allocate the nested-guest VMCB.
         */
        SUPPAGE SupNstGstVmcbPage;
        RT_ZERO(SupNstGstVmcbPage);
        SupNstGstVmcbPage.Phys = NIL_RTHCPHYS;
        Assert(SVM_VMCB_PAGES == 1);
        Assert(!pVCpu->cpum.s.Guest.hwvirt.svm.pVmcbR3);
        rc = SUPR3PageAllocEx(SVM_VMCB_PAGES, 0 /* fFlags */, (void **)&pVCpu->cpum.s.Guest.hwvirt.svm.pVmcbR3,
                              &pVCpu->cpum.s.Guest.hwvirt.svm.pVmcbR0, &SupNstGstVmcbPage);
        if (RT_FAILURE(rc))
        {
            Assert(!pVCpu->cpum.s.Guest.hwvirt.svm.pVmcbR3);
            LogRel(("CPUM%u: Failed to alloc %u pages for the nested-guest's VMCB\n", pVCpu->idCpu, SVM_VMCB_PAGES));
            break;
        }
        pVCpu->cpum.s.Guest.hwvirt.svm.HCPhysVmcb = SupNstGstVmcbPage.Phys;

        /*
         * Allocate the MSRPM (MSR Permission bitmap).
         */
        Assert(!pVCpu->cpum.s.Guest.hwvirt.svm.pvMsrBitmapR3);
        rc = SUPR3PageAllocEx(SVM_MSRPM_PAGES, 0 /* fFlags */, &pVCpu->cpum.s.Guest.hwvirt.svm.pvMsrBitmapR3,
                              &pVCpu->cpum.s.Guest.hwvirt.svm.pvMsrBitmapR0, NULL /* paPages */);
        if (RT_FAILURE(rc))
        {
            Assert(!pVCpu->cpum.s.Guest.hwvirt.svm.pvMsrBitmapR3);
            LogRel(("CPUM%u: Failed to alloc %u pages for the nested-guest's MSR permission bitmap\n", pVCpu->idCpu,
                    SVM_MSRPM_PAGES));
            break;
        }

        /*
         * Allocate the IOPM (IO Permission bitmap).
         */
        Assert(!pVCpu->cpum.s.Guest.hwvirt.svm.pvIoBitmapR3);
        rc = SUPR3PageAllocEx(SVM_IOPM_PAGES, 0 /* fFlags */, &pVCpu->cpum.s.Guest.hwvirt.svm.pvIoBitmapR3,
                              &pVCpu->cpum.s.Guest.hwvirt.svm.pvIoBitmapR0, NULL /* paPages */);
        if (RT_FAILURE(rc))
        {
            Assert(!pVCpu->cpum.s.Guest.hwvirt.svm.pvIoBitmapR3);
            LogRel(("CPUM%u: Failed to alloc %u pages for the nested-guest's IO permission bitmap\n", pVCpu->idCpu,
                    SVM_IOPM_PAGES));
            break;
        }
    }

    /* On any failure, cleanup. */
    if (RT_FAILURE(rc))
        cpumR3FreeHwVirtState(pVM);

    return rc;
}


/**
 * Initializes the CPUM.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3DECL(int) CPUMR3Init(PVM pVM)
{
    LogFlow(("CPUMR3Init\n"));

    /*
     * Assert alignment, sizes and tables.
     */
    AssertCompileMemberAlignment(VM, cpum.s, 32);
    AssertCompile(sizeof(pVM->cpum.s) <= sizeof(pVM->cpum.padding));
    AssertCompileSizeAlignment(CPUMCTX, 64);
    AssertCompileSizeAlignment(CPUMCTXMSRS, 64);
    AssertCompileSizeAlignment(CPUMHOSTCTX, 64);
    AssertCompileMemberAlignment(VM, cpum, 64);
    AssertCompileMemberAlignment(VM, aCpus, 64);
    AssertCompileMemberAlignment(VMCPU, cpum.s, 64);
    AssertCompileMemberSizeAlignment(VM, aCpus[0].cpum.s, 64);
#ifdef VBOX_STRICT
    int rc2 = cpumR3MsrStrictInitChecks();
    AssertRCReturn(rc2, rc2);
#endif

    /*
     * Initialize offsets.
     */

    /* Calculate the offset from CPUM to CPUMCPU for the first CPU. */
    pVM->cpum.s.offCPUMCPU0 = RT_OFFSETOF(VM, aCpus[0].cpum) - RT_OFFSETOF(VM, cpum);
    Assert((uintptr_t)&pVM->cpum + pVM->cpum.s.offCPUMCPU0 == (uintptr_t)&pVM->aCpus[0].cpum);


    /* Calculate the offset from CPUMCPU to CPUM. */
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU pVCpu = &pVM->aCpus[i];

        pVCpu->cpum.s.offCPUM = RT_OFFSETOF(VM, aCpus[i].cpum) - RT_OFFSETOF(VM, cpum);
        Assert((uintptr_t)&pVCpu->cpum - pVCpu->cpum.s.offCPUM == (uintptr_t)&pVM->cpum);
    }

    /*
     * Gather info about the host CPU.
     */
    if (!ASMHasCpuId())
    {
        Log(("The CPU doesn't support CPUID!\n"));
        return VERR_UNSUPPORTED_CPU;
    }

    pVM->cpum.s.fHostMxCsrMask = CPUMR3DeterminHostMxCsrMask();

    PCPUMCPUIDLEAF  paLeaves;
    uint32_t        cLeaves;
    int rc = CPUMR3CpuIdCollectLeaves(&paLeaves, &cLeaves);
    AssertLogRelRCReturn(rc, rc);

    rc = cpumR3CpuIdExplodeFeatures(paLeaves, cLeaves, &pVM->cpum.s.HostFeatures);
    RTMemFree(paLeaves);
    AssertLogRelRCReturn(rc, rc);
    pVM->cpum.s.GuestFeatures.enmCpuVendor = pVM->cpum.s.HostFeatures.enmCpuVendor;

    /*
     * Check that the CPU supports the minimum features we require.
     */
    if (!pVM->cpum.s.HostFeatures.fFxSaveRstor)
        return VMSetError(pVM, VERR_UNSUPPORTED_CPU, RT_SRC_POS, "Host CPU does not support the FXSAVE/FXRSTOR instruction.");
    if (!pVM->cpum.s.HostFeatures.fMmx)
        return VMSetError(pVM, VERR_UNSUPPORTED_CPU, RT_SRC_POS, "Host CPU does not support MMX.");
    if (!pVM->cpum.s.HostFeatures.fTsc)
        return VMSetError(pVM, VERR_UNSUPPORTED_CPU, RT_SRC_POS, "Host CPU does not support RDTSC.");

    /*
     * Setup the CR4 AND and OR masks used in the raw-mode switcher.
     */
    pVM->cpum.s.CR4.AndMask = X86_CR4_OSXMMEEXCPT | X86_CR4_PVI | X86_CR4_VME;
    pVM->cpum.s.CR4.OrMask  = X86_CR4_OSFXSR;

    /*
     * Figure out which XSAVE/XRSTOR features are available on the host.
     */
    uint64_t fXcr0Host = 0;
    uint64_t fXStateHostMask = 0;
    if (   pVM->cpum.s.HostFeatures.fXSaveRstor
        && pVM->cpum.s.HostFeatures.fOpSysXSaveRstor)
    {
        fXStateHostMask  = fXcr0Host = ASMGetXcr0();
        fXStateHostMask &= XSAVE_C_X87 | XSAVE_C_SSE | XSAVE_C_YMM | XSAVE_C_OPMASK | XSAVE_C_ZMM_HI256 | XSAVE_C_ZMM_16HI;
        AssertLogRelMsgStmt((fXStateHostMask & (XSAVE_C_X87 | XSAVE_C_SSE)) == (XSAVE_C_X87 | XSAVE_C_SSE),
                            ("%#llx\n", fXStateHostMask), fXStateHostMask = 0);
    }
    pVM->cpum.s.fXStateHostMask = fXStateHostMask;
    if (!HMIsEnabled(pVM)) /* For raw-mode, we only use XSAVE/XRSTOR when the guest starts using it (CPUID/CR4 visibility). */
        fXStateHostMask = 0;
    LogRel(("CPUM: fXStateHostMask=%#llx; initial: %#llx; host XCR0=%#llx\n",
            pVM->cpum.s.fXStateHostMask, fXStateHostMask, fXcr0Host));

    /*
     * Allocate memory for the extended CPU state and initialize the host XSAVE/XRSTOR mask.
     */
    uint32_t cbMaxXState = pVM->cpum.s.HostFeatures.cbMaxExtendedState;
    cbMaxXState = RT_ALIGN(cbMaxXState, 128);
    AssertLogRelReturn(cbMaxXState >= sizeof(X86FXSTATE) && cbMaxXState <= _8K, VERR_CPUM_IPE_2);

    uint8_t *pbXStates;
    rc = MMR3HyperAllocOnceNoRelEx(pVM, cbMaxXState * 3 * pVM->cCpus, PAGE_SIZE, MM_TAG_CPUM_CTX,
                                   MMHYPER_AONR_FLAGS_KERNEL_MAPPING, (void **)&pbXStates);
    AssertLogRelRCReturn(rc, rc);

    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU pVCpu = &pVM->aCpus[i];

        pVCpu->cpum.s.Guest.pXStateR3 = (PX86XSAVEAREA)pbXStates;
        pVCpu->cpum.s.Guest.pXStateR0 = MMHyperR3ToR0(pVM, pbXStates);
        pVCpu->cpum.s.Guest.pXStateRC = MMHyperR3ToR0(pVM, pbXStates);
        pbXStates += cbMaxXState;

        pVCpu->cpum.s.Host.pXStateR3  = (PX86XSAVEAREA)pbXStates;
        pVCpu->cpum.s.Host.pXStateR0  = MMHyperR3ToR0(pVM, pbXStates);
        pVCpu->cpum.s.Host.pXStateRC  = MMHyperR3ToR0(pVM, pbXStates);
        pbXStates += cbMaxXState;

        pVCpu->cpum.s.Hyper.pXStateR3 = (PX86XSAVEAREA)pbXStates;
        pVCpu->cpum.s.Hyper.pXStateR0 = MMHyperR3ToR0(pVM, pbXStates);
        pVCpu->cpum.s.Hyper.pXStateRC = MMHyperR3ToR0(pVM, pbXStates);
        pbXStates += cbMaxXState;

        pVCpu->cpum.s.Host.fXStateMask = fXStateHostMask;
    }

    /*
     * Register saved state data item.
     */
    rc = SSMR3RegisterInternal(pVM, "cpum", 1, CPUM_SAVED_STATE_VERSION, sizeof(CPUM),
                               NULL, cpumR3LiveExec, NULL,
                               NULL, cpumR3SaveExec, NULL,
                               cpumR3LoadPrep, cpumR3LoadExec, cpumR3LoadDone);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Register info handlers and registers with the debugger facility.
     */
    DBGFR3InfoRegisterInternalEx(pVM, "cpum",             "Displays the all the cpu states.",
                                 &cpumR3InfoAll, DBGFINFO_FLAGS_ALL_EMTS);
    DBGFR3InfoRegisterInternalEx(pVM, "cpumguest",        "Displays the guest cpu state.",
                                 &cpumR3InfoGuest, DBGFINFO_FLAGS_ALL_EMTS);
    DBGFR3InfoRegisterInternalEx(pVM, "cpumguesthwvirt",   "Displays the guest hwvirt. cpu state.",
                                 &cpumR3InfoGuestHwvirt, DBGFINFO_FLAGS_ALL_EMTS);
    DBGFR3InfoRegisterInternalEx(pVM, "cpumhyper",        "Displays the hypervisor cpu state.",
                                 &cpumR3InfoHyper, DBGFINFO_FLAGS_ALL_EMTS);
    DBGFR3InfoRegisterInternalEx(pVM, "cpumhost",         "Displays the host cpu state.",
                                 &cpumR3InfoHost, DBGFINFO_FLAGS_ALL_EMTS);
    DBGFR3InfoRegisterInternalEx(pVM, "cpumguestinstr",   "Displays the current guest instruction.",
                                 &cpumR3InfoGuestInstr, DBGFINFO_FLAGS_ALL_EMTS);
    DBGFR3InfoRegisterInternal(  pVM, "cpuid",            "Displays the guest cpuid leaves.",         &cpumR3CpuIdInfo);

    rc = cpumR3DbgInit(pVM);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Check if we need to workaround partial/leaky FPU handling.
     */
    cpumR3CheckLeakyFpu(pVM);

    /*
     * Initialize the Guest CPUID and MSR states.
     */
    rc = cpumR3InitCpuIdAndMsrs(pVM);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Allocate memory required by the guest hardware virtualization state.
     */
    if (pVM->cpum.ro.GuestFeatures.fSvm)
    {
        rc = cpumR3AllocHwVirtState(pVM);
        if (RT_FAILURE(rc))
            return rc;
    }

    CPUMR3Reset(pVM);
    return VINF_SUCCESS;
}


/**
 * Applies relocations to data and code managed by this
 * component. This function will be called at init and
 * whenever the VMM need to relocate it self inside the GC.
 *
 * The CPUM will update the addresses used by the switcher.
 *
 * @param   pVM     The cross context VM structure.
 */
VMMR3DECL(void) CPUMR3Relocate(PVM pVM)
{
    LogFlow(("CPUMR3Relocate\n"));

    pVM->cpum.s.GuestInfo.paMsrRangesRC   = MMHyperR3ToRC(pVM, pVM->cpum.s.GuestInfo.paMsrRangesR3);
    pVM->cpum.s.GuestInfo.paCpuIdLeavesRC = MMHyperR3ToRC(pVM, pVM->cpum.s.GuestInfo.paCpuIdLeavesR3);

    for (VMCPUID iCpu = 0; iCpu < pVM->cCpus; iCpu++)
    {
        PVMCPU pVCpu = &pVM->aCpus[iCpu];
        pVCpu->cpum.s.Guest.pXStateRC = MMHyperR3ToRC(pVM, pVCpu->cpum.s.Guest.pXStateR3);
        pVCpu->cpum.s.Host.pXStateRC  = MMHyperR3ToRC(pVM, pVCpu->cpum.s.Host.pXStateR3);
        pVCpu->cpum.s.Hyper.pXStateRC = MMHyperR3ToRC(pVM, pVCpu->cpum.s.Hyper.pXStateR3); /** @todo remove me */

        /* Recheck the guest DRx values in raw-mode. */
        CPUMRecalcHyperDRx(pVCpu, UINT8_MAX, false);
    }
}


/**
 * Apply late CPUM property changes based on the fHWVirtEx setting
 *
 * @param   pVM                 The cross context VM structure.
 * @param   fHWVirtExEnabled    HWVirtEx enabled/disabled
 */
VMMR3DECL(void) CPUMR3SetHWVirtEx(PVM pVM, bool fHWVirtExEnabled)
{
    /*
     * Workaround for missing cpuid(0) patches when leaf 4 returns GuestInfo.DefCpuId:
     * If we miss to patch a cpuid(0).eax then Linux tries to determine the number
     * of processors from (cpuid(4).eax >> 26) + 1.
     *
     * Note: this code is obsolete, but let's keep it here for reference.
     *       Purpose is valid when we artificially cap the max std id to less than 4.
     */
    if (!fHWVirtExEnabled)
    {
        Assert(   (pVM->cpum.s.aGuestCpuIdPatmStd[4].uEax & UINT32_C(0xffffc000)) == 0
               || pVM->cpum.s.aGuestCpuIdPatmStd[0].uEax < 0x4);
        pVM->cpum.s.aGuestCpuIdPatmStd[4].uEax &= UINT32_C(0x00003fff);
    }
}

/**
 * Terminates the CPUM.
 *
 * Termination means cleaning up and freeing all resources,
 * the VM it self is at this point powered off or suspended.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3DECL(int) CPUMR3Term(PVM pVM)
{
#ifdef VBOX_WITH_CRASHDUMP_MAGIC
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU   pVCpu = &pVM->aCpus[i];
        PCPUMCTX pCtx  = CPUMQueryGuestCtxPtr(pVCpu);

        memset(pVCpu->cpum.s.aMagic, 0, sizeof(pVCpu->cpum.s.aMagic));
        pVCpu->cpum.s.uMagic     = 0;
        pCtx->dr[5]              = 0;
    }
#endif

    if (pVM->cpum.ro.GuestFeatures.fSvm)
        cpumR3FreeHwVirtState(pVM);
    return VINF_SUCCESS;
}


/**
 * Resets a virtual CPU.
 *
 * Used by CPUMR3Reset and CPU hot plugging.
 *
 * @param   pVM     The cross context VM structure.
 * @param   pVCpu   The cross context virtual CPU structure of the CPU that is
 *                  being reset.  This may differ from the current EMT.
 */
VMMR3DECL(void) CPUMR3ResetCpu(PVM pVM, PVMCPU pVCpu)
{
    /** @todo anything different for VCPU > 0? */
    PCPUMCTX pCtx = &pVCpu->cpum.s.Guest;

    /*
     * Initialize everything to ZERO first.
     */
    uint32_t fUseFlags              =  pVCpu->cpum.s.fUseFlags & ~CPUM_USED_FPU_SINCE_REM;

    AssertCompile(RTASSERT_OFFSET_OF(CPUMCTX, pXStateR0) < RTASSERT_OFFSET_OF(CPUMCTX, pXStateR3));
    AssertCompile(RTASSERT_OFFSET_OF(CPUMCTX, pXStateR0) < RTASSERT_OFFSET_OF(CPUMCTX, pXStateRC));
    memset(pCtx, 0, RT_OFFSETOF(CPUMCTX, pXStateR0));

    pVCpu->cpum.s.fUseFlags         = fUseFlags;

    pCtx->cr0                       = X86_CR0_CD | X86_CR0_NW | X86_CR0_ET;  //0x60000010
    pCtx->eip                       = 0x0000fff0;
    pCtx->edx                       = 0x00000600;   /* P6 processor */
    pCtx->eflags.Bits.u1Reserved0   = 1;

    pCtx->cs.Sel                    = 0xf000;
    pCtx->cs.ValidSel               = 0xf000;
    pCtx->cs.fFlags                 = CPUMSELREG_FLAGS_VALID;
    pCtx->cs.u64Base                = UINT64_C(0xffff0000);
    pCtx->cs.u32Limit               = 0x0000ffff;
    pCtx->cs.Attr.n.u1DescType      = 1; /* code/data segment */
    pCtx->cs.Attr.n.u1Present       = 1;
    pCtx->cs.Attr.n.u4Type          = X86_SEL_TYPE_ER_ACC;

    pCtx->ds.fFlags                 = CPUMSELREG_FLAGS_VALID;
    pCtx->ds.u32Limit               = 0x0000ffff;
    pCtx->ds.Attr.n.u1DescType      = 1; /* code/data segment */
    pCtx->ds.Attr.n.u1Present       = 1;
    pCtx->ds.Attr.n.u4Type          = X86_SEL_TYPE_RW_ACC;

    pCtx->es.fFlags                 = CPUMSELREG_FLAGS_VALID;
    pCtx->es.u32Limit               = 0x0000ffff;
    pCtx->es.Attr.n.u1DescType      = 1; /* code/data segment */
    pCtx->es.Attr.n.u1Present       = 1;
    pCtx->es.Attr.n.u4Type          = X86_SEL_TYPE_RW_ACC;

    pCtx->fs.fFlags                 = CPUMSELREG_FLAGS_VALID;
    pCtx->fs.u32Limit               = 0x0000ffff;
    pCtx->fs.Attr.n.u1DescType      = 1; /* code/data segment */
    pCtx->fs.Attr.n.u1Present       = 1;
    pCtx->fs.Attr.n.u4Type          = X86_SEL_TYPE_RW_ACC;

    pCtx->gs.fFlags                 = CPUMSELREG_FLAGS_VALID;
    pCtx->gs.u32Limit               = 0x0000ffff;
    pCtx->gs.Attr.n.u1DescType      = 1; /* code/data segment */
    pCtx->gs.Attr.n.u1Present       = 1;
    pCtx->gs.Attr.n.u4Type          = X86_SEL_TYPE_RW_ACC;

    pCtx->ss.fFlags                 = CPUMSELREG_FLAGS_VALID;
    pCtx->ss.u32Limit               = 0x0000ffff;
    pCtx->ss.Attr.n.u1Present       = 1;
    pCtx->ss.Attr.n.u1DescType      = 1; /* code/data segment */
    pCtx->ss.Attr.n.u4Type          = X86_SEL_TYPE_RW_ACC;

    pCtx->idtr.cbIdt                = 0xffff;
    pCtx->gdtr.cbGdt                = 0xffff;

    pCtx->ldtr.fFlags               = CPUMSELREG_FLAGS_VALID;
    pCtx->ldtr.u32Limit             = 0xffff;
    pCtx->ldtr.Attr.n.u1Present     = 1;
    pCtx->ldtr.Attr.n.u4Type        = X86_SEL_TYPE_SYS_LDT;

    pCtx->tr.fFlags                 = CPUMSELREG_FLAGS_VALID;
    pCtx->tr.u32Limit               = 0xffff;
    pCtx->tr.Attr.n.u1Present       = 1;
    pCtx->tr.Attr.n.u4Type          = X86_SEL_TYPE_SYS_386_TSS_BUSY;    /* Deduction, not properly documented by Intel. */

    pCtx->dr[6]                     = X86_DR6_INIT_VAL;
    pCtx->dr[7]                     = X86_DR7_INIT_VAL;

    PX86FXSTATE pFpuCtx = &pCtx->pXStateR3->x87; AssertReleaseMsg(RT_VALID_PTR(pFpuCtx), ("%p\n", pFpuCtx));
    pFpuCtx->FTW                    = 0x00;         /* All empty (abbridged tag reg edition). */
    pFpuCtx->FCW                    = 0x37f;

    /* Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3A, Table 8-1.
       IA-32 Processor States Following Power-up, Reset, or INIT */
    pFpuCtx->MXCSR                  = 0x1F80;
    pFpuCtx->MXCSR_MASK             = pVM->cpum.s.GuestInfo.fMxCsrMask; /** @todo check if REM messes this up... */

    pCtx->aXcr[0]                   = XSAVE_C_X87;
    if (pVM->cpum.s.HostFeatures.cbMaxExtendedState >= RT_OFFSETOF(X86XSAVEAREA, Hdr))
    {
        /* The entire FXSAVE state needs loading when we switch to XSAVE/XRSTOR
           as we don't know what happened before.  (Bother optimize later?) */
        pCtx->pXStateR3->Hdr.bmXState = XSAVE_C_X87 | XSAVE_C_SSE;
    }

    /*
     * MSRs.
     */
    /* Init PAT MSR */
    pCtx->msrPAT                    = UINT64_C(0x0007040600070406); /** @todo correct? */

    /* EFER MBZ; see AMD64 Architecture Programmer's Manual Volume 2: Table 14-1. Initial Processor State.
     * The Intel docs don't mention it. */
    Assert(!pCtx->msrEFER);

    /* IA32_MISC_ENABLE - not entirely sure what the init/reset state really
       is supposed to be here, just trying provide useful/sensible values. */
    PCPUMMSRRANGE pRange = cpumLookupMsrRange(pVM, MSR_IA32_MISC_ENABLE);
    if (pRange)
    {
        pVCpu->cpum.s.GuestMsrs.msr.MiscEnable = MSR_IA32_MISC_ENABLE_BTS_UNAVAIL
                                               | MSR_IA32_MISC_ENABLE_PEBS_UNAVAIL
                                               | (pVM->cpum.s.GuestFeatures.fMonitorMWait ? MSR_IA32_MISC_ENABLE_MONITOR : 0)
                                               | MSR_IA32_MISC_ENABLE_FAST_STRINGS;
        pRange->fWrIgnMask |= MSR_IA32_MISC_ENABLE_BTS_UNAVAIL
                            | MSR_IA32_MISC_ENABLE_PEBS_UNAVAIL;
        pRange->fWrGpMask  &= ~pVCpu->cpum.s.GuestMsrs.msr.MiscEnable;
    }

    /** @todo Wire IA32_MISC_ENABLE bit 22 to our NT 4 CPUID trick. */

    /** @todo r=ramshankar: Currently broken for SMP as TMCpuTickSet() expects to be
     *        called from each EMT while we're getting called by CPUMR3Reset()
     *        iteratively on the same thread. Fix later.  */
#if 0 /** @todo r=bird: This we will do in TM, not here. */
    /* TSC must be 0. Intel spec. Table 9-1. "IA-32 Processor States Following Power-up, Reset, or INIT." */
    CPUMSetGuestMsr(pVCpu, MSR_IA32_TSC, 0);
#endif


    /* C-state control. Guesses. */
    pVCpu->cpum.s.GuestMsrs.msr.PkgCStateCfgCtrl = 1 /*C1*/ | RT_BIT_32(25) | RT_BIT_32(26) | RT_BIT_32(27) | RT_BIT_32(28);
    /* For Nehalem+ and Atoms, the 0xE2 MSR (MSR_PKG_CST_CONFIG_CONTROL) is documented. For Core 2,
     * it's undocumented but exists as MSR_PMG_CST_CONFIG_CONTROL and has similar but not identical
     * functionality. The default value must be different due to incompatible write mask.
     */
    if (CPUMMICROARCH_IS_INTEL_CORE2(pVM->cpum.s.GuestFeatures.enmMicroarch))
        pVCpu->cpum.s.GuestMsrs.msr.PkgCStateCfgCtrl = 0x202a01;    /* From Mac Pro Harpertown, unlocked. */

    /*
     * Hardware virtualization state.
     */
    /* SVM. */
    if (pCtx->hwvirt.svm.CTX_SUFF(pVmcb))
        memset(pCtx->hwvirt.svm.CTX_SUFF(pVmcb), 0, SVM_VMCB_PAGES << PAGE_SHIFT);
    pCtx->hwvirt.svm.uMsrHSavePa = 0;
    pCtx->hwvirt.svm.GCPhysVmcb = 0;
    pCtx->hwvirt.svm.fGif = 1;
}


/**
 * Resets the CPU.
 *
 * @returns VINF_SUCCESS.
 * @param   pVM         The cross context VM structure.
 */
VMMR3DECL(void) CPUMR3Reset(PVM pVM)
{
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        CPUMR3ResetCpu(pVM, &pVM->aCpus[i]);

#ifdef VBOX_WITH_CRASHDUMP_MAGIC
        PCPUMCTX pCtx = &pVM->aCpus[i].cpum.s.Guest;

        /* Magic marker for searching in crash dumps. */
        strcpy((char *)pVM->aCpus[i].cpum.s.aMagic, "CPUMCPU Magic");
        pVM->aCpus[i].cpum.s.uMagic     = UINT64_C(0xDEADBEEFDEADBEEF);
        pCtx->dr[5]                     = UINT64_C(0xDEADBEEFDEADBEEF);
#endif
    }
}




/**
 * Pass 0 live exec callback.
 *
 * @returns VINF_SSM_DONT_CALL_AGAIN.
 * @param   pVM                 The cross context VM structure.
 * @param   pSSM                The saved state handle.
 * @param   uPass               The pass (0).
 */
static DECLCALLBACK(int) cpumR3LiveExec(PVM pVM, PSSMHANDLE pSSM, uint32_t uPass)
{
    AssertReturn(uPass == 0, VERR_SSM_UNEXPECTED_PASS);
    cpumR3SaveCpuId(pVM, pSSM);
    return VINF_SSM_DONT_CALL_AGAIN;
}


/**
 * Execute state save operation.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pSSM            SSM operation handle.
 */
static DECLCALLBACK(int) cpumR3SaveExec(PVM pVM, PSSMHANDLE pSSM)
{
    /*
     * Save.
     */
    SSMR3PutU32(pSSM, pVM->cCpus);
    SSMR3PutU32(pSSM, sizeof(pVM->aCpus[0].cpum.s.GuestMsrs.msr));
    for (VMCPUID iCpu = 0; iCpu < pVM->cCpus; iCpu++)
    {
        PVMCPU pVCpu = &pVM->aCpus[iCpu];

        SSMR3PutStructEx(pSSM, &pVCpu->cpum.s.Hyper,     sizeof(pVCpu->cpum.s.Hyper),     0, g_aCpumCtxFields, NULL);

        PCPUMCTX pGstCtx = &pVCpu->cpum.s.Guest;
        SSMR3PutStructEx(pSSM, pGstCtx,                  sizeof(*pGstCtx),                0, g_aCpumCtxFields, NULL);
        SSMR3PutStructEx(pSSM, &pGstCtx->pXStateR3->x87, sizeof(pGstCtx->pXStateR3->x87), 0, g_aCpumX87Fields, NULL);
        if (pGstCtx->fXStateMask != 0)
            SSMR3PutStructEx(pSSM, &pGstCtx->pXStateR3->Hdr, sizeof(pGstCtx->pXStateR3->Hdr), 0, g_aCpumXSaveHdrFields, NULL);
        if (pGstCtx->fXStateMask & XSAVE_C_YMM)
        {
            PCX86XSAVEYMMHI pYmmHiCtx = CPUMCTX_XSAVE_C_PTR(pGstCtx, XSAVE_C_YMM_BIT, PCX86XSAVEYMMHI);
            SSMR3PutStructEx(pSSM, pYmmHiCtx, sizeof(*pYmmHiCtx), SSMSTRUCT_FLAGS_FULL_STRUCT, g_aCpumYmmHiFields, NULL);
        }
        if (pGstCtx->fXStateMask & XSAVE_C_BNDREGS)
        {
            PCX86XSAVEBNDREGS pBndRegs = CPUMCTX_XSAVE_C_PTR(pGstCtx, XSAVE_C_BNDREGS_BIT, PCX86XSAVEBNDREGS);
            SSMR3PutStructEx(pSSM, pBndRegs, sizeof(*pBndRegs), SSMSTRUCT_FLAGS_FULL_STRUCT, g_aCpumBndRegsFields, NULL);
        }
        if (pGstCtx->fXStateMask & XSAVE_C_BNDCSR)
        {
            PCX86XSAVEBNDCFG pBndCfg = CPUMCTX_XSAVE_C_PTR(pGstCtx, XSAVE_C_BNDCSR_BIT, PCX86XSAVEBNDCFG);
            SSMR3PutStructEx(pSSM, pBndCfg, sizeof(*pBndCfg), SSMSTRUCT_FLAGS_FULL_STRUCT, g_aCpumBndCfgFields, NULL);
        }
        if (pGstCtx->fXStateMask & XSAVE_C_ZMM_HI256)
        {
            PCX86XSAVEZMMHI256 pZmmHi256 = CPUMCTX_XSAVE_C_PTR(pGstCtx, XSAVE_C_ZMM_HI256_BIT, PCX86XSAVEZMMHI256);
            SSMR3PutStructEx(pSSM, pZmmHi256, sizeof(*pZmmHi256), SSMSTRUCT_FLAGS_FULL_STRUCT, g_aCpumZmmHi256Fields, NULL);
        }
        if (pGstCtx->fXStateMask & XSAVE_C_ZMM_16HI)
        {
            PCX86XSAVEZMM16HI pZmm16Hi = CPUMCTX_XSAVE_C_PTR(pGstCtx, XSAVE_C_ZMM_16HI_BIT, PCX86XSAVEZMM16HI);
            SSMR3PutStructEx(pSSM, pZmm16Hi, sizeof(*pZmm16Hi), SSMSTRUCT_FLAGS_FULL_STRUCT, g_aCpumZmm16HiFields, NULL);
        }

        SSMR3PutU32(pSSM, pVCpu->cpum.s.fUseFlags);
        SSMR3PutU32(pSSM, pVCpu->cpum.s.fChanged);
        AssertCompileSizeAlignment(pVCpu->cpum.s.GuestMsrs.msr, sizeof(uint64_t));
        SSMR3PutMem(pSSM, &pVCpu->cpum.s.GuestMsrs, sizeof(pVCpu->cpum.s.GuestMsrs.msr));
    }

    cpumR3SaveCpuId(pVM, pSSM);
    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{FNSSMINTLOADPREP}
 */
static DECLCALLBACK(int) cpumR3LoadPrep(PVM pVM, PSSMHANDLE pSSM)
{
    NOREF(pSSM);
    pVM->cpum.s.fPendingRestore = true;
    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{FNSSMINTLOADEXEC}
 */
static DECLCALLBACK(int) cpumR3LoadExec(PVM pVM, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    int rc; /* Only for AssertRCReturn use. */

    /*
     * Validate version.
     */
    if (    uVersion != CPUM_SAVED_STATE_VERSION_XSAVE
        &&  uVersion != CPUM_SAVED_STATE_VERSION_GOOD_CPUID_COUNT
        &&  uVersion != CPUM_SAVED_STATE_VERSION_BAD_CPUID_COUNT
        &&  uVersion != CPUM_SAVED_STATE_VERSION_PUT_STRUCT
        &&  uVersion != CPUM_SAVED_STATE_VERSION_MEM
        &&  uVersion != CPUM_SAVED_STATE_VERSION_NO_MSR_SIZE
        &&  uVersion != CPUM_SAVED_STATE_VERSION_VER3_2
        &&  uVersion != CPUM_SAVED_STATE_VERSION_VER3_0
        &&  uVersion != CPUM_SAVED_STATE_VERSION_VER2_1_NOMSR
        &&  uVersion != CPUM_SAVED_STATE_VERSION_VER2_0
        &&  uVersion != CPUM_SAVED_STATE_VERSION_VER1_6)
    {
        AssertMsgFailed(("cpumR3LoadExec: Invalid version uVersion=%d!\n", uVersion));
        return VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;
    }

    if (uPass == SSM_PASS_FINAL)
    {
        /*
         * Set the size of RTGCPTR for SSMR3GetGCPtr. (Only necessary for
         * really old SSM file versions.)
         */
        if (uVersion == CPUM_SAVED_STATE_VERSION_VER1_6)
            SSMR3HandleSetGCPtrSize(pSSM, sizeof(RTGCPTR32));
        else if (uVersion <= CPUM_SAVED_STATE_VERSION_VER3_0)
            SSMR3HandleSetGCPtrSize(pSSM, HC_ARCH_BITS == 32 ? sizeof(RTGCPTR32) : sizeof(RTGCPTR));

        /*
         * Figure x86 and ctx field definitions to use for older states.
         */
        uint32_t const  fLoad = uVersion > CPUM_SAVED_STATE_VERSION_MEM ? 0 : SSMSTRUCT_FLAGS_MEM_BAND_AID_RELAXED;
        PCSSMFIELD      paCpumCtx1Fields = g_aCpumX87Fields;
        PCSSMFIELD      paCpumCtx2Fields = g_aCpumCtxFields;
        if (uVersion == CPUM_SAVED_STATE_VERSION_VER1_6)
        {
            paCpumCtx1Fields = g_aCpumX87FieldsV16;
            paCpumCtx2Fields = g_aCpumCtxFieldsV16;
        }
        else if (uVersion <= CPUM_SAVED_STATE_VERSION_MEM)
        {
            paCpumCtx1Fields = g_aCpumX87FieldsMem;
            paCpumCtx2Fields = g_aCpumCtxFieldsMem;
        }

        /*
         * The hyper state used to preceed the CPU count.  Starting with
         * XSAVE it was moved down till after we've got the count.
         */
        if (uVersion < CPUM_SAVED_STATE_VERSION_XSAVE)
        {
            for (VMCPUID iCpu = 0; iCpu < pVM->cCpus; iCpu++)
            {
                PVMCPU      pVCpu = &pVM->aCpus[iCpu];
                X86FXSTATE  Ign;
                SSMR3GetStructEx(pSSM, &Ign, sizeof(Ign), fLoad | SSMSTRUCT_FLAGS_NO_TAIL_MARKER, paCpumCtx1Fields, NULL);
                uint64_t    uCR3  = pVCpu->cpum.s.Hyper.cr3;
                uint64_t    uRSP  = pVCpu->cpum.s.Hyper.rsp; /* see VMMR3Relocate(). */
                SSMR3GetStructEx(pSSM, &pVCpu->cpum.s.Hyper, sizeof(pVCpu->cpum.s.Hyper),
                                 fLoad | SSMSTRUCT_FLAGS_NO_LEAD_MARKER, paCpumCtx2Fields, NULL);
                pVCpu->cpum.s.Hyper.cr3 = uCR3;
                pVCpu->cpum.s.Hyper.rsp = uRSP;
            }
        }

        if (uVersion >= CPUM_SAVED_STATE_VERSION_VER2_1_NOMSR)
        {
            uint32_t cCpus;
            rc = SSMR3GetU32(pSSM, &cCpus); AssertRCReturn(rc, rc);
            AssertLogRelMsgReturn(cCpus == pVM->cCpus, ("Mismatching CPU counts: saved: %u; configured: %u \n", cCpus, pVM->cCpus),
                                  VERR_SSM_UNEXPECTED_DATA);
        }
        AssertLogRelMsgReturn(   uVersion > CPUM_SAVED_STATE_VERSION_VER2_0
                              || pVM->cCpus == 1,
                              ("cCpus=%u\n", pVM->cCpus),
                              VERR_SSM_UNEXPECTED_DATA);

        uint32_t cbMsrs = 0;
        if (uVersion > CPUM_SAVED_STATE_VERSION_NO_MSR_SIZE)
        {
            rc = SSMR3GetU32(pSSM, &cbMsrs); AssertRCReturn(rc, rc);
            AssertLogRelMsgReturn(RT_ALIGN(cbMsrs, sizeof(uint64_t)) == cbMsrs, ("Size of MSRs is misaligned: %#x\n", cbMsrs),
                                  VERR_SSM_UNEXPECTED_DATA);
            AssertLogRelMsgReturn(cbMsrs <= sizeof(CPUMCTXMSRS) && cbMsrs > 0,  ("Size of MSRs is out of range: %#x\n", cbMsrs),
                                  VERR_SSM_UNEXPECTED_DATA);
        }

        /*
         * Do the per-CPU restoring.
         */
        for (VMCPUID iCpu = 0; iCpu < pVM->cCpus; iCpu++)
        {
            PVMCPU   pVCpu = &pVM->aCpus[iCpu];
            PCPUMCTX pGstCtx = &pVCpu->cpum.s.Guest;

            if (uVersion >= CPUM_SAVED_STATE_VERSION_XSAVE)
            {
                /*
                 * The XSAVE saved state layout moved the hyper state down here.
                 */
                uint64_t    uCR3  = pVCpu->cpum.s.Hyper.cr3;
                uint64_t    uRSP  = pVCpu->cpum.s.Hyper.rsp; /* see VMMR3Relocate(). */
                rc = SSMR3GetStructEx(pSSM, &pVCpu->cpum.s.Hyper,     sizeof(pVCpu->cpum.s.Hyper),     0, g_aCpumCtxFields, NULL);
                pVCpu->cpum.s.Hyper.cr3 = uCR3;
                pVCpu->cpum.s.Hyper.rsp = uRSP;
                AssertRCReturn(rc, rc);

                /*
                 * Start by restoring the CPUMCTX structure and the X86FXSAVE bits of the extended state.
                 */
                rc = SSMR3GetStructEx(pSSM, pGstCtx,                  sizeof(*pGstCtx),                0, g_aCpumCtxFields, NULL);
                rc = SSMR3GetStructEx(pSSM, &pGstCtx->pXStateR3->x87, sizeof(pGstCtx->pXStateR3->x87), 0, g_aCpumX87Fields, NULL);
                AssertRCReturn(rc, rc);

                /* Check that the xsave/xrstor mask is valid (invalid results in #GP). */
                if (pGstCtx->fXStateMask != 0)
                {
                    AssertLogRelMsgReturn(!(pGstCtx->fXStateMask & ~pVM->cpum.s.fXStateGuestMask),
                                          ("fXStateMask=%#RX64 fXStateGuestMask=%#RX64\n",
                                           pGstCtx->fXStateMask, pVM->cpum.s.fXStateGuestMask),
                                          VERR_CPUM_INCOMPATIBLE_XSAVE_COMP_MASK);
                    AssertLogRelMsgReturn(pGstCtx->fXStateMask & XSAVE_C_X87,
                                          ("fXStateMask=%#RX64\n", pGstCtx->fXStateMask), VERR_CPUM_INVALID_XSAVE_COMP_MASK);
                    AssertLogRelMsgReturn((pGstCtx->fXStateMask & (XSAVE_C_SSE | XSAVE_C_YMM)) != XSAVE_C_YMM,
                                          ("fXStateMask=%#RX64\n", pGstCtx->fXStateMask), VERR_CPUM_INVALID_XSAVE_COMP_MASK);
                    AssertLogRelMsgReturn(   (pGstCtx->fXStateMask & (XSAVE_C_OPMASK | XSAVE_C_ZMM_HI256 | XSAVE_C_ZMM_16HI)) == 0
                                          ||    (pGstCtx->fXStateMask & (XSAVE_C_SSE | XSAVE_C_YMM | XSAVE_C_OPMASK | XSAVE_C_ZMM_HI256 | XSAVE_C_ZMM_16HI))
                                             ==                         (XSAVE_C_SSE | XSAVE_C_YMM | XSAVE_C_OPMASK | XSAVE_C_ZMM_HI256 | XSAVE_C_ZMM_16HI),
                                          ("fXStateMask=%#RX64\n", pGstCtx->fXStateMask), VERR_CPUM_INVALID_XSAVE_COMP_MASK);
                }

                /* Check that the XCR0 mask is valid (invalid results in #GP). */
                AssertLogRelMsgReturn(pGstCtx->aXcr[0] & XSAVE_C_X87, ("xcr0=%#RX64\n", pGstCtx->aXcr[0]), VERR_CPUM_INVALID_XCR0);
                if (pGstCtx->aXcr[0] != XSAVE_C_X87)
                {
                    AssertLogRelMsgReturn(!(pGstCtx->aXcr[0] & ~(pGstCtx->fXStateMask | XSAVE_C_X87)),
                                          ("xcr0=%#RX64 fXStateMask=%#RX64\n", pGstCtx->aXcr[0], pGstCtx->fXStateMask),
                                          VERR_CPUM_INVALID_XCR0);
                    AssertLogRelMsgReturn(pGstCtx->aXcr[0] & XSAVE_C_X87,
                                          ("xcr0=%#RX64\n", pGstCtx->aXcr[0]), VERR_CPUM_INVALID_XSAVE_COMP_MASK);
                    AssertLogRelMsgReturn((pGstCtx->aXcr[0] & (XSAVE_C_SSE | XSAVE_C_YMM)) != XSAVE_C_YMM,
                                          ("xcr0=%#RX64\n", pGstCtx->aXcr[0]), VERR_CPUM_INVALID_XSAVE_COMP_MASK);
                    AssertLogRelMsgReturn(   (pGstCtx->aXcr[0] & (XSAVE_C_OPMASK | XSAVE_C_ZMM_HI256 | XSAVE_C_ZMM_16HI)) == 0
                                          ||    (pGstCtx->aXcr[0] & (XSAVE_C_SSE | XSAVE_C_YMM | XSAVE_C_OPMASK | XSAVE_C_ZMM_HI256 | XSAVE_C_ZMM_16HI))
                                             ==                     (XSAVE_C_SSE | XSAVE_C_YMM | XSAVE_C_OPMASK | XSAVE_C_ZMM_HI256 | XSAVE_C_ZMM_16HI),
                                          ("xcr0=%#RX64\n", pGstCtx->aXcr[0]), VERR_CPUM_INVALID_XSAVE_COMP_MASK);
                }

                /* Check that the XCR1 is zero, as we don't implement it yet. */
                AssertLogRelMsgReturn(!pGstCtx->aXcr[1], ("xcr1=%#RX64\n", pGstCtx->aXcr[1]), VERR_SSM_DATA_UNIT_FORMAT_CHANGED);

                /*
                 * Restore the individual extended state components we support.
                 */
                if (pGstCtx->fXStateMask != 0)
                {
                    rc = SSMR3GetStructEx(pSSM, &pGstCtx->pXStateR3->Hdr, sizeof(pGstCtx->pXStateR3->Hdr),
                                          0, g_aCpumXSaveHdrFields, NULL);
                    AssertRCReturn(rc, rc);
                    AssertLogRelMsgReturn(!(pGstCtx->pXStateR3->Hdr.bmXState & ~pGstCtx->fXStateMask),
                                          ("bmXState=%#RX64 fXStateMask=%#RX64\n",
                                           pGstCtx->pXStateR3->Hdr.bmXState, pGstCtx->fXStateMask),
                                          VERR_CPUM_INVALID_XSAVE_HDR);
                }
                if (pGstCtx->fXStateMask & XSAVE_C_YMM)
                {
                    PX86XSAVEYMMHI pYmmHiCtx = CPUMCTX_XSAVE_C_PTR(pGstCtx, XSAVE_C_YMM_BIT, PX86XSAVEYMMHI);
                    SSMR3GetStructEx(pSSM, pYmmHiCtx, sizeof(*pYmmHiCtx), SSMSTRUCT_FLAGS_FULL_STRUCT, g_aCpumYmmHiFields, NULL);
                }
                if (pGstCtx->fXStateMask & XSAVE_C_BNDREGS)
                {
                    PX86XSAVEBNDREGS pBndRegs = CPUMCTX_XSAVE_C_PTR(pGstCtx, XSAVE_C_BNDREGS_BIT, PX86XSAVEBNDREGS);
                    SSMR3GetStructEx(pSSM, pBndRegs, sizeof(*pBndRegs), SSMSTRUCT_FLAGS_FULL_STRUCT, g_aCpumBndRegsFields, NULL);
                }
                if (pGstCtx->fXStateMask & XSAVE_C_BNDCSR)
                {
                    PX86XSAVEBNDCFG pBndCfg = CPUMCTX_XSAVE_C_PTR(pGstCtx, XSAVE_C_BNDCSR_BIT, PX86XSAVEBNDCFG);
                    SSMR3GetStructEx(pSSM, pBndCfg, sizeof(*pBndCfg), SSMSTRUCT_FLAGS_FULL_STRUCT, g_aCpumBndCfgFields, NULL);
                }
                if (pGstCtx->fXStateMask & XSAVE_C_ZMM_HI256)
                {
                    PX86XSAVEZMMHI256 pZmmHi256 = CPUMCTX_XSAVE_C_PTR(pGstCtx, XSAVE_C_ZMM_HI256_BIT, PX86XSAVEZMMHI256);
                    SSMR3GetStructEx(pSSM, pZmmHi256, sizeof(*pZmmHi256), SSMSTRUCT_FLAGS_FULL_STRUCT, g_aCpumZmmHi256Fields, NULL);
                }
                if (pGstCtx->fXStateMask & XSAVE_C_ZMM_16HI)
                {
                    PX86XSAVEZMM16HI pZmm16Hi = CPUMCTX_XSAVE_C_PTR(pGstCtx, XSAVE_C_ZMM_16HI_BIT, PX86XSAVEZMM16HI);
                    SSMR3GetStructEx(pSSM, pZmm16Hi, sizeof(*pZmm16Hi), SSMSTRUCT_FLAGS_FULL_STRUCT, g_aCpumZmm16HiFields, NULL);
                }
            }
            else
            {
                /*
                 * Pre XSAVE saved state.
                 */
                SSMR3GetStructEx(pSSM, &pGstCtx->pXStateR3->x87, sizeof(pGstCtx->pXStateR3->x87),
                                 fLoad | SSMSTRUCT_FLAGS_NO_TAIL_MARKER, paCpumCtx1Fields, NULL);
                SSMR3GetStructEx(pSSM, pGstCtx, sizeof(*pGstCtx), fLoad | SSMSTRUCT_FLAGS_NO_LEAD_MARKER, paCpumCtx2Fields, NULL);
            }

            /*
             * Restore a couple of flags and the MSRs.
             */
            SSMR3GetU32(pSSM, &pVCpu->cpum.s.fUseFlags);
            SSMR3GetU32(pSSM, &pVCpu->cpum.s.fChanged);

            rc = VINF_SUCCESS;
            if (uVersion > CPUM_SAVED_STATE_VERSION_NO_MSR_SIZE)
                rc = SSMR3GetMem(pSSM, &pVCpu->cpum.s.GuestMsrs.au64[0], cbMsrs);
            else if (uVersion >= CPUM_SAVED_STATE_VERSION_VER3_0)
            {
                SSMR3GetMem(pSSM, &pVCpu->cpum.s.GuestMsrs.au64[0], 2 * sizeof(uint64_t)); /* Restore two MSRs. */
                rc = SSMR3Skip(pSSM, 62 * sizeof(uint64_t));
            }
            AssertRCReturn(rc, rc);

            /* REM and other may have cleared must-be-one fields in DR6 and
               DR7, fix these. */
            pGstCtx->dr[6] &= ~(X86_DR6_RAZ_MASK | X86_DR6_MBZ_MASK);
            pGstCtx->dr[6] |= X86_DR6_RA1_MASK;
            pGstCtx->dr[7] &= ~(X86_DR7_RAZ_MASK | X86_DR7_MBZ_MASK);
            pGstCtx->dr[7] |= X86_DR7_RA1_MASK;
        }

        /* Older states does not have the internal selector register flags
           and valid selector value.  Supply those. */
        if (uVersion <= CPUM_SAVED_STATE_VERSION_MEM)
        {
            for (VMCPUID iCpu = 0; iCpu < pVM->cCpus; iCpu++)
            {
                PVMCPU      pVCpu  = &pVM->aCpus[iCpu];
                bool const  fValid = HMIsEnabled(pVM)
                                  || (   uVersion > CPUM_SAVED_STATE_VERSION_VER3_2
                                      && !(pVCpu->cpum.s.fChanged & CPUM_CHANGED_HIDDEN_SEL_REGS_INVALID));
                PCPUMSELREG paSelReg = CPUMCTX_FIRST_SREG(&pVCpu->cpum.s.Guest);
                if (fValid)
                {
                    for (uint32_t iSelReg = 0; iSelReg < X86_SREG_COUNT; iSelReg++)
                    {
                        paSelReg[iSelReg].fFlags   = CPUMSELREG_FLAGS_VALID;
                        paSelReg[iSelReg].ValidSel = paSelReg[iSelReg].Sel;
                    }

                    pVCpu->cpum.s.Guest.ldtr.fFlags   = CPUMSELREG_FLAGS_VALID;
                    pVCpu->cpum.s.Guest.ldtr.ValidSel = pVCpu->cpum.s.Guest.ldtr.Sel;
                }
                else
                {
                    for (uint32_t iSelReg = 0; iSelReg < X86_SREG_COUNT; iSelReg++)
                    {
                        paSelReg[iSelReg].fFlags   = 0;
                        paSelReg[iSelReg].ValidSel = 0;
                    }

                    /* This might not be 104% correct, but I think it's close
                       enough for all practical purposes...  (REM always loaded
                       LDTR registers.) */
                    pVCpu->cpum.s.Guest.ldtr.fFlags   = CPUMSELREG_FLAGS_VALID;
                    pVCpu->cpum.s.Guest.ldtr.ValidSel = pVCpu->cpum.s.Guest.ldtr.Sel;
                }
                pVCpu->cpum.s.Guest.tr.fFlags     = CPUMSELREG_FLAGS_VALID;
                pVCpu->cpum.s.Guest.tr.ValidSel   = pVCpu->cpum.s.Guest.tr.Sel;
            }
        }

        /* Clear CPUM_CHANGED_HIDDEN_SEL_REGS_INVALID. */
        if (   uVersion >  CPUM_SAVED_STATE_VERSION_VER3_2
            && uVersion <= CPUM_SAVED_STATE_VERSION_MEM)
            for (VMCPUID iCpu = 0; iCpu < pVM->cCpus; iCpu++)
                pVM->aCpus[iCpu].cpum.s.fChanged &= CPUM_CHANGED_HIDDEN_SEL_REGS_INVALID;

        /*
         * A quick sanity check.
         */
        for (VMCPUID iCpu = 0; iCpu < pVM->cCpus; iCpu++)
        {
            PVMCPU pVCpu = &pVM->aCpus[iCpu];
            AssertLogRelReturn(!(pVCpu->cpum.s.Guest.es.fFlags & ~CPUMSELREG_FLAGS_VALID_MASK), VERR_SSM_UNEXPECTED_DATA);
            AssertLogRelReturn(!(pVCpu->cpum.s.Guest.cs.fFlags & ~CPUMSELREG_FLAGS_VALID_MASK), VERR_SSM_UNEXPECTED_DATA);
            AssertLogRelReturn(!(pVCpu->cpum.s.Guest.ss.fFlags & ~CPUMSELREG_FLAGS_VALID_MASK), VERR_SSM_UNEXPECTED_DATA);
            AssertLogRelReturn(!(pVCpu->cpum.s.Guest.ds.fFlags & ~CPUMSELREG_FLAGS_VALID_MASK), VERR_SSM_UNEXPECTED_DATA);
            AssertLogRelReturn(!(pVCpu->cpum.s.Guest.fs.fFlags & ~CPUMSELREG_FLAGS_VALID_MASK), VERR_SSM_UNEXPECTED_DATA);
            AssertLogRelReturn(!(pVCpu->cpum.s.Guest.gs.fFlags & ~CPUMSELREG_FLAGS_VALID_MASK), VERR_SSM_UNEXPECTED_DATA);
        }
    }

    pVM->cpum.s.fPendingRestore = false;

    /*
     * Guest CPUIDs.
     */
    if (uVersion >= CPUM_SAVED_STATE_VERSION_VER3_2)
        return cpumR3LoadCpuId(pVM, pSSM, uVersion);
    return cpumR3LoadCpuIdPre32(pVM, pSSM, uVersion);
}


/**
 * @callback_method_impl{FNSSMINTLOADDONE}
 */
static DECLCALLBACK(int) cpumR3LoadDone(PVM pVM, PSSMHANDLE pSSM)
{
    if (RT_FAILURE(SSMR3HandleGetStatus(pSSM)))
        return VINF_SUCCESS;

    /* just check this since we can. */ /** @todo Add a SSM unit flag for indicating that it's mandatory during a restore.  */
    if (pVM->cpum.s.fPendingRestore)
    {
        LogRel(("CPUM: Missing state!\n"));
        return VERR_INTERNAL_ERROR_2;
    }

    bool const fSupportsLongMode = VMR3IsLongModeAllowed(pVM);
    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
    {
        PVMCPU pVCpu = &pVM->aCpus[idCpu];

        /* Notify PGM of the NXE states in case they've changed. */
        PGMNotifyNxeChanged(pVCpu, RT_BOOL(pVCpu->cpum.s.Guest.msrEFER & MSR_K6_EFER_NXE));

        /* During init. this is done in CPUMR3InitCompleted(). */
        if (fSupportsLongMode)
            pVCpu->cpum.s.fUseFlags |= CPUM_USE_SUPPORTS_LONGMODE;
    }
    return VINF_SUCCESS;
}


/**
 * Checks if the CPUM state restore is still pending.
 *
 * @returns true / false.
 * @param   pVM                 The cross context VM structure.
 */
VMMDECL(bool) CPUMR3IsStateRestorePending(PVM pVM)
{
    return pVM->cpum.s.fPendingRestore;
}


/**
 * Formats the EFLAGS value into mnemonics.
 *
 * @param   pszEFlags   Where to write the mnemonics. (Assumes sufficient buffer space.)
 * @param   efl         The EFLAGS value.
 */
static void cpumR3InfoFormatFlags(char *pszEFlags, uint32_t efl)
{
    /*
     * Format the flags.
     */
    static const struct
    {
        const char *pszSet; const char *pszClear; uint32_t fFlag;
    }   s_aFlags[] =
    {
        { "vip",NULL, X86_EFL_VIP },
        { "vif",NULL, X86_EFL_VIF },
        { "ac", NULL, X86_EFL_AC },
        { "vm", NULL, X86_EFL_VM },
        { "rf", NULL, X86_EFL_RF },
        { "nt", NULL, X86_EFL_NT },
        { "ov", "nv", X86_EFL_OF },
        { "dn", "up", X86_EFL_DF },
        { "ei", "di", X86_EFL_IF },
        { "tf", NULL, X86_EFL_TF },
        { "nt", "pl", X86_EFL_SF },
        { "nz", "zr", X86_EFL_ZF },
        { "ac", "na", X86_EFL_AF },
        { "po", "pe", X86_EFL_PF },
        { "cy", "nc", X86_EFL_CF },
    };
    char *psz = pszEFlags;
    for (unsigned i = 0; i < RT_ELEMENTS(s_aFlags); i++)
    {
        const char *pszAdd = s_aFlags[i].fFlag & efl ? s_aFlags[i].pszSet : s_aFlags[i].pszClear;
        if (pszAdd)
        {
            strcpy(psz, pszAdd);
            psz += strlen(pszAdd);
            *psz++ = ' ';
        }
    }
    psz[-1] = '\0';
}


/**
 * Formats a full register dump.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pCtx        The context to format.
 * @param   pCtxCore    The context core to format.
 * @param   pHlp        Output functions.
 * @param   enmType     The dump type.
 * @param   pszPrefix   Register name prefix.
 */
static void cpumR3InfoOne(PVM pVM, PCPUMCTX pCtx, PCCPUMCTXCORE pCtxCore, PCDBGFINFOHLP pHlp, CPUMDUMPTYPE enmType,
                          const char *pszPrefix)
{
    NOREF(pVM);

    /*
     * Format the EFLAGS.
     */
    uint32_t efl = pCtxCore->eflags.u32;
    char szEFlags[80];
    cpumR3InfoFormatFlags(&szEFlags[0], efl);

    /*
     * Format the registers.
     */
    switch (enmType)
    {
        case CPUMDUMPTYPE_TERSE:
            if (CPUMIsGuestIn64BitCodeEx(pCtx))
                pHlp->pfnPrintf(pHlp,
                    "%srax=%016RX64 %srbx=%016RX64 %srcx=%016RX64 %srdx=%016RX64\n"
                    "%srsi=%016RX64 %srdi=%016RX64 %sr8 =%016RX64 %sr9 =%016RX64\n"
                    "%sr10=%016RX64 %sr11=%016RX64 %sr12=%016RX64 %sr13=%016RX64\n"
                    "%sr14=%016RX64 %sr15=%016RX64\n"
                    "%srip=%016RX64 %srsp=%016RX64 %srbp=%016RX64 %siopl=%d %*s\n"
                    "%scs=%04x %sss=%04x %sds=%04x %ses=%04x %sfs=%04x %sgs=%04x                %seflags=%08x\n",
                    pszPrefix, pCtxCore->rax, pszPrefix, pCtxCore->rbx, pszPrefix, pCtxCore->rcx, pszPrefix, pCtxCore->rdx, pszPrefix, pCtxCore->rsi, pszPrefix, pCtxCore->rdi,
                    pszPrefix, pCtxCore->r8, pszPrefix, pCtxCore->r9, pszPrefix, pCtxCore->r10, pszPrefix, pCtxCore->r11, pszPrefix, pCtxCore->r12, pszPrefix, pCtxCore->r13,
                    pszPrefix, pCtxCore->r14, pszPrefix, pCtxCore->r15,
                    pszPrefix, pCtxCore->rip, pszPrefix, pCtxCore->rsp, pszPrefix, pCtxCore->rbp, pszPrefix, X86_EFL_GET_IOPL(efl), *pszPrefix ? 33 : 31, szEFlags,
                    pszPrefix, pCtxCore->cs.Sel, pszPrefix, pCtxCore->ss.Sel, pszPrefix, pCtxCore->ds.Sel, pszPrefix, pCtxCore->es.Sel,
                    pszPrefix, pCtxCore->fs.Sel, pszPrefix, pCtxCore->gs.Sel, pszPrefix, efl);
            else
                pHlp->pfnPrintf(pHlp,
                    "%seax=%08x %sebx=%08x %secx=%08x %sedx=%08x %sesi=%08x %sedi=%08x\n"
                    "%seip=%08x %sesp=%08x %sebp=%08x %siopl=%d %*s\n"
                    "%scs=%04x %sss=%04x %sds=%04x %ses=%04x %sfs=%04x %sgs=%04x                %seflags=%08x\n",
                    pszPrefix, pCtxCore->eax, pszPrefix, pCtxCore->ebx, pszPrefix, pCtxCore->ecx, pszPrefix, pCtxCore->edx, pszPrefix, pCtxCore->esi, pszPrefix, pCtxCore->edi,
                    pszPrefix, pCtxCore->eip, pszPrefix, pCtxCore->esp, pszPrefix, pCtxCore->ebp, pszPrefix, X86_EFL_GET_IOPL(efl), *pszPrefix ? 33 : 31, szEFlags,
                    pszPrefix, pCtxCore->cs.Sel, pszPrefix, pCtxCore->ss.Sel, pszPrefix, pCtxCore->ds.Sel, pszPrefix, pCtxCore->es.Sel,
                    pszPrefix, pCtxCore->fs.Sel, pszPrefix, pCtxCore->gs.Sel, pszPrefix, efl);
            break;

        case CPUMDUMPTYPE_DEFAULT:
            if (CPUMIsGuestIn64BitCodeEx(pCtx))
                pHlp->pfnPrintf(pHlp,
                    "%srax=%016RX64 %srbx=%016RX64 %srcx=%016RX64 %srdx=%016RX64\n"
                    "%srsi=%016RX64 %srdi=%016RX64 %sr8 =%016RX64 %sr9 =%016RX64\n"
                    "%sr10=%016RX64 %sr11=%016RX64 %sr12=%016RX64 %sr13=%016RX64\n"
                    "%sr14=%016RX64 %sr15=%016RX64\n"
                    "%srip=%016RX64 %srsp=%016RX64 %srbp=%016RX64 %siopl=%d %*s\n"
                    "%scs=%04x %sss=%04x %sds=%04x %ses=%04x %sfs=%04x %sgs=%04x %str=%04x      %seflags=%08x\n"
                    "%scr0=%08RX64 %scr2=%08RX64 %scr3=%08RX64 %scr4=%08RX64 %sgdtr=%016RX64:%04x %sldtr=%04x\n"
                    ,
                    pszPrefix, pCtxCore->rax, pszPrefix, pCtxCore->rbx, pszPrefix, pCtxCore->rcx, pszPrefix, pCtxCore->rdx, pszPrefix, pCtxCore->rsi, pszPrefix, pCtxCore->rdi,
                    pszPrefix, pCtxCore->r8, pszPrefix, pCtxCore->r9, pszPrefix, pCtxCore->r10, pszPrefix, pCtxCore->r11, pszPrefix, pCtxCore->r12, pszPrefix, pCtxCore->r13,
                    pszPrefix, pCtxCore->r14, pszPrefix, pCtxCore->r15,
                    pszPrefix, pCtxCore->rip, pszPrefix, pCtxCore->rsp, pszPrefix, pCtxCore->rbp, pszPrefix, X86_EFL_GET_IOPL(efl), *pszPrefix ? 33 : 31, szEFlags,
                    pszPrefix, pCtxCore->cs.Sel, pszPrefix, pCtxCore->ss.Sel, pszPrefix, pCtxCore->ds.Sel, pszPrefix, pCtxCore->es.Sel,
                    pszPrefix, pCtxCore->fs.Sel, pszPrefix, pCtxCore->gs.Sel, pszPrefix, pCtx->tr.Sel, pszPrefix, efl,
                    pszPrefix, pCtx->cr0, pszPrefix, pCtx->cr2, pszPrefix, pCtx->cr3, pszPrefix, pCtx->cr4,
                    pszPrefix, pCtx->gdtr.pGdt, pCtx->gdtr.cbGdt, pszPrefix, pCtx->ldtr.Sel);
            else
                pHlp->pfnPrintf(pHlp,
                    "%seax=%08x %sebx=%08x %secx=%08x %sedx=%08x %sesi=%08x %sedi=%08x\n"
                    "%seip=%08x %sesp=%08x %sebp=%08x %siopl=%d %*s\n"
                    "%scs=%04x %sss=%04x %sds=%04x %ses=%04x %sfs=%04x %sgs=%04x %str=%04x      %seflags=%08x\n"
                    "%scr0=%08RX64 %scr2=%08RX64 %scr3=%08RX64 %scr4=%08RX64 %sgdtr=%08RX64:%04x %sldtr=%04x\n"
                    ,
                    pszPrefix, pCtxCore->eax, pszPrefix, pCtxCore->ebx, pszPrefix, pCtxCore->ecx, pszPrefix, pCtxCore->edx, pszPrefix, pCtxCore->esi, pszPrefix, pCtxCore->edi,
                    pszPrefix, pCtxCore->eip, pszPrefix, pCtxCore->esp, pszPrefix, pCtxCore->ebp, pszPrefix, X86_EFL_GET_IOPL(efl), *pszPrefix ? 33 : 31, szEFlags,
                    pszPrefix, pCtxCore->cs.Sel, pszPrefix, pCtxCore->ss.Sel, pszPrefix, pCtxCore->ds.Sel, pszPrefix, pCtxCore->es.Sel,
                    pszPrefix, pCtxCore->fs.Sel, pszPrefix, pCtxCore->gs.Sel, pszPrefix, pCtx->tr.Sel, pszPrefix, efl,
                    pszPrefix, pCtx->cr0, pszPrefix, pCtx->cr2, pszPrefix, pCtx->cr3, pszPrefix, pCtx->cr4,
                    pszPrefix, pCtx->gdtr.pGdt, pCtx->gdtr.cbGdt, pszPrefix, pCtx->ldtr.Sel);
            break;

        case CPUMDUMPTYPE_VERBOSE:
            if (CPUMIsGuestIn64BitCodeEx(pCtx))
                pHlp->pfnPrintf(pHlp,
                    "%srax=%016RX64 %srbx=%016RX64 %srcx=%016RX64 %srdx=%016RX64\n"
                    "%srsi=%016RX64 %srdi=%016RX64 %sr8 =%016RX64 %sr9 =%016RX64\n"
                    "%sr10=%016RX64 %sr11=%016RX64 %sr12=%016RX64 %sr13=%016RX64\n"
                    "%sr14=%016RX64 %sr15=%016RX64\n"
                    "%srip=%016RX64 %srsp=%016RX64 %srbp=%016RX64 %siopl=%d %*s\n"
                    "%scs={%04x base=%016RX64 limit=%08x flags=%08x}\n"
                    "%sds={%04x base=%016RX64 limit=%08x flags=%08x}\n"
                    "%ses={%04x base=%016RX64 limit=%08x flags=%08x}\n"
                    "%sfs={%04x base=%016RX64 limit=%08x flags=%08x}\n"
                    "%sgs={%04x base=%016RX64 limit=%08x flags=%08x}\n"
                    "%sss={%04x base=%016RX64 limit=%08x flags=%08x}\n"
                    "%scr0=%016RX64 %scr2=%016RX64 %scr3=%016RX64 %scr4=%016RX64\n"
                    "%sdr0=%016RX64 %sdr1=%016RX64 %sdr2=%016RX64 %sdr3=%016RX64\n"
                    "%sdr4=%016RX64 %sdr5=%016RX64 %sdr6=%016RX64 %sdr7=%016RX64\n"
                    "%sgdtr=%016RX64:%04x  %sidtr=%016RX64:%04x  %seflags=%08x\n"
                    "%sldtr={%04x base=%08RX64 limit=%08x flags=%08x}\n"
                    "%str  ={%04x base=%08RX64 limit=%08x flags=%08x}\n"
                    "%sSysEnter={cs=%04llx eip=%016RX64 esp=%016RX64}\n"
                    ,
                    pszPrefix, pCtxCore->rax, pszPrefix, pCtxCore->rbx, pszPrefix, pCtxCore->rcx, pszPrefix, pCtxCore->rdx, pszPrefix, pCtxCore->rsi, pszPrefix, pCtxCore->rdi,
                    pszPrefix, pCtxCore->r8, pszPrefix, pCtxCore->r9, pszPrefix, pCtxCore->r10, pszPrefix, pCtxCore->r11, pszPrefix, pCtxCore->r12, pszPrefix, pCtxCore->r13,
                    pszPrefix, pCtxCore->r14, pszPrefix, pCtxCore->r15,
                    pszPrefix, pCtxCore->rip, pszPrefix, pCtxCore->rsp, pszPrefix, pCtxCore->rbp, pszPrefix, X86_EFL_GET_IOPL(efl), *pszPrefix ? 33 : 31, szEFlags,
                    pszPrefix, pCtxCore->cs.Sel, pCtx->cs.u64Base, pCtx->cs.u32Limit, pCtx->cs.Attr.u,
                    pszPrefix, pCtxCore->ds.Sel, pCtx->ds.u64Base, pCtx->ds.u32Limit, pCtx->ds.Attr.u,
                    pszPrefix, pCtxCore->es.Sel, pCtx->es.u64Base, pCtx->es.u32Limit, pCtx->es.Attr.u,
                    pszPrefix, pCtxCore->fs.Sel, pCtx->fs.u64Base, pCtx->fs.u32Limit, pCtx->fs.Attr.u,
                    pszPrefix, pCtxCore->gs.Sel, pCtx->gs.u64Base, pCtx->gs.u32Limit, pCtx->gs.Attr.u,
                    pszPrefix, pCtxCore->ss.Sel, pCtx->ss.u64Base, pCtx->ss.u32Limit, pCtx->ss.Attr.u,
                    pszPrefix, pCtx->cr0,  pszPrefix, pCtx->cr2, pszPrefix, pCtx->cr3,  pszPrefix, pCtx->cr4,
                    pszPrefix, pCtx->dr[0],  pszPrefix, pCtx->dr[1], pszPrefix, pCtx->dr[2],  pszPrefix, pCtx->dr[3],
                    pszPrefix, pCtx->dr[4],  pszPrefix, pCtx->dr[5], pszPrefix, pCtx->dr[6],  pszPrefix, pCtx->dr[7],
                    pszPrefix, pCtx->gdtr.pGdt, pCtx->gdtr.cbGdt, pszPrefix, pCtx->idtr.pIdt, pCtx->idtr.cbIdt, pszPrefix, efl,
                    pszPrefix, pCtx->ldtr.Sel, pCtx->ldtr.u64Base, pCtx->ldtr.u32Limit, pCtx->ldtr.Attr.u,
                    pszPrefix, pCtx->tr.Sel, pCtx->tr.u64Base, pCtx->tr.u32Limit, pCtx->tr.Attr.u,
                    pszPrefix, pCtx->SysEnter.cs, pCtx->SysEnter.eip, pCtx->SysEnter.esp);
            else
                pHlp->pfnPrintf(pHlp,
                    "%seax=%08x %sebx=%08x %secx=%08x %sedx=%08x %sesi=%08x %sedi=%08x\n"
                    "%seip=%08x %sesp=%08x %sebp=%08x %siopl=%d %*s\n"
                    "%scs={%04x base=%016RX64 limit=%08x flags=%08x} %sdr0=%08RX64 %sdr1=%08RX64\n"
                    "%sds={%04x base=%016RX64 limit=%08x flags=%08x} %sdr2=%08RX64 %sdr3=%08RX64\n"
                    "%ses={%04x base=%016RX64 limit=%08x flags=%08x} %sdr4=%08RX64 %sdr5=%08RX64\n"
                    "%sfs={%04x base=%016RX64 limit=%08x flags=%08x} %sdr6=%08RX64 %sdr7=%08RX64\n"
                    "%sgs={%04x base=%016RX64 limit=%08x flags=%08x} %scr0=%08RX64 %scr2=%08RX64\n"
                    "%sss={%04x base=%016RX64 limit=%08x flags=%08x} %scr3=%08RX64 %scr4=%08RX64\n"
                    "%sgdtr=%016RX64:%04x  %sidtr=%016RX64:%04x  %seflags=%08x\n"
                    "%sldtr={%04x base=%08RX64 limit=%08x flags=%08x}\n"
                    "%str  ={%04x base=%08RX64 limit=%08x flags=%08x}\n"
                    "%sSysEnter={cs=%04llx eip=%08llx esp=%08llx}\n"
                    ,
                    pszPrefix, pCtxCore->eax, pszPrefix, pCtxCore->ebx, pszPrefix, pCtxCore->ecx, pszPrefix, pCtxCore->edx, pszPrefix, pCtxCore->esi, pszPrefix, pCtxCore->edi,
                    pszPrefix, pCtxCore->eip, pszPrefix, pCtxCore->esp, pszPrefix, pCtxCore->ebp, pszPrefix, X86_EFL_GET_IOPL(efl), *pszPrefix ? 33 : 31, szEFlags,
                    pszPrefix, pCtxCore->cs.Sel, pCtx->cs.u64Base, pCtx->cs.u32Limit, pCtx->cs.Attr.u, pszPrefix, pCtx->dr[0],  pszPrefix, pCtx->dr[1],
                    pszPrefix, pCtxCore->ds.Sel, pCtx->ds.u64Base, pCtx->ds.u32Limit, pCtx->ds.Attr.u, pszPrefix, pCtx->dr[2],  pszPrefix, pCtx->dr[3],
                    pszPrefix, pCtxCore->es.Sel, pCtx->es.u64Base, pCtx->es.u32Limit, pCtx->es.Attr.u, pszPrefix, pCtx->dr[4],  pszPrefix, pCtx->dr[5],
                    pszPrefix, pCtxCore->fs.Sel, pCtx->fs.u64Base, pCtx->fs.u32Limit, pCtx->fs.Attr.u, pszPrefix, pCtx->dr[6],  pszPrefix, pCtx->dr[7],
                    pszPrefix, pCtxCore->gs.Sel, pCtx->gs.u64Base, pCtx->gs.u32Limit, pCtx->gs.Attr.u, pszPrefix, pCtx->cr0,  pszPrefix, pCtx->cr2,
                    pszPrefix, pCtxCore->ss.Sel, pCtx->ss.u64Base, pCtx->ss.u32Limit, pCtx->ss.Attr.u, pszPrefix, pCtx->cr3,  pszPrefix, pCtx->cr4,
                    pszPrefix, pCtx->gdtr.pGdt, pCtx->gdtr.cbGdt, pszPrefix, pCtx->idtr.pIdt, pCtx->idtr.cbIdt, pszPrefix, efl,
                    pszPrefix, pCtx->ldtr.Sel, pCtx->ldtr.u64Base, pCtx->ldtr.u32Limit, pCtx->ldtr.Attr.u,
                    pszPrefix, pCtx->tr.Sel, pCtx->tr.u64Base, pCtx->tr.u32Limit, pCtx->tr.Attr.u,
                    pszPrefix, pCtx->SysEnter.cs, pCtx->SysEnter.eip, pCtx->SysEnter.esp);

            pHlp->pfnPrintf(pHlp, "%sxcr=%016RX64 %sxcr1=%016RX64 %sxss=%016RX64 (fXStateMask=%016RX64)\n",
                            pszPrefix, pCtx->aXcr[0], pszPrefix, pCtx->aXcr[1],
                            pszPrefix, UINT64_C(0) /** @todo XSS */, pCtx->fXStateMask);
            if (pCtx->CTX_SUFF(pXState))
            {
                PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
                pHlp->pfnPrintf(pHlp,
                    "%sFCW=%04x %sFSW=%04x %sFTW=%04x %sFOP=%04x %sMXCSR=%08x %sMXCSR_MASK=%08x\n"
                    "%sFPUIP=%08x %sCS=%04x %sRsrvd1=%04x  %sFPUDP=%08x %sDS=%04x %sRsvrd2=%04x\n"
                    ,
                    pszPrefix, pFpuCtx->FCW,   pszPrefix, pFpuCtx->FSW, pszPrefix, pFpuCtx->FTW, pszPrefix, pFpuCtx->FOP,
                    pszPrefix, pFpuCtx->MXCSR, pszPrefix, pFpuCtx->MXCSR_MASK,
                    pszPrefix, pFpuCtx->FPUIP, pszPrefix, pFpuCtx->CS,  pszPrefix, pFpuCtx->Rsrvd1,
                    pszPrefix, pFpuCtx->FPUDP, pszPrefix, pFpuCtx->DS,  pszPrefix, pFpuCtx->Rsrvd2
                    );
                /*
                 * The FSAVE style memory image contains ST(0)-ST(7) at increasing addresses,
                 * not (FP)R0-7 as Intel SDM suggests.
                 */
                unsigned iShift = (pFpuCtx->FSW >> 11) & 7;
                for (unsigned iST = 0; iST < RT_ELEMENTS(pFpuCtx->aRegs); iST++)
                {
                    unsigned iFPR        = (iST + iShift) % RT_ELEMENTS(pFpuCtx->aRegs);
                    unsigned uTag        = (pFpuCtx->FTW >> (2 * iFPR)) & 3;
                    char     chSign      = pFpuCtx->aRegs[iST].au16[4] & 0x8000 ? '-' : '+';
                    unsigned iInteger    = (unsigned)(pFpuCtx->aRegs[iST].au64[0] >> 63);
                    uint64_t u64Fraction = pFpuCtx->aRegs[iST].au64[0] & UINT64_C(0x7fffffffffffffff);
                    int      iExponent   = pFpuCtx->aRegs[iST].au16[4] & 0x7fff;
                    iExponent -= 16383; /* subtract bias */
                    /** @todo This isn't entirenly correct and needs more work! */
                    pHlp->pfnPrintf(pHlp,
                                    "%sST(%u)=%sFPR%u={%04RX16'%08RX32'%08RX32} t%d %c%u.%022llu * 2 ^ %d (*)",
                                    pszPrefix, iST, pszPrefix, iFPR,
                                    pFpuCtx->aRegs[iST].au16[4], pFpuCtx->aRegs[iST].au32[1], pFpuCtx->aRegs[iST].au32[0],
                                    uTag, chSign, iInteger, u64Fraction, iExponent);
                    if (pFpuCtx->aRegs[iST].au16[5] || pFpuCtx->aRegs[iST].au16[6] || pFpuCtx->aRegs[iST].au16[7])
                        pHlp->pfnPrintf(pHlp, " res={%04RX16,%04RX16,%04RX16}\n",
                                        pFpuCtx->aRegs[iST].au16[5], pFpuCtx->aRegs[iST].au16[6], pFpuCtx->aRegs[iST].au16[7]);
                    else
                        pHlp->pfnPrintf(pHlp, "\n");
                }

                /* XMM/YMM/ZMM registers. */
                if (pCtx->fXStateMask & XSAVE_C_YMM)
                {
                    PCX86XSAVEYMMHI pYmmHiCtx = CPUMCTX_XSAVE_C_PTR(pCtx, XSAVE_C_YMM_BIT, PCX86XSAVEYMMHI);
                    if (!(pCtx->fXStateMask & XSAVE_C_ZMM_HI256))
                        for (unsigned i = 0; i < RT_ELEMENTS(pFpuCtx->aXMM); i++)
                            pHlp->pfnPrintf(pHlp, "%sYMM%u%s=%08RX32'%08RX32'%08RX32'%08RX32'%08RX32'%08RX32'%08RX32'%08RX32\n",
                                            pszPrefix, i, i < 10 ? " " : "",
                                            pYmmHiCtx->aYmmHi[i].au32[3],
                                            pYmmHiCtx->aYmmHi[i].au32[2],
                                            pYmmHiCtx->aYmmHi[i].au32[1],
                                            pYmmHiCtx->aYmmHi[i].au32[0],
                                            pFpuCtx->aXMM[i].au32[3],
                                            pFpuCtx->aXMM[i].au32[2],
                                            pFpuCtx->aXMM[i].au32[1],
                                            pFpuCtx->aXMM[i].au32[0]);
                    else
                    {
                        PCX86XSAVEZMMHI256 pZmmHi256 = CPUMCTX_XSAVE_C_PTR(pCtx, XSAVE_C_ZMM_HI256_BIT, PCX86XSAVEZMMHI256);
                        for (unsigned i = 0; i < RT_ELEMENTS(pFpuCtx->aXMM); i++)
                            pHlp->pfnPrintf(pHlp,
                                            "%sZMM%u%s=%08RX32'%08RX32'%08RX32'%08RX32'%08RX32'%08RX32'%08RX32'%08RX32''%08RX32'%08RX32'%08RX32'%08RX32'%08RX32'%08RX32'%08RX32'%08RX32\n",
                                            pszPrefix, i, i < 10 ? " " : "",
                                            pZmmHi256->aHi256Regs[i].au32[7],
                                            pZmmHi256->aHi256Regs[i].au32[6],
                                            pZmmHi256->aHi256Regs[i].au32[5],
                                            pZmmHi256->aHi256Regs[i].au32[4],
                                            pZmmHi256->aHi256Regs[i].au32[3],
                                            pZmmHi256->aHi256Regs[i].au32[2],
                                            pZmmHi256->aHi256Regs[i].au32[1],
                                            pZmmHi256->aHi256Regs[i].au32[0],
                                            pYmmHiCtx->aYmmHi[i].au32[3],
                                            pYmmHiCtx->aYmmHi[i].au32[2],
                                            pYmmHiCtx->aYmmHi[i].au32[1],
                                            pYmmHiCtx->aYmmHi[i].au32[0],
                                            pFpuCtx->aXMM[i].au32[3],
                                            pFpuCtx->aXMM[i].au32[2],
                                            pFpuCtx->aXMM[i].au32[1],
                                            pFpuCtx->aXMM[i].au32[0]);

                        PCX86XSAVEZMM16HI pZmm16Hi = CPUMCTX_XSAVE_C_PTR(pCtx, XSAVE_C_ZMM_16HI_BIT, PCX86XSAVEZMM16HI);
                        for (unsigned i = 0; i < RT_ELEMENTS(pZmm16Hi->aRegs); i++)
                            pHlp->pfnPrintf(pHlp,
                                            "%sZMM%u=%08RX32'%08RX32'%08RX32'%08RX32'%08RX32'%08RX32'%08RX32'%08RX32''%08RX32'%08RX32'%08RX32'%08RX32'%08RX32'%08RX32'%08RX32'%08RX32\n",
                                            pszPrefix, i + 16,
                                            pZmm16Hi->aRegs[i].au32[15],
                                            pZmm16Hi->aRegs[i].au32[14],
                                            pZmm16Hi->aRegs[i].au32[13],
                                            pZmm16Hi->aRegs[i].au32[12],
                                            pZmm16Hi->aRegs[i].au32[11],
                                            pZmm16Hi->aRegs[i].au32[10],
                                            pZmm16Hi->aRegs[i].au32[9],
                                            pZmm16Hi->aRegs[i].au32[8],
                                            pZmm16Hi->aRegs[i].au32[7],
                                            pZmm16Hi->aRegs[i].au32[6],
                                            pZmm16Hi->aRegs[i].au32[5],
                                            pZmm16Hi->aRegs[i].au32[4],
                                            pZmm16Hi->aRegs[i].au32[3],
                                            pZmm16Hi->aRegs[i].au32[2],
                                            pZmm16Hi->aRegs[i].au32[1],
                                            pZmm16Hi->aRegs[i].au32[0]);
                    }
                }
                else
                    for (unsigned i = 0; i < RT_ELEMENTS(pFpuCtx->aXMM); i++)
                        pHlp->pfnPrintf(pHlp,
                                        i & 1
                                        ? "%sXMM%u%s=%08RX32'%08RX32'%08RX32'%08RX32\n"
                                        : "%sXMM%u%s=%08RX32'%08RX32'%08RX32'%08RX32  ",
                                        pszPrefix, i, i < 10 ? " " : "",
                                        pFpuCtx->aXMM[i].au32[3],
                                        pFpuCtx->aXMM[i].au32[2],
                                        pFpuCtx->aXMM[i].au32[1],
                                        pFpuCtx->aXMM[i].au32[0]);

                if (pCtx->fXStateMask & XSAVE_C_OPMASK)
                {
                    PCX86XSAVEOPMASK pOpMask = CPUMCTX_XSAVE_C_PTR(pCtx, XSAVE_C_OPMASK_BIT, PCX86XSAVEOPMASK);
                    for (unsigned i = 0; i < RT_ELEMENTS(pOpMask->aKRegs); i += 4)
                        pHlp->pfnPrintf(pHlp, "%sK%u=%016RX64  %sK%u=%016RX64  %sK%u=%016RX64  %sK%u=%016RX64\n",
                                        pszPrefix, i + 0, pOpMask->aKRegs[i + 0],
                                        pszPrefix, i + 1, pOpMask->aKRegs[i + 1],
                                        pszPrefix, i + 2, pOpMask->aKRegs[i + 2],
                                        pszPrefix, i + 3, pOpMask->aKRegs[i + 3]);
                }

                if (pCtx->fXStateMask & XSAVE_C_BNDREGS)
                {
                    PCX86XSAVEBNDREGS pBndRegs = CPUMCTX_XSAVE_C_PTR(pCtx, XSAVE_C_BNDREGS_BIT, PCX86XSAVEBNDREGS);
                    for (unsigned i = 0; i < RT_ELEMENTS(pBndRegs->aRegs); i += 2)
                        pHlp->pfnPrintf(pHlp, "%sBNDREG%u=%016RX64/%016RX64  %sBNDREG%u=%016RX64/%016RX64\n",
                                        pszPrefix, i, pBndRegs->aRegs[i].uLowerBound, pBndRegs->aRegs[i].uUpperBound,
                                        pszPrefix, i + 1, pBndRegs->aRegs[i + 1].uLowerBound, pBndRegs->aRegs[i + 1].uUpperBound);
                }

                if (pCtx->fXStateMask & XSAVE_C_BNDCSR)
                {
                    PCX86XSAVEBNDCFG pBndCfg = CPUMCTX_XSAVE_C_PTR(pCtx, XSAVE_C_BNDCSR_BIT, PCX86XSAVEBNDCFG);
                    pHlp->pfnPrintf(pHlp, "%sBNDCFG.CONFIG=%016RX64 %sBNDCFG.STATUS=%016RX64\n",
                                    pszPrefix, pBndCfg->fConfig, pszPrefix, pBndCfg->fStatus);
                }

                for (unsigned i = 0; i < RT_ELEMENTS(pFpuCtx->au32RsrvdRest); i++)
                    if (pFpuCtx->au32RsrvdRest[i])
                        pHlp->pfnPrintf(pHlp, "%sRsrvdRest[%u]=%RX32 (offset=%#x)\n",
                                        pszPrefix, i, pFpuCtx->au32RsrvdRest[i], RT_OFFSETOF(X86FXSTATE, au32RsrvdRest[i]) );
            }

            pHlp->pfnPrintf(pHlp,
                "%sEFER         =%016RX64\n"
                "%sPAT          =%016RX64\n"
                "%sSTAR         =%016RX64\n"
                "%sCSTAR        =%016RX64\n"
                "%sLSTAR        =%016RX64\n"
                "%sSFMASK       =%016RX64\n"
                "%sKERNELGSBASE =%016RX64\n",
                pszPrefix, pCtx->msrEFER,
                pszPrefix, pCtx->msrPAT,
                pszPrefix, pCtx->msrSTAR,
                pszPrefix, pCtx->msrCSTAR,
                pszPrefix, pCtx->msrLSTAR,
                pszPrefix, pCtx->msrSFMASK,
                pszPrefix, pCtx->msrKERNELGSBASE);
            break;
    }
}


/**
 * Display all cpu states and any other cpum info.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pHlp        The info helper functions.
 * @param   pszArgs     Arguments, ignored.
 */
static DECLCALLBACK(void) cpumR3InfoAll(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    cpumR3InfoGuest(pVM, pHlp, pszArgs);
    cpumR3InfoGuestInstr(pVM, pHlp, pszArgs);
    cpumR3InfoGuestHwvirt(pVM, pHlp, pszArgs);
    cpumR3InfoHyper(pVM, pHlp, pszArgs);
    cpumR3InfoHost(pVM, pHlp, pszArgs);
}


/**
 * Parses the info argument.
 *
 * The argument starts with 'verbose', 'terse' or 'default' and then
 * continues with the comment string.
 *
 * @param   pszArgs         The pointer to the argument string.
 * @param   penmType        Where to store the dump type request.
 * @param   ppszComment     Where to store the pointer to the comment string.
 */
static void cpumR3InfoParseArg(const char *pszArgs, CPUMDUMPTYPE *penmType, const char **ppszComment)
{
    if (!pszArgs)
    {
        *penmType = CPUMDUMPTYPE_DEFAULT;
        *ppszComment = "";
    }
    else
    {
        if (!strncmp(pszArgs, RT_STR_TUPLE("verbose")))
        {
            pszArgs += 7;
            *penmType = CPUMDUMPTYPE_VERBOSE;
        }
        else if (!strncmp(pszArgs, RT_STR_TUPLE("terse")))
        {
            pszArgs += 5;
            *penmType = CPUMDUMPTYPE_TERSE;
        }
        else if (!strncmp(pszArgs, RT_STR_TUPLE("default")))
        {
            pszArgs += 7;
            *penmType = CPUMDUMPTYPE_DEFAULT;
        }
        else
            *penmType = CPUMDUMPTYPE_DEFAULT;
        *ppszComment = RTStrStripL(pszArgs);
    }
}


/**
 * Display the guest cpu state.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pHlp        The info helper functions.
 * @param   pszArgs     Arguments.
 */
static DECLCALLBACK(void) cpumR3InfoGuest(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    CPUMDUMPTYPE enmType;
    const char *pszComment;
    cpumR3InfoParseArg(pszArgs, &enmType, &pszComment);

    PVMCPU pVCpu = VMMGetCpu(pVM);
    if (!pVCpu)
        pVCpu = &pVM->aCpus[0];

    pHlp->pfnPrintf(pHlp, "Guest CPUM (VCPU %d) state: %s\n", pVCpu->idCpu, pszComment);

    PCPUMCTX pCtx = &pVCpu->cpum.s.Guest;
    cpumR3InfoOne(pVM, pCtx, CPUMCTX2CORE(pCtx), pHlp, enmType, "");
}


/**
 * Display the guest's hardware-virtualization cpu state.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pHlp        The info helper functions.
 * @param   pszArgs     Arguments, ignored.
 */
static DECLCALLBACK(void) cpumR3InfoGuestHwvirt(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    RT_NOREF(pszArgs);

    PVMCPU pVCpu = VMMGetCpu(pVM);
    if (!pVCpu)
        pVCpu = &pVM->aCpus[0];

    /*
     * Figure out what to dump.
     *
     * In the future we may need to dump everything whether or not we're actively in nested-guest mode
     * or not, hence the reason why we use a mask to determine what needs dumping. Currently, we only
     * dump hwvirt. state when the guest CPU is executing a nested-guest.
     */
    /** @todo perhaps make this configurable through pszArgs, depending on how much
     *        noise we wish to accept when nested hwvirt. isn't used. */
#define CPUMHWVIRTDUMP_NONE     (0)
#define CPUMHWVIRTDUMP_SVM      RT_BIT(0)
#define CPUMHWVIRTDUMP_VMX      RT_BIT(1)
#define CPUMHWVIRTDUMP_COMMON   RT_BIT(2)
#define CPUMHWVIRTDUMP_LAST     CPUMHWVIRTDUMP_VMX
#define CPUMHWVIRTDUMP_ALL      (CPUMHWVIRTDUMP_COMMON | CPUMHWVIRTDUMP_VMX | CPUMHWVIRTDUMP_SVM)

    PCPUMCTX pCtx = &pVCpu->cpum.s.Guest;
    static const char *const s_aHwvirtModes[] = { "No/inactive", "SVM", "VMX", "Common" };
    uint8_t const idxHwvirtState = CPUMIsGuestInSvmNestedHwVirtMode(pCtx) ? CPUMHWVIRTDUMP_SVM
                                 : CPUMIsGuestInVmxNestedHwVirtMode(pCtx) ? CPUMHWVIRTDUMP_VMX : CPUMHWVIRTDUMP_NONE;
    AssertCompile(CPUMHWVIRTDUMP_LAST <= RT_ELEMENTS(s_aHwvirtModes));
    Assert(idxHwvirtState < RT_ELEMENTS(s_aHwvirtModes));
    const char *pcszHwvirtMode   = s_aHwvirtModes[idxHwvirtState];
    uint32_t const fDumpState    = idxHwvirtState; /* | CPUMHWVIRTDUMP_ALL */

    /*
     * Dump it.
     */
    pHlp->pfnPrintf(pHlp, "VCPU[%u] hardware virtualization state:\n", pVCpu->idCpu);

    if (fDumpState & CPUMHWVIRTDUMP_COMMON)
        pHlp->pfnPrintf(pHlp, "fLocalForcedActions            = %#RX32\n",  pCtx->hwvirt.fLocalForcedActions);
    pHlp->pfnPrintf(pHlp, "%s hwvirt state%s\n", pcszHwvirtMode, fDumpState ? ":" : "");
    if (fDumpState & CPUMHWVIRTDUMP_SVM)
    {
        pHlp->pfnPrintf(pHlp, "  uMsrHSavePa                = %#RX64\n",    pCtx->hwvirt.svm.uMsrHSavePa);
        pHlp->pfnPrintf(pHlp, "  GCPhysVmcb                 = %#RGp\n",     pCtx->hwvirt.svm.GCPhysVmcb);
        pHlp->pfnPrintf(pHlp, "  VmcbCtrl:\n");
        HMR3InfoSvmVmcbCtrl(pHlp, &pCtx->hwvirt.svm.pVmcbR3->ctrl, "    " /* pszPrefix */);
        /** @todo HMR3InfoSvmVmcbStateSave. */
        pHlp->pfnPrintf(pHlp, "  HostState:\n");
        pHlp->pfnPrintf(pHlp, "    uEferMsr                   = %#RX64\n",  pCtx->hwvirt.svm.HostState.uEferMsr);
        pHlp->pfnPrintf(pHlp, "    uCr0                       = %#RX64\n",  pCtx->hwvirt.svm.HostState.uCr0);
        pHlp->pfnPrintf(pHlp, "    uCr4                       = %#RX64\n",  pCtx->hwvirt.svm.HostState.uCr4);
        pHlp->pfnPrintf(pHlp, "    uCr3                       = %#RX64\n",  pCtx->hwvirt.svm.HostState.uCr3);
        pHlp->pfnPrintf(pHlp, "    uRip                       = %#RX64\n",  pCtx->hwvirt.svm.HostState.uRip);
        pHlp->pfnPrintf(pHlp, "    uRsp                       = %#RX64\n",  pCtx->hwvirt.svm.HostState.uRsp);
        pHlp->pfnPrintf(pHlp, "    uRax                       = %#RX64\n",  pCtx->hwvirt.svm.HostState.uRax);
        pHlp->pfnPrintf(pHlp, "    rflags                     = %#RX64\n",  pCtx->hwvirt.svm.HostState.rflags.u64);
        PCPUMSELREG pSel = &pCtx->hwvirt.svm.HostState.es;
        pHlp->pfnPrintf(pHlp, "    es                         = {%04x base=%016RX64 limit=%08x flags=%08x}\n",
                        pSel->Sel, pSel->u64Base, pSel->u32Limit, pSel->fFlags);
        pSel = &pCtx->hwvirt.svm.HostState.cs;
        pHlp->pfnPrintf(pHlp, "    cs                         = {%04x base=%016RX64 limit=%08x flags=%08x}\n",
                        pSel->Sel, pSel->u64Base, pSel->u32Limit, pSel->fFlags);
        pSel = &pCtx->hwvirt.svm.HostState.ss;
        pHlp->pfnPrintf(pHlp, "    ss                         = {%04x base=%016RX64 limit=%08x flags=%08x}\n",
                        pSel->Sel, pSel->u64Base, pSel->u32Limit, pSel->fFlags);
        pSel = &pCtx->hwvirt.svm.HostState.ds;
        pHlp->pfnPrintf(pHlp, "    ds                         = {%04x base=%016RX64 limit=%08x flags=%08x}\n",
                        pSel->Sel, pSel->u64Base, pSel->u32Limit, pSel->fFlags);
        pHlp->pfnPrintf(pHlp, "    gdtr                       = %016RX64:%04x\n", pCtx->hwvirt.svm.HostState.gdtr.pGdt,
                        pCtx->hwvirt.svm.HostState.gdtr.cbGdt);
        pHlp->pfnPrintf(pHlp, "    idtr                       = %016RX64:%04x\n", pCtx->hwvirt.svm.HostState.idtr.pIdt,
                        pCtx->hwvirt.svm.HostState.idtr.cbIdt);
        pHlp->pfnPrintf(pHlp, "  fGif                       = %u\n",        pCtx->hwvirt.svm.fGif);
        pHlp->pfnPrintf(pHlp, "  cPauseFilter               = %RU16\n",     pCtx->hwvirt.svm.cPauseFilter);
        pHlp->pfnPrintf(pHlp, "  cPauseFilterThreshold      = %RU32\n",     pCtx->hwvirt.svm.cPauseFilterThreshold);
        pHlp->pfnPrintf(pHlp, "  fInterceptEvents           = %u\n",        pCtx->hwvirt.svm.fInterceptEvents);
        pHlp->pfnPrintf(pHlp, "  pvMsrBitmapR3              = %p\n",        pCtx->hwvirt.svm.pvMsrBitmapR3);
        pHlp->pfnPrintf(pHlp, "  pvMsrBitmapR0              = %RKv\n",      pCtx->hwvirt.svm.pvMsrBitmapR0);
        pHlp->pfnPrintf(pHlp, "  pvIoBitmapR3               = %p\n",        pCtx->hwvirt.svm.pvIoBitmapR3);
        pHlp->pfnPrintf(pHlp, "  pvIoBitmapR0               = %RKv\n",      pCtx->hwvirt.svm.pvIoBitmapR0);
    }

    /** @todo Intel.  */
#if 0
    if (fDumpState & CPUMHWVIRTDUMP_VMX)
    {
    }
#endif

#undef CPUMHWVIRTDUMP_NONE
#undef CPUMHWVIRTDUMP_COMMON
#undef CPUMHWVIRTDUMP_SVM
#undef CPUMHWVIRTDUMP_VMX
#undef CPUMHWVIRTDUMP_LAST
#undef CPUMHWVIRTDUMP_ALL
}

/**
 * Display the current guest instruction
 *
 * @param   pVM         The cross context VM structure.
 * @param   pHlp        The info helper functions.
 * @param   pszArgs     Arguments, ignored.
 */
static DECLCALLBACK(void) cpumR3InfoGuestInstr(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    NOREF(pszArgs);

    PVMCPU pVCpu = VMMGetCpu(pVM);
    if (!pVCpu)
        pVCpu = &pVM->aCpus[0];

    char szInstruction[256];
    szInstruction[0] = '\0';
    DBGFR3DisasInstrCurrent(pVCpu, szInstruction, sizeof(szInstruction));
    pHlp->pfnPrintf(pHlp, "\nCPUM%u: %s\n\n", pVCpu->idCpu, szInstruction);
}


/**
 * Display the hypervisor cpu state.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pHlp        The info helper functions.
 * @param   pszArgs     Arguments, ignored.
 */
static DECLCALLBACK(void) cpumR3InfoHyper(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    PVMCPU pVCpu = VMMGetCpu(pVM);
    if (!pVCpu)
        pVCpu = &pVM->aCpus[0];

    CPUMDUMPTYPE enmType;
    const char *pszComment;
    cpumR3InfoParseArg(pszArgs, &enmType, &pszComment);
    pHlp->pfnPrintf(pHlp, "Hypervisor CPUM state: %s\n", pszComment);
    cpumR3InfoOne(pVM, &pVCpu->cpum.s.Hyper, CPUMCTX2CORE(&pVCpu->cpum.s.Hyper), pHlp, enmType, ".");
    pHlp->pfnPrintf(pHlp, "CR4OrMask=%#x CR4AndMask=%#x\n", pVM->cpum.s.CR4.OrMask, pVM->cpum.s.CR4.AndMask);
}


/**
 * Display the host cpu state.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pHlp        The info helper functions.
 * @param   pszArgs     Arguments, ignored.
 */
static DECLCALLBACK(void) cpumR3InfoHost(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    CPUMDUMPTYPE enmType;
    const char *pszComment;
    cpumR3InfoParseArg(pszArgs, &enmType, &pszComment);
    pHlp->pfnPrintf(pHlp, "Host CPUM state: %s\n", pszComment);

    PVMCPU pVCpu = VMMGetCpu(pVM);
    if (!pVCpu)
        pVCpu = &pVM->aCpus[0];
    PCPUMHOSTCTX pCtx = &pVCpu->cpum.s.Host;

    /*
     * Format the EFLAGS.
     */
#if HC_ARCH_BITS == 32
    uint32_t efl = pCtx->eflags.u32;
#else
    uint64_t efl = pCtx->rflags;
#endif
    char szEFlags[80];
    cpumR3InfoFormatFlags(&szEFlags[0], efl);

    /*
     * Format the registers.
     */
#if HC_ARCH_BITS == 32
    pHlp->pfnPrintf(pHlp,
        "eax=xxxxxxxx ebx=%08x ecx=xxxxxxxx edx=xxxxxxxx esi=%08x edi=%08x\n"
        "eip=xxxxxxxx esp=%08x ebp=%08x iopl=%d %31s\n"
        "cs=%04x ds=%04x es=%04x fs=%04x gs=%04x                       eflags=%08x\n"
        "cr0=%08RX64 cr2=xxxxxxxx cr3=%08RX64 cr4=%08RX64 gdtr=%08x:%04x ldtr=%04x\n"
        "dr[0]=%08RX64 dr[1]=%08RX64x dr[2]=%08RX64 dr[3]=%08RX64x dr[6]=%08RX64 dr[7]=%08RX64\n"
        "SysEnter={cs=%04x eip=%08x esp=%08x}\n"
        ,
        /*pCtx->eax,*/ pCtx->ebx, /*pCtx->ecx, pCtx->edx,*/ pCtx->esi, pCtx->edi,
        /*pCtx->eip,*/ pCtx->esp, pCtx->ebp, X86_EFL_GET_IOPL(efl), szEFlags,
        pCtx->cs, pCtx->ds, pCtx->es, pCtx->fs, pCtx->gs, efl,
        pCtx->cr0, /*pCtx->cr2,*/ pCtx->cr3, pCtx->cr4,
        pCtx->dr0, pCtx->dr1, pCtx->dr2, pCtx->dr3, pCtx->dr6, pCtx->dr7,
        (uint32_t)pCtx->gdtr.uAddr, pCtx->gdtr.cb, pCtx->ldtr,
        pCtx->SysEnter.cs, pCtx->SysEnter.eip, pCtx->SysEnter.esp);
#else
    pHlp->pfnPrintf(pHlp,
        "rax=xxxxxxxxxxxxxxxx rbx=%016RX64 rcx=xxxxxxxxxxxxxxxx\n"
        "rdx=xxxxxxxxxxxxxxxx rsi=%016RX64 rdi=%016RX64\n"
        "rip=xxxxxxxxxxxxxxxx rsp=%016RX64 rbp=%016RX64\n"
        " r8=xxxxxxxxxxxxxxxx  r9=xxxxxxxxxxxxxxxx r10=%016RX64\n"
        "r11=%016RX64 r12=%016RX64 r13=%016RX64\n"
        "r14=%016RX64 r15=%016RX64\n"
        "iopl=%d  %31s\n"
        "cs=%04x  ds=%04x  es=%04x  fs=%04x  gs=%04x                   eflags=%08RX64\n"
        "cr0=%016RX64 cr2=xxxxxxxxxxxxxxxx cr3=%016RX64\n"
        "cr4=%016RX64 ldtr=%04x tr=%04x\n"
        "dr[0]=%016RX64 dr[1]=%016RX64 dr[2]=%016RX64\n"
        "dr[3]=%016RX64 dr[6]=%016RX64 dr[7]=%016RX64\n"
        "gdtr=%016RX64:%04x  idtr=%016RX64:%04x\n"
        "SysEnter={cs=%04x eip=%08x esp=%08x}\n"
        "FSbase=%016RX64 GSbase=%016RX64 efer=%08RX64\n"
        ,
        /*pCtx->rax,*/ pCtx->rbx, /*pCtx->rcx,
        pCtx->rdx,*/ pCtx->rsi, pCtx->rdi,
        /*pCtx->rip,*/ pCtx->rsp, pCtx->rbp,
        /*pCtx->r8,  pCtx->r9,*/  pCtx->r10,
        pCtx->r11, pCtx->r12, pCtx->r13,
        pCtx->r14, pCtx->r15,
        X86_EFL_GET_IOPL(efl), szEFlags,
        pCtx->cs, pCtx->ds, pCtx->es, pCtx->fs, pCtx->gs, efl,
        pCtx->cr0, /*pCtx->cr2,*/ pCtx->cr3,
        pCtx->cr4, pCtx->ldtr, pCtx->tr,
        pCtx->dr0, pCtx->dr1, pCtx->dr2,
        pCtx->dr3, pCtx->dr6, pCtx->dr7,
        pCtx->gdtr.uAddr, pCtx->gdtr.cb, pCtx->idtr.uAddr, pCtx->idtr.cb,
        pCtx->SysEnter.cs, pCtx->SysEnter.eip, pCtx->SysEnter.esp,
        pCtx->FSbase, pCtx->GSbase, pCtx->efer);
#endif
}

/**
 * Structure used when disassembling and instructions in DBGF.
 * This is used so the reader function can get the stuff it needs.
 */
typedef struct CPUMDISASSTATE
{
    /** Pointer to the CPU structure. */
    PDISCPUSTATE    pCpu;
    /** Pointer to the VM. */
    PVM             pVM;
    /** Pointer to the VMCPU. */
    PVMCPU          pVCpu;
    /** Pointer to the first byte in the segment. */
    RTGCUINTPTR     GCPtrSegBase;
    /** Pointer to the byte after the end of the segment. (might have wrapped!) */
    RTGCUINTPTR     GCPtrSegEnd;
    /** The size of the segment minus 1. */
    RTGCUINTPTR     cbSegLimit;
    /** Pointer to the current page - R3 Ptr. */
    void const     *pvPageR3;
    /** Pointer to the current page - GC Ptr. */
    RTGCPTR         pvPageGC;
    /** The lock information that PGMPhysReleasePageMappingLock needs. */
    PGMPAGEMAPLOCK  PageMapLock;
    /** Whether the PageMapLock is valid or not. */
    bool            fLocked;
    /** 64 bits mode or not. */
    bool            f64Bits;
} CPUMDISASSTATE, *PCPUMDISASSTATE;


/**
 * @callback_method_impl{FNDISREADBYTES}
 */
static DECLCALLBACK(int) cpumR3DisasInstrRead(PDISCPUSTATE pDis, uint8_t offInstr, uint8_t cbMinRead, uint8_t cbMaxRead)
{
    PCPUMDISASSTATE pState = (PCPUMDISASSTATE)pDis->pvUser;
    for (;;)
    {
        RTGCUINTPTR GCPtr = pDis->uInstrAddr + offInstr + pState->GCPtrSegBase;

        /*
         * Need to update the page translation?
         */
        if (   !pState->pvPageR3
            || (GCPtr >> PAGE_SHIFT) != (pState->pvPageGC >> PAGE_SHIFT))
        {
            int rc = VINF_SUCCESS;

            /* translate the address */
            pState->pvPageGC = GCPtr & PAGE_BASE_GC_MASK;
            if (   !HMIsEnabled(pState->pVM)
                && MMHyperIsInsideArea(pState->pVM, pState->pvPageGC))
            {
                pState->pvPageR3 = MMHyperRCToR3(pState->pVM, (RTRCPTR)pState->pvPageGC);
                if (!pState->pvPageR3)
                    rc = VERR_INVALID_POINTER;
            }
            else
            {
                /* Release mapping lock previously acquired. */
                if (pState->fLocked)
                    PGMPhysReleasePageMappingLock(pState->pVM, &pState->PageMapLock);
                rc = PGMPhysGCPtr2CCPtrReadOnly(pState->pVCpu, pState->pvPageGC, &pState->pvPageR3, &pState->PageMapLock);
                pState->fLocked = RT_SUCCESS_NP(rc);
            }
            if (RT_FAILURE(rc))
            {
                pState->pvPageR3 = NULL;
                return rc;
            }
        }

        /*
         * Check the segment limit.
         */
        if (!pState->f64Bits && pDis->uInstrAddr + offInstr > pState->cbSegLimit)
            return VERR_OUT_OF_SELECTOR_BOUNDS;

        /*
         * Calc how much we can read.
         */
        uint32_t cb = PAGE_SIZE - (GCPtr & PAGE_OFFSET_MASK);
        if (!pState->f64Bits)
        {
            RTGCUINTPTR cbSeg = pState->GCPtrSegEnd - GCPtr;
            if (cb > cbSeg && cbSeg)
                cb = cbSeg;
        }
        if (cb > cbMaxRead)
            cb = cbMaxRead;

        /*
         * Read and advance or exit.
         */
        memcpy(&pDis->abInstr[offInstr], (uint8_t *)pState->pvPageR3 + (GCPtr & PAGE_OFFSET_MASK), cb);
        offInstr  += (uint8_t)cb;
        if (cb >= cbMinRead)
        {
            pDis->cbCachedInstr = offInstr;
            return VINF_SUCCESS;
        }
        cbMinRead -= (uint8_t)cb;
        cbMaxRead -= (uint8_t)cb;
    }
}


/**
 * Disassemble an instruction and return the information in the provided structure.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest CPU context.
 * @param   GCPtrPC     Program counter (relative to CS) to disassemble from.
 * @param   pCpu        Disassembly state.
 * @param   pszPrefix   String prefix for logging (debug only).
 *
 */
VMMR3DECL(int) CPUMR3DisasmInstrCPU(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, RTGCPTR GCPtrPC, PDISCPUSTATE pCpu,
                                    const char *pszPrefix)
{
    CPUMDISASSTATE  State;
    int             rc;

    const PGMMODE enmMode = PGMGetGuestMode(pVCpu);
    State.pCpu            = pCpu;
    State.pvPageGC        = 0;
    State.pvPageR3        = NULL;
    State.pVM             = pVM;
    State.pVCpu           = pVCpu;
    State.fLocked         = false;
    State.f64Bits         = false;

    /*
     * Get selector information.
     */
    DISCPUMODE enmDisCpuMode;
    if (    (pCtx->cr0 & X86_CR0_PE)
        &&   pCtx->eflags.Bits.u1VM == 0)
    {
        if (!CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->cs))
        {
# ifdef VBOX_WITH_RAW_MODE_NOT_R0
            CPUMGuestLazyLoadHiddenSelectorReg(pVCpu, &pCtx->cs);
# endif
            if (!CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->cs))
                return VERR_CPUM_HIDDEN_CS_LOAD_ERROR;
        }
        State.f64Bits         = enmMode >= PGMMODE_AMD64 && pCtx->cs.Attr.n.u1Long;
        State.GCPtrSegBase    = pCtx->cs.u64Base;
        State.GCPtrSegEnd     = pCtx->cs.u32Limit + 1 + (RTGCUINTPTR)pCtx->cs.u64Base;
        State.cbSegLimit      = pCtx->cs.u32Limit;
        enmDisCpuMode         = (State.f64Bits)
                              ? DISCPUMODE_64BIT
                              : pCtx->cs.Attr.n.u1DefBig
                              ? DISCPUMODE_32BIT
                              : DISCPUMODE_16BIT;
    }
    else
    {
        /* real or V86 mode */
        enmDisCpuMode         = DISCPUMODE_16BIT;
        State.GCPtrSegBase    = pCtx->cs.Sel * 16;
        State.GCPtrSegEnd     = 0xFFFFFFFF;
        State.cbSegLimit      = 0xFFFFFFFF;
    }

    /*
     * Disassemble the instruction.
     */
    uint32_t cbInstr;
#ifndef LOG_ENABLED
    RT_NOREF_PV(pszPrefix);
    rc = DISInstrWithReader(GCPtrPC, enmDisCpuMode, cpumR3DisasInstrRead, &State, pCpu, &cbInstr);
    if (RT_SUCCESS(rc))
    {
#else
    char szOutput[160];
    rc = DISInstrToStrWithReader(GCPtrPC, enmDisCpuMode, cpumR3DisasInstrRead, &State,
                                 pCpu, &cbInstr, szOutput, sizeof(szOutput));
    if (RT_SUCCESS(rc))
    {
        /* log it */
        if (pszPrefix)
            Log(("%s-CPU%d: %s", pszPrefix, pVCpu->idCpu, szOutput));
        else
            Log(("%s", szOutput));
#endif
        rc = VINF_SUCCESS;
    }
    else
        Log(("CPUMR3DisasmInstrCPU: DISInstr failed for %04X:%RGv rc=%Rrc\n", pCtx->cs.Sel, GCPtrPC, rc));

    /* Release mapping lock acquired in cpumR3DisasInstrRead. */
    if (State.fLocked)
        PGMPhysReleasePageMappingLock(pVM, &State.PageMapLock);

    return rc;
}



/**
 * API for controlling a few of the CPU features found in CR4.
 *
 * Currently only X86_CR4_TSD is accepted as input.
 *
 * @returns VBox status code.
 *
 * @param   pVM     The cross context VM structure.
 * @param   fOr     The CR4 OR mask.
 * @param   fAnd    The CR4 AND mask.
 */
VMMR3DECL(int) CPUMR3SetCR4Feature(PVM pVM, RTHCUINTREG fOr, RTHCUINTREG fAnd)
{
    AssertMsgReturn(!(fOr & ~(X86_CR4_TSD)), ("%#x\n", fOr), VERR_INVALID_PARAMETER);
    AssertMsgReturn((fAnd & ~(X86_CR4_TSD)) == ~(X86_CR4_TSD), ("%#x\n", fAnd), VERR_INVALID_PARAMETER);

    pVM->cpum.s.CR4.OrMask &= fAnd;
    pVM->cpum.s.CR4.OrMask |= fOr;

    return VINF_SUCCESS;
}


/**
 * Enters REM, gets and resets the changed flags (CPUM_CHANGED_*).
 *
 * Only REM should ever call this function!
 *
 * @returns The changed flags.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   puCpl       Where to return the current privilege level (CPL).
 */
VMMR3DECL(uint32_t) CPUMR3RemEnter(PVMCPU pVCpu, uint32_t *puCpl)
{
    Assert(!pVCpu->cpum.s.fRawEntered);
    Assert(!pVCpu->cpum.s.fRemEntered);

    /*
     * Get the CPL first.
     */
    *puCpl = CPUMGetGuestCPL(pVCpu);

    /*
     * Get and reset the flags.
     */
    uint32_t fFlags = pVCpu->cpum.s.fChanged;
    pVCpu->cpum.s.fChanged = 0;

    /** @todo change the switcher to use the fChanged flags. */
    if (pVCpu->cpum.s.fUseFlags & CPUM_USED_FPU_SINCE_REM)
    {
        fFlags |= CPUM_CHANGED_FPU_REM;
        pVCpu->cpum.s.fUseFlags &= ~CPUM_USED_FPU_SINCE_REM;
    }

    pVCpu->cpum.s.fRemEntered = true;
    return fFlags;
}


/**
 * Leaves REM.
 *
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   fNoOutOfSyncSels    This is @c false if there are out of sync
 *                              registers.
 */
VMMR3DECL(void) CPUMR3RemLeave(PVMCPU pVCpu, bool fNoOutOfSyncSels)
{
    Assert(!pVCpu->cpum.s.fRawEntered);
    Assert(pVCpu->cpum.s.fRemEntered);

    RT_NOREF_PV(fNoOutOfSyncSels);

    pVCpu->cpum.s.fRemEntered = false;
}


/**
 * Called when the ring-3 init phase completes.
 *
 * @returns VBox status code.
 * @param   pVM                 The cross context VM structure.
 * @param   enmWhat             Which init phase.
 */
VMMR3DECL(int) CPUMR3InitCompleted(PVM pVM, VMINITCOMPLETED enmWhat)
{
    switch (enmWhat)
    {
        case VMINITCOMPLETED_RING3:
        {
            /*
             * Figure out if the guest uses 32-bit or 64-bit FPU state at runtime for 64-bit capable VMs.
             * Only applicable/used on 64-bit hosts, refer CPUMR0A.asm. See @bugref{7138}.
             */
            bool const fSupportsLongMode = VMR3IsLongModeAllowed(pVM);
            for (VMCPUID i = 0; i < pVM->cCpus; i++)
            {
                PVMCPU pVCpu = &pVM->aCpus[i];
                /* While loading a saved-state we fix it up in, cpumR3LoadDone(). */
                if (fSupportsLongMode)
                    pVCpu->cpum.s.fUseFlags |= CPUM_USE_SUPPORTS_LONGMODE;
            }

            cpumR3MsrRegStats(pVM);
            break;
        }

        default:
            break;
    }
    return VINF_SUCCESS;
}


/**
 * Called when the ring-0 init phases completed.
 *
 * @param   pVM                 The cross context VM structure.
 */
VMMR3DECL(void) CPUMR3LogCpuIds(PVM pVM)
{
    /*
     * Log the cpuid.
     */
    bool fOldBuffered = RTLogRelSetBuffering(true /*fBuffered*/);
    RTCPUSET OnlineSet;
    LogRel(("CPUM: Logical host processors: %u present, %u max, %u online, online mask: %016RX64\n",
                (unsigned)RTMpGetPresentCount(), (unsigned)RTMpGetCount(), (unsigned)RTMpGetOnlineCount(),
                RTCpuSetToU64(RTMpGetOnlineSet(&OnlineSet)) ));
    RTCPUID cCores = RTMpGetCoreCount();
    if (cCores)
        LogRel(("CPUM: Physical host cores: %u\n", (unsigned)cCores));
    LogRel(("************************* CPUID dump ************************\n"));
    DBGFR3Info(pVM->pUVM, "cpuid", "verbose", DBGFR3InfoLogRelHlp());
    LogRel(("\n"));
    DBGFR3_INFO_LOG_SAFE(pVM, "cpuid", "verbose"); /* macro */
    RTLogRelSetBuffering(fOldBuffered);
    LogRel(("******************** End of CPUID dump **********************\n"));
}

