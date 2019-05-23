/* $Id: DevSerial.cpp $ */
/** @file
 * DevSerial - 16550A UART emulation.
 * (taken from hw/serial.c 2010/05/15 with modifications)
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/*
 * This code is based on:
 *
 * QEMU 16550A UART emulation
 *
 * Copyright (c) 2003-2004 Fabrice Bellard
 * Copyright (c) 2008 Citrix Systems, Inc.
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
#define LOG_GROUP LOG_GROUP_DEV_SERIAL
#include <VBox/vmm/pdmdev.h>
#include <iprt/assert.h>
#include <iprt/uuid.h>
#include <iprt/string.h>
#include <iprt/semaphore.h>
#include <iprt/critsect.h>

#include "VBoxDD.h"

#undef VBOX_SERIAL_PCI /* The PCI variant has lots of problems: wrong IRQ line and wrong IO base assigned. */

#ifdef VBOX_SERIAL_PCI
# include <VBox/pci.h>
#endif /* VBOX_SERIAL_PCI */


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#define SERIAL_SAVED_STATE_VERSION_16450        3
#define SERIAL_SAVED_STATE_VERSION_MISSING_BITS 4
#define SERIAL_SAVED_STATE_VERSION              5

#define UART_LCR_DLAB       0x80        /* Divisor latch access bit */

#define UART_IER_MSI        0x08        /* Enable Modem status interrupt */
#define UART_IER_RLSI       0x04        /* Enable receiver line status interrupt */
#define UART_IER_THRI       0x02        /* Enable Transmitter holding register int. */
#define UART_IER_RDI        0x01        /* Enable receiver data interrupt */

#define UART_IIR_NO_INT     0x01        /* No interrupts pending */
#define UART_IIR_ID         0x06        /* Mask for the interrupt ID */

#define UART_IIR_MSI        0x00        /* Modem status interrupt */
#define UART_IIR_THRI       0x02        /* Transmitter holding register empty */
#define UART_IIR_RDI        0x04        /* Receiver data interrupt */
#define UART_IIR_RLSI       0x06        /* Receiver line status interrupt */
#define UART_IIR_CTI        0x0C        /* Character Timeout Indication */

#define UART_IIR_FENF       0x80        /* Fifo enabled, but not functioning */
#define UART_IIR_FE         0xC0        /* Fifo enabled */

/*
 * These are the definitions for the Modem Control Register
 */
#define UART_MCR_LOOP       0x10        /* Enable loopback test mode */
#define UART_MCR_OUT2       0x08        /* Out2 complement */
#define UART_MCR_OUT1       0x04        /* Out1 complement */
#define UART_MCR_RTS        0x02        /* RTS complement */
#define UART_MCR_DTR        0x01        /* DTR complement */

/*
 * These are the definitions for the Modem Status Register
 */
#define UART_MSR_DCD        0x80        /* Data Carrier Detect */
#define UART_MSR_RI         0x40        /* Ring Indicator */
#define UART_MSR_DSR        0x20        /* Data Set Ready */
#define UART_MSR_CTS        0x10        /* Clear to Send */
#define UART_MSR_DDCD       0x08        /* Delta DCD */
#define UART_MSR_TERI       0x04        /* Trailing edge ring indicator */
#define UART_MSR_DDSR       0x02        /* Delta DSR */
#define UART_MSR_DCTS       0x01        /* Delta CTS */
#define UART_MSR_ANY_DELTA  0x0F        /* Any of the delta bits! */

#define UART_LSR_TEMT       0x40        /* Transmitter empty */
#define UART_LSR_THRE       0x20        /* Transmit-hold-register empty */
#define UART_LSR_BI         0x10        /* Break interrupt indicator */
#define UART_LSR_FE         0x08        /* Frame error indicator */
#define UART_LSR_PE         0x04        /* Parity error indicator */
#define UART_LSR_OE         0x02        /* Overrun error indicator */
#define UART_LSR_DR         0x01        /* Receiver data ready */
#define UART_LSR_INT_ANY    0x1E        /* Any of the lsr-interrupt-triggering status bits */

/*
 * Interrupt trigger levels.
 * The byte-counts are for 16550A - in newer UARTs the byte-count for each ITL is higher.
 */
#define UART_FCR_ITL_1      0x00        /* 1 byte ITL */
#define UART_FCR_ITL_2      0x40        /* 4 bytes ITL */
#define UART_FCR_ITL_3      0x80        /* 8 bytes ITL */
#define UART_FCR_ITL_4      0xC0        /* 14 bytes ITL */

#define UART_FCR_DMS        0x08        /* DMA Mode Select */
#define UART_FCR_XFR        0x04        /* XMIT Fifo Reset */
#define UART_FCR_RFR        0x02        /* RCVR Fifo Reset */
#define UART_FCR_FE         0x01        /* FIFO Enable */

#define UART_FIFO_LENGTH    16          /* 16550A Fifo Length */

#define XMIT_FIFO           0
#define RECV_FIFO           1
#define MIN_XMIT_RETRY      16
#define MAX_XMIT_RETRY_TIME 1           /* max time (in seconds) for retrying the character xmit before dropping it */


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

struct SerialFifo
{
    uint8_t data[UART_FIFO_LENGTH];
    uint8_t count;
    uint8_t itl;
    uint8_t tail;
    uint8_t head;
};

/**
 * Serial device.
 *
 * @implements  PDMIBASE
 * @implements  PDMICHARPORT
 */
