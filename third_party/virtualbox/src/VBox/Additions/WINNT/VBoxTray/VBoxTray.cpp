/* $Id: VBoxTray.cpp $ */
/** @file
 * VBoxTray - Guest Additions Tray Application
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <package-generated.h>
#include "product-generated.h"

#include "VBoxTray.h"
#include "VBoxTrayMsg.h"
#include "VBoxHelpers.h"
#include "VBoxSeamless.h"
#include "VBoxClipboard.h"
#include "VBoxDisplay.h"
#include "VBoxVRDP.h"
#include "VBoxHostVersion.h"
#include "VBoxSharedFolders.h"
#ifdef VBOX_WITH_DRAG_AND_DROP
# include "VBoxDnD.h"
#endif
#include "VBoxIPC.h"
#include "VBoxLA.h"
#include <VBoxHook.h>
#include "resource.h"

#include <sddl.h>

#include <iprt/asm.h>
#include <iprt/buildconfig.h>
#include <iprt/ldr.h>
#include <iprt/path.h>
#include <iprt/process.h>
#include <iprt/system.h>
#include <iprt/time.h>

#ifdef DEBUG
# define LOG_ENABLED
# define LOG_GROUP LOG_GROUP_DEFAULT
#endif
#include <VBox/log.h>

/* Default desktop state tracking */
#include <Wtsapi32.h>

/*
 * St (session [state] tracking) functionality API
 *
 * !!!NOTE: this API is NOT thread-safe!!!
 * it is supposed to be called & used from within the window message handler thread
 * of the window passed to vboxStInit */
static int vboxStInit(HWND hWnd);
static void vboxStTerm(void);
/* @returns true on "IsActiveConsole" state change */
static BOOL vboxStHandleEvent(WPARAM EventID);
static BOOL vboxStIsActiveConsole();
static BOOL vboxStCheckTimer(WPARAM wEvent);

/*
 * Dt (desktop [state] tracking) functionality API
 *
 * !!!NOTE: this API is NOT thread-safe!!!
 * */
static int vboxDtInit();
static void vboxDtTerm();
/* @returns true on "IsInputDesktop" state change */
static BOOL vboxDtHandleEvent();
/* @returns true iff the application (VBoxTray) desktop is input */
static BOOL vboxDtIsInputDesktop();
static HANDLE vboxDtGetNotifyEvent();
static BOOL vboxDtCheckTimer(WPARAM wParam);

/* caps API */
#define VBOXCAPS_ENTRY_IDX_SEAMLESS  0
#define VBOXCAPS_ENTRY_IDX_GRAPHICS  1
#define VBOXCAPS_ENTRY_IDX_COUNT     2

typedef enum VBOXCAPS_ENTRY_FUNCSTATE
{
    /* the cap is unsupported */
    VBOXCAPS_ENTRY_FUNCSTATE_UNSUPPORTED = 0,
    /* the cap is supported */
    VBOXCAPS_ENTRY_FUNCSTATE_SUPPORTED,
    /* the cap functionality is started, it can be disabled however if its AcState is not ACQUIRED */
    VBOXCAPS_ENTRY_FUNCSTATE_STARTED,
} VBOXCAPS_ENTRY_FUNCSTATE;


static void VBoxCapsEntryFuncStateSet(uint32_t iCup, VBOXCAPS_ENTRY_FUNCSTATE enmFuncState);
static int VBoxCapsInit();
static int VBoxCapsReleaseAll();
static void VBoxCapsTerm();
static BOOL VBoxCapsEntryIsAcquired(uint32_t iCap);
static BOOL VBoxCapsEntryIsEnabled(uint32_t iCap);
static BOOL VBoxCapsCheckTimer(WPARAM wParam);
static int VBoxCapsEntryRelease(uint32_t iCap);
static int VBoxCapsEntryAcquire(uint32_t iCap);
static int VBoxCapsAcquireAllSupported();

/* console-related caps API */
static BOOL VBoxConsoleIsAllowed();
static void VBoxConsoleEnable(BOOL fEnable);
static void VBoxConsoleCapSetSupported(uint32_t iCap, BOOL fSupported);

static void VBoxGrapicsSetSupported(BOOL fSupported);


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int vboxTrayCreateTrayIcon(void);
static LRESULT CALLBACK vboxToolWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* Global message handler prototypes. */
static int vboxTrayGlMsgTaskbarCreated(WPARAM lParam, LPARAM wParam);
/*static int vboxTrayGlMsgShowBalloonMsg(WPARAM lParam, LPARAM wParam);*/

static int VBoxAcquireGuestCaps(uint32_t fOr, uint32_t fNot, bool fCfg);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
HANDLE                g_hStopSem;
HANDLE                g_hSeamlessWtNotifyEvent = 0;
HANDLE                g_hSeamlessKmNotifyEvent = 0;
HINSTANCE             g_hInstance;
HWND                  g_hwndToolWindow;
NOTIFYICONDATA        g_NotifyIconData;
DWORD                 g_dwMajorVersion;

uint32_t              g_fGuestDisplaysChanged = 0;

static PRTLOGGER      g_pLoggerRelease = NULL;
static uint32_t       g_cHistory = 10;                   /* Enable log rotation, 10 files. */
static uint32_t       g_uHistoryFileTime = RT_SEC_1DAY;  /* Max 1 day per file. */
static uint64_t       g_uHistoryFileSize = 100 * _1M;    /* Max 100MB per file. */

/**
 * The details of the services that has been compiled in.
 */
static VBOXSERVICEINFO g_aServices[] =
{
    { &g_SvcDescDisplay,        NIL_RTTHREAD, NULL, false, false, false, false, true },
    { &g_SvcDescClipboard,      NIL_RTTHREAD, NULL, false, false, false, false, true },
    { &g_SvcDescSeamless,       NIL_RTTHREAD, NULL, false, false, false, false, true },
    { &g_SvcDescVRDP,           NIL_RTTHREAD, NULL, false, false, false, false, true },
    { &g_SvcDescIPC,            NIL_RTTHREAD, NULL, false, false, false, false, true },
    { &g_SvcDescLA,             NIL_RTTHREAD, NULL, false, false, false, false, true },
#ifdef VBOX_WITH_DRAG_AND_DROP
    { &g_SvcDescDnD,            NIL_RTTHREAD, NULL, false, false, false, false, true }
#endif
};

/* The global message table. */
static VBOXGLOBALMESSAGE g_vboxGlobalMessageTable[] =
{
    /* Windows specific stuff. */
    {
        "TaskbarCreated",
        vboxTrayGlMsgTaskbarCreated
    },

    /* VBoxTray specific stuff. */
    /** @todo Add new messages here! */

    {
        NULL
    }
};

/**
 * Gets called whenever the Windows main taskbar
 * get (re-)created. Nice to install our tray icon.
 *
 * @return  IPRT status code.
 * @param   wParam
 * @param   lParam
 */
static int vboxTrayGlMsgTaskbarCreated(WPARAM wParam, LPARAM lParam)
{
    RT_NOREF(wParam, lParam);
    return vboxTrayCreateTrayIcon();
}

static int vboxTrayCreateTrayIcon(void)
{
    HICON hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_VIRTUALBOX));
    if (hIcon == NULL)
    {
        DWORD dwErr = GetLastError();
        LogFunc(("Could not load tray icon, error %08X\n", dwErr));
        return RTErrConvertFromWin32(dwErr);
    }

    /* Prepare the system tray icon. */
    RT_ZERO(g_NotifyIconData);
    g_NotifyIconData.cbSize           = NOTIFYICONDATA_V1_SIZE; // sizeof(NOTIFYICONDATA);
    g_NotifyIconData.hWnd             = g_hwndToolWindow;
    g_NotifyIconData.uID              = ID_TRAYICON;
    g_NotifyIconData.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_NotifyIconData.uCallbackMessage = WM_VBOXTRAY_TRAY_ICON;
    g_NotifyIconData.hIcon            = hIcon;

    sprintf(g_NotifyIconData.szTip, "%s Guest Additions %d.%d.%dr%d",
            VBOX_PRODUCT, VBOX_VERSION_MAJOR, VBOX_VERSION_MINOR, VBOX_VERSION_BUILD, VBOX_SVN_REV);

    int rc = VINF_SUCCESS;
    if (!Shell_NotifyIcon(NIM_ADD, &g_NotifyIconData))
    {
        DWORD dwErr = GetLastError();
        LogFunc(("Could not create tray icon, error=%ld\n", dwErr));
        rc = RTErrConvertFromWin32(dwErr);
        RT_ZERO(g_NotifyIconData);
    }

    if (hIcon)
        DestroyIcon(hIcon);
    return rc;
}

static void vboxTrayRemoveTrayIcon(void)
{
    if (g_NotifyIconData.cbSize > 0)
    {
        /* Remove the system tray icon and refresh system tray. */
        Shell_NotifyIcon(NIM_DELETE, &g_NotifyIconData);
        HWND hTrayWnd = FindWindow("Shell_TrayWnd", NULL); /* We assume we only have one tray atm. */
        if (hTrayWnd)
        {
            HWND hTrayNotifyWnd = FindWindowEx(hTrayWnd, 0, "TrayNotifyWnd", NULL);
            if (hTrayNotifyWnd)
                SendMessage(hTrayNotifyWnd, WM_PAINT, 0, NULL);
        }
        RT_ZERO(g_NotifyIconData);
    }
}

