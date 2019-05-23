/* $Id: kLdrModPE.c 89 2016-09-07 13:32:53Z bird $ */
/** @file
 * kLdr - The Module Interpreter for the Portable Executable (PE) Format.
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
#include <k/kLdrFmts/pe.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** @def KLDRMODPE_STRICT
 * Define KLDRMODPE_STRICT to enabled strict checks in KLDRMODPE. */
#define KLDRMODPE_STRICT 1

/** @def KLDRMODPE_ASSERT
 * Assert that an expression is true when KLDR_STRICT is defined.
 */
#ifdef KLDRMODPE_STRICT
# define KLDRMODPE_ASSERT(expr)  kHlpAssert(expr)
#else
# define KLDRMODPE_ASSERT(expr)  do {} while (0)
#endif

/** @def KLDRMODPE_RVA2TYPE
 * Converts a RVA to a pointer of the specified type.
 * @param   pvBits      The bits (image base).
 * @param   uRVA        The image relative virtual address.
 * @param   type        The type to cast to.
 */
#define KLDRMODPE_RVA2TYPE(pvBits, uRVA, type) \
        ( (type) ((KUPTR)(pvBits) + (KUPTR)(uRVA)) )

/** @def KLDRMODPE_VALID_RVA
 * Checks that the specified RVA value is non-zero and within the bounds of the image.
 * @returns true/false.
 * @param   pModPE      The PE module interpreter instance.
 * @param   uRVA        The RVA to validate.
 */
#define KLDRMODPE_VALID_RVA(pModPE, uRVA) \
        ( (uRVA) && (uRVA) < (pModPE)->Hdrs.OptionalHeader.SizeOfImage )



/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Instance data for the PE module interpreter.
 */
typedef struct KLDRMODPE
{
    /** Pointer to the module. (Follows the section table.) */
    PKLDRMOD                pMod;
    /** Pointer to the RDR mapping of the raw file bits. NULL if not mapped. */
    const void             *pvBits;
    /** Pointer to the user mapping. */
    const void             *pvMapping;
    /** Reserved flags. */
    KU32                    f32Reserved;
    /** The number of imported modules.
     * If ~(KU32)0 this hasn't been determined yet. */
    KU32                    cImportModules;
    /** The offset of the NT headers. */
    KLDRFOFF                offHdrs;
    /** Copy of the NT headers. */
    IMAGE_NT_HEADERS64      Hdrs;
    /** The section header table . */
    IMAGE_SECTION_HEADER    aShdrs[1];
} KLDRMODPE, *PKLDRMODPE;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static KI32 kldrModPENumberOfImports(PKLDRMOD pMod, const void *pvBits);
static int  kldrModPERelocateBits(PKLDRMOD pMod, void *pvBits, KLDRADDR NewBaseAddress, KLDRADDR OldBaseAddress,
                                  PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser);

static int  kldrModPEDoCreate(PKRDR pRdr, KLDRFOFF offNewHdr, PKLDRMODPE *ppMod);
/*static void kldrModPEDoLoadConfigConversion(PIMAGE_LOAD_CONFIG_DIRECTORY64 pLoadCfg); */
static int  kLdrModPEDoOptionalHeaderValidation(PKLDRMODPE pModPE);
static int  kLdrModPEDoSectionHeadersValidation(PKLDRMODPE pModPE);
static void kldrModPEDoOptionalHeaderConversion(PIMAGE_OPTIONAL_HEADER64 pOptionalHeader);
static int  kldrModPEDoForwarderQuery(PKLDRMODPE pModPE, const void *pvBits, const char *pszForwarder,
                                     PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser, PKLDRADDR puValue, KU32 *pfKind);
static int  kldrModPEDoFixups(PKLDRMODPE pModPE, void *pvMapping, KLDRADDR NewBaseAddress, KLDRADDR OldBaseAddress);
static int  kldrModPEDoImports32Bit(PKLDRMODPE pModPE, void *pvMapping, const IMAGE_IMPORT_DESCRIPTOR *pImpDesc,
                                    PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser);
static int  kldrModPEDoImports64Bit(PKLDRMODPE pModPE, void *pvMapping, const IMAGE_IMPORT_DESCRIPTOR *pImpDesc,
                                    PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser);
static int  kldrModPEDoImports(PKLDRMODPE pModPE, void *pvMapping, PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser);
static int  kldrModPEDoCallDLL(PKLDRMODPE pModPE, void *pvMapping, unsigned uOp, KUPTR uHandle);
static int  kldrModPEDoCallTLS(PKLDRMODPE pModPE, void *pvMapping, unsigned uOp, KUPTR uHandle);
static KI32 kldrModPEDoCall(KUPTR uEntrypoint, KUPTR uHandle, KU32 uOp, void *pvReserved);


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
static int kldrModPECreate(PCKLDRMODOPS pOps, PKRDR pRdr, KU32 fFlags, KCPUARCH enmCpuArch, KLDRFOFF offNewHdr, PPKLDRMOD ppMod)
{
    PKLDRMODPE pModPE;
    int rc;
    K_NOREF(fFlags);

    /*
     * Create the instance data and do a minimal header validation.
     */
    rc = kldrModPEDoCreate(pRdr, offNewHdr, &pModPE);
    if (!rc)
    {
        /*
         * Match up against the requested CPU architecture.
         */
        if (    enmCpuArch == KCPUARCH_UNKNOWN
            ||  pModPE->pMod->enmArch == enmCpuArch)
        {
            pModPE->pMod->pOps = pOps;
            pModPE->pMod->u32Magic = KLDRMOD_MAGIC;
            *ppMod = pModPE->pMod;
            return 0;
        }
        rc = KLDR_ERR_CPU_ARCH_MISMATCH;
    }
    kHlpFree(pModPE);
    return rc;
}


/**
 * Separate function for reading creating the PE module instance to
 * simplify cleanup on failure.
 */
