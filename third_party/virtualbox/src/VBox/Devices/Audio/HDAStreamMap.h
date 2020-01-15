/* $Id: HDAStreamMap.h $ */
/** @file
 * HDAStreamMap.h - Stream map functions for HD Audio.
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

#ifndef VBOX_INCLUDED_SRC_Audio_HDAStreamMap_h
#define VBOX_INCLUDED_SRC_Audio_HDAStreamMap_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/**
 * Structure for keeping an audio stream data mapping.
 */
typedef struct HDASTREAMMAP
{
    /** The stream's layout. */
    PDMAUDIOSTREAMLAYOUT               enmLayout;
    uint8_t                            cbFrameSize;
    /** Number of mappings in paMappings. */
    uint8_t                            cMappings;
    uint8_t                            aPadding[2];
    /** Array of stream mappings.
     *  Note: The mappings *must* be layed out in an increasing order, e.g.
     *        how the data appears in the given data block. */
    R3PTRTYPE(PPDMAUDIOSTREAMMAP)      paMappings;
#if HC_ARCH_BITS == 32
    RTR3PTR                            Padding1;
#endif
#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
    /** Circular buffer holding for holding audio data for this mapping. */
    R3PTRTYPE(PRTCIRCBUF)              pCircBuf;
#endif
} HDASTREAMMAP;
AssertCompileSizeAlignment(HDASTREAMMAP, 8);
typedef HDASTREAMMAP *PHDASTREAMMAP;

/** @name Stream mapping functions.
 * @{
 */
#ifdef IN_RING3
int  hdaR3StreamMapInit(PHDASTREAMMAP pMapping, PPDMAUDIOPCMPROPS pProps);
void hdaR3StreamMapDestroy(PHDASTREAMMAP pMapping);
void hdaR3StreamMapReset(PHDASTREAMMAP pMapping);
#endif /* IN_RING3 */
/** @} */

#endif /* !VBOX_INCLUDED_SRC_Audio_HDAStreamMap_h */

