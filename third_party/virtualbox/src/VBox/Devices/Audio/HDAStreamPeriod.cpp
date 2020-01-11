/* $Id: HDAStreamPeriod.cpp $ */
/** @file
 * HDAStreamPeriod.cpp - Stream period functions for HD Audio.
 *
 * Utility functions for handling HDA audio stream periods.  Stream period
 * handling is needed in order to keep track of a stream's timing
 * and processed audio data.
 *
 * As the HDA device only has one bit clock (WALCLK) but audio streams can be
 * processed at certain points in time, these functions can be used to estimate
 * and schedule the wall clock (WALCLK) for all streams accordingly.
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

#include <iprt/asm-math.h> /* For ASMMultU64ByU32DivByU32(). */

#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmaudioifs.h>

#include "DrvAudio.h"
#include "HDAStreamPeriod.h"


#ifdef IN_RING3 /* entire file currently */

/**
 * Creates a stream period.
 *
 * @return  IPRT status code.
 * @param   pPeriod             Stream period to initialize.
 */
int hdaR3StreamPeriodCreate(PHDASTREAMPERIOD pPeriod)
{
    Assert(!(pPeriod->fStatus & HDASTREAMPERIOD_FLAG_VALID));

    int rc = RTCritSectInit(&pPeriod->CritSect);
    AssertRCReturnStmt(rc, pPeriod->fStatus = 0, rc);
    pPeriod->fStatus = HDASTREAMPERIOD_FLAG_VALID;

    return VINF_SUCCESS;
}

/**
 * Destroys a formerly created stream period.
 *
 * @param   pPeriod             Stream period to destroy.
 */
void hdaR3StreamPeriodDestroy(PHDASTREAMPERIOD pPeriod)
{
    if (pPeriod->fStatus & HDASTREAMPERIOD_FLAG_VALID)
    {
        RTCritSectDelete(&pPeriod->CritSect);

        pPeriod->fStatus = HDASTREAMPERIOD_FLAG_NONE;
    }
}

/**
 * Initializes a given stream period with needed parameters.
 *
 * @return  VBox status code.
 * @param   pPeriod             Stream period to (re-)initialize. Must be created with hdaR3StreamPeriodCreate() first.
 * @param   u8SD                Stream descriptor (serial data #) number to assign this stream period to.
 * @param   u16LVI              The HDA stream's LVI value to use for the period calculation.
 * @param   u32CBL              The HDA stream's CBL value to use for the period calculation.
 * @param   pStreamCfg          Audio stream configuration to use for this period.
 */
int hdaR3StreamPeriodInit(PHDASTREAMPERIOD pPeriod,
                          uint8_t u8SD, uint16_t u16LVI, uint32_t u32CBL, PPDMAUDIOSTREAMCFG pStreamCfg)
{
    if (   !u16LVI
        || !u32CBL
        || !DrvAudioHlpPCMPropsAreValid(&pStreamCfg->Props))
    {
        return VERR_INVALID_PARAMETER;
    }

    /*
     * Linux guests (at least Ubuntu):
     * 17632 bytes (CBL) / 4 (frame size) = 4408 frames / 4 (LVI) = 1102 frames per period
     *
     * Windows guests (Win10 AU):
     * 3584  bytes (CBL) / 4 (frame size) = 896 frames / 2 (LVI)  = 448 frames per period
     */
    unsigned cTotalPeriods = u16LVI + 1;

    if (cTotalPeriods <= 1)
        cTotalPeriods = 2; /* At least two periods *must* be present (LVI >= 1). */

    uint32_t framesToTransfer = (u32CBL / 4 /** @todo Define frame size? */) / cTotalPeriods;

    pPeriod->u8SD              = u8SD;
    pPeriod->u64StartWalClk    = 0;
    pPeriod->u32Hz             = pStreamCfg->Props.uHz;
    pPeriod->u64DurationWalClk = hdaR3StreamPeriodFramesToWalClk(pPeriod, framesToTransfer);
    pPeriod->u64ElapsedWalClk  = 0;
    pPeriod->i64DelayWalClk    = 0;
    pPeriod->framesToTransfer  = framesToTransfer;
    pPeriod->framesTransferred = 0;
    pPeriod->cIntPending       = 0;

    Log3Func(("[SD%RU8] %RU64 long, Hz=%RU32, CBL=%RU32, LVI=%RU16 -> %u periods, %RU32 frames each\n",
              pPeriod->u8SD, pPeriod->u64DurationWalClk, pPeriod->u32Hz, u32CBL, u16LVI,
              cTotalPeriods, pPeriod->framesToTransfer));

    return VINF_SUCCESS;
}

