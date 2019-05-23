/* $Id: tstRTPath.cpp $ */
/** @file
 * IPRT Testcase - Test various path functions.
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
#include <iprt/path.h>

#include <iprt/err.h>
#include <iprt/initterm.h>
#include <iprt/param.h>
#include <iprt/process.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/test.h>


static void testParserAndSplitter(RTTEST hTest)
{
    static struct
    {
        uint16_t    cComps;
        uint16_t    cchPath;
        uint16_t    offSuffix;
        const char *pszPath;
        uint16_t    fProps;
        uint32_t    fFlags;
    } const s_aTests[] =
    {
        { 2,  5,  5,  "/bin/",            RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_DIR_SLASH,                                                RTPATH_STR_F_STYLE_UNIX },
        { 2, 13,  9,  "C:/Config.sys",    RTPATH_PROP_VOLUME | RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_FILENAME | RTPATH_PROP_SUFFIX,       RTPATH_STR_F_STYLE_DOS },
        { 2, 13, 10,  "C://Config.sys",   RTPATH_PROP_VOLUME | RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_FILENAME | RTPATH_PROP_SUFFIX | RTPATH_PROP_EXTRA_SLASHES, RTPATH_STR_F_STYLE_DOS },
        { 2, 12,  8,  "C:Config.sys",     RTPATH_PROP_VOLUME | RTPATH_PROP_RELATIVE | RTPATH_PROP_FILENAME | RTPATH_PROP_SUFFIX,                                RTPATH_STR_F_STYLE_DOS },
        { 1, 10,  6,  "Config.sys",       RTPATH_PROP_RELATIVE | RTPATH_PROP_FILENAME | RTPATH_PROP_SUFFIX,                                                     RTPATH_STR_F_STYLE_DOS },
        { 1,  4,  4,  "//./",             RTPATH_PROP_UNC | RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE,                                                      RTPATH_STR_F_STYLE_DOS },
        { 2,  5,  5,  "//./f",            RTPATH_PROP_UNC | RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_FILENAME,                               RTPATH_STR_F_STYLE_DOS },
        { 2,  5,  6,  "//.//f",           RTPATH_PROP_UNC | RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_FILENAME | RTPATH_PROP_EXTRA_SLASHES,   RTPATH_STR_F_STYLE_DOS },
        { 3,  7,  7,  "//././f",          RTPATH_PROP_UNC | RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_FILENAME | RTPATH_PROP_DOT_REFS,        RTPATH_STR_F_STYLE_DOS },
        { 3,  8,  8,  "//.././f",         RTPATH_PROP_UNC | RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_FILENAME | RTPATH_PROP_DOT_REFS,        RTPATH_STR_F_STYLE_DOS },
        { 3,  9,  9,  "//../../f",        RTPATH_PROP_UNC | RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_RELATIVE | RTPATH_PROP_FILENAME | RTPATH_PROP_DOTDOT_REFS,     RTPATH_STR_F_STYLE_DOS },
        { 1,  1,  1,  "/",                RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE,                                                                        RTPATH_STR_F_STYLE_UNIX },
        { 2,  4,  4,  "/bin",             RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_FILENAME,                                                 RTPATH_STR_F_STYLE_UNIX },
        { 2,  5,  5,  "/bin/",            RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_DIR_SLASH,                                                RTPATH_STR_F_STYLE_UNIX },
        { 3,  7,  7,  "/bin/ls",          RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_FILENAME,                                                 RTPATH_STR_F_STYLE_UNIX },
        { 3,  12, 7,  "/etc/rc.conf",     RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_FILENAME | RTPATH_PROP_SUFFIX,                            RTPATH_STR_F_STYLE_UNIX },
        { 1,  1,  2,  "//",               RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_EXTRA_SLASHES,                                            RTPATH_STR_F_STYLE_UNIX },
        { 1,  1,  3,  "///",              RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_EXTRA_SLASHES,                                            RTPATH_STR_F_STYLE_UNIX },
        { 3,  6,  7,  "/.//bin",          RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_EXTRA_SLASHES | RTPATH_PROP_DOT_REFS | RTPATH_PROP_FILENAME, RTPATH_STR_F_STYLE_UNIX },
        { 1,  3,  3,  "bin",              RTPATH_PROP_RELATIVE | RTPATH_PROP_FILENAME,                                                                          RTPATH_STR_F_STYLE_UNIX },
        { 1,  4,  4,  "bin/",             RTPATH_PROP_RELATIVE | RTPATH_PROP_DIR_SLASH,                                                                         RTPATH_STR_F_STYLE_UNIX },
        { 1,  4,  7,  "bin////",          RTPATH_PROP_RELATIVE | RTPATH_PROP_DIR_SLASH | RTPATH_PROP_EXTRA_SLASHES,                                             RTPATH_STR_F_STYLE_UNIX },
        { 3, 10, 10,  "bin/../usr",       RTPATH_PROP_RELATIVE | RTPATH_PROP_DOTDOT_REFS | RTPATH_PROP_FILENAME,                                                RTPATH_STR_F_STYLE_UNIX },
        { 4, 11, 11,  "/bin/../usr",      RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_RELATIVE | RTPATH_PROP_DOTDOT_REFS | RTPATH_PROP_FILENAME,                       RTPATH_STR_F_STYLE_UNIX },
        { 4,  8,  8,  "/a/.../u",         RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_FILENAME,                                                 RTPATH_STR_F_STYLE_UNIX },
        { 4,  8,  8,  "/a/.b./u",         RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_FILENAME,                                                 RTPATH_STR_F_STYLE_UNIX },
        { 4,  8,  8,  "/a/..c/u",         RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_FILENAME,                                                 RTPATH_STR_F_STYLE_UNIX },
        { 4,  8,  8,  "/a/d../u",         RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_FILENAME,                                                 RTPATH_STR_F_STYLE_UNIX },
        { 4,  8,  8,  "/a/.e/.u",         RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_FILENAME,                                                 RTPATH_STR_F_STYLE_UNIX },
        { 4,  8,  8,  "/a/.f/.u",         RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_FILENAME,                                                 RTPATH_STR_F_STYLE_UNIX },
        { 4,  8,  8,  "/a/.g/u.",         RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE | RTPATH_PROP_FILENAME,                                                 RTPATH_STR_F_STYLE_UNIX },
        { 3,  9, 10,  "/a/h/u.ext",       RTPATH_PROP_EXTRA_SLASHES | RTPATH_PROP_RELATIVE,                                                                     RTPATH_STR_F_STYLE_UNIX | RTPATH_STR_F_MIDDLE },
        { 3,  9,  9,  "a/h/u.ext",        RTPATH_PROP_RELATIVE,                                                                                                 RTPATH_STR_F_STYLE_UNIX | RTPATH_STR_F_MIDDLE },
        { 3,  9, 10,  "a/h/u.ext/",       RTPATH_PROP_EXTRA_SLASHES | RTPATH_PROP_RELATIVE,                                                                     RTPATH_STR_F_STYLE_UNIX | RTPATH_STR_F_MIDDLE },
    };

    char szPath1[RTPATH_MAX];
    union
    {
        RTPATHPARSED    Parsed;
        RTPATHSPLIT     Split;
        uint8_t         ab[4096];
    } u;

    RTTestSub(hTest, "RTPathParse");
    for (uint32_t i = 0; i < RT_ELEMENTS(s_aTests); i++)
    {
        memset(&u, i & 1 ? 0xff : 0, sizeof(u));
        int rc = RTPathParse(s_aTests[i].pszPath, &u.Parsed, sizeof(u), s_aTests[i].fFlags);
        if (   rc != VINF_SUCCESS
            || s_aTests[i].cComps    != u.Parsed.cComps
            || s_aTests[i].fProps    != u.Parsed.fProps
            || s_aTests[i].offSuffix != u.Parsed.offSuffix
            || s_aTests[i].cchPath   != u.Parsed.cchPath)
        {
            RTTestFailed(hTest, "i=%d rc=%Rrc %s", i, rc, s_aTests[i].pszPath);
            RTTestFailureDetails(hTest,
                                 "  cComps    %u, got %u\n"
                                 "  fProps    %#x, got %#x, xor=>%#x\n"
                                 "  offSuffix %u, got %u\n"
                                 "  cchPath   %u, got %u\n"
                                 ,
                                 s_aTests[i].cComps,    u.Parsed.cComps,
                                 s_aTests[i].fProps,    u.Parsed.fProps, s_aTests[i].fProps ^ u.Parsed.fProps,
                                 s_aTests[i].offSuffix, u.Parsed.offSuffix,
                                 s_aTests[i].cchPath,   u.Parsed.cchPath);
        }
        else
        {
            rc = RTPathParsedReassemble(s_aTests[i].pszPath, &u.Parsed, s_aTests[i].fFlags & ~RTPATH_STR_F_MIDDLE,
                                        szPath1, sizeof(szPath1));
            if (rc == VINF_SUCCESS)
            {
                RTTESTI_CHECK_MSG(strlen(szPath1) == s_aTests[i].cchPath, ("%s\n", szPath1));
                if (   !(u.Parsed.fProps & RTPATH_PROP_EXTRA_SLASHES)
                    && (s_aTests[i].fFlags & RTPATH_STR_F_STYLE_MASK) != RTPATH_STR_F_STYLE_DOS)
                    RTTESTI_CHECK_MSG(strcmp(szPath1, s_aTests[i].pszPath) == 0, ("%s\n", szPath1));
            }
            else
                RTTestIFailed("RTPathParsedReassemble -> %Rrc", rc);
        }
    }

    RTTestSub(hTest, "RTPathSplit");
    for (uint32_t i = 0; i < RT_ELEMENTS(s_aTests); i++)
    {
        memset(&u, i & 1 ? 0xff : 0, sizeof(u));
        int rc = RTPathSplit(s_aTests[i].pszPath, &u.Split, sizeof(u), s_aTests[i].fFlags);
        if (   rc != VINF_SUCCESS
            || s_aTests[i].cComps    != u.Split.cComps
            || s_aTests[i].fProps    != u.Split.fProps
            || s_aTests[i].cchPath   != u.Split.cchPath)
        {
            RTTestFailed(hTest, "i=%d rc=%Rrc %s", i, rc, s_aTests[i].pszPath);
            RTTestFailureDetails(hTest,
                                 "  cComps    %u, got %u\n"
                                 "  fProps    %#x, got %#x, xor=>%#x\n"
                                 "  cchPath   %u, got %u\n"
                                 ,
                                 s_aTests[i].cComps,    u.Split.cComps,
                                 s_aTests[i].fProps,    u.Split.fProps, s_aTests[i].fProps ^ u.Split.fProps,
                                 s_aTests[i].cchPath,   u.Split.cchPath);
        }
        else
        {
            RTTESTI_CHECK_MSG(*u.Split.pszSuffix == '\0' || *u.Split.pszSuffix == '.', ("%s", u.Split.pszSuffix));
            for (uint32_t idxComp = RTPATH_PROP_HAS_ROOT_SPEC(u.Split.fProps); idxComp < u.Split.cComps; idxComp++)
                if ( (s_aTests[i].fFlags & RTPATH_STR_F_STYLE_MASK) == RTPATH_STR_F_STYLE_DOS
                    ? strpbrk(u.Split.apszComps[idxComp], "/\\")
                    : strchr(u.Split.apszComps[idxComp], RTPATH_SLASH) )
                    RTTestFailed(hTest, "i=%d idxComp=%d '%s'", i, idxComp, u.Split.apszComps[idxComp]);

            PRTPATHSPLIT pSplit = NULL;
            RTTESTI_CHECK_RC(rc = RTPathSplitA(s_aTests[i].pszPath, &pSplit, s_aTests[i].fFlags), VINF_SUCCESS);
            if (RT_SUCCESS(rc))
            {
                RTTESTI_CHECK(pSplit);
                RTTESTI_CHECK(pSplit->cComps   == u.Split.cComps);
                RTTESTI_CHECK(pSplit->fProps   == u.Split.fProps);
                RTTESTI_CHECK(pSplit->cchPath  == u.Split.cchPath);
                RTTESTI_CHECK(pSplit->cbNeeded == u.Split.cbNeeded);
                RTTESTI_CHECK(!strcmp(pSplit->pszSuffix, u.Split.pszSuffix));
                for (uint32_t idxComp = 0; idxComp < u.Split.cComps; idxComp++)
                    RTTESTI_CHECK(!strcmp(pSplit->apszComps[idxComp], u.Split.apszComps[idxComp]));
                RTPathSplitFree(pSplit);
            }

            rc = RTPathSplitReassemble(&u.Split, s_aTests[i].fFlags & ~RTPATH_STR_F_MIDDLE, szPath1, sizeof(szPath1));
            if (rc == VINF_SUCCESS)
            {
                RTTESTI_CHECK_MSG(strlen(szPath1) == s_aTests[i].cchPath, ("%s\n", szPath1));
                if (   !(u.Parsed.fProps & RTPATH_PROP_EXTRA_SLASHES)
                    && (s_aTests[i].fFlags & RTPATH_STR_F_STYLE_MASK) != RTPATH_STR_F_STYLE_DOS)
                    RTTESTI_CHECK_MSG(strcmp(szPath1, s_aTests[i].pszPath) == 0, ("%s\n", szPath1));
            }
            else
                RTTestIFailed("RTPathSplitReassemble -> %Rrc", rc);
        }
    }
}


int main()
{
    char szPath[RTPATH_MAX];

    /*
     * Init RT+Test.
     */
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstRTPath", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);

    RTTestSub(hTest, "Environment");
