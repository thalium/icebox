/* $Id: VBoxWindowsAdditions.cpp $ */
/** @file
 * VBoxWindowsAdditions - The Windows Guest Additions Loader.
 *
 * This is STUB which select whether to install 32-bit or 64-bit additions.
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
#include <iprt/cdefs.h>
#include <iprt/win/windows.h>
#ifndef ERROR_ELEVATION_REQUIRED    /* Windows Vista and later. */
# define ERROR_ELEVATION_REQUIRED  740
#endif

#include <stdio.h>
#include <string.h>


static BOOL IsWow64(void)
{
    BOOL fIsWow64 = FALSE;
    typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(L"kernel32"), "IsWow64Process");
    if (fnIsWow64Process != NULL)
    {
        if (!fnIsWow64Process(GetCurrentProcess(), &fIsWow64))
        {
            fwprintf(stderr, L"ERROR: Could not determine process type!\n");

            /* Error in retrieving process type - assume that we're running on 32bit. */
            fIsWow64 = FALSE;
        }
    }
    return fIsWow64;
}

static void WaitForProcess2(HANDLE hProcess, int *piExitCode)
{
    /*
     * Wait for the process, make sure the deal with messages.
     */
    for (;;)
    {
        DWORD dwRc = MsgWaitForMultipleObjects(1, &hProcess, FALSE, 5000/*ms*/, QS_ALLEVENTS);

        MSG Msg;
        while (PeekMessageW(&Msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&Msg);
            DispatchMessageW(&Msg);
        }

        if (dwRc == WAIT_OBJECT_0)
            break;
        if (   dwRc != WAIT_TIMEOUT
            && dwRc != WAIT_OBJECT_0 + 1)
        {
            fwprintf(stderr, L"ERROR: MsgWaitForMultipleObjects failed: %u (%u)\n", dwRc, GetLastError());
            break;
        }
    }

    /*
     * Collect the process info.
     */
    DWORD dwExitCode;
    if (GetExitCodeProcess(hProcess, &dwExitCode))
        *piExitCode = (int)dwExitCode;
    else
    {
        fwprintf(stderr, L"ERROR: GetExitCodeProcess failed: %u\n", GetLastError());
        *piExitCode = 16;
    }
}

