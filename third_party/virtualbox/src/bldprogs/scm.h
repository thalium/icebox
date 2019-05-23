/* $Id: scm.h $ */
/** @file
 * IPRT Testcase / Tool - Source Code Massager.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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


void ScmSvnInit(void);
void ScmSvnTerm(void);
bool ScmSvnIsDirInWorkingCopy(const char *pszDir);
bool ScmSvnIsInWorkingCopy(PSCMRWSTATE pState);
int  ScmSvnQueryProperty(PSCMRWSTATE pState, const char *pszName, char **ppszValue);
int  ScmSvnSetProperty(PSCMRWSTATE pState, const char *pszName, const char *pszValue);
int  ScmSvnDelProperty(PSCMRWSTATE pState, const char *pszName);
int  ScmSvnDisplayChanges(PSCMRWSTATE pState);
int  ScmSvnApplyChanges(PSCMRWSTATE pState);

/** @} */


/** @name Code Parsing
 * @{  */

/**
 * Comment style.
 */
typedef enum SCMCOMMENTSTYLE
{
    kScmCommentStyle_Invalid = 0,
    kScmCommentStyle_C,
    kScmCommentStyle_Hash,
    kScmCommentStyle_Python,    /**< Same as hash, except for copyright/license. */
    kScmCommentStyle_Semicolon,
    kScmCommentStyle_Rem_Upper,
    kScmCommentStyle_Rem_Lower,
    kScmCommentStyle_Rem_Camel,
    kScmCommentStyle_Sql,
    kScmCommentStyle_Tick,
    kScmCommentStyle_End
} SCMCOMMENTSTYLE;

/**
 * Comment types.
 */
typedef enum SCMCOMMENTTYPE
{
    kScmCommentType_Invalid = 0,                /**< Customary invalid zero value. */
    kScmCommentType_Line,                       /**< Line comment. */
    kScmCommentType_Line_JavaDoc,               /**< Line comment, JavaDoc style. */
    kScmCommentType_Line_JavaDoc_After,         /**< Line comment, JavaDoc after-member style. */
    kScmCommentType_Line_Qt,                    /**< Line comment, JavaDoc style. */
    kScmCommentType_Line_Qt_After,              /**< Line comment, JavaDoc after-member style. */
    kScmCommentType_MultiLine,                  /**< Multi-line comment (e.g. ansi C).  */
    kScmCommentType_MultiLine_JavaDoc,          /**< Multi-line comment, JavaDoc style.  */
    kScmCommentType_MultiLine_JavaDoc_After,    /**< Multi-line comment, JavaDoc after-member style.  */
    kScmCommentType_MultiLine_Qt,               /**< Multi-line comment, Qt style.  */
    kScmCommentType_MultiLine_Qt_After,         /**< Multi-line comment, Qt after-member style.  */
    kScmCommentType_DocString,                  /**< Triple quoted python doc string. */
    kScmCommentType_End                         /**< Customary exclusive end value. */
} SCMCOMMENTTYPE;


/**
 * Comment information.
 */
typedef struct SCMCOMMENTINFO
{
    /** Comment type. */
    SCMCOMMENTTYPE  enmType;
    /** Start line number  (0-based). */
    uint32_t        iLineStart;
    /** Start line offset (0-based). */
    uint32_t        offStart;
    /** End line number  (0-based). */
    uint32_t        iLineEnd;
    /** End line offset  (0-based). */
    uint32_t        offEnd;
    /** Number of blank lines before the body (@a pszBody). */
    uint32_t        cBlankLinesBefore;
    /** Number of blank lines after the body (@a pszBody + @a cchBody). */
    uint32_t        cBlankLinesAfter;
    /** @todo add min/max indent. Raw length. Etc. */
} SCMCOMMENTINFO;
/** Pointer to comment info. */
typedef SCMCOMMENTINFO *PSCMCOMMENTINFO;
/** Pointer to const comment info. */
typedef SCMCOMMENTINFO const *PCSCMCOMMENTINFO;


/**
 * Comment enumeration callback function.
 *
 * @returns IPRT style status code.  Failures causes immediate return.  While an
 *          informational status code is saved (first one) and returned later.
 * @param   pInfo           Additional comment info.
 * @param   pszBody         The comment body.  This is somewhat stripped.
 * @param   cchBody         The comment body length.
 * @param   pvUser          User callback argument.
 */
typedef DECLCALLBACK(int) FNSCMCOMMENTENUMERATOR(PCSCMCOMMENTINFO pInfo, const char *pszBody, size_t cchBody, void *pvUser);
/** Poiter to a omment enumeration callback function. */
typedef FNSCMCOMMENTENUMERATOR *PFNSCMCOMMENTENUMERATOR;

