/* $Id: utf-8-case2.cpp $ */
/** @file
 * IPRT - UTF-8 Case Sensitivity and Folding, Part 2 (requires unidata-flags.cpp).
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
#include <iprt/string.h>
#include "internal/iprt.h"

#include <iprt/uni.h>
#include <iprt/alloc.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include "internal/string.h"


RTDECL(bool) RTStrIsCaseFoldable(const char *psz)
{
    /*
     * Loop the code points in the string, checking them one by one until we
     * find something that can be folded.
     */
    RTUNICP uc;
    do
    {
        int rc = RTStrGetCpEx(&psz, &uc);
        if (RT_SUCCESS(rc))
        {
            if (RTUniCpIsFoldable(uc))
                return true;
        }
        else
        {
            /* bad encoding, just skip it quietly (uc == RTUNICP_INVALID (!= 0)). */
            AssertRC(rc);
        }
    } while (uc != 0);

    return false;
}
RT_EXPORT_SYMBOL(RTStrIsCaseFoldable);


RTDECL(bool) RTStrIsUpperCased(const char *psz)
{
    /*
     * Check that there are no lower case chars in the string.
     */
    RTUNICP uc;
    do
    {
        int rc = RTStrGetCpEx(&psz, &uc);
        if (RT_SUCCESS(rc))
        {
            if (RTUniCpIsLower(uc))
                return false;
        }
        else
        {
            /* bad encoding, just skip it quietly (uc == RTUNICP_INVALID (!= 0)). */
            AssertRC(rc);
        }
    } while (uc != 0);

    return true;
}
RT_EXPORT_SYMBOL(RTStrIsUpperCased);


RTDECL(bool) RTStrIsLowerCased(const char *psz)
{
    /*
     * Check that there are no lower case chars in the string.
     */
    RTUNICP uc;
    do
    {
        int rc = RTStrGetCpEx(&psz, &uc);
        if (RT_SUCCESS(rc))
        {
            if (RTUniCpIsUpper(uc))
                return false;
        }
        else
        {
            /* bad encoding, just skip it quietly (uc == RTUNICP_INVALID (!= 0)). */
            AssertRC(rc);
        }
    } while (uc != 0);

    return true;
}
RT_EXPORT_SYMBOL(RTStrIsLowerCased);

