/* $Id: fileio-posix.cpp $ */
/** @file
 * IPRT - File I/O, POSIX, Part 1.
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

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#ifdef _MSC_VER
# include <io.h>
# include <stdio.h>
#else
# include <unistd.h>
# include <sys/time.h>
#endif
#ifdef RT_OS_LINUX
# include <sys/file.h>
#endif
#if defined(RT_OS_OS2) && (!defined(__INNOTEK_LIBC__) || __INNOTEK_LIBC__ < 0x006)
# include <io.h>
#endif
#if defined(RT_OS_DARWIN) || defined(RT_OS_FREEBSD)
# include <sys/disk.h>
#endif
#ifdef RT_OS_SOLARIS
# include <stropts.h>
# include <sys/dkio.h>
# include <sys/vtoc.h>
#endif /* RT_OS_SOLARIS */

#include <iprt/file.h>
#include <iprt/path.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/err.h>
#include <iprt/log.h>
#include "internal/file.h"
#include "internal/fs.h"
#include "internal/path.h"



/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** Default file permissions for newly created files. */
#if defined(S_IRUSR) && defined(S_IWUSR)
# define RT_FILE_PERMISSION  (S_IRUSR | S_IWUSR)
#else
# define RT_FILE_PERMISSION  (00600)
#endif


RTDECL(bool) RTFileExists(const char *pszPath)
{
    bool fRc = false;
    char const *pszNativePath;
    int rc = rtPathToNative(&pszNativePath, pszPath, NULL);
    if (RT_SUCCESS(rc))
    {
        struct stat s;
        fRc = !stat(pszNativePath, &s)
            && S_ISREG(s.st_mode);

        rtPathFreeNative(pszNativePath, pszPath);
    }

    LogFlow(("RTFileExists(%p={%s}): returns %RTbool\n", pszPath, pszPath, fRc));
    return fRc;
}


