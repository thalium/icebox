/* $Id: AudioMixer.h $ */
/** @file
 * VBox audio - Mixing routines.
 *
 * The mixing routines are mainly used by the various audio device emulations
 * to achieve proper multiplexing from/to attached devices LUNs.
 */

/*
 * Copyright (C) 2014-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBOX_INCLUDED_SRC_Audio_AudioMixer_h
#define VBOX_INCLUDED_SRC_Audio_AudioMixer_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/critsect.h>

#include <VBox/vmm/pdmaudioifs.h>

/**
 * Structure for maintaining an audio mixer instance.
 */
typedef struct AUDIOMIXER
{
    /** The mixer's name. */
    char                   *pszName;
    /** The mixer's critical section. */
    RTCRITSECT              CritSect;
    /** The master volume of this mixer. */
    PDMAUDIOVOLUME          VolMaster;
    /** List of audio mixer sinks. */
    RTLISTANCHOR            lstSinks;
    /** Number of used audio sinks. */
    uint8_t                 cSinks;
} AUDIOMIXER, *PAUDIOMIXER;

/** Defines an audio mixer stream's flags. */
#define AUDMIXSTREAMFLAGS uint32_t

/** No flags specified. */
#define AUDMIXSTREAM_FLAG_NONE                  0

/** Prototype needed for AUDMIXSTREAM struct definition. */
typedef struct AUDMIXSINK *PAUDMIXSINK;

/**
 * Structure for maintaining an audio mixer stream.
 */
typedef struct AUDMIXSTREAM
{
    /** List node. */
    RTLISTNODE              Node;
    /** Name of this stream. */
    char                   *pszName;
    /** The streams's critical section. */
    RTCRITSECT              CritSect;
    /** Sink this stream is attached to. */
    PAUDMIXSINK             pSink;
    /** Stream flags of type AUDMIXSTREAM_FLAG_. */
    uint32_t                fFlags;
    /** Pointer to audio connector being used. */
    PPDMIAUDIOCONNECTOR     pConn;
    /** Pointer to PDM audio stream this mixer stream handles. */
    PPDMAUDIOSTREAM         pStream;
    /** Last read (recording) / written (playback) timestamp (in ns). */
    uint64_t                tsLastReadWrittenNs;
    /** The stream's circular buffer for temporarily
     *  holding (raw) device audio data. */
    PRTCIRCBUF              pCircBuf;
} AUDMIXSTREAM, *PAUDMIXSTREAM;

/** Defines an audio sink's current status. */
#define AUDMIXSINKSTS uint32_t

/** No status specified. */
#define AUDMIXSINK_STS_NONE                  0
/** The sink is active and running. */
#define AUDMIXSINK_STS_RUNNING               RT_BIT(0)
/** The sink is in a pending disable state. */
#define AUDMIXSINK_STS_PENDING_DISABLE       RT_BIT(1)
/** Dirty flag.
 *  For output sinks this means that there is data in the
 *  sink which has not been played yet.
 *  For input sinks this means that there is data in the
 *  sink which has been recorded but not transferred to the
 *  destination yet. */
#define AUDMIXSINK_STS_DIRTY                 RT_BIT(2)

/**
 * Audio mixer sink direction.
 */
typedef enum AUDMIXSINKDIR
{
    /** Unknown direction. */
    AUDMIXSINKDIR_UNKNOWN = 0,
    /** Input (capturing from a device). */
    AUDMIXSINKDIR_INPUT,
    /** Output (playing to a device). */
    AUDMIXSINKDIR_OUTPUT,
    /** The usual 32-bit hack. */
    AUDMIXSINKDIR_32BIT_HACK = 0x7fffffff
} AUDMIXSINKDIR;

/**
 * Audio mixer sink command.
 */
