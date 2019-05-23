/* $Id: VBoxDrvInst.cpp $ */
/** @file
 * VBoxDrvInst - Driver and service installation helper for Windows guests.
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
#ifndef UNICODE
# define UNICODE
#endif

#include <iprt/cdefs.h>
#include <VBox/version.h>

#include <iprt/win/windows.h>
#include <iprt/win/setupapi.h>
#include <stdio.h>
#include <tchar.h>


/*********************************************************************************************************************************
*   Defines                                                                                                                      *
*********************************************************************************************************************************/

/* Exit codes */
#define EXIT_OK      (0)
#define EXIT_REBOOT  (1)
#define EXIT_FAIL    (2)
#define EXIT_USAGE   (3)

/* Prototypes */
typedef struct {
  PWSTR pApplicationId;
  PWSTR pDisplayName;
  PWSTR pProductName;
  PWSTR pMfgName;
} INSTALLERINFO, *PINSTALLERINFO;
typedef const PINSTALLERINFO PCINSTALLERINFO;

typedef enum {
  DIFXAPI_SUCCESS,
  DIFXAPI_INFO,
  DIFXAPI_WARNING,
  DIFXAPI_ERROR
} DIFXAPI_LOG;

typedef void (WINAPI * DIFXLOGCALLBACK_W)( DIFXAPI_LOG Event, DWORD Error, PCWSTR EventDescription, PVOID CallbackContext);
typedef void ( __cdecl* DIFXAPILOGCALLBACK_W)( DIFXAPI_LOG Event, DWORD Error, PCWSTR EventDescription, PVOID CallbackContext);

typedef DWORD (WINAPI *fnDriverPackageInstall) (PCTSTR DriverPackageInfPath, DWORD Flags, PCINSTALLERINFO pInstallerInfo, BOOL *pNeedReboot);
fnDriverPackageInstall g_pfnDriverPackageInstall = NULL;

typedef DWORD (WINAPI *fnDriverPackageUninstall) (PCTSTR DriverPackageInfPath, DWORD Flags, PCINSTALLERINFO pInstallerInfo, BOOL *pNeedReboot);
fnDriverPackageUninstall g_pfnDriverPackageUninstall = NULL;

typedef VOID (WINAPI *fnDIFXAPISetLogCallback) (DIFXAPILOGCALLBACK_W LogCallback, PVOID CallbackContext);
fnDIFXAPISetLogCallback g_pfnDIFXAPISetLogCallback = NULL;

/* Defines */
#define DRIVER_PACKAGE_REPAIR                 0x00000001
#define DRIVER_PACKAGE_SILENT                 0x00000002
#define DRIVER_PACKAGE_FORCE                  0x00000004
#define DRIVER_PACKAGE_ONLY_IF_DEVICE_PRESENT 0x00000008
#define DRIVER_PACKAGE_LEGACY_MODE            0x00000010
#define DRIVER_PACKAGE_DELETE_FILES           0x00000020

/* DIFx error codes */
/** @todo any reason why we're not using difxapi.h instead of these redefinitions? */
#ifndef ERROR_DRIVER_STORE_ADD_FAILED
# define ERROR_DRIVER_STORE_ADD_FAILED \
    (APPLICATION_ERROR_MASK | ERROR_SEVERITY_ERROR | 0x0247L)
#endif
#define ERROR_DEPENDENT_APPLICATIONS_EXIST \
    (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x300)
#define ERROR_DRIVER_PACKAGE_NOT_IN_STORE \
    (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x302)

/* Registry string list flags */
#define VBOX_REG_STRINGLIST_NONE              0x00000000        /* No flags set. */
#define VBOX_REG_STRINGLIST_ALLOW_DUPLICATES  0x00000001        /* Allows duplicates in list when adding a value. */

#ifdef DEBUG
# define VBOX_DRVINST_LOGFILE                 "C:\\Temp\\VBoxDrvInstDIFx.log"
#endif

/** @todo Get rid of all that TCHAR crap! Use WCHAR wherever possible. */

bool GetErrorMsg(DWORD dwLastError, _TCHAR *pszMsg, DWORD dwBufSize)
{
    if (::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwLastError, 0, pszMsg, dwBufSize / sizeof(TCHAR), NULL) == 0)
    {
        _sntprintf(pszMsg, dwBufSize / sizeof(TCHAR), _T("Unknown error!\n"), dwLastError);
        return false;
    }
    _TCHAR *p = _tcschr(pszMsg, _T('\r'));
    if (p != NULL)
        *p = _T('\0');
    return true;
}

/**
 * Log callback for DIFxAPI calls.
 *
 * @param   Event                   The event's structure to log.
 * @param   dwError                 The event's error level.
 * @param   pEventDescription       The event's text description.
 * @param   pCallbackContext        User-supplied callback context.
 */
void LogCallback(DIFXAPI_LOG Event, DWORD dwError, PCWSTR pEventDescription, PVOID pCallbackContext)
{
    if (dwError == 0)
        _tprintf(_T("(%u) %ws\n"), Event, pEventDescription);
    else
        _tprintf(_T("(%u) ERROR: %u - %ws\n"), Event, dwError, pEventDescription);

     if (pCallbackContext)
         fwprintf((FILE*)pCallbackContext, _T("(%u) %u - %s\n"), Event, dwError, pEventDescription);
}

/**
 * Loads a system DLL.
 *
 * @returns Module handle or NULL
 * @param   pwszName            The DLL name.
 */
static HMODULE loadInstalledDll(const wchar_t *pwszName)
{
    /* Get the process image path. */
    WCHAR  wszPath[MAX_PATH];
    UINT   cwcPath = GetModuleFileNameW(NULL, wszPath, MAX_PATH);
    if (!cwcPath || cwcPath >= MAX_PATH)
        return NULL;

    /* Drop the image filename. */
    UINT   off = cwcPath - 1;
    for (;;)
    {
        if (   wszPath[off] == '\\'
            || wszPath[off] == '/'
            || wszPath[off] == ':')
        {
            wszPath[off] = '\0';
            cwcPath = off;
            break;
        }
        if (!off--)
            return NULL; /* No path? Shouldn't ever happen! */
    }

    /* Check if there is room in the buffer to construct the desired name. */
    size_t cwcName = 0;
    while (pwszName[cwcName])
        cwcName++;
    if (cwcPath + 1 + cwcName + 1 > MAX_PATH)
        return NULL;

    wszPath[cwcPath] = '\\';
    memcpy(&wszPath[cwcPath + 1], pwszName, (cwcName + 1) * sizeof(wszPath[0]));
    return LoadLibraryW(wszPath);
}

