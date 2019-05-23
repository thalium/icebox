/* $Id: WebMWriter.cpp $ */
/** @file
 * WebMWriter.cpp - WebM container handling.
 */

/*
 * Copyright (C) 2013-2018 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/**
 * For more information, see:
 * - https://w3c.github.io/media-source/webm-byte-stream-format.html
 * - https://www.webmproject.org/docs/container/#muxer-guidelines
 */

#define LOG_GROUP LOG_GROUP_MAIN_DISPLAY
#include "LoggingNew.h"

#include <list>
#include <map>
#include <queue>
#include <stack>

#include <math.h> /* For lround.h. */

#include <iprt/asm.h>
#include <iprt/buildconfig.h>
#include <iprt/cdefs.h>
#include <iprt/critsect.h>
#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/rand.h>
#include <iprt/string.h>

#include <VBox/log.h>
#include <VBox/version.h>

#include "WebMWriter.h"
#include "EBML_MKV.h"


WebMWriter::WebMWriter(void)
{
    /* Size (in bytes) of time code to write. We use 2 bytes (16 bit) by default. */
    m_cbTimecode   = 2;
    m_uTimecodeMax = UINT16_MAX;

    m_fInTracksSection = false;
}

WebMWriter::~WebMWriter(void)
{
    Close();
}

/**
 * Opens (creates) an output file using an already open file handle.
 *
 * @returns VBox status code.
 * @param   a_pszFilename   Name of the file the file handle points at.
 * @param   a_phFile        Pointer to open file handle to use.
 * @param   a_enmAudioCodec Audio codec to use.
 * @param   a_enmVideoCodec Video codec to use.
 */
int WebMWriter::OpenEx(const char *a_pszFilename, PRTFILE a_phFile,
                       WebMWriter::AudioCodec a_enmAudioCodec, WebMWriter::VideoCodec a_enmVideoCodec)
{
    try
    {
        m_enmAudioCodec = a_enmAudioCodec;
        m_enmVideoCodec = a_enmVideoCodec;

        LogFunc(("Creating '%s'\n", a_pszFilename));

        int rc = createEx(a_pszFilename, a_phFile);
        if (RT_SUCCESS(rc))
        {
            rc = init();
            if (RT_SUCCESS(rc))
                rc = writeHeader();
        }
    }
    catch(int rc)
    {
        return rc;
    }
    return VINF_SUCCESS;
}

/**
 * Opens an output file.
 *
 * @returns VBox status code.
 * @param   a_pszFilename   Name of the file to create.
 * @param   a_fOpen         File open mode of type RTFILE_O_.
 * @param   a_enmAudioCodec Audio codec to use.
 * @param   a_enmVideoCodec Video codec to use.
 */
int WebMWriter::Open(const char *a_pszFilename, uint64_t a_fOpen,
                     WebMWriter::AudioCodec a_enmAudioCodec, WebMWriter::VideoCodec a_enmVideoCodec)
{
    try
    {
        m_enmAudioCodec = a_enmAudioCodec;
        m_enmVideoCodec = a_enmVideoCodec;

        LogFunc(("Creating '%s'\n", a_pszFilename));

        int rc = create(a_pszFilename, a_fOpen);
        if (RT_SUCCESS(rc))
        {
            rc = init();
            if (RT_SUCCESS(rc))
                rc = writeHeader();
        }
    }
    catch(int rc)
    {
        return rc;
    }
    return VINF_SUCCESS;
}

/**
 * Closes the WebM file and drains all queues.
 *
 * @returns IPRT status code.
 */
int WebMWriter::Close(void)
{
    LogFlowFuncEnter();

    if (!isOpen())
        return VINF_SUCCESS;

    /* Make sure to drain all queues. */
    processQueues(&CurSeg.queueBlocks, true /* fForce */);

    writeFooter();

    WebMTracks::iterator itTrack = CurSeg.mapTracks.begin();
    for (; itTrack != CurSeg.mapTracks.end(); ++itTrack)
    {
        WebMTrack *pTrack = itTrack->second;

        delete pTrack;
        CurSeg.mapTracks.erase(itTrack);
    }

    Assert(CurSeg.queueBlocks.Map.size() == 0);
    Assert(CurSeg.mapTracks.size() == 0);

    close();

    return VINF_SUCCESS;
}

