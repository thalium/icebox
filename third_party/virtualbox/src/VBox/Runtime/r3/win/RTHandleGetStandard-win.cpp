/* $Id: RTHandleGetStandard-win.cpp $ */
/** @file
 * IPRT - RTHandleGetStandard, Windows.
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
#include <iprt/handle.h>

#include <iprt/file.h>
#include <iprt/pipe.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/log.h>

#include <iprt/win/windows.h>

#include "internal/socket.h"            /* (Needs Windows.h.) */


RTDECL(int) RTHandleGetStandard(RTHANDLESTD enmStdHandle, PRTHANDLE ph)
{
    /*
     * Validate and convert input.
     */
    AssertPtrReturn(ph, VERR_INVALID_POINTER);
    DWORD dwStdHandle;
    switch (enmStdHandle)
    {
        case RTHANDLESTD_INPUT:  dwStdHandle = STD_INPUT_HANDLE; break;
        case RTHANDLESTD_OUTPUT: dwStdHandle = STD_OUTPUT_HANDLE; break;
        case RTHANDLESTD_ERROR:  dwStdHandle = STD_ERROR_HANDLE; break;
        default:
            AssertFailedReturn(VERR_INVALID_PARAMETER);
    }

    /*
     * Is the requested descriptor valid and which IPRT handle type does it
     * best map on to?
     */
    HANDLE hNative = GetStdHandle(dwStdHandle);
    if (hNative == INVALID_HANDLE_VALUE)
        return RTErrConvertFromWin32(GetLastError());

    DWORD dwInfo;
    if (!GetHandleInformation(hNative, &dwInfo))
        return RTErrConvertFromWin32(GetLastError());
    bool const fInherit = RT_BOOL(dwInfo & HANDLE_FLAG_INHERIT);

    RTHANDLE h;
    DWORD    dwType = GetFileType(hNative);
    switch (dwType & ~FILE_TYPE_REMOTE)
    {
        default:
        case FILE_TYPE_UNKNOWN:
        case FILE_TYPE_CHAR:
        case FILE_TYPE_DISK:
            h.enmType = RTHANDLETYPE_FILE;
            break;

        case FILE_TYPE_PIPE:
        {
            DWORD cMaxInstances;
            DWORD fInfo;
            if (!GetNamedPipeInfo(hNative, &fInfo, NULL, NULL, &cMaxInstances))
                h.enmType = RTHANDLETYPE_SOCKET;
            else
                h.enmType = RTHANDLETYPE_PIPE;
            break;
        }
    }

    /*
     * Create the IPRT handle.
     */
    int rc;
    switch (h.enmType)
    {
        case RTHANDLETYPE_FILE:
            rc = RTFileFromNative(&h.u.hFile, (RTHCUINTPTR)hNative);
            break;

        case RTHANDLETYPE_PIPE:
            rc = RTPipeFromNative(&h.u.hPipe, (RTHCUINTPTR)hNative,
                                    (enmStdHandle == RTHANDLESTD_INPUT ? RTPIPE_N_READ : RTPIPE_N_WRITE)
                                  | (fInherit ? RTPIPE_N_INHERIT : 0));
            break;

        case RTHANDLETYPE_SOCKET:
            rc = rtSocketCreateForNative(&h.u.hSocket, (RTHCUINTPTR)hNative);
            break;

        default: /* shut up gcc */
            return VERR_INTERNAL_ERROR;
    }

    if (RT_SUCCESS(rc))
        *ph = h;

    return rc;
}

