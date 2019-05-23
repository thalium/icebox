/* $Id: tls-win.cpp $ */
/** @file
 * IPRT - Thread Local Storage (TLS), Win32.
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
#define LOG_GROUP RTLOGGROUP_THREAD
#include <iprt/win/windows.h>

#include <iprt/thread.h>
#include <iprt/log.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include "internal/thread.h"


AssertCompile(sizeof(RTTLS) >= sizeof(DWORD));


RTR3DECL(RTTLS) RTTlsAlloc(void)
{
    DWORD iTls = TlsAlloc();
    return iTls != TLS_OUT_OF_INDEXES ? (RTTLS)iTls : NIL_RTTLS;
}


RTR3DECL(int) RTTlsAllocEx(PRTTLS piTls, PFNRTTLSDTOR pfnDestructor)
{
    AssertReturn(!pfnDestructor, VERR_NOT_SUPPORTED);
    DWORD iTls = TlsAlloc();
    if (iTls != TLS_OUT_OF_INDEXES)
    {
        Assert((RTTLS)iTls != NIL_RTTLS);
        *piTls = (RTTLS)iTls;
        Assert((DWORD)*piTls == iTls);
        return VINF_SUCCESS;
    }

    return VERR_NO_MEMORY;
}


RTR3DECL(int) RTTlsFree(RTTLS iTls)
{
    if (iTls == NIL_RTTLS)
        return VINF_SUCCESS;
    if (TlsFree((DWORD)iTls))
        return VINF_SUCCESS;
    return RTErrConvertFromWin32(GetLastError());
}


RTR3DECL(void *) RTTlsGet(RTTLS iTls)
{
    return TlsGetValue((DWORD)iTls);
}


RTR3DECL(int) RTTlsGetEx(RTTLS iTls, void **ppvValue)
{
    void *pv = TlsGetValue((DWORD)iTls);
    if (pv)
    {
        *ppvValue = pv;
        return VINF_SUCCESS;
    }

    /* TlsGetValue always updates last error */
    *ppvValue = NULL;
    return RTErrConvertFromWin32(GetLastError());
}


RTR3DECL(int) RTTlsSet(RTTLS iTls, void *pvValue)
{
    if (TlsSetValue((DWORD)iTls, pvValue))
        return VINF_SUCCESS;
    return RTErrConvertFromWin32(GetLastError());
}

