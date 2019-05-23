/* $Id: HDAStreamChannel.h $ */
/** @file
 * HDAStreamChannel.h - Stream channel functions for HD Audio.
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

#ifndef HDA_STREAMCHANNEL_H
#define HDA_STREAMCHANNEL_H

int hdaStreamChannelDataInit(PPDMAUDIOSTREAMCHANNELDATA pChanData, uint32_t fFlags);
void hdaStreamChannelDataDestroy(PPDMAUDIOSTREAMCHANNELDATA pChanData);
int hdaStreamChannelExtract(PPDMAUDIOSTREAMCHANNEL pChan, const void *pvBuf, size_t cbBuf);
int hdaStreamChannelAdvance(PPDMAUDIOSTREAMCHANNEL pChan, size_t cbAdv);
int hdaStreamChannelAcquireData(PPDMAUDIOSTREAMCHANNELDATA pChanData, void *ppvData, size_t *pcbData);
int hdaStreamChannelReleaseData(PPDMAUDIOSTREAMCHANNELDATA pChanData);

#endif /* !HDA_STREAMCHANNEL_H */

