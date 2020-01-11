/* $Id: DevSB16.cpp $ */
/** @file
 * DevSB16 - VBox SB16 Audio Controller.
 */

/*
 * Copyright (C) 2015-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 * --------------------------------------------------------------------
 *
 * This code is based on: sb16.c from QEMU AUDIO subsystem (r3917).
 * QEMU Soundblaster 16 emulation
 *
 * Copyright (c) 2003-2005 Vassili Karpov (malc)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEV_SB16
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/file.h>
#ifdef IN_RING3
# include <iprt/mem.h>
# include <iprt/string.h>
# include <iprt/uuid.h>
#endif

#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmaudioifs.h>

#include "VBoxDD.h"

#include "AudioMixBuffer.h"
#include "AudioMixer.h"
#include "DrvAudio.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** Current saved state version. */
#define SB16_SAVE_STATE_VERSION         2
/** The version used in VirtualBox version 3.0 and earlier. This didn't include the config dump. */
#define SB16_SAVE_STATE_VERSION_VBOX_30 1


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static const char e3[] = "COPYRIGHT (C) CREATIVE TECHNOLOGY LTD, 1992.";



/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Structure defining a (host backend) driver stream.
 * Each driver has its own instances of audio mixer streams, which then
 * can go into the same (or even different) audio mixer sinks.
 */
typedef struct SB16DRIVERSTREAM
{
    /** Associated PDM audio stream. */
    R3PTRTYPE(PPDMAUDIOSTREAM)         pStream;
    /** The stream's current configuration. */
} SB16DRIVERSTREAM, *PSB16DRIVERSTREAM;

/**
 * Struct for maintaining a host backend driver.
 */
typedef struct SB16STATE *PSB16STATE;
typedef struct SB16DRIVER
{
    /** Node for storing this driver in our device driver list of SB16STATE. */
    RTLISTNODER3                       Node;
    /** Pointer to SB16 controller (state). */
    R3PTRTYPE(PSB16STATE)              pSB16State;
    /** Driver flags. */
    PDMAUDIODRVFLAGS                   fFlags;
    uint32_t                           PaddingFlags;
    /** LUN # to which this driver has been assigned. */
    uint8_t                            uLUN;
    /** Whether this driver is in an attached state or not. */
    bool                               fAttached;
    uint8_t                            Padding[4];
    /** Pointer to attached driver base interface. */
    R3PTRTYPE(PPDMIBASE)               pDrvBase;
    /** Audio connector interface to the underlying host backend. */
    R3PTRTYPE(PPDMIAUDIOCONNECTOR)     pConnector;
    /** Stream for output. */
    SB16DRIVERSTREAM                   Out;
} SB16DRIVER, *PSB16DRIVER;

/**
 * Structure for a SB16 stream.
 */
typedef struct SB16STREAM
{
    /** The stream's current configuration. */
    PDMAUDIOSTREAMCFG                  Cfg;
} SB16STREAM, *PSB16STREAM;

typedef struct SB16STATE
{
#ifdef VBOX
    /** Pointer to the device instance. */
    PPDMDEVINSR3        pDevInsR3;
    /** Pointer to the connector of the attached audio driver. */
    PPDMIAUDIOCONNECTOR pDrv;
    int irqCfg;
    int dmaCfg;
    int hdmaCfg;
    int portCfg;
    int verCfg;
#endif
    int irq;
    int dma;
    int hdma;
    int port;
    int ver;

    int in_index;
    int out_data_len;
    int fmt_stereo;
    int fmt_signed;
    int fmt_bits;
    PDMAUDIOFMT fmt;
    int dma_auto;
    int block_size;
    int fifo;
    int freq;
    int time_const;
    int speaker;
    int needed_bytes;
    int cmd;
    int use_hdma;
    int highspeed;
    int can_write; /** @todo Value never gets 0? */

    int v2x6;

    uint8_t csp_param;
    uint8_t csp_value;
    uint8_t csp_mode;
    uint8_t csp_regs[256];
    uint8_t csp_index;
    uint8_t csp_reg83[4];
    int csp_reg83r;
    int csp_reg83w;

    uint8_t in2_data[10];
    uint8_t out_data[50];
    uint8_t test_reg;
    uint8_t last_read_byte;
    int nzero;

    int left_till_irq; /** Note: Can be < 0. */

    int dma_running;
    int bytes_per_second;
    int align;

    RTLISTANCHOR                   lstDrv;
    /** Number of active (running) SDn streams. */
    uint8_t                        cStreamsActive;
    /** The timer for pumping data thru the attached LUN drivers. */
    PTMTIMERR3                     pTimerIO;
    /** Flag indicating whether the timer is active or not. */
    bool                           fTimerActive;
    uint8_t                        u8Padding1[7];
    /** The timer interval for pumping data thru the LUN drivers in timer ticks. */
    uint64_t                       cTimerTicksIO;
    /** Timestamp of the last timer callback (sb16TimerIO).
     * Used to calculate the time actually elapsed between two timer callbacks. */
    uint64_t                       uTimerTSIO;
    PTMTIMER                       pTimerIRQ;
    /** The base interface for LUN\#0. */
    PDMIBASE                       IBase;
    /** Output stream. */
    SB16STREAM                     Out;

    /* mixer state */
    uint8_t mixer_nreg;
    uint8_t mixer_regs[256];
} SB16STATE, *PSB16STATE;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int sb16CheckAndReOpenOut(PSB16STATE pThis);
static int sb16OpenOut(PSB16STATE pThis, PPDMAUDIOSTREAMCFG pCfg);
static void sb16CloseOut(PSB16STATE pThis);
static void sb16TimerMaybeStart(PSB16STATE pThis);
static void sb16TimerMaybeStop(PSB16STATE pThis);



static int magic_of_irq(int irq)
{
    switch (irq)
    {
        case 5:
            return 2;
        case 7:
            return 4;
        case 9:
            return 1;
        case 10:
            return 8;
        default:
            break;
    }

    LogFlowFunc(("bad irq %d\n", irq));
    return 2;
}

static int irq_of_magic(int magic)
{
    switch (magic)
    {
        case 1:
            return 9;
        case 2:
            return 5;
        case 4:
            return 7;
        case 8:
            return 10;
        default:
            break;
    }

    LogFlowFunc(("bad irq magic %d\n", magic));
    return -1;
}

#if 0 // unused // def DEBUG
DECLINLINE(void) log_dsp(PSB16STATE pThis)
{
    LogFlowFunc(("%s:%s:%d:%s:dmasize=%d:freq=%d:const=%d:speaker=%d\n",
                 pThis->fmt_stereo ? "Stereo" : "Mono",
                 pThis->fmt_signed ? "Signed" : "Unsigned",
                 pThis->fmt_bits,
                 pThis->dma_auto ? "Auto" : "Single",
                 pThis->block_size,
                 pThis->freq,
                 pThis->time_const,
                 pThis->speaker));
}
#endif

static void sb16SpeakerControl(PSB16STATE pThis, int on)
{
    pThis->speaker = on;
    /* AUD_enable (pThis->voice, on); */
}

static void sb16Control(PSB16STATE pThis, int hold)
{
    int dma = pThis->use_hdma ? pThis->hdma : pThis->dma;
    pThis->dma_running = hold;

    LogFlowFunc(("hold %d high %d dma %d\n", hold, pThis->use_hdma, dma));

    PDMDevHlpDMASetDREQ(pThis->pDevInsR3, dma, hold);

    PSB16DRIVER pDrv;
    RTListForEach(&pThis->lstDrv, pDrv, SB16DRIVER, Node)
    {
        if (!pDrv->Out.pStream)
            continue;

        int rc2 = pDrv->pConnector->pfnStreamControl(pDrv->pConnector, pDrv->Out.pStream,
                                                     hold == 1 ? PDMAUDIOSTREAMCMD_ENABLE : PDMAUDIOSTREAMCMD_DISABLE);
        LogFlowFunc(("%s: rc=%Rrc\n", pDrv->Out.pStream->szName, rc2)); NOREF(rc2);
    }

    if (hold)
    {
        pThis->cStreamsActive++;
        sb16TimerMaybeStart(pThis);
        PDMDevHlpDMASchedule(pThis->pDevInsR3);
    }
    else
    {
        if (pThis->cStreamsActive)
            pThis->cStreamsActive--;
        sb16TimerMaybeStop(pThis);
    }
}

/**
 * @callback_method_impl{PFNTMTIMERDEV}
 */
static DECLCALLBACK(void) sb16TimerIRQ(PPDMDEVINS pDevIns, PTMTIMER pTimer, void *pvThis)
{
    RT_NOREF(pDevIns, pTimer);
    PSB16STATE pThis = (PSB16STATE)pvThis;
    pThis->can_write = 1;
    PDMDevHlpISASetIrq(pThis->pDevInsR3, pThis->irq, 1);
}

#define DMA8_AUTO 1
#define DMA8_HIGH 2

static void continue_dma8(PSB16STATE pThis)
{
    sb16CheckAndReOpenOut(pThis);
    sb16Control(pThis, 1);
}

static void dma_cmd8(PSB16STATE pThis, int mask, int dma_len)
{
    pThis->fmt        = PDMAUDIOFMT_U8;
    pThis->use_hdma   = 0;
    pThis->fmt_bits   = 8;
    pThis->fmt_signed = 0;
    pThis->fmt_stereo = (pThis->mixer_regs[0x0e] & 2) != 0;

    if (-1 == pThis->time_const)
    {
        if (pThis->freq <= 0)
            pThis->freq = 11025;
    }
    else
    {
        int tmp = (256 - pThis->time_const);
        pThis->freq = (1000000 + (tmp / 2)) / tmp;
    }

    if (dma_len != -1)
    {
        pThis->block_size = dma_len << pThis->fmt_stereo;
    }
    else
    {
        /* This is apparently the only way to make both Act1/PL
           and SecondReality/FC work

           r=andy Wow, actually someone who remembers Future Crew :-)

           Act1 sets block size via command 0x48 and it's an odd number
           SR does the same with even number
           Both use stereo, and Creatives own documentation states that
           0x48 sets block size in bytes less one.. go figure */
        pThis->block_size &= ~pThis->fmt_stereo;
    }

    pThis->freq >>= pThis->fmt_stereo;
    pThis->left_till_irq = pThis->block_size;
    pThis->bytes_per_second = (pThis->freq << pThis->fmt_stereo);
    /* pThis->highspeed = (mask & DMA8_HIGH) != 0; */
    pThis->dma_auto = (mask & DMA8_AUTO) != 0;
    pThis->align = (1 << pThis->fmt_stereo) - 1;

    if (pThis->block_size & pThis->align)
        LogFlowFunc(("warning: misaligned block size %d, alignment %d\n",
                     pThis->block_size, pThis->align + 1));

    LogFlowFunc(("freq %d, stereo %d, sign %d, bits %d, dma %d, auto %d, fifo %d, high %d\n",
                 pThis->freq, pThis->fmt_stereo, pThis->fmt_signed, pThis->fmt_bits,
                 pThis->block_size, pThis->dma_auto, pThis->fifo, pThis->highspeed));

    continue_dma8(pThis);
    sb16SpeakerControl(pThis, 1);
}

