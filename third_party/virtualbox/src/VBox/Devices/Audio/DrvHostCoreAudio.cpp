/* $Id: DrvHostCoreAudio.cpp $ */
/** @file
 * VBox audio devices - Mac OS X CoreAudio audio driver.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_DRV_HOST_AUDIO
#include <VBox/log.h>

#include "DrvAudio.h"
#include "VBoxDD.h"

#include <iprt/asm.h>
#include <iprt/cdefs.h>
#include <iprt/circbuf.h>
#include <iprt/mem.h>

#include <iprt/uuid.h>

#include <CoreAudio/CoreAudio.h>
#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioConverter.h>
#include <AudioToolbox/AudioToolbox.h>


/* Audio Queue buffer configuration. */
#define AQ_BUF_COUNT    32      /* Number of buffers. */
#define AQ_BUF_SIZE     512     /* Size of each buffer in bytes. */
#define AQ_BUF_TOTAL    (AQ_BUF_COUNT * AQ_BUF_SIZE)
#define AQ_BUF_SAMPLES  (AQ_BUF_TOTAL / 4)  /* Hardcoded 4 bytes per sample! */

/* Enables utilizing the Core Audio converter unit for converting
 * input / output from/to our requested formats. That might be more
 * performant than using our own routines later down the road. */
/** @todo Needs more investigation and testing first before enabling. */
//# define VBOX_WITH_AUDIO_CA_CONVERTER

/** @todo
 * - Maybe make sure the threads are immediately stopped if playing/recording stops.
 */

/*
 * Most of this is based on:
 * http://developer.apple.com/mac/library/technotes/tn2004/tn2097.html
 * http://developer.apple.com/mac/library/technotes/tn2002/tn2091.html
 * http://developer.apple.com/mac/library/qa/qa2007/qa1533.html
 * http://developer.apple.com/mac/library/qa/qa2001/qa1317.html
 * http://developer.apple.com/mac/library/documentation/AudioUnit/Reference/AUComponentServicesReference/Reference/reference.html
 */

/* Prototypes needed for COREAUDIODEVICE. */
struct DRVHOSTCOREAUDIO;
typedef struct DRVHOSTCOREAUDIO *PDRVHOSTCOREAUDIO;

/**
 * Structure for holding Core Audio-specific device data.
 * This data then lives in the pvData part of the PDMAUDIODEVICE struct.
 */
typedef struct COREAUDIODEVICEDATA
{
    /** Pointer to driver instance this device is bound to. */
    PDRVHOSTCOREAUDIO pDrv;
    /** The audio device ID of the currently used device (UInt32 typedef). */
    AudioDeviceID     deviceID;
    /** The device' UUID. */
    CFStringRef       UUID;
    /** List of attached (native) Core Audio streams attached to this device. */
    RTLISTANCHOR      lstStreams;
} COREAUDIODEVICEDATA, *PCOREAUDIODEVICEDATA;

/**
 * Host Coreaudio driver instance data.
 * @implements PDMIAUDIOCONNECTOR
 */
typedef struct DRVHOSTCOREAUDIO
{
    /** Pointer to the driver instance structure. */
    PPDMDRVINS              pDrvIns;
    /** Pointer to host audio interface. */
    PDMIHOSTAUDIO           IHostAudio;
    /** Critical section to serialize access. */
    RTCRITSECT              CritSect;
    /** Current (last reported) device enumeration. */
    PDMAUDIODEVICEENUM      Devices;
    /** Pointer to the currently used input device in the device enumeration.
     *  Can be NULL if none assigned. */
    PPDMAUDIODEVICE         pDefaultDevIn;
    /** Pointer to the currently used output device in the device enumeration.
     *  Can be NULL if none assigned. */
    PPDMAUDIODEVICE         pDefaultDevOut;
#ifdef VBOX_WITH_AUDIO_CALLBACKS
    /** Callback function to the upper driver.
     *  Can be NULL if not being used / registered. */
    PFNPDMHOSTAUDIOCALLBACK pfnCallback;
#endif
} DRVHOSTCOREAUDIO, *PDRVHOSTCOREAUDIO;

/** Converts a pointer to DRVHOSTCOREAUDIO::IHostAudio to a PDRVHOSTCOREAUDIO. */
#define PDMIHOSTAUDIO_2_DRVHOSTCOREAUDIO(pInterface) RT_FROM_MEMBER(pInterface, DRVHOSTCOREAUDIO, IHostAudio)

/**
 * Structure for holding a Core Audio unit
 * and its data.
 */
typedef struct COREAUDIOUNIT
{
    /** Pointer to the device this audio unit is bound to.
     *  Can be NULL if not bound to a device (anymore). */
    PPDMAUDIODEVICE             pDevice;
    /** The actual audio unit object. */
    AudioUnit                   audioUnit;
    /** Stream description for using with VBox:
     *  - When using this audio unit for input (capturing), this format states
     *    the unit's output format.
     *  - When using this audio unit for output (playback), this format states
     *    the unit's input format. */
    AudioStreamBasicDescription streamFmt;
} COREAUDIOUNIT, *PCOREAUDIOUNIT;

/*******************************************************************************
 *
 * Helper function section
 *
 ******************************************************************************/

/* Move these down below the internal function prototypes... */

static void coreAudioPrintASBD(const char *pszDesc, const AudioStreamBasicDescription *pASBD)
{
    char pszSampleRate[32];
    LogRel2(("CoreAudio: %s description:\n", pszDesc));
    LogRel2(("CoreAudio:\tFormat ID: %RU32 (%c%c%c%c)\n", pASBD->mFormatID,
             RT_BYTE4(pASBD->mFormatID), RT_BYTE3(pASBD->mFormatID),
             RT_BYTE2(pASBD->mFormatID), RT_BYTE1(pASBD->mFormatID)));
    LogRel2(("CoreAudio:\tFlags: %RU32", pASBD->mFormatFlags));
    if (pASBD->mFormatFlags & kAudioFormatFlagIsFloat)
        LogRel2((" Float"));
    if (pASBD->mFormatFlags & kAudioFormatFlagIsBigEndian)
        LogRel2((" BigEndian"));
    if (pASBD->mFormatFlags & kAudioFormatFlagIsSignedInteger)
        LogRel2((" SignedInteger"));
    if (pASBD->mFormatFlags & kAudioFormatFlagIsPacked)
        LogRel2((" Packed"));
    if (pASBD->mFormatFlags & kAudioFormatFlagIsAlignedHigh)
        LogRel2((" AlignedHigh"));
    if (pASBD->mFormatFlags & kAudioFormatFlagIsNonInterleaved)
        LogRel2((" NonInterleaved"));
    if (pASBD->mFormatFlags & kAudioFormatFlagIsNonMixable)
        LogRel2((" NonMixable"));
    if (pASBD->mFormatFlags & kAudioFormatFlagsAreAllClear)
        LogRel2((" AllClear"));
    LogRel2(("\n"));
    snprintf(pszSampleRate, 32, "%.2f", (float)pASBD->mSampleRate); /** @todo r=andy Use RTStrPrint*. */
    LogRel2(("CoreAudio:\tSampleRate      : %s\n", pszSampleRate));
    LogRel2(("CoreAudio:\tChannelsPerFrame: %RU32\n", pASBD->mChannelsPerFrame));
    LogRel2(("CoreAudio:\tFramesPerPacket : %RU32\n", pASBD->mFramesPerPacket));
    LogRel2(("CoreAudio:\tBitsPerChannel  : %RU32\n", pASBD->mBitsPerChannel));
    LogRel2(("CoreAudio:\tBytesPerFrame   : %RU32\n", pASBD->mBytesPerFrame));
    LogRel2(("CoreAudio:\tBytesPerPacket  : %RU32\n", pASBD->mBytesPerPacket));
}

static void coreAudioPCMPropsToASBD(PDMAUDIOPCMPROPS *pPCMProps, AudioStreamBasicDescription *pASBD)
{
    AssertPtrReturnVoid(pPCMProps);
    AssertPtrReturnVoid(pASBD);

    RT_BZERO(pASBD, sizeof(AudioStreamBasicDescription));

    pASBD->mFormatID         = kAudioFormatLinearPCM;
    pASBD->mFormatFlags      = kAudioFormatFlagIsPacked;
    pASBD->mFramesPerPacket  = 1; /* For uncompressed audio, set this to 1. */
    pASBD->mSampleRate       = (Float64)pPCMProps->uHz;
    pASBD->mChannelsPerFrame = pPCMProps->cChannels;
    pASBD->mBitsPerChannel   = pPCMProps->cBits;
    if (pPCMProps->fSigned)
        pASBD->mFormatFlags |= kAudioFormatFlagIsSignedInteger;
    pASBD->mBytesPerFrame    = pASBD->mChannelsPerFrame * (pASBD->mBitsPerChannel / 8);
    pASBD->mBytesPerPacket   = pASBD->mFramesPerPacket * pASBD->mBytesPerFrame;
}

#ifndef VBOX_WITH_AUDIO_CALLBACKS
static int coreAudioASBDToStreamCfg(AudioStreamBasicDescription *pASBD, PPDMAUDIOSTREAMCFG pCfg)
{
    AssertPtrReturn(pASBD, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pCfg,  VERR_INVALID_PARAMETER);

    pCfg->Props.cChannels = pASBD->mChannelsPerFrame;
    pCfg->Props.uHz       = (uint32_t)pASBD->mSampleRate;
    pCfg->Props.cBits     = pASBD->mBitsPerChannel;
    pCfg->Props.fSigned   = RT_BOOL(pASBD->mFormatFlags & kAudioFormatFlagIsSignedInteger);
    pCfg->Props.cShift    = PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(pCfg->Props.cBits, pCfg->Props.cChannels);

    return VINF_SUCCESS;
}
#endif /* !VBOX_WITH_AUDIO_CALLBACKS */

#if 0 /* unused */
static int coreAudioCFStringToCString(const CFStringRef pCFString, char **ppszString)
{
    CFIndex cLen = CFStringGetLength(pCFString) + 1;
    char *pszResult = (char *)RTMemAllocZ(cLen * sizeof(char));
    if (!CFStringGetCString(pCFString, pszResult, cLen, kCFStringEncodingUTF8))
    {
        RTMemFree(pszResult);
        return VERR_NOT_FOUND;
    }

    *ppszString = pszResult;
    return VINF_SUCCESS;
}

static AudioDeviceID coreAudioDeviceUIDtoID(const char* pszUID)
{
    /* Create a CFString out of our CString. */
    CFStringRef strUID = CFStringCreateWithCString(NULL, pszUID, kCFStringEncodingMacRoman);

    /* Fill the translation structure. */
    AudioDeviceID deviceID;

    AudioValueTranslation translation;
    translation.mInputData      = &strUID;
    translation.mInputDataSize  = sizeof(CFStringRef);
    translation.mOutputData     = &deviceID;
    translation.mOutputDataSize = sizeof(AudioDeviceID);

    /* Fetch the translation from the UID to the device ID. */
    AudioObjectPropertyAddress propAdr = { kAudioHardwarePropertyDeviceForUID, kAudioObjectPropertyScopeGlobal,
                                           kAudioObjectPropertyElementMaster };

    UInt32 uSize = sizeof(AudioValueTranslation);
    OSStatus err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propAdr, 0, NULL, &uSize, &translation);

    /* Release the temporary CFString */
    CFRelease(strUID);

    if (RT_LIKELY(err == noErr))
        return deviceID;

    /* Return the unknown device on error. */
    return kAudioDeviceUnknown;
}
#endif /* unused */


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/

/** @name Initialization status indicator used for the recreation of the AudioUnits.
 *
 * Global structures section
 *
 ******************************************************************************/

/**
 * Enumeration for a Core Audio stream status.
 */
typedef enum COREAUDIOSTATUS
{
    /** The device is uninitialized. */
    COREAUDIOSTATUS_UNINIT  = 0,
    /** The device is currently initializing. */
    COREAUDIOSTATUS_IN_INIT,
    /** The device is initialized. */
    COREAUDIOSTATUS_INIT,
    /** The device is currently uninitializing. */
    COREAUDIOSTATUS_IN_UNINIT,
#ifndef VBOX_WITH_AUDIO_CALLBACKS
    /** The device has to be reinitialized.
     *  Note: Only needed if VBOX_WITH_AUDIO_CALLBACKS is not defined, as otherwise
     *        the Audio Connector will take care of this as soon as this backend
     *        tells it to do so via the provided audio callback. */
    COREAUDIOSTATUS_REINIT,
#endif
    /** The usual 32-bit hack. */
    COREAUDIOSTATUS_32BIT_HACK = 0x7fffffff
} COREAUDIOSTATUS, *PCOREAUDIOSTATUS;

