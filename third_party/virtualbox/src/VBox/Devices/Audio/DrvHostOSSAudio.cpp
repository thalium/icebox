/* $Id: DrvHostOSSAudio.cpp $ */
/** @file
 * OSS (Open Sound System) host audio backend.
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

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/soundcard.h>
#include <unistd.h>

#include <iprt/alloc.h>
#include <iprt/uuid.h> /* For PDMIBASE_2_PDMDRV. */

#define LOG_GROUP LOG_GROUP_DRV_HOST_AUDIO
#include <VBox/log.h>
#include <VBox/vmm/pdmaudioifs.h>

#include "DrvAudio.h"
#include "VBoxDD.h"


/*********************************************************************************************************************************
*   Defines                                                                                                                      *
*********************************************************************************************************************************/

#if ((SOUND_VERSION > 360) && (defined(OSS_SYSINFO)))
/* OSS > 3.6 has a new syscall available for querying a bit more detailed information
 * about OSS' audio capabilities. This is handy for e.g. Solaris. */
# define VBOX_WITH_AUDIO_OSS_SYSINFO 1
#endif

/** Makes DRVHOSTOSSAUDIO out of PDMIHOSTAUDIO. */
#define PDMIHOSTAUDIO_2_DRVHOSTOSSAUDIO(pInterface) \
    ( (PDRVHOSTOSSAUDIO)((uintptr_t)pInterface - RT_OFFSETOF(DRVHOSTOSSAUDIO, IHostAudio)) )


/*********************************************************************************************************************************
*   Structures                                                                                                                   *
*********************************************************************************************************************************/

/**
 * OSS host audio driver instance data.
 * @implements PDMIAUDIOCONNECTOR
 */
typedef struct DRVHOSTOSSAUDIO
{
    /** Pointer to the driver instance structure. */
    PPDMDRVINS         pDrvIns;
    /** Pointer to host audio interface. */
    PDMIHOSTAUDIO      IHostAudio;
    /** Error count for not flooding the release log.
     *  UINT32_MAX for unlimited logging. */
    uint32_t           cLogErrors;
} DRVHOSTOSSAUDIO, *PDRVHOSTOSSAUDIO;

typedef struct OSSAUDIOSTREAMCFG
{
    PDMAUDIOPCMPROPS  Props;
    uint16_t          cFragments;
    uint32_t          cbFragmentSize;
} OSSAUDIOSTREAMCFG, *POSSAUDIOSTREAMCFG;

typedef struct OSSAUDIOSTREAM
{
    /** The stream's acquired configuration. */
    PPDMAUDIOSTREAMCFG pCfg;
    /** Buffer alignment. */
    uint8_t            uAlign;
    union
    {
        struct
        {

        } In;
        struct
        {
#ifndef RT_OS_L4
            /** Whether we use a memory mapped file instead of our
             *  own allocated PCM buffer below. */
            /** @todo The memory mapped code seems to be utterly broken.
             *        Needs investigation! */
            bool       fMMIO;
#endif
        } Out;
    };
    int                hFile;
    int                cFragments;
    int                cbFragmentSize;
    /** Own PCM buffer. */
    void              *pvBuf;
    /** Size (in bytes) of own PCM buffer. */
    size_t             cbBuf;
    int                old_optr;
} OSSAUDIOSTREAM, *POSSAUDIOSTREAM;

typedef struct OSSAUDIOCFG
{
#ifndef RT_OS_L4
    bool try_mmap;
#endif
    int nfrags;
    int fragsize;
    const char *devpath_out;
    const char *devpath_in;
    int debug;
} OSSAUDIOCFG, *POSSAUDIOCFG;

static OSSAUDIOCFG s_OSSConf =
{
#ifndef RT_OS_L4
    false,
#endif
    4,
    4096,
    "/dev/dsp",
    "/dev/dsp",
    0
};


/* http://www.df.lth.se/~john_e/gems/gem002d.html */
static uint32_t popcount(uint32_t u)
{
    u = ((u&0x55555555) + ((u>>1)&0x55555555));
    u = ((u&0x33333333) + ((u>>2)&0x33333333));
    u = ((u&0x0f0f0f0f) + ((u>>4)&0x0f0f0f0f));
    u = ((u&0x00ff00ff) + ((u>>8)&0x00ff00ff));
    u = ( u&0x0000ffff) + (u>>16);
    return u;
}


static uint32_t lsbindex(uint32_t u)
{
    return popcount ((u&-u)-1);
}