static void dma_cmd(PSB16STATE pThis, uint8_t cmd, uint8_t d0, int dma_len)
{
    pThis->use_hdma   = cmd < 0xc0;
    pThis->fifo       = (cmd >> 1) & 1;
    pThis->dma_auto   = (cmd >> 2) & 1;
    pThis->fmt_signed = (d0 >> 4) & 1;
    pThis->fmt_stereo = (d0 >> 5) & 1;

    switch (cmd >> 4)
    {
        case 11:
            pThis->fmt_bits = 16;
            break;

        case 12:
            pThis->fmt_bits = 8;
            break;
    }

    if (-1 != pThis->time_const)
    {
#if 1
        int tmp = 256 - pThis->time_const;
        pThis->freq = (1000000 + (tmp / 2)) / tmp;
#else
        /* pThis->freq = 1000000 / ((255 - pThis->time_const) << pThis->fmt_stereo); */
        pThis->freq = 1000000 / ((255 - pThis->time_const));
#endif
        pThis->time_const = -1;
    }

    pThis->block_size = dma_len + 1;
    pThis->block_size <<= ((pThis->fmt_bits == 16) ? 1 : 0);
    if (!pThis->dma_auto)
    {
        /*
         * It is clear that for DOOM and auto-init this value
         * shouldn't take stereo into account, while Miles Sound Systems
         * setsound.exe with single transfer mode wouldn't work without it
         * wonders of SB16 yet again.
         */
        pThis->block_size <<= pThis->fmt_stereo;
    }

    LogFlowFunc(("freq %d, stereo %d, sign %d, bits %d, dma %d, auto %d, fifo %d, high %d\n",
                 pThis->freq, pThis->fmt_stereo, pThis->fmt_signed, pThis->fmt_bits,
                 pThis->block_size, pThis->dma_auto, pThis->fifo, pThis->highspeed));

    if (16 == pThis->fmt_bits)
        pThis->fmt = pThis->fmt_signed ? PDMAUDIOFMT_S16 : PDMAUDIOFMT_U16;
    else
        pThis->fmt = pThis->fmt_signed ? PDMAUDIOFMT_S8 : PDMAUDIOFMT_U8;

    pThis->left_till_irq = pThis->block_size;

    pThis->bytes_per_second = (pThis->freq << pThis->fmt_stereo) << ((pThis->fmt_bits == 16) ? 1 : 0);
    pThis->highspeed = 0;
    pThis->align = (1 << (pThis->fmt_stereo + (pThis->fmt_bits == 16))) - 1;
    if (pThis->block_size & pThis->align)
    {
        LogFlowFunc(("warning: misaligned block size %d, alignment %d\n",
                     pThis->block_size, pThis->align + 1));
    }

    sb16CheckAndReOpenOut(pThis);
    sb16Control(pThis, 1);
    sb16SpeakerControl(pThis, 1);
}

static inline void dsp_out_data (PSB16STATE pThis, uint8_t val)
{
    LogFlowFunc(("outdata %#x\n", val));
    if ((size_t) pThis->out_data_len < sizeof (pThis->out_data)) {
        pThis->out_data[pThis->out_data_len++] = val;
    }
}

static inline uint8_t dsp_get_data (PSB16STATE pThis)
{
    if (pThis->in_index) {
        return pThis->in2_data[--pThis->in_index];
    }
    else {
        LogFlowFunc(("buffer underflow\n"));
        return 0;
    }
}

static void sb16HandleCommand(PSB16STATE pThis, uint8_t cmd)
{
    LogFlowFunc(("command %#x\n", cmd));

    if (cmd > 0xaf && cmd < 0xd0)
    {
        if (cmd & 8) /** @todo Handle recording. */
            LogFlowFunc(("ADC not yet supported (command %#x)\n", cmd));

        switch (cmd >> 4)
        {
            case 11:
            case 12:
                break;
            default:
                LogFlowFunc(("%#x wrong bits\n", cmd));
        }

        pThis->needed_bytes = 3;
    }
    else
    {
        pThis->needed_bytes = 0;

        switch (cmd)
        {
            case 0x03:
                dsp_out_data(pThis, 0x10); /* pThis->csp_param); */
                goto warn;

            case 0x04:
                pThis->needed_bytes = 1;
                goto warn;

            case 0x05:
                pThis->needed_bytes = 2;
                goto warn;

            case 0x08:
                /* __asm__ ("int3"); */
                goto warn;

            case 0x0e:
                pThis->needed_bytes = 2;
                goto warn;

            case 0x09:
                dsp_out_data(pThis, 0xf8);
                goto warn;

            case 0x0f:
                pThis->needed_bytes = 1;
                goto warn;

            case 0x10:
                pThis->needed_bytes = 1;
                goto warn;

            case 0x14:
                pThis->needed_bytes = 2;
                pThis->block_size = 0;
                break;

            case 0x1c:              /* Auto-Initialize DMA DAC, 8-bit */
                dma_cmd8(pThis, DMA8_AUTO, -1);
                break;

            case 0x20:              /* Direct ADC, Juice/PL */
                dsp_out_data(pThis, 0xff);
                goto warn;

            case 0x35:
                LogFlowFunc(("0x35 - MIDI command not implemented\n"));
                break;

            case 0x40:
                pThis->freq = -1;
                pThis->time_const = -1;
                pThis->needed_bytes = 1;
                break;

            case 0x41:
                pThis->freq = -1;
                pThis->time_const = -1;
                pThis->needed_bytes = 2;
                break;

            case 0x42:
                pThis->freq = -1;
                pThis->time_const = -1;
                pThis->needed_bytes = 2;
                goto warn;

            case 0x45:
                dsp_out_data(pThis, 0xaa);
                goto warn;

            case 0x47:                /* Continue Auto-Initialize DMA 16bit */
                break;

            case 0x48:
                pThis->needed_bytes = 2;
                break;

            case 0x74:
                pThis->needed_bytes = 2; /* DMA DAC, 4-bit ADPCM */
                LogFlowFunc(("0x75 - DMA DAC, 4-bit ADPCM not implemented\n"));
                break;

            case 0x75:              /* DMA DAC, 4-bit ADPCM Reference */
                pThis->needed_bytes = 2;
                LogFlowFunc(("0x74 - DMA DAC, 4-bit ADPCM Reference not implemented\n"));
                break;

            case 0x76:              /* DMA DAC, 2.6-bit ADPCM */
                pThis->needed_bytes = 2;
                LogFlowFunc(("0x74 - DMA DAC, 2.6-bit ADPCM not implemented\n"));
                break;

            case 0x77:              /* DMA DAC, 2.6-bit ADPCM Reference */
                pThis->needed_bytes = 2;
                LogFlowFunc(("0x74 - DMA DAC, 2.6-bit ADPCM Reference not implemented\n"));
                break;

            case 0x7d:
                LogFlowFunc(("0x7d - Autio-Initialize DMA DAC, 4-bit ADPCM Reference\n"));
                LogFlowFunc(("not implemented\n"));
                break;

            case 0x7f:
                LogFlowFunc(("0x7d - Autio-Initialize DMA DAC, 2.6-bit ADPCM Reference\n"));
                LogFlowFunc(("not implemented\n"));
                break;

            case 0x80:
                pThis->needed_bytes = 2;
                break;

            case 0x90:
            case 0x91:
                dma_cmd8(pThis, (((cmd & 1) == 0) ? 1 : 0) | DMA8_HIGH, -1);
                break;

            case 0xd0:              /* halt DMA operation. 8bit */
                sb16Control(pThis, 0);
                break;

            case 0xd1:              /* speaker on */
                sb16SpeakerControl(pThis, 1);
                break;

            case 0xd3:              /* speaker off */
                sb16SpeakerControl(pThis, 0);
                break;

            case 0xd4:              /* continue DMA operation. 8bit */
                /* KQ6 (or maybe Sierras audblst.drv in general) resets
                   the frequency between halt/continue */
                continue_dma8(pThis);
                break;

            case 0xd5:              /* halt DMA operation. 16bit */
                sb16Control(pThis, 0);
                break;

            case 0xd6:              /* continue DMA operation. 16bit */
                sb16Control(pThis, 1);
                break;

            case 0xd9:              /* exit auto-init DMA after this block. 16bit */
                pThis->dma_auto = 0;
                break;

            case 0xda:              /* exit auto-init DMA after this block. 8bit */
                pThis->dma_auto = 0;
                break;

            case 0xe0:              /* DSP identification */
                pThis->needed_bytes = 1;
                break;

            case 0xe1:
                dsp_out_data(pThis, pThis->ver & 0xff);
                dsp_out_data(pThis, pThis->ver >> 8);
                break;

            case 0xe2:
                pThis->needed_bytes = 1;
                goto warn;

            case 0xe3:
            {
                for (int i = sizeof (e3) - 1; i >= 0; --i)
                    dsp_out_data(pThis, e3[i]);

                break;
            }

            case 0xe4:              /* write test reg */
                pThis->needed_bytes = 1;
                break;

            case 0xe7:
                LogFlowFunc(("Attempt to probe for ESS (0xe7)?\n"));
                break;

            case 0xe8:              /* read test reg */
                dsp_out_data(pThis, pThis->test_reg);
                break;

            case 0xf2:
            case 0xf3:
                dsp_out_data(pThis, 0xaa);
                pThis->mixer_regs[0x82] |= (cmd == 0xf2) ? 1 : 2;
                PDMDevHlpISASetIrq(pThis->pDevInsR3, pThis->irq, 1);
                break;

            case 0xf8:
                /* Undocumented, used by old Creative diagnostic programs. */
                dsp_out_data (pThis, 0);
                goto warn;

            case 0xf9:
                pThis->needed_bytes = 1;
                goto warn;

            case 0xfa:
                dsp_out_data (pThis, 0);
                goto warn;

            case 0xfc:              /* FIXME */
                dsp_out_data (pThis, 0);
                goto warn;

            default:
                LogFlowFunc(("Unrecognized command %#x\n", cmd));
                break;
        }
    }

    if (!pThis->needed_bytes)
        LogFlow(("\n"));

exit:

     if (!pThis->needed_bytes)
        pThis->cmd = -1;
     else
        pThis->cmd = cmd;

    return;

warn:
    LogFlowFunc(("warning: command %#x,%d is not truly understood yet\n",
                 cmd, pThis->needed_bytes));
    goto exit;
}

static uint16_t dsp_get_lohi (PSB16STATE pThis)
{
    uint8_t hi = dsp_get_data (pThis);
    uint8_t lo = dsp_get_data (pThis);
    return (hi << 8) | lo;
}

static uint16_t dsp_get_hilo (PSB16STATE pThis)
{
    uint8_t lo = dsp_get_data (pThis);
    uint8_t hi = dsp_get_data (pThis);
    return (hi << 8) | lo;
}

