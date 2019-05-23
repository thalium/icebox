/* $Id: DrvAudioVRDE.cpp $ */
/** @file
 * VRDE audio backend for Main.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_DRV_HOST_AUDIO
#include "LoggingNew.h"

#include <VBox/log.h>
#include "DrvAudioVRDE.h"
#include "ConsoleImpl.h"
#include "ConsoleVRDPServer.h"

#include "../../Devices/Audio/DrvAudio.h"

#include <iprt/mem.h>
#include <iprt/cdefs.h>
#include <iprt/circbuf.h>

#include <VBox/vmm/pdmaudioifs.h>
#include <VBox/vmm/pdmdrv.h>
#include <VBox/RemoteDesktop/VRDE.h>
#include <VBox/vmm/cfgm.h>
#include <VBox/err.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Audio VRDE driver instance data.
 */
typedef struct DRVAUDIOVRDE
{
    /** Pointer to audio VRDE object. */
    AudioVRDE           *pAudioVRDE;
    /** Pointer to the driver instance structure. */
    PPDMDRVINS           pDrvIns;
    /** Pointer to host audio interface. */
    PDMIHOSTAUDIO        IHostAudio;
    /** Pointer to the VRDP's console object. */
    ConsoleVRDPServer   *pConsoleVRDPServer;
    /** Pointer to the DrvAudio port interface that is above us. */
    PPDMIAUDIOCONNECTOR  pDrvAudio;
} DRVAUDIOVRDE, *PDRVAUDIOVRDE;

typedef struct VRDESTREAM
{
    /** The stream's acquired configuration. */
    PPDMAUDIOSTREAMCFG pCfg;
    union
    {
        struct
        {
            /** Number of audio frames this stream can handle at once. */
            uint32_t    cfMax;
            /** Circular buffer for holding the recorded audio frames from the host. */
            PRTCIRCBUF  pCircBuf;
        } In;
        struct
        {
            /** Timestamp (in virtual time ticks) of the last audio playback (of the VRDP server). */
            uint64_t    ticksPlayedLast;
            /** Internal counter (in audio frames) to track if and how much we can write to the VRDP server
             *  for the current time period. */
            uint64_t    cfToWrite;
        } Out;
    };
} VRDESTREAM, *PVRDESTREAM;

/* Sanity. */
AssertCompileSize(PDMAUDIOFRAME, sizeof(int64_t) * 2 /* st_sample_t using by VRDP server */);

static int vrdeCreateStreamIn(PVRDESTREAM pStreamVRDE, PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    pStreamVRDE->In.cfMax = _1K; /** @todo Make this configurable. */

    int rc = RTCircBufCreate(&pStreamVRDE->In.pCircBuf, pStreamVRDE->In.cfMax * (pCfgReq->Props.cBits / 8) /* Bytes */);
    if (RT_SUCCESS(rc))
    {
        if (pCfgAcq)
        {
            /*
             * Because of historical reasons the VRDP server operates on st_sample_t structures internally,
             * which is 2 * int64_t for left/right (stereo) channels.
             *
             * As the audio connector also uses this format, set the layout to "raw" and just let pass through
             * the data without any layout modification needed.
             */
            pCfgAcq->enmLayout        = PDMAUDIOSTREAMLAYOUT_RAW;
            pCfgAcq->cFrameBufferHint = pStreamVRDE->In.cfMax;
        }
    }

    return rc;
}


static int vrdeCreateStreamOut(PVRDESTREAM pStreamVRDE, PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    RT_NOREF(pStreamVRDE, pCfgReq);

    if (pCfgAcq)
    {
        /*
         * Because of historical reasons the VRDP server operates on st_sample_t structures internally,
         * which is 2 * int64_t for left/right (stereo) channels.
         *
         * As the audio connector also uses this format, set the layout to "raw" and just let pass through
         * the data without any layout modification needed.
         */
        pCfgAcq->enmLayout         = PDMAUDIOSTREAMLAYOUT_RAW;
        pCfgAcq->cFrameBufferHint = _4K; /** @todo Make this configurable. */
    }

    return VINF_SUCCESS;
}


static int vrdeControlStreamOut(PDRVAUDIOVRDE pDrv, PVRDESTREAM pStreamVRDE, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    RT_NOREF(pDrv, pStreamVRDE, enmStreamCmd);

    LogFlowFunc(("enmStreamCmd=%ld\n", enmStreamCmd));

    return VINF_SUCCESS;
}


