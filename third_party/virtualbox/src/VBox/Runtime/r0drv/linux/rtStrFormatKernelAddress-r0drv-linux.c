/* $Id: rtStrFormatKernelAddress-r0drv-linux.c $ */
/** @file
 * IPRT - IPRT String Formatter, ring-0 addresses.
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
#define LOG_GROUP RTLOGGROUP_STRING
#include "the-linux-kernel.h"
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/string.h>

#include "internal/string.h"


DECLHIDDEN(size_t) rtStrFormatKernelAddress(char *pszBuf, size_t cbBuf, RTR0INTPTR uPtr, signed int cchWidth,
                                            signed int cchPrecision, unsigned int fFlags)
{
#if !defined(DEBUG) && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 38)
    RT_NOREF(cchWidth, cchPrecision);
    /* use the Linux kernel function which is able to handle "%pK" */
    static const char s_szFmt[] = "0x%pK";
    const char *pszFmt = s_szFmt;
    if (!(fFlags & RTSTR_F_SPECIAL))
        pszFmt += 2;
    return scnprintf(pszBuf, cbBuf, pszFmt, uPtr);
#else
    Assert(cbBuf >= 64);
    return RTStrFormatNumber(pszBuf, uPtr, 16, cchWidth, cchPrecision, fFlags);
#endif
}
