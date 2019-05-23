/* $Id: tstAudioMixBuffer.cpp $ */
/** @file
 * Audio testcase - Mixing buffer.
 */

/*
 * Copyright (C) 2014-2018 Oracle Corporation
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
#include <iprt/err.h>
#include <iprt/initterm.h>
#include <iprt/mem.h>
#include <iprt/rand.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/test.h>


#include "../AudioMixBuffer.h"
#include "../DrvAudio.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

static int tstSingle(RTTEST hTest)
{
    RTTestSubF(hTest, "Single buffer");

    /* 44100Hz, 2 Channels, S16 */
    PDMAUDIOPCMPROPS config = PDMAUDIOPCMPROPS_INITIALIZOR(
        16,                                                                 /* Bits */
        true,                                                               /* Signed */
        2,                                                                  /* Channels */
        44100,                                                              /* Hz */
        PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(16 /* Bits */, 2 /* Channels */), /* Shift */
        false                                                               /* Swap Endian */
    );

    RTTESTI_CHECK(DrvAudioHlpPCMPropsAreValid(&config));

    uint32_t cBufSize = _1K;

    /*
     * General stuff.
     */
    PDMAUDIOMIXBUF mb;
    RTTESTI_CHECK_RC_OK(AudioMixBufInit(&mb, "Single", &config, cBufSize));
    RTTESTI_CHECK(AudioMixBufSize(&mb) == cBufSize);
    RTTESTI_CHECK(AUDIOMIXBUF_B2F(&mb, AudioMixBufSizeBytes(&mb)) == cBufSize);
    RTTESTI_CHECK(AUDIOMIXBUF_F2B(&mb, AudioMixBufSize(&mb)) == AudioMixBufSizeBytes(&mb));
    RTTESTI_CHECK(AudioMixBufFree(&mb) == cBufSize);
    RTTESTI_CHECK(AUDIOMIXBUF_F2B(&mb, AudioMixBufFree(&mb)) == AudioMixBufFreeBytes(&mb));

    /*
     * Absolute writes.
     */
    uint32_t cFramesRead  = 0, cFramesWritten = 0, cFramesWrittenAbs = 0;
    int8_t  aFrames8 [2] = { 0x12, 0x34 };
    int16_t aFrames16[2] = { 0xAA, 0xBB };
    int32_t aFrames32[2] = { 0xCC, 0xDD };

    RTTESTI_CHECK_RC_OK(AudioMixBufWriteAt(&mb, 0 /* Offset */, &aFrames8, sizeof(aFrames8), &cFramesWritten));
    RTTESTI_CHECK(cFramesWritten == 0 /* Frames */);
    RTTESTI_CHECK(AudioMixBufUsed(&mb) == 0);

    RTTESTI_CHECK_RC_OK(AudioMixBufWriteAt(&mb, 0 /* Offset */, &aFrames16, sizeof(aFrames16), &cFramesWritten));
    RTTESTI_CHECK(cFramesWritten == 1 /* Frames */);
    RTTESTI_CHECK(AudioMixBufUsed(&mb) == 1);

    RTTESTI_CHECK_RC_OK(AudioMixBufWriteAt(&mb, 2 /* Offset */, &aFrames32, sizeof(aFrames32), &cFramesWritten));
    RTTESTI_CHECK(cFramesWritten == 2 /* Frames */);
    RTTESTI_CHECK(AudioMixBufUsed(&mb) == 2);

    /* Beyond buffer. */
    RTTESTI_CHECK_RC(AudioMixBufWriteAt(&mb, AudioMixBufSize(&mb) + 1, &aFrames16, sizeof(aFrames16),
                                        &cFramesWritten), VERR_BUFFER_OVERFLOW);

    /* Offset wrap-around: When writing as much (or more) frames the mixing buffer can hold. */
    uint32_t  cbSamples = cBufSize * sizeof(int16_t) * 2 /* Channels */;
    RTTESTI_CHECK(cbSamples);
    uint16_t *paSamples = (uint16_t *)RTMemAlloc(cbSamples);
    RTTESTI_CHECK(paSamples);
    RTTESTI_CHECK_RC_OK(AudioMixBufWriteAt(&mb, 0 /* Offset */, paSamples, cbSamples, &cFramesWritten));
    RTTESTI_CHECK(cFramesWritten == cBufSize /* Frames */);
    RTTESTI_CHECK(AudioMixBufUsed(&mb) == cBufSize);
    RTTESTI_CHECK(AudioMixBufReadPos(&mb) == 0);
    RTTESTI_CHECK(AudioMixBufWritePos(&mb) == 0);
    RTMemFree(paSamples);
    cbSamples = 0;

    /*
     * Circular writes.
     */
    AudioMixBufReset(&mb);

    RTTESTI_CHECK_RC_OK(AudioMixBufWriteAt(&mb, 2 /* Offset */, &aFrames32, sizeof(aFrames32), &cFramesWritten));
    RTTESTI_CHECK(cFramesWritten == 2 /* Frames */);
    RTTESTI_CHECK(AudioMixBufUsed(&mb) == 2);

    cFramesWrittenAbs = AudioMixBufUsed(&mb);

    uint32_t cToWrite = AudioMixBufSize(&mb) - cFramesWrittenAbs - 1; /* -1 as padding plus -2 frames for above. */
    for (uint32_t i = 0; i < cToWrite; i++)
    {
        RTTESTI_CHECK_RC_OK(AudioMixBufWriteCirc(&mb, &aFrames16, sizeof(aFrames16), &cFramesWritten));
        RTTESTI_CHECK(cFramesWritten == 1);
    }
    RTTESTI_CHECK(!AudioMixBufIsEmpty(&mb));
    RTTESTI_CHECK(AudioMixBufFree(&mb) == 1);
    RTTESTI_CHECK(AudioMixBufFreeBytes(&mb) == AUDIOMIXBUF_F2B(&mb, 1U));
    RTTESTI_CHECK(AudioMixBufUsed(&mb) == cToWrite + cFramesWrittenAbs /* + last absolute write */);

    RTTESTI_CHECK_RC_OK(AudioMixBufWriteCirc(&mb, &aFrames16, sizeof(aFrames16), &cFramesWritten));
    RTTESTI_CHECK(cFramesWritten == 1);
    RTTESTI_CHECK(AudioMixBufFree(&mb) == 0);
    RTTESTI_CHECK(AudioMixBufFreeBytes(&mb) == AUDIOMIXBUF_F2B(&mb, 0U));
    RTTESTI_CHECK(AudioMixBufUsed(&mb) == cBufSize);

    /* Circular reads. */
    uint32_t cToRead = AudioMixBufSize(&mb) - cFramesWrittenAbs - 1;
    for (uint32_t i = 0; i < cToRead; i++)
    {
        RTTESTI_CHECK_RC_OK(AudioMixBufAcquireReadBlock(&mb, &aFrames16, sizeof(aFrames16), &cFramesRead));
        RTTESTI_CHECK(cFramesRead == 1);
        AudioMixBufReleaseReadBlock(&mb, cFramesRead);
        AudioMixBufFinish(&mb, cFramesRead);
    }
    RTTESTI_CHECK(!AudioMixBufIsEmpty(&mb));
    RTTESTI_CHECK(AudioMixBufFree(&mb) == AudioMixBufSize(&mb) - cFramesWrittenAbs - 1);
    RTTESTI_CHECK(AudioMixBufFreeBytes(&mb) == AUDIOMIXBUF_F2B(&mb, cBufSize - cFramesWrittenAbs - 1));
    RTTESTI_CHECK(AudioMixBufUsed(&mb) == cBufSize - cToRead);

    RTTESTI_CHECK_RC_OK(AudioMixBufAcquireReadBlock(&mb, &aFrames16, sizeof(aFrames16), &cFramesRead));
    RTTESTI_CHECK(cFramesRead == 1);
    AudioMixBufReleaseReadBlock(&mb, cFramesRead);
    AudioMixBufFinish(&mb, cFramesRead);
    RTTESTI_CHECK(AudioMixBufFree(&mb) == cBufSize - cFramesWrittenAbs);
    RTTESTI_CHECK(AudioMixBufFreeBytes(&mb) == AUDIOMIXBUF_F2B(&mb, cBufSize - cFramesWrittenAbs));
    RTTESTI_CHECK(AudioMixBufUsed(&mb) == cFramesWrittenAbs);

    AudioMixBufDestroy(&mb);

    return RTTestSubErrorCount(hTest) ? VERR_GENERAL_FAILURE : VINF_SUCCESS;
}

