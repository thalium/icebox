/* $Id: DevPS2.cpp $ */
/** @file
 * DevPS2 - PS/2 keyboard & mouse controller device.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
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
 * This code is based on:
 *
 * QEMU PC keyboard emulation (revision 1.12)
 *
 * Copyright (c) 2003 Fabrice Bellard
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
 *
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEV_KBD
#include <VBox/vmm/pdmdev.h>
#include <iprt/assert.h>
#include <iprt/uuid.h>

#include "VBoxDD.h"
#include "PS2Dev.h"

/* Do not remove this (unless eliminating the corresponding ifdefs), it will
 * cause instant triple faults when booting Windows VMs. */
#define TARGET_I386

#define PCKBD_SAVED_STATE_VERSION 8

#ifndef VBOX_DEVICE_STRUCT_TESTCASE


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
RT_C_DECLS_BEGIN
PDMBOTHCBDECL(int) kbdIOPortDataRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb);
PDMBOTHCBDECL(int) kbdIOPortDataWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb);
PDMBOTHCBDECL(int) kbdIOPortStatusRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb);
PDMBOTHCBDECL(int) kbdIOPortCommandWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb);
RT_C_DECLS_END
#endif /* !VBOX_DEVICE_STRUCT_TESTCASE */

/* debug PC keyboard */
#define DEBUG_KBD

/* debug PC keyboard : only mouse */
#define DEBUG_MOUSE

/*      Keyboard Controller Commands */
#define KBD_CCMD_READ_MODE      0x20    /* Read mode bits */
#define KBD_CCMD_WRITE_MODE     0x60    /* Write mode bits */
#define KBD_CCMD_GET_VERSION    0xA1    /* Get controller version */
#define KBD_CCMD_MOUSE_DISABLE  0xA7    /* Disable mouse interface */
#define KBD_CCMD_MOUSE_ENABLE   0xA8    /* Enable mouse interface */
#define KBD_CCMD_TEST_MOUSE     0xA9    /* Mouse interface test */
#define KBD_CCMD_SELF_TEST      0xAA    /* Controller self test */
#define KBD_CCMD_KBD_TEST       0xAB    /* Keyboard interface test */
#define KBD_CCMD_KBD_DISABLE    0xAD    /* Keyboard interface disable */
#define KBD_CCMD_KBD_ENABLE     0xAE    /* Keyboard interface enable */
#define KBD_CCMD_READ_INPORT    0xC0    /* read input port */
#define KBD_CCMD_READ_OUTPORT   0xD0    /* read output port */
#define KBD_CCMD_WRITE_OUTPORT  0xD1    /* write output port */
#define KBD_CCMD_WRITE_OBUF     0xD2
#define KBD_CCMD_WRITE_AUX_OBUF 0xD3    /* Write to output buffer as if
                                           initiated by the auxiliary device */
#define KBD_CCMD_WRITE_MOUSE    0xD4    /* Write the following byte to the mouse */
#define KBD_CCMD_DISABLE_A20    0xDD    /* HP vectra only ? */
#define KBD_CCMD_ENABLE_A20     0xDF    /* HP vectra only ? */
#define KBD_CCMD_READ_TSTINP    0xE0    /* Read test inputs T0, T1 */
#define KBD_CCMD_RESET_ALT      0xF0
#define KBD_CCMD_RESET          0xFE

/* Status Register Bits */
#define KBD_STAT_OBF            0x01    /* Keyboard output buffer full */
#define KBD_STAT_IBF            0x02    /* Keyboard input buffer full */
#define KBD_STAT_SELFTEST       0x04    /* Self test successful */
#define KBD_STAT_CMD            0x08    /* Last write was a command write (0=data) */
#define KBD_STAT_UNLOCKED       0x10    /* Zero if keyboard locked */
#define KBD_STAT_MOUSE_OBF      0x20    /* Mouse output buffer full */
#define KBD_STAT_GTO            0x40    /* General receive/xmit timeout */
#define KBD_STAT_PERR           0x80    /* Parity error */

/* Controller Mode Register Bits */
#define KBD_MODE_KBD_INT        0x01    /* Keyboard data generate IRQ1 */
#define KBD_MODE_MOUSE_INT      0x02    /* Mouse data generate IRQ12 */
#define KBD_MODE_SYS            0x04    /* The system flag (?) */
#define KBD_MODE_NO_KEYLOCK     0x08    /* The keylock doesn't affect the keyboard if set */
#define KBD_MODE_DISABLE_KBD    0x10    /* Disable keyboard interface */
#define KBD_MODE_DISABLE_MOUSE  0x20    /* Disable mouse interface */
#define KBD_MODE_KCC            0x40    /* Scan code conversion to PC format */
#define KBD_MODE_RFU            0x80


/**
 * The keyboard controller/device state.
 *
 * @note We use the default critical section for serialize data access.
 */
