/** @file
 * Shared Clipboard: Win32 host.
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

#include <iprt/win/windows.h>

#include <VBox/HostServices/VBoxClipboardSvc.h>

#include <iprt/alloc.h>
#include <iprt/string.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/thread.h>
#include <iprt/ldr.h>
#include <process.h>

#include "VBoxClipboard.h"

#define dprintf Log

static char gachWindowClassName[] = "VBoxSharedClipboardClass";

enum { CBCHAIN_TIMEOUT = 5000 /* ms */ };

/* Dynamically load clipboard functions from User32.dll. */
typedef BOOL WINAPI FNADDCLIPBOARDFORMATLISTENER(HWND);
typedef FNADDCLIPBOARDFORMATLISTENER *PFNADDCLIPBOARDFORMATLISTENER;

typedef BOOL WINAPI FNREMOVECLIPBOARDFORMATLISTENER(HWND);
typedef FNREMOVECLIPBOARDFORMATLISTENER *PFNREMOVECLIPBOARDFORMATLISTENER;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int ConvertCFHtmlToMime(const char *pszSource, const uint32_t cch, char **ppszOutput, uint32_t *pch);
static int ConvertMimeToCFHTML(const char *pszSource, size_t cb, char **ppszOutput, uint32_t *pcbOutput);
static bool IsWindowsHTML(const char *source);


#ifndef WM_CLIPBOARDUPDATE
#define WM_CLIPBOARDUPDATE 0x031D
#endif

struct _VBOXCLIPBOARDCONTEXT
{
    HWND    hwnd;
    HWND    hwndNextInChain;

    UINT     timerRefresh;

    bool     fCBChainPingInProcess;

    RTTHREAD thread;

    HANDLE hRenderEvent;

    VBOXCLIPBOARDCLIENTDATA *pClient;

    PFNADDCLIPBOARDFORMATLISTENER    pfnAddClipboardFormatListener;
    PFNREMOVECLIPBOARDFORMATLISTENER pfnRemoveClipboardFormatListener;

};

/* Only one client is supported. There seems to be no need for more clients. */
static VBOXCLIPBOARDCONTEXT g_ctx;


#ifdef LOG_ENABLED
void vboxClipboardDump(const void *pv, size_t cb, uint32_t u32Format)
{
    if (u32Format & VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT)
    {
        Log(("DUMP: VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT:\n"));
        if (pv && cb)
            Log(("%ls\n", pv));
        else
            Log(("%p %d\n", pv, cb));
    }
    else if (u32Format & VBOX_SHARED_CLIPBOARD_FMT_BITMAP)
        dprintf(("DUMP: VBOX_SHARED_CLIPBOARD_FMT_BITMAP\n"));
    else if (u32Format & VBOX_SHARED_CLIPBOARD_FMT_HTML)
    {
        Log(("DUMP: VBOX_SHARED_CLIPBOARD_FMT_HTML:\n"));
        if (pv && cb)
        {
            Log(("%s\n", pv));

            //size_t cb = RTStrNLen(pv, );
            char *pszBuf = (char *)RTMemAllocZ(cb + 1);
            RTStrCopy(pszBuf, cb + 1, (const char *)pv);
            for (size_t off = 0; off < cb; ++off)
            {
                if (pszBuf[off] == '\n' || pszBuf[off] == '\r')
                    pszBuf[off] = ' ';
            }

            Log(("%s\n", pszBuf));
            RTMemFree(pszBuf);
        }
        else
            Log(("%p %d\n", pv, cb));
    }
    else
        dprintf(("DUMP: invalid format %02X\n", u32Format));
}
#else  /* !LOG_ENABLED */
# define vboxClipboardDump(__pv, __cb, __format) do { NOREF(__pv); NOREF(__cb); NOREF(__format); } while (0)
#endif /* !LOG_ENABLED */


static void vboxClipboardInitNewAPI(VBOXCLIPBOARDCONTEXT *pCtx)
{
    RTLDRMOD hUser32 = NIL_RTLDRMOD;
    int rc = RTLdrLoadSystem("User32.dll", /* fNoUnload = */ true, &hUser32);
    if (RT_SUCCESS(rc))
    {
        rc = RTLdrGetSymbol(hUser32, "AddClipboardFormatListener", (void**)&pCtx->pfnAddClipboardFormatListener);
        if (RT_SUCCESS(rc))
        {
            rc = RTLdrGetSymbol(hUser32, "RemoveClipboardFormatListener", (void**)&pCtx->pfnRemoveClipboardFormatListener);
        }

        RTLdrClose(hUser32);
    }

    if (RT_SUCCESS(rc))
    {
        Log(("New Clipboard API is enabled\n"));
    }
    else
    {
        pCtx->pfnAddClipboardFormatListener = NULL;
        pCtx->pfnRemoveClipboardFormatListener = NULL;
        Log(("New Clipboard API is not available. rc = %Rrc\n", rc));
    }
}

static bool vboxClipboardIsNewAPI(VBOXCLIPBOARDCONTEXT *pCtx)
{
    return pCtx->pfnAddClipboardFormatListener != NULL;
}


