/* $Id: VBoxGuestInstallHelper.cpp $ */
/** @file
 * VBoxGuestInstallHelper - Various helper routines for Windows guest installer.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
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
#include <stdlib.h>
#include <tchar.h>
#include <strsafe.h>
#pragma warning(push)
#pragma warning(disable: 4995) /* warning C4995: 'lstrcpyA': name was marked as #pragma deprecated */
#include "exdll.h"
#pragma warning(pop)

#include <iprt/err.h>
#include <iprt/initterm.h>
#include <iprt/ldr.h>
#include <iprt/localipc.h>
#include <iprt/mem.h>
#include <iprt/process.h>
#include <iprt/string.h>

/* Required structures/defines of VBoxTray. */
#include "../../VBoxTray/VBoxTrayMsg.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#define VBOXINSTALLHELPER_EXPORT extern "C" void __declspec(dllexport)


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
typedef DWORD (WINAPI *PFNSFCFILEEXCEPTION)(DWORD param1, PWCHAR param2, DWORD param3);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
HINSTANCE               g_hInstance;
HWND                    g_hwndParent;
PFNSFCFILEEXCEPTION     g_pfnSfcFileException = NULL;

/**
 * @todo Clean up this DLL, use more IPRT in here!
 */

/**
 * Pops (gets) a value from the internal NSIS stack.
 * Since the supplied popstring() method easily can cause buffer
 * overflows, use vboxPopString() instead!
 *
 * @return  HRESULT
 * @param   pszDest     Pointer to pre-allocated string to store result.
 * @param   cchDest     Size (in characters) of pre-allocated string.
 */
static HRESULT vboxPopString(TCHAR *pszDest, size_t cchDest)
{
    HRESULT hr = S_OK;
    if (!g_stacktop || !*g_stacktop)
        hr = __HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    else
    {
        stack_t *pStack = (*g_stacktop);
        if (pStack)
        {
            hr = StringCchCopy(pszDest, cchDest, pStack->text);
            if (SUCCEEDED(hr))
            {
                *g_stacktop = pStack->next;
                GlobalFree((HGLOBAL)pStack);
            }
        }
        else
            hr = __HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }
    return hr;
}

static HRESULT vboxPopULong(PULONG pulValue)
{
    HRESULT hr = S_OK;
    if (!g_stacktop || !*g_stacktop)
        hr = __HRESULT_FROM_WIN32(ERROR_EMPTY);
    else
    {
        stack_t *pStack = (*g_stacktop);
        if (pStack)
        {
            *pulValue = strtoul(pStack->text, NULL, 10 /* Base */);

            *g_stacktop = pStack->next;
            GlobalFree((HGLOBAL)pStack);
        }
    }
    return hr;
}

static void vboxPushResultAsString(HRESULT hr)
{
    TCHAR szErr[MAX_PATH + 1];
    if (FAILED(hr))
    {
        if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, 0, szErr, MAX_PATH, NULL))
            szErr[MAX_PATH] = '\0';
        else
            StringCchPrintf(szErr, sizeof(szErr),
                            "FormatMessage failed! Error = %ld", GetLastError());
    }
    else
        StringCchPrintf(szErr, sizeof(szErr), "0");
    pushstring(szErr);
}

/**
 * Connects to VBoxTray IPC under the behalf of the user running
 * in the current thread context.
 *
 * @return  IPRT status code.
 * @param   phSession               Where to store the IPC session.
 */
static int vboxConnectToVBoxTray(RTLOCALIPCSESSION *phSession)
{
    char szPipeName[512 + sizeof(VBOXTRAY_IPC_PIPE_PREFIX)];
    memcpy(szPipeName, VBOXTRAY_IPC_PIPE_PREFIX, sizeof(VBOXTRAY_IPC_PIPE_PREFIX));
    int rc = RTProcQueryUsername(NIL_RTPROCESS,
                                 &szPipeName[sizeof(VBOXTRAY_IPC_PIPE_PREFIX) - 1],
                                 sizeof(szPipeName) - sizeof(VBOXTRAY_IPC_PIPE_PREFIX) + 1,
                                 NULL /*pcbUser*/);
    if (RT_SUCCESS(rc))
        rc = RTLocalIpcSessionConnect(phSession, szPipeName, RTLOCALIPC_FLAGS_NATIVE_NAME);
    return rc;
}

static void vboxChar2WCharFree(PWCHAR pwString)
{
    if (pwString)
        HeapFree(GetProcessHeap(), 0, pwString);
}

static HRESULT vboxChar2WCharAlloc(const char *pszString, PWCHAR *ppwString)
{
    HRESULT hr;
    int iLen = strlen(pszString) + 2;
    WCHAR *pwString = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, iLen * sizeof(WCHAR));
    if (!pwString)
        hr = __HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    else
    {
        if (MultiByteToWideChar(CP_ACP, 0, pszString, -1, pwString, iLen) == 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            HeapFree(GetProcessHeap(), 0, pwString);
        }
        else
        {
            hr = S_OK;
            *ppwString = pwString;
        }
    }
    return hr;
}

