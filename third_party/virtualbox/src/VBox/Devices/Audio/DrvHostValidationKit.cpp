/* $Id: DrvHostValidationKit.cpp $ */
/** @file
 * ValidationKit audio driver - host backend for dumping and injecting audio data
 *                              from/to the device emulation.
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <iprt/alloc.h>
#include <iprt/uuid.h> /* For PDMIBASE_2_PDMDRV. */

#define LOG_GROUP LOG_GROUP_DRV_HOST_AUDIO
#include <VBox/log.h>
#include <VBox/vmm/pdmaudioifs.h>

#include "DrvAudio.h"
#include "VBoxDD.h"


/**
 * Structure for keeping a VAKIT input/output stream.
 */
typedef struct VAKITAUDIOSTREAM
{
    /** The stream's acquired configuration. */
    PPDMAUDIOSTREAMCFG pCfg;
    /** Audio file to dump output to or read input from. */
    PPDMAUDIOFILE      pFile;
    /** Text file to store timing of audio buffers submittions**/
    RTFILE             hFileTiming;
    /** Timestamp of the first play or record request**/
    uint64_t           tsStarted;
    /** Total number of samples played or recorded so far**/
    uint32_t           uSamplesSinceStarted;
    union
    {
        struct
        {
            /** Timestamp of last captured samples. */
            uint64_t   tsLastCaptured;
        } In;
        struct
        {
            /** Timestamp of last played samples. */
            uint64_t   tsLastPlayed;
            uint8_t   *pu8PlayBuffer;
            uint32_t   cbPlayBuffer;
        } Out;
    };
} VAKITAUDIOSTREAM, *PVAKITAUDIOSTREAM;

/**
 * VAKIT audio driver instance data.
 * @implements PDMIAUDIOCONNECTOR
 */
typedef struct DRVHOSTVAKITAUDIO
{
    /** Pointer to the driver instance structure. */
    PPDMDRVINS    pDrvIns;
    /** Pointer to host audio interface. */
    PDMIHOSTAUDIO IHostAudio;
} DRVHOSTVAKITAUDIO, *PDRVHOSTVAKITAUDIO;

/*******************************************PDM_AUDIO_DRIVER******************************/


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetConfig}
 */
static DECLCALLBACK(int) drvHostVaKitAudioGetConfig(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDCFG pBackendCfg)
{
    RT_NOREF(pInterface);
    AssertPtrReturn(pBackendCfg, VERR_INVALID_POINTER);

    pBackendCfg->cbStreamOut    = sizeof(VAKITAUDIOSTREAM);
    pBackendCfg->cbStreamIn     = sizeof(VAKITAUDIOSTREAM);

    pBackendCfg->cMaxStreamsOut = 1; /* Output */
    pBackendCfg->cMaxStreamsIn  = 2; /* Line input + microphone input. */

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnInit}
 */
static DECLCALLBACK(int) drvHostVaKitAudioInit(PPDMIHOSTAUDIO pInterface)
{
    RT_NOREF(pInterface);

    LogFlowFuncLeaveRC(VINF_SUCCESS);
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnShutdown}
 */
static DECLCALLBACK(void) drvHostVaKitAudioShutdown(PPDMIHOSTAUDIO pInterface)
{
    RT_NOREF(pInterface);
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetStatus}
 */
static DECLCALLBACK(PDMAUDIOBACKENDSTS) drvHostVaKitAudioGetStatus(PPDMIHOSTAUDIO pInterface, PDMAUDIODIR enmDir)
{
    RT_NOREF(enmDir);
    AssertPtrReturn(pInterface, PDMAUDIOBACKENDSTS_UNKNOWN);

    return PDMAUDIOBACKENDSTS_RUNNING;
}


static int debugCreateStreamIn(PDRVHOSTVAKITAUDIO pDrv, PVAKITAUDIOSTREAM pStreamDbg,
                               PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    RT_NOREF(pDrv, pStreamDbg, pCfgReq);

    if (pCfgAcq)
        pCfgAcq->cFrameBufferHint = _1K;

    return VINF_SUCCESS;
}


