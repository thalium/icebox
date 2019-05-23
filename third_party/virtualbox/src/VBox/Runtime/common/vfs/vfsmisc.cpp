/* $Id: vfsmisc.cpp $ */
/** @file
 * IPRT - Virtual File System, Misc functions with heavy dependencies.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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
#include "internal/iprt.h"
#include <iprt/vfs.h>

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/handle.h>



RTDECL(int)         RTVfsIoStrmFromStdHandle(RTHANDLESTD enmStdHandle, uint64_t fOpen, bool fLeaveOpen,
                                             PRTVFSIOSTREAM phVfsIos)
{
    /*
     * Input validation.
     */
    AssertPtrReturn(phVfsIos, VERR_INVALID_POINTER);
    *phVfsIos = NIL_RTVFSIOSTREAM;
    AssertReturn(   enmStdHandle == RTHANDLESTD_INPUT
                 || enmStdHandle == RTHANDLESTD_OUTPUT
                 || enmStdHandle == RTHANDLESTD_ERROR,
                 VERR_INVALID_PARAMETER);
    AssertReturn(!(fOpen & ~RTFILE_O_VALID_MASK), VERR_INVALID_PARAMETER);
    if (enmStdHandle == RTHANDLESTD_INPUT)
        fOpen |= RTFILE_O_READ;
    else
        fOpen |= RTFILE_O_WRITE;

    /*
     * Open the handle and see what we get back.
     */
    RTHANDLE h;
    int rc = RTHandleGetStandard(enmStdHandle, &h);
    if (RT_SUCCESS(rc))
    {
        switch (h.enmType)
        {
            case RTHANDLETYPE_FILE:
                rc = RTVfsIoStrmFromRTFile(h.u.hFile, fOpen, fLeaveOpen, phVfsIos);
                break;

            case RTHANDLETYPE_PIPE:
                rc = RTVfsIoStrmFromRTPipe(h.u.hPipe, fLeaveOpen, phVfsIos);
                break;

            case RTHANDLETYPE_SOCKET:
                /** @todo  */
                rc = VERR_NOT_IMPLEMENTED;
                break;

            default:
                rc = VERR_NOT_IMPLEMENTED;
                break;
        }
    }

    return rc;
}

