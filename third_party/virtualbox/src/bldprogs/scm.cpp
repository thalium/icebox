/* $Id: scm.cpp $ */
/** @file
 * IPRT Testcase / Tool - Source Code Massager.
 */

/*
 * Copyright (C) 2010-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/assert.h>
#include <iprt/ctype.h>
#include <iprt/dir.h>
#include <iprt/env.h>
#include <iprt/file.h>
#include <iprt/err.h>
#include <iprt/getopt.h>
#include <iprt/initterm.h>
#include <iprt/mem.h>
#include <iprt/message.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/process.h>
#include <iprt/stream.h>
#include <iprt/string.h>

#include "scm.h"
#include "scmdiff.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** The name of the settings files. */
#define SCM_SETTINGS_FILENAME           ".scm-settings"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * Option identifiers.
 *
 * @note    The first chunk, down to SCMOPT_TAB_SIZE, are alternately set &
 *          clear.  So, the option setting a flag (boolean) will have an even
 *          number and the one clearing it will have an odd number.
 * @note    Down to SCMOPT_LAST_SETTINGS corresponds exactly to SCMSETTINGSBASE.
 */
typedef enum SCMOPT
{
    SCMOPT_CONVERT_EOL = 10000,
    SCMOPT_NO_CONVERT_EOL,
    SCMOPT_CONVERT_TABS,
    SCMOPT_NO_CONVERT_TABS,
    SCMOPT_FORCE_FINAL_EOL,
    SCMOPT_NO_FORCE_FINAL_EOL,
    SCMOPT_FORCE_TRAILING_LINE,
    SCMOPT_NO_FORCE_TRAILING_LINE,
    SCMOPT_STRIP_TRAILING_BLANKS,
    SCMOPT_NO_STRIP_TRAILING_BLANKS,
    SCMOPT_STRIP_TRAILING_LINES,
    SCMOPT_NO_STRIP_TRAILING_LINES,
    SCMOPT_FIX_FLOWER_BOX_MARKERS,
    SCMOPT_NO_FIX_FLOWER_BOX_MARKERS,
    SCMOPT_FIX_TODOS,
    SCMOPT_NO_FIX_TODOS,
    SCMOPT_MIN_BLANK_LINES_BEFORE_FLOWER_BOX_MARKERS,
    SCMOPT_ONLY_SVN_DIRS,
    SCMOPT_NOT_ONLY_SVN_DIRS,
    SCMOPT_ONLY_SVN_FILES,
    SCMOPT_NOT_ONLY_SVN_FILES,
    SCMOPT_SET_SVN_EOL,
    SCMOPT_DONT_SET_SVN_EOL,
    SCMOPT_SET_SVN_EXECUTABLE,
    SCMOPT_DONT_SET_SVN_EXECUTABLE,
    SCMOPT_SET_SVN_KEYWORDS,
    SCMOPT_DONT_SET_SVN_KEYWORDS,
    SCMOPT_TAB_SIZE,
    SCMOPT_WIDTH,
    SCMOPT_FILTER_OUT_DIRS,
    SCMOPT_FILTER_FILES,
    SCMOPT_FILTER_OUT_FILES,
    SCMOPT_LAST_SETTINGS = SCMOPT_FILTER_OUT_FILES,
    //
    SCMOPT_DIFF_IGNORE_EOL,
    SCMOPT_DIFF_NO_IGNORE_EOL,
    SCMOPT_DIFF_IGNORE_SPACE,
    SCMOPT_DIFF_NO_IGNORE_SPACE,
    SCMOPT_DIFF_IGNORE_LEADING_SPACE,
    SCMOPT_DIFF_NO_IGNORE_LEADING_SPACE,
    SCMOPT_DIFF_IGNORE_TRAILING_SPACE,
    SCMOPT_DIFF_NO_IGNORE_TRAILING_SPACE,
    SCMOPT_DIFF_SPECIAL_CHARS,
    SCMOPT_DIFF_NO_SPECIAL_CHARS,
    SCMOPT_END
} SCMOPT;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
const char          g_szTabSpaces[16+1]     = "                ";
const char          g_szAsterisks[255+1]    =
"****************************************************************************************************"
"****************************************************************************************************"
"*******************************************************";
const char          g_szSpaces[255+1]       =
"                                                                                                    "
"                                                                                                    "
"                                                       ";
static const char   g_szProgName[]          = "scm";
static const char  *g_pszChangedSuff        = "";
static bool         g_fDryRun               = true;
static bool         g_fDiffSpecialChars     = true;
static bool         g_fDiffIgnoreEol        = false;
static bool         g_fDiffIgnoreLeadingWS  = false;
static bool         g_fDiffIgnoreTrailingWS = false;
static int          g_iVerbosity            = 2;//99; //0;

/** The global settings. */
static SCMSETTINGSBASE const g_Defaults =
{
    /* .fConvertEol = */                            true,
    /* .fConvertTabs = */                           true,
    /* .fForceFinalEol = */                         true,
    /* .fForceTrailingLine = */                     false,
    /* .fStripTrailingBlanks = */                   true,
    /* .fStripTrailingLines = */                    true,
    /* .fFixFlowerBoxMarkers = */                   true,
    /* .cMinBlankLinesBeforeFlowerBoxMakers = */    2,
    /* .fFixTodos = */                              true,
    /* .fOnlySvnFiles = */                          false,
    /* .fOnlySvnDirs = */                           false,
    /* .fSetSvnEol = */                             false,
    /* .fSetSvnExecutable = */                      false,
    /* .fSetSvnKeywords = */                        false,
    /* .cchTab = */                                 8,
    /* .cchWidth = */                               130,
    /* .pszFilterFiles = */                         (char *)"",
    /* .pszFilterOutFiles = */                      (char *)"*.exe|*.com|20*-*-*.log",
    /* .pszFilterOutDirs = */                       (char *)".svn|.hg|.git|CVS",
};

/** Option definitions for the base settings. */
static RTGETOPTDEF  g_aScmOpts[] =
{
    { "--convert-eol",                      SCMOPT_CONVERT_EOL,                     RTGETOPT_REQ_NOTHING },
    { "--no-convert-eol",                   SCMOPT_NO_CONVERT_EOL,                  RTGETOPT_REQ_NOTHING },
    { "--convert-tabs",                     SCMOPT_CONVERT_TABS,                    RTGETOPT_REQ_NOTHING },
    { "--no-convert-tabs",                  SCMOPT_NO_CONVERT_TABS,                 RTGETOPT_REQ_NOTHING },
    { "--force-final-eol",                  SCMOPT_FORCE_FINAL_EOL,                 RTGETOPT_REQ_NOTHING },
    { "--no-force-final-eol",               SCMOPT_NO_FORCE_FINAL_EOL,              RTGETOPT_REQ_NOTHING },
    { "--force-trailing-line",              SCMOPT_FORCE_TRAILING_LINE,             RTGETOPT_REQ_NOTHING },
    { "--no-force-trailing-line",           SCMOPT_NO_FORCE_TRAILING_LINE,          RTGETOPT_REQ_NOTHING },
    { "--strip-trailing-blanks",            SCMOPT_STRIP_TRAILING_BLANKS,           RTGETOPT_REQ_NOTHING },
    { "--no-strip-trailing-blanks",         SCMOPT_NO_STRIP_TRAILING_BLANKS,        RTGETOPT_REQ_NOTHING },
    { "--strip-trailing-lines",             SCMOPT_STRIP_TRAILING_LINES,            RTGETOPT_REQ_NOTHING },
    { "--strip-no-trailing-lines",          SCMOPT_NO_STRIP_TRAILING_LINES,         RTGETOPT_REQ_NOTHING },
    { "--min-blank-lines-before-flower-box-makers", SCMOPT_FIX_FLOWER_BOX_MARKERS,  RTGETOPT_REQ_UINT8 },
    { "--fix-flower-box-markers",           SCMOPT_FIX_FLOWER_BOX_MARKERS,          RTGETOPT_REQ_NOTHING },
    { "--no-fix-flower-box-markers",        SCMOPT_NO_FIX_FLOWER_BOX_MARKERS,       RTGETOPT_REQ_NOTHING },
    { "--fix-todos",                        SCMOPT_FIX_TODOS,                       RTGETOPT_REQ_NOTHING },
    { "--no-fix-todos",                     SCMOPT_NO_FIX_TODOS,                    RTGETOPT_REQ_NOTHING },
    { "--only-svn-dirs",                    SCMOPT_ONLY_SVN_DIRS,                   RTGETOPT_REQ_NOTHING },
    { "--not-only-svn-dirs",                SCMOPT_NOT_ONLY_SVN_DIRS,               RTGETOPT_REQ_NOTHING },
    { "--only-svn-files",                   SCMOPT_ONLY_SVN_FILES,                  RTGETOPT_REQ_NOTHING },
    { "--not-only-svn-files",               SCMOPT_NOT_ONLY_SVN_FILES,              RTGETOPT_REQ_NOTHING },
    { "--set-svn-eol",                      SCMOPT_SET_SVN_EOL,                     RTGETOPT_REQ_NOTHING },
    { "--dont-set-svn-eol",                 SCMOPT_DONT_SET_SVN_EOL,                RTGETOPT_REQ_NOTHING },
    { "--set-svn-executable",               SCMOPT_SET_SVN_EXECUTABLE,              RTGETOPT_REQ_NOTHING },
    { "--dont-set-svn-executable",          SCMOPT_DONT_SET_SVN_EXECUTABLE,         RTGETOPT_REQ_NOTHING },
    { "--set-svn-keywords",                 SCMOPT_SET_SVN_KEYWORDS,                RTGETOPT_REQ_NOTHING },
    { "--dont-set-svn-keywords",            SCMOPT_DONT_SET_SVN_KEYWORDS,           RTGETOPT_REQ_NOTHING },
    { "--tab-size",                         SCMOPT_TAB_SIZE,                        RTGETOPT_REQ_UINT8   },
    { "--width",                            SCMOPT_WIDTH,                           RTGETOPT_REQ_UINT8   },
    { "--filter-out-dirs",                  SCMOPT_FILTER_OUT_DIRS,                 RTGETOPT_REQ_STRING  },
    { "--filter-files",                     SCMOPT_FILTER_FILES,                    RTGETOPT_REQ_STRING  },
    { "--filter-out-files",                 SCMOPT_FILTER_OUT_FILES,                RTGETOPT_REQ_STRING  },
};

