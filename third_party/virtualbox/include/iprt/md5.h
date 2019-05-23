/** @file
 * IPRT - Message-Digest algorithm 5.
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

#ifndef ___iprt_md5_h
#define ___iprt_md5_h

#include <iprt/types.h>

/** @defgroup grp_rt_md5    RTMd5 - Message-Digest algorithm 5
 * @ingroup grp_rt
 * @{
 */

/** Size of a MD5 hash. */
#define RTMD5_HASH_SIZE     16
/** @deprecated Use RTMD5_HASH_SIZE. */
#define RTMD5HASHSIZE       RTMD5_HASH_SIZE
/** The length of a MD5 digest string. The terminator is not included. */
#define RTMD5_DIGEST_LEN    32
/** Size of a MD5 hash.
 * @deprecated Use RTMD5_DIGEST_LEN  */
#define RTMD5_STRING_LEN    RTMD5_DIGEST_LEN

/**
 * MD5 hash algorithm context.
 */
typedef union RTMD5CONTEXT
{
    uint64_t            u64BetterAlignment;
    uint8_t             abPadding[(4 + 6 + 16 + 1) * sizeof(uint32_t)];
    /** Context used by md5-alt.cpp. */
    struct
    {
        uint32_t        in[16];
        uint32_t        buf[4];
        uint32_t        bits[2];
    } AltPrivate;
#ifdef RT_MD5_OPENSSL_PRIVATE_CONTEXT
    /** Context used by md5-openssl.cpp. */
    MD5_CTX         OsslPrivate;
#endif
} RTMD5CONTEXT;
/** Pointer to MD5 hash algorithm context. */
typedef RTMD5CONTEXT *PRTMD5CONTEXT;

RT_C_DECLS_BEGIN

/**
 * Compute the MD5 hash of the data.
 *
 * @param   pvBuf       Pointer to data.
 * @param   cbBuf       Length of data (in bytes).
 * @param   pabDigest   Where to store the hash.
 *                      (What's passed is a pointer to the caller's buffer.)
 */
RTDECL(void) RTMd5(const void *pvBuf, size_t cbBuf, uint8_t pabDigest[RTMD5HASHSIZE]);

/**
 * Initialize MD5 context.
 *
 * @param   pCtx        Pointer to the MD5 context to initialize.
 */
RTDECL(void) RTMd5Init(PRTMD5CONTEXT pCtx);

/**
 * Feed data into the MD5 computation.
 *
 * @param   pCtx        Pointer to the MD5 context.
 * @param   pvBuf       Pointer to data.
 * @param   cbBuf       Length of data (in bytes).
 */
RTDECL(void) RTMd5Update(PRTMD5CONTEXT pCtx, const void *pvBuf, size_t cbBuf);

/**
 * Compute the MD5 hash of the data.
 *
 * @param   pabDigest   Where to store the hash.
 *                      (What's passed is a pointer to the caller's buffer.)
 * @param   pCtx        Pointer to the MD5 context.
 */
RTDECL(void) RTMd5Final(uint8_t pabDigest[RTMD5HASHSIZE], PRTMD5CONTEXT pCtx);

/**
 * Converts a MD5 hash to a digest string.
 *
 * @returns IPRT status code.
 *
 * @param   pabDigest   The binary digest returned by RTMd5Final or RTMd5.
 * @param   pszDigest   Where to return the stringified digest.
 * @param   cchDigest   The size of the output buffer. Should be at least
 *                      RTMD5_STRING_LEN + 1 bytes.
 */
RTDECL(int) RTMd5ToString(uint8_t const pabDigest[RTMD5_HASH_SIZE], char *pszDigest, size_t cchDigest);

/**
 * Converts a MD5 hash to a digest string.
 *
 * @returns IPRT status code.
 *
 * @param   pszDigest   The stringified digest. Leading and trailing spaces are
 *                      ignored.
 * @param   pabDigest   Where to store the hash. (What is passed is a pointer to
 *                      the caller's buffer.)
 */
RTDECL(int) RTMd5FromString(char const *pszDigest, uint8_t pabDigest[RTMD5_HASH_SIZE]);


RT_C_DECLS_END

/** @} */

#endif

