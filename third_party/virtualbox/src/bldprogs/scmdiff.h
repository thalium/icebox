/* $Id: scmdiff.h $ */
/** @file
 * IPRT Testcase / Tool - Source Code Massager Diff Code.
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

#ifndef ___scmdiff_h___
#define ___scmdiff_h___

#include <iprt/stream.h>
#include "scmstream.h"

RT_C_DECLS_BEGIN

/**
 * Diff state.
 */
typedef struct SCMDIFFSTATE
{
    size_t          cDiffs;
    const char     *pszFilename;

    PSCMSTREAM      pLeft;
    PSCMSTREAM      pRight;

    /** Whether to ignore end of line markers when diffing. */
    bool            fIgnoreEol;
    /** Whether to ignore trailing whitespace. */
    bool            fIgnoreTrailingWhite;
    /** Whether to ignore leading whitespace. */
    bool            fIgnoreLeadingWhite;
    /** Whether to print special characters in human readable form or not. */
    bool            fSpecialChars;
    /** The tab size. */
    size_t          cchTab;
    /** Where to push the diff. */
    PRTSTREAM       pDiff;
} SCMDIFFSTATE;
/** Pointer to a diff state. */
typedef SCMDIFFSTATE *PSCMDIFFSTATE;


size_t ScmDiffStreams(const char *pszFilename, PSCMSTREAM pLeft, PSCMSTREAM pRight, bool fIgnoreEol,
                      bool fIgnoreLeadingWhite, bool fIgnoreTrailingWhite, bool fSpecialChars,
                      size_t cchTab, PRTSTREAM pDiff);

RT_C_DECLS_END

#endif