/**
 * Adds an audio track.
 *
 * @returns IPRT status code.
 * @param   uHz             Input sampling rate.
 *                          Must be supported by the selected audio codec.
 * @param   cChannels       Number of input audio channels.
 * @param   cBits           Number of input bits per channel.
 * @param   puTrack         Track number on successful creation. Optional.
 */
int WebMWriter::AddAudioTrack(uint16_t uHz, uint8_t cChannels, uint8_t cBits, uint8_t *puTrack)
{
#ifdef VBOX_WITH_LIBOPUS
    int rc;

    /*
     * Check if the requested codec rate is supported.
     *
     * Only the following values are supported by an Opus standard build
     * -- every other rate only is supported by a custom build.
     */
    switch (uHz)
    {
        case 48000:
        case 24000:
        case 16000:
        case 12000:
        case  8000:
            rc = VINF_SUCCESS;
            break;

        default:
            rc = VERR_NOT_SUPPORTED;
            break;
    }

    if (RT_FAILURE(rc))
        return rc;

    /* Some players (e.g. Firefox with Nestegg) rely on track numbers starting at 1.
     * Using a track number 0 will show those files as being corrupted. */
    const uint8_t uTrack = (uint8_t)CurSeg.mapTracks.size() + 1;

    subStart(MkvElem_TrackEntry);

    serializeUnsignedInteger(MkvElem_TrackNumber, (uint8_t)uTrack);
    serializeString         (MkvElem_Language,    "und" /* "Undefined"; see ISO-639-2. */);
    serializeUnsignedInteger(MkvElem_FlagLacing,  (uint8_t)0);

    WebMTrack *pTrack = new WebMTrack(WebMTrackType_Audio, uTrack, RTFileTell(getFile()));

    pTrack->Audio.uHz            = uHz;
    pTrack->Audio.msPerBlock     = 20; /** Opus uses 20ms by default. Make this configurable? */
    pTrack->Audio.framesPerBlock = uHz / (1000 /* s in ms */ / pTrack->Audio.msPerBlock);

    WEBMOPUSPRIVDATA opusPrivData(uHz, cChannels);

    LogFunc(("Opus @ %RU16Hz (%RU16ms + %RU16 frames per block)\n",
             pTrack->Audio.uHz, pTrack->Audio.msPerBlock, pTrack->Audio.framesPerBlock));

    serializeUnsignedInteger(MkvElem_TrackUID,     pTrack->uUUID, 4)
          .serializeUnsignedInteger(MkvElem_TrackType,    2 /* Audio */)
          .serializeString(MkvElem_CodecID,               "A_OPUS")
          .serializeData(MkvElem_CodecPrivate,            &opusPrivData, sizeof(opusPrivData))
          .serializeUnsignedInteger(MkvElem_CodecDelay,   0)
          .serializeUnsignedInteger(MkvElem_SeekPreRoll,  80 * 1000000) /* 80ms in ns. */
          .subStart(MkvElem_Audio)
              .serializeFloat(MkvElem_SamplingFrequency,  (float)uHz)
              .serializeUnsignedInteger(MkvElem_Channels, cChannels)
              .serializeUnsignedInteger(MkvElem_BitDepth, cBits)
          .subEnd(MkvElem_Audio)
          .subEnd(MkvElem_TrackEntry);

    CurSeg.mapTracks[uTrack] = pTrack;

    if (puTrack)
        *puTrack = uTrack;

    return VINF_SUCCESS;
#else
    RT_NOREF(uHz, cChannels, cBits, puTrack);
    return VERR_NOT_SUPPORTED;
#endif
}