static int vboxOpenClipboard(HWND hwnd)
{
    /* "OpenClipboard fails if another window has the clipboard open."
     * So try a few times and wait up to 1 second.
     */
    BOOL fOpened = FALSE;

    int i = 0;
    for (;;)
    {
        if (OpenClipboard(hwnd))
        {
            fOpened = TRUE;
            break;
        }

        if (i >= 10) /* sleep interval = [1..512] ms */
            break;

        RTThreadSleep(1 << i);
        ++i;
    }

#ifdef LOG_ENABLED
    if (i > 0)
        LogFlowFunc(("%d times tried to open clipboard.\n", i + 1));
#endif

    int rc;
    if (fOpened)
        rc = VINF_SUCCESS;
    else
    {
        const DWORD err = GetLastError();
        LogFlowFunc(("error %d\n", err));
        rc = RTErrConvertFromWin32(err);
    }

    return rc;
}


/** @todo Someone please explain the protocol wrt overflows...  */
static void vboxClipboardGetData (uint32_t u32Format, const void *pvSrc, uint32_t cbSrc,
                                  void *pvDst, uint32_t cbDst, uint32_t *pcbActualDst)
{
    dprintf (("vboxClipboardGetData.\n"));

    LogFlow(("vboxClipboardGetData cbSrc = %d, cbDst = %d\n", cbSrc, cbDst));

    if (   u32Format == VBOX_SHARED_CLIPBOARD_FMT_HTML
        && IsWindowsHTML((const char *)pvSrc))
    {
        /** @todo r=bird: Why the double conversion? */
        char *pszBuf = NULL;
        uint32_t cbBuf = 0;
        int rc = ConvertCFHtmlToMime((const char*)pvSrc, cbSrc, &pszBuf, &cbBuf);
        if (RT_SUCCESS(rc))
        {
            *pcbActualDst = cbBuf;
            if (cbBuf > cbDst)
            {
                /* Do not copy data. The dst buffer is not enough. */
                RTMemFree(pszBuf);
                return;
            }
            memcpy(pvDst, pszBuf, cbBuf);
            RTMemFree(pszBuf);
        }
        else
            *pcbActualDst = 0;
    }
    else
    {
        *pcbActualDst = cbSrc;

        if (cbSrc > cbDst)
        {
            /* Do not copy data. The dst buffer is not enough. */
            return;
        }

        memcpy(pvDst, pvSrc, cbSrc);
    }

    vboxClipboardDump(pvDst, cbSrc, u32Format);

    return;
}

static int vboxClipboardReadDataFromClient (VBOXCLIPBOARDCONTEXT *pCtx, uint32_t u32Format)
{
    Assert(pCtx->pClient);
    Assert(pCtx->hRenderEvent);
    Assert(pCtx->pClient->data.pv == NULL && pCtx->pClient->data.cb == 0 && pCtx->pClient->data.u32Format == 0);

    LogFlow(("vboxClipboardReadDataFromClient u32Format = %02X\n", u32Format));

    ResetEvent (pCtx->hRenderEvent);

    vboxSvcClipboardReportMsg (pCtx->pClient, VBOX_SHARED_CLIPBOARD_HOST_MSG_READ_DATA, u32Format);

    DWORD ret = WaitForSingleObject(pCtx->hRenderEvent, INFINITE);
    LogFlow(("vboxClipboardReadDataFromClient wait completed, ret 0x%08X, err %d\n",
             ret, GetLastError())); NOREF(ret);

    return VINF_SUCCESS;
}

static void vboxClipboardChanged (VBOXCLIPBOARDCONTEXT *pCtx)
{
    LogFlow(("vboxClipboardChanged\n"));

    if (pCtx->pClient == NULL)
    {
        return;
    }

    /* Query list of available formats and report to host. */
    int rc = vboxOpenClipboard(pCtx->hwnd);
    if (RT_SUCCESS(rc))
    {
        uint32_t u32Formats = 0;

        UINT format = 0;

        while ((format = EnumClipboardFormats (format)) != 0)
        {
            LogFlow(("vboxClipboardChanged format %#x\n", format));
            switch (format)
            {
                case CF_UNICODETEXT:
                case CF_TEXT:
                    u32Formats |= VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT;
                    break;

                case CF_DIB:
                case CF_BITMAP:
                    u32Formats |= VBOX_SHARED_CLIPBOARD_FMT_BITMAP;
                    break;

                default:
                    if (format >= 0xC000)
                    {
                        TCHAR szFormatName[256];

                        int cActual = GetClipboardFormatName(format, szFormatName, sizeof(szFormatName)/sizeof (TCHAR));

                        if (cActual)
                        {
                            if (strcmp (szFormatName, "HTML Format") == 0)
                            {
                                u32Formats |= VBOX_SHARED_CLIPBOARD_FMT_HTML;
                            }
                        }
                    }
                    break;
            }
        }

        CloseClipboard ();

        LogFlow(("vboxClipboardChanged u32Formats %02X\n", u32Formats));

        vboxSvcClipboardReportMsg (pCtx->pClient, VBOX_SHARED_CLIPBOARD_HOST_MSG_FORMATS, u32Formats);
    }
    else
    {
        LogFlow(("vboxClipboardChanged: error in open clipboard. hwnd: %x. err: %Rrc\n", pCtx->hwnd, rc));
    }
}

