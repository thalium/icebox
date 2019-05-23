/** @file
 * IPRT Filesystem API.
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

#ifndef ___iprt_filesystem_h
#define ___iprt_filesystem_h

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/vfs.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_filesystem           IPRT Filesystem VFS
 *
 * @todo r=bird: WRONG WRONG WRONG FILE. We already have a file system API in
 * IPRT, it is RTFs*, see @ref grp_rt_fs.  NOBODY ADDS ANY NEW APIS HERE!!
 *
 * @{
 */

/**
 * Detect the filesystem in the image given by the VFS file handle
 * and create a new VFS object.
 *
 * @returns IPRT status code.
 * @retval VERR_NOT_SUPPORTED if the filesystem is not recognized.
 * @param  hVfsFile    The file to use as the filesystem medium.
 * @param  phVfs       Where to store the VFS handle on success.
 */
RTDECL(int) RTFilesystemVfsFromFile(RTVFSFILE hVfsFile, PRTVFS phVfs);

/** @} */

RT_C_DECLS_END

#endif /* !___iprt_filesystem_h */