#ifdef VBOX_WITH_AUDIO_CA_CONVERTER
 /* Error code which indicates "End of data" */
 static const OSStatus caConverterEOFDErr = 0x656F6664; /* 'eofd' */
#endif

/* Prototypes needed for COREAUDIOSTREAMCBCTX. */
struct COREAUDIOSTREAM;
typedef struct COREAUDIOSTREAM *PCOREAUDIOSTREAM;

/**
 * Structure for keeping a conversion callback context.
 * This is needed when using an audio converter during input/output processing.
 */
typedef struct COREAUDIOCONVCBCTX
{
    /** Pointer to the stream this context is bound to. */
    PCOREAUDIOSTREAM             pStream;
    /** Source stream description. */
    AudioStreamBasicDescription  asbdSrc;
    /** Destination stream description. */
    AudioStreamBasicDescription  asbdDst;
    /** Pointer to native buffer list used for rendering the source audio data into. */
    AudioBufferList             *pBufLstSrc;
    /** Total packet conversion count. */
    UInt32                       uPacketCnt;
    /** Current packet conversion index. */
    UInt32                       uPacketIdx;
    /** Error count, for limiting the logging. */
    UInt32                       cErrors;
} COREAUDIOCONVCBCTX, *PCOREAUDIOCONVCBCTX;

/**
 * Structure for keeping the input stream specifics.
 */
typedef struct COREAUDIOSTREAMIN
{
#ifdef VBOX_WITH_AUDIO_CA_CONVERTER
    /** The audio converter if necessary. NULL if no converter is being used. */
    AudioConverterRef           ConverterRef;
    /** Callback context for the audio converter. */
    COREAUDIOCONVCBCTX          convCbCtx;
#endif
    /** The ratio between the device & the stream sample rate. */
    Float64                     sampleRatio;
} COREAUDIOSTREAMIN, *PCOREAUDIOSTREAMIN;

/**
 * Structure for keeping the output stream specifics.
 */
typedef struct COREAUDIOSTREAMOUT
{
    /** Nothing here yet. */
} COREAUDIOSTREAMOUT, *PCOREAUDIOSTREAMOUT;

/**
 * Structure for maintaining a Core Audio stream.
 */
typedef struct COREAUDIOSTREAM
{
    /** The stream's acquired configuration. */
    PPDMAUDIOSTREAMCFG          pCfg;
    /** Stream-specific data, depending on the stream type. */
    union
    {
        COREAUDIOSTREAMIN       In;
        COREAUDIOSTREAMOUT      Out;
    };
    /** List node for the device's stream list. */
    RTLISTNODE                  Node;
    /** Pointer to driver instance this stream is bound to. */
    PDRVHOSTCOREAUDIO           pDrv;
    /** The stream's thread handle for maintaining the audio queue. */
    RTTHREAD                    hThread;
    /** Flag indicating to start a stream's data processing. */
    bool                        fRun;
    /** Whether the stream is in a running (active) state or not.
     *  For playback streams this means that audio data can be (or is being) played,
     *  for capturing streams this means that audio data is being captured (if available). */
    bool                        fIsRunning;
    /** Thread shutdown indicator. */
    bool                        fShutdown;
    /** Critical section for serializing access between thread + callbacks. */
    RTCRITSECT                  CritSect;
    /** The actual audio queue being used. */
    AudioQueueRef               audioQueue;
    /** The audio buffers which are used with the above audio queue. */
    AudioQueueBufferRef         audioBuffer[AQ_BUF_COUNT];
    /** The acquired (final) audio format for this stream. */
    AudioStreamBasicDescription asbdStream;
    /** The audio unit for this stream. */
    COREAUDIOUNIT               Unit;
    /** Initialization status tracker. Used when some of the device parameters
     *  or the device itself is changed during the runtime. */
    volatile uint32_t           enmStatus;
    /** An internal ring buffer for transferring data from/to the rendering callbacks. */
    PRTCIRCBUF                  pCircBuf;
} COREAUDIOSTREAM, *PCOREAUDIOSTREAM;

static int coreAudioStreamInit(PCOREAUDIOSTREAM pCAStream, PDRVHOSTCOREAUDIO pThis, PPDMAUDIODEVICE pDev);
#ifndef VBOX_WITH_AUDIO_CALLBACKS
static int coreAudioStreamReinit(PDRVHOSTCOREAUDIO pThis, PCOREAUDIOSTREAM pCAStream, PPDMAUDIODEVICE pDev);
#endif
static int coreAudioStreamUninit(PCOREAUDIOSTREAM pCAStream);

static int coreAudioStreamControl(PDRVHOSTCOREAUDIO pThis, PCOREAUDIOSTREAM pCAStream, PDMAUDIOSTREAMCMD enmStreamCmd);

static int coreAudioDeviceRegisterCallbacks(PDRVHOSTCOREAUDIO pThis, PPDMAUDIODEVICE pDev);
static int coreAudioDeviceUnregisterCallbacks(PDRVHOSTCOREAUDIO pThis, PPDMAUDIODEVICE pDev);
static void coreAudioDeviceDataInit(PCOREAUDIODEVICEDATA pDevData, AudioDeviceID deviceID, bool fIsInput, PDRVHOSTCOREAUDIO pDrv);

static OSStatus coreAudioDevPropChgCb(AudioObjectID propertyID, UInt32 nAddresses, const AudioObjectPropertyAddress properties[], void *pvUser);

static int coreAudioStreamInitQueue(PCOREAUDIOSTREAM pCAStream, PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq);
static void coreAudioInputQueueCb(void *pvUser, AudioQueueRef audioQueue, AudioQueueBufferRef audioBuffer, const AudioTimeStamp *pAudioTS, UInt32 cPacketDesc, const AudioStreamPacketDescription *paPacketDesc);
static void coreAudioOutputQueueCb(void *pvUser, AudioQueueRef audioQueue, AudioQueueBufferRef audioBuffer);

#ifdef VBOX_WITH_AUDIO_CA_CONVERTER
/**
 * Initializes a conversion callback context.
 *
 * @return  IPRT status code.
 * @param   pConvCbCtx          Conversion callback context to initialize.
 * @param   pStream             Pointer to stream to use.
 * @param   pASBDSrc            Input (source) stream description to use.
 * @param   pASBDDst            Output (destination) stream description to use.
 */
static int coreAudioInitConvCbCtx(PCOREAUDIOCONVCBCTX pConvCbCtx, PCOREAUDIOSTREAM pStream,
                                  AudioStreamBasicDescription *pASBDSrc, AudioStreamBasicDescription *pASBDDst)
{
    AssertPtrReturn(pConvCbCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    AssertPtrReturn(pASBDSrc,   VERR_INVALID_POINTER);
    AssertPtrReturn(pASBDDst,   VERR_INVALID_POINTER);

#ifdef DEBUG
    coreAudioPrintASBD("CbCtx: Src", pASBDSrc);
    coreAudioPrintASBD("CbCtx: Dst", pASBDDst);
#endif

    pConvCbCtx->pStream = pStream;

    memcpy(&pConvCbCtx->asbdSrc, pASBDSrc, sizeof(AudioStreamBasicDescription));
    memcpy(&pConvCbCtx->asbdDst, pASBDDst, sizeof(AudioStreamBasicDescription));

    pConvCbCtx->pBufLstSrc = NULL;
    pConvCbCtx->cErrors    = 0;

    return VINF_SUCCESS;
}


/**
 * Uninitializes a conversion callback context.
 *
 * @return  IPRT status code.
 * @param   pConvCbCtx          Conversion callback context to uninitialize.
 */
static void coreAudioUninitConvCbCtx(PCOREAUDIOCONVCBCTX pConvCbCtx)
{
    AssertPtrReturnVoid(pConvCbCtx);

    pConvCbCtx->pStream = NULL;

    RT_ZERO(pConvCbCtx->asbdSrc);
    RT_ZERO(pConvCbCtx->asbdDst);

    pConvCbCtx->pBufLstSrc = NULL;
    pConvCbCtx->cErrors    = 0;
}
#endif /* VBOX_WITH_AUDIO_CA_CONVERTER */


/**
 * Does a (re-)enumeration of the host's playback + recording devices.
 *
 * @return  IPRT status code.
 * @param   pThis               Host audio driver instance.
 * @param   enmUsage            Which devices to enumerate.
 * @param   pDevEnm             Where to store the enumerated devices.
 */
static int coreAudioDevicesEnumerate(PDRVHOSTCOREAUDIO pThis, PDMAUDIODIR enmUsage, PPDMAUDIODEVICEENUM pDevEnm)
{
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pDevEnm, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;

    do
    {
        AudioDeviceID defaultDeviceID = kAudioDeviceUnknown;

        /* Fetch the default audio device currently in use. */
        AudioObjectPropertyAddress propAdrDefaultDev = {   enmUsage == PDMAUDIODIR_IN
                                                         ? kAudioHardwarePropertyDefaultInputDevice
                                                         : kAudioHardwarePropertyDefaultOutputDevice,
                                                         kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
        UInt32 uSize = sizeof(defaultDeviceID);
        OSStatus err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propAdrDefaultDev, 0, NULL, &uSize, &defaultDeviceID);
        if (err != noErr)
        {
            LogRel(("CoreAudio: Unable to determine default %s device (%RI32)\n",
                    enmUsage == PDMAUDIODIR_IN ? "capturing" : "playback", err));
            return VERR_NOT_FOUND;
        }

        if (defaultDeviceID == kAudioDeviceUnknown)
        {
            LogFunc(("No default %s device found\n", enmUsage == PDMAUDIODIR_IN ? "capturing" : "playback"));
            /* Keep going. */
        }

        AudioObjectPropertyAddress propAdrDevList = { kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
                                                      kAudioObjectPropertyElementMaster };

        err = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &propAdrDevList, 0, NULL, &uSize);
        if (err != kAudioHardwareNoError)
            break;

        AudioDeviceID *pDevIDs = (AudioDeviceID *)alloca(uSize);
        if (pDevIDs == NULL)
            break;

        err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propAdrDevList, 0, NULL, &uSize, pDevIDs);
        if (err != kAudioHardwareNoError)
            break;

        rc = DrvAudioHlpDeviceEnumInit(pDevEnm);
        if (RT_FAILURE(rc))
            break;

        UInt16 cDevices = uSize / sizeof(AudioDeviceID);

        PPDMAUDIODEVICE pDev = NULL;
        for (UInt16 i = 0; i < cDevices; i++)
        {
            if (pDev) /* Some (skipped) device to clean up first? */
                DrvAudioHlpDeviceFree(pDev);

            pDev = DrvAudioHlpDeviceAlloc(sizeof(COREAUDIODEVICEDATA));
            if (!pDev)
            {
                rc = VERR_NO_MEMORY;
                break;
            }

            /* Set usage. */
            pDev->enmUsage = enmUsage;

            /* Init backend-specific device data. */
            PCOREAUDIODEVICEDATA pDevData = (PCOREAUDIODEVICEDATA)pDev->pvData;
            AssertPtr(pDevData);
            coreAudioDeviceDataInit(pDevData, pDevIDs[i], enmUsage == PDMAUDIODIR_IN, pThis);

            /* Check if the device is valid. */
            AudioDeviceID curDevID = pDevData->deviceID;

            /* Is the device the default device? */
            if (curDevID == defaultDeviceID)
                pDev->fFlags |= PDMAUDIODEV_FLAGS_DEFAULT;

            AudioObjectPropertyAddress propAddrCfg = { kAudioDevicePropertyStreamConfiguration,
                                                         enmUsage == PDMAUDIODIR_IN
                                                       ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput,
                                                       kAudioObjectPropertyElementMaster };

            err = AudioObjectGetPropertyDataSize(curDevID, &propAddrCfg, 0, NULL, &uSize);
            if (err != noErr)
                continue;

            AudioBufferList *pBufList = (AudioBufferList *)RTMemAlloc(uSize);
            if (!pBufList)
                continue;

            err = AudioObjectGetPropertyData(curDevID, &propAddrCfg, 0, NULL, &uSize, pBufList);
            if (err == noErr)
            {
                for (UInt32 a = 0; a < pBufList->mNumberBuffers; a++)
                {
                    if (enmUsage == PDMAUDIODIR_IN)
                        pDev->cMaxInputChannels  += pBufList->mBuffers[a].mNumberChannels;
                    else if (enmUsage == PDMAUDIODIR_OUT)
                        pDev->cMaxOutputChannels += pBufList->mBuffers[a].mNumberChannels;
                }
            }

            if (pBufList)
            {
                RTMemFree(pBufList);
                pBufList = NULL;
            }

            /* Check if the device is valid, e.g. has any input/output channels according to its usage. */
            if (   enmUsage == PDMAUDIODIR_IN
                && !pDev->cMaxInputChannels)
            {
                continue;
            }
            else if (   enmUsage == PDMAUDIODIR_OUT
                     && !pDev->cMaxOutputChannels)
            {
                continue;
            }

            /* Resolve the device's name. */
            AudioObjectPropertyAddress propAddrName = { kAudioObjectPropertyName,
                                                          enmUsage == PDMAUDIODIR_IN
                                                        ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput,
                                                        kAudioObjectPropertyElementMaster };
            uSize = sizeof(CFStringRef);
            CFStringRef pcfstrName = NULL;

            err = AudioObjectGetPropertyData(curDevID, &propAddrName, 0, NULL, &uSize, &pcfstrName);
            if (err != kAudioHardwareNoError)
                continue;

            CFIndex uMax = CFStringGetMaximumSizeForEncoding(CFStringGetLength(pcfstrName), kCFStringEncodingUTF8) + 1;
            if (uMax)
            {
                char *pszName = (char *)RTStrAlloc(uMax);
                if (   pszName
                    && CFStringGetCString(pcfstrName, pszName, uMax, kCFStringEncodingUTF8))
                {
                    RTStrPrintf(pDev->szName, sizeof(pDev->szName), "%s", pszName);
                }

                LogFunc(("Device '%s': %RU32\n", pszName, curDevID));

                if (pszName)
                {
                    RTStrFree(pszName);
                    pszName = NULL;
                }
            }

            CFRelease(pcfstrName);

            /* Check if the device is alive for the intended usage. */
            AudioObjectPropertyAddress propAddrAlive = { kAudioDevicePropertyDeviceIsAlive,
                                                          enmUsage == PDMAUDIODIR_IN
                                                        ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput,
                                                        kAudioObjectPropertyElementMaster };

            UInt32 uAlive = 0;
            uSize = sizeof(uAlive);

            err = AudioObjectGetPropertyData(curDevID, &propAddrAlive, 0, NULL, &uSize, &uAlive);
            if (   (err == noErr)
                && !uAlive)
            {
                pDev->fFlags |= PDMAUDIODEV_FLAGS_DEAD;
            }

            /* Check if the device is being hogged by someone else. */
            AudioObjectPropertyAddress propAddrHogged = { kAudioDevicePropertyHogMode,
                                                          kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };

            pid_t pid = 0;
            uSize = sizeof(pid);

            err = AudioObjectGetPropertyData(curDevID, &propAddrHogged, 0, NULL, &uSize, &pid);
            if (   (err == noErr)
                && (pid != -1))
            {
                pDev->fFlags |= PDMAUDIODEV_FLAGS_LOCKED;
            }

            /* Add the device to the enumeration. */
            rc = DrvAudioHlpDeviceEnumAdd(pDevEnm, pDev);
            if (RT_FAILURE(rc))
                break;

            /* NULL device pointer because it's now part of the device enumeration. */
            pDev = NULL;
        }

        if (RT_FAILURE(rc))
        {
            DrvAudioHlpDeviceFree(pDev);
            pDev = NULL;
        }

    } while (0);

    if (RT_SUCCESS(rc))
    {
#ifdef DEBUG
        LogFunc(("Devices for pDevEnm=%p, enmUsage=%RU32:\n", pDevEnm, enmUsage));
        DrvAudioHlpDeviceEnumPrint("Core Audio", pDevEnm);
#endif
    }
    else
        DrvAudioHlpDeviceEnumFree(pDevEnm);

    LogFlowFuncLeaveRC(rc);
    return rc;
}


