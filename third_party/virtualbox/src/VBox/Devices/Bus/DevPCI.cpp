/* $Id: DevPCI.cpp $ */
/** @file
 * DevPCI - PCI BUS Device.
 *
 * @remarks New code shall be added to DevPciIch9.cpp as that will become
 *          the common PCI bus code soon.  Don't fix code in both DevPCI.cpp
 *          and DevPciIch9.cpp when it's possible to just make the latter
 *          version common.   Common code uses the 'devpci' prefix, is
 *          prototyped in DevPciInternal.h, and is defined in DevPciIch9.cpp.
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
 * --------------------------------------------------------------------
 *
 * This code is based on:
 *
 * QEMU PCI bus manager
 *
 * Copyright (c) 2004 Fabrice Bellard
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
#define LOG_GROUP LOG_GROUP_DEV_PCI
#define PDMPCIDEV_INCLUDE_PRIVATE  /* Hack to get pdmpcidevint.h included at the right point. */
#include <VBox/vmm/pdmpcidev.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/mm.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/string.h>

#include "PciInline.h"
#include "VBoxDD.h"
#include "DevPciInternal.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** Saved state version of the PCI bus device. */
#define VBOX_PCI_SAVED_STATE_VERSION                VBOX_PCI_SAVED_STATE_VERSION_REGION_SIZES
/** Adds I/O region types and sizes for dealing changes in resource regions. */
#define VBOX_PCI_SAVED_STATE_VERSION_REGION_SIZES   4
/** Before region sizes, the first named one.
 * Looking at the code though, we support even older version.  */
#define VBOX_PCI_SAVED_STATE_VERSION_IRQ_STATES     3
/** Notes whether we use the I/O APIC. */
#define VBOX_PCI_SAVED_STATE_VERSION_USE_IO_APIC    2


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
RT_C_DECLS_BEGIN

PDMBOTHCBDECL(void) pciSetIrq(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, int iIrq, int iLevel, uint32_t uTag);
PDMBOTHCBDECL(void) pcibridgeSetIrq(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, int iIrq, int iLevel, uint32_t uTag);
PDMBOTHCBDECL(int)  pciIOPortAddressWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb);
PDMBOTHCBDECL(int)  pciIOPortAddressRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb);
PDMBOTHCBDECL(int)  pciIOPortDataWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb);
PDMBOTHCBDECL(int)  pciIOPortDataRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb);

#ifdef IN_RING3
DECLINLINE(PPDMPCIDEV) pciR3FindBridge(PDEVPCIBUS pBus, uint8_t iBus);
#endif

RT_C_DECLS_END

#define DEBUG_PCI

#define PCI_VENDOR_ID       0x00    /* 16 bits */
#define PCI_DEVICE_ID       0x02    /* 16 bits */
#define PCI_COMMAND         0x04    /* 16 bits */
#define  PCI_COMMAND_IO     0x01    /* Enable response in I/O space */
#define  PCI_COMMAND_MEMORY 0x02    /* Enable response in Memory space */
#define PCI_CLASS_DEVICE    0x0a    /* Device class */
#define PCI_INTERRUPT_LINE  0x3c    /* 8 bits */
#define PCI_INTERRUPT_PIN   0x3d    /* 8 bits */
#define PCI_MIN_GNT         0x3e    /* 8 bits */
#define PCI_MAX_LAT         0x3f    /* 8 bits */


static int pci_data_write(PDEVPCIROOT pGlobals, uint32_t addr, uint32_t val, int len)
{
    uint8_t iBus, iDevice;
    uint32_t config_addr;

    LogFunc(("addr=%08x val=%08x len=%d\n", pGlobals->uConfigReg, val, len));

    if (!(pGlobals->uConfigReg & (1 << 31))) {
        return VINF_SUCCESS;
    }
    if ((pGlobals->uConfigReg & 0x3) != 0) {
        return VINF_SUCCESS;
    }
    iBus = (pGlobals->uConfigReg >> 16) & 0xff;
    iDevice = (pGlobals->uConfigReg >> 8) & 0xff;
    config_addr = (pGlobals->uConfigReg & 0xfc) | (addr & 3);
    if (iBus != 0)
    {
        if (pGlobals->PciBus.cBridges)
        {
#ifdef IN_RING3 /** @todo do lookup in R0/RC too! */
            PPDMPCIDEV pBridgeDevice = pciR3FindBridge(&pGlobals->PciBus, iBus);
            if (pBridgeDevice)
            {
                AssertPtr(pBridgeDevice->Int.s.pfnBridgeConfigWrite);
                pBridgeDevice->Int.s.pfnBridgeConfigWrite(pBridgeDevice->Int.s.CTX_SUFF(pDevIns), iBus, iDevice, config_addr, val, len);
            }
#else
            RT_NOREF2(val, len);
            return VINF_IOM_R3_IOPORT_WRITE;
#endif
        }
    }
    else
    {
        R3PTRTYPE(PDMPCIDEV *) pci_dev = pGlobals->PciBus.apDevices[iDevice];
        if (pci_dev)
        {
#ifdef IN_RING3
            LogFunc(("%s: addr=%02x val=%08x len=%d\n", pci_dev->pszNameR3, config_addr, val, len));
            pci_dev->Int.s.pfnConfigWrite(pci_dev->Int.s.CTX_SUFF(pDevIns), pci_dev, config_addr, val, len);
#else
            return VINF_IOM_R3_IOPORT_WRITE;
#endif
        }
    }
    return VINF_SUCCESS;
}

static int pci_data_read(PDEVPCIROOT pGlobals, uint32_t addr, int len, uint32_t *pu32)
{
    uint8_t iBus, iDevice;
    uint32_t config_addr;

    *pu32 = 0xffffffff;

    if (!(pGlobals->uConfigReg & (1 << 31)))
        return VINF_SUCCESS;
    if ((pGlobals->uConfigReg & 0x3) != 0)
        return VINF_SUCCESS;
    iBus = (pGlobals->uConfigReg >> 16) & 0xff;
    iDevice = (pGlobals->uConfigReg >> 8) & 0xff;
    config_addr = (pGlobals->uConfigReg & 0xfc) | (addr & 3);
    if (iBus != 0)
    {
        if (pGlobals->PciBus.cBridges)
        {
#ifdef IN_RING3 /** @todo do lookup in R0/RC too! */
            PPDMPCIDEV pBridgeDevice = pciR3FindBridge(&pGlobals->PciBus, iBus);
            if (pBridgeDevice)
            {
                AssertPtr(pBridgeDevice->Int.s.pfnBridgeConfigRead);
                *pu32 = pBridgeDevice->Int.s.pfnBridgeConfigRead(pBridgeDevice->Int.s.CTX_SUFF(pDevIns), iBus, iDevice, config_addr, len);
            }
#else
            NOREF(len);
            return VINF_IOM_R3_IOPORT_READ;
#endif
        }
    }
    else
    {
        R3PTRTYPE(PDMPCIDEV *) pci_dev = pGlobals->PciBus.apDevices[iDevice];
        if (pci_dev)
        {
#ifdef IN_RING3
            *pu32 = pci_dev->Int.s.pfnConfigRead(pci_dev->Int.s.CTX_SUFF(pDevIns), pci_dev, config_addr, len);
            LogFunc(("%s: addr=%02x val=%08x len=%d\n", pci_dev->pszNameR3, config_addr, *pu32, len));
#else
            NOREF(len);
            return VINF_IOM_R3_IOPORT_READ;
#endif
        }
    }

    return VINF_SUCCESS;
}



/* return the global irq number corresponding to a given device irq
   pin. We could also use the bus number to have a more precise
   mapping.
   This is the implementation note described in the PCI spec chapter 2.2.6 */
static inline int pci_slot_get_pirq(uint8_t uDevFn, int irq_num)
{
    int slot_addend;
    slot_addend = (uDevFn >> 3) - 1;
    return (irq_num + slot_addend) & 3;
}

static inline int pci_slot_get_apic_pirq(uint8_t uDevFn, int irq_num)
{
    return (irq_num + (uDevFn >> 3)) & 7;
}

static inline int get_pci_irq_apic_level(PDEVPCIROOT pGlobals, int irq_num)
{
    return (pGlobals->auPciApicIrqLevels[irq_num] != 0);
}

static void apic_set_irq(PDEVPCIBUS pBus, uint8_t uDevFn, PDMPCIDEV *pPciDev, int irq_num1, int iLevel, int iAcpiIrq, uint32_t uTagSrc)
{
    /* This is only allowed to be called with a pointer to the host bus. */
    AssertMsg(pBus->iBus == 0, ("iBus=%u\n", pBus->iBus));

    if (iAcpiIrq == -1) {
        int apic_irq, apic_level;
        PDEVPCIROOT pGlobals = DEVPCIBUS_2_DEVPCIROOT(pBus);
        int irq_num = pci_slot_get_apic_pirq(uDevFn, irq_num1);

        if ((iLevel & PDM_IRQ_LEVEL_HIGH) == PDM_IRQ_LEVEL_HIGH)
            ASMAtomicIncU32(&pGlobals->auPciApicIrqLevels[irq_num]);
        else if ((iLevel & PDM_IRQ_LEVEL_HIGH) == PDM_IRQ_LEVEL_LOW)
            ASMAtomicDecU32(&pGlobals->auPciApicIrqLevels[irq_num]);

        apic_irq = irq_num + 0x10;
        apic_level = get_pci_irq_apic_level(pGlobals, irq_num);
        Log3Func(("%s: irq_num1=%d level=%d apic_irq=%d apic_level=%d irq_num1=%d\n",
              R3STRING(pPciDev->pszNameR3), irq_num1, iLevel, apic_irq, apic_level, irq_num));
        pBus->CTX_SUFF(pPciHlp)->pfnIoApicSetIrq(pBus->CTX_SUFF(pDevIns), apic_irq, apic_level, uTagSrc);

        if ((iLevel & PDM_IRQ_LEVEL_FLIP_FLOP) == PDM_IRQ_LEVEL_FLIP_FLOP) {
            ASMAtomicDecU32(&pGlobals->auPciApicIrqLevels[irq_num]);
            pPciDev->Int.s.uIrqPinState = PDM_IRQ_LEVEL_LOW;
            apic_level = get_pci_irq_apic_level(pGlobals, irq_num);
            Log3Func(("%s: irq_num1=%d level=%d apic_irq=%d apic_level=%d irq_num1=%d (flop)\n",
                  R3STRING(pPciDev->pszNameR3), irq_num1, iLevel, apic_irq, apic_level, irq_num));
            pBus->CTX_SUFF(pPciHlp)->pfnIoApicSetIrq(pBus->CTX_SUFF(pDevIns), apic_irq, apic_level, uTagSrc);
        }
    } else {
        Log3Func(("%s: irq_num1=%d level=%d iAcpiIrq=%d\n",
              R3STRING(pPciDev->pszNameR3), irq_num1, iLevel, iAcpiIrq));
        pBus->CTX_SUFF(pPciHlp)->pfnIoApicSetIrq(pBus->CTX_SUFF(pDevIns), iAcpiIrq, iLevel, uTagSrc);
    }
}