/**
 * Resets a stream period to its initial state.
 *
 * @param   pPeriod             Stream period to reset.
 */
void hdaR3StreamPeriodReset(PHDASTREAMPERIOD pPeriod)
{
    Log3Func(("[SD%RU8]\n", pPeriod->u8SD));

    if (pPeriod->cIntPending)
        LogRelMax(50, ("HDA: Warning: %RU8 interrupts for stream #%RU8 still pending -- so a period reset might trigger audio hangs\n",
                 pPeriod->cIntPending, pPeriod->u8SD));

    pPeriod->fStatus          &= ~HDASTREAMPERIOD_FLAG_ACTIVE;
    pPeriod->u64StartWalClk    = 0;
    pPeriod->u64ElapsedWalClk  = 0;
    pPeriod->framesTransferred = 0;
    pPeriod->cIntPending       = 0;
# ifdef LOG_ENABLED
    pPeriod->Dbg.tsStartNs     = 0;
# endif
}

/**
 * Begins a new period life span of a given period.
 *
 * @return  IPRT status code.
 * @param   pPeriod             Stream period to begin new life span for.
 * @param   u64WalClk           Wall clock (WALCLK) value to set for the period's starting point.
 */
int hdaR3StreamPeriodBegin(PHDASTREAMPERIOD pPeriod, uint64_t u64WalClk)
{
    Assert(!(pPeriod->fStatus & HDASTREAMPERIOD_FLAG_ACTIVE)); /* No nested calls. */

    pPeriod->fStatus          |= HDASTREAMPERIOD_FLAG_ACTIVE;
    pPeriod->u64StartWalClk    = u64WalClk;
    pPeriod->u64ElapsedWalClk  = 0;
    pPeriod->framesTransferred = 0;
    pPeriod->cIntPending       = 0;
# ifdef LOG_ENABLED
    pPeriod->Dbg.tsStartNs     = RTTimeNanoTS();
# endif

    Log3Func(("[SD%RU8] Starting @ %RU64 (%RU64 long)\n", pPeriod->u8SD, pPeriod->u64StartWalClk, pPeriod->u64DurationWalClk));
    return VINF_SUCCESS;
}

/**
 * Ends a formerly begun period life span.
 *
 * @param   pPeriod             Stream period to end life span for.
 */
void hdaR3StreamPeriodEnd(PHDASTREAMPERIOD pPeriod)
{
    Log3Func(("[SD%RU8] Took %zuus\n", pPeriod->u8SD, (RTTimeNanoTS() - pPeriod->Dbg.tsStartNs) / 1000));

    if (!(pPeriod->fStatus & HDASTREAMPERIOD_FLAG_ACTIVE))
        return;

    /* Sanity. */
    AssertMsg(pPeriod->cIntPending == 0,
              ("%RU8 interrupts for stream #%RU8 still pending -- so ending a period might trigger audio hangs\n",
               pPeriod->cIntPending, pPeriod->u8SD));
    Assert(hdaR3StreamPeriodIsComplete(pPeriod));

    pPeriod->fStatus &= ~HDASTREAMPERIOD_FLAG_ACTIVE;
}

/**
 * Pauses a period. All values remain intact.
 *
 * @param   pPeriod             Stream period to pause.
 */
void hdaR3StreamPeriodPause(PHDASTREAMPERIOD pPeriod)
{
    AssertMsg((pPeriod->fStatus & HDASTREAMPERIOD_FLAG_ACTIVE), ("Period %p already in inactive state\n", pPeriod));

    pPeriod->fStatus &= ~HDASTREAMPERIOD_FLAG_ACTIVE;

    Log3Func(("[SD%RU8]\n", pPeriod->u8SD));
}

/**
 * Resumes a formerly paused period.
 *
 * @param   pPeriod             Stream period to resume.
 */
void hdaR3StreamPeriodResume(PHDASTREAMPERIOD pPeriod)
{
    AssertMsg(!(pPeriod->fStatus & HDASTREAMPERIOD_FLAG_ACTIVE), ("Period %p already in active state\n", pPeriod));

    pPeriod->fStatus |= HDASTREAMPERIOD_FLAG_ACTIVE;

    Log3Func(("[SD%RU8]\n", pPeriod->u8SD));
}