#if defined(RT_OS_OS2) || defined(RT_OS_WINDOWS)
    RTTESTI_CHECK(RTPATH_STYLE == RTPATH_STR_F_STYLE_DOS);
# if RTPATH_STYLE == RTPATH_STR_F_STYLE_DOS
# else
    RTTestIFailed("#if RTPATH_STYLE == RTPATH_STR_F_STYLE_DOS");
# endif
    RTTESTI_CHECK(strcmp(RTPATH_SLASH_STR, "\\") == 0);
    RTTESTI_CHECK(RTPATH_SLASH == '\\');
    RTTESTI_CHECK(RTPATH_IS_SEP('/'));
    RTTESTI_CHECK(RTPATH_IS_SEP('\\'));
    RTTESTI_CHECK(RTPATH_IS_SEP(':'));

#else
    RTTESTI_CHECK(RTPATH_STYLE == RTPATH_STR_F_STYLE_UNIX);
# if RTPATH_STYLE == RTPATH_STR_F_STYLE_UNIX
# else
    RTTestIFailed("#if RTPATH_STYLE == RTPATH_STR_F_STYLE_UNIX");
# endif
    RTTESTI_CHECK(strcmp(RTPATH_SLASH_STR, "/") == 0);
    RTTESTI_CHECK(RTPATH_SLASH == '/');
    RTTESTI_CHECK(RTPATH_IS_SEP('/'));
    RTTESTI_CHECK(!RTPATH_IS_SEP('\\'));
    RTTESTI_CHECK(!RTPATH_IS_SEP(':'));
