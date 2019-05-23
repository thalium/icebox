/* $Id: DrvHostALSAAudio.cpp $ */
/** @file
 * ALSA audio driver.
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
 * --------------------------------------------------------------------
 *
 * This code is based on: alsaaudio.c
 *
 * QEMU ALSA audio driver
 *
 * Copyright (c) 2005 Vassili Karpov (malc)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DRV_HOST_AUDIO
#include <VBox/log.h>
#include <iprt/alloc.h>
#include <iprt/uuid.h> /* For PDMIBASE_2_PDMDRV. */
#include <VBox/vmm/pdmaudioifs.h>

RT_C_DECLS_BEGIN
 #include "alsa_stubs.h"
 #include "alsa_mangling.h"
RT_C_DECLS_END

#include <alsa/asoundlib.h>
#include <alsa/control.h> /* For device enumeration. */

#include "DrvAudio.h"
#include "VBoxDD.h"


/*********************************************************************************************************************************
*   Defines                                                                                                                      *
*********************************************************************************************************************************/

/** Makes DRVHOSTALSAAUDIO out of PDMIHOSTAUDIO. */
#define PDMIHOSTAUDIO_2_DRVHOSTALSAAUDIO(pInterface) \
    ( (PDRVHOSTALSAAUDIO)((uintptr_t)pInterface - RT_OFFSETOF(DRVHOSTALSAAUDIO, IHostAudio)) )


/*********************************************************************************************************************************
*   Structures                                                                                                                   *
*********************************************************************************************************************************/

typedef struct ALSAAUDIOSTREAM
{
    /** The stream's acquired configuration. */
    PPDMAUDIOSTREAMCFG pCfg;
    union
    {
        struct
        {

        } In;
        struct
        {
            /** Minimum samples required for ALSA to play data. */
            uint32_t    cSamplesMin;
        } Out;
    };
    snd_pcm_t          *phPCM;
    void               *pvBuf;
    size_t              cbBuf;
} ALSAAUDIOSTREAM, *PALSAAUDIOSTREAM;

/* latency = period_size * periods / (rate * bytes_per_frame) */

typedef struct ALSAAUDIOCFG
{
    int size_in_usec_in;
    int size_in_usec_out;
    const char *pcm_name_in;
    const char *pcm_name_out;
    unsigned int buffer_size_in;
    unsigned int period_size_in;
    unsigned int buffer_size_out;
    unsigned int period_size_out;
    unsigned int threshold;

    int buffer_size_in_overriden;
    int period_size_in_overriden;

    int buffer_size_out_overriden;
    int period_size_out_overriden;

} ALSAAUDIOCFG, *PALSAAUDIOCFG;

static int alsaStreamRecover(snd_pcm_t *phPCM);

static ALSAAUDIOCFG s_ALSAConf =
{
#ifdef HIGH_LATENCY
    1,
    1,
#else
    0,
    0,
#endif
    "default",
    "default",
#ifdef HIGH_LATENCY
    400000,
    400000 / 4,
    400000,
    400000 / 4,
#else
# define DEFAULT_BUFFER_SIZE 1024
# define DEFAULT_PERIOD_SIZE 256
    DEFAULT_BUFFER_SIZE * 4,
    DEFAULT_PERIOD_SIZE * 4,
    DEFAULT_BUFFER_SIZE,
    DEFAULT_PERIOD_SIZE,
#endif
    0,
    0,
    0,
    0,
    0
};

/**
 * Host Alsa audio driver instance data.
 * @implements PDMIAUDIOCONNECTOR
 */
typedef struct DRVHOSTALSAAUDIO
{
    /** Pointer to the driver instance structure. */
    PPDMDRVINS         pDrvIns;
    /** Pointer to host audio interface. */
    PDMIHOSTAUDIO      IHostAudio;
    /** Error count for not flooding the release log.
     *  UINT32_MAX for unlimited logging. */
    uint32_t           cLogErrors;
} DRVHOSTALSAAUDIO, *PDRVHOSTALSAAUDIO;

/** Maximum number of tries to recover a broken pipe. */
#define ALSA_RECOVERY_TRIES_MAX    5

typedef struct ALSAAUDIOSTREAMCFG
{
    unsigned int        freq;
    /** PCM sound format. */
    snd_pcm_format_t    fmt;
    /** PCM data access type. */
    snd_pcm_access_t    access;
    /** Whether resampling should be performed by alsalib or not. */
    int                 resample;
    int                 nchannels;
    unsigned long       buffer_size;
    unsigned long       period_size;
    snd_pcm_uframes_t   samples;
} ALSAAUDIOSTREAMCFG, *PALSAAUDIOSTREAMCFG;



static snd_pcm_format_t alsaAudioPropsToALSA(PPDMAUDIOPCMPROPS pProps)
{
    switch (pProps->cBits)
    {
        case 8:
            return pProps->fSigned ? SND_PCM_FORMAT_S8 : SND_PCM_FORMAT_U8;

        case 16:
            return pProps->fSigned ? SND_PCM_FORMAT_S16_LE : SND_PCM_FORMAT_U16_LE;

        case 32:
            return pProps->fSigned ? SND_PCM_FORMAT_S32_LE : SND_PCM_FORMAT_U32_LE;

        default:
            break;
    }

    AssertMsgFailed(("%RU8 bits not supported\n", pProps->cBits));
    return SND_PCM_FORMAT_U8;
}


static int alsaALSAToAudioProps(snd_pcm_format_t fmt, PPDMAUDIOPCMPROPS pProps)
{
    switch (fmt)
    {
        case SND_PCM_FORMAT_S8:
            pProps->cBits   = 8;
            pProps->fSigned = true;
            break;

        case SND_PCM_FORMAT_U8:
            pProps->cBits   = 8;
            pProps->fSigned = false;
            break;

        case SND_PCM_FORMAT_S16_LE:
            pProps->cBits   = 16;
            pProps->fSigned = true;
            break;

        case SND_PCM_FORMAT_U16_LE:
            pProps->cBits   = 16;
            pProps->fSigned = false;
            break;

        case SND_PCM_FORMAT_S16_BE:
            pProps->cBits       = 16;
            pProps->fSigned     = true;
#ifdef RT_LITTLE_ENDIAN
            pProps->fSwapEndian = true;
#endif
            break;

        case SND_PCM_FORMAT_U16_BE:
            pProps->cBits       = 16;
            pProps->fSigned     = false;
#ifdef RT_LITTLE_ENDIAN
            pProps->fSwapEndian = true;
#endif
            break;

        case SND_PCM_FORMAT_S32_LE:
            pProps->cBits   = 32;
            pProps->fSigned = true;
            break;

        case SND_PCM_FORMAT_U32_LE:
            pProps->cBits   = 32;
            pProps->fSigned = false;
            break;

        case SND_PCM_FORMAT_S32_BE:
            pProps->cBits       = 32;
            pProps->fSigned     = true;
#ifdef RT_LITTLE_ENDIAN
            pProps->fSwapEndian = true;
#endif
            break;

        case SND_PCM_FORMAT_U32_BE:
            pProps->cBits       = 32;
            pProps->fSigned     = false;
#ifdef RT_LITTLE_ENDIAN
            pProps->fSwapEndian = true;
#endif
            break;

        default:
            AssertMsgFailed(("Format %ld not supported\n", fmt));
            return VERR_NOT_SUPPORTED;
    }

    Assert(pProps->cBits);
    Assert(pProps->cChannels);
    pProps->cShift = PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(pProps->cBits, pProps->cChannels);

    return VINF_SUCCESS;
}


