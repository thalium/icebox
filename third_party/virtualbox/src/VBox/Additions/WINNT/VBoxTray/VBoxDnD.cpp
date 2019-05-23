/* $Id: VBoxDnD.cpp $ */
/** @file
 * VBoxDnD.cpp - Windows-specific bits of the drag and drop service.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_GUEST_DND
#include <iprt/win/windows.h>
#include "VBoxTray.h"
#include "VBoxHelpers.h"
#include "VBoxDnD.h"

#include <VBox/VBoxGuestLib.h>
#include "VBox/HostServices/DragAndDropSvc.h"

using namespace DragAndDropSvc;

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/ldr.h>
#include <iprt/list.h>
#include <iprt/mem.h>

#include <iprt/cpp/mtlist.h>
#include <iprt/cpp/ministring.h>

#include <iprt/cpp/mtlist.h>

#include <VBox/log.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/* Enable this define to see the proxy window(s) when debugging
 * their behavior. Don't have this enabled in release builds! */
#ifdef DEBUG
//# define VBOX_DND_DEBUG_WND
#endif

/** The drag and drop window's window class. */
#define VBOX_DND_WND_CLASS            "VBoxTrayDnDWnd"

/** @todo Merge this with messages from VBoxTray.h. */
#define WM_VBOXTRAY_DND_MESSAGE       WM_APP + 401


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/** Function pointer for SendInput(). This only is available starting
 *  at NT4 SP3+. */
typedef BOOL (WINAPI *PFNSENDINPUT)(UINT, LPINPUT, int);
typedef BOOL (WINAPI* PFNENUMDISPLAYMONITORS)(HDC, LPCRECT, MONITORENUMPROC, LPARAM);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Static pointer to SendInput() function. */
static PFNSENDINPUT             g_pfnSendInput = NULL;
static PFNENUMDISPLAYMONITORS   g_pfnEnumDisplayMonitors = NULL;

static VBOXDNDCONTEXT           g_Ctx = { 0 };


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static LRESULT CALLBACK vboxDnDWndProcInstance(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK vboxDnDWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);




VBoxDnDWnd::VBoxDnDWnd(void)
    : hThread(NIL_RTTHREAD),
      mEventSem(NIL_RTSEMEVENT),
      hWnd(NULL),
      uAllActions(DND_IGNORE_ACTION),
      mfMouseButtonDown(false),
#ifdef VBOX_WITH_DRAG_AND_DROP_GH
      pDropTarget(NULL),
#endif
      mMode(Unknown),
      mState(Uninitialized)
{
    RT_ZERO(startupInfo);

    LogFlowFunc(("Supported formats:\n"));
    const RTCString arrEntries[] = { VBOX_DND_FORMATS_DEFAULT };
    for (size_t i = 0; i < RT_ELEMENTS(arrEntries); i++)
    {
        LogFlowFunc(("\t%s\n", arrEntries[i].c_str()));
        this->lstFmtSup.append(arrEntries[i]);
    }
}

VBoxDnDWnd::~VBoxDnDWnd(void)
{
    Destroy();
}

/**
 * Initializes the proxy window with a given DnD context.
 *
 * @return  IPRT status code.
 * @param   pCtx    Pointer to context to use.
 */
int VBoxDnDWnd::Initialize(PVBOXDNDCONTEXT pCtx)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    /* Save the context. */
    this->pCtx = pCtx;

    int rc = RTSemEventCreate(&mEventSem);
    if (RT_SUCCESS(rc))
        rc = RTCritSectInit(&mCritSect);

    if (RT_SUCCESS(rc))
    {
        /* Message pump thread for our proxy window. */
        rc = RTThreadCreate(&hThread, VBoxDnDWnd::Thread, this,
                            0, RTTHREADTYPE_MSG_PUMP, RTTHREADFLAGS_WAITABLE,
                            "dndwnd"); /** @todo Include ID if there's more than one proxy window. */
        if (RT_SUCCESS(rc))
            rc = RTThreadUserWait(hThread, 30 * 1000 /* Timeout in ms */);
    }

    if (RT_FAILURE(rc))
        LogRel(("DnD: Failed to initialize proxy window, rc=%Rrc\n", rc));

    LogFlowThisFunc(("Returning rc=%Rrc\n", rc));
    return rc;
}

/**
 * Destroys the proxy window and releases all remaining
 * resources again.
 */
void VBoxDnDWnd::Destroy(void)
{
    if (hThread != NIL_RTTHREAD)
    {
        int rcThread = VERR_WRONG_ORDER;
        int rc = RTThreadWait(hThread, 60 * 1000 /* Timeout in ms */, &rcThread);
        LogFlowFunc(("Waiting for thread resulted in %Rrc (thread exited with %Rrc)\n",
                     rc, rcThread));
        NOREF(rc);
    }

    reset();

    RTCritSectDelete(&mCritSect);
    if (mEventSem != NIL_RTSEMEVENT)
    {
        RTSemEventDestroy(mEventSem);
        mEventSem = NIL_RTSEMEVENT;
    }

    if (pCtx->wndClass != 0)
    {
        UnregisterClass(VBOX_DND_WND_CLASS, pCtx->pEnv->hInstance);
        pCtx->wndClass = 0;
    }

    LogFlowFuncLeave();
}

/**
 * Thread for handling the window's message pump.
 *
 * @return  IPRT status code.
 * @param   hThread                 Handle to this thread.
 * @param   pvUser                  Pointer to VBoxDnDWnd instance which
 *                                  is using the thread.
 */