/** Consider files matching the following patterns (base names only). */
static const char  *g_pszFileFilter         = NULL;

static PFNSCMREWRITER const g_aRewritersFor_Makefile_kup[] =
{
    rewrite_SvnNoExecutable,
    rewrite_Makefile_kup
};

static PFNSCMREWRITER const g_aRewritersFor_Makefile_kmk[] =
{
    rewrite_ForceNativeEol,
    rewrite_StripTrailingBlanks,
    rewrite_AdjustTrailingLines,
    rewrite_SvnNoExecutable,
    rewrite_SvnKeywords,
    rewrite_Makefile_kmk
};

static PFNSCMREWRITER const g_aRewritersFor_C_and_CPP[] =
{
    rewrite_ForceNativeEol,
    rewrite_ExpandTabs,
    rewrite_StripTrailingBlanks,
    rewrite_AdjustTrailingLines,
    rewrite_SvnNoExecutable,
    rewrite_SvnKeywords,
    rewrite_FixFlowerBoxMarkers,
    rewrite_Fix_C_and_CPP_Todos,
    rewrite_C_and_CPP
};

static PFNSCMREWRITER const g_aRewritersFor_H_and_HPP[] =
{
    rewrite_ForceNativeEol,
    rewrite_ExpandTabs,
    rewrite_StripTrailingBlanks,
    rewrite_AdjustTrailingLines,
    rewrite_SvnNoExecutable,
    rewrite_C_and_CPP
};

static PFNSCMREWRITER const g_aRewritersFor_RC[] =
{
    rewrite_ForceNativeEol,
    rewrite_ExpandTabs,
    rewrite_StripTrailingBlanks,
    rewrite_AdjustTrailingLines,
    rewrite_SvnNoExecutable,
    rewrite_SvnKeywords
};

static PFNSCMREWRITER const g_aRewritersFor_DEF[] =
{
    rewrite_ForceNativeEol,
    rewrite_ExpandTabs,
    rewrite_StripTrailingBlanks,
    rewrite_AdjustTrailingLines,
    rewrite_SvnNoExecutable,
    rewrite_SvnKeywords
};

static PFNSCMREWRITER const g_aRewritersFor_ShellScripts[] =
{
    rewrite_ForceLF,
    rewrite_ExpandTabs,
    rewrite_StripTrailingBlanks
};

static PFNSCMREWRITER const g_aRewritersFor_BatchFiles[] =
{
    rewrite_ForceCRLF,
    rewrite_ExpandTabs,
    rewrite_StripTrailingBlanks
};

static PFNSCMREWRITER const g_aRewritersFor_SedScripts[] =
{
    rewrite_ForceLF,
    rewrite_ExpandTabs,
    rewrite_StripTrailingBlanks
};

static PFNSCMREWRITER const g_aRewritersFor_Python[] =
{
    /** @todo rewrite_ForceLFIfExecutable */
    rewrite_ExpandTabs,
    rewrite_StripTrailingBlanks,
    rewrite_AdjustTrailingLines,
    rewrite_SvnKeywords
};


static SCMCFGENTRY const g_aConfigs[] =
{
    { RT_ELEMENTS(g_aRewritersFor_Makefile_kup), &g_aRewritersFor_Makefile_kup[0], "Makefile.kup" },
    { RT_ELEMENTS(g_aRewritersFor_Makefile_kmk), &g_aRewritersFor_Makefile_kmk[0], "Makefile.kmk|Config.kmk" },
    { RT_ELEMENTS(g_aRewritersFor_C_and_CPP),    &g_aRewritersFor_C_and_CPP[0],    "*.c|*.cpp|*.C|*.CPP|*.cxx|*.cc|*.m|*.mm" },
    { RT_ELEMENTS(g_aRewritersFor_H_and_HPP),    &g_aRewritersFor_H_and_HPP[0],    "*.h|*.hpp" },
    { RT_ELEMENTS(g_aRewritersFor_RC),           &g_aRewritersFor_RC[0],           "*.rc" },
    { RT_ELEMENTS(g_aRewritersFor_DEF),          &g_aRewritersFor_DEF[0],          "*.def" },
    { RT_ELEMENTS(g_aRewritersFor_ShellScripts), &g_aRewritersFor_ShellScripts[0], "*.sh|configure" },
    { RT_ELEMENTS(g_aRewritersFor_BatchFiles),   &g_aRewritersFor_BatchFiles[0],   "*.bat|*.cmd|*.btm|*.vbs|*.ps1" },
    { RT_ELEMENTS(g_aRewritersFor_SedScripts),   &g_aRewritersFor_SedScripts[0],   "*.sed" },
    { RT_ELEMENTS(g_aRewritersFor_Python),       &g_aRewritersFor_Python[0],       "*.py" },
};



/* -=-=-=-=-=- settings -=-=-=-=-=- */


/**
 * Init a settings structure with settings from @a pSrc.
 *
 * @returns IPRT status code
 * @param   pSettings           The settings.
 * @param   pSrc                The source settings.
 */
static int scmSettingsBaseInitAndCopy(PSCMSETTINGSBASE pSettings, PCSCMSETTINGSBASE pSrc)
{
    *pSettings = *pSrc;

    int rc = RTStrDupEx(&pSettings->pszFilterFiles, pSrc->pszFilterFiles);
    if (RT_SUCCESS(rc))
    {
        rc = RTStrDupEx(&pSettings->pszFilterOutFiles, pSrc->pszFilterOutFiles);
        if (RT_SUCCESS(rc))
        {
            rc = RTStrDupEx(&pSettings->pszFilterOutDirs, pSrc->pszFilterOutDirs);
            if (RT_SUCCESS(rc))
                return VINF_SUCCESS;

            RTStrFree(pSettings->pszFilterOutFiles);
        }
        RTStrFree(pSettings->pszFilterFiles);
    }

    pSettings->pszFilterFiles = NULL;
    pSettings->pszFilterOutFiles = NULL;
    pSettings->pszFilterOutDirs = NULL;
    return rc;
}

/**
 * Init a settings structure.
 *
 * @returns IPRT status code
 * @param   pSettings           The settings.
 */
static int scmSettingsBaseInit(PSCMSETTINGSBASE pSettings)
{
    return scmSettingsBaseInitAndCopy(pSettings, &g_Defaults);
}

/**
 * Deletes the settings, i.e. free any dynamically allocated content.
 *
 * @param   pSettings           The settings.
 */
static void scmSettingsBaseDelete(PSCMSETTINGSBASE pSettings)
{
    if (pSettings)
    {
        Assert(pSettings->cchTab != UINT8_MAX);
        pSettings->cchTab = UINT8_MAX;

        RTStrFree(pSettings->pszFilterFiles);
        pSettings->pszFilterFiles = NULL;

        RTStrFree(pSettings->pszFilterOutFiles);
        pSettings->pszFilterOutFiles = NULL;

        RTStrFree(pSettings->pszFilterOutDirs);
        pSettings->pszFilterOutDirs = NULL;
    }
}


/**
 * Processes a RTGetOpt result.
 *
 * @retval  VINF_SUCCESS if handled.
 * @retval  VERR_OUT_OF_RANGE if the option value was out of range.
 * @retval  VERR_GETOPT_UNKNOWN_OPTION if the option was not recognized.
 *
 * @param   pSettings           The settings to change.
 * @param   rc                  The RTGetOpt return value.
 * @param   pValueUnion         The RTGetOpt value union.
 */