#endif

    /*
     * RTPathExecDir, RTPathUserHome and RTProcGetExecutablePath.
     */
    RTTestSub(hTest, "RTPathExecDir");
    RTTESTI_CHECK_RC(rc = RTPathExecDir(szPath, sizeof(szPath)), VINF_SUCCESS);
    if (RT_SUCCESS(rc))
        RTTestIPrintf(RTTESTLVL_INFO, "ExecDir={%s}\n", szPath);

    RTTestSub(hTest, "RTProcGetExecutablePath");
    if (RTProcGetExecutablePath(szPath, sizeof(szPath)) == szPath)
        RTTestIPrintf(RTTESTLVL_INFO, "ExecutableName={%s}\n", szPath);
    else
        RTTestIFailed("RTProcGetExecutablePath -> NULL");

    RTTestSub(hTest, "RTPathUserHome");
    RTTESTI_CHECK_RC(rc = RTPathUserHome(szPath, sizeof(szPath)), VINF_SUCCESS);
    if (RT_SUCCESS(rc))
        RTTestIPrintf(RTTESTLVL_INFO, "UserHome={%s}\n", szPath);

    RTTestSub(hTest, "RTPathUserDocuments");
    RTTESTI_CHECK_RC(rc = RTPathUserDocuments(szPath, sizeof(szPath)), VINF_SUCCESS);
    if (RT_SUCCESS(rc))
        RTTestIPrintf(RTTESTLVL_INFO, "UserDocuments={%s}\n", szPath);

    RTTestSub(hTest, "RTPathTemp");
    RTTESTI_CHECK_RC(rc = RTPathTemp(szPath, sizeof(szPath)), VINF_SUCCESS);
    if (RT_SUCCESS(rc))
        RTTestIPrintf(RTTESTLVL_INFO, "PathTemp={%s}\n", szPath);
    size_t cch = strlen(szPath);
    RTTESTI_CHECK_RC(RTPathTemp(szPath, cch), VERR_BUFFER_OVERFLOW);
    RTTESTI_CHECK_RC(RTPathTemp(szPath, cch+1), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTPathTemp(szPath, cch+2), VINF_SUCCESS);


    /*
     * RTPathAbsEx
     */
    RTTestSub(hTest, "RTPathAbsEx");
    static const struct
    {
        const char *pcszInputBase;
        const char *pcszInputPath;
        int rc;
        const char *pcszOutput;
    }
    s_aRTPathAbsExTests[] =
    {
#if defined (RT_OS_OS2) || defined (RT_OS_WINDOWS)
    { NULL, "", VERR_INVALID_PARAMETER, NULL },
    { NULL, ".", VINF_SUCCESS, "%p" },
    { NULL, "\\", VINF_SUCCESS, "%d\\" },
    { NULL, "\\..", VINF_SUCCESS, "%d\\" },
    { NULL, "/absolute/..", VINF_SUCCESS, "%d\\" },
    { NULL, "/absolute\\\\../..", VINF_SUCCESS, "%d\\" },
    { NULL, "/absolute//../path\\", VINF_SUCCESS, "%d\\path" },
    { NULL, "/absolute/../../path", VINF_SUCCESS, "%d\\path" },
    { NULL, "relative/../dir\\.\\.\\.\\file.txt", VINF_SUCCESS, "%p\\dir\\file.txt" },
    { NULL, "\\data\\", VINF_SUCCESS, "%d\\data" },
    { "relative_base/dir\\", "\\from_root", VINF_SUCCESS, "%d\\from_root" },
    { "relative_base/dir/", "relative_also", VINF_SUCCESS, "%p\\relative_base\\dir\\relative_also" },
#else
    { NULL, "", VERR_INVALID_PARAMETER, NULL },
    { NULL, ".", VINF_SUCCESS, "%p" },
    { NULL, "/", VINF_SUCCESS, "/" },
    { NULL, "/..", VINF_SUCCESS, "/" },
    { NULL, "/absolute/..", VINF_SUCCESS, "/" },
    { NULL, "/absolute\\\\../..", VINF_SUCCESS, "/" },
    { NULL, "/absolute//../path/", VINF_SUCCESS, "/path" },
    { NULL, "/absolute/../../path", VINF_SUCCESS, "/path" },
    { NULL, "relative/../dir/./././file.txt", VINF_SUCCESS, "%p/dir/file.txt" },
    { NULL, "relative/../dir\\.\\.\\.\\file.txt", VINF_SUCCESS, "%p/dir\\.\\.\\.\\file.txt" },  /* linux-specific */
    { NULL, "/data/", VINF_SUCCESS, "/data" },
    { "relative_base/dir/", "/from_root", VINF_SUCCESS, "/from_root" },
    { "relative_base/dir/", "relative_also", VINF_SUCCESS, "%p/relative_base/dir/relative_also" },
#endif
#if defined (RT_OS_OS2) || defined (RT_OS_WINDOWS)
    { NULL, "C:\\", VINF_SUCCESS, "C:\\" },
    { "C:\\", "..", VINF_SUCCESS, "C:\\" },
    { "C:\\temp", "..", VINF_SUCCESS, "C:\\" },
    { "C:\\VirtualBox/Machines", "..\\VirtualBox.xml", VINF_SUCCESS, "C:\\VirtualBox\\VirtualBox.xml" },
    { "C:\\MustDie", "\\from_root/dir/..", VINF_SUCCESS, "C:\\from_root" },
    { "C:\\temp", "D:\\data", VINF_SUCCESS, "D:\\data" },
    { NULL, "\\\\server\\..\\share", VINF_SUCCESS, "\\\\server\\..\\share" /* kind of strange */ },
    { NULL, "\\\\server/", VINF_SUCCESS, "\\\\server\\" },
    { NULL, "\\\\", VINF_SUCCESS, "\\\\" },
    { NULL, "\\\\\\something", VINF_SUCCESS, "\\\\\\something" /* kind of strange */ },
    { "\\\\server\\share_as_base", "/from_root", VINF_SUCCESS, "\\\\server\\from_root" },
    { "\\\\just_server", "/from_root", VINF_SUCCESS, "\\\\just_server\\from_root" },
    { "\\\\server\\share_as_base", "relative\\data", VINF_SUCCESS, "\\\\server\\share_as_base\\relative\\data" },
    { "base", "\\\\?\\UNC\\relative/edwef/..", VINF_SUCCESS, "\\\\?\\UNC\\relative" },
    { "\\\\?\\UNC\\base", "/from_root", VERR_INVALID_NAME, NULL },
#else
    { "/temp", "..", VINF_SUCCESS, "/" },
    { "/VirtualBox/Machines", "../VirtualBox.xml", VINF_SUCCESS, "/VirtualBox/VirtualBox.xml" },
    { "/MustDie", "/from_root/dir/..", VINF_SUCCESS, "/from_root" },
    { "\\temp", "\\data", VINF_SUCCESS, "%p/\\temp/\\data" },
#endif
    };

    for (unsigned i = 0; i < RT_ELEMENTS(s_aRTPathAbsExTests); ++ i)
    {
        rc = RTPathAbsEx(s_aRTPathAbsExTests[i].pcszInputBase,
                         s_aRTPathAbsExTests[i].pcszInputPath,
                         szPath, sizeof(szPath));
        if (rc != s_aRTPathAbsExTests[i].rc)
        {
            RTTestIFailed("unexpected result code!\n"
                          "   input base: '%s'\n"
                          "   input path: '%s'\n"
                          "       output: '%s'\n"
                          "           rc: %Rrc\n"
                          "  expected rc: %Rrc",
                          s_aRTPathAbsExTests[i].pcszInputBase,
                          s_aRTPathAbsExTests[i].pcszInputPath,
                          szPath, rc,
                          s_aRTPathAbsExTests[i].rc);
            continue;
        }

        char szTmp[RTPATH_MAX];
        char *pszExpected = NULL;
        if (s_aRTPathAbsExTests[i].pcszOutput != NULL)
        {
            if (s_aRTPathAbsExTests[i].pcszOutput[0] == '%')
            {
                RTTESTI_CHECK_RC(rc = RTPathGetCurrent(szTmp, sizeof(szTmp)), VINF_SUCCESS);
                if (RT_FAILURE(rc))
                    break;

                pszExpected = szTmp;

                if (s_aRTPathAbsExTests[i].pcszOutput[1] == 'p')
                {
                    cch = strlen(szTmp);
                    if (cch + strlen(s_aRTPathAbsExTests[i].pcszOutput) - 2 <= sizeof(szTmp))
                        strcpy(szTmp + cch, s_aRTPathAbsExTests[i].pcszOutput + 2);
                }
#if defined(RT_OS_OS2) || defined(RT_OS_WINDOWS)
                else if (s_aRTPathAbsExTests[i].pcszOutput[1] == 'd')
                {
                    if (2 + strlen(s_aRTPathAbsExTests[i].pcszOutput) - 2 <= sizeof(szTmp))
                        strcpy(szTmp + 2, s_aRTPathAbsExTests[i].pcszOutput + 2);
                }
#endif
            }
            else
            {
                strcpy(szTmp, s_aRTPathAbsExTests[i].pcszOutput);
                pszExpected = szTmp;
            }

            if (strcmp(szPath, pszExpected))
            {
                RTTestIFailed("Unexpected result\n"
                              "   input base: '%s'\n"
                              "   input path: '%s'\n"
                              "       output: '%s'\n"
                              "     expected: '%s'",
                              s_aRTPathAbsExTests[i].pcszInputBase,
                              s_aRTPathAbsExTests[i].pcszInputPath,
                              szPath,
                              s_aRTPathAbsExTests[i].pcszOutput);
            }
        }
    }

    /*
     * RTPathStripFilename
     */
    RTTestSub(hTest, "RTPathStripFilename");
    static const char *s_apszStripFilenameTests[] =
    {
        "/usr/include///",              "/usr/include//",
        "/usr/include/",                "/usr/include",
        "/usr/include",                 "/usr",
        "/usr",                         "/",
        "usr",                          ".",
#if defined (RT_OS_OS2) || defined (RT_OS_WINDOWS)
        "c:/windows",                   "c:/",
        "c:/",                          "c:/",
        "D:",                           "D:",
        "C:\\OS2\\DLLS",                "C:\\OS2",
#endif
    };
    for (unsigned i = 0; i < RT_ELEMENTS(s_apszStripFilenameTests); i += 2)
    {
        const char *pszInput  = s_apszStripFilenameTests[i];
        const char *pszExpect = s_apszStripFilenameTests[i + 1];
        strcpy(szPath, pszInput);
        RTPathStripFilename(szPath);
        if (strcmp(szPath, pszExpect))
        {
            RTTestIFailed("Unexpected result\n"
                          "   input: '%s'\n"
                          "  output: '%s'\n"
                          "expected: '%s'",
                          pszInput, szPath, pszExpect);
        }
    }

    /*
     * RTPathAppend.
     */
    RTTestSub(hTest, "RTPathAppend");
    static const char *s_apszAppendTests[] =
    {
        /* base                 append                  result */
        "/",                    "",                     "/",
        "",                     "/",                    "/",
        "/",                    "/",                    "/",
        "/x",                   "",                     "/x",
        "/x",                   "/",                    "/x/",
        "/",                    "x",                    "/x",
        "dir",                  "file",                 "dir" RTPATH_SLASH_STR "file",
        "dir",                  "/file",                "dir/file",
        "dir",                  "//file",               "dir/file",
        "dir",                  "///file",              "dir/file",
        "dir/",                 "/file",                "dir/file",
        "dir/",                 "//file",               "dir/file",
        "dir/",                 "///file",              "dir/file",
        "dir//",                "file",                 "dir/file",
        "dir//",                "/file",                "dir/file",
        "dir//",                "//file",               "dir/file",
        "dir///",               "///file",              "dir/file",
        "/bin/testcase",        "foo.r0",               "/bin/testcase" RTPATH_SLASH_STR "foo.r0",
#if defined (RT_OS_OS2) || defined (RT_OS_WINDOWS)
        "/",                    "\\",                   "/",
        "\\",                   "/",                    "\\",
        "\\\\srv\\shr",         "dir//",                "\\\\srv\\shr" RTPATH_SLASH_STR "dir//",
        "\\\\srv\\shr",         "dir//file",            "\\\\srv\\shr" RTPATH_SLASH_STR "dir//file",
        "\\\\srv\\shr",         "//dir//",              "\\\\srv\\shr/dir//",
        "\\\\srv\\shr",         "/\\dir//",             "\\\\srv\\shr\\dir//",
        "\\\\",                 "not-srv/not-shr/file", "\\not-srv/not-shr/file",
        "C:",                   "autoexec.bat",         "C:autoexec.bat",
        "C:",                   "/autoexec.bat",        "C:/autoexec.bat",
        "C:",                   "\\autoexec.bat",       "C:\\autoexec.bat",
        "C:\\",                 "/autoexec.bat",        "C:\\autoexec.bat",
        "C:\\\\",               "autoexec.bat",         "C:\\autoexec.bat",
        "E:\\bin\\testcase",    "foo.r0",               "E:\\bin\\testcase" RTPATH_SLASH_STR "foo.r0",
#endif
    };
    for (unsigned i = 0; i < RT_ELEMENTS(s_apszAppendTests); i += 3)
    {
        const char *pszInput  = s_apszAppendTests[i];
        const char *pszAppend = s_apszAppendTests[i + 1];
        const char *pszExpect = s_apszAppendTests[i + 2];
        strcpy(szPath, pszInput);
        RTTESTI_CHECK_RC(rc = RTPathAppend(szPath, sizeof(szPath), pszAppend), VINF_SUCCESS);
        if (RT_FAILURE(rc))
            continue;
        if (strcmp(szPath, pszExpect))
        {
            RTTestIFailed("Unexpected result\n"
                          "   input: '%s'\n"
                          "  append: '%s'\n"
                          "  output: '%s'\n"
                          "expected: '%s'",
                          pszInput, pszAppend, szPath, pszExpect);
        }
        else
        {
            size_t const cchResult = strlen(szPath);

            strcpy(szPath, pszInput);
            RTTESTI_CHECK_RC(rc = RTPathAppend(szPath, cchResult + 2, pszAppend), VINF_SUCCESS);
            RTTESTI_CHECK(RT_FAILURE(rc) || !strcmp(szPath, pszExpect));

            strcpy(szPath, pszInput);
            RTTESTI_CHECK_RC(rc = RTPathAppend(szPath, cchResult + 1, pszAppend), VINF_SUCCESS);
            RTTESTI_CHECK(RT_FAILURE(rc) || !strcmp(szPath, pszExpect));

            if (strlen(pszInput) < cchResult)
            {
                strcpy(szPath, pszInput);
                RTTESTI_CHECK_RC(RTPathAppend(szPath, cchResult, pszAppend), VERR_BUFFER_OVERFLOW);
            }
        }
    }

    /*
     * RTPathJoin - reuse the append tests.
     */
    RTTestSub(hTest, "RTPathJoin");
    for (unsigned i = 0; i < RT_ELEMENTS(s_apszAppendTests); i += 3)
    {
        const char *pszInput  = s_apszAppendTests[i];
        const char *pszAppend = s_apszAppendTests[i + 1];
        const char *pszExpect = s_apszAppendTests[i + 2];

        memset(szPath, 'a', sizeof(szPath)); szPath[sizeof(szPath) - 1] = '\0';

        RTTESTI_CHECK_RC(rc = RTPathJoin(szPath, sizeof(szPath), pszInput, pszAppend), VINF_SUCCESS);
        if (RT_FAILURE(rc))
            continue;
        if (strcmp(szPath, pszExpect))
        {
            RTTestIFailed("Unexpected result\n"
                          "   input: '%s'\n"
                          "  append: '%s'\n"
                          "  output: '%s'\n"
                          "expected: '%s'",
                          pszInput, pszAppend, szPath, pszExpect);
        }
        else
        {
            size_t const cchResult = strlen(szPath);

            memset(szPath, 'a', sizeof(szPath)); szPath[sizeof(szPath) - 1] = '\0';
            RTTESTI_CHECK_RC(rc = RTPathJoin(szPath, cchResult + 2, pszInput, pszAppend), VINF_SUCCESS);
            RTTESTI_CHECK(RT_FAILURE(rc) || !strcmp(szPath, pszExpect));

            memset(szPath, 'a', sizeof(szPath)); szPath[sizeof(szPath) - 1] = '\0';
            RTTESTI_CHECK_RC(rc = RTPathJoin(szPath, cchResult + 1, pszInput, pszAppend), VINF_SUCCESS);
            RTTESTI_CHECK(RT_FAILURE(rc) || !strcmp(szPath, pszExpect));

            RTTESTI_CHECK_RC(rc = RTPathJoin(szPath, cchResult, pszInput, pszAppend), VERR_BUFFER_OVERFLOW);
        }
    }

    /*
     * RTPathJoinA - reuse the append tests.
     */
    RTTestSub(hTest, "RTPathJoinA");
    for (unsigned i = 0; i < RT_ELEMENTS(s_apszAppendTests); i += 3)
    {
        const char *pszInput  = s_apszAppendTests[i];
        const char *pszAppend = s_apszAppendTests[i + 1];
        const char *pszExpect = s_apszAppendTests[i + 2];

        char *pszPathDst;
        RTTESTI_CHECK(pszPathDst = RTPathJoinA(pszInput, pszAppend));
        if (!pszPathDst)
            continue;
        if (strcmp(pszPathDst, pszExpect))
        {
            RTTestIFailed("Unexpected result\n"
                          "   input: '%s'\n"
                          "  append: '%s'\n"
                          "  output: '%s'\n"
                          "expected: '%s'",
                          pszInput, pszAppend, pszPathDst, pszExpect);
        }
        RTStrFree(pszPathDst);
    }

    /*
     * RTPathStripTrailingSlash
     */
    static const char *s_apszStripTrailingSlash[] =
    {
     /* input                   result */
        "/",                    "/",
        "//",                   "/",
        "////////////////////", "/",
        "/tmp",                 "/tmp",
        "/tmp////////////////", "/tmp",
        "tmp",                  "tmp",
        "tmp////////////////",  "tmp",
        "./",                   ".",
#if defined (RT_OS_OS2) || defined (RT_OS_WINDOWS)
        "////////////////////", "/",
        "D:",                   "D:",
        "D:/",                  "D:/",
        "D:\\",                 "D:\\",
        "D:\\/\\",              "D:\\",
        "D:/\\/\\",             "D:/",
        "C:/Temp",              "C:/Temp",
        "C:/Temp/",             "C:/Temp",
        "C:/Temp\\/",           "C:/Temp",
#endif
    };
    for (unsigned i = 0; i < RT_ELEMENTS(s_apszStripTrailingSlash); i += 2)
    {
        const char *pszInput  = s_apszStripTrailingSlash[i];
        const char *pszExpect = s_apszStripTrailingSlash[i + 1];

        strcpy(szPath, pszInput);
        cch = RTPathStripTrailingSlash(szPath);
        if (strcmp(szPath, pszExpect))
            RTTestIFailed("Unexpected result\n"
                          "   input: '%s'\n"
                          "  output: '%s'\n"
                          "expected: '%s'",
                          pszInput, szPath, pszExpect);
        else
            RTTESTI_CHECK(cch == strlen(szPath));
    }

    /*
     * RTPathCountComponents
     */
    RTTestSub(hTest, "RTPathCountComponents");
    RTTESTI_CHECK(RTPathCountComponents("") == 0);
    RTTESTI_CHECK(RTPathCountComponents("/") == 1);
    RTTESTI_CHECK(RTPathCountComponents("//") == 1);
    RTTESTI_CHECK(RTPathCountComponents("//////////////") == 1);
    RTTESTI_CHECK(RTPathCountComponents("//////////////bin") == 2);
    RTTESTI_CHECK(RTPathCountComponents("//////////////bin/") == 2);
    RTTESTI_CHECK(RTPathCountComponents("//////////////bin/////") == 2);
    RTTESTI_CHECK(RTPathCountComponents("..") == 1);
    RTTESTI_CHECK(RTPathCountComponents("../") == 1);
    RTTESTI_CHECK(RTPathCountComponents("../..") == 2);
    RTTESTI_CHECK(RTPathCountComponents("../../") == 2);
