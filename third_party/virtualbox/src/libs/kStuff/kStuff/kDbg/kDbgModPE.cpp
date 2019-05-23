/* $Id: kDbgModPE.cpp 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kDbg - The Debug Info Reader, PE Module (Generic).
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
#include "kDbg.h"
#include "kDbgInternal.h"
#include <kLdrModPE.h>
#include <string.h>


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * A dbghelp based PE debug reader.
 */
typedef struct KDBGMODPE
{
    /** The common module core. */
    KDBGMOD     Core;
    /** The image size. */
    uint32_t    cbImage;
    /** The number of sections. (We've added the implicit header section.) */
    int32_t     cSections;
    /** The section headers (variable size). The first section is the
     * implicit header section.*/
    IMAGE_SECTION_HEADER aSections[1];
} KDBGMODPE, *PKDBGMODPE;


/**
 * Calcs the RVA for a segment:offset address.
 *
 * @returns IPRT status code.
 *
 * @param   pModPe      The PE debug module instance.
 * @param   iSegment    The segment number. Special segments are dealt with as well.
 * @param   off         The segment offset.
 * @param   puRVA       Where to store the RVA on success.
 */
static int kDbgModPeSegOffToRVA(PKDBGMODPE pModPe, int32_t iSegment, KDBGADDR off, uint32_t *puRVA)
{
    if (iSegment >= 0)
    {
        kDbgAssertMsgReturn(iSegment < pModPe->cSections, ("iSegment=%x cSections=%x\n", iSegment, pModPe->cSections),
                        KDBG_ERR_INVALID_ADDRESS);
        kDbgAssertMsgReturn(off < pModPe->aSections[iSegment].Misc.VirtualSize,
                        ("off=" PRI_KDBGADDR " VirtualSize=%x\n", off, pModPe->aSections[iSegment].Misc.VirtualSize),
                        KDBG_ERR_INVALID_ADDRESS);
        *puRVA = pModPe->aSections[iSegment].VirtualAddress + (uint32_t)off;
        return 0;
    }

    if (iSegment == KDBGSEG_RVA)
    {
        kDbgAssertMsgReturn(off < pModPe->cbImage, ("off=" PRI_KDBGADDR ", cbImage=%x\n", off, pModPe->cbImage),
                            KDBG_ERR_INVALID_ADDRESS);
        *puRVA = (uint32_t)off;
        return 0;
    }
    kDbgAssertMsgFailedReturn(("iSegment=%d\n", iSegment), KDBG_ERR_INVALID_ADDRESS);
}


/**
 * Calcs the segment:offset address for a RVA.
 *
 * @returns IPRT status code.
 *
 * @param   pModPe      The PE debug module instance.
 * @param   uRVA        The RVA.
 * @param   piSegment   Where to store the segment number.
 * @param   poff        Where to store the segment offset.
 */
