/* $Id: tstkLdrMod.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kLdr - Module interpreter testcase.
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
#include <k/kLdr.h>
#include <k/kErr.h>
#include <k/kErrors.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** The default base address used in the tests. */
#define MY_BASEADDRESS      0x2400000


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

    printf("tstLdrMod: ");
    va_start(va, pszFormat);
    vprintf(pszFormat, va);
    va_end(va);
    printf("\n");
    return 1;
}


/** Dummy import resolver callback. */
static int BasicTestsGetImport(PKLDRMOD pMod, KU32 iImport, KU32 iSymbol, const char *pchSymbol, KSIZE cchSymbol,
                               const char *pszVersion, PKLDRADDR puValue, KU32 *pfKind, void *pvUser)
{
    *puValue = 0xdeadface;
    *pfKind = KLDRSYMKIND_NO_BIT | KLDRSYMKIND_NO_TYPE;
    return 0;
}


/**
 * Verbose memcmp().
 */
static int TestMemComp(const void *pv1, const void *pv2, KSIZE cb)
{
    KSIZE           off;
    const KU8      *pb1 = (const KU8 *)pv1;
    const KU8      *pb2 = (const KU8 *)pv2;
    if (!memcmp(pb1, pb2, cb))
        return 0;
    printf("Mismatching blocks pv1=%p pv2=%p cb=%#x:\n", pv1, pv2, cb);
    for (off = 0; off < cb; off++)
    {
        if (pb1[off] == pb2[off])
            continue;
        printf("%08x %02x != %02x\n", off, pb1[off], pb2[off]);
    }
    return memcmp(pb1, pb2, cb); /* lazy */
}


/**
 * Performs basic relocation tests.
 */
static int BasicTestsRelocate(PKLDRMOD pMod, void *pvBits, void *pvBits2)
{
    const KSIZE cbImage = (KSIZE)kLdrModSize(pMod);
    int rc;

    printf("* Relocation test...\n");

    /*
     * Get the same bits again to check that we get the same result.
     */
    memset(pvBits2, 0xfe, cbImage);
    rc = kLdrModGetBits(pMod, pvBits2, (KUPTR)pvBits, BasicTestsGetImport, NULL);
    if (rc)
        return Failure("failed to get image bits, rc=%d (%s) (a)", rc, kErrName(rc));
    if (TestMemComp(pvBits2, pvBits, cbImage))
        return Failure("relocation test failed, mismatching bits (a)");

    /*
     * Short relocation round trip.
     */
    rc = kLdrModRelocateBits(pMod, pvBits2, 0x1000, (KUPTR)pvBits, BasicTestsGetImport, NULL);
    if (rc)
        return Failure("failed to relocate, rc=%d (%s) (b1)", rc, kErrName(rc));
    rc = kLdrModRelocateBits(pMod, pvBits2, (KUPTR)pvBits, 0x1000, BasicTestsGetImport, NULL);
    if (rc)
        return Failure("failed to relocate, rc=%d (%s) (b2)", rc, kErrName(rc));
    if (TestMemComp(pvBits2, pvBits, cbImage))
        return Failure("relocation test failed, mismatching bits (b)");

    /*
     * Longer trip where we also check the intermediate results.
     */
    /* stage one */
    rc = kLdrModRelocateBits(pMod, pvBits, 0x1000000, (KUPTR)pvBits, BasicTestsGetImport, NULL);
    if (rc)
        return Failure("failed to relocate, rc=%d (%s) (c1)", rc, kErrName(rc));
    memset(pvBits2, 0xfe, cbImage);
    rc = kLdrModGetBits(pMod, pvBits2, 0x1000000, BasicTestsGetImport, NULL);
    if (rc)
        return Failure("failed to get image bits, rc=%d (%s) (c1)", rc, kErrName(rc));
    if (TestMemComp(pvBits2, pvBits, cbImage))
        return Failure("relocation test failed, mismatching bits (c1)");

    /* stage two */
    rc = kLdrModRelocateBits(pMod, pvBits, ~(KUPTR)0x1010000, 0x1000000, BasicTestsGetImport, NULL);
    if (rc)
        return Failure("failed to relocate, rc=%d (%s) (c2)", rc, kErrName(rc));
    memset(pvBits2, 0xef, cbImage);
    rc = kLdrModGetBits(pMod, pvBits2, ~(KUPTR)0x1010000, BasicTestsGetImport, NULL);
    if (rc)
        return Failure("failed to get image bits, rc=%d (%s) (c2)", rc, kErrName(rc));
    if (TestMemComp(pvBits2, pvBits, cbImage))
        return Failure("relocation test failed, mismatching bits (c2)");

    /* stage three */
    rc = kLdrModRelocateBits(pMod, pvBits, MY_BASEADDRESS, ~(KUPTR)0x1010000, BasicTestsGetImport, NULL);
    if (rc)
        return Failure("failed to relocate, rc=%d (%s) (c3)", rc, kErrName(rc));
    memset(pvBits2, 0xef, cbImage);
    rc = kLdrModGetBits(pMod, pvBits2, MY_BASEADDRESS, BasicTestsGetImport, NULL);
    if (rc)
        return Failure("failed to get image bits, rc=%d (%s) (c3)", rc, kErrName(rc));
    if (TestMemComp(pvBits2, pvBits, cbImage))
        return Failure("relocation test failed, mismatching bits (c3)");

    /* stage four */
    rc = kLdrModRelocateBits(pMod, pvBits, ~(KUPTR)0 / 2 - 0x10000, MY_BASEADDRESS, BasicTestsGetImport, NULL);
    if (rc)
        return Failure("failed to relocate, rc=%d %(s) (c4)", rc, kErrName(rc));
    memset(pvBits2, 0xdc, cbImage);
    rc = kLdrModGetBits(pMod, pvBits2, ~(KUPTR)0 / 2 - 0x10000, BasicTestsGetImport, NULL);
    if (rc)
        return Failure("failed to get image bits, rc=%d (%s) (c4)", rc, kErrName(rc));
    if (TestMemComp(pvBits2, pvBits, cbImage))
        return Failure("relocation test failed, mismatching bits (c4)");

    /* return */
    rc = kLdrModRelocateBits(pMod, pvBits, (KUPTR)pvBits, ~(KUPTR)0 / 2 - 0x10000, BasicTestsGetImport, NULL);
    if (rc)
        return Failure("failed to relocate, rc=%d (%s) (c5)", rc, kErrName(rc));
    memset(pvBits2, 0xcd, cbImage);
    rc = kLdrModGetBits(pMod, pvBits2, (KUPTR)pvBits, BasicTestsGetImport, NULL);
    if (rc)
        return Failure("failed to get image bits, rc=%d (%s) (c5)", rc, kErrName(rc));
    if (TestMemComp(pvBits2, pvBits, cbImage))
        return Failure("relocation test failed, mismatching bits (c5)");

    return 0;
}


