/* $Id: vboxext.c $ */
/** @file
 * VBox extension to Wine D3D
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

#include "config.h"
#include "wine/port.h"
#include "wined3d_private.h"
#include "vboxext.h"
#ifdef VBOX_WITH_WDDM
#include <VBoxCrHgsmi.h>
#include <iprt/err.h>
#endif

WINE_DEFAULT_DEBUG_CHANNEL(d3d_vbox);

typedef DECLCALLBACK(void) FNVBOXEXTWORKERCB(void *pvUser);
typedef FNVBOXEXTWORKERCB *PFNVBOXEXTWORKERCB;

HRESULT VBoxExtDwSubmitProcSync(PFNVBOXEXTWORKERCB pfnCb, void *pvCb);
HRESULT VBoxExtDwSubmitProcAsync(PFNVBOXEXTWORKERCB pfnCb, void *pvCb);

/*******************************/
#ifdef VBOX_WITH_WDDM
# if defined(VBOX_WDDM_WOW64)
# define VBOXEXT_WINE_MODULE_NAME "wined3dwddm-x86.dll"
# else
# define VBOXEXT_WINE_MODULE_NAME "wined3dwddm.dll"
# endif
#else
/* both 32bit and 64bit versions of xpdm wine libs are named identically */
# define VBOXEXT_WINE_MODULE_NAME "wined3d.dll"
#endif

typedef struct VBOXEXT_WORKER
{
    CRITICAL_SECTION CritSect;

    HANDLE hEvent;

    HANDLE hThread;
    DWORD  idThread;
    /* wine does not seem to guarantie the dll is not unloaded in case FreeLibrary is used
     * while d3d object is not terminated, keep an extra reference to ensure we're not unloaded
     * while we are active */
    HMODULE hSelf;
} VBOXEXT_WORKER, *PVBOXEXT_WORKER;



HRESULT VBoxExtWorkerCreate(PVBOXEXT_WORKER pWorker);
HRESULT VBoxExtWorkerDestroy(PVBOXEXT_WORKER pWorker);
HRESULT VBoxExtWorkerSubmitProc(PVBOXEXT_WORKER pWorker, PFNVBOXEXTWORKERCB pfnCb, void *pvCb);


/*******************************/
typedef struct VBOXEXT_GLOBAL
{
    VBOXEXT_WORKER Worker;
} VBOXEXT_GLOBAL, *PVBOXEXT_GLOBAL;

static VBOXEXT_GLOBAL g_VBoxExtGlobal;

#define WM_VBOXEXT_CALLPROC  (WM_APP+1)
#define WM_VBOXEXT_INIT_QUIT (WM_APP+2)

typedef struct VBOXEXT_CALLPROC
{
    PFNVBOXEXTWORKERCB pfnCb;
    void *pvCb;
} VBOXEXT_CALLPROC, *PVBOXEXT_CALLPROC;

static DWORD WINAPI vboxExtWorkerThread(void *pvUser)
{
    PVBOXEXT_WORKER pWorker = (PVBOXEXT_WORKER)pvUser;
    MSG Msg;

    PeekMessage(&Msg,
        NULL /* HWND hWnd */,
        WM_USER /* UINT wMsgFilterMin */,
        WM_USER /* UINT wMsgFilterMax */,
        PM_NOREMOVE);
    SetEvent(pWorker->hEvent);

    do
    {
        BOOL bResult = GetMessage(&Msg,
            0 /*HWND hWnd*/,
            0 /*UINT wMsgFilterMin*/,
            0 /*UINT wMsgFilterMax*/
            );

        if(!bResult) /* WM_QUIT was posted */
            break;

        Assert(bResult != -1);
        if(bResult == -1) /* error occurred */
            break;

        switch (Msg.message)
        {
            case WM_VBOXEXT_CALLPROC:
            {
                VBOXEXT_CALLPROC* pData = (VBOXEXT_CALLPROC*)Msg.lParam;
                pData->pfnCb(pData->pvCb);
                SetEvent(pWorker->hEvent);
                break;
            }
            case WM_VBOXEXT_INIT_QUIT:
            case WM_CLOSE:
            {
                PostQuitMessage(0);
                break;
            }
            default:
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
        }
    } while (1);
    return 0;
}

