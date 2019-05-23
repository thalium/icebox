/* $Id: strcache-stubs-generic.cpp $ */
/** @file
 * IPRT - String Cache, stub implementation.
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
#include <iprt/strcache.h>
#include "internal/iprt.h"

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/mempool.h>
#include <iprt/string.h>



RTDECL(int) RTStrCacheCreate(PRTSTRCACHE phStrCache, const char *pszName)
{
    AssertCompile(sizeof(RTSTRCACHE) == sizeof(RTMEMPOOL));
    AssertCompileNS(NIL_RTSTRCACHE     == (RTSTRCACHE)NIL_RTMEMPOOL);
    AssertCompileNS(RTSTRCACHE_DEFAULT == (RTSTRCACHE)RTMEMPOOL_DEFAULT);
    return RTMemPoolCreate((PRTMEMPOOL)phStrCache, pszName);
}
RT_EXPORT_SYMBOL(RTStrCacheCreate);


RTDECL(int) RTStrCacheDestroy(RTSTRCACHE hStrCache)
{
    if (    hStrCache == NIL_RTSTRCACHE
        ||  hStrCache == RTSTRCACHE_DEFAULT)
        return VINF_SUCCESS;
    return RTMemPoolDestroy((RTMEMPOOL)hStrCache);
}
RT_EXPORT_SYMBOL(RTStrCacheDestroy);


RTDECL(const char *) RTStrCacheEnterN(RTSTRCACHE hStrCache, const char *pchString, size_t cchString)
{
    AssertPtr(pchString);
    AssertReturn(cchString < _1G, NULL);
    Assert(!RTStrEnd(pchString, cchString));

    return (const char *)RTMemPoolDupEx((RTMEMPOOL)hStrCache, pchString, cchString, 1);
}
RT_EXPORT_SYMBOL(RTStrCacheEnterN);


RTDECL(const char *) RTStrCacheEnter(RTSTRCACHE hStrCache, const char *psz)
{
    return RTStrCacheEnterN(hStrCache, psz, strlen(psz));
}
RT_EXPORT_SYMBOL(RTStrCacheEnter);


RTDECL(const char *) RTStrCacheEnterLowerN(RTSTRCACHE hStrCache, const char *pchString, size_t cchString)
{
    AssertPtr(pchString);
    AssertReturn(cchString < _1G, NULL);
    Assert(!RTStrEnd(pchString, cchString));

    char *pszRet = (char *)RTMemPoolDupEx((RTMEMPOOL)hStrCache, pchString, cchString, 1);
    if (pszRet)
        RTStrToLower(pszRet);
    return pszRet;
}
RT_EXPORT_SYMBOL(RTStrCacheEnterLowerN);


RTDECL(const char *) RTStrCacheEnterLower(RTSTRCACHE hStrCache, const char *psz)
{
    return RTStrCacheEnterLowerN(hStrCache, psz, strlen(psz));
}
RT_EXPORT_SYMBOL(RTStrCacheEnterLower);


RTDECL(uint32_t) RTStrCacheRetain(const char *psz)
{
    AssertPtr(psz);
    return RTMemPoolRetain((void *)psz);
}
RT_EXPORT_SYMBOL(RTStrCacheRetain);


RTDECL(uint32_t) RTStrCacheRelease(RTSTRCACHE hStrCache, const char *psz)
{
    if (!psz)
        return 0;
    return RTMemPoolRelease((RTMEMPOOL)hStrCache, (void *)psz);
}
RT_EXPORT_SYMBOL(RTStrCacheRelease);


RTDECL(size_t) RTStrCacheLength(const char *psz)
{
    if (!psz)
        return 0;
    return strlen(psz);
}
RT_EXPORT_SYMBOL(RTStrCacheLength);


RTDECL(bool) RTStrCacheIsRealImpl(void)
{
    return false;
}
RT_EXPORT_SYMBOL(RTStrCacheIsRealImpl);


RTDECL(uint32_t) RTStrCacheGetStats(RTSTRCACHE hStrCache, size_t *pcbStrings, size_t *pcbChunks, size_t *pcbBigEntries,
                                    uint32_t *pcHashCollisions, uint32_t *pcHashCollisions2, uint32_t *pcHashInserts,
                                    uint32_t *pcRehashes)
{
    return UINT32_MAX;
}
RT_EXPORT_SYMBOL(RTStrCacheGetStats);

