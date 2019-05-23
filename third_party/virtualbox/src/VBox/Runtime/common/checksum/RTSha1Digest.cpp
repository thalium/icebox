/* $Id: RTSha1Digest.cpp $ */
/** @file
 * IPRT - SHA1 digest creation
 *
 * @todo Replace this with generic RTCrDigest based implementation. Too much
 *       stupid code duplication.
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
#include <iprt/sha.h>

#include <iprt/alloca.h>
#include <iprt/assert.h>
#include <iprt/mem.h>
#include <iprt/string.h>
#include <iprt/file.h>


RTR3DECL(int) RTSha1Digest(void* pvBuf, size_t cbBuf, char **ppszDigest, PFNRTPROGRESS pfnProgressCallback, void *pvUser)
{
    /* Validate input */
    AssertPtrReturn(pvBuf, VERR_INVALID_POINTER);
    AssertPtrReturn(ppszDigest, VERR_INVALID_POINTER);
    AssertPtrNullReturn(pfnProgressCallback, VERR_INVALID_PARAMETER);

    int rc = VINF_SUCCESS;
    *ppszDigest = NULL;

    /* Initialize the hash context. */
    RTSHA1CONTEXT Ctx;
    RTSha1Init(&Ctx);

    /* Buffer size for progress callback */
    double rdMulti = 100.0 / (cbBuf ? cbBuf : 1);

    /* Working buffer */
    char *pvTmp = (char*)pvBuf;

    /* Process the memory in blocks */
    size_t cbReadTotal = 0;
    for (;;)
    {
        size_t cbRead = RT_MIN(cbBuf - cbReadTotal, _1M);
        RTSha1Update(&Ctx, pvTmp, cbRead);
        cbReadTotal += cbRead;
        pvTmp += cbRead;

        /* Call the progress callback if one is defined */
        if (pfnProgressCallback)
        {
            rc = pfnProgressCallback((unsigned)(cbReadTotal * rdMulti), pvUser);
            if (RT_FAILURE(rc))
                break; /* canceled */
        }
        /* Finished? */
        if (cbReadTotal == cbBuf)
            break;
    }
    if (RT_SUCCESS(rc))
    {
        /* Finally calculate & format the SHA1 sum */
        uint8_t abHash[RTSHA1_HASH_SIZE];
        RTSha1Final(&Ctx, abHash);

        char *pszDigest;
        rc = RTStrAllocEx(&pszDigest, RTSHA1_DIGEST_LEN + 1);
        if (RT_SUCCESS(rc))
        {
            rc = RTSha1ToString(abHash, pszDigest, RTSHA1_DIGEST_LEN + 1);
            if (RT_SUCCESS(rc))
                *ppszDigest = pszDigest;
            else
                RTStrFree(pszDigest);
        }
    }

    return rc;
}

RTR3DECL(int) RTSha1DigestFromFile(const char *pszFile, char **ppszDigest, PFNRTPROGRESS pfnProgressCallback, void *pvUser)
{
    /* Validate input */
    AssertPtrReturn(pszFile, VERR_INVALID_POINTER);
    AssertPtrReturn(ppszDigest, VERR_INVALID_POINTER);
    AssertPtrNullReturn(pfnProgressCallback, VERR_INVALID_PARAMETER);

    *ppszDigest = NULL;

    /* Open the file to calculate a SHA1 sum of */
    RTFILE hFile;
    int rc = RTFileOpen(&hFile, pszFile, RTFILE_O_OPEN | RTFILE_O_READ | RTFILE_O_DENY_WRITE);
    if (RT_FAILURE(rc))
        return rc;

    /* Fetch the file size. Only needed if there is a progress callback. */
    double rdMulti = 0;
    if (pfnProgressCallback)
    {
        uint64_t cbFile;
        rc = RTFileGetSize(hFile, &cbFile);
        if (RT_FAILURE(rc))
        {
            RTFileClose(hFile);
            return rc;
        }
        rdMulti = 100.0 / (cbFile ? cbFile : 1);
    }

    /* Allocate a reasonably large buffer, fall back on a tiny one. */
    void  *pvBufFree;
    size_t cbBuf = _1M;
    void  *pvBuf = pvBufFree = RTMemTmpAlloc(cbBuf);
    if (!pvBuf)
    {
        cbBuf = 0x1000;
        pvBuf = alloca(cbBuf);
    }

    /* Initialize the hash context. */
    RTSHA1CONTEXT Ctx;
    RTSha1Init(&Ctx);

    /* Read that file in blocks */
    size_t cbReadTotal = 0;
    for (;;)
    {
        size_t cbRead;
        rc = RTFileRead(hFile, pvBuf, cbBuf, &cbRead);
        if (RT_FAILURE(rc) || !cbRead)
            break;
        RTSha1Update(&Ctx, pvBuf, cbRead);
        cbReadTotal += cbRead;

        /* Call the progress callback if one is defined */
        if (pfnProgressCallback)
        {
            rc = pfnProgressCallback((unsigned)(cbReadTotal * rdMulti), pvUser);
            if (RT_FAILURE(rc))
                break; /* canceled */
        }
    }
    RTMemTmpFree(pvBufFree);
    RTFileClose(hFile);

    if (RT_FAILURE(rc))
        return rc;

    /* Finally calculate & format the SHA1 sum */
    uint8_t abHash[RTSHA1_HASH_SIZE];
    RTSha1Final(&Ctx, abHash);

    char *pszDigest;
    rc = RTStrAllocEx(&pszDigest, RTSHA1_DIGEST_LEN + 1);
    if (RT_SUCCESS(rc))
    {
        rc = RTSha1ToString(abHash, pszDigest, RTSHA1_DIGEST_LEN + 1);
        if (RT_SUCCESS(rc))
            *ppszDigest = pszDigest;
        else
            RTStrFree(pszDigest);
    }

    return rc;
}

