/* $Id: tstVMM.cpp $ */
/** @file
 * VMM Testcase.
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
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/tm.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/ctype.h>
#include <iprt/getopt.h>
#include <iprt/initterm.h>
#include <iprt/message.h>
#include <iprt/semaphore.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/test.h>
#include <iprt/thread.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#define TESTCASE    "tstVMM"



/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static uint32_t g_cCpus = 1;
static bool     g_fStat = false;                /* don't create log files on the testboxes */


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
VMMR3DECL(int) VMMDoTest(PVM pVM);              /* Linked into VMM, see ../VMMTests.cpp. */
VMMR3DECL(int) VMMDoBruteForceMsrs(PVM pVM);    /* Ditto. */
VMMR3DECL(int) VMMDoKnownMsrs(PVM pVM);         /* Ditto. */
VMMR3DECL(int) VMMDoMsrExperiments(PVM pVM);    /* Ditto. */


/** Dummy timer callback. */
static DECLCALLBACK(void) tstTMDummyCallback(PVM pVM, PTMTIMER pTimer, void *pvUser)
{
    NOREF(pVM);
    NOREF(pTimer);
    NOREF(pvUser);
}


/**
 * This is called on each EMT and will beat TM.
 *
 * @returns VINF_SUCCESS, test failure is reported via RTTEST.
 * @param   pVM         Pointer to the VM.
 * @param   hTest       The test handle.
 */
DECLCALLBACK(int) tstTMWorker(PVM pVM, RTTEST hTest)
{
    VMCPUID idCpu = VMMGetCpuId(pVM);
    RTTestPrintfNl(hTest,  RTTESTLVL_ALWAYS,  "idCpu=%d STARTING\n", idCpu);

    /*
     * Create the test set.
     */
    int rc;
    PTMTIMER apTimers[5];
    for (size_t i = 0; i < RT_ELEMENTS(apTimers); i++)
    {
        rc = TMR3TimerCreateInternal(pVM, i & 1 ? TMCLOCK_VIRTUAL :  TMCLOCK_VIRTUAL_SYNC,
                                     tstTMDummyCallback, NULL, "test timer",  &apTimers[i]);
        RTTEST_CHECK_RET(hTest, RT_SUCCESS(rc), rc);
    }

    /*
     * The run loop.
     */
    unsigned        uPrevPct = 0;
    uint32_t const  cLoops   = 100000;
    for (uint32_t iLoop = 0; iLoop < cLoops; iLoop++)
    {
        size_t      cLeft = RT_ELEMENTS(apTimers);
        unsigned    i     = iLoop % RT_ELEMENTS(apTimers);
        while (cLeft-- > 0)
        {
            PTMTIMER pTimer = apTimers[i];

            if (    cLeft == RT_ELEMENTS(apTimers) / 2
                &&  TMTimerIsActive(pTimer))
            {
                rc = TMTimerStop(pTimer);
                RTTEST_CHECK_MSG(hTest, RT_SUCCESS(rc), (hTest, "TMTimerStop: %Rrc\n",  rc));
            }
            else
            {
                rc = TMTimerSetMicro(pTimer, 50 + cLeft);
                RTTEST_CHECK_MSG(hTest, RT_SUCCESS(rc), (hTest, "TMTimerSetMicro: %Rrc\n", rc));
            }

            /* next */
            i = (i + 1) % RT_ELEMENTS(apTimers);
        }

        if (i % 3)
            TMR3TimerQueuesDo(pVM);

        /* Progress report. */
        unsigned uPct = (unsigned)(100.0 * iLoop / cLoops);
        if (uPct != uPrevPct)
        {
            uPrevPct = uPct;
            if (!(uPct % 10))
                RTTestPrintfNl(hTest,  RTTESTLVL_ALWAYS,  "idCpu=%d - %3u%%\n", idCpu, uPct);
        }
    }

    RTTestPrintfNl(hTest,  RTTESTLVL_ALWAYS,  "idCpu=%d DONE\n", idCpu);
    return 0;
}


/** PDMR3LdrEnumModules callback, see FNPDMR3ENUM. */
static DECLCALLBACK(int)
tstVMMLdrEnum(PVM pVM, const char *pszFilename, const char *pszName, RTUINTPTR ImageBase, size_t cbImage,
              PDMLDRCTX enmCtx, void *pvUser)
{
    NOREF(pVM); NOREF(pszFilename); NOREF(enmCtx); NOREF(pvUser); NOREF(cbImage);
    RTPrintf("tstVMM: %RTptr %s\n", ImageBase, pszName);
    return VINF_SUCCESS;
}

