/* $Id: VBoxService-win.cpp $ */
/** @file
 * VBoxService - Guest Additions Service Skeleton, Windows Specific Parts.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/system.h> /* For querying OS version. */
#include <VBox/VBoxGuestLib.h>
#include "VBoxServiceInternal.h"

#include <iprt/win/windows.h>
#include <process.h>
#include <aclapi.h>


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static void WINAPI vgsvcWinMain(DWORD argc, LPTSTR *argv);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static DWORD          g_dwWinServiceLastStatus = 0;
SERVICE_STATUS_HANDLE g_hWinServiceStatus = NULL;
/** The semaphore for the dummy Windows service. */
static RTSEMEVENT     g_WindowsEvent = NIL_RTSEMEVENT;

static SERVICE_TABLE_ENTRY const g_aServiceTable[] =
{
    { VBOXSERVICE_NAME, vgsvcWinMain },
    { NULL,             NULL}
};


/**
 * @todo Format code style.
 * @todo Add full unicode support.
 * @todo Add event log capabilities / check return values.
 */
static DWORD vgsvcWinAddAceToObjectsSecurityDescriptor(LPTSTR pszObjName,
                                                             SE_OBJECT_TYPE ObjectType,
                                                             LPTSTR pszTrustee,
                                                             TRUSTEE_FORM TrusteeForm,
                                                             DWORD dwAccessRights,
                                                             ACCESS_MODE AccessMode,
                                                             DWORD dwInheritance)
{
    DWORD dwRes = 0;
    PACL pOldDACL = NULL, pNewDACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESS ea;

    if (NULL == pszObjName)
        return ERROR_INVALID_PARAMETER;

    /* Get a pointer to the existing DACL. */
    dwRes = GetNamedSecurityInfo(pszObjName, ObjectType,
                                 DACL_SECURITY_INFORMATION,
                                 NULL, NULL, &pOldDACL, NULL, &pSD);
    if (ERROR_SUCCESS != dwRes)
    {
        if (dwRes == ERROR_FILE_NOT_FOUND)
            VGSvcError("AddAceToObjectsSecurityDescriptor: Object not found/installed: %s\n", pszObjName);
        else
            VGSvcError("AddAceToObjectsSecurityDescriptor: GetNamedSecurityInfo: Error %u\n", dwRes);
        goto l_Cleanup;
    }

    /* Initialize an EXPLICIT_ACCESS structure for the new ACE. */
    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
    ea.grfAccessPermissions = dwAccessRights;
    ea.grfAccessMode = AccessMode;
    ea.grfInheritance= dwInheritance;
    ea.Trustee.TrusteeForm = TrusteeForm;
    ea.Trustee.ptstrName = pszTrustee;

    /* Create a new ACL that merges the new ACE into the existing DACL. */
    dwRes = SetEntriesInAcl(1, &ea, pOldDACL, &pNewDACL);
    if (ERROR_SUCCESS != dwRes)
    {
        VGSvcError("AddAceToObjectsSecurityDescriptor: SetEntriesInAcl: Error %u\n", dwRes);
        goto l_Cleanup;
    }

    /* Attach the new ACL as the object's DACL. */
    dwRes = SetNamedSecurityInfo(pszObjName, ObjectType,
                                 DACL_SECURITY_INFORMATION,
                                 NULL, NULL, pNewDACL, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        VGSvcError("AddAceToObjectsSecurityDescriptor: SetNamedSecurityInfo: Error %u\n", dwRes);
        goto l_Cleanup;
    }

    /** @todo get rid of that spaghetti jump ... */
l_Cleanup:

    if(pSD != NULL)
        LocalFree((HLOCAL) pSD);
    if(pNewDACL != NULL)
        LocalFree((HLOCAL) pNewDACL);

    return dwRes;
}