static int vrdeControlStreamIn(PDRVAUDIOVRDE pDrv, PVRDESTREAM pStreamVRDE, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    LogFlowFunc(("enmStreamCmd=%ld\n", enmStreamCmd));

    if (!pDrv->pConsoleVRDPServer)
        return VINF_SUCCESS;

    int rc;

    /* Initialize only if not already done. */
    switch (enmStreamCmd)
    {
        case PDMAUDIOSTREAMCMD_ENABLE:
        {
            rc = pDrv->pConsoleVRDPServer->SendAudioInputBegin(NULL, pStreamVRDE, pStreamVRDE->In.cfMax,
                                                               pStreamVRDE->pCfg->Props.uHz, pStreamVRDE->pCfg->Props.cChannels,
                                                               pStreamVRDE->pCfg->Props.cBits);
            if (rc == VERR_NOT_SUPPORTED)
            {
                LogFunc(("No RDP client connected, so no input recording supported\n"));
                rc = VINF_SUCCESS;
            }

            break;
        }

        case PDMAUDIOSTREAMCMD_DISABLE:
        {
            pDrv->pConsoleVRDPServer->SendAudioInputEnd(NULL /* pvUserCtx */);
            rc = VINF_SUCCESS;

            break;
        }

        case PDMAUDIOSTREAMCMD_PAUSE:
        {
            rc = VINF_SUCCESS;
            break;
        }

        case PDMAUDIOSTREAMCMD_RESUME:
        {
            rc = VINF_SUCCESS;
            break;
        }

        default:
        {
            rc = VERR_NOT_SUPPORTED;
            break;
        }
    }

    if (RT_FAILURE(rc))
        LogFunc(("Failed with %Rrc\n", rc));

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnInit}
 */
static DECLCALLBACK(int) drvAudioVRDEInit(PPDMIHOSTAUDIO pInterface)
{
    RT_NOREF(pInterface);
    LogFlowFuncEnter();

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamCapture}
 */
static DECLCALLBACK(int) drvAudioVRDEStreamCapture(PPDMIHOSTAUDIO pInterface,
                                                   PPDMAUDIOBACKENDSTREAM pStream, void *pvBuf, uint32_t cxBuf, uint32_t *pcxRead)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    AssertPtrReturn(pvBuf,      VERR_INVALID_POINTER);
    AssertReturn(cxBuf,         VERR_INVALID_PARAMETER);
    /* pcxRead is optional. */

    PVRDESTREAM pStreamVRDE = (PVRDESTREAM)pStream;

    size_t cbData = 0;

    if (RTCircBufUsed(pStreamVRDE->In.pCircBuf))
    {
        void *pvData;

        RTCircBufAcquireReadBlock(pStreamVRDE->In.pCircBuf, cxBuf, &pvData, &cbData);

        if (cbData)
            memcpy(pvBuf, pvData, cbData);

        RTCircBufReleaseReadBlock(pStreamVRDE->In.pCircBuf, cbData);
    }

    if (pcxRead)
        *pcxRead = (uint32_t)cbData;

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamPlay}
 */
