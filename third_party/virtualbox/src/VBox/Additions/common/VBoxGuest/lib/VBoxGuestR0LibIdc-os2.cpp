/* $Id: VBoxGuestR0LibIdc-os2.cpp $ */
/** @file
 * VBoxGuestLib - Ring-0 Support Library for VBoxGuest, IDC, OS/2 specific.
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
#include "VBoxGuestR0LibInternal.h"
#include <VBox/err.h>
#include <VBox/log.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/* This is defined in some assembly file.  The AttachDD operation
   is done in the driver init code. */
extern VBGLOS2ATTACHDD g_VBoxGuestIDC;



int VBOXCALL vbglR0IdcNativeOpen(PVBGLIDCHANDLE pHandle, PVBGLIOCIDCCONNECT pReq)
{
    /*
     * Just check whether the connection was made or not.
     */
    if (   g_VBoxGuestIDC.u32Version == VBGL_IOC_VERSION
        && RT_VALID_PTR(g_VBoxGuestIDC.u32Session)
        && RT_VALID_PTR(g_VBoxGuestIDC.pfnServiceEP))
        return g_VBoxGuestIDC.pfnServiceEP(g_VBoxGuestIDC.u32Session, VBGL_IOCTL_IDC_CONNECT, &pReq->Hdr, sizeof(*pReq));
    Log(("vbglDriverOpen: failed\n"));
    return VERR_FILE_NOT_FOUND;
}


int VBOXCALL vbglR0IdcNativeClose(PVBGLIDCHANDLE pHandle, PVBGLIOCIDCDISCONNECT pReq)
{
    return g_VBoxGuestIDC.pfnServiceEP((uintptr_t)pHandle->s.pvSession, VBGL_IOCTL_IDC_DISCONNECT, &pReq->Hdr, sizeof(*pReq));
}


/**
 * Makes an IDC call, returning only the I/O control status code.
 *
 * @returns VBox status code (the I/O control failure status).
 * @param   pHandle             The IDC handle.
 * @param   uReq                The request number.
 * @param   pReqHdr             The request header.
 * @param   cbReq               The request size.
 */
DECLR0VBGL(int) VbglR0IdcCallRaw(PVBGLIDCHANDLE pHandle, uintptr_t uReq, PVBGLREQHDR pReqHdr, uint32_t cbReq)
{
    return g_VBoxGuestIDC.pfnServiceEP((uintptr_t)pHandle->s.pvSession, uReq, pReqHdr, cbReq);
}

