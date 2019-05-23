/* $Id: seamless-x11.cpp $ */
/** @file
 * X11 Seamless mode.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
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
*   Header files                                                                                                                 *
*********************************************************************************************************************************/

#include <iprt/err.h>
#include <iprt/assert.h>
#include <iprt/vector.h>
#include <VBox/log.h>

#include "seamless-x11.h"
#include "VBoxClient.h"

#include <X11/Xatom.h>
#include <X11/Xmu/WinUtil.h>

#include <limits.h>

#ifdef TESTCASE
#undef DefaultRootWindow
#define DefaultRootWindow XDefaultRootWindow
#endif

/*****************************************************************************
* Static functions                                                           *
*****************************************************************************/

static unsigned char *XXGetProperty (Display *aDpy, Window aWnd, Atom aPropType,
                                    const char *aPropName, unsigned long *nItems)
{
    LogRelFlowFunc(("\n"));
    Atom propNameAtom = XInternAtom (aDpy, aPropName,
                                     True /* only_if_exists */);
    if (propNameAtom == None)
    {
        return NULL;
    }

    Atom actTypeAtom = None;
    int actFmt = 0;
    unsigned long nBytesAfter = 0;
    unsigned char *propVal = 0;
    int rc = XGetWindowProperty (aDpy, aWnd, propNameAtom,
                                 0, LONG_MAX, False /* delete */,
                                 aPropType, &actTypeAtom, &actFmt,
                                 nItems, &nBytesAfter, &propVal);
    if (rc != Success)
        return NULL;

    LogRelFlowFunc(("returning\n"));
    return propVal;
}

/**
  * Initialise the guest and ensure that it is capable of handling seamless mode
  *
  * @param  pHostCallback   host callback.
  * @returns true if it can handle seamless, false otherwise
  */
int SeamlessX11::init(PFNSENDREGIONUPDATE pHostCallback)
{
    int rc = VINF_SUCCESS;

    LogRelFlowFunc(("\n"));
    if (mHostCallback != NULL)  /* Assertion */
    {
        LogRel(("VBoxClient: ERROR: attempt to initialise seamless guest object twice!\n"));
        return VERR_INTERNAL_ERROR;
    }
    if (!(mDisplay = XOpenDisplay(NULL)))
    {
        LogRel(("VBoxClient: seamless guest object failed to acquire a connection to the display.\n"));
        return VERR_ACCESS_DENIED;
    }
    mHostCallback = pHostCallback;
    mEnabled = false;
    unmonitorClientList();
    LogRelFlowFunc(("returning %Rrc\n", rc));
    return rc;
}

/**
 * Read information about currently visible windows in the guest and subscribe to X11
 * events about changes to this information.
 *
 * @note This class does not contain its own event thread, so an external thread must
 *       call nextConfigurationEvent() for as long as events are wished.
 * @todo This function should switch the guest to fullscreen mode.
 */
int SeamlessX11::start(void)
{
    int rc = VINF_SUCCESS;
    /** Dummy values for XShapeQueryExtension */
    int error, event;

    LogRelFlowFunc(("\n"));
    if (mEnabled)
        return VINF_SUCCESS;
    mSupportsShape = XShapeQueryExtension(mDisplay, &event, &error);
    mEnabled = true;
    monitorClientList();
    rebuildWindowTree();
    LogRelFlowFunc(("returning %Rrc\n", rc));
    return rc;
}

/** Stop reporting seamless events to the host.  Free information about guest windows
    and stop requesting updates. */
void SeamlessX11::stop(void)
{
    LogRelFlowFunc(("\n"));
    if (!mEnabled)
        return;
    mEnabled = false;
    unmonitorClientList();
    freeWindowTree();
    LogRelFlowFunc(("returning\n"));
}

void SeamlessX11::monitorClientList(void)
{
    LogRelFlowFunc(("called\n"));
    XSelectInput(mDisplay, DefaultRootWindow(mDisplay), PropertyChangeMask | SubstructureNotifyMask);
}

void SeamlessX11::unmonitorClientList(void)
{
    LogRelFlowFunc(("called\n"));
    XSelectInput(mDisplay, DefaultRootWindow(mDisplay), PropertyChangeMask);
}