static int scmSettingsBaseHandleOpt(PSCMSETTINGSBASE pSettings, int rc, PRTGETOPTUNION pValueUnion)
{
    switch (rc)
    {
        case SCMOPT_CONVERT_EOL:
            pSettings->fConvertEol = true;
            return VINF_SUCCESS;
        case SCMOPT_NO_CONVERT_EOL:
            pSettings->fConvertEol = false;
            return VINF_SUCCESS;

        case SCMOPT_CONVERT_TABS:
            pSettings->fConvertTabs = true;
            return VINF_SUCCESS;
        case SCMOPT_NO_CONVERT_TABS:
            pSettings->fConvertTabs = false;
            return VINF_SUCCESS;

        case SCMOPT_FORCE_FINAL_EOL:
            pSettings->fForceFinalEol = true;
            return VINF_SUCCESS;
        case SCMOPT_NO_FORCE_FINAL_EOL:
            pSettings->fForceFinalEol = false;
            return VINF_SUCCESS;

        case SCMOPT_FORCE_TRAILING_LINE:
            pSettings->fForceTrailingLine = true;
            return VINF_SUCCESS;
        case SCMOPT_NO_FORCE_TRAILING_LINE:
            pSettings->fForceTrailingLine = false;
            return VINF_SUCCESS;


        case SCMOPT_STRIP_TRAILING_BLANKS:
            pSettings->fStripTrailingBlanks = true;
            return VINF_SUCCESS;
        case SCMOPT_NO_STRIP_TRAILING_BLANKS:
            pSettings->fStripTrailingBlanks = false;
            return VINF_SUCCESS;

        case SCMOPT_MIN_BLANK_LINES_BEFORE_FLOWER_BOX_MARKERS:
            pSettings->cMinBlankLinesBeforeFlowerBoxMakers = pValueUnion->u8;
            return VINF_SUCCESS;


        case SCMOPT_STRIP_TRAILING_LINES:
            pSettings->fStripTrailingLines = true;
            return VINF_SUCCESS;
        case SCMOPT_NO_STRIP_TRAILING_LINES:
            pSettings->fStripTrailingLines = false;
            return VINF_SUCCESS;

        case SCMOPT_FIX_FLOWER_BOX_MARKERS:
            pSettings->fFixFlowerBoxMarkers = true;
            return VINF_SUCCESS;
        case SCMOPT_NO_FIX_FLOWER_BOX_MARKERS:
            pSettings->fFixFlowerBoxMarkers = false;
            return VINF_SUCCESS;

        case SCMOPT_ONLY_SVN_DIRS:
            pSettings->fOnlySvnDirs = true;
            return VINF_SUCCESS;
        case SCMOPT_NOT_ONLY_SVN_DIRS:
            pSettings->fOnlySvnDirs = false;
            return VINF_SUCCESS;

        case SCMOPT_ONLY_SVN_FILES:
            pSettings->fOnlySvnFiles = true;
            return VINF_SUCCESS;
        case SCMOPT_NOT_ONLY_SVN_FILES:
            pSettings->fOnlySvnFiles = false;
            return VINF_SUCCESS;

        case SCMOPT_SET_SVN_EOL:
            pSettings->fSetSvnEol = true;
            return VINF_SUCCESS;
        case SCMOPT_DONT_SET_SVN_EOL:
            pSettings->fSetSvnEol = false;
            return VINF_SUCCESS;

        case SCMOPT_SET_SVN_EXECUTABLE:
            pSettings->fSetSvnExecutable = true;
            return VINF_SUCCESS;
        case SCMOPT_DONT_SET_SVN_EXECUTABLE:
            pSettings->fSetSvnExecutable = false;
            return VINF_SUCCESS;

        case SCMOPT_SET_SVN_KEYWORDS:
            pSettings->fSetSvnKeywords = true;
            return VINF_SUCCESS;
        case SCMOPT_DONT_SET_SVN_KEYWORDS:
            pSettings->fSetSvnKeywords = false;
            return VINF_SUCCESS;

        case SCMOPT_TAB_SIZE:
            if (   pValueUnion->u8 < 1
                || pValueUnion->u8 >= RT_ELEMENTS(g_szTabSpaces))
            {
                RTMsgError("Invalid tab size: %u - must be in {1..%u}\n",
                           pValueUnion->u8, RT_ELEMENTS(g_szTabSpaces) - 1);
                return VERR_OUT_OF_RANGE;
            }
            pSettings->cchTab = pValueUnion->u8;
            return VINF_SUCCESS;

        case SCMOPT_WIDTH:
            if (pValueUnion->u8 < 20 || pValueUnion->u8 > 200)
            {
                RTMsgError("Invalid width size: %u - must be in {20..200} range\n", pValueUnion->u8);
                return VERR_OUT_OF_RANGE;
            }
            pSettings->cchWidth = pValueUnion->u8;
            return VINF_SUCCESS;

        case SCMOPT_FILTER_OUT_DIRS:
        case SCMOPT_FILTER_FILES:
        case SCMOPT_FILTER_OUT_FILES:
        {
            char **ppsz = NULL;
            switch (rc)
            {
                case SCMOPT_FILTER_OUT_DIRS:    ppsz = &pSettings->pszFilterOutDirs; break;
                case SCMOPT_FILTER_FILES:       ppsz = &pSettings->pszFilterFiles; break;
                case SCMOPT_FILTER_OUT_FILES:   ppsz = &pSettings->pszFilterOutFiles; break;
            }

            /*
             * An empty string zaps the current list.
             */
            if (!*pValueUnion->psz)
                return RTStrATruncate(ppsz, 0);

            /*
             * Non-empty strings are appended to the pattern list.
             *
             * Strip leading and trailing pattern separators before attempting
             * to append it.  If it's just separators, don't do anything.
             */
            const char *pszSrc = pValueUnion->psz;
            while (*pszSrc == '|')
                pszSrc++;
            size_t cchSrc = strlen(pszSrc);
            while (cchSrc > 0 && pszSrc[cchSrc - 1] == '|')
                cchSrc--;
            if (!cchSrc)
                return VINF_SUCCESS;

            return RTStrAAppendExN(ppsz, 2,
                                   "|", *ppsz && **ppsz ? (size_t)1 : (size_t)0,
                                   pszSrc, cchSrc);
        }

        default:
            return VERR_GETOPT_UNKNOWN_OPTION;
    }
}

/**
 * Parses an option string.
 *
 * @returns IPRT status code.
 * @param   pBase               The base settings structure to apply the options
 *                              to.
 * @param   pszOptions          The options to parse.
 */
static int scmSettingsBaseParseString(PSCMSETTINGSBASE pBase, const char *pszLine)
{
    int    cArgs;
    char **papszArgs;
    int rc = RTGetOptArgvFromString(&papszArgs, &cArgs, pszLine, RTGETOPTARGV_CNV_QUOTE_BOURNE_SH, NULL);
    if (RT_SUCCESS(rc))
    {
        RTGETOPTUNION   ValueUnion;
        RTGETOPTSTATE   GetOptState;
        rc = RTGetOptInit(&GetOptState, cArgs, papszArgs, &g_aScmOpts[0], RT_ELEMENTS(g_aScmOpts), 0, 0 /*fFlags*/);
        if (RT_SUCCESS(rc))
        {
            while ((rc = RTGetOpt(&GetOptState, &ValueUnion)) != 0)
            {
                rc = scmSettingsBaseHandleOpt(pBase, rc, &ValueUnion);
                if (RT_FAILURE(rc))
                    break;
            }
        }
        RTGetOptArgvFree(papszArgs);
    }

    return rc;
}

/**
 * Parses an unterminated option string.
 *
 * @returns IPRT status code.
 * @param   pBase               The base settings structure to apply the options
 *                              to.
 * @param   pchLine             The line.
 * @param   cchLine             The line length.
 */
static int scmSettingsBaseParseStringN(PSCMSETTINGSBASE pBase, const char *pchLine, size_t cchLine)
{
    char *pszLine = RTStrDupN(pchLine, cchLine);
    if (!pszLine)
        return VERR_NO_MEMORY;
    int rc = scmSettingsBaseParseString(pBase, pszLine);
    RTStrFree(pszLine);
    return rc;
}

/**
 * Verifies the options string.
 *
 * @returns IPRT status code.
 * @param   pszOptions          The options to verify .
 */
static int scmSettingsBaseVerifyString(const char *pszOptions)
{
    SCMSETTINGSBASE Base;
    int rc = scmSettingsBaseInit(&Base);
    if (RT_SUCCESS(rc))
    {
        rc = scmSettingsBaseParseString(&Base, pszOptions);
        scmSettingsBaseDelete(&Base);
    }
    return rc;
}

/**
 * Loads settings found in editor and SCM settings directives within the
 * document (@a pStream).
 *
 * @returns IPRT status code.
 * @param   pBase               The settings base to load settings into.
 * @param   pStream             The stream to scan for settings directives.
 */