/**
 * Loads a system DLL.
 *
 * @returns Module handle or NULL
 * @param   pszName             The DLL name.
 */
static HMODULE loadSystemDll(const char *pszName)
{
    char   szPath[MAX_PATH];
    UINT   cchPath = GetSystemDirectoryA(szPath, sizeof(szPath));
    size_t cbName  = strlen(pszName) + 1;
    if (cchPath + 1 + cbName > sizeof(szPath))
        return NULL;
    szPath[cchPath] = '\\';
    memcpy(&szPath[cchPath + 1], pszName, cbName);
    return LoadLibraryA(szPath);
}

/**
 * Disables the Windows File Protection for a specified file
 * using an undocumented SFC API call. Don't try this at home!
 *
 * @param   hwndParent          Window handle of parent.
 * @param   string_size         Size of variable string.
 * @param   variables           The actual variable string.
 * @param   stacktop            Pointer to a pointer to the current stack.
 */
VBOXINSTALLHELPER_EXPORT DisableWFP(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop)
{
    NOREF(hwndParent);
    EXDLL_INIT();

    TCHAR szFile[MAX_PATH + 1];
    HRESULT hr = vboxPopString(szFile, sizeof(szFile) / sizeof(TCHAR));
    if (SUCCEEDED(hr))
    {
        HMODULE hSFC = loadSystemDll("sfc_os.dll"); /** @todo Replace this by RTLdr APIs. */
        if (NULL != hSFC)
        {
            g_pfnSfcFileException = (PFNSFCFILEEXCEPTION)GetProcAddress(hSFC, "SfcFileException");
            if (g_pfnSfcFileException == NULL)
            {
                /* If we didn't get the proc address with the call above, try it harder with
                 * the (zero based) index of the function list. */
                g_pfnSfcFileException = (PFNSFCFILEEXCEPTION)GetProcAddress(hSFC, (LPCSTR)5);
                if (g_pfnSfcFileException == NULL)
                    hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
            }
        }
        else
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

        if (SUCCEEDED(hr))
        {
            WCHAR *pwszFile;
            hr = vboxChar2WCharAlloc(szFile, &pwszFile);
            if (SUCCEEDED(hr))
            {
                if (g_pfnSfcFileException(0, pwszFile, UINT32_MAX) != 0)
                    hr = HRESULT_FROM_WIN32(GetLastError());
                vboxChar2WCharFree(pwszFile);
            }
        }

        if (hSFC)
            FreeLibrary(hSFC);
    }

    vboxPushResultAsString(hr);
}

/**
 * Retrieves a file's architecture (x86 or amd64).
 * Outputs "x86", "amd64" or an error message (if not found/invalid) on stack.
 *
 * @param   hwndParent          Window handle of parent.
 * @param   string_size         Size of variable string.
 * @param   variables           The actual variable string.
 * @param   stacktop            Pointer to a pointer to the current stack.
 */
VBOXINSTALLHELPER_EXPORT FileGetArchitecture(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop)
{
    NOREF(hwndParent);
    EXDLL_INIT();

    TCHAR szFile[MAX_PATH + 1];
    HRESULT hr = vboxPopString(szFile, sizeof(szFile) / sizeof(TCHAR));
    if (SUCCEEDED(hr))
    {
        RTLDRMOD hLdrMod;
        int rc = RTLdrOpen(szFile, RTLDR_O_FOR_VALIDATION, RTLDRARCH_WHATEVER, &hLdrMod);
        if (RT_SUCCESS(rc))
        {
            if (RTLdrGetFormat(hLdrMod) == RTLDRFMT_PE)
            {
                RTLDRARCH enmLdrArch = RTLdrGetArch(hLdrMod);
                switch (enmLdrArch)
                {
                    case RTLDRARCH_X86_32:
                        pushstring("x86");
                        break;

                    case RTLDRARCH_AMD64:
                        pushstring("amd64");
                        break;

                    default:
                        pushstring("Error: Unknown / invalid architecture");
                        break;
                }
            }
            else
                pushstring("Error: Unknown / invalid PE signature");

            RTLdrClose(hLdrMod);
        }
        else
        {
            char szMsg[64];
            RTStrPrintf(szMsg, sizeof(szMsg), "Error: Could not open file: %Rrc", rc);

            pushstring(szMsg);
        }
    }
    else
        vboxPushResultAsString(hr);
}

/**
 * Retrieves a file's vendor.
 * Outputs the vendor's name or an error message (if not found/invalid) on stack.
 *
 * @param   hwndParent          Window handle of parent.
 * @param   string_size         Size of variable string.
 * @param   variables           The actual variable string.
 * @param   stacktop            Pointer to a pointer to the current stack.
 */
