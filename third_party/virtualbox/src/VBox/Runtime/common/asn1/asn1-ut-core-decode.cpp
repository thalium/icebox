/* $Id: asn1-ut-core-decode.cpp $ */
/** @file
 * IPRT - ASN.1, Generic Core Type, Decoding.
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


RTDECL(int) RTAsn1Core_DecodeAsn1(PRTASN1CURSOR pCursor, uint32_t fFlags, PRTASN1CORE pThis, const char *pszErrorTag)
{
    int rc = RTAsn1CursorReadHdr(pCursor, pThis, pszErrorTag);
    if (RT_SUCCESS(rc))
    {
        RTAsn1CursorSkip(pCursor, pThis->cb);
        pThis->pOps = &g_RTAsn1Core_Vtable;
        return VINF_SUCCESS;
    }
    RT_NOREF_PV(fFlags);
    RT_ZERO(*pThis);
    return rc;
}


/*
 * Generate code for the associated collection types.
 */
#define RTASN1TMPL_TEMPLATE_FILE "../common/asn1/asn1-ut-core-template.h"
#include <iprt/asn1-generator-internal-header.h>
#include <iprt/asn1-generator-asn1-decoder.h>