/**
 * Dump symbols and check that we can query each of them recursivly.
 */
static int BasicTestsEnumSymCallback(PKLDRMOD pMod, KU32 iSymbol, const char *pchSymbol, KSIZE cchSymbol,
                                     const char *pszVersion, KLDRADDR uValue, KU32 fKind, void *pvUser)
{
    KLDRADDR    uValue2;
    KU32        fKind2;
    int         rc;

    /* dump */
    printf("#0x%08x: %016" PRI_KLDRADDR " %#08x", iSymbol, uValue, fKind);
    if (pchSymbol)
        printf(" %.*s", cchSymbol, pchSymbol);
    printf("\n");

    /* query by ordinal */
    if (iSymbol != NIL_KLDRMOD_SYM_ORDINAL)
    {
        fKind2 = 0;
        rc = kLdrModQuerySymbol(pMod, pvUser, MY_BASEADDRESS, iSymbol, NULL, 0, NULL, NULL, NULL,
                                &uValue2, &fKind2);
        if (rc)
            return Failure("Couldn't find symbol %#x (%.*s) by ordinal. rc=%d (%s)", iSymbol, cchSymbol, pchSymbol, rc, kErrName(rc));
        if (uValue != uValue2)
            return Failure("Symbol %#x (%.*s): Value mismatch %016" PRI_KLDRADDR " != %016" PRI_KLDRADDR " (enum!=query/ord)  pvBits=%p",
                           iSymbol, cchSymbol, pchSymbol, uValue, uValue2, pvUser);
        if (fKind != fKind2)
            return Failure("Symbol %#x (%.*s): Kind mismatch %#x != %#x (enum!=query/ord) pvBits=%p",
                           iSymbol, cchSymbol, pchSymbol, fKind, fKind2, pvUser);
    }

    /* query by name. */
    if (pchSymbol)
    {
        fKind2 = 0;
        rc = kLdrModQuerySymbol(pMod, pvUser, MY_BASEADDRESS, NIL_KLDRMOD_SYM_ORDINAL, pchSymbol, cchSymbol, pszVersion,
                                NULL, NULL, &uValue2, &fKind2);
        if (rc)
            return Failure("Couldn't find symbol %#x (%.*s) by name. rc=%d (%s)", iSymbol, cchSymbol, pchSymbol, rc, kErrName(rc));
        if (uValue != uValue2)
            return Failure("Symbol %#x (%.*s): Value mismatch %016" PRI_KLDRADDR " != %016" PRI_KLDRADDR " (enum!=query/name) pvBits=%p",
                           iSymbol, cchSymbol, pchSymbol, uValue, uValue2, pvUser);
        if (fKind != fKind2)
            return Failure("Symbol %#x (%.*s): Kind mismatch %#x != %#x (enum!=query/name) pvBits=%p",
                           iSymbol, cchSymbol, pchSymbol, fKind, fKind2, pvUser);
    }

    return 0;
}


