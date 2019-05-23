/* $Id: tst-0-driver.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kLdr - Dynamic Loader testcase no. 0, Driver.
 */

/*
 * Copyright (c) 2006-2007 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "tst.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** Select the appropriate KLDRSYMKIND bit define. */
#define MY_KLDRSYMKIND_BITS     ( sizeof(void *) == 4 ? KLDRSYMKIND_32BIT : KLDRSYMKIND_64BIT )


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** The numbers of errors. */
static int g_cErrors = 0;



/**
 * Report failure.
 */
static int Failure(const char *pszFormat, ...)
{
    va_list va;

    g_cErrors++;

    printf("tst-0-driver: ");
    va_start(va, pszFormat);
    vprintf(pszFormat, va);
    va_end(va);
    printf("\n");
    return 1;
}


int main(int argc, char **argv)
{
    const char *pszErrInit = "Error, szErr wasn't zapped";
    char szErr[512];
    char szBuf[512];
    char *psz;
    KSIZE cch;
    HKLDRMOD hMod;
    int rc;

    /*
     * The first thing to do is a simple load / unload test
     * using the tst-0-a library (it'll drag in tst-0-d).
     */
    printf("tst-0-driver: Basic API test using 'tst-0-a'...\n");
    hMod = (HKLDRMOD)0xffffeeee;
    strcpy(szErr, pszErrInit);
    rc = kLdrDyldLoad("tst-0-a", NULL, NULL, KLDRDYLD_SEARCH_HOST,
                      KLDRDYLD_LOAD_FLAGS_RECURSIVE_INIT, &hMod, szErr, sizeof(szErr));
    if (rc)
        Failure("kLdrDyldLoad(\"tst-0\",...) failed, rc=%d (%#x). szErr='%s'.\n", rc, rc, szErr);
    if (!strcmp(szErr, pszErrInit))
        Failure("szErr wasn't set.\n");
    if (hMod == (HKLDRMOD)0xffffeeee)
        Failure("hMod wasn't set.\n");
    if (hMod == NIL_HKLDRMOD && !rc)
        Failure("rc=0 but hMod=NIL_HKLDRMOD\n");
    if (!rc)
    {
        HKLDRMOD hMod2;
        HKLDRMOD hMod3;
        printf("tst-0-driver: hMod=%p ('tst-0-a')\n", (void *)hMod);

        /*
         * Simple test of kLdrDyldFindByName.
         */
        hMod2 = (HKLDRMOD)0xffffeeee;
        rc = kLdrDyldFindByName("tst-0", NULL, NULL, KLDRDYLD_SEARCH_HOST, 0, &hMod2);
        if (!rc)
            Failure("kLdrDyldFindByName(\"tst-0\",,,) didn't fail!\n");
        if (rc && hMod2 != NIL_HKLDRMOD)
            Failure("hMod2 wasn't set correctly on kLdrDyldFindByName failure!\n");

        hMod2 = (HKLDRMOD)0xffffeeee;
        rc = kLdrDyldFindByName("tst-0-a", NULL, NULL, KLDRDYLD_SEARCH_HOST, 0, &hMod2);
        if (rc)
            Failure("kLdrDyldFindByName(\"tst-0-a\",,,) failed, rc=%d (%#x)\n", rc, rc);
        if (!rc && hMod2 != hMod)
            Failure("kLdrDyldFindByName(\"tst-0-a\",,,) returned the wrong module handle: %p instead of %p\n",
                    (void *)hMod2, (void *)hMod);

        hMod2 = (HKLDRMOD)0xffffeeee;
        rc = kLdrDyldFindByName("tst-0-d", NULL, NULL, KLDRDYLD_SEARCH_HOST, 0, &hMod2);
        if (!rc)
            printf("tst-0-driver: hMod2=%p ('tst-0-d')\n", (void *)hMod2);
        else
            Failure("kLdrDyldFindByName(\"tst-0-d\",,,) failed, rc=%d (%#x)\n", rc, rc);

        /*
         * Get the name and filename for each of the two modules.
         */
        rc = kLdrDyldGetName(hMod2, szBuf, sizeof(szBuf));
        if (!rc)
        {
            printf("tst-0-driver: name: '%s' ('tst-0-d')\n", szBuf);
            psz = strstr(szBuf, "-0-");
            if (    !psz
                ||  strnicmp(psz, "-0-d", sizeof("-0-d") - 1))
                Failure("kLdrDyldGetName(\"tst-0-d\",,,) -> '%s': pattern '-0-d' not found\n", szBuf);

            /* overflow test. */
            cch = strlen(szBuf);
            szBuf[cch + 1] = szBuf[cch] = szBuf[cch - 1] = 'x';
            szBuf[cch + 2] = '\0';
            rc = kLdrDyldGetName(hMod2, szBuf, cch);
            if (rc == KERR_BUFFER_OVERFLOW)
            {
                if (!szBuf[0])
                    Failure("kLdrDyldGetName didn't return partial result on overflow\n");
                else if (szBuf[cch - 1])
                    Failure("kLdrDyldGetName didn't terminate partial result correctly overflow: '%s'\n", szBuf);
                else if (szBuf[cch] != 'x')
                    Failure("kLdrDyldGetName exceeded the buffer limit on partial overflow: '%s'\n", szBuf);
            }
            else
                Failure("kLdrDyldGetName(\"tst-0-d\",,,) -> rc=%d (%#x) instead of KERR_BUFFER_OVERFLOW\n", rc, rc);

            /* check that we can query the module by the returned name. */
            rc = kLdrDyldGetName(hMod2, szBuf, sizeof(szBuf));
            if (!rc)
            {
                hMod3 = (HKLDRMOD)0xffffeeee;
                rc = kLdrDyldFindByName(szBuf, NULL, NULL, KLDRDYLD_SEARCH_HOST, 0, &hMod3);
                if (rc || hMod3 != hMod2)
                    Failure("kLdrDyldFindByName(\"%s\",,,) failed, rc=%d (%#x) hMod3=%p hMod2=%p\n",
                            szBuf, rc, rc, (void *)hMod3, (void *)hMod2);
            }
            else
                Failure("kLdrDyldGetName(\"tst-0-d\",,,) failed (b), rc=%d (%#x)\n", rc, rc);
        }
        else
            Failure("kLdrDyldGetName(\"tst-0-d\",,,) failed, rc=%d (%#x)\n", rc, rc);

        rc = kLdrDyldGetFilename(hMod2, szBuf, sizeof(szBuf));
        if (!rc)
        {
            printf("tst-0-driver: filename: '%s' ('tst-0-d')\n", szBuf);

            /* overflow test. */
            cch = strlen(szBuf);
            szBuf[cch + 1] = szBuf[cch] = szBuf[cch - 1] = 'x';
            szBuf[cch + 2] = '\0';
            rc = kLdrDyldGetFilename(hMod2, szBuf, cch);
            if (rc == KERR_BUFFER_OVERFLOW)
            {
                if (!szBuf[0])
                    Failure("kLdrDyldGetFilename didn't return partial result on overflow\n");
                else if (szBuf[cch - 1])
                    Failure("kLdrDyldGetFilename didn't terminate partial result correctly overflow: '%s'\n", szBuf);
                else if (szBuf[cch] != 'x')
                    Failure("kLdrDyldGetFilename exceeded the buffer limit on partial overflow: '%s'\n", szBuf);
            }
            else
                Failure("kLdrDyldGetFilename(\"tst-0-d\",,,) -> rc=%d (%#x) instead of KERR_BUFFER_OVERFLOW\n", rc, rc);

            /* check that we can query the module by the returned filename. */
            rc = kLdrDyldGetFilename(hMod2, szBuf, sizeof(szBuf));
            if (!rc)
            {
                hMod3 = (HKLDRMOD)0xffffeeee;
                rc = kLdrDyldFindByName(szBuf, NULL, NULL, KLDRDYLD_SEARCH_HOST, 0, &hMod3);
                if (rc || hMod3 != hMod2)
                    Failure("kLdrDyldFindByName(\"%s\",,,) failed, rc=%d (%#x) hMod3=%p hMod2=%p\n",
                            szBuf, rc, rc, (void *)hMod3, (void *)hMod2);
            }
            else
                Failure("kLdrDyldGetName(\"tst-0-d\",,,) failed (b), rc=%d (%#x)\n", rc, rc);
        }
        else
            Failure("kLdrDyldGetFilename(\"tst-0-d\",,,) failed, rc=%d (%#x)\n", rc, rc);

        /* the other module */
        rc = kLdrDyldGetName(hMod, szBuf, sizeof(szBuf));
        if (!rc)
        {
            printf("tst-0-driver: name: '%s' ('tst-0-a')\n", szBuf);
            psz = strstr(szBuf, "-0-");
            if (    !psz
                ||  strnicmp(psz, "-0-a", sizeof("-0-a") - 1))
                Failure("kLdrDyldGetName(\"tst-0-a\",,,) -> '%s': pattern '-0-a' not found\n", szBuf);

            /* check that we can query the module by the returned name. */
            hMod3 = (HKLDRMOD)0xffffeeee;
            rc = kLdrDyldFindByName(szBuf, NULL, NULL, KLDRDYLD_SEARCH_HOST, 0, &hMod3);
            if (rc || hMod3 != hMod)
                Failure("kLdrDyldFindByName(\"%s\",,,) failed, rc=%d (%#x) hMod3=%p hMod=%p\n",
                        szBuf, rc, rc, (void *)hMod3, (void *)hMod);
        }
        else
            Failure("kLdrDyldGetName(\"tst-0-a\",,,) failed, rc=%d (%#x)\n", rc, rc);

        rc = kLdrDyldGetFilename(hMod, szBuf, sizeof(szBuf));
        if (!rc)
        {
            printf("tst-0-driver: filename: '%s' ('tst-0-a')\n", szBuf);

            /* check that we can query the module by the returned filename. */
            hMod3 = (HKLDRMOD)0xffffeeee;
            rc = kLdrDyldFindByName(szBuf, NULL, NULL, KLDRDYLD_SEARCH_HOST, 0, &hMod3);
            if (rc || hMod3 != hMod)
                Failure("kLdrDyldFindByName(\"%s\",,,) failed, rc=%d (%#x) hMod3=%p hMod=%p\n",
                        szBuf, rc, rc, (void *)hMod3, (void *)hMod);
        }
        else
            Failure("kLdrDyldGetFilename(\"tst-0-a\",,,) failed, rc=%d (%#x)\n", rc, rc);


        /*
         * Resolve the symbol exported by each of the two modules and call them.
         */
        if (!g_cErrors)
        {
            KUPTR uValue;
            KU32  fKind;

            fKind = 0xffeeffee;
            uValue = ~(KUPTR)42;
            rc = kLdrDyldQuerySymbol(hMod, NIL_KLDRMOD_SYM_ORDINAL, MY_NAME("FuncA"), NULL, &uValue, &fKind);
            if (!rc)
            {
                if (uValue == ~(KUPTR)42)
                    Failure("kLdrDyldQuerySymbol(\"tst-0-a\",,\"FuncA\",): uValue wasn't set.\n");
                if (fKind == 0xffeeffee)
                    Failure("kLdrDyldQuerySymbol(\"tst-0-a\",,\"FuncA\",): fKind wasn't set.\n");
                if (    (fKind & KLDRSYMKIND_BIT_MASK) != KLDRSYMKIND_NO_BIT
                    &&  (fKind & KLDRSYMKIND_BIT_MASK) != MY_KLDRSYMKIND_BITS)
                    Failure("fKind=%#x indicates a different code 'bit' mode than we running at.\n", fKind);
                if (    (fKind & KLDRSYMKIND_TYPE_MASK) != KLDRSYMKIND_NO_TYPE
                    &&  (fKind & KLDRSYMKIND_TYPE_MASK) != KLDRSYMKIND_CODE)
                    Failure("fKind=%#x indicates that \"FuncA\" isn't code.\n", fKind);
                if (fKind & KLDRSYMKIND_FORWARDER)
                    Failure("fKind=%#x indicates that \"FuncA\" is a forwarder. it isn't.\n", fKind);

                /* call it. */
                if (!g_cErrors)
                {
                    int (*pfnFuncA)(void) = (int (*)(void))uValue;
                    rc = pfnFuncA();
                    if (rc != 0x42000042)
                        Failure("FuncA returned %#x expected 0x42000042\n", rc);
                }

                /*
                 * Test kLdrDyldFindByAddress now that we've got an address.
                 */
                hMod3 = (HKLDRMOD)0xeeeeffff;
                rc = kLdrDyldFindByAddress(uValue, &hMod3, NULL, NULL);
                if (!rc)
                {
                    KUPTR offSegment;
                    KU32 iSegment;

                    if (hMod3 != hMod)
                        Failure("kLdrDyldFindByAddress(%#p/*FuncA*/, \"tst-0-a\",,,) return incorrect hMod3=%p instead of %p.\n",
                                uValue, hMod3, hMod);

                    hMod3 = (HKLDRMOD)0xeeeeffff;
                    iSegment = 0x42424242;
                    rc = kLdrDyldFindByAddress(uValue, &hMod3, &iSegment, &offSegment);
                    if (!rc)
                    {
                        if (hMod3 != hMod)
                            Failure("Bad hMod3 on 2nd kLdrDyldFindByAddress call.\n");
                        if (iSegment > 0x1000) /* safe guess */
                            Failure("Bad iSegment=%#x\n", iSegment);
                        if (offSegment > 0x100000) /* guesswork */
                            Failure("Bad offSegment=%p\n", (void *)offSegment);
                    }
                    else
                        Failure("kLdrDyldFindByAddress(%#p/*FuncA*/, \"tst-0-a\",,,) failed (b), rc=%d (%#x)\n",
                                uValue, rc, rc);

                    /* negative test */
                    hMod3 = (HKLDRMOD)0xeeeeffff;
                    iSegment = 0x42424242;
                    offSegment = 0x87654321;
                    rc = kLdrDyldFindByAddress(~(KUPTR)16, &hMod3, &iSegment, &offSegment);
                    if (!rc)
                        Failure("negative kLdrDyldFindByAddress test returned successfully!\n");
                    if (iSegment != ~(KU32)0)
                        Failure("negative kLdrDyldFindByAddress: bad iSegment=%#x\n", iSegment);
                    if (offSegment != ~(KUPTR)0)
                        Failure("negative kLdrDyldFindByAddress: bad offSegment=%p\n", (void *)offSegment);
                    if (hMod3 != NIL_HKLDRMOD)
                        Failure("negative kLdrDyldFindByAddress: bad hMod3=%p\n", (void *)hMod3);
                }
                else
                    Failure("kLdrDyldFindByAddress(%#p/*FuncA*/, \"tst-0-a\",,,) failed, rc=%d (%#x)\n",
                            uValue, rc, rc);
            }
            else
                Failure("kLdrDyldQuerySymbol(\"tst-0-a\",,\"FuncA\",) failed, rc=%d (%#x)\n", rc, rc);

            fKind = 0xffeeffee;
            uValue = ~(KUPTR)42;
            rc = kLdrDyldQuerySymbol(hMod2, NIL_KLDRMOD_SYM_ORDINAL, MY_NAME("FuncD"), NULL, &uValue, &fKind);
            if (!rc)
            {
                if (uValue == ~(KUPTR)42)
                    Failure("kLdrDyldQuerySymbol(\"tst-0-d\",,\"FuncD\",): uValue wasn't set.\n");
                if (fKind == 0xffeeffee)
                    Failure("kLdrDyldQuerySymbol(\"tst-0-d\",,\"FuncD\",): fKind wasn't set.\n");
                if (    (fKind & KLDRSYMKIND_BIT_MASK) != KLDRSYMKIND_NO_BIT
                    &&  (fKind & KLDRSYMKIND_BIT_MASK) != MY_KLDRSYMKIND_BITS)
                    Failure("fKind=%#x indicates a different code 'bit' mode than we running at.\n", fKind);
                if (    (fKind & KLDRSYMKIND_TYPE_MASK) != KLDRSYMKIND_NO_TYPE
                    &&  (fKind & KLDRSYMKIND_TYPE_MASK) != KLDRSYMKIND_CODE)
                    Failure("fKind=%#x indicates that \"FuncD\" isn't code.\n", fKind);
                if (fKind & KLDRSYMKIND_FORWARDER)
                    Failure("fKind=%#x indicates that \"FuncD\" is a forwarder. it isn't.\n", fKind);

                /* call it. */
                if (!g_cErrors)
                {
                    int (*pfnFuncD)(void) = (int (*)(void))uValue;
                    rc = pfnFuncD();
                    if (rc != 0x42000000)
                        Failure("FuncD returned %#x expected 0x42000000\n", rc);
                }

                /* use the address to get the module handle. */
                hMod3 = (HKLDRMOD)0xeeeeffff;
                rc = kLdrDyldFindByAddress(uValue, &hMod3, NULL, NULL);
                if (!rc)
                {
                    if (hMod3 != hMod2)
                        Failure("kLdrDyldFindByAddress(%#p/*FuncD*/,,,) return incorrect hMod3=%p instead of %p.\n",
                                uValue, hMod3, hMod2);
                }
                else
                    Failure("kLdrDyldFindByAddress(%#p/*FuncD*/,,,) failed, rc=%d (%#x)\n",
                            uValue, rc, rc);
            }
            else
                Failure("kLdrDyldQuerySymbol(\"tst-0-a\",,\"FuncA\",) failed, rc=%d (%#x)\n", rc, rc);

        }

        /*
         * Finally unload it.
         */
        rc = kLdrDyldUnload(hMod);
        if (rc)
            Failure("kLdrDyldUnload() failed. rc=%d (%#x)\n", rc, rc);
        if (!rc)
        {
            rc = kLdrDyldFindByName("tst-0-d", NULL, NULL, KLDRDYLD_SEARCH_HOST, 0, &hMod2);
            if (rc != KLDR_ERR_MODULE_NOT_FOUND)
                Failure("kLdrDyldFindByName(\"tst-0-d\",,,) return rc=%d (%#x), expected KLDR_ERR_MODULE_NOT_FOUND\n", rc, rc);

            rc = kLdrDyldFindByName("tst-0-a", NULL, NULL, KLDRDYLD_SEARCH_HOST, 0, &hMod2);
            if (rc != KLDR_ERR_MODULE_NOT_FOUND)
                Failure("kLdrDyldFindByName(\"tst-0-a\",,,) return rc=%d (%#x), expected KLDR_ERR_MODULE_NOT_FOUND\n", rc, rc);
        }
    }

    /*
     * Now do what tst-0 would do; load the three dlls, resolve and call their functions.
     */
    if (!g_cErrors)
    {
        HKLDRMOD hModA;
        int (*pfnFuncA)(void);
        HKLDRMOD hModB;
        int (*pfnFuncB)(void);
        HKLDRMOD hModC;
        int (*pfnFuncC)(void);
        KUPTR uValue;

        rc = kLdrDyldLoad("tst-0-a", NULL, NULL, KLDRDYLD_SEARCH_HOST, 0, &hModA, NULL, 0);
        if (rc)
            Failure("kLdrDyldLoad(\"tst-0-a\",,,,) -> %d (%#x)\n", rc, rc);
        if (!rc)
        {
            rc = kLdrDyldLoad("tst-0-b", NULL, NULL, KLDRDYLD_SEARCH_HOST, 0, &hModB, szErr, sizeof(szErr));
            if (rc)
                Failure("kLdrDyldLoad(\"tst-0-b\",,,,) -> %d (%#x) szErr='%s'\n", rc, rc, szErr);
        }
        if (!rc)
        {
            rc = kLdrDyldLoad("tst-0-c", NULL, NULL, KLDRDYLD_SEARCH_HOST, 0, &hModC, szErr, sizeof(szErr));
            if (rc)
                Failure("kLdrDyldLoad(\"tst-0-c\",,,,) -> %d (%#x) szErr='%s'\n", rc, rc, szErr);
        }
        if (!rc)
        {
            rc = kLdrDyldQuerySymbol(hModA, NIL_KLDRMOD_SYM_ORDINAL, MY_NAME("FuncA"), NULL, &uValue, NULL);
            if (!rc)
                pfnFuncA = (int (*)(void))uValue;
            else
                Failure("kLdrDyldQuerySymbol(,,\"FuncA\",,) -> %d (%#x)\n", rc, rc);
        }
        if (!rc)
        {
            rc = kLdrDyldQuerySymbol(hModB, NIL_KLDRMOD_SYM_ORDINAL, MY_NAME("FuncB"), NULL, &uValue, NULL);
            if (!rc)
                pfnFuncB = (int (*)(void))uValue;
            else
                Failure("kLdrDyldQuerySymbol(,,\"FuncB\",,) -> %d (%#x)\n", rc, rc);
        }
        if (!rc)
        {
            rc = kLdrDyldQuerySymbol(hModC, NIL_KLDRMOD_SYM_ORDINAL, MY_NAME("FuncC"), NULL, &uValue, NULL);
            if (!rc)
                pfnFuncC = (int (*)(void))uValue;
            else
                Failure("kLdrDyldQuerySymbol(,,\"FuncA\",,) -> %d (%#x)\n", rc, rc);
        }
        if (!rc)
        {
            int u = pfnFuncA() | pfnFuncB() | pfnFuncC();
            if (u == 0x42424242)
                printf("tst-0-driver: FuncA/B/C => %#x (correct)\n", u);
            else
                Failure("FuncA/B/C => %#x\n", u);

            rc = kLdrDyldUnload(hModA);
            if (rc)
                Failure("Unload A failed, rc=%d (%#x)\n", rc, rc);
            u = pfnFuncB() | pfnFuncC();
            if (u != 0x42424200)
                Failure("FuncB/C returns %#x instead of 0x42424200 after unloading A\n", u);

            rc = kLdrDyldUnload(hModB);
            if (rc)
                Failure("Unload B failed, rc=%d (%#x)\n", rc, rc);
            u = pfnFuncC();
            if (u != 0x42420000)
                Failure("FuncC returns %#x instead of 0x42420000 after unloading A\n", u);

            rc = kLdrDyldUnload(hModC);
            if (rc)
                Failure("Unload C failed, rc=%d (%#x)\n", rc, rc);

            rc = kLdrDyldFindByName("tst-0-d", NULL, NULL, KLDRDYLD_SEARCH_HOST, 0, &hMod);
            if (rc != KLDR_ERR_MODULE_NOT_FOUND)
                Failure("Query for \"tst-0-d\" after unloading A,B and C returns rc=%d (%#x) instead of KLDR_ERR_MODULE_NOT_FOUND\n",
                        rc, rc);
        }
    }

    /*
     * Now invoke the executable stub which launches the tst-0 program.
     */
    if (!g_cErrors)
    {
        /// @todo
    }

    /*
     * Summary
     */
    if (!g_cErrors)
        printf("tst-0-driver: SUCCESS\n");
    else
        printf("tst-0-driver: FAILURE - %d errors\n", g_cErrors);
    return !!g_cErrors;
}

