/* $Id: HDAStreamMap.cpp $ */
/** @file
 * HDAStreamMap.cpp - Stream mapping functions for HD Audio.
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

/**
 * Initializes a stream mapping structure according to the given PCM properties.
 *
 * @return  IPRT status code.
 * @param   pMapping            Pointer to mapping to initialize.
 * @param   pProps              Pointer to PCM properties to use for initialization.
 */
int hdaR3StreamMapInit(PHDASTREAMMAPPING pMapping, PPDMAUDIOPCMPROPS pProps)
{
    AssertPtrReturn(pMapping, VERR_INVALID_POINTER);
    AssertPtrReturn(pProps,   VERR_INVALID_POINTER);

    if (!DrvAudioHlpPCMPropsAreValid(pProps))
        return VERR_INVALID_PARAMETER;

    hdaR3StreamMapReset(pMapping);

    pMapping->paChannels = (PPDMAUDIOSTREAMCHANNEL)RTMemAlloc(sizeof(PDMAUDIOSTREAMCHANNEL) * pProps->cChannels);
    if (!pMapping->paChannels)
        return VERR_NO_MEMORY;

    int rc = VINF_SUCCESS;

    Assert(RT_IS_POWER_OF_TWO(pProps->cBytes * 8));

    /** @todo We assume all channels in a stream have the same format. */
    PPDMAUDIOSTREAMCHANNEL pChan = pMapping->paChannels;
    for (uint8_t i = 0; i < pProps->cChannels; i++)
    {
        pChan->uChannel = i;
        pChan->cbStep   = pProps->cBytes;
        pChan->cbFrame  = pChan->cbStep * pProps->cChannels;
        pChan->cbFirst  = i * pChan->cbStep;
        pChan->cbOff    = pChan->cbFirst;

        int rc2 = hdaR3StreamChannelDataInit(&pChan->Data, PDMAUDIOSTREAMCHANNELDATA_FLAG_NONE);
        if (RT_SUCCESS(rc))
            rc = rc2;

        if (RT_FAILURE(rc))
            break;

        pChan++;
    }

    if (   RT_SUCCESS(rc)
        /* Create circular buffer if not created yet. */
        && !pMapping->pCircBuf)
    {
        rc = RTCircBufCreate(&pMapping->pCircBuf, _4K); /** @todo Make size configurable? */
    }

    if (RT_SUCCESS(rc))
    {
        pMapping->cChannels = pProps->cChannels;
#ifdef VBOX_WITH_HDA_AUDIO_INTERLEAVING_STREAMS_SUPPORT
        pMapping->enmLayout = PDMAUDIOSTREAMLAYOUT_INTERLEAVED;
#else
        pMapping->enmLayout = PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED;
#endif
    }

    return rc;
}


/**
 * Destroys a given stream mapping.
 *
 * @param   pMapping            Pointer to mapping to destroy.
 */
void hdaR3StreamMapDestroy(PHDASTREAMMAPPING pMapping)
{
    hdaR3StreamMapReset(pMapping);

    if (pMapping->pCircBuf)
    {
        RTCircBufDestroy(pMapping->pCircBuf);
        pMapping->pCircBuf = NULL;
    }
}


/**
 * Resets a given stream mapping.
 *
 * @param   pMapping            Pointer to mapping to reset.
 */
void hdaR3StreamMapReset(PHDASTREAMMAPPING pMapping)
{
    AssertPtrReturnVoid(pMapping);

    pMapping->enmLayout = PDMAUDIOSTREAMLAYOUT_UNKNOWN;

    if (pMapping->cChannels)
    {
        for (uint8_t i = 0; i < pMapping->cChannels; i++)
            hdaR3StreamChannelDataDestroy(&pMapping->paChannels[i].Data);

        AssertPtr(pMapping->paChannels);
        RTMemFree(pMapping->paChannels);
        pMapping->paChannels = NULL;

        pMapping->cChannels = 0;
    }
}

#endif /* IN_RING3 */

