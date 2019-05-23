/* $Id: bs3-cmn-PicUpdateMask.c $ */
/** @file
 * BS3Kit - PIC Setup.
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
#include <iprt/asm-amd64-x86.h>
#include "bs3-cmn-pic.h"


#undef Bs3PicUpdateMask
BS3_CMN_DEF(uint16_t, Bs3PicUpdateMask,(uint16_t fAndMask, uint16_t fOrMask))
{
    uint8_t bPic0Mask = (ASMInU8(BS3_PIC_PORT_MASTER + 1) & (uint8_t)fAndMask) | (uint8_t)fOrMask;
    uint8_t bPic1Mask = (ASMInU8(BS3_PIC_PORT_SLAVE  + 1) & (fAndMask >> 8))   | (fOrMask >> 8);
    ASMOutU8(BS3_PIC_PORT_SLAVE  + 1, bPic1Mask);
    ASMOutU8(BS3_PIC_PORT_MASTER + 1, bPic0Mask);
    return RT_MAKE_U16(bPic0Mask, bPic1Mask);
}

