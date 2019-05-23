/* $Id: strhash1.cpp $ */
/** @file
 * IPRT - String Hashing by Algorithm \#1.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
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

#include "internal/strhash.h"


RTDECL(uint32_t)    RTStrHash1(const char *pszString)
{
    size_t cchIgnored;
    return sdbm(pszString, &cchIgnored);
}


RTDECL(uint32_t)    RTStrHash1N(const char *pszString, size_t cchString)
{
    size_t cchIgnored;
    return sdbmN(pszString, cchString, &cchIgnored);
}


RTDECL(uint32_t)    RTStrHash1ExN(size_t cPairs, ...)
{
    va_list va;
    va_start(va, cPairs);
    uint32_t uHash = RTStrHash1ExNV(cPairs, va);
    va_end(va);
    return uHash;
}


RTDECL(uint32_t)    RTStrHash1ExNV(size_t cPairs, va_list va)
{
    uint32_t uHash = 0;
    for (uint32_t i = 0; i < cPairs; i++)
    {
        const char *psz = va_arg(va, const char *);
        size_t      cch = va_arg(va, size_t);
        uHash += sdbmIncN(psz, cch, uHash);
    }
    return uHash;
}

