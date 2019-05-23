/* $Id: openssl-sha1.cpp $ */
/** @file
 * IPRT - SHA-1 hash functions.
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

#include <openssl/sha.h>

#define RT_SHA1_PRIVATE_CONTEXT
#include <iprt/sha.h>

#include <iprt/assert.h>
#include <iprt/string.h>


AssertCompile(RT_SIZEOFMEMB(RTSHA1CONTEXT, abPadding) >= RT_SIZEOFMEMB(RTSHA1CONTEXT, Private));


RTDECL(void) RTSha1(const void *pvBuf, size_t cbBuf, uint8_t pabDigest[RTSHA1_HASH_SIZE])
{
    RTSHA1CONTEXT Ctx;
    RTSha1Init(&Ctx);
    RTSha1Update(&Ctx, pvBuf, cbBuf);
    RTSha1Final(&Ctx, pabDigest);
}
RT_EXPORT_SYMBOL(RTSha1);


RTDECL(bool) RTSha1Check(const void *pvBuf, size_t cbBuf, uint8_t const pabDigest[RTSHA1_HASH_SIZE])
{
    RTSHA1CONTEXT Ctx;
    RTSha1Init(&Ctx);
    RTSha1Update(&Ctx, pvBuf, cbBuf);
    uint8_t abActualDigest[RTSHA1_HASH_SIZE];
    RTSha1Final(&Ctx, abActualDigest);
    bool fRet = memcmp(pabDigest, abActualDigest, RTSHA1_HASH_SIZE) == 0;
    RT_ZERO(abActualDigest);
    return fRet;
}
RT_EXPORT_SYMBOL(RTSha1Check);


RTDECL(void) RTSha1Init(PRTSHA1CONTEXT pCtx)
{
    SHA1_Init(&pCtx->Private);
}
RT_EXPORT_SYMBOL(RTSha1Init);


RTDECL(void) RTSha1Update(PRTSHA1CONTEXT pCtx, const void *pvBuf, size_t cbBuf)
{
    SHA1_Update(&pCtx->Private, pvBuf, cbBuf);
}
RT_EXPORT_SYMBOL(RTSha1Update);


RTDECL(void) RTSha1Final(PRTSHA1CONTEXT pCtx, uint8_t pabDigest[RTSHA1_HASH_SIZE])
{
    SHA1_Final((unsigned char *)&pabDigest[0], &pCtx->Private);
}
RT_EXPORT_SYMBOL(RTSha1Final);

