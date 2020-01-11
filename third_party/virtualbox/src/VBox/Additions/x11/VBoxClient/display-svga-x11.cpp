/* $Id: display-svga-x11.cpp $ */
/** @file
 * X11 guest client - VMSVGA emulation resize event pass-through to X.Org
 * guest driver.
 */

/*
 * Copyright (C) 2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/*
 * Known things to test when changing this code.  All assume a guest with VMSVGA
 * active and controlled by X11 or Wayland, and Guest Additions installed and
 * running, unless otherwise stated.
 *  - On Linux 4.6 and later guests, VBoxClient --vmsvga should be running as
 *    root and not as the logged-in user.  Dynamic resizing should work for all
 *    screens in any environment which handles kernel resize notifications,
 *    including at log-in screens.  Test GNOME Shell Wayland and GNOME Shell
 *    under X.Org or Unity or KDE at the log-in screen and after log-in.
 *  - Linux 4.10 changed the user-kernel-ABI introduced in 4.6: test both.
 *  - On other guests (than Linux 4.6 or later) running X.Org Server 1.3 or
 *    later, VBoxClient --vmsvga should never be running as root, and should run
 *    (and dynamic resizing and screen enable/disable should work for all
 *    screens) whenever a user is logged in to a supported desktop environment.
 *  - On guests running X.Org Server 1.2 or older, VBoxClient --vmsvga should
 *    never run as root and should run whenever a user is logged in to a
 *    supported desktop environment.  Dynamic resizing should work for the first
 *    screen, and enabling others should not be possible.
 *  - When VMSVGA is not enabled, VBoxClient --vmsvga should never stay running.
 */

#include "VBoxClient.h"

#include <VBox/VBoxGuestLib.h>

#include <iprt/assert.h>
#include <iprt/file.h>
#include <iprt/string.h>

#include <sys/utsname.h>

/** Maximum number of supported screens.  DRM and X11 both limit this to 32. */
/** @todo if this ever changes, dynamically allocate resizeable arrays in the
 *  context structure. */
#define VMW_MAX_HEADS 32

/* VMWare X.Org driver control parts definitions. */

#include <X11/Xlibint.h>

static bool checkRecentLinuxKernel(void)
{
    struct utsname name;

    if (uname(&name) == -1)
        VBClFatalError(("Failed to get kernel name.\n"));
    if (strcmp(name.sysname, "Linux"))
        return false;
    return (RTStrVersionCompare(name.release, "4.6") >= 0);
}

struct X11CONTEXT
{
    Display *pDisplay;
    int hRandRMajor;
    int hVMWMajor;
};

static void x11Connect(struct X11CONTEXT *pContext)
{
    int dummy;

    if (pContext->pDisplay != NULL)
        VBClFatalError(("%s called with bad argument\n", __func__));
    pContext->pDisplay = XOpenDisplay(NULL);
    if (pContext->pDisplay == NULL)
        return;
    if (   !XQueryExtension(pContext->pDisplay, "RANDR",
                            &pContext->hRandRMajor, &dummy, &dummy)
        || !XQueryExtension(pContext->pDisplay, "VMWARE_CTRL",
                            &pContext->hVMWMajor, &dummy, &dummy))
    {
        XCloseDisplay(pContext->pDisplay);
        pContext->pDisplay = NULL;
    }
}

#define X11_VMW_TOPOLOGY_REQ 2
struct X11VMWRECT /* xXineramaScreenInfo in Xlib headers. */
{
    int16_t x;
    int16_t y;
    uint16_t w;
    uint16_t h;
};
AssertCompileSize(struct X11VMWRECT, 8);

struct X11REQHEADER
{
    uint8_t hMajor;
    uint8_t idType;
    uint16_t cd;
};

struct X11VMWTOPOLOGYREQ
{
    struct X11REQHEADER header;
    uint32_t idX11Screen;
    uint32_t cScreens;
    uint32_t u32Pad;
    struct X11VMWRECT aRects[1];
};
AssertCompileSize(struct X11VMWTOPOLOGYREQ, 24);

#define X11_VMW_TOPOLOGY_REPLY_SIZE 32

#define X11_VMW_RESOLUTION_REQUEST 1
struct X11VMWRESOLUTIONREQ
{
    struct X11REQHEADER header;
    uint32_t idX11Screen;
    uint32_t w;
    uint32_t h;
};
AssertCompileSize(struct X11VMWRESOLUTIONREQ, 16);

