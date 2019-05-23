/* $Id: kLdrModLX.c 102 2017-10-02 10:45:31Z bird $ */
/** @file
 * kLdr - The Module Interpreter for the Linear eXecutable (LX) Format.
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
#include "kLdrInternal.h"
#include <k/kLdrFmts/lx.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** @def KLDRMODLX_STRICT
 * Define KLDRMODLX_STRICT to enabled strict checks in KLDRMODLX. */
#define KLDRMODLX_STRICT 1

/** @def KLDRMODLX_ASSERT
 * Assert that an expression is true when KLDR_STRICT is defined.
 */
#ifdef KLDRMODLX_STRICT
# define KLDRMODLX_ASSERT(expr)  kHlpAssert(expr)
#else
# define KLDRMODLX_ASSERT(expr)  do {} while (0)
#endif


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Instance data for the LX module interpreter.
 */
typedef struct KLDRMODLX
{
    /** Pointer to the module. (Follows the section table.) */
    PKLDRMOD                pMod;
    /** Pointer to the user mapping. */
    const void             *pvMapping;
    /** The size of the mapped LX image. */
    KSIZE                   cbMapped;
    /** Reserved flags. */
    KU32                    f32Reserved;

    /** The offset of the LX header. */
    KLDRFOFF                offHdr;
    /** Copy of the LX header. */
    struct e32_exe          Hdr;

    /** Pointer to the loader section.
     * Allocated together with this strcture. */
    const KU8              *pbLoaderSection;
    /** Pointer to the last byte in the loader section. */
    const KU8              *pbLoaderSectionLast;
    /** Pointer to the object table in the loader section. */
    const struct o32_obj   *paObjs;
    /** Pointer to the object page map table in the loader section. */
    const struct o32_map   *paPageMappings;
    /** Pointer to the resource table in the loader section. */
    const struct rsrc32    *paRsrcs;
    /** Pointer to the resident name table in the loader section. */
    const KU8              *pbResNameTab;
    /** Pointer to the entry table in the loader section. */
    const KU8              *pbEntryTab;

    /** Pointer to the non-resident name table. */
    KU8                    *pbNonResNameTab;
    /** Pointer to the last byte in the non-resident name table. */
    const KU8              *pbNonResNameTabLast;

    /** Pointer to the fixup section. */
    KU8                    *pbFixupSection;
    /** Pointer to the last byte in the fixup section. */
    const KU8              *pbFixupSectionLast;
    /** Pointer to the fixup page table within pvFixupSection. */
    const KU32             *paoffPageFixups;
    /** Pointer to the fixup record table within pvFixupSection. */
    const KU8              *pbFixupRecs;
    /** Pointer to the import module name table within pvFixupSection. */
    const KU8              *pbImportMods;
    /** Pointer to the import module name table within pvFixupSection. */
    const KU8              *pbImportProcs;
} KLDRMODLX, *PKLDRMODLX;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int kldrModLXHasDbgInfo(PKLDRMOD pMod, const void *pvBits);
static int kldrModLXRelocateBits(PKLDRMOD pMod, void *pvBits, KLDRADDR NewBaseAddress, KLDRADDR OldBaseAddress,
                                 PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser);
static int kldrModLXDoCreate(PKRDR pRdr, KLDRFOFF offNewHdr, PKLDRMODLX *ppModLX);
static const KU8 *kldrModLXDoNameTableLookupByOrdinal(const KU8 *pbNameTable, KSSIZE cbNameTable, KU32 iOrdinal);
static int kldrModLXDoNameLookup(PKLDRMODLX pModLX, const char *pchSymbol, KSIZE cchSymbol, KU32 *piSymbol);
static const KU8 *kldrModLXDoNameTableLookupByName(const KU8 *pbNameTable, KSSIZE cbNameTable,
                                                       const char *pchSymbol, KSIZE cchSymbol);
static int kldrModLXDoLoadBits(PKLDRMODLX pModLX, void *pvBits);
static int kldrModLXDoIterDataUnpacking(KU8 *pbDst, const KU8 *pbSrc, int cbSrc);
static int kldrModLXDoIterData2Unpacking(KU8 *pbDst, const KU8 *pbSrc, int cbSrc);
static void kLdrModLXMemCopyW(KU8 *pbDst, const KU8 *pbSrc, int cb);
static int kldrModLXDoProtect(PKLDRMODLX pModLX, void *pvBits, unsigned fUnprotectOrProtect);
static int kldrModLXDoCallDLL(PKLDRMODLX pModLX, void *pvMapping, unsigned uOp, KUPTR uHandle);
static int kldrModLXDoForwarderQuery(PKLDRMODLX pModLX, const struct e32_entry *pEntry,
                                     PFNKLDRMODGETIMPORT pfnGetForwarder, void *pvUser, PKLDRADDR puValue, KU32 *pfKind);
static int kldrModLXDoLoadFixupSection(PKLDRMODLX pModLX);
static KI32 kldrModLXDoCall(KUPTR uEntrypoint, KUPTR uHandle, KU32 uOp, void *pvReserved);
static int kldrModLXDoReloc(KU8 *pbPage, int off, KLDRADDR PageAddress, const struct r32_rlc *prlc,
                            int iSelector, KLDRADDR uValue, KU32 fKind);


/**
 * Create a loader module instance interpreting the executable image found
 * in the specified file provider instance.
 *
 * @returns 0 on success and *ppMod pointing to a module instance.
 *          On failure, a non-zero OS specific error code is returned.
 * @param   pOps            Pointer to the registered method table.
 * @param   pRdr            The file provider instance to use.
 * @param   fFlags          Flags, MBZ.
 * @param   enmCpuArch      The desired CPU architecture. KCPUARCH_UNKNOWN means
 *                          anything goes, but with a preference for the current
 *                          host architecture.
 * @param   offNewHdr       The offset of the new header in MZ files. -1 if not found.
 * @param   ppMod           Where to store the module instance pointer.
 */
static int kldrModLXCreate(PCKLDRMODOPS pOps, PKRDR pRdr, KU32 fFlags, KCPUARCH enmCpuArch, KLDRFOFF offNewHdr, PPKLDRMOD ppMod)
{
    PKLDRMODLX pModLX;
    int rc;
    K_NOREF(fFlags);

    /*
     * Create the instance data and do a minimal header validation.
     */
    rc = kldrModLXDoCreate(pRdr, offNewHdr, &pModLX);
    if (!rc)
    {
        /*
         * Match up against the requested CPU architecture.
         */
        if (    enmCpuArch == KCPUARCH_UNKNOWN
            ||  pModLX->pMod->enmArch == enmCpuArch)
        {
            pModLX->pMod->pOps = pOps;
            pModLX->pMod->u32Magic = KLDRMOD_MAGIC;
            *ppMod = pModLX->pMod;
            return 0;
        }
        rc = KLDR_ERR_CPU_ARCH_MISMATCH;
    }
    kHlpFree(pModLX);
    return rc;
}


/**
 * Separate function for reading creating the LX module instance to
 * simplify cleanup on failure.
 */
