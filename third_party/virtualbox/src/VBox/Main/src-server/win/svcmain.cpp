/* $Id: svcmain.cpp $ */
/** @file
 *
 * SVCMAIN - COM out-of-proc server main entry
 */

/*
 * Copyright (C) 2004-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <iprt/win/windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "VBox/com/defs.h"
#include "VBox/com/com.h"
#include "VBox/com/VirtualBox.h"

#include "VirtualBoxImpl.h"
#include "Logging.h"

#include "svchlp.h"

#include <VBox/err.h>
#include <iprt/buildconfig.h>
#include <iprt/initterm.h>
#include <iprt/string.h>
#include <iprt/uni.h>
#include <iprt/path.h>
#include <iprt/getopt.h>
#include <iprt/message.h>
#include <iprt/asm.h>

class CExeModule : public ATL::CComModule
{
public:
    LONG Unlock();
    DWORD dwThreadID;
    HANDLE hEventShutdown;
    void MonitorShutdown();
    bool StartMonitor();
    bool HasActiveConnection();
    bool bActivity;
};

/* Normal timeout usually used in Shutdown Monitor */
const DWORD dwNormalTimeout = 5000;
volatile uint32_t dwTimeOut = dwNormalTimeout; /* time for EXE to be idle before shutting down. Can be decreased at system shutdown phase. */

/* Passed to CreateThread to monitor the shutdown event */
static DWORD WINAPI MonitorProc(void* pv)
{
    CExeModule* p = (CExeModule*)pv;
    p->MonitorShutdown();
    return 0;
}

LONG CExeModule::Unlock()
{
    LONG l = ATL::CComModule::Unlock();
    if (l == 0)
    {
        bActivity = true;
        SetEvent(hEventShutdown); /* tell monitor that we transitioned to zero */
    }
    return l;
}

bool CExeModule::HasActiveConnection()
{
    return bActivity || GetLockCount() > 0;
}

/* Monitors the shutdown event */
void CExeModule::MonitorShutdown()
{
    while (1)
    {
        WaitForSingleObject(hEventShutdown, INFINITE);
        DWORD dwWait=0;
        do
        {
            bActivity = false;
            dwWait = WaitForSingleObject(hEventShutdown, dwTimeOut);
        } while (dwWait == WAIT_OBJECT_0);
        /* timed out */
        if (!HasActiveConnection()) /* if no activity let's really bail */
        {
            /* Disable log rotation at this point, worst case a log file
             * becomes slightly bigger than it should. Avoids quirks with
             * log rotation: there might be another API service process
             * running at this point which would rotate the logs concurrently,
             * creating a mess. */
            PRTLOGGER pReleaseLogger = RTLogRelGetDefaultInstance();
            if (pReleaseLogger)
            {
                char szDest[1024];
                int rc = RTLogGetDestinations(pReleaseLogger, szDest, sizeof(szDest));
                if (RT_SUCCESS(rc))
                {
                    rc = RTStrCat(szDest, sizeof(szDest), " nohistory");
                    if (RT_SUCCESS(rc))
                    {
                        rc = RTLogDestinations(pReleaseLogger, szDest);
                        AssertRC(rc);
                    }
                }
            }
#if _WIN32_WINNT >= 0x0400
            CoSuspendClassObjects();
            if (!HasActiveConnection())
#endif
                break;
        }
    }
    CloseHandle(hEventShutdown);
    PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
}

bool CExeModule::StartMonitor()
{
    hEventShutdown = CreateEvent(NULL, false, false, NULL);
    if (hEventShutdown == NULL)
        return false;
    DWORD dwThreadID;
    HANDLE h = CreateThread(NULL, 0, MonitorProc, this, 0, &dwThreadID);
    return (h != NULL);
}


BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_VirtualBox, VirtualBox)
END_OBJECT_MAP()

CExeModule* g_pModule = NULL;
HWND g_hMainWindow = NULL;
HINSTANCE g_hInstance = NULL;
#define MAIN_WND_CLASS L"VirtualBox Interface"