static DECLCALLBACK(int)
tstVMMConfigConstructor(PUVM pUVM, PVM pVM, void *pvUser)
{
    RT_NOREF2(pUVM, pvUser);
    int rc = CFGMR3ConstructDefaultTree(pVM);
    if (RT_SUCCESS(rc))
    {
        PCFGMNODE pRoot = CFGMR3GetRoot(pVM);
        if (g_cCpus < 2)
        {
            rc = CFGMR3InsertInteger(pRoot, "HMEnabled", false);
            RTTESTI_CHECK_MSG_RET(RT_SUCCESS(rc),
                                  ("CFGMR3InsertInteger(pRoot,\"HMEnabled\",) -> %Rrc\n", rc), rc);
        }
        else if (g_cCpus > 1)
        {
            CFGMR3RemoveValue(pRoot, "NumCPUs");
            rc = CFGMR3InsertInteger(pRoot, "NumCPUs", g_cCpus);
            RTTESTI_CHECK_MSG_RET(RT_SUCCESS(rc),
                                  ("CFGMR3InsertInteger(pRoot,\"NumCPUs\",) -> %Rrc\n", rc), rc);

            CFGMR3RemoveValue(pRoot, "HwVirtExtForced");
            rc = CFGMR3InsertInteger(pRoot, "HwVirtExtForced", true);
            RTTESTI_CHECK_MSG_RET(RT_SUCCESS(rc),
                                  ("CFGMR3InsertInteger(pRoot,\"HwVirtExtForced\",) -> %Rrc\n", rc), rc);
            PCFGMNODE pHwVirtExt = CFGMR3GetChild(pRoot, "HWVirtExt");
            CFGMR3RemoveNode(pHwVirtExt);
            rc = CFGMR3InsertNode(pRoot, "HWVirtExt", &pHwVirtExt);
            RTTESTI_CHECK_MSG_RET(RT_SUCCESS(rc),
                                  ("CFGMR3InsertNode(pRoot,\"HWVirtExt\",) -> %Rrc\n", rc), rc);
            rc = CFGMR3InsertInteger(pHwVirtExt, "Enabled", true);
            RTTESTI_CHECK_MSG_RET(RT_SUCCESS(rc),
                                  ("CFGMR3InsertInteger(pHwVirtExt,\"Enabled\",) -> %Rrc\n", rc), rc);
            rc = CFGMR3InsertInteger(pHwVirtExt, "64bitEnabled", false);
            RTTESTI_CHECK_MSG_RET(RT_SUCCESS(rc),
                                  ("CFGMR3InsertInteger(pHwVirtExt,\"64bitEnabled\",) -> %Rrc\n", rc), rc);
        }
    }
    return rc;
}


/**
 * Entry point.
 */
