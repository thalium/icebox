/* $Id: dvmvfs.cpp $ */
/** @file
 * IPRT Disk Volume Management API (DVM) - VFS glue.
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
#define LOG_GROUP RTLOGGROUP_FS /** @todo fix log group  */
#include <iprt/types.h>
#include <iprt/assert.h>
#include <iprt/mem.h>
#include <iprt/dvm.h>
#include <iprt/err.h>
#include <iprt/asm.h>
#include <iprt/string.h>
#include <iprt/file.h>
#include <iprt/sg.h>
#include <iprt/vfslowlevel.h>
#include <iprt/poll.h>
#include <iprt/log.h>
#include "internal/dvm.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * A volume manager VFS for use in chains (thing pseudo/devfs).
 */
typedef struct RTDVMVFSVOL
{
    /** The volume manager. */
    RTDVM           hVolMgr;
    /** Whether to close it on success. */
    bool            fCloseDvm;
    /** Whether the access is read-only. */
    bool            fReadOnly;
    /** Number of volumes. */
    uint32_t        cVolumes;
    /** Self reference. */
    RTVFS           hVfsSelf;
} RTDVMVFSVOL;
/** Poitner to a volume manager VFS. */
typedef RTDVMVFSVOL *PRTDVMVFSVOL;

/**
 * The volume manager VFS (root) dir data.
 */
typedef struct RTDVMVFSDIR
{
    /** Pointer to the VFS volume. */
    PRTDVMVFSVOL    pVfsVol;
    /** The current directory offset. */
    uint32_t        offDir;
    /** Set if we need to try return hCurVolume again because of buffer overflow. */
    bool            fReturnCurrent;
    /** The current DVM volume. */
    RTDVMVOLUME     hCurVolume;
} RTDVMVFSDIR;
/** Poitner to a volume manager VFS (root) dir. */
typedef RTDVMVFSDIR *PRTDVMVFSDIR;

/**
 * The internal data of a DVM volume I/O stream.
 */
typedef struct RTVFSDVMFILE
{
    /** The volume the VFS file belongs to. */
    RTDVMVOLUME     hVol;
    /** Pointer to the VFS volume.  Can be NULL. */
    PRTDVMVFSVOL    pVfsVol;
    /** Current position. */
    uint64_t        offCurPos;
    /** Set if readable. */
    bool            fCanRead;
    /** Set if writable. */
    bool            fCanWrite;
} RTVFSDVMFILE;
/** Pointer to a the internal data of a DVM volume file. */
typedef RTVFSDVMFILE *PRTVFSDVMFILE;


/**
 * @interface_method_impl{RTVFSOBJOPS,pfnClose}
 */
static DECLCALLBACK(int) rtDvmVfsFile_Close(void *pvThis)
{
    PRTVFSDVMFILE pThis = (PRTVFSDVMFILE)pvThis;

    RTDvmVolumeRelease(pThis->hVol);
    return VINF_SUCCESS;
}


/**
 * Worker for rtDvmVfsFile_QueryInfo, rtDvmVfsDir_QueryEntryInfo, and
 * rtDvmVfsDir_ReadDir.
 */
