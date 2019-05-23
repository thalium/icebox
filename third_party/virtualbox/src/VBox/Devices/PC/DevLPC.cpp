/* $Id: DevLPC.cpp $ */
/** @file
 * DevLPC - LPC device emulation
 *
 * @todo This needs to be _replaced_ by a proper chipset device one day. There
 *       are less than 10 C/C++ statements in this file doing active emulation.
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
 *  Low Pin Count emulation
 *
 *  Copyright (c) 2007 Alexander Graf
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * *****************************************************************
 *
 * This driver emulates an ICH-7 LPC partially. The LPC is basically the
 * same as the ISA-bridge in the existing PIIX implementation, but
 * more recent and includes support for HPET and Power Management.
 *
 */

/*
 * Oracle LGPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL or LGPL is available it will apply instead, Oracle elects to use only
 * the Lesser General Public License version 2.1 (LGPLv2) at this time for any software where
 * a choice of LGPL license versions is made available with the language indicating
 * that LGPLv2 or any later version may be used, or where a choice of which version
 * of the LGPL is applied is otherwise unspecified.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEV_LPC
#include <VBox/vmm/pdmdev.h>
#include <VBox/log.h>
#include <VBox/vmm/stam.h>
#include <iprt/assert.h>
#include <iprt/string.h>

#include "VBoxDD2.h"

#define RCBA_BASE                UINT32_C(0xFED1C000)

typedef struct
{
    /** PCI device structure. */
    PDMPCIDEV      dev;

    /** Pointer to the device instance. - R3 ptr. */
    PPDMDEVINSR3   pDevIns;

    /* So far, not much of a state */
} LPCState;


#ifndef VBOX_DEVICE_STRUCT_TESTCASE


static uint32_t rcba_ram_readl(LPCState* s, RTGCPHYS addr)
{
    RT_NOREF1(s);
    Log(("rcba_read at %llx\n", (uint64_t)addr));
    int32_t iIndex = (addr - RCBA_BASE);
    uint32_t value = 0;

    /* This is the HPET config pointer, HPAS in DSDT */
    switch (iIndex)
    {
       case 0x3404:
          Log(("rcba_read HPET_CONFIG_POINTER\n"));
          value = 0xf0; /*  enabled at 0xfed00000 */
          break;
       case 0x3410:
          /* This is the HPET config pointer */
          Log(("rcba_read GCS\n"));
          value = 0;
          break;
       default:
          Log(("Unknown RCBA read\n"));
          break;
    }

    return value;
}

static void rcba_ram_writel(LPCState* s, RTGCPHYS addr, uint32_t value)
{
    RT_NOREF2(s, value);
    Log(("rcba_write %llx = %#x\n", (uint64_t)addr, value));
    int32_t iIndex = (addr - RCBA_BASE);

    switch (iIndex)
    {
       case 0x3410:
          Log(("rcba_write GCS\n"));
          break;
       default:
          Log(("Unknown RCBA write\n"));
          break;
    }
}

/**
 * I/O handler for memory-mapped read operations.
 *
 * @returns VBox status code.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   GCPhysAddr  Physical address (in GC) where the read starts.
 * @param   pv          Where to store the result.
 * @param   cb          Number of bytes read.
 * @thread  EMT
 */
PDMBOTHCBDECL(int)  lpcMMIORead(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void *pv, unsigned cb)
{
    RT_NOREF2(pvUser, cb);
    LPCState *s = PDMINS_2_DATA(pDevIns, LPCState*);
    Assert(cb == 4); Assert(!(GCPhysAddr & 3));
    *(uint32_t*)pv = rcba_ram_readl(s, GCPhysAddr);
    return VINF_SUCCESS;
}

/**
 * Memory mapped I/O Handler for write operations.
 *
 * @returns VBox status code.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   GCPhysAddr  Physical address (in GC) where the read starts.
 * @param   pv          Where to fetch the value.
 * @param   cb          Number of bytes to write.
 * @thread  EMT
 */
PDMBOTHCBDECL(int) lpcMMIOWrite(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void const *pv, unsigned cb)
{
    RT_NOREF1(pvUser);
    LPCState *s = PDMINS_2_DATA(pDevIns, LPCState*);

    switch (cb)
    {
        case 1:
        case 2:
            break;
        case 4:
            rcba_ram_writel(s, GCPhysAddr, *(uint32_t *)pv);
            break;

        default:
            AssertReleaseMsgFailed(("cb=%d\n", cb)); /* for now we assume simple accesses. */
            return VERR_INTERNAL_ERROR;
    }
    return VINF_SUCCESS;
}

#ifdef IN_RING3

/**
 * Info handler, device version.
 *
 * @param   pDevIns     Device instance which registered the info.
 * @param   pHlp        Callback functions for doing output.
 * @param   pszArgs     Argument string. Optional and specific to the handler.
 */
