/* $Id: DevHDA.h $ */
/** @file
 * DevHDA.h - VBox Intel HD Audio Controller.
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef DEV_HDA_H
#define DEV_HDA_H

/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <VBox/vmm/pdmdev.h>

#include "AudioMixer.h"

#include "HDACodec.h"
#include "HDAStream.h"
#include "HDAStreamMap.h"
#include "HDAStreamPeriod.h"


/*********************************************************************************************************************************
*   Defines                                                                                                                      *
*********************************************************************************************************************************/


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * Structure defining an HDA mixer sink.
 * Its purpose is to know which audio mixer sink is bound to
 * which SDn (SDI/SDO) device stream.
 *
 * This is needed in order to handle interleaved streams
 * (that is, multiple channels in one stream) or non-interleaved
 * streams (each channel has a dedicated stream).
 *
 * This is only known to the actual device emulation level.
 */
typedef struct HDAMIXERSINK
{
    /** SDn ID this sink is assigned to. 0 if not assigned. */
    uint8_t                uSD;
    /** Channel ID of SDn ID. Only valid if SDn ID is valid. */
    uint8_t                uChannel;
    uint8_t                Padding[3];
    /** Pointer to the actual audio mixer sink. */
    R3PTRTYPE(PAUDMIXSINK) pMixSink;
} HDAMIXERSINK, *PHDAMIXERSINK;

/**
 * Structure for mapping a stream tag to an HDA stream.
 */
typedef struct HDATAG
{
    /** Own stream tag. */
    uint8_t               uTag;
    uint8_t               Padding[7];
    /** Pointer to associated stream. */
    R3PTRTYPE(PHDASTREAM) pStream;
} HDATAG, *PHDATAG;

#ifdef DEBUG
/** @todo Make STAM values out of this? */
typedef struct HDASTATEDBGINFO
{
    /** Timestamp (in ns) of the last timer callback (hdaTimer).
     * Used to calculate the time actually elapsed between two timer callbacks. */
    uint64_t                           tsTimerLastCalledNs;
    /** IRQ debugging information. */
    struct
    {
        /** Timestamp (in ns) of last processed (asserted / deasserted) IRQ. */
        uint64_t                       tsProcessedLastNs;
        /** Timestamp (in ns) of last asserted IRQ. */
        uint64_t                       tsAssertedNs;
        /** How many IRQs have been asserted already. */
        uint64_t                       cAsserted;
        /** Accumulated elapsed time (in ns) of all IRQ being asserted. */
        uint64_t                       tsAssertedTotalNs;
        /** Timestamp (in ns) of last deasserted IRQ. */
        uint64_t                       tsDeassertedNs;
        /** How many IRQs have been deasserted already. */
        uint64_t                       cDeasserted;
        /** Accumulated elapsed time (in ns) of all IRQ being deasserted. */
        uint64_t                       tsDeassertedTotalNs;
    } IRQ;
} HDASTATEDBGINFO, *PHDASTATEDBGINFO;
#endif

/**
 * ICH Intel HD Audio Controller state.
 */
