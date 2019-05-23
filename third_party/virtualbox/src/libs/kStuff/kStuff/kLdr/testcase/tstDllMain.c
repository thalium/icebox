/* $Id: tstDllMain.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kLdr testcase.
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
#include "tst.h"

#if K_OS == K_OS_OS2
# define INCL_BASE
# include <os2.h>
# include <string.h>

#elif K_OS == K_OS_WINDOWS
# include <windows.h>
# include <string.h>

#elif K_OS == K_OS_DARWIN
# include <unistd.h>
# include <string.h>

#else
# error "port me"
#endif


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
void tstWrite(const char *psz);



#if K_OS == K_OS_OS2
/**
 * OS/2 DLL 'main'
 */
ULONG _System _DLL_InitTerm(HMODULE hmod, ULONG fFlags)
{
    switch (fFlags)
    {
        case 0:
            tstWrite("init: ");
            tstWrite(g_pszName);
            tstWrite("\n");
            return TRUE;

        case 1:
            tstWrite("term: ");
            tstWrite(g_pszName);
            tstWrite("\n");
            return TRUE;

        default:
            tstWrite("!invalid!: ");
            tstWrite(g_pszName);
            tstWrite("\n");
            return FALSE;
    }
}

#elif K_OS == K_OS_WINDOWS

/**
 * OS/2 DLL 'main'
 */
BOOL __stdcall DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            tstWrite("init: ");
            tstWrite(g_pszName);
            tstWrite("\n");
            return TRUE;

        case DLL_PROCESS_DETACH:
            tstWrite("term: ");
            tstWrite(g_pszName);
            tstWrite("\n");
            return TRUE;

        case DLL_THREAD_ATTACH:
            tstWrite("thread init: ");
            tstWrite(g_pszName);
            tstWrite("\n");
            return TRUE;

        case DLL_THREAD_DETACH:
            tstWrite("thread term: ");
            tstWrite(g_pszName);
            tstWrite("\n");
            return TRUE;

        default:
            tstWrite("!invalid!: ");
            tstWrite(g_pszName);
            tstWrite("\n");
            return FALSE;
    }
}

#elif K_OS == K_OS_DARWIN
/* later */

#else
# error "port me"
#endif


/**
 * Writes a string with unix lineendings.
 *
 * @param   pszMsg  The string.
 */
void tstWrite(const char *pszMsg)
{
#if K_OS == K_OS_OS2 || K_OS == K_OS_WINDOWS
    /*
     * Line by line.
     */
    ULONG       cbWritten;
    const char *pszNl = strchr(pszMsg, '\n');

    while (pszNl)
    {
        cbWritten = pszNl - pszMsg;

#if K_OS == K_OS_OS2
        if (cbWritten)
            DosWrite((HFILE)2, pszMsg, cbWritten, &cbWritten);
        DosWrite((HFILE)2, "\r\n", 2, &cbWritten);
#else
        if (cbWritten)
            WriteFile((HANDLE)STD_ERROR_HANDLE, pszMsg, cbWritten, &cbWritten, NULL);
        WriteFile((HANDLE)STD_ERROR_HANDLE, "\r\n", 2, &cbWritten, NULL);
#endif

        /* next */
        pszMsg = pszNl + 1;
        pszNl = strchr(pszMsg, '\n');
    }

    /*
     * Remaining incomplete line.
     */
    if (*pszMsg)
    {
        cbWritten = strlen(pszMsg);
#if K_OS == K_OS_OS2
        DosWrite((HFILE)2, pszMsg, cbWritten, &cbWritten);
#else
        WriteFile((HANDLE)STD_ERROR_HANDLE, pszMsg, cbWritten, &cbWritten, NULL);
#endif
    }

#elif K_OS == K_OS_DARWIN
    write(STDERR_FILENO, pszMsg, strlen(pszMsg));

#else
# error "port me"
#endif
}


