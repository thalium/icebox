/* $Id: RTPathQueryInfo-nt.cpp $ */
/** @file
 * IPRT - RTPathQueryInfo[Ex], Native NT.
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
#include "internal-r3-nt.h"

#include <iprt/path.h>
#include <iprt/err.h>
#include <iprt/time.h>
#include "internal/fs.h"


/* ASSUMES FileID comes after ShortName and the structs are identical up to that point. */
AssertCompileMembersSameSizeAndOffset(FILE_BOTH_DIR_INFORMATION, NextEntryOffset, FILE_ID_BOTH_DIR_INFORMATION, NextEntryOffset);
AssertCompileMembersSameSizeAndOffset(FILE_BOTH_DIR_INFORMATION, FileIndex      , FILE_ID_BOTH_DIR_INFORMATION, FileIndex      );
AssertCompileMembersSameSizeAndOffset(FILE_BOTH_DIR_INFORMATION, CreationTime   , FILE_ID_BOTH_DIR_INFORMATION, CreationTime   );
AssertCompileMembersSameSizeAndOffset(FILE_BOTH_DIR_INFORMATION, LastAccessTime , FILE_ID_BOTH_DIR_INFORMATION, LastAccessTime );
AssertCompileMembersSameSizeAndOffset(FILE_BOTH_DIR_INFORMATION, LastWriteTime  , FILE_ID_BOTH_DIR_INFORMATION, LastWriteTime  );
AssertCompileMembersSameSizeAndOffset(FILE_BOTH_DIR_INFORMATION, ChangeTime     , FILE_ID_BOTH_DIR_INFORMATION, ChangeTime     );
AssertCompileMembersSameSizeAndOffset(FILE_BOTH_DIR_INFORMATION, EndOfFile      , FILE_ID_BOTH_DIR_INFORMATION, EndOfFile      );
AssertCompileMembersSameSizeAndOffset(FILE_BOTH_DIR_INFORMATION, AllocationSize , FILE_ID_BOTH_DIR_INFORMATION, AllocationSize );
AssertCompileMembersSameSizeAndOffset(FILE_BOTH_DIR_INFORMATION, FileAttributes , FILE_ID_BOTH_DIR_INFORMATION, FileAttributes );
AssertCompileMembersSameSizeAndOffset(FILE_BOTH_DIR_INFORMATION, FileNameLength , FILE_ID_BOTH_DIR_INFORMATION, FileNameLength );
AssertCompileMembersSameSizeAndOffset(FILE_BOTH_DIR_INFORMATION, EaSize         , FILE_ID_BOTH_DIR_INFORMATION, EaSize         );
AssertCompileMembersSameSizeAndOffset(FILE_BOTH_DIR_INFORMATION, ShortNameLength, FILE_ID_BOTH_DIR_INFORMATION, ShortNameLength);
AssertCompileMembersSameSizeAndOffset(FILE_BOTH_DIR_INFORMATION, ShortName      , FILE_ID_BOTH_DIR_INFORMATION, ShortName      );



RTR3DECL(int) RTPathQueryInfo(const char *pszPath, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAdditionalAttribs)
{
    return RTPathQueryInfoEx(pszPath, pObjInfo, enmAdditionalAttribs, RTPATH_F_ON_LINK);
}

