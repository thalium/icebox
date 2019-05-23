/* $Id: RegCleanup.cpp $ */
/** @file
 * delinvalid - remove "InvalidDisplay" key on NT4
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/*
 Purpose:

 Delete the "InvalidDisplay" key which causes the display
 applet to be started on every boot. For some reason this key
 isn't removed after setting the proper resolution and even not when
 doing a driver reinstall. Removing it doesn't seem to do any harm.
 The key is inserted by windows on first reboot after installing
 the VBox video driver using the VirtualBox utility.
 It's not inserted when using the Display applet for installation.
 There seems to be a subtle problem with the VirtualBox util.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
//#define _UNICODE

#include <iprt/win/windows.h>
#include <iprt/win/setupapi.h>
#include <regstr.h>
#include <DEVGUID.h>
#include <stdio.h>

#include "tchar.h"
#include "string.h"


BOOL isNT4(void)
{
    OSVERSIONINFO OSversion;

    OSversion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    ::GetVersionEx(&OSversion);

    switch (OSversion.dwPlatformId)
    {
    case VER_PLATFORM_WIN32s:
    case VER_PLATFORM_WIN32_WINDOWS:
        return FALSE;
    case VER_PLATFORM_WIN32_NT:
        if (OSversion.dwMajorVersion == 4)
            return TRUE;
        else return FALSE;
    default:
        break;
    }
    return FALSE;
}

int main()
{
    int rc = 0;

    /* This program is only for installing drivers on NT4 */
    if (!isNT4())
    {
        printf("This program only runs on NT4\n");
        return -1;
    }

    /* Delete the "InvalidDisplay" key which causes the display
     applet to be started on every boot. For some reason this key
     isn't removed after setting the proper resolution and even not when
     doing a driverreinstall.  */
    RegDeleteKey(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\InvalidDisplay"));
    RegDeleteKey(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\NewDisplay"));

    return rc;
}