/**
 * (Un)Installs a driver from/to the system.
 *
 * @return  Exit code (EXIT_OK, EXIT_FAIL)
 * @param   fInstall            Flag indicating whether to install (TRUE) or uninstall (FALSE) a driver.
 * @param   pszDriverPath       Pointer to full qualified path to the driver's .INF file (+ driver files).
 * @param   fSilent             Flag indicating a silent installation (TRUE) or not (FALSE).
 * @param   pszLogFile          Pointer to full qualified path to log file to be written during installation.
 *                              Optional.
 */
int VBoxInstallDriver(const BOOL fInstall, const _TCHAR *pszDriverPath, BOOL fSilent,
                      const _TCHAR *pszLogFile)
{
    HRESULT hr = S_OK;
    HMODULE hDIFxAPI = loadInstalledDll(L"DIFxAPI.dll");
    if (NULL == hDIFxAPI)
    {
        _tprintf(_T("ERROR: Unable to locate DIFxAPI.dll!\n"));
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
    else
    {
        if (fInstall)
        {
            g_pfnDriverPackageInstall = (fnDriverPackageInstall)GetProcAddress(hDIFxAPI, "DriverPackageInstallW");
            if (g_pfnDriverPackageInstall == NULL)
            {
                _tprintf(_T("ERROR: Unable to retrieve entry point for DriverPackageInstallW!\n"));
                hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
            }
        }
        else
        {
            g_pfnDriverPackageUninstall = (fnDriverPackageUninstall)GetProcAddress(hDIFxAPI, "DriverPackageUninstallW");
            if (g_pfnDriverPackageUninstall == NULL)
            {
                _tprintf(_T("ERROR: Unable to retrieve entry point for DriverPackageUninstallW!\n"));
                hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
            }
        }

        if (SUCCEEDED(hr))
        {
            g_pfnDIFXAPISetLogCallback = (fnDIFXAPISetLogCallback)GetProcAddress(hDIFxAPI, "DIFXAPISetLogCallbackW");
            if (g_pfnDIFXAPISetLogCallback == NULL)
            {
                _tprintf(_T("ERROR: Unable to retrieve entry point for DIFXAPISetLogCallbackW!\n"));
                hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        FILE *phFile = NULL;
        if (pszLogFile)
        {
            phFile = _wfopen(pszLogFile, _T("a"));
            if (!phFile)
                _tprintf(_T("ERROR: Unable to create log file!\n"));
            g_pfnDIFXAPISetLogCallback(LogCallback, phFile);
        }

        INSTALLERINFO instInfo =
        {
            TEXT("{7d2c708d-c202-40ab-b3e8-de21da1dc629}"), /* Our GUID for representing this installation tool. */
            TEXT("VirtualBox Guest Additions Install Helper"),
            TEXT("VirtualBox Guest Additions"), /** @todo Add version! */
            TEXT("Oracle Corporation")
        };

        _TCHAR szDriverInf[MAX_PATH + 1];
        if (0 == GetFullPathNameW(pszDriverPath, MAX_PATH, szDriverInf, NULL))
        {
            _tprintf(_T("ERROR: INF-Path too long / could not be retrieved!\n"));
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }
        else
        {
            if (fInstall)
                _tprintf(_T("Installing driver ...\n"));
            else
                _tprintf(_T("Uninstalling driver ...\n"));
            _tprintf(_T("INF-File: %ws\n"), szDriverInf);

            DWORD dwFlags = DRIVER_PACKAGE_FORCE;
            if (!fInstall)
                dwFlags |= DRIVER_PACKAGE_DELETE_FILES;

            OSVERSIONINFO osi;
            memset(&osi, 0, sizeof(OSVERSIONINFO));
            osi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
            if ( (GetVersionEx(&osi) != 0)
              && (osi.dwPlatformId == VER_PLATFORM_WIN32_NT)
              && (osi.dwMajorVersion < 6))
            {
                if (fInstall)
                {
                    _tprintf(_T("Using legacy mode for install ...\n"));
                    dwFlags |= DRIVER_PACKAGE_LEGACY_MODE;
                }
            }

            if (fSilent)
            {
                _tprintf(_T("Installation is silent ...\n"));
                /*
                 * Don't add DRIVER_PACKAGE_SILENT to dwFlags here, otherwise
                 * installation will fail because we (still) don't have WHQL certified
                 * drivers. See CERT_E_WRONG_USAGE on MSDN for more information.
                 */
            }

            BOOL fReboot;
            DWORD dwRet = fInstall ?
                            g_pfnDriverPackageInstall(szDriverInf, dwFlags, &instInfo, &fReboot)
                          : g_pfnDriverPackageUninstall(szDriverInf, dwFlags, &instInfo, &fReboot);
            if (dwRet != ERROR_SUCCESS)
            {
                switch (dwRet)
                {
                    case CRYPT_E_FILE_ERROR:
                        _tprintf(_T("ERROR: The catalog file for the specified driver package was not found!\n"));
                        break;

                    case ERROR_ACCESS_DENIED:
                        _tprintf(_T("ERROR: Caller is not in Administrators group to (un)install this driver package!\n"));
                        break;

                    case ERROR_BAD_ENVIRONMENT:
                        _tprintf(_T("ERROR: The current Microsoft Windows version does not support this operation!\n"));
                        break;

                    case ERROR_CANT_ACCESS_FILE:
                        _tprintf(_T("ERROR: The driver package files could not be accessed!\n"));
                        break;

                    case ERROR_DEPENDENT_APPLICATIONS_EXIST:
                        _tprintf(_T("ERROR: DriverPackageUninstall removed an association between the driver package and the specified application but the function did not uninstall the driver package because other applications are associated with the driver package!\n"));
                        break;

                    case ERROR_DRIVER_PACKAGE_NOT_IN_STORE:
                        _tprintf(_T("ERROR: There is no INF file in the DIFx driver store that corresponds to the INF file %ws!\n"), szDriverInf);
                        break;

                    case ERROR_FILE_NOT_FOUND:
                        _tprintf(_T("ERROR: File not found! File = %ws\n"), szDriverInf);
                        break;

                    case ERROR_IN_WOW64:
                        _tprintf(_T("ERROR: The calling application is a 32-bit application attempting to execute in a 64-bit environment, which is not allowed!\n"));
                        break;

                    case ERROR_INVALID_FLAGS:
                        _tprintf(_T("ERROR: The flags specified are invalid!\n"));
                        break;

                    case ERROR_INSTALL_FAILURE:
                        _tprintf(_T("ERROR: The (un)install operation failed! Consult the Setup API logs for more information.\n"));
                        break;

                    case ERROR_NO_MORE_ITEMS:
                        _tprintf(
                                 _T(
                                    "ERROR: The function found a match for the HardwareId value, but the specified driver was not a better match than the current driver and the caller did not specify the INSTALLFLAG_FORCE flag!\n"));
                        break;

                    case ERROR_NO_DRIVER_SELECTED:
                        _tprintf(_T("ERROR: No driver in .INF-file selected!\n"));
                        break;

                    case ERROR_SECTION_NOT_FOUND:
                        _tprintf(_T("ERROR: Section in .INF-file was not found!\n"));
                        break;

                    case ERROR_SHARING_VIOLATION:
                        _tprintf(_T("ERROR: A component of the driver package in the DIFx driver store is locked by a thread or process\n"));
                        break;

                    /*
                     * !    sig:           Verifying file against specific Authenticode(tm) catalog failed! (0x800b0109)
                     * !    sig:           Error 0x800b0109: A certificate chain processed, but terminated in a root certificate which is not trusted by the trust provider.
                     * !!!  sto:           No error message will be displayed as client is running in non-interactive mode.
                     * !!!  ndv:           Driver package failed signature validation. Error = 0xE0000247
                     */
                    case ERROR_DRIVER_STORE_ADD_FAILED:
                        _tprintf(_T("ERROR: Adding driver to the driver store failed!!\n"));
                        break;

                    case ERROR_UNSUPPORTED_TYPE:
                        _tprintf(_T("ERROR: The driver package type is not supported of INF %ws!\n"), szDriverInf);
                        break;

                    default:
                    {
                        /* Try error lookup with GetErrorMsg(). */
                        TCHAR szErrMsg[1024];
                        GetErrorMsg(dwRet, szErrMsg, sizeof(szErrMsg));
                        _tprintf(_T("ERROR (%08x): %ws\n"), dwRet, szErrMsg);
                        break;
                    }
                }
                hr = HRESULT_FROM_WIN32(dwRet);
            }
            g_pfnDIFXAPISetLogCallback(NULL, NULL);
            if (phFile)
                fclose(phFile);
            if (SUCCEEDED(hr))
            {
                _tprintf(_T("Driver was %sinstalled successfully!\n"), fInstall ? _T("") : _T("un"));
                if (fReboot)
                    _tprintf(_T("A reboot is needed to complete the driver %sinstallation!\n"), fInstall ? _T("") : _T("un"));
            }
        }
    }

    if (NULL != hDIFxAPI)
       FreeLibrary(hDIFxAPI);

    return SUCCEEDED(hr) ? EXIT_OK : EXIT_FAIL;
}

static UINT WINAPI vboxDrvInstExecuteInfFileCallback(PVOID Context,
                                                     UINT Notification,
                                                     UINT_PTR Param1,
                                                     UINT_PTR Param2)
{
#ifdef DEBUG
    _tprintf (_T( "Got installation notification %u\n"), Notification);
#endif

    switch (Notification)
    {
        case SPFILENOTIFY_NEEDMEDIA:
            _tprintf (_T( "Requesting installation media ...\n"));
            break;

        case SPFILENOTIFY_STARTCOPY:
            _tprintf (_T( "Copying driver files to destination ...\n"));
            break;

        case SPFILENOTIFY_TARGETNEWER:
        case SPFILENOTIFY_TARGETEXISTS:
            return TRUE;
    }

    return SetupDefaultQueueCallback(Context, Notification, Param1, Param2);
}

/**
 * Executes a sepcified .INF section to install/uninstall drivers and/or services.
 *
 * @return  Exit code (EXIT_OK, EXIT_FAIL)
 * @param   pszSection          Section to execute; usually it's "DefaultInstall".
 * @param   iMode               Execution mode to use (see MSDN).
 * @param   pszInf              Full qualified path of the .INF file to use.
 */
int ExecuteInfFile(const _TCHAR *pszSection, int iMode, const _TCHAR *pszInf)
{
    RT_NOREF(iMode);
    _tprintf(_T("Installing from INF-File: %ws (Section: %ws) ...\n"),
             pszInf, pszSection);

    UINT uErrorLine = 0;
    HINF hINF = SetupOpenInfFile(pszInf, NULL, INF_STYLE_WIN4, &uErrorLine);
    if (hINF != INVALID_HANDLE_VALUE)
    {
        PVOID pvQueue = SetupInitDefaultQueueCallback(NULL);

        BOOL fSuccess = SetupInstallFromInfSection(NULL,
                                                    hINF,
                                                    pszSection,
                                                    SPINST_ALL,
                                                    HKEY_LOCAL_MACHINE,
                                                    NULL,
                                                    SP_COPY_NEWER_OR_SAME | SP_COPY_NOSKIP,
                                                    vboxDrvInstExecuteInfFileCallback,
                                                    pvQueue,
                                                    NULL,
                                                    NULL
                                                    );
        if (fSuccess)
        {
            _tprintf (_T( "File installation stage successful\n"));

            fSuccess = SetupInstallServicesFromInfSection(hINF,
                                                          L"DefaultInstall.Services",
                                                          0 /* Flags */);
            if (fSuccess)
            {
                _tprintf (_T( "Service installation stage successful. Installation completed\n"));
            }
            else
            {
                DWORD dwErr = GetLastError();
                switch (dwErr)
                {
                    case ERROR_SUCCESS_REBOOT_REQUIRED:
                        _tprintf (_T( "A reboot is required to complete the installation\n"));
                        break;

                    case ERROR_SECTION_NOT_FOUND:
                        break;

                    default:
                        _tprintf (_T( "Error %ld while installing service\n"), dwErr);
                        break;
                }
            }
        }
        else
            _tprintf (_T( "Error %ld while installing files\n"), GetLastError());

        if (pvQueue)
            SetupTermDefaultQueueCallback(pvQueue);

        SetupCloseInfFile(hINF);
    }
    else
        _tprintf (_T( "Unable to open %ws: %ld (error line %u)\n"),
                  pszInf, GetLastError(), uErrorLine);

    return EXIT_OK;
}

/**
 * Adds a string entry to a MULTI_SZ registry list.
 *
 * @return  Exit code (EXIT_OK, EXIT_FAIL)
 * @param   pszSubKey           Sub key containing the list.
 * @param   pszKeyValue         The actual key name of the list.
 * @param   pszValueToAdd       The value to add to the list.
 * @param   uiOrder             Position (zero-based) of where to add the value to the list.
 */
int RegistryAddStringToMultiSZ(const TCHAR *pszSubKey, const TCHAR *pszKeyValue, const TCHAR *pszValueToAdd, unsigned int uiOrder)
{
#ifdef DEBUG
    _tprintf(_T("AddStringToMultiSZ: Adding MULTI_SZ string %ws to %ws\\%ws (Order = %d)\n"), pszValueToAdd, pszSubKey, pszKeyValue, uiOrder);
#endif

    HKEY hKey = NULL;
    DWORD disp, dwType;
    LONG lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, pszSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, &disp);
    if (lRet != ERROR_SUCCESS)
        _tprintf(_T("AddStringToMultiSZ: RegCreateKeyEx %ts failed with error %ld!\n"), pszSubKey, lRet);

    if (lRet == ERROR_SUCCESS)
    {
        TCHAR szKeyValue[512] = { 0 };
        TCHAR szNewKeyValue[512] = { 0 };
        DWORD cbKeyValue = sizeof(szKeyValue);

        lRet = RegQueryValueEx(hKey, pszKeyValue, NULL, &dwType, (LPBYTE)szKeyValue, &cbKeyValue);
        if (   lRet != ERROR_SUCCESS
            || dwType != REG_MULTI_SZ)
        {
           _tprintf(_T("AddStringToMultiSZ: RegQueryValueEx failed with error %ld, key type = 0x%x!\n"), lRet, dwType);
        }
        else
        {

            /* Look if the network provider is already in the list. */
            unsigned int iPos = 0;
            size_t cb = 0;

            /* Replace delimiting "\0"'s with "," to make tokenizing work. */
            for (unsigned i = 0; i < cbKeyValue / sizeof(TCHAR); i++)
                if (szKeyValue[i] == '\0') szKeyValue[i] = ',';

            TCHAR *pszToken = wcstok(szKeyValue, _T(","));
            TCHAR *pszNewToken = NULL;
            TCHAR *pNewKeyValuePos = szNewKeyValue;
            while (pszToken != NULL)
            {
               pszNewToken = wcstok(NULL, _T(","));

               /* Append new value (at beginning if iOrder=0). */
               if (iPos == uiOrder)
               {
                   memcpy(pNewKeyValuePos, pszValueToAdd, wcslen(pszValueToAdd)*sizeof(TCHAR));

                   cb += (wcslen(pszValueToAdd) + 1) * sizeof(TCHAR);  /* Add trailing zero as well. */
                   pNewKeyValuePos += wcslen(pszValueToAdd) + 1;
                   iPos++;
               }

               if (0 != wcsicmp(pszToken, pszValueToAdd))
               {
                   memcpy(pNewKeyValuePos, pszToken, wcslen(pszToken)*sizeof(TCHAR));
                   cb += (wcslen(pszToken) + 1) * sizeof(TCHAR);  /* Add trailing zero as well. */
                   pNewKeyValuePos += wcslen(pszToken) + 1;
                   iPos++;
               }

               pszToken = pszNewToken;
            }

            /* Append as last item if needed. */
            if (uiOrder >= iPos)
            {
                memcpy(pNewKeyValuePos, pszValueToAdd, wcslen(pszValueToAdd)*sizeof(TCHAR));
                cb += wcslen(pszValueToAdd) * sizeof(TCHAR);  /* Add trailing zero as well. */
            }

            lRet = RegSetValueExW(hKey, pszKeyValue, 0, REG_MULTI_SZ, (LPBYTE)szNewKeyValue, (DWORD)cb);
            if (lRet != ERROR_SUCCESS)
               _tprintf(_T("AddStringToMultiSZ: RegSetValueEx failed with error %ld!\n"), lRet);
        }

        RegCloseKey(hKey);
    #ifdef DEBUG
        if (lRet == ERROR_SUCCESS)
           _tprintf(_T("AddStringToMultiSZ: Value %ws successfully written!\n"), pszValueToAdd);
    #endif
    }

    return (lRet == ERROR_SUCCESS) ? EXIT_OK : EXIT_FAIL;
}

/**
 * Removes a string entry from a MULTI_SZ registry list.
 *
 * @return  Exit code (EXIT_OK, EXIT_FAIL)
 * @param   pszSubKey           Sub key containing the list.
 * @param   pszKeyValue         The actual key name of the list.
 * @param   pszValueToRemove    The value to remove from the list.
 */
int RegistryRemoveStringFromMultiSZ(const TCHAR *pszSubKey, const TCHAR *pszKeyValue, const TCHAR *pszValueToRemove)
{
    /// @todo Make string sizes dynamically allocated!

    const TCHAR *pszKey = pszSubKey;
#ifdef DEBUG
    _tprintf(_T("RemoveStringFromMultiSZ: Removing MULTI_SZ string: %ws from %ws\\%ws ...\n"), pszValueToRemove, pszSubKey, pszKeyValue);
#endif

    HKEY hkey;
    DWORD disp, dwType;
    LONG lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, pszKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hkey, &disp);
    if (lRet != ERROR_SUCCESS)
        _tprintf(_T("RemoveStringFromMultiSZ: RegCreateKeyEx %ts failed with error %ld!\n"), pszKey, lRet);

    if (lRet == ERROR_SUCCESS)
    {
        TCHAR szKeyValue[1024];
        DWORD cbKeyValue = sizeof(szKeyValue);

        lRet = RegQueryValueEx(hkey, pszKeyValue, NULL, &dwType, (LPBYTE)szKeyValue, &cbKeyValue);
        if (   lRet   != ERROR_SUCCESS
            || dwType != REG_MULTI_SZ)
        {
            _tprintf(_T("RemoveStringFromMultiSZ: RegQueryValueEx failed with %d, key type = 0x%x!\n"), lRet, dwType);
        }
        else
        {
        #ifdef DEBUG
            _tprintf(_T("RemoveStringFromMultiSZ: Current key len: %ld\n"), cbKeyValue);
        #endif

            TCHAR szCurString[1024] = { 0 };
            TCHAR szFinalString[1024] = { 0 };
            int iIndex = 0;
            int iNewIndex = 0;
            for (unsigned i = 0; i < cbKeyValue / sizeof(TCHAR); i++)
            {
                if (szKeyValue[i] != _T('\0'))
                    szCurString[iIndex++] = szKeyValue[i];

                if (   (!szKeyValue[i] == _T('\0'))
                    && (szKeyValue[i + 1] == _T('\0')))
                {
                    if (NULL == wcsstr(szCurString, pszValueToRemove))
                    {
                        wcscat(&szFinalString[iNewIndex], szCurString);

                        if (iNewIndex == 0)
                            iNewIndex = iIndex;
                        else iNewIndex += iIndex;

                        szFinalString[++iNewIndex] = _T('\0');
                    }

                    iIndex = 0;
                    ZeroMemory(szCurString, sizeof(szCurString));
                }
            }
            szFinalString[++iNewIndex] = _T('\0');
        #ifdef DEBUG
            _tprintf(_T("RemoveStringFromMultiSZ: New key value: %ws (%u bytes)\n"),
                     szFinalString, iNewIndex * sizeof(TCHAR));
        #endif

            lRet = RegSetValueExW(hkey, pszKeyValue, 0, REG_MULTI_SZ, (LPBYTE)szFinalString, iNewIndex * sizeof(TCHAR));
            if (lRet != ERROR_SUCCESS)
                _tprintf(_T("RemoveStringFromMultiSZ: RegSetValueEx failed with %d!\n"), lRet);
        }

        RegCloseKey(hkey);
    #ifdef DEBUG
        if (lRet == ERROR_SUCCESS)
            _tprintf(_T("RemoveStringFromMultiSZ: Value %ws successfully removed!\n"), pszValueToRemove);
    #endif
    }

    return (lRet == ERROR_SUCCESS) ? EXIT_OK : EXIT_FAIL;
}

/**
 * Adds a string to a registry string list (STRING_SZ).
 * Only operates in HKLM for now, needs to be extended later for
 * using other hives. Only processes lists with a "," separator
 * at the moment.
 *
 * @return  Exit code (EXIT_OK, EXIT_FAIL)
 * @param   pszSubKey           Sub key containing the list.
 * @param   pszKeyValue         The actual key name of the list.
 * @param   pszValueToAdd       The value to add to the list.
 * @param   uiOrder             Position (zero-based) of where to add the value to the list.
 * @param   dwFlags             Flags.
 */
int RegistryAddStringToList(const TCHAR *pszSubKey, const TCHAR *pszKeyValue, const TCHAR *pszValueToAdd,
                            unsigned int uiOrder, DWORD dwFlags)
{
    HKEY hKey = NULL;
    DWORD disp, dwType;
    LONG lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, pszSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, &disp);
    if (lRet != ERROR_SUCCESS)
        _tprintf(_T("RegistryAddStringToList: RegCreateKeyEx %ts failed with error %ld!\n"), pszSubKey, lRet);

    TCHAR szKeyValue[512] = { 0 };
    TCHAR szNewKeyValue[512] = { 0 };
    DWORD cbKeyValue = sizeof(szKeyValue);

    lRet = RegQueryValueEx(hKey, pszKeyValue, NULL, &dwType, (LPBYTE)szKeyValue, &cbKeyValue);
    if (   lRet != ERROR_SUCCESS
        || dwType != REG_SZ)
    {
        _tprintf(_T("RegistryAddStringToList: RegQueryValueEx failed with %d, key type = 0x%x!\n"), lRet, dwType);
    }

    if (lRet == ERROR_SUCCESS)
    {
    #ifdef DEBUG
        _tprintf(_T("RegistryAddStringToList: Key value: %ws\n"), szKeyValue);
    #endif

        /* Create entire new list. */
        unsigned int iPos = 0;
        TCHAR *pszToken = wcstok(szKeyValue, _T(","));
        TCHAR *pszNewToken = NULL;
        while (pszToken != NULL)
        {
            pszNewToken = wcstok(NULL, _T(","));

            /* Append new provider name (at beginning if iOrder=0). */
            if (iPos == uiOrder)
            {
                wcscat(szNewKeyValue, pszValueToAdd);
                wcscat(szNewKeyValue, _T(","));
                iPos++;
            }

            BOOL fAddToList = FALSE;
            if (   !wcsicmp(pszToken, pszValueToAdd)
                && (dwFlags & VBOX_REG_STRINGLIST_ALLOW_DUPLICATES))
                fAddToList = TRUE;
            else if (wcsicmp(pszToken, pszValueToAdd))
                fAddToList = TRUE;

            if (fAddToList)
            {
                wcscat(szNewKeyValue, pszToken);
                wcscat(szNewKeyValue, _T(","));
                iPos++;
            }

    #ifdef DEBUG
            _tprintf (_T("RegistryAddStringToList: Temp new key value: %ws\n"), szNewKeyValue);
    #endif
            pszToken = pszNewToken;
        }

        /* Append as last item if needed. */
        if (uiOrder >= iPos)
            wcscat(szNewKeyValue, pszValueToAdd);

        /* Last char a delimiter? Cut off ... */
        if (szNewKeyValue[wcslen(szNewKeyValue) - 1] == ',')
            szNewKeyValue[wcslen(szNewKeyValue) - 1] = '\0';

        size_t iNewLen = (wcslen(szNewKeyValue) * sizeof(WCHAR)) + sizeof(WCHAR);

    #ifdef DEBUG
        _tprintf(_T("RegistryAddStringToList: New provider list: %ws (%u bytes)\n"), szNewKeyValue, iNewLen);
    #endif

        lRet = RegSetValueExW(hKey, pszKeyValue, 0, REG_SZ, (LPBYTE)szNewKeyValue, (DWORD)iNewLen);
        if (lRet != ERROR_SUCCESS)
            _tprintf(_T("RegistryAddStringToList: RegSetValueEx failed with %ld!\n"), lRet);
    }

    RegCloseKey(hKey);
    return (lRet == ERROR_SUCCESS) ? EXIT_OK : EXIT_FAIL;
}

