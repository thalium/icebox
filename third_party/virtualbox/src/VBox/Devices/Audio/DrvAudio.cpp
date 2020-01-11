/* $Id: DrvAudio.cpp $ */
/** @file
 * Intermediate audio driver header.
 *
 * @remarks Intermediate audio driver for connecting the audio device emulation
 *          with the host backend.
 */

/*
 * Copyright (C) 2006-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#define LOG_GROUP LOG_GROUP_DRV_AUDIO
#include <VBox/log.h>
#include <VBox/vmm/pdm.h>
#include <VBox/err.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/pdmaudioifs.h>

#include <iprt/alloc.h>
#include <iprt/asm-math.h>
#include <iprt/assert.h>
#include <iprt/circbuf.h>
#include <iprt/string.h>
#include <iprt/uuid.h>

#include "VBoxDD.h"

#include <ctype.h>
#include <stdlib.h>

#include "DrvAudio.h"
#include "AudioMixBuffer.h"

#ifdef VBOX_WITH_AUDIO_ENUM
static int drvAudioDevicesEnumerateInternal(PDRVAUDIO pThis, bool fLog, PPDMAUDIODEVICEENUM pDevEnum);
#endif

static DECLCALLBACK(int) drvAudioStreamDestroy(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream);
static int drvAudioStreamControlInternalBackend(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd);
static int drvAudioStreamControlInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd);
static int drvAudioStreamCreateInternalBackend(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream, PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq);
static int drvAudioStreamDestroyInternalBackend(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream);
static void drvAudioStreamFree(PPDMAUDIOSTREAM pStream);
static int drvAudioStreamUninitInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream);
static int drvAudioStreamInitInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream, PPDMAUDIOSTREAMCFG pCfgHost, PPDMAUDIOSTREAMCFG pCfgGuest);
static int drvAudioStreamIterateInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream);
static int drvAudioStreamReInitInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream);
static void drvAudioStreamDropInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream);
static void drvAudioStreamResetInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream);

#ifndef VBOX_AUDIO_TESTCASE

# if 0 /* unused */

static PDMAUDIOFMT drvAudioGetConfFormat(PCFGMNODE pCfgHandle, const char *pszKey,
                                         PDMAUDIOFMT enmDefault, bool *pfDefault)
{
    if (   pCfgHandle == NULL
        || pszKey == NULL)
    {
        *pfDefault = true;
        return enmDefault;
    }

    char *pszValue = NULL;
    int rc = CFGMR3QueryStringAlloc(pCfgHandle, pszKey, &pszValue);
    if (RT_FAILURE(rc))
    {
        *pfDefault = true;
        return enmDefault;
    }

    PDMAUDIOFMT fmt = DrvAudioHlpStrToAudFmt(pszValue);
    if (fmt == PDMAUDIOFMT_INVALID)
    {
         *pfDefault = true;
        return enmDefault;
    }

    *pfDefault = false;
    return fmt;
}

static int drvAudioGetConfInt(PCFGMNODE pCfgHandle, const char *pszKey,
                              int iDefault, bool *pfDefault)
{

    if (   pCfgHandle == NULL
        || pszKey == NULL)
    {
        *pfDefault = true;
        return iDefault;
    }

    uint64_t u64Data = 0;
    int rc = CFGMR3QueryInteger(pCfgHandle, pszKey, &u64Data);
    if (RT_FAILURE(rc))
    {
        *pfDefault = true;
        return iDefault;

    }

    *pfDefault = false;
    return u64Data;
}

static const char *drvAudioGetConfStr(PCFGMNODE pCfgHandle, const char *pszKey,
                                      const char *pszDefault, bool *pfDefault)
{
    if (   pCfgHandle == NULL
        || pszKey == NULL)
    {
        *pfDefault = true;
        return pszDefault;
    }

    char *pszValue = NULL;
    int rc = CFGMR3QueryStringAlloc(pCfgHandle, pszKey, &pszValue);
    if (RT_FAILURE(rc))
    {
        *pfDefault = true;
        return pszDefault;
    }

    *pfDefault = false;
    return pszValue;
}

# endif /* unused */

#ifdef LOG_ENABLED
/**
 * Converts an audio stream status to a string.
 *
 * @returns Stringified stream status flags. Must be free'd with RTStrFree().
 *          "NONE" if no flags set.
 * @param   fStatus             Stream status flags to convert.
 */
static char *dbgAudioStreamStatusToStr(PDMAUDIOSTREAMSTS fStatus)
{
#define APPEND_FLAG_TO_STR(_aFlag)                 \
    if (fStatus & PDMAUDIOSTREAMSTS_FLAG_##_aFlag) \
    {                                              \
        if (pszFlags)                              \
        {                                          \
            rc2 = RTStrAAppend(&pszFlags, " ");    \
            if (RT_FAILURE(rc2))                   \
                break;                             \
        }                                          \
                                                   \
        rc2 = RTStrAAppend(&pszFlags, #_aFlag);    \
        if (RT_FAILURE(rc2))                       \
            break;                                 \
    }                                              \

    char *pszFlags = NULL;
    int rc2 = VINF_SUCCESS;

    do
    {
        APPEND_FLAG_TO_STR(INITIALIZED    );
        APPEND_FLAG_TO_STR(ENABLED        );
        APPEND_FLAG_TO_STR(PAUSED         );
        APPEND_FLAG_TO_STR(PENDING_DISABLE);
        APPEND_FLAG_TO_STR(PENDING_REINIT );
    } while (0);

    if (!pszFlags)
        rc2 = RTStrAAppend(&pszFlags, "NONE");

    if (   RT_FAILURE(rc2)
        && pszFlags)
    {
        RTStrFree(pszFlags);
        pszFlags = NULL;
    }

#undef APPEND_FLAG_TO_STR

    return pszFlags;
}
#endif /* defined(VBOX_STRICT) || defined(LOG_ENABLED) */

# if 0 /* unused */
static int drvAudioProcessOptions(PCFGMNODE pCfgHandle, const char *pszPrefix, audio_option *paOpts, size_t cOpts)
{
    AssertPtrReturn(pCfgHandle, VERR_INVALID_POINTER);
    AssertPtrReturn(pszPrefix,  VERR_INVALID_POINTER);
    /* oaOpts and cOpts are optional. */

    PCFGMNODE pCfgChildHandle = NULL;
    PCFGMNODE pCfgChildChildHandle = NULL;

   /* If pCfgHandle is NULL, let NULL be passed to get int and get string functions..
    * The getter function will return default values.
    */
    if (pCfgHandle != NULL)
    {
       /* If its audio general setting, need to traverse to one child node.
        * /Devices/ichac97/0/LUN#0/Config/Audio
        */
       if(!strncmp(pszPrefix, "AUDIO", 5)) /** @todo Use a \#define */
       {
            pCfgChildHandle = CFGMR3GetFirstChild(pCfgHandle);
            if(pCfgChildHandle)
                pCfgHandle = pCfgChildHandle;
        }
        else
        {
            /* If its driver specific configuration , then need to traverse two level deep child
             * child nodes. for eg. in case of DirectSoundConfiguration item
             * /Devices/ichac97/0/LUN#0/Config/Audio/DirectSoundConfig
             */
            pCfgChildHandle = CFGMR3GetFirstChild(pCfgHandle);
            if (pCfgChildHandle)
            {
                pCfgChildChildHandle = CFGMR3GetFirstChild(pCfgChildHandle);
                if (pCfgChildChildHandle)
                    pCfgHandle = pCfgChildChildHandle;
            }
        }
    }

    for (size_t i = 0; i < cOpts; i++)
    {
        audio_option *pOpt = &paOpts[i];
        if (!pOpt->valp)
        {
            LogFlowFunc(("Option value pointer for `%s' is not set\n", pOpt->name));
            continue;
        }

        bool fUseDefault;

        switch (pOpt->tag)
        {
            case AUD_OPT_BOOL:
            case AUD_OPT_INT:
            {
                int *intp = (int *)pOpt->valp;
                *intp = drvAudioGetConfInt(pCfgHandle, pOpt->name, *intp, &fUseDefault);

                break;
            }

            case AUD_OPT_FMT:
            {
                PDMAUDIOFMT *fmtp = (PDMAUDIOFMT *)pOpt->valp;
                *fmtp = drvAudioGetConfFormat(pCfgHandle, pOpt->name, *fmtp, &fUseDefault);

                break;
            }

            case AUD_OPT_STR:
            {
                const char **strp = (const char **)pOpt->valp;
                *strp = drvAudioGetConfStr(pCfgHandle, pOpt->name, *strp, &fUseDefault);

                break;
            }

            default:
                LogFlowFunc(("Bad value tag for option `%s' - %d\n", pOpt->name, pOpt->tag));
                fUseDefault = false;
                break;
        }

        if (!pOpt->overridenp)
            pOpt->overridenp = &pOpt->overriden;

        *pOpt->overridenp = !fUseDefault;
    }

    return VINF_SUCCESS;
}
# endif /* unused */
#endif /* !VBOX_AUDIO_TESTCASE */

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamControl}
 */
static DECLCALLBACK(int) drvAudioStreamControl(PPDMIAUDIOCONNECTOR pInterface,
                                               PPDMAUDIOSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);

    if (!pStream)
        return VINF_SUCCESS;

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    LogFlowFunc(("[%s] enmStreamCmd=%s\n", pStream->szName, DrvAudioHlpStreamCmdToStr(enmStreamCmd)));

    rc = drvAudioStreamControlInternal(pThis, pStream, enmStreamCmd);

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    return rc;
}

/**
 * Controls an audio stream.
 *
 * @returns IPRT status code.
 * @param   pThis               Pointer to driver instance.
 * @param   pStream             Stream to control.
 * @param   enmStreamCmd        Control command.
 */
static int drvAudioStreamControlInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    LogFunc(("[%s] enmStreamCmd=%s\n", pStream->szName, DrvAudioHlpStreamCmdToStr(enmStreamCmd)));

#ifdef LOG_ENABLED
    char *pszStreamSts = dbgAudioStreamStatusToStr(pStream->fStatus);
    LogFlowFunc(("fStatus=%s\n", pszStreamSts));
    RTStrFree(pszStreamSts);
#endif /* LOG_ENABLED */

    int rc = VINF_SUCCESS;

    switch (enmStreamCmd)
    {
        case PDMAUDIOSTREAMCMD_ENABLE:
        {
            if (!(pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_ENABLED))
            {
                /* Is a pending disable outstanding? Then disable first. */
                if (pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_PENDING_DISABLE)
                    rc = drvAudioStreamControlInternalBackend(pThis, pStream, PDMAUDIOSTREAMCMD_DISABLE);

                if (RT_SUCCESS(rc))
                    rc = drvAudioStreamControlInternalBackend(pThis, pStream, PDMAUDIOSTREAMCMD_ENABLE);

                if (RT_SUCCESS(rc))
                    pStream->fStatus |= PDMAUDIOSTREAMSTS_FLAG_ENABLED;
            }
            break;
        }

        case PDMAUDIOSTREAMCMD_DISABLE:
        {
            if (pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_ENABLED)
            {
                /*
                 * For playback (output) streams first mark the host stream as pending disable,
                 * so that the rest of the remaining audio data will be played first before
                 * closing the stream.
                 */
                if (pStream->enmDir == PDMAUDIODIR_OUT)
                {
                    LogFunc(("[%s] Pending disable/pause\n", pStream->szName));
                    pStream->fStatus |= PDMAUDIOSTREAMSTS_FLAG_PENDING_DISABLE;
                }

                /* Can we close the host stream as well (not in pending disable mode)? */
                if (!(pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_PENDING_DISABLE))
                {
                    rc = drvAudioStreamControlInternalBackend(pThis, pStream, PDMAUDIOSTREAMCMD_DISABLE);
                    if (RT_SUCCESS(rc))
                        drvAudioStreamResetInternal(pThis, pStream);
                }
            }
            break;
        }

        case PDMAUDIOSTREAMCMD_PAUSE:
        {
            if (!(pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_PAUSED))
            {
                rc = drvAudioStreamControlInternalBackend(pThis, pStream, PDMAUDIOSTREAMCMD_PAUSE);
                if (RT_SUCCESS(rc))
                    pStream->fStatus |= PDMAUDIOSTREAMSTS_FLAG_PAUSED;
            }
            break;
        }

        case PDMAUDIOSTREAMCMD_RESUME:
        {
            if (pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_PAUSED)
            {
                rc = drvAudioStreamControlInternalBackend(pThis, pStream, PDMAUDIOSTREAMCMD_RESUME);
                if (RT_SUCCESS(rc))
                    pStream->fStatus &= ~PDMAUDIOSTREAMSTS_FLAG_PAUSED;
            }
            break;
        }

        case PDMAUDIOSTREAMCMD_DROP:
        {
            rc = drvAudioStreamControlInternalBackend(pThis, pStream, PDMAUDIOSTREAMCMD_DROP);
            if (RT_SUCCESS(rc))
            {
                drvAudioStreamDropInternal(pThis, pStream);
            }
            break;
        }

        default:
            rc = VERR_NOT_IMPLEMENTED;
            break;
    }

    if (RT_FAILURE(rc))
        LogFunc(("[%s] Failed with %Rrc\n", pStream->szName, rc));

    return rc;
}

/**
 * Controls a stream's backend.
 * If the stream has no backend available, VERR_NOT_FOUND is returned.
 *
 * @returns IPRT status code.
 * @param   pThis               Pointer to driver instance.
 * @param   pStream             Stream to control.
 * @param   enmStreamCmd        Control command.
 */
static int drvAudioStreamControlInternalBackend(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

#ifdef LOG_ENABLED
    char *pszStreamSts = dbgAudioStreamStatusToStr(pStream->fStatus);
    LogFlowFunc(("[%s] enmStreamCmd=%s, fStatus=%s\n", pStream->szName, DrvAudioHlpStreamCmdToStr(enmStreamCmd), pszStreamSts));
    RTStrFree(pszStreamSts);
#endif /* LOG_ENABLED */

    if (!pThis->pHostDrvAudio) /* If not lower driver is configured, bail out. */
        return VINF_SUCCESS;

    LogRel2(("Audio: %s stream '%s'\n", DrvAudioHlpStreamCmdToStr(enmStreamCmd), pStream->szName));

    int rc = VINF_SUCCESS;

    switch (enmStreamCmd)
    {
        case PDMAUDIOSTREAMCMD_ENABLE:
        {
            if (!(pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_ENABLED))
                rc = pThis->pHostDrvAudio->pfnStreamControl(pThis->pHostDrvAudio, pStream->pvBackend, PDMAUDIOSTREAMCMD_ENABLE);
            break;
        }

        case PDMAUDIOSTREAMCMD_DISABLE:
        {
            if (pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_ENABLED)
                rc = pThis->pHostDrvAudio->pfnStreamControl(pThis->pHostDrvAudio, pStream->pvBackend, PDMAUDIOSTREAMCMD_DISABLE);
            break;
        }

        case PDMAUDIOSTREAMCMD_PAUSE:
        {
            /* Only pause if the stream is enabled. */
            if (!(pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_ENABLED))
                break;

            if (!(pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_PAUSED))
                rc = pThis->pHostDrvAudio->pfnStreamControl(pThis->pHostDrvAudio, pStream->pvBackend, PDMAUDIOSTREAMCMD_PAUSE);
            break;
        }

        case PDMAUDIOSTREAMCMD_RESUME:
        {
            /* Only need to resume if the stream is enabled. */
            if (!(pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_ENABLED))
                break;

            if (pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_PAUSED)
                rc = pThis->pHostDrvAudio->pfnStreamControl(pThis->pHostDrvAudio, pStream->pvBackend, PDMAUDIOSTREAMCMD_RESUME);
            break;
        }

        case PDMAUDIOSTREAMCMD_DRAIN:
        {
            rc = pThis->pHostDrvAudio->pfnStreamControl(pThis->pHostDrvAudio, pStream->pvBackend, PDMAUDIOSTREAMCMD_DRAIN);
            break;
        }

        case PDMAUDIOSTREAMCMD_DROP:
        {
            rc = pThis->pHostDrvAudio->pfnStreamControl(pThis->pHostDrvAudio, pStream->pvBackend, PDMAUDIOSTREAMCMD_DROP);
            break;
        }

        default:
        {
            AssertMsgFailed(("Command %RU32 not implemented\n", enmStreamCmd));
            rc = VERR_NOT_IMPLEMENTED;
            break;
        }
    }

    if (RT_FAILURE(rc))
    {
        if (   rc != VERR_NOT_IMPLEMENTED
            && rc != VERR_NOT_SUPPORTED)
            LogRel(("Audio: %s stream '%s' failed with %Rrc\n", DrvAudioHlpStreamCmdToStr(enmStreamCmd), pStream->szName, rc));

        LogFunc(("[%s] %s failed with %Rrc\n", pStream->szName, DrvAudioHlpStreamCmdToStr(enmStreamCmd), rc));
    }

    return rc;
}

/**
 * Initializes an audio stream with a given host and guest stream configuration.
 *
 * @returns IPRT status code.
 * @param   pThis               Pointer to driver instance.
 * @param   pStream             Stream to initialize.
 * @param   pCfgHost            Stream configuration to use for the host side (backend).
 * @param   pCfgGuest           Stream configuration to use for the guest side.
 */
