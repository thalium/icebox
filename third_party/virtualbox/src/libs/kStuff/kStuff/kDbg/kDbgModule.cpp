/* $Id: kDbgModule.cpp 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kDbg - The Debug Info Reader, Module API.
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
#include "kDbgInternal.h"
#include <k/kHlpString.h>
#include <k/kHlpAlloc.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/**
 * The built-in debug module readers.
 */
static PCKDBGMODOPS const g_aBuiltIns[] =
{
#if K_OS == K_OS_WINDOWS
    &g_kDbgModWinDbgHelpOpen,
#endif
    &g_kDbgModLdr,
//    &g_kDbgModCv8,
//    &g_kDbgModDwarf,
//    &g_kDbgModHll,
//    &g_kDbgModStabs,
//    &g_kDbgModSym,
//    &g_kDbgModMapILink,
//    &g_kDbgModMapMSLink,
//    &g_kDbgModMapNm,
//    &g_kDbgModMapWLink
};

/**
 * The debug module readers registered at runtime.
 */
static PKDBGMODOPS g_pHead = NULL;


/**
 * Register a debug module reader with the kDbgModule component.
 *
 * Dynamically registered readers are kept in FIFO order, and external
 * readers will be tried after the builtin ones.
 *
 * Like all other kDbg APIs serializing is left to the caller.
 *
 * @returns 0 on success.
 * @returns KERR_INVALID_POINTER if pOps is missing bits.
 * @returns KERR_INVALID_PARAMETER if pOps is already in the list.
 * @param   pOps        The reader method table, kDbg takes owner ship of
 *                      this. This must be writeable as the pNext pointer
 *                      will be update. It must also stick around for as
 *                      long as kDbg is in use.
 */
KDBG_DECL(int) kDbgModuleRegisterReader(PKDBGMODOPS pOps)
{
    /*
     * Validate input.
     */
    kDbgAssertPtrReturn(pOps, KERR_INVALID_POINTER);
    kDbgAssertPtrReturn(pOps->pszName, KERR_INVALID_POINTER);
    kDbgAssertPtrReturn(pOps->pfnOpen, KERR_INVALID_POINTER);
    kDbgAssertPtrReturn(pOps->pfnClose, KERR_INVALID_POINTER);
    kDbgAssertPtrReturn(pOps->pfnQuerySymbol, KERR_INVALID_POINTER);
    kDbgAssertPtrReturn(pOps->pfnQueryLine, KERR_INVALID_POINTER);
    kDbgAssertPtrReturn(pOps->pszName2, KERR_INVALID_POINTER);
    if (kHlpStrComp(pOps->pszName, pOps->pszName2))
        return KERR_INVALID_PARAMETER;
    kDbgAssertReturn(pOps->pNext == NULL, KERR_INVALID_PARAMETER);

    /*
     * Link it into the list.
     */
    if (!g_pHead)
        g_pHead = pOps;
    else
    {
        PKDBGMODOPS pPrev = g_pHead;
        while (pPrev->pNext)
            pPrev = pPrev->pNext;
        kDbgAssertReturn(pPrev != pOps, KERR_INVALID_PARAMETER);
        pPrev->pNext = pOps;
    }
    return 0;
}


/**
 * Deregister a debug module reader previously registered using
 * the kDbgModuleRegisterReader API.
 *
 * Deregistering a reader does not mean that non of its functions
 * will be called after successful return, it only means that it
 * will no longer be subjected to new module.
 *
 * @returns 0 on success.
 * @returns KERR_INVALID_POINTER if pOps isn't a valid pointer.
 * @returns KERR_INVALID_PARAMETER if pOps wasn't registered.
 * @param   pOps    The debug module method table to deregister.
 */
KDBG_DECL(int) kDbgModuleDeregisterReader(PKDBGMODOPS pOps)
{
    /*
     * Validate the pointer.
     */
    kDbgAssertPtrReturn(pOps, KERR_INVALID_POINTER);

    /*
     * Find it in the list and unlink it.
     */
    if (g_pHead == pOps)
        g_pHead = pOps->pNext;
    else
    {
        PKDBGMODOPS pPrev = g_pHead;
        while (pPrev && pPrev->pNext != pOps)
            pPrev = pPrev->pNext;
        if (!pPrev)
            return KERR_INVALID_PARAMETER;
        pPrev->pNext = pOps->pNext;
    }
    pOps->pNext = NULL;
    return 0;
}



