/* $Id: PciInline.h $ */
/** @file
 * PCI - The PCI Controller And Devices, inline device helpers.
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

#ifndef ___Bus_PciInline_h
#define ___Bus_PciInline_h

DECLINLINE(void) pciDevSetPci2PciBridge(PPDMPCIDEV pDev)
{
    pDev->Int.s.fFlags |= PCIDEV_FLAG_PCI_TO_PCI_BRIDGE;
}

DECLINLINE(bool) pciDevIsPci2PciBridge(PPDMPCIDEV pDev)
{
    return (pDev->Int.s.fFlags & PCIDEV_FLAG_PCI_TO_PCI_BRIDGE) != 0;
}

DECLINLINE(void) pciDevSetPciExpress(PPDMPCIDEV pDev)
{
    pDev->Int.s.fFlags |= PCIDEV_FLAG_PCI_EXPRESS_DEVICE;
}

DECLINLINE(bool) pciDevIsPciExpress(PPDMPCIDEV pDev)
{
    return (pDev->Int.s.fFlags & PCIDEV_FLAG_PCI_EXPRESS_DEVICE) != 0;
}

DECLINLINE(void) pciDevSetMsiCapable(PPDMPCIDEV pDev)
{
    pDev->Int.s.fFlags |= PCIDEV_FLAG_MSI_CAPABLE;
}

DECLINLINE(void) pciDevClearMsiCapable(PPDMPCIDEV pDev)
{
    pDev->Int.s.fFlags &= ~PCIDEV_FLAG_MSI_CAPABLE;
}

DECLINLINE(bool) pciDevIsMsiCapable(PPDMPCIDEV pDev)
{
    return (pDev->Int.s.fFlags & PCIDEV_FLAG_MSI_CAPABLE) != 0;
}

DECLINLINE(void) pciDevSetMsi64Capable(PPDMPCIDEV pDev)
{
    pDev->Int.s.fFlags |= PCIDEV_FLAG_MSI64_CAPABLE;
}

DECLINLINE(void) pciDevClearMsi64Capable(PPDMPCIDEV pDev)
{
    pDev->Int.s.fFlags &= ~PCIDEV_FLAG_MSI64_CAPABLE;
}

DECLINLINE(bool) pciDevIsMsi64Capable(PPDMPCIDEV pDev)
{
    return (pDev->Int.s.fFlags & PCIDEV_FLAG_MSI64_CAPABLE) != 0;
}

DECLINLINE(void) pciDevSetMsixCapable(PPDMPCIDEV pDev)
{
    pDev->Int.s.fFlags |= PCIDEV_FLAG_MSIX_CAPABLE;
}

DECLINLINE(void) pciDevClearMsixCapable(PPDMPCIDEV pDev)
{
    pDev->Int.s.fFlags &= ~PCIDEV_FLAG_MSIX_CAPABLE;
}

DECLINLINE(bool) pciDevIsMsixCapable(PPDMPCIDEV pDev)
{
    return (pDev->Int.s.fFlags & PCIDEV_FLAG_MSIX_CAPABLE) != 0;
}

DECLINLINE(void) pciDevSetPassthrough(PPDMPCIDEV pDev)
{
    pDev->Int.s.fFlags |= PCIDEV_FLAG_PASSTHROUGH;
}

DECLINLINE(void) pciDevClearPassthrough(PPDMPCIDEV pDev)
{
    pDev->Int.s.fFlags &= ~PCIDEV_FLAG_PASSTHROUGH;
}

DECLINLINE(bool) pciDevIsPassthrough(PPDMPCIDEV pDev)
{
    return (pDev->Int.s.fFlags & PCIDEV_FLAG_PASSTHROUGH) != 0;
}

#endif

