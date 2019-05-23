/* $Id: DevHDACommon.cpp $ */
/** @file
 * DevHDACommon.cpp - Shared HDA device functions.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/assert.h>
#include <iprt/err.h>

#define LOG_GROUP LOG_GROUP_DEV_HDA
#include <VBox/log.h>


#include "DevHDA.h"
#include "DevHDACommon.h"

#include "HDAStream.h"


#ifndef DEBUG
/**
 * Processes (de/asserts) the interrupt according to the HDA's current state.
 *
 * @returns IPRT status code.
 * @param   pThis               HDA state.
 */
int hdaProcessInterrupt(PHDASTATE pThis)
#else
/**
 * Processes (de/asserts) the interrupt according to the HDA's current state.
 * Debug version.
 *
 * @returns IPRT status code.
 * @param   pThis               HDA state.
 * @param   pszSource           Caller information.
 */
int hdaProcessInterrupt(PHDASTATE pThis, const char *pszSource)
#endif
{
    HDA_REG(pThis, INTSTS) = hdaGetINTSTS(pThis);

    Log3Func(("IRQL=%RU8\n", pThis->u8IRQL));

    /* NB: It is possible to have GIS set even when CIE/SIEn are all zero; the GIS bit does
     * not control the interrupt signal. See Figure 4 on page 54 of the HDA 1.0a spec.
     */

    /* If global interrupt enable (GIE) is set, check if any enabled interrupts are set. */
    if (   (HDA_REG(pThis, INTCTL) & HDA_INTCTL_GIE)
        && (HDA_REG(pThis, INTSTS) & HDA_REG(pThis, INTCTL) & (HDA_INTCTL_CIE | HDA_STRMINT_MASK)))
    {
        if (!pThis->u8IRQL)
        {
#ifdef DEBUG
            if (!pThis->Dbg.IRQ.tsProcessedLastNs)
                pThis->Dbg.IRQ.tsProcessedLastNs = RTTimeNanoTS();

            const uint64_t tsLastElapsedNs = RTTimeNanoTS() - pThis->Dbg.IRQ.tsProcessedLastNs;

            if (!pThis->Dbg.IRQ.tsAssertedNs)
                pThis->Dbg.IRQ.tsAssertedNs = RTTimeNanoTS();

            const uint64_t tsAssertedElapsedNs = RTTimeNanoTS() - pThis->Dbg.IRQ.tsAssertedNs;

            pThis->Dbg.IRQ.cAsserted++;
            pThis->Dbg.IRQ.tsAssertedTotalNs += tsAssertedElapsedNs;

            const uint64_t avgAssertedUs = (pThis->Dbg.IRQ.tsAssertedTotalNs / pThis->Dbg.IRQ.cAsserted) / 1000;

            if (avgAssertedUs > (1000 / HDA_TIMER_HZ) /* ms */ * 1000) /* Exceeds time slot? */
                Log3Func(("Asserted (%s): %zuus elapsed (%zuus on average) -- %zuus alternation delay\n",
                          pszSource, tsAssertedElapsedNs / 1000,
                          avgAssertedUs,
                          (pThis->Dbg.IRQ.tsDeassertedNs - pThis->Dbg.IRQ.tsAssertedNs) / 1000));
#endif
            Log3Func(("Asserted (%s): %RU64us between alternation (WALCLK=%RU64)\n",
                      pszSource, tsLastElapsedNs / 1000, pThis->u64WalClk));

            PDMDevHlpPCISetIrq(pThis->CTX_SUFF(pDevIns), 0, 1 /* Assert */);
            pThis->u8IRQL = 1;

#ifdef DEBUG
            pThis->Dbg.IRQ.tsAssertedNs = RTTimeNanoTS();
            pThis->Dbg.IRQ.tsProcessedLastNs = pThis->Dbg.IRQ.tsAssertedNs;
#endif
        }
    }
    else
    {
        if (pThis->u8IRQL)
        {
#ifdef DEBUG
            if (!pThis->Dbg.IRQ.tsProcessedLastNs)
                pThis->Dbg.IRQ.tsProcessedLastNs = RTTimeNanoTS();

            const uint64_t tsLastElapsedNs = RTTimeNanoTS() - pThis->Dbg.IRQ.tsProcessedLastNs;

            if (!pThis->Dbg.IRQ.tsDeassertedNs)
                pThis->Dbg.IRQ.tsDeassertedNs = RTTimeNanoTS();

            const uint64_t tsDeassertedElapsedNs = RTTimeNanoTS() - pThis->Dbg.IRQ.tsDeassertedNs;

            pThis->Dbg.IRQ.cDeasserted++;
            pThis->Dbg.IRQ.tsDeassertedTotalNs += tsDeassertedElapsedNs;

            const uint64_t avgDeassertedUs = (pThis->Dbg.IRQ.tsDeassertedTotalNs / pThis->Dbg.IRQ.cDeasserted) / 1000;

            if (avgDeassertedUs > (1000 / HDA_TIMER_HZ) /* ms */ * 1000) /* Exceeds time slot? */
                Log3Func(("Deasserted (%s): %zuus elapsed (%zuus on average)\n",
                          pszSource, tsDeassertedElapsedNs / 1000, avgDeassertedUs));

            Log3Func(("Deasserted (%s): %RU64us between alternation (WALCLK=%RU64)\n",
                      pszSource, tsLastElapsedNs / 1000, pThis->u64WalClk));
#endif
            PDMDevHlpPCISetIrq(pThis->CTX_SUFF(pDevIns), 0, 0 /* Deassert */);
            pThis->u8IRQL = 0;

#ifdef DEBUG
            pThis->Dbg.IRQ.tsDeassertedNs    = RTTimeNanoTS();
            pThis->Dbg.IRQ.tsProcessedLastNs = pThis->Dbg.IRQ.tsDeassertedNs;
#endif
        }
    }

    return VINF_SUCCESS;
}

