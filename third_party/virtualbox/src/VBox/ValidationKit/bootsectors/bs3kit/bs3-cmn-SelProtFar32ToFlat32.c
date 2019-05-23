/* $Id: bs3-cmn-SelProtFar32ToFlat32.c $ */
/** @file
 * BS3Kit - Bs3SelProtFar32ToFlat32
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

#include "bs3kit-template-header.h"


#undef Bs3SelProtFar32ToFlat32
BS3_CMN_DEF(uint32_t, Bs3SelProtFar32ToFlat32,(uint32_t off, uint16_t uSel))
{
    uint32_t    uRet;
    PCX86DESC   pEntry;
    if (!(uSel & X86_SEL_LDT))
         pEntry = &Bs3Gdt[uSel >> X86_SEL_SHIFT];
    else
         pEntry = &Bs3Ldt[uSel >> X86_SEL_SHIFT];
    uRet  = pEntry->Gen.u16BaseLow;
    uRet |= (uint32_t)pEntry->Gen.u8BaseHigh1 << 16;
    uRet |= (uint32_t)pEntry->Gen.u8BaseHigh2 << 24;
    uRet += off;
    return uRet;
}

