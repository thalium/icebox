/* $Id: HDAStreamPeriod.h $ */
/** @file
 * HDAStreamPeriod.h - Stream period functions for HD Audio.
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

#ifndef HDA_STREAMPERIOD_H
#define HDA_STREAMPERIOD_H

#include <iprt/critsect.h>
#ifdef DEBUG
# include <iprt/time.h>
#endif

struct HDASTREAM;
typedef HDASTREAM *PHDASTREAM;

#ifdef DEBUG
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
    /** Associated HDA stream descriptor (SD) number. */
    uint8_t                 u8SD;
    /** The period's status flags. */
    uint8_t                 fStatus;
    uint8_t                 Padding1[6];
    /** Critical section for serializing access. */
    RTCRITSECT              CritSect;
    uint32_t                Padding2[1];
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
    /** Number of pending interrupts required for this period. */
    uint8_t                 cIntPending;
    uint8_t                 Padding3[7];
#ifdef DEBUG
    /** Debugging information. */
    HDASTREAMPERIODDBGINFO  Dbg;
#endif
} HDASTREAMPERIOD, *PHDASTREAMPERIOD;

#ifdef IN_RING3
int      hdaStreamPeriodCreate(PHDASTREAMPERIOD pPeriod);
void     hdaStreamPeriodDestroy(PHDASTREAMPERIOD pPeriod);
void     hdaStreamPeriodInit(PHDASTREAMPERIOD pPeriod, uint8_t u8SD, uint16_t u16LVI, uint32_t u32CBL, PPDMAUDIOSTREAMCFG pStreamCfg);
void     hdaStreamPeriodReset(PHDASTREAMPERIOD pPeriod);
int      hdaStreamPeriodBegin(PHDASTREAMPERIOD pPeriod, uint64_t u64WalClk);
void     hdaStreamPeriodEnd(PHDASTREAMPERIOD pPeriod);
void     hdaStreamPeriodPause(PHDASTREAMPERIOD pPeriod);
void     hdaStreamPeriodResume(PHDASTREAMPERIOD pPeriod);
bool     hdaStreamPeriodLock(PHDASTREAMPERIOD pPeriod);
void     hdaStreamPeriodUnlock(PHDASTREAMPERIOD pPeriod);
uint64_t hdaStreamPeriodFramesToWalClk(PHDASTREAMPERIOD pPeriod, uint32_t uFrames);
uint64_t hdaStreamPeriodGetAbsEndWalClk(PHDASTREAMPERIOD pPeriod);
uint64_t hdaStreamPeriodGetAbsElapsedWalClk(PHDASTREAMPERIOD pPeriod);
uint32_t hdaStreamPeriodGetRemainingFrames(PHDASTREAMPERIOD pPeriod);
bool     hdaStreamPeriodHasElapsed(PHDASTREAMPERIOD pPeriod);
bool     hdaStreamPeriodHasPassedAbsWalClk(PHDASTREAMPERIOD pPeriod, uint64_t u64WalClk);
bool     hdaStreamPeriodNeedsInterrupt(PHDASTREAMPERIOD pPeriod);
void     hdaStreamPeriodAcquireInterrupt(PHDASTREAMPERIOD pPeriod);
void     hdaStreamPeriodReleaseInterrupt(PHDASTREAMPERIOD pPeriod);
void     hdaStreamPeriodInc(PHDASTREAMPERIOD pPeriod, uint32_t framesInc);
bool     hdaStreamPeriodIsComplete(PHDASTREAMPERIOD pPeriod);
#endif /* IN_RING3 */

#endif /* !HDA_STREAMPERIOD_H */