/*
* Wrapper for Win API function ShutdownBlockReasonCreate
* This function defined starting from Vista only.
*/
static BOOL ShutdownBlockReasonCreateAPI(HWND hWnd, LPCWSTR pwszReason)
{
    BOOL fResult = FALSE;
    typedef BOOL(WINAPI *PFNSHUTDOWNBLOCKREASONCREATE)(HWND hWnd, LPCWSTR pwszReason);

    PFNSHUTDOWNBLOCKREASONCREATE pfn = (PFNSHUTDOWNBLOCKREASONCREATE)GetProcAddress(
            GetModuleHandle(L"User32.dll"), "ShutdownBlockReasonCreate");
    AssertPtr(pfn);
    if (pfn)
        fResult = pfn(hWnd, pwszReason);
    return fResult;
}

/*
* Wrapper for Win API function ShutdownBlockReasonDestroy
* This function defined starting from Vista only.
*/
static BOOL ShutdownBlockReasonDestroyAPI(HWND hWnd)
{
    BOOL fResult = FALSE;
    typedef BOOL(WINAPI *PFNSHUTDOWNBLOCKREASONDESTROY)(HWND hWnd);

    PFNSHUTDOWNBLOCKREASONDESTROY pfn = (PFNSHUTDOWNBLOCKREASONDESTROY)GetProcAddress(
        GetModuleHandle(L"User32.dll"), "ShutdownBlockReasonDestroy");
    AssertPtr(pfn);
    if (pfn)
        fResult = pfn(hWnd);
    return fResult;
}

static LRESULT CALLBACK WinMainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT rc = 0;
    switch (msg)
    {
        case WM_QUERYENDSESSION:
        {
            if (g_pModule)
            {
                bool fActiveConnection = g_pModule->HasActiveConnection();
                if (fActiveConnection)
                {
                    /* place the VBoxSVC into system shutdown list */
                    ShutdownBlockReasonCreateAPI(hwnd, L"Has active connections.");
                    /* decrease a latency of MonitorShutdown loop */
                    ASMAtomicXchgU32(&dwTimeOut, 100);
                    Log(("VBoxSVCWinMain: WM_QUERYENDSESSION: VBoxSvc has active connections. bActivity = %d. Loc count = %d\n",
                         g_pModule->bActivity, g_pModule->GetLockCount()));
                }
                rc = !fActiveConnection;
            }
            else
                AssertMsgFailed(("VBoxSVCWinMain: WM_QUERYENDSESSION: Error: g_pModule is NULL"));
            break;
        }
        case WM_ENDSESSION:
        {
            /* Restore timeout of Monitor Shutdown if user canceled system shutdown */
            if (wParam == FALSE)
            {
                ASMAtomicXchgU32(&dwTimeOut, dwNormalTimeout);
                Log(("VBoxSVCWinMain: user canceled system shutdown.\n"));
            }
            break;
        }
        case WM_DESTROY:
        {
            ShutdownBlockReasonDestroyAPI(hwnd);
            PostQuitMessage(0);
            break;
        }
        default:
        {
            rc = DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }
    return rc;
}

static int CreateMainWindow()
{
    int rc = VINF_SUCCESS;
    Assert(g_hMainWindow == NULL);

    LogFlow(("CreateMainWindow\n"));

    g_hInstance = (HINSTANCE)GetModuleHandle(NULL);

    /* Register the Window Class. */
    WNDCLASS wc;
    RT_ZERO(wc);

    wc.style = CS_NOCLOSE;
    wc.lpfnWndProc = WinMainWndProc;
    wc.hInstance = g_hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
    wc.lpszClassName = MAIN_WND_CLASS;

    ATOM atomWindowClass = RegisterClass(&wc);
    if (atomWindowClass == 0)
    {
        Log(("Failed to register main window class\n"));
        rc = VERR_NOT_SUPPORTED;
    }
    else
    {
        /* Create the window. */
        g_hMainWindow = CreateWindowEx(WS_EX_TOOLWINDOW |  WS_EX_TOPMOST,
                                       MAIN_WND_CLASS, MAIN_WND_CLASS,
                                       WS_POPUPWINDOW,
                                       0, 0, 1, 1, NULL, NULL, g_hInstance, NULL);
        if (g_hMainWindow == NULL)
        {
            Log(("Failed to create main window\n"));
            rc = VERR_NOT_SUPPORTED;
        }
        else
        {
            SetWindowPos(g_hMainWindow, HWND_TOPMOST, -200, -200, 0, 0,
                         SWP_NOACTIVATE | SWP_HIDEWINDOW | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOSIZE);

        }
    }
    return 0;
}


