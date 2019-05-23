/** @file
 * IPRT - Async I/O manager.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
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

#ifndef ___iprt_aiomgr_h
#define ___iprt_aiomgr_h

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/sg.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_aiomgr RTAioMgr - Async I/O Manager.
 * @ingroup grp_rt
 * @{
 */

/**
 * Completion callback.
 *
 * @returns nothing.
 * @param   hAioMgrFile       File handle the completed request was for.
 * @param   rcReq             Status code of the completed request.
 * @param   pvUser            Opaque user data given when the request was initiated.
 */
typedef DECLCALLBACK(void) FNRTAIOMGRREQCOMPLETE(RTAIOMGRFILE hAioMgrFile, int rcReq, void *pvUser);
/** Pointer to a completion callback. */
typedef FNRTAIOMGRREQCOMPLETE *PFNRTAIOMGRREQCOMPLETE;

/**
 * Create a new async I/O manager.
 *
 * @returns IPRT statuse code.
 * @param   phAioMgr    Where to store the new async I/O manager handle on success.
 * @param   cReqsMax    Maximum number of async I/O requests expected.
 *                      Use UINT32_MAX to make it grow dynamically when required.
 */
RTDECL(int) RTAioMgrCreate(PRTAIOMGR phAioMgr, uint32_t cReqsMax);

/**
 * Retain a async I/O manager handle.
 *
 * @returns New reference count on success, UINT32_MAX on failure.
 * @param   hAioMgr     The async I/O manager to retain.
 */
RTDECL(uint32_t) RTAioMgrRetain(RTAIOMGR hAioMgr);

/**
 * Releases a async I/O manager handle.
 *
 * @returns New reference count on success (0 if closed), UINT32_MAX on failure.
 * @param   hAioMgr     The async I/O manager to release.
 */
RTDECL(uint32_t) RTAioMgrRelease(RTAIOMGR hAioMgr);

/**
 * Assign a given file handle to the given async I/O manager.
 *
 * @returns IPRT status code.
 * @param   hAioMgr           Async I/O manager handle.
 * @param   hFile             File handle to assign.
 * @param   pfnReqComplete    Callback to execute whenever a request for the
 *                            file completed.
 * @param   phAioMgrFile      Where to store the newly created async I/O manager
 *                            handle on success.
 * @param   pvUser            Opaque user data for this file handle.
 *
 * @note This function increases the reference count of the given async I/O manager
 *       by 1.
 */
RTDECL(int) RTAioMgrFileCreate(RTAIOMGR hAioMgr, RTFILE hFile, PFNRTAIOMGRREQCOMPLETE pfnReqComplete,
                               void *pvUser, PRTAIOMGRFILE phAioMgrFile);

/**
 * Retain a async I/O manager file handle.
 *
 * @returns New reference count on success, UINT32_MAX on failure.
 * @param   hAioMgrFile       The file handle to retain.
 */
RTDECL(uint32_t) RTAioMgrFileRetain(RTAIOMGRFILE hAioMgrFile);

/**
 * Releases a async I/O manager file handle.
 *
 * @returns New reference count on success (0 if closed), UINT32_MAX on failure.
 * @param   hAioMgrFile       The file handle to release.
 */
RTDECL(uint32_t) RTAioMgrFileRelease(RTAIOMGRFILE hAioMgrFile);

/**
 * Return opaque user data passed on creation.
 *
 * @returns Opaque user data or NULL if the handle is invalid.
 * @param   hAioMgrFile       The file handle.
 */
RTDECL(void *) RTAioMgrFileGetUser(RTAIOMGRFILE hAioMgrFile);

/**
 * Initiate a read request from the given file handle.
 *
 * @returns IPRT status code.
 * @param   hAioMgrFile       The file handle to read from.
 * @param   off               Start offset to read from.
 * @param   pSgBuf            S/G buffer to read into. The buffer is advanced
 *                            by the amount of data to read.
 * @param   cbRead            Number of bytes to read.
 * @param   pvUser            Opaque user data given in the completion callback.
 */
RTDECL(int) RTAioMgrFileRead(RTAIOMGRFILE hAioMgrFile, RTFOFF off,
                             PRTSGBUF pSgBuf, size_t cbRead, void *pvUser);

/**
 * Initiate a write request to the given file handle.
 *
 * @returns IPRT status code.
 * @param   hAioMgrFile       The file handle to write to.
 * @param   off               Start offset to write to.
 * @param   pSgBuf            S/G buffer to read from. The buffer is advanced
 *                            by the amount of data to write.
 * @param   cbWrite           Number of bytes to write.
 * @param   pvUser            Opaque user data given in the completion callback.
 */
RTDECL(int) RTAioMgrFileWrite(RTAIOMGRFILE hAioMgrFile, RTFOFF off,
                              PRTSGBUF pSgBuf, size_t cbWrite, void *pvUser);

/**
 * Initiates a flush request for the given file handle.
 *
 * @returns IPRT statuse code.
 * @param   hAioMgrFile       The file handle to write to.
 * @param   pvUser            Opaque user data given in the completion callback.
 */
RTDECL(int) RTAioMgrFileFlush(RTAIOMGRFILE hAioMgrFile, void *pvUser);

/** @} */

RT_C_DECLS_END

#endif

