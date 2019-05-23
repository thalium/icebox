/* $Id: bs3cpudt.c $ */
/** @file
 * BS3Kit - Tests Bs3CpuDetect_rm.
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

#include "bs3kit.h"
#include <stdio.h>
#include <stdint.h>


unsigned StoreMsw(void);
#pragma aux StoreMsw = \
    ".286" \
    "smsw ax" \
    value [ax];

void LoadMsw(unsigned);
#pragma aux LoadMsw = \
    ".286p" \
    "lmsw ax" \
    parm [ax];

int main()
{
    uint16_t volatile usCpu = Bs3CpuDetect_rm();
    printf("usCpu=%#x\n", usCpu);
    if ((usCpu & BS3CPU_TYPE_MASK) >= BS3CPU_80286)
    {
        printf("(42=%d) msw=%#x (42=%d)\n", 42, StoreMsw(), 42);
        LoadMsw(0);
        printf("lmsw 0 => msw=%#x (42=%d)\n", StoreMsw(), 42);
    }
    return 0;
}