/**
 * Checks if an audio device with a specific device ID is in the given device
 * enumeration or not.
 *
 * @retval  true if the node is the last element in the list.
 * @retval  false otherwise.
 *
 * @param   pEnmSrc               Device enumeration to search device ID in.
 * @param   deviceID              Device ID to search.
 */
bool coreAudioDevicesHasDevice(PPDMAUDIODEVICEENUM pEnmSrc, AudioDeviceID deviceID)
{
    PPDMAUDIODEVICE pDevSrc;
    RTListForEach(&pEnmSrc->lstDevices, pDevSrc, PDMAUDIODEVICE, Node)
    {
        PCOREAUDIODEVICEDATA pDevSrcData = (PCOREAUDIODEVICEDATA)pDevSrc->pvData;
        AssertPtr(pDevSrcData);

        if (pDevSrcData->deviceID == deviceID)
            return true;
    }

    return false;
}


/**
 * Enumerates all host devices and builds a final device enumeration list, consisting
 * of (duplex) input and output devices.
 *
 * @return  IPRT status code.
 * @param   pThis               Host audio driver instance.
 * @param   pEnmDst             Where to store the device enumeration list.
 */
int coreAudioDevicesEnumerateAll(PDRVHOSTCOREAUDIO pThis, PPDMAUDIODEVICEENUM pEnmDst)
{
    PDMAUDIODEVICEENUM devEnmIn;
    int rc = coreAudioDevicesEnumerate(pThis, PDMAUDIODIR_IN, &devEnmIn);
    if (RT_SUCCESS(rc))
    {
        PDMAUDIODEVICEENUM devEnmOut;
        rc = coreAudioDevicesEnumerate(pThis, PDMAUDIODIR_OUT, &devEnmOut);
        if (RT_SUCCESS(rc))
        {
            /*
             * Build up the final device enumeration, based on the input and output device lists
             * just enumerated.
             *
             * Also make sure to handle duplex devices, that is, devices which act as input and output
             * at the same time.
             */

            rc = DrvAudioHlpDeviceEnumInit(pEnmDst);
            if (RT_SUCCESS(rc))
            {
                PPDMAUDIODEVICE pDevSrcIn;
                RTListForEach(&devEnmIn.lstDevices, pDevSrcIn, PDMAUDIODEVICE, Node)
                {
                    PCOREAUDIODEVICEDATA pDevSrcInData = (PCOREAUDIODEVICEDATA)pDevSrcIn->pvData;
                    AssertPtr(pDevSrcInData);

                    PPDMAUDIODEVICE pDevDst = DrvAudioHlpDeviceAlloc(sizeof(COREAUDIODEVICEDATA));
                    if (!pDevDst)
                    {
                        rc = VERR_NO_MEMORY;
                        break;
                    }

                    PCOREAUDIODEVICEDATA pDevDstData = (PCOREAUDIODEVICEDATA)pDevDst->pvData;
                    AssertPtr(pDevDstData);
                    coreAudioDeviceDataInit(pDevDstData, pDevSrcInData->deviceID, true /* fIsInput */, pThis);

                    RTStrCopy(pDevDst->szName, sizeof(pDevDst->szName), pDevSrcIn->szName);

                    pDevDst->enmUsage          = PDMAUDIODIR_IN; /* Input device by default (simplex). */
                    pDevDst->cMaxInputChannels = pDevSrcIn->cMaxInputChannels;

                    /* Handle flags. */
                    if (pDevSrcIn->fFlags & PDMAUDIODEV_FLAGS_DEFAULT)
                        pDevDst->fFlags |= PDMAUDIODEV_FLAGS_DEFAULT;
                    /** @todo Handle hot plugging? */

                    /*
                     * Now search through the list of all found output devices and check if we found
                     * an output device with the same device ID as the currently handled input device.
                     *
                     * If found, this means we have to treat that device as a duplex device then.
                     */
                    PPDMAUDIODEVICE pDevSrcOut;
                    RTListForEach(&devEnmOut.lstDevices, pDevSrcOut, PDMAUDIODEVICE, Node)
                    {
                        PCOREAUDIODEVICEDATA pDevSrcOutData = (PCOREAUDIODEVICEDATA)pDevSrcOut->pvData;
                        AssertPtr(pDevSrcOutData);

                        if (pDevSrcInData->deviceID == pDevSrcOutData->deviceID)
                        {
                            pDevDst->enmUsage           = PDMAUDIODIR_ANY;
                            pDevDst->cMaxOutputChannels = pDevSrcOut->cMaxOutputChannels;
                            break;
                        }
                    }

                    if (RT_SUCCESS(rc))
                    {
                        rc = DrvAudioHlpDeviceEnumAdd(pEnmDst, pDevDst);
                    }
                    else
                    {
                        DrvAudioHlpDeviceFree(pDevDst);
                        pDevDst = NULL;
                    }
                }

                if (RT_SUCCESS(rc))
                {
                    /*
                     * As a last step, add all remaining output devices which have not been handled in the loop above,
                     * that is, all output devices which operate in simplex mode.
                     */
                    PPDMAUDIODEVICE pDevSrcOut;
                    RTListForEach(&devEnmOut.lstDevices, pDevSrcOut, PDMAUDIODEVICE, Node)
                    {
                        PCOREAUDIODEVICEDATA pDevSrcOutData = (PCOREAUDIODEVICEDATA)pDevSrcOut->pvData;
                        AssertPtr(pDevSrcOutData);

                        if (coreAudioDevicesHasDevice(pEnmDst, pDevSrcOutData->deviceID))
                            continue; /* Already in our list, skip. */

                        PPDMAUDIODEVICE pDevDst = DrvAudioHlpDeviceAlloc(sizeof(COREAUDIODEVICEDATA));
                        if (!pDevDst)
                        {
                            rc = VERR_NO_MEMORY;
                            break;
                        }

                        PCOREAUDIODEVICEDATA pDevDstData = (PCOREAUDIODEVICEDATA)pDevDst->pvData;
                        AssertPtr(pDevDstData);
                        coreAudioDeviceDataInit(pDevDstData, pDevSrcOutData->deviceID, false /* fIsInput */, pThis);

                        RTStrCopy(pDevDst->szName, sizeof(pDevDst->szName), pDevSrcOut->szName);

                        pDevDst->enmUsage           = PDMAUDIODIR_OUT;
                        pDevDst->cMaxOutputChannels = pDevSrcOut->cMaxOutputChannels;

                        pDevDstData->deviceID       = pDevSrcOutData->deviceID;

                        /* Handle flags. */
                        if (pDevSrcOut->fFlags & PDMAUDIODEV_FLAGS_DEFAULT)
                            pDevDst->fFlags |= PDMAUDIODEV_FLAGS_DEFAULT;
                        /** @todo Handle hot plugging? */

                        rc = DrvAudioHlpDeviceEnumAdd(pEnmDst, pDevDst);
                        if (RT_FAILURE(rc))
                        {
                            DrvAudioHlpDeviceFree(pDevDst);
                            break;
                        }
                    }
                }

                if (RT_FAILURE(rc))
                    DrvAudioHlpDeviceEnumFree(pEnmDst);
            }

            DrvAudioHlpDeviceEnumFree(&devEnmOut);
        }

        DrvAudioHlpDeviceEnumFree(&devEnmIn);
    }

#ifdef DEBUG
    if (RT_SUCCESS(rc))
        DrvAudioHlpDeviceEnumPrint("Core Audio (Final)", pEnmDst);
#endif

    LogFlowFuncLeaveRC(rc);
    return rc;
}


/**
 * Initializes a Core Audio-specific device data structure.
 *
 * @returns IPRT status code.
 * @param   pDevData            Device data structure to initialize.
 * @param   deviceID            Core Audio device ID to assign this structure to.
 * @param   fIsInput            Whether this is an input device or not.
 * @param   pDrv                Driver instance to use.
 */
static void coreAudioDeviceDataInit(PCOREAUDIODEVICEDATA pDevData, AudioDeviceID deviceID, bool fIsInput, PDRVHOSTCOREAUDIO pDrv)
{
    AssertPtrReturnVoid(pDevData);
    AssertPtrReturnVoid(pDrv);

    pDevData->deviceID = deviceID;
    pDevData->pDrv     = pDrv;

    /* Get the device UUID. */
    AudioObjectPropertyAddress propAdrDevUUID = { kAudioDevicePropertyDeviceUID,
                                                  fIsInput ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput,
                                                  kAudioObjectPropertyElementMaster };
    UInt32 uSize = sizeof(pDevData->UUID);
    OSStatus err = AudioObjectGetPropertyData(pDevData->deviceID, &propAdrDevUUID, 0, NULL, &uSize, &pDevData->UUID);
    if (err != noErr)
        LogRel(("CoreAudio: Failed to retrieve device UUID for device %RU32 (%RI32)\n", deviceID, err));

    RTListInit(&pDevData->lstStreams);
}


/**
 * Propagates an audio device status to all its connected Core Audio streams.
 *
 * @return IPRT status code.
 * @param  pDev                 Audio device to propagate status for.
 * @param  enmSts               Status to propagate.
 */