/**
 * Retrieves the currently set value for the wall clock.
 *
 * @return  IPRT status code.
 * @return  Currently set wall clock value.
 * @param   pThis               HDA state.
 *
 * @remark  Operation is atomic.
 */
uint64_t hdaWalClkGetCurrent(PHDASTATE pThis)
{
    return ASMAtomicReadU64(&pThis->u64WalClk);
}

#ifdef IN_RING3
/**
 * Sets the actual WALCLK register to the specified wall clock value.
 * The specified wall clock value only will be set (unless fForce is set to true) if all
 * handled HDA streams have passed (in time) that value. This guarantees that the WALCLK
 * register stays in sync with all handled HDA streams.
 *
 * @return  true if the WALCLK register has been updated, false if not.
 * @param   pThis               HDA state.
 * @param   u64WalClk           Wall clock value to set WALCLK register to.
 * @param   fForce              Whether to force setting the wall clock value or not.
 */
bool hdaWalClkSet(PHDASTATE pThis, uint64_t u64WalClk, bool fForce)
{
    const bool     fFrontPassed       = hdaStreamPeriodHasPassedAbsWalClk (&hdaGetStreamFromSink(pThis, &pThis->SinkFront)->State.Period,
                                                                           u64WalClk);
    const uint64_t u64FrontAbsWalClk  = hdaStreamPeriodGetAbsElapsedWalClk(&hdaGetStreamFromSink(pThis, &pThis->SinkFront)->State.Period);
#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
# error "Implement me!"
#endif

    const bool     fLineInPassed      = hdaStreamPeriodHasPassedAbsWalClk (&hdaGetStreamFromSink(pThis, &pThis->SinkLineIn)->State.Period, u64WalClk);
    const uint64_t u64LineInAbsWalClk = hdaStreamPeriodGetAbsElapsedWalClk(&hdaGetStreamFromSink(pThis, &pThis->SinkLineIn)->State.Period);
#ifdef VBOX_WITH_HDA_MIC_IN
    const bool     fMicInPassed       = hdaStreamPeriodHasPassedAbsWalClk (&hdaGetStreamFromSink(pThis, &pThis->SinkMicIn)->State.Period,  u64WalClk);
    const uint64_t u64MicInAbsWalClk  = hdaStreamPeriodGetAbsElapsedWalClk(&hdaGetStreamFromSink(pThis, &pThis->SinkMicIn)->State.Period);
#endif

#ifdef VBOX_STRICT
    const uint64_t u64WalClkCur = ASMAtomicReadU64(&pThis->u64WalClk);
#endif
          uint64_t u64WalClkSet = u64WalClk;

    /* Only drive the WALCLK register forward if all (active) stream periods have passed
     * the specified point in time given by u64WalClk. */
    if (  (   fFrontPassed
#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
# error "Implement me!"
#endif
           && fLineInPassed
#ifdef VBOX_WITH_HDA_MIC_IN
           && fMicInPassed
#endif
          )
       || fForce)
    {
        if (!fForce)
        {
            /* Get the maximum value of all periods we need to handle.
             * Not the most elegant solution, but works for now ... */
            u64WalClk = RT_MAX(u64WalClkSet, u64FrontAbsWalClk);
#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
# error "Implement me!"
#endif
            u64WalClk = RT_MAX(u64WalClkSet, u64LineInAbsWalClk);
#ifdef VBOX_WITH_HDA_MIC_IN
            u64WalClk = RT_MAX(u64WalClkSet, u64MicInAbsWalClk);
#endif

#ifdef VBOX_STRICT
            AssertMsg(u64WalClkSet >= u64WalClkCur,
                      ("Setting WALCLK to a value going backwards does not make any sense (old %RU64 vs. new %RU64)\n",
                       u64WalClkCur, u64WalClkSet));
            if (u64WalClkSet == u64WalClkCur)     /* Setting a stale value? */
            {
                if (pThis->u8WalClkStaleCnt++ > 3)
                    AssertMsgFailed(("Setting WALCLK to a stale value (%RU64) too often isn't a good idea really. "
                                     "Good luck with stuck audio stuff.\n", u64WalClkSet));
            }
            else
                pThis->u8WalClkStaleCnt = 0;
#endif
        }

        /* Set the new WALCLK value. */
        ASMAtomicWriteU64(&pThis->u64WalClk, u64WalClkSet);
    }

    const uint64_t u64WalClkNew = hdaWalClkGetCurrent(pThis);

    Log3Func(("Cur: %RU64, New: %RU64 (force %RTbool) -> %RU64 %s\n",
              u64WalClkCur, u64WalClk, fForce,
              u64WalClkNew, u64WalClkNew == u64WalClk ? "[OK]" : "[DELAYED]"));

    return (u64WalClkNew == u64WalClk);
}