static int debugCreateStreamOut(PDRVHOSTVAKITAUDIO pDrv, PVAKITAUDIOSTREAM pStreamDbg,
                                PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    RT_NOREF(pDrv);

    int rc = VINF_SUCCESS;

    pStreamDbg->tsStarted = 0;
    pStreamDbg->uSamplesSinceStarted = 0;
    pStreamDbg->Out.tsLastPlayed  = 0;
    pStreamDbg->Out.cbPlayBuffer  = 16 * _1K * PDMAUDIOSTREAMCFG_F2B(pCfgReq, 1); /** @todo Make this configurable? */
    pStreamDbg->Out.pu8PlayBuffer = (uint8_t *)RTMemAlloc(pStreamDbg->Out.cbPlayBuffer);
    if (!pStreamDbg->Out.pu8PlayBuffer)
        rc = VERR_NO_MEMORY;

    if (RT_SUCCESS(rc))
    {
        char szTemp[RTPATH_MAX];
        rc = RTPathTemp(szTemp, sizeof(szTemp));

        RTPathAppend(szTemp, sizeof(szTemp), "VBoxTestTmp\\VBoxAudioValKit");

        if (RT_SUCCESS(rc))
        {
            char szFile[RTPATH_MAX + 1];

            rc = DrvAudioHlpGetFileName(szFile, RT_ELEMENTS(szFile), szTemp, "VaKit",
                                        0 /* Instance */, PDMAUDIOFILETYPE_WAV, PDMAUDIOFILENAME_FLAG_NONE);
            if (RT_SUCCESS(rc))
            {
                rc = DrvAudioHlpFileCreate(PDMAUDIOFILETYPE_WAV, szFile, PDMAUDIOFILE_FLAG_NONE, &pStreamDbg->pFile);
                if (RT_SUCCESS(rc))
                    rc = DrvAudioHlpFileOpen(pStreamDbg->pFile, PDMAUDIOFILE_DEFAULT_OPEN_FLAGS, &pCfgReq->Props);
            }

            if (RT_FAILURE(rc))
            {
                LogRel(("VaKitAudio: Creating output file '%s' failed with %Rrc\n", szFile, rc));
            }
            else
            {
                size_t cch;
                char szTimingInfo[128];
                cch = RTStrPrintf(szTimingInfo, sizeof(szTimingInfo), "# %dHz %dch %dbps\n",
                    pCfgReq->Props.uHz, pCfgReq->Props.cChannels, pCfgReq->Props.cBits);

                RTFileWrite(pStreamDbg->hFileTiming, szTimingInfo, cch, NULL);
            }
        }
        else
            LogRel(("VaKitAudio: Unable to retrieve temp dir: %Rrc\n", rc));
    }

    if (RT_SUCCESS(rc))
    {
        if (pCfgAcq)
            pCfgAcq->cFrameBufferHint = PDMAUDIOSTREAMCFG_B2F(pCfgAcq, pStreamDbg->Out.cbPlayBuffer);
    }

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamCreate}
 */
static DECLCALLBACK(int) drvHostVaKitAudioStreamCreate(PPDMIHOSTAUDIO pInterface,
                                                       PPDMAUDIOBACKENDSTREAM pStream,
                                                       PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgReq,    VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgAcq,    VERR_INVALID_POINTER);

    PDRVHOSTVAKITAUDIO pDrv       = RT_FROM_MEMBER(pInterface, DRVHOSTVAKITAUDIO, IHostAudio);
    PVAKITAUDIOSTREAM  pStreamDbg = (PVAKITAUDIOSTREAM)pStream;

    int rc;
    if (pCfgReq->enmDir == PDMAUDIODIR_IN)
        rc = debugCreateStreamIn( pDrv, pStreamDbg, pCfgReq, pCfgAcq);
    else
        rc = debugCreateStreamOut(pDrv, pStreamDbg, pCfgReq, pCfgAcq);

    if (RT_SUCCESS(rc))
    {
        pStreamDbg->pCfg = DrvAudioHlpStreamCfgDup(pCfgAcq);
        if (!pStreamDbg->pCfg)
            rc = VERR_NO_MEMORY;
    }

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamPlay}
 */