#if 1
RTR3DECL(int) RTPathQueryInfoEx(const char *pszPath, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAdditionalAttribs, uint32_t fFlags)
{
    /*
     * Validate input.
     */
    AssertPtrReturn(pszPath, VERR_INVALID_POINTER);
    AssertReturn(*pszPath, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pObjInfo, VERR_INVALID_POINTER);
    AssertMsgReturn(    enmAdditionalAttribs >= RTFSOBJATTRADD_NOTHING
                    &&  enmAdditionalAttribs <= RTFSOBJATTRADD_LAST,
                    ("Invalid enmAdditionalAttribs=%p\n", enmAdditionalAttribs),
                    VERR_INVALID_PARAMETER);
    AssertMsgReturn(RTPATH_F_IS_VALID(fFlags, 0), ("%#x\n", fFlags), VERR_INVALID_PARAMETER);


    /*
     * Convert the input path.
     */
    HANDLE         hRootDir;
    UNICODE_STRING NtName;
    int rc = RTNtPathFromWinUtf8(&NtName, &hRootDir, pszPath);
    if (RT_SUCCESS(rc))
    {
        /*
         * There are a three different ways of doing this:
         *   1. Use NtQueryFullAttributesFile to the get basic file info.
         *   2. Open whatever the path points to and use NtQueryInformationFile.
         *   3. Open the parent directory and use NtQueryDirectoryFile like RTDirReadEx.
         *
         * The first two options may fail with sharing violations or access denied,
         * in which case we must use the last one as fallback.
         */
        HANDLE              hFile = RTNT_INVALID_HANDLE_VALUE;
        IO_STATUS_BLOCK     Ios = RTNT_IO_STATUS_BLOCK_INITIALIZER;
        NTSTATUS            rcNt;
        OBJECT_ATTRIBUTES   ObjAttr;
        union
        {
            FILE_NETWORK_OPEN_INFORMATION   NetOpenInfo;
            FILE_ALL_INFORMATION            AllInfo;
            FILE_FS_VOLUME_INFORMATION      VolInfo;
            FILE_BOTH_DIR_INFORMATION       Both;
            FILE_ID_BOTH_DIR_INFORMATION    BothId;
            uint8_t                         abPadding[sizeof(FILE_ID_BOTH_DIR_INFORMATION) + RTPATH_MAX * sizeof(wchar_t)];
        } uBuf;

        /*
         * We can only use the first option if no additional UNIX attribs are
         * requested and it isn't a symbolic link.
         */
        rc = VINF_TRY_AGAIN;
        if (enmAdditionalAttribs != RTFSOBJATTRADD_UNIX)
        {
            InitializeObjectAttributes(&ObjAttr, &NtName, OBJ_CASE_INSENSITIVE, hRootDir, NULL);
            rcNt = NtQueryFullAttributesFile(&ObjAttr, &uBuf.NetOpenInfo);
            if (NT_SUCCESS(rcNt))
            {
                if (!(uBuf.NetOpenInfo.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
                {
                    pObjInfo->cbObject    = uBuf.NetOpenInfo.EndOfFile.QuadPart;
                    pObjInfo->cbAllocated = uBuf.NetOpenInfo.AllocationSize.QuadPart;
                    RTTimeSpecSetNtTime(&pObjInfo->BirthTime,         uBuf.NetOpenInfo.CreationTime.QuadPart);
                    RTTimeSpecSetNtTime(&pObjInfo->AccessTime,        uBuf.NetOpenInfo.LastAccessTime.QuadPart);
                    RTTimeSpecSetNtTime(&pObjInfo->ModificationTime,  uBuf.NetOpenInfo.LastWriteTime.QuadPart);
                    RTTimeSpecSetNtTime(&pObjInfo->ChangeTime,        uBuf.NetOpenInfo.ChangeTime.QuadPart);
                    pObjInfo->Attr.fMode = rtFsModeFromDos((uBuf.NetOpenInfo.FileAttributes << RTFS_DOS_SHIFT) & RTFS_DOS_MASK_NT,
                                                           pszPath, strlen(pszPath), 0 /*uReparseTag*/);
                    pObjInfo->Attr.enmAdditional = enmAdditionalAttribs;
                    rc = VINF_SUCCESS;
                }
            }
            else if (   rcNt != STATUS_ACCESS_DENIED
                     && rcNt != STATUS_SHARING_VIOLATION)
                rc = RTErrConvertFromNtStatus(rcNt);
            else
                RTNT_IO_STATUS_BLOCK_REINIT(&Ios);
        }

        /*
         * Try the 2nd option.  We might have to redo this if not following symbolic
         * links and the reparse point isn't a symbolic link but a mount point or similar.
         * We want to return information about the mounted root directory if we can, not
         * the directory in which it was mounted.
         */
        if (rc == VINF_TRY_AGAIN)
        {
            InitializeObjectAttributes(&ObjAttr, &NtName, OBJ_CASE_INSENSITIVE, hRootDir, NULL);
            rcNt = NtCreateFile(&hFile,
                                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                                &ObjAttr,
                                &Ios,
                                NULL /*pcbFile*/,
                                FILE_ATTRIBUTE_NORMAL,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                FILE_OPEN,
                                FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT
                                | (fFlags & RTPATH_F_FOLLOW_LINK ? 0 : FILE_OPEN_REPARSE_POINT),
                                NULL /*pvEaBuffer*/,
                                0 /*cbEa*/);
            if (NT_SUCCESS(rcNt))
            {
                /* Query tag information first in order to try re-open non-symlink reparse points. */
                FILE_ATTRIBUTE_TAG_INFORMATION TagInfo;
                rcNt = NtQueryInformationFile(hFile, &Ios, &TagInfo, sizeof(TagInfo), FileAttributeTagInformation);
                if (!NT_SUCCESS(rcNt))
                    TagInfo.FileAttributes = TagInfo.ReparseTag = 0;
                if (   !(TagInfo.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
                    || TagInfo.ReparseTag == IO_REPARSE_TAG_SYMLINK
                    || (fFlags & RTPATH_F_FOLLOW_LINK))
                { /* likely */ }
                else
                {
                    /* Reparse point that isn't a symbolic link, try follow the reparsing. */
                    HANDLE hFile2;
                    RTNT_IO_STATUS_BLOCK_REINIT(&Ios);
                    rcNt = NtCreateFile(&hFile2,
                                        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                                        &ObjAttr,
                                        &Ios,
                                        NULL /*pcbFile*/,
                                        FILE_ATTRIBUTE_NORMAL,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                        FILE_OPEN,
                                        FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT,
                                        NULL /*pvEaBuffer*/,
                                        0 /*cbEa*/);
                    if (NT_SUCCESS(rcNt))
                    {
                        NtClose(hFile);
                        hFile = hFile2;
                        TagInfo.FileAttributes = TagInfo.ReparseTag = 0;
                    }
                }

                /*
                 * Get the information we need and convert it.
                 */
                /** @todo Try optimize this for when RTFSOBJATTRADD_UNIX isn't set? */
                RTNT_IO_STATUS_BLOCK_REINIT(&Ios);
                rcNt = NtQueryInformationFile(hFile, &Ios, &uBuf.AllInfo, sizeof(uBuf.AllInfo), FileAllInformation);
                if (NT_SUCCESS(rcNt) || rcNt == STATUS_BUFFER_OVERFLOW)
                {
                    pObjInfo->cbObject    = uBuf.AllInfo.StandardInformation.EndOfFile.QuadPart;
                    pObjInfo->cbAllocated = uBuf.AllInfo.StandardInformation.AllocationSize.QuadPart;
                    RTTimeSpecSetNtTime(&pObjInfo->BirthTime,         uBuf.AllInfo.BasicInformation.CreationTime.QuadPart);
                    RTTimeSpecSetNtTime(&pObjInfo->AccessTime,        uBuf.AllInfo.BasicInformation.LastAccessTime.QuadPart);
                    RTTimeSpecSetNtTime(&pObjInfo->ModificationTime,  uBuf.AllInfo.BasicInformation.LastWriteTime.QuadPart);
                    RTTimeSpecSetNtTime(&pObjInfo->ChangeTime,        uBuf.AllInfo.BasicInformation.ChangeTime.QuadPart);
                    pObjInfo->Attr.fMode = rtFsModeFromDos(  (uBuf.AllInfo.BasicInformation.FileAttributes << RTFS_DOS_SHIFT)
                                                           & RTFS_DOS_MASK_NT,
                                                           pszPath, strlen(pszPath), TagInfo.ReparseTag);
                    pObjInfo->Attr.enmAdditional = enmAdditionalAttribs;
                    if (enmAdditionalAttribs == RTFSOBJATTRADD_UNIX)
                    {
                        pObjInfo->Attr.u.Unix.uid             = ~0U;
                        pObjInfo->Attr.u.Unix.gid             = ~0U;
                        pObjInfo->Attr.u.Unix.cHardlinks      = RT_MAX(1, uBuf.AllInfo.StandardInformation.NumberOfLinks);
                        pObjInfo->Attr.u.Unix.INodeIdDevice   = 0; /* below */
                        pObjInfo->Attr.u.Unix.INodeId         = uBuf.AllInfo.InternalInformation.IndexNumber.QuadPart;
                        pObjInfo->Attr.u.Unix.fFlags          = 0;
                        pObjInfo->Attr.u.Unix.GenerationId    = 0;
                        pObjInfo->Attr.u.Unix.Device          = 0;

                        /* Get the serial number. */
                        rcNt = NtQueryVolumeInformationFile(hFile, &Ios, &uBuf, RT_MIN(sizeof(uBuf), _2K),
                                                            FileFsVolumeInformation);
                        if (NT_SUCCESS(rcNt) || rcNt == STATUS_BUFFER_OVERFLOW)
                            pObjInfo->Attr.u.Unix.INodeIdDevice = uBuf.VolInfo.VolumeSerialNumber;
                    }

                    rc = VINF_SUCCESS;
                }

                NtClose(hFile);
            }
            else if (   rcNt != STATUS_ACCESS_DENIED
                     && rcNt != STATUS_SHARING_VIOLATION)
                rc = RTErrConvertFromNtStatus(rcNt);
            else
                RTNT_IO_STATUS_BLOCK_REINIT(&Ios);
        }

        /*
         * Try the 3rd option if none of the other worked.
         * If none of the above worked, try open the directory and enumerate
         * the file we're after.  This
         */
        if (rc == VINF_TRY_AGAIN)
        {
            /* Drop the name from NtPath. */
            UNICODE_STRING NtDirName = NtName;
            size_t off = NtName.Length / sizeof(NtName.Buffer[0]);
            wchar_t wc;
            while (   off > 0
                   && (wc = NtName.Buffer[off - 1]) != '\\'
                   && wc != '/')
                off--;
            if (off != 0)
                NtDirName.Length = (USHORT)(off * sizeof(NtName.Buffer[0]));
            else
            {
                AssertFailed(); /* This is impossible and won't work (NT doesn't know '.' or '..').  */
                NtDirName.Buffer = L".";
                NtDirName.Length = sizeof(NtName.Buffer[0]);
                NtDirName.MaximumLength = 2 * sizeof(NtName.Buffer[0]);
            }

            UNICODE_STRING NtFilter;
            NtFilter.Buffer        = &NtName.Buffer[off];
            NtFilter.Length        = NtName.Length        - (USHORT)(off * sizeof(NtName.Buffer[0]));
            NtFilter.MaximumLength = NtName.MaximumLength - (USHORT)(off * sizeof(NtName.Buffer[0]));

            /* Try open the directory. */
            InitializeObjectAttributes(&ObjAttr, &NtDirName, OBJ_CASE_INSENSITIVE, hRootDir, NULL);
            rcNt = NtCreateFile(&hFile,
                                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                                &ObjAttr,
                                &Ios,
                                NULL /*pcbFile*/,
                                FILE_ATTRIBUTE_NORMAL,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                FILE_OPEN,
                                FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT,
                                NULL /*pvEaBuffer*/,
                                0 /*cbEa*/);
            if (NT_SUCCESS(rcNt))
            {
                FILE_INFORMATION_CLASS enmInfoClass;
                if (RT_MAKE_U64(RTNtCurrentPeb()->OSMinorVersion, RTNtCurrentPeb()->OSMajorVersion) > RT_MAKE_U64(0,5) /* > W2K */)
                    enmInfoClass = FileIdBothDirectoryInformation; /* Introduced in XP, from I can tell. */
                else
                    enmInfoClass = FileBothDirectoryInformation;
                rcNt = NtQueryDirectoryFile(hFile,
                                            NULL /* Event */,
                                            NULL /* ApcRoutine */,
                                            NULL /* ApcContext */,
                                            &Ios,
                                            &uBuf,
                                            RT_MIN(sizeof(uBuf), 0xfff0),
                                            enmInfoClass,
                                            TRUE /*ReturnSingleEntry */,
                                            &NtFilter,
                                            FALSE /*RestartScan */);
                if (NT_SUCCESS(rcNt))
                {
                    pObjInfo->cbObject    = uBuf.Both.EndOfFile.QuadPart;
                    pObjInfo->cbAllocated = uBuf.Both.AllocationSize.QuadPart;

                    RTTimeSpecSetNtTime(&pObjInfo->BirthTime,         uBuf.Both.CreationTime.QuadPart);
                    RTTimeSpecSetNtTime(&pObjInfo->AccessTime,        uBuf.Both.LastAccessTime.QuadPart);
                    RTTimeSpecSetNtTime(&pObjInfo->ModificationTime,  uBuf.Both.LastWriteTime.QuadPart);
                    RTTimeSpecSetNtTime(&pObjInfo->ChangeTime,        uBuf.Both.ChangeTime.QuadPart);

                    pObjInfo->Attr.fMode  = rtFsModeFromDos((uBuf.Both.FileAttributes << RTFS_DOS_SHIFT) & RTFS_DOS_MASK_NT,
                                                            pszPath, strlen(pszPath), uBuf.Both.EaSize);

                    pObjInfo->Attr.enmAdditional = enmAdditionalAttribs;
                    if (enmAdditionalAttribs == RTFSOBJATTRADD_UNIX)
                    {
                        pObjInfo->Attr.u.Unix.uid             = ~0U;
                        pObjInfo->Attr.u.Unix.gid             = ~0U;
                        pObjInfo->Attr.u.Unix.cHardlinks      = 1;
                        pObjInfo->Attr.u.Unix.INodeIdDevice   = 0; /* below */
                        pObjInfo->Attr.u.Unix.INodeId         = enmInfoClass == FileIdBothDirectoryInformation
                                                              ? uBuf.BothId.FileId.QuadPart : 0;
                        pObjInfo->Attr.u.Unix.fFlags          = 0;
                        pObjInfo->Attr.u.Unix.GenerationId    = 0;
                        pObjInfo->Attr.u.Unix.Device          = 0;

                        /* Get the serial number. */
                        rcNt = NtQueryVolumeInformationFile(hFile, &Ios, &uBuf, RT_MIN(sizeof(uBuf), _2K),
                                                            FileFsVolumeInformation);
                        if (NT_SUCCESS(rcNt))
                            pObjInfo->Attr.u.Unix.INodeIdDevice = uBuf.VolInfo.VolumeSerialNumber;
                    }

                    rc = VINF_SUCCESS;

                }
                else
                    rc = RTErrConvertFromNtStatus(rcNt);

                NtClose(hFile);
            }
            else
                rc = RTErrConvertFromNtStatus(rcNt);
        }

        /*
         * Fill in dummy additional attributes for the non-UNIX requests.
         */
        switch (enmAdditionalAttribs)
        {
            case RTFSOBJATTRADD_UNIX:
                break;

            case RTFSOBJATTRADD_NOTHING:
                pObjInfo->Attr.enmAdditional          = RTFSOBJATTRADD_NOTHING;
                break;

            case RTFSOBJATTRADD_UNIX_OWNER:
                pObjInfo->Attr.enmAdditional          = RTFSOBJATTRADD_UNIX_OWNER;
                pObjInfo->Attr.u.UnixOwner.uid        = ~0U;
                pObjInfo->Attr.u.UnixOwner.szName[0]  = '\0'; /** @todo return something sensible here. */
                break;

            case RTFSOBJATTRADD_UNIX_GROUP:
                pObjInfo->Attr.enmAdditional          = RTFSOBJATTRADD_UNIX_GROUP;
                pObjInfo->Attr.u.UnixGroup.gid        = ~0U;
                pObjInfo->Attr.u.UnixGroup.szName[0]  = '\0';
                break;

            case RTFSOBJATTRADD_EASIZE:
                pObjInfo->Attr.enmAdditional          = RTFSOBJATTRADD_EASIZE;
                pObjInfo->Attr.u.EASize.cb            = 0;
                break;

            default:
                AssertMsgFailed(("Impossible!\n"));
                rc = VERR_INTERNAL_ERROR;
        }

        RTNtPathFree(&NtName, &hRootDir);
    }

    return rc;
}
#endif