/** Reports our current status to the SCM. */
static BOOL vgsvcWinSetStatus(DWORD dwStatus, DWORD dwCheckPoint)
{
    if (g_hWinServiceStatus == NULL) /* Program could be in testing mode, so no service environment available. */
        return FALSE;

    VGSvcVerbose(2, "Setting service status to: %ld\n", dwStatus);
    g_dwWinServiceLastStatus = dwStatus;

    SERVICE_STATUS ss;
    RT_ZERO(ss);

    ss.dwServiceType              = SERVICE_WIN32_OWN_PROCESS;
    ss.dwCurrentState             = dwStatus;
    /* Don't accept controls when in start pending state. */
    if (ss.dwCurrentState != SERVICE_START_PENDING)
    {
        ss.dwControlsAccepted     = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
#ifndef TARGET_NT4
        /* Don't use SERVICE_ACCEPT_SESSIONCHANGE on Windows 2000.
         * This makes SCM angry. */
        char szOSVersion[32];
        int rc = RTSystemQueryOSInfo(RTSYSOSINFO_RELEASE,
                                     szOSVersion, sizeof(szOSVersion));
        if (RT_SUCCESS(rc))
        {
            if (RTStrVersionCompare(szOSVersion, "5.1") >= 0)
                ss.dwControlsAccepted |= SERVICE_ACCEPT_SESSIONCHANGE;
        }
        else
            VGSvcError("Error determining OS version, rc=%Rrc\n", rc);
#endif
    }

    ss.dwWin32ExitCode            = NO_ERROR;
    ss.dwServiceSpecificExitCode  = 0; /* Not used */
    ss.dwCheckPoint               = dwCheckPoint;
    ss.dwWaitHint                 = 3000;

    BOOL fStatusSet = SetServiceStatus(g_hWinServiceStatus, &ss);
    if (!fStatusSet)
        VGSvcError("Error reporting service status=%ld (controls=%x, checkpoint=%ld) to SCM: %ld\n",
                         dwStatus, ss.dwControlsAccepted, dwCheckPoint, GetLastError());
    return fStatusSet;
}


/**
 * Reports SERVICE_STOP_PENDING to SCM.
 *
 * @param   uCheckPoint         Some number.
 */
void VGSvcWinSetStopPendingStatus(uint32_t uCheckPoint)
{
    vgsvcWinSetStatus(SERVICE_STOP_PENDING, uCheckPoint);
}


static RTEXITCODE vgsvcWinSetDesc(SC_HANDLE hService)
{
#ifndef TARGET_NT4
    /* On W2K+ there's ChangeServiceConfig2() which lets us set some fields
       like a longer service description. */
    /** @todo On Vista+ SERVICE_DESCRIPTION also supports localized strings! */
    SERVICE_DESCRIPTION desc;
    desc.lpDescription = VBOXSERVICE_DESCRIPTION;
    if (!ChangeServiceConfig2(hService,
                              SERVICE_CONFIG_DESCRIPTION, /* Service info level */
                              &desc))
    {
        VGSvcError("Cannot set the service description! Error: %ld\n", GetLastError());
        return RTEXITCODE_FAILURE;
    }
#else
    RT_NOREF(hService);
#endif
    return RTEXITCODE_SUCCESS;
}


/**
 * Installs the service.
 */
RTEXITCODE VGSvcWinInstall(void)
{
    VGSvcVerbose(1, "Installing service ...\n");

    TCHAR imagePath[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, imagePath, sizeof(imagePath));

    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL)
    {
        VGSvcError("Could not open SCM! Error: %ld\n", GetLastError());
        return RTEXITCODE_FAILURE;
    }

    RTEXITCODE rc       = RTEXITCODE_SUCCESS;
    SC_HANDLE  hService = CreateService(hSCManager,
                                        VBOXSERVICE_NAME, VBOXSERVICE_FRIENDLY_NAME,
                                        SERVICE_ALL_ACCESS,
                                        SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                        SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
                                        imagePath, NULL, NULL, NULL, NULL, NULL);
    if (hService != NULL)
        VGSvcVerbose(0, "Service successfully installed!\n");
    else
    {
        DWORD dwErr = GetLastError();
        switch (dwErr)
        {
            case ERROR_SERVICE_EXISTS:
                VGSvcVerbose(1, "Service already exists, just updating the service config.\n");
                hService = OpenService(hSCManager, VBOXSERVICE_NAME, SERVICE_ALL_ACCESS);
                if (hService)
                {
                    if (ChangeServiceConfig (hService,
                                             SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                             SERVICE_DEMAND_START,
                                             SERVICE_ERROR_NORMAL,
                                             imagePath,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             VBOXSERVICE_FRIENDLY_NAME))
                        VGSvcVerbose(1, "The service config has been successfully updated.\n");
                    else
                        rc = VGSvcError("Could not change service config! Error: %ld\n", GetLastError());
                }
                else
                    rc = VGSvcError("Could not open service! Error: %ld\n", GetLastError());
                break;

            default:
                rc = VGSvcError("Could not create service! Error: %ld\n", dwErr);
                break;
        }
    }

    if (rc == RTEXITCODE_SUCCESS)
        rc = vgsvcWinSetDesc(hService);

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return rc;
}