/**
 * Removes a string from a registry string list (STRING_SZ).
 * Only operates in HKLM for now, needs to be extended later for
 * using other hives. Only processes lists with a "," separator
 * at the moment.
 *
 * @return  Exit code (EXIT_OK, EXIT_FAIL)
 * @param   pszSubKey           Sub key containing the list.
 * @param   pszKeyValue         The actual key name of the list.
 * @param   pszValueToRemove    The value to remove from the list.
 */
int RegistryRemoveStringFromList(const TCHAR *pszSubKey, const TCHAR *pszKeyValue, const TCHAR *pszValueToRemove)
{
    HKEY hKey = NULL;
    DWORD disp, dwType;
    LONG lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, pszSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, &disp);
    if (lRet != ERROR_SUCCESS)
        _tprintf(_T("RegistryRemoveStringFromList: RegCreateKeyEx %ts failed with error %ld!\n"), pszSubKey, lRet);

    TCHAR szKeyValue[512] = { 0 };
    TCHAR szNewKeyValue[512] = { 0 };
    DWORD cbKeyValue = sizeof(szKeyValue);

    lRet = RegQueryValueEx(hKey, pszKeyValue, NULL, &dwType, (LPBYTE)szKeyValue, &cbKeyValue);
    if (   lRet != ERROR_SUCCESS
        || dwType != REG_SZ)
    {
        _tprintf(_T("RegistryRemoveStringFromList: RegQueryValueEx failed with %d, key type = 0x%x!\n"), lRet, dwType);
    }

    if (lRet == ERROR_SUCCESS)
    {
    #ifdef DEBUG
        _tprintf(_T("RegistryRemoveStringFromList: Key value: %ws\n"), szKeyValue);
    #endif

        /* Create entire new list. */
        int iPos = 0;

        TCHAR *pszToken = wcstok(szKeyValue, _T(","));
        TCHAR *pszNewToken = NULL;
        while (pszToken != NULL)
        {
            pszNewToken = wcstok(NULL, _T(","));

            /* Append all list values as long as it's not the
             * value we want to remove. */
            if (wcsicmp(pszToken, pszValueToRemove))
            {
                wcscat(szNewKeyValue, pszToken);
                wcscat(szNewKeyValue, _T(","));
                iPos++;
            }

    #ifdef DEBUG
            _tprintf (_T("RegistryRemoveStringFromList: Temp new key value: %ws\n"), szNewKeyValue);
    #endif
            pszToken = pszNewToken;
        }

        /* Last char a delimiter? Cut off ... */
        if (szNewKeyValue[wcslen(szNewKeyValue) - 1] == ',')
            szNewKeyValue[wcslen(szNewKeyValue) - 1] = '\0';

        size_t iNewLen = (wcslen(szNewKeyValue) * sizeof(WCHAR)) + sizeof(WCHAR);

    #ifdef DEBUG
        _tprintf(_T("RegistryRemoveStringFromList: New provider list: %ws (%u bytes)\n"), szNewKeyValue, iNewLen);
    #endif

        lRet = RegSetValueExW(hKey, pszKeyValue, 0, REG_SZ, (LPBYTE)szNewKeyValue, (DWORD)iNewLen);
        if (lRet != ERROR_SUCCESS)
            _tprintf(_T("RegistryRemoveStringFromList: RegSetValueEx failed with %ld!\n"), lRet);
    }

    RegCloseKey(hKey);
    return (lRet == ERROR_SUCCESS) ? EXIT_OK : EXIT_FAIL;
}

