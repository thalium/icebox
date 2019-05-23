/** @file
 * IPRT - Lock Free Circular Buffer
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

#ifndef ___iprt_circbuf_h
#define ___iprt_circbuf_h

#include <iprt/types.h>

/** @defgroup grp_rt_circbuf    RTCircBuf - Lock Free Circular Buffer
 * @ingroup grp_rt
 *
 * Implementation of a lock free circular buffer which could be used in a multi
 * threaded environment. Note that only the acquire, release and getter
 * functions are threading aware. So don't use reset if the circular buffer is
 * still in use.
 *
 * @{
 */

RT_C_DECLS_BEGIN

/** Pointer to a circular buffer (abstract). */
typedef struct RTCIRCBUF *PRTCIRCBUF;

/**
 * Create a circular buffer.
 *
 * @returns IPRT status code.
 *
 * @param   ppBuf          Where to store the buffer.
 * @param   cbSize         The size of the new buffer.
 */
RTDECL(int) RTCircBufCreate(PRTCIRCBUF *ppBuf, size_t cbSize);

/**
 * Destroy the circular buffer.
 *
 * @param   pBuf           The buffer to destroy.  NULL is ignored.
 */
RTDECL(void) RTCircBufDestroy(PRTCIRCBUF pBuf);

/**
 * Reset all position information in the circular buffer.
 *
 * @note This function is not multi threading aware.
 *
 * @param   pBuf           The buffer to reset.
 */
RTDECL(void) RTCircBufReset(PRTCIRCBUF pBuf);

/**
 * Returns the current free space of the buffer.
 *
 * @param   pBuf           The buffer to query.
 */
RTDECL(size_t) RTCircBufFree(PRTCIRCBUF pBuf);

/**
 * Returns the current used space of the buffer.
 *
 * @param   pBuf           The buffer to query.
 */
RTDECL(size_t) RTCircBufUsed(PRTCIRCBUF pBuf);

/**
 * Returns the size of the buffer.
 *
 * @param   pBuf           The buffer to query.
 */
RTDECL(size_t) RTCircBufSize(PRTCIRCBUF pBuf);

RTDECL(bool) RTCircBufIsReading(PRTCIRCBUF pBuf);
RTDECL(bool) RTCircBufIsWriting(PRTCIRCBUF pBuf);

/**
 * Returns the current read offset (in bytes) within the buffer.
 *
 * @param   pBuf           The buffer to query.
 */
RTDECL(size_t) RTCircBufOffsetRead(PRTCIRCBUF pBuf);

/**
 * Returns the current write offset (in bytes) within the buffer.
 *
 * @param   pBuf           The buffer to query.
 */
RTDECL(size_t) RTCircBufOffsetWrite(PRTCIRCBUF pBuf);

/**
 * Acquire a block of the circular buffer for reading.
 *
 * @param   pBuf           The buffer to acquire from.
 * @param   cbReqSize      The requested size of the block.
 * @param   ppvStart       The resulting memory pointer.
 * @param   pcbSize        The resulting size of the memory pointer.
 */
RTDECL(void) RTCircBufAcquireReadBlock(PRTCIRCBUF pBuf, size_t cbReqSize, void **ppvStart, size_t *pcbSize);

/**
 * Release a block which was acquired by RTCircBufAcquireReadBlock.
 *
 * @param   pBuf           The buffer to acquire from.
 * @param   cbSize         The size of the block.
 */
RTDECL(void) RTCircBufReleaseReadBlock(PRTCIRCBUF pBuf, size_t cbSize);

/**
 * Acquire a block of the circular buffer for writing.
 *
 * @param   pBuf           The buffer to acquire from.
 * @param   cbReqSize      The requested size of the block.
 * @param   ppvStart       The resulting memory pointer.
 * @param   pcbSize        The resulting size of the memory pointer.
 */
RTDECL(void) RTCircBufAcquireWriteBlock(PRTCIRCBUF pBuf, size_t cbReqSize, void **ppvStart, size_t *pcbSize);

/**
 * Release a block which was acquired by RTCircBufAcquireWriteBlock.
 *
 * @param   pBuf           The buffer to acquire from.
 * @param   cbSize         The size of the block.
 */
RTDECL(void) RTCircBufReleaseWriteBlock(PRTCIRCBUF pBuf, size_t cbSize);

RT_C_DECLS_END

/** @} */

#endif /* !___iprt_circbuf_h */