/**
 * Returns the audio direction of a specified stream descriptor.
 *
 * The register layout specifies that input streams (SDI) come first,
 * followed by the output streams (SDO). So every stream ID below HDA_MAX_SDI
 * is an input stream, whereas everything >= HDA_MAX_SDI is an output stream.
 *
 * Note: SDnFMT register does not provide that information, so we have to judge
 *       for ourselves.
 *
 * @return  Audio direction.
 */
PDMAUDIODIR hdaGetDirFromSD(uint8_t uSD)
{
    AssertReturn(uSD < HDA_MAX_STREAMS, PDMAUDIODIR_UNKNOWN);

    if (uSD < HDA_MAX_SDI)
        return PDMAUDIODIR_IN;

    return PDMAUDIODIR_OUT;
}

/**
 * Returns the HDA stream of specified stream descriptor number.
 *
 * @return  Pointer to HDA stream, or NULL if none found.
 */
PHDASTREAM hdaGetStreamFromSD(PHDASTATE pThis, uint8_t uSD)
{
    AssertPtrReturn(pThis, NULL);
    AssertReturn(uSD < HDA_MAX_STREAMS, NULL);

    if (uSD >= HDA_MAX_STREAMS)
    {
        AssertMsgFailed(("Invalid / non-handled SD%RU8\n", uSD));
        return NULL;
    }

    return &pThis->aStreams[uSD];
}

/**
 * Returns the HDA stream of specified HDA sink.
 *
 * @return  Pointer to HDA stream, or NULL if none found.
 */
