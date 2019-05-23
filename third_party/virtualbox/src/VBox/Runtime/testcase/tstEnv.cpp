/* $Id: tstEnv.cpp $ */
/** @file
 * IPRT Testcase - Environment.
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
#include <iprt/env.h>
#include <iprt/initterm.h>
#include <iprt/err.h>
#include <iprt/string.h>
#include <iprt/stream.h>


int main()
{
    RTR3InitExeNoArguments(0);
    RTPrintf("tstEnv: TESTING...\n");

    int cErrors = 0;

#define CHECK(expr)  do { if (!(expr)) { RTPrintf("tstEnv: error line %d: %s\n", __LINE__, #expr); cErrors++; } } while (0)
#define CHECK_RC(expr, rc)  do { int rc2 = expr; if (rc2 != (rc)) { RTPrintf("tstEnv: error line %d: %s -> %Rrc expected %Rrc\n", __LINE__, #expr, rc2, rc); cErrors++; } } while (0)
#define CHECK_STR(str1, str2)  do { if (strcmp(str1, str2)) { RTPrintf("tstEnv: error line %d: '%s' != '%s' (*)\n", __LINE__, str1, str2); cErrors++; } } while (0)

    /*
     * Try mess around with the path a bit.
     */
#ifdef RT_OS_WINDOWS
    static const char * const k_pszPathVar = "Path";
#else
    static const char * const k_pszPathVar = "PATH";
