/* $Id: tstVMREQ.cpp $ */
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
#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/initterm.h>
#include <iprt/semaphore.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/thread.h>
#include <iprt/time.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#define TESTCASE    "tstVMREQ"


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** the error count. */
static int g_cErrors = 0;


/**
 * Testings va_list passing in VMSetRuntimeError.
 */
static DECLCALLBACK(void) MyAtRuntimeError(PUVM pUVM, void *pvUser, uint32_t fFlags, const char *pszErrorId, const char *pszFormat, va_list va)
{
    NOREF(pUVM);
    if (strcmp((const char *)pvUser, "user argument"))
    {
        RTPrintf(TESTCASE ": pvUser=%p:{%s}!\n", pvUser, (const char *)pvUser);
        g_cErrors++;
    }
    if (fFlags)
    {
        RTPrintf(TESTCASE ": fFlags=%#x!\n", fFlags);
        g_cErrors++;
    }
    if (strcmp(pszErrorId, "enum"))
    {
        RTPrintf(TESTCASE ": pszErrorId=%p:{%s}!\n", pszErrorId, pszErrorId);
        g_cErrors++;
    }
    if (strcmp(pszFormat, "some %s string"))
    {
        RTPrintf(TESTCASE ": pszFormat=%p:{%s}!\n", pszFormat, pszFormat);
        g_cErrors++;
    }

    char szBuf[1024];
    RTStrPrintfV(szBuf, sizeof(szBuf), pszFormat, va);
    if (strcmp(szBuf, "some error string"))
    {
        RTPrintf(TESTCASE ": RTStrPrintfV -> '%s'!\n", szBuf);
        g_cErrors++;
    }
}


/**
 * The function PassVA and PassVA2 calls.
 */
static DECLCALLBACK(int) PassVACallback(PUVM pUVM, unsigned u4K, unsigned u1G, const char *pszFormat, va_list *pva)
{
    NOREF(pUVM);
    if (u4K != _4K)
    {
        RTPrintf(TESTCASE ": u4K=%#x!\n", u4K);
        g_cErrors++;
    }
    if (u1G != _1G)
    {
        RTPrintf(TESTCASE ": u1G=%#x!\n", u1G);
        g_cErrors++;
    }

    if (strcmp(pszFormat, "hello %s"))
    {
        RTPrintf(TESTCASE ": pszFormat=%p:{%s}!\n", pszFormat, pszFormat);
        g_cErrors++;
    }

    char szBuf[1024];
    RTStrPrintfV(szBuf, sizeof(szBuf), pszFormat, *pva);
    if (strcmp(szBuf, "hello world"))
    {
        RTPrintf(TESTCASE ": RTStrPrintfV -> '%s'!\n", szBuf);
        g_cErrors++;
    }

    return VINF_SUCCESS;
}


/**
 * Functions that tests passing a va_list * argument in a request,
 * similar to VMSetRuntimeError.
 */
static void PassVA2(PUVM pUVM, const char *pszFormat, va_list va)
{
#if 0 /** @todo test if this is a GCC problem only or also happens with AMD64+VCC80... */
    void *pvVA = &va;
#else
    va_list va2;
    va_copy(va2, va);
    void *pvVA = &va2;
#endif

    int rc = VMR3ReqCallWaitU(pUVM, VMCPUID_ANY, (PFNRT)PassVACallback, 5, pUVM, _4K, _1G, pszFormat, pvVA);
    NOREF(rc);

#if 1
    va_end(va2);
#endif
}


/**
 * Functions that tests passing a va_list * argument in a request,
 * similar to VMSetRuntimeError.
 */
static void PassVA(PUVM pUVM, const char *pszFormat, ...)
{
    /* 1st test */
    va_list va1;
    va_start(va1, pszFormat);
    int rc = VMR3ReqCallWaitU(pUVM, VMCPUID_ANY, (PFNRT)PassVACallback, 5, pUVM, _4K, _1G, pszFormat, &va1);
    va_end(va1);
    NOREF(rc);

    /* 2nd test */
    va_list va2;
    va_start(va2, pszFormat);
    PassVA2(pUVM, pszFormat, va2);
    va_end(va2);
}


/**
 * Thread function which allocates and frees requests like wildfire.
 */
static DECLCALLBACK(int) Thread(RTTHREAD hThreadSelf, void *pvUser)
{
    int rc = VINF_SUCCESS;
    PUVM pUVM = (PUVM)pvUser;
    NOREF(hThreadSelf);

    for (unsigned i = 0; i < 100000; i++)
    {
        PVMREQ          apReq[17];
        const unsigned  cReqs = i % RT_ELEMENTS(apReq);
        unsigned        iReq;
        for (iReq = 0; iReq < cReqs; iReq++)
        {
            rc = VMR3ReqAlloc(pUVM, &apReq[iReq], VMREQTYPE_INTERNAL, VMCPUID_ANY);
            if (RT_FAILURE(rc))
            {
                RTPrintf(TESTCASE ": i=%d iReq=%d cReqs=%d rc=%Rrc (alloc)\n", i, iReq, cReqs, rc);
                return rc;
            }
            apReq[iReq]->iStatus = iReq + i;
        }

        for (iReq = 0; iReq < cReqs; iReq++)
        {
            if (apReq[iReq]->iStatus != (int)(iReq + i))
            {
                RTPrintf(TESTCASE ": i=%d iReq=%d cReqs=%d: iStatus=%d != %d\n", i, iReq, cReqs, apReq[iReq]->iStatus, iReq + i);
                return VERR_GENERAL_FAILURE;
            }
            rc = VMR3ReqFree(apReq[iReq]);
            if (RT_FAILURE(rc))
            {
                RTPrintf(TESTCASE ": i=%d iReq=%d cReqs=%d rc=%Rrc (free)\n", i, iReq, cReqs, rc);
                return rc;
            }
        }
        //if (!(i % 10000))
        //    RTPrintf(TESTCASE ": i=%d\n", i);
    }

    return VINF_SUCCESS;
}

