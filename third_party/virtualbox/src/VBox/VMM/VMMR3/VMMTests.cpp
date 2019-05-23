/* $Id: VMMTests.cpp $ */
/** @file
 * VMM - The Virtual Machine Monitor Core, Tests.
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

//#define NO_SUPCALLR0VMM


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_VMM
#include <iprt/asm-amd64-x86.h> /* for SUPGetCpuHzFromGIP */
#include <VBox/vmm/vmm.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/vmm/cpum.h>
#include <VBox/dbg.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/trpm.h>
#include <VBox/vmm/selm.h>
#include "VMMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/err.h>
#include <VBox/param.h>

#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/time.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/x86.h>


#ifdef VBOX_WITH_RAW_MODE

static void vmmR3TestClearStack(PVMCPU pVCpu)
{
    /* We leave the first 64 bytes of the stack alone because of strict
       ring-0 long jump code uses it. */
    memset(pVCpu->vmm.s.pbEMTStackR3 + 64, 0xaa, VMM_STACK_SIZE - 64);
}


static int vmmR3ReportMsrRange(PVM pVM, uint32_t uMsr, uint64_t cMsrs, PRTSTREAM pReportStrm, uint32_t *pcMsrsFound)
{
    /*
     * Preps.
     */
    RTRCPTR RCPtrEP;
    int rc = PDMR3LdrGetSymbolRC(pVM, VMMRC_MAIN_MODULE_NAME, "VMMRCTestReadMsrs", &RCPtrEP);
    AssertMsgRCReturn(rc, ("Failed to resolved VMMRC.rc::VMMRCEntry(), rc=%Rrc\n", rc), rc);

    uint32_t const      cMsrsPerCall = 16384;
    uint32_t            cbResults = cMsrsPerCall * sizeof(VMMTESTMSRENTRY);
    PVMMTESTMSRENTRY    paResults;
    rc = MMHyperAlloc(pVM, cbResults, 0, MM_TAG_VMM, (void **)&paResults);
    AssertMsgRCReturn(rc, ("Error allocating %#x bytes off the hyper heap: %Rrc\n", cbResults, rc), rc);
    /*
     * The loop.
     */
    RTRCPTR  RCPtrResults = MMHyperR3ToRC(pVM, paResults);
    uint32_t cMsrsFound   = 0;
    uint32_t uLastMsr     = uMsr;
    uint64_t uNsTsStart   = RTTimeNanoTS();

    for (;;)
    {
        if (   pReportStrm
            && uMsr - uLastMsr > _64K
            && (uMsr & (_4M - 1)) == 0)
        {
            if (uMsr - uLastMsr < 16U*_1M)
                RTStrmFlush(pReportStrm);
            RTPrintf("... %#010x [%u ns/msr] ...\n", uMsr, (RTTimeNanoTS() - uNsTsStart) / uMsr);
        }

        /*RT_BZERO(paResults, cbResults);*/
        uint32_t const cBatch = RT_MIN(cMsrsPerCall, cMsrs);
        rc = VMMR3CallRC(pVM, RCPtrEP, 4, pVM->pVMRC, uMsr, cBatch, RCPtrResults);
        if (RT_FAILURE(rc))
        {
            RTPrintf("VMM: VMMR3CallRC failed rc=%Rrc, uMsr=%#x\n", rc, uMsr);
            break;
        }

        for (uint32_t i = 0; i < cBatch; i++)
            if (paResults[i].uMsr != UINT64_MAX)
            {
                if (paResults[i].uValue == 0)
                {
                    if (pReportStrm)
                        RTStrmPrintf(pReportStrm,
                                     "    MVO(%#010llx, \"MSR\", UINT64_C(%#018llx)),\n", paResults[i].uMsr, paResults[i].uValue);
                    RTPrintf("%#010llx = 0\n", paResults[i].uMsr);
                }
                else
                {
                    if (pReportStrm)
                        RTStrmPrintf(pReportStrm,
                                     "    MVO(%#010llx, \"MSR\", UINT64_C(%#018llx)),\n", paResults[i].uMsr, paResults[i].uValue);
                    RTPrintf("%#010llx = %#010x`%08x\n", paResults[i].uMsr,
                             RT_HI_U32(paResults[i].uValue), RT_LO_U32(paResults[i].uValue));
                }
                cMsrsFound++;
                uLastMsr = paResults[i].uMsr;
            }

        /* Advance. */
        if (cMsrs <= cMsrsPerCall)
            break;
        cMsrs -= cMsrsPerCall;
        uMsr  += cMsrsPerCall;
    }

    *pcMsrsFound += cMsrsFound;
    MMHyperFree(pVM, paResults);
    return rc;
}


/**
 * Produces a quick report of MSRs.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pReportStrm     Pointer to the report output stream. Optional.
 * @param   fWithCpuId      Whether CPUID should be included.
 */
static int vmmR3DoMsrQuickReport(PVM pVM, PRTSTREAM pReportStrm, bool fWithCpuId)
{
    uint64_t uTsStart = RTTimeNanoTS();
    RTPrintf("=== MSR Quick Report Start ===\n");
    RTStrmFlush(g_pStdOut);
    if (fWithCpuId)
    {
        DBGFR3InfoStdErr(pVM->pUVM, "cpuid", "verbose");
        RTPrintf("\n");
    }
    if (pReportStrm)
        RTStrmPrintf(pReportStrm, "\n\n{\n");

    static struct { uint32_t uFirst, cMsrs; } const s_aRanges[] =
    {
        { 0x00000000, 0x00042000 },
        { 0x10000000, 0x00001000 },
        { 0x20000000, 0x00001000 },
        { 0x40000000, 0x00012000 },
        { 0x80000000, 0x00012000 },
//   Need 0xc0000000..0xc001106f (at least), but trouble on solaris w/ 10h and 0fh family cpus:
//      { 0xc0000000, 0x00022000 },
        { 0xc0000000, 0x00010000 },
        { 0xc0010000, 0x00001040 },
        { 0xc0011040, 0x00004040 }, /* should cause trouble... */
    };
    uint32_t cMsrsFound = 0;
    int rc = VINF_SUCCESS;
    for (unsigned i = 0; i < RT_ELEMENTS(s_aRanges) && RT_SUCCESS(rc); i++)
    {
//if (i >= 3)
//{
//RTStrmFlush(g_pStdOut);
//RTThreadSleep(40);
//}
        rc = vmmR3ReportMsrRange(pVM, s_aRanges[i].uFirst, s_aRanges[i].cMsrs, pReportStrm, &cMsrsFound);
    }

    if (pReportStrm)
        RTStrmPrintf(pReportStrm, "}; /* %u (%#x) MSRs; rc=%Rrc */\n", cMsrsFound, cMsrsFound, rc);
    RTPrintf("Total %u (%#x) MSRs\n", cMsrsFound, cMsrsFound);
    RTPrintf("=== MSR Quick Report End (rc=%Rrc, %'llu ns) ===\n", rc, RTTimeNanoTS() - uTsStart);
    return rc;
}


