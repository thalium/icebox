/* $Id: DoUInt32Div.c $ */
/** @file
 * AHCI host adapter driver to boot from SATA disks.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define IN_RING0
#define ARCH_BITS 16
#include <iprt/types.h>
#include <iprt/uint32.h>


uint32_t __cdecl DoUInt32Div(RTUINT32U Dividend, RTUINT32U Divisor, RTUINT32U __far *pReminder)
{
    RTUINT32U Quotient;
    RTUInt32DivRem(&Dividend, &Divisor, &Quotient, pReminder);
    return Quotient.u;
}