#if defined (RT_OS_OS2) || defined (RT_OS_WINDOWS)
    RTTESTI_CHECK(RTPathCountComponents("d:") == 1);
    RTTESTI_CHECK(RTPathCountComponents("d:/") == 1);
    RTTESTI_CHECK(RTPathCountComponents("d:/\\") == 1);
    RTTESTI_CHECK(RTPathCountComponents("d:\\") == 1);
    RTTESTI_CHECK(RTPathCountComponents("c:\\config.sys") == 2);
    RTTESTI_CHECK(RTPathCountComponents("c:\\windows") == 2);
    RTTESTI_CHECK(RTPathCountComponents("c:\\windows\\") == 2);
    RTTESTI_CHECK(RTPathCountComponents("c:\\windows\\system32") == 3);
    RTTESTI_CHECK(RTPathCountComponents("//./C$") == 1);
    RTTESTI_CHECK(RTPathCountComponents("\\\\.\\C$") == 1);
    RTTESTI_CHECK(RTPathCountComponents("/\\.\\C$") == 1);
    RTTESTI_CHECK(RTPathCountComponents("//myserver") == 1);
    RTTESTI_CHECK(RTPathCountComponents("//myserver/") == 1);
    RTTESTI_CHECK(RTPathCountComponents("//myserver/share") == 1);
    RTTESTI_CHECK(RTPathCountComponents("//myserver/share/") == 1);
    RTTESTI_CHECK(RTPathCountComponents("//myserver/share\\") == 1);
    RTTESTI_CHECK(RTPathCountComponents("//myserver/share\\x") == 2);
    RTTESTI_CHECK(RTPathCountComponents("//myserver/share\\x\\y") == 3);
    RTTESTI_CHECK(RTPathCountComponents("//myserver/share\\x\\y\\") == 3);
