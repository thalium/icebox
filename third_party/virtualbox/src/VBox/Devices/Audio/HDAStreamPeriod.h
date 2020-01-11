/* $Id: HDAStreamPeriod.h $ */
/** @file
 * HDAStreamPeriod.h - Stream period functions for HD Audio.
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

#ifndef VBOX_INCLUDED_SRC_Audio_HDAStreamPeriod_h
#define VBOX_INCLUDED_SRC_Audio_HDAStreamPeriod_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/critsect.h>
#ifdef DEBUG
# include <iprt/time.h>
#endif
#include <VBox/log.h> /* LOG_ENABLED */

struct HDASTREAM;
typedef HDASTREAM *PHDASTREAM;

#ifdef LOG_ENABLED
/**
 * Structure for debug information of an HDA stream's period.
 */
typedef struct HDASTREAMPERIODDBGINFO
{
    /** Host start time (in ns) of the period. */
    uint64_t                tsStartNs;
} HDASTREAMPERIODDBGINFO, *PHDASTREAMPERIODDBGINFO;
#endif

/** No flags set. */
#define HDASTREAMPERIOD_FLAG_NONE    0
/** The stream period has been initialized and is in a valid state. */
#define HDASTREAMPERIOD_FLAG_VALID   RT_BIT(0)
/** The stream period is active. */
#define HDASTREAMPERIOD_FLAG_ACTIVE  RT_BIT(1)

/**
 * Structure for keeping an HDA stream's (time) period.
 * This is needed in order to keep track of stream timing and interrupt delivery.
 */
typedef struct HDASTREAMPERIOD
{
    /** Critical section for serializing access. */
    RTCRITSECT              CritSect;
    /** Associated HDA stream descriptor (SD) number. */
    uint8_t                 u8SD;
    /** The period's status flags. */
    uint8_t                 fStatus;
    /** Number of pending interrupts required for this period. */
    uint8_t                 cIntPending;
    uint8_t                 bPadding0;
    /** Hertz (Hz) rate this period runs with. */
    uint32_t                u32Hz;
    /** Period start time (in wall clock counts). */
    uint64_t                u64StartWalClk;
    /** Period duration (in wall clock counts). */
    uint64_t                u64DurationWalClk;
    /** The period's (relative) elapsed time (in wall clock counts). */
    uint64_t                u64ElapsedWalClk;
    /** Delay (in wall clock counts) for tweaking the period timing. Optional. */
    int64_t                 i64DelayWalClk;
    /** Number of audio frames to transfer for this period. */
    uint32_t                framesToTransfer;
    /** Number of audio frames already transfered. */
    uint32_t                framesTransferred;
#ifdef LOG_ENABLED
    /** Debugging information. */
    HDASTREAMPERIODDBGINFO  Dbg;
#endif
} HDASTREAMPERIOD;
AssertCompileSizeAlignment(HDASTREAMPERIOD, 8);
/** Pointer to a HDA stream's time period keeper. */
typedef HDASTREAMPERIOD *PHDASTREAMPERIOD;

#ifdef IN_RING3
int      hdaR3StreamPeriodCreate(PHDASTREAMPERIOD pPeriod);
void     hdaR3StreamPeriodDestroy(PHDASTREAMPERIOD pPeriod);
int      hdaR3StreamPeriodInit(PHDASTREAMPERIOD pPeriod, uint8_t u8SD, uint16_t u16LVI, uint32_t u32CBL, PPDMAUDIOSTREAMCFG pStreamCfg);
void     hdaR3StreamPeriodReset(PHDASTREAMPERIOD pPeriod);
int      hdaR3StreamPeriodBegin(PHDASTREAMPERIOD pPeriod, uint64_t u64WalClk);
void     hdaR3StreamPeriodEnd(PHDASTREAMPERIOD pPeriod);
void     hdaR3StreamPeriodPause(PHDASTREAMPERIOD pPeriod);
void     hdaR3StreamPeriodResume(PHDASTREAMPERIOD pPeriod);
bool     hdaR3StreamPeriodLock(PHDASTREAMPERIOD pPeriod);
void     hdaR3StreamPeriodUnlock(PHDASTREAMPERIOD pPeriod);
uint64_t hdaR3StreamPeriodFramesToWalClk(PHDASTREAMPERIOD pPeriod, uint32_t uFrames);
uint64_t hdaR3StreamPeriodGetAbsEndWalClk(PHDASTREAMPERIOD pPeriod);
uint64_t hdaR3StreamPeriodGetAbsElapsedWalClk(PHDASTREAMPERIOD pPeriod);
uint32_t hdaR3StreamPeriodGetRemainingFrames(PHDASTREAMPERIOD pPeriod);
bool     hdaR3StreamPeriodHasElapsed(PHDASTREAMPERIOD pPeriod);
bool     hdaR3StreamPeriodHasPassedAbsWalClk(PHDASTREAMPERIOD pPeriod, uint64_t u64WalClk);
bool     hdaR3StreamPeriodNeedsInterrupt(PHDASTREAMPERIOD pPeriod);
void     hdaR3StreamPeriodAcquireInterrupt(PHDASTREAMPERIOD pPeriod);
void     hdaR3StreamPeriodReleaseInterrupt(PHDASTREAMPERIOD pPeriod);
void     hdaR3StreamPeriodInc(PHDASTREAMPERIOD pPeriod, uint32_t framesInc);
bool     hdaR3StreamPeriodIsComplete(PHDASTREAMPERIOD pPeriod);
#endif /* IN_RING3 */

#endif /* !VBOX_INCLUDED_SRC_Audio_HDAStreamPeriod_h */