/**
 * Recreate the table of toplevel windows of clients on the default root window of the
 * X server.
 */
void SeamlessX11::rebuildWindowTree(void)
{
    LogRelFlowFunc(("called\n"));
    freeWindowTree();
    addClients(DefaultRootWindow(mDisplay));
    mChanged = true;
}


/**
 * Look at the list of children of a virtual root window and add them to the list of clients
 * if they belong to a client which is not a virtual root.
 *
 * @param hRoot the virtual root window to be examined
 */
void SeamlessX11::addClients(const Window hRoot)
{
    /** Unused out parameters of XQueryTree */
    Window hRealRoot, hParent;
    /** The list of children of the root supplied, raw pointer */
    Window *phChildrenRaw = NULL;
    /** The list of children of the root supplied, auto-pointer */
    Window *phChildren;
    /** The number of children of the root supplied */
    unsigned cChildren;

    LogRelFlowFunc(("\n"));
    if (!XQueryTree(mDisplay, hRoot, &hRealRoot, &hParent, &phChildrenRaw, &cChildren))
        return;
    phChildren = phChildrenRaw;
    for (unsigned i = 0; i < cChildren; ++i)
        addClientWindow(phChildren[i]);
    XFree(phChildrenRaw);
    LogRelFlowFunc(("returning\n"));
}


void SeamlessX11::addClientWindow(const Window hWin)
{
    LogRelFlowFunc(("\n"));
    XWindowAttributes winAttrib;
    bool fAddWin = true;
    Window hClient = XmuClientWindow(mDisplay, hWin);

    if (isVirtualRoot(hClient))
        fAddWin = false;
    if (fAddWin && !XGetWindowAttributes(mDisplay, hWin, &winAttrib))
    {
        LogRelFunc(("VBoxClient: Failed to get the window attributes for window %d\n", hWin));
        fAddWin = false;
    }
    if (fAddWin && (winAttrib.map_state == IsUnmapped))
        fAddWin = false;
    XSizeHints dummyHints;
    long dummyLong;
    /* Apparently (?) some old kwin versions had unwanted client windows
     * without normal hints. */
    if (fAddWin && (!XGetWMNormalHints(mDisplay, hClient, &dummyHints,
                                       &dummyLong)))
    {
        LogRelFlowFunc(("window %lu, client window %lu has no size hints\n",
                     hWin, hClient));
        fAddWin = false;
    }
    if (fAddWin)
    {
        XRectangle *pRects = NULL;
        int cRects = 0, iOrdering;
        bool hasShape = false;

        LogRelFlowFunc(("adding window %lu, client window %lu\n", hWin,
                     hClient));
        if (mSupportsShape)
        {
            XShapeSelectInput(mDisplay, hWin, ShapeNotifyMask);
            pRects = XShapeGetRectangles(mDisplay, hWin, ShapeBounding, &cRects, &iOrdering);
            if (!pRects)
                cRects = 0;
            else
            {
                if (   (cRects > 1)
                    || (pRects[0].x != 0)
                    || (pRects[0].y != 0)
                    || (pRects[0].width != winAttrib.width)
                    || (pRects[0].height != winAttrib.height)
                   )
                    hasShape = true;
            }
        }
        mGuestWindows.addWindow(hWin, hasShape, winAttrib.x, winAttrib.y,
                                winAttrib.width, winAttrib.height, cRects,
                                pRects);
    }
    LogRelFlowFunc(("returning\n"));
}


/**
 * Checks whether a window is a virtual root.
 * @returns true if it is, false otherwise
 * @param hWin the window to be examined
 */
bool SeamlessX11::isVirtualRoot(Window hWin)
{
    unsigned char *windowTypeRaw = NULL;
    Atom *windowType;
    unsigned long ulCount;
    bool rc = false;

    LogRelFlowFunc(("\n"));
    windowTypeRaw = XXGetProperty(mDisplay, hWin, XA_ATOM, WM_TYPE_PROP, &ulCount);
    if (windowTypeRaw != NULL)
    {
        windowType = (Atom *)(windowTypeRaw);
        if (   (ulCount != 0)
            && (*windowType == XInternAtom(mDisplay, WM_TYPE_DESKTOP_PROP, True)))
            rc = true;
    }
    if (windowTypeRaw)
        XFree(windowTypeRaw);
    LogRelFlowFunc(("returning %RTbool\n", rc));
    return rc;
}

