/* $Id: HDAStreamMap.cpp $ */
/** @file
 * HDAStreamMap.cpp - Stream mapping functions for HD Audio.
 */

/*
 * Copyright (C) 2017-2019 Oracle Corporation
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

#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmaudioifs.h>

#include "DrvAudio.h"

#include "HDAStreamChannel.h"
#include "HDAStreamMap.h"

#ifdef IN_RING3

static int hdaR3StreamMapSetup(PHDASTREAMMAP pMap, PPDMAUDIOPCMPROPS pProps);

/**
 * Initializes a stream mapping structure according to the given PCM properties.
 *
 * @return  IPRT status code.
 * @param   pMap                Pointer to mapping to initialize.
 * @param   pProps              Pointer to PCM properties to use for initialization.
 */
int hdaR3StreamMapInit(PHDASTREAMMAP pMap, PPDMAUDIOPCMPROPS pProps)
{
    AssertPtrReturn(pMap, VERR_INVALID_POINTER);
    AssertPtrReturn(pProps,   VERR_INVALID_POINTER);

    if (!DrvAudioHlpPCMPropsAreValid(pProps))
        return VERR_INVALID_PARAMETER;

    hdaR3StreamMapReset(pMap);

    int rc = hdaR3StreamMapSetup(pMap, pProps);
    if (RT_FAILURE(rc))
        return rc;

#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
    if (   RT_SUCCESS(rc)
        /* Create circular buffer if not created yet. */
        && !pMap->pCircBuf)
    {
        rc = RTCircBufCreate(&pMap->pCircBuf, _4K); /** @todo Make size configurable? */
    }
#endif

    if (RT_SUCCESS(rc))
    {
        pMap->cbFrameSize = pProps->cChannels * pProps->cBytes;

        LogFunc(("cChannels=%RU8, cBytes=%RU8 -> cbFrameSize=%RU32\n",
                 pProps->cChannels, pProps->cBytes, pMap->cbFrameSize));

        Assert(pMap->cbFrameSize); /* Frame size must not be 0. */

        pMap->enmLayout = PDMAUDIOSTREAMLAYOUT_INTERLEAVED;
    }

    return rc;
}


/**
 * Destroys a given stream mapping.
 *
 * @param   pMap            Pointer to mapping to destroy.
 */
void hdaR3StreamMapDestroy(PHDASTREAMMAP pMap)
{
    hdaR3StreamMapReset(pMap);

#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
    if (pMap->pCircBuf)
    {
        RTCircBufDestroy(pMap->pCircBuf);
        pMap->pCircBuf = NULL;
    }
#endif
}


/**
 * Resets a given stream mapping.
 *
 * @param   pMap            Pointer to mapping to reset.
 */
void hdaR3StreamMapReset(PHDASTREAMMAP pMap)
{
    AssertPtrReturnVoid(pMap);

    pMap->enmLayout = PDMAUDIOSTREAMLAYOUT_UNKNOWN;

    if (pMap->paMappings)
    {
        for (uint8_t i = 0; i < pMap->cMappings; i++)
            hdaR3StreamChannelDataDestroy(&pMap->paMappings[i].Data);

        RTMemFree(pMap->paMappings);
        pMap->paMappings = NULL;

        pMap->cMappings = 0;
    }
}


/**
 * Sets up a stream mapping according to the given properties / configuration.
 *
 * @return VBox status code, or VERR_NOT_SUPPORTED if the channel setup is not supported (yet).
 * @param  pMap                 Pointer to mapping to set up.
 * @param  pProps               PCM audio properties to use for lookup.
 */
static int hdaR3StreamMapSetup(PHDASTREAMMAP pMap, PPDMAUDIOPCMPROPS pProps)
{
    int rc;

    /** @todo We ASSUME that all channels in a stream ...
     *        - have the same format
     *        - are in a consecutive order with no gaps in between
     *        - have a simple (raw) data layout
     *        - work in a non-striped fashion, e.g. interleaved (only on one SDn, not spread over multiple SDns) */
    if  (   pProps->cChannels == 1  /* Mono */
         || pProps->cChannels == 2  /* Stereo */
         || pProps->cChannels == 4  /* Quadrophonic */
         || pProps->cChannels == 6) /* Surround (5.1) */
    {
        /* For now we don't have anything other as mono / stereo channels being covered by the backends.
         * So just set up one channel covering those and skipping the rest (like configured rear or center/LFE outputs). */
        pMap->cMappings  = 1;
        pMap->paMappings = (PPDMAUDIOSTREAMMAP)RTMemAlloc(sizeof(PDMAUDIOSTREAMMAP) * pMap->cMappings);
        if (!pMap->paMappings)
            return VERR_NO_MEMORY;

        PPDMAUDIOSTREAMMAP pMapLR = &pMap->paMappings[0];

        pMapLR->aID[0]  = PDMAUDIOSTREAMCHANNELID_FRONT_LEFT;
        pMapLR->aID[1]  = PDMAUDIOSTREAMCHANNELID_FRONT_RIGHT;
        pMapLR->cbFrame = pProps->cBytes * pProps->cChannels;
        pMapLR->cbSize  = pProps->cBytes * 2 /* Front left + Front right channels */;
        pMapLR->cbFirst = 0;
        pMapLR->cbOff   = pMapLR->cbFirst;

        rc = hdaR3StreamChannelDataInit(&pMapLR->Data, PDMAUDIOSTREAMCHANNELDATA_FLAG_NONE);
        AssertRC(rc);
    }
    else
        rc = VERR_NOT_SUPPORTED; /** @todo r=andy Support more setups. */

    return rc;
}
#endif /* IN_RING3 */