static int rtDvmVfsFile_QueryInfoWorker(RTDVMVOLUME hVolume, RTDVM hVolMgr, bool fReadOnly,
                                        PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAddAttr)
{

    pObjInfo->cbObject    = RTDvmVolumeGetSize(hVolume);
    pObjInfo->cbAllocated = pObjInfo->cbObject;
    RTTimeSpecSetNano(&pObjInfo->AccessTime, 0);
    RTTimeSpecSetNano(&pObjInfo->ModificationTime, 0);
    RTTimeSpecSetNano(&pObjInfo->ChangeTime, 0);
    RTTimeSpecSetNano(&pObjInfo->BirthTime, 0);
    pObjInfo->Attr.fMode = RTFS_TYPE_FILE | RTFS_DOS_NT_NORMAL;
    if (fReadOnly)
        pObjInfo->Attr.fMode |= RTFS_DOS_READONLY | 0444;
    else
        pObjInfo->Attr.fMode |= 0666;

    switch (enmAddAttr)
    {
        case RTFSOBJATTRADD_NOTHING:
        case RTFSOBJATTRADD_UNIX:
            pObjInfo->Attr.u.Unix.uid           = (RTUID)RTDvmVolumeGetType(hVolume);
            pObjInfo->Attr.u.Unix.gid           = hVolMgr != NIL_RTDVM ? (RTGID)RTDvmMapGetFormatType(hVolMgr) : NIL_RTGID;
            pObjInfo->Attr.u.Unix.cHardlinks    = 1;
            pObjInfo->Attr.u.Unix.INodeIdDevice = 0;
            pObjInfo->Attr.u.Unix.INodeId       = 0;
            pObjInfo->Attr.u.Unix.fFlags        = 0;
            pObjInfo->Attr.u.Unix.GenerationId  = 0;
            pObjInfo->Attr.u.Unix.Device        = 0;
            break;

        case RTFSOBJATTRADD_UNIX_OWNER:
        {
            RTDVMVOLTYPE enmType = RTDvmVolumeGetType(hVolume);
            pObjInfo->Attr.u.UnixOwner.uid = (RTUID)enmType;
            RTStrCopy(pObjInfo->Attr.u.UnixOwner.szName, sizeof(pObjInfo->Attr.u.UnixOwner.szName),
                      RTDvmVolumeTypeGetDescr(enmType));
            break;
        }

        case RTFSOBJATTRADD_UNIX_GROUP:
            if (hVolMgr != NIL_RTDVM)
            {
                pObjInfo->Attr.u.UnixGroup.gid  = (RTGID)RTDvmMapGetFormatType(hVolMgr);
                RTStrCopy(pObjInfo->Attr.u.UnixGroup.szName, sizeof(pObjInfo->Attr.u.UnixGroup.szName),
                          RTDvmMapGetFormatName(hVolMgr));
            }
            else
            {
                pObjInfo->Attr.u.UnixGroup.gid  = NIL_RTGID;
                pObjInfo->Attr.u.UnixGroup.szName[0] = '\0';
            }
            break;

        case RTFSOBJATTRADD_EASIZE:
            pObjInfo->Attr.u.EASize.cb = 0;
            break;

        default:
            return VERR_INVALID_PARAMETER;
    }
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{RTVFSOBJOPS,pfnQueryInfo}
 */
static DECLCALLBACK(int) rtDvmVfsFile_QueryInfo(void *pvThis, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAddAttr)
{
    PRTVFSDVMFILE pThis = (PRTVFSDVMFILE)pvThis;
    return rtDvmVfsFile_QueryInfoWorker(pThis->hVol,
                                        pThis->pVfsVol ? pThis->pVfsVol->hVolMgr   : NIL_RTDVM,
                                        pThis->pVfsVol ? pThis->pVfsVol->fReadOnly : !pThis->fCanWrite,
                                        pObjInfo, enmAddAttr);
}


/**
 * @interface_method_impl{RTVFSIOSTREAMOPS,pfnRead}
 */
static DECLCALLBACK(int) rtDvmVfsFile_Read(void *pvThis, RTFOFF off, PCRTSGBUF pSgBuf, bool fBlocking, size_t *pcbRead)
{
    PRTVFSDVMFILE pThis = (PRTVFSDVMFILE)pvThis;
    int rc = VINF_SUCCESS;

    Assert(pSgBuf->cSegs == 1);
    NOREF(fBlocking);

    /*
     * Find the current position and check if it's within the volume.
     */
    uint64_t offUnsigned = off < 0 ? pThis->offCurPos : (uint64_t)off;
    if (offUnsigned >= RTDvmVolumeGetSize(pThis->hVol))
    {
        if (pcbRead)
        {
            *pcbRead = 0;
            pThis->offCurPos = offUnsigned;
            return VINF_EOF;
        }
        return VERR_EOF;
    }

    size_t cbLeftToRead;
    if (offUnsigned + pSgBuf->paSegs[0].cbSeg > RTDvmVolumeGetSize(pThis->hVol))
    {
        if (!pcbRead)
            return VERR_EOF;
        *pcbRead = cbLeftToRead = (size_t)(RTDvmVolumeGetSize(pThis->hVol) - offUnsigned);
    }
    else
    {
        cbLeftToRead = pSgBuf->paSegs[0].cbSeg;
        if (pcbRead)
            *pcbRead = cbLeftToRead;
    }

    /*
     * Ok, we've got a valid stretch within the file.  Do the reading.
     */
    if (cbLeftToRead > 0)
    {
        rc = RTDvmVolumeRead(pThis->hVol, (uint64_t)off, pSgBuf->paSegs[0].pvSeg, cbLeftToRead);
        if (RT_SUCCESS(rc))
            offUnsigned += cbLeftToRead;
    }

    pThis->offCurPos = offUnsigned;
    return rc;
}


/**
 * @interface_method_impl{RTVFSIOSTREAMOPS,pfnWrite}
 */
static DECLCALLBACK(int) rtDvmVfsFile_Write(void *pvThis, RTFOFF off, PCRTSGBUF pSgBuf, bool fBlocking, size_t *pcbWritten)
{
    PRTVFSDVMFILE pThis = (PRTVFSDVMFILE)pvThis;
    int rc = VINF_SUCCESS;

    Assert(pSgBuf->cSegs == 1);
    NOREF(fBlocking);

    /*
     * Find the current position and check if it's within the volume.
     * Writing beyond the end of a volume is not supported.
     */
    uint64_t offUnsigned = off < 0 ? pThis->offCurPos : (uint64_t)off;
    if (offUnsigned >= RTDvmVolumeGetSize(pThis->hVol))
    {
        if (pcbWritten)
        {
            *pcbWritten = 0;
            pThis->offCurPos = offUnsigned;
        }
        return VERR_NOT_SUPPORTED;
    }

    size_t cbLeftToWrite;
    if (offUnsigned + pSgBuf->paSegs[0].cbSeg > RTDvmVolumeGetSize(pThis->hVol))
    {
        if (!pcbWritten)
            return VERR_EOF;
        *pcbWritten = cbLeftToWrite = (size_t)(RTDvmVolumeGetSize(pThis->hVol) - offUnsigned);
    }
    else
    {
        cbLeftToWrite = pSgBuf->paSegs[0].cbSeg;
        if (pcbWritten)
            *pcbWritten = cbLeftToWrite;
    }

    /*
     * Ok, we've got a valid stretch within the file.  Do the reading.
     */
    if (cbLeftToWrite > 0)
    {
        rc = RTDvmVolumeWrite(pThis->hVol, (uint64_t)off, pSgBuf->paSegs[0].pvSeg, cbLeftToWrite);
        if (RT_SUCCESS(rc))
            offUnsigned += cbLeftToWrite;
    }

    pThis->offCurPos = offUnsigned;
    return rc;
}


/**
 * @interface_method_impl{RTVFSIOSTREAMOPS,pfnFlush}
 */
static DECLCALLBACK(int) rtDvmVfsFile_Flush(void *pvThis)
{
    NOREF(pvThis);
    return VINF_SUCCESS; /** @todo Implement missing DVM API. */
}


/**
 * @interface_method_impl{RTVFSIOSTREAMOPS,pfnPollOne}
 */
static DECLCALLBACK(int) rtDvmVfsFile_PollOne(void *pvThis, uint32_t fEvents, RTMSINTERVAL cMillies, bool fIntr,
                                              uint32_t *pfRetEvents)
{
    NOREF(pvThis);
    int rc;
    if (fEvents != RTPOLL_EVT_ERROR)
    {
        *pfRetEvents = fEvents & ~RTPOLL_EVT_ERROR;
        rc = VINF_SUCCESS;
    }
    else
        rc = RTVfsUtilDummyPollOne(fEvents, cMillies, fIntr, pfRetEvents);
    return rc;
}


/**
 * @interface_method_impl{RTVFSIOSTREAMOPS,pfnTell}
 */
static DECLCALLBACK(int) rtDvmVfsFile_Tell(void *pvThis, PRTFOFF poffActual)
{
    PRTVFSDVMFILE pThis = (PRTVFSDVMFILE)pvThis;
    *poffActual = pThis->offCurPos;
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{RTVFSOBJSETOPS,pfnMode}
 */
static DECLCALLBACK(int) rtDvmVfsFile_SetMode(void *pvThis, RTFMODE fMode, RTFMODE fMask)
{
    NOREF(pvThis);
    NOREF(fMode);
    NOREF(fMask);
    return VERR_NOT_SUPPORTED;
}


/**
 * @interface_method_impl{RTVFSOBJSETOPS,pfnSetTimes}
 */
static DECLCALLBACK(int) rtDvmVfsFile_SetTimes(void *pvThis, PCRTTIMESPEC pAccessTime, PCRTTIMESPEC pModificationTime,
                                               PCRTTIMESPEC pChangeTime, PCRTTIMESPEC pBirthTime)
{
    NOREF(pvThis);
    NOREF(pAccessTime);
    NOREF(pModificationTime);
    NOREF(pChangeTime);
    NOREF(pBirthTime);
    return VERR_NOT_SUPPORTED;
}


/**
 * @interface_method_impl{RTVFSOBJSETOPS,pfnSetOwner}
 */
static DECLCALLBACK(int) rtDvmVfsFile_SetOwner(void *pvThis, RTUID uid, RTGID gid)
{
    NOREF(pvThis);
    NOREF(uid);
    NOREF(gid);
    return VERR_NOT_SUPPORTED;
}


/**
 * @interface_method_impl{RTVFSFILEOPS,pfnSeek}
 */
static DECLCALLBACK(int) rtDvmVfsFile_Seek(void *pvThis, RTFOFF offSeek, unsigned uMethod, PRTFOFF poffActual)
{
    PRTVFSDVMFILE pThis = (PRTVFSDVMFILE)pvThis;

    /*
     * Seek relative to which position.
     */
    uint64_t offWrt;
    switch (uMethod)
    {
        case RTFILE_SEEK_BEGIN:
            offWrt = 0;
            break;

        case RTFILE_SEEK_CURRENT:
            offWrt = pThis->offCurPos;
            break;

        case RTFILE_SEEK_END:
            offWrt = RTDvmVolumeGetSize(pThis->hVol);
            break;

        default:
            return VERR_INTERNAL_ERROR_5;
    }

    /*
     * Calc new position, take care to stay within bounds.
     *
     * @todo: Setting position beyond the end of the volume does not make sense.
     */
    uint64_t offNew;
    if (offSeek == 0)
        offNew = offWrt;
    else if (offSeek > 0)
    {
        offNew = offWrt + offSeek;
        if (   offNew < offWrt
            || offNew > RTFOFF_MAX)
            offNew = RTFOFF_MAX;
    }
    else if ((uint64_t)-offSeek < offWrt)
        offNew = offWrt + offSeek;
    else
        offNew = 0;

    /*
     * Update the state and set return value.
     */
    pThis->offCurPos = offNew;

    *poffActual = offNew;
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{RTVFSFILEOPS,pfnQuerySize}
 */
static DECLCALLBACK(int) rtDvmVfsFile_QuerySize(void *pvThis, uint64_t *pcbFile)
{
    PRTVFSDVMFILE pThis = (PRTVFSDVMFILE)pvThis;
    *pcbFile = RTDvmVolumeGetSize(pThis->hVol);
    return VINF_SUCCESS;
}


/**
 * Standard file operations.
 */
DECL_HIDDEN_CONST(const RTVFSFILEOPS) g_rtDvmVfsStdFileOps =
{
    { /* Stream */
        { /* Obj */
            RTVFSOBJOPS_VERSION,
            RTVFSOBJTYPE_FILE,
            "DvmFile",
            rtDvmVfsFile_Close,
            rtDvmVfsFile_QueryInfo,
            RTVFSOBJOPS_VERSION
        },
        RTVFSIOSTREAMOPS_VERSION,
        RTVFSIOSTREAMOPS_FEAT_NO_SG,
        rtDvmVfsFile_Read,
        rtDvmVfsFile_Write,
        rtDvmVfsFile_Flush,
        rtDvmVfsFile_PollOne,
        rtDvmVfsFile_Tell,
        NULL /*Skip*/,
        NULL /*ZeroFill*/,
        RTVFSIOSTREAMOPS_VERSION,
    },
    RTVFSFILEOPS_VERSION,
    /*RTVFSIOFILEOPS_FEAT_NO_AT_OFFSET*/ 0,
    { /* ObjSet */
        RTVFSOBJSETOPS_VERSION,
        RT_OFFSETOF(RTVFSFILEOPS, Stream.Obj) - RT_OFFSETOF(RTVFSFILEOPS, ObjSet),
        rtDvmVfsFile_SetMode,
        rtDvmVfsFile_SetTimes,
        rtDvmVfsFile_SetOwner,
        RTVFSOBJSETOPS_VERSION
    },
    rtDvmVfsFile_Seek,
    rtDvmVfsFile_QuerySize,
    RTVFSFILEOPS_VERSION
};


/**
 * Internal worker for RTDvmVolumeCreateVfsFile and rtDvmVfsDir_OpenFile.
 *
 * @returns IPRT status code.
 * @param   pVfsVol         The VFS volume, optional.
 * @param   hVol            The volume handle. (Reference not consumed.)
 * @param   fOpen           RTFILE_O_XXX (valid).
 * @param   phVfsFileOut    Where to return the handle to the file.
 */
static int rtDvmVfsCreateFileForVolume(PRTDVMVFSVOL pVfsVol, RTDVMVOLUME hVol, uint64_t fOpen, PRTVFSFILE phVfsFileOut)
{
    uint32_t cRefs = RTDvmVolumeRetain(hVol);
    AssertReturn(cRefs != UINT32_MAX, VERR_INVALID_HANDLE);

    /*
     * Create the volume file.
     */
    RTVFSFILE       hVfsFile;
    PRTVFSDVMFILE   pThis;
    int rc = RTVfsNewFile(&g_rtDvmVfsStdFileOps, sizeof(*pThis), fOpen, NIL_RTVFS, NIL_RTVFSLOCK, &hVfsFile, (void **)&pThis);
    if (RT_SUCCESS(rc))
    {
        pThis->offCurPos = 0;
        pThis->hVol      = hVol;
        pThis->fCanRead  = RT_BOOL(fOpen & RTFILE_O_READ);
        pThis->fCanWrite = RT_BOOL(fOpen & RTFILE_O_WRITE);
        pThis->pVfsVol   = pVfsVol;

        *phVfsFileOut = hVfsFile;
        return VINF_SUCCESS;
    }

    RTDvmVolumeRelease(hVol);
    return rc;
}


RTDECL(int) RTDvmVolumeCreateVfsFile(RTDVMVOLUME hVol, uint64_t fOpen, PRTVFSFILE phVfsFileOut)
{
    AssertPtrReturn(hVol, VERR_INVALID_HANDLE);
    AssertPtrReturn(phVfsFileOut, VERR_INVALID_POINTER);
    AssertReturn(fOpen & RTFILE_O_ACCESS_MASK, VERR_INVALID_FLAGS);
    AssertReturn(!(fOpen & ~RTFILE_O_VALID_MASK), VERR_INVALID_FLAGS);
    return rtDvmVfsCreateFileForVolume(NULL, hVol, fOpen, phVfsFileOut);
}


/**
 * @interface_method_impl{RTVFSOBJOPS,pfnClose}
 */
static DECLCALLBACK(int) rtDvmVfsDir_Close(void *pvThis)
{
    PRTDVMVFSDIR pThis = (PRTDVMVFSDIR)pvThis;

    if (pThis->hCurVolume != NIL_RTDVMVOLUME)
    {
        RTDvmVolumeRelease(pThis->hCurVolume);
        pThis->hCurVolume = NIL_RTDVMVOLUME;
    }

    pThis->pVfsVol = NULL;

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{RTVFSOBJOPS,pfnQueryInfo}
 */
static DECLCALLBACK(int) rtDvmVfsDir_QueryInfo(void *pvThis, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAddAttr)
{
    PRTDVMVFSDIR pThis = (PRTDVMVFSDIR)pvThis;
    pObjInfo->cbObject    = pThis->pVfsVol->cVolumes;
    pObjInfo->cbAllocated = pThis->pVfsVol->cVolumes;
    RTTimeSpecSetNano(&pObjInfo->AccessTime, 0);
    RTTimeSpecSetNano(&pObjInfo->ModificationTime, 0);
    RTTimeSpecSetNano(&pObjInfo->ChangeTime, 0);
    RTTimeSpecSetNano(&pObjInfo->BirthTime, 0);
    pObjInfo->Attr.fMode = RTFS_TYPE_DIRECTORY | RTFS_DOS_DIRECTORY;
    if (pThis->pVfsVol->fReadOnly)
        pObjInfo->Attr.fMode |= RTFS_DOS_READONLY | 0555;
    else
        pObjInfo->Attr.fMode |= 0777;

    switch (enmAddAttr)
    {
        case RTFSOBJATTRADD_NOTHING:
        case RTFSOBJATTRADD_UNIX:
            pObjInfo->Attr.u.Unix.uid           = NIL_RTUID;
            pObjInfo->Attr.u.Unix.gid           = (RTGID)RTDvmMapGetFormatType(pThis->pVfsVol->hVolMgr);
            pObjInfo->Attr.u.Unix.cHardlinks    = pThis->pVfsVol->cVolumes;
            pObjInfo->Attr.u.Unix.INodeIdDevice = 0;
            pObjInfo->Attr.u.Unix.INodeId       = 0;
            pObjInfo->Attr.u.Unix.fFlags        = 0;
            pObjInfo->Attr.u.Unix.GenerationId  = 0;
            pObjInfo->Attr.u.Unix.Device        = 0;
            break;

        case RTFSOBJATTRADD_UNIX_OWNER:
            pObjInfo->Attr.u.UnixOwner.uid      = NIL_RTUID;
            pObjInfo->Attr.u.UnixOwner.szName[0] = '\0';
            break;

        case RTFSOBJATTRADD_UNIX_GROUP:
            pObjInfo->Attr.u.UnixGroup.gid      = (RTGID)RTDvmMapGetFormatType(pThis->pVfsVol->hVolMgr);
            RTStrCopy(pObjInfo->Attr.u.UnixGroup.szName, sizeof(pObjInfo->Attr.u.UnixGroup.szName),
                      RTDvmMapGetFormatName(pThis->pVfsVol->hVolMgr));
            break;

        case RTFSOBJATTRADD_EASIZE:
            pObjInfo->Attr.u.EASize.cb = 0;
            break;

        default:
            return VERR_INVALID_PARAMETER;
    }
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{RTVFSOBJSETOPS,pfnMode}
 */
static DECLCALLBACK(int) rtDvmVfsDir_SetMode(void *pvThis, RTFMODE fMode, RTFMODE fMask)
{
    NOREF(pvThis); NOREF(fMode); NOREF(fMask);
    return VERR_NOT_SUPPORTED;
}


/**
 * @interface_method_impl{RTVFSOBJSETOPS,pfnSetTimes}
 */
static DECLCALLBACK(int) rtDvmVfsDir_SetTimes(void *pvThis, PCRTTIMESPEC pAccessTime, PCRTTIMESPEC pModificationTime,
                                             PCRTTIMESPEC pChangeTime, PCRTTIMESPEC pBirthTime)
{
    NOREF(pvThis); NOREF(pAccessTime); NOREF(pModificationTime); NOREF(pChangeTime); NOREF(pBirthTime);
    return VERR_NOT_SUPPORTED;
}


/**
 * @interface_method_impl{RTVFSOBJSETOPS,pfnSetOwner}
 */
static DECLCALLBACK(int) rtDvmVfsDir_SetOwner(void *pvThis, RTUID uid, RTGID gid)
{
    RT_NOREF(pvThis, uid, gid);
    return VERR_NOT_SUPPORTED;
}


/**
 * @interface_method_impl{RTVFSOBJOPS,pfnTraversalOpen}
 */
static DECLCALLBACK(int) rtDvmVfsDir_TraversalOpen(void *pvThis, const char *pszEntry, PRTVFSDIR phVfsDir,
                                                  PRTVFSSYMLINK phVfsSymlink, PRTVFS phVfsMounted)
{
    RT_NOREF(pvThis, pszEntry);

    /* We don't do any subdirs. */
    if (phVfsSymlink)
        *phVfsSymlink = NIL_RTVFSSYMLINK;
    if (phVfsMounted)
        *phVfsMounted = NIL_RTVFS;
    if (phVfsDir)
        *phVfsDir = NIL_RTVFSDIR;
    return VERR_PATH_NOT_FOUND;
}


static int rtDvmVfsDir_FindEntry(PRTDVMVFSDIR pThis, const char *pszEntry, PRTDVMVOLUME phVolume)
{
    /*
     * Enumerate the volumes and try match the volume name.
     */
    int rc;
    PRTDVMVFSVOL pVfsVol = pThis->pVfsVol;
    if (pVfsVol->cVolumes > 0)
    {
        /* The first volume. */
        uint32_t iVol = 0;
        RTDVMVOLUME hVol;
        rc = RTDvmMapQueryFirstVolume(pThis->pVfsVol->hVolMgr, &hVol);
        while (RT_SUCCESS(rc))
        {
            /* Match the name. */
            bool  fMatch;
            char *pszVolName;
            rc = RTDvmVolumeQueryName(hVol, &pszVolName);
            if (RT_SUCCESS(rc))
            {
                fMatch = RTStrCmp(pszEntry, pszVolName) == 0 && *pszVolName != '\0';
                RTStrFree(pszVolName);
            }
            else if (rc == VERR_NOT_SUPPORTED)
                fMatch = false;
            else
            {
                RTDvmVolumeRelease(hVol);
                break;
            }

            /* Match the sequential volume number. */
            if (!fMatch)
            {
                char szTmp[16];
                RTStrPrintf(szTmp, sizeof(szTmp), "vol%u", iVol);
                fMatch = RTStrCmp(pszEntry, szTmp) == 0;
            }

            if (fMatch)
            {
                *phVolume = hVol;
                return VINF_SUCCESS;
            }

            /* More volumes? */
            iVol++;
            if (iVol >= pVfsVol->cVolumes)
            {
                RTDvmVolumeRelease(hVol);
                rc = VERR_FILE_NOT_FOUND;
                break;
            }

            /* Get the next volume. */
            RTDVMVOLUME hVolNext;
            rc = RTDvmMapQueryNextVolume(pThis->pVfsVol->hVolMgr, hVol, &hVolNext);
            RTDvmVolumeRelease(hVol);
            hVol = hVolNext;
        }
    }
    else
        rc = VERR_FILE_NOT_FOUND;
    return rc;
}


/**
 * @interface_method_impl{RTVFSDIROPS,pfnOpenFile}
 */
static DECLCALLBACK(int) rtDvmVfsDir_OpenFile(void *pvThis, const char *pszFilename, uint32_t fOpen, PRTVFSFILE phVfsFile)
{
    PRTDVMVFSDIR pThis = (PRTDVMVFSDIR)pvThis;
    RTDVMVOLUME  hVolume;
    int rc = rtDvmVfsDir_FindEntry(pThis, pszFilename, &hVolume);
    if (RT_SUCCESS(rc))
    {
        if (   (fOpen & RTFILE_O_ACTION_MASK) == RTFILE_O_OPEN
            || (fOpen & RTFILE_O_ACTION_MASK) == RTFILE_O_OPEN_CREATE
            || (fOpen & RTFILE_O_ACTION_MASK) == RTFILE_O_CREATE_REPLACE)
        {
            if (   !(fOpen & RTFILE_O_WRITE)
                || !pThis->pVfsVol->fReadOnly)
                rc = rtDvmVfsCreateFileForVolume(pThis->pVfsVol, hVolume, fOpen, phVfsFile);
            else
                rc = VERR_WRITE_PROTECT;
        }
        else
            rc = VERR_ALREADY_EXISTS;
        RTDvmVolumeRelease(hVolume);
    }
    return rc;
}


/**
 * @interface_method_impl{RTVFSDIROPS,pfnOpenDir}
 */
static DECLCALLBACK(int) rtDvmVfsDir_OpenDir(void *pvThis, const char *pszSubDir, uint32_t fFlags, PRTVFSDIR phVfsDir)
{
    NOREF(pvThis); NOREF(pszSubDir); NOREF(fFlags); NOREF(phVfsDir);
    return VERR_FILE_NOT_FOUND;
}


/**
 * @interface_method_impl{RTVFSDIROPS,pfnCreateDir}
 */
static DECLCALLBACK(int) rtDvmVfsDir_CreateDir(void *pvThis, const char *pszSubDir, RTFMODE fMode, PRTVFSDIR phVfsDir)
{
    RT_NOREF(pvThis, pszSubDir, fMode, phVfsDir);
    return VERR_NOT_SUPPORTED;
}


/**
 * @interface_method_impl{RTVFSDIROPS,pfnOpenSymlink}
 */
static DECLCALLBACK(int) rtDvmVfsDir_OpenSymlink(void *pvThis, const char *pszSymlink, PRTVFSSYMLINK phVfsSymlink)
{
    RT_NOREF(pvThis, pszSymlink, phVfsSymlink);
    return VERR_NOT_SUPPORTED;
}


/**
 * @interface_method_impl{RTVFSDIROPS,pfnCreateSymlink}
 */
static DECLCALLBACK(int) rtDvmVfsDir_CreateSymlink(void *pvThis, const char *pszSymlink, const char *pszTarget,
                                                  RTSYMLINKTYPE enmType, PRTVFSSYMLINK phVfsSymlink)
{
    RT_NOREF(pvThis, pszSymlink, pszTarget, enmType, phVfsSymlink);
    return VERR_NOT_SUPPORTED;
}


/**
 * @interface_method_impl{RTVFSDIROPS,pfnQueryEntryInfo}
 */
static DECLCALLBACK(int) rtDvmVfsDir_QueryEntryInfo(void *pvThis, const char *pszEntry,
                                                    PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAddAttr)
{
    PRTDVMVFSDIR pThis = (PRTDVMVFSDIR)pvThis;
    RTDVMVOLUME  hVolume;
    int rc = rtDvmVfsDir_FindEntry(pThis, pszEntry, &hVolume);
    if (RT_SUCCESS(rc))
    {
        rc = rtDvmVfsFile_QueryInfoWorker(hVolume, pThis->pVfsVol->hVolMgr, pThis->pVfsVol->fReadOnly, pObjInfo, enmAddAttr);
        RTDvmVolumeRelease(hVolume);
    }
    return rc;
}


/**
 * @interface_method_impl{RTVFSDIROPS,pfnUnlinkEntry}
 */
static DECLCALLBACK(int) rtDvmVfsDir_UnlinkEntry(void *pvThis, const char *pszEntry, RTFMODE fType)
{
    RT_NOREF(pvThis, pszEntry, fType);
    return VERR_NOT_IMPLEMENTED;
}


/**
 * @interface_method_impl{RTVFSDIROPS,pfnRenameEntry}
 */
static DECLCALLBACK(int) rtDvmVfsDir_RenameEntry(void *pvThis, const char *pszEntry, RTFMODE fType, const char *pszNewName)
{
    RT_NOREF(pvThis, pszEntry, fType, pszNewName);
    return VERR_NOT_IMPLEMENTED;
}


/**
 * @interface_method_impl{RTVFSDIROPS,pfnRewindDir}
 */
static DECLCALLBACK(int) rtDvmVfsDir_RewindDir(void *pvThis)
{
    PRTDVMVFSDIR pThis = (PRTDVMVFSDIR)pvThis;

    if (pThis->hCurVolume != NIL_RTDVMVOLUME)
    {
        RTDvmVolumeRelease(pThis->hCurVolume);
        pThis->hCurVolume = NIL_RTDVMVOLUME;
    }
    pThis->fReturnCurrent = false;
    pThis->offDir         = 0;

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{RTVFSDIROPS,pfnReadDir}
 */
static DECLCALLBACK(int) rtDvmVfsDir_ReadDir(void *pvThis, PRTDIRENTRYEX pDirEntry, size_t *pcbDirEntry,
                                            RTFSOBJATTRADD enmAddAttr)
{
    PRTDVMVFSDIR pThis   = (PRTDVMVFSDIR)pvThis;
    PRTDVMVFSVOL pVfsVol = pThis->pVfsVol;
    int          rc;

    /*
     * Get the volume to return info about.
     */
    if (!pThis->fReturnCurrent)
    {
        if (pThis->offDir < pVfsVol->cVolumes)
        {
            RTDVMVOLUME hNextVolume;
            if (pThis->offDir == 0)
                rc = RTDvmMapQueryFirstVolume(pVfsVol->hVolMgr, &hNextVolume);
            else
                rc = RTDvmMapQueryNextVolume(pVfsVol->hVolMgr, pThis->hCurVolume, &hNextVolume);
            if (RT_FAILURE(rc))
                return rc;
            RTDvmVolumeRelease(pThis->hCurVolume);
            pThis->hCurVolume = hNextVolume;
        }
        else
        {
            RTDvmVolumeRelease(pThis->hCurVolume);
            pThis->hCurVolume = NIL_RTDVMVOLUME;
            return VERR_NO_MORE_FILES;
        }
    }

    /*
     * Figure out the name length.
     */
    char szVolNo[16];
    RTStrPrintf(szVolNo, sizeof(szVolNo), "vol%u", pThis->offDir);

    char *pszVolName;
    rc = RTDvmVolumeQueryName(pThis->hCurVolume, &pszVolName);
    if (   RT_SUCCESS(rc)
        || rc == VERR_NOT_SUPPORTED)
    {
        if (rc == VERR_NOT_SUPPORTED)
            pszVolName = szVolNo;
        else if (*pszVolName == '\0')
        {
            RTStrFree(pszVolName);
            pszVolName = szVolNo;
        }

        size_t cchVolName = strlen(pszVolName);
        size_t cbNeeded   = RT_OFFSETOF(RTDIRENTRYEX,  szName[cchVolName + 1]);
        if (cbNeeded <= *pcbDirEntry)
        {
            *pcbDirEntry = cbNeeded;

            /* Do the names. */
            pDirEntry->cbName = (uint16_t)cchVolName;
            memcpy(pDirEntry->szName, pszVolName, cchVolName + 1);
            if (pszVolName != szVolNo)
            {
                RTStrFree(pszVolName);

                PRTUTF16 pwszShortName = pDirEntry->wszShortName;
                size_t   cwcShortName = 0;
                rc = RTStrToUtf16Ex(szVolNo, RTSTR_MAX, &pwszShortName, RT_ELEMENTS(pDirEntry->wszShortName), &cwcShortName);
                AssertRC(rc);
                pDirEntry->cwcShortName = (uint16_t)cwcShortName;
            }
            else
            {
                pDirEntry->cwcShortName = 0;
                pDirEntry->wszShortName[0] = '\0';
            }

            /* Do the rest. */
            rc = rtDvmVfsFile_QueryInfoWorker(pThis->hCurVolume, pVfsVol->hVolMgr, pVfsVol->fReadOnly,
                                              &pDirEntry->Info, enmAddAttr);
            pThis->fReturnCurrent = !RT_SUCCESS(rc);
            pThis->offDir        += RT_SUCCESS(rc);
            return rc;
        }

        *pcbDirEntry = cbNeeded;
        rc = VERR_BUFFER_OVERFLOW;

        if (pszVolName != szVolNo)
            RTStrFree(pszVolName);
    }

    pThis->fReturnCurrent = true;
    return rc;
}


/**
 * DVM (root) directory operations.
 */
static const RTVFSDIROPS g_rtDvmVfsDirOps =
{
    { /* Obj */
        RTVFSOBJOPS_VERSION,
        RTVFSOBJTYPE_DIR,
        "DvmDir",
        rtDvmVfsDir_Close,
        rtDvmVfsDir_QueryInfo,
        RTVFSOBJOPS_VERSION
    },
    RTVFSDIROPS_VERSION,
    0,
    { /* ObjSet */
        RTVFSOBJSETOPS_VERSION,
        RT_OFFSETOF(RTVFSDIROPS, Obj) - RT_OFFSETOF(RTVFSDIROPS, ObjSet),
        rtDvmVfsDir_SetMode,
        rtDvmVfsDir_SetTimes,
        rtDvmVfsDir_SetOwner,
        RTVFSOBJSETOPS_VERSION
    },
    rtDvmVfsDir_TraversalOpen,
    rtDvmVfsDir_OpenFile,
    rtDvmVfsDir_OpenDir,
    rtDvmVfsDir_CreateDir,
    rtDvmVfsDir_OpenSymlink,
    rtDvmVfsDir_CreateSymlink,
    rtDvmVfsDir_QueryEntryInfo,
    rtDvmVfsDir_UnlinkEntry,
    rtDvmVfsDir_RenameEntry,
    rtDvmVfsDir_RewindDir,
    rtDvmVfsDir_ReadDir,
    RTVFSDIROPS_VERSION,
};



/**
 * @interface_method_impl{RTVFSOBJOPS::Obj,pfnClose}
 */
static DECLCALLBACK(int) rtDvmVfsVol_Close(void *pvThis)
{
    PRTDVMVFSVOL pThis = (PRTDVMVFSVOL)pvThis;
    LogFlow(("rtDvmVfsVol_Close(%p)\n", pThis));

    if (   pThis->fCloseDvm
        && pThis->hVolMgr != NIL_RTDVM )
        RTDvmRelease(pThis->hVolMgr);
    pThis->hVolMgr = NIL_RTDVM;

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{RTVFSOBJOPS::Obj,pfnQueryInfo}
 */
static DECLCALLBACK(int) rtDvmVfsVol_QueryInfo(void *pvThis, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAddAttr)
{
    RT_NOREF(pvThis, pObjInfo, enmAddAttr);
    return VERR_WRONG_TYPE;
}


/**
 * @interface_method_impl{RTVFSOPS,pfnOpenRoot}
 */
static DECLCALLBACK(int) rtDvmVfsVol_OpenRoot(void *pvThis, PRTVFSDIR phVfsDir)
{
    PRTDVMVFSVOL pThis = (PRTDVMVFSVOL)pvThis;

    PRTDVMVFSDIR pNewDir;
    int rc = RTVfsNewDir(&g_rtDvmVfsDirOps, sizeof(*pNewDir), 0 /*fFlags*/, pThis->hVfsSelf,
                         NIL_RTVFSLOCK /*use volume lock*/, phVfsDir, (void **)&pNewDir);
    if (RT_SUCCESS(rc))
    {
        pNewDir->offDir         = 0;
        pNewDir->pVfsVol        = pThis;
        pNewDir->fReturnCurrent = false;
        pNewDir->hCurVolume     = NIL_RTDVMVOLUME;
    }
    return rc;
}


/**
 * @interface_method_impl{RTVFSOPS,pfnIsRangeInUse}
 */
static DECLCALLBACK(int) rtDvmVfsVol_IsRangeInUse(void *pvThis, RTFOFF off, size_t cb, bool *pfUsed)
{
    RT_NOREF(pvThis, off, cb, pfUsed);
    return VERR_NOT_IMPLEMENTED;
}


DECL_HIDDEN_CONST(const RTVFSOPS) g_rtDvmVfsVolOps =
{
    { /* Obj */
        RTVFSOBJOPS_VERSION,
        RTVFSOBJTYPE_VFS,
        "DvmVol",
        rtDvmVfsVol_Close,
        rtDvmVfsVol_QueryInfo,
        RTVFSOBJOPS_VERSION
    },
    RTVFSOPS_VERSION,
    0 /* fFeatures */,
    rtDvmVfsVol_OpenRoot,
    rtDvmVfsVol_IsRangeInUse,
    RTVFSOPS_VERSION
};



/**
 * @interface_method_impl{RTVFSCHAINELEMENTREG,pfnValidate}
 */
static DECLCALLBACK(int) rtDvmVfsChain_Validate(PCRTVFSCHAINELEMENTREG pProviderReg, PRTVFSCHAINSPEC pSpec,
                                                PRTVFSCHAINELEMSPEC pElement, uint32_t *poffError, PRTERRINFO pErrInfo)
{
    RT_NOREF(pProviderReg, pSpec);

    /*
     * Basic checks.
     */
    if (pElement->enmTypeIn != RTVFSOBJTYPE_FILE)
        return pElement->enmTypeIn == RTVFSOBJTYPE_INVALID ? VERR_VFS_CHAIN_CANNOT_BE_FIRST_ELEMENT : VERR_VFS_CHAIN_TAKES_FILE;
    if (pElement->enmType != RTVFSOBJTYPE_VFS)
        return VERR_VFS_CHAIN_ONLY_VFS;

    if (pElement->cArgs > 1)
        return VERR_VFS_CHAIN_AT_MOST_ONE_ARG;

    /*
     * Parse the flag if present, save in pElement->uProvider.
     */
    /** @todo allow specifying sector size   */
    bool fReadOnly = (pSpec->fOpenFile & RTFILE_O_ACCESS_MASK) == RTFILE_O_READ;
    if (pElement->cArgs > 0)
    {
        const char *psz = pElement->paArgs[0].psz;
        if (*psz)
        {
            if (   !strcmp(psz, "ro")
                || !strcmp(psz, "r"))
                fReadOnly = true;
            else if (!strcmp(psz, "rw"))
                fReadOnly = false;
            else
            {
                *poffError = pElement->paArgs[0].offSpec;
                return RTErrInfoSet(pErrInfo, VERR_VFS_CHAIN_INVALID_ARGUMENT, "Expected 'ro' or 'rw' as argument");
            }
        }
    }

    pElement->uProvider = fReadOnly;
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{RTVFSCHAINELEMENTREG,pfnInstantiate}
 */
static DECLCALLBACK(int) rtDvmVfsChain_Instantiate(PCRTVFSCHAINELEMENTREG pProviderReg, PCRTVFSCHAINSPEC pSpec,
                                                   PCRTVFSCHAINELEMSPEC pElement, RTVFSOBJ hPrevVfsObj,
                                                   PRTVFSOBJ phVfsObj, uint32_t *poffError, PRTERRINFO pErrInfo)
{
    RT_NOREF(pProviderReg, pSpec, poffError, pErrInfo);
    AssertReturn(hPrevVfsObj != NIL_RTVFSOBJ, VERR_VFS_CHAIN_IPE);

    /*
     * Instantiate the volume manager and open the map stuff.
     */
    RTVFSFILE hPrevVfsFile = RTVfsObjToFile(hPrevVfsObj);
    AssertReturn(hPrevVfsFile != NIL_RTVFSFILE, VERR_VFS_CHAIN_CAST_FAILED);

    RTDVM hVolMgr;
    int rc = RTDvmCreate(&hVolMgr, hPrevVfsFile, 512, 0 /*fFlags*/);
    RTVfsFileRelease(hPrevVfsFile);
    if (RT_SUCCESS(rc))
    {
        rc = RTDvmMapOpen(hVolMgr);
        if (RT_SUCCESS(rc))
        {
            /*
             * Create a VFS instance for the volume manager.
             */
            RTVFS        hVfs  = NIL_RTVFS;
            PRTDVMVFSVOL pThis = NULL;
            rc = RTVfsNew(&g_rtDvmVfsVolOps, sizeof(RTDVMVFSVOL), NIL_RTVFS, RTVFSLOCK_CREATE_RW, &hVfs, (void **)&pThis);
            if (RT_SUCCESS(rc))
            {
                pThis->hVolMgr   = hVolMgr;
                pThis->fCloseDvm = true;
                pThis->fReadOnly = pElement->uProvider == (uint64_t)true;
                pThis->cVolumes  = RTDvmMapGetValidVolumes(hVolMgr);
                pThis->hVfsSelf  = hVfs;

                *phVfsObj = RTVfsObjFromVfs(hVfs);
                RTVfsRelease(hVfs);
                return *phVfsObj != NIL_RTVFSOBJ ? VINF_SUCCESS : VERR_VFS_CHAIN_CAST_FAILED;
            }
        }
        else
            rc = RTErrInfoSetF(pErrInfo, rc, "RTDvmMapOpen failed: %Rrc", rc);
        RTDvmRelease(hVolMgr);
    }
    else
        rc = RTErrInfoSetF(pErrInfo, rc, "RTDvmCreate failed: %Rrc", rc);
    return rc;
}


/**
 * @interface_method_impl{RTVFSCHAINELEMENTREG,pfnCanReuseElement}
 */
static DECLCALLBACK(bool) rtDvmVfsChain_CanReuseElement(PCRTVFSCHAINELEMENTREG pProviderReg,
                                                        PCRTVFSCHAINSPEC pSpec, PCRTVFSCHAINELEMSPEC pElement,
                                                        PCRTVFSCHAINSPEC pReuseSpec, PCRTVFSCHAINELEMSPEC pReuseElement)
{
    RT_NOREF(pProviderReg, pSpec, pElement, pReuseSpec, pReuseElement);
    return false;
}


/** VFS chain element 'file'. */
static RTVFSCHAINELEMENTREG g_rtVfsChainIsoFsVolReg =
{
    /* uVersion = */            RTVFSCHAINELEMENTREG_VERSION,
    /* fReserved = */           0,
    /* pszName = */             "dvm",
    /* ListEntry = */           { NULL, NULL },
    /* pszHelp = */             "Opens a container image using the VD API.\n"
                                "Optionally takes one parameter 'ro' (read only) or 'rw' (read write).\n",
    /* pfnValidate = */         rtDvmVfsChain_Validate,
    /* pfnInstantiate = */      rtDvmVfsChain_Instantiate,
    /* pfnCanReuseElement = */  rtDvmVfsChain_CanReuseElement,
    /* uEndMarker = */          RTVFSCHAINELEMENTREG_VERSION
};

RTVFSCHAIN_AUTO_REGISTER_ELEMENT_PROVIDER(&g_rtVfsChainIsoFsVolReg, rtVfsChainIsoFsVolReg);