/**
 * The service thread.
 *
 * @returns Whatever the worker function returns.
 * @param   ThreadSelf      My thread handle.
 * @param   pvUser          The service index.
 */
static DECLCALLBACK(int) vboxTrayServiceThread(RTTHREAD ThreadSelf, void *pvUser)
{
    PVBOXSERVICEINFO pSvc = (PVBOXSERVICEINFO)pvUser;
    AssertPtr(pSvc);

#ifndef RT_OS_WINDOWS
    /*
     * Block all signals for this thread. Only the main thread will handle signals.
     */
    sigset_t signalMask;
    sigfillset(&signalMask);
    pthread_sigmask(SIG_BLOCK, &signalMask, NULL);
#endif

    int rc = pSvc->pDesc->pfnWorker(pSvc->pInstance, &pSvc->fShutdown);
    ASMAtomicXchgBool(&pSvc->fShutdown, true);
    RTThreadUserSignal(ThreadSelf);

    LogFunc(("Worker for '%s' ended with %Rrc\n", pSvc->pDesc->pszName, rc));
    return rc;
}

static int vboxTrayServicesStart(PVBOXSERVICEENV pEnv)
{
    AssertPtrReturn(pEnv, VERR_INVALID_POINTER);

    LogRel(("Starting services ...\n"));

    int rc = VINF_SUCCESS;

    for (unsigned i = 0; i < RT_ELEMENTS(g_aServices); i++)
    {
        PVBOXSERVICEINFO pSvc = &g_aServices[i];
        LogRel(("Starting service '%s' ...\n", pSvc->pDesc->pszName));

        pSvc->hThread   = NIL_RTTHREAD;
        pSvc->pInstance = NULL;
        pSvc->fStarted  = false;
        pSvc->fShutdown = false;

        int rc2 = VINF_SUCCESS;

        if (pSvc->pDesc->pfnInit)
            rc2 = pSvc->pDesc->pfnInit(pEnv, &pSvc->pInstance);

        if (RT_FAILURE(rc2))
        {
            LogRel(("Failed to initialize service '%s', rc=%Rrc\n", pSvc->pDesc->pszName, rc2));
            if (rc2 == VERR_NOT_SUPPORTED)
            {
                LogRel(("Service '%s' is not supported on this system\n", pSvc->pDesc->pszName));
                rc2 = VINF_SUCCESS;
            }
            /* Keep going. */
        }
        else
        {
            if (pSvc->pDesc->pfnWorker)
            {
                rc2 = RTThreadCreate(&pSvc->hThread, vboxTrayServiceThread, pSvc /* pvUser */,
                                     0 /* Default stack size */, RTTHREADTYPE_DEFAULT, RTTHREADFLAGS_WAITABLE, pSvc->pDesc->pszName);
                if (RT_SUCCESS(rc2))
                {
                    pSvc->fStarted = true;

                    RTThreadUserWait(pSvc->hThread, 30 * 1000 /* Timeout in ms */);
                    if (pSvc->fShutdown)
                    {
                        LogRel(("Service '%s' failed to start!\n", pSvc->pDesc->pszName));
                        rc = VERR_GENERAL_FAILURE;
                    }
                    else
                        LogRel(("Service '%s' started\n", pSvc->pDesc->pszName));
                }
                else
                {
                    LogRel(("Failed to start thread for service '%s': %Rrc\n", rc2));
                    if (pSvc->pDesc->pfnDestroy)
                        pSvc->pDesc->pfnDestroy(pSvc->pInstance);
                }
            }
        }

        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    if (RT_SUCCESS(rc))
        LogRel(("All services started\n"));
    else
        LogRel(("Services started, but some with errors\n"));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

static int vboxTrayServicesStop(VBOXSERVICEENV *pEnv)
{
    AssertPtrReturn(pEnv, VERR_INVALID_POINTER);

    LogRel2(("Stopping all services ...\n"));

    /*
     * Signal all the services.
     */
    for (unsigned i = 0; i < RT_ELEMENTS(g_aServices); i++)
        ASMAtomicWriteBool(&g_aServices[i].fShutdown, true);

    /*
     * Do the pfnStop callback on all running services.
     */
    for (unsigned i = 0; i < RT_ELEMENTS(g_aServices); i++)
    {
        PVBOXSERVICEINFO pSvc = &g_aServices[i];
        if (   pSvc->fStarted
            && pSvc->pDesc->pfnStop)
        {
            LogRel2(("Calling stop function for service '%s' ...\n", pSvc->pDesc->pszName));
            int rc2 = pSvc->pDesc->pfnStop(pSvc->pInstance);
            if (RT_FAILURE(rc2))
                LogRel(("Failed to stop service '%s': %Rrc\n", pSvc->pDesc->pszName, rc2));
        }
    }

    LogRel2(("All stop functions for services called\n"));

    int rc = VINF_SUCCESS;

    /*
     * Wait for all the service threads to complete.
     */
    for (unsigned i = 0; i < RT_ELEMENTS(g_aServices); i++)
    {
        PVBOXSERVICEINFO pSvc = &g_aServices[i];
        if (!pSvc->fEnabled) /* Only stop services which were started before. */
            continue;

        if (pSvc->hThread != NIL_RTTHREAD)
        {
            LogRel2(("Waiting for service '%s' to stop ...\n", pSvc->pDesc->pszName));
            int rc2 = VINF_SUCCESS;
            for (int j = 0; j < 30; j++) /* Wait 30 seconds in total */
            {
                rc2 = RTThreadWait(pSvc->hThread, 1000 /* Wait 1 second */, NULL);
                if (RT_SUCCESS(rc2))
                    break;
            }
            if (RT_FAILURE(rc2))
            {
                LogRel(("Service '%s' failed to stop (%Rrc)\n", pSvc->pDesc->pszName, rc2));
                if (RT_SUCCESS(rc))
                    rc = rc2;
            }
        }

        if (pSvc->pDesc->pfnDestroy)
        {
            LogRel2(("Terminating service '%s' ...\n", pSvc->pDesc->pszName));
            pSvc->pDesc->pfnDestroy(pSvc->pInstance);
        }
    }

    if (RT_SUCCESS(rc))
        LogRel(("All services stopped\n"));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

static int vboxTrayRegisterGlobalMessages(PVBOXGLOBALMESSAGE pTable)
{
    int rc = VINF_SUCCESS;
    if (pTable == NULL) /* No table to register? Skip. */
        return rc;
    while (   pTable->pszName
           && RT_SUCCESS(rc))
    {
        /* Register global accessible window messages. */
        pTable->uMsgID = RegisterWindowMessage(TEXT(pTable->pszName));
        if (!pTable->uMsgID)
        {
            DWORD dwErr = GetLastError();
            Log(("Registering global message \"%s\" failed, error = %08X\n", dwErr));
            rc = RTErrConvertFromWin32(dwErr);
        }

        /* Advance to next table element. */
        pTable++;
    }
    return rc;
}

static bool vboxTrayHandleGlobalMessages(PVBOXGLOBALMESSAGE pTable, UINT uMsg,
                                         WPARAM wParam, LPARAM lParam)
{
    if (pTable == NULL)
        return false;
    while (pTable && pTable->pszName)
    {
        if (pTable->uMsgID == uMsg)
        {
            if (pTable->pfnHandler)
                pTable->pfnHandler(wParam, lParam);
            return true;
        }

        /* Advance to next table element. */
        pTable++;
    }
    return false;
}

/**
 * Release logger callback.
 *
 * @return  IPRT status code.
 * @param   pLoggerRelease
 * @param   enmPhase
 * @param   pfnLog
 */
static void vboxTrayLogHeaderFooter(PRTLOGGER pLoggerRelease, RTLOGPHASE enmPhase, PFNRTLOGPHASEMSG pfnLog)
{
    /* Some introductory information. */
    static RTTIMESPEC s_TimeSpec;
    char szTmp[256];
    if (enmPhase == RTLOGPHASE_BEGIN)
        RTTimeNow(&s_TimeSpec);
    RTTimeSpecToString(&s_TimeSpec, szTmp, sizeof(szTmp));

    switch (enmPhase)
    {
        case RTLOGPHASE_BEGIN:
        {
            pfnLog(pLoggerRelease,
                   "VBoxTray %s r%s %s (%s %s) release log\n"
                   "Log opened %s\n",
                   RTBldCfgVersion(), RTBldCfgRevisionStr(), VBOX_BUILD_TARGET,
                   __DATE__, __TIME__, szTmp);

            int vrc = RTSystemQueryOSInfo(RTSYSOSINFO_PRODUCT, szTmp, sizeof(szTmp));
            if (RT_SUCCESS(vrc) || vrc == VERR_BUFFER_OVERFLOW)
                pfnLog(pLoggerRelease, "OS Product: %s\n", szTmp);
            vrc = RTSystemQueryOSInfo(RTSYSOSINFO_RELEASE, szTmp, sizeof(szTmp));
            if (RT_SUCCESS(vrc) || vrc == VERR_BUFFER_OVERFLOW)
                pfnLog(pLoggerRelease, "OS Release: %s\n", szTmp);
            vrc = RTSystemQueryOSInfo(RTSYSOSINFO_VERSION, szTmp, sizeof(szTmp));
            if (RT_SUCCESS(vrc) || vrc == VERR_BUFFER_OVERFLOW)
                pfnLog(pLoggerRelease, "OS Version: %s\n", szTmp);
            if (RT_SUCCESS(vrc) || vrc == VERR_BUFFER_OVERFLOW)
                pfnLog(pLoggerRelease, "OS Service Pack: %s\n", szTmp);

            /* the package type is interesting for Linux distributions */
            char szExecName[RTPATH_MAX];
            char *pszExecName = RTProcGetExecutablePath(szExecName, sizeof(szExecName));
            pfnLog(pLoggerRelease,
                   "Executable: %s\n"
                   "Process ID: %u\n"
                   "Package type: %s"
#ifdef VBOX_OSE
                   " (OSE)"
#endif
                   "\n",
                   pszExecName ? pszExecName : "unknown",
                   RTProcSelf(),
                   VBOX_PACKAGE_STRING);
            break;
        }

        case RTLOGPHASE_PREROTATE:
            pfnLog(pLoggerRelease, "Log rotated - Log started %s\n", szTmp);
            break;

        case RTLOGPHASE_POSTROTATE:
            pfnLog(pLoggerRelease, "Log continuation - Log started %s\n", szTmp);
            break;

        case RTLOGPHASE_END:
            pfnLog(pLoggerRelease, "End of log file - Log started %s\n", szTmp);
            break;

        default:
            /* nothing */;
    }
}

/**
 * Creates the default release logger outputting to the specified file.
 * Pass NULL for disabled logging.
 *
 * @return  IPRT status code.
 * @param   pszLogFile              Filename for log output.  Optional.
 */
static int vboxTrayLogCreate(const char *pszLogFile)
{
    /* Create release logger (stdout + file). */
    static const char * const s_apszGroups[] = VBOX_LOGGROUP_NAMES;
    RTUINT fFlags = RTLOGFLAGS_PREFIX_THREAD | RTLOGFLAGS_PREFIX_TIME_PROG;
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
    fFlags |= RTLOGFLAGS_USECRLF;
#endif
    RTERRINFOSTATIC ErrInfo;
    int rc = RTLogCreateEx(&g_pLoggerRelease, fFlags,
#ifdef DEBUG
                           "all.e.l.f",
                           "VBOXTRAY_LOG",
#else
                           "all",
                           "VBOXTRAY_RELEASE_LOG",
#endif
                           RT_ELEMENTS(s_apszGroups), s_apszGroups, RTLOGDEST_STDOUT,
                           vboxTrayLogHeaderFooter, g_cHistory, g_uHistoryFileSize, g_uHistoryFileTime,
                           RTErrInfoInitStatic(&ErrInfo), pszLogFile);
    if (RT_SUCCESS(rc))
    {
#ifdef DEBUG
        RTLogSetDefaultInstance(g_pLoggerRelease);
#else
        /* Register this logger as the release logger. */
        RTLogRelSetDefaultInstance(g_pLoggerRelease);
#endif
        /* Explicitly flush the log in case of VBOXTRAY_RELEASE_LOG=buffered. */
        RTLogFlush(g_pLoggerRelease);
    }
    else
        MessageBoxA(GetDesktopWindow(), ErrInfo.szMsg, "VBoxTray - Logging Error", MB_OK | MB_ICONERROR);

    return rc;
}

static void vboxTrayLogDestroy(void)
{
    RTLogDestroy(RTLogRelSetDefaultInstance(NULL));
}

static void vboxTrayDestroyToolWindow(void)
{
    if (g_hwndToolWindow)
    {
        Log(("Destroying tool window ...\n"));

        /* Destroy the tool window. */
        DestroyWindow(g_hwndToolWindow);
        g_hwndToolWindow = NULL;

        UnregisterClass("VBoxTrayToolWndClass", g_hInstance);
    }
}

static int vboxTrayCreateToolWindow(void)
{
    DWORD dwErr = ERROR_SUCCESS;

    /* Create a custom window class. */
    WNDCLASSEX wc = { 0 };
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_NOCLOSE;
    wc.lpfnWndProc   = (WNDPROC)vboxToolWndProc;
    wc.hInstance     = g_hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "VBoxTrayToolWndClass";

    if (!RegisterClassEx(&wc))
    {
        dwErr = GetLastError();
        Log(("Registering invisible tool window failed, error = %08X\n", dwErr));
    }
    else
    {
        /*
         * Create our (invisible) tool window.
         * Note: The window name ("VBoxTrayToolWnd") and class ("VBoxTrayToolWndClass") is
         * needed for posting globally registered messages to VBoxTray and must not be
         * changed! Otherwise things get broken!
         *
         */
        g_hwndToolWindow = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
                                         "VBoxTrayToolWndClass", "VBoxTrayToolWnd",
                                         WS_POPUPWINDOW,
                                         -200, -200, 100, 100, NULL, NULL, g_hInstance, NULL);
        if (!g_hwndToolWindow)
        {
            dwErr = GetLastError();
            Log(("Creating invisible tool window failed, error = %08X\n", dwErr));
        }
        else
        {
            /* Reload the cursor(s). */
            hlpReloadCursor();

            Log(("Invisible tool window handle = %p\n", g_hwndToolWindow));
        }
    }

    if (dwErr != ERROR_SUCCESS)
         vboxTrayDestroyToolWindow();
    return RTErrConvertFromWin32(dwErr);
}

static int vboxTraySetupSeamless(void)
{
    OSVERSIONINFO info;
    g_dwMajorVersion = 5; /* Default to Windows XP. */
    info.dwOSVersionInfoSize = sizeof(info);
    if (GetVersionEx(&info))
    {
        Log(("Windows version %ld.%ld\n", info.dwMajorVersion, info.dwMinorVersion));
        g_dwMajorVersion = info.dwMajorVersion;
    }

    /* We need to setup a security descriptor to allow other processes modify access to the seamless notification event semaphore. */
    SECURITY_ATTRIBUTES     SecAttr;
    DWORD                   dwErr = ERROR_SUCCESS;
    char                    secDesc[SECURITY_DESCRIPTOR_MIN_LENGTH];
    BOOL                    fRC;

    SecAttr.nLength              = sizeof(SecAttr);
    SecAttr.bInheritHandle       = FALSE;
    SecAttr.lpSecurityDescriptor = &secDesc;
    InitializeSecurityDescriptor(SecAttr.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    fRC = SetSecurityDescriptorDacl(SecAttr.lpSecurityDescriptor, TRUE, 0, FALSE);
    if (!fRC)
    {
        dwErr = GetLastError();
        Log(("SetSecurityDescriptorDacl failed with last error = %08X\n", dwErr));
    }
    else
    {
        /* For Vista and up we need to change the integrity of the security descriptor, too. */
        if (g_dwMajorVersion >= 6)
        {
            BOOL (WINAPI * pfnConvertStringSecurityDescriptorToSecurityDescriptorA)(LPCSTR StringSecurityDescriptor, DWORD StringSDRevision, PSECURITY_DESCRIPTOR  *SecurityDescriptor, PULONG  SecurityDescriptorSize);
            *(void **)&pfnConvertStringSecurityDescriptorToSecurityDescriptorA =
                RTLdrGetSystemSymbol("advapi32.dll", "ConvertStringSecurityDescriptorToSecurityDescriptorA");
            Log(("pfnConvertStringSecurityDescriptorToSecurityDescriptorA = %x\n", pfnConvertStringSecurityDescriptorToSecurityDescriptorA));
            if (pfnConvertStringSecurityDescriptorToSecurityDescriptorA)
            {
                PSECURITY_DESCRIPTOR    pSD;
                PACL                    pSacl          = NULL;
                BOOL                    fSaclPresent   = FALSE;
                BOOL                    fSaclDefaulted = FALSE;

                fRC = pfnConvertStringSecurityDescriptorToSecurityDescriptorA("S:(ML;;NW;;;LW)", /* this means "low integrity" */
                                                                              SDDL_REVISION_1, &pSD, NULL);
                if (!fRC)
                {
                    dwErr = GetLastError();
                    Log(("ConvertStringSecurityDescriptorToSecurityDescriptorA failed with last error = %08X\n", dwErr));
                }
                else
                {
                    fRC = GetSecurityDescriptorSacl(pSD, &fSaclPresent, &pSacl, &fSaclDefaulted);
                    if (!fRC)
                    {
                        dwErr = GetLastError();
                        Log(("GetSecurityDescriptorSacl failed with last error = %08X\n", dwErr));
                    }
                    else
                    {
                        fRC = SetSecurityDescriptorSacl(SecAttr.lpSecurityDescriptor, TRUE, pSacl, FALSE);
                        if (!fRC)
                        {
                            dwErr = GetLastError();
                            Log(("SetSecurityDescriptorSacl failed with last error = %08X\n", dwErr));
                        }
                    }
                }
            }
        }

        if (   dwErr == ERROR_SUCCESS
            && g_dwMajorVersion >= 5) /* Only for W2K and up ... */
        {
            g_hSeamlessWtNotifyEvent = CreateEvent(&SecAttr, FALSE, FALSE, VBOXHOOK_GLOBAL_WT_EVENT_NAME);
            if (g_hSeamlessWtNotifyEvent == NULL)
            {
                dwErr = GetLastError();
                Log(("CreateEvent for Seamless failed, last error = %08X\n", dwErr));
            }

            g_hSeamlessKmNotifyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (g_hSeamlessKmNotifyEvent == NULL)
            {
                dwErr = GetLastError();
                Log(("CreateEvent for Seamless failed, last error = %08X\n", dwErr));
            }
        }
    }
    return RTErrConvertFromWin32(dwErr);
}

static void vboxTrayShutdownSeamless(void)
{
    if (g_hSeamlessWtNotifyEvent)
    {
        CloseHandle(g_hSeamlessWtNotifyEvent);
        g_hSeamlessWtNotifyEvent = NULL;
    }

    if (g_hSeamlessKmNotifyEvent)
    {
        CloseHandle(g_hSeamlessKmNotifyEvent);
        g_hSeamlessKmNotifyEvent = NULL;
    }
}

static void VBoxTrayCheckDt()
{
    BOOL fOldAllowedState = VBoxConsoleIsAllowed();
    if (vboxDtHandleEvent())
    {
        if (!VBoxConsoleIsAllowed() != !fOldAllowedState)
            VBoxConsoleEnable(!fOldAllowedState);
    }
}

static int vboxTrayServiceMain(void)
{
    int rc = VINF_SUCCESS;
    LogFunc(("Entering vboxTrayServiceMain\n"));

    g_hStopSem = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_hStopSem == NULL)
    {
        rc = RTErrConvertFromWin32(GetLastError());
        LogFunc(("CreateEvent for stopping VBoxTray failed, rc=%Rrc\n", rc));
    }
    else
    {
        /*
         * Start services listed in the vboxServiceTable.
         */
        VBOXSERVICEENV svcEnv;
        svcEnv.hInstance = g_hInstance;

        /* Initializes disp-if to default (XPDM) mode. */
        VBoxDispIfInit(&svcEnv.dispIf); /* Cannot fail atm. */
    #ifdef VBOX_WITH_WDDM
        /*
         * For now the display mode will be adjusted to WDDM mode if needed
         * on display service initialization when it detects the display driver type.
         */
    #endif

        /* Finally start all the built-in services! */
        rc = vboxTrayServicesStart(&svcEnv);
        if (RT_FAILURE(rc))
        {
            /* Terminate service if something went wrong. */
            vboxTrayServicesStop(&svcEnv);
        }
        else
        {
            rc = vboxTrayCreateTrayIcon();
            if (   RT_SUCCESS(rc)
                && g_dwMajorVersion >= 5) /* Only for W2K and up ... */
            {
                /* We're ready to create the tooltip balloon.
                   Check in 10 seconds (@todo make seconds configurable) ... */
                SetTimer(g_hwndToolWindow,
                         TIMERID_VBOXTRAY_CHECK_HOSTVERSION,
                         10 * 1000, /* 10 seconds */
                         NULL       /* No timerproc */);
            }

            if (RT_SUCCESS(rc))
            {
                /* Do the Shared Folders auto-mounting stuff. */
                rc = VBoxSharedFoldersAutoMount();
                if (RT_SUCCESS(rc))
                {
                    /* Report the host that we're up and running! */
                    hlpReportStatus(VBoxGuestFacilityStatus_Active);
                }
            }

            if (RT_SUCCESS(rc))
            {
                /* Boost thread priority to make sure we wake up early for seamless window notifications
                 * (not sure if it actually makes any difference though). */
                SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

                /*
                 * Main execution loop
                 * Wait for the stop semaphore to be posted or a window event to arrive
                 */

                HANDLE hWaitEvent[4] = {0};
                DWORD dwEventCount = 0;

                hWaitEvent[dwEventCount++] = g_hStopSem;

                /* Check if seamless mode is not active and add seamless event to the list */
                if (0 != g_hSeamlessWtNotifyEvent)
                {
                    hWaitEvent[dwEventCount++] = g_hSeamlessWtNotifyEvent;
                }

                if (0 != g_hSeamlessKmNotifyEvent)
                {
                    hWaitEvent[dwEventCount++] = g_hSeamlessKmNotifyEvent;
                }

                if (0 != vboxDtGetNotifyEvent())
                {
                    hWaitEvent[dwEventCount++] = vboxDtGetNotifyEvent();
                }

                LogFlowFunc(("Number of events to wait in main loop: %ld\n", dwEventCount));
                while (true)
                {
                    DWORD waitResult = MsgWaitForMultipleObjectsEx(dwEventCount, hWaitEvent, 500, QS_ALLINPUT, 0);
                    waitResult = waitResult - WAIT_OBJECT_0;

                    /* Only enable for message debugging, lots of traffic! */
                    //Log(("Wait result  = %ld\n", waitResult));

                    if (waitResult == 0)
                    {
                        LogFunc(("Event 'Exit' triggered\n"));
                        /* exit */
                        break;
                    }
                    else
                    {
                        BOOL fHandled = FALSE;
                        if (waitResult < RT_ELEMENTS(hWaitEvent))
                        {
                            if (hWaitEvent[waitResult])
                            {
                                if (hWaitEvent[waitResult] == g_hSeamlessWtNotifyEvent)
                                {
                                    LogFunc(("Event 'Seamless' triggered\n"));

                                    /* seamless window notification */
                                    VBoxSeamlessCheckWindows(false);
                                    fHandled = TRUE;
                                }
                                else if (hWaitEvent[waitResult] == g_hSeamlessKmNotifyEvent)
                                {
                                    LogFunc(("Event 'Km Seamless' triggered\n"));

                                    /* seamless window notification */
                                    VBoxSeamlessCheckWindows(true);
                                    fHandled = TRUE;
                                }
                                else if (hWaitEvent[waitResult] == vboxDtGetNotifyEvent())
                                {
                                    LogFunc(("Event 'Dt' triggered\n"));
                                    VBoxTrayCheckDt();
                                    fHandled = TRUE;
                                }
                            }
                        }

                        if (!fHandled)
                        {
                            /* timeout or a window message, handle it */
                            MSG msg;
                            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                            {
#ifdef DEBUG_andy
                                LogFlowFunc(("PeekMessage %u\n", msg.message));
#endif
                                if (msg.message == WM_QUIT)
                                {
                                    LogFunc(("Terminating ...\n"));
                                    SetEvent(g_hStopSem);
                                }
                                TranslateMessage(&msg);
                                DispatchMessage(&msg);
                            }
                        }
                    }
                }
                LogFunc(("Returned from main loop, exiting ...\n"));
            }
            LogFunc(("Waiting for services to stop ...\n"));
            vboxTrayServicesStop(&svcEnv);
        } /* Services started */
        CloseHandle(g_hStopSem);
    } /* Stop event created */

    vboxTrayRemoveTrayIcon();

    LogFunc(("Leaving with rc=%Rrc\n", rc));
    return rc;
}

/**
 * Main function
 */
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    RT_NOREF(hPrevInstance, lpCmdLine, nCmdShow);
    int rc = RTR3InitExeNoArguments(0);
    if (RT_FAILURE(rc))
        return RTEXITCODE_INIT;

    /* Note: Do not use a global namespace ("Global\\") for mutex name here,
     * will blow up NT4 compatibility! */
    HANDLE hMutexAppRunning = CreateMutex(NULL, FALSE, "VBoxTray");
    if (   hMutexAppRunning != NULL
        && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        /* VBoxTray already running? Bail out. */
        CloseHandle (hMutexAppRunning);
        hMutexAppRunning = NULL;
        return 0;
    }

    LogRel(("%s r%s\n", RTBldCfgVersion(), RTBldCfgRevisionStr()));

    rc = vboxTrayLogCreate(NULL /* pszLogFile */);
    if (RT_SUCCESS(rc))
    {
        rc = VbglR3Init();
        if (RT_SUCCESS(rc))
        {
            /* Save instance handle. */
            g_hInstance = hInstance;

            hlpReportStatus(VBoxGuestFacilityStatus_Init);
            rc = vboxTrayCreateToolWindow();
            if (RT_SUCCESS(rc))
            {
                VBoxCapsInit();

                rc = vboxStInit(g_hwndToolWindow);
                if (!RT_SUCCESS(rc))
                {
                    LogFlowFunc(("vboxStInit failed, rc=%Rrc\n", rc));
                    /* ignore the St Init failure. this can happen for < XP win that do not support WTS API
                     * in that case the session is treated as active connected to the physical console
                     * (i.e. fallback to the old behavior that was before introduction of VBoxSt) */
                    Assert(vboxStIsActiveConsole());
                }

                rc = vboxDtInit();
                if (!RT_SUCCESS(rc))
                {
                    LogFlowFunc(("vboxDtInit failed, rc=%Rrc\n", rc));
                    /* ignore the Dt Init failure. this can happen for < XP win that do not support WTS API
                     * in that case the session is treated as active connected to the physical console
                     * (i.e. fallback to the old behavior that was before introduction of VBoxSt) */
                    Assert(vboxDtIsInputDesktop());
                }

                rc = VBoxAcquireGuestCaps(VMMDEV_GUEST_SUPPORTS_SEAMLESS | VMMDEV_GUEST_SUPPORTS_GRAPHICS, 0, true);
                if (!RT_SUCCESS(rc))
                    LogFlowFunc(("VBoxAcquireGuestCaps failed with rc=%Rrc, ignoring ...\n", rc));

                rc = vboxTraySetupSeamless(); /** @todo r=andy Do we really want to be this critical for the whole application? */
                if (RT_SUCCESS(rc))
                {
                    rc = vboxTrayServiceMain();
                    if (RT_SUCCESS(rc))
                        hlpReportStatus(VBoxGuestFacilityStatus_Terminating);
                    vboxTrayShutdownSeamless();
                }

                /* it should be safe to call vboxDtTerm even if vboxStInit above failed */
                vboxDtTerm();

                /* it should be safe to call vboxStTerm even if vboxStInit above failed */
                vboxStTerm();

                VBoxCapsTerm();

                vboxTrayDestroyToolWindow();
            }
            if (RT_SUCCESS(rc))
                hlpReportStatus(VBoxGuestFacilityStatus_Terminated);
            else
            {
                LogRel(("Error while starting, rc=%Rrc\n", rc));
                hlpReportStatus(VBoxGuestFacilityStatus_Failed);
            }

            LogRel(("Ended\n"));
            VbglR3Term();
        }
        else
            LogRel(("VbglR3Init failed: %Rrc\n", rc));
    }

    /* Release instance mutex. */
    if (hMutexAppRunning != NULL)
    {
        CloseHandle(hMutexAppRunning);
        hMutexAppRunning = NULL;
    }

    vboxTrayLogDestroy();

    return RT_SUCCESS(rc) ? 0 : 1;
}

/**
 * Window procedure for our main tool window.
 */
static LRESULT CALLBACK vboxToolWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LogFlowFunc(("hWnd=%p, uMsg=%u\n", hWnd, uMsg));

    switch (uMsg)
    {
        case WM_CREATE:
        {
            LogFunc(("Tool window created\n"));

            int rc = vboxTrayRegisterGlobalMessages(&g_vboxGlobalMessageTable[0]);
            if (RT_FAILURE(rc))
                LogFunc(("Error registering global window messages, rc=%Rrc\n", rc));
            return 0;
        }

        case WM_CLOSE:
            return 0;

        case WM_DESTROY:
        {
            LogFunc(("Tool window destroyed\n"));
            KillTimer(g_hwndToolWindow, TIMERID_VBOXTRAY_CHECK_HOSTVERSION);
            return 0;
        }

        case WM_TIMER:
        {
            if (VBoxCapsCheckTimer(wParam))
                return 0;
            if (vboxDtCheckTimer(wParam))
                return 0;
            if (vboxStCheckTimer(wParam))
                return 0;

            switch (wParam)
            {
                case TIMERID_VBOXTRAY_CHECK_HOSTVERSION:
                    if (RT_SUCCESS(VBoxCheckHostVersion()))
                    {
                        /* After successful run we don't need to check again. */
                        KillTimer(g_hwndToolWindow, TIMERID_VBOXTRAY_CHECK_HOSTVERSION);
                    }
                    return 0;

                default:
                    break;
            }

            break; /* Make sure other timers get processed the usual way! */
        }

        case WM_VBOXTRAY_TRAY_ICON:
        {
            switch (lParam)
            {
                case WM_LBUTTONDBLCLK:
                    break;

                case WM_RBUTTONDOWN:
                    break;
            }
            return 0;
        }

        case WM_VBOX_SEAMLESS_ENABLE:
        {
            VBoxCapsEntryFuncStateSet(VBOXCAPS_ENTRY_IDX_SEAMLESS, VBOXCAPS_ENTRY_FUNCSTATE_STARTED);
            if (VBoxCapsEntryIsEnabled(VBOXCAPS_ENTRY_IDX_SEAMLESS))
                VBoxSeamlessCheckWindows(true);
            return 0;
        }

        case WM_VBOX_SEAMLESS_DISABLE:
        {
            VBoxCapsEntryFuncStateSet(VBOXCAPS_ENTRY_IDX_SEAMLESS, VBOXCAPS_ENTRY_FUNCSTATE_SUPPORTED);
            return 0;
        }

        case WM_DISPLAYCHANGE:
            ASMAtomicUoWriteU32(&g_fGuestDisplaysChanged, 1);
            // No break or return is intentional here.
        case WM_VBOX_SEAMLESS_UPDATE:
        {
            if (VBoxCapsEntryIsEnabled(VBOXCAPS_ENTRY_IDX_SEAMLESS))
                VBoxSeamlessCheckWindows(true);
            return 0;
        }

        case WM_VBOX_GRAPHICS_SUPPORTED:
        {
            VBoxGrapicsSetSupported(TRUE);
            return 0;
        }

        case WM_VBOX_GRAPHICS_UNSUPPORTED:
        {
            VBoxGrapicsSetSupported(FALSE);
            return 0;
        }

        case WM_WTSSESSION_CHANGE:
        {
            BOOL fOldAllowedState = VBoxConsoleIsAllowed();
            if (vboxStHandleEvent(wParam))
            {
                if (!VBoxConsoleIsAllowed() != !fOldAllowedState)
                    VBoxConsoleEnable(!fOldAllowedState);
            }
            return 0;
        }

        default:
        {
            /* Handle all globally registered window messages. */
            if (vboxTrayHandleGlobalMessages(&g_vboxGlobalMessageTable[0], uMsg,
                                             wParam, lParam))
            {
                return 0; /* We handled the message. @todo Add return value!*/
            }
            break; /* We did not handle the message, dispatch to DefWndProc. */
        }
    }

    /* Only if message was *not* handled by our switch above, dispatch to DefWindowProc. */
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/* St (session [state] tracking) functionality API impl */

typedef struct VBOXST
{
    HWND hWTSAPIWnd;
    RTLDRMOD hLdrModWTSAPI32;
    BOOL fIsConsole;
    WTS_CONNECTSTATE_CLASS enmConnectState;
    UINT_PTR idDelayedInitTimer;
    BOOL (WINAPI * pfnWTSRegisterSessionNotification)(HWND hWnd, DWORD dwFlags);
    BOOL (WINAPI * pfnWTSUnRegisterSessionNotification)(HWND hWnd);
    BOOL (WINAPI * pfnWTSQuerySessionInformationA)(HANDLE hServer, DWORD SessionId, WTS_INFO_CLASS WTSInfoClass, LPTSTR *ppBuffer, DWORD *pBytesReturned);
} VBOXST;

static VBOXST gVBoxSt;

static int vboxStCheckState()
{
    int rc = VINF_SUCCESS;
    WTS_CONNECTSTATE_CLASS *penmConnectState = NULL;
    USHORT *pProtocolType = NULL;
    DWORD cbBuf = 0;
    if (gVBoxSt.pfnWTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSConnectState,
                                               (LPTSTR *)&penmConnectState, &cbBuf))
    {
        if (gVBoxSt.pfnWTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSClientProtocolType,
                                                   (LPTSTR *)&pProtocolType, &cbBuf))
        {
            gVBoxSt.fIsConsole = (*pProtocolType == 0);
            gVBoxSt.enmConnectState = *penmConnectState;
            return VINF_SUCCESS;
        }

        DWORD dwErr = GetLastError();
        LogFlowFunc(("WTSQuerySessionInformationA WTSClientProtocolType failed, error = %08X\n", dwErr));
        rc = RTErrConvertFromWin32(dwErr);
    }
    else
    {
        DWORD dwErr = GetLastError();
        LogFlowFunc(("WTSQuerySessionInformationA WTSConnectState failed, error = %08X\n", dwErr));
        rc = RTErrConvertFromWin32(dwErr);
    }

    /* failure branch, set to "console-active" state */
    gVBoxSt.fIsConsole = TRUE;
    gVBoxSt.enmConnectState = WTSActive;

    return rc;
}

