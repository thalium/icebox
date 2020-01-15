/* $Id: HDAStreamChannel.cpp $ */
/** @file
 * HDAStreamChannel.cpp - Stream channel functions for HD Audio.
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
int hdaR3StreamChannelDataInit(PPDMAUDIOSTREAMCHANNELDATA pChanData, uint32_t fFlags)
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
void hdaR3StreamChannelDataDestroy(PPDMAUDIOSTREAMCHANNELDATA pChanData)
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
 * Acquires (reads) audio channel data.
 * Must be released when done with hdaR3StreamChannelReleaseData().
 *
 * @returns IPRT status code.
 * @param   pChanData           Channel data to acquire audio channel data from.
 * @param   ppvData             Where to store the pointer to the acquired data.
 * @param   pcbData             Size (in bytes) of acquired data.
 */
int hdaR3StreamChannelAcquireData(PPDMAUDIOSTREAMCHANNELDATA pChanData, void **ppvData, size_t *pcbData)
{
    AssertPtrReturn(pChanData, VERR_INVALID_POINTER);
    AssertPtrReturn(ppvData,   VERR_INVALID_POINTER);
    AssertPtrReturn(pcbData,   VERR_INVALID_POINTER);

    RTCircBufAcquireReadBlock(pChanData->pCircBuf, 256 /** @todo Make this configurarble? */, ppvData, &pChanData->cbAcq);

    *pcbData = pChanData->cbAcq;
    return VINF_SUCCESS;
}

/**
 * Releases formerly acquired data by hdaR3StreamChannelAcquireData().
 *
 * @returns IPRT status code.
 * @param   pChanData           Channel data to release formerly acquired data for.
 */
int hdaR3StreamChannelReleaseData(PPDMAUDIOSTREAMCHANNELDATA pChanData)
{
    AssertPtrReturn(pChanData, VERR_INVALID_POINTER);
    RTCircBufReleaseReadBlock(pChanData->pCircBuf, pChanData->cbAcq);

    return VINF_SUCCESS;
}