/**
 * Worker for the kDbgModuleOpen* APIs.
 *
 * This will make sure the reader is buffered. I will also take care of
 * closing the reader opened by kDbgModuleOpen on failure.
 *
 * @returns 0 on success. An appropriate kErrors status code on failure.
 * @param   ppDbgMod        Where to store the new debug module reader instance.
 * @param   pRdr            The file provider.
 * @param   fCloseRdr       Whether pRdr should be close or not. This applies both
 *                          to the failure path and to the success path, where it'll
 *                          be close when the module is closed by kDbgModuleClose().
 * @param   off             The offset into the file where the debug info is supposed
 *                          to be found.
 *                          This is 0 if the entire file is the subject.
 * @param   cb              The size of the debug info part of the file.
 *                          This is KFOFF_MAX if the entire file is the subject.
 * @param   pLdrMod         An optional kLdrMod association.
 */
static int kdbgModuleOpenWorker(PPKDBGMOD ppDbgMod, PKRDR pRdr, KBOOL fCloseRdr, KFOFF off, KFOFF cb, struct KLDRMOD *pLdrMod)
{
    /*
     * If the reader isn't buffered create a buffered wrapper for it.
     */
    int rc;
    PKRDR pRdrWrapped = NULL;
    if (!kRdrBufIsBuffered(pRdr))
    {
        rc = kRdrBufWrap(&pRdrWrapped, pRdr, fCloseRdr);
        if (rc)
        {
            if (fCloseRdr)
                kRdrClose(pRdr);
            return rc;
        }
        pRdr = pRdrWrapped;
    }

    /*
     * Walk the built-in table and the list of registered readers
     * and let each of them have a go at the file. Stop and return
     * on the first one returning successfully.
     */
    rc = KDBG_ERR_UNKOWN_FORMAT;
    for (KSIZE i = 0; i < K_ELEMENTS(g_aBuiltIns); i++)
        if (g_aBuiltIns[i]->pfnOpen)
        {
            int rc2 = g_aBuiltIns[i]->pfnOpen(ppDbgMod, pRdr, fCloseRdr, off, cb, pLdrMod);
            if (!rc2)
                return 0;
            if (rc2 != KDBG_ERR_UNKOWN_FORMAT && rc == KDBG_ERR_UNKOWN_FORMAT)
                rc = rc2;
        }

    for (PKDBGMODOPS pCur = g_pHead; pCur; pCur = pCur->pNext)
        if (pCur->pfnOpen)
        {
            int rc2 = pCur->pfnOpen(ppDbgMod, pRdr, fCloseRdr, off, cb, pLdrMod);
            if (!rc2)
                return 0;
            if (rc2 != KDBG_ERR_UNKOWN_FORMAT && rc == KDBG_ERR_UNKOWN_FORMAT)
                rc = rc2;
        }

    if (pRdrWrapped)
        kRdrClose(pRdrWrapped);
    else if (fCloseRdr)
        kRdrClose(pRdr);
    return rc;
}


/**
 * Opens a debug module reader for the specified file or file section
 *
 * @returns kStuff status code.
 * @param   ppDbgMod            Where to store the debug module reader handle.
 * @param   pRdr                The file reader.
 * @param   off                 The offset of the file section. If the entire file, pass 0.
 * @param   cb                  The size of the file section. If the entire file, pass KFOFF_MAX.
 * @param   pLdrMod             Associated kLdr module that the kDbg component can use to
 *                              verify and suplement the debug info found in the file specified
 *                              by pszFilename. The module will be used by kDbg for as long as
 *                              the returned kDbg module remains open.
 *                              This is an optional parameter, pass NULL if no kLdr module at hand.
 */
