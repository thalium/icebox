/* $Id: DevPIC.cpp $ */
/** @file
 * DevPIC - Intel 8259 Programmable Interrupt Controller (PIC) Device.
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
 * -------------------------------------------------------------------
 *
 * This code is based on:
 *
 * QEMU 8259 interrupt controller emulation
 *
 * Copyright (c) 2003-2004 Fabrice Bellard
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
#define LOG_GROUP LOG_GROUP_DEV_PIC
#include <VBox/vmm/pdmdev.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/string.h>

#include "VBoxDD.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** @def PIC_LOCK
 * Acquires the PDM lock. This is a NOP if locking is disabled. */
/** @def PIC_UNLOCK
 * Releases the PDM lock. This is a NOP if locking is disabled. */
#define PIC_LOCK(pThis, rc) \
    do { \
        int rc2 = (pThis)->CTX_SUFF(pPicHlp)->pfnLock((pThis)->CTX_SUFF(pDevIns), rc); \
        if (rc2 != VINF_SUCCESS) \
            return rc2; \
    } while (0)
#define PIC_UNLOCK(pThis) \
    (pThis)->CTX_SUFF(pPicHlp)->pfnUnlock((pThis)->CTX_SUFF(pDevIns))


/* debug PIC */
#define DEBUG_PIC

/*#define DEBUG_IRQ_COUNT*/

/**
 * The instance data of one (1) PIC.
 */
typedef struct PICSTATE
{
    uint8_t last_irr;       /**< edge detection */
    uint8_t irr;            /**< interrupt request register */
    uint8_t imr;            /**< interrupt mask register */
    uint8_t isr;            /**< interrupt service register */
    uint8_t priority_add;   /**< highest irq priority */
    uint8_t irq_base;
    uint8_t read_reg_select;
    uint8_t poll;
    uint8_t special_mask;
    uint8_t init_state;
    uint8_t auto_eoi;
    uint8_t rotate_on_auto_eoi;
    uint8_t special_fully_nested_mode;
    uint8_t init4;          /**< true if 4 byte init */
    uint8_t elcr;           /**< PIIX edge/trigger selection*/
    uint8_t elcr_mask;
    /** Pointer to the device instance, R3 Ptr. */
    PPDMDEVINSR3    pDevInsR3;
    /** Pointer to the device instance, R0 Ptr. */
    PPDMDEVINSR0    pDevInsR0;
    /** Pointer to the device instance, RC Ptr. */
    PPDMDEVINSRC    pDevInsRC;
    /** The PIC index (0 or 1). */
    uint8_t         idxPic;
    uint8_t         abAlignment0[3]; /**< Alignment padding. */
    /** The IRQ tags and source IDs for each (tracing purposes). */
    uint32_t        auTags[8];
} PICSTATE;
/** Pointer to the state of one PIC. */
typedef PICSTATE *PPICSTATE;


/**
 * The whole PIC device instance data.
 */
typedef struct DEVPIC
{
    /** The two interrupt controllers. */
    PICSTATE                aPics[2];
    /** Pointer to the device instance - R3 Ptr. */
    PPDMDEVINSR3            pDevInsR3;
    /** Pointer to the PIC R3 helpers. */
    PCPDMPICHLPR3           pPicHlpR3;
    /** Pointer to the device instance - R0 Ptr. */
    PPDMDEVINSR0            pDevInsR0;
    /** Pointer to the PIC R0 helpers. */
    PCPDMPICHLPR0           pPicHlpR0;
    /** Pointer to the device instance - RC Ptr. */
    PPDMDEVINSRC            pDevInsRC;
    /** Pointer to the PIC RC helpers. */
    PCPDMPICHLPRC           pPicHlpRC;
    /** Number of release log entries. Used to prevent flooding. */
    uint32_t                cRelLogEntries;
    uint32_t                u32AlignmentPadding;
#ifdef VBOX_WITH_STATISTICS
    STAMCOUNTER             StatSetIrqGC;
    STAMCOUNTER             StatSetIrqHC;
    STAMCOUNTER             StatClearedActiveIRQ2;
    STAMCOUNTER             StatClearedActiveMasterIRQ;
    STAMCOUNTER             StatClearedActiveSlaveIRQ;
#endif
} DEVPIC;
/** Pointer to the whole PIC instance data. */
typedef DEVPIC *PDEVPIC;


#ifndef VBOX_DEVICE_STRUCT_TESTCASE
#ifdef LOG_ENABLED
DECLINLINE(void) DumpPICState(PPICSTATE pPic, const char *pszFn)
{
    PDEVPIC pThis = PDMINS_2_DATA(pPic->CTX_SUFF(pDevIns), PDEVPIC);

    Log2(("%s: pic%d: elcr=%x last_irr=%x irr=%x imr=%x isr=%x irq_base=%x\n",
          pszFn, (&pThis->aPics[0] == pPic) ? 0 : 1,
          pPic->elcr, pPic->last_irr, pPic->irr, pPic->imr, pPic->isr, pPic->irq_base));
}
#else
# define DumpPICState(pThis, szFn) do { } while (0)
#endif

