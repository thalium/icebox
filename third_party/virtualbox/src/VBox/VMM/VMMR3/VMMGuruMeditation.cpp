/* $Id: VMMGuruMeditation.cpp $ */
/** @file
 * VMM - The Virtual Machine Monitor, Guru Meditation Code.
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
#define LOG_GROUP LOG_GROUP_VMM
#include <VBox/vmm/vmm.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/vmm/pdmcritsect.h>
#include <VBox/vmm/trpm.h>
#include <VBox/vmm/dbgf.h>
#include "VMMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/iom.h>
#include <VBox/vmm/em.h>

#include <VBox/err.h>
#include <VBox/param.h>
#include <VBox/version.h>
#include <VBox/vmm/hm.h>
#include <iprt/assert.h>
#include <iprt/dbg.h>
#include <iprt/time.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/stdarg.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Structure to pass to DBGFR3Info() and for doing all other
 * output during fatal dump.
 */
typedef struct VMMR3FATALDUMPINFOHLP
{
    /** The helper core. */
    DBGFINFOHLP Core;
    /** The release logger instance. */
    PRTLOGGER   pRelLogger;
    /** The saved release logger flags. */
    uint32_t    fRelLoggerFlags;
    /** The logger instance. */
    PRTLOGGER   pLogger;
    /** The saved logger flags. */
    uint32_t    fLoggerFlags;
    /** The saved logger destination flags. */
    uint32_t    fLoggerDestFlags;
    /** Whether to output to stderr or not. */
    bool        fStdErr;
    /** Whether we're still recording the summary or not. */
    bool        fRecSummary;
    /** Buffer for the summary. */
    char        szSummary[4096-2];
    /** The current summary offset. */
    size_t      offSummary;
} VMMR3FATALDUMPINFOHLP, *PVMMR3FATALDUMPINFOHLP;
/** Pointer to a VMMR3FATALDUMPINFOHLP structure. */
typedef const VMMR3FATALDUMPINFOHLP *PCVMMR3FATALDUMPINFOHLP;


/**
 * Print formatted string.
 *
 * @param   pHlp        Pointer to this structure.
 * @param   pszFormat   The format string.
 * @param   ...         Arguments.
 */
static DECLCALLBACK(void) vmmR3FatalDumpInfoHlp_pfnPrintf(PCDBGFINFOHLP pHlp, const char *pszFormat, ...)
{
    va_list args;
    va_start(args, pszFormat);
    pHlp->pfnPrintfV(pHlp, pszFormat, args);
    va_end(args);
}


/**
 * Print formatted string.
 *
 * @param   pHlp        Pointer to this structure.
 * @param   pszFormat   The format string.
 * @param   args        Argument list.
 */
static DECLCALLBACK(void) vmmR3FatalDumpInfoHlp_pfnPrintfV(PCDBGFINFOHLP pHlp, const char *pszFormat, va_list args)
{
    PVMMR3FATALDUMPINFOHLP pMyHlp = (PVMMR3FATALDUMPINFOHLP)pHlp;

    if (pMyHlp->pRelLogger)
    {
        va_list args2;
        va_copy(args2, args);
        RTLogLoggerV(pMyHlp->pRelLogger, pszFormat, args2);
        va_end(args2);
    }
    if (pMyHlp->pLogger)
    {
        va_list args2;
        va_copy(args2, args);
        RTLogLoggerV(pMyHlp->pLogger, pszFormat, args);
        va_end(args2);
    }
    if (pMyHlp->fStdErr)
    {
        va_list args2;
        va_copy(args2, args);
        RTStrmPrintfV(g_pStdErr, pszFormat, args);
        va_end(args2);
    }
    if (pMyHlp->fRecSummary)
    {
        size_t cchLeft = sizeof(pMyHlp->szSummary) - pMyHlp->offSummary;
        if (cchLeft > 1)
        {
            va_list args2;
            va_copy(args2, args);
            size_t cch = RTStrPrintfV(&pMyHlp->szSummary[pMyHlp->offSummary], cchLeft, pszFormat, args);
            va_end(args2);
            Assert(cch <= cchLeft);
            pMyHlp->offSummary += cch;
        }
    }
}


/**
 * Initializes the fatal dump output helper.
 *
 * @param   pHlp        The structure to initialize.
 */
