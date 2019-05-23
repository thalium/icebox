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

#ifndef ___MSI_COMMON_H___
#define ___MSI_COMMON_H___

typedef CTX_SUFF(PCPDMPCIHLP) PCPDMPCIHLP;

#ifdef IN_RING3
int      MsiR3Init(PPDMPCIDEV pDev, PPDMMSIREG pMsiReg);
void     MsiR3PciConfigWrite(PPDMDEVINS pDevIns, PCPDMPCIHLP pPciHlp, PPDMPCIDEV pDev, uint32_t u32Address, uint32_t val, unsigned len);
#endif
bool     MsiIsEnabled(PPDMPCIDEV pDev);
void     MsiNotify(PPDMDEVINS pDevIns, PCPDMPCIHLP pPciHlp, PPDMPCIDEV pDev, int iVector, int iLevel, uint32_t uTagSrc);

#ifdef IN_RING3
int      MsixR3Init(PCPDMPCIHLP pPciHlp, PPDMPCIDEV pDev, PPDMMSIREG pMsiReg);
void     MsixR3PciConfigWrite(PPDMDEVINS pDevIns, PCPDMPCIHLP pPciHlp, PPDMPCIDEV pDev, uint32_t u32Address, uint32_t val, unsigned len);
#endif
bool     MsixIsEnabled(PPDMPCIDEV pDev);
void     MsixNotify(PPDMDEVINS pDevIns, PCPDMPCIHLP pPciHlp, PPDMPCIDEV pDev, int iVector, int iLevel, uint32_t uTagSrc);

#endif