static int drvAudioStreamInitInternal(PDRVAUDIO pThis,
                                      PPDMAUDIOSTREAM pStream, PPDMAUDIOSTREAMCFG pCfgHost, PPDMAUDIOSTREAMCFG pCfgGuest)
{
    AssertPtrReturn(pThis,     VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,   VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgHost,  VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgGuest, VERR_INVALID_POINTER);

    /*
     * Init host stream.
     */

    /* Set the host's default audio data layout. */
    pCfgHost->enmLayout = PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED;

#ifdef DEBUG
    LogFunc(("[%s] Requested host format:\n", pStream->szName));
    DrvAudioHlpStreamCfgPrint(pCfgHost);
#endif

    LogRel2(("Audio: Creating stream '%s'\n", pStream->szName));
    LogRel2(("Audio: Guest %s format for '%s': %RU32Hz, %RU8%s, %RU8 %s\n",
             pCfgGuest->enmDir == PDMAUDIODIR_IN ? "recording" : "playback", pStream->szName,
             pCfgGuest->Props.uHz, pCfgGuest->Props.cBytes * 8, pCfgGuest->Props.fSigned ? "S" : "U",
             pCfgGuest->Props.cChannels, pCfgGuest->Props.cChannels == 1 ? "Channel" : "Channels"));
    LogRel2(("Audio: Requested host %s format for '%s': %RU32Hz, %RU8%s, %RU8 %s\n",
             pCfgHost->enmDir == PDMAUDIODIR_IN ? "recording" : "playback", pStream->szName,
             pCfgHost->Props.uHz, pCfgHost->Props.cBytes * 8, pCfgHost->Props.fSigned ? "S" : "U",
             pCfgHost->Props.cChannels, pCfgHost->Props.cChannels == 1 ? "Channel" : "Channels"));

    PDMAUDIOSTREAMCFG CfgHostAcq;
    int rc = drvAudioStreamCreateInternalBackend(pThis, pStream, pCfgHost, &CfgHostAcq);
    if (RT_FAILURE(rc))
        return rc;

#ifdef DEBUG
    LogFunc(("[%s] Acquired host format:\n",  pStream->szName));
    DrvAudioHlpStreamCfgPrint(&CfgHostAcq);
#endif

    LogRel2(("Audio: Acquired host %s format for '%s': %RU32Hz, %RU8%s, %RU8 %s\n",
             CfgHostAcq.enmDir == PDMAUDIODIR_IN ? "recording" : "playback",  pStream->szName,
             CfgHostAcq.Props.uHz, CfgHostAcq.Props.cBytes * 8, CfgHostAcq.Props.fSigned ? "S" : "U",
             CfgHostAcq.Props.cChannels, CfgHostAcq.Props.cChannels == 1 ? "Channel" : "Channels"));

    /* Let the user know if the backend changed some of the tweakable values. */
    if (CfgHostAcq.Backend.cfBufferSize != pCfgHost->Backend.cfBufferSize)
        LogRel2(("Audio: Backend changed buffer size from %RU64ms (%RU32 frames) to %RU64ms (%RU32 frames)\n",
                 DrvAudioHlpFramesToMilli(pCfgHost->Backend.cfBufferSize, &pCfgHost->Props), pCfgHost->Backend.cfBufferSize,
                 DrvAudioHlpFramesToMilli(CfgHostAcq.Backend.cfBufferSize, &CfgHostAcq.Props), CfgHostAcq.Backend.cfBufferSize));

    if (CfgHostAcq.Backend.cfPeriod != pCfgHost->Backend.cfPeriod)
        LogRel2(("Audio: Backend changed period size from %RU64ms (%RU32 frames) to %RU64ms (%RU32 frames)\n",
                 DrvAudioHlpFramesToMilli(pCfgHost->Backend.cfPeriod, &pCfgHost->Props), pCfgHost->Backend.cfPeriod,
                 DrvAudioHlpFramesToMilli(CfgHostAcq.Backend.cfPeriod, &CfgHostAcq.Props), CfgHostAcq.Backend.cfPeriod));

    if (CfgHostAcq.Backend.cfPreBuf != pCfgHost->Backend.cfPreBuf)
        LogRel2(("Audio: Backend changed pre-buffering size from %RU64ms (%RU32 frames) to %RU64ms (%RU32 frames)\n",
                 DrvAudioHlpFramesToMilli(pCfgHost->Backend.cfPreBuf, &pCfgHost->Props), pCfgHost->Backend.cfPreBuf,
                 DrvAudioHlpFramesToMilli(CfgHostAcq.Backend.cfPreBuf, &CfgHostAcq.Props), CfgHostAcq.Backend.cfPreBuf));
    /*
     * Configure host buffers.
     */

    /* Check if the backend did return sane values and correct if necessary.
     * Should never happen with our own backends, but you never know ... */
    if (CfgHostAcq.Backend.cfBufferSize < CfgHostAcq.Backend.cfPreBuf)
    {
        LogRel2(("Audio: Warning: Pre-buffering size (%RU32 frames) of stream '%s' does not match buffer size (%RU32 frames), "
                 "setting pre-buffering size to %RU32 frames\n",
                 CfgHostAcq.Backend.cfPreBuf, pStream->szName, CfgHostAcq.Backend.cfBufferSize, CfgHostAcq.Backend.cfBufferSize));
        CfgHostAcq.Backend.cfPreBuf = CfgHostAcq.Backend.cfBufferSize;
    }

    if (CfgHostAcq.Backend.cfPeriod > CfgHostAcq.Backend.cfBufferSize)
    {
        LogRel2(("Audio: Warning: Period size (%RU32 frames) of stream '%s' does not match buffer size (%RU32 frames), setting to %RU32 frames\n",
                 CfgHostAcq.Backend.cfPeriod, pStream->szName, CfgHostAcq.Backend.cfBufferSize, CfgHostAcq.Backend.cfBufferSize));
        CfgHostAcq.Backend.cfPeriod = CfgHostAcq.Backend.cfBufferSize;
    }

    uint64_t msBufferSize = DrvAudioHlpFramesToMilli(CfgHostAcq.Backend.cfBufferSize, &CfgHostAcq.Props);

    LogRel2(("Audio: Buffer size of stream '%s' is %RU64ms (%RU32 frames)\n",
             pStream->szName, msBufferSize, CfgHostAcq.Backend.cfBufferSize));

    /* If no own pre-buffer is set, let the backend choose. */
    uint64_t msPreBuf = DrvAudioHlpFramesToMilli(CfgHostAcq.Backend.cfPreBuf, &CfgHostAcq.Props);
    LogRel2(("Audio: Pre-buffering size of stream '%s' is %RU64ms (%RU32 frames)\n",
             pStream->szName, msPreBuf, CfgHostAcq.Backend.cfPreBuf));

    /* Make sure the configured buffer size by the backend at least can hold the configured latency. */
    const uint32_t msPeriod = DrvAudioHlpFramesToMilli(CfgHostAcq.Backend.cfPeriod, &CfgHostAcq.Props);

    LogRel2(("Audio: Period size of stream '%s' is %RU64ms (%RU32 frames)\n",
             pStream->szName, msPeriod, CfgHostAcq.Backend.cfPeriod));

    if (   pCfgGuest->Device.uSchedulingHintMs             /* Any scheduling hint set? */
        && pCfgGuest->Device.uSchedulingHintMs > msPeriod) /* This might lead to buffer underflows. */
    {
        LogRel(("Audio: Warning: Scheduling hint of stream '%s' is bigger (%RU64ms) than used period size (%RU64ms)\n",
                pStream->szName, pCfgGuest->Device.uSchedulingHintMs, msPeriod));
    }

    /* Destroy any former mixing buffer. */
    AudioMixBufDestroy(&pStream->Host.MixBuf);

    /* Make sure to (re-)set the host buffer's shift size. */
    CfgHostAcq.Props.cShift = PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(CfgHostAcq.Props.cBytes, CfgHostAcq.Props.cChannels);

    rc = AudioMixBufInit(&pStream->Host.MixBuf, pStream->szName, &CfgHostAcq.Props, CfgHostAcq.Backend.cfBufferSize);
    AssertRC(rc);

    /* Make a copy of the acquired host stream configuration. */
    rc = DrvAudioHlpStreamCfgCopy(&pStream->Host.Cfg, &CfgHostAcq);
    AssertRC(rc);

    /*
     * Init guest stream.
     */

    if (pCfgGuest->Device.uSchedulingHintMs)
        LogRel2(("Audio: Stream '%s' got a scheduling hint of %RU32ms (%RU32 bytes)\n",
                 pStream->szName, pCfgGuest->Device.uSchedulingHintMs,
                 DrvAudioHlpMilliToBytes(pCfgGuest->Device.uSchedulingHintMs, &pCfgGuest->Props)));

    /* Destroy any former mixing buffer. */
    AudioMixBufDestroy(&pStream->Guest.MixBuf);

    /* Set the guests's default audio data layout. */
    pCfgGuest->enmLayout = PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED;

    /* Make sure to (re-)set the guest buffer's shift size. */
    pCfgGuest->Props.cShift = PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(pCfgGuest->Props.cBytes, pCfgGuest->Props.cChannels);

    rc = AudioMixBufInit(&pStream->Guest.MixBuf, pStream->szName, &pCfgGuest->Props, CfgHostAcq.Backend.cfBufferSize);
    AssertRC(rc);

    /* Make a copy of the guest stream configuration. */
    rc = DrvAudioHlpStreamCfgCopy(&pStream->Guest.Cfg, pCfgGuest);
    AssertRC(rc);

    if (RT_FAILURE(rc))
        LogRel(("Audio: Creating stream '%s' failed with %Rrc\n", pStream->szName, rc));

    if (pCfgGuest->enmDir == PDMAUDIODIR_IN)
    {
        /* Host (Parent) -> Guest (Child). */
        rc = AudioMixBufLinkTo(&pStream->Host.MixBuf, &pStream->Guest.MixBuf);
        AssertRC(rc);
    }
    else
    {
        /* Guest (Parent) -> Host (Child). */
        rc = AudioMixBufLinkTo(&pStream->Guest.MixBuf, &pStream->Host.MixBuf);
        AssertRC(rc);
    }

#ifdef VBOX_WITH_STATISTICS
    char szStatName[255];

    if (pCfgGuest->enmDir == PDMAUDIODIR_IN)
    {
        RTStrPrintf(szStatName, sizeof(szStatName), "Guest/%s/TotalFramesCaptured", pStream->szName);
        PDMDrvHlpSTAMRegCounterEx(pThis->pDrvIns, &pStream->In.Stats.TotalFramesCaptured,
                                  szStatName, STAMUNIT_COUNT, "Total frames played.");
        RTStrPrintf(szStatName, sizeof(szStatName), "Guest/%s/TotalTimesCaptured", pStream->szName);
        PDMDrvHlpSTAMRegCounterEx(pThis->pDrvIns, &pStream->In.Stats.TotalTimesCaptured,
                                  szStatName, STAMUNIT_COUNT, "Total number of playbacks.");
        RTStrPrintf(szStatName, sizeof(szStatName), "Guest/%s/TotalFramesRead", pStream->szName);
        PDMDrvHlpSTAMRegCounterEx(pThis->pDrvIns, &pStream->In.Stats.TotalFramesRead,
                                  szStatName, STAMUNIT_COUNT, "Total frames read.");
        RTStrPrintf(szStatName, sizeof(szStatName), "Guest/%s/TotalTimesRead", pStream->szName);
        PDMDrvHlpSTAMRegCounterEx(pThis->pDrvIns, &pStream->In.Stats.TotalTimesRead,
                                  szStatName, STAMUNIT_COUNT, "Total number of reads.");
    }
    else if (pCfgGuest->enmDir == PDMAUDIODIR_OUT)
    {
        RTStrPrintf(szStatName, sizeof(szStatName), "Guest/%s/TotalFramesPlayed", pStream->szName);
        PDMDrvHlpSTAMRegCounterEx(pThis->pDrvIns, &pStream->Out.Stats.TotalFramesPlayed,
                                  szStatName, STAMUNIT_COUNT, "Total frames played.");

        RTStrPrintf(szStatName, sizeof(szStatName), "Guest/%s/TotalTimesPlayed", pStream->szName);
        PDMDrvHlpSTAMRegCounterEx(pThis->pDrvIns, &pStream->Out.Stats.TotalTimesPlayed,
                                  szStatName, STAMUNIT_COUNT, "Total number of playbacks.");
        RTStrPrintf(szStatName, sizeof(szStatName), "Guest/%s/TotalFramesWritten", pStream->szName);
        PDMDrvHlpSTAMRegCounterEx(pThis->pDrvIns, &pStream->Out.Stats.TotalFramesWritten,
                                  szStatName, STAMUNIT_COUNT, "Total frames written.");

        RTStrPrintf(szStatName, sizeof(szStatName), "Guest/%s/TotalTimesWritten", pStream->szName);
        PDMDrvHlpSTAMRegCounterEx(pThis->pDrvIns, &pStream->Out.Stats.TotalTimesWritten,
                                  szStatName, STAMUNIT_COUNT, "Total number of writes.");
    }
    else
        AssertFailed();
#endif

    LogFlowFunc(("[%s] Returning %Rrc\n", pStream->szName, rc));
    return rc;
}

/**
 * Frees an audio stream and its allocated resources.
 *
 * @param   pStream             Audio stream to free.
 *                              After this call the pointer will not be valid anymore.
 */
static void drvAudioStreamFree(PPDMAUDIOSTREAM pStream)
{
    if (!pStream)
        return;

    LogFunc(("[%s]\n", pStream->szName));

    if (pStream->pvBackend)
    {
        Assert(pStream->cbBackend);
        RTMemFree(pStream->pvBackend);
        pStream->pvBackend = NULL;
    }

    RTMemFree(pStream);
    pStream = NULL;
}

#ifdef VBOX_WITH_AUDIO_CALLBACKS
/**
 * Schedules a re-initialization of all current audio streams.
 * The actual re-initialization will happen at some later point in time.
 *
 * @returns IPRT status code.
 * @param   pThis               Pointer to driver instance.
 */
static int drvAudioScheduleReInitInternal(PDRVAUDIO pThis)
{
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);

    LogFunc(("\n"));

    /* Mark all host streams to re-initialize. */
    PPDMAUDIOSTREAM pStream;
    RTListForEach(&pThis->lstStreams, pStream, PDMAUDIOSTREAM, Node)
        pStream->fStatus |= PDMAUDIOSTREAMSTS_FLAG_PENDING_REINIT;

# ifdef VBOX_WITH_AUDIO_ENUM
    /* Re-enumerate all host devices as soon as possible. */
    pThis->fEnumerateDevices = true;
# endif

    return VINF_SUCCESS;
}
#endif /* VBOX_WITH_AUDIO_CALLBACKS */

/**
 * Re-initializes an audio stream with its existing host and guest stream configuration.
 * This might be the case if the backend told us we need to re-initialize because something
 * on the host side has changed.
 *
 * @returns IPRT status code.
 * @param   pThis               Pointer to driver instance.
 * @param   pStream             Stream to re-initialize.
 */
static int drvAudioStreamReInitInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    LogFlowFunc(("[%s]\n", pStream->szName));

    /*
     * Gather current stream status.
     */
    bool fIsEnabled = RT_BOOL(pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_ENABLED); /* Stream is enabled? */

    /*
     * Destroy and re-create stream on backend side.
     */
    int rc = drvAudioStreamControlInternalBackend(pThis, pStream, PDMAUDIOSTREAMCMD_DISABLE);
    if (RT_SUCCESS(rc))
    {
        rc = drvAudioStreamDestroyInternalBackend(pThis, pStream);
        if (RT_SUCCESS(rc))
        {
            rc = drvAudioStreamCreateInternalBackend(pThis, pStream, &pStream->Host.Cfg, NULL /* pCfgAcq */);
            /** @todo Validate (re-)acquired configuration with pStream->Host.Cfg? */
        }
    }

    /* Drop all old data. */
    drvAudioStreamDropInternal(pThis, pStream);

    /*
     * Restore previous stream state.
     */
    if (fIsEnabled)
        rc = drvAudioStreamControlInternalBackend(pThis, pStream, PDMAUDIOSTREAMCMD_ENABLE);

    if (RT_FAILURE(rc))
        LogRel(("Audio: Re-initializing stream '%s' failed with %Rrc\n", pStream->szName, rc));

    LogFunc(("[%s] Returning %Rrc\n", pStream->szName, rc));
    return rc;
}

/**
 * Drops all audio data (and associated state) of a stream.
 *
 * @param   pThis               Pointer to driver instance.
 * @param   pStream             Stream to drop data for.
 */
static void drvAudioStreamDropInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream)
{
    RT_NOREF(pThis);

    LogFunc(("[%s]\n", pStream->szName));

    AudioMixBufReset(&pStream->Guest.MixBuf);
    AudioMixBufReset(&pStream->Host.MixBuf);

    pStream->tsLastIteratedNs       = 0;
    pStream->tsLastPlayedCapturedNs = 0;
    pStream->tsLastReadWrittenNs    = 0;

    pStream->fThresholdReached = false;
}

/**
 * Resets a given audio stream.
 *
 * @param   pThis               Pointer to driver instance.
 * @param   pStream             Stream to reset.
 */
static void drvAudioStreamResetInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream)
{
    drvAudioStreamDropInternal(pThis, pStream);

    LogFunc(("[%s]\n", pStream->szName));

    pStream->fStatus = PDMAUDIOSTREAMSTS_FLAG_INITIALIZED;

#ifdef VBOX_WITH_STATISTICS
    /*
     * Reset statistics.
     */
    if (pStream->enmDir == PDMAUDIODIR_IN)
    {
        STAM_COUNTER_RESET(&pStream->In.Stats.TotalFramesCaptured);
        STAM_COUNTER_RESET(&pStream->In.Stats.TotalFramesRead);
        STAM_COUNTER_RESET(&pStream->In.Stats.TotalTimesCaptured);
        STAM_COUNTER_RESET(&pStream->In.Stats.TotalTimesRead);
    }
    else if (pStream->enmDir == PDMAUDIODIR_OUT)
    {
        STAM_COUNTER_RESET(&pStream->Out.Stats.TotalFramesPlayed);
        STAM_COUNTER_RESET(&pStream->Out.Stats.TotalFramesWritten);
        STAM_COUNTER_RESET(&pStream->Out.Stats.TotalTimesPlayed);
        STAM_COUNTER_RESET(&pStream->Out.Stats.TotalTimesWritten);
    }
    else
        AssertFailed();
#endif
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamWrite}
 */
