/* $Id: VISO.cpp $ */
/** @file
 * VISO - Virtual ISO disk image, Core Code.
 */

/*
 * Copyright (C) 2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_VD
#include <VBox/vd-plugin.h>
#include <VBox/err.h>

#include <VBox/log.h>
//#include <VBox/scsiinline.h>
#include <iprt/assert.h>
#include <iprt/ctype.h>
#include <iprt/fsisomaker.h>
#include <iprt/getopt.h>
#include <iprt/mem.h>
#include <iprt/string.h>
#include <iprt/uuid.h>

#include "VDBackends.h"
#include "VDBackendsInline.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** The maximum file size. */
#if ARCH_BITS >= 64
# define VISO_MAX_FILE_SIZE     _32M
#else
# define VISO_MAX_FILE_SIZE     _8M
#endif


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * VBox ISO maker image instance.
 */
typedef struct VISOIMAGE
{
    /** The ISO maker output file handle.
     * This is NIL if in VD_OPEN_FLAGS_INFO mode. */
    RTVFSFILE           hIsoFile;
    /** The image size. */
    uint64_t            cbImage;
    /** The UUID ofr the image. */
    RTUUID              Uuid;

    /** Open flags passed by VD layer. */
    uint32_t            fOpenFlags;
    /** Image name (for debug).
     * Allocation follows the region list, no need to free. */
    const char         *pszFilename;

    /** I/O interface. */
    PVDINTERFACEIOINT   pIfIo;
    /** Error interface. */
    PVDINTERFACEERROR   pIfError;

    /** Internal region list (variable size). */
    VDREGIONLIST        RegionList;
} VISOIMAGE;
/** Pointer to an VBox ISO make image instance. */
typedef VISOIMAGE *PVISOIMAGE;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** NULL-terminated array of supported file extensions. */
static const VDFILEEXTENSION g_aVBoXIsoMakerFileExtensions[] =
{
    //{ "vbox-iso-maker", VDTYPE_OPTICAL_DISC }, - clumsy.
    { "viso",           VDTYPE_OPTICAL_DISC },
    { NULL,             VDTYPE_INVALID      }
};


/**
 * Parses the UUID that follows the marker argument.
 *
 * @returns IPRT status code.
 * @param   pszMarker           The marker.
 * @param   pUuid               Where to return the UUID.
 */
static int visoParseUuid(char *pszMarker, PRTUUID pUuid)
{
    /* Skip the marker. */
    char ch;
    while (   (ch = *pszMarker) != '\0'
           && !RT_C_IS_SPACE(ch)
           && ch != ':'
           && ch != '=')
        pszMarker++;

    /* Skip chars before the value. */
    if (   ch == ':'
        || ch == '=')
        ch = *++pszMarker;
    else
        while (RT_C_IS_SPACE(ch))
            ch = *++pszMarker;
    const char * const pszUuid = pszMarker;

    /* Find the end of the UUID value. */
    while (   ch != '\0'
           && !RT_C_IS_SPACE(ch))
        ch = *++pszMarker;

    /* Validate the value (temporarily terminate the value string) */
    *pszMarker = '\0';
    int rc = RTUuidFromStr(pUuid, pszUuid);
    if (RT_SUCCESS(rc))
    {
        *pszMarker = ch;
        return VINF_SUCCESS;
    }

    /* Complain and return VERR_VD_IMAGE_CORRUPTED to indicate we've identified
       the right image format, but the producer got something wrong. */
    if (pszUuid != pszMarker)
        LogRel(("visoParseUuid: Malformed UUID '%s': %Rrc\n", pszUuid, rc));
    else
        LogRel(("visoParseUuid: Empty/missing UUID!\n"));
    *pszMarker = ch;

    return VERR_VD_IMAGE_CORRUPTED;
}