typedef struct KBDState
{
    uint8_t write_cmd; /* if non zero, write data to port 60 is expected */
    uint8_t status;
    uint8_t mode;
    uint8_t dbbout;    /* data buffer byte */
    /* keyboard state */
    int32_t translate;
    int32_t xlat_state;

    /** Pointer to the device instance - RC. */
    PPDMDEVINSRC                pDevInsRC;
    /** Pointer to the device instance - R3 . */
    PPDMDEVINSR3                pDevInsR3;
    /** Pointer to the device instance. */
    PPDMDEVINSR0                pDevInsR0;

    /** Keyboard state (implemented in separate PS2K module). */
#ifdef VBOX_DEVICE_STRUCT_TESTCASE
    uint8_t                     KbdFiller[PS2K_STRUCT_FILLER];
#else
    PS2K                        Kbd;
#endif

    /** Mouse state (implemented in separate PS2M module). */
#ifdef VBOX_DEVICE_STRUCT_TESTCASE
    uint8_t                     AuxFiller[PS2M_STRUCT_FILLER];
#else
    PS2M                        Aux;
#endif
} KBDState;

#ifndef VBOX_DEVICE_STRUCT_TESTCASE

/* update irq and KBD_STAT_[MOUSE_]OBF */
static void kbd_update_irq(KBDState *s)
{
    int irq12_level, irq1_level;
    uint8_t val;

    irq1_level = 0;
    irq12_level = 0;

    /* Determine new OBF state, but only if OBF is clear. If OBF was already
     * set, we cannot risk changing the event type after an ISR potentially
     * started executing! Only kbd_read_data() clears the OBF bits.
     */
    if (!(s->status & KBD_STAT_OBF)) {
        s->status &= ~KBD_STAT_MOUSE_OBF;
        /* Keyboard data has priority if both kbd and aux data is available. */
        if (!(s->mode & KBD_MODE_DISABLE_KBD) && PS2KByteFromKbd(&s->Kbd, &val) == VINF_SUCCESS)
        {
            bool    fHaveData = true;

            /* If scancode translation is on (it usually is), there's more work to do. */
            if (s->translate)
            {
                uint8_t     xlated_val;

                s->xlat_state = XlateAT2PC(s->xlat_state, val, &xlated_val);
                val = xlated_val;

                /* If the translation state is XS_BREAK, there's nothing to report
                 * and we keep going until the state changes or there's no more data.
                 */
                while (s->xlat_state == XS_BREAK && PS2KByteFromKbd(&s->Kbd, &val) == VINF_SUCCESS)
                {
                    s->xlat_state = XlateAT2PC(s->xlat_state, val, &xlated_val);
                    val = xlated_val;
                }
                /* This can happen if the last byte in the queue is F0... */
                if (s->xlat_state == XS_BREAK)
                    fHaveData = false;
            }
            if (fHaveData)
            {
                s->dbbout = val;
                s->status |= KBD_STAT_OBF;
            }
        }
        else if (!(s->mode & KBD_MODE_DISABLE_MOUSE) && PS2MByteFromAux(&s->Aux, &val) == VINF_SUCCESS)
        {
            s->dbbout = val;
            s->status |= KBD_STAT_OBF | KBD_STAT_MOUSE_OBF;
        }
    }
    /* Determine new IRQ state. */
    if (s->status & KBD_STAT_OBF) {
        if (s->status & KBD_STAT_MOUSE_OBF)
        {
            if (s->mode & KBD_MODE_MOUSE_INT)
                irq12_level = 1;
        }
        else
        {   /* KBD_STAT_OBF set but KBD_STAT_MOUSE_OBF isn't. */
            if (s->mode & KBD_MODE_KBD_INT)
                irq1_level = 1;
        }
    }
    PDMDevHlpISASetIrq(s->CTX_SUFF(pDevIns), 1, irq1_level);
    PDMDevHlpISASetIrq(s->CTX_SUFF(pDevIns), 12, irq12_level);
}

void KBCUpdateInterrupts(void *pKbc)
{
    KBDState    *s = (KBDState *)pKbc;
    kbd_update_irq(s);
}

static void kbc_dbb_out(void *opaque, uint8_t val)
{
    KBDState *s = (KBDState*)opaque;

    s->dbbout = val;
    /* Set the OBF and raise IRQ. */
    s->status |= KBD_STAT_OBF;
    if (s->mode & KBD_MODE_KBD_INT)
        PDMDevHlpISASetIrq(s->CTX_SUFF(pDevIns), 1, 1);
}

static void kbc_dbb_out_aux(void *opaque, uint8_t val)
{
    KBDState *s = (KBDState*)opaque;

    s->dbbout = val;
    /* Set the aux OBF and raise IRQ. */
    s->status |= KBD_STAT_OBF | KBD_STAT_MOUSE_OBF;
    if (s->mode & KBD_MODE_MOUSE_INT)
        PDMDevHlpISASetIrq(s->CTX_SUFF(pDevIns), 12, PDM_IRQ_LEVEL_HIGH);
}

