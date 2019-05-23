/* $Id: bs3-cmn-SelSetup16BitData.c $ */
/** @file
 * BS3Kit - Bs3SelSetup16BitData
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
#include <bs3kit.h>


#undef Bs3SelSetup16BitData
BS3_CMN_DEF(void, Bs3SelSetup16BitData,(X86DESC BS3_FAR *pDesc, uint32_t uBaseAddr))
{
    pDesc->Gen.u16LimitLow = UINT16_C(0xffff);
    pDesc->Gen.u16BaseLow  = (uint16_t)uBaseAddr;
    pDesc->Gen.u8BaseHigh1 = (uint8_t)(uBaseAddr >> 16);
    pDesc->Gen.u4Type      = X86_SEL_TYPE_RW_ACC;
    pDesc->Gen.u1DescType  = 1; /* data/code */
    pDesc->Gen.u2Dpl       = 3;
    pDesc->Gen.u1Present   = 1;
    pDesc->Gen.u4LimitHigh = 0;
    pDesc->Gen.u1Available = 0;
    pDesc->Gen.u1Long      = 0;
    pDesc->Gen.u1DefBig    = 0;
    pDesc->Gen.u1Granularity = 0;
    pDesc->Gen.u8BaseHigh2 = (uint8_t)(uBaseAddr >> 24);
}