static int tstParentChild(RTTEST hTest)
{
    uint32_t cParentBufSize = RTRandU32Ex(_1K /* Min */, _16K /* Max */); /* Enough room for random sizes */

    /* 44100Hz, 2 Channels, S16 */
    PDMAUDIOPCMPROPS cfg_p = PDMAUDIOPCMPROPS_INITIALIZOR(
        16,                                                                 /* Bits */
        true,                                                               /* Signed */
        2,                                                                  /* Channels */
        44100,                                                              /* Hz */
        PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(16 /* Bits */, 2 /* Channels */), /* Shift */
        false                                                               /* Swap Endian */
    );

    RTTESTI_CHECK(DrvAudioHlpPCMPropsAreValid(&cfg_p));

    PDMAUDIOMIXBUF parent;
    RTTESTI_CHECK_RC_OK(AudioMixBufInit(&parent, "Parent", &cfg_p, cParentBufSize));

    /* 22050Hz, 2 Channels, S16 */
    PDMAUDIOPCMPROPS cfg_c1 = PDMAUDIOPCMPROPS_INITIALIZOR(/* Upmixing to parent */
        16,                                                                 /* Bits */
        true,                                                               /* Signed */
        2,                                                                  /* Channels */
        22050,                                                              /* Hz */
        PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(16 /* Bits */, 2 /* Channels */), /* Shift */
        false                                                               /* Swap Endian */
    );

    RTTESTI_CHECK(DrvAudioHlpPCMPropsAreValid(&cfg_c1));

    uint32_t cFrames      = 16;
    uint32_t cChildBufSize = RTRandU32Ex(cFrames /* Min */, 64 /* Max */);

    PDMAUDIOMIXBUF child1;
    RTTESTI_CHECK_RC_OK(AudioMixBufInit(&child1, "Child1", &cfg_c1, cChildBufSize));
    RTTESTI_CHECK_RC_OK(AudioMixBufLinkTo(&child1, &parent));

    /* 48000Hz, 2 Channels, S16 */
    PDMAUDIOPCMPROPS cfg_c2 = PDMAUDIOPCMPROPS_INITIALIZOR(/* Downmixing to parent */
        16,                                                                 /* Bits */
        true,                                                               /* Signed */
        2,                                                                  /* Channels */
        48000,                                                              /* Hz */
        PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(16 /* Bits */, 2 /* Channels */), /* Shift */
        false                                                               /* Swap Endian */
    );

    RTTESTI_CHECK(DrvAudioHlpPCMPropsAreValid(&cfg_c2));

    PDMAUDIOMIXBUF child2;
    RTTESTI_CHECK_RC_OK(AudioMixBufInit(&child2, "Child2", &cfg_c2, cChildBufSize));
    RTTESTI_CHECK_RC_OK(AudioMixBufLinkTo(&child2, &parent));

    /*
     * Writing + mixing from child/children -> parent, sequential.
     */
    uint32_t cbBuf = _1K;
    char pvBuf[_1K];
    int16_t aFrames16[32] = { 0xAA, 0xBB };
    uint32_t cFramesRead, cFramesWritten, cFramesMixed;

    uint32_t cFramesChild1  = cFrames;
    uint32_t cFramesChild2  = cFrames;

    uint32_t t = RTRandU32() % 32;

    RTTestPrintf(hTest, RTTESTLVL_DEBUG,
                 "cParentBufSize=%RU32, cChildBufSize=%RU32, %RU32 frames -> %RU32 iterations total\n",
                 cParentBufSize, cChildBufSize, cFrames, t);

    /*
     * Using AudioMixBufWriteAt for writing to children.
     */
    RTTestSubF(hTest, "2 Children -> Parent (AudioMixBufWriteAt)");

    uint32_t cChildrenSamplesMixedTotal = 0;

    for (uint32_t i = 0; i < t; i++)
    {
        RTTestPrintf(hTest, RTTESTLVL_DEBUG, "i=%RU32\n", i);

        uint32_t cChild1Writes = RTRandU32() % 8;

        for (uint32_t c1 = 0; c1 < cChild1Writes; c1++)
        {
            /* Child 1. */
            RTTESTI_CHECK_RC_OK_BREAK(AudioMixBufWriteAt(&child1, 0, &aFrames16, sizeof(aFrames16), &cFramesWritten));
            RTTESTI_CHECK_MSG_BREAK(cFramesWritten == cFramesChild1, ("Child1: Expected %RU32 written frames, got %RU32\n", cFramesChild1, cFramesWritten));
            RTTESTI_CHECK_RC_OK_BREAK(AudioMixBufMixToParent(&child1, cFramesWritten, &cFramesMixed));

            cChildrenSamplesMixedTotal += cFramesMixed;

            RTTESTI_CHECK_MSG_BREAK(cFramesWritten == cFramesMixed, ("Child1: Expected %RU32 mixed frames, got %RU32\n", cFramesWritten, cFramesMixed));
            RTTESTI_CHECK_MSG_BREAK(AudioMixBufUsed(&child1) == 0, ("Child1: Expected %RU32 used frames, got %RU32\n", 0, AudioMixBufUsed(&child1)));
        }

        uint32_t cChild2Writes = RTRandU32() % 8;

        for (uint32_t c2 = 0; c2 < cChild2Writes; c2++)
        {
            /* Child 2. */
            RTTESTI_CHECK_RC_OK_BREAK(AudioMixBufWriteAt(&child2, 0, &aFrames16, sizeof(aFrames16), &cFramesWritten));
            RTTESTI_CHECK_MSG_BREAK(cFramesWritten == cFramesChild2, ("Child2: Expected %RU32 written frames, got %RU32\n", cFramesChild2, cFramesWritten));
            RTTESTI_CHECK_RC_OK_BREAK(AudioMixBufMixToParent(&child2, cFramesWritten, &cFramesMixed));

            cChildrenSamplesMixedTotal += cFramesMixed;

            RTTESTI_CHECK_MSG_BREAK(cFramesWritten == cFramesMixed, ("Child2: Expected %RU32 mixed frames, got %RU32\n", cFramesWritten, cFramesMixed));
            RTTESTI_CHECK_MSG_BREAK(AudioMixBufUsed(&child2) == 0, ("Child2: Expected %RU32 used frames, got %RU32\n", 0, AudioMixBufUsed(&child2)));
        }

        /*
         * Read out all frames from the parent buffer and also mark the just-read frames as finished
         * so that both connected children buffers can keep track of their stuff.
         */
        uint32_t cParentSamples = AudioMixBufUsed(&parent);
        while (cParentSamples)
        {
            RTTESTI_CHECK_RC_OK_BREAK(AudioMixBufAcquireReadBlock(&parent, pvBuf, cbBuf, &cFramesRead));
            if (!cFramesRead)
                break;

            AudioMixBufReleaseReadBlock(&parent, cFramesRead);
            AudioMixBufFinish(&parent, cFramesRead);

            RTTESTI_CHECK(cParentSamples >= cFramesRead);
            cParentSamples -= cFramesRead;
        }

        RTTESTI_CHECK(cParentSamples == 0);
    }

    RTTESTI_CHECK(AudioMixBufUsed(&parent) == 0);
    RTTESTI_CHECK(AudioMixBufLive(&child1) == 0);
    RTTESTI_CHECK(AudioMixBufLive(&child2) == 0);

    AudioMixBufDestroy(&parent);
    AudioMixBufDestroy(&child1);
    AudioMixBufDestroy(&child2);

    return RTTestSubErrorCount(hTest) ? VERR_GENERAL_FAILURE : VINF_SUCCESS;
}