static int vboxStInit(HWND hWnd)
{
    RT_ZERO(gVBoxSt);
    int rc = RTLdrLoadSystem("WTSAPI32.DLL", false /*fNoUnload*/, &gVBoxSt.hLdrModWTSAPI32);
    if (RT_SUCCESS(rc))
    {
        rc = RTLdrGetSymbol(gVBoxSt.hLdrModWTSAPI32, "WTSRegisterSessionNotification",
                            (void **)&gVBoxSt.pfnWTSRegisterSessionNotification);
        if (RT_SUCCESS(rc))
        {
            rc = RTLdrGetSymbol(gVBoxSt.hLdrModWTSAPI32, "WTSUnRegisterSessionNotification",
                                (void **)&gVBoxSt.pfnWTSUnRegisterSessionNotification);
            if (RT_SUCCESS(rc))
            {
                rc = RTLdrGetSymbol(gVBoxSt.hLdrModWTSAPI32, "WTSQuerySessionInformationA",
                                    (void **)&gVBoxSt.pfnWTSQuerySessionInformationA);
                if (RT_FAILURE(rc))
                    LogFlowFunc(("WTSQuerySessionInformationA not found\n"));
            }
            else
                LogFlowFunc(("WTSUnRegisterSessionNotification not found\n"));
        }
        else
            LogFlowFunc(("WTSRegisterSessionNotification not found\n"));
        if (RT_SUCCESS(rc))
        {
            gVBoxSt.hWTSAPIWnd = hWnd;
            if (gVBoxSt.pfnWTSRegisterSessionNotification(gVBoxSt.hWTSAPIWnd, NOTIFY_FOR_THIS_SESSION))
                vboxStCheckState();
            else
            {
                DWORD dwErr = GetLastError();
                LogFlowFunc(("WTSRegisterSessionNotification failed, error = %08X\n", dwErr));
                if (dwErr == RPC_S_INVALID_BINDING)
                {
                    gVBoxSt.idDelayedInitTimer = SetTimer(gVBoxSt.hWTSAPIWnd, TIMERID_VBOXTRAY_ST_DELAYED_INIT_TIMER,
                                                          2000, (TIMERPROC)NULL);
                    gVBoxSt.fIsConsole = TRUE;
                    gVBoxSt.enmConnectState = WTSActive;
                    rc = VINF_SUCCESS;
                }
                else
                    rc = RTErrConvertFromWin32(dwErr);
            }

            if (RT_SUCCESS(rc))
                return VINF_SUCCESS;
        }

        RTLdrClose(gVBoxSt.hLdrModWTSAPI32);
    }
    else
        LogFlowFunc(("WTSAPI32 load failed, rc = %Rrc\n", rc));

    RT_ZERO(gVBoxSt);
    gVBoxSt.fIsConsole = TRUE;
    gVBoxSt.enmConnectState = WTSActive;
    return rc;
}

