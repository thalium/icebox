/* $Id: VBoxDrvCfg.cpp $ */
/** @file
 * VBoxDrvCfg.cpp - Windows Driver Manipulation API implementation
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
#include <VBox/VBoxDrvCfg-win.h>

#include <iprt/win/setupapi.h>
#include <iprt/win/shlobj.h>

#include <string.h>

#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>

#include <Newdev.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static PFNVBOXDRVCFG_LOG g_pfnVBoxDrvCfgLog;
static void *g_pvVBoxDrvCfgLog;

static PFNVBOXDRVCFG_PANIC g_pfnVBoxDrvCfgPanic;
static void *g_pvVBoxDrvCfgPanic;


VBOXDRVCFG_DECL(void) VBoxDrvCfgLoggerSet(PFNVBOXDRVCFG_LOG pfnLog, void *pvLog)
{
    g_pfnVBoxDrvCfgLog = pfnLog;
    g_pvVBoxDrvCfgLog = pvLog;
}

VBOXDRVCFG_DECL(void) VBoxDrvCfgPanicSet(PFNVBOXDRVCFG_PANIC pfnPanic, void *pvPanic)
{
    g_pfnVBoxDrvCfgPanic = pfnPanic;
    g_pvVBoxDrvCfgPanic = pvPanic;
}

static void vboxDrvCfgLogRel(LPCSTR szString, ...)
{
    PFNVBOXDRVCFG_LOG pfnLog = g_pfnVBoxDrvCfgLog;
    void * pvLog = g_pvVBoxDrvCfgLog;
    if (pfnLog)
    {
        char szBuffer[4096] = {0};
        va_list pArgList;
        va_start(pArgList, szString);
        _vsnprintf(szBuffer, RT_ELEMENTS(szBuffer), szString, pArgList);
        va_end(pArgList);
        pfnLog(VBOXDRVCFG_LOG_SEVERITY_REL, szBuffer, pvLog);
    }
}

static void vboxDrvCfgLogRegular(LPCSTR szString, ...)
{
    PFNVBOXDRVCFG_LOG pfnLog = g_pfnVBoxDrvCfgLog;
    void * pvLog = g_pvVBoxDrvCfgLog;
    if (pfnLog)
    {
        char szBuffer[4096] = {0};
        va_list pArgList;
        va_start(pArgList, szString);
        _vsnprintf(szBuffer, RT_ELEMENTS(szBuffer), szString, pArgList);
        va_end(pArgList);
        pfnLog(VBOXDRVCFG_LOG_SEVERITY_REGULAR, szBuffer, pvLog);
    }
}

static void vboxDrvCfgLogFlow(LPCSTR szString, ...)
{
    PFNVBOXDRVCFG_LOG pfnLog = g_pfnVBoxDrvCfgLog;
    void * pvLog = g_pvVBoxDrvCfgLog;
    if (pfnLog)
    {
        char szBuffer[4096] = {0};
        va_list pArgList;
        va_start(pArgList, szString);
        _vsnprintf(szBuffer, RT_ELEMENTS(szBuffer), szString, pArgList);
        va_end(pArgList);
        pfnLog(VBOXDRVCFG_LOG_SEVERITY_FLOW, szBuffer, pvLog);
    }
}

static void vboxDrvCfgPanic()
{
    PFNVBOXDRVCFG_PANIC pfnPanic = g_pfnVBoxDrvCfgPanic;
    void * pvPanic = g_pvVBoxDrvCfgPanic;
    if (pfnPanic)
    {
        pfnPanic(pvPanic);
    }
}

/* we do not use IPRT Logging because the lib is used in host installer and needs to
 * post its msgs to MSI logger */
#define NonStandardLogCrap(_m)     do { vboxDrvCfgLogRegular _m ; } while (0)
#define NonStandardLogFlowCrap(_m) do { vboxDrvCfgLogFlow _m ; } while (0)
#define NonStandardLogRelCrap(_m)  do { vboxDrvCfgLogRel _m ; } while (0)
#define NonStandardAssertFailed() vboxDrvCfgPanic()
#define NonStandardAssert(_m) do { \
        if (RT_UNLIKELY(!(_m))) {  vboxDrvCfgPanic(); } \
    } while (0)


class VBoxDrvCfgStringList
{
public:
    VBoxDrvCfgStringList(int aSize);