/**
 * Adds a network provider with a specified order to the system.
 *
 * @return  Exit code (EXIT_OK, EXIT_FAIL)
 * @param   pszProvider         Name of network provider to add.
 * @param   uiOrder             Position in list (zero-based) of where to add.
 */
int AddNetworkProvider(const TCHAR *pszProvider, unsigned int uiOrder)
{
    _tprintf(_T("Adding network provider \"%ws\" (Order = %u) ...\n"), pszProvider, uiOrder);
    int rc = RegistryAddStringToList(_T("System\\CurrentControlSet\\Control\\NetworkProvider\\Order"),
                                     _T("ProviderOrder"),
                                     pszProvider, uiOrder, VBOX_REG_STRINGLIST_NONE /* No flags set */);
    if (rc == EXIT_OK)
        _tprintf(_T("Network provider successfully added!\n"));
    return rc;
}

/**
 * Removes a network provider from the system.
 *
 * @return  Exit code (EXIT_OK, EXIT_FAIL)
 * @param   pszProvider         Name of network provider to remove.
 */
int RemoveNetworkProvider(const TCHAR *pszProvider)
{
    _tprintf(_T("Removing network provider \"%ws\" ...\n"), pszProvider);
    int rc = RegistryRemoveStringFromList(_T("System\\CurrentControlSet\\Control\\NetworkProvider\\Order"),
                                          _T("ProviderOrder"),
                                          pszProvider);
    if (rc == EXIT_OK)
        _tprintf(_T("Network provider successfully removed!\n"));
    return rc;
}