RTR3DECL(int) RTFileOpen(PRTFILE pFile, const char *pszFilename, uint64_t fOpen)
{
    /*
     * Validate input.
     */
    AssertPtrReturn(pFile, VERR_INVALID_POINTER);
    *pFile = NIL_RTFILE;
    AssertPtrReturn(pszFilename, VERR_INVALID_POINTER);

    /*
     * Merge forced open flags and validate them.
     */
    int rc = rtFileRecalcAndValidateFlags(&fOpen);
    if (RT_FAILURE(rc))
        return rc;
#ifndef O_NONBLOCK
    if (fOpen & RTFILE_O_NON_BLOCK)
    {
        AssertMsgFailed(("Invalid parameters! fOpen=%#llx\n", fOpen));
        return VERR_INVALID_PARAMETER;
    }
#endif

    /*
     * Calculate open mode flags.
     */
    int fOpenMode = 0;
#ifdef O_BINARY
    fOpenMode |= O_BINARY;              /* (pc) */
#endif
#ifdef O_LARGEFILE
    fOpenMode |= O_LARGEFILE;           /* (linux, solaris) */
#endif
#ifdef O_NOINHERIT
    if (!(fOpen & RTFILE_O_INHERIT))
        fOpenMode |= O_NOINHERIT;
#endif
#ifdef O_CLOEXEC
    static int s_fHave_O_CLOEXEC = 0; /* {-1,0,1}; since Linux 2.6.23 */
    if (!(fOpen & RTFILE_O_INHERIT) && s_fHave_O_CLOEXEC >= 0)
        fOpenMode |= O_CLOEXEC;
#endif
#ifdef O_NONBLOCK
    if (fOpen & RTFILE_O_NON_BLOCK)
        fOpenMode |= O_NONBLOCK;
#endif
#ifdef O_SYNC
    if (fOpen & RTFILE_O_WRITE_THROUGH)
        fOpenMode |= O_SYNC;
#endif
#if defined(O_DIRECT) && defined(RT_OS_LINUX)
    /* O_DIRECT is mandatory to get async I/O working on Linux. */
    if (fOpen & RTFILE_O_ASYNC_IO)
        fOpenMode |= O_DIRECT;
#endif
#if defined(O_DIRECT) && (defined(RT_OS_LINUX) || defined(RT_OS_FREEBSD) || defined(RT_OS_NETBSD))
    /* Disable the kernel cache. */
    if (fOpen & RTFILE_O_NO_CACHE)
        fOpenMode |= O_DIRECT;
#endif

    /* create/truncate file */
    switch (fOpen & RTFILE_O_ACTION_MASK)
    {
        case RTFILE_O_OPEN:             break;
        case RTFILE_O_OPEN_CREATE:      fOpenMode |= O_CREAT; break;
        case RTFILE_O_CREATE:           fOpenMode |= O_CREAT | O_EXCL; break;
        case RTFILE_O_CREATE_REPLACE:   fOpenMode |= O_CREAT | O_TRUNC; break; /** @todo replacing needs fixing, this is *not* a 1:1 mapping! */
    }
    if (fOpen & RTFILE_O_TRUNCATE)
        fOpenMode |= O_TRUNC;

    switch (fOpen & RTFILE_O_ACCESS_MASK)
    {
        case RTFILE_O_READ:
            fOpenMode |= O_RDONLY; /* RTFILE_O_APPEND is ignored. */
            break;
        case RTFILE_O_WRITE:
            fOpenMode |= fOpen & RTFILE_O_APPEND ? O_APPEND | O_WRONLY : O_WRONLY;
            break;
        case RTFILE_O_READWRITE:
            fOpenMode |= fOpen & RTFILE_O_APPEND ? O_APPEND | O_RDWR   : O_RDWR;
            break;
        default:
            AssertMsgFailed(("RTFileOpen received an invalid RW value, fOpen=%#llx\n", fOpen));
            return VERR_INVALID_PARAMETER;
    }

    /* File mode. */
    int fMode = (fOpen & RTFILE_O_CREATE_MODE_MASK)
              ? (fOpen & RTFILE_O_CREATE_MODE_MASK) >> RTFILE_O_CREATE_MODE_SHIFT
              : RT_FILE_PERMISSION;

    /** @todo sharing! */

    /*
     * Open/create the file.
     */
    char const *pszNativeFilename;
    rc = rtPathToNative(&pszNativeFilename, pszFilename, NULL);
    if (RT_FAILURE(rc))
        return (rc);

    int fh = open(pszNativeFilename, fOpenMode, fMode);
    int iErr = errno;

#ifdef O_CLOEXEC
    if (   (fOpenMode & O_CLOEXEC)
        && s_fHave_O_CLOEXEC == 0)
    {
        if (fh < 0 && iErr == EINVAL)
        {
            s_fHave_O_CLOEXEC = -1;
            fh = open(pszNativeFilename, fOpenMode, fMode);
            iErr = errno;
        }
        else if (fh >= 0)
            s_fHave_O_CLOEXEC = fcntl(fh, F_GETFD, 0) > 0 ? 1 : -1;
    }
#endif

    rtPathFreeNative(pszNativeFilename, pszFilename);
    if (fh >= 0)
    {
        iErr = 0;

        /*
         * Mark the file handle close on exec, unless inherit is specified.
         */
        if (    !(fOpen & RTFILE_O_INHERIT)
#ifdef O_NOINHERIT
            &&  !(fOpenMode & O_NOINHERIT)  /* Take care since it might be a zero value dummy. */
#endif
#ifdef O_CLOEXEC
            &&  s_fHave_O_CLOEXEC <= 0
#endif
            )
            iErr = fcntl(fh, F_SETFD, FD_CLOEXEC) >= 0 ? 0 : errno;

        /*
         * Switch direct I/O on now if requested and required.
         */
#if defined(RT_OS_DARWIN) \
 || (defined(RT_OS_SOLARIS) && !defined(IN_GUEST))
        if (iErr == 0 && (fOpen & RTFILE_O_NO_CACHE))
        {
# if defined(RT_OS_DARWIN)
            iErr = fcntl(fh, F_NOCACHE, 1)        >= 0 ? 0 : errno;
# else
            iErr = directio(fh, DIRECTIO_ON)      >= 0 ? 0 : errno;
# endif
        }
#endif

        /*
         * Implement / emulate file sharing.
         *
         * We need another mode which allows skipping this stuff completely
         * and do things the UNIX way. So for the present this is just a debug
         * aid that can be enabled by developers too lazy to test on Windows.
         */
#if 0 && defined(RT_OS_LINUX)
        if (iErr == 0)
        {
            /* This approach doesn't work because only knfsd checks for these
               buggers. :-( */
            int iLockOp;
            switch (fOpen & RTFILE_O_DENY_MASK)
            {
                default:
                AssertFailed();
                case RTFILE_O_DENY_NONE:
                case RTFILE_O_DENY_NOT_DELETE:
                    iLockOp = LOCK_MAND | LOCK_READ | LOCK_WRITE;
                    break;
                case RTFILE_O_DENY_READ:
                case RTFILE_O_DENY_READ | RTFILE_O_DENY_NOT_DELETE:
                    iLockOp = LOCK_MAND | LOCK_WRITE;
                    break;
                case RTFILE_O_DENY_WRITE:
                case RTFILE_O_DENY_WRITE | RTFILE_O_DENY_NOT_DELETE:
                    iLockOp = LOCK_MAND | LOCK_READ;
                    break;
                case RTFILE_O_DENY_WRITE | RTFILE_O_DENY_READ:
                case RTFILE_O_DENY_WRITE | RTFILE_O_DENY_READ | RTFILE_O_DENY_NOT_DELETE:
                    iLockOp = LOCK_MAND;
                    break;
            }
            iErr = flock(fh, iLockOp | LOCK_NB);
            if (iErr != 0)
                iErr = errno == EAGAIN ? ETXTBSY : 0;
        }
#endif /* 0 && RT_OS_LINUX */
#if defined(DEBUG_bird) && !defined(RT_OS_SOLARIS)
        if (iErr == 0)
        {
            /* This emulation is incomplete but useful. */
            switch (fOpen & RTFILE_O_DENY_MASK)
            {
                default:
                AssertFailed();
                case RTFILE_O_DENY_NONE:
                case RTFILE_O_DENY_NOT_DELETE:
                case RTFILE_O_DENY_READ:
                case RTFILE_O_DENY_READ | RTFILE_O_DENY_NOT_DELETE:
                    break;
                case RTFILE_O_DENY_WRITE:
                case RTFILE_O_DENY_WRITE | RTFILE_O_DENY_NOT_DELETE:
                case RTFILE_O_DENY_WRITE | RTFILE_O_DENY_READ:
                case RTFILE_O_DENY_WRITE | RTFILE_O_DENY_READ | RTFILE_O_DENY_NOT_DELETE:
                    if (fOpen & RTFILE_O_WRITE)
                    {
                        iErr = flock(fh, LOCK_EX | LOCK_NB);
                        if (iErr != 0)
                            iErr = errno == EAGAIN ? ETXTBSY : 0;
                    }
                    break;
            }
        }
#endif
#ifdef RT_OS_SOLARIS
        /** @todo Use fshare_t and associates, it's a perfect match. see sys/fcntl.h */
#endif

        /*
         * We're done.
         */
        if (iErr == 0)
        {
            *pFile = (RTFILE)(uintptr_t)fh;
            Assert((intptr_t)*pFile == fh);
            LogFlow(("RTFileOpen(%p:{%RTfile}, %p:{%s}, %#llx): returns %Rrc\n",
                     pFile, *pFile, pszFilename, pszFilename, fOpen, rc));
            return VINF_SUCCESS;
        }

        close(fh);
    }
    return RTErrConvertFromErrno(iErr);
}