static DECLCALLBACK(int) drvAudioStreamWrite(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream,
                                             const void *pvBuf, uint32_t cbBuf, uint32_t *pcbWritten)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    AssertPtrReturn(pvBuf,      VERR_INVALID_POINTER);
    AssertReturn(cbBuf,         VERR_INVALID_PARAMETER);
    /* pcbWritten is optional. */

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    AssertMsg(pStream->enmDir == PDMAUDIODIR_OUT,
              ("Stream '%s' is not an output stream and therefore cannot be written to (direction is '%s')\n",
               pStream->szName, DrvAudioHlpAudDirToStr(pStream->enmDir)));

    AssertMsg(DrvAudioHlpBytesIsAligned(cbBuf, &pStream->Guest.Cfg.Props),
              ("Stream '%s' got a non-frame-aligned write (%RU32 bytes)\n", pStream->szName, cbBuf));

    uint32_t cbWrittenTotal = 0;

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return rc;

#ifdef VBOX_WITH_STATISTICS
    STAM_PROFILE_ADV_START(&pThis->Stats.DelayOut, out);
#endif

#ifdef LOG_ENABLED
    char *pszStreamSts = dbgAudioStreamStatusToStr(pStream->fStatus);
    AssertPtr(pszStreamSts);
#endif

    /* Whether to discard the incoming data or not. */
    bool fToBitBucket = false;

    do
    {
        if (   !pThis->Out.fEnabled
            || !DrvAudioHlpStreamStatusIsReady(pStream->fStatus))
        {
            rc = VERR_AUDIO_STREAM_NOT_READY;
            break;
        }

        if (pThis->pHostDrvAudio)
        {
            /* If the backend's stream is not writable, all written data goes to /dev/null. */
            if (!DrvAudioHlpStreamStatusCanWrite(
                pThis->pHostDrvAudio->pfnStreamGetStatus(pThis->pHostDrvAudio, pStream->pvBackend)))
            {
                fToBitBucket = true;
                break;
            }
        }

        const uint32_t cbFree = AudioMixBufFreeBytes(&pStream->Host.MixBuf);
        if (cbFree < cbBuf)
            LogRel2(("Audio: Lost audio output (%RU64ms, %RU32 free but needs %RU32) due to full host stream buffer '%s'\n",
                     DrvAudioHlpBytesToMilli(cbBuf - cbFree, &pStream->Host.Cfg.Props), cbFree, cbBuf, pStream->szName));

        uint32_t cbToWrite = RT_MIN(cbBuf, cbFree);
        if (!cbToWrite)
        {
            rc = VERR_BUFFER_OVERFLOW;
            break;
        }

        /* We use the guest side mixing buffer as an intermediate buffer to do some
         * (first) processing (if needed), so always write the incoming data at offset 0. */
        uint32_t cfGstWritten = 0;
        rc = AudioMixBufWriteAt(&pStream->Guest.MixBuf, 0 /* offFrames */, pvBuf, cbToWrite, &cfGstWritten);
        if (   RT_FAILURE(rc)
            || !cfGstWritten)
        {
            AssertMsgFailed(("[%s] Write failed: cbToWrite=%RU32, cfWritten=%RU32, rc=%Rrc\n",
                             pStream->szName, cbToWrite, cfGstWritten, rc));
            break;
        }

        if (pThis->Out.Cfg.Dbg.fEnabled)
            DrvAudioHlpFileWrite(pStream->Out.Dbg.pFileStreamWrite, pvBuf, cbToWrite, 0 /* fFlags */);

        uint32_t cfGstMixed = 0;
        if (cfGstWritten)
        {
            int rc2 = AudioMixBufMixToParentEx(&pStream->Guest.MixBuf, 0 /* cSrcOffset */, cfGstWritten /* cSrcFrames */,
                                               &cfGstMixed /* pcSrcMixed */);
            if (RT_FAILURE(rc2))
            {
                AssertMsgFailed(("[%s] Mixing failed: cbToWrite=%RU32, cfWritten=%RU32, cfMixed=%RU32, rc=%Rrc\n",
                                 pStream->szName, cbToWrite, cfGstWritten, cfGstMixed, rc2));
            }
            else
            {
                const uint64_t tsNowNs = RTTimeNanoTS();

                Log3Func(("[%s] Writing %RU32 frames (%RU64ms)\n",
                          pStream->szName, cfGstWritten, DrvAudioHlpFramesToMilli(cfGstWritten, &pStream->Guest.Cfg.Props)));

                Log3Func(("[%s] Last written %RU64ns (%RU64ms), now filled with %RU64ms -- %RU8%%\n",
                          pStream->szName, tsNowNs - pStream->tsLastReadWrittenNs,
                          (tsNowNs - pStream->tsLastReadWrittenNs) / RT_NS_1MS,
                          DrvAudioHlpFramesToMilli(AudioMixBufUsed(&pStream->Host.MixBuf), &pStream->Host.Cfg.Props),
                          AudioMixBufUsed(&pStream->Host.MixBuf) * 100 / AudioMixBufSize(&pStream->Host.MixBuf)));

                pStream->tsLastReadWrittenNs = tsNowNs;
                /* Keep going. */
            }

            if (RT_SUCCESS(rc))
                rc = rc2;

            cbWrittenTotal = AUDIOMIXBUF_F2B(&pStream->Guest.MixBuf, cfGstWritten);

#ifdef VBOX_WITH_STATISTICS
            STAM_COUNTER_ADD(&pThis->Stats.TotalFramesWritten,  cfGstWritten);
            STAM_COUNTER_ADD(&pThis->Stats.TotalFramesMixedOut, cfGstMixed);
            Assert(cfGstWritten >= cfGstMixed);
            STAM_COUNTER_ADD(&pThis->Stats.TotalFramesLostOut,  cfGstWritten - cfGstMixed);
            STAM_COUNTER_ADD(&pThis->Stats.TotalBytesWritten,   cbWrittenTotal);

            STAM_COUNTER_ADD(&pStream->Out.Stats.TotalFramesWritten, cfGstWritten);
            STAM_COUNTER_INC(&pStream->Out.Stats.TotalTimesWritten);
#endif
        }

        Log3Func(("[%s] Dbg: cbBuf=%RU32, cbToWrite=%RU32, cfHstUsed=%RU32, cfHstfLive=%RU32, cfGstWritten=%RU32, "
                  "cfGstMixed=%RU32, cbWrittenTotal=%RU32, rc=%Rrc\n",
                  pStream->szName, cbBuf, cbToWrite, AudioMixBufUsed(&pStream->Host.MixBuf),
                  AudioMixBufLive(&pStream->Host.MixBuf), cfGstWritten, cfGstMixed, cbWrittenTotal, rc));

    } while (0);

#ifdef LOG_ENABLED
    RTStrFree(pszStreamSts);
#endif

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    if (RT_SUCCESS(rc))
    {
        if (fToBitBucket)
        {
            Log3Func(("[%s] Backend stream not ready (yet), discarding written data\n", pStream->szName));
            cbWrittenTotal = cbBuf; /* Report all data as being written to the caller. */
        }

        if (pcbWritten)
            *pcbWritten = cbWrittenTotal;
    }

    return rc;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamRetain}
 */
static DECLCALLBACK(uint32_t) drvAudioStreamRetain(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream)
{
   AssertPtrReturn(pInterface, UINT32_MAX);
   AssertPtrReturn(pStream,    UINT32_MAX);

   NOREF(pInterface);

   return ++pStream->cRefs;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamRelease}
 */
static DECLCALLBACK(uint32_t) drvAudioStreamRelease(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream)
{
   AssertPtrReturn(pInterface, UINT32_MAX);
   AssertPtrReturn(pStream,    UINT32_MAX);

   NOREF(pInterface);

   if (pStream->cRefs > 1) /* 1 reference always is kept by this audio driver. */
       pStream->cRefs--;

   return pStream->cRefs;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamIterate}
 */
static DECLCALLBACK(int) drvAudioStreamIterate(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    rc = drvAudioStreamIterateInternal(pThis, pStream);

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    if (RT_FAILURE(rc))
        LogFlowFuncLeaveRC(rc);

    return rc;
}

/**
 * Does one iteration of an audio stream.
 * This function gives the backend the chance of iterating / altering data and
 * does the actual mixing between the guest <-> host mixing buffers.
 *
 * @returns IPRT status code.
 * @param   pThis               Pointer to driver instance.
 * @param   pStream             Stream to iterate.
 */
static int drvAudioStreamIterateInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);

    if (!pThis->pHostDrvAudio)
        return VINF_SUCCESS;

    if (!pStream)
        return VINF_SUCCESS;

    int rc;

    /* Is the stream scheduled for re-initialization? Do so now. */
    if (pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_PENDING_REINIT)
    {
#ifdef VBOX_WITH_AUDIO_ENUM
        if (pThis->fEnumerateDevices)
        {
            /* Re-enumerate all host devices. */
            drvAudioDevicesEnumerateInternal(pThis, true /* fLog */, NULL /* pDevEnum */);

            pThis->fEnumerateDevices = false;
        }
#endif /* VBOX_WITH_AUDIO_ENUM */

        /* Remove the pending re-init flag in any case, regardless whether the actual re-initialization succeeded
         * or not. If it failed, the backend needs to notify us again to try again at some later point in time. */
        pStream->fStatus &= ~PDMAUDIOSTREAMSTS_FLAG_PENDING_REINIT;

        rc = drvAudioStreamReInitInternal(pThis, pStream);
        if (RT_FAILURE(rc))
            return rc;
    }

#ifdef LOG_ENABLED
    char *pszStreamSts = dbgAudioStreamStatusToStr(pStream->fStatus);
    Log3Func(("[%s] fStatus=%s\n", pStream->szName, pszStreamSts));
    RTStrFree(pszStreamSts);
#endif /* LOG_ENABLED */

    /* Not enabled or paused? Skip iteration. */
    if (   !(pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_ENABLED)
        ||  (pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_PAUSED))
    {
        return VINF_SUCCESS;
    }

    /* Whether to try closing a pending to close stream. */
    bool fTryClosePending = false;

    do
    {
        rc = pThis->pHostDrvAudio->pfnStreamIterate(pThis->pHostDrvAudio, pStream->pvBackend);
        if (RT_FAILURE(rc))
            break;

        if (pStream->enmDir == PDMAUDIODIR_OUT)
        {
            /* No audio frames to transfer from guest to host (anymore)?
             * Then try closing this stream if marked so in the next block. */
            const uint32_t cfLive = AudioMixBufLive(&pStream->Host.MixBuf);
            fTryClosePending = cfLive == 0;
            Log3Func(("[%s] fTryClosePending=%RTbool, cfLive=%RU32\n", pStream->szName, fTryClosePending, cfLive));
        }

        /* Has the host stream marked as pending to disable?
         * Try disabling the stream then. */
        if (   pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_PENDING_DISABLE
            && fTryClosePending)
        {
            /* Tell the backend to drain the stream, that is, play the remaining (buffered) data
             * on the backend side. */
            rc = drvAudioStreamControlInternalBackend(pThis, pStream, PDMAUDIOSTREAMCMD_DRAIN);
            if (rc == VERR_NOT_SUPPORTED) /* Not all backends support draining. */
                rc = VINF_SUCCESS;

            if (RT_SUCCESS(rc))
            {
                if (pThis->pHostDrvAudio->pfnStreamGetPending) /* Optional to implement. */
                {
                    const uint32_t cxPending = pThis->pHostDrvAudio->pfnStreamGetPending(pThis->pHostDrvAudio, pStream->pvBackend);
                    Log3Func(("[%s] cxPending=%RU32\n", pStream->szName, cxPending));

                    /* Only try close pending if no audio data is pending on the backend-side anymore. */
                    fTryClosePending = cxPending == 0;
                }

                if (fTryClosePending)
                {
                    LogFunc(("[%s] Closing pending stream\n", pStream->szName));
                    rc = drvAudioStreamControlInternalBackend(pThis, pStream, PDMAUDIOSTREAMCMD_DISABLE);
                    if (RT_SUCCESS(rc))
                    {
                        pStream->fStatus &= ~(PDMAUDIOSTREAMSTS_FLAG_ENABLED | PDMAUDIOSTREAMSTS_FLAG_PENDING_DISABLE);
                        drvAudioStreamDropInternal(pThis, pStream);
                    }
                    else
                       LogFunc(("[%s] Backend vetoed against closing pending input stream, rc=%Rrc\n", pStream->szName, rc));
                }
            }
        }

    } while (0);

    /* Update timestamps. */
    pStream->tsLastIteratedNs = RTTimeNanoTS();

    if (RT_FAILURE(rc))
        LogFunc(("[%s] Failed with %Rrc\n",  pStream->szName, rc));

    return rc;
}

/**
 * Plays an audio host output stream which has been configured for non-interleaved (layout) data.
 *
 * @return  IPRT status code.
 * @param   pThis               Pointer to driver instance.
 * @param   pStream             Stream to play.
 * @param   cfToPlay            Number of audio frames to play.
 * @param   pcfPlayed           Returns number of audio frames played. Optional.
 */
static int drvAudioStreamPlayNonInterleaved(PDRVAUDIO pThis,
                                            PPDMAUDIOSTREAM pStream, uint32_t cfToPlay, uint32_t *pcfPlayed)
{
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);
    /* pcfPlayed is optional. */

    if (!cfToPlay)
    {
        if (pcfPlayed)
            *pcfPlayed = 0;
        return VINF_SUCCESS;
    }

    /* Sanity. */
    Assert(pStream->enmDir == PDMAUDIODIR_OUT);
    Assert(pStream->Host.Cfg.enmLayout == PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED);

    int rc = VINF_SUCCESS;

    uint32_t cfPlayedTotal = 0;

    uint8_t auBuf[256]; /** @todo Get rid of this here. */

    uint32_t cfLeft  = cfToPlay;
    uint32_t cbChunk = sizeof(auBuf);

    while (cfLeft)
    {
        uint32_t cfRead = 0;
        rc = AudioMixBufAcquireReadBlock(&pStream->Host.MixBuf,
                                         auBuf, RT_MIN(cbChunk, AUDIOMIXBUF_F2B(&pStream->Host.MixBuf, cfLeft)),
                                         &cfRead);
        if (RT_FAILURE(rc))
            break;

        uint32_t cbRead = AUDIOMIXBUF_F2B(&pStream->Host.MixBuf, cfRead);
        Assert(cbRead <= cbChunk);

        uint32_t cfPlayed = 0;
        uint32_t cbPlayed = 0;
        rc = pThis->pHostDrvAudio->pfnStreamPlay(pThis->pHostDrvAudio, pStream->pvBackend,
                                                 auBuf, cbRead, &cbPlayed);
        if (   RT_SUCCESS(rc)
            && cbPlayed)
        {
            if (pThis->Out.Cfg.Dbg.fEnabled)
                DrvAudioHlpFileWrite(pStream->Out.Dbg.pFilePlayNonInterleaved, auBuf, cbPlayed, 0 /* fFlags */);

            if (cbRead != cbPlayed)
                LogRel2(("Audio: Host stream '%s' played wrong amount (%RU32 bytes read but played %RU32)\n",
                         pStream->szName, cbRead, cbPlayed));

            cfPlayed       = AUDIOMIXBUF_B2F(&pStream->Host.MixBuf, cbPlayed);
            cfPlayedTotal += cfPlayed;

            Assert(cfLeft >= cfPlayed);
            cfLeft        -= cfPlayed;
        }

        AudioMixBufReleaseReadBlock(&pStream->Host.MixBuf, cfPlayed);

        if (RT_FAILURE(rc))
            break;
    }

    Log3Func(("[%s] Played %RU32/%RU32 frames, rc=%Rrc\n", pStream->szName, cfPlayedTotal, cfToPlay, rc));

    if (RT_SUCCESS(rc))
    {
        if (pcfPlayed)
            *pcfPlayed = cfPlayedTotal;
    }

    return rc;
}

/**
 * Plays an audio host output stream which has been configured for raw audio (layout) data.
 *
 * @return  IPRT status code.
 * @param   pThis               Pointer to driver instance.
 * @param   pStream             Stream to play.
 * @param   cfToPlay            Number of audio frames to play.
 * @param   pcfPlayed           Returns number of audio frames played. Optional.
 */
static int drvAudioStreamPlayRaw(PDRVAUDIO pThis,
                                 PPDMAUDIOSTREAM pStream, uint32_t cfToPlay, uint32_t *pcfPlayed)
{
    AssertPtrReturn(pThis,      VERR_INVALID_POINTER);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);
    /* pcfPlayed is optional. */

    /* Sanity. */
    Assert(pStream->enmDir == PDMAUDIODIR_OUT);
    Assert(pStream->Host.Cfg.enmLayout == PDMAUDIOSTREAMLAYOUT_RAW);

    if (!cfToPlay)
    {
        if (pcfPlayed)
            *pcfPlayed = 0;
        return VINF_SUCCESS;
    }

    int rc = VINF_SUCCESS;

    uint32_t cfPlayedTotal = 0;

    PDMAUDIOFRAME aFrameBuf[_4K]; /** @todo Get rid of this here. */

    uint32_t cfLeft = cfToPlay;
    while (cfLeft)
    {
        uint32_t cfRead = 0;
        rc = AudioMixBufPeek(&pStream->Host.MixBuf, cfLeft, aFrameBuf,
                             RT_MIN(cfLeft, RT_ELEMENTS(aFrameBuf)), &cfRead);

        if (RT_SUCCESS(rc))
        {
            if (cfRead)
            {
                uint32_t cfPlayed;

                /* Note: As the stream layout is RPDMAUDIOSTREAMLAYOUT_RAW, operate on audio frames
                 *       rather on bytes. */
                Assert(cfRead <= RT_ELEMENTS(aFrameBuf));
                rc = pThis->pHostDrvAudio->pfnStreamPlay(pThis->pHostDrvAudio, pStream->pvBackend,
                                                         aFrameBuf, cfRead, &cfPlayed);
                if (   RT_FAILURE(rc)
                    || !cfPlayed)
                {
                    break;
                }

                cfPlayedTotal += cfPlayed;
                Assert(cfPlayedTotal <= cfToPlay);

                Assert(cfLeft >= cfRead);
                cfLeft        -= cfRead;
            }
            else
            {
                if (rc == VINF_AUDIO_MORE_DATA_AVAILABLE) /* Do another peeking round if there is more data available. */
                    continue;

                break;
            }
        }
        else if (RT_FAILURE(rc))
            break;
    }

    Log3Func(("[%s] Played %RU32/%RU32 frames, rc=%Rrc\n", pStream->szName, cfPlayedTotal, cfToPlay, rc));

    if (RT_SUCCESS(rc))
    {
        if (pcfPlayed)
            *pcfPlayed = cfPlayedTotal;

    }

    return rc;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamPlay}
 */
