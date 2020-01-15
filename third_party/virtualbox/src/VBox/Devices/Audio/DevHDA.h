/* $Id: DevHDA.h $ */
/** @file
 * DevHDA.h - VBox Intel HD Audio Controller.
 */

/*
 * Copyright (C) 2016-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBOX_INCLUDED_SRC_Audio_DevHDA_h
#define VBOX_INCLUDED_SRC_Audio_DevHDA_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/path.h>

#include <VBox/vmm/pdmdev.h>

#include "AudioMixer.h"

#include "HDACodec.h"
#include "HDAStream.h"
#include "HDAStreamMap.h"
#include "HDAStreamPeriod.h"



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
    R3PTRTYPE(PHDASTREAM)  pStream;
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

/** @todo Make STAM values out of this? */
typedef struct HDASTATEDBGINFO
{
#ifdef DEBUG
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
#endif
    /** Whether debugging is enabled or not. */
    bool                               fEnabled;
    /** Path where to dump the debug output to.
     *  Defaults to VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH. */
    char                               szOutPath[RTPATH_MAX + 1];
} HDASTATEDBGINFO, *PHDASTATEDBGINFO;

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
    /** Flag whether the R0 and RC parts are enabled. */
    bool                               fRZEnabled;
    /** Reserved. */
    bool                               fPadding1b;
    /** Number of active (running) SDn streams. */
    uint8_t                            cStreamsActive;
    /** The stream timers for pumping data thru the attached LUN drivers. */
    PTMTIMERR3                         pTimer[HDA_MAX_STREAMS];
#ifdef VBOX_WITH_STATISTICS
    STAMPROFILE                        StatTimer;
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
    /** Response Interrupt Count (RINTCNT). */
    uint16_t                           u16RespIntCnt;
    /** Position adjustment (in audio frames).
     *
     *  This is not an official feature of the HDA specs, but used by
     *  certain OS drivers (e.g. snd_hda_intel) to work around certain
     *  quirks by "real" HDA hardware implementations.
     *
     *  The position adjustment specifies how many audio frames
     *  a stream is ahead from its actual reading/writing position when
     *  starting a stream.
     */
    uint16_t                           cPosAdjustFrames;
    /** Whether the position adjustment is enabled or not. */
    bool                               fPosAdjustEnabled;
#ifdef VBOX_STRICT
    /** Wall clock (WALCLK) stale count.
     *  This indicates the number of set wall clock
     *  values which did not actually move the counter forward (stale). */
    uint8_t                            u8WalClkStaleCnt;
    uint8_t                            Padding1[2];
#else
    uint8_t                            Padding1[3];
#endif
    /** Current IRQ level. */
    uint8_t                            u8IRQL;
    /** The device timer Hz rate. Defaults to HDA_TIMER_HZ_DEFAULT. */
    uint16_t                           uTimerHz;
    /** Padding for alignment. */
    uint8_t                            au8Padding3[3];
    HDASTATEDBGINFO                    Dbg;
    /** This is for checking that the build was correctly configured in all contexts.
     * This is set to HDASTATE_ALIGNMENT_CHECK_MAGIC.  */
    uint64_t                            uAlignmentCheckMagic;
} HDASTATE, *PHDASTATE;

/** Value for HDASTATE:uAlignmentCheckMagic. */
#define HDASTATE_ALIGNMENT_CHECK_MAGIC  UINT64_C(0x1298afb75893e059)

#endif /* !VBOX_INCLUDED_SRC_Audio_DevHDA_h */