RTR3DECL(int)  RTFileOpenBitBucket(PRTFILE phFile, uint64_t fAccess)
{
    AssertReturn(   fAccess == RTFILE_O_READ
                 || fAccess == RTFILE_O_WRITE
                 || fAccess == RTFILE_O_READWRITE,
                 VERR_INVALID_PARAMETER);
    return RTFileOpen(phFile, "/dev/null", fAccess | RTFILE_O_DENY_NONE | RTFILE_O_OPEN);
}


RTR3DECL(int)  RTFileClose(RTFILE hFile)
{
    if (hFile == NIL_RTFILE)
        return VINF_SUCCESS;
    if (close(RTFileToNative(hFile)) == 0)
        return VINF_SUCCESS;
    return RTErrConvertFromErrno(errno);
}


RTR3DECL(int) RTFileFromNative(PRTFILE pFile, RTHCINTPTR uNative)
{
    AssertCompile(sizeof(uNative) == sizeof(*pFile));
    if (uNative < 0)
    {
        AssertMsgFailed(("%p\n", uNative));
        *pFile = NIL_RTFILE;
        return VERR_INVALID_HANDLE;
    }
    *pFile = (RTFILE)uNative;
    return VINF_SUCCESS;
}


RTR3DECL(RTHCINTPTR) RTFileToNative(RTFILE hFile)
{
    AssertReturn(hFile != NIL_RTFILE, -1);
    return (intptr_t)hFile;
}