static int kldrModPEDoCreate(PKRDR pRdr, KLDRFOFF offNewHdr, PKLDRMODPE *ppModPE)
{
    struct
    {
        KU32                Signature;
        IMAGE_FILE_HEADER   FileHdr;
    } s;
    PKLDRMODPE pModPE;
    PKLDRMOD pMod;
    KSIZE cb;
    KSIZE cchFilename;
    KLDRFOFF off;
    KU32 i;
    int rc;
    *ppModPE = NULL;

    /*
     * Read the signature and file header.
     */
    rc = kRdrRead(pRdr, &s, sizeof(s), offNewHdr > 0 ? offNewHdr : 0);
    if (rc)
        return rc;
    if (s.Signature != IMAGE_NT_SIGNATURE)
        return KLDR_ERR_UNKNOWN_FORMAT;

    /* sanity checks. */
    if (    s.FileHdr.NumberOfSections > 4096
        ||  (   s.FileHdr.SizeOfOptionalHeader != sizeof(IMAGE_OPTIONAL_HEADER32)
             && s.FileHdr.SizeOfOptionalHeader != sizeof(IMAGE_OPTIONAL_HEADER64))
        ||  !(s.FileHdr.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)
       )
        return KLDR_ERR_PE_BAD_FILE_HEADER;
    if (    s.FileHdr.Machine != IMAGE_FILE_MACHINE_I386
        &&  s.FileHdr.Machine != IMAGE_FILE_MACHINE_AMD64
       )
        return KLDR_ERR_PE_UNSUPPORTED_MACHINE;

    /*
     * Calc the instance size, allocate and initialize it.
     */
    cchFilename = kHlpStrLen(kRdrName(pRdr));
    cb = K_ALIGN_Z(K_OFFSETOF(KLDRMODPE, aShdrs[s.FileHdr.NumberOfSections]), 16)
       + K_OFFSETOF(KLDRMOD, aSegments[s.FileHdr.NumberOfSections + 1])
       + cchFilename + 1;
    pModPE = (PKLDRMODPE)kHlpAlloc(cb);
    if (!pModPE)
        return KERR_NO_MEMORY;
    *ppModPE = pModPE;

    /* KLDRMOD */
    pMod = (PKLDRMOD)((KU8 *)pModPE + K_ALIGN_Z(K_OFFSETOF(KLDRMODPE, aShdrs[s.FileHdr.NumberOfSections]), 16));
    pMod->pvData = pModPE;
    pMod->pRdr = pRdr;
    pMod->pOps = NULL;      /* set upon success. */
    pMod->cSegments = s.FileHdr.NumberOfSections + 1;
    pMod->cchFilename = (KU32)cchFilename;
    pMod->pszFilename = (char *)&pMod->aSegments[pMod->cSegments];
    kHlpMemCopy((char *)pMod->pszFilename, kRdrName(pRdr), cchFilename + 1);
    pMod->pszName = kHlpGetFilename(pMod->pszFilename);
    pMod->cchName = (KU32)(cchFilename - (pMod->pszName - pMod->pszFilename));
    pMod->fFlags = 0;
    switch (s.FileHdr.Machine)
    {
        case IMAGE_FILE_MACHINE_I386:
            pMod->enmCpu = KCPU_I386;
            pMod->enmArch = KCPUARCH_X86_32;
            pMod->enmEndian = KLDRENDIAN_LITTLE;
            break;

        case IMAGE_FILE_MACHINE_AMD64:
            pMod->enmCpu = KCPU_K8;
            pMod->enmArch = KCPUARCH_AMD64;
            pMod->enmEndian = KLDRENDIAN_LITTLE;
            break;
        default:
            kHlpAssert(0);
            break;
    }
    pMod->enmFmt = KLDRFMT_PE;
    if (s.FileHdr.Characteristics & IMAGE_FILE_DLL)
        pMod->enmType = !(s.FileHdr.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
            ? KLDRTYPE_SHARED_LIBRARY_RELOCATABLE
            : KLDRTYPE_SHARED_LIBRARY_FIXED;
    else
        pMod->enmType = !(s.FileHdr.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
            ? KLDRTYPE_EXECUTABLE_RELOCATABLE
            : KLDRTYPE_EXECUTABLE_FIXED;
    pMod->u32Magic = 0;     /* set upon success. */

    /* KLDRMODPE */
    pModPE->pMod = pMod;
    pModPE->pvBits = NULL;
    pModPE->pvMapping = NULL;
    pModPE->f32Reserved = 0;
    pModPE->cImportModules = ~(KU32)0;
    pModPE->offHdrs = offNewHdr >= 0 ? offNewHdr : 0;
    pModPE->Hdrs.Signature = s.Signature;
    pModPE->Hdrs.FileHeader = s.FileHdr;

    /*
     * Read the optional header and the section table.
     */
    off = pModPE->offHdrs + sizeof(pModPE->Hdrs.Signature) + sizeof(pModPE->Hdrs.FileHeader);
    rc = kRdrRead(pRdr, &pModPE->Hdrs.OptionalHeader, pModPE->Hdrs.FileHeader.SizeOfOptionalHeader, off);
    if (rc)
        return rc;
    if (pModPE->Hdrs.FileHeader.SizeOfOptionalHeader != sizeof(pModPE->Hdrs.OptionalHeader))
        kldrModPEDoOptionalHeaderConversion(&pModPE->Hdrs.OptionalHeader);
    off += pModPE->Hdrs.FileHeader.SizeOfOptionalHeader;
    rc = kRdrRead(pRdr, &pModPE->aShdrs[0], sizeof(IMAGE_SECTION_HEADER) * pModPE->Hdrs.FileHeader.NumberOfSections, off);
    if (rc)
        return rc;

    /*
     * Validate the two.
     */
    rc = kLdrModPEDoOptionalHeaderValidation(pModPE);
    if (rc)
        return rc;
    for (i = 0; i < pModPE->Hdrs.FileHeader.NumberOfSections; i++)
    {
        rc = kLdrModPEDoSectionHeadersValidation(pModPE);
        if (rc)
            return rc;
    }

    /*
     * Setup the KLDRMOD segment array.
     */
    /* The implied headers section. */
    pMod->aSegments[0].pvUser = NULL;
    pMod->aSegments[0].pchName = "TheHeaders";
    pMod->aSegments[0].cchName = sizeof("TheHeaders") - 1;
    pMod->aSegments[0].enmProt = KPROT_READONLY;
    pMod->aSegments[0].cb = pModPE->Hdrs.OptionalHeader.SizeOfHeaders;
    pMod->aSegments[0].Alignment = pModPE->Hdrs.OptionalHeader.SectionAlignment;
    pMod->aSegments[0].LinkAddress = pModPE->Hdrs.OptionalHeader.ImageBase;
    pMod->aSegments[0].offFile = 0;
    pMod->aSegments[0].cbFile = pModPE->Hdrs.OptionalHeader.SizeOfHeaders;
    pMod->aSegments[0].RVA = 0;
    if (pMod->cSegments > 1)
        pMod->aSegments[0].cbMapped = pModPE->aShdrs[0].VirtualAddress;
    else
        pMod->aSegments[0].cbMapped = pModPE->Hdrs.OptionalHeader.SizeOfHeaders;
    pMod->aSegments[0].MapAddress = 0;

    /* The section headers. */
    for (i = 0; i < pModPE->Hdrs.FileHeader.NumberOfSections; i++)
    {
        const char *pch;
        KU32        cb2;

        /* unused */
        pMod->aSegments[i + 1].pvUser = NULL;
        pMod->aSegments[i + 1].MapAddress = 0;
        pMod->aSegments[i + 1].SelFlat = 0;
        pMod->aSegments[i + 1].Sel16bit = 0;
        pMod->aSegments[i + 1].fFlags = 0;

        /* name */
        pMod->aSegments[i + 1].pchName = pch = (const char *)&pModPE->aShdrs[i].Name[0];
        cb2 = IMAGE_SIZEOF_SHORT_NAME;
        while (   cb2 > 0
               && (pch[cb2 - 1] == ' ' || pch[cb2 - 1] == '\0'))
            cb2--;
        pMod->aSegments[i + 1].cchName = cb2;

        /* size and addresses */
        if (!(pModPE->aShdrs[i].Characteristics & IMAGE_SCN_TYPE_NOLOAD))
        {
            /* Kluge to deal with wlink ".reloc" sections that has a VirtualSize of 0 bytes. */
            cb2 = pModPE->aShdrs[i].Misc.VirtualSize;
            if (!cb2)
                cb2 = K_ALIGN_Z(pModPE->aShdrs[i].SizeOfRawData, pModPE->Hdrs.OptionalHeader.SectionAlignment);
            pMod->aSegments[i + 1].cb          = pModPE->aShdrs[i].Misc.VirtualSize;
            pMod->aSegments[i + 1].LinkAddress = pModPE->aShdrs[i].VirtualAddress
                                               + pModPE->Hdrs.OptionalHeader.ImageBase;
            pMod->aSegments[i + 1].RVA         = pModPE->aShdrs[i].VirtualAddress;
            pMod->aSegments[i + 1].cbMapped    = cb2;
            if (i + 2 < pMod->cSegments)
                pMod->aSegments[i + 1].cbMapped= pModPE->aShdrs[i + 1].VirtualAddress
                                               - pModPE->aShdrs[i].VirtualAddress;
        }
        else
        {
            pMod->aSegments[i + 1].cb          = 0;
            pMod->aSegments[i + 1].cbMapped    = 0;
            pMod->aSegments[i + 1].LinkAddress = NIL_KLDRADDR;
            pMod->aSegments[i + 1].RVA         = 0;
        }

        /* file location */
        pMod->aSegments[i + 1].offFile = pModPE->aShdrs[i].PointerToRawData;
        pMod->aSegments[i + 1].cbFile  = pModPE->aShdrs[i].SizeOfRawData;
        if (    pMod->aSegments[i + 1].cbMapped > 0 /* if mapped */
            &&  (KLDRSIZE)pMod->aSegments[i + 1].cbFile > pMod->aSegments[i + 1].cbMapped)
            pMod->aSegments[i + 1].cbFile = (KLDRFOFF)(pMod->aSegments[i + 1].cbMapped);

        /* protection */
        switch (  pModPE->aShdrs[i].Characteristics
                & (IMAGE_SCN_MEM_SHARED | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE))
        {
            case 0:
            case IMAGE_SCN_MEM_SHARED:
                pMod->aSegments[i + 1].enmProt = KPROT_NOACCESS;
                break;
            case IMAGE_SCN_MEM_READ:
            case IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_SHARED:
                pMod->aSegments[i + 1].enmProt = KPROT_READONLY;
                break;
            case IMAGE_SCN_MEM_WRITE:
            case IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ:
                pMod->aSegments[i + 1].enmProt = KPROT_WRITECOPY;
                break;
            case IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_SHARED:
            case IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_SHARED | IMAGE_SCN_MEM_READ:
                pMod->aSegments[i + 1].enmProt = KPROT_READWRITE;
                break;
            case IMAGE_SCN_MEM_EXECUTE:
            case IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_SHARED:
                pMod->aSegments[i + 1].enmProt = KPROT_EXECUTE;
                break;
            case IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ:
            case IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_SHARED:
                pMod->aSegments[i + 1].enmProt = KPROT_EXECUTE_READ;
                break;
            case IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_WRITE:
            case IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ:
                pMod->aSegments[i + 1].enmProt = KPROT_EXECUTE_WRITECOPY;
                break;
            case IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_SHARED:
            case IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_SHARED | IMAGE_SCN_MEM_READ:
                pMod->aSegments[i + 1].enmProt = KPROT_EXECUTE_READWRITE;
                break;
        }

        /* alignment. */
        switch (pModPE->aShdrs[i].Characteristics & IMAGE_SCN_ALIGN_MASK)
        {
            case 0: /* hope this is right... */
                pMod->aSegments[i + 1].Alignment = pModPE->Hdrs.OptionalHeader.SectionAlignment;
                break;
            case IMAGE_SCN_ALIGN_1BYTES:        pMod->aSegments[i + 1].Alignment = 1; break;
            case IMAGE_SCN_ALIGN_2BYTES:        pMod->aSegments[i + 1].Alignment = 2; break;
            case IMAGE_SCN_ALIGN_4BYTES:        pMod->aSegments[i + 1].Alignment = 4; break;
            case IMAGE_SCN_ALIGN_8BYTES:        pMod->aSegments[i + 1].Alignment = 8; break;
            case IMAGE_SCN_ALIGN_16BYTES:       pMod->aSegments[i + 1].Alignment = 16; break;
            case IMAGE_SCN_ALIGN_32BYTES:       pMod->aSegments[i + 1].Alignment = 32; break;
            case IMAGE_SCN_ALIGN_64BYTES:       pMod->aSegments[i + 1].Alignment = 64; break;
            case IMAGE_SCN_ALIGN_128BYTES:      pMod->aSegments[i + 1].Alignment = 128; break;
            case IMAGE_SCN_ALIGN_256BYTES:      pMod->aSegments[i + 1].Alignment = 256; break;
            case IMAGE_SCN_ALIGN_512BYTES:      pMod->aSegments[i + 1].Alignment = 512; break;
            case IMAGE_SCN_ALIGN_1024BYTES:     pMod->aSegments[i + 1].Alignment = 1024; break;
            case IMAGE_SCN_ALIGN_2048BYTES:     pMod->aSegments[i + 1].Alignment = 2048; break;
            case IMAGE_SCN_ALIGN_4096BYTES:     pMod->aSegments[i + 1].Alignment = 4096; break;
            case IMAGE_SCN_ALIGN_8192BYTES:     pMod->aSegments[i + 1].Alignment = 8192; break;
            default: kHlpAssert(0);          pMod->aSegments[i + 1].Alignment = 0; break;
        }
    }

    /*
     * We're done.
     */
    *ppModPE = pModPE;
    return 0;
}


/**
 * Converts a 32-bit optional header to a 64-bit one
 *
 * @param   pOptHdr     The optional header to convert.
 */
static void kldrModPEDoOptionalHeaderConversion(PIMAGE_OPTIONAL_HEADER64 pOptHdr)
{
    /* volatile everywhere! */
    IMAGE_OPTIONAL_HEADER32 volatile *pOptHdr32 = (IMAGE_OPTIONAL_HEADER32 volatile *)pOptHdr;
    IMAGE_OPTIONAL_HEADER64 volatile *pOptHdr64 = pOptHdr;
    KU32 volatile                    *pu32Dst;
    KU32 volatile                    *pu32Src;
    KU32 volatile                    *pu32SrcLast;
    KU32                              u32;

    /* From LoaderFlags and out the difference is 4 * 32-bits. */
    pu32Dst     = (KU32 *)&pOptHdr64->DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES] - 1;
    pu32Src     = (KU32 *)&pOptHdr32->DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES] - 1;
    pu32SrcLast = (KU32 *)&pOptHdr32->LoaderFlags;
    while (pu32Src >= pu32SrcLast)
        *pu32Dst-- = *pu32Src--;

    /* The previous 4 fields are 32/64 and needs special attention. */
    pOptHdr64->SizeOfHeapCommit   = pOptHdr32->SizeOfHeapCommit;
    pOptHdr64->SizeOfHeapReserve  = pOptHdr32->SizeOfHeapReserve;
    pOptHdr64->SizeOfStackCommit  = pOptHdr32->SizeOfStackCommit;
    u32 = pOptHdr32->SizeOfStackReserve;
    pOptHdr64->SizeOfStackReserve = u32;

    /*
     * The rest matches except for BaseOfData which has been merged into ImageBase in the 64-bit version.
     * Thus, ImageBase needs some special treatement. It will probably work fine assigning one to the
     * other since this is all declared volatile, but taking now chances, we'll use a temp variable.
     */
    u32 = pOptHdr32->ImageBase;
    pOptHdr64->ImageBase = u32;
}


