/* $Id: bs3-cmn-test.h $ */
/** @file
 * BS3Kit - Bs3Test internal header.
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

#ifndef ___bs3_cmn_test_h
#define ___bs3_cmn_test_h

#include "bs3kit.h"
#include <VBox/VMMDevTesting.h>


/** Indicates whether the VMMDev is operational. */
#ifndef DOXYGEN_RUNNING
# define g_fbBs3VMMDevTesting BS3_DATA_NM(g_fbBs3VMMDevTesting)
#endif
extern bool                 g_fbBs3VMMDevTesting;

/** The number of tests that have failed. */
#ifndef DOXYGEN_RUNNING
# define g_cusBs3TestErrors BS3_DATA_NM(g_cusBs3TestErrors)
#endif
extern uint16_t             g_cusBs3TestErrors;

/** The start error count of the current subtest. */
#ifndef DOXYGEN_RUNNING
# define g_cusBs3SubTestAtErrors BS3_DATA_NM(g_cusBs3SubTestAtErrors)
#endif
extern uint16_t             g_cusBs3SubTestAtErrors;

/**  Whether we've reported the sub-test result or not. */
#ifndef DOXYGEN_RUNNING
# define g_fbBs3SubTestReported BS3_DATA_NM(g_fbBs3SubTestReported)
#endif
extern bool                 g_fbBs3SubTestReported;
/** Whether the sub-test has been skipped or not. */
#ifndef DOXYGEN_RUNNING
# define g_fbBs3SubTestSkipped BS3_DATA_NM(g_fbBs3SubTestSkipped)
#endif
extern bool                 g_fbBs3SubTestSkipped;

/** The number of sub tests. */
#ifndef DOXYGEN_RUNNING
# define g_cusBs3SubTests   BS3_DATA_NM(g_cusBs3SubTests)
#endif
extern uint16_t             g_cusBs3SubTests;

/** The number of sub tests that failed. */
#ifndef DOXYGEN_RUNNING
# define g_cusBs3SubTestsFailed BS3_DATA_NM(g_cusBs3SubTestsFailed)
#endif
extern uint16_t             g_cusBs3SubTestsFailed;

/** VMMDEV_TESTING_UNIT_XXX -> string */
#ifndef DOXYGEN_RUNNING
# define g_aszBs3TestUnitNames BS3_DATA_NM(g_aszBs3TestUnitNames)
#endif
extern char const           g_aszBs3TestUnitNames[][16];

/** The test name. */
extern const char BS3_FAR  *g_pszBs3Test_c16;
extern const char          *g_pszBs3Test_c32;
extern const char          *g_pszBs3Test_c64;

/** The subtest name. */
#ifndef DOXYGEN_RUNNING
# define g_szBs3SubTest     BS3_DATA_NM(g_szBs3SubTest)
#endif
extern char                 g_szBs3SubTest[64];


/**
 * Sends a command to VMMDev followed by a single string.
 *
 * If the VMMDev is not present or is not being used, this function will
 * do nothing.
 *
 * @param   uCmd        The command.
 * @param   pszString   The string.
 */
#ifndef DOXYGEN_RUNNING
# define bs3TestSendCmdWithStr BS3_CMN_NM(bs3TestSendCmdWithStr)
#endif
BS3_DECL(void) bs3TestSendCmdWithStr(uint32_t uCmd, const char BS3_FAR *pszString);

/**
 * Sends a command to VMMDev followed by a 32-bit unsigned integer value.
 *
 * If the VMMDev is not present or is not being used, this function will
 * do nothing.
 *
 * @param   uCmd        The command.
 * @param   uValue      The value.
 */
#ifndef DOXYGEN_RUNNING
# define bs3TestSendCmdWithU32 BS3_CMN_NM(bs3TestSendCmdWithU32)
#endif
BS3_DECL(void) bs3TestSendCmdWithU32(uint32_t uCmd, uint32_t uValue);

/**
 * Checks if the VMMDev is configured for testing.
 *
 * @returns true / false.
 */
#ifndef DOXYGEN_RUNNING
# define bs3TestIsVmmDevTestingPresent BS3_CMN_NM(bs3TestIsVmmDevTestingPresent)
#endif
BS3_DECL(bool) bs3TestIsVmmDevTestingPresent(void);

/**
 * Similar to rtTestSubCleanup.
 */
#ifndef DOXYGEN_RUNNING
# define bs3TestSubCleanup BS3_CMN_NM(bs3TestSubCleanup)
#endif
BS3_DECL(void) bs3TestSubCleanup(void);

/**
 * @callback_method_impl{FNBS3STRFORMATOUTPUT,
 *      Used by Bs3TestFailedV and Bs3TestSkippedV.
 *
 *      The @a pvUser parameter must point a BS3TESTFAILEDBUF structure. }
 */
#ifndef DOXYGEN_RUNNING
# define bs3TestFailedStrOutput BS3_CMN_NM(bs3TestFailedStrOutput)
#endif
BS3_DECL_CALLBACK(size_t) bs3TestFailedStrOutput(char ch, void BS3_FAR *pvUser);

/**
 * Output buffering for bs3TestFailedStrOutput.
 */
typedef struct BS3TESTFAILEDBUF
{
    /** Initialize to false. */
    bool    fNewLine;
    /** Initialize to zero. */
    uint8_t cchBuf;
    /** Buffer, uninitialized. */
    char    achBuf[128];
} BS3TESTFAILEDBUF;
/** Pointer to a bs3TestFailedStrOutput buffer.  */
typedef BS3TESTFAILEDBUF BS3_FAR *PBS3TESTFAILEDBUF;

#endif