/**
 * Performs a testcase.
 *
 * @returns return value from the test.
 * @param   pVM         The cross context VM structure.
 * @param   enmTestcase The testcase operation to perform.
 * @param   uVariation  The testcase variation id.
 */
static int vmmR3DoGCTest(PVM pVM, VMMRCOPERATION enmTestcase, unsigned uVariation)
{
    PVMCPU pVCpu = &pVM->aCpus[0];

    RTRCPTR RCPtrEP;
    int rc = PDMR3LdrGetSymbolRC(pVM, VMMRC_MAIN_MODULE_NAME, "VMMRCEntry", &RCPtrEP);
    if (RT_FAILURE(rc))
        return rc;

    Log(("vmmR3DoGCTest: %d %#x\n", enmTestcase, uVariation));
    CPUMSetHyperState(pVCpu, pVM->vmm.s.pfnCallTrampolineRC, pVCpu->vmm.s.pbEMTStackBottomRC, 0, 0);
    vmmR3TestClearStack(pVCpu);
    CPUMPushHyper(pVCpu, uVariation);
    CPUMPushHyper(pVCpu, enmTestcase);
    CPUMPushHyper(pVCpu, pVM->pVMRC);
    CPUMPushHyper(pVCpu, 3 * sizeof(RTRCPTR));    /* stack frame size */
    CPUMPushHyper(pVCpu, RCPtrEP);                /* what to call */
    Assert(CPUMGetHyperCR3(pVCpu) && CPUMGetHyperCR3(pVCpu) == PGMGetHyperCR3(pVCpu));
    rc = SUPR3CallVMMR0Fast(pVM->pVMR0, VMMR0_DO_RAW_RUN, 0);

# if 1
    /* flush the raw-mode logs. */
#  ifdef LOG_ENABLED
    PRTLOGGERRC pLogger = pVM->vmm.s.pRCLoggerR3;
    if (   pLogger
        && pLogger->offScratch > 0)
        RTLogFlushRC(NULL, pLogger);
#  endif
#  ifdef VBOX_WITH_RC_RELEASE_LOGGING
    PRTLOGGERRC pRelLogger = pVM->vmm.s.pRCRelLoggerR3;
    if (RT_UNLIKELY(pRelLogger && pRelLogger->offScratch > 0))
        RTLogFlushRC(RTLogRelGetDefaultInstance(), pRelLogger);
#  endif
# endif

    Log(("vmmR3DoGCTest: rc=%Rrc iLastGZRc=%Rrc\n", rc, pVCpu->vmm.s.iLastGZRc));
    if (RT_LIKELY(rc == VINF_SUCCESS))
        rc = pVCpu->vmm.s.iLastGZRc;
    return rc;
}


/**
 * Performs a trap test.
 *
 * @returns Return value from the trap test.
 * @param   pVM         The cross context VM structure.
 * @param   u8Trap      The trap number to test.
 * @param   uVariation  The testcase variation.
 * @param   rcExpect    The expected result.
 * @param   u32Eax      The expected eax value.
 * @param   pszFaultEIP The fault address. Pass NULL if this isn't available or doesn't apply.
 * @param   pszDesc     The test description.
 */