RTFILE rtFileGetStandard(RTHANDLESTD enmStdHandle)
{
    int fd;
    switch (enmStdHandle)
    {
        case RTHANDLESTD_INPUT:  fd = 0; break;
        case RTHANDLESTD_OUTPUT: fd = 1; break;
        case RTHANDLESTD_ERROR:  fd = 2; break;
        default:
            AssertFailedReturn(NIL_RTFILE);
    }

    struct stat st;
    int rc = fstat(fd, &st);
    if (rc == -1)
        return NIL_RTFILE;
    return (RTFILE)(intptr_t)fd;
}


RTR3DECL(int)  RTFileDelete(const char *pszFilename)
{
    char const *pszNativeFilename;
    int rc = rtPathToNative(&pszNativeFilename, pszFilename, NULL);
    if (RT_SUCCESS(rc))
    {
        if (unlink(pszNativeFilename) != 0)
            rc = RTErrConvertFromErrno(errno);
        rtPathFreeNative(pszNativeFilename, pszFilename);
    }
    return rc;
}


RTR3DECL(int)  RTFileSeek(RTFILE hFile, int64_t offSeek, unsigned uMethod, uint64_t *poffActual)
{
    static const unsigned aSeekRecode[] =
    {
        SEEK_SET,
        SEEK_CUR,
        SEEK_END,
    };

    /*
     * Validate input.
     */
    if (uMethod > RTFILE_SEEK_END)
    {
        AssertMsgFailed(("Invalid uMethod=%d\n", uMethod));
        return VERR_INVALID_PARAMETER;
    }

    /* check that within off_t range. */
    if (    sizeof(off_t) < sizeof(offSeek)
        && (    (offSeek > 0 && (unsigned)(offSeek >> 32) != 0)
            ||  (offSeek < 0 && (unsigned)(-offSeek >> 32) != 0)))
    {
        AssertMsgFailed(("64-bit search not supported\n"));
        return VERR_NOT_SUPPORTED;
    }

    off_t offCurrent = lseek(RTFileToNative(hFile), (off_t)offSeek, aSeekRecode[uMethod]);
    if (offCurrent != ~0)
    {
        if (poffActual)
            *poffActual = (uint64_t)offCurrent;
        return VINF_SUCCESS;
    }
    return RTErrConvertFromErrno(errno);
}


RTR3DECL(int)  RTFileRead(RTFILE hFile, void *pvBuf, size_t cbToRead, size_t *pcbRead)
{
    if (cbToRead <= 0)
        return VINF_SUCCESS;

    /*
     * Attempt read.
     */
    ssize_t cbRead = read(RTFileToNative(hFile), pvBuf, cbToRead);
    if (cbRead >= 0)
    {
        if (pcbRead)
            /* caller can handle partial read. */
            *pcbRead = cbRead;
        else
        {
            /* Caller expects all to be read. */
            while ((ssize_t)cbToRead > cbRead)
            {
                ssize_t cbReadPart = read(RTFileToNative(hFile), (char*)pvBuf + cbRead, cbToRead - cbRead);
                if (cbReadPart <= 0)
                {
                    if (cbReadPart == 0)
                        return VERR_EOF;
                    return RTErrConvertFromErrno(errno);
                }
                cbRead += cbReadPart;
            }
        }
        return VINF_SUCCESS;
    }

    return RTErrConvertFromErrno(errno);
}