static int visoProbeWorker(const char *pszFilename, PVDINTERFACEIOINT pIfIo, PRTUUID pUuid)
{
    PVDIOSTORAGE pStorage = NULL;
    int rc = vdIfIoIntFileOpen(pIfIo, pszFilename, RTFILE_O_READ | RTFILE_O_OPEN | RTFILE_O_DENY_NONE, &pStorage);
    if (RT_SUCCESS(rc))
    {
        /*
         * Read the first part of the file.
         */
        uint64_t cbFile = 0;
        rc = vdIfIoIntFileGetSize(pIfIo, pStorage, &cbFile);
        if (RT_SUCCESS(rc))
        {
            char   szChunk[_1K];
            size_t cbToRead = (size_t)RT_MIN(sizeof(szChunk) - 1, cbFile);
            rc = vdIfIoIntFileReadSync(pIfIo, pStorage, 0 /*off*/, szChunk, cbToRead);
            if (RT_SUCCESS(rc))
            {
                szChunk[cbToRead] = '\0';

                /*
                 * Skip leading spaces and check for the eye-catcher.
                 */
                char *psz = szChunk;
                while (RT_C_IS_SPACE(*psz))
                    psz++;
                if (strncmp(psz, RT_STR_TUPLE("--iprt-iso-maker-file-marker")) == 0)
                {
                    rc = visoParseUuid(psz, pUuid);
                    if (RT_SUCCESS(rc))
                    {
                        /*
                         * Check the file size.
                         */
                        if (cbFile <= VISO_MAX_FILE_SIZE)
                            rc = VINF_SUCCESS;
                        else
                        {
                            LogRel(("visoProbeWorker: VERR_VD_INVALID_SIZE - cbFile=%#RX64 cbMaxFile=%#RX64\n",
                                    cbFile, (uint64_t)VISO_MAX_FILE_SIZE));
                            rc = VERR_VD_INVALID_SIZE;
                        }
                    }
                    else
                        rc = VERR_VD_IMAGE_CORRUPTED;
                }
                else
                    rc = VERR_VD_GEN_INVALID_HEADER;
            }
        }
        vdIfIoIntFileClose(pIfIo, pStorage);
    }
    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnProbe}
 */
static DECLCALLBACK(int) visoProbe(const char *pszFilename, PVDINTERFACE pVDIfsDisk, PVDINTERFACE pVDIfsImage, VDTYPE *penmType)
{
    /*
     * Validate input.
     */
    AssertPtrReturn(penmType, VERR_INVALID_POINTER);
    *penmType = VDTYPE_INVALID;

    AssertPtrReturn(pszFilename, VERR_INVALID_POINTER);
    AssertReturn(*pszFilename, VERR_INVALID_POINTER);

    PVDINTERFACEIOINT pIfIo = VDIfIoIntGet(pVDIfsImage);
    AssertPtrReturn(pIfIo, VERR_INVALID_PARAMETER);

    RT_NOREF(pVDIfsDisk);

    /*
     * Share worker with visoOpen and visoSetFlags.
     */
    RTUUID UuidIgn;
    int rc = visoProbeWorker(pszFilename, pIfIo, &UuidIgn);
    if (RT_SUCCESS(rc))
        *penmType = VDTYPE_OPTICAL_DISC;
    else if (rc == VERR_VD_IMAGE_CORRUPTED || rc == VERR_VD_INVALID_SIZE)
        *penmType = VDTYPE_OPTICAL_DISC;
    else
        rc = VERR_VD_GEN_INVALID_HEADER; /* Caller has strict, though undocument, status code expectations. */

    LogFlowFunc(("returns %Rrc - *penmType=%d\n", rc, *penmType));
    return rc;
}


/**
 * Worker for visoOpen and visoSetOpenFlags that creates a VFS file for the ISO.
 *
 * This also updates cbImage and the Uuid members.
 *
 * @returns VBox status code.
 * @param   pThis               The VISO image instance.
 */
