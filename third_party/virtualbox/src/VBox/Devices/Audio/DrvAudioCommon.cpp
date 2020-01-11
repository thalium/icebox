/* $Id: DrvAudioCommon.cpp $ */
/** @file
 * Intermedia audio driver, common routines.
 *
 * These are also used in the drivers which are bound to Main, e.g. the VRDE
 * or the video audio recording drivers.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/alloc.h>
#include <iprt/asm-math.h>
#include <iprt/assert.h>
#include <iprt/dir.h>
#include <iprt/file.h>
#include <iprt/string.h>
#include <iprt/uuid.h>

#define LOG_GROUP LOG_GROUP_DRV_AUDIO
#include <VBox/log.h>

#include <VBox/err.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdm.h>
#include <VBox/vmm/mm.h>

#include <ctype.h>
#include <stdlib.h>

#include "DrvAudio.h"
#include "AudioMixBuffer.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Structure for building up a .WAV file header.
 */
typedef struct AUDIOWAVFILEHDR
{
    uint32_t u32RIFF;
    uint32_t u32Size;
    uint32_t u32WAVE;

    uint32_t u32Fmt;
    uint32_t u32Size1;
    uint16_t u16AudioFormat;
    uint16_t u16NumChannels;
    uint32_t u32SampleRate;
    uint32_t u32ByteRate;
    uint16_t u16BlockAlign;
    uint16_t u16BitsPerSample;

    uint32_t u32ID2;
    uint32_t u32Size2;
} AUDIOWAVFILEHDR, *PAUDIOWAVFILEHDR;
AssertCompileSize(AUDIOWAVFILEHDR, 11*4);

/**
 * Structure for keeeping the internal .WAV file data
 */
typedef struct AUDIOWAVFILEDATA
{
    /** The file header/footer. */
    AUDIOWAVFILEHDR Hdr;
} AUDIOWAVFILEDATA, *PAUDIOWAVFILEDATA;




/**
 * Retrieves the matching PDMAUDIOFMT for given bits + signing flag.
 *
 * @return  IPRT status code.
 * @return  PDMAUDIOFMT         Resulting audio format or PDMAUDIOFMT_INVALID if invalid.
 * @param   cBits               Bits to retrieve audio format for.
 * @param   fSigned             Signed flag for bits to retrieve audio format for.
 */
PDMAUDIOFMT DrvAudioAudFmtBitsToAudFmt(uint8_t cBits, bool fSigned)
{
    if (fSigned)
    {
        switch (cBits)
        {
            case 8:  return PDMAUDIOFMT_S8;
            case 16: return PDMAUDIOFMT_S16;
            case 32: return PDMAUDIOFMT_S32;
            default: break;
        }
    }
    else
    {
        switch (cBits)
        {
            case 8:  return PDMAUDIOFMT_U8;
            case 16: return PDMAUDIOFMT_U16;
            case 32: return PDMAUDIOFMT_U32;
            default: break;
        }
    }

    AssertMsgFailed(("Bogus audio bits %RU8\n", cBits));
    return PDMAUDIOFMT_INVALID;
}

/**
 * Clears a sample buffer by the given amount of audio frames with silence (according to the format
 * given by the PCM properties).
 *
 * @param   pPCMProps               PCM properties to use for the buffer to clear.
 * @param   pvBuf                   Buffer to clear.
 * @param   cbBuf                   Size (in bytes) of the buffer.
 * @param   cFrames                 Number of audio frames to clear in the buffer.
 */
void DrvAudioHlpClearBuf(const PPDMAUDIOPCMPROPS pPCMProps, void *pvBuf, size_t cbBuf, uint32_t cFrames)
{
    AssertPtrReturnVoid(pPCMProps);
    AssertPtrReturnVoid(pvBuf);

    if (!cbBuf || !cFrames)
        return;

    Assert(pPCMProps->cBytes);
    size_t cbToClear = DrvAudioHlpFramesToBytes(cFrames, pPCMProps);
    Assert(cbBuf >= cbToClear);

    if (cbBuf < cbToClear)
        cbToClear = cbBuf;

    Log2Func(("pPCMProps=%p, pvBuf=%p, cFrames=%RU32, fSigned=%RTbool, cBytes=%RU8\n",
              pPCMProps, pvBuf, cFrames, pPCMProps->fSigned, pPCMProps->cBytes));

    Assert(pPCMProps->fSwapEndian == false); /** @todo Swapping Endianness is not supported yet. */

    if (pPCMProps->fSigned)
    {
        RT_BZERO(pvBuf, cbToClear);
    }
    else /* Unsigned formats. */
    {
        switch (pPCMProps->cBytes)
        {
            case 1: /* 8 bit */
            {
                memset(pvBuf, 0x80, cbToClear);
                break;
            }

            case 2: /* 16 bit */
            {
                uint16_t *p = (uint16_t *)pvBuf;
                uint16_t  s = 0x0080;

                for (uint32_t i = 0; i < DrvAudioHlpBytesToFrames((uint32_t)cbToClear, pPCMProps); i++)
                    p[i] = s;

                break;
            }

            /** @todo Add 24 bit? */

            case 4: /* 32 bit */
            {
                uint32_t *p = (uint32_t *)pvBuf;
                uint32_t  s = 0x00000080;

                for (uint32_t i = 0; i < DrvAudioHlpBytesToFrames((uint32_t)cbToClear, pPCMProps); i++)
                    p[i] = s;

                break;
            }

            default:
            {
                AssertMsgFailed(("Invalid bytes per sample: %RU8\n", pPCMProps->cBytes));
                break;
            }
        }
    }
}

/**
 * Returns an unique file name for this given audio connector instance.
 *
 * @return  Allocated file name. Must be free'd using RTStrFree().
 * @param   uInstance           Driver / device instance.
 * @param   pszPath             Path name of the file to delete. The path must exist.
 * @param   pszSuffix           File name suffix to use.
 */
char *DrvAudioDbgGetFileNameA(uint8_t uInstance, const char *pszPath, const char *pszSuffix)
{
    char szFileName[64];
    RTStrPrintf(szFileName, sizeof(szFileName), "drvAudio%RU8-%s", uInstance, pszSuffix);

    char szFilePath[RTPATH_MAX];
    int rc2 = RTStrCopy(szFilePath, sizeof(szFilePath), pszPath);
    AssertRC(rc2);
    rc2 = RTPathAppend(szFilePath, sizeof(szFilePath), szFileName);
    AssertRC(rc2);

    return RTStrDup(szFilePath);
}

/**
 * Allocates an audio device.
 *
 * @returns Newly allocated audio device, or NULL if failed.
 * @param   cbData              How much additional data (in bytes) should be allocated to provide
 *                              a (backend) specific area to store additional data.
 *                              Optional, can be 0.
 */
PPDMAUDIODEVICE DrvAudioHlpDeviceAlloc(size_t cbData)
{
    PPDMAUDIODEVICE pDev = (PPDMAUDIODEVICE)RTMemAllocZ(sizeof(PDMAUDIODEVICE));
    if (!pDev)
        return NULL;

    if (cbData)
    {
        pDev->pvData = RTMemAllocZ(cbData);
        if (!pDev->pvData)
        {
            RTMemFree(pDev);
            return NULL;
        }
    }

    pDev->cbData = cbData;

    pDev->cMaxInputChannels  = 0;
    pDev->cMaxOutputChannels = 0;

    return pDev;
}

/**
 * Frees an audio device.
 *
 * @param pDev                  Device to free.
 */
void DrvAudioHlpDeviceFree(PPDMAUDIODEVICE pDev)
{
    if (!pDev)
        return;

    Assert(pDev->cRefCount == 0);

    if (pDev->pvData)
    {
        Assert(pDev->cbData);

        RTMemFree(pDev->pvData);
        pDev->pvData = NULL;
    }

    RTMemFree(pDev);
    pDev = NULL;
}

/**
 * Duplicates an audio device entry.
 *
 * @returns Duplicated audio device entry on success, or NULL on failure.
 * @param   pDev                Audio device entry to duplicate.
 * @param   fCopyUserData       Whether to also copy the user data portion or not.
 */