static int coreAudioDevicePropagateStatus(PPDMAUDIODEVICE pDev, COREAUDIOSTATUS enmSts)
{
    AssertPtrReturn(pDev, VERR_INVALID_POINTER);

    PCOREAUDIODEVICEDATA pDevData = (PCOREAUDIODEVICEDATA)pDev->pvData;
    AssertPtrReturn(pDevData, VERR_INVALID_POINTER);

    /* Sanity. */
    AssertPtr(pDevData->pDrv);

    LogFlowFunc(("pDev=%p, pDevData=%p, enmSts=%RU32\n", pDev, pDevData, enmSts));

    PCOREAUDIOSTREAM pCAStream;
    RTListForEach(&pDevData->lstStreams, pCAStream, COREAUDIOSTREAM, Node)
    {
        LogFlowFunc(("pCAStream=%p\n", pCAStream));

        /* We move the reinitialization to the next output event.
         * This make sure this thread isn't blocked and the
         * reinitialization is done when necessary only. */
        ASMAtomicXchgU32(&pCAStream->enmStatus, enmSts);
    }

    return VINF_SUCCESS;
}


static DECLCALLBACK(OSStatus) coreAudioDeviceStateChangedCb(AudioObjectID propertyID,
                                                            UInt32 nAddresses,
                                                            const AudioObjectPropertyAddress properties[],
                                                            void *pvUser)
{
    RT_NOREF(propertyID, nAddresses, properties);

    LogFlowFunc(("propertyID=%u, nAddresses=%u, pvUser=%p\n", propertyID, nAddresses, pvUser));

    PPDMAUDIODEVICE pDev = (PPDMAUDIODEVICE)pvUser;
    AssertPtr(pDev);

    PCOREAUDIODEVICEDATA pData = (PCOREAUDIODEVICEDATA)pDev->pvData;
    AssertPtrReturn(pData, VERR_INVALID_POINTER);

    PDRVHOSTCOREAUDIO pThis = pData->pDrv;
    AssertPtr(pThis);

    int rc2 = RTCritSectEnter(&pThis->CritSect);
    AssertRC(rc2);

    UInt32 uAlive = 1;
    UInt32 uSize  = sizeof(UInt32);

    AudioObjectPropertyAddress propAdr = { kAudioDevicePropertyDeviceIsAlive, kAudioObjectPropertyScopeGlobal,
                                           kAudioObjectPropertyElementMaster };

    AudioDeviceID deviceID = pData->deviceID;

    OSStatus err = AudioObjectGetPropertyData(deviceID, &propAdr, 0, NULL, &uSize, &uAlive);

    bool fIsDead = false;

    if (err == kAudioHardwareBadDeviceError)
        fIsDead = true; /* Unplugged. */
    else if ((err == kAudioHardwareNoError) && (!RT_BOOL(uAlive)))
        fIsDead = true; /* Something else happened. */

    if (fIsDead)
    {
        LogRel2(("CoreAudio: Device '%s' stopped functioning\n", pDev->szName));

        /* Mark device as dead. */
        rc2 = coreAudioDevicePropagateStatus(pDev, COREAUDIOSTATUS_UNINIT);
        AssertRC(rc2);
    }

    rc2 = RTCritSectLeave(&pThis->CritSect);
    AssertRC(rc2);

    return noErr;
}

/* Callback for getting notified when the default recording/playback device has been changed. */
static DECLCALLBACK(OSStatus) coreAudioDefaultDeviceChangedCb(AudioObjectID propertyID,
                                                              UInt32 nAddresses,
                                                              const AudioObjectPropertyAddress properties[],
                                                              void *pvUser)
{
    RT_NOREF(propertyID, nAddresses);

    LogFlowFunc(("propertyID=%u, nAddresses=%u, pvUser=%p\n", propertyID, nAddresses, pvUser));

    PDRVHOSTCOREAUDIO pThis = (PDRVHOSTCOREAUDIO)pvUser;
    AssertPtr(pThis);

    int rc2 = RTCritSectEnter(&pThis->CritSect);
    AssertRC(rc2);

    for (UInt32 idxAddress = 0; idxAddress < nAddresses; idxAddress++)
    {
        PPDMAUDIODEVICE pDev = NULL;

        /*
         * Check if the default input / output device has been changed.
         */
        const AudioObjectPropertyAddress *pProperty = &properties[idxAddress];
        switch (pProperty->mSelector)
        {
            case kAudioHardwarePropertyDefaultInputDevice:
                LogFlowFunc(("kAudioHardwarePropertyDefaultInputDevice\n"));
                pDev = pThis->pDefaultDevIn;
                break;

            case kAudioHardwarePropertyDefaultOutputDevice:
                LogFlowFunc(("kAudioHardwarePropertyDefaultOutputDevice\n"));
                pDev = pThis->pDefaultDevOut;
                break;

            default:
                /* Skip others. */
                break;
        }

        LogFlowFunc(("pDev=%p\n", pDev));

#ifndef VBOX_WITH_AUDIO_CALLBACKS
        if (pDev)
        {
            PCOREAUDIODEVICEDATA pData = (PCOREAUDIODEVICEDATA)pDev->pvData;
            AssertPtr(pData);

            /* This listener is called on every change of the hardware
             * device. So check if the default device has really changed. */
            UInt32 uSize = sizeof(AudioDeviceID);
            UInt32 uResp = 0;

            OSStatus err = AudioObjectGetPropertyData(kAudioObjectSystemObject, pProperty, 0, NULL, &uSize, &uResp);
            if (err == noErr)
            {
                if (pData->deviceID != uResp) /* Has the device ID changed? */
                {
                    rc2 = coreAudioDevicePropagateStatus(pDev, COREAUDIOSTATUS_REINIT);
                    AssertRC(rc2);
                }
            }
        }
#endif /* VBOX_WITH_AUDIO_CALLBACKS */
    }

#ifdef VBOX_WITH_AUDIO_CALLBACKS
    PFNPDMHOSTAUDIOCALLBACK pfnCallback = pThis->pfnCallback;
#endif

    /* Make sure to leave the critical section before calling the callback. */
    rc2 = RTCritSectLeave(&pThis->CritSect);
    AssertRC(rc2);

#ifdef VBOX_WITH_AUDIO_CALLBACKS
    if (pfnCallback)
        /* Ignore rc */ pfnCallback(pThis->pDrvIns, PDMAUDIOBACKENDCBTYPE_DEVICES_CHANGED, NULL, 0);
#endif

    return noErr;
}

#ifndef VBOX_WITH_AUDIO_CALLBACKS
/**
 * Re-initializes a Core Audio stream with a specific audio device and stream configuration.
 *
 * @return IPRT status code.
 * @param  pThis                Driver instance.
 * @param  pCAStream            Audio stream to re-initialize.
 * @param  pDev                 Audio device to use for re-initialization.
 * @param  pCfg                 Stream configuration to use for re-initialization.
 */
static int coreAudioStreamReinitEx(PDRVHOSTCOREAUDIO pThis,
                                   PCOREAUDIOSTREAM pCAStream, PPDMAUDIODEVICE pDev, PPDMAUDIOSTREAMCFG pCfg)
{
    LogFunc(("pCAStream=%p\n", pCAStream));

    int rc = coreAudioStreamUninit(pCAStream);
    if (RT_SUCCESS(rc))
    {
        rc = coreAudioStreamInit(pCAStream, pThis, pDev);
        if (RT_SUCCESS(rc))
        {
            rc = coreAudioStreamInitQueue(pCAStream, pCfg /* pCfgReq */, NULL /* pCfgAcq */);
            if (RT_SUCCESS(rc))
                rc = coreAudioStreamControl(pCAStream->pDrv, pCAStream, PDMAUDIOSTREAMCMD_ENABLE);

            if (RT_FAILURE(rc))
            {
                int rc2 = coreAudioStreamUninit(pCAStream);
                AssertRC(rc2);
            }
        }
    }

    if (RT_FAILURE(rc))
        LogRel(("CoreAudio: Unable to re-init stream: %Rrc\n", rc));

    return rc;
}

/**
 * Re-initializes a Core Audio stream with a specific audio device.
 *
 * @return IPRT status code.
 * @param  pThis                Driver instance.
 * @param  pCAStream            Audio stream to re-initialize.
 * @param  pDev                 Audio device to use for re-initialization.
 */
static int coreAudioStreamReinit(PDRVHOSTCOREAUDIO pThis, PCOREAUDIOSTREAM pCAStream, PPDMAUDIODEVICE pDev)
{
    int rc = coreAudioStreamUninit(pCAStream);
    if (RT_SUCCESS(rc))
    {
        /* Use the acquired stream configuration from the former initialization to
         * re-initialize the stream. */
        PDMAUDIOSTREAMCFG CfgAcq;
        rc = coreAudioASBDToStreamCfg(&pCAStream->Unit.streamFmt, &CfgAcq);
        if (RT_SUCCESS(rc))
            rc = coreAudioStreamReinitEx(pThis, pCAStream, pDev, &CfgAcq);
    }

    return rc;
}
#endif /* !VBOX_WITH_AUDIO_CALLBACKS */

#ifdef VBOX_WITH_AUDIO_CA_CONVERTER
/* Callback to convert audio input data from one format to another. */
static DECLCALLBACK(OSStatus) coreAudioConverterCb(AudioConverterRef              inAudioConverter,
                                                   UInt32                        *ioNumberDataPackets,
                                                   AudioBufferList               *ioData,
                                                   AudioStreamPacketDescription **ppASPD,
                                                   void                          *pvUser)
{
    RT_NOREF(inAudioConverter);

    AssertPtrReturn(ioNumberDataPackets, caConverterEOFDErr);
    AssertPtrReturn(ioData,              caConverterEOFDErr);

    PCOREAUDIOCONVCBCTX pConvCbCtx = (PCOREAUDIOCONVCBCTX)pvUser;
    AssertPtr(pConvCbCtx);

    /* Initialize values. */
    ioData->mBuffers[0].mNumberChannels = 0;
    ioData->mBuffers[0].mDataByteSize   = 0;
    ioData->mBuffers[0].mData           = NULL;

    if (ppASPD)
    {
        Log3Func(("Handling packet description not implemented\n"));
    }
    else
    {
        /** @todo Check converter ID? */

        /** @todo Handled non-interleaved data by going through the full buffer list,
         *        not only through the first buffer like we do now. */
        Log3Func(("ioNumberDataPackets=%RU32\n", *ioNumberDataPackets));

        UInt32 cNumberDataPackets = *ioNumberDataPackets;
        Assert(pConvCbCtx->uPacketIdx + cNumberDataPackets <= pConvCbCtx->uPacketCnt);

        if (cNumberDataPackets)
        {
            AssertPtr(pConvCbCtx->pBufLstSrc);
            Assert(pConvCbCtx->pBufLstSrc->mNumberBuffers == 1); /* Only one buffer for the source supported atm. */

            AudioStreamBasicDescription *pSrcASBD = &pConvCbCtx->asbdSrc;
            AudioBuffer                 *pSrcBuf  = &pConvCbCtx->pBufLstSrc->mBuffers[0];

            size_t cbOff   = pConvCbCtx->uPacketIdx * pSrcASBD->mBytesPerPacket;

            cNumberDataPackets = RT_MIN((pSrcBuf->mDataByteSize - cbOff) / pSrcASBD->mBytesPerPacket,
                                        cNumberDataPackets);

            void  *pvAvail = (uint8_t *)pSrcBuf->mData + cbOff;
            size_t cbAvail = RT_MIN(pSrcBuf->mDataByteSize - cbOff, cNumberDataPackets * pSrcASBD->mBytesPerPacket);

            Log3Func(("cNumberDataPackets=%RU32, cbOff=%zu, cbAvail=%zu\n", cNumberDataPackets, cbOff, cbAvail));

            /* Set input data for the converter to use.
             * Note: For VBR (Variable Bit Rates) or interleaved data handling we need multiple buffers here. */
            ioData->mNumberBuffers = 1;

            ioData->mBuffers[0].mNumberChannels = pSrcBuf->mNumberChannels;
            ioData->mBuffers[0].mDataByteSize   = cbAvail;
            ioData->mBuffers[0].mData           = pvAvail;

#ifdef VBOX_AUDIO_DEBUG_DUMP_PCM_DATA
            RTFILE fh;
            int rc = RTFileOpen(&fh,VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "caConverterCbInput.pcm",
                                RTFILE_O_OPEN_CREATE | RTFILE_O_APPEND | RTFILE_O_WRITE | RTFILE_O_DENY_NONE);
            if (RT_SUCCESS(rc))
            {
                RTFileWrite(fh, pvAvail, cbAvail, NULL);
                RTFileClose(fh);
            }
            else
                AssertFailed();
#endif
            pConvCbCtx->uPacketIdx += cNumberDataPackets;
            Assert(pConvCbCtx->uPacketIdx <= pConvCbCtx->uPacketCnt);

            *ioNumberDataPackets = cNumberDataPackets;
        }
    }

    Log3Func(("%RU32 / %RU32 -> ioNumberDataPackets=%RU32\n",
              pConvCbCtx->uPacketIdx, pConvCbCtx->uPacketCnt, *ioNumberDataPackets));

    return noErr;
}
#endif /* VBOX_WITH_AUDIO_CA_CONVERTER */


