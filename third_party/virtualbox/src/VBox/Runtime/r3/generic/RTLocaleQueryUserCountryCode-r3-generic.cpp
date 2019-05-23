/* $Id: RTLocaleQueryUserCountryCode-r3-generic.cpp $ */
/** @file
 * IPRT - RTLocaleQueryLocaleName, ring-3 generic.
 */

/*
 * Copyright (C) 2017 Oracle Corporation
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
#include <locale.h>

#include <iprt/locale.h>
#include "internal/iprt.h"

#include <iprt/ctype.h>
#include <iprt/err.h>
#include <iprt/string.h>



RTDECL(int) RTLocaleQueryUserCountryCode(char pszCountryCode[3])
{
    static const int s_aiLocales[] =
    {
        LC_ALL,
        LC_CTYPE,
        LC_COLLATE,
        LC_MONETARY,
        LC_NUMERIC,
        LC_TIME
    };

    for (unsigned i = 0; i < RT_ELEMENTS(s_aiLocales); i++)
    {
        const char *pszLocale = setlocale(s_aiLocales[i], NULL);
        if (   pszLocale
            && strlen(pszLocale) >= 5
            && RT_C_IS_ALPHA(pszLocale[0])
            && RT_C_IS_ALPHA(pszLocale[1])
            && pszLocale[2] == '_'
            && RT_C_IS_ALPHA(pszLocale[3])
            && RT_C_IS_ALPHA(pszLocale[4]))
        {
            pszCountryCode[0] = RT_C_TO_UPPER(pszLocale[3]);
            pszCountryCode[1] = RT_C_TO_UPPER(pszLocale[4]);
            pszCountryCode[2] = '\0';
            return VINF_SUCCESS;
        }
    }

    pszCountryCode[0] = 'Z';
    pszCountryCode[1] = 'Z';
    pszCountryCode[2] = '\0';
    return VERR_NOT_AVAILABLE;
}