/**
 * Adds a video track.
 *
 * @returns IPRT status code.
 * @param   uWidth              Width (in pixels) of the video track.
 * @param   uHeight             Height (in pixels) of the video track.
 * @param   uFPS                FPS (Frames Per Second) of the video track.
 * @param   puTrack             Track number of the added video track on success. Optional.
 */
int WebMWriter::AddVideoTrack(uint16_t uWidth, uint16_t uHeight, uint32_t uFPS, uint8_t *puTrack)
{
#ifdef VBOX_WITH_LIBVPX
    /* Some players (e.g. Firefox with Nestegg) rely on track numbers starting at 1.
     * Using a track number 0 will show those files as being corrupted. */
    const uint8_t uTrack = (uint8_t)CurSeg.mapTracks.size() + 1;

    subStart(MkvElem_TrackEntry);

    serializeUnsignedInteger(MkvElem_TrackNumber, (uint8_t)uTrack);
    serializeString         (MkvElem_Language,    "und" /* "Undefined"; see ISO-639-2. */);
    serializeUnsignedInteger(MkvElem_FlagLacing,  (uint8_t)0);

    WebMTrack *pTrack = new WebMTrack(WebMTrackType_Video, uTrack, RTFileTell(getFile()));

    /** @todo Resolve codec type. */
    serializeUnsignedInteger(MkvElem_TrackUID,    pTrack->uUUID /* UID */, 4)
          .serializeUnsignedInteger(MkvElem_TrackType,   1 /* Video */)
          .serializeString(MkvElem_CodecID,              "V_VP8")
          .subStart(MkvElem_Video)
              .serializeUnsignedInteger(MkvElem_PixelWidth,  uWidth)
              .serializeUnsignedInteger(MkvElem_PixelHeight, uHeight)
              /* Some players rely on the FPS rate for timing calculations.
               * So make sure to *always* include that. */
              .serializeFloat          (MkvElem_FrameRate,   (float)uFPS)
          .subEnd(MkvElem_Video);

    subEnd(MkvElem_TrackEntry);

    CurSeg.mapTracks[uTrack] = pTrack;

    if (puTrack)
        *puTrack = uTrack;

    return VINF_SUCCESS;
#else
    RT_NOREF(uWidth, uHeight, dbFPS, puTrack);
    return VERR_NOT_SUPPORTED;
#endif
}

/**
 * Gets file name.
 *
 * @returns File name as UTF-8 string.
 */
const Utf8Str& WebMWriter::GetFileName(void)
{
    return getFileName();
}

/**
 * Gets current output file size.
 *
 * @returns File size in bytes.
 */
uint64_t WebMWriter::GetFileSize(void)
{
    return getFileSize();
}

/**
 * Gets current free storage space available for the file.
 *
 * @returns Available storage free space.
 */
uint64_t WebMWriter::GetAvailableSpace(void)
{
    return getAvailableSpace();
}

/**
 * Takes care of the initialization of the instance.
 *
 * @returns IPRT status code.
 */
int WebMWriter::init(void)
{
    return CurSeg.init();
}

/**
 * Takes care of the destruction of the instance.
 */
void WebMWriter::destroy(void)
{
    CurSeg.destroy();
}

/**
 * Writes the WebM file header.
 *
 * @returns IPRT status code.
 */
int WebMWriter::writeHeader(void)
{
    LogFunc(("Header @ %RU64\n", RTFileTell(getFile())));

    subStart(MkvElem_EBML)
          .serializeUnsignedInteger(MkvElem_EBMLVersion, 1)
          .serializeUnsignedInteger(MkvElem_EBMLReadVersion, 1)
          .serializeUnsignedInteger(MkvElem_EBMLMaxIDLength, 4)
          .serializeUnsignedInteger(MkvElem_EBMLMaxSizeLength, 8)
          .serializeString(MkvElem_DocType, "webm")
          .serializeUnsignedInteger(MkvElem_DocTypeVersion, 2)
          .serializeUnsignedInteger(MkvElem_DocTypeReadVersion, 2)
          .subEnd(MkvElem_EBML);

    subStart(MkvElem_Segment);

    /* Save offset of current segment. */
    CurSeg.offStart = RTFileTell(getFile());

    writeSeekHeader();

    /* Save offset of upcoming tracks segment. */
    CurSeg.offTracks = RTFileTell(getFile());

    /* The tracks segment starts right after this header. */
    subStart(MkvElem_Tracks);
    m_fInTracksSection = true;

    return VINF_SUCCESS;
}