static int ossOSSToAudioProps(int fmt, PPDMAUDIOPCMPROPS pProps)
{
    RT_BZERO(pProps, sizeof(PDMAUDIOPCMPROPS));

    switch (fmt)
    {
        case AFMT_S8:
            pProps->cBits   = 8;
            pProps->fSigned = true;
            break;

        case AFMT_U8:
            pProps->cBits   = 8;
            pProps->fSigned = false;
            break;

        case AFMT_S16_LE:
            pProps->cBits   = 16;
            pProps->fSigned = true;
            break;

        case AFMT_U16_LE:
            pProps->cBits   = 16;
            pProps->fSigned = false;
            break;

       case AFMT_S16_BE:
            pProps->cBits   = 16;
            pProps->fSigned = true;
#ifdef RT_LITTLE_ENDIAN
            pProps->fSwapEndian = true;
#endif
            break;

        case AFMT_U16_BE:
            pProps->cBits   = 16;
            pProps->fSigned = false;
#ifdef RT_LITTLE_ENDIAN
            pProps->fSwapEndian = true;
#endif
            break;

        default:
            AssertMsgFailed(("Format %ld not supported\n", fmt));
            return VERR_NOT_SUPPORTED;
    }

    return VINF_SUCCESS;
}


static int ossStreamClose(int *phFile)
{
    if (!phFile || !*phFile || *phFile == -1)
        return VINF_SUCCESS;

    int rc;
    if (close(*phFile))
    {
        LogRel(("OSS: Closing stream failed: %s\n", strerror(errno)));
        rc = VERR_GENERAL_FAILURE; /** @todo */
    }
    else
    {
        *phFile = -1;
        rc = VINF_SUCCESS;
    }

    return rc;
}


static int ossStreamOpen(const char *pszDev, int fOpen, POSSAUDIOSTREAMCFG pOSSReq, POSSAUDIOSTREAMCFG pOSSAcq, int *phFile)
{
    int rc = VINF_SUCCESS;

    int hFile = -1;
    do
    {
        hFile = open(pszDev, fOpen);
        if (hFile == -1)
        {
            LogRel(("OSS: Failed to open %s: %s (%d)\n", pszDev, strerror(errno), errno));
            rc = RTErrConvertFromErrno(errno);
            break;
        }

        int iFormat;
        switch (pOSSReq->Props.cBits)
        {
            case 8:
                iFormat = pOSSReq->Props.fSigned ? AFMT_S8 : AFMT_U8;
                break;

            case 16:
                iFormat = pOSSReq->Props.fSigned ? AFMT_S16_LE : AFMT_U16_LE;
                break;

            default:
                rc = VERR_NOT_SUPPORTED;
                break;
        }

        if (RT_FAILURE(rc))
            break;

        if (ioctl(hFile, SNDCTL_DSP_SAMPLESIZE, &iFormat))
        {
            LogRel(("OSS: Failed to set audio format to %ld: %s (%d)\n", iFormat, strerror(errno), errno));
            rc = RTErrConvertFromErrno(errno);
            break;
        }

        int cChannels = pOSSReq->Props.cChannels;
        if (ioctl(hFile, SNDCTL_DSP_CHANNELS, &cChannels))
        {
            LogRel(("OSS: Failed to set number of audio channels (%RU8): %s (%d)\n",
                    pOSSReq->Props.cChannels, strerror(errno), errno));
            rc = RTErrConvertFromErrno(errno);
            break;
        }

        int freq = pOSSReq->Props.uHz;
        if (ioctl(hFile, SNDCTL_DSP_SPEED, &freq))
        {
            LogRel(("OSS: Failed to set audio frequency (%dHZ): %s (%d)\n", pOSSReq->Props.uHz, strerror(errno), errno));
            rc = RTErrConvertFromErrno(errno);
            break;
        }

        /* Obsolete on Solaris (using O_NONBLOCK is sufficient). */
#if !(defined(VBOX) && defined(RT_OS_SOLARIS))
        if (ioctl(hFile, SNDCTL_DSP_NONBLOCK))
        {
            LogRel(("OSS: Failed to set non-blocking mode: %s (%d)\n", strerror(errno), errno));
            rc = RTErrConvertFromErrno(errno);
            break;
        }
#endif

        /* Check access mode (input or output). */
        bool fIn = ((fOpen & O_ACCMODE) == O_RDONLY);

        LogRel2(("OSS: Requested %RU16 %s fragments, %RU32 bytes each\n",
                 pOSSReq->cFragments, fIn ? "input" : "output", pOSSReq->cbFragmentSize));

        int mmmmssss = (pOSSReq->cFragments << 16) | lsbindex(pOSSReq->cbFragmentSize);
        if (ioctl(hFile, SNDCTL_DSP_SETFRAGMENT, &mmmmssss))
        {
            LogRel(("OSS: Failed to set %RU16 fragments to %RU32 bytes each: %s (%d)\n",
                    pOSSReq->cFragments, pOSSReq->cbFragmentSize, strerror(errno), errno));
            rc = RTErrConvertFromErrno(errno);
            break;
        }

        audio_buf_info abinfo;
        if (ioctl(hFile, fIn ? SNDCTL_DSP_GETISPACE : SNDCTL_DSP_GETOSPACE, &abinfo))
        {
            LogRel(("OSS: Failed to retrieve %s buffer length: %s (%d)\n", fIn ? "input" : "output", strerror(errno), errno));
            rc = RTErrConvertFromErrno(errno);
            break;
        }

        rc = ossOSSToAudioProps(iFormat, &pOSSAcq->Props);
        if (RT_SUCCESS(rc))
        {
            pOSSAcq->Props.cChannels = cChannels;
            pOSSAcq->Props.uHz       = freq;
            pOSSAcq->Props.cShift    = PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(pOSSAcq->Props.cBits, pOSSAcq->Props.cChannels);

            pOSSAcq->cFragments      = abinfo.fragstotal;
            pOSSAcq->cbFragmentSize  = abinfo.fragsize;

            LogRel2(("OSS: Got %RU16 %s fragments, %RU32 bytes each\n",
                     pOSSAcq->cFragments, fIn ? "input" : "output", pOSSAcq->cbFragmentSize));

            *phFile = hFile;
        }
    }
    while (0);

    if (RT_FAILURE(rc))
        ossStreamClose(&hFile);

    LogFlowFuncLeaveRC(rc);
    return rc;
}


