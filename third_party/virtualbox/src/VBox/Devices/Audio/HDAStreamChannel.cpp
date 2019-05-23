/* $Id: HDAStreamChannel.cpp $ */
/** @file
 * HDAStreamChannel.cpp - Stream channel functions for HD Audio.
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

#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmaudioifs.h>

#include "HDAStreamChannel.h"

/**
 * Initializes a stream channel data structure.
 *
 * @returns IPRT status code.
 * @param   pChanData           Channel data to initialize.
 * @param   fFlags
 */
int hdaStreamChannelDataInit(PPDMAUDIOSTREAMCHANNELDATA pChanData, uint32_t fFlags)
{
    int rc = RTCircBufCreate(&pChanData->pCircBuf, 256); /** @todo Make this configurable? */
    if (RT_SUCCESS(rc))
    {
        pChanData->fFlags = fFlags;
    }

    return rc;
}

/**
 * Destroys a stream channel data structure.
 *
 * @param   pChanData           Channel data to destroy.
 */
void hdaStreamChannelDataDestroy(PPDMAUDIOSTREAMCHANNELDATA pChanData)
{
    if (!pChanData)
        return;

    if (pChanData->pCircBuf)
    {
        RTCircBufDestroy(pChanData->pCircBuf);
        pChanData->pCircBuf = NULL;
    }

    pChanData->fFlags = PDMAUDIOSTREAMCHANNELDATA_FLAG_NONE;
}

/**
 * Extracts HDA audio stream data and stores it into the given stream channel data block.
 *
 * @returns IPRT status code.
 * @param   pChan               Channel data to extract audio stream data into.
 * @param   pvBuf               Buffer of audio data to extract.
 * @param   cbBuf               Size (in bytes) of audio data to extract.
 */
int hdaStreamChannelExtract(PPDMAUDIOSTREAMCHANNEL pChan, const void *pvBuf, size_t cbBuf)
{
    AssertPtrReturn(pChan, VERR_INVALID_POINTER);
    AssertPtrReturn(pvBuf, VERR_INVALID_POINTER);
    AssertReturn(cbBuf,    VERR_INVALID_PARAMETER);

    AssertRelease(pChan->cbOff <= cbBuf);

    const uint8_t *pu8Buf = (const uint8_t *)pvBuf;

    size_t         cbSrc = cbBuf - pChan->cbOff;
    const uint8_t *pvSrc = &pu8Buf[pChan->cbOff];

    size_t         cbDst;
    uint8_t       *pvDst;
    RTCircBufAcquireWriteBlock(pChan->Data.pCircBuf, cbBuf, (void **)&pvDst, &cbDst);

    cbSrc = RT_MIN(cbSrc, cbDst);

    while (cbSrc)
    {
        AssertBreak(cbDst >= cbSrc);

        /* Enough data for at least one next frame? */
        if (cbSrc < pChan->cbFrame)
            break;

        memcpy(pvDst, pvSrc, pChan->cbFrame);

        /* Advance to next channel frame in stream. */
        pvSrc        += pChan->cbStep;
        Assert(cbSrc >= pChan->cbStep);
        cbSrc        -= pChan->cbStep;

        /* Advance destination by one frame. */
        pvDst        += pChan->cbFrame;
        Assert(cbDst >= pChan->cbFrame);
        cbDst        -= pChan->cbFrame;

        /* Adjust offset. */
        pChan->cbOff += pChan->cbFrame;
    }

    RTCircBufReleaseWriteBlock(pChan->Data.pCircBuf, cbDst);

    return VINF_SUCCESS;
}

/**
 * Advances the current read / write pointer by a certain amount.
 *
 * @returns IPRT status code.
 * @param   pChan               Channel data to advance read / write pointer for.
 * @param   cbAdv               Amount (in bytes) to advance read / write pointer.
 *
 * @remark  Currently not used / implemented.
 */
int hdaStreamChannelAdvance(PPDMAUDIOSTREAMCHANNEL pChan, size_t cbAdv)
{
    AssertPtrReturn(pChan, VERR_INVALID_POINTER);

    if (!cbAdv)
        return VINF_SUCCESS;

    return VINF_SUCCESS;
}

/**
 * Acquires (reads) audio channel data.
 * Must be released when done with hdaStreamChannelReleaseData().
 *
 * @returns IPRT status code.
 * @param   pChanData           Channel data to acquire audio channel data from.
 * @param   ppvData             Where to store the pointer to the acquired data.
 * @param   pcbData             Size (in bytes) of acquired data.
 */
int hdaStreamChannelAcquireData(PPDMAUDIOSTREAMCHANNELDATA pChanData, void **ppvData, size_t *pcbData)
{
    AssertPtrReturn(pChanData, VERR_INVALID_POINTER);
    AssertPtrReturn(ppvData,   VERR_INVALID_POINTER);
    AssertPtrReturn(pcbData,   VERR_INVALID_POINTER);

    RTCircBufAcquireReadBlock(pChanData->pCircBuf, 256 /** @todo Make this configurarble? */, ppvData, &pChanData->cbAcq);

    *pcbData = pChanData->cbAcq;
    return VINF_SUCCESS;
}

/**
 * Releases formerly acquired data by hdaStreamChannelAcquireData().
 *
 * @returns IPRT status code.
 * @param   pChanData           Channel data to release formerly acquired data for.
 */
int hdaStreamChannelReleaseData(PPDMAUDIOSTREAMCHANNELDATA pChanData)
{
    AssertPtrReturn(pChanData, VERR_INVALID_POINTER);
    RTCircBufReleaseReadBlock(pChanData->pCircBuf, pChanData->cbAcq);

    return VINF_SUCCESS;
}