static uint32_t kbd_read_status(void *opaque, uint32_t addr)
{
    KBDState *s = (KBDState*)opaque;
    int val = s->status;
    NOREF(addr);

#if defined(DEBUG_KBD)
    Log(("kbd: read status=0x%02x\n", val));
#endif
    return val;
}

static int kbd_write_command(void *opaque, uint32_t addr, uint32_t val)
{
    int rc = VINF_SUCCESS;
    KBDState *s = (KBDState*)opaque;
    NOREF(addr);

#ifdef DEBUG_KBD
    Log(("kbd: write cmd=0x%02x\n", val));
#endif
    switch(val) {
    case KBD_CCMD_READ_MODE:
        kbc_dbb_out(s, s->mode);
        break;
    case KBD_CCMD_WRITE_MODE:
    case KBD_CCMD_WRITE_OBUF:
    case KBD_CCMD_WRITE_AUX_OBUF:
    case KBD_CCMD_WRITE_MOUSE:
    case KBD_CCMD_WRITE_OUTPORT:
        s->write_cmd = val;
        break;
    case KBD_CCMD_MOUSE_DISABLE:
        s->mode |= KBD_MODE_DISABLE_MOUSE;
        break;
    case KBD_CCMD_MOUSE_ENABLE:
        s->mode &= ~KBD_MODE_DISABLE_MOUSE;
        /* Check for queued input. */
        kbd_update_irq(s);
        break;
    case KBD_CCMD_TEST_MOUSE:
        kbc_dbb_out(s, 0x00);
        break;
    case KBD_CCMD_SELF_TEST:
        /* Enable the A20 line - that is the power-on state(!). */
# ifndef IN_RING3
        if (!PDMDevHlpA20IsEnabled(s->CTX_SUFF(pDevIns)))
        {
            rc = VINF_IOM_R3_IOPORT_WRITE;
            break;
        }
# else /* IN_RING3 */
        PDMDevHlpA20Set(s->CTX_SUFF(pDevIns), true);
# endif /* IN_RING3 */
        s->status |= KBD_STAT_SELFTEST;
        s->mode |= KBD_MODE_DISABLE_KBD;
        kbc_dbb_out(s, 0x55);
        break;
    case KBD_CCMD_KBD_TEST:
        kbc_dbb_out(s, 0x00);
        break;
    case KBD_CCMD_KBD_DISABLE:
        s->mode |= KBD_MODE_DISABLE_KBD;
        break;
    case KBD_CCMD_KBD_ENABLE:
        s->mode &= ~KBD_MODE_DISABLE_KBD;
        /* Check for queued input. */
        kbd_update_irq(s);
        break;
    case KBD_CCMD_READ_INPORT:
        kbc_dbb_out(s, 0xBF);
        break;
    case KBD_CCMD_READ_OUTPORT:
        /* XXX: check that */
#ifdef TARGET_I386
        val = 0x01 | (PDMDevHlpA20IsEnabled(s->CTX_SUFF(pDevIns)) << 1);
#else
        val = 0x01;
#endif
        if (s->status & KBD_STAT_OBF)
            val |= 0x10;
        if (s->status & KBD_STAT_MOUSE_OBF)
            val |= 0x20;
        kbc_dbb_out(s, val);
        break;
#ifdef TARGET_I386
    case KBD_CCMD_ENABLE_A20:
# ifndef IN_RING3
        if (!PDMDevHlpA20IsEnabled(s->CTX_SUFF(pDevIns)))
            rc = VINF_IOM_R3_IOPORT_WRITE;
# else /* IN_RING3 */
        PDMDevHlpA20Set(s->CTX_SUFF(pDevIns), true);
# endif /* IN_RING3 */
        break;
    case KBD_CCMD_DISABLE_A20:
# ifndef IN_RING3
        if (PDMDevHlpA20IsEnabled(s->CTX_SUFF(pDevIns)))
            rc = VINF_IOM_R3_IOPORT_WRITE;
# else /* IN_RING3 */
        PDMDevHlpA20Set(s->CTX_SUFF(pDevIns), false);
# endif /* !IN_RING3 */
        break;
#endif
    case KBD_CCMD_READ_TSTINP:
        /* Keyboard clock line is zero IFF keyboard is disabled */
        val = (s->mode & KBD_MODE_DISABLE_KBD) ? 0 : 1;
        kbc_dbb_out(s, val);
        break;
    case KBD_CCMD_RESET:
    case KBD_CCMD_RESET_ALT:
#ifndef IN_RING3
        rc = VINF_IOM_R3_IOPORT_WRITE;
#else /* IN_RING3 */
        LogRel(("Reset initiated by keyboard controller\n"));
        rc = PDMDevHlpVMReset(s->CTX_SUFF(pDevIns), PDMVMRESET_F_KBD);
#endif /* !IN_RING3 */
        break;
    case 0xff:
        /* ignore that - I don't know what is its use */
        break;
    /* Make OS/2 happy. */
    /* The 8042 RAM is readable using commands 0x20 thru 0x3f, and writable
       by 0x60 thru 0x7f. Now days only the first byte, the mode, is used.
       We'll ignore the writes (0x61..7f) and return 0 for all the reads
       just to make some OS/2 debug stuff a bit happier. */
    case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
    case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
    case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
    case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
        kbc_dbb_out(s, 0);
        Log(("kbd: reading non-standard RAM addr %#x\n", val & 0x1f));
        break;
    default:
        Log(("kbd: unsupported keyboard cmd=0x%02x\n", val));
        break;
    }
    return rc;
}

