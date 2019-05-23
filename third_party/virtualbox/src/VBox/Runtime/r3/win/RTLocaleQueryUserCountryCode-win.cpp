/* $Id: RTLocaleQueryUserCountryCode-win.cpp $ */
/** @file
 * IPRT - RTLocaleQueryUserCountryCode, ring-3, Windows.
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
#include <iprt/win/windows.h>

#include <iprt/locale.h>
#include "internal/iprt.h"

#include <iprt/ctype.h>
#include <iprt/err.h>
#include <iprt/string.h>

#include "internal-r3-win.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
typedef GEOID (WINAPI *PFNGETUSERGEOID)(GEOCLASS);
typedef INT   (WINAPI *PFNGETGEOINFOW)(GEOID,GEOTYPE,LPWSTR,INT,LANGID);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Pointer to GetUserGeoID. */
static PFNGETUSERGEOID  g_pfnGetUserGeoID = NULL;
/** Pointer to GetGeoInfoW. */
static PFNGETGEOINFOW   g_pfnGetGeoInfoW = NULL;
/** Set if we've tried to resolve the APIs. */
static bool volatile    g_fResolvedApis = false;


RTDECL(int) RTLocaleQueryUserCountryCode(char pszCountryCode[3])
{
    /*
     * Get API pointers.
     */
    PFNGETUSERGEOID  pfnGetUserGeoID;
    PFNGETGEOINFOW   pfnGetGeoInfoW;
    if (g_fResolvedApis)
    {
        pfnGetUserGeoID = g_pfnGetUserGeoID;
        pfnGetGeoInfoW  = g_pfnGetGeoInfoW;
    }
    else
    {
        pfnGetUserGeoID = (PFNGETUSERGEOID)GetProcAddress(g_hModKernel32, "GetUserGeoID");
        pfnGetGeoInfoW  = (PFNGETGEOINFOW)GetProcAddress(g_hModKernel32, "GetGeoInfoW");
        g_pfnGetUserGeoID = pfnGetUserGeoID;
        g_pfnGetGeoInfoW  = pfnGetGeoInfoW;
        g_fResolvedApis   = true;
    }

    int rc;
    if (   pfnGetGeoInfoW
        && pfnGetUserGeoID)
    {
        /*
         * Call the API and retrieve the two letter ISO country code.
         */
        GEOID idGeo = pfnGetUserGeoID(GEOCLASS_NATION);
        if (idGeo != GEOID_NOT_AVAILABLE)
        {
            RTUTF16 wszName[16];
            RT_ZERO(wszName);
            DWORD cwcReturned = pfnGetGeoInfoW(idGeo, GEO_ISO2, wszName, RT_ELEMENTS(wszName), LOCALE_NEUTRAL);
            if (   cwcReturned >= 2
                && cwcReturned <= 3
                && wszName[2] == '\0'
                && wszName[1] != '\0'
                && RT_C_IS_ALPHA(wszName[1])
                && wszName[0] != '\0'
                && RT_C_IS_ALPHA(wszName[0]) )
            {
                pszCountryCode[0] = RT_C_TO_UPPER(wszName[0]);
                pszCountryCode[1] = RT_C_TO_UPPER(wszName[1]);
                pszCountryCode[2] = '\0';
                return VINF_SUCCESS;
            }
            AssertMsgFailed(("cwcReturned=%d err=%u wszName='%.16ls'\n", cwcReturned, GetLastError(), wszName));
        }
        rc = VERR_NOT_AVAILABLE;
    }
    else
        rc = VERR_NOT_SUPPORTED;
    pszCountryCode[0] = 'Z';
    pszCountryCode[1] = 'Z';
    pszCountryCode[2] = '\0';
    return rc;
}

