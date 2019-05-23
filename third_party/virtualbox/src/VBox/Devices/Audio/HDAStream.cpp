/* $Id: HDAStream.cpp $ */
/** @file
 * HDAStream.cpp - Stream functions for HD Audio.
 */

/*
 * Copyright (C) 2017-2018 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_DEV_HDA
#include <VBox/log.h>

#include <iprt/mem.h>
#include <iprt/semaphore.h>

#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmaudioifs.h>

#include "DrvAudio.h"

#include "DevHDA.h"
#include "HDAStream.h"


#ifdef IN_RING3

/**
 * Creates an HDA stream.
 *
 * @returns IPRT status code.
 * @param   pStream             HDA stream to create.
 * @param   pThis               HDA state to assign the HDA stream to.
 * @param   u8SD                Stream descriptor number to assign.
 */
int hdaR3StreamCreate(PHDASTREAM pStream, PHDASTATE pThis, uint8_t u8SD)
{
    RT_NOREF(pThis);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    pStream->u8SD           = u8SD;
    pStream->pMixSink       = NULL;
    pStream->pHDAState      = pThis;
    pStream->pTimer         = pThis->pTimer[u8SD];
    AssertPtr(pStream->pTimer);

    pStream->State.fInReset = false;
    pStream->State.fRunning = false;
#ifdef HDA_USE_DMA_ACCESS_HANDLER
    RTListInit(&pStream->State.lstDMAHandlers);
#endif

    int rc = RTCritSectInit(&pStream->CritSect);
    AssertRCReturn(rc, rc);

    rc = hdaR3StreamPeriodCreate(&pStream->State.Period);
    AssertRCReturn(rc, rc);

    pStream->State.tsLastUpdateNs = 0;

#ifdef DEBUG
    rc = RTCritSectInit(&pStream->Dbg.CritSect);
    AssertRCReturn(rc, rc);
#endif

    pStream->Dbg.Runtime.fEnabled = pThis->Dbg.fEnabled;

    if (pStream->Dbg.Runtime.fEnabled)
    {
        char szFile[64];

        if (hdaGetDirFromSD(pStream->u8SD) == PDMAUDIODIR_IN)
            RTStrPrintf(szFile, sizeof(szFile), "hdaStreamWriteSD%RU8", pStream->u8SD);
        else
            RTStrPrintf(szFile, sizeof(szFile), "hdaStreamReadSD%RU8", pStream->u8SD);

        char szPath[RTPATH_MAX + 1];
        int rc2 = DrvAudioHlpFileNameGet(szPath, sizeof(szPath), pThis->Dbg.szOutPath, szFile,
                                         0 /* uInst */, PDMAUDIOFILETYPE_WAV, PDMAUDIOFILENAME_FLAG_NONE);
        AssertRC(rc2);
        rc2 = DrvAudioHlpFileCreate(PDMAUDIOFILETYPE_WAV, szPath, PDMAUDIOFILE_FLAG_NONE, &pStream->Dbg.Runtime.pFileStream);
        AssertRC(rc2);

        if (hdaGetDirFromSD(pStream->u8SD) == PDMAUDIODIR_IN)
            RTStrPrintf(szFile, sizeof(szFile), "hdaDMAWriteSD%RU8", pStream->u8SD);
        else
            RTStrPrintf(szFile, sizeof(szFile), "hdaDMAReadSD%RU8", pStream->u8SD);

        rc2 = DrvAudioHlpFileNameGet(szPath, sizeof(szPath), pThis->Dbg.szOutPath, szFile,
                                     0 /* uInst */, PDMAUDIOFILETYPE_WAV, PDMAUDIOFILENAME_FLAG_NONE);
        AssertRC(rc2);

        rc2 = DrvAudioHlpFileCreate(PDMAUDIOFILETYPE_WAV, szPath, PDMAUDIOFILE_FLAG_NONE, &pStream->Dbg.Runtime.pFileDMA);
        AssertRC(rc2);

        /* Delete stale debugging files from a former run. */
        DrvAudioHlpFileDelete(pStream->Dbg.Runtime.pFileStream);
        DrvAudioHlpFileDelete(pStream->Dbg.Runtime.pFileDMA);
    }

    return rc;
}

/**
 * Destroys an HDA stream.
 *
 * @param   pStream             HDA stream to destroy.
 */
void hdaR3StreamDestroy(PHDASTREAM pStream)
{
    AssertPtrReturnVoid(pStream);

    LogFlowFunc(("[SD%RU8]: Destroying ...\n", pStream->u8SD));

    hdaR3StreamMapDestroy(&pStream->State.Mapping);

    int rc2;

#ifdef VBOX_WITH_AUDIO_HDA_ASYNC_IO
    rc2 = hdaR3StreamAsyncIODestroy(pStream);
    AssertRC(rc2);
#endif

    if (RTCritSectIsInitialized(&pStream->CritSect))
    {
        rc2 = RTCritSectDelete(&pStream->CritSect);
        AssertRC(rc2);
    }

    if (pStream->State.pCircBuf)
    {
        RTCircBufDestroy(pStream->State.pCircBuf);
        pStream->State.pCircBuf = NULL;
    }

    hdaR3StreamPeriodDestroy(&pStream->State.Period);

#ifdef DEBUG
    if (RTCritSectIsInitialized(&pStream->Dbg.CritSect))
    {
        rc2 = RTCritSectDelete(&pStream->Dbg.CritSect);
        AssertRC(rc2);
    }
#endif

    if (pStream->Dbg.Runtime.fEnabled)
    {
        DrvAudioHlpFileDestroy(pStream->Dbg.Runtime.pFileStream);
        pStream->Dbg.Runtime.pFileStream = NULL;

        DrvAudioHlpFileDestroy(pStream->Dbg.Runtime.pFileDMA);
        pStream->Dbg.Runtime.pFileDMA = NULL;
    }

    LogFlowFuncLeave();
}

/**
 * Initializes an HDA stream.
 *
 * @returns IPRT status code.
 * @param   pStream             HDA stream to initialize.
 * @param   uSD                 SD (stream descriptor) number to assign the HDA stream to.
 */
