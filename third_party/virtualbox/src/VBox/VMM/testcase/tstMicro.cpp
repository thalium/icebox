/* $Id: tstMicro.cpp $ */
/** @file
 * Micro Testcase, profiling special CPU operations.
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
#include <VBox/vmm/vm.h>
#include <VBox/vmm/vmm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/pgm.h>
#include <VBox/err.h>
#include <VBox/param.h>

#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/initterm.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/semaphore.h>

#include "tstMicro.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#define TESTCASE    "tstVMM"

static const char *GetDescription(TSTMICROTEST enmTest)
{
    switch (enmTest)
    {
        case TSTMICROTEST_OVERHEAD:         return "Overhead";
        case TSTMICROTEST_INVLPG_0:         return "invlpg [0]";
        case TSTMICROTEST_INVLPG_EIP:       return "invlpg [EIP]";
        case TSTMICROTEST_INVLPG_ESP:       return "invlpg [ESP]";
        case TSTMICROTEST_CR3_RELOAD:       return "cr3 reload";
        case TSTMICROTEST_WP_DISABLE:       return "CR0.WP <- 0";
        case TSTMICROTEST_WP_ENABLE:        return "CR0.WP <- 1";

        case TSTMICROTEST_PF_R0:            return "R0 #PG (NULL)";
        case TSTMICROTEST_PF_R1:            return "R1 #PG (NULL)";
        case TSTMICROTEST_PF_R2:            return "R2 #PG (NULL)";
        case TSTMICROTEST_PF_R3:            return "R3 #PG (NULL)";

        default:
        {
            static char sz[64];
            RTStrPrintf(sz, sizeof(sz), "%d?", enmTest);
            return sz;
        }
    }
}


static void PrintHeaderInstr(void)
{
    RTPrintf(TESTCASE ": %-25s  %10s  %10s  %10s\n",
             "Test name",
             "Min",
             "Avg",
             "Max");
}

static void PrintResultInstr(PTSTMICRO pTst, TSTMICROTEST enmTest, int rc, uint64_t cMinTicks, uint64_t cAvgTicks, uint64_t cMaxTicks)
{
    if (RT_FAILURE(rc))
        RTPrintf(TESTCASE ": %-25s  %10llu  %10llu  %10llu - %Rrc cr2=%x err=%x eip=%x!\n",
                 GetDescription(enmTest),
                 cMinTicks,
                 cAvgTicks,
                 cMaxTicks,
                 rc,
                 pTst->u32CR2,
                 pTst->u32ErrCd,
                 pTst->u32EIP);
    else
        RTPrintf(TESTCASE ": %-25s  %10llu  %10llu  %10llu\n",
                 GetDescription(enmTest),
                 cMinTicks,
                 cAvgTicks,
                 cMaxTicks);
}

static void PrintHeaderTraps(void)
{
    RTPrintf(TESTCASE ": %-25s  %10s  %10s  %10s  %10s  %10s\n",
             "Test name",
             "Total",
             "ToRx",
             "Trap",
             "ToRxTrap",
             "int42-done");
}

static void PrintResultTrap(PTSTMICRO pTst, TSTMICROTEST enmTest, int rc)
{
    if (RT_FAILURE(rc))
        RTPrintf(TESTCASE ": %-25s  %10llu  %10llu  %10llu  %10llu  %10llu - %Rrc cr2=%x err=%x eip=%x!\n",
                 GetDescription(enmTest),
                 pTst->aResults[enmTest].cTotalTicks,
                 pTst->aResults[enmTest].cToRxFirstTicks,
                 pTst->aResults[enmTest].cTrapTicks,
                 pTst->aResults[enmTest].cToRxTrapTicks,
                 pTst->aResults[enmTest].cToR0Ticks,
                 rc,
                 pTst->u32CR2,
                 pTst->u32ErrCd,
                 pTst->u32EIP);
    else
        RTPrintf(TESTCASE ": %-25s  %10llu  %10llu  %10llu  %10llu  %10llu\n",
                 GetDescription(enmTest),
                 pTst->aResults[enmTest].cTotalTicks,
                 pTst->aResults[enmTest].cToRxFirstTicks,
                 pTst->aResults[enmTest].cTrapTicks,
                 pTst->aResults[enmTest].cToRxTrapTicks,
                 pTst->aResults[enmTest].cToR0Ticks);
}


/**
 * 'Allocate' selectors for 32-bit code/data in rings 0-3.
 *
 * 0060 - r0 code
 * 0068 - r0 data
 *
 * 1060 - r1 code
 * 1068 - r1 data
 *
 * 2060 - r2 code
 * 2068 - r2 data
 *
 * 3060 - r3 code
 * 3068 - r3 data
 *
 */