static int vmmR3DoTrapTest(PVM pVM, uint8_t u8Trap, unsigned uVariation, int rcExpect, uint32_t u32Eax, const char *pszFaultEIP, const char *pszDesc)
{
    PVMCPU pVCpu = &pVM->aCpus[0];

    RTPrintf("VMM: testing 0%x / %d - %s\n", u8Trap, uVariation, pszDesc);

    RTRCPTR RCPtrEP;
    int rc = PDMR3LdrGetSymbolRC(pVM, VMMRC_MAIN_MODULE_NAME, "VMMRCEntry", &RCPtrEP);
    if (RT_FAILURE(rc))
        return rc;

    CPUMSetHyperState(pVCpu, pVM->vmm.s.pfnCallTrampolineRC, pVCpu->vmm.s.pbEMTStackBottomRC, 0, 0);
    vmmR3TestClearStack(pVCpu);
    CPUMPushHyper(pVCpu, uVariation);
    CPUMPushHyper(pVCpu, u8Trap + VMMRC_DO_TESTCASE_TRAP_FIRST);
    CPUMPushHyper(pVCpu, pVM->pVMRC);
    CPUMPushHyper(pVCpu, 3 * sizeof(RTRCPTR));    /* stack frame size */
    CPUMPushHyper(pVCpu, RCPtrEP);                /* what to call */
    Assert(CPUMGetHyperCR3(pVCpu) && CPUMGetHyperCR3(pVCpu) == PGMGetHyperCR3(pVCpu));
    rc = SUPR3CallVMMR0Fast(pVM->pVMR0, VMMR0_DO_RAW_RUN, 0);
    if (RT_LIKELY(rc == VINF_SUCCESS))
        rc = pVCpu->vmm.s.iLastGZRc;
    bool fDump = false;
    if (rc != rcExpect)
    {
        RTPrintf("VMM: FAILURE - rc=%Rrc expected %Rrc\n", rc, rcExpect);
        if (rc != VERR_NOT_IMPLEMENTED)
            fDump = true;
    }
    else if (    rcExpect != VINF_SUCCESS
             &&  u8Trap != 8 /* double fault doesn't dare set TrapNo. */
             &&  u8Trap != 3 /* guest only, we're not in guest. */
             &&  u8Trap != 1 /* guest only, we're not in guest. */
             &&  u8Trap != TRPMGetTrapNo(pVCpu))
    {
        RTPrintf("VMM: FAILURE - Trap %#x expected %#x\n", TRPMGetTrapNo(pVCpu), u8Trap);
        fDump = true;
    }
    else if (pszFaultEIP)
    {
        RTRCPTR RCPtrFault;
        int rc2 = PDMR3LdrGetSymbolRC(pVM, VMMRC_MAIN_MODULE_NAME, pszFaultEIP, &RCPtrFault);
        if (RT_FAILURE(rc2))
            RTPrintf("VMM: FAILURE - Failed to resolve symbol '%s', %Rrc!\n", pszFaultEIP, rc);
        else if (RCPtrFault != CPUMGetHyperEIP(pVCpu))
        {
            RTPrintf("VMM: FAILURE - EIP=%08RX32 expected %RRv (%s)\n", CPUMGetHyperEIP(pVCpu), RCPtrFault, pszFaultEIP);
            fDump = true;
        }
    }
    else if (rcExpect != VINF_SUCCESS)
    {
        if (CPUMGetHyperSS(pVCpu) == SELMGetHyperDS(pVM))
            RTPrintf("VMM: FAILURE - ss=%x expected %x\n", CPUMGetHyperSS(pVCpu), SELMGetHyperDS(pVM));
        if (CPUMGetHyperES(pVCpu) == SELMGetHyperDS(pVM))
            RTPrintf("VMM: FAILURE - es=%x expected %x\n", CPUMGetHyperES(pVCpu), SELMGetHyperDS(pVM));
        if (CPUMGetHyperDS(pVCpu) == SELMGetHyperDS(pVM))
            RTPrintf("VMM: FAILURE - ds=%x expected %x\n", CPUMGetHyperDS(pVCpu), SELMGetHyperDS(pVM));
        if (CPUMGetHyperFS(pVCpu) == SELMGetHyperDS(pVM))
            RTPrintf("VMM: FAILURE - fs=%x expected %x\n", CPUMGetHyperFS(pVCpu), SELMGetHyperDS(pVM));
        if (CPUMGetHyperGS(pVCpu) == SELMGetHyperDS(pVM))
            RTPrintf("VMM: FAILURE - gs=%x expected %x\n", CPUMGetHyperGS(pVCpu), SELMGetHyperDS(pVM));
        if (CPUMGetHyperEDI(pVCpu) == 0x01234567)
            RTPrintf("VMM: FAILURE - edi=%x expected %x\n", CPUMGetHyperEDI(pVCpu), 0x01234567);
        if (CPUMGetHyperESI(pVCpu) == 0x42000042)
            RTPrintf("VMM: FAILURE - esi=%x expected %x\n", CPUMGetHyperESI(pVCpu), 0x42000042);
        if (CPUMGetHyperEBP(pVCpu) == 0xffeeddcc)
            RTPrintf("VMM: FAILURE - ebp=%x expected %x\n", CPUMGetHyperEBP(pVCpu), 0xffeeddcc);
        if (CPUMGetHyperEBX(pVCpu) == 0x89abcdef)
            RTPrintf("VMM: FAILURE - ebx=%x expected %x\n", CPUMGetHyperEBX(pVCpu), 0x89abcdef);
        if (CPUMGetHyperECX(pVCpu) == 0xffffaaaa)
            RTPrintf("VMM: FAILURE - ecx=%x expected %x\n", CPUMGetHyperECX(pVCpu), 0xffffaaaa);
        if (CPUMGetHyperEDX(pVCpu) == 0x77778888)
            RTPrintf("VMM: FAILURE - edx=%x expected %x\n", CPUMGetHyperEDX(pVCpu), 0x77778888);
        if (CPUMGetHyperEAX(pVCpu) == u32Eax)
            RTPrintf("VMM: FAILURE - eax=%x expected %x\n", CPUMGetHyperEAX(pVCpu), u32Eax);
    }
    if (fDump)
        VMMR3FatalDump(pVM, pVCpu, rc);
    return rc;
}

#endif /* VBOX_WITH_RAW_MODE */


