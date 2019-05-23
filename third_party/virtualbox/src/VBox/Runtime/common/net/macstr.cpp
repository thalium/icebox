/* $Id: macstr.cpp $ */
/** @file
 * IPRT - Command Line Parsing
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
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
#include <iprt/net.h>                   /* must come before getopt.h */
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/ctype.h>
#include <iprt/err.h>
#include <iprt/message.h>
#include <iprt/string.h>


/**
 * Converts an stringified Ethernet MAC address into the RTMAC representation.
 *
 * @returns VINF_SUCCESS on success.
 *
 * @param   pszValue        The value to convert.
 * @param   pAddr           Where to store the result.
 */
RTDECL(int) RTNetStrToMacAddr(const char *pszValue, PRTMAC pAddr)
{
    /*
     * Not quite sure if I should accept stuff like "08::27:::1" here...
     * The code is accepting "::" patterns now, except for for the first
     * and last parts.
     */

    /* first */
    char *pszNext;
    int rc = RTStrToUInt8Ex(RTStrStripL(pszValue), &pszNext, 16, &pAddr->au8[0]);
    if (rc != VINF_SUCCESS && rc != VWRN_TRAILING_CHARS)
        return VERR_GETOPT_INVALID_ARGUMENT_FORMAT;
    if (*pszNext++ != ':')
        return VERR_GETOPT_INVALID_ARGUMENT_FORMAT;

    /* middle */
    for (unsigned i = 1; i < 5; i++)
    {
        if (*pszNext == ':')
            pAddr->au8[i] = 0;
        else
        {
            rc = RTStrToUInt8Ex(pszNext, &pszNext, 16, &pAddr->au8[i]);
            if (rc != VINF_SUCCESS && rc != VWRN_TRAILING_CHARS)
                return rc;
            if (*pszNext != ':')
                return VERR_INVALID_PARAMETER;
        }
        pszNext++;
    }

    /* last */
    rc = RTStrToUInt8Ex(pszNext, &pszNext, 16, &pAddr->au8[5]);
    if (rc != VINF_SUCCESS && rc != VWRN_TRAILING_SPACES)
        return rc;
    pszNext = RTStrStripL(pszNext);
    if (*pszNext)
        return VERR_INVALID_PARAMETER;

    return VINF_SUCCESS;
}
RT_EXPORT_SYMBOL(RTNetStrToMacAddr);