typedef struct HDASTATE
{
    /** The PCI device structure. */
    PDMPCIDEV                          PciDev;
    /** R3 Pointer to the device instance. */
    PPDMDEVINSR3                       pDevInsR3;
    /** R0 Pointer to the device instance. */
    PPDMDEVINSR0                       pDevInsR0;
    /** R0 Pointer to the device instance. */
    PPDMDEVINSRC                       pDevInsRC;
    /** Padding for alignment. */
    uint32_t                           u32Padding;
    /** Critical section protecting the HDA state. */
    PDMCRITSECT                        CritSect;
    /** The base interface for LUN\#0. */
    PDMIBASE                           IBase;
    RTGCPHYS                           MMIOBaseAddr;
    /** The HDA's register set. */
    uint32_t                           au32Regs[HDA_NUM_REGS];
    /** Internal stream states. */
    HDASTREAM                          aStreams[HDA_MAX_STREAMS];
    /** Mapping table between stream tags and stream states. */
    HDATAG                             aTags[HDA_MAX_TAGS];
    /** CORB buffer base address. */
    uint64_t                           u64CORBBase;
    /** RIRB buffer base address. */
    uint64_t                           u64RIRBBase;
    /** DMA base address.
     *  Made out of DPLBASE + DPUBASE (3.3.32 + 3.3.33). */
    uint64_t                           u64DPBase;
    /** Pointer to CORB buffer. */
    R3PTRTYPE(uint32_t *)              pu32CorbBuf;
    /** Size in bytes of CORB buffer. */
    uint32_t                           cbCorbBuf;
    /** Padding for alignment. */
    uint32_t                           u32Padding1;
    /** Pointer to RIRB buffer. */
    R3PTRTYPE(uint64_t *)              pu64RirbBuf;
    /** Size in bytes of RIRB buffer. */
    uint32_t                           cbRirbBuf;
    /** DMA position buffer enable bit. */
    bool                               fDMAPosition;
    /** Flag whether the R0 part is enabled. */
    bool                               fR0Enabled;
    /** Flag whether the RC part is enabled. */
    bool                               fRCEnabled;
    /** Number of active (running) SDn streams. */
    uint8_t                            cStreamsActive;
#ifndef VBOX_WITH_AUDIO_HDA_CALLBACKS
    /** The timer for pumping data thru the attached LUN drivers. */
    PTMTIMERR3                         pTimer;
    /** Flag indicating whether the timer is active or not. */
    bool                               fTimerActive;
    uint8_t                            u8Padding1[7];
    /** Timer ticks per Hz. */
    uint64_t                           cTimerTicks;
    /** The current timer expire time (in timer ticks). */
    uint64_t                           tsTimerExpire;
#endif
#ifdef VBOX_WITH_STATISTICS
# ifndef VBOX_WITH_AUDIO_HDA_CALLBACKS
    STAMPROFILE                        StatTimer;
# endif
    STAMPROFILE                        StatIn;
    STAMPROFILE                        StatOut;
    STAMCOUNTER                        StatBytesRead;
    STAMCOUNTER                        StatBytesWritten;
#endif
    /** Pointer to HDA codec to use. */
    R3PTRTYPE(PHDACODEC)               pCodec;
    /** List of associated LUN drivers (HDADRIVER). */
    RTLISTANCHORR3                     lstDrv;
    /** The device' software mixer. */
    R3PTRTYPE(PAUDIOMIXER)             pMixer;
    /** HDA sink for (front) output. */
    HDAMIXERSINK                       SinkFront;
#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
    /** HDA sink for center / LFE output. */
    HDAMIXERSINK                       SinkCenterLFE;
    /** HDA sink for rear output. */
    HDAMIXERSINK                       SinkRear;
#endif
    /** HDA mixer sink for line input. */
    HDAMIXERSINK                       SinkLineIn;
#ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
    /** Audio mixer sink for microphone input. */
    HDAMIXERSINK                       SinkMicIn;
#endif
    /** Last updated wall clock (WALCLK) counter. */
    uint64_t                           u64WalClk;
#ifdef VBOX_STRICT
    /** Wall clock (WALCLK) stale count.
     *  This indicates the number of set wall clock
     *  values which did not actually move the counter forward (stale). */
    uint8_t                            u8WalClkStaleCnt;
    uint8_t                            au8Padding2[7];
#endif
    /** Response Interrupt Count (RINTCNT). */
    uint8_t                            u8RespIntCnt;
    /** Current IRQ level. */
    uint8_t                            u8IRQL;
    /** Padding for alignment. */
    uint8_t                            au8Padding3[6];
#ifdef DEBUG
    HDASTATEDBGINFO                    Dbg;
#endif
} HDASTATE, *PHDASTATE;

#ifdef VBOX_WITH_AUDIO_HDA_CALLBACKS
typedef struct HDACALLBACKCTX
{
    PHDASTATE  pThis;
    PHDADRIVER pDriver;
} HDACALLBACKCTX, *PHDACALLBACKCTX;
#endif

#endif /* !DEV_HDA_H */

