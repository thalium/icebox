/* $Id: AudioMixer.cpp $ */
/** @file
 * Audio mixing routines for multiplexing audio sources in device emulations.
 *
 * This mixer acts as a layer between the audio connector interface and
 * the actual device emulation, providing mechanisms for audio sources (input)
 * and audio sinks (output).
 *
 * Think of this mixer as kind of a high(er) level interface for the audio
 * connector interface, abstracting common tasks such as creating and managing
 * various audio sources and sinks. This mixer class is purely optional and can
 * be left out when implementing a new device emulation, using only the audi
 * connector interface instead.  For example, the SB16 emulation does not use
 * this mixer and does all its stream management on its own.
 *
 * As audio driver instances are handled as LUNs on the device level, this
 * audio mixer then can take care of e.g. mixing various inputs/outputs to/from
 * a specific source/sink.
 *
 * How and which audio streams are connected to sinks/sources depends on how
 * the audio mixer has been set up.
 *
 * A sink can connect multiple output streams together, whereas a source
 * does this with input streams. Each sink / source consists of one or more
 * so-called mixer streams, which then in turn have pointers to the actual
 * PDM audio input/output streams.
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_AUDIO_MIXER
#include <VBox/log.h>
#include "AudioMixer.h"
#include "AudioMixBuffer.h"
#include "DrvAudio.h"

#include <VBox/vmm/pdm.h>
#include <VBox/err.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/pdmaudioifs.h>

#include <iprt/alloc.h>
#include <iprt/asm-math.h>
#include <iprt/assert.h>
#include <iprt/string.h>


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int audioMixerRemoveSinkInternal(PAUDIOMIXER pMixer, PAUDMIXSINK pSink);

static void audioMixerSinkDestroyInternal(PAUDMIXSINK pSink);
static int audioMixerSinkUpdateVolume(PAUDMIXSINK pSink, const PPDMAUDIOVOLUME pVolMaster);
static void audioMixerSinkRemoveAllStreamsInternal(PAUDMIXSINK pSink);
static int audioMixerSinkRemoveStreamInternal(PAUDMIXSINK pSink, PAUDMIXSTREAM pStream);
static void audioMixerSinkReset(PAUDMIXSINK pSink);
static int audioMixerSinkUpdateInternal(PAUDMIXSINK pSink);

int audioMixerStreamCtlInternal(PAUDMIXSTREAM pMixStream, PDMAUDIOSTREAMCMD enmCmd, uint32_t fCtl);
static void audioMixerStreamDestroyInternal(PAUDMIXSTREAM pStream);


#ifdef LOG_ENABLED
/**
 * Converts a mixer sink status to a string.
 *
 * @returns Stringified mixer sink flags. Must be free'd with RTStrFree().
 *          "NONE" if no flags set.
 * @param   fFlags              Mixer sink flags to convert.
 */