/**
 * Initializes a Core Audio stream.
 *
 * @return IPRT status code.
 * @param  pThis                Driver instance.
 * @param  pCAStream            Stream to initialize.
 * @param  pDev                 Audio device to use for this stream.
 */
static int coreAudioStreamInit(PCOREAUDIOSTREAM pCAStream, PDRVHOSTCOREAUDIO pThis, PPDMAUDIODEVICE pDev)
{
    AssertPtrReturn(pCAStream, VERR_INVALID_POINTER);
    AssertPtrReturn(pThis,     VERR_INVALID_POINTER);
    AssertPtrReturn(pDev,      VERR_INVALID_POINTER);

    Assert(pCAStream->Unit.pDevice == NULL); /* Make sure no device is assigned yet. */
    AssertPtr(pDev->pvData);
    Assert(pDev->cbData == sizeof(COREAUDIODEVICEDATA));

#ifdef DEBUG
    PCOREAUDIODEVICEDATA pData = (PCOREAUDIODEVICEDATA)pDev->pvData;
    LogFunc(("pCAStream=%p, pDev=%p ('%s', ID=%RU32)\n", pCAStream, pDev, pDev->szName, pData->deviceID));
#endif

    pCAStream->Unit.pDevice = pDev;
    pCAStream->pDrv = pThis;

    return VINF_SUCCESS;
}

# define CA_BREAK_STMT(stmt) \
    stmt; \
    break;

/**
 * Thread for a Core Audio stream's audio queue handling.
 * This thread is required per audio queue to pump data to/from the Core Audio stream and
 * handling its callbacks.
 *
 * @returns IPRT status code.
 * @param   hThreadSelf         Thread handle.
 * @param   pvUser              User argument.
 */
static DECLCALLBACK(int) coreAudioQueueThread(RTTHREAD hThreadSelf, void *pvUser)
{
    RT_NOREF(hThreadSelf);

    PCOREAUDIOSTREAM pCAStream = (PCOREAUDIOSTREAM)pvUser;
    AssertPtr(pCAStream);
    AssertPtr(pCAStream->pCfg);

    LogFunc(("Starting pCAStream=%p\n", pCAStream));

    const bool fIn = pCAStream->pCfg->enmDir == PDMAUDIODIR_IN;

    /*
     * Create audio queue.
     */
    OSStatus err;
    if (fIn)
        err = AudioQueueNewInput(&pCAStream->asbdStream, coreAudioInputQueueCb, pCAStream /* pvData */,
                                 CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 0, &pCAStream->audioQueue);
    else
        err = AudioQueueNewOutput(&pCAStream->asbdStream, coreAudioOutputQueueCb, pCAStream /* pvData */,
                                  CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 0, &pCAStream->audioQueue);

    if (err != noErr)
        return VERR_GENERAL_FAILURE; /** @todo Fudge! */

    /*
     * Assign device to queue.
     */
    PCOREAUDIODEVICEDATA pData = (PCOREAUDIODEVICEDATA)pCAStream->Unit.pDevice->pvData;
    AssertPtr(pData);

    UInt32 uSize = sizeof(pData->UUID);
    err = AudioQueueSetProperty(pCAStream->audioQueue, kAudioQueueProperty_CurrentDevice, &pData->UUID, uSize);
    if (err != noErr)
        return VERR_GENERAL_FAILURE; /** @todo Fudge! */

    const size_t cbBufSize = AQ_BUF_SIZE; /** @todo Make this configurable! */

    /*
     * Allocate audio buffers.
     */
    for (size_t i = 0; i < RT_ELEMENTS(pCAStream->audioBuffer); i++)
    {
        err = AudioQueueAllocateBuffer(pCAStream->audioQueue, cbBufSize, &pCAStream->audioBuffer[i]);
        if (err != noErr)
            break;
    }

    if (err != noErr)
        return VERR_GENERAL_FAILURE; /** @todo Fudge! */

    /* Signal the main thread before entering the main loop. */
    RTThreadUserSignal(RTThreadSelf());

    /*
     * Enter the main loop.
     */
    while (!ASMAtomicReadBool(&pCAStream->fShutdown))
    {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.10, 1);
    }

    /*
     * Cleanup.
     */
    if (fIn)
    {
        AudioQueueStop(pCAStream->audioQueue, 1);
    }
    else
    {
        AudioQueueStop(pCAStream->audioQueue, 0);
    }

    for (size_t i = 0; i < RT_ELEMENTS(pCAStream->audioBuffer); i++)
    {
        if (pCAStream->audioBuffer[i])
            AudioQueueFreeBuffer(pCAStream->audioQueue, pCAStream->audioBuffer[i]);
    }

    AudioQueueDispose(pCAStream->audioQueue, 1);

    LogFunc(("Ended pCAStream=%p\n", pCAStream));
    return VINF_SUCCESS;
}

/**
 * Processes input data of an audio queue buffer and stores it into a Core Audio stream.
 *
 * @returns IPRT status code.
 * @param   pCAStream           Core Audio stream to store input data into.
 * @param   audioBuffer         Audio buffer to process input data from.
 */
int coreAudioInputQueueProcBuffer(PCOREAUDIOSTREAM pCAStream, AudioQueueBufferRef audioBuffer)
{
    PRTCIRCBUF pCircBuf = pCAStream->pCircBuf;
    AssertPtr(pCircBuf);

    UInt8 *pvSrc = (UInt8 *)audioBuffer->mAudioData;
    UInt8 *pvDst = NULL;

    size_t cbWritten = 0;

    size_t cbToWrite = audioBuffer->mAudioDataByteSize;
    size_t cbLeft    = cbToWrite;

    while (cbLeft)
    {
        /* Try to acquire the necessary block from the ring buffer. */
        RTCircBufAcquireWriteBlock(pCircBuf, cbLeft, (void **)&pvDst, &cbToWrite);

        if (!cbToWrite)
            break;

        /* Copy the data from our ring buffer to the core audio buffer. */
        memcpy((UInt8 *)pvDst, pvSrc + cbWritten, cbToWrite);

        /* Release the read buffer, so it could be used for new data. */
        RTCircBufReleaseWriteBlock(pCircBuf, cbToWrite);

        cbWritten += cbToWrite;

        Assert(cbLeft >= cbToWrite);
        cbLeft -= cbToWrite;
    }

    Log3Func(("pCAStream=%p, cbBuffer=%RU32/%zu, cbWritten=%zu\n",
              pCAStream, audioBuffer->mAudioDataByteSize, audioBuffer->mAudioDataBytesCapacity, cbWritten));

    return VINF_SUCCESS;
}

/**
 * Input audio queue callback. Called whenever input data from the audio queue becomes available.
 *
 * @param   pvUser              User argument.
 * @param   audioQueue          Audio queue to process input data from.
 * @param   audioBuffer         Audio buffer to process input data from. Must be part of audio queue.
 * @param   pAudioTS            Audio timestamp.
 * @param   cPacketDesc         Number of packet descriptors.
 * @param   paPacketDesc        Array of packet descriptors.
 */
static DECLCALLBACK(void) coreAudioInputQueueCb(void *pvUser, AudioQueueRef audioQueue, AudioQueueBufferRef audioBuffer,
                                                const AudioTimeStamp *pAudioTS,
                                                UInt32 cPacketDesc, const AudioStreamPacketDescription *paPacketDesc)
{
    RT_NOREF(audioQueue, pAudioTS, cPacketDesc, paPacketDesc);

    PCOREAUDIOSTREAM pCAStream = (PCOREAUDIOSTREAM)pvUser;
    AssertPtr(pCAStream);

    int rc = RTCritSectEnter(&pCAStream->CritSect);
    AssertRC(rc);

    rc = coreAudioInputQueueProcBuffer(pCAStream, audioBuffer);
    if (RT_SUCCESS(rc))
        AudioQueueEnqueueBuffer(audioQueue, audioBuffer, 0, NULL);

    rc = RTCritSectLeave(&pCAStream->CritSect);
    AssertRC(rc);
}

/**
 * Processes output data of a Core Audio stream into an audio queue buffer.
 *
 * @returns IPRT status code.
 * @param   pCAStream           Core Audio stream to process output data for.
 * @param   audioBuffer         Audio buffer to store data into.
 */
int coreAudioOutputQueueProcBuffer(PCOREAUDIOSTREAM pCAStream, AudioQueueBufferRef audioBuffer)
{
    AssertPtr(pCAStream);

    PRTCIRCBUF pCircBuf = pCAStream->pCircBuf;
    AssertPtr(pCircBuf);

    size_t cbRead = 0;

    UInt8 *pvSrc = NULL;
    UInt8 *pvDst = (UInt8 *)audioBuffer->mAudioData;

    size_t cbToRead = RT_MIN(RTCircBufUsed(pCircBuf), audioBuffer->mAudioDataBytesCapacity);
    size_t cbLeft   = cbToRead;

    while (cbLeft)
    {
        /* Try to acquire the necessary block from the ring buffer. */
        RTCircBufAcquireReadBlock(pCircBuf, cbLeft, (void **)&pvSrc, &cbToRead);

        if (cbToRead)
        {
            /* Copy the data from our ring buffer to the core audio buffer. */
            memcpy((UInt8 *)pvDst + cbRead, pvSrc, cbToRead);
        }

        /* Release the read buffer, so it could be used for new data. */
        RTCircBufReleaseReadBlock(pCircBuf, cbToRead);

        if (!cbToRead)
            break;

        /* Move offset. */
        cbRead += cbToRead;
        Assert(cbRead <= audioBuffer->mAudioDataBytesCapacity);

        Assert(cbToRead <= cbLeft);
        cbLeft -= cbToRead;
    }

    audioBuffer->mAudioDataByteSize = cbRead;

    if (audioBuffer->mAudioDataByteSize < audioBuffer->mAudioDataBytesCapacity)
    {
        RT_BZERO((UInt8 *)audioBuffer->mAudioData + audioBuffer->mAudioDataByteSize,
                 audioBuffer->mAudioDataBytesCapacity - audioBuffer->mAudioDataByteSize);

        audioBuffer->mAudioDataByteSize = audioBuffer->mAudioDataBytesCapacity;
    }

    Log3Func(("pCAStream=%p, cbCapacity=%RU32, cbRead=%zu\n",
              pCAStream, audioBuffer->mAudioDataBytesCapacity, cbRead));

    return VINF_SUCCESS;
}

/**
 * Output audio queue callback. Called whenever an audio queue is ready to process more output data.
 *
 * @param   pvUser              User argument.
 * @param   audioQueue          Audio queue to process output data for.
 * @param   audioBuffer         Audio buffer to store output data in. Must be part of audio queue.
 */
static DECLCALLBACK(void) coreAudioOutputQueueCb(void *pvUser, AudioQueueRef audioQueue, AudioQueueBufferRef audioBuffer)
{
    RT_NOREF(audioQueue);

    PCOREAUDIOSTREAM pCAStream = (PCOREAUDIOSTREAM)pvUser;
    AssertPtr(pCAStream);

    int rc = RTCritSectEnter(&pCAStream->CritSect);
    AssertRC(rc);

    rc = coreAudioOutputQueueProcBuffer(pCAStream, audioBuffer);
    if (RT_SUCCESS(rc))
        AudioQueueEnqueueBuffer(audioQueue, audioBuffer, 0, NULL);

    rc = RTCritSectLeave(&pCAStream->CritSect);
    AssertRC(rc);
}

/**
 * Invalidates a Core Audio stream's audio queue.
 *
 * @returns IPRT status code.
 * @param   pCAStream           Core Audio stream to invalidate its queue for.
 */
static int coreAudioStreamInvalidateQueue(PCOREAUDIOSTREAM pCAStream)
{
    int rc = VINF_SUCCESS;

    for (size_t i = 0; i < RT_ELEMENTS(pCAStream->audioBuffer); i++)
    {
        AudioQueueBufferRef pBuf = pCAStream->audioBuffer[i];

        if (pCAStream->pCfg->enmDir == PDMAUDIODIR_IN)
        {
            int rc2 = coreAudioInputQueueProcBuffer(pCAStream, pBuf);
            if (RT_SUCCESS(rc2))
            {
                AudioQueueEnqueueBuffer(pCAStream->audioQueue, pBuf, 0, NULL);
            }
        }
        else if (pCAStream->pCfg->enmDir == PDMAUDIODIR_OUT)
        {
            int rc2 = coreAudioOutputQueueProcBuffer(pCAStream, pBuf);
            if (   RT_SUCCESS(rc2)
                && pBuf->mAudioDataByteSize)
            {
                AudioQueueEnqueueBuffer(pCAStream->audioQueue, pBuf, 0, NULL);
            }

            if (RT_SUCCESS(rc))
                rc = rc2;
        }
        else
            AssertFailed();
    }

    return rc;
}

