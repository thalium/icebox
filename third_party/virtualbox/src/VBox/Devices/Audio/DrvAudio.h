/* $Id: DrvAudio.h $ */
/** @file
 * Intermediate audio driver header.
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

#ifndef VBOX_INCLUDED_SRC_Audio_DrvAudio_h
#define VBOX_INCLUDED_SRC_Audio_DrvAudio_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <limits.h>

#include <iprt/circbuf.h>
#include <iprt/critsect.h>
#include <iprt/file.h>
#include <iprt/path.h>

#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdm.h>
#include <VBox/vmm/pdmaudioifs.h>

typedef enum
{
    AUD_OPT_INT,
    AUD_OPT_FMT,
    AUD_OPT_STR,
    AUD_OPT_BOOL
} audio_option_tag_e;

typedef struct audio_option
{
    const char *name;
    audio_option_tag_e tag;
    void *valp;
    const char *descr;
    int *overridenp;
    int overriden;
} audio_option;

#ifdef VBOX_WITH_STATISTICS
/**
 * Structure for keeping stream statistics for the
 * statistic manager (STAM).
 */
typedef struct DRVAUDIOSTATS
{
    STAMCOUNTER TotalStreamsActive;
    STAMCOUNTER TotalStreamsCreated;
    STAMCOUNTER TotalFramesRead;
    STAMCOUNTER TotalFramesWritten;
    STAMCOUNTER TotalFramesMixedIn;
    STAMCOUNTER TotalFramesMixedOut;
    STAMCOUNTER TotalFramesLostIn;
    STAMCOUNTER TotalFramesLostOut;
    STAMCOUNTER TotalFramesOut;
    STAMCOUNTER TotalFramesIn;
    STAMCOUNTER TotalBytesRead;
    STAMCOUNTER TotalBytesWritten;
    /** How much delay (in ms) for input processing. */
    STAMPROFILEADV DelayIn;
    /** How much delay (in ms) for output processing. */
    STAMPROFILEADV DelayOut;
} DRVAUDIOSTATS, *PDRVAUDIOSTATS;
#endif

/**
 * Audio driver configuration data, tweakable via CFGM.
 */
typedef struct DRVAUDIOCFG
{
    /** Configures the period size (in ms).
     *  This value reflects the time in between each hardware interrupt on the
     *  backend (host) side. */
    uint32_t             uPeriodSizeMs;
    /** Configures the (ring) buffer size (in ms). Often is a multiple of uPeriodMs. */
    uint32_t             uBufferSizeMs;
    /** Configures the pre-buffering size (in ms).
     *  Time needed in buffer before the stream becomes active (pre buffering).
     *  The bigger this value is, the more latency for the stream will occur.
     *  Set to 0 to disable pre-buffering completely.
     *  By default set to UINT32_MAX if not set to a custom value. */
    uint32_t             uPreBufSizeMs;
    /** The driver's debugging configuration. */
    struct
    {
        /** Whether audio debugging is enabled or not. */
        bool             fEnabled;
        /** Where to store the debugging files.
         *  Defaults to VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH if not set. */
        char             szPathOut[RTPATH_MAX + 1];
    } Dbg;
} DRVAUDIOCFG, *PDRVAUDIOCFG;

/**
 * Audio driver instance data.
 *
 * @implements PDMIAUDIOCONNECTOR
 */
