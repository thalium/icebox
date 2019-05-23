/* $Id: kDbgModWinDbgHelp.cpp 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kDbg - The Debug Info Reader, DbgHelp Based Reader.
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
#include <Windows.h>
#define _IMAGEHLP64
#include <DbgHelp.h>

#include "kDbgInternal.h"
#include <k/kHlpAlloc.h>
#include <k/kHlpString.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** The dbghelp.dll module handle. */
static HMODULE g_hDbgHelp = NULL;
/** Pointer to the dbhelp.dll SymInitialize function. */
static BOOL (WINAPI *g_pfnSymInitialize)(IN HANDLE,IN LPSTR,IN BOOL);
/** Pointer to the dbhelp.dll SymCleanup function. */
static BOOL (WINAPI *g_pfnSymCleanup)(IN HANDLE);
/** Pointer to the dbhelp.dll SymSetOptions function. */
static DWORD (WINAPI *g_pfnSymSetOptions)(IN DWORD);
/** Pointer to the dbhelp.dll SymLoadModule64 function. */
static DWORD64 (WINAPI *g_pfnSymLoadModule64)(IN HANDLE, IN HANDLE, IN PCSTR, IN PCSTR ModuleName, IN DWORD64, IN DWORD);
/** Pointer to the dbhelp.dll SymFromAddr function. */
static DWORD (WINAPI *g_pfnSymFromAddr)(IN HANDLE, IN DWORD64, OUT PDWORD64, OUT PSYMBOL_INFO);
/** Pointer to the dbhelp.dll SymGetLineFromAddr64 function. */
static DWORD (WINAPI *g_pfnSymGetLineFromAddr64)(IN HANDLE, IN DWORD64, OUT PDWORD64, OUT PIMAGEHLP_LINE64);



/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * A dbghelp based PE debug reader.
 */
typedef struct KDBGMODDBGHELP
{
    /** The common module core. */
    KDBGMOD     Core;
    /** The image base. */
    DWORD64     ImageBase;
    /** The "process" handle we present dbghelp. */
    HANDLE      hSymInst;
    /** The image size. */
    KU32        cbImage;
    /** The number of sections. (We've added the implicit header section.) */
    KI32        cSections;
    /** The section headers (variable size). The first section is the
     * implicit header section.*/
    IMAGE_SECTION_HEADER    aSections[1];
} KDBGMODDBGHELP, *PKDBGMODDBGHELP;


/**
 * Convers a Windows error to kDbg error code.
 *
 * @returns kDbg status code.
 * @param   rc          The Windows error.
 */
static int kdbgModDHConvWinError(DWORD rc)
{
    switch (rc)
    {
        case 0:                     return 0;
        default:                    return KERR_GENERAL_FAILURE;
    }
}


/**
 * Calcs the RVA for a segment:offset address.
 *
 * @returns IPRT status code.
 *
 * @param   pModDH      The PE debug module instance.
 * @param   iSegment    The segment number. Special segments are dealt with as well.
 * @param   off         The segment offset.
 * @param   puRVA       Where to store the RVA on success.
 */
static int kdbgModDHSegOffToRVA(PKDBGMODDBGHELP pModDH, KI32 iSegment, KDBGADDR off, KU32 *puRVA)
{
    if (iSegment >= 0)
    {
        kDbgAssertMsgReturn(iSegment < pModDH->cSections, ("iSegment=%x cSections=%x\n", iSegment, pModDH->cSections),
                            KDBG_ERR_INVALID_ADDRESS);
        kDbgAssertMsgReturn(off < pModDH->aSections[iSegment].Misc.VirtualSize,
                            ("off=" PRI_KDBGADDR " VirtualSize=%x\n", off, pModDH->aSections[iSegment].Misc.VirtualSize),
                            KDBG_ERR_INVALID_ADDRESS);
        *puRVA = pModDH->aSections[iSegment].VirtualAddress + (KU32)off;
        return 0;
    }

    if (iSegment == KDBGSEG_RVA)
    {
        kDbgAssertMsgReturn(off < pModDH->cbImage, ("off=" PRI_KDBGADDR ", cbImage=%x\n", off, pModDH->cbImage),
                            KDBG_ERR_INVALID_ADDRESS);
        *puRVA = (KU32)off;
        return 0;
    }
    kDbgAssertMsgFailedReturn(("iSegment=%d\n", iSegment), KDBG_ERR_INVALID_ADDRESS);
}


