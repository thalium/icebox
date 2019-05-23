/* $Id: VBoxNetLwfUninstall.cpp $ */
/** @file
 * NetLwfUninstall - VBoxNetLwf uninstaller command line tool
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
#include <stdio.h>

#define VBOX_NETCFG_APP_NAME L"NetLwfUninstall"
#define VBOX_NETLWF_RETRIES 10

static VOID winNetCfgLogger (LPCSTR szString)
{
    printf("%s", szString);
}

static int VBoxNetLwfUninstall()
{
    INetCfg *pnc;
    LPWSTR lpszLockedBy = NULL;
    int r;

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
                hr = VBoxNetCfgWinNetLwfUninstall(pnc);
                if (hr != S_OK)
                {
                    wprintf(L"error uninstalling VBoxNetLwf (0x%x)\n", hr);
                    r = 1;
                }
                else
                {
                    wprintf(L"uninstalled successfully\n");
                    r = 0;
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
                    wprintf(L"Error: write lock is owned by another application (%s), close the application and retry uninstalling\n", lpszLockedBy);
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
    return VBoxNetLwfUninstall();
}
