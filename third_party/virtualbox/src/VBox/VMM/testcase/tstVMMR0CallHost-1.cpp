/* $Id: tstVMMR0CallHost-1.cpp $ */
/** @file
 * Testcase for the VMMR0JMPBUF operations.
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
#include <VBox/err.h>
#include <VBox/param.h>
#include <iprt/alloca.h>
#include <iprt/initterm.h>
#include <iprt/rand.h>
#include <iprt/string.h>
#include <iprt/stream.h>
#include <iprt/test.h>

#define IN_VMM_R0
#define IN_RING0 /* pretent we're in Ring-0 to get the prototypes. */
#include <VBox/vmm/vmm.h>
#include "VMMInternal.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#if !defined(VMM_R0_SWITCH_STACK) && !defined(VMM_R0_NO_SWITCH_STACK)
# error "VMM_R0_SWITCH_STACK or VMM_R0_NO_SWITCH_STACK has to be defined."
#endif


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** The jump buffer. */
static VMMR0JMPBUF          g_Jmp;
/** The number of jumps we've done. */
static unsigned volatile    g_cJmps;
/** Number of bytes allocated last time we called foo(). */
static size_t volatile      g_cbFoo;
/** Number of bytes used last time we called foo(). */
static intptr_t volatile    g_cbFooUsed;


int foo(int i, int iZero, int iMinusOne)
{
    NOREF(iZero);

    /* allocate a buffer which we fill up to the end. */
    size_t cb = (i % 1555) + 32;
    g_cbFoo = cb;
    char  *pv = (char *)alloca(cb);
    RTStrPrintf(pv, cb, "i=%d%*s\n", i, cb, "");
#ifdef VMM_R0_SWITCH_STACK
    g_cbFooUsed = VMM_STACK_SIZE - ((uintptr_t)pv - (uintptr_t)g_Jmp.pvSavedStack);
    RTTESTI_CHECK_MSG_RET(g_cbFooUsed < (intptr_t)VMM_STACK_SIZE - 128, ("%#x - (%p - %p) -> %#x; cb=%#x i=%d\n", VMM_STACK_SIZE, pv, g_Jmp.pvSavedStack, g_cbFooUsed, cb, i), -15);
#elif defined(RT_ARCH_AMD64)
    g_cbFooUsed = (uintptr_t)g_Jmp.rsp - (uintptr_t)pv;
    RTTESTI_CHECK_MSG_RET(g_cbFooUsed < VMM_STACK_SIZE - 128, ("%p - %p -> %#x; cb=%#x i=%d\n", g_Jmp.rsp, pv, g_cbFooUsed, cb, i), -15);
#elif defined(RT_ARCH_X86)
    g_cbFooUsed = (uintptr_t)g_Jmp.esp - (uintptr_t)pv;
    RTTESTI_CHECK_MSG_RET(g_cbFooUsed < (intptr_t)VMM_STACK_SIZE - 128, ("%p - %p -> %#x; cb=%#x i=%d\n", g_Jmp.esp, pv, g_cbFooUsed, cb, i), -15);
#endif

    /* Twice in a row, every 7th time. */
    if ((i % 7) <= 1)
    {
        g_cJmps++;
        int rc = vmmR0CallRing3LongJmp(&g_Jmp, 42);
        if (!rc)
            return i + 10000;
        return -1;
    }
    NOREF(iMinusOne);
    return i;
}


DECLCALLBACK(int) tst2(intptr_t i, intptr_t i2)
{
    RTTESTI_CHECK_MSG_RET(i >= 0 && i <= 8192, ("i=%d is out of range [0..8192]\n", i),      1);
    RTTESTI_CHECK_MSG_RET(i2 == 0,             ("i2=%d is out of range [0]\n", i2),          1);
    int iExpect = (i % 7) <= 1 ? i + 10000 : i;
    int rc = foo(i, 0, -1);
    RTTESTI_CHECK_MSG_RET(rc == iExpect,       ("i=%d rc=%d expected=%d\n", i, rc, iExpect), 1);
    return 0;
}


