/* $Id: kHlpInt2Ascii.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kHlpString - kHlpInt2Ascii.
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
#include <k/kHlpString.h>


/**
 * Converts an signed integer to an ascii string.
 *
 * @returns psz.
 * @param   psz         Pointer to the output buffer.
 * @param   cch         The size of the output buffer.
 * @param   lVal        The value.
 * @param   iBase       The base to format it. (2,8,10 or 16)
 */
KHLP_DECL(char *) kHlpInt2Ascii(char *psz, KSIZE cch, long lVal, unsigned iBase)
{
    static const char s_szDigits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char *pszRet = psz;

    if (cch >= (lVal < 0 ? 3U : 2U) && psz)
    {
        /* prefix */
        if (lVal < 0)
        {
            *psz++ = '-';
            cch--;
            lVal = -lVal;
        }

        /* the digits */
        do
        {
            *psz++ = s_szDigits[lVal % iBase];
            cch--;
            lVal /= iBase;
        } while (lVal && cch > 1);

        /* overflow indicator */
        if (lVal)
            psz[-1] = '+';
    }
    else if (!pszRet)
        return pszRet;
    else if (cch < 1 || !pszRet)
        return pszRet;
    else
        *psz++ = '+';
    *psz = '\0';

    return pszRet;
}