DECLCALLBACK(int) VBoxGuestWinFree(VBoxGuestWinInfo *pInfo, void *pvParam)
{
    Display *pDisplay = (Display *)pvParam;

    XShapeSelectInput(pDisplay, pInfo->Core.Key, 0);
    delete pInfo;
    return VINF_SUCCESS;
}

/**
 * Free all information in the tree of visible windows
 */
void SeamlessX11::freeWindowTree(void)
{
    /* We use post-increment in the operation to prevent the iterator from being invalidated. */
    LogRelFlowFunc(("\n"));
    mGuestWindows.detachAll(VBoxGuestWinFree, mDisplay);
    LogRelFlowFunc(("returning\n"));
}


/**
 * Waits for a position or shape-related event from guest windows
 *
 * @note Called from the guest event thread.
 */
void SeamlessX11::nextConfigurationEvent(void)
{
    XEvent event;

    LogRelFlowFunc(("\n"));
    /* Start by sending information about the current window setup to the host.  We do this
       here because we want to send all such information from a single thread. */
    if (mChanged && mEnabled)
    {
        updateRects();
        mHostCallback(mpRects, mcRects);
    }
    mChanged = false;
    /* We execute this even when seamless is disabled, as it also waits for
     * enable and disable notification. */
    XNextEvent(mDisplay, &event);
    if (!mEnabled)
        return;
    switch (event.type)
    {
    case ConfigureNotify:
        {
            XConfigureEvent *pConf = &event.xconfigure;
            LogRelFlowFunc(("configure event, window=%lu, x=%i, y=%i, w=%i, h=%i, send_event=%RTbool\n",
                           (unsigned long) pConf->window, (int) pConf->x,
                           (int) pConf->y, (int) pConf->width,
                           (int) pConf->height, pConf->send_event));
        }
        doConfigureEvent(event.xconfigure.window);
        break;
    case MapNotify:
        LogRelFlowFunc(("map event, window=%lu, send_event=%RTbool\n",
                       (unsigned long) event.xmap.window,
                       event.xmap.send_event));
        rebuildWindowTree();
        break;
    case PropertyNotify:
        if (   event.xproperty.atom != XInternAtom(mDisplay, "_NET_CLIENT_LIST", True /* only_if_exists */)
            || event.xproperty.window != DefaultRootWindow(mDisplay))
            break;
        LogRelFlowFunc(("_NET_CLIENT_LIST property event on root window.\n"));
        rebuildWindowTree();
        break;
    case VBoxShapeNotify:  /* This is defined wrong in my X11 header files! */
        LogRelFlowFunc(("shape event, window=%lu, send_event=%RTbool\n",
                       (unsigned long) event.xany.window,
                       event.xany.send_event));
    /* the window member in xany is in the same place as in the shape event */
        doShapeEvent(event.xany.window);
        break;
    case UnmapNotify:
        LogRelFlowFunc(("unmap event, window=%lu, send_event=%RTbool\n",
                       (unsigned long) event.xunmap.window,
                       event.xunmap.send_event));
        rebuildWindowTree();
        break;
    default:
        break;
    }
    LogRelFlowFunc(("processed event\n"));
}

/**
 * Handle a configuration event in the seamless event thread by setting the new position.
 *
 * @param hWin the window to be examined
 */
void SeamlessX11::doConfigureEvent(Window hWin)
{
    VBoxGuestWinInfo *pInfo = mGuestWindows.find(hWin);
    if (pInfo)
    {
        XWindowAttributes winAttrib;

        if (!XGetWindowAttributes(mDisplay, hWin, &winAttrib))
            return;
        pInfo->mX = winAttrib.x;
        pInfo->mY = winAttrib.y;
        pInfo->mWidth = winAttrib.width;
        pInfo->mHeight = winAttrib.height;
        mChanged = true;
    }
}