int CreateService(const TCHAR *pszStartStopName,
                  const TCHAR *pszDisplayName,
                  int iServiceType,
                  int iStartType,
                  const TCHAR *pszBinPath,
                  const TCHAR *pszLoadOrderGroup,
                  const TCHAR *pszDependencies,
                  const TCHAR *pszLogonUser,
                  const TCHAR *pszLogonPassword)
{
    int rc = ERROR_SUCCESS;

    _tprintf(_T("Installing service %ws (%ws) ...\n"), pszDisplayName, pszStartStopName);

    SC_HANDLE hSCManager = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL)
    {
        _tprintf(_T("Could not get handle to SCM! Error: %ld\n"), GetLastError());
        return EXIT_FAIL;
    }

    /* Fixup end of multistring. */
    TCHAR szDepend[ _MAX_PATH ] = { 0 }; /** @todo Use dynamically allocated string here! */
    if (pszDependencies != NULL)
    {
        _tcsnccpy (szDepend, pszDependencies, wcslen(pszDependencies));
        DWORD len = (DWORD)wcslen (szDepend);
        szDepend [len + 1] = 0;

        /* Replace comma separator on null separator. */
        for (DWORD i = 0; i < len; i++)
        {
            if (',' == szDepend [i])
                szDepend [i] = 0;
        }
    }

    DWORD dwTag = 0xDEADBEAF;
    SC_HANDLE hService = CreateService (hSCManager,           /* SCManager database handle. */
                                        pszStartStopName,     /* Name of service. */
                                        pszDisplayName,       /* Name to display. */
                                        SERVICE_ALL_ACCESS,   /* Desired access. */
                                        iServiceType,         /* Service type. */
                                        iStartType,           /* Start type. */
                                        SERVICE_ERROR_NORMAL, /* Error control type. */
                                        pszBinPath,           /* Service's binary. */
                                        pszLoadOrderGroup,    /* Ordering group. */
                                        (pszLoadOrderGroup != NULL) ? &dwTag : NULL,           /* Tag identifier. */
                                        (pszDependencies != NULL) ? szDepend : NULL,           /* Dependencies. */
                                        (pszLogonUser != NULL) ? pszLogonUser: NULL,           /* Account. */
                                        (pszLogonPassword != NULL) ? pszLogonPassword : NULL); /* Password. */
    if (NULL == hService)
    {
        DWORD dwErr = GetLastError();
        switch (dwErr)
        {

        case ERROR_SERVICE_EXISTS:
        {
            _tprintf(_T("Service already exists. No installation required. Updating the service config.\n"));

            hService = OpenService (hSCManager,          /* SCManager database handle. */
                                    pszStartStopName,    /* Name of service. */
                                    SERVICE_ALL_ACCESS); /* Desired access. */
            if (NULL == hService)
            {
                dwErr = GetLastError();
                _tprintf(_T("Could not open service! Error: %ld\n"), dwErr);
            }
            else
            {
                BOOL fResult = ChangeServiceConfig (hService,            /* Service handle. */
                                                   iServiceType,         /* Service type. */
                                                   iStartType,           /* Start type. */
                                                   SERVICE_ERROR_NORMAL, /* Error control type. */
                                                   pszBinPath,           /* Service's binary. */
                                                   pszLoadOrderGroup,    /* Ordering group. */
                                                   (pszLoadOrderGroup != NULL) ? &dwTag : NULL,          /* Tag identifier. */
                                                   (pszDependencies != NULL) ? szDepend : NULL,          /* Dependencies. */
                                                   (pszLogonUser != NULL) ? pszLogonUser: NULL,          /* Account. */
                                                   (pszLogonPassword != NULL) ? pszLogonPassword : NULL, /* Password. */
                                                   pszDisplayName);      /* Name to display. */
                if (fResult)
                    _tprintf(_T("The service config has been successfully updated.\n"));
                else
                {
                    dwErr = GetLastError();
                    _tprintf(_T("Could not change service config! Error: %ld\n"), dwErr);
                }
                CloseServiceHandle(hService);
            }

            /*
             * This entire branch do not return an error to avoid installations failures,
             * if updating service parameters. Better to have a running system with old
             * parameters and the failure information in the installation log.
             */
            break;
        }

        case ERROR_INVALID_PARAMETER:

            _tprintf(_T("Invalid parameter specified!\n"));
            rc = EXIT_FAIL;
            break;

        default:

            _tprintf(_T("Could not create service! Error: %ld\n"), dwErr);
            rc = EXIT_FAIL;
            break;
        }

        if (rc == EXIT_FAIL)
            goto cleanup;
    }
    else
    {
        CloseServiceHandle (hService);
        _tprintf(_T("Installation of service successful!\n"));
    }

