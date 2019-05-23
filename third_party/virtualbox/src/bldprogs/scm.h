/* $Id: scm.h $ */
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


#ifndef ___scm_h___
#define ___scm_h___

#include "scmstream.h"

RT_C_DECLS_BEGIN

/** Pointer to the rewriter state. */
typedef struct SCMRWSTATE *PSCMRWSTATE;
/** Pointer to const massager settings. */
typedef struct SCMSETTINGSBASE const *PCSCMSETTINGSBASE;


/** @name Subversion Access
 * @{  */

/**
 * SVN property.
 */
typedef struct SCMSVNPROP
{
    /** The property. */
    char           *pszName;
    /** The value.
     * When used to record updates, this can be set to NULL to trigger the
     * deletion of the property. */
    char           *pszValue;
} SCMSVNPROP;
/** Pointer to a SVN property. */
typedef SCMSVNPROP *PSCMSVNPROP;
/** Pointer to a const  SVN property. */
typedef SCMSVNPROP const *PCSCMSVNPROP;


bool ScmSvnIsDirInWorkingCopy(const char *pszDir);
bool ScmSvnIsInWorkingCopy(PSCMRWSTATE pState);
int  ScmSvnQueryProperty(PSCMRWSTATE pState, const char *pszName, char **ppszValue);
int  ScmSvnSetProperty(PSCMRWSTATE pState, const char *pszName, const char *pszValue);
int  ScmSvnDelProperty(PSCMRWSTATE pState, const char *pszName);
int  ScmSvnDisplayChanges(PSCMRWSTATE pState);
int  ScmSvnApplyChanges(PSCMRWSTATE pState);

/** @} */


/** @name Rewriters
 * @{ */

/**
 * Rewriter state.
 */
typedef struct SCMRWSTATE
{
    /** The filename.  */
    const char         *pszFilename;
    /** Set after the printing the first verbose message about a file under
     *  rewrite. */
    bool                fFirst;
    /** The number of SVN property changes. */
    size_t              cSvnPropChanges;
    /** Pointer to an array of SVN property changes. */
    struct SCMSVNPROP  *paSvnPropChanges;
} SCMRWSTATE;

/**
 * A rewriter.
 *
 * This works like a stream editor, reading @a pIn, modifying it and writing it
 * to @a pOut.
 *
 * @returns true if any changes were made, false if not.
 * @param   pIn                 The input stream.
 * @param   pOut                The output stream.
 * @param   pSettings           The settings.
 */
typedef bool FNSCMREWRITER(PSCMRWSTATE pState, PSCMSTREAM pIn, PSCMSTREAM pOut, PCSCMSETTINGSBASE pSettings);
/** Pointer to a rewriter method. */
typedef FNSCMREWRITER *PFNSCMREWRITER;

FNSCMREWRITER rewrite_StripTrailingBlanks;
FNSCMREWRITER rewrite_ExpandTabs;
FNSCMREWRITER rewrite_ForceNativeEol;
FNSCMREWRITER rewrite_ForceLF;
FNSCMREWRITER rewrite_ForceCRLF;
FNSCMREWRITER rewrite_AdjustTrailingLines;
FNSCMREWRITER rewrite_SvnNoExecutable;
FNSCMREWRITER rewrite_SvnKeywords;
FNSCMREWRITER rewrite_Makefile_kup;
FNSCMREWRITER rewrite_Makefile_kmk;
FNSCMREWRITER rewrite_FixFlowerBoxMarkers;
FNSCMREWRITER rewrite_Fix_C_and_CPP_Todos;
FNSCMREWRITER rewrite_C_and_CPP;

/** @}  */


/** @name Settings
 * @{ */

/**
 * Configuration entry.
 */
typedef struct SCMCFGENTRY
{
    /** Number of rewriters. */
    size_t          cRewriters;
    /** Pointer to an array of rewriters. */
    PFNSCMREWRITER const  *papfnRewriter;
    /** File pattern (simple).  */
    const char     *pszFilePattern;
} SCMCFGENTRY;
typedef SCMCFGENTRY *PSCMCFGENTRY;
typedef SCMCFGENTRY const *PCSCMCFGENTRY;


