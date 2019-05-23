/* $Id: tstRTTemp.cpp $ */
/** @file
 * IPRT Testcase - Temporary files and directories.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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
#include <iprt/dir.h>
#include <iprt/file.h>
#include <iprt/path.h>

#include <iprt/err.h>
#include <iprt/initterm.h>
#include <iprt/mem.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/test.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static char g_szTempPath[RTPATH_MAX - 50];


static void tstObjectCreateTemp(const char *pszSubTest, const char *pszTemplate, bool fFile, RTFMODE fMode, unsigned cTimes, bool fSkipXCheck)
{
    RTTestISub(pszSubTest);
    const char *pcszAPI = fFile ? "RTFileCreateTemp" : "RTDirCreateTemp";

    /* Allocate the result array. */
    char **papszNames = (char **)RTMemTmpAllocZ(cTimes * sizeof(char *));
    RTTESTI_CHECK_RETV(papszNames != NULL);

    /* The test loop. */
    unsigned i;
    for (i = 0; i < cTimes; i++)
    {
        int rc;
        char szName[RTPATH_MAX];
        RTFMODE fModeFinal;

        RTTESTI_CHECK_RC(rc = RTPathAppend(strcpy(szName, g_szTempPath), sizeof(szName), pszTemplate), VINF_SUCCESS);
        if (RT_FAILURE(rc))
            break;

        RTTESTI_CHECK(papszNames[i] = RTStrDup(szName));
        if (!papszNames[i])
            break;

        rc =   fFile
             ? RTFileCreateTemp(papszNames[i], fMode)
             : RTDirCreateTemp(papszNames[i], fMode);
        if (rc != VINF_SUCCESS)
        {
            RTTestIFailed("%s(%s, %#o) call #%u -> %Rrc\n", pcszAPI, szName, (int)fMode, i, rc);
            RTStrFree(papszNames[i]);
            papszNames[i] = NULL;
            break;
        }
        /* Check that the final permissions are not more permissive than
         * the ones requested (less permissive is fine, c.f. umask etc.).
         * I mask out the group as I am not sure how we deal with that on
         * Windows. */
        RTTESTI_CHECK_RC_OK(rc = RTPathGetMode(papszNames[i], &fModeFinal));
        if (RT_SUCCESS(rc))
        {
            fModeFinal &= (RTFS_UNIX_IRWXU | RTFS_UNIX_IRWXO);
            RTTESTI_CHECK_MSG((fModeFinal & ~fMode) == 0,
                              ("%s: szName   %s\nfModeFinal ~= %#o, expected %#o\n",
                               pcszAPI, szName, fModeFinal, (int)fMode));
        }
        RTTestIPrintf(RTTESTLVL_DEBUG, "%s: %s\n", pcszAPI, papszNames[i]);
        RTTESTI_CHECK_MSG(strlen(szName) == strlen(papszNames[i]), ("%s: szName   %s\nReturned %s\n", pcszAPI, szName, papszNames[i]));
        if (!fSkipXCheck)
            RTTESTI_CHECK_MSG(strchr(RTPathFilename(papszNames[i]), 'X') == NULL, ("%s: szName   %s\nReturned %s\n", pcszAPI, szName, papszNames[i]));
    }

    /* cleanup */
    while (i-- > 0)
    {
        if (fFile)
            RTTESTI_CHECK_RC(RTFileDelete(papszNames[i]), VINF_SUCCESS);
        else
            RTTESTI_CHECK_RC(RTDirRemove(papszNames[i]), VINF_SUCCESS);
        RTStrFree(papszNames[i]);
    }
    RTMemTmpFree(papszNames);
}


static void tstFileCreateTemp(const char *pszSubTest, const char *pszTemplate, RTFMODE fMode, unsigned cTimes, bool fSkipXCheck)
{
    tstObjectCreateTemp(pszSubTest, pszTemplate, true /* fFile */, fMode,
                        cTimes, fSkipXCheck);
}


static void tstDirCreateTemp(const char *pszSubTest, const char *pszTemplate, RTFMODE fMode, unsigned cTimes, bool fSkipXCheck)
{
    tstObjectCreateTemp(pszSubTest, pszTemplate, false /* fFile */, fMode,
                        cTimes, fSkipXCheck);
}


static void tstBothCreateTemp(const char *pszSubTest, const char *pszTemplate, RTFMODE fMode, unsigned cTimes, bool fSkipXCheck)
{
    char pszSubTestLong[128];

    RTStrPrintf(pszSubTestLong, sizeof(pszSubTestLong), "RTFileCreateTemp %s",
                pszSubTest);
    tstFileCreateTemp(pszSubTestLong, pszTemplate, fMode, cTimes,
                      fSkipXCheck);
    RTStrPrintf(pszSubTestLong, sizeof(pszSubTestLong), "RTDirCreateTemp %s",
                pszSubTest);
    tstDirCreateTemp(pszSubTestLong, pszTemplate, fMode, cTimes, fSkipXCheck);
}


int main()
{
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstRTTemp", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);

    /*
     * Get the temp directory (this is essential to the testcase).
     */
    RTTESTI_CHECK_RC(rc = RTPathTemp(g_szTempPath, sizeof(g_szTempPath)), VINF_SUCCESS);
    if (RT_FAILURE(rc))
        return RTTestSummaryAndDestroy(hTest);

    /*
     * Create N temporary files and directories using RT(File|Dir)CreateTemp.
     */
    tstBothCreateTemp("#1 (standard)",   "rtRTTemp-XXXXXX",              0700, 128, false /*fSkipXCheck*/);
    tstBothCreateTemp("#2 (long)",       "rtRTTemp-XXXXXXXXXXXXXXXXX",   0700, 128, false /*fSkipXCheck*/);
    tstBothCreateTemp("#3 (short)",      "rtRTTemp-XX",                  0777, 128, false /*fSkipXCheck*/);
    tstBothCreateTemp("#4 (very short)", "rtRTTemp-X",                 0100, 26+10, false /*fSkipXCheck*/);
    tstBothCreateTemp("#5 (in-name)",    "rtRTTemp-XXXt",                  0301, 2, false /*fSkipXCheck*/);
    tstBothCreateTemp("#6 (in-name)",    "XXX-rtRTTemp",                   0355, 2, false /*fSkipXCheck*/);
    tstBothCreateTemp("#7 (in-name)",    "rtRTTemp-XXXXXXXXX.tmp",       0755, 128, false /*fSkipXCheck*/);
    tstBothCreateTemp("#8 (in-name)",    "rtRTTemp-XXXXXXX-X.tmp",       0700, 128, true /*fSkipXCheck*/);
    tstBothCreateTemp("#9 (in-name)",    "rtRTTemp-XXXXXX-XX.tmp",       0700, 128, true /*fSkipXCheck*/);

    /*
     * Summary.
     */
    return RTTestSummaryAndDestroy(hTest);
}

