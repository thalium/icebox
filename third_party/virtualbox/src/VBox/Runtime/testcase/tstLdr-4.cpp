/* $Id: tstLdr-4.cpp $ */
/** @file
 * IPRT - Testcase for RTLdrOpen using ldrLdrObjR0.r0.
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
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/ldr.h>
#include <iprt/alloc.h>
#include <iprt/log.h>
#include <iprt/stream.h>
#include <iprt/assert.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/initterm.h>
#include <iprt/err.h>
#include <iprt/string.h>


extern "C" DECLEXPORT(int) DisasmTest1(void);


/**
 * Resolve an external symbol during RTLdrGetBits().
 *
 * @returns iprt status code.
 * @param   hLdrMod         The loader module handle.
 * @param   pszModule       Module name.
 * @param   pszSymbol       Symbol name, NULL if uSymbol should be used.
 * @param   uSymbol         Symbol ordinal, ~0 if pszSymbol should be used.
 * @param   pValue          Where to store the symbol value (address).
 * @param   pvUser          User argument.
 */
static DECLCALLBACK(int) testGetImport(RTLDRMOD hLdrMod, const char *pszModule, const char *pszSymbol, unsigned uSymbol, RTUINTPTR *pValue, void *pvUser)
{
    RT_NOREF4(hLdrMod, pszModule, uSymbol, pvUser);
    if (     !strcmp(pszSymbol, "RTAssertMsg1Weak")     || !strcmp(pszSymbol, "_RTAssertMsg1Weak"))
        *pValue = (uintptr_t)RTAssertMsg1Weak;
    else if (!strcmp(pszSymbol, "RTAssertMsg2Weak")     || !strcmp(pszSymbol, "_RTAssertMsg2Weak"))
        *pValue = (uintptr_t)RTAssertMsg1Weak;
    else if (!strcmp(pszSymbol, "RTAssertMsg1")         || !strcmp(pszSymbol, "_RTAssertMsg1"))
        *pValue = (uintptr_t)RTAssertMsg1;
    else if (!strcmp(pszSymbol, "RTAssertMsg2")         || !strcmp(pszSymbol, "_RTAssertMsg2"))
        *pValue = (uintptr_t)RTAssertMsg2;
    else if (!strcmp(pszSymbol, "RTAssertMsg2V")        || !strcmp(pszSymbol, "_RTAssertMsg2V"))
        *pValue = (uintptr_t)RTAssertMsg2V;
    else if (!strcmp(pszSymbol, "RTAssertMayPanic")     || !strcmp(pszSymbol, "_RTAssertMayPanic"))
        *pValue = (uintptr_t)RTAssertMayPanic;
    else if (!strcmp(pszSymbol, "RTLogDefaultInstanceEx") || !strcmp(pszSymbol, "RTLogDefaultInstanceEx"))
        *pValue = (uintptr_t)RTLogDefaultInstanceEx;
    else if (!strcmp(pszSymbol, "RTLogLoggerExV")       || !strcmp(pszSymbol, "_RTLogLoggerExV"))
        *pValue = (uintptr_t)RTLogLoggerExV;
    else if (!strcmp(pszSymbol, "RTLogPrintfV")         || !strcmp(pszSymbol, "_RTLogPrintfV"))
        *pValue = (uintptr_t)RTLogPrintfV;
    else if (!strcmp(pszSymbol, "RTR0AssertPanicSystem")|| !strcmp(pszSymbol, "_RTR0AssertPanicSystem"))
        *pValue = (uintptr_t)0;
    else if (!strcmp(pszSymbol, "MyPrintf")             || !strcmp(pszSymbol, "_MyPrintf"))
        *pValue = (uintptr_t)RTPrintf;
    else if (!strcmp(pszSymbol, "SomeImportFunction")   || !strcmp(pszSymbol, "_SomeImportFunction"))
        *pValue = (uintptr_t)0;
    else
    {
        RTPrintf("tstLdr-4: Unexpected import '%s'!\n", pszSymbol);
        return VERR_SYMBOL_NOT_FOUND;
    }
    return VINF_SUCCESS;
}