static void vboxStTerm(void)
{
    if (!gVBoxSt.hWTSAPIWnd)
    {
        LogFlowFunc(("vboxStTerm called for non-initialized St\n"));
        return;
    }

    if (gVBoxSt.idDelayedInitTimer)
    {
        /* notification is not registered, just kill timer */
        KillTimer(gVBoxSt.hWTSAPIWnd, gVBoxSt.idDelayedInitTimer);
        gVBoxSt.idDelayedInitTimer = 0;
    }
    else
    {
        if (!gVBoxSt.pfnWTSUnRegisterSessionNotification(gVBoxSt.hWTSAPIWnd))
        {
            LogFlowFunc(("WTSAPI32 load failed, error = %08X\n", GetLastError()));
        }
    }

    RTLdrClose(gVBoxSt.hLdrModWTSAPI32);
    RT_ZERO(gVBoxSt);
}

#define VBOXST_DBG_MAKECASE(_val) case _val: return #_val;

static const char* vboxStDbgGetString(DWORD val)
{
    switch (val)
    {
        VBOXST_DBG_MAKECASE(WTS_CONSOLE_CONNECT);
        VBOXST_DBG_MAKECASE(WTS_CONSOLE_DISCONNECT);
        VBOXST_DBG_MAKECASE(WTS_REMOTE_CONNECT);
        VBOXST_DBG_MAKECASE(WTS_REMOTE_DISCONNECT);
        VBOXST_DBG_MAKECASE(WTS_SESSION_LOGON);
        VBOXST_DBG_MAKECASE(WTS_SESSION_LOGOFF);
        VBOXST_DBG_MAKECASE(WTS_SESSION_LOCK);
        VBOXST_DBG_MAKECASE(WTS_SESSION_UNLOCK);
        VBOXST_DBG_MAKECASE(WTS_SESSION_REMOTE_CONTROL);
        default:
            LogFlowFunc(("invalid WTS state %d\n", val));
            return "Unknown";
    }
}