int hdaR3StreamInit(PHDASTREAM pStream, uint8_t uSD)
{
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    PHDASTATE pThis = pStream->pHDAState;
    AssertPtr(pThis);

    pStream->u8SD       = uSD;
    pStream->u64BDLBase = RT_MAKE_U64(HDA_STREAM_REG(pThis, BDPL, pStream->u8SD),
                                      HDA_STREAM_REG(pThis, BDPU, pStream->u8SD));
    pStream->u16LVI     = HDA_STREAM_REG(pThis, LVI, pStream->u8SD);
    pStream->u32CBL     = HDA_STREAM_REG(pThis, CBL, pStream->u8SD);
    pStream->u16FIFOS   = HDA_STREAM_REG(pThis, FIFOS, pStream->u8SD) + 1;

    PPDMAUDIOSTREAMCFG pCfg = &pStream->State.Cfg;

    int rc = hdaR3SDFMTToPCMProps(HDA_STREAM_REG(pThis, FMT, uSD), &pCfg->Props);
    if (RT_FAILURE(rc))
    {
        LogRel(("HDA: Warning: Format 0x%x for stream #%RU8 not supported\n", HDA_STREAM_REG(pThis, FMT, uSD), uSD));
        return rc;
    }

    /* Set scheduling hint (if available). */
    if (pThis->u16TimerHz)
        pCfg->Device.uSchedulingHintMs = 1000 /* ms */ / pThis->u16TimerHz;

    /* (Re-)Allocate the stream's internal DMA buffer, based on the PCM  properties we just got above. */
    if (pStream->State.pCircBuf)
    {
        RTCircBufDestroy(pStream->State.pCircBuf);
        pStream->State.pCircBuf = NULL;
    }

    /* By default we allocate an internal buffer of 100ms. */
    rc = RTCircBufCreate(&pStream->State.pCircBuf, DrvAudioHlpMilliToBytes(100 /* ms */, &pCfg->Props)); /** @todo Make this configurable. */
    AssertRCReturn(rc, rc);

    /* Set the stream's direction. */
    pCfg->enmDir = hdaGetDirFromSD(pStream->u8SD);

    /* The the stream's name, based on the direction. */
    switch (pCfg->enmDir)
    {
        case PDMAUDIODIR_IN:
# ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
#  error "Implement me!"
# else
            pCfg->DestSource.Source = PDMAUDIORECSOURCE_LINE;
            pCfg->enmLayout         = PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED;
            RTStrCopy(pCfg->szName, sizeof(pCfg->szName), "Line In");
# endif
            break;

        case PDMAUDIODIR_OUT:
            /* Destination(s) will be set in hdaAddStreamOut(),
             * based on the channels / stream layout. */
            break;

        default:
            rc = VERR_NOT_SUPPORTED;
            break;
    }

    if (   !pStream->u32CBL
        || !pStream->u16LVI
        || !pStream->u64BDLBase
        || !pStream->u16FIFOS)
    {
        return VINF_SUCCESS;
    }

    /* Set the stream's frame size. */
    pStream->State.cbFrameSize = pCfg->Props.cChannels * pCfg->Props.cBytes;
    LogFunc(("[SD%RU8] cChannels=%RU8, cBytes=%RU8 -> cbFrameSize=%RU32\n",
             pStream->u8SD, pCfg->Props.cChannels, pCfg->Props.cBytes, pStream->State.cbFrameSize));
    Assert(pStream->State.cbFrameSize); /* Frame size must not be 0. */

    /*
     * Initialize the stream mapping in any case, regardless if
     * we support surround audio or not. This is needed to handle
     * the supported channels within a single audio stream, e.g. mono/stereo.
     *
     * In other words, the stream mapping *always* knows the real
     * number of channels in a single audio stream.
     */
    rc = hdaR3StreamMapInit(&pStream->State.Mapping, &pCfg->Props);
    AssertRCReturn(rc, rc);

    LogFunc(("[SD%RU8] DMA @ 0x%x (%RU32 bytes), LVI=%RU16, FIFOS=%RU16, Hz=%RU32, rc=%Rrc\n",
             pStream->u8SD, pStream->u64BDLBase, pStream->u32CBL, pStream->u16LVI, pStream->u16FIFOS,
             pStream->State.Cfg.Props.uHz, rc));

    /* Make sure that mandatory parameters are set up correctly. */
    AssertStmt(pStream->u32CBL %  pStream->State.cbFrameSize == 0, rc = VERR_INVALID_PARAMETER);
    AssertStmt(pStream->u16LVI >= 1,                               rc = VERR_INVALID_PARAMETER);

    if (RT_SUCCESS(rc))
    {
        /* Make sure that the chosen Hz rate dividable by the stream's rate. */
        if (pStream->State.Cfg.Props.uHz % pThis->u16TimerHz != 0)
            LogRel(("HDA: Device timer (%RU32) does not fit to stream #RU8 timing (%RU32)\n",
                    pThis->u16TimerHz, pStream->State.Cfg.Props.uHz));

        /* Figure out how many transfer fragments we're going to use for this stream. */
        /** @todo Use a more dynamic fragment size? */
        Assert(pStream->u16LVI <= UINT8_MAX - 1);
        uint8_t cFragments = pStream->u16LVI + 1;
        if (cFragments <= 1)
            cFragments = 2; /* At least two fragments (BDLEs) must be present. */

        /*
         * Handle the stream's position adjustment.
         */
        uint32_t cfPosAdjust = 0;

        LogFunc(("[SD%RU8] fPosAdjustEnabled=%RTbool, cPosAdjustFrames=%RU16\n",
                 pStream->u8SD, pThis->fPosAdjustEnabled, pThis->cPosAdjustFrames));

        if (pThis->fPosAdjustEnabled) /* Is the position adjustment enabled at all? */
        {
            HDABDLE BDLE;
            RT_ZERO(BDLE);

            int rc2 = hdaR3BDLEFetch(pThis, &BDLE, pStream->u64BDLBase, 0 /* Entry */);
            AssertRC(rc2);

            /* Note: Do *not* check if this BDLE aligns to the stream's frame size.
             *       It can happen that this isn't the case on some guests, e.g.
             *       on Windows with a 5.1 speaker setup.
             *
             *       The only thing which counts is that the stream's CBL value
             *       properly aligns to the stream's frame size.
             */

            /* If no custom set position adjustment is set, apply some
             * simple heuristics to detect the appropriate position adjustment. */
            if (   !pThis->cPosAdjustFrames
            /* Position adjustmenet buffer *must* have the IOC bit set! */
                && hdaR3BDLENeedsInterrupt(&BDLE))
            {
                /** @todo Implement / use a (dynamic) table once this gets more complicated. */
#ifdef VBOX_WITH_INTEL_HDA
                /* Intel ICH / PCH: 1 frame. */
                if (BDLE.Desc.u32BufSize == 1 * pStream->State.cbFrameSize)
                {
                    cfPosAdjust = 1;
                }
                /* Intel Baytrail / Braswell: 32 frames. */
                else if (BDLE.Desc.u32BufSize == 32 * pStream->State.cbFrameSize)
                {
                    cfPosAdjust = 32;
                }
#endif
            }
            else /* Go with the set default. */
                cfPosAdjust = pThis->cPosAdjustFrames;

            if (cfPosAdjust)
            {
                /* Also adjust the number of fragments, as the position adjustment buffer
                 * does not count as an own fragment as such.
                 *
                 * This e.g. can happen on (newer) Ubuntu guests which use
                 * 4 (IOC) + 4408 (IOC) + 4408 (IOC) + 4408 (IOC) + 4404 (= 17632) bytes,
                 * where the first buffer (4) is used as position adjustment.
                 *
                 * Only skip a fragment if the whole buffer fragment is used for
                 * position adjustment.
                 */
                if (   (cfPosAdjust * pStream->State.cbFrameSize) == BDLE.Desc.u32BufSize
                    && cFragments)
                {
                    cFragments--;
                }

                /* Initialize position adjustment counter. */
                pStream->State.cPosAdjustFramesLeft = cfPosAdjust;
                LogRel2(("HDA: Position adjustment for stream #%RU8 active (%RU32 frames)\n", pStream->u8SD, cfPosAdjust));
            }
        }

        LogFunc(("[SD%RU8] cfPosAdjust=%RU32, cFragments=%RU8\n", pStream->u8SD, cfPosAdjust, cFragments));

        /*
         * Set up data transfer transfer stuff.
         */

        /* Calculate the fragment size the guest OS expects interrupt delivery at. */
        pStream->State.cbTransferSize = pStream->u32CBL / cFragments;
        Assert(pStream->State.cbTransferSize);
        Assert(pStream->State.cbTransferSize % pStream->State.cbFrameSize == 0);

        /* Calculate the bytes we need to transfer to / from the stream's DMA per iteration.
         * This is bound to the device's Hz rate and thus to the (virtual) timing the device expects. */
        pStream->State.cbTransferChunk = (pStream->State.Cfg.Props.uHz / pThis->u16TimerHz) * pStream->State.cbFrameSize;
        Assert(pStream->State.cbTransferChunk);
        Assert(pStream->State.cbTransferChunk % pStream->State.cbFrameSize == 0);

        /* Make sure that the transfer chunk does not exceed the overall transfer size. */
        if (pStream->State.cbTransferChunk > pStream->State.cbTransferSize)
            pStream->State.cbTransferChunk = pStream->State.cbTransferSize;

        pStream->State.cbTransferProcessed        = 0;
        pStream->State.cTransferPendingInterrupts = 0;
        pStream->State.cbDMALeft                  = 0;
        pStream->State.tsLastUpdateNs             = 0;

        const uint64_t cTicksPerHz = TMTimerGetFreq(pStream->pTimer) / pThis->u16TimerHz;

        /* Calculate the timer ticks per byte for this stream. */
        pStream->State.cTicksPerByte = cTicksPerHz / pStream->State.cbTransferChunk;
        Assert(pStream->State.cTicksPerByte);

        /* Calculate timer ticks per transfer. */
        pStream->State.cTransferTicks     = pStream->State.cbTransferChunk * pStream->State.cTicksPerByte;
        Assert(pStream->State.cTransferTicks);

        /* Initialize the transfer timestamps. */
        pStream->State.tsTransferLast     = 0;
        pStream->State.tsTransferNext     = 0;

        LogFunc(("[SD%RU8] Timer %uHz (%RU64 ticks per Hz), cTicksPerByte=%RU64, cbTransferChunk=%RU32, cTransferTicks=%RU64, " \
                 "cbTransferSize=%RU32\n",
                 pStream->u8SD, pThis->u16TimerHz, cTicksPerHz, pStream->State.cTicksPerByte,
                 pStream->State.cbTransferChunk, pStream->State.cTransferTicks, pStream->State.cbTransferSize));

        /* Make sure to also update the stream's DMA counter (based on its current LPIB value). */
        hdaR3StreamSetPosition(pStream, HDA_STREAM_REG(pThis, LPIB, pStream->u8SD));

#ifdef LOG_ENABLED
        hdaR3BDLEDumpAll(pThis, pStream->u64BDLBase, pStream->u16LVI + 1);
#endif
    }

    if (RT_FAILURE(rc))
        LogRel(("HDA: Initializing stream #%RU8 failed with %Rrc\n", pStream->u8SD, rc));

    return rc;
}

/**
 * Resets an HDA stream.
 *
 * @param   pThis               HDA state.
 * @param   pStream             HDA stream to reset.
 * @param   uSD                 Stream descriptor (SD) number to use for this stream.
 */
