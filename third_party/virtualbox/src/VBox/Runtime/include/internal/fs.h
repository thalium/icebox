/* $Id: fs.h $ */
/** @file
 * IPRT - Internal RTFs header.
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

#ifndef ___internal_fs_h
#define ___internal_fs_h

#include <iprt/types.h>
#ifndef RT_OS_WINDOWS
# include <sys/stat.h>
#endif

RT_C_DECLS_BEGIN

/** IO_REPARSE_TAG_SYMLINK */
#define RTFSMODE_SYMLINK_REPARSE_TAG UINT32_C(0xa000000c)

RTFMODE rtFsModeFromDos(RTFMODE fMode, const char *pszName, size_t cbName, uint32_t uReparseTag);
RTFMODE rtFsModeFromUnix(RTFMODE fMode, const char *pszName, size_t cbName);
RTFMODE rtFsModeNormalize(RTFMODE fMode, const char *pszName, size_t cbName);
bool    rtFsModeIsValid(RTFMODE fMode);
bool    rtFsModeIsValidPermissions(RTFMODE fMode);

#ifndef RT_OS_WINDOWS
void    rtFsConvertStatToObjInfo(PRTFSOBJINFO pObjInfo, const struct stat *pStat, const char *pszName, unsigned cbName);
void    rtFsObjInfoAttrSetUnixOwner(PRTFSOBJINFO pObjInfo, RTUID uid);
void    rtFsObjInfoAttrSetUnixGroup(PRTFSOBJINFO pObjInfo, RTUID gid);
#endif

#ifdef RT_OS_LINUX
# ifdef __USE_MISC
#  define HAVE_STAT_TIMESPEC_BRIEF
# else
#  define HAVE_STAT_NSEC
# endif
#endif

RT_C_DECLS_END

#endif