/**
 * Initializes a Core Audio stream's audio queue.
 *
 * @returns IPRT status code.
 * @param   pCAStream           Core Audio stream to initialize audio queue for.
 * @param   pCfgReq             Requested stream configuration.
 * @param   pCfgAcq             Acquired stream configuration on success.
 */
static int coreAudioStreamInitQueue(PCOREAUDIOSTREAM pCAStream, PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    RT_NOREF(pCfgAcq);

    LogFunc(("pCAStream=%p, pCfgReq=%p, pCfgAcq=%p\n", pCAStream, pCfgReq, pCfgAcq));

    /* No device assigned? Bail out early. */
    if (pCAStream->Unit.pDevice == NULL)
        return VERR_NOT_AVAILABLE;

    const bool fIn = pCfgReq->enmDir == PDMAUDIODIR_IN;

    int rc = VINF_SUCCESS;

    /* Create the recording device's out format based on our required audio settings. */
    Assert(pCAStream->pCfg == NULL);
    pCAStream->pCfg = DrvAudioHlpStreamCfgDup(pCfgReq);
    if (!pCAStream->pCfg)
        rc = VERR_NO_MEMORY;

    coreAudioPCMPropsToASBD(&pCfgReq->Props, &pCAStream->asbdStream);
    /** @todo Do some validation? */

    coreAudioPrintASBD(  fIn
                       ? "Capturing queue format"
                       : "Playback queue format", &pCAStream->asbdStream);

    if (RT_FAILURE(rc))
    {
        LogRel(("CoreAudio: Failed to convert requested %s format to native format (%Rrc)\n", fIn ? "input" : "output", rc));
        return rc;
    }

    rc = RTCircBufCreate(&pCAStream->pCircBuf, PDMAUDIOSTREAMCFG_F2B(pCfgReq, 4096)); /** @todo Make this configurable. */
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Start the thread.
     */
    rc = RTThreadCreate(&pCAStream->hThread, coreAudioQueueThread,
                        pCAStream /* pvUser */, 0 /* Default stack size */,
                        RTTHREADTYPE_DEFAULT, RTTHREADFLAGS_WAITABLE, "CAQUEUE");
    if (RT_SUCCESS(rc))
        rc = RTThreadUserWait(pCAStream->hThread, 10 * 1000 /* 10s timeout */);

    LogFunc(("Returning %Rrc\n", rc));
    return rc;
}

/**
 * Unitializes a Core Audio stream's audio queue.
 *
 * @returns IPRT status code.
 * @param   pCAStream           Core Audio stream to unitialize audio queue for.
 */
static int coreAudioStreamUninitQueue(PCOREAUDIOSTREAM pCAStream)
{
    LogFunc(("pCAStream=%p\n", pCAStream));

    if (pCAStream->hThread != NIL_RTTHREAD)
    {
        LogFunc(("Waiting for thread ...\n"));

        ASMAtomicXchgBool(&pCAStream->fShutdown, true);

        int rcThread;
        int rc = RTThreadWait(pCAStream->hThread, 30 * 1000, &rcThread);
        if (RT_FAILURE(rc))
            return rc;

        RT_NOREF(rcThread);
        LogFunc(("Thread stopped with %Rrc\n", rcThread));

        pCAStream->hThread = NIL_RTTHREAD;
    }

    if (pCAStream->pCfg)
    {
        DrvAudioHlpStreamCfgFree(pCAStream->pCfg);
        pCAStream->pCfg = NULL;
    }

    if (pCAStream->pCircBuf)
    {
        RTCircBufDestroy(pCAStream->pCircBuf);
        pCAStream->pCircBuf = NULL;
    }

    LogFunc(("Returning\n"));
    return VINF_SUCCESS;
}

/**
 * Unitializes a Core Audio stream.
 *
 * @returns IPRT status code.
 * @param   pCAStream           Core Audio stream to uninitialize.
 */
static int coreAudioStreamUninit(PCOREAUDIOSTREAM pCAStream)
{
    LogFunc(("pCAStream=%p\n", pCAStream));

    int rc = coreAudioStreamUninitQueue(pCAStream);
    if (RT_SUCCESS(rc))
    {
        pCAStream->Unit.pDevice = NULL;
        pCAStream->pDrv         = NULL;
    }

    return rc;
}

/**
 * Registers callbacks for a specific Core Audio device.
 *
 * @return IPRT status code.
 * @param  pThis                Host audio driver instance.
 * @param  pDev                 Audio device to use for the registered callbacks.
 */
static int coreAudioDeviceRegisterCallbacks(PDRVHOSTCOREAUDIO pThis, PPDMAUDIODEVICE pDev)
{
    RT_NOREF(pThis);

    AudioDeviceID deviceID = kAudioDeviceUnknown;

    PCOREAUDIODEVICEDATA pData = (PCOREAUDIODEVICEDATA)pDev->pvData;
    if (pData)
        deviceID = pData->deviceID;

    if (deviceID != kAudioDeviceUnknown)
    {
        LogFunc(("deviceID=%RU32\n", deviceID));

        /*
         * Register device callbacks.
         */
        AudioObjectPropertyAddress propAdr = { kAudioDevicePropertyDeviceIsAlive, kAudioObjectPropertyScopeGlobal,
                                               kAudioObjectPropertyElementMaster };
        OSStatus err = AudioObjectAddPropertyListener(deviceID, &propAdr,
                                                      coreAudioDeviceStateChangedCb, pDev /* pvUser */);
        if (   err != noErr
            && err != kAudioHardwareIllegalOperationError)
        {
            LogRel(("CoreAudio: Failed to add the recording device state changed listener (%RI32)\n", err));
        }

        propAdr.mSelector = kAudioDeviceProcessorOverload;
        propAdr.mScope    = kAudioUnitScope_Global;
        err = AudioObjectAddPropertyListener(deviceID, &propAdr,
                                             coreAudioDevPropChgCb, pDev /* pvUser */);
        if (err != noErr)
            LogRel(("CoreAudio: Failed to register processor overload listener (%RI32)\n", err));

        propAdr.mSelector = kAudioDevicePropertyNominalSampleRate;
        propAdr.mScope    = kAudioUnitScope_Global;
        err = AudioObjectAddPropertyListener(deviceID, &propAdr,
                                             coreAudioDevPropChgCb, pDev /* pvUser */);
        if (err != noErr)
            LogRel(("CoreAudio: Failed to register sample rate changed listener (%RI32)\n", err));
    }

    return VINF_SUCCESS;
}

/**
 * Unregisters all formerly registered callbacks of a Core Audio device again.
 *
 * @return IPRT status code.
 * @param  pThis                Host audio driver instance.
 * @param  pDev                 Audio device to use for the registered callbacks.
 */
static int coreAudioDeviceUnregisterCallbacks(PDRVHOSTCOREAUDIO pThis, PPDMAUDIODEVICE pDev)
{
    RT_NOREF(pThis);

    AudioDeviceID deviceID = kAudioDeviceUnknown;

    if (pDev)
    {
        PCOREAUDIODEVICEDATA pData = (PCOREAUDIODEVICEDATA)pDev->pvData;
        if (pData)
            deviceID = pData->deviceID;
    }

    if (deviceID != kAudioDeviceUnknown)
    {
        LogFunc(("deviceID=%RU32\n", deviceID));

        /*
         * Unregister per-device callbacks.
         */
        AudioObjectPropertyAddress propAdr = { kAudioDeviceProcessorOverload, kAudioObjectPropertyScopeGlobal,
                                               kAudioObjectPropertyElementMaster };
        OSStatus err = AudioObjectRemovePropertyListener(deviceID, &propAdr,
                                                         coreAudioDevPropChgCb, pDev /* pvUser */);
        if (   err != noErr
            && err != kAudioHardwareBadObjectError)
        {
            LogRel(("CoreAudio: Failed to remove the recording processor overload listener (%RI32)\n", err));
        }

        propAdr.mSelector = kAudioDevicePropertyNominalSampleRate;
        err = AudioObjectRemovePropertyListener(deviceID, &propAdr,
                                                coreAudioDevPropChgCb, pDev /* pvUser */);
        if (   err != noErr
            && err != kAudioHardwareBadObjectError)
        {
            LogRel(("CoreAudio: Failed to remove the sample rate changed listener (%RI32)\n", err));
        }

        propAdr.mSelector = kAudioDevicePropertyDeviceIsAlive;
        err = AudioObjectRemovePropertyListener(deviceID, &propAdr,
                                                coreAudioDeviceStateChangedCb, pDev /* pvUser */);
        if (   err != noErr
            && err != kAudioHardwareBadObjectError)
        {
            LogRel(("CoreAudio: Failed to remove the device alive listener (%RI32)\n", err));
        }
    }

    return VINF_SUCCESS;
}

/* Callback for getting notified when some of the properties of an audio device have changed. */
static DECLCALLBACK(OSStatus) coreAudioDevPropChgCb(AudioObjectID                     propertyID,
                                                    UInt32                            cAddresses,
                                                    const AudioObjectPropertyAddress  properties[],
                                                    void                             *pvUser)
{
    RT_NOREF(cAddresses, properties, pvUser);

    PPDMAUDIODEVICE pDev = (PPDMAUDIODEVICE)pvUser;
    AssertPtr(pDev);

    LogFlowFunc(("propertyID=%u, nAddresses=%u, pDev=%p\n", propertyID, cAddresses, pDev));

    switch (propertyID)
    {
#ifdef DEBUG
       case kAudioDeviceProcessorOverload:
        {
            LogFunc(("Processor overload detected!\n"));
            break;
        }
#endif /* DEBUG */
        case kAudioDevicePropertyNominalSampleRate:
        {
#ifndef VBOX_WITH_AUDIO_CALLBACKS
            int rc2 = coreAudioDevicePropagateStatus(pDev, COREAUDIOSTATUS_REINIT);
            AssertRC(rc2);
#else
            RT_NOREF(pDev);
#endif
            break;
        }

        default:
            /* Just skip. */
            break;
    }

    return noErr;
}

/**
 * Enumerates all available host audio devices internally.
 *
 * @returns IPRT status code.
 * @param   pThis               Host audio driver instance.
 */
static int coreAudioEnumerateDevices(PDRVHOSTCOREAUDIO pThis)
{
    LogFlowFuncEnter();

    /*
     * Unregister old default devices, if any.
     */
    if (pThis->pDefaultDevIn)
    {
        coreAudioDeviceUnregisterCallbacks(pThis, pThis->pDefaultDevIn);
        pThis->pDefaultDevIn = NULL;
    }

    if (pThis->pDefaultDevOut)
    {
        coreAudioDeviceUnregisterCallbacks(pThis, pThis->pDefaultDevOut);
        pThis->pDefaultDevOut = NULL;
    }

    /* Remove old / stale device entries. */
    DrvAudioHlpDeviceEnumFree(&pThis->Devices);

    /* Enumerate all devices internally. */
    int rc = coreAudioDevicesEnumerateAll(pThis, &pThis->Devices);
    if (RT_SUCCESS(rc))
    {
        /*
         * Default input device.
         */
        pThis->pDefaultDevIn = DrvAudioHlpDeviceEnumGetDefaultDevice(&pThis->Devices, PDMAUDIODIR_IN);
        if (pThis->pDefaultDevIn)
        {
            LogRel2(("CoreAudio: Default capturing device is '%s'\n", pThis->pDefaultDevIn->szName));

#ifdef DEBUG
            PCOREAUDIODEVICEDATA pDevData = (PCOREAUDIODEVICEDATA)pThis->pDefaultDevIn->pvData;
            AssertPtr(pDevData);
            LogFunc(("pDefaultDevIn=%p, ID=%RU32\n", pThis->pDefaultDevIn, pDevData->deviceID));
#endif
            rc = coreAudioDeviceRegisterCallbacks(pThis, pThis->pDefaultDevIn);
        }
        else
            LogRel2(("CoreAudio: No default capturing device found\n"));

        /*
         * Default output device.
         */
        pThis->pDefaultDevOut = DrvAudioHlpDeviceEnumGetDefaultDevice(&pThis->Devices, PDMAUDIODIR_OUT);
        if (pThis->pDefaultDevOut)
        {
            LogRel2(("CoreAudio: Default playback device is '%s'\n", pThis->pDefaultDevOut->szName));

#ifdef DEBUG
            PCOREAUDIODEVICEDATA pDevData = (PCOREAUDIODEVICEDATA)pThis->pDefaultDevOut->pvData;
            AssertPtr(pDevData);
            LogFunc(("pDefaultDevOut=%p, ID=%RU32\n", pThis->pDefaultDevOut, pDevData->deviceID));
#endif
            rc = coreAudioDeviceRegisterCallbacks(pThis, pThis->pDefaultDevOut);
        }
        else
            LogRel2(("CoreAudio: No default playback device found\n"));
    }

    LogFunc(("Returning %Rrc\n", rc));
    return rc;
}

