/* $Id: kHlpCRTEnv.cpp 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kHlpEnv - Environment Manipulation.
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
#include <k/kHlpEnv.h>
#include <k/kHlpString.h>
#include <k/kErrors.h>
#include <stdlib.h>


KHLP_DECL(int) kHlpGetEnv(const char *pszVar, char *pszVal, KSIZE cchVal)
{
    int rc = 0;
    const char *pszValue = getenv(pszVar);
    if (pszValue)
    {
        KSIZE  cch = kHlpStrLen((const char *)pszValue);
        if (cchVal > cch)
            kHlpMemCopy(pszVal, pszValue, cch + 1);
        else
            rc = KERR_BUFFER_OVERFLOW;
    }
    else
        rc = KERR_ENVVAR_NOT_FOUND;
    return rc;
}