static int alsaGetSampleShift(snd_pcm_format_t fmt, unsigned *puShift)
{
    AssertPtrReturn(puShift, VERR_INVALID_POINTER);

    switch (fmt)
    {
        case SND_PCM_FORMAT_S8:
        case SND_PCM_FORMAT_U8:
            *puShift = 0;
            break;

        case SND_PCM_FORMAT_S16_LE:
        case SND_PCM_FORMAT_U16_LE:
        case SND_PCM_FORMAT_S16_BE:
        case SND_PCM_FORMAT_U16_BE:
            *puShift = 1;
            break;

        case SND_PCM_FORMAT_S32_LE:
        case SND_PCM_FORMAT_U32_LE:
        case SND_PCM_FORMAT_S32_BE:
        case SND_PCM_FORMAT_U32_BE:
            *puShift = 2;
            break;

        default:
            AssertMsgFailed(("Format %ld not supported\n", fmt));
            return VERR_NOT_SUPPORTED;
    }

    return VINF_SUCCESS;
}


static int alsaStreamSetThreshold(snd_pcm_t *phPCM, snd_pcm_uframes_t threshold)
{
    snd_pcm_sw_params_t *pSWParms = NULL;
    snd_pcm_sw_params_alloca(&pSWParms);
    if (!pSWParms)
        return VERR_NO_MEMORY;

    int rc;
    do
    {
        int err = snd_pcm_sw_params_current(phPCM, pSWParms);
        if (err < 0)
        {
            LogRel(("ALSA: Failed to get current software parameters for threshold: %s\n",
                    snd_strerror(err)));
            rc = VERR_ACCESS_DENIED;
            break;
        }

        err = snd_pcm_sw_params_set_start_threshold(phPCM, pSWParms, threshold);
        if (err < 0)
        {
            LogRel(("ALSA: Failed to set software threshold to %ld: %s\n",
                    threshold, snd_strerror(err)));
            rc = VERR_ACCESS_DENIED;
            break;
        }

        err = snd_pcm_sw_params_set_avail_min(phPCM, pSWParms, 512);
        if (err < 0)
        {
            LogRel(("ALSA: Failed to set available minimum to %ld: %s\n",
                    threshold, snd_strerror(err)));
            rc = VERR_ACCESS_DENIED;
            break;
        }

        err = snd_pcm_sw_params(phPCM, pSWParms);
        if (err < 0)
        {
            LogRel(("ALSA: Failed to set new software parameters for threshold: %s\n",
                    snd_strerror(err)));
            rc = VERR_ACCESS_DENIED;
            break;
        }

        LogFlowFunc(("Setting threshold to %RU32\n", threshold));
        rc = VINF_SUCCESS;
    }
    while (0);

    return rc;
}


static int alsaStreamClose(snd_pcm_t **pphPCM)
{
    if (!pphPCM || !*pphPCM)
        return VINF_SUCCESS;

    int rc;
    int rc2 = snd_pcm_close(*pphPCM);
    if (rc2)
    {
        LogRel(("ALSA: Closing PCM descriptor failed: %s\n", snd_strerror(rc2)));
        rc = VERR_GENERAL_FAILURE; /** @todo */
    }
    else
    {
        *pphPCM = NULL;
        rc = VINF_SUCCESS;
    }

    return rc;
}