typedef struct DRVAUDIO
{
    /** Friendly name of the driver. */
    char                    szName[64];
    /** Critical section for serializing access. */
    RTCRITSECT              CritSect;
    /** Shutdown indicator. */
    bool                    fTerminate;
    /** Our audio connector interface. */
    PDMIAUDIOCONNECTOR      IAudioConnector;
    /** Pointer to the driver instance. */
    PPDMDRVINS              pDrvIns;
    /** Pointer to audio driver below us. */
    PPDMIHOSTAUDIO          pHostDrvAudio;
    /** Pointer to CFGM configuration node of this driver. */
    PCFGMNODE               pCFGMNode;
    /** List of audio streams. */
    RTLISTANCHOR            lstStreams;
#ifdef VBOX_WITH_AUDIO_ENUM
    /** Flag indicating to perform an (re-)enumeration of the host audio devices. */
    bool                    fEnumerateDevices;
#endif
    /** Audio configuration settings retrieved from the backend. */
    PDMAUDIOBACKENDCFG      BackendCfg;
#ifdef VBOX_WITH_STATISTICS
    /** Statistics for the statistics manager (STAM). */
    DRVAUDIOSTATS           Stats;
#endif
    struct
    {
        /** Whether this driver's input streams are enabled or not.
         *  This flag overrides all the attached stream statuses. */
        bool                fEnabled;
        /** Max. number of free input streams.
         *  UINT32_MAX for unlimited streams. */
        uint32_t            cStreamsFree;
#ifdef VBOX_WITH_AUDIO_CALLBACKS
        RTLISTANCHOR        lstCB;
#endif
        /** The driver's input confguration (tweakable via CFGM). */
        DRVAUDIOCFG         Cfg;
    } In;
    struct
    {
        /** Whether this driver's output streams are enabled or not.
         *  This flag overrides all the attached stream statuses. */
        bool                fEnabled;
        /** Max. number of free output streams.
         *  UINT32_MAX for unlimited streams. */
        uint32_t            cStreamsFree;
#ifdef VBOX_WITH_AUDIO_CALLBACKS
        RTLISTANCHOR        lstCB;
#endif
        /** The driver's output confguration (tweakable via CFGM). */
        DRVAUDIOCFG         Cfg;
    } Out;
} DRVAUDIO, *PDRVAUDIO;

/** Makes a PDRVAUDIO out of a PPDMIAUDIOCONNECTOR. */
#define PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface) \
    ( (PDRVAUDIO)((uintptr_t)pInterface - RT_UOFFSETOF(DRVAUDIO, IAudioConnector)) )

/** @name Audio format helper methods.
 * @{ */
const char *DrvAudioHlpAudDirToStr(PDMAUDIODIR enmDir);
const char *DrvAudioHlpAudFmtToStr(PDMAUDIOFMT enmFmt);
bool DrvAudioHlpAudFmtIsSigned(PDMAUDIOFMT enmFmt);
uint8_t DrvAudioHlpAudFmtToBits(PDMAUDIOFMT enmFmt);
/** @}  */

/** @name Audio calculation helper methods.
 * @{ */
void DrvAudioHlpClearBuf(const PPDMAUDIOPCMPROPS pPCMProps, void *pvBuf, size_t cbBuf, uint32_t cFrames);
uint32_t DrvAudioHlpCalcBitrate(uint8_t cBits, uint32_t uHz, uint8_t cChannels);
uint32_t DrvAudioHlpCalcBitrate(const PPDMAUDIOPCMPROPS pProps);
uint32_t DrvAudioHlpBytesAlign(uint32_t cbSize, const PPDMAUDIOPCMPROPS pProps);
bool     DrvAudioHlpBytesIsAligned(uint32_t cbSize, const PPDMAUDIOPCMPROPS pProps);
uint32_t DrvAudioHlpBytesToFrames(uint32_t cbBytes, const PPDMAUDIOPCMPROPS pProps);
uint64_t DrvAudioHlpBytesToMilli(uint32_t cbBytes, const PPDMAUDIOPCMPROPS pProps);
uint64_t DrvAudioHlpBytesToMicro(uint32_t cbBytes, const PPDMAUDIOPCMPROPS pProps);
uint64_t DrvAudioHlpBytesToNano(uint32_t cbBytes, const PPDMAUDIOPCMPROPS pProps);
uint32_t DrvAudioHlpFramesToBytes(uint32_t cFrames, const PPDMAUDIOPCMPROPS pProps);
uint64_t DrvAudioHlpFramesToMilli(uint32_t cFrames, const PPDMAUDIOPCMPROPS pProps);
uint64_t DrvAudioHlpFramesToNano(uint32_t cFrames, const PPDMAUDIOPCMPROPS pProps);
uint32_t DrvAudioHlpMilliToBytes(uint64_t uMs, const PPDMAUDIOPCMPROPS pProps);
uint32_t DrvAudioHlpNanoToBytes(uint64_t uNs, const PPDMAUDIOPCMPROPS pProps);
uint32_t DrvAudioHlpMilliToFrames(uint64_t uMs, const PPDMAUDIOPCMPROPS pProps);
uint32_t DrvAudioHlpNanoToFrames(uint64_t uNs, const PPDMAUDIOPCMPROPS pProps);
/** @}  */