static int scmSettingsBaseLoadFromDocument(PSCMSETTINGSBASE pBase, PSCMSTREAM pStream)
{
    /** @todo Editor and SCM settings directives in documents.  */
    RT_NOREF2(pBase, pStream);
    return VINF_SUCCESS;
}

/**
 * Creates a new settings file struct, cloning @a pSettings.
 *
 * @returns IPRT status code.
 * @param   ppSettings          Where to return the new struct.
 * @param   pSettingsBase       The settings to inherit from.
 */
static int scmSettingsCreate(PSCMSETTINGS *ppSettings, PCSCMSETTINGSBASE pSettingsBase)
{
    PSCMSETTINGS pSettings = (PSCMSETTINGS)RTMemAlloc(sizeof(*pSettings));
    if (!pSettings)
        return VERR_NO_MEMORY;
    int rc = scmSettingsBaseInitAndCopy(&pSettings->Base, pSettingsBase);
    if (RT_SUCCESS(rc))
    {
        pSettings->pDown   = NULL;
        pSettings->pUp     = NULL;
        pSettings->paPairs = NULL;
        pSettings->cPairs  = 0;
        *ppSettings = pSettings;
        return VINF_SUCCESS;
    }
    RTMemFree(pSettings);
    return rc;
}

/**
 * Destroys a settings structure.
 *
 * @param   pSettings           The settings structure to destroy.  NULL is OK.
 */
static void scmSettingsDestroy(PSCMSETTINGS pSettings)
{
    if (pSettings)
    {
        scmSettingsBaseDelete(&pSettings->Base);
        for (size_t i = 0; i < pSettings->cPairs; i++)
        {
            RTStrFree(pSettings->paPairs[i].pszPattern);
            RTStrFree(pSettings->paPairs[i].pszOptions);
            pSettings->paPairs[i].pszPattern = NULL;
            pSettings->paPairs[i].pszOptions = NULL;
        }
        RTMemFree(pSettings->paPairs);
        pSettings->paPairs = NULL;
        RTMemFree(pSettings);
    }
}

/**
 * Adds a pattern/options pair to the settings structure.
 *
 * @returns IPRT status code.
 * @param   pSettings           The settings.
 * @param   pchLine             The line containing the unparsed pair.
 * @param   cchLine             The length of the line.
 */
static int scmSettingsAddPair(PSCMSETTINGS pSettings, const char *pchLine, size_t cchLine)
{
    /*
     * Split the string.
     */
    const char *pchOptions = (const char *)memchr(pchLine, ':', cchLine);
    if (!pchOptions)
        return VERR_INVALID_PARAMETER;
    size_t cchPattern = pchOptions - pchLine;
    size_t cchOptions = cchLine - cchPattern - 1;
    pchOptions++;

    /* strip spaces everywhere */
    while (cchPattern > 0 && RT_C_IS_SPACE(pchLine[cchPattern - 1]))
        cchPattern--;
    while (cchPattern > 0 && RT_C_IS_SPACE(*pchLine))
        cchPattern--, pchLine++;

    while (cchOptions > 0 && RT_C_IS_SPACE(pchOptions[cchOptions - 1]))
        cchOptions--;
    while (cchOptions > 0 && RT_C_IS_SPACE(*pchOptions))
        cchOptions--, pchOptions++;

    /* Quietly ignore empty patterns and empty options. */
    if (!cchOptions || !cchPattern)
        return VINF_SUCCESS;

    /*
     * Add the pair and verify the option string.
     */
    uint32_t iPair = pSettings->cPairs;
    if ((iPair % 32) == 0)
    {
        void *pvNew = RTMemRealloc(pSettings->paPairs, (iPair + 32) * sizeof(pSettings->paPairs[0]));
        if (!pvNew)
            return VERR_NO_MEMORY;
        pSettings->paPairs = (PSCMPATRNOPTPAIR)pvNew;
    }

    pSettings->paPairs[iPair].pszPattern = RTStrDupN(pchLine, cchPattern);
    pSettings->paPairs[iPair].pszOptions = RTStrDupN(pchOptions, cchOptions);
    int rc;
    if (   pSettings->paPairs[iPair].pszPattern
        && pSettings->paPairs[iPair].pszOptions)
        rc = scmSettingsBaseVerifyString(pSettings->paPairs[iPair].pszOptions);
    else
        rc = VERR_NO_MEMORY;
    if (RT_SUCCESS(rc))
        pSettings->cPairs = iPair + 1;
    else
    {
        RTStrFree(pSettings->paPairs[iPair].pszPattern);
        RTStrFree(pSettings->paPairs[iPair].pszOptions);
    }
    return rc;
}

/**
 * Loads in the settings from @a pszFilename.
 *
 * @returns IPRT status code.
 * @param   pSettings           Where to load the settings file.
 * @param   pszFilename         The file to load.
 */
static int scmSettingsLoadFile(PSCMSETTINGS pSettings, const char *pszFilename)
{
    ScmVerbose(NULL, 3, "Loading settings file '%s'...\n", pszFilename);

    SCMSTREAM Stream;
    int rc = ScmStreamInitForReading(&Stream, pszFilename);
    if (RT_FAILURE(rc))
    {
        RTMsgError("%s: ScmStreamInitForReading -> %Rrc\n", pszFilename, rc);
        return rc;
    }

    SCMEOL      enmEol;
    const char *pchLine;
    size_t      cchLine;
    while ((pchLine = ScmStreamGetLine(&Stream, &cchLine, &enmEol)) != NULL)
    {
        /* Ignore leading spaces. */
        while (cchLine > 0 && RT_C_IS_SPACE(*pchLine))
            pchLine++, cchLine--;

        /* Ignore empty lines and comment lines. */
        if (cchLine < 1 || *pchLine == '#')
            continue;

        /* What kind of line is it? */
        const char *pchColon = (const char *)memchr(pchLine, ':', cchLine);
        if (pchColon)
            rc = scmSettingsAddPair(pSettings, pchLine, cchLine);
        else
            rc = scmSettingsBaseParseStringN(&pSettings->Base, pchLine, cchLine);
        if (RT_FAILURE(rc))
        {
            RTMsgError("%s:%d: %Rrc\n", pszFilename, ScmStreamTellLine(&Stream), rc);
            break;
        }
    }

    if (RT_SUCCESS(rc))
    {
        rc = ScmStreamGetStatus(&Stream);
        if (RT_FAILURE(rc))
            RTMsgError("%s: ScmStreamGetStatus- > %Rrc\n", pszFilename, rc);
    }

    ScmStreamDelete(&Stream);
    return rc;
}

#if 0 /* unused */
/**
 * Parse the specified settings file creating a new settings struct from it.
 *
 * @returns IPRT status code
 * @param   ppSettings          Where to return the new settings.
 * @param   pszFilename         The file to parse.
 * @param   pSettingsBase       The base settings we inherit from.
 */
static int scmSettingsCreateFromFile(PSCMSETTINGS *ppSettings, const char *pszFilename, PCSCMSETTINGSBASE pSettingsBase)
{
    PSCMSETTINGS pSettings;
    int rc = scmSettingsCreate(&pSettings, pSettingsBase);
    if (RT_SUCCESS(rc))
    {
        rc = scmSettingsLoadFile(pSettings, pszFilename);
        if (RT_SUCCESS(rc))
        {
            *ppSettings = pSettings;
            return VINF_SUCCESS;
        }

        scmSettingsDestroy(pSettings);
    }
    *ppSettings = NULL;
    return rc;
}
#endif


/**
 * Create an initial settings structure when starting processing a new file or
 * directory.
 *
 * This will look for .scm-settings files from the root and down to the
 * specified directory, combining them into the returned settings structure.
 *
 * @returns IPRT status code.
 * @param   ppSettings          Where to return the pointer to the top stack
 *                              object.
 * @param   pBaseSettings       The base settings we inherit from (globals
 *                              typically).
 * @param   pszPath             The absolute path to the new directory or file.
 */
static int scmSettingsCreateForPath(PSCMSETTINGS *ppSettings, PCSCMSETTINGSBASE pBaseSettings, const char *pszPath)
{
    *ppSettings = NULL;                 /* try shut up gcc. */

    /*
     * We'll be working with a stack copy of the path.
     */
    char    szFile[RTPATH_MAX];
    size_t  cchDir = strlen(pszPath);
    if (cchDir >= sizeof(szFile) - sizeof(SCM_SETTINGS_FILENAME))
        return VERR_FILENAME_TOO_LONG;

    /*
     * Create the bottom-most settings.
     */
    PSCMSETTINGS pSettings;
    int rc = scmSettingsCreate(&pSettings, pBaseSettings);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Enumerate the path components from the root and down. Load any setting
     * files we find.
     */
    size_t cComponents = RTPathCountComponents(pszPath);
    for (size_t i = 1; i <= cComponents; i++)
    {
        rc = RTPathCopyComponents(szFile, sizeof(szFile), pszPath, i);
        if (RT_SUCCESS(rc))
            rc = RTPathAppend(szFile, sizeof(szFile), SCM_SETTINGS_FILENAME);
        if (RT_FAILURE(rc))
            break;
        if (RTFileExists(szFile))
        {
            rc = scmSettingsLoadFile(pSettings, szFile);
            if (RT_FAILURE(rc))
                break;
        }
    }

    if (RT_SUCCESS(rc))
        *ppSettings = pSettings;
    else
        scmSettingsDestroy(pSettings);
    return rc;
}

