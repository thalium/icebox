/* $Id: kLdrModNative.c 82 2016-08-22 21:01:51Z bird $ */
/** @file
 * kLdr - The Module Interpreter for the Native Loaders.
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

#if K_OS == K_OS_OS2
# define INCL_BASE
# include <os2.h>

# ifndef LIBPATHSTRICT
#  define LIBPATHSTRICT 3
# endif
  extern APIRET DosQueryHeaderInfo(HMODULE hmod, ULONG ulIndex, PVOID pvBuffer, ULONG cbBuffer, ULONG ulSubFunction);
# define QHINF_EXEINFO       1 /* NE exeinfo. */
# define QHINF_READRSRCTBL   2 /* Reads from the resource table. */
# define QHINF_READFILE      3 /* Reads from the executable file. */
# define QHINF_LIBPATHLENGTH 4 /* Gets the libpath length. */
# define QHINF_LIBPATH       5 /* Gets the entire libpath. */
# define QHINF_FIXENTRY      6 /* NE only */
# define QHINF_STE           7 /* NE only */
# define QHINF_MAPSEL        8 /* NE only */

#elif K_OS == K_OS_WINDOWS
# undef IMAGE_NT_SIGNATURE
# undef IMAGE_DOS_SIGNATURE
# include <windows.h>
# ifndef IMAGE_SCN_TYPE_NOLOAD
#  define IMAGE_SCN_TYPE_NOLOAD 0x00000002
# endif

/*#elif defined(__NT__)
#include <winnt.h> */

#elif K_OS == K_OS_DARWIN
# include <dlfcn.h>
# include <errno.h>

#else
# error "port me"
#endif



/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** @def KLDRMODNATIVE_STRICT
 * Define KLDRMODNATIVE_STRICT to enabled strict checks in KLDRMODNATIVE. */
#define KLDRMODNATIVE_STRICT 1

/** @def KLDRMODNATIVE_ASSERT
 * Assert that an expression is true when KLDR_STRICT is defined.
 */
#ifdef KLDRMODNATIVE_STRICT
# define KLDRMODNATIVE_ASSERT(expr)  kHlpAssert(expr)
#else
# define KLDRMODNATIVE_ASSERT(expr)  do {} while (0)
#endif

#if K_OS == K_OS_WINDOWS
/** @def KLDRMODNATIVE_RVA2TYPE
 * Converts a RVA to a pointer of the specified type.
 * @param   pvBits      The bits (image base).
 * @param   uRVA        The image relative virtual address.
 * @param   type        The type to cast to.
 */
# define KLDRMODNATIVE_RVA2TYPE(pvBits, uRVA, type) \
        ( (type) ((KUPTR)(pvBits) + (uRVA)) )

#endif /* PE OSes */



/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Instance data for the module interpreter for the Native Loaders.
 */
typedef struct KLDRMODNATIVE
{
    /** Pointer to the module. (Follows the section table.) */
    PKLDRMOD                    pMod;
    /** Reserved flags. */
    KU32                        f32Reserved;
    /** The number of imported modules.
     * If ~(KU32)0 this hasn't been determined yet. */
    KU32                        cImportModules;
#if K_OS == K_OS_OS2
    /** The module handle. */
    HMODULE                     hmod;

#elif K_OS == K_OS_WINDOWS
    /** The module handle. */
    HANDLE                      hmod;
    /** Pointer to the NT headers. */
    const IMAGE_NT_HEADERS     *pNtHdrs;
    /** Pointer to the section header array. */
    const IMAGE_SECTION_HEADER *paShdrs;

#elif K_OS == K_OS_DARWIN
    /** The dlopen() handle.*/
    void                       *pvMod;

#else
# error "Port me"
#endif
} KLDRMODNATIVE, *PKLDRMODNATIVE;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static KI32 kldrModNativeNumberOfImports(PKLDRMOD pMod, const void *pvBits);

/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
extern KLDRMODOPS g_kLdrModNativeOps;



/**
 * Use native loader to load the file opened by pRdr.
 *
 * @returns 0 on success and *ppMod pointing to a module instance.
 *          On failure, a non-zero OS specific error code is returned.
 * @param   pOps            Pointer to the registered method table.
 * @param   pRdr            The file provider instance to use.
 * @param   offNewHdr       The offset of the new header in MZ files. -1 if not found.
 * @param   ppMod           Where to store the module instance pointer.
 */
static int kldrModNativeCreate(PCKLDRMODOPS pOps, PKRDR pRdr, KU32 fFlags, KCPUARCH enmCpuArch,
                               KLDRFOFF offNewHdr, PPKLDRMOD ppMod)
{
    int rc = kLdrModOpenNative(kRdrName(pRdr), ppMod);
    if (rc)
        return rc;
    rc = kRdrClose(pRdr);
    KLDRMODNATIVE_ASSERT(!rc);
    return 0;
}