static uint32_t kbd_read_data(void *opaque, uint32_t addr)
{
    KBDState *s = (KBDState*)opaque;
    uint32_t val;
    NOREF(addr);

    /* Return the current DBB contents. */
    val = s->dbbout;

    /* Reading the DBB deasserts IRQs... */
    if (s->status & KBD_STAT_MOUSE_OBF)
        PDMDevHlpISASetIrq(s->CTX_SUFF(pDevIns), 12, 0);
    else
        PDMDevHlpISASetIrq(s->CTX_SUFF(pDevIns), 1, 0);
    /* ...and clears the OBF bits. */
    s->status &= ~(KBD_STAT_OBF | KBD_STAT_MOUSE_OBF);

    /* Check if more data is available. */
    kbd_update_irq(s);
#ifdef DEBUG_KBD
    Log(("kbd: read data=0x%02x\n", val));
#endif
    return val;
}

PS2K *KBDGetPS2KFromDevIns(PPDMDEVINS pDevIns)
{
    KBDState *pThis = PDMINS_2_DATA(pDevIns, KBDState *);
    return &pThis->Kbd;
}

PS2M *KBDGetPS2MFromDevIns(PPDMDEVINS pDevIns)
{
    KBDState *pThis = PDMINS_2_DATA(pDevIns, KBDState *);
    return &pThis->Aux;
}

static int kbd_write_data(void *opaque, uint32_t addr, uint32_t val)
{
    int rc = VINF_SUCCESS;
    KBDState *s = (KBDState*)opaque;
    NOREF(addr);

#ifdef DEBUG_KBD
    Log(("kbd: write data=0x%02x\n", val));
#endif

    switch(s->write_cmd) {
    case 0:
        /* Automatically enables keyboard interface. */
        s->mode &= ~KBD_MODE_DISABLE_KBD;
        rc = PS2KByteToKbd(&s->Kbd, val);
        if (rc == VINF_SUCCESS)
            kbd_update_irq(s);
        break;
    case KBD_CCMD_WRITE_MODE:
        s->mode = val;
        s->translate = (s->mode & KBD_MODE_KCC) == KBD_MODE_KCC;
        kbd_update_irq(s);
        break;
    case KBD_CCMD_WRITE_OBUF:
        kbc_dbb_out(s, val);
        break;
    case KBD_CCMD_WRITE_AUX_OBUF:
        kbc_dbb_out_aux(s, val);
        break;
    case KBD_CCMD_WRITE_OUTPORT:
#ifdef TARGET_I386
#  ifndef IN_RING3
        if (PDMDevHlpA20IsEnabled(s->CTX_SUFF(pDevIns)) != !!(val & 2))
            rc = VINF_IOM_R3_IOPORT_WRITE;
#  else /* IN_RING3 */
        PDMDevHlpA20Set(s->CTX_SUFF(pDevIns), !!(val & 2));
#  endif /* !IN_RING3 */
#endif
        if (!(val & 1)) {
# ifndef IN_RING3
            rc = VINF_IOM_R3_IOPORT_WRITE;
# else
            rc = PDMDevHlpVMReset(s->CTX_SUFF(pDevIns), PDMVMRESET_F_KBD);
# endif
        }
        break;
    case KBD_CCMD_WRITE_MOUSE:
        /* Automatically enables aux interface. */
        s->mode &= ~KBD_MODE_DISABLE_MOUSE;
        rc = PS2MByteToAux(&s->Aux, val);
        if (rc == VINF_SUCCESS)
            kbd_update_irq(s);
        break;
    default:
        break;
    }
    if (rc != VINF_IOM_R3_IOPORT_WRITE)
        s->write_cmd = 0;
    return rc;
}

#ifdef IN_RING3

static void kbd_reset(void *opaque)
{
    KBDState *s = (KBDState*)opaque;
    s->mode = KBD_MODE_KBD_INT | KBD_MODE_MOUSE_INT;
    s->status = KBD_STAT_CMD | KBD_STAT_UNLOCKED;
    /* Resetting everything, keyword was not working right on NT4 reboot. */
    s->write_cmd = 0;
    s->translate = 0;
}