typedef struct SerialState
{
    /** Access critical section. */
    PDMCRITSECT                     CritSect;
    /** Pointer to the device instance - R3 Ptr. */
    PPDMDEVINSR3                    pDevInsR3;
    /** Pointer to the device instance - R0 Ptr. */
    PPDMDEVINSR0                    pDevInsR0;
    /** Pointer to the device instance - RC Ptr. */
    PPDMDEVINSRC                    pDevInsRC;
    /** Alignment. */
    RTRCPTR                         Alignment0;
    /** LUN\#0: The base interface. */
    PDMIBASE                        IBase;
    /** LUN\#0: The character port interface. */
    PDMICHARPORT                    ICharPort;
    /** Pointer to the attached base driver. */
    R3PTRTYPE(PPDMIBASE)            pDrvBase;
    /** Pointer to the attached character driver. */
    R3PTRTYPE(PPDMICHARCONNECTOR)   pDrvChar;

    RTSEMEVENT                      ReceiveSem;
    PTMTIMERR3                      fifo_timeout_timer;
    PTMTIMERR3                      transmit_timerR3;
    PTMTIMERR0                      transmit_timerR0; /* currently not used */
    PTMTIMERRC                      transmit_timerRC; /* currently not used */
    RTRCPTR                         Alignment1;
    SerialFifo                      recv_fifo;
    SerialFifo                      xmit_fifo;

    uint32_t                        base;
    uint16_t                        divider;
    uint16_t                        Alignment2[1];
    uint8_t                         rbr; /**< receive register */
    uint8_t                         thr; /**< transmit holding register */
    uint8_t                         tsr; /**< transmit shift register */
    uint8_t                         ier; /**< interrupt enable register */
    uint8_t                         iir; /**< interrupt identification register, R/O */
    uint8_t                         lcr; /**< line control register */
    uint8_t                         mcr; /**< modem control register */
    uint8_t                         lsr; /**< line status register, R/O */
    uint8_t                         msr; /**< modem status register, R/O */
    uint8_t                         scr; /**< scratch register */
    uint8_t                         fcr; /**< fifo control register */
    uint8_t                         fcr_vmstate;
    /* NOTE: this hidden state is necessary for tx irq generation as
       it can be reset while reading iir */
    int                             thr_ipending;
    int                             timeout_ipending;
    int                             irq;
    int                             last_break_enable;
    /** Counter for retrying xmit */
    int                             tsr_retry;
    int                             tsr_retry_bound; /**< number of retries before dropping a character */
    int                             tsr_retry_bound_max; /**< maximum possible tsr_retry_bound value that can be set while dynamic bound adjustment */
    int                             tsr_retry_bound_min; /**< minimum possible tsr_retry_bound value that can be set while dynamic bound adjustment */
    bool                            msr_changed;
    bool                            fGCEnabled;
    bool                            fR0Enabled;
    bool                            fYieldOnLSRRead;
    bool volatile                   fRecvWaiting;
    bool                            f16550AEnabled;
    bool                            Alignment3[6];
    /** Time it takes to transmit a character */
    uint64_t                        char_transmit_time;

#ifdef VBOX_SERIAL_PCI
    PDMPCIDEV                       PciDev;
#endif /* VBOX_SERIAL_PCI */
} DEVSERIAL;
/** Pointer to the serial device state. */
typedef DEVSERIAL *PDEVSERIAL;

#ifndef VBOX_DEVICE_STRUCT_TESTCASE

#ifdef IN_RING3

static int serial_can_receive(PDEVSERIAL pThis);
static void serial_receive(PDEVSERIAL pThis, const uint8_t *buf, int size);

static void fifo_clear(PDEVSERIAL pThis, int fifo)
{
    SerialFifo *f = (fifo) ? &pThis->recv_fifo : &pThis->xmit_fifo;
    memset(f->data, 0, UART_FIFO_LENGTH);
    f->count = 0;
    f->head = 0;
    f->tail = 0;
}

static int fifo_put(PDEVSERIAL pThis, int fifo, uint8_t chr)
{
    SerialFifo *f = (fifo) ? &pThis->recv_fifo : &pThis->xmit_fifo;

    /* Receive overruns do not overwrite FIFO contents. */
    if (fifo == XMIT_FIFO || f->count < UART_FIFO_LENGTH)
    {
        f->data[f->head++] = chr;
        if (f->head == UART_FIFO_LENGTH)
            f->head = 0;
    }

    if (f->count < UART_FIFO_LENGTH)
        f->count++;
    else if (fifo == XMIT_FIFO) /* need to at least adjust tail to maintain pipe state consistency */
        ++f->tail;
    else if (fifo == RECV_FIFO)
        pThis->lsr |= UART_LSR_OE;

    return 1;
}

static uint8_t fifo_get(PDEVSERIAL pThis, int fifo)
{
    SerialFifo *f = (fifo) ? &pThis->recv_fifo : &pThis->xmit_fifo;
    uint8_t c;

    if (f->count == 0)
        return 0;

    c = f->data[f->tail++];
    if (f->tail == UART_FIFO_LENGTH)
        f->tail = 0;
    f->count--;

    return c;
}

static void serial_update_irq(PDEVSERIAL pThis)
{
    uint8_t tmp_iir = UART_IIR_NO_INT;

    if (   (pThis->ier & UART_IER_RLSI)
        && (pThis->lsr & UART_LSR_INT_ANY)) {
        tmp_iir = UART_IIR_RLSI;
    } else if ((pThis->ier & UART_IER_RDI) && pThis->timeout_ipending) {
        /* Note that(pThis->ier & UART_IER_RDI) can mask this interrupt,
         * this is not in the specification but is observed on existing
         * hardware. */
        tmp_iir = UART_IIR_CTI;
    } else if (   (pThis->ier & UART_IER_RDI)
               && (pThis->lsr & UART_LSR_DR)
               && (   !(pThis->fcr & UART_FCR_FE)
                   || pThis->recv_fifo.count >= pThis->recv_fifo.itl)) {
        tmp_iir = UART_IIR_RDI;
    } else if (   (pThis->ier & UART_IER_THRI)
               && pThis->thr_ipending) {
        tmp_iir = UART_IIR_THRI;
    } else if (   (pThis->ier & UART_IER_MSI)
               && (pThis->msr & UART_MSR_ANY_DELTA)) {
        tmp_iir = UART_IIR_MSI;
    }
    pThis->iir = tmp_iir | (pThis->iir & 0xF0);

    /** XXX only call the SetIrq function if the state really changes! */
    if (tmp_iir != UART_IIR_NO_INT) {
        Log(("serial_update_irq %d 1\n", pThis->irq));
# ifdef VBOX_SERIAL_PCI
        PDMDevHlpPCISetIrqNoWait(pThis->CTX_SUFF(pDevIns), 0, 1);
# else /* !VBOX_SERIAL_PCI */
        PDMDevHlpISASetIrqNoWait(pThis->CTX_SUFF(pDevIns), pThis->irq, 1);
# endif /* !VBOX_SERIAL_PCI */
    } else {
        Log(("serial_update_irq %d 0\n", pThis->irq));
# ifdef VBOX_SERIAL_PCI
        PDMDevHlpPCISetIrqNoWait(pThis->CTX_SUFF(pDevIns), 0, 0);
# else /* !VBOX_SERIAL_PCI */
        PDMDevHlpISASetIrqNoWait(pThis->CTX_SUFF(pDevIns), pThis->irq, 0);
# endif /* !VBOX_SERIAL_PCI */
    }
}

static void serial_tsr_retry_update_parameters(PDEVSERIAL pThis, uint64_t tf)
{
    pThis->tsr_retry_bound_max = RT_MAX((tf * MAX_XMIT_RETRY_TIME) / pThis->char_transmit_time, MIN_XMIT_RETRY);
    pThis->tsr_retry_bound_min = RT_MAX(pThis->tsr_retry_bound_max / (1000 * MAX_XMIT_RETRY_TIME), MIN_XMIT_RETRY);
    /* for simplicity just reset to max retry count */
    pThis->tsr_retry_bound = pThis->tsr_retry_bound_max;
}

static void serial_tsr_retry_bound_reached(PDEVSERIAL pThis)
{
    /* this is most likely means we have some backend connection issues */
    /* decrement the retry bound */
    pThis->tsr_retry_bound = RT_MAX(pThis->tsr_retry_bound / (10 * MAX_XMIT_RETRY_TIME), pThis->tsr_retry_bound_min);
}