PHDASTREAM hdaGetStreamFromSink(PHDASTATE pThis, PHDAMIXERSINK pSink)
{
    AssertPtrReturn(pThis, NULL);
    AssertPtrReturn(pSink, NULL);

    /** @todo Do something with the channel mapping here? */
    return hdaGetStreamFromSD(pThis, pSink->uSD);
}

/**
 * Reads DMA data from a given HDA output stream into its associated FIFO buffer.
 *
 * @return  IPRT status code.
 * @param   pThis               HDA state.
 * @param   pStream             HDA output stream to read DMA data from.
 * @param   cbToRead            How much (in bytes) to read from DMA.
 * @param   pcbRead             Returns read bytes from DMA. Optional.
 */
int hdaDMARead(PHDASTATE pThis, PHDASTREAM pStream, uint32_t cbToRead, uint32_t *pcbRead)
{
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);
    /* pcbRead is optional. */

    int rc = VINF_SUCCESS;

    uint32_t cbReadTotal = 0;

    PHDABDLE   pBDLE     = &pStream->State.BDLE;
    PRTCIRCBUF pCircBuf  = pStream->State.pCircBuf;
    AssertPtr(pCircBuf);

#ifdef HDA_DEBUG_SILENCE
    uint64_t   csSilence = 0;

    pStream->Dbg.cSilenceThreshold = 100;
    pStream->Dbg.cbSilenceReadMin  = _1M;
#endif

    while (cbToRead)
    {
        /* Make sure we only copy as much as the stream's FIFO can hold (SDFIFOS, 18.2.39). */
        void *pvBuf;
        size_t cbBuf;
        RTCircBufAcquireWriteBlock(pCircBuf, RT_MIN(cbToRead, pStream->u16FIFOS), &pvBuf, &cbBuf);

        if (cbBuf)
        {
            /*
             * Read from the current BDLE's DMA buffer.
             */
            int rc2 = PDMDevHlpPhysRead(pThis->CTX_SUFF(pDevIns),
                                        pBDLE->Desc.u64BufAdr + pBDLE->State.u32BufOff + cbReadTotal, pvBuf, cbBuf);
            AssertRC(rc2);

#ifdef HDA_DEBUG_SILENCE
            uint16_t *pu16Buf = (uint16_t *)pvBuf;
            for (size_t i = 0; i < cbBuf / sizeof(uint16_t); i++)
            {
                if (*pu16Buf == 0)
                {
                    csSilence++;
                }
                else
                    break;
                pu16Buf++;
            }
#endif

#ifdef VBOX_AUDIO_DEBUG_DUMP_PCM_DATA
            if (cbBuf)
            {
                RTFILE fh;
                rc2 = RTFileOpen(&fh, VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "hdaDMARead.pcm",
                                 RTFILE_O_OPEN_CREATE | RTFILE_O_APPEND | RTFILE_O_WRITE | RTFILE_O_DENY_NONE);
                if (RT_SUCCESS(rc2))
                {
                    RTFileWrite(fh, pvBuf, cbBuf, NULL);
                    RTFileClose(fh);
                }
                else
                    AssertRC(rc2);
            }
#endif

#if 0
            pStream->Dbg.cbReadTotal += cbBuf;
            const uint64_t cbWritten = ASMAtomicReadU64(&pStream->Dbg.cbWrittenTotal);
            Log3Func(("cbRead=%RU64, cbWritten=%RU64 -> %RU64 bytes %s\n",
                      pStream->Dbg.cbReadTotal, cbWritten,
                      pStream->Dbg.cbReadTotal >= cbWritten ? pStream->Dbg.cbReadTotal - cbWritten : cbWritten - pStream->Dbg.cbReadTotal,
                      pStream->Dbg.cbReadTotal > cbWritten ? "too much" : "too little"));
#endif

#ifdef VBOX_WITH_STATISTICS
            STAM_COUNTER_ADD(&pThis->StatBytesRead, cbBuf);
#endif
        }

        RTCircBufReleaseWriteBlock(pCircBuf, cbBuf);

        if (!cbBuf)
        {
            rc = VERR_BUFFER_OVERFLOW;
            break;
        }

        cbReadTotal += (uint32_t)cbBuf;
        Assert(pBDLE->State.u32BufOff + cbReadTotal <= pBDLE->Desc.u32BufSize);

        Assert(cbToRead >= cbBuf);
        cbToRead    -= (uint32_t)cbBuf;
    }