static DECLCALLBACK(int) drvHostVaKitAudioStreamPlay(PPDMIHOSTAUDIO pInterface,
                                                     PPDMAUDIOBACKENDSTREAM pStream, const void *pvBuf, uint32_t cxBuf,
                                                     uint32_t *pcxWritten)
{
    PDRVHOSTVAKITAUDIO pDrv       = RT_FROM_MEMBER(pInterface, DRVHOSTVAKITAUDIO, IHostAudio);
    PVAKITAUDIOSTREAM  pStreamDbg = (PVAKITAUDIOSTREAM)pStream;
    RT_NOREF(pDrv);

    uint64_t tsSinceStart;
    size_t cch;
    char szTimingInfo[128];

    if (pStreamDbg->tsStarted == 0)
    {
        pStreamDbg->tsStarted = RTTimeNanoTS();
        tsSinceStart = 0;
    }
    else
    {
        tsSinceStart = RTTimeNanoTS() - pStreamDbg->tsStarted;
    }

    // Microseconds are used everythere below
    uint32_t sBuf = cxBuf >> pStreamDbg->pCfg->Props.cShift;
    cch = RTStrPrintf(szTimingInfo, sizeof(szTimingInfo), "%d %d %d %d\n",
        (uint32_t)(tsSinceStart / 1000), // Host time elapsed since Guest submitted the first buffer for playback
        (uint32_t)(pStreamDbg->uSamplesSinceStarted * 1.0E6 / pStreamDbg->pCfg->Props.uHz), // how long all the samples submitted previously were played
        (uint32_t)(sBuf * 1.0E6 / pStreamDbg->pCfg->Props.uHz), // how long a new uSamplesReady samples should\will be played
        sBuf);
    RTFileWrite(pStreamDbg->hFileTiming, szTimingInfo, cch, NULL);
    pStreamDbg->uSamplesSinceStarted += sBuf;

    /* Remember when samples were consumed. */
   // pStreamDbg->Out.tsLastPlayed = PDMDrvHlpTMGetVirtualTime(pDrv->pDrvIns);;

    int rc2 = DrvAudioHlpFileWrite(pStreamDbg->pFile, pvBuf, cxBuf, 0 /* fFlags */);
    if (RT_FAILURE(rc2))
        LogRel(("DebugAudio: Writing output failed with %Rrc\n", rc2));

    *pcxWritten = cxBuf;

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamCapture}
 */
static DECLCALLBACK(int) drvHostVaKitAudioStreamCapture(PPDMIHOSTAUDIO pInterface,
                                                        PPDMAUDIOBACKENDSTREAM pStream, void *pvBuf, uint32_t cxBuf,
                                                        uint32_t *pcxRead)
{
    RT_NOREF(pInterface, pStream, pvBuf, cxBuf);

    /* Never capture anything. */
    if (pcxRead)
        *pcxRead = 0;

    return VINF_SUCCESS;
}


static int debugDestroyStreamIn(PDRVHOSTVAKITAUDIO pDrv, PVAKITAUDIOSTREAM pStreamDbg)
{
    RT_NOREF(pDrv, pStreamDbg);
    return VINF_SUCCESS;
}


static int debugDestroyStreamOut(PDRVHOSTVAKITAUDIO pDrv, PVAKITAUDIOSTREAM pStreamDbg)
{
    RT_NOREF(pDrv);

    if (pStreamDbg->Out.pu8PlayBuffer)
    {
        RTMemFree(pStreamDbg->Out.pu8PlayBuffer);
        pStreamDbg->Out.pu8PlayBuffer = NULL;
    }

    if (pStreamDbg->pFile)
    {
        size_t cbDataSize = DrvAudioHlpFileGetDataSize(pStreamDbg->pFile);
        if (cbDataSize)
            LogRel(("VaKitAudio: Created output file '%s' (%zu bytes)\n", pStreamDbg->pFile->szName, cbDataSize));

        DrvAudioHlpFileDestroy(pStreamDbg->pFile);
        pStreamDbg->pFile = NULL;
    }

    return VINF_SUCCESS;
}