static int kldrModLXDoCreate(PKRDR pRdr, KLDRFOFF offNewHdr, PKLDRMODLX *ppModLX)
{
    struct e32_exe Hdr;
    PKLDRMODLX pModLX;
    PKLDRMOD pMod;
    KSIZE cb;
    KSIZE cchFilename;
    KU32 off, offEnd;
    KU32 i;
    int rc;
    int fCanOptimizeMapping;
    KU32 NextRVA;
    *ppModLX = NULL;

    /*
     * Read the signature and file header.
     */
    rc = kRdrRead(pRdr, &Hdr, sizeof(Hdr), offNewHdr > 0 ? offNewHdr : 0);
    if (rc)
        return rc;
    if (    Hdr.e32_magic[0] != E32MAGIC1
        ||  Hdr.e32_magic[1] != E32MAGIC2)
        return KLDR_ERR_UNKNOWN_FORMAT;

    /* We're not interested in anything but x86 images. */
    if (    Hdr.e32_level != E32LEVEL
        ||  Hdr.e32_border != E32LEBO
        ||  Hdr.e32_worder != E32LEWO
        ||  Hdr.e32_cpu < E32CPU286
        ||  Hdr.e32_cpu > E32CPU486
        ||  Hdr.e32_pagesize != OBJPAGELEN
        )
        return KLDR_ERR_LX_BAD_HEADER;

    /* Some rough sanity checks. */
    offEnd = kRdrSize(pRdr) >= (KLDRFOFF)~(KU32)16 ? ~(KU32)16 : (KU32)kRdrSize(pRdr);
    if (    Hdr.e32_itermap > offEnd
        ||  Hdr.e32_datapage > offEnd
        ||  Hdr.e32_nrestab > offEnd
        ||  Hdr.e32_nrestab + Hdr.e32_cbnrestab > offEnd
        ||  Hdr.e32_ldrsize > offEnd - offNewHdr - sizeof(Hdr)
        ||  Hdr.e32_fixupsize > offEnd - offNewHdr - sizeof(Hdr)
        ||  Hdr.e32_fixupsize + Hdr.e32_ldrsize > offEnd - offNewHdr - sizeof(Hdr))
        return KLDR_ERR_LX_BAD_HEADER;

    /* Verify the loader section. */
    offEnd = Hdr.e32_objtab + Hdr.e32_ldrsize;
    if (Hdr.e32_objtab < sizeof(Hdr))
        return KLDR_ERR_LX_BAD_LOADER_SECTION;
    off = Hdr.e32_objtab + sizeof(struct o32_obj) * Hdr.e32_objcnt;
    if (off > offEnd)
        return KLDR_ERR_LX_BAD_LOADER_SECTION;
    if (    Hdr.e32_objmap
        &&  (Hdr.e32_objmap < off || Hdr.e32_objmap > offEnd))
        return KLDR_ERR_LX_BAD_LOADER_SECTION;
    if (    Hdr.e32_rsrccnt
        && (   Hdr.e32_rsrctab < off
            || Hdr.e32_rsrctab > offEnd
            || Hdr.e32_rsrctab + sizeof(struct rsrc32) * Hdr.e32_rsrccnt > offEnd))
        return KLDR_ERR_LX_BAD_LOADER_SECTION;
    if (    Hdr.e32_restab
        &&  (Hdr.e32_restab < off || Hdr.e32_restab > offEnd - 2))
        return KLDR_ERR_LX_BAD_LOADER_SECTION;
    if (    Hdr.e32_enttab
        &&  (Hdr.e32_enttab < off || Hdr.e32_enttab >= offEnd))
        return KLDR_ERR_LX_BAD_LOADER_SECTION;
    if (    Hdr.e32_dircnt
        && (Hdr.e32_dirtab < off || Hdr.e32_dirtab > offEnd - 2))
        return KLDR_ERR_LX_BAD_LOADER_SECTION;

    /* Verify the fixup section. */
    off = offEnd;
    offEnd = off + Hdr.e32_fixupsize;
    if (    Hdr.e32_fpagetab
        &&  (Hdr.e32_fpagetab < off || Hdr.e32_fpagetab > offEnd))
    {
        /*
         * wlink mixes the fixup section and the loader section.
         */
        off = Hdr.e32_fpagetab;
        offEnd = off + Hdr.e32_fixupsize;
        Hdr.e32_ldrsize = off - Hdr.e32_objtab;
    }
    if (    Hdr.e32_frectab
        &&  (Hdr.e32_frectab < off || Hdr.e32_frectab > offEnd))
        return KLDR_ERR_LX_BAD_FIXUP_SECTION;
    if (    Hdr.e32_impmod
        &&  (Hdr.e32_impmod < off || Hdr.e32_impmod > offEnd || Hdr.e32_impmod + Hdr.e32_impmodcnt > offEnd))
        return KLDR_ERR_LX_BAD_FIXUP_SECTION;
    if (    Hdr.e32_impproc
        &&  (Hdr.e32_impproc < off || Hdr.e32_impproc > offEnd))
        return KLDR_ERR_LX_BAD_FIXUP_SECTION;

    /*
     * Calc the instance size, allocate and initialize it.
     */
    cchFilename = kHlpStrLen(kRdrName(pRdr));
    cb = K_ALIGN_Z(sizeof(KLDRMODLX), 8)
       + K_ALIGN_Z(K_OFFSETOF(KLDRMOD, aSegments[Hdr.e32_objcnt + 1]), 8)
       + K_ALIGN_Z(cchFilename + 1, 8)
       + Hdr.e32_ldrsize + 2; /* +2 for two extra zeros. */
    pModLX = (PKLDRMODLX)kHlpAlloc(cb);
    if (!pModLX)
        return KERR_NO_MEMORY;
    *ppModLX = pModLX;

    /* KLDRMOD */
    pMod = (PKLDRMOD)((KU8 *)pModLX + K_ALIGN_Z(sizeof(KLDRMODLX), 8));
    pMod->pvData = pModLX;
    pMod->pRdr = pRdr;
    pMod->pOps = NULL;      /* set upon success. */
    pMod->cSegments = Hdr.e32_objcnt;
    pMod->cchFilename = (KU32)cchFilename;
    pMod->pszFilename = (char *)K_ALIGN_P(&pMod->aSegments[pMod->cSegments], 8);
    kHlpMemCopy((char *)pMod->pszFilename, kRdrName(pRdr), cchFilename + 1);
    pMod->pszName = NULL; /* finalized further down */
    pMod->cchName = 0;
    pMod->fFlags = 0;
    switch (Hdr.e32_cpu)
    {
        case E32CPU286:
            pMod->enmCpu = KCPU_I80286;
            pMod->enmArch = KCPUARCH_X86_16;
            break;
        case E32CPU386:
            pMod->enmCpu = KCPU_I386;
            pMod->enmArch = KCPUARCH_X86_32;
            break;
        case E32CPU486:
            pMod->enmCpu = KCPU_I486;
            pMod->enmArch = KCPUARCH_X86_32;
            break;
    }
    pMod->enmEndian = KLDRENDIAN_LITTLE;
    pMod->enmFmt = KLDRFMT_LX;
    switch (Hdr.e32_mflags & E32MODMASK)
    {
        case E32MODEXE:
            pMod->enmType = !(Hdr.e32_mflags & E32NOINTFIX)
                ? KLDRTYPE_EXECUTABLE_RELOCATABLE
                : KLDRTYPE_EXECUTABLE_FIXED;
            break;

        case E32MODDLL:
        case E32PROTDLL:
        case E32MODPROTDLL:
            pMod->enmType = !(Hdr.e32_mflags & E32SYSDLL)
                ? KLDRTYPE_SHARED_LIBRARY_RELOCATABLE
                : KLDRTYPE_SHARED_LIBRARY_FIXED;
            break;

        case E32MODPDEV:
        case E32MODVDEV:
            pMod->enmType = KLDRTYPE_SHARED_LIBRARY_RELOCATABLE;
            break;
    }
    pMod->u32Magic = 0;     /* set upon success. */

    /* KLDRMODLX */
    pModLX->pMod = pMod;
    pModLX->pvMapping = 0;
    pModLX->cbMapped = 0;
    pModLX->f32Reserved = 0;

    pModLX->offHdr = offNewHdr >= 0 ? offNewHdr : 0;
    kHlpMemCopy(&pModLX->Hdr, &Hdr, sizeof(Hdr));

    pModLX->pbLoaderSection = K_ALIGN_P(pMod->pszFilename + pMod->cchFilename + 1, 16);
    pModLX->pbLoaderSectionLast = pModLX->pbLoaderSection + pModLX->Hdr.e32_ldrsize - 1;
    pModLX->paObjs = NULL;
    pModLX->paPageMappings = NULL;
    pModLX->paRsrcs = NULL;
    pModLX->pbResNameTab = NULL;
    pModLX->pbEntryTab = NULL;

    pModLX->pbNonResNameTab = NULL;
    pModLX->pbNonResNameTabLast = NULL;

    pModLX->pbFixupSection = NULL;
    pModLX->pbFixupSectionLast = NULL;
    pModLX->paoffPageFixups = NULL;
    pModLX->pbFixupRecs = NULL;
    pModLX->pbImportMods = NULL;
    pModLX->pbImportProcs = NULL;

    /*
     * Read the loader data.
     */
    rc = kRdrRead(pRdr, (void *)pModLX->pbLoaderSection, pModLX->Hdr.e32_ldrsize, pModLX->Hdr.e32_objtab + pModLX->offHdr);
    if (rc)
        return rc;
    ((KU8 *)pModLX->pbLoaderSectionLast)[1] = 0;
    ((KU8 *)pModLX->pbLoaderSectionLast)[2] = 0;
    if (pModLX->Hdr.e32_objcnt)
        pModLX->paObjs = (const struct o32_obj *)pModLX->pbLoaderSection;
    if (pModLX->Hdr.e32_objmap)
        pModLX->paPageMappings = (const struct o32_map *)(pModLX->pbLoaderSection + pModLX->Hdr.e32_objmap - pModLX->Hdr.e32_objtab);
    if (pModLX->Hdr.e32_rsrccnt)
        pModLX->paRsrcs = (const struct rsrc32 *)(pModLX->pbLoaderSection + pModLX->Hdr.e32_rsrctab - pModLX->Hdr.e32_objtab);
    if (pModLX->Hdr.e32_restab)
        pModLX->pbResNameTab = pModLX->pbLoaderSection + pModLX->Hdr.e32_restab - pModLX->Hdr.e32_objtab;
    if (pModLX->Hdr.e32_enttab)
        pModLX->pbEntryTab = pModLX->pbLoaderSection + pModLX->Hdr.e32_enttab - pModLX->Hdr.e32_objtab;

    /*
     * Get the soname from the resident name table.
     * Very convenient that it's the 0 ordinal, because then we get a
     * free string terminator.
     * (The table entry consists of a pascal string followed by a 16-bit ordinal.)
     */
    if (pModLX->pbResNameTab)
        pMod->pszName = (const char *)kldrModLXDoNameTableLookupByOrdinal(pModLX->pbResNameTab,
                                                                          pModLX->pbLoaderSectionLast - pModLX->pbResNameTab + 1,
                                                                          0);
    if (!pMod->pszName)
        return KLDR_ERR_LX_NO_SONAME;
    pMod->cchName = *(const KU8 *)pMod->pszName++;
    if (pMod->cchName != kHlpStrLen(pMod->pszName))
        return KLDR_ERR_LX_BAD_SONAME;

    /*
     * Quick validation of the object table.
     */
    cb = 0;
    for (i = 0; i < pMod->cSegments; i++)
    {
        if (pModLX->paObjs[i].o32_base & (OBJPAGELEN - 1))
            return KLDR_ERR_LX_BAD_OBJECT_TABLE;
        if (pModLX->paObjs[i].o32_base + pModLX->paObjs[i].o32_size <= pModLX->paObjs[i].o32_base)
            return KLDR_ERR_LX_BAD_OBJECT_TABLE;
        if (pModLX->paObjs[i].o32_mapsize > (pModLX->paObjs[i].o32_size + (OBJPAGELEN - 1)))
            return KLDR_ERR_LX_BAD_OBJECT_TABLE;
        if (    pModLX->paObjs[i].o32_mapsize
            &&  (   (KU8 *)&pModLX->paPageMappings[pModLX->paObjs[i].o32_pagemap] > pModLX->pbLoaderSectionLast
                 || (KU8 *)&pModLX->paPageMappings[pModLX->paObjs[i].o32_pagemap + pModLX->paObjs[i].o32_mapsize]
                     > pModLX->pbLoaderSectionLast))
            return KLDR_ERR_LX_BAD_OBJECT_TABLE;
        if (i > 0 && !(pModLX->paObjs[i].o32_flags & OBJRSRC))
        {
            if (pModLX->paObjs[i].o32_base <= pModLX->paObjs[i - 1].o32_base)
                return KLDR_ERR_LX_BAD_OBJECT_TABLE;
            if (pModLX->paObjs[i].o32_base < pModLX->paObjs[i - 1].o32_base + pModLX->paObjs[i - 1].o32_mapsize)
                return KLDR_ERR_LX_BAD_OBJECT_TABLE;
        }
    }

    /*
     * Check if we can optimize the mapping by using a different
     * object alignment. The linker typically uses 64KB alignment,
     * we can easily get away with page alignment in most cases.
     */
    fCanOptimizeMapping = !(Hdr.e32_mflags & (E32NOINTFIX | E32SYSDLL));
    NextRVA = 0;

    /*
     * Setup the KLDRMOD segment array.
     */
    for (i = 0; i < pMod->cSegments; i++)
    {
        /* unused */
        pMod->aSegments[i].pvUser = NULL;
        pMod->aSegments[i].MapAddress = 0;
        pMod->aSegments[i].pchName = NULL;
        pMod->aSegments[i].cchName = 0;
        pMod->aSegments[i].offFile = -1;
        pMod->aSegments[i].cbFile = -1;
        pMod->aSegments[i].SelFlat = 0;
        pMod->aSegments[i].Sel16bit = 0;

        /* flags */
        pMod->aSegments[i].fFlags = 0;
        if (pModLX->paObjs[i].o32_flags & OBJBIGDEF)
            pMod->aSegments[i].fFlags = KLDRSEG_FLAG_16BIT;
        if (pModLX->paObjs[i].o32_flags & OBJALIAS16)
            pMod->aSegments[i].fFlags = KLDRSEG_FLAG_OS2_ALIAS16;
        if (pModLX->paObjs[i].o32_flags & OBJCONFORM)
            pMod->aSegments[i].fFlags = KLDRSEG_FLAG_OS2_CONFORM;
        if (pModLX->paObjs[i].o32_flags & OBJIOPL)
            pMod->aSegments[i].fFlags = KLDRSEG_FLAG_OS2_IOPL;

        /* size and addresses */
        pMod->aSegments[i].Alignment   = OBJPAGELEN;
        pMod->aSegments[i].cb          = pModLX->paObjs[i].o32_size;
        pMod->aSegments[i].LinkAddress = pModLX->paObjs[i].o32_base;
        pMod->aSegments[i].RVA         = NextRVA;
        if (    fCanOptimizeMapping
            ||  i + 1 >= pMod->cSegments
            ||  (pModLX->paObjs[i].o32_flags & OBJRSRC)
            ||  (pModLX->paObjs[i + 1].o32_flags & OBJRSRC))
            pMod->aSegments[i].cbMapped = K_ALIGN_Z(pModLX->paObjs[i].o32_size, OBJPAGELEN);
        else
            pMod->aSegments[i].cbMapped = pModLX->paObjs[i + 1].o32_base - pModLX->paObjs[i].o32_base;
        NextRVA += (KU32)pMod->aSegments[i].cbMapped;

        /* protection */
        switch (  pModLX->paObjs[i].o32_flags
                & (OBJSHARED | OBJREAD | OBJWRITE | OBJEXEC))
        {
            case 0:
            case OBJSHARED:
                pMod->aSegments[i].enmProt = KPROT_NOACCESS;
                break;
            case OBJREAD:
            case OBJREAD | OBJSHARED:
                pMod->aSegments[i].enmProt = KPROT_READONLY;
                break;
            case OBJWRITE:
            case OBJWRITE | OBJREAD:
                pMod->aSegments[i].enmProt = KPROT_WRITECOPY;
                break;
            case OBJWRITE | OBJSHARED:
            case OBJWRITE | OBJSHARED | OBJREAD:
                pMod->aSegments[i].enmProt = KPROT_READWRITE;
                break;
            case OBJEXEC:
            case OBJEXEC | OBJSHARED:
                pMod->aSegments[i].enmProt = KPROT_EXECUTE;
                break;
            case OBJEXEC | OBJREAD:
            case OBJEXEC | OBJREAD | OBJSHARED:
                pMod->aSegments[i].enmProt = KPROT_EXECUTE_READ;
                break;
            case OBJEXEC | OBJWRITE:
            case OBJEXEC | OBJWRITE | OBJREAD:
                pMod->aSegments[i].enmProt = KPROT_EXECUTE_WRITECOPY;
                break;
            case OBJEXEC | OBJWRITE | OBJSHARED:
            case OBJEXEC | OBJWRITE | OBJSHARED | OBJREAD:
                pMod->aSegments[i].enmProt = KPROT_EXECUTE_READWRITE;
                break;
        }
        if ((pModLX->paObjs[i].o32_flags & (OBJREAD | OBJWRITE | OBJEXEC | OBJRSRC)) == OBJRSRC)
            pMod->aSegments[i].enmProt = KPROT_READONLY;
        /*pMod->aSegments[i].f16bit = !(pModLX->paObjs[i].o32_flags & OBJBIGDEF)
        pMod->aSegments[i].fIOPL = !(pModLX->paObjs[i].o32_flags & OBJIOPL)
        pMod->aSegments[i].fConforming = !(pModLX->paObjs[i].o32_flags & OBJCONFORM) */
    }

    /* set the mapping size */
    pModLX->cbMapped = NextRVA;

    /*
     * We're done.
     */
    *ppModLX = pModLX;
    return 0;
}


