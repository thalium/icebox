/* $Id: tstStrSimplePattern.cpp $ */
/** @file
 * IPRT Testcase - RTStrSimplePattern.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
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
#include <iprt/string.h>
#include <iprt/stream.h>
#include <iprt/err.h>
#include <iprt/initterm.h>


int main()
{
    int cErrors = 0;

#define CHECK_EXPR(expr) \
    do { bool const f = !!(expr); if (RT_UNLIKELY(!f)) { RTPrintf("tstStrSimplePattern(%d): %s!\n", __LINE__, #expr); cErrors++; } } while (0)
#define CHECK_EXPR_MSG(expr, msg) \
    do { \
        bool const f = !!(expr); \
        if (RT_UNLIKELY(!f)) { \
            RTPrintf("tstStrSimplePattern(%d): %s!\n", __LINE__, #expr); \
            RTPrintf("tstStrSimplePattern: "); \
            RTPrintf msg; \
            ++cErrors; \
        } \
    } while (0)

    CHECK_EXPR(RTStrSimplePatternMatch("*", ""));
    CHECK_EXPR(RTStrSimplePatternMatch("*", "asdfasdflkjasdlfkj"));
    CHECK_EXPR(RTStrSimplePatternMatch("*?*?*?*?*", "asdfasdflkjasdlfkj"));
    CHECK_EXPR(RTStrSimplePatternMatch("asdf??df", "asdfasdf"));
    CHECK_EXPR(!RTStrSimplePatternMatch("asdf??dq", "asdfasdf"));
    CHECK_EXPR(RTStrSimplePatternMatch("asdf*df", "asdfasdf"));
    CHECK_EXPR(!RTStrSimplePatternMatch("asdf*dq", "asdfasdf"));
    CHECK_EXPR(RTStrSimplePatternMatch("a*", "asdfasdf"));
    CHECK_EXPR(RTStrSimplePatternMatch("a*f", "asdfasdf"));
    CHECK_EXPR(!RTStrSimplePatternMatch("a*q", "asdfasdf"));
    CHECK_EXPR(!RTStrSimplePatternMatch("a*q?", "asdfasdf"));
    CHECK_EXPR(RTStrSimplePatternMatch("?*df", "asdfasdf"));

    CHECK_EXPR(RTStrSimplePatternNMatch("*", 1, "", 0));
    CHECK_EXPR(RTStrSimplePatternNMatch("*", ~(size_t)0, "", 0));
    CHECK_EXPR(RTStrSimplePatternNMatch("*", ~(size_t)0, "", ~(size_t)0));
    CHECK_EXPR(RTStrSimplePatternNMatch("*", 1, "asdfasdflkjasdlfkj", ~(size_t)0));
    CHECK_EXPR(RTStrSimplePatternNMatch("*", ~(size_t)0, "asdfasdflkjasdlfkj", ~(size_t)0));
    CHECK_EXPR(RTStrSimplePatternNMatch("*", 1, "asdfasdflkjasdlfkj", 3));
    CHECK_EXPR(RTStrSimplePatternNMatch("*", 2, "asdfasdflkjasdlfkj", 10));
    CHECK_EXPR(RTStrSimplePatternNMatch("*", 15, "asdfasdflkjasdlfkj", 10));
    CHECK_EXPR(RTStrSimplePatternNMatch("*?*?*?*?*", 1, "asdfasdflkjasdlfkj", 128));
    CHECK_EXPR(RTStrSimplePatternNMatch("*?*?*?*?*", 5, "asdfasdflkjasdlfkj", 0));
    CHECK_EXPR(RTStrSimplePatternNMatch("*?*?*?*?*", 5, "asdfasdflkjasdlfkj", ~(size_t)0));
    CHECK_EXPR(RTStrSimplePatternNMatch("*?*?*?*?*", ~(size_t)0, "asdfasdflkjasdlfkj", ~(size_t)0));
    CHECK_EXPR(RTStrSimplePatternNMatch("asdf??df", 8, "asdfasdf", 8));
    CHECK_EXPR(RTStrSimplePatternNMatch("asdf??df", ~(size_t)0, "asdfasdf", 8));
    CHECK_EXPR(RTStrSimplePatternNMatch("asdf??df", ~(size_t)0, "asdfasdf", ~(size_t)0));
    CHECK_EXPR(RTStrSimplePatternNMatch("asdf??df", 7, "asdfasdf", 7));
    CHECK_EXPR(!RTStrSimplePatternNMatch("asdf??df", 7, "asdfasdf", 8));
    CHECK_EXPR(!RTStrSimplePatternNMatch("asdf??dq", 8, "asdfasdf", 8));
    CHECK_EXPR(RTStrSimplePatternNMatch("asdf??dq", 7, "asdfasdf", 7));
    CHECK_EXPR(RTStrSimplePatternNMatch("asdf*df", 8, "asdfasdf", 8));
    CHECK_EXPR(!RTStrSimplePatternNMatch("asdf*dq", 8, "asdfasdf", 8));
    CHECK_EXPR(RTStrSimplePatternNMatch("a*", 10, "asdfasdf", 8));
    CHECK_EXPR(RTStrSimplePatternNMatch("a*f", 3, "asdfasdf", ~(size_t)0));
    CHECK_EXPR(!RTStrSimplePatternNMatch("a*q", 3, "asdfasdf", ~(size_t)0));
    CHECK_EXPR(!RTStrSimplePatternNMatch("a*q?", 4, "asdfasdf", 9));
    CHECK_EXPR(RTStrSimplePatternNMatch("?*df", 4, "asdfasdf", 8));

    size_t offPattern;
    CHECK_EXPR(RTStrSimplePatternMultiMatch("asdq|a*f|a??t", ~(size_t)0, "asdf", 4, NULL));
    CHECK_EXPR(RTStrSimplePatternMultiMatch("asdq|a*f|a??t", ~(size_t)0, "asdf", 4, &offPattern));
    CHECK_EXPR(offPattern == 5);
    CHECK_EXPR(RTStrSimplePatternMultiMatch("asdq|a??t|a??f", ~(size_t)0, "asdf", 4, NULL));
    CHECK_EXPR(RTStrSimplePatternMultiMatch("asdq|a??t|a??f", ~(size_t)0, "asdf", 4, &offPattern));
    CHECK_EXPR(offPattern == 10);
    CHECK_EXPR(RTStrSimplePatternMultiMatch("a*f|a??t|a??f", ~(size_t)0, "asdf", 4, NULL));
    CHECK_EXPR(RTStrSimplePatternMultiMatch("a*f|a??t|a??f", ~(size_t)0, "asdf", 4, &offPattern));
    CHECK_EXPR(offPattern == 0);
    CHECK_EXPR(!RTStrSimplePatternMultiMatch("asdq|a??y|a??x", ~(size_t)0, "asdf", 4, NULL));
    CHECK_EXPR(!RTStrSimplePatternMultiMatch("asdq|a??y|a??x", ~(size_t)0, "asdf", 4, &offPattern));
    CHECK_EXPR(offPattern == ~(size_t)0);
    CHECK_EXPR(RTStrSimplePatternMultiMatch("asdq|a*f|a??t", 9, "asdf", 4, NULL));
    CHECK_EXPR(RTStrSimplePatternMultiMatch("asdq|a*f|a??t", 8, "asdf", 4, NULL));
    CHECK_EXPR(RTStrSimplePatternMultiMatch("asdq|a*f|a??t", 7, "asdf", 4, NULL));
    CHECK_EXPR(!RTStrSimplePatternMultiMatch("asdq|a*f|a??t", 6, "asdf", 4, NULL));
    CHECK_EXPR(!RTStrSimplePatternMultiMatch("asdq|a*f|a??t", 5, "asdf", 4, NULL));
    CHECK_EXPR(!RTStrSimplePatternMultiMatch("asdq|a*f|a??t", 4, "asdf", 4, NULL));
    CHECK_EXPR(!RTStrSimplePatternMultiMatch("asdq|a*f|a??t", 3, "asdf", 4, NULL));
    CHECK_EXPR(RTStrSimplePatternMultiMatch("asdf", 4, "asdf", 4, NULL));
    CHECK_EXPR(RTStrSimplePatternMultiMatch("asdf|", 5, "asdf", 4, NULL));


    /*
     * Summary.
     */
    if (!cErrors)
        RTPrintf("tstStrToNum: SUCCESS\n");
    else
        RTPrintf("tstStrToNum: FAILURE - %d errors\n", cErrors);
    return !!cErrors;
}