/**
 * Writes a simple block into the EBML structure.
 *
 * @returns IPRT status code.
 * @param   a_pTrack        Track the simple block is assigned to.
 * @param   a_pBlock        Simple block to write.
 */
int WebMWriter::writeSimpleBlockEBML(WebMTrack *a_pTrack, WebMSimpleBlock *a_pBlock)
{
#ifdef LOG_ENABLED
    WebMCluster &Cluster = CurSeg.CurCluster;

    Log3Func(("[T%RU8C%RU64] Off=%RU64, AbsPTSMs=%RU64, RelToClusterMs=%RU16, %zu bytes\n",
              a_pTrack->uTrack, Cluster.uID, RTFileTell(getFile()),
              a_pBlock->Data.tcAbsPTSMs, a_pBlock->Data.tcRelToClusterMs, a_pBlock->Data.cb));
#endif
    /*
     * Write a "Simple Block".
     */
    writeClassId(MkvElem_SimpleBlock);
    /* Block size. */
    writeUnsignedInteger(0x10000000u | (  1                 /* Track number size. */
                                        + m_cbTimecode      /* Timecode size .*/
                                        + 1                 /* Flags size. */
                                        + a_pBlock->Data.cb /* Actual frame data size. */),  4);
    /* Track number. */
    writeSize(a_pTrack->uTrack);
    /* Timecode (relative to cluster opening timecode). */
    writeUnsignedInteger(a_pBlock->Data.tcRelToClusterMs, m_cbTimecode);
    /* Flags. */
    writeUnsignedInteger(a_pBlock->Data.fFlags, 1);
    /* Frame data. */
    write(a_pBlock->Data.pv, a_pBlock->Data.cb);

    return VINF_SUCCESS;
}

/**
 * Writes a simple block and enqueues it into the segment's render queue.
 *
 * @returns IPRT status code.
 * @param   a_pTrack        Track the simple block is assigned to.
 * @param   a_pBlock        Simple block to write and enqueue.
 */
int WebMWriter::writeSimpleBlockQueued(WebMTrack *a_pTrack, WebMSimpleBlock *a_pBlock)
{
    RT_NOREF(a_pTrack);

    int rc = VINF_SUCCESS;

    try
    {
        const WebMTimecodeAbs tcAbsPTS = a_pBlock->Data.tcAbsPTSMs;

        /* See if we already have an entry for the specified timecode in our queue. */
        WebMBlockMap::iterator itQueue = CurSeg.queueBlocks.Map.find(tcAbsPTS);
        if (itQueue != CurSeg.queueBlocks.Map.end()) /* Use existing queue. */
        {
            WebMTimecodeBlocks &Blocks = itQueue->second;
            Blocks.Enqueue(a_pBlock);
        }
        else /* Create a new timecode entry. */
        {
            WebMTimecodeBlocks Blocks;
            Blocks.Enqueue(a_pBlock);

            CurSeg.queueBlocks.Map[tcAbsPTS] = Blocks;
        }

        processQueues(&CurSeg.queueBlocks, false /* fForce */);
    }
    catch(...)
    {
        delete a_pBlock;
        a_pBlock = NULL;

        rc = VERR_NO_MEMORY;
    }

    return rc;
}

#ifdef VBOX_WITH_LIBVPX
/**
 * Writes VPX (VP8 video) simple data block.
 *
 * @returns IPRT status code.
 * @param   a_pTrack        Track ID to write data to.
 * @param   a_pCfg          VPX encoder configuration to use.
 * @param   a_pPkt          VPX packet video data packet to write.
 */