#if 0
/**
 * Converts a 32-bit load config directory to a 64 bit one.
 *
 * @param   pOptHdr     The load config to convert.
 */
static void kldrModPEDoLoadConfigConversion(PIMAGE_LOAD_CONFIG_DIRECTORY64 pLoadCfg)
{
    /* volatile everywhere! */
    IMAGE_LOAD_CONFIG_DIRECTORY32 volatile *pLoadCfg32 = (IMAGE_LOAD_CONFIG_DIRECTORY32 volatile *)pLoadCfg;
    IMAGE_LOAD_CONFIG_DIRECTORY64 volatile *pLoadCfg64 = pLoadCfg;
    KU32                                    u32;

    pLoadCfg64->SEHandlerCount             = pLoadCfg32->SEHandlerCount;
    pLoadCfg64->SEHandlerTable             = pLoadCfg32->SEHandlerTable;
    pLoadCfg64->SecurityCookie             = pLoadCfg32->SecurityCookie;
    pLoadCfg64->EditList                   = pLoadCfg32->EditList;
    pLoadCfg64->Reserved1                  = pLoadCfg32->Reserved1;
    pLoadCfg64->CSDVersion                 = pLoadCfg32->CSDVersion;
    /* (ProcessHeapFlags switched place with ProcessAffinityMask, but we're
     * more than 16 byte off by now so it doesn't matter.) */
    pLoadCfg64->ProcessHeapFlags           = pLoadCfg32->ProcessHeapFlags;
    pLoadCfg64->ProcessAffinityMask        = pLoadCfg32->ProcessAffinityMask;
    pLoadCfg64->VirtualMemoryThreshold     = pLoadCfg32->VirtualMemoryThreshold;
    pLoadCfg64->MaximumAllocationSize      = pLoadCfg32->MaximumAllocationSize;
    pLoadCfg64->LockPrefixTable            = pLoadCfg32->LockPrefixTable;
    pLoadCfg64->DeCommitTotalFreeThreshold = pLoadCfg32->DeCommitTotalFreeThreshold;
    u32 = pLoadCfg32->DeCommitFreeBlockThreshold;
    pLoadCfg64->DeCommitFreeBlockThreshold = u32;
    /* the remainder matches. */
}
#endif


/**
 * Internal worker which validates the section headers.
 */
static int kLdrModPEDoOptionalHeaderValidation(PKLDRMODPE pModPE)
{
    const unsigned fIs32Bit = pModPE->Hdrs.FileHeader.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER32);

    /* the magic */
    if (    pModPE->Hdrs.OptionalHeader.Magic
        !=  (fIs32Bit ? IMAGE_NT_OPTIONAL_HDR32_MAGIC : IMAGE_NT_OPTIONAL_HDR64_MAGIC))
        return KLDR_ERR_PE_BAD_OPTIONAL_HEADER;

    /** @todo validate more */
    return 0;
}


/**
 * Internal worker which validates the section headers.
 */
static int kLdrModPEDoSectionHeadersValidation(PKLDRMODPE pModPE)
{
    /** @todo validate shdrs */
    K_NOREF(pModPE);
    return 0;
}


/** @copydoc KLDRMODOPS::pfnDestroy */
static int kldrModPEDestroy(PKLDRMOD pMod)
{
    PKLDRMODPE pModPE = (PKLDRMODPE)pMod->pvData;
    int rc = 0;
    KLDRMODPE_ASSERT(!pModPE->pvMapping);

    if (pMod->pRdr)
    {
        rc = kRdrClose(pMod->pRdr);
        pMod->pRdr = NULL;
    }
    pMod->u32Magic = 0;
    pMod->pOps = NULL;
    kHlpFree(pModPE);
    return rc;
}


/**
 * Performs the mapping of the image.
 *
 * This can be used to do the internal mapping as well as the
 * user requested mapping. fForReal indicates which is desired.
 *
 * @returns 0 on success, non-zero OS or kLdr status code on failure.
 * @param   pModPE          The interpreter module instance
 * @param   fForReal        If set, do the user mapping. if clear, do the internal mapping.
 */
static int kldrModPEDoMap(PKLDRMODPE pModPE, unsigned fForReal)
{
    PKLDRMOD    pMod = pModPE->pMod;
    KBOOL       fFixed;
    void       *pvBase;
    int         rc;
    KU32        i;

    /*
     * Map it.
     */
    /* fixed image? */
    fFixed = fForReal
          && (   pMod->enmType == KLDRTYPE_EXECUTABLE_FIXED
              || pMod->enmType == KLDRTYPE_SHARED_LIBRARY_FIXED);
    if (!fFixed)
        pvBase = NULL;
    else
    {
        pvBase = (void *)(KUPTR)pMod->aSegments[0].LinkAddress;
        if ((KUPTR)pvBase != pMod->aSegments[0].LinkAddress)
            return KLDR_ERR_ADDRESS_OVERFLOW;
    }

    /* try do the prepare */
    rc = kRdrMap(pMod->pRdr, &pvBase, pMod->cSegments, pMod->aSegments, fFixed);
    if (rc)
        return rc;

    /*
     * Update the segments with their map addresses.
     */
    if (fForReal)
    {
        for (i = 0; i < pMod->cSegments; i++)
        {
            if (pMod->aSegments[i].RVA != NIL_KLDRADDR)
                pMod->aSegments[i].MapAddress = (KUPTR)pvBase + (KUPTR)pMod->aSegments[i].RVA;
        }
        pModPE->pvMapping = pvBase;
    }
    else
        pModPE->pvBits = pvBase;
    return 0;
}


/**
 * Unmaps a image mapping.
 *
 * This can be used to do the internal mapping as well as the
 * user requested mapping. fForReal indicates which is desired.
 *
 * @returns 0 on success, non-zero OS or kLdr status code on failure.
 * @param   pModPE          The interpreter module instance
 * @param   pvMapping       The mapping to unmap.
 */
static int kldrModPEDoUnmap(PKLDRMODPE pModPE, const void *pvMapping)
{
    PKLDRMOD    pMod = pModPE->pMod;
    int         rc;
    KU32        i;

    /*
     * Try unmap the image.
     */
    rc = kRdrUnmap(pMod->pRdr, (void *)pvMapping, pMod->cSegments, pMod->aSegments);
    if (rc)
        return rc;

    /*
     * Update the segments to reflect that they aren't mapped any longer.
     */
    if (pModPE->pvMapping == pvMapping)
    {
        pModPE->pvMapping = NULL;
        for (i = 0; i < pMod->cSegments; i++)
            pMod->aSegments[i].MapAddress = 0;
    }
    if (pModPE->pvBits == pvMapping)
        pModPE->pvBits = NULL;

    return 0;
}


/**
 * Gets usable bits and the right base address.
 *
 * @returns 0 on success.
 * @returns A non-zero status code if the BaseAddress isn't right or some problem is encountered
 *          featch in a temp mapping the bits.
 * @param   pModPE          The interpreter module instance
 * @param   ppvBits         The bits address, IN & OUT.
 * @param   pBaseAddress    The base address, IN & OUT. Optional.
 */
