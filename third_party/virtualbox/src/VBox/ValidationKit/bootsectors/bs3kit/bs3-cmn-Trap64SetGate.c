/* $Id: bs3-cmn-Trap64SetGate.c $ */
/** @file
 * BS3Kit - Bs3Trap64SetGate
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "bs3kit-template-header.h"


#undef Bs3Trap64SetGate
BS3_CMN_DEF(void, Bs3Trap64SetGate,(uint8_t iIdt, uint8_t bType, uint8_t bDpl, uint16_t uSel, uint64_t off, uint8_t bIst))
{
    X86DESC64 BS3_FAR *pIdte = &Bs3Idt64[iIdt];

    BS3_ASSERT(bDpl <= 3);
    BS3_ASSERT(bType <= 15);
    BS3_ASSERT(bIst <= 7);
    pIdte->Gate.u16OffsetLow    = (uint16_t)off;
    pIdte->Gate.u16OffsetHigh   = (uint16_t)((uint32_t)off >> 16);
    pIdte->Gate.u32OffsetTop    = (uint32_t)(off >> 32);
    pIdte->Gate.u16Sel          = uSel;
    pIdte->Gate.u3IST           = bIst;
    pIdte->Gate.u4Type          = bType;
    pIdte->Gate.u2Dpl           = bDpl;
    pIdte->Gate.u5Reserved      = 0;
    pIdte->Gate.u1DescType      = 0; /* system */
    pIdte->Gate.u1Present       = 1;
    pIdte->Gate.u32Reserved     = 0;

}