RTR3DECL(int)  RTFileWrite(RTFILE hFile, const void *pvBuf, size_t cbToWrite, size_t *pcbWritten)
{
    if (cbToWrite <= 0)
        return VINF_SUCCESS;

    /*
     * Attempt write.
     */
    ssize_t cbWritten = write(RTFileToNative(hFile), pvBuf, cbToWrite);
    if (cbWritten >= 0)
    {
        if (pcbWritten)
            /* caller can handle partial write. */
            *pcbWritten = cbWritten;
        else
        {
            /* Caller expects all to be write. */
            while ((ssize_t)cbToWrite > cbWritten)
            {
                ssize_t cbWrittenPart = write(RTFileToNative(hFile), (const char *)pvBuf + cbWritten, cbToWrite - cbWritten);
                if (cbWrittenPart <= 0)
                    return RTErrConvertFromErrno(errno);
                cbWritten += cbWrittenPart;
            }
        }
        return VINF_SUCCESS;
    }
    return RTErrConvertFromErrno(errno);
}


RTR3DECL(int)  RTFileSetSize(RTFILE hFile, uint64_t cbSize)
{
    /*
     * Validate offset.
     */
    if (    sizeof(off_t) < sizeof(cbSize)
        &&  (cbSize >> 32) != 0)
    {
        AssertMsgFailed(("64-bit filesize not supported! cbSize=%lld\n", cbSize));
        return VERR_NOT_SUPPORTED;
    }

#if defined(_MSC_VER) || (defined(RT_OS_OS2) && (!defined(__INNOTEK_LIBC__) || __INNOTEK_LIBC__ < 0x006))
    if (chsize(RTFileToNative(hFile), (off_t)cbSize) == 0)
#else
    /* This relies on a non-standard feature of FreeBSD, Linux, and OS/2
     * LIBC v0.6 and higher. (SuS doesn't define ftruncate() and size bigger
     * than the file.)
     */
    if (ftruncate(RTFileToNative(hFile), (off_t)cbSize) == 0)
#endif
        return VINF_SUCCESS;
    return RTErrConvertFromErrno(errno);
}


RTR3DECL(int) RTFileGetSize(RTFILE hFile, uint64_t *pcbSize)
{
    /*
     * Ask fstat() first.
     */
    struct stat st;
    if (!fstat(RTFileToNative(hFile), &st))
    {
        *pcbSize = st.st_size;
        if (   st.st_size != 0
#if defined(RT_OS_SOLARIS)
            || (!S_ISBLK(st.st_mode) && !S_ISCHR(st.st_mode))
#elif defined(RT_OS_FREEBSD) || defined(RT_OS_NETBSD)
            || !S_ISCHR(st.st_mode)
#else
            || !S_ISBLK(st.st_mode)
#endif
            )
            return VINF_SUCCESS;

        /*
         * It could be a block device.  Try determin the size by I/O control
         * query or seek.
         */
#ifdef RT_OS_DARWIN
        uint64_t cBlocks;
        if (!ioctl(RTFileToNative(hFile), DKIOCGETBLOCKCOUNT, &cBlocks))
        {
            uint32_t cbBlock;
            if (!ioctl(RTFileToNative(hFile), DKIOCGETBLOCKSIZE, &cbBlock))
            {
                *pcbSize = cBlocks * cbBlock;
                return VINF_SUCCESS;
            }
        }
        /* must be a block device, fail on failure. */

#elif defined(RT_OS_SOLARIS)
        struct dk_minfo MediaInfo;
        if (!ioctl(RTFileToNative(hFile), DKIOCGMEDIAINFO, &MediaInfo))
        {
            *pcbSize = MediaInfo.dki_capacity * MediaInfo.dki_lbsize;
            return VINF_SUCCESS;
        }
        /* might not be a block device. */
        if (errno == EINVAL || errno == ENOTTY)
            return VINF_SUCCESS;

#elif defined(RT_OS_FREEBSD)
        off_t cbMedia = 0;
        if (!ioctl(RTFileToNative(hFile), DIOCGMEDIASIZE, &cbMedia))
        {
            *pcbSize = cbMedia;
            return VINF_SUCCESS;
        }
        /* might not be a block device. */
        if (errno == EINVAL || errno == ENOTTY)
            return VINF_SUCCESS;

#else
        /* PORTME! Avoid this path when possible. */
        uint64_t offSaved;
        int rc = RTFileSeek(hFile, 0, RTFILE_SEEK_CURRENT, &offSaved);
        if (RT_SUCCESS(rc))
        {
            rc = RTFileSeek(hFile, 0, RTFILE_SEEK_END, pcbSize);
            int rc2 = RTFileSeek(hFile, offSaved, RTFILE_SEEK_BEGIN, NULL);
            if (RT_SUCCESS(rc))
                return rc2;
        }
#endif
    }
    return RTErrConvertFromErrno(errno);
}


