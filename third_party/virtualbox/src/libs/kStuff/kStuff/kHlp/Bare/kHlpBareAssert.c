/* $Id: kHlpBareAssert.c 82 2016-08-22 21:01:51Z bird $ */
/** @file
 * kHlpBare - Assert Backend.
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
#include <k/kHlpAssert.h>
#include <k/kHlpString.h>

#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_FREEBSD \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_SOLARIS
# include <k/kHlpSys.h>

#elif K_OS == K_OS_OS2
# define INCL_BASE
# define INCL_ERRORS
# include <os2.h>

#elif  K_OS == K_OS_WINDOWS
# include <Windows.h>

#else
# error "port me"
#endif


/**
 * Writes a assert string with unix lineendings.
 *
 * @param   pszMsg  The string.
 */
static void kHlpAssertWrite(const char *pszMsg)
{
#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_FREEBSD \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_SOLARIS
    KSIZE cchMsg = kHlpStrLen(pszMsg);
    kHlpSys_write(2 /* stderr */, pszMsg, cchMsg);

#elif K_OS == K_OS_OS2 ||  K_OS == K_OS_WINDOWS
    /*
     * Line by line.
     */
    ULONG       cbWritten;
    const char *pszNl = kHlpStrChr(pszMsg, '\n');
    while (pszNl)
    {
        cbWritten = pszNl - pszMsg;

# if K_OS == K_OS_OS2
        if (cbWritten)
            DosWrite((HFILE)2, pszMsg, cbWritten, &cbWritten);
        DosWrite((HFILE)2, "\r\n", 2, &cbWritten);
# else /* K_OS == K_OS_WINDOWS */
        if (cbWritten)
            WriteFile((HANDLE)STD_ERROR_HANDLE, pszMsg, cbWritten, &cbWritten, NULL);
        WriteFile((HANDLE)STD_ERROR_HANDLE, "\r\n", 2, &cbWritten, NULL);
# endif

        /* next */
        pszMsg = pszNl + 1;
        pszNl = kHlpStrChr(pszMsg, '\n');
    }

    /*
     * Remaining incomplete line.
     */
    if (*pszMsg)
    {
        cbWritten = kHlpStrLen(pszMsg);
# if K_OS == K_OS_OS2
        DosWrite((HFILE)2, pszMsg, cbWritten, &cbWritten);
# else /* K_OS == K_OS_WINDOWS */
        WriteFile((HANDLE)STD_ERROR_HANDLE, pszMsg, cbWritten, &cbWritten, NULL);
# endif
    }

#else
# error "port me"
#endif
}


KHLP_DECL(void) kHlpAssertMsg1(const char *pszExpr, const char *pszFile, unsigned iLine, const char *pszFunction)
{
    char szLine[16];

    kHlpAssertWrite("\n!!!kLdr Assertion Failed!!!\nExpression: ");
    kHlpAssertWrite(pszExpr);
    kHlpAssertWrite("\nAt: ");
    kHlpAssertWrite(pszFile);
    kHlpAssertWrite("(");
    kHlpAssertWrite(kHlpInt2Ascii(szLine, sizeof(szLine), iLine, 10));
    kHlpAssertWrite(") ");
    kHlpAssertWrite(pszFunction);
    kHlpAssertWrite("\n");
}


KHLP_DECL(void) kHlpAssertMsg2(const char *pszFormat, ...)
{
    kHlpAssertWrite(pszFormat);
}