/** @copydoc KLDRMODOPS::pfnDestroy */
static int kldrModLXDestroy(PKLDRMOD pMod)
{
    PKLDRMODLX pModLX = (PKLDRMODLX)pMod->pvData;
    int rc = 0;
    KLDRMODLX_ASSERT(!pModLX->pvMapping);

    if (pMod->pRdr)
    {
        rc = kRdrClose(pMod->pRdr);
        pMod->pRdr = NULL;
    }
    if (pModLX->pbNonResNameTab)
    {
        kHlpFree(pModLX->pbNonResNameTab);
        pModLX->pbNonResNameTab = NULL;
    }
    if (pModLX->pbFixupSection)
    {
        kHlpFree(pModLX->pbFixupSection);
        pModLX->pbFixupSection = NULL;
    }
    pMod->u32Magic = 0;
    pMod->pOps = NULL;
    kHlpFree(pModLX);
    return rc;
}


/**
 * Resolved base address aliases.
 *
 * @param   pModLX          The interpreter module instance
 * @param   pBaseAddress    The base address, IN & OUT.
 */
static void kldrModLXResolveBaseAddress(PKLDRMODLX pModLX, PKLDRADDR pBaseAddress)
{
    if (*pBaseAddress == KLDRMOD_BASEADDRESS_MAP)
        *pBaseAddress = pModLX->pMod->aSegments[0].MapAddress;
    else if (*pBaseAddress == KLDRMOD_BASEADDRESS_LINK)
        *pBaseAddress = pModLX->pMod->aSegments[0].LinkAddress;
}


/** @copydoc kLdrModQuerySymbol */
static int kldrModLXQuerySymbol(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, KU32 iSymbol,
                                const char *pchSymbol, KSIZE cchSymbol, const char *pszVersion,
                                PFNKLDRMODGETIMPORT pfnGetForwarder, void *pvUser, PKLDRADDR puValue, KU32 *pfKind)
{
    PKLDRMODLX                  pModLX = (PKLDRMODLX)pMod->pvData;
    KU32                        iOrdinal;
    int                         rc;
    const struct b32_bundle     *pBundle;
    K_NOREF(pvBits);
    K_NOREF(pszVersion);

    /*
     * Give up at once if there is no entry table.
     */
    if (!pModLX->Hdr.e32_enttab)
        return KLDR_ERR_SYMBOL_NOT_FOUND;

    /*
     * Translate the symbol name into an ordinal.
     */
    if (pchSymbol)
    {
        rc = kldrModLXDoNameLookup(pModLX, pchSymbol, cchSymbol, &iSymbol);
        if (rc)
            return rc;
    }

    /*
     * Iterate the entry table.
     * (The entry table is made up of bundles of similar exports.)
     */
    iOrdinal = 1;
    pBundle = (const struct b32_bundle *)pModLX->pbEntryTab;
    while (pBundle->b32_cnt && iOrdinal <= iSymbol)
    {
        static const KSIZE s_cbEntry[] = { 0, 3, 5, 5, 7 };

        /*
         * Check for a hit first.
         */
        iOrdinal += pBundle->b32_cnt;
        if (iSymbol < iOrdinal)
        {
            KU32 offObject;
            const struct e32_entry *pEntry = (const struct e32_entry *)((KUPTR)(pBundle + 1)
                                                                        +   (iSymbol - (iOrdinal - pBundle->b32_cnt))
                                                                          * s_cbEntry[pBundle->b32_type]);

            /*
             * Calculate the return address.
             */
            kldrModLXResolveBaseAddress(pModLX, &BaseAddress);
            switch (pBundle->b32_type)
            {
                /* empty bundles are place holders unused ordinal ranges. */
                case EMPTY:
                    return KLDR_ERR_SYMBOL_NOT_FOUND;

                /* e32_flags + a 16-bit offset. */
                case ENTRY16:
                    offObject = pEntry->e32_variant.e32_offset.offset16;
                    if (pfKind)
                        *pfKind = KLDRSYMKIND_16BIT | KLDRSYMKIND_NO_TYPE;
                    break;

                /* e32_flags + a 16-bit offset + a 16-bit callgate selector. */
                case GATE16:
                    offObject = pEntry->e32_variant.e32_callgate.offset;
                    if (pfKind)
                        *pfKind = KLDRSYMKIND_16BIT | KLDRSYMKIND_CODE;
                    break;

                /* e32_flags + a 32-bit offset. */
                case ENTRY32:
                    offObject = pEntry->e32_variant.e32_offset.offset32;
                    if (pfKind)
                        *pfKind = KLDRSYMKIND_32BIT;
                    break;

                /* e32_flags + 16-bit import module ordinal + a 32-bit procname or ordinal. */
                case ENTRYFWD:
                    return kldrModLXDoForwarderQuery(pModLX, pEntry, pfnGetForwarder, pvUser, puValue, pfKind);

                default:
                    /* anyone actually using TYPEINFO will end up here. */
                    KLDRMODLX_ASSERT(!"Bad bundle type");
                    return KLDR_ERR_LX_BAD_BUNDLE;
            }

            /*
             * Validate the object number and calc the return address.
             */
            if (    pBundle->b32_obj <= 0
                ||  pBundle->b32_obj > pMod->cSegments)
                return KLDR_ERR_LX_BAD_BUNDLE;
            if (puValue)
                *puValue = BaseAddress
                         + offObject
                         + pMod->aSegments[pBundle->b32_obj - 1].RVA;
            return 0;
        }

        /*
         * Skip the bundle.
         */
        if (pBundle->b32_type > ENTRYFWD)
        {
            KLDRMODLX_ASSERT(!"Bad type"); /** @todo figure out TYPEINFO. */
            return KLDR_ERR_LX_BAD_BUNDLE;
        }
        if (pBundle->b32_type == 0)
            pBundle = (const struct b32_bundle *)((const KU8 *)pBundle + 2);
        else
            pBundle = (const struct b32_bundle *)((const KU8 *)(pBundle + 1) + s_cbEntry[pBundle->b32_type] * pBundle->b32_cnt);
    }

    return KLDR_ERR_SYMBOL_NOT_FOUND;
}


/**
 * Do name lookup.
 *
 * @returns See kLdrModQuerySymbol.
 * @param   pModLX      The module to lookup the symbol in.
 * @param   pchSymbol   The symbol to lookup.
 * @param   cchSymbol   The symbol name length.
 * @param   piSymbol    Where to store the symbol ordinal.
 */
static int kldrModLXDoNameLookup(PKLDRMODLX pModLX, const char *pchSymbol, KSIZE cchSymbol, KU32 *piSymbol)
{

    /*
     * First do a hash table lookup.
     */
    /** @todo hash name table for speed. */

    /*
     * Search the name tables.
     */
    const KU8 *pbName = kldrModLXDoNameTableLookupByName(pModLX->pbResNameTab,
                                                         pModLX->pbLoaderSectionLast - pModLX->pbResNameTab + 1,
                                                         pchSymbol, cchSymbol);
    if (!pbName)
    {
        if (!pModLX->pbNonResNameTab)
        {
            /* lazy load it */
            /** @todo non-resident name table. */
        }
        if (pModLX->pbNonResNameTab)
            pbName = kldrModLXDoNameTableLookupByName(pModLX->pbResNameTab,
                                                      pModLX->pbNonResNameTabLast - pModLX->pbResNameTab + 1,
                                                      pchSymbol, cchSymbol);
    }
    if (!pbName)
        return KLDR_ERR_SYMBOL_NOT_FOUND;

    *piSymbol = *(const KU16 *)(pbName + 1 + *pbName);
    return 0;
}


#if 0
/**
 * Hash a symbol using the algorithm from sdbm.
 *
 * The following was is the documenation of the orignal sdbm functions:
 *
 * This algorithm was created for sdbm (a public-domain reimplementation of
 * ndbm) database library. it was found to do well in scrambling bits,
 * causing better distribution of the keys and fewer splits. it also happens
 * to be a good general hashing function with good distribution. the actual
 * function is hash(i) = hash(i - 1) * 65599 + str[i]; what is included below
 * is the faster version used in gawk. [there is even a faster, duff-device
 * version] the magic constant 65599 was picked out of thin air while
 * experimenting with different constants, and turns out to be a prime.
 * this is one of the algorithms used in berkeley db (see sleepycat) and
 * elsewhere.
 */
static KU32 kldrModLXDoHash(const char *pchSymbol, KU8 cchSymbol)
{
    KU32 hash = 0;
    int ch;

    while (     cchSymbol-- > 0
           &&   (ch = *(unsigned const char *)pchSymbol++))
        hash = ch + (hash << 6) + (hash << 16) - hash;

    return hash;
}
#endif


/**
 * Lookup a name table entry by name.
 *
 * @returns Pointer to the name table entry if found.
 * @returns NULL if not found.
 * @param   pbNameTable     Pointer to the name table that should be searched.
 * @param   cbNameTable     The size of the name table.
 * @param   pchSymbol       The name of the symbol we're looking for.
 * @param   cchSymbol       The length of the symbol name.
 */
static const KU8 *kldrModLXDoNameTableLookupByName(const KU8 *pbNameTable, KSSIZE cbNameTable,
                                                   const char *pchSymbol, KSIZE cchSymbol)
{
    /*
     * Determin the namelength up front so we can skip anything which doesn't matches the length.
     */
    KU8 cbSymbol8Bit = (KU8)cchSymbol;
    if (cbSymbol8Bit != cchSymbol)
        return NULL; /* too long. */

    /*
     * Walk the name table.
     */
    while (*pbNameTable != 0 && cbNameTable > 0)
    {
        const KU8 cbName = *pbNameTable;

        cbNameTable -= cbName + 1 + 2;
        if (cbNameTable < 0)
            break;

        if (    cbName == cbSymbol8Bit
            &&  !kHlpMemComp(pbNameTable + 1, pchSymbol, cbName))
            return pbNameTable;

        /* next entry */
        pbNameTable += cbName + 1 + 2;
    }

    return NULL;
}


/**
 * Deal with a forwarder entry.
 *
 * @returns See kLdrModQuerySymbol.
 * @param   pModLX          The PE module interpreter instance.
 * @param   pEntry          The forwarder entry.
 * @param   pfnGetForwarder The callback for resolving forwarder symbols. (optional)
 * @param   pvUser          The user argument for the callback.
 * @param   puValue         Where to put the value. (optional)
 * @param   pfKind          Where to put the symbol kind. (optional)
 */