/**
 * Pushes a new settings set onto the stack.
 *
 * @param   ppSettingsStack     The pointer to the pointer to the top stack
 *                              element.  This will be used as input and output.
 * @param   pSettings           The settings to push onto the stack.
 */
static void scmSettingsStackPush(PSCMSETTINGS *ppSettingsStack, PSCMSETTINGS pSettings)
{
    PSCMSETTINGS pOld = *ppSettingsStack;
    pSettings->pDown  = pOld;
    pSettings->pUp    = NULL;
    if (pOld)
        pOld->pUp = pSettings;
    *ppSettingsStack = pSettings;
}

/**
 * Pushes the settings of the specified directory onto the stack.
 *
 * We will load any .scm-settings in the directory.  A stack entry is added even
 * if no settings file was found.
 *
 * @returns IPRT status code.
 * @param   ppSettingsStack     The pointer to the pointer to the top stack
 *                              element.  This will be used as input and output.
 * @param   pszDir              The directory to do this for.
 */
static int scmSettingsStackPushDir(PSCMSETTINGS *ppSettingsStack, const char *pszDir)
{
    char szFile[RTPATH_MAX];
    int rc = RTPathJoin(szFile, sizeof(szFile), pszDir, SCM_SETTINGS_FILENAME);
    if (RT_SUCCESS(rc))
    {
        PSCMSETTINGS pSettings;
        rc = scmSettingsCreate(&pSettings, &(*ppSettingsStack)->Base);
        if (RT_SUCCESS(rc))
        {
            if (RTFileExists(szFile))
                rc = scmSettingsLoadFile(pSettings, szFile);
            if (RT_SUCCESS(rc))
            {
                scmSettingsStackPush(ppSettingsStack, pSettings);
                return VINF_SUCCESS;
            }

            scmSettingsDestroy(pSettings);
        }
    }
    return rc;
}


/**
 * Pops a settings set off the stack.
 *
 * @returns The popped setttings.
 * @param   ppSettingsStack     The pointer to the pointer to the top stack
 *                              element.  This will be used as input and output.
 */
static PSCMSETTINGS scmSettingsStackPop(PSCMSETTINGS *ppSettingsStack)
{
    PSCMSETTINGS pRet = *ppSettingsStack;
    PSCMSETTINGS pNew = pRet ? pRet->pDown : NULL;
    *ppSettingsStack = pNew;
    if (pNew)
        pNew->pUp    = NULL;
    if (pRet)
    {
        pRet->pUp    = NULL;
        pRet->pDown  = NULL;
    }
    return pRet;
}

/**
 * Pops and destroys the top entry of the stack.
 *
 * @param   ppSettingsStack     The pointer to the pointer to the top stack
 *                              element.  This will be used as input and output.
 */
static void scmSettingsStackPopAndDestroy(PSCMSETTINGS *ppSettingsStack)
{
    scmSettingsDestroy(scmSettingsStackPop(ppSettingsStack));
}

/**
 * Constructs the base settings for the specified file name.
 *
 * @returns IPRT status code.
 * @param   pSettingsStack      The top element on the settings stack.
 * @param   pszFilename         The file name.
 * @param   pszBasename         The base name (pointer within @a pszFilename).
 * @param   cchBasename         The length of the base name.  (For passing to
 *                              RTStrSimplePatternMultiMatch.)
 * @param   pBase               Base settings to initialize.
 */
static int scmSettingsStackMakeFileBase(PCSCMSETTINGS pSettingsStack, const char *pszFilename,
                                        const char *pszBasename, size_t cchBasename, PSCMSETTINGSBASE pBase)
{
    int rc = scmSettingsBaseInitAndCopy(pBase, &pSettingsStack->Base);
    if (RT_SUCCESS(rc))
    {
        /* find the bottom entry in the stack. */
        PCSCMSETTINGS pCur = pSettingsStack;
        while (pCur->pDown)
            pCur = pCur->pDown;

        /* Work our way up thru the stack and look for matching pairs. */
        while (pCur)
        {
            size_t const cPairs = pCur->cPairs;
            if (cPairs)
            {
                for (size_t i = 0; i < cPairs; i++)
                    if (   RTStrSimplePatternMultiMatch(pCur->paPairs[i].pszPattern, RTSTR_MAX,
                                                        pszBasename,  cchBasename, NULL)
                        || RTStrSimplePatternMultiMatch(pCur->paPairs[i].pszPattern, RTSTR_MAX,
                                                        pszFilename,  RTSTR_MAX, NULL))
                    {
                        rc = scmSettingsBaseParseString(pBase, pCur->paPairs[i].pszOptions);
                        if (RT_FAILURE(rc))
                            break;
                    }
                if (RT_FAILURE(rc))
                    break;
            }

            /* advance */
            pCur = pCur->pUp;
        }
    }
    if (RT_FAILURE(rc))
        scmSettingsBaseDelete(pBase);
    return rc;
}


/* -=-=-=-=-=- misc -=-=-=-=-=- */


/**
 * Prints a verbose message if the level is high enough.
 *
 * @param   pState              The rewrite state.  Optional.
 * @param   iLevel              The required verbosity level.
 * @param   pszFormat           The message format string.  Can be NULL if we
 *                              only want to trigger the per file message.
 * @param   ...                 Format arguments.
 */
void ScmVerbose(PSCMRWSTATE pState, int iLevel, const char *pszFormat, ...)
{
    if (iLevel <= g_iVerbosity)
    {
        if (pState && !pState->fFirst)
        {
            RTPrintf("%s: info: --= Rewriting '%s' =--\n", g_szProgName, pState->pszFilename);
            pState->fFirst = true;
        }
        if (pszFormat)
        {
            RTPrintf(pState
                     ? "%s: info:   "
                     : "%s: info: ",
                     g_szProgName);
            va_list va;
            va_start(va, pszFormat);
            RTPrintfV(pszFormat, va);
            va_end(va);
        }
    }
}


/* -=-=-=-=-=- file and directory processing -=-=-=-=-=- */


/**
 * Processes a file.
 *
 * @returns IPRT status code.
 * @param   pState              The rewriter state.
 * @param   pszFilename         The file name.
 * @param   pszBasename         The base name (pointer within @a pszFilename).
 * @param   cchBasename         The length of the base name.  (For passing to
 *                              RTStrSimplePatternMultiMatch.)
 * @param   pBaseSettings       The base settings to use.  It's OK to modify
 *                              these.
 */
