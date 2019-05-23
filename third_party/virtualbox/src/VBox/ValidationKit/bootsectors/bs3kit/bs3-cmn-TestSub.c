/* $Id: bs3-cmn-TestSub.c $ */
/** @file
 * BS3Kit - Bs3TestSub, Bs3TestSubF, Bs3TestSubV.
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



/**
 * Equivalent to RTTestISubV.
 */
#undef Bs3TestSubV
BS3_CMN_DEF(void, Bs3TestSubV,(const char *pszFormat, va_list BS3_FAR va))
{
    size_t cch;

    /*
     * Cleanup any previous sub-test.
     */
    bs3TestSubCleanup();

    /*
     * Format the sub-test name and update globals.
     */
    cch = Bs3StrPrintfV(g_szBs3SubTest, sizeof(g_szBs3SubTest), pszFormat, va);
    g_cusBs3SubTestAtErrors = g_cusBs3TestErrors;
    BS3_ASSERT(!g_fbBs3SubTestSkipped);
    g_cusBs3SubTests++;

    /*
     * Tell VMMDev and output to the console.
     */
    bs3TestSendCmdWithStr(VMMDEV_TESTING_CMD_SUB_NEW, g_szBs3SubTest);

    Bs3PrintStr(g_szBs3SubTest);
    Bs3PrintChr(':');
    do
       Bs3PrintChr(' ');
    while (cch++ < 49);
    Bs3PrintStr(" TESTING\n");

    /* The sub-test result is not yet reported. */
    g_fbBs3SubTestReported = false;
}


/**
 * Equivalent to RTTestIFailedF.
 */
#undef Bs3TestSubF
BS3_CMN_DEF(void, Bs3TestSubF,(const char *pszFormat, ...))
{
    va_list va;
    va_start(va, pszFormat);
    BS3_CMN_NM(Bs3TestSubV)(pszFormat, va);
    va_end(va);
}


/**
 * Equivalent to RTTestISub.
 */
#undef Bs3TestSub
BS3_CMN_DEF(void, Bs3TestSub,(const char *pszMessage))
{
    BS3_CMN_NM(Bs3TestSubF)("%s", pszMessage);
}