static int kldrModPEBitsAndBaseAddress(PKLDRMODPE pModPE, const void **ppvBits, PKLDRADDR pBaseAddress)
{
    int rc = 0;

    /*
     * Correct the base address.
     *
     * We don't use the base address for interpreting the bits in this
     * interpreter, which makes things relativly simple.
     */
    if (pBaseAddress)
    {
        if (*pBaseAddress == KLDRMOD_BASEADDRESS_MAP)
            *pBaseAddress = pModPE->pMod->aSegments[0].MapAddress;
        else if (*pBaseAddress == KLDRMOD_BASEADDRESS_LINK)
            *pBaseAddress = pModPE->Hdrs.OptionalHeader.ImageBase;
    }

    /*
     * Get bits.
     */
    if (ppvBits && !*ppvBits)
    {
        if (pModPE->pvMapping)
            *ppvBits = pModPE->pvMapping;
        else if (pModPE->pvBits)
            *ppvBits = pModPE->pvBits;
        else
        {
            /* create an internal mapping. */
            rc = kldrModPEDoMap(pModPE, 0 /* not for real */);
            if (rc)
                return rc;
            KLDRMODPE_ASSERT(pModPE->pvBits);
            *ppvBits = pModPE->pvBits;
        }
    }

    return 0;
}


/** @copydoc kLdrModQuerySymbol */
static int kldrModPEQuerySymbol(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, KU32 iSymbol,
                                const char *pchSymbol, KSIZE cchSymbol, const char *pszVersion,
                                PFNKLDRMODGETIMPORT pfnGetForwarder, void *pvUser, PKLDRADDR puValue, KU32 *pfKind)

{
    PKLDRMODPE                      pModPE = (PKLDRMODPE)pMod->pvData;
    const KU32                     *paExportRVAs;
    const IMAGE_EXPORT_DIRECTORY   *pExpDir;
    KU32                            iExpOrd;
    KU32                            uRVA;
    int                             rc;

    /*
     * Make sure we've got mapped bits and resolve any base address aliases.
     */
    rc = kldrModPEBitsAndBaseAddress(pModPE, &pvBits, &BaseAddress);
    if (rc)
        return rc;
    if (    pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size
        <   sizeof(IMAGE_EXPORT_DIRECTORY))
        return KLDR_ERR_SYMBOL_NOT_FOUND;
    if (pszVersion && *pszVersion)
        return KLDR_ERR_SYMBOL_NOT_FOUND;

    pExpDir = KLDRMODPE_RVA2TYPE(pvBits,
                                 pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,
                                 PIMAGE_EXPORT_DIRECTORY);
    if (!pchSymbol)
    {
        /*
         * Simple, calculate the unbased ordinal and bounds check it.
         */
        iExpOrd = iSymbol - pExpDir->Base;
        if (iExpOrd >= K_MAX(pExpDir->NumberOfNames, pExpDir->NumberOfFunctions))
            return KLDR_ERR_SYMBOL_NOT_FOUND;
    }
    else
    {
        /*
         * Do a binary search for the name.
         * (The name table is sorted in ascending ordered by the linker.)
         */
        const KU32 *paRVANames = KLDRMODPE_RVA2TYPE(pvBits, pExpDir->AddressOfNames, const KU32 *);
        const KU16 *paOrdinals = KLDRMODPE_RVA2TYPE(pvBits, pExpDir->AddressOfNameOrdinals, const KU16 *);
        KI32        iStart = 1; /* one based binary searching is simpler. */
        KI32        iEnd = pExpDir->NumberOfNames;

        for (;;)
        {
            KI32        i;
            int         diff;
            const char *pszName;

            /* done? */
            if (iStart > iEnd)
            {
#ifdef KLDRMODPE_STRICT /* Make sure the linker and we both did our job right. */
                for (i = 0; i < (KI32)pExpDir->NumberOfNames; i++)

                {
                    pszName = KLDRMODPE_RVA2TYPE(pvBits, paRVANames[i], const char *);
                    KLDRMODPE_ASSERT(kHlpStrNComp(pszName, pchSymbol, cchSymbol) || pszName[cchSymbol]);
                    KLDRMODPE_ASSERT(i == 0 || kHlpStrComp(pszName, KLDRMODPE_RVA2TYPE(pvBits, paRVANames[i - 1], const char *)));
                }
#endif
                return KLDR_ERR_SYMBOL_NOT_FOUND;
            }

            i = (iEnd - iStart) / 2 + iStart;
            pszName = KLDRMODPE_RVA2TYPE(pvBits, paRVANames[i - 1], const char *);
            diff = kHlpStrNComp(pszName, pchSymbol, cchSymbol);
            if (!diff)
                diff = pszName[cchSymbol] - 0;
            if (diff < 0)
                iStart = i + 1;     /* The symbol must be after the current name. */
            else if (diff)
                iEnd = i - 1;       /* The symbol must be before the current name. */
            else
            {
                iExpOrd = paOrdinals[i - 1];    /* match! */
                break;
            }
        }
    }

    /*
     * Lookup the address in the 'symbol' table.
     */
    paExportRVAs = KLDRMODPE_RVA2TYPE(pvBits, pExpDir->AddressOfFunctions, const KU32 *);
    uRVA = paExportRVAs[iExpOrd];
    if (    uRVA - pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress
        <   pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size)
        return kldrModPEDoForwarderQuery(pModPE, pvBits, KLDRMODPE_RVA2TYPE(pvBits, uRVA, const char *),
                                         pfnGetForwarder, pvUser, puValue, pfKind);

    /*
     * Set the return value.
     */
    if (puValue)
        *puValue = BaseAddress + uRVA;
    if (pfKind)
        *pfKind = (pModPE->Hdrs.FileHeader.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER32)
                   ? KLDRSYMKIND_32BIT : KLDRSYMKIND_64BIT)
                | KLDRSYMKIND_NO_TYPE;
    return 0;
}


/**
 * Deal with a forwarder entry.
 *
 * We do this seprately from kldrModPEQuerySymbol because the code is clumsy (as is all PE code
 * thanks to the descriptive field names), and because it uses quite a bit more stack and we're
 * trying to avoid allocating stack unless we have to.
 *
 * @returns See kLdrModQuerySymbol.
 * @param   pModPE          The PE module interpreter instance.
 * @param   pvBits          Where to read the image from.
 * @param   pszForwarder    The forwarder entry name.
 * @param   pfnGetForwarder The callback for resolving forwarder symbols. (optional)
 * @param   pvUser          The user argument for the callback.
 * @param   puValue         Where to put the value. (optional)
 * @param   pfKind          Where to put the symbol kind. (optional)
 */
static int kldrModPEDoForwarderQuery(PKLDRMODPE pModPE, const void *pvBits, const char *pszForwarder,
                                     PFNKLDRMODGETIMPORT pfnGetForwarder, void *pvUser, PKLDRADDR puValue, KU32 *pfKind)
{
    const IMAGE_IMPORT_DESCRIPTOR *paImpDir;
    KU32            iImpModule;
    KU32            cchImpModule;
    const char     *pszSymbol;
    KU32            iSymbol;
    int             rc;

    if (!pfnGetForwarder)
        return KLDR_ERR_FORWARDER_SYMBOL;

    /*
     * Separate the name into a module name and a symbol name or ordinal.
     *
     * The module name ends at the first dot ('.').
     * After the dot follows either a symbol name or a hash ('#') + ordinal.
     */
    pszSymbol = pszForwarder;
    while (*pszSymbol != '.')
        pszSymbol++;
    if (!*pszSymbol)
        return KLDR_ERR_PE_BAD_FORWARDER;
    cchImpModule = (KU32)(pszSymbol - pszForwarder);

    pszSymbol++;                        /* skip the dot */
    if (!*pszSymbol)
        return KLDR_ERR_PE_BAD_FORWARDER;
    if (*pszSymbol == '#')
    {
        unsigned uBase;
        pszSymbol++;                    /* skip the hash */

        /* base detection */
        uBase = 10;
        if (pszSymbol[0] == '0' &&  (pszSymbol[1] == 'x' || pszSymbol[1] == 'X'))
        {
            uBase = 16;
            pszSymbol += 2;
        }

        /* ascii to integer */
        iSymbol = 0;
        for (;;)
        {
            /* convert char to digit. */
            unsigned uDigit = *pszSymbol++;
            if (uDigit >= '0' && uDigit <= '9')
                uDigit -= '0';
            else if (uDigit >= 'a' && uDigit <= 'z')
                uDigit -= 'a' + 10;
            else if (uDigit >= 'A' && uDigit <= 'Z')
                uDigit -= 'A' + 10;
            else if (!uDigit)
                break;
            else
                return KLDR_ERR_PE_BAD_FORWARDER;
            if (uDigit >= uBase)
                return KLDR_ERR_PE_BAD_FORWARDER;

            /* insert the digit */
            iSymbol *= uBase;
            iSymbol += uDigit;
        }

        pszSymbol = NULL;               /* no symbol name. */
    }
    else
        iSymbol = NIL_KLDRMOD_SYM_ORDINAL; /* no ordinal number. */


    /*
     * Find the import module name.
     *
     * We ASSUME the linker will make sure there is an import
     * entry for the module... not sure if this is right though.
     */
    if (    !pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size
        ||  !pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)
        return KLDR_ERR_PE_FORWARDER_IMPORT_NOT_FOUND;
    paImpDir = KLDRMODPE_RVA2TYPE(pvBits,
                                  pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress,
                                  const IMAGE_IMPORT_DESCRIPTOR *);

    kldrModPENumberOfImports(pModPE->pMod, pvBits);
    for (iImpModule = 0; iImpModule < pModPE->cImportModules; iImpModule++)
    {
        const char *pszName = KLDRMODPE_RVA2TYPE(pvBits, paImpDir[iImpModule].Name, const char *);
        KSIZE       cchName = kHlpStrLen(pszName);
        if (    (   cchName == cchImpModule
                 || (   cchName > cchImpModule
                     && pszName[cchImpModule] == '.'
                     && (pszName[cchImpModule + 1] == 'd' || pszName[cchImpModule + 1] == 'D')
                     && (pszName[cchImpModule + 2] == 'l' || pszName[cchImpModule + 2] == 'L')
                     && (pszName[cchImpModule + 3] == 'l' || pszName[cchImpModule + 3] == 'L'))
                )
            &&  kHlpMemICompAscii(pszName, pszForwarder, cchImpModule)
           )
        {
            /*
             * Now the rest is up to the callback (almost).
             */
            rc = pfnGetForwarder(pModPE->pMod, iImpModule, iSymbol, pszSymbol,
                                 pszSymbol ? kHlpStrLen(pszSymbol) : 0, NULL, puValue, pfKind, pvUser);
            if (!rc && pfKind)
                *pfKind |= KLDRSYMKIND_FORWARDER;
            return rc;
        }
    }
    return KLDR_ERR_PE_FORWARDER_IMPORT_NOT_FOUND;
}


