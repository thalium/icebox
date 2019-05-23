/* $Id: bs3-rm-InitGdt.c $ */
/** @file
 * BS3Kit - Bs3InitGdt
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
#define BS3_USE_RM_TEXT_SEG 1
#include "bs3kit-template-header.h"
#include <iprt/asm.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/


BS3_DECL_FAR(void) Bs3InitGdt_rm_far(void)
{
#if 1
    Bs3Gdte_R0_CS16.Gen.u16LimitLow     = Bs3Text16_Size - 1;
    Bs3Gdte_R1_CS16.Gen.u16LimitLow     = Bs3Text16_Size - 1;
    Bs3Gdte_R2_CS16.Gen.u16LimitLow     = Bs3Text16_Size - 1;
    Bs3Gdte_R3_CS16.Gen.u16LimitLow     = Bs3Text16_Size - 1;
#endif
    Bs3Gdte_RMTEXT16_CS.Gen.u16LimitLow = Bs3RmText16_Size - 1;
    Bs3Gdte_X0TEXT16_CS.Gen.u16LimitLow = Bs3X0Text16_Size - 1;
    Bs3Gdte_X1TEXT16_CS.Gen.u16LimitLow = Bs3X1Text16_Size - 1;

    Bs3Gdte_RMTEXT16_CS.Gen.u16BaseLow  = (uint16_t)Bs3RmText16_FlatAddr;
    Bs3Gdte_X0TEXT16_CS.Gen.u16BaseLow  = (uint16_t)Bs3X0Text16_FlatAddr;
    Bs3Gdte_X1TEXT16_CS.Gen.u16BaseLow  = (uint16_t)Bs3X1Text16_FlatAddr;

    Bs3Gdte_RMTEXT16_CS.Gen.u8BaseHigh1 = (uint8_t)(Bs3RmText16_FlatAddr >> 16);
    Bs3Gdte_X0TEXT16_CS.Gen.u8BaseHigh1 = (uint8_t)(Bs3X0Text16_FlatAddr >> 16);
    Bs3Gdte_X1TEXT16_CS.Gen.u8BaseHigh1 = (uint8_t)(Bs3X1Text16_FlatAddr >> 16);
}