DECLINLINE(int) get_pci_irq_level(PDEVPCIROOT pGlobals, int irq_num)
{
    return (pGlobals->Piix3.auPciLegacyIrqLevels[irq_num] != 0);
}

/**
 * Set the IRQ for a PCI device on the host bus - shared by host bus and bridge.
 *
 * @param   pGlobals        Device instance of the host PCI Bus.
 * @param   uDevFn          The device number on the host bus which will raise the IRQ
 * @param   pPciDev         The PCI device structure which raised the interrupt.
 * @param   iIrq            IRQ number to set.
 * @param   iLevel          IRQ level.
 * @param   uTagSrc         The IRQ tag and source ID (for tracing).
 * @remark  uDevFn and pPciDev->uDevFn are not the same if the device is behind
 *          a bridge. In that case uDevFn will be the slot of the bridge which
 *          is needed to calculate the PIRQ value.
 */
static void pciSetIrqInternal(PDEVPCIROOT pGlobals, uint8_t uDevFn, PPDMPCIDEV pPciDev, int iIrq, int iLevel, uint32_t uTagSrc)
{
    PDEVPCIBUS  pBus = &pGlobals->PciBus;
    uint8_t    *pbCfg = pGlobals->Piix3.PIIX3State.dev.abConfig;
    const bool  fIsAcpiDevice = pPciDev->abConfig[2] == 0x13 && pPciDev->abConfig[3] == 0x71;
    /* If the two configuration space bytes at 0xde, 0xad are set to 0xbe, 0xef, a back door
     * is opened to route PCI interrupts directly to the I/O APIC and bypass the PIC.
     * See the \_SB_.PCI0._PRT method in vbox.dsl.
     */
    const bool  fIsApicEnabled = pGlobals->fUseIoApic && pbCfg[0xde] == 0xbe && pbCfg[0xad] == 0xef;
    int pic_irq, pic_level;

    /* Check if the state changed. */
    if (pPciDev->Int.s.uIrqPinState != iLevel)
    {
        pPciDev->Int.s.uIrqPinState = (iLevel & PDM_IRQ_LEVEL_HIGH);

        /* Send interrupt to I/O APIC only. */
        if (fIsApicEnabled)
        {
            if (fIsAcpiDevice)
                /*
                 * ACPI needs special treatment since SCI is hardwired and
                 * should not be affected by PCI IRQ routing tables at the
                 * same time SCI IRQ is shared in PCI sense hence this
                 * kludge (i.e. we fetch the hardwired value from ACPIs
                 * PCI device configuration space).
                 */
                apic_set_irq(pBus, uDevFn, pPciDev, -1, iLevel, pPciDev->abConfig[PCI_INTERRUPT_LINE], uTagSrc);
            else
                apic_set_irq(pBus, uDevFn, pPciDev, iIrq, iLevel, -1, uTagSrc);
            return;
        }

        if (fIsAcpiDevice)
        {
            /* As per above treat ACPI in a special way */
            pic_irq = pPciDev->abConfig[PCI_INTERRUPT_LINE];
            pGlobals->Piix3.iAcpiIrq = pic_irq;
            pGlobals->Piix3.iAcpiIrqLevel = iLevel & PDM_IRQ_LEVEL_HIGH;
        }
        else
        {
            int irq_num;
            irq_num = pci_slot_get_pirq(uDevFn, iIrq);

            if (pPciDev->Int.s.uIrqPinState == PDM_IRQ_LEVEL_HIGH)
                ASMAtomicIncU32(&pGlobals->Piix3.auPciLegacyIrqLevels[irq_num]);
            else if (pPciDev->Int.s.uIrqPinState == PDM_IRQ_LEVEL_LOW)
                ASMAtomicDecU32(&pGlobals->Piix3.auPciLegacyIrqLevels[irq_num]);

            /* now we change the pic irq level according to the piix irq mappings */
            pic_irq = pbCfg[0x60 + irq_num];
            if (pic_irq >= 16)
            {
                if ((iLevel & PDM_IRQ_LEVEL_FLIP_FLOP) == PDM_IRQ_LEVEL_FLIP_FLOP)
                {
                    ASMAtomicDecU32(&pGlobals->Piix3.auPciLegacyIrqLevels[irq_num]);
                    pPciDev->Int.s.uIrqPinState = PDM_IRQ_LEVEL_LOW;
                }

                return;
            }
        }

        /* the pic level is the logical OR of all the PCI irqs mapped to it */
        pic_level = 0;
        if (pic_irq == pbCfg[0x60])
            pic_level |= get_pci_irq_level(pGlobals, 0);
        if (pic_irq == pbCfg[0x61])
            pic_level |= get_pci_irq_level(pGlobals, 1);
        if (pic_irq == pbCfg[0x62])
            pic_level |= get_pci_irq_level(pGlobals, 2);
        if (pic_irq == pbCfg[0x63])
            pic_level |= get_pci_irq_level(pGlobals, 3);
        if (pic_irq == pGlobals->Piix3.iAcpiIrq)
            pic_level |= pGlobals->Piix3.iAcpiIrqLevel;

        Log3Func(("%s: iLevel=%d iIrq=%d pic_irq=%d pic_level=%d uTagSrc=%#x\n",
              R3STRING(pPciDev->pszNameR3), iLevel, iIrq, pic_irq, pic_level, uTagSrc));
        pBus->CTX_SUFF(pPciHlp)->pfnIsaSetIrq(pBus->CTX_SUFF(pDevIns), pic_irq, pic_level, uTagSrc);

        /** @todo optimize pci irq flip-flop some rainy day. */
        if ((iLevel & PDM_IRQ_LEVEL_FLIP_FLOP) == PDM_IRQ_LEVEL_FLIP_FLOP)
            pciSetIrqInternal(pGlobals, uDevFn, pPciDev, iIrq, PDM_IRQ_LEVEL_LOW, uTagSrc);
    }
}


/**
 * @interface_method_impl{PDMPCIBUSREG,pfnSetIrqR3}
 */
PDMBOTHCBDECL(void) pciSetIrq(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, int iIrq, int iLevel, uint32_t uTagSrc)
{
    pciSetIrqInternal(PDMINS_2_DATA(pDevIns, PDEVPCIROOT), pPciDev->uDevFn, pPciDev, iIrq, iLevel, uTagSrc);
}

#ifdef IN_RING3

/**
 * Finds a bridge on the bus which contains the destination bus.
 *
 * @return Pointer to the device instance data of the bus or
 *         NULL if no bridge was found.
 * @param  pBus    Pointer to the bus to search on.
 * @param  iBus    Destination bus number.
 */
DECLINLINE(PPDMPCIDEV) pciR3FindBridge(PDEVPCIBUS pBus, uint8_t iBus)
{
    /* Search for a fitting bridge. */
    for (uint32_t iBridge = 0; iBridge < pBus->cBridges; iBridge++)
    {
        /*
         * Examine secondary and subordinate bus number.
         * If the target bus is in the range we pass the request on to the bridge.
         */
        PPDMPCIDEV pBridgeTemp = pBus->papBridgesR3[iBridge];
        AssertMsg(pBridgeTemp && pciDevIsPci2PciBridge(pBridgeTemp),
                  ("Device is not a PCI bridge but on the list of PCI bridges\n"));

        if (   iBus >= pBridgeTemp->abConfig[VBOX_PCI_SECONDARY_BUS]
            && iBus <= pBridgeTemp->abConfig[VBOX_PCI_SUBORDINATE_BUS])
            return pBridgeTemp;
    }

    /* Nothing found. */
    return NULL;
}

static void pciR3Piix3Reset(PIIX3ISABRIDGE *d)
{
    uint8_t *pci_conf = d->dev.abConfig;

    pci_conf[0x04] = 0x07; /* master, memory and I/O */
    pci_conf[0x05] = 0x00;
    pci_conf[0x06] = 0x00;
    pci_conf[0x07] = 0x02; /* PCI_status_devsel_medium */
    pci_conf[0x4c] = 0x4d;
    pci_conf[0x4e] = 0x03;
    pci_conf[0x4f] = 0x00;
    pci_conf[0x60] = 0x80;
    pci_conf[0x69] = 0x02;
    pci_conf[0x70] = 0x80;
    pci_conf[0x76] = 0x0c;
    pci_conf[0x77] = 0x0c;
    pci_conf[0x78] = 0x02;
    pci_conf[0x79] = 0x00;
    pci_conf[0x80] = 0x00;
    pci_conf[0x82] = 0x02; /* Get rid of the Linux guest "Enabling Passive Release" PCI quirk warning. */
    pci_conf[0xa0] = 0x08;
    pci_conf[0xa2] = 0x00;
    pci_conf[0xa3] = 0x00;
    pci_conf[0xa4] = 0x00;
    pci_conf[0xa5] = 0x00;
    pci_conf[0xa6] = 0x00;
    pci_conf[0xa7] = 0x00;
    pci_conf[0xa8] = 0x0f;
    pci_conf[0xaa] = 0x00;
    pci_conf[0xab] = 0x00;
    pci_conf[0xac] = 0x00;
    pci_conf[0xae] = 0x00;
}

