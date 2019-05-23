/* $Id: dirrel-r3-generic.cpp $ */
/** @file
 * IPRT - Directory relative base APIs, generic implementation.
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
#include <iprt/path.h>
#include <iprt/string.h>
#include <iprt/symlink.h>
#define RTDIR_AGNOSTIC
#include "internal/dir.h"



/**
 * Helper that builds a full path for a directory relative path.
 *
 * @returns IPRT status code.
 * @param   pThis               The directory.
 * @param   pszPathDst          The destination buffer.
 * @param   cbPathDst           The size of the destination buffer.
 * @param   pszRelPath          The relative path.
 */
static int rtDirRelBuildFullPath(PRTDIRINTERNAL pThis, char *pszPathDst, size_t cbPathDst, const char *pszRelPath)
{
    AssertMsgReturn(!RTPathStartsWithRoot(pszRelPath), ("pszRelPath='%s'\n", pszRelPath), VERR_PATH_IS_NOT_RELATIVE);

    /*
     * Let's hope we can avoid checking for ascension.
     *
     * Note! We don't take symbolic links into account here.  That can be
     *       done later if desired.
     */
    if (   !(pThis->fFlags & RTDIR_F_DENY_ASCENT)
        || strstr(pszRelPath, "..") == NULL)
    {
        size_t const cchRelPath = strlen(pszRelPath);
        size_t const cchDirPath = pThis->cchPath;
        if (cchDirPath + cchRelPath < cbPathDst)
        {
            memcpy(pszPathDst, pThis->pszPath, cchDirPath);
            memcpy(&pszPathDst[cchDirPath], pszRelPath, cchRelPath);
            pszPathDst[cchDirPath + cchRelPath] = '\0';
            return VINF_SUCCESS;
        }
        return VERR_FILENAME_TOO_LONG;
    }

    /*
     * Calc the absolute path using the directory as a base, then check if the result
     * still starts with the full directory path.
     *
     * This ASSUMES that pThis->pszPath is an absolute path.
     */
    int rc = RTPathAbsEx(pThis->pszPath, pszRelPath, pszPathDst, cbPathDst);
    if (RT_SUCCESS(rc))
    {
        if (RTPathStartsWith(pszPathDst, pThis->pszPath))
            return VINF_SUCCESS;
        return VERR_PATH_NOT_FOUND;
    }
    return rc;
}


/*
 *
 *
 * RTFile stuff.
 * RTFile stuff.
 * RTFile stuff.
 *
 *
 */




/**
 * Open a file relative to @a hDir.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory to open relative to.
 * @param   pszRelFilename  The relative path to the file.
 * @param   fOpen           Open flags, i.e a combination of the RTFILE_O_XXX
 *                          defines.  The ACCESS, ACTION and DENY flags are
 *                          mandatory!
 * @param   phFile          Where to store the handle to the opened file.
 *
 * @sa      RTFileOpen
 */
RTDECL(int)  RTDirRelFileOpen(RTDIR hDir, const char *pszRelFilename, uint64_t fOpen, PRTFILE phFile)
{
    PRTDIRINTERNAL pThis = hDir;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTDIR_MAGIC, VERR_INVALID_HANDLE);

    char szPath[RTPATH_MAX];
    int rc = rtDirRelBuildFullPath(pThis, szPath, sizeof(szPath), pszRelFilename);
    if (RT_SUCCESS(rc))
        rc = RTFileOpen(phFile, szPath, fOpen);
    return rc;
}



/*
 *
 *
 * RTDir stuff.
 * RTDir stuff.
 * RTDir stuff.
 *
 *
 */



/**
 * Opens a directory relative to @a hDir.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory to open relative to.
 * @param   pszDir          The relative path to the directory to open.
 * @param   phDir           Where to store the directory handle.
 *
 * @sa      RTDirOpen
 */
RTDECL(int) RTDirRelDirOpen(RTDIR hDir, const char *pszDir, RTDIR *phDir)
{
    PRTDIRINTERNAL pThis = hDir;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTDIR_MAGIC, VERR_INVALID_HANDLE);

    char szPath[RTPATH_MAX];
    int rc = rtDirRelBuildFullPath(pThis, szPath, sizeof(szPath), pszDir);
    if (RT_SUCCESS(rc))
        rc = RTDirOpen(phDir, szPath);
    return rc;

}


/**
 * Opens a directory relative to @a hDir, with flags and optional filtering.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory to open relative to.
 * @param   pszDirAndFilter The relative path to the directory to search, this
 *                          must include wildcards.
 * @param   enmFilter       The kind of filter to apply. Setting this to
 *                          RTDIRFILTER_NONE makes this function behave like
 *                          RTDirOpen.
 * @param   fFlags          Open flags, RTDIR_F_XXX.
 * @param   phDir           Where to store the directory handle.
 *
 * @sa      RTDirOpenFiltered
 */
