/* $Id: tstVBoxGINA.cpp $ */
/** @file
 * tstVBoxGINA.cpp - Simple testcase for invoking VBoxGINA.dll.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#define UNICODE
#include <iprt/win/windows.h>
#include <stdio.h>

int main()
{
    DWORD dwErr;

    /**
     * Be sure that:
     * - the debug VBoxGINA gets loaded instead of a maybe installed
     *   release version in "C:\Windows\system32".
     */

    HMODULE hMod = LoadLibraryW(L"VBoxGINA.dll");
    if (!hMod)
    {
        dwErr = GetLastError();
        wprintf(L"VBoxGINA.dll not found, error=%ld\n", dwErr);
    }
    else
    {
        wprintf(L"VBoxGINA found\n");

        FARPROC pfnDebug = GetProcAddress(hMod, "VBoxGINADebug");
        if (!pfnDebug)
        {
            dwErr = GetLastError();
            wprintf(L"Could not load VBoxGINADebug, error=%ld\n", dwErr);
        }
        else
        {
            wprintf(L"Calling VBoxGINA ...\n");
            dwErr = pfnDebug();
        }

        FreeLibrary(hMod);
    }

    wprintf(L"Test returned: %ld\n", dwErr);

    return dwErr == ERROR_SUCCESS ? 0 : 1;
}

