/* $Id: strpbrk.cpp $ */
/** @file
 * IPRT - strpbrk().
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


/**
 * Find the first occurrence of a character in pszChars in pszStr.
 *
 * @returns
 */
#ifdef _MSC_VER
# if _MSC_VER >= 1400
_CRTIMP __checkReturn _CONST_RETURN char *  __cdecl strpbrk(__in_z const char *pszStr, __in_z const char *pszChars)
# else
_CRTIMP char * __cdecl strpbrk(const char *pszStr, const char *pszChars)
# endif
#else
char *strpbrk(const char *pszStr, const char *pszChars)
# if defined(__THROW) && !defined(RT_OS_WINDOWS) && !defined(RT_OS_OS2)
    __THROW
# endif
#endif
{
    int chCur;
    while ((chCur = *pszStr++) != '\0')
    {
        int ch;
        const char *psz = pszChars;
        while ((ch = *psz++) != '\0')
            if (ch == chCur)
                return (char *)(pszStr - 1);

    }
    return NULL;
}

