/* $Id: kHlpIsFilenameOnly.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kHlpPath - kHlpIsFilenameOnly.
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
 * Checks if this is only a filename or if it contains any kind
 * of drive, directory, or server specs.
 *
 * @returns 1 if this is a filename only.
 * @returns 0 of it's isn't only a filename.
 * @param   pszFilename     The filename to parse.
 */
KHLP_DECL(int) kHlpIsFilenameOnly(const char *pszFilename)
{
    for (;;)
    {
        const char ch = *pszFilename++;
#if K_OS == K_OS_OS2 || K_OS == K_OS_WINDOWS
        if (ch == '/' || ch == '\\' || ch == ':')
#else
        if (ch == '/')
#endif
            return 0;
        if (!ch)
            return 1;
    }
}

