/* $Id: x509-init.cpp $ */
/** @file
 * IPRT - Crypto - X.509, Initialization API.
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
#include <iprt/crypto/x509.h>

#include <iprt/err.h>
#include <iprt/string.h>
#include <iprt/uni.h>

#include "x509-internal.h"


static int rtCrX509Extension_ExtnValue_Clone(PRTCRX509EXTENSION pThis, PCRTCRX509EXTENSION pSrc)
{
    pThis->enmValue = pSrc->enmValue;
    return VINF_SUCCESS;
}


RTDECL(int) RTCrX509Name_RecodeAsUtf8(PRTCRX509NAME pThis, PCRTASN1ALLOCATORVTABLE pAllocator)
{
    uint32_t                            cRdns = pThis->cItems;
    PRTCRX509RELATIVEDISTINGUISHEDNAME *ppRdn = pThis->papItems;
    while (cRdns-- > 0)
    {
        PRTCRX509RELATIVEDISTINGUISHEDNAME const pRdn     = *ppRdn;
        uint32_t                                 cAttribs = pRdn->cItems;
        PRTCRX509ATTRIBUTETYPEANDVALUE          *ppAttrib = pRdn->papItems;
        while (cAttribs-- > 0)
        {
            PRTCRX509ATTRIBUTETYPEANDVALUE const pAttrib = *ppAttrib;
            if (pAttrib->Value.enmType == RTASN1TYPE_STRING)
            {
                int rc = RTAsn1String_RecodeAsUtf8(&pAttrib->Value.u.String, pAllocator);
                if (RT_FAILURE(rc))
                    return rc;
            }
            ppAttrib++;
        }
        ppRdn++;
    }
    return VINF_SUCCESS;
}


/*
 * Generate the code.
 */
#include <iprt/asn1-generator-init.h>