cleanup:

    if (hSCManager != NULL)
        CloseServiceHandle (hSCManager);

    return rc;
}

int DelService(const TCHAR *pszStartStopName)
{
    int rc = ERROR_SUCCESS;

    _tprintf(_T("Deleting service '%ws' ...\n"), pszStartStopName);

    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    SC_HANDLE hService = NULL;
    if (hSCManager == NULL)
    {
        _tprintf(_T("Could not get handle to SCM! Error: %ld\n"), GetLastError());
        rc = EXIT_FAIL;
    }
    else
    {
        hService = OpenService(hSCManager, pszStartStopName, SERVICE_ALL_ACCESS);
        if (NULL == hService)
        {
            _tprintf(_T("Could not open service '%ws'! Error: %ld\n"), pszStartStopName, GetLastError());
            rc = EXIT_FAIL;
        }
    }

    if (hService != NULL)
    {
        SC_LOCK hSCLock = LockServiceDatabase(hSCManager);
        if (hSCLock != NULL)
        {
            if (FALSE == DeleteService(hService))
            {
                DWORD dwErr = GetLastError();
                switch (dwErr)
                {

                case ERROR_SERVICE_MARKED_FOR_DELETE:

                    _tprintf(_T("Service '%ws' already marked for deletion.\n"), pszStartStopName);
                    break;

                default:

                    _tprintf(_T("Could not delete service '%ws'! Error: %ld\n"), pszStartStopName, GetLastError());
                    rc = EXIT_FAIL;
                    break;
                }
            }
            else
            {
                _tprintf(_T("Service '%ws' successfully removed!\n"), pszStartStopName);
            }
            UnlockServiceDatabase(hSCLock);
        }
        else
        {
            _tprintf(_T("Unable to lock service database! Error: %ld\n"), GetLastError());
            rc = EXIT_FAIL;
        }
        CloseServiceHandle(hService);
    }

    if (hSCManager != NULL)
        CloseServiceHandle(hSCManager);

    return rc;
}

