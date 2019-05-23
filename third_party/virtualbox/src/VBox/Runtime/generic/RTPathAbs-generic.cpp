/* $Id: RTPathAbs-generic.cpp $ */
/** @file
 * IPRT - RTPathAbs, generic implementation.
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
#define LOG_GROUP RTLOGGROUP_PATH
#include <iprt/path.h>

#include <iprt/assert.h>
#include <iprt/ctype.h>
#include <iprt/err.h>
#include <iprt/log.h>
#include <iprt/string.h>
#include "internal/path.h"
#include "internal/fs.h"


static char *rtPathSkipRootSpec(char *pszCur)
{
#ifdef HAVE_DRIVE
    if (pszCur[0] && RTPATH_IS_VOLSEP(pszCur[1]) && pszCur[2] == RTPATH_SLASH)
        pszCur += 3;
# ifdef HAVE_UNC
    else if (pszCur[0] == RTPATH_SLASH && pszCur[1] == RTPATH_SLASH)
    {
        pszCur += 2;
        while (*pszCur == RTPATH_SLASH)
            pszCur++;
        if (*pszCur)
        {
            while (*pszCur != RTPATH_SLASH && *pszCur)
                pszCur++;
            if (*pszCur == RTPATH_SLASH)
            {
                pszCur++;
                while (*pszCur != RTPATH_SLASH && *pszCur)
                    pszCur++;
                if (*pszCur == RTPATH_SLASH)
                    pszCur++;
            }
        }
    }
# endif
#else
    if (pszCur[0] == RTPATH_SLASH)
        pszCur += 1;
#endif
    return pszCur;
}


/**
 * Cleans up a path specifier a little bit.
 *
 * This includes removing duplicate slashes, unnecessary single dots, and
 * trailing slashes. Also, replaces all slash characters with RTPATH_SLASH.
 *
 * @returns Length of the cleaned path (in chars).
 * @param   pszPath     The path to cleanup.
 */
static int fsCleanPath(char *pszPath)
{
    char *pszSrc = pszPath;
    char *pszTrg = pszPath;

    /*
     * On windows, you either use on or two slashes at the head of a path,
     * seems like it treats additional slashes as part of the UNC server name.
     * Just change slashes to RTPATH_SLASH and skip them.
     */
    /** @todo check how OS/2 treats unnecessary leading slashes */
    /*int cchIgnoreLeading = 0;*/
#ifdef HAVE_UNC
    if (   RTPATH_IS_SLASH(pszSrc[0])
        && RTPATH_IS_SLASH(pszSrc[1]))
    {
        pszTrg[0] = RTPATH_SLASH;
        pszTrg[1] = RTPATH_SLASH;
        pszTrg += 2;
        pszSrc += 2;
        /*cchIgnoreLeading = 1;*/
        while (RTPATH_IS_SLASH(*pszSrc))
        {
            /*cchIgnoreLeading++;*/
            pszSrc++;
            *pszTrg++ = RTPATH_SLASH;
        }
    }
#endif

    /*
     * Change slashes to RTPATH_SLASH and remove duplicates.
     */
    for (;;)
    {
        char ch = *pszSrc++;
        if (RTPATH_IS_SLASH(ch))
        {
            *pszTrg++ = RTPATH_SLASH;
            for (;;)
            {
                do
                    ch = *pszSrc++;
                while (RTPATH_IS_SLASH(ch));

                /* Remove '/./' and '/.'. */
                if (   ch != '.'
                    || (*pszSrc && !RTPATH_IS_SLASH(*pszSrc)))
                    break;
            }
        }
        *pszTrg = ch;
        if (!ch)
            break;
        pszTrg++;
    }

    return pszTrg - pszPath;
}