static void SetupSelectors(PVM pVM)
{
    /*
     * Find the GDT - This is a HACK :-)
     */
    RTRCPTR     RCPtr = CPUMGetHyperGDTR(VMMGetCpu0(pVM), NULL);
    PX86DESC    paGDTEs = (PX86DESC)MMHyperRCToR3(pVM, RCPtr);

    for (unsigned i = 0; i <= 3; i++)
    {
        RTSEL Sel = (i << 12) + 0x60;

        /* 32-bit code selector. */
        PX86DESC pGDTE = &paGDTEs[Sel >> X86_SEL_SHIFT];
        pGDTE->au32[0] = pGDTE->au32[1] = 0;
        pGDTE->Gen.u16LimitLow  = 0xffff;
        pGDTE->Gen.u4LimitHigh  = 0xf;
        pGDTE->Gen.u1Granularity= 1;
        pGDTE->Gen.u1Present    = 1;
        pGDTE->Gen.u2Dpl        = i;
        pGDTE->Gen.u1DefBig     = 1;
        pGDTE->Gen.u1DescType   = 1; /* !system */
        pGDTE->Gen.u4Type       = X86_SEL_TYPE_ER_ACC;

        /* 32-bit data selector. */
        pGDTE++;
        pGDTE->au32[0] = pGDTE->au32[1] = 0;
        pGDTE->Gen.u16LimitLow  = 0xffff;
        pGDTE->Gen.u4LimitHigh  = 0xf;
        pGDTE->Gen.u1Granularity= 1;
        pGDTE->Gen.u1Present    = 1;
        pGDTE->Gen.u2Dpl        = i;
        pGDTE->Gen.u1DefBig     = 1;
        pGDTE->Gen.u1DescType   = 1; /* !system */
        pGDTE->Gen.u4Type       = X86_SEL_TYPE_RW_ACC;
    }
}


static DECLCALLBACK(int) doit(PVM pVM)
{
    RTPrintf(TESTCASE ": testing...\n");
    SetupSelectors(pVM);

    /*
     * Loading the module and resolve the entry point.
     */
    int rc = PDMR3LdrLoadRC(pVM, NULL, "tstMicroRC.gc");
    if (RT_FAILURE(rc))
    {
        RTPrintf(TESTCASE ": Failed to load tstMicroRC.gc, rc=%Rra\n", rc);
        return rc;
    }
    RTRCPTR RCPtrEntry;
    rc = PDMR3LdrGetSymbolRC(pVM, "tstMicroRC.gc", "tstMicroRC", &RCPtrEntry);
    if (RT_FAILURE(rc))
    {
        RTPrintf(TESTCASE ": Failed to resolve the 'tstMicroRC' entry point in tstMicroRC.gc, rc=%Rra\n", rc);
        return rc;
    }
    RTRCPTR RCPtrStart;
    rc = PDMR3LdrGetSymbolRC(pVM, "tstMicroRC.gc", "tstMicroRCAsmStart", &RCPtrStart);
    if (RT_FAILURE(rc))
    {
        RTPrintf(TESTCASE ": Failed to resolve the 'tstMicroRCAsmStart' entry point in tstMicroRC.gc, rc=%Rra\n", rc);
        return rc;
    }
    RTRCPTR RCPtrEnd;
    rc = PDMR3LdrGetSymbolRC(pVM, "tstMicroRC.gc", "tstMicroRCAsmEnd", &RCPtrEnd);
    if (RT_FAILURE(rc))
    {
        RTPrintf(TESTCASE ": Failed to resolve the 'tstMicroRCAsmEnd' entry point in tstMicroRC.gc, rc=%Rra\n", rc);
        return rc;
    }

    /*
     * Allocate and initialize the instance data.
     */
    PTSTMICRO pTst;
    rc = MMHyperAlloc(pVM, RT_ALIGN_Z(sizeof(*pTst), PAGE_SIZE), PAGE_SIZE, MM_TAG_VM, (void **)&pTst);
    if (RT_FAILURE(rc))
    {
        RTPrintf(TESTCASE ": Failed to resolve allocate instance memory (%d bytes), rc=%Rra\n", sizeof(*pTst), rc);
        return rc;
    }
    pTst->RCPtr = MMHyperR3ToRC(pVM, pTst);
    pTst->RCPtrStack = MMHyperR3ToRC(pVM, &pTst->au8Stack[sizeof(pTst->au8Stack) - 32]);

    /* the page must be writable from user mode */
    rc = PGMMapModifyPage(pVM, pTst->RCPtr, sizeof(*pTst), X86_PTE_US | X86_PTE_RW, ~(uint64_t)(X86_PTE_US | X86_PTE_RW));
    if (RT_FAILURE(rc))
    {
        RTPrintf(TESTCASE ": PGMMapModifyPage -> rc=%Rra\n", rc);
        return rc;
    }

    /* all the code must be executable from R3. */
    rc = PGMMapModifyPage(pVM, RCPtrStart, RCPtrEnd - RCPtrStart + PAGE_SIZE, X86_PTE_US, ~(uint64_t)X86_PTE_US);
    if (RT_FAILURE(rc))
    {
        RTPrintf(TESTCASE ": PGMMapModifyPage -> rc=%Rra\n", rc);
        return rc;
    }
    DBGFR3PagingDumpEx(pVM->pUVM, 0 /*idCpu*/, DBGFPGDMP_FLAGS_CURRENT_CR3 | DBGFPGDMP_FLAGS_CURRENT_MODE
                       | DBGFPGDMP_FLAGS_SHADOW | DBGFPGDMP_FLAGS_HEADER | DBGFPGDMP_FLAGS_PRINT_CR3,
                       0 /*cr3*/, 0 /*u64FirstAddr*/, UINT64_MAX /*u64LastAddr*/, 99 /*cMaxDepth*/, NULL);

#if 0
    /*
     * Disassemble the assembly...
     */
    RTGCPTR GCPtr = RCPtrStart;
    while (GCPtr < RCPtrEnd)
    {
        size_t  cb = 0;
        char    sz[256];
        int rc = DBGFR3DisasInstrEx(pVM, CPUMGetHyperCS(pVM), GCPtr, 0, sz, sizeof(sz), &cb);
        if (RT_SUCCESS(rc))
            RTLogPrintf("%s\n", sz);
        else
        {
            RTLogPrintf("%RGv rc=%Rrc\n", GCPtr, rc);
            cb = 1;
        }
        GCPtr += cb;
    }
#endif

#ifdef VBOX_WITH_RAW_MODE
    /*
     * Do the profiling.
     */
    /* execute the instruction profiling tests */
    PrintHeaderInstr();
    int i;
    for (i = TSTMICROTEST_OVERHEAD; i < TSTMICROTEST_TRAP_FIRST; i++)
    {
        TSTMICROTEST enmTest = (TSTMICROTEST)i;
        uint64_t    cMin = UINT64_MAX;
        uint64_t    cMax = 0;
        uint64_t    cTotal = 0;
        unsigned    cSamples = 0;
        rc = VINF_SUCCESS;
        for (int c = 0; c < 100; c++)
        {
            int rc2 = VMMR3CallRC(pVM, RCPtrEntry, 2, pTst->RCPtr, enmTest);
            if (RT_SUCCESS(rc2))
            {
                uint64_t u64 = pTst->aResults[enmTest].cTotalTicks;
                if (cMin > u64)
                    cMin = u64;
                if (cMax < u64)
                    cMax = u64;
                cTotal += u64;
                cSamples++;
            }
            else if (RT_SUCCESS(rc))
                rc = rc2;
        }
        uint64_t cAvg = cTotal / (cSamples ? cSamples : 1);
        pTst->aResults[enmTest].cTotalTicks = cAvg;
        PrintResultInstr(pTst, enmTest, rc, cMin, cAvg, cMax);
        /* store the overhead */
        if (enmTest == TSTMICROTEST_OVERHEAD)
            pTst->u64Overhead = cMin;
    }
#endif


#ifdef VBOX_WITH_RAW_MODE
    /* execute the trap/cycle profiling tests. */
    RTPrintf("\n");
    PrintHeaderTraps();
    /* don't disable rdtsc in R1/R2/R3! */
    CPUMR3SetCR4Feature(pVM, 0, ~X86_CR4_TSD);
    for (i = TSTMICROTEST_TRAP_FIRST; i < TSTMICROTEST_MAX; i++)
    {
        TSTMICROTEST enmTest = (TSTMICROTEST)i;
        rc = VMMR3CallRC(pVM, RCPtrEntry, 2, pTst->RCPtr, enmTest);
        PrintResultTrap(pTst, enmTest, rc);
    }
#endif

    RTPrintf(TESTCASE ": done!\n");
    return VINF_SUCCESS;
}