static int kldrModLXDoForwarderQuery(PKLDRMODLX pModLX, const struct e32_entry *pEntry,
                                     PFNKLDRMODGETIMPORT pfnGetForwarder, void *pvUser, PKLDRADDR puValue, KU32 *pfKind)
{
    int rc;
    KU32 iSymbol;
    const char *pchSymbol;
    KU8 cchSymbol;

    if (!pfnGetForwarder)
        return KLDR_ERR_FORWARDER_SYMBOL;

    /*
     * Validate the entry import module ordinal.
     */
    if (    !pEntry->e32_variant.e32_fwd.modord
        ||  pEntry->e32_variant.e32_fwd.modord > pModLX->Hdr.e32_impmodcnt)
        return KLDR_ERR_LX_BAD_FORWARDER;

    /*
     * Figure out the parameters.
     */
    if (pEntry->e32_flags & FWD_ORDINAL)
    {
        iSymbol = pEntry->e32_variant.e32_fwd.value;
        pchSymbol = NULL;                   /* no symbol name. */
        cchSymbol = 0;
    }
    else
    {
        const KU8 *pbName;

        /* load the fixup section if necessary. */
        if (!pModLX->pbImportProcs)
        {
            rc = kldrModLXDoLoadFixupSection(pModLX);
            if (rc)
                return rc;
        }

        /* Make name pointer. */
        pbName = pModLX->pbImportProcs + pEntry->e32_variant.e32_fwd.value;
        if (    pbName >= pModLX->pbFixupSectionLast
            ||  pbName < pModLX->pbFixupSection
            || !*pbName)
            return KLDR_ERR_LX_BAD_FORWARDER;


        /* check for '#' name. */
        if (pbName[1] == '#')
        {
            KU8         cbLeft = *pbName;
            const KU8  *pb = pbName + 1;
            unsigned    uBase;

            /* base detection */
            uBase = 10;
            if (    cbLeft > 1
                &&  pb[1] == '0'
                &&  (pb[2] == 'x' || pb[2] == 'X'))
            {
                uBase = 16;
                pb += 2;
                cbLeft -= 2;
            }

            /* ascii to integer */
            iSymbol = 0;
            while (cbLeft-- > 0)
            {
                /* convert char to digit. */
                unsigned uDigit = *pb++;
                if (uDigit >= '0' && uDigit <= '9')
                    uDigit -= '0';
                else if (uDigit >= 'a' && uDigit <= 'z')
                    uDigit -= 'a' + 10;
                else if (uDigit >= 'A' && uDigit <= 'Z')
                    uDigit -= 'A' + 10;
                else if (!uDigit)
                    break;
                else
                    return KLDR_ERR_LX_BAD_FORWARDER;
                if (uDigit >= uBase)
                    return KLDR_ERR_LX_BAD_FORWARDER;

                /* insert the digit */
                iSymbol *= uBase;
                iSymbol += uDigit;
            }
            if (!iSymbol)
                return KLDR_ERR_LX_BAD_FORWARDER;

            pchSymbol = NULL;               /* no symbol name. */
            cchSymbol = 0;
        }
        else
        {
            pchSymbol = (char *)pbName + 1;
            cchSymbol = *pbName;
            iSymbol = NIL_KLDRMOD_SYM_ORDINAL;
        }
    }

    /*
     * Resolve the forwarder.
     */
    rc = pfnGetForwarder(pModLX->pMod, pEntry->e32_variant.e32_fwd.modord - 1, iSymbol, pchSymbol, cchSymbol, NULL, puValue, pfKind, pvUser);
    if (!rc && pfKind)
        *pfKind |= KLDRSYMKIND_FORWARDER;
    return rc;
}


/**
 * Loads the fixup section from the executable image.
 *
 * The fixup section isn't loaded until it's accessed. It's also freed by kLdrModDone().
 *
 * @returns 0 on success, non-zero kLdr or native status code on failure.
 * @param   pModLX          The PE module interpreter instance.
 */
static int kldrModLXDoLoadFixupSection(PKLDRMODLX pModLX)
{
    int rc;
    KU32 off;
    void *pv;

    pv = kHlpAlloc(pModLX->Hdr.e32_fixupsize);
    if (!pv)
        return KERR_NO_MEMORY;

    off = pModLX->Hdr.e32_objtab + pModLX->Hdr.e32_ldrsize;
    rc = kRdrRead(pModLX->pMod->pRdr, pv, pModLX->Hdr.e32_fixupsize,
                     off + pModLX->offHdr);
    if (!rc)
    {
        pModLX->pbFixupSection = pv;
        pModLX->pbFixupSectionLast = pModLX->pbFixupSection + pModLX->Hdr.e32_fixupsize;
        KLDRMODLX_ASSERT(!pModLX->paoffPageFixups);
        if (pModLX->Hdr.e32_fpagetab)
            pModLX->paoffPageFixups = (const KU32 *)(pModLX->pbFixupSection + pModLX->Hdr.e32_fpagetab - off);
        KLDRMODLX_ASSERT(!pModLX->pbFixupRecs);
        if (pModLX->Hdr.e32_frectab)
            pModLX->pbFixupRecs = pModLX->pbFixupSection + pModLX->Hdr.e32_frectab - off;
        KLDRMODLX_ASSERT(!pModLX->pbImportMods);
        if (pModLX->Hdr.e32_impmod)
            pModLX->pbImportMods = pModLX->pbFixupSection + pModLX->Hdr.e32_impmod - off;
        KLDRMODLX_ASSERT(!pModLX->pbImportProcs);
        if (pModLX->Hdr.e32_impproc)
            pModLX->pbImportProcs = pModLX->pbFixupSection + pModLX->Hdr.e32_impproc - off;
    }
    else
        kHlpFree(pv);
    return rc;
}


/** @copydoc kLdrModEnumSymbols */
static int kldrModLXEnumSymbols(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress,
                                KU32 fFlags, PFNKLDRMODENUMSYMS pfnCallback, void *pvUser)
{
    PKLDRMODLX pModLX = (PKLDRMODLX)pMod->pvData;
    const struct b32_bundle *pBundle;
    KU32 iOrdinal;
    int rc = 0;
    K_NOREF(pvBits);
    K_NOREF(fFlags);

    kldrModLXResolveBaseAddress(pModLX, &BaseAddress);

    /*
     * Enumerate the entry table.
     * (The entry table is made up of bundles of similar exports.)
     */
    iOrdinal = 1;
    pBundle = (const struct b32_bundle *)pModLX->pbEntryTab;
    while (pBundle->b32_cnt && iOrdinal)
    {
        static const KSIZE s_cbEntry[] = { 0, 3, 5, 5, 7 };

        /*
         * Enum the entries in the bundle.
         */
        if (pBundle->b32_type != EMPTY)
        {
            const struct e32_entry *pEntry;
            KSIZE cbEntry;
            KLDRADDR BundleRVA;
            unsigned cLeft;


            /* Validate the bundle. */
            switch (pBundle->b32_type)
            {
                case ENTRY16:
                case GATE16:
                case ENTRY32:
                    if (    pBundle->b32_obj <= 0
                        ||  pBundle->b32_obj > pMod->cSegments)
                        return KLDR_ERR_LX_BAD_BUNDLE;
                    BundleRVA = pMod->aSegments[pBundle->b32_obj - 1].RVA;
                    break;

                case ENTRYFWD:
                    BundleRVA = 0;
                    break;

                default:
                    /* anyone actually using TYPEINFO will end up here. */
                    KLDRMODLX_ASSERT(!"Bad bundle type");
                    return KLDR_ERR_LX_BAD_BUNDLE;
            }

            /* iterate the bundle entries. */
            cbEntry = s_cbEntry[pBundle->b32_type];
            pEntry = (const struct e32_entry *)(pBundle + 1);
            cLeft = pBundle->b32_cnt;
            while (cLeft-- > 0)
            {
                KLDRADDR uValue;
                KU32 fKind;
                int fFoundName;
                const KU8 *pbName;

                /*
                 * Calc the symbol value and kind.
                 */
                switch (pBundle->b32_type)
                {
                    /* e32_flags + a 16-bit offset. */
                    case ENTRY16:
                        uValue = BaseAddress + BundleRVA + pEntry->e32_variant.e32_offset.offset16;
                        fKind = KLDRSYMKIND_16BIT | KLDRSYMKIND_NO_TYPE;
                        break;

                    /* e32_flags + a 16-bit offset + a 16-bit callgate selector. */
                    case GATE16:
                        uValue = BaseAddress + BundleRVA + pEntry->e32_variant.e32_callgate.offset;
                        fKind = KLDRSYMKIND_16BIT | KLDRSYMKIND_CODE;
                        break;

                    /* e32_flags + a 32-bit offset. */
                    case ENTRY32:
                        uValue = BaseAddress + BundleRVA + pEntry->e32_variant.e32_offset.offset32;
                        fKind = KLDRSYMKIND_32BIT;
                        break;

                    /* e32_flags + 16-bit import module ordinal + a 32-bit procname or ordinal. */
                    case ENTRYFWD:
                        uValue = 0; /** @todo implement enumeration of forwarders properly. */
                        fKind = KLDRSYMKIND_FORWARDER;
                        break;

                    default: /* shut up gcc. */
                        uValue = 0;
                        fKind = KLDRSYMKIND_NO_BIT | KLDRSYMKIND_NO_TYPE;
                        break;
                }

                /*
                 * Any symbol names?
                 */
                fFoundName = 0;

                /* resident name table. */
                pbName = pModLX->pbResNameTab;
                if (pbName)
                {
                    do
                    {
                        pbName = kldrModLXDoNameTableLookupByOrdinal(pbName, pModLX->pbLoaderSectionLast - pbName + 1, iOrdinal);
                        if (!pbName)
                            break;
                        fFoundName = 1;
                        rc = pfnCallback(pMod, iOrdinal, (const char *)pbName + 1, *pbName, NULL, uValue, fKind, pvUser);
                        if (rc)
                            return rc;

                        /* skip to the next entry */
                        pbName += 1 + *pbName + 2;
                    } while (pbName < pModLX->pbLoaderSectionLast);
                }

                /* resident name table. */
                pbName = pModLX->pbNonResNameTab;
                /** @todo lazy load the non-resident name table. */
                if (pbName)
                {
                    do
                    {
                        pbName = kldrModLXDoNameTableLookupByOrdinal(pbName, pModLX->pbNonResNameTabLast - pbName + 1, iOrdinal);
                        if (!pbName)
                            break;
                        fFoundName = 1;
                        rc = pfnCallback(pMod, iOrdinal, (const char *)pbName + 1, *pbName, NULL, uValue, fKind, pvUser);
                        if (rc)
                            return rc;

                        /* skip to the next entry */
                        pbName += 1 + *pbName + 2;
                    } while (pbName < pModLX->pbLoaderSectionLast);
                }

                /*
                 * If no names, call once with the ordinal only.
                 */
                if (!fFoundName)
                {
                    rc = pfnCallback(pMod, iOrdinal, NULL, 0, NULL, uValue, fKind, pvUser);
                    if (rc)
                        return rc;
                }

                /* next */
                iOrdinal++;
                pEntry = (const struct e32_entry *)((KUPTR)pEntry + cbEntry);
            }
        }

        /*
         * The next bundle.
         */
        if (pBundle->b32_type > ENTRYFWD)
        {
            KLDRMODLX_ASSERT(!"Bad type"); /** @todo figure out TYPEINFO. */
            return KLDR_ERR_LX_BAD_BUNDLE;
        }
        if (pBundle->b32_type == 0)
            pBundle = (const struct b32_bundle *)((const KU8 *)pBundle + 2);
        else
            pBundle = (const struct b32_bundle *)((const KU8 *)(pBundle + 1) + s_cbEntry[pBundle->b32_type] * pBundle->b32_cnt);
    }

    return 0;
}


/**
 * Lookup a name table entry by ordinal.
 *
 * @returns Pointer to the name table entry if found.
 * @returns NULL if not found.
 * @param   pbNameTable Pointer to the name table that should be searched.
 * @param   cbNameTable The size of the name table.
 * @param   iOrdinal    The ordinal to search for.
 */
static const KU8 *kldrModLXDoNameTableLookupByOrdinal(const KU8 *pbNameTable, KSSIZE cbNameTable, KU32 iOrdinal)
{
    while (*pbNameTable != 0 && cbNameTable > 0)
    {
        const KU8   cbName = *pbNameTable;
        KU32        iName;

        cbNameTable -= cbName + 1 + 2;
        if (cbNameTable < 0)
            break;

        iName = *(pbNameTable + cbName + 1)
              | ((unsigned)*(pbNameTable + cbName + 2) << 8);
        if (iName == iOrdinal)
            return pbNameTable;

        /* next entry */
        pbNameTable += cbName + 1 + 2;
    }

    return NULL;
}