RTDECL(int) RTDirRelDirOpenFiltered(RTDIR hDir, const char *pszDirAndFilter, RTDIRFILTER enmFilter,
                                    uint32_t fFlags, RTDIR *phDir)
{
    PRTDIRINTERNAL pThis = hDir;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTDIR_MAGIC, VERR_INVALID_HANDLE);

    char szPath[RTPATH_MAX];
    int rc = rtDirRelBuildFullPath(pThis, szPath, sizeof(szPath), pszDirAndFilter);
    if (RT_SUCCESS(rc))
        rc = RTDirOpenFiltered(phDir, szPath, enmFilter, fFlags);
    return rc;
}


/**
 * Creates a directory relative to @a hDir.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory @a pszRelPath is relative to.
 * @param   pszRelPath      The relative path to the directory to create.
 * @param   fMode           The mode of the new directory.
 * @param   fCreate         Create flags, RTDIRCREATE_FLAGS_XXX.
 * @param   phSubDir        Where to return the handle of the created directory.
 *                          Optional.
 *
 * @sa      RTDirCreate
 */
RTDECL(int) RTDirRelDirCreate(RTDIR hDir, const char *pszRelPath, RTFMODE fMode, uint32_t fCreate, RTDIR *phSubDir)
{
    PRTDIRINTERNAL pThis = hDir;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTDIR_MAGIC, VERR_INVALID_HANDLE);

    char szPath[RTPATH_MAX];
    int rc = rtDirRelBuildFullPath(pThis, szPath, sizeof(szPath), pszRelPath);
    if (RT_SUCCESS(rc))
    {
        rc = RTDirCreate(szPath, fMode, fCreate);
        if (RT_SUCCESS(rc) && phSubDir)
            rc = RTDirOpen(phSubDir, szPath);
    }
    return rc;
}


/**
 * Removes a directory relative to @a hDir if empty.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory @a pszRelPath is relative to.
 * @param   pszRelPath      The relative path to the directory to remove.
 *
 * @sa      RTDirRemove
 */
RTDECL(int) RTDirRelDirRemove(RTDIR hDir, const char *pszRelPath)
{
    PRTDIRINTERNAL pThis = hDir;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTDIR_MAGIC, VERR_INVALID_HANDLE);

    char szPath[RTPATH_MAX];
    int rc = rtDirRelBuildFullPath(pThis, szPath, sizeof(szPath), pszRelPath);
    if (RT_SUCCESS(rc))
        rc = RTDirRemove(szPath);
    return rc;
}


/*
 *
 * RTPath stuff.
 * RTPath stuff.
 * RTPath stuff.
 *
 *
 */


/**
 * Query information about a file system object relative to @a hDir.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS if the object exists, information returned.
 * @retval  VERR_PATH_NOT_FOUND if any but the last component in the specified
 *          path was not found or was not a directory.
 * @retval  VERR_FILE_NOT_FOUND if the object does not exist (but path to the
 *          parent directory exists).
 *
 * @param   hDir            The directory @a pszRelPath is relative to.
 * @param   pszRelPath      The relative path to the file system object.
 * @param   pObjInfo        Object information structure to be filled on successful
 *                          return.
 * @param   enmAddAttr      Which set of additional attributes to request.
 *                          Use RTFSOBJATTRADD_NOTHING if this doesn't matter.
 * @param   fFlags          RTPATH_F_ON_LINK or RTPATH_F_FOLLOW_LINK.
 *
 * @sa      RTPathQueryInfoEx
 */
RTDECL(int) RTDirRelPathQueryInfo(RTDIR hDir, const char *pszRelPath, PRTFSOBJINFO pObjInfo,
                                  RTFSOBJATTRADD enmAddAttr, uint32_t fFlags)
{
    PRTDIRINTERNAL pThis = hDir;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTDIR_MAGIC, VERR_INVALID_HANDLE);

    char szPath[RTPATH_MAX];
    int rc = rtDirRelBuildFullPath(pThis, szPath, sizeof(szPath), pszRelPath);
    if (RT_SUCCESS(rc))
        rc = RTPathQueryInfoEx(szPath, pObjInfo, enmAddAttr, fFlags);
    return rc;
}


/**
 * Changes the mode flags of a file system object relative to @a hDir.
 *
 * The API requires at least one of the mode flag sets (Unix/Dos) to
 * be set. The type is ignored.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory @a pszRelPath is relative to.
 * @param   pszRelPath      The relative path to the file system object.
 * @param   fMode           The new file mode, see @ref grp_rt_fs for details.
 * @param   fFlags          RTPATH_F_ON_LINK or RTPATH_F_FOLLOW_LINK.
 *
 * @sa      RTPathSetMode
 */
