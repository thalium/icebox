/* $Id: RTFileReadAllEx-generic.cpp $ */
/** @file
 * IPRT - RTFileReadAllEx, generic implementation.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
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
#include <iprt/file.h>
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/err.h>


RTDECL(int) RTFileReadAllEx(const char *pszFilename, RTFOFF off, RTFOFF cbMax, uint32_t fFlags, void **ppvFile, size_t *pcbFile)
{
    AssertReturn(!(fFlags & ~RTFILE_RDALL_VALID_MASK), VERR_INVALID_PARAMETER);

    RTFILE File;
    int rc = RTFileOpen(&File, pszFilename, RTFILE_O_READ | RTFILE_O_OPEN | (fFlags & RTFILE_RDALL_O_DENY_MASK));
    if (RT_SUCCESS(rc))
    {
        rc = RTFileReadAllByHandleEx(File, off, cbMax, fFlags, ppvFile, pcbFile);
        RTFileClose(File);
    }
    return rc;
}
RT_EXPORT_SYMBOL(RTFileReadAllEx);