/** @copydoc kLdrModGetImport */
static int kldrModLXGetImport(PKLDRMOD pMod, const void *pvBits, KU32 iImport, char *pszName, KSIZE cchName)
{
    PKLDRMODLX  pModLX = (PKLDRMODLX)pMod->pvData;
    const KU8  *pb;
    int         rc;
    K_NOREF(pvBits);

    /*
     * Validate
     */
    if (iImport >= pModLX->Hdr.e32_impmodcnt)
        return KLDR_ERR_IMPORT_ORDINAL_OUT_OF_BOUNDS;

    /*
     * Lazy loading the fixup section.
     */
    if (!pModLX->pbImportMods)
    {
        rc = kldrModLXDoLoadFixupSection(pModLX);
        if (rc)
            return rc;
    }

    /*
     * Iterate the module import table until we reach the requested import ordinal.
     */
    pb = pModLX->pbImportMods;
    while (iImport-- > 0)
        pb += *pb + 1;

    /*
     * Copy out the result.
     */
    if (*pb < cchName)
    {
        kHlpMemCopy(pszName, pb + 1, *pb);
        pszName[*pb] = '\0';
        rc = 0;
    }
    else
    {
        kHlpMemCopy(pszName, pb + 1, cchName);
        if (cchName)
            pszName[cchName - 1] = '\0';
        rc = KERR_BUFFER_OVERFLOW;
    }

    return rc;
}


/** @copydoc kLdrModNumberOfImports */
static KI32 kldrModLXNumberOfImports(PKLDRMOD pMod, const void *pvBits)
{
    PKLDRMODLX pModLX = (PKLDRMODLX)pMod->pvData;
    K_NOREF(pvBits);
    return pModLX->Hdr.e32_impmodcnt;
}


/** @copydoc kLdrModGetStackInfo */
static int kldrModLXGetStackInfo(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, PKLDRSTACKINFO pStackInfo)
{
    PKLDRMODLX pModLX = (PKLDRMODLX)pMod->pvData;
    const KU32 i = pModLX->Hdr.e32_stackobj;
    K_NOREF(pvBits);

    if (    i
        &&  i <= pMod->cSegments
        &&  pModLX->Hdr.e32_esp <= pMod->aSegments[i - 1].LinkAddress + pMod->aSegments[i - 1].cb
        &&  pModLX->Hdr.e32_stacksize
        &&  pModLX->Hdr.e32_esp - pModLX->Hdr.e32_stacksize >= pMod->aSegments[i - 1].LinkAddress)
    {

        kldrModLXResolveBaseAddress(pModLX, &BaseAddress);
        pStackInfo->LinkAddress = pModLX->Hdr.e32_esp - pModLX->Hdr.e32_stacksize;
        pStackInfo->Address = BaseAddress
                            + pMod->aSegments[i - 1].RVA
                            + pModLX->Hdr.e32_esp - pModLX->Hdr.e32_stacksize - pMod->aSegments[i - 1].LinkAddress;
    }
    else
    {
        pStackInfo->Address = NIL_KLDRADDR;
        pStackInfo->LinkAddress = NIL_KLDRADDR;
    }
    pStackInfo->cbStack = pModLX->Hdr.e32_stacksize;
    pStackInfo->cbStackThread = 0;

    return 0;
}


/** @copydoc kLdrModQueryMainEntrypoint */
static int kldrModLXQueryMainEntrypoint(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, PKLDRADDR pMainEPAddress)
{
    PKLDRMODLX pModLX = (PKLDRMODLX)pMod->pvData;
    K_NOREF(pvBits);

    /*
     * Convert the address from the header.
     */
    kldrModLXResolveBaseAddress(pModLX, &BaseAddress);
    *pMainEPAddress = pModLX->Hdr.e32_startobj
                   && pModLX->Hdr.e32_startobj <= pMod->cSegments
                   && pModLX->Hdr.e32_eip < pMod->aSegments[pModLX->Hdr.e32_startobj - 1].cb
        ? BaseAddress + pMod->aSegments[pModLX->Hdr.e32_startobj - 1].RVA + pModLX->Hdr.e32_eip
        : NIL_KLDRADDR;
    return 0;
}


/** @copydoc kLdrModEnumDbgInfo */
static int kldrModLXEnumDbgInfo(PKLDRMOD pMod, const void *pvBits, PFNKLDRENUMDBG pfnCallback, void *pvUser)
{
    /*PKLDRMODLX pModLX = (PKLDRMODLX)pMod->pvData;*/
    K_NOREF(pfnCallback);
    K_NOREF(pvUser);

    /*
     * Quit immediately if no debug info.
     */
    if (kldrModLXHasDbgInfo(pMod, pvBits))
        return 0;
#if 0
    /*
     * Read the debug info and look for familiar magics and structures.
     */
    /** @todo */
#endif

    return 0;
}


/** @copydoc kLdrModHasDbgInfo */
static int kldrModLXHasDbgInfo(PKLDRMOD pMod, const void *pvBits)
{
    PKLDRMODLX pModLX = (PKLDRMODLX)pMod->pvData;
    K_NOREF(pvBits);

    /*
     * Don't curretnly bother with linkers which doesn't advertise it in the header.
     */
    if (    !pModLX->Hdr.e32_debuginfo
        ||  !pModLX->Hdr.e32_debuglen)
        return KLDR_ERR_NO_DEBUG_INFO;
    return 0;
}


/** @copydoc kLdrModMap */
static int kldrModLXMap(PKLDRMOD pMod)
{
    PKLDRMODLX  pModLX = (PKLDRMODLX)pMod->pvData;
    unsigned    fFixed;
    void       *pvBase;
    int         rc;

    /*
     * Already mapped?
     */
    if (pModLX->pvMapping)
        return KLDR_ERR_ALREADY_MAPPED;

    /*
     * Allocate memory for it.
     */
    /* fixed image? */
    fFixed = pMod->enmType == KLDRTYPE_EXECUTABLE_FIXED
          || pMod->enmType == KLDRTYPE_SHARED_LIBRARY_FIXED;
    if (!fFixed)
        pvBase = NULL;
    else
    {
        pvBase = (void *)(KUPTR)pMod->aSegments[0].LinkAddress;
        if ((KUPTR)pvBase != pMod->aSegments[0].LinkAddress)
            return KLDR_ERR_ADDRESS_OVERFLOW;
    }
    rc = kHlpPageAlloc(&pvBase, pModLX->cbMapped, KPROT_EXECUTE_READWRITE, fFixed);
    if (rc)
        return rc;

    /*
     * Load the bits, apply page protection, and update the segment table.
     */
    rc = kldrModLXDoLoadBits(pModLX, pvBase);
    if (!rc)
        rc = kldrModLXDoProtect(pModLX, pvBase, 0 /* protect */);
    if (!rc)
    {
        KU32 i;
        for (i = 0; i < pMod->cSegments; i++)
        {
            if (pMod->aSegments[i].RVA != NIL_KLDRADDR)
                pMod->aSegments[i].MapAddress = (KUPTR)pvBase + (KUPTR)pMod->aSegments[i].RVA;
        }
        pModLX->pvMapping = pvBase;
    }
    else
        kHlpPageFree(pvBase, pModLX->cbMapped);
    return rc;
}


/**
 * Loads the LX pages into the specified memory mapping.
 *
 * @returns 0 on success.
 * @returns non-zero kLdr or OS status code on failure.
 *
 * @param   pModLX  The LX module interpreter instance.
 * @param   pvBits  Where to load the bits.
 */
static int kldrModLXDoLoadBits(PKLDRMODLX pModLX, void *pvBits)
{
    const PKRDR pRdr = pModLX->pMod->pRdr;
    KU8 *pbTmpPage = NULL;
    int rc = 0;
    KU32 i;

    /*
     * Iterate the segments.
     */
    for (i = 0; i < pModLX->Hdr.e32_objcnt; i++)
    {
        const struct o32_obj * const pObj = &pModLX->paObjs[i];
        const KU32      cPages = (KU32)(pModLX->pMod->aSegments[i].cbMapped / OBJPAGELEN);
        KU32            iPage;
        KU8            *pbPage = (KU8 *)pvBits + (KUPTR)pModLX->pMod->aSegments[i].RVA;

        /*
         * Iterate the page map pages.
         */
        for (iPage = 0; !rc && iPage < pObj->o32_mapsize; iPage++, pbPage += OBJPAGELEN)
        {
            const struct o32_map *pMap = &pModLX->paPageMappings[iPage + pObj->o32_pagemap - 1];
            switch (pMap->o32_pageflags)
            {
                case VALID:
                    if (pMap->o32_pagesize == OBJPAGELEN)
                        rc = kRdrRead(pRdr, pbPage, OBJPAGELEN,
                                         pModLX->Hdr.e32_datapage + (pMap->o32_pagedataoffset << pModLX->Hdr.e32_pageshift));
                    else if (pMap->o32_pagesize < OBJPAGELEN)
                    {
                        rc = kRdrRead(pRdr, pbPage, pMap->o32_pagesize,
                                         pModLX->Hdr.e32_datapage + (pMap->o32_pagedataoffset << pModLX->Hdr.e32_pageshift));
                        kHlpMemSet(pbPage + pMap->o32_pagesize, 0, OBJPAGELEN - pMap->o32_pagesize);
                    }
                    else
                        rc = KLDR_ERR_LX_BAD_PAGE_MAP;
                    break;

                case ITERDATA:
                case ITERDATA2:
                    /* make sure we've got a temp page .*/
                    if (!pbTmpPage)
                    {
                        pbTmpPage = kHlpAlloc(OBJPAGELEN + 256);
                        if (!pbTmpPage)
                            break;
                    }
                    /* validate the size. */
                    if (pMap->o32_pagesize > OBJPAGELEN + 252)
                    {
                        rc = KLDR_ERR_LX_BAD_PAGE_MAP;
                        break;
                    }

                    /* read it and ensure 4 extra zero bytes. */
                    rc = kRdrRead(pRdr, pbTmpPage, pMap->o32_pagesize,
                                     pModLX->Hdr.e32_datapage + (pMap->o32_pagedataoffset << pModLX->Hdr.e32_pageshift));
                    if (rc)
                        break;
                    kHlpMemSet(pbTmpPage + pMap->o32_pagesize, 0, 4);

                    /* unpack it into the image page. */
                    if (pMap->o32_pageflags == ITERDATA2)
                        rc = kldrModLXDoIterData2Unpacking(pbPage, pbTmpPage, pMap->o32_pagesize);
                    else
                        rc = kldrModLXDoIterDataUnpacking(pbPage, pbTmpPage, pMap->o32_pagesize);
                    break;

                case INVALID: /* we're probably not dealing correctly with INVALID pages... */
                case ZEROED:
                    kHlpMemSet(pbPage, 0, OBJPAGELEN);
                    break;

                case RANGE:
                    KLDRMODLX_ASSERT(!"RANGE");
                    /* Falls through. */
                default:
                    rc = KLDR_ERR_LX_BAD_PAGE_MAP;
                    break;
            }
        }
        if (rc)
            break;

        /*
         * Zero the remaining pages.
         */
        if (iPage < cPages)
            kHlpMemSet(pbPage, 0, (cPages - iPage) * OBJPAGELEN);
    }

    if (pbTmpPage)
        kHlpFree(pbTmpPage);
    return rc;
}


/**
 * Unpacks iterdata (aka EXEPACK).
 *
 * @returns 0 on success, non-zero kLdr status code on failure.
 * @param   pbDst       Where to put the uncompressed data. (Assumes OBJPAGELEN size.)
 * @param   pbSrc       The compressed source data.
 * @param   cbSrc       The file size of the compressed data. The source buffer
 *                      contains 4 additional zero bytes.
 */