static int ossControlStreamIn(/*PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd*/ void)
{
    /** @todo Nothing to do here right now!? */

    return VINF_SUCCESS;
}


static int ossControlStreamOut(PPDMAUDIOBACKENDSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    POSSAUDIOSTREAM pStreamOSS = (POSSAUDIOSTREAM)pStream;

    int rc = VINF_SUCCESS;

    switch (enmStreamCmd)
    {
        case PDMAUDIOSTREAMCMD_ENABLE:
        case PDMAUDIOSTREAMCMD_RESUME:
        {
            DrvAudioHlpClearBuf(&pStreamOSS->pCfg->Props, pStreamOSS->pvBuf, pStreamOSS->cbBuf,
                                PDMAUDIOPCMPROPS_B2F(&pStreamOSS->pCfg->Props, pStreamOSS->cbBuf));

            int mask = PCM_ENABLE_OUTPUT;
            if (ioctl(pStreamOSS->hFile, SNDCTL_DSP_SETTRIGGER, &mask) < 0)
            {
                LogRel(("OSS: Failed to enable output stream: %s\n", strerror(errno)));
                rc = RTErrConvertFromErrno(errno);
            }

            break;
        }

        case PDMAUDIOSTREAMCMD_DISABLE:
        case PDMAUDIOSTREAMCMD_PAUSE:
        {
            int mask = 0;
            if (ioctl(pStreamOSS->hFile, SNDCTL_DSP_SETTRIGGER, &mask) < 0)
            {
                LogRel(("OSS: Failed to disable output stream: %s\n", strerror(errno)));
                rc = RTErrConvertFromErrno(errno);
            }

            break;
        }

        default:
            AssertMsgFailed(("Invalid command %ld\n", enmStreamCmd));
            rc = VERR_INVALID_PARAMETER;
            break;
    }

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnInit}
 */
static DECLCALLBACK(int) drvHostOSSAudioInit(PPDMIHOSTAUDIO pInterface)
{
    RT_NOREF(pInterface);

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamCapture}
 */