/**
 * Handle a window shape change event in the seamless event thread.
 *
 * @param hWin the window to be examined
 */
void SeamlessX11::doShapeEvent(Window hWin)
{
    LogRelFlowFunc(("\n"));
    VBoxGuestWinInfo *pInfo = mGuestWindows.find(hWin);
    if (pInfo)
    {
        XRectangle *pRects;
        int cRects = 0, iOrdering;

        pRects = XShapeGetRectangles(mDisplay, hWin, ShapeBounding, &cRects,
                                     &iOrdering);
        if (!pRects)
            cRects = 0;
        pInfo->mhasShape = true;
        if (pInfo->mpRects)
            XFree(pInfo->mpRects);
        pInfo->mcRects = cRects;
        pInfo->mpRects = pRects;
        mChanged = true;
    }
    LogRelFlowFunc(("returning\n"));
}

/**
 * Gets the list of visible rectangles
 */
RTRECT *SeamlessX11::getRects(void)
{
    return mpRects;
}

/**
 * Gets the number of rectangles in the visible rectangle list
 */
size_t SeamlessX11::getRectCount(void)
{
    return mcRects;
}

RTVEC_DECL(RectList, RTRECT)

DECLCALLBACK(int) getRectsCallback(VBoxGuestWinInfo *pInfo,
                                   struct RectList *pRects)
{
    if (pInfo->mhasShape)
    {
        for (int i = 0; i < pInfo->mcRects; ++i)
        {
            RTRECT *pRect;

            pRect = RectListPushBack(pRects);
            if (!pRect)
                return VERR_NO_MEMORY;
            pRect->xLeft   =   pInfo->mX
                             + pInfo->mpRects[i].x;
            pRect->yBottom =   pInfo->mY
                             + pInfo->mpRects[i].y
                             + pInfo->mpRects[i].height;
            pRect->xRight  =   pInfo->mX
                             + pInfo->mpRects[i].x
                             + pInfo->mpRects[i].width;
            pRect->yTop    =   pInfo->mY
                             + pInfo->mpRects[i].y;
        }
    }
    else
    {
        RTRECT *pRect;

        pRect = RectListPushBack(pRects);
        if (!pRect)
            return VERR_NO_MEMORY;
        pRect->xLeft   =  pInfo->mX;
        pRect->yBottom =  pInfo->mY
                        + pInfo->mHeight;
        pRect->xRight  =  pInfo->mX
                        + pInfo->mWidth;
        pRect->yTop    =  pInfo->mY;
    }
    return VINF_SUCCESS;
}

/**
 * Updates the list of seamless rectangles
 */
int SeamlessX11::updateRects(void)
{
    LogRelFlowFunc(("\n"));
    struct RectList rects = RTVEC_INITIALIZER;

    if (mcRects != 0)
    {
        int rc = RectListReserve(&rects, mcRects * 2);
        if (RT_FAILURE(rc))
            return rc;
    }
    mGuestWindows.doWithAll((PVBOXGUESTWINCALLBACK)getRectsCallback,
                            &rects);
    if (mpRects)
        RTMemFree(mpRects);
    mcRects = RectListSize(&rects);
    mpRects = RectListDetach(&rects);
    LogRelFlowFunc(("returning\n"));
    return VINF_SUCCESS;
}

/**
 * Send a client event to wake up the X11 seamless event loop prior to stopping it.
 *
 * @note This function should only be called from the host event thread.
 */
bool SeamlessX11::interruptEventWait(void)
{
    bool rc = false;
    Display *pDisplay = XOpenDisplay(NULL);

    LogRelFlowFunc(("\n"));
    if (pDisplay == NULL)
        VBClFatalError(("Failed to open X11 display.\n"));
    /* Message contents set to zero. */
    XClientMessageEvent clientMessage = { ClientMessage, 0, 0, 0, 0, 0, 8 };

    if (XSendEvent(pDisplay, DefaultRootWindow(mDisplay), false,
                   PropertyChangeMask, (XEvent *)&clientMessage))
        rc = true;
    XCloseDisplay(pDisplay);
    LogRelFlowFunc(("returning %RTbool\n", rc));
    return rc;
}
