/* $Id: fs.cpp $ */
/** @file
 * IPRT - File System.
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
#include <iprt/fs.h>
#include "internal/iprt.h"

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/ctype.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include <iprt/time.h>
#include "internal/fs.h"


/**
 * Converts dos-style attributes to Unix attributes.
 *
 * @returns
 * @param   fMode       The mode mask containing dos-style attributes only.
 * @param   pszName     The filename which this applies to (exe check).
 * @param   cbName      The length of that filename. (optional, set 0)
 * @param   uReparseTag The reparse tag if RTFS_DOS_NT_REPARSE_POINT is set.
 */
RTFMODE rtFsModeFromDos(RTFMODE fMode, const char *pszName, size_t cbName, uint32_t uReparseTag)
{
    fMode &= ~((1 << RTFS_DOS_SHIFT) - 1);

    /* everything is readable. */
    fMode |= RTFS_UNIX_IRUSR | RTFS_UNIX_IRGRP | RTFS_UNIX_IROTH;
    if (fMode & RTFS_DOS_DIRECTORY)
        /* directories are executable. */
        fMode |= RTFS_TYPE_DIRECTORY | RTFS_UNIX_IXUSR | RTFS_UNIX_IXGRP | RTFS_UNIX_IXOTH;
    else
    {
        fMode |= RTFS_TYPE_FILE;
        if (!cbName && pszName)
            cbName = strlen(pszName);
        if (cbName >= 4 && pszName[cbName - 4] == '.')
        {
            /* check for executable extension. */
            const char *pszExt = &pszName[cbName - 3];
            char szExt[4];
            szExt[0] = RT_C_TO_LOWER(pszExt[0]);
            szExt[1] = RT_C_TO_LOWER(pszExt[1]);
            szExt[2] = RT_C_TO_LOWER(pszExt[2]);
            szExt[3] = '\0';
            if (    !memcmp(szExt, "exe", 4)
                ||  !memcmp(szExt, "bat", 4)
                ||  !memcmp(szExt, "com", 4)
                ||  !memcmp(szExt, "cmd", 4)
                ||  !memcmp(szExt, "btm", 4)
               )
                fMode |= RTFS_UNIX_IXUSR | RTFS_UNIX_IXGRP | RTFS_UNIX_IXOTH;
        }
    }

    /* Is it really a symbolic link? */
    if ((fMode & RTFS_DOS_NT_REPARSE_POINT) && uReparseTag == RTFSMODE_SYMLINK_REPARSE_TAG)
        fMode = (fMode & ~RTFS_TYPE_MASK) | RTFS_TYPE_SYMLINK;

    /* writable? */
    if (!(fMode & RTFS_DOS_READONLY))
        fMode |= RTFS_UNIX_IWUSR | RTFS_UNIX_IWGRP | RTFS_UNIX_IWOTH;
    return fMode;
}


/**
 * Converts Unix attributes to Dos-style attributes.
 *
 * @returns File mode mask.
 * @param   fMode       The mode mask containing dos-style attributes only.
 * @param   pszName     The filename which this applies to (hidden check).
 * @param   cbName      The length of that filename. (optional, set 0)
 */
RTFMODE rtFsModeFromUnix(RTFMODE fMode, const char *pszName, size_t cbName)
{
    NOREF(cbName);

    fMode &= RTFS_UNIX_MASK;

    if (!(fMode & (RTFS_UNIX_IWUSR | RTFS_UNIX_IWGRP | RTFS_UNIX_IWOTH)))
        fMode |= RTFS_DOS_READONLY;
    if (RTFS_IS_DIRECTORY(fMode))
        fMode |= RTFS_DOS_DIRECTORY;
    if (!(fMode & RTFS_DOS_MASK))
        fMode |= RTFS_DOS_NT_NORMAL;
    if (!(fMode & RTFS_DOS_HIDDEN) && pszName)
    {
        pszName = RTPathFilename(pszName);
        if (pszName && *pszName == '.')
            fMode |= RTFS_DOS_HIDDEN;
    }
    return fMode;
}


/**
 * Normalizes the give mode mask.
 *
 * It will create the missing unix or dos mask from the other (one
 * of them is required by all APIs), and guess the file type if that's
 * missing.
 *
 * @returns Normalized file mode.
 * @param   fMode       The mode mask that may contain a partial/incomplete mask.
 * @param   pszName     The filename which this applies to (exe check).
 * @param   cbName      The length of that filename. (optional, set 0)
 */