HRESULT VBoxExtWorkerCreate(PVBOXEXT_WORKER pWorker)
{
    if(!GetModuleHandleEx(0, VBOXEXT_WINE_MODULE_NAME, &pWorker->hSelf))
    {
        DWORD dwEr = GetLastError();
        ERR("GetModuleHandleEx failed, %d", dwEr);
        return E_FAIL;
    }

    InitializeCriticalSection(&pWorker->CritSect);
    pWorker->hEvent = CreateEvent(NULL, /* LPSECURITY_ATTRIBUTES lpEventAttributes */
            FALSE, /* BOOL bManualReset */
            FALSE, /* BOOL bInitialState */
            NULL /* LPCTSTR lpName */
          );
    if (pWorker->hEvent)
    {
        pWorker->hThread = CreateThread(
                              NULL /* LPSECURITY_ATTRIBUTES lpThreadAttributes */,
                              0 /* SIZE_T dwStackSize */,
                              vboxExtWorkerThread,
                              pWorker,
                              0 /* DWORD dwCreationFlags */,
                              &pWorker->idThread);
        if (pWorker->hThread)
        {
            DWORD dwResult = WaitForSingleObject(pWorker->hEvent, INFINITE);
            if (WAIT_OBJECT_0 == dwResult)
                return S_OK;
            ERR("WaitForSingleObject returned %d\n", dwResult);
        }
        else
        {
            DWORD winErr = GetLastError();
            ERR("CreateThread failed, winErr = (%d)", winErr);
        }

        DeleteCriticalSection(&pWorker->CritSect);
    }
    else
    {
        DWORD winErr = GetLastError();
        ERR("CreateEvent failed, winErr = (%d)", winErr);
    }

    FreeLibrary(pWorker->hSelf);

    return E_FAIL;
}

HRESULT VBoxExtWorkerDestroy(PVBOXEXT_WORKER pWorker)
{
    BOOL bResult = PostThreadMessage(pWorker->idThread, WM_VBOXEXT_INIT_QUIT, 0, 0);
    DWORD dwErr;
    if (!bResult)
    {
        DWORD winErr = GetLastError();
        ERR("PostThreadMessage failed, winErr = (%d)", winErr);
        return E_FAIL;
    }

    dwErr = WaitForSingleObject(pWorker->hThread, INFINITE);
    if (dwErr != WAIT_OBJECT_0)
    {
        ERR("WaitForSingleObject returned (%d)", dwErr);
        return E_FAIL;
    }

    CloseHandle(pWorker->hEvent);
    DeleteCriticalSection(&pWorker->CritSect);

    FreeLibrary(pWorker->hSelf);

    CloseHandle(pWorker->hThread);

    return S_OK;
}

static HRESULT vboxExtWorkerSubmit(VBOXEXT_WORKER *pWorker, UINT Msg, LPARAM lParam, BOOL fSync)
{
    HRESULT hr = E_FAIL;
    BOOL bResult;
    /* need to serialize since vboxExtWorkerThread is using one pWorker->hEvent
     * to signal job completion */
    EnterCriticalSection(&pWorker->CritSect);
    bResult = PostThreadMessage(pWorker->idThread, Msg, 0, lParam);
    if (bResult)
    {
        if (fSync)
        {
            DWORD dwErr = WaitForSingleObject(pWorker->hEvent, INFINITE);
            if (dwErr == WAIT_OBJECT_0)
            {
                hr = S_OK;
            }
            else
            {
                ERR("WaitForSingleObject returned (%d)", dwErr);
            }
        }
        else
            hr = S_OK;
    }
    else
    {
        DWORD winErr = GetLastError();
        ERR("PostThreadMessage failed, winErr = (%d)", winErr);
        return E_FAIL;
    }
    LeaveCriticalSection(&pWorker->CritSect);
    return hr;
}

HRESULT VBoxExtWorkerSubmitProcSync(PVBOXEXT_WORKER pWorker, PFNVBOXEXTWORKERCB pfnCb, void *pvCb)
{
    VBOXEXT_CALLPROC Ctx;
    Ctx.pfnCb = pfnCb;
    Ctx.pvCb = pvCb;
    return vboxExtWorkerSubmit(pWorker, WM_VBOXEXT_CALLPROC, (LPARAM)&Ctx, TRUE);
}

static DECLCALLBACK(void) vboxExtWorkerSubmitProcAsyncWorker(void *pvUser)
{
    PVBOXEXT_CALLPROC pCallInfo = (PVBOXEXT_CALLPROC)pvUser;
    pCallInfo[1].pfnCb(pCallInfo[1].pvCb);
    HeapFree(GetProcessHeap(), 0, pCallInfo);
}