/**
 * Loads a module using the native module loader.
 *
 * @returns 0 on success.
 * @returns non-zero native or kLdr status code on failure.
 * @param   pszFilename         The filename or module name to be loaded.
 * @param   ppMod               Where to store the module interpreter instance pointer.
 */
int kLdrModOpenNative(const char *pszFilename, PPKLDRMOD ppMod)
{
    int rc;

    /*
     * Load the image.
     */
#if K_OS == K_OS_OS2
    HMODULE hmod;

    rc = DosLoadModule(NULL, 0, (PCSZ)pszFilename, &hmod);
    if (rc)
        return rc;
    rc = kLdrModOpenNativeByHandle((KUPTR)hmod, ppMod);
    if (rc)
        DosFreeModule(hmod);

#elif K_OS == K_OS_WINDOWS
    HMODULE hmod;

    hmod = LoadLibrary(pszFilename);
    if (!hmod)
        return GetLastError();
    rc = kLdrModOpenNativeByHandle((KUPTR)hmod, ppMod);
    if (rc)
        FreeLibrary(hmod);

#elif K_OS == K_OS_DARWIN
    void *pvMod;

    pvMod = dlopen(pszFilename, 0);
    if (!pvMod)
        return ENOENT;
    rc = kLdrModOpenNativeByHandle((KUPTR)pvMod, ppMod);
    if (rc)
        dlclose(pvMod);

#else
# error "Port me"
#endif
    return rc;
}


/**
 * Creates a native module interpret for an already module already
 * loaded by the native loader.
 *
 * @returns 0 on success.
 * @returns non-zero native or kLdr status code on failure.
 * @param   pszFilename         The filename or module name to be loaded.
 * @param   ppMod               Where to store the module interpreter instance pointer.
 * @remark  This will not make the native loader increment the load count.
 */
