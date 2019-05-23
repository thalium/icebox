/* $Id: MsiCommon.h $ */
/** @file
 * Header for MSI/MSI-X support routines.
 */
/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Maybe belongs to types.h */
#ifdef IN_RING3
typedef PCPDMPCIHLPR3 PCPDMPCIHLP;
#endif

#ifdef IN_RING0
typedef PCPDMPCIHLPR0 PCPDMPCIHLP;
#endif

#ifdef IN_RC
typedef PCPDMPCIHLPRC PCPDMPCIHLP;
#endif

#ifdef IN_RING3
/* Init MSI support in the device. */
int      MsiInit(PPDMPCIDEV pDev, PPDMMSIREG pMsiReg);
#endif

/* If MSI is enabled, so that MSINotify() shall be used for notifications.  */
bool     MsiIsEnabled(PPDMPCIDEV pDev);

/* Device notification (aka interrupt). */
void     MsiNotify(PPDMDEVINS pDevIns, PCPDMPCIHLP pPciHlp, PPDMPCIDEV pDev, int iVector, int iLevel, uint32_t uTagSrc);

#ifdef IN_RING3
/* PCI config space accessors for MSI registers */
void     MsiPciConfigWrite(PPDMDEVINS pDevIns, PCPDMPCIHLP pPciHlp, PPDMPCIDEV pDev, uint32_t u32Address, uint32_t val, unsigned len);
#endif

#ifdef IN_RING3
/* Init MSI-X support in the device. */
int      MsixInit(PCPDMPCIHLP pPciHlp, PPDMPCIDEV pDev, PPDMMSIREG pMsiReg);
#endif

/* If MSI-X is enabled, so that MSIXNotify() shall be used for notifications.  */
bool     MsixIsEnabled(PPDMPCIDEV pDev);

/* Device notification (aka interrupt). */
void     MsixNotify(PPDMDEVINS pDevIns, PCPDMPCIHLP pPciHlp, PPDMPCIDEV pDev, int iVector, int iLevel, uint32_t uTagSrc);

#ifdef IN_RING3
/* PCI config space accessors for MSI-X */
void     MsixPciConfigWrite(PPDMDEVINS pDevIns, PCPDMPCIHLP pPciHlp, PPDMPCIDEV pDev, uint32_t u32Address, uint32_t val, unsigned len);
#endif