KDBG_DECL(int) kDbgModuleOpenFilePart(PPKDBGMOD ppDbgMod, PKRDR pRdr, KFOFF off, KFOFF cb, struct KLDRMOD *pLdrMod)
{
    /*
     * Validate input.
     */
    kDbgAssertPtrReturn(ppDbgMod, KERR_INVALID_POINTER);
    kDbgAssertPtrReturn(pRdr, KERR_INVALID_POINTER);
    kDbgAssertPtrNullReturn(pLdrMod, KERR_INVALID_POINTER);
    kDbgAssertMsgReturn(off >= 0 && off < KFOFF_MAX, (KFOFF_PRI "\n", off), KERR_INVALID_OFFSET);
    kDbgAssertMsgReturn(cb >= 0 && cb <= KFOFF_MAX, (KFOFF_PRI "\n", cb), KERR_INVALID_SIZE);
    kDbgAssertMsgReturn(off + cb > off, ("off=" KFOFF_PRI " cb=" KFOFF_PRI "\n", off, cb), KERR_INVALID_RANGE);
    *ppDbgMod = NULL;

    /*
     * Hand it over to the internal worker.
     */
    return kdbgModuleOpenWorker(ppDbgMod, pRdr, K_FALSE /* fCloseRdr */, off, cb, pLdrMod);
}


/**
 * Opens a debug module reader for the specified file.
 *
 * @returns kStuff status code.
 * @param   ppDbgMod            Where to store the debug module reader handle.
 * @param   pRdr                The file reader.
 * @param   pLdrMod             Associated kLdr module that the kDbg component can use to
 *                              verify and suplement the debug info found in the file specified
 *                              by pszFilename. The module will be used by kDbg for as long as
 *                              the returned kDbg module remains open.
 *                              This is an optional parameter, pass NULL if no kLdr module at hand.
 */
KDBG_DECL(int) kDbgModuleOpenFile(PPKDBGMOD ppDbgMod, PKRDR pRdr, struct KLDRMOD *pLdrMod)
{
    return kDbgModuleOpenFilePart(ppDbgMod, pRdr, 0, KFOFF_MAX, pLdrMod);
}


/**
 * Opens the debug info for a specified executable module.
 *
 * @returns kStuff status code.
 * @param   ppDbgMod            Where to store the debug module handle.
 * @param   pszFilename         The name of the file containing debug info and/or which
 *                              debug info is wanted.
 * @param   pLdrMod             Associated kLdr module that the kDbg component can use to
 *                              verify and suplement the debug info found in the file specified
 *                              by pszFilename. The module will be used by kDbg for as long as
 *                              the returned kDbg module remains open.
 *                              This is an optional parameter, pass NULL if no kLdr module at hand.
 */
KDBG_DECL(int) kDbgModuleOpen(PPKDBGMOD ppDbgMod, const char *pszFilename, struct KLDRMOD *pLdrMod)
{
    /*
     * Validate input.
     */
    kDbgAssertPtrReturn(ppDbgMod, KERR_INVALID_POINTER);
    kDbgAssertPtrReturn(pszFilename, KERR_INVALID_POINTER);
    kDbgAssertMsgReturn(*pszFilename, ("%p\n", pszFilename), KERR_INVALID_PARAMETER);
    kDbgAssertPtrNullReturn(pLdrMod, KERR_INVALID_POINTER);
    *ppDbgMod = NULL;

    /*
     * Open the file and see if we can read it.
     */
    PKRDR pRdr;
    int rc = kRdrBufOpen(&pRdr, pszFilename);
    if (rc)
        return rc;
    rc = kdbgModuleOpenWorker(ppDbgMod, pRdr, K_TRUE /* fCloseRdr */, 0, KFOFF_MAX, pLdrMod);
    return rc;
}


/**
 * Closes the module.
 *
 * @returns IPRT status code.
 * @param   pMod        The module handle.
 */
KDBG_DECL(int) kDbgModuleClose(PKDBGMOD pMod)
{
    KDBGMOD_VALIDATE(pMod);
    int rc = pMod->pOps->pfnClose(pMod);
    if (!rc)
    {
        pMod->u32Magic++;
        kHlpFree(pMod);
    }
    return rc;
}