/* set irq level. If an edge is detected, then the IRR is set to 1 */
DECLINLINE(void) pic_set_irq1(PPICSTATE pPic, int irq, int level, uint32_t uTagSrc)
{
    Log(("pic_set_irq1: irq=%d level=%d\n", irq, level));
    int mask = 1 << irq;
    if (pPic->elcr & mask)
    {
        /* level triggered */
        if (level)
        {
            Log2(("pic_set_irq1(ls) irr=%d irrnew=%d\n", pPic->irr, pPic->irr | mask));
            pPic->irr |= mask;
            pPic->last_irr |= mask;
        }
        else
        {
            Log2(("pic_set_irq1(lc) irr=%d irrnew=%d\n", pPic->irr, pPic->irr & ~mask));
            pPic->irr &= ~mask;
            pPic->last_irr &= ~mask;
        }
    }
    else
    {
        /* edge triggered */
        if (level)
        {
            if ((pPic->last_irr & mask) == 0)
            {
                Log2(("pic_set_irq1 irr=%x last_irr=%x\n", pPic->irr | mask, pPic->last_irr));
                pPic->irr |= mask;
            }
            pPic->last_irr |= mask;
        }
        else
        {
            pPic->irr &= ~mask;
            pPic->last_irr &= ~mask;
        }
    }

    /* Save the tag. */
    if (level)
    {
        if (!pPic->auTags[irq])
            pPic->auTags[irq] = uTagSrc;
        else
            pPic->auTags[irq] |= RT_BIT_32(31);
    }

    DumpPICState(pPic, "pic_set_irq1");
}

/* return the highest priority found in mask (highest = smallest
   number). Return 8 if no irq */
DECLINLINE(int) get_priority(PPICSTATE pPic, int mask)
{
    int priority;
    if (mask == 0)
        return 8;
    priority = 0;
    while ((mask & (1 << ((priority + pPic->priority_add) & 7))) == 0)
        priority++;
    return priority;
}

/* return the pic wanted interrupt. return -1 if none */
static int pic_get_irq(PPICSTATE pPic)
{
    int mask, cur_priority, priority;
    Log(("pic_get_irq%d: mask=%x\n", pPic->idxPic, pPic->irr & ~pPic->imr));
    DumpPICState(pPic, "pic_get_irq");

    mask = pPic->irr & ~pPic->imr;
    priority = get_priority(pPic, mask);
    Log(("pic_get_irq: priority=%x\n", priority));
    if (priority == 8)
        return -1;
    /* compute current priority. If special fully nested mode on the
       master, the IRQ coming from the slave is not taken into account
       for the priority computation. */
    mask = pPic->isr;
    if (pPic->special_mask)
        mask &= ~pPic->imr;
    if (pPic->special_fully_nested_mode && pPic->idxPic == 0)
        mask &= ~(1 << 2);
    cur_priority = get_priority(pPic, mask);
    Log(("pic_get_irq%d: cur_priority=%x pending=%d\n", pPic->idxPic,
         cur_priority, (priority == 8) ? -1 : (priority + pPic->priority_add) & 7));
    if (priority < cur_priority)
    {
        /* higher priority found: an irq should be generated */
        return (priority + pPic->priority_add) & 7;
    }
    return -1;
}

/* raise irq to CPU if necessary. must be called every time the active
   irq may change */
static int pic_update_irq(PDEVPIC pThis)
{
    int irq2, irq;

    /* first look at slave pic */
    irq2 = pic_get_irq(&pThis->aPics[1]);
    Log(("pic_update_irq irq2=%d\n", irq2));
    if (irq2 >= 0)
    {
        /* if irq request by slave pic, signal master PIC */
        pic_set_irq1(&pThis->aPics[0], 2, 1, pThis->aPics[1].auTags[irq2]);
    }
    else
    {
        /* If not, clear the IR on the master PIC. */
        pic_set_irq1(&pThis->aPics[0], 2, 0, 0 /*uTagSrc*/);
    }
    /* look at requested irq */
    irq = pic_get_irq(&pThis->aPics[0]);
    if (irq >= 0)
    {
        /* If irq 2 is pending on the master pic, then there must be one pending on the slave pic too! Otherwise we'll get
         * spurious slave interrupts in picGetInterrupt.
         */
        if (irq != 2 || irq2 != -1)
        {
#if defined(DEBUG_PIC)
            for (int i = 0; i < 2; i++)
                Log(("pic%d: imr=%x irr=%x padd=%d\n", i, pThis->aPics[i].imr, pThis->aPics[i].irr, pThis->aPics[i].priority_add));
            Log(("pic: cpu_interrupt\n"));
#endif
            pThis->CTX_SUFF(pPicHlp)->pfnSetInterruptFF(pThis->CTX_SUFF(pDevIns));
        }
        else
        {
            STAM_COUNTER_INC(&pThis->StatClearedActiveIRQ2);
            Log(("pic_update_irq: irq 2 is active, but no interrupt is pending on the slave pic!!\n"));
            /* Clear it here, so lower priority interrupts can still be dispatched. */

            /* if this was the only pending irq, then we must clear the interrupt ff flag */
            pThis->CTX_SUFF(pPicHlp)->pfnClearInterruptFF(pThis->CTX_SUFF(pDevIns));

            /** @todo Is this correct? */
            pThis->aPics[0].irr &= ~(1 << 2);

            /* Call ourselves again just in case other interrupts are pending */
            return pic_update_irq(pThis);
        }
    }
    else
    {
        Log(("pic_update_irq: no interrupt is pending!!\n"));

        /* we must clear the interrupt ff flag */
        pThis->CTX_SUFF(pPicHlp)->pfnClearInterruptFF(pThis->CTX_SUFF(pDevIns));
    }
    return VINF_SUCCESS;
}