#endif

    /*
     * RTPathCopyComponents
     */
    struct
    {
        const char *pszSrc;
        size_t      cComponents;
        const char *pszResult;
    } s_aCopyComponents[] =
    {
        { "",                           0, "" },
        { "",                           5, "" },
        { "/",                          0, "" },
        { "/",                          1, "/" },
        { "/",                          2, "/" },
        { "/usr/bin/sed",               0, "" },
        { "/usr/bin/sed",               1, "/" },
        { "/usr/bin/sed",               2, "/usr/" },
        { "/usr/bin/sed",               3, "/usr/bin/" },
        { "/usr/bin/sed",               4, "/usr/bin/sed" },
        { "/usr/bin/sed",               5, "/usr/bin/sed" },
        { "/usr/bin/sed",               6, "/usr/bin/sed" },
        { "/usr///bin/sed",             2, "/usr///" },
    };
    for (unsigned i = 0; i < RT_ELEMENTS(s_aCopyComponents); i++)
    {
        const char *pszInput    = s_aCopyComponents[i].pszSrc;
        size_t      cComponents = s_aCopyComponents[i].cComponents;
        const char *pszResult   = s_aCopyComponents[i].pszResult;

        memset(szPath, 'a', sizeof(szPath));
        rc = RTPathCopyComponents(szPath, sizeof(szPath), pszInput, cComponents);
        RTTESTI_CHECK_RC(rc, VINF_SUCCESS);
        if (RT_SUCCESS(rc) && strcmp(szPath, pszResult))
            RTTestIFailed("Unexpected result\n"
                          "   input: '%s' cComponents=%u\n"
                          "  output: '%s'\n"
                          "expected: '%s'",
                          pszInput, cComponents, szPath, pszResult);
        else if (RT_SUCCESS(rc))
        {
            RTTESTI_CHECK_RC(RTPathCopyComponents(szPath, strlen(pszResult) + 1, pszInput, cComponents), VINF_SUCCESS);
            RTTESTI_CHECK_RC(RTPathCopyComponents(szPath, strlen(pszResult), pszInput, cComponents), VERR_BUFFER_OVERFLOW);
        }
    }


    /*
     * RTPathStripSuffix
     */
    RTTestSub(hTest, "RTPathStripSuffix");
    struct
    {
        const char *pszSrc;
        const char *pszResult;
    } s_aStripExt[] =
    {
        { "filename.ext",               "filename" },
        { "filename.ext1.ext2.ext3",    "filename.ext1.ext2" },
        { "filename..ext",              "filename." },
        { "filename.ext.",              "filename.ext." },
    };
    for (unsigned i = 0; i < RT_ELEMENTS(s_aStripExt); i++)
    {
        const char *pszInput    = s_aStripExt[i].pszSrc;
        const char *pszResult   = s_aStripExt[i].pszResult;

        strcpy(szPath, pszInput);
        RTPathStripSuffix(szPath);
        if (strcmp(szPath, pszResult))
            RTTestIFailed("Unexpected result\n"
                          "   input: '%s'\n"
                          "  output: '%s'\n"
                          "expected: '%s'",
                          pszInput, szPath, pszResult);
    }

    /*
     * RTPathCalcRelative
     */
    RTTestSub(hTest, "RTPathCalcRelative");
    struct
    {
        const char *pszFrom;
        const char *pszTo;
        int rc;
        const char *pszExpected;
    } s_aRelPath[] =
    {
        { "/home/test.ext", "/home/test2.ext", VINF_SUCCESS, "test2.ext"},
        { "/dir/test.ext", "/dir/dir2/test2.ext", VINF_SUCCESS, "dir2/test2.ext"},
        { "/dir/dir2/test.ext", "/dir/test2.ext", VINF_SUCCESS, ".." RTPATH_SLASH_STR "test2.ext"},
        { "/dir/dir2/test.ext", "/dir/dir3/test2.ext", VINF_SUCCESS, ".." RTPATH_SLASH_STR "dir3/test2.ext"},
#if defined (RT_OS_OS2) || defined (RT_OS_WINDOWS)
        { "\\\\server\\share\\test.ext", "\\\\server\\share2\\test2.ext", VERR_NOT_SUPPORTED, ""},
        { "c:\\dir\\test.ext", "f:\\dir\\test.ext", VERR_NOT_SUPPORTED, ""}
#endif
    };
    for (unsigned i = 0; i < RT_ELEMENTS(s_aRelPath); i++)
    {
        const char *pszFrom = s_aRelPath[i].pszFrom;
        const char *pszTo   = s_aRelPath[i].pszTo;

        rc = RTPathCalcRelative(szPath, sizeof(szPath), pszFrom, pszTo);
        if (rc != s_aRelPath[i].rc)
            RTTestIFailed("Unexpected return code\n"
                          "     got: %Rrc\n"
                          "expected: %Rrc",
                          rc, s_aRelPath[i].rc);
        else if (   RT_SUCCESS(rc)
                 && strcmp(szPath, s_aRelPath[i].pszExpected))
            RTTestIFailed("Unexpected result\n"
                          "    from: '%s'\n"
                          "      to: '%s'\n"
                          "  output: '%s'\n"
                          "expected: '%s'",
                          pszFrom, pszTo, szPath, s_aRelPath[i].pszExpected);
    }

    testParserAndSplitter(hTest);

    /*
     * Summary.
     */
    return RTTestSummaryAndDestroy(hTest);
}