VBOXINSTALLHELPER_EXPORT FileGetVendor(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop)
{
    NOREF(hwndParent);
    EXDLL_INIT();

    TCHAR szFile[MAX_PATH + 1];
    HRESULT hr = vboxPopString(szFile, sizeof(szFile) / sizeof(TCHAR));
    if (SUCCEEDED(hr))
    {
        DWORD dwInfoSize = GetFileVersionInfoSize(szFile, NULL /* lpdwHandle */);
        if (dwInfoSize)
        {
            void *pFileInfo = GlobalAlloc(GMEM_FIXED, dwInfoSize);
            if (pFileInfo)
            {
                if (GetFileVersionInfo(szFile, 0, dwInfoSize, pFileInfo))
                {
                    LPVOID pvInfo;
                    UINT puInfoLen;
                    if (VerQueryValue(pFileInfo, _T("\\VarFileInfo\\Translation"),
                                      &pvInfo, &puInfoLen))
                    {
                        WORD wCodePage = LOWORD(*(DWORD*)pvInfo);
                        WORD wLanguageID = HIWORD(*(DWORD*)pvInfo);

                        TCHAR szQuery[MAX_PATH];
#pragma warning(suppress:4995) /* warning C4995: '_sntprintf': name was marked as #pragma deprecated */
                        _sntprintf(szQuery, sizeof(szQuery), _T("StringFileInfo\\%04X%04X\\CompanyName"),
                                   wCodePage,wLanguageID);

                        LPCTSTR pcData;
                        if (VerQueryValue(pFileInfo, szQuery,(void**)&pcData, &puInfoLen))
                        {
                            pushstring(pcData);
                        }
                        else
                            hr = __HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
                    }
                    else
                        hr = __HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
                }
                GlobalFree(pFileInfo);
            }
            else
                hr = __HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        }
        else
            hr = __HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    if (FAILED(hr))
        vboxPushResultAsString(hr);
}

/**
 * Shows a balloon message using VBoxTray's notification area in the
 * Windows task bar.
 *
 * @param   hwndParent          Window handle of parent.
 * @param   string_size         Size of variable string.
 * @param   variables           The actual variable string.
 * @param   stacktop            Pointer to a pointer to the current stack.
 */
VBOXINSTALLHELPER_EXPORT VBoxTrayShowBallonMsg(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop)
{
    NOREF(hwndParent);
    EXDLL_INIT();

    char szMsg[256];
    char szTitle[128];
    HRESULT hr = vboxPopString(szMsg, sizeof(szMsg) / sizeof(char));
    if (SUCCEEDED(hr))
        hr = vboxPopString(szTitle, sizeof(szTitle) / sizeof(char));

    /** @todo Do we need to restore the stack on failure? */

    if (SUCCEEDED(hr))
    {
        RTR3InitDll(0);

        uint32_t cbMsg = sizeof(VBOXTRAYIPCMSG_SHOWBALLOONMSG)
                       + strlen(szMsg) + 1    /* Include terminating zero */
                       + strlen(szTitle) + 1; /* Dito. */
        Assert(cbMsg);
        PVBOXTRAYIPCMSG_SHOWBALLOONMSG pIpcMsg =
            (PVBOXTRAYIPCMSG_SHOWBALLOONMSG)RTMemAlloc(cbMsg);
        if (pIpcMsg)
        {
            /* Stuff in the strings. */
            memcpy(pIpcMsg->szMsgContent, szMsg, strlen(szMsg)   + 1);
            memcpy(pIpcMsg->szMsgTitle, szTitle, strlen(szTitle) + 1);

            /* Pop off the values in reverse order from the stack. */
            if (SUCCEEDED(hr))
                hr = vboxPopULong((ULONG*)&pIpcMsg->uType);
            if (SUCCEEDED(hr))
                hr = vboxPopULong((ULONG*)&pIpcMsg->uShowMS);

            if (SUCCEEDED(hr))
            {
                RTLOCALIPCSESSION hSession = 0;
                int rc = vboxConnectToVBoxTray(&hSession);
                if (RT_SUCCESS(rc))
                {
                    VBOXTRAYIPCHEADER ipcHdr = { VBOXTRAY_IPC_HDR_MAGIC, 0 /* Header version */,
                                                 VBOXTRAYIPCMSGTYPE_SHOWBALLOONMSG, cbMsg };

                    rc = RTLocalIpcSessionWrite(hSession, &ipcHdr, sizeof(ipcHdr));
                    if (RT_SUCCESS(rc))
                        rc = RTLocalIpcSessionWrite(hSession, pIpcMsg, cbMsg);

                    int rc2 = RTLocalIpcSessionClose(hSession);
                    if (RT_SUCCESS(rc))
                        rc = rc2;
                }

                if (RT_FAILURE(rc))
                    hr = __HRESULT_FROM_WIN32(ERROR_BROKEN_PIPE);
            }

            RTMemFree(pIpcMsg);
        }
        else
            hr = __HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    /* Push simple return value on stack. */
    SUCCEEDED(hr) ? pushstring("0") : pushstring("1");
}

BOOL WINAPI DllMain(HANDLE hInst, ULONG uReason, LPVOID pReserved)
{
    RT_NOREF(uReason, pReserved);
    g_hInstance = (HINSTANCE)hInst;
    return TRUE;
}

