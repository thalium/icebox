/* $Id: tstVBInsTstR3.cpp $ */
/** @file
 * Instruction Test Environment - IPRT ring-3 driver.
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
#include <iprt/mem.h>
#include <iprt/string.h>
#include <iprt/test.h>

#ifdef RT_OS_WINDOWS
# define NO_LOW_MEM
#elif defined(RT_OS_OS2) || defined(RT_OS_HAIKU)
# define NO_LOW_MEM
#else
# include <sys/mman.h>
#endif


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
#if HC_ARCH_BITS == 64
typedef uint64_t VBINSTSTREG;
#else
typedef uint32_t VBINSTSTREG;
#endif


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
RTTEST g_hTest;


RT_C_DECLS_BEGIN
extern void *g_pvLow16Mem4K;
extern void *g_pvLow32Mem4K;
DECLASM(void)    TestInstrMain(void);

DECLEXPORT(void) VBInsTstFailure(const char *pszMessage);
DECLEXPORT(void) VBInsTstFailure1(const char *pszFmt, VBINSTSTREG uArg1);
DECLEXPORT(void) VBInsTstFailure2(const char *pszFmt, VBINSTSTREG uArg1, VBINSTSTREG uArg2);
DECLEXPORT(void) VBInsTstFailure3(const char *pszFmt, VBINSTSTREG uArg1, VBINSTSTREG uArg2, VBINSTSTREG uArg3);
DECLEXPORT(void) VBInsTstFailure4(const char *pszFmt, VBINSTSTREG uArg1, VBINSTSTREG uArg2, VBINSTSTREG uArg3, VBINSTSTREG uArg4);
RT_C_DECLS_END


DECLEXPORT(void) VBInsTstFailure(const char *pszMessage)
{
    RTTestFailed(g_hTest, "%s", pszMessage);
}

DECLEXPORT(void) VBInsTstFailure1(const char *pszFmt, VBINSTSTREG uArg1)
{
    RTTestFailed(g_hTest, pszFmt, uArg1);
}


DECLEXPORT(void) VBInsTstFailure2(const char *pszFmt, VBINSTSTREG uArg1, VBINSTSTREG uArg2)
{
    RTTestFailed(g_hTest, pszFmt, uArg1, uArg2);
}


DECLEXPORT(void) VBInsTstFailure3(const char *pszFmt, VBINSTSTREG uArg1, VBINSTSTREG uArg2, VBINSTSTREG uArg3)
{
    RTTestFailed(g_hTest, pszFmt, uArg1, uArg2, uArg3);
}


DECLEXPORT(void) VBInsTstFailure4(const char *pszFmt, VBINSTSTREG uArg1, VBINSTSTREG uArg2, VBINSTSTREG uArg3, VBINSTSTREG uArg4)
{
    RTTestFailed(g_hTest, pszFmt, uArg1, uArg2, uArg3, uArg4);
}




int main()
{
    RTEXITCODE rcExit = RTTestInitAndCreate("VBInsTstR3", &g_hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;
    RTTestBanner(g_hTest);

    int rc = RTMemAllocEx(_4K, 0, RTMEMALLOCEX_FLAGS_16BIT_REACH, &g_pvLow16Mem4K);
    if (RT_FAILURE(rc))
    {
        RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Could not allocate low 16-bit memory (%Rrc)\n", rc);
        g_pvLow16Mem4K = NULL;
    }

    rc = RTMemAllocEx(_4K, 0, RTMEMALLOCEX_FLAGS_32BIT_REACH, &g_pvLow32Mem4K);
    if (RT_FAILURE(rc))
    {
        RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Could not allocate low 32-bit memory (%Rrc)\n", rc);
        g_pvLow32Mem4K = NULL;
    }

    TestInstrMain();

    return RTTestSummaryAndDestroy(g_hTest);
}