static void serial_tsr_retry_succeeded(PDEVSERIAL pThis)
{
    /* success means we have a backend connection working OK,
     * set retry bound to its maximum value */
    pThis->tsr_retry_bound = pThis->tsr_retry_bound_max;
}

static void serial_update_parameters(PDEVSERIAL pThis)
{
    int speed, parity, data_bits, stop_bits, frame_size;

    if (pThis->divider == 0)
        return;

    frame_size = 1;
    if (pThis->lcr & 0x08) {
        frame_size++;
        if (pThis->lcr & 0x10)
            parity = 'E';
        else
            parity = 'O';
    } else {
            parity = 'N';
    }
    if (pThis->lcr & 0x04)
        stop_bits = 2;
    else
        stop_bits = 1;

    data_bits = (pThis->lcr & 0x03) + 5;
    frame_size += data_bits + stop_bits;
    speed = 115200 / pThis->divider;
    uint64_t tf = TMTimerGetFreq(CTX_SUFF(pThis->transmit_timer));
    pThis->char_transmit_time = (tf / speed) * frame_size;
    serial_tsr_retry_update_parameters(pThis, tf);

    Log(("speed=%d parity=%c data=%d stop=%d\n", speed, parity, data_bits, stop_bits));

    if (RT_LIKELY(pThis->pDrvChar))
        pThis->pDrvChar->pfnSetParameters(pThis->pDrvChar, speed, parity, data_bits, stop_bits);
}

static void serial_xmit(PDEVSERIAL pThis, bool bRetryXmit)
{
    if (pThis->tsr_retry <= 0) {
        if (pThis->fcr & UART_FCR_FE) {
            pThis->tsr = fifo_get(pThis, XMIT_FIFO);
            if (!pThis->xmit_fifo.count)
                pThis->lsr |= UART_LSR_THRE;
        } else {
            pThis->tsr = pThis->thr;
            pThis->lsr |= UART_LSR_THRE;
        }
    }

    if (pThis->mcr & UART_MCR_LOOP) {
        /* in loopback mode, say that we just received a char */
        serial_receive(pThis, &pThis->tsr, 1);
    } else if (   RT_LIKELY(pThis->pDrvChar)
               && RT_FAILURE(pThis->pDrvChar->pfnWrite(pThis->pDrvChar, &pThis->tsr, 1))) {
        if ((pThis->tsr_retry >= 0) && ((!bRetryXmit) || (pThis->tsr_retry <= pThis->tsr_retry_bound))) {
            if (!pThis->tsr_retry)
                pThis->tsr_retry = 1; /* make sure the retry state is always set */
            else if (bRetryXmit) /* do not increase the retry count if the retry is actually caused by next char write */
                pThis->tsr_retry++;

            TMTimerSet(CTX_SUFF(pThis->transmit_timer), TMTimerGet(CTX_SUFF(pThis->transmit_timer)) + pThis->char_transmit_time * 4);
            return;
        } else {
            /* drop this character. */
            pThis->tsr_retry = 0;
            serial_tsr_retry_bound_reached(pThis);
        }
    }
    else {
        pThis->tsr_retry = 0;
        serial_tsr_retry_succeeded(pThis);
    }

    if (!(pThis->lsr & UART_LSR_THRE))
        TMTimerSet(CTX_SUFF(pThis->transmit_timer),
                   TMTimerGet(CTX_SUFF(pThis->transmit_timer)) + pThis->char_transmit_time);

    if (pThis->lsr & UART_LSR_THRE) {
        pThis->lsr |= UART_LSR_TEMT;
        pThis->thr_ipending = 1;
        serial_update_irq(pThis);
    }
}

#endif /* IN_RING3 */

static int serial_ioport_write(PDEVSERIAL pThis, uint32_t addr, uint32_t val)
{
#ifndef IN_RING3
    NOREF(pThis); RT_NOREF_PV(addr); RT_NOREF_PV(val);
    return VINF_IOM_R3_IOPORT_WRITE;
#else
    addr &= 7;
    switch(addr) {
    default:
    case 0:
        if (pThis->lcr & UART_LCR_DLAB) {
            pThis->divider = (pThis->divider & 0xff00) | val;
            serial_update_parameters(pThis);
        } else {
            pThis->thr = (uint8_t) val;
            if (pThis->fcr & UART_FCR_FE) {
                fifo_put(pThis, XMIT_FIFO, pThis->thr);
                pThis->thr_ipending = 0;
                pThis->lsr &= ~UART_LSR_TEMT;
                pThis->lsr &= ~UART_LSR_THRE;
                serial_update_irq(pThis);
            } else {
                pThis->thr_ipending = 0;
                pThis->lsr &= ~UART_LSR_THRE;
                serial_update_irq(pThis);
            }
            serial_xmit(pThis, false);
        }
        break;
    case 1:
        if (pThis->lcr & UART_LCR_DLAB) {
            pThis->divider = (pThis->divider & 0x00ff) | (val << 8);
            serial_update_parameters(pThis);
        } else {
            pThis->ier = val & 0x0f;
            if (pThis->lsr & UART_LSR_THRE) {
                pThis->thr_ipending = 1;
                serial_update_irq(pThis);
            }
        }
        break;
    case 2:
        if (!pThis->f16550AEnabled)
            break;

        val = val & 0xFF;

        if (pThis->fcr == val)
            break;

        /* Did the enable/disable flag change? If so, make sure FIFOs get flushed */
        if ((val ^ pThis->fcr) & UART_FCR_FE)
            val |= UART_FCR_XFR | UART_FCR_RFR;

        /* FIFO clear */
        if (val & UART_FCR_RFR) {
            TMTimerStop(pThis->fifo_timeout_timer);
            pThis->timeout_ipending = 0;
            fifo_clear(pThis, RECV_FIFO);
        }
        if (val & UART_FCR_XFR) {
            fifo_clear(pThis, XMIT_FIFO);
        }

        if (val & UART_FCR_FE) {
            pThis->iir |= UART_IIR_FE;
            /* Set RECV_FIFO trigger Level */
            switch (val & 0xC0) {
            case UART_FCR_ITL_1:
                pThis->recv_fifo.itl = 1;
                break;
            case UART_FCR_ITL_2:
                pThis->recv_fifo.itl = 4;
                break;
            case UART_FCR_ITL_3:
                pThis->recv_fifo.itl = 8;
                break;
            case UART_FCR_ITL_4:
                pThis->recv_fifo.itl = 14;
                break;
            }
        } else
            pThis->iir &= ~UART_IIR_FE;

        /* Set fcr - or at least the bits in it that are supposed to "stick" */
        pThis->fcr = val & 0xC9;
        serial_update_irq(pThis);
        break;
    case 3:
        {
            int break_enable;
            pThis->lcr = val;
            serial_update_parameters(pThis);
            break_enable = (val >> 6) & 1;
            if (break_enable != pThis->last_break_enable) {
                pThis->last_break_enable = break_enable;
                if (RT_LIKELY(pThis->pDrvChar))
                {
                    Log(("serial_ioport_write: Set break %d\n", break_enable));
                    int rc = pThis->pDrvChar->pfnSetBreak(pThis->pDrvChar, !!break_enable);
                    AssertRC(rc);
                }
            }
        }
        break;
    case 4:
        pThis->mcr = val & 0x1f;
        if (RT_LIKELY(pThis->pDrvChar))
        {
            int rc = pThis->pDrvChar->pfnSetModemLines(pThis->pDrvChar,
                                                   !!(pThis->mcr & UART_MCR_RTS),
                                                   !!(pThis->mcr & UART_MCR_DTR));
            AssertRC(rc);
        }
        break;
    case 5:
        break;
    case 6:
        break;
    case 7:
        pThis->scr = val;
        break;
    }
    return VINF_SUCCESS;
#endif
}