PPDMAUDIODEVICE DrvAudioHlpDeviceDup(const PPDMAUDIODEVICE pDev, bool fCopyUserData)
{
    AssertPtrReturn(pDev, NULL);

    PPDMAUDIODEVICE pDevDup = DrvAudioHlpDeviceAlloc(fCopyUserData ? pDev->cbData : 0);
    if (pDevDup)
    {
        memcpy(pDevDup, pDev, sizeof(PDMAUDIODEVICE));

        if (   fCopyUserData
            && pDevDup->cbData)
        {
            memcpy(pDevDup->pvData, pDev->pvData, pDevDup->cbData);
        }
        else
        {
            pDevDup->cbData = 0;
            pDevDup->pvData = NULL;
        }
    }

    return pDevDup;
}

/**
 * Initializes an audio device enumeration structure.
 *
 * @returns IPRT status code.
 * @param   pDevEnm             Device enumeration to initialize.
 */
int DrvAudioHlpDeviceEnumInit(PPDMAUDIODEVICEENUM pDevEnm)
{
    AssertPtrReturn(pDevEnm, VERR_INVALID_POINTER);

    RTListInit(&pDevEnm->lstDevices);
    pDevEnm->cDevices = 0;

    return VINF_SUCCESS;
}

/**
 * Frees audio device enumeration data.
 *
 * @param pDevEnm               Device enumeration to destroy.
 */
void DrvAudioHlpDeviceEnumFree(PPDMAUDIODEVICEENUM pDevEnm)
{
    if (!pDevEnm)
        return;

    PPDMAUDIODEVICE pDev, pDevNext;
    RTListForEachSafe(&pDevEnm->lstDevices, pDev, pDevNext, PDMAUDIODEVICE, Node)
    {
        RTListNodeRemove(&pDev->Node);

        DrvAudioHlpDeviceFree(pDev);

        pDevEnm->cDevices--;
    }

    /* Sanity. */
    Assert(RTListIsEmpty(&pDevEnm->lstDevices));
    Assert(pDevEnm->cDevices == 0);
}

/**
 * Adds an audio device to a device enumeration.
 *
 * @return IPRT status code.
 * @param  pDevEnm              Device enumeration to add device to.
 * @param  pDev                 Device to add. The pointer will be owned by the device enumeration  then.
 */
int DrvAudioHlpDeviceEnumAdd(PPDMAUDIODEVICEENUM pDevEnm, PPDMAUDIODEVICE pDev)
{
    AssertPtrReturn(pDevEnm, VERR_INVALID_POINTER);
    AssertPtrReturn(pDev,    VERR_INVALID_POINTER);

    RTListAppend(&pDevEnm->lstDevices, &pDev->Node);
    pDevEnm->cDevices++;

    return VINF_SUCCESS;
}

/**
 * Duplicates a device enumeration.
 *
 * @returns Duplicated device enumeration, or NULL on failure.
 *          Must be free'd with DrvAudioHlpDeviceEnumFree().
 * @param   pDevEnm             Device enumeration to duplicate.
 */
PPDMAUDIODEVICEENUM DrvAudioHlpDeviceEnumDup(const PPDMAUDIODEVICEENUM pDevEnm)
{
    AssertPtrReturn(pDevEnm, NULL);

    PPDMAUDIODEVICEENUM pDevEnmDup = (PPDMAUDIODEVICEENUM)RTMemAlloc(sizeof(PDMAUDIODEVICEENUM));
    if (!pDevEnmDup)
        return NULL;

    int rc2 = DrvAudioHlpDeviceEnumInit(pDevEnmDup);
    AssertRC(rc2);

    PPDMAUDIODEVICE pDev;
    RTListForEach(&pDevEnm->lstDevices, pDev, PDMAUDIODEVICE, Node)
    {
        PPDMAUDIODEVICE pDevDup = DrvAudioHlpDeviceDup(pDev, true /* fCopyUserData */);
        if (!pDevDup)
        {
            rc2 = VERR_NO_MEMORY;
            break;
        }

        rc2 = DrvAudioHlpDeviceEnumAdd(pDevEnmDup, pDevDup);
        if (RT_FAILURE(rc2))
        {
            DrvAudioHlpDeviceFree(pDevDup);
            break;
        }
    }

    if (RT_FAILURE(rc2))
    {
        DrvAudioHlpDeviceEnumFree(pDevEnmDup);
        pDevEnmDup = NULL;
    }

    return pDevEnmDup;
}

/**
 * Copies device enumeration entries from the source to the destination enumeration.
 *
 * @returns IPRT status code.
 * @param   pDstDevEnm          Destination enumeration to store enumeration entries into.
 * @param   pSrcDevEnm          Source enumeration to use.
 * @param   enmUsage            Which entries to copy. Specify PDMAUDIODIR_ANY to copy all entries.
 * @param   fCopyUserData       Whether to also copy the user data portion or not.
 */
