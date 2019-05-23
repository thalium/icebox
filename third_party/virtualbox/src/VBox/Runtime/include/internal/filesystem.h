/* $Id: filesystem.h $ */
/** @file
 * IPRT - Filesystem implementations.
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

#ifndef ___internal_filesystem_h
#define ___internal_filesystem_h

#include <iprt/types.h>
#include <iprt/err.h>
#include <iprt/assert.h>
#include <iprt/vfslowlevel.h>
#include "internal/magics.h"

RT_C_DECLS_BEGIN

/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/

/** Filesystem format specific initialization structure. */
typedef DECLCALLBACK(int) FNRTFILESYSTEMINIT(void *pvThis, RTVFSFILE hVfsFile);
/** Pointer to a format specific initialization structure. */
typedef FNRTFILESYSTEMINIT *PFNRTFILESYSTEMINIT;

#define RTFILESYSTEM_MATCH_SCORE_UNSUPPORTED 0
#define RTFILESYSTEM_MATCH_SCORE_SUPPORTED UINT32_MAX

/**
 * Filesystem descriptor.
 */
typedef struct RTFILESYSTEMDESC
{
    /** Size of the filesystem specific state in bytes. */
    size_t        cbFs;
    /** Pointer to the VFS vtable. */
    RTVFSOPS      VfsOps;

    /**
     * Probes the underlying for a known filesystem.
     *
     * @returns IPRT status code.
     * @param   hVfsFile    VFS file handle of the underlying medium.
     * @param   puScore     Where to store the match score on success.
     */
    DECLCALLBACKMEMBER(int, pfnProbe) (RTVFSFILE hVfsFile, uint32_t *puScore);

    /**
     * Initializes the given filesystem state.
     *
     * @returns IPRT status code.
     * @param   pvThis      Uninitialized filesystem state.
     * @param   hVfsFile    VFS file handle of the underlying medium.
     */
    DECLCALLBACKMEMBER(int, pfnInit) (void *pvThis, RTVFSFILE hVfsFile);

} RTFILESYSTEMDESC;
/** Pointer to a filesystem descriptor. */
typedef RTFILESYSTEMDESC *PRTFILESYSTEMDESC;
typedef const RTFILESYSTEMDESC *PCRTFILESYSTEMDESC;

extern DECLHIDDEN(RTFILESYSTEMDESC const) g_rtFsExt;

RT_C_DECLS_END

#endif /* !__internal_filesystem_h */