int WebMWriter::writeSimpleBlockVP8(WebMTrack *a_pTrack, const vpx_codec_enc_cfg_t *a_pCfg, const vpx_codec_cx_pkt_t *a_pPkt)
{
    RT_NOREF(a_pTrack);

    /* Calculate the absolute PTS of this frame (in ms). */
    WebMTimecodeAbs tcAbsPTSMs =   a_pPkt->data.frame.pts * 1000
                                 * (uint64_t) a_pCfg->g_timebase.num / a_pCfg->g_timebase.den;
    if (   tcAbsPTSMs
        && tcAbsPTSMs <= a_pTrack->tcAbsLastWrittenMs)
    {
        tcAbsPTSMs = a_pTrack->tcAbsLastWrittenMs + 1;
    }

    const bool fKeyframe = RT_BOOL(a_pPkt->data.frame.flags & VPX_FRAME_IS_KEY);

    uint8_t fFlags = VBOX_WEBM_BLOCK_FLAG_NONE;
    if (fKeyframe)
        fFlags |= VBOX_WEBM_BLOCK_FLAG_KEY_FRAME;
    if (a_pPkt->data.frame.flags & VPX_FRAME_IS_INVISIBLE)
        fFlags |= VBOX_WEBM_BLOCK_FLAG_INVISIBLE;

    return writeSimpleBlockQueued(a_pTrack,
                                  new WebMSimpleBlock(a_pTrack,
                                                      tcAbsPTSMs, a_pPkt->data.frame.buf, a_pPkt->data.frame.sz, fFlags));
}
#endif /* VBOX_WITH_LIBVPX */

#ifdef VBOX_WITH_LIBOPUS
/**
 * Writes an Opus (audio) simple data block.
 *
 * @returns IPRT status code.
 * @param   a_pTrack        Track ID to write data to.
 * @param   pvData          Pointer to simple data block to write.
 * @param   cbData          Size (in bytes) of simple data block to write.
 * @param   tcAbsPTSMs      Absolute PTS of simple data block.
 *
 * @remarks Audio blocks that have same absolute timecode as video blocks SHOULD be written before the video blocks.
 */
int WebMWriter::writeSimpleBlockOpus(WebMTrack *a_pTrack, const void *pvData, size_t cbData, WebMTimecodeAbs tcAbsPTSMs)
{
    AssertPtrReturn(a_pTrack, VERR_INVALID_POINTER);
    AssertPtrReturn(pvData,   VERR_INVALID_POINTER);
    AssertReturn(cbData,      VERR_INVALID_PARAMETER);

    /* Every Opus frame is a key frame. */
    const uint8_t fFlags = VBOX_WEBM_BLOCK_FLAG_KEY_FRAME;

    return writeSimpleBlockQueued(a_pTrack,
                                  new WebMSimpleBlock(a_pTrack, tcAbsPTSMs, pvData, cbData, fFlags));
}
#endif /* VBOX_WITH_LIBOPUS */

/**
 * Writes a data block to the specified track.
 *
 * @returns IPRT status code.
 * @param   uTrack          Track ID to write data to.
 * @param   pvData          Pointer to data block to write.
 * @param   cbData          Size (in bytes) of data block to write.
 */
