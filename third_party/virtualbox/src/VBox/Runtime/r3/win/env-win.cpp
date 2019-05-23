/* $Id: env-win.cpp $ */
/** @file
 * IPRT - Environment, Posix.
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
#include <iprt/env.h>

#include <iprt/alloca.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/mem.h>

#include <stdlib.h>
#include <errno.h>


RTDECL(bool) RTEnvExistsBad(const char *pszVar)
{
    return RTEnvGetBad(pszVar) != NULL;
}


RTDECL(bool) RTEnvExist(const char *pszVar)
{
    return RTEnvExistsBad(pszVar);
}


RTDECL(bool) RTEnvExistsUtf8(const char *pszVar)
{
    AssertReturn(strchr(pszVar, '=') == NULL, false);

    PRTUTF16 pwszVar;
    int rc = RTStrToUtf16(pszVar, &pwszVar);
    AssertRCReturn(rc, false);
    bool fRet = _wgetenv(pwszVar) != NULL;
    RTUtf16Free(pwszVar);
    return fRet;
}


RTDECL(const char *) RTEnvGetBad(const char *pszVar)
{
    AssertReturn(strchr(pszVar, '=') == NULL, NULL);
    return getenv(pszVar);
}


RTDECL(const char *) RTEnvGet(const char *pszVar)
{
    return RTEnvGetBad(pszVar);
}

RTDECL(int) RTEnvGetUtf8(const char *pszVar, char *pszValue, size_t cbValue, size_t *pcchActual)
{
    AssertPtrReturn(pszVar, VERR_INVALID_POINTER);
    AssertPtrNullReturn(pszValue, VERR_INVALID_POINTER);
    AssertReturn(pszValue || !cbValue, VERR_INVALID_PARAMETER);
    AssertPtrNullReturn(pcchActual, VERR_INVALID_POINTER);
    AssertReturn(pcchActual || (pszValue && cbValue), VERR_INVALID_PARAMETER);
    AssertReturn(strchr(pszVar, '=') == NULL, VERR_ENV_INVALID_VAR_NAME);

    if (pcchActual)
        *pcchActual = 0;

    PRTUTF16 pwszVar;
    int rc = RTStrToUtf16(pszVar, &pwszVar);
    AssertRCReturn(rc, rc);

    /** @todo Consider _wgetenv_s or GetEnvironmentVariableW here to avoid the
     *        potential race with a concurrent _wputenv/_putenv. */
    PCRTUTF16 pwszValue = _wgetenv(pwszVar);
    RTUtf16Free(pwszVar);
    if (pwszValue)
    {
        if (cbValue)
            rc = RTUtf16ToUtf8Ex(pwszValue, RTSTR_MAX, &pszValue, cbValue, pcchActual);
        else
            rc = RTUtf16CalcUtf8LenEx(pwszValue, RTSTR_MAX, pcchActual);
    }
    else
        rc = VERR_ENV_VAR_NOT_FOUND;
    return rc;
}


RTDECL(int) RTEnvPutBad(const char *pszVarEqualValue)
{
    /** @todo putenv is a source memory leaks. deal with this on a per system basis. */
    if (!putenv((char *)pszVarEqualValue))
        return 0;
    return RTErrConvertFromErrno(errno);
}


RTDECL(int) RTEnvPut(const char *pszVarEqualValue)
{
    return RTEnvPutBad(pszVarEqualValue);
}


RTDECL(int) RTEnvPutUtf8(const char *pszVarEqualValue)
{
    PRTUTF16 pwszVarEqualValue;
    int rc = RTStrToUtf16(pszVarEqualValue, &pwszVarEqualValue);
    if (RT_SUCCESS(rc))
    {
        if (!_wputenv(pwszVarEqualValue))
            rc = VINF_SUCCESS;
        else
            rc = RTErrConvertFromErrno(errno);
        RTUtf16Free(pwszVarEqualValue);
    }
    return rc;
}



RTDECL(int) RTEnvSetBad(const char *pszVar, const char *pszValue)
{
    AssertMsgReturn(strchr(pszVar, '=') == NULL, ("'%s'\n", pszVar), VERR_ENV_INVALID_VAR_NAME);

    /* make a local copy and feed it to putenv. */
    const size_t cchVar = strlen(pszVar);
    const size_t cchValue = strlen(pszValue);
    char *pszTmp = (char *)alloca(cchVar + cchValue + 2 + !*pszValue);
    memcpy(pszTmp, pszVar, cchVar);
    pszTmp[cchVar] = '=';
    if (*pszValue)
        memcpy(pszTmp + cchVar + 1, pszValue, cchValue + 1);
    else
    {
        pszTmp[cchVar + 1] = ' '; /* wrong, but putenv will remove it otherwise. */
        pszTmp[cchVar + 2] = '\0';
    }

    if (!putenv(pszTmp))
        return 0;
    return RTErrConvertFromErrno(errno);
}