static DECLCALLBACK(int) drvAudioVRDEStreamPlay(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream,
                                                const void *pvBuf, uint32_t cxBuf, uint32_t *pcxWritten)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    AssertPtrReturn(pvBuf,      VERR_INVALID_POINTER);
    AssertReturn(cxBuf,         VERR_INVALID_PARAMETER);
    /* pcxWritten is optional. */

    PDRVAUDIOVRDE pDrv        = RT_FROM_MEMBER(pInterface, DRVAUDIOVRDE, IHostAudio);
    PVRDESTREAM   pStreamVRDE = (PVRDESTREAM)pStream;

    if (!pDrv->pConsoleVRDPServer)
        return VERR_NOT_AVAILABLE;

    /* Note: We get the number of *frames* in cxBuf
     *       (since we specified PDMAUDIOSTREAMLAYOUT_RAW as the audio data layout) on stream creation. */
    uint32_t cfLive           = cxBuf;

    PPDMAUDIOPCMPROPS pProps  = &pStreamVRDE->pCfg->Props;

    VRDEAUDIOFORMAT format = VRDE_AUDIO_FMT_MAKE(pProps->uHz,
                                                 pProps->cChannels,
                                                 pProps->cBits,
                                                 pProps->fSigned);

    /* Use the internal counter to track if we (still) can write to the VRDP server
     * or if we need to wait another round (time slot). */
    uint32_t cfToWrite = pStreamVRDE->Out.cfToWrite;

    Log3Func(("cfLive=%RU32, cfToWrite=%RU32\n", cfLive, cfToWrite));

    /* Don't play more than available. */
    if (cfToWrite > cfLive)
        cfToWrite = cfLive;

    int rc = VINF_SUCCESS;

    PPDMAUDIOFRAME paSampleBuf = (PPDMAUDIOFRAME)pvBuf;
    AssertPtr(paSampleBuf);

    /*
     * Call the VRDP server with the data.
     */
    uint32_t cfWritten = 0;
    while (cfToWrite)
    {
        uint32_t cfChunk = cfToWrite; /** @todo For now write all at once. */

        if (!cfChunk) /* Nothing to send. Bail out. */
            break;

        /* Note: The VRDP server expects int64_t samples per channel, regardless of the actual
         *       sample bits (e.g 8 or 16 bits). */
        pDrv->pConsoleVRDPServer->SendAudioSamples(paSampleBuf + cfWritten, cfChunk /* Frames */, format);

        cfWritten += cfChunk;
        Assert(cfWritten <= cfLive);

        Assert(cfToWrite >= cfChunk);
        cfToWrite -= cfChunk;
    }

    if (RT_SUCCESS(rc))
    {
        /* Subtract written frames from the counter. */
        Assert(pStreamVRDE->Out.cfToWrite >= cfWritten);
        pStreamVRDE->Out.cfToWrite      -= cfWritten;

        /* Remember when frames were consumed. */
        pStreamVRDE->Out.ticksPlayedLast = PDMDrvHlpTMGetVirtualTime(pDrv->pDrvIns);

        /* Return frames instead of bytes here
         * (since we specified PDMAUDIOSTREAMLAYOUT_RAW as the audio data layout). */
        if (pcxWritten)
            *pcxWritten = cfWritten;
    }

    return rc;
}


static int vrdeDestroyStreamIn(PDRVAUDIOVRDE pDrv, PVRDESTREAM pStreamVRDE)
{
    if (pDrv->pConsoleVRDPServer)
        pDrv->pConsoleVRDPServer->SendAudioInputEnd(NULL);

    if (pStreamVRDE->In.pCircBuf)
    {
        RTCircBufDestroy(pStreamVRDE->In.pCircBuf);
        pStreamVRDE->In.pCircBuf = NULL;
    }

    return VINF_SUCCESS;
}


static int vrdeDestroyStreamOut(PDRVAUDIOVRDE pDrv, PVRDESTREAM pStreamVRDE)
{
    RT_NOREF(pDrv, pStreamVRDE);

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetConfig}
 */
static DECLCALLBACK(int) drvAudioVRDEGetConfig(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDCFG pBackendCfg)
{
    RT_NOREF(pInterface);
    AssertPtrReturn(pBackendCfg, VERR_INVALID_POINTER);

    pBackendCfg->cbStreamOut    = sizeof(VRDESTREAM);
    pBackendCfg->cbStreamIn     = sizeof(VRDESTREAM);
    pBackendCfg->cMaxStreamsIn  = UINT32_MAX;
    pBackendCfg->cMaxStreamsOut = UINT32_MAX;

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnShutdown}
 */
static DECLCALLBACK(void) drvAudioVRDEShutdown(PPDMIHOSTAUDIO pInterface)
{
    PDRVAUDIOVRDE pDrv = RT_FROM_MEMBER(pInterface, DRVAUDIOVRDE, IHostAudio);
    AssertPtrReturnVoid(pDrv);

    if (pDrv->pConsoleVRDPServer)
        pDrv->pConsoleVRDPServer->SendAudioInputEnd(NULL);
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetStatus}
 */
static DECLCALLBACK(PDMAUDIOBACKENDSTS) drvAudioVRDEGetStatus(PPDMIHOSTAUDIO pInterface, PDMAUDIODIR enmDir)
{
    RT_NOREF(enmDir);
    AssertPtrReturn(pInterface, PDMAUDIOBACKENDSTS_UNKNOWN);

    return PDMAUDIOBACKENDSTS_RUNNING;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamCreate}
 */