static void kbd_save(PSSMHANDLE pSSM, KBDState *s)
{
    SSMR3PutU8(pSSM, s->write_cmd);
    SSMR3PutU8(pSSM, s->status);
    SSMR3PutU8(pSSM, s->mode);
    SSMR3PutU8(pSSM, s->dbbout);

    /* terminator */
    SSMR3PutU32(pSSM, UINT32_MAX);
}

static int kbd_load(PSSMHANDLE pSSM, KBDState *s, uint32_t version_id)
{
    uint32_t    u32, i;
    uint8_t u8Dummy;
    uint32_t u32Dummy;
    int         rc;

#if 0
    /** @todo enable this and remove the "if (version_id == 4)" code at some
     * later time */
    /* Version 4 was never created by any publicly released version of VBox */
    AssertReturn(version_id != 4, VERR_NOT_SUPPORTED);
#endif
    if (version_id < 2 || version_id > PCKBD_SAVED_STATE_VERSION)
        return VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;
    SSMR3GetU8(pSSM, &s->write_cmd);
    SSMR3GetU8(pSSM, &s->status);
    SSMR3GetU8(pSSM, &s->mode);
    if (version_id <= 5)
    {
        SSMR3GetU32(pSSM, (uint32_t *)&u32Dummy);
        SSMR3GetU32(pSSM, (uint32_t *)&u32Dummy);
    }
    else
    {
        SSMR3GetU8(pSSM, &s->dbbout);
    }
    if (version_id <= 7)
    {
        int32_t     i32Dummy;
        uint8_t     u8State;
        uint8_t     u8Rate;
        uint8_t     u8Proto;

        SSMR3GetU32(pSSM, &u32Dummy);
        SSMR3GetU8(pSSM, &u8State);
        SSMR3GetU8(pSSM, &u8Dummy);
        SSMR3GetU8(pSSM, &u8Rate);
        SSMR3GetU8(pSSM, &u8Dummy);
        SSMR3GetU8(pSSM, &u8Proto);
        SSMR3GetU8(pSSM, &u8Dummy);
        SSMR3GetS32(pSSM, &i32Dummy);
        SSMR3GetS32(pSSM, &i32Dummy);
        SSMR3GetS32(pSSM, &i32Dummy);
        if (version_id > 2)
        {
            SSMR3GetS32(pSSM, &i32Dummy);
            SSMR3GetS32(pSSM, &i32Dummy);
        }
        rc = SSMR3GetU8(pSSM, &u8Dummy);
        if (version_id == 4)
        {
            SSMR3GetU32(pSSM, &u32Dummy);
            rc = SSMR3GetU32(pSSM, &u32Dummy);
        }
        if (version_id > 3)
            rc = SSMR3GetU8(pSSM, &u8Dummy);
        if (version_id == 4)
            rc = SSMR3GetU8(pSSM, &u8Dummy);
        AssertLogRelRCReturn(rc, rc);

        PS2MFixupState(&s->Aux, u8State, u8Rate, u8Proto);
    }

    /* Determine the translation state. */
    s->translate = (s->mode & KBD_MODE_KCC) == KBD_MODE_KCC;

    /*
     * Load the queues
     */
    if (version_id <= 5)
    {
        rc = SSMR3GetU32(pSSM, &u32);
        if (RT_FAILURE(rc))
            return rc;
        for (i = 0; i < u32; i++)
        {
            rc = SSMR3GetU8(pSSM, &u8Dummy);
            if (RT_FAILURE(rc))
                return rc;
        }
        Log(("kbd_load: %d keyboard queue items discarded from old saved state\n", u32));
    }

    if (version_id <= 7)
    {
        rc = SSMR3GetU32(pSSM, &u32);
        if (RT_FAILURE(rc))
            return rc;
        for (i = 0; i < u32; i++)
        {
            rc = SSMR3GetU8(pSSM, &u8Dummy);
            if (RT_FAILURE(rc))
                return rc;
        }
        Log(("kbd_load: %d mouse event queue items discarded from old saved state\n", u32));

        rc = SSMR3GetU32(pSSM, &u32);
        if (RT_FAILURE(rc))
            return rc;
        for (i = 0; i < u32; i++)
        {
            rc = SSMR3GetU8(pSSM, &u8Dummy);
            if (RT_FAILURE(rc))
                return rc;
        }
        Log(("kbd_load: %d mouse command queue items discarded from old saved state\n", u32));
    }

    /* terminator */
    rc = SSMR3GetU32(pSSM, &u32);
    if (RT_FAILURE(rc))
        return rc;
    if (u32 != ~0U)
    {
        AssertMsgFailed(("u32=%#x\n", u32));
        return VERR_SSM_DATA_UNIT_FORMAT_CHANGED;
    }
    return 0;
}
#endif /* IN_RING3 */