/**
 * One test iteration with one file.
 *
 * The test is very simple, we load the file three times
 * into two different regions. The first two into each of the
 * regions the for compare usage. The third is loaded into one
 * and then relocated between the two and other locations a few times.
 *
 * @returns number of errors.
 * @param   pszFilename     The file to load the mess with.
 */
static int testLdrOne(const char *pszFilename)
{
    int             cErrors = 0;
    size_t          cbImage = 0;
    struct Load
    {
        RTLDRMOD    hLdrMod;
        void       *pvBits;
        size_t      cbBits;
        const char *pszName;
    }   aLoads[6] =
    {
        { NULL, NULL, 0, "foo" },
        { NULL, NULL, 0, "bar" },
        { NULL, NULL, 0, "foobar" },
        { NULL, NULL, 0, "kLdr-foo" },
        { NULL, NULL, 0, "kLdr-bar" },
        { NULL, NULL, 0, "kLdr-foobar" }
    };
    unsigned i;
    int rc;

    /*
     * Load them.
     */
    for (i = 0; i < RT_ELEMENTS(aLoads); i++)
    {
        if (!strncmp(aLoads[i].pszName, RT_STR_TUPLE("kLdr-")))
        {
            rc = RTLdrOpenkLdr(pszFilename, 0, RTLDRARCH_WHATEVER, &aLoads[i].hLdrMod);
            if (rc == VERR_ELF_EXE_NOT_SUPPORTED)
                continue;
        }
        else
            rc = RTLdrOpen(pszFilename, 0, RTLDRARCH_WHATEVER, &aLoads[i].hLdrMod);
        if (RT_FAILURE(rc))
        {
            RTPrintf("tstLdr-4: Failed to open '%s'/%d, rc=%Rrc. aborting test.\n", pszFilename, i, rc);
            Assert(aLoads[i].hLdrMod == NIL_RTLDRMOD);
            cErrors++;
            break;
        }

        /* size it */
        size_t cb = RTLdrSize(aLoads[i].hLdrMod);
        if (cbImage && cb != cbImage)
        {
            RTPrintf("tstLdr-4: Size mismatch '%s'/%d. aborting test.\n", pszFilename, i);
            cErrors++;
            break;
        }
        aLoads[i].cbBits = cbImage = cb;

        /* Allocate bits. */
        aLoads[i].pvBits = RTMemExecAlloc(cb);
        if (!aLoads[i].pvBits)
        {
            RTPrintf("tstLdr-4: Out of memory '%s'/%d cbImage=%d. aborting test.\n", pszFilename, i, cbImage);
            cErrors++;
            break;
        }

        /* Get the bits. */
        rc = RTLdrGetBits(aLoads[i].hLdrMod, aLoads[i].pvBits, (uintptr_t)aLoads[i].pvBits, testGetImport, NULL);
        if (RT_FAILURE(rc))
        {
            RTPrintf("tstLdr-4: Failed to get bits for '%s'/%d, rc=%Rrc. aborting test\n", pszFilename, i, rc);
            cErrors++;
            break;
        }
    }

    /*
     * Execute the code.
     */
    if (!cErrors)
    {
        for (i = 0; i < RT_ELEMENTS(aLoads); i += 1)
        {
            /* VERR_ELF_EXE_NOT_SUPPORTED in the previous loop? */
            if (!aLoads[i].hLdrMod)
                continue;
            /* get the pointer. */
            RTUINTPTR Value;
            rc = RTLdrGetSymbolEx(aLoads[i].hLdrMod, aLoads[i].pvBits, (uintptr_t)aLoads[i].pvBits,
                                  UINT32_MAX, "DisasmTest1", &Value);
            if (rc == VERR_SYMBOL_NOT_FOUND)
                rc = RTLdrGetSymbolEx(aLoads[i].hLdrMod, aLoads[i].pvBits, (uintptr_t)aLoads[i].pvBits,
                                      UINT32_MAX, "_DisasmTest1", &Value);
            if (RT_FAILURE(rc))
            {
                RTPrintf("tstLdr-4: Failed to get symbol \"DisasmTest1\" from load #%d: %Rrc\n", i, rc);
                cErrors++;
                break;
            }
            DECLCALLBACKPTR(int, pfnDisasmTest1)(void) = (DECLCALLBACKPTR(int, RT_NOTHING)(void))(uintptr_t)Value; /* eeeh. */
            RTPrintf("tstLdr-4: pfnDisasmTest1=%p / add-symbol-file %s %#x\n", pfnDisasmTest1, pszFilename, aLoads[i].pvBits);

            /* call the test function. */
            rc = pfnDisasmTest1();
            if (rc)
            {
                RTPrintf("tstLdr-4: load #%d Test1 -> %#x\n", i, rc);
                cErrors++;
            }

            /* While we're here, check a couple of RTLdrQueryProp calls too */
            void *pvBits = aLoads[i].pvBits;
            for (unsigned iBits = 0; iBits < 2; iBits++, pvBits = NULL)
            {
                union
                {
                    char szName[127];
                } uBuf;
                rc = RTLdrQueryPropEx(aLoads[i].hLdrMod, RTLDRPROP_INTERNAL_NAME, aLoads[i].pvBits,
                                      uBuf.szName, sizeof(uBuf.szName), NULL);
                if (RT_SUCCESS(rc))
                    RTPrintf("tstLdr-4: internal name #%d: '%s'\n", i, uBuf.szName);
                else if (rc != VERR_NOT_FOUND && rc != VERR_NOT_SUPPORTED)
                    RTPrintf("tstLdr-4: internal name #%d failed: %Rrc\n", i, rc);
            }
        }
    }

    /*
     * Clean up.
     */
    for (i = 0; i < RT_ELEMENTS(aLoads); i++)
    {
        if (aLoads[i].pvBits)
            RTMemExecFree(aLoads[i].pvBits, aLoads[i].cbBits);
        if (aLoads[i].hLdrMod)
        {
            rc = RTLdrClose(aLoads[i].hLdrMod);
            if (RT_FAILURE(rc))
            {
                RTPrintf("tstLdr-4: Failed to close '%s' i=%d, rc=%Rrc.\n", pszFilename, i, rc);
                cErrors++;
            }
        }
    }

    return cErrors;
}



int main(int argc, char **argv)
{
    int cErrors = 0;
    RTR3InitExe(argc, &argv, 0);

    /*
     * Sanity check.
     */
    int rc = DisasmTest1();
    if (rc)
    {
        RTPrintf("tstLdr-4: FATAL ERROR - DisasmTest1 is buggy: rc=%#x\n", rc);
        return 1;
    }

    /*
     * Execute the test.
     */
    char szPath[RTPATH_MAX];
    rc = RTPathExecDir(szPath, sizeof(szPath) - sizeof("/tstLdrObjR0.r0"));
    if (RT_SUCCESS(rc))
    {
        strcat(szPath, "/tstLdrObjR0.r0");
        RTPrintf("tstLdr-4: TESTING '%s'...\n", szPath);
        cErrors += testLdrOne(szPath);
    }
    else
    {
        RTPrintf("tstLdr-4: RTPathExecDir -> %Rrc\n", rc);
        cErrors++;
    }

    /*
     * Test result summary.
     */
    if (!cErrors)
        RTPrintf("tstLdr-4: SUCCESS\n");
    else
        RTPrintf("tstLdr-4: FAILURE - %d errors\n", cErrors);
    return !!cErrors;
}