/** @copydoc kLdrModEnumSymbols */
static int kldrModPEEnumSymbols(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress,
                                KU32 fFlags, PFNKLDRMODENUMSYMS pfnCallback, void *pvUser)
{
    PKLDRMODPE                      pModPE = (PKLDRMODPE)pMod->pvData;
    const KU32                     *paFunctions;
    const IMAGE_EXPORT_DIRECTORY   *pExpDir;
    const KU32                     *paRVANames;
    const KU16                     *paOrdinals;
    KU32                            iFunction;
    KU32                            cFunctions;
    KU32                            cNames;
    int                             rc;
    K_NOREF(fFlags);

    /*
     * Make sure we've got mapped bits and resolve any base address aliases.
     */
    rc = kldrModPEBitsAndBaseAddress(pModPE, &pvBits, &BaseAddress);
    if (rc)
        return rc;

    if (    pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size
        <   sizeof(IMAGE_EXPORT_DIRECTORY))
        return 0; /* no exports to enumerate, return success. */

    pExpDir = KLDRMODPE_RVA2TYPE(pvBits,
                                 pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,
                                 PIMAGE_EXPORT_DIRECTORY);

    /*
     * Enumerate the ordinal exports.
     */
    paRVANames = KLDRMODPE_RVA2TYPE(pvBits, pExpDir->AddressOfNames, const KU32 *);
    paOrdinals = KLDRMODPE_RVA2TYPE(pvBits, pExpDir->AddressOfNameOrdinals, const KU16 *);
    paFunctions = KLDRMODPE_RVA2TYPE(pvBits, pExpDir->AddressOfFunctions, const KU32 *);
    cFunctions = pExpDir->NumberOfFunctions;
    cNames = pExpDir->NumberOfNames;
    for (iFunction = 0; iFunction < cFunctions; iFunction++)
    {
        unsigned        fFoundName;
        KU32            iName;
        const KU32      uRVA = paFunctions[iFunction];
        const KLDRADDR  uValue = BaseAddress + uRVA;
        KU32            fKind = (pModPE->Hdrs.FileHeader.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER32)
                              ? KLDRSYMKIND_32BIT : KLDRSYMKIND_64BIT)
                              | KLDRSYMKIND_NO_TYPE;
        if (    uRVA - pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress
            <   pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size)
            fKind |= KLDRSYMKIND_FORWARDER;

        /*
         * Any symbol names?
         */
        fFoundName = 0;
        for (iName = 0; iName < cNames; iName++)
        {
            const char *pszName;
            if (paOrdinals[iName] != iFunction)
                continue;
            fFoundName = 1;
            pszName = KLDRMODPE_RVA2TYPE(pvBits, paRVANames[iName], const char *);
            rc = pfnCallback(pMod, iFunction + pExpDir->Base, pszName, kHlpStrLen(pszName), NULL,
                             uValue, fKind, pvUser);
            if (rc)
                return rc;
        }

        /*
         * If no names, call once with the ordinal only.
         */
        if (!fFoundName)
        {
            rc = pfnCallback(pMod, iFunction + pExpDir->Base, NULL, 0, NULL, uValue, fKind, pvUser);
            if (rc)
                return rc;
        }
    }

    return 0;
}


/** @copydoc kLdrModGetImport */
static int kldrModPEGetImport(PKLDRMOD pMod, const void *pvBits, KU32 iImport, char *pszName, KSIZE cchName)
{
    PKLDRMODPE                      pModPE = (PKLDRMODPE)pMod->pvData;
    const IMAGE_IMPORT_DESCRIPTOR  *pImpDesc;
    const char                     *pszImportName;
    KSIZE                           cchImportName;
    int                             rc;

    /*
     * Make sure we've got mapped bits and resolve any base address aliases.
     */
    rc = kldrModPEBitsAndBaseAddress(pModPE, &pvBits, NULL);
    if (rc)
        return rc;

    /*
     * Simple bounds check.
     */
    if (iImport >= (KU32)kldrModPENumberOfImports(pMod, pvBits))
        return KLDR_ERR_IMPORT_ORDINAL_OUT_OF_BOUNDS;

    /*
     * Get the name.
     */
    pImpDesc = KLDRMODPE_RVA2TYPE(pvBits,
                                  pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
                                  + sizeof(IMAGE_IMPORT_DESCRIPTOR) * iImport,
                                  const IMAGE_IMPORT_DESCRIPTOR *);
    pszImportName = KLDRMODPE_RVA2TYPE(pvBits, pImpDesc->Name, const char *);
    cchImportName = kHlpStrLen(pszImportName);
    if (cchImportName < cchName)
    {
        kHlpMemCopy(pszName, pszImportName, cchImportName + 1);
        rc = 0;
    }
    else
    {
        kHlpMemCopy(pszName, pszImportName, cchName);
        if (cchName)
            pszName[cchName - 1] = '\0';
        rc = KERR_BUFFER_OVERFLOW;
    }

    return rc;
}


/** @copydoc kLdrModNumberOfImports */
static KI32 kldrModPENumberOfImports(PKLDRMOD pMod, const void *pvBits)
{
    PKLDRMODPE pModPE = (PKLDRMODPE)pMod->pvData;
    if (pModPE->cImportModules == ~(KU32)0)
    {
        /*
         * We'll have to walk the import descriptors to figure out their number.
         * First, make sure we've got mapped bits.
         */
        if (kldrModPEBitsAndBaseAddress(pModPE, &pvBits, NULL))
            return -1;
        pModPE->cImportModules = 0;
        if (    pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size
            &&  pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)
        {
            const IMAGE_IMPORT_DESCRIPTOR  *pImpDesc;

            pImpDesc = KLDRMODPE_RVA2TYPE(pvBits,
                                          pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress,
                                          const IMAGE_IMPORT_DESCRIPTOR *);
            while (pImpDesc->Name && pImpDesc->FirstThunk)
            {
                pModPE->cImportModules++;
                pImpDesc++;
            }
        }
    }
    return pModPE->cImportModules;
}


/** @copydoc kLdrModGetStackInfo */
static int kldrModPEGetStackInfo(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, PKLDRSTACKINFO pStackInfo)
{
    PKLDRMODPE pModPE = (PKLDRMODPE)pMod->pvData;
    K_NOREF(pvBits);
    K_NOREF(BaseAddress);

    pStackInfo->Address = NIL_KLDRADDR;
    pStackInfo->LinkAddress = NIL_KLDRADDR;
    pStackInfo->cbStack = pStackInfo->cbStackThread = pModPE->Hdrs.OptionalHeader.SizeOfStackReserve;

    return 0;
}


/** @copydoc kLdrModQueryMainEntrypoint */
static int kldrModPEQueryMainEntrypoint(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, PKLDRADDR pMainEPAddress)
{
    PKLDRMODPE pModPE = (PKLDRMODPE)pMod->pvData;
    int rc;
    K_NOREF(pvBits);

    /*
     * Resolve base address alias if any.
     */
    rc = kldrModPEBitsAndBaseAddress(pModPE, NULL, &BaseAddress);
    if (rc)
        return rc;

    /*
     * Convert the address from the header.
     */
    *pMainEPAddress = pModPE->Hdrs.OptionalHeader.AddressOfEntryPoint
        ? BaseAddress + pModPE->Hdrs.OptionalHeader.AddressOfEntryPoint
        : NIL_KLDRADDR;
    return 0;
}


/** @copydoc kLdrModEnumDbgInfo */
static int kldrModPEEnumDbgInfo(PKLDRMOD pMod, const void *pvBits, PFNKLDRENUMDBG pfnCallback, void *pvUser)
{
    PKLDRMODPE                      pModPE = (PKLDRMODPE)pMod->pvData;
    const IMAGE_DEBUG_DIRECTORY    *pDbgDir;
    KU32                            iDbgInfo;
    KU32                            cb;
    int                             rc;

    /*
     * Check that there is a debug directory first.
     */
    cb = pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
    if (    cb < sizeof(IMAGE_DEBUG_DIRECTORY) /* screw borland linkers */
        ||  !pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress)
        return 0;

    /*
     * Make sure we've got mapped bits.
     */
    rc = kldrModPEBitsAndBaseAddress(pModPE, &pvBits, NULL);
    if (rc)
        return rc;

    /*
     * Enumerate the debug directory.
     */
    pDbgDir = KLDRMODPE_RVA2TYPE(pvBits,
                                 pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress,
                                 const IMAGE_DEBUG_DIRECTORY *);
    for (iDbgInfo = 0;; iDbgInfo++, pDbgDir++, cb -= sizeof(IMAGE_DEBUG_DIRECTORY))
    {
        KLDRDBGINFOTYPE     enmDbgInfoType;

        /* convert the type. */
        switch (pDbgDir->Type)
        {
            case IMAGE_DEBUG_TYPE_UNKNOWN:
            case IMAGE_DEBUG_TYPE_FPO:
            case IMAGE_DEBUG_TYPE_COFF: /*stabs dialect??*/
            case IMAGE_DEBUG_TYPE_MISC:
            case IMAGE_DEBUG_TYPE_EXCEPTION:
            case IMAGE_DEBUG_TYPE_FIXUP:
            case IMAGE_DEBUG_TYPE_BORLAND:
            default:
                enmDbgInfoType = KLDRDBGINFOTYPE_UNKNOWN;
                break;
            case IMAGE_DEBUG_TYPE_CODEVIEW:
                enmDbgInfoType = KLDRDBGINFOTYPE_CODEVIEW;
                break;
        }

        rc = pfnCallback(pMod, iDbgInfo,
                         enmDbgInfoType, pDbgDir->MajorVersion, pDbgDir->MinorVersion, NULL,
                         pDbgDir->PointerToRawData ? (KLDRFOFF)pDbgDir->PointerToRawData : -1,
                         pDbgDir->AddressOfRawData ? pDbgDir->AddressOfRawData : NIL_KLDRADDR,
                         pDbgDir->SizeOfData,
                         NULL,
                         pvUser);
        if (rc)
            break;

        /* next */
        if (cb <= sizeof(IMAGE_DEBUG_DIRECTORY))
            break;
    }

    return rc;
}