static uint32_t serial_ioport_read(PDEVSERIAL pThis, uint32_t addr, int *pRC)
{
    uint32_t ret = ~0U;

    *pRC = VINF_SUCCESS;

    addr &= 7;
    switch(addr) {
    default:
    case 0:
        if (pThis->lcr & UART_LCR_DLAB) {
            /* DLAB == 1: divisor latch (LS) */
            ret = pThis->divider & 0xff;
        } else {
#ifndef IN_RING3
            *pRC = VINF_IOM_R3_IOPORT_READ;
#else
            if (pThis->fcr & UART_FCR_FE) {
                ret = fifo_get(pThis, RECV_FIFO);
                if (pThis->recv_fifo.count == 0)
                    pThis->lsr &= ~(UART_LSR_DR | UART_LSR_BI);
                else
                    TMTimerSet(pThis->fifo_timeout_timer,
                               TMTimerGet(pThis->fifo_timeout_timer) + pThis->char_transmit_time * 4);
                pThis->timeout_ipending = 0;
            } else {
                Log(("serial_io_port_read: read 0x%X\n", pThis->rbr));
                ret = pThis->rbr;
                pThis->lsr &= ~(UART_LSR_DR | UART_LSR_BI);
            }
            serial_update_irq(pThis);
            if (pThis->fRecvWaiting)
            {
                pThis->fRecvWaiting = false;
                int rc = RTSemEventSignal(pThis->ReceiveSem);
                AssertRC(rc);
            }
#endif
        }
        break;
    case 1:
        if (pThis->lcr & UART_LCR_DLAB) {
            /* DLAB == 1: divisor latch (MS) */
            ret = (pThis->divider >> 8) & 0xff;
        } else {
            ret = pThis->ier;
        }
        break;
    case 2:
#ifndef IN_RING3
        *pRC = VINF_IOM_R3_IOPORT_READ;
#else
        ret = pThis->iir;
        if ((ret & UART_IIR_ID) == UART_IIR_THRI) {
            pThis->thr_ipending = 0;
            serial_update_irq(pThis);
        }
        /* reset msr changed bit */
        pThis->msr_changed = false;
#endif
        break;
    case 3:
        ret = pThis->lcr;
        break;
    case 4:
        ret = pThis->mcr;
        break;
    case 5:
        if ((pThis->lsr & UART_LSR_DR) == 0 && pThis->fYieldOnLSRRead)
        {
            /* No data available and yielding is enabled, so yield in ring3. */
#ifndef IN_RING3
            *pRC = VINF_IOM_R3_IOPORT_READ;
            break;
#else
            RTThreadYield ();
#endif
        }
        ret = pThis->lsr;
        /* Clear break and overrun interrupts */
        if (pThis->lsr & (UART_LSR_BI|UART_LSR_OE)) {
#ifndef IN_RING3
            *pRC = VINF_IOM_R3_IOPORT_READ;
#else
            pThis->lsr &= ~(UART_LSR_BI|UART_LSR_OE);
            serial_update_irq(pThis);
#endif
        }
        break;
    case 6:
        if (pThis->mcr & UART_MCR_LOOP) {
            /* in loopback, the modem output pins are connected to the
               inputs */
            ret = (pThis->mcr & 0x0c) << 4;
            ret |= (pThis->mcr & 0x02) << 3;
            ret |= (pThis->mcr & 0x01) << 5;
        } else {
            ret = pThis->msr;
            /* Clear delta bits & msr int after read, if they were set */
            if (pThis->msr & UART_MSR_ANY_DELTA) {
#ifndef IN_RING3
                *pRC = VINF_IOM_R3_IOPORT_READ;
#else
                pThis->msr &= 0xF0;
                serial_update_irq(pThis);
#endif
            }
        }
        break;
    case 7:
        ret = pThis->scr;
        break;
    }
    return ret;
}

#ifdef IN_RING3

static int serial_can_receive(PDEVSERIAL pThis)
{
    if (pThis->fcr & UART_FCR_FE) {
        if (pThis->recv_fifo.count < UART_FIFO_LENGTH)
            return (pThis->recv_fifo.count <= pThis->recv_fifo.itl)
                ? pThis->recv_fifo.itl - pThis->recv_fifo.count : 1;
        else
            return 0;
    } else {
        return !(pThis->lsr & UART_LSR_DR);
    }
}

static void serial_receive(PDEVSERIAL pThis, const uint8_t *buf, int size)
{
    if (pThis->fcr & UART_FCR_FE) {
        int i;
        for (i = 0; i < size; i++) {
            fifo_put(pThis, RECV_FIFO, buf[i]);
        }
        pThis->lsr |= UART_LSR_DR;
        /* call the timeout receive callback in 4 char transmit time */
        TMTimerSet(pThis->fifo_timeout_timer, TMTimerGet(pThis->fifo_timeout_timer) + pThis->char_transmit_time * 4);
    } else {
        if (pThis->lsr & UART_LSR_DR)
            pThis->lsr |= UART_LSR_OE;
        pThis->rbr = buf[0];
        pThis->lsr |= UART_LSR_DR;
    }
    serial_update_irq(pThis);
}


/**
 * @interface_method_impl{PDMICHARPORT,pfnNotifyRead}
 */
static DECLCALLBACK(int) serialNotifyRead(PPDMICHARPORT pInterface, const void *pvBuf, size_t *pcbRead)
{
    PDEVSERIAL pThis = RT_FROM_MEMBER(pInterface, DEVSERIAL, ICharPort);
    const uint8_t *pu8Buf = (const uint8_t*)pvBuf;
    size_t cbRead = *pcbRead;

    PDMCritSectEnter(&pThis->CritSect, VERR_PERMISSION_DENIED);
    for (; cbRead > 0; cbRead--, pu8Buf++)
    {
        if (!serial_can_receive(pThis))
        {
            /* If we cannot receive then wait for not more than 250ms. If we still
             * cannot receive then the new character will either overwrite rbr
             * or it will be dropped at fifo_put(). */
            pThis->fRecvWaiting = true;
            PDMCritSectLeave(&pThis->CritSect);
            RTSemEventWait(pThis->ReceiveSem, 250);
            PDMCritSectEnter(&pThis->CritSect, VERR_PERMISSION_DENIED);
        }
        serial_receive(pThis, &pu8Buf[0], 1);
    }
    PDMCritSectLeave(&pThis->CritSect);
    return VINF_SUCCESS;
}