/**
 * Set the an IRQ.
 *
 * @param   pDevIns         Device instance of the PICs.
 * @param   iIrq            IRQ number to set.
 * @param   iLevel          IRQ level.
 * @param   uTagSrc         The IRQ tag and source ID (for tracing).
 */
PDMBOTHCBDECL(void) picSetIrq(PPDMDEVINS pDevIns, int iIrq, int iLevel, uint32_t uTagSrc)
{
    PDEVPIC     pThis = PDMINS_2_DATA(pDevIns, PDEVPIC);
    Assert(pThis->CTX_SUFF(pDevIns) == pDevIns);
    Assert(pThis->aPics[0].CTX_SUFF(pDevIns) == pDevIns);
    Assert(pThis->aPics[1].CTX_SUFF(pDevIns) == pDevIns);
    AssertMsg(iIrq < 16, ("iIrq=%d\n", iIrq));

    Log(("picSetIrq %d %d\n", iIrq, iLevel));
    DumpPICState(&pThis->aPics[0], "picSetIrq");
    DumpPICState(&pThis->aPics[1], "picSetIrq");
    STAM_COUNTER_INC(&pThis->CTXSUFF(StatSetIrq));
    if ((iLevel & PDM_IRQ_LEVEL_FLIP_FLOP) == PDM_IRQ_LEVEL_FLIP_FLOP)
    {
        /* A flip-flop lowers the IRQ line and immediately raises it, so
         * that a rising edge is guaranteed to occur. Note that the IRQ
         * line must be held high for a while to avoid spurious interrupts.
         */
        pic_set_irq1(&pThis->aPics[iIrq >> 3], iIrq & 7, 0, uTagSrc);
        pic_update_irq(pThis);
    }
    pic_set_irq1(&pThis->aPics[iIrq >> 3], iIrq & 7, iLevel & PDM_IRQ_LEVEL_HIGH, uTagSrc);
    pic_update_irq(pThis);
}


/* acknowledge interrupt 'irq' */
DECLINLINE(void) pic_intack(PPICSTATE pPic, int irq)
{
    if (pPic->auto_eoi)
    {
        if (pPic->rotate_on_auto_eoi)
            pPic->priority_add = (irq + 1) & 7;
    }
    else
        pPic->isr |= (1 << irq);

    /* We don't clear a level sensitive interrupt here */
    if (!(pPic->elcr & (1 << irq)))
    {
        Log2(("pic_intack: irr=%x irrnew=%x\n", pPic->irr, pPic->irr & ~(1 << irq)));
        pPic->irr &= ~(1 << irq);
    }
}


/**
 * Get a pending interrupt.
 *
 * @returns Pending interrupt number.
 * @param   pDevIns         Device instance of the PICs.
 * @param   puTagSrc        Where to return the IRQ tag and source ID.
 */
PDMBOTHCBDECL(int) picGetInterrupt(PPDMDEVINS pDevIns, uint32_t *puTagSrc)
{
    PDEVPIC     pThis = PDMINS_2_DATA(pDevIns, PDEVPIC);
    int         irq;
    int         irq2;
    int         intno;

    /* read the irq from the PIC */
    DumpPICState(&pThis->aPics[0], "picGetInterrupt");
    DumpPICState(&pThis->aPics[1], "picGetInterrupt");

    irq = pic_get_irq(&pThis->aPics[0]);
    if (irq >= 0)
    {
        pic_intack(&pThis->aPics[0], irq);
        if (irq == 2)
        {
            irq2 = pic_get_irq(&pThis->aPics[1]);
            if (irq2 >= 0)
                pic_intack(&pThis->aPics[1], irq2);
            else
            {
                /* Interrupt went away or is now masked. */
                Log(("picGetInterrupt: spurious IRQ on slave controller, converted to IRQ15\n"));
                irq2 = 7;
            }
            intno = pThis->aPics[1].irq_base + irq2;
            *puTagSrc = pThis->aPics[0].auTags[irq2];
            pThis->aPics[0].auTags[irq2] = 0;
            Log2(("picGetInterrupt1: %x base=%x irq=%x uTagSrc=%#x\n", intno, pThis->aPics[1].irq_base, irq2, *puTagSrc));
            irq = irq2 + 8;
        }
        else
        {
            intno = pThis->aPics[0].irq_base + irq;
            *puTagSrc = pThis->aPics[0].auTags[irq];
            pThis->aPics[0].auTags[irq] = 0;
            Log2(("picGetInterrupt0: %x base=%x irq=%x uTagSrc=%#x\n", intno, pThis->aPics[0].irq_base, irq, *puTagSrc));
        }
    }
    else
    {
        /* Interrupt went away or is now masked. */
        Log(("picGetInterrupt: spurious IRQ on master controller, converted to IRQ7\n"));
        irq = 7;
        intno = pThis->aPics[0].irq_base + irq;
        *puTagSrc = 0;
    }
    pic_update_irq(pThis);

    Log(("picGetInterrupt: 0x%02x pending 0:%d 1:%d\n", intno, pic_get_irq(&pThis->aPics[0]), pic_get_irq(&pThis->aPics[1])));

    return intno;
}