    ~VBoxDrvCfgStringList();

    HRESULT add(LPWSTR pStr);

    int size() {return mSize;}

    LPWSTR get(int i) {return maList[i];}
private:
    HRESULT resize(int newSize);

    LPWSTR *maList;
    int mBufSize;
    int mSize;
};

VBoxDrvCfgStringList::VBoxDrvCfgStringList(int aSize)
{
    maList = (LPWSTR*)malloc( sizeof(maList[0]) * aSize);
    mBufSize = aSize;
    mSize = 0;
}

VBoxDrvCfgStringList::~VBoxDrvCfgStringList()
{
    if (!mBufSize)
        return;

    for (int i = 0; i < mSize; ++i)
    {
        free(maList[i]);
    }

    free(maList);
}

HRESULT VBoxDrvCfgStringList::add(LPWSTR pStr)
{
    if (mSize == mBufSize)
    {
        int hr = resize(mBufSize+10);
        if (SUCCEEDED(hr))
            return hr;
    }
    size_t cStr = wcslen(pStr) + 1;
    LPWSTR str = (LPWSTR)malloc( sizeof(maList[0][0]) * cStr);
    memcpy(str, pStr, sizeof(maList[0][0]) * cStr);
    maList[mSize] = str;
    ++mSize;
    return S_OK;
}

HRESULT VBoxDrvCfgStringList::resize(int newSize)
{
    NonStandardAssert(newSize >= mSize);
    if (newSize < mSize)
        return E_FAIL;
    LPWSTR* pOld = maList;
    maList = (LPWSTR*)malloc( sizeof(maList[0]) * newSize);
    mBufSize = newSize;
    memcpy(maList, pOld, mSize*sizeof(maList[0]));
    free(pOld);
    return S_OK;
}

/*
 * inf file manipulation API
 */
typedef bool (*PFNVBOXNETCFG_ENUMERATION_CALLBACK) (LPCWSTR lpszFileName, PVOID pContext);

typedef struct _INF_INFO
{
    LPCWSTR lpszClassName;
    LPCWSTR lpszPnPId;
} INF_INFO, *PINF_INFO;

typedef struct _INFENUM_CONTEXT
{
    INF_INFO InfInfo;
    DWORD Flags;
    HRESULT hr;
} INFENUM_CONTEXT, *PINFENUM_CONTEXT;

static HRESULT vboxDrvCfgInfQueryContext(HINF hInf, LPCWSTR lpszSection, LPCWSTR lpszKey, PINFCONTEXT pCtx)
{
    if (!SetupFindFirstLineW(hInf, lpszSection, lpszKey, pCtx))
    {
        DWORD dwErr = GetLastError();
        NonStandardLogRelCrap((__FUNCTION__ ": SetupFindFirstLine failed WinEr (%d) for Section(%S), Key(%S)\n", dwErr, lpszSection, lpszKey));
        return HRESULT_FROM_WIN32(dwErr);
    }
    return S_OK;
}

static HRESULT vboxDrvCfgInfQueryKeyValue(PINFCONTEXT pCtx, DWORD iValue, LPWSTR *lppszValue, PDWORD pcValue)
{
    DWORD dwErr;
    DWORD cValue;

    if (!SetupGetStringFieldW(pCtx, iValue, NULL, 0, &cValue))
    {
        dwErr = GetLastError();
//        NonStandardAssert(dwErr == ERROR_INSUFFICIENT_BUFFER);
        if (dwErr != ERROR_INSUFFICIENT_BUFFER)
        {
            NonStandardLogFlowCrap((__FUNCTION__ ": SetupGetStringField failed WinEr (%d) for iValue(%d)\n", dwErr, iValue));
            return HRESULT_FROM_WIN32(dwErr);
        }
    }

    LPWSTR lpszValue = (LPWSTR)malloc(cValue * sizeof (lpszValue[0]));
    NonStandardAssert(lpszValue);
    if (!lpszValue)
    {
        NonStandardLogRelCrap((__FUNCTION__ ": SetCoTaskMemAlloc failed to alloc mem of size (%d), for iValue(%d)\n", cValue * sizeof (lpszValue[0]), iValue));
        return E_FAIL;
    }

    if (!SetupGetStringFieldW(pCtx, iValue, lpszValue, cValue, &cValue))
    {
        dwErr = GetLastError();
        NonStandardLogRelCrap((__FUNCTION__ ": SetupGetStringField failed WinEr (%d) for iValue(%d)\n", dwErr, iValue));
        NonStandardAssert(0);
        free(lpszValue);
        return HRESULT_FROM_WIN32(dwErr);
    }

    *lppszValue = lpszValue;
    if (pcValue)
        *pcValue = cValue;
    return S_OK;
}
#if defined(RT_ARCH_AMD64)
# define VBOXDRVCFG_ARCHSTR L"amd64"
#else
# define VBOXDRVCFG_ARCHSTR L"x86"
#endif