static void complete(PSB16STATE pThis)
{
    int d0, d1, d2;
    LogFlowFunc(("complete command %#x, in_index %d, needed_bytes %d\n",
            pThis->cmd, pThis->in_index, pThis->needed_bytes));

    if (pThis->cmd > 0xaf && pThis->cmd < 0xd0)
    {
        d2 = dsp_get_data (pThis);
        d1 = dsp_get_data (pThis);
        d0 = dsp_get_data (pThis);

        if (pThis->cmd & 8)
            LogFlowFunc(("ADC params cmd = %#x d0 = %d, d1 = %d, d2 = %d\n", pThis->cmd, d0, d1, d2));
        else
        {
            LogFlowFunc(("cmd = %#x d0 = %d, d1 = %d, d2 = %d\n", pThis->cmd, d0, d1, d2));
            dma_cmd(pThis, pThis->cmd, d0, d1 + (d2 << 8));
        }
    }
    else
    {
        switch (pThis->cmd)
        {
        case 0x04:
            pThis->csp_mode = dsp_get_data (pThis);
            pThis->csp_reg83r = 0;
            pThis->csp_reg83w = 0;
            LogFlowFunc(("CSP command 0x04: mode=%#x\n", pThis->csp_mode));
            break;

        case 0x05:
            pThis->csp_param = dsp_get_data (pThis);
            pThis->csp_value = dsp_get_data (pThis);
            LogFlowFunc(("CSP command 0x05: param=%#x value=%#x\n",
                    pThis->csp_param,
                    pThis->csp_value));
            break;

        case 0x0e:
        {
            d0 = dsp_get_data(pThis);
            d1 = dsp_get_data(pThis);
            LogFlowFunc(("write CSP register %d <- %#x\n", d1, d0));
            if (d1 == 0x83)
            {
                LogFlowFunc(("0x83[%d] <- %#x\n", pThis->csp_reg83r, d0));
                pThis->csp_reg83[pThis->csp_reg83r % 4] = d0;
                pThis->csp_reg83r += 1;
            }
            else
                pThis->csp_regs[d1] = d0;
            break;
        }

        case 0x0f:
            d0 = dsp_get_data(pThis);
            LogFlowFunc(("read CSP register %#x -> %#x, mode=%#x\n", d0, pThis->csp_regs[d0], pThis->csp_mode));
            if (d0 == 0x83)
            {
                LogFlowFunc(("0x83[%d] -> %#x\n",
                        pThis->csp_reg83w,
                        pThis->csp_reg83[pThis->csp_reg83w % 4]));
                dsp_out_data (pThis, pThis->csp_reg83[pThis->csp_reg83w % 4]);
                pThis->csp_reg83w += 1;
            }
            else
                dsp_out_data(pThis, pThis->csp_regs[d0]);
            break;

        case 0x10:
            d0 = dsp_get_data(pThis);
            LogFlowFunc(("cmd 0x10 d0=%#x\n", d0));
            break;

        case 0x14:
            dma_cmd8(pThis, 0, dsp_get_lohi (pThis) + 1);
            break;

        case 0x40:
            pThis->time_const = dsp_get_data(pThis);
            LogFlowFunc(("set time const %d\n", pThis->time_const));
            break;

        case 0x42:              /* FT2 sets output freq with this, go figure */
#if 0
            LogFlowFunc(("cmd 0x42 might not do what it think it should\n"));
#endif
        case 0x41:
            pThis->freq = dsp_get_hilo(pThis);
            LogFlowFunc(("set freq %d\n", pThis->freq));
            break;

        case 0x48:
            pThis->block_size = dsp_get_lohi(pThis) + 1;
            LogFlowFunc(("set dma block len %d\n", pThis->block_size));
            break;

        case 0x74:
        case 0x75:
        case 0x76:
        case 0x77:
            /* ADPCM stuff, ignore */
            break;

        case 0x80:
        {
            int freq, samples, bytes;
            uint64_t ticks;

            freq = pThis->freq > 0 ? pThis->freq : 11025;
            samples = dsp_get_lohi (pThis) + 1;
            bytes = samples << pThis->fmt_stereo << ((pThis->fmt_bits == 16) ? 1 : 0);
            ticks = (bytes * TMTimerGetFreq(pThis->pTimerIRQ)) / freq;
            if (ticks < TMTimerGetFreq(pThis->pTimerIRQ) / 1024)
                PDMDevHlpISASetIrq(pThis->pDevInsR3, pThis->irq, 1);
            else
                TMTimerSet(pThis->pTimerIRQ, TMTimerGet(pThis->pTimerIRQ) + ticks);
            LogFlowFunc(("mix silence: %d samples, %d bytes, %RU64 ticks\n", samples, bytes, ticks));
            break;
        }

        case 0xe0:
            d0 = dsp_get_data(pThis);
            pThis->out_data_len = 0;
            LogFlowFunc(("E0 data = %#x\n", d0));
            dsp_out_data(pThis, ~d0);
            break;

        case 0xe2:
            d0 = dsp_get_data(pThis);
            LogFlow(("SB16:E2 = %#x\n", d0));
            break;

        case 0xe4:
            pThis->test_reg = dsp_get_data(pThis);
            break;

        case 0xf9:
            d0 = dsp_get_data(pThis);
            LogFlowFunc(("command 0xf9 with %#x\n", d0));
            switch (d0) {
            case 0x0e:
                dsp_out_data(pThis, 0xff);
                break;

            case 0x0f:
                dsp_out_data(pThis, 0x07);
                break;

            case 0x37:
                dsp_out_data(pThis, 0x38);
                break;

            default:
                dsp_out_data(pThis, 0x00);
                break;
            }
            break;

        default:
            LogFlowFunc(("complete: unrecognized command %#x\n", pThis->cmd));
            return;
        }
    }

    LogFlow(("\n"));
    pThis->cmd = -1;
    return;
}

static uint8_t sb16MixRegToVol(PSB16STATE pThis, int reg)
{
    /* The SB16 mixer has a 0 to -62dB range in 32 levels (2dB each step).
     * We use a 0 to -96dB range in 256 levels (0.375dB each step).
     * Only the top 5 bits of a mixer register are used.
     */
    uint8_t steps = 31 - (pThis->mixer_regs[reg] >> 3);
    uint8_t vol   = 255 - steps * 16 / 3;   /* (2dB*8) / (0.375dB*8) */
    return vol;
}

/**
 * Returns the device's current master volume.
 *
 * @param   pThis               SB16 state.
 * @param   pVol                Where to store the master volume information.
 */
static void sb16GetMasterVolume(PSB16STATE pThis, PPDMAUDIOVOLUME pVol)
{
    /* There's no mute switch, only volume controls. */
    uint8_t lvol = sb16MixRegToVol(pThis, 0x30);
    uint8_t rvol = sb16MixRegToVol(pThis, 0x31);

    pVol->fMuted = false;
    pVol->uLeft  = lvol;
    pVol->uRight = rvol;
}

/**
 * Returns the device's current output stream volume.
 *
 * @param   pThis               SB16 state.
 * @param   pVol                Where to store the output stream volume information.
 */
static void sb16GetPcmOutVolume(PSB16STATE pThis, PPDMAUDIOVOLUME pVol)
{
    /* There's no mute switch, only volume controls. */
    uint8_t lvol = sb16MixRegToVol(pThis, 0x32);
    uint8_t rvol = sb16MixRegToVol(pThis, 0x33);

    pVol->fMuted = false;
    pVol->uLeft  = lvol;
    pVol->uRight = rvol;
}

static void sb16UpdateVolume(PSB16STATE pThis)
{
    PDMAUDIOVOLUME VolMaster;
    sb16GetMasterVolume(pThis, &VolMaster);

    PDMAUDIOVOLUME VolOut;
    sb16GetPcmOutVolume(pThis, &VolOut);

    /* Combine the master + output stream volume. */
    PDMAUDIOVOLUME VolCombined;
    RT_ZERO(VolCombined);

    VolCombined.fMuted = VolMaster.fMuted || VolOut.fMuted;
    if (!VolCombined.fMuted)
    {
        VolCombined.uLeft  = (   (VolOut.uLeft    ? VolOut.uLeft     : 1)
                               * (VolMaster.uLeft ? VolMaster.uLeft  : 1)) / PDMAUDIO_VOLUME_MAX;

        VolCombined.uRight = (  (VolOut.uRight    ? VolOut.uRight    : 1)
                              * (VolMaster.uRight ? VolMaster.uRight : 1)) / PDMAUDIO_VOLUME_MAX;
    }

    PSB16DRIVER pDrv;
    RTListForEach(&pThis->lstDrv, pDrv, SB16DRIVER, Node)
    {
        if (!pDrv->Out.pStream)
            continue;

        int rc2 = pDrv->pConnector->pfnStreamSetVolume(pDrv->pConnector, pDrv->Out.pStream, &VolCombined);
        AssertRC(rc2);
    }
}

static void sb16CmdResetLegacy(PSB16STATE pThis)
{
    LogFlowFuncEnter();

    pThis->freq       = 11025;
    pThis->fmt_signed = 0;
    pThis->fmt_bits   = 8;
    pThis->fmt_stereo = 0;

    /* At the moment we only have one stream, the output stream. */
    PPDMAUDIOSTREAMCFG pCfg = &pThis->Out.Cfg;

    pCfg->enmDir          = PDMAUDIODIR_OUT;
    pCfg->DestSource.Dest = PDMAUDIOPLAYBACKDEST_FRONT;
    pCfg->enmLayout       = PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED;

    pCfg->Props.uHz       = pThis->freq;
    pCfg->Props.cChannels = 1; /* Mono */
    pCfg->Props.cBytes    = 1 /* 8-bit */;
    pCfg->Props.fSigned   = false;
    pCfg->Props.cShift    = PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(pCfg->Props.cBytes, pCfg->Props.cChannels);

    AssertCompile(sizeof(pCfg->szName) > sizeof("Output"));
    strcpy(pCfg->szName, "Output");

    sb16CloseOut(pThis);
}

static void sb16CmdReset(PSB16STATE pThis)
{
    PDMDevHlpISASetIrq(pThis->pDevInsR3, pThis->irq, 0);
    if (pThis->dma_auto)
    {
        PDMDevHlpISASetIrq(pThis->pDevInsR3, pThis->irq, 1);
        PDMDevHlpISASetIrq(pThis->pDevInsR3, pThis->irq, 0);
    }

    pThis->mixer_regs[0x82] = 0;
    pThis->dma_auto = 0;
    pThis->in_index = 0;
    pThis->out_data_len = 0;
    pThis->left_till_irq = 0;
    pThis->needed_bytes = 0;
    pThis->block_size = -1;
    pThis->nzero = 0;
    pThis->highspeed = 0;
    pThis->v2x6 = 0;
    pThis->cmd = -1;

    dsp_out_data(pThis, 0xaa);
    sb16SpeakerControl(pThis, 0);

    sb16Control(pThis, 0);
    sb16CmdResetLegacy(pThis);
}

/**
 * @callback_method_impl{PFNIOMIOPORTOUT}
 */
