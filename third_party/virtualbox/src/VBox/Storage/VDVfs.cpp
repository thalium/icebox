/* $Id: VDVfs.cpp $ */
/** @file
 * Virtual Disk Container implementation. - VFS glue.
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
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/types.h>
#include <iprt/assert.h>
#include <iprt/mem.h>
#include <iprt/err.h>
#include <iprt/asm.h>
#include <iprt/string.h>
#include <iprt/file.h>
#include <iprt/sg.h>
#include <iprt/vfslowlevel.h>
#include <iprt/poll.h>
#include <VBox/vd.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * The internal data of a DVM volume I/O stream.
 */
typedef struct VDVFSFILE
{
    /** The volume the VFS file belongs to. */
    PVDISK         pDisk;
    /** Current position. */
    uint64_t       offCurPos;
    /** Flags given during creation. */
    uint32_t       fFlags;
} VDVFSFILE;
/** Pointer to a the internal data of a DVM volume file. */
typedef VDVFSFILE *PVDVFSFILE;

/**
 * VD read helper taking care of unaligned accesses.
 *
 * @return  VBox status code.
 * @param   pDisk    VD disk container.
 * @param   off      Offset to start reading from.
 * @param   pvBuf    Pointer to the buffer to read into.
 * @param   cbRead   Amount of bytes to read.
 */
static int vdReadHelper(PVDISK pDisk, uint64_t off, void *pvBuf, size_t cbRead)
{
    int rc = VINF_SUCCESS;

    /* Take shortcut if possible. */
    if (   off % 512 == 0
        && cbRead % 512 == 0)
        rc = VDRead(pDisk, off, pvBuf, cbRead);
    else
    {
        uint8_t *pbBuf = (uint8_t *)pvBuf;
        uint8_t abBuf[512];

        /* Unaligned access, make it aligned. */
        if (off % 512 != 0)
        {
            uint64_t offAligned = off & ~(uint64_t)(512 - 1);
            size_t cbToCopy = 512 - (off - offAligned);
            rc = VDRead(pDisk, offAligned, abBuf, 512);
            if (RT_SUCCESS(rc))
            {
                memcpy(pbBuf, &abBuf[off - offAligned], cbToCopy);
                pbBuf  += cbToCopy;
                off    += cbToCopy;
                cbRead -= cbToCopy;
            }
        }

        if (   RT_SUCCESS(rc)
            && (cbRead & ~(uint64_t)(512 - 1)))
        {
            size_t cbReadAligned = cbRead & ~(uint64_t)(512 - 1);

            Assert(!(off % 512));
            rc = VDRead(pDisk, off, pbBuf, cbReadAligned);
            if (RT_SUCCESS(rc))
            {
                pbBuf  += cbReadAligned;
                off    += cbReadAligned;
                cbRead -= cbReadAligned;
            }
        }

        if (   RT_SUCCESS(rc)
            && cbRead)
        {
            Assert(cbRead < 512);
            Assert(!(off % 512));

            rc = VDRead(pDisk, off, abBuf, 512);
            if (RT_SUCCESS(rc))
                memcpy(pbBuf, abBuf, cbRead);
        }
    }

    return rc;
}


/**
 * VD write helper taking care of unaligned accesses.
 *
 * @return  VBox status code.
 * @param   pDisk    VD disk container.
 * @param   off      Offset to start writing to.
 * @param   pvBuf    Pointer to the buffer to read from.
 * @param   cbWrite  Amount of bytes to write.
 */