/* execute the switch. */
VMMR3DECL(int) VMMDoTest(PVM pVM)
{
    int rc = VINF_SUCCESS;

#ifdef VBOX_WITH_RAW_MODE
    PVMCPU pVCpu = &pVM->aCpus[0];
    PUVM   pUVM  = pVM->pUVM;

# ifdef NO_SUPCALLR0VMM
    RTPrintf("NO_SUPCALLR0VMM\n");
    return rc;
# endif

    /*
     * Setup stack for calling VMMRCEntry().
     */
    RTRCPTR RCPtrEP;
    rc = PDMR3LdrGetSymbolRC(pVM, VMMRC_MAIN_MODULE_NAME, "VMMRCEntry", &RCPtrEP);
    if (RT_SUCCESS(rc))
    {
        RTPrintf("VMM: VMMRCEntry=%RRv\n", RCPtrEP);

        /*
         * Test various crashes which we must be able to recover from.
         */
        vmmR3DoTrapTest(pVM, 0x3, 0, VINF_EM_DBG_HYPER_ASSERTION,  0xf0f0f0f0, "vmmGCTestTrap3_FaultEIP", "int3");
        vmmR3DoTrapTest(pVM, 0x3, 1, VINF_EM_DBG_HYPER_ASSERTION,  0xf0f0f0f0, "vmmGCTestTrap3_FaultEIP", "int3 WP");

# if 0//defined(DEBUG_bird) /* guess most people would like to skip these since they write to com1. */
        vmmR3DoTrapTest(pVM, 0x8, 0, VERR_TRPM_PANIC,       0x00000000, "vmmGCTestTrap8_FaultEIP", "#DF [#PG]");
        SELMR3Relocate(pVM); /* this resets the busy flag of the Trap 08 TSS */
        bool f;
        rc = CFGMR3QueryBool(CFGMR3GetRoot(pVM), "DoubleFault", &f);
# if !defined(DEBUG_bird)
        if (RT_SUCCESS(rc) && f)
# endif
        {
            /* see triple fault warnings in SELM and VMMRC.cpp. */
            vmmR3DoTrapTest(pVM, 0x8, 1, VERR_TRPM_PANIC,       0x00000000, "vmmGCTestTrap8_FaultEIP", "#DF [#PG] WP");
            SELMR3Relocate(pVM); /* this resets the busy flag of the Trap 08 TSS */
        }
# endif

        vmmR3DoTrapTest(pVM, 0xd, 0, VERR_TRPM_DONT_PANIC,  0xf0f0f0f0, "vmmGCTestTrap0d_FaultEIP", "ltr #GP");
        /// @todo find a better \#GP case, on intel ltr will \#PF (busy update?) and not \#GP.
        //vmmR3DoTrapTest(pVM, 0xd, 1, VERR_TRPM_DONT_PANIC,  0xf0f0f0f0, "vmmGCTestTrap0d_FaultEIP", "ltr #GP WP");

        vmmR3DoTrapTest(pVM, 0xe, 0, VERR_TRPM_DONT_PANIC,  0x00000000, "vmmGCTestTrap0e_FaultEIP", "#PF (NULL)");
        vmmR3DoTrapTest(pVM, 0xe, 1, VERR_TRPM_DONT_PANIC,  0x00000000, "vmmGCTestTrap0e_FaultEIP", "#PF (NULL) WP");
        vmmR3DoTrapTest(pVM, 0xe, 2, VINF_SUCCESS,          0x00000000, NULL,                       "#PF w/Tmp Handler");
        /* This test is no longer relevant as fs and gs are loaded with NULL
           selectors and we will always return to HC if a #GP occurs while
           returning to guest code.
        vmmR3DoTrapTest(pVM, 0xe, 4, VINF_SUCCESS,          0x00000000, NULL,                       "#PF w/Tmp Handler and bad fs");
        */

        /*
         * Set a debug register and perform a context switch.
         */
        rc = vmmR3DoGCTest(pVM, VMMRC_DO_TESTCASE_NOP, 0);
        if (rc != VINF_SUCCESS)
        {
            RTPrintf("VMM: Nop test failed, rc=%Rrc not VINF_SUCCESS\n", rc);
            return RT_FAILURE(rc) ? rc : VERR_IPE_UNEXPECTED_INFO_STATUS;
        }

        /* a harmless breakpoint */
        RTPrintf("VMM: testing hardware bp at 0x10000 (not hit)\n");
        DBGFADDRESS Addr;
        DBGFR3AddrFromFlat(pUVM, &Addr, 0x10000);
        RTUINT iBp0;
        rc = DBGFR3BpSetReg(pUVM, &Addr, 0,  ~(uint64_t)0, X86_DR7_RW_EO, 1, &iBp0);
        AssertReleaseRC(rc);
        rc = vmmR3DoGCTest(pVM, VMMRC_DO_TESTCASE_NOP, 0);
        if (rc != VINF_SUCCESS)
        {
            RTPrintf("VMM: DR0=0x10000 test failed with rc=%Rrc!\n", rc);
            return RT_FAILURE(rc) ? rc : VERR_IPE_UNEXPECTED_INFO_STATUS;
        }

        /* a bad one at VMMRCEntry */
        RTPrintf("VMM: testing hardware bp at VMMRCEntry (hit)\n");
        DBGFR3AddrFromFlat(pUVM, &Addr, RCPtrEP);
        RTUINT iBp1;
        rc = DBGFR3BpSetReg(pUVM, &Addr, 0,  ~(uint64_t)0, X86_DR7_RW_EO, 1, &iBp1);
        AssertReleaseRC(rc);
        rc = vmmR3DoGCTest(pVM, VMMRC_DO_TESTCASE_NOP, 0);
        if (rc != VINF_EM_DBG_HYPER_BREAKPOINT)
        {
            RTPrintf("VMM: DR1=VMMRCEntry test failed with rc=%Rrc! expected VINF_EM_RAW_BREAKPOINT_HYPER\n", rc);
            return RT_FAILURE(rc) ? rc : VERR_IPE_UNEXPECTED_INFO_STATUS;
        }

        /* resume the breakpoint */
        RTPrintf("VMM: resuming hyper after breakpoint\n");
        CPUMSetHyperEFlags(pVCpu, CPUMGetHyperEFlags(pVCpu) | X86_EFL_RF);
        rc = VMMR3ResumeHyper(pVM, pVCpu);
        if (rc != VINF_SUCCESS)
        {
            RTPrintf("VMM: failed to resume on hyper breakpoint, rc=%Rrc = KNOWN BUG\n", rc); /** @todo fix VMMR3ResumeHyper */
            return RT_FAILURE(rc) ? rc : VERR_IPE_UNEXPECTED_INFO_STATUS;
        }

        /* engage the breakpoint again and try single stepping. */
        RTPrintf("VMM: testing hardware bp at VMMRCEntry + stepping\n");
        rc = vmmR3DoGCTest(pVM, VMMRC_DO_TESTCASE_NOP, 0);
        if (rc != VINF_EM_DBG_HYPER_BREAKPOINT)
        {
            RTPrintf("VMM: DR1=VMMRCEntry test failed with rc=%Rrc! expected VINF_EM_RAW_BREAKPOINT_HYPER\n", rc);
            return RT_FAILURE(rc) ? rc : VERR_IPE_UNEXPECTED_INFO_STATUS;
        }

        RTGCUINTREG OldPc = CPUMGetHyperEIP(pVCpu);
        RTPrintf("%RGr=>", OldPc);
        unsigned i;
        for (i = 0; i < 8; i++)
        {
            CPUMSetHyperEFlags(pVCpu, CPUMGetHyperEFlags(pVCpu) | X86_EFL_TF | X86_EFL_RF);
            rc = VMMR3ResumeHyper(pVM, pVCpu);
            if (rc != VINF_EM_DBG_HYPER_STEPPED)
            {
                RTPrintf("\nVMM: failed to step on hyper breakpoint, rc=%Rrc\n", rc);
                return RT_FAILURE(rc) ? rc : VERR_IPE_UNEXPECTED_INFO_STATUS;
            }
            RTGCUINTREG Pc = CPUMGetHyperEIP(pVCpu);
            RTPrintf("%RGr=>", Pc);
            if (Pc == OldPc)
            {
                RTPrintf("\nVMM: step failed, PC: %RGr -> %RGr\n", OldPc, Pc);
                return VERR_GENERAL_FAILURE;
            }
            OldPc = Pc;
        }
        RTPrintf("ok\n");

        /* done, clear it */
        if (    RT_FAILURE(DBGFR3BpClear(pUVM, iBp0))
            ||  RT_FAILURE(DBGFR3BpClear(pUVM, iBp1)))
        {
            RTPrintf("VMM: Failed to clear breakpoints!\n");
            return VERR_GENERAL_FAILURE;
        }
        rc = vmmR3DoGCTest(pVM, VMMRC_DO_TESTCASE_NOP, 0);
        if (rc != VINF_SUCCESS)
        {
            RTPrintf("VMM: NOP failed, rc=%Rrc\n", rc);
            return RT_FAILURE(rc) ? rc : VERR_IPE_UNEXPECTED_INFO_STATUS;
        }

        /*
         * Interrupt masking.  Failure may indiate NMI watchdog activity.
         */
        RTPrintf("VMM: interrupt masking...\n"); RTStrmFlush(g_pStdOut); RTThreadSleep(250);
        for (i = 0; i < 10000; i++)
        {
            uint64_t StartTick = ASMReadTSC();
            rc = vmmR3DoGCTest(pVM, VMMRC_DO_TESTCASE_INTERRUPT_MASKING, 0);
            if (rc != VINF_SUCCESS)
            {
                RTPrintf("VMM: Interrupt masking failed: rc=%Rrc\n", rc);
                return RT_FAILURE(rc) ? rc : VERR_IPE_UNEXPECTED_INFO_STATUS;
            }
            uint64_t Ticks = ASMReadTSC() - StartTick;
            if (Ticks < (SUPGetCpuHzFromGip(g_pSUPGlobalInfoPage) / 10000))
                RTPrintf("Warning: Ticks=%RU64 (< %RU64)\n", Ticks, SUPGetCpuHzFromGip(g_pSUPGlobalInfoPage) / 10000);
        }

        /*
         * Interrupt forwarding.
         */
        CPUMSetHyperState(pVCpu, pVM->vmm.s.pfnCallTrampolineRC, pVCpu->vmm.s.pbEMTStackBottomRC, 0, 0);
        CPUMPushHyper(pVCpu, 0);
        CPUMPushHyper(pVCpu, VMMRC_DO_TESTCASE_HYPER_INTERRUPT);
        CPUMPushHyper(pVCpu, pVM->pVMRC);
        CPUMPushHyper(pVCpu, 3 * sizeof(RTRCPTR));    /* stack frame size */
        CPUMPushHyper(pVCpu, RCPtrEP);                /* what to call */
        Log(("trampoline=%x\n", pVM->vmm.s.pfnCallTrampolineRC));

        /*
         * Switch and do da thing.
         */
        RTPrintf("VMM: interrupt forwarding...\n"); RTStrmFlush(g_pStdOut); RTThreadSleep(250);
        i = 0;
        uint64_t    tsBegin = RTTimeNanoTS();
        uint64_t    TickStart = ASMReadTSC();
        Assert(CPUMGetHyperCR3(pVCpu) && CPUMGetHyperCR3(pVCpu) == PGMGetHyperCR3(pVCpu));
        do
        {
            rc = SUPR3CallVMMR0Fast(pVM->pVMR0, VMMR0_DO_RAW_RUN, 0);
            if (RT_LIKELY(rc == VINF_SUCCESS))
                rc = pVCpu->vmm.s.iLastGZRc;
            if (RT_FAILURE(rc))
            {
                Log(("VMM: GC returned fatal %Rra in iteration %d\n", rc, i));
                VMMR3FatalDump(pVM, pVCpu, rc);
                return rc;
            }
            i++;
            if (!(i % 32))
                Log(("VMM: iteration %d, esi=%08x edi=%08x ebx=%08x\n",
                       i, CPUMGetHyperESI(pVCpu), CPUMGetHyperEDI(pVCpu), CPUMGetHyperEBX(pVCpu)));
        } while (rc == VINF_EM_RAW_INTERRUPT_HYPER);
        uint64_t    TickEnd = ASMReadTSC();
        uint64_t    tsEnd = RTTimeNanoTS();

        uint64_t    Elapsed = tsEnd - tsBegin;
        uint64_t    PerIteration = Elapsed / (uint64_t)i;
        uint64_t    cTicksElapsed = TickEnd - TickStart;
        uint64_t    cTicksPerIteration = cTicksElapsed / (uint64_t)i;

        RTPrintf("VMM: %8d interrupts in %11llu ns (%11llu ticks),  %10llu ns/iteration (%11llu ticks)\n",
                 i, Elapsed, cTicksElapsed, PerIteration, cTicksPerIteration);
        Log(("VMM: %8d interrupts in %11llu ns (%11llu ticks),  %10llu ns/iteration (%11llu ticks)\n",
             i, Elapsed, cTicksElapsed, PerIteration, cTicksPerIteration));

        /*
         * These forced actions are not necessary for the test and trigger breakpoints too.
         */
        VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_TRPM_SYNC_IDT);
        VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_SELM_SYNC_TSS);

        /*
         * Profile switching.
         */
        RTPrintf("VMM: profiling switcher...\n");
        Log(("VMM: profiling switcher...\n"));
        uint64_t TickMin = UINT64_MAX;
        tsBegin = RTTimeNanoTS();
        TickStart = ASMReadTSC();
        Assert(CPUMGetHyperCR3(pVCpu) && CPUMGetHyperCR3(pVCpu) == PGMGetHyperCR3(pVCpu));
        for (i = 0; i < 1000000; i++)
        {
            CPUMSetHyperState(pVCpu, pVM->vmm.s.pfnCallTrampolineRC, pVCpu->vmm.s.pbEMTStackBottomRC, 0, 0);
            CPUMPushHyper(pVCpu, 0);
            CPUMPushHyper(pVCpu, VMMRC_DO_TESTCASE_NOP);
            CPUMPushHyper(pVCpu, pVM->pVMRC);
            CPUMPushHyper(pVCpu, 3 * sizeof(RTRCPTR));    /* stack frame size */
            CPUMPushHyper(pVCpu, RCPtrEP);                /* what to call */

            uint64_t TickThisStart = ASMReadTSC();
            rc = SUPR3CallVMMR0Fast(pVM->pVMR0, VMMR0_DO_RAW_RUN, 0);
            if (RT_LIKELY(rc == VINF_SUCCESS))
                rc = pVCpu->vmm.s.iLastGZRc;
            uint64_t TickThisElapsed = ASMReadTSC() - TickThisStart;
            if (RT_FAILURE(rc))
            {
                Log(("VMM: GC returned fatal %Rra in iteration %d\n", rc, i));
                VMMR3FatalDump(pVM, pVCpu, rc);
                return rc;
            }
            if (TickThisElapsed < TickMin)
                TickMin = TickThisElapsed;
        }
        TickEnd = ASMReadTSC();
        tsEnd = RTTimeNanoTS();

        Elapsed = tsEnd - tsBegin;
        PerIteration = Elapsed / (uint64_t)i;
        cTicksElapsed = TickEnd - TickStart;
        cTicksPerIteration = cTicksElapsed / (uint64_t)i;

        RTPrintf("VMM: %8d cycles     in %11llu ns (%11lld ticks),  %10llu ns/iteration (%11lld ticks)  Min %11lld ticks\n",
                 i, Elapsed, cTicksElapsed, PerIteration, cTicksPerIteration, TickMin);
        Log(("VMM: %8d cycles     in %11llu ns (%11lld ticks),  %10llu ns/iteration (%11lld ticks)  Min %11lld ticks\n",
             i, Elapsed, cTicksElapsed, PerIteration, cTicksPerIteration, TickMin));

        rc = VINF_SUCCESS;