/* Add ourselves into the chain of cliboard listeners */
static void addToCBChain (VBOXCLIPBOARDCONTEXT *pCtx)
{
    if (vboxClipboardIsNewAPI(pCtx))
        pCtx->pfnAddClipboardFormatListener(pCtx->hwnd);
    else
        pCtx->hwndNextInChain = SetClipboardViewer(pCtx->hwnd);
}

/* Remove ourselves from the chain of cliboard listeners */
static void removeFromCBChain (VBOXCLIPBOARDCONTEXT *pCtx)
{
    if (vboxClipboardIsNewAPI(pCtx))
    {
        pCtx->pfnRemoveClipboardFormatListener(pCtx->hwnd);
    }
    else
    {
        ChangeClipboardChain(pCtx->hwnd, pCtx->hwndNextInChain);
        pCtx->hwndNextInChain = NULL;
    }
}

/* Callback which is invoked when we have successfully pinged ourselves down the
 * clipboard chain.  We simply unset a boolean flag to say that we are responding.
 * There is a race if a ping returns after the next one is initiated, but nothing
 * very bad is likely to happen. */
VOID CALLBACK CBChainPingProc(HWND hwnd, UINT uMsg, ULONG_PTR dwData, LRESULT lResult)
{
    (void) hwnd;
    (void) uMsg;
    (void) lResult;
    VBOXCLIPBOARDCONTEXT *pCtx = (VBOXCLIPBOARDCONTEXT *)dwData;
    pCtx->fCBChainPingInProcess = FALSE;
}