static DECLCALLBACK(int) dsp_write(PPDMDEVINS pDevIns, void *opaque, RTIOPORT nport, uint32_t val, unsigned cb)
{
    RT_NOREF(pDevIns, cb);
    PSB16STATE pThis = (PSB16STATE)opaque;
    int iport = nport - pThis->port;

    LogFlowFunc(("write %#x <- %#x\n", nport, val));
    switch (iport)
    {
        case 0x06:
            switch (val)
            {
                case 0x00:
                {
                    if (pThis->v2x6 == 1)
                    {
                        if (0 && pThis->highspeed)
                        {
                            pThis->highspeed = 0;
                            PDMDevHlpISASetIrq(pThis->pDevInsR3, pThis->irq, 0);
                            sb16Control(pThis, 0);
                        }
                        else
                            sb16CmdReset(pThis);
                    }
                    pThis->v2x6 = 0;
                    break;
                }

                case 0x01:
                case 0x03:              /* FreeBSD kludge */
                    pThis->v2x6 = 1;
                    break;

                case 0xc6:
                    pThis->v2x6 = 0;    /* Prince of Persia, csp.sys, diagnose.exe */
                    break;

                case 0xb8:              /* Panic */
                    sb16CmdReset(pThis);
                    break;

                case 0x39:
                    dsp_out_data(pThis, 0x38);
                    sb16CmdReset(pThis);
                    pThis->v2x6 = 0x39;
                    break;

                default:
                    pThis->v2x6 = val;
                    break;
            }
            break;

        case 0x0c:                      /* Write data or command | write status */
#if 0
            if (pThis->highspeed)
                break;
#endif
            if (0 == pThis->needed_bytes)
            {
                sb16HandleCommand(pThis, val);
#if 0
                if (0 == pThis->needed_bytes) {
                    log_dsp (pThis);
                }
#endif
            }
            else
            {
                if (pThis->in_index == sizeof (pThis->in2_data))
                {
                    LogFlowFunc(("in data overrun\n"));
                }
                else
                {
                    pThis->in2_data[pThis->in_index++] = val;
                    if (pThis->in_index == pThis->needed_bytes)
                    {
                        pThis->needed_bytes = 0;
                        complete (pThis);
#if 0
                        log_dsp (pThis);
#endif
                    }
                }
            }
            break;

        default:
            LogFlowFunc(("nport=%#x, val=%#x)\n", nport, val));
            break;
    }

    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{PFNIOMIOPORTIN}
 */
static DECLCALLBACK(int) dsp_read(PPDMDEVINS pDevIns, void *opaque, RTIOPORT nport, uint32_t *pu32, unsigned cb)
{
    RT_NOREF(pDevIns, cb);
    PSB16STATE pThis = (PSB16STATE)opaque;
    int iport, retval, ack = 0;

    iport = nport - pThis->port;

    /** @todo reject non-byte access?
     *  The spec does not mention a non-byte access so we should check how real hardware behaves. */

    switch (iport)
    {
        case 0x06:                  /* reset */
            retval = 0xff;
            break;

        case 0x0a:                  /* read data */
            if (pThis->out_data_len)
            {
                retval = pThis->out_data[--pThis->out_data_len];
                pThis->last_read_byte = retval;
            }
            else
            {
                if (pThis->cmd != -1)
                    LogFlowFunc(("empty output buffer for command %#x\n", pThis->cmd));
                retval = pThis->last_read_byte;
                /* goto error; */
            }
            break;

        case 0x0c:                  /* 0 can write */
            retval = pThis->can_write ? 0 : 0x80;
            break;

        case 0x0d:                  /* timer interrupt clear */
            /* LogFlowFunc(("timer interrupt clear\n")); */
            retval = 0;
            break;

        case 0x0e:                  /* data available status | irq 8 ack */
            retval = (!pThis->out_data_len || pThis->highspeed) ? 0 : 0x80;
            if (pThis->mixer_regs[0x82] & 1)
            {
                ack = 1;
                pThis->mixer_regs[0x82] &= ~1;
                PDMDevHlpISASetIrq(pThis->pDevInsR3, pThis->irq, 0);
            }
            break;

        case 0x0f:                  /* irq 16 ack */
            retval = 0xff;
            if (pThis->mixer_regs[0x82] & 2)
            {
                ack = 1;
                pThis->mixer_regs[0x82] &= ~2;
               PDMDevHlpISASetIrq(pThis->pDevInsR3, pThis->irq, 0);
            }
            break;

        default:
            goto error;
    }

    if (!ack)
        LogFlowFunc(("read %#x -> %#x\n", nport, retval));

    *pu32 = retval;
    return VINF_SUCCESS;

 error:
    LogFlowFunc(("warning: dsp_read %#x error\n", nport));
    return VERR_IOM_IOPORT_UNUSED;
}

static void sb16MixerReset(PSB16STATE pThis)
{
    memset(pThis->mixer_regs, 0xff, 0x7f);
    memset(pThis->mixer_regs + 0x83, 0xff, sizeof (pThis->mixer_regs) - 0x83);

    pThis->mixer_regs[0x02] = 4;    /* master volume 3bits */
    pThis->mixer_regs[0x06] = 4;    /* MIDI volume 3bits */
    pThis->mixer_regs[0x08] = 0;    /* CD volume 3bits */
    pThis->mixer_regs[0x0a] = 0;    /* voice volume 2bits */

    /* d5=input filt, d3=lowpass filt, d1,d2=input source */
    pThis->mixer_regs[0x0c] = 0;

    /* d5=output filt, d1=stereo switch */
    pThis->mixer_regs[0x0e] = 0;

    /* voice volume L d5,d7, R d1,d3 */
    pThis->mixer_regs[0x04] = (12 << 4) | 12;
    /* master ... */
    pThis->mixer_regs[0x22] = (12 << 4) | 12;
    /* MIDI ... */
    pThis->mixer_regs[0x26] = (12 << 4) | 12;

    /* master/voice/MIDI L/R volume */
    for (int i = 0x30; i < 0x36; i++)
        pThis->mixer_regs[i] = 24 << 3; /* -14 dB */

    /* treble/bass */
    for (int i = 0x44; i < 0x48; i++)
        pThis->mixer_regs[i] = 0x80;

    /* Update the master (mixer) and PCM out volumes. */
    sb16UpdateVolume(pThis);
}

static int mixer_write_indexb(PSB16STATE pThis, uint8_t val)
{
    pThis->mixer_nreg = val;
    return VINF_SUCCESS;
}

uint32_t popcount(uint32_t u) /** @todo r=andy WTF? */
{
    u = ((u&0x55555555) + ((u>>1)&0x55555555));
    u = ((u&0x33333333) + ((u>>2)&0x33333333));
    u = ((u&0x0f0f0f0f) + ((u>>4)&0x0f0f0f0f));
    u = ((u&0x00ff00ff) + ((u>>8)&0x00ff00ff));
    u = ( u&0x0000ffff) + (u>>16);
    return u;
}

uint32_t lsbindex(uint32_t u)
{
    return popcount((u & -(int32_t)u) - 1);
}

/* Convert SB16 to SB Pro mixer volume (left). */
static inline void sb16ConvVolumeL(PSB16STATE pThis, unsigned reg, uint8_t val)
{
    /* High nibble in SBP mixer. */
    pThis->mixer_regs[reg] = (pThis->mixer_regs[reg] & 0x0f) | (val & 0xf0);
}

/* Convert SB16 to SB Pro mixer volume (right). */
static inline void sb16ConvVolumeR(PSB16STATE pThis, unsigned reg, uint8_t val)
{
    /* Low nibble in SBP mixer. */
    pThis->mixer_regs[reg] = (pThis->mixer_regs[reg] & 0xf0) | (val >> 4);
}

/* Convert SB Pro to SB16 mixer volume (left + right). */
static inline void sb16ConvVolumeOldToNew(PSB16STATE pThis, unsigned reg, uint8_t val)
{
    /* Left channel. */
    pThis->mixer_regs[reg + 0] = (val & 0xf0) | RT_BIT(3);
    /* Right channel (the register immediately following). */
    pThis->mixer_regs[reg + 1] = (val << 4)   | RT_BIT(3);
}


static int mixer_write_datab(PSB16STATE pThis, uint8_t val)
{
    bool        fUpdateMaster = false;
    bool        fUpdateStream = false;

    LogFlowFunc(("mixer_write [%#x] <- %#x\n", pThis->mixer_nreg, val));

    switch (pThis->mixer_nreg)
    {
        case 0x00:
            sb16MixerReset(pThis);
            /* And update the actual volume, too. */
            fUpdateMaster = true;
            fUpdateStream = true;
            break;

        case 0x04:  /* Translate from old style voice volume (L/R). */
            sb16ConvVolumeOldToNew(pThis, 0x32, val);
            fUpdateStream = true;
            break;

        case 0x22:  /* Translate from old style master volume (L/R). */
            sb16ConvVolumeOldToNew(pThis, 0x30, val);
            fUpdateMaster = true;
            break;

        case 0x26:  /* Translate from old style MIDI volume (L/R). */
            sb16ConvVolumeOldToNew(pThis, 0x34, val);
            break;

        case 0x28:  /* Translate from old style CD volume (L/R). */
            sb16ConvVolumeOldToNew(pThis, 0x36, val);
            break;

        case 0x2E:  /* Translate from old style line volume (L/R). */
            sb16ConvVolumeOldToNew(pThis, 0x38, val);
            break;

        case 0x30:  /* Translate to old style master volume (L). */
            sb16ConvVolumeL(pThis, 0x22, val);
            fUpdateMaster = true;
            break;

        case 0x31:  /* Translate to old style master volume (R). */
            sb16ConvVolumeR(pThis, 0x22, val);
            fUpdateMaster = true;
            break;

        case 0x32:  /* Translate to old style voice volume (L). */
            sb16ConvVolumeL(pThis, 0x04, val);
            fUpdateStream = true;
            break;

        case 0x33:  /* Translate to old style voice volume (R). */
            sb16ConvVolumeR(pThis, 0x04, val);
            fUpdateStream = true;
            break;

        case 0x34:  /* Translate to old style MIDI volume (L). */
            sb16ConvVolumeL(pThis, 0x26, val);
            break;

        case 0x35:  /* Translate to old style MIDI volume (R). */
            sb16ConvVolumeR(pThis, 0x26, val);
            break;

        case 0x36:  /* Translate to old style CD volume (L). */
            sb16ConvVolumeL(pThis, 0x28, val);
            break;

        case 0x37:  /* Translate to old style CD volume (R). */
            sb16ConvVolumeR(pThis, 0x28, val);
            break;

        case 0x38:  /* Translate to old style line volume (L). */
            sb16ConvVolumeL(pThis, 0x2E, val);
            break;

        case 0x39:  /* Translate to old style line volume (R). */
            sb16ConvVolumeR(pThis, 0x2E, val);
            break;

        case 0x80:
        {
            int irq = irq_of_magic(val);
            LogFlowFunc(("setting irq to %d (val=%#x)\n", irq, val));
            if (irq > 0)
                pThis->irq = irq;
            break;
        }

        case 0x81:
        {
            int dma, hdma;

            dma = lsbindex (val & 0xf);
            hdma = lsbindex (val & 0xf0);
            if (dma != pThis->dma || hdma != pThis->hdma)
                LogFlow(("SB16: attempt to change DMA 8bit %d(%d), 16bit %d(%d) (val=%#x)\n",
                         dma, pThis->dma, hdma, pThis->hdma, val));
#if 0
            pThis->dma = dma;
            pThis->hdma = hdma;
#endif
            break;
        }

        case 0x82:
            LogFlowFunc(("attempt to write into IRQ status register (val=%#x)\n", val));
            return VINF_SUCCESS;

        default:
            if (pThis->mixer_nreg >= 0x80)
                LogFlowFunc(("attempt to write mixer[%#x] <- %#x\n", pThis->mixer_nreg, val));
            break;
    }

    pThis->mixer_regs[pThis->mixer_nreg] = val;

    /* Update the master (mixer) volume. */
    if (   fUpdateMaster
        || fUpdateStream)
    {
        sb16UpdateVolume(pThis);
    }

    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{PFNIOMIOPORTOUT}
 */
static DECLCALLBACK(int) mixer_write(PPDMDEVINS pDevIns, void *opaque, RTIOPORT nport, uint32_t val, unsigned cb)
{
    RT_NOREF(pDevIns);
    PSB16STATE pThis = (PSB16STATE)opaque;
    int iport = nport - pThis->port;
    switch (cb)
    {
        case 1:
            switch (iport)
            {
                case 4:
                    mixer_write_indexb(pThis, val);
                    break;
                case 5:
                    mixer_write_datab(pThis, val);
                    break;
            }
            break;
        case 2:
            mixer_write_indexb(pThis, val & 0xff);
            mixer_write_datab(pThis, (val >> 8) & 0xff);
            break;
        default:
            AssertMsgFailed(("Port=%#x cb=%d u32=%#x\n", nport, cb, val));
            break;
    }
    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{PFNIOMIOPORTIN}
 */
static DECLCALLBACK(int) mixer_read(PPDMDEVINS pDevIns, void *opaque, RTIOPORT nport, uint32_t *pu32, unsigned cb)
{
    RT_NOREF(pDevIns, cb, nport);
    PSB16STATE pThis = (PSB16STATE)opaque;

#ifndef DEBUG_SB16_MOST
    if (pThis->mixer_nreg != 0x82)
        LogFlowFunc(("mixer_read[%#x] -> %#x\n", pThis->mixer_nreg, pThis->mixer_regs[pThis->mixer_nreg]));
#else
    LogFlowFunc(("mixer_read[%#x] -> %#x\n", pThis->mixer_nreg, pThis->mixer_regs[pThis->mixer_nreg]));
#endif
    *pu32 = pThis->mixer_regs[pThis->mixer_nreg];
    return VINF_SUCCESS;
}

/**
 * Called by sb16DMARead.
 */
static int sb16WriteAudio(PSB16STATE pThis, int nchan, uint32_t dma_pos, uint32_t dma_len, int len)
{
    uint8_t  tmpbuf[_4K]; /** @todo Have a buffer on the heap. */
    uint32_t cbToWrite = len;
    uint32_t cbWrittenTotal = 0;

    while (cbToWrite)
    {
        uint32_t cbToRead = RT_MIN(dma_len - dma_pos, cbToWrite);
        if (cbToRead > sizeof(tmpbuf))
            cbToRead = sizeof(tmpbuf);

        uint32_t cbRead = 0;
        int rc2 = PDMDevHlpDMAReadMemory(pThis->pDevInsR3, nchan, tmpbuf, dma_pos, cbToRead, &cbRead);
        AssertMsgRC(rc2, (" from DMA failed: %Rrc\n", rc2));

#ifdef VBOX_AUDIO_DEBUG_DUMP_PCM_DATA
        if (cbRead)
        {
            RTFILE fh;
            RTFileOpen(&fh, VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "sb16WriteAudio.pcm",
                       RTFILE_O_OPEN_CREATE | RTFILE_O_APPEND | RTFILE_O_WRITE | RTFILE_O_DENY_NONE);
            RTFileWrite(fh, tmpbuf, cbRead, NULL);
            RTFileClose(fh);
        }
#endif
        /*
         * Write data to the backends.
         */
        uint32_t cbWritten = 0;

        /* Just multiplex the output to the connected backends.
         * No need to utilize the virtual mixer here (yet). */
        PSB16DRIVER pDrv;
        RTListForEach(&pThis->lstDrv, pDrv, SB16DRIVER, Node)
        {
            if (!pDrv->Out.pStream)
                continue;

            if (!DrvAudioHlpStreamStatusCanWrite(pDrv->pConnector->pfnStreamGetStatus(pDrv->pConnector,  pDrv->Out.pStream)))
                continue;

            uint32_t cbWrittenToStream = 0;
            rc2 = pDrv->pConnector->pfnStreamWrite(pDrv->pConnector, pDrv->Out.pStream, tmpbuf, cbRead, &cbWrittenToStream);

            LogFlowFunc(("\tLUN#%RU8: rc=%Rrc, cbWrittenToStream=%RU32\n", pDrv->uLUN, rc2, cbWrittenToStream));
        }

        cbWritten = cbRead; /* Always report everything written, as the backends need to keep up themselves. */

        LogFlowFunc(("\tcbToRead=%RU32, cbToWrite=%RU32, cbWritten=%RU32, cbLeft=%RU32\n",
                     cbToRead, cbToWrite, cbWritten, cbToWrite - cbWrittenTotal));

        Assert(cbToWrite >= cbWritten);
        cbToWrite      -= cbWritten;
        dma_pos         = (dma_pos + cbWritten) % dma_len;
        cbWrittenTotal += cbWritten;

        if (!cbWritten)
            break;
    }

    return cbWrittenTotal;
}

/**
 * @callback_method_impl{FNDMATRANSFERHANDLER,
 *      Worker callback for both DMA channels.}
 */
static DECLCALLBACK(uint32_t) sb16DMARead(PPDMDEVINS pDevIns, void *opaque, unsigned nchan, uint32_t dma_pos, uint32_t dma_len)
{
    RT_NOREF(pDevIns);
    PSB16STATE pThis = (PSB16STATE)opaque;
    int till, copy, written, free;

    if (pThis->block_size <= 0)
    {
        LogFlowFunc(("invalid block size=%d nchan=%d dma_pos=%d dma_len=%d\n",
                     pThis->block_size, nchan, dma_pos, dma_len));
        return dma_pos;
    }

    if (pThis->left_till_irq < 0)
        pThis->left_till_irq = pThis->block_size;

    free = dma_len;

    copy = free;
    till = pThis->left_till_irq;

#ifdef DEBUG_SB16_MOST
    LogFlowFunc(("pos:%06d %d till:%d len:%d\n", dma_pos, free, till, dma_len));
#endif

    if (copy >= till)
    {
        if (0 == pThis->dma_auto)
        {
            copy = till;
        }
        else
        {
            if (copy >= till + pThis->block_size)
                copy = till; /* Make sure we won't skip IRQs. */
        }
    }

    written = sb16WriteAudio(pThis, nchan, dma_pos, dma_len, copy);
    dma_pos = (dma_pos + written) % dma_len;
    pThis->left_till_irq -= written;

    if (pThis->left_till_irq <= 0)
    {
        pThis->mixer_regs[0x82] |= (nchan & 4) ? 2 : 1;
        PDMDevHlpISASetIrq(pThis->pDevInsR3, pThis->irq, 1);
        if (0 == pThis->dma_auto)
        {
            sb16Control(pThis, 0);
            sb16SpeakerControl(pThis, 0);
        }
    }

    Log3Func(("pos %d/%d free %5d till %5d copy %5d written %5d block_size %5d\n",
               dma_pos, dma_len, free, pThis->left_till_irq, copy, written,
               pThis->block_size));

    while (pThis->left_till_irq <= 0)
        pThis->left_till_irq += pThis->block_size;

    return dma_pos;
}

static void sb16TimerMaybeStart(PSB16STATE pThis)
{
    LogFlowFunc(("cStreamsActive=%RU8\n", pThis->cStreamsActive));

    if (pThis->cStreamsActive == 0) /* Only start the timer if there are no active streams. */
        return;

    if (!pThis->pTimerIO)
        return;

    /* Set timer flag. */
    ASMAtomicXchgBool(&pThis->fTimerActive, true);

    /* Update current time timestamp. */
    pThis->uTimerTSIO = TMTimerGet(pThis->pTimerIO);

    /* Fire off timer. */
    TMTimerSet(pThis->pTimerIO, TMTimerGet(pThis->pTimerIO) + pThis->cTimerTicksIO);
}

static void sb16TimerMaybeStop(PSB16STATE pThis)
{
    LogFlowFunc(("cStreamsActive=%RU8\n", pThis->cStreamsActive));

    if (pThis->cStreamsActive) /* Some streams still active? Bail out. */
        return;

    if (!pThis->pTimerIO)
        return;

    /* Set timer flag. */
    ASMAtomicXchgBool(&pThis->fTimerActive, false);
}

/**
 * @callback_method_impl{FNTMTIMERDEV}
 */
static DECLCALLBACK(void) sb16TimerIO(PPDMDEVINS pDevIns, PTMTIMER pTimer, void *pvUser)
{
    RT_NOREF(pDevIns);
    PSB16STATE pThis = (PSB16STATE)pvUser;
    Assert(pThis == PDMINS_2_DATA(pDevIns, PSB16STATE));
    AssertPtr(pThis);

    uint64_t cTicksNow     = TMTimerGet(pTimer);
    bool     fIsPlaying    = false; /* Whether one or more streams are still playing. */
    bool     fDoTransfer   = false;

    pThis->uTimerTSIO = cTicksNow;

    PSB16DRIVER pDrv;
    RTListForEach(&pThis->lstDrv, pDrv, SB16DRIVER, Node)
    {
        PPDMAUDIOSTREAM pStream = pDrv->Out.pStream;
        if (!pStream)
            continue;

        PPDMIAUDIOCONNECTOR pConn = pDrv->pConnector;
        if (!pConn)
            continue;

        int rc2 = pConn->pfnStreamIterate(pConn, pStream);
        if (RT_SUCCESS(rc2))
        {
            rc2 = pConn->pfnStreamPlay(pConn, pStream, NULL /* cPlayed */);
            if (RT_FAILURE(rc2))
            {
                LogFlowFunc(("%s: Failed playing stream, rc=%Rrc\n", pStream->szName, rc2));
                continue;
            }

            /* Only do the next DMA transfer if we're able to write the remaining data block. */
            fDoTransfer = pConn->pfnStreamGetWritable(pConn, pStream) > (unsigned)pThis->left_till_irq;
        }

        PDMAUDIOSTREAMSTS strmSts = pConn->pfnStreamGetStatus(pConn, pStream);
        fIsPlaying |= (   (strmSts & PDMAUDIOSTREAMSTS_FLAG_ENABLED)
                       || (strmSts & PDMAUDIOSTREAMSTS_FLAG_PENDING_DISABLE));
    }

    bool fTimerActive = ASMAtomicReadBool(&pThis->fTimerActive);
    bool fKickTimer   = fTimerActive || fIsPlaying;

    LogFlowFunc(("fTimerActive=%RTbool, fIsPlaying=%RTbool\n", fTimerActive, fIsPlaying));

    if (fDoTransfer)
    {
        /* Schedule the next transfer. */
        PDMDevHlpDMASchedule(pThis->pDevInsR3);

        /* Kick the timer at least one more time. */
        fKickTimer = true;
    }

    /*
     * Recording.
     */
    /** @todo Implement recording. */

    if (fKickTimer)
    {
        /* Kick the timer again. */
        uint64_t cTicks = pThis->cTimerTicksIO;
        /** @todo adjust cTicks down by now much cbOutMin represents. */
        TMTimerSet(pThis->pTimerIO, cTicksNow + cTicks);
    }
}

/**
 * @callback_method_impl{FNSSMDEVLIVEEXEC}
 */
static DECLCALLBACK(int) sb16LiveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uPass)
{
    RT_NOREF(uPass);
    PSB16STATE pThis = PDMINS_2_DATA(pDevIns, PSB16STATE);

    SSMR3PutS32(pSSM, pThis->irqCfg);
    SSMR3PutS32(pSSM, pThis->dmaCfg);
    SSMR3PutS32(pSSM, pThis->hdmaCfg);
    SSMR3PutS32(pSSM, pThis->portCfg);
    SSMR3PutS32(pSSM, pThis->verCfg);
    return VINF_SSM_DONT_CALL_AGAIN;
}

/**
 * Worker for sb16SaveExec.
 */
static int sb16Save(PSSMHANDLE pSSM, PSB16STATE pThis)
{
    SSMR3PutS32(pSSM, pThis->irq);
    SSMR3PutS32(pSSM, pThis->dma);
    SSMR3PutS32(pSSM, pThis->hdma);
    SSMR3PutS32(pSSM, pThis->port);
    SSMR3PutS32(pSSM, pThis->ver);
    SSMR3PutS32(pSSM, pThis->in_index);
    SSMR3PutS32(pSSM, pThis->out_data_len);
    SSMR3PutS32(pSSM, pThis->fmt_stereo);
    SSMR3PutS32(pSSM, pThis->fmt_signed);
    SSMR3PutS32(pSSM, pThis->fmt_bits);

    SSMR3PutU32(pSSM, pThis->fmt);

    SSMR3PutS32(pSSM, pThis->dma_auto);
    SSMR3PutS32(pSSM, pThis->block_size);
    SSMR3PutS32(pSSM, pThis->fifo);
    SSMR3PutS32(pSSM, pThis->freq);
    SSMR3PutS32(pSSM, pThis->time_const);
    SSMR3PutS32(pSSM, pThis->speaker);
    SSMR3PutS32(pSSM, pThis->needed_bytes);
    SSMR3PutS32(pSSM, pThis->cmd);
    SSMR3PutS32(pSSM, pThis->use_hdma);
    SSMR3PutS32(pSSM, pThis->highspeed);
    SSMR3PutS32(pSSM, pThis->can_write);
    SSMR3PutS32(pSSM, pThis->v2x6);

    SSMR3PutU8 (pSSM, pThis->csp_param);
    SSMR3PutU8 (pSSM, pThis->csp_value);
    SSMR3PutU8 (pSSM, pThis->csp_mode);
    SSMR3PutU8 (pSSM, pThis->csp_param); /* Bug compatible! */
    SSMR3PutMem(pSSM, pThis->csp_regs, 256);
    SSMR3PutU8 (pSSM, pThis->csp_index);
    SSMR3PutMem(pSSM, pThis->csp_reg83, 4);
    SSMR3PutS32(pSSM, pThis->csp_reg83r);
    SSMR3PutS32(pSSM, pThis->csp_reg83w);

    SSMR3PutMem(pSSM, pThis->in2_data, sizeof(pThis->in2_data));
    SSMR3PutMem(pSSM, pThis->out_data, sizeof(pThis->out_data));
    SSMR3PutU8 (pSSM, pThis->test_reg);
    SSMR3PutU8 (pSSM, pThis->last_read_byte);

    SSMR3PutS32(pSSM, pThis->nzero);
    SSMR3PutS32(pSSM, pThis->left_till_irq);
    SSMR3PutS32(pSSM, pThis->dma_running);
    SSMR3PutS32(pSSM, pThis->bytes_per_second);
    SSMR3PutS32(pSSM, pThis->align);

    SSMR3PutS32(pSSM, pThis->mixer_nreg);
    return SSMR3PutMem(pSSM, pThis->mixer_regs, 256);
}

/**
 * @callback_method_impl{FNSSMDEVSAVEEXEC}
 */
static DECLCALLBACK(int) sb16SaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    PSB16STATE pThis = PDMINS_2_DATA(pDevIns, PSB16STATE);

    sb16LiveExec(pDevIns, pSSM, 0);
    return sb16Save(pSSM, pThis);
}

/**
 * Worker for sb16LoadExec.
 */
static int sb16Load(PSSMHANDLE pSSM, PSB16STATE pThis)
{
    SSMR3GetS32(pSSM, &pThis->irq);
    SSMR3GetS32(pSSM, &pThis->dma);
    SSMR3GetS32(pSSM, &pThis->hdma);
    SSMR3GetS32(pSSM, &pThis->port);
    SSMR3GetS32(pSSM, &pThis->ver);
    SSMR3GetS32(pSSM, &pThis->in_index);
    SSMR3GetS32(pSSM, &pThis->out_data_len);
    SSMR3GetS32(pSSM, &pThis->fmt_stereo);
    SSMR3GetS32(pSSM, &pThis->fmt_signed);
    SSMR3GetS32(pSSM, &pThis->fmt_bits);

    SSMR3GetU32(pSSM, (uint32_t *)&pThis->fmt);

    SSMR3GetS32(pSSM, &pThis->dma_auto);
    SSMR3GetS32(pSSM, &pThis->block_size);
    SSMR3GetS32(pSSM, &pThis->fifo);
    SSMR3GetS32(pSSM, &pThis->freq);
    SSMR3GetS32(pSSM, &pThis->time_const);
    SSMR3GetS32(pSSM, &pThis->speaker);
    SSMR3GetS32(pSSM, &pThis->needed_bytes);
    SSMR3GetS32(pSSM, &pThis->cmd);
    SSMR3GetS32(pSSM, &pThis->use_hdma);
    SSMR3GetS32(pSSM, &pThis->highspeed);
    SSMR3GetS32(pSSM, &pThis->can_write);
    SSMR3GetS32(pSSM, &pThis->v2x6);

    SSMR3GetU8 (pSSM, &pThis->csp_param);
    SSMR3GetU8 (pSSM, &pThis->csp_value);
    SSMR3GetU8 (pSSM, &pThis->csp_mode);
    SSMR3GetU8 (pSSM, &pThis->csp_param);   /* Bug compatible! */
    SSMR3GetMem(pSSM, pThis->csp_regs, 256);
    SSMR3GetU8 (pSSM, &pThis->csp_index);
    SSMR3GetMem(pSSM, pThis->csp_reg83, 4);
    SSMR3GetS32(pSSM, &pThis->csp_reg83r);
    SSMR3GetS32(pSSM, &pThis->csp_reg83w);

    SSMR3GetMem(pSSM, pThis->in2_data, sizeof(pThis->in2_data));
    SSMR3GetMem(pSSM, pThis->out_data, sizeof(pThis->out_data));
    SSMR3GetU8 (pSSM, &pThis->test_reg);
    SSMR3GetU8 (pSSM, &pThis->last_read_byte);

    SSMR3GetS32(pSSM, &pThis->nzero);
    SSMR3GetS32(pSSM, &pThis->left_till_irq);
    SSMR3GetS32(pSSM, &pThis->dma_running);
    SSMR3GetS32(pSSM, &pThis->bytes_per_second);
    SSMR3GetS32(pSSM, &pThis->align);

    int32_t mixer_nreg = 0;
    int rc = SSMR3GetS32(pSSM, &mixer_nreg);
    AssertRCReturn(rc, rc);
    pThis->mixer_nreg = (uint8_t)mixer_nreg;
    rc = SSMR3GetMem(pSSM, pThis->mixer_regs, 256);
    AssertRCReturn(rc, rc);

    if (pThis->dma_running)
    {
        sb16CheckAndReOpenOut(pThis);
        sb16Control(pThis, 1);
        sb16SpeakerControl(pThis, pThis->speaker);
    }

    /* Update the master (mixer) and PCM out volumes. */
    sb16UpdateVolume(pThis);

    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{FNSSMDEVLOADEXEC}
 */
static DECLCALLBACK(int) sb16LoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    PSB16STATE pThis = PDMINS_2_DATA(pDevIns, PSB16STATE);

    AssertMsgReturn(    uVersion == SB16_SAVE_STATE_VERSION
                    ||  uVersion == SB16_SAVE_STATE_VERSION_VBOX_30,
                    ("%u\n", uVersion),
                    VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION);
    if (uVersion > SB16_SAVE_STATE_VERSION_VBOX_30)
    {
        int32_t irq;
        SSMR3GetS32 (pSSM, &irq);
        int32_t dma;
        SSMR3GetS32 (pSSM, &dma);
        int32_t hdma;
        SSMR3GetS32 (pSSM, &hdma);
        int32_t port;
        SSMR3GetS32 (pSSM, &port);
        int32_t ver;
        int rc = SSMR3GetS32 (pSSM, &ver);
        AssertRCReturn (rc, rc);

        if (   irq  != pThis->irqCfg
            || dma  != pThis->dmaCfg
            || hdma != pThis->hdmaCfg
            || port != pThis->portCfg
            || ver  != pThis->verCfg)
        {
            return SSMR3SetCfgError(pSSM, RT_SRC_POS,
                                    N_("config changed: irq=%x/%x dma=%x/%x hdma=%x/%x port=%x/%x ver=%x/%x (saved/config)"),
                                    irq,  pThis->irqCfg,
                                    dma,  pThis->dmaCfg,
                                    hdma, pThis->hdmaCfg,
                                    port, pThis->portCfg,
                                    ver,  pThis->verCfg);
        }
    }

    if (uPass != SSM_PASS_FINAL)
        return VINF_SUCCESS;

    return sb16Load(pSSM, pThis);
}

/**
 * Creates a PDM audio stream for a specific driver.
 *
 * @returns IPRT status code.
 * @param   pThis               SB16 state.
 * @param   pCfg                Stream configuration to use.
 * @param   pDrv                Driver stream to create PDM stream for.
 */
static int sb16CreateDrvStream(PSB16STATE pThis, PPDMAUDIOSTREAMCFG pCfg, PSB16DRIVER pDrv)
{
    RT_NOREF(pThis);

    AssertReturn(pCfg->enmDir == PDMAUDIODIR_OUT, VERR_INVALID_PARAMETER);
    Assert(DrvAudioHlpStreamCfgIsValid(pCfg));

    PPDMAUDIOSTREAMCFG pCfgHost = DrvAudioHlpStreamCfgDup(pCfg);
    if (!pCfgHost)
        return VERR_NO_MEMORY;

    LogFunc(("[LUN#%RU8] %s\n", pDrv->uLUN, pCfgHost->szName));

    AssertMsg(pDrv->Out.pStream == NULL, ("[LUN#%RU8] Driver stream already present when it must not\n", pDrv->uLUN));

    /* Disable pre-buffering for SB16; not needed for that bit of data. */
    pCfgHost->Backend.cfPreBuf = 0;

    int rc = pDrv->pConnector->pfnStreamCreate(pDrv->pConnector, pCfgHost, pCfg /* pCfgGuest */, &pDrv->Out.pStream);
    if (RT_SUCCESS(rc))
    {
        pDrv->pConnector->pfnStreamRetain(pDrv->pConnector, pDrv->Out.pStream);
        LogFlowFunc(("LUN#%RU8: Created output \"%s\", rc=%Rrc\n", pDrv->uLUN, pCfg->szName, rc));
    }

    DrvAudioHlpStreamCfgFree(pCfgHost);

    return rc;
}

/**
 * Destroys a PDM audio stream of a specific driver.
 *
 * @param   pThis               SB16 state.
 * @param   pDrv                Driver stream to destroy PDM stream for.
 */
static void sb16DestroyDrvStream(PSB16STATE pThis, PSB16DRIVER pDrv)
{
    AssertPtrReturnVoid(pThis);
    AssertPtrReturnVoid(pDrv);

    if (pDrv->Out.pStream)
    {
        pDrv->pConnector->pfnStreamRelease(pDrv->pConnector, pDrv->Out.pStream);

        int rc2 = pDrv->pConnector->pfnStreamControl(pDrv->pConnector, pDrv->Out.pStream, PDMAUDIOSTREAMCMD_DISABLE);
        AssertRC(rc2);

        rc2 = pDrv->pConnector->pfnStreamDestroy(pDrv->pConnector, pDrv->Out.pStream);
        AssertRC(rc2);

        pDrv->Out.pStream = NULL;
    }
}

/**
 * Checks if the output stream needs to be (re-)created and does so if needed.
 *
 * @return VBox status code.
 * @param  pThis                SB16 state.
 */
static int sb16CheckAndReOpenOut(PSB16STATE pThis)
{
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;

    if (pThis->freq > 0)
    {
        /* At the moment we only have one stream, the output stream. */
        PDMAUDIOSTREAMCFG Cfg;
        RT_ZERO(Cfg);

        Cfg.Props.uHz       = pThis->freq;
        Cfg.Props.cChannels = 1 << pThis->fmt_stereo;
        Cfg.Props.cBytes    = pThis->fmt_bits / 8;
        Cfg.Props.fSigned   = RT_BOOL(pThis->fmt_signed);
        Cfg.Props.cShift    = PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(Cfg.Props.cBytes, Cfg.Props.cChannels);

        if (!DrvAudioHlpPCMPropsAreEqual(&Cfg.Props, &pThis->Out.Cfg.Props))
        {
            Cfg.enmDir          = PDMAUDIODIR_OUT;
            Cfg.DestSource.Dest = PDMAUDIOPLAYBACKDEST_FRONT;
            Cfg.enmLayout       = PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED;

            strcpy(Cfg.szName, "Output");

            sb16CloseOut(pThis);

            rc = sb16OpenOut(pThis, &Cfg);
            AssertRC(rc);
        }
    }
    else
        sb16CloseOut(pThis);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

static int sb16OpenOut(PSB16STATE pThis, PPDMAUDIOSTREAMCFG pCfg)
{
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfg,  VERR_INVALID_POINTER);

    LogFlowFuncEnter();

    if (!DrvAudioHlpStreamCfgIsValid(pCfg))
        return VERR_INVALID_PARAMETER;

    int rc = DrvAudioHlpStreamCfgCopy(&pThis->Out.Cfg, pCfg);
    if (RT_FAILURE(rc))
        return rc;

    /* Set scheduling hint (if available). */
    if (pThis->cTimerTicksIO)
        pThis->Out.Cfg.Device.uSchedulingHintMs = 1000 /* ms */ / (TMTimerGetFreq(pThis->pTimerIO) / pThis->cTimerTicksIO);

    PSB16DRIVER pDrv;
    RTListForEach(&pThis->lstDrv, pDrv, SB16DRIVER, Node)
    {
        int rc2 = sb16CreateDrvStream(pThis, &pThis->Out.Cfg, pDrv);
        if (RT_FAILURE(rc2))
            LogFunc(("Attaching stream failed with %Rrc\n", rc2));

        /* Do not pass failure to rc here, as there might be drivers which aren't
         * configured / ready yet. */
    }

    sb16UpdateVolume(pThis);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

static void sb16CloseOut(PSB16STATE pThis)
{
    AssertPtrReturnVoid(pThis);

    LogFlowFuncEnter();

    PSB16DRIVER pDrv;
    RTListForEach(&pThis->lstDrv, pDrv, SB16DRIVER, Node)
        sb16DestroyDrvStream(pThis, pDrv);

    LogFlowFuncLeave();
}


/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) sb16QueryInterface(struct PDMIBASE *pInterface, const char *pszIID)
{
    PSB16STATE pThis = RT_FROM_MEMBER(pInterface, SB16STATE, IBase);
    Assert(&pThis->IBase == pInterface);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pThis->IBase);
    return NULL;
}