static DECLCALLBACK(int) drvHostOSSAudioStreamCapture(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream,
                                                      void *pvBuf, uint32_t cxBuf, uint32_t *pcxRead)
{
    RT_NOREF(pInterface);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    POSSAUDIOSTREAM pStreamOSS = (POSSAUDIOSTREAM)pStream;

    int rc = VINF_SUCCESS;

    size_t cbToRead = RT_MIN(pStreamOSS->cbBuf, cxBuf);

    LogFlowFunc(("cbToRead=%zi\n", cbToRead));

    uint32_t cbReadTotal = 0;
    uint32_t cbTemp;
    ssize_t  cbRead;
    size_t   offWrite = 0;

    while (cbToRead)
    {
        cbTemp = RT_MIN(cbToRead, pStreamOSS->cbBuf);
        AssertBreakStmt(cbTemp, rc = VERR_NO_DATA);
        cbRead = read(pStreamOSS->hFile, (uint8_t *)pStreamOSS->pvBuf, cbTemp);

        LogFlowFunc(("cbRead=%zi, cbTemp=%RU32, cbToRead=%zu\n", cbRead, cbTemp, cbToRead));

        if (cbRead < 0)
        {
            switch (errno)
            {
                case 0:
                {
                    LogFunc(("Failed to read %z frames\n", cbRead));
                    rc = VERR_ACCESS_DENIED;
                    break;
                }

                case EINTR:
                case EAGAIN:
                    rc = VERR_NO_DATA;
                    break;

                default:
                    LogFlowFunc(("Failed to read %zu input frames, rc=%Rrc\n", cbTemp, rc));
                    rc = VERR_GENERAL_FAILURE; /** @todo Fix this. */
                    break;
            }

            if (RT_FAILURE(rc))
                break;
        }
        else if (cbRead)
        {
            memcpy((uint8_t *)pvBuf + offWrite, pStreamOSS->pvBuf, cbRead);

            Assert((ssize_t)cbToRead >= cbRead);
            cbToRead    -= cbRead;
            offWrite    += cbRead;
            cbReadTotal += cbRead;
        }
        else /* No more data, try next round. */
            break;
    }

    if (rc == VERR_NO_DATA)
        rc = VINF_SUCCESS;

    if (RT_SUCCESS(rc))
    {
        if (pcxRead)
            *pcxRead = cbReadTotal;
    }

    return rc;
}


static int ossDestroyStreamIn(PPDMAUDIOBACKENDSTREAM pStream)
{
    POSSAUDIOSTREAM pStreamOSS = (POSSAUDIOSTREAM)pStream;

    LogFlowFuncEnter();

    if (pStreamOSS->pvBuf)
    {
        Assert(pStreamOSS->cbBuf);

        RTMemFree(pStreamOSS->pvBuf);
        pStreamOSS->pvBuf = NULL;
    }

    pStreamOSS->cbBuf = 0;

    ossStreamClose(&pStreamOSS->hFile);

    return VINF_SUCCESS;
}


static int ossDestroyStreamOut(PPDMAUDIOBACKENDSTREAM pStream)
{
    POSSAUDIOSTREAM pStreamOSS = (POSSAUDIOSTREAM)pStream;

#ifndef RT_OS_L4
    if (pStreamOSS->Out.fMMIO)
    {
        if (pStreamOSS->pvBuf)
        {
            Assert(pStreamOSS->cbBuf);

            int rc2 = munmap(pStreamOSS->pvBuf, pStreamOSS->cbBuf);
            if (rc2 == 0)
            {
                pStreamOSS->pvBuf      = NULL;
                pStreamOSS->cbBuf      = 0;

                pStreamOSS->Out.fMMIO  = false;
            }
            else
                LogRel(("OSS: Failed to memory unmap playback buffer on close: %s\n", strerror(errno)));
        }
    }
    else
    {
#endif
        if (pStreamOSS->pvBuf)
        {
            Assert(pStreamOSS->cbBuf);

            RTMemFree(pStreamOSS->pvBuf);
            pStreamOSS->pvBuf = NULL;
        }

        pStreamOSS->cbBuf = 0;
#ifndef RT_OS_L4
    }
#endif

    ossStreamClose(&pStreamOSS->hFile);

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetConfig}
 */