# if 0  /* drop this for now as it causes trouble on AMDs (Opteron 2384 and possibly others). */
        /*
         * A quick MSR report.
         */
        vmmR3DoMsrQuickReport(pVM, NULL, true);
# endif
    }
    else
        AssertMsgFailed(("Failed to resolved VMMRC.rc::VMMRCEntry(), rc=%Rrc\n", rc));
#else  /* !VBOX_WITH_RAW_MODE */
    RT_NOREF(pVM);
#endif /* !VBOX_WITH_RAW_MODE */
    return rc;
}

#define SYNC_SEL(pHyperCtx, reg)                                                        \
        if (pHyperCtx->reg.Sel)                                                         \
        {                                                                               \
            DBGFSELINFO selInfo;                                                        \
            int rc2 = SELMR3GetShadowSelectorInfo(pVM, pHyperCtx->reg.Sel, &selInfo);   \
            AssertRC(rc2);                                                              \
                                                                                        \
            pHyperCtx->reg.u64Base              = selInfo.GCPtrBase;                    \
            pHyperCtx->reg.u32Limit             = selInfo.cbLimit;                      \
            pHyperCtx->reg.Attr.n.u1Present     = selInfo.u.Raw.Gen.u1Present;          \
            pHyperCtx->reg.Attr.n.u1DefBig      = selInfo.u.Raw.Gen.u1DefBig;           \
            pHyperCtx->reg.Attr.n.u1Granularity = selInfo.u.Raw.Gen.u1Granularity;      \
            pHyperCtx->reg.Attr.n.u4Type        = selInfo.u.Raw.Gen.u4Type;             \
            pHyperCtx->reg.Attr.n.u2Dpl         = selInfo.u.Raw.Gen.u2Dpl;              \
            pHyperCtx->reg.Attr.n.u1DescType    = selInfo.u.Raw.Gen.u1DescType;         \
            pHyperCtx->reg.Attr.n.u1Long        = selInfo.u.Raw.Gen.u1Long;             \
        }