/** @name Audio PCM properties helper methods.
 * @{ */
bool DrvAudioHlpPCMPropsAreEqual(const PPDMAUDIOPCMPROPS pPCMProps1, const PPDMAUDIOPCMPROPS pPCMProps2);
bool DrvAudioHlpPCMPropsAreEqual(const PPDMAUDIOPCMPROPS pPCMProps, const PPDMAUDIOSTREAMCFG pCfg);
bool DrvAudioHlpPCMPropsAreValid(const PPDMAUDIOPCMPROPS pProps);
uint32_t DrvAudioHlpPCMPropsBytesPerFrame(const PPDMAUDIOPCMPROPS pProps);
void DrvAudioHlpPCMPropsPrint(const PPDMAUDIOPCMPROPS pProps);
int DrvAudioHlpPCMPropsToStreamCfg(const PPDMAUDIOPCMPROPS pPCMProps, PPDMAUDIOSTREAMCFG pCfg);
/** @}  */

/** @name Audio configuration helper methods.
 * @{ */
void DrvAudioHlpStreamCfgInit(PPDMAUDIOSTREAMCFG pCfg);
bool DrvAudioHlpStreamCfgIsValid(const PPDMAUDIOSTREAMCFG pCfg);
int DrvAudioHlpStreamCfgCopy(PPDMAUDIOSTREAMCFG pDstCfg, const PPDMAUDIOSTREAMCFG pSrcCfg);
PPDMAUDIOSTREAMCFG DrvAudioHlpStreamCfgDup(const PPDMAUDIOSTREAMCFG pCfg);
void DrvAudioHlpStreamCfgFree(PPDMAUDIOSTREAMCFG pCfg);
void DrvAudioHlpStreamCfgPrint(const PPDMAUDIOSTREAMCFG pCfg);
/** @}  */

/** @name Audio stream command helper methods.
 * @{ */
const char *DrvAudioHlpStreamCmdToStr(PDMAUDIOSTREAMCMD enmCmd);
/** @}  */

/** @name Audio stream status helper methods.
 * @{ */
bool DrvAudioHlpStreamStatusCanRead(PDMAUDIOSTREAMSTS enmStatus);
bool DrvAudioHlpStreamStatusCanWrite(PDMAUDIOSTREAMSTS enmStatus);
bool DrvAudioHlpStreamStatusIsReady(PDMAUDIOSTREAMSTS enmStatus);
/** @}  */

/** @name Audio file (name) helper methods.
 * @{ */
int DrvAudioHlpFileNameSanitize(char *pszPath, size_t cbPath);
int DrvAudioHlpFileNameGet(char *pszFile, size_t cchFile, const char *pszPath, const char *pszName, uint32_t uInstance, PDMAUDIOFILETYPE enmType, PDMAUDIOFILENAMEFLAGS fFlags);
/** @}  */

/** @name Audio device methods.
 * @{ */
PPDMAUDIODEVICE DrvAudioHlpDeviceAlloc(size_t cbData);
void DrvAudioHlpDeviceFree(PPDMAUDIODEVICE pDev);
PPDMAUDIODEVICE DrvAudioHlpDeviceDup(const PPDMAUDIODEVICE pDev, bool fCopyUserData);
/** @}  */

