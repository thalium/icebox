/* $Id: kLdr-win.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kLdr - The Dynamic Loader, Windows Specifics.
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
#include <Windows.h>

#include <k/kLdr.h>
#include "kLdrInternal.h"


/**
 * The DLL main function.
 *
 * @returns TRUE / FALSE.
 * @param   hDllHandle  The dll handle.
 * @param   dwReason    The reason we're being called.
 * @param   lpReserved  Reserved.
 */
BOOL __stdcall DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            int rc = kldrInit();
            return rc == 0;
        }

        case DLL_PROCESS_DETACH:
            kldrTerm();
            return TRUE;

        case DLL_THREAD_ATTACH:
        {
            //int rc = kLdrDyldThreadAttach();
            //return rc == 0;
            return TRUE;
        }

        case DLL_THREAD_DETACH:
            //kLdrDyldThreadDetach();
            return TRUE;

        default:
            return FALSE;
    }
}