static void pic_reset(PPICSTATE pPic)
{
    PPDMDEVINSR3 pDevInsR3 = pPic->pDevInsR3;
    PPDMDEVINSR0 pDevInsR0 = pPic->pDevInsR0;
    PPDMDEVINSRC pDevInsRC = pPic->pDevInsRC;
    int elcr_mask = pPic->elcr_mask;
    int elcr = pPic->elcr;

    memset(pPic, 0, sizeof(*pPic));

    pPic->elcr = elcr;
    pPic->elcr_mask = elcr_mask;
    pPic->pDevInsRC = pDevInsRC;
    pPic->pDevInsR0 = pDevInsR0;
    pPic->pDevInsR3 = pDevInsR3;
}


static int pic_ioport_write(PDEVPIC pThis, PPICSTATE pPic, uint32_t addr, uint32_t val)
{
    int         rc = VINF_SUCCESS;
    int         irq;

    Log(("pic_write: addr=0x%02x val=0x%02x\n", addr, val));
    addr &= 1;
    if (addr == 0)
    {
        if (val & 0x10)
        {
            /* init */
            pic_reset(pPic);
            /* deassert a pending interrupt */
            pThis->CTX_SUFF(pPicHlp)->pfnClearInterruptFF(pThis->CTX_SUFF(pDevIns));

            pPic->init_state = 1;
            pPic->init4 = val & 1;
            if (val & 0x02)
                AssertReleaseMsgFailed(("single mode not supported"));
            if (val & 0x08)
                if (pThis->cRelLogEntries++ < 64)
                    LogRel(("pic_write: Level sensitive IRQ setting ignored.\n"));
        }
        else if (val & 0x08)
        {
            if (val & 0x04)
                pPic->poll = 1;
            if (val & 0x02)
                pPic->read_reg_select = val & 1;
            if (val & 0x40)
                pPic->special_mask = (val >> 5) & 1;
        }
        else
        {
            int cmd = val >> 5;
            switch (cmd)
            {
                case 0:
                case 4:
                    pPic->rotate_on_auto_eoi = cmd >> 2;
                    break;
                case 1: /* end of interrupt */
                case 5:
                {
                    int priority = get_priority(pPic, pPic->isr);
                    if (priority != 8) {
                        irq = (priority + pPic->priority_add) & 7;
                        Log(("pic_write: EOI prio=%d irq=%d\n", priority, irq));
                        pPic->isr &= ~(1 << irq);
                        if (cmd == 5)
                            pPic->priority_add = (irq + 1) & 7;
                        rc = pic_update_irq(pThis);
                        Assert(rc == VINF_SUCCESS);
                        DumpPICState(pPic, "eoi");
                    }
                    break;
                }
                case 3:
                {
                    irq = val & 7;
                    Log(("pic_write: EOI2 for irq %d\n", irq));
                    pPic->isr &= ~(1 << irq);
                    rc = pic_update_irq(pThis);
                    Assert(rc == VINF_SUCCESS);
                    DumpPICState(pPic, "eoi2");
                    break;
                }
                case 6:
                {
                    pPic->priority_add = (val + 1) & 7;
                    Log(("pic_write: lowest priority %d (highest %d)\n", val & 7, pPic->priority_add));
                    rc = pic_update_irq(pThis);
                    Assert(rc == VINF_SUCCESS);
                    break;
                }
                case 7:
                {
                    irq = val & 7;
                    Log(("pic_write: EOI3 for irq %d\n", irq));
                    pPic->isr &= ~(1 << irq);
                    pPic->priority_add = (irq + 1) & 7;
                    rc = pic_update_irq(pThis);
                    Assert(rc == VINF_SUCCESS);
                    DumpPICState(pPic, "eoi3");
                    break;
                }
                default:
                    /* no operation */
                    break;
            }
        }
    }
    else
    {
        switch (pPic->init_state)
        {
            case 0:
                /* normal mode */
                pPic->imr = val;
                rc = pic_update_irq(pThis);
                Assert(rc == VINF_SUCCESS);
                break;
            case 1:
                pPic->irq_base = val & 0xf8;
                pPic->init_state = 2;
                Log(("pic_write: set irq base to %x\n", pPic->irq_base));
                break;
            case 2:
                if (pPic->init4)
                    pPic->init_state = 3;
                else
                    pPic->init_state = 0;
                break;
            case 3:
                pPic->special_fully_nested_mode = (val >> 4) & 1;
                pPic->auto_eoi = (val >> 1) & 1;
                pPic->init_state = 0;
                Log(("pic_write: special_fully_nested_mode=%d auto_eoi=%d\n", pPic->special_fully_nested_mode, pPic->auto_eoi));
                break;
        }
    }
    return rc;
}


