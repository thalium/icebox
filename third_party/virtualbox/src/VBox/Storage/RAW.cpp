/* $Id: RAW.cpp $ */
/** @file
 * RawHDDCore - Raw Disk image, Core Code.
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
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_VD_RAW
#include <VBox/vd-plugin.h>
#include <VBox/err.h>

#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/alloc.h>
#include <iprt/path.h>

#include "VDBackends.h"
#include "VDBackendsInline.h"


/*********************************************************************************************************************************
*   Constants And Macros, Structures and Typedefs                                                                                *
*********************************************************************************************************************************/

/**
 * Raw image data structure.
 */
typedef struct RAWIMAGE
{
    /** Image name. */
    const char          *pszFilename;
    /** Storage handle. */
    PVDIOSTORAGE        pStorage;

    /** Pointer to the per-disk VD interface list. */
    PVDINTERFACE        pVDIfsDisk;
    /** Pointer to the per-image VD interface list. */
    PVDINTERFACE        pVDIfsImage;
    /** Error interface. */
    PVDINTERFACEERROR   pIfError;
    /** I/O interface. */
    PVDINTERFACEIOINT   pIfIo;

    /** Open flags passed by VBoxHD layer. */
    unsigned            uOpenFlags;
    /** Image flags defined during creation or determined during open. */
    unsigned            uImageFlags;
    /** Total size of the image. */
    uint64_t            cbSize;
    /** Position in the image (only truly used for sequential access). */
    uint64_t            offAccess;
    /** Flag if this is a newly created image. */
    bool                fCreate;
    /** Physical geometry of this image. */
    VDGEOMETRY          PCHSGeometry;
    /** Logical geometry of this image. */
    VDGEOMETRY          LCHSGeometry;
    /** Sector size of the image. */
    uint32_t            cbSector;
    /** The static region list. */
    VDREGIONLIST        RegionList;
} RAWIMAGE, *PRAWIMAGE;


/** Size of write operations when filling an image with zeroes. */
#define RAW_FILL_SIZE (128 * _1K)

/** The maximum reasonable size of a floppy image (big format 2.88MB medium). */
#define RAW_MAX_FLOPPY_IMG_SIZE (512 * 82 * 48 * 2)


/*********************************************************************************************************************************
*   Static Variables                                                                                                             *
*********************************************************************************************************************************/

/** NULL-terminated array of supported file extensions. */
static const VDFILEEXTENSION s_aRawFileExtensions[] =
{
    {"iso", VDTYPE_OPTICAL_DISC},
    {"cdr", VDTYPE_OPTICAL_DISC},
    {"img", VDTYPE_FLOPPY},
    {"ima", VDTYPE_FLOPPY},
    {"dsk", VDTYPE_FLOPPY},
    {"flp", VDTYPE_FLOPPY},
    {"vfd", VDTYPE_FLOPPY},
    {NULL, VDTYPE_INVALID}
};


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/

/**
 * Internal. Flush image data to disk.
 */
static int rawFlushImage(PRAWIMAGE pImage)
{
    int rc = VINF_SUCCESS;

    if (   pImage->pStorage
        && !(pImage->uOpenFlags & VD_OPEN_FLAGS_READONLY))
        rc = vdIfIoIntFileFlushSync(pImage->pIfIo, pImage->pStorage);

    return rc;
}

/**
 * Internal. Free all allocated space for representing an image except pImage,
 * and optionally delete the image from disk.
 */
