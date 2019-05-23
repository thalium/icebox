/* $Id: SUPR0IdcClient-linux.c $ */
/** @file
 * VirtualBox Support Driver - IDC Client Lib, Linux Specific Code.
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
#include "../SUPR0IdcClientInternal.h"
#include <VBox/err.h>


int VBOXCALL supR0IdcNativeOpen(PSUPDRVIDCHANDLE pHandle, PSUPDRVIDCREQCONNECT pReq)
{
    return supR0IdcNativeCall(pHandle, SUPDRV_IDC_REQ_CONNECT, &pReq->Hdr);
}


int VBOXCALL supR0IdcNativeClose(PSUPDRVIDCHANDLE pHandle, PSUPDRVIDCREQHDR pReq)
{
    return supR0IdcNativeCall(pHandle, SUPDRV_IDC_REQ_DISCONNECT, pReq);
}


int VBOXCALL supR0IdcNativeCall(PSUPDRVIDCHANDLE pHandle, uint32_t iReq, PSUPDRVIDCREQHDR pReq)
{
    int rc = SUPDrvLinuxIDC(iReq, pReq);
    if (RT_SUCCESS(rc))
        rc = pReq->rc;

    NOREF(pHandle);
    return rc;
}