RTDECL(int) RTDirRelPathSetMode(RTDIR hDir, const char *pszRelPath, RTFMODE fMode, uint32_t fFlags)
{
    PRTDIRINTERNAL pThis = hDir;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTDIR_MAGIC, VERR_INVALID_HANDLE);
    AssertMsgReturn(RTPATH_F_IS_VALID(fFlags, 0), ("%#x\n", fFlags), VERR_INVALID_FLAGS);

    char szPath[RTPATH_MAX];
    int rc = rtDirRelBuildFullPath(pThis, szPath, sizeof(szPath), pszRelPath);
    if (RT_SUCCESS(rc))
    {
#ifndef RT_OS_WINDOWS
        rc = RTPathSetMode(szPath, fMode); /** @todo fFlags is currently ignored. */
#else
        rc = VERR_NOT_IMPLEMENTED; /** @todo implement RTPathSetMode on windows. */
        RT_NOREF(fMode);
#endif
    }
    return rc;
}


/**
 * Changes one or more of the timestamps associated of file system object
 * relative to @a hDir.
 *
 * @returns IPRT status code.
 * @param   hDir                The directory @a pszRelPath is relative to.
 * @param   pszRelPath          The relative path to the file system object.
 * @param   pAccessTime         Pointer to the new access time.
 * @param   pModificationTime   Pointer to the new modification time.
 * @param   pChangeTime         Pointer to the new change time. NULL if not to be changed.
 * @param   pBirthTime          Pointer to the new time of birth. NULL if not to be changed.
 * @param   fFlags              RTPATH_F_ON_LINK or RTPATH_F_FOLLOW_LINK.
 *
 * @remark  The file system might not implement all these time attributes,
 *          the API will ignore the ones which aren't supported.
 *
 * @remark  The file system might not implement the time resolution
 *          employed by this interface, the time will be chopped to fit.
 *
 * @remark  The file system may update the change time even if it's
 *          not specified.
 *
 * @remark  POSIX can only set Access & Modification and will always set both.
 *
 * @sa      RTPathSetTimesEx
 */
RTDECL(int) RTDirRelPathSetTimes(RTDIR hDir, const char *pszRelPath, PCRTTIMESPEC pAccessTime, PCRTTIMESPEC pModificationTime,
                                 PCRTTIMESPEC pChangeTime, PCRTTIMESPEC pBirthTime, uint32_t fFlags)
{
    PRTDIRINTERNAL pThis = hDir;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTDIR_MAGIC, VERR_INVALID_HANDLE);

    char szPath[RTPATH_MAX];
    int rc = rtDirRelBuildFullPath(pThis, szPath, sizeof(szPath), pszRelPath);
    if (RT_SUCCESS(rc))
        rc = RTPathSetTimesEx(szPath, pAccessTime, pModificationTime, pChangeTime, pBirthTime, fFlags);
    return rc;
}


/**
 * Changes the owner and/or group of a file system object relative to @a hDir.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory @a pszRelPath is relative to.
 * @param   pszRelPath      The relative path to the file system object.
 * @param   uid             The new file owner user id.  Pass NIL_RTUID to leave
 *                          this unchanged.
 * @param   gid             The new group id.  Pass NIL_RTGID to leave this
 *                          unchanged.
 * @param   fFlags          RTPATH_F_ON_LINK or RTPATH_F_FOLLOW_LINK.
 *
 * @sa      RTPathSetOwnerEx
 */
RTDECL(int) RTDirRelPathSetOwner(RTDIR hDir, const char *pszRelPath, uint32_t uid, uint32_t gid, uint32_t fFlags)
{
    PRTDIRINTERNAL pThis = hDir;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTDIR_MAGIC, VERR_INVALID_HANDLE);

    char szPath[RTPATH_MAX];
    int rc = rtDirRelBuildFullPath(pThis, szPath, sizeof(szPath), pszRelPath);
    if (RT_SUCCESS(rc))
    {
#ifndef RT_OS_WINDOWS
        rc = RTPathSetOwnerEx(szPath, uid, gid, fFlags);
#else
        rc = VERR_NOT_IMPLEMENTED;
        RT_NOREF(uid, gid, fFlags);
#endif
    }
    return rc;
}


/**
 * Renames a directory relative path within a filesystem.
 *
 * This will rename symbolic links.  If RTPATHRENAME_FLAGS_REPLACE is used and
 * pszDst is a symbolic link, it will be replaced and not its target.
 *
 * @returns IPRT status code.
 * @param   hDirSrc         The directory the source path is relative to.
 * @param   pszSrc          The source path, relative to @a hDirSrc.
 * @param   hDirDst         The directory the destination path is relative to.
 * @param   pszDst          The destination path, relative to @a hDirDst.
 * @param   fRename         Rename flags, RTPATHRENAME_FLAGS_XXX.
 *
 * @sa      RTPathRename
 */
