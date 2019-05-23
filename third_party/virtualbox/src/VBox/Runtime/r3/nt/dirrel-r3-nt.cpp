/* $Id: dirrel-r3-nt.cpp $ */
/** @file
 * IPRT - Directory relative base APIs, NT implementation
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
#include "internal-r3-nt.h"

#include <iprt/assert.h>
#include <iprt/file.h>
#include <iprt/err.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include <iprt/symlink.h>
#include "internal/dir.h"
#include "internal/file.h"
#include "internal/fs.h"
#include "internal/path.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** Getst the RTNTPATHRELATIVEASCENT value for RTNtPathRelativeFromUtf8. */
#define RTDIRREL_NT_GET_ASCENT(a_pThis) \
    ( !(pThis->fFlags & RTDIR_F_DENY_ASCENT) ? kRTNtPathRelativeAscent_Allow : kRTNtPathRelativeAscent_Fail )



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


RTDECL(int)  RTDirRelFileOpen(RTDIR hDir, const char *pszRelFilename, uint64_t fOpen, PRTFILE phFile)
{
    PRTDIRINTERNAL pThis = hDir;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTDIR_MAGIC, VERR_INVALID_HANDLE);

    /*
     * Validate and convert flags.
     */
    uint32_t    fDesiredAccess;
    uint32_t    fObjAttribs;
    uint32_t    fFileAttribs;
    uint32_t    fShareAccess;
    uint32_t    fCreateDisposition;
    uint32_t    fCreateOptions;
    int rc = rtFileNtValidateAndConvertFlags(fOpen, &fDesiredAccess, &fObjAttribs, &fFileAttribs,
                                             &fShareAccess, &fCreateDisposition, &fCreateOptions);
    if (RT_SUCCESS(rc))
    {
        /*
         * Convert and normalize the path.
         */
        UNICODE_STRING NtName;
        HANDLE hRoot = pThis->hDir;
        rc = RTNtPathRelativeFromUtf8(&NtName, &hRoot, pszRelFilename, RTDIRREL_NT_GET_ASCENT(pThis),
                                      pThis->enmInfoClass == FileMaximumInformation);
        if (RT_SUCCESS(rc))
        {
            HANDLE              hFile = RTNT_INVALID_HANDLE_VALUE;
            IO_STATUS_BLOCK     Ios   = RTNT_IO_STATUS_BLOCK_INITIALIZER;
            OBJECT_ATTRIBUTES   ObjAttr;
            InitializeObjectAttributes(&ObjAttr, &NtName, fObjAttribs, hRoot, NULL /*pSecDesc*/);

            NTSTATUS rcNt = NtCreateFile(&hFile,
                                         fDesiredAccess,
                                         &ObjAttr,
                                         &Ios,
                                         NULL /* AllocationSize*/,
                                         fFileAttribs,
                                         fShareAccess,
                                         fCreateDisposition,
                                         fCreateOptions,
                                         NULL /*EaBuffer*/,
                                         0 /*EaLength*/);
            if (NT_SUCCESS(rcNt))
            {
                rc = RTFileFromNative(phFile, (uintptr_t)hFile);
                if (RT_FAILURE(rc))
                    NtClose(hFile);
            }
            else
                rc = RTErrConvertFromNtStatus(rcNt);
            RTNtPathFree(&NtName, NULL);
        }
    }
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



RTDECL(int) RTDirRelDirOpen(RTDIR hDir, const char *pszDir, RTDIR *phDir)
{
    return RTDirRelDirOpenFiltered(hDir, pszDir, RTDIRFILTER_NONE, 0 /*fFlags*/, phDir);
}


