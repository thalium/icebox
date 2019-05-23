/* $Id: bs3-cmn-StrPrintf.c $ */
/** @file
 * BS3Kit - Bs3StrPrintf, Bs3StrPrintfV
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
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
#include "bs3kit-template-header.h"
#include <iprt/ctype.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
typedef struct BS3STRPRINTFSTATE
{
    /** Current buffer position. */
    char   *pchBuf;
    /** Number of bytes left in the buffer.  */
    size_t  cchLeft;
} BS3STRPRINTFSTATE;
typedef BS3STRPRINTFSTATE BS3_FAR *PBS3STRPRINTFSTATE;



static BS3_DECL_CALLBACK(size_t) bs3StrPrintfFmtOutput(char ch, void BS3_FAR *pvUser)
{
    PBS3STRPRINTFSTATE pState = (PBS3STRPRINTFSTATE)pvUser;
    if (ch)
    {
        /* Put to the buffer if there is place for this char and a terminator. */
        if (pState->cchLeft > 1)
        {
            pState->cchLeft--;
            *pState->pchBuf++ = ch;
        }

        /* Always return 1. */
        return 1;
    }

    /* Terminate the string. */
    if (pState->cchLeft)
    {
        pState->cchLeft--;
        *pState->pchBuf++ = '\0';
    }
    return 0;
}


#undef Bs3StrPrintfV
BS3_CMN_DEF(size_t, Bs3StrPrintfV,(char BS3_FAR *pszBuf, size_t cchBuf, const char BS3_FAR *pszFormat, va_list BS3_FAR va))
{
    BS3STRPRINTFSTATE State;
    State.pchBuf  = pszBuf;
    State.cchLeft = cchBuf;
    return Bs3StrFormatV(pszFormat, va, bs3StrPrintfFmtOutput, &State);
}


#undef Bs3StrPrintf
BS3_CMN_DEF(size_t, Bs3StrPrintf,(char BS3_FAR *pszBuf, size_t cchBuf, const char BS3_FAR *pszFormat, ...))
{
    size_t cchRet;
    va_list va;
    va_start(va, pszFormat);
    cchRet = BS3_CMN_NM(Bs3StrPrintfV)(pszBuf, cchBuf, pszFormat, va);
    va_end(va);
    return cchRet;
}