/**
 * Locks a stream period for serializing access.
 *
 * @return  true if locking was successful, false if not.
 * @param   pPeriod             Stream period to lock.
 */
bool hdaR3StreamPeriodLock(PHDASTREAMPERIOD pPeriod)
{
    return RT_SUCCESS(RTCritSectEnter(&pPeriod->CritSect));
}

/**
 * Unlocks a formerly locked stream period.
 *
 * @param   pPeriod             Stream period to unlock.
 */
void hdaR3StreamPeriodUnlock(PHDASTREAMPERIOD pPeriod)
{
    int rc2 = RTCritSectLeave(&pPeriod->CritSect);
    AssertRC(rc2);
}

/**
 * Returns the wall clock (WALCLK) value for a given amount of stream period audio frames.
 *
 * @return  Calculated wall clock value.
 * @param   pPeriod             Stream period to calculate wall clock value for.
 * @param   uFrames             Number of audio frames to calculate wall clock value for.
 *
 * @remark  Calculation depends on the given stream period and assumes a 24 MHz wall clock counter (WALCLK).
 */
uint64_t hdaR3StreamPeriodFramesToWalClk(PHDASTREAMPERIOD pPeriod, uint32_t uFrames)
{
    /* Prevent division by zero. */
    const uint32_t uHz = pPeriod->u32Hz ? pPeriod->u32Hz : 1;

    /* 24 MHz wall clock (WALCLK): 42ns resolution. */
    return ASMMultU64ByU32DivByU32(uFrames, 24000000, uHz);
}

/**
 * Returns the absolute wall clock (WALCLK) value for the already elapsed time of
 * a given stream period.
 *
 * @return  Absolute elapsed time as wall clock (WALCLK) value.
 * @param   pPeriod             Stream period to use.
 */
uint64_t hdaR3StreamPeriodGetAbsElapsedWalClk(PHDASTREAMPERIOD pPeriod)
{
    return pPeriod->u64StartWalClk
         + pPeriod->u64ElapsedWalClk
         + pPeriod->i64DelayWalClk;
}

/**
 * Returns the absolute wall clock (WALCLK) value for the calculated end time of
 * a given stream period.
 *
 * @return  Absolute end time as wall clock (WALCLK) value.
 * @param   pPeriod             Stream period to use.
 */
uint64_t hdaR3StreamPeriodGetAbsEndWalClk(PHDASTREAMPERIOD pPeriod)
{
    return pPeriod->u64StartWalClk + pPeriod->u64DurationWalClk;
}

/**
 * Returns the remaining audio frames to process for a given stream period.
 *
 * @return  Number of remaining audio frames to process. 0 if all were processed.
 * @param   pPeriod             Stream period to return value for.
 */
uint32_t hdaR3StreamPeriodGetRemainingFrames(PHDASTREAMPERIOD pPeriod)
{
    Assert(pPeriod->framesToTransfer >= pPeriod->framesTransferred);
    return pPeriod->framesToTransfer - pPeriod->framesTransferred;
}

/**
 * Tells whether a given stream period has elapsed (time-wise) or not.
 *
 * @return  true if the stream period has elapsed, false if not.
 * @param   pPeriod             Stream period to get status for.
 */
bool hdaR3StreamPeriodHasElapsed(PHDASTREAMPERIOD pPeriod)
{
    return (pPeriod->u64ElapsedWalClk >= pPeriod->u64DurationWalClk);
}

/**
 * Tells whether a given stream period has passed the given absolute wall clock (WALCLK)
 * time or not
 *
 * @return  true if the stream period has passed the given time, false if not.
 * @param   pPeriod             Stream period to get status for.
 * @param   u64WalClk           Absolute wall clock (WALCLK) time to check for.
 */
bool hdaR3StreamPeriodHasPassedAbsWalClk(PHDASTREAMPERIOD pPeriod, uint64_t u64WalClk)
{
    /* Period not in use? */
    if (!(pPeriod->fStatus & HDASTREAMPERIOD_FLAG_ACTIVE))
        return true; /* ... implies that it has passed. */

    if (hdaR3StreamPeriodHasElapsed(pPeriod))
        return true; /* Period already has elapsed. */

    return (pPeriod->u64StartWalClk + pPeriod->u64ElapsedWalClk) >= u64WalClk;
}