static DECLCALLBACK(int) drvAudioVRDEStreamCreate(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream,
                                                  PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgReq,    VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgAcq,    VERR_INVALID_POINTER);

    RT_NOREF(pInterface);

    PVRDESTREAM pStreamVRDE = (PVRDESTREAM)pStream;

    int rc;
    if (pCfgReq->enmDir == PDMAUDIODIR_IN)
        rc = vrdeCreateStreamIn (pStreamVRDE, pCfgReq, pCfgAcq);
    else
        rc = vrdeCreateStreamOut(pStreamVRDE, pCfgReq, pCfgAcq);

    if (RT_SUCCESS(rc))
    {
        pStreamVRDE->pCfg = DrvAudioHlpStreamCfgDup(pCfgAcq);
        if (!pStreamVRDE->pCfg)
            rc = VERR_NO_MEMORY;
    }

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamDestroy}
 */
static DECLCALLBACK(int) drvAudioVRDEStreamDestroy(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    PDRVAUDIOVRDE pDrv        = RT_FROM_MEMBER(pInterface, DRVAUDIOVRDE, IHostAudio);
    PVRDESTREAM   pStreamVRDE = (PVRDESTREAM)pStream;

    if (!pStreamVRDE->pCfg) /* Not (yet) configured? Skip. */
        return VINF_SUCCESS;

    int rc;
    if (pStreamVRDE->pCfg->enmDir == PDMAUDIODIR_IN)
        rc = vrdeDestroyStreamIn(pDrv, pStreamVRDE);
    else
        rc = vrdeDestroyStreamOut(pDrv, pStreamVRDE);

    if (RT_SUCCESS(rc))
    {
        DrvAudioHlpStreamCfgFree(pStreamVRDE->pCfg);
        pStreamVRDE->pCfg = NULL;
    }

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamControl}
 */
static DECLCALLBACK(int) drvAudioVRDEStreamControl(PPDMIHOSTAUDIO pInterface,
                                                   PPDMAUDIOBACKENDSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    PDRVAUDIOVRDE pDrv        = RT_FROM_MEMBER(pInterface, DRVAUDIOVRDE, IHostAudio);
    PVRDESTREAM   pStreamVRDE = (PVRDESTREAM)pStream;

    if (!pStreamVRDE->pCfg) /* Not (yet) configured? Skip. */
        return VINF_SUCCESS;

    int rc;
    if (pStreamVRDE->pCfg->enmDir == PDMAUDIODIR_IN)
        rc = vrdeControlStreamIn(pDrv, pStreamVRDE, enmStreamCmd);
    else
        rc = vrdeControlStreamOut(pDrv, pStreamVRDE, enmStreamCmd);

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetReadable}
 */
static DECLCALLBACK(uint32_t) drvAudioVRDEStreamGetReadable(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface);

    PVRDESTREAM pStreamVRDE = (PVRDESTREAM)pStream;

    if (pStreamVRDE->pCfg->enmDir == PDMAUDIODIR_IN)
    {
        /* Return frames instead of bytes here
         * (since we specified PDMAUDIOSTREAMLAYOUT_RAW as the audio data layout). */
        return (uint32_t)PDMAUDIOSTREAMCFG_B2F(pStreamVRDE->pCfg, RTCircBufUsed(pStreamVRDE->In.pCircBuf));
    }

    return 0;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetWritable}
 */
static DECLCALLBACK(uint32_t) drvAudioVRDEStreamGetWritable(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    PDRVAUDIOVRDE pDrv        = RT_FROM_MEMBER(pInterface, DRVAUDIOVRDE, IHostAudio);
    PVRDESTREAM   pStreamVRDE = (PVRDESTREAM)pStream;

    const uint64_t ticksNow     = PDMDrvHlpTMGetVirtualTime(pDrv->pDrvIns);
    const uint64_t ticksElapsed = ticksNow  - pStreamVRDE->Out.ticksPlayedLast;
    const uint64_t ticksPerSec  = PDMDrvHlpTMGetVirtualFreq(pDrv->pDrvIns);

    PPDMAUDIOPCMPROPS pProps  = &pStreamVRDE->pCfg->Props;

    /* Minimize the rounding error: frames = int((ticks * freq) / ticks_per_second + 0.5). */
    pStreamVRDE->Out.cfToWrite = (int)((2 * ticksElapsed * pProps->uHz + ticksPerSec) / ticksPerSec / 2);

    /* Return frames instead of bytes here
     * (since we specified PDMAUDIOSTREAMLAYOUT_RAW as the audio data layout). */
    return pStreamVRDE->Out.cfToWrite;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetStatus}
 */