HRESULT VBoxExtWorkerSubmitProcAsync(PVBOXEXT_WORKER pWorker, PFNVBOXEXTWORKERCB pfnCb, void *pvCb)
{
    HRESULT hr;
    PVBOXEXT_CALLPROC pCallInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof (VBOXEXT_CALLPROC) * 2);
    if (!pCallInfo)
    {
        ERR("HeapAlloc failed\n");
        return E_OUTOFMEMORY;
    }
    pCallInfo[0].pfnCb = vboxExtWorkerSubmitProcAsyncWorker;
    pCallInfo[0].pvCb = pCallInfo;
    pCallInfo[1].pfnCb = pfnCb;
    pCallInfo[1].pvCb = pvCb;
    hr = vboxExtWorkerSubmit(pWorker, WM_VBOXEXT_CALLPROC, (LPARAM)pCallInfo, FALSE);
    if (FAILED(hr))
    {
        ERR("vboxExtWorkerSubmit failed, hr 0x%x\n", hr);
        HeapFree(GetProcessHeap(), 0, pCallInfo);
        return hr;
    }
    return S_OK;
}


static HRESULT vboxExtInit(void)
{
    HRESULT hr = S_OK;
#ifdef VBOX_WITH_WDDM
    int rc = VBoxCrHgsmiInit();
    if (!RT_SUCCESS(rc))
    {
        ERR("VBoxCrHgsmiInit failed rc %d", rc);
        return E_FAIL;
    }
#endif
    memset(&g_VBoxExtGlobal, 0, sizeof (g_VBoxExtGlobal));
    hr = VBoxExtWorkerCreate(&g_VBoxExtGlobal.Worker);
    if (SUCCEEDED(hr))
        return S_OK;

    /* failure branch */
#ifdef VBOX_WITH_WDDM
    VBoxCrHgsmiTerm();
#endif
    return hr;
}


static HRESULT vboxExtWndCleanup(void);

static HRESULT vboxExtTerm(void)
{
    HRESULT hr = vboxExtWndCleanup();
    if (!SUCCEEDED(hr))
    {
        ERR("vboxExtWndCleanup failed, hr %d", hr);
        return hr;
    }

    hr = VBoxExtWorkerDestroy(&g_VBoxExtGlobal.Worker);
    if (!SUCCEEDED(hr))
    {
        ERR("VBoxExtWorkerDestroy failed, hr %d", hr);
        return hr;
    }

#ifdef VBOX_WITH_WDDM
    VBoxCrHgsmiTerm();
#endif

    return S_OK;
}

/* wine serializes all calls to us, so no need for any synchronization here */
static DWORD g_cVBoxExtInits = 0;

static DWORD vboxExtAddRef(void)
{
    return ++g_cVBoxExtInits;
}

static DWORD vboxExtRelease(void)
{
    DWORD cVBoxExtInits = --g_cVBoxExtInits;
    Assert(cVBoxExtInits < UINT32_MAX/2);
    return cVBoxExtInits;
}

static DWORD vboxExtGetRef(void)
{
    return g_cVBoxExtInits;
}

HRESULT VBoxExtCheckInit(void)
{
    HRESULT hr = S_OK;
    if (!vboxExtGetRef())
    {
        hr = vboxExtInit();
        if (FAILED(hr))
        {
            ERR("vboxExtInit failed, hr (0x%x)", hr);
            return hr;
        }
    }
    vboxExtAddRef();
    return S_OK;
}

HRESULT VBoxExtCheckTerm(void)
{
    HRESULT hr = S_OK;
    if (vboxExtGetRef() == 1)
    {
        hr = vboxExtTerm();
        if (FAILED(hr))
        {
            ERR("vboxExtTerm failed, hr (0x%x)", hr);
            return hr;
        }
    }
    vboxExtRelease();
    return S_OK;
}

HRESULT VBoxExtDwSubmitProcSync(PFNVBOXEXTWORKERCB pfnCb, void *pvCb)
{
    return VBoxExtWorkerSubmitProcSync(&g_VBoxExtGlobal.Worker, pfnCb, pvCb);
}

HRESULT VBoxExtDwSubmitProcAsync(PFNVBOXEXTWORKERCB pfnCb, void *pvCb)
{
    return VBoxExtWorkerSubmitProcAsync(&g_VBoxExtGlobal.Worker, pfnCb, pvCb);
}