/* Test 8-bit sample conversion (8-bit -> internal -> 8-bit). */
static int tstConversion8(RTTEST hTest)
{
    unsigned         i;
    uint32_t         cBufSize = 256;

    RTTestSubF(hTest, "Sample conversion (U8)");

    /* 44100Hz, 1 Channel, U8 */
    PDMAUDIOPCMPROPS cfg_p = PDMAUDIOPCMPROPS_INITIALIZOR(
        8,                                                                 /* Bits */
        false,                                                             /* Signed */
        1,                                                                 /* Channels */
        44100,                                                             /* Hz */
        PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(8 /* Bits */, 1 /* Channels */), /* Shift */
        false                                                              /* Swap Endian */
    );

    RTTESTI_CHECK(DrvAudioHlpPCMPropsAreValid(&cfg_p));

    PDMAUDIOMIXBUF parent;
    RTTESTI_CHECK_RC_OK(AudioMixBufInit(&parent, "Parent", &cfg_p, cBufSize));

    /* Child uses half the sample rate; that ensures the mixing engine can't
     * take shortcuts and performs conversion. Because conversion to double
     * the sample rate effectively inserts one additional sample between every
     * two source frames, N source frames will be converted to N * 2 - 1
     * frames. However, the last source sample will be saved for later
     * interpolation and not immediately output.
     */

    /* 22050Hz, 1 Channel, U8 */
    PDMAUDIOPCMPROPS cfg_c = PDMAUDIOPCMPROPS_INITIALIZOR( /* Upmixing to parent */
        8,                                                                 /* Bits */
        false,                                                             /* Signed */
        1,                                                                 /* Channels */
        22050,                                                             /* Hz */
        PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(8 /* Bits */, 1 /* Channels */), /* Shift */
        false                                                              /* Swap Endian */
    );

    RTTESTI_CHECK(DrvAudioHlpPCMPropsAreValid(&cfg_c));

    PDMAUDIOMIXBUF child;
    RTTESTI_CHECK_RC_OK(AudioMixBufInit(&child, "Child", &cfg_c, cBufSize));
    RTTESTI_CHECK_RC_OK(AudioMixBufLinkTo(&child, &parent));

    /* 8-bit unsigned frames. Often used with SB16 device. */
    uint8_t aFrames8U[16]  = { 0xAA, 0xBB, 0, 1, 43, 125, 126, 127,
                               128, 129, 130, 131, 132, UINT8_MAX - 1, UINT8_MAX, 0 };

    /*
     * Writing + mixing from child -> parent, sequential.
     */
    uint32_t    cbBuf = 256;
    char        achBuf[256];
    uint32_t    cFramesRead, cFramesWritten, cFramesMixed;

    uint32_t cFramesChild  = 16;
    uint32_t cFramesParent = cFramesChild * 2 - 2;
    uint32_t cFramesTotalRead   = 0;

    /**** 8-bit unsigned samples ****/
    RTTestPrintf(hTest, RTTESTLVL_DEBUG, "Conversion test %uHz %uch 8-bit\n", cfg_c.uHz, cfg_c.cChannels);
    RTTESTI_CHECK_RC_OK(AudioMixBufWriteCirc(&child, &aFrames8U, sizeof(aFrames8U), &cFramesWritten));
    RTTESTI_CHECK_MSG(cFramesWritten == cFramesChild, ("Child: Expected %RU32 written frames, got %RU32\n", cFramesChild, cFramesWritten));
    RTTESTI_CHECK_RC_OK(AudioMixBufMixToParent(&child, cFramesWritten, &cFramesMixed));
    uint32_t cFrames = AudioMixBufUsed(&parent);
    RTTESTI_CHECK_MSG(AudioMixBufLive(&child) == cFrames, ("Child: Expected %RU32 mixed frames, got %RU32\n", AudioMixBufLive(&child), cFrames));

    RTTESTI_CHECK(AudioMixBufUsed(&parent) == AudioMixBufLive(&child));

    for (;;)
    {
        RTTESTI_CHECK_RC_OK_BREAK(AudioMixBufAcquireReadBlock(&parent, achBuf, cbBuf, &cFramesRead));
        if (!cFramesRead)
            break;
        cFramesTotalRead += cFramesRead;
        AudioMixBufReleaseReadBlock(&parent, cFramesRead);
        AudioMixBufFinish(&parent, cFramesRead);
    }

    RTTESTI_CHECK_MSG(cFramesTotalRead == cFramesParent, ("Parent: Expected %RU32 mixed frames, got %RU32\n", cFramesParent, cFramesTotalRead));

    /* Check that the frames came out unharmed. Every other sample is interpolated and we ignore it. */
    /* NB: This also checks that the default volume setting is 0dB attenuation. */
    uint8_t *pSrc8 = &aFrames8U[0];
    uint8_t *pDst8 = (uint8_t *)achBuf;

    for (i = 0; i < cFramesChild - 1; ++i)
    {
        RTTESTI_CHECK_MSG(*pSrc8 == *pDst8, ("index %u: Dst=%d, Src=%d\n", i, *pDst8, *pSrc8));
        pSrc8 += 1;
        pDst8 += 2;
    }

    RTTESTI_CHECK(AudioMixBufUsed(&parent) == 0);
    RTTESTI_CHECK(AudioMixBufLive(&child)  == 0);

    AudioMixBufDestroy(&parent);
    AudioMixBufDestroy(&child);

    return RTTestSubErrorCount(hTest) ? VERR_GENERAL_FAILURE : VINF_SUCCESS;
}