#if 0 /* After Beta. */
static int alsaSetHWParams(snd_pcm_t *phPCM, PALSAAUDIOSTREAMCFG pCfg)
{
    int rc;
    snd_pcm_hw_params_t *pParams = NULL;

    do
    {
        snd_pcm_hw_params_alloca(&pParams);
        if (!pParams)
        {
            rc = VERR_NO_MEMORY;
            break;
        }

        unsigned int rrate;
        snd_pcm_uframes_t size;
        int dir;

        /* choose all parameters */
        int err = snd_pcm_hw_params_any(phPCM, pParams);
        if (err < 0)
        {
            LogRel(("ALSA: Broken configuration for playback: no configurations available: %s\n", snd_strerror(err)));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }
        /* set hardware resampling */
        err = snd_pcm_hw_params_set_rate_resample(phPCM, pParams, pCfg->resample);
        if (err < 0)
        {
            LogRel(("ALSA: Resampling setup failed for playback: %s\n", snd_strerror(err)));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }
        /* set the interleaved read/write format */
        err = snd_pcm_hw_params_set_access(phPCM, pParams, pCfg->access);
        if (err < 0)
        {
            LogRel(("ALSA: Access type not available for playback: %s\n", snd_strerror(err)));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }
        /* set the sample format */
        err = snd_pcm_hw_params_set_format(phPCM, pParams, pCfg->fmt);
        if (err < 0)
        {
            LogRel(("ALSA: Sample format not available for playback: %s\n", snd_strerror(err)));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }
        /* set the count of channels */
        err = snd_pcm_hw_params_set_channels(phPCM, pParams, pCfg->nchannels);
        if (err < 0)
        {
            LogRel(("ALSA: Channels count (%d) not available for playbacks: %s\n", pCfg->nchannels, snd_strerror(err)));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }
        /* set the stream rate */
        rrate = pCfg->freq;
        err = snd_pcm_hw_params_set_rate_near(phPCM, pParams, &rrate, 0);
        if (err < 0)
        {
            LogRel(("ALSA: Rate %uHz not available for playback: %s\n", pCfg->freq, snd_strerror(err)));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }
        if (rrate != pCfg->freq)
        {
            LogRel(("ALSA: Rate doesn't match (requested %iHz, get %uHz)\n", pCfg->freq, err));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }
        /* set the buffer time */
        err = snd_pcm_hw_params_set_buffer_time_near(phPCM, pParams, &pCfg->buffer_time, &dir);
        if (err < 0)
        {
            LogRel(("ALSA: Unable to set buffer time %i for playback: %s\n", buffer_time, snd_strerror(err)));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }
        err = snd_pcm_hw_params_get_buffer_size(pParams, &size);
        if (err < 0)
        {
            LogRel(("ALSA: Unable to get buffer size for playback: %s\n", snd_strerror(err)));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }
        buffer_size = size;
        /* set the period time */
        err = snd_pcm_hw_params_set_period_time_near(phPCM, pParams, &period_time, &dir);
        if (err < 0)
        {
            LogRel(("ALSA: Unable to set period time %i for playback: %s\n", period_time, snd_strerror(err)));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }
        err = snd_pcm_hw_params_get_period_size(pParams, &size, &dir);
        if (err < 0)
        {
            LogRel(("ALSA: Unable to get period size for playback: %s\n", snd_strerror(err)));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }
        period_size = size;
        /* write the parameters to device */
        err = snd_pcm_hw_params(phPCM, pParams);
        if (err < 0)
        {
            LogRel(("ALSA: Unable to set hw params for playback: %s\n", snd_strerror(err)));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }

        rc = VINF_SUCCESS;

    } while (0);

    if (pParams)
    {
        snd_pcm_hw_params_free(pParams);
        pParams = NULL;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}


static int alsaSetSWParams(snd_pcm_t *phPCM, PALSAAUDIOCFG pCfg)
{
    int rc;
    snd_pcm_sw_params_t *pParams = NULL;

    do
    {
        snd_pcm_sw_params_alloca(&pParams);
        if (!pParams)
        {
            rc = VERR_NO_MEMORY;
            break;
        }
        /* get the current swparams */
        int err = snd_pcm_sw_params_current(phPCM, pParams);
        if (err < 0)
        {
            LogRel(("ALSA: Unable to determine current swparams for playback: %s\n", snd_strerror(err)));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }
        /* start the transfer when the buffer is almost full: */
        /* (buffer_size / avail_min) * avail_min */
        err = snd_pcm_sw_params_set_start_threshold(phPCM, pParams, (buffer_size / period_size) * period_size);
        if (err < 0)
        {
            LogRel(("ALSA: Unable to set start threshold mode for playback: %s\n", snd_strerror(err)));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }
        /* allow the transfer when at least period_size samples can be processed */
        /* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
        err = snd_pcm_sw_params_set_avail_min(phPCM, pParams, period_size);
        if (err < 0)
        {
            LogRel(("ALSA: Unable to set avail min for playback: %s\n", snd_strerror(err)));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }
        /* write the parameters to the playback device */
        err = snd_pcm_sw_params(phPCM, pParams);
        if (err < 0)
        {
            LogRel(("ALSA: Unable to set sw params for playback: %s\n", snd_strerror(err)));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }

        rc = VINF_SUCCESS;

    } while (0);

    if (pParams)
    {
        snd_pcm_sw_params_free(pParams);
        pParams = NULL;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}
#endif


static int alsaStreamOpen(bool fIn, PALSAAUDIOSTREAMCFG pCfgReq, PALSAAUDIOSTREAMCFG pCfgObt, snd_pcm_t **pphPCM)
{
    snd_pcm_t *phPCM = NULL;
    int rc;

    unsigned int cChannels = pCfgReq->nchannels;
    unsigned int uFreq = pCfgReq->freq;
    snd_pcm_uframes_t obt_buffer_size;

    do
    {
        const char *pszDev = fIn ? s_ALSAConf.pcm_name_in : s_ALSAConf.pcm_name_out;
        if (!pszDev)
        {
            LogRel(("ALSA: Invalid or no %s device name set\n", fIn ? "input" : "output"));
            rc = VERR_INVALID_PARAMETER;
            break;
        }

        int err = snd_pcm_open(&phPCM, pszDev,
                               fIn ? SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK,
                               SND_PCM_NONBLOCK);
        if (err < 0)
        {
            LogRel(("ALSA: Failed to open \"%s\" as %s device: %s\n", pszDev, fIn ? "input" : "output", snd_strerror(err)));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }

        LogRel(("ALSA: Using %s device \"%s\"\n", fIn ? "input" : "output", pszDev));

        snd_pcm_hw_params_t *pHWParms;
        snd_pcm_hw_params_alloca(&pHWParms); /** @todo Check for successful allocation? */
        err = snd_pcm_hw_params_any(phPCM, pHWParms);
        if (err < 0)
        {
            LogRel(("ALSA: Failed to initialize hardware parameters: %s\n", snd_strerror(err)));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }

        err = snd_pcm_hw_params_set_access(phPCM, pHWParms,
                                           SND_PCM_ACCESS_RW_INTERLEAVED);
        if (err < 0)
        {
            LogRel(("ALSA: Failed to set access type: %s\n", snd_strerror(err)));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }

        err = snd_pcm_hw_params_set_format(phPCM, pHWParms, pCfgReq->fmt);
        if (err < 0)
        {
            LogRel(("ALSA: Failed to set audio format to %d: %s\n", pCfgReq->fmt, snd_strerror(err)));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }

        err = snd_pcm_hw_params_set_rate_near(phPCM, pHWParms, &uFreq, 0);
        if (err < 0)
        {
            LogRel(("ALSA: Failed to set frequency to %uHz: %s\n", pCfgReq->freq, snd_strerror(err)));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }

        err = snd_pcm_hw_params_set_channels_near(phPCM, pHWParms, &cChannels);
        if (err < 0)
        {
            LogRel(("ALSA: Failed to set number of channels to %d\n", pCfgReq->nchannels));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }

        if (   cChannels != 1
            && cChannels != 2)
        {
            LogRel(("ALSA: Number of audio channels (%u) not supported\n", cChannels));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }

        unsigned int period_size = pCfgReq->period_size;
        unsigned int buffer_size = pCfgReq->buffer_size;

        if (   !((fIn && s_ALSAConf.size_in_usec_in)
            ||  (!fIn && s_ALSAConf.size_in_usec_out)))
        {
            if (!buffer_size)
            {
                buffer_size = DEFAULT_BUFFER_SIZE;
                period_size = DEFAULT_PERIOD_SIZE;
            }
        }

        if (buffer_size)
        {
            if (   ( fIn && s_ALSAConf.size_in_usec_in)
                || (!fIn && s_ALSAConf.size_in_usec_out))
            {
                if (period_size)
                {
                    err = snd_pcm_hw_params_set_period_time_near(phPCM, pHWParms,
                                                                 &period_size, 0);
                    if (err < 0)
                    {
                        LogRel(("ALSA: Failed to set period time %d\n", pCfgReq->period_size));
                        rc = VERR_AUDIO_BACKEND_INIT_FAILED;
                        break;
                    }
                }

                err = snd_pcm_hw_params_set_buffer_time_near(phPCM, pHWParms,
                                                             &buffer_size, 0);
                if (err < 0)
                {
                    LogRel(("ALSA: Failed to set buffer time %d\n", pCfgReq->buffer_size));
                    rc = VERR_AUDIO_BACKEND_INIT_FAILED;
                    break;
                }
            }
            else
            {
                snd_pcm_uframes_t period_size_f = (snd_pcm_uframes_t)period_size;
                snd_pcm_uframes_t buffer_size_f = (snd_pcm_uframes_t)buffer_size;

                snd_pcm_uframes_t minval;

                if (period_size_f)
                {
                    minval = period_size_f;

                    int dir = 0;
                    err = snd_pcm_hw_params_get_period_size_min(pHWParms,
                                                                &minval, &dir);
                    if (err < 0)
                    {
                        LogRel(("ALSA: Could not determine minimal period size\n"));
                        rc = VERR_AUDIO_BACKEND_INIT_FAILED;
                        break;
                    }
                    else
                    {
                        LogFunc(("Minimal period size is: %ld\n", minval));
                        if (period_size_f < minval)
                        {
                            if (   ( fIn && s_ALSAConf.period_size_in_overriden)
                                || (!fIn && s_ALSAConf.period_size_out_overriden))
                            {
                                LogFunc(("Period size %RU32 is less than minimal period size %RU32\n",
                                         period_size_f, minval));
                            }

                            period_size_f = minval;
                        }
                    }

                    err = snd_pcm_hw_params_set_period_size_near(phPCM, pHWParms,
                                                                 &period_size_f, 0);
                    LogFunc(("Period size is: %RU32\n", period_size_f));
                    if (err < 0)
                    {
                        LogRel(("ALSA: Failed to set period size %d (%s)\n",
                                period_size_f, snd_strerror(err)));
                        rc = VERR_AUDIO_BACKEND_INIT_FAILED;
                        break;
                    }
                }

                /* Calculate default buffer size here since it might have been changed
                 * in the _near functions */
                buffer_size_f = 4 * period_size_f;

                minval = buffer_size_f;
                err = snd_pcm_hw_params_get_buffer_size_min(pHWParms, &minval);
                if (err < 0)
                {
                    LogRel(("ALSA: Could not retrieve minimal buffer size\n"));
                    rc = VERR_AUDIO_BACKEND_INIT_FAILED;
                    break;
                }
                else
                {
                    LogFunc(("Minimal buffer size is: %RU32\n", minval));
                    if (buffer_size_f < minval)
                    {
                        if (   ( fIn && s_ALSAConf.buffer_size_in_overriden)
                            || (!fIn && s_ALSAConf.buffer_size_out_overriden))
                        {
                            LogFunc(("Buffer size %RU32 is less than minimal buffer size %RU32\n",
                                     buffer_size_f, minval));
                        }

                        buffer_size_f = minval;
                    }
                }

                err = snd_pcm_hw_params_set_buffer_size_near(phPCM,
                                                             pHWParms, &buffer_size_f);
                LogFunc(("Buffer size is: %RU32\n", buffer_size_f));
                if (err < 0)
                {
                    LogRel(("ALSA: Failed to set buffer size %d: %s\n",
                            buffer_size_f, snd_strerror(err)));
                    rc = VERR_AUDIO_BACKEND_INIT_FAILED;
                    break;
                }
            }
        }
        else
            LogFunc(("Warning: Buffer size is not set\n"));

        err = snd_pcm_hw_params(phPCM, pHWParms);
        if (err < 0)
        {
            LogRel(("ALSA: Failed to apply audio parameters\n"));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }

        err = snd_pcm_hw_params_get_buffer_size(pHWParms, &obt_buffer_size);
        if (err < 0)
        {
            LogRel(("ALSA: Failed to get buffer size\n"));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }

        LogFunc(("Buffer sample size is: %RU32\n", obt_buffer_size));

        snd_pcm_uframes_t obt_period_size;
        int dir = 0;
        err = snd_pcm_hw_params_get_period_size(pHWParms, &obt_period_size, &dir);
        if (err < 0)
        {
            LogRel(("ALSA: Failed to get period size\n"));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }

        LogFunc(("Freq=%dHz, period size=%RU32, buffer size=%RU32\n",
                 pCfgReq->freq, obt_period_size, obt_buffer_size));

        err = snd_pcm_prepare(phPCM);
        if (err < 0)
        {
            LogRel(("ALSA: Could not prepare hPCM %p\n", (void *)phPCM));
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
            break;
        }

        if (   !fIn
            && s_ALSAConf.threshold)
        {
            unsigned uShift;
            rc = alsaGetSampleShift(pCfgReq->fmt, &uShift);
            if (RT_SUCCESS(rc))
            {
                int bytes_per_sec = uFreq
                    << (cChannels == 2)
                    << uShift;

                snd_pcm_uframes_t threshold
                    = (s_ALSAConf.threshold * bytes_per_sec) / 1000;

                rc = alsaStreamSetThreshold(phPCM, threshold);
            }
        }
        else
            rc = VINF_SUCCESS;
    }
    while (0);

    if (RT_SUCCESS(rc))
    {
        pCfgObt->fmt       = pCfgReq->fmt;
        pCfgObt->nchannels = cChannels;
        pCfgObt->freq      = uFreq;
        pCfgObt->samples   = obt_buffer_size;

        *pphPCM = phPCM;
    }
    else
        alsaStreamClose(&phPCM);

    LogFlowFuncLeaveRC(rc);
    return rc;
}


#ifdef DEBUG
static void alsaDbgErrorHandler(const char *file, int line, const char *function,
                                int err, const char *fmt, ...)
{
    /** @todo Implement me! */
    RT_NOREF(file, line, function, err, fmt);
}
#endif


static int alsaStreamGetAvail(snd_pcm_t *phPCM, snd_pcm_sframes_t *pFramesAvail)
{
    AssertPtrReturn(phPCM, VERR_INVALID_POINTER);
    /* pFramesAvail is optional. */

    int rc;

    snd_pcm_sframes_t framesAvail = snd_pcm_avail_update(phPCM);
    if (framesAvail < 0)
    {
        if (framesAvail == -EPIPE)
        {
            rc = alsaStreamRecover(phPCM);
            if (RT_SUCCESS(rc))
                framesAvail = snd_pcm_avail_update(phPCM);
        }
        else
            rc = VERR_ACCESS_DENIED; /** @todo Find a better rc. */
    }
    else
        rc = VINF_SUCCESS;

    if (RT_SUCCESS(rc))
    {
        if (pFramesAvail)
            *pFramesAvail = framesAvail;
    }

    LogFunc(("cFrames=%ld, rc=%Rrc\n", framesAvail, rc));
    return rc;
}


static int alsaStreamRecover(snd_pcm_t *phPCM)
{
    AssertPtrReturn(phPCM, VERR_INVALID_POINTER);

    int err = snd_pcm_prepare(phPCM);
    if (err < 0)
    {
        LogFunc(("Failed to recover stream %p: %s\n", phPCM, snd_strerror(err)));
        return VERR_ACCESS_DENIED; /** @todo Find a better rc. */
    }

    return VINF_SUCCESS;
}


static int alsaStreamResume(snd_pcm_t *phPCM)
{
    AssertPtrReturn(phPCM, VERR_INVALID_POINTER);

    int err = snd_pcm_resume(phPCM);
    if (err < 0)
    {
        LogFunc(("Failed to resume stream %p: %s\n", phPCM, snd_strerror(err)));
        return VERR_ACCESS_DENIED; /** @todo Find a better rc. */
    }

    return VINF_SUCCESS;
}


static int drvHostALSAAudioStreamCtl(PALSAAUDIOSTREAM pStreamALSA, bool fPause)
{
    int rc = VINF_SUCCESS;

    const bool fInput = pStreamALSA->pCfg->enmDir == PDMAUDIODIR_IN;

    int err;
    if (fPause)
    {
        err = snd_pcm_drop(pStreamALSA->phPCM);
        if (err < 0)
        {
            LogRel(("ALSA: Error stopping %s stream: %s\n", fInput ? "input" : "output", snd_strerror(err)));
            rc = VERR_ACCESS_DENIED; /** @todo Find a better rc. */
        }
    }
    else
    {
        err = snd_pcm_prepare(pStreamALSA->phPCM);
        if (err < 0)
        {
            LogRel(("ALSA: Error preparing %s stream: %s\n", fInput ? "input" : "output", snd_strerror(err)));
            rc = VERR_ACCESS_DENIED; /** @todo Find a better rc. */
        }
        else
        {
            Assert(snd_pcm_state(pStreamALSA->phPCM) == SND_PCM_STATE_PREPARED);

            if (fInput) /* Only start the PCM stream for input streams. */
            {
                err = snd_pcm_start(pStreamALSA->phPCM);
                if (err < 0)
                {
                    LogRel(("ALSA: Error starting input stream: %s\n", snd_strerror(err)));
                    rc = VERR_ACCESS_DENIED; /** @todo Find a better rc. */
                }
            }
        }
    }

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnInit}
 */
static DECLCALLBACK(int) drvHostALSAAudioInit(PPDMIHOSTAUDIO pInterface)
{
    RT_NOREF(pInterface);

    LogFlowFuncEnter();

    int rc = audioLoadAlsaLib();
    if (RT_FAILURE(rc))
        LogRel(("ALSA: Failed to load the ALSA shared library, rc=%Rrc\n", rc));
    else
    {
#ifdef DEBUG
        snd_lib_error_set_handler(alsaDbgErrorHandler);
#endif
    }

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamCapture}
 */
static DECLCALLBACK(int) drvHostALSAAudioStreamCapture(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream,
                                                       void *pvBuf, uint32_t cxBuf, uint32_t *pcxRead)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    AssertPtrReturn(pvBuf,      VERR_INVALID_POINTER);
    AssertReturn(cxBuf,         VERR_INVALID_PARAMETER);
    /* pcbRead is optional. */

    PALSAAUDIOSTREAM pStreamALSA = (PALSAAUDIOSTREAM)pStream;

    snd_pcm_sframes_t cAvail;
    int rc = alsaStreamGetAvail(pStreamALSA->phPCM, &cAvail);
    if (RT_FAILURE(rc))
    {
        LogFunc(("Error getting number of captured frames, rc=%Rrc\n", rc));
        return rc;
    }

    PPDMAUDIOSTREAMCFG pCfg = pStreamALSA->pCfg;
    AssertPtr(pCfg);

    if (!cAvail) /* No data yet? */
    {
        snd_pcm_state_t state = snd_pcm_state(pStreamALSA->phPCM);
        switch (state)
        {
            case SND_PCM_STATE_PREPARED:
                cAvail = PDMAUDIOSTREAMCFG_B2F(pCfg, cxBuf);
                break;

            case SND_PCM_STATE_SUSPENDED:
            {
                rc = alsaStreamResume(pStreamALSA->phPCM);
                if (RT_FAILURE(rc))
                    break;

                LogFlow(("Resuming suspended input stream\n"));
                break;
            }

            default:
                LogFlow(("No frames available, state=%d\n", state));
                break;
        }

        if (!cAvail)
        {
            if (pcxRead)
                *pcxRead = 0;
            return VINF_SUCCESS;
        }
    }

    /*
     * Check how much we can read from the capture device without overflowing
     * the mixer buffer.
     */
    size_t cbToRead = RT_MIN((size_t)PDMAUDIOSTREAMCFG_F2B(pCfg, cAvail), cxBuf);

    LogFlowFunc(("cbToRead=%zu, cAvail=%RI32\n", cbToRead, cAvail));

    uint32_t cbReadTotal = 0;

    snd_pcm_uframes_t cToRead;
    snd_pcm_sframes_t cRead;

    while (   cbToRead
           && RT_SUCCESS(rc))
    {
        cToRead = RT_MIN(PDMAUDIOSTREAMCFG_B2F(pCfg, cbToRead),
                         PDMAUDIOSTREAMCFG_B2F(pCfg, pStreamALSA->cbBuf));
        AssertBreakStmt(cToRead, rc = VERR_NO_DATA);
        cRead = snd_pcm_readi(pStreamALSA->phPCM, pStreamALSA->pvBuf, cToRead);
        if (cRead <= 0)
        {
            switch (cRead)
            {
                case 0:
                {
                    LogFunc(("No input frames available\n"));
                    rc = VERR_ACCESS_DENIED;
                    break;
                }

                case -EAGAIN:
                {
                    /*
                     * Don't set error here because EAGAIN means there are no further frames
                     * available at the moment, try later. As we might have read some frames
                     * already these need to be processed instead.
                     */
                    cbToRead = 0;
                    break;
                }

                case -EPIPE:
                {
                    rc = alsaStreamRecover(pStreamALSA->phPCM);
                    if (RT_FAILURE(rc))
                        break;

                    LogFlowFunc(("Recovered from capturing\n"));
                    continue;
                }

                default:
                {
                    LogFunc(("Failed to read input frames: %s\n", snd_strerror(cRead)));
                    rc = VERR_GENERAL_FAILURE; /** @todo Fudge! */
                    break;
                }
            }
        }
        else
        {
            /*
             * We should not run into a full mixer buffer or we loose samples and
             * run into an endless loop if ALSA keeps producing samples ("null"
             * capture device for example).
             */
            uint32_t cbRead = PDMAUDIOSTREAMCFG_F2B(pCfg, cRead);

            memcpy(pvBuf, pStreamALSA->pvBuf, cbRead);

            Assert(cbToRead >= cbRead);
            cbToRead    -= cbRead;
            cbReadTotal += cbRead;
        }
    }

    if (RT_SUCCESS(rc))
    {
        if (pcxRead)
            *pcxRead = cbReadTotal;
    }

    return rc;
}

/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamPlay}
 */