static int visoOpenWorker(PVISOIMAGE pThis)
{
    /*
     * Open the file and read it into memory.
     */
    PVDIOSTORAGE pStorage = NULL;
    int rc = vdIfIoIntFileOpen(pThis->pIfIo, pThis->pszFilename, RTFILE_O_READ | RTFILE_O_OPEN | RTFILE_O_DENY_NONE, &pStorage);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Read the file into memory, prefixing it with a dummy command name.
     */
    uint64_t cbFile = 0;
    rc = vdIfIoIntFileGetSize(pThis->pIfIo, pStorage, &cbFile);
    if (RT_SUCCESS(rc))
    {
        if (cbFile <= VISO_MAX_FILE_SIZE)
        {
            static char const s_szCmdPrefix[] = "VBox-Iso-Maker ";

            char *pszContent = (char *)RTMemTmpAlloc(sizeof(s_szCmdPrefix) + cbFile);
            if (pszContent)
            {
                char *pszReadDst = &pszContent[sizeof(s_szCmdPrefix) - 1];
                rc = vdIfIoIntFileReadSync(pThis->pIfIo, pStorage, 0 /*off*/, pszReadDst, (size_t)cbFile);
                if (RT_SUCCESS(rc))
                {
                    /*
                     * Check the file marker and get the UUID that follows it.
                     * Ignore leading blanks.
                     */
                    pszReadDst[(size_t)cbFile] = '\0';
                    memcpy(pszContent, s_szCmdPrefix, sizeof(s_szCmdPrefix) - 1);

                    while (RT_C_IS_SPACE(*pszReadDst))
                        pszReadDst++;
                    if (strncmp(pszReadDst, RT_STR_TUPLE("--iprt-iso-maker-file-marker")) == 0)
                    {
                        rc = visoParseUuid(pszReadDst, &pThis->Uuid);
                        if (RT_SUCCESS(rc))
                        {
                            /*
                             * Make sure it's valid UTF-8 before letting
                             */
                            rc = RTStrValidateEncodingEx(pszContent, sizeof(s_szCmdPrefix) + cbFile,
                                                         RTSTR_VALIDATE_ENCODING_EXACT_LENGTH
                                                         | RTSTR_VALIDATE_ENCODING_ZERO_TERMINATED);
                            if (RT_SUCCESS(rc))
                            {
                                /*
                                 * Convert it into an argument vector.
                                 * Free the content afterwards to reduce memory pressure.
                                 */
                                uint32_t fGetOpt = strncmp(pszReadDst, RT_STR_TUPLE("--iprt-iso-maker-file-marker-ms")) != 0
                                                 ? RTGETOPTARGV_CNV_QUOTE_BOURNE_SH : RTGETOPTARGV_CNV_QUOTE_MS_CRT;
                                fGetOpt |= RTGETOPTARGV_CNV_MODIFY_INPUT;
                                char **papszArgs;
                                int    cArgs;
                                rc = RTGetOptArgvFromString(&papszArgs, &cArgs, pszContent, fGetOpt, NULL);

                                if (RT_SUCCESS(rc))
                                {
                                    /*
                                     * Try instantiate the ISO image maker.
                                     * Free the argument vector afterward to reduce memory pressure.
                                     */
                                    RTVFSFILE       hVfsFile;
                                    RTERRINFOSTATIC ErrInfo;
                                    rc = RTFsIsoMakerCmdEx(cArgs, papszArgs, &hVfsFile, RTErrInfoInitStatic(&ErrInfo));

                                    RTGetOptArgvFreeEx(papszArgs, fGetOpt);
                                    papszArgs = NULL;

                                    if (RT_SUCCESS(rc))
                                    {
                                        uint64_t cbImage;
                                        rc = RTVfsFileGetSize(hVfsFile, &cbImage);
                                        if (RT_SUCCESS(rc))
                                        {
                                            /*
                                             * Update the state.
                                             */
                                            pThis->cbImage = cbImage;
                                            pThis->RegionList.aRegions[0].cRegionBlocksOrBytes = cbImage;

                                            pThis->hIsoFile = hVfsFile;
                                            hVfsFile = NIL_RTVFSFILE;

                                            rc = VINF_SUCCESS;
                                            LogRel(("VISO: %'RU64 bytes (%#RX64) - %s\n", cbImage, cbImage, pThis->pszFilename));
                                        }

                                        RTVfsFileRelease(hVfsFile);
                                    }
                                    else if (RTErrInfoIsSet(&ErrInfo.Core))
                                    {
                                        LogRel(("visoOpenWorker: RTFsIsoMakerCmdEx failed: %Rrc - %s\n", rc, ErrInfo.Core.pszMsg));
                                        vdIfError(pThis->pIfError, rc, RT_SRC_POS, "VISO: %s", ErrInfo.Core.pszMsg);
                                    }
                                    else
                                    {
                                        LogRel(("visoOpenWorker: RTFsIsoMakerCmdEx failed: %Rrc\n", rc));
                                        vdIfError(pThis->pIfError, rc, RT_SRC_POS, "VISO: RTFsIsoMakerCmdEx failed: %Rrc", rc);
                                    }
                                }
                            }
                            else
                                vdIfError(pThis->pIfError, rc, RT_SRC_POS, "VISO: Invalid file encoding");
                        }
                    }
                    else
                        rc = VERR_VD_GEN_INVALID_HEADER;
                }

                RTMemTmpFree(pszContent);
            }
            else
                rc = VERR_NO_TMP_MEMORY;
        }
        else
        {
            LogRel(("visoOpen: VERR_VD_INVALID_SIZE - cbFile=%#RX64 cbMaxFile=%#RX64\n",
                    cbFile, (uint64_t)VISO_MAX_FILE_SIZE));
            rc = VERR_VD_INVALID_SIZE;
        }
    }

    vdIfIoIntFileClose(pThis->pIfIo, pStorage);
    return rc;
}