static DECLCALLBACK(PDMAUDIOSTREAMSTS) drvAudioVRDEStreamGetStatus(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface, pStream);

    return (PDMAUDIOSTREAMSTS_FLAG_INITIALIZED | PDMAUDIOSTREAMSTS_FLAG_ENABLED);
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamIterate}
 */
static DECLCALLBACK(int) drvAudioVRDEStreamIterate(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    /* Nothing to do here for VRDE. */
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) drvAudioVRDEQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVAUDIOVRDE pThis = PDMINS_2_DATA(pDrvIns, PDRVAUDIOVRDE);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIHOSTAUDIO, &pThis->IHostAudio);
    return NULL;
}


AudioVRDE::AudioVRDE(Console *pConsole)
    : mpDrv(NULL),
      mParent(pConsole)
{
}


AudioVRDE::~AudioVRDE(void)
{
    if (mpDrv)
    {
        mpDrv->pAudioVRDE = NULL;
        mpDrv = NULL;
    }
}


int AudioVRDE::onVRDEControl(bool fEnable, uint32_t uFlags)
{
    RT_NOREF(fEnable, uFlags);
    LogFlowThisFunc(("fEnable=%RTbool, uFlags=0x%x\n", fEnable, uFlags));

    if (mpDrv == NULL)
        return VERR_INVALID_STATE;

    return VINF_SUCCESS; /* Never veto. */
}


/**
 * Marks the beginning of sending captured audio data from a connected
 * RDP client.
 *
 * @return  IPRT status code.
 * @param   pvContext               The context; in this case a pointer to a
 *                                  VRDESTREAMIN structure.
 * @param   pVRDEAudioBegin         Pointer to a VRDEAUDIOINBEGIN structure.
 */
int AudioVRDE::onVRDEInputBegin(void *pvContext, PVRDEAUDIOINBEGIN pVRDEAudioBegin)
{
    AssertPtrReturn(pvContext, VERR_INVALID_POINTER);
    AssertPtrReturn(pVRDEAudioBegin, VERR_INVALID_POINTER);

    PVRDESTREAM pVRDEStrmIn = (PVRDESTREAM)pvContext;
    AssertPtrReturn(pVRDEStrmIn, VERR_INVALID_POINTER);

    VRDEAUDIOFORMAT audioFmt = pVRDEAudioBegin->fmt;

    int iSampleHz  = VRDE_AUDIO_FMT_SAMPLE_FREQ(audioFmt);     RT_NOREF(iSampleHz);
    int cChannels  = VRDE_AUDIO_FMT_CHANNELS(audioFmt);        RT_NOREF(cChannels);
    int cBits      = VRDE_AUDIO_FMT_BITS_PER_SAMPLE(audioFmt); RT_NOREF(cBits);
    bool fUnsigned = VRDE_AUDIO_FMT_SIGNED(audioFmt);          RT_NOREF(fUnsigned);

    LogFlowFunc(("cbSample=%RU32, iSampleHz=%d, cChannels=%d, cBits=%d, fUnsigned=%RTbool\n",
                 VRDE_AUDIO_FMT_BYTES_PER_SAMPLE(audioFmt), iSampleHz, cChannels, cBits, fUnsigned));

    return VINF_SUCCESS;
}


int AudioVRDE::onVRDEInputData(void *pvContext, const void *pvData, uint32_t cbData)
{
    PVRDESTREAM pStreamVRDE = (PVRDESTREAM)pvContext;
    AssertPtrReturn(pStreamVRDE, VERR_INVALID_POINTER);

    void  *pvBuf;
    size_t cbBuf;

    RTCircBufAcquireWriteBlock(pStreamVRDE->In.pCircBuf, cbData, &pvBuf, &cbBuf);

    if (cbBuf)
        memcpy(pvBuf, pvData, cbBuf);

    RTCircBufReleaseWriteBlock(pStreamVRDE->In.pCircBuf, cbBuf);

    if (cbBuf < cbData)
        LogRel(("VRDE: Capturing audio data lost %zu bytes\n", cbData - cbBuf)); /** @todo Use an error counter. */

    return VINF_SUCCESS; /** @todo r=andy How to tell the caller if we were not able to handle *all* input data? */
}