static DECLCALLBACK(int) drvHostALSAAudioStreamPlay(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream,
                                                    const void *pvBuf, uint32_t cxBuf, uint32_t *pcxWritten)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    AssertPtrReturn(pvBuf,      VERR_INVALID_POINTER);
    AssertReturn(cxBuf,         VERR_INVALID_PARAMETER);
    /* pcxWritten is optional. */

    PALSAAUDIOSTREAM pStreamALSA = (PALSAAUDIOSTREAM)pStream;

    PPDMAUDIOSTREAMCFG pCfg = pStreamALSA->pCfg;
    AssertPtr(pCfg);

    int rc;

    uint32_t cbWrittenTotal = 0;

    do
    {
        snd_pcm_sframes_t csAvail;
        rc = alsaStreamGetAvail(pStreamALSA->phPCM, &csAvail);
        if (RT_FAILURE(rc))
        {
            LogFunc(("Error getting number of playback frames, rc=%Rrc\n", rc));
            break;
        }

        if (!csAvail)
            break;

        size_t cbToWrite = RT_MIN((unsigned)PDMAUDIOSTREAMCFG_F2B(pCfg, csAvail), pStreamALSA->cbBuf);
        if (!cbToWrite)
            break;

        /* Do not write more than available. */
        if (cbToWrite > cxBuf)
            cbToWrite = cxBuf;

        memcpy(pStreamALSA->pvBuf, pvBuf, cbToWrite);

        snd_pcm_sframes_t csWritten = 0;

        /* Don't try infinitely on recoverable errors. */
        unsigned iTry;
        for (iTry = 0; iTry < ALSA_RECOVERY_TRIES_MAX; iTry++)
        {
            csWritten = snd_pcm_writei(pStreamALSA->phPCM, pStreamALSA->pvBuf,
                                       PDMAUDIOSTREAMCFG_B2F(pCfg, cbToWrite));
            if (csWritten <= 0)
            {
                switch (csWritten)
                {
                    case 0:
                    {
                        LogFunc(("Failed to write %zu bytes\n", cbToWrite));
                        rc = VERR_ACCESS_DENIED;
                        break;
                    }

                    case -EPIPE:
                    {
                        rc = alsaStreamRecover(pStreamALSA->phPCM);
                        if (RT_FAILURE(rc))
                            break;

                        LogFlowFunc(("Recovered from playback\n"));
                        continue;
                    }

                    case -ESTRPIPE:
                    {
                        /* Stream was suspended and waiting for a recovery. */
                        rc = alsaStreamResume(pStreamALSA->phPCM);
                        if (RT_FAILURE(rc))
                        {
                            LogRel(("ALSA: Failed to resume output stream\n"));
                            break;
                        }

                        LogFlowFunc(("Resumed suspended output stream\n"));
                        continue;
                    }

                    default:
                        LogFlowFunc(("Failed to write %RU32 bytes, error unknown\n", cbToWrite));
                        rc = VERR_GENERAL_FAILURE; /** @todo */
                        break;
                }
            }
            else
                break;
        } /* For number of tries. */

        if (   iTry == ALSA_RECOVERY_TRIES_MAX
            && csWritten <= 0)
            rc = VERR_BROKEN_PIPE;

        if (RT_FAILURE(rc))
            break;

        cbWrittenTotal = PDMAUDIOSTREAMCFG_F2B(pCfg, csWritten);

    } while (0);

    if (RT_SUCCESS(rc))
    {
        if (pcxWritten)
            *pcxWritten = cbWrittenTotal;
    }

    return rc;
}