/** @copydoc kLdrModHasDbgInfo */
static int kldrModPEHasDbgInfo(PKLDRMOD pMod, const void *pvBits)
{
    PKLDRMODPE pModPE = (PKLDRMODPE)pMod->pvData;
    K_NOREF(pvBits);

    /*
     * Base this entirely on the presence of a debug directory.
     */
    if (    pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size
            < sizeof(IMAGE_DEBUG_DIRECTORY) /* screw borland linkers */
        ||  !pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress)
        return KLDR_ERR_NO_DEBUG_INFO;
    return 0;
}


/** @copydoc kLdrModMap */
static int kldrModPEMap(PKLDRMOD pMod)
{
    PKLDRMODPE  pModPE = (PKLDRMODPE)pMod->pvData;
    int         rc;

    /*
     * Already mapped?
     */
    if (pModPE->pvMapping)
        return KLDR_ERR_ALREADY_MAPPED;

    /*
     * We've got a common worker which does this.
     */
    rc = kldrModPEDoMap(pModPE, 1 /* the real thing */);
    if (rc)
        return rc;
    KLDRMODPE_ASSERT(pModPE->pvMapping);
    return 0;
}


/** @copydoc kLdrModUnmap */
static int kldrModPEUnmap(PKLDRMOD pMod)
{
    PKLDRMODPE  pModPE = (PKLDRMODPE)pMod->pvData;
    int         rc;

    /*
     * Mapped?
     */
    if (!pModPE->pvMapping)
        return KLDR_ERR_NOT_MAPPED;

    /*
     * We've got a common worker which does this.
     */
    rc = kldrModPEDoUnmap(pModPE, pModPE->pvMapping);
    if (rc)
        return rc;
    KLDRMODPE_ASSERT(!pModPE->pvMapping);
    return 0;

}


/** @copydoc kLdrModAllocTLS */
static int kldrModPEAllocTLS(PKLDRMOD pMod, void *pvMapping)
{
    PKLDRMODPE  pModPE = (PKLDRMODPE)pMod->pvData;

    /*
     * Mapped?
     */
    if (pvMapping == KLDRMOD_INT_MAP)
    {
        pvMapping = (void *)pModPE->pvMapping;
        if (!pvMapping)
            return KLDR_ERR_NOT_MAPPED;
    }

    /*
     * If no TLS directory then there is nothing to do.
     */
    if (    !pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size
        ||  !pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress)
        return 0;
    /** @todo implement TLS. */
    return -1;
}


/** @copydoc kLdrModFreeTLS */
static void kldrModPEFreeTLS(PKLDRMOD pMod, void *pvMapping)
{
    PKLDRMODPE  pModPE = (PKLDRMODPE)pMod->pvData;

    /*
     * Mapped?
     */
    if (pvMapping == KLDRMOD_INT_MAP)
    {
        pvMapping = (void *)pModPE->pvMapping;
        if (!pvMapping)
            return;
    }

    /*
     * If no TLS directory then there is nothing to do.
     */
    if (    !pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size
        ||  !pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress)
        return;
    /** @todo implement TLS. */
    return;
}


/** @copydoc kLdrModReload */
static int kldrModPEReload(PKLDRMOD pMod)
{
    PKLDRMODPE pModPE = (PKLDRMODPE)pMod->pvData;

    /*
     * Mapped?
     */
    if (!pModPE->pvMapping)
        return KLDR_ERR_NOT_MAPPED;

    /* the file provider does it all */
    return kRdrRefresh(pMod->pRdr, (void *)pModPE->pvMapping, pMod->cSegments, pMod->aSegments);
}


/** @copydoc kLdrModFixupMapping */
static int kldrModPEFixupMapping(PKLDRMOD pMod, PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser)
{
    PKLDRMODPE pModPE = (PKLDRMODPE)pMod->pvData;
    int rc, rc2;

    /*
     * Mapped?
     */
    if (!pModPE->pvMapping)
        return KLDR_ERR_NOT_MAPPED;

    /*
     * Before doing anything we'll have to make all pages writable.
     */
    rc = kRdrProtect(pMod->pRdr, (void *)pModPE->pvMapping, pMod->cSegments, pMod->aSegments, 1 /* unprotect */);
    if (rc)
        return rc;

    /*
     * Apply base relocations.
     */
    rc = kldrModPEDoFixups(pModPE, (void *)pModPE->pvMapping, (KUPTR)pModPE->pvMapping,
                           pModPE->Hdrs.OptionalHeader.ImageBase);

    /*
     * Resolve imports.
     */
    if (!rc)
        rc = kldrModPEDoImports(pModPE, (void *)pModPE->pvMapping, pfnGetImport, pvUser);

    /*
     * Restore protection.
     */
    rc2 = kRdrProtect(pMod->pRdr, (void *)pModPE->pvMapping, pMod->cSegments, pMod->aSegments, 0 /* protect */);
    if (!rc && rc2)
        rc = rc2;
    return rc;
}


/**
 * Applies base relocations to a (unprotected) image mapping.
 *
 * @returns 0 on success, non-zero kLdr status code on failure.
 * @param   pModPE          The PE module interpreter instance.
 * @param   pvMapping       The mapping to fixup.
 * @param   NewBaseAddress  The address to fixup the mapping to.
 * @param   OldBaseAddress  The address the mapping is currently fixed up to.
 */
static int  kldrModPEDoFixups(PKLDRMODPE pModPE, void *pvMapping, KLDRADDR NewBaseAddress, KLDRADDR OldBaseAddress)
{
    const KLDRADDR                  Delta = NewBaseAddress - OldBaseAddress;
    KU32                            cbLeft = pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
    const IMAGE_BASE_RELOCATION    *pBR, *pFirstBR;

    /*
     * Don't don anything if the delta is 0 or there aren't any relocations.
     */
    if (    !Delta
        ||  !cbLeft
        ||  !pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress)
        return 0;

    /*
     * Process the fixups block by block.
     * (These blocks appears to be 4KB on all archs despite the native page size.)
     */
    pBR = pFirstBR = KLDRMODPE_RVA2TYPE(pvMapping,
                                        pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress,
                                        const IMAGE_BASE_RELOCATION *);
    while (     cbLeft > sizeof(IMAGE_BASE_RELOCATION)
           &&   pBR->SizeOfBlock >= sizeof(IMAGE_BASE_RELOCATION) /* paranoia */)
    {
        union
        {
            KU8        *pu8;
            KU16       *pu16;
            KU32       *pu32;
            KU64       *pu64;
        }               uChunk,
                        u;
        const KU16 *poffFixup = (const KU16 *)(pBR + 1);
        const KU32  cbBlock = K_MIN(cbLeft, pBR->SizeOfBlock) - sizeof(IMAGE_BASE_RELOCATION); /* more caution... */
        KU32        cFixups = cbBlock / sizeof(poffFixup[0]);
        uChunk.pu8 = KLDRMODPE_RVA2TYPE(pvMapping, pBR->VirtualAddress, KU8 *);

        /*
         * Loop thru the fixups in this chunk.
         */
        while (cFixups > 0)
        {
            u.pu8 = uChunk.pu8 + (*poffFixup & 0xfff);
            switch (*poffFixup >> 12) /* ordered by value. */
            {
                /* 0 - Alignment placeholder. */
                case IMAGE_REL_BASED_ABSOLUTE:
                    break;

                /* 1 - 16-bit, add 2nd 16-bit part of the delta. (rare) */
                case IMAGE_REL_BASED_HIGH:
                    *u.pu16 += (KU16)(Delta >> 16);
                    break;

                /* 2 - 16-bit, add 1st 16-bit part of the delta. (rare) */
                case IMAGE_REL_BASED_LOW:
                    *u.pu16 += (KU16)Delta;
                    break;

                /* 3 - 32-bit, add delta. (frequent in 32-bit images) */
                case IMAGE_REL_BASED_HIGHLOW:
                    *u.pu32 += (KU32)Delta;
                    break;

                /* 4 - 16-bit, add 2nd 16-bit of the delta, sign adjust for the lower 16-bit. one arg. (rare)  */
                case IMAGE_REL_BASED_HIGHADJ:
                {
                    KI32 i32;
                    if (cFixups <= 1)
                        return KLDR_ERR_PE_BAD_FIXUP;

                    i32 = (KU32)*u.pu16 << 16;
                    i32 |= *++poffFixup; cFixups--; /* the addend argument */
                    i32 += (KU32)Delta;
                    i32 += 0x8000;
                    *u.pu16 = (KU16)(i32 >> 16);
                    break;
                }

                /* 5 - 32-bit MIPS JMPADDR, no implemented. */
                case IMAGE_REL_BASED_MIPS_JMPADDR:
                    *u.pu32 = (*u.pu32 & 0xc0000000)
                            | ((KU32)((*u.pu32 << 2) + (KU32)Delta) >> 2);
                    break;

                /* 6 - Intra section? Reserved value in later specs. Not implemented. */
                case IMAGE_REL_BASED_SECTION:
                    KLDRMODPE_ASSERT(!"SECTION");
                    return KLDR_ERR_PE_BAD_FIXUP;

                /* 7 - Relative intra section? Reserved value in later specs. Not implemented. */
                case IMAGE_REL_BASED_REL32:
                    KLDRMODPE_ASSERT(!"SECTION");
                    return KLDR_ERR_PE_BAD_FIXUP;

                /* 8 - reserved according to binutils... */
                case 8:
                    KLDRMODPE_ASSERT(!"RESERVERED8");
                    return KLDR_ERR_PE_BAD_FIXUP;

                /* 9 - IA64_IMM64 (/ MIPS_JMPADDR16), no specs nor need to support the platform yet.
                 * Bet this requires more code than all the other fixups put together in good IA64 spirit :-) */
                case IMAGE_REL_BASED_IA64_IMM64:
                    KLDRMODPE_ASSERT(!"IA64_IMM64 / MIPS_JMPADDR16");
                    return KLDR_ERR_PE_BAD_FIXUP;

                /* 10 - 64-bit, add delta. (frequently in 64-bit images) */
                case IMAGE_REL_BASED_DIR64:
                    *u.pu64 += (KU64)Delta;
                    break;

                /* 11 - 16-bit, add 3rd 16-bit of the delta, sign adjust for the lower 32-bit. two args. (rare) */
                case IMAGE_REL_BASED_HIGH3ADJ:
                {
                    KI64 i64;
                    if (cFixups <= 2)
                        return KLDR_ERR_PE_BAD_FIXUP;

                    i64 = (KU64)*u.pu16 << 32
                        | ((KU32)poffFixup[2] << 16)
                        | poffFixup[1];
                    i64 += Delta;
                    i64 += 0x80008000UL;
                    *u.pu16 = (KU16)(i64 >> 32);
                    /* skip the addends arguments */
                    poffFixup += 2;
                    cFixups -= 2;
                    break;
                }

                /* the rest are yet to be defined.*/
                default:
                    return KLDR_ERR_PE_BAD_FIXUP;
            }

            /*
             * Next relocation.
             */
            poffFixup++;
            cFixups--;
        }


        /*
         * Next block.
         */
        cbLeft -= pBR->SizeOfBlock;
        pBR = (PIMAGE_BASE_RELOCATION)((KUPTR)pBR + pBR->SizeOfBlock);
    }

    return 0;
}



