/* $Id: VBoxGuestR3LibBalloon.cpp $ */
/** @file
 * VBoxGuestR3Lib - Ring-3 Support Library for VirtualBox guest additions, Ballooning.
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
#include <iprt/string.h>


/**
 * Refresh the memory balloon after a change.
 *
 * @returns IPRT status code.
 * @param   pcChunks        The size of the balloon in chunks of 1MB (out).
 * @param   pfHandleInR3    Allocating of memory in R3 required (out).
 */
VBGLR3DECL(int) VbglR3MemBalloonRefresh(uint32_t *pcChunks, bool *pfHandleInR3)
{
    VBGLIOCCHECKBALLOON Info;
    VBGLREQHDR_INIT(&Info.Hdr, CHECK_BALLOON);
    int rc = vbglR3DoIOCtl(VBGL_IOCTL_CHECK_BALLOON, &Info.Hdr, sizeof(Info));
    if (RT_SUCCESS(rc))
    {
        *pcChunks = Info.u.Out.cBalloonChunks;
        *pfHandleInR3 = Info.u.Out.fHandleInR3 != false;
    }
    return rc;
}


/**
 * Change the memory by granting/reclaiming memory to/from R0.
 *
 * @returns IPRT status code.
 * @param   pv          Memory chunk (1MB).
 * @param   fInflate    true = inflate balloon (grant memory).
 *                      false = deflate balloon (reclaim memory).
 */
VBGLR3DECL(int) VbglR3MemBalloonChange(void *pv, bool fInflate)
{
    VBGLIOCCHANGEBALLOON Info;
    VBGLREQHDR_INIT(&Info.Hdr, CHANGE_BALLOON);
    Info.u.In.pvChunk  = pv;
    Info.u.In.fInflate = fInflate;
    RT_ZERO(Info.u.In.abPadding);
    return vbglR3DoIOCtl(VBGL_IOCTL_CHANGE_BALLOON, &Info.Hdr, sizeof(Info));
}