int kLdrModOpenNativeByHandle(KUPTR uHandle, PPKLDRMOD ppMod)
{
    KSIZE cb;
    KU32 cchFilename;
    KU32 cSegments;
    PKLDRMOD pMod;
    PKLDRMODNATIVE pModNative;

    /*
     * Delcare variables, parse the module header or whatever and determin the
     * size of the module instance.
     */
#if K_OS == K_OS_OS2
    char szFilename[CCHMAXPATH];
    int rc;

    /* get the filename. */
    rc = DosQueryModuleName((HMODULE)uHandle, sizeof(szFilename), szFilename);
    if (rc)
    {
        KLDRMODNATIVE_ASSERT(rc);
        szFilename[0] = '\0';
    }

    /* get the segment count. */
    /** @todo DosQueryHeaderInfo should be able to get us what we want on OS/2. */
    cSegments = 1;

#elif K_OS == K_OS_WINDOWS
    DWORD                       dw;
    char                        szFilename[MAX_PATH];
    const IMAGE_NT_HEADERS     *pNtHdrs;
    const IMAGE_SECTION_HEADER *paShdrs;
    const IMAGE_DOS_HEADER     *pDosHdr = (const IMAGE_DOS_HEADER *)uHandle;
    unsigned                    i;

    /* get the filename. */
    dw = GetModuleFileName((HANDLE)uHandle, szFilename, sizeof(szFilename));
    if (dw <= 0)
    {
        KLDRMODNATIVE_ASSERT(dw <= 0);
        szFilename[0] = '\0';
    }

    /* get the segment count. */
    if (pDosHdr->e_magic == IMAGE_DOS_SIGNATURE)
        pNtHdrs = (const IMAGE_NT_HEADERS *)((KUPTR)pDosHdr + pDosHdr->e_lfanew);
    else
        pNtHdrs = (const IMAGE_NT_HEADERS *)pDosHdr;
    if (pNtHdrs->Signature != IMAGE_NT_SIGNATURE)
    {
        KLDRMODNATIVE_ASSERT(!"bad signature");
        return KLDR_ERR_UNKNOWN_FORMAT;
    }
    if (pNtHdrs->FileHeader.SizeOfOptionalHeader != sizeof(IMAGE_OPTIONAL_HEADER))
    {
        KLDRMODNATIVE_ASSERT(!"bad optional header size");
        return KLDR_ERR_UNKNOWN_FORMAT;
    }
    cSegments = pNtHdrs->FileHeader.NumberOfSections + 1;
    paShdrs = (const IMAGE_SECTION_HEADER *)(pNtHdrs + 1);

#elif K_OS == K_OS_DARWIN
    char    szFilename[1] = "";
    cSegments = 0; /** @todo Figure out the Mac OS X dynamic loader. */

#else
# error "Port me"
#endif

    /*
     * Calc the instance size, allocate and initialize it.
     */
    cchFilename = (KU32)kHlpStrLen(szFilename);
    cb = K_ALIGN_Z(sizeof(KLDRMODNATIVE), 16)
       + K_OFFSETOF(KLDRMOD, aSegments[cSegments])
       + cchFilename + 1;
    pModNative = (PKLDRMODNATIVE)kHlpAlloc(cb);
    if (!pModNative)
        return KERR_NO_MEMORY;

    /* KLDRMOD */
    pMod = (PKLDRMOD)((KU8 *)pModNative + K_ALIGN_Z(sizeof(KLDRMODNATIVE), 16));
    pMod->pvData = pModNative;
    pMod->pRdr = NULL;
    pMod->pOps = NULL;      /* set upon success. */
    pMod->cSegments = cSegments;
    pMod->cchFilename = cchFilename;
    pMod->pszFilename = (char *)&pMod->aSegments[pMod->cSegments];
    kHlpMemCopy((char *)pMod->pszFilename, szFilename, cchFilename + 1);
    pMod->pszName = kHlpGetFilename(pMod->pszFilename); /** @todo get soname */
    pMod->cchName = cchFilename - (KU32)(pMod->pszName - pMod->pszFilename);
    pMod->fFlags = 0;
#if defined(__i386__) || defined(__X86__) || defined(_M_IX86)
    pMod->enmCpu = KCPU_I386;
    pMod->enmArch = KCPUARCH_X86_32;
    pMod->enmEndian = KLDRENDIAN_LITTLE;
#elif defined(__X86_64__) || defined(__x86_64__) || defined(__AMD64__) || defined(_M_IX64)
    pMod->enmCpu = KCPU_K8;
    pMod->enmArch = KCPUARCH_AMD64;
    pMod->enmEndian = KLDRENDIAN_LITTLE;
#else
# error "Port me"
#endif
    pMod->enmFmt = KLDRFMT_NATIVE;
    pMod->enmType = KLDRTYPE_SHARED_LIBRARY_RELOCATABLE;
    pMod->u32Magic = 0;     /* set upon success. */

    /* KLDRMODNATIVE */
    pModNative->pMod = pMod;
    pModNative->f32Reserved = 0;
    pModNative->cImportModules = ~(KU32)0;

    /*
     * Set native instance data.
     */
#if K_OS == K_OS_OS2
    pModNative->hmod = (HMODULE)uHandle;

    /* just fake a segment for now. */
    pMod->aSegments[0].pvUser = NULL;
    pMod->aSegments[0].pchName = "fake";
    pMod->aSegments[0].cchName = sizeof("fake") - 1;
    pMod->aSegments[0].enmProt = KPROT_NOACCESS;
    pMod->aSegments[0].cb = 0;
    pMod->aSegments[0].Alignment = 0;
    pMod->aSegments[0].LinkAddress = NIL_KLDRADDR;
    pMod->aSegments[0].offFile = -1;
    pMod->aSegments[0].cbFile = 0;
    pMod->aSegments[0].RVA = NIL_KLDRADDR;
    pMod->aSegments[0].cbMapped = 0;
    pMod->aSegments[0].MapAddress = 0;

#elif K_OS == K_OS_WINDOWS
    pModNative->hmod = (HMODULE)uHandle;
    pModNative->pNtHdrs = pNtHdrs;
    pModNative->paShdrs = paShdrs;

    if (pNtHdrs->FileHeader.Characteristics & IMAGE_FILE_DLL)
        pMod->enmType = !(pNtHdrs->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
            ? KLDRTYPE_SHARED_LIBRARY_RELOCATABLE
            : KLDRTYPE_SHARED_LIBRARY_FIXED;
    else
        pMod->enmType = !(pNtHdrs->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
            ? KLDRTYPE_EXECUTABLE_RELOCATABLE
            : KLDRTYPE_EXECUTABLE_FIXED;

    /* The implied headers section. */
    pMod->aSegments[0].pvUser = NULL;
    pMod->aSegments[0].pchName = "TheHeaders";
    pMod->aSegments[0].cchName = sizeof("TheHeaders") - 1;
    pMod->aSegments[0].enmProt = KPROT_READONLY;
    pMod->aSegments[0].cb = pNtHdrs->OptionalHeader.SizeOfHeaders;
    pMod->aSegments[0].Alignment = pNtHdrs->OptionalHeader.SectionAlignment;
    pMod->aSegments[0].LinkAddress = pNtHdrs->OptionalHeader.ImageBase;
    pMod->aSegments[0].offFile = 0;
    pMod->aSegments[0].cbFile = pNtHdrs->OptionalHeader.SizeOfHeaders;
    pMod->aSegments[0].RVA = 0;
    if (pMod->cSegments > 1)
        pMod->aSegments[0].cbMapped = paShdrs[0].VirtualAddress;
    else
        pMod->aSegments[0].cbMapped = pNtHdrs->OptionalHeader.SizeOfHeaders;
    pMod->aSegments[0].MapAddress = uHandle;

    /* The section headers. */
    for (i = 0; i < pNtHdrs->FileHeader.NumberOfSections; i++)
    {
        const char *pch;
        KU32        cchSegName;

        /* unused */
        pMod->aSegments[i + 1].pvUser = NULL;

        /* name */
        pMod->aSegments[i + 1].pchName = pch = &paShdrs[i].Name[0];
        cchSegName = IMAGE_SIZEOF_SHORT_NAME;
        while (    cchSegName > 0
               && (pch[cchSegName - 1] == ' ' || pch[cchSegName - 1] == '\0'))
            cchSegName--;
        pMod->aSegments[i + 1].cchName = cchSegName;

        /* size and addresses */
        if (!(paShdrs[i].Characteristics & IMAGE_SCN_TYPE_NOLOAD))
        {
            pMod->aSegments[i + 1].cb          = paShdrs[i].Misc.VirtualSize;
            pMod->aSegments[i + 1].RVA         = paShdrs[i].VirtualAddress;
            pMod->aSegments[i + 1].LinkAddress = paShdrs[i].VirtualAddress + pNtHdrs->OptionalHeader.ImageBase;
            pMod->aSegments[i + 1].MapAddress  = paShdrs[i].VirtualAddress + uHandle;
            pMod->aSegments[i + 1].cbMapped    = paShdrs[i].Misc.VirtualSize;
            if (i + 2 < pMod->cSegments)
                pMod->aSegments[i + 1].cbMapped = paShdrs[i + 1].VirtualAddress
                                                - paShdrs[i].VirtualAddress;
        }
        else
        {
            pMod->aSegments[i + 1].cb          = 0;
            pMod->aSegments[i + 1].cbMapped    = 0;
            pMod->aSegments[i + 1].LinkAddress = NIL_KLDRADDR;
            pMod->aSegments[i + 1].RVA         = 0;
            pMod->aSegments[i + 1].MapAddress  = 0;
        }

        /* file location */
        pMod->aSegments[i + 1].offFile = paShdrs[i].PointerToRawData;
        pMod->aSegments[i + 1].cbFile  = paShdrs[i].SizeOfRawData;
        if (    pMod->aSegments[i + 1].cbMapped > 0 /* if mapped */
            &&  (KLDRSIZE)pMod->aSegments[i + 1].cbFile > pMod->aSegments[i + 1].cbMapped)
            pMod->aSegments[i + 1].cbFile = (KLDRFOFF)pMod->aSegments[i + 1].cbMapped;

        /* protection */
        switch (  paShdrs[i].Characteristics
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
        switch (paShdrs[i].Characteristics & IMAGE_SCN_ALIGN_MASK)
        {
            case 0: /* hope this is right... */
                pMod->aSegments[i + 1].Alignment = pNtHdrs->OptionalHeader.SectionAlignment;
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

#elif K_OS == K_OS_DARWIN
    /** @todo Figure out the Mac OS X dynamic loader. */

#else
# error "Port me"
#endif

    /*
     * We're done.
     */
    pMod->u32Magic = KLDRMOD_MAGIC;
    pMod->pOps = &g_kLdrModNativeOps;
    *ppMod = pMod;
    return 0;
}


/** @copydoc KLDRMODOPS::pfnDestroy */
static int kldrModNativeDestroy(PKLDRMOD pMod)
{
    PKLDRMODNATIVE pModNative = (PKLDRMODNATIVE)pMod->pvData;
    int rc;

#if K_OS == K_OS_OS2
    rc = DosFreeModule(pModNative->hmod);

#elif K_OS == K_OS_WINDOWS
    if (FreeLibrary(pModNative->hmod))
        rc = 0;
    else
        rc = GetLastError();

#elif K_OS == K_OS_DARWIN
    dlclose(pModNative->pvMod);

#else
# error "Port me"
#endif

    pMod->u32Magic = 0;
    pMod->pOps = NULL;
    kHlpFree(pModNative);
    return rc;
}


/** @copydoc kLdrModQuerySymbol */
static int kldrModNativeQuerySymbol(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, KU32 iSymbol,
                                    const char *pchSymbol, KSIZE cchSymbol, const char *pszVersion,
                                    PFNKLDRMODGETIMPORT pfnGetForwarder, void *pvUser, PKLDRADDR puValue, KU32 *pfKind)
{
    PKLDRMODNATIVE pModNative = (PKLDRMODNATIVE)pMod->pvData;
    const char *pszSymbol = pchSymbol;
#if K_OS == K_OS_OS2
    APIRET rc;
    PFN pfn;
#elif K_OS == K_OS_WINDOWS
    FARPROC pfn;
#elif K_OS == K_OS_DARWIN
    void *pfn;
#else
# error "Port me"
#endif

    /* make stack copy of the symbol if it isn't zero terminated. */
    if (pszSymbol && pszSymbol[cchSymbol])
    {
        char *pszCopy = kHlpAllocA(cchSymbol + 1);
        kHlpMemCopy(pszCopy, pchSymbol, cchSymbol);
        pszCopy[cchSymbol] = '\0';
        pszSymbol = pszCopy;
    }

#if K_OS == K_OS_OS2
    if (!pchSymbol && iSymbol >= 0x10000)
        return KLDR_ERR_SYMBOL_NOT_FOUND;

    if (puValue)
    {
        rc = DosQueryProcAddr(pModNative->hmod,
                              pszSymbol ? 0 : iSymbol,
                              (PCSZ)pszSymbol,
                              &pfn);
        if (rc)
            return rc == ERROR_PROC_NOT_FOUND ? KLDR_ERR_SYMBOL_NOT_FOUND : rc;
        *puValue = (KUPTR)pfn;
    }
    if (pfKind)
    {
        ULONG ulProcType;
        rc = DosQueryProcType(pModNative->hmod,
                              pszSymbol ? 0 : iSymbol,
                              (PCSZ)pszSymbol,
                              &ulProcType);
        if (rc)
        {
            if (puValue)
                *puValue = 0;
            return rc == ERROR_PROC_NOT_FOUND ? KLDR_ERR_SYMBOL_NOT_FOUND : rc;
        }
        *pfKind = (ulProcType & PT_32BIT ? KLDRSYMKIND_32BIT : KLDRSYMKIND_16BIT)
                | KLDRSYMKIND_NO_TYPE;
    }

#elif K_OS == K_OS_WINDOWS
    if (!pszSymbol && iSymbol >= 0x10000)
        return KLDR_ERR_SYMBOL_NOT_FOUND;

    pfn = GetProcAddress(pModNative->hmod, pszSymbol ? pszSymbol : (const char *)(KUPTR)iSymbol);
    if (puValue)
        *puValue = (KUPTR)pfn;
    if (pfKind)
        *pfKind = (pModNative->pNtHdrs->FileHeader.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER32)
                   ? KLDRSYMKIND_32BIT : KLDRSYMKIND_16BIT)
                | KLDRSYMKIND_NO_TYPE;

#elif K_OS == K_OS_DARWIN
    if (!pszSymbol && iSymbol != NIL_KLDRMOD_SYM_ORDINAL)
        return KLDR_ERR_SYMBOL_NOT_FOUND;

    pfn = dlsym(pModNative->pvMod, pszSymbol);
    if (!pfn)
        return KLDR_ERR_SYMBOL_NOT_FOUND;
    if (puValue)
        *puValue = (KUPTR)pfn;
    if (pfKind)
        *pfKind = (sizeof(KUPTR) == 4 ? KLDRSYMKIND_32BIT : KLDRSYMKIND_64BIT)
                | KLDRSYMKIND_NO_TYPE;

#else
# error "Port me"
#endif

    return 0;
}


/** @copydoc kLdrModEnumSymbols */
static int kldrModNativeEnumSymbols(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress,
                                    KU32 fFlags, PFNKLDRMODENUMSYMS pfnCallback, void *pvUser)
{
    PKLDRMODNATIVE                  pModNative = (PKLDRMODNATIVE)pMod->pvData;
#if K_OS == K_OS_OS2

    /** @todo implement export enumeration on OS/2. */
    (void)pModNative;
    return ERROR_NOT_SUPPORTED;

#elif K_OS == K_OS_WINDOWS || defined(__NT__)
    const KU32                     *paFunctions;
    const IMAGE_EXPORT_DIRECTORY   *pExpDir;
    const KU32                     *paRVANames;
    const KU16                     *paOrdinals;
    KU32                            iFunction;
    KU32                            cFunctions;
    KU32                            cNames;
    int                             rc;

    /*
     * Make sure we've got mapped bits and resolve any base address aliases.
     */
    if (    pModNative->pNtHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size
        <   sizeof(IMAGE_EXPORT_DIRECTORY))
        return 0; /* no exports to enumerate, return success. */

    pExpDir = KLDRMODNATIVE_RVA2TYPE(pModNative->hmod,
                                     pModNative->pNtHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,
                                     PIMAGE_EXPORT_DIRECTORY);

    /*
     * Enumerate the ordinal exports.
     */
    paRVANames = KLDRMODNATIVE_RVA2TYPE(pModNative->hmod, pExpDir->AddressOfNames, const KU32 *);
    paOrdinals = KLDRMODNATIVE_RVA2TYPE(pModNative->hmod, pExpDir->AddressOfNameOrdinals, const KU16 *);
    paFunctions = KLDRMODNATIVE_RVA2TYPE(pModNative->hmod, pExpDir->AddressOfFunctions, const KU32 *);
    cFunctions = pExpDir->NumberOfFunctions;
    cNames = pExpDir->NumberOfNames;
    for (iFunction = 0; iFunction < cFunctions; iFunction++)
    {
        unsigned        fFoundName;
        KU32            iName;
        const KU32      uRVA = paFunctions[iFunction];
        const KLDRADDR  uValue = BaseAddress + uRVA;
        KU32            fKind = (pModNative->pNtHdrs->FileHeader.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER32)
                              ? KLDRSYMKIND_32BIT : KLDRSYMKIND_64BIT)
                              | KLDRSYMKIND_NO_TYPE;
        if (    uRVA - pModNative->pNtHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress
            <   pModNative->pNtHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size)
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

            pszName = KLDRMODNATIVE_RVA2TYPE(pModNative->hmod, paRVANames[iName], const char *);
            rc = pfnCallback(pMod, iFunction + pExpDir->Base, pszName, strlen(pszName), NULL,
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

#elif K_OS == K_OS_DARWIN
    /** @todo implement enumeration on darwin. */
    (void)pModNative;
    return KLDR_ERR_TODO;

#else
# error "Port me"
#endif

}


/** @copydoc kLdrModGetImport */
static int kldrModNativeGetImport(PKLDRMOD pMod, const void *pvBits, KU32 iImport, char *pszName, KSIZE cchName)
{
    PKLDRMODNATIVE                  pModNative = (PKLDRMODNATIVE)pMod->pvData;
#if K_OS == K_OS_OS2

    /** @todo implement import enumeration on OS/2. */
    (void)pModNative;
    return ERROR_NOT_SUPPORTED;

#elif K_OS == K_OS_WINDOWS || defined(__NT__)
    const IMAGE_IMPORT_DESCRIPTOR  *pImpDesc;
    const char                     *pszImportName;
    KSIZE                           cchImportName;
    int                             rc;

    /*
     * Simple bounds check.
     */
    if (iImport >= (KU32)kldrModNativeNumberOfImports(pMod, pvBits))
        return KLDR_ERR_IMPORT_ORDINAL_OUT_OF_BOUNDS;

    /*
     * Get the name.
     */
    pImpDesc = KLDRMODNATIVE_RVA2TYPE(pModNative->hmod,
                                      pModNative->pNtHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
                                      + sizeof(IMAGE_IMPORT_DESCRIPTOR) * iImport,
                                      const IMAGE_IMPORT_DESCRIPTOR *);
    pszImportName = KLDRMODNATIVE_RVA2TYPE(pModNative->hmod, pImpDesc->Name, const char *);
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

#elif K_OS == K_OS_DARWIN
    /** @todo Implement import enumeration on darwin. */
    (void)pModNative;
    return KLDR_ERR_TODO;

#else
# error "Port me"
#endif
}


/** @copydoc kLdrModNumberOfImports */
static KI32 kldrModNativeNumberOfImports(PKLDRMOD pMod, const void *pvBits)
{
    PKLDRMODNATIVE pModNative = (PKLDRMODNATIVE)pMod->pvData;
#if K_OS == K_OS_OS2

    /** @todo implement import counting on OS/2. */
    (void)pModNative;
    return -1;

#elif K_OS == K_OS_WINDOWS || defined(__NT__)
    if (pModNative->cImportModules == ~(KU32)0)
    {
        /*
         * We'll have to walk the import descriptors to figure out their number.
         */
        pModNative->cImportModules = 0;
        if (    pModNative->pNtHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size
            &&  pModNative->pNtHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)
        {
            const IMAGE_IMPORT_DESCRIPTOR  *pImpDesc;

            pImpDesc = KLDRMODNATIVE_RVA2TYPE(pModNative->hmod,
                                              pModNative->pNtHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress,
                                              const IMAGE_IMPORT_DESCRIPTOR *);
            while (pImpDesc->Name && pImpDesc->FirstThunk)
            {
                pModNative->cImportModules++;
                pImpDesc++;
            }
        }
    }
    return pModNative->cImportModules;

#elif K_OS == K_OS_DARWIN
    /** @todo Implement import counting on Darwin. */
    (void)pModNative;
    return -1;

#else
# error "Port me"
#endif
}


/** @copydoc kLdrModGetStackInfo */
static int kldrModNativeGetStackInfo(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, PKLDRSTACKINFO pStackInfo)
{
    PKLDRMODNATIVE pModNative = (PKLDRMODNATIVE)pMod->pvData;
#if K_OS == K_OS_OS2

    /** @todo implement stack info on OS/2. */
    (void)pModNative;
    return ERROR_NOT_SUPPORTED;

#elif K_OS == K_OS_WINDOWS || defined(__NT__)
    pStackInfo->Address = NIL_KLDRADDR;
    pStackInfo->LinkAddress = NIL_KLDRADDR;
    pStackInfo->cbStack = pStackInfo->cbStackThread = pModNative->pNtHdrs->OptionalHeader.SizeOfStackReserve;

    return 0;

#elif K_OS == K_OS_DARWIN
    /** @todo Implement stack info on Darwin. */
    (void)pModNative;
    return KLDR_ERR_TODO;

#else
# error "Port me"
#endif
}


/** @copydoc kLdrModQueryMainEntrypoint */
static int kldrModNativeQueryMainEntrypoint(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, PKLDRADDR pMainEPAddress)
{
    PKLDRMODNATIVE pModNative = (PKLDRMODNATIVE)pMod->pvData;
#if K_OS == K_OS_OS2

    /** @todo implement me on OS/2. */
    (void)pModNative;
    return ERROR_NOT_SUPPORTED;

#elif K_OS == K_OS_WINDOWS || defined(__NT__)
    /*
     * Convert the address from the header.
     */
    *pMainEPAddress = pModNative->pNtHdrs->OptionalHeader.AddressOfEntryPoint
        ? BaseAddress + pModNative->pNtHdrs->OptionalHeader.AddressOfEntryPoint
        : NIL_KLDRADDR;
    return 0;

#elif K_OS == K_OS_DARWIN
    /** @todo Implement me on Darwin. */
    (void)pModNative;
    return KLDR_ERR_TODO;

#else
# error "Port me"
#endif
}


/** @copydoc kLdrModEnumDbgInfo */
static int kldrModNativeEnumDbgInfo(PKLDRMOD pMod, const void *pvBits, PFNKLDRENUMDBG pfnCallback, void *pvUser)
{
    PKLDRMODNATIVE                  pModNative = (PKLDRMODNATIVE)pMod->pvData;
#if K_OS == K_OS_OS2

    /** @todo implement me on OS/2. */
    (void)pModNative;
    return ERROR_NOT_SUPPORTED;

#elif K_OS == K_OS_WINDOWS || defined(__NT__)
    const IMAGE_DEBUG_DIRECTORY    *pDbgDir;
    KU32                            iDbgInfo;
    KU32                            cb;
    int                             rc;

    /*
     * Check that there is a debug directory first.
     */
    cb = pModNative->pNtHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
    if (    cb < sizeof(IMAGE_DEBUG_DIRECTORY) /* screw borland linkers */
        ||  !pModNative->pNtHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress)
        return 0;

    /*
     * Enumerate the debug directory.
     */
    pDbgDir = KLDRMODNATIVE_RVA2TYPE(pModNative->hmod,
                                     pModNative->pNtHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress,
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
                         enmDbgInfoType, pDbgDir->MajorVersion, pDbgDir->MinorVersion, NULL /*pszPartNm*/,
                         pDbgDir->PointerToRawData ? pDbgDir->PointerToRawData : -1,
                         pDbgDir->AddressOfRawData ? pDbgDir->AddressOfRawData : NIL_KLDRADDR,
                         pDbgDir->SizeOfData,
                         NULL /*pszExtFile*/, pvUser);
        if (rc)
            break;

        /* next */
        if (cb <= sizeof(IMAGE_DEBUG_DIRECTORY))
            break;
    }

    return rc;

#elif K_OS == K_OS_DARWIN
    /** @todo Implement me on Darwin. */
    (void)pModNative;
    return KLDR_ERR_TODO;

#else
# error "Port me"
#endif
}


/** @copydoc kLdrModHasDbgInfo */
static int kldrModNativeHasDbgInfo(PKLDRMOD pMod, const void *pvBits)
{
    PKLDRMODNATIVE pModNative = (PKLDRMODNATIVE)pMod->pvData;
#if K_OS == K_OS_OS2

    /** @todo implement me on OS/2. */
    (void)pModNative;
    return KLDR_ERR_NO_DEBUG_INFO;

#elif K_OS == K_OS_WINDOWS || defined(__NT__)
    /*
     * Base this entirely on the presence of a debug directory.
     */
    if (    pModNative->pNtHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size
            < sizeof(IMAGE_DEBUG_DIRECTORY) /* screw borland linkers */
        ||  !pModNative->pNtHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress)
        return KLDR_ERR_NO_DEBUG_INFO;
    return 0;

#elif K_OS == K_OS_DARWIN
    /** @todo Implement me on Darwin. */
    (void)pModNative;
    return KLDR_ERR_NO_DEBUG_INFO;

#else
# error "Port me"
#endif
}


/** @copydoc kLdrModMap */
static int kldrModNativeMap(PKLDRMOD pMod)
{
    return 0;
}


/** @copydoc kLdrModUnmap */
static int kldrModNativeUnmap(PKLDRMOD pMod)
{
    return 0;
}


/** @copydoc kLdrModAllocTLS */
static int kldrModNativeAllocTLS(PKLDRMOD pMod, void *pvMapping)
{
    return 0;
}


/** @copydoc kLdrModFreeTLS */
static void kldrModNativeFreeTLS(PKLDRMOD pMod, void *pvMapping)
{
}


/** @copydoc kLdrModReload */
static int kldrModNativeReload(PKLDRMOD pMod)
{
    return 0;
}


/** @copydoc kLdrModFixupMapping */
static int kldrModNativeFixupMapping(PKLDRMOD pMod, PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser)
{
    return 0;
}


/** @copydoc kLdrModCallInit */
static int kldrModNativeCallInit(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle)
{
    return 0;
}


/** @copydoc kLdrModCallTerm */
static int kldrModNativeCallTerm(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle)
{
    return 0;
}


/** @copydoc kLdrModCallThread */
static int kldrModNativeCallThread(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle, unsigned fAttachingOrDetaching)
{
    return 0;
}


/** @copydoc kLdrModSize */
static KLDRADDR kldrModNativeSize(PKLDRMOD pMod)
{
#if K_OS == K_OS_OS2
    return 0; /* don't bother */

#elif K_OS == K_OS_WINDOWS || defined(__NT__)
    /* just because we can. */
    PKLDRMODNATIVE pModNative = (PKLDRMODNATIVE)pMod->pvData;
    return pModNative->pNtHdrs->OptionalHeader.SizeOfImage;

#elif K_OS == K_OS_DARWIN
    /** @todo Implement me on Darwin. */
    return 0;

#else
# error "Port me"
#endif
}


/** @copydoc kLdrModGetBits */
static int kldrModNativeGetBits(PKLDRMOD pMod, void *pvBits, KLDRADDR BaseAddress, PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser)
{
#if K_OS == K_OS_OS2
    return ERROR_NOT_SUPPORTED; /* don't bother */

#elif K_OS == K_OS_WINDOWS || defined(__NT__)
    return ERROR_NOT_SUPPORTED; /* don't bother even if we could implement this. */

#elif K_OS == K_OS_DARWIN
    return KLDR_ERR_TODO; /* don't bother. */

#else
# error "Port me"
#endif
}


/** @copydoc kLdrModRelocateBits */
static int kldrModNativeRelocateBits(PKLDRMOD pMod, void *pvBits, KLDRADDR NewBaseAddress, KLDRADDR OldBaseAddress,
                                     PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser)
{
#if K_OS == K_OS_OS2
    return ERROR_NOT_SUPPORTED; /* don't bother */

#elif K_OS == K_OS_WINDOWS || defined(__NT__)
    return ERROR_NOT_SUPPORTED; /* don't bother even if we could implement this. */

#elif K_OS == K_OS_DARWIN
    return KLDR_ERR_TODO; /* don't bother. */

#else
# error "Port me"
#endif
}


/**
 * The native module interpreter method table.
 */
KLDRMODOPS g_kLdrModNativeOps =
{
    "Native",
    NULL,
    kldrModNativeCreate,
    kldrModNativeDestroy,
    kldrModNativeQuerySymbol,
    kldrModNativeEnumSymbols,
    kldrModNativeGetImport,
    kldrModNativeNumberOfImports,
    NULL /* can execute one is optional */,
    kldrModNativeGetStackInfo,
    kldrModNativeQueryMainEntrypoint,
    NULL /* pfnQueryImageUuid */,
    NULL /* fixme */,
    NULL /* fixme */,
    kldrModNativeEnumDbgInfo,
    kldrModNativeHasDbgInfo,
    kldrModNativeMap,
    kldrModNativeUnmap,
    kldrModNativeAllocTLS,
    kldrModNativeFreeTLS,
    kldrModNativeReload,
    kldrModNativeFixupMapping,
    kldrModNativeCallInit,
    kldrModNativeCallTerm,
    kldrModNativeCallThread,
    kldrModNativeSize,
    kldrModNativeGetBits,
    kldrModNativeRelocateBits,
    NULL /* fixme */,
    42 /* the end */
};