static char *dbgAudioMixerSinkStatusToStr(AUDMIXSINKSTS fStatus)
{
#define APPEND_FLAG_TO_STR(_aFlag)              \
    if (fStatus & AUDMIXSINK_STS_##_aFlag)      \
    {                                           \
        if (pszFlags)                           \
        {                                       \
            rc2 = RTStrAAppend(&pszFlags, " "); \
            if (RT_FAILURE(rc2))                \
                break;                          \
        }                                       \
                                                \
        rc2 = RTStrAAppend(&pszFlags, #_aFlag); \
        if (RT_FAILURE(rc2))                    \
            break;                              \
    }                                           \

    char *pszFlags = NULL;
    int rc2 = VINF_SUCCESS;

    do
    {
        APPEND_FLAG_TO_STR(NONE);
        APPEND_FLAG_TO_STR(RUNNING);
        APPEND_FLAG_TO_STR(PENDING_DISABLE);
        APPEND_FLAG_TO_STR(DIRTY);

    } while (0);

    if (   RT_FAILURE(rc2)
        && pszFlags)
    {
        RTStrFree(pszFlags);
        pszFlags = NULL;
    }

#undef APPEND_FLAG_TO_STR

    return pszFlags;
}
#endif /* DEBUG */

/**
 * Creates an audio sink and attaches it to the given mixer.
 *
 * @returns IPRT status code.
 * @param   pMixer              Mixer to attach created sink to.
 * @param   pszName             Name of the sink to create.
 * @param   enmDir              Direction of the sink to create.
 * @param   ppSink              Pointer which returns the created sink on success.
 */
int AudioMixerCreateSink(PAUDIOMIXER pMixer, const char *pszName, AUDMIXSINKDIR enmDir, PAUDMIXSINK *ppSink)
{
    AssertPtrReturn(pMixer, VERR_INVALID_POINTER);
    AssertPtrReturn(pszName, VERR_INVALID_POINTER);
    /* ppSink is optional. */

    int rc = RTCritSectEnter(&pMixer->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    PAUDMIXSINK pSink = (PAUDMIXSINK)RTMemAllocZ(sizeof(AUDMIXSINK));
    if (pSink)
    {
        pSink->pszName = RTStrDup(pszName);
        if (!pSink->pszName)
            rc = VERR_NO_MEMORY;

        if (RT_SUCCESS(rc))
            rc = RTCritSectInit(&pSink->CritSect);

        if (RT_SUCCESS(rc))
        {
            pSink->pParent  = pMixer;
            pSink->enmDir   = enmDir;
            RTListInit(&pSink->lstStreams);

            /* Set initial volume to max. */
            pSink->Volume.fMuted = false;
            pSink->Volume.uLeft  = PDMAUDIO_VOLUME_MAX;
            pSink->Volume.uRight = PDMAUDIO_VOLUME_MAX;

            /* Ditto for the combined volume. */
            pSink->VolumeCombined.fMuted = false;
            pSink->VolumeCombined.uLeft  = PDMAUDIO_VOLUME_MAX;
            pSink->VolumeCombined.uRight = PDMAUDIO_VOLUME_MAX;

            RTListAppend(&pMixer->lstSinks, &pSink->Node);
            pMixer->cSinks++;

            LogFlowFunc(("pMixer=%p, pSink=%p, cSinks=%RU8\n",
                         pMixer, pSink, pMixer->cSinks));

            if (ppSink)
                *ppSink = pSink;
        }

        if (RT_FAILURE(rc))
        {
            RTCritSectDelete(&pSink->CritSect);

            if (pSink)
            {
                RTMemFree(pSink);
                pSink = NULL;
            }
        }
    }
    else
        rc = VERR_NO_MEMORY;

    int rc2 = RTCritSectLeave(&pMixer->CritSect);
    AssertRC(rc2);

    return rc;
}

/**
 * Creates an audio mixer.
 *
 * @returns IPRT status code.
 * @param   pszName             Name of the audio mixer.
 * @param   fFlags              Creation flags. Not used at the moment and must be 0.
 * @param   ppMixer             Pointer which returns the created mixer object.
 */
int AudioMixerCreate(const char *pszName, uint32_t fFlags, PAUDIOMIXER *ppMixer)
{
    RT_NOREF(fFlags);
    AssertPtrReturn(pszName, VERR_INVALID_POINTER);
    /** @todo Add fFlags validation. */
    AssertPtrReturn(ppMixer, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;

    PAUDIOMIXER pMixer = (PAUDIOMIXER)RTMemAllocZ(sizeof(AUDIOMIXER));
    if (pMixer)
    {
        pMixer->pszName = RTStrDup(pszName);
        if (!pMixer->pszName)
            rc = VERR_NO_MEMORY;

        if (RT_SUCCESS(rc))
            rc = RTCritSectInit(&pMixer->CritSect);

        if (RT_SUCCESS(rc))
        {
            pMixer->cSinks = 0;
            RTListInit(&pMixer->lstSinks);

            /* Set master volume to the max. */
            pMixer->VolMaster.fMuted = false;
            pMixer->VolMaster.uLeft  = PDMAUDIO_VOLUME_MAX;
            pMixer->VolMaster.uRight = PDMAUDIO_VOLUME_MAX;

            LogFlowFunc(("Created mixer '%s'\n", pMixer->pszName));

            *ppMixer = pMixer;
        }
        else
            RTMemFree(pMixer);
    }
    else
        rc = VERR_NO_MEMORY;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Helper function for the internal debugger to print the mixer's current
 * state, along with the attached sinks.
 *
 * @param   pMixer              Mixer to print debug output for.
 * @param   pHlp                Debug info helper to use.
 * @param   pszArgs             Optional arguments. Not being used at the moment.
 */
void AudioMixerDebug(PAUDIOMIXER pMixer, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    RT_NOREF(pszArgs);
    PAUDMIXSINK pSink;
    unsigned    iSink = 0;

    int rc2 = RTCritSectEnter(&pMixer->CritSect);
    if (RT_FAILURE(rc2))
        return;

    pHlp->pfnPrintf(pHlp, "[Master] %s: lVol=%u, rVol=%u, fMuted=%RTbool\n", pMixer->pszName,
                    pMixer->VolMaster.uLeft, pMixer->VolMaster.uRight, pMixer->VolMaster.fMuted);

    RTListForEach(&pMixer->lstSinks, pSink, AUDMIXSINK, Node)
    {
        pHlp->pfnPrintf(pHlp, "[Sink %u] %s: lVol=%u, rVol=%u, fMuted=%RTbool\n", iSink, pSink->pszName,
                        pSink->Volume.uLeft, pSink->Volume.uRight, pSink->Volume.fMuted);
        ++iSink;
    }

    rc2 = RTCritSectLeave(&pMixer->CritSect);
    AssertRC(rc2);
}

/**
 * Destroys an audio mixer.
 *
 * @param   pMixer              Audio mixer to destroy.
 */
void AudioMixerDestroy(PAUDIOMIXER pMixer)
{
    if (!pMixer)
        return;

    int rc2 = RTCritSectEnter(&pMixer->CritSect);
    AssertRC(rc2);

    LogFlowFunc(("Destroying %s ...\n", pMixer->pszName));

    PAUDMIXSINK pSink, pSinkNext;
    RTListForEachSafe(&pMixer->lstSinks, pSink, pSinkNext, AUDMIXSINK, Node)
    {
        /* Save a pointer to the sink to remove, as pSink
         * will not be valid anymore after calling audioMixerRemoveSinkInternal(). */
        PAUDMIXSINK pSinkToRemove = pSink;

        audioMixerRemoveSinkInternal(pMixer, pSinkToRemove);
        audioMixerSinkDestroyInternal(pSinkToRemove);
    }

    pMixer->cSinks = 0;

    if (pMixer->pszName)
    {
        RTStrFree(pMixer->pszName);
        pMixer->pszName = NULL;
    }

    rc2 = RTCritSectLeave(&pMixer->CritSect);
    AssertRC(rc2);

    RTCritSectDelete(&pMixer->CritSect);

    RTMemFree(pMixer);
    pMixer = NULL;
}

/**
 * Invalidates all internal data, internal version.
 *
 * @returns IPRT status code.
 * @param   pMixer              Mixer to invalidate data for.
 */
int audioMixerInvalidateInternal(PAUDIOMIXER pMixer)
{
    AssertPtrReturn(pMixer, VERR_INVALID_POINTER);

    LogFlowFunc(("[%s]\n", pMixer->pszName));

    /* Propagate new master volume to all connected sinks. */
    PAUDMIXSINK pSink;
    RTListForEach(&pMixer->lstSinks, pSink, AUDMIXSINK, Node)
    {
        int rc2 = audioMixerSinkUpdateVolume(pSink, &pMixer->VolMaster);
        AssertRC(rc2);
    }

    return VINF_SUCCESS;
}

/**
 * Invalidates all internal data.
 *
 * @returns IPRT status code.
 * @param   pMixer              Mixer to invalidate data for.
 */
void AudioMixerInvalidate(PAUDIOMIXER pMixer)
{
    AssertPtrReturnVoid(pMixer);

    int rc2 = RTCritSectEnter(&pMixer->CritSect);
    AssertRC(rc2);

    LogFlowFunc(("[%s]\n", pMixer->pszName));

    rc2 = audioMixerInvalidateInternal(pMixer);
    AssertRC(rc2);

    rc2 = RTCritSectLeave(&pMixer->CritSect);
    AssertRC(rc2);
}

/**
 * Removes a formerly attached audio sink for an audio mixer, internal version.
 *
 * @returns IPRT status code.
 * @param   pMixer              Mixer to remove sink from.
 * @param   pSink               Sink to remove.
 */
static int audioMixerRemoveSinkInternal(PAUDIOMIXER pMixer, PAUDMIXSINK pSink)
{
    AssertPtrReturn(pMixer, VERR_INVALID_POINTER);
    if (!pSink)
        return VERR_NOT_FOUND;

    AssertMsgReturn(pSink->pParent == pMixer, ("%s: Is not part of mixer '%s'\n",
                                               pSink->pszName, pMixer->pszName), VERR_NOT_FOUND);

    LogFlowFunc(("[%s] pSink=%s, cSinks=%RU8\n",
                 pMixer->pszName, pSink->pszName, pMixer->cSinks));

    /* Remove sink from mixer. */
    RTListNodeRemove(&pSink->Node);
    Assert(pMixer->cSinks);

    /* Set mixer to NULL so that we know we're not part of any mixer anymore. */
    pSink->pParent = NULL;

    return VINF_SUCCESS;
}

/**
 * Removes a formerly attached audio sink for an audio mixer.
 *
 * @returns IPRT status code.
 * @param   pMixer              Mixer to remove sink from.
 * @param   pSink               Sink to remove.
 */
void AudioMixerRemoveSink(PAUDIOMIXER pMixer, PAUDMIXSINK pSink)
{
    int rc2 = RTCritSectEnter(&pMixer->CritSect);
    AssertRC(rc2);

    audioMixerSinkRemoveAllStreamsInternal(pSink);
    audioMixerRemoveSinkInternal(pMixer, pSink);

    rc2 = RTCritSectLeave(&pMixer->CritSect);
}

/**
 * Sets the mixer's master volume.
 *
 * @returns IPRT status code.
 * @param   pMixer              Mixer to set master volume for.
 * @param   pVol                Volume to set.
 */
int AudioMixerSetMasterVolume(PAUDIOMIXER pMixer, PPDMAUDIOVOLUME pVol)
{
    AssertPtrReturn(pMixer, VERR_INVALID_POINTER);
    AssertPtrReturn(pVol,   VERR_INVALID_POINTER);

    int rc = RTCritSectEnter(&pMixer->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    memcpy(&pMixer->VolMaster, pVol, sizeof(PDMAUDIOVOLUME));

    LogFlowFunc(("[%s] lVol=%RU32, rVol=%RU32 => fMuted=%RTbool, lVol=%RU32, rVol=%RU32\n",
                 pMixer->pszName, pVol->uLeft, pVol->uRight,
                 pMixer->VolMaster.fMuted, pMixer->VolMaster.uLeft, pMixer->VolMaster.uRight));

    rc = audioMixerInvalidateInternal(pMixer);

    int rc2 = RTCritSectLeave(&pMixer->CritSect);
    AssertRC(rc2);

    return rc;
}

/*********************************************************************************************************************************
 * Mixer Sink implementation.
 ********************************************************************************************************************************/

/**
 * Adds an audio stream to a specific audio sink.
 *
 * @returns IPRT status code.
 * @param   pSink               Sink to add audio stream to.
 * @param   pStream             Stream to add.
 */
int AudioMixerSinkAddStream(PAUDMIXSINK pSink, PAUDMIXSTREAM pStream)
{
    AssertPtrReturn(pSink,   VERR_INVALID_POINTER);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    int rc = RTCritSectEnter(&pSink->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    if (pSink->cStreams == UINT8_MAX) /* 255 streams per sink max. */
    {
        int rc2 = RTCritSectLeave(&pSink->CritSect);
        AssertRC(rc2);

        return VERR_NO_MORE_HANDLES;
    }

    LogFlowFuncEnter();

#ifdef VBOX_AUDIO_MIXER_WITH_MIXBUF
    /* Make sure only compatible streams are added. */
    if (pStream->enmDir == PDMAUDIODIR_IN)
    {
        if (DrvAudioHlpPCMPropsAreEqual(&pSink->PCMProps, &pStream->InOut.pIn->Props))
        {
#ifdef VBOX_AUDIO_MIXER_WITH_MIXBUF
            /* Chain: Stream (Child) -> Sink (Child) -> Guest (Parent). */
            PPDMAUDIOMIXBUF pHstIn = &pStream->InOut.pIn->pHstStrmIn->MixBuf;
            PPDMAUDIOMIXBUF pGstIn = &pStream->InOut.pIn->MixBuf;

            /* Unlink any former parent from host input. */
            AudioMixBufUnlink(pHstIn);

            /* Link host input to this sink as a parent. */
            rc = AudioMixBufLinkTo(pHstIn, &pSink->MixBuf);
            AssertRC(rc);

            /* Unlink any former parent from this sink. */
            AudioMixBufUnlink(&pSink->MixBuf);

            /* Link guest input to this sink as a parent. */
            rc = AudioMixBufLinkTo(&pSink->MixBuf, pGstIn);
            AssertRC(rc);
# ifdef DEBUG
            AudioMixBufDbgPrintChain(&pStream->InOut.pIn->MixBuf);
# endif
#endif /* VBOX_AUDIO_MIXER_WITH_MIXBUF */
        }
        else
            AssertFailedStmt(rc = VERR_WRONG_TYPE);
    }
    else if (pStream->enmDir == PDMAUDIODIR_OUT)
    {
        if (DrvAudioHlpPCMPropsAreEqual(&pSink->PCMProps, &pStream->InOut.pOut->Props))
        {
#ifdef VBOX_AUDIO_MIXER_WITH_MIXBUF
            /* Chain: Guest (Child) -> Sink (Child) -> Stream (Parent). */
            rc = AudioMixBufLinkTo(&pStream->InOut.pOut->pHstStrmOut->MixBuf, &pSink->MixBuf);
# ifdef DEBUG
            AudioMixBufDbgPrintChain(&pSink->MixBuf);
# endif
#endif /* VBOX_AUDIO_MIXER_WITH_MIXBUF */
        }
        else
            AssertFailedStmt(rc = VERR_WRONG_TYPE);
    }
    else
        AssertFailedStmt(rc = VERR_NOT_IMPLEMENTED);
#else
    rc = VINF_SUCCESS;
#endif

    if (RT_SUCCESS(rc))
    {
        /** @todo Check if stream already is assigned to (another) sink. */

        /* If the sink is running and not in pending disable mode,
         * make sure that the added stream also is enabled. */
        if (    (pSink->fStatus & AUDMIXSINK_STS_RUNNING)
            && !(pSink->fStatus & AUDMIXSINK_STS_PENDING_DISABLE))
        {
            rc = audioMixerStreamCtlInternal(pStream, PDMAUDIOSTREAMCMD_ENABLE, AUDMIXSTRMCTL_FLAG_NONE);
        }

        if (RT_SUCCESS(rc))
        {
            /* Apply the sink's combined volume to the stream. */
            rc = pStream->pConn->pfnStreamSetVolume(pStream->pConn, pStream->pStream, &pSink->VolumeCombined);
            AssertRC(rc);
        }

        if (RT_SUCCESS(rc))
        {
            /* Save pointer to sink the stream is attached to. */
            pStream->pSink = pSink;

            /* Append stream to sink's list. */
            RTListAppend(&pSink->lstStreams, &pStream->Node);
            pSink->cStreams++;
        }
    }

    LogFlowFunc(("[%s] cStreams=%RU8, rc=%Rrc\n", pSink->pszName, pSink->cStreams, rc));

    int rc2 = RTCritSectLeave(&pSink->CritSect);
    AssertRC(rc2);

    return rc;
}

/**
 * Creates an audio mixer stream.
 *
 * @returns IPRT status code.
 * @param   pSink               Sink to use for creating the stream.
 * @param   pConn               Audio connector interface to use.
 * @param   pCfg                Audio stream configuration to use.
 * @param   fFlags              Stream creation flags. Currently unused, set to 0.
 * @param   ppStream            Pointer which receives the newly created audio stream.
 */
int AudioMixerSinkCreateStream(PAUDMIXSINK pSink,
                               PPDMIAUDIOCONNECTOR pConn, PPDMAUDIOSTREAMCFG pCfg, uint32_t fFlags, PAUDMIXSTREAM *ppStream)
{
    AssertPtrReturn(pSink, VERR_INVALID_POINTER);
    AssertPtrReturn(pConn, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfg,  VERR_INVALID_POINTER);
    /** @todo Validate fFlags. */
    /* ppStream is optional. */

    PAUDMIXSTREAM pMixStream = (PAUDMIXSTREAM)RTMemAllocZ(sizeof(AUDMIXSTREAM));
    if (!pMixStream)
        return VERR_NO_MEMORY;

    pMixStream->pszName = RTStrDup(pCfg->szName);
    if (!pMixStream->pszName)
    {
        RTMemFree(pMixStream);
        return VERR_NO_MEMORY;
    }

    int rc = RTCritSectEnter(&pSink->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    LogFlowFunc(("[%s] fFlags=0x%x (enmDir=%ld, %RU8 bits, %RU8 channels, %RU32Hz)\n",
                 pSink->pszName, fFlags, pCfg->enmDir, pCfg->Props.cBits, pCfg->Props.cChannels, pCfg->Props.uHz));

    /*
     * Initialize the host-side configuration for the stream to be created.
     * Always use the sink's PCM audio format as the host side when creating a stream for it.
     */
    PDMAUDIOSTREAMCFG CfgHost;
    rc = DrvAudioHlpPCMPropsToStreamCfg(&pSink->PCMProps, &CfgHost);
    AssertRCReturn(rc, rc);

    /* Apply the sink's direction for the configuration to use to
     * create the stream. */
    if (pSink->enmDir == AUDMIXSINKDIR_INPUT)
    {
        CfgHost.DestSource.Source = pCfg->DestSource.Source;
        CfgHost.enmDir            = PDMAUDIODIR_IN;
        CfgHost.enmLayout         = pCfg->enmLayout;
    }
    else
    {
        CfgHost.DestSource.Dest = pCfg->DestSource.Dest;
        CfgHost.enmDir          = PDMAUDIODIR_OUT;
        CfgHost.enmLayout       = pCfg->enmLayout;
    }

    RTStrPrintf(CfgHost.szName, sizeof(CfgHost.szName), "%s", pCfg->szName);

    rc = RTCritSectInit(&pMixStream->CritSect);
    if (RT_SUCCESS(rc))
    {
        PPDMAUDIOSTREAM pStream;
        rc = pConn->pfnStreamCreate(pConn, &CfgHost, pCfg, &pStream);
        if (RT_SUCCESS(rc))
        {
            /* Save the audio stream pointer to this mixing stream. */
            pMixStream->pStream = pStream;

            /* Increase the stream's reference count to let others know
             * we're reyling on it to be around now. */
            pConn->pfnStreamRetain(pConn, pStream);
        }
    }

    if (RT_SUCCESS(rc))
    {
        pMixStream->fFlags = fFlags;
        pMixStream->pConn  = pConn;

        if (ppStream)
            *ppStream = pMixStream;
    }
    else if (pMixStream)
    {
        int rc2 = RTCritSectDelete(&pMixStream->CritSect);
        AssertRC(rc2);

        if (pMixStream->pszName)
        {
            RTStrFree(pMixStream->pszName);
            pMixStream->pszName = NULL;
        }

        RTMemFree(pMixStream);
        pMixStream = NULL;
    }

    int rc2 = RTCritSectLeave(&pSink->CritSect);
    AssertRC(rc2);

    return rc;
}

/**
 * Static helper function to translate a sink command
 * to a PDM audio stream command.
 *
 * @returns PDM audio stream command, or PDMAUDIOSTREAMCMD_UNKNOWN if not found.
 * @param   enmCmd              Mixer sink command to translate.
 */
static PDMAUDIOSTREAMCMD audioMixerSinkToStreamCmd(AUDMIXSINKCMD enmCmd)
{
    switch (enmCmd)
    {
        case AUDMIXSINKCMD_ENABLE:   return PDMAUDIOSTREAMCMD_ENABLE;
        case AUDMIXSINKCMD_DISABLE:  return PDMAUDIOSTREAMCMD_DISABLE;
        case AUDMIXSINKCMD_PAUSE:    return PDMAUDIOSTREAMCMD_PAUSE;
        case AUDMIXSINKCMD_RESUME:   return PDMAUDIOSTREAMCMD_RESUME;
        default:                     break;
    }

    AssertMsgFailed(("Unsupported sink command %d\n", enmCmd));
    return PDMAUDIOSTREAMCMD_UNKNOWN;
}

/**
 * Controls a mixer sink.
 *
 * @returns IPRT status code.
 * @param   pSink               Mixer sink to control.
 * @param   enmSinkCmd          Sink command to set.
 */
int AudioMixerSinkCtl(PAUDMIXSINK pSink, AUDMIXSINKCMD enmSinkCmd)
{
    AssertPtrReturn(pSink, VERR_INVALID_POINTER);

    PDMAUDIOSTREAMCMD enmCmdStream = audioMixerSinkToStreamCmd(enmSinkCmd);
    if (enmCmdStream == PDMAUDIOSTREAMCMD_UNKNOWN)
        return VERR_NOT_SUPPORTED;

    int rc = RTCritSectEnter(&pSink->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    PAUDMIXSTREAM pStream;
    RTListForEach(&pSink->lstStreams, pStream, AUDMIXSTREAM, Node)
    {
        int rc2 = audioMixerStreamCtlInternal(pStream, enmCmdStream, AUDMIXSTRMCTL_FLAG_NONE);
        if (RT_SUCCESS(rc))
            rc = rc2;
        /* Keep going. Flag? */
    }

    if (enmSinkCmd == AUDMIXSINKCMD_ENABLE)
    {
        /* Make sure to clear any other former flags again by assigning AUDMIXSINK_STS_RUNNING directly. */
        pSink->fStatus = AUDMIXSINK_STS_RUNNING;
    }
    else if (enmSinkCmd == AUDMIXSINKCMD_DISABLE)
    {
        if (pSink->fStatus & AUDMIXSINK_STS_RUNNING)
        {
            /* Set the sink in a pending disable state first.
             * The final status (disabled) will be set in the sink's iteration. */
            pSink->fStatus |= AUDMIXSINK_STS_PENDING_DISABLE;
        }
    }

#ifdef LOG_ENABLED
    char *pszStatus = dbgAudioMixerSinkStatusToStr(pSink->fStatus);
    LogFlowFunc(("[%s] enmCmd=%d, fStatus=%s, rc=%Rrc\n", pSink->pszName, enmSinkCmd, pszStatus, rc));
    RTStrFree(pszStatus);
#endif

    int rc2 = RTCritSectLeave(&pSink->CritSect);
    AssertRC(rc2);

    return rc;
}

/**
 * Destroys a mixer sink and removes it from the attached mixer (if any).
 *
 * @param   pSink               Mixer sink to destroy.
 */
void AudioMixerSinkDestroy(PAUDMIXSINK pSink)
{
    if (!pSink)
        return;

    int rc2 = RTCritSectEnter(&pSink->CritSect);
    AssertRC(rc2);

    if (pSink->pParent)
    {
        /* Save mixer pointer, as after audioMixerRemoveSinkInternal() the
         * pointer will be gone from the stream. */
        PAUDIOMIXER pMixer = pSink->pParent;
        AssertPtr(pMixer);

        audioMixerRemoveSinkInternal(pMixer, pSink);

        Assert(pMixer->cSinks);
        pMixer->cSinks--;
    }

    rc2 = RTCritSectLeave(&pSink->CritSect);
    AssertRC(rc2);

    audioMixerSinkDestroyInternal(pSink);
}

/**
 * Destroys a mixer sink.
 *
 * @param   pSink               Mixer sink to destroy.
 */
static void audioMixerSinkDestroyInternal(PAUDMIXSINK pSink)
{
    AssertPtrReturnVoid(pSink);

    LogFunc(("%s\n", pSink->pszName));

    PAUDMIXSTREAM pStream, pStreamNext;
    RTListForEachSafe(&pSink->lstStreams, pStream, pStreamNext, AUDMIXSTREAM, Node)
    {
        /* Save a pointer to the stream to remove, as pStream
         * will not be valid anymore after calling audioMixerSinkRemoveStreamInternal(). */
        PAUDMIXSTREAM pStreamToRemove = pStream;

        audioMixerSinkRemoveStreamInternal(pSink, pStreamToRemove);
        audioMixerStreamDestroyInternal(pStreamToRemove);
    }

    if (pSink->pszName)
    {
        RTStrFree(pSink->pszName);
        pSink->pszName = NULL;
    }

    RTCritSectDelete(&pSink->CritSect);

    RTMemFree(pSink);
    pSink = NULL;
}

/**
 * Returns the amount of bytes ready to be read from a sink since the last call
 * to AudioMixerSinkUpdate().
 *
 * @returns Amount of bytes ready to be read from the sink.
 * @param   pSink               Sink to return number of available bytes for.
 */
uint32_t AudioMixerSinkGetReadable(PAUDMIXSINK pSink)
{
    AssertPtrReturn(pSink, 0);

    AssertMsg(pSink->enmDir == AUDMIXSINKDIR_INPUT, ("%s: Can't read from a non-input sink\n", pSink->pszName));

    int rc = RTCritSectEnter(&pSink->CritSect);
    if (RT_FAILURE(rc))
        return 0;

    uint32_t cbReadable = 0;

#ifdef VBOX_AUDIO_MIXER_WITH_MIXBUF
# error "Implement me!"
#else
    /* The hosts sets the pace --
     * so we try to find the maximum of readable data of all connected streams to this sink. */
    PAUDMIXSTREAM pMixStream;
    RTListForEach(&pSink->lstStreams, pMixStream, AUDMIXSTREAM, Node)
    {
        if (!(pMixStream->pConn->pfnStreamGetStatus(pMixStream->pConn, pMixStream->pStream) & PDMAUDIOSTREAMSTS_FLAG_ENABLED))
        {
            Log3Func(("[%s] Stream '%s' disabled, skipping ...\n", pSink->pszName, pMixStream->pszName));
            continue;
        }

        cbReadable = RT_MAX(cbReadable,
                            pMixStream->pConn->pfnStreamGetReadable(pMixStream->pConn, pMixStream->pStream));

        break; /** @todo For now we only support recording by the first stream added. */
    }
#endif

    Log3Func(("[%s] cbReadable=%RU32\n", pSink->pszName, cbReadable));

    int rc2 = RTCritSectLeave(&pSink->CritSect);
    AssertRC(rc2);

    return cbReadable;
}

/**
 * Returns the amount of bytes ready to be written to a sink since the last call
 * to AudioMixerSinkUpdate().
 *
 * @returns Amount of bytes ready to be written to the sink.
 * @param   pSink               Sink to return number of available bytes for.
 */
uint32_t AudioMixerSinkGetWritable(PAUDMIXSINK pSink)
{
    AssertPtrReturn(pSink, 0);

    AssertMsg(pSink->enmDir == AUDMIXSINKDIR_OUTPUT, ("%s: Can't write to a non-output sink\n", pSink->pszName));

    int rc = RTCritSectEnter(&pSink->CritSect);
    if (RT_FAILURE(rc))
        return 0;

    uint32_t cbWritable = UINT32_MAX;

    if (    (pSink->fStatus & AUDMIXSINK_STS_RUNNING)
        && !(pSink->fStatus & AUDMIXSINK_STS_PENDING_DISABLE))
    {
#ifdef VBOX_AUDIO_MIXER_WITH_MIXBUF
# error "Implement me!"
#else
        /* The hosts sets the pace --
         * so we try to find the minimum of writable data to all connected streams to this sink. */
        PAUDMIXSTREAM pMixStream;
        RTListForEach(&pSink->lstStreams, pMixStream, AUDMIXSTREAM, Node)
        {
            const uint32_t cbWritableStream = pMixStream->pConn->pfnStreamGetWritable(pMixStream->pConn, pMixStream->pStream);

            if (cbWritableStream < cbWritable)
                cbWritable = cbWritableStream;
        }
#endif
    }

    if (cbWritable == UINT32_MAX)
        cbWritable = 0;

    Log3Func(("[%s] cbWritable=%RU32\n", pSink->pszName, cbWritable));

    int rc2 = RTCritSectLeave(&pSink->CritSect);
    AssertRC(rc2);

    return cbWritable;
}

/**
 * Returns the sink's mixing direction.
 *
 * @returns Mixing direction.
 * @param   pSink               Sink to return direction for.
 */
AUDMIXSINKDIR AudioMixerSinkGetDir(PAUDMIXSINK pSink)
{
    AssertPtrReturn(pSink, AUDMIXSINKDIR_UNKNOWN);

    int rc = RTCritSectEnter(&pSink->CritSect);
    if (RT_FAILURE(rc))
        return AUDMIXSINKDIR_UNKNOWN;

    AUDMIXSINKDIR enmDir = pSink->enmDir;

    int rc2 = RTCritSectLeave(&pSink->CritSect);
    AssertRC(rc2);

    return enmDir;
}

/**
 * Returns a specific mixer stream from a sink, based on its index.
 *
 * @returns Mixer stream if found, or NULL if not found.
 * @param   pSink               Sink to retrieve mixer stream from.
 * @param   uIndex              Index of the mixer stream to return.
 */
PAUDMIXSTREAM AudioMixerSinkGetStream(PAUDMIXSINK pSink, uint8_t uIndex)
{
    AssertPtrReturn(pSink, NULL);

    int rc = RTCritSectEnter(&pSink->CritSect);
    if (RT_FAILURE(rc))
        return NULL;

    AssertMsgReturn(uIndex < pSink->cStreams,
                    ("Index %RU8 exceeds stream count (%RU8)", uIndex, pSink->cStreams), NULL);

    /* Slow lookup, d'oh. */
    PAUDMIXSTREAM pStream = RTListGetFirst(&pSink->lstStreams, AUDMIXSTREAM, Node);
    while (uIndex)
    {
        pStream = RTListGetNext(&pSink->lstStreams, pStream, AUDMIXSTREAM, Node);
        uIndex--;
    }

    /** @todo Do we need to raise the stream's reference count here? */

    int rc2 = RTCritSectLeave(&pSink->CritSect);
    AssertRC(rc2);

    AssertPtr(pStream);
    return pStream;
}

/**
 * Returns the current status of a mixer sink.
 *
 * @returns The sink's current status.
 * @param   pSink               Mixer sink to return status for.
 */
AUDMIXSINKSTS AudioMixerSinkGetStatus(PAUDMIXSINK pSink)
{
    if (!pSink)
        return AUDMIXSINK_STS_NONE;

    int rc2 = RTCritSectEnter(&pSink->CritSect);
    if (RT_FAILURE(rc2))
        return AUDMIXSINK_STS_NONE;

    /* If the dirty flag is set, there is unprocessed data in the sink. */
    AUDMIXSINKSTS stsSink = pSink->fStatus;

    rc2 = RTCritSectLeave(&pSink->CritSect);
    AssertRC(rc2);

    return stsSink;
}

/**
 * Returns the number of attached mixer streams to a mixer sink.
 *
 * @returns The number of attached mixer streams.
 * @param   pSink               Mixer sink to return number for.
 */
uint8_t AudioMixerSinkGetStreamCount(PAUDMIXSINK pSink)
{
    if (!pSink)
        return 0;

    int rc2 = RTCritSectEnter(&pSink->CritSect);
    if (RT_FAILURE(rc2))
        return 0;

    uint8_t cStreams = pSink->cStreams;

    rc2 = RTCritSectLeave(&pSink->CritSect);
    AssertRC(rc2);

    return cStreams;
}

/**
 * Returns whether the sink is in an active state or not.
 * Note: The pending disable state also counts as active.
 *
 * @returns True if active, false if not.
 * @param   pSink               Sink to return active state for.
 */
bool AudioMixerSinkIsActive(PAUDMIXSINK pSink)
{
    if (!pSink)
        return false;

    int rc2 = RTCritSectEnter(&pSink->CritSect);
    if (RT_FAILURE(rc2))
        return false;

    bool fIsActive = pSink->fStatus & AUDMIXSINK_STS_RUNNING;
    /* Note: AUDMIXSINK_STS_PENDING_DISABLE implies AUDMIXSINK_STS_RUNNING. */

    Log3Func(("[%s] fActive=%RTbool\n", pSink->pszName, fIsActive));

    rc2 = RTCritSectLeave(&pSink->CritSect);
    AssertRC(rc2);

    return fIsActive;
}

/**
 * Reads audio data from a mixer sink.
 *
 * @returns IPRT status code.
 * @param   pSink               Mixer sink to read data from.
 * @param   enmOp               Mixer operation to use for reading the data.
 * @param   pvBuf               Buffer where to store the read data.
 * @param   cbBuf               Buffer size (in bytes) where to store the data.
 * @param   pcbRead             Number of bytes read. Optional.
 */
int AudioMixerSinkRead(PAUDMIXSINK pSink, AUDMIXOP enmOp, void *pvBuf, uint32_t cbBuf, uint32_t *pcbRead)
{
    AssertPtrReturn(pSink, VERR_INVALID_POINTER);
    RT_NOREF(enmOp);
    AssertPtrReturn(pvBuf, VERR_INVALID_POINTER);
    AssertReturn(cbBuf,    VERR_INVALID_PARAMETER);
    /* pcbRead is optional. */

    /** @todo Handle mixing operation enmOp! */

    int rc = RTCritSectEnter(&pSink->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    AssertMsg(pSink->enmDir == AUDMIXSINKDIR_INPUT,
              ("Can't read from a sink which is not an input sink\n"));

#ifdef VBOX_AUDIO_MIXER_WITH_MIXBUF
# error "Implement me!"
#else
    uint8_t *pvMixBuf = (uint8_t *)RTMemAlloc(cbBuf);
    if (!pvMixBuf)
    {
        int rc2 = RTCritSectLeave(&pSink->CritSect);
        AssertRC(rc2);

        return VERR_NO_MEMORY;
    }
#endif

    uint32_t cbRead = 0;

    /* Flag indicating whether this sink is in a 'clean' state,
     * e.g. there is no more data to read from. */
    bool fClean = true;

    PAUDMIXSTREAM pMixStream;
    RTListForEach(&pSink->lstStreams, pMixStream, AUDMIXSTREAM, Node)
    {
        if (!(pMixStream->pConn->pfnStreamGetStatus(pMixStream->pConn, pMixStream->pStream) & PDMAUDIOSTREAMSTS_FLAG_ENABLED))
        {
            Log3Func(("[%s] Stream '%s' disabled, skipping ...\n", pSink->pszName, pMixStream->pszName));
            continue;
        }

        uint32_t cbTotalRead = 0;
        uint32_t cbToRead    = cbBuf;

        int rc2 = VINF_SUCCESS;

        while (cbToRead)
        {
            uint32_t cbReadStrm;
            AssertPtr(pMixStream->pConn);
#ifdef VBOX_AUDIO_MIXER_WITH_MIXBUF
# error "Implement me!"
#else
            rc2 = pMixStream->pConn->pfnStreamRead(pMixStream->pConn, pMixStream->pStream,
                                                   (uint8_t *)pvMixBuf + cbTotalRead, cbToRead, &cbReadStrm);
#endif
            if (RT_FAILURE(rc2))
                LogFunc(("[%s] Failed reading from stream '%s': %Rrc\n", pSink->pszName, pMixStream->pszName, rc2));

            Log3Func(("[%s] Stream '%s': Read %RU32 bytes\n", pSink->pszName, pMixStream->pszName, cbReadStrm));

            if (   RT_FAILURE(rc2)
                || !cbReadStrm)
                break;

            /** @todo Right now we only handle one stream (the last one added in fact). */

            AssertBreakStmt(cbReadStrm <= cbToRead, rc = VERR_BUFFER_OVERFLOW);
            cbToRead    -= cbReadStrm;
            cbTotalRead += cbReadStrm;
        }

        if (RT_FAILURE(rc2))
            continue;

        cbRead = RT_MAX(cbRead, cbTotalRead);

        uint32_t cbReadable = pMixStream->pConn->pfnStreamGetReadable(pMixStream->pConn, pMixStream->pStream);

        /* Still some data available? Then sink is not clean (yet). */
        if (cbReadable)
            fClean = false;
    }

    if (RT_SUCCESS(rc))
    {
        if (fClean)
            pSink->fStatus &= ~AUDMIXSINK_STS_DIRTY;

#ifdef VBOX_AUDIO_MIXER_WITH_MIXBUF
# error "Implement me!"
#else
        if (cbRead)
            memcpy(pvBuf, pvMixBuf, cbRead);
#endif
        if (pcbRead)
            *pcbRead = cbRead;
    }

#ifndef VBOX_AUDIO_MIXER_WITH_MIXBUF
    RTMemFree(pvMixBuf);
#endif


#ifdef LOG_ENABLED
    char *pszStatus = dbgAudioMixerSinkStatusToStr(pSink->fStatus);
    Log2Func(("[%s] cbRead=%RU32, fClean=%RTbool, fStatus=%s, rc=%Rrc\n", pSink->pszName, cbRead, fClean, pszStatus, rc));
    RTStrFree(pszStatus);
#endif

    int rc2 = RTCritSectLeave(&pSink->CritSect);
    AssertRC(rc2);

    return rc;
}

/**
 * Removes a mixer stream from a mixer sink, internal version.
 *
 * @returns IPRT status code.
 * @param   pSink               Sink to remove mixer stream from.
 * @param   pStream             Stream to remove.
 */
static int audioMixerSinkRemoveStreamInternal(PAUDMIXSINK pSink, PAUDMIXSTREAM pStream)
{
    AssertPtrReturn(pSink, VERR_INVALID_PARAMETER);
    if (   !pStream
        || !pStream->pSink) /* Not part of a sink anymore? */
    {
        return VERR_NOT_FOUND;
    }

    AssertMsgReturn(pStream->pSink == pSink, ("Stream '%s' is not part of sink '%s'\n",
                                              pStream->pszName, pSink->pszName), VERR_NOT_FOUND);

    LogFlowFunc(("[%s] (Stream = %s), cStreams=%RU8\n",
                 pSink->pszName, pStream->pStream->szName, pSink->cStreams));

#ifdef VBOX_AUDIO_MIXER_WITH_MIXBUF
    /* Unlink mixing buffer. */
    AudioMixBufUnlink(&pStream->pStream->MixBuf);
#endif

    /* Remove stream from sink. */
    RTListNodeRemove(&pStream->Node);

    /* Set sink to NULL so that we know we're not part of any sink anymore. */
    pStream->pSink = NULL;

    return VINF_SUCCESS;
}

/**
 * Removes a mixer stream from a mixer sink.
 *
 * @param   pSink               Sink to remove mixer stream from.
 * @param   pStream             Stream to remove.
 */
void AudioMixerSinkRemoveStream(PAUDMIXSINK pSink, PAUDMIXSTREAM pStream)
{
    int rc2 = RTCritSectEnter(&pSink->CritSect);
    AssertRC(rc2);

    rc2 = audioMixerSinkRemoveStreamInternal(pSink, pStream);
    if (RT_SUCCESS(rc2))
    {
        Assert(pSink->cStreams);
        pSink->cStreams--;
    }

    rc2 = RTCritSectLeave(&pSink->CritSect);
    AssertRC(rc2);
}

/**
 * Removes all attached streams from a given sink.
 *
 * @param pSink                 Sink to remove attached streams from.
 */
static void audioMixerSinkRemoveAllStreamsInternal(PAUDMIXSINK pSink)
{
    if (!pSink)
        return;

    LogFunc(("%s\n", pSink->pszName));

    PAUDMIXSTREAM pStream, pStreamNext;
    RTListForEachSafe(&pSink->lstStreams, pStream, pStreamNext, AUDMIXSTREAM, Node)
        audioMixerSinkRemoveStreamInternal(pSink, pStream);
}

/**
 * Resets the sink's state.
 *
 * @param   pSink               Sink to reset.
 */
static void audioMixerSinkReset(PAUDMIXSINK pSink)
{
    if (!pSink)
        return;

    LogFunc(("[%s]\n", pSink->pszName));

    if (pSink->enmDir == AUDMIXSINKDIR_INPUT)
    {
#ifdef VBOX_AUDIO_MIXER_WITH_MIXBUF
        AudioMixBufReset(&pSink->MixBuf);
#else
        pSink->In.cbReadable = 0;
#endif
    }
    else if (pSink->enmDir == AUDMIXSINKDIR_OUTPUT)
    {
#ifdef VBOX_AUDIO_MIXER_WITH_MIXBUF
        AudioMixBufReset(&pSink->MixBuf);
#else
        pSink->Out.cbWritable = 0;
#endif
    }

    /* Update last updated timestamp. */
    pSink->tsLastUpdatedMS = RTTimeMilliTS();

    /* Reset status. */
    pSink->fStatus = AUDMIXSINK_STS_NONE;
}

/**
 * Removes all attached streams from a given sink.
 *
 * @param pSink                 Sink to remove attached streams from.
 */
void AudioMixerSinkRemoveAllStreams(PAUDMIXSINK pSink)
{
    if (!pSink)
        return;

    int rc2 = RTCritSectEnter(&pSink->CritSect);
    AssertRC(rc2);

    audioMixerSinkRemoveAllStreamsInternal(pSink);

    pSink->cStreams = 0;

    rc2 = RTCritSectLeave(&pSink->CritSect);
    AssertRC(rc2);
}

/**
 * Resets a sink. This will immediately stop all processing.
 *
 * @param pSink                 Sink to reset.
 */
void AudioMixerSinkReset(PAUDMIXSINK pSink)
{
    if (!pSink)
        return;

    int rc2 = RTCritSectEnter(&pSink->CritSect);
    AssertRC(rc2);

    LogFlowFunc(("[%s]\n", pSink->pszName));

    audioMixerSinkReset(pSink);

    rc2 = RTCritSectLeave(&pSink->CritSect);
    AssertRC(rc2);
}

/**
 * Returns the audio format of a mixer sink.
 *
 * @param   pSink               Sink to retrieve audio format for.
 * @param   pPCMProps           Where to the returned audio format.
 */
void AudioMixerSinkGetFormat(PAUDMIXSINK pSink, PPDMAUDIOPCMPROPS pPCMProps)
{
    AssertPtrReturnVoid(pSink);
    AssertPtrReturnVoid(pPCMProps);

    int rc2 = RTCritSectEnter(&pSink->CritSect);
    if (RT_FAILURE(rc2))
        return;

    memcpy(pPCMProps, &pSink->PCMProps, sizeof(PDMAUDIOPCMPROPS));

    rc2 = RTCritSectLeave(&pSink->CritSect);
    AssertRC(rc2);
}

/**
 * Sets the audio format of a mixer sink.
 *
 * @returns IPRT status code.
 * @param   pSink               Sink to set audio format for.
 * @param   pPCMProps           Audio format (PCM properties) to set.
 */
int AudioMixerSinkSetFormat(PAUDMIXSINK pSink, PPDMAUDIOPCMPROPS pPCMProps)
{
    AssertPtrReturn(pSink,     VERR_INVALID_POINTER);
    AssertPtrReturn(pPCMProps, VERR_INVALID_POINTER);

    int rc = RTCritSectEnter(&pSink->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    if (DrvAudioHlpPCMPropsAreEqual(&pSink->PCMProps, pPCMProps)) /* Bail out early if PCM properties are equal. */
    {
        rc = RTCritSectLeave(&pSink->CritSect);
        AssertRC(rc);

        return rc;
    }

    if (pSink->PCMProps.uHz)
        LogFlowFunc(("[%s] Old format: %RU8 bit, %RU8 channels, %RU32Hz\n",
                     pSink->pszName, pSink->PCMProps.cBits, pSink->PCMProps.cChannels, pSink->PCMProps.uHz));

    memcpy(&pSink->PCMProps, pPCMProps, sizeof(PDMAUDIOPCMPROPS));

    LogFlowFunc(("[%s] New format %RU8 bit, %RU8 channels, %RU32Hz\n",
                 pSink->pszName, pSink->PCMProps.cBits, pSink->PCMProps.cChannels, pSink->PCMProps.uHz));

#ifdef VBOX_AUDIO_MIXER_WITH_MIXBUF
    /* Also update the sink's mixing buffer format. */
    AudioMixBufDestroy(&pSink->MixBuf);
    rc = AudioMixBufInit(&pSink->MixBuf, pSink->pszName, pPCMProps, _4K /** @todo Make configurable? */);
    if (RT_SUCCESS(rc))
    {
        PAUDMIXSTREAM pStream;
        RTListForEach(&pSink->lstStreams, pStream, AUDMIXSTREAM, Node)
        {
            /** @todo Invalidate mix buffers! */
        }
    }
#endif /* VBOX_AUDIO_MIXER_WITH_MIXBUF */

    int rc2 = RTCritSectLeave(&pSink->CritSect);
    AssertRC(rc2);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Set the volume of an individual sink.
 *
 * @returns IPRT status code.
 * @param   pSink               Sink to set volume for.
 * @param   pVol                Volume to set.
 */
int AudioMixerSinkSetVolume(PAUDMIXSINK pSink, PPDMAUDIOVOLUME pVol)
{
    AssertPtrReturn(pSink, VERR_INVALID_POINTER);
    AssertPtrReturn(pVol,  VERR_INVALID_POINTER);

    int rc = RTCritSectEnter(&pSink->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    memcpy(&pSink->Volume, pVol, sizeof(PDMAUDIOVOLUME));

    LogFlowFunc(("[%s] fMuted=%RTbool, lVol=%RU32, rVol=%RU32\n",
                 pSink->pszName, pSink->Volume.fMuted, pSink->Volume.uLeft, pSink->Volume.uRight));

    AssertPtr(pSink->pParent);
    rc = audioMixerSinkUpdateVolume(pSink, &pSink->pParent->VolMaster);

    int rc2 = RTCritSectLeave(&pSink->CritSect);
    AssertRC(rc2);

    return rc;
}

/**
 * Updates a mixer sink, internal version.
 *
 * @returns IPRT status code.
 * @param   pSink               Mixer sink to update.
 */
static int audioMixerSinkUpdateInternal(PAUDMIXSINK pSink)
{
    AssertPtrReturn(pSink, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;

#ifdef LOG_ENABLED
    char *pszStatus = dbgAudioMixerSinkStatusToStr(pSink->fStatus);
    Log3Func(("[%s] fStatus=%s\n", pSink->pszName, pszStatus));
    RTStrFree(pszStatus);
#endif

    /* Sink disabled? Take a shortcut. */
    if (!(pSink->fStatus & AUDMIXSINK_STS_RUNNING))
        return rc;

    /* Number of detected disabled streams of this sink. */
    uint8_t cStreamsDisabled = 0;

    PAUDMIXSTREAM pMixStream, pMixStreamNext;
    RTListForEachSafe(&pSink->lstStreams, pMixStream, pMixStreamNext, AUDMIXSTREAM, Node)
    {
        PPDMAUDIOSTREAM pStream   = pMixStream->pStream;
        AssertPtr(pStream);

        PPDMIAUDIOCONNECTOR pConn = pMixStream->pConn;
        AssertPtr(pConn);

        uint32_t cfProc = 0;

        int rc2 = pConn->pfnStreamIterate(pConn, pStream);
        if (RT_SUCCESS(rc2))
        {
            if (pSink->enmDir == AUDMIXSINKDIR_INPUT)
            {
                rc = pConn->pfnStreamCapture(pConn, pStream, &cfProc);
                if (RT_FAILURE(rc2))
                {
                    LogFunc(("%s: Failed capturing stream '%s', rc=%Rrc\n", pSink->pszName, pStream->szName, rc2));
                    if (RT_SUCCESS(rc))
                        rc = rc2;
                    continue;
                }

                if (cfProc)
                    pSink->fStatus |= AUDMIXSINK_STS_DIRTY;
            }
            else if (pSink->enmDir == AUDMIXSINKDIR_OUTPUT)
            {
                rc2 = pConn->pfnStreamPlay(pConn, pStream, &cfProc);
                if (RT_FAILURE(rc2))
                {
                    LogFunc(("%s: Failed playing stream '%s', rc=%Rrc\n", pSink->pszName, pStream->szName, rc2));
                    if (RT_SUCCESS(rc))
                        rc = rc2;
                    continue;
                }
            }
            else
            {
                AssertFailedStmt(rc = VERR_NOT_IMPLEMENTED);
                continue;
            }

            rc2 = pConn->pfnStreamIterate(pConn, pStream);
            if (RT_FAILURE(rc2))
            {
                LogFunc(("%s: Failed re-iterating stream '%s', rc=%Rrc\n", pSink->pszName, pStream->szName, rc2));
                if (RT_SUCCESS(rc))
                    rc = rc2;
                continue;
            }

            PDMAUDIOSTREAMSTS strmSts = pConn->pfnStreamGetStatus(pConn, pStream);

            /* Is the stream not enabled and also is not in a pending disable state anymore? */
            if (   !(strmSts & PDMAUDIOSTREAMSTS_FLAG_ENABLED)
                && !(strmSts & PDMAUDIOSTREAMSTS_FLAG_PENDING_DISABLE))
            {
                cStreamsDisabled++;
            }
        }

        Log3Func(("\t%s: cPlayed/cCaptured=%RU32, rc2=%Rrc\n", pStream->szName, cfProc, rc2));
    }

    Log3Func(("[%s] fPendingDisable=%RTbool, %RU8/%RU8 streams disabled\n",
              pSink->pszName, RT_BOOL(pSink->fStatus & AUDMIXSINK_STS_PENDING_DISABLE), cStreamsDisabled, pSink->cStreams));

    /* Update last updated timestamp. */
    pSink->tsLastUpdatedMS = RTTimeMilliTS();

    /* All streams disabled and the sink is in pending disable mode? */
    if (   cStreamsDisabled == pSink->cStreams
        && (pSink->fStatus & AUDMIXSINK_STS_PENDING_DISABLE))
    {
        audioMixerSinkReset(pSink);
    }

    Log3Func(("[%s] cbReadable=%RU32, cbWritable=%RU32, rc=%Rrc\n",
              pSink->pszName, pSink->In.cbReadable, pSink->Out.cbWritable, rc));

    return rc;
}

/**
 * Updates (invalidates) a mixer sink.
 *
 * @returns IPRT status code.
 * @param   pSink               Mixer sink to update.
 */
int AudioMixerSinkUpdate(PAUDMIXSINK pSink)
{
    AssertPtrReturn(pSink, VERR_INVALID_POINTER);

    int rc = RTCritSectEnter(&pSink->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    rc = audioMixerSinkUpdateInternal(pSink);

    int rc2 = RTCritSectLeave(&pSink->CritSect);
    AssertRC(rc2);

    return rc;
}

/**
 * Updates the (master) volume of a mixer sink.
 *
 * @returns IPRT status code.
 * @param   pSink               Mixer sink to update volume for.
 * @param   pVolMaster          Master volume to set.
 */
static int audioMixerSinkUpdateVolume(PAUDMIXSINK pSink, const PPDMAUDIOVOLUME pVolMaster)
{
    AssertPtrReturn(pSink,      VERR_INVALID_POINTER);
    AssertPtrReturn(pVolMaster, VERR_INVALID_POINTER);

    LogFlowFunc(("[%s] Master fMuted=%RTbool, lVol=%RU32, rVol=%RU32\n",
                  pSink->pszName, pVolMaster->fMuted, pVolMaster->uLeft, pVolMaster->uRight));
    LogFlowFunc(("[%s] fMuted=%RTbool, lVol=%RU32, rVol=%RU32 ",
                  pSink->pszName, pSink->Volume.fMuted, pSink->Volume.uLeft, pSink->Volume.uRight));

    /** @todo Very crude implementation for now -- needs more work! */

    pSink->VolumeCombined.fMuted  = pVolMaster->fMuted || pSink->Volume.fMuted;

    pSink->VolumeCombined.uLeft   = (  (pSink->Volume.uLeft ? pSink->Volume.uLeft : 1)
                                     * (pVolMaster->uLeft   ? pVolMaster->uLeft   : 1)) / PDMAUDIO_VOLUME_MAX;

    pSink->VolumeCombined.uRight  = (  (pSink->Volume.uRight ? pSink->Volume.uRight : 1)
                                     * (pVolMaster->uRight   ? pVolMaster->uRight   : 1)) / PDMAUDIO_VOLUME_MAX;

    LogFlow(("-> fMuted=%RTbool, lVol=%RU32, rVol=%RU32\n",
             pSink->VolumeCombined.fMuted, pSink->VolumeCombined.uLeft, pSink->VolumeCombined.uRight));

    /* Propagate new sink volume to all streams in the sink. */
    PAUDMIXSTREAM pMixStream;
    RTListForEach(&pSink->lstStreams, pMixStream, AUDMIXSTREAM, Node)
    {
        int rc2 = pMixStream->pConn->pfnStreamSetVolume(pMixStream->pConn, pMixStream->pStream, &pSink->VolumeCombined);
        AssertRC(rc2);
    }

    return VINF_SUCCESS;
}

/**
 * Writes data to a mixer sink.
 *
 * @returns IPRT status code.
 * @param   pSink               Sink to write data to.
 * @param   enmOp               Mixer operation to use when writing data to the sink.
 * @param   pvBuf               Buffer containing the audio data to write.
 * @param   cbBuf               Size (in bytes) of the buffer containing the audio data.
 * @param   pcbWritten          Number of bytes written. Optional.
 */
int AudioMixerSinkWrite(PAUDMIXSINK pSink, AUDMIXOP enmOp, const void *pvBuf, uint32_t cbBuf, uint32_t *pcbWritten)
{
    AssertPtrReturn(pSink, VERR_INVALID_POINTER);
    RT_NOREF(enmOp);
    AssertPtrReturn(pvBuf, VERR_INVALID_POINTER);
    AssertReturn   (cbBuf, VERR_INVALID_PARAMETER);
    /* pcbWritten is optional. */

    int rc = RTCritSectEnter(&pSink->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    AssertMsg(pSink->fStatus & AUDMIXSINK_STS_RUNNING,
              ("%s: Can't write to a sink which is not running (anymore) (status 0x%x)\n", pSink->pszName, pSink->fStatus));
    AssertMsg(pSink->enmDir == AUDMIXSINKDIR_OUTPUT,
              ("%s: Can't write to a sink which is not an output sink\n", pSink->pszName));

    Log3Func(("[%s] enmOp=%d, cbBuf=%RU32\n", pSink->pszName, enmOp, cbBuf));

    uint32_t cbWritten = UINT32_MAX;

    PAUDMIXSTREAM pMixStream;
    RTListForEach(&pSink->lstStreams, pMixStream, AUDMIXSTREAM, Node)
    {
        uint32_t cbProcessed = 0;
        int rc2 = pMixStream->pConn->pfnStreamWrite(pMixStream->pConn, pMixStream->pStream, pvBuf, cbBuf, &cbProcessed);
        if (RT_FAILURE(rc2))
            LogFunc(("[%s] Failed writing to stream '%s': %Rrc\n", pSink->pszName, pMixStream->pszName, rc2));

        Log3Func(("[%s] Written %RU32 to '%s'\n", pSink->pszName, cbProcessed, pMixStream->pszName));

        if (cbProcessed)
        {
            /* Set dirty bit. */
            pSink->fStatus |= AUDMIXSINK_STS_DIRTY;

            /*
             * Return the minimum bytes processed by all connected streams.
             * The host sets the pace, so all backends have to behave accordingly.
             */
            if (cbWritten > cbProcessed)
                cbWritten = cbProcessed;
        }
    }

    if (cbWritten == UINT32_MAX)
        cbWritten = 0;

    Log3Func(("[%s] cbWritten=%RU32\n", pSink->pszName, cbWritten));

    if (pcbWritten)
        *pcbWritten = cbWritten;

    int rc2 = RTCritSectLeave(&pSink->CritSect);
    AssertRC(rc2);

    return rc;
}

/*********************************************************************************************************************************
 * Mixer Stream implementation.
 ********************************************************************************************************************************/

/**
 * Controls a mixer stream, internal version.
 *
 * @returns IPRT status code.
 * @param   pMixStream          Mixer stream to control.
 * @param   enmCmd              Mixer stream command to use.
 * @param   fCtl                Additional control flags. Pass 0.
 */
int audioMixerStreamCtlInternal(PAUDMIXSTREAM pMixStream, PDMAUDIOSTREAMCMD enmCmd, uint32_t fCtl)
{
    AssertPtr(pMixStream->pConn);
    AssertPtr(pMixStream->pStream);

    RT_NOREF(fCtl);

    int rc = pMixStream->pConn->pfnStreamControl(pMixStream->pConn, pMixStream->pStream, enmCmd);

    LogFlowFunc(("[%s] enmCmd=%ld, rc=%Rrc\n", pMixStream->pszName, enmCmd, rc));

    return rc;
}

/**
 * Controls a mixer stream.
 *
 * @returns IPRT status code.
 * @param   pMixStream          Mixer stream to control.
 * @param   enmCmd              Mixer stream command to use.
 * @param   fCtl                Additional control flags. Pass 0.
 */
int AudioMixerStreamCtl(PAUDMIXSTREAM pMixStream, PDMAUDIOSTREAMCMD enmCmd, uint32_t fCtl)
{
    RT_NOREF(fCtl);
    AssertPtrReturn(pMixStream, VERR_INVALID_POINTER);
    /** @todo Validate fCtl. */

    int rc = RTCritSectEnter(&pMixStream->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    rc = audioMixerStreamCtlInternal(pMixStream, enmCmd, fCtl);

    int rc2 = RTCritSectLeave(&pMixStream->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    return rc;
}

/**
 * Destroys a mixer stream, internal version.
 *
 * @param   pMixStream          Mixer stream to destroy.
 */
static void audioMixerStreamDestroyInternal(PAUDMIXSTREAM pMixStream)
{
    AssertPtrReturnVoid(pMixStream);

    LogFunc(("%s\n", pMixStream->pszName));

    if (pMixStream->pConn) /* Stream has a connector interface present? */
    {
        if (pMixStream->pStream)
        {
            pMixStream->pConn->pfnStreamRelease(pMixStream->pConn, pMixStream->pStream);
            pMixStream->pConn->pfnStreamDestroy(pMixStream->pConn, pMixStream->pStream);

            pMixStream->pStream = NULL;
        }

        pMixStream->pConn = NULL;
    }

    if (pMixStream->pszName)
    {
        RTStrFree(pMixStream->pszName);
        pMixStream->pszName = NULL;
    }

    int rc2 = RTCritSectDelete(&pMixStream->CritSect);
    AssertRC(rc2);

    RTMemFree(pMixStream);
    pMixStream = NULL;
}

/**
 * Destroys a mixer stream.
 *
 * @param   pMixStream          Mixer stream to destroy.
 */
void AudioMixerStreamDestroy(PAUDMIXSTREAM pMixStream)
{
    if (!pMixStream)
        return;

    int rc2 = RTCritSectEnter(&pMixStream->CritSect);
    AssertRC(rc2);

    LogFunc(("%s\n", pMixStream->pszName));

    if (pMixStream->pSink) /* Is the stream part of a sink? */
    {
        /* Save sink pointer, as after audioMixerSinkRemoveStreamInternal() the
         * pointer will be gone from the stream. */
        PAUDMIXSINK pSink = pMixStream->pSink;

        rc2 = audioMixerSinkRemoveStreamInternal(pSink, pMixStream);
        if (RT_SUCCESS(rc2))
        {
            Assert(pSink->cStreams);
            pSink->cStreams--;
        }
    }
    else
        rc2 = VINF_SUCCESS;

    int rc3 = RTCritSectLeave(&pMixStream->CritSect);
    AssertRC(rc3);

    if (RT_SUCCESS(rc2))
    {
        audioMixerStreamDestroyInternal(pMixStream);
        pMixStream = NULL;
    }

    LogFlowFunc(("Returning %Rrc\n", rc2));
}

/**
 * Returns whether a mixer stream currently is active (playing/recording) or not.
 *
 * @returns @c true if playing/recording, @c false if not.
 * @param   pMixStream          Mixer stream to return status for.
 */
bool AudioMixerStreamIsActive(PAUDMIXSTREAM pMixStream)
{
    int rc2 = RTCritSectEnter(&pMixStream->CritSect);
    if (RT_FAILURE(rc2))
        return false;

    AssertPtr(pMixStream->pConn);
    AssertPtr(pMixStream->pStream);

    bool fIsActive;

    if (   pMixStream->pConn
        && pMixStream->pStream
        && RT_BOOL(pMixStream->pConn->pfnStreamGetStatus(pMixStream->pConn, pMixStream->pStream) & PDMAUDIOSTREAMSTS_FLAG_ENABLED))
    {
        fIsActive = true;
    }
    else
        fIsActive = false;

    rc2 = RTCritSectLeave(&pMixStream->CritSect);
    AssertRC(rc2);

    return fIsActive;
}

/**
 * Returns whether a mixer stream is valid (e.g. initialized and in a working state) or not.
 *
 * @returns @c true if valid, @c false if not.
 * @param   pMixStream          Mixer stream to return status for.
 */
bool AudioMixerStreamIsValid(PAUDMIXSTREAM pMixStream)
{
    if (!pMixStream)
        return false;

    int rc2 = RTCritSectEnter(&pMixStream->CritSect);
    if (RT_FAILURE(rc2))
        return false;

    bool fIsValid;

    if (   pMixStream->pConn
        && pMixStream->pStream
        && RT_BOOL(pMixStream->pConn->pfnStreamGetStatus(pMixStream->pConn, pMixStream->pStream) & PDMAUDIOSTREAMSTS_FLAG_INITIALIZED))
    {
        fIsValid = true;
    }
    else
        fIsValid = false;

    rc2 = RTCritSectLeave(&pMixStream->CritSect);
    AssertRC(rc2);

    return fIsValid;
}

