/* $Id: tstX86-1.cpp $ */
/** @file
 * X86 instruction set exploration/testcase #1.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
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
#include <iprt/test.h>
#include <iprt/param.h>
#include <iprt/mem.h>
#include <iprt/err.h>
#include <iprt/assert.h>
#include <iprt/x86.h>

#ifdef RT_OS_WINDOWS
# include <iprt/win/windows.h>
#else
# ifdef RT_OS_DARWIN
#  define _XOPEN_SOURCE
# endif
# include <signal.h>
# include <ucontext.h>
# define USE_SIGNAL
#endif


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
typedef struct TRAPINFO
{
    uintptr_t   uTrapPC;
    uintptr_t   uResumePC;
    uint8_t     u8Trap;
    uint8_t     cbInstr;
    uint8_t     auAlignment[sizeof(uintptr_t) * 2 - 2];
} TRAPINFO;
typedef TRAPINFO const *PCTRAPINFO;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
RT_C_DECLS_BEGIN
uint8_t *g_pbEfPage = NULL;
uint8_t *g_pbEfExecPage = NULL;
extern TRAPINFO g_aTrapInfo[];
RT_C_DECLS_END


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
DECLASM(int32_t) x861_Test1(void);
DECLASM(int32_t) x861_Test2(void);
DECLASM(int32_t) x861_Test3(void);
DECLASM(int32_t) x861_Test4(void);
DECLASM(int32_t) x861_Test5(void);
DECLASM(int32_t) x861_Test6(void);
DECLASM(int32_t) x861_Test7(void);
DECLASM(int32_t) x861_TestFPUInstr1(void);



static PCTRAPINFO findTrapInfo(uintptr_t uTrapPC, uintptr_t uTrapSP)
{
    /* Search by trap program counter. */
    for (unsigned i = 0; g_aTrapInfo[i].uTrapPC; i++)
        if (g_aTrapInfo[i].uTrapPC == uTrapPC)
            return &g_aTrapInfo[i];

    /* Search by return address. */
    uintptr_t uReturn = *(uintptr_t *)uTrapSP;
    for (unsigned i = 0; g_aTrapInfo[i].uTrapPC; i++)
        if (g_aTrapInfo[i].uTrapPC + g_aTrapInfo[i].cbInstr == uReturn)
            return &g_aTrapInfo[i];

    return NULL;
}

#ifdef USE_SIGNAL
static void sigHandler(int iSig, siginfo_t *pSigInfo, void *pvSigCtx)
{
    ucontext_t *pCtx = (ucontext_t *)pvSigCtx;
    NOREF(pSigInfo);

# if defined(RT_ARCH_AMD64) && defined(RT_OS_DARWIN)
    uintptr_t  *puPC    = (uintptr_t *)&pCtx->uc_mcontext->__ss.__rip;
    uintptr_t  *puSP    = (uintptr_t *)&pCtx->uc_mcontext->__ss.__rsp;
    uintptr_t   uTrapNo = pCtx->uc_mcontext->__es.__trapno;
    uintptr_t   uErr    = pCtx->uc_mcontext->__es.__err;
    uintptr_t   uCr2    = pCtx->uc_mcontext->__es.__faultvaddr;

# elif defined(RT_ARCH_AMD64) && defined(RT_OS_FREEBSD)
    uintptr_t  *puPC    = (uintptr_t *)&pCtx->uc_mcontext.mc_rip;
    uintptr_t  *puSP    = (uintptr_t *)&pCtx->uc_mcontext.mc_rsp;
    uintptr_t   uTrapNo = ~(uintptr_t)0;
    uintptr_t   uErr    = ~(uintptr_t)0;
    uintptr_t   uCr2    = ~(uintptr_t)0;

# elif defined(RT_ARCH_AMD64)
    uintptr_t  *puPC    = (uintptr_t *)&pCtx->uc_mcontext.gregs[REG_RIP];
    uintptr_t  *puSP    = (uintptr_t *)&pCtx->uc_mcontext.gregs[REG_RSP];
    uintptr_t   uTrapNo = pCtx->uc_mcontext.gregs[REG_TRAPNO];
    uintptr_t   uErr    = pCtx->uc_mcontext.gregs[REG_ERR];
    uintptr_t   uCr2    = pCtx->uc_mcontext.gregs[REG_CR2];

# elif defined(RT_ARCH_X86) && defined(RT_OS_DARWIN)
    uintptr_t  *puPC    = (uintptr_t *)&pCtx->uc_mcontext->__ss.__eip;
    uintptr_t  *puSP    = (uintptr_t *)&pCtx->uc_mcontext->__ss.__esp;
    uintptr_t   uTrapNo = pCtx->uc_mcontext->__es.__trapno;
    uintptr_t   uErr    = pCtx->uc_mcontext->__es.__err;
    uintptr_t   uCr2    = pCtx->uc_mcontext->__es.__faultvaddr;

# elif defined(RT_ARCH_X86) && defined(RT_OS_FREEBSD)
    uintptr_t  *puPC    = (uintptr_t *)&pCtx->uc_mcontext.mc_eip;
    uintptr_t  *puSP    = (uintptr_t *)&pCtx->uc_mcontext.mc_esp;
    uintptr_t   uTrapNo = ~(uintptr_t)0;
    uintptr_t   uErr    = ~(uintptr_t)0;
    uintptr_t   uCr2    = ~(uintptr_t)0;

# elif defined(RT_ARCH_X86)
    uintptr_t  *puPC    = (uintptr_t *)&pCtx->uc_mcontext.gregs[REG_EIP];
    uintptr_t  *puSP    = (uintptr_t *)&pCtx->uc_mcontext.gregs[REG_ESP];
    uintptr_t   uTrapNo = pCtx->uc_mcontext.gregs[REG_TRAPNO];
    uintptr_t   uErr    = pCtx->uc_mcontext.gregs[REG_ERR];
#  ifdef REG_CR2 /** @todo ... */
    uintptr_t   uCr2    = pCtx->uc_mcontext.gregs[REG_CR2];
#  else
    uintptr_t   uCr2    = ~(uintptr_t)0;
#  endif

# else
    uintptr_t  *puPC    = NULL;
    uintptr_t  *puSP    = NULL;
    uintptr_t   uTrapNo = ~(uintptr_t)0;
    uintptr_t   uErr    = ~(uintptr_t)0;
    uintptr_t   uCr2    = ~(uintptr_t)0;
# endif
    if (uTrapNo == X86_XCPT_PF)
        RTAssertMsg2("tstX86-1: Trap #%#04x err=%#06x at %p / %p\n", uTrapNo, uErr, *puPC, uCr2);
    else
        RTAssertMsg2("tstX86-1: Trap #%#04x err=%#06x at %p\n", uTrapNo, uErr, *puPC);

    PCTRAPINFO pTrapInfo = findTrapInfo(*puPC, *puSP);
    if (pTrapInfo)
    {
        if (pTrapInfo->u8Trap != uTrapNo && uTrapNo != ~(uintptr_t)0)
            RTAssertMsg2("tstX86-1: Expected #%#04x, got #%#04x\n", pTrapInfo->u8Trap, uTrapNo);
        else
        {
            if (*puPC != pTrapInfo->uTrapPC)
                *puSP += sizeof(uintptr_t);
            *puPC = pTrapInfo->uResumePC;
            return;
        }
    }
    else
        RTAssertMsg2("tstX86-1: Unexpected trap!\n");

    /* die */
    signal(iSig, SIG_IGN);
}
#else