static HRESULT vboxDrvCfgInfQueryModelsSectionName(HINF hInf, LPWSTR *lppszValue, PDWORD pcValue)
{
    INFCONTEXT InfCtx;
    LPWSTR lpszModels, lpszPlatform = NULL, lpszPlatformCur;
    LPWSTR lpszResult = NULL;
    DWORD cModels, cPlatform = 0, cPlatformCur, cResult = 0;
    bool bNt = false, bArch = false /*, bOs = false */;

    HRESULT hr = vboxDrvCfgInfQueryContext(hInf, L"Manufacturer", NULL, &InfCtx);
    if (hr != S_OK)
    {
        NonStandardLogCrap((__FUNCTION__ ": vboxDrvCfgInfQueryContext for Manufacturer failed, hr=0x%x\n", hr));
        return hr;
    }

    hr = vboxDrvCfgInfQueryKeyValue(&InfCtx, 1, &lpszModels, &cModels);
    if (hr != S_OK)
    {
        NonStandardLogRelCrap((__FUNCTION__ ": vboxDrvCfgRegQueryKeyValue 1 for Manufacturer failed, hr=0x%x\n", hr));
        return hr;
    }

    for (DWORD i = 2; (hr = vboxDrvCfgInfQueryKeyValue(&InfCtx, i, &lpszPlatformCur, &cPlatformCur)) == S_OK; ++i)
    {
        if (wcsicmp(lpszPlatformCur, L"NT"VBOXDRVCFG_ARCHSTR))
        {
            if (bNt)
            {
                free(lpszPlatformCur);
                lpszPlatformCur = NULL;
                continue;
            }

            if (wcsicmp(lpszPlatformCur, L"NT"))
            {
                free(lpszPlatformCur);
                lpszPlatformCur = NULL;
                continue;
            }

            bNt = true;
        }
        else
        {
            bArch = true;
        }

        cPlatform = cPlatformCur;
        if(lpszPlatform)
            free(lpszPlatform);
        lpszPlatform = lpszPlatformCur;
        lpszPlatformCur = NULL;
    }

    hr = S_OK;

    if (lpszPlatform)
    {
        lpszResult = (LPWSTR)malloc((cModels + cPlatform) * sizeof (lpszResult[0]));
        if (lpszResult)
        {
            memcpy(lpszResult, lpszModels, (cModels - 1) * sizeof (lpszResult[0]));
            *(lpszResult + cModels - 1) = L'.';
            memcpy(lpszResult + cModels, lpszPlatform, cPlatform * sizeof (lpszResult[0]));
            cResult = cModels + cPlatform;
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        lpszResult = lpszModels;
        cResult = cModels;
        lpszModels = NULL;
    }

    if (lpszModels)
        free(lpszModels);
    if (lpszPlatform)
        free(lpszPlatform);

    if (hr == S_OK)
    {
        *lppszValue = lpszResult;
        if (pcValue)
            *pcValue = cResult;
    }

    return hr;
}

static HRESULT vboxDrvCfgInfQueryFirstPnPId(HINF hInf, LPWSTR *lppszPnPId)
{
    *lppszPnPId = NULL;

    LPWSTR lpszModels;
    HRESULT hr = vboxDrvCfgInfQueryModelsSectionName(hInf, &lpszModels, NULL);
    NonStandardLogRelCrap((__FUNCTION__ ": vboxDrvCfgInfQueryModelsSectionName returned lpszModels = (%S)", lpszModels));
    if (hr != S_OK)
    {
        NonStandardLogCrap((__FUNCTION__ ": vboxDrvCfgRegQueryKeyValue for Manufacturer failed, hr=0x%x\n", hr));
        return hr;
    }

    LPWSTR lpszPnPId = NULL;
    INFCONTEXT InfCtx;
    hr = vboxDrvCfgInfQueryContext(hInf, lpszModels, NULL, &InfCtx);
    if (hr != S_OK)
    {
        NonStandardLogRelCrap((__FUNCTION__ ": vboxDrvCfgInfQueryContext for models (%S) failed, hr=0x%x\n", lpszModels, hr));
    }
    else
    {
        hr = vboxDrvCfgInfQueryKeyValue(&InfCtx, 2, &lpszPnPId, NULL);
        NonStandardLogRelCrap((__FUNCTION__ ": vboxDrvCfgRegQueryKeyValue for models (%S) returned lpszPnPId (%S) \n", lpszModels, lpszPnPId));

        if (hr != S_OK)
            NonStandardLogRelCrap((__FUNCTION__ ": vboxDrvCfgRegQueryKeyValue for models (%S) failed, hr=0x%x\n", lpszModels, hr));
    }
    /* free models string right away */
    free(lpszModels);
    if (hr != S_OK)
        return hr;

    *lppszPnPId = lpszPnPId;
    return S_OK;
}

static bool vboxDrvCfgInfEnumerationCallback(LPCWSTR lpszFileName, PVOID pCtxt);

#define VBOXDRVCFG_S_INFEXISTS (HRESULT_FROM_WIN32(ERROR_FILE_EXISTS))

static HRESULT vboxDrvCfgInfCopyEx(IN LPCWSTR lpszInfPath, IN DWORD fCopyStyle, OUT LPWSTR lpszDstName, IN DWORD cbDstName, OUT PDWORD pcbDstNameSize, OUT LPWSTR* lpszDstNameComponent)
{
    WCHAR aMediaLocation[_MAX_DIR];
    WCHAR aDir[_MAX_DIR];

    _wsplitpath(lpszInfPath, aMediaLocation, aDir, NULL, NULL);
    wcscat(aMediaLocation, aDir);

    if (!SetupCopyOEMInfW(lpszInfPath, aMediaLocation, SPOST_PATH, fCopyStyle,
            lpszDstName, cbDstName, pcbDstNameSize,
            lpszDstNameComponent))
    {
        DWORD dwErr = GetLastError();
        HRESULT hr = HRESULT_FROM_WIN32(dwErr);
        if (fCopyStyle != SP_COPY_REPLACEONLY || hr != VBOXDRVCFG_S_INFEXISTS)
        {
            NonStandardLogRelCrap((__FUNCTION__ ": SetupCopyOEMInf fail dwErr=%ld\n", dwErr));
        }
        return hr;
    }

    return S_OK;
}

static HRESULT vboxDrvCfgInfCopy(IN LPCWSTR lpszInfPath)
{
    return vboxDrvCfgInfCopyEx(lpszInfPath, 0, NULL, 0, NULL, NULL);
}

VBOXDRVCFG_DECL(HRESULT) VBoxDrvCfgInfInstall(IN LPCWSTR lpszInfPath)
{
    return vboxDrvCfgInfCopy(lpszInfPath);
}

VBOXDRVCFG_DECL(HRESULT) VBoxDrvCfgInfUninstall(IN LPCWSTR lpszInfPath, DWORD fFlags)
{
    WCHAR DstInfName[MAX_PATH];
    DWORD cbDword = sizeof (DstInfName);
    HRESULT hr = vboxDrvCfgInfCopyEx(lpszInfPath, SP_COPY_REPLACEONLY, DstInfName, cbDword, &cbDword, NULL);
    if (hr == VBOXDRVCFG_S_INFEXISTS)
    {
        if (!SetupUninstallOEMInfW(DstInfName, fFlags, NULL /*__in PVOID Reserved == NULL */))
        {
            DWORD dwErr = GetLastError();
            NonStandardLogRelCrap((__FUNCTION__ ": SetupUninstallOEMInf failed for file (%S), oem(%S), dwErr=%ld\n", lpszInfPath, DstInfName, dwErr));
            NonStandardAssert(0);
            return HRESULT_FROM_WIN32(dwErr);
        }
    }
    return S_OK;
}


static HRESULT vboxDrvCfgCollectInfsSetupDi(const GUID * pGuid, LPCWSTR pPnPId, VBoxDrvCfgStringList & list)
{
    DWORD dwErr = ERROR_SUCCESS;
    HDEVINFO hDevInfo = SetupDiCreateDeviceInfoList(
                            pGuid, /* IN LPGUID ClassGuid, OPTIONAL */
                            NULL /*IN HWND hwndParent OPTIONAL */
                            );
    if (hDevInfo != INVALID_HANDLE_VALUE)
    {
        if (SetupDiBuildDriverInfoList(hDevInfo,
                    NULL, /*IN OUT PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL*/
                    SPDIT_CLASSDRIVER  /*IN DWORD DriverType*/
                    ))
        {
            SP_DRVINFO_DATA DrvInfo;
            DrvInfo.cbSize = sizeof(SP_DRVINFO_DATA);
            char DetailBuf[16384];
            PSP_DRVINFO_DETAIL_DATA pDrvDetail = (PSP_DRVINFO_DETAIL_DATA)DetailBuf;

            for (DWORD i = 0; ; i++)
            {
                if (SetupDiEnumDriverInfo(hDevInfo,
                        NULL, /* IN PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL*/
                        SPDIT_CLASSDRIVER , /*IN DWORD DriverType,*/
                        i, /*IN DWORD MemberIndex,*/
                        &DrvInfo /*OUT PSP_DRVINFO_DATA DriverInfoData*/
                        ))
                {
                    DWORD dwReq;
                    pDrvDetail->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
                    if (SetupDiGetDriverInfoDetail(
                            hDevInfo, /*IN HDEVINFO DeviceInfoSet,*/
                            NULL, /*IN PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL*/
                            &DrvInfo, /*IN PSP_DRVINFO_DATA DriverInfoData,*/
                            pDrvDetail, /*OUT PSP_DRVINFO_DETAIL_DATA DriverInfoDetailData, OPTIONAL*/
                            sizeof(DetailBuf), /*IN DWORD DriverInfoDetailDataSize,*/
                            &dwReq /*OUT PDWORD RequiredSize OPTIONAL*/
                            ))
                    {
                        for (WCHAR * pHwId = pDrvDetail->HardwareID; pHwId && *pHwId && pHwId < (TCHAR*)(DetailBuf + sizeof(DetailBuf)/sizeof(DetailBuf[0])) ;pHwId += wcslen(pHwId) + 1)
                        {
                            if (!wcsicmp(pHwId, pPnPId))
                            {
                                NonStandardAssert(pDrvDetail->InfFileName[0]);
                                if (pDrvDetail->InfFileName)
                                {
                                    list.add(pDrvDetail->InfFileName);
                                    NonStandardLogRelCrap((__FUNCTION__": %S added to list", pDrvDetail->InfFileName));
                                }
                            }
                        }
                    }
                    else
                    {
                        DWORD dwErr = GetLastError();
                        NonStandardLogRelCrap((__FUNCTION__": SetupDiGetDriverInfoDetail fail dwErr=%ld, size(%d)", dwErr, dwReq));
//                        NonStandardAssert(0);
                    }

                }
                else
                {
                    DWORD dwErr = GetLastError();
                    if (dwErr == ERROR_NO_MORE_ITEMS)
                    {
                        NonStandardLogRelCrap((__FUNCTION__": dwErr == ERROR_NO_MORE_ITEMS -> search was finished "));
                        break;
                    }

                    NonStandardAssert(0);
                }
            }

            SetupDiDestroyDriverInfoList(hDevInfo,
                      NULL, /*IN PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL*/
                      SPDIT_CLASSDRIVER/*IN DWORD DriverType*/
                      );
        }
        else
        {
            dwErr = GetLastError();
            NonStandardAssert(0);
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
    else
    {
        dwErr = GetLastError();
        NonStandardAssert(0);
    }

    return HRESULT_FROM_WIN32(dwErr);
}

#if 0
VBOXDRVCFG_DECL(HRESULT) VBoxDrvCfgInit()
{
    int rc = RTR3InitDll(0);
    if (rc != VINF_SUCCESS)
    {
        NonStandardLogRelCrap(("Could not init IPRT!, rc (%d)\n", rc));
        return E_FAIL;
    }

    return S_OK;
}

VBOXDRVCFG_DECL(HRESULT) VBoxDrvCfgTerm()
{
    return S_OK;
}
#endif

VBOXDRVCFG_DECL(HRESULT) VBoxDrvCfgInfUninstallAllSetupDi(IN const GUID * pGuidClass, IN LPCWSTR lpszClassName, IN LPCWSTR lpszPnPId, IN DWORD Flags)
{
    VBoxDrvCfgStringList list(128);
    HRESULT hr = vboxDrvCfgCollectInfsSetupDi(pGuidClass, lpszPnPId, list);
    NonStandardLogRelCrap((__FUNCTION__": vboxDrvCfgCollectInfsSetupDi returned %d devices with PnPId %S and class name %S", list.size(), lpszPnPId, lpszClassName));
    if (hr == S_OK)
    {
        INFENUM_CONTEXT Context;
        Context.InfInfo.lpszClassName = lpszClassName;
        Context.InfInfo.lpszPnPId = lpszPnPId;
        Context.Flags = Flags;
        Context.hr = S_OK;
        int size = list.size();
        for (int i = 0; i < size; ++i)
        {
            LPCWSTR pInf = list.get(i);
            const WCHAR* pRel = wcsrchr(pInf, '\\');
            if (pRel)
                ++pRel;
            else
                pRel = pInf;

            vboxDrvCfgInfEnumerationCallback(pRel, &Context);
            NonStandardLogRelCrap((__FUNCTION__": inf = %S\n", list.get(i)));
        }
    }
    return hr;
}

static HRESULT vboxDrvCfgEnumFiles(LPCWSTR pPattern, PFNVBOXNETCFG_ENUMERATION_CALLBACK pfnCallback, PVOID pContext)
{
    WIN32_FIND_DATA Data;
    memset(&Data, 0, sizeof(Data));
    HRESULT hr = S_OK;

    HANDLE hEnum = FindFirstFile(pPattern,&Data);
    if (hEnum != INVALID_HANDLE_VALUE)
    {

        do
        {
            if (!pfnCallback(Data.cFileName, pContext))
            {
                break;
            }

            /* next iteration */
            memset(&Data, 0, sizeof(Data));
            BOOL bNext = FindNextFile(hEnum,&Data);
            if (!bNext)
            {
                DWORD dwErr = GetLastError();
                if (dwErr != ERROR_NO_MORE_FILES)
                {
                    NonStandardLogRelCrap((__FUNCTION__": FindNextFile fail dwErr=%ld\n", dwErr));
                    NonStandardAssert(0);
                    hr = HRESULT_FROM_WIN32(dwErr);
                }
                break;
            }
        }while (true);
        FindClose(hEnum);
    }
    else
    {
        DWORD dwErr = GetLastError();
        if (dwErr != ERROR_NO_MORE_FILES)
        {
            NonStandardLogRelCrap((__FUNCTION__": FindFirstFile fail dwErr=%ld\n", dwErr));
            NonStandardAssert(0);
            hr = HRESULT_FROM_WIN32(dwErr);
        }
    }

    return hr;
}

static bool vboxDrvCfgInfEnumerationCallback(LPCWSTR lpszFileName, PVOID pCtxt)
{
    PINFENUM_CONTEXT pContext = (PINFENUM_CONTEXT)pCtxt;
    DWORD dwErr;
    NonStandardLogRelCrap((__FUNCTION__": lpszFileName (%S)\n", lpszFileName));
    NonStandardLogRelCrap((__FUNCTION__ ": pContext->InfInfo.lpszClassName = (%S)\n", pContext->InfInfo.lpszClassName));
    HINF hInf = SetupOpenInfFileW(lpszFileName, pContext->InfInfo.lpszClassName, INF_STYLE_WIN4, NULL /*__in PUINT ErrorLine */);
    if (hInf == INVALID_HANDLE_VALUE)
    {
        dwErr = GetLastError();
//        NonStandardAssert(dwErr == ERROR_CLASS_MISMATCH);
        if (dwErr != ERROR_CLASS_MISMATCH)
        {
            NonStandardLogCrap((__FUNCTION__ ": SetupOpenInfFileW err dwErr=%ld\n", dwErr));
        }
        else
        {
            NonStandardLogCrap((__FUNCTION__ ": dwErr == ERROR_CLASS_MISMATCH\n"));
        }
        return true;
    }

    LPWSTR lpszPnPId;
    HRESULT hr = vboxDrvCfgInfQueryFirstPnPId(hInf, &lpszPnPId);
    NonStandardLogRelCrap((__FUNCTION__ ": vboxDrvCfgInfQueryFirstPnPId returned lpszPnPId = (%S)\n", lpszPnPId));
    NonStandardLogRelCrap((__FUNCTION__ ": pContext->InfInfo.lpszPnPId = (%S)\n", pContext->InfInfo.lpszPnPId));
    if (hr == S_OK)
    {
        if (!wcsicmp(pContext->InfInfo.lpszPnPId, lpszPnPId))
        {
            if (!SetupUninstallOEMInfW(lpszFileName,
                        pContext->Flags, /*DWORD Flags could be SUOI_FORCEDELETE */
                        NULL /*__in PVOID Reserved == NULL */
                        ))
            {
                dwErr = GetLastError();
                NonStandardLogRelCrap((__FUNCTION__ ": SetupUninstallOEMInf failed for file (%S), dwErr=%ld\n", lpszFileName, dwErr));
                NonStandardAssert(0);
                hr = HRESULT_FROM_WIN32( dwErr );
            }
        }

        free(lpszPnPId);
    }
    else
    {
        NonStandardLogCrap((__FUNCTION__ ": vboxDrvCfgInfQueryFirstPnPId failed, hr=0x%x\n", hr));
    }

    SetupCloseInfFile(hInf);

    return true;
}

VBOXDRVCFG_DECL(HRESULT) VBoxDrvCfgInfUninstallAllF(LPCWSTR lpszClassName, LPCWSTR lpszPnPId, DWORD Flags)
{
    static WCHAR const s_wszFilter[] = L"\\inf\\oem*.inf";
    HRESULT hr;
    WCHAR wszInfDirPath[MAX_PATH];
    UINT cwcInput = RT_ELEMENTS(wszInfDirPath) - RT_ELEMENTS(s_wszFilter);
    UINT cwcWindows = GetSystemWindowsDirectory(wszInfDirPath, cwcInput);
    if (cwcWindows > 0 && cwcWindows < cwcInput)
    {
        wcscpy(&wszInfDirPath[cwcWindows], s_wszFilter);

        INFENUM_CONTEXT Context;
        Context.InfInfo.lpszClassName = lpszClassName;
        Context.InfInfo.lpszPnPId = lpszPnPId;
        Context.Flags = Flags;
        Context.hr = S_OK;
        NonStandardLogRelCrap((__FUNCTION__": Calling vboxDrvCfgEnumFiles(wszInfDirPath, vboxDrvCfgInfEnumerationCallback, &Context)"));
        hr = vboxDrvCfgEnumFiles(wszInfDirPath, vboxDrvCfgInfEnumerationCallback, &Context);
        NonStandardAssert(hr == S_OK);
        if (hr == S_OK)
        {
            hr = Context.hr;
        }
        else
        {
            NonStandardLogRelCrap((__FUNCTION__": vboxDrvCfgEnumFiles failed, hr=0x%x\n", hr));
        }
    }
    else
    {
        NonStandardLogRelCrap((__FUNCTION__": GetSystemWindowsDirectory failed, cwcWindows=%u lasterr=%u\n", cwcWindows, GetLastError()));
        NonStandardAssertFailed();
        hr = E_FAIL;
    }

    return hr;

}

/* time intervals in milliseconds */
/* max time to wait for the service to startup */
#define VBOXDRVCFG_SVC_WAITSTART_TIME 10000
/* sleep time before service status polls */
#define VBOXDRVCFG_SVC_WAITSTART_TIME_PERIOD 100
/* number of service start polls */
#define VBOXDRVCFG_SVC_WAITSTART_RETRIES (VBOXDRVCFG_SVC_WAITSTART_TIME/VBOXDRVCFG_SVC_WAITSTART_TIME_PERIOD)

VBOXDRVCFG_DECL(HRESULT) VBoxDrvCfgSvcStart(LPCWSTR lpszSvcName)
{
    SC_HANDLE hMgr = OpenSCManager(NULL, NULL, SERVICE_QUERY_STATUS | SERVICE_START);
    if (hMgr == NULL)
    {
        DWORD dwErr = GetLastError();
        NonStandardLogRelCrap((__FUNCTION__": OpenSCManager failed, dwErr=%ld\n", dwErr));
        return HRESULT_FROM_WIN32(dwErr);
    }

    HRESULT hr = S_OK;
    SC_HANDLE hSvc = OpenServiceW(hMgr, lpszSvcName, SERVICE_QUERY_STATUS | SERVICE_START);
    if (hSvc)
    {
        do
        {
            SERVICE_STATUS Status;
            BOOL fRc = QueryServiceStatus(hSvc, &Status);
            if (!fRc)
            {
                DWORD dwErr = GetLastError();
                NonStandardLogRelCrap((__FUNCTION__": QueryServiceStatus failed dwErr=%ld\n", dwErr));
                hr = HRESULT_FROM_WIN32(dwErr);
                break;
            }

            if (Status.dwCurrentState != SERVICE_RUNNING && Status.dwCurrentState != SERVICE_START_PENDING)
            {
                NonStandardLogRelCrap(("Starting service (%S)\n", lpszSvcName));

                fRc = StartService(hSvc, 0, NULL);
                if (!fRc)
                {
                    DWORD dwErr = GetLastError();
                    NonStandardLogRelCrap((__FUNCTION__": StartService failed dwErr=%ld\n", dwErr));
                    hr = HRESULT_FROM_WIN32(dwErr);
                    break;
                }
            }

            fRc = QueryServiceStatus(hSvc, &Status);
            if (!fRc)
            {
                DWORD dwErr = GetLastError();
                NonStandardLogRelCrap((__FUNCTION__": QueryServiceStatus failed dwErr=%ld\n", dwErr));
                hr = HRESULT_FROM_WIN32(dwErr);
                break;
            }

            if (Status.dwCurrentState == SERVICE_START_PENDING)
            {
                for (int i = 0; i < VBOXDRVCFG_SVC_WAITSTART_RETRIES; ++i)
                {
                    Sleep(VBOXDRVCFG_SVC_WAITSTART_TIME_PERIOD);
                    fRc = QueryServiceStatus(hSvc, &Status);
                    if (!fRc)
                    {
                        DWORD dwErr = GetLastError();
                        NonStandardLogRelCrap((__FUNCTION__": QueryServiceStatus failed dwErr=%ld\n", dwErr));
                        hr = HRESULT_FROM_WIN32(dwErr);
                        break;
                    }
                    else if (Status.dwCurrentState != SERVICE_START_PENDING)
                        break;
                }
            }

            if (hr != S_OK || Status.dwCurrentState != SERVICE_RUNNING)
            {
                NonStandardLogRelCrap((__FUNCTION__": Failed to start the service\n"));
                hr = E_FAIL;
                break;
            }

        } while (0);

        CloseServiceHandle(hSvc);
    }
    else
    {
        DWORD dwErr = GetLastError();
        NonStandardLogRelCrap((__FUNCTION__": OpenServiceW failed, dwErr=%ld\n", dwErr));
        hr = HRESULT_FROM_WIN32(dwErr);
    }

    CloseServiceHandle(hMgr);

    return hr;
}


HRESULT VBoxDrvCfgDrvUpdate(LPCWSTR pcszwHwId, LPCWSTR pcsxwInf, BOOL *pbRebootRequired)
{
    if (pbRebootRequired)
        *pbRebootRequired = FALSE;
    BOOL bRebootRequired = FALSE;
    WCHAR InfFullPath[MAX_PATH];
    DWORD dwChars = GetFullPathNameW(pcsxwInf,
            sizeof (InfFullPath) / sizeof (InfFullPath[0]),
            InfFullPath,
            NULL /* LPTSTR *lpFilePart */
            );
    if (!dwChars || dwChars >= MAX_PATH)
    {
        NonStandardLogCrap(("GetFullPathNameW failed, dwErr=%ld, dwChars=%ld\n",
                            GetLastError(), dwChars));
        return E_INVALIDARG;
    }


    if (!UpdateDriverForPlugAndPlayDevicesW(NULL, /* HWND hwndParent */
            pcszwHwId,
            InfFullPath,
            INSTALLFLAG_FORCE,
            &bRebootRequired))
    {
        DWORD dwErr = GetLastError();
        NonStandardLogCrap(("UpdateDriverForPlugAndPlayDevicesW failed, dwErr=%ld\n",
                            dwErr));
        return HRESULT_FROM_WIN32(dwErr);
    }


    if (bRebootRequired)
        NonStandardLogCrap(("!!Driver Update: REBOOT REQUIRED!!\n", GetLastError(), dwChars));

    if (pbRebootRequired)
        *pbRebootRequired = bRebootRequired;

    return S_OK;
}