/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamCapture}
 */
static DECLCALLBACK(int) drvHostCoreAudioStreamCapture(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream,
                                                       void *pvBuf, uint32_t cxBuf, uint32_t *pcxRead)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    /* pcxRead is optional. */

    PCOREAUDIOSTREAM  pCAStream = (PCOREAUDIOSTREAM)pStream;
    PDRVHOSTCOREAUDIO pThis     = PDMIHOSTAUDIO_2_DRVHOSTCOREAUDIO(pInterface);

#ifndef VBOX_WITH_AUDIO_CALLBACKS
    /* Check if the audio device should be reinitialized. If so do it. */
    if (ASMAtomicReadU32(&pCAStream->enmStatus) == COREAUDIOSTATUS_REINIT)
    {
        /* For now re just re-initialize with the current input device. */
        if (pThis->pDefaultDevIn)
        {
            int rc2 = coreAudioStreamReinit(pThis, pCAStream, pThis->pDefaultDevIn);
            if (RT_FAILURE(rc2))
                return VERR_NOT_AVAILABLE;
        }
        else
            return VERR_NOT_AVAILABLE;
    }
#else
    RT_NOREF(pThis);
#endif

    if (ASMAtomicReadU32(&pCAStream->enmStatus) != COREAUDIOSTATUS_INIT)
    {
        if (pcxRead)
            *pcxRead = 0;
        return VINF_SUCCESS;
    }

    int rc = VINF_SUCCESS;

    uint32_t cbReadTotal = 0;

    rc = RTCritSectEnter(&pCAStream->CritSect);
    AssertRC(rc);

    do
    {
        size_t cbToWrite = RT_MIN(cxBuf, RTCircBufUsed(pCAStream->pCircBuf));

        uint8_t *pvChunk;
        size_t   cbChunk;

        Log3Func(("cbToWrite=%zu/%zu\n", cbToWrite, RTCircBufSize(pCAStream->pCircBuf)));

        while (cbToWrite)
        {
            /* Try to acquire the necessary block from the ring buffer. */
            RTCircBufAcquireReadBlock(pCAStream->pCircBuf, cbToWrite, (void **)&pvChunk, &cbChunk);
            if (cbChunk)
                memcpy((uint8_t *)pvBuf + cbReadTotal, pvChunk, cbChunk);

            /* Release the read buffer, so it could be used for new data. */
            RTCircBufReleaseReadBlock(pCAStream->pCircBuf, cbChunk);

            if (RT_FAILURE(rc))
                break;

            Assert(cbToWrite >= cbChunk);
            cbToWrite      -= cbChunk;

            cbReadTotal    += cbChunk;
        }
    }
    while (0);

    int rc2 = RTCritSectLeave(&pCAStream->CritSect);
    AssertRC(rc2);

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
static DECLCALLBACK(int) drvHostCoreAudioStreamPlay(PPDMIHOSTAUDIO pInterface,
                                                    PPDMAUDIOBACKENDSTREAM pStream, const void *pvBuf, uint32_t cxBuf,
                                                    uint32_t *pcxWritten)
{
    PDRVHOSTCOREAUDIO pThis     = PDMIHOSTAUDIO_2_DRVHOSTCOREAUDIO(pInterface);
    PCOREAUDIOSTREAM  pCAStream = (PCOREAUDIOSTREAM)pStream;

#ifndef VBOX_WITH_AUDIO_CALLBACKS
    /* Check if the audio device should be reinitialized. If so do it. */
    if (ASMAtomicReadU32(&pCAStream->enmStatus) == COREAUDIOSTATUS_REINIT)
    {
        if (pThis->pDefaultDevOut)
        {
            /* For now re just re-initialize with the current output device. */
            int rc2 = coreAudioStreamReinit(pThis, pCAStream, pThis->pDefaultDevOut);
            if (RT_FAILURE(rc2))
                return VERR_NOT_AVAILABLE;
        }
        else
            return VERR_NOT_AVAILABLE;
    }
#else
    RT_NOREF(pThis);
#endif

    if (ASMAtomicReadU32(&pCAStream->enmStatus) != COREAUDIOSTATUS_INIT)
    {
        if (pcxWritten)
            *pcxWritten = 0;
        return VINF_SUCCESS;
    }

    uint32_t cbWrittenTotal = 0;

    int rc = VINF_SUCCESS;

    rc = RTCritSectEnter(&pCAStream->CritSect);
    AssertRC(rc);

    size_t cbToWrite = RT_MIN(cxBuf, RTCircBufFree(pCAStream->pCircBuf));
    Log3Func(("cbToWrite=%zu\n", cbToWrite));

    uint8_t *pvChunk;
    size_t   cbChunk;

    while (cbToWrite)
    {
        /* Try to acquire the necessary space from the ring buffer. */
        RTCircBufAcquireWriteBlock(pCAStream->pCircBuf, cbToWrite, (void **)&pvChunk, &cbChunk);
        if (!cbChunk)
        {
            RTCircBufReleaseWriteBlock(pCAStream->pCircBuf, cbChunk);
            break;
        }

        Assert(cbChunk <= cbToWrite);
        Assert(cbWrittenTotal + cbChunk <= cxBuf);

        memcpy(pvChunk, (uint8_t *)pvBuf + cbWrittenTotal, cbChunk);

#ifdef VBOX_AUDIO_DEBUG_DUMP_PCM_DATA
        RTFILE fh;
        rc = RTFileOpen(&fh,VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "caPlayback.pcm",
                        RTFILE_O_OPEN_CREATE | RTFILE_O_APPEND | RTFILE_O_WRITE | RTFILE_O_DENY_NONE);
        if (RT_SUCCESS(rc))
        {
            RTFileWrite(fh, pvChunk, cbChunk, NULL);
            RTFileClose(fh);
        }
        else
            AssertFailed();
#endif

        /* Release the ring buffer, so the read thread could start reading this data. */
        RTCircBufReleaseWriteBlock(pCAStream->pCircBuf, cbChunk);

        if (RT_FAILURE(rc))
            break;

        Assert(cbToWrite >= cbChunk);
        cbToWrite      -= cbChunk;

        cbWrittenTotal += cbChunk;
    }

    if (    RT_SUCCESS(rc)
        &&  pCAStream->fRun
        && !pCAStream->fIsRunning)
    {
        rc = coreAudioStreamInvalidateQueue(pCAStream);
        if (RT_SUCCESS(rc))
        {
            AudioQueueStart(pCAStream->audioQueue, NULL);
            pCAStream->fRun       = false;
            pCAStream->fIsRunning = true;
        }
    }

    int rc2 = RTCritSectLeave(&pCAStream->CritSect);
    AssertRC(rc2);

    if (RT_SUCCESS(rc))
    {
        if (pcxWritten)
            *pcxWritten = cbWrittenTotal;
    }

    return rc;
}

static DECLCALLBACK(int) coreAudioStreamControl(PDRVHOSTCOREAUDIO pThis,
                                                PCOREAUDIOSTREAM pCAStream, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    RT_NOREF(pThis);

    uint32_t enmStatus = ASMAtomicReadU32(&pCAStream->enmStatus);

    LogFlowFunc(("enmStreamCmd=%RU32, enmStatus=%RU32\n", enmStreamCmd, enmStatus));

    if (!(   enmStatus == COREAUDIOSTATUS_INIT
#ifndef VBOX_WITH_AUDIO_CALLBACKS
          || enmStatus == COREAUDIOSTATUS_REINIT
#endif
          ))
    {
        return VINF_SUCCESS;
    }

    if (!pCAStream->pCfg) /* Not (yet) configured? Skip. */
        return VINF_SUCCESS;

    int rc = VINF_SUCCESS;

    switch (enmStreamCmd)
    {
        case PDMAUDIOSTREAMCMD_ENABLE:
        case PDMAUDIOSTREAMCMD_RESUME:
        {
            LogFunc(("Queue enable\n"));
            if (pCAStream->pCfg->enmDir == PDMAUDIODIR_IN)
            {
                rc = coreAudioStreamInvalidateQueue(pCAStream);
                if (RT_SUCCESS(rc))
                {
                    /* Start the audio queue immediately. */
                    AudioQueueStart(pCAStream->audioQueue, NULL);
                }
            }
            else if (pCAStream->pCfg->enmDir == PDMAUDIODIR_OUT)
            {
                /* Touch the run flag to start the audio queue as soon as
                 * we have anough data to actually play something. */
                ASMAtomicXchgBool(&pCAStream->fRun, true);
            }
            break;
        }

        case PDMAUDIOSTREAMCMD_DISABLE:
        {
            LogFunc(("Queue disable\n"));
            AudioQueueStop(pCAStream->audioQueue, 1 /* Immediately */);
            ASMAtomicXchgBool(&pCAStream->fRun,       false);
            ASMAtomicXchgBool(&pCAStream->fIsRunning, false);
            break;
        }
        case PDMAUDIOSTREAMCMD_PAUSE:
        {
            LogFunc(("Queue pause\n"));
            AudioQueuePause(pCAStream->audioQueue);
            ASMAtomicXchgBool(&pCAStream->fIsRunning, false);
            break;
        }

        default:
            rc = VERR_NOT_SUPPORTED;
            break;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetConfig}
 */
static DECLCALLBACK(int) drvHostCoreAudioGetConfig(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDCFG pBackendCfg)
{
    AssertPtrReturn(pInterface,  VERR_INVALID_POINTER);
    AssertPtrReturn(pBackendCfg, VERR_INVALID_POINTER);

    RT_BZERO(pBackendCfg, sizeof(PDMAUDIOBACKENDCFG));

    pBackendCfg->cbStreamIn  = sizeof(COREAUDIOSTREAM);
    pBackendCfg->cbStreamOut = sizeof(COREAUDIOSTREAM);

    pBackendCfg->cMaxStreamsIn  = UINT32_MAX;
    pBackendCfg->cMaxStreamsOut = UINT32_MAX;

    LogFlowFunc(("Returning %Rrc\n", VINF_SUCCESS));
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetDevices}
 */
static DECLCALLBACK(int) drvHostCoreAudioGetDevices(PPDMIHOSTAUDIO pInterface, PPDMAUDIODEVICEENUM pDeviceEnum)
{
    AssertPtrReturn(pInterface,  VERR_INVALID_POINTER);
    AssertPtrReturn(pDeviceEnum, VERR_INVALID_POINTER);

    PDRVHOSTCOREAUDIO pThis = PDMIHOSTAUDIO_2_DRVHOSTCOREAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_SUCCESS(rc))
    {
        rc = coreAudioEnumerateDevices(pThis);
        if (RT_SUCCESS(rc))
        {
            if (pDeviceEnum)
            {
                rc = DrvAudioHlpDeviceEnumInit(pDeviceEnum);
                if (RT_SUCCESS(rc))
                    rc = DrvAudioHlpDeviceEnumCopy(pDeviceEnum, &pThis->Devices);

                if (RT_FAILURE(rc))
                    DrvAudioHlpDeviceEnumFree(pDeviceEnum);
            }
        }

        int rc2 = RTCritSectLeave(&pThis->CritSect);
        AssertRC(rc2);
    }

    LogFlowFunc(("Returning %Rrc\n", rc));
    return rc;
}


#ifdef VBOX_WITH_AUDIO_CALLBACKS
/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnSetCallback}
 */
static DECLCALLBACK(int) drvHostCoreAudioSetCallback(PPDMIHOSTAUDIO pInterface, PFNPDMHOSTAUDIOCALLBACK pfnCallback)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    /* pfnCallback will be handled below. */

    PDRVHOSTCOREAUDIO pThis = PDMIHOSTAUDIO_2_DRVHOSTCOREAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_SUCCESS(rc))
    {
        LogFunc(("pfnCallback=%p\n", pfnCallback));

        if (pfnCallback) /* Register. */
        {
            Assert(pThis->pfnCallback == NULL);
            pThis->pfnCallback = pfnCallback;
        }
        else /* Unregister. */
        {
            if (pThis->pfnCallback)
                pThis->pfnCallback = NULL;
        }

        int rc2 = RTCritSectLeave(&pThis->CritSect);
        AssertRC(rc2);
    }

    return rc;
}
#endif


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetStatus}
 */
