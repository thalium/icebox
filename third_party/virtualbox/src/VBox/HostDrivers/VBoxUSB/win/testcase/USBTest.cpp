/* $Id: USBTest.cpp $ */
/** @file
 * VBox host drivers - USB drivers - Filter & driver installation
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
#include <iprt/win/setupapi.h>
#include <newdev.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include <VBox/err.h>
#include <stdio.h>
#include <VBox/usblib.h>
#include <VBox/VBoxDrvCfg-win.h>

/** Handle to the open device. */
static HANDLE   g_hUSBMonitor = INVALID_HANDLE_VALUE;
/** Flags whether or not we started the service. */
static bool     g_fStartedService = false;

/**
 * Attempts to start the service, creating it if necessary.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 * @param   fRetry  Indicates retry call.
 */
int usbMonStartService(void)
{
    HRESULT hr = VBoxDrvCfgSvcStart(USBMON_SERVICE_NAME_W);
    if (hr != S_OK)
    {
        AssertMsgFailed(("couldn't start service, hr (0x%x)\n", hr));
        return -1;
    }
    return 0;
}

/**
 * Stops a possibly running service.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 */
int usbMonStopService(void)
{
    printf("usbMonStopService\n");
    /*
     * Assume it didn't exist, so we'll create the service.
     */
    int rc = -1;
    SC_HANDLE   hSMgr = OpenSCManager(NULL, NULL, SERVICE_STOP | SERVICE_QUERY_STATUS);
    DWORD LastError = GetLastError(); NOREF(LastError);
    AssertMsg(hSMgr, ("OpenSCManager(,,delete) failed rc=%d\n", LastError));
    if (hSMgr)
    {
        SC_HANDLE hService = OpenServiceW(hSMgr, USBMON_SERVICE_NAME_W, SERVICE_STOP | SERVICE_QUERY_STATUS);
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
                int iWait = 100;
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
 * Release specified USB device to the host.
 *
 * @returns VBox status code
 * @param usVendorId        Vendor id
 * @param usProductId       Product id
 * @param usRevision        Revision
 */
int usbMonReleaseDevice(USHORT usVendorId, USHORT usProductId, USHORT usRevision)
{
    USBSUP_RELEASE release;
    DWORD          cbReturned = 0;

    printf("usbLibReleaseDevice %x %x %x\n", usVendorId, usProductId, usRevision);

    release.usVendorId  = usVendorId;
    release.usProductId = usProductId;
    release.usRevision  = usRevision;

    if (!DeviceIoControl(g_hUSBMonitor, SUPUSBFLT_IOCTL_RELEASE_DEVICE, &release, sizeof(release),  NULL, 0, &cbReturned, NULL))
    {
        AssertMsgFailed(("DeviceIoControl failed with %d\n", GetLastError()));
        return RTErrConvertFromWin32(GetLastError());
    }

    return VINF_SUCCESS;
}


/**
 * Add USB device filter
 *
 * @returns VBox status code.
 * @param   pszVendor       Vendor filter string
 * @param   pszProduct      Product filter string
 * @param   pszRevision     Revision filter string
 * @param   ppID            Pointer to filter id
 */
int usbMonInsertFilter(const char *pszVendor, const char *pszProduct, const char *pszRevision, void **ppID)
{
    USBFILTER           filter;
    USBSUP_FLTADDOUT    flt_add;
    DWORD               cbReturned = 0;

    Assert(g_hUSBMonitor);

    printf("usblibInsertFilter %s %s %s\n", pszVendor, pszProduct, pszRevision);

//    strncpy(filter.szVendor,   pszVendor,   sizeof(filter.szVendor));
//    strncpy(filter.szProduct,  pszProduct,  sizeof(filter.szProduct));
//    strncpy(filter.szRevision, pszRevision, sizeof(filter.szRevision));

    if (!DeviceIoControl(g_hUSBMonitor, SUPUSBFLT_IOCTL_ADD_FILTER, &filter, sizeof(filter), &flt_add, sizeof(flt_add), &cbReturned, NULL))
    {
        AssertMsgFailed(("DeviceIoControl failed with %d\n", GetLastError()));
        return RTErrConvertFromWin32(GetLastError());
    }
    *ppID = (void *)flt_add.uId;
    return VINF_SUCCESS;
}

/**
 * Remove USB device filter
 *
 * @returns VBox status code.
 * @param   aID             Filter id
 */
int usbMonRemoveFilter (void *aID)
{
    uintptr_t   uId;
    DWORD       cbReturned = 0;

    Assert(g_hUSBMonitor);

    printf("usblibRemoveFilter %p\n", aID);

    uId = (uintptr_t)aID;
    if (!DeviceIoControl(g_hUSBMonitor, SUPUSBFLT_IOCTL_REMOVE_FILTER, &uId, sizeof(uId),  NULL, 0,&cbReturned, NULL))
    {
        AssertMsgFailed(("DeviceIoControl failed with %d\n", GetLastError()));
        return RTErrConvertFromWin32(GetLastError());
    }
    return VINF_SUCCESS;
}

/**
 * Initialize the USB monitor
 *
 * @returns VBox status code.
 */
int usbMonitorInit()
{
    int            rc;
    USBSUP_VERSION version = {0};
    DWORD          cbReturned;

    printf("usbproxy: usbLibInit\n");

    g_hUSBMonitor = CreateFile (USBMON_DEVICE_NAME,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL, // no SECURITY_ATTRIBUTES structure
                               OPEN_EXISTING, // No special create flags
                               FILE_ATTRIBUTE_SYSTEM,
                               NULL); // No template file

    if (g_hUSBMonitor == INVALID_HANDLE_VALUE)
    {
        usbMonStartService();

        g_hUSBMonitor = CreateFile (USBMON_DEVICE_NAME,
                                   GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL, // no SECURITY_ATTRIBUTES structure
                                   OPEN_EXISTING, // No special create flags
                                   FILE_ATTRIBUTE_SYSTEM,
                                   NULL); // No template file

        if (g_hUSBMonitor == INVALID_HANDLE_VALUE)
        {
            /* AssertFailed(); */
            printf("usbproxy: Unable to open filter driver!! (rc=%d)\n", GetLastError());
            rc = VERR_FILE_NOT_FOUND;
            goto failure;
        }
    }

    /*
     * Check the version
     */
    cbReturned = 0;
    if (!DeviceIoControl(g_hUSBMonitor, SUPUSBFLT_IOCTL_GET_VERSION, NULL, 0,&version, sizeof(version),  &cbReturned, NULL))
    {
        printf("usbproxy: Unable to query filter version!! (rc=%d)\n", GetLastError());
        rc = VERR_VERSION_MISMATCH;
        goto failure;
    }

    if (   version.u32Major != USBMON_MAJOR_VERSION
#if USBMON_MINOR_VERSION != 0
        || version.u32Minor < USBMON_MINOR_VERSION
#endif
        )
    {
        printf("usbproxy: Filter driver version mismatch!!\n");
        rc = VERR_VERSION_MISMATCH;
        goto failure;
    }

    return VINF_SUCCESS;

failure:
    if (g_hUSBMonitor != INVALID_HANDLE_VALUE)
    {
        CloseHandle(g_hUSBMonitor);
        g_hUSBMonitor = INVALID_HANDLE_VALUE;
    }
    return rc;
}



/**
 * Terminate the USB monitor
 *
 * @returns VBox status code.
 */
int usbMonitorTerm()
{
    if (g_hUSBMonitor != INVALID_HANDLE_VALUE)
    {
        CloseHandle(g_hUSBMonitor);
        g_hUSBMonitor = INVALID_HANDLE_VALUE;
    }
    /*
     * If we started the service we might consider stopping it too.
     *
     * Since this won't work unless the process starting it is the
     * last user we might wanna skip this...
     */
    if (g_fStartedService)
    {
        usbMonStopService();
        g_fStartedService = false;
    }

    return VINF_SUCCESS;
}


int __cdecl main(int argc, char **argv)
{
    int rc;
    RT_NOREF2(argc, argv);

    printf("USB test\n");

    rc = usbMonitorInit();
    AssertRC(rc);

    void *pId1, *pId2;

    usbMonInsertFilter("0529", "0514", "0100", &pId1);
    usbMonInsertFilter("0A16", "2499", "0100", &pId2);

    printf("Waiting to capture device\n");
    getchar();

    printf("Releasing device\n");
    usbMonReleaseDevice(0xA16, 0x2499, 0x100);

    usbMonRemoveFilter(pId1);
    usbMonRemoveFilter(pId2);

    rc = usbMonitorTerm();

    return 0;
}
