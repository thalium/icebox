/* $Id: dir-win.cpp $ */
/** @file
 * IPRT - Directory, Windows.
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
#include <iprt/win/windows.h>

#include <iprt/dir.h>
#include <iprt/path.h>
#include <iprt/mem.h>
#include <iprt/string.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/log.h>
#include "internal/fs.h"
#include "internal/path.h"



RTDECL(int) RTDirCreate(const char *pszPath, RTFMODE fMode, uint32_t fCreate)
{
    /*
     * Validate the file mode.
     */
    int rc;
    fMode = rtFsModeNormalize(fMode, pszPath, 0);
    if (rtFsModeIsValidPermissions(fMode))
    {
        /*
         * Convert to UTF-16.
         */
        PRTUTF16 pwszString;
        rc = RTStrToUtf16(pszPath, &pwszString);
        AssertRC(rc);
        if (RT_SUCCESS(rc))
        {
            /*
             * Create the directory.
             */
            if (CreateDirectoryW((LPCWSTR)pwszString, NULL))
                rc = VINF_SUCCESS;
            else
                rc = RTErrConvertFromWin32(GetLastError());

            /*
             * Turn off indexing of directory through Windows Indexing Service
             */
            /** @todo This FILE_ATTRIBUTE_NOT_CONTENT_INDEXED hack (for .VDI files,
             *        really) may cause failures on samba shares.  That really sweet and
             *        need to be addressed differently.  We shouldn't be doing this
             *        unless the caller actually asks for it, must less returning failure,
             *        for crying out loud!  This is only important a couple of places in
             *        main, if important is the right way to put it... */
            if (   RT_SUCCESS(rc)
                && !(fCreate & RTDIRCREATE_FLAGS_NOT_CONTENT_INDEXED_DONT_SET))
            {
                if (   SetFileAttributesW((LPCWSTR)pwszString, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)
                    || (fCreate & RTDIRCREATE_FLAGS_NOT_CONTENT_INDEXED_NOT_CRITICAL) )
                    rc = VINF_SUCCESS;
                else
                    rc = RTErrConvertFromWin32(GetLastError());
            }

            RTUtf16Free(pwszString);
        }
    }
    else
    {
        AssertMsgFailed(("Invalid file mode! %RTfmode\n", fMode));
        rc = VERR_INVALID_FMODE;
    }

    LogFlow(("RTDirCreate(%p:{%s}, %RTfmode): returns %Rrc\n", pszPath, pszPath, fMode, rc));
    return rc;
}


RTDECL(int) RTDirRemove(const char *pszPath)
{
    /*
     * Convert to UTF-16.
     */
    PRTUTF16 pwszString;
    int rc = RTStrToUtf16(pszPath, &pwszString);
    AssertRC(rc);
    if (RT_SUCCESS(rc))
    {
        /*
         * Remove the directory.
         */
        if (RemoveDirectoryW((LPCWSTR)pwszString))
            rc = VINF_SUCCESS;
        else
            rc = RTErrConvertFromWin32(GetLastError());

        RTUtf16Free(pwszString);
    }

    LogFlow(("RTDirRemove(%p:{%s}): returns %Rrc\n", pszPath, pszPath, rc));
    return rc;
}


RTDECL(int) RTDirFlush(const char *pszPath)
{
    RT_NOREF_PV(pszPath);
    return VERR_NOT_SUPPORTED;
}


RTDECL(int) RTDirRename(const char *pszSrc, const char *pszDst, unsigned fRename)
{
    /*
     * Validate input.
     */
    AssertMsgReturn(VALID_PTR(pszSrc), ("%p\n", pszSrc), VERR_INVALID_POINTER);
    AssertMsgReturn(VALID_PTR(pszDst), ("%p\n", pszDst), VERR_INVALID_POINTER);
    AssertMsgReturn(*pszSrc, ("%p\n", pszSrc), VERR_INVALID_PARAMETER);
    AssertMsgReturn(*pszDst, ("%p\n", pszDst), VERR_INVALID_PARAMETER);
    AssertMsgReturn(!(fRename & ~RTPATHRENAME_FLAGS_REPLACE), ("%#x\n", fRename), VERR_INVALID_PARAMETER);

    /*
     * Call the worker.
     */
    int rc = rtPathWin32MoveRename(pszSrc, pszDst,
                                   fRename & RTPATHRENAME_FLAGS_REPLACE ? MOVEFILE_REPLACE_EXISTING : 0,
                                   RTFS_TYPE_DIRECTORY);

    LogFlow(("RTDirRename(%p:{%s}, %p:{%s}, %#x): returns %Rrc\n", pszSrc, pszSrc, pszDst, pszDst, fRename, rc));
    return rc;
}

