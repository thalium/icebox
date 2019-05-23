/* $Id: RTUtf16CopyAscii.cpp $ */
/** @file
 * IPRT - RTUtf16CopyAscii.
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
#include <iprt/string.h>
#include "internal/iprt.h"


RTDECL(int) RTUtf16CopyAscii(PRTUTF16 pwszDst, size_t cwcDst, const char *pszSrc)
{
    int rc;
    size_t cchSrc = strlen(pszSrc);
    size_t cchCopy;
    if (RT_LIKELY(cchSrc < cwcDst))
    {
        rc = VINF_SUCCESS;
        cchCopy = cchSrc;
    }
    else if (cwcDst != 0)
    {
        rc = VERR_BUFFER_OVERFLOW;
        cchCopy = cwcDst - 1;
    }
    else
        return VERR_BUFFER_OVERFLOW;

    pwszDst[cchCopy] = '\0';
    while (cchCopy-- > 0)
    {
        unsigned char ch = pszSrc[cchCopy];
        if (RT_LIKELY(ch < 0x80))
            pwszDst[cchCopy] = ch;
        else
        {
            AssertMsgFailed(("ch=%#x\n", ch));
            pwszDst[cchCopy] = 0x7f;
            if (rc == VINF_SUCCESS)
                rc = VERR_OUT_OF_RANGE;
        }
    }
    return rc;
}
RT_EXPORT_SYMBOL(RTUtf16CopyAscii);