static LRESULT CALLBACK vboxClipboardWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT rc = 0;

    VBOXCLIPBOARDCONTEXT *pCtx = &g_ctx;

    switch (msg)
    {
        case WM_CLIPBOARDUPDATE:
        {
            Log(("WM_CLIPBOARDUPDATE\n"));

            if (GetClipboardOwner() != hwnd)
            {
                /* Clipboard was updated by another application. */
                vboxClipboardChanged(pCtx);
            }
        } break;

        case WM_CHANGECBCHAIN:
        {
            Log(("WM_CHANGECBCHAIN\n"));

            if (vboxClipboardIsNewAPI(pCtx))
            {
                rc = DefWindowProc(hwnd, msg, wParam, lParam);
                break;
            }

            HWND hwndRemoved = (HWND)wParam;
            HWND hwndNext    = (HWND)lParam;

            if (hwndRemoved == pCtx->hwndNextInChain)
            {
                /* The window that was next to our in the chain is being removed.
                 * Relink to the new next window.
                 */
                pCtx->hwndNextInChain = hwndNext;
            }
            else
            {
                if (pCtx->hwndNextInChain)
                {
                    /* Pass the message further. */
                    DWORD_PTR dwResult;
                    rc = SendMessageTimeout(pCtx->hwndNextInChain, WM_CHANGECBCHAIN, wParam, lParam, 0, CBCHAIN_TIMEOUT, &dwResult);
                    if (!rc)
                        rc = (LRESULT)dwResult;
                }
            }
        } break;

        case WM_DRAWCLIPBOARD:
        {
            Log(("WM_DRAWCLIPBOARD\n"));

            if (GetClipboardOwner () != hwnd)
            {
                /* Clipboard was updated by another application. */
                vboxClipboardChanged (pCtx);
            }

            if (pCtx->hwndNextInChain)
            {
                Log(("WM_DRAWCLIPBOARD next %p\n", pCtx->hwndNextInChain));
                /* Pass the message to next windows in the clipboard chain. */
                DWORD_PTR dwResult;
                rc = SendMessageTimeout(pCtx->hwndNextInChain, msg, wParam, lParam, 0, CBCHAIN_TIMEOUT, &dwResult);
                if (!rc)
                    rc = dwResult;
            }
        } break;

        case WM_TIMER:
        {
            if (vboxClipboardIsNewAPI(pCtx))
                break;

            HWND hViewer = GetClipboardViewer();

            /* Re-register ourselves in the clipboard chain if our last ping
             * timed out or there seems to be no valid chain. */
            if (!hViewer || pCtx->fCBChainPingInProcess)
            {
                removeFromCBChain(pCtx);
                addToCBChain(pCtx);
            }
            /* Start a new ping by passing a dummy WM_CHANGECBCHAIN to be
             * processed by ourselves to the chain. */
            pCtx->fCBChainPingInProcess = TRUE;
            hViewer = GetClipboardViewer();
            if (hViewer)
                SendMessageCallback(hViewer, WM_CHANGECBCHAIN, (WPARAM)pCtx->hwndNextInChain, (LPARAM)pCtx->hwndNextInChain, CBChainPingProc, (ULONG_PTR) pCtx);
        } break;

        case WM_RENDERFORMAT:
        {
            /* Insert the requested clipboard format data into the clipboard. */
            uint32_t u32Format = 0;

            UINT format = (UINT)wParam;

            Log(("WM_RENDERFORMAT %d\n", format));

            switch (format)
            {
                case CF_UNICODETEXT:
                    u32Format |= VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT;
                    break;

                case CF_DIB:
                    u32Format |= VBOX_SHARED_CLIPBOARD_FMT_BITMAP;
                    break;

                default:
                    if (format >= 0xC000)
                    {
                        TCHAR szFormatName[256];

                        int cActual = GetClipboardFormatName(format, szFormatName, sizeof(szFormatName)/sizeof (TCHAR));

                        if (cActual)
                        {
                            if (strcmp (szFormatName, "HTML Format") == 0)
                            {
                                u32Format |= VBOX_SHARED_CLIPBOARD_FMT_HTML;
                            }
                        }
                    }
                    break;
            }

            if (u32Format == 0 || pCtx->pClient == NULL)
            {
                /* Unsupported clipboard format is requested. */
                Log(("WM_RENDERFORMAT unsupported format requested or client is not active.\n"));
                EmptyClipboard ();
            }
            else
            {
                int vboxrc = vboxClipboardReadDataFromClient (pCtx, u32Format);

                dprintf(("vboxClipboardReadDataFromClient vboxrc = %d, pv %p, cb %d, u32Format %d\n",
                          vboxrc, pCtx->pClient->data.pv, pCtx->pClient->data.cb, pCtx->pClient->data.u32Format));

                if (   RT_SUCCESS (vboxrc)
                    && pCtx->pClient->data.pv != NULL
                    && pCtx->pClient->data.cb > 0
                    && pCtx->pClient->data.u32Format == u32Format)
                {
                    HANDLE hMem = GlobalAlloc (GMEM_DDESHARE | GMEM_MOVEABLE, pCtx->pClient->data.cb);

                    dprintf(("hMem %p\n", hMem));

                    if (hMem)
                    {
                        void *pMem = GlobalLock (hMem);

                        dprintf(("pMem %p, GlobalSize %d\n", pMem, GlobalSize (hMem)));

                        if (pMem)
                        {
                            Log(("WM_RENDERFORMAT setting data\n"));

                            if (pCtx->pClient->data.pv)
                            {
                                memcpy (pMem, pCtx->pClient->data.pv, pCtx->pClient->data.cb);

                                RTMemFree (pCtx->pClient->data.pv);
                                pCtx->pClient->data.pv        = NULL;
                            }

                            pCtx->pClient->data.cb        = 0;
                            pCtx->pClient->data.u32Format = 0;

                            /* The memory must be unlocked before inserting to the Clipboard. */
                            GlobalUnlock (hMem);

                            /* 'hMem' contains the host clipboard data.
                             * size is 'cb' and format is 'format'.
                             */
                            HANDLE hClip = SetClipboardData (format, hMem);

                            dprintf(("vboxClipboardHostEvent hClip %p\n", hClip));

                            if (hClip)
                            {
                                /* The hMem ownership has gone to the system. Nothing to do. */
                                break;
                            }
                        }

                        GlobalFree (hMem);
                    }
                }

                RTMemFree (pCtx->pClient->data.pv);
                pCtx->pClient->data.pv        = NULL;
                pCtx->pClient->data.cb        = 0;
                pCtx->pClient->data.u32Format = 0;

                /* Something went wrong. */
                EmptyClipboard ();
            }
        } break;

        case WM_RENDERALLFORMATS:
        {
            Log(("WM_RENDERALLFORMATS\n"));

            /* Do nothing. The clipboard formats will be unavailable now, because the
             * windows is to be destroyed and therefore the guest side becomes inactive.
             */
            int vboxrc = vboxOpenClipboard(hwnd);
            if (RT_SUCCESS(vboxrc))
            {
                EmptyClipboard();

                CloseClipboard();
            }
            else
            {
                LogFlow(("vboxClipboardWndProc: WM_RENDERALLFORMATS: error in open clipboard. hwnd: %x, rc: %Rrc\n", hwnd, vboxrc));
            }
        } break;

        case WM_USER:
        {
            if (pCtx->pClient == NULL || pCtx->pClient->fMsgFormats)
            {
                /* Host has pending formats message. Ignore the guest announcement,
                 * because host clipboard has more priority.
                 */
                Log(("WM_USER ignored\n"));
                break;
            }

            /* Announce available formats. Do not insert data, they will be inserted in WM_RENDER*. */
            uint32_t u32Formats = (uint32_t)lParam;

            Log(("WM_USER u32Formats = %02X\n", u32Formats));

            int vboxrc = vboxOpenClipboard(hwnd);
            if (RT_SUCCESS(vboxrc))
            {
                EmptyClipboard();

                Log(("WM_USER emptied clipboard\n"));

                HANDLE hClip = NULL;

                if (u32Formats & VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT)
                {
                    dprintf(("window proc WM_USER: VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT\n"));

                    hClip = SetClipboardData (CF_UNICODETEXT, NULL);
                }

                if (u32Formats & VBOX_SHARED_CLIPBOARD_FMT_BITMAP)
                {
                    dprintf(("window proc WM_USER: VBOX_SHARED_CLIPBOARD_FMT_BITMAP\n"));

                    hClip = SetClipboardData (CF_DIB, NULL);
                }

                if (u32Formats & VBOX_SHARED_CLIPBOARD_FMT_HTML)
                {
                    UINT format = RegisterClipboardFormat ("HTML Format");
                    dprintf(("window proc WM_USER: VBOX_SHARED_CLIPBOARD_FMT_HTML 0x%04X\n", format));
                    if (format != 0)
                    {
                        hClip = SetClipboardData (format, NULL);
                    }
                }

                CloseClipboard();

                dprintf(("window proc WM_USER: hClip %p, err %d\n", hClip, GetLastError ()));
            }
            else
            {
                dprintf(("window proc WM_USER: failed to open clipboard. rc: %Rrc\n", vboxrc));
            }
        } break;

        case WM_DESTROY:
        {
            /* MS recommends to remove from Clipboard chain in this callback */
            Assert(pCtx->hwnd);
            removeFromCBChain(pCtx);
            if (pCtx->timerRefresh)
                KillTimer(pCtx->hwnd, 0);
            PostQuitMessage(0);
        } break;

        default:
        {
            Log(("WM_ %p\n", msg));
            rc = DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }

    Log(("WM_ rc %d\n", rc));
    return rc;
}