RTDECL(int) RTPathAbs(const char *pszPath, char *pszAbsPath, size_t cchAbsPath)
{
    int rc;

    /*
     * Validation.
     */
    AssertPtr(pszAbsPath);
    AssertPtr(pszPath);
    if (RT_UNLIKELY(!*pszPath))
        return VERR_INVALID_PARAMETER; //VERR_INVALID_NAME;

    /*
     * Make a clean working copy of the input.
     */
    size_t cchPath = strlen(pszPath);
    if (cchPath >= RTPATH_MAX)
    {
        LogFlow(("RTPathAbs(%p:{%s}, %p, %d): returns %Rrc\n", pszPath, pszPath, pszAbsPath, cchAbsPath, VERR_FILENAME_TOO_LONG));
        return VERR_FILENAME_TOO_LONG;
    }

    char szTmpPath[RTPATH_MAX];
    memcpy(szTmpPath, pszPath, cchPath + 1);
    size_t cchTmpPath = fsCleanPath(szTmpPath);

    /*
     * Handle "." specially (fsCleanPath does).
     */
    if (szTmpPath[0] == '.')
    {
        if (   cchTmpPath == 1
            || (cchTmpPath == 2 && szTmpPath[1] == RTPATH_SLASH))
        {
            rc = RTPathGetCurrent(pszAbsPath, cchAbsPath);
            if (RT_SUCCESS(rc))
            {
                size_t cch = fsCleanPath(pszAbsPath);
                char  *pszTop = rtPathSkipRootSpec(pszAbsPath);
#if 1
                if ((uintptr_t)&pszAbsPath[cch] > (uintptr_t)pszTop && pszAbsPath[cch - 1] == RTPATH_SLASH)
                    pszAbsPath[cch - 1] = '\0';
#else
                if (   cchTmpPath == 2
                    && (uintptr_t)&pszAbsPath[cch - 1] > (uintptr_t)pszTop && pszAbsPath[cch - 1] != RTPATH_SLASH)
                {
                    if (cch + 1 < cchAbsPath)
                    {
                        pszAbsPath[cch++] = RTPATH_SLASH;
                        pszAbsPath[cch] = '\0';
                    }
                    else
                        rc = VERR_BUFFER_OVERFLOW;
                }
#endif
            }
            return rc;
        }
    }

    /*
     * Do we have an incomplete root spec?  Supply the missing bits.
     */
#ifdef HAVE_DRIVE
    if (   !(szTmpPath[0] && RTPATH_IS_VOLSEP(szTmpPath[1]) && szTmpPath[2] == RTPATH_SLASH)
# ifdef HAVE_UNC
        && !(szTmpPath[0] == RTPATH_SLASH && szTmpPath[1] == RTPATH_SLASH)
# endif
        )
#else
    if (szTmpPath[0] != RTPATH_SLASH)
#endif
    {
        char    szCurDir[RTPATH_MAX];
        size_t  cchCurDir;
        int     offApplyAt;
        bool    fNeedSlash;
#ifdef HAVE_DRIVE
        if (szTmpPath[0] && RTPATH_IS_VOLSEP(szTmpPath[1]) && szTmpPath[2] != RTPATH_SLASH)
        {
            /*
             * Relative to drive specific current directory.
             */
            rc = RTPathGetCurrentOnDrive(szTmpPath[0], szCurDir, sizeof(szCurDir));
            fNeedSlash = true;
            offApplyAt = 2;
        }
# ifdef HAVE_UNC
        else if (szTmpPath[0] == RTPATH_SLASH && szTmpPath[1] != RTPATH_SLASH)
# else
        else if (szTmpPath[0] == RTPATH_SLASH)
# endif
        {
            /*
             * Root of current drive.  This may return a UNC root if we're not
             * standing on a drive but on a UNC share.
             */
            rc = RTPathGetCurrentDrive(szCurDir, sizeof(szCurDir));
            fNeedSlash = false;
            offApplyAt = 0;
        }
        else
#endif
        {
            /*
             * Relative to current directory.
             */
            rc = RTPathGetCurrent(szCurDir, sizeof(szCurDir));
            fNeedSlash = true;
            offApplyAt = 0;
        }
        if (RT_SUCCESS(rc))
        {
            cchCurDir = fsCleanPath(szCurDir);
            if (fNeedSlash && cchCurDir > 0 && szCurDir[cchCurDir - 1] == RTPATH_SLASH)
                fNeedSlash = false;

            if (cchCurDir + fNeedSlash + cchTmpPath - offApplyAt <= RTPATH_MAX)
            {
                memmove(szTmpPath + cchCurDir + fNeedSlash, szTmpPath + offApplyAt, cchTmpPath + 1 - offApplyAt);
                memcpy(szTmpPath, szCurDir, cchCurDir);
                if (fNeedSlash)
                    szTmpPath[cchCurDir] = RTPATH_SLASH;
            }
            else
                rc = VERR_FILENAME_TOO_LONG;
        }
        if (RT_FAILURE(rc))
        {
            LogFlow(("RTPathAbs(%p:{%s}, %p, %d): returns %Rrc\n", pszPath, pszPath, pszAbsPath, cchAbsPath, rc));
            return rc;
        }
    }

    /*
     * Skip past the root spec.
     */
    char *pszCur = rtPathSkipRootSpec(szTmpPath);
    AssertMsgReturn(pszCur != &szTmpPath[0], ("pszCur=%s\n", pszCur), VERR_INTERNAL_ERROR);
    char * const pszTop = pszCur;

    /*
     * Get rid of double dot path components by evaluating them.
     */
    for (;;)
    {
        char const chFirst = pszCur[0];
        if (   chFirst == '.'
            && pszCur[1] == '.'
            && (!pszCur[2] || pszCur[2] == RTPATH_SLASH))
        {
            /* rewind to the previous component if any */
            char *pszPrev = pszCur;
            if ((uintptr_t)pszPrev > (uintptr_t)pszTop)
            {
                pszPrev--;
                while (   (uintptr_t)pszPrev > (uintptr_t)pszTop
                       && pszPrev[-1] != RTPATH_SLASH)
                    pszPrev--;
            }
            if (!pszCur[2])
            {
                if (pszPrev != pszTop)
                    pszPrev[-1] = '\0';
                else
                    *pszPrev = '\0';
                break;
            }
            Assert(pszPrev[-1] == RTPATH_SLASH);
            memmove(pszPrev, pszCur + 3, strlen(pszCur + 3) + 1);
            pszCur = pszPrev - 1;
        }
        else if (   chFirst == '.'
                 && (!pszCur[1] || pszCur[1] == RTPATH_SLASH))
        {
            /* remove unnecessary '.' */
            if (!pszCur[1])
            {
                if (pszCur != pszTop)
                    pszCur[-1] = '\0';
                else
                    *pszCur = '\0';
                break;
            }
            memmove(pszCur, pszCur + 2, strlen(pszCur + 2) + 1);
            continue;
        }
        else
        {
            /* advance to end of component. */
            while (*pszCur && *pszCur != RTPATH_SLASH)
                pszCur++;
        }

        if (!*pszCur)
            break;

        /* skip the slash */
        ++pszCur;
    }

    cchTmpPath = pszCur - szTmpPath;

#if 1
    /*
     * Strip trailing slash if that's what's desired.
     */

    if ((uintptr_t)&szTmpPath[cchTmpPath] > (uintptr_t)pszTop && szTmpPath[cchTmpPath - 1] == RTPATH_SLASH)
        szTmpPath[--cchTmpPath] = '\0';
#endif

    /*
     * Copy the result to the user buffer.
     */
    if (cchTmpPath < cchAbsPath)
    {
        memcpy(pszAbsPath, szTmpPath, cchTmpPath + 1);
        rc = VINF_SUCCESS;
    }
    else
        rc = VERR_BUFFER_OVERFLOW;

    LogFlow(("RTPathAbs(%p:{%s}, %p:{%s}, %d): returns %Rrc\n", pszPath, pszPath, pszAbsPath,
             RT_SUCCESS(rc) ? pszAbsPath : "<failed>", cchAbsPath, rc));
    return rc;
}