/* static */
int VBoxDnDWnd::Thread(RTTHREAD hThread, void *pvUser)
{
    AssertPtrReturn(pvUser, VERR_INVALID_POINTER);

    LogFlowFuncEnter();

    VBoxDnDWnd *pThis = (VBoxDnDWnd*)pvUser;
    AssertPtr(pThis);

    PVBOXDNDCONTEXT pCtx = pThis->pCtx;
    AssertPtr(pCtx);
    AssertPtr(pCtx->pEnv);

    int rc = VINF_SUCCESS;

    AssertPtr(pCtx->pEnv);
    HINSTANCE hInstance = pCtx->pEnv->hInstance;
    Assert(hInstance != 0);

    /* Create our proxy window. */
    WNDCLASSEX wc = { 0 };
    wc.cbSize     = sizeof(WNDCLASSEX);

    if (!GetClassInfoEx(hInstance, VBOX_DND_WND_CLASS, &wc))
    {
        wc.lpfnWndProc   = vboxDnDWndProc;
        wc.lpszClassName = VBOX_DND_WND_CLASS;
        wc.hInstance     = hInstance;
        wc.style         = CS_NOCLOSE;
#ifdef VBOX_DND_DEBUG_WND
        wc.style        |= CS_HREDRAW | CS_VREDRAW;
        wc.hbrBackground = (HBRUSH)(CreateSolidBrush(RGB(255, 0, 0)));
#else
        wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
#endif
        if (!RegisterClassEx(&wc))
        {
            DWORD dwErr = GetLastError();
            LogFlowFunc(("Unable to register proxy window class, error=%ld\n", dwErr));
            rc = RTErrConvertFromWin32(dwErr);
        }
    }

    if (RT_SUCCESS(rc))
    {
        DWORD dwExStyle = WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
        DWORD dwStyle = WS_POPUP;
#ifdef VBOX_DND_DEBUG_WND
        dwExStyle &= ~WS_EX_TRANSPARENT; /* Remove transparency bit. */
        dwStyle |= WS_VISIBLE; /* Make the window visible. */
#endif
        pThis->hWnd =
            CreateWindowEx(dwExStyle,
                           VBOX_DND_WND_CLASS, VBOX_DND_WND_CLASS,
                           dwStyle,
#ifdef VBOX_DND_DEBUG_WND
                           CW_USEDEFAULT, CW_USEDEFAULT, 200, 200, NULL, NULL,
#else
                           -200, -200, 100, 100, NULL, NULL,
#endif
                           hInstance, pThis /* lParm */);
        if (!pThis->hWnd)
        {
            DWORD dwErr = GetLastError();
            LogFlowFunc(("Unable to create proxy window, error=%ld\n", dwErr));
            rc = RTErrConvertFromWin32(dwErr);
        }
        else
        {
#ifndef VBOX_DND_DEBUG_WND
            SetWindowPos(pThis->hWnd, HWND_TOPMOST, -200, -200, 0, 0,
                           SWP_NOACTIVATE | SWP_HIDEWINDOW
                         | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOSIZE);
            LogFlowFunc(("Proxy window created, hWnd=0x%x\n", pThis->hWnd));
#else
            LogFlowFunc(("Debug proxy window created, hWnd=0x%x\n", pThis->hWnd));

            /*
             * Install some mouse tracking.
             */
            TRACKMOUSEEVENT me;
            RT_ZERO(me);
            me.cbSize    = sizeof(TRACKMOUSEEVENT);
            me.dwFlags   = TME_HOVER | TME_LEAVE | TME_NONCLIENT;
            me.hwndTrack = pThis->hWnd;
            BOOL fRc = TrackMouseEvent(&me);
            Assert(fRc);
#endif
        }
    }

    HRESULT hr = OleInitialize(NULL);
    if (SUCCEEDED(hr))
    {
#ifdef VBOX_WITH_DRAG_AND_DROP_GH
        rc = pThis->RegisterAsDropTarget();
#else
        rc = VINF_SUCCESS;
#endif
    }
    else
    {
        LogRel(("DnD: Unable to initialize OLE, hr=%Rhrc\n", hr));
        rc = VERR_COM_UNEXPECTED;
    }

    bool fSignalled = false;

    if (RT_SUCCESS(rc))
    {
        rc = RTThreadUserSignal(hThread);
        fSignalled = RT_SUCCESS(rc);

        bool fShutdown = false;
        for (;;)
        {
            MSG uMsg;
            BOOL fRet;
            while ((fRet = GetMessage(&uMsg, 0, 0, 0)) > 0)
            {
                TranslateMessage(&uMsg);
                DispatchMessage(&uMsg);
            }
            Assert(fRet >= 0);

            if (ASMAtomicReadBool(&pCtx->fShutdown))
                fShutdown = true;

            if (fShutdown)
            {
                LogFlowFunc(("Closing proxy window ...\n"));
                break;
            }

            /** @todo Immediately drop on failure? */
        }

#ifdef VBOX_WITH_DRAG_AND_DROP_GH
        int rc2 = pThis->UnregisterAsDropTarget();
        if (RT_SUCCESS(rc))
            rc = rc2;
#endif
        OleUninitialize();
    }

    if (!fSignalled)
    {
        int rc2 = RTThreadUserSignal(hThread);
        AssertRC(rc2);
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Monitor enumeration callback for building up a simple bounding
 * box, capable of holding all enumerated monitors.
 *
 * @return  BOOL                    TRUE if enumeration should continue,
 *                                  FALSE if not.
 * @param   hMonitor                Handle to current monitor being enumerated.
 * @param   hdcMonitor              The current monitor's DC (device context).
 * @param   lprcMonitor             The current monitor's RECT.
 * @param   lParam                  Pointer to a RECT structure holding the
 *                                  bounding box to build.
 */
/* static */
BOOL CALLBACK VBoxDnDWnd::MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM lParam)
{
    RT_NOREF(hMonitor, hdcMonitor);
    LPRECT pRect = (LPRECT)lParam;
    AssertPtrReturn(pRect, FALSE);

    AssertPtr(lprcMonitor);
    LogFlowFunc(("Monitor is %ld,%ld,%ld,%ld\n",
                 lprcMonitor->left, lprcMonitor->top,
                 lprcMonitor->right, lprcMonitor->bottom));

    /* Build up a simple bounding box to hold the entire (virtual) screen. */
    if (pRect->left > lprcMonitor->left)
        pRect->left = lprcMonitor->left;
    if (pRect->right < lprcMonitor->right)
        pRect->right = lprcMonitor->right;
    if (pRect->top > lprcMonitor->top)
        pRect->top = lprcMonitor->top;
    if (pRect->bottom < lprcMonitor->bottom)
        pRect->bottom = lprcMonitor->bottom;

    return TRUE;
}

/**
 * The proxy window's WndProc.
 */
LRESULT CALLBACK VBoxDnDWnd::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            int rc = OnCreate();
            /** @todo r=bird: MSDN says this returns 0 on success and -1 on failure, not
             *        TRUE/FALSE... */
            if (RT_FAILURE(rc))
                return FALSE;
            return TRUE;
        }

        case WM_QUIT:
        {
            LogFlowThisFunc(("WM_QUIT\n"));
            PostQuitMessage(0);
            return 0;
        }

        case WM_DESTROY:
        {
            LogFlowThisFunc(("WM_DESTROY\n"));

            OnDestroy();
            return 0;
        }

        case WM_LBUTTONDOWN:
        {
            LogFlowThisFunc(("WM_LBUTTONDOWN\n"));
            mfMouseButtonDown = true;
            return 0;
        }

        case WM_LBUTTONUP:
        {
            LogFlowThisFunc(("WM_LBUTTONUP\n"));
            mfMouseButtonDown = false;

            /* As the mouse button was released, Hide the proxy window again.
             * This can happen if
             * - the user bumped a guest window to the screen's edges
             * - there was no drop data from the guest available and the user
             *   enters the guest screen again after this unsuccessful operation */
            reset();
            return 0;
        }

        case WM_MOUSELEAVE:
        {
            LogFlowThisFunc(("WM_MOUSELEAVE\n"));
            return 0;
        }

        /* Will only be called once; after the first mouse move, this
         * window will be hidden! */
        case WM_MOUSEMOVE:
        {
            LogFlowThisFunc(("WM_MOUSEMOVE: mfMouseButtonDown=%RTbool, mMode=%ld, mState=%ld\n",
                             mfMouseButtonDown, mMode, mState));
#ifdef DEBUG_andy
            POINT p;
            GetCursorPos(&p);
            LogFlowThisFunc(("WM_MOUSEMOVE: curX=%ld, curY=%ld\n", p.x, p.y));
#endif
            int rc = VINF_SUCCESS;
            if (mMode == HG) /* Host to guest. */
            {
                /* Dragging not started yet? Kick it off ... */
                if (   mfMouseButtonDown
                    && (mState != Dragging))
                {
                    mState = Dragging;
#if 0
                    /* Delay hiding the proxy window a bit when debugging, to see
                     * whether the desired range is covered correctly. */
                    RTThreadSleep(5000);
#endif
                    hide();

                    LogFlowThisFunc(("Starting drag and drop: uAllActions=0x%x, dwOKEffects=0x%x ...\n",
                                     uAllActions, startupInfo.dwOKEffects));

                    AssertPtr(startupInfo.pDataObject);
                    AssertPtr(startupInfo.pDropSource);
                    DWORD dwEffect;
                    HRESULT hr = DoDragDrop(startupInfo.pDataObject, startupInfo.pDropSource,
                                            startupInfo.dwOKEffects, &dwEffect);
                    LogFlowThisFunc(("hr=%Rhrc, dwEffect=%RI32\n", hr, dwEffect));
                    switch (hr)
                    {
                        case DRAGDROP_S_DROP:
                            mState = Dropped;
                            break;

                        case DRAGDROP_S_CANCEL:
                            mState = Canceled;
                            break;

                        default:
                            LogFlowThisFunc(("Drag and drop failed with %Rhrc\n", hr));
                            mState = Canceled;
                            rc = VERR_GENERAL_FAILURE; /** @todo Find a better status code. */
                            break;
                    }

                    int rc2 = RTCritSectEnter(&mCritSect);
                    if (RT_SUCCESS(rc2))
                    {
                        startupInfo.pDropSource->Release();
                        startupInfo.pDataObject->Release();

                        RT_ZERO(startupInfo);

                        rc2 = RTCritSectLeave(&mCritSect);
                        if (RT_SUCCESS(rc))
                            rc = rc2;
                    }

                    mMode = Unknown;
                }
            }
            else if (mMode == GH) /* Guest to host. */
            {
                /* Starting here VBoxDnDDropTarget should
                 * take over; was instantiated when registering
                 * this proxy window as a (valid) drop target. */
            }
            else
                rc = VERR_NOT_SUPPORTED;

            LogFlowThisFunc(("WM_MOUSEMOVE: mMode=%ld, mState=%ld, rc=%Rrc\n",
                             mMode, mState, rc));
            return 0;
        }

        case WM_NCMOUSEHOVER:
            LogFlowThisFunc(("WM_NCMOUSEHOVER\n"));
            return 0;

        case WM_NCMOUSELEAVE:
            LogFlowThisFunc(("WM_NCMOUSELEAVE\n"));
            return 0;

        case WM_VBOXTRAY_DND_MESSAGE:
        {
            VBOXDNDEVENT *pEvent = (PVBOXDNDEVENT)lParam;
            if (!pEvent)
                break; /* No event received, bail out. */

            LogFlowThisFunc(("Received uType=%RU32, uScreenID=%RU32\n",
                         pEvent->Event.uType, pEvent->Event.uScreenId));

            int rc;
            switch (pEvent->Event.uType)
            {
                case HOST_DND_HG_EVT_ENTER:
                {
                    LogFlowThisFunc(("HOST_DND_HG_EVT_ENTER\n"));

                    if (pEvent->Event.cbFormats)
                    {
                        RTCList<RTCString> lstFormats =
                            RTCString(pEvent->Event.pszFormats, pEvent->Event.cbFormats - 1).split("\r\n");
                        rc = OnHgEnter(lstFormats, pEvent->Event.u.a.uAllActions);
                    }
                    else
                    {
                        AssertMsgFailed(("cbFormats is 0\n"));
                        rc = VERR_INVALID_PARAMETER;
                    }

                    /* Note: After HOST_DND_HG_EVT_ENTER there immediately is a move
                     *       event, so fall through is intentional here. */
                }

                case HOST_DND_HG_EVT_MOVE:
                {
                    LogFlowThisFunc(("HOST_DND_HG_EVT_MOVE: %d,%d\n",
                                     pEvent->Event.u.a.uXpos, pEvent->Event.u.a.uYpos));

                    rc = OnHgMove(pEvent->Event.u.a.uXpos, pEvent->Event.u.a.uYpos,
                                  pEvent->Event.u.a.uDefAction);
                    break;
                }

                case HOST_DND_HG_EVT_LEAVE:
                {
                    LogFlowThisFunc(("HOST_DND_HG_EVT_LEAVE\n"));

                    rc = OnHgLeave();
                    break;
                }

                case HOST_DND_HG_EVT_DROPPED:
                {
                    LogFlowThisFunc(("HOST_DND_HG_EVT_DROPPED\n"));

                    rc = OnHgDrop();
                    break;
                }

                case HOST_DND_HG_SND_DATA:
                    /* Protocol v1 + v2: Also contains the header data.
                    /* Note: Fall through is intentional. */
                case HOST_DND_HG_SND_DATA_HDR:
                {
                    LogFlowThisFunc(("HOST_DND_HG_SND_DATA\n"));

                    rc = OnHgDataReceived(pEvent->Event.u.b.pvData,
                                          pEvent->Event.u.b.cbData);
                    break;
                }

                case HOST_DND_HG_EVT_CANCEL:
                {
                    LogFlowThisFunc(("HOST_DND_HG_EVT_CANCEL\n"));

                    rc = OnHgCancel();
                    break;
                }

                case HOST_DND_GH_REQ_PENDING:
                {
                    LogFlowThisFunc(("HOST_DND_GH_REQ_PENDING\n"));
#ifdef VBOX_WITH_DRAG_AND_DROP_GH
                    rc = OnGhIsDnDPending(pEvent->Event.uScreenId);

#else
                    rc = VERR_NOT_SUPPORTED;
#endif
                    break;
                }

                case HOST_DND_GH_EVT_DROPPED:
                {
                    LogFlowThisFunc(("HOST_DND_GH_EVT_DROPPED\n"));
#ifdef VBOX_WITH_DRAG_AND_DROP_GH
                    rc = OnGhDropped(pEvent->Event.pszFormats,
                                     pEvent->Event.cbFormats,
                                     pEvent->Event.u.a.uDefAction);
#else
                    rc = VERR_NOT_SUPPORTED;
#endif
                    break;
                }

                default:
                    rc = VERR_NOT_SUPPORTED;
                    break;
            }

            /* Some messages require cleanup. */
            switch (pEvent->Event.uType)
            {
                case HOST_DND_HG_EVT_ENTER:
                case HOST_DND_HG_EVT_MOVE:
                case HOST_DND_HG_EVT_DROPPED:
#ifdef VBOX_WITH_DRAG_AND_DROP_GH
                case HOST_DND_GH_EVT_DROPPED:
#endif
                {
                    if (pEvent->Event.pszFormats)
                        RTMemFree(pEvent->Event.pszFormats);
                    break;
                }

                case HOST_DND_HG_SND_DATA:
                case HOST_DND_HG_SND_DATA_HDR:
                {
                    if (pEvent->Event.pszFormats)
                        RTMemFree(pEvent->Event.pszFormats);
                    if (pEvent->Event.u.b.pvData)
                        RTMemFree(pEvent->Event.u.b.pvData);
                    break;
                }

                default:
                    /* Ignore. */
                    break;
            }

            if (pEvent)
            {
                LogFlowThisFunc(("Processing event %RU32 resulted in rc=%Rrc\n",
                                 pEvent->Event.uType, rc));

                RTMemFree(pEvent);
            }
            return 0;
        }

        default:
            break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