/* VirtualBox code start */

/* -=-=-=-=-=- wrappers -=-=-=-=-=- */

/**
 * Port I/O Handler for keyboard data IN operations.
 *
 * @returns VBox status code.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument - ignored.
 * @param   Port        Port number used for the IN operation.
 * @param   pu32        Where to store the result.
 * @param   cb          Number of bytes read.
 */
PDMBOTHCBDECL(int) kbdIOPortDataRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb)
{
    NOREF(pvUser);
    if (cb == 1)
    {
        KBDState *pThis = PDMINS_2_DATA(pDevIns, KBDState *);
        *pu32 = kbd_read_data(pThis, Port);
        Log2(("kbdIOPortDataRead: Port=%#x cb=%d *pu32=%#x\n", Port, cb, *pu32));
        return VINF_SUCCESS;
    }
    AssertMsgFailed(("Port=%#x cb=%d\n", Port, cb));
    return VERR_IOM_IOPORT_UNUSED;
}

/**
 * Port I/O Handler for keyboard data OUT operations.
 *
 * @returns VBox status code.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument - ignored.
 * @param   Port        Port number used for the IN operation.
 * @param   u32         The value to output.
 * @param   cb          The value size in bytes.
 */
PDMBOTHCBDECL(int) kbdIOPortDataWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb)
{
    int rc = VINF_SUCCESS;
    NOREF(pvUser);
    if (cb == 1 || cb == 2)
    {
        KBDState *pThis = PDMINS_2_DATA(pDevIns, KBDState *);
        rc = kbd_write_data(pThis, Port, (uint8_t)u32);
        Log2(("kbdIOPortDataWrite: Port=%#x cb=%d u32=%#x\n", Port, cb, u32));
    }
    else
        AssertMsgFailed(("Port=%#x cb=%d\n", Port, cb));
    return rc;
}

/**
 * Port I/O Handler for keyboard status IN operations.
 *
 * @returns VBox status code.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument - ignored.
 * @param   Port        Port number used for the IN operation.
 * @param   pu32        Where to store the result.
 * @param   cb          Number of bytes read.
 */
PDMBOTHCBDECL(int) kbdIOPortStatusRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb)
{
    uint16_t    fluff = 0;
    KBDState    *pThis = PDMINS_2_DATA(pDevIns, KBDState *);

    NOREF(pvUser);
    switch (cb) {
    case 2:
        fluff = 0xff00;
        RT_FALL_THRU();
    case 1:
        *pu32 = fluff | kbd_read_status(pThis, Port);
        Log2(("kbdIOPortStatusRead: Port=%#x cb=%d -> *pu32=%#x\n", Port, cb, *pu32));
        return VINF_SUCCESS;
    default:
        AssertMsgFailed(("Port=%#x cb=%d\n", Port, cb));
        return VERR_IOM_IOPORT_UNUSED;
    }
}

/**
 * Port I/O Handler for keyboard command OUT operations.
 *
 * @returns VBox status code.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument - ignored.
 * @param   Port        Port number used for the IN operation.
 * @param   u32         The value to output.
 * @param   cb          The value size in bytes.
 */
PDMBOTHCBDECL(int) kbdIOPortCommandWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb)
{
    int rc = VINF_SUCCESS;
    NOREF(pvUser);
    if (cb == 1 || cb == 2)
    {
        KBDState *pThis = PDMINS_2_DATA(pDevIns, KBDState *);
        rc = kbd_write_command(pThis, Port, (uint8_t)u32);
        Log2(("kbdIOPortCommandWrite: Port=%#x cb=%d u32=%#x rc=%Rrc\n", Port, cb, u32, rc));
    }
    else
        AssertMsgFailed(("Port=%#x cb=%d\n", Port, cb));
    return rc;
}

#ifdef IN_RING3

/**
 * Saves a state of the keyboard device.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance.
 * @param   pSSM  The handle to save the state to.
 */
static DECLCALLBACK(int) kbdSaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    KBDState    *pThis = PDMINS_2_DATA(pDevIns, KBDState *);
    kbd_save(pSSM, pThis);
    PS2KSaveState(&pThis->Kbd, pSSM);
    PS2MSaveState(&pThis->Aux, pSSM);
    return VINF_SUCCESS;
}


/**
 * Loads a saved keyboard device state.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance.
 * @param   pSSM  The handle to the saved state.
 * @param   uVersion    The data unit version number.
 * @param   uPass       The data pass.
 */
static DECLCALLBACK(int) kbdLoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    KBDState    *pThis = PDMINS_2_DATA(pDevIns, KBDState *);
    int rc;

    Assert(uPass == SSM_PASS_FINAL); NOREF(uPass);
    rc = kbd_load(pSSM, pThis, uVersion);
    if (uVersion >= 6)
        rc = PS2KLoadState(&pThis->Kbd, pSSM, uVersion);
    if (uVersion >= 8)
        rc = PS2MLoadState(&pThis->Aux, pSSM, uVersion);
    return rc;
}