int ScmEnumerateComments(PSCMSTREAM pIn, SCMCOMMENTSTYLE enmCommentStyle, PFNSCMCOMMENTENUMERATOR pfnCallback, void *pvUser);

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
    /** Cached ScmSvnIsInWorkingCopy response. 0 indicates not known, 1 means it
     * is in WC, -1 means it doesn't. */
    int8_t              fIsInSvnWorkingCopy;
    /** The number of SVN property changes. */
    size_t              cSvnPropChanges;
    /** Pointer to an array of SVN property changes. */
    struct SCMSVNPROP  *paSvnPropChanges;
    /** For error propagation. */
    int32_t             rc;
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
FNSCMREWRITER rewrite_SvnNoKeywords;
FNSCMREWRITER rewrite_SvnNoEolStyle;
FNSCMREWRITER rewrite_SvnBinary;
FNSCMREWRITER rewrite_SvnKeywords;
FNSCMREWRITER rewrite_Copyright_CstyleComment;
FNSCMREWRITER rewrite_Copyright_HashComment;
FNSCMREWRITER rewrite_Copyright_PythonComment;
FNSCMREWRITER rewrite_Copyright_RemComment;
FNSCMREWRITER rewrite_Copyright_SemicolonComment;
FNSCMREWRITER rewrite_Copyright_SqlComment;
FNSCMREWRITER rewrite_Copyright_TickComment;
FNSCMREWRITER rewrite_Makefile_kup;
FNSCMREWRITER rewrite_Makefile_kmk;
FNSCMREWRITER rewrite_FixFlowerBoxMarkers;
FNSCMREWRITER rewrite_Fix_C_and_CPP_Todos;
FNSCMREWRITER rewrite_C_and_CPP;

/**
 * Rewriter configuration.
 */
typedef struct SCMREWRITERCFG
{
    /** The rewriter function. */
    PFNSCMREWRITER  pfnRewriter;
    /** The name of the rewriter. */
    const char     *pszName;
} SCMREWRITERCFG;
/** Pointer to a const rewriter config. */
typedef SCMREWRITERCFG const *PCSCMREWRITERCFG;

/** @}  */


/** @name Settings
 * @{ */

/**
 * Configuration entry.
 */
typedef struct SCMCFGENTRY
{
    /** Number of rewriters. */
    size_t                  cRewriters;
    /** Pointer to an array of rewriters. */
    PCSCMREWRITERCFG const *paRewriters;
    /** Set if the entry handles binaries.  */
    bool                    fBinary;
    /** File pattern (simple).  */
    const char             *pszFilePattern;
    /** Name (for treat as).  */
    const char             *pszName;
} SCMCFGENTRY;
typedef SCMCFGENTRY *PSCMCFGENTRY;
typedef SCMCFGENTRY const *PCSCMCFGENTRY;


/** License update options. */
typedef enum SCMLICENSE
{
    kScmLicense_LeaveAlone = 0,     /**< Leave it alone. */
    kScmLicense_OseGpl,             /**< VBox OSE GPL if public. */
    kScmLicense_OseDualGplCddl,     /**< VBox OSE dual GPL & CDDL if public. */
    kScmLicense_OseCddl,            /**< VBox OSE CDDL if public. */
    kScmLicense_Lgpl,               /**< LGPL if public. */
    kScmLicense_Mit,                /**< MIT if public. */
    kScmLicense_BasedOnMit,         /**< Copyright us but based on someone else's MIT. */
    kScmLicense_End
} SCMLICENSE;

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

    /** Update the copyright year. */
    bool            fUpdateCopyrightYear;
    /** Only external copyright holders. */
    bool            fExternalCopyright;
    /** Whether there should be a LGPL disclaimer. */
    bool            fLgplDisclaimer;
    /** How to update the license. */
    SCMLICENSE      enmUpdateLicense;

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
    /** Free the treat as structure. */
    bool            fFreeTreatAs;
    /** Prematched config entry. */
    PCSCMCFGENTRY   pTreatAs;
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
    char *pszRelativeTo;
    bool  fMultiPattern;
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


void ScmVerboseBanner(PSCMRWSTATE pState, int iLevel);
void ScmVerbose(PSCMRWSTATE pState, int iLevel, const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(3, 4);
bool ScmError(PSCMRWSTATE pState, int rc, const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(3, 4);

extern const char g_szTabSpaces[16+1];
extern const char g_szAsterisks[255+1];
extern const char g_szSpaces[255+1];
extern uint32_t g_uYear;

RT_C_DECLS_END

#endif

