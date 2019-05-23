/* $Id: utf8-win.cpp $ */
/** @file
 * IPRT - UTF8 helpers.
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
#define LOG_GROUP RTLOGGROUP_UTF8
#include <iprt/win/windows.h>

#include <iprt/string.h>
#include <iprt/alloc.h>
#include <iprt/assert.h>
#include <iprt/err.h>


RTR3DECL(int)  RTStrUtf8ToCurrentCPTag(char **ppszString, const char *pszString, const char *pszTag)
{
    Assert(ppszString);
    Assert(pszString);

    /*
     * Check for zero length input string.
     */
    if (!*pszString)
    {
        *ppszString = (char *)RTMemTmpAllocZTag(sizeof(char), pszTag);
        if (*ppszString)
            return VINF_SUCCESS;
        return VERR_NO_TMP_MEMORY;
    }

    *ppszString = NULL;

    /*
     * Convert to wide char first.
     */
    PRTUTF16 pwszString = NULL;
    int rc = RTStrToUtf16(pszString, &pwszString);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * First calc result string length.
     */
    int cbResult = WideCharToMultiByte(CP_ACP, 0, pwszString, -1, NULL, 0, NULL, NULL);
    if (cbResult > 0)
    {
        /*
         * Alloc space for result buffer.
         */
        LPSTR lpString = (LPSTR)RTMemTmpAllocTag(cbResult, pszTag);
        if (lpString)
        {
            /*
             * Do the translation.
             */
            if (WideCharToMultiByte(CP_ACP, 0, pwszString, -1, lpString, cbResult, NULL, NULL) > 0)
            {
                /* ok */
                *ppszString = lpString;
                RTMemTmpFree(pwszString);
                return VINF_SUCCESS;
            }

            /* translation error */
            int iLastErr = GetLastError();
            AssertMsgFailed(("Unicode to ACP translation failed. lasterr=%d\n", iLastErr));
            rc = RTErrConvertFromWin32(iLastErr);
        }
        else
            rc = VERR_NO_TMP_MEMORY;
        RTMemTmpFree(lpString);
    }
    else
    {
        /* translation error */
        int iLastErr = GetLastError();
        AssertMsgFailed(("Unicode to ACP translation failed lasterr=%d\n", iLastErr));
        rc = RTErrConvertFromWin32(iLastErr);
    }
    RTMemTmpFree(pwszString);
    return rc;
}


RTR3DECL(int)  RTStrCurrentCPToUtf8Tag(char **ppszString, const char *pszString, const char *pszTag)
{
    Assert(ppszString);
    Assert(pszString);
    *ppszString = NULL;

    /** @todo is there a quicker way? Currently: ACP -> UTF-16 -> UTF-8 */

    size_t cch = strlen(pszString);
    if (cch <= 0)
    {
        /* zero length string passed. */
        *ppszString = (char *)RTMemTmpAllocZTag(sizeof(char), pszTag);
        if (*ppszString)
            return VINF_SUCCESS;
        return VERR_NO_TMP_MEMORY;
    }

    /*
     * First calc result string length.
     */
    int rc;
    int cwc = MultiByteToWideChar(CP_ACP, 0, pszString, -1, NULL, 0);
    if (cwc > 0)
    {
        /*
         * Alloc space for result buffer.
         */
        PRTUTF16 pwszString = (PRTUTF16)RTMemTmpAlloc(cwc * sizeof(RTUTF16));
        if (pwszString)
        {
            /*
             * Do the translation.
             */
            if (MultiByteToWideChar(CP_ACP, 0, pszString, -1, pwszString, cwc) > 0)
            {
                /*
                 * Now we got UTF-16, convert it to UTF-8
                 */
                rc = RTUtf16ToUtf8(pwszString, ppszString);
                RTMemTmpFree(pwszString);
                return rc;
            }
            RTMemTmpFree(pwszString);
            /* translation error */
            int iLastErr = GetLastError();
            AssertMsgFailed(("ACP to Unicode translation failed. lasterr=%d\n", iLastErr));
            rc = RTErrConvertFromWin32(iLastErr);
        }
        else
            rc = VERR_NO_TMP_MEMORY;
    }
    else
    {
        /* translation error */
        int iLastErr = GetLastError();
        AssertMsgFailed(("Unicode to ACP translation failed lasterr=%d\n", iLastErr));
        rc = RTErrConvertFromWin32(iLastErr);
    }
    return rc;
}