/**
 * Attach command, internal version.
 *
 * This is called to let the device attach to a driver for a specified LUN
 * during runtime. This is not called during VM construction, the device
 * constructor has to attach to all the available drivers.
 *
 * @returns VBox status code.
 * @param   pThis       SB16 state.
 * @param   uLUN        The logical unit which is being detached.
 * @param   fFlags      Flags, combination of the PDMDEVATT_FLAGS_* \#defines.
 * @param   ppDrv       Attached driver instance on success. Optional.
 */
static int sb16AttachInternal(PSB16STATE pThis, unsigned uLUN, uint32_t fFlags, PSB16DRIVER *ppDrv)
{
    RT_NOREF(fFlags);

    /*
     * Attach driver.
     */
    char *pszDesc;
    if (RTStrAPrintf(&pszDesc, "Audio driver port (SB16) for LUN #%u", uLUN) <= 0)
        AssertLogRelFailedReturn(VERR_NO_MEMORY);

    PPDMIBASE pDrvBase;
    int rc = PDMDevHlpDriverAttach(pThis->pDevInsR3, uLUN,
                                   &pThis->IBase, &pDrvBase, pszDesc);
    if (RT_SUCCESS(rc))
    {
        PSB16DRIVER pDrv = (PSB16DRIVER)RTMemAllocZ(sizeof(SB16DRIVER));
        if (pDrv)
        {
            pDrv->pDrvBase   = pDrvBase;
            pDrv->pConnector = PDMIBASE_QUERY_INTERFACE(pDrvBase, PDMIAUDIOCONNECTOR);
            AssertMsg(pDrv->pConnector != NULL, ("Configuration error: LUN #%u has no host audio interface, rc=%Rrc\n", uLUN, rc));
            pDrv->pSB16State = pThis;
            pDrv->uLUN       = uLUN;

            /*
             * For now we always set the driver at LUN 0 as our primary
             * host backend. This might change in the future.
             */
            if (pDrv->uLUN == 0)
                pDrv->fFlags |= PDMAUDIODRVFLAGS_PRIMARY;

            LogFunc(("LUN#%RU8: pCon=%p, drvFlags=0x%x\n", uLUN, pDrv->pConnector, pDrv->fFlags));

            /* Attach to driver list if not attached yet. */
            if (!pDrv->fAttached)
            {
                RTListAppend(&pThis->lstDrv, &pDrv->Node);
                pDrv->fAttached = true;
            }

            if (ppDrv)
                *ppDrv = pDrv;
        }
        else
            rc = VERR_NO_MEMORY;
    }
    else if (rc == VERR_PDM_NO_ATTACHED_DRIVER)
        LogFunc(("No attached driver for LUN #%u\n", uLUN));

    if (RT_FAILURE(rc))
    {
        /* Only free this string on failure;
         * must remain valid for the live of the driver instance. */
        RTStrFree(pszDesc);
    }

    LogFunc(("iLUN=%u, fFlags=0x%x, rc=%Rrc\n", uLUN, fFlags, rc));
    return rc;
}