/**
 * Calcs the segment:offset address for a RVA.
 *
 * @returns IPRT status code.
 *
 * @param   pModDH      The PE debug module instance.
 * @param   uRVA        The RVA.
 * @param   piSegment   Where to store the segment number.
 * @param   poff        Where to store the segment offset.
 */
static int kdbgModDHRVAToSegOff(PKDBGMODDBGHELP pModDH, KU32 uRVA, KI32 *piSegment, KDBGADDR *poff)
{
    kDbgAssertMsgReturn(uRVA < pModDH->cbImage, ("uRVA=%x, cbImage=%x\n", uRVA, pModDH->cbImage),
                        KDBG_ERR_INVALID_ADDRESS);
    for (KI32 iSegment = 0; iSegment < pModDH->cSections; iSegment++)
    {
        /** @todo should probably be less strict about address in the alignment gaps. */
        KU32 off = uRVA - pModDH->aSections[iSegment].VirtualAddress;
        if (off < pModDH->aSections[iSegment].Misc.VirtualSize)
        {
            *poff = off;
            *piSegment = iSegment;
            return 0;
        }
    }
    kDbgAssertMsgFailedReturn(("uRVA=%x\n", uRVA), KDBG_ERR_INVALID_ADDRESS);
}


/**
 * @copydoc KDBGMODOPS::pfnQueryLine
 */
static int kdbgModDHQueryLine(PKDBGMOD pMod, KI32 iSegment, KDBGADDR off, PKDBGLINE pLine)
{
    PKDBGMODDBGHELP pModDH = (PKDBGMODDBGHELP)pMod;

    /*
     * Translate the address to an RVA.
     */
    KU32 uRVA;
    int rc = kdbgModDHSegOffToRVA(pModDH, iSegment, off, &uRVA);
    if (!rc)
    {
        DWORD64 off;
        IMAGEHLP_LINE64 Line;
        Line.SizeOfStruct = sizeof(Line);
        if (g_pfnSymGetLineFromAddr64(pModDH->hSymInst, pModDH->ImageBase + uRVA, &off, &Line))
        {
            pLine->RVA = (KDBGADDR)(Line.Address - pModDH->ImageBase);
            rc = kdbgModDHRVAToSegOff(pModDH, (KU32)pLine->RVA, &pLine->iSegment, &pLine->offSegment);
            pLine->iLine = Line.LineNumber;
            KSIZE cchFile = kHlpStrLen(Line.FileName);
            pLine->cchFile = cchFile < sizeof(pLine->szFile)
                           ? (KU16)cchFile
                           : (KU16)sizeof(pLine->szFile) - 1;
            kHlpMemCopy(pLine->szFile, Line.FileName, pLine->cchFile);
            pLine->szFile[pLine->cchFile] = '\0';
        }
        else
        {
            DWORD Err = GetLastError();
            rc = kdbgModDHConvWinError(Err);
        }
    }
    return rc;
}


/**
 * @copydoc KDBGMODOPS::pfnQuerySymbol
 */