/**
 * Uninstalls the service.
 */
RTEXITCODE VGSvcWinUninstall(void)
{
    VGSvcVerbose(1, "Uninstalling service ...\n");

    SC_HANDLE hSCManager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL)
    {
        VGSvcError("Could not open SCM! Error: %d\n", GetLastError());
        return RTEXITCODE_FAILURE;
    }

    RTEXITCODE rcExit;
    SC_HANDLE  hService = OpenService(hSCManager, VBOXSERVICE_NAME, SERVICE_ALL_ACCESS );
    if (hService != NULL)
    {
        if (DeleteService(hService))
        {
            /*
             * ???
             */
            HKEY hKey = NULL;
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             "SYSTEM\\CurrentControlSet\\Services\\EventLog\\System",
                             0,
                             KEY_ALL_ACCESS,
                             &hKey)
                == ERROR_SUCCESS)
            {
                RegDeleteKey(hKey, VBOXSERVICE_NAME);
                RegCloseKey(hKey);
            }

            VGSvcVerbose(0, "Service successfully uninstalled!\n");
            rcExit = RTEXITCODE_SUCCESS;
        }
        else
            rcExit = VGSvcError("Could not remove service! Error: %d\n", GetLastError());
        CloseServiceHandle(hService);
    }
    else
        rcExit = VGSvcError("Could not open service! Error: %d\n", GetLastError());
    CloseServiceHandle(hSCManager);

    return rcExit;
}


static int vgsvcWinStart(void)
{
    int rc = VINF_SUCCESS;

#ifndef TARGET_NT4
    /* Create a well-known SID for the "Builtin Users" group. */
    PSID pBuiltinUsersSID = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_LOCAL_SID_AUTHORITY;

    if (!AllocateAndInitializeSid(&SIDAuthWorld, 1,
                                  SECURITY_LOCAL_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &pBuiltinUsersSID))
    {
        rc = RTErrConvertFromWin32(GetLastError());
    }
    else
    {
        DWORD dwRes = vgsvcWinAddAceToObjectsSecurityDescriptor(TEXT("\\\\.\\VBoxMiniRdrDN"),
                                                                      SE_FILE_OBJECT,
                                                                      (LPTSTR)pBuiltinUsersSID,
                                                                      TRUSTEE_IS_SID,
                                                                      FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                                                                      SET_ACCESS,
                                                                      NO_INHERITANCE);
        if (dwRes != ERROR_SUCCESS)
        {
            if (dwRes == ERROR_FILE_NOT_FOUND)
            {
                /* If we don't find our "VBoxMiniRdrDN" (for Shared Folders) object above,
                   don't report an error; it just might be not installed. Otherwise this
                  would cause the SCM to hang on starting up the service. */
                rc = VINF_SUCCESS;
            }
            else
                rc = RTErrConvertFromWin32(dwRes);
        }
    }
#endif

    if (RT_SUCCESS(rc))
    {
        vgsvcWinSetStatus(SERVICE_START_PENDING, 0);

        rc = VGSvcStartServices();
        if (RT_SUCCESS(rc))
        {
            vgsvcWinSetStatus(SERVICE_RUNNING, 0);
            VGSvcMainWait();
        }
        else
        {
            vgsvcWinSetStatus(SERVICE_STOPPED, 0);
#if 0 /** @todo r=bird: Enable this if SERVICE_CONTROL_STOP isn't triggered automatically */
            VGSvcStopServices();
#endif
        }
    }
    else
        vgsvcWinSetStatus(SERVICE_STOPPED, 0);

    if (RT_FAILURE(rc))
        VGSvcError("Service failed to start with rc=%Rrc!\n", rc);

    return rc;
}


/**
 * Call StartServiceCtrlDispatcher.
 *
 * The main() thread invokes this when not started in foreground mode.  It
 * won't return till the service is being shutdown (unless start up fails).
 *
 * @returns RTEXITCODE_SUCCESS on normal return after service shutdown.
 *          Something else on failure, error will have been reported.
 */
RTEXITCODE VGSvcWinEnterCtrlDispatcher(void)
{
    if (!StartServiceCtrlDispatcher(&g_aServiceTable[0]))
        return VGSvcError("StartServiceCtrlDispatcher: %u. Please start %s with option -f (foreground)!\n",
                                GetLastError(), g_pszProgName);
    return RTEXITCODE_SUCCESS;
}


