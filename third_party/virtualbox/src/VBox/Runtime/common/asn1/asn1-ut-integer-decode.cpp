/* $Id: asn1-ut-integer-decode.cpp $ */
/** @file
 * IPRT - ASN.1, INTEGER Type, Decoding.
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
#include <iprt/asn1.h>

#include <iprt/err.h>
#include <iprt/string.h>

#include <iprt/formats/asn1.h>


RTDECL(int) RTAsn1Integer_DecodeAsn1(PRTASN1CURSOR pCursor, uint32_t fFlags, PRTASN1INTEGER pThis, const char *pszErrorTag)
{
    pThis->uValue.u = 0;
    int rc = RTAsn1CursorReadHdr(pCursor, &pThis->Asn1Core, pszErrorTag);
    if (RT_SUCCESS(rc))
    {
        rc = RTAsn1CursorMatchTagClassFlags(pCursor, &pThis->Asn1Core, ASN1_TAG_INTEGER,
                                            ASN1_TAGCLASS_UNIVERSAL | ASN1_TAGFLAG_PRIMITIVE,
                                            fFlags, pszErrorTag, "INTEGER");
        if (RT_SUCCESS(rc))
        {
            uint32_t offLast = pThis->Asn1Core.cb - 1;
            switch (pThis->Asn1Core.cb)
            {
                default:
                case 8: pThis->uValue.u |= (uint64_t)pThis->Asn1Core.uData.pu8[offLast - 7] << 56; RT_FALL_THRU();
                case 7: pThis->uValue.u |= (uint64_t)pThis->Asn1Core.uData.pu8[offLast - 6] << 48; RT_FALL_THRU();
                case 6: pThis->uValue.u |= (uint64_t)pThis->Asn1Core.uData.pu8[offLast - 5] << 40; RT_FALL_THRU();
                case 5: pThis->uValue.u |= (uint64_t)pThis->Asn1Core.uData.pu8[offLast - 4] << 32; RT_FALL_THRU();
                case 4: pThis->uValue.u |= (uint32_t)pThis->Asn1Core.uData.pu8[offLast - 3] << 24; RT_FALL_THRU();
                case 3: pThis->uValue.u |= (uint32_t)pThis->Asn1Core.uData.pu8[offLast - 2] << 16; RT_FALL_THRU();
                case 2: pThis->uValue.u |= (uint16_t)pThis->Asn1Core.uData.pu8[offLast - 1] <<  8; RT_FALL_THRU();
                case 1: pThis->uValue.u |=           pThis->Asn1Core.uData.pu8[offLast];
            }
            RTAsn1CursorSkip(pCursor, pThis->Asn1Core.cb);
            pThis->Asn1Core.fFlags |= RTASN1CORE_F_PRIMITE_TAG_STRUCT;
            pThis->Asn1Core.pOps    = &g_RTAsn1Integer_Vtable;
            return VINF_SUCCESS;
        }
    }
    RT_ZERO(*pThis);
    return rc;
}


/*
 * Generate code for the associated collection types.
 */
#define RTASN1TMPL_TEMPLATE_FILE "../common/asn1/asn1-ut-integer-template.h"
#include <iprt/asn1-generator-internal-header.h>
#include <iprt/asn1-generator-asn1-decoder.h>