static int alsaDestroyStreamIn(PALSAAUDIOSTREAM pStreamALSA)
{
    alsaStreamClose(&pStreamALSA->phPCM);

    if (pStreamALSA->pvBuf)
    {
        RTMemFree(pStreamALSA->pvBuf);
        pStreamALSA->pvBuf = NULL;
    }

    return VINF_SUCCESS;
}


static int alsaDestroyStreamOut(PALSAAUDIOSTREAM pStreamALSA)
{
    alsaStreamClose(&pStreamALSA->phPCM);

    if (pStreamALSA->pvBuf)
    {
        RTMemFree(pStreamALSA->pvBuf);
        pStreamALSA->pvBuf = NULL;
    }

    return VINF_SUCCESS;
}


static int alsaCreateStreamOut(PALSAAUDIOSTREAM pStreamALSA, PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    snd_pcm_t *phPCM = NULL;

    int rc;

    do
    {
        ALSAAUDIOSTREAMCFG req;
        req.fmt         = alsaAudioPropsToALSA(&pCfgReq->Props);
        req.freq        = pCfgReq->Props.uHz;
        req.nchannels   = pCfgReq->Props.cChannels;
        req.period_size = s_ALSAConf.period_size_out; /** @todo Make this configurable. */
        req.buffer_size = s_ALSAConf.buffer_size_out; /** @todo Make this configurable. */

        ALSAAUDIOSTREAMCFG obt;
        rc = alsaStreamOpen(false /* fIn */, &req, &obt, &phPCM);
        if (RT_FAILURE(rc))
            break;

        pCfgAcq->Props.uHz       = obt.freq;
        pCfgAcq->Props.cChannels = obt.nchannels;

        rc = alsaALSAToAudioProps(obt.fmt, &pCfgAcq->Props);
        if (RT_FAILURE(rc))
            break;

        pCfgAcq->cFrameBufferHint = obt.samples * 4;

        AssertBreakStmt(obt.samples, rc = VERR_INVALID_PARAMETER);

        size_t cbBuf = obt.samples * PDMAUDIOSTREAMCFG_F2B(pCfgAcq, 1);
        AssertBreakStmt(cbBuf, rc = VERR_INVALID_PARAMETER);

        pStreamALSA->pvBuf = RTMemAllocZ(cbBuf);
        if (!pStreamALSA->pvBuf)
        {
            LogRel(("ALSA: Not enough memory for output DAC buffer (%RU32 samples, %zu bytes)\n", obt.samples, cbBuf));
            rc = VERR_NO_MEMORY;
            break;
        }

        pStreamALSA->cbBuf = cbBuf;
        pStreamALSA->phPCM = phPCM;
    }
    while (0);

    if (RT_FAILURE(rc))
        alsaStreamClose(&phPCM);

    LogFlowFuncLeaveRC(rc);
    return rc;
}