#ifdef HDA_DEBUG_SILENCE

    if (csSilence)
        pStream->Dbg.csSilence += csSilence;

    if (   csSilence == 0
        && pStream->Dbg.csSilence   >  pStream->Dbg.cSilenceThreshold
        && pStream->Dbg.cbReadTotal >= pStream->Dbg.cbSilenceReadMin)
    {
        LogFunc(("Silent block detected: %RU64 audio samples\n", pStream->Dbg.csSilence));
        pStream->Dbg.csSilence = 0;
    }
#endif

    if (RT_SUCCESS(rc))
    {
        if (pcbRead)
            *pcbRead = cbReadTotal;
    }

    return rc;
}

/**
 * Writes audio data from an HDA input stream's FIFO to its associated DMA area.
 *
 * @return  IPRT status code.
 * @param   pThis               HDA state.
 * @param   pStream             HDA input stream to write audio data to.
 * @param   cbToWrite           How much (in bytes) to write.
 * @param   pcbWritten          Returns written bytes on success. Optional.
 */
int hdaDMAWrite(PHDASTATE pThis, PHDASTREAM pStream, uint32_t cbToWrite, uint32_t *pcbWritten)
{
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);
    /* pcbWritten is optional. */

    PHDABDLE   pBDLE    = &pStream->State.BDLE;
    PRTCIRCBUF pCircBuf = pStream->State.pCircBuf;
    AssertPtr(pCircBuf);

    int rc = VINF_SUCCESS;

    uint32_t cbWrittenTotal = 0;

    void *pvBuf  = NULL;
    size_t cbBuf = 0;

    uint8_t abSilence[HDA_FIFO_MAX + 1] = { 0 };

    while (cbToWrite)
    {
        size_t cbChunk = RT_MIN(cbToWrite, pStream->u16FIFOS);

        size_t cbBlock = 0;
        RTCircBufAcquireReadBlock(pCircBuf, cbChunk, &pvBuf, &cbBlock);

        if (cbBlock)
        {
            cbBuf = cbBlock;
        }
        else /* No audio data available? Send silence. */
        {
            pvBuf = &abSilence;
            cbBuf = cbChunk;
        }

        /* Sanity checks. */
        Assert(cbBuf <= pBDLE->Desc.u32BufSize - pBDLE->State.u32BufOff);
        Assert(cbBuf % HDA_FRAME_SIZE == 0);
        Assert((cbBuf >> 1) >= 1);

#ifdef VBOX_AUDIO_DEBUG_DUMP_PCM_DATA
        RTFILE fh;
        RTFileOpen(&fh, VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "hdaDMAWrite.pcm",
                   RTFILE_O_OPEN_CREATE | RTFILE_O_APPEND | RTFILE_O_WRITE | RTFILE_O_DENY_NONE);
        RTFileWrite(fh, pvBuf, cbBuf, NULL);
        RTFileClose(fh);
#endif
        int rc2 = PDMDevHlpPCIPhysWrite(pThis->CTX_SUFF(pDevIns),
                                        pBDLE->Desc.u64BufAdr + pBDLE->State.u32BufOff + cbWrittenTotal,
                                        pvBuf, cbBuf);
        AssertRC(rc2);

#ifdef VBOX_WITH_STATISTICS
        STAM_COUNTER_ADD(&pThis->StatBytesWritten, cbBuf);
#endif
        if (cbBlock)
            RTCircBufReleaseReadBlock(pCircBuf, cbBlock);

        Assert(cbToWrite >= cbBuf);
        cbToWrite -= (uint32_t)cbBuf;

        cbWrittenTotal += (uint32_t)cbBuf;
    }

    if (RT_SUCCESS(rc))
    {
        if (pcbWritten)
            *pcbWritten = cbWrittenTotal;
    }
    else
        LogFunc(("Failed with %Rrc\n", rc));

    return rc;
}
#endif /* IN_RING3 */