static void DestroyMainWindow()
{
    Assert(g_hMainWindow != NULL);
    Log(("SVCMain: DestroyMainWindow \n"));
    if (g_hMainWindow != NULL)
    {
        DestroyWindow(g_hMainWindow);
        g_hMainWindow = NULL;
        if (g_hInstance != NULL)
        {
            UnregisterClass(MAIN_WND_CLASS, g_hInstance);
            g_hInstance = NULL;
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nShowCmd*/)
{
    int    argc = __argc;
    char **argv = __argv;

    /*
     * Need to parse the command line before initializing the VBox runtime so we can
     * change to the user home directory before logs are being created.
     */
    for (int i = 1; i < argc; i++)
        if (   (argv[i][0] == '/' || argv[i][0] == '-')
            && stricmp(&argv[i][1], "embedding") == 0) /* ANSI */
        {
            /* %HOMEDRIVE%%HOMEPATH% */
            wchar_t wszHome[RTPATH_MAX];
            DWORD cEnv = GetEnvironmentVariable(L"HOMEDRIVE", &wszHome[0], RTPATH_MAX);
            if (cEnv && cEnv < RTPATH_MAX)
            {
                DWORD cwc = cEnv; /* doesn't include NUL */
                cEnv = GetEnvironmentVariable(L"HOMEPATH", &wszHome[cEnv], RTPATH_MAX - cwc);
                if (cEnv && cEnv < RTPATH_MAX - cwc)
                {
                    /* If this fails there is nothing we can do. Ignore. */
                    SetCurrentDirectory(wszHome);
                }
            }
        }

    /*
     * Initialize the VBox runtime without loading
     * the support driver.
     */
    RTR3InitExe(argc, &argv, 0);

    static const RTGETOPTDEF s_aOptions[] =
    {
        { "--embedding",    'e',    RTGETOPT_REQ_NOTHING | RTGETOPT_FLAG_ICASE },
        { "-embedding",     'e',    RTGETOPT_REQ_NOTHING | RTGETOPT_FLAG_ICASE },
        { "/embedding",     'e',    RTGETOPT_REQ_NOTHING | RTGETOPT_FLAG_ICASE },
        { "--unregserver",  'u',    RTGETOPT_REQ_NOTHING | RTGETOPT_FLAG_ICASE },
        { "-unregserver",   'u',    RTGETOPT_REQ_NOTHING | RTGETOPT_FLAG_ICASE },
        { "/unregserver",   'u',    RTGETOPT_REQ_NOTHING | RTGETOPT_FLAG_ICASE },
        { "--regserver",    'r',    RTGETOPT_REQ_NOTHING | RTGETOPT_FLAG_ICASE },
        { "-regserver",     'r',    RTGETOPT_REQ_NOTHING | RTGETOPT_FLAG_ICASE },
        { "/regserver",     'r',    RTGETOPT_REQ_NOTHING | RTGETOPT_FLAG_ICASE },
        { "--reregserver",  'f',    RTGETOPT_REQ_NOTHING | RTGETOPT_FLAG_ICASE },
        { "-reregserver",   'f',    RTGETOPT_REQ_NOTHING | RTGETOPT_FLAG_ICASE },
        { "/reregserver",   'f',    RTGETOPT_REQ_NOTHING | RTGETOPT_FLAG_ICASE },
        { "--helper",       'H',    RTGETOPT_REQ_STRING | RTGETOPT_FLAG_ICASE },
        { "-helper",        'H',    RTGETOPT_REQ_STRING | RTGETOPT_FLAG_ICASE },
        { "/helper",        'H',    RTGETOPT_REQ_STRING | RTGETOPT_FLAG_ICASE },
        { "--logfile",      'F',    RTGETOPT_REQ_STRING | RTGETOPT_FLAG_ICASE },
        { "-logfile",       'F',    RTGETOPT_REQ_STRING | RTGETOPT_FLAG_ICASE },
        { "/logfile",       'F',    RTGETOPT_REQ_STRING | RTGETOPT_FLAG_ICASE },
        { "--logrotate",    'R',    RTGETOPT_REQ_UINT32 | RTGETOPT_FLAG_ICASE },
        { "-logrotate",     'R',    RTGETOPT_REQ_UINT32 | RTGETOPT_FLAG_ICASE },
        { "/logrotate",     'R',    RTGETOPT_REQ_UINT32 | RTGETOPT_FLAG_ICASE },
        { "--logsize",      'S',    RTGETOPT_REQ_UINT64 | RTGETOPT_FLAG_ICASE },
        { "-logsize",       'S',    RTGETOPT_REQ_UINT64 | RTGETOPT_FLAG_ICASE },
        { "/logsize",       'S',    RTGETOPT_REQ_UINT64 | RTGETOPT_FLAG_ICASE },
        { "--loginterval",  'I',    RTGETOPT_REQ_UINT32 | RTGETOPT_FLAG_ICASE },
        { "-loginterval",   'I',    RTGETOPT_REQ_UINT32 | RTGETOPT_FLAG_ICASE },
        { "/loginterval",   'I',    RTGETOPT_REQ_UINT32 | RTGETOPT_FLAG_ICASE },
    };

    bool            fRun = true;
    bool            fRegister = false;
    bool            fUnregister = false;
    const char      *pszPipeName = NULL;
    const char      *pszLogFile = NULL;
    uint32_t        cHistory = 10;                  // enable log rotation, 10 files
    uint32_t        uHistoryFileTime = RT_SEC_1DAY; // max 1 day per file
    uint64_t        uHistoryFileSize = 100 * _1M;   // max 100MB per file

    RTGETOPTSTATE   GetOptState;
    int vrc = RTGetOptInit(&GetOptState, argc, argv, &s_aOptions[0], RT_ELEMENTS(s_aOptions), 1, 0 /*fFlags*/);
    AssertRC(vrc);

    RTGETOPTUNION   ValueUnion;
    while ((vrc = RTGetOpt(&GetOptState, &ValueUnion)))
    {
        switch (vrc)
        {
            case 'e':
                /* already handled above */
                break;

            case 'u':
                fUnregister = true;
                fRun = false;
                break;

            case 'r':
                fRegister = true;
                fRun = false;
                break;

            case 'f':
                fUnregister = true;
                fRegister = true;
                fRun = false;
                break;

            case 'H':
                pszPipeName = ValueUnion.psz;
                if (!pszPipeName)
                    pszPipeName = "";
                fRun = false;
                break;

            case 'F':
                pszLogFile = ValueUnion.psz;
                break;

            case 'R':
                cHistory = ValueUnion.u32;
                break;

            case 'S':
                uHistoryFileSize = ValueUnion.u64;
                break;

            case 'I':
                uHistoryFileTime = ValueUnion.u32;
                break;

            case 'h':
            {
                TCHAR txt[]= L"Options:\n\n"
                             L"/RegServer:\tregister COM out-of-proc server\n"
                             L"/UnregServer:\tunregister COM out-of-proc server\n"
                             L"/ReregServer:\tunregister and register COM server\n"
                             L"no options:\trun the server";
                TCHAR title[]=_T("Usage");
                fRun = false;
                MessageBox(NULL, txt, title, MB_OK);
                return 0;
            }

            case 'V':
            {
                char *psz = NULL;
                RTStrAPrintf(&psz, "%sr%s\n", RTBldCfgVersion(), RTBldCfgRevisionStr());
                PRTUTF16 txt = NULL;
                RTStrToUtf16(psz, &txt);
                TCHAR title[]=_T("Version");
                fRun = false;
                MessageBox(NULL, txt, title, MB_OK);
                RTStrFree(psz);
                RTUtf16Free(txt);
                return 0;
            }

            default:
                /** @todo this assumes that stderr is visible, which is not
                 * true for standard Windows applications. */
                /* continue on command line errors... */
                RTGetOptPrintError(vrc, &ValueUnion);
        }
    }

    /* Only create the log file when running VBoxSVC normally, but not when
     * registering/unregistering or calling the helper functionality. */
    if (fRun)
    {
        /** @todo Merge this code with server.cpp (use Logging.cpp?). */
        char szLogFile[RTPATH_MAX];
        if (!pszLogFile)
        {
            vrc = com::GetVBoxUserHomeDirectory(szLogFile, sizeof(szLogFile));
            if (RT_SUCCESS(vrc))
                vrc = RTPathAppend(szLogFile, sizeof(szLogFile), "VBoxSVC.log");
        }
        else
        {
            if (!RTStrPrintf(szLogFile, sizeof(szLogFile), "%s", pszLogFile))
                vrc = VERR_NO_MEMORY;
        }
        if (RT_FAILURE(vrc))
            return RTMsgErrorExit(RTEXITCODE_FAILURE, "failed to create logging file name, rc=%Rrc", vrc);

        char szError[RTPATH_MAX + 128];
        vrc = com::VBoxLogRelCreate("COM Server", szLogFile,
                                    RTLOGFLAGS_PREFIX_THREAD | RTLOGFLAGS_PREFIX_TIME_PROG,
                                    VBOXSVC_LOG_DEFAULT, "VBOXSVC_RELEASE_LOG",
                                    RTLOGDEST_FILE, UINT32_MAX /* cMaxEntriesPerGroup */,
                                    cHistory, uHistoryFileTime, uHistoryFileSize,
                                    szError, sizeof(szError));
        if (RT_FAILURE(vrc))
            return RTMsgErrorExit(RTEXITCODE_FAILURE, "failed to open release log (%s, %Rrc)", szError, vrc);
    }

    /* Set up a build identifier so that it can be seen from core dumps what
     * exact build was used to produce the core. Same as in Console::i_powerUpThread(). */
    static char saBuildID[48];
    RTStrPrintf(saBuildID, sizeof(saBuildID), "%s%s%s%s VirtualBox %s r%u %s%s%s%s",
                "BU", "IL", "DI", "D", RTBldCfgVersion(), RTBldCfgRevision(), "BU", "IL", "DI", "D");

    int nRet = 0;
    HRESULT hRes = com::Initialize(false /*fGui*/, fRun /*fAutoRegUpdate*/);
    AssertLogRelMsg(SUCCEEDED(hRes), ("SVCMAIN: init failed: %Rhrc\n", hRes));

    g_pModule = new CExeModule();
    if(g_pModule == NULL)
        return RTMsgErrorExit(RTEXITCODE_FAILURE, "not enough memory to create ExeModule.");
    g_pModule->Init(ObjectMap, hInstance, &LIBID_VirtualBox);
    g_pModule->dwThreadID = GetCurrentThreadId();

    if (!fRun)
    {
#ifndef VBOX_WITH_MIDL_PROXY_STUB /* VBoxProxyStub.dll does all the registration work. */
        if (fUnregister)
        {
            g_pModule->UpdateRegistryFromResource(IDR_VIRTUALBOX, FALSE);
            nRet = g_pModule->UnregisterServer(TRUE);
        }
        if (fRegister)
        {
            g_pModule->UpdateRegistryFromResource(IDR_VIRTUALBOX, TRUE);
            nRet = g_pModule->RegisterServer(TRUE);
        }
#endif
        if (pszPipeName)
        {
            Log(("SVCMAIN: Processing Helper request (cmdline=\"%s\")...\n", pszPipeName));

            if (!*pszPipeName)
                vrc = VERR_INVALID_PARAMETER;

            if (RT_SUCCESS(vrc))
            {
                /* do the helper job */
                SVCHlpServer server;
                vrc = server.open(pszPipeName);
                if (RT_SUCCESS(vrc))
                    vrc = server.run();
            }
            if (RT_FAILURE(vrc))
            {
                Log(("SVCMAIN: Failed to process Helper request (%Rrc).", vrc));
                nRet = 1;
            }
        }
    }
    else
    {
        g_pModule->StartMonitor();
#if _WIN32_WINNT >= 0x0400
        hRes = g_pModule->RegisterClassObjects(CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE | REGCLS_SUSPENDED);
        _ASSERTE(SUCCEEDED(hRes));
        hRes = CoResumeClassObjects();
#else
        hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE);
#endif
        _ASSERTE(SUCCEEDED(hRes));

        if (RT_SUCCESS(CreateMainWindow()))
            Log(("SVCMain: Main window succesfully created\n"));
        else
            Log(("SVCMain: Failed to create main window\n"));

        MSG msg;
        while (GetMessage(&msg, 0, 0, 0) > 0)
        {
            DispatchMessage(&msg);
            TranslateMessage(&msg);
        }

        DestroyMainWindow();

        g_pModule->RevokeClassObjects();
    }

    g_pModule->Term();

    com::Shutdown();

    if(g_pModule)
        delete g_pModule;
    g_pModule = NULL;

    Log(("SVCMAIN: Returning, COM server process ends.\n"));
    return nRet;
}