/**
 * Tells whether a given stream period has some required interrupts pending or not.
 *
 * @return  true if period has interrupts pending, false if not.
 * @param   pPeriod             Stream period to get status for.
 */
bool hdaR3StreamPeriodNeedsInterrupt(PHDASTREAMPERIOD pPeriod)
{
    return pPeriod->cIntPending > 0;
}

/**
 * Acquires (references) an (pending) interrupt for a given stream period.
 *
 * @param   pPeriod             Stream period to acquire interrupt for.
 *
 * @remark  This routine does not do any actual interrupt processing; it only
 *          keeps track of the required (pending) interrupts for a stream period.
 */
void hdaR3StreamPeriodAcquireInterrupt(PHDASTREAMPERIOD pPeriod)
{
    uint32_t cIntPending = pPeriod->cIntPending;
    if (cIntPending)
    {
        Log3Func(("[SD%RU8] Already pending\n", pPeriod->u8SD));
        return;
    }

    pPeriod->cIntPending++;

    Log3Func(("[SD%RU8] %RU32\n", pPeriod->u8SD, pPeriod->cIntPending));
}

/**
 * Releases (dereferences) a pending interrupt.
 *
 * @param   pPeriod             Stream period to release pending interrupt for.
 */
void hdaR3StreamPeriodReleaseInterrupt(PHDASTREAMPERIOD pPeriod)
{
    Assert(pPeriod->cIntPending);
    pPeriod->cIntPending--;

    Log3Func(("[SD%RU8] %RU32\n", pPeriod->u8SD, pPeriod->cIntPending));
}

/**
 * Adds an amount of (processed) audio frames to a given stream period.
 *
 * @return  IPRT status code.
 * @param   pPeriod             Stream period to add audio frames to.
 * @param   framesInc           Audio frames to add.
 */
void hdaR3StreamPeriodInc(PHDASTREAMPERIOD pPeriod, uint32_t framesInc)
{
    pPeriod->framesTransferred += framesInc;
    Assert(pPeriod->framesTransferred <= pPeriod->framesToTransfer);

    pPeriod->u64ElapsedWalClk   = hdaR3StreamPeriodFramesToWalClk(pPeriod, pPeriod->framesTransferred);
    Assert(pPeriod->u64ElapsedWalClk <= pPeriod->u64DurationWalClk);

    Log3Func(("[SD%RU8] cbTransferred=%RU32, u64ElapsedWalClk=%RU64\n",
              pPeriod->u8SD, pPeriod->framesTransferred, pPeriod->u64ElapsedWalClk));
}

/**
 * Tells whether a given stream period is considered as complete or not.
 *
 * @return  true if stream period is complete, false if not.
 * @param   pPeriod             Stream period to report status for.
 *
 * @remark  A stream period is considered complete if it has 1) passed (elapsed) its calculated period time
 *          and 2) processed all required audio frames.
 */
bool hdaR3StreamPeriodIsComplete(PHDASTREAMPERIOD pPeriod)
{
    const bool fIsComplete = /* Has the period elapsed time-wise? */
                                hdaR3StreamPeriodHasElapsed(pPeriod)
                             /* All frames transferred? */
                             && pPeriod->framesTransferred >= pPeriod->framesToTransfer;
# ifdef VBOX_STRICT
    if (fIsComplete)
    {
        Assert(pPeriod->framesTransferred == pPeriod->framesToTransfer);
        Assert(pPeriod->u64ElapsedWalClk  == pPeriod->u64DurationWalClk);
    }
# endif

    Log3Func(("[SD%RU8] Period %s - runtime %RU64 / %RU64 (abs @ %RU64, starts @ %RU64, ends @ %RU64), %RU8 IRQs pending\n",
              pPeriod->u8SD,
              fIsComplete ? "COMPLETE" : "NOT COMPLETE YET",
              pPeriod->u64ElapsedWalClk, pPeriod->u64DurationWalClk,
              hdaR3StreamPeriodGetAbsElapsedWalClk(pPeriod), pPeriod->u64StartWalClk,
              hdaR3StreamPeriodGetAbsEndWalClk(pPeriod), pPeriod->cIntPending));

    return fIsComplete;
}

#endif /* IN_RING3 */