static BOOL vboxStCheckTimer(WPARAM wEvent)
{
    if (wEvent != gVBoxSt.idDelayedInitTimer)
        return FALSE;

    if (gVBoxSt.pfnWTSRegisterSessionNotification(gVBoxSt.hWTSAPIWnd, NOTIFY_FOR_THIS_SESSION))
    {
        KillTimer(gVBoxSt.hWTSAPIWnd, gVBoxSt.idDelayedInitTimer);
        gVBoxSt.idDelayedInitTimer = 0;
        vboxStCheckState();
    }
    else
    {
        LogFlowFunc(("timer WTSRegisterSessionNotification failed, error = %08X\n", GetLastError()));
        Assert(gVBoxSt.fIsConsole == TRUE);
        Assert(gVBoxSt.enmConnectState == WTSActive);
    }

    return TRUE;
}


static BOOL vboxStHandleEvent(WPARAM wEvent)
{
    RT_NOREF(wEvent);
    LogFlowFunc(("WTS Event: %s\n", vboxStDbgGetString(wEvent)));
    BOOL fOldIsActiveConsole = vboxStIsActiveConsole();

    vboxStCheckState();

    return !vboxStIsActiveConsole() != !fOldIsActiveConsole;
}

static BOOL vboxStIsActiveConsole(void)
{
    return (gVBoxSt.enmConnectState == WTSActive && gVBoxSt.fIsConsole);
}

