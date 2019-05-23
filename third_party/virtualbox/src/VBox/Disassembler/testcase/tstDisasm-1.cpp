/* $Id: tstDisasm-1.cpp $ */
/** @file
 * VBox disassembler - Test application
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
#include <VBox/dis.h>
#include <iprt/test.h>
#include <iprt/ctype.h>
#include <iprt/string.h>
#include <iprt/err.h>
#include <iprt/time.h>


DECLASM(int) TestProc32(void);
DECLASM(int) TestProc32_EndProc(void);
#ifndef RT_OS_OS2
DECLASM(int) TestProc64(void);
DECLASM(int) TestProc64_EndProc(void);
#endif
//uint8_t aCode16[] = { 0x66, 0x67, 0x89, 0x07 };

static void testDisas(const char *pszSub, uint8_t const *pabInstrs, uintptr_t uEndPtr, DISCPUMODE enmDisCpuMode)
{
    RTTestISub(pszSub);
    size_t const cbInstrs = uEndPtr - (uintptr_t)pabInstrs;
    for (size_t off = 0; off < cbInstrs;)
    {
        uint32_t const  cErrBefore = RTTestIErrorCount();
        uint32_t        cb = 1;
        DISSTATE        Dis;
        char            szOutput[256] = {0};
        int rc = DISInstrToStr(&pabInstrs[off], enmDisCpuMode, &Dis, &cb, szOutput, sizeof(szOutput));

        RTTESTI_CHECK_RC(rc, VINF_SUCCESS);
        RTTESTI_CHECK(cb == Dis.cbInstr);
        RTTESTI_CHECK(cb > 0);
        RTTESTI_CHECK(cb <= 16);
        RTStrStripR(szOutput);
        RTTESTI_CHECK(szOutput[0]);
        if (szOutput[0])
        {
            char *pszBytes = strchr(szOutput, '[');
            RTTESTI_CHECK(pszBytes);
            if (pszBytes)
            {
                RTTESTI_CHECK(pszBytes[-1] == ' ');
                RTTESTI_CHECK(RT_C_IS_XDIGIT(pszBytes[1]));
                RTTESTI_CHECK(pszBytes[cb * 3] == ']');
                RTTESTI_CHECK(pszBytes[cb * 3 + 1] == ' ');

                size_t cch = strlen(szOutput);
                RTTESTI_CHECK(szOutput[cch - 1] != ',');
            }
        }
        if (cErrBefore != RTTestIErrorCount())
            RTTestIFailureDetails("rc=%Rrc, off=%#x (%u) cbInstr=%u enmDisCpuMode=%d\n",
                                  rc, off, off, Dis.cbInstr, enmDisCpuMode);
        RTTestIPrintf(RTTESTLVL_ALWAYS, "%s\n", szOutput);

        /* Check with size-only. */
        uint32_t        cbOnly = 1;
        DISSTATE        DisOnly;
        rc = DISInstrWithPrefetchedBytes((uintptr_t)&pabInstrs[off], enmDisCpuMode,  0 /*fFilter - none */,
                                         Dis.abInstr, Dis.cbCachedInstr, NULL, NULL, &DisOnly, &cbOnly);

        RTTESTI_CHECK_RC(rc, VINF_SUCCESS);
        RTTESTI_CHECK(cbOnly == DisOnly.cbInstr);
        RTTESTI_CHECK_MSG(cbOnly == cb, ("%#x vs %#x\n", cbOnly, cb));

        off += cb;
    }
}


static DECLCALLBACK(int) testReadBytes(PDISSTATE pDis, uint8_t offInstr, uint8_t cbMinRead, uint8_t cbMaxRead)
{
    RT_NOREF1(cbMinRead);
    memcpy(&pDis->abInstr[offInstr], (void *)((uintptr_t)pDis->uInstrAddr + offInstr), cbMaxRead);
    pDis->cbCachedInstr = offInstr + cbMaxRead;
    return VINF_SUCCESS;
}