#if defined(VBOX_WINE_WITH_SINGLE_CONTEXT) || defined(VBOX_WINE_WITH_SINGLE_SWAPCHAIN_CONTEXT)
# ifndef VBOX_WITH_WDDM
typedef struct VBOXEXT_GETDC_CB
{
    HWND hWnd;
    HDC hDC;
} VBOXEXT_GETDC_CB, *PVBOXEXT_GETDC_CB;

static DECLCALLBACK(void) vboxExtGetDCWorker(void *pvUser)
{
    PVBOXEXT_GETDC_CB pData = (PVBOXEXT_GETDC_CB)pvUser;
    pData->hDC = GetDC(pData->hWnd);
}

typedef struct VBOXEXT_RELEASEDC_CB
{
    HWND hWnd;
    HDC hDC;
    int ret;
} VBOXEXT_RELEASEDC_CB, *PVBOXEXT_RELEASEDC_CB;

static DECLCALLBACK(void) vboxExtReleaseDCWorker(void *pvUser)
{
    PVBOXEXT_RELEASEDC_CB pData = (PVBOXEXT_RELEASEDC_CB)pvUser;
    pData->ret = ReleaseDC(pData->hWnd, pData->hDC);
}

HDC VBoxExtGetDC(HWND hWnd)
{
    HRESULT hr;
    VBOXEXT_GETDC_CB Data = {0};
    Data.hWnd = hWnd;
    Data.hDC = NULL;

    hr = VBoxExtDwSubmitProcSync(vboxExtGetDCWorker, &Data);
    if (FAILED(hr))
    {
        ERR("VBoxExtDwSubmitProcSync feiled, hr (0x%x)\n", hr);
        return NULL;
    }

    return Data.hDC;
}

int VBoxExtReleaseDC(HWND hWnd, HDC hDC)
{
    HRESULT hr;
    VBOXEXT_RELEASEDC_CB Data = {0};
    Data.hWnd = hWnd;
    Data.hDC = hDC;
    Data.ret = 0;

    hr = VBoxExtDwSubmitProcSync(vboxExtReleaseDCWorker, &Data);
    if (FAILED(hr))
    {
        ERR("VBoxExtDwSubmitProcSync feiled, hr (0x%x)\n", hr);
        return -1;
    }

    return Data.ret;
}
# endif /* #ifndef VBOX_WITH_WDDM */

static DECLCALLBACK(void) vboxExtReleaseContextWorker(void *pvUser)
{
    struct wined3d_context *context = (struct wined3d_context *)pvUser;
    VBoxTlsRefRelease(context);
}

void VBoxExtReleaseContextAsync(struct wined3d_context *context)
{
    HRESULT hr;

    hr = VBoxExtDwSubmitProcAsync(vboxExtReleaseContextWorker, context);
    if (FAILED(hr))
    {
        ERR("VBoxExtDwSubmitProcAsync feiled, hr (0x%x)\n", hr);
        return;
    }
}

#endif /* #if defined(VBOX_WINE_WITH_SINGLE_CONTEXT) || defined(VBOX_WINE_WITH_SINGLE_SWAPCHAIN_CONTEXT) */