/**
 * @callback_method_impl{FNSSMDEVLOADDONE, Key state fix-up after loading}
 */
static DECLCALLBACK(int) kbdLoadDone(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    KBDState    *pThis = PDMINS_2_DATA(pDevIns, KBDState *);
    int rc;

    rc = PS2KLoadDone(&pThis->Kbd, pSSM);
    return rc;
}

/**
 * Reset notification.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance data.
 */
static DECLCALLBACK(void)  kbdReset(PPDMDEVINS pDevIns)
{
    KBDState   *pThis = PDMINS_2_DATA(pDevIns, KBDState *);

    kbd_reset(pThis);
    PS2KReset(&pThis->Kbd);
    PS2MReset(&pThis->Aux);
}


/* -=-=-=-=-=- real code -=-=-=-=-=- */


/**
 * Attach command.
 *
 * This is called to let the device attach to a driver for a specified LUN
 * during runtime. This is not called during VM construction, the device
 * constructor have to attach to all the available drivers.
 *
 * This is like plugging in the keyboard or mouse after turning on the PC.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance.
 * @param   iLUN        The logical unit which is being detached.
 * @param   fFlags      Flags, combination of the PDMDEVATT_FLAGS_* \#defines.
 * @remark  The keyboard controller doesn't support this action, this is just
 *          implemented to try out the driver<->device structure.
 */
static DECLCALLBACK(int)  kbdAttach(PPDMDEVINS pDevIns, unsigned iLUN, uint32_t fFlags)
{
    int         rc;
    KBDState   *pThis = PDMINS_2_DATA(pDevIns, KBDState *);

    AssertMsgReturn(fFlags & PDM_TACH_FLAGS_NOT_HOT_PLUG,
                    ("PS/2 device does not support hotplugging\n"),
                    VERR_INVALID_PARAMETER);

    switch (iLUN)
    {
        /* LUN #0: keyboard */
        case 0:
            rc = PS2KAttach(&pThis->Kbd, pDevIns, iLUN, fFlags);
            if (RT_FAILURE(rc))
                return rc;
            break;

        /* LUN #1: aux/mouse */
        case 1:
            rc = PS2MAttach(&pThis->Aux, pDevIns, iLUN, fFlags);
            break;

        default:
            AssertMsgFailed(("Invalid LUN #%d\n", iLUN));
            return VERR_PDM_NO_SUCH_LUN;
    }

    return rc;
}


/**
 * Detach notification.
 *
 * This is called when a driver is detaching itself from a LUN of the device.
 * The device should adjust it's state to reflect this.
 *
 * This is like unplugging the network cable to use it for the laptop or
 * something while the PC is still running.
 *
 * @param   pDevIns     The device instance.
 * @param   iLUN        The logical unit which is being detached.
 * @param   fFlags      Flags, combination of the PDMDEVATT_FLAGS_* \#defines.
 * @remark  The keyboard controller doesn't support this action, this is just
 *          implemented to try out the driver<->device structure.
 */
static DECLCALLBACK(void)  kbdDetach(PPDMDEVINS pDevIns, unsigned iLUN, uint32_t fFlags)
{
#if 0
    /*
     * Reset the interfaces and update the controller state.
     */
    KBDState   *pThis = PDMINS_2_DATA(pDevIns, KBDState *);
    switch (iLUN)
    {
        /* LUN #0: keyboard */
        case 0:
            pThis->Keyboard.pDrv = NULL;
            pThis->Keyboard.pDrvBase = NULL;
            break;

        /* LUN #1: aux/mouse */
        case 1:
            pThis->Mouse.pDrv = NULL;
            pThis->Mouse.pDrvBase = NULL;
            break;

        default:
            AssertMsgFailed(("Invalid LUN #%d\n", iLUN));
            break;
    }
#else
    NOREF(pDevIns); NOREF(iLUN); NOREF(fFlags);
#endif
}


/**
 * @copydoc FNPDMDEVRELOCATE
 */
static DECLCALLBACK(void) kbdRelocate(PPDMDEVINS pDevIns, RTGCINTPTR offDelta)
{
    KBDState   *pThis = PDMINS_2_DATA(pDevIns, KBDState *);
    pThis->pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);
    PS2KRelocate(&pThis->Kbd, offDelta, pDevIns);
    PS2MRelocate(&pThis->Aux, offDelta, pDevIns);
}