/* host irqs corresponding to PCI irqs A-D */
static const uint8_t pci_irqs[4] = { 11, 10, 9, 11 };   /* bird: added const */

static void pci_bios_init_device(PDEVPCIROOT pGlobals, PDEVPCIBUS pBus, PPDMPCIDEV pPciDev, uint8_t cBridgeDepth, uint8_t *paBridgePositions)
{
    uint32_t *paddr;
    int pin, pic_irq;
    uint16_t devclass, vendor_id, device_id;

    devclass  = devpciR3GetWord(pPciDev, PCI_CLASS_DEVICE);
    vendor_id = devpciR3GetWord(pPciDev, PCI_VENDOR_ID);
    device_id = devpciR3GetWord(pPciDev, PCI_DEVICE_ID);

    /* Check if device is present. */
    if (vendor_id != 0xffff)
    {
        switch(devclass)
        {
            case 0x0101:
                if (   (vendor_id == 0x8086)
                    && (device_id == 0x7010 || device_id == 0x7111 || device_id == 0x269e))
                {
                    /* PIIX3, PIIX4 or ICH6 IDE */
                    devpciR3SetWord(pPciDev, 0x40, 0x8000); /* enable IDE0 */
                    devpciR3SetWord(pPciDev, 0x42, 0x8000); /* enable IDE1 */
                    goto default_map;
                }
                else
                {
                    /* IDE: we map it as in ISA mode */
                    devpciR3BiosInitSetRegionAddress(pBus, pPciDev, 0, 0x1f0);
                    devpciR3BiosInitSetRegionAddress(pBus, pPciDev, 1, 0x3f4);
                    devpciR3BiosInitSetRegionAddress(pBus, pPciDev, 2, 0x170);
                    devpciR3BiosInitSetRegionAddress(pBus, pPciDev, 3, 0x374);
                    devpciR3SetWord(pPciDev, PCI_COMMAND,
                                      devpciR3GetWord(pPciDev, PCI_COMMAND)
                                    | PCI_COMMAND_IOACCESS);
                }
                break;
            case 0x0300:
                if (vendor_id != 0x80ee)
                    goto default_map;
                /* VGA: map frame buffer to default Bochs VBE address */
                devpciR3BiosInitSetRegionAddress(pBus, pPciDev, 0, 0xe0000000);
                /*
                 * Legacy VGA I/O ports are implicitly decoded by a VGA class device. But
                 * only the framebuffer (i.e., a memory region) is explicitly registered via
                 * devpciR3BiosInitSetRegionAddress, so don't forget to enable I/O decoding.
                 */
                devpciR3SetWord(pPciDev, PCI_COMMAND,
                                  devpciR3GetWord(pPciDev, PCI_COMMAND)
                                | PCI_COMMAND_IOACCESS | PCI_COMMAND_MEMACCESS);
                break;
            case 0x0800:
                /* PIC */
                vendor_id = devpciR3GetWord(pPciDev, PCI_VENDOR_ID);
                device_id = devpciR3GetWord(pPciDev, PCI_DEVICE_ID);
                if (vendor_id == 0x1014)
                {
                    /* IBM */
                    if (device_id == 0x0046 || device_id == 0xFFFF)
                    {
                        /* MPIC & MPIC2 */
                        devpciR3BiosInitSetRegionAddress(pBus, pPciDev, 0, 0x80800000 + 0x00040000);
                        devpciR3SetWord(pPciDev, PCI_COMMAND,
                                          devpciR3GetWord(pPciDev, PCI_COMMAND)
                                        | PCI_COMMAND_MEMACCESS);
                    }
                }
                break;
            case 0xff00:
                if (   (vendor_id == 0x0106b)
                    && (device_id == 0x0017 || device_id == 0x0022))
                {
                    /* macio bridge */
                    devpciR3BiosInitSetRegionAddress(pBus, pPciDev, 0, 0x80800000);
                    devpciR3SetWord(pPciDev, PCI_COMMAND,
                                      devpciR3GetWord(pPciDev, PCI_COMMAND)
                                    | PCI_COMMAND_MEMACCESS);
                }
                break;
            case 0x0604:
            {
                /* Init PCI-to-PCI bridge. */
                devpciR3SetByte(pPciDev, VBOX_PCI_PRIMARY_BUS, pBus->iBus);

                AssertMsg(pGlobals->uPciBiosBus < 255, ("Too many bridges on the bus\n"));
                pGlobals->uPciBiosBus++;
                devpciR3SetByte(pPciDev, VBOX_PCI_SECONDARY_BUS, pGlobals->uPciBiosBus);
                devpciR3SetByte(pPciDev, VBOX_PCI_SUBORDINATE_BUS, 0xff); /* Temporary until we know how many other bridges are behind this one. */

                /* Add position of this bridge into the array. */
                paBridgePositions[cBridgeDepth+1] = (pPciDev->uDevFn >> 3);

                /*
                 * The I/O range for the bridge must be aligned to a 4KB boundary.
                 * This does not change anything really as the access to the device is not going
                 * through the bridge but we want to be compliant to the spec.
                 */
                if ((pGlobals->uPciBiosIo % _4K) != 0)
                    pGlobals->uPciBiosIo = RT_ALIGN_32(pGlobals->uPciBiosIo, _4K);
                LogFunc(("Aligned I/O start address. New address %#x\n", pGlobals->uPciBiosIo));
                devpciR3SetByte(pPciDev, VBOX_PCI_IO_BASE, (pGlobals->uPciBiosIo >> 8) & 0xf0);

                /* The MMIO range for the bridge must be aligned to a 1MB boundary. */
                if ((pGlobals->uPciBiosMmio % _1M) != 0)
                    pGlobals->uPciBiosMmio = RT_ALIGN_32(pGlobals->uPciBiosMmio, _1M);
                LogFunc(("Aligned MMIO start address. New address %#x\n", pGlobals->uPciBiosMmio));
                devpciR3SetWord(pPciDev, VBOX_PCI_MEMORY_BASE, (pGlobals->uPciBiosMmio >> 16) & UINT32_C(0xffff0));

                /* Save values to compare later to. */
                uint32_t u32IoAddressBase = pGlobals->uPciBiosIo;
                uint32_t u32MMIOAddressBase = pGlobals->uPciBiosMmio;

                /* Init devices behind the bridge and possibly other bridges as well. */
                PDEVPCIBUS pChildBus = PDMINS_2_DATA(pPciDev->Int.s.CTX_SUFF(pDevIns), PDEVPCIBUS);
                for (uint32_t uDevFn = 0; uDevFn < RT_ELEMENTS(pChildBus->apDevices); uDevFn++)
                {
                    PPDMPCIDEV pChildPciDev = pChildBus->apDevices[uDevFn];
                    if (pChildPciDev)
                        pci_bios_init_device(pGlobals, pChildBus, pChildPciDev, cBridgeDepth + 1, paBridgePositions);
                }

                /* The number of bridges behind the this one is now available. */
                devpciR3SetByte(pPciDev, VBOX_PCI_SUBORDINATE_BUS, pGlobals->uPciBiosBus);

                /*
                 * Set I/O limit register. If there is no device with I/O space behind the bridge
                 * we set a lower value than in the base register.
                 * The result with a real bridge is that no I/O transactions are passed to the secondary
                 * interface. Again this doesn't really matter here but we want to be compliant to the spec.
                 */
                if ((u32IoAddressBase != pGlobals->uPciBiosIo) && ((pGlobals->uPciBiosIo % _4K) != 0))
                {
                    /* The upper boundary must be one byte less than a 4KB boundary. */
                    pGlobals->uPciBiosIo = RT_ALIGN_32(pGlobals->uPciBiosIo, _4K);
                }
                devpciR3SetByte(pPciDev, VBOX_PCI_IO_LIMIT, ((pGlobals->uPciBiosIo >> 8) & 0xf0) - 1);

                /* Same with the MMIO limit register but with 1MB boundary here. */
                if ((u32MMIOAddressBase != pGlobals->uPciBiosMmio) && ((pGlobals->uPciBiosMmio % _1M) != 0))
                {
                    /* The upper boundary must be one byte less than a 1MB boundary. */
                    pGlobals->uPciBiosMmio = RT_ALIGN_32(pGlobals->uPciBiosMmio, _1M);
                }
                devpciR3SetWord(pPciDev, VBOX_PCI_MEMORY_LIMIT, ((pGlobals->uPciBiosMmio >> 16) & UINT32_C(0xfff0)) - 1);

                /*
                 * Set the prefetch base and limit registers. We currently have no device with a prefetchable region
                 * which may be behind a bridge. That's why it is unconditionally disabled here atm by writing a higher value into
                 * the base register than in the limit register.
                 */
                devpciR3SetWord(pPciDev, VBOX_PCI_PREF_MEMORY_BASE, 0xfff0);
                devpciR3SetWord(pPciDev, VBOX_PCI_PREF_MEMORY_LIMIT, 0x0);
                devpciR3SetDWord(pPciDev, VBOX_PCI_PREF_BASE_UPPER32, 0x00);
                devpciR3SetDWord(pPciDev, VBOX_PCI_PREF_LIMIT_UPPER32, 0x00);
                break;
            }
            default:
            default_map:
            {
                /* default memory mappings */
                bool fActiveMemRegion = false;
                bool fActiveIORegion = false;
                /*
                 * PCI_NUM_REGIONS is 7 because of the rom region but there are only 6 base address register defined by the PCI spec.
                 * Leaving only PCI_NUM_REGIONS would cause reading another and enabling a memory region which does not exist.
                 */
                for (unsigned i = 0; i < (PCI_NUM_REGIONS-1); i++)
                {
                    uint32_t u32Size;
                    uint8_t  u8RessourceType;
                    uint32_t u32Address = 0x10 + i * 4;

                    /* Calculate size. */
                    u8RessourceType = devpciR3GetByte(pPciDev, u32Address);
                    devpciR3SetDWord(pPciDev, u32Address, UINT32_C(0xffffffff));
                    u32Size = devpciR3GetDWord(pPciDev, u32Address);
                    bool fIsPio = ((u8RessourceType & PCI_COMMAND_IOACCESS) == PCI_COMMAND_IOACCESS);
                    /* Clear resource information depending on resource type. */
                    if (fIsPio) /* I/O */
                        u32Size &= ~(0x01);
                    else                        /* MMIO */
                        u32Size &= ~(0x0f);

                    /*
                     * Invert all bits and add 1 to get size of the region.
                     * (From PCI implementation note)
                     */
                    if (fIsPio && (u32Size & UINT32_C(0xffff0000)) == 0)
                        u32Size = (~(u32Size | UINT32_C(0xffff0000))) + 1;
                    else
                        u32Size = (~u32Size) + 1;

                    Log2Func(("Size of region %u for device %d on bus %d is %u\n", i, pPciDev->uDevFn, pBus->iBus, u32Size));

                    if (u32Size)
                    {
                        if (fIsPio)
                            paddr = &pGlobals->uPciBiosIo;
                        else
                            paddr = &pGlobals->uPciBiosMmio;
                        uint32_t uNew = *paddr;
                        uNew = (uNew + u32Size - 1) & ~(u32Size - 1);
                        if (fIsPio)
                            uNew &= UINT32_C(0xffff);
                        /* Unconditionally exclude I/O-APIC/HPET/ROM. Pessimistic, but better than causing a mess. */
                        if (!uNew || (uNew <= UINT32_C(0xffffffff) && uNew + u32Size - 1 >= UINT32_C(0xfec00000)))
                        {
                            LogRel(("PCI: no space left for BAR%u of device %u/%u/%u (vendor=%#06x device=%#06x)\n",
                                    i, pBus->iBus, pPciDev->uDevFn >> 3, pPciDev->uDevFn & 7, vendor_id, device_id)); /** @todo make this a VM start failure later. */
                            /* Undo the mapping mess caused by the size probing. */
                            devpciR3SetDWord(pPciDev, u32Address, UINT32_C(0));
                        }
                        else
                        {
                            LogFunc(("Start address of %s region %u is %#x\n", (fIsPio ? "I/O" : "MMIO"), i, uNew));
                            devpciR3BiosInitSetRegionAddress(pBus, pPciDev, i, uNew);
                            if (fIsPio)
                                fActiveIORegion = true;
                            else
                                fActiveMemRegion = true;
                            *paddr = uNew + u32Size;
                            Log2Func(("New address is %#x\n", *paddr));
                        }
                    }
                }

                /* Update the command word appropriately. */
                devpciR3SetWord(pPciDev, PCI_COMMAND,
                                  devpciR3GetWord(pPciDev, PCI_COMMAND)
                                | (fActiveMemRegion ? PCI_COMMAND_MEMACCESS : 0)
                                | (fActiveIORegion ? PCI_COMMAND_IOACCESS : 0));

                break;
            }
        }

        /* map the interrupt */
        pin = devpciR3GetByte(pPciDev, PCI_INTERRUPT_PIN);
        if (pin != 0)
        {
            uint8_t uBridgeDevFn = pPciDev->uDevFn;
            pin--;

            /* We need to go up to the host bus to see which irq this device will assert there. */
            while (cBridgeDepth != 0)
            {
                /* Get the pin the device would assert on the bridge. */
                pin = ((uBridgeDevFn >> 3) + pin) & 3;
                uBridgeDevFn = paBridgePositions[cBridgeDepth];
                cBridgeDepth--;
            }

            pin = pci_slot_get_pirq(pPciDev->uDevFn, pin);
            pic_irq = pci_irqs[pin];
            devpciR3SetByte(pPciDev, PCI_INTERRUPT_LINE, pic_irq);
        }
    }
}