#ifdef VBOX_WITH_DRAG_AND_DROP_GH
/**
 * Registers this proxy window as a local drop target.
 *
 * @return  IPRT status code.
 */
int VBoxDnDWnd::RegisterAsDropTarget(void)
{
    if (pDropTarget) /* Already registered as drop target? */
        return VINF_SUCCESS;

    int rc;
    try
    {
        pDropTarget = new VBoxDnDDropTarget(this /* pParent */);
        HRESULT hr = CoLockObjectExternal(pDropTarget, TRUE /* fLock */,
                                          FALSE /* fLastUnlockReleases */);
        if (SUCCEEDED(hr))
            hr = RegisterDragDrop(hWnd, pDropTarget);

        if (FAILED(hr))
        {
            LogRel(("DnD: Creating drop target failed with hr=%Rhrc\n", hr));
            rc = VERR_GENERAL_FAILURE; /** @todo Find a better rc. */
        }
        else
        {
            rc = VINF_SUCCESS;
        }
    }
    catch (std::bad_alloc)
    {
        rc = VERR_NO_MEMORY;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Unregisters this proxy as a drop target.
 *
 * @return  IPRT status code.
 */
int VBoxDnDWnd::UnregisterAsDropTarget(void)
{
    LogFlowFuncEnter();

    if (!pDropTarget) /* No drop target? Bail out. */
        return VINF_SUCCESS;

    HRESULT hr = RevokeDragDrop(hWnd);
    if (SUCCEEDED(hr))
        hr = CoLockObjectExternal(pDropTarget, FALSE /* fLock */,
                                  TRUE /* fLastUnlockReleases */);
    if (SUCCEEDED(hr))
    {
        ULONG cRefs = pDropTarget->Release();
        Assert(cRefs == 0); NOREF(cRefs);
        pDropTarget = NULL;
    }

    int rc = SUCCEEDED(hr)
           ? VINF_SUCCESS : VERR_GENERAL_FAILURE; /** @todo Fix this. */

    LogFlowFuncLeaveRC(rc);
    return rc;
}
#endif /* VBOX_WITH_DRAG_AND_DROP_GH */

/**
 * Handles the creation of a proxy window.
 *
 * @return  IPRT status code.
 */
int VBoxDnDWnd::OnCreate(void)
{
    LogFlowFuncEnter();
    int rc = VbglR3DnDConnect(&mDnDCtx);
    if (RT_FAILURE(rc))
    {
        LogFlowThisFunc(("Connection to host service failed, rc=%Rrc\n", rc));
        return rc;
    }

    LogFlowThisFunc(("Client ID=%RU32, rc=%Rrc\n", mDnDCtx.uClientID, rc));
    return rc;
}

/**
 * Handles the destruction of a proxy window.
 */
void VBoxDnDWnd::OnDestroy(void)
{
    DestroyWindow(hWnd);

    VbglR3DnDDisconnect(&mDnDCtx);
    LogFlowThisFuncLeave();
}

/**
 * Handles actions required when the host cursor enters
 * the guest's screen to initiate a host -> guest DnD operation.
 *
 * @return  IPRT status code.
 * @param   lstFormats              Supported formats offered by the host.
 * @param   uAllActions             Supported actions offered by the host.
 */
int VBoxDnDWnd::OnHgEnter(const RTCList<RTCString> &lstFormats, uint32_t uAllActions)
{
    if (mMode == GH) /* Wrong mode? Bail out. */
        return VERR_WRONG_ORDER;

#ifdef DEBUG
    LogFlowThisFunc(("uActions=0x%x, lstFormats=%zu: ", uAllActions, lstFormats.size()));
    for (size_t i = 0; i < lstFormats.size(); i++)
        LogFlow(("'%s' ", lstFormats.at(i).c_str()));
    LogFlow(("\n"));
#endif

    reset();
    setMode(HG);

    int rc = VINF_SUCCESS;

    try
    {
        /* Save all allowed actions. */
        this->uAllActions = uAllActions;

        /*
         * Check if reported formats from host are compatible with this client.
         */
        size_t cFormatsSup    = this->lstFmtSup.size();
        ULONG  cFormatsActive = 0;

        LPFORMATETC pFormatEtc = new FORMATETC[cFormatsSup];
        RT_BZERO(pFormatEtc, sizeof(FORMATETC) * cFormatsSup);

        LPSTGMEDIUM pStgMeds   = new STGMEDIUM[cFormatsSup];
        RT_BZERO(pStgMeds, sizeof(STGMEDIUM) * cFormatsSup);

        LogRel2(("DnD: Reported formats:\n"));
        for (size_t i = 0; i < lstFormats.size(); i++)
        {
            bool fSupported = false;
            for (size_t a = 0; a < this->lstFmtSup.size(); a++)
            {
                const char *pszFormat = lstFormats.at(i).c_str();
                LogFlowThisFunc(("\t\"%s\" <=> \"%s\"\n", this->lstFmtSup.at(a).c_str(), pszFormat));

                fSupported = RTStrICmp(this->lstFmtSup.at(a).c_str(), pszFormat) == 0;
                if (fSupported)
                {
                    this->lstFmtActive.append(lstFormats.at(i));

                    /** @todo Put this into a \#define / struct. */
                    if (!RTStrICmp(pszFormat, "text/uri-list"))
                    {
                        pFormatEtc[cFormatsActive].cfFormat = CF_HDROP;
                        pFormatEtc[cFormatsActive].dwAspect = DVASPECT_CONTENT;
                        pFormatEtc[cFormatsActive].lindex   = -1;
                        pFormatEtc[cFormatsActive].tymed    = TYMED_HGLOBAL;

                        pStgMeds  [cFormatsActive].tymed    = TYMED_HGLOBAL;
                        cFormatsActive++;
                    }
                    else if (   !RTStrICmp(pszFormat, "text/plain")
                             || !RTStrICmp(pszFormat, "text/html")
                             || !RTStrICmp(pszFormat, "text/plain;charset=utf-8")
                             || !RTStrICmp(pszFormat, "text/plain;charset=utf-16")
                             || !RTStrICmp(pszFormat, "text/plain")
                             || !RTStrICmp(pszFormat, "text/richtext")
                             || !RTStrICmp(pszFormat, "UTF8_STRING")
                             || !RTStrICmp(pszFormat, "TEXT")
                             || !RTStrICmp(pszFormat, "STRING"))
                    {
                        pFormatEtc[cFormatsActive].cfFormat = CF_TEXT;
                        pFormatEtc[cFormatsActive].dwAspect = DVASPECT_CONTENT;
                        pFormatEtc[cFormatsActive].lindex   = -1;
                        pFormatEtc[cFormatsActive].tymed    = TYMED_HGLOBAL;

                        pStgMeds  [cFormatsActive].tymed    = TYMED_HGLOBAL;
                        cFormatsActive++;
                    }
                    else /* Should never happen. */
                        AssertReleaseMsgFailedBreak(("Format specification for '%s' not implemented\n", pszFormat));
                    break;
                }
            }

            LogRel2(("DnD: \t%s: %RTbool\n", lstFormats.at(i).c_str(), fSupported));
        }

        /*
         * Warn in the log if this guest does not accept anything.
         */
        Assert(cFormatsActive <= cFormatsSup);
        if (cFormatsActive)
        {
            LogRel2(("DnD: %RU32 supported formats found:\n", cFormatsActive));
            for (size_t i = 0; i < cFormatsActive; i++)
                LogRel2(("DnD: \t%s\n", this->lstFmtActive.at(i).c_str()));
        }
        else
            LogRel(("DnD: Warning: No supported drag and drop formats on the guest found!\n"));

        /*
         * Prepare the startup info for DoDragDrop().
         */

        /* Translate our drop actions into allowed Windows drop effects. */
        startupInfo.dwOKEffects = DROPEFFECT_NONE;
        if (uAllActions)
        {
            if (uAllActions & DND_COPY_ACTION)
                startupInfo.dwOKEffects |= DROPEFFECT_COPY;
            if (uAllActions & DND_MOVE_ACTION)
                startupInfo.dwOKEffects |= DROPEFFECT_MOVE;
            if (uAllActions & DND_LINK_ACTION)
                startupInfo.dwOKEffects |= DROPEFFECT_LINK;
        }

        LogRel2(("DnD: Supported drop actions: 0x%x\n", startupInfo.dwOKEffects));

        startupInfo.pDropSource = new VBoxDnDDropSource(this);
        startupInfo.pDataObject = new VBoxDnDDataObject(pFormatEtc, pStgMeds, cFormatsActive);

        if (pFormatEtc)
            delete pFormatEtc;
        if (pStgMeds)
            delete pStgMeds;
    }
    catch (std::bad_alloc)
    {
        rc = VERR_NO_MEMORY;
    }

    if (RT_SUCCESS(rc))
        rc = makeFullscreen();

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Handles actions required when the host cursor moves inside
 * the guest's screen.
 *
 * @return  IPRT status code.
 * @param   u32xPos                 Absolute X position (in pixels) of the host cursor
 *                                  inside the guest.
 * @param   u32yPos                 Absolute Y position (in pixels) of the host cursor
 *                                  inside the guest.
 * @param   uAction                 Action the host wants to perform while moving.
 *                                  Currently ignored.
 */
int VBoxDnDWnd::OnHgMove(uint32_t u32xPos, uint32_t u32yPos, uint32_t uAction)
{
    RT_NOREF(uAction);
    int rc;

    uint32_t uActionNotify = DND_IGNORE_ACTION;
    if (mMode == HG)
    {
        LogFlowThisFunc(("u32xPos=%RU32, u32yPos=%RU32, uAction=0x%x\n",
                         u32xPos, u32yPos, uAction));

        rc = mouseMove(u32xPos, u32yPos, MOUSEEVENTF_LEFTDOWN);

        if (RT_SUCCESS(rc))
            rc = RTCritSectEnter(&mCritSect);
        if (RT_SUCCESS(rc))
        {
            if (   (Dragging == mState)
                && startupInfo.pDropSource)
                uActionNotify = startupInfo.pDropSource->GetCurrentAction();

            RTCritSectLeave(&mCritSect);
        }
    }
    else /* Just acknowledge the operation with an ignore action. */
        rc = VINF_SUCCESS;

    if (RT_SUCCESS(rc))
    {
        rc = VbglR3DnDHGSendAckOp(&mDnDCtx, uActionNotify);
        if (RT_FAILURE(rc))
            LogFlowThisFunc(("Acknowledging operation failed with rc=%Rrc\n", rc));
    }

    LogFlowThisFunc(("Returning uActionNotify=0x%x, rc=%Rrc\n", uActionNotify, rc));
    return rc;
}

/**
 * Handles actions required when the host cursor leaves
 * the guest's screen again.
 *
 * @return  IPRT status code.
 */
int VBoxDnDWnd::OnHgLeave(void)
{
    if (mMode == GH) /* Wrong mode? Bail out. */
        return VERR_WRONG_ORDER;

    LogFlowThisFunc(("mMode=%ld, mState=%RU32\n", mMode, mState));
    LogRel(("DnD: Drag and drop operation aborted\n"));

    reset();

    int rc = VINF_SUCCESS;

    /* Post ESC to our window to officially abort the
     * drag and drop operation. */
    this->PostMessage(WM_KEYDOWN, VK_ESCAPE /* wParam */, 0 /* lParam */);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Handles actions required when the host cursor wants to drop
 * and therefore start a "drop" action in the guest.
 *
 * @return  IPRT status code.
 */
int VBoxDnDWnd::OnHgDrop(void)
{
    if (mMode == GH)
        return VERR_WRONG_ORDER;

    LogFlowThisFunc(("mMode=%ld, mState=%RU32\n", mMode, mState));

    int rc = VINF_SUCCESS;
    if (mState == Dragging)
    {
        if (lstFmtActive.size() >= 1)
        {
            /** @todo What to do when multiple formats are available? */
            mFormatRequested = lstFmtActive.at(0);

            rc = RTCritSectEnter(&mCritSect);
            if (RT_SUCCESS(rc))
            {
                if (startupInfo.pDataObject)
                    startupInfo.pDataObject->SetStatus(VBoxDnDDataObject::Dropping);
                else
                    rc = VERR_NOT_FOUND;

                RTCritSectLeave(&mCritSect);
            }

            if (RT_SUCCESS(rc))
            {
                LogRel(("DnD: Requesting data as '%s' ...\n", mFormatRequested.c_str()));
                rc = VbglR3DnDHGSendReqData(&mDnDCtx, mFormatRequested.c_str());
                if (RT_FAILURE(rc))
                    LogFlowThisFunc(("Requesting data failed with rc=%Rrc\n", rc));
            }

        }
        else /* Should never happen. */
            LogRel(("DnD: Error: Host did not specify a data format for drop data\n"));
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Handles actions required when the host has sent over DnD data
 * to the guest after a "drop" event.
 *
 * @return  IPRT status code.
 * @param   pvData                  Pointer to raw data received.
 * @param   cbData                  Size of data (in bytes) received.
 */
int VBoxDnDWnd::OnHgDataReceived(const void *pvData, uint32_t cbData)
{
    LogFlowThisFunc(("mState=%ld, pvData=%p, cbData=%RU32\n",
                     mState, pvData, cbData));

    mState = Dropped;

    int rc = VINF_SUCCESS;
    if (pvData)
    {
        Assert(cbData);
        rc = RTCritSectEnter(&mCritSect);
        if (RT_SUCCESS(rc))
        {
            if (startupInfo.pDataObject)
                rc = startupInfo.pDataObject->Signal(mFormatRequested, pvData, cbData);
            else
                rc = VERR_NOT_FOUND;

            RTCritSectLeave(&mCritSect);
        }
    }

    int rc2 = mouseRelease();
    if (RT_SUCCESS(rc))
        rc = rc2;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Handles actions required when the host wants to cancel the current
 * host -> guest operation.
 *
 * @return  IPRT status code.
 */
int VBoxDnDWnd::OnHgCancel(void)
{
    int rc = RTCritSectEnter(&mCritSect);
    if (RT_SUCCESS(rc))
    {
        if (startupInfo.pDataObject)
            startupInfo.pDataObject->Abort();

        RTCritSectLeave(&mCritSect);
    }

    int rc2 = mouseRelease();
    if (RT_SUCCESS(rc))
        rc = rc2;

    reset();

    return rc;
}

#ifdef VBOX_WITH_DRAG_AND_DROP_GH
/**
 * Handles actions required to start a guest -> host DnD operation.
 * This works by letting the host ask whether a DnD operation is pending
 * on the guest. The guest must not know anything about the host's DnD state
 * and/or operations due to security reasons.
 *
 * To capture a pending DnD operation on the guest which then can be communicated
 * to the host the proxy window needs to be registered as a drop target. This drop
 * target then will act as a proxy target between the guest OS and the host. In other
 * words, the guest OS will use this proxy target as a regular (invisible) window
 * which can be used by the regular guest OS' DnD mechanisms, independently of the
 * host OS. To make sure this proxy target is able receive an in-progress DnD operation
 * on the guest, it will be shown invisibly across all active guest OS screens. Just
 * think of an opened umbrella across all screens here.
 *
 * As soon as the proxy target and its underlying data object receive appropriate
 * DnD messages they'll be hidden again, and the control will be transferred back
 * this class again.
 *
 * @return  IPRT status code.
 * @param   uScreenID               Screen ID the host wants to query a pending operation
 *                                  for. Currently not used/needed here.
 */
int VBoxDnDWnd::OnGhIsDnDPending(uint32_t uScreenID)
{
    RT_NOREF(uScreenID);
    LogFlowThisFunc(("mMode=%ld, mState=%ld, uScreenID=%RU32\n", mMode, mState, uScreenID));

    if (mMode == Unknown)
        setMode(GH);

    if (mMode != GH)
        return VERR_WRONG_ORDER;

    if (mState == Uninitialized)
    {
        /* Nothing to do here yet. */
        mState = Initialized;
    }

    int rc;
    if (mState == Initialized)
    {
        rc = makeFullscreen();
        if (RT_SUCCESS(rc))
        {
            /*
             * We have to release the left mouse button to
             * get into our (invisible) proxy window.
             */
            mouseRelease();

            /*
             * Even if we just released the left mouse button
             * we're still in the dragging state to handle our
             * own drop target (for the host).
             */
            mState = Dragging;
        }
    }
    else
        rc = VINF_SUCCESS;

    /**
     * Some notes regarding guest cursor movement:
     * - The host only sends an HOST_DND_GH_REQ_PENDING message to the guest
     *   if the mouse cursor is outside the VM's window.
     * - The guest does not know anything about the host's cursor
     *   position / state due to security reasons.
     * - The guest *only* knows that the host currently is asking whether a
     *   guest DnD operation is in progress.
     */

    if (   RT_SUCCESS(rc)
        && mState == Dragging)
    {
        /** @todo Put this block into a function! */
        POINT p;
        GetCursorPos(&p);
        ClientToScreen(hWnd, &p);
#ifdef DEBUG_andy
        LogFlowThisFunc(("Client to screen curX=%ld, curY=%ld\n", p.x, p.y));
#endif

        /** @todo Multi-monitor setups? */
#if 0 /* unused */
        int iScreenX = GetSystemMetrics(SM_CXSCREEN) - 1;
        int iScreenY = GetSystemMetrics(SM_CYSCREEN) - 1;
#endif

        LONG px = p.x;
        if (px <= 0)
            px = 1;
        LONG py = p.y;
        if (py <= 0)
            py = 1;

        rc = mouseMove(px, py, 0 /* dwMouseInputFlags */);
    }

    if (RT_SUCCESS(rc))
    {
        uint32_t uDefAction = DND_IGNORE_ACTION;

        AssertPtr(pDropTarget);
        RTCString strFormats = pDropTarget->Formats();
        if (!strFormats.isEmpty())
        {
            uDefAction = DND_COPY_ACTION;

            LogFlowFunc(("Acknowledging pDropTarget=0x%p, uDefAction=0x%x, uAllActions=0x%x, strFormats=%s\n",
                         pDropTarget, uDefAction, uAllActions, strFormats.c_str()));
        }
        else
        {
            strFormats = "unknown"; /* Prevent VERR_IO_GEN_FAILURE for IOCTL. */
            LogFlowFunc(("No format data available yet\n"));
        }

        /** @todo Support more than one action at a time. */
        uAllActions = uDefAction;

        int rc2 = VbglR3DnDGHSendAckPending(&mDnDCtx,
                                            uDefAction, uAllActions,
                                            strFormats.c_str(), (uint32_t)strFormats.length() + 1 /* Include termination */);
        if (RT_FAILURE(rc2))
        {
            char szMsg[256]; /* Sizes according to MSDN. */
            char szTitle[64];

            /** @todo Add some i18l tr() macros here. */
            RTStrPrintf(szTitle, sizeof(szTitle), "VirtualBox Guest Additions Drag and Drop");
            RTStrPrintf(szMsg, sizeof(szMsg), "Drag and drop to the host either is not supported or disabled. "
                                              "Please enable Guest to Host or Bidirectional drag and drop mode "
                                              "or re-install the VirtualBox Guest Additions.");
            switch (rc2)
            {
                case VERR_ACCESS_DENIED:
                {
                    rc = hlpShowBalloonTip(g_hInstance, g_hwndToolWindow, ID_TRAYICON,
                                           szMsg, szTitle,
                                           15 * 1000 /* Time to display in msec */, NIIF_INFO);
                    AssertRC(rc);
                    break;
                }

                default:
                    break;
            }

            LogRel2(("DnD: Host refuses drag and drop operation from guest: %Rrc\n", rc2));
            reset();
        }
    }

    if (RT_FAILURE(rc))
        reset(); /* Reset state on failure. */

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Handles actions required to let the guest know that the host
 * started a "drop" action on the host. This will tell the guest
 * to send data in a specific format the host requested.
 *
 * @return  IPRT status code.
 * @param   pszFormat               Format the host requests the data in.
 * @param   cbFormat                Size (in bytes) of format string.
 * @param   uDefAction              Default action on the host.
 */
int VBoxDnDWnd::OnGhDropped(const char *pszFormat, uint32_t cbFormat, uint32_t uDefAction)
{
    RT_NOREF(uDefAction);
    AssertPtrReturn(pszFormat, VERR_INVALID_POINTER);
    AssertReturn(cbFormat,     VERR_INVALID_PARAMETER);

    LogFlowThisFunc(("mMode=%ld, mState=%ld, pDropTarget=0x%p, pszFormat=%s, uDefAction=0x%x\n",
                     mMode, mState, pDropTarget, pszFormat, uDefAction));
    int rc;
    if (mMode == GH)
    {
        if (mState == Dragging)
        {
            AssertPtr(pDropTarget);
            rc = pDropTarget->WaitForDrop(5 * 1000 /* 5s timeout */);

            reset();
        }
        else if (mState == Dropped)
        {
            rc = VINF_SUCCESS;
        }
        else
            rc = VERR_WRONG_ORDER;

        if (RT_SUCCESS(rc))
        {
            /** @todo Respect uDefAction. */
            void *pvData    = pDropTarget->DataMutableRaw();
            uint32_t cbData = (uint32_t)pDropTarget->DataSize();
            Assert(cbData == pDropTarget->DataSize());

            if (   pvData
                && cbData)
            {
                rc = VbglR3DnDGHSendData(&mDnDCtx, pszFormat, pvData, cbData);
                LogFlowFunc(("Sent pvData=0x%p, cbData=%RU32, rc=%Rrc\n", pvData, cbData, rc));
            }
            else
                rc = VERR_NO_DATA;
        }
    }
    else
        rc = VERR_WRONG_ORDER;

    if (RT_FAILURE(rc))
    {
        /*
         * If an error occurred or the guest is in a wrong DnD mode,
         * send an error to the host in any case so that the host does
         * not wait for the data it expects from the guest.
         */
        int rc2 = VbglR3DnDGHSendError(&mDnDCtx, rc);
        AssertRC(rc2);
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}
#endif /* VBOX_WITH_DRAG_AND_DROP_GH */

void VBoxDnDWnd::PostMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LogFlowFunc(("Posting message %u\n"));
    BOOL fRc = ::PostMessage(hWnd, uMsg, wParam, lParam);
    Assert(fRc); NOREF(fRc);
}

/**
 * Injects a DnD event in this proxy window's Windows
 * event queue. The (allocated) event will be deleted by
 * this class after processing.
 *
 * @return  IPRT status code.
 * @param   pEvent                  Event to inject.
 */
int VBoxDnDWnd::ProcessEvent(PVBOXDNDEVENT pEvent)
{
    AssertPtrReturn(pEvent, VERR_INVALID_POINTER);

    BOOL fRc = ::PostMessage(hWnd, WM_VBOXTRAY_DND_MESSAGE,
                             0 /* wParm */, (LPARAM)pEvent /* lParm */);
    if (!fRc)
    {
        DWORD dwErr = GetLastError();

        static int s_iBitchedAboutFailedDnDMessages = 0;
        if (s_iBitchedAboutFailedDnDMessages++ < 32)
        {
            LogRel(("DnD: Processing event %p failed with %ld (%Rrc), skipping\n",
                    pEvent, dwErr, RTErrConvertFromWin32(dwErr)));
        }

        RTMemFree(pEvent);
        pEvent = NULL;

        return RTErrConvertFromWin32(dwErr);
    }

    return VINF_SUCCESS;
}

/**
 * Hides the proxy window again.
 *
 * @return  IPRT status code.
 */
int VBoxDnDWnd::hide(void)
{
#ifdef DEBUG_andy
    LogFlowFunc(("\n"));
#endif
    ShowWindow(hWnd, SW_HIDE);

    return VINF_SUCCESS;
}

/**
 * Shows the (invisible) proxy window in fullscreen,
 * spawned across all active guest monitors.
 *
 * @return  IPRT status code.
 */
int VBoxDnDWnd::makeFullscreen(void)
{
    int rc = VINF_SUCCESS;

    RECT r;
    RT_ZERO(r);

    BOOL fRc;
    HDC hDC = GetDC(NULL /* Entire screen */);
    if (hDC)
    {
        fRc = g_pfnEnumDisplayMonitors
            /* EnumDisplayMonitors is not available on NT4. */
            ? g_pfnEnumDisplayMonitors(hDC, NULL, VBoxDnDWnd::MonitorEnumProc, (LPARAM)&r):
              FALSE;

        if (!fRc)
            rc = VERR_NOT_FOUND;
        ReleaseDC(NULL, hDC);
    }
    else
        rc = VERR_ACCESS_DENIED;

    if (RT_FAILURE(rc))
    {
        /* If multi-monitor enumeration failed above, try getting at least the
         * primary monitor as a fallback. */
        r.left   = 0;
        r.top    = 0;
        r.right  = GetSystemMetrics(SM_CXSCREEN);
        r.bottom = GetSystemMetrics(SM_CYSCREEN);
        rc = VINF_SUCCESS;
    }

    if (RT_SUCCESS(rc))
    {
        LONG lStyle = GetWindowLong(hWnd, GWL_STYLE);
        SetWindowLong(hWnd, GWL_STYLE,
                      lStyle & ~(WS_CAPTION | WS_THICKFRAME));
        LONG lExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
        SetWindowLong(hWnd, GWL_EXSTYLE,
                      lExStyle & ~(  WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE
                                   | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));

        fRc = SetWindowPos(hWnd, HWND_TOPMOST,
                           r.left,
                           r.top,
                           r.right  - r.left,
                           r.bottom - r.top,
#ifdef VBOX_DND_DEBUG_WND
                           SWP_SHOWWINDOW | SWP_FRAMECHANGED);
#else
                           SWP_SHOWWINDOW | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_NOACTIVATE);
#endif
        if (fRc)
        {
            LogFlowFunc(("Virtual screen is %ld,%ld,%ld,%ld (%ld x %ld)\n",
                         r.left, r.top, r.right, r.bottom,
                         r.right - r.left, r.bottom - r.top));
        }
        else
        {
            DWORD dwErr = GetLastError();
            LogRel(("DnD: Failed to set proxy window position, rc=%Rrc\n",
                    RTErrConvertFromWin32(dwErr)));
        }
    }
    else
        LogRel(("DnD: Failed to determine virtual screen size, rc=%Rrc\n", rc));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Moves the guest mouse cursor to a specific position.
 *
 * @return  IPRT status code.
 * @param   x                       X position (in pixels) to move cursor to.
 * @param   y                       Y position (in pixels) to move cursor to.
 * @param   dwMouseInputFlags       Additional movement flags. @sa MOUSEEVENTF_ flags.
 */
int VBoxDnDWnd::mouseMove(int x, int y, DWORD dwMouseInputFlags)
{
    int iScreenX = GetSystemMetrics(SM_CXSCREEN) - 1;
    int iScreenY = GetSystemMetrics(SM_CYSCREEN) - 1;

    INPUT Input[1] = { 0 };
    Input[0].type       = INPUT_MOUSE;
    Input[0].mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE
                        | dwMouseInputFlags;
    Input[0].mi.dx      = x * (65535 / iScreenX);
    Input[0].mi.dy      = y * (65535 / iScreenY);

    int rc;
    if (g_pfnSendInput(1 /* Number of inputs */,
                       Input, sizeof(INPUT)))
    {
#ifdef DEBUG_andy
        CURSORINFO ci;
        RT_ZERO(ci);
        ci.cbSize = sizeof(ci);
        BOOL fRc = GetCursorInfo(&ci);
        if (fRc)
            LogFlowThisFunc(("Cursor shown=%RTbool, cursor=0x%p, x=%d, y=%d\n",
                             (ci.flags & CURSOR_SHOWING) ? true : false,
                             ci.hCursor, ci.ptScreenPos.x, ci.ptScreenPos.y));
#endif
        rc = VINF_SUCCESS;
    }
    else
    {
        DWORD dwErr = GetLastError();
        rc = RTErrConvertFromWin32(dwErr);
        LogFlowFunc(("SendInput failed with rc=%Rrc\n", rc));
    }

    return rc;
}

/**
 * Releases a previously pressed left guest mouse button.
 *
 * @return  IPRT status code.
 */
int VBoxDnDWnd::mouseRelease(void)
{
    LogFlowFuncEnter();

    int rc;

    /* Release mouse button in the guest to start the "drop"
     * action at the current mouse cursor position. */
    INPUT Input[1] = { 0 };
    Input[0].type       = INPUT_MOUSE;
    Input[0].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    if (!g_pfnSendInput(1, Input, sizeof(INPUT)))
    {
        DWORD dwErr = GetLastError();
        rc = RTErrConvertFromWin32(dwErr);
        LogFlowFunc(("SendInput failed with rc=%Rrc\n", rc));
    }
    else
        rc = VINF_SUCCESS;

    return rc;
}

/**
 * Resets the proxy window.
 */
void VBoxDnDWnd::reset(void)
{
    LogFlowThisFunc(("Resetting, old mMode=%ld, mState=%ld\n",
                     mMode, mState));

    /*
     * Note: Don't clear this->lstAllowedFormats at the moment, as this value is initialized
     *       on class creation. We might later want to modify the allowed formats at runtime,
     *       so keep this in mind when implementing this.
     */

    this->lstFmtActive.clear();
    this->uAllActions = DND_IGNORE_ACTION;

    int rc2 = setMode(Unknown);
    AssertRC(rc2);

    hide();
}

/**
 * Sets the current operation mode of this proxy window.
 *
 * @return  IPRT status code.
 * @param   enmMode                 New mode to set.
 */
int VBoxDnDWnd::setMode(Mode enmMode)
{
    LogFlowThisFunc(("Old mode=%ld, new mode=%ld\n",
                     mMode, enmMode));

    mMode = enmMode;
    mState = Initialized;

    return VINF_SUCCESS;
}

/**
 * Static helper function for having an own WndProc for proxy
 * window instances.
 */
static LRESULT CALLBACK vboxDnDWndProcInstance(HWND hWnd, UINT uMsg,
                                               WPARAM wParam, LPARAM lParam)
{
    LONG_PTR pUserData = GetWindowLongPtr(hWnd, GWLP_USERDATA);
    AssertPtrReturn(pUserData, 0);

    VBoxDnDWnd *pWnd = reinterpret_cast<VBoxDnDWnd *>(pUserData);
    if (pWnd)
        return pWnd->WndProc(hWnd, uMsg, wParam, lParam);

    return 0;
}

/**
 * Static helper function for routing Windows messages to a specific
 * proxy window instance.
 */
static LRESULT CALLBACK vboxDnDWndProc(HWND hWnd, UINT uMsg,
                                       WPARAM wParam, LPARAM lParam)
{
    /* Note: WM_NCCREATE is not the first ever message which arrives, but
     *       early enough for us. */
    if (uMsg == WM_NCCREATE)
    {
        LPCREATESTRUCT pCS = (LPCREATESTRUCT)lParam;
        AssertPtr(pCS);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pCS->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)vboxDnDWndProcInstance);

        return vboxDnDWndProcInstance(hWnd, uMsg, wParam, lParam);
    }

    /* No window associated yet. */
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/**
 * Initializes drag and drop.
 *
 * @return  IPRT status code.
 * @param   pEnv                        The DnD service's environment.
 * @param   ppInstance                  The instance pointer which refer to this object.
 */
DECLCALLBACK(int) VBoxDnDInit(const PVBOXSERVICEENV pEnv, void **ppInstance)
{
    AssertPtrReturn(pEnv, VERR_INVALID_POINTER);
    AssertPtrReturn(ppInstance, VERR_INVALID_POINTER);

    LogFlowFuncEnter();

    PVBOXDNDCONTEXT pCtx = &g_Ctx; /* Only one instance at the moment. */
    AssertPtr(pCtx);

    int rc;
    bool fSupportedOS = true;

    if (VbglR3AutoLogonIsRemoteSession())
    {
        /* Do not do drag and drop for remote sessions. */
        LogRel(("DnD: Drag and drop has been disabled for a remote session\n"));
        rc = VERR_NOT_SUPPORTED;
    }
    else
        rc = VINF_SUCCESS;

    if (RT_SUCCESS(rc))
    {
        g_pfnSendInput = (PFNSENDINPUT)
            RTLdrGetSystemSymbol("User32.dll", "SendInput");
        fSupportedOS = !RT_BOOL(g_pfnSendInput == NULL);
        g_pfnEnumDisplayMonitors = (PFNENUMDISPLAYMONITORS)
            RTLdrGetSystemSymbol("User32.dll", "EnumDisplayMonitors");
        /* g_pfnEnumDisplayMonitors is optional. */

        if (!fSupportedOS)
        {
            LogRel(("DnD: Not supported Windows version, disabling drag and drop support\n"));
            rc = VERR_NOT_SUPPORTED;
        }
    }

    if (RT_SUCCESS(rc))
    {
        /* Assign service environment to our context. */
        pCtx->pEnv = pEnv;

        /* Create the proxy window. At the moment we
         * only support one window at a time. */
        VBoxDnDWnd *pWnd = NULL;
        try
        {
            pWnd = new VBoxDnDWnd();
            rc = pWnd->Initialize(pCtx);

            /* Add proxy window to our proxy windows list. */
            if (RT_SUCCESS(rc))
                pCtx->lstWnd.append(pWnd);
        }
        catch (std::bad_alloc)
        {
            rc = VERR_NO_MEMORY;
        }
    }

    if (RT_SUCCESS(rc))
        rc = RTSemEventCreate(&pCtx->hEvtQueueSem);
    if (RT_SUCCESS(rc))
    {
        *ppInstance = pCtx;

        LogRel(("DnD: Drag and drop service successfully started\n"));
    }
    else
        LogRel(("DnD: Initializing drag and drop service failed with rc=%Rrc\n", rc));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

DECLCALLBACK(int) VBoxDnDStop(void *pInstance)
{
    AssertPtrReturn(pInstance, VERR_INVALID_POINTER);

    LogFunc(("Stopping pInstance=%p\n", pInstance));

    PVBOXDNDCONTEXT pCtx = (PVBOXDNDCONTEXT)pInstance;
    AssertPtr(pCtx);

    /* Set shutdown indicator. */
    ASMAtomicWriteBool(&pCtx->fShutdown, true);

    /* Disconnect. */
    VbglR3DnDDisconnect(&pCtx->cmdCtx);

    LogFlowFuncLeaveRC(VINF_SUCCESS);
    return VINF_SUCCESS;
}

DECLCALLBACK(void) VBoxDnDDestroy(void *pInstance)
{
    AssertPtrReturnVoid(pInstance);

    LogFunc(("Destroying pInstance=%p\n", pInstance));

    PVBOXDNDCONTEXT pCtx = (PVBOXDNDCONTEXT)pInstance;
    AssertPtr(pCtx);

    /** @todo At the moment we only have one DnD proxy window. */
    Assert(pCtx->lstWnd.size() == 1);
    VBoxDnDWnd *pWnd = pCtx->lstWnd.first();
    if (pWnd)
    {
        delete pWnd;
        pWnd = NULL;
    }

    if (pCtx->hEvtQueueSem != NIL_RTSEMEVENT)
    {
        RTSemEventDestroy(pCtx->hEvtQueueSem);
        pCtx->hEvtQueueSem = NIL_RTSEMEVENT;
    }

    LogFunc(("Destroyed pInstance=%p\n", pInstance));
}

DECLCALLBACK(int) VBoxDnDWorker(void *pInstance, bool volatile *pfShutdown)
{
    AssertPtr(pInstance);
    AssertPtr(pfShutdown);

    LogFlowFunc(("pInstance=%p\n", pInstance));

    /*
     * Tell the control thread that it can continue
     * spawning services.
     */
    RTThreadUserSignal(RTThreadSelf());

    PVBOXDNDCONTEXT pCtx = (PVBOXDNDCONTEXT)pInstance;
    AssertPtr(pCtx);

    int rc = VbglR3DnDConnect(&pCtx->cmdCtx);
    if (RT_FAILURE(rc))
        return rc;

    /** @todo At the moment we only have one DnD proxy window. */
    Assert(pCtx->lstWnd.size() == 1);
    VBoxDnDWnd *pWnd = pCtx->lstWnd.first();
    AssertPtr(pWnd);

    /* Number of invalid messages skipped in a row. */
    int cMsgSkippedInvalid = 0;
    PVBOXDNDEVENT pEvent = NULL;

    for (;;)
    {
        pEvent = (PVBOXDNDEVENT)RTMemAllocZ(sizeof(VBOXDNDEVENT));
        if (!pEvent)
        {
            rc = VERR_NO_MEMORY;
            break;
        }
        /* Note: pEvent will be free'd by the consumer later. */

        rc = VbglR3DnDRecvNextMsg(&pCtx->cmdCtx, &pEvent->Event);
        LogFlowFunc(("VbglR3DnDRecvNextMsg: uType=%RU32, rc=%Rrc\n", pEvent->Event.uType, rc));

        if (   RT_SUCCESS(rc)
            /* Cancelled from host. */
            || rc == VERR_CANCELLED
            )
        {
            cMsgSkippedInvalid = 0; /* Reset skipped messages count. */

            LogFlowFunc(("Received new event, type=%RU32, rc=%Rrc\n", pEvent->Event.uType, rc));

            rc = pWnd->ProcessEvent(pEvent);
            if (RT_SUCCESS(rc))
            {
                /* Event was consumed and the proxy window till take care of the memory -- NULL it. */
                pEvent = NULL;
            }
            else
                LogRel(("DnD: Processing proxy window event %RU32 on screen %RU32 failed with %Rrc\n",
                        pEvent->Event.uType, pEvent->Event.uScreenId, rc));
        }
        else if (rc == VERR_INTERRUPTED) /* Disconnected from service. */
        {
            LogRel(("DnD: Received quit message, shutting down ...\n"));
            pWnd->PostMessage(WM_QUIT, 0 /* wParm */, 0 /* lParm */);
            rc = VINF_SUCCESS;
        }

        if (RT_FAILURE(rc))
        {
            LogFlowFunc(("Processing next message failed with rc=%Rrc\n", rc));

            /* Old(er) hosts either are broken regarding DnD support or otherwise
             * don't support the stuff we do on the guest side, so make sure we
             * don't process invalid messages forever. */
            if (rc == VERR_INVALID_PARAMETER)
                cMsgSkippedInvalid++;
            if (cMsgSkippedInvalid > 32)
            {
                LogRel(("DnD: Too many invalid/skipped messages from host, exiting ...\n"));
                break;
            }

            /* Make sure our proxy window is hidden when an error occured to
             * not block the guest's UI. */
            pWnd->hide();

            int rc2 = VbglR3DnDGHSendError(&pCtx->cmdCtx, rc);
            if (RT_FAILURE(rc2))
            {
                /* Ignore the following errors reported back from the host. */
                if (   rc2 != VERR_NOT_SUPPORTED
                    && rc2 != VERR_NOT_IMPLEMENTED)
                {
                    LogRel(("DnD: Could not report error %Rrc back to host: %Rrc\n", rc, rc2));
                }
            }
        }

        if (*pfShutdown)
            break;

        if (ASMAtomicReadBool(&pCtx->fShutdown))
            break;

        if (RT_FAILURE(rc)) /* Don't hog the CPU on errors. */
            RTThreadSleep(1000 /* ms */);
    }

    if (pEvent)
    {
        RTMemFree(pEvent);
        pEvent = NULL;
    }

    VbglR3DnDDisconnect(&pCtx->cmdCtx);

    LogRel(("DnD: Ended\n"));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * The service description.
 */
VBOXSERVICEDESC g_SvcDescDnD =
{
    /* pszName. */
    "draganddrop",
    /* pszDescription. */
    "Drag and Drop",
    /* methods */
    VBoxDnDInit,
    VBoxDnDWorker,
    VBoxDnDStop,
    VBoxDnDDestroy
};