static int vdWriteHelper(PVDISK pDisk, uint64_t off, const void *pvBuf, size_t cbWrite)
{
    int rc = VINF_SUCCESS;

    /* Take shortcut if possible. */
    if (   off % 512 == 0
        && cbWrite % 512 == 0)
        rc = VDWrite(pDisk, off, pvBuf, cbWrite);
    else
    {
        uint8_t *pbBuf = (uint8_t *)pvBuf;
        uint8_t abBuf[512];

        /* Unaligned access, make it aligned. */
        if (off % 512 != 0)
        {
            uint64_t offAligned = off & ~(uint64_t)(512 - 1);
            size_t cbToCopy = 512 - (off - offAligned);
            rc = VDRead(pDisk, offAligned, abBuf, 512);
            if (RT_SUCCESS(rc))
            {
                memcpy(&abBuf[off - offAligned], pbBuf, cbToCopy);
                rc = VDWrite(pDisk, offAligned, abBuf, 512);

                pbBuf   += cbToCopy;
                off     += cbToCopy;
                cbWrite -= cbToCopy;
            }
        }

        if (   RT_SUCCESS(rc)
            && (cbWrite & ~(uint64_t)(512 - 1)))
        {
            size_t cbWriteAligned = cbWrite & ~(uint64_t)(512 - 1);

            Assert(!(off % 512));
            rc = VDWrite(pDisk, off, pbBuf, cbWriteAligned);
            if (RT_SUCCESS(rc))
            {
                pbBuf   += cbWriteAligned;
                off     += cbWriteAligned;
                cbWrite -= cbWriteAligned;
            }
        }

        if (   RT_SUCCESS(rc)
            && cbWrite)
        {
            Assert(cbWrite < 512);
            Assert(!(off % 512));

            rc = VDRead(pDisk, off, abBuf, 512);
            if (RT_SUCCESS(rc))
            {
                memcpy(abBuf, pbBuf, cbWrite);
                rc = VDWrite(pDisk, off, abBuf, 512);
            }
        }
    }

    return rc;
}


/**
 * @interface_method_impl{RTVFSOBJOPS,pfnClose}
 */