RTDECL(int) RTEnvSet(const char *pszVar, const char *pszValue)
{
    return RTEnvSetBad(pszVar, pszValue);
}

RTDECL(int) RTEnvSetUtf8(const char *pszVar, const char *pszValue)
{
    AssertReturn(strchr(pszVar, '=') == NULL, VERR_ENV_INVALID_VAR_NAME);

    size_t cwcVar;
    int rc = RTStrCalcUtf16LenEx(pszVar, RTSTR_MAX, &cwcVar);
    if (RT_SUCCESS(rc))
    {
        size_t cwcValue;
        rc = RTStrCalcUtf16LenEx(pszVar, RTSTR_MAX, &cwcValue);
        if (RT_SUCCESS(rc))
        {
            PRTUTF16 pwszTmp = (PRTUTF16)RTMemTmpAlloc((cwcVar + 1 + cwcValue + 1) * sizeof(RTUTF16));
            if (pwszTmp)
            {
                rc = RTStrToUtf16Ex(pszVar, RTSTR_MAX, &pwszTmp, cwcVar + 1, NULL);
                if (RT_SUCCESS(rc))
                {
                    PRTUTF16 pwszTmpValue = &pwszTmp[cwcVar];
                    *pwszTmpValue++ = '=';
                    rc = RTStrToUtf16Ex(pszValue, RTSTR_MAX, &pwszTmpValue, cwcValue + 1, NULL);
                    if (RT_SUCCESS(rc))
                    {
                        if (!_wputenv(pwszTmp))
                            rc = VINF_SUCCESS;
                        else
                            rc = RTErrConvertFromErrno(errno);
                    }
                }
                RTMemTmpFree(pwszTmp);
            }
            else
                rc = VERR_NO_TMP_MEMORY;
        }
    }
    return rc;
}


RTDECL(int) RTEnvUnsetBad(const char *pszVar)
{
    AssertReturn(strchr(pszVar, '=') == NULL, VERR_ENV_INVALID_VAR_NAME);

    /*
     * Check that it exists first.
     */
    if (!RTEnvExist(pszVar))
        return VINF_ENV_VAR_NOT_FOUND;

    /*
     * Ok, try remove it.
     */
#ifdef RT_OS_WINDOWS
    /* Use putenv(var=) since Windows does not have unsetenv(). */
    size_t cchVar = strlen(pszVar);
    char *pszBuf = (char *)alloca(cchVar + 2);
    memcpy(pszBuf, pszVar, cchVar);
    pszBuf[cchVar]     = '=';
    pszBuf[cchVar + 1] = '\0';

    if (!putenv(pszBuf))
        return VINF_SUCCESS;

#else
    /* This is the preferred function as putenv() like used above does neither work on Solaris nor on Darwin. */
    if (!unsetenv((char*)pszVar))
        return VINF_SUCCESS;
#endif

    return RTErrConvertFromErrno(errno);
}


RTDECL(int) RTEnvUnset(const char *pszVar)
{
    return RTEnvUnsetBad(pszVar);
}


RTDECL(int) RTEnvUnsetUtf8(const char *pszVar)
{
    AssertReturn(strchr(pszVar, '=') == NULL, VERR_ENV_INVALID_VAR_NAME);

    size_t cwcVar;
    int rc = RTStrCalcUtf16LenEx(pszVar, RTSTR_MAX, &cwcVar);
    if (RT_SUCCESS(rc))
    {
        PRTUTF16 pwszTmp = (PRTUTF16)RTMemTmpAlloc((cwcVar + 1 + 1) * sizeof(RTUTF16));
        if (pwszTmp)
        {
            rc = RTStrToUtf16Ex(pszVar, RTSTR_MAX, &pwszTmp, cwcVar + 1, NULL);
            if (RT_SUCCESS(rc))
            {
                pwszTmp[cwcVar] = '=';
                pwszTmp[cwcVar + 1] = '\0';
                if (!_wputenv(pwszTmp))
                    rc = VINF_SUCCESS;
                else
                    rc = RTErrConvertFromErrno(errno);
            }
            RTMemTmpFree(pwszTmp);
        }
    }
    return rc;
}