static uint32_t pic_poll_read(PPICSTATE pPic, uint32_t addr1)
{
    PDEVPIC pThis = RT_FROM_MEMBER(pPic, DEVPIC, aPics[pPic->idxPic]);

    int ret = pic_get_irq(pPic);
    if (ret >= 0)
    {
        if (addr1 >> 7)
        {
            Log2(("pic_poll_read: clear slave irq (isr)\n"));
            pThis->aPics[0].isr &= ~(1 << 2);
            pThis->aPics[0].irr &= ~(1 << 2);
        }
        Log2(("pic_poll_read: clear irq %d (isr)\n", ret));
        pPic->irr &= ~(1 << ret);
        pPic->isr &= ~(1 << ret);
        if (addr1 >> 7 || ret != 2)
            pic_update_irq(pThis);
    }
    else
    {
        ret = 0;
        pic_update_irq(pThis);
    }

    return ret;
}


static uint32_t pic_ioport_read(PPICSTATE pPic, uint32_t addr1, int *pRC)
{
    unsigned int addr;
    int ret;

    *pRC = VINF_SUCCESS;

    addr = addr1;
    addr &= 1;
    if (pPic->poll)
    {
        ret = pic_poll_read(pPic, addr1);
        pPic->poll = 0;
    }
    else
    {
        if (addr == 0)
        {
            if (pPic->read_reg_select)
                ret = pPic->isr;
            else
                ret = pPic->irr;
        }
        else
            ret = pPic->imr;
    }
    Log(("pic_read: addr=0x%02x val=0x%02x\n", addr1, ret));
    return ret;
}



/* -=-=-=-=-=- I/O ports -=-=-=-=-=- */

/**
 * @callback_method_impl{FNIOMIOPORTIN}
 */
PDMBOTHCBDECL(int) picIOPortRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint32_t *pu32, unsigned cb)
{
    PDEVPIC     pThis = PDMINS_2_DATA(pDevIns, PDEVPIC);
    uint32_t    iPic  = (uint32_t)(uintptr_t)pvUser;

    Assert(iPic == 0 || iPic == 1);
    if (cb == 1)
    {
        int rc;
        PIC_LOCK(pThis, VINF_IOM_R3_IOPORT_READ);
        *pu32 = pic_ioport_read(&pThis->aPics[iPic], uPort, &rc);
        PIC_UNLOCK(pThis);
        return rc;
    }
    return VERR_IOM_IOPORT_UNUSED;
}


/**
 * @callback_method_impl{FNIOMIOPORTOUT}
 */
PDMBOTHCBDECL(int) picIOPortWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint32_t u32, unsigned cb)
{
    PDEVPIC     pThis = PDMINS_2_DATA(pDevIns, PDEVPIC);
    uint32_t    iPic  = (uint32_t)(uintptr_t)pvUser;

    Assert(iPic == 0 || iPic == 1);

    if (cb == 1)
    {
        int rc;
        PIC_LOCK(pThis, VINF_IOM_R3_IOPORT_WRITE);
        rc = pic_ioport_write(pThis, &pThis->aPics[iPic], uPort, u32);
        PIC_UNLOCK(pThis);
        return rc;
    }
    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{FNIOMIOPORTIN, ELCR}
 */
PDMBOTHCBDECL(int) picIOPortElcrRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb)
{
    if (cb == 1)
    {
        PPICSTATE pPic = (PPICSTATE)pvUser;
        PIC_LOCK(PDMINS_2_DATA(pDevIns, PDEVPIC), VINF_IOM_R3_IOPORT_READ);
        *pu32 = pPic->elcr;
        PIC_UNLOCK(PDMINS_2_DATA(pDevIns, PDEVPIC));
        return VINF_SUCCESS;
    }
    NOREF(Port);
    return VERR_IOM_IOPORT_UNUSED;
}


/**
 * @callback_method_impl{FNIOMIOPORTOUT, ELCR}
 */
PDMBOTHCBDECL(int) picIOPortElcrWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb)
{
    if (cb == 1)
    {
        PPICSTATE pPic = (PPICSTATE)pvUser;
        PIC_LOCK(PDMINS_2_DATA(pDevIns, PDEVPIC), VINF_IOM_R3_IOPORT_WRITE);
        pPic->elcr = u32 & pPic->elcr_mask;
        PIC_UNLOCK(PDMINS_2_DATA(pDevIns, PDEVPIC));
    }
    NOREF(Port);
    return VINF_SUCCESS;
}


#ifdef IN_RING3

/**
 * @callback_method_impl{FNDBGFHANDLERDEV}
 */
static DECLCALLBACK(void) picInfo(PPDMDEVINS pDevIns, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    PDEVPIC pThis = PDMINS_2_DATA(pDevIns, PDEVPIC);
    NOREF(pszArgs);

    /*
     * Show info.
     */
    for (int i = 0; i < 2; i++)
    {
        PPICSTATE pPic = &pThis->aPics[i];

        pHlp->pfnPrintf(pHlp, "PIC%d:\n", i);
        pHlp->pfnPrintf(pHlp, " IMR :%02x ISR   :%02x IRR   :%02x LIRR:%02x\n",
                        pPic->imr, pPic->isr, pPic->irr, pPic->last_irr);
        pHlp->pfnPrintf(pHlp, " Base:%02x PriAdd:%02x RegSel:%02x\n",
                        pPic->irq_base, pPic->priority_add, pPic->read_reg_select);
        pHlp->pfnPrintf(pHlp, " Poll:%02x SpMask:%02x IState:%02x\n",
                        pPic->poll, pPic->special_mask, pPic->init_state);
        pHlp->pfnPrintf(pHlp, " AEOI:%02x Rotate:%02x FNest :%02x Ini4:%02x\n",
                        pPic->auto_eoi, pPic->rotate_on_auto_eoi,
                        pPic->special_fully_nested_mode, pPic->init4);
        pHlp->pfnPrintf(pHlp, " ELCR:%02x ELMask:%02x\n", pPic->elcr, pPic->elcr_mask);
    }
}