void hdaR3StreamReset(PHDASTATE pThis, PHDASTREAM pStream, uint8_t uSD)
{
    AssertPtrReturnVoid(pThis);
    AssertPtrReturnVoid(pStream);
    AssertReturnVoid(uSD < HDA_MAX_STREAMS);

# ifdef VBOX_STRICT
    AssertReleaseMsg(!pStream->State.fRunning, ("[SD%RU8] Cannot reset stream while in running state\n", uSD));
# endif

    LogFunc(("[SD%RU8]: Reset\n", uSD));

    /*
     * Set reset state.
     */
    Assert(ASMAtomicReadBool(&pStream->State.fInReset) == false); /* No nested calls. */
    ASMAtomicXchgBool(&pStream->State.fInReset, true);

    /*
     * Second, initialize the registers.
     */
    HDA_STREAM_REG(pThis, STS,   uSD) = HDA_SDSTS_FIFORDY;
    /* According to the ICH6 datasheet, 0x40000 is the default value for stream descriptor register 23:20
     * bits are reserved for stream number 18.2.33, resets SDnCTL except SRST bit. */
    HDA_STREAM_REG(pThis, CTL,   uSD) = 0x40000 | (HDA_STREAM_REG(pThis, CTL, uSD) & HDA_SDCTL_SRST);
    /* ICH6 defines default values (120 bytes for input and 192 bytes for output descriptors) of FIFO size. 18.2.39. */
    HDA_STREAM_REG(pThis, FIFOS, uSD) = hdaGetDirFromSD(uSD) == PDMAUDIODIR_IN ? HDA_SDIFIFO_120B : HDA_SDOFIFO_192B;
    /* See 18.2.38: Always defaults to 0x4 (32 bytes). */
    HDA_STREAM_REG(pThis, FIFOW, uSD) = HDA_SDFIFOW_32B;
    HDA_STREAM_REG(pThis, LPIB,  uSD) = 0;
    HDA_STREAM_REG(pThis, CBL,   uSD) = 0;
    HDA_STREAM_REG(pThis, LVI,   uSD) = 0;
    HDA_STREAM_REG(pThis, FMT,   uSD) = 0;
    HDA_STREAM_REG(pThis, BDPU,  uSD) = 0;
    HDA_STREAM_REG(pThis, BDPL,  uSD) = 0;

#ifdef HDA_USE_DMA_ACCESS_HANDLER
    hdaR3StreamUnregisterDMAHandlers(pThis, pStream);
#endif

    /* Assign the default mixer sink to the stream. */
    pStream->pMixSink             = hdaR3GetDefaultSink(pThis, uSD);

    pStream->State.tsTransferLast = 0;
    pStream->State.tsTransferNext = 0;

    RT_ZERO(pStream->State.BDLE);
    pStream->State.uCurBDLE = 0;

    if (pStream->State.pCircBuf)
        RTCircBufReset(pStream->State.pCircBuf);

    /* Reset stream map. */
    hdaR3StreamMapReset(&pStream->State.Mapping);

    /* (Re-)initialize the stream with current values. */
    int rc2 = hdaR3StreamInit(pStream, uSD);
    AssertRC(rc2);

    /* Reset the stream's period. */
    hdaR3StreamPeriodReset(&pStream->State.Period);

#ifdef DEBUG
    pStream->Dbg.cReadsTotal      = 0;
    pStream->Dbg.cbReadTotal      = 0;
    pStream->Dbg.tsLastReadNs     = 0;
    pStream->Dbg.cWritesTotal     = 0;
    pStream->Dbg.cbWrittenTotal   = 0;
    pStream->Dbg.cWritesHz        = 0;
    pStream->Dbg.cbWrittenHz      = 0;
    pStream->Dbg.tsWriteSlotBegin = 0;
#endif

    /* Report that we're done resetting this stream. */
    HDA_STREAM_REG(pThis, CTL,   uSD) = 0;

    LogFunc(("[SD%RU8] Reset\n", uSD));

    /* Exit reset mode. */
    ASMAtomicXchgBool(&pStream->State.fInReset, false);
}

/**
 * Enables or disables an HDA audio stream.
 *
 * @returns IPRT status code.
 * @param   pStream             HDA stream to enable or disable.
 * @param   fEnable             Whether to enable or disble the stream.
 */
int hdaR3StreamEnable(PHDASTREAM pStream, bool fEnable)
{
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    LogFunc(("[SD%RU8]: fEnable=%RTbool, pMixSink=%p\n", pStream->u8SD, fEnable, pStream->pMixSink));

    int rc = VINF_SUCCESS;

    AUDMIXSINKCMD enmCmd = fEnable
                         ? AUDMIXSINKCMD_ENABLE : AUDMIXSINKCMD_DISABLE;

    /* First, enable or disable the stream and the stream's sink, if any. */
    if (   pStream->pMixSink
        && pStream->pMixSink->pMixSink)
        rc = AudioMixerSinkCtl(pStream->pMixSink->pMixSink, enmCmd);

    if (   RT_SUCCESS(rc)
        && fEnable
        && pStream->Dbg.Runtime.fEnabled)
    {
        Assert(DrvAudioHlpPCMPropsAreValid(&pStream->State.Cfg.Props));

        if (fEnable)
        {
            if (!DrvAudioHlpFileIsOpen(pStream->Dbg.Runtime.pFileStream))
            {
                int rc2 = DrvAudioHlpFileOpen(pStream->Dbg.Runtime.pFileStream, PDMAUDIOFILE_DEFAULT_OPEN_FLAGS,
                                              &pStream->State.Cfg.Props);
                AssertRC(rc2);
            }

            if (!DrvAudioHlpFileIsOpen(pStream->Dbg.Runtime.pFileDMA))
            {
                int rc2 = DrvAudioHlpFileOpen(pStream->Dbg.Runtime.pFileDMA, PDMAUDIOFILE_DEFAULT_OPEN_FLAGS,
                                              &pStream->State.Cfg.Props);
                AssertRC(rc2);
            }
        }
    }

    if (RT_SUCCESS(rc))
    {
        pStream->State.fRunning = fEnable;
    }

    LogFunc(("[SD%RU8] rc=%Rrc\n", pStream->u8SD, rc));
    return rc;
}

uint32_t hdaR3StreamGetPosition(PHDASTATE pThis, PHDASTREAM pStream)
{
    return HDA_STREAM_REG(pThis, LPIB, pStream->u8SD);
}

/**
 * Updates an HDA stream's current read or write buffer position (depending on the stream type) by
 * updating its associated LPIB register and DMA position buffer (if enabled).
 *
 * @param   pStream             HDA stream to update read / write position for.
 * @param   u32LPIB             Absolute position (in bytes) to set current read / write position to.
 */
void hdaR3StreamSetPosition(PHDASTREAM pStream, uint32_t u32LPIB)
{
    AssertPtrReturnVoid(pStream);

    Log3Func(("[SD%RU8] LPIB=%RU32 (DMA Position Buffer Enabled: %RTbool)\n",
              pStream->u8SD, u32LPIB, pStream->pHDAState->fDMAPosition));

    /* Update LPIB in any case. */
    HDA_STREAM_REG(pStream->pHDAState, LPIB, pStream->u8SD) = u32LPIB;

    /* Do we need to tell the current DMA position? */
    if (pStream->pHDAState->fDMAPosition)
    {
        int rc2 = PDMDevHlpPCIPhysWrite(pStream->pHDAState->CTX_SUFF(pDevIns),
                                        pStream->pHDAState->u64DPBase + (pStream->u8SD * 2 * sizeof(uint32_t)),
                                        (void *)&u32LPIB, sizeof(uint32_t));
        AssertRC(rc2);
    }
}

/**
 * Retrieves the available size of (buffered) audio data (in bytes) of a given HDA stream.
 *
 * @returns Available data (in bytes).
 * @param   pStream             HDA stream to retrieve size for.
 */
uint32_t hdaR3StreamGetUsed(PHDASTREAM pStream)
{
    AssertPtrReturn(pStream, 0);

    if (!pStream->State.pCircBuf)
        return 0;

    return (uint32_t)RTCircBufUsed(pStream->State.pCircBuf);
}

/**
 * Retrieves the free size of audio data (in bytes) of a given HDA stream.
 *
 * @returns Free data (in bytes).
 * @param   pStream             HDA stream to retrieve size for.
 */
uint32_t hdaR3StreamGetFree(PHDASTREAM pStream)
{
    AssertPtrReturn(pStream, 0);

    if (!pStream->State.pCircBuf)
        return 0;

    return (uint32_t)RTCircBufFree(pStream->State.pCircBuf);
}

/**
 * Returns whether a next transfer for a given stream is scheduled or not.
 * This takes pending stream interrupts into account as well as the next scheduled
 * transfer timestamp.
 *
 * @returns True if a next transfer is scheduled, false if not.
 * @param   pStream             HDA stream to retrieve schedule status for.
 */
bool hdaR3StreamTransferIsScheduled(PHDASTREAM pStream)
{
    if (pStream)
    {
        AssertPtrReturn(pStream->pHDAState, false);

        if (pStream->State.fRunning)
        {
            if (pStream->State.cTransferPendingInterrupts)
            {
                Log3Func(("[SD%RU8] Scheduled (%RU8 IRQs pending)\n", pStream->u8SD, pStream->State.cTransferPendingInterrupts));
                return true;
            }

            const uint64_t tsNow = TMTimerGet(pStream->pTimer);
            if (pStream->State.tsTransferNext > tsNow)
            {
                Log3Func(("[SD%RU8] Scheduled in %RU64\n", pStream->u8SD, pStream->State.tsTransferNext - tsNow));
                return true;
            }
        }
    }
    return false;
}

/**
 * Returns the (virtual) clock timestamp of the next transfer, if any.
 * Will return 0 if no new transfer is scheduled.
 *
 * @returns The (virtual) clock timestamp of the next transfer.
 * @param   pStream             HDA stream to retrieve timestamp for.
 */
uint64_t hdaR3StreamTransferGetNext(PHDASTREAM pStream)
{
    return pStream->State.tsTransferNext;
}