/**
 * Worker for Fake PCI BIOS config, triggered by magic port access by BIOS.
 *
 * @returns VBox status code.
 *
 * @param   pDevIns     i440FX device instance.
 */
static int pciR3FakePCIBIOS(PPDMDEVINS pDevIns)
{
    uint8_t         elcr[2]    = {0, 0};
    PDEVPCIROOT     pGlobals   = PDMINS_2_DATA(pDevIns, PDEVPCIROOT);
    PVM             pVM        = PDMDevHlpGetVM(pDevIns); Assert(pVM);
    PVMCPU          pVCpu      = PDMDevHlpGetVMCPU(pDevIns); Assert(pVM);
    uint32_t const  cbBelow4GB = MMR3PhysGetRamSizeBelow4GB(pVM);
    uint64_t const  cbAbove4GB = MMR3PhysGetRamSizeAbove4GB(pVM);
    RT_NOREF(cbBelow4GB, cbAbove4GB);

    LogRel(("PCI: setting up resources and interrupts\n"));

    /*
     * Set the start addresses.
     */
    pGlobals->uPciBiosBus  = 0;
    pGlobals->uPciBiosIo   = 0xd000;
    pGlobals->uPciBiosMmio = UINT32_C(0xf0000000);

    /*
     * Activate IRQ mappings.
     */
    PPDMPCIDEV pPIIX3 = &pGlobals->Piix3.PIIX3State.dev;
    for (unsigned i = 0; i < 4; i++)
    {
        uint8_t irq = pci_irqs[i];
        /* Set to trigger level. */
        elcr[irq >> 3] |= (1 << (irq & 7));
        /* Activate irq remapping in PIIX3. */
        devpciR3SetByte(pPIIX3, 0x60 + i, irq);
    }

    /* Tell to the PIC. */
    VBOXSTRICTRC rcStrict = IOMIOPortWrite(pVM, pVCpu, 0x4d0, elcr[0], sizeof(uint8_t));
    if (rcStrict == VINF_SUCCESS)
        rcStrict = IOMIOPortWrite(pVM, pVCpu, 0x4d1, elcr[1], sizeof(uint8_t));
    if (rcStrict != VINF_SUCCESS)
    {
        AssertMsgFailed(("Writing to PIC failed! rcStrict=%Rrc\n", VBOXSTRICTRC_VAL(rcStrict)));
        return RT_SUCCESS(rcStrict) ? VERR_INTERNAL_ERROR : VBOXSTRICTRC_VAL(rcStrict);
    }

    /*
     * Init the devices.
     */
    PDEVPCIBUS pBus = &pGlobals->PciBus;
    for (uint32_t uDevFn = 0; uDevFn < RT_ELEMENTS(pBus->apDevices); uDevFn++)
    {
        PPDMPCIDEV pPciDev = pBus->apDevices[uDevFn];
        uint8_t aBridgePositions[256];

        if (pPciDev)
        {
            memset(aBridgePositions, 0, sizeof(aBridgePositions));
            Log2(("PCI: Initializing device %d (%#x)\n",
                  uDevFn, 0x80000000 | (uDevFn << 8)));
            pci_bios_init_device(pGlobals, pBus, pPciDev, 0, aBridgePositions);
        }
    }

    return VINF_SUCCESS;
}

#endif /* IN_RING3 */


/* -=-=-=-=-=- I/O ports -=-=-=-=-=- */

/**
 * @callback_method_impl{FNIOMIOPORTOUT, PCI address}
 */
PDMBOTHCBDECL(int) pciIOPortAddressWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb)
{
    LogFunc(("Port=%#x u32=%#x cb=%d\n", Port, u32, cb));
    RT_NOREF2(Port, pvUser);
    if (cb == 4)
    {
        PDEVPCIROOT pThis = PDMINS_2_DATA(pDevIns, PDEVPCIROOT);
        PCI_LOCK(pDevIns, VINF_IOM_R3_IOPORT_WRITE);
        pThis->uConfigReg = u32 & ~3; /* Bits 0-1 are reserved and we silently clear them */
        PCI_UNLOCK(pDevIns);
    }
    /* else: 440FX does "pass through to the bus" for other writes, what ever that means.
     * Linux probes for cmd640 using byte writes/reads during ide init. We'll just ignore it. */
    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{FNIOMIOPORTIN, PCI address}
 */
PDMBOTHCBDECL(int) pciIOPortAddressRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb)
{
    RT_NOREF2(Port, pvUser);
    if (cb == 4)
    {
        PDEVPCIROOT pThis = PDMINS_2_DATA(pDevIns, PDEVPCIROOT);
        PCI_LOCK(pDevIns, VINF_IOM_R3_IOPORT_READ);
        *pu32 = pThis->uConfigReg;
        PCI_UNLOCK(pDevIns);
        LogFunc(("Port=%#x cb=%d -> %#x\n", Port, cb, *pu32));
        return VINF_SUCCESS;
    }
    /* else: 440FX does "pass through to the bus" for other writes, what ever that means.
     * Linux probes for cmd640 using byte writes/reads during ide init. We'll just ignore it. */
    LogFunc(("Port=%#x cb=%d VERR_IOM_IOPORT_UNUSED\n", Port, cb));
    return VERR_IOM_IOPORT_UNUSED;
}


/**
 * @callback_method_impl{FNIOMIOPORTOUT, PCI data}
 */
PDMBOTHCBDECL(int) pciIOPortDataWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb)
{
    LogFunc(("Port=%#x u32=%#x cb=%d\n", Port, u32, cb));
    NOREF(pvUser);
    int rc = VINF_SUCCESS;
    if (!(Port % cb))
    {
        PCI_LOCK(pDevIns, VINF_IOM_R3_IOPORT_WRITE);
        rc = pci_data_write(PDMINS_2_DATA(pDevIns, PDEVPCIROOT), Port, u32, cb);
        PCI_UNLOCK(pDevIns);
    }
    else
        AssertMsgFailed(("Write to port %#x u32=%#x cb=%d\n", Port, u32, cb));
    return rc;
}


/**
 * @callback_method_impl{FNIOMIOPORTIN, PCI data}
 */