static int alsaCreateStreamIn(PALSAAUDIOSTREAM pStreamALSA, PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    int rc;

    snd_pcm_t *phPCM = NULL;

    do
    {
        ALSAAUDIOSTREAMCFG req;
        req.fmt         = alsaAudioPropsToALSA(&pCfgReq->Props);
        req.freq        = pCfgReq->Props.uHz;
        req.nchannels   = pCfgReq->Props.cChannels;
        req.period_size = s_ALSAConf.period_size_in; /** @todo Make this configurable. */
        req.buffer_size = s_ALSAConf.buffer_size_in; /** @todo Make this configurable. */

        ALSAAUDIOSTREAMCFG obt;
        rc = alsaStreamOpen(true /* fIn */, &req, &obt, &phPCM);
        if (RT_FAILURE(rc))
            break;

        pCfgAcq->Props.uHz       = obt.freq;
        pCfgAcq->Props.cChannels = obt.nchannels;

        rc = alsaALSAToAudioProps(obt.fmt, &pCfgAcq->Props);
        if (RT_FAILURE(rc))
            break;

        pCfgAcq->cFrameBufferHint = obt.samples;

        AssertBreakStmt(obt.samples, rc = VERR_INVALID_PARAMETER);

        size_t cbBuf = obt.samples * PDMAUDIOSTREAMCFG_F2B(pCfgAcq, 1);
        AssertBreakStmt(cbBuf, rc = VERR_INVALID_PARAMETER);

        pStreamALSA->pvBuf = RTMemAlloc(cbBuf);
        if (!pStreamALSA->pvBuf)
        {
            LogRel(("ALSA: Not enough memory for input ADC buffer (%RU32 samples, %zu bytes)\n", obt.samples, cbBuf));
            rc = VERR_NO_MEMORY;
            break;
        }

        pStreamALSA->cbBuf = cbBuf;
        pStreamALSA->phPCM = phPCM;
    }
    while (0);

    if (RT_FAILURE(rc))
        alsaStreamClose(&phPCM);

    LogFlowFuncLeaveRC(rc);
    return rc;
}


