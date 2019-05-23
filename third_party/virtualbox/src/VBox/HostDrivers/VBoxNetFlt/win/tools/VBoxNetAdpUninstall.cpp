/* $Id: VBoxNetAdpUninstall.cpp $ */
/** @file
 * NetAdpUninstall - VBoxNetAdp uninstaller command line tool
 */

/*
 * Copyright (C) 2009-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <VBox/VBoxNetCfg-win.h>
#include <VBox/VBoxDrvCfg-win.h>
#include <stdio.h>

#include <devguid.h>

#ifdef NDIS60
#define VBOX_NETADP_HWID L"sun_VBoxNetAdp6"
#else /* !NDIS60 */
#define VBOX_NETADP_HWID L"sun_VBoxNetAdp"
#endif /* !NDIS60 */

static VOID winNetCfgLogger (LPCSTR szString)
{
    printf("%s", szString);
}

static int VBoxNetAdpUninstall()
{
    int r = 1;
    VBoxNetCfgWinSetLogging(winNetCfgLogger);

    printf("uninstalling all Host-Only interfaces..\n");

    HRESULT hr = CoInitialize(NULL);
    if (hr == S_OK)
    {
        hr = VBoxNetCfgWinRemoveAllNetDevicesOfId(VBOX_NETADP_HWID);
        if (hr == S_OK)
        {
            hr = VBoxDrvCfgInfUninstallAllSetupDi(&GUID_DEVCLASS_NET, L"Net", VBOX_NETADP_HWID, 0/* could be SUOI_FORCEDELETE */);
            if (hr == S_OK)
            {
                printf("uninstalled successfully\n");
            }
            else
            {
                printf("uninstalled successfully, but failed to remove infs\n");
            }
            r = 0;
        }
        else
        {
            printf("uninstall failed, hr = 0x%x\n", hr);
        }

        CoUninitialize();
    }
    else
    {
        wprintf(L"Error initializing COM (0x%x)\n", hr);
    }

    VBoxNetCfgWinSetLogging(NULL);

    return r;
}

int __cdecl main(int argc, char **argv)
{
    RT_NOREF2(argc, argv);
    return VBoxNetAdpUninstall();
}
