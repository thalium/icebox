/* $Id: dllmain-win.cpp 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kProfiler Mark 2 - The Windows DllMain for the profiler DLL.
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
#include "kProfileR3.h"


/**
 * The DLL Main for the kPrf DLL.
 *
 * This is required because we need to initialize the profiler at some point
 * and because we need to know when threads terminate. (We don't care about
 * when threads get created, we simply pick them up when we see them the
 * first time.)
 *
 * @returns Success indicator.
 * @param   hInstDll        The instance handle of the DLL. (i.e. the module handle)
 * @param   fdwReason       The reason why we're here. This is a 'flag' for reasons of
 *                          tradition, it's really a kind of enum.
 * @param   pReserved       Reserved / undocumented something.
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, PVOID pReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            if (kPrfInitialize())
                return FALSE;
            break;

        case DLL_PROCESS_DETACH:
            kPrfTerminate();
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            kPrfTerminateThread();
            break;
    }

    return TRUE;
}

