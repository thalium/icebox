/* $Id: RTFileSetAllocationSize-posix.cpp $ */
/** @file
 * IPRT - RTFileSetAllocationSize, linux implementation.
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
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
#include <iprt/file.h>
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/err.h>

#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

/**
 * The posix_fallocate() method.
 */
typedef int (*PFNPOSIXFALLOCATE) (int iFd, off_t offStart, off_t cb);

RTDECL(int) RTFileSetAllocationSize(RTFILE hFile, uint64_t cbSize, uint32_t fFlags)
{
    AssertReturn(hFile != NIL_RTFILE, VERR_INVALID_PARAMETER);
    AssertReturn(!(fFlags & ~RTFILE_ALLOC_SIZE_F_VALID), VERR_INVALID_PARAMETER);
    AssertMsgReturn(sizeof(off_t) >= sizeof(cbSize) ||  RT_HIDWORD(cbSize) == 0,
                    ("64-bit filesize not supported! cbSize=%lld\n", cbSize),
                    VERR_NOT_SUPPORTED);

    if (fFlags & RTFILE_ALLOC_SIZE_F_KEEP_SIZE)
        return VERR_NOT_SUPPORTED;

    int rc = VINF_SUCCESS;
    PFNPOSIXFALLOCATE pfnPosixFAllocate = (PFNPOSIXFALLOCATE)(uintptr_t)dlsym(RTLD_DEFAULT, "posix_fallocate");
    if (VALID_PTR(pfnPosixFAllocate))
    {
        int rcPosix = pfnPosixFAllocate(RTFileToNative(hFile), 0, cbSize);
        if (rcPosix != 0)
        {
            if (errno == EOPNOTSUPP)
                rc = VERR_NOT_SUPPORTED;
            else
                rc = RTErrConvertFromErrno(errno);
        }
    }
    else
        rc = VERR_NOT_SUPPORTED;

    return rc;
}
RT_EXPORT_SYMBOL(RTFileSetAllocationSize);