/** @name Audio device enumartion methods.
 * @{ */
int DrvAudioHlpDeviceEnumInit(PPDMAUDIODEVICEENUM pDevEnm);
void DrvAudioHlpDeviceEnumFree(PPDMAUDIODEVICEENUM pDevEnm);
int DrvAudioHlpDeviceEnumAdd(PPDMAUDIODEVICEENUM pDevEnm, PPDMAUDIODEVICE pDev);
int DrvAudioHlpDeviceEnumCopyEx(PPDMAUDIODEVICEENUM pDstDevEnm, const PPDMAUDIODEVICEENUM pSrcDevEnm, PDMAUDIODIR enmUsage);
int DrvAudioHlpDeviceEnumCopy(PPDMAUDIODEVICEENUM pDstDevEnm, const PPDMAUDIODEVICEENUM pSrcDevEnm);
PPDMAUDIODEVICEENUM DrvAudioHlpDeviceEnumDup(const PPDMAUDIODEVICEENUM pDevEnm);
int DrvAudioHlpDeviceEnumCopy(PPDMAUDIODEVICEENUM pDstDevEnm, const PPDMAUDIODEVICEENUM pSrcDevEnm);
int DrvAudioHlpDeviceEnumCopyEx(PPDMAUDIODEVICEENUM pDstDevEnm, const PPDMAUDIODEVICEENUM pSrcDevEnm, PDMAUDIODIR enmUsage, bool fCopyUserData);
PPDMAUDIODEVICE DrvAudioHlpDeviceEnumGetDefaultDevice(const PPDMAUDIODEVICEENUM pDevEnm, PDMAUDIODIR enmDir);
uint16_t DrvAudioHlpDeviceEnumGetDeviceCount(const PPDMAUDIODEVICEENUM pDevEnm, PDMAUDIODIR enmUsage);
void DrvAudioHlpDeviceEnumPrint(const char *pszDesc, const PPDMAUDIODEVICEENUM pDevEnm);
/** @}  */

/** @name Audio string-ify methods.
 * @{ */
const char *DrvAudioHlpAudMixerCtlToStr(PDMAUDIOMIXERCTL enmMixerCtl);
const char *DrvAudioHlpPlaybackDstToStr(const PDMAUDIOPLAYBACKDEST enmPlaybackDst);
const char *DrvAudioHlpRecSrcToStr(const PDMAUDIORECSOURCE enmRecSource);
PDMAUDIOFMT DrvAudioHlpStrToAudFmt(const char *pszFmt);
char *DrvAudioHlpAudDevFlagsToStrA(PDMAUDIODEVFLAG fFlags);
/** @}  */

/** @name Audio file methods.
 * @{ */
int DrvAudioHlpFileCreate(PDMAUDIOFILETYPE enmType, const char *pszFile, PDMAUDIOFILEFLAGS fFlags, PPDMAUDIOFILE *ppFile);
void DrvAudioHlpFileDestroy(PPDMAUDIOFILE pFile);
int DrvAudioHlpFileOpen(PPDMAUDIOFILE pFile, uint32_t fOpen, const PPDMAUDIOPCMPROPS pProps);
int DrvAudioHlpFileClose(PPDMAUDIOFILE pFile);
int DrvAudioHlpFileDelete(PPDMAUDIOFILE pFile);
size_t DrvAudioHlpFileGetDataSize(PPDMAUDIOFILE pFile);
bool DrvAudioHlpFileIsOpen(PPDMAUDIOFILE pFile);
int DrvAudioHlpFileWrite(PPDMAUDIOFILE pFile, const void *pvBuf, size_t cbBuf, uint32_t fFlags);
/** @}  */

#define AUDIO_MAKE_FOURCC(c0, c1, c2, c3) RT_H2LE_U32_C(RT_MAKE_U32_FROM_U8(c0, c1, c2, c3))

#endif /* !VBOX_INCLUDED_SRC_Audio_DrvAudio_h */