/* -=-=-=-=-=- Saved State -=-=-=-=-=- */

/**
 * @callback_method_impl{FNSSMDEVSAVEEXEC}
 */
static DECLCALLBACK(int) picSaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    PDEVPIC     pThis = PDMINS_2_DATA(pDevIns, PDEVPIC);
    for (unsigned i = 0; i < RT_ELEMENTS(pThis->aPics); i++)
    {
        SSMR3PutU8(pSSM, pThis->aPics[i].last_irr);
        SSMR3PutU8(pSSM, pThis->aPics[i].irr);
        SSMR3PutU8(pSSM, pThis->aPics[i].imr);
        SSMR3PutU8(pSSM, pThis->aPics[i].isr);
        SSMR3PutU8(pSSM, pThis->aPics[i].priority_add);
        SSMR3PutU8(pSSM, pThis->aPics[i].irq_base);
        SSMR3PutU8(pSSM, pThis->aPics[i].read_reg_select);
        SSMR3PutU8(pSSM, pThis->aPics[i].poll);
        SSMR3PutU8(pSSM, pThis->aPics[i].special_mask);
        SSMR3PutU8(pSSM, pThis->aPics[i].init_state);
        SSMR3PutU8(pSSM, pThis->aPics[i].auto_eoi);
        SSMR3PutU8(pSSM, pThis->aPics[i].rotate_on_auto_eoi);
        SSMR3PutU8(pSSM, pThis->aPics[i].special_fully_nested_mode);
        SSMR3PutU8(pSSM, pThis->aPics[i].init4);
        SSMR3PutU8(pSSM, pThis->aPics[i].elcr);
    }
    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{FNSSMDEVLOADEXEC}
 */
static DECLCALLBACK(int) picLoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    PDEVPIC pThis = PDMINS_2_DATA(pDevIns, PDEVPIC);

    if (uVersion != 1)
        return VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;
    Assert(uPass == SSM_PASS_FINAL); NOREF(uPass);

    for (unsigned i = 0; i < RT_ELEMENTS(pThis->aPics); i++)
    {
        SSMR3GetU8(pSSM, &pThis->aPics[i].last_irr);
        SSMR3GetU8(pSSM, &pThis->aPics[i].irr);
        SSMR3GetU8(pSSM, &pThis->aPics[i].imr);
        SSMR3GetU8(pSSM, &pThis->aPics[i].isr);
        SSMR3GetU8(pSSM, &pThis->aPics[i].priority_add);
        SSMR3GetU8(pSSM, &pThis->aPics[i].irq_base);
        SSMR3GetU8(pSSM, &pThis->aPics[i].read_reg_select);
        SSMR3GetU8(pSSM, &pThis->aPics[i].poll);
        SSMR3GetU8(pSSM, &pThis->aPics[i].special_mask);
        SSMR3GetU8(pSSM, &pThis->aPics[i].init_state);
        SSMR3GetU8(pSSM, &pThis->aPics[i].auto_eoi);
        SSMR3GetU8(pSSM, &pThis->aPics[i].rotate_on_auto_eoi);
        SSMR3GetU8(pSSM, &pThis->aPics[i].special_fully_nested_mode);
        SSMR3GetU8(pSSM, &pThis->aPics[i].init4);
        SSMR3GetU8(pSSM, &pThis->aPics[i].elcr);
    }
    return VINF_SUCCESS;
}


/* -=-=-=-=-=- PDMDEVREG -=-=-=-=-=- */

/**
 * @interface_method_impl{PDMDEVREG,pfnReset}
 */
static DECLCALLBACK(void)  picReset(PPDMDEVINS pDevIns)
{
    PDEVPIC     pThis = PDMINS_2_DATA(pDevIns, PDEVPIC);
    unsigned    i;
    LogFlow(("picReset:\n"));
    pThis->pPicHlpR3->pfnLock(pDevIns, VERR_INTERNAL_ERROR);

    for (i = 0; i < RT_ELEMENTS(pThis->aPics); i++)
        pic_reset(&pThis->aPics[i]);

    PIC_UNLOCK(pThis);
}


/**
 * @interface_method_impl{PDMDEVREG,pfnRelocate}
 */
static DECLCALLBACK(void) picRelocate(PPDMDEVINS pDevIns, RTGCINTPTR offDelta)
{
    RT_NOREF1(offDelta);
    PDEVPIC         pThis = PDMINS_2_DATA(pDevIns, PDEVPIC);
    unsigned        i;

    pThis->pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);
    pThis->pPicHlpRC = pThis->pPicHlpR3->pfnGetRCHelpers(pDevIns);
    for (i = 0; i < RT_ELEMENTS(pThis->aPics); i++)
        pThis->aPics[i].pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);
}