/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnOpen}
 */
static DECLCALLBACK(int) visoOpen(const char *pszFilename, unsigned uOpenFlags, PVDINTERFACE pVDIfsDisk, PVDINTERFACE pVDIfsImage,
                                  VDTYPE enmType, void **ppBackendData)
{
    uint32_t const fOpenFlags = uOpenFlags;
    LogFlowFunc(("pszFilename='%s' fOpenFlags=%#x pVDIfsDisk=%p pVDIfsImage=%p enmType=%u ppBackendData=%p\n",
                 pszFilename, fOpenFlags, pVDIfsDisk, pVDIfsImage, enmType, ppBackendData));

    /*
     * Validate input.
     */
    AssertPtrReturn(ppBackendData, VERR_INVALID_POINTER);
    *ppBackendData = NULL;

    AssertPtrReturn(pszFilename, VERR_INVALID_POINTER);
    AssertReturn(*pszFilename, VERR_INVALID_POINTER);

    AssertReturn(!(fOpenFlags & ~VD_OPEN_FLAGS_MASK), VERR_INVALID_FLAGS);

    PVDINTERFACEIOINT pIfIo = VDIfIoIntGet(pVDIfsImage);
    AssertPtrReturn(pIfIo, VERR_INVALID_PARAMETER);

    PVDINTERFACEERROR pIfError = VDIfErrorGet(pVDIfsDisk);

    AssertReturn(enmType == VDTYPE_OPTICAL_DISC, VERR_NOT_SUPPORTED);

    /*
     * Allocate and initialize the backend image instance data.
     */
    int         rc;
    size_t      cbFilename = strlen(pszFilename) + 1;
    PVISOIMAGE  pThis = (PVISOIMAGE)RTMemAllocZ(RT_UOFFSETOF(VISOIMAGE, RegionList.aRegions[1]) + cbFilename);
    if (pThis)
    {
        pThis->hIsoFile    = NIL_RTVFSFILE;
        pThis->cbImage     = 0;
        pThis->fOpenFlags  = fOpenFlags;
        pThis->pszFilename = (char *)memcpy(&pThis->RegionList.aRegions[1], pszFilename, cbFilename);
        pThis->pIfIo       = pIfIo;
        pThis->pIfError    = pIfError;

        pThis->RegionList.fFlags   = 0;
        pThis->RegionList.cRegions = 1;
        pThis->RegionList.aRegions[0].offRegion            = 0;
        pThis->RegionList.aRegions[0].cRegionBlocksOrBytes = 0;
        pThis->RegionList.aRegions[0].cbBlock              = 2048;
        pThis->RegionList.aRegions[0].enmDataForm          = VDREGIONDATAFORM_RAW;
        pThis->RegionList.aRegions[0].enmMetadataForm      = VDREGIONMETADATAFORM_NONE;
        pThis->RegionList.aRegions[0].cbData               = 2048;
        pThis->RegionList.aRegions[0].cbMetadata           = 0;

        /*
         * Only go all the way if this isn't an info query.  Re-mastering an ISO
         * can potentially be a lot of work and we don't want to go thru with it
         * just because the GUI wants to display the image size.
         */
        if (!(fOpenFlags & VD_OPEN_FLAGS_INFO))
            rc = visoOpenWorker(pThis);
        else
            rc = visoProbeWorker(pThis->pszFilename, pThis->pIfIo, &pThis->Uuid);
        if (RT_SUCCESS(rc))
        {
            *ppBackendData = pThis;
            LogFlowFunc(("returns VINF_SUCCESS (UUID=%RTuuid, pszFilename=%s)\n", &pThis->Uuid, pThis->pszFilename));
            return VINF_SUCCESS;
        }

        RTMemFree(pThis);
    }
    else
        rc = VERR_NO_MEMORY;
    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}


