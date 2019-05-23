/* $Id: check3d.cpp $ */
/** @file
 * X11 guest client - 3D pass-through check.
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

#include "VBoxClient.h"

#include <VBox/VBoxGuest.h>
#include <VBox/VBoxGuestLib.h>
#include <VBox/HostServices/VBoxCrOpenGLSvc.h>

#include <stdlib.h>

#define CR_VBOX_CAP_HOST_CAPS_NOT_SUFFICIENT 0x00000020

static int run(struct VBCLSERVICE **ppInterface, bool fDaemonised)
{
    int rc;
    HGCMCLIENTID idClient;
    CRVBOXHGCMGETCAPS caps;
    LogFlowFunc(("\n"));

    NOREF(ppInterface);
    NOREF(fDaemonised);
    /* Initialise the guest library. */
    rc = VbglR3InitUser();
    if (RT_FAILURE(rc))
        exit(1);
    rc = VbglR3HGCMConnect("VBoxSharedCrOpenGL", &idClient);
    if (RT_FAILURE(rc))
        exit(1);

    VBGL_HGCM_HDR_INIT(&caps.hdr, idClient, SHCRGL_GUEST_FN_GET_CAPS_LEGACY, SHCRGL_CPARMS_GET_CAPS_LEGACY);
    caps.Caps.type       = VMMDevHGCMParmType_32bit;
    caps.Caps.u.value32  = 0;
    rc = VbglR3HGCMCall(&caps.hdr, sizeof(caps));
    if (RT_FAILURE(rc))
        exit(1);
    if (caps.Caps.u.value32 & CR_VBOX_CAP_HOST_CAPS_NOT_SUFFICIENT)
        exit(1);
    VbglR3HGCMDisconnect(idClient);
    VbglR3Term();
    LogFlowFunc(("returning %Rrc\n", rc));
    return rc;
}

static struct VBCLSERVICE g_vbclCheck3DInterface =
{
    NULL,
    VBClServiceDefaultHandler, /* init */
    run,
    VBClServiceDefaultCleanup
};
static struct VBCLSERVICE *g_pvbclCheck3DInterface = &g_vbclCheck3DInterface;

/* Static factory */
struct VBCLSERVICE **VBClCheck3DService()
{
    return &g_pvbclCheck3DInterface;
}