static DECLCALLBACK(void) lpcInfo(PPDMDEVINS pDevIns, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    RT_NOREF1(pszArgs);
    LPCState   *pThis = PDMINS_2_DATA(pDevIns, LPCState *);
    LogFlow(("lpcInfo: \n"));

    if (pThis->dev.abConfig[0xde] == 0xbe && pThis->dev.abConfig[0xad] == 0xef)
        pHlp->pfnPrintf(pHlp, "APIC backdoor activated\n");
    else
        pHlp->pfnPrintf(pHlp, "APIC backdoor closed: %02x %02x\n",
                        pThis->dev.abConfig[0xde], pThis->dev.abConfig[0xad]);


    for (int iLine = 0; iLine < 8; ++iLine)
    {

        int      iBase = iLine < 4 ? 0x60 : 0x64;
        uint8_t  iMap = PCIDevGetByte(&pThis->dev, iBase + iLine);

        if ((iMap & 0x80) != 0)
            pHlp->pfnPrintf(pHlp, "PIRQ%c disabled\n", 'A' + iLine);
        else
            pHlp->pfnPrintf(pHlp, "PIRQ%c -> IRQ%d\n", 'A' + iLine, iMap & 0xf);
    }
}

/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) lpcConstruct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    RT_NOREF2(iInstance, pCfg);
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);
    LPCState   *pThis = PDMINS_2_DATA(pDevIns, LPCState *);
    int         rc;
    Assert(iInstance == 0);

    pThis->pDevIns = pDevIns;

    /*
     * Register the PCI device.
     */
    PCIDevSetVendorId         (&pThis->dev, 0x8086); /* Intel */
    PCIDevSetDeviceId         (&pThis->dev, 0x27b9);
    PCIDevSetCommand          (&pThis->dev, PCI_COMMAND_IOACCESS | PCI_COMMAND_MEMACCESS | PCI_COMMAND_BUSMASTER);
    PCIDevSetRevisionId       (&pThis->dev, 0x02);
    PCIDevSetClassSub         (&pThis->dev, 0x01); /* PCI-to-ISA Bridge */
    PCIDevSetClassBase        (&pThis->dev, 0x06); /* Bridge */
    PCIDevSetHeaderType       (&pThis->dev, 0x80); /* normal, multifunction device (so that other devices can be its functions) */
    PCIDevSetSubSystemVendorId(&pThis->dev, 0x8086);
    PCIDevSetSubSystemId      (&pThis->dev, 0x7270);
    PCIDevSetInterruptPin     (&pThis->dev, 0x00); /* The LPC device itself generates no interrupts */
    PCIDevSetStatus           (&pThis->dev, 0x0200); /* PCI_status_devsel_medium */

    /** @todo rewrite using PCI accessors; Update, rewrite this device from
     *        scratch! Possibly against ICH9 or something else matching our
     *        chipset of choice. (Note that the exteremely partial emulation here
     *        is supposed to be of ICH7 if what's on the top of the file is
     *        anything to go by.) */
    /* See p. 427 of ICH9 specification for register description */

    /* 40h - 43h PMBASE 40-43 ACPI Base Address */
    pThis->dev.abConfig[0x40] = 0x01; /* IO space */
    pThis->dev.abConfig[0x41] = 0x80; /* base address / 128, see DevACPI.cpp */

    /* 44h       ACPI_CNTL    ACPI Control */
    pThis->dev.abConfig[0x44] = 0x00 | (1<<7); /* SCI is IRQ9, ACPI enabled */
    /* 48h–4Bh   GPIOBASE     GPIO Base Address */

    /* 4C        GC           GPIO Control */
    pThis->dev.abConfig[0x4c] = 0x4d;
    /* ???? */
    pThis->dev.abConfig[0x4e] = 0x03;
    pThis->dev.abConfig[0x4f] = 0x00;

    /* 60h-63h PIRQ[n]_ROUT PIRQ[A-D] Routing Control */
    pThis->dev.abConfig[0x60] = 0x0b; /* PCI A -> IRQ 11 */
    pThis->dev.abConfig[0x61] = 0x09; /* PCI B -> IRQ 9  */
    pThis->dev.abConfig[0x62] = 0x0b; /* PCI C -> IRQ 11 */
    pThis->dev.abConfig[0x63] = 0x09; /* PCI D -> IRQ 9  */

    /* 64h SIRQ_CNTL Serial IRQ Control 10h R/W, RO */
    pThis->dev.abConfig[0x64] = 0x10;

    /* 68h-6Bh PIRQ[n]_ROUT PIRQ[E-H] Routing Control  */
    pThis->dev.abConfig[0x68] = 0x80;
    pThis->dev.abConfig[0x69] = 0x80;
    pThis->dev.abConfig[0x6A] = 0x80;
    pThis->dev.abConfig[0x6B] = 0x80;

    /* 6C-6Dh     LPC_IBDF  IOxAPIC Bus:Device:Function   00F8h     R/W */
    pThis->dev.abConfig[0x70] = 0x80;
    pThis->dev.abConfig[0x76] = 0x0c;
    pThis->dev.abConfig[0x77] = 0x0c;
    pThis->dev.abConfig[0x78] = 0x02;
    pThis->dev.abConfig[0x79] = 0x00;
    /* 80h        LPC_I/O_DEC I/O Decode Ranges           0000h     R/W */
    /* 82h-83h    LPC_EN   LPC I/F Enables                0000h     R/W */
    /* 84h-87h    GEN1_DEC   LPC I/F Generic Decode Range 1 00000000h   R/W */
    /* 88h-8Bh    GEN2_DEC LPC I/F Generic Decode Range 2 00000000h R/W */
    /* 8Ch-8Eh    GEN3_DEC LPC I/F Generic Decode Range 3 00000000h R/W */
    /* 90h-93h    GEN4_DEC LPC I/F Generic Decode Range 4 00000000h R/W */

    /* A0h-CFh    Power Management */
    pThis->dev.abConfig[0xa0] = 0x08;
    pThis->dev.abConfig[0xa2] = 0x00;
    pThis->dev.abConfig[0xa3] = 0x00;
    pThis->dev.abConfig[0xa4] = 0x00;
    pThis->dev.abConfig[0xa5] = 0x00;
    pThis->dev.abConfig[0xa6] = 0x00;
    pThis->dev.abConfig[0xa7] = 0x00;
    pThis->dev.abConfig[0xa8] = 0x0f;
    pThis->dev.abConfig[0xaa] = 0x00;
    pThis->dev.abConfig[0xab] = 0x00;
    pThis->dev.abConfig[0xac] = 0x00;
    pThis->dev.abConfig[0xae] = 0x00;

    /* D0h-D3h   FWH_SEL1  Firmware Hub Select 1  */
    /* D4h-D5h   FWH_SEL2  Firmware Hub Select 2 */
    /* D8h-D9h   FWH_DEC_EN1 Firmware Hub Decode Enable 1 */
    /* DCh       BIOS_CNTL BIOS Control */
    /* E0h-E1h   FDCAP     Feature Detection Capability ID   */
    /* E2h       FDLEN     Feature Detection Capability Length */
    /* E3h       FDVER     Feature Detection Version  */
    /* E4h-EBh   FDVCT     Feature Vector Description */

    /* F0h-F3h RCBA Root Complex Base Address */
    pThis->dev.abConfig[0xf0] = RT_BYTE1(RCBA_BASE | 1); /* enabled */
    pThis->dev.abConfig[0xf1] = RT_BYTE2(RCBA_BASE);
    pThis->dev.abConfig[0xf2] = RT_BYTE3(RCBA_BASE);
    pThis->dev.abConfig[0xf3] = RT_BYTE4(RCBA_BASE);

    rc = PDMDevHlpPCIRegisterEx(pDevIns, &pThis->dev, PDMPCIDEVREG_CFG_PRIMARY, PDMPCIDEVREG_F_NOT_MANDATORY_NO,
                                31 /*uPciDevNo*/, 0 /*uPciFunNo*/, "lpc");
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Register the MMIO regions.
     */
    rc = PDMDevHlpMMIORegister(pDevIns, RCBA_BASE, 0x4000, pThis,
                               IOMMMIO_FLAGS_READ_DWORD | IOMMMIO_FLAGS_WRITE_PASSTHRU,
                               lpcMMIOWrite, lpcMMIORead, "LPC Memory");
    if (RT_FAILURE(rc))
        return rc;

    /* No state in the LPC right now */

    /**
     * @todo: Register statistics.
     */
    PDMDevHlpDBGFInfoRegister(pDevIns, "lpc", "Display LPC status. (no arguments)", lpcInfo);

    return VINF_SUCCESS;
}


/**
 * The device registration structure.
 */
const PDMDEVREG g_DeviceLPC =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "lpc",
    /* szRCMod */
    "VBoxDD2RC.rc",
    /* szR0Mod */
    "VBoxDD2R0.r0",
    /* pszDescription */
    "Low Pin Count (LPC) Bus",
    /* fFlags */
    PDM_DEVREG_FLAGS_HOST_BITS_DEFAULT | PDM_DEVREG_FLAGS_GUEST_BITS_32_64 | PDM_DEVREG_FLAGS_PAE36,
    /* fClass */
    PDM_DEVREG_CLASS_MISC,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(LPCState),
    /* pfnConstruct */
    lpcConstruct,
    /* pfnDestruct */
    NULL,
    /* pfnRelocate */
    NULL,
    /* pfnMemSetup */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    NULL,
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

#endif /* VBOX_DEVICE_STRUCT_TESTCASE */
