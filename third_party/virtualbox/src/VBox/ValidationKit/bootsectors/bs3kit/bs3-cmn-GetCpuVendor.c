/* $Id: bs3-cmn-GetCpuVendor.c $ */
/** @file
 * BS3Kit - Bs3GetCpuVendor
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

#include <iprt/asm-amd64-x86.h>


#undef Bs3GetCpuVendor
BS3_CMN_DEF(BS3CPUVENDOR, Bs3GetCpuVendor,(void))
{
    if (g_uBs3CpuDetected & BS3CPU_F_CPUID)
    {
        uint32_t uEbx, uEcx, uEdx;
        ASMCpuIdExSlow(0, 0, 0, 0, NULL, &uEbx, &uEcx, &uEdx);
        if (ASMIsIntelCpuEx(uEbx, uEcx, uEdx))
            return BS3CPUVENDOR_INTEL;
        if (ASMIsAmdCpuEx(uEbx, uEcx, uEdx))
            return BS3CPUVENDOR_AMD;
        if (ASMIsViaCentaurCpuEx(uEbx, uEcx, uEdx))
            return BS3CPUVENDOR_VIA;
        return BS3CPUVENDOR_UNKNOWN;
    }
    return BS3CPUVENDOR_INTEL;
}