static int kldrModLXDoIterDataUnpacking(KU8 *pbDst, const KU8 *pbSrc, int cbSrc)
{
    const struct LX_Iter   *pIter = (const struct LX_Iter *)pbSrc;
    int                     cbDst = OBJPAGELEN;

    /* Validate size of data. */
    if (cbSrc >= (int)OBJPAGELEN - 2)
        return KLDR_ERR_LX_BAD_ITERDATA;

    /*
     * Expand the page.
     */
    while (cbSrc > 0 && pIter->LX_nIter)
    {
        if (pIter->LX_nBytes == 1)
        {
            /*
             * Special case - one databyte.
             */
            cbDst -= pIter->LX_nIter;
            if (cbDst < 0)
                return KLDR_ERR_LX_BAD_ITERDATA;

            cbSrc -= 4 + 1;
            if (cbSrc < -4)
                return KLDR_ERR_LX_BAD_ITERDATA;

            kHlpMemSet(pbDst, pIter->LX_Iterdata, pIter->LX_nIter);
            pbDst += pIter->LX_nIter;
            pIter++;
        }
        else
        {
            /*
             * General.
             */
            int i;

            cbDst -= pIter->LX_nIter * pIter->LX_nBytes;
            if (cbDst < 0)
                return KLDR_ERR_LX_BAD_ITERDATA;

            cbSrc -= 4 + pIter->LX_nBytes;
            if (cbSrc < -4)
                return KLDR_ERR_LX_BAD_ITERDATA;

            for (i = pIter->LX_nIter; i > 0; i--, pbDst += pIter->LX_nBytes)
                kHlpMemCopy(pbDst, &pIter->LX_Iterdata, pIter->LX_nBytes);
            pIter   = (struct LX_Iter *)((char*)pIter + 4 + pIter->LX_nBytes);
        }
    }

    /*
     * Zero remainder of the page.
     */
    if (cbDst > 0)
        kHlpMemSet(pbDst, 0, cbDst);

    return 0;
}


/**
 * Unpacks iterdata (aka EXEPACK).
 *
 * @returns 0 on success, non-zero kLdr status code on failure.
 * @param   pbDst       Where to put the uncompressed data. (Assumes OBJPAGELEN size.)
 * @param   pbSrc       The compressed source data.
 * @param   cbSrc       The file size of the compressed data. The source buffer
 *                      contains 4 additional zero bytes.
 */
static int kldrModLXDoIterData2Unpacking(KU8 *pbDst, const KU8 *pbSrc, int cbSrc)
{
    int cbDst = OBJPAGELEN;

    while (cbSrc > 0)
    {
        /*
         * Bit 0 and 1 is the encoding type.
         */
        switch (*pbSrc & 0x03)
        {
            /*
             *
             *  0  1  2  3  4  5  6  7
             *  type  |              |
             *        ----------------
             *             cb         <cb bytes of data>
             *
             * Bits 2-7 is, if not zero, the length of an uncompressed run
             * starting at the following byte.
             *
             *  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23
             *  type  |              |  |                    | |                     |
             *        ----------------  ---------------------- -----------------------
             *             zero                 cb                 char to multiply
             *
             * If the bits are zero, the following two bytes describes a 1 byte interation
             * run. First byte is count, second is the byte to copy. A count of zero is
             * means end of data, and we simply stops. In that case the rest of the data
             * should be zero.
             */
            case 0:
            {
                if (*pbSrc)
                {
                    const int cb = *pbSrc >> 2;
                    cbDst -= cb;
                    if (cbDst < 0)
                        return KLDR_ERR_LX_BAD_ITERDATA2;
                    cbSrc -= cb + 1;
                    if (cbSrc < 0)
                        return KLDR_ERR_LX_BAD_ITERDATA2;
                    kHlpMemCopy(pbDst, ++pbSrc, cb);
                    pbDst += cb;
                    pbSrc += cb;
                }
                else if (cbSrc < 2)
                    return KLDR_ERR_LX_BAD_ITERDATA2;
                else
                {
                    const int cb = pbSrc[1];
                    if (!cb)
                        goto l_endloop;
                    cbDst -= cb;
                    if (cbDst < 0)
                        return KLDR_ERR_LX_BAD_ITERDATA2;
                    cbSrc -= 3;
                    if (cbSrc < 0)
                        return KLDR_ERR_LX_BAD_ITERDATA2;
                    kHlpMemSet(pbDst, pbSrc[2], cb);
                    pbDst += cb;
                    pbSrc += 3;
                }
                break;
            }


            /*
             *  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
             *  type  |  |  |     |  |                       |
             *        ----  -------  -------------------------
             *        cb1   cb2 - 3          offset            <cb1 bytes of data>
             *
             * Two bytes layed out as described above, followed by cb1 bytes of data to be copied.
             * The cb2(+3) and offset describes an amount of data to be copied from the expanded
             * data relative to the current position. The data copied as you would expect it to be.
             */
            case 1:
            {
                cbSrc -= 2;
                if (cbSrc < 0)
                    return KLDR_ERR_LX_BAD_ITERDATA2;
                else
                {
                    const unsigned  off = ((unsigned)pbSrc[1] << 1) | (*pbSrc >> 7);
                    const int       cb1 = (*pbSrc >> 2) & 3;
                    const int       cb2 = ((*pbSrc >> 4) & 7) + 3;

                    pbSrc += 2;
                    cbSrc -= cb1;
                    if (cbSrc < 0)
                        return KLDR_ERR_LX_BAD_ITERDATA2;
                    cbDst -= cb1;
                    if (cbDst < 0)
                        return KLDR_ERR_LX_BAD_ITERDATA2;
                    kHlpMemCopy(pbDst, pbSrc, cb1);
                    pbDst += cb1;
                    pbSrc += cb1;

                    if (off > OBJPAGELEN - (unsigned)cbDst)
                        return KLDR_ERR_LX_BAD_ITERDATA2;
                    cbDst -= cb2;
                    if (cbDst < 0)
                        return KLDR_ERR_LX_BAD_ITERDATA2;
                    kHlpMemMove(pbDst, pbDst - off, cb2);
                    pbDst += cb2;
                }
                break;
            }


            /*
             *  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
             *  type  |  |  |                                |
             *        ----  ----------------------------------
             *       cb-3               offset
             *
             * Two bytes layed out as described above.
             * The cb(+3) and offset describes an amount of data to be copied from the expanded
             * data relative to the current position.
             *
             * If offset == 1 the data is not copied as expected, but in the memcpyw manner.
             */
            case 2:
            {
                cbSrc -= 2;
                if (cbSrc < 0)
                    return KLDR_ERR_LX_BAD_ITERDATA2;
                else
                {
                    const unsigned  off = ((unsigned)pbSrc[1] << 4) | (*pbSrc >> 4);
                    const int       cb = ((*pbSrc >> 2) & 3) + 3;

                    pbSrc += 2;
                    if (off > OBJPAGELEN - (unsigned)cbDst)
                        return KLDR_ERR_LX_BAD_ITERDATA2;
                    cbDst -= cb;
                    if (cbDst < 0)
                        return KLDR_ERR_LX_BAD_ITERDATA2;
                    kLdrModLXMemCopyW(pbDst, pbDst - off, cb);
                    pbDst += cb;
                }
                break;
            }


            /*
             *  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23
             *  type  |        |  |              |  |                                |
             *        ----------  ----------------  ----------------------------------
             *           cb1            cb2                      offset                <cb1 bytes of data>
             *
             * Three bytes layed out as described above, followed by cb1 bytes of data to be copied.
             * The cb2 and offset describes an amount of data to be copied from the expanded
             * data relative to the current position.
             *
             * If offset == 1 the data is not copied as expected, but in the memcpyw manner.
             */
            case 3:
            {
                cbSrc -= 3;
                if (cbSrc < 0)
                    return KLDR_ERR_LX_BAD_ITERDATA2;
                else
                {
                    const int       cb1 = (*pbSrc >> 2) & 0xf;
                    const int       cb2 = ((pbSrc[1] & 0xf) << 2) | (*pbSrc >> 6);
                    const unsigned  off = ((unsigned)pbSrc[2] << 4) | (pbSrc[1] >> 4);

                    pbSrc += 3;
                    cbSrc -= cb1;
                    if (cbSrc < 0)
                        return KLDR_ERR_LX_BAD_ITERDATA2;
                    cbDst -= cb1;
                    if (cbDst < 0)
                        return KLDR_ERR_LX_BAD_ITERDATA2;
                    kHlpMemCopy(pbDst, pbSrc, cb1);
                    pbDst += cb1;
                    pbSrc += cb1;

                    if (off > OBJPAGELEN - (unsigned)cbDst)
                        return KLDR_ERR_LX_BAD_ITERDATA2;
                    cbDst -= cb2;
                    if (cbDst < 0)
                        return KLDR_ERR_LX_BAD_ITERDATA2;
                    kLdrModLXMemCopyW(pbDst, pbDst - off, cb2);
                    pbDst += cb2;
                }
                break;
            }
        } /* type switch. */
    } /* unpack loop */

l_endloop:


    /*
     * Zero remainder of the page.
     */
    if (cbDst > 0)
        kHlpMemSet(pbDst, 0, cbDst);

    return 0;
}


/**
 * Special memcpy employed by the iterdata2 algorithm.
 *
 * Emulate a 16-bit memcpy (copying 16-bit at a time) and the effects this
 * has if src is very close to the destination.
 *
 * @param   pbDst   Destination pointer.
 * @param   pbSrc   Source pointer. Will always be <= pbDst.
 * @param   cb      Amount of data to be copied.
 * @remark  This assumes that unaligned word and dword access is fine.
 */
static void kLdrModLXMemCopyW(KU8 *pbDst, const KU8 *pbSrc, int cb)
{
    switch (pbDst - pbSrc)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            /* 16-bit copy (unaligned) */
            if (cb & 1)
                *pbDst++ = *pbSrc++;
            for (cb >>= 1; cb > 0; cb--, pbDst += 2, pbSrc += 2)
                *(KU16 *)pbDst = *(const KU16 *)pbSrc;
            break;

        default:
            /* 32-bit copy (unaligned) */
            if (cb & 1)
                *pbDst++ = *pbSrc++;
            if (cb & 2)
            {
                *(KU16 *)pbDst = *(const KU16 *)pbSrc;
                pbDst += 2;
                pbSrc += 2;
            }
            for (cb >>= 2; cb > 0; cb--, pbDst += 4, pbSrc += 4)
                *(KU32 *)pbDst = *(const KU32 *)pbSrc;
            break;
    }
}


/**
 * Unprotects or protects the specified image mapping.
 *
 * @returns 0 on success.
 * @returns non-zero kLdr or OS status code on failure.
 *
 * @param   pModLX  The LX module interpreter instance.
 * @param   pvBits  The mapping to protect.
 * @param   UnprotectOrProtect  If 1 unprotect (i.e. make all writable), otherwise
 *          protect according to the object table.
 */
static int kldrModLXDoProtect(PKLDRMODLX pModLX, void *pvBits, unsigned fUnprotectOrProtect)
{
    KU32 i;
    PKLDRMOD pMod = pModLX->pMod;

    /*
     * Change object protection.
     */
    for (i = 0; i < pMod->cSegments; i++)
    {
        int rc;
        void *pv;
        KPROT enmProt;

        /* calc new protection. */
        enmProt = pMod->aSegments[i].enmProt;
        if (fUnprotectOrProtect)
        {
            switch (enmProt)
            {
                case KPROT_NOACCESS:
                case KPROT_READONLY:
                case KPROT_READWRITE:
                case KPROT_WRITECOPY:
                    enmProt = KPROT_READWRITE;
                    break;
                case KPROT_EXECUTE:
                case KPROT_EXECUTE_READ:
                case KPROT_EXECUTE_READWRITE:
                case KPROT_EXECUTE_WRITECOPY:
                    enmProt = KPROT_EXECUTE_READWRITE;
                    break;
                default:
                    KLDRMODLX_ASSERT(!"bad enmProt");
                    return -1;
            }
        }
        else
        {
            /* copy on write -> normal write. */
            if (enmProt == KPROT_EXECUTE_WRITECOPY)
                enmProt = KPROT_EXECUTE_READWRITE;
            else if (enmProt == KPROT_WRITECOPY)
                enmProt = KPROT_READWRITE;
        }


        /* calc the address and set page protection. */
        pv = (KU8 *)pvBits + pMod->aSegments[i].RVA;

        rc = kHlpPageProtect(pv, pMod->aSegments[i].cbMapped, enmProt);
        if (rc)
            break;

        /** @todo the gap page should be marked NOACCESS! */
    }

    return 0;
}


