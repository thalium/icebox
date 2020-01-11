/* $Id: DrvHostDebugAudio.cpp $ */
/** @file
 * Debug audio driver.
 *
 * Host backend for dumping and injecting audio data from/to the device emulation.
 */

/*
 * Copyright (C) 2016-2019 Oracle Corporation
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
 * Structure for keeping a debug input/output stream.
 */
typedef struct DEBUGAUDIOSTREAM
{
    /** The stream's acquired configuration. */
    PPDMAUDIOSTREAMCFG pCfg;
    /** Audio file to dump output to or read input from. */
    PPDMAUDIOFILE      pFile;
    union
    {
        struct
        {
            /** Timestamp of last captured samples. */
            uint64_t   tsLastCaptured;
        } In;
    };
} DEBUGAUDIOSTREAM, *PDEBUGAUDIOSTREAM;

/**
 * Debug audio driver instance data.
 * @implements PDMIAUDIOCONNECTOR
 */
typedef struct DRVHOSTDEBUGAUDIO
{
    /** Pointer to the driver instance structure. */
    PPDMDRVINS    pDrvIns;
    /** Pointer to host audio interface. */
    PDMIHOSTAUDIO IHostAudio;
} DRVHOSTDEBUGAUDIO, *PDRVHOSTDEBUGAUDIO;

/*******************************************PDM_AUDIO_DRIVER******************************/


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetConfig}
 */
static DECLCALLBACK(int) drvHostDebugAudioGetConfig(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDCFG pBackendCfg)
{
    RT_NOREF(pInterface);
    AssertPtrReturn(pBackendCfg, VERR_INVALID_POINTER);

    RTStrPrintf2(pBackendCfg->szName, sizeof(pBackendCfg->szName), "Debug audio driver");

    pBackendCfg->cbStreamOut    = sizeof(DEBUGAUDIOSTREAM);
    pBackendCfg->cbStreamIn     = sizeof(DEBUGAUDIOSTREAM);

    pBackendCfg->cMaxStreamsOut = 1; /* Output; writing to a file. */
    pBackendCfg->cMaxStreamsIn  = 0; /** @todo Right now we don't support any input (capturing, injecting from a file). */

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnInit}
 */
static DECLCALLBACK(int) drvHostDebugAudioInit(PPDMIHOSTAUDIO pInterface)
{
    RT_NOREF(pInterface);

    LogFlowFuncLeaveRC(VINF_SUCCESS);
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnShutdown}
 */
static DECLCALLBACK(void) drvHostDebugAudioShutdown(PPDMIHOSTAUDIO pInterface)
{
    RT_NOREF(pInterface);
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetStatus}
 */
static DECLCALLBACK(PDMAUDIOBACKENDSTS) drvHostDebugAudioGetStatus(PPDMIHOSTAUDIO pInterface, PDMAUDIODIR enmDir)
{
    RT_NOREF(enmDir);
    AssertPtrReturn(pInterface, PDMAUDIOBACKENDSTS_UNKNOWN);

    return PDMAUDIOBACKENDSTS_RUNNING;
}


static int debugCreateStreamIn(PDRVHOSTDEBUGAUDIO pDrv, PDEBUGAUDIOSTREAM pStreamDbg,
                               PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    RT_NOREF(pDrv, pStreamDbg, pCfgReq, pCfgAcq);

    return VINF_SUCCESS;
}


static int debugCreateStreamOut(PDRVHOSTDEBUGAUDIO pDrv, PDEBUGAUDIOSTREAM pStreamDbg,
                                PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    RT_NOREF(pDrv, pCfgAcq);

    char szTemp[RTPATH_MAX];
    int rc = RTPathTemp(szTemp, sizeof(szTemp));
    if (RT_SUCCESS(rc))
    {
        char szFile[RTPATH_MAX];
        rc = DrvAudioHlpFileNameGet(szFile, RT_ELEMENTS(szFile), szTemp, "DebugAudioOut",
                                    pDrv->pDrvIns->iInstance, PDMAUDIOFILETYPE_WAV, PDMAUDIOFILENAME_FLAG_NONE);
        if (RT_SUCCESS(rc))
        {
            rc = DrvAudioHlpFileCreate(PDMAUDIOFILETYPE_WAV, szFile, PDMAUDIOFILE_FLAG_NONE, &pStreamDbg->pFile);
            if (RT_SUCCESS(rc))
            {
                rc = DrvAudioHlpFileOpen(pStreamDbg->pFile, RTFILE_O_WRITE | RTFILE_O_DENY_WRITE | RTFILE_O_CREATE_REPLACE,
                                         &pCfgReq->Props);
            }

            if (RT_FAILURE(rc))
                LogRel(("DebugAudio: Creating output file '%s' failed with %Rrc\n", szFile, rc));
        }
        else
            LogRel(("DebugAudio: Unable to build file name for temp dir '%s': %Rrc\n", szTemp, rc));
    }
    else
        LogRel(("DebugAudio: Unable to retrieve temp dir: %Rrc\n", rc));

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamCreate}
 */
