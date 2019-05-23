/* $Id: bs3-cmn-TestData.c $ */
/** @file
 * BS3Kit - Test Data.
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
#include "bs3kit-template-header.h"
#include "bs3-cmn-test.h"


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
#if ARCH_BITS == 16

/** Indicates whether the VMMDev is operational. */
bool        g_fbBs3VMMDevTesting = true;

/** Alignment padding.    */
bool        g_fTestDataPadding0 = true;

/** The number of tests that have failed. */
uint16_t    g_cusBs3TestErrors = 0;

/** The start error count of the current subtest. */
uint16_t    g_cusBs3SubTestAtErrors = 0;

/** Whether we've reported the sub-test result or not. */
bool        g_fbBs3SubTestReported = true;
/** Whether the sub-test has been skipped or not. */
bool        g_fbBs3SubTestSkipped = false;

/** The number of sub tests. */
uint16_t    g_cusBs3SubTests = 0;

/** The number of sub tests that failed. */
uint16_t    g_cusBs3SubTestsFailed = 0;

/** VMMDEV_TESTING_UNIT_XXX -> string */
char const  g_aszBs3TestUnitNames[][16] =
{
    "inv",
    "%",
    "bytes",
    "bytes/s",
    "KB",
    "KB/s",
    "MB",
    "MB/s",
    "packets",
    "packets/s",
    "frames",
    "frames/",
    "occ",
    "occ/s",
    "rndtrp",
    "calls",
    "calls/s",
    "s",
    "ms",
    "ns",
    "ns/call",
    "ns/frame",
    "ns/occ",
    "ns/packet",
    "ns/rndtrp",
    "ins",
    "ins/s",
    "", /* none */
    "pp1k",
    "pp10k",
    "ppm",
    "ppb",
};


/** The subtest name. */
char        g_szBs3SubTest[64];

/** The current test step. */
uint16_t    g_usBs3TestStep;

#endif /* ARCH_BITS == 16 */

/** The test name. */
const char BS3_FAR *BS3_CMN_NM(g_pszBs3Test) = NULL;