/**
 * Gets a symbol by segment:offset.
 * This will be approximated to the nearest symbol if there is no exact match.
 *
 * @returns IPRT status code.
 * @param   pMod        The module.
 * @param   iSegment    The segment this offset is relative to.
 *                      The -1 segment is special, it means that the addres is relative to
 *                      the image base. The image base is where the first bit of the image
 *                      is mapped during load.
 * @param   off         The offset into the segment.
 * @param   pSym        Where to store the symbol details.
 */
KDBG_DECL(int) kDbgModuleQuerySymbol(PKDBGMOD pMod, KI32 iSegment, KDBGADDR off, PKDBGSYMBOL pSym)
{
    KDBGMOD_VALIDATE(pMod);
    kDbgAssertPtrReturn(pSym, KERR_INVALID_POINTER);
    return pMod->pOps->pfnQuerySymbol(pMod, iSegment, off, pSym);
}


/**
 * Gets & allocates a symbol by segment:offset.
 * This will be approximated to the nearest symbol if there is no exact match.
 *
 * @returns IPRT status code.
 * @param   pMod        The module.
 * @param   iSegment    The segment this offset is relative to.
 *                      The -1 segment is special, it means that the addres is relative to
 *                      the image base. The image base is where the first bit of the image
 *                      is mapped during load.
 * @param   off         The offset into the segment.
 * @param   ppSym       Where to store the pointer to the symbol info.
 *                      Free the returned symbol using kDbgSymbolFree().
 */
KDBG_DECL(int) kDbgModuleQuerySymbolA(PKDBGMOD pMod, KI32 iSegment, KDBGADDR off, PPKDBGSYMBOL ppSym)
{
    kDbgAssertPtrReturn(ppSym, KERR_INVALID_POINTER);

    KDBGSYMBOL Sym;
    int rc = kDbgModuleQuerySymbol(pMod, iSegment, off, &Sym);
    if (!rc)
    {
        *ppSym = kDbgSymbolDup(&Sym);
        if (!*ppSym)
            rc = KERR_NO_MEMORY;
    }
    else
        *ppSym = NULL;
    return rc;
}


/**
 * Gets a line number entry by segment:offset.
 * This will be approximated to the nearest line number there is no exact match.
 *
 * @returns IPRT status code.
 * @param   pMod        The module.
 * @param   iSegment    The segment this offset is relative to.
 *                      The -1 segment is special, it means that the addres is relative to
 *                      the image base. The image base is where the first bit of the image
 *                      is mapped during load.
 * @param   off         The offset into the segment.
 * @param   pLine       Where to store the line number details.
 */
KDBG_DECL(int) kDbgModuleQueryLine(PKDBGMOD pMod, KI32 iSegment, KDBGADDR off, PKDBGLINE pLine)
{
    KDBGMOD_VALIDATE(pMod);
    kDbgAssertPtrReturn(pLine, KERR_INVALID_POINTER);
    return pMod->pOps->pfnQueryLine(pMod, iSegment, off, pLine);
}


/**
 * Gets & allocates a line number entry by segment:offset.
 * This will be approximated to the nearest line number there is no exact match.
 *
 * @returns IPRT status code.
 * @param   pMod        The module.
 * @param   iSegment    The segment this offset is relative to.
 *                      The -1 segment is special, it means that the addres is relative to
 *                      the image base. The image base is where the first bit of the image
 *                      is mapped during load.
 * @param   off         The offset into the segment.
 * @param   ppLine      Where to store the pointer to the line number info.
 *                      Free the returned line number using kDbgLineFree().
 */
KDBG_DECL(int) kDbgModuleQueryLineA(PKDBGMOD pMod, KI32 iSegment, KDBGADDR off, PPKDBGLINE ppLine)
{
    kDbgAssertPtrReturn(ppLine, KERR_INVALID_POINTER);

    KDBGLINE Line;
    int rc = kDbgModuleQueryLine(pMod, iSegment, off, &Line);
    if (!rc)
    {
        *ppLine = kDbgLineDup(&Line);
        if (!*ppLine)
            rc = KERR_NO_MEMORY;
    }
    else
        *ppLine = NULL;
    return rc;
}