static int scmProcessFileInner(PSCMRWSTATE pState, const char *pszFilename, const char *pszBasename, size_t cchBasename,
                               PSCMSETTINGSBASE pBaseSettings)
{
    /*
     * Do the file level filtering.
     */
    if (   pBaseSettings->pszFilterFiles
        && *pBaseSettings->pszFilterFiles
        && !RTStrSimplePatternMultiMatch(pBaseSettings->pszFilterFiles, RTSTR_MAX, pszBasename, cchBasename, NULL))
    {
        ScmVerbose(NULL, 5, "skipping '%s': file filter mismatch\n", pszFilename);
        return VINF_SUCCESS;
    }
    if (   pBaseSettings->pszFilterOutFiles
        && *pBaseSettings->pszFilterOutFiles
        && (   RTStrSimplePatternMultiMatch(pBaseSettings->pszFilterOutFiles, RTSTR_MAX, pszBasename, cchBasename, NULL)
            || RTStrSimplePatternMultiMatch(pBaseSettings->pszFilterOutFiles, RTSTR_MAX, pszFilename, RTSTR_MAX, NULL)) )
    {
        ScmVerbose(NULL, 5, "skipping '%s': filterd out\n", pszFilename);
        return VINF_SUCCESS;
    }
    if (   pBaseSettings->fOnlySvnFiles
        && !ScmSvnIsInWorkingCopy(pState))
    {
        ScmVerbose(NULL, 5, "skipping '%s': not in SVN WC\n", pszFilename);
        return VINF_SUCCESS;
    }

    /*
     * Try find a matching rewrite config for this filename.
     */
    PCSCMCFGENTRY pCfg = NULL;
    for (size_t iCfg = 0; iCfg < RT_ELEMENTS(g_aConfigs); iCfg++)
        if (RTStrSimplePatternMultiMatch(g_aConfigs[iCfg].pszFilePattern, RTSTR_MAX, pszBasename, cchBasename, NULL))
        {
            pCfg = &g_aConfigs[iCfg];
            break;
        }
    if (!pCfg)
    {
        ScmVerbose(NULL, 4, "skipping '%s': no rewriters configured\n", pszFilename);
        return VINF_SUCCESS;
    }
    ScmVerbose(pState, 4, "matched \"%s\"\n", pCfg->pszFilePattern);

    /*
     * Create an input stream from the file and check that it's text.
     */
    SCMSTREAM Stream1;
    int rc = ScmStreamInitForReading(&Stream1, pszFilename);
    if (RT_FAILURE(rc))
    {
        RTMsgError("Failed to read '%s': %Rrc\n", pszFilename, rc);
        return rc;
    }
    if (ScmStreamIsText(&Stream1))
    {
        ScmVerbose(pState, 3, NULL);

        /*
         * Gather SCM and editor settings from the stream.
         */
        rc = scmSettingsBaseLoadFromDocument(pBaseSettings, &Stream1);
        if (RT_SUCCESS(rc))
        {
            ScmStreamRewindForReading(&Stream1);

            /*
             * Create two more streams for output and push the text thru all the
             * rewriters, switching the two streams around when something is
             * actually rewritten.  Stream1 remains unchanged.
             */
            SCMSTREAM Stream2;
            rc = ScmStreamInitForWriting(&Stream2, &Stream1);
            if (RT_SUCCESS(rc))
            {
                SCMSTREAM Stream3;
                rc = ScmStreamInitForWriting(&Stream3, &Stream1);
                if (RT_SUCCESS(rc))
                {
                    bool        fModified = false;
                    PSCMSTREAM  pIn       = &Stream1;
                    PSCMSTREAM  pOut      = &Stream2;
                    for (size_t iRw = 0; iRw < pCfg->cRewriters; iRw++)
                    {
                        bool fRc = pCfg->papfnRewriter[iRw](pState, pIn, pOut, pBaseSettings);
                        if (fRc)
                        {
                            PSCMSTREAM pTmp = pOut;
                            pOut = pIn == &Stream1 ? &Stream3 : pIn;
                            pIn  = pTmp;
                            fModified = true;
                        }
                        ScmStreamRewindForReading(pIn);
                        ScmStreamRewindForWriting(pOut);
                    }

                    rc = ScmStreamGetStatus(&Stream1);
                    if (RT_SUCCESS(rc))
                        rc = ScmStreamGetStatus(&Stream2);
                    if (RT_SUCCESS(rc))
                        rc = ScmStreamGetStatus(&Stream3);
                    if (RT_SUCCESS(rc))
                    {
                        /*
                         * If rewritten, write it back to disk.
                         */
                        if (fModified)
                        {
                            if (!g_fDryRun)
                            {
                                ScmVerbose(pState, 1, "writing modified file to \"%s%s\"\n", pszFilename, g_pszChangedSuff);
                                rc = ScmStreamWriteToFile(pIn, "%s%s", pszFilename, g_pszChangedSuff);
                                if (RT_FAILURE(rc))
                                    RTMsgError("Error writing '%s%s': %Rrc\n", pszFilename, g_pszChangedSuff, rc);
                            }
                            else
                            {
                                ScmVerbose(pState, 1, NULL);
                                ScmDiffStreams(pszFilename, &Stream1, pIn, g_fDiffIgnoreEol, g_fDiffIgnoreLeadingWS,
                                               g_fDiffIgnoreTrailingWS, g_fDiffSpecialChars, pBaseSettings->cchTab, g_pStdOut);
                                ScmVerbose(pState, 2, "would have modified the file \"%s%s\"\n", pszFilename, g_pszChangedSuff);
                            }
                        }

                        /*
                         * If pending SVN property changes, apply them.
                         */
                        if (pState->cSvnPropChanges && RT_SUCCESS(rc))
                        {
                            if (!g_fDryRun)
                            {
                                rc = ScmSvnApplyChanges(pState);
                                if (RT_FAILURE(rc))
                                    RTMsgError("%s: failed to apply SVN property changes (%Rrc)\n", pszFilename, rc);
                            }
                            else
                                ScmSvnDisplayChanges(pState);
                        }

                        if (!fModified && !pState->cSvnPropChanges)
                            ScmVerbose(pState, 3, "no change\n", pszFilename);
                    }
                    else
                        RTMsgError("%s: stream error %Rrc\n", pszFilename, rc);
                    ScmStreamDelete(&Stream3);
                }
                else
                    RTMsgError("Failed to init stream for writing: %Rrc\n", rc);
                ScmStreamDelete(&Stream2);
            }
            else
                RTMsgError("Failed to init stream for writing: %Rrc\n", rc);
        }
        else
            RTMsgError("scmSettingsBaseLoadFromDocument: %Rrc\n", rc);
    }
    else
        ScmVerbose(pState, 4, "not text file: \"%s\"\n", pszFilename);
    ScmStreamDelete(&Stream1);

    return rc;
}

/**
 * Processes a file.
 *
 * This is just a wrapper for scmProcessFileInner for avoid wasting stack in the
 * directory recursion method.
 *
 * @returns IPRT status code.
 * @param   pszFilename         The file name.
 * @param   pszBasename         The base name (pointer within @a pszFilename).
 * @param   cchBasename         The length of the base name.  (For passing to
 *                              RTStrSimplePatternMultiMatch.)
 * @param   pSettingsStack      The settings stack (pointer to the top element).
 */
static int scmProcessFile(const char *pszFilename, const char *pszBasename, size_t cchBasename,
                          PSCMSETTINGS pSettingsStack)
{
    SCMSETTINGSBASE Base;
    int rc = scmSettingsStackMakeFileBase(pSettingsStack, pszFilename, pszBasename, cchBasename, &Base);
    if (RT_SUCCESS(rc))
    {
        SCMRWSTATE State;
        State.fFirst           = false;
        State.pszFilename      = pszFilename;
        State.cSvnPropChanges  = 0;
        State.paSvnPropChanges = NULL;

        rc = scmProcessFileInner(&State, pszFilename, pszBasename, cchBasename, &Base);

        size_t i = State.cSvnPropChanges;
        while (i-- > 0)
        {
            RTStrFree(State.paSvnPropChanges[i].pszName);
            RTStrFree(State.paSvnPropChanges[i].pszValue);
        }
        RTMemFree(State.paSvnPropChanges);

        scmSettingsBaseDelete(&Base);
    }
    return rc;
}


/**
 * Tries to correct RTDIRENTRY_UNKNOWN.
 *
 * @returns Corrected type.
 * @param   pszPath             The path to the object in question.
 */
static RTDIRENTRYTYPE scmFigureUnknownType(const char *pszPath)
{
    RTFSOBJINFO Info;
    int rc = RTPathQueryInfo(pszPath, &Info, RTFSOBJATTRADD_NOTHING);
    if (RT_FAILURE(rc))
        return RTDIRENTRYTYPE_UNKNOWN;
    if (RTFS_IS_DIRECTORY(Info.Attr.fMode))
        return RTDIRENTRYTYPE_DIRECTORY;
    if (RTFS_IS_FILE(Info.Attr.fMode))
        return RTDIRENTRYTYPE_FILE;
    return RTDIRENTRYTYPE_UNKNOWN;
}

/**
 * Recurse into a sub-directory and process all the files and directories.
 *
 * @returns IPRT status code.
 * @param   pszBuf              Path buffer containing the directory path on
 *                              entry.  This ends with a dot.  This is passed
 *                              along when recursing in order to save stack space
 *                              and avoid needless copying.
 * @param   cchDir              Length of our path in pszbuf.
 * @param   pEntry              Directory entry buffer.  This is also passed
 *                              along when recursing to save stack space.
 * @param   pSettingsStack      The settings stack (pointer to the top element).
 * @param   iRecursion          The recursion depth.  This is used to restrict
 *                              the recursions.
 */