DECLCALLBACK(int) VBoxClipboardThread (RTTHREAD hThreadSelf, void *pvUser)
{
    RT_NOREF2(hThreadSelf, pvUser);
    /* Create a window and make it a clipboard viewer. */
    int rc = VINF_SUCCESS;

    LogFlow(("VBoxClipboardThread\n"));

    VBOXCLIPBOARDCONTEXT *pCtx = &g_ctx;

    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);

    /* Register the Window Class. */
    WNDCLASS wc;
    RT_ZERO(wc);

    wc.style         = CS_NOCLOSE;
    wc.lpfnWndProc   = vboxClipboardWndProc;
    wc.hInstance     = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
    wc.lpszClassName = gachWindowClassName;

    ATOM atomWindowClass = RegisterClass(&wc);

    if (atomWindowClass == 0)
    {
        Log(("Failed to register window class\n"));
        rc = VERR_NOT_SUPPORTED;
    }
    else
    {
        /* Create the window. */
        pCtx->hwnd = CreateWindowEx (WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
                                     gachWindowClassName, gachWindowClassName,
                                     WS_POPUPWINDOW,
                                     -200, -200, 100, 100, NULL, NULL, hInstance, NULL);

        if (pCtx->hwnd == NULL)
        {
            Log(("Failed to create window\n"));
            rc = VERR_NOT_SUPPORTED;
        }
        else
        {
            SetWindowPos(pCtx->hwnd, HWND_TOPMOST, -200, -200, 0, 0,
                         SWP_NOACTIVATE | SWP_HIDEWINDOW | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOSIZE);

            addToCBChain(pCtx);
            if (!vboxClipboardIsNewAPI(pCtx))
                pCtx->timerRefresh = SetTimer(pCtx->hwnd, 0, 10 * 1000, NULL);

            MSG msg;
            BOOL msgret = 0;
            while ((msgret = GetMessage(&msg, NULL, 0, 0)) > 0)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            /*
            * Window procedure can return error,
            * but this is exceptional situation
            * that should be identified in testing
            */
            Assert(msgret >= 0);
            Log(("VBoxClipboardThread Message loop finished. GetMessage returned %d, message id: %d \n", msgret, msg.message));
        }
    }

    pCtx->hwnd = NULL;

    if (atomWindowClass != 0)
    {
        UnregisterClass (gachWindowClassName, hInstance);
        atomWindowClass = 0;
    }

    return 0;
}

/*
 * Public platform dependent functions.
 */
int vboxClipboardInit (void)
{
    int rc = VINF_SUCCESS;

    RT_ZERO(g_ctx);

    /* Check that new Clipboard API is available */
    vboxClipboardInitNewAPI(&g_ctx);

    g_ctx.hRenderEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    rc = RTThreadCreate (&g_ctx.thread, VBoxClipboardThread, NULL, 65536,
                         RTTHREADTYPE_IO, RTTHREADFLAGS_WAITABLE, "SHCLIP");

    if (RT_FAILURE (rc))
    {
        CloseHandle (g_ctx.hRenderEvent);
    }

    return rc;
}

void vboxClipboardDestroy (void)
{
    Log(("vboxClipboardDestroy\n"));

    if (g_ctx.hwnd)
    {
        PostMessage (g_ctx.hwnd, WM_CLOSE, 0, 0);
    }

    CloseHandle (g_ctx.hRenderEvent);

    /* Wait for the window thread to terminate. */
    RTThreadWait (g_ctx.thread, RT_INDEFINITE_WAIT, NULL);

    g_ctx.thread = NIL_RTTHREAD;
}