/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) kbdConstruct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    KBDState   *pThis = PDMINS_2_DATA(pDevIns, KBDState *);
    int         rc;
    bool        fGCEnabled;
    bool        fR0Enabled;
    Assert(iInstance == 0);

    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    /*
     * Validate and read the configuration.
     */
    if (!CFGMR3AreValuesValid(pCfg, "GCEnabled\0R0Enabled\0"))
        return VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES;
    rc = CFGMR3QueryBoolDef(pCfg, "GCEnabled", &fGCEnabled, true);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Failed to query \"GCEnabled\" from the config"));
    rc = CFGMR3QueryBoolDef(pCfg, "R0Enabled", &fR0Enabled, true);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Failed to query \"R0Enabled\" from the config"));
    Log(("pckbd: fGCEnabled=%RTbool fR0Enabled=%RTbool\n", fGCEnabled, fR0Enabled));


    /*
     * Initialize the interfaces.
     */
    pThis->pDevInsR3 = pDevIns;
    pThis->pDevInsR0 = PDMDEVINS_2_R0PTR(pDevIns);
    pThis->pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);

    rc = PS2KConstruct(&pThis->Kbd, pDevIns, pThis, iInstance);
    if (RT_FAILURE(rc))
        return rc;

    rc = PS2MConstruct(&pThis->Aux, pDevIns, pThis, iInstance);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Register I/O ports, save state, keyboard event handler and mouse event handlers.
     */
    rc = PDMDevHlpIOPortRegister(pDevIns, 0x60, 1, NULL, kbdIOPortDataWrite,    kbdIOPortDataRead, NULL, NULL,   "PC Keyboard - Data");
    if (RT_FAILURE(rc))
        return rc;
    rc = PDMDevHlpIOPortRegister(pDevIns, 0x64, 1, NULL, kbdIOPortCommandWrite, kbdIOPortStatusRead, NULL, NULL, "PC Keyboard - Command / Status");
    if (RT_FAILURE(rc))
        return rc;
    if (fGCEnabled)
    {
        rc = PDMDevHlpIOPortRegisterRC(pDevIns, 0x60, 1, 0, "kbdIOPortDataWrite",    "kbdIOPortDataRead", NULL, NULL,   "PC Keyboard - Data");
        if (RT_FAILURE(rc))
            return rc;
        rc = PDMDevHlpIOPortRegisterRC(pDevIns, 0x64, 1, 0, "kbdIOPortCommandWrite", "kbdIOPortStatusRead", NULL, NULL, "PC Keyboard - Command / Status");
        if (RT_FAILURE(rc))
            return rc;
    }
    if (fR0Enabled)
    {
        rc = PDMDevHlpIOPortRegisterR0(pDevIns, 0x60, 1, 0, "kbdIOPortDataWrite",    "kbdIOPortDataRead", NULL, NULL,   "PC Keyboard - Data");
        if (RT_FAILURE(rc))
            return rc;
        rc = PDMDevHlpIOPortRegisterR0(pDevIns, 0x64, 1, 0, "kbdIOPortCommandWrite", "kbdIOPortStatusRead", NULL, NULL, "PC Keyboard - Command / Status");
        if (RT_FAILURE(rc))
            return rc;
    }
    rc = PDMDevHlpSSMRegisterEx(pDevIns, PCKBD_SAVED_STATE_VERSION, sizeof(*pThis), NULL,
                                NULL, NULL, NULL,
                                NULL, kbdSaveExec, NULL,
                                NULL, kbdLoadExec, kbdLoadDone);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Attach to the keyboard and mouse drivers.
     */
    rc = kbdAttach(pDevIns, 0 /* keyboard LUN # */, PDM_TACH_FLAGS_NOT_HOT_PLUG);
    if (RT_FAILURE(rc))
        return rc;
    rc = kbdAttach(pDevIns, 1 /* aux/mouse LUN # */, PDM_TACH_FLAGS_NOT_HOT_PLUG);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Initialize the device state.
     */
    kbdReset(pDevIns);

    return VINF_SUCCESS;
}


/**
 * The device registration structure.
 */
const PDMDEVREG g_DevicePS2KeyboardMouse =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "pckbd",
    /* szRCMod */
    "VBoxDDRC.rc",
    /* szR0Mod */
    "VBoxDDR0.r0",
    /* pszDescription */
    "PS/2 Keyboard and Mouse device. Emulates both the keyboard, mouse and the keyboard controller. "
    "LUN #0 is the keyboard connector. "
    "LUN #1 is the aux/mouse connector.",
    /* fFlags */
    PDM_DEVREG_FLAGS_HOST_BITS_DEFAULT | PDM_DEVREG_FLAGS_GUEST_BITS_32_64 | PDM_DEVREG_FLAGS_PAE36 | PDM_DEVREG_FLAGS_RC | PDM_DEVREG_FLAGS_R0,
    /* fClass */
    PDM_DEVREG_CLASS_INPUT,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(KBDState),
    /* pfnConstruct */
    kbdConstruct,
    /* pfnDestruct */
    NULL,
    /* pfnRelocate */
    kbdRelocate,
    /* pfnMemSetup */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    kbdReset,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    kbdAttach,
    /* pfnDetach */
    kbdDetach,
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