static DECLCALLBACK(int) drvAudioStreamPlay(PPDMIAUDIOCONNECTOR pInterface,
                                            PPDMAUDIOSTREAM pStream, uint32_t *pcFramesPlayed)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    /* pcFramesPlayed is optional. */

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    AssertMsg(pStream->enmDir == PDMAUDIODIR_OUT,
              ("Stream '%s' is not an output stream and therefore cannot be played back (direction is 0x%x)\n",
               pStream->szName, pStream->enmDir));

    uint32_t cfPlayedTotal = 0;

    PDMAUDIOSTREAMSTS stsStream = pStream->fStatus;
#ifdef LOG_ENABLED
    char *pszStreamSts = dbgAudioStreamStatusToStr(stsStream);
    Log3Func(("[%s] Start fStatus=%s\n", pStream->szName, pszStreamSts));
    RTStrFree(pszStreamSts);
#endif /* LOG_ENABLED */

    do
    {
        if (!pThis->pHostDrvAudio)
        {
            rc = VERR_PDM_NO_ATTACHED_DRIVER;
            break;
        }

        if (   !pThis->Out.fEnabled
            || !DrvAudioHlpStreamStatusIsReady(stsStream))
        {
            rc = VERR_AUDIO_STREAM_NOT_READY;
            break;
        }

        const uint32_t cfLive       = AudioMixBufLive(&pStream->Host.MixBuf);
#ifdef LOG_ENABLED
        const uint8_t  uLivePercent = (100 * cfLive) / AudioMixBufSize(&pStream->Host.MixBuf);
#endif
        const uint64_t tsDeltaPlayedCapturedNs = RTTimeNanoTS() - pStream->tsLastPlayedCapturedNs;
        const uint32_t cfPassedReal            = DrvAudioHlpNanoToFrames(tsDeltaPlayedCapturedNs, &pStream->Host.Cfg.Props);

        const uint32_t cfPeriod     = pStream->Host.Cfg.Backend.cfPeriod;

        Log3Func(("[%s] Last played %RU64ns (%RU64ms), filled with %RU64ms (%RU8%%) total (fThresholdReached=%RTbool)\n",
                  pStream->szName, tsDeltaPlayedCapturedNs, tsDeltaPlayedCapturedNs / RT_NS_1MS_64,
                  DrvAudioHlpFramesToMilli(cfLive, &pStream->Host.Cfg.Props), uLivePercent, pStream->fThresholdReached));

        if (   pStream->fThresholdReached         /* Has the treshold been reached (e.g. are we in playing stage) ... */
            && cfLive == 0)                       /* ... and we now have no live samples to process? */
        {
            LogRel2(("Audio: Buffer underrun for stream '%s' occurred (%RU64ms passed)\n",
                     pStream->szName, DrvAudioHlpFramesToMilli(cfPassedReal, &pStream->Host.Cfg.Props)));

            if (pStream->Host.Cfg.Backend.cfPreBuf) /* Any pre-buffering configured? */
            {
                /* Enter pre-buffering stage again. */
                pStream->fThresholdReached = false;
            }
        }

        bool fDoPlay      = pStream->fThresholdReached;
        bool fJustStarted = false;
        if (!fDoPlay)
        {
            /* Did we reach the backend's playback (pre-buffering) threshold? Can be 0 if no threshold set. */
            if (cfLive >= pStream->Host.Cfg.Backend.cfPreBuf)
            {
                LogRel2(("Audio: Stream '%s' buffering complete\n", pStream->szName));
                Log3Func(("[%s] Dbg: Buffering complete\n", pStream->szName));
                fDoPlay = true;
            }
            /* Some audio files are shorter than the pre-buffering level (e.g. the "click" Explorer sounds on some Windows guests),
             * so make sure that we also play those by checking if the stream already is pending disable mode, even if we didn't
             * hit the pre-buffering watermark yet.
             *
             * To reproduce, use "Windows Navigation Start.wav" on Windows 7 (2824 samples). */
            else if (   cfLive
                     && pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_PENDING_DISABLE)
            {
                LogRel2(("Audio: Stream '%s' buffering complete (short sound)\n", pStream->szName));
                Log3Func(("[%s] Dbg: Buffering complete (short)\n", pStream->szName));
                fDoPlay = true;
            }

            if (fDoPlay)
            {
                pStream->fThresholdReached = true;
                fJustStarted = true;
                LogRel2(("Audio: Stream '%s' started playing\n", pStream->szName));
                Log3Func(("[%s] Dbg: started playing\n", pStream->szName));
            }
            else /* Not yet, so still buffering audio data. */
                LogRel2(("Audio: Stream '%s' is buffering (%RU8%% complete)\n",
                         pStream->szName, (100 * cfLive) / pStream->Host.Cfg.Backend.cfPreBuf));
        }

        if (fDoPlay)
        {
            uint32_t cfWritable = PDMAUDIOPCMPROPS_B2F(&pStream->Host.Cfg.Props,
                                                       pThis->pHostDrvAudio->pfnStreamGetWritable(pThis->pHostDrvAudio, pStream->pvBackend));

            uint32_t cfToPlay = 0;
            if (fJustStarted)
                cfToPlay = RT_MIN(cfWritable, cfPeriod);

            if (!cfToPlay)
            {
                /* Did we reach/pass (in real time) the device scheduling slot?
                 * Play as much as we can write to the backend then. */
                if (cfPassedReal >= DrvAudioHlpMilliToFrames(pStream->Guest.Cfg.Device.uSchedulingHintMs, &pStream->Host.Cfg.Props))
                    cfToPlay = cfWritable;
            }

            if (cfToPlay > cfLive) /* Don't try to play more than available. */
                cfToPlay = cfLive;
#ifdef DEBUG
            Log3Func(("[%s] Playing %RU32 frames (%RU64ms), now filled with %RU64ms -- %RU8%%\n",
                      pStream->szName, cfToPlay, DrvAudioHlpFramesToMilli(cfToPlay, &pStream->Host.Cfg.Props),
                      DrvAudioHlpFramesToMilli(AudioMixBufUsed(&pStream->Host.MixBuf), &pStream->Host.Cfg.Props),
                      AudioMixBufUsed(&pStream->Host.MixBuf) * 100 / AudioMixBufSize(&pStream->Host.MixBuf)));
#endif
            if (cfToPlay)
            {
                if (pThis->pHostDrvAudio->pfnStreamPlayBegin)
                    pThis->pHostDrvAudio->pfnStreamPlayBegin(pThis->pHostDrvAudio, pStream->pvBackend);

                if (RT_LIKELY(pStream->Host.Cfg.enmLayout == PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED))
                {
                    rc = drvAudioStreamPlayNonInterleaved(pThis, pStream, cfToPlay, &cfPlayedTotal);
                }
                else if (pStream->Host.Cfg.enmLayout == PDMAUDIOSTREAMLAYOUT_RAW)
                {
                    rc = drvAudioStreamPlayRaw(pThis, pStream, cfToPlay, &cfPlayedTotal);
                }
                else
                    AssertFailedStmt(rc = VERR_NOT_IMPLEMENTED);

                if (pThis->pHostDrvAudio->pfnStreamPlayEnd)
                    pThis->pHostDrvAudio->pfnStreamPlayEnd(pThis->pHostDrvAudio, pStream->pvBackend);

                pStream->tsLastPlayedCapturedNs = RTTimeNanoTS();
            }

            Log3Func(("[%s] Dbg: fJustStarted=%RTbool, cfSched=%RU32 (%RU64ms), cfPassedReal=%RU32 (%RU64ms), "
                      "cfLive=%RU32 (%RU64ms), cfPeriod=%RU32 (%RU64ms), cfWritable=%RU32 (%RU64ms), "
                      "-> cfToPlay=%RU32 (%RU64ms), cfPlayed=%RU32 (%RU64ms)\n",
                      pStream->szName, fJustStarted,
                      DrvAudioHlpMilliToFrames(pStream->Guest.Cfg.Device.uSchedulingHintMs, &pStream->Host.Cfg.Props),
                      pStream->Guest.Cfg.Device.uSchedulingHintMs,
                      cfPassedReal, DrvAudioHlpFramesToMilli(cfPassedReal, &pStream->Host.Cfg.Props),
                      cfLive, DrvAudioHlpFramesToMilli(cfLive, &pStream->Host.Cfg.Props),
                      cfPeriod, DrvAudioHlpFramesToMilli(cfPeriod, &pStream->Host.Cfg.Props),
                      cfWritable, DrvAudioHlpFramesToMilli(cfWritable, &pStream->Host.Cfg.Props),
                      cfToPlay, DrvAudioHlpFramesToMilli(cfToPlay, &pStream->Host.Cfg.Props),
                      cfPlayedTotal, DrvAudioHlpFramesToMilli(cfPlayedTotal, &pStream->Host.Cfg.Props)));
        }

        if (RT_SUCCESS(rc))
        {
            AudioMixBufFinish(&pStream->Host.MixBuf, cfPlayedTotal);

#ifdef VBOX_WITH_STATISTICS
            STAM_COUNTER_ADD     (&pThis->Stats.TotalFramesOut, cfPlayedTotal);
            STAM_PROFILE_ADV_STOP(&pThis->Stats.DelayOut, out);

            STAM_COUNTER_ADD     (&pStream->Out.Stats.TotalFramesPlayed, cfPlayedTotal);
            STAM_COUNTER_INC     (&pStream->Out.Stats.TotalTimesPlayed);
#endif
        }

    } while (0);

#ifdef LOG_ENABLED
    uint32_t cfLive = AudioMixBufLive(&pStream->Host.MixBuf);
    pszStreamSts  = dbgAudioStreamStatusToStr(stsStream);
    Log3Func(("[%s] End fStatus=%s, cfLive=%RU32, cfPlayedTotal=%RU32, rc=%Rrc\n",
              pStream->szName, pszStreamSts, cfLive, cfPlayedTotal, rc));
    RTStrFree(pszStreamSts);
#endif /* LOG_ENABLED */

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    if (RT_SUCCESS(rc))
    {
        if (pcFramesPlayed)
            *pcFramesPlayed = cfPlayedTotal;
    }

    if (RT_FAILURE(rc))
        LogFlowFunc(("[%s] Failed with %Rrc\n", pStream->szName, rc));

    return rc;
}

/**
 * Captures non-interleaved input from a host stream.
 *
 * @returns IPRT status code.
 * @param   pThis               Driver instance.
 * @param   pStream             Stream to capture from.
 * @param   pcfCaptured         Number of (host) audio frames captured. Optional.
 */
static int drvAudioStreamCaptureNonInterleaved(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream, uint32_t *pcfCaptured)
{
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);
    /* pcfCaptured is optional. */

    /* Sanity. */
    Assert(pStream->enmDir == PDMAUDIODIR_IN);
    Assert(pStream->Host.Cfg.enmLayout == PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED);

    int rc = VINF_SUCCESS;

    uint32_t cfCapturedTotal = 0;

    AssertPtr(pThis->pHostDrvAudio->pfnStreamGetReadable);

    uint8_t  auBuf[_1K]; /** @todo Get rid of this. */
    uint32_t cbBuf = sizeof(auBuf);

    uint32_t cbReadable = pThis->pHostDrvAudio->pfnStreamGetReadable(pThis->pHostDrvAudio, pStream->pvBackend);
    if (!cbReadable)
        Log2Func(("[%s] No readable data available\n", pStream->szName));

    uint32_t cbFree = AudioMixBufFreeBytes(&pStream->Guest.MixBuf); /* Parent */
    if (!cbFree)
        Log2Func(("[%s] Buffer full\n", pStream->szName));

    if (cbReadable > cbFree) /* More data readable than we can store at the moment? Limit. */
        cbReadable = cbFree;

    while (cbReadable)
    {
        uint32_t cbCaptured;
        rc = pThis->pHostDrvAudio->pfnStreamCapture(pThis->pHostDrvAudio, pStream->pvBackend,
                                                    auBuf, RT_MIN(cbReadable, cbBuf), &cbCaptured);
        if (RT_FAILURE(rc))
        {
            int rc2 = drvAudioStreamControlInternalBackend(pThis, pStream, PDMAUDIOSTREAMCMD_DISABLE);
            AssertRC(rc2);

            break;
        }

        Assert(cbCaptured <= cbBuf);
        if (cbCaptured > cbBuf) /* Paranoia. */
            cbCaptured = cbBuf;

        if (!cbCaptured) /* Nothing captured? Take a shortcut. */
            break;

        /* We use the host side mixing buffer as an intermediate buffer to do some
         * (first) processing (if needed), so always write the incoming data at offset 0. */
        uint32_t cfHstWritten = 0;
        rc = AudioMixBufWriteAt(&pStream->Host.MixBuf, 0 /* offFrames */, auBuf, cbCaptured, &cfHstWritten);
        if (   RT_FAILURE(rc)
            || !cfHstWritten)
        {
            AssertMsgFailed(("[%s] Write failed: cbCaptured=%RU32, cfHstWritten=%RU32, rc=%Rrc\n",
                             pStream->szName, cbCaptured, cfHstWritten, rc));
            break;
        }

        if (pThis->In.Cfg.Dbg.fEnabled)
            DrvAudioHlpFileWrite(pStream->In.Dbg.pFileCaptureNonInterleaved, auBuf, cbCaptured, 0 /* fFlags */);

        uint32_t cfHstMixed = 0;
        if (cfHstWritten)
        {
            int rc2 = AudioMixBufMixToParentEx(&pStream->Host.MixBuf, 0 /* cSrcOffset */, cfHstWritten /* cSrcFrames */,
                                               &cfHstMixed /* pcSrcMixed */);
            Log3Func(("[%s] cbCaptured=%RU32, cfWritten=%RU32, cfMixed=%RU32, rc=%Rrc\n",
                      pStream->szName, cbCaptured, cfHstWritten, cfHstMixed, rc2));
            AssertRC(rc2);
        }

        Assert(cbReadable >= cbCaptured);
        cbReadable      -= cbCaptured;
        cfCapturedTotal += cfHstMixed;
    }

    if (RT_SUCCESS(rc))
    {
        if (cfCapturedTotal)
            Log2Func(("[%s] %RU32 frames captured, rc=%Rrc\n", pStream->szName, cfCapturedTotal, rc));
    }
    else
        LogFunc(("[%s] Capturing failed with rc=%Rrc\n", pStream->szName, rc));

    if (pcfCaptured)
        *pcfCaptured = cfCapturedTotal;

    return rc;
}

/**
 * Captures raw input from a host stream.
 * Raw input means that the backend directly operates on PDMAUDIOFRAME structs without
 * no data layout processing done in between.
 *
 * Needed for e.g. the VRDP audio backend (in Main).
 *
 * @returns IPRT status code.
 * @param   pThis               Driver instance.
 * @param   pStream             Stream to capture from.
 * @param   pcfCaptured         Number of (host) audio frames captured. Optional.
 */
static int drvAudioStreamCaptureRaw(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream, uint32_t *pcfCaptured)
{
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);
    /* pcfCaptured is optional. */

    /* Sanity. */
    Assert(pStream->enmDir == PDMAUDIODIR_IN);
    Assert(pStream->Host.Cfg.enmLayout == PDMAUDIOSTREAMLAYOUT_RAW);

    int rc = VINF_SUCCESS;

    uint32_t cfCapturedTotal = 0;

    AssertPtr(pThis->pHostDrvAudio->pfnStreamGetReadable);

    /* Note: Raw means *audio frames*, not bytes! */
    uint32_t cfReadable = pThis->pHostDrvAudio->pfnStreamGetReadable(pThis->pHostDrvAudio, pStream->pvBackend);
    if (!cfReadable)
        Log2Func(("[%s] No readable data available\n", pStream->szName));

    const uint32_t cfFree = AudioMixBufFree(&pStream->Guest.MixBuf); /* Parent */
    if (!cfFree)
        Log2Func(("[%s] Buffer full\n", pStream->szName));

    if (cfReadable > cfFree) /* More data readable than we can store at the moment? Limit. */
        cfReadable = cfFree;

    while (cfReadable)
    {
        PPDMAUDIOFRAME paFrames;
        uint32_t cfWritable;
        rc = AudioMixBufPeekMutable(&pStream->Host.MixBuf, cfReadable, &paFrames, &cfWritable);
        if (   RT_FAILURE(rc)
            || !cfWritable)
        {
            break;
        }

        uint32_t cfCaptured;
        rc = pThis->pHostDrvAudio->pfnStreamCapture(pThis->pHostDrvAudio, pStream->pvBackend,
                                                    paFrames, cfWritable, &cfCaptured);
        if (RT_FAILURE(rc))
        {
            int rc2 = drvAudioStreamControlInternalBackend(pThis, pStream, PDMAUDIOSTREAMCMD_DISABLE);
            AssertRC(rc2);

            break;
        }

        Assert(cfCaptured <= cfWritable);
        if (cfCaptured > cfWritable) /* Paranoia. */
            cfCaptured = cfWritable;

        Assert(cfReadable >= cfCaptured);
        cfReadable      -= cfCaptured;
        cfCapturedTotal += cfCaptured;
    }

    Log2Func(("[%s] %RU32 frames captured, rc=%Rrc\n", pStream->szName, cfCapturedTotal, rc));

    if (pcfCaptured)
        *pcfCaptured = cfCapturedTotal;

    return rc;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamCapture}
 */