/**
 * Returns a new INTSTS value based on the current device state.
 *
 * @returns Determined INTSTS register value.
 * @param   pThis               HDA state.
 *
 * @remark  This function does *not* set INTSTS!
 */
uint32_t hdaGetINTSTS(PHDASTATE pThis)
{
    uint32_t intSts = 0;

    /* Check controller interrupts (RIRB, STATEST). */
    if (   (HDA_REG(pThis, RIRBSTS) & HDA_REG(pThis, RIRBCTL) & (HDA_RIRBCTL_ROIC | HDA_RIRBCTL_RINTCTL))
        /* SDIN State Change Status Flags (SCSF). */
        || (HDA_REG(pThis, STATESTS) & HDA_STATESTS_SCSF_MASK))
    {
        intSts |= HDA_INTSTS_CIS; /* Set the Controller Interrupt Status (CIS). */
    }

    if (HDA_REG(pThis, STATESTS) & HDA_REG(pThis, WAKEEN))
    {
        intSts |= HDA_INTSTS_CIS; /* Touch Controller Interrupt Status (CIS). */
    }

    /* For each stream, check if any interrupt status bit is set and enabled. */
    for (uint8_t iStrm = 0; iStrm < HDA_MAX_STREAMS; ++iStrm)
    {
        if (HDA_STREAM_REG(pThis, STS, iStrm) & HDA_STREAM_REG(pThis, CTL, iStrm) & (HDA_SDCTL_DEIE | HDA_SDCTL_FEIE  | HDA_SDCTL_IOCE))
        {
            Log3Func(("[SD%d] interrupt status set\n", iStrm));
            intSts |= RT_BIT(iStrm);
        }
    }

    if (intSts)
        intSts |= HDA_INTSTS_GIS; /* Set the Global Interrupt Status (GIS). */

    Log3Func(("-> 0x%x\n", intSts));

    return intSts;
}

/**
 * Converts an HDA stream's SDFMT register into a given PCM properties structure.
 *
 * @return  IPRT status code.
 * @param   u32SDFMT            The HDA stream's SDFMT value to convert.
 * @param   pProps              PCM properties structure to hold converted result on success.
 */
int hdaSDFMTToPCMProps(uint32_t u32SDFMT, PPDMAUDIOPCMPROPS pProps)
{
    AssertPtrReturn(pProps, VERR_INVALID_POINTER);

# define EXTRACT_VALUE(v, mask, shift) ((v & ((mask) << (shift))) >> (shift))

    int rc = VINF_SUCCESS;

    uint32_t u32Hz     = EXTRACT_VALUE(u32SDFMT, HDA_SDFMT_BASE_RATE_MASK, HDA_SDFMT_BASE_RATE_SHIFT)
                       ? 44100 : 48000;
    uint32_t u32HzMult = 1;
    uint32_t u32HzDiv  = 1;

    switch (EXTRACT_VALUE(u32SDFMT, HDA_SDFMT_MULT_MASK, HDA_SDFMT_MULT_SHIFT))
    {
        case 0: u32HzMult = 1; break;
        case 1: u32HzMult = 2; break;
        case 2: u32HzMult = 3; break;
        case 3: u32HzMult = 4; break;
        default:
            LogFunc(("Unsupported multiplier %x\n",
                     EXTRACT_VALUE(u32SDFMT, HDA_SDFMT_MULT_MASK, HDA_SDFMT_MULT_SHIFT)));
            rc = VERR_NOT_SUPPORTED;
            break;
    }
    switch (EXTRACT_VALUE(u32SDFMT, HDA_SDFMT_DIV_MASK, HDA_SDFMT_DIV_SHIFT))
    {
        case 0: u32HzDiv = 1; break;
        case 1: u32HzDiv = 2; break;
        case 2: u32HzDiv = 3; break;
        case 3: u32HzDiv = 4; break;
        case 4: u32HzDiv = 5; break;
        case 5: u32HzDiv = 6; break;
        case 6: u32HzDiv = 7; break;
        case 7: u32HzDiv = 8; break;
        default:
            LogFunc(("Unsupported divisor %x\n",
                     EXTRACT_VALUE(u32SDFMT, HDA_SDFMT_DIV_MASK, HDA_SDFMT_DIV_SHIFT)));
            rc = VERR_NOT_SUPPORTED;
            break;
    }

    uint8_t cBits = 0;
    switch (EXTRACT_VALUE(u32SDFMT, HDA_SDFMT_BITS_MASK, HDA_SDFMT_BITS_SHIFT))
    {
        case 0:
            cBits = 8;
            break;
        case 1:
            cBits = 16;
            break;
        case 4:
            cBits = 32;
            break;
        default:
            AssertMsgFailed(("Unsupported bits per sample %x\n",
                             EXTRACT_VALUE(u32SDFMT, HDA_SDFMT_BITS_MASK, HDA_SDFMT_BITS_SHIFT)));
            rc = VERR_NOT_SUPPORTED;
            break;
    }

    if (RT_SUCCESS(rc))
    {
        RT_BZERO(pProps, sizeof(PDMAUDIOPCMPROPS));

        pProps->cBits     = cBits;
        pProps->fSigned   = true;
        pProps->cChannels = (u32SDFMT & 0xf) + 1;
        pProps->uHz       = u32Hz * u32HzMult / u32HzDiv;
        pProps->cShift    = PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(pProps->cBits, pProps->cChannels);
    }

# undef EXTRACT_VALUE
    return rc;
}

