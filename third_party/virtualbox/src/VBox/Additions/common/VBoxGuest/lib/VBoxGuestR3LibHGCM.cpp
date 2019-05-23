/* $Id: VBoxGuestR3LibHGCM.cpp $ */
/** @file
 * VBoxGuestR3Lib - Ring-3 Support Library for VirtualBox guest additions,
 * generic HGCM.
 */

/*
 * Copyright (C) 2015-2017 Oracle Corporation
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
#include <VBox/VBoxGuestLib.h>
#include <iprt/string.h>


/**
 * Connects to an HGCM service.
 *
 * @returns VBox status code
 * @param   pszServiceName  Name of the host service.
 * @param   pidClient       Where to put the client ID on success. The client ID
 *                          must be passed to all the other calls to the service.
 */
VBGLR3DECL(int) VbglR3HGCMConnect(const char *pszServiceName, HGCMCLIENTID *pidClient)
{
    VBGLIOCHGCMCONNECT Info;
    RT_ZERO(Info);
    VBGLREQHDR_INIT(&Info.Hdr, HGCM_CONNECT);
    Info.u.In.Loc.type = VMMDevHGCMLoc_LocalHost_Existing;
    strcpy(Info.u.In.Loc.u.host.achName, pszServiceName);

    int rc = vbglR3DoIOCtl(VBGL_IOCTL_HGCM_CONNECT, &Info.Hdr, sizeof(Info));
    if (RT_SUCCESS(rc))
        *pidClient = Info.u.Out.idClient;
    return rc;
}


/**
 * Disconnect from an HGCM service.
 *
 * @returns VBox status code.
 * @param   idClient        The client id returned by VbglR3InfoSvcConnect().
 */
VBGLR3DECL(int) VbglR3HGCMDisconnect(HGCMCLIENTID idClient)
{
    VBGLIOCHGCMDISCONNECT Info;
    VBGLREQHDR_INIT(&Info.Hdr, HGCM_DISCONNECT);
    Info.u.In.idClient = idClient;

    return vbglR3DoIOCtl(VBGL_IOCTL_HGCM_DISCONNECT, &Info.Hdr, sizeof(Info));
}


/**
 * Makes a fully prepared HGCM call.
 *
 * @returns VBox status code.
 * @param   pInfo           Fully prepared HGCM call info.
 * @param   cbInfo          Size of the info.  This may sometimes be larger than
 *                          what the parameter count indicates because of
 *                          parameter changes between versions and such.
 */
VBGLR3DECL(int) VbglR3HGCMCall(PVBGLIOCHGCMCALL pInfo, size_t cbInfo)
{
    /* Expect caller to have filled in pInfo. */
    AssertMsg(pInfo->Hdr.cbIn  == cbInfo, ("cbIn=%#x cbInfo=%#zx\n", pInfo->Hdr.cbIn, cbInfo));
    AssertMsg(pInfo->Hdr.cbOut == cbInfo, ("cbOut=%#x cbInfo=%#zx\n", pInfo->Hdr.cbOut, cbInfo));
    Assert(sizeof(*pInfo) + pInfo->cParms * sizeof(HGCMFunctionParameter) <= cbInfo);
    Assert(pInfo->u32ClientID != 0);

    return vbglR3DoIOCtl(VBGL_IOCTL_HGCM_CALL(cbInfo), &pInfo->Hdr, cbInfo);
}