int AudioVRDE::onVRDEInputEnd(void *pvContext)
{
    RT_NOREF(pvContext);

    return VINF_SUCCESS;
}


int AudioVRDE::onVRDEInputIntercept(bool fEnabled)
{
    RT_NOREF(fEnabled);
    return VINF_SUCCESS; /* Never veto. */
}


/**
 * Construct a VRDE audio driver instance.
 *
 * @copydoc FNPDMDRVCONSTRUCT
 */
/* static */
DECLCALLBACK(int) AudioVRDE::drvConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    RT_NOREF(fFlags);

    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);
    PDRVAUDIOVRDE pThis = PDMINS_2_DATA(pDrvIns, PDRVAUDIOVRDE);

    AssertPtrReturn(pDrvIns, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfg, VERR_INVALID_POINTER);

    LogRel(("Audio: Initializing VRDE driver\n"));
    LogFlowFunc(("fFlags=0x%x\n", fFlags));

    AssertMsgReturn(PDMDrvHlpNoAttach(pDrvIns) == VERR_PDM_NO_ATTACHED_DRIVER,
                    ("Configuration error: Not possible to attach anything to this driver!\n"),
                    VERR_PDM_DRVINS_NO_ATTACH);

    /*
     * Init the static parts.
     */
    pThis->pDrvIns                   = pDrvIns;
    /* IBase */
    pDrvIns->IBase.pfnQueryInterface = drvAudioVRDEQueryInterface;
    /* IHostAudio */
    PDMAUDIO_IHOSTAUDIO_CALLBACKS(drvAudioVRDE);

    /*
     * Get the ConsoleVRDPServer object pointer.
     */
    void *pvUser;
    int rc = CFGMR3QueryPtr(pCfg, "ObjectVRDPServer", &pvUser); /** @todo r=andy Get rid of this hack and use IHostAudio::SetCallback. */
    AssertMsgRCReturn(rc, ("Confguration error: No/bad \"ObjectVRDPServer\" value, rc=%Rrc\n", rc), rc);

    /* CFGM tree saves the pointer to ConsoleVRDPServer in the Object node of AudioVRDE. */
    pThis->pConsoleVRDPServer = (ConsoleVRDPServer *)pvUser;

    /*
     * Get the AudioVRDE object pointer.
     */
    pvUser = NULL;
    rc = CFGMR3QueryPtr(pCfg, "Object", &pvUser); /** @todo r=andy Get rid of this hack and use IHostAudio::SetCallback. */
    AssertMsgRCReturn(rc, ("Confguration error: No/bad \"Object\" value, rc=%Rrc\n", rc), rc);

    pThis->pAudioVRDE = (AudioVRDE *)pvUser;
    pThis->pAudioVRDE->mpDrv = pThis;

    /*
     * Get the interface for the above driver (DrvAudio) to make mixer/conversion calls.
     * Described in CFGM tree.
     */
    pThis->pDrvAudio = PDMIBASE_QUERY_INTERFACE(pDrvIns->pUpBase, PDMIAUDIOCONNECTOR);
    AssertMsgReturn(pThis->pDrvAudio, ("Configuration error: No upper interface specified!\n"), VERR_PDM_MISSING_INTERFACE_ABOVE);

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDRVREG,pfnDestruct}
 */
/* static */
DECLCALLBACK(void) AudioVRDE::drvDestruct(PPDMDRVINS pDrvIns)
{
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);
    PDRVAUDIOVRDE pThis = PDMINS_2_DATA(pDrvIns, PDRVAUDIOVRDE);
    LogFlowFuncEnter();

    /*
     * If the AudioVRDE object is still alive, we must clear it's reference to
     * us since we'll be invalid when we return from this method.
     */
    if (pThis->pAudioVRDE)
    {
        pThis->pAudioVRDE->mpDrv = NULL;
        pThis->pAudioVRDE = NULL;
    }
}


/**
 * VRDE audio driver registration record.
 */
const PDMDRVREG AudioVRDE::DrvReg =
{
    PDM_DRVREG_VERSION,
    /* szName */
    "AudioVRDE",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "Audio driver for VRDE backend",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_AUDIO,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(DRVAUDIOVRDE),
    /* pfnConstruct */
    AudioVRDE::drvConstruct,
    /* pfnDestruct */
    AudioVRDE::drvDestruct,
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