/**
 * Dump debugger information and check it for correctness.
 */
static int BasicTestEnumDbgInfoCallback(PKLDRMOD pMod, KU32 iDbgInfo, KLDRDBGINFOTYPE enmType,
                                        KI16 iMajorVer, KI16 iMinorVer, KLDRFOFF offFile, KLDRADDR LinkAddress,
                                        KLDRSIZE cb, const char *pszExtFile, void *pvUser)
{
    printf("#0x%08x: enmType=%d %d.%d offFile=0x%" PRI_KLDRADDR " LinkAddress=%" PRI_KLDRADDR " cb=%" PRI_KLDRSIZE " pvUser=%p\n",
           iDbgInfo, enmType, iMajorVer, iMinorVer, (KLDRADDR)offFile, LinkAddress, cb, pvUser);
    if (pszExtFile)
        printf("            pszExtFile=%p '%s'\n", pszExtFile, pszExtFile);

    if (enmType >= KLDRDBGINFOTYPE_END || enmType <= KLDRDBGINFOTYPE_INVALID)
        return Failure("Bad enmType");
    if (pvUser != NULL)
        return Failure("pvUser");

    return 0;
}


/**
 * Performs the basic module loader test on the specified module and image bits.
 */
static int BasicTestsSub2(PKLDRMOD pMod, void *pvBits)
{
    KI32 cImports;
    KI32 i;
    int rc;
    KU32 fKind;
    KLDRADDR Value;
    KLDRADDR MainEPAddress;
    KLDRSTACKINFO StackInfo;

    printf("* Testing queries with pvBits=%p...\n", pvBits);

    /*
     * Get the import modules.
     */
    cImports = kLdrModNumberOfImports(pMod, pvBits);
    printf("cImports=%d\n", cImports);
    if (cImports < 0)
        return Failure("failed to query the number of import, cImports=%d", cImports);
    for (i = 0; i < cImports; i++)
    {
        char szImportModule[260];
        rc = kLdrModGetImport(pMod, pvBits, i, szImportModule, sizeof(szImportModule));
        if (rc)
            return Failure("failed to get import module name, rc=%d (%s). (%.260s)", rc, kErrName(rc), szImportModule);
        printf("import #%d: '%s'\n", i, szImportModule);
    }

    /*
     * Query stack info.
     */
    StackInfo.Address = ~(KLDRADDR)42;
    StackInfo.LinkAddress = ~(KLDRADDR)42;
    StackInfo.cbStack = ~(KLDRSIZE)42;
    StackInfo.cbStackThread = ~(KLDRSIZE)42;
    rc = kLdrModGetStackInfo(pMod, pvBits, MY_BASEADDRESS, &StackInfo);
    if (rc)
        return Failure("kLdrModGetStackInfo failed with rc=%d (%s)", rc, kErrName(rc));
    printf("Stack: Address=%016" PRI_KLDRADDR "   LinkAddress=%016" PRI_KLDRADDR "\n"
           "       cbStack=%016" PRI_KLDRSIZE " cbStackThread=%016" PRI_KLDRSIZE "\n",
           StackInfo.Address, StackInfo.LinkAddress, StackInfo.cbStack, StackInfo.cbStackThread);
    if (StackInfo.Address == ~(KLDRADDR)42)
        return Failure("Bad StackInfo.Address");
    if (StackInfo.LinkAddress == ~(KLDRADDR)42)
        return Failure("Bad StackInfo.LinkAddress");
    if (StackInfo.cbStack == ~(KLDRSIZE)42)
        return Failure("Bad StackInfo.cbStack");
    if (StackInfo.cbStackThread == ~(KLDRSIZE)42)
        return Failure("Bad StackInfo.cbStackThread");

    /*
     * Query entrypoint.
     */
    MainEPAddress = ~(KLDRADDR)42;
    rc = kLdrModQueryMainEntrypoint(pMod, pvBits, MY_BASEADDRESS, &MainEPAddress);
    if (rc)
        return Failure("kLdrModQueryMainEntrypoint failed with rc=%d (%s)", rc, kErrName(rc));
    printf("Entrypoint: %016" PRI_KLDRADDR "\n", MainEPAddress);
    if (MainEPAddress == ~(KLDRADDR)42)
        return Failure("MainEPAddress wasn't set.");
    if (MainEPAddress != NIL_KLDRADDR && MainEPAddress < MY_BASEADDRESS)
        return Failure("Bad MainEPAddress (a).");
    if (MainEPAddress != NIL_KLDRADDR && MainEPAddress >= MY_BASEADDRESS + kLdrModSize(pMod))
        return Failure("Bad MainEPAddress (b).");

    /*
     * Debugger information.
     */
    rc = kLdrModHasDbgInfo(pMod, pvBits);
    if (!rc)
        printf("Has Debugger Information\n");
    else if (rc == KLDR_ERR_NO_DEBUG_INFO)
        printf("NO Debugger Information\n");
    else
        return Failure("kLdrModHasDbgInfo failed with rc=%d (%s)", rc, kErrName(rc));
    rc = kLdrModEnumDbgInfo(pMod, pvBits, BasicTestEnumDbgInfoCallback, NULL);
    if (rc)
        return Failure("kLdrModEnumDbgInfo failed with rc=%d (%s)", rc, kErrName(rc));


    /*
     * Negative symbol query tests.
     */
    fKind = 0;
    Value = 0x0badc0de;
    rc = kLdrModQuerySymbol(pMod, pvBits, MY_BASEADDRESS, NIL_KLDRMOD_SYM_ORDINAL - 20, NULL, 0, NULL, NULL, NULL,
                            &Value, &fKind);
    if (rc)
    {
        if (Value != 0)
            return Failure("Value wasn't cleared on failure.");
    }

    fKind = 0;
    Value = 0x0badc0de;
    rc = kLdrModQuerySymbol(pMod, pvBits, MY_BASEADDRESS, NIL_KLDRMOD_SYM_ORDINAL, NULL, 0, NULL, NULL, NULL,
                            &Value, &fKind);
    if (!rc)
        return Failure("NIL ordinal succeeded!");
    if (Value != 0)
        return Failure("Value wasn't cleared on failure.");

    /*
     * Enumerate and query all symbols.
     */
    printf("\n"
           "Symbols:\n");
    rc = kLdrModEnumSymbols(pMod, pvBits, MY_BASEADDRESS, 0, BasicTestsEnumSymCallback, pvBits);
    if (rc)
        return Failure("kLdrModEnumSymbols failed with rc=%d (%s)", rc, kErrName(rc));


/*int     kLdrModCanExecuteOn(PKLDRMOD pMod, const void *pvBits, KCPUARCH enmArch, KCPU enmCpu);
*/

    return 0;
}