/**
 * @@interface_method_impl{PDMICHARPORT,pfnNotifyStatusLinesChanged}
 */
static DECLCALLBACK(int) serialNotifyStatusLinesChanged(PPDMICHARPORT pInterface, uint32_t newStatusLines)
{
    PDEVSERIAL pThis = RT_FROM_MEMBER(pInterface, DEVSERIAL, ICharPort);
    uint8_t newMsr = 0;

    Log(("%s: pInterface=%p newStatusLines=%u\n", __FUNCTION__, pInterface, newStatusLines));

    PDMCritSectEnter(&pThis->CritSect, VERR_PERMISSION_DENIED);

    /* Set new states. */
    if (newStatusLines & PDMICHARPORT_STATUS_LINES_DCD)
        newMsr |= UART_MSR_DCD;
    if (newStatusLines & PDMICHARPORT_STATUS_LINES_RI)
        newMsr |= UART_MSR_RI;
    if (newStatusLines & PDMICHARPORT_STATUS_LINES_DSR)
        newMsr |= UART_MSR_DSR;
    if (newStatusLines & PDMICHARPORT_STATUS_LINES_CTS)
        newMsr |= UART_MSR_CTS;

    /* Compare the old and the new states and set the delta bits accordingly. */
    if ((newMsr & UART_MSR_DCD) != (pThis->msr & UART_MSR_DCD))
        newMsr |= UART_MSR_DDCD;
    if ((newMsr & UART_MSR_RI) != 0 && (pThis->msr & UART_MSR_RI) == 0)
        newMsr |= UART_MSR_TERI;
    if ((newMsr & UART_MSR_DSR) != (pThis->msr & UART_MSR_DSR))
        newMsr |= UART_MSR_DDSR;
    if ((newMsr & UART_MSR_CTS) != (pThis->msr & UART_MSR_CTS))
        newMsr |= UART_MSR_DCTS;

    pThis->msr = newMsr;
    pThis->msr_changed = true;
    serial_update_irq(pThis);

    PDMCritSectLeave(&pThis->CritSect);

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMICHARPORT,pfnNotifyBufferFull}
 */
static DECLCALLBACK(int) serialNotifyBufferFull(PPDMICHARPORT pInterface, bool fFull)
{
    RT_NOREF(pInterface, fFull);
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMICHARPORT,pfnNotifyBreak}
 */
static DECLCALLBACK(int) serialNotifyBreak(PPDMICHARPORT pInterface)
{
    PDEVSERIAL pThis = RT_FROM_MEMBER(pInterface, DEVSERIAL, ICharPort);

    Log(("%s: pInterface=%p\n", __FUNCTION__, pInterface));

    PDMCritSectEnter(&pThis->CritSect, VERR_PERMISSION_DENIED);

    pThis->lsr |= UART_LSR_BI;
    serial_update_irq(pThis);

    PDMCritSectLeave(&pThis->CritSect);

    return VINF_SUCCESS;
}


/* -=-=-=-=-=-=-=-=- Timer callbacks -=-=-=-=-=-=-=-=- */

/**
 * @callback_method_impl{FNTMTIMERDEV, Fifo timer function.}
 */
static DECLCALLBACK(void) serialFifoTimer(PPDMDEVINS pDevIns, PTMTIMER pTimer, void *pvUser)
{
    RT_NOREF(pDevIns, pTimer);
    PDEVSERIAL pThis = (PDEVSERIAL)pvUser;
    Assert(PDMCritSectIsOwner(&pThis->CritSect));
    if (pThis->recv_fifo.count)
    {
        pThis->timeout_ipending = 1;
        serial_update_irq(pThis);
    }
}

/**
 * @callback_method_impl{FNTMTIMERDEV, Transmit timer function.}
 *
 * Just retry to transmit a character.
 */
static DECLCALLBACK(void) serialTransmitTimer(PPDMDEVINS pDevIns, PTMTIMER pTimer, void *pvUser)
{
    RT_NOREF(pDevIns, pTimer);
    PDEVSERIAL pThis = (PDEVSERIAL)pvUser;
    Assert(PDMCritSectIsOwner(&pThis->CritSect));
    serial_xmit(pThis, true);
}

#endif /* IN_RING3 */

/* -=-=-=-=-=-=-=-=- I/O Port Access Handlers -=-=-=-=-=-=-=-=- */

/**
 * @callback_method_impl{FNIOMIOPORTOUT}
 */
PDMBOTHCBDECL(int) serialIOPortWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint32_t u32, unsigned cb)
{
    PDEVSERIAL pThis = PDMINS_2_DATA(pDevIns, PDEVSERIAL);
    int          rc;
    Assert(PDMCritSectIsOwner(&pThis->CritSect));
    RT_NOREF_PV(pvUser);

    if (cb == 1)
    {
        Log2(("%s: port %#06x val %#04x\n", __FUNCTION__, uPort, u32));
        rc = serial_ioport_write(pThis, uPort, u32);
    }
    else
    {
        AssertMsgFailed(("uPort=%#x cb=%d u32=%#x\n", uPort, cb, u32));
        rc = VINF_SUCCESS;
    }

    return rc;
}


/**
 * @callback_method_impl{FNIOMIOPORTIN}
 */
PDMBOTHCBDECL(int) serialIOPortRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint32_t *pu32, unsigned cb)
{
    PDEVSERIAL pThis = PDMINS_2_DATA(pDevIns, PDEVSERIAL);
    int          rc;
    Assert(PDMCritSectIsOwner(&pThis->CritSect));
    RT_NOREF_PV(pvUser);

    if (cb == 1)
    {
        *pu32 = serial_ioport_read(pThis, uPort, &rc);
        Log2(("%s: port %#06x val %#04x\n", __FUNCTION__, uPort, *pu32));
    }
    else
        rc = VERR_IOM_IOPORT_UNUSED;

    return rc;
}

#ifdef IN_RING3

/* -=-=-=-=-=-=-=-=- Saved State -=-=-=-=-=-=-=-=- */

/**
 * @callback_method_impl{FNSSMDEVLIVEEXEC}
 */
static DECLCALLBACK(int) serialLiveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uPass)
{
    RT_NOREF(uPass);
    PDEVSERIAL pThis = PDMINS_2_DATA(pDevIns, PDEVSERIAL);
    SSMR3PutS32(pSSM, pThis->irq);
    SSMR3PutU32(pSSM, pThis->base);
    return VINF_SSM_DONT_CALL_AGAIN;
}


/**
 * @callback_method_impl{FNSSMDEVSAVEEXEC}
 */