static DECLCALLBACK(int) drvAudioStreamCapture(PPDMIAUDIOCONNECTOR pInterface,
                                               PPDMAUDIOSTREAM pStream, uint32_t *pcFramesCaptured)
{
    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    AssertMsg(pStream->enmDir == PDMAUDIODIR_IN,
              ("Stream '%s' is not an input stream and therefore cannot be captured (direction is 0x%x)\n",
               pStream->szName, pStream->enmDir));

    uint32_t cfCaptured = 0;

#ifdef LOG_ENABLED
    char *pszStreamSts = dbgAudioStreamStatusToStr(pStream->fStatus);
    Log3Func(("[%s] fStatus=%s\n", pStream->szName, pszStreamSts));
    RTStrFree(pszStreamSts);
#endif /* LOG_ENABLED */

    do
    {
        if (!pThis->pHostDrvAudio)
        {
            rc = VERR_PDM_NO_ATTACHED_DRIVER;
            break;
        }

        if (   !pThis->In.fEnabled
            || !DrvAudioHlpStreamStatusCanRead(pStream->fStatus))
        {
            rc = VERR_AUDIO_STREAM_NOT_READY;
            break;
        }

        /*
         * Do the actual capturing.
         */

        if (pThis->pHostDrvAudio->pfnStreamCaptureBegin)
            pThis->pHostDrvAudio->pfnStreamCaptureBegin(pThis->pHostDrvAudio, pStream->pvBackend);

        if (RT_LIKELY(pStream->Host.Cfg.enmLayout == PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED))
        {
            rc = drvAudioStreamCaptureNonInterleaved(pThis, pStream, &cfCaptured);
        }
        else if (pStream->Host.Cfg.enmLayout == PDMAUDIOSTREAMLAYOUT_RAW)
        {
            rc = drvAudioStreamCaptureRaw(pThis, pStream, &cfCaptured);
        }
        else
            AssertFailedStmt(rc = VERR_NOT_IMPLEMENTED);

        if (pThis->pHostDrvAudio->pfnStreamCaptureEnd)
            pThis->pHostDrvAudio->pfnStreamCaptureEnd(pThis->pHostDrvAudio, pStream->pvBackend);

        if (RT_SUCCESS(rc))
        {
            Log3Func(("[%s] %RU32 frames captured, rc=%Rrc\n", pStream->szName, cfCaptured, rc));

#ifdef VBOX_WITH_STATISTICS
            STAM_COUNTER_ADD(&pThis->Stats.TotalFramesIn,            cfCaptured);

            STAM_COUNTER_ADD(&pStream->In.Stats.TotalFramesCaptured, cfCaptured);
#endif
        }
        else if (RT_UNLIKELY(RT_FAILURE(rc)))
        {
            LogRel(("Audio: Capturing stream '%s' failed with %Rrc\n", pStream->szName, rc));
        }

    } while (0);

    if (pcFramesCaptured)
        *pcFramesCaptured = cfCaptured;

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    if (RT_FAILURE(rc))
        LogFlowFuncLeaveRC(rc);

    return rc;
}

#ifdef VBOX_WITH_AUDIO_CALLBACKS
/**
 * Duplicates an audio callback.
 *
 * @returns Pointer to duplicated callback, or NULL on failure.
 * @param   pCB                 Callback to duplicate.
 */
static PPDMAUDIOCBRECORD drvAudioCallbackDuplicate(PPDMAUDIOCBRECORD pCB)
{
    AssertPtrReturn(pCB, NULL);

    PPDMAUDIOCBRECORD pCBCopy = (PPDMAUDIOCBRECORD)RTMemDup((void *)pCB, sizeof(PDMAUDIOCBRECORD));
    if (!pCBCopy)
        return NULL;

    if (pCB->pvCtx)
    {
        pCBCopy->pvCtx = RTMemDup(pCB->pvCtx, pCB->cbCtx);
        if (!pCBCopy->pvCtx)
        {
            RTMemFree(pCBCopy);
            return NULL;
        }

        pCBCopy->cbCtx = pCB->cbCtx;
    }

    return pCBCopy;
}

/**
 * Destroys a given callback.
 *
 * @param   pCB                 Callback to destroy.
 */
static void drvAudioCallbackDestroy(PPDMAUDIOCBRECORD pCB)
{
    if (!pCB)
        return;

    RTListNodeRemove(&pCB->Node);
    if (pCB->pvCtx)
    {
        Assert(pCB->cbCtx);
        RTMemFree(pCB->pvCtx);
    }
    RTMemFree(pCB);
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnRegisterCallbacks}
 */
static DECLCALLBACK(int) drvAudioRegisterCallbacks(PPDMIAUDIOCONNECTOR pInterface,
                                                   PPDMAUDIOCBRECORD paCallbacks, size_t cCallbacks)
{
    AssertPtrReturn(pInterface,  VERR_INVALID_POINTER);
    AssertPtrReturn(paCallbacks, VERR_INVALID_POINTER);
    AssertReturn(cCallbacks,     VERR_INVALID_PARAMETER);

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    for (size_t i = 0; i < cCallbacks; i++)
    {
        PPDMAUDIOCBRECORD pCB = drvAudioCallbackDuplicate(&paCallbacks[i]);
        if (!pCB)
        {
            rc = VERR_NO_MEMORY;
            break;
        }

        switch (pCB->enmSource)
        {
            case PDMAUDIOCBSOURCE_DEVICE:
            {
                switch (pCB->Device.enmType)
                {
                    case PDMAUDIODEVICECBTYPE_DATA_INPUT:
                        RTListAppend(&pThis->In.lstCB, &pCB->Node);
                        break;

                    case PDMAUDIODEVICECBTYPE_DATA_OUTPUT:
                        RTListAppend(&pThis->Out.lstCB, &pCB->Node);
                        break;

                    default:
                        AssertMsgFailed(("Not supported\n"));
                        break;
                }

                break;
            }

            default:
               AssertMsgFailed(("Not supported\n"));
               break;
        }
    }

    /** @todo Undo allocations on error. */

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    return rc;
}
#endif /* VBOX_WITH_AUDIO_CALLBACKS */

#ifdef VBOX_WITH_AUDIO_CALLBACKS
/**
 * Backend callback implementation.
 *
 * Important: No calls back to the backend within this function, as the backend
 *            might hold any locks / critical sections while executing this callback.
 *            Will result in some ugly deadlocks (or at least locking order violations) then.
 *
 * @copydoc FNPDMHOSTAUDIOCALLBACK
 */
static DECLCALLBACK(int) drvAudioBackendCallback(PPDMDRVINS pDrvIns,
                                                 PDMAUDIOBACKENDCBTYPE enmType, void *pvUser, size_t cbUser)
{
    AssertPtrReturn(pDrvIns, VERR_INVALID_POINTER);
    RT_NOREF(pvUser, cbUser);
    /* pvUser and cbUser are optional. */

    /* Get the upper driver (PDMIAUDIOCONNECTOR). */
    AssertPtr(pDrvIns->pUpBase);
    PPDMIAUDIOCONNECTOR pInterface = PDMIBASE_QUERY_INTERFACE(pDrvIns->pUpBase, PDMIAUDIOCONNECTOR);
    AssertPtr(pInterface);
    PDRVAUDIO           pThis      = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    AssertRCReturn(rc, rc);

    LogFunc(("pThis=%p, enmType=%RU32, pvUser=%p, cbUser=%zu\n", pThis, enmType, pvUser, cbUser));

    switch (enmType)
    {
        case PDMAUDIOBACKENDCBTYPE_DEVICES_CHANGED:
            LogRel(("Audio: Device configuration of driver '%s' has changed\n", pThis->szName));
            rc = drvAudioScheduleReInitInternal(pThis);
            break;

        default:
            AssertMsgFailed(("Not supported\n"));
            break;
    }

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    LogFlowFunc(("Returning %Rrc\n", rc));
    return rc;
}
#endif /* VBOX_WITH_AUDIO_CALLBACKS */

#ifdef VBOX_WITH_AUDIO_ENUM
/**
 * Enumerates all host audio devices.
 * This functionality might not be implemented by all backends and will return VERR_NOT_SUPPORTED
 * if not being supported.
 *
 * @returns IPRT status code.
 * @param   pThis               Driver instance to be called.
 * @param   fLog                Whether to print the enumerated device to the release log or not.
 * @param   pDevEnum            Where to store the device enumeration.
 */
static int drvAudioDevicesEnumerateInternal(PDRVAUDIO pThis, bool fLog, PPDMAUDIODEVICEENUM pDevEnum)
{
    int rc;

    /*
     * If the backend supports it, do a device enumeration.
     */
    if (pThis->pHostDrvAudio->pfnGetDevices)
    {
        PDMAUDIODEVICEENUM DevEnum;
        rc = pThis->pHostDrvAudio->pfnGetDevices(pThis->pHostDrvAudio, &DevEnum);
        if (RT_SUCCESS(rc))
        {
            if (fLog)
                LogRel(("Audio: Found %RU16 devices for driver '%s'\n", DevEnum.cDevices, pThis->szName));

            PPDMAUDIODEVICE pDev;
            RTListForEach(&DevEnum.lstDevices, pDev, PDMAUDIODEVICE, Node)
            {
                if (fLog)
                {
                    char *pszFlags = DrvAudioHlpAudDevFlagsToStrA(pDev->fFlags);

                    LogRel(("Audio: Device '%s':\n", pDev->szName));
                    LogRel(("Audio: \tUsage           = %s\n",   DrvAudioHlpAudDirToStr(pDev->enmUsage)));
                    LogRel(("Audio: \tFlags           = %s\n",   pszFlags ? pszFlags : "<NONE>"));
                    LogRel(("Audio: \tInput channels  = %RU8\n", pDev->cMaxInputChannels));
                    LogRel(("Audio: \tOutput channels = %RU8\n", pDev->cMaxOutputChannels));

                    if (pszFlags)
                        RTStrFree(pszFlags);
                }
            }

            if (pDevEnum)
                rc = DrvAudioHlpDeviceEnumCopy(pDevEnum, &DevEnum);

            DrvAudioHlpDeviceEnumFree(&DevEnum);
        }
        else
        {
            if (fLog)
                LogRel(("Audio: Device enumeration for driver '%s' failed with %Rrc\n", pThis->szName, rc));
            /* Not fatal. */
        }
    }
    else
    {
        rc = VERR_NOT_SUPPORTED;

        if (fLog)
            LogRel2(("Audio: Host driver '%s' does not support audio device enumeration, skipping\n", pThis->szName));
    }

    LogFunc(("Returning %Rrc\n", rc));
    return rc;
}
#endif /* VBOX_WITH_AUDIO_ENUM */

/**
 * Initializes the host backend and queries its initial configuration.
 * If the host backend fails, VERR_AUDIO_BACKEND_INIT_FAILED will be returned.
 *
 * Note: As this routine is called when attaching to the device LUN in the
 *       device emulation, we either check for success or VERR_AUDIO_BACKEND_INIT_FAILED.
 *       Everything else is considered as fatal and must be handled separately in
 *       the device emulation!
 *
 * @return  IPRT status code.
 * @param   pThis               Driver instance to be called.
 * @param   pCfgHandle          CFGM configuration handle to use for this driver.
 */
static int drvAudioHostInit(PDRVAUDIO pThis, PCFGMNODE pCfgHandle)
{
    /* pCfgHandle is optional. */
    NOREF(pCfgHandle);
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);

    LogFlowFuncEnter();

    AssertPtr(pThis->pHostDrvAudio);
    int rc = pThis->pHostDrvAudio->pfnInit(pThis->pHostDrvAudio);
    if (RT_FAILURE(rc))
    {
        LogRel(("Audio: Initialization of host driver '%s' failed with %Rrc\n", pThis->szName, rc));
        return VERR_AUDIO_BACKEND_INIT_FAILED;
    }

    /*
     * Get the backend configuration.
     */
    rc = pThis->pHostDrvAudio->pfnGetConfig(pThis->pHostDrvAudio, &pThis->BackendCfg);
    if (RT_FAILURE(rc))
    {
        LogRel(("Audio: Getting configuration for driver '%s' failed with %Rrc\n", pThis->szName, rc));
        return VERR_AUDIO_BACKEND_INIT_FAILED;
    }

    pThis->In.cStreamsFree  = pThis->BackendCfg.cMaxStreamsIn;
    pThis->Out.cStreamsFree = pThis->BackendCfg.cMaxStreamsOut;

    LogFlowFunc(("cStreamsFreeIn=%RU8, cStreamsFreeOut=%RU8\n", pThis->In.cStreamsFree, pThis->Out.cStreamsFree));

    LogRel2(("Audio: Host driver '%s' supports %RU32 input streams and %RU32 output streams at once\n",
             pThis->szName,
             /* Clamp for logging. Unlimited streams are defined by UINT32_MAX. */
             RT_MIN(64, pThis->In.cStreamsFree), RT_MIN(64, pThis->Out.cStreamsFree)));

#ifdef VBOX_WITH_AUDIO_ENUM
    int rc2 = drvAudioDevicesEnumerateInternal(pThis, true /* fLog */, NULL /* pDevEnum */);
    if (rc2 != VERR_NOT_SUPPORTED) /* Some backends don't implement device enumeration. */
        AssertRC(rc2);

    RT_NOREF(rc2);
    /* Ignore rc. */
#endif

#ifdef VBOX_WITH_AUDIO_CALLBACKS
    /*
     * If the backend supports it, offer a callback to this connector.
     */
    if (pThis->pHostDrvAudio->pfnSetCallback)
    {
        rc2 = pThis->pHostDrvAudio->pfnSetCallback(pThis->pHostDrvAudio, drvAudioBackendCallback);
        if (RT_FAILURE(rc2))
             LogRel(("Audio: Error registering callback for host driver '%s', rc=%Rrc\n", pThis->szName, rc2));
        /* Not fatal. */
    }
#endif

    LogFlowFuncLeave();
    return VINF_SUCCESS;
}

/**
 * Handles state changes for all audio streams.
 *
 * @param   pDrvIns             Pointer to driver instance.
 * @param   enmCmd              Stream command to set for all streams.
 */
static void drvAudioStateHandler(PPDMDRVINS pDrvIns, PDMAUDIOSTREAMCMD enmCmd)
{
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);
    PDRVAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVAUDIO);

    LogFlowFunc(("enmCmd=%s\n", DrvAudioHlpStreamCmdToStr(enmCmd)));

    int rc2 = RTCritSectEnter(&pThis->CritSect);
    AssertRC(rc2);

    if (pThis->pHostDrvAudio)
    {
        PPDMAUDIOSTREAM pStream;
        RTListForEach(&pThis->lstStreams, pStream, PDMAUDIOSTREAM, Node)
            drvAudioStreamControlInternal(pThis, pStream, enmCmd);
    }

    rc2 = RTCritSectLeave(&pThis->CritSect);
    AssertRC(rc2);
}

/**
 * Retrieves an audio configuration from the specified CFGM node.
 *
 * @return VBox status code.
 * @param  pThis                Driver instance to be called.
 * @param  pCfg                 Where to store the retrieved audio configuration to.
 * @param  pNode                Where to get the audio configuration from.
 */
static int drvAudioGetCfgFromCFGM(PDRVAUDIO pThis, PDRVAUDIOCFG pCfg, PCFGMNODE pNode)
{
    RT_NOREF(pThis);

    /* Debug stuff. */
    CFGMR3QueryBoolDef(pNode, "DebugEnabled",      &pCfg->Dbg.fEnabled,  false);
    int rc2 = CFGMR3QueryString(pNode, "DebugPathOut", pCfg->Dbg.szPathOut, sizeof(pCfg->Dbg.szPathOut));
    if (   RT_FAILURE(rc2)
        || !strlen(pCfg->Dbg.szPathOut))
    {
        RTStrPrintf(pCfg->Dbg.szPathOut, sizeof(pCfg->Dbg.szPathOut), VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH);
    }

    if (pCfg->Dbg.fEnabled)
        LogRel(("Audio: Debugging for driver '%s' enabled (audio data written to '%s')\n", pThis->szName, pCfg->Dbg.szPathOut));

    /* Buffering stuff. */
    CFGMR3QueryU32Def(pNode, "PeriodSizeMs",    &pCfg->uPeriodSizeMs, 0);
    CFGMR3QueryU32Def(pNode, "BufferSizeMs",    &pCfg->uBufferSizeMs, 0);
    CFGMR3QueryU32Def(pNode, "PreBufferSizeMs", &pCfg->uPreBufSizeMs, UINT32_MAX /* No custom value set */);

    LogFunc(("pCfg=%p, uPeriodSizeMs=%RU32, uBufferSizeMs=%RU32, uPreBufSizeMs=%RU32\n",
             pCfg, pCfg->uPeriodSizeMs, pCfg->uBufferSizeMs, pCfg->uPreBufSizeMs));

    return VINF_SUCCESS;
}

/**
 * Intializes an audio driver instance.
 *
 * @returns IPRT status code.
 * @param   pDrvIns             Pointer to driver instance.
 * @param   pCfgHandle          CFGM handle to use for configuration.
 */