/**
 * Writes audio data from a mixer sink into an HDA stream's DMA buffer.
 *
 * @returns IPRT status code.
 * @param   pStream             HDA stream to write to.
 * @param   pvBuf               Data buffer to write.
 *                              If NULL, silence will be written.
 * @param   cbBuf               Number of bytes of data buffer to write.
 * @param   pcbWritten          Number of bytes written. Optional.
 */
int hdaR3StreamWrite(PHDASTREAM pStream, const void *pvBuf, uint32_t cbBuf, uint32_t *pcbWritten)
{
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);
    /* pvBuf is optional. */
    AssertReturn(cbBuf,      VERR_INVALID_PARAMETER);
    /* pcbWritten is optional. */

    PRTCIRCBUF pCircBuf = pStream->State.pCircBuf;
    AssertPtr(pCircBuf);

    int rc = VINF_SUCCESS;

    uint32_t cbWrittenTotal = 0;
    uint32_t cbLeft         = RT_MIN(cbBuf, (uint32_t)RTCircBufFree(pCircBuf));

    while (cbLeft)
    {
        void *pvDst;
        size_t cbDst;

        RTCircBufAcquireWriteBlock(pCircBuf, cbLeft, &pvDst, &cbDst);

        if (cbDst)
        {
            if (pvBuf)
            {
                memcpy(pvDst, (uint8_t *)pvBuf + cbWrittenTotal, cbDst);
            }
            else /* Send silence. */
            {
                /** @todo Use a sample spec for "silence" based on the PCM parameters.
                 *        For now we ASSUME that silence equals NULLing the data. */
                RT_BZERO(pvDst, cbDst);
            }

            if (pStream->Dbg.Runtime.fEnabled)
                DrvAudioHlpFileWrite(pStream->Dbg.Runtime.pFileStream, pvDst, cbDst, 0 /* fFlags */);
        }

        RTCircBufReleaseWriteBlock(pCircBuf, cbDst);

        if (RT_FAILURE(rc))
            break;

        Assert(cbLeft  >= (uint32_t)cbDst);
        cbLeft         -= (uint32_t)cbDst;

        cbWrittenTotal += (uint32_t)cbDst;
    }

    Log3Func(("cbWrittenTotal=%RU32\n", cbWrittenTotal));

    if (pcbWritten)
        *pcbWritten = cbWrittenTotal;

    return rc;
}


/**
 * Reads audio data from an HDA stream's DMA buffer and writes into a specified mixer sink.
 *
 * @returns IPRT status code.
 * @param   pStream             HDA stream to read audio data from.
 * @param   cbToRead            Number of bytes to read.
 * @param   pcbRead             Number of bytes read. Optional.
 */
int hdaR3StreamRead(PHDASTREAM pStream, uint32_t cbToRead, uint32_t *pcbRead)
{
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);
    AssertReturn(cbToRead,   VERR_INVALID_PARAMETER);
    /* pcbWritten is optional. */

    PHDAMIXERSINK pSink = pStream->pMixSink;
    if (!pSink)
    {
        AssertMsgFailed(("[SD%RU8]: Can't read from a stream with no sink attached\n", pStream->u8SD));

        if (pcbRead)
            *pcbRead = 0;
        return VINF_SUCCESS;
    }

    PRTCIRCBUF pCircBuf = pStream->State.pCircBuf;
    AssertPtr(pCircBuf);

    int rc = VINF_SUCCESS;

    uint32_t cbReadTotal = 0;
    uint32_t cbLeft      = RT_MIN(cbToRead, (uint32_t)RTCircBufUsed(pCircBuf));

    while (cbLeft)
    {
        void *pvSrc;
        size_t cbSrc;

        uint32_t cbWritten = 0;

        RTCircBufAcquireReadBlock(pCircBuf, cbLeft, &pvSrc, &cbSrc);

        if (cbSrc)
        {
            if (pStream->Dbg.Runtime.fEnabled)
                DrvAudioHlpFileWrite(pStream->Dbg.Runtime.pFileStream, pvSrc, cbSrc, 0 /* fFlags */);

            rc = AudioMixerSinkWrite(pSink->pMixSink, AUDMIXOP_COPY, pvSrc, (uint32_t)cbSrc, &cbWritten);
            AssertRC(rc);

            Assert(cbSrc >= cbWritten);
            Log2Func(("[SD%RU8]: %RU32/%zu bytes read\n", pStream->u8SD, cbWritten, cbSrc));
        }

        RTCircBufReleaseReadBlock(pCircBuf, cbWritten);

        if (RT_FAILURE(rc))
            break;

        Assert(cbLeft  >= cbWritten);
        cbLeft         -= cbWritten;

        cbReadTotal    += cbWritten;
    }

    if (pcbRead)
        *pcbRead = cbReadTotal;

    return rc;
}

/**
 * Transfers data of an HDA stream according to its usage (input / output).
 *
 * For an SDO (output) stream this means reading DMA data from the device to
 * the HDA stream's internal FIFO buffer.
 *
 * For an SDI (input) stream this is reading audio data from the HDA stream's
 * internal FIFO buffer and writing it as DMA data to the device.
 *
 * @returns IPRT status code.
 * @param   pStream             HDA stream to update.
 * @param   cbToProcessMax      How much data (in bytes) to process as maximum.
 */
