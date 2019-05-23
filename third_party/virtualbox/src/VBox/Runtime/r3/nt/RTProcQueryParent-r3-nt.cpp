/* $Id: RTProcQueryParent-r3-nt.cpp $ */
/** @file
 * IPRT - Process, Windows.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
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
#define LOG_GROUP RTLOGGROUP_PROCESS
#include <iprt/nt/nt.h>

#include <iprt/process.h>
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/err.h>



RTR3DECL(int) RTProcQueryParent(RTPROCESS hProcess, PRTPROCESS phParent)
{
    NTSTATUS    rcNt;
    HANDLE      hClose = RTNT_INVALID_HANDLE_VALUE;
    HANDLE      hNtProc;

    /*
     * Open the process.  We take a shortcut if it's the current process.
     */
    if (hProcess == RTProcSelf())
        hNtProc = NtCurrentProcess();
    else
    {
        CLIENT_ID ClientId;
        ClientId.UniqueProcess = (HANDLE)(uintptr_t)hProcess;
        ClientId.UniqueThread  = NULL;

        OBJECT_ATTRIBUTES ObjAttrs;
        InitializeObjectAttributes(&ObjAttrs, NULL, OBJ_CASE_INSENSITIVE, NULL, NULL);

        rcNt = NtOpenProcess(&hClose, PROCESS_QUERY_LIMITED_INFORMATION, &ObjAttrs, &ClientId);
        if (!NT_SUCCESS(rcNt))
            rcNt = NtOpenProcess(&hClose, PROCESS_QUERY_INFORMATION, &ObjAttrs, &ClientId);
        if (!NT_SUCCESS(rcNt))
            return RTErrConvertFromNtStatus(rcNt);
        hNtProc = hClose;
    }

    /*
     * Query the information.
     */
    int rc;
    PROCESS_BASIC_INFORMATION BasicInfo;
    ULONG cbIgn;
    rcNt = NtQueryInformationProcess(hNtProc, ProcessBasicInformation, &BasicInfo, sizeof(BasicInfo), &cbIgn);
    if (NT_SUCCESS(rcNt))
    {
        *phParent = BasicInfo.InheritedFromUniqueProcessId;
        rc = VINF_SUCCESS;
    }
    else
        rc = RTErrConvertFromNtStatus(rcNt);

    /*
     * Clean up.
     */
    if (hClose != RTNT_INVALID_HANDLE_VALUE)
        NtClose(hClose);

    return rc;
}