static int kDbgModPeRVAToSegOff(PKDBGMODPE pModPe, uint32_t uRVA, int32_t *piSegment, KDBGADDR *poff)
{
    kDbgAssertMsgReturn(uRVA < pModPe->cbImage, ("uRVA=%x, cbImage=%x\n", uRVA, pModPe->cbImage),
                        KDBG_ERR_INVALID_ADDRESS);
    for (int32_t iSegment = 0; iSegment < pModPe->cSections; iSegment++)
    {
        /** @todo should probably be less strict about address in the alignment gaps. */
        uint32_t off = uRVA - pModPe->aSections[iSegment].VirtualAddress;
        if (off < pModPe->aSections[iSegment].Misc.VirtualSize)
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
static int kDbgModPeQueryLine(PKDBGMOD pMod, int32_t iSegment, KDBGADDR off, PKDBGLINE pLine)
{
    PKDBGMODPE pModPe = (PKDBGMODPE)pMod;

    /*
     * Translate the address to an RVA.
     */
    uint32_t uRVA;
    int rc = kDbgModPeSegOffToRVA(pModPe, iSegment, off, &uRVA);
    if (!rc)
    {
#if 0
        DWORD64 off;
        IMAGEHLP_LINE64 Line;
        Line.SizeOfStruct = sizeof(Line);
        if (g_pfnSymGetLineFromAddr64(pModPe->hSymInst, pModPe->ImageBase + uRVA, &off, &Line))
        {
            pLine->RVA = (KDBGADDR)(Line.Address - pModPe->ImageBase);
            rc = kDbgModPeRVAToSegOff(pModPe, pLine->RVA, &pLine->iSegment, &pLine->offSegment);
            pLine->iLine = Line.LineNumber;
            pLine->cchFile = strlen(Line.FileName);
            if (pLine->cchFile >= sizeof(pLine->szFile))
                pLine->cchFile = sizeof(pLine->szFile) - 1;
            memcpy(pLine->szFile, Line.FileName, pLine->cchFile + 1);
        }
        else
        {
            DWORD Err = GetLastError();
            rc = kDbgModPeConvWinError(Err);
        }
#endif
        rc = KERR_NOT_IMPLEMENTED;
    }
    return rc;
}


/**
 * @copydoc KDBGMODOPS::pfnQuerySymbol
 */
static int kDbgModPeQuerySymbol(PKDBGMOD pMod, int32_t iSegment, KDBGADDR off, PKDBGSYMBOL pSym)
{
    PKDBGMODPE pModPe = (PKDBGMODPE)pMod;

    /*
     * Translate the address to an RVA.
     */
    uint32_t uRVA;
    int rc = kDbgModPeSegOffToRVA(pModPe, iSegment, off, &uRVA);
    if (!rc)
    {
#if 0
        DWORD64 off;
        union
        {
            SYMBOL_INFO Sym;
            char        achBuffer[sizeof(SYMBOL_INFO) + KDBG_SYMBOL_MAX];
        } Buf;
        Buf.Sym.SizeOfStruct = sizeof(SYMBOL_INFO);
        Buf.Sym.MaxNameLen = KDBG_SYMBOL_MAX;
        if (g_pfnSymFromAddr(pModPe->hSymInst, pModPe->ImageBase + uRVA, &off, &Buf.Sym))
        {
            pSym->cb     = Buf.Sym.Size;
            pSym->fFlags = 0;
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
                pSym->RVA        = (KDBGADDR)(Buf.Sym.Address - pModPe->ImageBase);
                rc = kDbgModPeRVAToSegOff(pModPe, pSym->RVA, &pSym->iSegment, &pSym->offSegment);
            }
            pSym->cchName = (uint16_t)Buf.Sym.NameLen;
            if (pSym->cchName >= sizeof(pSym->szName))
                pSym->cchName = sizeof(pSym->szName) - 1;
            memcpy(pSym->szName, Buf.Sym.Name, Buf.Sym.NameLen);
            pSym->szName[Buf.Sym.NameLen] = '\0';
        }
        else
        {
            DWORD Err = GetLastError();
            rc = kDbgModPeConvWinError(Err);
        }
#endif
        rc = KERR_NOT_IMPLEMENTED;
    }
    return rc;
}


/**
 * @copydoc KDBGMODOPS::pfnClose
 */
static int kDbgModPeClose(PKDBGMOD pMod)
{
    PKDBGMODPE pModPe = (PKDBGMODPE)pMod;

    //if (g_pfnSymCleanup(pModPe->hSymInst))
    //    return 0;
    //
    //DWORD Err = GetLastError();
    //int rc = kDbgModPeConvWinError(Err);
    //kDbgAssertMsgFailed(("SymInitialize failed: Err=%d rc=%Rrc\n", Err, rc));
    //return rc;
    return KERR_NOT_IMPLEMENTED;
}


/**
 * Opens the debug info for a PE image using the windows dbghelp library.
 *
 * @returns IPRT status code.
 *
 * @param   pFile               The handle to the module.
 * @param   offHdr              The offset of the PE header.
 * @param   pszModulePath       The path to the module.
 * @param   ppDbgMod            Where to store the module handle.
 *
 */
int kdbgModPEOpen(PKDBGHLPFILE pFile, int64_t offHdr, const char *pszModulePath, PKDBGMOD *ppDbgMod)
{
    /*
     * We need to read the section headers and get the image size.
     */
    IMAGE_FILE_HEADER FHdr;
    int rc = kDbgHlpReadAt(pFile, offHdr + KDBG_OFFSETOF(IMAGE_NT_HEADERS32, FileHeader), &FHdr, sizeof(FHdr));
    kDbgAssertRCReturn(rc, rc);

    uint32_t cbImage;
    if (FHdr.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER32))
        rc = kDbgHlpReadAt(pFile, offHdr + KDBG_OFFSETOF(IMAGE_NT_HEADERS32, OptionalHeader.SizeOfImage),
                           &cbImage, sizeof(cbImage));
    else if (FHdr.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER64))
        rc = kDbgHlpReadAt(pFile, offHdr + KDBG_OFFSETOF(IMAGE_NT_HEADERS64, OptionalHeader.SizeOfImage),
                           &cbImage, sizeof(cbImage));
    else
        kDbgAssertFailedReturn(KDBG_ERR_BAD_EXE_FORMAT);
    kDbgAssertRCReturn(rc, rc);

    /*
     * Allocate the module and read/construct the section headers.
     */
    PKDBGMODPE pModPe = (PKDBGMODPE)kDbgHlpAlloc(KDBG_OFFSETOF(KDBGMODPE, aSections[FHdr.NumberOfSections + 2]));
    kDbgAssertReturn(pModPe, KERR_NO_MEMORY);
    pModPe->Core.u32Magic   = KDBGMOD_MAGIC;
    pModPe->Core.pOps       = &g_kDbgModPeOps;
    pModPe->Core.pFile      = pFile;
    pModPe->cbImage         = cbImage;
    pModPe->cSections       = 1 + FHdr.NumberOfSections;
    rc = kDbgHlpReadAt(pFile, offHdr + KDBG_OFFSETOF(IMAGE_NT_HEADERS32, OptionalHeader) + FHdr.SizeOfOptionalHeader,
                       &pModPe->aSections[1], sizeof(pModPe->aSections[0]) * FHdr.NumberOfSections);
    if (!rc)
    {
        PIMAGE_SECTION_HEADER pSH = &pModPe->aSections[0];
        memcpy(pSH->Name, "headers", sizeof(pSH->Name));
        pSH->Misc.VirtualSize       = pModPe->aSections[1].VirtualAddress;
        pSH->VirtualAddress         = 0;
        pSH->SizeOfRawData          = pSH->Misc.VirtualSize;
        pSH->PointerToRawData       = 0;
        pSH->PointerToRelocations   = 0;
        pSH->PointerToLinenumbers   = 0;
        pSH->NumberOfRelocations    = 0;
        pSH->NumberOfLinenumbers    = 0;
        pSH->Characteristics        = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_ALIGN_4BYTES | IMAGE_SCN_MEM_READ;

        uint32_t uTheEnd = pModPe->aSections[FHdr.NumberOfSections].VirtualAddress
                         + pModPe->aSections[FHdr.NumberOfSections].Misc.VirtualSize;
        if (uTheEnd < cbImage)
        {
            pSH = &pModPe->aSections[pModPe->cSections++];
            memcpy(pSH->Name, "tail\0\0\0", sizeof(pSH->Name));
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

#if 0
        /*
         * Find a new dbghelp handle.
         *
         * We assume 4GB of handles outlast most debugging sessions, or in anyways that
         * when we start reusing handles they are no longer in use. :-)
         */
        static volatile uint32_t s_u32LastHandle = 1;
        HANDLE hSymInst = (HANDLE)ASMAtomicIncU32(&s_u32LastHandle);
        while (     hSymInst == INVALID_HANDLE_VALUE
               ||   hSymInst == (HANDLE)0
               ||   hSymInst == GetCurrentProcess())
            hSymInst = (HANDLE)ASMAtomicIncU32(&s_u32LastHandle);

        /*
         * Initialize dbghelp and try open the specified module.
         */
        if (g_pfnSymInitialize(hSymInst, NULL, FALSE))
        {
            g_pfnSymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_AUTO_PUBLICS | SYMOPT_ALLOW_ABSOLUTE_SYMBOLS);

            kDbgHlpSeek(pFile, 0); /* don't know if this is required or not... */
            DWORD64 ImageBase = g_pfnSymLoadModule64(hSymInst, (HANDLE)File, pszModulePath, NULL, 0x00400000, 0);
            if (ImageBase)
            {
                pModPe->hSymInst    = hSymInst;
                pModPe->ImageBase   = ImageBase;
                *ppDbgMod = &pModPe->Core;
                return rc;
            }

            DWORD Err = GetLastError();
            rc = kDbgModPeConvWinError(Err);
            kDbgAssertMsgFailed(("SymLoadModule64 failed: Err=%d rc=%Rrc\n", Err, rc));
            g_pfnSymCleanup(hSymInst);
        }
        else
        {
            DWORD Err = GetLastError();
            rc = kDbgModPeConvWinError(Err);
            kDbgAssertMsgFailed(("SymInitialize failed: Err=%d rc=%Rrc\n", Err, rc));
        }
#endif
        rc = KERR_NOT_IMPLEMENTED;
    }
    else
        kDbgAssertRC(rc);

    kDbgHlpFree(pModPe);
    return rc;
}


/**
 * Methods for a PE module.
 */
const KDBGMODOPS g_kDbgModPeOps =
{
    "PE",
    kDbgModPeClose,
    kDbgModPeQuerySymbol,
    kDbgModPeQueryLine
};