#define X11_VMW_RESOLUTION_REPLY_SIZE 32

#define X11_RANDR_GET_SCREEN_REQUEST 5
struct X11RANDRGETSCREENREQ
{
    struct X11REQHEADER header;
    uint32_t hWindow;
};
AssertCompileSize(struct X11RANDRGETSCREENREQ, 8);

#define X11_RANDR_GET_SCREEN_REPLY_SIZE 32

/* This was a macro in old Xlib versions and a function in newer ones; the
 * display members touched by the macro were declared as ABI for compatibility
 * reasons.  To simplify building with different generations, we duplicate the
 * code. */
static void x11GetRequest(struct X11CONTEXT *pContext, uint8_t hMajor,
                          uint8_t idType, size_t cb, struct X11REQHEADER **ppReq)
{
    if (pContext->pDisplay->bufptr + cb > pContext->pDisplay->bufmax)
        _XFlush(pContext->pDisplay);
    if (pContext->pDisplay->bufptr + cb > pContext->pDisplay->bufmax)
        VBClFatalError(("%s display buffer overflow.\n", __func__));
    if (cb % 4 != 0)
        VBClFatalError(("%s bad parameter.\n", __func__));
    pContext->pDisplay->last_req = pContext->pDisplay->bufptr;
    *ppReq = (struct X11REQHEADER *)pContext->pDisplay->bufptr;
    (*ppReq)->hMajor = hMajor;
    (*ppReq)->idType = idType;
    (*ppReq)->cd = cb / 4;
    pContext->pDisplay->bufptr += cb;
    pContext->pDisplay->request++;
}

static void x11SendHints(struct X11CONTEXT *pContext, struct X11VMWRECT *pRects,
                         unsigned cRects)
{
    unsigned i;
    struct X11VMWTOPOLOGYREQ     *pReqTopology;
    uint8_t                       repTopology[X11_VMW_TOPOLOGY_REPLY_SIZE];
    struct X11VMWRESOLUTIONREQ   *pReqResolution;
    uint8_t                       repResolution[X11_VMW_RESOLUTION_REPLY_SIZE];

    if (!VALID_PTR(pContext->pDisplay))
        VBClFatalError(("%s bad display argument.\n", __func__));
    if (cRects == 0)
        return;
    /* Try a topology (multiple screen) request. */
    x11GetRequest(pContext, pContext->hVMWMajor, X11_VMW_TOPOLOGY_REQ,
                    sizeof(struct X11VMWTOPOLOGYREQ)
                  + sizeof(struct X11VMWRECT) * (cRects - 1),
                  (struct X11REQHEADER **)&pReqTopology);
    pReqTopology->idX11Screen = DefaultScreen(pContext->pDisplay);
    pReqTopology->cScreens = cRects;
    for (i = 0; i < cRects; ++i)
        pReqTopology->aRects[i] = pRects[i];
    _XSend(pContext->pDisplay, NULL, 0);
    if (_XReply(pContext->pDisplay, (xReply *)&repTopology, 0, xTrue))
        return;
    /* That failed, so try the old single screen set resolution.  We prefer
     * simpler code to negligeably improved efficiency, so we just always try
     * both requests instead of doing version checks or caching. */
    x11GetRequest(pContext, pContext->hVMWMajor, X11_VMW_RESOLUTION_REQUEST,
                  sizeof(struct X11VMWRESOLUTIONREQ),
                  (struct X11REQHEADER **)&pReqResolution);
    pReqResolution->idX11Screen = DefaultScreen(pContext->pDisplay);
    pReqResolution->w = pRects[0].w;
    pReqResolution->h = pRects[0].h;
    if (_XReply(pContext->pDisplay, (xReply *)&repResolution, 0, xTrue))
        return;
    /* What now? */
    VBClFatalError(("%s failed to set resolution\n", __func__));
}

/** Call RRGetScreenInfo to wake up the server to the new modes. */
static void x11GetScreenInfo(struct X11CONTEXT *pContext)
{
    struct X11RANDRGETSCREENREQ *pReqGetScreen;
    uint8_t                      repGetScreen[X11_RANDR_GET_SCREEN_REPLY_SIZE];

    if (!VALID_PTR(pContext->pDisplay))
        VBClFatalError(("%s bad display argument.\n", __func__));
    x11GetRequest(pContext, pContext->hRandRMajor, X11_RANDR_GET_SCREEN_REQUEST,
                    sizeof(struct X11RANDRGETSCREENREQ),
                  (struct X11REQHEADER **)&pReqGetScreen);
    pReqGetScreen->hWindow = DefaultRootWindow(pContext->pDisplay);
    _XSend(pContext->pDisplay, NULL, 0);
    if (!_XReply(pContext->pDisplay, (xReply *)&repGetScreen, 0, xTrue))
        VBClFatalError(("%s failed to set resolution\n", __func__));
}