static DECLCALLBACK(int) drvHostVaKitAudioStreamDestroy(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);

    PDRVHOSTVAKITAUDIO pDrv       = RT_FROM_MEMBER(pInterface, DRVHOSTVAKITAUDIO, IHostAudio);
    PVAKITAUDIOSTREAM  pStreamDbg = (PVAKITAUDIOSTREAM)pStream;

    if (!pStreamDbg->pCfg) /* Not (yet) configured? Skip. */
        return VINF_SUCCESS;

    int rc;
    if (pStreamDbg->pCfg->enmDir == PDMAUDIODIR_IN)
        rc = debugDestroyStreamIn (pDrv, pStreamDbg);
    else
        rc = debugDestroyStreamOut(pDrv, pStreamDbg);

    if (RT_SUCCESS(rc))
    {
        DrvAudioHlpStreamCfgFree(pStreamDbg->pCfg);
        pStreamDbg->pCfg = NULL;
    }

    return rc;
}

static DECLCALLBACK(int) drvHostVaKitAudioStreamControl(PPDMIHOSTAUDIO pInterface,
                                                        PPDMAUDIOBACKENDSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    RT_NOREF(enmStreamCmd);
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetReadable}
 */
static DECLCALLBACK(uint32_t) drvHostVaKitAudioStreamGetReadable(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface, pStream);

    return UINT32_MAX;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetWritable}
 */
static DECLCALLBACK(uint32_t) drvHostVaKitAudioStreamGetWritable(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface, pStream);

    return UINT32_MAX;
}

static DECLCALLBACK(PDMAUDIOSTREAMSTS) drvHostVaKitAudioStreamGetStatus(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface, pStream);

    return (PDMAUDIOSTREAMSTS_FLAG_INITIALIZED | PDMAUDIOSTREAMSTS_FLAG_ENABLED);
}

static DECLCALLBACK(int) drvHostVaKitAudioStreamIterate(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface, pStream);
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) drvHostVaKitAudioQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS         pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVHOSTVAKITAUDIO pThis   = PDMINS_2_DATA(pDrvIns, PDRVHOSTVAKITAUDIO);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIHOSTAUDIO, &pThis->IHostAudio);
    return NULL;
}


/**
 * Constructs a VaKit audio driver instance.
 *
 * @copydoc FNPDMDRVCONSTRUCT
 */
static DECLCALLBACK(int) drvHostVaKitAudioConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    RT_NOREF(pCfg, fFlags);
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);
    PDRVHOSTVAKITAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTVAKITAUDIO);
    LogRel(("Audio: Initializing VAKIT driver\n"));

    /*
     * Init the static parts.
     */
    pThis->pDrvIns                   = pDrvIns;
    /* IBase */
    pDrvIns->IBase.pfnQueryInterface = drvHostVaKitAudioQueryInterface;
    /* IHostAudio */
    PDMAUDIO_IHOSTAUDIO_CALLBACKS(drvHostVaKitAudio);

    return VINF_SUCCESS;
}

/**
 * Char driver registration record.
 */
const PDMDRVREG g_DrvHostValidationKitAudio =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "ValidationKitAudio",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "ValidationKitAudio audio host driver",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_AUDIO,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(DRVHOSTVAKITAUDIO),
    /* pfnConstruct */
    drvHostVaKitAudioConstruct,
    /* pfnDestruct */
    NULL,
    /* pfnRelocate */
    NULL,
    /* pfnIOCtl */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    NULL,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnPowerOff */
    NULL,
    /* pfnSoftReset */
    NULL,
    /* u32EndVersion */
    PDM_DRVREG_VERSION
};