#ifndef TARGET_NT4
static const char* vgsvcWTSStateToString(DWORD dwEvent)
{
    switch (dwEvent)
    {
        case WTS_CONSOLE_CONNECT:
            return "A session was connected to the console terminal";

        case WTS_CONSOLE_DISCONNECT:
            return "A session was disconnected from the console terminal";

        case WTS_REMOTE_CONNECT:
            return "A session connected to the remote terminal";

        case WTS_REMOTE_DISCONNECT:
            return "A session was disconnected from the remote terminal";

        case WTS_SESSION_LOGON:
            return "A user has logged on to a session";

        case WTS_SESSION_LOGOFF:
            return "A user has logged off the session";

        case WTS_SESSION_LOCK:
            return "A session has been locked";

        case WTS_SESSION_UNLOCK:
            return "A session has been unlocked";

        case WTS_SESSION_REMOTE_CONTROL:
            return "A session has changed its remote controlled status";
#if 0
        case WTS_SESSION_CREATE:
            return "A session has been created";

        case WTS_SESSION_TERMINATE:
            return "The session has been terminated";
#endif
        default:
            break;
    }

    return "Uknonwn state";
}
#endif /* !TARGET_NT4 */


#ifdef TARGET_NT4
static VOID WINAPI vgsvcWinCtrlHandler(DWORD dwControl)
#else
static DWORD WINAPI vgsvcWinCtrlHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
#endif
{
#ifdef TARGET_NT4
    VGSvcVerbose(2, "Control handler: Control=%#x\n", dwControl);
#else
    RT_NOREF1(lpContext);
    VGSvcVerbose(2, "Control handler: Control=%#x, EventType=%#x\n", dwControl, dwEventType);
#endif

    DWORD rcRet = NO_ERROR;
    switch (dwControl)
    {
        case SERVICE_CONTROL_INTERROGATE:
            vgsvcWinSetStatus(g_dwWinServiceLastStatus, 0);
            break;

        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
        {
            vgsvcWinSetStatus(SERVICE_STOP_PENDING, 0);

            int rc2 = VGSvcStopServices();
            if (RT_FAILURE(rc2))
                rcRet = ERROR_GEN_FAILURE;
            else
            {
                rc2 = VGSvcReportStatus(VBoxGuestFacilityStatus_Terminated);
                AssertRC(rc2);
            }

            vgsvcWinSetStatus(SERVICE_STOPPED, 0);
            break;
        }

# ifndef TARGET_NT4
        case SERVICE_CONTROL_SESSIONCHANGE: /* Only Windows 2000 and up. */
        {
            AssertPtr(lpEventData);
            PWTSSESSION_NOTIFICATION pNotify = (PWTSSESSION_NOTIFICATION)lpEventData;
            Assert(pNotify->cbSize == sizeof(WTSSESSION_NOTIFICATION));

            VGSvcVerbose(1, "Control handler: %s (Session=%ld, Event=%#x)\n",
                               vgsvcWTSStateToString(dwEventType),
                               pNotify->dwSessionId, dwEventType);

            /* Handle all events, regardless of dwEventType. */
            int rc2 = VGSvcVMInfoSignal();
            AssertRC(rc2);
            break;
        }
# endif /* !TARGET_NT4 */

        default:
            VGSvcVerbose(1, "Control handler: Function not implemented: %#x\n", dwControl);
            rcRet = ERROR_CALL_NOT_IMPLEMENTED;
            break;
    }

#ifndef TARGET_NT4
    return rcRet;
#endif
}


static void WINAPI vgsvcWinMain(DWORD argc, LPTSTR *argv)
{
    RT_NOREF2(argc, argv);
    VGSvcVerbose(2, "Registering service control handler ...\n");
#ifdef TARGET_NT4
    g_hWinServiceStatus = RegisterServiceCtrlHandler(VBOXSERVICE_NAME, vgsvcWinCtrlHandler);
#else
    g_hWinServiceStatus = RegisterServiceCtrlHandlerEx(VBOXSERVICE_NAME, vgsvcWinCtrlHandler, NULL);
#endif
    if (g_hWinServiceStatus != NULL)
    {
        VGSvcVerbose(2, "Service control handler registered.\n");
        vgsvcWinStart();
    }
    else
    {
        DWORD dwErr = GetLastError();
        switch (dwErr)
        {
            case ERROR_INVALID_NAME:
                VGSvcError("Invalid service name!\n");
                break;
            case ERROR_SERVICE_DOES_NOT_EXIST:
                VGSvcError("Service does not exist!\n");
                break;
            default:
                VGSvcError("Could not register service control handle! Error: %ld\n", dwErr);
                break;
        }
    }
}