/**
 * Performs the basic module loader test on the specified module
 */
static int BasicTestsSub(PKLDRMOD pMod)
{
    int         rc;
    KU32        i;
    void       *pvBits;
    KSIZE       cbImage;

    /*
     * Check/dump the module structure.
     */
    printf("pMod=%p u32Magic=%#x cSegments=%d\n", (void *)pMod, pMod->u32Magic, pMod->cSegments);
    printf("enmType=%d enmFmt=%d enmArch=%d enmCpu=%d enmEndian=%d\n",
           pMod->enmType, pMod->enmFmt, pMod->enmArch, pMod->enmCpu, pMod->enmEndian);
    printf("Filename: %s (%d bytes)\n", pMod->pszFilename, pMod->cchFilename);
    printf("    Name: %s (%d bytes)\n", pMod->pszName, pMod->cchName);
    printf("\n");

    if (pMod->u32Magic != KLDRMOD_MAGIC)
        return Failure("Bad u32Magic");
    if (strlen(pMod->pszFilename) != pMod->cchFilename)
        return Failure("Bad cchFilename");
    if (strlen(pMod->pszName) != pMod->cchName)
        return Failure("Bad cchName");
    if (pMod->enmFmt >= KLDRFMT_END || pMod->enmFmt <= KLDRFMT_INVALID)
        return Failure("Bad enmFmt");
    if (pMod->enmType >= KLDRTYPE_END || pMod->enmType <= KLDRTYPE_INVALID)
        return Failure("Bad enmType: %d", pMod->enmType);
    if (!K_ARCH_IS_VALID(pMod->enmArch))
        return Failure("Bad enmArch");
    if (pMod->enmCpu >= KCPU_END || pMod->enmCpu <= KCPU_INVALID)
        return Failure("Bad enmCpu");
    if (pMod->enmEndian >= KLDRENDIAN_END || pMod->enmEndian <= KLDRENDIAN_INVALID)
        return Failure("Bad enmEndian");

    for (i = 0; i < pMod->cSegments; i++)
    {
        printf("seg #%d: pvUser=%p enmProt=%d Name: '%.*s' (%d bytes)\n",
               i, pMod->aSegments[i].pvUser, pMod->aSegments[i].enmProt,
               pMod->aSegments[i].cchName, pMod->aSegments[i].pchName, pMod->aSegments[i].cchName);
        printf("LinkAddress: %016" PRI_KLDRADDR "       cb: %016" PRI_KLDRSIZE "  Alignment=%08" PRI_KLDRADDR " \n",
               pMod->aSegments[i].LinkAddress, pMod->aSegments[i].cb, pMod->aSegments[i].Alignment);
        printf("        RVA: %016" PRI_KLDRADDR " cbMapped: %016" PRI_KLDRSIZE " MapAddress=%p\n",
               pMod->aSegments[i].RVA, (KLDRSIZE)pMod->aSegments[i].cbMapped, (void *)pMod->aSegments[i].MapAddress);
        printf("    offFile: %016" PRI_KLDRADDR "   cbFile: %016" PRI_KLDRSIZE "\n",
               (KLDRADDR)pMod->aSegments[i].offFile, (KLDRSIZE)pMod->aSegments[i].cbFile);
        printf("\n");

        if (pMod->aSegments[i].pvUser != NULL)
            return Failure("Bad pvUser");
        if (pMod->aSegments[i].enmProt >= KPROT_END || pMod->aSegments[i].enmProt <= KPROT_INVALID)
            return Failure("Bad enmProt");
        if (pMod->aSegments[i].MapAddress != 0)
            return Failure("Bad MapAddress");
        if (pMod->aSegments[i].cbMapped < pMod->aSegments[i].cb)
            return Failure("Bad cbMapped (1)");
        if (pMod->aSegments[i].cbMapped && !pMod->aSegments[i].Alignment)
            return Failure("Bad cbMapped (2)");
        if (pMod->aSegments[i].cbMapped > kLdrModSize(pMod))
            return Failure("Bad cbMapped (3)");
        if (    pMod->aSegments[i].Alignment
            &&  (pMod->aSegments[i].RVA & (pMod->aSegments[i].Alignment - 1)))
            return Failure("Bad RVA (1)");
        if (pMod->aSegments[i].RVA != NIL_KLDRADDR && !pMod->aSegments[i].Alignment)
            return Failure("Bad RVA (2)");
        if (    pMod->aSegments[i].RVA != NIL_KLDRADDR
            &&  pMod->aSegments[i].RVA >= kLdrModSize(pMod))
            return Failure("Bad RVA (3)");
        if (    pMod->aSegments[i].RVA != NIL_KLDRADDR
            &&  pMod->aSegments[i].RVA + pMod->aSegments[i].cbMapped > kLdrModSize(pMod))
            return Failure("Bad RVA/cbMapped (4)");
        if (pMod->aSegments[i].LinkAddress != NIL_KLDRADDR && !pMod->aSegments[i].Alignment)
            return Failure("Bad LinkAddress");
        if (    pMod->aSegments[i].LinkAddress != NIL_KLDRADDR
            &&  (pMod->aSegments[i].LinkAddress) & (pMod->aSegments[i].Alignment - 1))
            return Failure("Bad LinkAddress alignment");
        if (pMod->aSegments[i].offFile != -1 && pMod->aSegments[i].cbFile == -1)
            return Failure("Bad offFile");
        if (pMod->aSegments[i].offFile == -1 && pMod->aSegments[i].cbFile != -1)
            return Failure("Bad cbFile");
    }


    /*
     * Get image the size and query the image bits.
     */
    printf("* Testing user mapping...\n");

    cbImage = (KSIZE)kLdrModSize(pMod);
    if (cbImage != kLdrModSize(pMod))
        return Failure("aborting test because the image is too huge!");
    pvBits = malloc((KSIZE)cbImage);
    if (!pvBits)
        return Failure("failed to allocate %d bytes for the image", cbImage);

    rc = kLdrModGetBits(pMod, pvBits, (KUPTR)pvBits, BasicTestsGetImport, NULL);
    if (rc)
        return Failure("failed to get image bits, rc=%d (%s)", rc, kErrName(rc));

    /*
     * Another cleanup nesting.
     */
    rc = BasicTestsSub2(pMod, pvBits);
    if (!rc)
    {
        /*
         * Test relocating the bits in a few different ways before we're done with them.
         */
        void *pvBits2 = malloc((KSIZE)cbImage);
        if (pvBits2)
        {
            rc = BasicTestsRelocate(pMod, pvBits, pvBits2);
            free(pvBits2);
        }
        else
            rc = Failure("failed to allocate %d bytes for the 2nd image", cbImage);
    }

    free(pvBits);
    return rc;
}


