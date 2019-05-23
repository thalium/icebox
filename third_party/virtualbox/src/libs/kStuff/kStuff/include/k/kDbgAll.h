/* $Id: kDbgAll.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kDbg - The Debug Info Read, All Details and Dependencies Included.
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

#ifndef ___k_kDbgAll_h___
#define ___k_kDbgAll_h___

#include <k/kDefs.h>
#include <k/kTypes.h>
#include <k/kRdr.h>
#include <k/kLdr.h>
#include <k/kDbg.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup grp_kDbgAll   All
 * @addtogroup grp_kDbg
 * @{
 */

/**
 * The debug module method table.
 */
typedef struct KDBGMODOPS
{
    /** The name of the reader. */
    const char *pszName;

    /** Pointer to the next debug module readers.
     * This is only used for dynamically registered readers. */
    struct KDBGMODOPS  *pNext;

    /**
     * Tries to open the module.
     *
     * @returns 0 on success, KDBG_ERR on failure.
     * @param   ppMod           Where to store the module that's been opened.
     * @param   pRdr            The file provider.
     * @param   fCloseRdrs       Whether the reader should be closed or not when the module is destroyed.
     * @param   off             The file offset of the debug info. This is 0 if there isn't
     *                          any specfic debug info section and the reader should start
     *                          looking for debug info at the start of the file.
     * @param   cb              The size of the debug info in the file. INT64_MAX if we don't
     *                          know or there isn't any particular debug info section in the file.
     * @param   pLdrMod         The associated loader module. This can be NULL.
     */
    int (*pfnOpen)(PKDBGMOD *ppMod, PKRDR pRdr, KBOOL fCloseRdr, KFOFF off, KFOFF cb, struct KLDRMOD *pLdrMod);

    /**
     * Closes the module.
     *
     * This should free all resources associated with the module
     * except the pMod which is freed by the caller.
     *
     * @returns IPRT status code.
     * @param   pMod        The module.
     */
    int (*pfnClose)(PKDBGMOD pMod);

    /**
     * Gets a symbol by segment:offset.
     * This will be approximated to the nearest symbol if there is no exact match.
     *
     * @returns 0 on success. KLDR_ERR_* on failure.
     * @param   pMod        The module.
     * @param   iSegment    The segment this offset is relative to.
     *                      The -1 segment is special, it means that the addres is relative to
     *                      the image base. The image base is where the first bit of the image
     *                      is mapped during load.
     * @param   off         The offset into the segment.
     * @param   pSym        Where to store the symbol details.
     */
    int (*pfnQuerySymbol)(PKDBGMOD pMod, KI32 iSegment, KDBGADDR off, PKDBGSYMBOL pSym);

    /**
     * Gets a line number entry by segment:offset.
     * This will be approximated to the nearest line number there is no exact match.
     *
     * @returns 0 on success. KLDR_ERR_* on failure.
     * @param   pMod        The module.
     * @param   iSegment    The segment this offset is relative to.
     *                      The -1 segment is special, it means that the addres is relative to
     *                      the image base. The image base is where the first bit of the image
     *                      is mapped during load.
     * @param   off         The offset into the segment.
     * @param   pLine       Where to store the line number details.
     */
    int (*pfnQueryLine)(PKDBGMOD pMod, KI32 iSegment, KDBGADDR uOffset, PKDBGLINE pLine);

    /** This is just to make sure you've initialized all the fields.
     * Must be identical to pszName. */
    const char *pszName2;
} KDBGMODOPS;
/** Pointer to a module method table. */
typedef KDBGMODOPS *PKDBGMODOPS;
/** Pointer to a const module method table. */
typedef const KDBGMODOPS *PCKDBGMODOPS;

/**
 * Register a debug module reader with the kDbgModule component.
 *
 * Dynamically registered readers are kept in FIFO order, and external
 * readers will be tried after the builtin ones.
 *
 * @returns 0 on success.
 * @returns KERR_INVALID_POINTER if pOps is missing bits.
 * @returns KERR_INVALID_PARAMETER if pOps is already in the list.
 * @param   pOps        The reader method table, kDbg takes owner ship of
 *                      this. This must be writeable as the pNext pointer
 *                      will be update. It must also stick around for as
 *                      long as kDbg is in use.
 */
KDBG_DECL(int) kDbgModuleRegisterReader(PKDBGMODOPS pOps);



/**
 * Internal representation of a debug module.
 */
typedef struct KDBGMOD
{
    /** Magic value (KDBGMOD_MAGIC). */
    KI32            u32Magic;
    /** Pointer to the method table. */
    PCKDBGMODOPS    pOps;
    /** The file provider for the file containing the debug info. */
    PKRDR           pRdr;
    /** Whether or not to close pRdr. */
    KBOOL           fCloseRdr;
    /** The associated kLdr module. This may be NULL. */
    PKLDRMOD        pLdrMod;
} KDBGMOD;

/** @}*/

#ifdef __cplusplus
}
#endif

#endif