RTFMODE rtFsModeNormalize(RTFMODE fMode, const char *pszName, size_t cbName)
{
    if (!(fMode & RTFS_UNIX_MASK))
        fMode = rtFsModeFromDos(fMode, pszName, cbName, RTFSMODE_SYMLINK_REPARSE_TAG);
    else if (!(fMode & RTFS_DOS_MASK))
        fMode = rtFsModeFromUnix(fMode, pszName, cbName);
    else if (!(fMode & RTFS_TYPE_MASK))
        fMode |= fMode & RTFS_DOS_DIRECTORY ? RTFS_TYPE_DIRECTORY : RTFS_TYPE_FILE;
    else if (RTFS_IS_DIRECTORY(fMode))
        fMode |= RTFS_DOS_DIRECTORY;
    return fMode;
}


/**
 * Checks if the file mode is valid or not.
 *
 * @return  true if valid.
 * @return  false if invalid, done bitching.
 * @param   fMode       The file mode.
 */
bool rtFsModeIsValid(RTFMODE fMode)
{
    AssertMsgReturn(   (!RTFS_IS_DIRECTORY(fMode) && !(fMode & RTFS_DOS_DIRECTORY))
                    || (RTFS_IS_DIRECTORY(fMode) && (fMode & RTFS_DOS_DIRECTORY)),
                    ("%RTfmode\n", fMode), false);
    AssertMsgReturn(RTFS_TYPE_MASK & fMode,
                    ("%RTfmode\n", fMode), false);
    /** @todo more checks! */
    return true;
}


/**
 * Checks if the file mode is valid as a permission mask or not.
 *
 * @return  true if valid.
 * @return  false if invalid, done bitching.
 * @param   fMode       The file mode.
 */
bool rtFsModeIsValidPermissions(RTFMODE fMode)
{
    AssertMsgReturn(   (!RTFS_IS_DIRECTORY(fMode) && !(fMode & RTFS_DOS_DIRECTORY))
                    || (RTFS_IS_DIRECTORY(fMode) && (fMode & RTFS_DOS_DIRECTORY)),
                    ("%RTfmode\n", fMode), false);
    /** @todo more checks! */
    return true;
}


RTDECL(const char *) RTFsTypeName(RTFSTYPE enmType)
{
    switch (enmType)
    {
        case RTFSTYPE_UNKNOWN:      return "unknown";
        case RTFSTYPE_UDF:          return "udf";
        case RTFSTYPE_ISO9660:      return "iso9660";
        case RTFSTYPE_FUSE:         return "fuse";
        case RTFSTYPE_VBOXSHF:      return "vboxshf";

        case RTFSTYPE_EXT:          return "ext";
        case RTFSTYPE_EXT2:         return "ext2";
        case RTFSTYPE_EXT3:         return "ext3";
        case RTFSTYPE_EXT4:         return "ext4";
        case RTFSTYPE_XFS:          return "xfs";
        case RTFSTYPE_CIFS:         return "cifs";
        case RTFSTYPE_SMBFS:        return "smbfs";
        case RTFSTYPE_TMPFS:        return "tmpfs";
        case RTFSTYPE_SYSFS:        return "sysfs";
        case RTFSTYPE_PROC:         return "proc";
        case RTFSTYPE_OCFS2:        return "ocfs2";
        case RTFSTYPE_BTRFS:        return "btrfs";

        case RTFSTYPE_NTFS:         return "ntfs";
        case RTFSTYPE_FAT:          return "fat";
        case RTFSTYPE_EXFAT:        return "exfat";

        case RTFSTYPE_ZFS:          return "zfs";
        case RTFSTYPE_UFS:          return "ufs";
        case RTFSTYPE_NFS:          return "nfs";

        case RTFSTYPE_HFS:          return "hfs";
        case RTFSTYPE_AUTOFS:       return "autofs";
        case RTFSTYPE_DEVFS:        return "devfs";

        case RTFSTYPE_HPFS:         return "hpfs";
        case RTFSTYPE_JFS:          return "jfs";

        case RTFSTYPE_END:          return "end";
        case RTFSTYPE_32BIT_HACK:   break;
    }

    /* Don't put this in as 'default:', we wish GCC to warn about missing cases. */
    static char                 s_asz[4][64];
    static uint32_t volatile    s_i = 0;
    uint32_t i = ASMAtomicIncU32(&s_i) % RT_ELEMENTS(s_asz);
    RTStrPrintf(s_asz[i], sizeof(s_asz[i]), "type=%d", enmType);
    return s_asz[i];
}