extern "C" DECLEXPORT(int) TrustedMain(int argc, char **argv, char **envp)
{
    RT_NOREF1(envp);

    /*
     * Init runtime and the test environment.
     */
    RTTEST hTest;
    RTEXITCODE rcExit = RTTestInitExAndCreate(argc, &argv, RTR3INIT_FLAGS_SUPLIB, "tstVMM", &hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;

    /*
     * Parse arguments.
     */
    static const RTGETOPTDEF s_aOptions[] =
    {
        { "--cpus",          'c', RTGETOPT_REQ_UINT8 },
        { "--test",          't', RTGETOPT_REQ_STRING },
        { "--stat",          's', RTGETOPT_REQ_NOTHING },
    };
    enum
    {
        kTstVMMTest_VMM,  kTstVMMTest_TM, kTstVMMTest_MSRs, kTstVMMTest_KnownMSRs, kTstVMMTest_MSRExperiments
    } enmTestOpt = kTstVMMTest_VMM;

    int ch;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    RTGetOptInit(&GetState, argc, argv, s_aOptions, RT_ELEMENTS(s_aOptions), 1, 0);
    while ((ch = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (ch)
        {
            case 'c':
                g_cCpus = ValueUnion.u8;
                break;

            case 't':
                if (!strcmp("vmm", ValueUnion.psz))
                    enmTestOpt = kTstVMMTest_VMM;
                else if (!strcmp("tm", ValueUnion.psz))
                    enmTestOpt = kTstVMMTest_TM;
                else if (!strcmp("msr", ValueUnion.psz) || !strcmp("msrs", ValueUnion.psz))
                    enmTestOpt = kTstVMMTest_MSRs;
                else if (!strcmp("known-msr", ValueUnion.psz) || !strcmp("known-msrs", ValueUnion.psz))
                    enmTestOpt = kTstVMMTest_KnownMSRs;
                else if (!strcmp("msr-experiments", ValueUnion.psz))
                    enmTestOpt = kTstVMMTest_MSRExperiments;
                else
                {
                    RTPrintf("tstVMM: unknown test: '%s'\n", ValueUnion.psz);
                    return 1;
                }
                break;

            case 's':
                g_fStat = true;
                break;

            case 'h':
                RTPrintf("usage: tstVMM [--cpus|-c cpus] [-s] [--test <vmm|tm|msrs|known-msrs>]\n");
                return 1;

            case 'V':
                RTPrintf("$Revision: 118412 $\n");
                return 0;

            default:
                return RTGetOptPrintError(ch, &ValueUnion);
        }
    }

    /*
     * Create the test VM.
     */
    RTPrintf(TESTCASE ": Initializing...\n");
    PVM pVM;
    PUVM pUVM;
    int rc = VMR3Create(g_cCpus, NULL, NULL, NULL, tstVMMConfigConstructor, NULL, &pVM, &pUVM);
    if (RT_SUCCESS(rc))
    {
        PDMR3LdrEnumModules(pVM, tstVMMLdrEnum, NULL);
        RTStrmFlush(g_pStdOut);
        RTThreadSleep(256);

        /*
         * Do the requested testing.
         */
        switch (enmTestOpt)
        {
            case kTstVMMTest_VMM:
            {
                RTTestSub(hTest, "VMM");
                rc = VMR3ReqCallWaitU(pUVM, VMCPUID_ANY, (PFNRT)VMMDoTest, 1, pVM);
                if (RT_FAILURE(rc))
                    RTTestFailed(hTest, "VMMDoTest failed: rc=%Rrc\n", rc);
                if (g_fStat)
                    STAMR3Dump(pUVM, "*");
                break;
            }

            case kTstVMMTest_TM:
            {
                RTTestSub(hTest, "TM");
                for (VMCPUID idCpu = 1; idCpu < g_cCpus; idCpu++)
                {
                    rc = VMR3ReqCallNoWaitU(pUVM, idCpu, (PFNRT)tstTMWorker, 2, pVM, hTest);
                    if (RT_FAILURE(rc))
                        RTTestFailed(hTest, "VMR3ReqCall failed: rc=%Rrc\n", rc);
                }

                rc = VMR3ReqCallWaitU(pUVM, 0 /*idDstCpu*/, (PFNRT)tstTMWorker, 2, pVM, hTest);
                if (RT_FAILURE(rc))
                    RTTestFailed(hTest, "VMMDoTest failed: rc=%Rrc\n", rc);
                if (g_fStat)
                    STAMR3Dump(pUVM, "*");
                break;
            }

            case kTstVMMTest_MSRs:
            {
                RTTestSub(hTest, "MSRs");
                if (g_cCpus == 1)
                {
                    rc = VMR3ReqCallWaitU(pUVM, 0 /*idDstCpu*/, (PFNRT)VMMDoBruteForceMsrs, 1, pVM);
                    if (RT_FAILURE(rc))
                        RTTestFailed(hTest, "VMMDoBruteForceMsrs failed: rc=%Rrc\n", rc);
                }
                else
                    RTTestFailed(hTest, "The MSR test can only be run with one VCpu!\n");
                break;
            }

            case kTstVMMTest_KnownMSRs:
            {
                RTTestSub(hTest, "Known MSRs");
                if (g_cCpus == 1)
                {
                    rc = VMR3ReqCallWaitU(pUVM, 0 /*idDstCpu*/, (PFNRT)VMMDoKnownMsrs, 1, pVM);
                    if (RT_FAILURE(rc))
                        RTTestFailed(hTest, "VMMDoKnownMsrs failed: rc=%Rrc\n", rc);
                }
                else
                    RTTestFailed(hTest, "The MSR test can only be run with one VCpu!\n");
                break;
            }

            case kTstVMMTest_MSRExperiments:
            {
                RTTestSub(hTest, "MSR Experiments");
                if (g_cCpus == 1)
                {
                    rc = VMR3ReqCallWaitU(pUVM, 0 /*idDstCpu*/, (PFNRT)VMMDoMsrExperiments, 1, pVM);
                    if (RT_FAILURE(rc))
                        RTTestFailed(hTest, "VMMDoMsrExperiments failed: rc=%Rrc\n", rc);
                }
                else
                    RTTestFailed(hTest, "The MSR test can only be run with one VCpu!\n");
                break;
            }

        }

        /*
         * Cleanup.
         */
        rc = VMR3PowerOff(pUVM);
        if (RT_FAILURE(rc))
            RTTestFailed(hTest, "VMR3PowerOff failed: rc=%Rrc\n", rc);
        rc = VMR3Destroy(pUVM);
        if (RT_FAILURE(rc))
            RTTestFailed(hTest, "VMR3Destroy failed: rc=%Rrc\n", rc);
        VMR3ReleaseUVM(pUVM);
    }
    else
        RTTestFailed(hTest, "VMR3Create failed: rc=%Rrc\n", rc);

    return RTTestSummaryAndDestroy(hTest);
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

