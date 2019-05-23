/* $Id: pkix-util.cpp $ */
/** @file
 * IPRT - Crypto - Public Key Infrastructure API, Utilities.
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
#include "internal/iprt.h"
#include <iprt/crypto/pkix.h>

#include <iprt/err.h>
#include <iprt/string.h>

#ifdef IPRT_WITH_OPENSSL
# include "internal/iprt-openssl.h"
# include "openssl/evp.h"
#endif




RTDECL(const char *) RTCrPkixGetCiperOidFromSignatureAlgorithm(PCRTASN1OBJID pAlgorithm)
{
    /*
     * This is all hardcoded, at least for the time being.
     */
    if (RTAsn1ObjId_StartsWith(pAlgorithm, RTCR_PKCS1_OID))
    {
        if (RTAsn1ObjIdCountComponents(pAlgorithm) == 7)
            switch (RTAsn1ObjIdGetLastComponentsAsUInt32(pAlgorithm))
            {
                case 2:
                case 3:
                case 4:
                case 5:
                case 11:
                case 12:
                case 13:
                case 14:
                    return RTCR_PKCS1_RSA_OID;
                case 1: AssertFailed();
                    RT_FALL_THRU();
                default:
                    return NULL;
            }
    }
    /*
     * OIW oddballs.
     */
    else if (RTAsn1ObjId_StartsWith(pAlgorithm, "1.3.14.3.2"))
    {
        if (RTAsn1ObjIdCountComponents(pAlgorithm) == 6)
            switch (RTAsn1ObjIdGetLastComponentsAsUInt32(pAlgorithm))
            {
                case 11:
                case 14:
                case 15:
                case 24:
                case 25:
                case 29:
                    return RTCR_PKCS1_RSA_OID;
                default:
                    return NULL;
            }
    }


    return NULL;
}

