/* $Id: kHlpGetFilename.c 85 2016-09-06 03:21:04Z bird $ */
/** @file
 * kHlpPath - kHlpGetFilename.
 */

/*
 * Copyright (c) 2006-2016 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
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
 * Get the pointer to the filename part of the name.
 *
 * @returns Pointer to where the filename starts within the string pointed to by pszFilename.
 * @returns Pointer to the terminator char if no filename.
 * @param   pszFilename     The filename to parse.
 */
KHLP_DECL(char *) kHlpGetFilename(const char *pszFilename)
{
    const char *pszLast = pszFilename;
    for (;;)
    {
        char ch = *pszFilename;
#if K_OS == K_OS_OS2 || K_OS == K_OS_WINDOWS
        if (ch == '/' || ch == '\\' || ch == ':')
        {
            while ((ch = *++pszFilename) == '/' || ch == '\\' || ch == ':')
                /* nothing */;
            pszLast = pszFilename;
        }
#else
        if (ch == '/')
        {
            while ((ch = *++pszFilename) == '/')
                /* betsuni */;
            pszLast = pszFilename;
        }
#endif
        if (ch)
            pszFilename++;
        else
            return (char *)pszLast;
    }
}