/**
 * Source Code Massager Settings.
 */
typedef struct SCMSETTINGSBASE
{
    bool            fConvertEol;
    bool            fConvertTabs;
    bool            fForceFinalEol;
    bool            fForceTrailingLine;
    bool            fStripTrailingBlanks;
    bool            fStripTrailingLines;

    /** Whether to fix C/C++ flower box section markers. */
    bool            fFixFlowerBoxMarkers;
    /** The minimum number of blank lines we want before flowerbox markers. */
    uint8_t         cMinBlankLinesBeforeFlowerBoxMakers;

    /** Whether to fix C/C++ todos. */
    bool            fFixTodos;

    /** Only process files that are part of a SVN working copy. */
    bool            fOnlySvnFiles;
    /** Only recurse into directories containing an .svn dir.  */
    bool            fOnlySvnDirs;
    /** Set svn:eol-style if missing or incorrect. */
    bool            fSetSvnEol;
    /** Set svn:executable according to type (unusually this means deleting it). */
    bool            fSetSvnExecutable;
    /** Set svn:keyword if completely or partially missing. */
    bool            fSetSvnKeywords;
    /** Tab size. */
    uint8_t         cchTab;
    /** Optimal source code width. */
    uint8_t         cchWidth;
    /** Only consider files matching these patterns.  This is only applied to the
     *  base names. */
    char           *pszFilterFiles;
    /** Filter out files matching the following patterns.  This is applied to base
     *  names as well as the absolute paths.  */
    char           *pszFilterOutFiles;
    /** Filter out directories matching the following patterns.  This is applied
     *  to base names as well as the absolute paths.  All absolute paths ends with a
     *  slash and dot ("/.").  */
    char           *pszFilterOutDirs;
} SCMSETTINGSBASE;
/** Pointer to massager settings. */
typedef SCMSETTINGSBASE *PSCMSETTINGSBASE;

/**
 * File/dir pattern + options.
 */
typedef struct SCMPATRNOPTPAIR
{
    char *pszPattern;
    char *pszOptions;
} SCMPATRNOPTPAIR;
/** Pointer to a pattern + option pair. */
typedef SCMPATRNOPTPAIR *PSCMPATRNOPTPAIR;


/** Pointer to a settings set. */
typedef struct SCMSETTINGS *PSCMSETTINGS;
/**
 * Settings set.
 *
 * This structure is constructed from the command line arguments or any
 * .scm-settings file found in a directory we recurse into.  When recursing in
 * and out of a directory, we push and pop a settings set for it.
 *
 * The .scm-settings file has two kinds of setttings, first there are the
 * unqualified base settings and then there are the settings which applies to a
 * set of files or directories.  The former are lines with command line options.
 * For the latter, the options are preceded by a string pattern and a colon.
 * The pattern specifies which files (and/or directories) the options applies
 * to.
 *
 * We parse the base options into the Base member and put the others into the
 * paPairs array.
 */
typedef struct SCMSETTINGS
{
    /** Pointer to the setting file below us in the stack. */
    PSCMSETTINGS        pDown;
    /** Pointer to the setting file above us in the stack. */
    PSCMSETTINGS        pUp;
    /** File/dir patterns and their options. */
    PSCMPATRNOPTPAIR    paPairs;
    /** The number of entires in paPairs. */
    uint32_t            cPairs;
    /** The base settings that was read out of the file. */
    SCMSETTINGSBASE     Base;
} SCMSETTINGS;
/** Pointer to a const settings set. */
typedef SCMSETTINGS const *PCSCMSETTINGS;

/** @} */


void ScmVerbose(PSCMRWSTATE pState, int iLevel, const char *pszFormat, ...);

extern const char g_szTabSpaces[16+1];
extern const char g_szAsterisks[255+1];
extern const char g_szSpaces[255+1];

RT_C_DECLS_END

#endif