static DECLCALLBACK(int) serialSaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    PDEVSERIAL pThis = PDMINS_2_DATA(pDevIns, PDEVSERIAL);

    SSMR3PutU16(pSSM, pThis->divider);
    SSMR3PutU8(pSSM, pThis->rbr);
    SSMR3PutU8(pSSM, pThis->ier);
    SSMR3PutU8(pSSM, pThis->lcr);
    SSMR3PutU8(pSSM, pThis->mcr);
    SSMR3PutU8(pSSM, pThis->lsr);
    SSMR3PutU8(pSSM, pThis->msr);
    SSMR3PutU8(pSSM, pThis->scr);
    SSMR3PutU8(pSSM, pThis->fcr); /* 16550A */
    SSMR3PutS32(pSSM, pThis->thr_ipending);
    SSMR3PutS32(pSSM, pThis->irq);
    SSMR3PutS32(pSSM, pThis->last_break_enable);
    SSMR3PutU32(pSSM, pThis->base);
    SSMR3PutBool(pSSM, pThis->msr_changed);

    /* Version 5, safe everything that might be of importance.  Much better than
       missing relevant bits! */
    SSMR3PutU8(pSSM, pThis->thr);
    SSMR3PutU8(pSSM, pThis->tsr);
    SSMR3PutU8(pSSM, pThis->iir);
    SSMR3PutS32(pSSM, pThis->timeout_ipending);
    TMR3TimerSave(pThis->fifo_timeout_timer, pSSM);
    TMR3TimerSave(pThis->transmit_timerR3, pSSM);
    SSMR3PutU8(pSSM, pThis->recv_fifo.itl);
    SSMR3PutU8(pSSM, pThis->xmit_fifo.itl);

    /* Don't store:
     *  - the content of the FIFO
     *  - tsr_retry
     */

    return SSMR3PutU32(pSSM, UINT32_MAX); /* sanity/terminator */
}


/**
 * @callback_method_impl{FNSSMDEVLOADEXEC}
 */
static DECLCALLBACK(int) serialLoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    PDEVSERIAL  pThis = PDMINS_2_DATA(pDevIns, PDEVSERIAL);
    int32_t     iIrq;
    uint32_t    IOBase;

    AssertMsgReturn(uVersion >= SERIAL_SAVED_STATE_VERSION_16450, ("%d\n", uVersion), VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION);

    if (uPass != SSM_PASS_FINAL)
    {
        SSMR3GetS32(pSSM, &iIrq);
        int rc = SSMR3GetU32(pSSM, &IOBase);
        AssertRCReturn(rc, rc);
    }
    else
    {
        if (uVersion == SERIAL_SAVED_STATE_VERSION_16450)
        {
            pThis->f16550AEnabled = false;
            LogRel(("Serial#%d: falling back to 16450 mode from load state\n", pDevIns->iInstance));
        }

        SSMR3GetU16(pSSM, &pThis->divider);
        SSMR3GetU8(pSSM, &pThis->rbr);
        SSMR3GetU8(pSSM, &pThis->ier);
        SSMR3GetU8(pSSM, &pThis->lcr);
        SSMR3GetU8(pSSM, &pThis->mcr);
        SSMR3GetU8(pSSM, &pThis->lsr);
        SSMR3GetU8(pSSM, &pThis->msr);
        SSMR3GetU8(pSSM, &pThis->scr);
        if (uVersion > SERIAL_SAVED_STATE_VERSION_16450)
            SSMR3GetU8(pSSM, &pThis->fcr);
        SSMR3GetS32(pSSM, &pThis->thr_ipending);
        SSMR3GetS32(pSSM, &iIrq);
        SSMR3GetS32(pSSM, &pThis->last_break_enable);
        SSMR3GetU32(pSSM, &IOBase);
        SSMR3GetBool(pSSM, &pThis->msr_changed);

        if (uVersion > SERIAL_SAVED_STATE_VERSION_MISSING_BITS)
        {
            SSMR3GetU8(pSSM, &pThis->thr);
            SSMR3GetU8(pSSM, &pThis->tsr);
            SSMR3GetU8(pSSM, &pThis->iir);
            SSMR3GetS32(pSSM, &pThis->timeout_ipending);
            TMR3TimerLoad(pThis->fifo_timeout_timer, pSSM);
            TMR3TimerLoad(pThis->transmit_timerR3, pSSM);
            SSMR3GetU8(pSSM, &pThis->recv_fifo.itl);
            SSMR3GetU8(pSSM, &pThis->xmit_fifo.itl);
        }

        /* the marker. */
        uint32_t u32;
        int rc = SSMR3GetU32(pSSM, &u32);
        if (RT_FAILURE(rc))
            return rc;
        AssertMsgReturn(u32 == UINT32_MAX, ("%#x\n", u32), VERR_SSM_DATA_UNIT_FORMAT_CHANGED);

        if (   (pThis->lsr & UART_LSR_DR)
            || pThis->fRecvWaiting)
        {
            pThis->fRecvWaiting = false;
            rc = RTSemEventSignal(pThis->ReceiveSem);
            AssertRC(rc);
        }

        /* this isn't strictly necessary but cannot hurt... */
        pThis->pDevInsR3 = pDevIns;
        pThis->pDevInsR0 = PDMDEVINS_2_R0PTR(pDevIns);
        pThis->pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);
    }

    /*
     * Check the config.
     */
    if (    pThis->irq  != iIrq
        ||  pThis->base != IOBase)
        return SSMR3SetCfgError(pSSM, RT_SRC_POS,
                                N_("Config mismatch - saved irq=%#x iobase=%#x; configured irq=%#x iobase=%#x"),
                                iIrq, IOBase, pThis->irq, pThis->base);

    return VINF_SUCCESS;
}


#ifdef VBOX_SERIAL_PCI
/* -=-=-=-=-=-=-=-=- PCI Device Callback(s) -=-=-=-=-=-=-=-=- */

/**
 * @callback_method_impl{FNPCIIOREGIONMAP}
 */
static DECLCALLBACK(int) serialIOPortRegionMap(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion,
                                               RTGCPHYS GCPhysAddress, RTGCPHYS cb, PCIADDRESSSPACE enmType)
{
    PDEVSERIAL pThis = RT_FROM_MEMBER(pPciDev, DEVSERIAL, PciDev);
    int rc = VINF_SUCCESS;

    Assert(enmType == PCI_ADDRESS_SPACE_IO);
    Assert(iRegion == 0);
    Assert(cb == 8);
    AssertMsg(RT_ALIGN(GCPhysAddress, 8) == GCPhysAddress, ("Expected 8 byte alignment. GCPhysAddress=%#x\n", GCPhysAddress));

    pThis->base = (RTIOPORT)GCPhysAddress;
    LogRel(("Serial#%d: mapping I/O at %#06x\n", pThis->pDevIns->iInstance, pThis->base));

    /*
     * Register our port IO handlers.
     */
    rc = PDMDevHlpIOPortRegister(pDevIns, (RTIOPORT)GCPhysAddress, 8, (void *)pThis,
                                 serial_io_write, serial_io_read, NULL, NULL, "SERIAL");
    AssertRC(rc);
    return rc;
}

