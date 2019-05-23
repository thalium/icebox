/* $Id: VBoxGuestInst.cpp $ */
/** @file
 * Small tool to (un)install the VBoxGuest device driver.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/win/windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <VBox/VBoxGuest.h> /* for VBOXGUEST_SERVICE_NAME */


//#define TESTMODE



static int installDriver(void)
{
    /*
     * Assume it didn't exist, so we'll create the service.
     */
    SC_HANDLE   hSMgrCreate = OpenSCManager(NULL, NULL, SERVICE_CHANGE_CONFIG);
    if (!hSMgrCreate)
    {
        printf("OpenSCManager(,,create) failed rc=%d\n", GetLastError());
        return -1;
    }

    char szDriver[MAX_PATH];
    GetSystemDirectory(szDriver, sizeof(szDriver));
    strcat(szDriver, "\\drivers\\VBoxGuestNT.sys");

    SC_HANDLE hService = CreateService(hSMgrCreate,
                                       VBOXGUEST_SERVICE_NAME,
                                       "VBoxGuest Support Driver",
                                       SERVICE_QUERY_STATUS,
                                       SERVICE_KERNEL_DRIVER,
                                       SERVICE_BOOT_START,
                                       SERVICE_ERROR_NORMAL,
                                       szDriver,
                                       "System",
                                       NULL, NULL, NULL, NULL);
    if (!hService)
        printf("CreateService failed! lasterr=%d\n", GetLastError());
    else
        CloseServiceHandle(hService);
    CloseServiceHandle(hSMgrCreate);
    return hService ? 0 : -1;
}

static int uninstallDriver(void)
{
    int rc = -1;
    SC_HANDLE   hSMgr = OpenSCManager(NULL, NULL, SERVICE_CHANGE_CONFIG);
    if (!hSMgr)
    {
        printf("OpenSCManager(,,delete) failed rc=%d\n", GetLastError());
        return -1;
    }
    SC_HANDLE hService = OpenService(hSMgr, VBOXGUEST_SERVICE_NAME, DELETE);
    if (hService)
    {
        /*
         * Delete the service.
         */
        if (DeleteService(hService))
            rc = 0;
        else
            printf("DeleteService failed lasterr=%d\n", GetLastError());
        CloseServiceHandle(hService);
    }
    else if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
        rc = 0;
    else
        printf("OpenService failed lasterr=%d\n", GetLastError());
    CloseServiceHandle(hSMgr);
    return rc;
}

#ifdef TESTMODE

static HANDLE openDriver(void)
{
    HANDLE hDevice;

    hDevice = CreateFile(VBOXGUEST_DEVICE_NAME, // Win2k+: VBOXGUEST_DEVICE_NAME_GLOBAL
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        printf("CreateFile did not work. GetLastError() 0x%x\n", GetLastError());
    }
    return hDevice;
}

static int closeDriver(HANDLE hDevice)
{
    CloseHandle(hDevice);
    return 0;
}

static int performTest(void)
{
    int rc = 0;

    HANDLE hDevice = openDriver();

    if (hDevice != INVALID_HANDLE_VALUE)
        closeDriver(hDevice);
    else
        printf("openDriver failed!\n");

    return rc;
}

#endif /* TESTMODE */

static int usage(char *programName)
{
    printf("error, syntax: %s [install|uninstall]\n", programName);
    return 1;
}

int main(int argc, char **argv)
{
    bool installMode;
#ifdef TESTMODE
    bool testMode = false;
#endif

    if (argc != 2)
        return usage(argv[0]);

    if (strcmp(argv[1], "install") == 0)
        installMode = true;
    else if (strcmp(argv[1], "uninstall") == 0)
        installMode = false;
#ifdef TESTMODE
    else if (strcmp(argv[1], "test") == 0)
        testMode = true;
#endif
    else
        return usage(argv[0]);


    int rc;
#ifdef TESTMODE
    if (testMode)
        rc = performTest();
    else
#endif
    if (installMode)
        rc = installDriver();
    else
        rc = uninstallDriver();

    if (rc == 0)
        printf("operation completed successfully!\n");
    else
        printf("error: operation failed with status code %d\n", rc);

    return rc;
}