/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int)  picConstruct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);
    RT_NOREF1(iInstance);
    PDEVPIC         pThis = PDMINS_2_DATA(pDevIns, PDEVPIC);
    int             rc;
    bool            fGCEnabled;
    bool            fR0Enabled;
    Assert(iInstance == 0);

    /*
     * Validate and read configuration.
     */
    if (!CFGMR3AreValuesValid(pCfg, "GCEnabled\0" "R0Enabled\0"))
        return VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES;

    rc = CFGMR3QueryBoolDef(pCfg, "GCEnabled", &fGCEnabled, true);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: failed to read GCEnabled as boolean"));

    rc = CFGMR3QueryBoolDef(pCfg, "R0Enabled", &fR0Enabled, true);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: failed to read R0Enabled as boolean"));

    Log(("DevPIC: fGCEnabled=%RTbool fR0Enabled=%RTbool\n", fGCEnabled, fR0Enabled));

    /*
     * Init the data.
     */
    Assert(RT_ELEMENTS(pThis->aPics) == 2);
    pThis->pDevInsR3 = pDevIns;
    pThis->pDevInsR0 = PDMDEVINS_2_R0PTR(pDevIns);
    pThis->pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);
    pThis->aPics[0].elcr_mask = 0xf8;
    pThis->aPics[1].elcr_mask = 0xde;
    pThis->aPics[0].pDevInsR3 = pDevIns;
    pThis->aPics[1].pDevInsR3 = pDevIns;
    pThis->aPics[0].pDevInsR0 = PDMDEVINS_2_R0PTR(pDevIns);
    pThis->aPics[1].pDevInsR0 = PDMDEVINS_2_R0PTR(pDevIns);
    pThis->aPics[0].pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);
    pThis->aPics[1].pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);
    pThis->aPics[0].idxPic    = 0;
    pThis->aPics[1].idxPic    = 1;
    pThis->cRelLogEntries     = 0;

    /*
     * Register us as the PIC with PDM.
     */
    PDMPICREG PicReg;
    PicReg.u32Version           = PDM_PICREG_VERSION;
    PicReg.pfnSetIrqR3          = picSetIrq;
    PicReg.pfnGetInterruptR3    = picGetInterrupt;

    if (fGCEnabled)
    {
        PicReg.pszSetIrqRC          = "picSetIrq";
        PicReg.pszGetInterruptRC    = "picGetInterrupt";
    }
    else
    {
        PicReg.pszSetIrqRC          = NULL;
        PicReg.pszGetInterruptRC    = NULL;
    }

    if (fR0Enabled)
    {
        PicReg.pszSetIrqR0          = "picSetIrq";
        PicReg.pszGetInterruptR0    = "picGetInterrupt";
    }
    else
    {
        PicReg.pszSetIrqR0          = NULL;
        PicReg.pszGetInterruptR0    = NULL;
    }

    rc = PDMDevHlpPICRegister(pDevIns, &PicReg, &pThis->pPicHlpR3);
    AssertLogRelMsgRCReturn(rc, ("PICRegister -> %Rrc\n", rc), rc);
    if (fGCEnabled)
        pThis->pPicHlpRC = pThis->pPicHlpR3->pfnGetRCHelpers(pDevIns);
    if (fR0Enabled)
        pThis->pPicHlpR0 = pThis->pPicHlpR3->pfnGetR0Helpers(pDevIns);

    /*
     * Since the PIC helper interface provides access to the PDM lock,
     * we need no device level critical section.
     */
    rc = PDMDevHlpSetDeviceCritSect(pDevIns, PDMDevHlpCritSectGetNop(pDevIns));
    AssertRCReturn(rc, rc);

    /*
     * Register I/O ports and save state.
     */
    rc = PDMDevHlpIOPortRegister(pDevIns,  0x20, 2, (void *)0, picIOPortWrite, picIOPortRead, NULL, NULL, "i8259 PIC #0");
    if (RT_FAILURE(rc))
        return rc;
    rc = PDMDevHlpIOPortRegister(pDevIns,  0xa0, 2, (void *)1, picIOPortWrite, picIOPortRead, NULL, NULL, "i8259 PIC #1");
    if (RT_FAILURE(rc))
        return rc;
    if (fGCEnabled)
    {
        rc = PDMDevHlpIOPortRegisterRC(pDevIns,  0x20, 2, 0, "picIOPortWrite", "picIOPortRead", NULL, NULL, "i8259 PIC #0");
        if (RT_FAILURE(rc))
            return rc;
        rc = PDMDevHlpIOPortRegisterRC(pDevIns,  0xa0, 2, 1, "picIOPortWrite", "picIOPortRead", NULL, NULL, "i8259 PIC #1");
        if (RT_FAILURE(rc))
            return rc;
    }
    if (fR0Enabled)
    {
        rc = PDMDevHlpIOPortRegisterR0(pDevIns,  0x20, 2, 0, "picIOPortWrite", "picIOPortRead", NULL, NULL, "i8259 PIC #0");
        if (RT_FAILURE(rc))
            return rc;
        rc = PDMDevHlpIOPortRegisterR0(pDevIns,  0xa0, 2, 1, "picIOPortWrite", "picIOPortRead", NULL, NULL, "i8259 PIC #1");
        if (RT_FAILURE(rc))
            return rc;
    }

    rc = PDMDevHlpIOPortRegister(pDevIns, 0x4d0, 1, &pThis->aPics[0],
                                 picIOPortElcrWrite, picIOPortElcrRead, NULL, NULL, "i8259 PIC #0 - elcr");
    if (RT_FAILURE(rc))
        return rc;
    rc = PDMDevHlpIOPortRegister(pDevIns, 0x4d1, 1, &pThis->aPics[1],
                                 picIOPortElcrWrite, picIOPortElcrRead, NULL, NULL, "i8259 PIC #1 - elcr");
    if (RT_FAILURE(rc))
        return rc;
    if (fGCEnabled)
    {
        RTRCPTR pDataRC = PDMINS_2_DATA_RCPTR(pDevIns);
        rc = PDMDevHlpIOPortRegisterRC(pDevIns, 0x4d0, 1, pDataRC + RT_OFFSETOF(DEVPIC, aPics[0]),
                                       "picIOPortElcrWrite", "picIOPortElcrRead", NULL, NULL, "i8259 PIC #0 - elcr");
        if (RT_FAILURE(rc))
            return rc;
        rc = PDMDevHlpIOPortRegisterRC(pDevIns, 0x4d1, 1, pDataRC + RT_OFFSETOF(DEVPIC, aPics[1]),
                                       "picIOPortElcrWrite", "picIOPortElcrRead", NULL, NULL, "i8259 PIC #1 - elcr");
        if (RT_FAILURE(rc))
            return rc;
    }
    if (fR0Enabled)
    {
        RTR0PTR pDataR0 = PDMINS_2_DATA_R0PTR(pDevIns);
        rc = PDMDevHlpIOPortRegisterR0(pDevIns, 0x4d0, 1, pDataR0 + RT_OFFSETOF(DEVPIC, aPics[0]),
                                       "picIOPortElcrWrite", "picIOPortElcrRead", NULL, NULL, "i8259 PIC #0 - elcr");
        if (RT_FAILURE(rc))
            return rc;
        rc = PDMDevHlpIOPortRegisterR0(pDevIns, 0x4d1, 1, pDataR0 + RT_OFFSETOF(DEVPIC, aPics[1]),
                                       "picIOPortElcrWrite", "picIOPortElcrRead", NULL, NULL, "i8259 PIC #1 - elcr");
        if (RT_FAILURE(rc))
            return rc;
    }

    rc = PDMDevHlpSSMRegister(pDevIns, 1 /* uVersion */, sizeof(*pThis), picSaveExec, picLoadExec);
    if (RT_FAILURE(rc))
        return rc;


    /*
     * Register the info item.
     */
    PDMDevHlpDBGFInfoRegister(pDevIns, "pic", "PIC info.", picInfo);

    /*
     * Initialize the device state.
     */
    picReset(pDevIns);