static int drvAudioInit(PPDMDRVINS pDrvIns, PCFGMNODE pCfgHandle)
{
    AssertPtrReturn(pCfgHandle, VERR_INVALID_POINTER);
    AssertPtrReturn(pDrvIns,    VERR_INVALID_POINTER);

    PDRVAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVAUDIO);

    int rc = RTCritSectInit(&pThis->CritSect);
    AssertRCReturn(rc, rc);

    /*
     * Configure driver from CFGM.
     */
#ifdef DEBUG
    CFGMR3Dump(pCfgHandle);
#endif

    pThis->fTerminate = false;
    pThis->pCFGMNode  = pCfgHandle;

    int rc2 = CFGMR3QueryString(pThis->pCFGMNode, "DriverName", pThis->szName, sizeof(pThis->szName));
    if (RT_FAILURE(rc2))
        RTStrPrintf(pThis->szName, sizeof(pThis->szName), "Untitled");

    /* By default we don't enable anything if wrongly / not set-up. */
    CFGMR3QueryBoolDef(pThis->pCFGMNode, "InputEnabled",  &pThis->In.fEnabled,   false);
    CFGMR3QueryBoolDef(pThis->pCFGMNode, "OutputEnabled", &pThis->Out.fEnabled,  false);

    LogRel2(("Audio: Verbose logging for driver '%s' enabled\n", pThis->szName));

    LogRel2(("Audio: Initial status for driver '%s' is: input is %s, output is %s\n",
             pThis->szName, pThis->In.fEnabled ? "enabled" : "disabled", pThis->Out.fEnabled ? "enabled" : "disabled"));

    /*
     * Load configurations.
     */
    rc = drvAudioGetCfgFromCFGM(pThis, &pThis->In.Cfg, pThis->pCFGMNode);
    if (RT_SUCCESS(rc))
        rc = drvAudioGetCfgFromCFGM(pThis, &pThis->Out.Cfg, pThis->pCFGMNode);

    LogFunc(("[%s] rc=%Rrc\n", pThis->szName, rc));
    return rc;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamRead}
 */
static DECLCALLBACK(int) drvAudioStreamRead(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream,
                                            void *pvBuf, uint32_t cbBuf, uint32_t *pcbRead)
{
    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);

    AssertPtrReturn(pStream, VERR_INVALID_POINTER);
    AssertPtrReturn(pvBuf,   VERR_INVALID_POINTER);
    AssertReturn(cbBuf,      VERR_INVALID_PARAMETER);
    /* pcbRead is optional. */

    AssertMsg(pStream->enmDir == PDMAUDIODIR_IN,
              ("Stream '%s' is not an input stream and therefore cannot be read from (direction is 0x%x)\n",
               pStream->szName, pStream->enmDir));

    uint32_t cbReadTotal = 0;

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    do
    {
        if (   !pThis->In.fEnabled
            || !DrvAudioHlpStreamStatusCanRead(pStream->fStatus))
        {
            rc = VERR_AUDIO_STREAM_NOT_READY;
            break;
        }

        /*
         * Read from the parent buffer (that is, the guest buffer) which
         * should have the audio data in the format the guest needs.
         */
        uint32_t cfReadTotal = 0;

        const uint32_t cfBuf = AUDIOMIXBUF_B2F(&pStream->Guest.MixBuf, cbBuf);

        uint32_t cfToRead = RT_MIN(cfBuf, AudioMixBufLive(&pStream->Guest.MixBuf));
        while (cfToRead)
        {
            uint32_t cfRead;
            rc = AudioMixBufAcquireReadBlock(&pStream->Guest.MixBuf,
                                             (uint8_t *)pvBuf + AUDIOMIXBUF_F2B(&pStream->Guest.MixBuf, cfReadTotal),
                                             AUDIOMIXBUF_F2B(&pStream->Guest.MixBuf, cfToRead), &cfRead);
            if (RT_FAILURE(rc))
                break;

#ifdef VBOX_WITH_STATISTICS
            const uint32_t cbRead = AUDIOMIXBUF_F2B(&pStream->Guest.MixBuf, cfRead);

            STAM_COUNTER_ADD(&pThis->Stats.TotalBytesRead,       cbRead);

            STAM_COUNTER_ADD(&pStream->In.Stats.TotalFramesRead, cfRead);
            STAM_COUNTER_INC(&pStream->In.Stats.TotalTimesRead);
#endif
            Assert(cfToRead >= cfRead);
            cfToRead -= cfRead;

            cfReadTotal += cfRead;

            AudioMixBufReleaseReadBlock(&pStream->Guest.MixBuf, cfRead);
        }

        if (cfReadTotal)
        {
            if (pThis->In.Cfg.Dbg.fEnabled)
                DrvAudioHlpFileWrite(pStream->In.Dbg.pFileStreamRead,
                                     pvBuf, AUDIOMIXBUF_F2B(&pStream->Guest.MixBuf, cfReadTotal), 0 /* fFlags */);

            AudioMixBufFinish(&pStream->Guest.MixBuf, cfReadTotal);
        }

        /* If we were not able to read as much data as requested, fill up the returned
         * data with silence.
         *
         * This is needed to keep the device emulation DMA transfers up and running at a constant rate. */
        if (cfReadTotal < cfBuf)
        {
            Log3Func(("[%s] Filling in silence (%RU64ms / %RU64ms)\n", pStream->szName,
                      DrvAudioHlpFramesToMilli(cfBuf - cfReadTotal, &pStream->Guest.Cfg.Props),
                      DrvAudioHlpFramesToMilli(cfBuf, &pStream->Guest.Cfg.Props)));

            DrvAudioHlpClearBuf(&pStream->Guest.Cfg.Props,
                                (uint8_t *)pvBuf + AUDIOMIXBUF_F2B(&pStream->Guest.MixBuf, cfReadTotal),
                                AUDIOMIXBUF_F2B(&pStream->Guest.MixBuf, cfBuf - cfReadTotal),
                                cfBuf - cfReadTotal);

            cfReadTotal = cfBuf;
        }

        cbReadTotal = AUDIOMIXBUF_F2B(&pStream->Guest.MixBuf, cfReadTotal);

        pStream->tsLastReadWrittenNs = RTTimeNanoTS();

        Log3Func(("[%s] fEnabled=%RTbool, cbReadTotal=%RU32, rc=%Rrc\n", pStream->szName, pThis->In.fEnabled, cbReadTotal, rc));

    } while (0);


    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    if (RT_SUCCESS(rc))
    {
        if (pcbRead)
            *pcbRead = cbReadTotal;
    }

    return rc;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamCreate}
 */
static DECLCALLBACK(int) drvAudioStreamCreate(PPDMIAUDIOCONNECTOR pInterface,
                                              PPDMAUDIOSTREAMCFG pCfgHost, PPDMAUDIOSTREAMCFG pCfgGuest,
                                              PPDMAUDIOSTREAM *ppStream)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgHost,   VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgGuest,  VERR_INVALID_POINTER);
    AssertPtrReturn(ppStream,   VERR_INVALID_POINTER);

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    LogFlowFunc(("Host=%s, Guest=%s\n", pCfgHost->szName, pCfgGuest->szName));
#ifdef DEBUG
    DrvAudioHlpStreamCfgPrint(pCfgHost);
    DrvAudioHlpStreamCfgPrint(pCfgGuest);
#endif

    PPDMAUDIOSTREAM pStream = NULL;

#define RC_BREAK(x) { rc = x; break; }

    do
    {
        if (   !DrvAudioHlpStreamCfgIsValid(pCfgHost)
            || !DrvAudioHlpStreamCfgIsValid(pCfgGuest))
        {
            RC_BREAK(VERR_INVALID_PARAMETER);
        }

        /* Make sure that both configurations actually intend the same thing. */
        if (pCfgHost->enmDir != pCfgGuest->enmDir)
        {
            AssertMsgFailed(("Stream configuration directions do not match\n"));
            RC_BREAK(VERR_INVALID_PARAMETER);
        }

        /* Note: cbHstStrm will contain the size of the data the backend needs to operate on. */
        size_t cbHstStrm;
        if (pCfgHost->enmDir == PDMAUDIODIR_IN)
        {
            if (!pThis->In.cStreamsFree)
            {
                LogFlowFunc(("Maximum number of host input streams reached\n"));
                RC_BREAK(VERR_AUDIO_NO_FREE_INPUT_STREAMS);
            }

            cbHstStrm = pThis->BackendCfg.cbStreamIn;
        }
        else /* Out */
        {
            if (!pThis->Out.cStreamsFree)
            {
                LogFlowFunc(("Maximum number of host output streams reached\n"));
                RC_BREAK(VERR_AUDIO_NO_FREE_OUTPUT_STREAMS);
            }

            cbHstStrm = pThis->BackendCfg.cbStreamOut;
        }

        /*
         * Allocate and initialize common state.
         */

        pStream = (PPDMAUDIOSTREAM)RTMemAllocZ(sizeof(PDMAUDIOSTREAM));
        AssertPtrBreakStmt(pStream, rc = VERR_NO_MEMORY);

        /* Retrieve host driver name for easier identification. */
        AssertPtr(pThis->pHostDrvAudio);
        PPDMDRVINS pDrvAudioInst = PDMIBASE_2_PDMDRV(pThis->pDrvIns->pDownBase);
        AssertPtr(pDrvAudioInst);
        AssertPtr(pDrvAudioInst->pReg);

        Assert(pDrvAudioInst->pReg->szName[0] != '\0');
        RTStrPrintf(pStream->szName, RT_ELEMENTS(pStream->szName), "[%s] %s",
                    pDrvAudioInst->pReg->szName[0] != '\0' ? pDrvAudioInst->pReg->szName : "Untitled",
                    pCfgHost->szName[0] != '\0' ? pCfgHost->szName : "<Untitled>");

        pStream->enmDir = pCfgHost->enmDir;

        /*
         * Allocate and init backend-specific data.
         */

        if (cbHstStrm) /* High unlikely that backends do not have an own space for data, but better check. */
        {
            pStream->pvBackend = RTMemAllocZ(cbHstStrm);
            AssertPtrBreakStmt(pStream->pvBackend, rc = VERR_NO_MEMORY);

            pStream->cbBackend = cbHstStrm;
        }

        /*
         * Try to init the rest.
         */

        rc = drvAudioStreamInitInternal(pThis, pStream, pCfgHost, pCfgGuest);
        if (RT_FAILURE(rc))
            break;

    } while (0);

#undef RC_BREAK

    if (RT_FAILURE(rc))
    {
        if (pStream)
        {
            int rc2 = drvAudioStreamUninitInternal(pThis, pStream);
            if (RT_SUCCESS(rc2))
            {
                drvAudioStreamFree(pStream);
                pStream = NULL;
            }
        }
    }
    else
    {
        /* Append the stream to our stream list. */
        RTListAppend(&pThis->lstStreams, &pStream->Node);

        /* Set initial reference counts. */
        pStream->cRefs = 1;

        if (pCfgHost->enmDir == PDMAUDIODIR_IN)
        {
            if (pThis->In.Cfg.Dbg.fEnabled)
            {
                char szFile[RTPATH_MAX + 1];

                int rc2 = DrvAudioHlpFileNameGet(szFile, RT_ELEMENTS(szFile), pThis->In.Cfg.Dbg.szPathOut, "CaptureNonInterleaved",
                                                 pThis->pDrvIns->iInstance, PDMAUDIOFILETYPE_WAV, PDMAUDIOFILENAME_FLAG_NONE);
                if (RT_SUCCESS(rc2))
                {
                    rc2 = DrvAudioHlpFileCreate(PDMAUDIOFILETYPE_WAV, szFile, PDMAUDIOFILE_FLAG_NONE,
                                                &pStream->In.Dbg.pFileCaptureNonInterleaved);
                    if (RT_SUCCESS(rc2))
                        rc2 = DrvAudioHlpFileOpen(pStream->In.Dbg.pFileCaptureNonInterleaved, PDMAUDIOFILE_DEFAULT_OPEN_FLAGS,
                                                  &pStream->Host.Cfg.Props);
                }

                if (RT_SUCCESS(rc2))
                {
                    rc2 = DrvAudioHlpFileNameGet(szFile, RT_ELEMENTS(szFile), pThis->In.Cfg.Dbg.szPathOut, "StreamRead",
                                                 pThis->pDrvIns->iInstance, PDMAUDIOFILETYPE_WAV, PDMAUDIOFILENAME_FLAG_NONE);
                    if (RT_SUCCESS(rc2))
                    {
                        rc2 = DrvAudioHlpFileCreate(PDMAUDIOFILETYPE_WAV, szFile, PDMAUDIOFILE_FLAG_NONE,
                                                    &pStream->In.Dbg.pFileStreamRead);
                        if (RT_SUCCESS(rc2))
                            rc2 = DrvAudioHlpFileOpen(pStream->In.Dbg.pFileStreamRead, PDMAUDIOFILE_DEFAULT_OPEN_FLAGS,
                                                      &pStream->Host.Cfg.Props);
                    }
                }
            }

            if (pThis->In.cStreamsFree)
                pThis->In.cStreamsFree--;
        }
        else /* Out */
        {
            if (pThis->Out.Cfg.Dbg.fEnabled)
            {
                char szFile[RTPATH_MAX + 1];

                int rc2 = DrvAudioHlpFileNameGet(szFile, RT_ELEMENTS(szFile), pThis->Out.Cfg.Dbg.szPathOut, "PlayNonInterleaved",
                                                 pThis->pDrvIns->iInstance, PDMAUDIOFILETYPE_WAV, PDMAUDIOFILENAME_FLAG_NONE);
                if (RT_SUCCESS(rc2))
                {
                    rc2 = DrvAudioHlpFileCreate(PDMAUDIOFILETYPE_WAV, szFile, PDMAUDIOFILE_FLAG_NONE,
                                                &pStream->Out.Dbg.pFilePlayNonInterleaved);
                    if (RT_SUCCESS(rc2))
                        rc = DrvAudioHlpFileOpen(pStream->Out.Dbg.pFilePlayNonInterleaved, PDMAUDIOFILE_DEFAULT_OPEN_FLAGS,
                                                 &pStream->Host.Cfg.Props);
                }

                if (RT_SUCCESS(rc2))
                {
                    rc2 = DrvAudioHlpFileNameGet(szFile, RT_ELEMENTS(szFile), pThis->Out.Cfg.Dbg.szPathOut, "StreamWrite",
                                                 pThis->pDrvIns->iInstance, PDMAUDIOFILETYPE_WAV, PDMAUDIOFILENAME_FLAG_NONE);
                    if (RT_SUCCESS(rc2))
                    {
                        rc2 = DrvAudioHlpFileCreate(PDMAUDIOFILETYPE_WAV, szFile, PDMAUDIOFILE_FLAG_NONE,
                                                    &pStream->Out.Dbg.pFileStreamWrite);
                        if (RT_SUCCESS(rc2))
                            rc2 = DrvAudioHlpFileOpen(pStream->Out.Dbg.pFileStreamWrite, PDMAUDIOFILE_DEFAULT_OPEN_FLAGS,
                                                      &pStream->Host.Cfg.Props);
                    }
                }
            }

            if (pThis->Out.cStreamsFree)
                pThis->Out.cStreamsFree--;
        }

#ifdef VBOX_WITH_STATISTICS
        STAM_COUNTER_ADD(&pThis->Stats.TotalStreamsCreated, 1);
#endif
        *ppStream = pStream;
    }

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnEnable}
 */
static DECLCALLBACK(int) drvAudioEnable(PPDMIAUDIOCONNECTOR pInterface, PDMAUDIODIR enmDir, bool fEnable)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    bool *pfEnabled;
    if (enmDir == PDMAUDIODIR_IN)
        pfEnabled = &pThis->In.fEnabled;
    else if (enmDir == PDMAUDIODIR_OUT)
        pfEnabled = &pThis->Out.fEnabled;
    else
        AssertFailedReturn(VERR_INVALID_PARAMETER);

    if (fEnable != *pfEnabled)
    {
        LogRel(("Audio: %s %s for driver '%s'\n",
                fEnable ? "Enabling" : "Disabling", enmDir == PDMAUDIODIR_IN ? "input" : "output", pThis->szName));

        PPDMAUDIOSTREAM pStream;
        RTListForEach(&pThis->lstStreams, pStream, PDMAUDIOSTREAM, Node)
        {
            if (pStream->enmDir != enmDir) /* Skip unwanted streams. */
                continue;

            int rc2 = drvAudioStreamControlInternal(pThis, pStream,
                                                    fEnable ? PDMAUDIOSTREAMCMD_ENABLE : PDMAUDIOSTREAMCMD_DISABLE);
            if (RT_FAILURE(rc2))
                LogRel(("Audio: Failed to %s %s stream '%s', rc=%Rrc\n",
                        fEnable ? "enable" : "disable", enmDir == PDMAUDIODIR_IN ? "input" : "output", pStream->szName, rc2));

            if (RT_SUCCESS(rc))
                rc = rc2;

            /* Keep going. */
        }

        *pfEnabled = fEnable;
    }

    int rc3 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc3;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnIsEnabled}
 */
static DECLCALLBACK(bool) drvAudioIsEnabled(PPDMIAUDIOCONNECTOR pInterface, PDMAUDIODIR enmDir)
{
    AssertPtrReturn(pInterface, false);

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc2 = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc2))
        return false;

    bool *pfEnabled;
    if (enmDir == PDMAUDIODIR_IN)
        pfEnabled = &pThis->In.fEnabled;
    else if (enmDir == PDMAUDIODIR_OUT)
        pfEnabled = &pThis->Out.fEnabled;
    else
        AssertFailedReturn(false);

    const bool fIsEnabled = *pfEnabled;

    rc2 = RTCritSectLeave(&pThis->CritSect);
    AssertRC(rc2);

    return fIsEnabled;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnGetConfig}
 */
