/** @file
 *
 * VBox host drivers - USB drivers - Filter & driver uninstallation
 *
 * Installation code
 *
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/win/windows.h>
#include <iprt/win/setupapi.h>
#include <newdev.h>

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include <VBox/err.h>
#include <VBox/VBoxDrvCfg-win.h>
#include <stdio.h>


int usblibOsStopService(void);
int usblibOsDeleteService(void);

static DECLCALLBACK(void) vboxUsbLog(VBOXDRVCFG_LOG_SEVERITY enmSeverity, char *pszMsg, void *pvContext)
{
    RT_NOREF1(pvContext);
    switch (enmSeverity)
    {
        case VBOXDRVCFG_LOG_SEVERITY_FLOW:
        case VBOXDRVCFG_LOG_SEVERITY_REGULAR:
            break;
        case VBOXDRVCFG_LOG_SEVERITY_REL:
            printf("%s", pszMsg);
            break;
        default:
            break;
    }
}

static DECLCALLBACK(void) vboxUsbPanic(void *pvPanic)
{
    RT_NOREF1(pvPanic);
#ifndef DEBUG_bird
    AssertFailed();
#endif
}


int __cdecl main(int argc, char **argv)
{
    RT_NOREF2(argc, argv);
    printf("USB uninstallation\n");

    VBoxDrvCfgLoggerSet(vboxUsbLog, NULL);
    VBoxDrvCfgPanicSet(vboxUsbPanic, NULL);

    usblibOsStopService();
    usblibOsDeleteService();

    HRESULT hr = VBoxDrvCfgInfUninstallAllF(L"USB", L"USB\\VID_80EE&PID_CAFE", SUOI_FORCEDELETE);
    if (hr != S_OK)
    {
        printf("SetupUninstallOEMInf failed with hr=0x%x\n", hr);
        return 1;
    }

    printf("USB uninstallation succeeded!\n");

    return 0;
}

/** The support service name. */
#define SERVICE_NAME    "VBoxUSBMon"
/** Win32 Device name. */
#define DEVICE_NAME     "\\\\.\\VBoxUSBMon"
/** NT Device name. */
#define DEVICE_NAME_NT   L"\\Device\\VBoxUSBMon"
/** Win32 Symlink name. */
#define DEVICE_NAME_DOS  L"\\DosDevices\\VBoxUSBMon"

/**
 * Stops a possibly running service.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 */
int usblibOsStopService(void)
{
    /*
     * Assume it didn't exist, so we'll create the service.
     */
    int rc = -1;
    SC_HANDLE   hSMgr = OpenSCManager(NULL, NULL, SERVICE_STOP | SERVICE_QUERY_STATUS);
    DWORD LastError = GetLastError(); NOREF(LastError);
    AssertMsg(hSMgr, ("OpenSCManager(,,delete) failed rc=%d\n", LastError));
    if (hSMgr)
    {
        SC_HANDLE hService = OpenService(hSMgr, SERVICE_NAME, SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (hService)
        {
            /*
             * Stop the service.
             */
            SERVICE_STATUS  Status;
            QueryServiceStatus(hService, &Status);
            if (Status.dwCurrentState == SERVICE_STOPPED)
                rc = 0;
            else if (ControlService(hService, SERVICE_CONTROL_STOP, &Status))
            {
                /*
                 * Wait for finish about 1 minute.
                 * It should be enough for work with driver verifier
                 */
                int iWait = 600;
                while (Status.dwCurrentState == SERVICE_STOP_PENDING && iWait-- > 0)
                {
                    Sleep(100);
                    QueryServiceStatus(hService, &Status);
                }
                if (Status.dwCurrentState == SERVICE_STOPPED)
                    rc = 0;
                else
                   AssertMsgFailed(("Failed to stop service. status=%d\n", Status.dwCurrentState));
            }
            else
            {
                DWORD LastError = GetLastError(); NOREF(LastError);
                AssertMsgFailed(("ControlService failed with LastError=%Rwa. status=%d\n", LastError, Status.dwCurrentState));
            }
            CloseServiceHandle(hService);
        }
        else if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
            rc = 0;
        else
        {
            DWORD LastError = GetLastError(); NOREF(LastError);
            AssertMsgFailed(("OpenService failed LastError=%Rwa\n", LastError));
        }
        CloseServiceHandle(hSMgr);
    }
    return rc;
}


/**
 * Deletes the service.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 */
int usblibOsDeleteService(void)
{
    /*
     * Assume it didn't exist, so we'll create the service.
     */
    int rc = -1;
    SC_HANDLE   hSMgr = OpenSCManager(NULL, NULL, SERVICE_CHANGE_CONFIG);
    DWORD LastError = GetLastError(); NOREF(LastError);
    AssertMsg(hSMgr, ("OpenSCManager(,,delete) failed rc=%d\n", LastError));
    if (hSMgr)
    {
        SC_HANDLE hService = OpenService(hSMgr, SERVICE_NAME, DELETE);
        if (hService)
        {
            /*
             * Delete the service.
             */
            if (DeleteService(hService))
                rc = 0;
            else
            {
                DWORD LastError = GetLastError(); NOREF(LastError);
                AssertMsgFailed(("DeleteService failed LastError=%Rwa\n", LastError));
            }
            CloseServiceHandle(hService);
        }
        else if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
            rc = 0;
        else
        {
            DWORD LastError = GetLastError(); NOREF(LastError);
            AssertMsgFailed(("OpenService failed LastError=%Rwa\n", LastError));
        }
        CloseServiceHandle(hSMgr);
    }
    return rc;
}