/* window creation API */
static LRESULT CALLBACK vboxExtWndProc(HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch(uMsg)
    {
        case WM_CLOSE:
            TRACE("got WM_CLOSE for hwnd(0x%x)", hwnd);
            return 0;
        case WM_DESTROY:
            TRACE("got WM_DESTROY for hwnd(0x%x)", hwnd);
            return 0;
        case WM_NCHITTEST:
            TRACE("got WM_NCHITTEST for hwnd(0x%x)\n", hwnd);
            return HTNOWHERE;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

#define VBOXEXTWND_NAME "VboxDispD3DWineWnd"

static HRESULT vboxExtWndDoCleanup(void)
{
    HRESULT hr = S_OK;
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);
    WNDCLASS wc;
    if (GetClassInfo(hInstance, VBOXEXTWND_NAME, &wc))
    {
        if (!UnregisterClass(VBOXEXTWND_NAME, hInstance))
        {
            DWORD winEr = GetLastError();
            ERR("UnregisterClass failed, winErr(%d)\n", winEr);
            hr = E_FAIL;
        }
    }
    return hr;
}

static HRESULT vboxExtWndDoCreate(DWORD w, DWORD h, HWND *phWnd, HDC *phDC)
{
    HRESULT hr = S_OK;
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);
    /* Register the Window Class. */
    WNDCLASS wc;
    if (!GetClassInfo(hInstance, VBOXEXTWND_NAME, &wc))
    {
        wc.style = 0;//CS_OWNDC;
        wc.lpfnWndProc = vboxExtWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hInstance;
        wc.hIcon = NULL;
        wc.hCursor = NULL;
        wc.hbrBackground = NULL;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = VBOXEXTWND_NAME;
        if (!RegisterClass(&wc))
        {
            DWORD winErr = GetLastError();
            ERR("RegisterClass failed, winErr(%d)\n", winErr);
            hr = E_FAIL;
        }
    }

    if (hr == S_OK)
    {
        HWND hWnd = CreateWindowEx (WS_EX_TOOLWINDOW,
                                        VBOXEXTWND_NAME, VBOXEXTWND_NAME,
                                        WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_DISABLED,
                                        0, 0,
                                        w, h,
                                        NULL, //GetDesktopWindow() /* hWndParent */,
                                        NULL /* hMenu */,
                                        hInstance,
                                        NULL /* lpParam */);
        Assert(hWnd);
        if (hWnd)
        {
            *phWnd = hWnd;
            *phDC = GetDC(hWnd);
            /* make sure we keep inited until the window is active */
            vboxExtAddRef();
        }
        else
        {
            DWORD winErr = GetLastError();
            ERR("CreateWindowEx failed, winErr(%d)\n", winErr);
            hr = E_FAIL;
        }
    }

    return hr;
}

static HRESULT vboxExtWndDoDestroy(HWND hWnd, HDC hDC)
{
    BOOL bResult;
    DWORD winErr;
    ReleaseDC(hWnd, hDC);
    bResult = DestroyWindow(hWnd);
    Assert(bResult);
    if (bResult)
    {
        /* release the reference we previously acquired on window creation */
        vboxExtRelease();
        return S_OK;
    }

    winErr = GetLastError();
    ERR("DestroyWindow failed, winErr(%d) for hWnd(0x%x)\n", winErr, hWnd);

    return E_FAIL;
}

typedef struct VBOXEXTWND_CREATE_INFO
{
    int hr;
    HWND hWnd;
    HDC hDC;
    DWORD width;
    DWORD height;
} VBOXEXTWND_CREATE_INFO;

typedef struct VBOXEXTWND_DESTROY_INFO
{
    int hr;
    HWND hWnd;
    HDC hDC;
} VBOXEXTWND_DESTROY_INFO;

typedef struct VBOXEXTWND_CLEANUP_INFO
{
    int hr;
} VBOXEXTWND_CLEANUP_INFO;

static DECLCALLBACK(void) vboxExtWndDestroyWorker(void *pvUser)
{
    VBOXEXTWND_DESTROY_INFO *pInfo = (VBOXEXTWND_DESTROY_INFO*)pvUser;
    pInfo->hr = vboxExtWndDoDestroy(pInfo->hWnd, pInfo->hDC);
    Assert(pInfo->hr == S_OK);
}

static DECLCALLBACK(void) vboxExtWndCreateWorker(void *pvUser)
{
    VBOXEXTWND_CREATE_INFO *pInfo = (VBOXEXTWND_CREATE_INFO*)pvUser;
    pInfo->hr = vboxExtWndDoCreate(pInfo->width, pInfo->height, &pInfo->hWnd, &pInfo->hDC);
    Assert(pInfo->hr == S_OK);
}

static DECLCALLBACK(void) vboxExtWndCleanupWorker(void *pvUser)
{
    VBOXEXTWND_CLEANUP_INFO *pInfo = (VBOXEXTWND_CLEANUP_INFO*)pvUser;
    pInfo-> hr = vboxExtWndDoCleanup();
}

HRESULT VBoxExtWndDestroy(HWND hWnd, HDC hDC)
{
    HRESULT hr;
    VBOXEXTWND_DESTROY_INFO Info;
    Info.hr = E_FAIL;
    Info.hWnd = hWnd;
    Info.hDC = hDC;
    hr = VBoxExtDwSubmitProcSync(vboxExtWndDestroyWorker, &Info);
    if (!SUCCEEDED(hr))
    {
        ERR("VBoxExtDwSubmitProcSync-vboxExtWndDestroyWorker failed hr %d", hr);
        return hr;
    }

    if (!SUCCEEDED(Info.hr))
    {
        ERR("vboxExtWndDestroyWorker failed hr %d", Info.hr);
        return Info.hr;
    }

    return S_OK;
}

