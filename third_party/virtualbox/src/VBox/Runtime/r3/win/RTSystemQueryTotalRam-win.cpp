/* $Id: RTSystemQueryTotalRam-win.cpp $ */
/** @file
 * IPRT - RTSystemQueryTotalRam, windows ring-3.
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
#include <iprt/win/windows.h>
#include <iprt/system.h>
#include "internal/iprt.h"

#include <iprt/err.h>
#include <iprt/assert.h>
#include <iprt/string.h>

#include "internal-r3-win.h"


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static bool volatile            g_fInitialized = false;
typedef BOOL (WINAPI *PFNGLOBALMEMORYSTATUSEX)(LPMEMORYSTATUSEX);
static PFNGLOBALMEMORYSTATUSEX  g_pfnGlobalMemoryStatusEx = NULL;


/**
 * The GlobalMemoryStatusEx API is not available on older Windows version.
 *
 * @returns Pointer to GlobalMemoryStatusEx or NULL if not available.
 */
DECLINLINE(PFNGLOBALMEMORYSTATUSEX) rtSystemWinGetExApi(void)
{
    PFNGLOBALMEMORYSTATUSEX pfnEx;
    if (g_fInitialized)
        pfnEx = g_pfnGlobalMemoryStatusEx;
    else
    {
        pfnEx = (PFNGLOBALMEMORYSTATUSEX)GetProcAddress(g_hModKernel32, "GlobalMemoryStatusEx");
        g_pfnGlobalMemoryStatusEx = pfnEx;
        g_fInitialized = true;
    }
    return pfnEx;
}


RTDECL(int) RTSystemQueryTotalRam(uint64_t *pcb)
{
    AssertPtrReturn(pcb, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;
    PFNGLOBALMEMORYSTATUSEX pfnGlobalMemoryStatusEx = rtSystemWinGetExApi();
    if (pfnGlobalMemoryStatusEx)
    {
        MEMORYSTATUSEX MemStatus;
        MemStatus.dwLength = sizeof(MemStatus);
        if (pfnGlobalMemoryStatusEx(&MemStatus))
            *pcb = MemStatus.ullTotalPhys;
        else
            rc = RTErrConvertFromWin32(GetLastError());
    }
    else
    {
        MEMORYSTATUS MemStatus;
        RT_ZERO(MemStatus);
        MemStatus.dwLength = sizeof(MemStatus);
        GlobalMemoryStatus(&MemStatus);
        *pcb = MemStatus.dwTotalPhys;
    }
    return rc;
}


RTDECL(int) RTSystemQueryAvailableRam(uint64_t *pcb)
{
    AssertPtrReturn(pcb, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;
    PFNGLOBALMEMORYSTATUSEX pfnGlobalMemoryStatusEx = rtSystemWinGetExApi();
    if (pfnGlobalMemoryStatusEx)
    {
        MEMORYSTATUSEX MemStatus;
        MemStatus.dwLength = sizeof(MemStatus);
        if (pfnGlobalMemoryStatusEx(&MemStatus))
            *pcb = MemStatus.ullAvailPhys;
        else
            rc = RTErrConvertFromWin32(GetLastError());
    }
    else
    {
        MEMORYSTATUS MemStatus;
        RT_ZERO(MemStatus);
        MemStatus.dwLength = sizeof(MemStatus);
        GlobalMemoryStatus(&MemStatus);
        *pcb = MemStatus.dwAvailPhys;
    }
    return rc;
}
