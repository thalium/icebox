/* $Id: strstrip.cpp $ */
/** @file
 * IPRT - String Stripping and Trimming.
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

#include <iprt/ctype.h>
#include <iprt/string.h>


/**
 * Strips blankspaces from both ends of the string.
 *
 * @returns Pointer to first non-blank char in the string.
 * @param   psz     The string to strip.
 */
RTDECL(char *) RTStrStrip(char *psz)
{
    /* left */
    while (RT_C_IS_SPACE(*psz))
        psz++;

    /* right */
    char *pszEnd = strchr(psz, '\0');
    while (--pszEnd > psz && RT_C_IS_SPACE(*pszEnd))
        *pszEnd = '\0';

    return psz;
}
RT_EXPORT_SYMBOL(RTStrStrip);


/**
 * Strips blankspaces from the start of the string.
 *
 * @returns Pointer to first non-blank char in the string.
 * @param   psz     The string to strip.
 */
RTDECL(char *) RTStrStripL(const char *psz)
{
    /* left */
    while (RT_C_IS_SPACE(*psz))
        psz++;

    return (char *)psz;
}
RT_EXPORT_SYMBOL(RTStrStripL);


/**
 * Strips blankspaces from the end of the string.
 *
 * @returns psz.
 * @param   psz     The string to strip.
 */
RTDECL(char *) RTStrStripR(char *psz)
{
    /* right */
    char *pszEnd = strchr(psz, '\0');
    while (--pszEnd > psz && RT_C_IS_SPACE(*pszEnd))
        *pszEnd = '\0';

    return psz;
}
RT_EXPORT_SYMBOL(RTStrStripR);

