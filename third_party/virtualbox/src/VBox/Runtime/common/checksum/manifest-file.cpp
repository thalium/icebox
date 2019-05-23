/* $Id: manifest-file.cpp $ */
/** @file
 * IPRT - Manifest, the bits with file dependencies
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "internal/iprt.h"
#include <iprt/manifest.h>

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/mem.h>
#include <iprt/string.h>
#include <iprt/vfs.h>

#include "internal/magics.h"



RTDECL(int) RTManifestReadStandardFromFile(RTMANIFEST hManifest, const char *pszFilename)
{
    RTFILE      hFile;
    uint32_t    fFlags = RTFILE_O_READ | RTFILE_O_DENY_WRITE | RTFILE_O_OPEN;
    int rc = RTFileOpen(&hFile, pszFilename, fFlags);
    if (RT_SUCCESS(rc))
    {
        RTVFSIOSTREAM hVfsIos;
        rc = RTVfsIoStrmFromRTFile(hFile, fFlags, true /*fLeaveOpen*/, &hVfsIos);
        if (RT_SUCCESS(rc))
        {
            rc = RTManifestReadStandard(hManifest, hVfsIos);
            RTVfsIoStrmRelease(hVfsIos);
        }
        RTFileClose(hFile);
    }
    return rc;
}


RTDECL(int) RTManifestWriteStandardToFile(RTMANIFEST hManifest, const char *pszFilename)
{
    RTFILE      hFile;
    uint32_t    fFlags = RTFILE_O_WRITE | RTFILE_O_DENY_WRITE | RTFILE_O_CREATE_REPLACE;
    int rc = RTFileOpen(&hFile, pszFilename, fFlags);
    if (RT_SUCCESS(rc))
    {
        RTVFSIOSTREAM hVfsIos;
        rc = RTVfsIoStrmFromRTFile(hFile, fFlags, true /*fLeaveOpen*/, &hVfsIos);
        if (RT_SUCCESS(rc))
        {
            rc = RTManifestWriteStandard(hManifest, hVfsIos);
            RTVfsIoStrmRelease(hVfsIos);
        }
        RTFileClose(hFile);
    }
    return rc;
}

