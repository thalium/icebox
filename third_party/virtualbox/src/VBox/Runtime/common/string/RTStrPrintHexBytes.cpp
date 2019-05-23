/* $Id: RTStrPrintHexBytes.cpp $ */
/** @file
 * IPRT - RTStrPrintHexBytes.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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
#include <iprt/string.h>

#include <iprt/assert.h>
#include <iprt/err.h>


RTDECL(int) RTStrPrintHexBytes(char *pszBuf, size_t cbBuf, void const *pv, size_t cb, uint32_t fFlags)
{
    AssertReturn(   !(fFlags & ~(RTSTRPRINTHEXBYTES_F_UPPER | RTSTRPRINTHEXBYTES_F_SEP_SPACE | RTSTRPRINTHEXBYTES_F_SEP_COLON))
                 &&    (fFlags & (RTSTRPRINTHEXBYTES_F_SEP_SPACE | RTSTRPRINTHEXBYTES_F_SEP_COLON))
                    != (RTSTRPRINTHEXBYTES_F_SEP_SPACE | RTSTRPRINTHEXBYTES_F_SEP_COLON),
                 VERR_INVALID_FLAGS);
    AssertPtrReturn(pszBuf, VERR_INVALID_POINTER);
    AssertReturn(cb * 2 >= cb, VERR_BUFFER_OVERFLOW);
    char const chSep = fFlags & RTSTRPRINTHEXBYTES_F_SEP_SPACE ? ' '
                     : fFlags & RTSTRPRINTHEXBYTES_F_SEP_COLON ? ':' : '\0';
    AssertReturn(cbBuf >= cb * (2 + (chSep != '\0')) - (chSep != '\0') + 1, VERR_BUFFER_OVERFLOW);
    if (cb)
        AssertPtrReturn(pv, VERR_INVALID_POINTER);

    static char const s_szHexDigitsLower[17] = "0123456789abcdef";
    static char const s_szHexDigitsUpper[17] = "0123456789ABCDEF";
    const char *pszHexDigits = !(fFlags & RTSTRPRINTHEXBYTES_F_UPPER) ? s_szHexDigitsLower : s_szHexDigitsUpper;

    uint8_t const *pb = (uint8_t const *)pv;

    if (!chSep)
    {
        while (cb-- > 0)
        {
            uint8_t b = *pb++;
            *pszBuf++ = pszHexDigits[b >> 4];
            *pszBuf++ = pszHexDigits[b & 0xf];
        }
    }
    else if (cb-- > 0)
    {
        uint8_t b = *pb++;
        *pszBuf++ = pszHexDigits[b >> 4];
        *pszBuf++ = pszHexDigits[b & 0xf];

        while (cb-- > 0)
        {
            b = *pb++;
            *pszBuf++ = chSep;
            *pszBuf++ = pszHexDigits[b >> 4];
            *pszBuf++ = pszHexDigits[b & 0xf];
        }
    }

    *pszBuf = '\0';
    return VINF_SUCCESS;
}