RTDECL(int) RTDirRelDirOpenFiltered(RTDIR hDir, const char *pszDirAndFilter, RTDIRFILTER enmFilter,
                                    uint32_t fFlags, RTDIR *phDir)
{
    PRTDIRINTERNAL pThis = hDir;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTDIR_MAGIC, VERR_INVALID_HANDLE);

    /*
     * Convert and normalize the path.
     */
    UNICODE_STRING NtName;
    HANDLE hRoot = pThis->hDir;
    int rc = RTNtPathRelativeFromUtf8(&NtName, &hRoot, pszDirAndFilter, RTDIRREL_NT_GET_ASCENT(pThis),
                                      pThis->enmInfoClass == FileMaximumInformation);
    if (RT_SUCCESS(rc))
    {
        rc = rtDirOpenRelativeOrHandle(phDir, pszDirAndFilter, enmFilter, fFlags, (uintptr_t)hRoot, &NtName);
        RTNtPathFree(&NtName, NULL);
    }
    return rc;
}


RTDECL(int) RTDirRelDirCreate(RTDIR hDir, const char *pszRelPath, RTFMODE fMode, uint32_t fCreate, RTDIR *phSubDir)
{
    PRTDIRINTERNAL pThis = hDir;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTDIR_MAGIC, VERR_INVALID_HANDLE);
    AssertReturn(!(fCreate & ~RTDIRCREATE_FLAGS_VALID_MASK), VERR_INVALID_FLAGS);
    fMode = rtFsModeNormalize(fMode, pszRelPath, 0);
    AssertReturn(rtFsModeIsValidPermissions(fMode), VERR_INVALID_FMODE);
    AssertPtrNullReturn(phSubDir, VERR_INVALID_POINTER);

    /*
     * Convert and normalize the path.
     */
    UNICODE_STRING NtName;
    HANDLE hRoot = pThis->hDir;
    int rc = RTNtPathRelativeFromUtf8(&NtName, &hRoot, pszRelPath, RTDIRREL_NT_GET_ASCENT(pThis),
                                      pThis->enmInfoClass == FileMaximumInformation);
    if (RT_SUCCESS(rc))
    {
        HANDLE              hNewDir = RTNT_INVALID_HANDLE_VALUE;
        IO_STATUS_BLOCK     Ios     = RTNT_IO_STATUS_BLOCK_INITIALIZER;
        OBJECT_ATTRIBUTES   ObjAttr;
        InitializeObjectAttributes(&ObjAttr, &NtName, 0 /*fAttrib*/, hRoot, NULL);

        ULONG fDirAttribs = (fCreate & RTFS_DOS_MASK_NT) >> RTFS_DOS_SHIFT;
        if (!(fCreate & RTDIRCREATE_FLAGS_NOT_CONTENT_INDEXED_DONT_SET))
            fDirAttribs |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
        if (!fDirAttribs)
            fDirAttribs = FILE_ATTRIBUTE_NORMAL;

        NTSTATUS rcNt = NtCreateFile(&hNewDir,
                                     phSubDir
                                     ? FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES | FILE_LIST_DIRECTORY | FILE_TRAVERSE | SYNCHRONIZE
                                     : SYNCHRONIZE,
                                     &ObjAttr,
                                     &Ios,
                                     NULL /*AllocationSize*/,
                                     fDirAttribs,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     FILE_CREATE,
                                     FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT,
                                     NULL /*EaBuffer*/,
                                     0 /*EaLength*/);

        /* Just in case someone takes offence at FILE_ATTRIBUTE_NOT_CONTENT_INDEXED. */
        if (   (   rcNt == STATUS_INVALID_PARAMETER
                || rcNt == STATUS_INVALID_PARAMETER_7)
            && (fDirAttribs & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)
            && (fCreate & RTDIRCREATE_FLAGS_NOT_CONTENT_INDEXED_NOT_CRITICAL) )
        {
            fDirAttribs &= ~FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
            if (!fDirAttribs)
                fDirAttribs = FILE_ATTRIBUTE_NORMAL;
            rcNt = NtCreateFile(&hNewDir,
                                phSubDir
                                ? FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES | FILE_LIST_DIRECTORY | FILE_TRAVERSE | SYNCHRONIZE
                                : SYNCHRONIZE,
                                &ObjAttr,
                                &Ios,
                                NULL /*AllocationSize*/,
                                fDirAttribs,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                FILE_CREATE,
                                FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT,
                                NULL /*EaBuffer*/,
                                0 /*EaLength*/);
        }

        if (NT_SUCCESS(rcNt))
        {
            if (!phSubDir)
            {
                NtClose(hNewDir);
                rc = VINF_SUCCESS;
            }
            else
            {
                rc = rtDirOpenRelativeOrHandle(phSubDir, pszRelPath, RTDIRFILTER_NONE, 0 /*fFlags*/,
                                               (uintptr_t)hNewDir, NULL /*pvNativeRelative*/);
                if (RT_FAILURE(rc))
                    NtClose(hNewDir);
            }
        }
        else
            rc = RTErrConvertFromNtStatus(rcNt);
        RTNtPathFree(&NtName, NULL);
    }
    return rc;
}