DWORD RegistryWrite(HKEY hRootKey,
                    const _TCHAR *pszSubKey,
                    const _TCHAR *pszValueName,
                    DWORD dwType,
                    const BYTE *pbData,
                    DWORD cbData)
{
    DWORD lRet;
    HKEY hKey;
    lRet = RegCreateKeyEx (hRootKey,
                           pszSubKey,
                           0,           /* Reserved */
                           NULL,        /* lpClass [in, optional] */
                           0,           /* dwOptions [in] */
                           KEY_WRITE,
                           NULL,        /* lpSecurityAttributes [in, optional] */
                           &hKey,
                           NULL);       /* lpdwDisposition [out, optional] */
    if (lRet != ERROR_SUCCESS)
    {
        _tprintf(_T("Could not open registry key! Error: %ld\n"), GetLastError());
    }
    else
    {
        lRet = RegSetValueEx(hKey, pszValueName, 0, dwType, (BYTE*)pbData, cbData);
        if (lRet != ERROR_SUCCESS)
            _tprintf(_T("Could not write to registry! Error: %ld\n"), GetLastError());
        RegCloseKey(hKey);

    }
    return lRet;
}

void PrintHelp(void)
{
    _tprintf(_T("VirtualBox Guest Additions Installation Helper for Windows\n"));
    _tprintf(_T("Version: %d.%d.%d.%d\n\n"), VBOX_VERSION_MAJOR, VBOX_VERSION_MINOR, VBOX_VERSION_BUILD, VBOX_SVN_REV);
    _tprintf(_T("Syntax:\n"));
    _tprintf(_T("\n"));
    _tprintf(_T("Drivers:\n"));
    _tprintf(_T("\tVBoxDrvInst         driver install <inf-file> [log file]\n"));
    _tprintf(_T("\tVBoxDrvInst         driver uninstall <inf-file> [log file]\n"));
    _tprintf(_T("\tVBoxDrvInst         driver executeinf <inf-file>\n"));
    _tprintf(_T("\n"));
    _tprintf(_T("Network Provider:\n"));
    _tprintf(_T("\tVBoxDrvInst         netprovider add <name> [order]\n"));
    _tprintf(_T("\tVBoxDrvInst         netprovider remove <name>\n"));
    _tprintf(_T("\n"));
    _tprintf(_T("Registry:\n"));
    _tprintf(_T("\tVBoxDrvInst         registry write <root> <sub key>\n")
             _T("\t                    <key name> <key type> <value>\n")
             _T("\t                    [type] [size]\n"));
    _tprintf(_T("\tVBoxDrvInst         registry addmultisz <root> <sub key>\n")
             _T("\t                    <value> [order]\n"));
    _tprintf(_T("\tVBoxDrvInst         registry delmultisz <root> <sub key>\n")
             _T("\t                    <key name> <value to remove>\n"));
    /** @todo Add "service" category! */
    _tprintf(_T("\n"));
}