int hdaR3StreamTransfer(PHDASTREAM pStream, uint32_t cbToProcessMax)
{
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    hdaR3StreamLock(pStream);

    PHDASTATE pThis = pStream->pHDAState;
    AssertPtr(pThis);

    PHDASTREAMPERIOD pPeriod = &pStream->State.Period;
    if (!hdaR3StreamPeriodLock(pPeriod))
        return VERR_ACCESS_DENIED;

    bool fProceed = true;

    /* Stream not running? */
    if (!pStream->State.fRunning)
    {
        Log3Func(("[SD%RU8] Not running\n", pStream->u8SD));
        fProceed = false;
    }
    else if (HDA_STREAM_REG(pThis, STS, pStream->u8SD) & HDA_SDSTS_BCIS)
    {
        Log3Func(("[SD%RU8] BCIS bit set\n", pStream->u8SD));
        fProceed = false;
    }

    if (!fProceed)
    {
        hdaR3StreamPeriodUnlock(pPeriod);
        hdaR3StreamUnlock(pStream);
        return VINF_SUCCESS;
    }

    const uint64_t tsNow = TMTimerGet(pStream->pTimer);

    if (!pStream->State.tsTransferLast)
        pStream->State.tsTransferLast = tsNow;

#ifdef DEBUG
    const int64_t iTimerDelta = tsNow - pStream->State.tsTransferLast;
    Log3Func(("[SD%RU8] Time now=%RU64, last=%RU64 -> %RI64 ticks delta\n",
              pStream->u8SD, tsNow, pStream->State.tsTransferLast, iTimerDelta));
#endif

    pStream->State.tsTransferLast = tsNow;

    /* Sanity checks. */
    Assert(pStream->u8SD < HDA_MAX_STREAMS);
    Assert(pStream->u64BDLBase);
    Assert(pStream->u32CBL);
    Assert(pStream->u16FIFOS);

    /* State sanity checks. */
    Assert(ASMAtomicReadBool(&pStream->State.fInReset) == false);

    int rc = VINF_SUCCESS;

    /* Fetch first / next BDL entry. */
    PHDABDLE pBDLE = &pStream->State.BDLE;
    if (hdaR3BDLEIsComplete(pBDLE))
    {
        rc = hdaR3BDLEFetch(pThis, pBDLE, pStream->u64BDLBase, pStream->State.uCurBDLE);
        AssertRC(rc);
    }

    uint32_t cbToProcess = RT_MIN(pStream->State.cbTransferSize - pStream->State.cbTransferProcessed,
                                  pStream->State.cbTransferChunk);

    Log3Func(("[SD%RU8] cbToProcess=%RU32, cbToProcessMax=%RU32\n", pStream->u8SD, cbToProcess, cbToProcessMax));

    if (cbToProcess > cbToProcessMax)
    {
        LogFunc(("[SD%RU8] Limiting transfer (cbToProcess=%RU32, cbToProcessMax=%RU32)\n",
                 pStream->u8SD, cbToProcess, cbToProcessMax));

        /* Never process more than a stream currently can handle. */
        cbToProcess = cbToProcessMax;
    }

    uint32_t cbProcessed = 0;
    uint32_t cbLeft      = cbToProcess;

    uint8_t abChunk[HDA_FIFO_MAX + 1];
    while (cbLeft)
    {
        /* Limit the chunk to the stream's FIFO size and what's left to process. */
        uint32_t cbChunk = RT_MIN(cbLeft, pStream->u16FIFOS);

        /* Limit the chunk to the remaining data of the current BDLE. */
        cbChunk = RT_MIN(cbChunk, pBDLE->Desc.u32BufSize - pBDLE->State.u32BufOff);

        /* If there are position adjustment frames left to be processed,
         * make sure that we process them first as a whole. */
        if (pStream->State.cPosAdjustFramesLeft)
            cbChunk = RT_MIN(cbChunk, uint32_t(pStream->State.cPosAdjustFramesLeft * pStream->State.cbFrameSize));

        Log3Func(("[SD%RU8] cbChunk=%RU32, cPosAdjustFramesLeft=%RU16\n",
                  pStream->u8SD, cbChunk, pStream->State.cPosAdjustFramesLeft));

        if (!cbChunk)
            break;

        uint32_t   cbDMA    = 0;
        PRTCIRCBUF pCircBuf = pStream->State.pCircBuf;

        if (hdaGetDirFromSD(pStream->u8SD) == PDMAUDIODIR_IN) /* Input (SDI). */
        {
            STAM_PROFILE_START(&pThis->StatIn, a);

            uint32_t cbDMAWritten = 0;
            uint32_t cbDMAToWrite = cbChunk;

            /** @todo Do we need interleaving streams support here as well?
             *        Never saw anything else besides mono/stereo mics (yet). */
            while (cbDMAToWrite)
            {
                void *pvBuf; size_t cbBuf;
                RTCircBufAcquireReadBlock(pCircBuf, cbDMAToWrite, &pvBuf, &cbBuf);

                if (   !cbBuf
                    && !RTCircBufUsed(pCircBuf))
                    break;

                memcpy(abChunk + cbDMAWritten, pvBuf, cbBuf);

                RTCircBufReleaseReadBlock(pCircBuf, cbBuf);

                Assert(cbDMAToWrite >= cbBuf);
                cbDMAToWrite -= (uint32_t)cbBuf;
                cbDMAWritten += (uint32_t)cbBuf;
                Assert(cbDMAWritten <= cbChunk);
            }

            if (cbDMAToWrite)
            {
                LogRel2(("HDA: FIFO underflow for stream #%RU8 (%RU32 bytes outstanding)\n", pStream->u8SD, cbDMAToWrite));

                Assert(cbChunk == cbDMAWritten + cbDMAToWrite);
                memset((uint8_t *)abChunk + cbDMAWritten, 0, cbDMAToWrite);
                cbDMAWritten = cbChunk;
            }

            rc = hdaR3DMAWrite(pThis, pStream, abChunk, cbDMAWritten, &cbDMA /* pcbWritten */);
            if (RT_FAILURE(rc))
                LogRel(("HDA: Writing to stream #%RU8 DMA failed with %Rrc\n", pStream->u8SD, rc));

            STAM_PROFILE_STOP(&pThis->StatIn, a);
        }
        else if (hdaGetDirFromSD(pStream->u8SD) == PDMAUDIODIR_OUT) /* Output (SDO). */
        {
            STAM_PROFILE_START(&pThis->StatOut, a);

            rc = hdaR3DMARead(pThis, pStream, abChunk, cbChunk, &cbDMA /* pcbRead */);
            if (RT_SUCCESS(rc))
            {
                const uint32_t cbDMAFree = (uint32_t)RTCircBufFree(pCircBuf);
                Assert(cbDMAFree >= cbDMA); /* This must always hold. */

#ifndef VBOX_WITH_HDA_AUDIO_INTERLEAVING_STREAMS_SUPPORT
                /*
                 * Most guests don't use different stream frame sizes than
                 * the default one, so save a bit of CPU time and don't go into
                 * the frame extraction code below.
                 *
                 * Only macOS guests need the frame extraction branch below at the moment AFAIK.
                 */
                if (pStream->State.cbFrameSize == HDA_FRAME_SIZE)
                {
                    uint32_t cbDMARead = 0;
                    uint32_t cbDMALeft = RT_MIN(cbDMA, cbDMAFree);

                    while (cbDMALeft)
                    {
                        void *pvBuf; size_t cbBuf;
                        RTCircBufAcquireWriteBlock(pCircBuf, cbDMALeft, &pvBuf, &cbBuf);

                        if (cbBuf)
                        {
                            memcpy(pvBuf, abChunk + cbDMARead, cbBuf);
                            cbDMARead += (uint32_t)cbBuf;
                            cbDMALeft -= (uint32_t)cbBuf;
                        }

                        RTCircBufReleaseWriteBlock(pCircBuf, cbBuf);
                    }
                }
                else
                {
                    /*
                     * The following code extracts the required audio stream (channel) data
                     * of non-interleaved *and* interleaved audio streams.
                     *
                     * We by default only support 2 channels with 16-bit samples (HDA_FRAME_SIZE),
                     * but an HDA audio stream can have interleaved audio data of multiple audio
                     * channels in such a single stream ("AA,AA,AA vs. AA,BB,AA,BB").
                     *
                     * So take this into account by just handling the first channel in such a stream ("A")
                     * and just discard the other channel's data.
                     *
                     */
                    /** @todo Optimize this stuff -- copying only one frame a time is expensive. */
                    uint32_t cbDMARead = pStream->State.cbDMALeft ? pStream->State.cbFrameSize - pStream->State.cbDMALeft : 0;
                    uint32_t cbDMALeft = RT_MIN(cbDMA, cbDMAFree);

                    while (cbDMALeft >= pStream->State.cbFrameSize)
                    {
                        void *pvBuf; size_t cbBuf;
                        RTCircBufAcquireWriteBlock(pCircBuf, HDA_FRAME_SIZE, &pvBuf, &cbBuf);

                        AssertBreak(cbDMARead <= sizeof(abChunk));

                        if (cbBuf)
                            memcpy(pvBuf, abChunk + cbDMARead, cbBuf);

                        RTCircBufReleaseWriteBlock(pCircBuf, cbBuf);

                        Assert(cbDMALeft >= pStream->State.cbFrameSize);
                        cbDMALeft -= pStream->State.cbFrameSize;
                        cbDMARead += pStream->State.cbFrameSize;
                    }

                    pStream->State.cbDMALeft = cbDMALeft;
                    Assert(pStream->State.cbDMALeft < pStream->State.cbFrameSize);
                }
#else
                /** @todo This needs making use of HDAStreamMap + HDAStreamChannel. */
# error "Implement reading interleaving streams support here."
#endif
            }
            else
                LogRel(("HDA: Reading from stream #%RU8 DMA failed with %Rrc\n", pStream->u8SD, rc));

            STAM_PROFILE_STOP(&pThis->StatOut, a);
        }

        else /** @todo Handle duplex streams? */
            AssertFailed();

        if (cbDMA)
        {
            /* We always increment the position of DMA buffer counter because we're always reading
             * into an intermediate DMA buffer. */
            pBDLE->State.u32BufOff += (uint32_t)cbDMA;
            Assert(pBDLE->State.u32BufOff <= pBDLE->Desc.u32BufSize);

            /* Are we done doing the position adjustment?
             * Only then do the transfer accounting .*/
            if (pStream->State.cPosAdjustFramesLeft == 0)
            {
                Assert(cbLeft >= cbDMA);
                cbLeft        -= cbDMA;

                cbProcessed   += cbDMA;
            }

            /*
             * Update the stream's current position.
             * Do this as accurate and close to the actual data transfer as possible.
             * All guetsts rely on this, depending on the mechanism they use (LPIB register or DMA counters).
             */
            uint32_t cbStreamPos = hdaR3StreamGetPosition(pThis, pStream);
            if (cbStreamPos == pStream->u32CBL)
                cbStreamPos = 0;

            hdaR3StreamSetPosition(pStream, cbStreamPos + cbDMA);
        }

        if (hdaR3BDLEIsComplete(pBDLE))
        {
            Log3Func(("[SD%RU8] Complete: %R[bdle]\n", pStream->u8SD, pBDLE));

                /* Does the current BDLE require an interrupt to be sent? */
            if (   hdaR3BDLENeedsInterrupt(pBDLE)
                /* Are we done doing the position adjustment?
                 * It can happen that a BDLE which is handled while doing the
                 * position adjustment requires an interrupt on completion (IOC) being set.
                 *
                 * In such a case we need to skip such an interrupt and just move on. */
                && pStream->State.cPosAdjustFramesLeft == 0)
            {
                /* If the IOCE ("Interrupt On Completion Enable") bit of the SDCTL register is set
                 * we need to generate an interrupt.
                 */
                if (HDA_STREAM_REG(pThis, CTL, pStream->u8SD) & HDA_SDCTL_IOCE)
                {
                    pStream->State.cTransferPendingInterrupts++;

                    AssertMsg(pStream->State.cTransferPendingInterrupts <= 32,
                              ("Too many pending interrupts (%RU8) for stream #%RU8\n",
                               pStream->State.cTransferPendingInterrupts, pStream->u8SD));
                }
            }

            if (pStream->State.uCurBDLE == pStream->u16LVI)
            {
                pStream->State.uCurBDLE = 0;
            }
            else
                pStream->State.uCurBDLE++;

            /* Fetch the next BDLE entry. */
            hdaR3BDLEFetch(pThis, pBDLE, pStream->u64BDLBase, pStream->State.uCurBDLE);
        }

        /* Do the position adjustment accounting. */
        pStream->State.cPosAdjustFramesLeft -= RT_MIN(pStream->State.cPosAdjustFramesLeft, cbDMA / pStream->State.cbFrameSize);

        if (RT_FAILURE(rc))
            break;
    }

    Log3Func(("[SD%RU8] cbToProcess=%RU32, cbProcessed=%RU32, cbLeft=%RU32, %R[bdle], rc=%Rrc\n",
              pStream->u8SD, cbToProcess, cbProcessed, cbLeft, pBDLE, rc));

    /* Sanity. */
    Assert(cbProcessed == cbToProcess);
    Assert(cbLeft      == 0);

    /* Only do the data accounting if we don't have to do any position
     * adjustment anymore. */
    if (pStream->State.cPosAdjustFramesLeft == 0)
    {
        hdaR3StreamPeriodInc(pPeriod, RT_MIN(cbProcessed / pStream->State.cbFrameSize,
                                             hdaR3StreamPeriodGetRemainingFrames(pPeriod)));

        pStream->State.cbTransferProcessed += cbProcessed;
    }

    /* Make sure that we never report more stuff processed than initially announced. */
    if (pStream->State.cbTransferProcessed > pStream->State.cbTransferSize)
        pStream->State.cbTransferProcessed = pStream->State.cbTransferSize;

    uint32_t cbTransferLeft     = pStream->State.cbTransferSize - pStream->State.cbTransferProcessed;
    bool     fTransferComplete  = !cbTransferLeft;
    uint64_t tsTransferNext     = 0;

    if (fTransferComplete)
    {
        /*
         * Try updating the wall clock.
         *
         * Note 1) Only certain guests (like Linux' snd_hda_intel) rely on the WALCLK register
         *         in order to determine the correct timing of the sound device. Other guests
         *         like Windows 7 + 10 (or even more exotic ones like Haiku) will completely
         *         ignore this.
         *
         * Note 2) When updating the WALCLK register too often / early (or even in a non-monotonic
         *         fashion) this *will* upset guest device drivers and will completely fuck up the
         *         sound output. Running VLC on the guest will tell!
         */
        const bool fWalClkSet = hdaR3WalClkSet(pThis,
                                                 hdaWalClkGetCurrent(pThis)
                                               + hdaR3StreamPeriodFramesToWalClk(pPeriod,
                                                                                   pStream->State.cbTransferProcessed
                                                                                 / pStream->State.cbFrameSize),
                                               false /* fForce */);
        RT_NOREF(fWalClkSet);
    }

    /* Does the period have any interrupts outstanding? */
    if (pStream->State.cTransferPendingInterrupts)
    {
        Log3Func(("[SD%RU8] Scheduling interrupt\n", pStream->u8SD));

        /*
         * Set the stream's BCIS bit.
         *
         * Note: This only must be done if the whole period is complete, and not if only
         * one specific BDL entry is complete (if it has the IOC bit set).
         *
         * This will otherwise confuses the guest when it 1) deasserts the interrupt,
         * 2) reads SDSTS (with BCIS set) and then 3) too early reads a (wrong) WALCLK value.
         *
         * snd_hda_intel on Linux will tell.
         */
        HDA_STREAM_REG(pThis, STS, pStream->u8SD) |= HDA_SDSTS_BCIS;

        /* Trigger an interrupt first and let hdaRegWriteSDSTS() deal with
         * ending / beginning a period. */
#ifndef LOG_ENABLED
        hdaProcessInterrupt(pThis);
#else
        hdaProcessInterrupt(pThis, __FUNCTION__);
#endif
    }
    else /* Transfer still in-flight -- schedule the next timing slot. */
    {
        uint32_t cbTransferNext = cbTransferLeft;

        /* No data left to transfer anymore or do we have more data left
         * than we can transfer per timing slot? Clamp. */
        if (   !cbTransferNext
            || cbTransferNext > pStream->State.cbTransferChunk)
        {
            cbTransferNext = pStream->State.cbTransferChunk;
        }

        tsTransferNext = tsNow + (cbTransferNext * pStream->State.cTicksPerByte);

        /*
         * If the current transfer is complete, reset our counter.
         *
         * This can happen for examlpe if the guest OS (like macOS) sets up
         * big BDLEs without IOC bits set (but for the last one) and the
         * transfer is complete before we reach such a BDL entry.
         */
        if (fTransferComplete)
            pStream->State.cbTransferProcessed = 0;
    }

    /* If we need to do another transfer, (re-)arm the device timer.  */
    if (tsTransferNext) /* Can be 0 if no next transfer is needed. */
    {
        Log3Func(("[SD%RU8] Scheduling timer\n", pStream->u8SD));

        TMTimerUnlock(pStream->pTimer);

        LogFunc(("Timer set SD%RU8\n", pStream->u8SD));
        hdaR3TimerSet(pStream->pHDAState, pStream, tsTransferNext, false /* fForce */);

        TMTimerLock(pStream->pTimer, VINF_SUCCESS);

        pStream->State.tsTransferNext = tsTransferNext;
    }

    pStream->State.tsTransferLast = tsNow;

    Log3Func(("[SD%RU8] cbTransferLeft=%RU32 -- %RU32/%RU32\n",
              pStream->u8SD, cbTransferLeft, pStream->State.cbTransferProcessed, pStream->State.cbTransferSize));
    Log3Func(("[SD%RU8] fTransferComplete=%RTbool, cTransferPendingInterrupts=%RU8\n",
              pStream->u8SD, fTransferComplete, pStream->State.cTransferPendingInterrupts));
    Log3Func(("[SD%RU8] tsNow=%RU64, tsTransferNext=%RU64 (in %RU64 ticks)\n",
              pStream->u8SD, tsNow, tsTransferNext, tsTransferNext - tsNow));

    hdaR3StreamPeriodUnlock(pPeriod);
    hdaR3StreamUnlock(pStream);

    return VINF_SUCCESS;
}