/**
 * Detach command, internal version.
 *
 * This is called to let the device detach from a driver for a specified LUN
 * during runtime.
 *
 * @returns VBox status code.
 * @param   pThis       SB16 state.
 * @param   pDrv        Driver to detach device from.
 * @param   fFlags      Flags, combination of the PDMDEVATT_FLAGS_* \#defines.
 */
static int sb16DetachInternal(PSB16STATE pThis, PSB16DRIVER pDrv, uint32_t fFlags)
{
    RT_NOREF(fFlags);

    sb16DestroyDrvStream(pThis, pDrv);

    RTListNodeRemove(&pDrv->Node);

    LogFunc(("uLUN=%u, fFlags=0x%x\n", pDrv->uLUN, fFlags));
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMDEVREG,pfnAttach}
 */
static DECLCALLBACK(int) sb16Attach(PPDMDEVINS pDevIns, unsigned uLUN, uint32_t fFlags)
{
    PSB16STATE pThis = PDMINS_2_DATA(pDevIns, PSB16STATE);

    LogFunc(("uLUN=%u, fFlags=0x%x\n", uLUN, fFlags));

    PSB16DRIVER pDrv;
    int rc2 = sb16AttachInternal(pThis, uLUN, fFlags, &pDrv);
    if (RT_SUCCESS(rc2))
        rc2 = sb16CreateDrvStream(pThis, &pThis->Out.Cfg, pDrv);

    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMDEVREG,pfnDetach}
 */