/**
 * Tests the mapping related api, after mapping.
 */
static int BasicTestsSubMap2(PKLDRMOD pMod)
{
    int rc;

    rc = kLdrModFixupMapping(pMod, BasicTestsGetImport, NULL);
    if (rc)
        return Failure("kLdrModFixupMapping (a) failed, rc=%d (%s)", rc, kErrName(rc));

    rc = kLdrModReload(pMod);
    if (rc)
        return Failure("kLdrModReload (a) failed, rc=%d (%s)", rc, kErrName(rc));

    rc = kLdrModReload(pMod);
    if (rc)
        return Failure("kLdrModReload (b) failed, rc=%d (%s)", rc, kErrName(rc));

    rc = kLdrModFixupMapping(pMod, BasicTestsGetImport, NULL);
    if (rc)
        return Failure("kLdrModFixupMapping (b) failed, rc=%d (%s)", rc, kErrName(rc));

    rc = kLdrModAllocTLS(pMod);
    if (rc)
        return Failure("kLdrModAllocTLS (a) failed, rc=%d (%s)", rc, kErrName(rc));
    kLdrModFreeTLS(pMod);

    rc = kLdrModAllocTLS(pMod);
    if (rc)
        return Failure("kLdrModAllocTLS (b) failed, rc=%d (%s)", rc, kErrName(rc));
    kLdrModFreeTLS(pMod);

    /*
     * Repeat the BasicTestsSub2 with pvBits as NULL to test module
     * interpreters that can utilize the mapping.
     */
    rc = BasicTestsSub2(pMod, NULL);
    if (rc)
        return Failure("BasicTestsSub2 in Map2 failed, rc=%d (%s)", rc, kErrName(rc));
    return 0;
}


