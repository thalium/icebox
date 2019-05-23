/* $Id: VBoxGuestR3LibMouse.cpp $ */
/** @file
 * VBoxGuestR3Lib - Ring-3 Support Library for VirtualBox guest additions, Mouse.
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
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
#include "VBoxGuestR3LibInternal.h"


/**
 * Retrieve mouse coordinates and features from the host.
 *
 * @returns VBox status code.
 *
 * @param   pfFeatures  Where to store the mouse features.
 * @param   px          Where to store the X co-ordinate.
 * @param   py          Where to store the Y co-ordinate.
 */
VBGLR3DECL(int) VbglR3GetMouseStatus(uint32_t *pfFeatures, uint32_t *px, uint32_t *py)
{
    VMMDevReqMouseStatus Req;
    vmmdevInitRequest(&Req.header, VMMDevReq_GetMouseStatus);
    Req.mouseFeatures = 0;
    Req.pointerXPos = 0;
    Req.pointerYPos = 0;
    int rc = vbglR3GRPerform(&Req.header);
    if (RT_SUCCESS(rc))
    {
        if (pfFeatures)
            *pfFeatures = Req.mouseFeatures;
        if (px)
            *px = Req.pointerXPos;
        if (py)
            *py = Req.pointerYPos;
    }
    return rc;
}


/**
 * Send mouse features to the host.
 *
 * @returns VBox status code.
 *
 * @param   fFeatures  Supported mouse pointer features.  The main guest driver
 *                     will mediate different callers and show the host any
 *                     feature enabled by any guest caller.
 */
VBGLR3DECL(int) VbglR3SetMouseStatus(uint32_t fFeatures)
{
    VBGLIOCSETMOUSESTATUS Req;
    VBGLREQHDR_INIT(&Req.Hdr, SET_MOUSE_STATUS);
    Req.u.In.fStatus = fFeatures;
    return vbglR3DoIOCtl(VBGL_IOCTL_SET_MOUSE_STATUS, &Req.Hdr, sizeof(Req));
}