/* Test 16-bit sample conversion (16-bit -> internal -> 16-bit). */
static int tstConversion16(RTTEST hTest)
{
    unsigned         i;
    uint32_t         cBufSize = 256;

    RTTestSubF(hTest, "Sample conversion (S16)");

    /* 44100Hz, 1 Channel, S16 */
    PDMAUDIOPCMPROPS cfg_p = PDMAUDIOPCMPROPS_INITIALIZOR(
        16,                                                                 /* Bits */
        true,                                                               /* Signed */
        1,                                                                  /* Channels */
        44100,                                                              /* Hz */
        PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(16 /* Bits */, 1 /* Channels */), /* Shift */
        false                                                               /* Swap Endian */
    );

    RTTESTI_CHECK(DrvAudioHlpPCMPropsAreValid(&cfg_p));

    PDMAUDIOMIXBUF parent;
    RTTESTI_CHECK_RC_OK(AudioMixBufInit(&parent, "Parent", &cfg_p, cBufSize));

    /* 22050Hz, 1 Channel, S16 */
    PDMAUDIOPCMPROPS cfg_c = PDMAUDIOPCMPROPS_INITIALIZOR( /* Upmixing to parent */
        16,                                                                 /* Bits */
        true,                                                               /* Signed */
        1,                                                                  /* Channels */
        22050,                                                              /* Hz */
        PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(16 /* Bits */, 1 /* Channels */), /* Shift */
        false                                                               /* Swap Endian */
    );

    RTTESTI_CHECK(DrvAudioHlpPCMPropsAreValid(&cfg_c));

    PDMAUDIOMIXBUF child;
    RTTESTI_CHECK_RC_OK(AudioMixBufInit(&child, "Child", &cfg_c, cBufSize));
    RTTESTI_CHECK_RC_OK(AudioMixBufLinkTo(&child, &parent));

    /* 16-bit signed. More or less exclusively used as output, and usually as input, too. */
    int16_t     aFrames16S[16] = { 0xAA, 0xBB, INT16_MIN, INT16_MIN + 1, INT16_MIN / 2, -3, -2, -1,
                                   0, 1, 2, 3, INT16_MAX / 2, INT16_MAX - 1, INT16_MAX, 0 };

    /*
     * Writing + mixing from child -> parent, sequential.
     */
    uint32_t    cbBuf = 256;
    char        achBuf[256];
    uint32_t    cFramesRead, cFramesWritten, cFramesMixed;

    uint32_t cFramesChild  = 16;
    uint32_t cFramesParent = cFramesChild * 2 - 2;
    uint32_t cFramesTotalRead   = 0;

    /**** 16-bit signed samples ****/
    RTTestPrintf(hTest, RTTESTLVL_DEBUG, "Conversion test %uHz %uch 16-bit\n", cfg_c.uHz, cfg_c.cChannels);
    RTTESTI_CHECK_RC_OK(AudioMixBufWriteCirc(&child, &aFrames16S, sizeof(aFrames16S), &cFramesWritten));
    RTTESTI_CHECK_MSG(cFramesWritten == cFramesChild, ("Child: Expected %RU32 written frames, got %RU32\n", cFramesChild, cFramesWritten));
    RTTESTI_CHECK_RC_OK(AudioMixBufMixToParent(&child, cFramesWritten, &cFramesMixed));
    uint32_t cFrames = AudioMixBufUsed(&parent);
    RTTESTI_CHECK_MSG(AudioMixBufLive(&child) == cFrames, ("Child: Expected %RU32 mixed frames, got %RU32\n", AudioMixBufLive(&child), cFrames));

    RTTESTI_CHECK(AudioMixBufUsed(&parent) == AudioMixBufLive(&child));

    for (;;)
    {
        RTTESTI_CHECK_RC_OK_BREAK(AudioMixBufAcquireReadBlock(&parent, achBuf, cbBuf, &cFramesRead));
        if (!cFramesRead)
            break;
        cFramesTotalRead += cFramesRead;
        AudioMixBufReleaseReadBlock(&parent, cFramesRead);
        AudioMixBufFinish(&parent, cFramesRead);
    }
    RTTESTI_CHECK_MSG(cFramesTotalRead == cFramesParent, ("Parent: Expected %RU32 mixed frames, got %RU32\n", cFramesParent, cFramesTotalRead));

    /* Check that the frames came out unharmed. Every other sample is interpolated and we ignore it. */
    /* NB: This also checks that the default volume setting is 0dB attenuation. */
    int16_t *pSrc16 = &aFrames16S[0];
    int16_t *pDst16 = (int16_t *)achBuf;

    for (i = 0; i < cFramesChild - 1; ++i)
    {
        RTTESTI_CHECK_MSG(*pSrc16 == *pDst16, ("index %u: Dst=%d, Src=%d\n", i, *pDst16, *pSrc16));
        pSrc16 += 1;
        pDst16 += 2;
    }

    RTTESTI_CHECK(AudioMixBufUsed(&parent) == 0);
    RTTESTI_CHECK(AudioMixBufLive(&child)  == 0);

    AudioMixBufDestroy(&parent);
    AudioMixBufDestroy(&child);

    return RTTestSubErrorCount(hTest) ? VERR_GENERAL_FAILURE : VINF_SUCCESS;
}