static int alsaControlStreamIn(PALSAAUDIOSTREAM pStreamALSA, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    int rc;
    switch (enmStreamCmd)
    {
        case PDMAUDIOSTREAMCMD_ENABLE:
        case PDMAUDIOSTREAMCMD_RESUME:
            rc = drvHostALSAAudioStreamCtl(pStreamALSA, false /* fStop */);
            break;

        case PDMAUDIOSTREAMCMD_DISABLE:
        case PDMAUDIOSTREAMCMD_PAUSE:
            rc = drvHostALSAAudioStreamCtl(pStreamALSA, true /* fStop */);
            break;

        default:
            AssertMsgFailed(("Invalid command %ld\n", enmStreamCmd));
            rc = VERR_INVALID_PARAMETER;
            break;
    }

    return rc;
}


static int alsaControlStreamOut(PALSAAUDIOSTREAM pStreamALSA, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    int rc;
    switch (enmStreamCmd)
    {
        case PDMAUDIOSTREAMCMD_ENABLE:
        case PDMAUDIOSTREAMCMD_RESUME:
            rc = drvHostALSAAudioStreamCtl(pStreamALSA, false /* fStop */);
            break;

        case PDMAUDIOSTREAMCMD_DISABLE:
        case PDMAUDIOSTREAMCMD_PAUSE:
            rc = drvHostALSAAudioStreamCtl(pStreamALSA, true /* fStop */);
            break;

        default:
            AssertMsgFailed(("Invalid command %ld\n", enmStreamCmd));
            rc = VERR_INVALID_PARAMETER;
            break;
    }

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetConfig}
 */
static DECLCALLBACK(int) drvHostALSAAudioGetConfig(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDCFG pBackendCfg)
{
    RT_NOREF(pInterface);
    AssertPtrReturn(pBackendCfg, VERR_INVALID_POINTER);

    pBackendCfg->cbStreamIn  = sizeof(ALSAAUDIOSTREAM);
    pBackendCfg->cbStreamOut = sizeof(ALSAAUDIOSTREAM);

    /* Enumerate sound devices. */
    char **pszHints;
    int err = snd_device_name_hint(-1 /* All cards */, "pcm", (void***)&pszHints);
    if (err == 0)
    {
        char** pszHintCur = pszHints;
        while (*pszHintCur != NULL)
        {
            char *pszDev = snd_device_name_get_hint(*pszHintCur, "NAME");
            bool fSkip =    !pszDev
                         || !RTStrICmp("null", pszDev);
            if (fSkip)
            {
                if (pszDev)
                    free(pszDev);
                pszHintCur++;
                continue;
            }

            char *pszIOID = snd_device_name_get_hint(*pszHintCur, "IOID");
            if (pszIOID)
            {
#if 0
                if (!RTStrICmp("input", pszIOID))

                else if (!RTStrICmp("output", pszIOID))
#endif
            }
            else /* NULL means bidirectional, input + output. */
            {
            }

            LogRel2(("ALSA: Found %s device: %s\n", pszIOID ?  RTStrToLower(pszIOID) : "bidirectional", pszDev));

            /* Special case for ALSAAudio. */
            if (   pszDev
                && RTStrIStr("pulse", pszDev) != NULL)
                LogRel2(("ALSA: ALSAAudio plugin in use\n"));

            if (pszIOID)
                free(pszIOID);

            if (pszDev)
                free(pszDev);

            pszHintCur++;
        }

        snd_device_name_free_hint((void **)pszHints);
        pszHints = NULL;
    }
    else
        LogRel2(("ALSA: Error enumerating PCM devices: %Rrc (%d)\n", RTErrConvertFromErrno(err), err));

    /* ALSA allows exactly one input and one output used at a time for the selected device(s). */
    pBackendCfg->cMaxStreamsIn  = 1;
    pBackendCfg->cMaxStreamsOut = 1;

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnShutdown}
 */