RTR3DECL(int) RTFileGetMaxSizeEx(RTFILE hFile, PRTFOFF pcbMax)
{
    /*
     * Save the current location
     */
    uint64_t offOld;
    int rc = RTFileSeek(hFile, 0, RTFILE_SEEK_CURRENT, &offOld);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Perform a binary search for the max file size.
     */
    uint64_t offLow  =       0;
    uint64_t offHigh = 8 * _1T; /* we don't need bigger files */
    /** @todo Unfortunately this does not work for certain file system types,
     * for instance cifs mounts. Even worse, statvfs.f_fsid returns 0 for such
     * file systems. */
    //uint64_t offHigh = INT64_MAX;
    for (;;)
    {
        uint64_t cbInterval = (offHigh - offLow) >> 1;
        if (cbInterval == 0)
        {
            if (pcbMax)
                *pcbMax = offLow;
            return RTFileSeek(hFile, offOld, RTFILE_SEEK_BEGIN, NULL);
        }

        rc = RTFileSeek(hFile, offLow + cbInterval, RTFILE_SEEK_BEGIN, NULL);
        if (RT_FAILURE(rc))
            offHigh = offLow + cbInterval;
        else
            offLow  = offLow + cbInterval;
    }
}


RTR3DECL(bool) RTFileIsValid(RTFILE hFile)
{
    if (hFile != NIL_RTFILE)
    {
        int fFlags = fcntl(RTFileToNative(hFile), F_GETFD);
        if (fFlags >= 0)
            return true;
    }
    return false;
}


RTR3DECL(int)  RTFileFlush(RTFILE hFile)
{
    if (fsync(RTFileToNative(hFile)))
        return RTErrConvertFromErrno(errno);
    return VINF_SUCCESS;
}


RTR3DECL(int) RTFileIoCtl(RTFILE hFile, unsigned long ulRequest, void *pvData, unsigned cbData, int *piRet)
{
    NOREF(cbData);
    int rc = ioctl(RTFileToNative(hFile), ulRequest, pvData);
    if (piRet)
        *piRet = rc;
    return rc >= 0 ? VINF_SUCCESS : RTErrConvertFromErrno(errno);
}


RTR3DECL(int) RTFileSetMode(RTFILE hFile, RTFMODE fMode)
{
    /*
     * Normalize the mode and call the API.
     */
    fMode = rtFsModeNormalize(fMode, NULL, 0);
    if (!rtFsModeIsValid(fMode))
        return VERR_INVALID_PARAMETER;

    if (fchmod(RTFileToNative(hFile), fMode & RTFS_UNIX_MASK))
    {
        int rc = RTErrConvertFromErrno(errno);
        Log(("RTFileSetMode(%RTfile,%RTfmode): returns %Rrc\n", hFile, fMode, rc));
        return rc;
    }
    return VINF_SUCCESS;
}


RTDECL(int) RTFileSetOwner(RTFILE hFile, uint32_t uid, uint32_t gid)
{
    uid_t uidNative = uid != NIL_RTUID ? (uid_t)uid : (uid_t)-1;
    AssertReturn(uid == uidNative, VERR_INVALID_PARAMETER);
    gid_t gidNative = gid != NIL_RTGID ? (gid_t)gid : (gid_t)-1;
    AssertReturn(gid == gidNative, VERR_INVALID_PARAMETER);

    if (fchown(RTFileToNative(hFile), uidNative, gidNative))
        return RTErrConvertFromErrno(errno);
    return VINF_SUCCESS;
}


RTR3DECL(int) RTFileRename(const char *pszSrc, const char *pszDst, unsigned fRename)
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
     * Take common cause with RTPathRename.
     */
    int rc = rtPathPosixRename(pszSrc, pszDst, fRename, RTFS_TYPE_FILE);

    LogFlow(("RTDirRename(%p:{%s}, %p:{%s}, %#x): returns %Rrc\n",
             pszSrc, pszSrc, pszDst, pszDst, fRename, rc));
    return rc;
}

