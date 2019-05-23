/** @file
 * IPRT - Message-Digest Algorithm 2.
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

#ifndef ___iprt_md2_h
#define ___iprt_md2_h

#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_md2    RTMd2 - Message-Digest algorithm 2
 * @ingroup grp_rt
 * @{
 */

/** Size of a MD2 hash. */
#define RTMD2_HASH_SIZE     16
/** The length of a MD2 digest string. The terminator is not included. */
#define RTMD2_DIGEST_LEN    32

/**
 * MD2 hash algorithm context.
 */
typedef union RTMD2CONTEXT
{
    uint64_t            u64BetterAlignment;
    uint8_t             abPadding[4 + 16 + 16*4 + 16*4];
#ifdef RT_MD2_PRIVATE_CONTEXT
    MD2_CTX             Private;
#endif
#ifdef RT_MD2_PRIVATE_ALT_CONTEXT
    RTMD2ALTPRIVATECTX  AltPrivate;
#endif
} RTMD2CONTEXT;

/** Pointer to MD2 hash algorithm context. */
typedef RTMD2CONTEXT *PRTMD2CONTEXT;


/**
 * Compute the MD2 hash of the data.
 *
 * @param   pvBuf       Pointer to data.
 * @param   cbBuf       Length of data (in bytes).
 * @param   pabDigest   Where to store the hash.
 *                      (What's passed is a pointer to the caller's buffer.)
 */
RTDECL(void) RTMd2(const void *pvBuf, size_t cbBuf, uint8_t pabDigest[RTMD2_HASH_SIZE]);

/**
 * Initialize MD2 context.
 *
 * @param   pCtx        Pointer to the MD2 context to initialize.
 */
RTDECL(void) RTMd2Init(PRTMD2CONTEXT pCtx);

/**
 * Feed data into the MD2 computation.
 *
 * @param   pCtx        Pointer to the MD2 context.
 * @param   pvBuf       Pointer to data.
 * @param   cbBuf       Length of data (in bytes).
 */
RTDECL(void) RTMd2Update(PRTMD2CONTEXT pCtx, const void *pvBuf, size_t cbBuf);

/**
 * Compute the MD2 hash of the data.
 *
 * @param   pCtx        Pointer to the MD2 context.
 * @param   pabDigest   Where to store the hash. (What's passed is a pointer to
 *                      the caller's buffer.)
 */
RTDECL(void) RTMd2Final(PRTMD2CONTEXT pCtx, uint8_t pabDigest[RTMD2_HASH_SIZE]);

/**
 * Converts a MD2 hash to a digest string.
 *
 * @returns IPRT status code.
 *
 * @param   pabDigest   The binary digest returned by RTMd2Final or RTMd2.
 * @param   pszDigest   Where to return the stringified digest.
 * @param   cchDigest   The size of the output buffer. Should be at least
 *                      RTMD2_STRING_LEN + 1 bytes.
 */
RTDECL(int) RTMd2ToString(uint8_t const pabDigest[RTMD2_HASH_SIZE], char *pszDigest, size_t cchDigest);

/**
 * Converts a MD2 hash to a digest string.
 *
 * @returns IPRT status code.
 *
 * @param   pszDigest   The stringified digest. Leading and trailing spaces are
 *                      ignored.
 * @param   pabDigest   Where to store the hash. (What is passed is a pointer to
 *                      the caller's buffer.)
 */
RTDECL(int) RTMd2FromString(char const *pszDigest, uint8_t pabDigest[RTMD2_HASH_SIZE]);

/** @} */

RT_C_DECLS_END

#endif