RTDECL(int) RTDirRelDirRemove(RTDIR hDir, const char *pszRelPath)
{
    PRTDIRINTERNAL pThis = hDir;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTDIR_MAGIC, VERR_INVALID_HANDLE);

    /*
     * Convert and normalize the path.
     */
    UNICODE_STRING NtName;
    HANDLE hRoot = pThis->hDir;
    int rc = RTNtPathRelativeFromUtf8(&NtName, &hRoot, pszRelPath, RTDIRREL_NT_GET_ASCENT(pThis),
                                      pThis->enmInfoClass == FileMaximumInformation);
    if (RT_SUCCESS(rc))
    {
        HANDLE              hSubDir = RTNT_INVALID_HANDLE_VALUE;
        IO_STATUS_BLOCK     Ios     = RTNT_IO_STATUS_BLOCK_INITIALIZER;
        OBJECT_ATTRIBUTES   ObjAttr;
        InitializeObjectAttributes(&ObjAttr, &NtName, 0 /*fAttrib*/, hRoot, NULL);

        NTSTATUS rcNt = NtCreateFile(&hSubDir,
                                     DELETE | SYNCHRONIZE,
                                     &ObjAttr,
                                     &Ios,
                                     NULL /*AllocationSize*/,
                                     FILE_ATTRIBUTE_NORMAL,
                                     FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     FILE_OPEN,
                                     FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT,
                                     NULL /*EaBuffer*/,
                                     0 /*EaLength*/);
        if (NT_SUCCESS(rcNt))
        {
            FILE_DISPOSITION_INFORMATION DispInfo;
            DispInfo.DeleteFile = TRUE;
            RTNT_IO_STATUS_BLOCK_REINIT(&Ios);
            rcNt = NtSetInformationFile(hSubDir, &Ios, &DispInfo, sizeof(DispInfo), FileDispositionInformation);

            NTSTATUS rcNt2 = NtClose(hSubDir);
            if (!NT_SUCCESS(rcNt2) && NT_SUCCESS(rcNt))
                rcNt = rcNt2;
        }

        if (NT_SUCCESS(rcNt))
            rc = VINF_SUCCESS;
        else
            rc = RTErrConvertFromNtStatus(rcNt);

        RTNtPathFree(&NtName, NULL);
    }
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