static DECLCALLBACK(int) drvHostOSSAudioGetConfig(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDCFG pBackendCfg)
{
    RT_NOREF(pInterface);

    pBackendCfg->cbStreamIn  = sizeof(OSSAUDIOSTREAM);
    pBackendCfg->cbStreamOut = sizeof(OSSAUDIOSTREAM);

    int hFile = open("/dev/dsp", O_WRONLY | O_NONBLOCK, 0);
    if (hFile == -1)
    {
        /* Try opening the mixing device instead. */
        hFile = open("/dev/mixer", O_RDONLY | O_NONBLOCK, 0);
    }

    int ossVer = -1;

#ifdef VBOX_WITH_AUDIO_OSS_SYSINFO
    oss_sysinfo ossInfo;
    RT_ZERO(ossInfo);
#endif

    if (hFile != -1)
    {
        int err = ioctl(hFile, OSS_GETVERSION, &ossVer);
        if (err == 0)
        {
            LogRel2(("OSS: Using version: %d\n", ossVer));
#ifdef VBOX_WITH_AUDIO_OSS_SYSINFO
            err = ioctl(hFile, OSS_SYSINFO, &ossInfo);
            if (err == 0)
            {
                LogRel2(("OSS: Number of DSPs: %d\n", ossInfo.numaudios));
                LogRel2(("OSS: Number of mixers: %d\n", ossInfo.nummixers));

                int cDev = ossInfo.nummixers;
                if (!cDev)
                    cDev = ossInfo.numaudios;

                pBackendCfg->cMaxStreamsIn   = UINT32_MAX;
                pBackendCfg->cMaxStreamsOut  = UINT32_MAX;
            }
            else
            {
#endif
                /* Since we cannot query anything, assume that we have at least
                 * one input and one output if we found "/dev/dsp" or "/dev/mixer". */

                pBackendCfg->cMaxStreamsIn   = UINT32_MAX;
                pBackendCfg->cMaxStreamsOut  = UINT32_MAX;
#ifdef VBOX_WITH_AUDIO_OSS_SYSINFO
            }
#endif
        }
        else
            LogRel(("OSS: Unable to determine installed version: %s (%d)\n", strerror(err), err));
    }
    else
        LogRel(("OSS: No devices found, audio is not available\n"));

    return VINF_SUCCESS;
}


static int ossCreateStreamIn(POSSAUDIOSTREAM pStreamOSS, PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    int rc;

    int hFile = -1;

    do
    {
        OSSAUDIOSTREAMCFG ossReq;
        memcpy(&ossReq.Props, &pCfgReq->Props, sizeof(PDMAUDIOPCMPROPS));

        ossReq.cFragments     = s_OSSConf.nfrags;
        ossReq.cbFragmentSize = s_OSSConf.fragsize;

        OSSAUDIOSTREAMCFG ossAcq;

        rc = ossStreamOpen(s_OSSConf.devpath_in, O_RDONLY | O_NONBLOCK, &ossReq, &ossAcq, &hFile);
        if (RT_SUCCESS(rc))
        {
            memcpy(&pCfgAcq->Props, &ossAcq.Props, sizeof(PDMAUDIOPCMPROPS));

            if (ossAcq.cFragments * ossAcq.cbFragmentSize & pStreamOSS->uAlign)
            {
                LogRel(("OSS: Warning: Misaligned capturing buffer: Size = %zu, Alignment = %u\n",
                        ossAcq.cFragments * ossAcq.cbFragmentSize, pStreamOSS->uAlign + 1));
            }

            uint32_t cSamples = PDMAUDIOSTREAMCFG_B2F(pCfgAcq, ossAcq.cFragments * ossAcq.cbFragmentSize);
            if (!cSamples)
                rc = VERR_INVALID_PARAMETER;

            if (RT_SUCCESS(rc))
            {
                size_t cbBuf = PDMAUDIOSTREAMCFG_F2B(pCfgAcq, cSamples);
                void  *pvBuf = RTMemAlloc(cbBuf);
                if (!pvBuf)
                {
                    LogRel(("OSS: Failed allocating capturing buffer with (%zu bytes)\n", cbBuf));
                    rc = VERR_NO_MEMORY;
                }

                pStreamOSS->hFile = hFile;
                pStreamOSS->pvBuf = pvBuf;
                pStreamOSS->cbBuf = cbBuf;

                pCfgAcq->cFrameBufferHint = cSamples;
            }
        }

    } while (0);

    if (RT_FAILURE(rc))
        ossStreamClose(&hFile);

    LogFlowFuncLeaveRC(rc);
    return rc;
}