/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnClose}
 */
static DECLCALLBACK(int) visoClose(void *pBackendData, bool fDelete)
{
    PVISOIMAGE pThis = (PVISOIMAGE)pBackendData;
    LogFlowFunc(("pThis=%p fDelete=%RTbool\n", pThis, fDelete));

    if (pThis)
    {
        if (fDelete)
            vdIfIoIntFileDelete(pThis->pIfIo, pThis->pszFilename);

        if (pThis->hIsoFile != NIL_RTVFSFILE)
        {
            RTVfsFileRelease(pThis->hIsoFile);
            pThis->hIsoFile = NIL_RTVFSFILE;
        }

        RTMemFree(pThis);
    }

    LogFlowFunc(("returns VINF_SUCCESS\n"));
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnRead}
 */
static DECLCALLBACK(int) visoRead(void *pBackendData, uint64_t uOffset, size_t cbToRead, PVDIOCTX pIoCtx, size_t *pcbActuallyRead)
{
    PVISOIMAGE pThis = (PVISOIMAGE)pBackendData;
    uint64_t   off   = uOffset;
    AssertPtrReturn(pThis, VERR_VD_NOT_OPENED);
    AssertReturn(pThis->hIsoFile != NIL_RTVFSFILE, VERR_VD_NOT_OPENED);
    LogFlowFunc(("pThis=%p off=%#RX64 cbToRead=%#zx pIoCtx=%p pcbActuallyRead=%p\n", pThis, off, cbToRead, pIoCtx, pcbActuallyRead));

    /*
     * Check request.
     */
    AssertReturn(   off < pThis->cbImage
                 || (off == pThis->cbImage && cbToRead == 0), VERR_EOF);

    uint64_t cbLeftInImage = pThis->cbImage - off;
    if (cbToRead >= cbLeftInImage)
        cbToRead = cbLeftInImage; /* ASSUMES the caller can deal with this, given the pcbActuallyRead parameter... */

    /*
     * Work the I/O context using vdIfIoIntIoCtxSegArrayCreate.
     */
    int    rc = VINF_SUCCESS;
    size_t cbActuallyRead = 0;
    while (cbToRead > 0)
    {
        RTSGSEG     Seg;
        unsigned    cSegs = 1;
        size_t      cbThisRead = vdIfIoIntIoCtxSegArrayCreate(pThis->pIfIo, pIoCtx, &Seg, &cSegs, cbToRead);
        AssertBreakStmt(cbThisRead != 0, rc = VERR_INTERNAL_ERROR_2);
        Assert(cbThisRead == Seg.cbSeg);

        rc = RTVfsFileReadAt(pThis->hIsoFile, off, Seg.pvSeg, cbThisRead, NULL);
        AssertRCBreak(rc);

        /* advance. */
        cbActuallyRead += cbThisRead;
        off            += cbThisRead;
        cbToRead       -= cbThisRead;
    }

    *pcbActuallyRead = cbActuallyRead;
    return rc;
}

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnWrite}
 */
static DECLCALLBACK(int) visoWrite(void *pBackendData, uint64_t uOffset, size_t cbToWrite,
                                   PVDIOCTX pIoCtx, size_t *pcbWriteProcess, size_t *pcbPreRead,
                                   size_t *pcbPostRead, unsigned fWrite)
{
    RT_NOREF(uOffset, cbToWrite, pIoCtx, pcbWriteProcess, pcbPreRead, pcbPostRead, fWrite);
    PVISOIMAGE pThis = (PVISOIMAGE)pBackendData;
    AssertPtrReturn(pThis, VERR_VD_NOT_OPENED);
    AssertReturn(pThis->hIsoFile != NIL_RTVFSFILE, VERR_VD_NOT_OPENED);
    LogFlowFunc(("pThis=%p off=%#RX64 pIoCtx=%p cbToWrite=%#zx pcbWriteProcess=%p pcbPreRead=%p pcbPostRead=%p -> VERR_VD_IMAGE_READ_ONLY\n",
                 pThis, uOffset, pIoCtx, cbToWrite, pcbWriteProcess, pcbPreRead, pcbPostRead));
    return VERR_VD_IMAGE_READ_ONLY;
}

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnFlush}
 */
