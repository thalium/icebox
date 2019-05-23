/* $Id: HDAStream.h $ */
/** @file
 * HDAStream.h - Stream functions for HD Audio.
 */

/*
 * Copyright (C) 2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef HDA_STREAM_H
#define HDA_STREAM_H

#include "DevHDACommon.h"

#include "HDAStreamMap.h"
#include "HDAStreamPeriod.h"

/*********************************************************************************************************************************
*   Prototypes                                                                                                                   *
*********************************************************************************************************************************/

typedef struct HDAMIXERSINK *PHDAMIXERSINK;

#ifdef VBOX_WITH_AUDIO_HDA_ASYNC_IO
/**
 * Structure keeping the HDA stream's state for asynchronous I/O.
 */
typedef struct HDASTREAMSTATEAIO
{
    /** Thread handle for the actual I/O thread. */
    RTTHREAD              Thread;
    /** Event for letting the thread know there is some data to process. */
    RTSEMEVENT            Event;
    /** Critical section for synchronizing access. */
    RTCRITSECT            CritSect;
    /** Started indicator. */
    volatile bool         fStarted;
    /** Shutdown indicator. */
    volatile bool         fShutdown;
    /** Whether the thread should do any data processing or not. */
    volatile bool         fEnabled;
    uint32_t              Padding1;
} HDASTREAMSTATEAIO, *PHDASTREAMSTATEAIO;
#endif

#if defined (DEBUG) || defined(HDA_USE_DMA_ACCESS_HANDLER)
typedef struct HDASTREAMDBGINFO
{
    /** Critical section to serialize access if needed. */
    RTCRITSECT              CritSect;
    uint32_t                Padding1[2];
    /** Number of total read accesses. */
    uint64_t                cReadsTotal;
    /** Number of total DMA bytes read. */
    uint64_t                cbReadTotal;
    /** Timestamp (in ns) of last read access. */
    uint64_t                tsLastReadNs;
    /** Number of total write accesses. */
    uint64_t                cWritesTotal;
    /** Number of total DMA bytes written. */
    uint64_t                cbWrittenTotal;
    /** Number of total write accesses since last iteration (Hz). */
    uint64_t                cWritesHz;
    /** Number of total DMA bytes written since last iteration (Hz). */
    uint64_t                cbWrittenHz;
    /** Timestamp (in ns) of beginning a new write slot. */
    uint64_t                tsWriteSlotBegin;
    /** Number of current silence samples in a (consecutive) row. */
    uint64_t                csSilence;
    /** Number of silent samples in a row to consider an audio block as audio gap (silence). */
    uint64_t                cSilenceThreshold;
    /** How many bytes to skip in an audio stream before detecting silence.
     *  (useful for intros and silence at the beginning of a song). */
    uint64_t                cbSilenceReadMin;
} HDASTREAMDBGINFO ,*PHDASTREAMDBGINFO;
#endif /* defined (DEBUG) || defined(HDA_USE_DMA_ACCESS_HANDLER) */

/**
 * Internal state of a HDA stream.
 */
typedef struct HDASTREAMSTATE
{
    /** Current BDLE to use. Wraps around to 0 if
     *  maximum (cBDLE) is reached. */
    uint16_t                uCurBDLE;
    /** Flag indicating whether this stream currently is
     *  in reset mode and therefore not acccessible by the guest. */
    volatile bool           fInReset;
    /** Unused, padding. */
    uint32_t                Padding0;
    /** Critical section to serialize access. */
    RTCRITSECT              CritSect;
#ifdef VBOX_WITH_AUDIO_HDA_ASYNC_IO
    /** Asynchronous I/O state members. */
    HDASTREAMSTATEAIO       AIO;
#endif
    /** This stream's data mapping. */
    HDASTREAMMAPPING        Mapping;
    /** Current BDLE (Buffer Descriptor List Entry). */
    HDABDLE                 BDLE;
    /** Circular buffer (FIFO) for holding DMA'ed data. */
    R3PTRTYPE(PRTCIRCBUF)   pCircBuf;
    /** Timestamp of the last success DMA data transfer.
     *  Used to calculate the time actually elapsed between two transfers. */
    uint64_t                uTimerTS;
    /** The stream's period. Need for timing. */
    HDASTREAMPERIOD         Period;
    /** The stream's current configuration.
     *  Should match SDFMT. */
    PDMAUDIOSTREAMCFG       Cfg;
#ifdef HDA_USE_DMA_ACCESS_HANDLER
    /** List of DMA handlers. */
    RTLISTANCHORR3          lstDMAHandlers;
#endif
    /** Unused, padding. */
    uint8_t                 Padding1[3];
} HDASTREAMSTATE, *PHDASTREAMSTATE;