static int ossCreateStreamOut(POSSAUDIOSTREAM pStreamOSS, PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    int rc;
    int hFile = -1;

    do
    {
        uint32_t cSamples;

        OSSAUDIOSTREAMCFG reqStream, obtStream;

        memcpy(&reqStream.Props, &pCfgReq->Props, sizeof(PDMAUDIOPCMPROPS));

        reqStream.cFragments     = s_OSSConf.nfrags;
        reqStream.cbFragmentSize = s_OSSConf.fragsize;

        rc = ossStreamOpen(s_OSSConf.devpath_out, O_WRONLY, &reqStream, &obtStream, &hFile);
        if (RT_SUCCESS(rc))
        {
            memcpy(&pCfgAcq->Props, &obtStream.Props, sizeof(PDMAUDIOPCMPROPS));

            cSamples = PDMAUDIOSTREAMCFG_B2F(pCfgAcq, obtStream.cFragments * obtStream.cbFragmentSize);

            if (obtStream.cFragments * obtStream.cbFragmentSize & pStreamOSS->uAlign)
            {
                LogRel(("OSS: Warning: Misaligned playback buffer: Size = %zu, Alignment = %u\n",
                        obtStream.cFragments * obtStream.cbFragmentSize, pStreamOSS->uAlign + 1));
            }
        }

        if (RT_SUCCESS(rc))
        {
            pStreamOSS->Out.fMMIO = false;

            size_t cbBuf = PDMAUDIOSTREAMCFG_F2B(pCfgAcq, cSamples);
            Assert(cbBuf);

#ifndef RT_OS_L4
            if (s_OSSConf.try_mmap)
            {
                pStreamOSS->pvBuf = mmap(0, cbBuf, PROT_READ | PROT_WRITE, MAP_SHARED, hFile, 0);
                if (pStreamOSS->pvBuf == MAP_FAILED)
                {
                    LogRel(("OSS: Failed to memory map %zu bytes of playback buffer: %s\n", cbBuf, strerror(errno)));
                    rc = RTErrConvertFromErrno(errno);
                    break;
                }
                else
                {
                    int mask = 0;
                    if (ioctl(hFile, SNDCTL_DSP_SETTRIGGER, &mask) < 0)
                    {
                        LogRel(("OSS: Failed to retrieve initial trigger mask for playback buffer: %s\n", strerror(errno)));
                        rc = RTErrConvertFromErrno(errno);
                        /* Note: No break here, need to unmap file first! */
                    }
                    else
                    {
                        mask = PCM_ENABLE_OUTPUT;
                        if (ioctl (hFile, SNDCTL_DSP_SETTRIGGER, &mask) < 0)
                        {
                            LogRel(("OSS: Failed to retrieve PCM_ENABLE_OUTPUT mask: %s\n", strerror(errno)));
                            rc = RTErrConvertFromErrno(errno);
                            /* Note: No break here, need to unmap file first! */
                        }
                        else
                        {
                            pStreamOSS->Out.fMMIO = true;
                            LogRel(("OSS: Using MMIO\n"));
                        }
                    }

                    if (RT_FAILURE(rc))
                    {
                        int rc2 = munmap(pStreamOSS->pvBuf, cbBuf);
                        if (rc2)
                            LogRel(("OSS: Failed to memory unmap playback buffer: %s\n", strerror(errno)));
                        break;
                    }
                }
            }
#endif /* !RT_OS_L4 */

            /* Memory mapping failed above? Try allocating an own buffer. */
#ifndef RT_OS_L4
            if (!pStreamOSS->Out.fMMIO)
            {
#endif
                void *pvBuf = RTMemAlloc(cbBuf);
                if (!pvBuf)
                {
                    LogRel(("OSS: Failed allocating playback buffer with %RU32 samples (%zu bytes)\n", cSamples, cbBuf));
                    rc = VERR_NO_MEMORY;
                    break;
                }

                pStreamOSS->hFile = hFile;
                pStreamOSS->pvBuf = pvBuf;
                pStreamOSS->cbBuf = cbBuf;
#ifndef RT_OS_L4
            }
#endif
            pCfgAcq->cFrameBufferHint = cSamples;
        }

    } while (0);

    if (RT_FAILURE(rc))
        ossStreamClose(&hFile);

    LogFlowFuncLeaveRC(rc);
    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamPlay}
 */