static DECLCALLBACK(int) visoFlush(void *pBackendData, PVDIOCTX pIoCtx)
{
    PVISOIMAGE pThis = (PVISOIMAGE)pBackendData;
    AssertPtrReturn(pThis, VERR_VD_NOT_OPENED);
    AssertReturn(pThis->hIsoFile != NIL_RTVFSFILE, VERR_VD_NOT_OPENED);
    RT_NOREF(pIoCtx);
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnGetVersion}
 */
static DECLCALLBACK(unsigned) visoGetVersion(void *pBackendData)
{
    PVISOIMAGE pThis = (PVISOIMAGE)pBackendData;
    AssertPtrReturn(pThis, 0);
    LogFlowFunc(("pThis=%#p -> 1\n", pThis));
    return 1;
}

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnGetFileSize}
 */
static DECLCALLBACK(uint64_t) visoGetFileSize(void *pBackendData)
{
    PVISOIMAGE pThis = (PVISOIMAGE)pBackendData;
    AssertPtrReturn(pThis, 0);
    LogFlowFunc(("pThis=%p -> %RX64 (%s)\n", pThis, pThis->cbImage, pThis->hIsoFile == NIL_RTVFSFILE ? "fake!" : "real"));
    return pThis->cbImage;
}

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnGetPCHSGeometry}
 */
static DECLCALLBACK(int) visoGetPCHSGeometry(void *pBackendData, PVDGEOMETRY pPCHSGeometry)
{
    PVISOIMAGE pThis = (PVISOIMAGE)pBackendData;
    AssertPtrReturn(pThis, VERR_VD_NOT_OPENED);
    LogFlowFunc(("pThis=%p pPCHSGeometry=%p -> VERR_NOT_SUPPORTED\n", pThis, pPCHSGeometry));
    RT_NOREF(pPCHSGeometry);
    return VERR_NOT_SUPPORTED;
}

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnSetPCHSGeometry}
 */
static DECLCALLBACK(int) visoSetPCHSGeometry(void *pBackendData, PCVDGEOMETRY pPCHSGeometry)
{
    PVISOIMAGE pThis = (PVISOIMAGE)pBackendData;
    AssertPtrReturn(pThis, VERR_VD_NOT_OPENED);
    LogFlowFunc(("pThis=%p pPCHSGeometry=%p:{%u/%u/%u} -> VERR_VD_IMAGE_READ_ONLY\n",
                 pThis, pPCHSGeometry, pPCHSGeometry->cCylinders, pPCHSGeometry->cHeads, pPCHSGeometry->cSectors));
    RT_NOREF(pPCHSGeometry);
    return VERR_VD_IMAGE_READ_ONLY;
}

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnGetLCHSGeometry}
 */
static DECLCALLBACK(int) visoGetLCHSGeometry(void *pBackendData, PVDGEOMETRY pLCHSGeometry)
{
    PVISOIMAGE pThis = (PVISOIMAGE)pBackendData;
    AssertPtrReturn(pThis, VERR_VD_NOT_OPENED);
    LogFlowFunc(("pThis=%p pLCHSGeometry=%p -> VERR_NOT_SUPPORTED\n", pThis, pLCHSGeometry));
    RT_NOREF(pLCHSGeometry);
    return VERR_NOT_SUPPORTED;
}

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnSetLCHSGeometry}
 */
static DECLCALLBACK(int) visoSetLCHSGeometry(void *pBackendData, PCVDGEOMETRY pLCHSGeometry)
{
    PVISOIMAGE pThis = (PVISOIMAGE)pBackendData;
    AssertPtrReturn(pThis, VERR_VD_NOT_OPENED);
    LogFlowFunc(("pThis=%p pLCHSGeometry=%p:{%u/%u/%u} -> VERR_VD_IMAGE_READ_ONLY\n",
                 pThis, pLCHSGeometry, pLCHSGeometry->cCylinders, pLCHSGeometry->cHeads, pLCHSGeometry->cSectors));
    RT_NOREF(pLCHSGeometry);
    return VERR_VD_IMAGE_READ_ONLY;
}

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnQueryRegions}
 */
static DECLCALLBACK(int) visoQueryRegions(void *pBackendData, PCVDREGIONLIST *ppRegionList)
{
    PVISOIMAGE pThis = (PVISOIMAGE)pBackendData;
    *ppRegionList = NULL;
    AssertPtrReturn(pThis, VERR_VD_NOT_OPENED);

    *ppRegionList = &pThis->RegionList;
    LogFlowFunc(("returns VINF_SUCCESS (one region: 0 LB %RX64; pThis=%p)\n", pThis->RegionList.aRegions[0].cbData, pThis));
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnRegionListRelease}
 */
