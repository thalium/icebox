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
#include "internal/dvm.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * The internal data of a DVM volume I/O stream.
 */
typedef struct RTVFSDVMFILE
{
    /** The volume the VFS file belongs to. */
    RTDVMVOLUME    hVol;
    /** Current position. */
    uint64_t       offCurPos;
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
 * @interface_method_impl{RTVFSOBJOPS,pfnQueryInfo}
 */
static DECLCALLBACK(int) rtDvmVfsFile_QueryInfo(void *pvThis, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAddAttr)
{
    NOREF(pvThis);
    NOREF(pObjInfo);
    NOREF(enmAddAttr);
    return VERR_NOT_SUPPORTED;
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


RTDECL(int) RTDvmVolumeCreateVfsFile(RTDVMVOLUME hVol, PRTVFSFILE phVfsFileOut)
{
    AssertPtrReturn(hVol, VERR_INVALID_HANDLE);
    AssertPtrReturn(phVfsFileOut, VERR_INVALID_POINTER);

    uint32_t cRefs = RTDvmVolumeRetain(hVol);
    AssertReturn(cRefs != UINT32_MAX, VERR_INVALID_HANDLE);

    /*
     * Create the volume file.
     */
    RTVFSFILE       hVfsFile;
    PRTVFSDVMFILE   pThis;
    int rc = RTVfsNewFile(&g_rtDvmVfsStdFileOps, sizeof(*pThis), RTFILE_O_OPEN | RTFILE_O_READ | RTFILE_O_WRITE,
                          NIL_RTVFS, NIL_RTVFSLOCK, &hVfsFile, (void **)&pThis);
    if (RT_SUCCESS(rc))
    {
        pThis->offCurPos = 0;
        pThis->hVol      = hVol;

        *phVfsFileOut = hVfsFile;
        return VINF_SUCCESS;
    }
    else
        RTDvmVolumeRelease(hVol);
    return rc;
}