static DECLCALLBACK(void) sb16Detach(PPDMDEVINS pDevIns, unsigned uLUN, uint32_t fFlags)
{
    PSB16STATE pThis = PDMINS_2_DATA(pDevIns, PSB16STATE);

    LogFunc(("uLUN=%u, fFlags=0x%x\n", uLUN, fFlags));

    PSB16DRIVER pDrv, pDrvNext;
    RTListForEachSafe(&pThis->lstDrv, pDrv, pDrvNext, SB16DRIVER, Node)
    {
        if (pDrv->uLUN == uLUN)
        {
            int rc2 = sb16DetachInternal(pThis, pDrv, fFlags);
            if (RT_SUCCESS(rc2))
            {
                RTMemFree(pDrv);
                pDrv = NULL;
            }

            break;
        }
    }
}

/**
 * Re-attaches (replaces) a driver with a new driver.
 *
 * @returns VBox status code.
 * @param   pThis       Device instance.
 * @param   pDrv        Driver instance used for attaching to.
 *                      If NULL is specified, a new driver will be created and appended
 *                      to the driver list.
 * @param   uLUN        The logical unit which is being re-detached.
 * @param   pszDriver   New driver name to attach.
 */
static int sb16Reattach(PSB16STATE pThis, PSB16DRIVER pDrv, uint8_t uLUN, const char *pszDriver)
{
    AssertPtrReturn(pThis,     VERR_INVALID_POINTER);
    AssertPtrReturn(pszDriver, VERR_INVALID_POINTER);

    int rc;

    if (pDrv)
    {
        rc = sb16DetachInternal(pThis, pDrv, 0 /* fFlags */);
        if (RT_SUCCESS(rc))
            rc = PDMDevHlpDriverDetach(pThis->pDevInsR3, PDMIBASE_2_PDMDRV(pDrv->pDrvBase), 0 /* fFlags */);

        if (RT_FAILURE(rc))
            return rc;

        pDrv = NULL;
    }

    PVM pVM = PDMDevHlpGetVM(pThis->pDevInsR3);
    PCFGMNODE pRoot = CFGMR3GetRoot(pVM);
    PCFGMNODE pDev0 = CFGMR3GetChild(pRoot, "Devices/sb16/0/");

    /* Remove LUN branch. */
    CFGMR3RemoveNode(CFGMR3GetChildF(pDev0, "LUN#%u/", uLUN));

    if (pDrv)
    {
        /* Re-use the driver instance so detach it before. */
        rc = PDMDevHlpDriverDetach(pThis->pDevInsR3, PDMIBASE_2_PDMDRV(pDrv->pDrvBase), 0 /* fFlags */);
        if (RT_FAILURE(rc))
            return rc;
    }

#define RC_CHECK() if (RT_FAILURE(rc)) { AssertReleaseRC(rc); break; }

    do
    {
        PCFGMNODE pLunL0;
        rc = CFGMR3InsertNodeF(pDev0, &pLunL0, "LUN#%u/", uLUN);        RC_CHECK();
        rc = CFGMR3InsertString(pLunL0, "Driver",       "AUDIO");       RC_CHECK();
        rc = CFGMR3InsertNode(pLunL0,   "Config/",       NULL);         RC_CHECK();

        PCFGMNODE pLunL1, pLunL2;
        rc = CFGMR3InsertNode  (pLunL0, "AttachedDriver/", &pLunL1);    RC_CHECK();
        rc = CFGMR3InsertNode  (pLunL1,  "Config/",        &pLunL2);    RC_CHECK();
        rc = CFGMR3InsertString(pLunL1,  "Driver",          pszDriver); RC_CHECK();

        rc = CFGMR3InsertString(pLunL2, "AudioDriver", pszDriver);      RC_CHECK();

    } while (0);

    if (RT_SUCCESS(rc))
        rc = sb16AttachInternal(pThis, uLUN, 0 /* fFlags */, NULL /* ppDrv */);

    LogFunc(("pThis=%p, uLUN=%u, pszDriver=%s, rc=%Rrc\n", pThis, uLUN, pszDriver, rc));

#undef RC_CHECK

    return rc;
}