/**
 * Structure for keeping a HDA stream (SDI / SDO).
 *
 * Note: This HDA stream has nothing to do with a regular audio stream handled
 * by the audio connector or the audio mixer. This HDA stream is a serial data in/out
 * stream (SDI/SDO) defined in hardware and can contain multiple audio streams
 * in one single SDI/SDO (interleaving streams).
 *
 * How a specific SDI/SDO is mapped to our internal audio streams relies on the
 * stream channel mappings.
 *
 * Contains only register values which do *not* change until a
 * stream reset occurs.
 */
typedef struct HDASTREAM
{
    /** Stream descriptor number (SDn). */
    uint8_t                  u8SD;
    uint8_t                  Padding0[7];
    /** DMA base address (SDnBDPU - SDnBDPL). */
    uint64_t                 u64BDLBase;
    /** Cyclic Buffer Length (SDnCBL).
     *  Represents the size of the ring buffer. */
    uint32_t                 u32CBL;
    /** Format (SDnFMT). */
    uint16_t                 u16FMT;
    /** FIFO Size (FIFOS).
     *  Maximum number of bytes that may have been DMA'd into
     *  memory but not yet transmitted on the link. */
    uint16_t                 u16FIFOS;
    /** FIFO Watermark. */
    uint16_t                 u16FIFOW;
    /** Last Valid Index (SDnLVI). */
    uint16_t                 u16LVI;
    uint16_t                 Padding1[2];
    /** Pointer to the HDA state this stream is attached to. */
    R3PTRTYPE(PHDASTATE)     pHDAState;
    /** Pointer to HDA sink this stream is attached to. */
    R3PTRTYPE(PHDAMIXERSINK) pMixSink;
    /** Internal state of this stream. */
    HDASTREAMSTATE           State;
#ifdef DEBUG
    /** Debug information. */
    HDASTREAMDBGINFO         Dbg;
#endif
} HDASTREAM, *PHDASTREAM;

#ifdef VBOX_WITH_AUDIO_HDA_ASYNC_IO
/**
 * Structure for keeping a HDA stream thread context.
 */
typedef struct HDASTREAMTHREADCTX
{
    PHDASTATE  pThis;
    PHDASTREAM pStream;
} HDASTREAMTHREADCTX, *PHDASTREAMTHREADCTX;
#endif

#ifdef IN_RING3

/** @name Stream functions.
 * @{
 */
int               hdaStreamCreate(PHDASTREAM pStream, PHDASTATE pThis);
void              hdaStreamDestroy(PHDASTREAM pStream);
int               hdaStreamInit(PHDASTREAM pStream, uint8_t uSD);
void              hdaStreamReset(PHDASTATE pThis, PHDASTREAM pStream, uint8_t uSD);
int               hdaStreamEnable(PHDASTREAM pStream, bool fEnable);
uint32_t          hdaStreamGetUsed(PHDASTREAM pStream);
uint32_t          hdaStreamGetFree(PHDASTREAM pStream);
int               hdaStreamTransfer(PHDASTREAM pStream, uint32_t cbToProcessMax);
uint32_t          hdaStreamUpdateLPIB(PHDASTREAM pStream, uint32_t u32LPIB);
void              hdaStreamLock(PHDASTREAM pStream);
void              hdaStreamUnlock(PHDASTREAM pStream);
int               hdaStreamRead(PHDASTREAM pStream, uint32_t cbToRead, uint32_t *pcbRead);
int               hdaStreamWrite(PHDASTREAM pStream, uint32_t cbToWrite, uint32_t *pcbWritten);
void              hdaStreamUpdate(PHDASTREAM pStream, bool fAsync);
# ifdef HDA_USE_DMA_ACCESS_HANDLER
bool              hdaStreamRegisterDMAHandlers(PHDASTREAM pStream);
void              hdaStreamUnregisterDMAHandlers(PHDASTREAM pStream);
# endif /* HDA_USE_DMA_ACCESS_HANDLER */
/** @} */

/** @name Async I/O stream functions.
 * @{
 */
# ifdef VBOX_WITH_AUDIO_HDA_ASYNC_IO
DECLCALLBACK(int) hdaStreamAsyncIOThread(RTTHREAD hThreadSelf, void *pvUser);
int               hdaStreamAsyncIOCreate(PHDASTREAM pStream);
int               hdaStreamAsyncIODestroy(PHDASTREAM pStream);
int               hdaStreamAsyncIONotify(PHDASTREAM pStream);
void              hdaStreamAsyncIOLock(PHDASTREAM pStream);
void              hdaStreamAsyncIOUnlock(PHDASTREAM pStream);
void              hdaStreamAsyncIOEnable(PHDASTREAM pStream, bool fEnable);
# endif /* VBOX_WITH_AUDIO_HDA_ASYNC_IO */
/** @} */

#endif /* IN_RING3 */

#endif /* !HDA_STREAM_H */