static int kdbgModDHQuerySymbol(PKDBGMOD pMod, KI32 iSegment, KDBGADDR off, PKDBGSYMBOL pSym)
{
    PKDBGMODDBGHELP pModDH = (PKDBGMODDBGHELP)pMod;

    /*
     * Translate the address to an RVA.
     */
    KU32 uRVA;
    int rc = kdbgModDHSegOffToRVA(pModDH, iSegment, off, &uRVA);
    if (!rc)
    {
        DWORD64 off;
        union
        {
            SYMBOL_INFO Sym;
            char        achBuffer[sizeof(SYMBOL_INFO) + KDBG_SYMBOL_MAX];
        } Buf;
        Buf.Sym.SizeOfStruct = sizeof(SYMBOL_INFO);
        Buf.Sym.MaxNameLen = KDBG_SYMBOL_MAX;
        if (g_pfnSymFromAddr(pModDH->hSymInst, pModDH->ImageBase + uRVA, &off, &Buf.Sym))
        {
            pSym->cb      = Buf.Sym.Size;
            pSym->Address = NIL_KDBGADDR;
            pSym->fFlags  = 0;
            if (Buf.Sym.Flags & SYMFLAG_FUNCTION)
                pSym->fFlags |= KDBGSYM_FLAGS_CODE;
            else if (Buf.Sym.Flags & SYMFLAG_CONSTANT)
                pSym->fFlags |= KDBGSYM_FLAGS_ABS; /** @todo SYMFLAG_CONSTANT must be tested - documentation is too brief to say what is really meant here.*/
            else
                pSym->fFlags |= KDBGSYM_FLAGS_DATA;
            if (Buf.Sym.Flags & SYMFLAG_EXPORT)
                pSym->fFlags |= KDBGSYM_FLAGS_EXPORTED;
            if ((Buf.Sym.Flags & (SYMFLAG_VALUEPRESENT | SYMFLAG_CONSTANT)) == (SYMFLAG_VALUEPRESENT | SYMFLAG_CONSTANT))
            {
                pSym->iSegment   = KDBGSEG_ABS;
                pSym->offSegment = (KDBGADDR)Buf.Sym.Value;
                pSym->RVA        = (KDBGADDR)Buf.Sym.Value;
            }
            else
            {
                pSym->RVA        = (KDBGADDR)(Buf.Sym.Address - pModDH->ImageBase);
                rc = kdbgModDHRVAToSegOff(pModDH, (KU32)pSym->RVA, &pSym->iSegment, &pSym->offSegment);
            }
            pSym->cchName = (KU16)Buf.Sym.NameLen;
            if (pSym->cchName >= sizeof(pSym->szName))
                pSym->cchName = sizeof(pSym->szName) - 1;
            kHlpMemCopy(pSym->szName, Buf.Sym.Name, Buf.Sym.NameLen);
            pSym->szName[Buf.Sym.NameLen] = '\0';
        }
        else
        {
            DWORD Err = GetLastError();
            rc = kdbgModDHConvWinError(Err);
        }
    }
    return rc;
}


/**
 * @copydoc KDBGMODOPS::pfnClose
 */
static int kdbgModDHClose(PKDBGMOD pMod)
{
    PKDBGMODDBGHELP pModDH = (PKDBGMODDBGHELP)pMod;

    if (g_pfnSymCleanup(pModDH->hSymInst))
        return 0;

    DWORD Err = GetLastError();
    int rc = kdbgModDHConvWinError(Err);
    kDbgAssertMsgFailed(("SymInitialize failed: Err=%d rc=%d\n", Err, rc));
    return rc;
}


/**
 * Checks if the specified dbghelp.dll is usable.
 *
 * @returns IPRT status code.
 *
 * @param   pszPath     the path to the dbghelp.dll.
 */
static int kdbgModDHTryDbgHelp(const char *pszPath, KU32 *pu32FileVersionMS, KU32 *pu32FileVersionLS)
{
    int rc;
    DWORD dwHandle = 0;
    DWORD cb = GetFileVersionInfoSize(pszPath, &dwHandle);
    if (cb > 0)
    {
        void *pvBuf = alloca(cb);
        if (GetFileVersionInfo(pszPath, dwHandle, cb, pvBuf))
        {
            UINT cbValue = 0;
            VS_FIXEDFILEINFO *pFileInfo;
            if (VerQueryValue(pvBuf, "\\", (void **)&pFileInfo, &cbValue))
            {
                /** @todo somehow reject 64-bit .dlls when in 32-bit mode... dwFileOS is completely useless. */
                if (    *pu32FileVersionMS < pFileInfo->dwFileVersionMS
                    ||  (   *pu32FileVersionMS == pFileInfo->dwFileVersionMS
                         && *pu32FileVersionLS  > pFileInfo->dwFileVersionLS))
                {
                    *pu32FileVersionMS = pFileInfo->dwFileVersionMS;
                    *pu32FileVersionLS = pFileInfo->dwFileVersionLS;
                }
                if (pFileInfo->dwFileVersionMS >= 0x60004)
                    rc = 0;
                else
                    rc = KDBG_ERR_DBGHLP_VERSION_MISMATCH;
            }
            else
                rc = KERR_GENERAL_FAILURE;
        }
        else
            rc = kdbgModDHConvWinError(GetLastError());
    }
    else
        rc = kdbgModDHConvWinError(GetLastError());
    return rc;
}