static DECLCALLBACK(int) drvAudioGetConfig(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOBACKENDCFG pCfg)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfg,       VERR_INVALID_POINTER);

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    if (pThis->pHostDrvAudio)
    {
        if (pThis->pHostDrvAudio->pfnGetConfig)
            rc = pThis->pHostDrvAudio->pfnGetConfig(pThis->pHostDrvAudio, pCfg);
        else
            rc = VERR_NOT_SUPPORTED;
    }
    else
        rc = VERR_PDM_NO_ATTACHED_DRIVER;

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnGetStatus}
 */
static DECLCALLBACK(PDMAUDIOBACKENDSTS) drvAudioGetStatus(PPDMIAUDIOCONNECTOR pInterface, PDMAUDIODIR enmDir)
{
    AssertPtrReturn(pInterface, PDMAUDIOBACKENDSTS_UNKNOWN);

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    PDMAUDIOBACKENDSTS backendSts = PDMAUDIOBACKENDSTS_UNKNOWN;

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_SUCCESS(rc))
    {
        if (pThis->pHostDrvAudio)
        {
            if (pThis->pHostDrvAudio->pfnGetStatus)
                backendSts = pThis->pHostDrvAudio->pfnGetStatus(pThis->pHostDrvAudio, enmDir);
        }
        else
            backendSts = PDMAUDIOBACKENDSTS_NOT_ATTACHED;

        int rc2 = RTCritSectLeave(&pThis->CritSect);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    LogFlowFuncLeaveRC(rc);
    return backendSts;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamGetReadable}
 */
static DECLCALLBACK(uint32_t) drvAudioStreamGetReadable(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pInterface, 0);
    AssertPtrReturn(pStream,    0);

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc2 = RTCritSectEnter(&pThis->CritSect);
    AssertRC(rc2);

    AssertMsg(pStream->enmDir == PDMAUDIODIR_IN, ("Can't read from a non-input stream\n"));

    uint32_t cbReadable = 0;

    if (   pThis->pHostDrvAudio
        && DrvAudioHlpStreamStatusCanRead(pStream->fStatus))
    {
        const uint32_t cfReadable = AudioMixBufLive(&pStream->Guest.MixBuf);

        cbReadable = AUDIOMIXBUF_F2B(&pStream->Guest.MixBuf, cfReadable);

        if (!cbReadable)
        {
            /*
             * If nothing is readable, check if the stream on the backend side is ready to be read from.
             * If it isn't, return the number of bytes readable since the last read from this stream.
             *
             * This is needed for backends (e.g. VRDE) which do not provide any input data in certain
             * situations, but the device emulation needs input data to keep the DMA transfers moving.
             * Reading the actual data from a stream then will return silence then.
             */
            if (!DrvAudioHlpStreamStatusCanRead(
                pThis->pHostDrvAudio->pfnStreamGetStatus(pThis->pHostDrvAudio, pStream->pvBackend)))
            {
                cbReadable = DrvAudioHlpNanoToBytes(RTTimeNanoTS() - pStream->tsLastReadWrittenNs,
                                                    &pStream->Host.Cfg.Props);
                Log3Func(("[%s] Backend stream not ready, returning silence\n", pStream->szName));
            }
        }

        /* Make sure to align the readable size to the guest's frame size. */
        cbReadable = DrvAudioHlpBytesAlign(cbReadable, &pStream->Guest.Cfg.Props);
    }

    Log3Func(("[%s] cbReadable=%RU32 (%RU64ms)\n",
              pStream->szName, cbReadable, DrvAudioHlpBytesToMilli(cbReadable, &pStream->Host.Cfg.Props)));

    rc2 = RTCritSectLeave(&pThis->CritSect);
    AssertRC(rc2);

    /* Return bytes instead of audio frames. */
    return cbReadable;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamGetWritable}
 */
static DECLCALLBACK(uint32_t) drvAudioStreamGetWritable(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pInterface, 0);
    AssertPtrReturn(pStream,    0);

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc2 = RTCritSectEnter(&pThis->CritSect);
    AssertRC(rc2);

    AssertMsg(pStream->enmDir == PDMAUDIODIR_OUT, ("Can't write to a non-output stream\n"));

    uint32_t cbWritable = 0;

    /* Note: We don't propage the backend stream's status to the outside -- it's the job of this
     *       audio connector to make sense of it. */
    if (DrvAudioHlpStreamStatusCanWrite(pStream->fStatus))
    {
        cbWritable = AudioMixBufFreeBytes(&pStream->Host.MixBuf);

        /* Make sure to align the writable size to the host's frame size. */
        cbWritable = DrvAudioHlpBytesAlign(cbWritable, &pStream->Host.Cfg.Props);
    }

    Log3Func(("[%s] cbWritable=%RU32 (%RU64ms)\n",
              pStream->szName, cbWritable, DrvAudioHlpBytesToMilli(cbWritable, &pStream->Host.Cfg.Props)));

    rc2 = RTCritSectLeave(&pThis->CritSect);
    AssertRC(rc2);

    return cbWritable;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamGetStatus}
 */
static DECLCALLBACK(PDMAUDIOSTREAMSTS) drvAudioStreamGetStatus(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pInterface, false);

    if (!pStream)
        return PDMAUDIOSTREAMSTS_FLAG_NONE;

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc2 = RTCritSectEnter(&pThis->CritSect);
    AssertRC(rc2);

    PDMAUDIOSTREAMSTS stsStream = pStream->fStatus;

#ifdef LOG_ENABLED
    char *pszStreamSts = dbgAudioStreamStatusToStr(stsStream);
    Log3Func(("[%s] %s\n", pStream->szName, pszStreamSts));
    RTStrFree(pszStreamSts);
#endif

    rc2 = RTCritSectLeave(&pThis->CritSect);
    AssertRC(rc2);

    return stsStream;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamSetVolume}
 */
static DECLCALLBACK(int) drvAudioStreamSetVolume(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream, PPDMAUDIOVOLUME pVol)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    AssertPtrReturn(pVol,       VERR_INVALID_POINTER);

    LogFlowFunc(("[%s] volL=%RU32, volR=%RU32, fMute=%RTbool\n", pStream->szName, pVol->uLeft, pVol->uRight, pVol->fMuted));

    AudioMixBufSetVolume(&pStream->Guest.MixBuf, pVol);
    AudioMixBufSetVolume(&pStream->Host.MixBuf, pVol);

    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamDestroy}
 */
static DECLCALLBACK(int) drvAudioStreamDestroy(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    AssertRC(rc);

    LogRel2(("Audio: Destroying stream '%s'\n", pStream->szName));

    LogFlowFunc(("[%s] cRefs=%RU32\n", pStream->szName, pStream->cRefs));
    if (pStream->cRefs > 1)
        rc = VERR_WRONG_ORDER;

    if (RT_SUCCESS(rc))
    {
        rc = drvAudioStreamUninitInternal(pThis, pStream);
        if (RT_FAILURE(rc))
            LogRel(("Audio: Uninitializing stream '%s' failed with %Rrc\n", pStream->szName, rc));
    }

    if (RT_SUCCESS(rc))
    {
        if (pStream->enmDir == PDMAUDIODIR_IN)
        {
            pThis->In.cStreamsFree++;
        }
        else /* Out */
        {
            pThis->Out.cStreamsFree++;
        }

        RTListNodeRemove(&pStream->Node);

        drvAudioStreamFree(pStream);
        pStream = NULL;
    }

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Creates an audio stream on the backend side.
 *
 * @returns IPRT status code.
 * @param   pThis               Pointer to driver instance.
 * @param   pStream             Audio stream to create the backend side for.
 * @param   pCfgReq             Requested audio stream configuration to use for stream creation.
 * @param   pCfgAcq             Acquired audio stream configuration returned by the backend.
 *
 * @note    Configuration precedence for requested audio stream configuration (first has highest priority, if set):
 *          - per global extra-data
 *          - per-VM extra-data
 *          - requested configuration (by pCfgReq)
 *          - default value
 */
static int drvAudioStreamCreateInternalBackend(PDRVAUDIO pThis,
                                               PPDMAUDIOSTREAM pStream, PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgReq, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgAcq, VERR_INVALID_POINTER);

    AssertMsg((pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_INITIALIZED) == 0,
              ("Stream '%s' already initialized in backend\n", pStream->szName));

    /* Get the right configuration for the stream to be created. */
    PDRVAUDIOCFG pDrvCfg = pCfgReq->enmDir == PDMAUDIODIR_IN ? &pThis->In.Cfg : &pThis->Out.Cfg;

    /* Fill in the tweakable parameters into the requested host configuration.
     * All parameters in principle can be changed and returned by the backend via the acquired configuration. */

    char szWhat[64]; /* Log where a value came from. */

    /*
     * Period size
     */
    if (pDrvCfg->uPeriodSizeMs)
    {
        pCfgReq->Backend.cfPeriod = DrvAudioHlpMilliToFrames(pDrvCfg->uPeriodSizeMs, &pCfgReq->Props);
        RTStrPrintf(szWhat, sizeof(szWhat), "global / per-VM");
    }

    if (!pCfgReq->Backend.cfPeriod) /* Set default period size if nothing explicitly is set. */
    {
        pCfgReq->Backend.cfPeriod = DrvAudioHlpMilliToFrames(50 /* ms */, &pCfgReq->Props);
        RTStrPrintf(szWhat, sizeof(szWhat), "default");
    }
    else
        RTStrPrintf(szWhat, sizeof(szWhat), "device-specific");

    LogRel2(("Audio: Using %s period size (%RU64ms, %RU32 frames) for stream '%s'\n",
             szWhat,
             DrvAudioHlpFramesToMilli(pCfgReq->Backend.cfPeriod, &pCfgReq->Props), pCfgReq->Backend.cfPeriod, pStream->szName));

    /*
     * Buffer size
     */
    if (pDrvCfg->uBufferSizeMs)
    {
        pCfgReq->Backend.cfBufferSize = DrvAudioHlpMilliToFrames(pDrvCfg->uBufferSizeMs, &pCfgReq->Props);
        RTStrPrintf(szWhat, sizeof(szWhat), "global / per-VM");
    }

    if (!pCfgReq->Backend.cfBufferSize) /* Set default buffer size if nothing explicitly is set. */
    {
        pCfgReq->Backend.cfBufferSize = DrvAudioHlpMilliToFrames(250 /* ms */, &pCfgReq->Props);
        RTStrPrintf(szWhat, sizeof(szWhat), "default");
    }
    else
        RTStrPrintf(szWhat, sizeof(szWhat), "device-specific");

    LogRel2(("Audio: Using %s buffer size (%RU64ms, %RU32 frames) for stream '%s'\n",
             szWhat,
             DrvAudioHlpFramesToMilli(pCfgReq->Backend.cfBufferSize, &pCfgReq->Props), pCfgReq->Backend.cfBufferSize, pStream->szName));

    /*
     * Pre-buffering size
     */
    if (pDrvCfg->uPreBufSizeMs != UINT32_MAX) /* Anything set via global / per-VM extra-data? */
    {
        pCfgReq->Backend.cfPreBuf = DrvAudioHlpMilliToFrames(pDrvCfg->uPreBufSizeMs, &pCfgReq->Props);
        RTStrPrintf(szWhat, sizeof(szWhat), "global / per-VM");
    }
    else /* No, then either use the default or device-specific settings (if any). */
    {
        if (pCfgReq->Backend.cfPreBuf == UINT32_MAX) /* Set default pre-buffering size if nothing explicitly is set. */
        {
            /* For pre-buffering to finish the buffer at least must be full one time. */
            pCfgReq->Backend.cfPreBuf = pCfgReq->Backend.cfBufferSize;
            RTStrPrintf(szWhat, sizeof(szWhat), "default");
        }
        else
            RTStrPrintf(szWhat, sizeof(szWhat), "device-specific");
    }

    LogRel2(("Audio: Using %s pre-buffering size (%RU64ms, %RU32 frames) for stream '%s'\n",
             szWhat,
             DrvAudioHlpFramesToMilli(pCfgReq->Backend.cfPreBuf, &pCfgReq->Props), pCfgReq->Backend.cfPreBuf, pStream->szName));

    /*
     * Validate input.
     */
    if (pCfgReq->Backend.cfBufferSize < pCfgReq->Backend.cfPeriod)
    {
        LogRel(("Audio: Error for stream '%s': Buffering size (%RU64ms) must not be smaller than the period size (%RU64ms)\n",
                pStream->szName, DrvAudioHlpFramesToMilli(pCfgReq->Backend.cfBufferSize, &pCfgReq->Props),
                DrvAudioHlpFramesToMilli(pCfgReq->Backend.cfPeriod, &pCfgReq->Props)));
        return VERR_INVALID_PARAMETER;
    }

    if (   pCfgReq->Backend.cfPreBuf != UINT32_MAX /* Custom pre-buffering set? */
        && pCfgReq->Backend.cfPreBuf)
    {
        if (pCfgReq->Backend.cfBufferSize < pCfgReq->Backend.cfPreBuf)
        {
            LogRel(("Audio: Error for stream '%s': Buffering size (%RU64ms) must not be smaller than the pre-buffering size (%RU64ms)\n",
                    pStream->szName, DrvAudioHlpFramesToMilli(pCfgReq->Backend.cfPreBuf, &pCfgReq->Props),
                    DrvAudioHlpFramesToMilli(pCfgReq->Backend.cfBufferSize, &pCfgReq->Props)));
            return VERR_INVALID_PARAMETER;
        }
    }

    /* Make the acquired host configuration the requested host configuration initially,
     * in case the backend does not report back an acquired configuration. */
    int rc = DrvAudioHlpStreamCfgCopy(pCfgAcq, pCfgReq);
    if (RT_FAILURE(rc))
    {
        LogRel(("Audio: Creating stream '%s' with an invalid backend configuration not possible, skipping\n",
                pStream->szName));
        return rc;
    }

    rc = pThis->pHostDrvAudio->pfnStreamCreate(pThis->pHostDrvAudio, pStream->pvBackend, pCfgReq, pCfgAcq);
    if (RT_FAILURE(rc))
    {
        if (rc == VERR_NOT_SUPPORTED)
            LogRel2(("Audio: Creating stream '%s' in backend not supported\n", pStream->szName));
        else if (rc == VERR_AUDIO_STREAM_COULD_NOT_CREATE)
            LogRel2(("Audio: Stream '%s' could not be created in backend because of missing hardware / drivers\n", pStream->szName));
        else
            LogRel(("Audio: Creating stream '%s' in backend failed with %Rrc\n", pStream->szName, rc));

        return rc;
    }

    /* Validate acquired configuration. */
    if (!DrvAudioHlpStreamCfgIsValid(pCfgAcq))
    {
        LogRel(("Audio: Creating stream '%s' returned an invalid backend configuration, skipping\n", pStream->szName));
        return VERR_INVALID_PARAMETER;
    }

    /* Let the user know that the backend changed one of the values requested above. */
    if (pCfgAcq->Backend.cfBufferSize != pCfgReq->Backend.cfBufferSize)
        LogRel2(("Audio: Buffer size overwritten by backend for stream '%s' (now %RU64ms, %RU32 frames)\n",
                 pStream->szName, DrvAudioHlpFramesToMilli(pCfgAcq->Backend.cfBufferSize, &pCfgAcq->Props), pCfgAcq->Backend.cfBufferSize));

    if (pCfgAcq->Backend.cfPeriod != pCfgReq->Backend.cfPeriod)
        LogRel2(("Audio: Period size overwritten by backend for stream '%s' (now %RU64ms, %RU32 frames)\n",
                 pStream->szName, DrvAudioHlpFramesToMilli(pCfgAcq->Backend.cfPeriod, &pCfgAcq->Props), pCfgAcq->Backend.cfPeriod));

    /* Was pre-buffering requested, but the acquired configuration from the backend told us something else? */
    if (   pCfgReq->Backend.cfPreBuf
        && pCfgAcq->Backend.cfPreBuf != pCfgReq->Backend.cfPreBuf)
    {
        LogRel2(("Audio: Pre-buffering size overwritten by backend for stream '%s' (now %RU64ms, %RU32 frames)\n",
                 pStream->szName, DrvAudioHlpFramesToMilli(pCfgAcq->Backend.cfPreBuf, &pCfgAcq->Props), pCfgAcq->Backend.cfPreBuf));
    }
    else if (pCfgReq->Backend.cfPreBuf == 0) /* Was the pre-buffering requested as being disabeld? Tell the users. */
    {
        LogRel2(("Audio: Pre-buffering is disabled for stream '%s'\n", pStream->szName));
        pCfgAcq->Backend.cfPreBuf = 0;
    }

    /* Sanity for detecting buggy backends. */
    AssertMsgReturn(pCfgAcq->Backend.cfPeriod < pCfgAcq->Backend.cfBufferSize,
                    ("Acquired period size must be smaller than buffer size\n"),
                    VERR_INVALID_PARAMETER);
    AssertMsgReturn(pCfgAcq->Backend.cfPreBuf <= pCfgAcq->Backend.cfBufferSize,
                    ("Acquired pre-buffering size must be smaller or as big as the buffer size\n"),
                    VERR_INVALID_PARAMETER);

    pStream->fStatus |= PDMAUDIOSTREAMSTS_FLAG_INITIALIZED;

    return VINF_SUCCESS;
}

/**
 * Calls the backend to give it the chance to destroy its part of the audio stream.
 *
 * @returns IPRT status code.
 * @param   pThis               Pointer to driver instance.
 * @param   pStream             Audio stream destruct backend for.
 */