DECLCALLBACK(DECL_NO_INLINE(RT_NOTHING, int)) stackRandom(PVMMR0JMPBUF pJmpBuf, PFNVMMR0SETJMP pfn, PVM pVM, PVMCPU pVCpu)
{
#ifdef RT_ARCH_AMD64
    uint32_t            cbRand  = RTRandU32Ex(1, 96);
#else
    uint32_t            cbRand  = 1;
#endif
    uint8_t volatile   *pabFuzz = (uint8_t volatile *)alloca(cbRand);
    memset((void *)pabFuzz, 0xfa, cbRand);
    int rc = vmmR0CallRing3SetJmp(pJmpBuf, pfn, pVM, pVCpu);
    memset((void *)pabFuzz, 0xaf, cbRand);
    return rc;
}


void tst(int iFrom, int iTo, int iInc)
{
#ifdef VMM_R0_SWITCH_STACK
    int const cIterations = iFrom > iTo ? iFrom - iTo : iTo - iFrom;
    void   *pvPrev = alloca(1);
#endif

    RTR0PTR R0PtrSaved = g_Jmp.pvSavedStack;
    RT_ZERO(g_Jmp);
    g_Jmp.pvSavedStack = R0PtrSaved;
    memset((void *)g_Jmp.pvSavedStack, '\0', VMM_STACK_SIZE);
    g_cbFoo = 0;
    g_cJmps = 0;
    g_cbFooUsed = 0;

    for (int i = iFrom, iItr = 0; i != iTo; i += iInc, iItr++)
    {
        int rc = stackRandom(&g_Jmp, (PFNVMMR0SETJMP)tst2, (PVM)(uintptr_t)i, 0);
        RTTESTI_CHECK_MSG_RETV(rc == 0 || rc == 42, ("i=%d rc=%d setjmp; cbFoo=%#x cbFooUsed=%#x\n", i, rc, g_cbFoo, g_cbFooUsed));

#ifdef VMM_R0_SWITCH_STACK
        /* Make the stack pointer slide for the second half of the calls. */
        if (iItr >= cIterations / 2)
        {
            /* Note! gcc does funny rounding up of alloca(). */
            void  *pv2 = alloca((i % 63) | 1);
            size_t cb2 = (uintptr_t)pvPrev - (uintptr_t)pv2;
            RTTESTI_CHECK_MSG(cb2 >= 16 && cb2 <= 128, ("cb2=%zu pv2=%p pvPrev=%p iAlloca=%d\n", cb2, pv2, pvPrev, iItr));
            memset(pv2, 0xff, cb2);
            memset(pvPrev, 0xee, 1);
            pvPrev = pv2;
        }
#endif
    }
    RTTESTI_CHECK_MSG_RETV(g_cJmps, ("No jumps!"));
    if (g_Jmp.cbUsedAvg || g_Jmp.cUsedTotal)
        RTTestIPrintf(RTTESTLVL_ALWAYS, "cbUsedAvg=%#x cbUsedMax=%#x cUsedTotal=%#llx\n",
                      g_Jmp.cbUsedAvg, g_Jmp.cbUsedMax, g_Jmp.cUsedTotal);
}


int main()
{
    /*
     * Init.
     */
    RTTEST hTest;
    RTEXITCODE rcExit = RTTestInitAndCreate("tstVMMR0CallHost-1", &hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;
    RTTestBanner(hTest);

    g_Jmp.pvSavedStack = (RTR0PTR)RTTestGuardedAllocTail(hTest, VMM_STACK_SIZE);

    /*
     * Run two test with about 1000 long jumps each.
     */
    RTTestSub(hTest, "Increasing stack usage");
    tst(0, 7000, 1);
    RTTestSub(hTest, "Decreasing stack usage");
    tst(7599, 0, -1);

    return RTTestSummaryAndDestroy(hTest);
}
