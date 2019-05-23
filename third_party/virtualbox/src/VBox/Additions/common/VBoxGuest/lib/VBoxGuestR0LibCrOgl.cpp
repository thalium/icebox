/* $Id: VBoxGuestR0LibCrOgl.cpp $ */
/** @file
 * VBoxGuestLib - Ring-3 Support Library for VirtualBox guest additions, Chromium OpenGL Service.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
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
#include <iprt/string.h>
#include "VBoxGuestR0LibInternal.h"

#ifdef VBGL_VBOXGUEST
# error "This file shouldn't be part of the VBoxGuestR0LibBase library that is linked into VBoxGuest.  It's client code."
#endif


DECLR0VBGL(int) VbglR0CrCtlCreate(VBGLCRCTLHANDLE *phCtl)
{
    int rc;

    if (phCtl)
    {
        struct VBGLHGCMHANDLEDATA *pHandleData = vbglR0HGCMHandleAlloc();
        if (pHandleData)
        {
            rc = VbglR0IdcOpen(&pHandleData->IdcHandle,
                               VBGL_IOC_VERSION /*uReqVersion*/,
                               VBGL_IOC_VERSION & UINT32_C(0xffff0000) /*uMinVersion*/,
                               NULL /*puSessionVersion*/, NULL /*puDriverVersion*/, NULL /*uDriverRevision*/);
            if (RT_SUCCESS(rc))
            {
                *phCtl = pHandleData;
                return VINF_SUCCESS;
            }

            vbglR0HGCMHandleFree(pHandleData);
        }
        else
            rc = VERR_NO_MEMORY;

        *phCtl = NULL;
    }
    else
        rc = VERR_INVALID_PARAMETER;

    return rc;
}

DECLR0VBGL(int) VbglR0CrCtlDestroy(VBGLCRCTLHANDLE hCtl)
{
    VbglR0IdcClose(&hCtl->IdcHandle);

    vbglR0HGCMHandleFree(hCtl);

    return VINF_SUCCESS;
}

DECLR0VBGL(int) VbglR0CrCtlConConnect(VBGLCRCTLHANDLE hCtl, HGCMCLIENTID *pidClient)
{
    VBGLIOCHGCMCONNECT info;
    int rc;

    if (!hCtl || !pidClient)
        return VERR_INVALID_PARAMETER;

    RT_ZERO(info);
    VBGLREQHDR_INIT(&info.Hdr, HGCM_CONNECT);
    info.u.In.Loc.type = VMMDevHGCMLoc_LocalHost_Existing;
    RTStrCopy(info.u.In.Loc.u.host.achName, sizeof(info.u.In.Loc.u.host.achName), "VBoxSharedCrOpenGL");
    rc = VbglR0IdcCall(&hCtl->IdcHandle, VBGL_IOCTL_HGCM_CONNECT, &info.Hdr, sizeof(info));
    if (RT_SUCCESS(rc))
    {
        Assert(info.u.Out.idClient);
        *pidClient = info.u.Out.idClient;
        return rc;
    }

    AssertRC(rc);
    *pidClient = 0;
    return rc;
}

DECLR0VBGL(int) VbglR0CrCtlConDisconnect(VBGLCRCTLHANDLE hCtl, HGCMCLIENTID idClient)
{
    VBGLIOCHGCMDISCONNECT info;
    VBGLREQHDR_INIT(&info.Hdr, HGCM_DISCONNECT);
    info.u.In.idClient = idClient;
    return VbglR0IdcCall(&hCtl->IdcHandle, VBGL_IOCTL_HGCM_DISCONNECT, &info.Hdr, sizeof(info));
}

DECLR0VBGL(int) VbglR0CrCtlConCallRaw(VBGLCRCTLHANDLE hCtl, PVBGLIOCHGCMCALL pCallInfo, int cbCallInfo)
{
    return VbglR0IdcCallRaw(&hCtl->IdcHandle, VBGL_IOCTL_HGCM_CALL(cbCallInfo), &pCallInfo->Hdr, cbCallInfo);
}

DECLR0VBGL(int) VbglR0CrCtlConCall(VBGLCRCTLHANDLE hCtl, PVBGLIOCHGCMCALL pCallInfo, int cbCallInfo)
{
    int rc = VbglR0IdcCallRaw(&hCtl->IdcHandle, VBGL_IOCTL_HGCM_CALL(cbCallInfo), &pCallInfo->Hdr, cbCallInfo);
    if (RT_SUCCESS(rc))
        rc = pCallInfo->Hdr.rc;
    return rc;
}

DECLR0VBGL(int) VbglR0CrCtlConCallUserDataRaw(VBGLCRCTLHANDLE hCtl, PVBGLIOCHGCMCALL pCallInfo, int cbCallInfo)
{
    return VbglR0IdcCallRaw(&hCtl->IdcHandle, VBGL_IOCTL_HGCM_CALL_WITH_USER_DATA(cbCallInfo), &pCallInfo->Hdr, cbCallInfo);
}