#endif



int main()
{
    /*
     * Set up the test environment.
     */
    RTTEST hTest;
    RTEXITCODE rcExit = RTTestInitAndCreate("tstX86-1", &hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;
    RTTestBanner(hTest);

    g_pbEfPage = (uint8_t *)RTTestGuardedAllocTail(hTest, PAGE_SIZE);
    RTTESTI_CHECK(g_pbEfPage != NULL);

    g_pbEfExecPage = (uint8_t *)RTMemExecAlloc(PAGE_SIZE*2);
    RTTESTI_CHECK(g_pbEfExecPage != NULL);
    RTTESTI_CHECK(!((uintptr_t)g_pbEfExecPage & PAGE_OFFSET_MASK));
    RTTESTI_CHECK_RC(RTMemProtect(g_pbEfExecPage + PAGE_SIZE, PAGE_SIZE, RTMEM_PROT_NONE), VINF_SUCCESS);

#ifdef USE_SIGNAL
    static int const s_aiSigs[] = { SIGBUS, SIGSEGV, SIGFPE, SIGILL };
    for (unsigned i = 0; i < RT_ELEMENTS(s_aiSigs); i++)
    {
        struct sigaction SigAct;
        RTTESTI_CHECK_BREAK(sigaction(s_aiSigs[i], NULL, &SigAct) == 0);
        SigAct.sa_sigaction = sigHandler;
        SigAct.sa_flags    |= SA_SIGINFO;
        RTTESTI_CHECK(sigaction(s_aiSigs[i], &SigAct, NULL) == 0);
    }
#else
    /** @todo implement me. */
#endif


    if (!RTTestErrorCount(hTest))
    {
        /*
         * Do the testing.
         */
        int32_t rc;
#if 0
        RTTestSub(hTest, "Misc 1");
        rc = x861_Test1();
        if (rc != 0)
            RTTestFailed(hTest, "x861_Test1 -> %d", rc);

        RTTestSub(hTest, "Prefixes and groups");
        rc = x861_Test2();
        if (rc != 0)
            RTTestFailed(hTest, "x861_Test2 -> %d", rc);

        RTTestSub(hTest, "fxsave / fxrstor and #PFs");
        rc = x861_Test3();
        if (rc != 0)
            RTTestFailed(hTest, "x861_Test3 -> %d", rc);

        RTTestSub(hTest, "Multibyte NOPs");
        rc = x861_Test4();
        if (rc != 0)
            RTTestFailed(hTest, "x861_Test4 -> %d", rc);
//#endif

        RTTestSub(hTest, "Odd encodings and odd ends");
        rc = x861_Test5();
        if (rc != 0)
            RTTestFailed(hTest, "x861_Test5 -> %d", rc);

//#if 0
        RTTestSub(hTest, "Odd floating point encodings");
        rc = x861_Test6();
        if (rc != 0)
            RTTestFailed(hTest, "x861_Test5 -> %d", rc);

        RTTestSub(hTest, "Floating point exceptions ++");
        rc = x861_Test7();
        if (rc != 0)
            RTTestFailed(hTest, "x861_Test6 -> %d", rc);
#endif

        rc = x861_TestFPUInstr1();
        if (rc != 0)
            RTTestFailed(hTest, "x861_TestFPUInstr1 -> %d", rc);
    }

    return RTTestSummaryAndDestroy(hTest);
}