#endif /* VBOX_SERIAL_PCI */


/* -=-=-=-=-=-=-=-=- PDMIBASE on LUN#1 -=-=-=-=-=-=-=-=- */

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) serialQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PDEVSERIAL pThis = RT_FROM_MEMBER(pInterface, DEVSERIAL, IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pThis->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMICHARPORT, &pThis->ICharPort);
    return NULL;
}


/* -=-=-=-=-=-=-=-=- PDMDEVREG -=-=-=-=-=-=-=-=- */

/**
 * @interface_method_impl{PDMDEVREG,pfnRelocate}
 */
static DECLCALLBACK(void) serialRelocate(PPDMDEVINS pDevIns, RTGCINTPTR offDelta)
{
    RT_NOREF(offDelta);
    PDEVSERIAL pThis = PDMINS_2_DATA(pDevIns, PDEVSERIAL);
    pThis->pDevInsRC        = PDMDEVINS_2_RCPTR(pDevIns);
    pThis->transmit_timerRC = TMTimerRCPtr(pThis->transmit_timerR3);
}


/**
 * @interface_method_impl{PDMDEVREG,pfnReset}
 */
static DECLCALLBACK(void) serialReset(PPDMDEVINS pDevIns)
{
    PDEVSERIAL pThis = PDMINS_2_DATA(pDevIns, PDEVSERIAL);

    pThis->rbr = 0;
    pThis->ier = 0;
    pThis->iir = UART_IIR_NO_INT;
    pThis->lcr = 3; /* 8 data bits, no parity, 1 stop bit */
    pThis->lsr = UART_LSR_TEMT | UART_LSR_THRE;
    pThis->msr = UART_MSR_DCD | UART_MSR_DSR | UART_MSR_CTS;
    /* Default to 9600 baud, 1 start bit, 8 data bits, 1 stop bit, no parity. */
    pThis->divider = 0x0C;
    pThis->mcr = UART_MCR_OUT2;
    pThis->scr = 0;
    pThis->tsr_retry = 0;
    uint64_t tf = TMTimerGetFreq(CTX_SUFF(pThis->transmit_timer));
    pThis->char_transmit_time = (tf / 9600) * 10;
    serial_tsr_retry_update_parameters(pThis, tf);

    /* Update parameters of the underlying driver to stay in sync. */
    serial_update_parameters(pThis);

    fifo_clear(pThis, RECV_FIFO);
    fifo_clear(pThis, XMIT_FIFO);

    pThis->thr_ipending = 0;
    pThis->last_break_enable = 0;
# ifdef VBOX_SERIAL_PCI
        PDMDevHlpPCISetIrqNoWait(pThis->CTX_SUFF(pDevIns), 0, 0);
# else /* !VBOX_SERIAL_PCI */
        PDMDevHlpISASetIrqNoWait(pThis->CTX_SUFF(pDevIns), pThis->irq, 0);
# endif /* !VBOX_SERIAL_PCI */
}


/**
 * @interface_method_impl{PDMDEVREG,pfnDestruct}
 */
static DECLCALLBACK(int) serialDestruct(PPDMDEVINS pDevIns)
{
    PDEVSERIAL pThis = PDMINS_2_DATA(pDevIns, PDEVSERIAL);
    PDMDEV_CHECK_VERSIONS_RETURN_QUIET(pDevIns);

    RTSemEventDestroy(pThis->ReceiveSem);
    pThis->ReceiveSem = NIL_RTSEMEVENT;

    PDMR3CritSectDelete(&pThis->CritSect);
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) serialConstruct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    PDEVSERIAL     pThis = PDMINS_2_DATA(pDevIns, PDEVSERIAL);
    int            rc;
    uint16_t       io_base;
    uint8_t        irq_lvl;

    Assert(iInstance < 4);
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    /*
     * Initialize the instance data.
     * (Do this early or the destructor might choke on something!)
     */
    pThis->pDevInsR3 = pDevIns;
    pThis->pDevInsR0 = PDMDEVINS_2_R0PTR(pDevIns);
    pThis->pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);
    pThis->ReceiveSem = NIL_RTSEMEVENT;

    /* IBase */
    pThis->IBase.pfnQueryInterface = serialQueryInterface;

    /* ICharPort */
    pThis->ICharPort.pfnNotifyRead               = serialNotifyRead;
    pThis->ICharPort.pfnNotifyStatusLinesChanged = serialNotifyStatusLinesChanged;
    pThis->ICharPort.pfnNotifyBufferFull         = serialNotifyBufferFull;
    pThis->ICharPort.pfnNotifyBreak              = serialNotifyBreak;

#ifdef VBOX_SERIAL_PCI
    /* the PCI device */
    pThis->PciDev.abConfig[0x00] = 0xee; /* Vendor: ??? */
    pThis->PciDev.abConfig[0x01] = 0x80;
    pThis->PciDev.abConfig[0x02] = 0x01; /* Device: ??? */
    pThis->PciDev.abConfig[0x03] = 0x01;
    pThis->PciDev.abConfig[0x04] = PCI_COMMAND_IOACCESS;
    pThis->PciDev.abConfig[0x09] = 0x01; /* Programming interface: 16450 */
    pThis->PciDev.abConfig[0x0a] = 0x00; /* Subclass: Serial controller */
    pThis->PciDev.abConfig[0x0b] = 0x07; /* Class: Communication controller */
    pThis->PciDev.abConfig[0x0e] = 0x00; /* Header type: standard */
    pThis->PciDev.abConfig[0x3c] = irq_lvl; /* preconfigure IRQ number (0 = autoconfig)*/
    pThis->PciDev.abConfig[0x3d] = 1;    /* interrupt pin 0 */
#endif /* VBOX_SERIAL_PCI */

    /*
     * Validate and read the configuration.
     */
    if (!CFGMR3AreValuesValid(pCfg, "IRQ\0"
                                    "IOBase\0"
                                    "GCEnabled\0"
                                    "R0Enabled\0"
                                    "YieldOnLSRRead\0"
                                    "Enable16550A\0"
                                    ))
    {
        AssertMsgFailed(("serialConstruct Invalid configuration values\n"));
        return VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES;
    }

    rc = CFGMR3QueryBoolDef(pCfg, "GCEnabled", &pThis->fGCEnabled, true);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to get the \"GCEnabled\" value"));

    rc = CFGMR3QueryBoolDef(pCfg, "R0Enabled", &pThis->fR0Enabled, true);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to get the \"R0Enabled\" value"));

    rc = CFGMR3QueryBoolDef(pCfg, "YieldOnLSRRead", &pThis->fYieldOnLSRRead, false);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to get the \"YieldOnLSRRead\" value"));

    rc = CFGMR3QueryU8(pCfg, "IRQ", &irq_lvl);
    if (rc == VERR_CFGM_VALUE_NOT_FOUND)
    {
        /* Provide sensible defaults. */
        if (iInstance == 0)
            irq_lvl = 4;
        else if (iInstance == 1)
            irq_lvl = 3;
        else
            AssertReleaseFailed(); /* irq_lvl is undefined. */
    }
    else if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to get the \"IRQ\" value"));

    rc = CFGMR3QueryU16(pCfg, "IOBase", &io_base);
    if (rc == VERR_CFGM_VALUE_NOT_FOUND)
    {
        if (iInstance == 0)
            io_base = 0x3f8;
        else if (iInstance == 1)
            io_base = 0x2f8;
        else
            AssertReleaseFailed(); /* io_base is undefined */
    }
    else if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to get the \"IOBase\" value"));

    Log(("DevSerial: instance %d iobase=%04x irq=%d\n", iInstance, io_base, irq_lvl));

    rc = CFGMR3QueryBoolDef(pCfg, "Enable16550A", &pThis->f16550AEnabled, true);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to get the \"Enable16550A\" value"));

    pThis->irq = irq_lvl;