#ifdef VBOX_WITH_STATISTICS
    /*
     * Statistics.
     */
    PDMDevHlpSTAMRegister(pDevIns, &pThis->StatSetIrqGC, STAMTYPE_COUNTER, "/Devices/PIC/SetIrqGC", STAMUNIT_OCCURENCES, "Number of PIC SetIrq calls in GC.");
    PDMDevHlpSTAMRegister(pDevIns, &pThis->StatSetIrqHC, STAMTYPE_COUNTER, "/Devices/PIC/SetIrqHC", STAMUNIT_OCCURENCES, "Number of PIC SetIrq calls in HC.");

    PDMDevHlpSTAMRegister(pDevIns, &pThis->StatClearedActiveIRQ2,       STAMTYPE_COUNTER, "/Devices/PIC/Masked/ActiveIRQ2",   STAMUNIT_OCCURENCES, "Number of cleared irq 2.");
    PDMDevHlpSTAMRegister(pDevIns, &pThis->StatClearedActiveMasterIRQ,  STAMTYPE_COUNTER, "/Devices/PIC/Masked/ActiveMaster", STAMUNIT_OCCURENCES, "Number of cleared master irqs.");
    PDMDevHlpSTAMRegister(pDevIns, &pThis->StatClearedActiveSlaveIRQ,   STAMTYPE_COUNTER, "/Devices/PIC/Masked/ActiveSlave",  STAMUNIT_OCCURENCES, "Number of cleared slave irqs.");
#endif

    return VINF_SUCCESS;
}


/**
 * The device registration structure.
 */
const PDMDEVREG g_DeviceI8259 =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "i8259",
    /* szRCMod */
    "VBoxDDRC.rc",
    /* szR0Mod */
    "VBoxDDR0.r0",
    /* pszDescription */
    "Intel 8259 Programmable Interrupt Controller (PIC) Device.",
    /* fFlags */
    PDM_DEVREG_FLAGS_HOST_BITS_DEFAULT | PDM_DEVREG_FLAGS_GUEST_BITS_32_64 | PDM_DEVREG_FLAGS_PAE36 | PDM_DEVREG_FLAGS_RC | PDM_DEVREG_FLAGS_R0,
    /* fClass */
    PDM_DEVREG_CLASS_PIC,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(DEVPIC),
    /* pfnConstruct */
    picConstruct,
    /* pfnDestruct */
    NULL,
    /* pfnRelocate */
    picRelocate,
    /* pfnMemSetup */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    picReset,
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