/**
 * Find the dbghelp.dll
 */
static int kdbgModDHFindDbgHelp(char *pszPath, KSIZE cchPath)
{
    /*
     * Try the current directory.
     */
    KU32 FileVersionMS = 0;
    KU32 FileVersionLS = 0;
    int rc = KERR_GENERAL_FAILURE;
    static char s_szDbgHelp[] = "\\dbghelp.dll";
    if (GetCurrentDirectory((DWORD)(cchPath - sizeof(s_szDbgHelp) + 1), pszPath))
    {
        strcat(pszPath, s_szDbgHelp);
        int rc2 = kdbgModDHTryDbgHelp(pszPath, &FileVersionMS, &FileVersionLS);
        if (!rc2)
            return rc2;
        if (rc != KDBG_ERR_DBGHLP_VERSION_MISMATCH)
            rc = rc2;
    }

    /*
     * Try the application directory.
     */
    if (GetModuleFileName(NULL, pszPath, (DWORD)(cchPath - sizeof(s_szDbgHelp) + 1)))
    {
        kHlpStrCat(kHlpStrRChr(pszPath, '\\'), s_szDbgHelp);
        int rc2 = kdbgModDHTryDbgHelp(pszPath, &FileVersionMS, &FileVersionLS);
        if (!rc)
            return rc2;
        if (rc != KDBG_ERR_DBGHLP_VERSION_MISMATCH)
            rc = rc2;
    }

    /*
     * Try the windows directory.
     */
    if (GetSystemDirectory(pszPath, (DWORD)(cchPath - sizeof(s_szDbgHelp) + 1)))
    {
        kHlpStrCat(pszPath, s_szDbgHelp);
        int rc2 = kdbgModDHTryDbgHelp(pszPath, &FileVersionMS, &FileVersionLS);
        if (!rc2)
            return rc2;
        if (rc != KDBG_ERR_DBGHLP_VERSION_MISMATCH)
            rc = rc2;
    }

    /*
     * Try the windows directory.
     */
    if (GetWindowsDirectory(pszPath, (DWORD)(cchPath - sizeof(s_szDbgHelp) + 1)))
    {
        kHlpStrCat(pszPath, s_szDbgHelp);
        int rc2 = kdbgModDHTryDbgHelp(pszPath, &FileVersionMS, &FileVersionLS);
        if (!rc2)
            return rc2;
        if (rc != KDBG_ERR_DBGHLP_VERSION_MISMATCH)
            rc = rc2;
    }

    /*
     * Try the path.
     */
    /** @todo find the actual path specs, I'm probably not doing this 100% correctly here. */
    DWORD cb = GetEnvironmentVariable("PATH", NULL, 0) + 64;
    char *pszSearchPath = (char *) alloca(cb);
    if (GetEnvironmentVariable("PATH", pszSearchPath, cb) < cb)
    {
        char *psz = pszSearchPath;
        while (*psz)
        {
            /* find the end of the path. */
            char *pszEnd = kHlpStrChr(psz, ';');
            if (!pszEnd)
                pszEnd = kHlpStrChr(psz, '\0');
            if (pszEnd != psz)
            {
                /* construct filename and try it out */
                kHlpMemCopy(pszPath, psz, pszEnd - psz);
                kHlpMemCopy(&pszPath[pszEnd - psz], s_szDbgHelp, sizeof(s_szDbgHelp));
                int rc2 = kdbgModDHTryDbgHelp(pszPath, &FileVersionMS, &FileVersionLS);
                if (!rc2)
                    return rc2;
                if (rc != KDBG_ERR_DBGHLP_VERSION_MISMATCH)
                    rc = rc2;
            }

            /* next path */
            if (!*pszEnd)
                break;
            psz = pszEnd + 1;
        }
    }

    if (rc == KDBG_ERR_DBGHLP_VERSION_MISMATCH)
        kDbgAssertMsgFailed(("dbghelp.dll found, but it was ancient! The highest file version found was 0x%08x'%08x.\n"
                             "This program require a file version of at least 0x00060004'00000000. Please download\n"
                             "the latest windbg and use the dbghelp.dll from that package. Just put it somewhere in\n"
                             "the PATH and we'll find it.\n", FileVersionMS, FileVersionLS));
    else
        kDbgAssertMsgFailed(("dbghelp.dll was not found! Download the latest windbg and use the dbghelp.dll\n"
                             "from that package - just put it somewhere in the PATH and we'll find it.\n"));
    return rc;
}


