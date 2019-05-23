/* $Id: DrvAudio.cpp $ */
/** @file
 * Intermediate audio driver header.
 *
 * @remarks Intermediate audio driver for connecting the audio device emulation
 *          with the host backend.
 */

/*
 * Copyright (C) 2006-2018 Oracle Corporation
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
static int drvAudioStreamCreateInternalBackend(PDRVAUDIO pThis, PPDMAUDIOSTREAM pHstStream, PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq);
static int drvAudioStreamDestroyInternalBackend(PDRVAUDIO pThis, PPDMAUDIOSTREAM pHstStream);
static void drvAudioStreamFree(PPDMAUDIOSTREAM pStream);
static int drvAudioStreamUninitInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream);
static int drvAudioStreamInitInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream, PPDMAUDIOSTREAMCFG pCfgHost, PPDMAUDIOSTREAMCFG pCfgGuest);
static int drvAudioStreamIterateInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream);
static int drvAudioStreamReInitInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream);
static int drvAudioStreamLinkToInternal(PPDMAUDIOSTREAM pStream, PPDMAUDIOSTREAM pPair);

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

/**
 * Returns the host stream part of an audio stream pair, or NULL
 * if no host stream has been assigned / is not available.
 *
 * @returns IPRT status code.
 * @param   pStream             Audio stream to retrieve host stream part for.
 */
DECLINLINE(PPDMAUDIOSTREAM) drvAudioGetHostStream(PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pStream, NULL);

    if (!pStream)
        return NULL;

    PPDMAUDIOSTREAM pHstStream = pStream->enmCtx == PDMAUDIOSTREAMCTX_HOST
                               ? pStream
                               : pStream->pPair;
    if (pHstStream)
    {
        AssertReleaseMsg(pHstStream->enmCtx == PDMAUDIOSTREAMCTX_HOST,
                         ("Stream '%s' resolved as host part is not marked as such (enmCtx=%RU32)\n",
                          pHstStream->szName, pHstStream->enmCtx));

        AssertReleaseMsg(pHstStream->pPair != NULL,
                         ("Stream '%s' resolved as host part has no guest part (anymore)\n", pHstStream->szName));
    }
    else
        LogRel(("Audio: Warning: Stream '%s' does not have a host stream (anymore)\n", pStream->szName));

    return pHstStream;
}

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

    PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
    PPDMAUDIOSTREAM pGstStream = pHstStream ? pHstStream->pPair : pStream;
    AssertPtr(pGstStream);

#ifdef LOG_ENABLED
    char *pszHstSts = dbgAudioStreamStatusToStr(pHstStream->fStatus);
    char *pszGstSts = dbgAudioStreamStatusToStr(pHstStream->fStatus);
    LogFlowFunc(("Status host=%s, guest=%s\n", pszHstSts, pszGstSts));
    RTStrFree(pszGstSts);
    RTStrFree(pszHstSts);