/** @copydoc kLdrModUnmap */
static int kldrModLXUnmap(PKLDRMOD pMod)
{
    PKLDRMODLX  pModLX = (PKLDRMODLX)pMod->pvData;
    KU32        i;
    int         rc;

    /*
     * Mapped?
     */
    if (!pModLX->pvMapping)
        return KLDR_ERR_NOT_MAPPED;

    /*
     * Free the mapping and update the segments.
     */
    rc = kHlpPageFree((void *)pModLX->pvMapping, pModLX->cbMapped);
    KLDRMODLX_ASSERT(!rc);
    pModLX->pvMapping = NULL;

    for (i = 0; i < pMod->cSegments; i++)
        pMod->aSegments[i].MapAddress = 0;

    return rc;
}


/** @copydoc kLdrModAllocTLS */
static int kldrModLXAllocTLS(PKLDRMOD pMod, void *pvMapping)
{
    PKLDRMODLX  pModLX = (PKLDRMODLX)pMod->pvData;

    /* no tls, just do the error checking. */
    if (   pvMapping == KLDRMOD_INT_MAP
        && pModLX->pvMapping)
        return KLDR_ERR_NOT_MAPPED;
    return 0;
}


/** @copydoc kLdrModFreeTLS */
static void kldrModLXFreeTLS(PKLDRMOD pMod, void *pvMapping)
{
    /* no tls. */
    K_NOREF(pMod);
    K_NOREF(pvMapping);

}


/** @copydoc kLdrModReload */
static int kldrModLXReload(PKLDRMOD pMod)
{
    PKLDRMODLX pModLX = (PKLDRMODLX)pMod->pvData;
    int rc, rc2;

    /*
     * Mapped?
     */
    if (!pModLX->pvMapping)
        return KLDR_ERR_NOT_MAPPED;

    /*
     * Before doing anything we'll have to make all pages writable.
     */
    rc = kldrModLXDoProtect(pModLX, (void *)pModLX->pvMapping, 1 /* unprotect */);
    if (rc)
        return rc;

    /*
     * Load the bits again.
     */
    rc = kldrModLXDoLoadBits(pModLX, (void *)pModLX->pvMapping);

    /*
     * Restore protection.
     */
    rc2 = kldrModLXDoProtect(pModLX, (void *)pModLX->pvMapping, 0 /* protect */);
    if (!rc && rc2)
        rc = rc2;
    return rc;
}


/** @copydoc kLdrModFixupMapping */
static int kldrModLXFixupMapping(PKLDRMOD pMod, PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser)
{
    PKLDRMODLX pModLX = (PKLDRMODLX)pMod->pvData;
    int rc, rc2;

    /*
     * Mapped?
     */
    if (!pModLX->pvMapping)
        return KLDR_ERR_NOT_MAPPED;

    /*
     * Before doing anything we'll have to make all pages writable.
     */
    rc = kldrModLXDoProtect(pModLX, (void *)pModLX->pvMapping, 1 /* unprotect */);
    if (rc)
        return rc;

    /*
     * Apply fixups and resolve imports.
     */
    rc = kldrModLXRelocateBits(pMod, (void *)pModLX->pvMapping, (KUPTR)pModLX->pvMapping,
                               pMod->aSegments[0].LinkAddress, pfnGetImport, pvUser);

    /*
     * Restore protection.
     */
    rc2 = kldrModLXDoProtect(pModLX, (void *)pModLX->pvMapping, 0 /* protect */);
    if (!rc && rc2)
        rc = rc2;
    return rc;
}


/** @copydoc kLdrModCallInit */
static int kldrModLXCallInit(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle)
{
    PKLDRMODLX pModLX = (PKLDRMODLX)pMod->pvData;
    int rc;

    /*
     * Mapped?
     */
    if (pvMapping == KLDRMOD_INT_MAP)
    {
        pvMapping = (void *)pModLX->pvMapping;
        if (!pvMapping)
            return KLDR_ERR_NOT_MAPPED;
    }

    /*
     * Do TLS callbacks first and then call the init/term function if it's a DLL.
     */
    if ((pModLX->Hdr.e32_mflags & E32MODMASK) == E32MODDLL)
        rc = kldrModLXDoCallDLL(pModLX, pvMapping, 0 /* attach */, uHandle);
    else
        rc = 0;
    return rc;
}


/**
 * Call the DLL entrypoint.
 *
 * @returns 0 on success.
 * @returns KLDR_ERR_MODULE_INIT_FAILED  or KLDR_ERR_THREAD_ATTACH_FAILED on failure.
 * @param   pModLX          The LX module interpreter instance.
 * @param   pvMapping       The module mapping to use (resolved).
 * @param   uOp             The operation (DLL_*).
 * @param   uHandle         The module handle to present.
 */
static int kldrModLXDoCallDLL(PKLDRMODLX pModLX, void *pvMapping, unsigned uOp, KUPTR uHandle)
{
    int rc;

    /*
     * If no entrypoint there isn't anything to be done.
     */
    if (    !pModLX->Hdr.e32_startobj
        ||  pModLX->Hdr.e32_startobj > pModLX->Hdr.e32_objcnt)
        return 0;

    /*
     * Invoke the entrypoint and convert the boolean result to a kLdr status code.
     */
    rc = kldrModLXDoCall((KUPTR)pvMapping
                         + (KUPTR)pModLX->pMod->aSegments[pModLX->Hdr.e32_startobj - 1].RVA
                         + pModLX->Hdr.e32_eip,
                         uHandle, uOp, NULL);
    if (rc)
        rc = 0;
    else if (uOp == 0 /* attach */)
        rc = KLDR_ERR_MODULE_INIT_FAILED;
    else /* detach: ignore failures */
        rc = 0;
    return rc;
}


/**
 * Do a 3 parameter callback.
 *
 * @returns 32-bit callback return.
 * @param   uEntrypoint     The address of the function to be called.
 * @param   uHandle         The first argument, the module handle.
 * @param   uOp             The second argumnet, the reason we're calling.
 * @param   pvReserved      The third argument, reserved argument. (figure this one out)
 */
static KI32 kldrModLXDoCall(KUPTR uEntrypoint, KUPTR uHandle, KU32 uOp, void *pvReserved)
{
#if defined(__X86__) || defined(__i386__) || defined(_M_IX86)
    KI32 rc;
/** @todo try/except */

    /*
     * Paranoia.
     */
# ifdef __GNUC__
    __asm__ __volatile__(
        "pushl  %2\n\t"
        "pushl  %1\n\t"
        "pushl  %0\n\t"
        "lea   12(%%esp), %2\n\t"
        "call  *%3\n\t"
        "movl   %2, %%esp\n\t"
        : "=a" (rc)
        : "d" (uOp),
          "S" (0),
          "c" (uEntrypoint),
          "0" (uHandle));
# elif defined(_MSC_VER)
    __asm {
        mov     eax, [uHandle]
        mov     edx, [uOp]
        mov     ecx, 0
        mov     ebx, [uEntrypoint]
        push    edi
        mov     edi, esp
        push    ecx
        push    edx
        push    eax
        call    ebx
        mov     esp, edi
        pop     edi
        mov     [rc], eax
    }
# else
#  error "port me!"
# endif
    K_NOREF(pvReserved);
    return rc;

#else
    K_NOREF(uEntrypoint);
    K_NOREF(uHandle);
    K_NOREF(uOp);
    K_NOREF(pvReserved);
    return KCPU_ERR_ARCH_CPU_NOT_COMPATIBLE;
#endif
}


/** @copydoc kLdrModCallTerm */
static int kldrModLXCallTerm(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle)
{
    PKLDRMODLX  pModLX = (PKLDRMODLX)pMod->pvData;

    /*
     * Mapped?
     */
    if (pvMapping == KLDRMOD_INT_MAP)
    {
        pvMapping = (void *)pModLX->pvMapping;
        if (!pvMapping)
            return KLDR_ERR_NOT_MAPPED;
    }

    /*
     * Do the call.
     */
    if ((pModLX->Hdr.e32_mflags & E32MODMASK) == E32MODDLL)
        kldrModLXDoCallDLL(pModLX, pvMapping, 1 /* detach */, uHandle);

    return 0;
}


/** @copydoc kLdrModCallThread */
static int kldrModLXCallThread(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle, unsigned fAttachingOrDetaching)
{
    /* no thread attach/detach callout. */
    K_NOREF(pMod);
    K_NOREF(pvMapping);
    K_NOREF(uHandle);
    K_NOREF(fAttachingOrDetaching);
    return 0;
}


/** @copydoc kLdrModSize */
static KLDRADDR kldrModLXSize(PKLDRMOD pMod)
{
    PKLDRMODLX pModLX = (PKLDRMODLX)pMod->pvData;
    return pModLX->cbMapped;
}


/** @copydoc kLdrModGetBits */
static int kldrModLXGetBits(PKLDRMOD pMod, void *pvBits, KLDRADDR BaseAddress, PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser)
{
    PKLDRMODLX  pModLX = (PKLDRMODLX)pMod->pvData;
    int         rc;

    /*
     * Load the image bits.
     */
    rc = kldrModLXDoLoadBits(pModLX, pvBits);
    if (rc)
        return rc;

    /*
     * Perform relocations.
     */
    return kldrModLXRelocateBits(pMod, pvBits, BaseAddress, pMod->aSegments[0].LinkAddress, pfnGetImport, pvUser);

}