static DECLCALLBACK(int)
tstVMREQConfigConstructor(PUVM pUVM, PVM pVM, void *pvUser)
{
    RT_NOREF2(pUVM, pvUser);
    return CFGMR3ConstructDefaultTree(pVM);
}

/**
 *  Entry point.
 */
extern "C" DECLEXPORT(int) TrustedMain(int argc, char **argv, char **envp)
{
    RT_NOREF1(envp);
    RTR3InitExe(argc, &argv, RTR3INIT_FLAGS_SUPLIB);
    RTPrintf(TESTCASE ": TESTING...\n");
    RTStrmFlush(g_pStdOut);

    /*
     * Create empty VM.
     */
    PUVM pUVM;
    int rc = VMR3Create(1, NULL, NULL, NULL, tstVMREQConfigConstructor, NULL, NULL, &pUVM);
    if (RT_SUCCESS(rc))
    {
        /*
         * Do testing.
         */
        uint64_t u64StartTS = RTTimeNanoTS();
        RTTHREAD Thread0;
        rc = RTThreadCreate(&Thread0, Thread, pUVM, 0, RTTHREADTYPE_DEFAULT, RTTHREADFLAGS_WAITABLE, "REQ1");
        if (RT_SUCCESS(rc))
        {
            RTTHREAD Thread1;
            rc = RTThreadCreate(&Thread1, Thread, pUVM, 0, RTTHREADTYPE_DEFAULT, RTTHREADFLAGS_WAITABLE, "REQ1");
            if (RT_SUCCESS(rc))
            {
                int rcThread1;
                rc = RTThreadWait(Thread1, RT_INDEFINITE_WAIT, &rcThread1);
                if (RT_FAILURE(rc))
                {
                    RTPrintf(TESTCASE ": RTThreadWait(Thread1,,) failed, rc=%Rrc\n", rc);
                    g_cErrors++;
                }
                if (RT_FAILURE(rcThread1))
                    g_cErrors++;
            }
            else
            {
                RTPrintf(TESTCASE ": RTThreadCreate(&Thread1,,,,) failed, rc=%Rrc\n", rc);
                g_cErrors++;
            }

            int rcThread0;
            rc = RTThreadWait(Thread0, RT_INDEFINITE_WAIT, &rcThread0);
            if (RT_FAILURE(rc))
            {
                RTPrintf(TESTCASE ": RTThreadWait(Thread1,,) failed, rc=%Rrc\n", rc);
                g_cErrors++;
            }
            if (RT_FAILURE(rcThread0))
                g_cErrors++;
        }
        else
        {
            RTPrintf(TESTCASE ": RTThreadCreate(&Thread0,,,,) failed, rc=%Rrc\n", rc);
            g_cErrors++;
        }
        uint64_t u64ElapsedTS = RTTimeNanoTS() - u64StartTS;
        RTPrintf(TESTCASE  ": %llu ns elapsed\n", u64ElapsedTS);
        RTStrmFlush(g_pStdOut);

        /*
         * Print stats.
         */
        STAMR3Print(pUVM, "/VM/Req/*");

        /*
         * Testing va_list fun.
         */
        RTPrintf(TESTCASE ": va_list argument test...\n"); RTStrmFlush(g_pStdOut);
        PassVA(pUVM, "hello %s", "world");
        VMR3AtRuntimeErrorRegister(pUVM, MyAtRuntimeError, (void *)"user argument");
        VMSetRuntimeError(VMR3GetVM(pUVM), 0 /*fFlags*/, "enum", "some %s string", "error");

        /*
         * Cleanup.
         */
        rc = VMR3PowerOff(pUVM);
        if (!RT_SUCCESS(rc))
        {
            RTPrintf(TESTCASE ": error: failed to power off vm! rc=%Rrc\n", rc);
            g_cErrors++;
        }
        rc = VMR3Destroy(pUVM);
        if (!RT_SUCCESS(rc))
        {
            RTPrintf(TESTCASE ": error: failed to destroy vm! rc=%Rrc\n", rc);
            g_cErrors++;
        }
        VMR3ReleaseUVM(pUVM);
    }
    else
    {
        RTPrintf(TESTCASE ": fatal error: failed to create vm! rc=%Rrc\n", rc);
        g_cErrors++;
    }

    /*
     * Summary and return.
     */
    if (!g_cErrors)
        RTPrintf(TESTCASE ": SUCCESS\n");
    else
        RTPrintf(TESTCASE ": FAILURE - %d errors\n", g_cErrors);

    return !!g_cErrors;
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