static DECLCALLBACK(void) visoRegionListRelease(void *pBackendData, PCVDREGIONLIST pRegionList)
{
    /* Nothing to do here.  Just assert the input to avoid unused parameter warnings. */
    PVISOIMAGE pThis = (PVISOIMAGE)pBackendData;
    LogFlowFunc(("pThis=%p pRegionList=%p\n", pThis, pRegionList));
    AssertPtrReturnVoid(pThis);
    AssertReturnVoid(pRegionList == &pThis->RegionList || pRegionList == 0);
}

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnGetImageFlags}
 */
static DECLCALLBACK(unsigned) visoGetImageFlags(void *pBackendData)
{
    PVISOIMAGE pThis = (PVISOIMAGE)pBackendData;
    LogFlowFunc(("pThis=%p -> VD_IMAGE_FLAGS_NONE\n", pThis));
    AssertPtrReturn(pThis, VD_IMAGE_FLAGS_NONE);
    return VD_IMAGE_FLAGS_NONE;
}

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnGetOpenFlags}
 */
static DECLCALLBACK(unsigned) visoGetOpenFlags(void *pBackendData)
{
    LogFlowFunc(("pBackendData=%#p\n", pBackendData));
    PVISOIMAGE pThis = (PVISOIMAGE)pBackendData;
    AssertPtrReturn(pThis, 0);

    LogFlowFunc(("returns %#x\n", pThis->fOpenFlags));
    return pThis->fOpenFlags;
}

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnSetOpenFlags}
 */
static DECLCALLBACK(int) visoSetOpenFlags(void *pBackendData, unsigned uOpenFlags)
{
    PVISOIMAGE pThis = (PVISOIMAGE)pBackendData;
    LogFlowFunc(("pThis=%p fOpenFlags=%#x\n", pThis, uOpenFlags));

    AssertPtrReturn(pThis, VERR_INVALID_POINTER);
    uint32_t const fSupported = VD_OPEN_FLAGS_READONLY | VD_OPEN_FLAGS_INFO
                              | VD_OPEN_FLAGS_ASYNC_IO | VD_OPEN_FLAGS_SHAREABLE
                              | VD_OPEN_FLAGS_SEQUENTIAL | VD_OPEN_FLAGS_SKIP_CONSISTENCY_CHECKS;
    AssertMsgReturn(!(uOpenFlags & ~fSupported), ("fOpenFlags=%#x\n", uOpenFlags), VERR_INVALID_FLAGS);

    /*
     * Only react if we switch from VD_OPEN_FLAGS_INFO to non-VD_OPEN_FLAGS_INFO mode,
     * becuase that means we need to open the image.
     */
    if (   (pThis->fOpenFlags & VD_OPEN_FLAGS_INFO)
        && !(uOpenFlags & VD_OPEN_FLAGS_INFO)
        && pThis->hIsoFile == NIL_RTVFSFILE)
    {
        int rc = visoOpenWorker(pThis);
        if (RT_FAILURE(rc))
        {
            LogFlowFunc(("returns %Rrc\n", rc));
            return rc;
        }
    }

    /*
     * Update the flags.
     */
    pThis->fOpenFlags &= ~fSupported;
    pThis->fOpenFlags |= fSupported & uOpenFlags;
    pThis->fOpenFlags |= VD_OPEN_FLAGS_READONLY;
    if (pThis->hIsoFile != NIL_RTVFSFILE)
        pThis->fOpenFlags &= ~VD_OPEN_FLAGS_INFO;

    return VINF_SUCCESS;
}

#define uOpenFlags fOpenFlags /* sigh */

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnGetComment}
 */
VD_BACKEND_CALLBACK_GET_COMMENT_DEF_NOT_SUPPORTED(visoGetComment);

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnSetComment}
 */
VD_BACKEND_CALLBACK_SET_COMMENT_DEF_NOT_SUPPORTED(visoSetComment, PVISOIMAGE);

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnGetUuid}
 */
