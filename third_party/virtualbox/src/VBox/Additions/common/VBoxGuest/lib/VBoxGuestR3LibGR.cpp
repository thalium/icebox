/* $Id: VBoxGuestR3LibGR.cpp $ */
/** @file
 * VBoxGuestR3Lib - Ring-3 Support Library for VirtualBox guest additions, GR.
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
#include <iprt/mem.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/err.h>
#include "VBoxGuestR3LibInternal.h"


int vbglR3GRAlloc(VMMDevRequestHeader **ppReq, size_t cb, VMMDevRequestType enmReqType)
{
    VMMDevRequestHeader *pReq;

    AssertPtrReturn(ppReq, VERR_INVALID_PARAMETER);
    AssertMsgReturn(cb >= sizeof(VMMDevRequestHeader) && cb < _1G, ("%#zx vs %#zx\n", cb, sizeof(VMMDevRequestHeader)),
                    VERR_INVALID_PARAMETER);

    pReq = (VMMDevRequestHeader *)RTMemTmpAlloc(cb);
    if (RT_LIKELY(pReq))
    {
        pReq->size        = (uint32_t)cb;
        pReq->version     = VMMDEV_REQUEST_HEADER_VERSION;
        pReq->requestType = enmReqType;
        pReq->rc          = VERR_GENERAL_FAILURE;
        pReq->reserved1   = 0;
        pReq->reserved2   = 0;

        *ppReq = pReq;

        return VINF_SUCCESS;
    }
    return VERR_NO_MEMORY;
}


int vbglR3GRPerform(VMMDevRequestHeader *pReq)
{
    PVBGLREQHDR    pReqHdr = (PVBGLREQHDR)pReq;
    uint32_t const cbReq   = pReqHdr->cbIn;
    Assert(pReqHdr->cbOut == 0 || pReqHdr->cbOut == cbReq);
    pReqHdr->cbOut = cbReq;
    if (pReq->size < _1K)
        return vbglR3DoIOCtl(VBGL_IOCTL_VMMDEV_REQUEST(cbReq), pReqHdr, cbReq);
    return vbglR3DoIOCtl(VBGL_IOCTL_VMMDEV_REQUEST_BIG, pReqHdr, cbReq);
}


void vbglR3GRFree(VMMDevRequestHeader *pReq)
{
    RTMemTmpFree(pReq);
}