/* execute the switch. */
VMMR3DECL(int) VMMDoHmTest(PVM pVM)
{
    uint32_t i;
    int      rc;
    PCPUMCTX pHyperCtx, pGuestCtx;
    RTGCPHYS CR3Phys = 0x0; /* fake address */
    PVMCPU   pVCpu = &pVM->aCpus[0];

    if (!HMIsEnabled(pVM))
    {
        RTPrintf("VMM: Hardware accelerated test not available!\n");
        return VERR_ACCESS_DENIED;
    }

#ifdef VBOX_WITH_RAW_MODE
    /*
     * These forced actions are not necessary for the test and trigger breakpoints too.
     */
    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_TRPM_SYNC_IDT);
    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_SELM_SYNC_TSS);
#endif

    /* Enable mapping of the hypervisor into the shadow page table. */
    uint32_t cb;
    rc = PGMR3MappingsSize(pVM, &cb);
    AssertRCReturn(rc, rc);

    /* Pretend the mappings are now fixed; to force a refresh of the reserved PDEs. */
    rc = PGMR3MappingsFix(pVM, MM_HYPER_AREA_ADDRESS, cb);
    AssertRCReturn(rc, rc);

    pHyperCtx = CPUMGetHyperCtxPtr(pVCpu);

    pHyperCtx->cr0 = X86_CR0_PE | X86_CR0_WP | X86_CR0_PG | X86_CR0_TS | X86_CR0_ET | X86_CR0_NE | X86_CR0_MP;
    pHyperCtx->cr4 = X86_CR4_PGE | X86_CR4_OSFXSR | X86_CR4_OSXMMEEXCPT;
    PGMChangeMode(pVCpu, pHyperCtx->cr0, pHyperCtx->cr4, pHyperCtx->msrEFER);
    PGMSyncCR3(pVCpu, pHyperCtx->cr0, CR3Phys, pHyperCtx->cr4, true);

    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_TO_R3);
    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_TIMER);
    VM_FF_CLEAR(pVM, VM_FF_TM_VIRTUAL_SYNC);
    VM_FF_CLEAR(pVM, VM_FF_REQUEST);

    /*
     * Setup stack for calling VMMRCEntry().
     */
    RTRCPTR RCPtrEP;
    rc = PDMR3LdrGetSymbolRC(pVM, VMMRC_MAIN_MODULE_NAME, "VMMRCEntry", &RCPtrEP);
    if (RT_SUCCESS(rc))
    {
        RTPrintf("VMM: VMMRCEntry=%RRv\n", RCPtrEP);

        pHyperCtx = CPUMGetHyperCtxPtr(pVCpu);

        /* Fill in hidden selector registers for the hypervisor state. */
        SYNC_SEL(pHyperCtx, cs);
        SYNC_SEL(pHyperCtx, ds);
        SYNC_SEL(pHyperCtx, es);
        SYNC_SEL(pHyperCtx, fs);
        SYNC_SEL(pHyperCtx, gs);
        SYNC_SEL(pHyperCtx, ss);
        SYNC_SEL(pHyperCtx, tr);

        /*
         * Profile switching.
         */
        RTPrintf("VMM: profiling switcher...\n");
        Log(("VMM: profiling switcher...\n"));
        uint64_t TickMin = UINT64_MAX;
        uint64_t tsBegin = RTTimeNanoTS();
        uint64_t TickStart = ASMReadTSC();
        for (i = 0; i < 1000000; i++)
        {
            CPUMSetHyperState(pVCpu, pVM->vmm.s.pfnCallTrampolineRC, pVCpu->vmm.s.pbEMTStackBottomRC, 0, 0);
            CPUMPushHyper(pVCpu, 0);
            CPUMPushHyper(pVCpu, VMMRC_DO_TESTCASE_HM_NOP);
            CPUMPushHyper(pVCpu, pVM->pVMRC);
            CPUMPushHyper(pVCpu, 3 * sizeof(RTRCPTR));    /* stack frame size */
            CPUMPushHyper(pVCpu, RCPtrEP);                /* what to call */

            pHyperCtx = CPUMGetHyperCtxPtr(pVCpu);
            pGuestCtx = CPUMQueryGuestCtxPtr(pVCpu);

            /* Copy the hypervisor context to make sure we have a valid guest context. */
            *pGuestCtx = *pHyperCtx;
            pGuestCtx->cr3 = CR3Phys;

            VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_TO_R3);
            VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_TIMER);
            VM_FF_CLEAR(pVM, VM_FF_TM_VIRTUAL_SYNC);

            uint64_t TickThisStart = ASMReadTSC();
            rc = SUPR3CallVMMR0Fast(pVM->pVMR0, VMMR0_DO_HM_RUN, 0);
            uint64_t TickThisElapsed = ASMReadTSC() - TickThisStart;
            if (RT_FAILURE(rc))
            {
                Log(("VMM: R0 returned fatal %Rrc in iteration %d\n", rc, i));
                VMMR3FatalDump(pVM, pVCpu, rc);
                return rc;
            }
            if (TickThisElapsed < TickMin)
                TickMin = TickThisElapsed;
        }
        uint64_t TickEnd = ASMReadTSC();
        uint64_t tsEnd = RTTimeNanoTS();

        uint64_t Elapsed = tsEnd - tsBegin;
        uint64_t PerIteration = Elapsed / (uint64_t)i;
        uint64_t cTicksElapsed = TickEnd - TickStart;
        uint64_t cTicksPerIteration = cTicksElapsed / (uint64_t)i;

        RTPrintf("VMM: %8d cycles     in %11llu ns (%11lld ticks),  %10llu ns/iteration (%11lld ticks)  Min %11lld ticks\n",
                 i, Elapsed, cTicksElapsed, PerIteration, cTicksPerIteration, TickMin);
        Log(("VMM: %8d cycles     in %11llu ns (%11lld ticks),  %10llu ns/iteration (%11lld ticks)  Min %11lld ticks\n",
             i, Elapsed, cTicksElapsed, PerIteration, cTicksPerIteration, TickMin));

        rc = VINF_SUCCESS;
    }
    else
        AssertMsgFailed(("Failed to resolved VMMRC.rc::VMMRCEntry(), rc=%Rrc\n", rc));

    return rc;
}