static DECLCALLBACK(void) drvHostALSAAudioShutdown(PPDMIHOSTAUDIO pInterface)
{
    RT_NOREF(pInterface);
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetStatus}
 */
static DECLCALLBACK(PDMAUDIOBACKENDSTS) drvHostALSAAudioGetStatus(PPDMIHOSTAUDIO pInterface, PDMAUDIODIR enmDir)
{
    RT_NOREF(enmDir);
    AssertPtrReturn(pInterface, PDMAUDIOBACKENDSTS_UNKNOWN);

    return PDMAUDIOBACKENDSTS_RUNNING;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamCreate}
 */
static DECLCALLBACK(int) drvHostALSAAudioStreamCreate(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream,
                                                      PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgReq,    VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgAcq,    VERR_INVALID_POINTER);

    PALSAAUDIOSTREAM pStreamALSA = (PALSAAUDIOSTREAM)pStream;

    int rc;
    if (pCfgReq->enmDir == PDMAUDIODIR_IN)
        rc = alsaCreateStreamIn (pStreamALSA, pCfgReq, pCfgAcq);
    else
        rc = alsaCreateStreamOut(pStreamALSA, pCfgReq, pCfgAcq);

    if (RT_SUCCESS(rc))
    {
        pStreamALSA->pCfg = DrvAudioHlpStreamCfgDup(pCfgAcq);
        if (!pStreamALSA->pCfg)
            rc = VERR_NO_MEMORY;
    }

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamDestroy}
 */
static DECLCALLBACK(int) drvHostALSAAudioStreamDestroy(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    PALSAAUDIOSTREAM pStreamALSA = (PALSAAUDIOSTREAM)pStream;

    if (!pStreamALSA->pCfg) /* Not (yet) configured? Skip. */
        return VINF_SUCCESS;

    int rc;
    if (pStreamALSA->pCfg->enmDir == PDMAUDIODIR_IN)
        rc = alsaDestroyStreamIn(pStreamALSA);
    else
        rc = alsaDestroyStreamOut(pStreamALSA);

    if (RT_SUCCESS(rc))
    {
        DrvAudioHlpStreamCfgFree(pStreamALSA->pCfg);
        pStreamALSA->pCfg = NULL;
    }

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamControl}
 */
static DECLCALLBACK(int) drvHostALSAAudioStreamControl(PPDMIHOSTAUDIO pInterface,
                                                       PPDMAUDIOBACKENDSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    PALSAAUDIOSTREAM pStreamALSA = (PALSAAUDIOSTREAM)pStream;

    if (!pStreamALSA->pCfg) /* Not (yet) configured? Skip. */
        return VINF_SUCCESS;

    int rc;
    if (pStreamALSA->pCfg->enmDir == PDMAUDIODIR_IN)
        rc = alsaControlStreamIn (pStreamALSA, enmStreamCmd);
    else
        rc = alsaControlStreamOut(pStreamALSA, enmStreamCmd);

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetReadable}
 */
static DECLCALLBACK(uint32_t) drvHostALSAAudioStreamGetReadable(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface);

    PALSAAUDIOSTREAM pStreamALSA = (PALSAAUDIOSTREAM)pStream;

    uint32_t cbAvail = 0;

    snd_pcm_sframes_t cFramesAvail;
    int rc = alsaStreamGetAvail(pStreamALSA->phPCM, &cFramesAvail);
    if (RT_SUCCESS(rc))
        cbAvail = PDMAUDIOSTREAMCFG_F2B(pStreamALSA->pCfg, cFramesAvail);

    return cbAvail;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetWritable}
 */
static DECLCALLBACK(uint32_t) drvHostALSAAudioStreamGetWritable(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface);

    PALSAAUDIOSTREAM pStreamALSA = (PALSAAUDIOSTREAM)pStream;

    uint32_t cbAvail = 0;

    snd_pcm_sframes_t cFramesAvail;
    int rc = alsaStreamGetAvail(pStreamALSA->phPCM, &cFramesAvail);
    if (   RT_SUCCESS(rc)
        && (uint32_t)cFramesAvail >= pStreamALSA->Out.cSamplesMin)
    {
        cbAvail = PDMAUDIOSTREAMCFG_F2B(pStreamALSA->pCfg, cFramesAvail);
    }

    return cbAvail;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetStatus}
 */
static DECLCALLBACK(PDMAUDIOSTREAMSTS) drvHostALSAAudioStreamGetStatus(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface, pStream);

    return (PDMAUDIOSTREAMSTS_FLAG_INITIALIZED | PDMAUDIOSTREAMSTS_FLAG_ENABLED);
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamIterate}
 */
static DECLCALLBACK(int) drvHostALSAAudioStreamIterate(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    LogFlowFuncEnter();

    /* Nothing to do here for ALSA. */
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) drvHostALSAAudioQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS        pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVHOSTALSAAUDIO pThis   = PDMINS_2_DATA(pDrvIns, PDRVHOSTALSAAUDIO);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIHOSTAUDIO, &pThis->IHostAudio);

    return NULL;
}


/**
 * Construct a DirectSound Audio driver instance.
 *
 * @copydoc FNPDMDRVCONSTRUCT
 */
static DECLCALLBACK(int) drvHostAlsaAudioConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    RT_NOREF(pCfg, fFlags);
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);
    PDRVHOSTALSAAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTALSAAUDIO);
    LogRel(("Audio: Initializing ALSA driver\n"));

    /*
     * Init the static parts.
     */
    pThis->pDrvIns                   = pDrvIns;
    /* IBase */
    pDrvIns->IBase.pfnQueryInterface = drvHostALSAAudioQueryInterface;
    /* IHostAudio */
    PDMAUDIO_IHOSTAUDIO_CALLBACKS(drvHostALSAAudio);

    return VINF_SUCCESS;
}


/**
 * Char driver registration record.
 */
const PDMDRVREG g_DrvHostALSAAudio =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "ALSAAudio",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "ALSA host audio driver",
    /* fFlags */
     PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_AUDIO,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(DRVHOSTALSAAUDIO),
    /* pfnConstruct */
    drvHostAlsaAudioConstruct,
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