RTDECL(int) RTDirRelPathQueryInfo(RTDIR hDir, const char *pszRelPath, PRTFSOBJINFO pObjInfo,
                                  RTFSOBJATTRADD enmAddAttr, uint32_t fFlags)
{
    PRTDIRINTERNAL pThis = hDir;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTDIR_MAGIC, VERR_INVALID_HANDLE);

    /*
     * Validate and convert flags.
     */
    UNICODE_STRING NtName;
    HANDLE hRoot = pThis->hDir;
    int rc = RTNtPathRelativeFromUtf8(&NtName, &hRoot, pszRelPath, RTDIRREL_NT_GET_ASCENT(pThis),
                                      pThis->enmInfoClass == FileMaximumInformation);
    if (RT_SUCCESS(rc))
    {
        rc = rtPathNtQueryInfoWorker(hRoot, &NtName, pObjInfo, enmAddAttr, fFlags, pszRelPath);
        RTNtPathFree(&NtName, NULL);
    }
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
    fMode = rtFsModeNormalize(fMode, pszRelPath, 0);
    AssertReturn(rtFsModeIsValidPermissions(fMode), VERR_INVALID_FMODE);
    AssertMsgReturn(RTPATH_F_IS_VALID(fFlags, 0), ("%#x\n", fFlags), VERR_INVALID_FLAGS);

    /*
     * Convert and normalize the path.
     */
    UNICODE_STRING NtName;
    HANDLE hRoot = pThis->hDir;
    int rc = RTNtPathRelativeFromUtf8(&NtName, &hRoot, pszRelPath, RTDIRREL_NT_GET_ASCENT(pThis),
                                      pThis->enmInfoClass == FileMaximumInformation);
    if (RT_SUCCESS(rc))
    {
        HANDLE              hSubDir = RTNT_INVALID_HANDLE_VALUE;
        IO_STATUS_BLOCK     Ios     = RTNT_IO_STATUS_BLOCK_INITIALIZER;
        OBJECT_ATTRIBUTES   ObjAttr;
        InitializeObjectAttributes(&ObjAttr, &NtName, 0 /*fAttrib*/, hRoot, NULL);

        ULONG fOpenOptions = FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT;
        if (fFlags & RTPATH_F_ON_LINK)
            fOpenOptions |= FILE_OPEN_REPARSE_POINT;
        NTSTATUS rcNt = NtCreateFile(&hSubDir,
                                     FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                                     &ObjAttr,
                                     &Ios,
                                     NULL /*AllocationSize*/,
                                     FILE_ATTRIBUTE_NORMAL,
                                     FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     FILE_OPEN,
                                     fOpenOptions,
                                     NULL /*EaBuffer*/,
                                     0 /*EaLength*/);
        if (NT_SUCCESS(rcNt))
        {
            rc = rtNtFileSetModeWorker(hSubDir, fMode);

            rcNt = NtClose(hSubDir);
            if (!NT_SUCCESS(rcNt) && RT_SUCCESS(rc))
                rc = RTErrConvertFromNtStatus(rcNt);
        }
        else
            rc = RTErrConvertFromNtStatus(rcNt);

        RTNtPathFree(&NtName, NULL);
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
    {
RTAssertMsg2("DBG: RTDirRelPathSetTimes(%s)...\n", szPath);
        rc = RTPathSetTimesEx(szPath, pAccessTime, pModificationTime, pChangeTime, pBirthTime, fFlags);
    }
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
RTAssertMsg2("DBG: RTDirRelPathSetOwner(%s)...\n", szPath);
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
 * @param   hDirSrc         The directory the destination path is relative to.
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
        {
RTAssertMsg2("DBG: RTDirRelPathRename(%s,%s)...\n", szSrcPath, szDstPath);
            rc = RTPathRename(szSrcPath, szDstPath, fRename);
        }
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
    {
RTAssertMsg2("DBG: RTDirRelPathUnlink(%s)...\n", szPath);
        rc = RTPathUnlink(szPath, fUnlink);
    }
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
    {
RTAssertMsg2("DBG: RTDirRelSymlinkCreate(%s)...\n", szPath);
        rc = RTSymlinkCreate(szPath, pszTarget, enmType, fCreate);
    }
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
    {
RTAssertMsg2("DBG: RTDirRelSymlinkRead(%s)...\n", szPath);
        rc = RTSymlinkRead(szPath, pszTarget, cbTarget, fRead);
    }
    return rc;
}