#ifdef VBOX_WITH_RAW_MODE

/**
 * Used by VMMDoBruteForceMsrs to dump the CPUID info of the host CPU as a
 * prefix to the MSR report.
 */
static DECLCALLBACK(void) vmmDoPrintfVToStream(PCDBGFINFOHLP pHlp, const char *pszFormat, va_list va)
{
    PRTSTREAM pOutStrm = ((PRTSTREAM *)pHlp)[-1];
    RTStrmPrintfV(pOutStrm, pszFormat, va);
}

/**
 * Used by VMMDoBruteForceMsrs to dump the CPUID info of the host CPU as a
 * prefix to the MSR report.
 */
static DECLCALLBACK(void) vmmDoPrintfToStream(PCDBGFINFOHLP pHlp, const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    vmmDoPrintfVToStream(pHlp, pszFormat, va);
    va_end(va);
}

#endif


/**
 * Uses raw-mode to query all possible MSRs on the real hardware.
 *
 * This generates a msr-report.txt file (appending, no overwriting) as well as
 * writing the values and process to stdout.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3DECL(int) VMMDoBruteForceMsrs(PVM pVM)
{
#ifdef VBOX_WITH_RAW_MODE
    PRTSTREAM pOutStrm;
    int rc = RTStrmOpen("msr-report.txt", "a", &pOutStrm);
    if (RT_SUCCESS(rc))
    {
        /* Header */
        struct
        {
            PRTSTREAM   pOutStrm;
            DBGFINFOHLP Hlp;
        } MyHlp = { pOutStrm, { vmmDoPrintfToStream, vmmDoPrintfVToStream } };
        DBGFR3Info(pVM->pUVM, "cpuid", "verbose", &MyHlp.Hlp);
        RTStrmPrintf(pOutStrm, "\n");

        uint32_t cMsrsFound = 0;
        vmmR3ReportMsrRange(pVM, 0, _4G, pOutStrm, &cMsrsFound);

        RTStrmPrintf(pOutStrm, "Total %u (%#x) MSRs\n", cMsrsFound, cMsrsFound);
        RTPrintf("Total %u (%#x) MSRs\n", cMsrsFound, cMsrsFound);

        RTStrmClose(pOutStrm);
    }
    return rc;
#else
    RT_NOREF(pVM);
    return VERR_NOT_SUPPORTED;
#endif
}