int WebMWriter::WriteBlock(uint8_t uTrack, const void *pvData, size_t cbData)
{
    RT_NOREF(cbData); /* Only needed for assertions for now. */

    int rc = RTCritSectEnter(&CurSeg.CritSect);
    AssertRC(rc);

    WebMTracks::iterator itTrack = CurSeg.mapTracks.find(uTrack);
    if (itTrack == CurSeg.mapTracks.end())
    {
        RTCritSectLeave(&CurSeg.CritSect);
        return VERR_NOT_FOUND;
    }

    WebMTrack *pTrack = itTrack->second;
    AssertPtr(pTrack);

    if (m_fInTracksSection)
    {
        subEnd(MkvElem_Tracks);
        m_fInTracksSection = false;
    }

    switch (pTrack->enmType)
    {

        case WebMTrackType_Audio:
        {
#ifdef VBOX_WITH_LIBOPUS
            if (m_enmAudioCodec == WebMWriter::AudioCodec_Opus)
            {
                Assert(cbData == sizeof(WebMWriter::BlockData_Opus));
                WebMWriter::BlockData_Opus *pData = (WebMWriter::BlockData_Opus *)pvData;
                rc = writeSimpleBlockOpus(pTrack, pData->pvData, pData->cbData, pData->uPTSMs);
            }
            else
#endif /* VBOX_WITH_LIBOPUS */
                rc = VERR_NOT_SUPPORTED;
            break;
        }

        case WebMTrackType_Video:
        {
#ifdef VBOX_WITH_LIBVPX
            if (m_enmVideoCodec == WebMWriter::VideoCodec_VP8)
            {
                Assert(cbData == sizeof(WebMWriter::BlockData_VP8));
                WebMWriter::BlockData_VP8 *pData = (WebMWriter::BlockData_VP8 *)pvData;
                rc = writeSimpleBlockVP8(pTrack, pData->pCfg, pData->pPkt);
            }
            else
#endif /* VBOX_WITH_LIBVPX */
                rc = VERR_NOT_SUPPORTED;
            break;
        }

        default:
            rc = VERR_NOT_SUPPORTED;
            break;
    }

    int rc2 = RTCritSectLeave(&CurSeg.CritSect);
    AssertRC(rc2);

    return rc;
}

/**
 * Processes a render queue.
 *
 * @returns IPRT status code.
 * @param   pQueue          Queue to process.
 * @param   fForce          Whether forcing to process the render queue or not.
 *                          Needed to drain the queues when terminating.
 */