int DrvAudioHlpDeviceEnumCopyEx(PPDMAUDIODEVICEENUM pDstDevEnm, const PPDMAUDIODEVICEENUM pSrcDevEnm,
                                PDMAUDIODIR enmUsage, bool fCopyUserData)
{
    AssertPtrReturn(pDstDevEnm, VERR_INVALID_POINTER);
    AssertPtrReturn(pSrcDevEnm, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;

    PPDMAUDIODEVICE pSrcDev;
    RTListForEach(&pSrcDevEnm->lstDevices, pSrcDev, PDMAUDIODEVICE, Node)
    {
        if (   enmUsage != PDMAUDIODIR_ANY
            && enmUsage != pSrcDev->enmUsage)
        {
            continue;
        }

        PPDMAUDIODEVICE pDstDev = DrvAudioHlpDeviceDup(pSrcDev, fCopyUserData);
        if (!pDstDev)
        {
            rc = VERR_NO_MEMORY;
            break;
        }

        rc = DrvAudioHlpDeviceEnumAdd(pDstDevEnm, pDstDev);
        if (RT_FAILURE(rc))
            break;
    }

    return rc;
}

/**
 * Copies all device enumeration entries from the source to the destination enumeration.
 *
 * Note: Does *not* copy the user-specific data assigned to a device enumeration entry.
 *       To do so, use DrvAudioHlpDeviceEnumCopyEx().
 *
 * @returns IPRT status code.
 * @param   pDstDevEnm          Destination enumeration to store enumeration entries into.
 * @param   pSrcDevEnm          Source enumeration to use.
 */
int DrvAudioHlpDeviceEnumCopy(PPDMAUDIODEVICEENUM pDstDevEnm, const PPDMAUDIODEVICEENUM pSrcDevEnm)
{
    return DrvAudioHlpDeviceEnumCopyEx(pDstDevEnm, pSrcDevEnm, PDMAUDIODIR_ANY, false /* fCopyUserData */);
}

/**
 * Returns the default device of a given device enumeration.
 * This assumes that only one default device per usage is set.
 *
 * @returns Default device if found, or NULL if none found.
 * @param   pDevEnm             Device enumeration to get default device for.
 * @param   enmUsage            Usage to get default device for.
 */
PPDMAUDIODEVICE DrvAudioHlpDeviceEnumGetDefaultDevice(const PPDMAUDIODEVICEENUM pDevEnm, PDMAUDIODIR enmUsage)
{
    AssertPtrReturn(pDevEnm, NULL);

    PPDMAUDIODEVICE pDev;
    RTListForEach(&pDevEnm->lstDevices, pDev, PDMAUDIODEVICE, Node)
    {
        if (enmUsage != PDMAUDIODIR_ANY)
        {
            if (enmUsage != pDev->enmUsage) /* Wrong usage? Skip. */
                continue;
        }

        if (pDev->fFlags & PDMAUDIODEV_FLAGS_DEFAULT)
            return pDev;
    }

    return NULL;
}

/**
 * Returns the number of enumerated devices of a given device enumeration.
 *
 * @returns Number of devices if found, or 0 if none found.
 * @param   pDevEnm             Device enumeration to get default device for.
 * @param   enmUsage            Usage to get default device for.
 */
uint16_t DrvAudioHlpDeviceEnumGetDeviceCount(const PPDMAUDIODEVICEENUM pDevEnm, PDMAUDIODIR enmUsage)
{
    AssertPtrReturn(pDevEnm, 0);

    if (enmUsage == PDMAUDIODIR_ANY)
        return pDevEnm->cDevices;

    uint32_t cDevs = 0;

    PPDMAUDIODEVICE pDev;
    RTListForEach(&pDevEnm->lstDevices, pDev, PDMAUDIODEVICE, Node)
    {
        if (enmUsage == pDev->enmUsage)
            cDevs++;
    }

    return cDevs;
}

/**
 * Logs an audio device enumeration.
 *
 * @param  pszDesc              Logging description.
 * @param  pDevEnm              Device enumeration to log.
 */
void DrvAudioHlpDeviceEnumPrint(const char *pszDesc, const PPDMAUDIODEVICEENUM pDevEnm)
{
    AssertPtrReturnVoid(pszDesc);
    AssertPtrReturnVoid(pDevEnm);

    LogFunc(("%s: %RU16 devices\n", pszDesc, pDevEnm->cDevices));

    PPDMAUDIODEVICE pDev;
    RTListForEach(&pDevEnm->lstDevices, pDev, PDMAUDIODEVICE, Node)
    {
        char *pszFlags = DrvAudioHlpAudDevFlagsToStrA(pDev->fFlags);

        LogFunc(("Device '%s':\n", pDev->szName));
        LogFunc(("\tUsage           = %s\n",             DrvAudioHlpAudDirToStr(pDev->enmUsage)));
        LogFunc(("\tFlags           = %s\n",             pszFlags ? pszFlags : "<NONE>"));
        LogFunc(("\tInput channels  = %RU8\n",           pDev->cMaxInputChannels));
        LogFunc(("\tOutput channels = %RU8\n",           pDev->cMaxOutputChannels));
        LogFunc(("\tData            = %p (%zu bytes)\n", pDev->pvData, pDev->cbData));

        if (pszFlags)
            RTStrFree(pszFlags);
    }
}

/**
 * Converts an audio direction to a string.
 *
 * @returns Stringified audio direction, or "Unknown", if not found.
 * @param   enmDir              Audio direction to convert.
 */
const char *DrvAudioHlpAudDirToStr(PDMAUDIODIR enmDir)
{
    switch (enmDir)
    {
        case PDMAUDIODIR_UNKNOWN: return "Unknown";
        case PDMAUDIODIR_IN:      return "Input";
        case PDMAUDIODIR_OUT:     return "Output";
        case PDMAUDIODIR_ANY:     return "Duplex";
        default:                  break;
    }

    AssertMsgFailed(("Invalid audio direction %ld\n", enmDir));
    return "Unknown";
}

/**
 * Converts an audio mixer control to a string.
 *
 * @returns Stringified audio mixer control or "Unknown", if not found.
 * @param   enmMixerCtl         Audio mixer control to convert.
 */
const char *DrvAudioHlpAudMixerCtlToStr(PDMAUDIOMIXERCTL enmMixerCtl)
{
    switch (enmMixerCtl)
    {
        case PDMAUDIOMIXERCTL_VOLUME_MASTER: return "Master Volume";
        case PDMAUDIOMIXERCTL_FRONT:         return "Front";
        case PDMAUDIOMIXERCTL_CENTER_LFE:    return "Center / LFE";
        case PDMAUDIOMIXERCTL_REAR:          return "Rear";
        case PDMAUDIOMIXERCTL_LINE_IN:       return "Line-In";
        case PDMAUDIOMIXERCTL_MIC_IN:        return "Microphone-In";
        default:                             break;
    }

    AssertMsgFailed(("Invalid mixer control %ld\n", enmMixerCtl));
    return "Unknown";
}

/**
 * Converts an audio device flags to a string.
 *
 * @returns Stringified audio flags. Must be free'd with RTStrFree().
 *          NULL if no flags set.
 * @param   fFlags              Audio flags to convert.
 */
char *DrvAudioHlpAudDevFlagsToStrA(PDMAUDIODEVFLAG fFlags)
{
#define APPEND_FLAG_TO_STR(_aFlag)              \
    if (fFlags & PDMAUDIODEV_FLAGS_##_aFlag)    \
    {                                           \
        if (pszFlags)                           \
        {                                       \
            rc2 = RTStrAAppend(&pszFlags, " "); \
            if (RT_FAILURE(rc2))                \
                break;                          \
        }                                       \
                                                \
        rc2 = RTStrAAppend(&pszFlags, #_aFlag); \
        if (RT_FAILURE(rc2))                    \
            break;                              \
    }                                           \

    char *pszFlags = NULL;
    int rc2 = VINF_SUCCESS;

    do
    {
        APPEND_FLAG_TO_STR(DEFAULT);
        APPEND_FLAG_TO_STR(HOTPLUG);
        APPEND_FLAG_TO_STR(BUGGY);
        APPEND_FLAG_TO_STR(IGNORE);
        APPEND_FLAG_TO_STR(LOCKED);
        APPEND_FLAG_TO_STR(DEAD);

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

/**
 * Converts a playback destination enumeration to a string.
 *
 * @returns Stringified playback destination, or "Unknown", if not found.
 * @param   enmPlaybackDst      Playback destination to convert.
 */
const char *DrvAudioHlpPlaybackDstToStr(const PDMAUDIOPLAYBACKDEST enmPlaybackDst)
{
    switch (enmPlaybackDst)
    {
        case PDMAUDIOPLAYBACKDEST_UNKNOWN:    return "Unknown";
        case PDMAUDIOPLAYBACKDEST_FRONT:      return "Front";
        case PDMAUDIOPLAYBACKDEST_CENTER_LFE: return "Center / LFE";
        case PDMAUDIOPLAYBACKDEST_REAR:       return "Rear";
        default:
            break;
    }

    AssertMsgFailed(("Invalid playback destination %ld\n", enmPlaybackDst));
    return "Unknown";
}

/**
 * Converts a recording source enumeration to a string.
 *
 * @returns Stringified recording source, or "Unknown", if not found.
 * @param   enmRecSrc           Recording source to convert.
 */
const char *DrvAudioHlpRecSrcToStr(const PDMAUDIORECSOURCE enmRecSrc)
{
    switch (enmRecSrc)
    {
        case PDMAUDIORECSOURCE_UNKNOWN: return "Unknown";
        case PDMAUDIORECSOURCE_MIC:     return "Microphone In";
        case PDMAUDIORECSOURCE_CD:      return "CD";
        case PDMAUDIORECSOURCE_VIDEO:   return "Video";
        case PDMAUDIORECSOURCE_AUX:     return "AUX";
        case PDMAUDIORECSOURCE_LINE:    return "Line In";
        case PDMAUDIORECSOURCE_PHONE:   return "Phone";
        default:
            break;
    }

    AssertMsgFailed(("Invalid recording source %ld\n", enmRecSrc));
    return "Unknown";
}

/**
 * Returns wether the given audio format has signed bits or not.
 *
 * @return  IPRT status code.
 * @return  bool                @c true for signed bits, @c false for unsigned.
 * @param   enmFmt              Audio format to retrieve value for.
 */
bool DrvAudioHlpAudFmtIsSigned(PDMAUDIOFMT enmFmt)
{
    switch (enmFmt)
    {
        case PDMAUDIOFMT_S8:
        case PDMAUDIOFMT_S16:
        case PDMAUDIOFMT_S32:
            return true;

        case PDMAUDIOFMT_U8:
        case PDMAUDIOFMT_U16:
        case PDMAUDIOFMT_U32:
            return false;

        default:
            break;
    }

    AssertMsgFailed(("Bogus audio format %ld\n", enmFmt));
    return false;
}

/**
 * Returns the bits of a given audio format.
 *
 * @return  IPRT status code.
 * @return  uint8_t             Bits of audio format.
 * @param   enmFmt              Audio format to retrieve value for.
 */
uint8_t DrvAudioHlpAudFmtToBits(PDMAUDIOFMT enmFmt)
{
    switch (enmFmt)
    {
        case PDMAUDIOFMT_S8:
        case PDMAUDIOFMT_U8:
            return 8;

        case PDMAUDIOFMT_U16:
        case PDMAUDIOFMT_S16:
            return 16;

        case PDMAUDIOFMT_U32:
        case PDMAUDIOFMT_S32:
            return 32;

        default:
            break;
    }

    AssertMsgFailed(("Bogus audio format %ld\n", enmFmt));
    return 0;
}

/**
 * Converts an audio format to a string.
 *
 * @returns Stringified audio format, or "Unknown", if not found.
 * @param   enmFmt              Audio format to convert.
 */
const char *DrvAudioHlpAudFmtToStr(PDMAUDIOFMT enmFmt)
{
    switch (enmFmt)
    {
        case PDMAUDIOFMT_U8:
            return "U8";

        case PDMAUDIOFMT_U16:
            return "U16";

        case PDMAUDIOFMT_U32:
            return "U32";

        case PDMAUDIOFMT_S8:
            return "S8";

        case PDMAUDIOFMT_S16:
            return "S16";

        case PDMAUDIOFMT_S32:
            return "S32";

        default:
            break;
    }

    AssertMsgFailed(("Bogus audio format %ld\n", enmFmt));
    return "Unknown";
}

/**
 * Converts a given string to an audio format.
 *
 * @returns Audio format for the given string, or PDMAUDIOFMT_INVALID if not found.
 * @param   pszFmt              String to convert to an audio format.
 */
PDMAUDIOFMT DrvAudioHlpStrToAudFmt(const char *pszFmt)
{
    AssertPtrReturn(pszFmt, PDMAUDIOFMT_INVALID);

    if (!RTStrICmp(pszFmt, "u8"))
        return PDMAUDIOFMT_U8;
    else if (!RTStrICmp(pszFmt, "u16"))
        return PDMAUDIOFMT_U16;
    else if (!RTStrICmp(pszFmt, "u32"))
        return PDMAUDIOFMT_U32;
    else if (!RTStrICmp(pszFmt, "s8"))
        return PDMAUDIOFMT_S8;
    else if (!RTStrICmp(pszFmt, "s16"))
        return PDMAUDIOFMT_S16;
    else if (!RTStrICmp(pszFmt, "s32"))
        return PDMAUDIOFMT_S32;

    AssertMsgFailed(("Invalid audio format '%s'\n", pszFmt));
    return PDMAUDIOFMT_INVALID;
}

/**
 * Checks whether two given PCM properties are equal.
 *
 * @returns @c true if equal, @c false if not.
 * @param   pProps1             First properties to compare.
 * @param   pProps2             Second properties to compare.
 */
bool DrvAudioHlpPCMPropsAreEqual(const PPDMAUDIOPCMPROPS pProps1, const PPDMAUDIOPCMPROPS pProps2)
{
    AssertPtrReturn(pProps1, false);
    AssertPtrReturn(pProps2, false);

    if (pProps1 == pProps2) /* If the pointers match, take a shortcut. */
        return true;

    return    pProps1->uHz         == pProps2->uHz
           && pProps1->cChannels   == pProps2->cChannels
           && pProps1->cBytes      == pProps2->cBytes
           && pProps1->fSigned     == pProps2->fSigned
           && pProps1->fSwapEndian == pProps2->fSwapEndian;
}

/**
 * Checks whether given PCM properties are valid or not.
 *
 * Returns @c true if properties are valid, @c false if not.
 * @param   pProps              PCM properties to check.
 */
bool DrvAudioHlpPCMPropsAreValid(const PPDMAUDIOPCMPROPS pProps)
{
    AssertPtrReturn(pProps, false);

    /* Minimum 1 channel (mono), maximum 7.1 (= 8) channels. */
    bool fValid = (   pProps->cChannels >= 1
                   && pProps->cChannels <= 8);

    if (fValid)
    {
        switch (pProps->cBytes)
        {
            case 1: /* 8 bit */
               if (pProps->fSigned)
                   fValid = false;
               break;
            case 2: /* 16 bit */
                if (!pProps->fSigned)
                    fValid = false;
                break;
            /** @todo Do we need support for 24 bit samples? */
            case 4: /* 32 bit */
                if (!pProps->fSigned)
                    fValid = false;
                break;
            default:
                fValid = false;
                break;
        }
    }

    if (!fValid)
        return false;

    fValid &= pProps->uHz > 0;
    fValid &= pProps->cShift == PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(pProps->cBytes, pProps->cChannels);
    fValid &= pProps->fSwapEndian == false; /** @todo Handling Big Endian audio data is not supported yet. */

    return fValid;
}

/**
 * Checks whether the given PCM properties are equal with the given
 * stream configuration.
 *
 * @returns @c true if equal, @c false if not.
 * @param   pProps              PCM properties to compare.
 * @param   pCfg                Stream configuration to compare.
 */
bool DrvAudioHlpPCMPropsAreEqual(const PPDMAUDIOPCMPROPS pProps, const PPDMAUDIOSTREAMCFG pCfg)
{
    AssertPtrReturn(pProps, false);
    AssertPtrReturn(pCfg,   false);

    return DrvAudioHlpPCMPropsAreEqual(pProps, &pCfg->Props);
}

/**
 * Returns the bytes per frame for given PCM properties.
 *
 * @return Bytes per (audio) frame.
 * @param  pProps               PCM properties to retrieve bytes per frame for.
 */
uint32_t DrvAudioHlpPCMPropsBytesPerFrame(const PPDMAUDIOPCMPROPS pProps)
{
    return PDMAUDIOPCMPROPS_F2B(pProps, 1 /* Frame */);
}

/**
 * Prints PCM properties to the debug log.
 *
 * @param   pProps              Stream configuration to log.
 */
void DrvAudioHlpPCMPropsPrint(const PPDMAUDIOPCMPROPS pProps)
{
    AssertPtrReturnVoid(pProps);

    Log(("uHz=%RU32, cChannels=%RU8, cBits=%RU8%s",
         pProps->uHz, pProps->cChannels, pProps->cBytes * 8, pProps->fSigned ? "S" : "U"));
}

/**
 * Converts PCM properties to a audio stream configuration.
 *
 * @return  IPRT status code.
 * @param   pProps              PCM properties to convert.
 * @param   pCfg                Stream configuration to store result into.
 */
int DrvAudioHlpPCMPropsToStreamCfg(const PPDMAUDIOPCMPROPS pProps, PPDMAUDIOSTREAMCFG pCfg)
{
    AssertPtrReturn(pProps, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfg,   VERR_INVALID_POINTER);

    DrvAudioHlpStreamCfgInit(pCfg);

    memcpy(&pCfg->Props, pProps, sizeof(PDMAUDIOPCMPROPS));
    return VINF_SUCCESS;
}

/**
 * Initializes a stream configuration with its default values.
 *
 * @param   pCfg                Stream configuration to initialize.
 */
void DrvAudioHlpStreamCfgInit(PPDMAUDIOSTREAMCFG pCfg)
{
    AssertPtrReturnVoid(pCfg);

    RT_BZERO(pCfg, sizeof(PDMAUDIOSTREAMCFG));

    pCfg->Backend.cfPreBuf = UINT32_MAX; /* Explicitly set to "undefined". */
}

/**
 * Checks whether a given stream configuration is valid or not.
 *
 * Returns @c true if configuration is valid, @c false if not.
 * @param   pCfg                Stream configuration to check.
 */
bool DrvAudioHlpStreamCfgIsValid(const PPDMAUDIOSTREAMCFG pCfg)
{
    AssertPtrReturn(pCfg, false);

    bool fValid = (   pCfg->enmDir == PDMAUDIODIR_IN
                   || pCfg->enmDir == PDMAUDIODIR_OUT);

    fValid &= (   pCfg->enmLayout == PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED
               || pCfg->enmLayout == PDMAUDIOSTREAMLAYOUT_RAW);

    if (fValid)
        fValid = DrvAudioHlpPCMPropsAreValid(&pCfg->Props);

    return fValid;
}

/**
 * Frees an allocated audio stream configuration.
 *
 * @param   pCfg                Audio stream configuration to free.
 */
void DrvAudioHlpStreamCfgFree(PPDMAUDIOSTREAMCFG pCfg)
{
    if (pCfg)
    {
        RTMemFree(pCfg);
        pCfg = NULL;
    }
}

/**
 * Copies a source stream configuration to a destination stream configuration.
 *
 * @returns IPRT status code.
 * @param   pDstCfg             Destination stream configuration to copy source to.
 * @param   pSrcCfg             Source stream configuration to copy to destination.
 */
int DrvAudioHlpStreamCfgCopy(PPDMAUDIOSTREAMCFG pDstCfg, const PPDMAUDIOSTREAMCFG pSrcCfg)
{
    AssertPtrReturn(pDstCfg, VERR_INVALID_POINTER);
    AssertPtrReturn(pSrcCfg, VERR_INVALID_POINTER);

#ifdef VBOX_STRICT
    if (!DrvAudioHlpStreamCfgIsValid(pSrcCfg))
    {
        AssertMsgFailed(("Stream config '%s' (%p) is invalid\n", pSrcCfg->szName, pSrcCfg));
        return VERR_INVALID_PARAMETER;
    }
#endif

    memcpy(pDstCfg, pSrcCfg, sizeof(PDMAUDIOSTREAMCFG));

    return VINF_SUCCESS;
}

/**
 * Duplicates an audio stream configuration.
 * Must be free'd with DrvAudioHlpStreamCfgFree().
 *
 * @return  Duplicates audio stream configuration on success, or NULL on failure.
 * @param   pCfg                    Audio stream configuration to duplicate.
 */
PPDMAUDIOSTREAMCFG DrvAudioHlpStreamCfgDup(const PPDMAUDIOSTREAMCFG pCfg)
{
    AssertPtrReturn(pCfg, NULL);

#ifdef VBOX_STRICT
    if (!DrvAudioHlpStreamCfgIsValid(pCfg))
    {
        AssertMsgFailed(("Stream config '%s' (%p) is invalid\n", pCfg->szName, pCfg));
        return NULL;
    }
#endif

    PPDMAUDIOSTREAMCFG pDst = (PPDMAUDIOSTREAMCFG)RTMemAllocZ(sizeof(PDMAUDIOSTREAMCFG));
    if (!pDst)
        return NULL;

    int rc2 = DrvAudioHlpStreamCfgCopy(pDst, pCfg);
    if (RT_FAILURE(rc2))
    {
        DrvAudioHlpStreamCfgFree(pDst);
        pDst = NULL;
    }

    AssertPtr(pDst);
    return pDst;
}

/**
 * Prints an audio stream configuration to the debug log.
 *
 * @param   pCfg                Stream configuration to log.
 */
void DrvAudioHlpStreamCfgPrint(const PPDMAUDIOSTREAMCFG pCfg)
{
    if (!pCfg)
        return;

    LogFunc(("szName=%s, enmDir=%RU32 (uHz=%RU32, cBits=%RU8%s, cChannels=%RU8)\n",
             pCfg->szName, pCfg->enmDir,
             pCfg->Props.uHz, pCfg->Props.cBytes * 8, pCfg->Props.fSigned ? "S" : "U", pCfg->Props.cChannels));
}

/**
 * Converts a stream command to a string.
 *
 * @returns Stringified stream command, or "Unknown", if not found.
 * @param   enmCmd              Stream command to convert.
 */
const char *DrvAudioHlpStreamCmdToStr(PDMAUDIOSTREAMCMD enmCmd)
{
    switch (enmCmd)
    {
        case PDMAUDIOSTREAMCMD_UNKNOWN: return "Unknown";
        case PDMAUDIOSTREAMCMD_ENABLE:  return "Enable";
        case PDMAUDIOSTREAMCMD_DISABLE: return "Disable";
        case PDMAUDIOSTREAMCMD_PAUSE:   return "Pause";
        case PDMAUDIOSTREAMCMD_RESUME:  return "Resume";
        case PDMAUDIOSTREAMCMD_DRAIN:   return "Drain";
        case PDMAUDIOSTREAMCMD_DROP:    return "Drop";
        default:                        break;
    }

    AssertMsgFailed(("Invalid stream command %ld\n", enmCmd));
    return "Unknown";
}

/**
 * Returns @c true if the given stream status indicates a can-be-read-from stream,
 * @c false if not.
 *
 * @returns @c true if ready to be read from, @c if not.
 * @param   enmStatus           Stream status to evaluate.
 */
bool DrvAudioHlpStreamStatusCanRead(PDMAUDIOSTREAMSTS enmStatus)
{
    AssertReturn(enmStatus & PDMAUDIOSTREAMSTS_VALID_MASK, false);

    return      enmStatus & PDMAUDIOSTREAMSTS_FLAG_INITIALIZED
           &&   enmStatus & PDMAUDIOSTREAMSTS_FLAG_ENABLED
           && !(enmStatus & PDMAUDIOSTREAMSTS_FLAG_PAUSED)
           && !(enmStatus & PDMAUDIOSTREAMSTS_FLAG_PENDING_REINIT);
}

/**
 * Returns @c true if the given stream status indicates a can-be-written-to stream,
 * @c false if not.
 *
 * @returns @c true if ready to be written to, @c if not.
 * @param   enmStatus           Stream status to evaluate.
 */
bool DrvAudioHlpStreamStatusCanWrite(PDMAUDIOSTREAMSTS enmStatus)
{
    AssertReturn(enmStatus & PDMAUDIOSTREAMSTS_VALID_MASK, false);

    return      enmStatus & PDMAUDIOSTREAMSTS_FLAG_INITIALIZED
           &&   enmStatus & PDMAUDIOSTREAMSTS_FLAG_ENABLED
           && !(enmStatus & PDMAUDIOSTREAMSTS_FLAG_PAUSED)
           && !(enmStatus & PDMAUDIOSTREAMSTS_FLAG_PENDING_DISABLE)
           && !(enmStatus & PDMAUDIOSTREAMSTS_FLAG_PENDING_REINIT);
}

/**
 * Returns @c true if the given stream status indicates a ready-to-operate stream,
 * @c false if not.
 *
 * @returns @c true if ready to operate, @c if not.
 * @param   enmStatus           Stream status to evaluate.
 */
bool DrvAudioHlpStreamStatusIsReady(PDMAUDIOSTREAMSTS enmStatus)
{
    AssertReturn(enmStatus & PDMAUDIOSTREAMSTS_VALID_MASK, false);

    return      enmStatus & PDMAUDIOSTREAMSTS_FLAG_INITIALIZED
           &&   enmStatus & PDMAUDIOSTREAMSTS_FLAG_ENABLED
           && !(enmStatus & PDMAUDIOSTREAMSTS_FLAG_PENDING_REINIT);
}

/**
 * Calculates the audio bit rate of the given bits per sample, the Hz and the number
 * of audio channels.
 *
 * Divide the result by 8 to get the byte rate.
 *
 * @returns The calculated bit rate.
 * @param   cBits               Number of bits per sample.
 * @param   uHz                 Hz (Hertz) rate.
 * @param   cChannels           Number of audio channels.
 */
uint32_t DrvAudioHlpCalcBitrate(uint8_t cBits, uint32_t uHz, uint8_t cChannels)
{
    return (cBits * uHz * cChannels);
}

/**
 * Calculates the audio bit rate out of a given audio stream configuration.
 *
 * Divide the result by 8 to get the byte rate.
 *
 * @returns The calculated bit rate.
 * @param   pProps              PCM properties to calculate bitrate for.
 *
 * @remark
 */
uint32_t DrvAudioHlpCalcBitrate(const PPDMAUDIOPCMPROPS pProps)
{
    return DrvAudioHlpCalcBitrate(pProps->cBytes * 8, pProps->uHz, pProps->cChannels);
}

/**
 * Aligns the given byte amount to the given PCM properties and returns the aligned
 * size.
 *
 * @return  Aligned size (in bytes).
 * @param   cbSize              Size (in bytes) to align.
 * @param   pProps              PCM properties to align size to.
 */
uint32_t DrvAudioHlpBytesAlign(uint32_t cbSize, const PPDMAUDIOPCMPROPS pProps)
{
    AssertPtrReturn(pProps, 0);

    if (!cbSize)
        return 0;

    return PDMAUDIOPCMPROPS_B2F(pProps, cbSize) * PDMAUDIOPCMPROPS_F2B(pProps, 1 /* Frame */);
}

/**
 * Returns if the the given size is properly aligned to the given PCM properties.
 *
 * @return  @c true if properly aligned, @c false if not.
 * @param   cbSize              Size (in bytes) to check alignment for.
 * @param   pProps              PCM properties to use for checking the alignment.
 */
bool DrvAudioHlpBytesIsAligned(uint32_t cbSize, const PPDMAUDIOPCMPROPS pProps)
{
    AssertPtrReturn(pProps, 0);

    if (!cbSize)
        return true;

    return (cbSize % PDMAUDIOPCMPROPS_F2B(pProps, 1 /* Frame */) == 0);
}

/**
 * Returns the bytes per second for given PCM properties.
 *
 * @returns Bytes per second.
 * @param   pProps              PCM properties to retrieve size for.
 */
DECLINLINE(uint64_t) drvAudioHlpBytesPerSec(const PPDMAUDIOPCMPROPS pProps)
{
    return PDMAUDIOPCMPROPS_F2B(pProps, 1 /* Frame */) * pProps->uHz;
}

/**
 * Returns the number of audio frames for a given amount of bytes.
 *
 * @return Calculated audio frames for given bytes.
 * @param  cbBytes              Bytes to convert to audio frames.
 * @param  pProps               PCM properties to calulate frames for.
 */
uint32_t DrvAudioHlpBytesToFrames(uint32_t cbBytes, const PPDMAUDIOPCMPROPS pProps)
{
    AssertPtrReturn(pProps, 0);

    return PDMAUDIOPCMPROPS_B2F(pProps, cbBytes);
}

/**
 * Returns the time (in ms) for given byte amount and PCM properties.
 *
 * @return  uint64_t            Calculated time (in ms).
 * @param   cbBytes             Amount of bytes to calculate time for.
 * @param   pProps              PCM properties to calculate amount of bytes for.
 *
 * @note    Does rounding up the result.
 */
uint64_t DrvAudioHlpBytesToMilli(uint32_t cbBytes, const PPDMAUDIOPCMPROPS pProps)
{
    AssertPtrReturn(pProps, 0);

    if (!pProps->uHz) /* Prevent division by zero. */
        return 0;

    const unsigned cbFrame = PDMAUDIOPCMPROPS_F2B(pProps, 1 /* Frame */);

    if (!cbFrame) /* Prevent division by zero. */
        return 0;

    uint64_t uTimeMs = ((cbBytes + cbFrame - 1) / cbFrame) * RT_MS_1SEC;

    return (uTimeMs + pProps->uHz - 1) / pProps->uHz;
}

/**
 * Returns the time (in us) for given byte amount and PCM properties.
 *
 * @return  uint64_t            Calculated time (in us).
 * @param   cbBytes             Amount of bytes to calculate time for.
 * @param   pProps              PCM properties to calculate amount of bytes for.
 *
 * @note    Does rounding up the result.
 */
uint64_t DrvAudioHlpBytesToMicro(uint32_t cbBytes, const PPDMAUDIOPCMPROPS pProps)
{
    AssertPtrReturn(pProps, 0);

    if (!pProps->uHz) /* Prevent division by zero. */
        return 0;

    const unsigned cbFrame = PDMAUDIOPCMPROPS_F2B(pProps, 1 /* Frame */);

    if (!cbFrame) /* Prevent division by zero. */
        return 0;

    uint64_t uTimeUs = ((cbBytes + cbFrame - 1) / cbFrame) * RT_US_1SEC;

    return (uTimeUs + pProps->uHz - 1) / pProps->uHz;
}

/**
 * Returns the time (in ns) for given byte amount and PCM properties.
 *
 * @return  uint64_t            Calculated time (in ns).
 * @param   cbBytes             Amount of bytes to calculate time for.
 * @param   pProps              PCM properties to calculate amount of bytes for.
 *
 * @note    Does rounding up the result.
 */
uint64_t DrvAudioHlpBytesToNano(uint32_t cbBytes, const PPDMAUDIOPCMPROPS pProps)
{
    AssertPtrReturn(pProps, 0);

    if (!pProps->uHz) /* Prevent division by zero. */
        return 0;

    const unsigned cbFrame = PDMAUDIOPCMPROPS_F2B(pProps, 1 /* Frame */);

    if (!cbFrame) /* Prevent division by zero. */
        return 0;

    uint64_t uTimeNs = ((cbBytes + cbFrame - 1) / cbFrame) * RT_NS_1SEC;

    return (uTimeNs + pProps->uHz - 1) / pProps->uHz;
}

/**
 * Returns the bytes for a given audio frames amount and PCM properties.
 *
 * @return Calculated bytes for given audio frames.
 * @param  cFrames              Amount of audio frames to calculate bytes for.
 * @param  pProps               PCM properties to calculate bytes for.
 */
uint32_t DrvAudioHlpFramesToBytes(uint32_t cFrames, const PPDMAUDIOPCMPROPS pProps)
{
    AssertPtrReturn(pProps, 0);

    if (!cFrames)
        return 0;

    return cFrames * PDMAUDIOPCMPROPS_F2B(pProps, 1 /* Frame */);
}

/**
 * Returns the time (in ms) for given audio frames amount and PCM properties.
 *
 * @return  uint64_t            Calculated time (in ms).
 * @param   cFrames             Amount of audio frames to calculate time for.
 * @param   pProps              PCM properties to calculate time (in ms) for.
 */
uint64_t DrvAudioHlpFramesToMilli(uint32_t cFrames, const PPDMAUDIOPCMPROPS pProps)
{
    AssertPtrReturn(pProps, 0);

    if (!cFrames)
        return 0;

    if (!pProps->uHz) /* Prevent division by zero. */
        return 0;

    return cFrames / ((double)pProps->uHz / (double)RT_MS_1SEC);
}

/**
 * Returns the time (in ns) for given audio frames amount and PCM properties.
 *
 * @return  uint64_t            Calculated time (in ns).
 * @param   cFrames             Amount of audio frames to calculate time for.
 * @param   pProps              PCM properties to calculate time (in ns) for.
 */
uint64_t DrvAudioHlpFramesToNano(uint32_t cFrames, const PPDMAUDIOPCMPROPS pProps)
{
    AssertPtrReturn(pProps, 0);

    if (!cFrames)
        return 0;

    if (!pProps->uHz) /* Prevent division by zero. */
        return 0;

    return cFrames / ((double)pProps->uHz / (double)RT_NS_1SEC);
}

/**
 * Returns the amount of bytes for a given time (in ms) and PCM properties.
 *
 * Note: The result will return an amount of bytes which is aligned to the audio frame size.
 *
 * @return  uint32_t            Calculated amount of bytes.
 * @param   uMs                 Time (in ms) to calculate amount of bytes for.
 * @param   pProps              PCM properties to calculate amount of bytes for.
 */
uint32_t DrvAudioHlpMilliToBytes(uint64_t uMs, const PPDMAUDIOPCMPROPS pProps)
{
    AssertPtrReturn(pProps, 0);

    if (!uMs)
        return 0;

    const uint32_t uBytesPerFrame = DrvAudioHlpPCMPropsBytesPerFrame(pProps);

    uint32_t uBytes = ((double)drvAudioHlpBytesPerSec(pProps) / (double)RT_MS_1SEC) * uMs;
    if (uBytes % uBytesPerFrame) /* Any remainder? Make the returned bytes an integral number to the given frames. */
        uBytes = uBytes + (uBytesPerFrame - uBytes % uBytesPerFrame);

    Assert(uBytes % uBytesPerFrame == 0); /* Paranoia. */

    return uBytes;
}

/**
 * Returns the amount of bytes for a given time (in ns) and PCM properties.
 *
 * Note: The result will return an amount of bytes which is aligned to the audio frame size.
 *
 * @return  uint32_t            Calculated amount of bytes.
 * @param   uNs                 Time (in ns) to calculate amount of bytes for.
 * @param   pProps              PCM properties to calculate amount of bytes for.
 */
uint32_t DrvAudioHlpNanoToBytes(uint64_t uNs, const PPDMAUDIOPCMPROPS pProps)
{
    AssertPtrReturn(pProps, 0);

    if (!uNs)
        return 0;

    const uint32_t uBytesPerFrame = DrvAudioHlpPCMPropsBytesPerFrame(pProps);

    uint32_t uBytes = ((double)drvAudioHlpBytesPerSec(pProps) / (double)RT_NS_1SEC) * uNs;
    if (uBytes % uBytesPerFrame) /* Any remainder? Make the returned bytes an integral number to the given frames. */
        uBytes = uBytes + (uBytesPerFrame - uBytes % uBytesPerFrame);

    Assert(uBytes % uBytesPerFrame == 0); /* Paranoia. */

    return uBytes;
}

/**
 * Returns the amount of audio frames for a given time (in ms) and PCM properties.
 *
 * @return  uint32_t            Calculated amount of audio frames.
 * @param   uMs                 Time (in ms) to calculate amount of frames for.
 * @param   pProps              PCM properties to calculate amount of frames for.
 */
uint32_t DrvAudioHlpMilliToFrames(uint64_t uMs, const PPDMAUDIOPCMPROPS pProps)
{
    AssertPtrReturn(pProps, 0);

    const uint32_t cbFrame = PDMAUDIOPCMPROPS_F2B(pProps, 1 /* Frame */);
    if (!cbFrame) /* Prevent division by zero. */
        return 0;

    return DrvAudioHlpMilliToBytes(uMs, pProps) / cbFrame;
}

/**
 * Returns the amount of audio frames for a given time (in ns) and PCM properties.
 *
 * @return  uint32_t            Calculated amount of audio frames.
 * @param   uNs                 Time (in ns) to calculate amount of frames for.
 * @param   pProps              PCM properties to calculate amount of frames for.
 */
uint32_t DrvAudioHlpNanoToFrames(uint64_t uNs, const PPDMAUDIOPCMPROPS pProps)
{
    AssertPtrReturn(pProps, 0);

    const uint32_t cbFrame = PDMAUDIOPCMPROPS_F2B(pProps, 1 /* Frame */);
    if (!cbFrame) /* Prevent division by zero. */
        return 0;

    return DrvAudioHlpNanoToBytes(uNs, pProps) / cbFrame;
}

/**
 * Sanitizes the file name component so that unsupported characters
 * will be replaced by an underscore ("_").
 *
 * @return  IPRT status code.
 * @param   pszPath             Path to sanitize.
 * @param   cbPath              Size (in bytes) of path to sanitize.
 */
int DrvAudioHlpFileNameSanitize(char *pszPath, size_t cbPath)
{
    RT_NOREF(cbPath);
    int rc = VINF_SUCCESS;
#ifdef RT_OS_WINDOWS
    /* Filter out characters not allowed on Windows platforms, put in by
       RTTimeSpecToString(). */
    /** @todo Use something like RTPathSanitize() if available later some time. */
    static RTUNICP const s_uszValidRangePairs[] =
    {
        ' ', ' ',
        '(', ')',
        '-', '.',
        '0', '9',
        'A', 'Z',
        'a', 'z',
        '_', '_',
        0xa0, 0xd7af,
        '\0'
    };
    ssize_t cReplaced = RTStrPurgeComplementSet(pszPath, s_uszValidRangePairs, '_' /* Replacement */);
    if (cReplaced < 0)
        rc = VERR_INVALID_UTF8_ENCODING;
#else
    RT_NOREF(pszPath);
#endif
    return rc;
}

/**
 * Constructs an unique file name, based on the given path and the audio file type.
 *
 * @returns IPRT status code.
 * @param   pszFile             Where to store the constructed file name.
 * @param   cchFile             Size (in characters) of the file name buffer.
 * @param   pszPath             Base path to use.
 * @param   pszName             A name for better identifying the file.
 * @param   uInstance           Device / driver instance which is using this file.
 * @param   enmType             Audio file type to construct file name for.
 * @param   fFlags              File naming flags.
 */
int DrvAudioHlpFileNameGet(char *pszFile, size_t cchFile, const char *pszPath, const char *pszName,
                           uint32_t uInstance, PDMAUDIOFILETYPE enmType, PDMAUDIOFILENAMEFLAGS fFlags)
{
    AssertPtrReturn(pszFile, VERR_INVALID_POINTER);
    AssertReturn(cchFile,    VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszPath, VERR_INVALID_POINTER);
    AssertPtrReturn(pszName, VERR_INVALID_POINTER);
    /** @todo Validate fFlags. */

    int rc;

    do
    {
        char szFilePath[RTPATH_MAX + 1];
        RTStrPrintf2(szFilePath, sizeof(szFilePath), "%s", pszPath);

        /* Create it when necessary. */
        if (!RTDirExists(szFilePath))
        {
            rc = RTDirCreateFullPath(szFilePath, RTFS_UNIX_IRWXU);
            if (RT_FAILURE(rc))
                break;
        }

        char szFileName[RTPATH_MAX + 1];
        szFileName[0] = '\0';

        if (fFlags & PDMAUDIOFILENAME_FLAG_TS)
        {
            RTTIMESPEC time;
            if (!RTTimeSpecToString(RTTimeNow(&time), szFileName, sizeof(szFileName)))
            {
                rc = VERR_BUFFER_OVERFLOW;
                break;
            }

            rc = DrvAudioHlpFileNameSanitize(szFileName, sizeof(szFileName));
            if (RT_FAILURE(rc))
                break;

            rc = RTStrCat(szFileName, sizeof(szFileName), "-");
            if (RT_FAILURE(rc))
                break;
        }

        rc = RTStrCat(szFileName, sizeof(szFileName), pszName);
        if (RT_FAILURE(rc))
            break;

        rc = RTStrCat(szFileName, sizeof(szFileName), "-");
        if (RT_FAILURE(rc))
            break;

        char szInst[16];
        RTStrPrintf2(szInst, sizeof(szInst), "%RU32", uInstance);
        rc = RTStrCat(szFileName, sizeof(szFileName), szInst);
        if (RT_FAILURE(rc))
            break;

        switch (enmType)
        {
            case PDMAUDIOFILETYPE_RAW:
                rc = RTStrCat(szFileName, sizeof(szFileName), ".pcm");
                break;

            case PDMAUDIOFILETYPE_WAV:
                rc = RTStrCat(szFileName, sizeof(szFileName), ".wav");
                break;

            default:
                AssertFailedStmt(rc = VERR_NOT_IMPLEMENTED);
                break;
        }

        if (RT_FAILURE(rc))
            break;

        rc = RTPathAppend(szFilePath, sizeof(szFilePath), szFileName);
        if (RT_FAILURE(rc))
            break;

        RTStrPrintf2(pszFile, cchFile, "%s", szFilePath);

    } while (0);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Creates an audio file.
 *
 * @returns IPRT status code.
 * @param   enmType             Audio file type to open / create.
 * @param   pszFile             File path of file to open or create.
 * @param   fFlags              Audio file flags.
 * @param   ppFile              Where to store the created audio file handle.
 *                              Needs to be destroyed with DrvAudioHlpFileDestroy().
 */
int DrvAudioHlpFileCreate(PDMAUDIOFILETYPE enmType, const char *pszFile, PDMAUDIOFILEFLAGS fFlags, PPDMAUDIOFILE *ppFile)
{
    AssertPtrReturn(pszFile, VERR_INVALID_POINTER);
    /** @todo Validate fFlags. */

    PPDMAUDIOFILE pFile = (PPDMAUDIOFILE)RTMemAlloc(sizeof(PDMAUDIOFILE));
    if (!pFile)
        return VERR_NO_MEMORY;

    int rc = VINF_SUCCESS;

    switch (enmType)
    {
        case PDMAUDIOFILETYPE_RAW:
        case PDMAUDIOFILETYPE_WAV:
            pFile->enmType = enmType;
            break;

        default:
            rc = VERR_INVALID_PARAMETER;
            break;
    }

    if (RT_SUCCESS(rc))
    {
        RTStrPrintf(pFile->szName, RT_ELEMENTS(pFile->szName), "%s", pszFile);
        pFile->hFile  = NIL_RTFILE;
        pFile->fFlags = fFlags;
        pFile->pvData = NULL;
        pFile->cbData = 0;
    }

    if (RT_FAILURE(rc))
    {
        RTMemFree(pFile);
        pFile = NULL;
    }
    else
        *ppFile = pFile;

    return rc;
}

/**
 * Destroys a formerly created audio file.
 *
 * @param   pFile               Audio file (object) to destroy.
 */
void DrvAudioHlpFileDestroy(PPDMAUDIOFILE pFile)
{
    if (!pFile)
        return;

    DrvAudioHlpFileClose(pFile);

    RTMemFree(pFile);
    pFile = NULL;
}

/**
 * Opens or creates an audio file.
 *
 * @returns IPRT status code.
 * @param   pFile               Pointer to audio file handle to use.
 * @param   fOpen               Open flags.
 *                              Use PDMAUDIOFILE_DEFAULT_OPEN_FLAGS for the default open flags.
 * @param   pProps              PCM properties to use.
 */
int DrvAudioHlpFileOpen(PPDMAUDIOFILE pFile, uint32_t fOpen, const PPDMAUDIOPCMPROPS pProps)
{
    AssertPtrReturn(pFile,   VERR_INVALID_POINTER);
    /** @todo Validate fOpen flags. */
    AssertPtrReturn(pProps,  VERR_INVALID_POINTER);

    int rc;

    if (pFile->enmType == PDMAUDIOFILETYPE_RAW)
    {
        rc = RTFileOpen(&pFile->hFile, pFile->szName, fOpen);
    }
    else if (pFile->enmType == PDMAUDIOFILETYPE_WAV)
    {
        Assert(pProps->cChannels);
        Assert(pProps->uHz);
        Assert(pProps->cBytes);

        pFile->pvData = (PAUDIOWAVFILEDATA)RTMemAllocZ(sizeof(AUDIOWAVFILEDATA));
        if (pFile->pvData)
        {
            pFile->cbData = sizeof(PAUDIOWAVFILEDATA);

            PAUDIOWAVFILEDATA pData = (PAUDIOWAVFILEDATA)pFile->pvData;
            AssertPtr(pData);

            /* Header. */
            pData->Hdr.u32RIFF          = AUDIO_MAKE_FOURCC('R','I','F','F');
            pData->Hdr.u32Size          = 36;
            pData->Hdr.u32WAVE          = AUDIO_MAKE_FOURCC('W','A','V','E');

            pData->Hdr.u32Fmt           = AUDIO_MAKE_FOURCC('f','m','t',' ');
            pData->Hdr.u32Size1         = 16; /* Means PCM. */
            pData->Hdr.u16AudioFormat   = 1;  /* PCM, linear quantization. */
            pData->Hdr.u16NumChannels   = pProps->cChannels;
            pData->Hdr.u32SampleRate    = pProps->uHz;
            pData->Hdr.u32ByteRate      = DrvAudioHlpCalcBitrate(pProps) / 8;
            pData->Hdr.u16BlockAlign    = pProps->cChannels * pProps->cBytes;
            pData->Hdr.u16BitsPerSample = pProps->cBytes * 8;

            /* Data chunk. */
            pData->Hdr.u32ID2           = AUDIO_MAKE_FOURCC('d','a','t','a');
            pData->Hdr.u32Size2         = 0;

            rc = RTFileOpen(&pFile->hFile, pFile->szName, fOpen);
            if (RT_SUCCESS(rc))
            {
                rc = RTFileWrite(pFile->hFile, &pData->Hdr, sizeof(pData->Hdr), NULL);
                if (RT_FAILURE(rc))
                {
                    RTFileClose(pFile->hFile);
                    pFile->hFile = NIL_RTFILE;
                }
            }

            if (RT_FAILURE(rc))
            {
                RTMemFree(pFile->pvData);
                pFile->pvData = NULL;
                pFile->cbData = 0;
            }
        }
        else
            rc = VERR_NO_MEMORY;
    }
    else
        rc = VERR_INVALID_PARAMETER;

    if (RT_SUCCESS(rc))
    {
        LogRel2(("Audio: Opened file '%s'\n", pFile->szName));
    }
    else
        LogRel(("Audio: Failed opening file '%s', rc=%Rrc\n", pFile->szName, rc));

    return rc;
}

/**
 * Closes an audio file.
 *
 * @returns IPRT status code.
 * @param   pFile               Audio file handle to close.
 */
int DrvAudioHlpFileClose(PPDMAUDIOFILE pFile)
{
    if (!pFile)
        return VINF_SUCCESS;

    size_t cbSize = DrvAudioHlpFileGetDataSize(pFile);

    int rc = VINF_SUCCESS;

    if (pFile->enmType == PDMAUDIOFILETYPE_RAW)
    {
        if (RTFileIsValid(pFile->hFile))
            rc = RTFileClose(pFile->hFile);
    }
    else if (pFile->enmType == PDMAUDIOFILETYPE_WAV)
    {
        if (RTFileIsValid(pFile->hFile))
        {
            PAUDIOWAVFILEDATA pData = (PAUDIOWAVFILEDATA)pFile->pvData;
            if (pData) /* The .WAV file data only is valid when a file actually has been created. */
            {
                /* Update the header with the current data size. */
                RTFileWriteAt(pFile->hFile, 0, &pData->Hdr, sizeof(pData->Hdr), NULL);
            }

            rc = RTFileClose(pFile->hFile);
        }

        if (pFile->pvData)
        {
            RTMemFree(pFile->pvData);
            pFile->pvData = NULL;
        }
    }
    else
        AssertFailedStmt(rc = VERR_NOT_IMPLEMENTED);

    if (   RT_SUCCESS(rc)
        && !cbSize
        && !(pFile->fFlags & PDMAUDIOFILE_FLAG_KEEP_IF_EMPTY))
    {
        rc = DrvAudioHlpFileDelete(pFile);
    }

    pFile->cbData = 0;

    if (RT_SUCCESS(rc))
    {
        pFile->hFile = NIL_RTFILE;
        LogRel2(("Audio: Closed file '%s' (%zu bytes)\n", pFile->szName, cbSize));
    }
    else
        LogRel(("Audio: Failed closing file '%s', rc=%Rrc\n", pFile->szName, rc));

    return rc;
}

/**
 * Deletes an audio file.
 *
 * @returns IPRT status code.
 * @param   pFile               Audio file handle to delete.
 */
int DrvAudioHlpFileDelete(PPDMAUDIOFILE pFile)
{
    AssertPtrReturn(pFile, VERR_INVALID_POINTER);

    int rc = RTFileDelete(pFile->szName);
    if (RT_SUCCESS(rc))
    {
        LogRel2(("Audio: Deleted file '%s'\n", pFile->szName));
    }
    else if (rc == VERR_FILE_NOT_FOUND) /* Don't bitch if the file is not around (anymore). */
        rc = VINF_SUCCESS;

    if (RT_FAILURE(rc))
        LogRel(("Audio: Failed deleting file '%s', rc=%Rrc\n", pFile->szName, rc));

    return rc;
}

/**
 * Returns the raw audio data size of an audio file.
 *
 * Note: This does *not* include file headers and other data which does
 *       not belong to the actual PCM audio data.
 *
 * @returns Size (in bytes) of the raw PCM audio data.
 * @param   pFile               Audio file handle to retrieve the audio data size for.
 */
size_t DrvAudioHlpFileGetDataSize(PPDMAUDIOFILE pFile)
{
    AssertPtrReturn(pFile, 0);

    size_t cbSize = 0;

    if (pFile->enmType == PDMAUDIOFILETYPE_RAW)
    {
        cbSize = RTFileTell(pFile->hFile);
    }
    else if (pFile->enmType == PDMAUDIOFILETYPE_WAV)
    {
        PAUDIOWAVFILEDATA pData = (PAUDIOWAVFILEDATA)pFile->pvData;
        if (pData) /* The .WAV file data only is valid when a file actually has been created. */
            cbSize = pData->Hdr.u32Size2;
    }

    return cbSize;
}

/**
 * Returns whether the given audio file is open and in use or not.
 *
 * @return  bool                True if open, false if not.
 * @param   pFile               Audio file handle to check open status for.
 */
bool DrvAudioHlpFileIsOpen(PPDMAUDIOFILE pFile)
{
    if (!pFile)
        return false;

    return RTFileIsValid(pFile->hFile);
}

/**
 * Write PCM data to a wave (.WAV) file.
 *
 * @returns IPRT status code.
 * @param   pFile               Audio file handle to write PCM data to.
 * @param   pvBuf               Audio data to write.
 * @param   cbBuf               Size (in bytes) of audio data to write.
 * @param   fFlags              Additional write flags. Not being used at the moment and must be 0.
 */
int DrvAudioHlpFileWrite(PPDMAUDIOFILE pFile, const void *pvBuf, size_t cbBuf, uint32_t fFlags)
{
    AssertPtrReturn(pFile, VERR_INVALID_POINTER);
    AssertPtrReturn(pvBuf, VERR_INVALID_POINTER);

    AssertReturn(fFlags == 0, VERR_INVALID_PARAMETER); /** @todo fFlags are currently not implemented. */

    if (!cbBuf)
        return VINF_SUCCESS;

    AssertReturn(RTFileIsValid(pFile->hFile), VERR_WRONG_ORDER);

    int rc;

    if (pFile->enmType == PDMAUDIOFILETYPE_RAW)
    {
        rc = RTFileWrite(pFile->hFile, pvBuf, cbBuf, NULL);
    }
    else if (pFile->enmType == PDMAUDIOFILETYPE_WAV)
    {
        PAUDIOWAVFILEDATA pData = (PAUDIOWAVFILEDATA)pFile->pvData;
        AssertPtr(pData);

        rc = RTFileWrite(pFile->hFile, pvBuf, cbBuf, NULL);
        if (RT_SUCCESS(rc))
        {
            pData->Hdr.u32Size  += (uint32_t)cbBuf;
            pData->Hdr.u32Size2 += (uint32_t)cbBuf;
        }
    }
    else
        rc = VERR_NOT_SUPPORTED;

    return rc;
}

