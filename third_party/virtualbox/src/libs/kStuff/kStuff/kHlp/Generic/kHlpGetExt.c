/* $Id: kHlpGetExt.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kHlpPath - kHlpGetExt and kHlpGetSuff.
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
#include <k/kHlpPath.h>
#include <k/kHlpString.h>


/**
 * Gets the filename suffix.
 *
 * @returns Pointer to where the suffix starts within the string pointed to by pszFilename.
 * @returns Pointer to the terminator char if no suffix.
 * @param   pszFilename     The filename to parse.
 */
KHLP_DECL(char *) kHlpGetSuff(const char *pszFilename)
{
    const char *pszDot = NULL;
    pszFilename = kHlpGetFilename(pszFilename);
    for (;;)
    {
        char ch = *pszFilename;
        if (ch == '.')
        {
            while ((ch = *++pszFilename) == '.')
                /* nothing */;
            if (ch)
                pszDot = pszFilename - 1;
        }
        if (!ch)
            return (char *)(pszDot ? pszDot : pszFilename);
        pszFilename++;
    }
}


/**
 * Gets the filename extention.
 *
 * @returns Pointer to where the extension starts within the string pointed to by pszFilename.
 * @returns Pointer to the terminator char if no extension.
 * @param   pszFilename     The filename to parse.
 */
KHLP_DECL(char *) kHlpGetExt(const char *pszFilename)
{
    char *psz = kHlpGetSuff(pszFilename);
    return *psz ? psz + 1 : psz;
}

