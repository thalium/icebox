/* $Id: dir2.cpp $ */
/** @file
 * IPRT - Directory Manipulation, Part 2.
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
#define LOG_GROUP RTLOGGROUP_DIR
#include <iprt/dir.h>
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/file.h>
#include <iprt/err.h>
#include <iprt/log.h>
#include <iprt/mem.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include "internal/path.h"


/**
 * Recursion worker for RTDirRemoveRecursive.
 *
 * @returns IPRT status code.
 * @param   pszBuf              The path buffer.  Contains the abs path to the
 *                              directory to recurse into.  Trailing slash.
 * @param   cchDir              The length of the directory we're cursing into,
 *                              including the trailing slash.
 * @param   pDirEntry           The dir entry buffer.  (Shared to save stack.)
 * @param   pObjInfo            The object info buffer.  (ditto)
 */
static int rtDirRemoveRecursiveSub(char *pszBuf, size_t cchDir, PRTDIRENTRY pDirEntry, PRTFSOBJINFO pObjInfo)
{
    AssertReturn(RTPATH_IS_SLASH(pszBuf[cchDir - 1]), VERR_INTERNAL_ERROR_4);

    /*
     * Enumerate the directory content and dispose of it.
     */
    RTDIR hDir;
    int rc = RTDirOpen(&hDir, pszBuf);
    if (RT_FAILURE(rc))
        return rc;
    while (RT_SUCCESS(rc = RTDirRead(hDir, pDirEntry, NULL)))
    {
        if (!RTDirEntryIsStdDotLink(pDirEntry))
        {
            /* Construct the full name of the entry. */
            if (cchDir + pDirEntry->cbName + 1 /* dir slash */ >= RTPATH_MAX)
            {
                rc = VERR_FILENAME_TOO_LONG;
                break;
            }
            memcpy(&pszBuf[cchDir], pDirEntry->szName, pDirEntry->cbName + 1);

            /* Deal with the unknown type. */
            if (pDirEntry->enmType == RTDIRENTRYTYPE_UNKNOWN)
            {
                rc = RTPathQueryInfoEx(pszBuf, pObjInfo, RTFSOBJATTRADD_NOTHING, RTPATH_F_ON_LINK);
                if (RT_SUCCESS(rc) && RTFS_IS_DIRECTORY(pObjInfo->Attr.fMode))
                    pDirEntry->enmType = RTDIRENTRYTYPE_DIRECTORY;
                else if (RT_SUCCESS(rc) && RTFS_IS_FILE(pObjInfo->Attr.fMode))
                    pDirEntry->enmType = RTDIRENTRYTYPE_FILE;
                else if (RT_SUCCESS(rc) && RTFS_IS_SYMLINK(pObjInfo->Attr.fMode))
                    pDirEntry->enmType = RTDIRENTRYTYPE_SYMLINK;
            }

            /* Try the delete the fs object. */
            switch (pDirEntry->enmType)
            {
                case RTDIRENTRYTYPE_FILE:
                    rc = RTFileDelete(pszBuf);
                    break;

                case RTDIRENTRYTYPE_DIRECTORY:
                {
                    size_t cchSubDir = cchDir + pDirEntry->cbName;
                    pszBuf[cchSubDir++] = '/';
                    pszBuf[cchSubDir]   = '\0';
                    rc = rtDirRemoveRecursiveSub(pszBuf, cchSubDir, pDirEntry, pObjInfo);
                    if (RT_SUCCESS(rc))
                    {
                        pszBuf[cchSubDir] = '\0';
                        rc = RTDirRemove(pszBuf);
                    }
                    break;
                }

                //case RTDIRENTRYTYPE_SYMLINK:
                //    rc = RTSymlinkDelete(pszBuf, 0);
                //    break;

                default:
                    /** @todo not implemented yet. */
                    rc = VINF_SUCCESS;
                    break;
            }
            if (RT_FAILURE(rc))
               break;
        }
    }
    if (rc == VERR_NO_MORE_FILES)
        rc = VINF_SUCCESS;
    RTDirClose(hDir);
    return rc;
}


RTDECL(int) RTDirRemoveRecursive(const char *pszPath, uint32_t fFlags)
{
    AssertReturn(!(fFlags & ~RTDIRRMREC_F_VALID_MASK), VERR_INVALID_PARAMETER);

    /* Get an absolute path because this is easier to work with. */
    /** @todo use RTPathReal here instead? */
    char szAbsPath[RTPATH_MAX];
    int rc = RTPathAbs(pszPath, szAbsPath, sizeof(szAbsPath));
    if (RT_FAILURE(rc))
        return rc;

    /* This API is not permitted applied to the root of anything. */
    if (RTPathCountComponents(szAbsPath) <= 1)
        return VERR_ACCESS_DENIED;

    /* Because of the above restriction, we never have to deal with the root
       slash problem and can safely strip any trailing slashes and add a
       definite one. */
    RTPathStripTrailingSlash(szAbsPath);
    size_t cchAbsPath = strlen(szAbsPath);
    if (cchAbsPath + 1 >= RTPATH_MAX)
        return VERR_FILENAME_TOO_LONG;
    szAbsPath[cchAbsPath++] = '/';
    szAbsPath[cchAbsPath] = 0;

    /* Check if it exists so we can return quietly if it doesn't. */
    RTFSOBJINFO SharedObjInfoBuf;
    rc = RTPathQueryInfoEx(szAbsPath, &SharedObjInfoBuf, RTFSOBJATTRADD_NOTHING, RTPATH_F_ON_LINK);
    if (   rc == VERR_PATH_NOT_FOUND
        || rc == VERR_FILE_NOT_FOUND)
        return VINF_SUCCESS;
    if (RT_FAILURE(rc))
        return rc;
    if (!RTFS_IS_DIRECTORY(SharedObjInfoBuf.Attr.fMode))
        return VERR_NOT_A_DIRECTORY;

    /* We're all set for the recursion now, so get going. */
    RTDIRENTRY  SharedDirEntryBuf;
    rc = rtDirRemoveRecursiveSub(szAbsPath, cchAbsPath, &SharedDirEntryBuf, &SharedObjInfoBuf);

    /* Remove the specified directory if desired and removing the content was
       successful. */
    if (   RT_SUCCESS(rc)
        && !(fFlags & RTDIRRMREC_F_CONTENT_ONLY))
    {
        szAbsPath[cchAbsPath] = 0;
        rc = RTDirRemove(szAbsPath);
    }
    return rc;
}

