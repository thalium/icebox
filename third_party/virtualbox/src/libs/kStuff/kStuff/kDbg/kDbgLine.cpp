/* $Id: kDbgLine.cpp 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kDbg - The Debug Info Read, Line Numbers.
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
 * Duplicates a line number.
 *
 * To save heap space, the returned line number will not own more heap space
 * than it strictly need to. So, it's not possible to append stuff to the symbol
 * or anything of that kind.
 *
 * @returns Pointer to the duplicate.
 *          This must be freed using kDbgSymbolFree().
 * @param   pLine       The line number to be duplicated.
 */
KDBG_DECL(PKDBGLINE) kDbgLineDup(PCKDBGLINE pLine)
{
    kDbgAssertPtrReturn(pLine, NULL);
    KSIZE cb = K_OFFSETOF(KDBGLINE, szFile[pLine->cchFile + 1]);
    PKDBGLINE pNewLine = (PKDBGLINE)kHlpDup(pLine, cb);
    if (pNewLine)
        pNewLine->cbSelf = cb;
    return pNewLine;
}


/**
 * Frees a line number obtained from the kDbg API.
 *
 * @returns 0 on success.
 * @returns KERR_INVALID_POINTER if pLine isn't a valid pointer.
 *
 * @param   pLine       The line number to be freed. The null pointer is ignored.
 */
KDBG_DECL(int) kDbgLineFree(PKDBGLINE pLine)
{
    if (pLine)
    {
        kDbgAssertPtrReturn(pLine, KERR_INVALID_POINTER);
        pLine->cbSelf = 0;
        kHlpFree(pLine);
    }
    return 0;
}