PDMBOTHCBDECL(int) pciIOPortDataRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb)
{
    NOREF(pvUser);
    if (!(Port % cb))
    {
        PCI_LOCK(pDevIns, VINF_IOM_R3_IOPORT_READ);
        int rc = pci_data_read(PDMINS_2_DATA(pDevIns, PDEVPCIROOT), Port, cb, pu32);
        PCI_UNLOCK(pDevIns);
        LogFunc(("Port=%#x cb=%#x -> %#x (%Rrc)\n", Port, cb, *pu32, rc));
        return rc;
    }
    AssertMsgFailed(("Read from port %#x cb=%d\n", Port, cb));
    return VERR_IOM_IOPORT_UNUSED;
}

#ifdef IN_RING3

/**
 * @callback_method_impl{FNIOMIOPORTOUT, PCI data}
 */
DECLCALLBACK(int) pciR3IOPortMagicPCIWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb)
{
    RT_NOREF2(pvUser, Port);
    LogFunc(("Port=%#x u32=%#x cb=%d\n", Port, u32, cb));
    if (cb == 4)
    {
        if (u32 == UINT32_C(19200509)) // Richard Adams
        {
            int rc = pciR3FakePCIBIOS(pDevIns);
            AssertRC(rc);
        }
    }

    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{FNIOMIOPORTIN, PCI data}
 */
DECLCALLBACK(int) pciR3IOPortMagicPCIRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb)
{
    RT_NOREF5(pDevIns, pvUser, Port, pu32, cb);
    LogFunc(("Port=%#x cb=%d VERR_IOM_IOPORT_UNUSED\n", Port, cb));
    return VERR_IOM_IOPORT_UNUSED;
}


/*
 * Include code we share with the other PCI bus implementation.
 *
 * Note! No #ifdefs, use instant data booleans/flags/whatever.  Goal is to
 *       completely merge these files!  File #1 contains code we write, where
 *       as a possible file #2 contains external code if there's any left.
 */
# include "DevPciMerge1.cpp.h"


/* -=-=-=-=-=- Saved state -=-=-=-=-=- */

/**
 * Common worker for pciR3SaveExec and pcibridgeR3SaveExec.
 *
 * @returns VBox status code.
 * @param   pBus            The bus to save.
 * @param   pSSM            The saved state handle.
 */
static int pciR3CommonSaveExec(PDEVPCIBUS pBus, PSSMHANDLE pSSM)
{
    /*
     * Iterate thru all the devices.
     */
    for (uint32_t uDevFn = 0; uDevFn < RT_ELEMENTS(pBus->apDevices); uDevFn++)
    {
        PPDMPCIDEV pDev = pBus->apDevices[uDevFn];
        if (pDev)
        {
            SSMR3PutU32(pSSM, uDevFn);
            SSMR3PutMem(pSSM, pDev->abConfig, sizeof(pDev->abConfig));

            SSMR3PutS32(pSSM, pDev->Int.s.uIrqPinState);

            /* Save the type an size of all the regions. */
            for (uint32_t iRegion = 0; iRegion < VBOX_PCI_NUM_REGIONS; iRegion++)
            {
                SSMR3PutU8(pSSM, pDev->Int.s.aIORegions[iRegion].type);
                SSMR3PutU64(pSSM, pDev->Int.s.aIORegions[iRegion].size);
            }
        }
    }
    return SSMR3PutU32(pSSM, UINT32_MAX); /* terminator */
}


/**
 * @callback_method_impl{FNSSMDEVSAVEEXEC}
 */
static DECLCALLBACK(int) pciR3SaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    PDEVPCIROOT pThis = PDMINS_2_DATA(pDevIns, PDEVPCIROOT);

    /*
     * Bus state data.
     */
    SSMR3PutU32(pSSM, pThis->uConfigReg);
    SSMR3PutBool(pSSM, pThis->fUseIoApic);

    /*
     * Save IRQ states.
     */
    for (unsigned i = 0; i < RT_ELEMENTS(pThis->Piix3.auPciLegacyIrqLevels); i++)
        SSMR3PutU32(pSSM, pThis->Piix3.auPciLegacyIrqLevels[i]);
    for (unsigned i = 0; i < RT_ELEMENTS(pThis->auPciApicIrqLevels); i++)
        SSMR3PutU32(pSSM, pThis->auPciApicIrqLevels[i]);

    SSMR3PutU32(pSSM, pThis->Piix3.iAcpiIrqLevel);
    SSMR3PutS32(pSSM, pThis->Piix3.iAcpiIrq);

    SSMR3PutU32(pSSM, UINT32_MAX);      /* separator */

    /*
     * Join paths with pcibridgeR3SaveExec.
     */
    return pciR3CommonSaveExec(&pThis->PciBus, pSSM);
}


/**
 * Common worker for pciR3LoadExec and pcibridgeR3LoadExec.
 *
 * @returns VBox status code.
 * @param   pBus                The bus which data is being loaded.
 * @param   pSSM                The saved state handle.
 * @param   uVersion            The data version.
 * @param   uPass               The pass.
 */
static DECLCALLBACK(int) pciR3CommonLoadExec(PDEVPCIBUS pBus, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    uint32_t    u32;
    int         rc;

    Assert(uPass == SSM_PASS_FINAL); NOREF(uPass);

    /*
     * Iterate thru all the devices and write 0 to the COMMAND register so
     * that all the memory is unmapped before we start restoring the saved
     * mapping locations.
     *
     * The register value is restored afterwards so we can do proper
     * LogRels in devpciR3CommonRestoreConfig.
     */
    for (uint32_t uDevFn = 0; uDevFn < RT_ELEMENTS(pBus->apDevices); uDevFn++)
    {
        PPDMPCIDEV pDev = pBus->apDevices[uDevFn];
        if (pDev)
        {
            uint16_t u16 = PCIDevGetCommand(pDev);
            pDev->Int.s.pfnConfigWrite(pDev->Int.s.CTX_SUFF(pDevIns), pDev, VBOX_PCI_COMMAND, 0, 2);
            PCIDevSetCommand(pDev, u16);
            Assert(PCIDevGetCommand(pDev) == u16);
        }
    }

    /*
     * Iterate all the devices.
     */
    for (uint32_t uDevFn = 0;; uDevFn++)
    {
        /* index / terminator */
        rc = SSMR3GetU32(pSSM, &u32);
        if (RT_FAILURE(rc))
            return rc;
        if (u32 == UINT32_MAX)
            break;
        if (    u32 >= RT_ELEMENTS(pBus->apDevices)
            ||  u32 < uDevFn)
        {
            AssertMsgFailed(("u32=%#x uDevFn=%#x\n", u32, uDevFn));
            return rc;
        }

        /* skip forward to the device checking that no new devices are present. */
        for (; uDevFn < u32; uDevFn++)
        {
            if (pBus->apDevices[uDevFn])
            {
                LogRel(("PCI: New device in slot %#x, %s (vendor=%#06x device=%#06x)\n", uDevFn, pBus->apDevices[uDevFn]->pszNameR3,
                        PCIDevGetVendorId(pBus->apDevices[uDevFn]), PCIDevGetDeviceId(pBus->apDevices[uDevFn])));
                if (SSMR3HandleGetAfter(pSSM) != SSMAFTER_DEBUG_IT)
                    return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("New device in slot %#x, %s (vendor=%#06x device=%#06x)"),
                                            uDevFn, pBus->apDevices[uDevFn]->pszNameR3, PCIDevGetVendorId(pBus->apDevices[uDevFn]), PCIDevGetDeviceId(pBus->apDevices[uDevFn]));
            }
        }

        /* get the data */
        PDMPCIDEV DevTmp;
        RT_ZERO(DevTmp);
        DevTmp.Int.s.uIrqPinState = ~0; /* Invalid value in case we have an older saved state to force a state change in pciSetIrq. */
        SSMR3GetMem(pSSM, DevTmp.abConfig, sizeof(DevTmp.abConfig));
        if (uVersion < VBOX_PCI_SAVED_STATE_VERSION_IRQ_STATES)
        {
            int32_t i32Temp;
            /* Irq value not needed anymore. */
            rc = SSMR3GetS32(pSSM, &i32Temp);
            if (RT_FAILURE(rc))
                return rc;
        }
        else
        {
            rc = SSMR3GetS32(pSSM, &DevTmp.Int.s.uIrqPinState);
            if (RT_FAILURE(rc))
                return rc;
        }

        /* Load the region types and sizes. */
        if (uVersion >= VBOX_PCI_SAVED_STATE_VERSION_REGION_SIZES)
        {
            for (uint32_t iRegion = 0; iRegion < VBOX_PCI_NUM_REGIONS; iRegion++)
            {
                SSMR3GetU8(pSSM, &DevTmp.Int.s.aIORegions[iRegion].type);
                rc = SSMR3GetU64(pSSM, &DevTmp.Int.s.aIORegions[iRegion].size);
                AssertLogRelRCReturn(rc, rc);
            }
        }

        /* check that it's still around. */
        PPDMPCIDEV pDev = pBus->apDevices[uDevFn];
        if (!pDev)
        {
            LogRel(("PCI: Device in slot %#x has been removed! vendor=%#06x device=%#06x\n", uDevFn,
                    PCIDevGetVendorId(&DevTmp), PCIDevGetDeviceId(&DevTmp)));
            if (SSMR3HandleGetAfter(pSSM) != SSMAFTER_DEBUG_IT)
                return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("Device in slot %#x has been removed! vendor=%#06x device=%#06x"),
                                        uDevFn, PCIDevGetVendorId(&DevTmp), PCIDevGetDeviceId(&DevTmp));
            continue;
        }

        /* match the vendor id assuming that this will never be changed. */
        if (   DevTmp.abConfig[0] != pDev->abConfig[0]
            || DevTmp.abConfig[1] != pDev->abConfig[1])
            return SSMR3SetCfgError(pSSM, RT_SRC_POS,
                                    N_("Device in slot %#x (%s) vendor id mismatch! saved=%.4Rhxs current=%.4Rhxs"),
                                    uDevFn, pDev->pszNameR3, DevTmp.abConfig, pDev->abConfig);

        /* commit the loaded device config. */
        rc = devpciR3CommonRestoreRegions(pSSM, pDev, DevTmp.Int.s.aIORegions,
                                          uVersion >= VBOX_PCI_SAVED_STATE_VERSION_REGION_SIZES);
        if (RT_FAILURE(rc))
            break;
        devpciR3CommonRestoreConfig(pDev, &DevTmp.abConfig[0]);

        pDev->Int.s.uIrqPinState = DevTmp.Int.s.uIrqPinState;
    }

    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{FNSSMDEVLOADEXEC}
 */