/*
 * Dt (desktop [state] tracking) functionality API impl
 *
 * !!!NOTE: this API is NOT thread-safe!!!
 * */

typedef struct VBOXDT
{
    HANDLE hNotifyEvent;
    BOOL fIsInputDesktop;
    UINT_PTR idTimer;
    RTLDRMOD hLdrModHook;
    BOOL (* pfnVBoxHookInstallActiveDesktopTracker)(HMODULE hDll);
    BOOL (* pfnVBoxHookRemoveActiveDesktopTracker)();
    HDESK (WINAPI * pfnGetThreadDesktop)(DWORD dwThreadId);
    HDESK (WINAPI * pfnOpenInputDesktop)(DWORD dwFlags, BOOL fInherit, ACCESS_MASK dwDesiredAccess);
    BOOL (WINAPI * pfnCloseDesktop)(HDESK hDesktop);
} VBOXDT;

static VBOXDT gVBoxDt;

static BOOL vboxDtCalculateIsInputDesktop()
{
    BOOL fIsInputDt = FALSE;
    HDESK hInput = gVBoxDt.pfnOpenInputDesktop(0, FALSE, DESKTOP_CREATEWINDOW);
    if (hInput)
    {
//        DWORD dwThreadId = GetCurrentThreadId();
//        HDESK hThreadDt = gVBoxDt.pfnGetThreadDesktop(dwThreadId);
//        if (hThreadDt)
//        {
            fIsInputDt = TRUE;
//        }
//        else
//        {
//            DWORD dwErr = GetLastError();
//            LogFlowFunc(("pfnGetThreadDesktop for Seamless failed, last error = %08X\n", dwErr));
//        }

        gVBoxDt.pfnCloseDesktop(hInput);
    }
    else
    {
//        DWORD dwErr = GetLastError();
//        LogFlowFunc(("pfnOpenInputDesktop for Seamless failed, last error = %08X\n", dwErr));
    }
    return fIsInputDt;
}

static BOOL vboxDtCheckTimer(WPARAM wParam)
{
    if (wParam != gVBoxDt.idTimer)
        return FALSE;

    VBoxTrayCheckDt();

    return TRUE;
}

static int vboxDtInit()
{
    int rc = VINF_SUCCESS;
    OSVERSIONINFO info;
    g_dwMajorVersion = 5; /* Default to Windows XP. */
    info.dwOSVersionInfoSize = sizeof(info);
    if (GetVersionEx(&info))
    {
        LogRel(("Windows version %ld.%ld\n", info.dwMajorVersion, info.dwMinorVersion));
        g_dwMajorVersion = info.dwMajorVersion;
    }

    RT_ZERO(gVBoxDt);

    gVBoxDt.hNotifyEvent = CreateEvent(NULL, FALSE, FALSE, VBOXHOOK_GLOBAL_DT_EVENT_NAME);
    if (gVBoxDt.hNotifyEvent != NULL)
    {
        /* Load the hook dll and resolve the necessary entry points. */
        rc = RTLdrLoadAppPriv(VBOXHOOK_DLL_NAME, &gVBoxDt.hLdrModHook);
        if (RT_SUCCESS(rc))
        {
            rc = RTLdrGetSymbol(gVBoxDt.hLdrModHook, "VBoxHookInstallActiveDesktopTracker",
                                (void **)&gVBoxDt.pfnVBoxHookInstallActiveDesktopTracker);
            if (RT_SUCCESS(rc))
            {
                rc = RTLdrGetSymbol(gVBoxDt.hLdrModHook, "VBoxHookRemoveActiveDesktopTracker",
                                    (void **)&gVBoxDt.pfnVBoxHookRemoveActiveDesktopTracker);
                if (RT_FAILURE(rc))
                    LogFlowFunc(("VBoxHookRemoveActiveDesktopTracker not found\n"));
            }
            else
                LogFlowFunc(("VBoxHookInstallActiveDesktopTracker not found\n"));
            if (RT_SUCCESS(rc))
            {
                /* Try get the system APIs we need. */
                *(void **)&gVBoxDt.pfnGetThreadDesktop = RTLdrGetSystemSymbol("user32.dll", "GetThreadDesktop");
                if (!gVBoxDt.pfnGetThreadDesktop)
                {
                    LogFlowFunc(("GetThreadDesktop not found\n"));
                    rc = VERR_NOT_SUPPORTED;
                }

                *(void **)&gVBoxDt.pfnOpenInputDesktop = RTLdrGetSystemSymbol("user32.dll", "OpenInputDesktop");
                if (!gVBoxDt.pfnOpenInputDesktop)
                {
                    LogFlowFunc(("OpenInputDesktop not found\n"));
                    rc = VERR_NOT_SUPPORTED;
                }

                *(void **)&gVBoxDt.pfnCloseDesktop = RTLdrGetSystemSymbol("user32.dll", "CloseDesktop");
                if (!gVBoxDt.pfnCloseDesktop)
                {
                    LogFlowFunc(("CloseDesktop not found\n"));
                    rc = VERR_NOT_SUPPORTED;
                }

                if (RT_SUCCESS(rc))
                {
                    BOOL fRc = FALSE;
                    /* For Vista and up we need to change the integrity of the security descriptor, too. */
                    if (g_dwMajorVersion >= 6)
                    {
                        HMODULE hModHook = (HMODULE)RTLdrGetNativeHandle(gVBoxDt.hLdrModHook);
                        Assert((uintptr_t)hModHook != ~(uintptr_t)0);
                        fRc = gVBoxDt.pfnVBoxHookInstallActiveDesktopTracker(hModHook);
                        if (!fRc)
                            LogFlowFunc(("pfnVBoxHookInstallActiveDesktopTracker failed, last error = %08X\n", GetLastError()));
                    }

                    if (!fRc)
                    {
                        gVBoxDt.idTimer = SetTimer(g_hwndToolWindow, TIMERID_VBOXTRAY_DT_TIMER, 500, (TIMERPROC)NULL);
                        if (!gVBoxDt.idTimer)
                        {
                            DWORD dwErr = GetLastError();
                            LogFlowFunc(("SetTimer error %08X\n", dwErr));
                            rc = RTErrConvertFromWin32(dwErr);
                        }
                    }

                    if (RT_SUCCESS(rc))
                    {
                        gVBoxDt.fIsInputDesktop = vboxDtCalculateIsInputDesktop();
                        return VINF_SUCCESS;
                    }
                }
            }

            RTLdrClose(gVBoxDt.hLdrModHook);
        }
        else
        {
            DWORD dwErr = GetLastError();
            LogFlowFunc(("CreateEvent for Seamless failed, last error = %08X\n", dwErr));
            rc = RTErrConvertFromWin32(dwErr);
        }

        CloseHandle(gVBoxDt.hNotifyEvent);
    }
    else
    {
        DWORD dwErr = GetLastError();
        LogFlowFunc(("CreateEvent for Seamless failed, last error = %08X\n", dwErr));
        rc = RTErrConvertFromWin32(dwErr);
    }


    RT_ZERO(gVBoxDt);
    gVBoxDt.fIsInputDesktop = TRUE;

    return rc;
}