static DECLCALLBACK(PDMAUDIOBACKENDSTS) drvHostCoreAudioGetStatus(PPDMIHOSTAUDIO pInterface, PDMAUDIODIR enmDir)
{
    RT_NOREF(pInterface, enmDir);
    AssertPtrReturn(pInterface, PDMAUDIOBACKENDSTS_UNKNOWN);

    return PDMAUDIOBACKENDSTS_RUNNING;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamCreate}
 */
static DECLCALLBACK(int) drvHostCoreAudioStreamCreate(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream,
                                                      PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgReq,    VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgAcq,    VERR_INVALID_POINTER);

    PDRVHOSTCOREAUDIO pThis     = PDMIHOSTAUDIO_2_DRVHOSTCOREAUDIO(pInterface);
    PCOREAUDIOSTREAM  pCAStream = (PCOREAUDIOSTREAM)pStream;

    int rc = RTCritSectInit(&pCAStream->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    pCAStream->hThread    = NIL_RTTHREAD;
    pCAStream->fRun       = false;
    pCAStream->fIsRunning = false;
    pCAStream->fShutdown  = false;

    /* Input or output device? */
    bool fIn = pCfgReq->enmDir == PDMAUDIODIR_IN;

    /* For now, just use the default device available. */
    PPDMAUDIODEVICE pDev = fIn ? pThis->pDefaultDevIn : pThis->pDefaultDevOut;

    LogFunc(("pStream=%p, pCfgReq=%p, pCfgAcq=%p, fIn=%RTbool, pDev=%p\n", pStream, pCfgReq, pCfgAcq, fIn, pDev));

    if (pDev) /* (Default) device available? */
    {
        /* Sanity. */
        AssertPtr(pDev->pvData);
        Assert(pDev->cbData);

        /* Init the Core Audio stream. */
        rc = coreAudioStreamInit(pCAStream, pThis, pDev);
        if (RT_SUCCESS(rc))
        {
            ASMAtomicXchgU32(&pCAStream->enmStatus, COREAUDIOSTATUS_IN_INIT);

            rc = coreAudioStreamInitQueue(pCAStream, pCfgReq, pCfgAcq);
            if (RT_SUCCESS(rc))
            {
                pCfgAcq->cFrameBufferHint = AQ_BUF_SAMPLES; /** @todo Make this configurable. */
            }
            if (RT_SUCCESS(rc))
            {
                ASMAtomicXchgU32(&pCAStream->enmStatus, COREAUDIOSTATUS_INIT);
            }
            else
            {
                ASMAtomicXchgU32(&pCAStream->enmStatus, COREAUDIOSTATUS_IN_UNINIT);

                int rc2 = coreAudioStreamUninit(pCAStream);
                AssertRC(rc2);

                ASMAtomicXchgU32(&pCAStream->enmStatus, COREAUDIOSTATUS_UNINIT);
            }
        }
    }
    else
        rc = VERR_NOT_AVAILABLE;

    LogFunc(("Returning %Rrc\n", rc));
    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamDestroy}
 */
static DECLCALLBACK(int) drvHostCoreAudioStreamDestroy(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    PDRVHOSTCOREAUDIO pThis = PDMIHOSTAUDIO_2_DRVHOSTCOREAUDIO(pInterface);

    PCOREAUDIOSTREAM pCAStream = (PCOREAUDIOSTREAM)pStream;

    uint32_t status = ASMAtomicReadU32(&pCAStream->enmStatus);
    if (!(   status == COREAUDIOSTATUS_INIT
#ifndef VBOX_WITH_AUDIO_CALLBACKS
          || status == COREAUDIOSTATUS_REINIT
#endif
          ))
    {
        AssertFailed();
        return VINF_SUCCESS;
    }

    if (!pCAStream->pCfg) /* Not (yet) configured? Skip. */
        return VINF_SUCCESS;

    int rc = coreAudioStreamControl(pThis, pCAStream, PDMAUDIOSTREAMCMD_DISABLE);
    if (RT_SUCCESS(rc))
    {
        ASMAtomicXchgU32(&pCAStream->enmStatus, COREAUDIOSTATUS_IN_UNINIT);

        rc = coreAudioStreamUninit(pCAStream);

        if (RT_SUCCESS(rc))
            ASMAtomicXchgU32(&pCAStream->enmStatus, COREAUDIOSTATUS_UNINIT);
    }

    if (RT_SUCCESS(rc))
    {
        if (RTCritSectIsInitialized(&pCAStream->CritSect))
            RTCritSectDelete(&pCAStream->CritSect);
    }

    LogFunc(("rc=%Rrc\n", rc));
    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamControl}
 */
static DECLCALLBACK(int) drvHostCoreAudioStreamControl(PPDMIHOSTAUDIO pInterface,
                                                       PPDMAUDIOBACKENDSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    PDRVHOSTCOREAUDIO pThis    = PDMIHOSTAUDIO_2_DRVHOSTCOREAUDIO(pInterface);
    PCOREAUDIOSTREAM pCAStream = (PCOREAUDIOSTREAM)pStream;

    return coreAudioStreamControl(pThis, pCAStream, enmStreamCmd);
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetReadable}
 */
static DECLCALLBACK(uint32_t) drvHostCoreAudioStreamGetReadable(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    PCOREAUDIOSTREAM pCAStream = (PCOREAUDIOSTREAM)pStream;

    if (ASMAtomicReadU32(&pCAStream->enmStatus) != COREAUDIOSTATUS_INIT)
        return 0;

    AssertPtr(pCAStream->pCfg);
    AssertPtr(pCAStream->pCircBuf);

    switch (pCAStream->pCfg->enmDir)
    {
        case PDMAUDIODIR_IN:
            return (uint32_t)RTCircBufUsed(pCAStream->pCircBuf);

        case PDMAUDIODIR_OUT:
        default:
            AssertFailed();
            break;
    }

    return 0;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetWritable}
 */
static DECLCALLBACK(uint32_t) drvHostCoreAudioStreamGetWritable(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    PCOREAUDIOSTREAM pCAStream = (PCOREAUDIOSTREAM)pStream;

    if (ASMAtomicReadU32(&pCAStream->enmStatus) != COREAUDIOSTATUS_INIT)
        return 0;

    AssertPtr(pCAStream->pCfg);
    AssertPtr(pCAStream->pCircBuf);

    switch (pCAStream->pCfg->enmDir)
    {
        case PDMAUDIODIR_OUT:
            return (uint32_t)RTCircBufFree(pCAStream->pCircBuf);

        case PDMAUDIODIR_IN:
        default:
            AssertFailed();
            break;
    }

    return 0;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetStatus}
 */
static DECLCALLBACK(PDMAUDIOSTREAMSTS) drvHostCoreAudioStreamGetStatus(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    PCOREAUDIOSTREAM pCAStream = (PCOREAUDIOSTREAM)pStream;

    PDMAUDIOSTREAMSTS strmSts = PDMAUDIOSTREAMSTS_FLAG_NONE;

    if (!pCAStream->pCfg) /* Not (yet) configured? Skip. */
        return strmSts;

    if (ASMAtomicReadU32(&pCAStream->enmStatus) == COREAUDIOSTATUS_INIT)
        strmSts |= PDMAUDIOSTREAMSTS_FLAG_INITIALIZED | PDMAUDIOSTREAMSTS_FLAG_ENABLED;

    return strmSts;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamIterate}
 */
static DECLCALLBACK(int) drvHostCoreAudioStreamIterate(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    RT_NOREF(pInterface, pStream);

    /* Nothing to do here for Core Audio. */
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnInit}
 */
static DECLCALLBACK(int) drvHostCoreAudioInit(PPDMIHOSTAUDIO pInterface)
{
    PDRVHOSTCOREAUDIO pThis = PDMIHOSTAUDIO_2_DRVHOSTCOREAUDIO(pInterface);

    int rc = DrvAudioHlpDeviceEnumInit(&pThis->Devices);
    if (RT_SUCCESS(rc))
    {
        /* Do the first (initial) internal device enumeration. */
        rc = coreAudioEnumerateDevices(pThis);
    }

    if (RT_SUCCESS(rc))
    {
        /* Register system callbacks. */
        AudioObjectPropertyAddress propAdr = { kAudioHardwarePropertyDefaultInputDevice, kAudioObjectPropertyScopeGlobal,
                                               kAudioObjectPropertyElementMaster };

        OSStatus err = AudioObjectAddPropertyListener(kAudioObjectSystemObject, &propAdr,
                                                      coreAudioDefaultDeviceChangedCb, pThis /* pvUser */);
        if (   err != noErr
            && err != kAudioHardwareIllegalOperationError)
        {
            LogRel(("CoreAudio: Failed to add the input default device changed listener (%RI32)\n", err));
        }

        propAdr.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
        err = AudioObjectAddPropertyListener(kAudioObjectSystemObject, &propAdr,
                                             coreAudioDefaultDeviceChangedCb, pThis /* pvUser */);
        if (   err != noErr
            && err != kAudioHardwareIllegalOperationError)
        {
            LogRel(("CoreAudio: Failed to add the output default device changed listener (%RI32)\n", err));
        }
    }

    LogFlowFunc(("Returning %Rrc\n", rc));
    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnShutdown}
 */
static DECLCALLBACK(void) drvHostCoreAudioShutdown(PPDMIHOSTAUDIO pInterface)
{
    PDRVHOSTCOREAUDIO pThis = PDMIHOSTAUDIO_2_DRVHOSTCOREAUDIO(pInterface);

    /*
     * Unregister system callbacks.
     */
    AudioObjectPropertyAddress propAdr = { kAudioHardwarePropertyDefaultInputDevice, kAudioObjectPropertyScopeGlobal,
                                           kAudioObjectPropertyElementMaster };

    OSStatus err = AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &propAdr,
                                                     coreAudioDefaultDeviceChangedCb, pThis /* pvUser */);
    if (   err != noErr
        && err != kAudioHardwareBadObjectError)
    {
        LogRel(("CoreAudio: Failed to remove the default input device changed listener (%RI32)\n", err));
    }

    propAdr.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
    err = AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &propAdr,
                                            coreAudioDefaultDeviceChangedCb, pThis /* pvUser */);
    if (   err != noErr
        && err != kAudioHardwareBadObjectError)
    {
        LogRel(("CoreAudio: Failed to remove the default output device changed listener (%RI32)\n", err));
    }

    LogFlowFuncEnter();
}


/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) drvHostCoreAudioQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS        pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVHOSTCOREAUDIO pThis   = PDMINS_2_DATA(pDrvIns, PDRVHOSTCOREAUDIO);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE,      &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIHOSTAUDIO, &pThis->IHostAudio);

    return NULL;
}


/**
 * @callback_method_impl{FNPDMDRVCONSTRUCT,
 *      Construct a Core Audio driver instance.}
 */
static DECLCALLBACK(int) drvHostCoreAudioConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    RT_NOREF(pCfg, fFlags);
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);
    PDRVHOSTCOREAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTCOREAUDIO);
    LogRel(("Audio: Initializing Core Audio driver\n"));

    /*
     * Init the static parts.
     */
    pThis->pDrvIns                   = pDrvIns;
    /* IBase */
    pDrvIns->IBase.pfnQueryInterface = drvHostCoreAudioQueryInterface;
    /* IHostAudio */
    PDMAUDIO_IHOSTAUDIO_CALLBACKS(drvHostCoreAudio);

    /* This backend supports device enumeration. */
    pThis->IHostAudio.pfnGetDevices  = drvHostCoreAudioGetDevices;

#ifdef VBOX_WITH_AUDIO_CALLBACKS
    /* This backend supports host audio callbacks. */
    pThis->IHostAudio.pfnSetCallback = drvHostCoreAudioSetCallback;
    pThis->pfnCallback               = NULL;
#endif

    int rc = RTCritSectInit(&pThis->CritSect);

#ifdef VBOX_AUDIO_DEBUG_DUMP_PCM_DATA
    RTFileDelete(VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "caConverterCbInput.pcm");
    RTFileDelete(VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "caPlayback.pcm");
#endif

    LogFlowFuncLeaveRC(rc);
    return rc;
}


/**
 * @callback_method_impl{FNPDMDRVDESTRUCT}
 */
static DECLCALLBACK(void) drvHostCoreAudioDestruct(PPDMDRVINS pDrvIns)
{
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);
    PDRVHOSTCOREAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTCOREAUDIO);

    int rc2 = RTCritSectDelete(&pThis->CritSect);
    AssertRC(rc2);

    LogFlowFuncLeaveRC(rc2);
}


/**
 * Char driver registration record.
 */
const PDMDRVREG g_DrvHostCoreAudio =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "CoreAudio",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "Core Audio host driver",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_AUDIO,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(DRVHOSTCOREAUDIO),
    /* pfnConstruct */
    drvHostCoreAudioConstruct,
    /* pfnDestruct */
    drvHostCoreAudioDestruct,
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