HRESULT VBoxExtWndCreate(DWORD width, DWORD height, HWND *phWnd, HDC *phDC)
{
    HRESULT hr;
    VBOXEXTWND_CREATE_INFO Info;
    Info.hr = E_FAIL;
    Info.width = width;
    Info.height = height;
    hr = VBoxExtDwSubmitProcSync(vboxExtWndCreateWorker, &Info);
    if (!SUCCEEDED(hr))
    {
        ERR("VBoxExtDwSubmitProcSync-vboxExtWndCreateWorker failed hr %d", hr);
        return hr;
    }

    Assert(Info.hr == S_OK);
    if (!SUCCEEDED(Info.hr))
    {
        ERR("vboxExtWndCreateWorker failed hr %d", Info.hr);
        return Info.hr;
    }

    *phWnd = Info.hWnd;
    *phDC = Info.hDC;
    return S_OK;
}

static HRESULT vboxExtWndCleanup(void)
{
    HRESULT hr;
    VBOXEXTWND_CLEANUP_INFO Info;
    Info.hr = E_FAIL;
    hr = VBoxExtDwSubmitProcSync(vboxExtWndCleanupWorker, &Info);
    if (!SUCCEEDED(hr))
    {
        ERR("VBoxExtDwSubmitProcSync-vboxExtWndCleanupWorker failed hr %d", hr);
        return hr;
    }

    if (!SUCCEEDED(Info.hr))
    {
        ERR("vboxExtWndCleanupWorker failed hr %d", Info.hr);
        return Info.hr;
    }

    return S_OK;
}


/* hash map impl */
static void vboxExtHashInitEntries(PVBOXEXT_HASHMAP pMap)
{
    uint32_t i;
    pMap->cEntries = 0;
    for (i = 0; i < RT_ELEMENTS(pMap->aBuckets); ++i)
    {
        RTListInit(&pMap->aBuckets[i].EntryList);
    }
}

void VBoxExtHashInit(PVBOXEXT_HASHMAP pMap, PFNVBOXEXT_HASHMAP_HASH pfnHash, PFNVBOXEXT_HASHMAP_EQUAL pfnEqual)
{
    pMap->pfnHash = pfnHash;
    pMap->pfnEqual = pfnEqual;
    vboxExtHashInitEntries(pMap);
}

DECLINLINE(uint32_t) vboxExtHashIdx(uint32_t u32Hash)
{
    return u32Hash % VBOXEXT_HASHMAP_NUM_BUCKETS;
}

#define VBOXEXT_FOREACH_NODE(_pNode, _pList, _op) do { \
        PRTLISTNODE _pNode; \
        PRTLISTNODE __pNext; \
        for (_pNode = (_pList)->pNext; \
                _pNode != (_pList); \
                _pNode = __pNext) \
        { \
            __pNext = _pNode->pNext; /* <- the _pNode should not be referenced after the _op */ \
            _op \
        } \
    } while (0)

DECLINLINE(PVBOXEXT_HASHMAP_ENTRY) vboxExtHashSearchEntry(PVBOXEXT_HASHMAP pMap, void *pvKey)
{
    uint32_t u32Hash = pMap->pfnHash(pvKey);
    uint32_t u32HashIdx = vboxExtHashIdx(u32Hash);
    PVBOXEXT_HASHMAP_BUCKET pBucket = &pMap->aBuckets[u32HashIdx];
    PVBOXEXT_HASHMAP_ENTRY pEntry;
    VBOXEXT_FOREACH_NODE(pNode, &pBucket->EntryList,
        pEntry = RT_FROM_MEMBER(pNode, VBOXEXT_HASHMAP_ENTRY, ListNode);
        if (pEntry->u32Hash != u32Hash)
            continue;

        if (!pMap->pfnEqual(pvKey, pEntry->pvKey))
            continue;
        return pEntry;
    );
    return NULL;
}

void* VBoxExtHashRemoveEntry(PVBOXEXT_HASHMAP pMap, PVBOXEXT_HASHMAP_ENTRY pEntry)
{
    RTListNodeRemove(&pEntry->ListNode);
    --pMap->cEntries;
    Assert(pMap->cEntries <= UINT32_MAX/2);
    return pEntry->pvKey;
}