int __cdecl _tmain(int argc, _TCHAR *argv[])
{
    int rc = EXIT_USAGE;

    OSVERSIONINFO OSinfo;
    OSinfo.dwOSVersionInfoSize = sizeof(OSinfo);
    GetVersionEx(&OSinfo);

    if (argc >= 2)
    {
        if (   !_tcsicmp(argv[1], _T("driver"))
            && argc >= 3)
        {
            _TCHAR szINF[_MAX_PATH] = { 0 }; /* Complete path to INF file.*/
            if (   (   !_tcsicmp(argv[2], _T("install"))
                    || !_tcsicmp(argv[2], _T("uninstall")))
                && argc >= 4)
            {
                if (OSinfo.dwMajorVersion < 5)
                {
                    _tprintf(_T("ERROR: Platform not supported for driver (un)installation!\n"));
                    rc = EXIT_FAIL;
                }
                else
                {
                    _sntprintf(szINF, sizeof(szINF) / sizeof(TCHAR), _T("%ws"), argv[3]);

                    _TCHAR szLogFile[_MAX_PATH] = { 0 };
                    if (argc > 4)
                        _sntprintf(szLogFile, sizeof(szLogFile) / sizeof(TCHAR), _T("%ws"), argv[4]);
                    rc = VBoxInstallDriver(!_tcsicmp(argv[2], _T("install")) ? TRUE : FALSE, szINF,
                                           FALSE /* Not silent */, szLogFile[0] != NULL ? szLogFile : NULL);
                }
            }
            else if (   !_tcsicmp(argv[2], _T("executeinf"))
                     && argc == 4)
            {
                _sntprintf(szINF, sizeof(szINF) / sizeof(TCHAR), _T("%ws"), argv[3]);
                rc = ExecuteInfFile(_T("DefaultInstall"), 132, szINF);
            }
        }
        else if (   !_tcsicmp(argv[1], _T("netprovider"))
                 && argc >= 3)
        {
            _TCHAR szProvider[_MAX_PATH] = { 0 }; /* The network provider name for the registry. */
            if (  !_tcsicmp(argv[2], _T("add"))
                && argc >= 4)
            {
                int iOrder = 0;
                if (argc > 4)
                    iOrder = _ttoi(argv[4]);
                _sntprintf(szProvider, sizeof(szProvider) / sizeof(TCHAR), _T("%ws"), argv[3]);
                rc = AddNetworkProvider(szProvider, iOrder);
            }
            else if (  !_tcsicmp(argv[2], _T("remove"))
                && argc >= 4)
            {
                _sntprintf(szProvider, sizeof(szProvider) / sizeof(TCHAR), _T("%ws"), argv[3]);
                rc = RemoveNetworkProvider(szProvider);
            }
        }
        else if (   !_tcsicmp(argv[1], _T("service"))
                 && argc >= 3)
        {
            if (   !_tcsicmp(argv[2], _T("create"))
                && argc >= 8)
            {
                rc = CreateService(argv[3],
                                   argv[4],
                                   _ttoi(argv[5]),
                                   _ttoi(argv[6]),
                                   argv[7],
                                   (argc > 8) ? argv[8] : NULL,
                                   (argc > 9) ? argv[9] : NULL,
                                   (argc > 10) ? argv[10] : NULL,
                                   (argc > 11) ? argv[11] : NULL);
            }
            else if (   !_tcsicmp(argv[2], _T("delete"))
                     && argc == 4)
            {
                rc = DelService(argv[3]);
            }
        }
        else if (   !_tcsicmp(argv[1], _T("registry"))
                 && argc >= 3)
        {
            /** @todo add a handleRegistry(argc, argv) method to keep things cleaner */
            if (   !_tcsicmp(argv[2], _T("addmultisz"))
                && argc == 7)
            {
                rc = RegistryAddStringToMultiSZ(argv[3], argv[4], argv[5], _ttoi(argv[6]));
            }
            else if (   !_tcsicmp(argv[2], _T("delmultisz"))
                     && argc == 6)
            {
                rc = RegistryRemoveStringFromMultiSZ(argv[3], argv[4], argv[5]);
            }
            else if (   !_tcsicmp(argv[2], _T("write"))
                     && argc >= 8)
            {
                HKEY hRootKey = HKEY_LOCAL_MACHINE;  /** @todo needs to be expanded (argv[3]) */
                DWORD dwValSize = 0;
                BYTE *pbVal = NULL;
                DWORD dwVal;

                if (argc > 8)
                {
                    if (!_tcsicmp(argv[8], _T("dword")))
                    {
                        dwVal = _ttol(argv[7]);
                        pbVal = (BYTE*)&dwVal;
                        dwValSize = sizeof(DWORD);
                    }
                }
                if (pbVal == NULL) /* By default interpret value as string */
                {
                    pbVal = (BYTE *)argv[7];
                    dwValSize = (DWORD)_tcslen(argv[7]);
                }
                if (argc > 9)
                    dwValSize = _ttol(argv[9]);      /* Get the size in bytes of the value we want to write */
                rc = RegistryWrite(hRootKey,
                                   argv[4],          /* Sub key */
                                   argv[5],          /* Value name */
                                   REG_BINARY,       /** @todo needs to be expanded (argv[6]) */
                                   pbVal,            /* The value itself */
                                   dwValSize);       /* Size of the value */
            }
#if 0
            else if (!_tcsicmp(argv[2], _T("read")))
            {
            }
            else if (!_tcsicmp(argv[2], _T("del")))
            {
            }
#endif
        }
        else if (!_tcsicmp(argv[1], _T("--version")))
        {
            _tprintf(_T("%d.%d.%d.%d\n"), VBOX_VERSION_MAJOR, VBOX_VERSION_MINOR, VBOX_VERSION_BUILD, VBOX_SVN_REV);
            rc = EXIT_OK;
        }
        else if (   !_tcsicmp(argv[1], _T("--help"))
                 || !_tcsicmp(argv[1], _T("/help"))
                 || !_tcsicmp(argv[1], _T("/h"))
                 || !_tcsicmp(argv[1], _T("/?")))
        {
            PrintHelp();
            rc = EXIT_OK;
        }
    }

    if (rc == EXIT_USAGE)
        _tprintf(_T("No or wrong parameters given! Please consult the help (\"--help\" or \"/?\") for more information.\n"));

    return rc;
}