/* Test volume control. */
static int tstVolume(RTTEST hTest)
{
    unsigned         i;
    uint32_t         cBufSize = 256;

    RTTestSubF(hTest, "Volume control");

    /* Same for parent/child. */
    /* 44100Hz, 2 Channels, S16 */
    PDMAUDIOPCMPROPS cfg = PDMAUDIOPCMPROPS_INITIALIZOR(
        16,                                                                 /* Bits */
        true,                                                               /* Signed */
        2,                                                                  /* Channels */
        44100,                                                              /* Hz */
        PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(16 /* Bits */, 2 /* Channels */), /* Shift */
        false                                                               /* Swap Endian */
    );

    RTTESTI_CHECK(DrvAudioHlpPCMPropsAreValid(&cfg));

    PDMAUDIOVOLUME vol = { false, 0, 0 };   /* Not muted. */
    PDMAUDIOMIXBUF parent;
    RTTESTI_CHECK_RC_OK(AudioMixBufInit(&parent, "Parent", &cfg, cBufSize));

    PDMAUDIOMIXBUF child;
    RTTESTI_CHECK_RC_OK(AudioMixBufInit(&child, "Child", &cfg, cBufSize));
    RTTESTI_CHECK_RC_OK(AudioMixBufLinkTo(&child, &parent));

    /* A few 16-bit signed samples. */
    int16_t     aFrames16S[16] = { INT16_MIN, INT16_MIN + 1, -128, -64, -4, -1, 0, 1,
                                   2, 255, 256, INT16_MAX / 2, INT16_MAX - 2, INT16_MAX - 1, INT16_MAX, 0 };

    /*
     * Writing + mixing from child -> parent.
     */
    uint32_t    cbBuf = 256;
    char        achBuf[256];
    uint32_t    cFramesRead, cFramesWritten, cFramesMixed;

    uint32_t cFramesChild  = 8;
    uint32_t cFramesParent = cFramesChild;
    uint32_t cFramesTotalRead;
    int16_t *pSrc16;
    int16_t *pDst16;

    /**** Volume control test ****/
    RTTestPrintf(hTest, RTTESTLVL_DEBUG, "Volume control test %uHz %uch \n", cfg.uHz, cfg.cChannels);

    /* 1) Full volume/0dB attenuation (255). */
    vol.uLeft = vol.uRight = 255;
    AudioMixBufSetVolume(&child, &vol);

    RTTESTI_CHECK_RC_OK(AudioMixBufWriteCirc(&child, &aFrames16S, sizeof(aFrames16S), &cFramesWritten));
    RTTESTI_CHECK_MSG(cFramesWritten == cFramesChild, ("Child: Expected %RU32 written frames, got %RU32\n", cFramesChild, cFramesWritten));
    RTTESTI_CHECK_RC_OK(AudioMixBufMixToParent(&child, cFramesWritten, &cFramesMixed));

    cFramesTotalRead = 0;
    for (;;)
    {
        RTTESTI_CHECK_RC_OK_BREAK(AudioMixBufAcquireReadBlock(&parent, achBuf, cbBuf, &cFramesRead));
        if (!cFramesRead)
            break;
        cFramesTotalRead += cFramesRead;
        AudioMixBufReleaseReadBlock(&parent, cFramesRead);
        AudioMixBufFinish(&parent, cFramesRead);
    }
    RTTESTI_CHECK_MSG(cFramesTotalRead == cFramesParent, ("Parent: Expected %RU32 mixed frames, got %RU32\n", cFramesParent, cFramesTotalRead));

    /* Check that at 0dB the frames came out unharmed. */
    pSrc16 = &aFrames16S[0];
    pDst16 = (int16_t *)achBuf;

    for (i = 0; i < cFramesParent * 2 /* stereo */; ++i)
    {
        RTTESTI_CHECK_MSG(*pSrc16 == *pDst16, ("index %u: Dst=%d, Src=%d\n", i, *pDst16, *pSrc16));
        ++pSrc16;
        ++pDst16;
    }
    AudioMixBufReset(&child);

    /* 2) Half volume/-6dB attenuation (16 steps down). */
    vol.uLeft = vol.uRight = 255 - 16;
    AudioMixBufSetVolume(&child, &vol);

    RTTESTI_CHECK_RC_OK(AudioMixBufWriteCirc(&child, &aFrames16S, sizeof(aFrames16S), &cFramesWritten));
    RTTESTI_CHECK_MSG(cFramesWritten == cFramesChild, ("Child: Expected %RU32 written frames, got %RU32\n", cFramesChild, cFramesWritten));
    RTTESTI_CHECK_RC_OK(AudioMixBufMixToParent(&child, cFramesWritten, &cFramesMixed));

    cFramesTotalRead = 0;
    for (;;)
    {
        RTTESTI_CHECK_RC_OK_BREAK(AudioMixBufAcquireReadBlock(&parent, achBuf, cbBuf, &cFramesRead));
        if (!cFramesRead)
            break;
        cFramesTotalRead += cFramesRead;
        AudioMixBufReleaseReadBlock(&parent, cFramesRead);
        AudioMixBufFinish(&parent, cFramesRead);
    }
    RTTESTI_CHECK_MSG(cFramesTotalRead == cFramesParent, ("Parent: Expected %RU32 mixed frames, got %RU32\n", cFramesParent, cFramesTotalRead));

    /* Check that at -6dB the sample values are halved. */
    pSrc16 = &aFrames16S[0];
    pDst16 = (int16_t *)achBuf;

    for (i = 0; i < cFramesParent * 2 /* stereo */; ++i)
    {
        /* Watch out! For negative values, x >> 1 is not the same as x / 2. */
        RTTESTI_CHECK_MSG(*pSrc16 >> 1 == *pDst16, ("index %u: Dst=%d, Src=%d\n", i, *pDst16, *pSrc16));
        ++pSrc16;
        ++pDst16;
    }

    AudioMixBufDestroy(&parent);
    AudioMixBufDestroy(&child);

    return RTTestSubErrorCount(hTest) ? VERR_GENERAL_FAILURE : VINF_SUCCESS;
}

int main(int argc, char **argv)
{
    RTR3InitExe(argc, &argv, 0);

    /*
     * Initialize IPRT and create the test.
     */
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstAudioMixBuffer", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);

    rc = tstSingle(hTest);
    if (RT_SUCCESS(rc))
        rc = tstParentChild(hTest);
    if (RT_SUCCESS(rc))
        rc = tstConversion8(hTest);
    if (RT_SUCCESS(rc))
        rc = tstConversion16(hTest);
    if (RT_SUCCESS(rc))
        rc = tstVolume(hTest);

    /*
     * Summary
     */
    return RTTestSummaryAndDestroy(hTest);
}
