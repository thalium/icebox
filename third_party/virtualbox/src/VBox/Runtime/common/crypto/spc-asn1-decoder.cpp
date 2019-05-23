/* $Id: spc-asn1-decoder.cpp $ */
/** @file
 * IPRT - Crypto - Microsoft SPC / Authenticode, Decoder for ASN.1.
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
#include <iprt/crypto/spc.h>

#include <iprt/err.h>
#include <iprt/string.h>
#include <iprt/uuid.h>

#include "spc-internal.h"


/*
 * One SPC Serialized Object Attribute.
 */

/** Decode the content of the octet string value if known. */
static int rtCrSpcSerializedObject_DecodeMore(PRTASN1CURSOR pCursor, uint32_t fFlags,
                                              PRTCRSPCSERIALIZEDOBJECT pThis, const char *pszErrorTag)

{
    RT_NOREF_PV(fFlags); RT_NOREF_PV(pszErrorTag);

    int rc;
    RTASN1CURSOR SubCursor;
    if (RTUuidCompareStr(pThis->Uuid.Asn1Core.uData.pUuid, RTCRSPCSERIALIZEDOBJECT_UUID_STR) == 0)
    {
        rc = RTAsn1MemAllocZ(&pThis->SerializedData.EncapsulatedAllocation, (void **)&pThis->u.pData, sizeof(*pThis->u.pData));
        if (RT_SUCCESS(rc))
        {
            pThis->SerializedData.pEncapsulated = (PRTASN1CORE)pThis->u.pData;
            pThis->enmType = RTCRSPCSERIALIZEDOBJECTTYPE_ATTRIBUTES;

            rc = RTAsn1CursorInitSubFromCore(pCursor, &pThis->SerializedData.Asn1Core, &SubCursor, "SerializedData");
            if (RT_SUCCESS(rc))
                rc = RTCrSpcSerializedObjectAttributes_DecodeAsn1(&SubCursor, 0, pThis->u.pData, "SD");
        }
    }
    else
        return VINF_SUCCESS;
    if (RT_SUCCESS(rc))
        rc = RTAsn1CursorCheckEnd(&SubCursor);
    return rc;
}

/*
 * Generate the code.
 */
#include <iprt/asn1-generator-asn1-decoder.h>