typedef enum AUDMIXSINKCMD
{
    /** Unknown command, do not use. */
    AUDMIXSINKCMD_UNKNOWN = 0,
    /** Enables the sink. */
    AUDMIXSINKCMD_ENABLE,
    /** Disables the sink. */
    AUDMIXSINKCMD_DISABLE,
    /** Pauses the sink. */
    AUDMIXSINKCMD_PAUSE,
    /** Resumes the sink. */
    AUDMIXSINKCMD_RESUME,
    /** Tells the sink's streams to drop all (buffered) data immediately. */
    AUDMIXSINKCMD_DROP,
    /** Hack to blow the type up to 32-bit. */
    AUDMIXSINKCMD_32BIT_HACK = 0x7fffffff
} AUDMIXSINKCMD;

/**
 * Structure for keeping audio input sink specifics.
 * Do not use directly. Instead, use AUDMIXSINK.
 */
typedef struct AUDMIXSINKIN
{
    /** The current recording source. Can be NULL if not set. */
    PAUDMIXSTREAM  pStreamRecSource;
} AUDMIXSINKIN;

/**
 * Structure for keeping audio output sink specifics.
 * Do not use directly. Instead, use AUDMIXSINK.
 */
typedef struct AUDMIXSINKOUT
{
} AUDMIXSINKOUT;

/**
 * Structure for maintaining an audio mixer sink.
 */
typedef struct AUDMIXSINK
{
    RTLISTNODE              Node;
    /** Pointer to mixer object this sink is bound to. */
    PAUDIOMIXER             pParent;
    /** Name of this sink. */
    char                   *pszName;
    /** The sink direction, that is,
     *  if this sink handles input or output. */
    AUDMIXSINKDIR           enmDir;
    /** The sink's critical section. */
    RTCRITSECT              CritSect;
    /** This sink's mixing buffer, acting as
     * a parent buffer for all streams this sink owns. */
    PDMAUDIOMIXBUF          MixBuf;
    /** Union for input/output specifics. */
    union
    {
        AUDMIXSINKIN        In;
        AUDMIXSINKOUT       Out;
    };
    /** Sink status of type AUDMIXSINK_STS_XXX. */
    AUDMIXSINKSTS           fStatus;
    /** The sink's PCM format. */
    PDMAUDIOPCMPROPS        PCMProps;
    /** Number of streams assigned. */
    uint8_t                 cStreams;
    /** List of assigned streams.
     *  Note: All streams have the same PCM properties, so the
     *        mixer does not do any conversion. */
    /** @todo Use something faster -- vector maybe? */
    RTLISTANCHOR            lstStreams;
    /** The volume of this sink. The volume always will
     *  be combined with the mixer's master volume. */
    PDMAUDIOVOLUME          Volume;
    /** The volume of this sink, combined with the last set  master volume. */
    PDMAUDIOVOLUME          VolumeCombined;
    /** Timestamp since last update (in ms). */
    uint64_t                tsLastUpdatedMs;
    /** Last read (recording) / written (playback) timestamp (in ns). */
    uint64_t                tsLastReadWrittenNs;
#ifdef VBOX_AUDIO_MIXER_DEBUG
    struct
    {
        PPDMAUDIOFILE       pFile;
    } Dbg;
#endif
} AUDMIXSINK, *PAUDMIXSINK;

/**
 * Audio mixer operation.
 */
typedef enum AUDMIXOP
{
    /** Invalid operation, do not use. */
    AUDMIXOP_INVALID = 0,
    /** Copy data from A to B, overwriting data in B. */
    AUDMIXOP_COPY,
    /** Blend data from A with (existing) data in B. */
    AUDMIXOP_BLEND,
    /** The usual 32-bit hack. */
    AUDMIXOP_32BIT_HACK = 0x7fffffff
} AUDMIXOP;

/** No flags specified. */
#define AUDMIXSTRMCTL_FLAG_NONE         0

