/* $Id: tstRTCidr.cpp $ */
/** @file
 * IPRT Testcase - IPv4.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
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
#include <iprt/cidr.h>

#include <iprt/err.h>
#include <iprt/initterm.h>
#include <iprt/test.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#define CHECKNETWORK(String, rcExpected, ExpectedNetwork, ExpectedNetMask) \
    do { \
        RTNETADDRIPV4 Network, NetMask; \
        int rc2 = RTCidrStrToIPv4(String, &Network, &NetMask); \
        if ((rcExpected) && !rc2) \
        { \
            RTTestIFailed("at line %d: '%s': expected %Rrc got %Rrc\n", \
                          __LINE__, String, (rcExpected), rc2); \
        } \
        else if (   (rcExpected) != rc2 \
                 || (   rc2 == VINF_SUCCESS \
                     && (   (ExpectedNetwork) != Network.u \
                         || (ExpectedNetMask) != NetMask.u))) \
        { \
            RTTestIFailed("at line %d: '%s': expected %Rrc got %Rrc, expected network %RTnaipv4 got %RTnaipv4, expected netmask %RTnaipv4 got %RTnaipv4\n", \
                          __LINE__, String, rcExpected, rc2, (ExpectedNetwork), Network.u, (ExpectedNetMask), NetMask.u); \
        } \
    } while (0)


int main()
{
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstRTCidr", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);

    CHECKNETWORK("10.0.0/45",      VERR_INVALID_PARAMETER,          0,          0);
    CHECKNETWORK("10.0.0/-45",     VERR_INVALID_PARAMETER,          0,          0);
    CHECKNETWORK("10.0.0/24",                VINF_SUCCESS, 0x0A000000, 0xFFFFFF00);
    CHECKNETWORK("10..0.0/24",     VERR_INVALID_PARAMETER,          0,          0);
    CHECKNETWORK(".10.0.0/24",     VERR_INVALID_PARAMETER,          0,          0);
    CHECKNETWORK("10.0.0//24",     VERR_INVALID_PARAMETER,          0,          0);
    CHECKNETWORK("10.0.0/8",                 VINF_SUCCESS, 0x0A000000, 0xFF000000);
    CHECKNETWORK("10.0.0./24",     VERR_INVALID_PARAMETER,          0,          0);
    CHECKNETWORK("0.1.0/24",       VERR_INVALID_PARAMETER,          0,          0);
    /* RFC 4632 s3.1: n.n.n.0/24, where n is an 8-bit decimal octet value*/
    CHECKNETWORK("10.255.0.0/24",            VINF_SUCCESS, 0x0AFF0000, 0xFFFFFF00);
    CHECKNETWORK("10.1234.0.0/24", VERR_INVALID_PARAMETER,          0,          0);
    CHECKNETWORK("10.256.0.0/24",  VERR_INVALID_PARAMETER,          0,          0);
    CHECKNETWORK("10.0.0/3",       VERR_INVALID_PARAMETER,          0,          0);
    /* RFC 4632 s3.1: legacy "Class A" is n.0.0.0/8 */
    CHECKNETWORK("10.1.2.3/8",     VERR_INVALID_PARAMETER,          0,          0);
    CHECKNETWORK("10.1.2.4/30",              VINF_SUCCESS, 0x0A010204, 0xFFFFFFFC);
    CHECKNETWORK("10.0.0/29",      VERR_INVALID_PARAMETER,          0,          0);
    CHECKNETWORK("10.0.0/240",     VERR_INVALID_PARAMETER,          0,          0);
    CHECKNETWORK("10.0.0/24.",     VERR_INVALID_PARAMETER,          0,          0);
    /* RFC 4632 s3.1: legacy "Class B" is n.n.0.0/16 */
    CHECKNETWORK("10.1.2/16",      VERR_INVALID_PARAMETER,          0,          0);
    CHECKNETWORK("10.1/16",                  VINF_SUCCESS, 0x0A010000, 0xFFFF0000);
    CHECKNETWORK("10.1.0.0/16",              VINF_SUCCESS, 0x0A010000, 0xFFFF0000);
    CHECKNETWORK("10.1.0.0.0/16",  VERR_INVALID_PARAMETER,          0,          0);
    CHECKNETWORK("1.2.3.4",                  VINF_SUCCESS, 0x01020304, 0xFFFFFFFF);
    CHECKNETWORK("1.2.3.255",                VINF_SUCCESS, 0x010203FF, 0xFFFFFFFF);
    CHECKNETWORK("1.2.3.256",      VERR_INVALID_PARAMETER,          0,          0);
    CHECKNETWORK("10.1.255/24",              VINF_SUCCESS, 0x0A01FF00, 0xFFFFFF00);
    CHECKNETWORK("10.1.254/24",              VINF_SUCCESS, 0x0A01FE00, 0xFFFFFF00);
    CHECKNETWORK("10.255.1/24",              VINF_SUCCESS, 0x0AFF0100, 0xFFFFFF00);
    CHECKNETWORK("10.255.1.1/24",  VERR_INVALID_PARAMETER,          0,          0);
    CHECKNETWORK("1.2",            VERR_INVALID_PARAMETER,          0,          0);
    CHECKNETWORK("1.2.3.4.5",      VERR_INVALID_PARAMETER,          0,          0);

    return RTTestSummaryAndDestroy(hTest);
}