/**
 * Resolves imports.
 *
 * @returns 0 on success, non-zero kLdr status code on failure.
 * @param   pModPE          The PE module interpreter instance.
 * @param   pvMapping       The mapping which imports should be resolved.
 * @param   pfnGetImport    The callback for resolving an imported symbol.
 * @param   pvUser          User argument to the callback.
 */
static int  kldrModPEDoImports(PKLDRMODPE pModPE, void *pvMapping, PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser)
{
    const IMAGE_IMPORT_DESCRIPTOR *pImpDesc;

    /*
     * If no imports, there is nothing to do.
     */
    kldrModPENumberOfImports(pModPE->pMod, pvMapping);
    if (!pModPE->cImportModules)
        return 0;

    pImpDesc = KLDRMODPE_RVA2TYPE(pvMapping,
                                  pModPE->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress,
                                  const IMAGE_IMPORT_DESCRIPTOR *);
    if (pModPE->Hdrs.FileHeader.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER32))
        return kldrModPEDoImports32Bit(pModPE, pvMapping, pImpDesc, pfnGetImport, pvUser);
    return kldrModPEDoImports64Bit(pModPE, pvMapping, pImpDesc, pfnGetImport, pvUser);
}


/**
 * Resolves imports, 32-bit image.
 *
 * @returns 0 on success, non-zero kLdr status code on failure.
 * @param   pModPE          The PE module interpreter instance.
 * @param   pvMapping       The mapping which imports should be resolved.
 * @param   pImpDesc        Pointer to the first import descriptor.
 * @param   pfnGetImport    The callback for resolving an imported symbol.
 * @param   pvUser          User argument to the callback.
 */
static int  kldrModPEDoImports32Bit(PKLDRMODPE pModPE, void *pvMapping, const IMAGE_IMPORT_DESCRIPTOR *pImpDesc,
                                    PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser)
{
    PKLDRMOD pMod = pModPE->pMod;
    KU32 iImp;

    /*
     * Iterate the import descriptors.
     */
    for (iImp = 0; iImp < pModPE->cImportModules; iImp++, pImpDesc++)
    {
        PIMAGE_THUNK_DATA32         pFirstThunk = KLDRMODPE_RVA2TYPE(pvMapping, pImpDesc->FirstThunk, PIMAGE_THUNK_DATA32);
        const IMAGE_THUNK_DATA32   *pThunk = pImpDesc->u.OriginalFirstThunk
            ? KLDRMODPE_RVA2TYPE(pvMapping, pImpDesc->u.OriginalFirstThunk, const IMAGE_THUNK_DATA32 *)
            : KLDRMODPE_RVA2TYPE(pvMapping, pImpDesc->FirstThunk, const IMAGE_THUNK_DATA32 *);

        /* Iterate the thunks. */
        while (pThunk->u1.Ordinal != 0)
        {
            KLDRADDR    Value;
            KU32        fKind = KLDRSYMKIND_REQ_FLAT;
            int         rc;

            /* Ordinal or name import? */
            if (IMAGE_SNAP_BY_ORDINAL32(pThunk->u1.Ordinal))
                rc = pfnGetImport(pMod, iImp, IMAGE_ORDINAL32(pThunk->u1.Ordinal), NULL, 0, NULL, &Value, &fKind, pvUser);
            else if (KLDRMODPE_VALID_RVA(pModPE, pThunk->u1.Ordinal))
            {
                const IMAGE_IMPORT_BY_NAME *pName = KLDRMODPE_RVA2TYPE(pvMapping, pThunk->u1.Ordinal, const IMAGE_IMPORT_BY_NAME *);
                rc = pfnGetImport(pMod, iImp, NIL_KLDRMOD_SYM_ORDINAL, (const char *)pName->Name,
                                  kHlpStrLen((const char *)pName->Name), NULL, &Value, &fKind, pvUser);
            }
            else
            {
                KLDRMODPE_ASSERT(!"bad 32-bit import");
                return KLDR_ERR_PE_BAD_IMPORT;
            }
            if (rc)
                return rc;

            /* Apply it. */
            pFirstThunk->u1.Function = (KU32)Value;
            if (pFirstThunk->u1.Function != Value)
            {
                KLDRMODPE_ASSERT(!"overflow");
                return KLDR_ERR_ADDRESS_OVERFLOW;
            }

            /* next */
            pThunk++;
            pFirstThunk++;
        }
    }

    return 0;
}


/**
 * Resolves imports, 64-bit image.
 *
 * @returns 0 on success, non-zero kLdr status code on failure.
 * @param   pModPE          The PE module interpreter instance.
 * @param   pvMapping       The mapping which imports should be resolved.
 * @param   pImpDesc        Pointer to the first import descriptor.
 * @param   pfnGetImport    The callback for resolving an imported symbol.
 * @param   pvUser          User argument to the callback.
 */
static int  kldrModPEDoImports64Bit(PKLDRMODPE pModPE, void *pvMapping, const IMAGE_IMPORT_DESCRIPTOR *pImpDesc,
                                    PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser)
{
    PKLDRMOD pMod = pModPE->pMod;
    KU32 iImp;

    /*
     * Iterate the import descriptors.
     */
    for (iImp = 0; iImp < pModPE->cImportModules; iImp++, pImpDesc++)
    {
        PIMAGE_THUNK_DATA64         pFirstThunk = KLDRMODPE_RVA2TYPE(pvMapping, pImpDesc->FirstThunk, PIMAGE_THUNK_DATA64);
        const IMAGE_THUNK_DATA64   *pThunk = pImpDesc->u.OriginalFirstThunk
            ? KLDRMODPE_RVA2TYPE(pvMapping, pImpDesc->u.OriginalFirstThunk, const IMAGE_THUNK_DATA64 *)
            : KLDRMODPE_RVA2TYPE(pvMapping, pImpDesc->FirstThunk, const IMAGE_THUNK_DATA64 *);

        /* Iterate the thunks. */
        while (pThunk->u1.Ordinal != 0)
        {
            KLDRADDR    Value;
            KU32        fKind = KLDRSYMKIND_REQ_FLAT;
            int         rc;

            /* Ordinal or name import? */
            if (IMAGE_SNAP_BY_ORDINAL64(pThunk->u1.Ordinal))
                rc = pfnGetImport(pMod, iImp, (KU32)IMAGE_ORDINAL64(pThunk->u1.Ordinal), NULL, 0, NULL, &Value, &fKind, pvUser);
            else if (KLDRMODPE_VALID_RVA(pModPE, pThunk->u1.Ordinal))
            {
                const IMAGE_IMPORT_BY_NAME *pName = KLDRMODPE_RVA2TYPE(pvMapping, pThunk->u1.Ordinal, const IMAGE_IMPORT_BY_NAME *);
                rc = pfnGetImport(pMod, iImp, NIL_KLDRMOD_SYM_ORDINAL, (const char *)pName->Name,
                                  kHlpStrLen((const char *)pName->Name), NULL, &Value, &fKind, pvUser);
            }
            else
            {
                KLDRMODPE_ASSERT(!"bad 64-bit import");
                return KLDR_ERR_PE_BAD_IMPORT;
            }
            if (rc)
                return rc;

            /* Apply it. */
            pFirstThunk->u1.Function = Value;

            /* next */
            pThunk++;
            pFirstThunk++;
        }
    }

    return 0;
}