static DECLCALLBACK(int) vdVfsFile_Close(void *pvThis)
{
    PVDVFSFILE pThis = (PVDVFSFILE)pvThis;

    if (pThis->fFlags & VD_VFSFILE_DESTROY_ON_RELEASE)
        VDDestroy(pThis->pDisk);

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{RTVFSOBJOPS,pfnQueryInfo}
 */
static DECLCALLBACK(int) vdVfsFile_QueryInfo(void *pvThis, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAddAttr)
{
    PVDVFSFILE pThis = (PVDVFSFILE)pvThis;
    unsigned const cOpenImages = VDGetCount(pThis->pDisk);

    pObjInfo->cbObject    = VDGetSize(pThis->pDisk, cOpenImages - 1);
    pObjInfo->cbAllocated = 0;
    for (unsigned iImage = 0; iImage < cOpenImages; iImage++)
        pObjInfo->cbAllocated += VDGetFileSize(pThis->pDisk, iImage);

    /** @todo enumerate the disk images directly...   */
    RTTimeNow(&pObjInfo->AccessTime);
    pObjInfo->BirthTime        = pObjInfo->AccessTime;
    pObjInfo->ChangeTime       = pObjInfo->AccessTime;
    pObjInfo->ModificationTime = pObjInfo->AccessTime;

    pObjInfo->Attr.fMode       = RTFS_DOS_NT_NORMAL | RTFS_TYPE_FILE | 0644;
    pObjInfo->Attr.enmAdditional = enmAddAttr;
    switch (enmAddAttr)
    {
        case RTFSOBJATTRADD_UNIX:
            pObjInfo->Attr.u.Unix.uid           = NIL_RTUID;
            pObjInfo->Attr.u.Unix.gid           = NIL_RTGID;
            pObjInfo->Attr.u.Unix.cHardlinks    = 1;
            pObjInfo->Attr.u.Unix.INodeIdDevice = 0;
            pObjInfo->Attr.u.Unix.INodeId       = 0;
            pObjInfo->Attr.u.Unix.fFlags        = 0;
            pObjInfo->Attr.u.Unix.GenerationId  = 0;
            pObjInfo->Attr.u.Unix.Device        = 0;
            break;

        case RTFSOBJATTRADD_UNIX_OWNER:
            pObjInfo->Attr.u.UnixOwner.uid = NIL_RTUID;
            pObjInfo->Attr.u.UnixOwner.szName[0] = '\0';
            break;
        case RTFSOBJATTRADD_UNIX_GROUP:
            pObjInfo->Attr.u.UnixGroup.gid = NIL_RTGID;
            pObjInfo->Attr.u.UnixGroup.szName[0] = '\0';
            break;
        case RTFSOBJATTRADD_EASIZE:
            pObjInfo->Attr.u.EASize.cb = 0;
            break;

        default:
            AssertFailedReturn(VERR_INVALID_PARAMETER);
    }

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{RTVFSIOSTREAMOPS,pfnRead}
 */
static DECLCALLBACK(int) vdVfsFile_Read(void *pvThis, RTFOFF off, PCRTSGBUF pSgBuf, bool fBlocking, size_t *pcbRead)
{
    PVDVFSFILE pThis = (PVDVFSFILE)pvThis;

    Assert(pSgBuf->cSegs == 1);
    NOREF(fBlocking);

    /*
     * Find the current position and check if it's within the volume.
     */
    uint64_t offUnsigned = off < 0 ? pThis->offCurPos : (uint64_t)off;
    uint64_t const cbImage = VDGetSize(pThis->pDisk, VD_LAST_IMAGE);
    if (offUnsigned >= cbImage)
    {
        if (pcbRead)
        {
            *pcbRead = 0;
            pThis->offCurPos = cbImage;
            return VINF_EOF;
        }
        return VERR_EOF;
    }

    int rc = VINF_SUCCESS;
    size_t cbLeftToRead = pSgBuf->paSegs[0].cbSeg;
    if (offUnsigned + cbLeftToRead <= cbImage)
    {
        if (pcbRead)
            *pcbRead = cbLeftToRead;
    }
    else
    {
        if (!pcbRead)
            return VERR_EOF;
        *pcbRead = cbLeftToRead = (size_t)(cbImage - offUnsigned);
        rc = VINF_EOF;
    }

    /*
     * Ok, we've got a valid stretch within the file.  Do the reading.
     */
    if (cbLeftToRead > 0)
    {
        int rc2 = vdReadHelper(pThis->pDisk, offUnsigned, pSgBuf->paSegs[0].pvSeg, cbLeftToRead);
        if (RT_SUCCESS(rc2))
            offUnsigned += cbLeftToRead;
        else
            rc = rc2;
    }

    pThis->offCurPos = offUnsigned;
    return rc;
}


/**
 * @interface_method_impl{RTVFSIOSTREAMOPS,pfnWrite}
 */
static DECLCALLBACK(int) vdVfsFile_Write(void *pvThis, RTFOFF off, PCRTSGBUF pSgBuf, bool fBlocking, size_t *pcbWritten)
{
    PVDVFSFILE pThis = (PVDVFSFILE)pvThis;

    Assert(pSgBuf->cSegs == 1);
    NOREF(fBlocking);

    /*
     * Find the current position and check if it's within the volume.
     * Writing beyond the end of a volume is not supported.
     */
    uint64_t offUnsigned = off < 0 ? pThis->offCurPos : (uint64_t)off;
    uint64_t const cbImage = VDGetSize(pThis->pDisk, VD_LAST_IMAGE);
    if (offUnsigned >= cbImage)
    {
        if (pcbWritten)
        {
            *pcbWritten = 0;
            pThis->offCurPos = cbImage;
        }
        return VERR_EOF;
    }

    size_t cbLeftToWrite;
    if (offUnsigned + pSgBuf->paSegs[0].cbSeg < cbImage)
    {
        cbLeftToWrite = pSgBuf->paSegs[0].cbSeg;
        if (pcbWritten)
            *pcbWritten = cbLeftToWrite;
    }
    else
    {
        if (!pcbWritten)
            return VERR_EOF;
        *pcbWritten = cbLeftToWrite = (size_t)(cbImage - offUnsigned);
    }

    /*
     * Ok, we've got a valid stretch within the file.  Do the reading.
     */
    int rc = VINF_SUCCESS;
    if (cbLeftToWrite > 0)
    {
        rc = vdWriteHelper(pThis->pDisk, offUnsigned, pSgBuf->paSegs[0].pvSeg, cbLeftToWrite);
        if (RT_SUCCESS(rc))
            offUnsigned += cbLeftToWrite;
    }

    pThis->offCurPos = offUnsigned;
    return rc;
}


/**
 * @interface_method_impl{RTVFSIOSTREAMOPS,pfnFlush}
 */
static DECLCALLBACK(int) vdVfsFile_Flush(void *pvThis)
{
    PVDVFSFILE pThis = (PVDVFSFILE)pvThis;
    return VDFlush(pThis->pDisk);
}


/**
 * @interface_method_impl{RTVFSIOSTREAMOPS,pfnPollOne}
 */
static DECLCALLBACK(int) vdVfsFile_PollOne(void *pvThis, uint32_t fEvents, RTMSINTERVAL cMillies, bool fIntr,
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
static DECLCALLBACK(int) vdVfsFile_Tell(void *pvThis, PRTFOFF poffActual)
{
    PVDVFSFILE pThis = (PVDVFSFILE)pvThis;
    *poffActual = pThis->offCurPos;
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{RTVFSOBJSETOPS,pfnSetMode}
 */
static DECLCALLBACK(int) vdVfsFile_SetMode(void *pvThis, RTFMODE fMode, RTFMODE fMask)
{
    NOREF(pvThis);
    NOREF(fMode);
    NOREF(fMask);
    return VERR_NOT_SUPPORTED;
}


/**
 * @interface_method_impl{RTVFSOBJSETOPS,pfnSetTimes}
 */
static DECLCALLBACK(int) vdVfsFile_SetTimes(void *pvThis, PCRTTIMESPEC pAccessTime, PCRTTIMESPEC pModificationTime,
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
static DECLCALLBACK(int) vdVfsFile_SetOwner(void *pvThis, RTUID uid, RTGID gid)
{
    NOREF(pvThis);
    NOREF(uid);
    NOREF(gid);
    return VERR_NOT_SUPPORTED;
}


/**
 * @interface_method_impl{RTVFSFILEOPS,pfnSeek}
 */
static DECLCALLBACK(int) vdVfsFile_Seek(void *pvThis, RTFOFF offSeek, unsigned uMethod, PRTFOFF poffActual)
{
    PVDVFSFILE pThis = (PVDVFSFILE)pvThis;

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
            offWrt = VDGetSize(pThis->pDisk, VD_LAST_IMAGE);
            break;

        default:
            return VERR_INTERNAL_ERROR_5;
    }

    /*
     * Calc new position, take care to stay without bounds.
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
static DECLCALLBACK(int) vdVfsFile_QuerySize(void *pvThis, uint64_t *pcbFile)
{
    PVDVFSFILE pThis = (PVDVFSFILE)pvThis;
    *pcbFile = VDGetSize(pThis->pDisk, VD_LAST_IMAGE);
    return VINF_SUCCESS;
}


/**
 * Standard file operations.
 */
DECL_HIDDEN_CONST(const RTVFSFILEOPS) g_vdVfsStdFileOps =
{
    { /* Stream */
        { /* Obj */
            RTVFSOBJOPS_VERSION,
            RTVFSOBJTYPE_FILE,
            "VDFile",
            vdVfsFile_Close,
            vdVfsFile_QueryInfo,
            RTVFSOBJOPS_VERSION
        },
        RTVFSIOSTREAMOPS_VERSION,
        RTVFSIOSTREAMOPS_FEAT_NO_SG,
        vdVfsFile_Read,
        vdVfsFile_Write,
        vdVfsFile_Flush,
        vdVfsFile_PollOne,
        vdVfsFile_Tell,
        NULL /*Skip*/,
        NULL /*ZeroFill*/,
        RTVFSIOSTREAMOPS_VERSION,
    },
    RTVFSFILEOPS_VERSION,
    /*RTVFSIOFILEOPS_FEAT_NO_AT_OFFSET*/ 0,
    { /* ObjSet */
        RTVFSOBJSETOPS_VERSION,
        RT_OFFSETOF(RTVFSFILEOPS, Stream.Obj) - RT_OFFSETOF(RTVFSFILEOPS, ObjSet),
        vdVfsFile_SetMode,
        vdVfsFile_SetTimes,
        vdVfsFile_SetOwner,
        RTVFSOBJSETOPS_VERSION
    },
    vdVfsFile_Seek,
    vdVfsFile_QuerySize,
    RTVFSFILEOPS_VERSION
};


VBOXDDU_DECL(int) VDCreateVfsFileFromDisk(PVDISK pDisk, uint32_t fFlags,
                                          PRTVFSFILE phVfsFile)
{
    AssertPtrReturn(pDisk, VERR_INVALID_HANDLE);
    AssertPtrReturn(phVfsFile, VERR_INVALID_POINTER);
    AssertReturn((fFlags & ~VD_VFSFILE_FLAGS_MASK) == 0, VERR_INVALID_PARAMETER);

    /*
     * Create the volume file.
     */
    RTVFSFILE  hVfsFile;
    PVDVFSFILE pThis;
    int rc = RTVfsNewFile(&g_vdVfsStdFileOps, sizeof(*pThis), RTFILE_O_OPEN | RTFILE_O_READ | RTFILE_O_WRITE,
                          NIL_RTVFS, NIL_RTVFSLOCK, &hVfsFile, (void **)&pThis);
    if (RT_SUCCESS(rc))
    {
        pThis->offCurPos = 0;
        pThis->pDisk     = pDisk;
        pThis->fFlags    = fFlags;

        *phVfsFile = hVfsFile;
        return VINF_SUCCESS;
    }

    return rc;
}


/**
 * @interface_method_impl{RTVFSCHAINELEMENTREG,pfnValidate}
 */
static DECLCALLBACK(int) vdVfsChain_Validate(PCRTVFSCHAINELEMENTREG pProviderReg, PRTVFSCHAINSPEC pSpec,
                                             PRTVFSCHAINELEMSPEC pElement, uint32_t *poffError, PRTERRINFO pErrInfo)
{
    RT_NOREF(pProviderReg, pSpec);

    /*
     * Basic checks.
     */
    if (pElement->enmTypeIn != RTVFSOBJTYPE_INVALID)
        return VERR_VFS_CHAIN_MUST_BE_FIRST_ELEMENT;
    if (   pElement->enmType != RTVFSOBJTYPE_FILE
        && pElement->enmType != RTVFSOBJTYPE_IO_STREAM)
        return VERR_VFS_CHAIN_ONLY_FILE_OR_IOS;

    if (pElement->cArgs < 1)
        return VERR_VFS_CHAIN_AT_LEAST_ONE_ARG;
    if (pElement->cArgs > 2)
        return VERR_VFS_CHAIN_AT_MOST_TWO_ARGS;

    /*
     * Parse the flag if present, save in pElement->uProvider.
     */
    uint32_t fFlags = (pSpec->fOpenFile & RTFILE_O_ACCESS_MASK) == RTFILE_O_READ
                    ? VD_OPEN_FLAGS_READONLY : VD_OPEN_FLAGS_NORMAL;
    if (pElement->cArgs > 1)
    {
        const char *psz = pElement->paArgs[1].psz;
        if (*psz)
        {
            if (   !strcmp(psz, "ro")
                || !strcmp(psz, "r"))
            {
                fFlags &= ~(VD_OPEN_FLAGS_READONLY | VD_OPEN_FLAGS_NORMAL);
                fFlags |= VD_OPEN_FLAGS_READONLY;
            }
            else if (!strcmp(psz, "rw"))
            {
                fFlags &= ~(VD_OPEN_FLAGS_READONLY | VD_OPEN_FLAGS_NORMAL);
                fFlags |= VD_OPEN_FLAGS_NORMAL;
            }
            else
            {
                *poffError = pElement->paArgs[0].offSpec;
                return RTErrInfoSet(pErrInfo, VERR_VFS_CHAIN_INVALID_ARGUMENT, "Expected 'ro' or 'rw' as argument");
            }
        }
    }

    pElement->uProvider = fFlags;
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{RTVFSCHAINELEMENTREG,pfnInstantiate}
 */
static DECLCALLBACK(int) vdVfsChain_Instantiate(PCRTVFSCHAINELEMENTREG pProviderReg, PCRTVFSCHAINSPEC pSpec,
                                                PCRTVFSCHAINELEMSPEC pElement, RTVFSOBJ hPrevVfsObj,
                                                PRTVFSOBJ phVfsObj, uint32_t *poffError, PRTERRINFO pErrInfo)
{
    RT_NOREF(pProviderReg, pSpec, poffError, pErrInfo);
    AssertReturn(hPrevVfsObj == NIL_RTVFSOBJ, VERR_VFS_CHAIN_IPE);

    /* Determin the format. */
    char  *pszFormat = NULL;
    VDTYPE enmType   = VDTYPE_INVALID;
    int rc = VDGetFormat(NULL, NULL, pElement->paArgs[0].psz, &pszFormat, &enmType);
    if (RT_SUCCESS(rc))
    {
        PVDISK pDisk = NULL;
        rc = VDCreate(NULL, enmType, &pDisk);
        if (RT_SUCCESS(rc))
        {
            rc = VDOpen(pDisk, pszFormat, pElement->paArgs[0].psz, (uint32_t)pElement->uProvider, NULL);
            if (RT_SUCCESS(rc))
            {
                RTVFSFILE hVfsFile;
                rc = VDCreateVfsFileFromDisk(pDisk, VD_VFSFILE_DESTROY_ON_RELEASE, &hVfsFile);
                if (RT_SUCCESS(rc))
                {
                    RTStrFree(pszFormat);

                    *phVfsObj = RTVfsObjFromFile(hVfsFile);
                    RTVfsFileRelease(hVfsFile);

                    if (*phVfsObj != NIL_RTVFSOBJ)
                        return VINF_SUCCESS;
                    return VERR_VFS_CHAIN_CAST_FAILED;
                }
            }
            VDDestroy(pDisk);
        }
        RTStrFree(pszFormat);
    }
    return rc;
}


/**
 * @interface_method_impl{RTVFSCHAINELEMENTREG,pfnCanReuseElement}
 */
static DECLCALLBACK(bool) vdVfsChain_CanReuseElement(PCRTVFSCHAINELEMENTREG pProviderReg,
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
    /* pszName = */             "vd",
    /* ListEntry = */           { NULL, NULL },
    /* pszHelp = */             "Opens a container image using the VD API.\n",
    /* pfnValidate = */         vdVfsChain_Validate,
    /* pfnInstantiate = */      vdVfsChain_Instantiate,
    /* pfnCanReuseElement = */  vdVfsChain_CanReuseElement,
    /* uEndMarker = */          RTVFSCHAINELEMENTREG_VERSION
};

RTVFSCHAIN_AUTO_REGISTER_ELEMENT_PROVIDER(&g_rtVfsChainIsoFsVolReg, rtVfsChainIsoFsVolReg);

