/* $Id: RTFileQueryFsSizes-posix.cpp $ */
/** @file
 * IPRT - File I/O, RTFileFsQuerySizes, POSIX.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP RTLOGGROUP_FILE

#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/statvfs.h>

#include <iprt/file.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/log.h>
#include <iprt/string.h>


RTR3DECL(int) RTFileQueryFsSizes(RTFILE hFile, PRTFOFF pcbTotal, RTFOFF *pcbFree,
                                 uint32_t *pcbBlock, uint32_t *pcbSector)
{
    struct statvfs StatVFS;
    RT_ZERO(StatVFS);
    if (fstatvfs(RTFileToNative(hFile), &StatVFS))
        return RTErrConvertFromErrno(errno);

    /*
     * Calc the returned values.
     */
    if (pcbTotal)
        *pcbTotal = (RTFOFF)StatVFS.f_blocks * StatVFS.f_frsize;
    if (pcbFree)
        *pcbFree = (RTFOFF)StatVFS.f_bavail * StatVFS.f_frsize;
    if (pcbBlock)
        *pcbBlock = StatVFS.f_frsize;
    /* no idea how to get the sector... */
    if (pcbSector)
        *pcbSector = 512;

    return VINF_SUCCESS;
}