static DECLCALLBACK(int) drvHostDebugAudioStreamCreate(PPDMIHOSTAUDIO pInterface,
                                                       PPDMAUDIOBACKENDSTREAM pStream,
                                                       PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgReq,    VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgAcq,    VERR_INVALID_POINTER);

    PDRVHOSTDEBUGAUDIO pDrv       = RT_FROM_MEMBER(pInterface, DRVHOSTDEBUGAUDIO, IHostAudio);
    PDEBUGAUDIOSTREAM  pStreamDbg = (PDEBUGAUDIOSTREAM)pStream;

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
static DECLCALLBACK(int) drvHostDebugAudioStreamPlay(PPDMIHOSTAUDIO pInterface,
                                                     PPDMAUDIOBACKENDSTREAM pStream, const void *pvBuf, uint32_t cxBuf,
                                                     uint32_t *pcxWritten)
{
    RT_NOREF(pInterface);
    PDEBUGAUDIOSTREAM  pStreamDbg = (PDEBUGAUDIOSTREAM)pStream;

    int rc = DrvAudioHlpFileWrite(pStreamDbg->pFile, pvBuf, cxBuf, 0 /* fFlags */);
    if (RT_FAILURE(rc))
    {
        LogRel(("DebugAudio: Writing output failed with %Rrc\n", rc));
        return rc;
    }

    if (pcxWritten)
        *pcxWritten = cxBuf;

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamCapture}
 */
static DECLCALLBACK(int) drvHostDebugAudioStreamCapture(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream,
                                                        void *pvBuf, uint32_t cxBuf, uint32_t *pcxRead)
{
    RT_NOREF(pInterface, pStream, pvBuf, cxBuf);

    /* Never capture anything. */
    if (pcxRead)
        *pcxRead = 0;

    return VINF_SUCCESS;
}


static int debugDestroyStreamIn(PDRVHOSTDEBUGAUDIO pDrv, PDEBUGAUDIOSTREAM pStreamDbg)
{
    RT_NOREF(pDrv, pStreamDbg);
    return VINF_SUCCESS;
}


static int debugDestroyStreamOut(PDRVHOSTDEBUGAUDIO pDrv, PDEBUGAUDIOSTREAM pStreamDbg)
{
    RT_NOREF(pDrv);

    DrvAudioHlpFileDestroy(pStreamDbg->pFile);

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamDestroy}
 */
static DECLCALLBACK(int) drvHostDebugAudioStreamDestroy(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);

    PDRVHOSTDEBUGAUDIO pDrv       = RT_FROM_MEMBER(pInterface, DRVHOSTDEBUGAUDIO, IHostAudio);
    PDEBUGAUDIOSTREAM  pStreamDbg = (PDEBUGAUDIOSTREAM)pStream;

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


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamControl}
 */
static DECLCALLBACK(int) drvHostDebugAudioStreamControl(PPDMIHOSTAUDIO pInterface,
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
static DECLCALLBACK(uint32_t) drvHostDebugAudioStreamGetReadable(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface, pStream);

    return 0; /* Never capture anything. */
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetWritable}
 */
static DECLCALLBACK(uint32_t) drvHostDebugAudioStreamGetWritable(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface, pStream);

    return UINT32_MAX;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetWritable}
 */
static DECLCALLBACK(PDMAUDIOSTREAMSTS) drvHostDebugAudioStreamGetStatus(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface, pStream);

    return (PDMAUDIOSTREAMSTS_FLAG_INITIALIZED | PDMAUDIOSTREAMSTS_FLAG_ENABLED);
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamIterate}
 */
static DECLCALLBACK(int) drvHostDebugAudioStreamIterate(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface, pStream);
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) drvHostDebugAudioQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS         pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVHOSTDEBUGAUDIO pThis   = PDMINS_2_DATA(pDrvIns, PDRVHOSTDEBUGAUDIO);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIHOSTAUDIO, &pThis->IHostAudio);
    return NULL;
}


/**
 * Constructs a Null audio driver instance.
 *
 * @copydoc FNPDMDRVCONSTRUCT
 */
static DECLCALLBACK(int) drvHostDebugAudioConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    RT_NOREF(pCfg, fFlags);
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);
    PDRVHOSTDEBUGAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTDEBUGAUDIO);
    LogRel(("Audio: Initializing DEBUG driver\n"));

    /*
     * Init the static parts.
     */
    pThis->pDrvIns                   = pDrvIns;
    /* IBase */
    pDrvIns->IBase.pfnQueryInterface = drvHostDebugAudioQueryInterface;
    /* IHostAudio */
    PDMAUDIO_IHOSTAUDIO_CALLBACKS(drvHostDebugAudio);

#ifdef VBOX_AUDIO_DEBUG_DUMP_PCM_DATA
    RTFileDelete(VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "AudioDebugOutput.pcm");
#endif

    return VINF_SUCCESS;
}

/**
 * Char driver registration record.
 */
const PDMDRVREG g_DrvHostDebugAudio =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "DebugAudio",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "Debug audio host driver",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_AUDIO,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(DRVHOSTDEBUGAUDIO),
    /* pfnConstruct */
    drvHostDebugAudioConstruct,
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