/**
 * Loads the dbghelp.dll, check that it's the right version, and
 * resolves all the symbols we need.
 *
 * @returns IPRT status code.
 */
static int kdbgModDHLoadDbgHelp(void)
{
    if (g_hDbgHelp)
        return 0;

    /* primitive locking - make some useful API for this kind of spinning! */
    static volatile long s_lLock = 0;
    while (InterlockedCompareExchange(&s_lLock, 1, 0))
        while (s_lLock)
            Sleep(1);
    if (g_hDbgHelp)
    {
        InterlockedExchange(&s_lLock, 0);
        return 0;
    }

    /*
     * Load it - try current dir first.
     */
    char szPath[260];
    int rc = kdbgModDHFindDbgHelp(szPath, sizeof(szPath));
    if (rc)
    {
        InterlockedExchange(&s_lLock, 0);
        return rc;
    }

    HMODULE hmod = LoadLibraryEx(szPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!hmod)
    {
        DWORD Err = GetLastError();
        int rc = kdbgModDHConvWinError(Err);
        InterlockedExchange(&s_lLock, 0);
        kDbgAssertMsgFailedReturn(("Failed to load '%s', Err=%d rc=%d\n", szPath, Err, rc), rc);
    }

    /*
     * Check the API version (too).
     */
    LPAPI_VERSION (WINAPI *pfnImagehlpApiVersion)(VOID);
    FARPROC *ppfn = (FARPROC *)&pfnImagehlpApiVersion;
    *ppfn = GetProcAddress(hmod, "ImagehlpApiVersion");
    if (*ppfn)
    {
        LPAPI_VERSION pVersion = pfnImagehlpApiVersion();
        if (    pVersion
            &&  (   pVersion->MajorVersion > 4
                 || (pVersion->MajorVersion == 4 && pVersion->MinorVersion > 0)
                 || (pVersion->MajorVersion == 4 && pVersion->MinorVersion == 0 && pVersion->Revision >= 5)
                )
           )
        {
            /*
             * Resolve the entrypoints we need.
             */
            static const struct
            {
                const char *pszName;
                FARPROC *ppfn;
            }   s_aFunctions[] =
            {
                { "SymInitialize",      (FARPROC *)&g_pfnSymInitialize },
                { "SymCleanup",         (FARPROC *)&g_pfnSymCleanup },
                { "SymSetOptions",      (FARPROC *)&g_pfnSymSetOptions },
                { "SymLoadModule64",    (FARPROC *)&g_pfnSymLoadModule64 },
                { "SymFromAddr",        (FARPROC *)&g_pfnSymFromAddr },
                { "SymFromAddr",        (FARPROC *)&g_pfnSymFromAddr },
                { "SymGetLineFromAddr64", (FARPROC *)&g_pfnSymGetLineFromAddr64 },
            };
            for (unsigned i = 0; i < K_ELEMENTS(s_aFunctions); i++)
            {
                FARPROC pfn = GetProcAddress(hmod, s_aFunctions[i].pszName);
                if (!pfn)
                {
                    DWORD Err = GetLastError();
                    rc = kdbgModDHConvWinError(Err);
                    kDbgAssertMsgFailed(("Failed to resolve %s in dbghelp, Err=%d rc=%d\n",
                                         s_aFunctions[i].pszName, Err, rc));
                    break;
                }
                *s_aFunctions[i].ppfn = pfn;
            }
            if (!rc)
            {
                g_hDbgHelp = hmod;
                Sleep(1);
                InterlockedExchange(&s_lLock, 0);
                return 0;
            }
        }
        else
        {
            rc = KDBG_ERR_DBGHLP_VERSION_MISMATCH;
            kDbgAssertMsgFailed(("ImagehlpApiVersion -> %p and MajorVersion=%d.\n", pVersion, pVersion ? pVersion->MajorVersion : 0));
        }
    }
    else
    {
        DWORD Err = GetLastError();
        rc = kdbgModDHConvWinError(Err);
        kDbgAssertMsgFailed(("Failed to resolve ImagehlpApiVersionEx in dbghelp, Err=%d rc=%d\n", Err, rc));
    }
    FreeLibrary(hmod);
    InterlockedExchange(&s_lLock, 0);
    return rc;
}