/** @copydoc kLdrModRelocateBits */
static int kldrModLXRelocateBits(PKLDRMOD pMod, void *pvBits, KLDRADDR NewBaseAddress, KLDRADDR OldBaseAddress,
                                 PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser)
{
    PKLDRMODLX pModLX = (PKLDRMODLX)pMod->pvData;
    KU32 iSeg;
    int rc;

    /*
     * Do we need to to *anything*?
     */
    if (    NewBaseAddress == OldBaseAddress
        &&  NewBaseAddress == pModLX->paObjs[0].o32_base
        &&  !pModLX->Hdr.e32_impmodcnt)
        return 0;

    /*
     * Load the fixup section.
     */
    if (!pModLX->pbFixupSection)
    {
        rc = kldrModLXDoLoadFixupSection(pModLX);
        if (rc)
            return rc;
    }

    /*
     * Iterate the segments.
     */
    for (iSeg = 0; iSeg < pModLX->Hdr.e32_objcnt; iSeg++)
    {
        const struct o32_obj * const pObj = &pModLX->paObjs[iSeg];
        KLDRADDR        PageAddress = NewBaseAddress + pModLX->pMod->aSegments[iSeg].RVA;
        KU32            iPage;
        KU8            *pbPage = (KU8 *)pvBits + (KUPTR)pModLX->pMod->aSegments[iSeg].RVA;

        /*
         * Iterate the page map pages.
         */
        for (iPage = 0, rc = 0; !rc && iPage < pObj->o32_mapsize; iPage++, pbPage += OBJPAGELEN, PageAddress += OBJPAGELEN)
        {
            const KU8 * const   pbFixupRecEnd = pModLX->pbFixupRecs + pModLX->paoffPageFixups[iPage + pObj->o32_pagemap];
            const KU8          *pb            = pModLX->pbFixupRecs + pModLX->paoffPageFixups[iPage + pObj->o32_pagemap - 1];
            KLDRADDR            uValue        = NIL_KLDRADDR;
            KU32                fKind         = 0;
            int                 iSelector;

            /* sanity */
            if (pbFixupRecEnd < pb)
                return KLDR_ERR_BAD_FIXUP;
            if (pbFixupRecEnd - 1 > pModLX->pbFixupSectionLast)
                return KLDR_ERR_BAD_FIXUP;
            if (pb < pModLX->pbFixupSection)
                return KLDR_ERR_BAD_FIXUP;

            /*
             * Iterate the fixup record.
             */
            while (pb < pbFixupRecEnd)
            {
                union _rel
                {
                    const KU8 *             pb;
                    const struct r32_rlc   *prlc;
                } u;

                u.pb = pb;
                pb += 3 + (u.prlc->nr_stype & NRCHAIN ? 0 : 1); /* place pch at the 4th member. */

                /*
                 * Figure out the target.
                 */
                switch (u.prlc->nr_flags & NRRTYP)
                {
                    /*
                     * Internal fixup.
                     */
                    case NRRINT:
                    {
                        KU16 iTrgObject;
                        KU32 offTrgObject;

                        /* the object */
                        if (u.prlc->nr_flags & NR16OBJMOD)
                        {
                            iTrgObject = *(const KU16 *)pb;
                            pb += 2;
                        }
                        else
                            iTrgObject = *pb++;
                        iTrgObject--;
                        if (iTrgObject >= pModLX->Hdr.e32_objcnt)
                            return KLDR_ERR_BAD_FIXUP;

                        /* the target */
                        if ((u.prlc->nr_stype & NRSRCMASK) != NRSSEG)
                        {
                            if (u.prlc->nr_flags & NR32BITOFF)
                            {
                                offTrgObject = *(const KU32 *)pb;
                                pb += 4;
                            }
                            else
                            {
                                offTrgObject = *(const KU16 *)pb;
                                pb += 2;
                            }

                            /* calculate the symbol info. */
                            uValue = offTrgObject + NewBaseAddress + pMod->aSegments[iTrgObject].RVA;
                        }
                        else
                            uValue = NewBaseAddress + pMod->aSegments[iTrgObject].RVA;
                        if (    (u.prlc->nr_stype & NRALIAS)
                            ||  (pMod->aSegments[iTrgObject].fFlags & KLDRSEG_FLAG_16BIT))
                            iSelector = pMod->aSegments[iTrgObject].Sel16bit;
                        else
                            iSelector = pMod->aSegments[iTrgObject].SelFlat;
                        fKind = 0;
                        break;
                    }

                    /*
                     * Import by symbol ordinal.
                     */
                    case NRRORD:
                    {
                        KU16 iModule;
                        KU32 iSymbol;

                        /* the module ordinal */
                        if (u.prlc->nr_flags & NR16OBJMOD)
                        {
                            iModule = *(const KU16 *)pb;
                            pb += 2;
                        }
                        else
                            iModule = *pb++;
                        iModule--;
                        if (iModule >= pModLX->Hdr.e32_impmodcnt)
                            return KLDR_ERR_BAD_FIXUP;
#if 1
                        if (u.prlc->nr_flags & NRICHAIN)
                            return KLDR_ERR_BAD_FIXUP;
#endif

                        /* . */
                        if (u.prlc->nr_flags & NR32BITOFF)
                        {
                            iSymbol = *(const KU32 *)pb;
                            pb += 4;
                        }
                        else if (!(u.prlc->nr_flags & NR8BITORD))
                        {
                            iSymbol = *(const KU16 *)pb;
                            pb += 2;
                        }
                        else
                            iSymbol = *pb++;

                        /* resolve it. */
                        rc = pfnGetImport(pMod, iModule, iSymbol, NULL, 0, NULL, &uValue, &fKind, pvUser);
                        if (rc)
                            return rc;
                        iSelector = -1;
                        break;
                    }

                    /*
                     * Import by symbol name.
                     */
                    case NRRNAM:
                    {
                        KU32 iModule;
                        KU16 offSymbol;
                        const KU8 *pbSymbol;

                        /* the module ordinal */
                        if (u.prlc->nr_flags & NR16OBJMOD)
                        {
                            iModule = *(const KU16 *)pb;
                            pb += 2;
                        }
                        else
                            iModule = *pb++;
                        iModule--;
                        if (iModule >= pModLX->Hdr.e32_impmodcnt)
                            return KLDR_ERR_BAD_FIXUP;
#if 1
                        if (u.prlc->nr_flags & NRICHAIN)
                            return KLDR_ERR_BAD_FIXUP;
#endif

                        /* . */
                        if (u.prlc->nr_flags & NR32BITOFF)
                        {
                            offSymbol = *(const KU32 *)pb;
                            pb += 4;
                        }
                        else if (!(u.prlc->nr_flags & NR8BITORD))
                        {
                            offSymbol = *(const KU16 *)pb;
                            pb += 2;
                        }
                        else
                            offSymbol = *pb++;
                        pbSymbol = pModLX->pbImportProcs + offSymbol;
                        if (    pbSymbol < pModLX->pbImportProcs
                            ||  pbSymbol > pModLX->pbFixupSectionLast)
                            return KLDR_ERR_BAD_FIXUP;

                        /* resolve it. */
                        rc = pfnGetImport(pMod, iModule, NIL_KLDRMOD_SYM_ORDINAL, (const char *)pbSymbol + 1, *pbSymbol, NULL,
                                          &uValue, &fKind, pvUser);
                        if (rc)
                            return rc;
                        iSelector = -1;
                        break;
                    }

                    case NRRENT:
                        KLDRMODLX_ASSERT(!"NRRENT");
                        /* Falls through. */
                    default:
                        iSelector = -1;
                        break;
                }

                /* addend */
                if (u.prlc->nr_flags & NRADD)
                {
                    if (u.prlc->nr_flags & NR32BITADD)
                    {
                        uValue += *(const KU32 *)pb;
                        pb += 4;
                    }
                    else
                    {
                        uValue += *(const KU16 *)pb;
                        pb += 2;
                    }
                }


                /*
                 * Deal with the 'source' (i.e. the place that should be modified - very logical).
                 */
                if (!(u.prlc->nr_stype & NRCHAIN))
                {
                    int off = u.prlc->r32_soff;

                    /* common / simple */
                    if (    (u.prlc->nr_stype & NRSRCMASK) == NROFF32
                        &&  off >= 0
                        &&  off <= (int)OBJPAGELEN - 4)
                        *(KU32 *)&pbPage[off] = (KU32)uValue;
                    else if (    (u.prlc->nr_stype & NRSRCMASK) == NRSOFF32
                            &&  off >= 0
                            &&  off <= (int)OBJPAGELEN - 4)
                        *(KU32 *)&pbPage[off] = (KU32)(uValue - (PageAddress + off + 4));
                    else
                    {
                        /* generic */
                        rc = kldrModLXDoReloc(pbPage, off, PageAddress, u.prlc, iSelector, uValue, fKind);
                        if (rc)
                            return rc;
                    }
                }
                else if (!(u.prlc->nr_flags & NRICHAIN))
                {
                    const KI16 *poffSrc = (const KI16 *)pb;
                    KU8 c = u.pb[2];

                    /* common / simple */
                    if ((u.prlc->nr_stype & NRSRCMASK) == NROFF32)
                    {
                        while (c-- > 0)
                        {
                            int off = *poffSrc++;
                            if (off >= 0 && off <= (int)OBJPAGELEN - 4)
                                *(KU32 *)&pbPage[off] = (KU32)uValue;
                            else
                            {
                                rc = kldrModLXDoReloc(pbPage, off, PageAddress, u.prlc, iSelector, uValue, fKind);
                                if (rc)
                                    return rc;
                            }
                        }
                    }
                    else if ((u.prlc->nr_stype & NRSRCMASK) == NRSOFF32)
                    {
                        while (c-- > 0)
                        {
                            int off = *poffSrc++;
                            if (off >= 0 && off <= (int)OBJPAGELEN - 4)
                                *(KU32 *)&pbPage[off] = (KU32)(uValue - (PageAddress + off + 4));
                            else
                            {
                                rc = kldrModLXDoReloc(pbPage, off, PageAddress, u.prlc, iSelector, uValue, fKind);
                                if (rc)
                                    return rc;
                            }
                        }
                    }
                    else
                    {
                        while (c-- > 0)
                        {
                            rc = kldrModLXDoReloc(pbPage, *poffSrc++, PageAddress, u.prlc, iSelector, uValue, fKind);
                            if (rc)
                                return rc;
                        }
                    }
                    pb = (const KU8 *)poffSrc;
                }
                else
                {
                    /* This is a pain because it will require virgin pages on a relocation. */
                    KLDRMODLX_ASSERT(!"NRICHAIN");
                    return KLDR_ERR_LX_NRICHAIN_NOT_SUPPORTED;
                }
            }
        }
    }

    return 0;
}


/**
 * Applies the relocation to one 'source' in a page.
 *
 * This takes care of the more esotic case while the common cases
 * are dealt with seperately.
 *
 * @returns 0 on success, non-zero kLdr status code on failure.
 * @param   pbPage      The page in which to apply the fixup.
 * @param   off         Page relative offset of where to apply the offset.
 * @param   uValue      The target value.
 * @param   fKind       The target kind.
 */
static int kldrModLXDoReloc(KU8 *pbPage, int off, KLDRADDR PageAddress, const struct r32_rlc *prlc,
                            int iSelector, KLDRADDR uValue, KU32 fKind)
{
#pragma pack(1) /* just to be sure */
    union
    {
        KU8         ab[6];
        KU32        off32;
        KU16        off16;
        KU8         off8;
        struct
        {
            KU16    off;
            KU16    Sel;
        }           Far16;
        struct
        {
            KU32    off;
            KU16    Sel;
        }           Far32;
    }               uData;
#pragma pack()
    const KU8      *pbSrc;
    KU8            *pbDst;
    KU8             cb;

    K_NOREF(fKind);

    /*
     * Compose the fixup data.
     */
    switch (prlc->nr_stype & NRSRCMASK)
    {
        case NRSBYT:
            uData.off8 = (KU8)uValue;
            cb = 1;
            break;
        case NRSSEG:
            if (iSelector == -1)
            {
                /* fixme */
            }
            uData.off16 = iSelector;
            cb = 2;
            break;
        case NRSPTR:
            if (iSelector == -1)
            {
                /* fixme */
            }
            uData.Far16.off = (KU16)uValue;
            uData.Far16.Sel = iSelector;
            cb = 4;
            break;
        case NRSOFF:
            uData.off16 = (KU16)uValue;
            cb = 2;
            break;
        case NRPTR48:
            if (iSelector == -1)
            {
                /* fixme */
            }
            uData.Far32.off = (KU32)uValue;
            uData.Far32.Sel = iSelector;
            cb = 6;
            break;
        case NROFF32:
            uData.off32 = (KU32)uValue;
            cb = 4;
            break;
        case NRSOFF32:
            uData.off32 = (KU32)(uValue - (PageAddress + off + 4));
            cb = 4;
            break;
        default:
            return KLDR_ERR_LX_BAD_FIXUP_SECTION; /** @todo fix error, add more checks! */
    }

    /*
     * Apply it. This is sloooow...
     */
    pbSrc = &uData.ab[0];
    pbDst = pbPage + off;
    while (cb-- > 0)
    {
        if (off > (int)OBJPAGELEN)
            break;
        if (off >= 0)
            *pbDst = *pbSrc;
        pbSrc++;
        pbDst++;
    }

    return 0;
}


/**
 * The LX module interpreter method table.
 */
KLDRMODOPS g_kLdrModLXOps =
{
    "LX",
    NULL,
    kldrModLXCreate,
    kldrModLXDestroy,
    kldrModLXQuerySymbol,
    kldrModLXEnumSymbols,
    kldrModLXGetImport,
    kldrModLXNumberOfImports,
    NULL /* can execute one is optional */,
    kldrModLXGetStackInfo,
    kldrModLXQueryMainEntrypoint,
    NULL /* pfnQueryImageUuid */,
    NULL /* fixme */,
    NULL /* fixme */,
    kldrModLXEnumDbgInfo,
    kldrModLXHasDbgInfo,
    kldrModLXMap,
    kldrModLXUnmap,
    kldrModLXAllocTLS,
    kldrModLXFreeTLS,
    kldrModLXReload,
    kldrModLXFixupMapping,
    kldrModLXCallInit,
    kldrModLXCallTerm,
    kldrModLXCallThread,
    kldrModLXSize,
    kldrModLXGetBits,
    kldrModLXRelocateBits,
    NULL /* fixme: pfnMostlyDone */,
    42 /* the end */
};

