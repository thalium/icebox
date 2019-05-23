/* $Id: systemmem-os2.cpp $ */
/** @file
 * IPRT - RTSystemQueryTotalRam, OS/2 ring-3.
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
#define INCL_DOSMISC
#define INCL_ERRORS
#include <os2.h>
#undef RT_MAX

#include <iprt/system.h>
#include "internal/iprt.h"

#include <iprt/err.h>
#include <iprt/assert.h>
#include <iprt/string.h>


RTDECL(int) RTSystemQueryTotalRam(uint64_t *pcb)
{
    AssertPtrReturn(pcb, VERR_INVALID_POINTER);

    ULONG cbMem;
    APIRET rc = DosQuerySysInfo(QSV_TOTPHYSMEM, QSV_TOTPHYSMEM, &cbMem, sizeof(cbMem));
    if (rc != NO_ERROR)
        return RTErrConvertFromOS2(rc);

    *pcb = cbMem;
    return VINF_SUCCESS;
}


RTDECL(int) RTSystemQueryAvailableRam(uint64_t *pcb)
{
    AssertPtrReturn(pcb, VERR_INVALID_POINTER);

    ULONG cbAvailMem;
    APIRET rc = DosQuerySysInfo(QSV_TOTAVAILMEM, QSV_TOTAVAILMEM, &cbAvailMem, sizeof(cbAvailMem));
    if (rc != NO_ERROR)
        return RTErrConvertFromOS2(rc);

    *pcb = cbAvailMem;
    return VINF_SUCCESS;
}

