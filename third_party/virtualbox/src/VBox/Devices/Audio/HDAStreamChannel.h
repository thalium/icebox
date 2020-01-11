/* $Id: HDAStreamChannel.h $ */
/** @file
 * HDAStreamChannel.h - Stream channel functions for HD Audio.
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

#ifndef VBOX_INCLUDED_SRC_Audio_HDAStreamChannel_h
#define VBOX_INCLUDED_SRC_Audio_HDAStreamChannel_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

int  hdaR3StreamChannelDataInit(PPDMAUDIOSTREAMCHANNELDATA pChanData, uint32_t fFlags);
void hdaR3StreamChannelDataDestroy(PPDMAUDIOSTREAMCHANNELDATA pChanData);
int  hdaR3StreamChannelAcquireData(PPDMAUDIOSTREAMCHANNELDATA pChanData, void *ppvData, size_t *pcbData);
int  hdaR3StreamChannelReleaseData(PPDMAUDIOSTREAMCHANNELDATA pChanData);

#endif /* !VBOX_INCLUDED_SRC_Audio_HDAStreamChannel_h */