static DECLCALLBACK(int) drvHostOSSAudioStreamPlay(PPDMIHOSTAUDIO pInterface,
                                                   PPDMAUDIOBACKENDSTREAM pStream, const void *pvBuf, uint32_t cxBuf,
                                                   uint32_t *pcxWritten)
{
    RT_NOREF(pInterface);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    POSSAUDIOSTREAM pStreamOSS = (POSSAUDIOSTREAM)pStream;

    int rc = VINF_SUCCESS;
    uint32_t cbWrittenTotal = 0;

#ifndef RT_OS_L4
    count_info cntinfo;
#endif

    do
    {
        uint32_t cbAvail = cxBuf;
        uint32_t cbToWrite;

#ifndef RT_OS_L4
        if (pStreamOSS->Out.fMMIO)
        {
            /* Get current playback pointer. */
            int rc2 = ioctl(pStreamOSS->hFile, SNDCTL_DSP_GETOPTR, &cntinfo);
            if (!rc2)
            {
                LogRel(("OSS: Failed to retrieve current playback pointer: %s\n", strerror(errno)));
                rc = RTErrConvertFromErrno(errno);
                break;
            }

            /* Nothing to play? */
            if (cntinfo.ptr == pStreamOSS->old_optr)
                break;

            int cbData;
            if (cntinfo.ptr > pStreamOSS->old_optr)
                cbData = cntinfo.ptr - pStreamOSS->old_optr;
            else
                cbData = cxBuf + cntinfo.ptr - pStreamOSS->old_optr;
            Assert(cbData >= 0);

            cbToWrite = RT_MIN((unsigned)cbData, cbAvail);
        }
        else
        {
#endif
            audio_buf_info abinfo;
            int rc2 = ioctl(pStreamOSS->hFile, SNDCTL_DSP_GETOSPACE, &abinfo);
            if (rc2 < 0)
            {
                LogRel(("OSS: Failed to retrieve current playback buffer: %s\n", strerror(errno)));
                rc = RTErrConvertFromErrno(errno);
                break;
            }

            if ((size_t)abinfo.bytes > cxBuf)
            {
                LogRel2(("OSS: Warning: Too big output size (%d > %RU32), limiting to %RU32\n", abinfo.bytes, cxBuf, cxBuf));
                abinfo.bytes = cxBuf;
                /* Keep going. */
            }

            if (abinfo.bytes < 0)
            {
                LogRel2(("OSS: Warning: Invalid available size (%d vs. %RU32)\n", abinfo.bytes, cxBuf));
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            cbToWrite = RT_MIN(unsigned(abinfo.fragments * abinfo.fragsize), cbAvail);
#ifndef RT_OS_L4
        }
#endif
        cbToWrite = RT_MIN(cbToWrite, pStreamOSS->cbBuf);

        while (cbToWrite)
        {
            uint32_t cbWritten = cbToWrite;

            memcpy(pStreamOSS->pvBuf, pvBuf, cbWritten);

            uint32_t cbChunk    = cbWritten;
            uint32_t cbChunkOff = 0;
            while (cbChunk)
            {
                ssize_t cbChunkWritten = write(pStreamOSS->hFile, (uint8_t *)pStreamOSS->pvBuf + cbChunkOff,
                                               RT_MIN(cbChunk, (unsigned)s_OSSConf.fragsize));
                if (cbChunkWritten < 0)
                {
                    LogRel(("OSS: Failed writing output data: %s\n", strerror(errno)));
                    rc = RTErrConvertFromErrno(errno);
                    break;
                }

                if (cbChunkWritten & pStreamOSS->uAlign)
                {
                    LogRel(("OSS: Misaligned write (written %z, expected %RU32)\n", cbChunkWritten, cbChunk));
                    break;
                }

                cbChunkOff += (uint32_t)cbChunkWritten;
                Assert(cbChunkOff <= cbWritten);
                Assert(cbChunk    >= (uint32_t)cbChunkWritten);
                cbChunk    -= (uint32_t)cbChunkWritten;
            }

            Assert(cbToWrite >= cbWritten);
            cbToWrite      -= cbWritten;
            cbWrittenTotal += cbWritten;
        }

#ifndef RT_OS_L4
        /* Update read pointer. */
        if (pStreamOSS->Out.fMMIO)
            pStreamOSS->old_optr = cntinfo.ptr;
#endif

    } while(0);

    if (RT_SUCCESS(rc))
    {
        if (pcxWritten)
            *pcxWritten = cbWrittenTotal;
    }

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnShutdown}
 */
static DECLCALLBACK(void) drvHostOSSAudioShutdown(PPDMIHOSTAUDIO pInterface)
{
    RT_NOREF(pInterface);
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetStatus}
 */
static DECLCALLBACK(PDMAUDIOBACKENDSTS) drvHostOSSAudioGetStatus(PPDMIHOSTAUDIO pInterface, PDMAUDIODIR enmDir)
{
    AssertPtrReturn(pInterface, PDMAUDIOBACKENDSTS_UNKNOWN);
    RT_NOREF(enmDir);

    return PDMAUDIOBACKENDSTS_RUNNING;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamCreate}
 */
static DECLCALLBACK(int) drvHostOSSAudioStreamCreate(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream,
                                                     PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgReq,    VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgAcq,    VERR_INVALID_POINTER);

    POSSAUDIOSTREAM pStreamOSS = (POSSAUDIOSTREAM)pStream;

    int rc;
    if (pCfgReq->enmDir == PDMAUDIODIR_IN)
        rc = ossCreateStreamIn (pStreamOSS, pCfgReq, pCfgAcq);
    else
        rc = ossCreateStreamOut(pStreamOSS, pCfgReq, pCfgAcq);

    if (RT_SUCCESS(rc))
    {
        pStreamOSS->pCfg = DrvAudioHlpStreamCfgDup(pCfgAcq);
        if (!pStreamOSS->pCfg)
            rc = VERR_NO_MEMORY;
    }

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamDestroy}
 */