static void vmmR3FatalDumpInfoHlpInit(PVMMR3FATALDUMPINFOHLP pHlp)
{
    RT_BZERO(pHlp, sizeof(*pHlp));

    pHlp->Core.pfnPrintf  = vmmR3FatalDumpInfoHlp_pfnPrintf;
    pHlp->Core.pfnPrintfV = vmmR3FatalDumpInfoHlp_pfnPrintfV;

    /*
     * The loggers.
     */
    pHlp->pRelLogger  = RTLogRelGetDefaultInstance();
#ifdef LOG_ENABLED
    pHlp->pLogger     = RTLogDefaultInstance();
#else
    if (pHlp->pRelLogger)
        pHlp->pLogger = RTLogGetDefaultInstance();
    else
        pHlp->pLogger = RTLogDefaultInstance();
#endif

    if (pHlp->pRelLogger)
    {
        pHlp->fRelLoggerFlags = pHlp->pRelLogger->fFlags;
        pHlp->pRelLogger->fFlags &= ~RTLOGFLAGS_DISABLED;
        pHlp->pRelLogger->fFlags |= RTLOGFLAGS_BUFFERED;
    }

    if (pHlp->pLogger)
    {
        pHlp->fLoggerFlags     = pHlp->pLogger->fFlags;
        pHlp->fLoggerDestFlags = pHlp->pLogger->fDestFlags;
        pHlp->pLogger->fFlags     &= ~RTLOGFLAGS_DISABLED;
        pHlp->pLogger->fFlags     |= RTLOGFLAGS_BUFFERED;
#ifndef DEBUG_sandervl
        pHlp->pLogger->fDestFlags |= RTLOGDEST_DEBUGGER;
#endif
    }

    /*
     * Check if we need write to stderr.
     */
    pHlp->fStdErr = (!pHlp->pRelLogger || !(pHlp->pRelLogger->fDestFlags & (RTLOGDEST_STDOUT | RTLOGDEST_STDERR)))
                 && (!pHlp->pLogger    || !(pHlp->pLogger->fDestFlags    & (RTLOGDEST_STDOUT | RTLOGDEST_STDERR)));
#ifdef DEBUG_sandervl
    pHlp->fStdErr = false; /* takes too long to display here */
#endif

    /*
     * Init the summary recording.
     */
    pHlp->fRecSummary  = true;
    pHlp->offSummary   = 0;
    pHlp->szSummary[0] = '\0';
}


/**
 * Deletes the fatal dump output helper.
 *
 * @param   pHlp        The structure to delete.
 */
static void vmmR3FatalDumpInfoHlpDelete(PVMMR3FATALDUMPINFOHLP pHlp)
{
    if (pHlp->pRelLogger)
    {
        RTLogFlush(pHlp->pRelLogger);
        pHlp->pRelLogger->fFlags = pHlp->fRelLoggerFlags;
    }

    if (pHlp->pLogger)
    {
        RTLogFlush(pHlp->pLogger);
        pHlp->pLogger->fFlags     = pHlp->fLoggerFlags;
        pHlp->pLogger->fDestFlags = pHlp->fLoggerDestFlags;
    }
}


/**
 * Dumps the VM state on a fatal error.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   rcErr       VBox status code.
 */
