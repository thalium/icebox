/** @file
 * IPRT - Kernel module.
 */

/*
 * Copyright (C) 2017 Oracle Corporation
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

#ifndef ___iprt_krnlmod_h
#define ___iprt_krnlmod_h

#include <iprt/cdefs.h>
#include <iprt/types.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_kmod RTKrnlMod - Kernel module/driver userspace side API.
 * @ingroup grp_rt
 * @{
 */

/**
 * Checks whether the given kernel module was loaded.
 *
 * @returns IPRT status code.
 * @param   pszName          The driver name to check.
 * @param   pfLoaded         Where to store the flag whether the module is loaded on success.
 */
RTDECL(int) RTKrnlModQueryLoaded(const char *pszName, bool *pfLoaded);

/**
 * Returns the kernel module information handle for the given loaded kernel module.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_FOUND if the kernel driver is not loaded.
 * @param   pszName          The driver name.
 * @param   phKrnlModInfo    Where to store the handle to the kernel module information record.
 */
RTDECL(int) RTKrnlModLoadedQueryInfo(const char *pszName, PRTKRNLMODINFO phKrnlModInfo);

/**
 * Returns the number of kernel modules loaded on the host system.
 *
 * @returns Number of kernel modules loaded.
 */
RTDECL(uint32_t) RTKrnlModLoadedGetCount(void);

/**
 * Returns all loaded kernel modules on the host.
 *
 * @returns IPRT status code.
 * @retval  VERR_BUFFER_OVERFLOW if there are not enough entries in the passed handle array.
 *                               The required number of entries will be returned in pcEntries.
 * @param   pahKrnlModInfo   Where to store the handles to the kernel module information records
 *                           on success.
 * @param   cEntriesMax      Maximum number of entries fitting in the given array.
 * @param   pcEntries        Where to store the number of entries used/required.
 */
RTDECL(int) RTKrnlModLoadedQueryInfoAll(PRTKRNLMODINFO pahKrnlModInfo, uint32_t cEntriesMax,
                                        uint32_t *pcEntries);

/**
 * Retains the given kernel module information record handle.
 *
 * @returns New reference count.
 * @param   hKrnlModInfo     The kernel module information record handle.
 */
RTDECL(uint32_t) RTKrnlModInfoRetain(RTKRNLMODINFO hKrnlModInfo);

/**
 * Releases the given kernel module information record handle.
 *
 * @returns New reference count, on 0 the handle is destroyed.
 * @param   hKrnlModInfo     The kernel module information record handle.
 */
RTDECL(uint32_t) RTKrnlModInfoRelease(RTKRNLMODINFO hKrnlModInfo);

/**
 * Returns the number of references held onto the kernel module by other
 * drivers or userspace clients.
 *
 * @returns Number of references held on the kernel module.
 * @param   hKrnlModInfo     The kernel module information record handle.
 */
RTDECL(uint32_t) RTKrnlModInfoGetRefCnt(RTKRNLMODINFO hKrnlModInfo);

/**
 * Returns the name of the kernel module.
 *
 * @returns Pointer to the kernel module name.
 * @param   hKrnlModInfo     The kernel module information record handle.
 */
RTDECL(const char *) RTKrnlModInfoGetName(RTKRNLMODINFO hKrnlModInfo);

/**
 * Returns the filepath of the kernel module.
 *
 * @returns Pointer to the kernel module path.
 * @param   hKrnlModInfo     The kernel module information record handle.
 */
RTDECL(const char *) RTKrnlModInfoGetFilePath(RTKRNLMODINFO hKrnlModInfo);

/**
 * Returns the size of the kernel module.
 *
 * @returns Size of the kernel module in bytes.
 * @param   hKrnlModInfo     The kernel module information record handle.
 */
RTDECL(size_t) RTKrnlModInfoGetSize(RTKRNLMODINFO hKrnlModInfo);

/**
 * Returns the load address of the kernel module.
 *
 * @returns Load address of the kernel module.
 * @param   hKrnlModInfo     The kernel module information record handle.
 */
RTDECL(RTR0UINTPTR) RTKrnlModInfoGetLoadAddr(RTKRNLMODINFO hKrnlModInfo);

/**
 * Query the kernel information record for a referencing kernel module of the
 * given record.
 *
 * @returns IPRT status code.
 * @param   hKrnlModInfo     The kernel module information record handle.
 * @param   idx              Referencing kernel module index (< reference count
 *                           as retrieved by RTKrnlModInfoGetRefCnt() ).
 * @param   phKrnlModInfoRef Where to store the handle to the referencing kernel module
 *                           information record.
 */
RTDECL(int) RTKrnlModInfoQueryRefModInfo(RTKRNLMODINFO hKrnlModInfo, uint32_t idx,
                                         PRTKRNLMODINFO phKrnlModInfoRef);

/** @} */

RT_C_DECLS_END

#endif