/**
 * Uses raw-mode to query all known MSRS on the real hardware.
 *
 * This generates a known-msr-report.txt file (appending, no overwriting) as
 * well as writing the values and process to stdout.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3DECL(int) VMMDoKnownMsrs(PVM pVM)
{
#ifdef VBOX_WITH_RAW_MODE
    PRTSTREAM pOutStrm;
    int rc = RTStrmOpen("known-msr-report.txt", "a", &pOutStrm);
    if (RT_SUCCESS(rc))
    {
        vmmR3DoMsrQuickReport(pVM, pOutStrm, false);
        RTStrmClose(pOutStrm);
    }
    return rc;
#else
    RT_NOREF(pVM);
    return VERR_NOT_SUPPORTED;
#endif
}


/**
 * MSR experimentation.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3DECL(int) VMMDoMsrExperiments(PVM pVM)
{
#ifdef VBOX_WITH_RAW_MODE
    /*
     * Preps.
     */
    RTRCPTR RCPtrEP;
    int rc = PDMR3LdrGetSymbolRC(pVM, VMMRC_MAIN_MODULE_NAME, "VMMRCTestTestWriteMsr", &RCPtrEP);
    AssertMsgRCReturn(rc, ("Failed to resolved VMMRC.rc::VMMRCEntry(), rc=%Rrc\n", rc), rc);

    uint64_t *pauValues;
    rc = MMHyperAlloc(pVM, 2 * sizeof(uint64_t), 0, MM_TAG_VMM, (void **)&pauValues);
    AssertMsgRCReturn(rc, ("Error allocating %#x bytes off the hyper heap: %Rrc\n", 2 * sizeof(uint64_t), rc), rc);
    RTRCPTR RCPtrValues = MMHyperR3ToRC(pVM, pauValues);

    /*
     * Do the experiments.
     */
    uint32_t uMsr   = 0x00000277;
    uint64_t uValue = UINT64_C(0x0007010600070106);
# if 0
    uValue &= ~(RT_BIT_64(17) | RT_BIT_64(16) | RT_BIT_64(15) | RT_BIT_64(14) | RT_BIT_64(13));
    uValue |= RT_BIT_64(13);
    rc = VMMR3CallRC(pVM, RCPtrEP, 6, pVM->pVMRC, uMsr, RT_LODWORD(uValue), RT_HIDWORD(uValue),
                     RCPtrValues, RCPtrValues + sizeof(uint64_t));
    RTPrintf("uMsr=%#010x before=%#018llx written=%#018llx after=%#018llx rc=%Rrc\n",
             uMsr, pauValues[0], uValue, pauValues[1], rc);
# elif 1
    const uint64_t uOrgValue = uValue;
    uint32_t       cChanges = 0;
    for (int iBit = 63; iBit >= 58; iBit--)
    {
        uValue = uOrgValue & ~RT_BIT_64(iBit);
        rc = VMMR3CallRC(pVM, RCPtrEP, 6, pVM->pVMRC, uMsr, RT_LODWORD(uValue), RT_HIDWORD(uValue),
                         RCPtrValues, RCPtrValues + sizeof(uint64_t));
        RTPrintf("uMsr=%#010x before=%#018llx written=%#018llx after=%#018llx rc=%Rrc\nclear bit=%u -> %s\n",
                 uMsr, pauValues[0], uValue, pauValues[1], rc, iBit,
                 (pauValues[0] ^  pauValues[1]) & RT_BIT_64(iBit) ?  "changed" : "unchanged");
        cChanges += RT_BOOL(pauValues[0] ^ pauValues[1]);

        uValue = uOrgValue | RT_BIT_64(iBit);
        rc = VMMR3CallRC(pVM, RCPtrEP, 6, pVM->pVMRC, uMsr, RT_LODWORD(uValue), RT_HIDWORD(uValue),
                         RCPtrValues, RCPtrValues + sizeof(uint64_t));
        RTPrintf("uMsr=%#010x before=%#018llx written=%#018llx after=%#018llx rc=%Rrc\nset   bit=%u -> %s\n",
                 uMsr, pauValues[0], uValue, pauValues[1], rc, iBit,
                 (pauValues[0] ^  pauValues[1]) & RT_BIT_64(iBit) ?  "changed" : "unchanged");
        cChanges += RT_BOOL(pauValues[0] ^ pauValues[1]);
    }
    RTPrintf("%u change(s)\n", cChanges);
# else
    uint64_t fWriteable = 0;
    for (uint32_t i = 0; i <= 63; i++)
    {
        uValue = RT_BIT_64(i);
# if 0
        if (uValue & (0x7))
            continue;
# endif
        rc = VMMR3CallRC(pVM, RCPtrEP, 6, pVM->pVMRC, uMsr, RT_LODWORD(uValue), RT_HIDWORD(uValue),
                         RCPtrValues, RCPtrValues + sizeof(uint64_t));
        RTPrintf("uMsr=%#010x before=%#018llx written=%#018llx after=%#018llx rc=%Rrc\n",
                 uMsr, pauValues[0], uValue, pauValues[1], rc);
        if (RT_SUCCESS(rc))
            fWriteable |= RT_BIT_64(i);
    }

    uValue = 0;
    rc = VMMR3CallRC(pVM, RCPtrEP, 6, pVM->pVMRC, uMsr, RT_LODWORD(uValue), RT_HIDWORD(uValue),
                     RCPtrValues, RCPtrValues + sizeof(uint64_t));
    RTPrintf("uMsr=%#010x before=%#018llx written=%#018llx after=%#018llx rc=%Rrc\n",
             uMsr, pauValues[0], uValue, pauValues[1], rc);

    uValue = UINT64_MAX;
    rc = VMMR3CallRC(pVM, RCPtrEP, 6, pVM->pVMRC, uMsr, RT_LODWORD(uValue), RT_HIDWORD(uValue),
                     RCPtrValues, RCPtrValues + sizeof(uint64_t));
    RTPrintf("uMsr=%#010x before=%#018llx written=%#018llx after=%#018llx rc=%Rrc\n",
             uMsr, pauValues[0], uValue, pauValues[1], rc);

    uValue = fWriteable;
    rc = VMMR3CallRC(pVM, RCPtrEP, 6, pVM->pVMRC, uMsr, RT_LODWORD(uValue), RT_HIDWORD(uValue),
                     RCPtrValues, RCPtrValues + sizeof(uint64_t));
    RTPrintf("uMsr=%#010x before=%#018llx written=%#018llx after=%#018llx rc=%Rrc [fWriteable]\n",
             uMsr, pauValues[0], uValue, pauValues[1], rc);

# endif

    /*
     * Cleanups.
     */
    MMHyperFree(pVM, pauValues);
    return rc;
#else
    RT_NOREF(pVM);
    return VERR_NOT_SUPPORTED;
#endif
}

