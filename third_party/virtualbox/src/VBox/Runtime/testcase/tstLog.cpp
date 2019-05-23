/* $Id: tstLog.cpp $ */
/** @file
 * IPRT Testcase - Log Formatting.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
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
#include <iprt/log.h>
#include <iprt/initterm.h>
#include <iprt/err.h>

#include <stdio.h>

int main()
{
    RTR3InitExeNoArguments(0);
    printf("tstLog: Requires manual inspection of the log output!\n");
    RTLogPrintf("%%Rrc %d: %Rrc\n", VERR_INVALID_PARAMETER, VERR_INVALID_PARAMETER);
    RTLogPrintf("%%Rrs %d: %Rrs\n", VERR_INVALID_PARAMETER, VERR_INVALID_PARAMETER);
    RTLogPrintf("%%Rrf %d: %Rrf\n", VERR_INVALID_PARAMETER, VERR_INVALID_PARAMETER);
    RTLogPrintf("%%Rra %d: %Rra\n", VERR_INVALID_PARAMETER, VERR_INVALID_PARAMETER);

    static uint8_t au8Hex[256];
    for (unsigned iHex = 0; iHex < sizeof(au8Hex); iHex++)
        au8Hex[iHex] = (uint8_t)iHex;
    RTLogPrintf("%%Rhxs   : %Rhxs\n", &au8Hex[0]);
    RTLogPrintf("%%.32Rhxs: %.32Rhxs\n", &au8Hex[0]);

    RTLogPrintf("%%Rhxd   :\n%Rhxd\n", &au8Hex[0]);
    RTLogPrintf("%%.64Rhxd:\n%.64Rhxd\n", &au8Hex[0]);
    RTLogPrintf("%%.*Rhxd:\n%.*Rhxd\n", 64, &au8Hex[0]);
    RTLogPrintf("%%32.256Rhxd : \n%32.256Rhxd\n", &au8Hex[0]);
    RTLogPrintf("%%32.*Rhxd : \n%32.*Rhxd\n", 256, &au8Hex[0]);
    RTLogPrintf("%%7.32Rhxd : \n%7.32Rhxd\n", &au8Hex[0]);
    RTLogPrintf("%%7.*Rhxd : \n%7.*Rhxd\n", 32, &au8Hex[0]);
    RTLogPrintf("%%*.*Rhxd : \n%*.*Rhxd\n", 7, 32, &au8Hex[0]);

    RTLogPrintf("%%RGp: %RGp\n", (RTGCPHYS)0x87654321);
    RTLogPrintf("%%RGv: %RGv\n", (RTGCPTR)0x87654321);
    RTLogPrintf("%%RHp: %RHp\n", (RTGCPHYS)0x87654321);
    RTLogPrintf("%%RHv: %RHv\n", (RTGCPTR)0x87654321);

    RTLogPrintf("%%RI8 : %RI8\n", (uint8_t)88);
    RTLogPrintf("%%RI16: %RI16\n", (uint16_t)16016);
    RTLogPrintf("%%RI32: %RI32\n", _1G);
    RTLogPrintf("%%RI64: %RI64\n", _1E);

    RTLogPrintf("%%RU8 : %RU8\n", (uint8_t)88);
    RTLogPrintf("%%RU16: %RU16\n", (uint16_t)16016);
    RTLogPrintf("%%RU32: %RU32\n", _2G32);
    RTLogPrintf("%%RU64: %RU64\n", _2E);

    RTLogPrintf("%%RX8 : %RX8 %#RX8\n",   (uint8_t)88, (uint8_t)88);
    RTLogPrintf("%%RX16: %RX16 %#RX16\n", (uint16_t)16016, (uint16_t)16016);
    RTLogPrintf("%%RX32: %RX32 %#RX32\n", _2G32, _2G32);
    RTLogPrintf("%%RX64: %RX64 %#RX64\n", _2E, _2E);

    RTLogFlush(NULL);

    return 0;
}

