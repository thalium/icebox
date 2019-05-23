/* $Id: VBoxNetLwfInstall.cpp $ */
/** @file
 * NetLwfInstall - VBoxNetLwf installer command line tool
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#include <VBox/VBoxNetCfg-win.h>
#include <devguid.h>
#include <stdio.h>

#define VBOX_NETCFG_APP_NAME L"NetLwfInstall"
#define VBOX_NETLWF_INF L".\\VBoxNetLwf.inf"
#define VBOX_NETLWF_RETRIES 10


static VOID winNetCfgLogger (LPCSTR szString)
{
    printf("%s", szString);
}

/** Wrapper around GetfullPathNameW that will try an alternative INF location.
 *
 * The default location is the current directory.  If not found there, the
 * alternative location is the executable directory.  If not found there either,
 * the first alternative is present to the caller.
 */
static DWORD MyGetfullPathNameW(LPCWSTR pwszName, size_t cchFull, LPWSTR pwszFull)
{
    LPWSTR pwszFilePart;
    DWORD dwSize = GetFullPathNameW(pwszName, (DWORD)cchFull, pwszFull, &pwszFilePart);
    if (dwSize <= 0)
        return dwSize;

    /* if it doesn't exist, see if the file exists in the same directory as the executable. */
    if (GetFileAttributesW(pwszFull) == INVALID_FILE_ATTRIBUTES)
    {
        WCHAR wsz[512];
        DWORD cch = GetModuleFileNameW(GetModuleHandle(NULL), &wsz[0], sizeof(wsz) / sizeof(wsz[0]));
        if (cch > 0)
        {
            while (cch > 0 && wsz[cch - 1] != '/' && wsz[cch - 1] != '\\' && wsz[cch - 1] != ':')
                cch--;
            unsigned i = 0;
            while (cch < sizeof(wsz) / sizeof(wsz[0]))
            {
                wsz[cch] = pwszFilePart[i++];
                if (!wsz[cch])
                {
                    dwSize = GetFullPathNameW(wsz, (DWORD)cchFull, pwszFull, NULL);
                    if (dwSize > 0 && GetFileAttributesW(pwszFull) != INVALID_FILE_ATTRIBUTES)
                        return dwSize;
                    break;
                }
                cch++;
            }
        }
    }

    /* fallback */
    return GetFullPathNameW(pwszName, (DWORD)cchFull, pwszFull, NULL);
}

static int VBoxNetLwfInstall()
{
    WCHAR Inf[MAX_PATH];
    INetCfg *pnc;
    LPWSTR lpszLockedBy = NULL;
    int r = 1;

    VBoxNetCfgWinSetLogging(winNetCfgLogger);

    HRESULT hr = CoInitialize(NULL);
    if (hr == S_OK)
    {
        int i = 0;
        do
        {
            hr = VBoxNetCfgWinQueryINetCfg(&pnc, TRUE, VBOX_NETCFG_APP_NAME, 10000, &lpszLockedBy);
            if (hr == S_OK)
            {
                DWORD dwSize;
                dwSize = MyGetfullPathNameW(VBOX_NETLWF_INF, sizeof(Inf)/sizeof(Inf[0]), Inf);
                if (dwSize > 0)
                {
                    /** @todo add size check for (sizeof(Inf)/sizeof(Inf[0])) == dwSize (string length in sizeof(Inf[0])) */
                    hr = VBoxNetCfgWinNetLwfInstall(pnc, Inf);
                    if (hr == S_OK)
                    {
                        wprintf(L"installed successfully\n");
                        r = 0;
                    }
                    else
                    {
                        wprintf(L"error installing VBoxNetLwf (0x%x)\n", hr);
                    }
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    wprintf(L"error getting full inf path for VBoxNetLwf.inf (0x%x)\n", hr);
                }


                VBoxNetCfgWinReleaseINetCfg(pnc, TRUE);
                break;
            }
            else if (hr == NETCFG_E_NO_WRITE_LOCK && lpszLockedBy)
            {
                if (i < VBOX_NETLWF_RETRIES && !wcscmp(lpszLockedBy, L"6to4svc.dll"))
                {
                    wprintf(L"6to4svc.dll is holding the lock, retrying %d out of %d\n", ++i, VBOX_NETLWF_RETRIES);
                    CoTaskMemFree(lpszLockedBy);
                }
                else
                {
                    wprintf(L"Error: write lock is owned by another application (%s), close the application and retry installing\n", lpszLockedBy);
                    r = 1;
                    CoTaskMemFree(lpszLockedBy);
                    break;
                }
            }
            else
            {
                wprintf(L"Error getting the INetCfg interface (0x%x)\n", hr);
                r = 1;
                break;
            }
        } while (true);

        CoUninitialize();
    }
    else
    {
        wprintf(L"Error initializing COM (0x%x)\n", hr);
        r = 1;
    }

    VBoxNetCfgWinSetLogging(NULL);

    return r;
}

int __cdecl main(int argc, char **argv)
{
    RT_NOREF2(argc, argv);
    return VBoxNetLwfInstall();
}