int vboxClipboardConnect (VBOXCLIPBOARDCLIENTDATA *pClient, bool fHeadless)
{
    NOREF(fHeadless);
    Log(("vboxClipboardConnect\n"));

    if (g_ctx.pClient != NULL)
    {
        /* One client only. */
        return VERR_NOT_SUPPORTED;
    }

    pClient->pCtx = &g_ctx;

    pClient->pCtx->pClient = pClient;

    /* Sync the host clipboard content with the client. */
    vboxClipboardSync (pClient);

    return VINF_SUCCESS;
}

int vboxClipboardSync (VBOXCLIPBOARDCLIENTDATA *pClient)
{
    /* Sync the host clipboard content with the client. */
    vboxClipboardChanged (pClient->pCtx);

    return VINF_SUCCESS;
}

void vboxClipboardDisconnect (VBOXCLIPBOARDCLIENTDATA *pClient)
{
    RT_NOREF1(pClient);
    Log(("vboxClipboardDisconnect\n"));

    g_ctx.pClient = NULL;
}

void vboxClipboardFormatAnnounce (VBOXCLIPBOARDCLIENTDATA *pClient, uint32_t u32Formats)
{
    /*
     * The guest announces formats. Forward to the window thread.
     */
    PostMessage (pClient->pCtx->hwnd, WM_USER, 0, u32Formats);
}

int DumpHtml(const char *pszSrc, size_t cb)
{
    size_t cchIgnored = 0;
    int rc = RTStrNLenEx(pszSrc, cb, &cchIgnored);
    if (RT_SUCCESS(rc))
    {
        char *pszBuf = (char *)RTMemAllocZ(cb + 1);
        if (pszBuf != NULL)
        {
            rc = RTStrCopy(pszBuf, cb + 1, (const char *)pszSrc);
            if (RT_SUCCESS(rc))
            {
                for (size_t i = 0; i < cb; ++i)
                    if (pszBuf[i] == '\n' || pszBuf[i] == '\r')
                        pszBuf[i] = ' ';
            }
            else
                Log(("Error in copying string.\n"));
            Log(("Removed \\r\\n: %s\n", pszBuf));
            RTMemFree(pszBuf);
        }
        else
        {
            rc = VERR_NO_MEMORY;
            Log(("Not enough memory to allocate buffer.\n"));
        }
    }
    return rc;
}

int vboxClipboardReadData (VBOXCLIPBOARDCLIENTDATA *pClient, uint32_t u32Format, void *pv, uint32_t cb, uint32_t *pcbActual)
{
    LogFlow(("vboxClipboardReadData: u32Format = %02X\n", u32Format));

    HANDLE hClip = NULL;

    /*
     * The guest wants to read data in the given format.
     */
    int rc = vboxOpenClipboard(pClient->pCtx->hwnd);
    if (RT_SUCCESS(rc))
    {
        dprintf(("Clipboard opened.\n"));

        if (u32Format & VBOX_SHARED_CLIPBOARD_FMT_BITMAP)
        {
            hClip = GetClipboardData (CF_DIB);

            if (hClip != NULL)
            {
                LPVOID lp = GlobalLock (hClip);

                if (lp != NULL)
                {
                    dprintf(("CF_DIB\n"));

                    vboxClipboardGetData (VBOX_SHARED_CLIPBOARD_FMT_BITMAP, lp, GlobalSize (hClip),
                                          pv, cb, pcbActual);

                    GlobalUnlock(hClip);
                }
                else
                {
                    hClip = NULL;
                }
            }
        }
        else if (u32Format & VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT)
        {
            hClip = GetClipboardData(CF_UNICODETEXT);

            if (hClip != NULL)
            {
                LPWSTR uniString = (LPWSTR)GlobalLock (hClip);

                if (uniString != NULL)
                {
                    dprintf(("CF_UNICODETEXT\n"));

                    vboxClipboardGetData (VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT, uniString, (lstrlenW (uniString) + 1) * 2,
                                          pv, cb, pcbActual);

                    GlobalUnlock(hClip);
                }
                else
                {
                    hClip = NULL;
                }
            }
        }
        else if (u32Format & VBOX_SHARED_CLIPBOARD_FMT_HTML)
        {
            UINT format = RegisterClipboardFormat ("HTML Format");

            if (format != 0)
            {
                hClip = GetClipboardData (format);

                if (hClip != NULL)
                {
                    LPVOID lp = GlobalLock (hClip);

                    if (lp != NULL)
                    {
                        dprintf(("CF_HTML\n"));

                        vboxClipboardGetData (VBOX_SHARED_CLIPBOARD_FMT_HTML, lp, GlobalSize (hClip),
                                              pv, cb, pcbActual);
                        LogRelFlowFunc(("Raw HTML clipboard data from host :"));
                        DumpHtml((char*)pv, cb);
                        GlobalUnlock(hClip);
                    }
                    else
                    {
                        hClip = NULL;
                    }
                }
            }
        }

        CloseClipboard ();
    }
    else
    {
        dprintf(("vboxClipboardReadData: failed to open clipboard, rc: %Rrc\n", rc));
    }

    if (hClip == NULL)
    {
        /* Reply with empty data. */
        vboxClipboardGetData (0, NULL, 0,
                              pv, cb, pcbActual);
    }

    return VINF_SUCCESS;
}