/**
 *  Entry point.
 */
extern "C" DECLEXPORT(int) TrustedMain(int argc, char **argv, char **envp)
{
    RT_NOREF1(envp);
    int     rcRet = 0;                  /* error count. */

    RTR3InitExe(argc, &argv, RTR3INIT_FLAGS_SUPLIB);

    /*
     * Create empty VM.
     */
    PVM pVM;
    PUVM pUVM;
    int rc = VMR3Create(1, NULL, NULL, NULL, NULL, NULL, &pVM, &pUVM);
    if (RT_SUCCESS(rc))
    {
        /*
         * Do testing.
         */
        rc = VMR3ReqCallVoidWaitU(pUVM, VMCPUID_ANY, (PFNRT)doit, 1, pVM);
        AssertRC(rc);
        STAMR3Dump(pUVM, "*");

        /*
         * Cleanup.
         */
        rc = VMR3PowerOff(pUVM);
        if (!RT_SUCCESS(rc))
        {
            RTPrintf(TESTCASE ": error: failed to power off vm! rc=%Rrc\n", rc);
            rcRet++;
        }
        rc = VMR3Destroy(pUVM);
        if (!RT_SUCCESS(rc))
        {
            RTPrintf(TESTCASE ": error: failed to destroy vm! rc=%Rrc\n", rc);
            rcRet++;
        }
        VMR3ReleaseUVM(pUVM);
    }
    else
    {
        RTPrintf(TESTCASE ": fatal error: failed to create vm! rc=%Rrc\n", rc);
        rcRet++;
    }

    return rcRet;
}


#if !defined(VBOX_WITH_HARDENING) || !defined(RT_OS_WINDOWS)
/**
 * Main entry point.
 */
int main(int argc, char **argv, char **envp)
{
    return TrustedMain(argc, argv, envp);
}
#endif