static void vboxDtTerm()
{
    if (!gVBoxDt.hLdrModHook)
        return;

    gVBoxDt.pfnVBoxHookRemoveActiveDesktopTracker();

    RTLdrClose(gVBoxDt.hLdrModHook);
    CloseHandle(gVBoxDt.hNotifyEvent);

    RT_ZERO(gVBoxDt);
}
/* @returns true on "IsInputDesktop" state change */
static BOOL vboxDtHandleEvent()
{
    BOOL fIsInputDesktop = gVBoxDt.fIsInputDesktop;
    gVBoxDt.fIsInputDesktop = vboxDtCalculateIsInputDesktop();
    return !fIsInputDesktop != !gVBoxDt.fIsInputDesktop;
}

static HANDLE vboxDtGetNotifyEvent()
{
    return gVBoxDt.hNotifyEvent;
}

/* @returns true iff the application (VBoxTray) desktop is input */
static BOOL vboxDtIsInputDesktop()
{
    return gVBoxDt.fIsInputDesktop;
}

/* we need to perform Acquire/Release using the file handled we use for rewuesting events from VBoxGuest
 * otherwise Acquisition mechanism will treat us as different client and will not propagate necessary requests
 * */
static int VBoxAcquireGuestCaps(uint32_t fOr, uint32_t fNot, bool fCfg)
{
    Log(("VBoxAcquireGuestCaps or(0x%x), not(0x%x), cfx(%d)\n", fOr, fNot, fCfg));
    int rc = VbglR3AcquireGuestCaps(fOr, fNot, fCfg);
    if (RT_FAILURE(rc))
        LogFlowFunc(("VBOXGUEST_IOCTL_GUEST_CAPS_ACQUIRE failed: %Rrc\n", rc));
    return rc;
}

typedef enum VBOXCAPS_ENTRY_ACSTATE
{
    /* the given cap is released */
    VBOXCAPS_ENTRY_ACSTATE_RELEASED = 0,
    /* the given cap acquisition is in progress */
    VBOXCAPS_ENTRY_ACSTATE_ACQUIRING,
    /* the given cap is acquired */
    VBOXCAPS_ENTRY_ACSTATE_ACQUIRED
} VBOXCAPS_ENTRY_ACSTATE;


struct VBOXCAPS_ENTRY;
struct VBOXCAPS;

typedef DECLCALLBACKPTR(void, PFNVBOXCAPS_ENTRY_ON_ENABLE)(struct VBOXCAPS *pConsole, struct VBOXCAPS_ENTRY *pCap, BOOL fEnabled);

typedef struct VBOXCAPS_ENTRY
{
    uint32_t fCap;
    uint32_t iCap;
    VBOXCAPS_ENTRY_FUNCSTATE enmFuncState;
    VBOXCAPS_ENTRY_ACSTATE enmAcState;
    PFNVBOXCAPS_ENTRY_ON_ENABLE pfnOnEnable;
} VBOXCAPS_ENTRY;


typedef struct VBOXCAPS
{
    UINT_PTR idTimer;
    VBOXCAPS_ENTRY aCaps[VBOXCAPS_ENTRY_IDX_COUNT];
} VBOXCAPS;

static VBOXCAPS gVBoxCaps;

static DECLCALLBACK(void) vboxCapsOnEnableSeamless(struct VBOXCAPS *pConsole, struct VBOXCAPS_ENTRY *pCap, BOOL fEnabled)
{
    RT_NOREF(pConsole, pCap);
    if (fEnabled)
    {
        Log(("vboxCapsOnEnableSeamless: ENABLED\n"));
        Assert(pCap->enmAcState == VBOXCAPS_ENTRY_ACSTATE_ACQUIRED);
        Assert(pCap->enmFuncState == VBOXCAPS_ENTRY_FUNCSTATE_STARTED);
        VBoxSeamlessEnable();
    }
    else
    {
        Log(("vboxCapsOnEnableSeamless: DISABLED\n"));
        Assert(pCap->enmAcState != VBOXCAPS_ENTRY_ACSTATE_ACQUIRED || pCap->enmFuncState != VBOXCAPS_ENTRY_FUNCSTATE_STARTED);
        VBoxSeamlessDisable();
    }
}

static void vboxCapsEntryAcStateSet(VBOXCAPS_ENTRY *pCap, VBOXCAPS_ENTRY_ACSTATE enmAcState)
{
    VBOXCAPS *pConsole = &gVBoxCaps;

    Log(("vboxCapsEntryAcStateSet: new state enmAcState(%d); pCap: fCap(%d), iCap(%d), enmFuncState(%d), enmAcState(%d)\n",
            enmAcState, pCap->fCap, pCap->iCap, pCap->enmFuncState, pCap->enmAcState));

    if (pCap->enmAcState == enmAcState)
        return;

    VBOXCAPS_ENTRY_ACSTATE enmOldAcState = pCap->enmAcState;
    pCap->enmAcState = enmAcState;

    if (enmAcState == VBOXCAPS_ENTRY_ACSTATE_ACQUIRED)
    {
        if (pCap->enmFuncState == VBOXCAPS_ENTRY_FUNCSTATE_STARTED)
        {
            if (pCap->pfnOnEnable)
                pCap->pfnOnEnable(pConsole, pCap, TRUE);
        }
    }
    else if (enmOldAcState == VBOXCAPS_ENTRY_ACSTATE_ACQUIRED && pCap->enmFuncState == VBOXCAPS_ENTRY_FUNCSTATE_STARTED)
    {
        if (pCap->pfnOnEnable)
            pCap->pfnOnEnable(pConsole, pCap, FALSE);
    }
}

static void vboxCapsEntryFuncStateSet(VBOXCAPS_ENTRY *pCap, VBOXCAPS_ENTRY_FUNCSTATE enmFuncState)
{
    VBOXCAPS *pConsole = &gVBoxCaps;

    Log(("vboxCapsEntryFuncStateSet: new state enmAcState(%d); pCap: fCap(%d), iCap(%d), enmFuncState(%d), enmAcState(%d)\n",
            enmFuncState, pCap->fCap, pCap->iCap, pCap->enmFuncState, pCap->enmAcState));

    if (pCap->enmFuncState == enmFuncState)
        return;

    VBOXCAPS_ENTRY_FUNCSTATE enmOldFuncState = pCap->enmFuncState;

    pCap->enmFuncState = enmFuncState;

    if (enmFuncState == VBOXCAPS_ENTRY_FUNCSTATE_STARTED)
    {
        Assert(enmOldFuncState == VBOXCAPS_ENTRY_FUNCSTATE_SUPPORTED);
        if (pCap->enmAcState == VBOXCAPS_ENTRY_ACSTATE_ACQUIRED)
        {
            if (pCap->pfnOnEnable)
                pCap->pfnOnEnable(pConsole, pCap, TRUE);
        }
    }
    else if (pCap->enmAcState == VBOXCAPS_ENTRY_ACSTATE_ACQUIRED && enmOldFuncState == VBOXCAPS_ENTRY_FUNCSTATE_STARTED)
    {
        if (pCap->pfnOnEnable)
            pCap->pfnOnEnable(pConsole, pCap, FALSE);
    }
}

static void VBoxCapsEntryFuncStateSet(uint32_t iCup, VBOXCAPS_ENTRY_FUNCSTATE enmFuncState)
{
    VBOXCAPS *pConsole = &gVBoxCaps;
    VBOXCAPS_ENTRY *pCap = &pConsole->aCaps[iCup];
    vboxCapsEntryFuncStateSet(pCap, enmFuncState);
}