void vboxClipboardWriteData (VBOXCLIPBOARDCLIENTDATA *pClient, void *pv, uint32_t cb, uint32_t u32Format)
{
    LogFlow(("vboxClipboardWriteData\n"));

    /*
     * The guest returns data that was requested in the WM_RENDERFORMAT handler.
     */
    Assert(pClient->data.pv == NULL && pClient->data.cb == 0 && pClient->data.u32Format == 0);

    vboxClipboardDump(pv, cb, u32Format);

    if (cb > 0)
    {
        char *pszResult = NULL;

        if (   u32Format == VBOX_SHARED_CLIPBOARD_FMT_HTML
            && !IsWindowsHTML((const char*)pv))
        {
            /* check that this is not already CF_HTML */
            uint32_t cbResult;
            int rc = ConvertMimeToCFHTML((const char *)pv, cb, &pszResult, &cbResult);
            if (RT_SUCCESS(rc))
            {
                if (pszResult != NULL && cbResult != 0)
                {
                    pClient->data.pv        = pszResult;
                    pClient->data.cb        = cbResult;
                    pClient->data.u32Format = u32Format;
                }
            }
        }
        else
        {
            pClient->data.pv = RTMemDup(pv, cb);
            if (pClient->data.pv)
            {
                pClient->data.cb = cb;
                pClient->data.u32Format = u32Format;
            }
        }
    }

    SetEvent(pClient->pCtx->hRenderEvent);
}


/**
 * Extracts field value from CF_HTML struct
 *
 * @returns VBox status code
 * @param   pszSrc      source in CF_HTML format
 * @param   pszOption   Name of CF_HTML field
 * @param   puValue     Where to return extracted value of CF_HTML field
 */
static int GetHeaderValue(const char *pszSrc, const char *pszOption, uint32_t *puValue)
{
    int rc = VERR_INVALID_PARAMETER;

    Assert(pszSrc);
    Assert(pszOption);

    const char *pszOptionValue = RTStrStr(pszSrc, pszOption);
    if (pszOptionValue)
    {
        size_t cchOption = strlen(pszOption);
        Assert(cchOption);

        rc = RTStrToUInt32Ex(pszOptionValue + cchOption, NULL, 10, puValue);
    }
    return rc;
}


/**
 * Check that the source string contains CF_HTML struct
 *
 * @param   pszSource   source string.
 *
 * @returns @c true if the @a pszSource string is in CF_HTML format
 */
static bool IsWindowsHTML(const char *pszSource)
{
    return RTStrStr(pszSource, "Version:") != NULL
        && RTStrStr(pszSource, "StartHTML:") != NULL;
}


/*
 * Converts clipboard data from CF_HTML format to mimie clipboard format
 *
 * Returns allocated buffer that contains html converted to text/html mime type
 *
 * @returns VBox status code.
 * @param   pszSource   The input.
 * @param   cch         The length of the input.
 * @param   ppszOutput  Where to return the result.  Free using RTMemFree.
 * @param   pcbOutput   Where to the return length of the result (bytes/chars).
 */
static int ConvertCFHtmlToMime(const char *pszSource, const uint32_t cch, char **ppszOutput, uint32_t *pcbOutput)
{
    Assert(pszSource);
    Assert(cch);
    Assert(ppszOutput);
    Assert(pcbOutput);

    uint32_t offStart;
    int rc = GetHeaderValue(pszSource, "StartFragment:", &offStart);
    if (RT_SUCCESS(rc))
    {
        uint32_t offEnd;
        rc = GetHeaderValue(pszSource, "EndFragment:", &offEnd);
        if (RT_SUCCESS(rc))
        {
            if (   offStart > 0
                && offEnd > 0
                && offEnd > offStart
                && offEnd <= cch)
            {
                uint32_t cchSubStr = offEnd - offStart;
                char *pszResult = (char *)RTMemAlloc(cchSubStr + 1);
                if (pszResult)
                {
                    rc = RTStrCopyEx(pszResult, cchSubStr + 1, pszSource + offStart, cchSubStr);
                    if (RT_SUCCESS(rc))
                    {
                        *ppszOutput = pszResult;
                        *pcbOutput  = (uint32_t)(cchSubStr + 1);
                        rc = VINF_SUCCESS;
                    }
                    else
                    {
                        LogRelFlowFunc(("Error: Unknown CF_HTML format. Expected EndFragment. rc = %Rrc\n", rc));
                        RTMemFree(pszResult);
                    }
                }
                else
                {
                    LogRelFlowFunc(("Error: Unknown CF_HTML format. Expected EndFragment.\n"));
                    rc = VERR_NO_MEMORY;
                }
            }
            else
            {
                LogRelFlowFunc(("Error: CF_HTML out of bounds - offStart=%#x offEnd=%#x cch=%#x\n", offStart, offEnd, cch));
                rc = VERR_INVALID_PARAMETER;
            }
        }
        else
        {
            LogRelFlowFunc(("Error: Unknown CF_HTML format. Expected EndFragment. rc = %Rrc.\n", rc));
            rc = VERR_INVALID_PARAMETER;
        }
    }
    else
    {
        LogRelFlowFunc(("Error: Unknown CF_HTML format. Expected StartFragment. rc = %Rrc.\n", rc));
        rc = VERR_INVALID_PARAMETER;
    }

    return rc;
}