#ifdef IN_RING3
/**
 * Fetches a Bundle Descriptor List Entry (BDLE) from the DMA engine.
 *
 * @param   pThis                   Pointer to HDA state.
 * @param   pBDLE                   Where to store the fetched result.
 * @param   u64BaseDMA              Address base of DMA engine to use.
 * @param   u16Entry                BDLE entry to fetch.
 */
int hdaBDLEFetch(PHDASTATE pThis, PHDABDLE pBDLE, uint64_t u64BaseDMA, uint16_t u16Entry)
{
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pBDLE,   VERR_INVALID_POINTER);
    AssertReturn(u64BaseDMA, VERR_INVALID_PARAMETER);

    if (!u64BaseDMA)
    {
        LogRel2(("HDA: Unable to fetch BDLE #%RU16 - no base DMA address set (yet)\n", u16Entry));
        return VERR_NOT_FOUND;
    }
    /** @todo Compare u16Entry with LVI. */

    int rc = PDMDevHlpPhysRead(pThis->CTX_SUFF(pDevIns), u64BaseDMA + (u16Entry * sizeof(HDABDLEDESC)),
                               &pBDLE->Desc, sizeof(pBDLE->Desc));

    if (RT_SUCCESS(rc))
    {
        /* Reset internal state. */
        RT_ZERO(pBDLE->State);
        pBDLE->State.u32BDLIndex = u16Entry;
    }

    Log3Func(("Entry #%d @ 0x%x: %R[bdle], rc=%Rrc\n", u16Entry, u64BaseDMA + (u16Entry * sizeof(HDABDLEDESC)), pBDLE, rc));


    return VINF_SUCCESS;
}

/**
 * Tells whether a given BDLE is complete or not.
 *
 * @return  true if BDLE is complete, false if not.
 * @param   pBDLE               BDLE to retrieve status for.
 */
bool hdaBDLEIsComplete(PHDABDLE pBDLE)
{
    bool fIsComplete = false;

    if (   !pBDLE->Desc.u32BufSize /* There can be BDLEs with 0 size. */
        || (pBDLE->State.u32BufOff >= pBDLE->Desc.u32BufSize))
    {
        Assert(pBDLE->State.u32BufOff == pBDLE->Desc.u32BufSize);
        fIsComplete = true;
    }

    Log3Func(("%R[bdle] => %s\n", pBDLE, fIsComplete ? "COMPLETE" : "INCOMPLETE"));

    return fIsComplete;
}

/**
 * Tells whether a given BDLE needs an interrupt or not.
 *
 * @return  true if BDLE needs an interrupt, false if not.
 * @param   pBDLE               BDLE to retrieve status for.
 */
bool hdaBDLENeedsInterrupt(PHDABDLE pBDLE)
{
    return (pBDLE->Desc.fFlags & HDA_BDLE_FLAG_IOC);
}
#endif /* IN_RING3 */