static int scmProcessDirTreeRecursion(char *pszBuf, size_t cchDir, PRTDIRENTRY pEntry,
                                      PSCMSETTINGS pSettingsStack, unsigned iRecursion)
{
    int rc;
    Assert(cchDir > 1 && pszBuf[cchDir - 1] == '.');

    /*
     * Make sure we stop somewhere.
     */
    if (iRecursion > 128)
    {
        RTMsgError("recursion too deep: %d\n", iRecursion);
        return VINF_SUCCESS; /* ignore */
    }

    /*
     * Check if it's excluded by --only-svn-dir.
     */
    if (pSettingsStack->Base.fOnlySvnDirs)
    {
        if (!ScmSvnIsDirInWorkingCopy(pszBuf))
            return VINF_SUCCESS;
    }

    /*
     * Try open and read the directory.
     */
    PRTDIR pDir;
    rc = RTDirOpenFiltered(&pDir, pszBuf, RTDIRFILTER_NONE, 0);
    if (RT_FAILURE(rc))
    {
        RTMsgError("Failed to enumerate directory '%s': %Rrc", pszBuf, rc);
        return rc;
    }
    for (;;)
    {
        /* Read the next entry. */
        rc = RTDirRead(pDir, pEntry, NULL);
        if (RT_FAILURE(rc))
        {
            if (rc == VERR_NO_MORE_FILES)
                rc = VINF_SUCCESS;
            else
                RTMsgError("RTDirRead -> %Rrc\n", rc);
            break;
        }

        /* Skip '.' and '..'. */
        if (    pEntry->szName[0] == '.'
            &&  (   pEntry->cbName == 1
                 || (   pEntry->cbName == 2
                     && pEntry->szName[1] == '.')))
            continue;

        /* Enter it into the buffer so we've got a full name to work
           with when needed. */
        if (pEntry->cbName + cchDir >= RTPATH_MAX)
        {
            RTMsgError("Skipping too long entry: %s", pEntry->szName);
            continue;
        }
        memcpy(&pszBuf[cchDir - 1], pEntry->szName, pEntry->cbName + 1);

        /* Figure the type. */
        RTDIRENTRYTYPE enmType = pEntry->enmType;
        if (enmType == RTDIRENTRYTYPE_UNKNOWN)
            enmType = scmFigureUnknownType(pszBuf);

        /* Process the file or directory, skip the rest. */
        if (enmType == RTDIRENTRYTYPE_FILE)
            rc = scmProcessFile(pszBuf, pEntry->szName, pEntry->cbName, pSettingsStack);
        else if (enmType == RTDIRENTRYTYPE_DIRECTORY)
        {
            /* Append the dot for the benefit of the pattern matching. */
            if (pEntry->cbName + cchDir + 5 >= RTPATH_MAX)
            {
                RTMsgError("Skipping too deep dir entry: %s", pEntry->szName);
                continue;
            }
            memcpy(&pszBuf[cchDir - 1 + pEntry->cbName], "/.", sizeof("/."));
            size_t cchSubDir = cchDir - 1 + pEntry->cbName + sizeof("/.") - 1;

            if (   !pSettingsStack->Base.pszFilterOutDirs
                || !*pSettingsStack->Base.pszFilterOutDirs
                || (   !RTStrSimplePatternMultiMatch(pSettingsStack->Base.pszFilterOutDirs, RTSTR_MAX,
                                                     pEntry->szName, pEntry->cbName, NULL)
                    && !RTStrSimplePatternMultiMatch(pSettingsStack->Base.pszFilterOutDirs, RTSTR_MAX,
                                                     pszBuf, cchSubDir, NULL)
                   )
               )
            {
                rc = scmSettingsStackPushDir(&pSettingsStack, pszBuf);
                if (RT_SUCCESS(rc))
                {
                    rc = scmProcessDirTreeRecursion(pszBuf, cchSubDir, pEntry, pSettingsStack, iRecursion + 1);
                    scmSettingsStackPopAndDestroy(&pSettingsStack);
                }
            }
        }
        if (RT_FAILURE(rc))
            break;
    }
    RTDirClose(pDir);
    return rc;

}

/**
 * Process a directory tree.
 *
 * @returns IPRT status code.
 * @param   pszDir              The directory to start with.  This is pointer to
 *                              a RTPATH_MAX sized buffer.
 */
static int scmProcessDirTree(char *pszDir, PSCMSETTINGS pSettingsStack)
{
    /*
     * Setup the recursion.
     */
    int rc = RTPathAppend(pszDir, RTPATH_MAX, ".");
    if (RT_SUCCESS(rc))
    {
        RTDIRENTRY Entry;
        rc = scmProcessDirTreeRecursion(pszDir, strlen(pszDir), &Entry, pSettingsStack, 0);
    }
    else
        RTMsgError("RTPathAppend: %Rrc\n", rc);
    return rc;
}


/**
 * Processes a file or directory specified as an command line argument.
 *
 * @returns IPRT status code
 * @param   pszSomething        What we found in the command line arguments.
 * @param   pSettingsStack      The settings stack (pointer to the top element).
 */
static int scmProcessSomething(const char *pszSomething, PSCMSETTINGS pSettingsStack)
{
    char szBuf[RTPATH_MAX];
    int rc = RTPathAbs(pszSomething, szBuf, sizeof(szBuf));
    if (RT_SUCCESS(rc))
    {
        RTPathChangeToUnixSlashes(szBuf, false /*fForce*/);

        PSCMSETTINGS pSettings;
        rc = scmSettingsCreateForPath(&pSettings, &pSettingsStack->Base, szBuf);
        if (RT_SUCCESS(rc))
        {
            scmSettingsStackPush(&pSettingsStack, pSettings);

            if (RTFileExists(szBuf))
            {
                const char *pszBasename = RTPathFilename(szBuf);
                if (pszBasename)
                {
                    size_t cchBasename = strlen(pszBasename);
                    rc = scmProcessFile(szBuf, pszBasename, cchBasename, pSettingsStack);
                }
                else
                {
                    RTMsgError("RTPathFilename: NULL\n");
                    rc = VERR_IS_A_DIRECTORY;
                }
            }
            else
                rc = scmProcessDirTree(szBuf, pSettingsStack);

            PSCMSETTINGS pPopped = scmSettingsStackPop(&pSettingsStack);
            Assert(pPopped == pSettings); RT_NOREF_PV(pPopped);
            scmSettingsDestroy(pSettings);
        }
        else
            RTMsgError("scmSettingsInitStack: %Rrc\n", rc);
    }
    else
        RTMsgError("RTPathAbs: %Rrc\n", rc);
    return rc;
}

static void usage(PCRTGETOPTDEF paOpts, size_t cOpts)
{
    RTPrintf("VirtualBox Source Code Massager\n"
             "\n"
             "Usage: %s [options] <files & dirs>\n"
             "\n"
             "Options:\n", g_szProgName);
    for (size_t i = 0; i < cOpts; i++)
    {
        bool fAdvanceTwo = false;
        if ((paOpts[i].fFlags & RTGETOPT_REQ_MASK) == RTGETOPT_REQ_NOTHING)
        {
            fAdvanceTwo = i + 1 < cOpts
                       && (   strstr(paOpts[i+1].pszLong, "-no-") != NULL
                           || strstr(paOpts[i+1].pszLong, "-not-") != NULL
                           || strstr(paOpts[i+1].pszLong, "-dont-") != NULL
                           || (paOpts[i].iShort == 'q' && paOpts[i+1].iShort == 'v')
                           || (paOpts[i].iShort == 'd' && paOpts[i+1].iShort == 'D')
                          );
            if (fAdvanceTwo)
                RTPrintf("  %s, %s\n", paOpts[i].pszLong, paOpts[i + 1].pszLong);
            else
                RTPrintf("  %s\n", paOpts[i].pszLong);
        }
        else if ((paOpts[i].fFlags & RTGETOPT_REQ_MASK) == RTGETOPT_REQ_STRING)
            RTPrintf("  %s string\n", paOpts[i].pszLong);
        else
            RTPrintf("  %s value\n", paOpts[i].pszLong);
        switch (paOpts[i].iShort)
        {
            case 'd':
            case 'D':                           RTPrintf("      Default: --dry-run\n"); break;
            case 'f':                           RTPrintf("      Default: none\n"); break;
            case 'q':
            case 'v':                           RTPrintf("      Default: -vv\n"); break;

            case SCMOPT_DIFF_IGNORE_EOL:        RTPrintf("      Default: false\n"); break;
            case SCMOPT_DIFF_IGNORE_SPACE:      RTPrintf("      Default: false\n"); break;
            case SCMOPT_DIFF_IGNORE_LEADING_SPACE:  RTPrintf("      Default: false\n"); break;
            case SCMOPT_DIFF_IGNORE_TRAILING_SPACE: RTPrintf("      Default: false\n"); break;
            case SCMOPT_DIFF_SPECIAL_CHARS:     RTPrintf("      Default: true\n"); break;

            case SCMOPT_CONVERT_EOL:            RTPrintf("      Default: %RTbool\n", g_Defaults.fConvertEol); break;
            case SCMOPT_CONVERT_TABS:           RTPrintf("      Default: %RTbool\n", g_Defaults.fConvertTabs); break;
            case SCMOPT_FORCE_FINAL_EOL:        RTPrintf("      Default: %RTbool\n", g_Defaults.fForceFinalEol); break;
            case SCMOPT_FORCE_TRAILING_LINE:    RTPrintf("      Default: %RTbool\n", g_Defaults.fForceTrailingLine); break;
            case SCMOPT_STRIP_TRAILING_BLANKS:  RTPrintf("      Default: %RTbool\n", g_Defaults.fStripTrailingBlanks); break;
            case SCMOPT_STRIP_TRAILING_LINES:   RTPrintf("      Default: %RTbool\n", g_Defaults.fStripTrailingLines); break;
            case SCMOPT_FIX_FLOWER_BOX_MARKERS: RTPrintf("      Default: %RTbool\n", g_Defaults.fFixFlowerBoxMarkers); break;
            case SCMOPT_FIX_TODOS:              RTPrintf("      Default: %RTbool\n", g_Defaults.fFixTodos); break;
            case SCMOPT_MIN_BLANK_LINES_BEFORE_FLOWER_BOX_MARKERS: RTPrintf("      Default: %u\n", g_Defaults.cMinBlankLinesBeforeFlowerBoxMakers); break;
            case SCMOPT_ONLY_SVN_DIRS:          RTPrintf("      Default: %RTbool\n", g_Defaults.fOnlySvnDirs); break;
            case SCMOPT_ONLY_SVN_FILES:         RTPrintf("      Default: %RTbool\n", g_Defaults.fOnlySvnFiles); break;
            case SCMOPT_SET_SVN_EOL:            RTPrintf("      Default: %RTbool\n", g_Defaults.fSetSvnEol); break;
            case SCMOPT_SET_SVN_EXECUTABLE:     RTPrintf("      Default: %RTbool\n", g_Defaults.fSetSvnExecutable); break;
            case SCMOPT_SET_SVN_KEYWORDS:       RTPrintf("      Default: %RTbool\n", g_Defaults.fSetSvnKeywords); break;
            case SCMOPT_TAB_SIZE:               RTPrintf("      Default: %u\n", g_Defaults.cchTab); break;
            case SCMOPT_WIDTH:                  RTPrintf("      Default: %u\n", g_Defaults.cchWidth); break;
            case SCMOPT_FILTER_OUT_DIRS:        RTPrintf("      Default: %s\n", g_Defaults.pszFilterOutDirs); break;
            case SCMOPT_FILTER_FILES:           RTPrintf("      Default: %s\n", g_Defaults.pszFilterFiles); break;
            case SCMOPT_FILTER_OUT_FILES:       RTPrintf("      Default: %s\n", g_Defaults.pszFilterOutFiles); break;
            default: AssertMsgFailed(("i=%d %d %s\n", i, paOpts[i].iShort, paOpts[i].pszLong));
        }
        i += fAdvanceTwo;
    }

}