/**
 * Updates a HDA stream by doing its required data transfers.
 * The host sink(s) set the overall pace.
 *
 * This routine is called by both, the synchronous and the asynchronous, implementations.
 *
 * This routine is called by both, the synchronous and the asynchronous
 * (VBOX_WITH_AUDIO_HDA_ASYNC_IO), implementations.
 *
 * When running synchronously, the device DMA transfers *and* the mixer sink
 * processing is within the device timer.
 *
 * When running asynchronously, only the device DMA transfers are done in the
 * device timer, whereas the mixer sink processing then is done in the stream's
 * own async I/O thread. This thread also will call this function
 * (with fInTimer set to @c false).
 *
 * @param   pStream             HDA stream to update.
 * @param   fInTimer            Whether to this function was called from the timer
 *                              context or an asynchronous I/O stream thread (if supported).
 */
void hdaR3StreamUpdate(PHDASTREAM pStream, bool fInTimer)
{
    if (!pStream)
        return;

    PAUDMIXSINK pSink = NULL;
    if (   pStream->pMixSink
        && pStream->pMixSink->pMixSink)
    {
        pSink = pStream->pMixSink->pMixSink;
    }

    if (!AudioMixerSinkIsActive(pSink)) /* No sink available? Bail out. */
        return;

    int rc2;

    if (hdaGetDirFromSD(pStream->u8SD) == PDMAUDIODIR_OUT) /* Output (SDO). */
    {
        bool fDoRead = false; /* Whether to read from the HDA stream or not. */

# ifdef VBOX_WITH_AUDIO_HDA_ASYNC_IO
        if (fInTimer)
# endif
        {
            const uint32_t cbStreamFree = hdaR3StreamGetFree(pStream);
            if (cbStreamFree)
            {
                /* Do the DMA transfer. */
                rc2 = hdaR3StreamTransfer(pStream, cbStreamFree);
                AssertRC(rc2);
            }

            /* Only read from the HDA stream at the given scheduling rate. */
            const uint64_t tsNowNs = RTTimeNanoTS();
            if (tsNowNs - pStream->State.tsLastUpdateNs >= pStream->State.Cfg.Device.uSchedulingHintMs * RT_NS_1MS)
            {
                fDoRead = true;
                pStream->State.tsLastUpdateNs = tsNowNs;
            }
        }

        Log3Func(("[SD%RU8] fInTimer=%RTbool, fDoRead=%RTbool\n", pStream->u8SD, fInTimer, fDoRead));

# ifdef VBOX_WITH_AUDIO_HDA_ASYNC_IO
        if (fDoRead)
        {
            rc2 = hdaR3StreamAsyncIONotify(pStream);
            AssertRC(rc2);
        }
# endif

# ifdef VBOX_WITH_AUDIO_HDA_ASYNC_IO
        if (!fInTimer) /* In async I/O thread */
        {
# else
        if (fDoRead)
        {
# endif
            const uint32_t cbSinkWritable     = AudioMixerSinkGetWritable(pSink);
            const uint32_t cbStreamReadable   = hdaR3StreamGetUsed(pStream);
            const uint32_t cbToReadFromStream = RT_MIN(cbStreamReadable, cbSinkWritable);

            Log3Func(("[SD%RU8] cbSinkWritable=%RU32, cbStreamReadable=%RU32\n", pStream->u8SD, cbSinkWritable, cbStreamReadable));

            if (cbToReadFromStream)
            {
                /* Read (guest output) data and write it to the stream's sink. */
                rc2 = hdaR3StreamRead(pStream, cbToReadFromStream, NULL);
                AssertRC(rc2);
            }

            /* When running synchronously, update the associated sink here.
             * Otherwise this will be done in the async I/O thread. */
            rc2 = AudioMixerSinkUpdate(pSink);
            AssertRC(rc2);
        }
    }
    else /* Input (SDI). */
    {
# ifdef VBOX_WITH_AUDIO_HDA_ASYNC_IO
        if (!fInTimer)
        {
# endif
            rc2 = AudioMixerSinkUpdate(pSink);
            AssertRC(rc2);

            /* Is the sink ready to be read (host input data) from? If so, by how much? */
            uint32_t cbSinkReadable = AudioMixerSinkGetReadable(pSink);

            /* How much (guest input) data is available for writing at the moment for the HDA stream? */
            const uint32_t cbStreamFree = hdaR3StreamGetFree(pStream);

            Log3Func(("[SD%RU8] cbSinkReadable=%RU32, cbStreamFree=%RU32\n", pStream->u8SD, cbSinkReadable, cbStreamFree));

            /* Do not read more than the HDA stream can hold at the moment.
             * The host sets the overall pace. */
            if (cbSinkReadable > cbStreamFree)
                cbSinkReadable = cbStreamFree;

            if (cbSinkReadable)
            {
                uint8_t abFIFO[HDA_FIFO_MAX + 1];
                while (cbSinkReadable)
                {
                    uint32_t cbRead;
                    rc2 = AudioMixerSinkRead(pSink, AUDMIXOP_COPY,
                                             abFIFO, RT_MIN(cbSinkReadable, (uint32_t)sizeof(abFIFO)), &cbRead);
                    AssertRCBreak(rc2);

                    if (!cbRead)
                    {
                        AssertMsgFailed(("Nothing read from sink, even if %RU32 bytes were (still) announced\n", cbSinkReadable));
                        break;
                    }

                    /* Write (guest input) data to the stream which was read from stream's sink before. */
                    uint32_t cbWritten;
                    rc2 = hdaR3StreamWrite(pStream, abFIFO, cbRead, &cbWritten);
                    AssertRCBreak(rc2);

                    if (!cbWritten)
                    {
                        AssertFailed(); /* Should never happen, as we know how much we can write. */
                        break;
                    }

                    Assert(cbSinkReadable >= cbRead);
                    cbSinkReadable -= cbRead;
                }
            }
# ifdef VBOX_WITH_AUDIO_HDA_ASYNC_IO
        }
        else /* fInTimer */
        {
# endif

# ifdef VBOX_WITH_AUDIO_HDA_ASYNC_IO
            const uint64_t tsNowNs = RTTimeNanoTS();
            if (tsNowNs - pStream->State.tsLastUpdateNs >= pStream->State.Cfg.Device.uSchedulingHintMs * RT_NS_1MS)
            {
                rc2 = hdaR3StreamAsyncIONotify(pStream);
                AssertRC(rc2);

                pStream->State.tsLastUpdateNs = tsNowNs;
            }
# endif
            const uint32_t cbStreamUsed = hdaR3StreamGetUsed(pStream);
            if (cbStreamUsed)
            {
                rc2 = hdaR3StreamTransfer(pStream, cbStreamUsed);
                AssertRC(rc2);
            }
# ifdef VBOX_WITH_AUDIO_HDA_ASYNC_IO
        }
# endif
    }
}

/**
 * Locks an HDA stream for serialized access.
 *
 * @returns IPRT status code.
 * @param   pStream             HDA stream to lock.
 */
void hdaR3StreamLock(PHDASTREAM pStream)
{
    AssertPtrReturnVoid(pStream);
    int rc2 = RTCritSectEnter(&pStream->CritSect);
    AssertRC(rc2);
}

/**
 * Unlocks a formerly locked HDA stream.
 *
 * @returns IPRT status code.
 * @param   pStream             HDA stream to unlock.
 */
void hdaR3StreamUnlock(PHDASTREAM pStream)
{
    AssertPtrReturnVoid(pStream);
    int rc2 = RTCritSectLeave(&pStream->CritSect);
    AssertRC(rc2);
}

/**
 * Updates an HDA stream's current read or write buffer position (depending on the stream type) by
 * updating its associated LPIB register and DMA position buffer (if enabled).
 *
 * @returns Set LPIB value.
 * @param   pStream             HDA stream to update read / write position for.
 * @param   u32LPIB             New LPIB (position) value to set.
 */
uint32_t hdaR3StreamUpdateLPIB(PHDASTREAM pStream, uint32_t u32LPIB)
{
    AssertPtrReturn(pStream, 0);

    AssertMsg(u32LPIB <= pStream->u32CBL,
              ("[SD%RU8] New LPIB (%RU32) exceeds CBL (%RU32)\n", pStream->u8SD, u32LPIB, pStream->u32CBL));

    const PHDASTATE pThis = pStream->pHDAState;

    u32LPIB = RT_MIN(u32LPIB, pStream->u32CBL);

    LogFlowFunc(("[SD%RU8]: LPIB=%RU32 (DMA Position Buffer Enabled: %RTbool)\n",
                 pStream->u8SD, u32LPIB, pThis->fDMAPosition));

    /* Update LPIB in any case. */
    HDA_STREAM_REG(pThis, LPIB, pStream->u8SD) = u32LPIB;

    /* Do we need to tell the current DMA position? */
    if (pThis->fDMAPosition)
    {
        int rc2 = PDMDevHlpPCIPhysWrite(pThis->CTX_SUFF(pDevIns),
                                        pThis->u64DPBase + (pStream->u8SD * 2 * sizeof(uint32_t)),
                                        (void *)&u32LPIB, sizeof(uint32_t));
        AssertRC(rc2);
    }

    return u32LPIB;
}

# ifdef HDA_USE_DMA_ACCESS_HANDLER
/**
 * Registers access handlers for a stream's BDLE DMA accesses.
 *
 * @returns true if registration was successful, false if not.
 * @param   pStream             HDA stream to register BDLE access handlers for.
 */
bool hdaR3StreamRegisterDMAHandlers(PHDASTREAM pStream)
{
    /* At least LVI and the BDL base must be set. */
    if (   !pStream->u16LVI
        || !pStream->u64BDLBase)
    {
        return false;
    }

    hdaR3StreamUnregisterDMAHandlers(pStream);

    LogFunc(("Registering ...\n"));

    int rc = VINF_SUCCESS;

    /*
     * Create BDLE ranges.
     */

    struct BDLERANGE
    {
        RTGCPHYS uAddr;
        uint32_t uSize;
    } arrRanges[16]; /** @todo Use a define. */

    size_t cRanges = 0;

    for (uint16_t i = 0; i < pStream->u16LVI + 1; i++)
    {
        HDABDLE BDLE;
        rc = hdaR3BDLEFetch(pThis, &BDLE, pStream->u64BDLBase, i /* Index */);
        if (RT_FAILURE(rc))
            break;

        bool fAddRange = true;
        BDLERANGE *pRange;

        if (cRanges)
        {
            pRange = &arrRanges[cRanges - 1];

            /* Is the current range a direct neighbor of the current BLDE? */
            if ((pRange->uAddr + pRange->uSize) == BDLE.Desc.u64BufAdr)
            {
                /* Expand the current range by the current BDLE's size. */
                pRange->uSize += BDLE.Desc.u32BufSize;

                /* Adding a new range in this case is not needed anymore. */
                fAddRange = false;

                LogFunc(("Expanding range %zu by %RU32 (%RU32 total now)\n", cRanges - 1, BDLE.Desc.u32BufSize, pRange->uSize));
            }
        }

        /* Do we need to add a new range? */
        if (   fAddRange
            && cRanges < RT_ELEMENTS(arrRanges))
        {
            pRange = &arrRanges[cRanges];

            pRange->uAddr = BDLE.Desc.u64BufAdr;
            pRange->uSize = BDLE.Desc.u32BufSize;

            LogFunc(("Adding range %zu - 0x%x (%RU32)\n", cRanges, pRange->uAddr, pRange->uSize));

            cRanges++;
        }
    }

    LogFunc(("%zu ranges total\n", cRanges));

    /*
     * Register all ranges as DMA access handlers.
     */

    for (size_t i = 0; i < cRanges; i++)
    {
        BDLERANGE *pRange = &arrRanges[i];

        PHDADMAACCESSHANDLER pHandler = (PHDADMAACCESSHANDLER)RTMemAllocZ(sizeof(HDADMAACCESSHANDLER));
        if (!pHandler)
        {
            rc = VERR_NO_MEMORY;
            break;
        }

        RTListAppend(&pStream->State.lstDMAHandlers, &pHandler->Node);

        pHandler->pStream = pStream; /* Save a back reference to the owner. */

        char szDesc[32];
        RTStrPrintf(szDesc, sizeof(szDesc), "HDA[SD%RU8 - RANGE%02zu]", pStream->u8SD, i);

        int rc2 = PGMR3HandlerPhysicalTypeRegister(PDMDevHlpGetVM(pStream->pHDAState->pDevInsR3), PGMPHYSHANDLERKIND_WRITE,
                                                   hdaDMAAccessHandler,
                                                   NULL, NULL, NULL,
                                                   NULL, NULL, NULL,
                                                   szDesc, &pHandler->hAccessHandlerType);
        AssertRCBreak(rc2);

        pHandler->BDLEAddr  = pRange->uAddr;
        pHandler->BDLESize  = pRange->uSize;

        /* Get first and last pages of the BDLE range. */
        RTGCPHYS pgFirst = pRange->uAddr & ~PAGE_OFFSET_MASK;
        RTGCPHYS pgLast  = RT_ALIGN(pgFirst + pRange->uSize, PAGE_SIZE);

        /* Calculate the region size (in pages). */
        RTGCPHYS regionSize = RT_ALIGN(pgLast - pgFirst, PAGE_SIZE);

        pHandler->GCPhysFirst = pgFirst;
        pHandler->GCPhysLast  = pHandler->GCPhysFirst + (regionSize - 1);

        LogFunc(("\tRegistering region '%s': 0x%x - 0x%x (region size: %zu)\n",
                 szDesc, pHandler->GCPhysFirst, pHandler->GCPhysLast, regionSize));
        LogFunc(("\tBDLE @ 0x%x - 0x%x (%RU32)\n",
                 pHandler->BDLEAddr, pHandler->BDLEAddr + pHandler->BDLESize, pHandler->BDLESize));

        rc2 = PGMHandlerPhysicalRegister(PDMDevHlpGetVM(pStream->pHDAState->pDevInsR3),
                                         pHandler->GCPhysFirst, pHandler->GCPhysLast,
                                         pHandler->hAccessHandlerType, pHandler, NIL_RTR0PTR, NIL_RTRCPTR,
                                         szDesc);
        AssertRCBreak(rc2);

        pHandler->fRegistered = true;
    }

    LogFunc(("Registration ended with rc=%Rrc\n", rc));

    return RT_SUCCESS(rc);
}

/**
 * Unregisters access handlers of a stream's BDLEs.
 *
 * @param   pStream             HDA stream to unregister BDLE access handlers for.
 */
void hdaR3StreamUnregisterDMAHandlers(PHDASTREAM pStream)
{
    LogFunc(("\n"));

    PHDADMAACCESSHANDLER pHandler, pHandlerNext;
    RTListForEachSafe(&pStream->State.lstDMAHandlers, pHandler, pHandlerNext, HDADMAACCESSHANDLER, Node)
    {
        if (!pHandler->fRegistered) /* Handler not registered? Skip. */
            continue;

        LogFunc(("Unregistering 0x%x - 0x%x (%zu)\n",
                 pHandler->GCPhysFirst, pHandler->GCPhysLast, pHandler->GCPhysLast - pHandler->GCPhysFirst));

        int rc2 = PGMHandlerPhysicalDeregister(PDMDevHlpGetVM(pStream->pHDAState->pDevInsR3),
                                               pHandler->GCPhysFirst);
        AssertRC(rc2);

        RTListNodeRemove(&pHandler->Node);

        RTMemFree(pHandler);
        pHandler = NULL;
    }

    Assert(RTListIsEmpty(&pStream->State.lstDMAHandlers));
}
# endif /* HDA_USE_DMA_ACCESS_HANDLER */

# ifdef VBOX_WITH_AUDIO_HDA_ASYNC_IO
/**
 * Asynchronous I/O thread for a HDA stream.
 * This will do the heavy lifting work for us as soon as it's getting notified by another thread.
 *
 * @returns IPRT status code.
 * @param   hThreadSelf         Thread handle.
 * @param   pvUser              User argument. Must be of type PHDASTREAMTHREADCTX.
 */
DECLCALLBACK(int) hdaR3StreamAsyncIOThread(RTTHREAD hThreadSelf, void *pvUser)
{
    PHDASTREAMTHREADCTX pCtx = (PHDASTREAMTHREADCTX)pvUser;
    AssertPtr(pCtx);

    PHDASTREAM pStream = pCtx->pStream;
    AssertPtr(pStream);

    PHDASTREAMSTATEAIO pAIO = &pCtx->pStream->State.AIO;

    ASMAtomicXchgBool(&pAIO->fStarted, true);

    RTThreadUserSignal(hThreadSelf);

    LogFunc(("[SD%RU8]: Started\n", pStream->u8SD));

    for (;;)
    {
        int rc2 = RTSemEventWait(pAIO->Event, RT_INDEFINITE_WAIT);
        if (RT_FAILURE(rc2))
            break;

        if (ASMAtomicReadBool(&pAIO->fShutdown))
            break;

        rc2 = RTCritSectEnter(&pAIO->CritSect);
        if (RT_SUCCESS(rc2))
        {
            if (!pAIO->fEnabled)
            {
                RTCritSectLeave(&pAIO->CritSect);
                continue;
            }

            hdaR3StreamUpdate(pStream, false /* fInTimer */);

            int rc3 = RTCritSectLeave(&pAIO->CritSect);
            AssertRC(rc3);
        }

        AssertRC(rc2);
    }

    LogFunc(("[SD%RU8]: Ended\n", pStream->u8SD));

    ASMAtomicXchgBool(&pAIO->fStarted, false);

    return VINF_SUCCESS;
}

/**
 * Creates the async I/O thread for a specific HDA audio stream.
 *
 * @returns IPRT status code.
 * @param   pStream             HDA audio stream to create the async I/O thread for.
 */
int hdaR3StreamAsyncIOCreate(PHDASTREAM pStream)
{
    PHDASTREAMSTATEAIO pAIO = &pStream->State.AIO;

    int rc;

    if (!ASMAtomicReadBool(&pAIO->fStarted))
    {
        pAIO->fShutdown = false;
        pAIO->fEnabled  = true; /* Enabled by default. */

        rc = RTSemEventCreate(&pAIO->Event);
        if (RT_SUCCESS(rc))
        {
            rc = RTCritSectInit(&pAIO->CritSect);
            if (RT_SUCCESS(rc))
            {
                HDASTREAMTHREADCTX Ctx = { pStream->pHDAState, pStream };

                char szThreadName[64];
                RTStrPrintf2(szThreadName, sizeof(szThreadName), "hdaAIO%RU8", pStream->u8SD);

                rc = RTThreadCreate(&pAIO->Thread, hdaR3StreamAsyncIOThread, &Ctx,
                                    0, RTTHREADTYPE_IO, RTTHREADFLAGS_WAITABLE, szThreadName);
                if (RT_SUCCESS(rc))
                    rc = RTThreadUserWait(pAIO->Thread, 10 * 1000 /* 10s timeout */);
            }
        }
    }
    else
        rc = VINF_SUCCESS;

    LogFunc(("[SD%RU8]: Returning %Rrc\n", pStream->u8SD, rc));
    return rc;
}

/**
 * Destroys the async I/O thread of a specific HDA audio stream.
 *
 * @returns IPRT status code.
 * @param   pStream             HDA audio stream to destroy the async I/O thread for.
 */
int hdaR3StreamAsyncIODestroy(PHDASTREAM pStream)
{
    PHDASTREAMSTATEAIO pAIO = &pStream->State.AIO;

    if (!ASMAtomicReadBool(&pAIO->fStarted))
        return VINF_SUCCESS;

    ASMAtomicWriteBool(&pAIO->fShutdown, true);

    int rc = hdaR3StreamAsyncIONotify(pStream);
    AssertRC(rc);

    int rcThread;
    rc = RTThreadWait(pAIO->Thread, 30 * 1000 /* 30s timeout */, &rcThread);
    LogFunc(("Async I/O thread ended with %Rrc (%Rrc)\n", rc, rcThread));

    if (RT_SUCCESS(rc))
    {
        rc = RTCritSectDelete(&pAIO->CritSect);
        AssertRC(rc);

        rc = RTSemEventDestroy(pAIO->Event);
        AssertRC(rc);

        pAIO->fStarted  = false;
        pAIO->fShutdown = false;
        pAIO->fEnabled  = false;
    }

    LogFunc(("[SD%RU8]: Returning %Rrc\n", pStream->u8SD, rc));
    return rc;
}

/**
 * Lets the stream's async I/O thread know that there is some data to process.
 *
 * @returns IPRT status code.
 * @param   pStream             HDA stream to notify async I/O thread for.
 */
int hdaR3StreamAsyncIONotify(PHDASTREAM pStream)
{
    return RTSemEventSignal(pStream->State.AIO.Event);
}

/**
 * Locks the async I/O thread of a specific HDA audio stream.
 *
 * @param   pStream             HDA stream to lock async I/O thread for.
 */
void hdaR3StreamAsyncIOLock(PHDASTREAM pStream)
{
    PHDASTREAMSTATEAIO pAIO = &pStream->State.AIO;

    if (!ASMAtomicReadBool(&pAIO->fStarted))
        return;

    int rc2 = RTCritSectEnter(&pAIO->CritSect);
    AssertRC(rc2);
}

/**
 * Unlocks the async I/O thread of a specific HDA audio stream.
 *
 * @param   pStream             HDA stream to unlock async I/O thread for.
 */
void hdaR3StreamAsyncIOUnlock(PHDASTREAM pStream)
{
    PHDASTREAMSTATEAIO pAIO = &pStream->State.AIO;

    if (!ASMAtomicReadBool(&pAIO->fStarted))
        return;

    int rc2 = RTCritSectLeave(&pAIO->CritSect);
    AssertRC(rc2);
}

/**
 * Enables (resumes) or disables (pauses) the async I/O thread.
 *
 * @param   pStream             HDA stream to enable/disable async I/O thread for.
 * @param   fEnable             Whether to enable or disable the I/O thread.
 *
 * @remarks Does not do locking.
 */
void hdaR3StreamAsyncIOEnable(PHDASTREAM pStream, bool fEnable)
{
    PHDASTREAMSTATEAIO pAIO = &pStream->State.AIO;
    ASMAtomicXchgBool(&pAIO->fEnabled, fEnable);
}
# endif /* VBOX_WITH_AUDIO_HDA_ASYNC_IO */

#endif /* IN_RING3 */