static int VBoxCapsInit()
{
    VBOXCAPS *pConsole = &gVBoxCaps;
    memset(pConsole, 0, sizeof (*pConsole));
    pConsole->aCaps[VBOXCAPS_ENTRY_IDX_SEAMLESS].fCap = VMMDEV_GUEST_SUPPORTS_SEAMLESS;
    pConsole->aCaps[VBOXCAPS_ENTRY_IDX_SEAMLESS].iCap = VBOXCAPS_ENTRY_IDX_SEAMLESS;
    pConsole->aCaps[VBOXCAPS_ENTRY_IDX_SEAMLESS].pfnOnEnable = vboxCapsOnEnableSeamless;
    pConsole->aCaps[VBOXCAPS_ENTRY_IDX_GRAPHICS].fCap = VMMDEV_GUEST_SUPPORTS_GRAPHICS;
    pConsole->aCaps[VBOXCAPS_ENTRY_IDX_GRAPHICS].iCap = VBOXCAPS_ENTRY_IDX_GRAPHICS;
    return VINF_SUCCESS;
}

static int VBoxCapsReleaseAll()
{
    VBOXCAPS *pConsole = &gVBoxCaps;
    Log(("VBoxCapsReleaseAll\n"));
    int rc = VBoxAcquireGuestCaps(0, VMMDEV_GUEST_SUPPORTS_SEAMLESS | VMMDEV_GUEST_SUPPORTS_GRAPHICS, false);
    if (!RT_SUCCESS(rc))
    {
        LogFlowFunc(("vboxCapsEntryReleaseAll VBoxAcquireGuestCaps failed rc %d\n", rc));
        return rc;
    }

    if (pConsole->idTimer)
    {
        Log(("killing console timer\n"));
        KillTimer(g_hwndToolWindow, pConsole->idTimer);
        pConsole->idTimer = 0;
    }

    for (int i = 0; i < RT_ELEMENTS(pConsole->aCaps); ++i)
    {
        vboxCapsEntryAcStateSet(&pConsole->aCaps[i], VBOXCAPS_ENTRY_ACSTATE_RELEASED);
    }

    return rc;
}

static void VBoxCapsTerm()
{
    VBOXCAPS *pConsole = &gVBoxCaps;
    VBoxCapsReleaseAll();
    memset(pConsole, 0, sizeof (*pConsole));
}

static BOOL VBoxCapsEntryIsAcquired(uint32_t iCap)
{
    VBOXCAPS *pConsole = &gVBoxCaps;
    return pConsole->aCaps[iCap].enmAcState == VBOXCAPS_ENTRY_ACSTATE_ACQUIRED;
}

static BOOL VBoxCapsEntryIsEnabled(uint32_t iCap)
{
    VBOXCAPS *pConsole = &gVBoxCaps;
    return pConsole->aCaps[iCap].enmAcState == VBOXCAPS_ENTRY_ACSTATE_ACQUIRED
            && pConsole->aCaps[iCap].enmFuncState == VBOXCAPS_ENTRY_FUNCSTATE_STARTED;
}

static BOOL VBoxCapsCheckTimer(WPARAM wParam)
{
    VBOXCAPS *pConsole = &gVBoxCaps;
    if (wParam != pConsole->idTimer)
        return FALSE;

    uint32_t u32AcquiredCaps = 0;
    BOOL fNeedNewTimer = FALSE;

    for (int i = 0; i < RT_ELEMENTS(pConsole->aCaps); ++i)
    {
        VBOXCAPS_ENTRY *pCap = &pConsole->aCaps[i];
        if (pCap->enmAcState != VBOXCAPS_ENTRY_ACSTATE_ACQUIRING)
            continue;

        int rc = VBoxAcquireGuestCaps(pCap->fCap, 0, false);
        if (RT_SUCCESS(rc))
        {
            vboxCapsEntryAcStateSet(&pConsole->aCaps[i], VBOXCAPS_ENTRY_ACSTATE_ACQUIRED);
            u32AcquiredCaps |= pCap->fCap;
        }
        else
        {
            Assert(rc == VERR_RESOURCE_BUSY);
            fNeedNewTimer = TRUE;
        }
    }

    if (!fNeedNewTimer)
    {
        KillTimer(g_hwndToolWindow, pConsole->idTimer);
        /* cleanup timer data */
        pConsole->idTimer = 0;
    }

    return TRUE;
}

static int VBoxCapsEntryRelease(uint32_t iCap)
{
    VBOXCAPS *pConsole = &gVBoxCaps;
    VBOXCAPS_ENTRY *pCap = &pConsole->aCaps[iCap];
    if (pCap->enmAcState == VBOXCAPS_ENTRY_ACSTATE_RELEASED)
    {
        LogFlowFunc(("invalid cap[%d] state[%d] on release\n", iCap, pCap->enmAcState));
        return VERR_INVALID_STATE;
    }

    if (pCap->enmAcState == VBOXCAPS_ENTRY_ACSTATE_ACQUIRED)
    {
        int rc = VBoxAcquireGuestCaps(0, pCap->fCap, false);
        AssertRC(rc);
    }

    vboxCapsEntryAcStateSet(pCap, VBOXCAPS_ENTRY_ACSTATE_RELEASED);

    return VINF_SUCCESS;
}

static int VBoxCapsEntryAcquire(uint32_t iCap)
{
    VBOXCAPS *pConsole = &gVBoxCaps;
    Assert(VBoxConsoleIsAllowed());
    VBOXCAPS_ENTRY *pCap = &pConsole->aCaps[iCap];
    Log(("VBoxCapsEntryAcquire %d\n", iCap));
    if (pCap->enmAcState != VBOXCAPS_ENTRY_ACSTATE_RELEASED)
    {
        LogFlowFunc(("invalid cap[%d] state[%d] on acquire\n", iCap, pCap->enmAcState));
        return VERR_INVALID_STATE;
    }

    vboxCapsEntryAcStateSet(pCap, VBOXCAPS_ENTRY_ACSTATE_ACQUIRING);
    int rc = VBoxAcquireGuestCaps(pCap->fCap, 0, false);
    if (RT_SUCCESS(rc))
    {
        vboxCapsEntryAcStateSet(pCap, VBOXCAPS_ENTRY_ACSTATE_ACQUIRED);
        return VINF_SUCCESS;
    }

    if (rc != VERR_RESOURCE_BUSY)
    {
        LogFlowFunc(("vboxCapsEntryReleaseAll VBoxAcquireGuestCaps failed rc %d\n", rc));
        return rc;
    }

    LogFlowFunc(("iCap %d is busy!\n", iCap));

    /* the cap was busy, most likely it is still used by other VBoxTray instance running in another session,
     * queue the retry timer */
    if (!pConsole->idTimer)
    {
        pConsole->idTimer = SetTimer(g_hwndToolWindow, TIMERID_VBOXTRAY_CAPS_TIMER, 100, (TIMERPROC)NULL);
        if (!pConsole->idTimer)
        {
            DWORD dwErr = GetLastError();
            LogFlowFunc(("SetTimer error %08X\n", dwErr));
            return RTErrConvertFromWin32(dwErr);
        }
    }

    return rc;
}

static int VBoxCapsAcquireAllSupported()
{
    VBOXCAPS *pConsole = &gVBoxCaps;
    Log(("VBoxCapsAcquireAllSupported\n"));
    for (int i = 0; i < RT_ELEMENTS(pConsole->aCaps); ++i)
    {
        if (pConsole->aCaps[i].enmFuncState >= VBOXCAPS_ENTRY_FUNCSTATE_SUPPORTED)
        {
            Log(("VBoxCapsAcquireAllSupported acquiring cap %d, state %d\n", i, pConsole->aCaps[i].enmFuncState));
            VBoxCapsEntryAcquire(i);
        }
        else
        {
            LogFlowFunc(("VBoxCapsAcquireAllSupported: WARN: cap %d not supported, state %d\n", i, pConsole->aCaps[i].enmFuncState));
        }
    }
    return VINF_SUCCESS;
}

static BOOL VBoxConsoleIsAllowed()
{
    return vboxDtIsInputDesktop() && vboxStIsActiveConsole();
}

static void VBoxConsoleEnable(BOOL fEnable)
{
    if (fEnable)
        VBoxCapsAcquireAllSupported();
    else
        VBoxCapsReleaseAll();
}

static void VBoxConsoleCapSetSupported(uint32_t iCap, BOOL fSupported)
{
    if (fSupported)
    {
        VBoxCapsEntryFuncStateSet(iCap, VBOXCAPS_ENTRY_FUNCSTATE_SUPPORTED);

        if (VBoxConsoleIsAllowed())
            VBoxCapsEntryAcquire(iCap);
    }
    else
    {
        VBoxCapsEntryFuncStateSet(iCap, VBOXCAPS_ENTRY_FUNCSTATE_UNSUPPORTED);

        VBoxCapsEntryRelease(iCap);
    }
}

void VBoxSeamlessSetSupported(BOOL fSupported)
{
    VBoxConsoleCapSetSupported(VBOXCAPS_ENTRY_IDX_SEAMLESS, fSupported);
}

static void VBoxGrapicsSetSupported(BOOL fSupported)
{
    VBoxConsoleCapSetSupported(VBOXCAPS_ENTRY_IDX_GRAPHICS, fSupported);
}
