/* $Id: kHlpGetEnvUZ.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kHlpEnv - kHlpGetEnvUZ.
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


/**
 * Gets an environment variable and converts it to a KSIZE.
 *
 * @returns 0 and *pcb on success.
 * @returns On failure see kHlpGetEnv.
 * @param   pszVar  The name of the variable.
 * @param   pcb     Where to put the value.
 */
KHLP_DECL(int) kHlpGetEnvUZ(const char *pszVar, KSIZE *pcb)
{
    KSIZE       cb;
    unsigned    uBase;
    char        szVal[64];
    KSIZE       cchVal = sizeof(szVal);
    const char *psz;
    int         rc;

    *pcb = 0;
    rc = kHlpGetEnv(pszVar, szVal, cchVal);
    if (rc)
        return rc;

    /* figure out the base. */
    uBase = 10;
    psz = szVal;
    if (    *psz == '0'
        &&  (psz[1] == 'x' || psz[1] == 'X'))
    {
        uBase = 16;
        psz += 2;
    }

    /* convert it up to the first unknown char. */
    cb = 0;
    for(;;)
    {
        const char ch = *psz;
        unsigned uDigit;
        if (!ch)
            break;
        else if (ch >= '0' && ch <= '9')
            uDigit = ch - '0';
        else if (ch >= 'a' && ch <= 'z')
            uDigit = ch - 'a' + 10;
        else if (ch >= 'A' && ch <= 'Z')
            uDigit = ch - 'A' + 10;
        else
            break;
        if (uDigit >= uBase)
            break;

        /* add the digit */
        cb *= uBase;
        cb += uDigit;

        psz++;
    }

    /* check for unit */
    if (*psz == 'm' || *psz == 'M')
        cb *= 1024*1024;
    else if (*psz == 'k' ||*psz == 'K')
        cb *= 1024;
    else if (*psz == 'g' || *psz == 'G')
        cb *= 1024*1024*1024;

    *pcb = cb;
    return 0;
}


