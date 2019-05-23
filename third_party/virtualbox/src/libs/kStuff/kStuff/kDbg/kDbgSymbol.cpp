/* $Id: kDbgSymbol.cpp 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kDbg - The Debug Info Reader, Symbols.
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
#include <k/kHlpAlloc.h>


/**
 * Duplicates a symbol.
 *
 * To save heap space, the returned symbol will not own more heap space than
 * it strictly need to. So, it's not possible to append stuff to the symbol
 * or anything of that kind.
 *
 * @returns Pointer to the duplicate.
 *          This must be freed using kDbgSymbolFree().
 * @param   pSymbol     The symbol to be duplicated.
 */
KDBG_DECL(PKDBGSYMBOL) kDbgSymbolDup(PCKDBGSYMBOL pSymbol)
{
    kDbgAssertPtrReturn(pSymbol, NULL);
    KSIZE cb = K_OFFSETOF(KDBGSYMBOL, szName[pSymbol->cchName + 1]);
    PKDBGSYMBOL pNewSymbol = (PKDBGSYMBOL)kHlpDup(pSymbol, cb);
    if (pNewSymbol)
        pNewSymbol->cbSelf = cb;
    return pNewSymbol;
}


/**
 * Frees a symbol obtained from the kDbg API.
 *
 * @returns 0 on success.
 * @returns KERR_INVALID_POINTER if pSymbol isn't a valid pointer.
 *
 * @param   pSymbol     The symbol to be freed. The null pointer is ignored.
 */
KDBG_DECL(int) kDbgSymbolFree(PKDBGSYMBOL pSymbol)
{
    if (!pSymbol)
    {
        kDbgAssertPtrReturn(pSymbol, KERR_INVALID_POINTER);
        pSymbol->cbSelf = 0;
        kHlpFree(pSymbol);
    }
    return 0;
}