static DECLCALLBACK(int) drvHostOSSAudioStreamDestroy(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    POSSAUDIOSTREAM pStreamOSS = (POSSAUDIOSTREAM)pStream;

    if (!pStreamOSS->pCfg) /* Not (yet) configured? Skip. */
        return VINF_SUCCESS;

    int rc;
    if (pStreamOSS->pCfg->enmDir == PDMAUDIODIR_IN)
        rc = ossDestroyStreamIn(pStream);
    else
        rc = ossDestroyStreamOut(pStream);

    if (RT_SUCCESS(rc))
    {
        DrvAudioHlpStreamCfgFree(pStreamOSS->pCfg);
        pStreamOSS->pCfg = NULL;
    }

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamControl}
 */
static DECLCALLBACK(int) drvHostOSSAudioStreamControl(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream,
                                                      PDMAUDIOSTREAMCMD enmStreamCmd)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    POSSAUDIOSTREAM pStreamOSS = (POSSAUDIOSTREAM)pStream;

    if (!pStreamOSS->pCfg) /* Not (yet) configured? Skip. */
        return VINF_SUCCESS;

    int rc;
    if (pStreamOSS->pCfg->enmDir == PDMAUDIODIR_IN)
        rc = ossControlStreamIn(/*pInterface,  pStream, enmStreamCmd*/);
    else
        rc = ossControlStreamOut(pStreamOSS, enmStreamCmd);

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamIterate}
 */
static DECLCALLBACK(int) drvHostOSSAudioStreamIterate(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    LogFlowFuncEnter();

    /* Nothing to do here for OSS. */
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetReadable}
 */
static DECLCALLBACK(uint32_t) drvHostOSSAudioStreamGetReadable(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface, pStream);

    return UINT32_MAX;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetWritable}
 */
static DECLCALLBACK(uint32_t) drvHostOSSAudioStreamGetWritable(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface, pStream);

    return UINT32_MAX;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetStatus}
 */
static DECLCALLBACK(PDMAUDIOSTREAMSTS) drvHostOSSAudioStreamGetStatus(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface, pStream);

    PDMAUDIOSTREAMSTS strmSts =   PDMAUDIOSTREAMSTS_FLAG_INITIALIZED
                              | PDMAUDIOSTREAMSTS_FLAG_ENABLED;
    return strmSts;
}

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) drvHostOSSAudioQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS       pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVHOSTOSSAUDIO pThis   = PDMINS_2_DATA(pDrvIns, PDRVHOSTOSSAUDIO);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIHOSTAUDIO, &pThis->IHostAudio);

    return NULL;
}

/**
 * Constructs an OSS audio driver instance.
 *
 * @copydoc FNPDMDRVCONSTRUCT
 */
static DECLCALLBACK(int) drvHostOSSAudioConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    RT_NOREF(pCfg, fFlags);
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);
    PDRVHOSTOSSAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTOSSAUDIO);
    LogRel(("Audio: Initializing OSS driver\n"));

    /*
     * Init the static parts.
     */
    pThis->pDrvIns                   = pDrvIns;
    /* IBase */
    pDrvIns->IBase.pfnQueryInterface = drvHostOSSAudioQueryInterface;
    /* IHostAudio */
    PDMAUDIO_IHOSTAUDIO_CALLBACKS(drvHostOSSAudio);

    return VINF_SUCCESS;
}

/**
 * Char driver registration record.
 */
const PDMDRVREG g_DrvHostOSSAudio =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "OSSAudio",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "OSS audio host driver",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_AUDIO,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(DRVHOSTOSSAUDIO),
    /* pfnConstruct */
    drvHostOSSAudioConstruct,
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