RTDECL(int) RTDirRelPathRename(RTDIR hDirSrc, const char *pszSrc, RTDIR hDirDst, const char *pszDst, unsigned fRename)
{
    PRTDIRINTERNAL pThis = hDirSrc;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTDIR_MAGIC, VERR_INVALID_HANDLE);

    PRTDIRINTERNAL pThat = hDirDst;
    if (pThat != pThis)
    {
        AssertPtrReturn(pThat, VERR_INVALID_HANDLE);
        AssertReturn(pThat->u32Magic != RTDIR_MAGIC, VERR_INVALID_HANDLE);
    }

    char szSrcPath[RTPATH_MAX];
    int rc = rtDirRelBuildFullPath(pThis, szSrcPath, sizeof(szSrcPath), pszSrc);
    if (RT_SUCCESS(rc))
    {
        char szDstPath[RTPATH_MAX];
        rc = rtDirRelBuildFullPath(pThis, szDstPath, sizeof(szDstPath), pszDst);
        if (RT_SUCCESS(rc))
            rc = RTPathRename(szSrcPath, szDstPath, fRename);
    }
    return rc;
}


/**
 * Removes the last component of the directory relative path.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory @a pszRelPath is relative to.
 * @param   pszRelPath      The relative path to the file system object.
 * @param   fUnlink         Unlink flags, RTPATHUNLINK_FLAGS_XXX.
 *
 * @sa      RTPathUnlink
 */
RTDECL(int) RTDirRelPathUnlink(RTDIR hDir, const char *pszRelPath, uint32_t fUnlink)
{
    PRTDIRINTERNAL pThis = hDir;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTDIR_MAGIC, VERR_INVALID_HANDLE);

    char szPath[RTPATH_MAX];
    int rc = rtDirRelBuildFullPath(pThis, szPath, sizeof(szPath), pszRelPath);
    if (RT_SUCCESS(rc))
        rc = RTPathUnlink(szPath, fUnlink);
    return rc;
}


/*
 *
 * RTSymlink stuff.
 * RTSymlink stuff.
 * RTSymlink stuff.
 *
 *
 */


/**
 * Creates a symbolic link (@a pszSymlink) relative to @a hDir targeting @a
 * pszTarget.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory @a pszSymlink is relative to.
 * @param   pszSymlink      The relative path of the symbolic link.
 * @param   pszTarget       The path to the symbolic link target.  This is
 *                          relative to @a pszSymlink or an absolute path.
 * @param   enmType         The symbolic link type.  For Windows compatability
 *                          it is very important to set this correctly.  When
 *                          RTSYMLINKTYPE_UNKNOWN is used, the API will try
 *                          make a guess and may attempt query information
 *                          about @a pszTarget in the process.
 * @param   fCreate         Create flags, RTSYMLINKCREATE_FLAGS_XXX.
 *
 * @sa      RTSymlinkCreate
 */
RTDECL(int) RTDirRelSymlinkCreate(RTDIR hDir, const char *pszSymlink, const char *pszTarget,
                                  RTSYMLINKTYPE enmType, uint32_t fCreate)
{
    PRTDIRINTERNAL pThis = hDir;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTDIR_MAGIC, VERR_INVALID_HANDLE);

    char szPath[RTPATH_MAX];
    int rc = rtDirRelBuildFullPath(pThis, szPath, sizeof(szPath), pszSymlink);
    if (RT_SUCCESS(rc))
        rc = RTSymlinkCreate(szPath, pszTarget, enmType, fCreate);
    return rc;
}


/**
 * Read the symlink target relative to @a hDir.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SYMLINK if @a pszSymlink does not specify a symbolic link.
 * @retval  VERR_BUFFER_OVERFLOW if the link is larger than @a cbTarget.  The
 *          buffer will contain what all we managed to read, fully terminated
 *          if @a cbTarget > 0.
 *
 * @param   hDir            The directory @a pszSymlink is relative to.
 * @param   pszSymlink      The relative path to the symbolic link that should
 *                          be read.
 * @param   pszTarget       The target buffer.
 * @param   cbTarget        The size of the target buffer.
 * @param   fRead           Read flags, RTSYMLINKREAD_FLAGS_XXX.
 *
 * @sa      RTSymlinkRead
 */
RTDECL(int) RTDirRelSymlinkRead(RTDIR hDir, const char *pszSymlink, char *pszTarget, size_t cbTarget, uint32_t fRead)
{
    PRTDIRINTERNAL pThis = hDir;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTDIR_MAGIC, VERR_INVALID_HANDLE);

    char szPath[RTPATH_MAX];
    int rc = rtDirRelBuildFullPath(pThis, szPath, sizeof(szPath), pszSymlink);
    if (RT_SUCCESS(rc))
        rc = RTSymlinkRead(szPath, pszTarget, cbTarget, fRead);
    return rc;
}