static void vboxExtHashPutEntry(PVBOXEXT_HASHMAP pMap, PVBOXEXT_HASHMAP_BUCKET pBucket, PVBOXEXT_HASHMAP_ENTRY pEntry)
{
    RTListNodeInsertAfter(&pBucket->EntryList, &pEntry->ListNode);
    ++pMap->cEntries;
}

PVBOXEXT_HASHMAP_ENTRY VBoxExtHashRemove(PVBOXEXT_HASHMAP pMap, void *pvKey)
{
    PVBOXEXT_HASHMAP_ENTRY pEntry = vboxExtHashSearchEntry(pMap, pvKey);
    if (!pEntry)
        return NULL;

    VBoxExtHashRemoveEntry(pMap, pEntry);
    return pEntry;
}

PVBOXEXT_HASHMAP_ENTRY VBoxExtHashPut(PVBOXEXT_HASHMAP pMap, void *pvKey, PVBOXEXT_HASHMAP_ENTRY pEntry)
{
    PVBOXEXT_HASHMAP_ENTRY pOldEntry = VBoxExtHashRemove(pMap, pvKey);
    uint32_t u32Hash = pMap->pfnHash(pvKey);
    uint32_t u32HashIdx = vboxExtHashIdx(u32Hash);
    pEntry->pvKey = pvKey;
    pEntry->u32Hash = u32Hash;
    vboxExtHashPutEntry(pMap, &pMap->aBuckets[u32HashIdx], pEntry);
    return pOldEntry;
}


PVBOXEXT_HASHMAP_ENTRY VBoxExtHashGet(PVBOXEXT_HASHMAP pMap, void *pvKey)
{
    return vboxExtHashSearchEntry(pMap, pvKey);
}

void VBoxExtHashVisit(PVBOXEXT_HASHMAP pMap, PFNVBOXEXT_HASHMAP_VISITOR pfnVisitor, void *pvVisitor)
{
    uint32_t iBucket = 0, iEntry = 0;
    uint32_t cEntries = pMap->cEntries;

    if (!cEntries)
        return;

    for (; ; ++iBucket)
    {
        PVBOXEXT_HASHMAP_ENTRY pEntry;
        PVBOXEXT_HASHMAP_BUCKET pBucket = &pMap->aBuckets[iBucket];
        Assert(iBucket < RT_ELEMENTS(pMap->aBuckets));
        VBOXEXT_FOREACH_NODE(pNode, &pBucket->EntryList,
            pEntry = RT_FROM_MEMBER(pNode, VBOXEXT_HASHMAP_ENTRY, ListNode);
            if (!pfnVisitor(pMap, pEntry->pvKey, pEntry, pvVisitor))
                return;

            if (++iEntry == cEntries)
                return;
        );
    }

    /* should not be here! */
    AssertFailed();
}

void VBoxExtHashCleanup(PVBOXEXT_HASHMAP pMap, PFNVBOXEXT_HASHMAP_VISITOR pfnVisitor, void *pvVisitor)
{
    VBoxExtHashVisit(pMap, pfnVisitor, pvVisitor);
    vboxExtHashInitEntries(pMap);
}

static DECLCALLBACK(bool) vboxExtCacheCleanupCb(struct VBOXEXT_HASHMAP *pMap, void *pvKey, struct VBOXEXT_HASHMAP_ENTRY *pValue, void *pvVisitor)
{
    PVBOXEXT_HASHCACHE pCache = VBOXEXT_HASHCACHE_FROM_MAP(pMap);
    PVBOXEXT_HASHCACHE_ENTRY pCacheEntry = VBOXEXT_HASHCACHE_ENTRY_FROM_MAP(pValue);
    pCache->pfnCleanupEntry(pvKey, pCacheEntry);
    return TRUE;
}

void VBoxExtCacheCleanup(PVBOXEXT_HASHCACHE pCache)
{
    VBoxExtHashCleanup(&pCache->Map, vboxExtCacheCleanupCb, NULL);
}

#if defined(VBOXWINEDBG_SHADERS) || defined(VBOX_WINE_WITH_PROFILE)
void vboxWDbgPrintF(char * szString, ...)
{
    char szBuffer[4096*2] = {0};
    va_list pArgList;
    va_start(pArgList, szString);
    _vsnprintf(szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]), szString, pArgList);
    va_end(pArgList);

    OutputDebugStringA(szBuffer);
}
#endif