static void WaitForProcess(HANDLE hProcess, int *piExitCode)
{
    DWORD WaitRc = WaitForSingleObjectEx(hProcess, INFINITE, TRUE);
    while (   WaitRc == WAIT_IO_COMPLETION
           || WaitRc == WAIT_TIMEOUT)
        WaitRc = WaitForSingleObjectEx(hProcess, INFINITE, TRUE);
    if (WaitRc == WAIT_OBJECT_0)
    {
        DWORD dwExitCode;
        if (GetExitCodeProcess(hProcess, &dwExitCode))
            *piExitCode = (int)dwExitCode;
        else
        {
            fwprintf(stderr, L"ERROR: GetExitCodeProcess failed: %u\n", GetLastError());
            *piExitCode = 16;
        }
    }
    else
    {
        fwprintf(stderr, L"ERROR: WaitForSingleObjectEx failed: %u (%u)\n", WaitRc, GetLastError());
        *piExitCode = 16;
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    RT_NOREF(hInstance, hPrevInstance, lpCmdLine, nCmdShow);

    /*
     * Gather the parameters of the real installer program.
     */

    SetLastError(NO_ERROR);
    WCHAR wszCurDir[_MAX_PATH] = { 0 };
    DWORD cchCurDir = GetCurrentDirectoryW(sizeof(wszCurDir), wszCurDir);
    if (cchCurDir == 0 || cchCurDir >= sizeof(wszCurDir))
    {
        fwprintf(stderr, L"ERROR: GetCurrentDirectoryW failed: %u (ret %u)\n", GetLastError(), cchCurDir);
        return 12;
    }

    SetLastError(NO_ERROR);
    WCHAR wszModule[_MAX_PATH] = { 0 };
    DWORD cchModule = GetModuleFileNameW(NULL, wszModule, sizeof(wszModule));
    if (cchModule == 0 || cchModule >= sizeof(wszModule))
    {
        fwprintf(stderr, L"ERROR: GetModuleFileNameW failed: %u (ret %u)\n", GetLastError(), cchModule);
        return 13;
    }

    /* Strip the extension off the module name and construct the arch specific
       one of the real installer program. */
    DWORD off = cchModule - 1;
    while (   off > 0
           && (   wszModule[off] != '/'
               && wszModule[off] != '\\'
               && wszModule[off] != ':'))
    {
        if (wszModule[off] == '.')
        {
            wszModule[off] = '\0';
            cchModule = off;
            break;
        }
        off--;
    }

    WCHAR const  *pwszSuff = IsWow64() ? L"-amd64.exe" : L"-x86.exe";
    size_t        cchSuff  = wcslen(pwszSuff);
    if (cchSuff + cchModule >= sizeof(wszModule))
    {
        fwprintf(stderr, L"ERROR: Real installer name is too long (%u chars)\n", cchSuff + cchModule);
        return 14;
    }
    wcscpy(&wszModule[cchModule], pwszSuff);
    cchModule += cchSuff;

    /* Replace the first argument of the argument list. */
    PWCHAR  pwszNewCmdLine = NULL;
    LPCWSTR pwszOrgCmdLine = GetCommandLineW();
    if (pwszOrgCmdLine) /* Dunno if this can be NULL, but whatever. */
    {
        /* Skip the first argument in the original. */
        /** @todo Is there some ISBLANK or ISSPACE macro/function in Win32 that we could
         *        use here, if it's correct wrt. command line conventions? */
        WCHAR wch;
        while ((wch = *pwszOrgCmdLine) == L' ' || wch == L'\t')
            pwszOrgCmdLine++;
        if (wch == L'"')
        {
            pwszOrgCmdLine++;
            while ((wch = *pwszOrgCmdLine) != L'\0')
            {
                pwszOrgCmdLine++;
                if (wch == L'"')
                    break;
            }
        }
        else
        {
            while ((wch = *pwszOrgCmdLine) != L'\0')
            {
                pwszOrgCmdLine++;
                if (wch == L' ' || wch == L'\t')
                    break;
            }
        }
        while ((wch = *pwszOrgCmdLine) == L' ' || wch == L'\t')
            pwszOrgCmdLine++;

        /* Join up "szModule" with the remainder of the original command line. */
        size_t cchOrgCmdLine = wcslen(pwszOrgCmdLine);
        size_t cchNewCmdLine = 1 + cchModule + 1 + 1 + cchOrgCmdLine + 1;
        PWCHAR pwsz = pwszNewCmdLine = (PWCHAR)LocalAlloc(LPTR, cchNewCmdLine * sizeof(WCHAR));
        if (!pwsz)
        {
            fwprintf(stderr, L"ERROR: Out of memory (%u bytes)\n", (unsigned)cchNewCmdLine);
            return 15;
        }
        *pwsz++ = L'"';
        wcscpy(pwsz, wszModule);
        pwsz += cchModule;
        *pwsz++ = L'"';
        if (cchOrgCmdLine)
        {
            *pwsz++ = L' ';
            wcscpy(pwsz, pwszOrgCmdLine);
        }
        else
        {
            *pwsz = L'\0';
            pwszOrgCmdLine = NULL;
        }
    }

    /*
     * Start the process.
     */
    int iRet = 0;
    STARTUPINFOW        StartupInfo = { sizeof(StartupInfo), 0 };
    PROCESS_INFORMATION ProcInfo = { 0 };
    SetLastError(740);
    BOOL fOk = CreateProcessW(wszModule,
                              pwszNewCmdLine,
                              NULL /*pProcessAttributes*/,
                              NULL /*pThreadAttributes*/,
                              TRUE /*fInheritHandles*/,
                              0    /*dwCreationFlags*/,
                              NULL /*pEnvironment*/,
                              NULL /*pCurrentDirectory*/,
                              &StartupInfo,
                              &ProcInfo);
    if (fOk)
    {
        /* Wait for the process to finish. */
        CloseHandle(ProcInfo.hThread);
        WaitForProcess(ProcInfo.hProcess, &iRet);
        CloseHandle(ProcInfo.hProcess);
    }
    else if (GetLastError() == ERROR_ELEVATION_REQUIRED)
    {
        /*
         * Elevation is required. That can be accomplished via ShellExecuteEx
         * and the runas atom.
         */
        MSG Msg;
        PeekMessage(&Msg, NULL, 0, 0, PM_NOREMOVE);
        CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

        SHELLEXECUTEINFOW ShExecInfo = { 0 };
        ShExecInfo.cbSize       = sizeof(SHELLEXECUTEINFOW);
        ShExecInfo.fMask        = SEE_MASK_NOCLOSEPROCESS;
        ShExecInfo.hwnd         = NULL;
        ShExecInfo.lpVerb       = L"runas" ;
        ShExecInfo.lpFile       = wszModule;
        ShExecInfo.lpParameters = pwszOrgCmdLine; /* pass only args here!!! */
        ShExecInfo.lpDirectory  = wszCurDir;
        ShExecInfo.nShow        = SW_NORMAL;
        ShExecInfo.hProcess     = INVALID_HANDLE_VALUE;
        if (ShellExecuteExW(&ShExecInfo))
        {
            if (ShExecInfo.hProcess != INVALID_HANDLE_VALUE)
            {
                WaitForProcess2(ShExecInfo.hProcess, &iRet);
                CloseHandle(ShExecInfo.hProcess);
            }
            else
            {
                fwprintf(stderr, L"ERROR: ShellExecuteExW did not return a valid process handle!\n");
                iRet = 1;
            }
        }
        else
        {
            fwprintf(stderr, L"ERROR: Failed to execute '%ws' via ShellExecuteExW: %u\n", wszModule, GetLastError());
            iRet = 9;
        }
    }
    else
    {
        fwprintf(stderr, L"ERROR: Failed to execute '%ws' via CreateProcessW: %u\n", wszModule, GetLastError());
        iRet = 8;
    }

    if (pwszNewCmdLine)
        LocalFree(pwszNewCmdLine);

#if 0
    fwprintf(stderr, L"DEBUG: iRet=%d\n", iRet);
    fflush(stderr);
#endif
    return iRet;
}