int WebMWriter::processQueues(WebMQueue *pQueue, bool fForce)
{
    if (pQueue->tsLastProcessedMs == 0)
        pQueue->tsLastProcessedMs = RTTimeMilliTS();

    if (!fForce)
    {
        /* Only process when we reached a certain threshold. */
        if (RTTimeMilliTS() - pQueue->tsLastProcessedMs < 5000 /* ms */ /** @todo Make this configurable */)
            return VINF_SUCCESS;
    }

    WebMCluster &Cluster = CurSeg.CurCluster;

    /* Iterate through the block map. */
    WebMBlockMap::iterator it = pQueue->Map.begin();
    while (it != CurSeg.queueBlocks.Map.end())
    {
        WebMTimecodeAbs    mapAbsPTSMs = it->first;
        WebMTimecodeBlocks mapBlocks   = it->second;

        /* Whether to start a new cluster or not. */
        bool fClusterStart = false;

        /* No blocks written yet? Start a new cluster. */
        if (Cluster.cBlocks == 0)
            fClusterStart = true;

        /* Did we reach the maximum a cluster can hold? Use a new cluster then. */
        if (mapAbsPTSMs - Cluster.tcAbsStartMs > VBOX_WEBM_CLUSTER_MAX_LEN_MS)
            fClusterStart = true;

        /* If the block map indicates that a cluster is needed for this timecode, create one. */
        if (mapBlocks.fClusterNeeded)
            fClusterStart = true;

        if (   fClusterStart
            && !mapBlocks.fClusterStarted)
        {
            if (Cluster.fOpen) /* Close current cluster first. */
            {
                /* Make sure that the current cluster contained some data.  */
                Assert(Cluster.offStart);
                Assert(Cluster.cBlocks);

                subEnd(MkvElem_Cluster);
                Cluster.fOpen = false;
            }

            Cluster.fOpen        = true;
            Cluster.uID          = CurSeg.cClusters;
            Cluster.tcAbsStartMs = mapAbsPTSMs;
            Cluster.offStart     = RTFileTell(getFile());
            Cluster.cBlocks      = 0;

            Log2Func(("[C%RU64] Start @ %RU64ms (map TC is %RU64) / %RU64 bytes\n",
                      Cluster.uID, Cluster.tcAbsStartMs, mapAbsPTSMs, Cluster.offStart));

            if (CurSeg.cClusters)
                AssertMsg(Cluster.tcAbsStartMs, ("[C%RU64] @ %RU64 starting timecode is 0 which is invalid\n",
                                                 Cluster.uID, Cluster.offStart));

            WebMTracks::iterator itTrack = CurSeg.mapTracks.begin();
            while (itTrack != CurSeg.mapTracks.end())
            {
                /* Insert cue points for all tracks if a new cluster has been started. */
                WebMCuePoint cue(itTrack->second /* pTrack */, Cluster.offStart, Cluster.tcAbsStartMs);
                CurSeg.lstCues.push_back(cue);

                ++itTrack;
            }

            subStart(MkvElem_Cluster)
                .serializeUnsignedInteger(MkvElem_Timecode, Cluster.tcAbsStartMs);

            CurSeg.cClusters++;

            mapBlocks.fClusterStarted = true;
        }

        /* Iterate through all blocks related to the current timecode. */
        while (!mapBlocks.Queue.empty())
        {
            WebMSimpleBlock *pBlock = mapBlocks.Queue.front();
            AssertPtr(pBlock);

            WebMTrack       *pTrack = pBlock->pTrack;
            AssertPtr(pTrack);

            /* Calculate the block's relative time code to the current cluster's starting time code. */
            Assert(pBlock->Data.tcAbsPTSMs >= Cluster.tcAbsStartMs);
            pBlock->Data.tcRelToClusterMs = pBlock->Data.tcAbsPTSMs - Cluster.tcAbsStartMs;

            int rc2 = writeSimpleBlockEBML(pTrack, pBlock);
            AssertRC(rc2);

            Cluster.cBlocks++;

            pTrack->cTotalBlocks++;
            pTrack->tcAbsLastWrittenMs = pBlock->Data.tcAbsPTSMs;

            if (CurSeg.tcAbsLastWrittenMs < pTrack->tcAbsLastWrittenMs)
                CurSeg.tcAbsLastWrittenMs = pTrack->tcAbsLastWrittenMs;

            /* Save a cue point if this is a keyframe (if no new cluster has been started,
             * as this implies that a cue point already is present. */
            if (   !fClusterStart
                && (pBlock->Data.fFlags & VBOX_WEBM_BLOCK_FLAG_KEY_FRAME))
            {
                WebMCuePoint cue(pBlock->pTrack, Cluster.offStart, Cluster.tcAbsStartMs);
                CurSeg.lstCues.push_back(cue);
            }

            delete pBlock;
            pBlock = NULL;

            mapBlocks.Queue.pop();
        }

        Assert(mapBlocks.Queue.empty());

        CurSeg.queueBlocks.Map.erase(it);

        it = CurSeg.queueBlocks.Map.begin();
    }

    Assert(CurSeg.queueBlocks.Map.empty());

    pQueue->tsLastProcessedMs = RTTimeMilliTS();

    return VINF_SUCCESS;
}

/**
 * Writes the WebM footer.
 *
 * @returns IPRT status code.
 */
int WebMWriter::writeFooter(void)
{
    AssertReturn(isOpen(), VERR_WRONG_ORDER);

    if (m_fInTracksSection)
    {
        subEnd(MkvElem_Tracks);
        m_fInTracksSection = false;
    }

    if (CurSeg.CurCluster.fOpen)
    {
        subEnd(MkvElem_Cluster);
        CurSeg.CurCluster.fOpen = false;
    }

    /*
     * Write Cues element.
     */
    CurSeg.offCues = RTFileTell(getFile());
    LogFunc(("Cues @ %RU64\n", CurSeg.offCues));

    subStart(MkvElem_Cues);

    std::list<WebMCuePoint>::iterator itCuePoint = CurSeg.lstCues.begin();
    while (itCuePoint != CurSeg.lstCues.end())
    {
        /* Sanity. */
        AssertPtr(itCuePoint->pTrack);

        LogFunc(("CuePoint @ %RU64: Track #%RU8 (Cluster @ %RU64, tcAbs=%RU64)\n",
                 RTFileTell(getFile()), itCuePoint->pTrack->uTrack,
                 itCuePoint->offCluster, itCuePoint->tcAbs));

        subStart(MkvElem_CuePoint)
            .serializeUnsignedInteger(MkvElem_CueTime,                itCuePoint->tcAbs)
            .subStart(MkvElem_CueTrackPositions)
                .serializeUnsignedInteger(MkvElem_CueTrack,           itCuePoint->pTrack->uTrack)
                .serializeUnsignedInteger(MkvElem_CueClusterPosition, itCuePoint->offCluster - CurSeg.offStart, 8)
                .subEnd(MkvElem_CueTrackPositions)
            .subEnd(MkvElem_CuePoint);

        itCuePoint++;
    }

    subEnd(MkvElem_Cues);
    subEnd(MkvElem_Segment);

    /*
     * Re-Update seek header with final information.
     */

    writeSeekHeader();

    return RTFileSeek(getFile(), 0, RTFILE_SEEK_END, NULL);
}