/**
 * @copydoc KDBGMODOPS::pfnOpen
 */
static int kdbgModDHOpen(PKDBGMOD *ppMod, PKRDR pRdr, KBOOL fCloseRdr, KFOFF off, KFOFF cb, struct KLDRMOD *pLdrMod)
{
    /*
     * This reader doesn't support partial files.
     * Also weed out small files early on as they cannot be
     * PE images and will only cause read errors
     */
    if (    off != 0
        ||  cb != KFOFF_MAX)
        return KDBG_ERR_UNKOWN_FORMAT;
    if (kRdrSize(pRdr) < sizeof(IMAGE_NT_HEADERS32) + sizeof(IMAGE_SECTION_HEADER))
        return KDBG_ERR_UNKOWN_FORMAT;

    /*
     * We need to read the section headers and get the image size.
     */
    /* Find the PE header magic. */
    KU32 offHdr = 0;
    KU32 u32Magic;
    int rc = kRdrRead(pRdr, &u32Magic, sizeof(u32Magic), 0);
    kDbgAssertRCReturn(rc, rc);
    if ((KU16)u32Magic == IMAGE_DOS_SIGNATURE)
    {
        rc = kRdrRead(pRdr, &offHdr, sizeof(offHdr), K_OFFSETOF(IMAGE_DOS_HEADER, e_lfanew));
        kDbgAssertRCReturn(rc, rc);
        if (!offHdr)
            return KDBG_ERR_FORMAT_NOT_SUPPORTED;
        if (    offHdr < sizeof(IMAGE_DOS_SIGNATURE)
            ||  offHdr >= kRdrSize(pRdr) - 4)
            return KDBG_ERR_BAD_EXE_FORMAT;

        rc = kRdrRead(pRdr, &u32Magic, sizeof(u32Magic), offHdr);
        kDbgAssertRCReturn(rc, rc);
    }
    if (u32Magic != IMAGE_NT_SIGNATURE)
        return KDBG_ERR_FORMAT_NOT_SUPPORTED;

    /* read the file header and the image size in the optional header.. */
    IMAGE_FILE_HEADER FHdr;
    rc = kRdrRead(pRdr, &FHdr, sizeof(FHdr), offHdr + K_OFFSETOF(IMAGE_NT_HEADERS32, FileHeader));
    kDbgAssertRCReturn(rc, rc);

    KU32 cbImage;
    if (FHdr.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER32))
        rc = kRdrRead(pRdr, &cbImage, sizeof(cbImage),
                      offHdr + K_OFFSETOF(IMAGE_NT_HEADERS32, OptionalHeader.SizeOfImage));
    else if (FHdr.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER64))
        rc = kRdrRead(pRdr, &cbImage, sizeof(cbImage),
                      offHdr + K_OFFSETOF(IMAGE_NT_HEADERS64, OptionalHeader.SizeOfImage));
    else
        kDbgAssertFailedReturn(KDBG_ERR_BAD_EXE_FORMAT);
    kDbgAssertRCReturn(rc, rc);

    /*
     * Load dbghelp.dll.
     */
    rc = kdbgModDHLoadDbgHelp();
    if (rc)
        return rc;

    /*
     * Allocate the module and read/construct the section headers.
     */
    PKDBGMODDBGHELP pModDH = (PKDBGMODDBGHELP)kHlpAlloc(K_OFFSETOF(KDBGMODDBGHELP, aSections[FHdr.NumberOfSections + 2]));
    kDbgAssertReturn(pModDH, KERR_NO_MEMORY);
    pModDH->Core.u32Magic   = KDBGMOD_MAGIC;
    pModDH->Core.pOps       = &g_kDbgModWinDbgHelpOpen;
    pModDH->Core.pRdr       = pRdr;
    pModDH->Core.fCloseRdr  = fCloseRdr;
    pModDH->Core.pLdrMod    = pLdrMod;
    pModDH->cbImage         = cbImage;
    pModDH->cSections       = 1 + FHdr.NumberOfSections;

    rc = kRdrRead(pRdr, &pModDH->aSections[1], sizeof(pModDH->aSections[0]) * FHdr.NumberOfSections,
                  offHdr + K_OFFSETOF(IMAGE_NT_HEADERS32, OptionalHeader) + FHdr.SizeOfOptionalHeader);
    if (!rc)
    {
        PIMAGE_SECTION_HEADER pSH = &pModDH->aSections[0];
        kHlpMemCopy(pSH->Name, "headers", sizeof(pSH->Name));
        pSH->Misc.VirtualSize       = pModDH->aSections[1].VirtualAddress;
        pSH->VirtualAddress         = 0;
        pSH->SizeOfRawData          = pSH->Misc.VirtualSize;
        pSH->PointerToRawData       = 0;
        pSH->PointerToRelocations   = 0;
        pSH->PointerToLinenumbers   = 0;
        pSH->NumberOfRelocations    = 0;
        pSH->NumberOfLinenumbers    = 0;
        pSH->Characteristics        = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_ALIGN_4BYTES | IMAGE_SCN_MEM_READ;

        KU32 uTheEnd = pModDH->aSections[FHdr.NumberOfSections].VirtualAddress
                     + pModDH->aSections[FHdr.NumberOfSections].Misc.VirtualSize;
        if (uTheEnd < cbImage)
        {
            pSH = &pModDH->aSections[pModDH->cSections++];
            kHlpMemCopy(pSH->Name, "tail\0\0\0", sizeof(pSH->Name));
            pSH->Misc.VirtualSize       = cbImage - uTheEnd;
            pSH->VirtualAddress         = uTheEnd;
            pSH->SizeOfRawData          = pSH->Misc.VirtualSize;
            pSH->PointerToRawData       = 0;
            pSH->PointerToRelocations   = 0;
            pSH->PointerToLinenumbers   = 0;
            pSH->NumberOfRelocations    = 0;
            pSH->NumberOfLinenumbers    = 0;
            pSH->Characteristics        = IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_ALIGN_1BYTES | IMAGE_SCN_MEM_READ;
        }

        /*
         * Find a new dbghelp handle.
         *
         * We assume 4GB of handles outlast most debugging sessions, or in anyways that
         * when we start reusing handles they are no longer in use. :-)
         */
        static volatile long s_u32LastHandle = 1;
        HANDLE hSymInst = (HANDLE)InterlockedIncrement(&s_u32LastHandle);
        while (     hSymInst == INVALID_HANDLE_VALUE
               ||   hSymInst == (HANDLE)0
               ||   hSymInst == GetCurrentProcess())
            hSymInst = (HANDLE)InterlockedIncrement(&s_u32LastHandle);

        /*
         * Initialize dbghelp and try open the specified module.
         */
        if (g_pfnSymInitialize(hSymInst, NULL, FALSE))
        {
            g_pfnSymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_AUTO_PUBLICS | SYMOPT_ALLOW_ABSOLUTE_SYMBOLS);

            KIPTR NativeFH = kRdrNativeFH(pRdr);
            DWORD64 ImageBase = g_pfnSymLoadModule64(hSymInst, NativeFH == -1 ? NULL : (HANDLE)NativeFH,
                                                     kRdrName(pRdr), NULL, 0x00400000, 0);
            if (ImageBase)
            {
                pModDH->hSymInst        = hSymInst;
                pModDH->ImageBase       = ImageBase;
                *ppMod = &pModDH->Core;
                return rc;
            }

            DWORD Err = GetLastError();
            rc = kdbgModDHConvWinError(Err);
            kDbgAssertMsgFailed(("SymLoadModule64 failed: Err=%d rc=%d\n", Err, rc));
            g_pfnSymCleanup(hSymInst);
        }
        else
        {
            DWORD Err = GetLastError();
            rc = kdbgModDHConvWinError(Err);
            kDbgAssertMsgFailed(("SymInitialize failed: Err=%d rc=%d\n", Err, rc));
        }
    }
    else
        kDbgAssertRC(rc);

    kHlpFree(pModDH);
    return rc;
}


/**
 * Methods for a PE module.
 */
KDBGMODOPS const g_kDbgModWinDbgHelpOpen =
{
    "Windows DbgHelp",
    NULL,
    kdbgModDHOpen,
    kdbgModDHClose,
    kdbgModDHQuerySymbol,
    kdbgModDHQueryLine,
    "Windows DbgHelp"
};