/**
 * @interface_method_impl{PDMDEVREG,pfnReset}
 */
static DECLCALLBACK(void) sb16DevReset(PPDMDEVINS pDevIns)
{
    PSB16STATE pThis = PDMINS_2_DATA(pDevIns, PSB16STATE);

    /* Bring back the device to initial state, and especially make
     * sure there's no interrupt or DMA activity.
     */
    PDMDevHlpISASetIrq(pThis->pDevInsR3, pThis->irq, 0);

    pThis->mixer_regs[0x82] = 0;
    pThis->csp_regs[5]      = 1;
    pThis->csp_regs[9]      = 0xf8;

    pThis->dma_auto = 0;
    pThis->in_index = 0;
    pThis->out_data_len = 0;
    pThis->left_till_irq = 0;
    pThis->needed_bytes = 0;
    pThis->block_size = -1;
    pThis->nzero = 0;
    pThis->highspeed = 0;
    pThis->v2x6 = 0;
    pThis->cmd = -1;

    sb16MixerReset(pThis);
    sb16SpeakerControl(pThis, 0);
    sb16Control(pThis, 0);
    sb16CmdResetLegacy(pThis);
}

/**
 * Powers off the device.
 *
 * @param   pDevIns             Device instance to power off.
 */
static DECLCALLBACK(void) sb16PowerOff(PPDMDEVINS pDevIns)
{
    PSB16STATE pThis = PDMINS_2_DATA(pDevIns, PSB16STATE);

    LogRel2(("SB16: Powering off ...\n"));

    PSB16DRIVER pDrv;
    RTListForEach(&pThis->lstDrv, pDrv, SB16DRIVER, Node)
        sb16DestroyDrvStream(pThis, pDrv);
}

/**
 * @interface_method_impl{PDMDEVREG,pfnDestruct}
 */
static DECLCALLBACK(int) sb16Destruct(PPDMDEVINS pDevIns)
{
    PDMDEV_CHECK_VERSIONS_RETURN_QUIET(pDevIns); /* this shall come first */
    PSB16STATE pThis = PDMINS_2_DATA(pDevIns, PSB16STATE);

    LogFlowFuncEnter();

    PSB16DRIVER pDrv;
    while (!RTListIsEmpty(&pThis->lstDrv))
    {
        pDrv = RTListGetFirst(&pThis->lstDrv, SB16DRIVER, Node);

        RTListNodeRemove(&pDrv->Node);
        RTMemFree(pDrv);
    }

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) sb16Construct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    RT_NOREF(iInstance);
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns); /* this shall come first */
    PSB16STATE pThis = PDMINS_2_DATA(pDevIns, PSB16STATE);

    /*
     * Initialize the data so sb16Destruct runs without a hitch if we return early.
     */
    pThis->pDevInsR3               = pDevIns;
    pThis->IBase.pfnQueryInterface = sb16QueryInterface;
    pThis->cmd                     = -1;

    pThis->csp_regs[5]             = 1;
    pThis->csp_regs[9]             = 0xf8;

    RTListInit(&pThis->lstDrv);

    /*
     * Validations.
     */
    Assert(iInstance == 0);
    if (!CFGMR3AreValuesValid(pCfg,
                              "IRQ\0"
                              "DMA\0"
                              "DMA16\0"
                              "Port\0"
                              "Version\0"
                              "TimerHz\0"))
        return PDMDEV_SET_ERROR(pDevIns, VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES,
                                N_("Invalid configuration for SB16 device"));

    /*
     * Read config data.
     */
    int rc = CFGMR3QuerySIntDef(pCfg, "IRQ", &pThis->irq, 5);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("SB16 configuration error: Failed to get the \"IRQ\" value"));
    pThis->irqCfg  = pThis->irq;

    rc = CFGMR3QuerySIntDef(pCfg, "DMA", &pThis->dma, 1);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("SB16 configuration error: Failed to get the \"DMA\" value"));
    pThis->dmaCfg  = pThis->dma;

    rc = CFGMR3QuerySIntDef(pCfg, "DMA16", &pThis->hdma, 5);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("SB16 configuration error: Failed to get the \"DMA16\" value"));
    pThis->hdmaCfg = pThis->hdma;

    RTIOPORT Port;
    rc = CFGMR3QueryPortDef(pCfg, "Port", &Port, 0x220);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("SB16 configuration error: Failed to get the \"Port\" value"));
    pThis->port    = Port;
    pThis->portCfg = Port;

    uint16_t u16Version;
    rc = CFGMR3QueryU16Def(pCfg, "Version", &u16Version, 0x0405);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("SB16 configuration error: Failed to get the \"Version\" value"));
    pThis->ver     = u16Version;
    pThis->verCfg  = u16Version;

    uint16_t uTimerHz;
    rc = CFGMR3QueryU16Def(pCfg, "TimerHz", &uTimerHz, 100 /* Hz */);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("SB16 configuration error: failed to read Hertz (Hz) rate as unsigned integer"));
    /*
     * Setup the mixer now that we've got the irq and dma channel numbers.
     */
    pThis->mixer_regs[0x80] = magic_of_irq(pThis->irq);
    pThis->mixer_regs[0x81] = (1 << pThis->dma) | (1 << pThis->hdma);
    pThis->mixer_regs[0x82] = 2 << 5;

    sb16MixerReset(pThis);

    /*
     * Create timer(s), register & attach stuff.
     */
    rc = PDMDevHlpTMTimerCreate(pDevIns, TMCLOCK_VIRTUAL, sb16TimerIRQ, pThis,
                                TMTIMER_FLAGS_DEFAULT_CRIT_SECT, "SB16 IRQ timer", &pThis->pTimerIRQ);
    if (RT_FAILURE(rc))
        AssertMsgFailedReturn(("Error creating IRQ timer, rc=%Rrc\n", rc), rc);

    rc = PDMDevHlpIOPortRegister(pDevIns, pThis->port + 0x04,  2, pThis, mixer_write, mixer_read, NULL, NULL, "SB16");
    if (RT_FAILURE(rc))
        return rc;
    rc = PDMDevHlpIOPortRegister(pDevIns, pThis->port + 0x06, 10, pThis, dsp_write,   dsp_read,   NULL, NULL, "SB16");
    if (RT_FAILURE(rc))
        return rc;

    rc = PDMDevHlpDMARegister(pDevIns, pThis->hdma, sb16DMARead, pThis);
    if (RT_FAILURE(rc))
        return rc;
    rc = PDMDevHlpDMARegister(pDevIns, pThis->dma, sb16DMARead, pThis);
    if (RT_FAILURE(rc))
        return rc;

    pThis->can_write = 1;

    rc = PDMDevHlpSSMRegister3(pDevIns, SB16_SAVE_STATE_VERSION, sizeof(SB16STATE), sb16LiveExec, sb16SaveExec, sb16LoadExec);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Attach driver.
     */
    uint8_t uLUN;
    for (uLUN = 0; uLUN < UINT8_MAX; ++uLUN)
    {
        LogFunc(("Trying to attach driver for LUN #%RU8 ...\n", uLUN));
        rc = sb16AttachInternal(pThis, uLUN, 0 /* fFlags */, NULL /* ppDrv */);
        if (RT_FAILURE(rc))
        {
            if (rc == VERR_PDM_NO_ATTACHED_DRIVER)
                rc = VINF_SUCCESS;
            else if (rc == VERR_AUDIO_BACKEND_INIT_FAILED)
            {
                sb16Reattach(pThis, NULL /* pDrv */, uLUN, "NullAudio");
                PDMDevHlpVMSetRuntimeError(pDevIns, 0 /*fFlags*/, "HostAudioNotResponding",
                        N_("Host audio backend initialization has failed. Selecting the NULL audio backend "
                           "with the consequence that no sound is audible"));
                /* Attaching to the NULL audio backend will never fail. */
                rc = VINF_SUCCESS;
            }
            break;
        }
    }

    LogFunc(("cLUNs=%RU8, rc=%Rrc\n", uLUN, rc));

    sb16CmdResetLegacy(pThis);

#ifdef VBOX_WITH_AUDIO_SB16_ONETIME_INIT
    PSB16DRIVER pDrv;
    RTListForEach(&pThis->lstDrv, pDrv, SB16DRIVER, Node)
    {
        /*
         * Only primary drivers are critical for the VM to run. Everything else
         * might not worth showing an own error message box in the GUI.
         */
        if (!(pDrv->fFlags & PDMAUDIODRVFLAGS_PRIMARY))
            continue;

        PPDMIAUDIOCONNECTOR pCon = pDrv->pConnector;
        AssertPtr(pCon);

        /** @todo No input streams available for SB16 yet. */

        if (!pDrv->Out.pStream)
            continue;

        bool fValidOut = pCon->pfnStreamGetStatus(pCon, pDrv->Out.pStream) & PDMAUDIOSTREAMSTS_FLAG_INITIALIZED;
        if (!fValidOut)
        {
            LogRel(("SB16: Falling back to NULL backend (no sound audible)\n"));

            sb16CmdResetLegacy(pThis);
            sb16Reattach(pThis, pDrv, pDrv->uLUN, "NullAudio");

            PDMDevHlpVMSetRuntimeError(pDevIns, 0 /*fFlags*/, "HostAudioNotResponding",
                N_("No audio devices could be opened. Selecting the NULL audio backend "
                   "with the consequence that no sound is audible"));
        }
    }
#endif

    if (RT_SUCCESS(rc))
    {
        rc = PDMDevHlpTMTimerCreate(pDevIns, TMCLOCK_VIRTUAL, sb16TimerIO, pThis,
                                    TMTIMER_FLAGS_DEFAULT_CRIT_SECT, "SB16 IO timer", &pThis->pTimerIO);
        if (RT_SUCCESS(rc))
        {
            pThis->cTimerTicksIO = TMTimerGetFreq(pThis->pTimerIO) / uTimerHz;
            pThis->uTimerTSIO    = TMTimerGet(pThis->pTimerIO);
            LogFunc(("Timer ticks=%RU64 (%RU16 Hz)\n", pThis->cTimerTicksIO, uTimerHz));
        }
        else
            AssertMsgFailedReturn(("Error creating I/O timer, rc=%Rrc\n", rc), rc);
    }

#ifdef VBOX_AUDIO_DEBUG_DUMP_PCM_DATA
    RTFileDelete(VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "sb16WriteAudio.pcm");
#endif

    return VINF_SUCCESS;
}

const PDMDEVREG g_DeviceSB16 =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "sb16",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "Sound Blaster 16 Controller",
    /* fFlags */
    PDM_DEVREG_FLAGS_DEFAULT_BITS,
    /* fClass */
    PDM_DEVREG_CLASS_AUDIO,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(SB16STATE),
    /* pfnConstruct */
    sb16Construct,
    /* pfnDestruct */
    sb16Destruct,
    /* pfnRelocate */
    NULL,
    /* pfnMemSetup */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    sb16DevReset,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    sb16Attach,
    /* pfnDetach */
    sb16Detach,
    /* pfnQueryInterface */
    NULL,
    /* pfnInitComplete */
    NULL,
    /* pfnPowerOff */
    sb16PowerOff,
    /* pfnSoftReset */
    NULL,
    /* u32VersionEnd */
    PDM_DEVREG_VERSION
};