static DECLCALLBACK(int) pciR3LoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    PDEVPCIROOT pThis = PDMINS_2_DATA(pDevIns, PDEVPCIROOT);
    PDEVPCIBUS  pBus  = &pThis->PciBus;
    uint32_t    u32;
    int         rc;

    /*
     * Check the version.
     */
    if (uVersion > VBOX_PCI_SAVED_STATE_VERSION)
        return VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;
    Assert(uPass == SSM_PASS_FINAL); NOREF(uPass);

    /*
     * Bus state data.
     */
    SSMR3GetU32(pSSM, &pThis->uConfigReg);
    if (uVersion >= VBOX_PCI_SAVED_STATE_VERSION_USE_IO_APIC)
        SSMR3GetBool(pSSM, &pThis->fUseIoApic);

    /* Load IRQ states. */
    if (uVersion >= VBOX_PCI_SAVED_STATE_VERSION_IRQ_STATES)
    {
        for (uint8_t i = 0; i < RT_ELEMENTS(pThis->Piix3.auPciLegacyIrqLevels); i++)
            SSMR3GetU32(pSSM, (uint32_t *)&pThis->Piix3.auPciLegacyIrqLevels[i]);
        for (uint8_t i = 0; i < RT_ELEMENTS(pThis->auPciApicIrqLevels); i++)
            SSMR3GetU32(pSSM, (uint32_t *)&pThis->auPciApicIrqLevels[i]);

        SSMR3GetU32(pSSM, &pThis->Piix3.iAcpiIrqLevel);
        SSMR3GetS32(pSSM, &pThis->Piix3.iAcpiIrq);
    }

    /* separator */
    rc = SSMR3GetU32(pSSM, &u32);
    if (RT_FAILURE(rc))
        return rc;
    if (u32 != UINT32_MAX)
        AssertMsgFailedReturn(("u32=%#x\n", u32), rc);

    /*
     * The devices.
     */
    return pciR3CommonLoadExec(pBus, pSSM, uVersion, uPass);
}


/* -=-=-=-=-=- PCI Bus Interface Methods (PDMPCIBUSREG) -=-=-=-=-=- */


/* -=-=-=-=-=- Debug Info Handlers -=-=-=-=-=- */

/**
 * @callback_method_impl{FNDBGFHANDLERDEV}
 */
static DECLCALLBACK(void) pciR3IrqRouteInfo(PPDMDEVINS pDevIns, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    PDEVPCIROOT pGlobals = PDMINS_2_DATA(pDevIns, PDEVPCIROOT);
    PPDMPCIDEV pPIIX3 = &pGlobals->Piix3.PIIX3State.dev;
    NOREF(pszArgs);

    uint16_t router = pPIIX3->uDevFn;
    pHlp->pfnPrintf(pHlp, "PCI interrupt router at: %02X:%02X:%X\n",
                    router >> 8, (router >> 3) & 0x1f, router & 0x7);

    for (int i = 0; i < 4; ++i)
    {
        uint8_t irq_map = devpciR3GetByte(pPIIX3, 0x60 + i);
        if (irq_map & 0x80)
            pHlp->pfnPrintf(pHlp, "PIRQ%c disabled\n", 'A' + i);
        else
            pHlp->pfnPrintf(pHlp, "PIRQ%c -> IRQ%d\n", 'A' + i, irq_map & 0xf);
    }
}


/* -=-=-=-=-=- PDMDEVREG  -=-=-=-=-=- */


/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int)   pciR3Construct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    RT_NOREF1(iInstance);
    Assert(iInstance == 0);
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    /*
     * Validate and read configuration.
     */
    if (!CFGMR3AreValuesValid(pCfg, "IOAPIC\0" "GCEnabled\0" "R0Enabled\0"))
        return VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES;

    /* query whether we got an IOAPIC */
    bool fUseIoApic;
    int rc = CFGMR3QueryBoolDef(pCfg, "IOAPIC", &fUseIoApic, false);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to query boolean value \"IOAPIC\""));

    /* check if RC code is enabled. */
    bool fGCEnabled;
    rc = CFGMR3QueryBoolDef(pCfg, "GCEnabled", &fGCEnabled, true);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to query boolean value \"GCEnabled\""));

    /* check if R0 code is enabled. */
    bool fR0Enabled;
    rc = CFGMR3QueryBoolDef(pCfg, "R0Enabled", &fR0Enabled, true);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to query boolean value \"R0Enabled\""));
    Log(("PCI: fUseIoApic=%RTbool fGCEnabled=%RTbool fR0Enabled=%RTbool\n", fUseIoApic, fGCEnabled, fR0Enabled));

    /*
     * Init data and register the PCI bus.
     */
    PDEVPCIROOT pGlobals = PDMINS_2_DATA(pDevIns, PDEVPCIROOT);
    pGlobals->uPciBiosIo   = 0xc000;
    pGlobals->uPciBiosMmio = 0xf0000000;
    memset((void *)&pGlobals->Piix3.auPciLegacyIrqLevels, 0, sizeof(pGlobals->Piix3.auPciLegacyIrqLevels));
    pGlobals->fUseIoApic   = fUseIoApic;
    memset((void *)&pGlobals->auPciApicIrqLevels, 0, sizeof(pGlobals->auPciApicIrqLevels));

    pGlobals->pDevInsR3 = pDevIns;
    pGlobals->pDevInsR0 = PDMDEVINS_2_R0PTR(pDevIns);
    pGlobals->pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);

    pGlobals->PciBus.fTypePiix3  = true;
    pGlobals->PciBus.fTypeIch9   = false;
    pGlobals->PciBus.fPureBridge = false;
    pGlobals->PciBus.pDevInsR3 = pDevIns;
    pGlobals->PciBus.pDevInsR0 = PDMDEVINS_2_R0PTR(pDevIns);
    pGlobals->PciBus.pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);
    pGlobals->PciBus.papBridgesR3 = (PPDMPCIDEV *)PDMDevHlpMMHeapAllocZ(pDevIns,
                                                                        sizeof(PPDMPCIDEV)
                                                                        * RT_ELEMENTS(pGlobals->PciBus.apDevices));
    AssertLogRelReturn(pGlobals->PciBus.papBridgesR3, VERR_NO_MEMORY);


    PDMPCIBUSREG PciBusReg;
    PDEVPCIBUS   pBus = &pGlobals->PciBus;
    PciBusReg.u32Version              = PDM_PCIBUSREG_VERSION;
    PciBusReg.pfnRegisterR3           = pciR3MergedRegister;
    PciBusReg.pfnRegisterMsiR3        = NULL;
    PciBusReg.pfnIORegionRegisterR3   = devpciR3CommonIORegionRegister;
    PciBusReg.pfnSetConfigCallbacksR3 = devpciR3CommonSetConfigCallbacks;
    PciBusReg.pfnSetIrqR3             = pciSetIrq;
    PciBusReg.pszSetIrqRC             = fGCEnabled ? "pciSetIrq" : NULL;
    PciBusReg.pszSetIrqR0             = fR0Enabled ? "pciSetIrq" : NULL;
    rc = PDMDevHlpPCIBusRegister(pDevIns, &PciBusReg, &pBus->pPciHlpR3, &pBus->iBus);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Failed to register ourselves as a PCI Bus"));
    Assert(pBus->iBus == 0);
    if (pBus->pPciHlpR3->u32Version != PDM_PCIHLPR3_VERSION)
        return PDMDevHlpVMSetError(pDevIns, VERR_VERSION_MISMATCH, RT_SRC_POS,
                                   N_("PCI helper version mismatch; got %#x expected %#x"),
                                   pBus->pPciHlpR3->u32Version, PDM_PCIHLPR3_VERSION);

    pBus->pPciHlpRC = pBus->pPciHlpR3->pfnGetRCHelpers(pDevIns);
    pBus->pPciHlpR0 = pBus->pPciHlpR3->pfnGetR0Helpers(pDevIns);

    /* Disable default device locking. */
    rc = PDMDevHlpSetDeviceCritSect(pDevIns, PDMDevHlpCritSectGetNop(pDevIns));
    AssertRCReturn(rc, rc);

    /*
     * Fill in PCI configs and add them to the bus.
     */
    /* i440FX */
    PCIDevSetVendorId(  &pBus->PciDev, 0x8086); /* Intel */
    PCIDevSetDeviceId(  &pBus->PciDev, 0x1237);
    PCIDevSetRevisionId(&pBus->PciDev,   0x02);
    PCIDevSetClassSub(  &pBus->PciDev,   0x00); /* host2pci */
    PCIDevSetClassBase( &pBus->PciDev,   0x06); /* PCI_bridge */
    PCIDevSetHeaderType(&pBus->PciDev,   0x00);
    rc = PDMDevHlpPCIRegisterEx(pDevIns, &pBus->PciDev, PDMPCIDEVREG_CFG_PRIMARY, 0 /*fFlags*/,
                                0 /*uPciDevNo*/, 0 /*uPciFunNo*/, "i440FX");
    AssertLogRelRCReturn(rc, rc);

    /* PIIX3 */
    PCIDevSetVendorId(  &pGlobals->Piix3.PIIX3State.dev, 0x8086); /* Intel */
    PCIDevSetDeviceId(  &pGlobals->Piix3.PIIX3State.dev, 0x7000); /* 82371SB PIIX3 PCI-to-ISA bridge (Step A1) */
    PCIDevSetClassSub(  &pGlobals->Piix3.PIIX3State.dev,   0x01); /* PCI_ISA */
    PCIDevSetClassBase( &pGlobals->Piix3.PIIX3State.dev,   0x06); /* PCI_bridge */
    PCIDevSetHeaderType(&pGlobals->Piix3.PIIX3State.dev,   0x80); /* PCI_multifunction, generic */
    rc = PDMDevHlpPCIRegisterEx(pDevIns, &pGlobals->Piix3.PIIX3State.dev, PDMPCIDEVREG_CFG_NEXT, 0 /*fFlags*/,
                                1 /*uPciDevNo*/, 0 /*uPciFunNo*/, "PIIX3");
    AssertLogRelRCReturn(rc, rc);
    pciR3Piix3Reset(&pGlobals->Piix3.PIIX3State);

    pBus->iDevSearch = 16;

    /*
     * Register I/O ports and save state.
     */
    rc = PDMDevHlpIOPortRegister(pDevIns, 0x0cf8, 1, NULL, pciIOPortAddressWrite, pciIOPortAddressRead, NULL, NULL, "i440FX (PCI)");
    if (RT_FAILURE(rc))
        return rc;
    rc = PDMDevHlpIOPortRegister(pDevIns, 0x0cfc, 4, NULL, pciIOPortDataWrite, pciIOPortDataRead, NULL, NULL, "i440FX (PCI)");
    if (RT_FAILURE(rc))
        return rc;
    if (fGCEnabled)
    {
        rc = PDMDevHlpIOPortRegisterRC(pDevIns, 0x0cf8, 1, NIL_RTGCPTR, "pciIOPortAddressWrite", "pciIOPortAddressRead", NULL, NULL, "i440FX (PCI)");
        if (RT_FAILURE(rc))
            return rc;
        rc = PDMDevHlpIOPortRegisterRC(pDevIns, 0x0cfc, 4, NIL_RTGCPTR, "pciIOPortDataWrite", "pciIOPortDataRead", NULL, NULL, "i440FX (PCI)");
        if (RT_FAILURE(rc))
            return rc;
    }
    if (fR0Enabled)
    {
        rc = PDMDevHlpIOPortRegisterR0(pDevIns, 0x0cf8, 1, NIL_RTR0PTR, "pciIOPortAddressWrite", "pciIOPortAddressRead", NULL, NULL, "i440FX (PCI)");
        if (RT_FAILURE(rc))
            return rc;
        rc = PDMDevHlpIOPortRegisterR0(pDevIns, 0x0cfc, 4, NIL_RTR0PTR, "pciIOPortDataWrite", "pciIOPortDataRead", NULL, NULL, "i440FX (PCI)");
        if (RT_FAILURE(rc))
            return rc;
    }

    rc = PDMDevHlpIOPortRegister(pDevIns, 0x0410, 1, NULL, pciR3IOPortMagicPCIWrite, pciR3IOPortMagicPCIRead, NULL, NULL, "i440FX (Fake PCI BIOS trigger)")