/**
 * Converts source UTF-8 MIME HTML clipboard data to UTF-8 CF_HTML format.
 *
 * This is just encapsulation work, slapping a header on the data.
 *
 * It allocates
 *
 * Calculations:
 *   Header length = format Length + (2*(10 - 5('%010d'))('digits')) - 2('%s') = format length + 8
 *   EndHtml       = Header length + fragment length
 *   StartHtml     = 105(constant)
 *   StartFragment = 141(constant) may vary if the header html content will be extended
 *   EndFragment   = Header length + fragment length - 38(ending length)
 *
 * @param   pszSource   Source buffer that contains utf-16 string in mime html format
 * @param   cb          Size of source buffer in bytes
 * @param   ppszOutput  Where to return the allocated output buffer to put converted UTF-8
 *                      CF_HTML clipboard data.  This function allocates memory for this.
 * @param   pcbOutput   Where to return the size of allocated result buffer in bytes/chars, including zero terminator
 *
 * @note    output buffer should be free using RTMemFree()
 * @note    Everything inside of fragment can be UTF8. Windows allows it. Everything in header should be Latin1.
 */
static int ConvertMimeToCFHTML(const char *pszSource, size_t cb, char **ppszOutput, uint32_t *pcbOutput)
{
    Assert(ppszOutput);
    Assert(pcbOutput);
    Assert(pszSource);
    Assert(cb);

    /* construct CF_HTML formatted string */
    char *pszResult = NULL;
    size_t cchFragment;
    int rc = RTStrNLenEx(pszSource, cb, &cchFragment);
    if (!RT_SUCCESS(rc))
    {
        LogRelFlowFunc(("Error: invalid source fragment. rc = %Rrc.\n"));
        return VERR_INVALID_PARAMETER;
    }

    /*
    @StartHtml - pos before <html>
    @EndHtml - whole size of text excluding ending zero char
    @StartFragment - pos after <!--StartFragment-->
    @EndFragment - pos before <!--EndFragment-->
    @note: all values includes CR\LF inserted into text
    Calculations:
    Header length = format Length + (3*6('digits')) - 2('%s') = format length + 16 (control value - 183)
    EndHtml  = Header length + fragment length
    StartHtml = 105(constant)
    StartFragment = 143(constant)
    EndFragment  = Header length + fragment length - 40(ending length)
    */
    static const char s_szFormatSample[] =
    /*   0:   */ "Version:1.0\r\n"
    /*  13:   */ "StartHTML:000000101\r\n"
    /*  34:   */ "EndHTML:%0000009u\r\n" // END HTML = Header length + fragment length
    /*  53:   */ "StartFragment:000000137\r\n"
    /*  78:   */ "EndFragment:%0000009u\r\n"
    /* 101:   */ "<html>\r\n"
    /* 109:   */ "<body>\r\n"
    /* 117:   */ "<!--StartFragment-->"
    /* 137:   */ "%s"
    /* 137+2: */ "<!--EndFragment-->\r\n"
    /* 157+2: */ "</body>\r\n"
    /* 166+2: */ "</html>\r\n";
    /* 175+2: */
    AssertCompile(sizeof(s_szFormatSample) == 175+2+1);

    /* calculate parameters of CF_HTML header */
    size_t cchHeader      = sizeof(s_szFormatSample) - 1;
    size_t offEndHtml     = cchHeader + cchFragment;
    size_t offEndFragment = cchHeader + cchFragment - 38; /* 175-137 = 38 */
    pszResult = (char *)RTMemAlloc(offEndHtml + 1);
    if (pszResult == NULL)
    {
        LogRelFlowFunc(("Error: Cannot allocate memory for result buffer. rc = %Rrc.\n"));
        return VERR_NO_MEMORY;
    }

    /* format result CF_HTML string */
    size_t cchFormatted = RTStrPrintf(pszResult, offEndHtml + 1,
                                      s_szFormatSample, offEndHtml, offEndFragment, pszSource);
    Assert(offEndHtml == cchFormatted); NOREF(cchFormatted);

#ifdef VBOX_STRICT
    /* Control calculations. check consistency.*/
    static const char s_szStartFragment[] = "<!--StartFragment-->";
    static const char s_szEndFragment[] = "<!--EndFragment-->";

    /* check 'StartFragment:' value */
    const char *pszRealStartFragment = RTStrStr(pszResult, s_szStartFragment);
    Assert(&pszRealStartFragment[sizeof(s_szStartFragment) - 1] - pszResult == 137);

    /* check 'EndFragment:' value */
    const char *pszRealEndFragment = RTStrStr(pszResult, s_szEndFragment);
    Assert((size_t)(pszRealEndFragment - pszResult) == offEndFragment);
#endif

    *ppszOutput = pszResult;
    *pcbOutput = (uint32_t)cchFormatted + 1;
    Assert(*pcbOutput == cchFormatted + 1);

    return VINF_SUCCESS;
}