VMMR3DECL(void) VMMR3FatalDump(PVM pVM, PVMCPU pVCpu, int rcErr)
{
    /*
     * Create our output helper and sync it with the log settings.
     * This helper will be used for all the output.
     */
    VMMR3FATALDUMPINFOHLP   Hlp;
    PCDBGFINFOHLP           pHlp = &Hlp.Core;
    vmmR3FatalDumpInfoHlpInit(&Hlp);

    /* Release owned locks to make sure other VCPUs can continue in case they were waiting for one. */
    PDMR3CritSectLeaveAll(pVM);

    /*
     * Header.
     */
    pHlp->pfnPrintf(pHlp,
                    "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
                    "!!\n"
                    "!!         VCPU%u: Guru Meditation %d (%Rrc)\n"
                    "!!\n",
                    pVCpu->idCpu, rcErr, rcErr);

    /*
     * Continue according to context.
     */
    bool fDoneHyper = false;
    switch (rcErr)
    {
        /*
         * Hypervisor errors.
         */
        case VERR_VMM_RING0_ASSERTION:
        case VINF_EM_DBG_HYPER_ASSERTION:
        case VERR_VMM_RING3_CALL_DISABLED:
        {
            const char *pszMsg1 = VMMR3GetRZAssertMsg1(pVM);
            while (pszMsg1 && *pszMsg1 == '\n')
                pszMsg1++;
            const char *pszMsg2 = VMMR3GetRZAssertMsg2(pVM);
            while (pszMsg2 && *pszMsg2 == '\n')
                pszMsg2++;
            pHlp->pfnPrintf(pHlp,
                            "%s"
                            "%s",
                            pszMsg1,
                            pszMsg2);
            if (    !pszMsg2
                ||  !*pszMsg2
                ||  strchr(pszMsg2, '\0')[-1] != '\n')
                pHlp->pfnPrintf(pHlp, "\n");
        }
        RT_FALL_THRU();
        case VERR_TRPM_DONT_PANIC:
        case VERR_TRPM_PANIC:
        case VINF_EM_RAW_STALE_SELECTOR:
        case VINF_EM_RAW_IRET_TRAP:
        case VINF_EM_DBG_HYPER_BREAKPOINT:
        case VINF_EM_DBG_HYPER_STEPPED:
        case VINF_EM_TRIPLE_FAULT:
        case VERR_VMM_HYPER_CR3_MISMATCH:
        {
            /*
             * Active trap? This is only of partial interest when in hardware
             * assisted virtualization mode, thus the different messages.
             */
            uint32_t        uEIP       = CPUMGetHyperEIP(pVCpu);
            TRPMEVENT       enmType;
            uint8_t         u8TrapNo   =       0xce;
            RTGCUINT        uErrorCode = 0xdeadface;
            RTGCUINTPTR     uCR2       = 0xdeadface;
            uint8_t         cbInstr    = UINT8_MAX;
            int rc2 = TRPMQueryTrapAll(pVCpu, &u8TrapNo, &enmType, &uErrorCode, &uCR2, &cbInstr);
            if (!HMIsEnabled(pVM))
            {
                if (RT_SUCCESS(rc2))
                    pHlp->pfnPrintf(pHlp,
                                    "!! TRAP=%02x ERRCD=%RGv CR2=%RGv EIP=%RX32 Type=%d cbInstr=%02x\n",
                                    u8TrapNo, uErrorCode, uCR2, uEIP, enmType, cbInstr);
                else
                    pHlp->pfnPrintf(pHlp,
                                    "!! EIP=%RX32 NOTRAP\n",
                                    uEIP);
            }
            else if (RT_SUCCESS(rc2))
                pHlp->pfnPrintf(pHlp,
                                "!! ACTIVE TRAP=%02x ERRCD=%RGv CR2=%RGv PC=%RGr Type=%d cbInstr=%02x (Guest!)\n",
                                u8TrapNo, uErrorCode, uCR2, CPUMGetGuestRIP(pVCpu), enmType, cbInstr);

            /*
             * Dump the relevant hypervisor registers and stack.
             */
            if (HMIsEnabled(pVM))
            {
                if (   rcErr == VERR_VMM_RING0_ASSERTION /* fInRing3Call has already been cleared here. */
                    || pVCpu->vmm.s.CallRing3JmpBufR0.fInRing3Call)
                {
                    /* Dump the jmpbuf.  */
                    pHlp->pfnPrintf(pHlp,
                                    "!!\n"
                                    "!! CallRing3JmpBuf:\n"
                                    "!!\n");
                    pHlp->pfnPrintf(pHlp,
                                    "SavedEsp=%RHv SavedEbp=%RHv SpResume=%RHv SpCheck=%RHv\n",
                                    pVCpu->vmm.s.CallRing3JmpBufR0.SavedEsp,
                                    pVCpu->vmm.s.CallRing3JmpBufR0.SavedEbp,
                                    pVCpu->vmm.s.CallRing3JmpBufR0.SpResume,
                                    pVCpu->vmm.s.CallRing3JmpBufR0.SpCheck);
                    pHlp->pfnPrintf(pHlp,
                                    "pvSavedStack=%RHv cbSavedStack=%#x  fInRing3Call=%RTbool\n",
                                    pVCpu->vmm.s.CallRing3JmpBufR0.pvSavedStack,
                                    pVCpu->vmm.s.CallRing3JmpBufR0.cbSavedStack,
                                    pVCpu->vmm.s.CallRing3JmpBufR0.fInRing3Call);
                    pHlp->pfnPrintf(pHlp,
                                    "cbUsedMax=%#x cbUsedAvg=%#x cbUsedTotal=%#llx cUsedTotal=%#llx\n",
                                    pVCpu->vmm.s.CallRing3JmpBufR0.cbUsedMax,
                                    pVCpu->vmm.s.CallRing3JmpBufR0.cbUsedAvg,
                                    pVCpu->vmm.s.CallRing3JmpBufR0.cbUsedTotal,
                                    pVCpu->vmm.s.CallRing3JmpBufR0.cUsedTotal);

                    /* Dump the resume register frame on the stack. */
                    PRTHCUINTPTR pBP;
#ifdef VMM_R0_SWITCH_STACK
                    pBP = (PRTHCUINTPTR)&pVCpu->vmm.s.pbEMTStackR3[  pVCpu->vmm.s.CallRing3JmpBufR0.SavedEbp
                                                                   - MMHyperCCToR0(pVM, pVCpu->vmm.s.pbEMTStackR3)];
#else
                    pBP = (PRTHCUINTPTR)&pVCpu->vmm.s.pbEMTStackR3[  pVCpu->vmm.s.CallRing3JmpBufR0.cbSavedStack
                                                                   - pVCpu->vmm.s.CallRing3JmpBufR0.SpCheck
                                                                   + pVCpu->vmm.s.CallRing3JmpBufR0.SavedEbp];
#endif
#if HC_ARCH_BITS == 32
                    pHlp->pfnPrintf(pHlp,
                                    "eax=volatile ebx=%08x ecx=volatile edx=volatile esi=%08x edi=%08x\n"
                                    "eip=%08x esp=%08x ebp=%08x efl=%08x\n"
                                    ,
                                    pBP[-3], pBP[-2], pBP[-1],
                                    pBP[1], pVCpu->vmm.s.CallRing3JmpBufR0.SavedEbp - 8, pBP[0], pBP[-4]);
#else
# ifdef RT_OS_WINDOWS
                    pHlp->pfnPrintf(pHlp,
                                    "rax=volatile         rbx=%016RX64 rcx=volatile         rdx=volatile\n"
                                    "rsi=%016RX64 rdi=%016RX64  r8=volatile          r9=volatile        \n"
                                    "r10=volatile         r11=volatile         r12=%016RX64 r13=%016RX64\n"
                                    "r14=%016RX64 r15=%016RX64\n"
                                    "rip=%016RX64 rsp=%016RX64 rbp=%016RX64 rfl=%08RX64\n"
                                    ,
                                    pBP[-7],
                                    pBP[-6], pBP[-5],
                                    pBP[-4], pBP[-3],
                                    pBP[-2], pBP[-1],
                                    pBP[1], pVCpu->vmm.s.CallRing3JmpBufR0.SavedEbp - 16, pBP[0], pBP[-8]);
# else
                    pHlp->pfnPrintf(pHlp,
                                    "rax=volatile         rbx=%016RX64 rcx=volatile         rdx=volatile\n"
                                    "rsi=volatile         rdi=volatile          r8=volatile          r9=volatile        \n"
                                    "r10=volatile         r11=volatile         r12=%016RX64 r13=%016RX64\n"
                                    "r14=%016RX64 r15=%016RX64\n"
                                    "rip=%016RX64 rsp=%016RX64 rbp=%016RX64 rflags=%08RX64\n"
                                    ,
                                    pBP[-5],
                                    pBP[-4], pBP[-3],
                                    pBP[-2], pBP[-1],
                                    pBP[1], pVCpu->vmm.s.CallRing3JmpBufR0.SavedEbp - 16, pBP[0], pBP[-6]);
# endif
#endif

                    /* Callstack. */
                    DBGFADDRESS pc;
                    pc.fFlags    = DBGFADDRESS_FLAGS_RING0 | DBGFADDRESS_FLAGS_VALID;
#if HC_ARCH_BITS == 64
                    pc.FlatPtr   = pc.off = pVCpu->vmm.s.CallRing3JmpBufR0.rip;
#else
                    pc.FlatPtr   = pc.off = pVCpu->vmm.s.CallRing3JmpBufR0.eip;
#endif
                    pc.Sel       = DBGF_SEL_FLAT;

                    DBGFADDRESS ebp;
                    ebp.fFlags   = DBGFADDRESS_FLAGS_RING0 | DBGFADDRESS_FLAGS_VALID;
                    ebp.FlatPtr  = ebp.off = pVCpu->vmm.s.CallRing3JmpBufR0.SavedEbp;
                    ebp.Sel      = DBGF_SEL_FLAT;

                    DBGFADDRESS esp;
                    esp.fFlags   = DBGFADDRESS_FLAGS_RING0 | DBGFADDRESS_FLAGS_VALID;
                    esp.Sel      = DBGF_SEL_FLAT;
                    esp.FlatPtr  = esp.off = pVCpu->vmm.s.CallRing3JmpBufR0.SavedEsp;

                    PCDBGFSTACKFRAME pFirstFrame;
                    rc2 = DBGFR3StackWalkBeginEx(pVM->pUVM, pVCpu->idCpu, DBGFCODETYPE_RING0, &ebp, &esp, &pc,
                                                 DBGFRETURNTYPE_INVALID, &pFirstFrame);
                    if (RT_SUCCESS(rc2))
                    {
                        pHlp->pfnPrintf(pHlp,
                                        "!!\n"
                                        "!! Call Stack:\n"
                                        "!!\n");
#if HC_ARCH_BITS == 32
                        pHlp->pfnPrintf(pHlp, "EBP      Ret EBP  Ret CS:EIP    Arg0     Arg1     Arg2     Arg3     CS:EIP        Symbol [line]\n");
#else
                        pHlp->pfnPrintf(pHlp, "RBP              Ret RBP          Ret RIP          RIP              Symbol [line]\n");
#endif
                        for (PCDBGFSTACKFRAME pFrame = pFirstFrame;
                             pFrame;
                             pFrame = DBGFR3StackWalkNext(pFrame))
                        {
#if HC_ARCH_BITS == 32
                            pHlp->pfnPrintf(pHlp,
                                            "%RHv %RHv %04RX32:%RHv %RHv %RHv %RHv %RHv",
                                            (RTHCUINTPTR)pFrame->AddrFrame.off,
                                            (RTHCUINTPTR)pFrame->AddrReturnFrame.off,
                                            (RTHCUINTPTR)pFrame->AddrReturnPC.Sel,
                                            (RTHCUINTPTR)pFrame->AddrReturnPC.off,
                                            pFrame->Args.au32[0],
                                            pFrame->Args.au32[1],
                                            pFrame->Args.au32[2],
                                            pFrame->Args.au32[3]);
                            pHlp->pfnPrintf(pHlp, " %RTsel:%08RHv", pFrame->AddrPC.Sel, pFrame->AddrPC.off);
#else
                            pHlp->pfnPrintf(pHlp,
                                            "%RHv %RHv %RHv %RHv",
                                            (RTHCUINTPTR)pFrame->AddrFrame.off,
                                            (RTHCUINTPTR)pFrame->AddrReturnFrame.off,
                                            (RTHCUINTPTR)pFrame->AddrReturnPC.off,
                                            (RTHCUINTPTR)pFrame->AddrPC.off);
#endif
                            if (pFrame->pSymPC)
                            {
                                RTGCINTPTR offDisp = pFrame->AddrPC.FlatPtr - pFrame->pSymPC->Value;
                                if (offDisp > 0)
                                    pHlp->pfnPrintf(pHlp, " %s+%llx", pFrame->pSymPC->szName, (int64_t)offDisp);
                                else if (offDisp < 0)
                                    pHlp->pfnPrintf(pHlp, " %s-%llx", pFrame->pSymPC->szName, -(int64_t)offDisp);
                                else
                                    pHlp->pfnPrintf(pHlp, " %s", pFrame->pSymPC->szName);
                            }
                            if (pFrame->pLinePC)
                                pHlp->pfnPrintf(pHlp, " [%s @ 0i%d]", pFrame->pLinePC->szFilename, pFrame->pLinePC->uLineNo);
                            pHlp->pfnPrintf(pHlp, "\n");
                        }
                        DBGFR3StackWalkEnd(pFirstFrame);
                    }

                    /* Symbols on the stack. */
#ifdef VMM_R0_SWITCH_STACK
                    uint32_t const   iLast   = VMM_STACK_SIZE / sizeof(uintptr_t);
                    uint32_t         iAddr   = (uint32_t)(  pVCpu->vmm.s.CallRing3JmpBufR0.SavedEsp
                                                          - MMHyperCCToR0(pVM, pVCpu->vmm.s.pbEMTStackR3)) / sizeof(uintptr_t);
                    if (iAddr > iLast)
                        iAddr = 0;
#else
                    uint32_t const   iLast   = RT_MIN(pVCpu->vmm.s.CallRing3JmpBufR0.cbSavedStack, VMM_STACK_SIZE)
                                             / sizeof(uintptr_t);
                    uint32_t         iAddr   = 0;
#endif
                    pHlp->pfnPrintf(pHlp,
                                    "!!\n"
                                    "!! Addresses on the stack (iAddr=%#x, iLast=%#x)\n"
                                    "!!\n",
                                    iAddr, iLast);
                    uintptr_t const *paAddr  = (uintptr_t const *)pVCpu->vmm.s.pbEMTStackR3;
                    while (iAddr < iLast)
                    {
                        uintptr_t const uAddr = paAddr[iAddr];
                        if (uAddr > X86_PAGE_SIZE)
                        {
                            DBGFADDRESS  Addr;
                            DBGFR3AddrFromFlat(pVM->pUVM, &Addr, uAddr);
                            RTGCINTPTR   offDisp = 0;
                            PRTDBGSYMBOL pSym  = DBGFR3AsSymbolByAddrA(pVM->pUVM, DBGF_AS_R0, &Addr,
                                                                       RTDBGSYMADDR_FLAGS_LESS_OR_EQUAL, &offDisp, NULL);
                            RTGCINTPTR   offLineDisp;
                            PRTDBGLINE   pLine = DBGFR3AsLineByAddrA(pVM->pUVM, DBGF_AS_R0, &Addr, &offLineDisp, NULL);
                            if (pLine || pSym)
                            {
                                pHlp->pfnPrintf(pHlp, "%#06x: %p =>", iAddr * sizeof(uintptr_t), uAddr);
                                if (pSym)
                                    pHlp->pfnPrintf(pHlp, " %s + %#x", pSym->szName, (intptr_t)offDisp);
                                if (pLine)
                                    pHlp->pfnPrintf(pHlp, " [%s:%u + %#x]\n", pLine->szFilename, pLine->uLineNo, offLineDisp);
                                else
                                    pHlp->pfnPrintf(pHlp, "\n");
                                RTDbgSymbolFree(pSym);
                                RTDbgLineFree(pLine);
                            }
                        }
                        iAddr++;
                    }

                    /* raw stack */
                    Hlp.fRecSummary = false;
                    pHlp->pfnPrintf(pHlp,
                                    "!!\n"
                                    "!! Raw stack (mind the direction).\n"
                                    "!! pbEMTStackR0=%RHv pbEMTStackBottomR0=%RHv VMM_STACK_SIZE=%#x\n"
                                    "!! pbEmtStackR3=%p\n"
                                    "!!\n"
                                    "%.*Rhxd\n",
                                    MMHyperCCToR0(pVM, pVCpu->vmm.s.pbEMTStackR3),
                                    MMHyperCCToR0(pVM, pVCpu->vmm.s.pbEMTStackR3) + VMM_STACK_SIZE,
                                    VMM_STACK_SIZE,
                                    pVCpu->vmm.s.pbEMTStackR3,
                                    VMM_STACK_SIZE, pVCpu->vmm.s.pbEMTStackR3);
                }
                else
                {
                    pHlp->pfnPrintf(pHlp,
                                    "!! Skipping ring-0 registers and stack, rcErr=%Rrc\n", rcErr);
                }
            }
            else
            {
                /*
                 * Try figure out where eip is.
                 */
                /* core code? */
                if (uEIP - (RTGCUINTPTR)pVM->vmm.s.pvCoreCodeRC < pVM->vmm.s.cbCoreCode)
                    pHlp->pfnPrintf(pHlp,
                                "!! EIP is in CoreCode, offset %#x\n",
                                uEIP - (RTGCUINTPTR)pVM->vmm.s.pvCoreCodeRC);
                else
                {   /* ask PDM */  /** @todo ask DBGFR3Sym later? */
                    char        szModName[64];
                    RTRCPTR     RCPtrMod;
                    char        szNearSym1[260];
                    RTRCPTR     RCPtrNearSym1;
                    char        szNearSym2[260];
                    RTRCPTR     RCPtrNearSym2;
                    int rc = PDMR3LdrQueryRCModFromPC(pVM, uEIP,
                                                      &szModName[0],  sizeof(szModName),  &RCPtrMod,
                                                      &szNearSym1[0], sizeof(szNearSym1), &RCPtrNearSym1,
                                                      &szNearSym2[0], sizeof(szNearSym2), &RCPtrNearSym2);
                    if (RT_SUCCESS(rc))
                        pHlp->pfnPrintf(pHlp,
                                        "!! EIP in %s (%RRv) at rva %x near symbols:\n"
                                        "!!    %RRv rva %RRv off %08x  %s\n"
                                        "!!    %RRv rva %RRv off -%08x %s\n",
                                        szModName,  RCPtrMod, (unsigned)(uEIP - RCPtrMod),
                                        RCPtrNearSym1, RCPtrNearSym1 - RCPtrMod, (unsigned)(uEIP - RCPtrNearSym1), szNearSym1,
                                        RCPtrNearSym2, RCPtrNearSym2 - RCPtrMod, (unsigned)(RCPtrNearSym2 - uEIP), szNearSym2);
                    else
                        pHlp->pfnPrintf(pHlp,
                                        "!! EIP is not in any code known to VMM!\n");
                }

                /* Disassemble the instruction. */
                char szInstr[256];
                rc2 = DBGFR3DisasInstrEx(pVM->pUVM, pVCpu->idCpu, 0, 0,
                                         DBGF_DISAS_FLAGS_CURRENT_HYPER | DBGF_DISAS_FLAGS_DEFAULT_MODE,
                                         &szInstr[0], sizeof(szInstr), NULL);
                if (RT_SUCCESS(rc2))
                    pHlp->pfnPrintf(pHlp,
                                    "!! %s\n", szInstr);

                /* Dump the hypervisor cpu state. */
                pHlp->pfnPrintf(pHlp,
                                "!!\n"
                                "!!\n"
                                "!!\n");
                rc2 = DBGFR3Info(pVM->pUVM, "cpumhyper", "verbose", pHlp);
                fDoneHyper = true;

                /* Callstack. */
                PCDBGFSTACKFRAME pFirstFrame;
                rc2 = DBGFR3StackWalkBegin(pVM->pUVM, pVCpu->idCpu, DBGFCODETYPE_HYPER, &pFirstFrame);
                if (RT_SUCCESS(rc2))
                {
                    pHlp->pfnPrintf(pHlp,
                                    "!!\n"
                                    "!! Call Stack:\n"
                                    "!!\n"
                                    "EBP      Ret EBP  Ret CS:EIP    Arg0     Arg1     Arg2     Arg3     CS:EIP        Symbol [line]\n");
                    for (PCDBGFSTACKFRAME pFrame = pFirstFrame;
                         pFrame;
                         pFrame = DBGFR3StackWalkNext(pFrame))
                    {
                        pHlp->pfnPrintf(pHlp,
                                        "%08RX32 %08RX32 %04RX32:%08RX32 %08RX32 %08RX32 %08RX32 %08RX32",
                                        (uint32_t)pFrame->AddrFrame.off,
                                        (uint32_t)pFrame->AddrReturnFrame.off,
                                        (uint32_t)pFrame->AddrReturnPC.Sel,
                                        (uint32_t)pFrame->AddrReturnPC.off,
                                        pFrame->Args.au32[0],
                                        pFrame->Args.au32[1],
                                        pFrame->Args.au32[2],
                                        pFrame->Args.au32[3]);
                        pHlp->pfnPrintf(pHlp, " %RTsel:%08RGv", pFrame->AddrPC.Sel, pFrame->AddrPC.off);
                        if (pFrame->pSymPC)
                        {
                            RTGCINTPTR offDisp = pFrame->AddrPC.FlatPtr - pFrame->pSymPC->Value;
                            if (offDisp > 0)
                                pHlp->pfnPrintf(pHlp, " %s+%llx", pFrame->pSymPC->szName, (int64_t)offDisp);
                            else if (offDisp < 0)
                                pHlp->pfnPrintf(pHlp, " %s-%llx", pFrame->pSymPC->szName, -(int64_t)offDisp);
                            else
                                pHlp->pfnPrintf(pHlp, " %s", pFrame->pSymPC->szName);
                        }
                        if (pFrame->pLinePC)
                            pHlp->pfnPrintf(pHlp, " [%s @ 0i%d]", pFrame->pLinePC->szFilename, pFrame->pLinePC->uLineNo);
                        pHlp->pfnPrintf(pHlp, "\n");
                    }
                    DBGFR3StackWalkEnd(pFirstFrame);
                }

                /* raw stack */
                Hlp.fRecSummary = false;
                pHlp->pfnPrintf(pHlp,
                                "!!\n"
                                "!! Raw stack (mind the direction). pbEMTStackRC=%RRv pbEMTStackBottomRC=%RRv\n"
                                "!!\n"
                                "%.*Rhxd\n",
                                pVCpu->vmm.s.pbEMTStackRC, pVCpu->vmm.s.pbEMTStackBottomRC,
                                VMM_STACK_SIZE, pVCpu->vmm.s.pbEMTStackR3);
            } /* !HMIsEnabled */
            break;
        }

        case VERR_IEM_INSTR_NOT_IMPLEMENTED:
        case VERR_IEM_ASPECT_NOT_IMPLEMENTED:
        case VERR_PATM_IPE_TRAP_IN_PATCH_CODE:
        case VERR_EM_GUEST_CPU_HANG:
        {
            DBGFR3Info(pVM->pUVM, "cpumguest", NULL, pHlp);
            DBGFR3Info(pVM->pUVM, "cpumguestinstr", NULL, pHlp);
            DBGFR3Info(pVM->pUVM, "cpumguesthwvirt", NULL, pHlp);
            break;
        }

        default:
        {
            break;
        }

    } /* switch (rcErr) */
    Hlp.fRecSummary = false;


    /*
     * Generic info dumper loop.
     */
    static struct
    {
        const char *pszInfo;
        const char *pszArgs;
    } const     aInfo[] =
    {
        { "mappings",        NULL },
        { "hma",             NULL },
        { "cpumguest",       "verbose" },
        { "cpumguesthwvirt", "verbose" },
        { "cpumguestinstr",  "verbose" },
        { "cpumhyper",       "verbose" },
        { "cpumhost",        "verbose" },
        { "mode",            "all" },
        { "cpuid",           "verbose" },
        { "handlers",        "phys virt hyper stats" },
        { "timers",          NULL },
        { "activetimers",    NULL },
    };
    for (unsigned i = 0; i < RT_ELEMENTS(aInfo); i++)
    {
        if (fDoneHyper && !strcmp(aInfo[i].pszInfo, "cpumhyper"))
            continue;
        pHlp->pfnPrintf(pHlp,
                        "!!\n"
                        "!! {%s, %s}\n"
                        "!!\n",
                        aInfo[i].pszInfo, aInfo[i].pszArgs);
        DBGFR3Info(pVM->pUVM, aInfo[i].pszInfo, aInfo[i].pszArgs, pHlp);
    }

    /* All other info items */
    DBGFR3InfoMulti(pVM,
                    "*",
                    "mappings|hma|cpum|cpumguest|cpumguesthwvirt|cpumguestinstr|cpumhyper|cpumhost|mode|cpuid"
                    "|pgmpd|pgmcr3|timers|activetimers|handlers|help",
                    "!!\n"
                    "!! {%s}\n"
                    "!!\n",
                    pHlp);


    /* done */
    pHlp->pfnPrintf(pHlp,
                    "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");


    /*
     * Repeat the summary to stderr so we don't have to scroll half a mile up.
     */
    if (Hlp.szSummary[0])
        RTStrmPrintf(g_pStdErr,
                     "%s"
                     "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",
                     Hlp.szSummary);

    /*
     * Delete the output instance (flushing and restoring of flags).
     */
    vmmR3FatalDumpInfoHlpDelete(&Hlp);
}