;
    if (RT_FAILURE(rc))
        return rc;


    rc = PDMDevHlpSSMRegisterEx(pDevIns, VBOX_PCI_SAVED_STATE_VERSION, sizeof(*pBus) + 16*128, "pgm",
                                NULL, NULL, NULL,
                                NULL, pciR3SaveExec, NULL,
                                NULL, pciR3LoadExec, NULL);
    if (RT_FAILURE(rc))
        return rc;

    PDMDevHlpDBGFInfoRegister(pDevIns, "pci",
                              "Display PCI bus status. Recognizes 'basic' or 'verbose' as arguments, defaults to 'basic'.",
                              devpciR3InfoPci);
    PDMDevHlpDBGFInfoRegister(pDevIns, "pciirq", "Display PCI IRQ state. (no arguments)", devpciR3InfoPciIrq);
    PDMDevHlpDBGFInfoRegister(pDevIns, "irqroute", "Display PCI IRQ routing. (no arguments)", pciR3IrqRouteInfo);

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnDestruct}
 */
static DECLCALLBACK(int)   pciR3Destruct(PPDMDEVINS pDevIns)
{
    PDEVPCIROOT pGlobals = PDMINS_2_DATA(pDevIns, PDEVPCIROOT);
    if (pGlobals->PciBus.papBridgesR3)
    {
        PDMDevHlpMMHeapFree(pDevIns, pGlobals->PciBus.papBridgesR3);
        pGlobals->PciBus.papBridgesR3 = NULL;
    }
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnReset}
 */
static DECLCALLBACK(void) pciR3Reset(PPDMDEVINS pDevIns)
{
    PDEVPCIROOT pGlobals = PDMINS_2_DATA(pDevIns, PDEVPCIROOT);
    PDEVPCIBUS  pBus     = &pGlobals->PciBus;

    /* PCI-specific reset for each device. */
    for (uint32_t uDevFn = 0; uDevFn < RT_ELEMENTS(pBus->apDevices); uDevFn++)
    {
        if (pBus->apDevices[uDevFn])
            devpciR3ResetDevice(pBus->apDevices[uDevFn]);
    }

    pciR3Piix3Reset(&pGlobals->Piix3.PIIX3State);
}


/**
 * The device registration structure.
 */
const PDMDEVREG g_DevicePCI =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "pci",
    /* szRCMod */
    "VBoxDDRC.rc",
    /* szR0Mod */
    "VBoxDDR0.r0",
    /* pszDescription */
    "i440FX PCI bridge and PIIX3 ISA bridge.",
    /* fFlags */
    PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_RC | PDM_DEVREG_FLAGS_R0,
    /* fClass */
    PDM_DEVREG_CLASS_BUS_PCI | PDM_DEVREG_CLASS_BUS_ISA,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(DEVPCIROOT),
    /* pfnConstruct */
    pciR3Construct,
    /* pfnDestruct */
    pciR3Destruct,
    /* pfnRelocate */
    devpciR3RootRelocate,
    /* pfnMemSetup */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    pciR3Reset,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnQueryInterface */
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



/* -=-=-=-=-=- The PCI bridge specific bits -=-=-=-=-=- */

/**
 * @interface_method_impl{PDMPCIBUSREG,pfnSetIrqR3}
 */
PDMBOTHCBDECL(void) pcibridgeSetIrq(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, int iIrq, int iLevel, uint32_t uTagSrc)
{
    /*
     * The PCI-to-PCI bridge specification defines how the interrupt pins
     * are routed from the secondary to the primary bus (see chapter 9).
     * iIrq gives the interrupt pin the pci device asserted.
     * We change iIrq here according to the spec and call the SetIrq function
     * of our parent passing the device which asserted the interrupt instead of the device of the bridge.
     */
    PDEVPCIBUS pBus          = PDMINS_2_DATA(pDevIns, PDEVPCIBUS);
    PPDMPCIDEV pPciDevBus    = pPciDev;
    int        iIrqPinBridge = iIrq;
    uint8_t    uDevFnBridge  = 0;

    /* Walk the chain until we reach the host bus. */
    do
    {
        uDevFnBridge  = pBus->PciDev.uDevFn;
        iIrqPinBridge = ((pPciDevBus->uDevFn >> 3) + iIrqPinBridge) & 3;

        /* Get the parent. */
        pBus = pBus->PciDev.Int.s.CTX_SUFF(pBus);
        pPciDevBus = &pBus->PciDev;
    } while (pBus->iBus != 0);

    AssertMsg(pBus->iBus == 0, ("This is not the host pci bus iBus=%d\n", pBus->iBus));
    pciSetIrqInternal(DEVPCIBUS_2_DEVPCIROOT(pBus), uDevFnBridge, pPciDev, iIrqPinBridge, iLevel, uTagSrc);
}

#ifdef IN_RING3

/**
 * @callback_method_impl{FNPCIBRIDGECONFIGWRITE}
 */
static DECLCALLBACK(void) pcibridgeR3ConfigWrite(PPDMDEVINSR3 pDevIns, uint8_t iBus, uint8_t iDevice, uint32_t u32Address, uint32_t u32Value, unsigned cb)
{
    PDEVPCIBUS pBus = PDMINS_2_DATA(pDevIns, PDEVPCIBUS);

    LogFlowFunc(("pDevIns=%p iBus=%d iDevice=%d u32Address=%u u32Value=%u cb=%d\n", pDevIns, iBus, iDevice, u32Address, u32Value, cb));

    /* If the current bus is not the target bus search for the bus which contains the device. */
    if (iBus != pBus->PciDev.abConfig[VBOX_PCI_SECONDARY_BUS])
    {
        PPDMPCIDEV pBridgeDevice = pciR3FindBridge(pBus, iBus);
        if (pBridgeDevice)
        {
            AssertPtr(pBridgeDevice->Int.s.pfnBridgeConfigWrite);
            pBridgeDevice->Int.s.pfnBridgeConfigWrite(pBridgeDevice->Int.s.CTX_SUFF(pDevIns), iBus, iDevice, u32Address, u32Value, cb);
        }
    }
    else
    {
        /* This is the target bus, pass the write to the device. */
        PPDMPCIDEV pPciDev = pBus->apDevices[iDevice];
        if (pPciDev)
        {
            LogFunc(("%s: addr=%02x val=%08x len=%d\n", pPciDev->pszNameR3, u32Address, u32Value, cb));
            pPciDev->Int.s.pfnConfigWrite(pPciDev->Int.s.CTX_SUFF(pDevIns), pPciDev, u32Address, u32Value, cb);
        }
    }
}


/**
 * @callback_method_impl{FNPCIBRIDGECONFIGREAD}
 */