static int drvAudioStreamDestroyInternalBackend(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;

#ifdef LOG_ENABLED
    char *pszStreamSts = dbgAudioStreamStatusToStr(pStream->fStatus);
    LogFunc(("[%s] fStatus=%s\n", pStream->szName, pszStreamSts));
#endif

    if (pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_INITIALIZED)
    {
        AssertPtr(pStream->pvBackend);

        /* Check if the pointer to  the host audio driver is still valid.
         * It can be NULL if we were called in drvAudioDestruct, for example. */
        if (pThis->pHostDrvAudio)
            rc = pThis->pHostDrvAudio->pfnStreamDestroy(pThis->pHostDrvAudio, pStream->pvBackend);

        pStream->fStatus &= ~PDMAUDIOSTREAMSTS_FLAG_INITIALIZED;
    }

#ifdef LOG_ENABLED
    RTStrFree(pszStreamSts);
#endif

    LogFlowFunc(("[%s] Returning %Rrc\n", pStream->szName, rc));
    return rc;
}

/**
 * Uninitializes an audio stream.
 *
 * @returns IPRT status code.
 * @param   pThis               Pointer to driver instance.
 * @param   pStream             Pointer to audio stream to uninitialize.
 */
static int drvAudioStreamUninitInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    LogFlowFunc(("[%s] cRefs=%RU32\n", pStream->szName, pStream->cRefs));

    if (pStream->cRefs > 1)
        return VERR_WRONG_ORDER;

    int rc = drvAudioStreamControlInternal(pThis, pStream, PDMAUDIOSTREAMCMD_DISABLE);
    if (RT_SUCCESS(rc))
        rc = drvAudioStreamDestroyInternalBackend(pThis, pStream);

    /* Destroy mixing buffers. */
    AudioMixBufDestroy(&pStream->Guest.MixBuf);
    AudioMixBufDestroy(&pStream->Host.MixBuf);

    if (RT_SUCCESS(rc))
    {
#ifdef LOG_ENABLED
        if (pStream->fStatus != PDMAUDIOSTREAMSTS_FLAG_NONE)
        {
            char *pszStreamSts = dbgAudioStreamStatusToStr(pStream->fStatus);
            LogFunc(("[%s] Warning: Still has %s set when uninitializing\n", pStream->szName, pszStreamSts));
            RTStrFree(pszStreamSts);
        }
#endif
        pStream->fStatus = PDMAUDIOSTREAMSTS_FLAG_NONE;
    }

    if (pStream->enmDir == PDMAUDIODIR_IN)
    {
#ifdef VBOX_WITH_STATISTICS
        PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pStream->In.Stats.TotalFramesCaptured);
        PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pStream->In.Stats.TotalTimesCaptured);
        PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pStream->In.Stats.TotalFramesRead);
        PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pStream->In.Stats.TotalTimesRead);
#endif
        if (pThis->In.Cfg.Dbg.fEnabled)
        {
            DrvAudioHlpFileDestroy(pStream->In.Dbg.pFileCaptureNonInterleaved);
            pStream->In.Dbg.pFileCaptureNonInterleaved = NULL;

            DrvAudioHlpFileDestroy(pStream->In.Dbg.pFileStreamRead);
            pStream->In.Dbg.pFileStreamRead = NULL;
        }
    }
    else if (pStream->enmDir == PDMAUDIODIR_OUT)
    {
#ifdef VBOX_WITH_STATISTICS
        PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pStream->Out.Stats.TotalFramesPlayed);
        PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pStream->Out.Stats.TotalTimesPlayed);
        PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pStream->Out.Stats.TotalFramesWritten);
        PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pStream->Out.Stats.TotalTimesWritten);
#endif
        if (pThis->Out.Cfg.Dbg.fEnabled)
        {
            DrvAudioHlpFileDestroy(pStream->Out.Dbg.pFilePlayNonInterleaved);
            pStream->Out.Dbg.pFilePlayNonInterleaved = NULL;

            DrvAudioHlpFileDestroy(pStream->Out.Dbg.pFileStreamWrite);
            pStream->Out.Dbg.pFileStreamWrite = NULL;
        }
    }
    else
        AssertFailed();

    LogFlowFunc(("Returning %Rrc\n", rc));
    return rc;
}

/**
 * Does the actual backend driver attaching and queries the backend's interface.
 *
 * @return VBox status code.
 * @param  pThis                Pointer to driver instance.
 * @param  fFlags               Attach flags; see PDMDrvHlpAttach().
 */
static int drvAudioDoAttachInternal(PDRVAUDIO pThis, uint32_t fFlags)
{
    Assert(pThis->pHostDrvAudio == NULL); /* No nested attaching. */

    /*
     * Attach driver below and query its connector interface.
     */
    PPDMIBASE pDownBase;
    int rc = PDMDrvHlpAttach(pThis->pDrvIns, fFlags, &pDownBase);
    if (RT_SUCCESS(rc))
    {
        pThis->pHostDrvAudio = PDMIBASE_QUERY_INTERFACE(pDownBase, PDMIHOSTAUDIO);
        if (!pThis->pHostDrvAudio)
        {
            LogRel(("Audio: Failed to query interface for underlying host driver '%s'\n", pThis->szName));
            rc = PDMDRV_SET_ERROR(pThis->pDrvIns, VERR_PDM_MISSING_INTERFACE_BELOW,
                                  N_("Host audio backend missing or invalid"));
        }
    }

    if (RT_SUCCESS(rc))
    {
        /*
         * If everything went well, initialize the lower driver.
         */
        AssertPtr(pThis->pCFGMNode);
        rc = drvAudioHostInit(pThis, pThis->pCFGMNode);
    }

    LogFunc(("[%s] rc=%Rrc\n", pThis->szName, rc));
    return rc;
}


/********************************************************************/

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) drvAudioQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    LogFlowFunc(("pInterface=%p, pszIID=%s\n", pInterface, pszIID));

    PPDMDRVINS pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVAUDIO  pThis   = PDMINS_2_DATA(pDrvIns, PDRVAUDIO);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIAUDIOCONNECTOR, &pThis->IAudioConnector);

    return NULL;
}

/**
 * Power Off notification.
 *
 * @param   pDrvIns     The driver instance data.
 */
static DECLCALLBACK(void) drvAudioPowerOff(PPDMDRVINS pDrvIns)
{
    PDRVAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVAUDIO);

    LogFlowFuncEnter();

    if (!pThis->pHostDrvAudio) /* If not lower driver is configured, bail out. */
        return;

    /* Just destroy the host stream on the backend side.
     * The rest will either be destructed by the device emulation or
     * in drvAudioDestruct(). */
    PPDMAUDIOSTREAM pStream;
    RTListForEach(&pThis->lstStreams, pStream, PDMAUDIOSTREAM, Node)
    {
        drvAudioStreamControlInternalBackend(pThis, pStream, PDMAUDIOSTREAMCMD_DISABLE);
        drvAudioStreamDestroyInternalBackend(pThis, pStream);
    }

    /*
     * Last call for the driver below us.
     * Let it know that we reached end of life.
     */
    if (pThis->pHostDrvAudio->pfnShutdown)
        pThis->pHostDrvAudio->pfnShutdown(pThis->pHostDrvAudio);

    pThis->pHostDrvAudio = NULL;

    LogFlowFuncLeave();
}

/**
 * Constructs an audio driver instance.
 *
 * @copydoc FNPDMDRVCONSTRUCT
 */
static DECLCALLBACK(int) drvAudioConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    LogFlowFunc(("pDrvIns=%#p, pCfgHandle=%#p, fFlags=%x\n", pDrvIns, pCfg, fFlags));

    PDRVAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVAUDIO);
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);

    RTListInit(&pThis->lstStreams);
#ifdef VBOX_WITH_AUDIO_CALLBACKS
    RTListInit(&pThis->In.lstCB);
    RTListInit(&pThis->Out.lstCB);
#endif

    /*
     * Init the static parts.
     */
    pThis->pDrvIns                              = pDrvIns;
    /* IBase. */
    pDrvIns->IBase.pfnQueryInterface            = drvAudioQueryInterface;
    /* IAudioConnector. */
    pThis->IAudioConnector.pfnEnable            = drvAudioEnable;
    pThis->IAudioConnector.pfnIsEnabled         = drvAudioIsEnabled;
    pThis->IAudioConnector.pfnGetConfig         = drvAudioGetConfig;
    pThis->IAudioConnector.pfnGetStatus         = drvAudioGetStatus;
    pThis->IAudioConnector.pfnStreamCreate      = drvAudioStreamCreate;
    pThis->IAudioConnector.pfnStreamDestroy     = drvAudioStreamDestroy;
    pThis->IAudioConnector.pfnStreamRetain      = drvAudioStreamRetain;
    pThis->IAudioConnector.pfnStreamRelease     = drvAudioStreamRelease;
    pThis->IAudioConnector.pfnStreamControl     = drvAudioStreamControl;
    pThis->IAudioConnector.pfnStreamRead        = drvAudioStreamRead;
    pThis->IAudioConnector.pfnStreamWrite       = drvAudioStreamWrite;
    pThis->IAudioConnector.pfnStreamIterate     = drvAudioStreamIterate;
    pThis->IAudioConnector.pfnStreamGetReadable = drvAudioStreamGetReadable;
    pThis->IAudioConnector.pfnStreamGetWritable = drvAudioStreamGetWritable;
    pThis->IAudioConnector.pfnStreamGetStatus   = drvAudioStreamGetStatus;
    pThis->IAudioConnector.pfnStreamSetVolume   = drvAudioStreamSetVolume;
    pThis->IAudioConnector.pfnStreamPlay        = drvAudioStreamPlay;
    pThis->IAudioConnector.pfnStreamCapture     = drvAudioStreamCapture;
#ifdef VBOX_WITH_AUDIO_CALLBACKS
    pThis->IAudioConnector.pfnRegisterCallbacks = drvAudioRegisterCallbacks;
#endif

    int rc = drvAudioInit(pDrvIns, pCfg);
    if (RT_SUCCESS(rc))
    {
#ifdef VBOX_WITH_STATISTICS
        PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->Stats.TotalStreamsActive,   "TotalStreamsActive",
                                  STAMUNIT_COUNT, "Total active audio streams.");
        PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->Stats.TotalStreamsCreated,  "TotalStreamsCreated",
                                  STAMUNIT_COUNT, "Total created audio streams.");
        PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->Stats.TotalFramesRead,      "TotalFramesRead",
                                  STAMUNIT_COUNT, "Total frames read by device emulation.");
        PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->Stats.TotalFramesWritten,   "TotalFramesWritten",
                                  STAMUNIT_COUNT, "Total frames written by device emulation ");
        PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->Stats.TotalFramesMixedIn,   "TotalFramesMixedIn",
                                  STAMUNIT_COUNT, "Total input frames mixed.");
        PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->Stats.TotalFramesMixedOut,  "TotalFramesMixedOut",
                                  STAMUNIT_COUNT, "Total output frames mixed.");
        PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->Stats.TotalFramesLostIn,    "TotalFramesLostIn",
                                  STAMUNIT_COUNT, "Total input frames lost.");
        PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->Stats.TotalFramesLostOut,   "TotalFramesLostOut",
                                  STAMUNIT_COUNT, "Total output frames lost.");
        PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->Stats.TotalFramesOut,       "TotalFramesOut",
                                  STAMUNIT_COUNT, "Total frames played by backend.");
        PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->Stats.TotalFramesIn,        "TotalFramesIn",
                                  STAMUNIT_COUNT, "Total frames captured by backend.");
        PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->Stats.TotalBytesRead,       "TotalBytesRead",
                                  STAMUNIT_BYTES, "Total bytes read.");
        PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->Stats.TotalBytesWritten,    "TotalBytesWritten",
                                  STAMUNIT_BYTES, "Total bytes written.");

        PDMDrvHlpSTAMRegProfileAdvEx(pDrvIns, &pThis->Stats.DelayIn,           "DelayIn",
                                     STAMUNIT_NS_PER_CALL, "Profiling of input data processing.");
        PDMDrvHlpSTAMRegProfileAdvEx(pDrvIns, &pThis->Stats.DelayOut,          "DelayOut",
                                     STAMUNIT_NS_PER_CALL, "Profiling of output data processing.");
#endif
    }

    rc = drvAudioDoAttachInternal(pThis, fFlags);
    if (RT_FAILURE(rc))
    {
        /* No lower attached driver (yet)? Not a failure, might get attached later at runtime, just skip. */
        if (rc == VERR_PDM_NO_ATTACHED_DRIVER)
            rc = VINF_SUCCESS;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Destructs an audio driver instance.
 *
 * @copydoc FNPDMDRVDESTRUCT
 */
static DECLCALLBACK(void) drvAudioDestruct(PPDMDRVINS pDrvIns)
{
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);
    PDRVAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVAUDIO);

    LogFlowFuncEnter();

    int rc2;

    if (RTCritSectIsInitialized(&pThis->CritSect))
    {
        rc2 = RTCritSectEnter(&pThis->CritSect);
        AssertRC(rc2);
    }

    /*
     * Note: No calls here to the driver below us anymore,
     *       as PDM already has destroyed it.
     *       If you need to call something from the host driver,
     *       do this in drvAudioPowerOff() instead.
     */

    /* Thus, NULL the pointer to the host audio driver first,
     * so that routines like drvAudioStreamDestroyInternal() don't call the driver(s) below us anymore. */
    pThis->pHostDrvAudio = NULL;

    PPDMAUDIOSTREAM pStream, pStreamNext;
    RTListForEachSafe(&pThis->lstStreams, pStream, pStreamNext, PDMAUDIOSTREAM, Node)
    {
        rc2 = drvAudioStreamUninitInternal(pThis, pStream);
        if (RT_SUCCESS(rc2))
        {
            RTListNodeRemove(&pStream->Node);

            drvAudioStreamFree(pStream);
            pStream = NULL;
        }
    }

    /* Sanity. */
    Assert(RTListIsEmpty(&pThis->lstStreams));

#ifdef VBOX_WITH_AUDIO_CALLBACKS
    /*
     * Destroy callbacks, if any.
     */
    PPDMAUDIOCBRECORD pCB, pCBNext;
    RTListForEachSafe(&pThis->In.lstCB, pCB, pCBNext, PDMAUDIOCBRECORD, Node)
        drvAudioCallbackDestroy(pCB);

    RTListForEachSafe(&pThis->Out.lstCB, pCB, pCBNext, PDMAUDIOCBRECORD, Node)
        drvAudioCallbackDestroy(pCB);
#endif

    if (RTCritSectIsInitialized(&pThis->CritSect))
    {
        rc2 = RTCritSectLeave(&pThis->CritSect);
        AssertRC(rc2);

        rc2 = RTCritSectDelete(&pThis->CritSect);
        AssertRC(rc2);
    }

#ifdef VBOX_WITH_STATISTICS
    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pThis->Stats.TotalStreamsActive);
    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pThis->Stats.TotalStreamsCreated);
    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pThis->Stats.TotalFramesRead);
    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pThis->Stats.TotalFramesWritten);
    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pThis->Stats.TotalFramesMixedIn);
    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pThis->Stats.TotalFramesMixedOut);
    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pThis->Stats.TotalFramesLostIn);
    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pThis->Stats.TotalFramesLostOut);
    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pThis->Stats.TotalFramesOut);
    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pThis->Stats.TotalFramesIn);
    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pThis->Stats.TotalBytesRead);
    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pThis->Stats.TotalBytesWritten);
    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pThis->Stats.DelayIn);
    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pThis->Stats.DelayOut);
#endif

    LogFlowFuncLeave();
}

/**
 * Suspend notification.
 *
 * @param   pDrvIns     The driver instance data.
 */
static DECLCALLBACK(void) drvAudioSuspend(PPDMDRVINS pDrvIns)
{
    drvAudioStateHandler(pDrvIns, PDMAUDIOSTREAMCMD_PAUSE);
}

/**
 * Resume notification.
 *
 * @param   pDrvIns     The driver instance data.
 */
static DECLCALLBACK(void) drvAudioResume(PPDMDRVINS pDrvIns)
{
    drvAudioStateHandler(pDrvIns, PDMAUDIOSTREAMCMD_RESUME);
}

/**
 * Attach notification.
 *
 * @param   pDrvIns     The driver instance data.
 * @param   fFlags      Attach flags.
 */
static DECLCALLBACK(int) drvAudioAttach(PPDMDRVINS pDrvIns, uint32_t fFlags)
{
    RT_NOREF(fFlags);

    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);
    PDRVAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVAUDIO);

    int rc2 = RTCritSectEnter(&pThis->CritSect);
    AssertRC(rc2);

    LogFunc(("%s\n", pThis->szName));

    int rc = drvAudioDoAttachInternal(pThis, fFlags);

    rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    return rc;
}

/**
 * Detach notification.
 *
 * @param   pDrvIns     The driver instance data.
 * @param   fFlags      Detach flags.
 */
static DECLCALLBACK(void) drvAudioDetach(PPDMDRVINS pDrvIns, uint32_t fFlags)
{
    RT_NOREF(fFlags);

    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);
    PDRVAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVAUDIO);

    int rc2 = RTCritSectEnter(&pThis->CritSect);
    AssertRC(rc2);

    pThis->pHostDrvAudio = NULL;

    LogFunc(("%s\n", pThis->szName));

    rc2 = RTCritSectLeave(&pThis->CritSect);
    AssertRC(rc2);
}

/**
 * Audio driver registration record.
 */
const PDMDRVREG g_DrvAUDIO =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "AUDIO",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "Audio connector driver",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass */
    PDM_DRVREG_CLASS_AUDIO,
    /* cMaxInstances */
    UINT32_MAX,
    /* cbInstance */
    sizeof(DRVAUDIO),
    /* pfnConstruct */
    drvAudioConstruct,
    /* pfnDestruct */
    drvAudioDestruct,
    /* pfnRelocate */
    NULL,
    /* pfnIOCtl */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    NULL,
    /* pfnSuspend */
    drvAudioSuspend,
    /* pfnResume */
    drvAudioResume,
    /* pfnAttach */
    drvAudioAttach,
    /* pfnDetach */
    drvAudioDetach,
    /* pfnPowerOff */
    drvAudioPowerOff,
    /* pfnSoftReset */
    NULL,
    /* u32EndVersion */
    PDM_DRVREG_VERSION
};