#endif /* LOG_ENABLED */

    int rc = VINF_SUCCESS;

    switch (enmStreamCmd)
    {
        case PDMAUDIOSTREAMCMD_ENABLE:
        {
            if (!(pGstStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_ENABLED))
            {
                if (pHstStream)
                {
                    /* Is a pending disable outstanding? Then disable first. */
                    if (pHstStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_PENDING_DISABLE)
                        rc = drvAudioStreamControlInternalBackend(pThis, pHstStream, PDMAUDIOSTREAMCMD_DISABLE);

                    if (RT_SUCCESS(rc))
                        rc = drvAudioStreamControlInternalBackend(pThis, pHstStream, PDMAUDIOSTREAMCMD_ENABLE);
                }

                if (RT_SUCCESS(rc))
                    pGstStream->fStatus |= PDMAUDIOSTREAMSTS_FLAG_ENABLED;
            }
            break;
        }

        case PDMAUDIOSTREAMCMD_DISABLE:
        {
            if (pGstStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_ENABLED)
            {
                if (pHstStream)
                {
                    /*
                     * For playback (output) streams first mark the host stream as pending disable,
                     * so that the rest of the remaining audio data will be played first before
                     * closing the stream.
                     */
                    if (pHstStream->enmDir == PDMAUDIODIR_OUT)
                    {
                        LogFunc(("[%s] Pending disable/pause\n", pHstStream->szName));
                        pHstStream->fStatus |= PDMAUDIOSTREAMSTS_FLAG_PENDING_DISABLE;
                    }

                    /* Can we close the host stream as well (not in pending disable mode)? */
                    if (!(pHstStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_PENDING_DISABLE))
                        rc = drvAudioStreamControlInternalBackend(pThis, pHstStream, PDMAUDIOSTREAMCMD_DISABLE);
                }

                if (RT_SUCCESS(rc))
                    pGstStream->fStatus &= ~PDMAUDIOSTREAMSTS_FLAG_ENABLED;
            }
            break;
        }

        case PDMAUDIOSTREAMCMD_PAUSE:
        {
            if (!(pGstStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_PAUSED))
            {
                if (pHstStream)
                    rc = drvAudioStreamControlInternalBackend(pThis, pHstStream, PDMAUDIOSTREAMCMD_PAUSE);

                if (RT_SUCCESS(rc))
                    pGstStream->fStatus |= PDMAUDIOSTREAMSTS_FLAG_PAUSED;
            }
            break;
        }

        case PDMAUDIOSTREAMCMD_RESUME:
        {
            if (pGstStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_PAUSED)
            {
                if (pHstStream)
                    rc = drvAudioStreamControlInternalBackend(pThis, pHstStream, PDMAUDIOSTREAMCMD_RESUME);

                if (RT_SUCCESS(rc))
                    pGstStream->fStatus &= ~PDMAUDIOSTREAMSTS_FLAG_PAUSED;
            }
            break;
        }

        default:
            AssertMsgFailed(("Command %s (%RU32) not implemented\n", DrvAudioHlpStreamCmdToStr(enmStreamCmd), enmStreamCmd));
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

    PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
    if (!pHstStream) /* Stream does not have a host backend? Bail out. */
        return VERR_NOT_FOUND;

#ifdef LOG_ENABLED
    char *pszHstSts = dbgAudioStreamStatusToStr(pHstStream->fStatus);
    LogFlowFunc(("[%s] enmStreamCmd=%s, fStatus=%s\n", pHstStream->szName, DrvAudioHlpStreamCmdToStr(enmStreamCmd), pszHstSts));
    RTStrFree(pszHstSts);
#endif /* LOG_ENABLED */

    LogRel2(("Audio: %s stream '%s'\n", DrvAudioHlpStreamCmdToStr(enmStreamCmd), pHstStream->szName));

    AssertPtr(pThis->pHostDrvAudio);

    int rc = VINF_SUCCESS;

    switch (enmStreamCmd)
    {
        case PDMAUDIOSTREAMCMD_ENABLE:
        {
            if (!(pHstStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_ENABLED))
            {
                rc = pThis->pHostDrvAudio->pfnStreamControl(pThis->pHostDrvAudio, pHstStream->pvBackend,
                                                            PDMAUDIOSTREAMCMD_ENABLE);
                if (RT_SUCCESS(rc))
                {
                    pHstStream->fStatus |= PDMAUDIOSTREAMSTS_FLAG_ENABLED;
                }
            }
            break;
        }

        case PDMAUDIOSTREAMCMD_DISABLE:
        {
            if (pHstStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_ENABLED)
            {
                rc = pThis->pHostDrvAudio->pfnStreamControl(pThis->pHostDrvAudio, pHstStream->pvBackend,
                                                            PDMAUDIOSTREAMCMD_DISABLE);
                if (RT_SUCCESS(rc))
                {
                    pHstStream->fStatus = PDMAUDIOSTREAMSTS_FLAG_INITIALIZED; /* Reset to initialized state. */
                    AudioMixBufReset(&pHstStream->MixBuf);
                }
            }
            break;
        }

        case PDMAUDIOSTREAMCMD_PAUSE:
        {
            /* Only pause if the stream is enabled. */
            if (!(pHstStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_ENABLED))
                break;

            if (!(pHstStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_PAUSED))
            {
                rc = pThis->pHostDrvAudio->pfnStreamControl(pThis->pHostDrvAudio, pHstStream->pvBackend,
                                                            PDMAUDIOSTREAMCMD_PAUSE);
                if (RT_SUCCESS(rc))
                {
                    pHstStream->fStatus |= PDMAUDIOSTREAMSTS_FLAG_PAUSED;
                }
            }
            break;
        }

        case PDMAUDIOSTREAMCMD_RESUME:
        {
            /* Only need to resume if the stream is enabled. */
            if (!(pHstStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_ENABLED))
                break;

            if (pHstStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_PAUSED)
            {
                rc = pThis->pHostDrvAudio->pfnStreamControl(pThis->pHostDrvAudio, pHstStream->pvBackend,
                                                            PDMAUDIOSTREAMCMD_RESUME);
                if (RT_SUCCESS(rc))
                {
                    pHstStream->fStatus &= ~PDMAUDIOSTREAMSTS_FLAG_PAUSED;
                }
            }
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
        LogRel2(("Audio: %s stream '%s' failed with %Rrc\n", DrvAudioHlpStreamCmdToStr(enmStreamCmd), pHstStream->szName, rc));
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

    PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
    PPDMAUDIOSTREAM pGstStream = pHstStream ? pHstStream->pPair : pStream;
    AssertPtr(pGstStream);

    /*
     * Init host stream.
     */

    /* Set the host's default audio data layout. */
    pCfgHost->enmLayout = PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED;

#ifdef DEBUG
    LogFunc(("[%s] Requested host format:\n", pStream->szName));
    DrvAudioHlpStreamCfgPrint(pCfgHost);
#else
    LogRel2(("Audio: Creating stream '%s'\n", pStream->szName));
    LogRel2(("Audio: Requested %s host format for '%s': %RU32Hz, %RU8%s, %RU8 %s\n",
             pCfgGuest->enmDir == PDMAUDIODIR_IN ? "recording" : "playback", pStream->szName,
             pCfgHost->Props.uHz, pCfgHost->Props.cBits, pCfgHost->Props.fSigned ? "S" : "U",
             pCfgHost->Props.cChannels, pCfgHost->Props.cChannels == 0 ? "Channel" : "Channels"));
#endif

    PDMAUDIOSTREAMCFG CfgHostAcq;
    int rc = drvAudioStreamCreateInternalBackend(pThis, pHstStream, pCfgHost, &CfgHostAcq);
    if (RT_FAILURE(rc))
        return rc;

#ifdef DEBUG
    LogFunc(("[%s] Acquired host format:\n",  pStream->szName));
    DrvAudioHlpStreamCfgPrint(&CfgHostAcq);
#else
    LogRel2(("Audio: Acquired %s host format for '%s': %RU32Hz, %RU8%s, %RU8 %s\n",
             CfgHostAcq.enmDir == PDMAUDIODIR_IN ? "recording" : "playback",  pStream->szName,
             CfgHostAcq.Props.uHz, CfgHostAcq.Props.cBits, CfgHostAcq.Props.fSigned ? "S" : "U",
             CfgHostAcq.Props.cChannels, CfgHostAcq.Props.cChannels == 0 ? "Channel" : "Channels"));
#endif

    /* No frame buffer size hint given by the backend? Default to some sane value. */
    if (!CfgHostAcq.cFrameBufferHint)
    {
        CfgHostAcq.cFrameBufferHint = _1K; /** @todo Make this configurable? */
    }

    /* Destroy any former mixing buffer. */
    AudioMixBufDestroy(&pHstStream->MixBuf);

    /* Make sure to (re-)set the host buffer's shift size. */
    CfgHostAcq.Props.cShift = PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(CfgHostAcq.Props.cBits, CfgHostAcq.Props.cChannels);

    /* Set set host buffer size multiplicator. */
    const unsigned cFrameBufferHostFactor = 2; /** @todo Make this configurable. */

    LogFunc(("[%s] cFrames=%RU32 (x %u)\n", pHstStream->szName, CfgHostAcq.cFrameBufferHint, cFrameBufferHostFactor));

    int rc2 = AudioMixBufInit(&pHstStream->MixBuf, pHstStream->szName, &CfgHostAcq.Props,
                              CfgHostAcq.cFrameBufferHint * cFrameBufferHostFactor);
    AssertRC(rc2);

    /* Make a copy of the acquired host stream configuration. */
    rc2 = DrvAudioHlpStreamCfgCopy(&pHstStream->Cfg, &CfgHostAcq);
    AssertRC(rc2);

    /*
     * Init guest stream.
     */

    /* Destroy any former mixing buffer. */
    AudioMixBufDestroy(&pGstStream->MixBuf);

    /* Set the guests's default audio data layout. */
    pCfgHost->enmLayout = PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED;

    /* Make sure to (re-)set the guest buffer's shift size. */
    pCfgGuest->Props.cShift = PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(pCfgGuest->Props.cBits, pCfgGuest->Props.cChannels);

    /* Set set guest buffer size multiplicator. */
    const unsigned cFrameBufferGuestFactor = 10; /** @todo Make this configurable. */

    LogFunc(("[%s] cFrames=%RU32 (x %u)\n", pGstStream->szName, CfgHostAcq.cFrameBufferHint, cFrameBufferGuestFactor));

    rc2 = AudioMixBufInit(&pGstStream->MixBuf, pGstStream->szName, &pCfgGuest->Props,
                          CfgHostAcq.cFrameBufferHint * cFrameBufferGuestFactor);
    AssertRC(rc2);

    if (pCfgGuest->enmDir == PDMAUDIODIR_IN)
    {
        /* Host (Parent) -> Guest (Child). */
        rc2 = AudioMixBufLinkTo(&pHstStream->MixBuf, &pGstStream->MixBuf);
        AssertRC(rc2);
    }
    else
    {
        /* Guest (Parent) -> Host (Child). */
        rc2 = AudioMixBufLinkTo(&pGstStream->MixBuf, &pHstStream->MixBuf);
        AssertRC(rc2);
    }

    /* Make a copy of the guest stream configuration. */
    rc2 = DrvAudioHlpStreamCfgCopy(&pGstStream->Cfg, pCfgGuest);
    AssertRC(rc2);

    if (RT_FAILURE(rc))
        LogRel2(("Audio: Creating stream '%s' failed with %Rrc\n", pStream->szName, rc));

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
    PPDMAUDIOSTREAM pHstStream;
    RTListForEach(&pThis->lstHstStreams, pHstStream, PDMAUDIOSTREAM, Node)
        pHstStream->fStatus |= PDMAUDIOSTREAMSTS_FLAG_PENDING_REINIT;

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

    PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
    AssertPtr(pHstStream);

    /*
     * Gather current stream status.
     */
    bool fIsEnabled = RT_BOOL(pHstStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_ENABLED); /* Stream is enabled? */

    /*
     * Destroy and re-create stream on backend side.
     */
    int rc = drvAudioStreamControlInternalBackend(pThis, pHstStream, PDMAUDIOSTREAMCMD_DISABLE);
    if (RT_SUCCESS(rc))
    {
        rc = drvAudioStreamDestroyInternalBackend(pThis, pHstStream);
        if (RT_SUCCESS(rc))
        {
            rc = drvAudioStreamCreateInternalBackend(pThis, pHstStream, &pHstStream->Cfg, NULL /* pCfgAcq */);
            /** @todo Validate (re-)acquired configuration with pHstStream->Cfg? */
        }
    }

    /*
     * Restore previous stream state.
     */
    if (RT_SUCCESS(rc))
    {
        PPDMAUDIOSTREAM pGstStream = pHstStream->pPair;

        if (fIsEnabled)
        {
            rc = drvAudioStreamControlInternalBackend(pThis, pHstStream, PDMAUDIOSTREAMCMD_ENABLE);
            if (RT_SUCCESS(rc))
            {
                if (pGstStream)
                {
                    /* Also reset the guest stream mixing buffer. */
                    AudioMixBufReset(&pGstStream->MixBuf);
                }
            }
        }

#ifdef VBOX_WITH_STATISTICS
        /*
         * Reset statistics.
         */
        if (RT_SUCCESS(rc))
        {
            if (pHstStream->enmDir == PDMAUDIODIR_IN)
            {
                STAM_COUNTER_RESET(&pHstStream->In.StatBytesElapsed);
                STAM_COUNTER_RESET(&pHstStream->In.StatBytesTotalRead);
                STAM_COUNTER_RESET(&pHstStream->In.StatFramesCaptured);

                if (pGstStream)
                {
                    Assert(pGstStream->enmDir == pHstStream->enmDir);

                    STAM_COUNTER_RESET(&pGstStream->In.StatBytesElapsed);
                    STAM_COUNTER_RESET(&pGstStream->In.StatBytesTotalRead);
                    STAM_COUNTER_RESET(&pGstStream->In.StatFramesCaptured);
                }
            }
            else if (pHstStream->enmDir == PDMAUDIODIR_OUT)
            {
                STAM_COUNTER_RESET(&pHstStream->Out.StatBytesElapsed);
                STAM_COUNTER_RESET(&pHstStream->Out.StatBytesTotalWritten);
                STAM_COUNTER_RESET(&pHstStream->Out.StatFramesPlayed);

                if (pGstStream)
                {
                    Assert(pGstStream->enmDir == pHstStream->enmDir);

                    STAM_COUNTER_RESET(&pGstStream->Out.StatBytesElapsed);
                    STAM_COUNTER_RESET(&pGstStream->Out.StatBytesTotalWritten);
                    STAM_COUNTER_RESET(&pGstStream->Out.StatFramesPlayed);
                }
            }
            else
                AssertFailed();
        }
#endif
    }

    if (RT_FAILURE(rc))
        LogRel2(("Audio: Re-initializing stream '%s' failed with %Rrc\n", pStream->szName, rc));

    LogFunc(("[%s] Returning %Rrc\n", pStream->szName, rc));
    return rc;
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
              ("Stream '%s' is not an output stream and therefore cannot be written to (direction is 0x%x)\n",
               pStream->szName, pStream->enmDir));

    uint32_t cbWrittenTotal = 0;

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return rc;

#ifdef VBOX_WITH_STATISTICS
    STAM_PROFILE_ADV_START(&pThis->Stats.DelayOut, out);
#endif

#ifdef LOG_ENABLED
    char *pszGstSts = NULL;
    char *pszHstSts = NULL;
#endif

    do
    {
        if (!pThis->Out.fEnabled)
        {
            cbWrittenTotal = cbBuf;
            break;
        }

        if (   pThis->pHostDrvAudio
            && pThis->pHostDrvAudio->pfnGetStatus
            && pThis->pHostDrvAudio->pfnGetStatus(pThis->pHostDrvAudio, PDMAUDIODIR_OUT) != PDMAUDIOBACKENDSTS_RUNNING)
        {
            rc = VERR_NOT_AVAILABLE;
            break;
        }

        PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
        if (!pHstStream)
        {
            rc = VERR_NOT_AVAILABLE;
            break;
        }

#ifdef LOG_ENABLED
        pszHstSts = dbgAudioStreamStatusToStr(pHstStream->fStatus);
        AssertPtr(pszHstSts);
#endif
        PPDMAUDIOSTREAM pGstStream = pHstStream->pPair;
        AssertPtr(pGstStream);

#ifdef LOG_ENABLED
        pszGstSts = dbgAudioStreamStatusToStr(pGstStream->fStatus);
        AssertPtr(pszGstSts);
#endif

        if (!(pGstStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_ENABLED))
        {
            Log3Func(("[%s] Writing to disabled guest output stream not possible (status is %s, host status %s)\n",
                      pGstStream->szName, pszGstSts, pszHstSts));
#ifdef DEBUG_andy
            AssertFailed();
#endif
            rc = VERR_NOT_AVAILABLE;
            break;
        }

        const uint32_t cbFree = AudioMixBufFreeBytes(&pGstStream->MixBuf);
        if (!cbFree) /* No free guest side buffer space, bail out. */
        {
            AssertMsgFailed(("[%s] Stream full\n", pGstStream->szName));
            break;
        }

        /* Do not attempt to write more than the guest side currently can handle. */
        if (cbBuf > cbFree)
            cbBuf = cbFree;

        /* We use the guest side mixing buffer as an intermediate buffer to do some
         * (first) processing (if needed), so always write the incoming data at offset 0. */
        uint32_t cfWritten = 0;
        rc = AudioMixBufWriteAt(&pGstStream->MixBuf, 0 /* offFrames */, pvBuf, cbBuf, &cfWritten);
        if (   RT_FAILURE(rc)
            || !cfWritten)
        {
            AssertMsgFailed(("[%s] Write failed: cbBuf=%RU32, cfWritten=%RU32, rc=%Rrc\n",
                             pGstStream->szName, cbBuf, cfWritten, rc));
            break;
        }

        if (pThis->Dbg.fEnabled)
            DrvAudioHlpFileWrite(pHstStream->Out.Dbg.pFileStreamWrite, pvBuf, cbBuf, 0 /* fFlags */);

#ifdef VBOX_WITH_STATISTICS
        STAM_COUNTER_ADD(&pThis->Stats.TotalFramesWritten, cfWritten);
#endif
        uint32_t cfMixed = 0;
        if (cfWritten)
        {
            int rc2 = AudioMixBufMixToParentEx(&pGstStream->MixBuf, 0 /* cSrcOffset */, cfWritten /* cSrcFrames */,
                                               &cfMixed /* pcSrcMixed */);
            if (   RT_FAILURE(rc2)
                || cfMixed < cfWritten)
            {
                AssertMsgFailed(("[%s] Mixing failed: cbBuf=%RU32, cfWritten=%RU32, cfMixed=%RU32, rc=%Rrc\n",
                                 pGstStream->szName, cbBuf, cfWritten, cfMixed, rc2));

                LogRel2(("Audio: Lost audio frames (%RU32) due to full host stream '%s', expect stuttering audio output\n",
                         cfWritten - cfMixed, pHstStream->szName));

                /* Keep going. */
            }

            if (RT_SUCCESS(rc))
                rc = rc2;

            cbWrittenTotal = AUDIOMIXBUF_F2B(&pGstStream->MixBuf, cfWritten);

#ifdef VBOX_WITH_STATISTICS
            STAM_COUNTER_ADD(&pThis->Stats.TotalFramesMixedOut,      cfMixed);
            Assert(cfWritten >= cfMixed);
            STAM_COUNTER_ADD(&pThis->Stats.TotalFramesLostOut,       cfWritten - cfMixed);
            STAM_COUNTER_ADD(&pThis->Stats.TotalBytesWritten,        cbWrittenTotal);
            STAM_COUNTER_ADD(&pGstStream->Out.StatBytesTotalWritten, cbWrittenTotal);
#endif
        }

        Log3Func(("[%s] cbBuf=%RU32, cUsed=%RU32, cLive=%RU32, cWritten=%RU32, cMixed=%RU32, rc=%Rrc\n",
                  pGstStream->szName,
                  cbBuf, AudioMixBufUsed(&pGstStream->MixBuf), AudioMixBufLive(&pGstStream->MixBuf), cfWritten, cfMixed, rc));

    } while (0);

#ifdef LOG_ENABLED
    RTStrFree(pszHstSts);
    RTStrFree(pszGstSts);
#endif

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    if (RT_SUCCESS(rc))
    {
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

    PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
    AssertPtr(pHstStream);
    PPDMAUDIOSTREAM pGstStream = pHstStream ? pHstStream->pPair : NULL;
    AssertPtr(pGstStream);

    int rc;

    /* Is the stream scheduled for re-initialization? Do so now. */
    if (pHstStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_PENDING_REINIT)
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
        pHstStream->fStatus &= ~PDMAUDIOSTREAMSTS_FLAG_PENDING_REINIT;

        rc = drvAudioStreamReInitInternal(pThis, pStream);
        if (RT_FAILURE(rc))
            return rc;
    }

#ifdef LOG_ENABLED
    char *pszHstSts = dbgAudioStreamStatusToStr(pHstStream->fStatus);
    Log3Func(("[%s] fStatus=%s\n", pHstStream->szName, pszHstSts));
    RTStrFree(pszHstSts);
#endif /* LOG_ENABLED */

    /* Not enabled or paused? Skip iteration. */
    if (   !(pHstStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_ENABLED)
        ||  (pHstStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_PAUSED))
    {
        return VINF_SUCCESS;
    }

    /* Whether to try closing a pending to close stream. */
    bool fTryClosePending = false;

    do
    {
        uint32_t cfMixed = 0;

        rc = pThis->pHostDrvAudio->pfnStreamIterate(pThis->pHostDrvAudio, pHstStream->pvBackend);
        if (RT_FAILURE(rc))
            break;

        if (pHstStream->enmDir == PDMAUDIODIR_IN)
        {
            /* Has the host captured any frames which were not mixed to the guest side yet? */
            uint32_t cfCaptured = AudioMixBufUsed(&pHstStream->MixBuf);
            if (cfCaptured)
            {
                /* When capturing frames, the guest is the parent while the host is the child.
                 * So try mixing not yet mixed host-side frames to the guest-side buffer. */
                rc = AudioMixBufMixToParent(&pHstStream->MixBuf, cfCaptured, &cfMixed);
                if (RT_FAILURE(rc))
                {
                    if (rc == VERR_BUFFER_OVERFLOW)
                        LogRel2(("Audio: Guest input stream '%s' full, expect stuttering audio capture\n", pGstStream->szName));
                    else
                        LogRel2(("Audio: Mixing to guest input stream '%s' failed: %Rrc\n", pGstStream->szName, rc));
#ifdef DEBUG_andy_disabled
                    AssertFailed();
#endif
                }

#ifdef VBOX_WITH_STATISTICS
                STAM_COUNTER_ADD(&pThis->Stats.TotalFramesMixedIn, cfMixed);
                Assert(cfCaptured >= cfMixed);
                STAM_COUNTER_ADD(&pThis->Stats.TotalFramesLostIn,  cfCaptured - cfMixed);
#endif
                Log3Func(("[%s] %RU32/%RU32 input frames mixed, rc=%Rrc\n", pHstStream->szName, cfMixed, cfCaptured, rc));
            }
            else
            {
                /* No audio frames to transfer host to the guest (anymore)?
                 * Then try closing this stream if marked so in the next block. */
                fTryClosePending = true;
            }
        }
        else if (pHstStream->enmDir == PDMAUDIODIR_OUT)
        {
            /* No audio frames to transfer from guest to host (anymore)?
             * Then try closing this stream if marked so in the next block. */
            fTryClosePending = AudioMixBufLive(&pHstStream->MixBuf) == 0;
        }
        else
            AssertFailedStmt(rc = VERR_NOT_IMPLEMENTED);

        /* Has the host stream marked as pending to disable?
         * Try disabling the stream then. */
        if (   pHstStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_PENDING_DISABLE
            && fTryClosePending)
        {
            if (pThis->pHostDrvAudio->pfnStreamGetPending) /* Optional to implement. */
            {
                const uint32_t cxPending = pThis->pHostDrvAudio->pfnStreamGetPending(pThis->pHostDrvAudio, pHstStream->pvBackend);
                Log3Func(("[%s] cxPending=%RU32\n", pHstStream->szName, cxPending));

                /* Only try close pending if no audio data is pending on the backend-side anymore. */
                fTryClosePending = cxPending == 0;
            }

            if (fTryClosePending)
            {
                LogFunc(("[%s] Closing pending stream\n", pHstStream->szName));
                rc = drvAudioStreamControlInternalBackend(pThis, pHstStream, PDMAUDIOSTREAMCMD_DISABLE);
                if (RT_FAILURE(rc))
                    LogFunc(("[%s] Backend vetoed against closing pending input stream, rc=%Rrc\n", pHstStream->szName, rc));
            }
        }

    } while (0);

    /* Update timestamps. */
    pHstStream->tsLastIterateMS = RTTimeMilliTS();
    pGstStream->tsLastIterateMS = RTTimeMilliTS();

    if (RT_FAILURE(rc))
        LogFunc(("[%s] Failed with %Rrc\n",  pHstStream->szName, rc));

    return rc;
}

/**
 * Links an audio stream to another audio stream (pair).
 *
 * @returns IPRT status code.
 * @param   pStream             Stream to handle linking for.
 * @param   pPair               Stream to link pStream to. Specify NULL to unlink pStream from a former linked stream.
 */
static int drvAudioStreamLinkToInternal(PPDMAUDIOSTREAM pStream, PPDMAUDIOSTREAM pPair)
{
    if (pPair) /* Link. */
    {
        pStream->pPair = pPair;
        pPair->pPair   = pStream;

        LogRel2(("Audio: Linked audio stream '%s' to '%s'\n", pStream->szName, pPair->szName));
    }
    else /* Unlink. */
    {
        if (pStream->pPair)
        {
            LogRel2(("Audio: Unlinked pair '%s' from stream '%s'\n", pStream->pPair->szName, pStream->szName));

            AssertMsg(pStream->pPair->pPair == pStream,
                      ("Pair '%s' is not linked to '%s' (linked to '%s')\n",
                       pStream->pPair->szName, pStream->szName, pStream->pPair->pPair ? pStream->pPair->pPair->szName : "<NONE>"));

            pStream->pPair->pPair = NULL; /* Also make sure to unlink the pair from pStream */
            pStream->pPair = NULL;
        }
    }

    return VINF_SUCCESS;
}

/**
 * Plays an audio host output stream which has been configured for non-interleaved (layout) data.
 *
 * @return  IPRT status code.
 * @param   pThis               Pointer to driver instance.
 * @param   pHstStream          Host stream to play.
 * @param   cfToPlay            Number of audio frames to play.
 * @param   pcfPlayed           Returns number of audio frames played. Optional.
 */
static int drvAudioStreamPlayNonInterleaved(PDRVAUDIO pThis,
                                            PPDMAUDIOSTREAM pHstStream, uint32_t cfToPlay, uint32_t *pcfPlayed)
{
    AssertPtrReturn(pThis,      VERR_INVALID_POINTER);
    AssertPtrReturn(pHstStream, VERR_INVALID_POINTER);
    /* pcfPlayed is optional. */

    /* Sanity. */
    Assert(pHstStream->enmCtx == PDMAUDIOSTREAMCTX_HOST);
    Assert(pHstStream->enmDir == PDMAUDIODIR_OUT);
    Assert(pHstStream->Cfg.enmLayout == PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED);

    if (!cfToPlay)
    {
        if (pcfPlayed)
            *pcfPlayed = 0;
        return VINF_SUCCESS;
    }

    int rc = VINF_SUCCESS;

    uint32_t cfPlayedTotal = 0;

    AssertPtr(pThis->pHostDrvAudio->pfnStreamGetWritable);
    uint32_t cbWritable = pThis->pHostDrvAudio->pfnStreamGetWritable(pThis->pHostDrvAudio, pHstStream->pvBackend);
    if (cbWritable)
    {
        if (cfToPlay > AUDIOMIXBUF_B2F(&pHstStream->MixBuf, cbWritable)) /* More frames available than we can write? Limit. */
            cfToPlay = AUDIOMIXBUF_B2F(&pHstStream->MixBuf, cbWritable);

        if (cfToPlay)
        {
            uint8_t auBuf[256]; /** @todo Get rid of this here. */

            uint32_t cbLeft  = AUDIOMIXBUF_F2B(&pHstStream->MixBuf, cfToPlay);
            uint32_t cbChunk = sizeof(auBuf);

            while (cbLeft)
            {
                uint32_t cfRead = 0;
                rc = AudioMixBufAcquireReadBlock(&pHstStream->MixBuf, auBuf, RT_MIN(cbChunk, cbLeft), &cfRead);
                if (RT_FAILURE(rc))
                    break;

                uint32_t cbRead = AUDIOMIXBUF_F2B(&pHstStream->MixBuf, cfRead);
                Assert(cbRead <= cbChunk);

                uint32_t cfPlayed = 0;
                uint32_t cbPlayed = 0;
                rc = pThis->pHostDrvAudio->pfnStreamPlay(pThis->pHostDrvAudio, pHstStream->pvBackend,
                                                         auBuf, cbRead, &cbPlayed);
                if (   RT_SUCCESS(rc)
                    && cbPlayed)
                {
                    if (pThis->Dbg.fEnabled)
                        DrvAudioHlpFileWrite(pHstStream->Out.Dbg.pFilePlayNonInterleaved, auBuf, cbPlayed, 0 /* fFlags */);

                    if (cbRead != cbPlayed)
                        LogRel2(("Audio: Host stream '%s' played wrong amount (%RU32 bytes read but played %RU32 (%RI32), writable was %RU32)\n",
                                 pHstStream->szName, cbRead, cbPlayed, cbRead - cbPlayed, cbWritable));

                    cfPlayed       = AUDIOMIXBUF_B2F(&pHstStream->MixBuf, cbPlayed);
                    cfPlayedTotal += cfPlayed;

                    Assert(cbLeft >= cbPlayed);
                    cbLeft        -= cbPlayed;
                }

                AudioMixBufReleaseReadBlock(&pHstStream->MixBuf, cfPlayed);

                if (RT_FAILURE(rc))
                    break;
            }
        }
    }

    Log3Func(("[%s] Played %RU32/%RU32 frames, rc=%Rrc\n", pHstStream->szName, cfPlayedTotal, cfToPlay, rc));

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
 * @param   pHstStream          Host stream to play.
 * @param   cfToPlay            Number of audio frames to play.
 * @param   pcfPlayed           Returns number of audio frames played. Optional.
 */
static int drvAudioStreamPlayRaw(PDRVAUDIO pThis,
                                 PPDMAUDIOSTREAM pHstStream, uint32_t cfToPlay, uint32_t *pcfPlayed)
{
    AssertPtrReturn(pThis,      VERR_INVALID_POINTER);
    AssertPtrReturn(pHstStream, VERR_INVALID_POINTER);
    /* pcfPlayed is optional. */

    /* Sanity. */
    Assert(pHstStream->enmCtx == PDMAUDIOSTREAMCTX_HOST);
    Assert(pHstStream->enmDir == PDMAUDIODIR_OUT);
    Assert(pHstStream->Cfg.enmLayout == PDMAUDIOSTREAMLAYOUT_RAW);

    if (!cfToPlay)
    {
        if (pcfPlayed)
            *pcfPlayed = 0;
        return VINF_SUCCESS;
    }

    int rc = VINF_SUCCESS;

    uint32_t cfPlayedTotal = 0;

    AssertPtr(pThis->pHostDrvAudio->pfnStreamGetWritable);
    uint32_t cfWritable = pThis->pHostDrvAudio->pfnStreamGetWritable(pThis->pHostDrvAudio, pHstStream->pvBackend);
    if (cfWritable)
    {
        if (cfToPlay > cfWritable) /* More frames available than we can write? Limit. */
            cfToPlay = cfWritable;

        PDMAUDIOFRAME aFrameBuf[_4K]; /** @todo Get rid of this here. */

        uint32_t cfLeft = cfToPlay;
        while (cfLeft)
        {
            uint32_t cfRead = 0;
            rc = AudioMixBufPeek(&pHstStream->MixBuf, cfLeft, aFrameBuf,
                                 RT_MIN(cfLeft, RT_ELEMENTS(aFrameBuf)), &cfRead);

            if (RT_SUCCESS(rc))
            {
                if (cfRead)
                {
                    uint32_t cfPlayed;

                    /* Note: As the stream layout is RPDMAUDIOSTREAMLAYOUT_RAW, operate on audio frames
                     *       rather on bytes. */
                    Assert(cfRead <= RT_ELEMENTS(aFrameBuf));
                    rc = pThis->pHostDrvAudio->pfnStreamPlay(pThis->pHostDrvAudio, pHstStream->pvBackend,
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
    }

    Log3Func(("[%s] Played %RU32/%RU32 frames, rc=%Rrc\n", pHstStream->szName, cfPlayedTotal, cfToPlay, rc));

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

    do
    {
        if (!pThis->pHostDrvAudio)
            break;

        PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
        AssertPtr(pHstStream);
        PPDMAUDIOSTREAM pGstStream = pHstStream ? pHstStream->pPair : NULL;
        AssertPtr(pGstStream);

        AssertReleaseMsgBreakStmt(pHstStream != NULL,
                                  ("[%s] Host stream is NULL (cRefs=%RU32, fStatus=%x, enmCtx=%ld)\n",
                                   pStream->szName, pStream->cRefs, pStream->fStatus, pStream->enmCtx),
                                  rc = VERR_NOT_AVAILABLE);
        AssertReleaseMsgBreakStmt(pGstStream != NULL,
                                  ("[%s] Guest stream is NULL (cRefs=%RU32, fStatus=%x, enmCtx=%ld)\n",
                                   pStream->szName, pStream->cRefs, pStream->fStatus, pStream->enmCtx),
                                  rc = VERR_NOT_AVAILABLE);

        /*
         * Check if the backend is ready to operate.
         */

        AssertPtr(pThis->pHostDrvAudio->pfnStreamGetStatus);
        PDMAUDIOSTREAMSTS stsBackend = pThis->pHostDrvAudio->pfnStreamGetStatus(pThis->pHostDrvAudio, pHstStream->pvBackend);
#ifdef LOG_ENABLED
        char *pszBackendSts = dbgAudioStreamStatusToStr(stsBackend);
        Log3Func(("[%s] Start: stsBackend=%s\n", pHstStream->szName, pszBackendSts));
        RTStrFree(pszBackendSts);
#endif /* LOG_ENABLED */
        if (!(stsBackend & PDMAUDIOSTREAMSTS_FLAG_ENABLED)) /* Backend disabled? Bail out. */
            break;

        uint32_t cfToPlay = AudioMixBufLive(&pHstStream->MixBuf);

        if (pThis->pHostDrvAudio->pfnStreamPlayBegin)
            pThis->pHostDrvAudio->pfnStreamPlayBegin(pThis->pHostDrvAudio, pHstStream->pvBackend);

        if (RT_LIKELY(pHstStream->Cfg.enmLayout == PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED))
        {
            rc = drvAudioStreamPlayNonInterleaved(pThis, pHstStream, cfToPlay, &cfPlayedTotal);
        }
        else if (pHstStream->Cfg.enmLayout == PDMAUDIOSTREAMLAYOUT_RAW)
        {
            rc = drvAudioStreamPlayRaw(pThis, pHstStream, cfToPlay, &cfPlayedTotal);
        }
        else
            AssertFailedStmt(rc = VERR_NOT_IMPLEMENTED);

        if (pThis->pHostDrvAudio->pfnStreamPlayEnd)
            pThis->pHostDrvAudio->pfnStreamPlayEnd(pThis->pHostDrvAudio, pHstStream->pvBackend);

#ifdef LOG_ENABLED
        uint32_t cfLive = 0;
#endif
        if (RT_SUCCESS(rc))
        {
            AudioMixBufFinish(&pHstStream->MixBuf, cfPlayedTotal);

#ifdef VBOX_WITH_STATISTICS
            STAM_COUNTER_ADD     (&pThis->Stats.TotalFramesOut, cfPlayedTotal);
            STAM_PROFILE_ADV_STOP(&pThis->Stats.DelayOut, out);
            STAM_COUNTER_ADD     (&pHstStream->Out.StatFramesPlayed, cfPlayedTotal);
#endif

#ifdef LOG_ENABLED
            cfLive = AudioMixBufLive(&pHstStream->MixBuf);
#endif
        }

#ifdef LOG_ENABLED
        pszBackendSts = dbgAudioStreamStatusToStr(stsBackend);
        Log3Func(("[%s] End: stsBackend=%s, cfLive=%RU32, cfPlayedTotal=%RU32, rc=%Rrc\n",
                  pHstStream->szName, pszBackendSts, cfLive, cfPlayedTotal, rc));
        RTStrFree(pszBackendSts);
#endif /* LOG_ENABLED */

    } while (0);

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
 * @param   pHstStream          Host stream to capture from.
 * @param   pcfCaptured         Number of (host) audio frames captured. Optional.
 */
static int drvAudioStreamCaptureNonInterleaved(PDRVAUDIO pThis, PPDMAUDIOSTREAM pHstStream, uint32_t *pcfCaptured)
{
    AssertPtrReturn(pThis,      VERR_INVALID_POINTER);
    AssertPtrReturn(pHstStream, VERR_INVALID_POINTER);
    /* pcfCaptured is optional. */

    /* Sanity. */
    Assert(pHstStream->enmCtx == PDMAUDIOSTREAMCTX_HOST);
    Assert(pHstStream->enmDir == PDMAUDIODIR_IN);
    Assert(pHstStream->Cfg.enmLayout == PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED);

    int rc = VINF_SUCCESS;

    uint32_t cfCapturedTotal = 0;

    AssertPtr(pThis->pHostDrvAudio->pfnStreamGetReadable);

    uint8_t auBuf[_1K]; /** @todo Get rid of this. */
    size_t  cbBuf = sizeof(auBuf);

    for (;;)
    {
        uint32_t cbReadable = pThis->pHostDrvAudio->pfnStreamGetReadable(pThis->pHostDrvAudio, pHstStream->pvBackend);
        if (!cbReadable)
        {
            Log2Func(("[%s] No readable data available, skipping\n", pHstStream->szName));
            break;
        }

        uint32_t cbFree = AUDIOMIXBUF_F2B(&pHstStream->MixBuf, AudioMixBufFree(&pHstStream->MixBuf));
        if (!cbFree)
        {
            Log2Func(("[%s] Host buffer full, skipping\n", pHstStream->szName));
            break;
        }

        if (cbFree < cbReadable) /* More data captured than we can read? */
        {
            /** @todo Warn? */
        }

        if (cbFree > cbBuf) /* Limit to buffer size. */
            cbFree = (uint32_t)cbBuf;

        uint32_t cbCaptured;
        rc = pThis->pHostDrvAudio->pfnStreamCapture(pThis->pHostDrvAudio, pHstStream->pvBackend,
                                                    auBuf, cbFree, &cbCaptured);
        if (RT_FAILURE(rc))
        {
            int rc2 = drvAudioStreamControlInternalBackend(pThis, pHstStream, PDMAUDIOSTREAMCMD_DISABLE);
            AssertRC(rc2);
        }
        else if (cbCaptured)
        {
            if (pThis->Dbg.fEnabled)
                DrvAudioHlpFileWrite(pHstStream->In.Dbg.pFileCaptureNonInterleaved, auBuf, cbCaptured, 0 /* fFlags */);

            Assert(cbCaptured <= cbBuf);
            if (cbCaptured > cbBuf) /* Paranoia. */
                cbCaptured = (uint32_t)cbBuf;

            uint32_t cfCaptured = 0;
            rc = AudioMixBufWriteCirc(&pHstStream->MixBuf, auBuf, cbCaptured, &cfCaptured);
            if (RT_SUCCESS(rc))
                cfCapturedTotal += cfCaptured;
        }
        else /* Nothing captured -- bail out. */
            break;

        if (RT_FAILURE(rc))
            break;
    }

    if (RT_SUCCESS(rc))
    {
        if (cfCapturedTotal)
            Log2Func(("[%s] %RU32 frames captured, rc=%Rrc\n", pHstStream->szName, cfCapturedTotal, rc));
    }
    else
        LogFunc(("[%s] Capturing failed with rc=%Rrc\n", pHstStream->szName, rc));

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
 * @param   pHstStream          Host stream to capture from.
 * @param   pcfCaptured         Number of (host) audio frames captured. Optional.
 */
static int drvAudioStreamCaptureRaw(PDRVAUDIO pThis, PPDMAUDIOSTREAM pHstStream, uint32_t *pcfCaptured)
{
    AssertPtrReturn(pThis,      VERR_INVALID_POINTER);
    AssertPtrReturn(pHstStream, VERR_INVALID_POINTER);
    /* pcfCaptured is optional. */

    /* Sanity. */
    Assert(pHstStream->enmCtx == PDMAUDIOSTREAMCTX_HOST);
    Assert(pHstStream->enmDir == PDMAUDIODIR_IN);
    Assert(pHstStream->Cfg.enmLayout == PDMAUDIOSTREAMLAYOUT_RAW);

    int rc = VINF_SUCCESS;

    uint32_t cfCapturedTotal = 0;

    AssertPtr(pThis->pHostDrvAudio->pfnStreamGetReadable);

    for (;;)
    {
        uint32_t cbReadable = pThis->pHostDrvAudio->pfnStreamGetReadable(pThis->pHostDrvAudio, pHstStream->pvBackend);
        if (!cbReadable) /* Nothing to read on the backend side? Bail out. */
            break;

        const uint32_t cbFree = AudioMixBufFreeBytes(&pHstStream->MixBuf);
        if (!cbFree) /* No space left in the host stream? */
            break;

        if (cbReadable > cbFree) /* Don't capture more than the host stream currently can hold. */
            cbReadable = cbFree;

        PPDMAUDIOFRAME paFrames;
        uint32_t cfWritable;
        rc = AudioMixBufPeekMutable(&pHstStream->MixBuf, AUDIOMIXBUF_B2F(&pHstStream->MixBuf, cbReadable),
                                    &paFrames, &cfWritable);
        if (   RT_FAILURE(rc)
            || !cfWritable)
        {
            break;
        }

        uint32_t cfCaptured;
        rc = pThis->pHostDrvAudio->pfnStreamCapture(pThis->pHostDrvAudio, pHstStream->pvBackend,
                                                    paFrames, cfWritable, &cfCaptured);
        if (RT_FAILURE(rc))
        {
            int rc2 = drvAudioStreamControlInternalBackend(pThis, pHstStream, PDMAUDIOSTREAMCMD_DISABLE);
            AssertRC(rc2);
        }
        else if (cfCaptured)
        {
            Assert(cfCaptured <= cfWritable);
            if (cfCaptured > cfWritable) /* Paranoia. */
                cfCaptured = cfWritable;

            cfCapturedTotal += cfCaptured;
        }
        else /* Nothing captured -- bail out. */
            break;

        if (RT_FAILURE(rc))
            break;
    }

    Log2Func(("[%s] %RU32 frames captured, rc=%Rrc\n", pHstStream->szName, cfCapturedTotal, rc));

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

    do
    {
        if (!pThis->pHostDrvAudio)
            break;

        PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
        AssertPtr(pHstStream);
        PPDMAUDIOSTREAM pGstStream = pHstStream ? pHstStream->pPair : NULL;
        AssertPtr(pGstStream);

        AssertReleaseMsgBreakStmt(pHstStream != NULL,
                                  ("[%s] Host stream is NULL (cRefs=%RU32, fStatus=%x, enmCtx=%ld)\n",
                                   pStream->szName, pStream->cRefs, pStream->fStatus, pStream->enmCtx),
                                  rc = VERR_NOT_AVAILABLE);
        AssertReleaseMsgBreakStmt(pGstStream != NULL,
                                  ("[%s] Guest stream is NULL (cRefs=%RU32, fStatus=%x, enmCtx=%ld)\n",
                                   pStream->szName, pStream->cRefs, pStream->fStatus, pStream->enmCtx),
                                  rc = VERR_NOT_AVAILABLE);

        /*
         * Check if the backend is ready to operate.
         */

        AssertPtr(pThis->pHostDrvAudio->pfnStreamGetStatus);
        PDMAUDIOSTREAMSTS stsBackend = pThis->pHostDrvAudio->pfnStreamGetStatus(pThis->pHostDrvAudio, pHstStream->pvBackend);
#ifdef LOG_ENABLED
        char *pszBackendSts = dbgAudioStreamStatusToStr(stsBackend);
        Log3Func(("[%s] Start: stsBackend=%s\n", pHstStream->szName, pszBackendSts));
        RTStrFree(pszBackendSts);
#endif /* LOG_ENABLED */
        if (!(stsBackend & PDMAUDIOSTREAMSTS_FLAG_ENABLED)) /* Backend disabled? Bail out. */
            break;

        /*
         * Do the actual capturing.
         */

        if (pThis->pHostDrvAudio->pfnStreamCaptureBegin)
            pThis->pHostDrvAudio->pfnStreamCaptureBegin(pThis->pHostDrvAudio, pHstStream->pvBackend);

        if (RT_LIKELY(pHstStream->Cfg.enmLayout == PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED))
        {
            rc = drvAudioStreamCaptureNonInterleaved(pThis, pHstStream, &cfCaptured);
        }
        else if (pHstStream->Cfg.enmLayout == PDMAUDIOSTREAMLAYOUT_RAW)
        {
            rc = drvAudioStreamCaptureRaw(pThis, pHstStream, &cfCaptured);
        }
        else
            AssertFailedStmt(rc = VERR_NOT_IMPLEMENTED);

        if (pThis->pHostDrvAudio->pfnStreamCaptureEnd)
            pThis->pHostDrvAudio->pfnStreamCaptureEnd(pThis->pHostDrvAudio, pHstStream->pvBackend);

#ifdef LOG_ENABLED
        pszBackendSts = dbgAudioStreamStatusToStr(stsBackend);
        Log3Func(("[%s] End: stsBackend=%s, cfCaptured=%RU32, rc=%Rrc\n",
                  pHstStream->szName, pszBackendSts, cfCaptured, rc));
        RTStrFree(pszBackendSts);
#endif /* LOG_ENABLED */

        if (RT_SUCCESS(rc))
        {
            Log3Func(("[%s] %RU32 frames captured, rc=%Rrc\n", pHstStream->szName, cfCaptured, rc));

#ifdef VBOX_WITH_STATISTICS
            STAM_COUNTER_ADD(&pThis->Stats.TotalFramesIn,        cfCaptured);
            STAM_COUNTER_ADD(&pHstStream->In.StatFramesCaptured, cfCaptured);
#endif
        }
        else if (RT_UNLIKELY(RT_FAILURE(rc)))
        {
            LogRel2(("Audio: Capturing stream '%s' failed with %Rrc\n", pHstStream->szName, rc));
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
            LogRel(("Audio: Host audio device configuration has changed\n"));
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
                LogRel(("Audio: Found %RU16 devices\n", DevEnum.cDevices));

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
                LogRel(("Audio: Device enumeration failed with %Rrc\n", rc));
            /* Not fatal. */
        }
    }
    else
    {
        rc = VERR_NOT_SUPPORTED;

        if (fLog)
            LogRel3(("Audio: Host audio backend does not support audio device enumeration, skipping\n"));
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
        LogRel(("Audio: Initialization of host backend failed with %Rrc\n", rc));
        return VERR_AUDIO_BACKEND_INIT_FAILED;
    }

    /*
     * Get the backend configuration.
     */
    rc = pThis->pHostDrvAudio->pfnGetConfig(pThis->pHostDrvAudio, &pThis->BackendCfg);
    if (RT_FAILURE(rc))
    {
        LogRel(("Audio: Getting host backend configuration failed with %Rrc\n", rc));
        return VERR_AUDIO_BACKEND_INIT_FAILED;
    }

    pThis->In.cStreamsFree  = pThis->BackendCfg.cMaxStreamsIn;
    pThis->Out.cStreamsFree = pThis->BackendCfg.cMaxStreamsOut;

    LogFlowFunc(("cStreamsFreeIn=%RU8, cStreamsFreeOut=%RU8\n", pThis->In.cStreamsFree, pThis->Out.cStreamsFree));

    LogRel2(("Audio: Host audio backend supports %RU32 input streams and %RU32 output streams at once\n",
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
             LogRel(("Audio: Error registering backend callback, rc=%Rrc\n", rc2));
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
        PPDMAUDIOSTREAM pHstStream;
        RTListForEach(&pThis->lstHstStreams, pHstStream, PDMAUDIOSTREAM, Node)
            drvAudioStreamControlInternalBackend(pThis, pHstStream, enmCmd);
    }

    rc2 = RTCritSectLeave(&pThis->CritSect);
    AssertRC(rc2);
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

    LogRel2(("Audio: Verbose logging enabled\n"));

    PDRVAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVAUDIO);

    int rc = RTCritSectInit(&pThis->CritSect);
    AssertRCReturn(rc, rc);

    /*
     * Configure driver from CFGM stuff.
     */
#ifdef DEBUG
    CFGMR3Dump(pCfgHandle);
#endif

    int rc2 = CFGMR3QueryString(pCfgHandle, "DriverName", pThis->szName, sizeof(pThis->szName));
    if (RT_FAILURE(rc2))
        RTStrPrintf(pThis->szName, sizeof(pThis->szName), "Untitled");

    /* By default we don't enable anything if wrongly / not set-up. */
    CFGMR3QueryBoolDef(pCfgHandle, "InputEnabled",  &pThis->In.fEnabled,   false);
    CFGMR3QueryBoolDef(pCfgHandle, "OutputEnabled", &pThis->Out.fEnabled,  false);

    CFGMR3QueryBoolDef(pCfgHandle, "DebugEnabled",      &pThis->Dbg.fEnabled,  false);
    rc2 = CFGMR3QueryString(pCfgHandle, "DebugPathOut", pThis->Dbg.szPathOut, sizeof(pThis->Dbg.szPathOut));
    if (   RT_FAILURE(rc2)
        || !strlen(pThis->Dbg.szPathOut))
    {
        RTStrPrintf(pThis->Dbg.szPathOut, sizeof(pThis->Dbg.szPathOut), VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH);
    }

    if (pThis->Dbg.fEnabled)
        LogRel(("Audio: Debugging enabled (audio data written to '%s')\n", pThis->Dbg.szPathOut));

    LogRel2(("Audio: Initial status for driver '%s': Input is %s, output is %s\n",
             pThis->szName, pThis->In.fEnabled ? "enabled" : "disabled", pThis->Out.fEnabled ? "enabled" : "disabled"));

    /*
     * If everything went well, initialize the lower driver.
     */
    rc = drvAudioHostInit(pThis, pCfgHandle);

    LogFlowFuncLeaveRC(rc);
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
        if (!pThis->In.fEnabled)
        {
            RT_BZERO(pvBuf, cbBuf);
            cbReadTotal = cbBuf;
            break;
        }

        if (   pThis->pHostDrvAudio
            && pThis->pHostDrvAudio->pfnGetStatus
            && pThis->pHostDrvAudio->pfnGetStatus(pThis->pHostDrvAudio, PDMAUDIODIR_IN) != PDMAUDIOBACKENDSTS_RUNNING)
        {
            rc = VERR_NOT_AVAILABLE;
            break;
        }

        PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
        if (!pHstStream)
        {
            rc = VERR_NOT_AVAILABLE;
            break;
        }

        PPDMAUDIOSTREAM pGstStream = pHstStream->pPair;
        AssertPtr(pGstStream);

        /*
         * Read from the parent buffer (that is, the guest buffer) which
         * should have the audio data in the format the guest needs.
         */
        uint32_t cReadTotal = 0;

        uint32_t cToRead = RT_MIN(AUDIOMIXBUF_B2F(&pGstStream->MixBuf, cbBuf), AudioMixBufUsed(&pGstStream->MixBuf));
        while (cToRead)
        {
            uint32_t cRead;
            rc = AudioMixBufAcquireReadBlock(&pGstStream->MixBuf, (uint8_t *)pvBuf + AUDIOMIXBUF_F2B(&pGstStream->MixBuf, cReadTotal),
                                             AUDIOMIXBUF_F2B(&pGstStream->MixBuf, cToRead), &cRead);
            if (RT_FAILURE(rc))
                break;

#if defined (VBOX_WITH_STATISTICS) || defined (VBOX_AUDIO_DEBUG_DUMP_PCM_DATA)
            const uint32_t cbRead = AUDIOMIXBUF_F2B(&pGstStream->MixBuf, cRead);
#endif

#ifdef VBOX_WITH_STATISTICS
            STAM_COUNTER_ADD(&pThis->Stats.TotalBytesRead,       cbRead);
            STAM_COUNTER_ADD(&pGstStream->In.StatBytesTotalRead, cbRead);
#endif
            Assert(cToRead >= cRead);
            cToRead -= cRead;

            cReadTotal += cRead;

            AudioMixBufReleaseReadBlock(&pGstStream->MixBuf, cRead);
        }

        if (cReadTotal)
        {
            if (pThis->Dbg.fEnabled)
                DrvAudioHlpFileWrite(pHstStream->In.Dbg.pFileStreamRead,
                                     pvBuf, AUDIOMIXBUF_F2B(&pGstStream->MixBuf, cReadTotal), 0 /* fFlags */);

            AudioMixBufFinish(&pGstStream->MixBuf, cReadTotal);

            pGstStream->In.tsLastReadMS = RTTimeMilliTS();

            cbReadTotal = AUDIOMIXBUF_F2B(&pGstStream->MixBuf, cReadTotal);
        }

    } while (0);

    Log3Func(("[%s] cbReadTotal=%RU32, rc=%Rrc\n", pStream->szName, cbReadTotal, rc));

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

    /*
     * The guest stream always will get the audio stream configuration told
     * by the device emulation (which in turn was/could be set by the guest OS).
     */
    PPDMAUDIOSTREAM pGstStrm = NULL;

    /** @todo Docs! */
    PPDMAUDIOSTREAM pHstStrm = NULL;

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
        size_t cbHstStrm = 0;
        if (pCfgHost->enmDir == PDMAUDIODIR_IN)
        {
            if (!pThis->In.cStreamsFree)
                LogFunc(("Warning: No more input streams free to use\n"));

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

        pHstStrm = (PPDMAUDIOSTREAM)RTMemAllocZ(sizeof(PDMAUDIOSTREAM));
        AssertPtrBreakStmt(pHstStrm, rc = VERR_NO_MEMORY);

        if (cbHstStrm) /* High unlikely that backends do not have an own space for data, but better check. */
        {
            pHstStrm->pvBackend = RTMemAllocZ(cbHstStrm);
            AssertPtrBreakStmt(pHstStrm->pvBackend, rc = VERR_NO_MEMORY);

            pHstStrm->cbBackend = cbHstStrm;
        }

        pHstStrm->enmCtx = PDMAUDIOSTREAMCTX_HOST;
        pHstStrm->enmDir = pCfgHost->enmDir;

        pGstStrm = (PPDMAUDIOSTREAM)RTMemAllocZ(sizeof(PDMAUDIOSTREAM));
        AssertPtrBreakStmt(pGstStrm, rc = VERR_NO_MEMORY);

        pGstStrm->enmCtx = PDMAUDIOSTREAMCTX_GUEST;
        pGstStrm->enmDir = pCfgGuest->enmDir;

        /* Retrieve host driver name for easier identification. */
        AssertPtr(pThis->pHostDrvAudio);
        PPDMDRVINS pDrvAudioInst = PDMIBASE_2_PDMDRV(pThis->pDrvIns->pDownBase);
        AssertPtr(pDrvAudioInst);
        AssertPtr(pDrvAudioInst->pReg);

        char szDriver[64];
        RTStrPrintf(szDriver, RT_ELEMENTS(szDriver), "%s", pDrvAudioInst->pReg->szName);
        if (!strlen(szDriver))
        {
            RTStrPrintf(szDriver, RT_ELEMENTS(szDriver), "Untitled");
            AssertFailed(); /* Should never happen. */
        }

        /*
         * Init host stream.
         */
        RTStrPrintf(pHstStrm->szName, RT_ELEMENTS(pHstStrm->szName), "[%s] %s (Host)",
                    szDriver, strlen(pCfgHost->szName) ? pCfgHost->szName : "<Untitled>");

        rc = drvAudioStreamLinkToInternal(pHstStrm, pGstStrm);
        AssertRCBreak(rc);

        /*
         * Init guest stream.
         */
        RTStrPrintf(pGstStrm->szName, RT_ELEMENTS(pGstStrm->szName), "[%s] %s (Guest)",
                    szDriver, strlen(pCfgGuest->szName) ? pCfgGuest->szName : "<Untitled>");

        pGstStrm->fStatus = pHstStrm->fStatus; /* Reflect the host stream's status. */

        rc = drvAudioStreamLinkToInternal(pGstStrm, pHstStrm);
        AssertRCBreak(rc);

        /*
         * Try to init the rest.
         */
        rc = drvAudioStreamInitInternal(pThis, pHstStrm, pCfgHost, pCfgGuest);
        if (RT_FAILURE(rc))
            break;

#ifdef VBOX_WITH_STATISTICS
        char szStatName[255];

        if (pCfgGuest->enmDir == PDMAUDIODIR_IN)
        {
            RTStrPrintf(szStatName, sizeof(szStatName), "Guest/%s/BytesElapsed", pGstStrm->szName);
            PDMDrvHlpSTAMRegCounterEx(pThis->pDrvIns, &pGstStrm->In.StatBytesElapsed,
                                      szStatName, STAMUNIT_BYTES, "Elapsed bytes read.");

            RTStrPrintf(szStatName, sizeof(szStatName), "Guest/%s/BytesRead", pGstStrm->szName);
            PDMDrvHlpSTAMRegCounterEx(pThis->pDrvIns, &pGstStrm->In.StatBytesTotalRead,
                                      szStatName, STAMUNIT_BYTES, "Total bytes read.");

            RTStrPrintf(szStatName, sizeof(szStatName), "Host/%s/FramesCaptured", pHstStrm->szName);
            PDMDrvHlpSTAMRegCounterEx(pThis->pDrvIns, &pHstStrm->In.StatFramesCaptured,
                                      szStatName, STAMUNIT_COUNT, "Total frames captured.");
        }
        else if (pCfgGuest->enmDir == PDMAUDIODIR_OUT)
        {
            RTStrPrintf(szStatName, sizeof(szStatName), "Guest/%s/BytesElapsed", pGstStrm->szName);
            PDMDrvHlpSTAMRegCounterEx(pThis->pDrvIns, &pGstStrm->Out.StatBytesElapsed,
                                      szStatName, STAMUNIT_BYTES, "Elapsed bytes written.");

            RTStrPrintf(szStatName, sizeof(szStatName), "Guest/%s/BytesWritten", pGstStrm->szName);
            PDMDrvHlpSTAMRegCounterEx(pThis->pDrvIns, &pGstStrm->Out.StatBytesTotalWritten,
                                      szStatName, STAMUNIT_BYTES, "Total bytes written.");

            RTStrPrintf(szStatName, sizeof(szStatName), "Host/%s/FramesPlayed", pHstStrm->szName);
            PDMDrvHlpSTAMRegCounterEx(pThis->pDrvIns, &pHstStrm->Out.StatFramesPlayed,
                                      szStatName, STAMUNIT_COUNT, "Total frames played.");
        }
        else
            AssertFailed();
#endif

    } while (0);

#undef RC_BREAK

    if (RT_FAILURE(rc))
    {
        if (pGstStrm)
        {
            int rc2 = drvAudioStreamUninitInternal(pThis, pGstStrm);
            if (RT_SUCCESS(rc2))
            {
                RTMemFree(pGstStrm);
                pGstStrm = NULL;
            }
        }

        if (pHstStrm)
        {
            int rc2 = drvAudioStreamUninitInternal(pThis, pHstStrm);
            if (RT_SUCCESS(rc2))
            {
                drvAudioStreamFree(pHstStrm);
                pHstStrm = NULL;
            }
        }
    }
    else
    {
        /* Set initial reference counts. */
        RTListAppend(&pThis->lstGstStreams, &pGstStrm->Node);
        pGstStrm->cRefs = 1;

        RTListAppend(&pThis->lstHstStreams, &pHstStrm->Node);
        pHstStrm->cRefs = 1;

        if (pCfgHost->enmDir == PDMAUDIODIR_IN)
        {
            if (pThis->Dbg.fEnabled)
            {
                char szFile[RTPATH_MAX + 1];

                int rc2 = DrvAudioHlpGetFileName(szFile, RT_ELEMENTS(szFile), pThis->Dbg.szPathOut, "CaptureNonInterleaved",
                                                 pThis->pDrvIns->iInstance, PDMAUDIOFILETYPE_WAV, PDMAUDIOFILENAME_FLAG_NONE);
                if (RT_SUCCESS(rc2))
                {
                    rc2 = DrvAudioHlpFileCreate(PDMAUDIOFILETYPE_WAV, szFile, PDMAUDIOFILE_FLAG_NONE,
                                                &pHstStrm->In.Dbg.pFileCaptureNonInterleaved);
                    if (RT_SUCCESS(rc2))
                        rc2 = DrvAudioHlpFileOpen(pHstStrm->In.Dbg.pFileCaptureNonInterleaved, PDMAUDIOFILE_DEFAULT_OPEN_FLAGS,
                                                  &pHstStrm->Cfg.Props);
                }

                if (RT_SUCCESS(rc2))
                {
                    rc2 = DrvAudioHlpGetFileName(szFile, RT_ELEMENTS(szFile), pThis->Dbg.szPathOut, "StreamRead",
                                                 pThis->pDrvIns->iInstance, PDMAUDIOFILETYPE_WAV, PDMAUDIOFILENAME_FLAG_NONE);
                    if (RT_SUCCESS(rc2))
                    {
                        rc2 = DrvAudioHlpFileCreate(PDMAUDIOFILETYPE_WAV, szFile, PDMAUDIOFILE_FLAG_NONE,
                                                    &pHstStrm->In.Dbg.pFileStreamRead);
                        if (RT_SUCCESS(rc2))
                            rc2 = DrvAudioHlpFileOpen(pHstStrm->In.Dbg.pFileStreamRead, PDMAUDIOFILE_DEFAULT_OPEN_FLAGS,
                                                      &pHstStrm->Cfg.Props);
                    }
                }
            }

            if (pThis->In.cStreamsFree)
                pThis->In.cStreamsFree--;
        }
        else /* Out */
        {
            if (pThis->Dbg.fEnabled)
            {
                char szFile[RTPATH_MAX + 1];

                int rc2 = DrvAudioHlpGetFileName(szFile, RT_ELEMENTS(szFile), pThis->Dbg.szPathOut, "PlayNonInterleaved",
                                                 pThis->pDrvIns->iInstance, PDMAUDIOFILETYPE_WAV, PDMAUDIOFILENAME_FLAG_NONE);
                if (RT_SUCCESS(rc2))
                {
                    rc2 = DrvAudioHlpFileCreate(PDMAUDIOFILETYPE_WAV, szFile, PDMAUDIOFILE_FLAG_NONE,
                                                &pHstStrm->Out.Dbg.pFilePlayNonInterleaved);
                    if (RT_SUCCESS(rc2))
                        rc = DrvAudioHlpFileOpen(pHstStrm->Out.Dbg.pFilePlayNonInterleaved, PDMAUDIOFILE_DEFAULT_OPEN_FLAGS,
                                                 &pHstStrm->Cfg.Props);
                }

                if (RT_SUCCESS(rc2))
                {
                    rc2 = DrvAudioHlpGetFileName(szFile, RT_ELEMENTS(szFile), pThis->Dbg.szPathOut, "StreamWrite",
                                                 pThis->pDrvIns->iInstance, PDMAUDIOFILETYPE_WAV, PDMAUDIOFILENAME_FLAG_NONE);
                    if (RT_SUCCESS(rc2))
                    {
                        rc2 = DrvAudioHlpFileCreate(PDMAUDIOFILETYPE_WAV, szFile, PDMAUDIOFILE_FLAG_NONE,
                                                    &pHstStrm->Out.Dbg.pFileStreamWrite);
                        if (RT_SUCCESS(rc2))
                            rc2 = DrvAudioHlpFileOpen(pHstStrm->Out.Dbg.pFileStreamWrite, PDMAUDIOFILE_DEFAULT_OPEN_FLAGS,
                                                      &pHstStrm->Cfg.Props);
                    }
                }
            }

            if (pThis->Out.cStreamsFree)
                pThis->Out.cStreamsFree--;
        }

#ifdef VBOX_WITH_STATISTICS
        STAM_COUNTER_ADD(&pThis->Stats.TotalStreamsCreated, 1);
#endif
        /* Always return the guest-side part to the device emulation. */
        *ppStream = pGstStrm;
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
        PPDMAUDIOSTREAM pStream;
        RTListForEach(&pThis->lstHstStreams, pStream, PDMAUDIOSTREAM, Node)
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
        AssertFailed();

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
        if (   pThis->pHostDrvAudio
            && pThis->pHostDrvAudio->pfnGetStatus)
        {
             backendSts = pThis->pHostDrvAudio->pfnGetStatus(pThis->pHostDrvAudio, enmDir);
        }

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

    PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
    if (!pHstStream) /* No host stream available? Bail out early. */
    {
        rc2 = RTCritSectLeave(&pThis->CritSect);
        AssertRC(rc2);

        return 0;
    }

    uint32_t cReadable = 0;

    PPDMAUDIOSTREAM pGstStream = pHstStream->pPair;
    if (pGstStream)
        cReadable = AudioMixBufLive(&pGstStream->MixBuf);

    Log3Func(("[%s] cbReadable=%RU32 (%zu bytes)\n", pHstStream->szName, cReadable,
              AUDIOMIXBUF_F2B(&pGstStream->MixBuf, cReadable)));

    uint32_t cbReadable = AUDIOMIXBUF_F2B(&pGstStream->MixBuf, cReadable);

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

    PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
    if (!pHstStream) /* No host stream available? Bail out early. */
    {
        rc2 = RTCritSectLeave(&pThis->CritSect);
        AssertRC(rc2);

        AssertMsgFailed(("Guest stream '%s' does not have a host stream attached\n", pStream->szName));
        return 0;
    }

    /* As the host side sets the overall pace, return the writable bytes from that side. */
    uint32_t cbWritable = AudioMixBufFreeBytes(&pHstStream->MixBuf);

    Log3Func(("[%s] cbWritable=%RU32\n", pHstStream->szName, cbWritable));

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

    PDMAUDIOSTREAMSTS strmSts = PDMAUDIOSTREAMSTS_FLAG_NONE;

    PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
    if (pHstStream)
    {
        strmSts = pHstStream->fStatus;
#ifdef LOG_ENABLED
        char *pszHstSts = dbgAudioStreamStatusToStr(pHstStream->fStatus);
        Log3Func(("[%s] %s\n", pHstStream->szName, pszHstSts));
        RTStrFree(pszHstSts);
#endif
    }

    rc2 = RTCritSectLeave(&pThis->CritSect);
    AssertRC(rc2);

    return strmSts;
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

    PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
    PPDMAUDIOSTREAM pGstStream = pHstStream ? pHstStream->pPair : pStream;

    AudioMixBufSetVolume(&pHstStream->MixBuf, pVol);
    AudioMixBufSetVolume(&pGstStream->MixBuf, pVol);
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

    PDMAUDIODIR enmDir = pStream->enmDir;

    LogFlowFunc(("[%s] cRefs=%RU32\n", pStream->szName, pStream->cRefs));
    if (pStream->cRefs > 1)
        rc = VERR_WRONG_ORDER;

    if (RT_SUCCESS(rc))
    {
        PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
        PPDMAUDIOSTREAM pGstStream = pHstStream ? pHstStream->pPair : pStream;

        LogRel2(("Audio: Destroying host stream '%s' (guest stream '%s')\n",
                 pHstStream ? pHstStream->szName : "<None>",
                 pGstStream ? pGstStream->szName : "<None>"));

        /* Should prevent double frees. */
        Assert(pHstStream != pGstStream);

        if (pHstStream)
        {
            rc = drvAudioStreamUninitInternal(pThis, pHstStream);
            if (RT_SUCCESS(rc))
            {
                if (pHstStream->enmDir == PDMAUDIODIR_IN)
                {
#ifdef VBOX_WITH_STATISTICS
                    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pHstStream->In.StatFramesCaptured);
#endif
                    if (pThis->Dbg.fEnabled)
                    {
                        DrvAudioHlpFileDestroy(pHstStream->In.Dbg.pFileCaptureNonInterleaved);
                        DrvAudioHlpFileDestroy(pHstStream->In.Dbg.pFileStreamRead);
                    }
                }
                else if (pHstStream->enmDir == PDMAUDIODIR_OUT)
                {
#ifdef VBOX_WITH_STATISTICS
                    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pHstStream->Out.StatFramesPlayed);
#endif
                    if (pThis->Dbg.fEnabled)
                    {
                        DrvAudioHlpFileDestroy(pHstStream->Out.Dbg.pFilePlayNonInterleaved);
                        DrvAudioHlpFileDestroy(pHstStream->Out.Dbg.pFileStreamWrite);
                    }
                }
                else
                    AssertFailed();

                RTListNodeRemove(&pHstStream->Node);

                drvAudioStreamFree(pHstStream);
                pHstStream = NULL;
            }
            else
                LogRel2(("Audio: Uninitializing host stream '%s' failed with %Rrc\n", pHstStream->szName, rc));
        }

        if (   RT_SUCCESS(rc)
            && pGstStream)
        {
            rc = drvAudioStreamUninitInternal(pThis, pGstStream);
            if (RT_SUCCESS(rc))
            {
#ifdef VBOX_WITH_STATISTICS
                if (pGstStream->enmDir == PDMAUDIODIR_IN)
                {
                    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pGstStream->In.StatBytesElapsed);
                    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pGstStream->In.StatBytesTotalRead);
                    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pGstStream->In.StatFramesCaptured);
                }
                else if (pGstStream->enmDir == PDMAUDIODIR_OUT)
                {
                    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pGstStream->Out.StatBytesElapsed);
                    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pGstStream->Out.StatBytesTotalWritten);
                    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pGstStream->Out.StatFramesPlayed);
                }
                else
                    AssertFailed();
#endif
                RTListNodeRemove(&pGstStream->Node);

                RTMemFree(pGstStream);
                pGstStream = NULL;
            }
            else
                LogRel2(("Audio: Uninitializing guest stream '%s' failed with %Rrc\n", pGstStream->szName, rc));
        }
    }

    if (RT_SUCCESS(rc))
    {
        if (enmDir == PDMAUDIODIR_IN)
        {
            pThis->In.cStreamsFree++;
        }
        else /* Out */
        {
            pThis->Out.cStreamsFree++;
        }
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
 * @param   pHstStream          (Host) audio stream to use for creating the stream on the backend side.
 * @param   pCfgReq             Requested audio stream configuration to use for stream creation.
 * @param   pCfgAcq             Acquired audio stream configuration returned by the backend. Optional, can be NULL.
 */
static int drvAudioStreamCreateInternalBackend(PDRVAUDIO pThis,
                                               PPDMAUDIOSTREAM pHstStream, PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    AssertPtrReturn(pThis,      VERR_INVALID_POINTER);
    AssertPtrReturn(pHstStream, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgReq,    VERR_INVALID_POINTER);
    /* pCfgAcq is optional. */

    AssertMsg(pHstStream->enmCtx == PDMAUDIOSTREAMCTX_HOST,
              ("Stream '%s' is not a host stream and therefore has no backend\n", pHstStream->szName));

    AssertMsg((pHstStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_INITIALIZED) == 0,
              ("Stream '%s' already initialized in backend\n", pHstStream->szName));

    /* Make the acquired host configuration the requested host configuration initially,
     * in case the backend does not report back an acquired configuration. */
    PDMAUDIOSTREAMCFG CfgAcq;
    int rc = DrvAudioHlpStreamCfgCopy(&CfgAcq, pCfgReq);
    if (RT_FAILURE(rc))
    {
        LogRel(("Audio: Creating stream '%s' with an invalid backend configuration not possible, skipping\n",
                pHstStream->szName));
        return rc;
    }

    rc = pThis->pHostDrvAudio->pfnStreamCreate(pThis->pHostDrvAudio, pHstStream->pvBackend, pCfgReq, &CfgAcq);
    if (RT_FAILURE(rc))
    {
        if (rc == VERR_NOT_SUPPORTED)
            LogRel2(("Audio: Creating stream '%s' in backend not supported, skipping\n", pHstStream->szName));
        else
            LogRel2(("Audio: Creating stream '%s' in backend failed with %Rrc\n", pHstStream->szName, rc));

        return rc;
    }

    /* Validate acquired configuration. */
    if (!DrvAudioHlpStreamCfgIsValid(&CfgAcq))
    {
        LogRel(("Audio: Creating stream '%s' returned an invalid backend configuration, skipping\n", pHstStream->szName));
        return VERR_INVALID_PARAMETER;
    }

    /* Only set the host's stream to initialized if we were able create the stream
     * in the host backend. This is necessary for trying to re-initialize the stream
     * at some later point in time. */
    pHstStream->fStatus |= PDMAUDIOSTREAMSTS_FLAG_INITIALIZED;

    if (pCfgAcq)
    {
        int rc2 = DrvAudioHlpStreamCfgCopy(pCfgAcq, &CfgAcq);
        AssertRC(rc2);
    }

    return VINF_SUCCESS;
}

/**
 * Calls the backend to give it the chance to destroy its part of the audio stream.
 *
 * @returns IPRT status code.
 * @param   pThis               Pointer to driver instance.
 * @param   pHstStream          Host audio stream to call the backend destruction for.
 */
static int drvAudioStreamDestroyInternalBackend(PDRVAUDIO pThis, PPDMAUDIOSTREAM pHstStream)
{
    AssertPtrReturn(pThis,      VERR_INVALID_POINTER);
    AssertPtrReturn(pHstStream, VERR_INVALID_POINTER);

    AssertMsg(pHstStream->enmCtx == PDMAUDIOSTREAMCTX_HOST,
              ("Stream '%s' is not a host stream and therefore has no backend\n", pHstStream->szName));

    int rc = VINF_SUCCESS;

#ifdef LOG_ENABLED
    char *pszHstSts = dbgAudioStreamStatusToStr(pHstStream->fStatus);
    LogFunc(("[%s] fStatus=%s\n", pHstStream->szName, pszHstSts));
    RTStrFree(pszHstSts);
#endif

    if (pHstStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_INITIALIZED)
    {
        /* Check if the pointer to  the host audio driver is still valid.
         * It can be NULL if we were called in drvAudioDestruct, for example. */
        if (pThis->pHostDrvAudio)
            rc = pThis->pHostDrvAudio->pfnStreamDestroy(pThis->pHostDrvAudio, pHstStream->pvBackend);
        if (RT_SUCCESS(rc))
        {
            pHstStream->fStatus &= ~PDMAUDIOSTREAMSTS_FLAG_INITIALIZED;

#ifdef LOG_ENABLED
            /* This is not fatal, but log it anyway. */
            if (pHstStream->fStatus != PDMAUDIOSTREAMSTS_FLAG_NONE)
                LogFunc(("[%s] Warning: Stream still has %s set when destroying, must properly drain first\n",
                         pHstStream->szName, pszHstSts));
#endif
        }
    }

    LogFlowFunc(("[%s] Returning %Rrc\n", pHstStream->szName, rc));
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

    int rc = VINF_SUCCESS;

    if (pStream->enmCtx == PDMAUDIOSTREAMCTX_GUEST)
    {
        if (pStream->fStatus & PDMAUDIOSTREAMSTS_FLAG_INITIALIZED)
        {
            rc = drvAudioStreamControlInternal(pThis, pStream, PDMAUDIOSTREAMCMD_DISABLE);
            if (RT_SUCCESS(rc))
            {
                pStream->fStatus &= ~PDMAUDIOSTREAMSTS_FLAG_INITIALIZED;
                Assert(pStream->fStatus == PDMAUDIOSTREAMSTS_FLAG_NONE);
            }
        }
    }
    else if (pStream->enmCtx == PDMAUDIOSTREAMCTX_HOST)
    {
        rc = drvAudioStreamDestroyInternalBackend(pThis, pStream);
    }
    else
        AssertFailedReturn(VERR_NOT_IMPLEMENTED);

    if (RT_SUCCESS(rc))
    {
        /* Make sure that the pair (if any) knows that we're not valid anymore. */
        int rc2 = drvAudioStreamLinkToInternal(pStream, NULL);
        AssertRC(rc2);

        /* Reset status. */
        pStream->fStatus = PDMAUDIOSTREAMSTS_FLAG_NONE;

        /* Destroy mixing buffer. */
        AudioMixBufDestroy(&pStream->MixBuf);
    }

    LogFlowFunc(("Returning %Rrc\n", rc));
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

    /* Just destroy the host stream on the backend side.
     * The rest will either be destructed by the device emulation or
     * in drvAudioDestruct(). */
    PPDMAUDIOSTREAM pStream;
    RTListForEach(&pThis->lstHstStreams, pStream, PDMAUDIOSTREAM, Node)
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

    RTListInit(&pThis->lstHstStreams);
    RTListInit(&pThis->lstGstStreams);
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

    /*
     * Attach driver below and query its connector interface.
     */
    PPDMIBASE pDownBase;
    int rc = PDMDrvHlpAttach(pDrvIns, fFlags, &pDownBase);
    if (RT_FAILURE(rc))
    {
        LogRel(("Audio: Failed to attach to driver %p below (flags=0x%x), rc=%Rrc\n",
                pDrvIns, fFlags, rc));
        return rc;
    }

    pThis->pHostDrvAudio = PDMIBASE_QUERY_INTERFACE(pDownBase, PDMIHOSTAUDIO);
    if (!pThis->pHostDrvAudio)
    {
        LogRel(("Audio: Failed to query interface for underlying host driver\n"));
        return PDMDRV_SET_ERROR(pDrvIns, VERR_PDM_MISSING_INTERFACE_BELOW,
                                N_("Host audio backend missing or invalid"));
    }

    rc = drvAudioInit(pDrvIns, pCfg);
    if (RT_SUCCESS(rc))
    {
        pThis->fTerminate = false;
        pThis->pDrvIns    = pDrvIns;

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
        PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->Stats.TotalFramesOut,       "TotalFramesPlayed",
                                  STAMUNIT_COUNT, "Total frames played by backend.");
        PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->Stats.TotalFramesIn,        "TotalFramesCaptured",
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
    RTListForEachSafe(&pThis->lstHstStreams, pStream, pStreamNext, PDMAUDIOSTREAM, Node)
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
    Assert(RTListIsEmpty(&pThis->lstHstStreams));

    RTListForEachSafe(&pThis->lstGstStreams, pStream, pStreamNext, PDMAUDIOSTREAM, Node)
    {
        rc2 = drvAudioStreamUninitInternal(pThis, pStream);
        if (RT_SUCCESS(rc2))
        {
            RTListNodeRemove(&pStream->Node);

            RTMemFree(pStream);
            pStream = NULL;
        }
    }

    /* Sanity. */
    Assert(RTListIsEmpty(&pThis->lstGstStreams));

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

    int rc = VINF_SUCCESS;

    LogFunc(("%s\n", pThis->szName));

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