static DECLCALLBACK(uint32_t) pcibridgeR3ConfigRead(PPDMDEVINSR3 pDevIns, uint8_t iBus, uint8_t iDevice, uint32_t u32Address, unsigned cb)
{
    PDEVPCIBUS pBus = PDMINS_2_DATA(pDevIns, PDEVPCIBUS);
    uint32_t u32Value = 0xffffffff; /* Return value in case there is no device. */

    LogFlowFunc(("pDevIns=%p iBus=%d iDevice=%d u32Address=%u cb=%d\n", pDevIns, iBus, iDevice, u32Address, cb));

    /* If the current bus is not the target bus search for the bus which contains the device. */
    if (iBus != pBus->PciDev.abConfig[VBOX_PCI_SECONDARY_BUS])
    {
        PPDMPCIDEV pBridgeDevice = pciR3FindBridge(pBus, iBus);
        if (pBridgeDevice)
        {
            AssertPtr( pBridgeDevice->Int.s.pfnBridgeConfigRead);
            u32Value = pBridgeDevice->Int.s.pfnBridgeConfigRead(pBridgeDevice->Int.s.CTX_SUFF(pDevIns), iBus, iDevice, u32Address, cb);
        }
    }
    else
    {
        /* This is the target bus, pass the read to the device. */
        PPDMPCIDEV pPciDev = pBus->apDevices[iDevice];
        if (pPciDev)
        {
            u32Value = pPciDev->Int.s.pfnConfigRead(pPciDev->Int.s.CTX_SUFF(pDevIns), pPciDev, u32Address, cb);
            LogFunc(("%s: u32Address=%02x u32Value=%08x cb=%d\n", pPciDev->pszNameR3, u32Address, u32Value, cb));
        }
    }

    return u32Value;
}


/**
 * @callback_method_impl{FNSSMDEVSAVEEXEC}
 */
static DECLCALLBACK(int) pcibridgeR3SaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    PDEVPCIBUS pThis = PDMINS_2_DATA(pDevIns, PDEVPCIBUS);
    return pciR3CommonSaveExec(pThis, pSSM);
}


/**
 * @callback_method_impl{FNSSMDEVLOADEXEC}
 */
static DECLCALLBACK(int) pcibridgeR3LoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    PDEVPCIBUS pThis = PDMINS_2_DATA(pDevIns, PDEVPCIBUS);
    if (uVersion > VBOX_PCI_SAVED_STATE_VERSION)
        return VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;
    return pciR3CommonLoadExec(pThis, pSSM, uVersion, uPass);
}


/**
 * @interface_method_impl{PDMDEVREG,pfnReset}
 */
static DECLCALLBACK(void) pcibridgeR3Reset(PPDMDEVINS pDevIns)
{
    PDEVPCIBUS pBus = PDMINS_2_DATA(pDevIns, PDEVPCIBUS);

    /* Reset config space to default values. */
    pBus->PciDev.abConfig[VBOX_PCI_PRIMARY_BUS] = 0;
    pBus->PciDev.abConfig[VBOX_PCI_SECONDARY_BUS] = 0;
    pBus->PciDev.abConfig[VBOX_PCI_SUBORDINATE_BUS] = 0;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int)   pcibridgeR3Construct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    RT_NOREF(iInstance);
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    /*
     * Validate and read configuration.
     */
    if (!CFGMR3AreValuesValid(pCfg, "GCEnabled\0" "R0Enabled\0"))
        return VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES;

    /* check if RC code is enabled. */
    bool fGCEnabled;
    int rc = CFGMR3QueryBoolDef(pCfg, "GCEnabled", &fGCEnabled, true);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to query boolean value \"GCEnabled\""));

    /* check if R0 code is enabled. */
    bool fR0Enabled;
    rc = CFGMR3QueryBoolDef(pCfg, "R0Enabled", &fR0Enabled, true);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to query boolean value \"R0Enabled\""));
    Log(("PCI: fGCEnabled=%RTbool fR0Enabled=%RTbool\n", fGCEnabled, fR0Enabled));

    /*
     * Init data and register the PCI bus.
     */
    PDEVPCIBUS pBus = PDMINS_2_DATA(pDevIns, PDEVPCIBUS);
    pBus->fTypePiix3  = true;
    pBus->fTypeIch9   = false;
    pBus->fPureBridge = true;
    pBus->pDevInsR3 = pDevIns;
    pBus->pDevInsR0 = PDMDEVINS_2_R0PTR(pDevIns);
    pBus->pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);
    pBus->papBridgesR3 = (PPDMPCIDEV *)PDMDevHlpMMHeapAllocZ(pDevIns, sizeof(PPDMPCIDEV) * RT_ELEMENTS(pBus->apDevices));
    AssertLogRelReturn(pBus->papBridgesR3, VERR_NO_MEMORY);

    PDMPCIBUSREG PciBusReg;
    PciBusReg.u32Version              = PDM_PCIBUSREG_VERSION;
    PciBusReg.pfnRegisterR3           = pcibridgeR3MergedRegisterDevice;
    PciBusReg.pfnRegisterMsiR3        = NULL;
    PciBusReg.pfnIORegionRegisterR3   = devpciR3CommonIORegionRegister;
    PciBusReg.pfnSetConfigCallbacksR3 = devpciR3CommonSetConfigCallbacks;
    PciBusReg.pfnSetIrqR3             = pcibridgeSetIrq;
    PciBusReg.pszSetIrqRC             = fGCEnabled ? "pcibridgeSetIrq" : NULL;
    PciBusReg.pszSetIrqR0             = fR0Enabled ? "pcibridgeSetIrq" : NULL;
    rc = PDMDevHlpPCIBusRegister(pDevIns, &PciBusReg, &pBus->pPciHlpR3, &pBus->iBus);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Failed to register ourselves as a PCI Bus"));
    Assert(pBus->iBus == (uint32_t)iInstance + 1); /* Can be removed when adding support for multiple bridge implementations. */
    if (pBus->pPciHlpR3->u32Version != PDM_PCIHLPR3_VERSION)
        return PDMDevHlpVMSetError(pDevIns, VERR_VERSION_MISMATCH, RT_SRC_POS,
                                   N_("PCI helper version mismatch; got %#x expected %#x"),
                                   pBus->pPciHlpR3->u32Version, PDM_PCIHLPR3_VERSION);

    pBus->pPciHlpRC = pBus->pPciHlpR3->pfnGetRCHelpers(pDevIns);
    pBus->pPciHlpR0 = pBus->pPciHlpR3->pfnGetR0Helpers(pDevIns);

    /*
     * Fill in PCI configs and add them to the bus.
     */
    PCIDevSetVendorId(  &pBus->PciDev, 0x8086); /* Intel */
    PCIDevSetDeviceId(  &pBus->PciDev, 0x2448); /* 82801 Mobile PCI bridge. */
    PCIDevSetRevisionId(&pBus->PciDev,   0xf2);
    PCIDevSetClassSub(  &pBus->PciDev,   0x04); /* pci2pci */
    PCIDevSetClassBase( &pBus->PciDev,   0x06); /* PCI_bridge */
    PCIDevSetClassProg( &pBus->PciDev,   0x01); /* Supports subtractive decoding. */
    PCIDevSetHeaderType(&pBus->PciDev,   0x01); /* Single function device which adheres to the PCI-to-PCI bridge spec. */
    PCIDevSetCommand(   &pBus->PciDev, 0x0000);
    PCIDevSetStatus(    &pBus->PciDev, 0x0020); /* 66MHz Capable. */
    PCIDevSetInterruptLine(&pBus->PciDev, 0x00); /* This device does not assert interrupts. */

    /*
     * This device does not generate interrupts. Interrupt delivery from
     * devices attached to the bus is unaffected.
     */
    PCIDevSetInterruptPin(&pBus->PciDev, 0x00);

    /*
     * Register this PCI bridge. The called function will take care on which bus we will get registered.
     */
    rc = PDMDevHlpPCIRegisterEx(pDevIns, &pBus->PciDev, PDMPCIDEVREG_CFG_PRIMARY, PDMPCIDEVREG_F_PCI_BRIDGE,
                                PDMPCIDEVREG_DEV_NO_FIRST_UNUSED, PDMPCIDEVREG_FUN_NO_FIRST_UNUSED, "pcibridge");
    if (RT_FAILURE(rc))
        return rc;
    pBus->PciDev.Int.s.pfnBridgeConfigRead  = pcibridgeR3ConfigRead;
    pBus->PciDev.Int.s.pfnBridgeConfigWrite = pcibridgeR3ConfigWrite;

    pBus->iDevSearch = 0;

    /*
     * Register SSM handlers. We use the same saved state version as for the host bridge
     * to make changes easier.
     */
    rc = PDMDevHlpSSMRegisterEx(pDevIns, VBOX_PCI_SAVED_STATE_VERSION, sizeof(*pBus) + 16*128, "pgm",
                                NULL, NULL, NULL,
                                NULL, pcibridgeR3SaveExec, NULL,
                                NULL, pcibridgeR3LoadExec, NULL);
    if (RT_FAILURE(rc))
        return rc;

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnDestruct}
 */
static DECLCALLBACK(int)   pcibridgeR3Destruct(PPDMDEVINS pDevIns)
{
    PDEVPCIBUS pBus = PDMINS_2_DATA(pDevIns, PDEVPCIBUS);
    if (pBus->papBridgesR3)
    {
        PDMDevHlpMMHeapFree(pDevIns, pBus->papBridgesR3);
        pBus->papBridgesR3 = NULL;
    }
    return VINF_SUCCESS;
}


/**
 * The device registration structure
 * for the PCI-to-PCI bridge.
 */
const PDMDEVREG g_DevicePCIBridge =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "pcibridge",
    /* szRCMod */
    "VBoxDDRC.rc",
    /* szR0Mod */
    "VBoxDDR0.r0",
    /* pszDescription */
    "82801 Mobile PCI to PCI bridge",
    /* fFlags */
    PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_RC | PDM_DEVREG_FLAGS_R0,
    /* fClass */
    PDM_DEVREG_CLASS_BUS_PCI,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(DEVPCIBUS),
    /* pfnConstruct */
    pcibridgeR3Construct,
    /* pfnDestruct */
    pcibridgeR3Destruct,
    /* pfnRelocate */
    devpciR3BusRelocate,
    /* pfnMemSetup */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    pcibridgeR3Reset,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnQueryInterface */
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

