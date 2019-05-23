/* $Id: filesystem.cpp $ */
/** @file
 * IPRT Filesystem API (Filesystem) - generic code.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEFAULT
#include <iprt/types.h>
#include <iprt/assert.h>
#include <iprt/mem.h>
#include <iprt/filesystem.h>
#include <iprt/err.h>
#include <iprt/asm.h>
#include <iprt/string.h>
#include <iprt/list.h>
#include "internal/filesystem.h"


/*********************************************************************************************************************************
*   Global variables                                                                                                             *
*********************************************************************************************************************************/

/**
 * Supported volume formats.
 */
static PCRTFILESYSTEMDESC g_aFsFmts[] =
{
    &g_rtFsExt
};

static int rtFsGetFormat(RTVFSFILE hVfsFile, PCRTFILESYSTEMDESC *ppFsDesc)
{
    int                   rc = VINF_SUCCESS;
    uint32_t              uScoreMax = RTFILESYSTEM_MATCH_SCORE_UNSUPPORTED;
    PCRTFILESYSTEMDESC    pFsFmtMatch = NULL;

    for (unsigned i = 0; i < RT_ELEMENTS(g_aFsFmts); i++)
    {
        uint32_t uScore;
        PCRTFILESYSTEMDESC pFsFmt = g_aFsFmts[i];

        rc = pFsFmt->pfnProbe(hVfsFile, &uScore);
        if (   RT_SUCCESS(rc)
            && uScore > uScoreMax)
        {
            pFsFmtMatch = pFsFmt;
            uScoreMax   = uScore;
        }
        else if (RT_FAILURE(rc))
            break;
    }

    if (RT_SUCCESS(rc))
    {
        if (uScoreMax > RTFILESYSTEM_MATCH_SCORE_UNSUPPORTED)
        {
            AssertPtr(pFsFmtMatch);
            *ppFsDesc = pFsFmtMatch;
        }
        else
            rc = VERR_NOT_SUPPORTED;
    }

    return rc;
}

RTDECL(int) RTFilesystemVfsFromFile(RTVFSFILE hVfsFile, PRTVFS phVfs)
{
    int rc = VINF_SUCCESS;
    PCRTFILESYSTEMDESC pFsDesc = NULL;
    RTVFS hVfs = NIL_RTVFS;
    void *pvThis = NULL;

    AssertPtrReturn(hVfsFile, VERR_INVALID_HANDLE);
    AssertPtrReturn(phVfs, VERR_INVALID_POINTER);

    rc = rtFsGetFormat(hVfsFile, &pFsDesc);
    if (RT_SUCCESS(rc))
    {
        rc = RTVfsNew(&pFsDesc->VfsOps, pFsDesc->cbFs, NIL_RTVFS, NIL_RTVFSLOCK,
                      &hVfs, &pvThis);
        if (RT_SUCCESS(rc))
        {
            rc = pFsDesc->pfnInit(pvThis, hVfsFile);
            if (RT_SUCCESS(rc))
                *phVfs = hVfs;
            else
                RTVfsRelease(hVfs);
        }
    }

    return rc;
}

