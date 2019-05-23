/* $Id: HDAStreamMap.h $ */
/** @file
 * HDAStreamMap.h - Stream mapping functions for HD Audio.
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

#ifndef HDA_STREAMMAP_H
#define HDA_STREAMMAP_H

/**
 * Structure for keeping an audio stream data mapping.
 */
typedef struct HDASTREAMMAPPING
{
    /** The stream's layout. */
    PDMAUDIOSTREAMLAYOUT              enmLayout;
    /** Number of audio channels in this stream. */
    uint8_t                           cChannels;
    /** Array of audio channels. */
    R3PTRTYPE(PPDMAUDIOSTREAMCHANNEL) paChannels;
    /** Circular buffer holding for holding audio data for this mapping. */
    R3PTRTYPE(PRTCIRCBUF)             pCircBuf;
} HDASTREAMMAPPING, *PHDASTREAMMAPPING;

/** @name Stream mapping functions.
 * @{
 */
#ifdef IN_RING3
int  hdaStreamMapInit(PHDASTREAMMAPPING pMapping, PPDMAUDIOPCMPROPS pProps);
void hdaStreamMapDestroy(PHDASTREAMMAPPING pMapping);
void hdaStreamMapReset(PHDASTREAMMAPPING pMapping);
#endif /* IN_RING3 */
/** @} */

#endif /* !HDA_STREAMMAP_H */