/** @copydoc kLdrModCallInit */
static int kldrModPECallInit(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle)
{
    PKLDRMODPE pModPE = (PKLDRMODPE)pMod->pvData;
    int rc;

    /*
     * Mapped?
     */
    if (pvMapping == KLDRMOD_INT_MAP)
    {
        pvMapping = (void *)pModPE->pvMapping;
        if (!pvMapping)
            return KLDR_ERR_NOT_MAPPED;
    }

    /*
     * Do TLS callbacks first and then call the init/term function if it's a DLL.
     */
    rc = kldrModPEDoCallTLS(pModPE, pvMapping, DLL_PROCESS_ATTACH, uHandle);
    if (    !rc
        &&  (pModPE->Hdrs.FileHeader.Characteristics & IMAGE_FILE_DLL))
    {
        rc = kldrModPEDoCallDLL(pModPE, pvMapping, DLL_PROCESS_ATTACH, uHandle);
        if (rc)
            kldrModPEDoCallTLS(pModPE, pvMapping, DLL_PROCESS_DETACH, uHandle);
    }

    return rc;
}


/**
 * Call the DLL entrypoint.
 *
 * @returns 0 on success.
 * @returns KLDR_ERR_MODULE_INIT_FAILED  or KLDR_ERR_THREAD_ATTACH_FAILED on failure.
 * @param   pModPE          The PE module interpreter instance.
 * @param   pvMapping       The module mapping to use (resolved).
 * @param   uOp             The operation (DLL_*).
 * @param   uHandle         The module handle to present.
 */
static int  kldrModPEDoCallDLL(PKLDRMODPE pModPE, void *pvMapping, unsigned uOp, KUPTR uHandle)
{
    int rc;

    /*
     * If no entrypoint there isn't anything to be done.
     */
    if (!pModPE->Hdrs.OptionalHeader.AddressOfEntryPoint)
        return 0;

    /*
     * Invoke the entrypoint and convert the boolean result to a kLdr status code.
     */
    rc = kldrModPEDoCall((KUPTR)pvMapping + pModPE->Hdrs.OptionalHeader.AddressOfEntryPoint, uHandle, uOp, NULL);
    if (rc)
        rc = 0;
    else if (uOp == DLL_PROCESS_ATTACH)
        rc = KLDR_ERR_MODULE_INIT_FAILED;
    else if (uOp == DLL_THREAD_ATTACH)
        rc = KLDR_ERR_THREAD_ATTACH_FAILED;
    else /* detach: ignore failures */
        rc = 0;
    return rc;
}


/**
 * Call the TLS entrypoints.
 *
 * @returns 0 on success.
 * @returns KLDR_ERR_THREAD_ATTACH_FAILED on failure.
 * @param   pModPE          The PE module interpreter instance.
 * @param   pvMapping       The module mapping to use (resolved).
 * @param   uOp             The operation (DLL_*).
 * @param   uHandle         The module handle to present.
 */
static int  kldrModPEDoCallTLS(PKLDRMODPE pModPE, void *pvMapping, unsigned uOp, KUPTR uHandle)
{
    /** @todo implement TLS support. */
    K_NOREF(pModPE);
    K_NOREF(pvMapping);
    K_NOREF(uOp);
    K_NOREF(uHandle);
    return 0;
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
static KI32 kldrModPEDoCall(KUPTR uEntrypoint, KUPTR uHandle, KU32 uOp, void *pvReserved)
{
    KI32 rc;

/** @todo try/except */
#if defined(__X86__) || defined(__i386__) || defined(_M_IX86)
    /*
     * Be very careful.
     * Not everyone will have got the calling convention right.
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

#elif defined(__AMD64__) || defined(__x86_64__) || defined(_M_IX86)
    /*
     * For now, let's just get the work done...
     */
    /** @todo Deal with GCC / MSC differences in some sensible way. */
    int (*pfn)(KUPTR uHandle, KU32 uOp, void *pvReserved);
    pfn = (int (*)(KUPTR uHandle, KU32 uOp, void *pvReserved))uEntrypoint;
    rc = pfn(uHandle, uOp, NULL);

#else
# error "port me"
#endif
    K_NOREF(pvReserved);

    return rc;
}


/** @copydoc kLdrModCallTerm */
static int kldrModPECallTerm(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle)
{
    PKLDRMODPE  pModPE = (PKLDRMODPE)pMod->pvData;

    /*
     * Mapped?
     */
    if (pvMapping == KLDRMOD_INT_MAP)
    {
        pvMapping = (void *)pModPE->pvMapping;
        if (!pvMapping)
            return KLDR_ERR_NOT_MAPPED;
    }

    /*
     * Do TLS callbacks first.
     */
    kldrModPEDoCallTLS(pModPE, pvMapping, DLL_PROCESS_DETACH, uHandle);
    if (pModPE->Hdrs.FileHeader.Characteristics & IMAGE_FILE_DLL)
        kldrModPEDoCallDLL(pModPE, pvMapping, DLL_PROCESS_DETACH, uHandle);

    return 0;
}


/** @copydoc kLdrModCallThread */
static int kldrModPECallThread(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle, unsigned fAttachingOrDetaching)
{
    PKLDRMODPE  pModPE = (PKLDRMODPE)pMod->pvData;
    unsigned    uOp = fAttachingOrDetaching ? DLL_THREAD_ATTACH : DLL_THREAD_DETACH;
    int         rc;

    /*
     * Mapped?
     */
    if (pvMapping == KLDRMOD_INT_MAP)
    {
        pvMapping = (void *)pModPE->pvMapping;
        if (!pvMapping)
            return KLDR_ERR_NOT_MAPPED;
    }

    /*
     * Do TLS callbacks first and then call the init/term function if it's a DLL.
     */
    rc = kldrModPEDoCallTLS(pModPE, pvMapping, uOp, uHandle);
    if (!fAttachingOrDetaching)
        rc = 0;
    if (    !rc
        &&  (pModPE->Hdrs.FileHeader.Characteristics & IMAGE_FILE_DLL))
    {
        rc = kldrModPEDoCallDLL(pModPE, pvMapping, uOp, uHandle);
        if (!fAttachingOrDetaching)
            rc = 0;
        if (rc)
            kldrModPEDoCallTLS(pModPE, pvMapping, uOp, uHandle);
    }

    return rc;
}


/** @copydoc kLdrModSize */
static KLDRADDR kldrModPESize(PKLDRMOD pMod)
{
    PKLDRMODPE pModPE = (PKLDRMODPE)pMod->pvData;
    return pModPE->Hdrs.OptionalHeader.SizeOfImage;
}


/** @copydoc kLdrModGetBits */
static int kldrModPEGetBits(PKLDRMOD pMod, void *pvBits, KLDRADDR BaseAddress, PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser)
{
    PKLDRMODPE  pModPE = (PKLDRMODPE)pMod->pvData;
    KU32        i;
    int         rc;

    /*
     * Zero the entire buffer first to simplify things.
     */
    kHlpMemSet(pvBits, 0, pModPE->Hdrs.OptionalHeader.SizeOfImage);

    /*
     * Iterate the segments and read the data within them.
     */
    for (i = 0; i < pMod->cSegments; i++)
    {
        /* skip it? */
        if (    pMod->aSegments[i].cbFile == -1
            ||  pMod->aSegments[i].offFile == -1
            ||  pMod->aSegments[i].LinkAddress == NIL_KLDRADDR
            ||  !pMod->aSegments[i].Alignment)
            continue;
        rc = kRdrRead(pMod->pRdr,
                         (KU8 *)pvBits + (pMod->aSegments[i].LinkAddress - pModPE->Hdrs.OptionalHeader.ImageBase),
                         pMod->aSegments[i].cbFile,
                         pMod->aSegments[i].offFile);
        if (rc)
            return rc;
    }

    /*
     * Perform relocations.
     */
    return kldrModPERelocateBits(pMod, pvBits, BaseAddress, pModPE->Hdrs.OptionalHeader.ImageBase, pfnGetImport, pvUser);

}


/** @copydoc kLdrModRelocateBits */
static int kldrModPERelocateBits(PKLDRMOD pMod, void *pvBits, KLDRADDR NewBaseAddress, KLDRADDR OldBaseAddress,
                                 PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser)
{
    PKLDRMODPE pModPE = (PKLDRMODPE)pMod->pvData;
    int rc;

    /*
     * Call workers to do the jobs.
     */
    rc = kldrModPEDoFixups(pModPE, pvBits, NewBaseAddress, OldBaseAddress);
    if (!rc)
        rc = kldrModPEDoImports(pModPE, pvBits, pfnGetImport, pvUser);

    return rc;
}


/**
 * The PE module interpreter method table.
 */
KLDRMODOPS g_kLdrModPEOps =
{
    "PE",
    NULL,
    kldrModPECreate,
    kldrModPEDestroy,
    kldrModPEQuerySymbol,
    kldrModPEEnumSymbols,
    kldrModPEGetImport,
    kldrModPENumberOfImports,
    NULL /* can execute one is optional */,
    kldrModPEGetStackInfo,
    kldrModPEQueryMainEntrypoint,
    NULL /* pfnQueryImageUuid */,
    NULL, /** @todo resources */
    NULL, /** @todo resources */
    kldrModPEEnumDbgInfo,
    kldrModPEHasDbgInfo,
    kldrModPEMap,
    kldrModPEUnmap,
    kldrModPEAllocTLS,
    kldrModPEFreeTLS,
    kldrModPEReload,
    kldrModPEFixupMapping,
    kldrModPECallInit,
    kldrModPECallTerm,
    kldrModPECallThread,
    kldrModPESize,
    kldrModPEGetBits,
    kldrModPERelocateBits,
    NULL, /** @todo mostly done */
    42 /* the end */
};