static int rawFreeImage(PRAWIMAGE pImage, bool fDelete)
{
    int rc = VINF_SUCCESS;

    /* Freeing a never allocated image (e.g. because the open failed) is
     * not signalled as an error. After all nothing bad happens. */
    if (pImage)
    {
        if (pImage->pStorage)
        {
            /* No point updating the file that is deleted anyway. */
            if (!fDelete)
            {
                /* For newly created images in sequential mode fill it to
                 * the nominal size. */
                if (   pImage->uOpenFlags & VD_OPEN_FLAGS_SEQUENTIAL
                    && !(pImage->uOpenFlags & VD_OPEN_FLAGS_READONLY)
                    && pImage->fCreate)
                {
                    /* Fill rest of image with zeroes, a must for sequential
                     * images to reach the nominal size. */
                    uint64_t uOff;
                    void *pvBuf = RTMemTmpAllocZ(RAW_FILL_SIZE);
                    if (RT_LIKELY(pvBuf))
                    {
                        uOff = pImage->offAccess;
                        /* Write data to all image blocks. */
                        while (uOff < pImage->cbSize)
                        {
                            unsigned cbChunk = (unsigned)RT_MIN(pImage->cbSize - uOff,
                                                                RAW_FILL_SIZE);

                            rc = vdIfIoIntFileWriteSync(pImage->pIfIo, pImage->pStorage,
                                                        uOff, pvBuf, cbChunk);
                            if (RT_FAILURE(rc))
                                break;

                            uOff += cbChunk;
                        }

                        RTMemTmpFree(pvBuf);
                    }
                    else
                        rc = VERR_NO_MEMORY;
                }
                rawFlushImage(pImage);
            }

            rc = vdIfIoIntFileClose(pImage->pIfIo, pImage->pStorage);
            pImage->pStorage = NULL;
        }

        if (fDelete && pImage->pszFilename)
            vdIfIoIntFileDelete(pImage->pIfIo, pImage->pszFilename);
    }

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/**
 * Internal: Open an image, constructing all necessary data structures.
 */
static int rawOpenImage(PRAWIMAGE pImage, unsigned uOpenFlags)
{
    pImage->uOpenFlags = uOpenFlags;
    pImage->fCreate = false;

    pImage->pIfError = VDIfErrorGet(pImage->pVDIfsDisk);
    pImage->pIfIo = VDIfIoIntGet(pImage->pVDIfsImage);
    AssertPtrReturn(pImage->pIfIo, VERR_INVALID_PARAMETER);

    /* Open the image. */
    int rc = vdIfIoIntFileOpen(pImage->pIfIo, pImage->pszFilename,
                               VDOpenFlagsToFileOpenFlags(uOpenFlags,
                                                          false /* fCreate */),
                               &pImage->pStorage);
    if (RT_SUCCESS(rc))
    {
        rc = vdIfIoIntFileGetSize(pImage->pIfIo, pImage->pStorage, &pImage->cbSize);
        if (   RT_SUCCESS(rc)
            && !(pImage->cbSize % 512))
            pImage->uImageFlags |= VD_IMAGE_FLAGS_FIXED;
        else if (RT_SUCCESS(rc))
            rc = VERR_VD_RAW_SIZE_MODULO_512;
    }
    /* else: Do NOT signal an appropriate error here, as the VD layer has the
     *       choice of retrying the open if it failed. */

    if (RT_SUCCESS(rc))
    {
        PVDREGIONDESC pRegion = &pImage->RegionList.aRegions[0];
        pImage->RegionList.fFlags   = 0;
        pImage->RegionList.cRegions = 1;

        pRegion->offRegion            = 0; /* Disk start. */
        pRegion->cbBlock              = pImage->cbSector;
        pRegion->enmDataForm          = VDREGIONDATAFORM_RAW;
        pRegion->enmMetadataForm      = VDREGIONMETADATAFORM_NONE;
        pRegion->cbData               = pImage->cbSector;
        pRegion->cbMetadata           = 0;
        pRegion->cRegionBlocksOrBytes = pImage->cbSize;
    }
    else
        rawFreeImage(pImage, false);
    return rc;
}

/**
 * Internal: Create a raw image.
 */
static int rawCreateImage(PRAWIMAGE pImage, uint64_t cbSize,
                          unsigned uImageFlags, const char *pszComment,
                          PCVDGEOMETRY pPCHSGeometry,
                          PCVDGEOMETRY pLCHSGeometry, unsigned uOpenFlags,
                          PVDINTERFACEPROGRESS pIfProgress,
                          unsigned uPercentStart, unsigned uPercentSpan)
{
    RT_NOREF1(pszComment);
    int rc = VINF_SUCCESS;

    pImage->fCreate      = true;
    pImage->uOpenFlags   = uOpenFlags & ~VD_OPEN_FLAGS_READONLY;
    pImage->uImageFlags  = uImageFlags | VD_IMAGE_FLAGS_FIXED;
    pImage->PCHSGeometry = *pPCHSGeometry;
    pImage->LCHSGeometry = *pLCHSGeometry;
    pImage->pIfError     = VDIfErrorGet(pImage->pVDIfsDisk);
    pImage->pIfIo        = VDIfIoIntGet(pImage->pVDIfsImage);
    AssertPtrReturn(pImage->pIfIo, VERR_INVALID_PARAMETER);

    if (!(pImage->uImageFlags & VD_IMAGE_FLAGS_DIFF))
    {
        /* Create image file. */
        uint32_t fOpen = VDOpenFlagsToFileOpenFlags(pImage->uOpenFlags, true /* fCreate */);
        if (uOpenFlags & VD_OPEN_FLAGS_SEQUENTIAL)
            fOpen &= ~RTFILE_O_READ;
        rc = vdIfIoIntFileOpen(pImage->pIfIo, pImage->pszFilename, fOpen, &pImage->pStorage);
        if (RT_SUCCESS(rc))
        {
            if (!(uOpenFlags & VD_OPEN_FLAGS_SEQUENTIAL))
            {
                RTFOFF cbFree = 0;

                /* Check the free space on the disk and leave early if there is not
                 * sufficient space available. */
                rc = vdIfIoIntFileGetFreeSpace(pImage->pIfIo, pImage->pszFilename, &cbFree);
                if (RT_FAILURE(rc) /* ignore errors */ || ((uint64_t)cbFree >= cbSize))
                {
                    rc = vdIfIoIntFileSetAllocationSize(pImage->pIfIo, pImage->pStorage, cbSize, 0 /* fFlags */,
                                                        pIfProgress, uPercentStart, uPercentSpan);
                    if (RT_SUCCESS(rc))
                    {
                        vdIfProgress(pIfProgress, uPercentStart + uPercentSpan * 98 / 100);

                        pImage->cbSize = cbSize;
                        rc = rawFlushImage(pImage);
                    }
                }
                else
                    rc = vdIfError(pImage->pIfError, VERR_DISK_FULL, RT_SRC_POS, N_("Raw: disk would overflow creating image '%s'"), pImage->pszFilename);
            }
            else
            {
                rc = vdIfIoIntFileSetSize(pImage->pIfIo, pImage->pStorage, cbSize);
                if (RT_SUCCESS(rc))
                    pImage->cbSize = cbSize;
            }
        }
        else
            rc = vdIfError(pImage->pIfError, rc, RT_SRC_POS, N_("Raw: cannot create image '%s'"), pImage->pszFilename);
    }
    else
        rc = vdIfError(pImage->pIfError, VERR_VD_RAW_INVALID_TYPE, RT_SRC_POS, N_("Raw: cannot create diff image '%s'"), pImage->pszFilename);

    if (RT_SUCCESS(rc))
    {
        PVDREGIONDESC pRegion = &pImage->RegionList.aRegions[0];
        pImage->RegionList.fFlags   = 0;
        pImage->RegionList.cRegions = 1;

        pRegion->offRegion            = 0; /* Disk start. */
        pRegion->cbBlock              = pImage->cbSector;
        pRegion->enmDataForm          = VDREGIONDATAFORM_RAW;
        pRegion->enmMetadataForm      = VDREGIONMETADATAFORM_NONE;
        pRegion->cbData               = pImage->cbSector;
        pRegion->cbMetadata           = 0;
        pRegion->cRegionBlocksOrBytes = pImage->cbSize;

        vdIfProgress(pIfProgress, uPercentStart + uPercentSpan);
    }

    if (RT_FAILURE(rc))
        rawFreeImage(pImage, rc != VERR_ALREADY_EXISTS);
    return rc;
}


/** @copydoc VDIMAGEBACKEND::pfnProbe */
static DECLCALLBACK(int) rawProbe(const char *pszFilename, PVDINTERFACE pVDIfsDisk,
                                  PVDINTERFACE pVDIfsImage, VDTYPE *penmType)
{
    RT_NOREF1(pVDIfsDisk);
    LogFlowFunc(("pszFilename=\"%s\" pVDIfsDisk=%#p pVDIfsImage=%#p\n", pszFilename, pVDIfsDisk, pVDIfsImage));
    PVDIOSTORAGE pStorage = NULL;
    PVDINTERFACEIOINT pIfIo = VDIfIoIntGet(pVDIfsImage);

    AssertPtrReturn(pIfIo, VERR_INVALID_PARAMETER);
    AssertReturn((VALID_PTR(pszFilename) && *pszFilename), VERR_INVALID_PARAMETER);

    /*
     * Open the file and read the footer.
     */
    int rc = vdIfIoIntFileOpen(pIfIo, pszFilename,
                               VDOpenFlagsToFileOpenFlags(VD_OPEN_FLAGS_READONLY,
                                                          false /* fCreate */),
                               &pStorage);
    if (RT_SUCCESS(rc))
    {
        uint64_t cbFile;
        const char *pszSuffix = RTPathSuffix(pszFilename);
        rc = vdIfIoIntFileGetSize(pIfIo, pStorage, &cbFile);

        /* Try to guess the image type based on the extension. */
        if (   RT_SUCCESS(rc)
            && pszSuffix)
        {
            if (   !RTStrICmp(pszSuffix, ".iso")
                || !RTStrICmp(pszSuffix, ".cdr")) /* DVD images. */
            {
                /* Note that there are ISO images smaller than 1 MB; it is impossible to distinguish
                 * between raw floppy and CD images based on their size (and cannot be reliably done
                 * based on contents, either).
                 * bird: Not sure what this comment is mumbling about, the test below is 32KB not 1MB.
                 */
                if (cbFile % 2048)
                    rc = VERR_VD_RAW_SIZE_MODULO_2048;
                else if (cbFile <= 32768)
                    rc = VERR_VD_RAW_SIZE_OPTICAL_TOO_SMALL;
                else
                {
                    *penmType = VDTYPE_OPTICAL_DISC;
                    rc = VINF_SUCCESS;
                }
            }
            else if (   !RTStrICmp(pszSuffix, ".img")
                     || !RTStrICmp(pszSuffix, ".ima")
                     || !RTStrICmp(pszSuffix, ".dsk")
                     || !RTStrICmp(pszSuffix, ".flp")
                     || !RTStrICmp(pszSuffix, ".vfd")) /* Floppy images */
            {
                if (cbFile % 512)
                    rc = VERR_VD_RAW_SIZE_MODULO_512;
                else if (cbFile > RAW_MAX_FLOPPY_IMG_SIZE)
                    rc = VERR_VD_RAW_SIZE_FLOPPY_TOO_BIG;
                else
                {
                    *penmType = VDTYPE_FLOPPY;
                    rc = VINF_SUCCESS;
                }
            }
            else
                rc = VERR_VD_RAW_INVALID_HEADER;
        }
        else
            rc = VERR_VD_RAW_INVALID_HEADER;
    }

    if (pStorage)
        vdIfIoIntFileClose(pIfIo, pStorage);

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VDIMAGEBACKEND::pfnOpen */
static DECLCALLBACK(int) rawOpen(const char *pszFilename, unsigned uOpenFlags,
                                 PVDINTERFACE pVDIfsDisk, PVDINTERFACE pVDIfsImage,
                                 VDTYPE enmType, void **ppBackendData)
{
    LogFlowFunc(("pszFilename=\"%s\" uOpenFlags=%#x pVDIfsDisk=%#p pVDIfsImage=%#p enmType=%u ppBackendData=%#p\n",
                 pszFilename, uOpenFlags, pVDIfsDisk, pVDIfsImage, enmType, ppBackendData));
    int rc;
    PRAWIMAGE pImage;

    /* Check open flags. All valid flags are supported. */
    AssertReturn(!(uOpenFlags & ~VD_OPEN_FLAGS_MASK), VERR_INVALID_PARAMETER);
    AssertReturn((VALID_PTR(pszFilename) && *pszFilename), VERR_INVALID_PARAMETER);

    pImage = (PRAWIMAGE)RTMemAllocZ(RT_UOFFSETOF(RAWIMAGE, RegionList.aRegions[1]));
    if (RT_LIKELY(pImage))
    {
        pImage->pszFilename = pszFilename;
        pImage->pStorage = NULL;
        pImage->pVDIfsDisk = pVDIfsDisk;
        pImage->pVDIfsImage = pVDIfsImage;

        if (enmType == VDTYPE_OPTICAL_DISC)
            pImage->cbSector = 2048;
        else
            pImage->cbSector = 512;

        rc = rawOpenImage(pImage, uOpenFlags);
        if (RT_SUCCESS(rc))
            *ppBackendData = pImage;
        else
            RTMemFree(pImage);
    }
    else
        rc = VERR_NO_MEMORY;

    LogFlowFunc(("returns %Rrc (pBackendData=%#p)\n", rc, *ppBackendData));
    return rc;
}

/** @copydoc VDIMAGEBACKEND::pfnCreate */
static DECLCALLBACK(int) rawCreate(const char *pszFilename, uint64_t cbSize,
                                   unsigned uImageFlags, const char *pszComment,
                                   PCVDGEOMETRY pPCHSGeometry, PCVDGEOMETRY pLCHSGeometry,
                                   PCRTUUID pUuid, unsigned uOpenFlags,
                                   unsigned uPercentStart, unsigned uPercentSpan,
                                   PVDINTERFACE pVDIfsDisk, PVDINTERFACE pVDIfsImage,
                                   PVDINTERFACE pVDIfsOperation, VDTYPE enmType,
                                   void **ppBackendData)
{
    RT_NOREF1(pUuid);
    LogFlowFunc(("pszFilename=\"%s\" cbSize=%llu uImageFlags=%#x pszComment=\"%s\" pPCHSGeometry=%#p pLCHSGeometry=%#p Uuid=%RTuuid uOpenFlags=%#x uPercentStart=%u uPercentSpan=%u pVDIfsDisk=%#p pVDIfsImage=%#p pVDIfsOperation=%#p enmType=%u ppBackendData=%#p",
                 pszFilename, cbSize, uImageFlags, pszComment, pPCHSGeometry, pLCHSGeometry, pUuid, uOpenFlags, uPercentStart, uPercentSpan, pVDIfsDisk, pVDIfsImage, pVDIfsOperation, enmType, ppBackendData));

    /* Check the VD container type. Yes, hard disk must be allowed, otherwise
     * various tools using this backend for hard disk images will fail. */
    if (enmType != VDTYPE_HDD && enmType != VDTYPE_OPTICAL_DISC && enmType != VDTYPE_FLOPPY)
        return VERR_VD_INVALID_TYPE;

    int rc = VINF_SUCCESS;
    PVDINTERFACEPROGRESS pIfProgress = VDIfProgressGet(pVDIfsOperation);

    /* Check arguments. */
    AssertReturn(!(uOpenFlags & ~VD_OPEN_FLAGS_MASK), VERR_INVALID_PARAMETER);
    AssertReturn(   VALID_PTR(pszFilename)
                 && *pszFilename
                 && VALID_PTR(pPCHSGeometry)
                 && VALID_PTR(pLCHSGeometry), VERR_INVALID_PARAMETER);

    PRAWIMAGE pImage = (PRAWIMAGE)RTMemAllocZ(RT_UOFFSETOF(RAWIMAGE, RegionList.aRegions[1]));
    if (RT_LIKELY(pImage))
    {
        pImage->pszFilename = pszFilename;
        pImage->pStorage = NULL;
        pImage->pVDIfsDisk = pVDIfsDisk;
        pImage->pVDIfsImage = pVDIfsImage;

        rc = rawCreateImage(pImage, cbSize, uImageFlags, pszComment,
                            pPCHSGeometry, pLCHSGeometry, uOpenFlags,
                            pIfProgress, uPercentStart, uPercentSpan);
        if (RT_SUCCESS(rc))
        {
            /* So far the image is opened in read/write mode. Make sure the
             * image is opened in read-only mode if the caller requested that. */
            if (uOpenFlags & VD_OPEN_FLAGS_READONLY)
            {
                rawFreeImage(pImage, false);
                rc = rawOpenImage(pImage, uOpenFlags);
            }

            if (RT_SUCCESS(rc))
                *ppBackendData = pImage;
        }

        if (RT_FAILURE(rc))
            RTMemFree(pImage);
    }
    else
        rc = VERR_NO_MEMORY;

    LogFlowFunc(("returns %Rrc (pBackendData=%#p)\n", rc, *ppBackendData));
    return rc;
}

/** @copydoc VDIMAGEBACKEND::pfnRename */
static DECLCALLBACK(int) rawRename(void *pBackendData, const char *pszFilename)
{
    LogFlowFunc(("pBackendData=%#p pszFilename=%#p\n", pBackendData, pszFilename));
    PRAWIMAGE pImage = (PRAWIMAGE)pBackendData;

    AssertReturn((pImage && pszFilename && *pszFilename), VERR_INVALID_PARAMETER);

    /* Close the image. */
    int rc = rawFreeImage(pImage, false);
    if (RT_SUCCESS(rc))
    {
        /* Rename the file. */
        rc = vdIfIoIntFileMove(pImage->pIfIo, pImage->pszFilename, pszFilename, 0);
        if (RT_SUCCESS(rc))
        {
            /* Update pImage with the new information. */
            pImage->pszFilename = pszFilename;

            /* Open the old image with new name. */
            rc = rawOpenImage(pImage, pImage->uOpenFlags);
        }
        else
        {
            /* The move failed, try to reopen the original image. */
            int rc2 = rawOpenImage(pImage, pImage->uOpenFlags);
            if (RT_FAILURE(rc2))
                rc = rc2;
        }
    }

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VDIMAGEBACKEND::pfnClose */
static DECLCALLBACK(int) rawClose(void *pBackendData, bool fDelete)
{
    LogFlowFunc(("pBackendData=%#p fDelete=%d\n", pBackendData, fDelete));
    PRAWIMAGE pImage = (PRAWIMAGE)pBackendData;
    int rc = rawFreeImage(pImage, fDelete);
    RTMemFree(pImage);

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VDIMAGEBACKEND::pfnRead */
static DECLCALLBACK(int) rawRead(void *pBackendData, uint64_t uOffset, size_t cbToRead,
                                 PVDIOCTX pIoCtx, size_t *pcbActuallyRead)
{
    int rc = VINF_SUCCESS;
    PRAWIMAGE pImage = (PRAWIMAGE)pBackendData;

    /* For sequential access do not allow to go back. */
    if (   pImage->uOpenFlags & VD_OPEN_FLAGS_SEQUENTIAL
        && uOffset < pImage->offAccess)
    {
        *pcbActuallyRead = 0;
        return VERR_INVALID_PARAMETER;
    }

    rc = vdIfIoIntFileReadUser(pImage->pIfIo, pImage->pStorage, uOffset,
                               pIoCtx, cbToRead);
    if (RT_SUCCESS(rc))
    {
        *pcbActuallyRead = cbToRead;
        pImage->offAccess = uOffset + cbToRead;
    }

    return rc;
}

/** @copydoc VDIMAGEBACKEND::pfnWrite */
static DECLCALLBACK(int) rawWrite(void *pBackendData, uint64_t uOffset, size_t cbToWrite,
                                  PVDIOCTX pIoCtx, size_t *pcbWriteProcess, size_t *pcbPreRead,
                                  size_t *pcbPostRead, unsigned fWrite)
{
    RT_NOREF1(fWrite);
    int rc = VINF_SUCCESS;
    PRAWIMAGE pImage = (PRAWIMAGE)pBackendData;

    /* For sequential access do not allow to go back. */
    if (   pImage->uOpenFlags & VD_OPEN_FLAGS_SEQUENTIAL
        && uOffset < pImage->offAccess)
    {
        *pcbWriteProcess = 0;
        *pcbPostRead = 0;
        *pcbPreRead  = 0;
        return VERR_INVALID_PARAMETER;
    }

    rc = vdIfIoIntFileWriteUser(pImage->pIfIo, pImage->pStorage, uOffset,
                                pIoCtx, cbToWrite, NULL, NULL);
    if (RT_SUCCESS(rc))
    {
        *pcbWriteProcess = cbToWrite;
        *pcbPostRead = 0;
        *pcbPreRead  = 0;
        pImage->offAccess = uOffset + cbToWrite;
    }

    return rc;
}

/** @copydoc VDIMAGEBACKEND::pfnFlush */
static DECLCALLBACK(int) rawFlush(void *pBackendData, PVDIOCTX pIoCtx)
{
    int rc = VINF_SUCCESS;
    PRAWIMAGE pImage = (PRAWIMAGE)pBackendData;

    if (!(pImage->uOpenFlags & VD_OPEN_FLAGS_READONLY))
        rc = vdIfIoIntFileFlush(pImage->pIfIo, pImage->pStorage, pIoCtx,
                                NULL, NULL);

    return rc;
}

/** @copydoc VDIMAGEBACKEND::pfnGetVersion */
static DECLCALLBACK(unsigned) rawGetVersion(void *pBackendData)
{
    LogFlowFunc(("pBackendData=%#p\n", pBackendData));
    PRAWIMAGE pImage = (PRAWIMAGE)pBackendData;

    AssertPtrReturn(pImage, 0);

    return 1;
}

/** @copydoc VDIMAGEBACKEND::pfnGetFileSize */
static DECLCALLBACK(uint64_t) rawGetFileSize(void *pBackendData)
{
    LogFlowFunc(("pBackendData=%#p\n", pBackendData));
    PRAWIMAGE pImage = (PRAWIMAGE)pBackendData;

    AssertPtrReturn(pImage, 0);

    uint64_t cbFile = 0;
    if (pImage->pStorage)
    {
        int rc = vdIfIoIntFileGetSize(pImage->pIfIo, pImage->pStorage, &cbFile);
        if (RT_FAILURE(rc))
            cbFile = 0; /* Make sure it is 0 */
    }

    LogFlowFunc(("returns %lld\n", cbFile));
    return cbFile;
}

/** @copydoc VDIMAGEBACKEND::pfnGetPCHSGeometry */
static DECLCALLBACK(int) rawGetPCHSGeometry(void *pBackendData,
                                            PVDGEOMETRY pPCHSGeometry)
{
    LogFlowFunc(("pBackendData=%#p pPCHSGeometry=%#p\n", pBackendData, pPCHSGeometry));
    PRAWIMAGE pImage = (PRAWIMAGE)pBackendData;
    int rc = VINF_SUCCESS;

    AssertPtrReturn(pImage, VERR_VD_NOT_OPENED);

    if (pImage->PCHSGeometry.cCylinders)
        *pPCHSGeometry = pImage->PCHSGeometry;
    else
        rc = VERR_VD_GEOMETRY_NOT_SET;

    LogFlowFunc(("returns %Rrc (PCHS=%u/%u/%u)\n", rc, pPCHSGeometry->cCylinders, pPCHSGeometry->cHeads, pPCHSGeometry->cSectors));
    return rc;
}

/** @copydoc VDIMAGEBACKEND::pfnSetPCHSGeometry */
static DECLCALLBACK(int) rawSetPCHSGeometry(void *pBackendData,
                                            PCVDGEOMETRY pPCHSGeometry)
{
    LogFlowFunc(("pBackendData=%#p pPCHSGeometry=%#p PCHS=%u/%u/%u\n",
                 pBackendData, pPCHSGeometry, pPCHSGeometry->cCylinders, pPCHSGeometry->cHeads, pPCHSGeometry->cSectors));
    PRAWIMAGE pImage = (PRAWIMAGE)pBackendData;
    int rc = VINF_SUCCESS;

    AssertPtrReturn(pImage, VERR_VD_NOT_OPENED);

    if (pImage->uOpenFlags & VD_OPEN_FLAGS_READONLY)
        rc = VERR_VD_IMAGE_READ_ONLY;
    else
        pImage->PCHSGeometry = *pPCHSGeometry;

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VDIMAGEBACKEND::pfnGetLCHSGeometry */
static DECLCALLBACK(int) rawGetLCHSGeometry(void *pBackendData,
                                            PVDGEOMETRY pLCHSGeometry)
{
     LogFlowFunc(("pBackendData=%#p pLCHSGeometry=%#p\n", pBackendData, pLCHSGeometry));
    PRAWIMAGE pImage = (PRAWIMAGE)pBackendData;
    int rc = VINF_SUCCESS;

    AssertPtrReturn(pImage, VERR_VD_NOT_OPENED);

    if (pImage->LCHSGeometry.cCylinders)
        *pLCHSGeometry = pImage->LCHSGeometry;
    else
        rc = VERR_VD_GEOMETRY_NOT_SET;

    LogFlowFunc(("returns %Rrc (LCHS=%u/%u/%u)\n", rc, pLCHSGeometry->cCylinders, pLCHSGeometry->cHeads, pLCHSGeometry->cSectors));
    return rc;
}

/** @copydoc VDIMAGEBACKEND::pfnSetLCHSGeometry */
static DECLCALLBACK(int) rawSetLCHSGeometry(void *pBackendData,
                                            PCVDGEOMETRY pLCHSGeometry)
{
    LogFlowFunc(("pBackendData=%#p pLCHSGeometry=%#p LCHS=%u/%u/%u\n",
                 pBackendData, pLCHSGeometry, pLCHSGeometry->cCylinders, pLCHSGeometry->cHeads, pLCHSGeometry->cSectors));
    PRAWIMAGE pImage = (PRAWIMAGE)pBackendData;
    int rc = VINF_SUCCESS;

    AssertPtrReturn(pImage, VERR_VD_NOT_OPENED);

    if (pImage->uOpenFlags & VD_OPEN_FLAGS_READONLY)
        rc = VERR_VD_IMAGE_READ_ONLY;
    else
        pImage->LCHSGeometry = *pLCHSGeometry;

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VDIMAGEBACKEND::pfnQueryRegions */
static DECLCALLBACK(int) rawQueryRegions(void *pBackendData, PCVDREGIONLIST *ppRegionList)
{
    LogFlowFunc(("pBackendData=%#p ppRegionList=%#p\n", pBackendData, ppRegionList));
    PRAWIMAGE pThis = (PRAWIMAGE)pBackendData;

    AssertPtrReturn(pThis, VERR_VD_NOT_OPENED);

    *ppRegionList = &pThis->RegionList;
    LogFlowFunc(("returns %Rrc\n", VINF_SUCCESS));
    return VINF_SUCCESS;
}

/** @copydoc VDIMAGEBACKEND::pfnRegionListRelease */
static DECLCALLBACK(void) rawRegionListRelease(void *pBackendData, PCVDREGIONLIST pRegionList)
{
    RT_NOREF1(pRegionList);
    LogFlowFunc(("pBackendData=%#p pRegionList=%#p\n", pBackendData, pRegionList));
    PRAWIMAGE pThis = (PRAWIMAGE)pBackendData;
    AssertPtr(pThis); RT_NOREF(pThis);

    /* Nothing to do here. */
}

/** @copydoc VDIMAGEBACKEND::pfnGetImageFlags */
static DECLCALLBACK(unsigned) rawGetImageFlags(void *pBackendData)
{
    LogFlowFunc(("pBackendData=%#p\n", pBackendData));
    PRAWIMAGE pImage = (PRAWIMAGE)pBackendData;

    AssertPtrReturn(pImage, 0);

    LogFlowFunc(("returns %#x\n", pImage->uImageFlags));
    return pImage->uImageFlags;
}

/** @copydoc VDIMAGEBACKEND::pfnGetOpenFlags */
static DECLCALLBACK(unsigned) rawGetOpenFlags(void *pBackendData)
{
    LogFlowFunc(("pBackendData=%#p\n", pBackendData));
    PRAWIMAGE pImage = (PRAWIMAGE)pBackendData;

    AssertPtrReturn(pImage, 0);

    LogFlowFunc(("returns %#x\n", pImage->uOpenFlags));
    return pImage->uOpenFlags;
}

/** @copydoc VDIMAGEBACKEND::pfnSetOpenFlags */
static DECLCALLBACK(int) rawSetOpenFlags(void *pBackendData, unsigned uOpenFlags)
{
    LogFlowFunc(("pBackendData=%#p\n uOpenFlags=%#x", pBackendData, uOpenFlags));
    PRAWIMAGE pImage = (PRAWIMAGE)pBackendData;
    int rc = VINF_SUCCESS;

    /* Image must be opened and the new flags must be valid. */
    if (!pImage || (uOpenFlags & ~(  VD_OPEN_FLAGS_READONLY | VD_OPEN_FLAGS_INFO
                                   | VD_OPEN_FLAGS_ASYNC_IO | VD_OPEN_FLAGS_SHAREABLE
                                   | VD_OPEN_FLAGS_SEQUENTIAL | VD_OPEN_FLAGS_SKIP_CONSISTENCY_CHECKS)))
        rc = VERR_INVALID_PARAMETER;
    else
    {
        /* Implement this operation via reopening the image. */
        rc = rawFreeImage(pImage, false);
        if (RT_SUCCESS(rc))
            rc = rawOpenImage(pImage, uOpenFlags);
    }

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VDIMAGEBACKEND::pfnGetComment */
VD_BACKEND_CALLBACK_GET_COMMENT_DEF_NOT_SUPPORTED(rawGetComment);

/** @copydoc VDIMAGEBACKEND::pfnSetComment */
VD_BACKEND_CALLBACK_SET_COMMENT_DEF_NOT_SUPPORTED(rawSetComment, PRAWIMAGE);

/** @copydoc VDIMAGEBACKEND::pfnGetUuid */
VD_BACKEND_CALLBACK_GET_UUID_DEF_NOT_SUPPORTED(rawGetUuid);

/** @copydoc VDIMAGEBACKEND::pfnSetUuid */
VD_BACKEND_CALLBACK_SET_UUID_DEF_NOT_SUPPORTED(rawSetUuid, PRAWIMAGE);

/** @copydoc VDIMAGEBACKEND::pfnGetModificationUuid */
VD_BACKEND_CALLBACK_GET_UUID_DEF_NOT_SUPPORTED(rawGetModificationUuid);

/** @copydoc VDIMAGEBACKEND::pfnSetModificationUuid */
VD_BACKEND_CALLBACK_SET_UUID_DEF_NOT_SUPPORTED(rawSetModificationUuid, PRAWIMAGE);

/** @copydoc VDIMAGEBACKEND::pfnGetParentUuid */
VD_BACKEND_CALLBACK_GET_UUID_DEF_NOT_SUPPORTED(rawGetParentUuid);

/** @copydoc VDIMAGEBACKEND::pfnSetParentUuid */
VD_BACKEND_CALLBACK_SET_UUID_DEF_NOT_SUPPORTED(rawSetParentUuid, PRAWIMAGE);

/** @copydoc VDIMAGEBACKEND::pfnGetParentModificationUuid */
VD_BACKEND_CALLBACK_GET_UUID_DEF_NOT_SUPPORTED(rawGetParentModificationUuid);

/** @copydoc VDIMAGEBACKEND::pfnSetParentModificationUuid */
VD_BACKEND_CALLBACK_SET_UUID_DEF_NOT_SUPPORTED(rawSetParentModificationUuid, PRAWIMAGE);

/** @copydoc VDIMAGEBACKEND::pfnDump */
static DECLCALLBACK(void) rawDump(void *pBackendData)
{
    PRAWIMAGE pImage = (PRAWIMAGE)pBackendData;

    AssertPtrReturnVoid(pImage);
    vdIfErrorMessage(pImage->pIfError, "Header: Geometry PCHS=%u/%u/%u LCHS=%u/%u/%u cbSector=%llu\n",
                     pImage->PCHSGeometry.cCylinders, pImage->PCHSGeometry.cHeads, pImage->PCHSGeometry.cSectors,
                     pImage->LCHSGeometry.cCylinders, pImage->LCHSGeometry.cHeads, pImage->LCHSGeometry.cSectors,
                     pImage->cbSize / 512);
}



const VDIMAGEBACKEND g_RawBackend =
{
    /* u32Version */
    VD_IMGBACKEND_VERSION,
    /* pszBackendName */
    "RAW",
    /* uBackendCaps */
    VD_CAP_CREATE_FIXED | VD_CAP_FILE | VD_CAP_ASYNC | VD_CAP_VFS,
    /* paFileExtensions */
    s_aRawFileExtensions,
    /* paConfigInfo */
    NULL,
    /* pfnProbe */
    rawProbe,
    /* pfnOpen */
    rawOpen,
    /* pfnCreate */
    rawCreate,
    /* pfnRename */
    rawRename,
    /* pfnClose */
    rawClose,
    /* pfnRead */
    rawRead,
    /* pfnWrite */
    rawWrite,
    /* pfnFlush */
    rawFlush,
    /* pfnDiscard */
    NULL,
    /* pfnGetVersion */
    rawGetVersion,
    /* pfnGetFileSize */
    rawGetFileSize,
    /* pfnGetPCHSGeometry */
    rawGetPCHSGeometry,
    /* pfnSetPCHSGeometry */
    rawSetPCHSGeometry,
    /* pfnGetLCHSGeometry */
    rawGetLCHSGeometry,
    /* pfnSetLCHSGeometry */
    rawSetLCHSGeometry,
    /* pfnQueryRegions */
    rawQueryRegions,
    /* pfnRegionListRelease */
    rawRegionListRelease,
    /* pfnGetImageFlags */
    rawGetImageFlags,
    /* pfnGetOpenFlags */
    rawGetOpenFlags,
    /* pfnSetOpenFlags */
    rawSetOpenFlags,
    /* pfnGetComment */
    rawGetComment,
    /* pfnSetComment */
    rawSetComment,
    /* pfnGetUuid */
    rawGetUuid,
    /* pfnSetUuid */
    rawSetUuid,
    /* pfnGetModificationUuid */
    rawGetModificationUuid,
    /* pfnSetModificationUuid */
    rawSetModificationUuid,
    /* pfnGetParentUuid */
    rawGetParentUuid,
    /* pfnSetParentUuid */
    rawSetParentUuid,
    /* pfnGetParentModificationUuid */
    rawGetParentModificationUuid,
    /* pfnSetParentModificationUuid */
    rawSetParentModificationUuid,
    /* pfnDump */
    rawDump,
    /* pfnGetTimestamp */
    NULL,
    /* pfnGetParentTimestamp */
    NULL,
    /* pfnSetParentTimestamp */
    NULL,
    /* pfnGetParentFilename */
    NULL,
    /* pfnSetParentFilename */
    NULL,
    /* pfnComposeLocation */
    genericFileComposeLocation,
    /* pfnComposeName */
    genericFileComposeName,
    /* pfnCompact */
    NULL,
    /* pfnResize */
    NULL,
    /* pfnRepair */
    NULL,
    /* pfnTraverseMetadata */
    NULL,
    /* u32VersionEnd */
    VD_IMGBACKEND_VERSION
};