#ifdef VBOX_SERIAL_PCI
    pThis->base = -1;
#else
    pThis->base = io_base;
#endif

    LogRel(("Serial#%d: emulating %s\n", pDevIns->iInstance, pThis->f16550AEnabled ? "16550A" : "16450"));

    /*
     * Initialize critical section and the semaphore.  Change the default
     * critical section to ours so that TM and IOM will enter it before
     * calling us.
     *
     * Note! This must of be done BEFORE creating timers, registering I/O ports
     *       and other things which might pick up the default CS or end up
     *       calling back into the device.
     */
    rc = PDMDevHlpCritSectInit(pDevIns, &pThis->CritSect, RT_SRC_POS, "Serial#%u", iInstance);
    AssertRCReturn(rc, rc);

    rc = PDMDevHlpSetDeviceCritSect(pDevIns, &pThis->CritSect);
    AssertRCReturn(rc, rc);

    rc = RTSemEventCreate(&pThis->ReceiveSem);
    AssertRCReturn(rc, rc);

    /*
     * Create the timers.
     */
    rc = PDMDevHlpTMTimerCreate(pDevIns, TMCLOCK_VIRTUAL, serialFifoTimer, pThis,
                                TMTIMER_FLAGS_DEFAULT_CRIT_SECT, "Serial Fifo Timer",
                                &pThis->fifo_timeout_timer);
    AssertRCReturn(rc, rc);

    rc = PDMDevHlpTMTimerCreate(pDevIns, TMCLOCK_VIRTUAL, serialTransmitTimer, pThis,
                                TMTIMER_FLAGS_DEFAULT_CRIT_SECT, "Serial Transmit Timer",
                                &pThis->transmit_timerR3);
    AssertRCReturn(rc, rc);
    pThis->transmit_timerR0 = TMTimerR0Ptr(pThis->transmit_timerR3);
    pThis->transmit_timerRC = TMTimerRCPtr(pThis->transmit_timerR3);

#ifdef VBOX_SERIAL_PCI
    /*
     * Register the PCI Device and region.
     */
    rc = PDMDevHlpPCIRegister(pDevIns, &pThis->PciDev);
    if (RT_FAILURE(rc))
        return rc;
    rc = PDMDevHlpPCIIORegionRegister(pDevIns, 0, 8, PCI_ADDRESS_SPACE_IO, serialIOPortRegionMap);
    if (RT_FAILURE(rc))
        return rc;

#else /* !VBOX_SERIAL_PCI */
    /*
     * Register the I/O ports.
     */
    pThis->base = io_base;
    rc = PDMDevHlpIOPortRegister(pDevIns, io_base, 8, 0,
                                 serialIOPortWrite, serialIOPortRead,
                                 NULL, NULL, "SERIAL");
    if (RT_FAILURE(rc))
        return rc;

    if (pThis->fGCEnabled)
    {
        rc = PDMDevHlpIOPortRegisterRC(pDevIns, io_base, 8, 0, "serialIOPortWrite",
                                      "serialIOPortRead", NULL, NULL, "Serial");
        if (RT_FAILURE(rc))
            return rc;
    }

    if (pThis->fR0Enabled)
    {
        rc = PDMDevHlpIOPortRegisterR0(pDevIns, io_base, 8, 0, "serialIOPortWrite",
                                      "serialIOPortRead", NULL, NULL, "Serial");
        if (RT_FAILURE(rc))
            return rc;
    }
#endif /* !VBOX_SERIAL_PCI */

    /*
     * Saved state.
     */
    rc = PDMDevHlpSSMRegister3(pDevIns, SERIAL_SAVED_STATE_VERSION, sizeof (*pThis),
                               serialLiveExec, serialSaveExec, serialLoadExec);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Attach the char driver and get the interfaces.
     * For now no run-time changes are supported.
     */
    rc = PDMDevHlpDriverAttach(pDevIns, 0, &pThis->IBase, &pThis->pDrvBase, "Serial Char");
    if (RT_SUCCESS(rc))
    {
        pThis->pDrvChar = PDMIBASE_QUERY_INTERFACE(pThis->pDrvBase, PDMICHARCONNECTOR);
        if (!pThis->pDrvChar)
        {
            AssertLogRelMsgFailed(("Configuration error: instance %d has no char interface!\n", iInstance));
            return VERR_PDM_MISSING_INTERFACE;
        }
        /** @todo provide read notification interface!!!! */
    }
    else if (rc == VERR_PDM_NO_ATTACHED_DRIVER)
    {
        pThis->pDrvBase = NULL;
        pThis->pDrvChar = NULL;
        LogRel(("Serial%d: no unit\n", iInstance));
    }
    else
    {
        AssertLogRelMsgFailed(("Serial%d: Failed to attach to char driver. rc=%Rrc\n", iInstance, rc));
        /* Don't call VMSetError here as we assume that the driver already set an appropriate error */
        return rc;
    }

    serialReset(pDevIns);
    return VINF_SUCCESS;
}


/**
 * The device registration structure.
 */
const PDMDEVREG g_DeviceSerialPort =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "serial",
    /* szRCMod */
    "VBoxDDRC.rc",
    /* szR0Mod */
    "VBoxDDR0.r0",
    /* pszDescription */
    "Serial Communication Port",
    /* fFlags */
    PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_RC | PDM_DEVREG_FLAGS_R0,
    /* fClass */
    PDM_DEVREG_CLASS_SERIAL,
    /* cMaxInstances */
    UINT32_MAX,
    /* cbInstance */
    sizeof(DEVSERIAL),
    /* pfnConstruct */
    serialConstruct,
    /* pfnDestruct */
    serialDestruct,
    /* pfnRelocate */
    serialRelocate,
    /* pfnMemSetup */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    serialReset,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnQueryInterface. */
    NULL,
    /* pfnInitComplete */
    NULL,
    /* pfnPowerOff */
    NULL,
    /* pfnSoftReset */
    NULL,
    /* u32VersionEnd */
    PDM_DEVREG_VERSION
};
#endif /* IN_RING3 */

#endif /* !VBOX_DEVICE_STRUCT_TESTCASE */