static const char *getPidFilePath()
{
    return ".vboxclient-display-svga-x11.pid";
}

static int run(struct VBCLSERVICE **ppInterface, bool fDaemonised)
{
    (void)ppInterface;
    (void)fDaemonised;
    struct X11CONTEXT x11Context = { NULL };
    unsigned i;
    int rc;
    uint32_t acx[VMW_MAX_HEADS] = { 0 };
    uint32_t acy[VMW_MAX_HEADS] = { 0 };
    uint32_t adx[VMW_MAX_HEADS] = { 0 };
    uint32_t ady[VMW_MAX_HEADS] = { 0 };
    uint32_t afEnabled[VMW_MAX_HEADS] = { false };
    struct X11VMWRECT aRects[VMW_MAX_HEADS];
    unsigned cHeads;

    if (checkRecentLinuxKernel())
        return VINF_SUCCESS;
    x11Connect(&x11Context);
    if (x11Context.pDisplay == NULL)
        return VINF_SUCCESS;
    /* Initialise the guest library. */
    rc = VbglR3InitUser();
    if (RT_FAILURE(rc))
        VBClFatalError(("Failed to connect to the VirtualBox kernel service, rc=%Rrc\n", rc));
    rc = VbglR3CtlFilterMask(VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST, 0);
    if (RT_FAILURE(rc))
        VBClFatalError(("Failed to request display change events, rc=%Rrc\n", rc));
    rc = VbglR3AcquireGuestCaps(VMMDEV_GUEST_SUPPORTS_GRAPHICS, 0, false);
    if (rc == VERR_RESOURCE_BUSY)  /* Someone else has already acquired it. */
        return VINF_SUCCESS;
    if (RT_FAILURE(rc))
        VBClFatalError(("Failed to register resizing support, rc=%Rrc\n", rc));
    for (;;)
    {
        uint32_t events;

        rc = VbglR3WaitEvent(VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST, RT_INDEFINITE_WAIT, &events);
        if (RT_FAILURE(rc))
            VBClFatalError(("Failure waiting for event, rc=%Rrc\n", rc));
        while (rc != VERR_TIMEOUT)
        {
            uint32_t cx, cy, cBits, dx, dy, idx;
            bool fEnabled, fChangeOrigin;

            rc = VbglR3GetDisplayChangeRequest(&cx, &cy, &cBits, &idx, &dx, &dy, &fEnabled, &fChangeOrigin, true);
            if (RT_FAILURE(rc))
                VBClFatalError(("Failed to get display change request, rc=%Rrc\n", rc));
            if (idx < VMW_MAX_HEADS)
            {
                acx[idx] = cx;
                acy[idx] = cy;
                if (fChangeOrigin)
                    adx[idx] = dx < INT32_MAX ? dx : 0;
                if (fChangeOrigin)
                    ady[idx] = dy < INT32_MAX ? dy : 0;
                afEnabled[idx] = fEnabled;
            }
            rc = VbglR3WaitEvent(VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST, 0, &events);
            if (RT_FAILURE(rc) && rc != VERR_TIMEOUT && rc != VERR_INTERRUPTED)
                VBClFatalError(("Failure waiting for event, rc=%Rrc\n", rc));
        }
        for (i = 0, cHeads = 0; i < VMW_MAX_HEADS; ++i)
        {
            if (afEnabled[i])
            {
                aRects[cHeads].x = (int16_t)adx[i];
                aRects[cHeads].y = (int16_t)ady[i];
                aRects[cHeads].w = (uint16_t)acx[i];
                aRects[cHeads].h = (uint16_t)acy[i];
                ++cHeads;
            }
        }
        x11SendHints(&x11Context, aRects, cHeads);
        x11GetScreenInfo(&x11Context);
    }
}

static struct VBCLSERVICE interface =
{
    getPidFilePath,
    VBClServiceDefaultHandler, /* Init */
    run,
    VBClServiceDefaultCleanup
}, *pInterface = &interface;

struct VBCLSERVICE **VBClDisplaySVGAX11Service()
{
    return &pInterface;
}