/**
 * Writes the segment's seek header.
 */
void WebMWriter::writeSeekHeader(void)
{
    if (CurSeg.offSeekInfo)
        RTFileSeek(getFile(), CurSeg.offSeekInfo, RTFILE_SEEK_BEGIN, NULL);
    else
        CurSeg.offSeekInfo = RTFileTell(getFile());

    LogFunc(("Seek Headeder @ %RU64\n", CurSeg.offSeekInfo));

    subStart(MkvElem_SeekHead);

    subStart(MkvElem_Seek)
          .serializeUnsignedInteger(MkvElem_SeekID, MkvElem_Tracks)
          .serializeUnsignedInteger(MkvElem_SeekPosition, CurSeg.offTracks - CurSeg.offStart, 8)
          .subEnd(MkvElem_Seek);

    if (CurSeg.offCues)
        LogFunc(("Updating Cues @ %RU64\n", CurSeg.offCues));

    subStart(MkvElem_Seek)
          .serializeUnsignedInteger(MkvElem_SeekID, MkvElem_Cues)
          .serializeUnsignedInteger(MkvElem_SeekPosition, CurSeg.offCues - CurSeg.offStart, 8)
          .subEnd(MkvElem_Seek);

    subStart(MkvElem_Seek)
          .serializeUnsignedInteger(MkvElem_SeekID, MkvElem_Info)
          .serializeUnsignedInteger(MkvElem_SeekPosition, CurSeg.offInfo - CurSeg.offStart, 8)
          .subEnd(MkvElem_Seek);

    subEnd(MkvElem_SeekHead);

    /*
     * Write the segment's info element.
     */

    /* Save offset of the segment's info element. */
    CurSeg.offInfo = RTFileTell(getFile());

    LogFunc(("Info @ %RU64\n", CurSeg.offInfo));

    char szMux[64];
    RTStrPrintf(szMux, sizeof(szMux),
#ifdef VBOX_WITH_LIBVPX
                 "vpxenc%s", vpx_codec_version_str());
#else
                 "unknown");
#endif
    char szApp[64];
    RTStrPrintf(szApp, sizeof(szApp), VBOX_PRODUCT " %sr%u", VBOX_VERSION_STRING, RTBldCfgRevision());

    const WebMTimecodeAbs tcAbsDurationMs = CurSeg.tcAbsLastWrittenMs - CurSeg.tcAbsStartMs;

    if (!CurSeg.lstCues.empty())
    {
        LogFunc(("tcAbsDurationMs=%RU64\n", tcAbsDurationMs));
        AssertMsg(tcAbsDurationMs, ("Segment seems to be empty (duration is 0)\n"));
    }

    subStart(MkvElem_Info)
        .serializeUnsignedInteger(MkvElem_TimecodeScale, CurSeg.uTimecodeScaleFactor)
        .serializeFloat(MkvElem_Segment_Duration, tcAbsDurationMs)
        .serializeString(MkvElem_MuxingApp, szMux)
        .serializeString(MkvElem_WritingApp, szApp)
        .subEnd(MkvElem_Info);
}