/**
 * Tests the mapping related api.
 */
static int BasicTestsSubMap(PKLDRMOD pMod)
{
    int rc, rc2;
    printf("* Mapping tests...\n");

    rc = kLdrModMap(pMod);
    if (rc)
        return Failure("kLdrModMap failed, rc=%d (%s)", rc, kErrName(rc));
    rc = BasicTestsSubMap2(pMod);
    rc2 = kLdrModUnmap(pMod);
    if (rc2)
    {
        Failure("kLdrModUnmap failed, rc=%d (%s)", rc2, kErrName(rc2));
        rc = rc ? rc : rc2;
    }

    printf("* Mapping tests done.\n");
    return rc;
}


/**
 * Performs basic module loader tests on the specified file.
 */
static int BasicTests(const char *pszFilename)
{
    PKLDRMOD pMod;
    int rc, rc2;

    printf("tstLdrMod: Testing '%s'", pszFilename);
    rc = kLdrModOpen(pszFilename, &pMod);
    if (!rc)
    {
        rc = BasicTestsSub(pMod);
        if (!rc)
            rc = BasicTestsSubMap(pMod);
        if (!rc)
            rc = BasicTestsSub2(pMod, NULL);
        rc2 = kLdrModClose(pMod);
        if (rc2)
            Failure("failed to close '%s', rc=%d (%s)", pszFilename, rc, kErrName(rc));
        if (rc2 && !rc)
            rc = rc2;
    }
    else
        Failure("Failed to open '%s', rc=%d (%s)", pszFilename, rc, kErrName(rc));
    return rc ? 1 : 0;
}


int main(int argc, char **argv)
{
    BasicTests(argv[argc-1]);

    if (!g_cErrors)
        printf("tstLdrMod: SUCCESS\n");
    else
        printf("tstLdrMod: FAILURE - %d errors\n", g_cErrors);
    return !!g_cErrors;
}