int main(int argc, char **argv)
{
    int rc = RTR3InitExe(argc, &argv, 0);
    if (RT_FAILURE(rc))
        return 1;

    /*
     * Init the settings.
     */
    PSCMSETTINGS pSettings;
    rc = scmSettingsCreate(&pSettings, &g_Defaults);
    if (RT_FAILURE(rc))
    {
        RTMsgError("scmSettingsCreate: %Rrc\n", rc);
        return 1;
    }

    /*
     * Parse arguments and process input in order (because this is the only
     * thing that works at the moment).
     */
    static RTGETOPTDEF s_aOpts[14 + RT_ELEMENTS(g_aScmOpts)] =
    {
        { "--dry-run",                          'd',                                    RTGETOPT_REQ_NOTHING },
        { "--real-run",                         'D',                                    RTGETOPT_REQ_NOTHING },
        { "--file-filter",                      'f',                                    RTGETOPT_REQ_STRING  },
        { "--quiet",                            'q',                                    RTGETOPT_REQ_NOTHING },
        { "--verbose",                          'v',                                    RTGETOPT_REQ_NOTHING },
        { "--diff-ignore-eol",                  SCMOPT_DIFF_IGNORE_EOL,                 RTGETOPT_REQ_NOTHING },
        { "--diff-no-ignore-eol",               SCMOPT_DIFF_NO_IGNORE_EOL,              RTGETOPT_REQ_NOTHING },
        { "--diff-ignore-space",                SCMOPT_DIFF_IGNORE_SPACE,               RTGETOPT_REQ_NOTHING },
        { "--diff-no-ignore-space",             SCMOPT_DIFF_NO_IGNORE_SPACE,            RTGETOPT_REQ_NOTHING },
        { "--diff-ignore-leading-space",        SCMOPT_DIFF_IGNORE_LEADING_SPACE,       RTGETOPT_REQ_NOTHING },
        { "--diff-no-ignore-leading-space",     SCMOPT_DIFF_NO_IGNORE_LEADING_SPACE,    RTGETOPT_REQ_NOTHING },
        { "--diff-ignore-trailing-space",       SCMOPT_DIFF_IGNORE_TRAILING_SPACE,      RTGETOPT_REQ_NOTHING },
        { "--diff-no-ignore-trailing-space",    SCMOPT_DIFF_NO_IGNORE_TRAILING_SPACE,   RTGETOPT_REQ_NOTHING },
        { "--diff-special-chars",               SCMOPT_DIFF_SPECIAL_CHARS,              RTGETOPT_REQ_NOTHING },
        { "--diff-no-special-chars",            SCMOPT_DIFF_NO_SPECIAL_CHARS,           RTGETOPT_REQ_NOTHING },
    };
    memcpy(&s_aOpts[RT_ELEMENTS(s_aOpts) - RT_ELEMENTS(g_aScmOpts)], &g_aScmOpts[0], sizeof(g_aScmOpts));

    RTGETOPTUNION   ValueUnion;
    RTGETOPTSTATE   GetOptState;
    rc = RTGetOptInit(&GetOptState, argc, argv, &s_aOpts[0], RT_ELEMENTS(s_aOpts), 1, RTGETOPTINIT_FLAGS_OPTS_FIRST);
    AssertReleaseRCReturn(rc, 1);
    size_t          cProcessed = 0;

    while ((rc = RTGetOpt(&GetOptState, &ValueUnion)) != 0)
    {
        switch (rc)
        {
            case 'd':
                g_fDryRun = true;
                break;
            case 'D':
                g_fDryRun = false;
                break;

            case 'f':
                g_pszFileFilter = ValueUnion.psz;
                break;

            case 'h':
                usage(s_aOpts, RT_ELEMENTS(s_aOpts));
                return 1;

            case 'q':
                g_iVerbosity = 0;
                break;

            case 'v':
                g_iVerbosity++;
                break;

            case 'V':
            {
                /* The following is assuming that svn does it's job here. */
                static const char s_szRev[] = "$Revision: 118411 $";
                const char *psz = RTStrStripL(strchr(s_szRev, ' '));
                RTPrintf("r%.*s\n", strchr(psz, ' ') - psz, psz);
                return 0;
            }

            case SCMOPT_DIFF_IGNORE_EOL:
                g_fDiffIgnoreEol = true;
                break;
            case SCMOPT_DIFF_NO_IGNORE_EOL:
                g_fDiffIgnoreEol = false;
                break;

            case SCMOPT_DIFF_IGNORE_SPACE:
                g_fDiffIgnoreTrailingWS = g_fDiffIgnoreLeadingWS = true;
                break;
            case SCMOPT_DIFF_NO_IGNORE_SPACE:
                g_fDiffIgnoreTrailingWS = g_fDiffIgnoreLeadingWS = false;
                break;

            case SCMOPT_DIFF_IGNORE_LEADING_SPACE:
                g_fDiffIgnoreLeadingWS = true;
                break;
            case SCMOPT_DIFF_NO_IGNORE_LEADING_SPACE:
                g_fDiffIgnoreLeadingWS = false;
                break;

            case SCMOPT_DIFF_IGNORE_TRAILING_SPACE:
                g_fDiffIgnoreTrailingWS = true;
                break;
            case SCMOPT_DIFF_NO_IGNORE_TRAILING_SPACE:
                g_fDiffIgnoreTrailingWS = false;
                break;

            case SCMOPT_DIFF_SPECIAL_CHARS:
                g_fDiffSpecialChars = true;
                break;
            case SCMOPT_DIFF_NO_SPECIAL_CHARS:
                g_fDiffSpecialChars = false;
                break;

            case VINF_GETOPT_NOT_OPTION:
            {
                if (!g_fDryRun)
                {
                    if (!cProcessed)
                    {
                        RTPrintf("%s: Warning! This program will make changes to your source files and\n"
                                 "%s:          there is a slight risk that bugs or a full disk may cause\n"
                                 "%s:          LOSS OF DATA.   So, please make sure you have checked in\n"
                                 "%s:          all your changes already.  If you didn't, then don't blame\n"
                                 "%s:          anyone for not warning you!\n"
                                 "%s:\n"
                                 "%s:          Press any key to continue...\n",
                                 g_szProgName, g_szProgName, g_szProgName, g_szProgName, g_szProgName,
                                 g_szProgName, g_szProgName);
                        RTStrmGetCh(g_pStdIn);
                    }
                    cProcessed++;
                }
                rc = scmProcessSomething(ValueUnion.psz, pSettings);
                if (RT_FAILURE(rc))
                    return rc;
                break;
            }

            default:
            {
                int rc2 = scmSettingsBaseHandleOpt(&pSettings->Base, rc, &ValueUnion);
                if (RT_SUCCESS(rc2))
                    break;
                if (rc2 != VERR_GETOPT_UNKNOWN_OPTION)
                    return 2;
                return RTGetOptPrintError(rc, &ValueUnion);
            }
        }
    }

    scmSettingsDestroy(pSettings);
    return 0;
}