static DECLCALLBACK(int) visoGetUuid(void *pBackendData, PRTUUID pUuid)
{
    PVISOIMAGE pThis = (PVISOIMAGE)pBackendData;
    *pUuid = pThis->Uuid;
    LogFlowFunc(("returns VIF_SUCCESS (%RTuuid)\n", pUuid));
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnSetUuid}
 */
VD_BACKEND_CALLBACK_SET_UUID_DEF_NOT_SUPPORTED(visoSetUuid, PVISOIMAGE);

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnGetModificationUuid}
 */
VD_BACKEND_CALLBACK_GET_UUID_DEF_NOT_SUPPORTED(visoGetModificationUuid);

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnSetModificationUuid}
 */
VD_BACKEND_CALLBACK_SET_UUID_DEF_NOT_SUPPORTED(visoSetModificationUuid, PVISOIMAGE);

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnGetParentUuid}
 */
VD_BACKEND_CALLBACK_GET_UUID_DEF_NOT_SUPPORTED(visoGetParentUuid);

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnSetParentUuid}
 */
VD_BACKEND_CALLBACK_SET_UUID_DEF_NOT_SUPPORTED(visoSetParentUuid, PVISOIMAGE);

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnGetParentModificationUuid}
 */
VD_BACKEND_CALLBACK_GET_UUID_DEF_NOT_SUPPORTED(visoGetParentModificationUuid);

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnSetParentModificationUuid}
 */
VD_BACKEND_CALLBACK_SET_UUID_DEF_NOT_SUPPORTED(visoSetParentModificationUuid, PVISOIMAGE);

#undef uOpenFlags

/**
 * @interface_method_impl{VDIMAGEBACKEND,pfnDump}
 */
static DECLCALLBACK(void) visoDump(void *pBackendData)
{
    PVISOIMAGE pThis = (PVISOIMAGE)pBackendData;
    AssertPtrReturnVoid(pThis);

    vdIfErrorMessage(pThis->pIfError, "Dumping CUE image '%s' fOpenFlags=%x cbImage=%#RX64\n",
                     pThis->pszFilename, pThis->fOpenFlags, pThis->cbImage);
}



/**
 * VBox ISO maker backend.
 */
const VDIMAGEBACKEND g_VBoxIsoMakerBackend =
{
    /* u32Version */
    VD_IMGBACKEND_VERSION,
    /* pszBackendName */
    "VBoxIsoMaker",
    /* uBackendCaps */
    VD_CAP_FILE,
    /* paFileExtensions */
    g_aVBoXIsoMakerFileExtensions,
    /* paConfigInfo */
    NULL,
    /* pfnProbe */
    visoProbe,
    /* pfnOpen */
    visoOpen,
    /* pfnCreate */
    NULL,
    /* pfnRename */
    NULL,
    /* pfnClose */
    visoClose,
    /* pfnRead */
    visoRead,
    /* pfnWrite */
    visoWrite,
    /* pfnFlush */
    visoFlush,
    /* pfnDiscard */
    NULL,
    /* pfnGetVersion */
    visoGetVersion,
    /* pfnGetFileSize */
    visoGetFileSize,
    /* pfnGetPCHSGeometry */
    visoGetPCHSGeometry,
    /* pfnSetPCHSGeometry */
    visoSetPCHSGeometry,
    /* pfnGetLCHSGeometry */
    visoGetLCHSGeometry,
    /* pfnSetLCHSGeometry */
    visoSetLCHSGeometry,
    /* pfnQueryRegions */
    visoQueryRegions,
    /* pfnRegionListRelease */
    visoRegionListRelease,
    /* pfnGetImageFlags */
    visoGetImageFlags,
    /* pfnGetOpenFlags */
    visoGetOpenFlags,
    /* pfnSetOpenFlags */
    visoSetOpenFlags,
    /* pfnGetComment */
    visoGetComment,
    /* pfnSetComment */
    visoSetComment,
    /* pfnGetUuid */
    visoGetUuid,
    /* pfnSetUuid */
    visoSetUuid,
    /* pfnGetModificationUuid */
    visoGetModificationUuid,
    /* pfnSetModificationUuid */
    visoSetModificationUuid,
    /* pfnGetParentUuid */
    visoGetParentUuid,
    /* pfnSetParentUuid */
    visoSetParentUuid,
    /* pfnGetParentModificationUuid */
    visoGetParentModificationUuid,
    /* pfnSetParentModificationUuid */
    visoSetParentModificationUuid,
    /* pfnDump */
    visoDump,
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