#endif
    static const char * const k_pszNonExistantVar = "IPRT_I_DON_T_EXIST";

    CHECK(RTEnvExist(k_pszPathVar));
    CHECK(RTEnvExistEx(RTENV_DEFAULT, k_pszPathVar));
    CHECK(!RTEnvExist(k_pszNonExistantVar));
    CHECK(!RTEnvExistEx(RTENV_DEFAULT, k_pszNonExistantVar));

    CHECK(RTEnvGet(k_pszPathVar) != NULL);
    char szBuf[8192];
    size_t cch;
    CHECK_RC(RTEnvGetEx(RTENV_DEFAULT, k_pszPathVar, NULL, 0, &cch), VINF_SUCCESS);
    CHECK(cch < sizeof(szBuf));
    CHECK_RC(RTEnvGetEx(RTENV_DEFAULT, k_pszPathVar, szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_RC(RTEnvGetEx(RTENV_DEFAULT, k_pszPathVar, szBuf, sizeof(szBuf), NULL), VINF_SUCCESS);
    CHECK_RC(RTEnvGetEx(RTENV_DEFAULT, k_pszPathVar, szBuf, 1, &cch), VERR_BUFFER_OVERFLOW);
    CHECK_RC(RTEnvGetEx(RTENV_DEFAULT, k_pszPathVar, szBuf, 1, NULL), VERR_BUFFER_OVERFLOW);

    /* ditto for a clone. */
    RTENV Env;
    CHECK_RC(RTEnvClone(&Env, RTENV_DEFAULT), VINF_SUCCESS);

    CHECK(RTEnvExistEx(Env, k_pszPathVar));
    CHECK(!RTEnvExistEx(Env, k_pszNonExistantVar));

    CHECK_RC(RTEnvGetEx(Env, k_pszPathVar, NULL, 0, &cch), VINF_SUCCESS);
    CHECK(cch < sizeof(szBuf));
    CHECK_RC(RTEnvGetEx(Env, k_pszPathVar, szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_RC(RTEnvGetEx(Env, k_pszPathVar, szBuf, sizeof(szBuf), NULL), VINF_SUCCESS);
    CHECK_RC(RTEnvGetEx(Env, k_pszPathVar, szBuf, 1, &cch), VERR_BUFFER_OVERFLOW);
    CHECK_RC(RTEnvGetEx(Env, k_pszPathVar, szBuf, 1, NULL), VERR_BUFFER_OVERFLOW);

    /*
     * Set and Unset
     */
    CHECK_RC(RTEnvSetEx(RTENV_DEFAULT, "IPRTMyNewVar", "MyValue1"), VINF_SUCCESS);
    CHECK_RC(RTEnvGetEx(RTENV_DEFAULT, "IPRTMyNewVar", szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_STR(szBuf, "MyValue1");
    CHECK_RC(RTEnvSetEx(RTENV_DEFAULT, "IPRTMyNewVar", "MyValue2"), VINF_SUCCESS);
    CHECK_RC(RTEnvGetEx(RTENV_DEFAULT, "IPRTMyNewVar", szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_STR(szBuf, "MyValue2");

    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar", "MyValue1"), VINF_SUCCESS);
    CHECK_RC(RTEnvGetEx(Env, "IPRTMyNewVar", szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_STR(szBuf, "MyValue1");
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar", "MyValue2"), VINF_SUCCESS);
    CHECK_RC(RTEnvGetEx(Env, "IPRTMyNewVar", szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_STR(szBuf, "MyValue2");

    CHECK_RC(RTEnvUnsetEx(Env, "IPRTMyNewVar"), VINF_SUCCESS);
    CHECK_RC(RTEnvGetEx(Env, "IPRTMyNewVar", szBuf, sizeof(szBuf), &cch), VERR_ENV_VAR_NOT_FOUND);
    CHECK_RC(RTEnvUnsetEx(Env, "IPRTMyNewVar"), VINF_ENV_VAR_NOT_FOUND);

    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar0", "MyValue0"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar1", "MyValue1"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar2", "MyValue2"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar3", "MyValue3"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar4", "MyValue4"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar5", "MyValue5"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar6", "MyValue6"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar7", "MyValue7"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar8", "MyValue8"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar9", "MyValue9"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar10", "MyValue10"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar11", "MyValue11"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar12", "MyValue12"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar13", "MyValue13"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar14", "MyValue14"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar15", "MyValue15"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar16", "MyValue16"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar17", "MyValue17"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar18", "MyValue18"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar19", "MyValue19"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar20", "MyValue20"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar21", "MyValue21"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar22", "MyValue22"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar23", "MyValue23"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar24", "MyValue24"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar25", "MyValue25"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar26", "MyValue26"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar27", "MyValue27"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar28", "MyValue28"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar29", "MyValue29"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar30", "MyValue30"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar31", "MyValue31"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar32", "MyValue32"), VINF_SUCCESS);
    CHECK_RC(RTEnvSetEx(Env, "IPRTMyNewVar33", "MyValue33"), VINF_SUCCESS);
    CHECK_RC(RTEnvGetEx(Env, "IPRTMyNewVar30", szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_STR(szBuf, "MyValue30");
    CHECK_RC(RTEnvGetEx(Env, "IPRTMyNewVar31", szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_STR(szBuf, "MyValue31");
    CHECK_RC(RTEnvGetEx(Env, "IPRTMyNewVar32", szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_STR(szBuf, "MyValue32");
    CHECK_RC(RTEnvGetEx(Env, "IPRTMyNewVar33", szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_STR(szBuf, "MyValue33");

    CHECK_RC(RTEnvUnsetEx(Env, "IPRTMyNewVar33"), VINF_SUCCESS);
    CHECK(!RTEnvExistEx(Env, "IPRTMyNewVar33"));
    CHECK_RC(RTEnvGetEx(Env, "IPRTMyNewVar33", szBuf, sizeof(szBuf), &cch), VERR_ENV_VAR_NOT_FOUND);
    CHECK_RC(RTEnvGetEx(Env, "IPRTMyNewVar32", szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_STR(szBuf, "MyValue32");
    CHECK_RC(RTEnvGetEx(Env, "IPRTMyNewVar15", szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_STR(szBuf, "MyValue15");

    CHECK_RC(RTEnvUnsetEx(Env, "IPRTMyNewVar3"), VINF_SUCCESS);
    CHECK(!RTEnvExistEx(Env, "IPRTMyNewVar3"));
    CHECK_RC(RTEnvGetEx(Env, "IPRTMyNewVar3", szBuf, sizeof(szBuf), &cch), VERR_ENV_VAR_NOT_FOUND);
    CHECK_RC(RTEnvGetEx(Env, "IPRTMyNewVar32", szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_STR(szBuf, "MyValue32");
    CHECK_RC(RTEnvGetEx(Env, "IPRTMyNewVar15", szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_STR(szBuf, "MyValue15");

    CHECK_RC(RTEnvUnsetEx(Env, k_pszPathVar), VINF_SUCCESS);
    CHECK(!RTEnvExistEx(Env, k_pszPathVar));
    CHECK_RC(RTEnvGetEx(Env, "IPRTMyNewVar32", szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_STR(szBuf, "MyValue32");
    CHECK_RC(RTEnvGetEx(Env, "IPRTMyNewVar15", szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_STR(szBuf, "MyValue15");

    /*
     * Put.
     */
    CHECK_RC(RTEnvPutEx(Env, "IPRTMyNewVar28"), VINF_SUCCESS);
    CHECK(!RTEnvExistEx(Env, "IPRTMyNewVar28"));
    CHECK_RC(RTEnvGetEx(Env, "IPRTMyNewVar32", szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_STR(szBuf, "MyValue32");
    CHECK_RC(RTEnvGetEx(Env, "IPRTMyNewVar15", szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_STR(szBuf, "MyValue15");

    CHECK_RC(RTEnvPutEx(Env, "IPRTMyNewVar28=MyValue28"), VINF_SUCCESS);
    CHECK_RC(RTEnvGetEx(Env, "IPRTMyNewVar28", szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_STR(szBuf, "MyValue28");
    CHECK_RC(RTEnvGetEx(Env, "IPRTMyNewVar32", szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_STR(szBuf, "MyValue32");
    CHECK_RC(RTEnvGetEx(Env, "IPRTMyNewVar15", szBuf, sizeof(szBuf), &cch), VINF_SUCCESS);
    CHECK_STR(szBuf, "MyValue15");

    /*
     * Dup.
     */
    char *psz1;
    CHECK(RTEnvDupEx(Env, "NonExistantVariable") == NULL);
    psz1 = RTEnvDupEx(Env, "IPRTMyNewVar15");
    CHECK(psz1);
    if (psz1)
        CHECK_STR(psz1, "MyValue15");
    RTStrFree(psz1);

    static char s_szBigValue[10999];
    memset(s_szBigValue, 'a', sizeof(s_szBigValue));
    s_szBigValue[sizeof(s_szBigValue) - 1] = '\0';
    CHECK_RC(RTEnvSetEx(Env, "IPRTBigValue", s_szBigValue), VINF_SUCCESS);
    psz1 = RTEnvDupEx(Env, "IPRTBigValue");
    CHECK(psz1);
    if (psz1)
        CHECK_STR(psz1, s_szBigValue);
    RTStrFree(psz1);

    /*
     * Another cloning.
     */
    RTENV Env2;
    CHECK_RC(RTEnvClone(&Env2, Env), VINF_SUCCESS);
    CHECK_RC(RTEnvDestroy(Env2), VINF_SUCCESS);

    /*
     * execve envp and we're done.
     */
    const char * const *papsz = RTEnvGetExecEnvP(RTENV_DEFAULT);
    CHECK(papsz != NULL);
    papsz = RTEnvGetExecEnvP(RTENV_DEFAULT);
    CHECK(papsz != NULL);

    papsz = RTEnvGetExecEnvP(Env);
    CHECK(papsz != NULL);
    papsz = RTEnvGetExecEnvP(Env);
    CHECK(papsz != NULL);

    CHECK_RC(RTEnvDestroy(Env), VINF_SUCCESS);

    /*
     * Summary
     */
    if (!cErrors)
        RTPrintf("tstEnv: SUCCESS\n");
    else
        RTPrintf("tstEnv: FAILURE - %d errors\n", cErrors);
    return !!cErrors;
}

