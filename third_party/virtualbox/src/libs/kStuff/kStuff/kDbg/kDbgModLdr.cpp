/* $Id: kDbgModLdr.cpp 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kDbg - The Debug Info Reader, kLdr Based.
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
#include <k/kDbg.h>
#include <k/kLdr.h>
#include "kDbgInternal.h"


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * A kLdr based debug reader.
 */
typedef struct KDBGMODLDR
{
    /** The common module core. */
    KDBGMOD     Core;
    /** Pointer to the loader module. */
    PKLDRMOD    pLdrMod;
} KDBGMODLDR, *PKDBGMODLDR;


/**
 * @copydoc KDBGMODOPS::pfnQueryLine
 */
static int kDbgModLdrQueryLine(PKDBGMOD pMod, KI32 iSegment, KDBGADDR uOffset, PKDBGLINE pLine)
{
    //PKDBGMODLDR pThis = (PKDBGMODLDR)pMod;
    return KERR_NOT_IMPLEMENTED;
}


/**
 * @copydoc KDBGMODOPS::pfnQuerySymbol
 */
static int kDbgModLdrQuerySymbol(PKDBGMOD pMod, KI32 iSegment, KDBGADDR off, PKDBGSYMBOL pSym)
{
    //PKDBGMODLDR pThis = (PKDBGMODLDR)pMod;
    return KERR_NOT_IMPLEMENTED;
}


/**
 * @copydoc KDBGMODOPS::pfnClose
 */
static int kDbgModLdrClose(PKDBGMOD pMod)
{
    //PKDBGMODLDr pThis = (PKDBGMODLDR)pMod;
    return KERR_NOT_IMPLEMENTED;
}


/**
 * @copydocs  KDBGMODOPS::pfnOpen.
 */
static int kDbgModLdrOpen(PKDBGMOD *ppMod, PKRDR pRdr, KBOOL fCloseRdr, KFOFF off, KFOFF cb, struct KLDRMOD *pLdrMod)
{
    return KERR_NOT_IMPLEMENTED;
}


/**
 * Methods for a PE module.
 */
const KDBGMODOPS g_kDbgModLdr =
{
    "kLdr",
    NULL,
    kDbgModLdrOpen,
    kDbgModLdrClose,
    kDbgModLdrQuerySymbol,
    kDbgModLdrQueryLine,
    "kLdr"
};