static void testPerformance(const char *pszSub, uint8_t const *pabInstrs, uintptr_t uEndPtr, DISCPUMODE enmDisCpuMode)
{
    RTTestISubF("Performance - %s", pszSub);

    size_t const    cbInstrs = uEndPtr - (uintptr_t)pabInstrs;
    uint64_t        cInstrs  = 0;
    uint64_t        nsStart  = RTTimeNanoTS();
    for (uint32_t i = 0; i < _512K; i++) /* the samples are way to small. :-) */
    {
        for (size_t off = 0; off < cbInstrs; cInstrs++)
        {
            uint32_t    cb = 1;
            DISSTATE    Dis;
            DISInstrWithReader((uintptr_t)&pabInstrs[off], enmDisCpuMode, testReadBytes, NULL, &Dis, &cb);
            off += cb;
        }
    }
    uint64_t cNsElapsed = RTTimeNanoTS() - nsStart;

    RTTestIValueF(cNsElapsed, RTTESTUNIT_NS, "%s-Total", pszSub);
    RTTestIValueF(cNsElapsed / cInstrs, RTTESTUNIT_NS_PER_CALL, "%s-per-instruction", pszSub);
}

void testTwo(void)
{
    static const struct
    {
        DISCPUMODE  enmMode;
        uint8_t     abInstr[24];
        uint8_t     cbParam1;
        uint8_t     cbParam2;
        uint8_t     cbParam3;
    } s_gInstrs[] =
    {
        { DISCPUMODE_64BIT, { 0x48, 0xc7, 0x03, 0x00, 0x00, 0x00, 0x00, },                 8, 8, 0, },
    };
    for (unsigned i = 0; i < RT_ELEMENTS(s_gInstrs); i++)
    {
        uint32_t    cb = 1;
        DISSTATE    Dis;
        int rc;
        RTTESTI_CHECK_RC(rc = DISInstr(s_gInstrs[i].abInstr, s_gInstrs[i].enmMode, &Dis, &cb), VINF_SUCCESS);
        if (rc == VINF_SUCCESS)
        {
            uint32_t cb2;
            RTTESTI_CHECK_MSG((cb2 = DISGetParamSize(&Dis, &Dis.Param1)) == s_gInstrs[i].cbParam1,
                              ("%u: %#x vs %#x\n", i , cb2, s_gInstrs[i].cbParam1));
            RTTESTI_CHECK_MSG((cb2 = DISGetParamSize(&Dis, &Dis.Param2)) == s_gInstrs[i].cbParam2,
                              ("%u: %#x vs %#x (%s)\n", i , cb2, s_gInstrs[i].cbParam2, Dis.pCurInstr->pszOpcode));
            RTTESTI_CHECK_MSG((cb2 = DISGetParamSize(&Dis, &Dis.Param3)) == s_gInstrs[i].cbParam3,
                              ("%u: %#x vs %#x\n", i , cb2, s_gInstrs[i].cbParam3));
        }
    }
}


int main(int argc, char **argv)
{
    RT_NOREF2(argc, argv);
    RTTEST hTest;
    RTEXITCODE rcExit = RTTestInitAndCreate("tstDisasm", &hTest);
    if (rcExit)
        return rcExit;
    RTTestBanner(hTest);

    static const struct
    {
        const char     *pszDesc;
        uint8_t const  *pbStart;
        uintptr_t       uEndPtr;
        DISCPUMODE      enmCpuMode;
    } aSnippets[] =
    {
        { "32-bit",     (uint8_t const *)(uintptr_t)TestProc32, (uintptr_t)&TestProc32_EndProc, DISCPUMODE_32BIT },
        { "64-bit",     (uint8_t const *)(uintptr_t)TestProc64, (uintptr_t)&TestProc64_EndProc, DISCPUMODE_64BIT },
    };

    for (unsigned i = 0; i < RT_ELEMENTS(aSnippets); i++)
        testDisas(aSnippets[i].pszDesc, aSnippets[i].pbStart, aSnippets[i].uEndPtr, aSnippets[i].enmCpuMode);

    testTwo();

    if (RTTestIErrorCount() == 0)
    {
        for (unsigned i = 0; i < RT_ELEMENTS(aSnippets); i++)
            testPerformance(aSnippets[i].pszDesc, aSnippets[i].pbStart, aSnippets[i].uEndPtr, aSnippets[i].enmCpuMode);
    }

    return RTTestSummaryAndDestroy(hTest);
}