int AudioMixerCreate(const char *pszName, uint32_t uFlags, PAUDIOMIXER *ppMixer);
int AudioMixerCreateSink(PAUDIOMIXER pMixer, const char *pszName, AUDMIXSINKDIR enmDir, PAUDMIXSINK *ppSink);
void AudioMixerDestroy(PAUDIOMIXER pMixer);
void AudioMixerInvalidate(PAUDIOMIXER pMixer);
void AudioMixerRemoveSink(PAUDIOMIXER pMixer, PAUDMIXSINK pSink);
int AudioMixerSetMasterVolume(PAUDIOMIXER pMixer, PPDMAUDIOVOLUME pVol);
void AudioMixerDebug(PAUDIOMIXER pMixer, PCDBGFINFOHLP pHlp, const char *pszArgs);

int AudioMixerSinkAddStream(PAUDMIXSINK pSink, PAUDMIXSTREAM pStream);
int AudioMixerSinkCreateStream(PAUDMIXSINK pSink, PPDMIAUDIOCONNECTOR pConnector, PPDMAUDIOSTREAMCFG pCfg, AUDMIXSTREAMFLAGS fFlags, PAUDMIXSTREAM *ppStream);
int AudioMixerSinkCtl(PAUDMIXSINK pSink, AUDMIXSINKCMD enmCmd);
void AudioMixerSinkDestroy(PAUDMIXSINK pSink);
uint32_t AudioMixerSinkGetReadable(PAUDMIXSINK pSink);
uint32_t AudioMixerSinkGetWritable(PAUDMIXSINK pSink);
AUDMIXSINKDIR AudioMixerSinkGetDir(PAUDMIXSINK pSink);
const char *AudioMixerSinkGetName(const PAUDMIXSINK pSink);
PAUDMIXSTREAM AudioMixerSinkGetRecordingSource(PAUDMIXSINK pSink);
PAUDMIXSTREAM AudioMixerSinkGetStream(PAUDMIXSINK pSink, uint8_t uIndex);
AUDMIXSINKSTS AudioMixerSinkGetStatus(PAUDMIXSINK pSink);
uint8_t AudioMixerSinkGetStreamCount(PAUDMIXSINK pSink);
bool AudioMixerSinkIsActive(PAUDMIXSINK pSink);
int AudioMixerSinkRead(PAUDMIXSINK pSink, AUDMIXOP enmOp, void *pvBuf, uint32_t cbBuf, uint32_t *pcbRead);
void AudioMixerSinkRemoveStream(PAUDMIXSINK pSink, PAUDMIXSTREAM pStream);
void AudioMixerSinkRemoveAllStreams(PAUDMIXSINK pSink);
void AudioMixerSinkReset(PAUDMIXSINK pSink);
void AudioMixerSinkGetFormat(PAUDMIXSINK pSink, PPDMAUDIOPCMPROPS pPCMProps);
int AudioMixerSinkSetFormat(PAUDMIXSINK pSink, PPDMAUDIOPCMPROPS pPCMProps);
int AudioMixerSinkSetRecordingSource(PAUDMIXSINK pSink, PAUDMIXSTREAM pStream);
int AudioMixerSinkSetVolume(PAUDMIXSINK pSink, PPDMAUDIOVOLUME pVol);
int AudioMixerSinkWrite(PAUDMIXSINK pSink, AUDMIXOP enmOp, const void *pvBuf, uint32_t cbBuf, uint32_t *pcbWritten);
int AudioMixerSinkUpdate(PAUDMIXSINK pSink);

int AudioMixerStreamCtl(PAUDMIXSTREAM pStream, PDMAUDIOSTREAMCMD enmCmd, uint32_t fCtl);
void AudioMixerStreamDestroy(PAUDMIXSTREAM pStream);
bool AudioMixerStreamIsActive(PAUDMIXSTREAM pStream);
bool AudioMixerStreamIsValid(PAUDMIXSTREAM pStream);

#endif /* !VBOX_INCLUDED_SRC_Audio_AudioMixer_h */

